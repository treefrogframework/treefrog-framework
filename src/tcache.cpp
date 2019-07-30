/* Copyright (c) 2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tcache.h"
#include "tcachesqlitestore.h"
#include "tsystemglobal.h"
#include <QtCore>

constexpr auto CACHE_FILE = "cache_db";


TCache::TCache(CacheType, bool lz4Compression, int gcDivisor) :
    _compression(lz4Compression),
    _gcDivisor(qMax(1, gcDivisor)),
    _cacheStore(new TCacheSQLiteStore(CACHE_FILE))
{
    _cacheStore->open();
}


TCache::~TCache()
{
    _cacheStore->close();
    delete _cacheStore;
}


bool TCache::set(const QByteArray &key, const QByteArray &value, qint64 msecs)
{
    bool ret = false;

    if (_compression) {
        ret = _cacheStore->set(key, Tf::lz4Compress(value), msecs);
    } else {
        ret = _cacheStore->set(key, value, msecs);
    }

    // GC
    if (Tf::random(1, _gcDivisor) == 1) {
        _cacheStore->gc();
    }
    return ret;
}


QByteArray TCache::get(const QByteArray &key)
{
    QByteArray value = _cacheStore->get(key);
    if (_compression) {
        value = Tf::lz4Uncompress(value);
    }
    return value;
}


void TCache::remove(const QByteArray &key)
{
    _cacheStore->remove(key);
}


void TCache::clear()
{
    _cacheStore->clear();
}
