/* Copyright (c) 2023, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tfilesystemlogger.h"
#include <QMutexLocker>

/*!
  \class TFileSystemLogger
  \brief The TFileSystemLogger class provides writing functionality to a file.
*/


TFileSystemLogger::TFileSystemLogger(const QString &name)
{
    _logFile.setFileName(name);
}


TFileSystemLogger::~TFileSystemLogger()
{
    close();
}


bool TFileSystemLogger::open()
{
    QMutexLocker locker(&_mutex);

    if (_logFile.fileName().isEmpty()) {
        return false;
    }

    bool res = true;
    if (!_logFile.isOpen()) {
        res = _logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text | QIODevice::Unbuffered);
    }
    return res;
}


void TFileSystemLogger::close()
{
    if (!isOpen()) {
        return;
    }

    QMutexLocker locker(&_mutex);
    _logFile.close();
}


bool TFileSystemLogger::isOpen() const
{
    return _logFile.isOpen();
}


int TFileSystemLogger::write(const char *data, int length)
{
    if (!isOpen()) {
        return -1;
    }

    QMutexLocker locker(&_mutex);
    return _logFile.write(data, length);
}


void TFileSystemLogger::flush()
{}


void TFileSystemLogger::setFileName(const QString &name)
{
    if (isOpen()) {
        close();
    }

    QMutexLocker locker(&_mutex);
    _logFile.setFileName(name);
}
