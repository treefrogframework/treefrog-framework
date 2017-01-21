/* Copyright (c) 2012-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TMongoCursor>
#include <TBson>
extern "C" {
#include "mongoc.h"
}


TMongoCursor::TMongoCursor()
    : mongoCursor(nullptr), bsonDoc(nullptr)
{ }


TMongoCursor::~TMongoCursor()
{
    release();
}


bool TMongoCursor::next()
{
    bsonDoc = nullptr;
    return (mongoCursor) ? mongoc_cursor_next(mongoCursor, (const bson_t **)&bsonDoc) : false;
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
