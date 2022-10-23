/* Copyright (c) 2022, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tcachememcachedstore.h"
#include <TMemcached>


TCacheMemcachedStore::TCacheMemcachedStore()
{
}


bool TCacheMemcachedStore::open()
{
    return true;
}


void TCacheMemcachedStore::close()
{
}


QByteArray TCacheMemcachedStore::get(const QByteArray &key)
{
    TMemcached memcached(Tf::KvsEngine::CacheKvs);
    return memcached.get(key);
}


bool TCacheMemcachedStore::set(const QByteArray &key, const QByteArray &value, int seconds)
{
    TMemcached memcached(Tf::KvsEngine::CacheKvs);
    return memcached.set(key, value, seconds);
}


bool TCacheMemcachedStore::remove(const QByteArray &key)
{
    TMemcached memcached(Tf::KvsEngine::CacheKvs);
    return memcached.remove(key);
}


void TCacheMemcachedStore::clear()
{
    TMemcached memcached(Tf::KvsEngine::CacheKvs);
    memcached.flushAll();
}


void TCacheMemcachedStore::gc()
{
}


QMap<QString, QVariant> TCacheMemcachedStore::defaultSettings() const
{
    QMap<QString, QVariant> settings {
        {"HostName", "localhost"},
    };
    return settings;
}
