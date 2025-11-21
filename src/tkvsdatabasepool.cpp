/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tkvsdatabasepool.h"
#include "tfnamespace.h"
#include "tsystemglobal.h"
#include "tstack.h"
#include <QDateTime>
#include <QMap>
#include <QStringList>
#include <QThread>
#include <QMutexLocker>
#include <TWebApplication>
#include <ctime>
#include <memory>

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
    static std::unique_ptr<TKvsDatabasePool> databasePool = []() {
        std::unique_ptr<TKvsDatabasePool> pool { new TKvsDatabasePool };
        pool->maxConnects = Tf::app()->maxNumberOfThreadsPerAppServer();
        pool->init();
        return pool;
    }();
    return databasePool.get();
}


TKvsDatabasePool::TKvsDatabasePool() :
    QObject()
{
}


TKvsDatabasePool::~TKvsDatabasePool()
{
#if 0
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
#else
    QMutexLocker locker(&_mutex);
    timer.stop();

    for (int eng = 0; eng < (int)Tf::KvsEngine::Num; eng++) {
        if (!Tf::app()->isKvsAvailable((Tf::KvsEngine)eng)) {
            continue;
        }

        auto &cache = cachedDatabases[eng];
        QString name;
        while (!cache.empty()) {
            auto db = cache.pop();
            name = db->connectionName();
            db->close();
            TKvsDatabase::removeDatabase(name);
        }

        auto &stack = availableDatabases[eng];
        while (!stack.empty()) {
            auto db = stack.pop();
            name = db->connectionName();
            TKvsDatabase::removeDatabase(name);
        }
    }

    delete[] lastCachedTime;
#endif
}


void TKvsDatabasePool::init()
{
#if 0
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
            tSystemDebug("KVS database not available. engine:{}", (int)engine);
            continue;
        } else {
            aval = true;
            tSystemDebug("KVS database available. engine:{}", (int)engine);
        }

        auto &stack = availableNames[(int)engine];
        for (int i = 0; i < maxConnects; ++i) {
            TKvsDatabase db = TKvsDatabase::addDatabase(drv, QString::asprintf(CONN_NAME_FORMAT, (int)engine, i));
            if (!db.isValid()) {
                Tf::warn("KVS init parameter is invalid");
                break;
            }

            setDatabaseSettings(db, engine);
            stack.push(db.connectionName());  // push onto stack
            tSystemDebug("Add KVS successfully. name:{}", db.connectionName());
        }
    }

    if (aval) {
        // Starts the timer to close extra-connection
        timer.start(10000, this);
    }
#else
    if (lastCachedTime) {
        return;
    }

    QMutexLocker locker(&_mutex);

    cachedDatabases.resize((int)Tf::KvsEngine::Num);
    availableDatabases.resize((int)Tf::KvsEngine::Num);
    lastCachedTime = new TAtomic<uint>[(int)Tf::KvsEngine::Num];
    bool aval = false;

    // Adds databases previously
    for (auto it = kvsEngineHash()->begin(); it != kvsEngineHash()->end(); ++it) {
        Tf::KvsEngine engine = it.key();
        const QString &drv = it.value();

        if (!Tf::app()->isKvsAvailable(engine)) {
            tSystemDebug("KVS database not available. engine:{}", (int)engine);
            continue;
        } else {
            aval = true;
            tSystemDebug("KVS database available. engine:{}", (int)engine);
        }

        auto &stack = availableDatabases[(int)engine];
        for (int i = 0; i < maxConnects; ++i) {
            QString name = QString::asprintf(CONN_NAME_FORMAT, (int)engine, i);
            TKvsDatabase::addDatabase(drv, name);
            auto db = TKvsDatabase::database(name);
            if (!db->isValid()) {
                Tf::warn("KVS init parameter is invalid");
                break;
            }

            setDatabaseSettings(*db, engine);
            tSystemDebug("Add KVS successfully. name:{}", db->connectionName());
            stack.push(std::move(db));  // push onto stack
        }
    }

    if (aval) {
        // Starts the timer to close extra-connection
        timer.start(10000, this);
    }
