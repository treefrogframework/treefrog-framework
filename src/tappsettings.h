#ifndef TAPPSETTINGS_H
#define TAPPSETTINGS_H

#include <QMap>
#include <QVariant>
#include <QMutex>
#include <TGlobal>


class T_CORE_EXPORT TAppSettings
{
public:
    enum AppAttribute {
        ListenPort = 0,
        InternalEncoding,
        HttpOutputEncoding,
        Locale,
        MultiProcessingModule,
        UploadTemporaryDirectory,
        SqlDatabaseSettingsFiles,
        MongoDbSettingsFile,
        SqlQueriesStoredDirectory,
        DirectViewRenderMode,
        SystemLogFile,
        SqlQueryLogFile,
        ApplicationAbortOnFatal,
        LimitRequestBody,
        EnableCsrfProtectionModule,
        EnableHttpMethodOverride,
        SessionName,
        SessionStoreType,
        SessionAutoIdRegeneration,
        SessionLifeTime,
        SessionCookiePath,
        SessionGcProbability,
        SessionGcMaxLifeTime,
        SessionSecret,
        SessionCsrfProtectionKey,
        MPMThreadMaxAppServers,
        MPMThreadMaxThreadsPerAppServer,
        MPMPreforkMaxAppServers,
        MPMPreforkMinAppServers,
        MPMPreforkSpareAppServers,
        MPMHybridMaxAppServers,
        MPMHybridMaxWorkersPerAppServer,
        SystemLogFilePath,
        SystemLogLayout,
        SystemLogDateTimeFormat,
        AccessLogFilePath,
        AccessLogLayout,
        AccessLogDateTimeFormat,
        ActionMailerDeliveryMethod,
        ActionMailerCharacterSet,
        ActionMailerDelayedDelivery,
        ActionMailerSmtpHostName,
        ActionMailerSmtpPort,
        ActionMailerSmtpAuthentication,
        ActionMailerSmtpUserName,
        ActionMailerSmtpPassword,
        ActionMailerSmtpEnablePopBeforeSmtp,
        ActionMailerSmtpPopServerHostName,
        ActionMailerSmtpPopServerPort,
        ActionMailerSmtpPopServerEnableApop,
        ActionMailerSendmailCommandLocation,
    };

    const QVariant &value(AppAttribute attr) const;
    static TAppSettings *instance();

private:
    TAppSettings();

    mutable QMap<int, QVariant> settingsCache;
    mutable QMutex mutex;

    Q_DISABLE_COPY(TAppSettings)
};
#endif // TAPPSETTINGS_H
