/* Copyright (c) 2014-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TAppSettings>
#include <TSystemGlobal>
#include <QSettings>
#include <QCoreApplication>
#include <QMutexLocker>
#include <cstdio>

static TAppSettings *appSettings = 0;


class AttributeMap : public QMap<int, QString>
{
public:
    AttributeMap() : QMap<int, QString>()
    {
        insert(Tf::ListenPort, "ListenPort");
        insert(Tf::InternalEncoding, "InternalEncoding");
        insert(Tf::HttpOutputEncoding, "HttpOutputEncoding");
        insert(Tf::Locale, "Locale");
        insert(Tf::MultiProcessingModule, "MultiProcessingModule");
        insert(Tf::UploadTemporaryDirectory, "UploadTemporaryDirectory");
        insert(Tf::SqlDatabaseSettingsFiles, "SqlDatabaseSettingsFiles");
        insert(Tf::MongoDbSettingsFile, "MongoDbSettingsFile");
        insert(Tf::RedisSettingsFile, "RedisSettingsFile");
        insert(Tf::SqlQueriesStoredDirectory, "SqlQueriesStoredDirectory");
        insert(Tf::DirectViewRenderMode, "DirectViewRenderMode");
        insert(Tf::SystemLogFile, "SystemLogFile");
        insert(Tf::SqlQueryLogFile, "SqlQueryLogFile");
        insert(Tf::ApplicationAbortOnFatal, "ApplicationAbortOnFatal");
        insert(Tf::LimitRequestBody, "LimitRequestBody");
        insert(Tf::EnableCsrfProtectionModule, "EnableCsrfProtectionModule");
        insert(Tf::EnableHttpMethodOverride, "EnableHttpMethodOverride");
        insert(Tf::HttpKeepAliveTimeout, "HttpKeepAliveTimeout");
        insert(Tf::LDPreload, "LDPreload");
        insert(Tf::JavaScriptPath, "JavaScriptPath");
        insert(Tf::SessionName, "Session.Name");
        insert(Tf::SessionStoreType, "Session.StoreType");
        insert(Tf::SessionAutoIdRegeneration, "Session.AutoIdRegeneration");
        insert(Tf::SessionLifeTime, "Session.LifeTime");
        insert(Tf::SessionCookiePath, "Session.CookiePath");
        insert(Tf::SessionGcProbability, "Session.GcProbability");
        insert(Tf::SessionGcMaxLifeTime, "Session.GcMaxLifeTime");
        insert(Tf::SessionSecret, "Session.Secret");
        insert(Tf::SessionCsrfProtectionKey, "Session.CsrfProtectionKey");
        insert(Tf::MPMThreadMaxAppServers, "MPM.thread.MaxAppServers");
        insert(Tf::MPMThreadMaxThreadsPerAppServer, "MPM.thread.MaxThreadsPerAppServer");
        insert(Tf::MPMHybridMaxAppServers, "MPM.hybrid.MaxAppServers");
        insert(Tf::MPMHybridMaxWorkersPerAppServer, "MPM.hybrid.MaxWorkersPerAppServer");
        insert(Tf::SystemLogFilePath, "SystemLog.FilePath");
        insert(Tf::SystemLogLayout, "SystemLog.Layout");
        insert(Tf::SystemLogDateTimeFormat, "SystemLog.DateTimeFormat");
        insert(Tf::AccessLogFilePath, "AccessLog.FilePath");
        insert(Tf::AccessLogLayout, "AccessLog.Layout");
        insert(Tf::AccessLogDateTimeFormat, "AccessLog.DateTimeFormat");
        insert(Tf::ActionMailerDeliveryMethod, "ActionMailer.DeliveryMethod");
        insert(Tf::ActionMailerCharacterSet, "ActionMailer.CharacterSet");
        insert(Tf::ActionMailerDelayedDelivery, "ActionMailer.DelayedDelivery");
        insert(Tf::ActionMailerSmtpHostName, "ActionMailer.smtp.HostName");
        insert(Tf::ActionMailerSmtpMode, "ActionMailer.smtp.Mode");
        insert(Tf::ActionMailerSmtpPort, "ActionMailer.smtp.Port");
        insert(Tf::ActionMailerSmtpAuthentication, "ActionMailer.smtp.Authentication");
        insert(Tf::ActionMailerSmtpUserName, "ActionMailer.smtp.UserName");
        insert(Tf::ActionMailerSmtpPassword, "ActionMailer.smtp.Password");
        insert(Tf::ActionMailerSmtpEnablePopBeforeSmtp, "ActionMailer.smtp.EnablePopBeforeSmtp");
        insert(Tf::ActionMailerSmtpPopServerHostName, "ActionMailer.smtp.PopServer.HostName");
        insert(Tf::ActionMailerSmtpPopServerPort, "ActionMailer.smtp.PopServer.Port");
        insert(Tf::ActionMailerSmtpPopServerEnableApop, "ActionMailer.smtp.PopServer.EnableApop");
        insert(Tf::ActionMailerSendmailCommandLocation, "ActionMailer.sendmail.CommandLocation");
    }
};
Q_GLOBAL_STATIC(AttributeMap, attributeMap)


TAppSettings::TAppSettings(const QString &path)
    : appIniSettings(new QSettings(path, QSettings::IniFormat))
{ }


QVariant TAppSettings::value(Tf::AppAttribute attr, const QVariant &defaultValue) const
{
    QVariant ret = settingsCache.value((int)attr, QVariant());
    if (ret.isNull()) {
        QMutexLocker locker(&mutex);
        const QString &keystr = (*attributeMap())[attr];
        if (!appIniSettings->contains(keystr)) {
            return defaultValue;
        }

        ret = readValue(keystr);
        if (ret.isNull()) {
            ret = QVariant("");
        }
        settingsCache.insert(attr, ret);
    }
    return ret;
}


QVariant TAppSettings::readValue(const QString &attr, const QVariant &defaultValue) const
{
    return appIniSettings->value(attr, defaultValue);
}


TAppSettings *TAppSettings::instance()
{
    return appSettings;
}


static void cleanup()
{
    if (appSettings) {
        delete appSettings;
        appSettings = 0;
    }
}


void TAppSettings::instantiate(const QString &path)
{
    if (!appSettings) {
        appSettings = new TAppSettings(path);
        qAddPostRoutine(::cleanup);
    }
}
