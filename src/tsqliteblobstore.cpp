/* Copyright (c) 2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsqliteblobstore.h"
#include "tsystemglobal.h"
#include <QByteArray>
#include <QDateTime>
#include <QFileInfo>
#include <sqlite3.h>

constexpr auto TABLE_NAME = "kb";
constexpr auto KEY_COLUMN  = "k";
constexpr auto BLOB_COLUMN = "b";
constexpr auto TIMESTAMP_COLUMN = "t";


class TSQLiteBlob {
public:
    TSQLiteBlob() {}
    ~TSQLiteBlob() { close(); }

    bool open(sqlite3 *db, const char *table, const char *column, sqlite3_int64 rowid);
    void close();
    bool isOpen() const { return (bool)_handle; }
    bool read(QByteArray &data);
    bool write(const QByteArray &data);
    qint64 length() const;

private:
    sqlite3_blob *_handle {nullptr};
};


bool TSQLiteBlob::open(sqlite3 *db, const char *table, const char *column, sqlite3_int64 rowid)
{
    if (isOpen()) {
        close();
    }

    int rc = sqlite3_blob_open(db, "main", table, column, rowid, 1, &_handle);
    if (rc != SQLITE_OK) {
        _handle = nullptr;
        tSystemError("Failed to open blob of sqlite3 : %s [%d] ROWID:%lld\n", sqlite3_errmsg(db), rc, rowid);
    }
    return (rc == SQLITE_OK);
}


void TSQLiteBlob::close()
{
    if (isOpen()) {
        sqlite3_blob_close(_handle);
        _handle = nullptr;
    }
}


bool TSQLiteBlob::read(QByteArray &data)
{
    auto len = length();
    if (len < 1) {
        return false;
    }

    // Read
    data.reserve(len);
    int rc = sqlite3_blob_read(_handle, data.data(), (int)len, 0);
    if (rc != SQLITE_OK) {
        tSystemError("Failed to read blob. Return code : %d", rc);
        return false;
    }

    data.resize(len);
    return true;
}


qint64 TSQLiteBlob::length() const
{
    return (isOpen()) ? sqlite3_blob_bytes(_handle) : -1;
}


bool TSQLiteBlob::write(const QByteArray &data)
{
    if (! isOpen()) {
        return false;
    }

    int rc = sqlite3_blob_write(_handle, data.data(), (int)data.length(), 0);
    if (rc != SQLITE_OK) {
        tSystemError("Failed to write blob. Return code : %d", rc);
        return false;
    }
    return true;
}


bool TSQLiteBlobStore::setup(const QByteArray &fileName)
{
    constexpr int PAGESIZE = 4096;
    TSQLiteBlobStore store;

    if (! store.open(fileName)) {
        return false;
    }

    char *sql = sqlite3_mprintf(
        "pragma page_size=%d;\n"
        "vacuum;\n"
        "begin;\n"
        "create table if not exists %s (%s text primary key, %s blob, %s integer);\n"
        "commit;\n",
        PAGESIZE, TABLE_NAME, KEY_COLUMN, BLOB_COLUMN, TIMESTAMP_COLUMN
    );

    char *errmsg = nullptr;
    int rc = sqlite3_exec(store._db, sql, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        tSystemError("Failed to create a database : %s  [%s:%d]", sqlite3_errmsg(store._db), __FILE__, __LINE__);
    }

    sqlite3_free(errmsg);
    sqlite3_free(sql);
    store.close();
    return true;
}


bool TSQLiteBlobStore::open(const QByteArray &fileName)
{
    if (isOpen()) {
        close();
    }

    int rc = sqlite3_open(fileName.data(), &_db);
    if (rc != SQLITE_OK) {
         _db = nullptr;
         tSystemError("Failed to open database file for sqlite3 : %s", sqlite3_errmsg(_db));
    }

    char *errmsg = nullptr;
	rc = sqlite3_exec(_db,
        "PRAGMA journal_mode=WAL;" \
        "PRAGMA foreign_keys=ON;" \
        "PRAGMA busy_timeout=5000;" \
        "PRAGMA synchronous=NORMAL;",
	    nullptr,
	    nullptr,
	    &errmsg
    );
    if (rc != SQLITE_OK) {
        tSystemError("sqlite3_exec failed: %s  [%s:%d]", sqlite3_errmsg(_db), __FILE__, __LINE__);
    }
    sqlite3_free(errmsg);

    return (rc == SQLITE_OK);
}


void TSQLiteBlobStore::close()
{
    if (isOpen()) {
        sqlite3_close(_db);
        _db = nullptr;
    }
}


int TSQLiteBlobStore::count() const
{
    int cnt = -1;
    auto sql = sqlite3_mprintf(
	    "select count(1) from %s",
	    TABLE_NAME
	);

	sqlite3_stmt *stmt = nullptr;
    int rc = sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        tSystemError("sqlite3_prepare_v2 failed: %s  [%s:%d]", sqlite3_errmsg(_db), __FILE__, __LINE__);
    } else {
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            tSystemError("sqlite3_step failed: %s  [%s:%d]", sqlite3_errmsg(_db), __FILE__, __LINE__);
        } else {
            cnt = sqlite3_column_int(stmt, 0);
        }
    }

    sqlite3_free(sql);
    sqlite3_finalize(stmt);
    return cnt;
}


bool TSQLiteBlobStore::exists(const QByteArray &name) const
{
    if (name.isEmpty()) {
        return false;
    }

    auto sql = sqlite3_mprintf(
        "select exists(select 1 from %s where %s=%Q limit 1)",
        TABLE_NAME,
        KEY_COLUMN,
        name.data()
    );

    int exist = 0;
	sqlite3_stmt *stmt = nullptr;
    int rc = sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        tSystemError("sqlite3_prepare_v2 failed: %s  [%s:%d]", sqlite3_errmsg(_db), __FILE__, __LINE__);
    } else {
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            tSystemError("sqlite3_step failed: %s  [%s:%d]", sqlite3_errmsg(_db), __FILE__, __LINE__);
        } else {
            exist = sqlite3_column_int(stmt, 0);
        }
    }

    sqlite3_free(sql);
    sqlite3_finalize(stmt);
    return (exist > 0);
}


bool TSQLiteBlobStore::read(const QByteArray &name, QByteArray &blob, qint64 &timestamp)
{
    bool ret = false;

    if (name.isEmpty()) {
        return ret;
    }

    auto sql = sqlite3_mprintf(
	    "select ROWID,%s from %s where %s=%Q",
        TIMESTAMP_COLUMN,
	    TABLE_NAME,
        KEY_COLUMN,
		name.data()
	);

	sqlite3_stmt *stmt = nullptr;
    int rc = sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        tSystemError("sqlite3_prepare_v2 failed: %s  [%s:%d]", sqlite3_errmsg(_db), __FILE__, __LINE__);
    } else {
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            tSystemError("sqlite3_step failed: %s  [%s:%d]", sqlite3_errmsg(_db), __FILE__, __LINE__);
        } else {
            qint64 rowid = sqlite3_column_int64(stmt, 0);
            timestamp = sqlite3_column_int64(stmt, 1);

            TSQLiteBlob sqblob;
            ret = sqblob.open(_db, TABLE_NAME, BLOB_COLUMN, rowid);
            if (ret) {
                ret = sqblob.read(blob);
            }
        }
    }

    sqlite3_free(sql);
    sqlite3_finalize(stmt);
    return ret;
}


bool TSQLiteBlobStore::write(const QByteArray &name, const QByteArray &blob, qint64 timestamp)
{
    bool ret = false;

    if (name.isEmpty() || blob.isEmpty()) {
        return ret;
    }

    char *errmsg = nullptr;
    int rc = sqlite3_exec(_db, "begin", nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        tSystemError("Failed to begin transaction : %s", errmsg);
    }
    sqlite3_free(errmsg);

    auto sql = sqlite3_mprintf(
	    "insert into %s (%s,%s,%s) values (%Q,?1,%lld)",
	    TABLE_NAME,
        KEY_COLUMN,
        BLOB_COLUMN,
        TIMESTAMP_COLUMN,
        name.data(),
        timestamp
	);

	sqlite3_stmt *stmt = nullptr;
    rc = sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        tSystemError("sqlite3_prepare_v2 failed: %s  [%s:%d]", sqlite3_errmsg(_db), __FILE__, __LINE__);
    } else {
        rc = sqlite3_bind_blob(stmt, 1, blob.data(), blob.length(), SQLITE_STATIC);
        if (rc != SQLITE_OK) {
            tSystemError("sqlite3_bind_blob failed: %s  [%s:%d]", sqlite3_errmsg(_db), __FILE__, __LINE__);
        } else {
            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                tSystemError("sqlite3_step failed: %s  [%s:%d]", sqlite3_errmsg(_db), __FILE__, __LINE__);
            } else {
                ret = true;
            }
        }
    }

    errmsg = nullptr;
    rc = sqlite3_exec(_db, "commit", nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        tSystemError("Failed to commit transaction : %s", errmsg);
    }
    sqlite3_free(errmsg);
    sqlite3_free(sql);
    sqlite3_finalize(stmt);
    return ret;
}


int TSQLiteBlobStore::remove(const QByteArray &name)
{
    auto sql = sqlite3_mprintf(
	    "delete from %s where %s=%Q",
	    TABLE_NAME,
        KEY_COLUMN,
		name.data()
	);

	char *errmsg = nullptr;
	int rc = sqlite3_exec(_db, sql, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        tSystemError("sqlite3_exec failed: %s  [%s:%d]", sqlite3_errmsg(_db), __FILE__, __LINE__);
    }
    sqlite3_free(errmsg);
    sqlite3_free(sql);
    return (rc != SQLITE_OK) ? -1 : sqlite3_changes(_db);
}


int TSQLiteBlobStore::removeOlder(int num)
{
    auto sql = sqlite3_mprintf(
        "delete from %s where ROWID in (select ROWID from %s order by t asc limit %d)",
        TABLE_NAME,
        TABLE_NAME,
        num
	);

 	char *errmsg = nullptr;
	int rc = sqlite3_exec(_db, sql, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        tSystemError("sqlite3_exec failed: %s  [%s:%d]", sqlite3_errmsg(_db), __FILE__, __LINE__);
    }
    sqlite3_free(errmsg);
    sqlite3_free(sql);
    return (rc != SQLITE_OK) ? -1 : sqlite3_changes(_db);
}


int TSQLiteBlobStore::removeOlderThan(qint64 timestamp)
{
    auto sql = sqlite3_mprintf(
	    "delete from %s where %s<%lld",
	    TABLE_NAME,
        TIMESTAMP_COLUMN,
		timestamp
	);

	char *errmsg = nullptr;
	int rc = sqlite3_exec(_db, sql, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        tSystemError("sqlite3_exec failed: %s  [%s:%d]", sqlite3_errmsg(_db), __FILE__, __LINE__);
    }
    sqlite3_free(errmsg);
    sqlite3_free(sql);
    return (rc != SQLITE_OK) ? -1 : sqlite3_changes(_db);
}


int TSQLiteBlobStore::removeAll()
{
    auto sql = sqlite3_mprintf(
	    "delete from %s",
	    TABLE_NAME
	);

	char *errmsg = nullptr;
	int rc = sqlite3_exec(_db, sql, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        tSystemError("sqlite3_exec failed: %s  [%s:%d]", sqlite3_errmsg(_db), __FILE__, __LINE__);
    }
    sqlite3_free(errmsg);
    sqlite3_free(sql);
    return (rc != SQLITE_OK) ? -1 : sqlite3_changes(_db);
}


bool TSQLiteBlobStore::vacuum()
{
    char *errmsg = nullptr;
	int rc = sqlite3_exec(_db, "vacuum", nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        tSystemError("sqlite3_exec failed: %s  [%s:%d]", sqlite3_errmsg(_db), __FILE__, __LINE__);
    }
    sqlite3_free(errmsg);
    return (rc == SQLITE_OK);
}
