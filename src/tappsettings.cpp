/* Copyright (c) 2014, AOYAMA Kazuharu
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
#include <stdio.h>

static TAppSettings *appSettings = 0;

static const QMap<int, QString> attributeMap = {
    { Tf::ListenPort, "ListenPort" },
    { Tf::InternalEncoding, "InternalEncoding" },
    { Tf::HttpOutputEncoding, "HttpOutputEncoding" },
    { Tf::Locale, "Locale" },
    { Tf::MultiProcessingModule, "MultiProcessingModule" },
    { Tf::UploadTemporaryDirectory, "UploadTemporaryDirectory" },
    { Tf::SqlDatabaseSettingsFiles, "SqlDatabaseSettingsFiles" },
    { Tf::MongoDbSettingsFile, "MongoDbSettingsFile" },
    { Tf::SqlQueriesStoredDirectory, "SqlQueriesStoredDirectory" },
    { Tf::DirectViewRenderMode, "DirectViewRenderMode" },
    { Tf::SystemLogFile, "SystemLogFile" },
    { Tf::SqlQueryLogFile, "SqlQueryLogFile" },
    { Tf::ApplicationAbortOnFatal, "ApplicationAbortOnFatal" },
    { Tf::LimitRequestBody, "LimitRequestBody" },
    { Tf::EnableCsrfProtectionModule, "EnableCsrfProtectionModule" },
    { Tf::EnableHttpMethodOverride, "EnableHttpMethodOverride" },
    { Tf::SessionName, "Session.Name" },
    { Tf::SessionStoreType, "Session.StoreType" },
    { Tf::SessionAutoIdRegeneration, "Session.AutoIdRegeneration" },
    { Tf::SessionLifeTime, "Session.LifeTime" },
    { Tf::SessionCookiePath, "Session.CookiePath" },
    { Tf::SessionGcProbability, "Session.GcProbability" },
    { Tf::SessionGcMaxLifeTime, "Session.GcMaxLifeTime" },
    { Tf::SessionSecret, "Session.Secret" },
    { Tf::SessionCsrfProtectionKey, "Session.CsrfProtectionKey" },
    { Tf::MPMThreadMaxAppServers, "MPM.thread.MaxAppServers" },
    { Tf::MPMThreadMaxThreadsPerAppServer, "MPM.thread.MaxThreadsPerAppServer" },
    { Tf::MPMPreforkMaxAppServers, "MPM.prefork.MaxAppServers" },
    { Tf::MPMPreforkMinAppServers, "MPM.prefork.MinAppServers" },
    { Tf::MPMPreforkSpareAppServers, "MPM.prefork.SpareAppServers" },
    { Tf::MPMHybridMaxAppServers, "MPM.hybrid.MaxAppServers" },
    { Tf::MPMHybridMaxWorkersPerAppServer, "MPM.hybrid.MaxWorkersPerAppServer" },
    { Tf::SystemLogFilePath, "SystemLog.FilePath" },
    { Tf::SystemLogLayout, "SystemLog.Layout" },
    { Tf::SystemLogDateTimeFormat, "SystemLog.DateTimeFormat" },
    { Tf::AccessLogFilePath, "AccessLog.FilePath" },
    { Tf::AccessLogLayout, "AccessLog.Layout" },
    { Tf::AccessLogDateTimeFormat, "AccessLog.DateTimeFormat" },
    { Tf::ActionMailerDeliveryMethod, "ActionMailer.DeliveryMethod" },
    { Tf::ActionMailerCharacterSet, "ActionMailer.CharacterSet" },
    { Tf::ActionMailerDelayedDelivery, "ActionMailer.DelayedDelivery" },
    { Tf::ActionMailerSmtpHostName, "ActionMailer.smtp.HostName" },
    { Tf::ActionMailerSmtpPort, "ActionMailer.smtp.Port" },
    { Tf::ActionMailerSmtpAuthentication, "ActionMailer.smtp.Authentication" },
    { Tf::ActionMailerSmtpUserName, "ActionMailer.smtp.UserName" },
    { Tf::ActionMailerSmtpPassword, "ActionMailer.smtp.Password" },
    { Tf::ActionMailerSmtpEnablePopBeforeSmtp, "ActionMailer.smtp.EnablePopBeforeSmtp" },
    { Tf::ActionMailerSmtpPopServerHostName, "ActionMailer.smtp.PopServer.HostName" },
    { Tf::ActionMailerSmtpPopServerPort, "ActionMailer.smtp.PopServer.Port" },
    { Tf::ActionMailerSmtpPopServerEnableApop, "ActionMailer.smtp.PopServer.EnableApop" },
    { Tf::ActionMailerSendmailCommandLocation, "ActionMailer.sendmail.CommandLocation" },
};


TAppSettings::TAppSettings(const QString &path)
    : appIniSettings(new QSettings(path, QSettings::IniFormat))
{ }


const QVariant &TAppSettings::value(Tf::AppAttribute attr, const QVariant &defaultValue) const
{
    if (!settingsCache.contains((int)attr)) {
        QMutexLocker locker(&mutex);
        const QString &keystr = attributeMap[attr];
        if (!appIniSettings->contains(keystr)) {
            return defaultValue;
        }

        QVariant val = readValue(keystr);
        settingsCache.insert(attr, val);
    }
    return settingsCache[(int)attr];
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
