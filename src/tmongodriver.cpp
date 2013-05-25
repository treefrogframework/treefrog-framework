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
    : mongoConnection(new mongo), mongoCursor(new TMongoCursor())
{
    mongo_init(mongoConnection);
}


TMongoDriver::~TMongoDriver()
{
    close();
    delete mongoCursor;
    mongo_destroy(mongoConnection);
    delete mongoConnection;
}


bool TMongoDriver::open(const QString &db, const QString &user, const QString &password, const QString &host, quint16 port, const QString &)
{
    if (host.isEmpty()) {
        return false;
    }

    if (!port)
        port = MONGO_DEFAULT_PORT;

    mongo_clear_errors(mongoConnection);
    mongo_set_op_timeout(mongoConnection, 1000);
    int status = mongo_client(mongoConnection, qPrintable(host), port);

    if (status != MONGO_OK) {
        switch (mongoConnection->err) {
        case MONGO_CONN_NO_SOCKET:
            tSystemError("MongoDB socket error: %s", mongoConnection->lasterrstr);
            break;

        case MONGO_CONN_FAIL:
            tSystemError("MongoDB connection failed: %s", mongoConnection->lasterrstr);
            break;

        case MONGO_CONN_NOT_MASTER:
            tSystemDebug("MongoDB not master: %s", mongoConnection->lasterrstr);
            break;

        default:
            tSystemError("MongoDB error: %s", mongoConnection->lasterrstr);
            break;
        }
        return false;
    }

    if (!user.isEmpty()) {
        status = mongo_cmd_authenticate(mongoConnection, qPrintable(db), qPrintable(user), qPrintable(password));
        if (status != MONGO_OK) {
            tSystemDebug("MongoDB authentication error: %s", mongoConnection->lasterrstr);
            return false;
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


bool TMongoDriver::find(const QString &ns, const QVariantMap &criteria, const QStringList &fields,
                        int limit, int skip, int options)
{
    mongo_clear_errors(mongoConnection);
    mongo_cursor *cursor = mongo_find(mongoConnection, qPrintable(ns), (bson *)TBson::toBson(criteria).data(),
                                      (bson *)TBson::toBson(fields).data(), limit, skip, options);
    mongoCursor->setCursor(cursor);
    if (!cursor) {
        tSystemError("MongoDB Error: %s", mongoConnection->lasterrstr);
    }

    return (bool)cursor;
}


QVariantMap TMongoDriver::findOne(const QString &ns, const QVariantMap &criteria,
                                  const QStringList &fields)
{
    TBson bs;

    mongo_clear_errors(mongoConnection);
    int status = mongo_find_one(mongoConnection, qPrintable(ns), (bson *)TBson::toBson(criteria).data(),
                                (bson *)TBson::toBson(fields).data(), (bson *)bs.data());
    if (status != MONGO_OK) {
        tSystemError("MongoDB Error: %s", mongoConnection->lasterrstr);
        return QVariantMap();
    }
    return TBson::fromBson(bs);
}


bool TMongoDriver::insert(const QString &ns, const QVariantMap &object)
{
    mongo_clear_errors(mongoConnection);
    int status = mongo_insert(mongoConnection, qPrintable(ns),
                              (const bson *)TBson::toBson(object).constData(), 0);
    if (status != MONGO_OK) {
        tSystemError("MongoDB Error: %s", mongoConnection->lasterrstr);
        return false;
    }
    return true;
}


bool TMongoDriver::remove(const QString &ns, const QVariantMap &object)
{
    mongo_clear_errors(mongoConnection);
    int status = mongo_remove(mongoConnection, qPrintable(ns),
                              (const bson *)TBson::toBson(object).data(), 0);
    if (status != MONGO_OK) {
        tSystemError("MongoDB Error: %s", mongoConnection->lasterrstr);
        return false;
    }
    return true;
}


bool TMongoDriver::update(const QString &ns, const QVariantMap &criteria, const QVariantMap &object,
                          bool upsert)
{
    mongo_clear_errors(mongoConnection);
    int flag = (upsert) ? MONGO_UPDATE_UPSERT : MONGO_UPDATE_BASIC;
    int status = mongo_update(mongoConnection, qPrintable(ns), (const bson *)TBson::toBson(criteria).data(),
                              (const bson *)TBson::toBson(object).data(), flag, 0);
    if (status != MONGO_OK) {
        tSystemError("MongoDB Error: %s", mongoConnection->lasterrstr);
        return false;
    }
    return true;
}


bool TMongoDriver::updateMulti(const QString &ns, const QVariantMap &criteria, const QVariantMap &object)
{
    mongo_clear_errors(mongoConnection);
    int status = mongo_update(mongoConnection, qPrintable(ns), (const bson *)TBson::toBson(criteria).data(),
                              (const bson *)TBson::toBson(object).data(), MONGO_UPDATE_MULTI, 0);
   if (status != MONGO_OK) {
        tSystemError("MongoDB Error: %s", mongoConnection->lasterrstr);
        return false;
    }
   return true;
}
