/* Copyright (c) 2011-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TSqlTransaction>
#include <TWebApplication>
#include <TSystemGlobal>
#include <QSqlDriver>
#include "tsqldatabasepool.h"

/*!
  \class TSqlTransaction
  \brief The TSqlTransaction class provides a transaction of database.
*/


TSqlTransaction::TSqlTransaction()
{ }


TSqlTransaction::TSqlTransaction(const TSqlTransaction &other) :
    _enabled(other._enabled),
    _database(other._database),
    _active(other._active)
{ }


TSqlTransaction::~TSqlTransaction()
{
    rollback();
}


TSqlTransaction &TSqlTransaction::operator=(const TSqlTransaction &other)
{
    _enabled = other._enabled;
    _database = other._database;
    _active = other._active;
    return *this;
}


bool TSqlTransaction::begin()
{
    if (Q_UNLIKELY(!_database.isValid())) {
        tSystemError("Can not begin transaction. Invalid database: %s", qPrintable(_database.connectionName()));
        return false;
    }

    if (!_enabled) {
        return true;
    }

    if (! _database.driver()->hasFeature(QSqlDriver::Transactions)) {
        return true;
    }

    if (_active) {
        tSystemDebug("Has begun transaction already. database:%s", qPrintable(_database.connectionName()));
        return true;
    }

    _active = _database.transaction();
    int id = TSqlDatabasePool::getDatabaseId(_database);
    if (Q_LIKELY(_active)) {
        Tf::traceQueryLog("[BEGIN] [databaseId:%d] %s", id, qPrintable(_database.connectionName()));
    } else {
        Tf::traceQueryLog("[BEGIN Failed] [databaseId:%d] %s", id, qPrintable(_database.connectionName()));
    }
    return _active;
}


bool TSqlTransaction::commit()
{
    bool res = true;

    if (!_enabled) {
        return res;
    }

    if (_active && _database.isValid()) {
        res = _database.commit();

        int id = TSqlDatabasePool::getDatabaseId(_database);
        if (Q_LIKELY(res)) {
            Tf::traceQueryLog("[COMMIT] [databaseId:%d]", id);
        } else {
            Tf::traceQueryLog("[COMMIT Failed] [databaseId:%d]", id);
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

    if (_active && _database.isValid()) {
        res = _database.rollback();

        int id = TSqlDatabasePool::getDatabaseId(_database);
        if (Q_LIKELY(res)) {
            Tf::traceQueryLog("[ROLLBACK] [databaseId:%d]", id);
        } else {
            Tf::traceQueryLog("[ROLLBACK Failed] [databaseId:%d]", id);
        }
    }

    _active = false;
    return res;
}
