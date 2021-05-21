/* Copyright (c) 2013-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tfcore_unix.h"
#include "tfileaiowriter.h"
#include "tqueue.h"
#include <QList>
#include <QMutexLocker>

constexpr int MAX_NUM_BUFFERING_DATA = 10000;


class TFileAioWriterData {
public:
#if QT_VERSION < 0x060000
    mutable QMutex mutex {QMutex::Recursive};
#else
    mutable QRecursiveMutex mutex;
#endif
    QString fileName;
    int fileDescriptor {0};
    TQueue<struct aiocb *> syncBuffer;

    TFileAioWriterData() {}
};

/*!
  Constructor.
 */
TFileAioWriter::TFileAioWriter(const QString &name) :
    d(new TFileAioWriterData())
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
        if (d->fileName.isEmpty()) {
            return false;
        }

        d->fileDescriptor = ::open(qUtf8Printable(d->fileName), (O_CREAT | O_WRONLY | O_APPEND | O_CLOEXEC), 0666);
        if (d->fileDescriptor < 0) {
            //fprintf(stderr, "file open failed: %s\n", qUtf8Printable(d->fileName));
        }
    }

    return (d->fileDescriptor > 0);
}


void TFileAioWriter::close()
{
    QMutexLocker locker(&d->mutex);

    flush();

    if (d->fileDescriptor > 0) {
        tf_close(d->fileDescriptor);
    }
    d->fileDescriptor = 0;
}


bool TFileAioWriter::isOpen() const
{
    return (d->fileDescriptor > 0);
}


int TFileAioWriter::write(const char *data, int length)
{
    if (!isOpen()) {
        return -1;
    }

    if (length <= 0) {
        return -1;
    }

    if (d->syncBuffer.count() > 0) {
        if (d->mutex.tryLock()) {
            // check whether head's item  writing is finished
            struct aiocb *headcb;
            while (d->syncBuffer.head(headcb)) {
                if (aio_error(headcb) == EINPROGRESS) {
                    break;
                }

                if (d->syncBuffer.dequeue(headcb)) {
                    delete[](char *) headcb->aio_buf;
                    delete headcb;
                } else {
                    break;
                }
            }
            d->mutex.unlock();
        }

        if (d->syncBuffer.count() > MAX_NUM_BUFFERING_DATA) {
            flush();
        }
    }

    struct aiocb *cb = new struct aiocb;
    memset(cb, 0, sizeof(struct aiocb));

    cb->aio_fildes = d->fileDescriptor;
    cb->aio_nbytes = length;
    cb->aio_buf = new char[length];
    memcpy((void *)cb->aio_buf, data, length);

    int ret = tf_aio_write(cb);
    int err = errno;

    if (ret < 0) {
        //fprintf(stderr, "aio_write error fd:%d (pid:%d tid:%d) ret:%d errno:%d\n", d->fileDescriptor, getpid(), gettid(), ret, err);
        //fprintf(stderr, "aio_write str: %s\n", data);
        delete[](char *) cb->aio_buf;
        delete cb;

        if (err != EAGAIN) {
            close();
        } else {
#ifdef Q_OS_DARWIN
            // try sync-write
            return (tf_write(d->fileDescriptor, data, length) > 0) ? 0 : -1;
#endif
        }
        return ret;
    }

    d->syncBuffer.enqueue(cb);
    return 0;
}


void TFileAioWriter::flush()
{
    if (!isOpen()) {
        return;
    }

    if (d->syncBuffer.count() == 0) {
        return;
    }

    QMutexLocker locker(&d->mutex);
    struct aiocb *headcb;

    while (d->syncBuffer.count() > 0) {
        if (d->syncBuffer.head(headcb) && aio_error(headcb) != EINPROGRESS) {
            // Dequeue
            if (d->syncBuffer.dequeue(headcb)) {
                delete[](char *) headcb->aio_buf;
                delete headcb;
            }
        }
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
