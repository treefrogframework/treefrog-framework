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
#include <TCache>
#include <TWebApplication>
#include <ctime>

namespace {
// Stores a pointer to current database context into TLS
//  - qulonglong type to prevent qThreadStorage_deleteData() function to work
QThreadStorage<qulonglong> databaseContextPtrTls;
QSqlDatabase invalidDb;

}

// std::map<int, TSqlTransaction> TDatabaseContext::sqlDatabases;
// std::map<int, TKvsDatabase> TDatabaseContext::kvsDatabases;

/*!
  \class TDatabaseContext
  \brief The TDatabaseContext class is the base class of contexts for
  database access.
*/

TDatabaseContext::TDatabaseContext()
//:
    // sqlDatabases(),
    // kvsDatabases()
{
    const int count = Tf::app()->sqlDatabaseSettingsCount();
    if (sqlDatabases.size() < (size_t)count) {
        sqlDatabases.resize(count);
    }

    if (kvsDatabases.size() < (size_t)count) {
        kvsDatabases.resize(count);
    }
}


TDatabaseContext::~TDatabaseContext()
{
    release();
    delete cachep;
}


TSqlDatabase &TDatabaseContext::getSqlDatabase(int id)
{
    if (id < 0) {
        throw RuntimeException("error database id", __FILE__, __LINE__);throw RuntimeException("error database id", __FILE__, __LINE__);
    }

    if (id >= Tf::app()->sqlDatabaseSettingsCount()) {
        throw RuntimeException("error database id", __FILE__, __LINE__);
    }

    TSqlTransaction &tx = sqlDatabases[id];
    TSqlDatabase::Handle &handle = tx.database();

    if (handle && handle->isValid() && tx.isActive()) {
        return *handle;
    }

    int n = 0;
    do {
        if (!handle || !handle->isValid()) {
            handle = std::move(TSqlDatabasePool::instance()->database(id));
        }

        if (tx.begin()) {
            break;
        }
    } while (++n < 2);  // try two times

    idleElapsed = (uint)std::time(nullptr);
    return *handle;
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
    for (auto it = kvsDatabases.begin(); it != kvsDatabases.end(); ++it) {
        // TKvsDatabasePool::instance()->pool(it->second);
        TKvsDatabasePool::instance()->pool(*it);
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
        Tf::error("Invalid database ID: {}", id);
        return;
    }
tSystemDebug("### setTransactionEnabled : id:{}", id);
    return sqlDatabases[id].setEnabled(enable);
}


void TDatabaseContext::commitTransactions()
{
    for (auto it = sqlDatabases.begin(); it != sqlDatabases.end(); ++it) {
        // TSqlTransaction &tx = it->second;
        // tx.commit();
        // TSqlDatabasePool::instance()->pool(tx.database());
        it->commit();
        //TSqlDatabasePool::instance()->pool(it->database());  もともとはpoolで戻していたが必要か？
    }
}


bool TDatabaseContext::commitTransaction(int id)
{
    bool res = false;

    if (id < 0 || id >= (int)sqlDatabases.size()) {
        Tf::error("Failed to commit transaction. Invalid database ID: {}", id);
        return res;
    }

    TSqlTransaction &tx = sqlDatabases[id];
    res = tx.commit();
    //TSqlDatabasePool::instance()->pool(sqlDatabases[id].database());
    return res;
}


void TDatabaseContext::rollbackTransactions()
{
    for (auto it = sqlDatabases.begin(); it != sqlDatabases.end(); ++it) {
        // TSqlTransaction &tx = it->second;
        // tx.rollback();
        // TSqlDatabasePool::instance()->pool(tx.database(), true);
        it->rollback();
        //TSqlDatabasePool::instance()->pool(it->database(), true);
    }
}


bool TDatabaseContext::rollbackTransaction(int id)
{
    bool res = false;

    if (id < 0 || id >= (int)sqlDatabases.size()) {
        Tf::error("Failed to rollback transaction. Invalid database ID: {}", id);
        return res;
    }
    res = sqlDatabases[id].rollback();
    //TSqlDatabasePool::instance()->pool(sqlDatabases[id].database(), true);
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


TCache *TDatabaseContext::cache()
{
    if (!cachep) {
        cachep = new TCache;
    }
    return cachep;
}
