/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsqldatabasepool.h"
#include "tsqldatabase.h"
#include "tsqldriverextensionfactory.h"
#include "tsystemglobal.h"
#include <QDir>
#include <QFileInfo>
#include <QMutexLocker>
#include <TAppSettings>
#include <TSqlQuery>
#include <TWebApplication>
#include <ctime>

constexpr auto CONN_NAME_FORMAT = "rdb%02d_%d";


TSqlDatabasePool *TSqlDatabasePool::instance()
{
    static TSqlDatabasePool *databasePool = []() {
        auto *pool = new TSqlDatabasePool;
        pool->maxConnects = Tf::app()->maxNumberOfThreadsPerAppServer();
        pool->init();
        return pool;
    }();
    return databasePool;
}


TSqlDatabasePool::TSqlDatabasePool() :
    QObject()
{
}


TSqlDatabasePool::~TSqlDatabasePool()
{
    timer.stop();

    for (int j = 0; j < Tf::app()->sqlDatabaseSettingsCount(); ++j) {
        auto &cache = cachedDatabase[j];
        QString name;
        while (cache.pop(name)) {
            QSqlDatabase db = TSqlDatabase::database(name).sqlDatabase();
            db.close();
            TSqlDatabase::removeDatabase(name);
        }

        auto &stack = availableNames[j];
        while (stack.pop(name)) {
            TSqlDatabase::removeDatabase(name);
        }
    }

    delete[] cachedDatabase;
    delete[] lastCachedTime;
    delete[] availableNames;
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

    cachedDatabase = new TStack<QString>[Tf::app()->sqlDatabaseSettingsCount()];
    lastCachedTime = new TAtomic<uint>[Tf::app()->sqlDatabaseSettingsCount()];
    availableNames = new TStack<QString>[Tf::app()->sqlDatabaseSettingsCount()];
    bool aval = false;
    tSystemDebug("SQL database available");

    // Adds databases previously
    for (int j = 0; j < Tf::app()->sqlDatabaseSettingsCount(); ++j) {
        QString type = driverType(j);
        if (type.isEmpty()) {
            continue;
        }
        aval = true;

        auto &stack = availableNames[j];
        for (int i = 0; i < maxConnects; ++i) {
            TSqlDatabase &db = TSqlDatabase::addDatabase(type, QString::asprintf(CONN_NAME_FORMAT, j, i));
            if (!db.isValid()) {
                tWarn("Parameter 'DriverType' is invalid");
                break;
            }

            setDatabaseSettings(db, j);
            stack.push(db.connectionName());  // push onto stack
            tSystemDebug("Add Database successfully. name:%s", qUtf8Printable(db.connectionName()));
        }
    }

    if (aval) {
        // Starts the timer to close extra-connection
        timer.start(10000, this);
    }
}


QSqlDatabase TSqlDatabasePool::database(int databaseId)
{
    TSqlDatabase tdb;

    if (Q_LIKELY(databaseId >= 0 && databaseId < Tf::app()->sqlDatabaseSettingsCount())) {
        auto &cache = cachedDatabase[databaseId];
        auto &stack = availableNames[databaseId];

        for (;;) {
            QString name;
            if (cache.pop(name)) {
                tdb = TSqlDatabase::database(name);
                if (Q_LIKELY(tdb.sqlDatabase().isOpen())) {
                    tSystemDebug("Gets cached database: %s", qUtf8Printable(tdb.connectionName()));
                    return tdb.sqlDatabase();
                } else {
                    tSystemError("Pooled database is not open: %s  [%s:%d]", qUtf8Printable(tdb.connectionName()), __FILE__, __LINE__);
                    stack.push(name);
                    continue;
                }
            }

            if (Q_LIKELY(stack.pop(name))) {
                auto tdb = TSqlDatabase::database(name);
                if (Q_UNLIKELY(tdb.sqlDatabase().isOpen())) {
                    tSystemWarn("Gets a opend database: %s", qUtf8Printable(tdb.connectionName()));
                    return tdb.sqlDatabase();
                } else {
                    if (Q_UNLIKELY(!tdb.sqlDatabase().open())) {
                        tError("Database open error. Invalid database settings, or maximum number of SQL connection exceeded.");
                        tSystemError("SQL database open error: %s", qUtf8Printable(tdb.sqlDatabase().connectionName()));
                        stack.push(name);
                        return QSqlDatabase();
                    }

                    tSystemDebug("SQL database opened successfully (env:%s)", qUtf8Printable(Tf::app()->databaseEnvironment()));
                    tSystemDebug("Gets database: %s", qUtf8Printable(tdb.sqlDatabase().connectionName()));

                    // Executes setup-queries
                    if (!tdb.postOpenStatements().isEmpty()) {
                        TSqlQuery query(tdb.sqlDatabase());
                        for (QString st : tdb.postOpenStatements()) {
                            st = st.trimmed();
                            query.exec(st);
                        }
                    }
                    return tdb.sqlDatabase();
                }
            }
        }
    }
    throw RuntimeException("No pooled connection", __FILE__, __LINE__);
}


