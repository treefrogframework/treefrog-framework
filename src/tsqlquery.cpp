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

    QSqlField field(QStringLiteral("dummy"), type);
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
