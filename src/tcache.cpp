/* Copyright (c) 2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tcache.h"
#include "tcachefactory.h"
#include "tcachestore.h"
#include <TAppSettings>


TCache::TCache()
{
    _cache = TCacheFactory::create(backend());
    _gcDivisor = TAppSettings::instance()->value(Tf::CacheGcProbability).toInt();
}


TCache::~TCache()
{
    TCacheFactory::destroy(backend(), _cache);
}


bool TCache::open()
{
    bool ret = (_cache) ? _cache->open() : false;
    return ret;
}


void TCache::close()
{
    if (_cache) {
        _cache->close();
    }
}


bool TCache::set(const QByteArray &key, const QByteArray &value, qint64 msecs)
{
    bool ret = false;

    if (_cache) {
        if (compressionEnabled()) {
            ret = _cache->set(key, Tf::lz4Compress(value), msecs);
        } else {
            ret = _cache->set(key, value, msecs);
        }

        // GC
        if (_gcDivisor > 0 && Tf::random(1, _gcDivisor) == 1) {
            _cache->gc();
        }
    }
    return ret;
}


QByteArray TCache::get(const QByteArray &key)
{
    QByteArray value;

    if (_cache) {
        value = _cache->get(key);
        if (compressionEnabled()) {
            value = Tf::lz4Uncompress(value);
        }
    }
    return value;
}


void TCache::remove(const QByteArray &key)
{
    if (_cache) {
        _cache->remove(key);
    }
}


void TCache::clear()
{
    if (_cache) {
        _cache->clear();
    }
}


TCache &TCache::instance()
{
    static TCache globalInstance;
    return globalInstance;
}


QString TCache::backend()
{
    static QString cacheBackend = Tf::appSettings()->value(Tf::CacheBackend).toString().toLower();
    return cacheBackend;
}


bool TCache::compressionEnabled()
{
    static bool compression = Tf::appSettings()->value(Tf::CacheEnableCompression).toBool();
    return compression;
}
