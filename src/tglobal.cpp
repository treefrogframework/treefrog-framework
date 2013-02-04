/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QStringList>
#include <TGlobal>
#include <TWebApplication>
#include <TLogger>
#include <TLog>
#include <stdlib.h>
#include <limits.h>
#include "tloggerfactory.h"
#include "tsharedmemorylogstream.h"
#include "tbasiclogstream.h"
#include "tsystemglobal.h"
#ifdef Q_OS_WIN
# include <Windows.h>
#endif
#undef tDebug
#undef tTrace
#define APPLICATION_ABORT  "ApplicationAbortOnFatal"

static TAbstractLogStream *stream = 0;


void tSetupLoggers()
{    
    QList<TLogger *> loggers;
    QStringList loggerList = Tf::app()->loggerSettings().value("Loggers").toString().split(' ', QString::SkipEmptyParts);
    
    for (QStringListIterator i(loggerList); i.hasNext(); ) {
        TLogger *lgr = TLoggerFactory::create(i.next());
        if (lgr) {
            loggers << lgr;
            tSystemDebug("Logger added: %s", qPrintable(lgr->key()));
        }
    }
    
    if (!stream) {
        if (Tf::app()->multiProcessingModule() == TWebApplication::Thread) {
            stream = new TBasicLogStream(loggers, qApp);
        } else {
            stream = new TSharedMemoryLogStream(loggers, 4096, qApp);
        }
    }
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


void tFatal(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    tMessage(TLogger::Fatal, msg, ap);
    va_end(ap);
    tFlushMessage();

    if (Tf::app()->appSettings().value(APPLICATION_ABORT).toBool()) {
#if (defined(Q_OS_UNIX) || defined(Q_CC_MINGW))
        abort(); // trap; generates core dump
#else
        _exit(-1);
#endif
    }
}


void tError(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    tMessage(TLogger::Error, msg, ap);
    va_end(ap);
    tFlushMessage();
}


void tWarn(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    tMessage(TLogger::Warn, msg, ap);
    va_end(ap);
    tFlushMessage();
}


void tInfo(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    tMessage(TLogger::Info, msg, ap);
    va_end(ap);
}


void tDebug(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    tMessage(TLogger::Debug, msg, ap);
    va_end(ap);
}


void tTrace(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    tMessage(TLogger::Trace, msg, ap);
    va_end(ap);
}


TWebApplication *Tf::app()
{
    return static_cast<TWebApplication *>(qApp);
}


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

/*!
  Random number generator in the range from 0 to \a max.
  The maximum number of \a max is UINT_MAX.
 */
quint32 Tf::random(quint32 max)
{
    return (quint32)((double)randXor128() * (1.0 + max) / (1.0 + UINT_MAX));
}

/*
  Xorshift random number generator implement
*/
static QMutex randMutex;
static quint32 x = 123456789;
static quint32 y = 362436169;
static quint32 z = 1943734098;
static quint32 w = 1;


void Tf::srandXor128(quint32 seed)
{
    QMutexLocker lock(&randMutex);
    w = seed;
}


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
