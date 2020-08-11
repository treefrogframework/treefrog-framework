#pragma once
#include <QMap>
#include <QSettings>
#include <QVariant>
#include <TGlobal>

class TAccessLog;
class QSqlError;

namespace Tf {
T_CORE_EXPORT void setupSystemLogger();  // internal use
T_CORE_EXPORT void releaseSystemLogger();  // internal use
T_CORE_EXPORT void setupAccessLogger();  // internal use
T_CORE_EXPORT void releaseAccessLogger();  // internal use
T_CORE_EXPORT bool isAccessLoggerAvailable();  // internal use
T_CORE_EXPORT void setupQueryLogger();  // internal use
T_CORE_EXPORT void releaseQueryLogger();  // internal use
T_CORE_EXPORT void writeAccessLog(const TAccessLog &log);  // write access log
T_CORE_EXPORT void writeQueryLog(const QString &query, bool success, const QSqlError &error);
T_CORE_EXPORT void traceQueryLog(const char *, ...)  // SQL query log
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__((format(printf, 1, 2)))
#endif
    ;

enum SystemOpCode {
    InvalidOpCode = 0x00,
    WebSocketSendText = 0x01,
    WebSocketSendBinary = 0x02,
    WebSocketPublishText = 0x03,
    WebSocketPublishBinary = 0x04,
    MaxOpCode = 0x04,
};

T_CORE_EXPORT QMap<QString, QVariant> settingsToMap(QSettings &settings, const QString &env = QString());
}

T_CORE_EXPORT void tSystemError(const char *, ...)  // system error message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__((format(printf, 1, 2)))
#endif
    ;

T_CORE_EXPORT void tSystemWarn(const char *, ...)  // system warn message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__((format(printf, 1, 2)))
#endif
    ;

T_CORE_EXPORT void tSystemInfo(const char *, ...)  // system info message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__((format(printf, 1, 2)))
#endif
    ;

T_CORE_EXPORT void tSystemDebug(const char *, ...)  // system debug message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__((format(printf, 1, 2)))
#endif
    ;

T_CORE_EXPORT void tSystemTrace(const char *, ...)  // system trace message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__((format(printf, 1, 2)))
#endif
    ;

