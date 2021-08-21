/* Copyright (c) 2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tcachesqlitestore.h"
#include "tsqlquery.h"
#include "tsystemglobal.h"
#include <QByteArray>
#include <QDateTime>
#include <TfCore>
#include <TDatabaseContext>
#include <mutex>

constexpr auto TABLE_NAME = "kb";
constexpr auto KEY_COLUMN = "k";
constexpr auto BLOB_COLUMN = "b";
constexpr auto TIMESTAMP_COLUMN = "t";
constexpr int PAGESIZE = 4096;


inline QSqlError lastError()
{
    return Tf::currentSqlDatabase(Tf::app()->databaseIdForCache()).lastError();
}


inline QString lastErrorString()
{
    return lastError().text();
}


static bool query(const QString &sql)
{
    TSqlQuery qry(Tf::app()->databaseIdForCache());
    bool ret = qry.exec(sql);
    if (!ret) {
        tSystemError("SQLite error : %s, query:'%s' [%s:%d]", qUtf8Printable(lastErrorString()), qUtf8Printable(sql), __FILE__, __LINE__);
    }
    return ret;
}


bool TCacheSQLiteStore::createTable(const QString &table)
{
    query(QStringLiteral("PRAGMA page_size=%1").arg(PAGESIZE));
    bool ret = query(QStringLiteral("CREATE TABLE IF NOT EXISTS %1 (%2 TEXT PRIMARY KEY, %3 INTEGER, %4 BLOB)").arg(table, KEY_COLUMN, TIMESTAMP_COLUMN, BLOB_COLUMN));
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


bool TCacheSQLiteStore::open()
{
    static std::once_flag once;
    std::call_once(once, []() { createTable(TABLE_NAME); });
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
    qint64 current = QDateTime::currentMSecsSinceEpoch() / 1000;

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
    qint64 expire = 0;
    qint64 current = QDateTime::currentMSecsSinceEpoch() / 1000;

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

    remove(key);
    qint64 expire = QDateTime::currentMSecsSinceEpoch() / 1000 + seconds;
    return write(key, value, expire);
}


bool TCacheSQLiteStore::read(const QByteArray &key, QByteArray &blob, qint64 &timestamp)
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
        tSystemError("SQLite error : %s [%s:%d]", qUtf8Printable(lastErrorString()), __FILE__, __LINE__);
    }
    return ret;
}


bool TCacheSQLiteStore::write(const QByteArray &key, const QByteArray &blob, qint64 timestamp)
{
    bool ret = false;

    if (key.isEmpty()) {
        return ret;
    }

    TSqlQuery query(Tf::app()->databaseIdForCache());
    QString sql = QStringLiteral("insert into %1 (%2,%3,%4) values (:key,:ts,:blob)").arg(_table, KEY_COLUMN, TIMESTAMP_COLUMN, BLOB_COLUMN);

    query.prepare(sql);
    query.bind(":key", key).bind(":ts", timestamp).bind(":blob", blob);
    ret = query.exec();
    if (!ret && lastError().isValid()) {
        tSystemError("SQLite error : %s [%s:%d]", qUtf8Printable(lastErrorString()), __FILE__, __LINE__);
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
        tSystemError("SQLite error : %s [%s:%d]", qUtf8Printable(lastErrorString()), __FILE__, __LINE__);
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
        tSystemError("SQLite error : %s [%s:%d]", qUtf8Printable(lastErrorString()), __FILE__, __LINE__);
    }
    return cnt;
}


int TCacheSQLiteStore::removeOlderThan(qint64 timestamp)
{
    int cnt = -1;

    TSqlQuery query(Tf::app()->databaseIdForCache());
    QString sql = QStringLiteral("delete from %1 where %2<:ts").arg(_table, TIMESTAMP_COLUMN);

    query.prepare(sql);
    query.bind(":ts", timestamp);
    if (query.exec()) {
        cnt = query.numRowsAffected();
    } else {
        if (lastError().isValid()) {
            tSystemError("SQLite error : %s [%s:%d]", qUtf8Printable(lastErrorString()), __FILE__, __LINE__);
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
        tSystemError("SQLite error : %s [%s:%d]", qUtf8Printable(lastErrorString()), __FILE__, __LINE__);
    }
    return cnt;
}


qint64 TCacheSQLiteStore::dbSize()
{
    qint64 sz = -1;

    TSqlQuery query(Tf::app()->databaseIdForCache());
    bool ok = query.exec(QStringLiteral("PRAGMA page_size"));
    if (ok && query.next()) {
        qint64 size = query.value(0).toLongLong();

        ok = query.exec(QStringLiteral("PRAGMA page_count"));
        if (ok && query.next()) {
            qint64 count = query.value(0).toLongLong();
            sz = size * count;
        }
    }
    return sz;
}


void TCacheSQLiteStore::gc()
{
    int removed = removeOlderThan(1 + QDateTime::currentMSecsSinceEpoch() / 1000);
    tSystemDebug("removeOlderThan: %d\n", removed);
}


QMap<QString, QVariant> TCacheSQLiteStore::defaultSettings() const
{
    QMap<QString, QVariant> settings {
        {"DriverType", "QSQLITE"},
        {"DatabaseName", "cachedb"},
        {"PostOpenStatements", "PRAGMA journal_mode=WAL; PRAGMA busy_timeout=5000; PRAGMA synchronous=NORMAL;"},
    };
    return settings;
}
