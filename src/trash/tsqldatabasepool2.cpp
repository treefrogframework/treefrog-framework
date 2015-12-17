/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <TWebApplication>
#include <TAppSettings>
#include "tsqldatabasepool2.h"
#include "tatomicset.h"
#include "tsystemglobal.h"

#define CONN_NAME_FORMAT  "rdb%02d_%d"

static TSqlDatabasePool2 *databasePool = 0;


static void cleanup()
{
    if (databasePool) {
        delete databasePool;
        databasePool = 0;
    }
}


TSqlDatabasePool2::TSqlDatabasePool2(const QString &environment)
    : QObject(), dbSet(0), maxConnects(0), dbEnvironment(environment)
{
    // Starts the timer to close extra-connection
    timer.start(10000, this);
}


TSqlDatabasePool2::~TSqlDatabasePool2()
{
    timer.stop();

    int setCount = Tf::app()->sqlDatabaseSettingsCount();

    for (int j = 0; j < setCount; ++j) {
        for (int i = 0; i < maxConnects; ++i) {
            QString dbName = QString().sprintf(CONN_NAME_FORMAT, j, i);

            QSqlDatabase::database(dbName, false).close();
            if (QSqlDatabase::contains(dbName)) {
                QSqlDatabase::removeDatabase(dbName);
            }

            DatabaseUse *du = (DatabaseUse *)dbSet[j].peekPop(i);
            if (du) {
                delete du;
            } else {
                tSystemWarn("Leak memory of DatabaseUse: %s", qPrintable(dbName));
            }
        }
    }

    if (dbSet)
        delete[] dbSet;
}


void TSqlDatabasePool2::init()
{
    if (!Tf::app()->isSqlDatabaseAvailable()) {
        tSystemWarn("SQL database not available");
        return;
    } else {
        tSystemDebug("SQL database available");
    }

    int setCount = Tf::app()->sqlDatabaseSettingsCount();

    dbSet = new TAtomicSet[setCount];

    // Adds databases previously
    for (int j = 0; j < setCount; ++j) {
        dbSet[j].setMaxCount(maxConnects);

        QString type = driverType(dbEnvironment, j);
        if (type.isEmpty()) {
            continue;
        }

        for (int i = 0; i < maxConnects; ++i) {
            QString dbName = QString().sprintf(CONN_NAME_FORMAT, j, i);
            QSqlDatabase db = QSqlDatabase::addDatabase(type, dbName);
            if (!db.isValid()) {
                tWarn("Parameter 'DriverType' is invalid");
                break;
            }

            setDatabaseSettings(db, dbEnvironment, j);

            DatabaseUse *du = new DatabaseUse;
            du->dbName = dbName;
            du->lastUsed = 0;
            if (dbSet[j].push(du)) {
                tSystemDebug("Add Database successfully. name:%s", qPrintable(db.connectionName()));
            } else {
                tSystemError("Failed to add database. name:%s", qPrintable(db.connectionName()));
                delete du;
            }
        }
    }
}


QSqlDatabase TSqlDatabasePool2::database(int databaseId)
{
    QSqlDatabase db;

    if (!Tf::app()->isSqlDatabaseAvailable()) {
        return db;
    }

    int setCount = Tf::app()->sqlDatabaseSettingsCount();

    if (databaseId >= 0 && databaseId < setCount) {
        DatabaseUse *du = (DatabaseUse *)dbSet[databaseId].pop();
        if (du) {
            db = QSqlDatabase::database(du->dbName, false);
            delete du;

            if (!db.isOpen()) {
                if (!db.open()) {
                    tError("Database open error. Invalid database settings, or maximum number of SQL connection exceeded.");
                    tSystemError("Database open error: %s", qPrintable(db.connectionName()));
                    return QSqlDatabase();
                }

                tSystemDebug("Database opened successfully (env:%s)", qPrintable(dbEnvironment));
                tSystemDebug("Gets database: %s", qPrintable(db.connectionName()));
            }
            return db;
        }
    }

    throw RuntimeException("No pooled connection", __FILE__, __LINE__);
}


