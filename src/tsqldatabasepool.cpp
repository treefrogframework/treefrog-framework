/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsqldatabasepool.h"
#include "tsqldatabase.h"
#include "tsqldriverextensionfactory.h"
#include "tsqldriverextension.h"
#include "tsystemglobal.h"
#include <QDir>
#include <QFileInfo>
#include <QMutexLocker>
#include <TAppSettings>
#include <TSqlQuery>
#include <QMutexLocker>
#include <TWebApplication>
#include <ctime>

constexpr auto CONN_NAME_FORMAT = "rdb%02d_%d";


TSqlDatabasePool *TSqlDatabasePool::instance()
{
    static std::unique_ptr<TSqlDatabasePool> databasePool = []() {
        std::unique_ptr<TSqlDatabasePool> pool{new TSqlDatabasePool};
        pool->maxConnects = Tf::app()->maxNumberOfThreadsPerAppServer();
        pool->init();
        return pool;
    }();
    return databasePool.get();
}


TSqlDatabasePool::TSqlDatabasePool() :
    QObject()
{
}


TSqlDatabasePool::~TSqlDatabasePool()
{
    timer.stop();

    for (int j = 0; j < Tf::app()->sqlDatabaseSettingsCount(); ++j) {
        auto &cache = cachedDatabases[j];
        QString name;
        while (!cache.empty()) {
            std::optional<SqlDbPtr> db = cache.pop();
            if (db) {
                name = (*db)->connectionName();
                closeDatabase(*(*db));
                TSqlDatabase::removeDatabase(name);
            }
        }

        auto &stack = availableDatabases[j];
        while (!stack.empty()) {
            std::optional<SqlDbPtr> db = stack.pop();
            if (db) {
                TSqlDatabase::removeDatabase((*db)->sqlDatabase().connectionName());
            }
        }
    }

    delete[] lastCachedTime;
}


static QString driverType(int databaseId)
{
    auto settings = Tf::app()->sqlDatabaseSettings(databaseId);
    QString key = QLatin1String("DriverType");
    QString type = settings.value(key).toString().trimmed();

    if (type.isEmpty()) {
        tWarn() << "Empty parameter: " << key << " databaseId:" << databaseId;
    }
    return type;
}


void TSqlDatabasePool::init()
{
    if (Tf::app()->sqlDatabaseSettingsCount() == 0) {
        tSystemWarn("SQL database not available");
        return;
    }

    //QMutexLocker locker(&_mutex);

    if (lastCachedTime) {
        return;
    }

    const int dbcount = Tf::app()->sqlDatabaseSettingsCount();
    cachedDatabases.resize(dbcount);
    availableDatabases.resize(dbcount);
    lastCachedTime = new TAtomic<uint>[dbcount];
    tSystemDebug("SQL database available. maxConnects:{}", maxConnects);
    bool aval = false;

    // Adds databases previously
    for (int j = 0; j < dbcount; ++j) {
        QString type = driverType(j);
        if (type.isEmpty()) {
            continue;
        }
        aval = true;

        auto &stack = availableDatabases[j];
        for (int i = 0; i < maxConnects; ++i) {
            QString name = QString::asprintf(CONN_NAME_FORMAT, j, i);
            bool res = TSqlDatabase::addDatabase(type, name);
            if (!res) {
                Tf::warn("Parameter 'DriverType' is invalid");
                continue;
            }
            auto db = TSqlDatabase::database(name);
            if (!db->isValid()) {
                Tf::warn("Parameter 'DriverType' is invalid");
                break;
            }

            setDatabaseSettings(*db, j);
            tSystemDebug("Add Database successfully. name:{}", db->connectionName());
            stack.push(std::move(db));  // push onto stack
        }
    }

    if (aval) {
        // Starts the timer to close extra-connection
        timer.start(10000, this);
    }
}


