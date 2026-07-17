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
#include "mongoc/mongoc.h"
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
    _bsonDoc = nullptr;

    if (_mongoCursor) {
        ret = mongoc_cursor_next(_mongoCursor, (const bson_t **)&_bsonDoc);
        if (!ret) {
            if (mongoc_cursor_error(_mongoCursor, &error)) {
                tSystemError("MongoDB Cursor Error: {}", (const char *)error.message);
            }
        }
    }
    return ret;
}


QVariantMap TMongoCursor::value() const
{
    return (_mongoCursor && _bsonDoc) ? TBson::fromBson(_bsonDoc) : QVariantMap();
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
    if (_mongoCursor) {
        mongoc_cursor_destroy(_mongoCursor);
        _mongoCursor = nullptr;
    }
    _bsonDoc = nullptr;
}


void TMongoCursor::setCursor(void *cursor)
{
    release();
    _mongoCursor = (mongoc_cursor_t *)cursor;
}
