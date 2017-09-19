/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <TSqlQuery>
#include <TWebApplication>
#include <TAppSettings>
#include "tsystemglobal.h"

static QMap<QString, QString> queryCache;
static QMutex cacheMutex;

/*!
  \class TSqlQuery
  \brief The TSqlQuery class provides a means of executing and manipulating
         SQL statements.
*/


/*!
  Constructs a TSqlQuery object using the database \a databaseId.
*/
TSqlQuery::TSqlQuery(int databaseId)
    : QSqlQuery(QString(), Tf::currentSqlDatabase(databaseId))
{ }


TSqlQuery::TSqlQuery(QSqlDatabase db)
    : QSqlQuery(db)
{ }


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
    tSystemDebug("SQL_QUERY_ROOT: %s", qPrintable(dir.dirName()));
    tSystemDebug("filename: %s", qPrintable(file.fileName()));
    if (!file.open(QIODevice::ReadOnly)) {
        tSystemError("Unable to open file: %s", qPrintable(file.fileName()));
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
    QString dir = Tf::app()->webRootPath() + Tf::appSettings()->value(Tf::SqlQueriesStoredDirectory).toString();
    dir.replace(QChar('/'), QDir::separator());
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
QString TSqlQuery::formatValue(const QVariant &val, QVariant::Type type, int databaseId)
{
    return formatValue(val, type, Tf::currentSqlDatabase(databaseId));
}

/*!
  Returns a string representation of the value \a val for the database
  \a database.
*/
QString TSqlQuery::formatValue(const QVariant &val, QVariant::Type type, const QSqlDatabase &database)
{
    if (Q_UNLIKELY(type == QVariant::Invalid)) {
        type = val.type();
    }

    QSqlField field("dummy", type);
    field.setValue(val);
    return database.driver()->formatValue(field);
}

/*!
  Returns a string representation of the value \a val for the database
  \a database.
*/
QString TSqlQuery::formatValue(const QVariant &val, const QSqlDatabase &database)
{
    return formatValue(val, val.type(), database);
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
  Returns the number of rows affected by the result's SQL statement, or -1
  if it cannot be determined. Note that for SELECT statements, the value is
  undefined; use size() instead. If the query is not active, -1 is returned.
 */
int TSqlQuery::numRowsAffected() const
{
    return QSqlQuery::numRowsAffected();
}

/*!
  Returns the size of the result (number of rows returned), or -1 if the size
  cannot be determined or if the database does not support reporting information
  about query sizes. Note that for non-SELECT statements (isSelect() returns
  false), size() will return -1. If the query is not active (isActive() returns
  false), -1 is returned.
  To determine the number of rows affected by a non-SELECT statement, use
  numRowsAffected().
 */
int TSqlQuery::size() const
{
    return QSqlQuery::size();
}

/*!
  Retrieves the next record in the result, if available, and positions the
  query on the retrieved record. Note that the result must be in the active
  state and isSelect() must return true before calling this function or it
  will do nothing and return false.
 */
bool TSqlQuery::next()
{
    return QSqlQuery::next();
}

/*!
  Returns the value of field index in the current record.
 */
QVariant TSqlQuery::value(int index) const
{
    return QSqlQuery::value(index);
}
