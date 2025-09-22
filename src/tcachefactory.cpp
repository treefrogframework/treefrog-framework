#include "tcachefactory.h"
#include "tcachemongostore.h"
#include "tcacheredisstore.h"
#include "tcachesqlitestore.h"
#include "tcachememcachedstore.h"
#include "tcachesharedmemorystore.h"
#include "tsystemglobal.h"
#include <QDir>
#include <TAppSettings>

namespace {
QString SQLITE_CACHE_KEY;
QString MONGO_CACHE_KEY;
QString REDIS_CACHE_KEY;
QString MEMCACHED_CACHE_KEY;
QString MEMORY_CACHE_KEY;
}


QStringList TCacheFactory::keys()
{
    loadCacheKeys();

    QStringList ret;
    ret << SQLITE_CACHE_KEY
        << MONGO_CACHE_KEY
        << REDIS_CACHE_KEY
        << MEMCACHED_CACHE_KEY
        << MEMORY_CACHE_KEY;
    return ret;
}


TCacheStore *TCacheFactory::create(const QString &key)
{
    loadCacheKeys();

    TCacheStore *ptr = nullptr;
    QString k = key.toLower();
    if (k == SQLITE_CACHE_KEY) {
        ptr = new TCacheSQLiteStore;
    } else if (k == MONGO_CACHE_KEY) {
        ptr = new TCacheMongoStore;
    } else if (k == REDIS_CACHE_KEY) {
        ptr = new TCacheRedisStore;
    } else if (k == MEMCACHED_CACHE_KEY) {
        ptr = new TCacheMemcachedStore;
    } else if (k == MEMORY_CACHE_KEY) {
        ptr = new TCacheSharedMemoryStore;
    } else {
        tSystemError("Not found cache store: {}", key);
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
    } else if (k == MEMCACHED_CACHE_KEY) {
        delete store;
    } else if (k == MEMORY_CACHE_KEY) {
        delete store;
    } else {
        delete store;
    }
}


QMap<QString, QVariant> TCacheFactory::defaultSettings(const QString &key)
{
    loadCacheKeys();

    QMap<QString, QVariant> settings;
    QString k = key.toLower();
    if (k == SQLITE_CACHE_KEY) {
        settings = TCacheSQLiteStore().defaultSettings();
    } else if (k == MONGO_CACHE_KEY) {
        settings = TCacheMongoStore().defaultSettings();
    } else if (k == REDIS_CACHE_KEY) {
        settings = TCacheRedisStore().defaultSettings();
    } else if (k == MEMCACHED_CACHE_KEY) {
        settings = TCacheMemcachedStore().defaultSettings();
    } else if (k == MEMORY_CACHE_KEY) {
        settings = TCacheSharedMemoryStore().defaultSettings();
    } else {
        // Invalid key
    }
    return settings;
}


TCacheStore::DbType TCacheFactory::dbType(const QString &key)
{
    loadCacheKeys();

    TCacheStore::DbType type = TCacheStore::Invalid;
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
        MEMCACHED_CACHE_KEY = TCacheMemcachedStore().key().toLower();
        MEMORY_CACHE_KEY = TCacheSharedMemoryStore().key().toLower();
        return true;
    }();
    return done;
}
