/* Copyright (c) 2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tcache.h"
#include "tsqliteblobstore.h"
#include "tsystemglobal.h"
#include <QtCore>

constexpr auto CACHE_FILE = "cache_db";


TCache::TCache(qint64 thresholdFileSize, int gcDivisor) :
    _thresholdFileSize(thresholdFileSize),
    _gcDivisor(qMax(1, gcDivisor)),
    _blobStore(new TSQLiteBlobStore(CACHE_FILE))
{
    _blobStore->open();
}


TCache::~TCache()
{
    delete _blobStore;
}


bool TCache::insert(const QByteArray &key, const QByteArray &value, int seconds)
{
    if (key.isEmpty() || value.isEmpty() || seconds <= 0) {
        return false;
    }

    qint64 expire = QDateTime::currentMSecsSinceEpoch() + seconds * 1000;
    _blobStore->remove(key);
    bool ret = _blobStore->write(key, value, expire);

    // GC
    if (Tf::random(1, _gcDivisor) == 1) {
        gc();
    }

    return ret;
}


QByteArray TCache::value(const QByteArray &key, const QByteArray &defaultValue)
{
    QByteArray blob;
    qint64 expire = 0;
    qint64 current = QDateTime::currentMSecsSinceEpoch();

    if (_blobStore->read(key, blob, expire)) {
        if (expire > current) {
            return blob;
        } else {
            remove(key);
        }
    }
    return defaultValue;
}


QByteArray TCache::take(const QByteArray &key)
{
    QByteArray val = value(key);
    if (! val.isNull()) {
        remove(key);
    }
    return val;
}


void TCache::remove(const QByteArray &key)
{
    _blobStore->remove(key);
}


void TCache::clear()
{
    _blobStore->removeAll();
    _blobStore->vacuum();
}


int TCache::count() const
{
    return _blobStore->count();
}


bool TCache::setup()
{
    return TSQLiteBlobStore::setup(CACHE_FILE);
}


qint64 TCache::fileSize() const
{
    return QFileInfo(CACHE_FILE).size();
}


void TCache::gc()
{
    int removed = 0;
    qint64 current = QDateTime::currentMSecsSinceEpoch();

    if (_thresholdFileSize > 0 && fileSize() > _thresholdFileSize) {
        for (int i = 0; i < 8; i++) {
            removed += _blobStore->removeOlder(count() * 0.3);
            _blobStore->vacuum();
            if (fileSize() < _thresholdFileSize * 0.8) {
                break;
            }
        }
        tSystemDebug("removeOlder: %d\n", removed);
    } else {
        removed = _blobStore->removeOlderThan(current);
        _blobStore->vacuum();
        tSystemDebug("removeOlderThan: %d\n", removed);
    }
}
