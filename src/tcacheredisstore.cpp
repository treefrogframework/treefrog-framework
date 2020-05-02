/* Copyright (c) 2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tcacheredisstore.h"
#include <TRedis>


TCacheRedisStore::TCacheRedisStore()
{
}


bool TCacheRedisStore::open()
{
    return true;
}


void TCacheRedisStore::close()
{
}


QByteArray TCacheRedisStore::get(const QByteArray &key)
{
    TRedis redis(Tf::KvsEngine::CacheKvs);
    return redis.get(key);
}


bool TCacheRedisStore::set(const QByteArray &key, const QByteArray &value, int seconds)
{
    TRedis redis(Tf::KvsEngine::CacheKvs);
    return redis.setEx(key, value, seconds);
}


bool TCacheRedisStore::remove(const QByteArray &key)
{
    TRedis redis(Tf::KvsEngine::CacheKvs);
    return redis.del(key);
}


void TCacheRedisStore::clear()
{
    TRedis redis(Tf::KvsEngine::CacheKvs);
    return redis.flushDb();
}


void TCacheRedisStore::gc()
{
}


QMap<QString, QVariant> TCacheRedisStore::defaultSettings() const
{
    QMap<QString, QVariant> settings {
        {"HostName", "localhost"},
    };
    return settings;
}
