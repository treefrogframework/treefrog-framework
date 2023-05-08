#pragma once
#include <QByteArray>
#include <QCoreApplication>
#include <QDataStream>
#include <QDateTime>
#include <TGlobal>


class T_CORE_EXPORT TLog {
public:
    TLog() { }
    TLog(int pri, const QByteArray &msg, int dur = 0);

    friend QDataStream &operator<<(QDataStream &out, const TLog &log);
    friend QDataStream &operator>>(QDataStream &in, TLog &log);

    QDateTime timestamp;  //!< Timestamp.
    int priority {0};  //!< Priority. @sa enum TLogger::Priority
    qulonglong pid {0};  //!< PID.
    qulonglong threadId {0};  //!< Thread ID.
    QByteArray message;  //!< Message.
    int duration {0};  //!< Duration (msecs).
};
