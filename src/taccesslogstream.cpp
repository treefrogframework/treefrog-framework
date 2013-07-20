/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QSystemSemaphore>
#include <QFileInfo>
#include <TWebApplication>
#include <TSystemGlobal>
#include "tfilelogger.h"
#include "taccesslogstream.h"

/*!
  \class TAccessLogStream
  \brief The TAccessLogStream class provides a stream for access log.
*/


TAccessLogStream::TAccessLogStream(const QString &fileName)
    : logger(new TFileLogger), semaphore(0)
{
    logger->setFileName(fileName);

    if (Tf::app()->multiProcessingModule() == TWebApplication::Prefork) {
        semaphore = new QSystemSemaphore(QLatin1String("TreeFrog_") + QFileInfo(fileName).fileName(), 1, QSystemSemaphore::Open);
    } else {
        logger->open();
    }
}


TAccessLogStream::~TAccessLogStream()
{
    delete logger;
    if (semaphore)
        delete semaphore;
}


void TAccessLogStream::writeLog(const QByteArray &log)
{
    if (semaphore) {
        semaphore->acquire();
        logger->open();
    }

    logger->log(log);
    logger->flush();

    if (semaphore) {
        logger->close();
        semaphore->release();
    }
}
