/* Copyright (c) 2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tcachesqlitestore.h"
#include "tsqlquery.h"
#include "tsystemglobal.h"
#include "tsqldatabasepool.h"
#include <QByteArray>
#include <QDateTime>
#include <TfCore>
#include <mutex>

constexpr auto TABLE_NAME = "kb";
constexpr auto KEY_COLUMN = "k";
constexpr auto BLOB_COLUMN = "b";
constexpr auto TIMESTAMP_COLUMN = "t";

static int sqliteMajorVersion;
static int sqliteMinorVersion;


inline QSqlError lastError()
{
    return Tf::currentSqlDatabase(Tf::app()->databaseIdForCache()).lastError();
}


inline QString lastErrorString()
{
    return lastError().text();
}


static void getVersion()
{
    TSqlQuery query(Tf::app()->databaseIdForCache());

    if (query.exec(QStringLiteral("select sqlite_version()")) && query.next()) {
        auto sqliteVersion = query.value(0).toString().split(".");
        if (sqliteVersion.count() > 1) {
            sqliteMajorVersion = sqliteVersion[0].toInt();
            sqliteMinorVersion = sqliteVersion[1].toInt();
        }
    }
}

// Query without a transaction
static bool queryNonTrx(const QSqlDatabase &db, const QString &sql)
{
    TSqlQuery qry(db);
    bool ret = qry.exec(sql);
    if (!ret && !lastErrorString().isEmpty()) {
        tSystemError("SQLite error : {}, query:'{}' [{}:{}]", lastErrorString(), sql, __FILE__, __LINE__);
    }
    return ret;
}


bool TCacheSQLiteStore::createTable(const QString &table)
{
    int id = Tf::app()->databaseIdForCache();
    auto db = TSqlDatabasePool::instance()->database(id);
    bool ret = queryNonTrx(db, QStringLiteral("CREATE TABLE IF NOT EXISTS %1 (%2 TEXT PRIMARY KEY, %3 INTEGER, %4 BLOB)").arg(table, KEY_COLUMN, TIMESTAMP_COLUMN, BLOB_COLUMN));
    queryNonTrx(db, QStringLiteral("VACUUM"));

    TSqlDatabasePool::instance()->pool(db);
    return ret;
}


TCacheSQLiteStore::TCacheSQLiteStore(const QByteArray &table) :
    _table(table.isEmpty() ? QString(TABLE_NAME) : QString(table))
{
}


TCacheSQLiteStore::~TCacheSQLiteStore()
{
    close();
}


void TCacheSQLiteStore::init()
{
    createTable(TABLE_NAME);
}


bool TCacheSQLiteStore::open()
{
    static std::once_flag once;

    std::call_once(once, []() {
        getVersion();
    });
    return true;
}


void TCacheSQLiteStore::close()
{
}


bool TCacheSQLiteStore::isOpen() const
{
    return true;
}


int TCacheSQLiteStore::count()
{
    int cnt = -1;

    TSqlQuery query(Tf::app()->databaseIdForCache());
    QString sql = QStringLiteral("select count(1) from %1").arg(_table);

    if (query.exec(sql) && query.next()) {
        cnt = query.value(0).toInt();
    }
    return cnt;
}


bool TCacheSQLiteStore::exists(const QByteArray &key)
{
    int exist = 0;
    TSqlQuery query(Tf::app()->databaseIdForCache());
    QString sql = QStringLiteral("select exists(select 1 from %1 where %2=:name and %3>:ts limit 1)").arg(_table).arg(KEY_COLUMN).arg(TIMESTAMP_COLUMN);
    auto current = QDateTime::currentMSecsSinceEpoch() / 1000;

    query.prepare(sql);
    query.bind(":name", key);
    query.bind(":ts", current);
    if (query.exec() && query.next()) {
        exist = query.value(0).toInt();
    }
    return (exist > 0);
}


QByteArray TCacheSQLiteStore::get(const QByteArray &key)
{
    QByteArray value;
    int64_t expire = 0;
    int64_t current = QDateTime::currentMSecsSinceEpoch() / 1000;

    if (read(key, value, expire)) {
        if (expire <= current) {
            value.clear();
            remove(key);
        }
    }
    return value;
}


bool TCacheSQLiteStore::set(const QByteArray &key, const QByteArray &value, int seconds)
{
    if (key.isEmpty() || seconds <= 0) {
        return false;
    }

    int64_t expire = QDateTime::currentMSecsSinceEpoch() / 1000 + seconds;
    return write(key, value, expire);
}


bool TCacheSQLiteStore::read(const QByteArray &key, QByteArray &blob, int64_t &timestamp)
{
    bool ret = false;

    if (key.isEmpty()) {
        return ret;
    }

    TSqlQuery query(Tf::app()->databaseIdForCache());
    query.prepare(QStringLiteral("select %1,%2 from %3 where %4=:key").arg(TIMESTAMP_COLUMN, BLOB_COLUMN, _table, KEY_COLUMN));
    query.bind(":key", key);
    ret = query.exec();
    if (ret) {
        if (query.next()) {
            timestamp = query.value(0).toLongLong();
            blob = query.value(1).toByteArray();
        }
    } else {
        auto error = lastErrorString();
        if (!error.isEmpty()) {
            tSystemError("SQLite error : {} [{}:{}]", error, __FILE__, __LINE__);
        }
    }
    return ret;
}


bool TCacheSQLiteStore::write(const QByteArray &key, const QByteArray &blob, int64_t timestamp)
{
    bool ret = false;

    if (key.isEmpty()) {
        return ret;
    }

    QString sql;
    if (sqliteMajorVersion >= 3 && sqliteMinorVersion >= 24) {
        // upsert-clause
        sql = QStringLiteral("insert into %1 (%2,%3,%4) values (:key,:ts,:blob) on conflict(k) do update set b=:blob, t=:ts").arg(_table, KEY_COLUMN, TIMESTAMP_COLUMN, BLOB_COLUMN);
    } else {
        sql = QStringLiteral("replace into %1 (%2,%3,%4) values (:key,:ts,:blob)").arg(_table, KEY_COLUMN, TIMESTAMP_COLUMN, BLOB_COLUMN);
    }

    TSqlQuery query(Tf::app()->databaseIdForCache());
    query.prepare(sql);
    query.bind(":key", key).bind(":ts", (qint64)timestamp).bind(":blob", blob);
    ret = query.exec();
    if (!ret && lastError().isValid()) {
        tSystemError("SQLite error : {} [{}:{}]", lastErrorString(), __FILE__, __LINE__);
    }
    return ret;
}


bool TCacheSQLiteStore::remove(const QByteArray &key)
{
    bool ret = false;

    if (key.isEmpty()) {
        return ret;
    }

    TSqlQuery query(Tf::app()->databaseIdForCache());
    QString sql = QStringLiteral("delete from %1 where %2=:key").arg(_table, KEY_COLUMN);

    query.prepare(sql);
    query.bind(":key", key);
    ret = query.exec();
    if (!ret && lastError().isValid()) {
        tSystemError("SQLite error : {} [{}:{}]", lastErrorString(), __FILE__, __LINE__);
    }
    return ret;
}


void TCacheSQLiteStore::clear()
{
    removeAll();
}


int TCacheSQLiteStore::removeOlder(int num)
{
    int cnt = -1;

    if (num < 1) {
        return cnt;
    }

    TSqlQuery query(Tf::app()->databaseIdForCache());
    QString sql = QStringLiteral("delete from %1 where ROWID in (select ROWID from %1 order by t asc limit :num)").arg(_table);

    query.prepare(sql);
    query.bind(":num", num);
    if (query.exec()) {
        cnt = query.numRowsAffected();
    } else {
        tSystemError("SQLite error : {} [{}:{}]", lastErrorString(), __FILE__, __LINE__);
    }
    return cnt;
}


int TCacheSQLiteStore::removeOlderThan(int64_t timestamp)
{
    int cnt = -1;

    TSqlQuery query(Tf::app()->databaseIdForCache());
    QString sql = QStringLiteral("delete from %1 where %2<:ts").arg(_table, TIMESTAMP_COLUMN);

    query.prepare(sql);
    query.bind(":ts", (qint64)timestamp);
    if (query.exec()) {
        cnt = query.numRowsAffected();
    } else {
        if (lastError().isValid()) {
            tSystemError("SQLite error : {} [{}:{}]", lastErrorString(), __FILE__, __LINE__);
        }
    }
    return cnt;
}


int TCacheSQLiteStore::removeAll()
{
    int cnt = -1;

    TSqlQuery query(Tf::app()->databaseIdForCache());
    QString sql = QStringLiteral("delete from %1").arg(_table);

    if (query.exec(sql)) {
        cnt = query.numRowsAffected();
    } else {
        tSystemError("SQLite error : {} [{}:{}]", lastErrorString(), __FILE__, __LINE__);
    }
    return cnt;
}


int64_t TCacheSQLiteStore::dbSize()
{
    int64_t sz = -1;

    TSqlQuery query(Tf::app()->databaseIdForCache());
    bool ok = query.exec(QStringLiteral("PRAGMA page_size"));
    if (ok && query.next()) {
        int64_t size = query.value(0).toLongLong();

        ok = query.exec(QStringLiteral("PRAGMA page_count"));
        if (ok && query.next()) {
            int64_t count = query.value(0).toLongLong();
            sz = size * count;
        }
    }
    return sz;
}


void TCacheSQLiteStore::gc()
{
    int removed = removeOlderThan(1 + QDateTime::currentMSecsSinceEpoch() / 1000);
    tSystemDebug("removeOlderThan: {}\n", removed);
}


QMap<QString, QVariant> TCacheSQLiteStore::defaultSettings() const
{
    QMap<QString, QVariant> settings {
        {"DriverType", "QSQLITE"},
        {"DatabaseName", "cachedb"},
        {"PostOpenStatements", "PRAGMA journal_mode=OFF; PRAGMA busy_timeout=5; PRAGMA synchronous=OFF;"},
    };
    return settings;
}