#endif
}


TKvsDatabase::Handle TKvsDatabasePool::database(Tf::KvsEngine engine)
{
#if 0
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
                tSystemDebug("Gets cached KVS database: {}", db.connectionName());
                db.moveToThread(QThread::currentThread());  // move to thread
                return db;
            } else {
                tSystemError("Pooled database is not open: {}  [{}:{}]", db.connectionName(), __FILE__, __LINE__);
                stack.push(name);
                continue;
            }
        }

        if (Q_LIKELY(!stack.isEmpty())) {
            name = stack.pop();
            auto db = TKvsDatabase::database(name);
            db.moveToThread(QThread::currentThread());  // move to thread

            if (Q_UNLIKELY(!db.open())) {
                Tf::error("KVS Database open error. Invalid database settings, or maximum number of KVS connection exceeded.");
                tSystemError("KVS database open error: {}", db.connectionName());
                return TKvsDatabase();
            }

            tSystemDebug("KVS opened successfully  env:{} connectname:{} dbname:{}", Tf::app()->databaseEnvironment(), db.connectionName(), db.databaseName());
            tSystemDebug("Gets KVS database: {}", db.connectionName());
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
#else
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
        return TKvsDatabase::Handle();
    }

    auto &cache = cachedDatabases[(int)engine];
    auto &stack = availableDatabases[(int)engine];

    for (;;) {
        QString name;
        if (!cache.empty()) {
            KvsDbPtr dbptr {cache.pop()};
            if (dbptr->isOpen()) {
                tSystemDebug("Gets cached KVS database: {}", dbptr->connectionName());
                dbptr->moveToThread(QThread::currentThread());  // move to thread
                return TKvsDatabase::Handle(std::move(dbptr));
            } else {
                tSystemError("Pooled database is not open: {}  [{}:{}]", dbptr->connectionName(), __FILE__, __LINE__);
                stack.push(std::move(dbptr));
                continue;
            }
        }

        if (!stack.empty()) {
            KvsDbPtr dbptr {stack.pop()};
            dbptr->moveToThread(QThread::currentThread());  // move to thread

            if (!dbptr->open()) {
                Tf::error("KVS Database open error. Invalid database settings, or maximum number of KVS connection exceeded.");
                tSystemError("KVS database open error: {}", dbptr->connectionName());
                return TKvsDatabase::Handle();
            }

            tSystemDebug("KVS opened successfully  env:{} connectname:{} dbname:{}", Tf::app()->databaseEnvironment(), dbptr->connectionName(), dbptr->databaseName());
            tSystemDebug("Gets KVS database: {}", dbptr->connectionName());

            // Executes post-open statements
            if (!dbptr->postOpenStatements().isEmpty()) {
                for (QString st : dbptr->postOpenStatements()) {
                    st = st.trimmed();
                    dbptr->command(st);
                }
            }
            return TKvsDatabase::Handle(std::move(dbptr));
        }
    }

    throw RuntimeException("No pooled connection", __FILE__, __LINE__);
#endif
}


bool TKvsDatabasePool::setDatabaseSettings(TKvsDatabase &database, Tf::KvsEngine engine) const
{
    // Initiates database
    const QVariantMap &settings = Tf::app()->kvsSettings(engine);
    QString databaseName = settings.value("DatabaseName").toString().trimmed();

    if (!databaseName.isEmpty()) {
        tSystemDebug("KVS db name:{}  driver name:{}", databaseName, database.driverName());
        database.setDatabaseName(databaseName);
    }

    QString hostName = settings.value("HostName").toString().trimmed();
    tSystemDebug("KVS HostName: {}", hostName);
    if (!hostName.isEmpty()) {
        database.setHostName(hostName);
    }

    int port = settings.value("Port").toInt();
    tSystemDebug("KVS Port: {}", port);
    if (port > 0) {
        database.setPort(port);
    }

    QString userName = settings.value("UserName").toString().trimmed();
    tSystemDebug("KVS UserName: {}", userName);
    if (!userName.isEmpty()) {
        database.setUserName(userName);
    }

    QString password = settings.value("Password").toString().trimmed();
    tSystemDebug("KVS Password: {}", password);
    if (!password.isEmpty()) {
        database.setPassword(password);
    }

    QString connectOptions = settings.value("ConnectOptions").toString().trimmed();
    tSystemDebug("KVS ConnectOptions: {}", connectOptions);
    if (!connectOptions.isEmpty()) {
        database.setConnectOptions(connectOptions);
    }

    QStringList postOpenStatements = settings.value("PostOpenStatements").toString().trimmed().split(";", Tf::SkipEmptyParts);
    tSystemDebug("KVS postOpenStatements: {}", postOpenStatements.join(";"));
    if (!postOpenStatements.isEmpty()) {
        database.setPostOpenStatements(postOpenStatements);
    }

    return true;
}


