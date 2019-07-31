/* Copyright (c) 2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tcache.h"
#include "tcachefactory.h"
#include "tcachesqlitestore.h"
#include "tsystemglobal.h"
#include <TAppSettings>
#include <QtCore>


TCache::TCache(BackEnd backend, bool lz4Compression) :
    _compression(lz4Compression)
{
    _gcDivisor = TAppSettings::instance()->value(Tf::CacheGcProbability).toInt();
    _cacheStore = TCacheFactory::create("singlefile");
}


TCache::~TCache()
{
    TCacheFactory::destroy("singlefile", _cacheStore);
}


bool TCache::open()
{
    return  (_cacheStore) ? _cacheStore->open() : false;
}


void TCache::close()
{
    if (_cacheStore) {
        _cacheStore->close();
    }
}


bool TCache::set(const QByteArray &key, const QByteArray &value, qint64 msecs)
{
    bool ret = false;

    if (! _cacheStore) {
        return ret;
    }

    if (_compression) {
        ret = _cacheStore->set(key, Tf::lz4Compress(value), msecs);
    } else {
        ret = _cacheStore->set(key, value, msecs);
    }

    // GC
    if (_gcDivisor > 0 && Tf::random(1, _gcDivisor) == 1) {
        _cacheStore->gc();
    }
    return ret;
}


QByteArray TCache::get(const QByteArray &key)
{
    QByteArray value;

    if (! _cacheStore) {
        return value;
    }

    value = _cacheStore->get(key);
    if (_compression) {
        value = Tf::lz4Uncompress(value);
    }
    return value;
}


void TCache::remove(const QByteArray &key)
{
    if (_cacheStore) {
        _cacheStore->remove(key);
    }
}


void TCache::clear()
{
    if (_cacheStore) {
        _cacheStore->clear();
    }
}