TSqlDatabase::Handle TSqlDatabasePool::database(int databaseId)
{
    if (databaseId < 0 || databaseId >= Tf::app()->sqlDatabaseSettingsCount()) {
        throw RuntimeException("No pooled connection", __FILE__, __LINE__);
    }

    auto &cache = cachedDatabases[databaseId];
    auto &stack = availableDatabases[databaseId];

    while (true) {
        std::optional<SqlDbPtr> sqlptr = cache.pop();
        if (sqlptr) {
            if ((*sqlptr)->sqlDatabase().isOpen()) {
                tSystemDebug("Gets cached database: {}", (*sqlptr)->connectionName());
                return TSqlDatabase::Handle(std::move((*sqlptr)));
            } else {
                tSystemError("Pooled database is not open: {}  [{}:{}]", (*sqlptr)->connectionName(), __FILE__, __LINE__);
                stack.push(std::move(*sqlptr));
                continue;
            }
        }

        sqlptr = stack.pop();
        if (!sqlptr) {
            break;
        }

        if (!openDatabase(*(*sqlptr))) {
            Tf::error("Database open error. Invalid database settings, or maximum number of SQL connection exceeded.");
            tSystemError("SQL database open error: {}", (*sqlptr)->sqlDatabase().connectionName());
            stack.push(std::move(*sqlptr));
            return TSqlDatabase::Handle();
        }

        tSystemDebug("SQL database opened successfully (env:{})", Tf::app()->databaseEnvironment());
        tSystemDebug("Gets database: {}", (*sqlptr)->sqlDatabase().connectionName());

        // Executes setup-queries
        if (!(*sqlptr)->postOpenStatements().isEmpty()) {
            TSqlQuery query((*sqlptr)->sqlDatabase());
            for (QString st : (*sqlptr)->postOpenStatements()) {
                st = st.trimmed();
                query.exec(st);
            }
        }
        return TSqlDatabase::Handle(std::move(*sqlptr));
    }

    tSystemError("Empty available databases  [{}:{}]", __FILE__, __LINE__);
    return TSqlDatabase::Handle();
}


bool TSqlDatabasePool::setDatabaseSettings(TSqlDatabase &database, int databaseId)
{
    // Initiates database
    auto settings = Tf::app()->sqlDatabaseSettings(databaseId);

    QString databaseName = settings.value("DatabaseName").toString().trimmed();
    if (databaseName.isEmpty()) {
        Tf::error("Database name empty string");
        return false;
    }
    tSystemDebug("SQL driver name:{}  dbname:{}", database.sqlDatabase().driverName(), databaseName);
    if (database.dbmsType() == TSqlDatabase::SQLite) {
        if (!databaseName.contains(':')) {
            QFileInfo fi(databaseName);
            if (fi.isRelative()) {
                // For SQLite
                databaseName = Tf::app()->webRootPath() + databaseName;
            }
        }
    }
    database.sqlDatabase().setDatabaseName(databaseName);

    QString hostName = settings.value("HostName").toString().trimmed();
    tSystemDebug("Database HostName: {}", hostName);
    if (!hostName.isEmpty()) {
        database.sqlDatabase().setHostName(hostName);
    }

    int port = settings.value("Port").toInt();
    tSystemDebug("Database Port: {}", port);
    if (port > 0) {
        database.sqlDatabase().setPort(port);
    }

    QString userName = settings.value("UserName").toString().trimmed();
    tSystemDebug("Database UserName: {}", userName);
    if (!userName.isEmpty()) {
        database.sqlDatabase().setUserName(userName);
    }

    QString password = settings.value("Password").toString().trimmed();
    tSystemDebug("Database Password: {}", password);
    if (!password.isEmpty()) {
        database.sqlDatabase().setPassword(password);
    }

    QString connectOptions = settings.value("ConnectOptions").toString().trimmed();
    tSystemDebug("Database ConnectOptions: {}", connectOptions);
    if (!connectOptions.isEmpty()) {
        database.sqlDatabase().setConnectOptions(connectOptions);
    }

    QStringList postOpenStatements = settings.value("PostOpenStatements").toString().trimmed().split(";", Tf::SkipEmptyParts);
    tSystemDebug("Database postOpenStatements: {}", postOpenStatements.join(";"));
    if (!postOpenStatements.isEmpty()) {
        database.setPostOpenStatements(postOpenStatements);
    }

    bool enableUpsert = settings.value("EnableUpsert", false).toBool();
    tSystemDebug("Database enableUpsert: {}", enableUpsert);
    database.setUpsertEnabled(enableUpsert);

    return true;
}