TKvsDatabaseSettings TKvsDatabasePool::getDatabaseSettings(Tf::KvsEngine engine) const
{
#if 0
    QMutexLocker locker(&_mutex);

    TKvsDatabaseSettings settrings;
    auto &stack = availableNames[(int)engine];

    QString name;
    for (;;) {
        if (Q_LIKELY(!stack.isEmpty())) {
            name = stack.pop();
            stack.push(name);
            return TKvsDatabase::settings(name);
        }
    }

    throw RuntimeException("No pooled connection", __FILE__, __LINE__);
#else
    QMutexLocker locker(&_mutex);

    TKvsDatabaseSettings settrings;
    auto &stack = availableDatabases[(int)engine];

    if (!stack.empty()) {
        auto &db = stack.top();
        QString name = db->connectionName();
        return TKvsDatabase::settings(name);
    }

    throw RuntimeException("No pooled connection", __FILE__, __LINE__);
#endif
}


void TKvsDatabasePool::pool(KvsDbPtr dbptr)
{
#if 0
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
            tSystemDebug("Pooled KVS database: {}  count:{}", database.connectionName(), (qint64)cachedDatabase->count());
        } else {
            tSystemWarn("Closed KVS database connection, name: {}", database.connectionName());
            availableNames[engine].push(database.connectionName());
        }
    }
    database = TKvsDatabase();  // Sets an invalid object
#else
    QMutexLocker locker(&_mutex);

    if (dbptr->isValid()) {
        bool ok;
        int engine = dbptr->connectionName().left(2).toInt(&ok);
        if (!ok) {
            throw RuntimeException("No such KVS engine", __FILE__, __LINE__);
        }

        if (dbptr->isOpen()) {
            tSystemDebug("Pooled KVS database: {}  count:{}", dbptr->connectionName(), (qint64)cachedDatabases.size());
            cachedDatabases[engine].push(std::move(dbptr));
            lastCachedTime[engine].store((uint)std::time(nullptr));
        } else {
            tSystemWarn("Closed KVS database connection, name: {}", dbptr->connectionName());
            availableDatabases[engine].push(std::move(dbptr));
        }
    }
#endif
}


void TKvsDatabasePool::timerEvent(QTimerEvent *event)
{
#if 0
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
                tSystemDebug("Closed KVS database connection, name: {}", name);
                availableNames[e].push(name);
            }
        }
    } else {
        QObject::timerEvent(event);
    }
#else
    if (event->timerId() == timer.timerId()) {
        QMutexLocker locker(&_mutex);
        QString name;

        // Closes extra-connection
        for (int e = 0; e < kvsEngineHash()->count(); e++) {
            if (!Tf::app()->isKvsAvailable((Tf::KvsEngine)e)) {
                continue;
            }

            auto &cache = cachedDatabases[e];
            while (lastCachedTime[e].load() < (uint)std::time(nullptr) - 30
                && !cache.empty()) {
                auto db = cache.pop();
                db->close();
                tSystemDebug("Closed KVS database connection, name: {}", db->connectionName());
                availableDatabases[e].push(std::move(db));
            }
        }
    } else {
        QObject::timerEvent(event);
    }
#endif
}


QString TKvsDatabasePool::driverName(Tf::KvsEngine engine)
{
    return kvsEngineHash()->value(engine);
}
