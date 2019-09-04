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

constexpr auto SESSION_DB_FILE = "sessiondb";


class Store {
public:
    Store();
    ~Store() { _store->close(); delete _store; }
    TCacheSQLiteStore &operator()() const { return *_store; }

private:
    TCacheSQLiteStore *_store {nullptr};
};

Store::Store()
{
    static QString path = Tf::app()->tmpPath() + QLatin1String(SESSION_DB_FILE);
    _store = new TCacheSQLiteStore(path);
    _store->open();
}


/*!
  \class TSessionFileDbStore
  \brief The TSessionFileDbStore class stores HTTP sessions to files.
*/

bool TSessionFileDbStore::store(TSession &session)
{
    QByteArray buffer;
    QDataStream dsbuf(&buffer, QIODevice::WriteOnly);
    dsbuf << *static_cast<const QVariantMap *>(&session);
    buffer = Tf::lz4Compress(buffer);  // compress

    Store store;
    return store().set(session.id(), buffer, lifeTimeSecs() * 1000);
}


TSession TSessionFileDbStore::find(const QByteArray &id)
{
    TSession result(id);
    Store store;
    QByteArray buffer = store().get(id);
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
    Store store;
    return store().remove(id);
}


int TSessionFileDbStore::gc(const QDateTime &)
{
    Store store;
    store().gc();
    return 0;
}
