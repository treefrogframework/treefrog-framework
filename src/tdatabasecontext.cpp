/* Copyright (c) 2015-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QtCore>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <TWebApplication>
#include <TKvsDriver>
#include <ctime>
#include "tdatabasecontext.h"
#include "tsqldatabasepool.h"
#include "tkvsdatabasepool.h"
#include "tsystemglobal.h"

/*!
  \class TDatabaseContext
  \brief The TDatabaseContext class is the base class of contexts for
  database access.
*/

TDatabaseContext::TDatabaseContext()
    : sqlDatabases(),
      kvsDatabases(),
      transactions()
{ }


TDatabaseContext::~TDatabaseContext()
{
    release();
}


QSqlDatabase &TDatabaseContext::getSqlDatabase(int id)
{
    T_TRACEFUNC("id:%d", id);

    if (!Tf::app()->isSqlDatabaseAvailable()) {
        return sqlDatabases[0];  // invalid database
    }

    if (id < 0 || id >= Tf::app()->sqlDatabaseSettingsCount()) {
        throw RuntimeException("error database id", __FILE__, __LINE__);
    }

    QSqlDatabase &db = sqlDatabases[id];
    if (!db.isValid()) {
        db = TSqlDatabasePool::instance()->database(id);
        beginTransaction(db);
    }

    idleElapsed = (uint)std::time(nullptr);
    return db;
}


void TDatabaseContext::releaseSqlDatabases()
{
    rollbackTransactions();

    for (QMap<int, QSqlDatabase>::iterator it = sqlDatabases.begin(); it != sqlDatabases.end(); ++it) {
        TSqlDatabasePool::instance()->pool(it.value());
    }
    sqlDatabases.clear();
}


TKvsDatabase &TDatabaseContext::getKvsDatabase(TKvsDatabase::Type type)
{
    T_TRACEFUNC("type:%d", (int)type);

    TKvsDatabase &db = kvsDatabases[(int)type];
    if (!db.isValid()) {
        db = TKvsDatabasePool::instance()->database(type);
    }

    if (db.driver()) {
        db.driver()->moveToThread(QThread::currentThread());
    }

    idleElapsed = (uint)std::time(nullptr);
    return db;
}


void TDatabaseContext::releaseKvsDatabases()
{
    for (QMap<int, TKvsDatabase>::iterator it = kvsDatabases.begin(); it != kvsDatabases.end(); ++it) {
        TKvsDatabasePool::instance()->pool(it.value());
    }
    kvsDatabases.clear();
}


void TDatabaseContext::release()
{
    // Releases all SQL database sessions
    releaseSqlDatabases();

    // Releases all KVS database sessions
    releaseKvsDatabases();

    idleElapsed = 0;
}


void TDatabaseContext::setTransactionEnabled(bool enable)
{
    transactions.setEnabled(enable);
}


bool TDatabaseContext::beginTransaction(QSqlDatabase &database)
{
    bool ret = true;
    if (database.driver()->hasFeature(QSqlDriver::Transactions)) {
        ret = transactions.begin(database);
    }
    return ret;
}


void TDatabaseContext::commitTransactions()
{
    transactions.commitAll();
}


bool TDatabaseContext::commitTransaction(int id)
{
    return transactions.commit(id);
}


void TDatabaseContext::rollbackTransactions()
{
    transactions.rollbackAll();
}


bool TDatabaseContext::rollbackTransaction(int id)
{
    return transactions.rollback(id);
}


int TDatabaseContext::idleTime() const
{
    return (idleElapsed > 0) ? (uint)std::time(nullptr) - idleElapsed : -1;
}
