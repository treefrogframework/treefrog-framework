/* Copyright (c) 2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TMongoODMapper>
#include <TCriteria>
#include "tsessionmongoobjectstore.h"
#include "tsessionmongoobject.h"

/*!
  \class TSessionMongoObjectStore
  \brief The TSessionMongoObjectStore class stores HTTP sessions into MongoDB
         system using object-document mapping tool.
  \sa TSessionMongoObject
*/

bool TSessionMongoObjectStore::store(TSession &session)
{
    TMongoODMapper<TSessionMongoObject> mapper;
    TCriteria cri;
    cri.add(TSessionMongoObject::SessionId, TMongo::Equal, session.id());
    TSessionMongoObject so = mapper.findOne(cri);

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

    QDataStream ds(&so.data, QIODevice::WriteOnly);
    ds << *static_cast<const QVariantMap *>(&session);

    if (ds.status() != QDataStream::Ok) {
        tSystemError("Failed to store session. Must set objects that can be serialized.");
        return false;
    }

    if (so.isNull()) {
        so.sessionId = session.id();
        return so.create();
    }
    return so.update();
}


TSession TSessionMongoObjectStore::find(const QByteArray &id)
{
    QDateTime modified = QDateTime::currentDateTime().addSecs(-lifeTimeSecs());
    TMongoODMapper<TSessionMongoObject> mapper;
    TCriteria cri;
    cri.add(TSessionMongoObject::SessionId, TMongo::Equal, id);
    cri.add(TSessionMongoObject::UpdatedAt, TMongo::GreaterEqual, modified);

    TSessionMongoObject so = mapper.findOne(cri);
    if (so.isNull()) {
        return TSession();
    }

    TSession session(id);
    QDataStream ds(&so.data, QIODevice::ReadOnly);
    ds >> *static_cast<QVariantMap *>(&session);

    if (ds.status() != QDataStream::Ok) {
        tSystemError("Failed to load a session from the mongoobject store.");
    }
    return session;
}


bool TSessionMongoObjectStore::remove(const QByteArray &id)
{
    TMongoODMapper<TSessionMongoObject> mapper;
    int cnt = mapper.removeAll(TCriteria(TSessionMongoObject::Id, id));
    return (cnt > 0);
}


int TSessionMongoObjectStore::gc(const QDateTime &expire)
{
    TMongoODMapper<TSessionMongoObject> mapper;
    TCriteria cri(TSessionMongoObject::UpdatedAt, TMongo::LessThan, expire);
    int cnt = mapper.removeAll(cri);
    return cnt;
}
