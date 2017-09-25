/* Copyright (c) 2011-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TSqlTransaction>
#include <TWebApplication>
#include <TSystemGlobal>
#include "tsqldatabasepool.h"

/*!
  \class TSqlTransaction
  \brief The TSqlTransaction class provides a transaction of database.
*/


TSqlTransaction::TSqlTransaction()
    : enabled(true), databases(Tf::app()->sqlDatabaseSettingsCount())
{ }


TSqlTransaction::~TSqlTransaction()
{
    rollbackAll();
}


bool TSqlTransaction::begin(QSqlDatabase &database)
{
    if (!database.isValid()) {
        tSystemError("Can not begin transaction. Invalid database: %s", qPrintable(database.connectionName()));
        return false;
    }

    if (!enabled)
        return true;

    int id = TSqlDatabasePool::getDatabaseId(database);

    if (id < 0 || id >= databases.count()) {
        tSystemError("Internal Error  [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    if (databases[id].isValid()) {
        tSystemWarn("Has begun transaction already. database:%s", qPrintable(database.connectionName()));
        return true;
    }

    if (database.transaction()) {
        Tf::traceQueryLog("[BEGIN] [databaseId:%d]", id);
    }

    databases[id] = database;
    return true;
}


bool TSqlTransaction::commit(int id)
{
    bool res = false;

    if (id >= 0 && id < databases.count()) {
        QSqlDatabase &db = databases[id];
        if (db.isValid()) {
            res = db.commit();
            if (res) {
                Tf::traceQueryLog("[COMMIT] [databaseId:%d]", id);
            } else {
                Tf::traceQueryLog("[COMMIT Failed] [databaseId:%d]", id);
            }

            db = QSqlDatabase();
        }
    }
    return res;
}


void TSqlTransaction::commitAll()
{
    for (int i = 0; i < databases.count(); ++i) {
        commit(i);
    }
}


bool TSqlTransaction::rollback(int id)
{
    bool res = false;

    if (id >= 0 && id < databases.count()) {
        QSqlDatabase &db = databases[id];
        if (db.isValid()) {
            res = db.rollback();
            if (res) {
                Tf::traceQueryLog("[ROLLBACK] [databaseId:%d]", id);
            } else {
                Tf::traceQueryLog("[ROLLBACK Failed] [databaseId:%d]", id);
            }

            db = QSqlDatabase();
        }
    }
    return res;
}


void TSqlTransaction::rollbackAll()
{
    for (int i = 0; i < databases.count(); ++i) {
        rollback(i);
    }
}
