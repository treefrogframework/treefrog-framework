#pragma once
#include <QByteArray>
#include <QDateTime>
#include <QElapsedTimer>
#include <TGlobal>
#include <memory>


class T_CORE_EXPORT TAccessLog {
public:
    TAccessLog();
    TAccessLog(const QByteArray &remoteHost, const QByteArray &request, int dur);
    QByteArray toByteArray(const QByteArray &layout, const QByteArray &dateTimeFormat) const;

    QDateTime timestamp;
    QByteArray remoteHost;
    QByteArray request;
    int statusCode {0};
    int responseBytes {0};
    int duration {0};
};


class T_CORE_EXPORT TAccessLogger {
public:
    TAccessLogger();
    TAccessLogger(TAccessLogger &&other) = default;
    TAccessLogger &operator=(TAccessLogger &&other) = default;

    void write();

    void setTimestamp(const QDateTime &timestamp)
    {
        if (_accessLog) {
            _accessLog->timestamp = timestamp;
        }
    }

    void setRemoteHost(const QByteArray &host)
    {
        if (_accessLog) {
            _accessLog->remoteHost = host;
        }
    }

    void setRequest(const QByteArray &request)
    {
        if (_accessLog) {
            _accessLog->request = request;
        }
    }

    inline int statusCode() const { return (_accessLog) ? _accessLog->statusCode : -1; }

    inline void setStatusCode(Tf::StatusCode statusCode)
    {
        setStatusCode((int)statusCode);
    }

    void setStatusCode(int statusCode)
    {
        if (_accessLog) {
            _accessLog->statusCode = statusCode;
        }
    }

    inline int responseBytes() const { return (_accessLog) ? _accessLog->responseBytes : -1; }

    void setResponseBytes(int bytes)
    {
        if (_accessLog) {
            _accessLog->responseBytes = bytes;
        }
    }

    void startElapsedTimer()
    {
        _timer.start();
    }

private:
    std::unique_ptr<TAccessLog> _accessLog;
    QElapsedTimer _timer;

    T_DISABLE_COPY(TAccessLogger)
};
