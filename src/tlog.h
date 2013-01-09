#ifndef TLOG_H
#define TLOG_H

#include <QCoreApplication>
#include <QDateTime>
#include <QByteArray>
#include <QDataStream>
#include <TGlobal>


class T_CORE_EXPORT TLog
{
public:
    TLog() { }
    TLog(int pri, const QByteArray &msg);

    friend QDataStream &operator<<(QDataStream &out, const TLog &log);
    friend QDataStream &operator>>(QDataStream &in, TLog &log);
    
    QDateTime timestamp;  //!< Timestamp.
    int priority;         //!< Priority. @sa enum TLogger::Priority
    qint64 pid;           //!< PID.
    qulonglong threadId;  //!< Thread ID.
    QByteArray message;   //!< Message.
};

#endif // TLOG_H
