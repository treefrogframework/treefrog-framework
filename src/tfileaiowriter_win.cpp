/* Copyright (c) 2013-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tfileaiowriter.h"
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <windows.h>


typedef struct
{
    char *aio_buf;
    int aio_nbytes;
    OVERLAPPED aio_overlap;
} aiobuf_t;


class TFileAioWriterData
{
public:
    mutable QMutex mutex;
    QString fileName;
    HANDLE fileHandle;
    QList<aiobuf_t *> syncBuffer;

    TFileAioWriterData() : mutex(QMutex::Recursive), fileName(), fileHandle(INVALID_HANDLE_VALUE), syncBuffer() { }
    void clearSyncBuffer();
};


void TFileAioWriterData::clearSyncBuffer()
{
    for (QListIterator<aiobuf_t *> it(syncBuffer); it.hasNext(); ) {
        aiobuf_t *ab = it.next();
        delete (char *)ab->aio_buf;
        delete ab;
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

    if (d->fileHandle == INVALID_HANDLE_VALUE) {
        if (d->fileName.isEmpty())
            return false;

        d->fileHandle = CreateFile((const wchar_t*)d->fileName.utf16(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
        if (d->fileHandle == INVALID_HANDLE_VALUE) {
            //fprintf(stderr, "file open failed: %s\n", qPrintable(d->fileName));
        }
    }

    return (d->fileHandle != INVALID_HANDLE_VALUE);
}


void TFileAioWriter::close()
{
    QMutexLocker locker(&d->mutex);

    flush();

    if (d->fileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(d->fileHandle);
    }
    d->fileHandle = INVALID_HANDLE_VALUE;
}


bool TFileAioWriter::isOpen() const
{
    return (d->fileHandle != INVALID_HANDLE_VALUE);
}


int TFileAioWriter::write(const char *data, int length)
{
    QMutexLocker locker(&d->mutex);

    if (!isOpen()) {
        return -1;
    }

    // check whether last writing is finished
    if (d->syncBuffer.count() > 0) {
        aiobuf_t *lastab = d->syncBuffer.last();
        if (HasOverlappedIoCompleted(&lastab->aio_overlap)) {
            d->clearSyncBuffer();
        }
    }

    if (length <= 0)
        return -1;

    int len = length;
    if (data[length - 1] == '\n') {
        ++len;
    }

    aiobuf_t *ab = new aiobuf_t;
    memset(ab, 0, sizeof(aiobuf_t));

    ab->aio_nbytes = len;
    ab->aio_buf = new char[len];
    ab->aio_overlap.Offset = 0xFFFFFFFF;
    ab->aio_overlap.OffsetHigh = 0xFFFFFFFF;
    memcpy((void *)ab->aio_buf, data, length);

    // the last char only LF -> CRLF
    if (len != length) {
        ab->aio_buf[len - 2] = '\r';
        ab->aio_buf[len - 1] = '\n';
    }

    WriteFile(d->fileHandle, ab->aio_buf, (DWORD)len, NULL, &ab->aio_overlap);
    if (GetLastError() != ERROR_IO_PENDING) {
        //fprintf(stderr, "WriteFile error str: %s\n", data);
        delete (char *)ab->aio_buf;
        delete ab;

        close();
        return -1;
    }

    d->syncBuffer << ab;
    return 0;
}


void TFileAioWriter::flush()
{
    QMutexLocker locker(&d->mutex);

    FlushFileBuffers(d->fileHandle);
    d->clearSyncBuffer();
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
