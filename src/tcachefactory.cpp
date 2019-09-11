#include "tcachefactory.h"
#include "tcacheinmemorystore.h"
#include "tcachesqlitestore.h"
#include "tsystemglobal.h"
#include <TAppSettings>
#include <QDir>

static QString INMEMORY_CACHE_KEY;
static QString SINGLEFILEDB_CACHE_KEY;


static void loadCacheKeys()
{
    static bool done = []() {
        INMEMORY_CACHE_KEY = TCacheInMemoryStore().key().toLower();
        SINGLEFILEDB_CACHE_KEY = TCacheSQLiteStore().key().toLower();
        return true;
    }();

    Q_ASSERT(!INMEMORY_CACHE_KEY.isEmpty());
    Q_ASSERT(!SINGLEFILEDB_CACHE_KEY.isEmpty());
    Q_UNUSED(done);
}


QStringList TCacheFactory::keys()
{
    loadCacheKeys();

    QStringList ret;
    ret << INMEMORY_CACHE_KEY
        << SINGLEFILEDB_CACHE_KEY;
    return ret;
}


TCacheStore *TCacheFactory::create(const QString &key)
{
    TCacheStore *ptr = nullptr;
    loadCacheKeys();

    QString k = key.toLower();
    if (k == INMEMORY_CACHE_KEY) {
        static const qint64 DbSizeThreshold = TAppSettings::instance()->value(Tf::CacheMemoryDbSizeThreshold).toLongLong();
        ptr = new TCacheInMemoryStore(DbSizeThreshold);
    } else if (k == SINGLEFILEDB_CACHE_KEY) {
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
    if (k == INMEMORY_CACHE_KEY) {
        delete store;
    } else if (k == SINGLEFILEDB_CACHE_KEY) {
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
    if (k == INMEMORY_CACHE_KEY) {
        settings = TCacheInMemoryStore().defaultSettings();
    } else if (k == SINGLEFILEDB_CACHE_KEY) {
        settings = TCacheSQLiteStore().defaultSettings();
    } else {
        // Invalid key
    }
    return settings;
}
