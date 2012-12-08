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
    
    QDateTime timestamp;
    int priority;
    qint64 pid;
    qulonglong threadId;
    QByteArray message;
};

#endif // TLOG_H
