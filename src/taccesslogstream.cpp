/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "taccesslogstream.h"
#include "tfilelogger.h"

/*!
  \class TAccessLogStream
  \brief The TAccessLogStream class provides a stream for access log.
*/


TAccessLogStream::TAccessLogStream(const QString &fileName)
{
    TFileLogger *fileLogger = new TFileLogger;
    fileLogger->setFileName(fileName);
    fileLogger->open();
    logger = fileLogger;
}


TAccessLogStream::~TAccessLogStream()
{
    logger->flush();
    delete logger;
}


void TAccessLogStream::writeLog(const QByteArray &log)
{
    if (logger->isOpen()) {
        logger->log(log);
    }
}


void TAccessLogStream::flush()
{
    logger->flush();
}
