/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tdatabasecontext.h"
#include "tkvsdatabasepool.h"
#include "tsqldatabasepool.h"
#include "tsystemglobal.h"
#include <QSqlDatabase>
#include <QThreadStorage>
#include <QtCore>
#include <TKvsDriver>
#include <TWebApplication>
#include <ctime>

namespace {
// Stores a pointer to current database context into TLS
//  - qulonglong type to prevent qThreadStorage_deleteData() function to work
QThreadStorage<qulonglong> databaseContextPtrTls;
QSqlDatabase invalidDb;
}

/*!
  \class TDatabaseContext
  \brief The TDatabaseContext class is the base class of contexts for
  database access.
*/

TDatabaseContext::TDatabaseContext() :
    sqlDatabases(),
    kvsDatabases()
{
}


TDatabaseContext::~TDatabaseContext()
{
    release();
}


QSqlDatabase &TDatabaseContext::getSqlDatabase(int id)
{
    if (id < 0) {
        return invalidDb;  // invalid database
    }

    if (id >= Tf::app()->sqlDatabaseSettingsCount()) {
        throw RuntimeException("error database id", __FILE__, __LINE__);
    }

    TSqlTransaction &tx = sqlDatabases[id];
    QSqlDatabase &db = tx.database();

    if (db.isValid() && tx.isActive()) {
        return db;
    }

    int n = 0;
    do {
        if (!db.isValid()) {
            db = TSqlDatabasePool::instance()->database(id);
        }

        if (tx.begin()) {
            break;
        }
        TSqlDatabasePool::instance()->pool(db, true);
    } while (++n < 2);  // try two times

    idleElapsed = (uint)std::time(nullptr);
    return db;
}


void TDatabaseContext::releaseSqlDatabases()
{
    rollbackTransactions();
    sqlDatabases.clear();
}


TKvsDatabase &TDatabaseContext::getKvsDatabase(Tf::KvsEngine engine)
{
    TKvsDatabase &db = kvsDatabases[(int)engine];
    if (!db.isValid()) {
        db = TKvsDatabasePool::instance()->database(engine);
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


void TDatabaseContext::setTransactionEnabled(bool enable, int id)
{
    if (id < 0) {
        tError("Invalid database ID: %d", id);
        return;
    }
    return sqlDatabases[id].setEnabled(enable);
}


void TDatabaseContext::commitTransactions()
{
    for (QMap<int, TSqlTransaction>::iterator it = sqlDatabases.begin(); it != sqlDatabases.end(); ++it) {
        TSqlTransaction &tx = it.value();
        tx.commit();
        TSqlDatabasePool::instance()->pool(tx.database());
    }
}


bool TDatabaseContext::commitTransaction(int id)
{
    bool res = false;

    if (id < 0 || id >= sqlDatabases.count()) {
        tError("Failed to commit transaction. Invalid database ID: %d", id);
        return res;
    }

    TSqlTransaction &tx = sqlDatabases[id];
    res = tx.commit();
    TSqlDatabasePool::instance()->pool(sqlDatabases[id].database());
    return res;
}


void TDatabaseContext::rollbackTransactions()
{
    for (QMap<int, TSqlTransaction>::iterator it = sqlDatabases.begin(); it != sqlDatabases.end(); ++it) {
        TSqlTransaction &tx = it.value();
        tx.rollback();
        TSqlDatabasePool::instance()->pool(tx.database(), true);
    }
}


bool TDatabaseContext::rollbackTransaction(int id)
{
    bool res = false;

    if (id < 0 || id >= sqlDatabases.count()) {
        tError("Failed to rollback transaction. Invalid database ID: %d", id);
        return res;
    }
    res = sqlDatabases[id].rollback();
    TSqlDatabasePool::instance()->pool(sqlDatabases[id].database(), true);
    return res;
}


int TDatabaseContext::idleTime() const
{
    return (idleElapsed > 0) ? (uint)std::time(nullptr) - idleElapsed : -1;
}


TDatabaseContext *TDatabaseContext::currentDatabaseContext()
{
    return reinterpret_cast<TDatabaseContext *>(databaseContextPtrTls.localData());
}


void TDatabaseContext::setCurrentDatabaseContext(TDatabaseContext *context)
{
    if (context && databaseContextPtrTls.localData()) {
        tSystemWarn("Duplicate set : setCurrentDatabaseContext()");
    }
    databaseContextPtrTls.setLocalData((qulonglong)context);
}
