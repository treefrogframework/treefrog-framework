/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsessionsqlobjectstore.h"
#include "tsessionobject.h"
#include <TCriteria>
#include <TSqlORMapper>
#include <mutex>


static void createSessionTable()
{
    struct Table {
        static void create()
        {
            TSqlQuery query;
            QString create = "CREATE TABLE IF NOT EXISTS %1 (id VARCHAR(50) PRIMARY KEY, data TEXT, updated_at TIMESTAMP)";
            create = create.arg(TSessionObject().tableName());
            query.exec(create);
        }
    };

    static std::once_flag once;
    std::call_once(once, []() { Table::create(); });
}

/*!
  \class TSessionSqlObjectStore
  \brief The TSessionSqlObjectStore class stores HTTP sessions into database
         system using object-relational mapping tool.
  \details Table schema: 'CREATE TABLE session (id VARCHAR(50) PRIMARY KEY, data TEXT, updated_at TIMESTAMP);'
  \sa TSessionObject
*/


bool TSessionSqlObjectStore::store(TSession &session)
{
    createSessionTable();

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

    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << *static_cast<const QVariantMap *>(&session);
    so.data = QString::fromLatin1(Tf::lz4Compress(data).toBase64());

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
    createSessionTable();

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
    QByteArray data = Tf::lz4Uncompress(QByteArray::fromBase64(so.data.toLatin1()));
    QDataStream ds(&data, QIODevice::ReadOnly);
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
