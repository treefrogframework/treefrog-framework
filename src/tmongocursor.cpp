/* Copyright (c) 2012-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TMongoCursor>
#include <TBson>
#include "mongo.h"


TMongoCursor::TMongoCursor()
    : mongoCursor(0)
{ }


TMongoCursor::~TMongoCursor()
{
    release();
}


bool TMongoCursor::next()
{
    return (mongoCursor) ? (mongo_cursor_next((mongo_cursor *)mongoCursor) == MONGO_OK) : false;
}


QVariantMap TMongoCursor::value() const
{
    if (mongoCursor) {
        const bson *bs = mongo_cursor_bson((mongo_cursor *)mongoCursor);
        return (bs) ? TBson::fromBson(bs) : QVariantMap();
    }
    return QVariantMap();
}


QVariantList TMongoCursor::toList()
{
    QVariantList list;
    while (next()) {
        list.append(value());
    }
    return list;
}


void TMongoCursor::init(mongo *connection, const QString &ns)
{
    release();

    mongoCursor = mongo_cursor_alloc();
    ((mongo_cursor *)mongoCursor)->flags |= MONGO_CURSOR_MUST_FREE;
    mongo_cursor_init((mongo_cursor *)mongoCursor, connection, qPrintable(ns));
}


void TMongoCursor::release()
{
    if (mongoCursor) {
        mongo_cursor_destroy((mongo_cursor *)mongoCursor);
        mongoCursor = 0;
    }
}


void TMongoCursor::setCursor(TMongoCursorObject *cursor)
{
    release();
    mongoCursor = cursor;
}
