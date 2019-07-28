/* Copyright (c) 2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsqliteblobstore.h"
#include "tsystemglobal.h"
#include "tsqlquery.h"
#include <QByteArray>
#include <QDateTime>
#include <QFileInfo>

constexpr auto TABLE_NAME = "kb";
constexpr auto KEY_COLUMN  = "k";
constexpr auto BLOB_COLUMN = "b";
constexpr auto TIMESTAMP_COLUMN = "t";
constexpr int  PAGESIZE = 4096;


bool TSQLiteBlobStore::setup(const QByteArray &fileName)
{
    TSQLiteBlobStore sqlite(fileName);

    if (! sqlite.open()) {
        return false;
    }

    sqlite.exec(QStringLiteral("pragma page_size=%1").arg(PAGESIZE));
    sqlite.exec(QStringLiteral("vacuum"));
    sqlite.exec(QStringLiteral("begin"));
    sqlite.exec(QStringLiteral("create table if not exists %1 (%2 text primary key, %3 integer, %4 blob)").arg(TABLE_NAME, KEY_COLUMN, TIMESTAMP_COLUMN, BLOB_COLUMN));
    //sqlite.exec(QStringLiteral("create table if not exists %1 (%2 text primary key, %4 blob, %3 integer)").arg(TABLE_NAME, KEY_COLUMN, TIMESTAMP_COLUMN, BLOB_COLUMN));
    return sqlite.exec(QStringLiteral("commit"));
}


TSQLiteBlobStore::TSQLiteBlobStore(const QString &fileName) :
    _dbFile(fileName),
    _connectionName(QString::number(QDateTime::currentMSecsSinceEpoch()))
{}


bool TSQLiteBlobStore::open()
{
    if (isOpen()) {
        return true;
    }

    _db = QSqlDatabase::addDatabase("QSQLITE", _connectionName);
    _db.setDatabaseName(_dbFile);

    bool ok = _db.open();
    if  (ok) {
        exec(QStringLiteral("pragma journal_mode=WAL"));
        exec(QStringLiteral("pragma foreign_keys=ON"));
        exec(QStringLiteral("pragma synchronous=NORMAL"));
        exec(QStringLiteral("pragma busy_timeout=5000"));
    } else {
        printf("failed !!!!!!!!!!!!!\n");
    }
    return ok;
}


void TSQLiteBlobStore::close()
{
    if (isOpen()) {
        _db.close();
    }
}


int TSQLiteBlobStore::count() const
{
    int cnt = 0;

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


bool TSQLiteBlobStore::exists(const QByteArray &name) const
{
    if (! isOpen()) {
        return false;
    }

    TSqlQuery query(_db);
    int exist = 0;
    QString sql = QStringLiteral("select exists(select 1 from %1 where %2=:val limit 1)").arg(TABLE_NAME).arg(KEY_COLUMN);

    query.prepare(sql);
    query.bind(":val", name);
    if (query.exec() && query.next()) {
        exist = query.value(0).toInt();
    }
    return (exist > 0);
}


bool TSQLiteBlobStore::read(const QByteArray &name, QByteArray &blob, qint64 &timestamp)
{
    bool ret = false;

    if (! isOpen() || name.isEmpty()) {
        return ret;
    }

    TSqlQuery query(_db);

//    query.prepare(QStringLiteral("select %1,%2 from %3 where %4=:name").arg(TIMESTAMP_COLUMN, BLOB_COLUMN, TABLE_NAME, KEY_COLUMN));
    query.prepare(QStringLiteral("select t,b from kb where k=:name"));
    query.bind(":name", name);
    ret = query.exec();
    if (ret) {
        if (query.next()) {
            timestamp = query.value(0).toLongLong();
            blob = query.value(1).toByteArray();
        } else {
            timestamp = 0;
            blob.resize(0);
        }
    } else {
        printf("read error: %s\n", qPrintable(_db.lastError().text()));
    }
    return ret;
}


bool TSQLiteBlobStore::write(const QByteArray &name, const QByteArray &blob, qint64 timestamp)
{
    bool ret = false;

    if (! isOpen() || name.isEmpty()) {
        printf("##0 error:: %s\n", qPrintable(name));
        return ret;
    }

    TSqlQuery query(_db);
    QString sql = QStringLiteral("insert into %1 (%2,%3,%4) values (:name,:ts,:blob)").arg(TABLE_NAME, KEY_COLUMN, TIMESTAMP_COLUMN, BLOB_COLUMN);

    query.prepare(sql);
    query.bind(":name", name).bind(":ts", timestamp).bind(":blob", blob);
    ret = query.exec();
    if (! ret) {
        printf("##2 error: %s\n", qPrintable(_db.lastError().text()));
    }
    return ret;
}


int TSQLiteBlobStore::remove(const QByteArray &name)
{
    int cnt = -1;

    if (! isOpen() || name.isEmpty()) {
        return cnt;
    }

    TSqlQuery query(_db);
    QString sql = QStringLiteral("delete from %1 where %2=:name").arg(TABLE_NAME, KEY_COLUMN);

    query.prepare(sql);
    query.bind(":name", name);
    if (query.exec()) {
        cnt = query.numRowsAffected();
    }
    return cnt;
}


int TSQLiteBlobStore::removeOlder(int num)
{
    bool cnt = -1;

    if (! isOpen() || num < 1) {
        return cnt;
    }

    TSqlQuery query(_db);
    QString sql = QStringLiteral("delete from %1 where ROWID in (select ROWID from %1 order by t asc limit :num)").arg(TABLE_NAME);

    query.prepare(sql);
    query.bind(":num", num);
    if (query.exec()) {
        cnt = query.numRowsAffected();
    }
    return cnt;
}


int TSQLiteBlobStore::removeOlderThan(qint64 timestamp)
{
    bool cnt = -1;

    if (! isOpen()) {
        return cnt;
    }

    TSqlQuery query(_db);
    QString sql = QStringLiteral("delete from %1 where %2<:ts").arg(TABLE_NAME, TIMESTAMP_COLUMN);

    query.prepare(sql);
    query.bind(":ts", timestamp);
    if (query.exec()) {
        cnt = query.numRowsAffected();
    }
    return cnt;
}


int TSQLiteBlobStore::removeAll()
{
    bool cnt = -1;

    if (! isOpen()) {
        return cnt;
    }

    TSqlQuery query(_db);
    QString sql = QStringLiteral("delete from %1").arg(TABLE_NAME);

    if (query.exec(sql)) {
        cnt = query.numRowsAffected();
    }
    return cnt;
}


bool TSQLiteBlobStore::vacuum()
{
    return exec(QStringLiteral("vacuum"));
}


bool TSQLiteBlobStore::exec(const QString &sql)
{
    if (! isOpen()) {
        return false;
    }

    TSqlQuery query(_db);
    bool ret = query.exec(sql);
    if (! ret) {
        printf("error: %s\n", qPrintable(_db.lastError().text()));
    }
    return ret;
}
