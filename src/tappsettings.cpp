/* Copyright (c) 2014-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QCoreApplication>
#include <QMutexLocker>
#include <QSettings>
#include <TAppSettings>
#include <TSystemGlobal>
#include <cstdio>

namespace {
TAppSettings *appSettings = nullptr;
}


class AttributeMap : public QMap<Tf::AppAttribute, QString> {
public:
    AttributeMap() :
        QMap<Tf::AppAttribute, QString>()
    {
        insert(Tf::ListenPort, "ListenPort");
        insert(Tf::ListenAddress, "ListenAddress");
        insert(Tf::InternalEncoding, "InternalEncoding");
        insert(Tf::HttpOutputEncoding, "HttpOutputEncoding");
        insert(Tf::Locale, "Locale");
        insert(Tf::MultiProcessingModule, "MultiProcessingModule");
        insert(Tf::UploadTemporaryDirectory, "UploadTemporaryDirectory");
        insert(Tf::SqlDatabaseSettingsFiles, "SqlDatabaseSettingsFiles");
        insert(Tf::MongoDbSettingsFile, "MongoDbSettingsFile");
        insert(Tf::RedisSettingsFile, "RedisSettingsFile");
        insert(Tf::MemcachedSettingsFile, "MemcachedSettingsFile");
        insert(Tf::SqlQueriesStoredDirectory, "SqlQueriesStoredDirectory");
        insert(Tf::DirectViewRenderMode, "DirectViewRenderMode");
        insert(Tf::SqlQueryLogFile, "SqlQueryLogFile");  // Deprecated
        insert(Tf::SqlQueryLogFilePath, "SqlQueryLog.FilePath");
        insert(Tf::SqlQueryLogLayout, "SqlQueryLog.Layout");
        insert(Tf::SqlQueryLogDateTimeFormat, "SqlQueryLog.DateTimeFormat");
        insert(Tf::ApplicationAbortOnFatal, "ApplicationAbortOnFatal");
        insert(Tf::LimitRequestBody, "LimitRequestBody");
        insert(Tf::EnableCsrfProtectionModule, "EnableCsrfProtectionModule");
        insert(Tf::EnableHttpMethodOverride, "EnableHttpMethodOverride");
        insert(Tf::EnableForwardedForHeader, "EnableForwardedForHeader");
        insert(Tf::TrustedProxyServers, "TrustedProxyServers");
        insert(Tf::HttpKeepAliveTimeout, "HttpKeepAliveTimeout");
        insert(Tf::LDPreload, "LDPreload");
        insert(Tf::JavaScriptPath, "JavaScriptPath");
        insert(Tf::SessionName, "Session.Name");
        insert(Tf::SessionStoreType, "Session.StoreType");
        insert(Tf::SessionAutoIdRegeneration, "Session.AutoIdRegeneration");
        insert(Tf::SessionCookieMaxAge, "Session.CookieMaxAge");
        insert(Tf::SessionCookieDomain, "Session.CookieDomain");
        insert(Tf::SessionCookiePath, "Session.CookiePath");
        insert(Tf::SessionCookieSameSite, "Session.CookieSameSite");
        insert(Tf::SessionGcProbability, "Session.GcProbability");
        insert(Tf::SessionGcMaxLifeTime, "Session.GcMaxLifeTime");
        insert(Tf::SessionSecret, "Session.Secret");
        insert(Tf::SessionCsrfProtectionKey, "Session.CsrfProtectionKey");
        insert(Tf::MPMThreadMaxAppServers, "MPM.thread.MaxAppServers");
        insert(Tf::MPMThreadMaxThreadsPerAppServer, "MPM.thread.MaxThreadsPerAppServer");
        insert(Tf::MPMEpollMaxAppServers, "MPM.epoll.MaxAppServers");
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
        insert(Tf::ActionMailerSmtpPort, "ActionMailer.smtp.Port");
        insert(Tf::ActionMailerSmtpRequireTLS, "ActionMailer.smtp.RequireTLS");
        insert(Tf::ActionMailerSmtpAuthentication, "ActionMailer.smtp.Authentication");
        insert(Tf::ActionMailerSmtpUserName, "ActionMailer.smtp.UserName");
        insert(Tf::ActionMailerSmtpPassword, "ActionMailer.smtp.Password");
        insert(Tf::ActionMailerSmtpEnablePopBeforeSmtp, "ActionMailer.smtp.EnablePopBeforeSmtp");
        insert(Tf::ActionMailerSmtpPopServerHostName, "ActionMailer.smtp.PopServer.HostName");
        insert(Tf::ActionMailerSmtpPopServerPort, "ActionMailer.smtp.PopServer.Port");
        insert(Tf::ActionMailerSmtpPopServerEnableApop, "ActionMailer.smtp.PopServer.EnableApop");
        insert(Tf::ActionMailerSendmailCommandLocation, "ActionMailer.sendmail.CommandLocation");
        insert(Tf::CacheSettingsFile, "Cache.SettingsFile");
        insert(Tf::CacheBackend, "Cache.Backend");
        insert(Tf::CacheGcProbability, "Cache.GcProbability");
        insert(Tf::CacheEnableCompression, "Cache.EnableCompression");
    }
};
Q_GLOBAL_STATIC(AttributeMap, attributeMap)


class DefaultValue : public QMap<Tf::AppAttribute, QVariant> {
public:
    DefaultValue() :
        QMap<Tf::AppAttribute, QVariant>()
    {
        insert(Tf::ListenPort, 8800);
        insert(Tf::SystemLogFilePath, "log/treefrog.log");
        insert(Tf::SqlQueryLogLayout, "%d [%t] %m%n");
        insert(Tf::SqlQueryLogDateTimeFormat, "yyyy-MM-dd hh:mm:ss");
        insert(Tf::SystemLogLayout, "%d %5P %m%n");
        insert(Tf::SystemLogDateTimeFormat, "yyyy-MM-dd hh:mm:ss");
        insert(Tf::AccessLogLayout, "%h %d \"%r\" %s %O%n");
        insert(Tf::AccessLogDateTimeFormat, "yyyy-MM-dd hh:mm:ss");
        insert(Tf::EnableCsrfProtectionModule, true);
        insert(Tf::HttpKeepAliveTimeout, 10);
        insert(Tf::LimitRequestBody, 0);
        insert(Tf::ActionMailerCharacterSet, "UTF-8");
        insert(Tf::ActionMailerSmtpEnablePopBeforeSmtp, false);
        insert(Tf::ActionMailerSmtpPopServerEnableApop, false);
        insert(Tf::EnableHttpMethodOverride, false);
        insert(Tf::EnableForwardedForHeader, false);
        insert(Tf::CacheGcProbability, 1000);
        insert(Tf::CacheEnableCompression, true);
        insert(Tf::JavaScriptPath, "script;node_modules");
        insert(Tf::SessionAutoIdRegeneration, false);
        insert(Tf::ActionMailerDelayedDelivery, false);
        insert(Tf::InternalEncoding, "UTF-8");
    }
};
Q_GLOBAL_STATIC(DefaultValue, defaultValueMap)


TAppSettings::TAppSettings(const QString &path) :
    appIniSettings(new QSettings(path, QSettings::IniFormat))
{
}


QString TAppSettings::key(Tf::AppAttribute attr) const
{
    return attributeMap()->value(attr);
}


QVariant TAppSettings::value(Tf::AppAttribute attr) const
{
    QVariant ret = settingsCache.value((int)attr, QVariant());
    if (ret.isNull()) {
        QMutexLocker locker(&mutex);
        const QString &keystr = (*attributeMap())[attr];
        if (!appIniSettings->contains(keystr)) {
            return defaultValueMap()->value(attr);
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


QList<Tf::AppAttribute> TAppSettings::keys() const
{
    auto keylist = attributeMap()->keys();
    return keylist;
}


TAppSettings *TAppSettings::instance()
{
    return appSettings;
}


void TAppSettings::instantiate(const QString &path)
{
    if (!appSettings) {
        appSettings = new TAppSettings(path);
    }
}
