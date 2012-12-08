#ifndef TACCESSLOG_H
#define TACCESSLOG_H

#include <QDateTime>
#include <QByteArray>
#include <TGlobal>


class T_CORE_EXPORT TAccessLog
{
public:
    TAccessLog();
    TAccessLog(const QByteArray &remoteHost, const QByteArray &request);
    QByteArray toByteArray(const QByteArray &layout, const QByteArray &dateTimeFormat) const;

    QDateTime timestamp;
    QByteArray remoteHost;
    QByteArray request;
    int statusCode;
    int responseBytes;
};

#endif // TACCESSLOG_H
