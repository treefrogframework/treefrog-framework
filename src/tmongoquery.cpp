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

/*!
  \class TMongoQuery
  \brief The TMongoQuery class provides a means of operating a MongoDB
  system.
*/

/*!
  Constructs a TMongoQuery object using the collection \a collection.
*/
TMongoQuery::TMongoQuery(const QString &collection)
    : database(TActionContext::current()->getKvsDatabase(TKvsDatabase::MongoDB)),
      nameSpace(), queryLimit(0), queryOffset(0)
{
    nameSpace = database.databaseName() + '.' + collection.trimmed();
}

/*!
  Copy constructor.
*/
TMongoQuery::TMongoQuery(const TMongoQuery &other)
    : database(other.database), nameSpace(other.nameSpace),
      queryLimit(other.queryLimit), queryOffset(other.queryOffset)
{ }

/*!
  Assignment operator.
*/
TMongoQuery &TMongoQuery::operator=(const TMongoQuery &other)
{
    database = other.database;
    nameSpace = other.nameSpace;
    queryLimit = other.queryLimit;
    queryOffset = other.queryOffset;
    return *this;
}

/*!
  Finds documents by the criteria \a criteria in the collection.
  Use the \a fields parameter to control the fields to return.
  \sa TMongoQuery::next()
*/
bool TMongoQuery::find(const QVariantMap &criteria, const QStringList &fields)
{
    if (!database.isValid()) {
        tSystemError("TMongoQuery::find : driver not loaded");
        return false;
    }

    return driver()->find(nameSpace, criteria, fields, queryLimit, queryOffset, 0);
}

/*!
  Retrieves the next document in the result set, if available, and positions
  on the retrieved document. Returns true if the record is successfully
  retrieved; otherwise returns false.
*/
bool TMongoQuery::next()
{
    if (!database.isValid()) {
        return false;
    }

    return driver()->cursor().next();
}

/*!
  Returns the current document as a QVariantMap object.
*/
QVariantMap TMongoQuery::value() const
{
    if (!database.isValid()) {
        return QVariantMap();
    }

    return driver()->cursor().value();
}

/*!
  Finds documents by the criteria \a criteria in the collection
  and returns a retrieved document as a QVariantMap object.
  Use the \a fields parameter to control the fields to return.
*/
QVariantMap TMongoQuery::findOne(const QVariantMap &criteria, const QStringList &fields)
{
    if (!database.isValid()) {
        tSystemError("TMongoQuery::findOne : driver not loaded");
        return QVariantMap();
    }

    return driver()->findOne(nameSpace, criteria, fields);
}

/*!
  Inserts the document \a document into the collection.
*/
bool TMongoQuery::insert(const QVariantMap &document)
{
    if (!database.isValid()) {
        tSystemError("TMongoQuery::insert : driver not loaded");
        return false;
    }

    return driver()->insert(nameSpace, document);
}

/*!
  Removes documents that matches the \a criteria from the collection.
*/
bool TMongoQuery::remove(const QVariantMap &criteria)
{
    if (!database.isValid()) {
        tSystemError("TMongoQuery::remove : driver not loaded");
        return false;
    }

    return driver()->remove(nameSpace, criteria);
}

/*!
  Updates an existing document of the selection criteria \a criteria in
  the collection with new document \a document.
  When the \a upsert is true, inserts the document in the collection
  if no document matches the \a criteria.
*/
bool TMongoQuery::update(const QVariantMap &criteria, const QVariantMap &document, bool upsert)
{
    if (!database.isValid()) {
        tSystemError("TMongoQuery::update : driver not loaded");
        return false;
    }

    return driver()->update(nameSpace, criteria, document, upsert);
}

/*!
  Updates existing documents of the selection criteria \a criteria in
  the collection with new document \a document.
  When the \a upsert is true, inserts the document in the collection
  if no document matches the \a criteria.
*/
bool TMongoQuery::updateMulti(const QVariantMap &criteria, const QVariantMap &document, bool upsert)
{
    if (!database.isValid()) {
        tSystemError("TMongoQuery::updateMulti : driver not loaded");
        return false;
    }

    return driver()->updateMulti(nameSpace, criteria, document, upsert);
}

/*!
  Returns the MongoDB driver associated with the TMongoQuery object.
*/
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

/*!
  Returns the MongoDB driver associated with the TMongoQuery object.
*/
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


/*!
  \fn void TMongoQuery::setLimit(int limit)
  Sets the limit to limit, which is the limited number of documents
  for finding documents.
  \sa TMongoQuery::find()
*/

/*!
  \fn void TMongoQuery::setOffset(int offset)
  Sets the offset to offset, which is the number of documents to skip
  for finding documents.
  \sa TMongoQuery::find()
*/
