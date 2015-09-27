/* Copyright (c) 2011-2015, AOYAMA Kazuharu
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
    rollback();
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
        tQueryLog("[BEGIN] [databaseId:%d]", id);
    }

    databases[id] = database;
    return true;
}


void TSqlTransaction::commit()
{
    for (int i = 0; i < databases.count(); ++i) {
        QSqlDatabase &db = databases[i];
        if (db.isValid()) {
            if (db.commit()) {
                tQueryLog("[COMMIT] [databaseId:%d]", i);
            }
        }
        db = QSqlDatabase();
    }
}


void TSqlTransaction::rollback()
{
    for (int i = 0; i < databases.count(); ++i) {
        QSqlDatabase &db = databases[i];
        if (db.isValid()) {
            if (db.rollback()) {
                tQueryLog("[ROLLBACK] [databaseId:%d]", i);
            }
        }
        db = QSqlDatabase();
    }
}
