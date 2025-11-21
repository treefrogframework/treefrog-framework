/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsqldatabasepool.h"
#include <QSqlDriver>
#include <TSqlTransaction>
#include <TSystemGlobal>
#include <TWebApplication>

/*!
  \class TSqlTransaction
  \brief The TSqlTransaction class provides a transaction of database.
*/


TSqlTransaction::TSqlTransaction()
{
}


TSqlTransaction::~TSqlTransaction()
{
    rollback();
}


bool TSqlTransaction::begin()
{
    if (!_database->isValid()) {
        tSystemError("Can not begin transaction. Invalid database: {}", _database->connectionName());
        return false;
    }

    if (!_enabled) {
        return true;
    }

    if (!_database->sqlDatabase().driver()->hasFeature(QSqlDriver::Transactions)) {
        return true;
    }

    if (_active) {
        tSystemDebug("Has begun transaction already. database:{}", _database->connectionName());
        return true;
    }

    QElapsedTimer time;
    time.start();

    _active = _database->sqlDatabase().transaction();
    _connectionName = _database->sqlDatabase().connectionName();
    int id = TSqlDatabasePool::getDatabaseId(_database->sqlDatabase());
    if (_active) {
        Tf::traceQueryLog(time.elapsed(), "[BEGIN] [databaseId:{}] {}", id, _connectionName);
    } else {
        Tf::traceQueryLog(time.elapsed(), "[BEGIN Failed] [databaseId:{}] {}", id, _connectionName);
    }
    return _active;
}


bool TSqlTransaction::commit()
{
    bool res = true;

    if (!_enabled) {
        return res;
    }

    if (_active) {
        if (!_database->sqlDatabase().isValid()) {
            tSystemWarn("Database is invalid. [{}]  [{}:{}]", _connectionName, __FILE__, __LINE__);
        } else {
            QElapsedTimer time;
            time.start();

            res = _database->sqlDatabase().commit();
            int id = TSqlDatabasePool::getDatabaseId(_database->sqlDatabase());

            if (Q_LIKELY(res)) {
                Tf::traceQueryLog(time.elapsed(), "[COMMIT] [databaseId:{}] {}", id, qUtf8Printable(_database->connectionName()));
            } else {
                Tf::traceQueryLog(time.elapsed(), "[COMMIT Failed] [databaseId:{}] {}", id, qUtf8Printable(_database->connectionName()));
            }
        }
    }

    _active = false;
    return res;
}


bool TSqlTransaction::rollback()
{
    bool res = true;

    if (!_enabled) {
        return res;
    }

    if (_active) {
        if (!_database->isValid()) {
            tSystemWarn("Database is invalid. [{}]  [{}:{}]", _connectionName, __FILE__, __LINE__);
        } else {
            QElapsedTimer time;
            time.start();

            res = _database->sqlDatabase().rollback();
            int id = TSqlDatabasePool::getDatabaseId(_database->sqlDatabase());

            if (Q_LIKELY(res)) {
                Tf::traceQueryLog(time.elapsed(), "[ROLLBACK] [databaseId:{}] {}", id, qUtf8Printable(_database->connectionName()));
            } else {
                Tf::traceQueryLog(time.elapsed(), "[ROLLBACK Failed] [databaseId:{}] {}", id, qUtf8Printable(_database->connectionName()));
            }
        }
    }

    _active = false;
    return res;
}
