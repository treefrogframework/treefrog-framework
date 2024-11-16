#pragma once
#include <QMap>
#include <QSettings>
#include <QVariant>
#include <TGlobal>
#if __cplusplus >= 202002L  // C++20
#include <format>
#endif

class TSystemLogger;
class TAccessLog;
class QSqlError;

namespace Tf {
T_CORE_EXPORT void setupSystemLogger(TSystemLogger *logger = nullptr);  // internal use
T_CORE_EXPORT void releaseSystemLogger();  // internal use
T_CORE_EXPORT void tSystemMessage(int priority, const std::string &message);  // internal use
T_CORE_EXPORT void setupAccessLogger();  // internal use
T_CORE_EXPORT void releaseAccessLogger();  // internal use
T_CORE_EXPORT bool isAccessLoggerAvailable();  // internal use
T_CORE_EXPORT void setupQueryLogger();  // internal use
T_CORE_EXPORT void releaseQueryLogger();  // internal use
T_CORE_EXPORT void writeAccessLog(const TAccessLog &log);  // write access log
T_CORE_EXPORT void writeQueryLog(const QString &query, bool success, const QSqlError &error, int duration);
T_CORE_EXPORT void traceQuery(int duration, const std::string &msg);

#if __cplusplus >= 202002L  // C++20

template<typename... Args>
void traceQueryLog(int duration, const std::format_string<Args...> &fmt, Args&&... args)
{
    auto msg = std::format(fmt, std::forward<Args>(args)...);
    traceQuery(duration, msg);
}

#endif

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

#if __cplusplus >= 202002L  // C++20

template<typename... Args>
void tSystemError(const std::format_string<Args...> &fmt, Args&&... args)
{
    std::string msg = std::format(fmt, std::forward<Args>(args)...);
    Tf::tSystemMessage((int)Tf::ErrorLevel, msg);
}

template<typename... Args>
void tSystemWarn(const std::format_string<Args...> &fmt, Args&&... args)
{
    std::string msg = std::format(fmt, std::forward<Args>(args)...);
    Tf::tSystemMessage((int)Tf::WarnLevel, msg);
}

template<typename... Args>
void tSystemInfo(const std::format_string<Args...> &fmt, Args&&... args)
{
    auto msg = std::format(fmt, std::forward<Args>(args)...);
    Tf::tSystemMessage((int)Tf::InfoLevel, msg);
}

#endif

#if !defined(TF_NO_DEBUG) && __cplusplus >= 202002L  // C++20

template<typename... Args>
void tSystemDebug(const std::format_string<Args...> &fmt, Args&&... args)
{
    auto msg = std::format(fmt, std::forward<Args>(args)...);
    Tf::tSystemMessage((int)Tf::DebugLevel, msg);
}

template<typename... Args>
void tSystemTrace(const std::format_string<Args...> &fmt, Args&&... args)
{
    auto msg = std::format(fmt, std::forward<Args>(args)...);
    Tf::tSystemMessage((int)Tf::TraceLevel, msg);
}

#else

template<typename... Args>
void tSystemDebug(const std::format_string<Args...> &, Args&&...)
{
}

template<typename... Args>
void tSystemTrace(const std::format_string<Args...> &, Args&&...)
{
}

#endif
