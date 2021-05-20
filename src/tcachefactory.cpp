#include "tcachefactory.h"
#include "tcachemongostore.h"
#include "tcacheredisstore.h"
#include "tcachesqlitestore.h"
#include "tsystemglobal.h"
#include <QDir>
#include <TAppSettings>

namespace {
QString SQLITE_CACHE_KEY;
QString MONGO_CACHE_KEY;
QString REDIS_CACHE_KEY;
}


QStringList TCacheFactory::keys()
{
    loadCacheKeys();

    QStringList ret;
    ret << SQLITE_CACHE_KEY
        << MONGO_CACHE_KEY
        << REDIS_CACHE_KEY;
    return ret;
}


TCacheStore *TCacheFactory::create(const QString &key)
{
    TCacheStore *ptr = nullptr;
    loadCacheKeys();

    QString k = key.toLower();
    if (k == SQLITE_CACHE_KEY) {
        ptr = new TCacheSQLiteStore;
    } else if (k == MONGO_CACHE_KEY) {
        ptr = new TCacheMongoStore;
    } else if (k == REDIS_CACHE_KEY) {
        ptr = new TCacheRedisStore;
    } else {
        tSystemError("Not found cache store: %s", qUtf8Printable(key));
    }

    return ptr;
}


void TCacheFactory::destroy(const QString &key, TCacheStore *store)
{
    loadCacheKeys();

    QString k = key.toLower();
    if (k == SQLITE_CACHE_KEY) {
        delete store;
    } else if (k == MONGO_CACHE_KEY) {
        delete store;
    } else if (k == REDIS_CACHE_KEY) {
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
    if (k == SQLITE_CACHE_KEY) {
        settings = TCacheSQLiteStore().defaultSettings();
    } else if (k == MONGO_CACHE_KEY) {
        settings = TCacheMongoStore().defaultSettings();
    } else if (k == REDIS_CACHE_KEY) {
        settings = TCacheRedisStore().defaultSettings();
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
        SQLITE_CACHE_KEY = TCacheSQLiteStore().key().toLower();
        MONGO_CACHE_KEY = TCacheMongoStore().key().toLower();
        REDIS_CACHE_KEY = TCacheRedisStore().key().toLower();
        return true;
    }();
    return done;
}
