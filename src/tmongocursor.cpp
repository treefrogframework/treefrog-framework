/* Copyright (c) 2012-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsystemglobal.h"
#include <TBson>
#include <TMongoCursor>
extern "C" {
#include <mongoc.h>
}


TMongoCursor::TMongoCursor()
{
}


TMongoCursor::~TMongoCursor()
{
    release();
}


bool TMongoCursor::next()
{
    bool ret = false;
    bson_error_t error;
    bsonDoc = nullptr;

    if (mongoCursor) {
        ret = mongoc_cursor_next(mongoCursor, (const bson_t **)&bsonDoc);
        if (!ret) {
            if (mongoc_cursor_error(mongoCursor, &error)) {
                tSystemError("MongoDB Cursor Error: %s", error.message);
            }
        }
    }
    return ret;
}


QVariantMap TMongoCursor::value() const
{
    return (mongoCursor && bsonDoc) ? TBson::fromBson(bsonDoc) : QVariantMap();
}


QVariantList TMongoCursor::toList()
{
    QVariantList list;
    while (next()) {
        list.append(value());
    }
    return list;
}


void TMongoCursor::release()
{
    if (mongoCursor) {
        mongoc_cursor_destroy(mongoCursor);
        mongoCursor = nullptr;
    }
    bsonDoc = nullptr;
}


void TMongoCursor::setCursor(void *cursor)
{
    release();
    mongoCursor = (mongoc_cursor_t *)cursor;
}
