/* Copyright (c) 2017-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsqldatabase.h"
#include "tsqldriverextension.h"
#include "tsystemglobal.h"
#include <QFileInfo>
#include <QMap>
#include <QReadWriteLock>


class TDatabaseDict : public QMap<QString, TSqlDatabase> {
public:
    mutable QReadWriteLock lock;
};
Q_GLOBAL_STATIC(TDatabaseDict, dbDict)


TSqlDatabase::DbmsType TSqlDatabase::dbmsType() const
{
    return (_sqlDatabase.driver()) ? (TSqlDatabase::DbmsType)_sqlDatabase.driver()->dbmsType() : UnknownDbms;
}


void TSqlDatabase::setDriverExtension(TSqlDriverExtension *extension)
{
    _driverExtension = extension;
}


TSqlDatabase &TSqlDatabase::database(const QString &connectionName)
{
    static TSqlDatabase defaultDatabase;
    auto *dict = dbDict();
    QReadLocker locker(&dict->lock);

    if (dict->contains(connectionName)) {
        return (*dict)[connectionName];
    } else {
        return defaultDatabase;
    }
}


TSqlDatabase &TSqlDatabase::addDatabase(const QString &driver, const QString &connectionName)
{
    TSqlDatabase db(QSqlDatabase::addDatabase(driver, connectionName));
    auto *dict = dbDict();
    QWriteLocker locker(&dict->lock);

    if (dict->contains(connectionName)) {
        dict->take(connectionName);
    }

    dict->insert(connectionName, db);
    return (*dict)[connectionName];
}


void TSqlDatabase::removeDatabase(const QString &connectionName)
{
    auto *dict = dbDict();
    QWriteLocker locker(&dict->lock);
    dict->take(connectionName);
    QSqlDatabase::removeDatabase(connectionName);
}


bool TSqlDatabase::contains(const QString &connectionName)
{
    auto *dict = dbDict();
    QReadLocker locker(&dict->lock);
    return dict->contains(connectionName);
}


bool TSqlDatabase::isUpsertSupported() const
{
    return _driverExtension && _driverExtension->isUpsertSupported();
}


bool TSqlDatabase::isPreparedStatementSupported() const
{
    return _driverExtension && _driverExtension->isPreparedStatementSupported();
}
