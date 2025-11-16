/* Copyright (c) 2017-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsqldatabase.h"
#include "tsqldatabasepool.h"
#include "tsqldriverextension.h"
#include "tsystemglobal.h"
#include <QFileInfo>
#include <QSet>
#include <QReadWriteLock>


class TDatabaseDict : public QSet<QString> {
public:
    mutable QReadWriteLock lock;
};
Q_GLOBAL_STATIC(TDatabaseDict, dbDict)


TSqlDatabase::~TSqlDatabase()
{}


TSqlDatabase::TSqlDatabase(const QSqlDatabase &database) :
    _sqlDatabase(database)
{}


TSqlDatabase::TSqlDatabase(TSqlDatabase &&other)
{
    *this = std::move(other);
}


TSqlDatabase &TSqlDatabase::operator=(TSqlDatabase &&other)
{
    if (this != &other) {
        _sqlDatabase = other._sqlDatabase;
        _postOpenStatements = other._postOpenStatements;
        _enableUpsert = other._enableUpsert;
        _driverExtension = std::move(other._driverExtension);
    }
    return *this;
}


TSqlDatabase::DbmsType TSqlDatabase::dbmsType() const
{
    return (_sqlDatabase.driver()) ? (TSqlDatabase::DbmsType)_sqlDatabase.driver()->dbmsType() : UnknownDbms;
}


void TSqlDatabase::setDriverExtension(std::unique_ptr<TSqlDriverExtension> extension)
{
    _driverExtension = std::move(extension);
}


std::unique_ptr<TSqlDatabase> TSqlDatabase::database(const QString &connectionName)
{
    auto *dict = dbDict();
    QReadLocker locker(&dict->lock);

    if (dict->contains(connectionName)) {
        return std::unique_ptr<TSqlDatabase>{new TSqlDatabase{QSqlDatabase::database(connectionName)}};
    } else {
        return std::unique_ptr<TSqlDatabase>{new TSqlDatabase{TSqlDatabase{}}};
    }
}


bool TSqlDatabase::addDatabase(const QString &driver, const QString &connectionName)
{
    auto *dict = dbDict();
    QWriteLocker locker(&dict->lock);

    if (!dict->contains(connectionName)) {
        auto db = QSqlDatabase::addDatabase(driver, connectionName);
        if (db.isValid()) {
            dict->insert(connectionName);
        } else {
            tSystemError("TSqlDatabase::addDatabase error.  driver:{}  name:{}", driver, connectionName);
            return false;
        }
    }
    return true;
}


void TSqlDatabase::removeDatabase(const QString &connectionName)
{
    auto *dict = dbDict();
    QWriteLocker locker(&dict->lock);
    if (dict->remove(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
    }
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


TSqlDatabase::Handle::~Handle()
{
    tSystemDebug("Handle::~Handle");
    if (_conn) {
        TSqlDatabasePool::instance()->pool(std::move(_conn));
    }
}
