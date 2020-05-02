/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "TLog"
#include "tfcore.h"
#include <QThread>

/*!
  \class TLog
  \brief The TLog class contains log messages for web application.
*/

/*!
  \fn TLog::TLog()
  Constructor.
*/

/*!
  Constructor.
*/
TLog::TLog(int pri, const QByteArray &msg) :
    timestamp(QDateTime::currentDateTime()),
    priority(pri),
    pid(QCoreApplication::applicationPid()),
#ifdef Q_OS_UNIX
    threadId(tf_gettid()),
#else
    threadId((qulonglong)QThread::currentThreadId()),
#endif
    message(msg)
{
}


QDataStream &operator<<(QDataStream &out, const TLog &log)
{
    out << log.timestamp << log.priority << log.pid << log.threadId << log.message;
    return out;
}


QDataStream &operator>>(QDataStream &in, TLog &log)
{
    in >> log.timestamp >> log.priority >> log.pid >> log.threadId >> log.message;
    return in;
}
