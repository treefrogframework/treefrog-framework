/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QMutexLocker>
#include <QFileInfo>
#include <QDir>
#include <TWebApplication>
#include <TSqlQuery>
#include <ctime>
#include "tsqldatabase.h"
#include "tsqldatabasepool.h"
#include "tsqldriverextensionfactory.h"
#include "tsystemglobal.h"

#define CONN_NAME_FORMAT  "rdb%02d_%d"

static TSqlDatabasePool *databasePool = nullptr;


static void cleanup()
{
    delete databasePool;
    databasePool = nullptr;
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


TSqlDatabasePool::TSqlDatabasePool(const QString &environment)
    : QObject(), maxConnects(0), dbEnvironment(environment)
{ }


void TSqlDatabasePool::init()
{
    if (!Tf::app()->isSqlDatabaseAvailable()) {
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
        QString type = driverType(dbEnvironment, j);
        if (type.isEmpty()) {
            continue;
        }
        aval = true;

        auto &stack = availableNames[j];
        for (int i = 0; i < maxConnects; ++i) {
            TSqlDatabase &db = TSqlDatabase::addDatabase(type, QString().sprintf(CONN_NAME_FORMAT, j, i));
            if (!db.isValid()) {
                tWarn("Parameter 'DriverType' is invalid");
                break;
            }

            setDatabaseSettings(db, dbEnvironment, j);
            stack.push(db.connectionName());  // push onto stack
            tSystemDebug("Add Database successfully. name:%s", qPrintable(db.connectionName()));
        }
    }

    if (aval) {
        // Starts the timer to close extra-connection
        timer.start(10000, this);
    }
}


QSqlDatabase TSqlDatabasePool::database(int databaseId)
{
    T_TRACEFUNC("");
    TSqlDatabase tdb;

    if (Q_UNLIKELY(!Tf::app()->isSqlDatabaseAvailable())) {
        return tdb.sqlDatabase();
    }

    if (Q_LIKELY(databaseId >= 0 && databaseId < Tf::app()->sqlDatabaseSettingsCount())) {
        auto &cache = cachedDatabase[databaseId];
        auto &stack = availableNames[databaseId];

        for (;;) {
            QString name;
            if (cache.pop(name)) {
                tdb = TSqlDatabase::database(name);
                if (Q_LIKELY(tdb.sqlDatabase().isOpen())) {
                    tSystemDebug("Gets cached database: %s", qPrintable(tdb.connectionName()));
                    return tdb.sqlDatabase();
                } else {
                    tSystemError("Pooled database is not open: %s  [%s:%d]", qPrintable(tdb.connectionName()), __FILE__, __LINE__);
                    stack.push(name);
                    continue;
                }
            }

            if (Q_LIKELY(stack.pop(name))) {
                auto tdb = TSqlDatabase::database(name);
                if (Q_UNLIKELY(tdb.sqlDatabase().isOpen())) {
                    tSystemWarn("Gets a opend database: %s", qPrintable(tdb.connectionName()));
                    return tdb.sqlDatabase();
                } else {
                    if (Q_UNLIKELY(!tdb.sqlDatabase().open())) {
                        tError("Database open error. Invalid database settings, or maximum number of SQL connection exceeded.");
                        tSystemError("SQL database open error: %s", qPrintable(tdb.sqlDatabase().connectionName()));
                        stack.push(name);
                        return QSqlDatabase();
                    }

                    tSystemDebug("SQL database opened successfully (env:%s)", qPrintable(dbEnvironment));
                    tSystemDebug("Gets database: %s", qPrintable(tdb.sqlDatabase().connectionName()));

                    // Executes setup-queries
                    if (! tdb.postOpenStatements().isEmpty()) {
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


bool TSqlDatabasePool::setDatabaseSettings(TSqlDatabase &database, const QString &env, int databaseId)
{
    // Initiates database
    QSettings &settings = Tf::app()->sqlDatabaseSettings(databaseId);
    settings.beginGroup(env);

    QString databaseName = settings.value("DatabaseName").toString().trimmed();
    if (databaseName.isEmpty()) {
        tError("Database name empty string");
        settings.endGroup();
        return false;
    }
    tSystemDebug("SQL driver name:%s  dbname:%s", qPrintable(database.sqlDatabase().driverName()), qPrintable(databaseName));
    if (database.dbmsType() == TSqlDatabase::SQLite) {
        QFileInfo fi(databaseName);
        if (fi.isRelative()) {
            // For SQLite
            databaseName = Tf::app()->webRootPath() + databaseName;
        }
    }
    database.sqlDatabase().setDatabaseName(databaseName);

    QString hostName = settings.value("HostName").toString().trimmed();
    tSystemDebug("Database HostName: %s", qPrintable(hostName));
    if (!hostName.isEmpty()) {
        database.sqlDatabase().setHostName(hostName);
    }

    int port = settings.value("Port").toInt();
    tSystemDebug("Database Port: %d", port);
    if (port > 0) {
        database.sqlDatabase().setPort(port);
    }

    QString userName = settings.value("UserName").toString().trimmed();
    tSystemDebug("Database UserName: %s", qPrintable(userName));
    if (!userName.isEmpty()) {
        database.sqlDatabase().setUserName(userName);
    }

    QString password = settings.value("Password").toString().trimmed();
    tSystemDebug("Database Password: %s", qPrintable(password));
    if (!password.isEmpty()) {
        database.sqlDatabase().setPassword(password);
    }

    QString connectOptions = settings.value("ConnectOptions").toString().trimmed();
    tSystemDebug("Database ConnectOptions: %s", qPrintable(connectOptions));
    if (!connectOptions.isEmpty()) {
        database.sqlDatabase().setConnectOptions(connectOptions);
    }

    QStringList postOpenStatements = settings.value("PostOpenStatements").toString().trimmed().split(";", QString::SkipEmptyParts);
    tSystemDebug("Database postOpenStatements: %s", qPrintable(postOpenStatements.join(";")));
    if (!postOpenStatements.isEmpty()) {
        database.setPostOpenStatements(postOpenStatements);
    }

    bool enableUpsert = settings.value("EnableUpsert", false).toBool();
    tSystemDebug("Database enableUpsert: %d", enableUpsert);
    database.setUpsertEnabled(enableUpsert);

    auto *extension = TSqlDriverExtensionFactory::create(database.sqlDatabase().driverName(), database.sqlDatabase().driver());
    database.setDriverExtension(extension);

    settings.endGroup();
    return true;
}


void TSqlDatabasePool::pool(QSqlDatabase &database)
{
    T_TRACEFUNC("");

    if (database.isValid()) {
        int databaseId = getDatabaseId(database);

        if (databaseId >= 0 && databaseId < Tf::app()->sqlDatabaseSettingsCount()) {
            // pool
            cachedDatabase[databaseId].push(database.connectionName());
            lastCachedTime[databaseId].store((uint)std::time(nullptr));
            tSystemDebug("Pooled database: %s", qPrintable(database.connectionName()));
        } else {
            tSystemError("Pooled invalid database  [%s:%d]", __FILE__, __LINE__);
        }
    }
    database = QSqlDatabase();  // Sets an invalid object
}


void TSqlDatabasePool::timerEvent(QTimerEvent *event)
{
    T_TRACEFUNC("");

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
                db.close();
                tSystemDebug("Closed database connection, name: %s", qPrintable(name));
                availableNames[i].push(name);
            }
        }
    } else {
        QObject::timerEvent(event);
    }
}


/*!
 * Initializes.
 * Call this in main thread.
 */
void TSqlDatabasePool::instantiate()
{
    if (!databasePool) {
        databasePool = new TSqlDatabasePool(Tf::app()->databaseEnvironment());
        databasePool->maxConnects = Tf::app()->maxNumberOfThreadsPerAppServer();
        databasePool->init();
        qAddPostRoutine(::cleanup);
    }
}


TSqlDatabasePool *TSqlDatabasePool::instance()
{
    if (Q_UNLIKELY(!databasePool)) {
        tFatal("Call TSqlDatabasePool::initialize() function first");
    }
    return databasePool;
}


QString TSqlDatabasePool::driverType(const QString &env, int databaseId)
{
    QSettings &settings = Tf::app()->sqlDatabaseSettings(databaseId);
    settings.beginGroup(env);
    QString type = settings.value("DriverType").toString().trimmed();
    settings.endGroup();

    if (type.isEmpty()) {
        tDebug("Parameter 'DriverType' is empty");
    }
    return type;
}


int TSqlDatabasePool::getDatabaseId(const QSqlDatabase &database)
{
    bool ok;
    int id = database.connectionName().mid(3,2).toInt(&ok);

    if (Q_LIKELY(ok && id >= 0)) {
        return id;
    }
    return -1;
}
