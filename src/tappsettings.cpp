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


const QMap<Tf::AppAttribute, QString> attributeMap = {
    {Tf::ListenPort, "ListenPort"},
    {Tf::ListenAddress, "ListenAddress"},
    {Tf::InternalEncoding, "InternalEncoding"},
    {Tf::HttpOutputEncoding, "HttpOutputEncoding"},
    {Tf::Locale, "Locale"},
    {Tf::MultiProcessingModule, "MultiProcessingModule"},
    {Tf::UploadTemporaryDirectory, "UploadTemporaryDirectory"},
    {Tf::SqlDatabaseSettingsFiles, "SqlDatabaseSettingsFiles"},
    {Tf::MongoDbSettingsFile, "MongoDbSettingsFile"},
    {Tf::RedisSettingsFile, "RedisSettingsFile"},
    {Tf::MemcachedSettingsFile, "MemcachedSettingsFile"},
    {Tf::SqlQueriesStoredDirectory, "SqlQueriesStoredDirectory"},
    {Tf::DirectViewRenderMode, "DirectViewRenderMode"},
    {Tf::SqlQueryLogFilePath, "SqlQueryLog.FilePath"},
    {Tf::SqlQueryLogLayout, "SqlQueryLog.Layout"},
    {Tf::SqlQueryLogDateTimeFormat, "SqlQueryLog.DateTimeFormat"},
    {Tf::ApplicationAbortOnFatal, "ApplicationAbortOnFatal"},
    {Tf::LimitRequestBody, "LimitRequestBody"},
    {Tf::EnableCsrfProtectionModule, "EnableCsrfProtectionModule"},
    {Tf::EnableHttpMethodOverride, "EnableHttpMethodOverride"},
    {Tf::EnableForwardedForHeader, "EnableForwardedForHeader"},
    {Tf::TrustedProxyServers, "TrustedProxyServers"},
    {Tf::HttpKeepAliveTimeout, "HttpKeepAliveTimeout"},
    {Tf::LDPreload, "LDPreload"},
    {Tf::JavaScriptPath, "JavaScriptPath"},
    {Tf::SessionName, "Session.Name"},
    {Tf::SessionStoreType, "Session.StoreType"},
    {Tf::SessionAutoIdRegeneration, "Session.AutoIdRegeneration"},
    {Tf::SessionCookieMaxAge, "Session.CookieMaxAge"},
    {Tf::SessionCookieDomain, "Session.CookieDomain"},
    {Tf::SessionCookiePath, "Session.CookiePath"},
    {Tf::SessionCookieSameSite, "Session.CookieSameSite"},
    {Tf::SessionGcProbability, "Session.GcProbability"},
    {Tf::SessionGcMaxLifeTime, "Session.GcMaxLifeTime"},
    {Tf::SessionSecret, "Session.Secret"},
    {Tf::SessionCsrfProtectionKey, "Session.CsrfProtectionKey"},
    {Tf::MPMThreadMaxAppServers, "MPM.thread.MaxAppServers"},
    {Tf::MPMThreadMaxThreadsPerAppServer, "MPM.thread.MaxThreadsPerAppServer"},
    {Tf::MPMEpollMaxAppServers, "MPM.epoll.MaxAppServers"},
    {Tf::SystemLogFilePath, "SystemLog.FilePath"},
    {Tf::SystemLogLayout, "SystemLog.Layout"},
    {Tf::SystemLogDateTimeFormat, "SystemLog.DateTimeFormat"},
    {Tf::AccessLogFilePath, "AccessLog.FilePath"},
    {Tf::AccessLogLayout, "AccessLog.Layout"},
    {Tf::AccessLogDateTimeFormat, "AccessLog.DateTimeFormat"},
    {Tf::ActionMailerDeliveryMethod, "ActionMailer.DeliveryMethod"},
    {Tf::ActionMailerCharacterSet, "ActionMailer.CharacterSet"},
    {Tf::ActionMailerDelayedDelivery, "ActionMailer.DelayedDelivery"},
    {Tf::ActionMailerSmtpHostName, "ActionMailer.smtp.HostName"},
    {Tf::ActionMailerSmtpPort, "ActionMailer.smtp.Port"},
    {Tf::ActionMailerSmtpRequireTLS, "ActionMailer.smtp.RequireTLS"},
    {Tf::ActionMailerSmtpAuthentication, "ActionMailer.smtp.Authentication"},
    {Tf::ActionMailerSmtpUserName, "ActionMailer.smtp.UserName"},
    {Tf::ActionMailerSmtpPassword, "ActionMailer.smtp.Password"},
    {Tf::ActionMailerSmtpEnablePopBeforeSmtp, "ActionMailer.smtp.EnablePopBeforeSmtp"},
    {Tf::ActionMailerSmtpPopServerHostName, "ActionMailer.smtp.PopServer.HostName"},
    {Tf::ActionMailerSmtpPopServerPort, "ActionMailer.smtp.PopServer.Port"},
    {Tf::ActionMailerSmtpPopServerEnableApop, "ActionMailer.smtp.PopServer.EnableApop"},
    {Tf::ActionMailerSendmailCommandLocation, "ActionMailer.sendmail.CommandLocation"},
    {Tf::CacheSettingsFile, "Cache.SettingsFile"},
    {Tf::CacheBackend, "Cache.Backend"},
    {Tf::CacheGcProbability, "Cache.GcProbability"},
    {Tf::CacheEnableCompression, "Cache.EnableCompression"},
};


