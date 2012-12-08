/* Copyright (c) 2010, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tbufferedlogstream.h"

const int MAX_LOG_COUNT = 40;


TBufferedLogStream::TBufferedLogStream(const QList<TLogger *> loggers, QObject *parent)
    : QObject(parent),
      TAbstractLogStream(loggers),
      nonBuffering(false)
{
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(setNonBufferingMode()));
}


TBufferedLogStream::~TBufferedLogStream()
{
    flush();
}


void TBufferedLogStream::writeLog(const TLog &log)
{
    QMutexLocker locker(&mutex);
    if (nonBuffering) {
        loggerWrite(log);
        return;
    }

    logList << log;
    if (logList.count() > MAX_LOG_COUNT) {
        loggerWrite(logList);
        logList.clear();
        return;
    }

    if (!timer.isActive())
        timer.start(200, this);
}


void TBufferedLogStream::flush()
{
    QMutexLocker locker(&mutex);
    loggerWrite(logList);
    logList.clear();
}


void TBufferedLogStream::setNonBufferingMode()
{
    if (!nonBuffering) {
        flush();
        nonBuffering = true;
    }
}


void TBufferedLogStream::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != timer.timerId()) {
        QObject::timerEvent(event);
        return;
    }
    
    flush();
    timer.stop();
}
