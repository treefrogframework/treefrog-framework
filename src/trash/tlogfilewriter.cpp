/* Copyright (c) 2010, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tlogfilewriter.h"


TLogFileWriter::TLogFileWriter(const QString &logFileName)
    : logFile(logFileName)
{
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        fprintf(stderr, "file open failed: %s\n", qUtf8Printable(logFile.fileName()));
    }
}


TLogFileWriter::~TLogFileWriter()
{
    logFile.close();
}


void TLogFileWriter::writeLog(const char *msg)
{
    QMutexLocker locker(&mutex);
    if (logFile.isOpen()) {
        int len = logFile.write(msg);
        if (len < 0) {
            fprintf(stderr, "log write failed\n");
            return;
        }
        Q_ASSERT(len == (int)strlen(msg));
        logFile.flush();
    }
}
