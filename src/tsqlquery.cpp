/* Copyright (c) 2010-2013, AOYAMA Kazuharu
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
#include "tsystemglobal.h"

static QMap<QString, QString> queryCache;
static QMutex cacheMutex;

/*!
  \class TSqlQuery
  \brief The TSqlQuery class provides a means of executing and manipulating
         SQL statements.
*/

/*!
  Constructs a TSqlQuery object using the SQL \a query and the database
  \a databaseId. If \a query is not an empty string, it will be executed.
 */
TSqlQuery::TSqlQuery(const QString &query, int databaseId)
    : QSqlQuery(query, Tf::currentSqlDatabase(databaseId))
{
    if (!query.isEmpty()) {  // will be executed immediately
        QString q = (!lastError().isValid()) ? query : QLatin1String("(Query failed) ") + query;
        tQueryLog("%s", qPrintable(q));
    }
}

/*!
  Constructs a TSqlQuery object using the database \a databaseId.
*/
TSqlQuery::TSqlQuery(int databaseId)
    : QSqlQuery(QString(), Tf::currentSqlDatabase(databaseId))
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
    QString dir = Tf::app()->webRootPath() + QDir::separator() + Tf::app()->appSettings().value("SqlQueriesStoredDirectory").toString();

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
    return escapeIdentifier(identifier, type, Tf::currentSqlDatabase(databaseId));
}

/*!
  Returns the \a identifier escaped according to the rules of the
  database \a database. The \a identifier can either be a table name
  or field name, dependent on \a type.
*/
QString TSqlQuery::escapeIdentifier(const QString &identifier, QSqlDriver::IdentifierType type, const QSqlDatabase &database)
{
    QString ret = identifier;
    QSqlDriver *driver = database.driver();
    if (!driver->isIdentifierEscaped(identifier, type)) {
        ret = driver->escapeIdentifier(identifier, type);
    }
    return ret;
}

/*!
  Returns a string representation of the value \a val for the database
  \a databaseId.
*/
QString TSqlQuery::formatValue(const QVariant &val, int databaseId)
{
    return formatValue(val, Tf::currentSqlDatabase(databaseId));
}

/*!
  Returns a string representation of the value \a val for the database
  \a database.
*/
QString TSqlQuery::formatValue(const QVariant &val, const QSqlDatabase &database)
{
    QSqlField field("dummy", val.type());
    field.setValue(val);
    return database.driver()->formatValue(field);
}

/*!
  Executes the SQL in \a query. Returns true and sets the query state to
  active if the query was successful; otherwise returns false. Logs error texts from db if available.
*/
bool TSqlQuery::exec(const QString &query)
{
    bool ret = QSqlQuery::exec(query);
    QString qs = "";
    qs = this->lastError().databaseText() + query;
    if (qs.isEmpty())
        qs = ((ret) ? query : QLatin1String("(Query failed) ") + query);
    tQueryLog("%s", qPrintable(qs));
    return ret;
}

/*!
  Executes a previously prepared SQL query. Returns true if the query
  executed successfully; otherwise returns false. Logs error texts from db if available.
*/
bool TSqlQuery::exec()
{
    bool ret = QSqlQuery::exec();
    QString qs = executedQuery();
    QString str = "";
    str = this->lastError().databaseText() + lastQuery();
    if (qs.isEmpty())
        str = (ret) ? qs : (QLatin1String("(Query failed) ") + (qs.isEmpty() ? lastQuery() : qs));
    tQueryLog("%s", qPrintable(str));
    return ret;
}
