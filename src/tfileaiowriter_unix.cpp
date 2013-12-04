/* Copyright (c) 2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QList>
#include <QMutexLocker>
#include <QSharedData>
#include "tfileaiowriter.h"
#include "tfcore_unix.h"


class TFileAioWriterData : public QSharedData
{
public:
    mutable QMutex mutex;
    QString fileName;
    int fileDescriptor;
    QList<struct aiocb *> syncBuffer;

    TFileAioWriterData() : mutex(QMutex::Recursive), fileName(), fileDescriptor(0), syncBuffer() { }
    void clearSyncBuffer();
};


void TFileAioWriterData::clearSyncBuffer()
{
    for (QListIterator<struct aiocb *> it(syncBuffer); it.hasNext(); ) {
        struct aiocb *cb = it.next();
        delete (char *)cb->aio_buf;
        delete cb;
    }
    syncBuffer.clear();
}

/*!
  Constructor.
 */
TFileAioWriter::TFileAioWriter(const QString &name)
    : d(new TFileAioWriterData)
{
    d->fileName = name;
}


TFileAioWriter::~TFileAioWriter()
{
    close();
    delete d;
}


bool TFileAioWriter::open()
{
    QMutexLocker locker(&d->mutex);

    if (d->fileDescriptor <= 0) {
        if (d->fileName.isEmpty())
            return false;

        d->fileDescriptor = ::open(qPrintable(d->fileName), (O_CREAT | O_WRONLY | O_APPEND | O_CLOEXEC), 0666);
        if (d->fileDescriptor < 0) {
            //tSystemError("file open failed: %s", qPrintable(d->fileName));
        }
    }

    return (d->fileDescriptor > 0);
}


void TFileAioWriter::close()
{
    QMutexLocker locker(&d->mutex);

    flush();

    if (d->fileDescriptor > 0) {
        TF_CLOSE(d->fileDescriptor);
    }
    d->fileDescriptor = 0;
}


bool TFileAioWriter::isOpen() const
{
    return (d->fileDescriptor > 0);
}


int TFileAioWriter::write(const char *data, int length)
{
    QMutexLocker locker(&d->mutex);

    if (!isOpen()) {
        return -1;
    }

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
    cb->aio_nbytes = length;
    cb->aio_buf = new char[length];
    memcpy((void *)cb->aio_buf, data, length);

    int ret = tf_aio_write(cb);
    if (ret < 0) {
        delete (char *)cb->aio_buf;
        delete cb;

        close();
        return -1;
    }

    d->syncBuffer << cb;
    return ret;
}


void TFileAioWriter::flush()
{
    QMutexLocker locker(&d->mutex);

    if (d->syncBuffer.count() > 0) {
        struct aiocb *lastcb = d->syncBuffer.last();
        while (aio_error(lastcb) == EINPROGRESS) { }
        d->clearSyncBuffer();
    }
}

void TFileAioWriter::setFileName(const QString &name)
{
    QMutexLocker locker(&d->mutex);

    if (isOpen()) {
        close();
    }
    d->fileName = name;
}


QString TFileAioWriter::fileName() const
{
    QMutexLocker locker(&d->mutex);
    return d->fileName;
}
