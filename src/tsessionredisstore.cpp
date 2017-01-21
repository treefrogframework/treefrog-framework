/* Copyright (c) 2015-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QByteArray>
#include <QDataStream>
#include <TAppSettings>
#include <TRedis>
#include <TSystemGlobal>
#include "tsessionredisstore.h"

/*!
  \class TSessionRedisStore
  \brief The TSessionRedisStore class stores HTTP sessions into Redis system.
*/

bool TSessionRedisStore::store(TSession &session)
{
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << *static_cast<const QVariantMap *>(&session);

#ifndef TF_NO_DEBUG
    {
        QByteArray badummy;
        QDataStream dsdmy(&badummy, QIODevice::ReadWrite);
        dsdmy << *static_cast<const QVariantMap *>(&session);

        TSession dummy;
        dsdmy.device()->seek(0);
        dsdmy >> *static_cast<QVariantMap *>(&dummy);
        if (dsdmy.status() != QDataStream::Ok) {
            tSystemError("Failed to store a session into the cookie store. Must set objects that can be serialized.");
        }
    }
#endif

    TRedis redis;
    tSystemDebug("TSessionRedisStore::store  id:%s", session.id().data());
    return redis.setEx('_' + session.id(), data, lifeTimeSecs());
}


TSession TSessionRedisStore::find(const QByteArray &id)
{
    TRedis redis;
    QByteArray data = redis.get('_' + id);

    if (data.isEmpty()) {
        return TSession();
    }

    QDataStream ds(data);
    TSession session(id);
    ds >> *static_cast<QVariantMap *>(&session);

    if (ds.status() != QDataStream::Ok) {
        tSystemError("Failed to load a session from the redis store.");
    }
    return session;
}


bool TSessionRedisStore::remove(const QByteArray &id)
{
    TRedis redis;
    return redis.del('_' + id);
}


int TSessionRedisStore::gc(const QDateTime &)
{
    return 0;
}
