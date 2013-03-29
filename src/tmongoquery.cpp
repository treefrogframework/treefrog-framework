/* Copyright (c) 2012-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TMongoQuery>
#include <TMongoDriver>
#include <TMongoCursor>


TMongoQuery::TMongoQuery(const QString &ns, TMongoDatabase db)
    : database(db), nameSpace(ns), queryLimit(0), queryOffset(0)
{
    if (!database.isValid()) {
        database = TMongoDatabase::database();
    }
}


TMongoQuery::TMongoQuery(const TMongoQuery &other)
    : database(other.database), nameSpace(other.nameSpace),
      queryLimit(other.queryLimit), queryOffset(other.queryOffset)
{ }


TMongoQuery &TMongoQuery::operator=(const TMongoQuery &other)
{
    database = other.database;
    nameSpace = other.nameSpace;
    queryLimit = other.queryLimit;
    queryOffset = other.queryOffset;
    return *this;
}


bool TMongoQuery::find(const QVariantMap &query, const QStringList &fields)
{
    database.driver()->find(nameSpace, query, fields, queryLimit, queryOffset, 0);
    return true;
}


bool TMongoQuery::next()
{
    return database.driver()->cursor().next();
}


QVariantMap TMongoQuery::value() const
{
    return database.driver()->cursor().value();
}


QVariantMap TMongoQuery::findFirst(const QVariantMap &query, const QStringList &fields)
{
    return database.driver()->findFirst(nameSpace, query, fields);
}


bool TMongoQuery::insert(const QVariantMap &object)
{
    return database.driver()->insert(nameSpace, object);
}


bool TMongoQuery::remove(const QVariantMap &object)
{
    return database.driver()->remove(nameSpace, object);
}


bool TMongoQuery::update(const QVariantMap &query, const QVariantMap &object, bool upsert)
{
    return database.driver()->update(nameSpace, query, object, upsert);
}


bool TMongoQuery::updateMulti(const QVariantMap &query, const QVariantMap &object, bool upsert)
{
    return database.driver()->updateMulti(nameSpace, query, object, upsert);
}
