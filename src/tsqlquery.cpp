/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <TSqlQuery>
#include <TWebApplication>
#include <TActionContext>
#include "tsystemglobal.h"

static QHash<QString, QString> queryCache;
static QMutex cacheMutex;

/*!
  \class TSqlQuery
  \brief The TSqlQuery class provides a means of executing and manipulating
         SQL statements.
*/

/*!
  Constructor.
 */
TSqlQuery::TSqlQuery(const QString &query, int databaseId)
    : QSqlQuery(query, TActionContext::current()->getDatabase(databaseId))
{ }

/*!
  Constructor.
 */
TSqlQuery::TSqlQuery(int databaseId)
    : QSqlQuery(QString(), TActionContext::current()->getDatabase(databaseId))
{ }


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


QString TSqlQuery::queryDirPath() const
{
    QString dir = Tf::app()->webRootPath() + QDir::separator() + Tf::app()->appSettings().value("SqlQueriesStoredDirectory").toString();
    
    dir.replace(QChar('/'), QDir::separator());
    return dir;
}


void TSqlQuery::clearCachedQueries()
{
    QMutexLocker locker(&cacheMutex);
    queryCache.clear();
}


QString TSqlQuery::escapeIdentifier(const QString &identifier, QSqlDriver::IdentifierType type, int databaseId)
{
    return escapeIdentifier(identifier, type, TActionContext::current()->getDatabase(databaseId));
}


QString TSqlQuery::escapeIdentifier(const QString &identifier, QSqlDriver::IdentifierType type, const QSqlDatabase &database)
{
    QString ret = identifier;
    QSqlDriver *driver = database.driver();
    if (!driver->isIdentifierEscaped(identifier, type)) {
        ret = driver->escapeIdentifier(identifier, type);
    }
    return ret;
}


QString TSqlQuery::formatValue(const QVariant &val, int databaseId)
{
    return formatValue(val, TActionContext::current()->getDatabase(databaseId));
}


QString TSqlQuery::formatValue(const QVariant &val, const QSqlDatabase &database)
{
    QSqlField field("dummy", val.type());
    field.setValue(val);
    return database.driver()->formatValue(field);
}


bool TSqlQuery::exec(const QString &query)
{
    bool ret = QSqlQuery::exec(query);
    QString q = (ret) ? query : QLatin1String("(Query failed) ") + query;
    tQueryLog("%s", qPrintable(q));
    return ret;
}


bool TSqlQuery::exec()
{
    bool ret = QSqlQuery::exec();
    QString q = executedQuery();
    QString str = (ret) ? q : (QLatin1String("(Query failed) ") + (q.isEmpty() ? lastQuery() : q));
    tQueryLog("%s", qPrintable(str));
    return ret;   
}
