/* Copyright (c) 2012-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TMongoDriver>
#include <TMongoCursor>
#include <TBson>
#include <TSystemGlobal>
#include <QDateTime>
#include "mongo.h"


TMongoDriver::TMongoDriver()
    : mongoConnection(0), mongoCursor(new TMongoCursor())
{
    mongoConnection = mongo_create();
    mongo_init(mongoConnection);
}


TMongoDriver::~TMongoDriver()
{
    close();
    delete mongoCursor;
    mongo_destroy(mongoConnection);
    mongo_dispose(mongoConnection);
}


bool TMongoDriver::open(const QString &host)
{
    if (host.isEmpty()) {
        return false;
    }

    mongo_set_op_timeout(mongoConnection, 1000);
    int status = mongo_client(mongoConnection, qPrintable(host), MONGO_DEFAULT_PORT);

    if (status != MONGO_OK) {
        switch (mongoConnection->err) {
        case MONGO_CONN_NO_SOCKET:
            tSystemError("no socket");
            break;

        case MONGO_CONN_FAIL:
            tSystemError("connection failed");
            break;
            
        case MONGO_CONN_NOT_MASTER:
            tSystemError("not master");
            break;

        default:
            break;
        }
    }

    return (status == MONGO_OK);
}


void TMongoDriver::close()
{
    if (isOpen())
        mongo_disconnect(mongoConnection);
}


bool TMongoDriver::isOpen() const
{
    return (bool)mongoConnection->connected;
}


bool TMongoDriver::find(const QString &ns, const QVariantMap &query, const QStringList &fields,
                        int limit, int skip, int options)
{
    mongoCursor->release();
    mongoCursor->init(mongoConnection, ns);

    mongo_cursor *cur = (mongo_cursor *)mongoCursor->cursor();
    mongo_cursor_set_query(cur, (const bson *)TBson::toBson(query).data());
    if (!fields.isEmpty())
        mongo_cursor_set_fields(cur, (const bson *)TBson::toBson(fields).data());
    
    if (limit > 0)
        mongo_cursor_set_limit(cur, limit);
    
    if (skip > 0)
        mongo_cursor_set_skip(cur, skip);
    
    mongo_cursor_set_options(cur, options);
    return true;
}


QVariantMap TMongoDriver::findFirst(const QString &ns, const QVariantMap &query,
                                    const QStringList &fields)
{
    TBson bs;
    int status = mongo_find_one(mongoConnection, qPrintable(ns), (bson *)TBson::toBson(query).data(),
                                (bson *)TBson::toBson(fields).data(), (bson *)bs.data());
    return (status == MONGO_OK) ? TBson::fromBson(bs) : QVariantMap();
}


bool TMongoDriver::insert(const QString &ns, const QVariantMap &object)
{
    int status = mongo_insert(mongoConnection, qPrintable(ns),
                              (const bson *)TBson::toBson(object).constData(), 0);
    return (status == MONGO_OK);
}


bool TMongoDriver::remove(const QString &ns, const QVariantMap &object)
{
    int status = mongo_remove(mongoConnection, qPrintable(ns),
                              (const bson *)TBson::toBson(object).data(), 0);
    return (status == MONGO_OK);
}


bool TMongoDriver::update(const QString &ns, const QVariantMap &query, const QVariantMap &object,
                          bool upsert)
{
    int flag = (upsert) ? MONGO_UPDATE_UPSERT : MONGO_UPDATE_BASIC;
    int status = mongo_update(mongoConnection, qPrintable(ns), (const bson *)TBson::toBson(query).data(),
                              (const bson *)TBson::toBson(object).data(), flag, 0);
    return (status == MONGO_OK);
}


bool TMongoDriver::updateMulti(const QString &ns, const QVariantMap &query, const QVariantMap &object,
                               bool upsert)
{
    int flag = (upsert) ? (MONGO_UPDATE_UPSERT | MONGO_UPDATE_MULTI) : MONGO_UPDATE_MULTI;
    int status = mongo_update(mongoConnection, qPrintable(ns), (const bson *)TBson::toBson(query).data(),
                              (const bson *)TBson::toBson(object).data(), flag, 0);
    return (status == MONGO_OK);
}
