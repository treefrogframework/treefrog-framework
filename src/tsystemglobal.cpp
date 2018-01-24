/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <TWebApplication>
#include <TAppSettings>
#include <TLogger>
#include <TLog>
#include <TAccessLog>
#include "tsystemglobal.h"
#include "taccesslogstream.h"
#include "tfileaiowriter.h"

#define DEFAULT_SYSTEMLOG_LAYOUT           "%d %5P %m%n"
#define DEFAULT_SYSTEMLOG_DATETIME_FORMAT  "yyyy-MM-ddThh:mm:ss"
#define DEFAULT_ACCESSLOG_LAYOUT           "%h %d \"%r\" %s %O%n"
#define DEFAULT_ACCESSLOG_DATETIME_FORMAT  "yyyy-MM-ddThh:mm:ss"

static TAccessLogStream *accesslogstrm = nullptr;
static TAccessLogStream *sqllogstrm = nullptr;
static TFileAioWriter systemLog;
static QByteArray syslogLayout = DEFAULT_SYSTEMLOG_LAYOUT;
static QByteArray syslogDateTimeFormat = DEFAULT_SYSTEMLOG_DATETIME_FORMAT;
static QByteArray accessLogLayout = DEFAULT_ACCESSLOG_LAYOUT;
static QByteArray accessLogDateTimeFormat;


void Tf::writeAccessLog(const TAccessLog &log)
{
    if (accesslogstrm) {
        accesslogstrm->writeLog(log.toByteArray(accessLogLayout, accessLogDateTimeFormat));
    }
}


void Tf::setupSystemLogger()
{
    // Log directory
    QDir logdir(Tf::app()->logPath());
    if (!logdir.exists()) {
        logdir.mkpath(".");
    }

    // system log
    systemLog.setFileName(Tf::app()->systemLogFilePath());
    systemLog.open();

    syslogLayout = Tf::appSettings()->value(Tf::SystemLogLayout, DEFAULT_SYSTEMLOG_LAYOUT).toByteArray();
    syslogDateTimeFormat = Tf::appSettings()->value(Tf::SystemLogDateTimeFormat, DEFAULT_SYSTEMLOG_DATETIME_FORMAT).toByteArray();
}


void Tf::releaseSystemLogger()
{
    systemLog.close();
}


void Tf::setupAccessLogger()
{
    // access log
    QString accesslogpath = Tf::app()->accessLogFilePath();
    if (!accesslogstrm && !accesslogpath.isEmpty()) {
        accesslogstrm = new TAccessLogStream(accesslogpath);
    }

    accessLogLayout = Tf::appSettings()->value(Tf::AccessLogLayout, DEFAULT_ACCESSLOG_LAYOUT).toByteArray();
    accessLogDateTimeFormat = Tf::appSettings()->value(Tf::AccessLogDateTimeFormat, DEFAULT_ACCESSLOG_DATETIME_FORMAT).toByteArray();
}


void Tf::releaseAccessLogger()
{
    delete accesslogstrm;
    accesslogstrm = nullptr;
}


void Tf::setupQueryLogger()
{
    // sql query log
    QString querylogpath = Tf::app()->sqlQueryLogFilePath();
    if (!sqllogstrm && !querylogpath.isEmpty()) {
        sqllogstrm = new TAccessLogStream(querylogpath);
    }
}


void Tf::releaseQueryLogger()
{
    delete sqllogstrm;
    sqllogstrm = nullptr;
}


static void tSystemMessage(int priority, const char *msg, va_list ap)
{
    TLog log(priority, QString().vsprintf(msg, ap).toLocal8Bit());
    QByteArray buf = TLogger::logToByteArray(log, syslogLayout, syslogDateTimeFormat);
    systemLog.write(buf.data(), buf.length());
}


void tSystemError(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    tSystemMessage(Tf::ErrorLevel, msg, ap);
    va_end(ap);
}


void tSystemWarn(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    tSystemMessage(Tf::WarnLevel, msg, ap);
    va_end(ap);
}


void tSystemInfo(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    tSystemMessage(Tf::InfoLevel, msg, ap);
    va_end(ap);
}

#ifndef TF_NO_DEBUG

void tSystemDebug(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    tSystemMessage(Tf::DebugLevel, msg, ap);
    va_end(ap);
}


void tSystemTrace(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    tSystemMessage(Tf::TraceLevel, msg, ap);
    va_end(ap);
}

#else

void tSystemDebug(const char *, ...) { }
void tSystemTrace(const char *, ...) { }

#endif


void Tf::traceQueryLog(const char *msg, ...)
{
    if (sqllogstrm) {
        va_list ap;
        va_start(ap, msg);
        TLog log(-1, QString().vsprintf(msg, ap).toLocal8Bit());
        QByteArray buf = TLogger::logToByteArray(log, syslogLayout, syslogDateTimeFormat);
        sqllogstrm->writeLog(buf);
        va_end(ap);
    }
}


void Tf::writeQueryLog(const QString &query, bool success, const QSqlError &error)
{
    QString q = query;

    if (!success) {
        QString err = (!error.databaseText().isEmpty()) ? error.databaseText() : error.text().trimmed();
        if (!err.isEmpty()) {
            err = QLatin1Char('[') + err + QLatin1String("] ");
        }
        q = QLatin1String("(Query failed) ") + err + query;
    }
    Tf::traceQueryLog("%s", qPrintable(q));
}