const QMap<Tf::AppAttribute, QVariant> defaultValueMap = {
    {Tf::ListenPort, 8800},
    {Tf::SystemLogFilePath, "log/treefrog.log"},
    {Tf::SqlQueryLogLayout, "%d [%t] %m%n"},
    {Tf::SqlQueryLogDateTimeFormat, "yyyy-MM-dd hh:mm:ss"},
    {Tf::SystemLogLayout, "%d %5P %m%n"},
    {Tf::SystemLogDateTimeFormat, "yyyy-MM-dd hh:mm:ss"},
    {Tf::AccessLogLayout, "%h %d \"%r\" %s %O%n"},
    {Tf::AccessLogDateTimeFormat, "yyyy-MM-dd hh:mm:ss"},
    {Tf::EnableCsrfProtectionModule, true},
    {Tf::HttpKeepAliveTimeout, 10},
    {Tf::LimitRequestBody, 0},
    {Tf::ActionMailerCharacterSet, "UTF-8"},
    {Tf::ActionMailerSmtpEnablePopBeforeSmtp, false},
    {Tf::ActionMailerSmtpPopServerEnableApop, false},
    {Tf::EnableHttpMethodOverride, false},
    {Tf::EnableForwardedForHeader, false},
    {Tf::CacheGcProbability, 1000},
    {Tf::CacheEnableCompression, true},
    {Tf::JavaScriptPath, "script;node_modules"},
    {Tf::SessionAutoIdRegeneration, false},
    {Tf::ActionMailerDelayedDelivery, false},
    {Tf::InternalEncoding, "UTF-8"},
};


TAppSettings::TAppSettings(const QString &path) :
    appIniSettings(new QSettings(path, QSettings::IniFormat))
{
}


QString TAppSettings::key(Tf::AppAttribute attr) const
{
    return attributeMap.value(attr);
}


QVariant TAppSettings::value(Tf::AppAttribute attr) const
{
    QVariant ret = settingsCache.value((int)attr, QVariant());
    if (ret.isNull()) {
        QMutexLocker locker(&mutex);
        const QString &keystr = (attributeMap)[attr];
        if (!appIniSettings->contains(keystr)) {
            return defaultValueMap.value(attr);
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
    auto keylist = attributeMap.keys();
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
