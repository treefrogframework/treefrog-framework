/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TSqlORMapper>
#include <TCriteria>
#include "tsessionsqlobjectstore.h"
#include "tsessionobject.h"

/*!
  \class TSessionSqlObjectStore
  \brief The TSessionSqlObjectStore class stores HTTP sessions into database
         system using object-relational mapping tool.
  \sa TSessionObject
*/

/* create table session ( id varchar(50) primary key, data blob, updated_at datetime ); */

bool TSessionSqlObjectStore::store(TSession &session)
{
    TSqlORMapper<TSessionObject> mapper;
    TCriteria cri(TSessionObject::Id, TSql::Equal, session.id());
    TSessionObject so = mapper.findFirst(cri);

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
        so.id = session.id();
        return so.create();
    }
    return so.update();
}


TSession TSessionSqlObjectStore::find(const QByteArray &id)
{
    QDateTime modified = QDateTime::currentDateTime().addSecs(-lifeTimeSecs());
    TSqlORMapper<TSessionObject> mapper;
    TCriteria cri;
    cri.add(TSessionObject::Id, TSql::Equal, id);
    cri.add(TSessionObject::UpdatedAt, TSql::GreaterEqual, modified);

    TSessionObject so = mapper.findFirst(cri);
    if (so.isNull()) {
        return TSession();
    }

    TSession session(id);
    QDataStream ds(&so.data, QIODevice::ReadOnly);
    ds >> *static_cast<QVariantMap *>(&session);

    if (ds.status() != QDataStream::Ok) {
        tSystemError("Failed to load a session from the sqlobject store.");
    }
    return session;
}


bool TSessionSqlObjectStore::remove(const QByteArray &id)
{
    TSqlORMapper<TSessionObject> mapper;
    int cnt = mapper.removeAll(TCriteria(TSessionObject::Id, id));
    return (cnt > 0);
}


int TSessionSqlObjectStore::gc(const QDateTime &expire)
{
    TSqlORMapper<TSessionObject> mapper;
    TCriteria cri(TSessionObject::UpdatedAt, TSql::LessThan, expire);
    int cnt = mapper.removeAll(cri);
    return cnt;
}
