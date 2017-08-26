/* Copyright (c) 2012-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TMongoQuery>
#include <TMongoDriver>
#include <TMongoCursor>
#include <TBson>
#include <TActionContext>
#include <TSystemGlobal>

const QLatin1String ObjectIdKey("_id");

/*!
  \class TMongoQuery
  \brief The TMongoQuery class provides a means of operating a MongoDB
  system.
*/

/*!
  Constructs a TMongoQuery object using the collection \a collection.
*/
TMongoQuery::TMongoQuery(const QString &collection)
    : database(Tf::currentDatabaseContext()->getKvsDatabase(TKvsDatabase::MongoDB)),
      collection(collection.trimmed()), queryLimit(0), queryOffset(0)
{ }

/*!
  Copy constructor.
*/
TMongoQuery::TMongoQuery(const TMongoQuery &other)
    : database(other.database), collection(other.collection),
      queryLimit(other.queryLimit), queryOffset(other.queryOffset)
{ }

/*!
  Assignment operator.
*/
TMongoQuery &TMongoQuery::operator=(const TMongoQuery &other)
{
    database = other.database;
    collection = other.collection;
    queryLimit = other.queryLimit;
    queryOffset = other.queryOffset;
    return *this;
}

/*!
  Finds documents by the criteria \a criteria in the collection and
  returns the number of the documents. Use the \a fields parameter to
  control the fields to return.
  \sa TMongoQuery::next()
*/
bool TMongoQuery::find(const QVariantMap &criteria, const QVariantMap &orderBy, const QStringList &fields)
{
    if (!database.isValid()) {
        tSystemError("TMongoQuery::find : driver not loaded");
        return false;
    }
    return driver()->find(collection, criteria, orderBy, fields, queryLimit, queryOffset, 0);
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
    return driver()->findOne(collection, criteria, fields);
}


QVariantMap TMongoQuery::findById(const QString &id, const QStringList &fields)
{
    QVariantMap criteria;

    if (id.isEmpty()) {
        tSystemError("TMongoQuery::findById : ObjectId not found");
        return QVariantMap();
    }

    criteria[ObjectIdKey] = id;
    return findOne(criteria, fields);
}

/*!
  Inserts the document \a document into the collection.
*/
bool TMongoQuery::insert(QVariantMap &document)
{
    if (!database.isValid()) {
        tSystemError("TMongoQuery::insert : driver not loaded");
        return false;
    }

    if (!document.contains(ObjectIdKey)) {
        // Sets Object ID
        document.insert(ObjectIdKey, TBson::generateObjectId());
    }

    bool ret = driver()->insert(collection, document);
    return ret;
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

    bool ret = driver()->remove(collection, criteria);
    return ret;
}

/*!
  Removes an existing document that matches the ObjectID of the
  \a document from the collection.
*/
bool TMongoQuery::removeById(const QVariantMap &document)
{
    QString id = document[ObjectIdKey].toString();
    if (id.isEmpty()) {
        tSystemError("TMongoQuery::removeById : ObjectId not found");
        return false;
    }

    QVariantMap criteria;
    criteria[ObjectIdKey] = id;
    return remove(criteria);
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

    bool ret = driver()->update(collection, criteria, document, upsert);
    return ret;
}

/*!
  Updates existing documents of the selection criteria \a criteria in
  the collection with new document \a document.
*/
bool TMongoQuery::updateMulti(const QVariantMap &criteria, const QVariantMap &document)
{
    QVariantMap doc;

    if (!database.isValid()) {
        tSystemError("TMongoQuery::updateMulti : driver not loaded");
        return false;
    }

    if (!document.contains(QLatin1String("$set"))) {
        doc.insert("$set", document);
    } else {
        doc = document;
    }

    bool ret = driver()->updateMulti(collection, criteria, doc);
    return ret;
}

/*!
  Updates an existing document that matches the ObjectID with the
  \a document.
*/
bool TMongoQuery::updateById(const QVariantMap &document)
{
    QString id = document[ObjectIdKey].toString();
    if (id.isEmpty()) {
        tSystemError("TMongoQuery::updateById : ObjectId not found");
        return false;
    }

    QVariantMap criteria;
    criteria[ObjectIdKey] = id;
    return update(criteria, document, false);
}


int TMongoQuery::count(const QVariantMap &criteria)
{
    if (!database.isValid()) {
        tSystemError("TMongoQuery::count : driver not loaded");
        return -1;
    }
    return driver()->count(collection, criteria);
}

/*!
  Returns the number of documents affected by the result's Mongo statement,
  or -1 if it cannot be determined.
*/
int TMongoQuery::numDocsAffected() const
{
    int num = 0;
    QVariantMap status = driver()->getLastCommandStatus();
    num += status.value(QLatin1String("nInserted"), 0).toInt();
    num += status.value(QLatin1String("nMatched"), 0).toInt();
    num += status.value(QLatin1String("nUpserted"), 0).toInt();
    num += status.value(QLatin1String("nRemoved"), 0).toInt();
    return num;
}


/*!
  Returns the string of the most recent error with the current connection.
*/
QString TMongoQuery::lastErrorString() const
{
    return driver()->lastErrorString();
}


/*!
  Returns the MongoDB driver associated with the TMongoQuery object.
*/
TMongoDriver *TMongoQuery::driver()
{
#ifdef TF_NO_DEBUG
    return (TMongoDriver *)database.driver();
#else
    if (!database.driver()) {
        return nullptr;
    }

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
    if (!database.driver()) {
        return nullptr;
    }

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


/*!
  \fn void TMongoQuery::lastError() const
  Returns the VariantMap object of the error status of the last operation.
*/
