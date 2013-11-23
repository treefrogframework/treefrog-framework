/* Copyright (c) 2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QMutexLocker>
#include <TSystemGlobal>
#include "tfileaiologger.h"
#include "tfcore_unix.h"

/*!
  \class TFileAioLogger
  \brief The TFileAioLogger class provides asynchronous logging functionality
  to a log file.
*/

TFileAioLogger::TFileAioLogger()
    : TLogger(), mutex(QMutex::Recursive), fileName(), fileDescriptor(0), syncBuffer()
{
    readSettings();
    fileName = target_;
}


TFileAioLogger::~TFileAioLogger()
{
    close();
}


bool TFileAioLogger::open()
{
    QMutexLocker locker(&mutex);

    if (fileDescriptor <= 0) {
        fileDescriptor = ::open(qPrintable(fileName), (O_CREAT | O_WRONLY | O_APPEND | O_CLOEXEC), 0666);
        if (fileDescriptor < 0) {
            tSystemError("file open failed: %s", qPrintable(fileName));
        }
    }

    return (fileDescriptor > 0);
}


void TFileAioLogger::close()
{
    QMutexLocker locker(&mutex);

    if (syncBuffer.count() > 0) {
        struct aiocb *lastcb = syncBuffer.last();
        while (aio_error(lastcb) == EINPROGRESS) { }
        clearSyncBuffer();
    }

    if (fileDescriptor > 0) {
        tf_close(fileDescriptor);
    }
    fileDescriptor = 0;
}


bool TFileAioLogger::isOpen() const
{
    return (fileDescriptor > 0);
}


void TFileAioLogger::log(const TLog &tlog)
{
    log(logToByteArray(tlog));
}


void TFileAioLogger::log(const QByteArray &msg)
{
    QMutexLocker locker(&mutex);

    Q_ASSERT(isOpen());

    // check whether last writing is finished
    if (syncBuffer.count() > 0) {
        struct aiocb *lastcb = syncBuffer.last();
        if (aio_error(lastcb) != EINPROGRESS) {
            clearSyncBuffer();
        }
    }

    struct aiocb *cb = new struct aiocb;
    memset(cb, 0, sizeof(struct aiocb));

    cb->aio_fildes = fileDescriptor;
    cb->aio_nbytes = msg.length();
    cb->aio_buf = new char[msg.length()];
    memcpy((void *)cb->aio_buf, msg.data(), msg.length());

    if (tf_aio_write(cb) < 0) {
        tSystemError("log write failed");
        delete (char *)cb->aio_buf;
        delete cb;

        close();
        return;
    }
    syncBuffer << cb;
}


void TFileAioLogger::flush()
{ }


void TFileAioLogger::setFileName(const QString &name)
{
    QMutexLocker locker(&mutex);

    if (isOpen()) {
        close();
    }

    fileName = name;
}


void TFileAioLogger::clearSyncBuffer()
{
    for (QListIterator<struct aiocb *> it(syncBuffer); it.hasNext(); ) {
        struct aiocb *cb = it.next();
        delete (char *)cb->aio_buf;
        delete cb;
    }
    syncBuffer.clear();
}
