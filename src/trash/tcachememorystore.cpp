/* Copyright (c) 2022, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tcachememorystore.h"
#include <QMap>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QWriteLocker>


/*!
  \class TCacheMemoryStore
  \brief The TCacheMemoryStore class represents a on-memory store for cache on
         the server process.
*/


static QMap<QByteArray, QPair<int64_t, QByteArray>> memmap;
static QReadWriteLock lock;


TCacheMemoryStore::TCacheMemoryStore()
{
}


bool TCacheMemoryStore::open()
{
    return true;
}


void TCacheMemoryStore::close()
{
}


QByteArray TCacheMemoryStore::get(const QByteArray &key)
{
    QReadLocker locker(&lock);

    auto val = memmap.value(key);
    if (val.first < QDateTime::currentDateTime().toMSecsSinceEpoch()) {
        return QByteArray();
    }
    return val.second;
}


bool TCacheMemoryStore::set(const QByteArray &key, const QByteArray &value, int seconds)
{
    QWriteLocker locker(&lock);
    int64_t expire = QDateTime::currentDateTime().toMSecsSinceEpoch() + seconds * 1000;
    memmap.insert(key, QPair<int64_t, QByteArray>(expire, value));
    return true;
}


bool TCacheMemoryStore::remove(const QByteArray &key)
{
    QWriteLocker locker(&lock);
    memmap.remove(key);
    return true;
}


void TCacheMemoryStore::clear()
{
    QWriteLocker locker(&lock);
    memmap.clear();
}


void TCacheMemoryStore::gc()
{
    QWriteLocker locker(&lock);

    auto current = QDateTime::currentDateTime().toMSecsSinceEpoch();
    for (auto it = memmap.begin(); it != memmap.end(); ++it) {
        if (it.value().first < current) {
            it = memmap.erase(it);
        }
    }
}


QMap<QString, QVariant> TCacheMemoryStore::defaultSettings() const
{
    return QMap<QString, QVariant>();
}
