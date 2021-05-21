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
    if (Q_UNLIKELY(!_database.isValid())) {
        tSystemError("Can not begin transaction. Invalid database: %s", qUtf8Printable(_database.connectionName()));
        return false;
    }

    if (!_enabled) {
        return true;
    }

    if (!_database.driver()->hasFeature(QSqlDriver::Transactions)) {
        return true;
    }

    if (_active) {
        tSystemDebug("Has begun transaction already. database:%s", qUtf8Printable(_database.connectionName()));
        return true;
    }

    _active = _database.transaction();
    _connectionName = _database.connectionName();
    int id = TSqlDatabasePool::getDatabaseId(_database);
    if (Q_LIKELY(_active)) {
        Tf::traceQueryLog("[BEGIN] [databaseId:%d] %s", id, qUtf8Printable(_connectionName));
    } else {
        Tf::traceQueryLog("[BEGIN Failed] [databaseId:%d] %s", id, qUtf8Printable(_connectionName));
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
        if (!_database.isValid()) {
            tSystemWarn("Database is invalid. [%s]  [%s:%d]", qUtf8Printable(_connectionName), __FILE__, __LINE__);
        } else {
            res = _database.commit();

            int id = TSqlDatabasePool::getDatabaseId(_database);
            if (Q_LIKELY(res)) {
                Tf::traceQueryLog("[COMMIT] [databaseId:%d] %s", id, qUtf8Printable(_database.connectionName()));
            } else {
                Tf::traceQueryLog("[COMMIT Failed] [databaseId:%d] %s", id, qUtf8Printable(_database.connectionName()));
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
        if (!_database.isValid()) {
            tSystemWarn("Database is invalid. [%s]  [%s:%d]", qUtf8Printable(_connectionName), __FILE__, __LINE__);
        } else {
            res = _database.rollback();

            int id = TSqlDatabasePool::getDatabaseId(_database);
            if (Q_LIKELY(res)) {
                Tf::traceQueryLog("[ROLLBACK] [databaseId:%d] %s", id, qUtf8Printable(_database.connectionName()));
            } else {
                Tf::traceQueryLog("[ROLLBACK Failed] [databaseId:%d] %s", id, qUtf8Printable(_database.connectionName()));
            }
        }
    }

    _active = false;
    return res;
}
