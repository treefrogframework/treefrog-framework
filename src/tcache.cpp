/* Copyright (c) 2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tcachefactory.h"
#include "tcachestore.h"
#include <TAppSettings>
#include <TCache>
#include <TWebApplication>

/*!
  \class TCache
  \brief The TCache class stores items so that can be served faster.
*/

TCache::TCache()
{
    static int CacheGcProbability = TAppSettings::instance()->value(Tf::CacheGcProbability, 0).toInt();
    _gcDivisor = CacheGcProbability;

    if (Tf::app()->cacheEnabled()) {
        _cache = TCacheFactory::create(Tf::app()->cacheBackend());
        if (_cache) {
            if (!_cache->open()) {
                tError() << "Failed to open cache. Check the settings of cache.ini.";
                TCacheFactory::destroy(Tf::app()->cacheBackend(), _cache);
                _cache = nullptr;
            }
        }
    } else {
        tWarn() << "Cache not available. Check the settings of application.ini.";
    }
}


TCache::~TCache()
{
    if (_cache) {
        _cache->close();
        TCacheFactory::destroy(Tf::app()->cacheBackend(), _cache);
    }
}

/*!
  Stores a new item with the \a key and a \a value in the cache and sets the
  timeout after a given number of \a seconds.
 */
bool TCache::set(const QByteArray &key, const QByteArray &value, int seconds)
{
    bool ret = false;

    if (_cache) {
        if (compressionEnabled()) {
            ret = _cache->set(key, Tf::lz4Compress(value), seconds);
        } else {
            ret = _cache->set(key, value, seconds);
        }

        // GC
        if (_gcDivisor > 0 && Tf::random(1, _gcDivisor) == 1) {
            _cache->gc();
        }
    }
    return ret;
}

/*!
  Returns the value associated with the \a key.
 */
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

/*!
  Removes the item that have the \a key from the cache.
 */
void TCache::remove(const QByteArray &key)
{
    if (_cache) {
        _cache->remove(key);
    }
}

/*!
  Removes all items from the cache.
 */
void TCache::clear()
{
    if (_cache) {
        _cache->clear();
    }
}


bool TCache::compressionEnabled()
{
    static bool compression = Tf::appSettings()->value(Tf::CacheEnableCompression, true).toBool();
    return compression;
}
