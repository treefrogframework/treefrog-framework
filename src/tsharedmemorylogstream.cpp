/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsharedmemorylogstream.h"
#include <QSharedMemory>
#include <TSystemGlobal>

constexpr auto CREATE_KEY = "TreeFrogLogStream";


class TSharedMemoryLocker {
public:
    TSharedMemoryLocker(QSharedMemory *memory) :
        sm(memory) { sm->lock(); }
    ~TSharedMemoryLocker() { sm->unlock(); }

private:
    QSharedMemory *sm;
};


TSharedMemoryLogStream::TSharedMemoryLogStream(const QList<TLogger *> loggers, int size, QObject *parent) :
    TAbstractLogStream(loggers, parent),
    shareMem(new QSharedMemory(CREATE_KEY))
{
    if (size < dataSizeOf(QList<TLog>())) {
        tSystemError("Shared memory size not enough: %lld (bytes)", shareMem->size());
        return;
    }

    if (shareMem->create(size)) {
        TSharedMemoryLocker locker(shareMem);
        clearBuffer();
    } else {
        if (shareMem->error() != QSharedMemory::AlreadyExists) {
            tSystemError("Shared memory create error: %s", qUtf8Printable(shareMem->errorString()));
        } else {
            if (!shareMem->attach()) {
                tSystemError("Shared memory attach error: %s", qUtf8Printable(shareMem->errorString()));
            }
        }
    }
}


TSharedMemoryLogStream::~TSharedMemoryLogStream()
{
    flush();
    delete shareMem;
}


void TSharedMemoryLogStream::writeLog(const TLog &log)
{
    TSharedMemoryLocker locker(shareMem);
    if (isNonBufferingMode()) {
        QList<TLog> logs;
        logs << log;
        loggerWriteLog(logs);
        return;
    }

    QList<TLog> logList = smRead();
    logList << log;
    if (smWrite(logList)) {
        if (!timer.isActive())
            timer.start(200, this);
    } else {
        loggerWriteLog(logList);
        clearBuffer();
    }
}


void TSharedMemoryLogStream::flush()
{
    if (isNonBufferingMode())
        return;

    TSharedMemoryLocker locker(shareMem);
    loggerWriteLog(smRead());
    clearBuffer();
}


QString TSharedMemoryLogStream::errorString() const
{
    return shareMem->errorString();
}


void TSharedMemoryLogStream::loggerWriteLog(const QList<TLog> &logs)
{
    loggerOpen();
    loggerWrite(logs);
    loggerFlush();
    loggerClose(MultiProcessUnsafe);
}


void TSharedMemoryLogStream::clearBuffer()
{
    smWrite(QList<TLog>());
    timer.stop();
}


void TSharedMemoryLogStream::setNonBufferingMode()
{
    tSystemDebug("TSharedMemoryLogStream::setNonBufferingMode()");
    if (!isNonBufferingMode()) {
        flush();
        shareMem->detach();
    }
    TAbstractLogStream::setNonBufferingMode();
}


QList<TLog> TSharedMemoryLogStream::smRead()
{
    QList<TLog> logs;
    if (!shareMem->data()) {
        tSystemError("Shared memory not attached");
        return logs;
    }

    QByteArray buffer((char *)shareMem->data(), shareMem->size());
    QDataStream ds(&buffer, QIODevice::ReadOnly);
    ds >> logs;
    if (ds.status() != QDataStream::Ok) {
        tSystemError("Shared memory read error");
        clearBuffer();
        return QList<TLog>();
    }
    return logs;
}


bool TSharedMemoryLogStream::smWrite(const QList<TLog> &logs)
{
    QByteArray buffer;
    QDataStream ds(&buffer, QIODevice::WriteOnly);
    ds << logs;

    if (buffer.size() > shareMem->size()) {
        // over the shared memory size
        return false;
    }

    if (!shareMem->data()) {
        tSystemError("Shared memory not attached");
        return false;
    }

    memcpy(shareMem->data(), buffer.constData(), buffer.size());
    return true;
}


int TSharedMemoryLogStream::dataSizeOf(const QList<TLog> &logs)
{
    QByteArray ba;
    QDataStream ds(&ba, QIODevice::WriteOnly);
    ds << logs;
    return ba.size();
}


void TSharedMemoryLogStream::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != timer.timerId()) {
        QObject::timerEvent(event);
        return;
    }

    flush();
    timer.stop();
}
