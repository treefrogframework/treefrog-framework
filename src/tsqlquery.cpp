/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsystemglobal.h"
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <TAppSettings>
#include <TSqlQuery>
#include <TWebApplication>

namespace {
QMap<QString, QString> queryCache;
QMutex cacheMutex;
}

/*!
  \class TSqlQuery
  \brief The TSqlQuery class provides a means of executing and manipulating
         SQL statements.
*/


/*!
  Constructs a TSqlQuery object using the database \a databaseId.
*/
TSqlQuery::TSqlQuery(int databaseId) :
    QSqlQuery(QString(), Tf::currentSqlDatabase(databaseId))
{
}


TSqlQuery::TSqlQuery(const QSqlDatabase &db) :
    QSqlQuery(db)
{
}


/*!
  Loads a query from the given file \a filename.
*/
bool TSqlQuery::load(const QString &filename)
{
    QMutexLocker locker(&cacheMutex);

    QString query = queryCache.value(filename);
    if (!query.isEmpty()) {
        return QSqlQuery::prepare(query);
    }

    QDir dir(queryDirPath());
    QFile file(dir.filePath(filename));
    tSystemDebug("SQL_QUERY_ROOT: %s", qUtf8Printable(dir.dirName()));
    tSystemDebug("filename: %s", qUtf8Printable(file.fileName()));

    if (!file.open(QIODevice::ReadOnly)) {
        tSystemError("Unable to open file: %s", qUtf8Printable(file.fileName()));
        return false;
    }

    query = QObject::tr(file.readAll().constData());
    bool res = QSqlQuery::prepare(query);
    if (res) {
        // Caches the query-string
        queryCache.insert(filename, query);
    }
    return res;
}

/*!
  Returns the directory path for SQL query files, which is indicated by
  the value for application setting \a SqlQueriesStoredDirectory.
*/
QString TSqlQuery::queryDirPath() const
{
    const QString dir = Tf::app()->webRootPath() + Tf::appSettings()->value(Tf::SqlQueriesStoredDirectory).toString();
    return dir;
}

/*!
  Clears currently cached SQL queries that are loaded by the load() function.
*/
void TSqlQuery::clearCachedQueries()
{
    QMutexLocker locker(&cacheMutex);
    queryCache.clear();
}

/*!
  Returns the \a identifier escaped according to the rules of the database
  \a databaseId. The \a identifier can either be a table name or field name,
  dependent on \a type.
*/
QString TSqlQuery::escapeIdentifier(const QString &identifier, QSqlDriver::IdentifierType type, int databaseId)
{
    return escapeIdentifier(identifier, type, Tf::currentSqlDatabase(databaseId).driver());
}

/*!
  Returns the \a identifier escaped according to the rules of the
  driver \a driver. The \a identifier can either be a table name
  or field name, dependent on \a type.
*/
QString TSqlQuery::escapeIdentifier(const QString &identifier, QSqlDriver::IdentifierType type, const QSqlDriver *driver)
{
    QString ret = identifier;
    if (driver && !driver->isIdentifierEscaped(identifier, type)) {
        ret = driver->escapeIdentifier(identifier, type);
    }
    return ret;
}

/*!
  Returns a string representation of the value \a val for the database
  \a databaseId.
*/
#if QT_VERSION < 0x060000
QString TSqlQuery::formatValue(const QVariant &val, QVariant::Type type, int databaseId)
#else
QString TSqlQuery::formatValue(const QVariant &val, const QMetaType &type, int databaseId)
#endif
{
    return formatValue(val, type, Tf::currentSqlDatabase(databaseId));
}

