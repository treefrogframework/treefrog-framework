/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tabstractlogstream.h"
#include <TLogger>
#include <TSystemGlobal>

/*!
  \class TAbstractLogStream
  \brief The TAbstractLogStream class is the abstract base class of
  log stream, providing functionality common to log stream.
*/


TAbstractLogStream::TAbstractLogStream(const QList<TLogger *> &loggers, QObject *parent) :
    QObject(parent),
    loggerList(loggers),
    nonBuffering(false)
{
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(setNonBufferingMode()));
}


bool TAbstractLogStream::loggerOpen(LoggerType type)
{
    bool res = true;
    for (auto logger : (const QList<TLogger *> &)loggerList) {
        if (logger) {
            if (type == All
                || (type == MultiProcessSafe && logger->isMultiProcessSafe())
                || (type == MultiProcessUnsafe && !logger->isMultiProcessSafe())) {
                res &= logger->open();
            }
        }
    }
    return res;
}


void TAbstractLogStream::loggerClose(LoggerType type)
{
    for (auto logger : (const QList<TLogger *> &)loggerList) {
        if (logger) {
            if (type == All
                || (type == MultiProcessSafe && logger->isMultiProcessSafe())
                || (type == MultiProcessUnsafe && !logger->isMultiProcessSafe())) {
                logger->close();
            }
        }
    }
}


void TAbstractLogStream::loggerWrite(const TLog &log)
{
    for (auto logger : (const QList<TLogger *> &)loggerList) {
        if (logger && logger->isOpen() && log.priority <= logger->threshold()) {
            logger->log(log);
            if (nonBuffering)
                logger->flush();
        }
    }
}


void TAbstractLogStream::loggerWrite(const QList<TLog> &logs)
{
    for (auto logger : (const QList<TLog> &)logs) {
        loggerWrite(logger);
    }
}


void TAbstractLogStream::loggerFlush()
{
    for (auto logger : (const QList<TLogger *> &)loggerList) {
        if (logger && logger->isOpen())
            logger->flush();
    }
}


void TAbstractLogStream::setNonBufferingMode()
{
    nonBuffering = true;
}