bool TSqlDatabasePool::setDatabaseSettings(TSqlDatabase &database, int databaseId)
{
    // Initiates database
    auto settings = Tf::app()->sqlDatabaseSettings(databaseId);

    QString databaseName = settings.value("DatabaseName").toString().trimmed();
    if (databaseName.isEmpty()) {
        tError("Database name empty string");
        return false;
    }
    tSystemDebug("SQL driver name:%s  dbname:%s", qUtf8Printable(database.sqlDatabase().driverName()), qUtf8Printable(databaseName));
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
    tSystemDebug("Database HostName: %s", qUtf8Printable(hostName));
    if (!hostName.isEmpty()) {
        database.sqlDatabase().setHostName(hostName);
    }

    int port = settings.value("Port").toInt();
    tSystemDebug("Database Port: %d", port);
    if (port > 0) {
        database.sqlDatabase().setPort(port);
    }

    QString userName = settings.value("UserName").toString().trimmed();
    tSystemDebug("Database UserName: %s", qUtf8Printable(userName));
    if (!userName.isEmpty()) {
        database.sqlDatabase().setUserName(userName);
    }

    QString password = settings.value("Password").toString().trimmed();
    tSystemDebug("Database Password: %s", qUtf8Printable(password));
    if (!password.isEmpty()) {
        database.sqlDatabase().setPassword(password);
    }

    QString connectOptions = settings.value("ConnectOptions").toString().trimmed();
    tSystemDebug("Database ConnectOptions: %s", qUtf8Printable(connectOptions));
    if (!connectOptions.isEmpty()) {
        database.sqlDatabase().setConnectOptions(connectOptions);
    }

    QStringList postOpenStatements = settings.value("PostOpenStatements").toString().trimmed().split(";", Tf::SkipEmptyParts);
    tSystemDebug("Database postOpenStatements: %s", qUtf8Printable(postOpenStatements.join(";")));
    if (!postOpenStatements.isEmpty()) {
        database.setPostOpenStatements(postOpenStatements);
    }

    bool enableUpsert = settings.value("EnableUpsert", false).toBool();
    tSystemDebug("Database enableUpsert: %d", enableUpsert);
    database.setUpsertEnabled(enableUpsert);

    auto *extension = TSqlDriverExtensionFactory::create(database.sqlDatabase().driverName(), database.sqlDatabase().driver());
    database.setDriverExtension(extension);

    return true;
}


void TSqlDatabasePool::pool(QSqlDatabase &database, bool forceClose)
{
    if (database.isValid()) {
        int databaseId = getDatabaseId(database);

        if (databaseId >= 0 && databaseId < Tf::app()->sqlDatabaseSettingsCount()) {
            if (forceClose) {
                tSystemWarn("Force close database: %s", qUtf8Printable(database.connectionName()));
                closeDatabase(database);
            } else {
                // pool
                cachedDatabase[databaseId].push(database.connectionName());
                lastCachedTime[databaseId].store((uint)std::time(nullptr));
                tSystemDebug("Pooled database: %s", qUtf8Printable(database.connectionName()));
            }
        } else {
            tSystemError("Pooled invalid database  [%s:%d]", __FILE__, __LINE__);
        }
    }
    database = QSqlDatabase();  // Sets an invalid object
}


void TSqlDatabasePool::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == timer.timerId()) {
        QString name;

        // Closes extra-connection
        for (int i = 0; i < Tf::app()->sqlDatabaseSettingsCount(); ++i) {
            auto &cache = cachedDatabase[i];
            if (cache.count() == 0) {
                continue;
            }

            while (lastCachedTime[i].load() < (uint)std::time(nullptr) - 30
                && cache.pop(name)) {
                QSqlDatabase db = TSqlDatabase::database(name).sqlDatabase();
                closeDatabase(db);
            }
        }
    } else {
        QObject::timerEvent(event);
    }
}


void TSqlDatabasePool::closeDatabase(QSqlDatabase &database)
{
    int id = getDatabaseId(database);
    QString name = database.connectionName();
    database.close();
    tSystemDebug("Closed database connection, name: %s", qUtf8Printable(name));
    availableNames[id].push(name);
}


int TSqlDatabasePool::getDatabaseId(const QSqlDatabase &database)
{
    bool ok;
    int id = database.connectionName().mid(3, 2).toInt(&ok);

    if (Q_LIKELY(ok && id >= 0)) {
        return id;
    }
    return -1;
}
