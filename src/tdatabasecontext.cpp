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
#include <TKvsDriver>
#include <TCache>
#include <TSystemGlobal>
#include <TWebApplication>
#include <QSqlDatabase>
#include <QtCore>
#include <ctime>
#include <thread>

namespace {
// Stores a pointer to current database context into TLS
thread_local TDatabaseContext *databaseContextPtrTls = nullptr;

}

/*!
  \class TDatabaseContext
  \brief The TDatabaseContext class is the base class of contexts for
  database access.
*/

TDatabaseContext::TDatabaseContext()
{
    const int count = Tf::app()->sqlDatabaseSettingsCount();
    if (sqlTransactions.size() < (size_t)count) {
        sqlTransactions.resize(count);
    }

    if (kvsDatabases.size() < (size_t)Tf::KvsEngine::Num) {
        kvsDatabases.resize((size_t)Tf::KvsEngine::Num);
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
        throw RuntimeException("error database id", __FILE__, __LINE__);
    }

    if (id >= Tf::app()->sqlDatabaseSettingsCount()) {
        throw RuntimeException("error database id", __FILE__, __LINE__);
    }

    TSqlTransaction &tx = sqlTransactions[id];
    TSqlDatabase::Handle &handle = tx.database();

    if (handle && handle->isValid()) {
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

        handle = std::move(TSqlDatabase::Handle{});
    } while (++n < 2);  // try two times

    idleElapsed = (uint)std::time(nullptr);
    return *handle;
}


void TDatabaseContext::releaseSqlDatabases()
{
    rollbackTransactions();

    for (auto &tx : sqlTransactions) {
        if (tx.database()) {
            tx.database() = TSqlDatabase::Handle{};
        }
    }
}


TKvsDatabase::Handle &TDatabaseContext::getKvsDatabase(Tf::KvsEngine engine)
{
    auto &handle = kvsDatabases[(int)engine];
    if (!handle || !handle->isValid()) {
        handle = std::move(TKvsDatabasePool::instance()->database(engine));
    }

    idleElapsed = (uint)std::time(nullptr);
    return handle;
}


void TDatabaseContext::releaseKvsDatabases()
{
    for (auto &handle : kvsDatabases) {
        if (handle) {
            handle = TKvsDatabase::Handle{};
        }
    }
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
    return sqlTransactions[id].setEnabled(enable);
}


void TDatabaseContext::commitTransactions()
{
    for (auto it = sqlTransactions.begin(); it != sqlTransactions.end(); ++it) {
        it->commit();
    }
}


bool TDatabaseContext::commitTransaction(int id)
{
    bool res = false;

    if (id < 0 || id >= (int)sqlTransactions.size()) {
        Tf::error("Failed to commit transaction. Invalid database ID: {}", id);
        return res;
    }

    TSqlTransaction &tx = sqlTransactions[id];
    res = tx.commit();
    return res;
}


void TDatabaseContext::rollbackTransactions()
{
    for (auto it = sqlTransactions.begin(); it != sqlTransactions.end(); ++it) {
        it->rollback();
    }
}


bool TDatabaseContext::rollbackTransaction(int id)
{
    bool res = false;

    if (id < 0 || id >= (int)sqlTransactions.size()) {
        Tf::error("Failed to rollback transaction. Invalid database ID: {}", id);
        return res;
    }
    res = sqlTransactions[id].rollback();
    return res;
}


int TDatabaseContext::idleTime() const
{
    return (idleElapsed > 0) ? (uint)std::time(nullptr) - idleElapsed : -1;
}


TDatabaseContext *TDatabaseContext::currentDatabaseContext()
{
    return databaseContextPtrTls;
}


void TDatabaseContext::setCurrentDatabaseContext(TDatabaseContext *context)
{
    if (context && databaseContextPtrTls) {
        tSystemWarn("Duplicate set : setCurrentDatabaseContext()");
    }
    databaseContextPtrTls = context;
}


TCache *TDatabaseContext::cache()
{
    if (!cachep) {
        cachep = new TCache;
    }
    return cachep;
}
