/* Copyright (c) 2010-2015, AOYAMA Kazuharu
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

    QDataStream ds(&so.data, QIODevice::WriteOnly);
    ds << *static_cast<const QVariantMap *>(&session);

    if (so.isEmpty()) {
        so.id = session.id();
        return so.create();
    }
    return so.update();
}


TSession TSessionSqlObjectStore::find(const QByteArray &id)
{
    QDateTime modified = QDateTime::currentDateTime().addSecs(-lifeTimeSecs);
    TSqlORMapper<TSessionObject> mapper;
    TCriteria cri;
    cri.add(TSessionObject::Id, TSql::Equal, id);
    cri.add(TSessionObject::UpdatedAt, TSql::GreaterEqual, modified);

    TSessionObject sess = mapper.findFirst(cri);
    if (sess.isEmpty())
        return TSession();

    TSession result(id);
    QDataStream ds(&sess.data, QIODevice::ReadOnly);
    ds >> *static_cast<QVariantMap *>(&result);
    return result;
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
