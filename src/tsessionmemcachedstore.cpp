/* Copyright (c) 2023, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsessionmemcachedstore.h"
#include <QByteArray>
#include <QDataStream>
#include <TAppSettings>
#include <TMemcached>
#include <TSystemGlobal>

/*!
  \class TSessionMemcachedStore
  \brief The TSessionMemcachedStore class stores HTTP sessions into Memcached system.
*/

bool TSessionMemcachedStore::store(TSession &session)
{
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << *static_cast<const QVariantMap *>(&session);
    data = Tf::lz4Compress(data);

#ifndef TF_NO_DEBUG
    {
        QByteArray badummy;
        QDataStream dsdmy(&badummy, QIODevice::ReadWrite);
        dsdmy << *static_cast<const QVariantMap *>(&session);

        TSession dummy;
        dsdmy.device()->seek(0);
        dsdmy >> *static_cast<QVariantMap *>(&dummy);
        if (dsdmy.status() != QDataStream::Ok) {
            tSystemError("Failed to store a session into the store. Must set objects that can be serialized.");
        }
    }
#endif

    TMemcached memcached;
    tSystemDebug("TSessionMemcachedStore::store  id:%s", session.id().data());
    return memcached.set('_' + session.id(), data, lifeTimeSecs());
}


TSession TSessionMemcachedStore::find(const QByteArray &id)
{
    TMemcached memcached;
    QByteArray data = memcached.get('_' + id);

    if (data.isEmpty()) {
        return TSession();
    }

    data = Tf::lz4Uncompress(data);
    QDataStream ds(data);
    TSession session(id);
    ds >> *static_cast<QVariantMap *>(&session);

    if (ds.status() != QDataStream::Ok) {
        tSystemError("Failed to load a session from the memcached store.");
    }
    return session;
}


bool TSessionMemcachedStore::remove(const QByteArray &id)
{
    TMemcached memcached;
    return memcached.remove('_' + id);
}


int TSessionMemcachedStore::gc(const QDateTime &)
{
    return 0;
}