/*!
  Returns a string representation of the value \a val for the database
  \a database.
*/
#if QT_VERSION < 0x060000
QString TSqlQuery::formatValue(const QVariant &val, QVariant::Type type, const QSqlDatabase &database)
{
    if (Q_UNLIKELY(type == QVariant::Invalid)) {
        type = val.type();
    }

    QSqlField field(QStringLiteral("dummy"), type);
    field.setValue(val);
    return database.driver()->formatValue(field);
}
#else
QString TSqlQuery::formatValue(const QVariant &val, const QMetaType &type, const QSqlDatabase &database)
{
    QMetaType metaType = type;
    if (Q_UNLIKELY(!metaType.isValid())) {
        metaType = val.metaType();
    }

    QSqlField field(QStringLiteral("dummy"), metaType);
    field.setValue(val);
    return database.driver()->formatValue(field);
}
#endif

/*!
  Returns a string representation of the value \a val for the database
  \a database.
*/
QString TSqlQuery::formatValue(const QVariant &val, const QSqlDatabase &database)
{
#if QT_VERSION < 0x060000
    return formatValue(val, val.type(), database);
#else
    return formatValue(val, val.metaType(), database);
#endif
}

/*!
  Prepares the SQL query \a query for execution.
*/
TSqlQuery &TSqlQuery::prepare(const QString &query)
{
    bool ret = QSqlQuery::prepare(query);
    if (!ret) {
        Tf::writeQueryLog(QLatin1String("(Query prepare) ") + query, ret, lastError());
    }
    return *this;
}

/*!
  Executes the SQL in \a query. Returns true and sets the query state to
  active if the query was successful; otherwise returns false.
*/
bool TSqlQuery::exec(const QString &query)
{
    bool ret = QSqlQuery::exec(query);
    Tf::writeQueryLog(query, ret, lastError());
    return ret;
}

/*!
  Executes a previously prepared SQL query. Returns true if the query
  executed successfully; otherwise returns false.
*/
bool TSqlQuery::exec()
{
    bool ret = QSqlQuery::exec();
    Tf::writeQueryLog(executedQuery(), ret, lastError());
    return ret;
}

/*!
  Set the placeholder \a placeholder to be bound to value \a val in the
  prepared statement.
  \fn TSqlQuery &TSqlQuery::bind(const QString &placeholder, const QVariant &val)
*/

/*!
  Set the placeholder in position \a pos to be bound to value \a val in
  the prepared statement. Field numbering starts at 0.
  \fn TSqlQuery &TSqlQuery::bind(int pos, const QVariant &val)
*/

/*!
  Adds the value \a val to the list of values when using positional value
  binding and returns the query object. The order of the addBind() calls
  determines which placeholder a value will be bound to in the prepared
  query.
  \fn TSqlQuery &TSqlQuery::addBind(const QVariant &val)
*/

/*!
  Returns the value of first field in the next object and advances the
  internal iterator by one position. It can be used for a query returning
  at least one result, such as 'SELECT COUNT(*)'.
  \fn QVariant TSqlQuery::getNextValue()
*/

/*!
  Returns the number of rows affected by the result's SQL statement, or -1
  if it cannot be determined. Note that for SELECT statements, the value is
  undefined; use size() instead. If the query is not active, -1 is returned.
  \fn int TSqlQuery::numRowsAffected() const
 */

/*!
  Returns the size of the result (number of rows returned), or -1 if the size
  cannot be determined or if the database does not support reporting information
  about query sizes. Note that for non-SELECT statements (isSelect() returns
  false), size() will return -1. If the query is not active (isActive() returns
  false), -1 is returned.
  To determine the number of rows affected by a non-SELECT statement, use
  numRowsAffected().
  \fn int TSqlQuery::size() const
 */

/*!
  Retrieves the next record in the result, if available, and positions the
  query on the retrieved record. Note that the result must be in the active
  state and isSelect() must return true before calling this function or it
  will do nothing and return false.
  \fn bool TSqlQuery::next()
 */

/*!
  Returns the value of field index in the current record.
  \fn QVariant TSqlQuery::value(int index) const
 */

/*!
  Returns the value of the field called name in the current record.
  If field name does not exist an invalid variant is returned.
  \fn QVariant TSqlQuery::value(const QString &name) const
 */
