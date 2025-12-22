/* Copyright (c) 2012-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tkvsdatabase.h"
#include "tkvsdatabasepool.h"
#include "tmongodriver.h"
#include "tredisdriver.h"
#include "tmemcacheddriver.h"
#include "tsharedmemorykvsdriver.h"
#include <QMap>
#include <QReadWriteLock>
#include <QString>
#include <TKvsDriver>
#include <TSystemGlobal>

/*!
  \class TKvsDatabase
  \brief The TKvsDatabase class represents a connection to a key-value
  store database.
*/

const char *const TKvsDatabase::defaultConnection = "tf_default_connection";


// Map of connection name and database data
class TKvsDatabaseDict : public QMap<QString, TKvsDatabaseSettings> {
public:
    mutable QReadWriteLock lock;
};


static TKvsDatabaseDict *databaseDict()
{
    static std::unique_ptr<TKvsDatabaseDict> dict{new TKvsDatabaseDict{}};
    return dict.get();
}


static TKvsDriver *createDriver(const QString &driverName)
{
    TKvsDriver *driver = nullptr;
    QString name = driverName.toLower();

    if (name.isEmpty()) {
        return driver;
    }

    if (name == QLatin1String("mongodb")) {
        driver = new TMongoDriver();
    } else if (name == QLatin1String("redis")) {
        driver = new TRedisDriver();
    } else if (name == QLatin1String("memcached")) {
        driver = new TMemcachedDriver();
    } else if (name == QLatin1String("memory")) {
        driver = new TSharedMemoryKvsDriver();
    } else {
        Tf::warn("TKvsDatabase: {} driver not found", driverName);
        return driver;
    }

    if (!driver) {
        Tf::warn("TKvsDatabase: {} driver not loaded", driverName);
    }
    return driver;
}


std::unique_ptr<TKvsDatabase> TKvsDatabase::database(const QString &connectionName)
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);

    if (!dict->contains(connectionName)) {
        tSystemWarn("No such KVS database: {}", connectionName);
        return std::unique_ptr<TKvsDatabase>{new TKvsDatabase{}};
    }

    const TKvsDatabaseSettings &d = (*dict)[connectionName];
    return std::unique_ptr<TKvsDatabase>{new TKvsDatabase(d.connectionName, d.driver)};
}


bool TKvsDatabase::addDatabase(const QString &driver, const QString &connectionName)
{
    auto *dict = databaseDict();
    QWriteLocker locker(&dict->lock);

    // Removes it if exists
    if (dict->contains(connectionName)) {
        auto data = dict->take(connectionName);
        delete data.driver;
    }

    TKvsDatabaseSettings data;
    data.connectionName = connectionName;
    data.driver = createDriver(driver);  // creates a driver
    dict->insert(connectionName, data);
    return true;
}


void TKvsDatabase::removeDatabase(const QString &connectionName)
{
    auto *dict = databaseDict();
    QWriteLocker locker(&dict->lock);

    TKvsDatabase db(dict->take(connectionName));

    db.close();
    delete db._driver;
}


TKvsDatabaseSettings TKvsDatabase::settings(const QString &connectionName)
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);
    return (*dict)[connectionName];
}


TKvsDatabase::TKvsDatabase(const QString &connectionName, TKvsDriver *driver) :
    _connectName(connectionName),
    _driver(driver)
{
}


TKvsDatabase::TKvsDatabase(const TKvsDatabaseSettings &data) :
    _connectName(data.connectionName),
    _driver(data.driver)
{
}


bool TKvsDatabase::isValid() const
{
    return (bool)driver();
}


bool TKvsDatabase::open()
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);
    const TKvsDatabaseSettings &data = (*dict)[_connectName];
    return (driver()) ? driver()->open(data.databaseName, data.userName, data.password, data.hostName, data.port, data.connectOptions) : false;
}


void TKvsDatabase::close()
{
    if (driver()) {
        driver()->close();
    }
}


bool TKvsDatabase::command(const QString &cmd)
{
    return (driver()) ? driver()->command(cmd.toUtf8()) : false;
}


bool TKvsDatabase::isOpen() const
{
    return (driver()) ? driver()->isOpen() : false;
}


QString TKvsDatabase::driverName() const
{
    return (driver()) ? driver()->key() : QString();
}


QString TKvsDatabase::databaseName() const
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);
    return (*dict)[_connectName].databaseName;
}


void TKvsDatabase::setDatabaseName(const QString &name)
{
    if (!_connectName.isEmpty()) {
        auto *dict = databaseDict();
        QWriteLocker locker(&dict->lock);
        (*dict)[_connectName].databaseName = name;
    }
}


QString TKvsDatabase::hostName() const
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);
    return (*dict)[_connectName].hostName;
}


void TKvsDatabase::setHostName(const QString &hostName)
{
    if (!_connectName.isEmpty()) {
        auto *dict = databaseDict();
        QWriteLocker locker(&dict->lock);
        (*dict)[_connectName].hostName = hostName;
    }
}


int TKvsDatabase::port() const
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);
    return (*dict)[_connectName].port;
}


void TKvsDatabase::setPort(int port)
{
    if (!_connectName.isEmpty()) {
        auto *dict = databaseDict();
        QWriteLocker locker(&dict->lock);
        (*dict)[_connectName].port = port;
    }
}


QString TKvsDatabase::userName() const
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);
    return (*dict)[_connectName].userName;
}


void TKvsDatabase::setUserName(const QString &userName)
{
    if (!_connectName.isEmpty()) {
        auto *dict = databaseDict();
        QWriteLocker locker(&dict->lock);
        (*dict)[_connectName].userName = userName;
    }
}


QString TKvsDatabase::password() const
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);
    return (*dict)[_connectName].password;
}


void TKvsDatabase::setPassword(const QString &password)
{
    if (!_connectName.isEmpty()) {
        auto *dict = databaseDict();
        QWriteLocker locker(&dict->lock);
        (*dict)[_connectName].password = password;
    }
}


QString TKvsDatabase::connectOptions() const
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);
    return (*dict)[_connectName].connectOptions;
}


void TKvsDatabase::setConnectOptions(const QString &options)
{
    if (!_connectName.isEmpty()) {
        auto *dict = databaseDict();
        QWriteLocker locker(&dict->lock);
        (*dict)[_connectName].connectOptions = options;
    }
}


QStringList TKvsDatabase::postOpenStatements() const
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);
    return (*dict)[_connectName].postOpenStatements;
}


void TKvsDatabase::setPostOpenStatements(const QStringList &statements)
{
    if (!_connectName.isEmpty()) {
        auto *dict = databaseDict();
        QWriteLocker locker(&dict->lock);
        (*dict)[_connectName].postOpenStatements = statements;
    }
}


void TKvsDatabase::moveToThread(QThread *targetThread)
{
    if (driver()) {
        driver()->moveToThread(targetThread);
    }
}


TKvsDatabase &TKvsDatabase::operator=(TKvsDatabase &&other)
{
    if (this != &other) {
        _connectName = std::forward<QString>(other._connectName);
        _driver = other._driver;
        other._driver = nullptr;
    }
    return *this;
}


TKvsDatabase::Handle::~Handle()
{
    if (_dbptr) {
        TKvsDatabasePool::instance()->pool(std::move(_dbptr));
    }
}


TKvsDatabase::Handle &TKvsDatabase::Handle::operator=(TKvsDatabase::Handle &&other)
{
    if (this != &other) {
        if (_dbptr) {
            TKvsDatabasePool::instance()->pool(std::move(_dbptr));
        }
        _dbptr = std::move(other._dbptr);
    }
    return *this;
}
