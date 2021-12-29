#pragma once
#include <QMap>
#include <QMutex>
#include <QVariant>
#include <TGlobal>

class QSettings;


class T_CORE_EXPORT TAppSettings {
public:
    QString key(Tf::AppAttribute attr) const;
    QVariant value(Tf::AppAttribute attr) const;
    QVariant readValue(const QString &attr, const QVariant &defaultValue = QVariant()) const;
    QList<Tf::AppAttribute> keys() const;

    static TAppSettings *instance();

private:
    TAppSettings(const QString &path);
    static void instantiate(const QString &path);

    mutable QMutex mutex;
    mutable QMap<int, QVariant> settingsCache;
    QSettings *appIniSettings {nullptr};

    friend class TWebApplication;
    T_DISABLE_COPY(TAppSettings)
    T_DISABLE_MOVE(TAppSettings)
};
