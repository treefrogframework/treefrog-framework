/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tfilelogger.h"
#include <QMutexLocker>
#include <TSystemGlobal>

/*!
  \class TFileLogger
  \brief The TFileLogger class provides logging functionality to a log file.
*/

TFileLogger::TFileLogger() :
    TLogger()
{
    logFile.setFileName(target());
}


TFileLogger::~TFileLogger()
{
    close();
}


bool TFileLogger::open()
{
    QMutexLocker locker(&mutex);

    if (logFile.fileName().isEmpty()) {
        return false;
    }

    bool res = true;
    if (!logFile.isOpen()) {
        res = logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        if (!res) {
            tSystemError("file open failed: %s", qUtf8Printable(logFile.fileName()));
        }
    }
    return res;
}


void TFileLogger::close()
{
    QMutexLocker locker(&mutex);

    if (logFile.isOpen()) {
        logFile.flush();
    }
    logFile.close();
}


bool TFileLogger::isOpen() const
{
    return logFile.isOpen();
}


void TFileLogger::log(const TLog &tlog)
{
    log(logToByteArray(tlog));
}


void TFileLogger::log(const QByteArray &msg)
{
    if (!isOpen()) {
        return;
    }

    QMutexLocker locker(&mutex);

    int len = logFile.write(msg);
    if (len < 0) {
        tSystemError("log write failed");
        return;
    }
    Q_ASSERT(len == msg.length());
}


void TFileLogger::flush()
{
    QMutexLocker locker(&mutex);

    if (logFile.isOpen()) {
        logFile.flush();
    }
}


void TFileLogger::setFileName(const QString &name)
{
    if (isOpen()) {
        close();
    }

    QMutexLocker locker(&mutex);
    logFile.setFileName(name);
}
