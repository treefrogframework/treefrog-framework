/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsystemglobal.h"
#include "taccesslogstream.h"
#include "tfileaiowriter.h"
#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QString>
#include <TAccessLog>
#include <TAppSettings>
#include <TLog>
#include <TLogger>
#include <TWebApplication>

constexpr auto DEFAULT_SYSTEMLOG_LAYOUT = "%d %5P %m%n";
constexpr auto DEFAULT_SYSTEMLOG_DATETIME_FORMAT = "yyyy-MM-ddThh:mm:ss";
constexpr auto DEFAULT_ACCESSLOG_LAYOUT = "%h %d \"%r\" %s %O%n";
constexpr auto DEFAULT_ACCESSLOG_DATETIME_FORMAT = "yyyy-MM-ddThh:mm:ss";

namespace {
TAccessLogStream *accesslogstrm = nullptr;
TAccessLogStream *sqllogstrm = nullptr;
TFileAioWriter systemLog;
QByteArray syslogLayout = DEFAULT_SYSTEMLOG_LAYOUT;
QByteArray syslogDateTimeFormat = DEFAULT_SYSTEMLOG_DATETIME_FORMAT;
QByteArray accessLogLayout = DEFAULT_ACCESSLOG_LAYOUT;
QByteArray accessLogDateTimeFormat;


void tSystemMessage(int priority, const char *msg, va_list ap)
{
    TLog log(priority, QString::vasprintf(msg, ap).toLocal8Bit());
    QByteArray buf = TLogger::logToByteArray(log, syslogLayout, syslogDateTimeFormat);
    systemLog.write(buf.data(), buf.length());
}
}


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


bool Tf::isAccessLoggerAvailable()
{
    return (bool)accesslogstrm;
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

void tSystemDebug(const char *, ...)
{
}
void tSystemTrace(const char *, ...) { }

#endif


void Tf::traceQueryLog(const char *msg, ...)
{
    if (sqllogstrm) {
        va_list ap;
        va_start(ap, msg);
        TLog log(-1, QString::vasprintf(msg, ap).toLocal8Bit());
        QByteArray buf = TLogger::logToByteArray(log, syslogLayout, syslogDateTimeFormat);
        sqllogstrm->writeLog(buf);
        va_end(ap);
    }
}


void Tf::writeQueryLog(const QString &query, bool success, const QSqlError &error)
{
    if (sqllogstrm) {
        QString q = query;

        if (!success) {
            QString err = (!error.databaseText().isEmpty()) ? error.databaseText() : error.text().trimmed();
            if (!err.isEmpty()) {
                err = QLatin1Char('[') + err + QLatin1String("] ");
            }
            q = QLatin1String("(Query failed) ") + err + query;
        }
        Tf::traceQueryLog("%s", qUtf8Printable(q));
    }
}


QMap<QString, QVariant> Tf::settingsToMap(QSettings &settings, const QString &env)
{
    // QSettings not thread-safe
    // May crash in endGroup() if used on multi-threading. #215

    QMap<QString, QVariant> map;

    if (!env.isEmpty()) {
        settings.beginGroup(env);
    }
    for (auto &k : settings.allKeys()) {
        map.insert(k, settings.value(k));
    }
    if (!env.isEmpty()) {
        settings.endGroup();
    }
    return map;
}
