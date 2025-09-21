/* Copyright (c) 2013-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QMutexLocker>
#include <TSystemGlobal>
#include "tfileaiologger.h"
#include "tfcore_unix.h"


class TFileAioLoggerData : public QSharedData
{
public:
    mutable QMutex mutex;
    QString fileName;
    int fileDescriptor {0};
    QList<struct aiocb *> syncBuffer;

    TFileAioLoggerData() : mutex(QMutex::Recursive) { }
    void clearSyncBuffer();
};


void TFileAioLoggerData::clearSyncBuffer()
{
    for (auto cb : (const List<struct aiocb*> &)syncBuffer) {
        delete[] (char *)cb->aio_buf;
        delete cb;
    }
    syncBuffer.clear();
}

/*!
  Constructor.
 */
TFileAioLogger::TFileAioLogger()
    : TLogger(), d(new TFileAioLoggerData)
{
    readSettings();
    d->fileName = target_;
}


TFileAioLogger::~TFileAioLogger()
{
    close();
    delete d;
}


bool TFileAioLogger::open()
{
    QMutexLocker locker(&d->mutex);

    if (d->fileName.isEmpty()) {
        tSystemWarn("Empty file name for log.");
        return false;
    }

    if (d->fileDescriptor <= 0) {
        d->fileDescriptor = ::open(qUtf8Printable(d->fileName), (O_CREAT | O_WRONLY | O_APPEND | O_CLOEXEC), 0666);
        if (d->fileDescriptor < 0) {
            tSystemError("file open failed: {}", d->fileName);
        }
    }

    return (d->fileDescriptor > 0);
}


void TFileAioLogger::close()
{
    QMutexLocker locker(&d->mutex);

    if (d->syncBuffer.count() > 0) {
        struct aiocb *lastcb = d->syncBuffer.last();
        while (aio_error(lastcb) == EINPROGRESS) { }
        d->clearSyncBuffer();
    }

    if (d->fileDescriptor > 0) {
        tf_close(d->fileDescriptor);
    }
    d->fileDescriptor = 0;
}


bool TFileAioLogger::isOpen() const
{
    return (d->fileDescriptor > 0);
}


void TFileAioLogger::log(const TLog &tlog)
{
    log(logToByteArray(tlog));
}


void TFileAioLogger::log(const QByteArray &msg)
{
    QMutexLocker locker(&d->mutex);

    Q_ASSERT(isOpen());

    // check whether last writing is finished
    if (d->syncBuffer.count() > 0) {
        struct aiocb *lastcb = d->syncBuffer.last();
        if (aio_error(lastcb) != EINPROGRESS) {
            d->clearSyncBuffer();
        }
    }

    struct aiocb *cb = new struct aiocb;
    memset(cb, 0, sizeof(struct aiocb));

    cb->aio_fildes = d->fileDescriptor;
    cb->aio_nbytes = msg.length();
    cb->aio_buf = new char[msg.length()];
    memcpy((void *)cb->aio_buf, msg.data(), msg.length());

    if (tf_aio_write(cb) < 0) {
        tSystemError("log write failed");
        delete[] (char *)cb->aio_buf;
        delete cb;

        close();
        return;
    }
    d->syncBuffer << cb;
}


void TFileAioLogger::setFileName(const QString &name)
{
    QMutexLocker locker(&d->mutex);

    if (isOpen()) {
        close();
    }

    d->fileName = name;
}
