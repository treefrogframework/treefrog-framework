/* Copyright (c) 2010-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TSystemGlobal>
#include <TLogger>
#include "tabstractlogstream.h"

/*!
  \class TAbstractLogStream
  \brief The TAbstractLogStream class is the abstract base class of
  log stream, providing functionality common to log stream.
*/


TAbstractLogStream::TAbstractLogStream(const QList<TLogger *> &loggers, QObject *parent)
    : QObject(parent), loggerList(loggers), nonBuffering(false)
{
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(setNonBufferingMode()));
}


bool TAbstractLogStream::loggerOpen(LoggerType type)
{
    bool res = true;
    for (QListIterator<TLogger *> i(loggerList); i.hasNext(); ) {
        TLogger *logger = i.next();
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
    for (QListIterator<TLogger *> i(loggerList); i.hasNext(); ) {
        TLogger *logger = i.next();
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
    for (QListIterator<TLogger *> i(loggerList); i.hasNext(); ) {
        TLogger *logger = i.next();
        if (logger && logger->isOpen() && log.priority <= logger->threshold()) {
            logger->log(log);
            if (nonBuffering)
                logger->flush();
        }
    }
}


void TAbstractLogStream::loggerWrite(const QList<TLog> &logs)
{
    for (QListIterator<TLog> i(logs); i.hasNext(); ) {
        loggerWrite(i.next());
    }
}


void TAbstractLogStream::loggerFlush()
{
    for (QListIterator<TLogger *> i(loggerList); i.hasNext(); ) {
        TLogger *logger = i.next();
        if (logger && logger->isOpen())
            logger->flush();
    }
}


void TAbstractLogStream::setNonBufferingMode()
{
    nonBuffering = true;
}
