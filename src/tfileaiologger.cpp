/* Copyright (c) 2013-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tfileaiologger.h"
#include "tfileaiowriter.h"

/*!
  \class TFileAioLogger
  \brief The TFileAioLogger class provides asynchronous logging functionality
  to a log file.
*/

/*!
  Constructor.
 */
TFileAioLogger::TFileAioLogger() :
    TLogger(),
    writer(new TFileAioWriter)
{
    writer->setFileName(target());
}


TFileAioLogger::~TFileAioLogger()
{
    delete writer;
}


bool TFileAioLogger::open()
{
    return writer->open();
}


void TFileAioLogger::close()
{
    writer->close();
}


void TFileAioLogger::log(const TLog &tlog)
{
    log(logToByteArray(tlog));
}


void TFileAioLogger::log(const QByteArray &msg)
{
    writer->write(msg.data(), msg.length());
}


bool TFileAioLogger::isOpen() const
{
    return writer->isOpen();
}

void TFileAioLogger::flush()
{
    // do nothing
}


void TFileAioLogger::setFileName(const QString &name)
{
    writer->setFileName(name);
}
