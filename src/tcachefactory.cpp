#include "tcachefactory.h"
#include "tcachesqlitestore.h"
#include "tsystemglobal.h"
#include <TAppSettings>
#include <QDir>

static QString SINGLEFILEDB_CACHE_KEY;


QStringList TCacheFactory::keys()
{
    loadCacheKeys();

    QStringList ret;
    ret << SINGLEFILEDB_CACHE_KEY;
    return ret;
}


TCacheStore *TCacheFactory::create(const QString &key)
{
    TCacheStore *ptr = nullptr;
    loadCacheKeys();

    QString k = key.toLower();
    if (k == SINGLEFILEDB_CACHE_KEY) {
        static const qint64 FileSizeThreshold = TAppSettings::instance()->value(Tf::CacheSingleFileDbFileSizeThreshold).toLongLong();
        ptr = new TCacheSQLiteStore(FileSizeThreshold);
    } else {
        tSystemError("Not found cache store: %s", qPrintable(key));
    }

    return ptr;
}


void TCacheFactory::destroy(const QString &key, TCacheStore *store)
{
    loadCacheKeys();

    QString k = key.toLower();
    if (k == SINGLEFILEDB_CACHE_KEY) {
        delete store;
    } else {
        delete store;
    }
}


QMap<QString, QVariant> TCacheFactory::defaultSettings(const QString &key)
{
    QMap<QString, QVariant> settings;
    loadCacheKeys();

    QString k = key.toLower();
    if (k == SINGLEFILEDB_CACHE_KEY) {
        settings = TCacheSQLiteStore().defaultSettings();
    } else {
        // Invalid key
    }
    return settings;
}


bool TCacheFactory::loadCacheKeys()
{
    static bool done = []() {
        SINGLEFILEDB_CACHE_KEY = TCacheSQLiteStore().key().toLower();
        return true;
    }();
    return done;
}