void TSqlDatabasePool::pool(SqlDbPtr dbptr, bool forceClose)
{
    //QMutexLocker locker(&_mutex);

    if (dbptr->isValid()) {
        int databaseId = getDatabaseId(dbptr->sqlDatabase());

        if (databaseId >= 0 && databaseId < Tf::app()->sqlDatabaseSettingsCount()) {
            if (forceClose) {
                tSystemWarn("Force close database: {}", dbptr->connectionName());
                closeDatabase(*dbptr);
                availableDatabases[databaseId].push(std::move(dbptr));
            } else {
                if (dbptr->sqlDatabase().isOpen()) {
                    // pool
                    tSystemDebug("Pooled cached connection, name: {}", dbptr->connectionName());
                    cachedDatabases[databaseId].push(std::move(dbptr));
                    lastCachedTime[databaseId].store((uint)std::time(nullptr));
                } else {
                    tSystemWarn("Pooled database, name: {}", dbptr->connectionName());
                    availableDatabases[databaseId].push(std::move(dbptr));
                }
            }
        } else {
            tSystemError("Pooled invalid database  [{}:{}]", __FILE__, __LINE__);
        }
    }
}


void TSqlDatabasePool::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == timer.timerId()) {
        //QMutexLocker locker(&_mutex);
        QString name;

#if 0
        // Closes extra-connection
        for (int i = 0; i < Tf::app()->sqlDatabaseSettingsCount(); ++i) {
            auto &cache = cachedDatabase[i];
            if (cache.count() == 0) {
                continue;
            }

            while (lastCachedTime[i].load() < (uint)std::time(nullptr) - 30
                && !cache.isEmpty()) {
                name = cache.pop();
                TSqlDatabase &db = TSqlDatabase::database(name);
                closeDatabase(db);
            }
        }
#else
        // Closes extra-connection
        for (int i = 0; i < Tf::app()->sqlDatabaseSettingsCount(); ++i) {
            auto &cache = cachedDatabases[i];
            if (cache.size() == 0) {
                continue;
            }

            while (lastCachedTime[i].load() < (uint)std::time(nullptr) - 30
                && !cache.empty()) {
                auto db = cache.pop();
                if (db) {
                    closeDatabase(*(*db));
                    availableDatabases[i].push(std::move(*db));
                }
            }
        }
#endif
    } else {
        QObject::timerEvent(event);
    }
}


bool TSqlDatabasePool::openDatabase(TSqlDatabase &database)
{
    bool ret = database.sqlDatabase().open();

    if (ret) {
        TSqlDriverExtension *extension = database.driverExtension();
        if (extension) {
            TSqlDriverExtensionFactory::destroy(database.sqlDatabase().driverName(), extension);
        }

        extension = TSqlDriverExtensionFactory::create(database.sqlDatabase().driverName(), database.sqlDatabase().driver());
        database.setDriverExtension(std::unique_ptr<TSqlDriverExtension>{extension});
    }

    return ret;
}


void TSqlDatabasePool::closeDatabase(TSqlDatabase &database)
{
    //QMutexLocker locker(&_mutex);

    QSqlDatabase db = database.sqlDatabase();
    QString name = db.connectionName();
    db.close();

    TSqlDriverExtension *extension = database.driverExtension();
    if (extension) {
        TSqlDriverExtensionFactory::destroy(database.sqlDatabase().driverName(), extension);
    }
    database.setDriverExtension(std::unique_ptr<TSqlDriverExtension>{nullptr});

    tSystemDebug("Closed database connection, name: {}", name);
}


int TSqlDatabasePool::getDatabaseId(const QSqlDatabase &database)
{
    return databaseIdFromName(database.connectionName());
}


int TSqlDatabasePool::databaseIdFromName(const QString &name)
{
    bool ok;
    int id = name.mid(3, 2).toInt(&ok);
    if (ok && id >= 0) {
        return id;
    }
    return -1;
}
