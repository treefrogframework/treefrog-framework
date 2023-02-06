/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsystemglobal.h"
#include "taccesslogstream.h"
#include "tfilesystemlogger.h"
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

namespace {
TAccessLogStream *accesslogstrm = nullptr;
TAccessLogStream *sqllogstrm = nullptr;
TSystemLogger *systemLogger = nullptr;
QByteArray syslogLayout;
QByteArray syslogDateTimeFormat;
QByteArray accessLogLayout;
QByteArray accessLogDateTimeFormat;
QByteArray queryLogLayout;
QByteArray queryLogDateTimeFormat;


void tSystemMessage(int priority, const char *msg, va_list ap)
{
    if (!systemLogger) {
        return;
    }

    TLog log(priority, QString::vasprintf(msg, ap).toLocal8Bit());
    QByteArray buf = TLogger::logToByteArray(log, syslogLayout, syslogDateTimeFormat);
    systemLogger->write(buf.data(), buf.length());
}
}


void Tf::writeAccessLog(const TAccessLog &log)
{
    if (accesslogstrm) {
        accesslogstrm->writeLog(log.toByteArray(accessLogLayout, accessLogDateTimeFormat));
    }
}


void Tf::setupSystemLogger(TSystemLogger *logger)
{
    if (systemLogger) {
        return;
    }

    // Log directory
    QDir logdir(Tf::app()->logPath());
    if (!logdir.exists()) {
        logdir.mkpath(".");
    }

    // system logger
    systemLogger = (logger) ? logger : new TFileSystemLogger(Tf::app()->systemLogFilePath());
    systemLogger->open();

    syslogLayout = Tf::appSettings()->value(Tf::SystemLogLayout).toByteArray();
    syslogDateTimeFormat = Tf::appSettings()->value(Tf::SystemLogDateTimeFormat).toByteArray();
}


void Tf::releaseSystemLogger()
{
    if (systemLogger) {
        systemLogger->close();
        delete systemLogger;
    }
    systemLogger = nullptr;
}


void Tf::setupAccessLogger()
{
    // access log
    QString accesslogpath = Tf::app()->accessLogFilePath();
    if (!accesslogstrm && !accesslogpath.isEmpty()) {
        accesslogstrm = new TAccessLogStream(accesslogpath);
    }

    accessLogLayout = Tf::appSettings()->value(Tf::AccessLogLayout).toByteArray();
    accessLogDateTimeFormat = Tf::appSettings()->value(Tf::AccessLogDateTimeFormat).toByteArray();
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

    queryLogLayout = Tf::appSettings()->value(Tf::SqlQueryLogLayout).toByteArray();
    queryLogDateTimeFormat = Tf::appSettings()->value(Tf::SqlQueryLogDateTimeFormat).toByteArray();
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


void Tf::traceQueryLog(int duration, const char *msg, ...)
{
    if (sqllogstrm) {
        va_list ap;
        va_start(ap, msg);
        TLog log(-1, QString::vasprintf(msg, ap).toLocal8Bit(), duration);
        QByteArray buf = TLogger::logToByteArray(log, queryLogLayout, queryLogDateTimeFormat);
        sqllogstrm->writeLog(buf);
        va_end(ap);
    }
}


void Tf::writeQueryLog(const QString &query, bool success, const QSqlError &error, int duration)
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
        Tf::traceQueryLog(duration, "%s", qUtf8Printable(q));
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
