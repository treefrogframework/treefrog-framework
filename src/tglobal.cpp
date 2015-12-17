/* Copyright (c) 2010-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QStringList>
#include <TGlobal>
#include <TWebApplication>
#include <TAppSettings>
#include <TLogger>
#include <TLog>
#include <TActionContext>
#include <TDatabaseContext>
#include <TActionThread>
#include <TApplicationScheduler>
#include <TScheduler>
#ifdef Q_OS_LINUX
# include <TActionWorker>
#endif
#include <cstdlib>
#include <climits>
#include <random>
#include "twebsocketworker.h"
#include "tloggerfactory.h"
#include "tsharedmemorylogstream.h"
#include "tbasiclogstream.h"
#include "tsystemglobal.h"
#ifdef Q_OS_WIN
# include <Windows.h>
#endif
#undef tDebug
#undef tTrace

static TAbstractLogStream *stream = 0;
static QList<TLogger *> loggers;

/*!
  Sets up all the loggers set in the logger.ini.
  This function is for internal use only.
*/
void tSetupAppLoggers()
{
    QStringList loggerList = Tf::app()->loggerSettings().value("Loggers").toString().split(' ', QString::SkipEmptyParts);

    for (QStringListIterator i(loggerList); i.hasNext(); ) {
        TLogger *lgr = TLoggerFactory::create(i.next());
        if (lgr) {
            loggers << lgr;
            tSystemDebug("Logger added: %s", qPrintable(lgr->key()));
        }
    }

    if (!stream) {
        stream = new TBasicLogStream(loggers, qApp);
    }
}

/*!
  Releases all the loggers.
  This function is for internal use only.
*/
void tReleaseAppLoggers()
{
    if (stream) {
        delete stream;
        stream = 0;
    }

    for (QListIterator<TLogger *> it(loggers); it.hasNext(); ) {
        delete it.next();
    }
    loggers.clear();
}


static void tMessage(int priority, const char *msg, va_list ap)
{
    TLog log(priority, QString().vsprintf(msg, ap).toLocal8Bit());
    if (stream)
        stream->writeLog(log);
}


static void tFlushMessage()
{
    if (stream)
        stream->flush();
}

/*!
  Writes the fatal message \a msg to the file app.log.
*/
void tFatal(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    tMessage(TLogger::Fatal, msg, ap);
    va_end(ap);
    tFlushMessage();

    if (Tf::appSettings()->value(Tf::ApplicationAbortOnFatal).toBool()) {
#if (defined(Q_OS_UNIX) || defined(Q_CC_MINGW))
        abort(); // trap; generates core dump
#else
        _exit(-1);
#endif
    }
}

/*!
  Writes the error message \a msg to the file app.log.
*/
void tError(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    tMessage(TLogger::Error, msg, ap);
    va_end(ap);
    tFlushMessage();
}

/*!
  Writes the warning message \a msg to the file app.log.
*/
void tWarn(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    tMessage(TLogger::Warn, msg, ap);
    va_end(ap);
    tFlushMessage();
}

/*!
  Writes the information message \a msg to the file app.log.
*/
void tInfo(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    tMessage(TLogger::Info, msg, ap);
    va_end(ap);
}

/*!
  Writes the debug message \a msg to the file app.log.
*/
void tDebug(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    tMessage(TLogger::Debug, msg, ap);
    va_end(ap);
}

/*!
  Writes the trace message \a msg to the file app.log.
*/
void tTrace(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    tMessage(TLogger::Trace, msg, ap);
    va_end(ap);
}

/*!
  Returns a global pointer referring to the unique application object.
*/
TWebApplication *Tf::app()
{
    return static_cast<TWebApplication *>(qApp);
}

/*!
  Returns a global pointer referring to the unique application settings object.
*/
TAppSettings *Tf::appSettings()
{
    return TAppSettings::instance();
}

/*!
  Causes the current thread to sleep for \a msecs milliseconds.
*/
void Tf::msleep(unsigned long msecs)
{
#if defined(Q_OS_WIN)
    ::Sleep(msecs);
#else
    struct timeval tv;
    gettimeofday(&tv, 0);
    struct timespec ti;
    ti.tv_nsec = (tv.tv_usec + (msecs % 1000) * 1000) * 1000;
    ti.tv_sec = tv.tv_sec + (msecs / 1000) + (ti.tv_nsec / 1000000000);
    ti.tv_nsec %= 1000000000;

    pthread_mutex_t mtx;
    pthread_cond_t cnd;

    pthread_mutex_init(&mtx, 0);
    pthread_cond_init(&cnd, 0);

    pthread_mutex_lock(&mtx);
    pthread_cond_timedwait(&cnd, &mtx, &ti);
    pthread_mutex_unlock(&mtx);

    pthread_cond_destroy(&cnd);
    pthread_mutex_destroy(&mtx);
#endif
}

