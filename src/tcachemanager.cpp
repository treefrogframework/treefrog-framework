/* Copyright (c) 2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tcachemanager.h"
#include "tcachefactory.h"
#include "tcachestore.h"
#include <TAppSettings>

bool TCacheManager::compression = true;


TCacheManager::TCacheManager()
{
    _gcDivisor = TAppSettings::instance()->value(Tf::CacheGcProbability).toInt();
}


TCacheManager::~TCacheManager()
{ }


bool TCacheManager::open()
{
    bool ret = false;
    TCacheStore *store = TCacheFactory::create(backend());

    if (store) {
        ret = store->open();
        TCacheFactory::destroy(backend(), store);
    }
    return ret;
}


void TCacheManager::close()
{
    TCacheStore *store = TCacheFactory::create(backend());

    if (store) {
        store->close();
        TCacheFactory::destroy(backend(), store);
    }
}


bool TCacheManager::set(const QByteArray &key, const QByteArray &value, qint64 msecs)
{
    bool ret = false;
    TCacheStore *store = TCacheFactory::create(backend());

    if (store) {
        ret = store->set(key, value, msecs);

        if (compression) {
            ret = store->set(key, Tf::lz4Compress(value), msecs);
        } else {
            ret = store->set(key, value, msecs);
        }

        // GC
        if (_gcDivisor > 0 && Tf::random(1, _gcDivisor) == 1) {
            store->gc();
        }

        TCacheFactory::destroy(backend(), store);
    }
    return ret;
}


QByteArray TCacheManager::get(const QByteArray &key)
{
    QByteArray value;
    TCacheStore *store = TCacheFactory::create(backend());

    if (store) {
        value = store->get(key);
        if (compression) {
            value = Tf::lz4Uncompress(value);
        }
        TCacheFactory::destroy(backend(), store);
    }
    return value;
}


void TCacheManager::remove(const QByteArray &key)
{
    TCacheStore *store = TCacheFactory::create(backend());

    if (store) {
        store->remove(key);
        TCacheFactory::destroy(backend(), store);
    }
}


void TCacheManager::clear()
{
    TCacheStore *store = TCacheFactory::create(backend());

    if (store) {
        store->clear();
        TCacheFactory::destroy(backend(), store);
    }
}


QString TCacheManager::backend() const
{
    static QString cacheBackend = Tf::appSettings()->value(Tf::CacheBackend).toString().toLower();
    return cacheBackend;
}


TCacheManager &TCacheManager::instance()
{
    static TCacheManager manager;
    return manager;
}
