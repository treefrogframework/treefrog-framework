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
}

/*!
  Sets up all the loggers set in the logger.ini.
  This function is for internal use only.
*/
void Tf::setupAppLoggers()
{
    const QStringList loggerList = Tf::app()->loggerSettings().value("Loggers").toString().split(' ', Tf::SkipEmptyParts);

    for (auto &lg : loggerList) {
        TLogger *lgr = TLoggerFactory::create(lg);
        if (lgr) {
            loggers << lgr;
            tSystemDebug("Logger added: %s", qUtf8Printable(lgr->key()));
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
void Tf::releaseAppLoggers()
{
    delete stream;
    stream = nullptr;

    for (auto &logger : (const QList<TLogger *> &)loggers) {
        delete logger;
    }
    loggers.clear();
}


static void tMessage(int priority, const char *msg, va_list ap)
{
    if (stream) {
        TLog log(priority, QString::vasprintf(msg, ap).toLocal8Bit());
        stream->writeLog(log);
    }
}


static void tFlushMessage()
{
    if (stream) {
        stream->flush();
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
    tMessage(Tf::FatalLevel, fmt, ap);
    va_end(ap);
    tFlushMessage();

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
    tMessage(Tf::ErrorLevel, fmt, ap);
    va_end(ap);
    tFlushMessage();
}

/*!
  Writes the warning message \a fmt to the file app.log.
*/
void TDebug::warn(const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    tMessage(Tf::WarnLevel, fmt, ap);
    va_end(ap);
    tFlushMessage();
}

/*!
  Writes the information message \a fmt to the file app.log.
*/
void TDebug::info(const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    tMessage(Tf::InfoLevel, fmt, ap);
    va_end(ap);
}

/*!
  Writes the debug message \a fmt to the file app.log.
*/
void TDebug::debug(const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    tMessage(Tf::DebugLevel, fmt, ap);
    va_end(ap);
}

/*!
  Writes the trace message \a fmt to the file app.log.
*/
void TDebug::trace(const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    tMessage(Tf::TraceLevel, fmt, ap);
    va_end(ap);
}
