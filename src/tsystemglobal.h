#ifndef TSYSTEMGLOBAL_H
#define TSYSTEMGLOBAL_H

#include <TGlobal>

#define ENABLE_TO_TRACE_FUNCTION  0

class TAccessLog;
class QSqlError;


T_CORE_EXPORT void tSetupSystemLogger();   // internal use
T_CORE_EXPORT void tReleaseSystemLogger(); // internal use
T_CORE_EXPORT void tSetupAccessLogger();   // internal use
T_CORE_EXPORT void tReleaseAccessLogger(); // internal use
T_CORE_EXPORT void tSetupQueryLogger();    // internal use
T_CORE_EXPORT void tReleaseQueryLogger();  // internal use
T_CORE_EXPORT void writeAccessLog(const TAccessLog &log);  // write access log

T_CORE_EXPORT void tSystemError(const char *, ...) // system error message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

T_CORE_EXPORT void tSystemWarn(const char *, ...) // system warn message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

T_CORE_EXPORT void tSystemInfo(const char *, ...) // system info message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

T_CORE_EXPORT void tSystemDebug(const char *, ...) // system debug message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

T_CORE_EXPORT void tSystemTrace(const char *, ...) // system trace message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

T_CORE_EXPORT void tQueryLog(const char *, ...) // SQL query log
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

T_CORE_EXPORT void tWriteQueryLog(const QString &query, bool success, const QSqlError &error);


namespace Tf
{
enum ServerOpCode {
    WebSocketSendText       = 0x01,
    WebSocketSendBinary     = 0x02,
    WebSocketPublishText    = 0x03,
    WebSocketPublishBinary  = 0x04,
    MaxServerOpCode,
};
}

#if !defined(TF_NO_DEBUG) && ENABLE_TO_TRACE_FUNCTION && !defined(Q_OS_WIN)

class T_CORE_EXPORT TTraceFunc
{
public:
    TTraceFunc(const char *funcname) : functionName(funcname) { }
    ~TTraceFunc() { tSystemTrace("<- Leave %s()", functionName); }

private:
    const char *functionName;
};

# define T_TRACEFUNC(fmt, ...)  TTraceFunc ___tracefunc(__func__);     \
                                tSystemTrace("-> Enter %s() " fmt, __func__, ## __VA_ARGS__)

#else
# define T_TRACEFUNC(fmt, ...)
#endif

#endif // TSYSTEMGLOBAL_H
