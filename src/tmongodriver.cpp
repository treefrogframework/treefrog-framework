/* Copyright (c) 2012-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TBson>
#include <TMongoCursor>
#include <TMongoDriver>
#include <TSystemGlobal>
extern "C" {
#include "mongoc.h"
#if !MONGOC_CHECK_VERSION(1, 9, 0)
#error Supports for MongoDB C driver version 1.9.0 or later.
#endif
}
#include <QDateTime>


TMongoDriver::TMongoDriver() :
    mongoCursor(new TMongoCursor)
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
    uri.reserve(256);
    if (!user.isEmpty()) {
        uri += user;
        if (!password.isEmpty()) {
            uri += ':';
            uri += password;
            uri += '@';
        }
    }
    uri += (host.isEmpty()) ? QStringLiteral("127.0.0.1") : host;
    if (!options.isEmpty()) {
        uri += QLatin1String("/?");
        uri += options;
    }

    if (!uri.startsWith("mongodb://") && !uri.startsWith("mongodb+srv://")) {
        uri.prepend(QLatin1String("mongodb://"));
    }

    // connect
    mongoClient = mongoc_client_new(qUtf8Printable(uri));
    if (mongoClient) {
        dbName = db;
        serverVersionNumber();  // Gets server version
    } else {
        tSystemError("MongoDB client create error. Connection URI: %s", qUtf8Printable(uri));
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
    const QStringList &fields, int limit, int skip)
{
    if (!isOpen()) {
        return false;
    }

    bson_error_t error;
    clearError();

    mongoc_collection_t *col = mongoc_client_get_collection(mongoClient, qUtf8Printable(dbName), qUtf8Printable(collection));
    if (!col) {
        tSystemError("MongoDB GetCollection Error");
        return false;
    }

    bson_t *opts = BCON_NEW("skip", BCON_INT64((skip > 0) ? skip : 0));
    if (limit > 0) {
        bson_append_int64(opts, "limit", 5, limit);
    }

    if (!fields.isEmpty()) {
        bson_append_document(opts, "projection", 10, (bson_t *)TBson::toBson(fields).data());
    }

    mongoc_cursor_t *cursor = nullptr;
    if (serverVersionNumber() < 0x030200) {
        cursor = mongoc_collection_find_with_opts(col,
            (bson_t *)TBson::toBson(criteria, orderBy).data(),
            opts, nullptr);
    } else {
        if (!orderBy.isEmpty()) {
            bson_append_document(opts, "sort", 4, (bson_t *)TBson::toBson(orderBy).data());
        }
        cursor = mongoc_collection_find_with_opts(col,
            (bson_t *)TBson::toBson(criteria).data(),
            opts, nullptr);
    }
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

    mongoc_collection_destroy(col);
    return (bool)cursor;
}


QVariantMap TMongoDriver::findOne(const QString &collection, const QVariantMap &criteria,
    const QStringList &fields)
{
    QVariantMap ret;

    bool res = find(collection, criteria, QVariantMap(), fields, 1, 0);
    if (res && mongoCursor->next()) {
        ret = mongoCursor->value();
    }
    return ret;
}


bool TMongoDriver::insertOne(const QString &collection, const QVariantMap &object, QVariantMap *reply)
{
    if (!isOpen()) {
        return false;
    }

    bson_error_t error;
    clearError();

    mongoc_collection_t *col = mongoc_client_get_collection(mongoClient, qUtf8Printable(dbName), qUtf8Printable(collection));
    bson_t rep;
    bool res = mongoc_collection_insert_one(col, (bson_t *)TBson::toBson(object).constData(),
        nullptr, &rep, &error);
    mongoc_collection_destroy(col);

    if (res) {
        if (reply) {
            *reply = TBson::fromBson((TBsonObject *)&rep);
        }
    } else {
        tSystemError("MongoDB Insert Error: %s", error.message);
        setLastError(&error);
    }
    return res;
}


bool TMongoDriver::removeOne(const QString &collection, const QVariantMap &criteria, QVariantMap *reply)
{
    if (!isOpen()) {
        return false;
    }

    bson_error_t error;
    clearError();

    mongoc_collection_t *col = mongoc_client_get_collection(mongoClient, qUtf8Printable(dbName), qUtf8Printable(collection));
    bson_t rep;
    bool res = mongoc_collection_delete_one(col, (bson_t *)TBson::toBson(criteria).constData(), nullptr, &rep, &error);
    mongoc_collection_destroy(col);

    if (res) {
        if (reply) {
            *reply = TBson::fromBson((TBsonObject *)&rep);
        }
    } else {
        tSystemError("MongoDB Remove Error: %s", error.message);
        setLastError(&error);
    }
    return res;
}


bool TMongoDriver::removeMany(const QString &collection, const QVariantMap &criteria, QVariantMap *reply)
{
    if (!isOpen()) {
        return false;
    }

    bson_error_t error;
    clearError();

    mongoc_collection_t *col = mongoc_client_get_collection(mongoClient, qUtf8Printable(dbName), qUtf8Printable(collection));
    bson_t rep;
    bool res = mongoc_collection_delete_many(col, (bson_t *)TBson::toBson(criteria).constData(), nullptr, &rep, &error);
    mongoc_collection_destroy(col);

    if (res) {
        if (reply) {
            *reply = TBson::fromBson((TBsonObject *)&rep);
        }
    } else {
        tSystemError("MongoDB Remove Error: %s", error.message);
        setLastError(&error);
    }
    return res;
}


bool TMongoDriver::updateOne(const QString &collection, const QVariantMap &criteria, const QVariantMap &object,
    bool upsert, QVariantMap *reply)
{
    if (!isOpen()) {
        return false;
    }

    bson_error_t error;
    clearError();

    mongoc_collection_t *col = mongoc_client_get_collection(mongoClient, qUtf8Printable(dbName), qUtf8Printable(collection));
    bson_t rep;
    bson_t *opts = BCON_NEW("upsert", BCON_BOOL(upsert));
    bool res = mongoc_collection_update_one(col, (bson_t *)TBson::toBson(criteria).data(),
        (bson_t *)TBson::toBson(object).data(), opts, &rep, &error);
    bson_free(opts);
    mongoc_collection_destroy(col);

    if (res) {
        if (reply) {
            *reply = TBson::fromBson((TBsonObject *)&rep);
        }
    } else {
        tSystemError("MongoDB Update Error: %s", error.message);
        setLastError(&error);
    }
    return res;
}


bool TMongoDriver::updateMany(const QString &collection, const QVariantMap &criteria, const QVariantMap &object, bool upsert, QVariantMap *reply)
{
    if (!isOpen()) {
        return false;
    }

    bson_error_t error;
    clearError();

    mongoc_collection_t *col = mongoc_client_get_collection(mongoClient, qUtf8Printable(dbName), qUtf8Printable(collection));
    bson_t rep;
    bson_t *opts = BCON_NEW("upsert", BCON_BOOL(upsert));
    bool res = mongoc_collection_update_many(col, (bson_t *)TBson::toBson(criteria).data(),
        (bson_t *)TBson::toBson(object).data(), opts, &rep, &error);
    bson_free(opts);
    mongoc_collection_destroy(col);

    if (res) {
        if (reply) {
            *reply = TBson::fromBson((TBsonObject *)&rep);
        }
    } else {
        tSystemError("MongoDB UpdateMulti Error: %s", error.message);
        setLastError(&error);
    }
    return res;
}


qint64 TMongoDriver::count(const QString &collection, const QVariantMap &criteria)
{
    qint64 count = -1;

    if (!isOpen()) {
        return count;
    }

    bson_error_t error;
    clearError();

    mongoc_collection_t *col = mongoc_client_get_collection(mongoClient, qUtf8Printable(dbName), qUtf8Printable(collection));
#if MONGOC_CHECK_VERSION(1, 11, 0)
    count = mongoc_collection_count_documents(col, (bson_t *)TBson::toBson(criteria).data(), nullptr, nullptr, nullptr, &error);
#else
    count = mongoc_collection_count(col, MONGOC_QUERY_NONE, (bson_t *)TBson::toBson(criteria).data(), 0, 0, nullptr, &error);
#endif
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


void TMongoDriver::setLastError(const bson_error_t *error)
{
    errorDomain = error->domain;
    errorCode = error->code;
    errorString = QString::fromLatin1(error->message);
}


QStringList TMongoDriver::getCollectionNames()
{
    QStringList names;
    bson_t opts = BSON_INITIALIZER;
    bson_error_t error;
    char **strv = nullptr;

    if (!isOpen()) {
        return names;
    }

    auto *database = mongoc_client_get_database(mongoClient, qUtf8Printable(dbName));
    auto *rc = mongoc_read_concern_new();
    mongoc_read_concern_set_level(rc, MONGOC_READ_CONCERN_LEVEL_LOCAL);
    mongoc_read_concern_append(rc, &opts);
    strv = mongoc_database_get_collection_names_with_opts(database, &opts, &error);

    if (strv) {
        for (int i = 0; strv[i]; i++) {
            names << QString::fromUtf8(strv[i]);
        }
        bson_strfreev(strv);
    } else {
        tSystemError("MongoDB get_collection_names error: %s", error.message);
        setLastError(&error);
    }

    mongoc_read_concern_destroy(rc);
    bson_destroy(&opts);
    mongoc_database_destroy(database);
    return names;
}


QString TMongoDriver::serverVersion()
{
    if (!isOpen()) {
        return QString();
    }

    bson_t rep;
    bson_error_t error;

    TBson bson = TBson::toBson(QVariantMap({{"buildinfo", 1}}));
    mongoc_client_command_simple(mongoClient, "admin", (bson_t *)bson.data(), nullptr, &rep, &error);
    auto map = TBson::fromBson((TBsonObject *)&rep);
    bson_destroy(&rep);

    QString version = map.value("version").toString();
    tSystemDebug("MongoDB server version: %s", qUtf8Printable(version));
    return version;
}


int TMongoDriver::serverVersionNumber()
{
    if (serverVerionNumber < 0) {
        int number = 0;
        QString version = serverVersion();

        if (!version.isEmpty()) {
            auto vers = version.split('.', Tf::SkipEmptyParts);
            for (auto &v : vers) {
                number <<= 8;
                number |= v.toInt() & 0xFF;
            }

            serverVerionNumber = number;
            tSystemDebug("MongoDB server version number: %x", number);
        }
    }
    return serverVerionNumber;
}
