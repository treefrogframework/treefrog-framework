#pragma once
#include <QByteArray>
#include <QDateTime>
#include <TGlobal>


class T_CORE_EXPORT TAccessLog {
public:
    TAccessLog();
    TAccessLog(const QByteArray &remoteHost, const QByteArray &request);
    QByteArray toByteArray(const QByteArray &layout, const QByteArray &dateTimeFormat) const;

    QDateTime timestamp;
    QByteArray remoteHost;
    QByteArray request;
    int statusCode {0};
    int responseBytes {0};
};


class T_CORE_EXPORT TAccessLogger {
public:
    TAccessLogger();
    TAccessLogger(const TAccessLogger &other);
    ~TAccessLogger();
    TAccessLogger &operator=(const TAccessLogger &other);

    void open();
    void write();
    void close();
    void setTimestamp(const QDateTime &timestamp)
    {
        if (accessLog) {
            accessLog->timestamp = timestamp;
        }
    }
    void setRemoteHost(const QByteArray &host)
    {
        if (accessLog) {
            accessLog->remoteHost = host;
        }
    }
    void setRequest(const QByteArray &request)
    {
        if (accessLog) {
            accessLog->request = request;
        }
    }
    int statusCode() const { return (accessLog) ? accessLog->statusCode : -1; }
    void setStatusCode(int statusCode)
    {
        if (accessLog) {
            accessLog->statusCode = statusCode;
        }
    }
    int responseBytes() const { return (accessLog) ? accessLog->responseBytes : -1; }
    void setResponseBytes(int bytes)
    {
        if (accessLog) {
            accessLog->responseBytes = bytes;
        }
    }

private:
    TAccessLog *accessLog {nullptr};
};
