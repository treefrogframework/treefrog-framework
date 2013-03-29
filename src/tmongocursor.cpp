/* Copyright (c) 2012-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TMongoCursor>
#include <TBson>
#include "mongo.h"


TMongoCursor::TMongoCursor()
    : initiated(false)
{
    mongoCursor = mongo_cursor_create();
}


TMongoCursor::~TMongoCursor()
{
    release();
    mongo_cursor_dispose((mongo_cursor *)mongoCursor);
}


bool TMongoCursor::next()
{
    return (mongo_cursor_next((mongo_cursor *)mongoCursor) == MONGO_OK);
}


QVariantMap TMongoCursor::value() const
{
    const bson *bs = mongo_cursor_bson((mongo_cursor *)mongoCursor);
    return (bs) ? TBson::fromBson(bs) : QVariantMap();
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
    mongo_cursor_init((mongo_cursor *)mongoCursor, connection, qPrintable(ns));
    initiated = true;
}


void TMongoCursor::release()
{
    if (initiated) {
        mongo_cursor_destroy((mongo_cursor *)mongoCursor);
        initiated = false;
    }
}
