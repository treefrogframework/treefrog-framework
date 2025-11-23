#include "tcachesharedmemorystore.h"
#include "tsharedmemorykvs.h"
#include "TWebApplication"


TCacheSharedMemoryStore::TCacheSharedMemoryStore()
{}


TCacheSharedMemoryStore::~TCacheSharedMemoryStore()
{}


void TCacheSharedMemoryStore::init()
{
    const QVariantMap &settings = Tf::app()->kvsSettings(Tf::KvsEngine::CacheKvs);
    auto databaseName = settings.value("DatabaseName").toString().trimmed();
    auto connectOptions = settings.value("ConnectOptions").toString().trimmed();
    TSharedMemoryKvs::initialize(databaseName, connectOptions);
}


void TCacheSharedMemoryStore::cleanup()
{
    TSharedMemoryKvs kvs(Tf::KvsEngine::CacheKvs);
    kvs.cleanup();
}


bool TCacheSharedMemoryStore::set(const QByteArray &key, const QByteArray &value, int seconds)
{
    TSharedMemoryKvs kvs(Tf::KvsEngine::CacheKvs);
    return kvs.set(key, value, seconds);
}


QByteArray TCacheSharedMemoryStore::get(const QByteArray &key)
{
    TSharedMemoryKvs kvs(Tf::KvsEngine::CacheKvs);
    return kvs.get(key);
}


bool TCacheSharedMemoryStore::remove(const QByteArray &key)
{
    TSharedMemoryKvs kvs(Tf::KvsEngine::CacheKvs);
    return kvs.remove(key);
}


void TCacheSharedMemoryStore::clear()
{
    TSharedMemoryKvs kvs(Tf::KvsEngine::CacheKvs);
    return kvs.clear();
}


void TCacheSharedMemoryStore::gc()
{
    TSharedMemoryKvs kvs(Tf::KvsEngine::CacheKvs);
    return kvs.gc();
}


QMap<QString, QVariant> TCacheSharedMemoryStore::defaultSettings() const
{
    QMap<QString, QVariant> settings {
        {"DatabaseName", "tfcache.shm"},
        {"ConnectOptions", "MEMORY_SIZE=512M"},
    };
    return settings;
}
