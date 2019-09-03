/* Copyright (c) 2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tcachesqlitestore.h"
#include "tsystemglobal.h"
#include "tsqlquery.h"
#include <QByteArray>
#include <QDateTime>
#include <QReadWriteLock>
#include <atomic>

constexpr auto TABLE_NAME = "kb";
constexpr auto KEY_COLUMN  = "k";
constexpr auto BLOB_COLUMN = "b";
constexpr auto TIMESTAMP_COLUMN = "t";
constexpr int  PAGESIZE = 4096;

static std::atomic_ullong suffix {0};


static bool exec(const QSqlDatabase &db, const QString &sql)
{
    if (! db.isOpen()) {
        return false;
    }

    TSqlQuery query(db);
    bool ret = query.exec(sql);
    if (! ret) {
        tSystemError("SQLite error : %s, query:'%s' [%s:%d]", qPrintable(db.lastError().text()), qPrintable(sql), __FILE__, __LINE__);
    }
    return ret;
}


TCacheSQLiteStore::TCacheSQLiteStore(const QString &fileName, const QString &connectOptions, qint64 thresholdFileSize) :
    _dbFile(fileName),
    _connectName(QLatin1String("TCacheSQLiteStore") + QString::number(suffix.fetch_add(1))),
    _connectOptions(connectOptions),
    _thresholdFileSize(thresholdFileSize)
{}


TCacheSQLiteStore::~TCacheSQLiteStore()
{
    close();
    QSqlDatabase::removeDatabase(_connectName);
}


bool TCacheSQLiteStore::open()
{
    static bool created = [&]() {
        // Creates a table
        auto db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(_dbFile);
        if (! _connectOptions.isEmpty()) {
            db.setConnectOptions(_connectOptions);
        }

        bool ok = db.open();
        if (ok) {
            exec(db, QStringLiteral("pragma page_size=%1").arg(PAGESIZE));
            exec(db, QStringLiteral("begin"));
            exec(db, QStringLiteral("create table if not exists %1 (%2 text primary key, %3 integer, %4 blob)").arg(TABLE_NAME, KEY_COLUMN, TIMESTAMP_COLUMN, BLOB_COLUMN));
            exec(db, QStringLiteral("commit"));
            db.close();
        }
        return ok;
    }();
    Q_UNUSED(created);

    if (isOpen()) {
        return true;
    }

    if (! _db.isValid()) {
        _db = QSqlDatabase::addDatabase("QSQLITE", _connectName);
        _db.setDatabaseName(_dbFile);
        if (! _connectOptions.isEmpty()) {
            _db.setConnectOptions(_connectOptions);
        }
    }

    bool ok = _db.open();
    if  (ok) {
        exec(_db, QStringLiteral("pragma journal_mode=WAL"));
        exec(_db, QStringLiteral("pragma foreign_keys=ON"));
        exec(_db, QStringLiteral("pragma synchronous=NORMAL"));
        exec(_db, QStringLiteral("pragma busy_timeout=5000"));
    } else {
        tSystemError("SQLite open failed : %s", qPrintable(_dbFile));
    }
    return ok;
}


void TCacheSQLiteStore::close()
{
    _db.close();
}


int TCacheSQLiteStore::count()
{
    int cnt = -1;

    if (! isOpen()) {
        return cnt;
    }

    TSqlQuery query(_db);
    QString sql = QStringLiteral("select count(1) from %1").arg(TABLE_NAME);

    if (query.exec(sql) && query.next()) {
        cnt = query.value(0).toInt();
    }
    return cnt;
}


bool TCacheSQLiteStore::exists(const QByteArray &key)
{
    if (! isOpen()) {
        return false;
    }

    TSqlQuery query(_db);
    int exist = 0;
    QString sql = QStringLiteral("select exists(select 1 from %1 where %2=:name limit 1)").arg(TABLE_NAME).arg(KEY_COLUMN);

    query.prepare(sql);
    query.bind(":name", key);
    if (query.exec() && query.next()) {
        exist = query.value(0).toInt();
    }
    return (exist > 0);
}


QByteArray TCacheSQLiteStore::get(const QByteArray &key)
{
    QByteArray value;
    qint64 expire = 0;
    qint64 current = QDateTime::currentMSecsSinceEpoch();

    if (read(key, value, expire)) {
        if (expire < current) {
            value.clear();
            remove(key);
        }
    }
    return value;
}


bool TCacheSQLiteStore::set(const QByteArray &key, const QByteArray &value, qint64 msecs)
{
    if (key.isEmpty() || msecs <= 0) {
        return false;
    }

    remove(key);
    qint64 expire = QDateTime::currentMSecsSinceEpoch() + msecs;
    return write(key, value, expire);
}


bool TCacheSQLiteStore::read(const QByteArray &key, QByteArray &blob, qint64 &timestamp)
{
    bool ret = false;

    if (key.isEmpty()) {
        return ret;
    }

    if (! isOpen()) {
        return ret;
    }

    TSqlQuery query(_db);
    query.prepare(QStringLiteral("select %1,%2 from %3 where %4=:key").arg(TIMESTAMP_COLUMN, BLOB_COLUMN, TABLE_NAME, KEY_COLUMN));
    query.bind(":key", key);
    ret = query.exec();
    if (ret) {
        if (query.next()) {
            timestamp = query.value(0).toLongLong();
            blob = query.value(1).toByteArray();
        }
    } else {
        tSystemError("SQLite error : %s [%s:%d]", qPrintable(_db.lastError().text()), __FILE__, __LINE__);
    }
    return ret;
}


bool TCacheSQLiteStore::write(const QByteArray &key, const QByteArray &blob, qint64 timestamp)
{
    bool ret = false;

    if (key.isEmpty()) {
        return ret;
    }

    if (! isOpen()) {
        return ret;
    }

    TSqlQuery query(_db);
    QString sql = QStringLiteral("insert into %1 (%2,%3,%4) values (:key,:ts,:blob)").arg(TABLE_NAME, KEY_COLUMN, TIMESTAMP_COLUMN, BLOB_COLUMN);

    query.prepare(sql);
    query.bind(":key", key).bind(":ts", timestamp).bind(":blob", blob);
    ret = query.exec();
    if (!ret && _db.lastError().isValid()) {
        tSystemError("SQLite error : %s [%s:%d]", qPrintable(_db.lastError().text()), __FILE__, __LINE__);
    }
    return ret;
}


bool TCacheSQLiteStore::remove(const QByteArray &key)
{
    bool ret = false;

    if (key.isEmpty()) {
        return ret;
    }

    if (! isOpen()) {
        return ret;
    }

    TSqlQuery query(_db);
    QString sql = QStringLiteral("delete from %1 where %2=:key").arg(TABLE_NAME, KEY_COLUMN);

    query.prepare(sql);
    query.bind(":key", key);
    ret = query.exec();
    if (! ret) {
        tSystemError("SQLite error : %s [%s:%d]", qPrintable(_db.lastError().text()), __FILE__, __LINE__);
    }
    return ret;
}


void TCacheSQLiteStore::clear()
{
    removeAll();
    vacuum();
}


int TCacheSQLiteStore::removeOlder(int num)
{
    int cnt = -1;

    if (num < 1) {
        return cnt;
    }

    if (! isOpen()) {
        return cnt;
    }

    TSqlQuery query(_db);
    QString sql = QStringLiteral("delete from %1 where ROWID in (select ROWID from %1 order by t asc limit :num)").arg(TABLE_NAME);

    query.prepare(sql);
    query.bind(":num", num);
    if (query.exec()) {
        cnt = query.numRowsAffected();
    } else {
        tSystemError("SQLite error : %s [%s:%d]", qPrintable(_db.lastError().text()), __FILE__, __LINE__);
    }
    return cnt;
}


int TCacheSQLiteStore::removeOlderThan(qint64 timestamp)
{
    int cnt = -1;

    if (! isOpen()) {
        return cnt;
    }

    TSqlQuery query(_db);
    QString sql = QStringLiteral("delete from %1 where %2<:ts").arg(TABLE_NAME, TIMESTAMP_COLUMN);

    query.prepare(sql);
    query.bind(":ts", timestamp);
    if (query.exec()) {
        cnt = query.numRowsAffected();
    } else {
        tSystemError("SQLite error : %s [%s:%d]", qPrintable(_db.lastError().text()), __FILE__, __LINE__);
    }
    return cnt;
}


int TCacheSQLiteStore::removeAll()
{
    int cnt = -1;

    if (! isOpen()) {
        return cnt;
    }

    TSqlQuery query(_db);
    QString sql = QStringLiteral("delete from %1").arg(TABLE_NAME);

    if (query.exec(sql)) {
        cnt = query.numRowsAffected();
    } else {
        tSystemError("SQLite error : %s [%s:%d]", qPrintable(_db.lastError().text()), __FILE__, __LINE__);
    }
    return cnt;
}


bool TCacheSQLiteStore::vacuum()
{
    bool ret = false;

    if (! isOpen()) {
        return ret;
    }

    ret = exec(_db, QStringLiteral("vacuum"));
    return ret;
}


qint64 TCacheSQLiteStore::dbSize()
{
    qint64 sz = -1;

    if (! isOpen()) {
        return sz;
    }

    TSqlQuery query(_db);
    bool ok = query.exec(QStringLiteral("select (page_count * page_size) from pragma_page_count(), pragma_page_size()"));
    if (ok && query.next()) {
        sz = query.value(0).toLongLong();
    }
    return sz;
}


void TCacheSQLiteStore::gc()
{
    int removed = removeOlderThan(QDateTime::currentMSecsSinceEpoch());
    tSystemDebug("removeOlderThan: %d\n", removed);
    vacuum();

    if (_thresholdFileSize > 0 && dbSize() > _thresholdFileSize) {
        for (int i = 0; i < 3; i++) {
            removed += removeOlder(count() * 0.3);
            vacuum();
            if (dbSize() < _thresholdFileSize * 0.8) {
                break;
            }
        }
        tSystemDebug("removeOlder: %d\n", removed);
    }
}
