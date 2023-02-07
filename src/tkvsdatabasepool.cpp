/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tkvsdatabasepool.h"
#include "tfnamespace.h"
#include "tsqldatabasepool.h"
#include "tsystemglobal.h"
#include "tstack.h"
#include <QDateTime>
#include <QMap>
#include <QStringList>
#include <QThread>
#include <QMutexLocker>
#include <TWebApplication>
#include <ctime>

/*!
  \class TKvsDatabasePool
  \brief The TKvsDatabasePool class manages a collection of TKvsDatabase instances.
*/

constexpr auto CONN_NAME_FORMAT = "%02dkvs_%d";


class KvsEngineHash : public QMap<Tf::KvsEngine, QString> {
public:
    KvsEngineHash() :
        QMap<Tf::KvsEngine, QString>()
    {
        // DriverName, Engine
        insert(Tf::KvsEngine::MongoDB, "mongodb");
        insert(Tf::KvsEngine::Redis, "redis");
        insert(Tf::KvsEngine::Memcached, "memcached");
        insert(Tf::KvsEngine::SharedMemory, "memory");

        if (Tf::app()->isKvsAvailable(Tf::KvsEngine::CacheKvs)) {
            auto backend = Tf::app()->cacheBackend();
            insert(Tf::KvsEngine::CacheKvs, backend);
        }
    }
};

static KvsEngineHash *kvsEngineHash()
{
    static auto *hash = new KvsEngineHash;
    return hash;
}


TKvsDatabasePool *TKvsDatabasePool::instance()
{
    static TKvsDatabasePool *databasePool = []() {
        auto *pool = new TKvsDatabasePool;
        pool->maxConnects = Tf::app()->maxNumberOfThreadsPerAppServer();
        pool->init();
        return pool;
    }();
    return databasePool;
}


TKvsDatabasePool::TKvsDatabasePool() :
    QObject()
{
}


TKvsDatabasePool::~TKvsDatabasePool()
{
    QMutexLocker locker(&_mutex);
    timer.stop();

    for (int eng = 0; eng < (int)Tf::KvsEngine::Num; eng++) {
        if (!Tf::app()->isKvsAvailable((Tf::KvsEngine)eng)) {
            continue;
        }

        auto &cache = cachedDatabase[eng];
        QString name;
        while (!cache.isEmpty()) {
            name = cache.pop();
            TKvsDatabase::database(name).close();
            TKvsDatabase::removeDatabase(name);
        }

        auto &stack = availableNames[eng];
        while (!stack.isEmpty()) {
            name = stack.pop();
            TKvsDatabase::removeDatabase(name);
        }
    }

    delete[] cachedDatabase;
    delete[] lastCachedTime;
    delete[] availableNames;
}


void TKvsDatabasePool::init()
{
    if (cachedDatabase) {
        return;
    }

    QMutexLocker locker(&_mutex);

    cachedDatabase = new QStack<QString>[(int)Tf::KvsEngine::Num];
    lastCachedTime = new TAtomic<uint>[(int)Tf::KvsEngine::Num];
    availableNames = new QStack<QString>[(int)Tf::KvsEngine::Num];
    bool aval = false;

    // Adds databases previously
    for (auto it = kvsEngineHash()->begin(); it != kvsEngineHash()->end(); ++it) {
        Tf::KvsEngine engine = it.key();
        const QString &drv = it.value();

        if (!Tf::app()->isKvsAvailable(engine)) {
            tSystemDebug("KVS database not available. engine:%d", (int)engine);
            continue;
        } else {
            aval = true;
            tSystemDebug("KVS database available. engine:%d", (int)engine);
        }

        auto &stack = availableNames[(int)engine];
        for (int i = 0; i < maxConnects; ++i) {
            TKvsDatabase db = TKvsDatabase::addDatabase(drv, QString::asprintf(CONN_NAME_FORMAT, (int)engine, i));
            if (!db.isValid()) {
                tWarn("KVS init parameter is invalid");
                break;
            }

            setDatabaseSettings(db, engine);
            stack.push(db.connectionName());  // push onto stack
            tSystemDebug("Add KVS successfully. name:%s", qUtf8Printable(db.connectionName()));
        }
    }

    if (aval) {
        // Starts the timer to close extra-connection
        timer.start(10000, this);
    }
}


TKvsDatabase TKvsDatabasePool::database(Tf::KvsEngine engine)
{
    QMutexLocker locker(&_mutex);

    if (!Tf::app()->isKvsAvailable(engine)) {
        switch (engine) {
        case Tf::KvsEngine::MongoDB:
            tSystemError("MongoDB not available. Check the settings file.");
            break;

        case Tf::KvsEngine::Redis:
            tSystemError("Redis not available. Check the settings file.");
            break;

        case Tf::KvsEngine::Memcached:
            tSystemError("Memcached not available. Check the settings file.");
            break;

        case Tf::KvsEngine::SharedMemory:
            tSystemWarn("SharedMemory not available. Check the settings file.");
            break;

        case Tf::KvsEngine::CacheKvs:
            tSystemError("CacheKvs not available. Check the settings file.");
            break;

        default:
            throw RuntimeException("No such KVS engine", __FILE__, __LINE__);
            break;
        }
        return TKvsDatabase();
    }

    auto &cache = cachedDatabase[(int)engine];
    auto &stack = availableNames[(int)engine];

    for (;;) {
        QString name;
        if (!cache.isEmpty()) {
            name = cache.pop();
            auto db = TKvsDatabase::database(name);
            if (Q_LIKELY(db.isOpen())) {
                tSystemDebug("Gets cached KVS database: %s", qUtf8Printable(db.connectionName()));
                db.moveToThread(QThread::currentThread());  // move to thread
                return db;
            } else {
                tSystemError("Pooled database is not open: %s  [%s:%d]", qUtf8Printable(db.connectionName()), __FILE__, __LINE__);
                stack.push(name);
                continue;
            }
        }

        if (Q_LIKELY(!stack.isEmpty())) {
            name = stack.pop();
            auto db = TKvsDatabase::database(name);
            db.moveToThread(QThread::currentThread());  // move to thread

            if (Q_UNLIKELY(!db.open())) {
                tError("KVS Database open error. Invalid database settings, or maximum number of KVS connection exceeded.");
                tSystemError("KVS database open error: %s", qUtf8Printable(db.connectionName()));
                return TKvsDatabase();
            }

            tSystemDebug("KVS opened successfully  env:%s connectname:%s dbname:%s", qUtf8Printable(Tf::app()->databaseEnvironment()), qUtf8Printable(db.connectionName()), qUtf8Printable(db.databaseName()));
            tSystemDebug("Gets KVS database: %s", qUtf8Printable(db.connectionName()));
            // Executes post-open statements
            if (!db.postOpenStatements().isEmpty()) {
                for (QString st : db.postOpenStatements()) {
                    st = st.trimmed();
                    db.command(st);
                }
            }

            return db;
        }
    }

    throw RuntimeException("No pooled connection", __FILE__, __LINE__);
}


