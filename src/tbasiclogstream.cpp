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


TBasicLogStream::TBasicLogStream(const QList<TLogger *> loggers) :
    TAbstractLogStream(loggers)
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
}


void TBasicLogStream::flush()
{
    QMutexLocker locker(&mutex);
    loggerFlush();
}
