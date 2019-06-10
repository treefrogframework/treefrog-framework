/* Copyright (c) 2012-2019, AOYAMA Kazuharu
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
extern "C" {
#include "mongoc.h"
}


TMongoDriver::TMongoDriver() :
    mongoCursor(new TMongoCursor())
{
    mongoc_init();
}


TMongoDriver::~TMongoDriver()
{
    close();
    delete mongoCursor;
}


bool TMongoDriver::open(const QString &db, const QString &user, const QString &password, const QString &host, quint16 port, const QString &options)
{
    if (isOpen()) {
        return true;
    }

    if (!port) {
        port = MONGOC_DEFAULT_PORT;
    }
    QString uri;
    if (!user.isEmpty()) {
        uri += user;
        if (!password.isEmpty()) {
            uri += ':';
            uri += password;
            uri += '@';
        }
    }
    uri += host;
    if (!options.isEmpty()) {
        uri += "/?";
        uri += options;
    }

    if (!uri.isEmpty()) {
        uri.prepend(QLatin1String("mongodb://"));
    }

    // connect
    mongoClient = mongoc_client_new(qPrintable(uri));
    if (mongoClient) {
        dbName = db;
    } else {
        tSystemError("MongoDB client create error");
    }
    return (bool)mongoClient;
}


void TMongoDriver::close()
{
    if (isOpen()) {
        mongoc_client_destroy(mongoClient);
        mongoClient = nullptr;
    }
}


bool TMongoDriver::isOpen() const
{
    return (bool)mongoClient;
}


bool TMongoDriver::find(const QString &collection, const QVariantMap &criteria, const QVariantMap &orderBy,
                        const QStringList &fields, int limit, int skip, int )
{
    if (!isOpen()) {
        return false;
    }

    bson_error_t error;
    clearError();

    mongoc_collection_t *col = mongoc_client_get_collection(mongoClient, qPrintable(dbName), qPrintable(collection));
    bson_t *opts = BCON_NEW("skip", BCON_INT64((skip > 0) ? skip : 0));
    if (limit > 0) {
        bson_append_int64(opts, "limit", 5, limit);
    }
    if (! fields.isEmpty()) {
        bson_append_document(opts, "projection", 10, (bson_t *)TBson::toBson(fields).data());
    }
    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(col,
                                                               (bson_t *)TBson::toBson(criteria, orderBy).data(),
                                                               opts, nullptr);

    mongoc_collection_destroy(col);
    bson_destroy(opts);
    mongoCursor->setCursor(cursor);

    if (cursor) {
        if (mongoc_cursor_error(cursor, &error)) {
            tSystemError("MongoDB Find Error: %s", error.message);
            setLastError(&error);
        }
    } else {
        tSystemError("MongoDB Cursor Error");
    }
    return (bool)cursor;
}


QVariantMap TMongoDriver::findOne(const QString &collection, const QVariantMap &criteria,
                                  const QStringList &fields)
{
    QVariantMap ret;

    bool res = find(collection, criteria, QVariantMap(), fields, 1, 0, 0);
    if (res && mongoCursor->next()) {
        ret = mongoCursor->value();
    }
    return ret;
}


bool TMongoDriver::insert(const QString &collection, const QVariantMap &object)
{
    if (!isOpen()) {
        return false;
    }

    bson_error_t error;
    clearError();

    mongoc_collection_t *col = mongoc_client_get_collection(mongoClient, qPrintable(dbName), qPrintable(collection));
    bool res = mongoc_collection_insert(col, MONGOC_INSERT_NONE, (bson_t *)TBson::toBson(object).constData(),
                                        nullptr, &error);

    mongoc_collection_destroy(col);
    if (!res) {
        tSystemError("MongoDB Insert Error: %s", error.message);
        setLastError(&error);
    }
    return res;
}


bool TMongoDriver::remove(const QString &collection, const QVariantMap &object)
{
    if (!isOpen()) {
        return false;
    }

    bson_error_t error;
    clearError();

    mongoc_collection_t *col = mongoc_client_get_collection(mongoClient, qPrintable(dbName), qPrintable(collection));
    bool res = mongoc_collection_remove(col, MONGOC_REMOVE_SINGLE_REMOVE,
                                        (bson_t *)TBson::toBson(object).constData(), nullptr, &error);

    mongoc_collection_destroy(col);

    if (!res) {
        tSystemError("MongoDB Remove Error: %s", error.message);
        setLastError(&error);
    }
    return res;
}


bool TMongoDriver::update(const QString &collection, const QVariantMap &criteria, const QVariantMap &object,
                          bool upsert)
{
    if (!isOpen()) {
        return false;
    }

    bson_error_t error;
    clearError();

    mongoc_update_flags_t flag = (upsert) ? MONGOC_UPDATE_UPSERT : MONGOC_UPDATE_NONE;
    mongoc_collection_t *col = mongoc_client_get_collection(mongoClient, qPrintable(dbName), qPrintable(collection));
    bool res = mongoc_collection_update(col, flag, (bson_t *)TBson::toBson(criteria).data(),
                                        (bson_t *)TBson::toBson(object).data(), nullptr, &error);

    mongoc_collection_destroy(col);

    if (!res) {
        tSystemError("MongoDB Update Error: %s", error.message);
        setLastError(&error);
    }
    return res;
}


bool TMongoDriver::updateMulti(const QString &collection, const QVariantMap &criteria, const QVariantMap &object)
{
    if (!isOpen()) {
        return false;
    }

    bson_error_t error;
    clearError();

    mongoc_collection_t *col = mongoc_client_get_collection(mongoClient, qPrintable(dbName), qPrintable(collection));
    bool res = mongoc_collection_update(col, MONGOC_UPDATE_MULTI_UPDATE,
                                        (bson_t *)TBson::toBson(criteria).data(),
                                        (bson_t *)TBson::toBson(object).data(), nullptr, &error);

    mongoc_collection_destroy(col);

    if (!res) {
        tSystemError("MongoDB UpdateMulti Error: %s", error.message);
        setLastError(&error);
    }
    return res;
}


int TMongoDriver::count(const QString &collection, const QVariantMap &criteria)
{
    if (!isOpen()) {
        return false;
    }

    bson_error_t error;
    clearError();

    mongoc_collection_t *col = mongoc_client_get_collection(mongoClient, qPrintable(dbName), qPrintable(collection));
    int count = mongoc_collection_count(col, MONGOC_QUERY_NONE, (bson_t *)TBson::toBson(criteria).data(),
                                        0, 0, nullptr, &error);

    mongoc_collection_destroy(col);

    if (count < 0) {
        tSystemError("MongoDB Count Error: %s", error.message);
        setLastError(&error);
    }
    return count;
}


void TMongoDriver::clearError()
{
    errorDomain = 0;
    errorCode = 0;
    errorString.resize(0);
}


void TMongoDriver::setLastError(const bson_error_t* error)
{
    errorDomain = error->domain;
    errorCode = error->code;
    errorString = QString::fromLatin1(error->message);
}