bool TKvsDatabasePool::setDatabaseSettings(TKvsDatabase &database, Tf::KvsEngine engine) const
{
    // Initiates database
    const QVariantMap &settings = Tf::app()->kvsSettings(engine);
    QString databaseName = settings.value("DatabaseName").toString().trimmed();

    if (!databaseName.isEmpty()) {
        tSystemDebug("KVS db name:%s  driver name:%s", qUtf8Printable(databaseName), qUtf8Printable(database.driverName()));
        database.setDatabaseName(databaseName);
    }

    QString hostName = settings.value("HostName").toString().trimmed();
    tSystemDebug("KVS HostName: %s", qUtf8Printable(hostName));
    if (!hostName.isEmpty()) {
        database.setHostName(hostName);
    }

    int port = settings.value("Port").toInt();
    tSystemDebug("KVS Port: %d", port);
    if (port > 0) {
        database.setPort(port);
    }

    QString userName = settings.value("UserName").toString().trimmed();
    tSystemDebug("KVS UserName: %s", qUtf8Printable(userName));
    if (!userName.isEmpty()) {
        database.setUserName(userName);
    }

    QString password = settings.value("Password").toString().trimmed();
    tSystemDebug("KVS Password: %s", qUtf8Printable(password));
    if (!password.isEmpty()) {
        database.setPassword(password);
    }

    QString connectOptions = settings.value("ConnectOptions").toString().trimmed();
    tSystemDebug("KVS ConnectOptions: %s", qUtf8Printable(connectOptions));
    if (!connectOptions.isEmpty()) {
        database.setConnectOptions(connectOptions);
    }

    QStringList postOpenStatements = settings.value("PostOpenStatements").toString().trimmed().split(";", Tf::SkipEmptyParts);
    tSystemDebug("KVS postOpenStatements: %s", qUtf8Printable(postOpenStatements.join(";")));
    if (!postOpenStatements.isEmpty()) {
        database.setPostOpenStatements(postOpenStatements);
    }

    return true;
}


TKvsDatabaseData TKvsDatabasePool::getDatabaseSettings(Tf::KvsEngine engine) const
{
    QMutexLocker locker(&_mutex);

    TKvsDatabaseData settrings;
    auto &stack = availableNames[(int)engine];

    QString name;
    for (;;) {
        if (Q_LIKELY(!stack.isEmpty())) {
            name = stack.pop();
            return TKvsDatabase::settings(name);
            stack.push(name);
        }
    }

    throw RuntimeException("No pooled connection", __FILE__, __LINE__);
}


void TKvsDatabasePool::pool(TKvsDatabase &database)
{
    QMutexLocker locker(&_mutex);

    if (Q_LIKELY(database.isValid())) {
        bool ok;
        int engine = database.connectionName().left(2).toInt(&ok);
        if (Q_UNLIKELY(!ok)) {
            throw RuntimeException("No such KVS engine", __FILE__, __LINE__);
        }

        if (database.isOpen()) {
            cachedDatabase[engine].push(database.connectionName());
            lastCachedTime[engine].store((uint)std::time(nullptr));
            tSystemDebug("Pooled KVS database: %s  count:%lld", qUtf8Printable(database.connectionName()), cachedDatabase->count());
        } else {
            tSystemWarn("Closed KVS database connection, name: %s", qUtf8Printable(database.connectionName()));
            availableNames[engine].push(database.connectionName());
        }
    }
    database = TKvsDatabase();  // Sets an invalid object
}


void TKvsDatabasePool::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == timer.timerId()) {
        QMutexLocker locker(&_mutex);
        QString name;

        // Closes extra-connection
        for (int e = 0; e < kvsEngineHash()->count(); e++) {
            if (!Tf::app()->isKvsAvailable((Tf::KvsEngine)e)) {
                continue;
            }

            auto &cache = cachedDatabase[e];
            while (lastCachedTime[e].load() < (uint)std::time(nullptr) - 30
                && !cache.isEmpty()) {
                name = cache.pop();
                TKvsDatabase::database(name).close();
                tSystemDebug("Closed KVS database connection, name: %s", qUtf8Printable(name));
                availableNames[e].push(name);
            }
        }
    } else {
        QObject::timerEvent(event);
    }
}


QString TKvsDatabasePool::driverName(Tf::KvsEngine engine)
{
    return kvsEngineHash()->value(engine);
}
