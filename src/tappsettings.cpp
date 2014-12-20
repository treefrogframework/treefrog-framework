/* Copyright (c) 2014, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TAppSettings>
#include <TWebApplication>
#include <QMutexLocker>


class AttributeMap : public QMap<int, QString>
{
public:
    AttributeMap() : QMap<int, QString>()
    {
        insert(TAppSettings::ListenPort, "ListenPort");
        insert(TAppSettings::InternalEncoding, "InternalEncoding");
        insert(TAppSettings::HttpOutputEncoding, "HttpOutputEncoding");
        insert(TAppSettings::Locale, "Locale");
        insert(TAppSettings::MultiProcessingModule, "MultiProcessingModule");
        insert(TAppSettings::UploadTemporaryDirectory, "UploadTemporaryDirectory");
        insert(TAppSettings::SqlDatabaseSettingsFiles, "SqlDatabaseSettingsFiles");
        insert(TAppSettings::MongoDbSettingsFile, "MongoDbSettingsFile");
        insert(TAppSettings::SqlQueriesStoredDirectory, "SqlQueriesStoredDirectory");
        insert(TAppSettings::DirectViewRenderMode, "DirectViewRenderMode");
        insert(TAppSettings::SystemLogFile, "SystemLogFile");
        insert(TAppSettings::SqlQueryLogFile, "SqlQueryLogFile");
        insert(TAppSettings::ApplicationAbortOnFatal, "ApplicationAbortOnFatal");
        insert(TAppSettings::LimitRequestBody, "LimitRequestBody");
        insert(TAppSettings::EnableCsrfProtectionModule, "EnableCsrfProtectionModule");
        insert(TAppSettings::EnableHttpMethodOverride, "EnableHttpMethodOverride");
        insert(TAppSettings::SessionName, "Session.Name");
        insert(TAppSettings::SessionStoreType, "Session.StoreType");
        insert(TAppSettings::SessionAutoIdRegeneration, "Session.AutoIdRegeneration");
        insert(TAppSettings::SessionLifeTime, "Session.LifeTime");
        insert(TAppSettings::SessionCookiePath, "Session.CookiePath");
        insert(TAppSettings::SessionGcProbability, "Session.GcProbability");
        insert(TAppSettings::SessionGcMaxLifeTime, "Session.GcMaxLifeTime");
        insert(TAppSettings::SessionSecret, "Session.Secret");
        insert(TAppSettings::SessionCsrfProtectionKey, "Session.CsrfProtectionKey");
        insert(TAppSettings::MPMThreadMaxAppServers, "MPM.thread.MaxAppServers");
        insert(TAppSettings::MPMThreadMaxThreadsPerAppServer, "MPM.thread.MaxThreadsPerAppServer");
        insert(TAppSettings::MPMPreforkMaxAppServers, "MPM.prefork.MaxAppServers");
        insert(TAppSettings::MPMPreforkMinAppServers, "MPM.prefork.MinAppServers");
        insert(TAppSettings::MPMPreforkSpareAppServers, "MPM.prefork.SpareAppServers");
        insert(TAppSettings::MPMHybridMaxAppServers, "MPM.hybrid.MaxAppServers");
        insert(TAppSettings::MPMHybridMaxWorkersPerAppServer, "MPM.hybrid.MaxWorkersPerAppServer");
        insert(TAppSettings::SystemLogFilePath, "SystemLog.FilePath");
        insert(TAppSettings::SystemLogLayout, "SystemLog.Layout");
        insert(TAppSettings::SystemLogDateTimeFormat, "SystemLog.DateTimeFormat");
        insert(TAppSettings::AccessLogFilePath, "AccessLog.FilePath");
        insert(TAppSettings::AccessLogLayout, "AccessLog.Layout");
        insert(TAppSettings::AccessLogDateTimeFormat, "AccessLog.DateTimeFormat");
        insert(TAppSettings::ActionMailerDeliveryMethod, "ActionMailer.DeliveryMethod");
        insert(TAppSettings::ActionMailerCharacterSet, "ActionMailer.CharacterSet");
        insert(TAppSettings::ActionMailerDelayedDelivery, "ActionMailer.DelayedDelivery");
        insert(TAppSettings::ActionMailerSmtpHostName, "ActionMailer.smtp.HostName");
        insert(TAppSettings::ActionMailerSmtpPort, "ActionMailer.smtp.Port");
        insert(TAppSettings::ActionMailerSmtpAuthentication, "ActionMailer.smtp.Authentication");
        insert(TAppSettings::ActionMailerSmtpUserName, "ActionMailer.smtp.UserName");
        insert(TAppSettings::ActionMailerSmtpPassword, "ActionMailer.smtp.Password");
        insert(TAppSettings::ActionMailerSmtpEnablePopBeforeSmtp, "ActionMailer.smtp.EnablePopBeforeSmtp");
        insert(TAppSettings::ActionMailerSmtpPopServerHostName, "ActionMailer.smtp.PopServer.HostName");
        insert(TAppSettings::ActionMailerSmtpPopServerPort, "ActionMailer.smtp.PopServer.Port");
        insert(TAppSettings::ActionMailerSmtpPopServerEnableApop, "ActionMailer.smtp.PopServer.EnableApop");
        insert(TAppSettings::ActionMailerSendmailCommandLocation, "ActionMailer.sendmail.CommandLocation");
    }
};
Q_GLOBAL_STATIC(AttributeMap, attributeMap)


TAppSettings::TAppSettings()
{ }


const QVariant &TAppSettings::value(AppAttribute attr) const
{
    if (!settingsCache.contains((int)attr)) {
        QMutexLocker locker(&mutex);
        const QString &keystr = (*attributeMap())[attr];
        QVariant val = Tf::app()->appSettings().value(keystr);
        settingsCache.insert(attr, val);
    }
    return settingsCache[(int)attr];
}


TAppSettings *TAppSettings::instance()
{
    static TAppSettings settings;
    return &settings;
}
