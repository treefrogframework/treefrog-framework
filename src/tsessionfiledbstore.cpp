/* Copyright (c) 2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsessionfiledbstore.h"
#include "tsystemglobal.h"
#include "tcachesqlitestore.h"
#include <TWebApplication>
#include <QDataStream>

constexpr auto SESSION_TABLE = "sess";

/*!
  \class TSessionFileDbStore
  \brief The TSessionFileDbStore class stores HTTP sessions to files.
*/

TSessionFileDbStore::TSessionFileDbStore() :
    _store(new TCacheSQLiteStore(0, SESSION_TABLE))
{
    T_ONCE(TCacheSQLiteStore::createTable(SESSION_TABLE));
    _store->open();
}


TSessionFileDbStore::~TSessionFileDbStore()
{
    _store->close();
    delete _store;
}


bool TSessionFileDbStore::store(TSession &session)
{
    QByteArray buffer;
    QDataStream dsbuf(&buffer, QIODevice::WriteOnly);
    dsbuf << *static_cast<const QVariantMap *>(&session);
    buffer = Tf::lz4Compress(buffer);  // compress
    return _store->set(session.id(), buffer, lifeTimeSecs() * 1000);
}


TSession TSessionFileDbStore::find(const QByteArray &id)
{
    TSession result(id);

    QByteArray buffer = _store->get(id);
    buffer = Tf::lz4Uncompress(buffer);

    if (buffer.isEmpty()) {
        return result;
    }

    QDataStream dsbuf(&buffer, QIODevice::ReadOnly);
    dsbuf >> *static_cast<QVariantMap *>(&result);

    if (dsbuf.status() != QDataStream::Ok) {
        tSystemError("Failed to load a session from the file store.");
    }
    return result;
}


bool TSessionFileDbStore::remove(const QByteArray &id)
{
    return _store->remove(id);
}


int TSessionFileDbStore::gc(const QDateTime &)
{
    _store->gc();
    return 0;
}