/*
  Xorshift random number generator implement
*/
static QMutex randMutex;
static quint32 x = 123456789;
static quint32 y = 362436069;
static quint32 z = 987654321;
static quint32 w = 1;

/*!
  Sets the argument \a seed to be used to generate a new random number sequence
  of xorshift random integers to be returned by randXor128().
  This function is thread-safe.
*/
void Tf::srandXor128(quint32 seed)
{
    randMutex.lock();
    w = seed;
    z = w ^ (w >> 8) ^ (w << 5);
    randMutex.unlock();
}

/*!
  Returns a value between 0 and UINT_MAX, the next number in the current
  sequence of xorshift random integers.
  This function is thread-safe.
*/
quint32 Tf::randXor128()
{
    QMutexLocker lock(&randMutex);
    quint32 t;
    t = x ^ (x << 11);
    x = y;
    y = z;
    z = w;
    w = w ^ (w >> 19) ^ (t ^ (t >> 8));
    return w;
}

static std::random_device randev;
static std::mt19937     mt(randev());
static std::mt19937_64  mt64(randev());

uint32_t Tf::rand32_r()
{
    randMutex.lock();
    uint32_t ret = mt();
    randMutex.unlock();
    return ret;
}


uint64_t Tf::rand64_r()
{
    randMutex.lock();
    uint64_t ret = mt64();
    randMutex.unlock();
    return ret;
}

/*!
  Random number generator in the range from \a min to \a max.
*/
uint64_t Tf::random(uint64_t min, uint64_t max)
{
    randMutex.lock();
    std::uniform_int_distribution<uint64_t> uniform(min, max);
    uint64_t ret = uniform(mt64);
    randMutex.unlock();
    return ret;
}

/*!
  Random number generator in the range from 0 to \a max.
*/
uint64_t Tf::random(uint64_t max)
{
    return random(0, max);
}


TActionContext *Tf::currentContext()
{
    TActionContext *context = nullptr;

    switch ( Tf::app()->multiProcessingModule() ) {
    case TWebApplication::Thread:
        context = qobject_cast<TActionThread *>(QThread::currentThread());
        if (Q_LIKELY(context))
            return context;
        break;

    case TWebApplication::Hybrid:
#ifdef Q_OS_LINUX
        context = qobject_cast<TActionWorker *>(QThread::currentThread());
        if (Q_LIKELY(context))
            return context;
        break;
#else
        tFatal("Unsupported MPM: hybrid");
#endif
        break;

    default:
        break;
    }

    throw RuntimeException("Can not cast the current thread", __FILE__, __LINE__);
}


TDatabaseContext *Tf::currentDatabaseContext()
{
    TDatabaseContext *context = nullptr;

    switch ( Tf::app()->multiProcessingModule() ) {
    case TWebApplication::Thread:
        context = qobject_cast<TActionThread *>(QThread::currentThread());
        if (Q_LIKELY(context))
            return context;
        break;

    case TWebApplication::Hybrid:
#ifdef Q_OS_LINUX
        context = qobject_cast<TActionWorker *>(QThread::currentThread());
        if (Q_LIKELY(context))
            return context;
        break;
#else
        tFatal("Unsupported MPM: hybrid");
#endif
        break;

    default:
        break;
    }

    // TWebSocketWorker
    context = qobject_cast<TWebSocketWorker *>(QThread::currentThread());
    if (context)
        return context;

    // TApplicationScheduler
    context = qobject_cast<TApplicationScheduler *>(QThread::currentThread());
    if (context)
        return context;

    // TScheduler
    context = qobject_cast<TScheduler *>(QThread::currentThread());
    if (context)
        return context;

    throw RuntimeException("Can not cast the current thread", __FILE__, __LINE__);
}


QSqlDatabase &Tf::currentSqlDatabase(int id)
{
    return currentDatabaseContext()->getSqlDatabase(id);
}

/*!
  Returns the current datetime in the local time zone.
  It provides 1-second accuracy.
*/
/*
QDateTime Tf::currentDateTimeSec()
{
    // QDateTime::currentDateTime() is slow.
    // Faster function..

    QDateTime current;

#if defined(Q_OS_WIN)
    SYSTEMTIME st;
    memset(&st, 0, sizeof(SYSTEMTIME));
    GetLocalTime(&st);
    current.setDate(QDate(st.wYear, st.wMonth, st.wDay));
    current.setTime(QTime(st.wHour, st.wMinute, st.wSecond));
#elif defined(Q_OS_UNIX)
    time_t ltime = 0;
    tm *t = 0;
    time(&ltime);
# if defined(_POSIX_THREAD_SAFE_FUNCTIONS)
    tzset();
    tm res;
    t = localtime_r(&ltime, &res);
# else
    t = localtime(&ltime);
# endif // _POSIX_THREAD_SAFE_FUNCTIONS
    current.setDate(QDate(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday));
    current.setTime(QTime(t->tm_hour, t->tm_min, t->tm_sec));
#endif // Q_OS_UNIX

    return current;
}
*/
