/* Copyright (c) 2012-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TMongoQuery>
#include <TMongoDriver>
#include <TMongoCursor>
#include <TActionContext>
#include <TSystemGlobal>


TMongoQuery::TMongoQuery(const QString &collection)
    : database(TActionContext::current()->getKvsDatabase(TKvsDatabase::MongoDB)),
      nameSpace(), queryLimit(0), queryOffset(0)
{
    nameSpace = database.databaseName() + '.' + collection.trimmed();
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
    if (!database.isValid()) {
        tSystemError("TMongoQuery::find : driver not loaded");
        return false;
    }

    return driver()->find(nameSpace, query, fields, queryLimit, queryOffset, 0);
}


bool TMongoQuery::next()
{
    if (!database.isValid()) {
        return false;
    }

    return driver()->cursor().next();
}


QVariantMap TMongoQuery::value() const
{
    if (!database.isValid())
        return QVariantMap();

    return driver()->cursor().value();
}


QVariantMap TMongoQuery::findOne(const QVariantMap &query, const QStringList &fields)
{
    if (!database.isValid()) {
        tSystemError("TMongoQuery::findOne : driver not loaded");
        return QVariantMap();
    }

    return driver()->findOne(nameSpace, query, fields);
}


bool TMongoQuery::insert(const QVariantMap &object)
{
    if (!database.isValid()) {
        tSystemError("TMongoQuery::insert : driver not loaded");
        return false;
    }

    return driver()->insert(nameSpace, object);
}


bool TMongoQuery::remove(const QVariantMap &query)
{
    if (!database.isValid()) {
        tSystemError("TMongoQuery::remove : driver not loaded");
        return false;
    }

    return driver()->remove(nameSpace, query);
}


bool TMongoQuery::update(const QVariantMap &query, const QVariantMap &object, bool upsert)
{
    if (!database.isValid()) {
        tSystemError("TMongoQuery::update : driver not loaded");
        return false;
    }

    return driver()->update(nameSpace, query, object, upsert);
}


bool TMongoQuery::updateMulti(const QVariantMap &query, const QVariantMap &object, bool upsert)
{
    if (!database.isValid()) {
        tSystemError("TMongoQuery::updateMulti : driver not loaded");
        return false;
    }

    return driver()->updateMulti(nameSpace, query, object, upsert);
}


TMongoDriver *TMongoQuery::driver()
{
#ifdef TF_NO_DEBUG
    return (TMongoDriver *)database.driver();
#else
    TMongoDriver *driver = dynamic_cast<TMongoDriver *>(database.driver());
    if (!driver) {
        throw RuntimeException("cast error", __FILE__, __LINE__);
    }
    return driver;
#endif
}


const TMongoDriver *TMongoQuery::driver() const
{
#ifdef TF_NO_DEBUG
    return (const TMongoDriver *)database.driver();
#else
    const TMongoDriver *driver = dynamic_cast<const TMongoDriver *>(database.driver());
    if (!driver) {
        throw RuntimeException("cast error", __FILE__, __LINE__);
    }
    return driver;
#endif
}