bool TSqlDatabasePool2::setDatabaseSettings(QSqlDatabase &database, const QString &env, int databaseId)
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
    tSystemDebug("SQL driver name:%s  dbname:%s", qPrintable(database.driverName()), qPrintable(databaseName));

    if (database.driverName().toUpper().startsWith("QSQLITE")) {
        QFileInfo fi(databaseName);
        if (fi.isRelative()) {
            // For SQLite
            databaseName = Tf::app()->webRootPath() + databaseName;
        }
    }
    database.setDatabaseName(databaseName);

    QString hostName = settings.value("HostName").toString().trimmed();
    tSystemDebug("Database HostName: %s", qPrintable(hostName));
    if (!hostName.isEmpty())
        database.setHostName(hostName);

    int port = settings.value("Port").toInt();
    tSystemDebug("Database Port: %d", port);
    if (port > 0)
        database.setPort(port);

    QString userName = settings.value("UserName").toString().trimmed();
    tSystemDebug("Database UserName: %s", qPrintable(userName));
    if (!userName.isEmpty())
        database.setUserName(userName);

    QString password = settings.value("Password").toString().trimmed();
    tSystemDebug("Database Password: %s", qPrintable(password));
    if (!password.isEmpty())
        database.setPassword(password);

    QString connectOptions = settings.value("ConnectOptions").toString().trimmed();
    tSystemDebug("Database ConnectOptions: %s", qPrintable(connectOptions));
    if (!connectOptions.isEmpty())
        database.setConnectOptions(connectOptions);

    settings.endGroup();
    return true;
}


void TSqlDatabasePool2::pool(QSqlDatabase &database)
{
    if (database.isValid()) {
        int databaseId = getDatabaseId(database);

        if (databaseId >= 0 && databaseId < Tf::app()->sqlDatabaseSettingsCount()) {
            DatabaseUse *du = new DatabaseUse;
            du->dbName = database.connectionName();
            du->lastUsed = QDateTime::currentDateTime().toTime_t();

            if (dbSet[databaseId].push(du)) {
                tSystemDebug("Pooled database: %s", qPrintable(database.connectionName()));
            } else {
                tSystemError("Failed to pool database: %s", qPrintable(database.connectionName()));
                delete du;
            }
        } else {
            tSystemError("Pooled invalid database  [%s:%d]", __FILE__, __LINE__);
        }
    }
    database = QSqlDatabase();  // Sets an invalid object
}


void TSqlDatabasePool2::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != timer.timerId()) {
        QObject::timerEvent(event);
        return;
    }

    if (!Tf::app()->isSqlDatabaseAvailable()) {
        return;
    }

    // Closes connection
    int setCount = Tf::app()->sqlDatabaseSettingsCount();
    for (int j = 0; j < setCount; ++j) {
        for (int i = 0; i < dbSet[j].maxCount(); ++i) {
            DatabaseUse *du = (DatabaseUse *)dbSet[j].peekPop(i);
            if (du) {
                if (du->lastUsed < QDateTime::currentDateTime().toTime_t() - 30) {
                    QSqlDatabase db = QSqlDatabase::database(du->dbName, false);
                    if (db.isOpen()) {
                        db.close();
                        tSystemDebug("Closed database connection, name: %s", qPrintable(du->dbName));
                    }
                }
                dbSet[j].peekPush(du);
            }
        }
    }
}


/*!
 * Initializes.
 * Call this in main thread.
 */
void TSqlDatabasePool2::instantiate(int maxConnections)
{
    if (!databasePool) {
        databasePool = new TSqlDatabasePool2(Tf::app()->databaseEnvironment());
        databasePool->maxConnects = (maxConnections > 0) ? maxConnections : maxDbConnectionsPerProcess();
        databasePool->init();
        qAddPostRoutine(::cleanup);
    }
}


TSqlDatabasePool2 *TSqlDatabasePool2::instance()
{
    if (Q_UNLIKELY(!databasePool)) {
        tFatal("Call TSqlDatabasePool2::initialize() function first");
    }
    return databasePool;
}


QString TSqlDatabasePool2::driverType(const QString &env, int databaseId)
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


int TSqlDatabasePool2::maxDbConnectionsPerProcess()
{
    int maxConnections = 0;
    QString mpm = Tf::appSettings()->value(Tf::MultiProcessingModule).toString().toLower();

    switch (Tf::app()->multiProcessingModule()) {
    case TWebApplication::Thread:
        maxConnections = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxThreadsPerAppServer").toInt();
        if (maxConnections <= 0) {
            maxConnections = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxServers", "128").toInt();
        }
        break;

    case TWebApplication::Prefork:
        maxConnections = 1;
        break;

    case TWebApplication::Hybrid:
        maxConnections = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxWorkersPerAppServer").toInt();
        if (maxConnections <= 0) {
            maxConnections = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxWorkersPerServer", "128").toInt();
        }
        break;

    default:
        break;
    }

    return maxConnections;
}


int TSqlDatabasePool2::getDatabaseId(const QSqlDatabase &database)
{
    bool ok;
    int id = database.connectionName().mid(3,2).toInt(&ok);

    if (Q_LIKELY(ok && id >= 0)) {
        return id;
    }
    return -1;
}
