#include "tcachefactory.h"
#include "tcachesqlitestore.h"
#include "tcachemongostore.h"
#include "tsystemglobal.h"
#include <TAppSettings>
#include <QDir>

static QString SINGLEFILEDB_CACHE_KEY;
static QString MONGODB_CACHE_KEY;


QStringList TCacheFactory::keys()
{
    loadCacheKeys();

    QStringList ret;
    ret << SINGLEFILEDB_CACHE_KEY
        << MONGODB_CACHE_KEY;
    return ret;
}


TCacheStore *TCacheFactory::create(const QString &key)
{
    TCacheStore *ptr = nullptr;
    loadCacheKeys();

    QString k = key.toLower();
    if (k == SINGLEFILEDB_CACHE_KEY) {
        static const qint64 FileSizeThreshold = TAppSettings::instance()->value(Tf::CacheSingleFileDbFileSizeThreshold, 0).toLongLong();
        ptr = new TCacheSQLiteStore(FileSizeThreshold);
    } else if (k == MONGODB_CACHE_KEY) {
        ptr = new TCacheMongoStore;
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
    } else if (k == MONGODB_CACHE_KEY) {
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
    } else if (k == MONGODB_CACHE_KEY) {

    } else {
        // Invalid key
    }
    return settings;
}


TCacheStore::DbType TCacheFactory::dbType(const QString &key)
{
    TCacheStore::DbType type = TCacheStore::Invalid;
    loadCacheKeys();

    TCacheStore *store = create(key);
    if (store) {
        type = store->dbType();
        destroy(key, store);
    }
    return type;
}


bool TCacheFactory::loadCacheKeys()
{
    static bool done = []() {
        SINGLEFILEDB_CACHE_KEY = TCacheSQLiteStore().key().toLower();
        MONGODB_CACHE_KEY = TCacheMongoStore().key().toLower();
        return true;
    }();
    return done;
}
