/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QStringList>
#include <QAtomicPointer>
#include <TGlobal>
#include <TWebApplication>
#include <TLogger>
#include <TLog>
#include <TActionContext>
#include <TActionThread>
#include <TActionWorker>
#include <TActionForkProcess>
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

/*!
  Sets up all the loggers set in the logger.ini.
  This function is for internal use only.
*/
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
        if (Tf::app()->multiProcessingModule() == TWebApplication::Prefork) {
            stream = new TSharedMemoryLogStream(loggers, 4096, qApp);
        } else {
            stream = new TBasicLogStream(loggers, qApp);
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

    if (Tf::app()->appSettings().value(APPLICATION_ABORT).toBool()) {
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

/*!
  Random number generator in the range from 0 to \a max.
  The maximum number of \a max is UINT_MAX.
 */
quint32 Tf::random(quint32 max)
{
    return (quint32)((double)randXor128() * (1.0 + max) / (1.0 + UINT_MAX));
}


/*!
 * Randomizes the random function anew by applying a new seed. Should be called occasionally to receive
 * less predictable numbers, but not for every call. Only usable with C++11 or newer.
 */
void Tf::randomize()
{
//#if __cplusplus >= 201103L
    static std::random_device randdev{};
    mt_gen.seed(randdev());
//#endif
}

/*!
 * Generates a 32bit random number with the PRNG algorithm Mersenne Twister. At first use you should set
 * doSeed so a seed is set. Don't use this method without a seed. Only usable with C++11 or newer.
 * \param doSeed Will set a new seed via randomize() if true. Default is false.
 * \return Returns a PRN as an unsigned int32
 */
quint32 Tf::random_mt(bool doSeed)
{
//#if __cplusplus >= 201103L
    if (doSeed)
        randomize();

    return((quint32) mt_gen());
//#endif
//    return(0);
}


/*
  Xorshift random number generator implement
*/
struct Rand
{
    quint32 x;
    quint32 y;
    quint32 z;
    quint32 w;
};

static QAtomicPointer<Rand> randNumber;


/*!
  Sets the argument \a seed to be used to generate a new random number sequence
  of xorshift random integers to be returned by randXor128().
  This function is thread-safe.
*/
void Tf::srandXor128(quint32 seed)
{
    // initial numbers
    Rand *r = new Rand;
    r->x = 123456789;
    r->y = 362436169;
    r->z = 777777777;
    r->w = seed;

    Rand *oldr = randNumber.fetchAndStoreOrdered(r);
    if (oldr)
        delete oldr;
}

/*!
  Returns a value between 0 and UINT_MAX, the next number in the current
  sequence of xorshift random integers.
  This function is thread-safe.
*/
quint32 Tf::randXor128()
{
    Rand *newr = new Rand;
    Rand tmp;
    quint32 t;

    for (;;) {
        Rand *oldr = randNumber.fetchAndAddOrdered(0);
        memcpy(&tmp, oldr, sizeof(tmp));
        t = tmp.x ^ (tmp.x << 11);
        newr->x = tmp.y;
        newr->y = tmp.z;
        newr->z = tmp.w;
        newr->w = tmp.w ^ (tmp.w >> 19) ^ (t ^ (t >> 8));

        if (randNumber.testAndSetOrdered(oldr, newr)) {
            delete oldr;
            break;
        }
    }
    return newr->w;
}


TActionContext *Tf::currentContext()
{
    TActionContext *context = 0;

    switch ( Tf::app()->multiProcessingModule() ) {
    case TWebApplication::Prefork:
        context = TActionForkProcess::currentContext();
        if (!context) {
            throw RuntimeException("The current process is not TActionProcess", __FILE__, __LINE__);
        }
        break;

    case TWebApplication::Thread:
        context = qobject_cast<TActionThread *>(QThread::currentThread());
        if (!context) {
            throw RuntimeException("The current thread is not TActionThread", __FILE__, __LINE__);
        }
        break;

    case TWebApplication::Hybrid:
#ifdef Q_OS_LINUX
        context = qobject_cast<TActionWorker *>(QThread::currentThread());
        if (!context) {
            context = qobject_cast<TActionThread *>(QThread::currentThread());
            if (!context) {
                throw RuntimeException("The current thread is not TActionContext", __FILE__, __LINE__);
            }
        }
#else
        tFatal("Unsupported MPM: hybrid");
#endif
        break;

    default:
        break;
    }
    return context;
}


QSqlDatabase &Tf::currentSqlDatabase(int id)
{
    return currentContext()->getSqlDatabase(id);
}
