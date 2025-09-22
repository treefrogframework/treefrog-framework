/* Copyright (c) 2017-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tbasiclogstream.h"
#include "tloggerfactory.h"
#include "tsystemglobal.h"
#include <TDebug>
#include <TfCore>
#include <TAppSettings>
#include <TLog>
#include <TLogger>

#undef tFatal
#undef tError
#undef tWarn
#undef tInfo
#undef tDebug
#undef tTrace

/*!
  \class TDebug
  \brief The TDebug class provides a file output stream for debugging information.
*/

namespace {

TAbstractLogStream *stream = nullptr;
QList<TLogger *> loggers;
QTimer *flushTimer = nullptr;

/*!
  Flushes all the loggers.
*/
void flushAppLoggers()
{
    if (stream) {
        stream->flush();
    }
}

}

/*!
  Sets up all the loggers set in the logger.ini.
  This function is for internal use only.
*/
void Tf::setupAppLoggers(TLogger *logger)
{
    if (stream) {
        return;
    }

    if (logger) {
        loggers << logger;
    } else {
        const QStringList loggerList = Tf::app()->loggerSettings().value("Loggers").toString().split(' ', Tf::SkipEmptyParts);

        for (auto &lg : loggerList) {
            TLogger *lgr = TLoggerFactory::create(lg);
            if (lgr) {
                loggers << lgr;
                tSystemDebug("Logger added: {}", lgr->key());
            }
        }
    }

    if (loggers.isEmpty()) {
        return;
    }

    stream = new TBasicLogStream(loggers);

    // Starts flash timer for appliation logger
    flushTimer = new QTimer();
    QObject::connect(flushTimer, &QTimer::timeout, flushAppLoggers);
    flushTimer->start(2000);  // 2 seconds
}

/*!
  Releases all the loggers.
  This function is for internal use only.
*/
void Tf::releaseAppLoggers()
{
    delete stream;
    stream = nullptr;

    for (auto *logger : (const QList<TLogger *> &)loggers) {
        delete logger;
    }
    loggers.clear();
}


void Tf::logging(int priority, const QByteArray &msg)
{
    if (stream) {
        TLog log(priority, msg);
        stream->writeLog(log);
    }
}


static void tLogging(int priority, const char *msg, va_list ap)
{
    if (stream) {
        TLog log(priority, QString::vasprintf(msg, ap).toLocal8Bit());
        stream->writeLog(log);
    }
}


TDebug::~TDebug()
{
    ts.flush();
    if (!buffer.isNull()) {
        TLog log(msgPriority, buffer.toLocal8Bit());
        if (stream) {
            stream->writeLog(log);
        }
    }
}


TDebug::TDebug(const TDebug &other) :
    buffer(other.buffer), ts(&buffer, QIODevice::WriteOnly), msgPriority(other.msgPriority)
{
}


TDebug &TDebug::operator=(const TDebug &other)
{
    buffer = other.buffer;
    ts.setString(&buffer, QIODevice::WriteOnly);
    msgPriority = other.msgPriority;
    return *this;
}

/*!
  Writes the fatal message \a fmt to the file app.log.
*/
void TDebug::fatal(const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    tLogging(Tf::FatalLevel, fmt, ap);
    va_end(ap);
    flushAppLoggers();

    if (Tf::appSettings()->value(Tf::ApplicationAbortOnFatal).toBool()) {
#if (defined(Q_OS_UNIX) || defined(Q_CC_MINGW))
        abort();  // trap; generates core dump
#else
        _exit(-1);
#endif
    }
}

/*!
  Writes the error message \a fmt to the file app.log.
*/
void TDebug::error(const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    tLogging(Tf::ErrorLevel, fmt, ap);
    va_end(ap);
    flushAppLoggers();
}

/*!
  Writes the warning message \a fmt to the file app.log.
*/
void TDebug::warn(const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    tLogging(Tf::WarnLevel, fmt, ap);
    va_end(ap);
    flushAppLoggers();
}

/*!
  Writes the information message \a fmt to the file app.log.
*/
void TDebug::info(const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    tLogging(Tf::InfoLevel, fmt, ap);
    va_end(ap);
}

/*!
  Writes the debug message \a fmt to the file app.log.
*/
void TDebug::debug(const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    tLogging(Tf::DebugLevel, fmt, ap);
    va_end(ap);
}

/*!
  Writes the trace message \a fmt to the file app.log.
*/
void TDebug::trace(const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    tLogging(Tf::TraceLevel, fmt, ap);
    va_end(ap);
}
