/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tbasiclogstream.h"
#include <QMutexLocker>
#include <QThread>
#include <TSystemGlobal>

/*!
  \class TBasicLogStream
  \brief The TBasicLogStream class provides a basic stream for logs.
*/


TBasicLogStream::TBasicLogStream(const QList<TLogger *> loggers, QObject *parent) :
    TAbstractLogStream(loggers, parent)
{
    loggerOpen();
}


TBasicLogStream::~TBasicLogStream()
{
    flush();
}


void TBasicLogStream::writeLog(const TLog &log)
{
    QMutexLocker locker(&mutex);
    loggerWrite(log);

    if (!isNonBufferingMode()) {
        if (thread() == QThread::currentThread()) {
            if (!timer.isActive()) {
                timer.start(200, this);
            }
        } else {
            // timers cannot be started from another thread
            loggerFlush();
        }
    }
}


void TBasicLogStream::flush()
{
    QMutexLocker locker(&mutex);
    loggerFlush();
}


void TBasicLogStream::setNonBufferingMode()
{
    QMutexLocker locker(&mutex);
    TAbstractLogStream::setNonBufferingMode();
}


void TBasicLogStream::timerEvent(QTimerEvent *event)
{
    QMutexLocker locker(&mutex);
    if (event->timerId() != timer.timerId()) {
        QObject::timerEvent(event);
        return;
    }

    timer.stop();
    loggerFlush();  // Flush logger
}
