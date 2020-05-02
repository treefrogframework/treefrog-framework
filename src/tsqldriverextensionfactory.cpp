/* Copyright (c) 2017-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsqldriverextensionfactory.h"
#include "tsqldriverextension.h"
#include "tsystemglobal.h"
#include <QSqlDriver>
#include <QSqlField>
#include <QSqlRecord>


namespace {
QString prepareIdentifier(const QString &identifier, QSqlDriver::IdentifierType type,
    const QSqlDriver *driver)
{
    QString ret = identifier;
    if (!driver->isIdentifierEscaped(identifier, type)) {
        ret = driver->escapeIdentifier(identifier, type);
    }
    return ret;
}


QString generateInsertValues(const QSqlRecord &record, const QSqlDriver *driver, QString &statement)
{
    QString state, vals;
    for (int i = 0; i < record.count(); ++i) {
        if (!record.isGenerated(i)) {
            continue;
        }
        state.append(prepareIdentifier(record.fieldName(i), QSqlDriver::FieldName, driver)).append(QLatin1String(", "));
        vals.append(driver->formatValue(record.field(i)));
        vals.append(QLatin1String(", "));
    }

    state.chop(2);
    statement += state;
    vals.chop(2);
    return vals;
}


QString generateUpdateValues(const QString &table, const QSqlRecord &record, const QString &lockRevisionField, const QSqlDriver *driver)
{
    QString vals;
    for (int i = 0; i < record.count(); ++i) {
        if (!record.isGenerated(i)) {
            continue;
        }
        vals.append(prepareIdentifier(record.fieldName(i), QSqlDriver::FieldName, driver));
        vals.append(QLatin1Char('='));
        vals.append(driver->formatValue(record.field(i)));
        vals.append(QLatin1String(", "));
    }

    if (!lockRevisionField.isEmpty()) {
        auto str = prepareIdentifier(lockRevisionField, QSqlDriver::FieldName, driver);
        vals.append(str).append(QLatin1String("=1+"));
        if (!table.isEmpty()) {
            vals.append(table).append(QLatin1Char('.'));
        }
        vals.append(str).append(QLatin1String(", "));
    }

    vals.chop(2);  // remove trailing comma
    return vals;
}
}

class TMySQLDriverExtension : public TSqlDriverExtension {
public:
    TMySQLDriverExtension(const QSqlDriver *drv = nullptr) :
        driver(drv) { }
    QString key() const override { return QLatin1String("QMYSQL"); }
    bool isUpsertSupported() const override { return true; }
    QString upsertStatement(const QString &tableName, const QSqlRecord &recordToInsert, const QSqlRecord &recordToUpdate,
        const QString &pkField, const QString &lockRevisionField) const override;

private:
    const QSqlDriver *driver {nullptr};
};


QString TMySQLDriverExtension::upsertStatement(const QString &tableName, const QSqlRecord &recordToInsert,
    const QSqlRecord &recordToUpdate, const QString &, const QString &lockRevisionField) const
{
    QString statement;
    QString vals;

    if (tableName.isEmpty() || recordToInsert.isEmpty() || recordToUpdate.isEmpty()) {
        return statement;
    }

    statement.reserve(256);
    statement.append(QLatin1String("INSERT INTO ")).append(tableName).append(QLatin1String(" ("));

    vals = generateInsertValues(recordToInsert, driver, statement);
    if (vals.isEmpty()) {
        return QString();
    }

    statement.append(QLatin1String(") VALUES (")).append(vals);
    statement.append(QLatin1String(") ON DUPLICATE KEY UPDATE "));

    vals = generateUpdateValues("", recordToUpdate, lockRevisionField, driver);
    if (vals.isEmpty()) {
        return QString();
    }

    statement.append(vals);
    return statement;
}


class TPostgreSQLDriverExtension : public TSqlDriverExtension {
public:
    TPostgreSQLDriverExtension(const QSqlDriver *drv = nullptr) :
        driver(drv) { }
    QString key() const override { return QLatin1String("QPSQL"); }
    bool isUpsertSupported() const override { return true; }
    QString upsertStatement(const QString &tableName, const QSqlRecord &recordToInsert, const QSqlRecord &recordToUpdate,
        const QString &pkField, const QString &lockRevisionField) const override;

private:
    const QSqlDriver *driver {nullptr};
};


QString TPostgreSQLDriverExtension::upsertStatement(const QString &tableName, const QSqlRecord &recordToInsert,
    const QSqlRecord &recordToUpdate, const QString &pkField, const QString &lockRevisionField) const
{
    QString statement;
    QString vals;

    if (tableName.isEmpty() || recordToInsert.isEmpty() || pkField.isEmpty() || recordToUpdate.isEmpty()) {
        return statement;
    }

    statement.reserve(256);
    statement.append(QLatin1String("INSERT INTO ")).append(tableName).append(QLatin1String(" AS t0 ("));

    vals = generateInsertValues(recordToInsert, driver, statement);
    if (vals.isEmpty()) {
        return QString();
    }

    statement.append(QLatin1String(") VALUES (")).append(vals);
    statement.append(QLatin1String(") ON CONFLICT ("));
    statement.append(prepareIdentifier(pkField, QSqlDriver::FieldName, driver));
    statement.append(") DO UPDATE SET ");

    vals = generateUpdateValues("t0", recordToUpdate, lockRevisionField, driver);
    if (vals.isEmpty()) {
        return QString();
    }

    statement.append(vals);
    return statement;
}

namespace {
// Extension Keys
QString MYSQL_KEY;
QString PSQL_KEY;


void loadKeys()
{
    static bool done = []() {
        // Constants
        MYSQL_KEY = TMySQLDriverExtension().key().toLower();
        PSQL_KEY = TPostgreSQLDriverExtension().key().toLower();
        return true;
    }();
    Q_UNUSED(done);
}
}

/*!
  TSqlDriverExtensionFactory class
 */
QStringList TSqlDriverExtensionFactory::keys()
{
    QStringList ret;

    loadKeys();
    ret << MYSQL_KEY
        << PSQL_KEY;
    return ret;
}

TSqlDriverExtension *TSqlDriverExtensionFactory::create(const QString &key, const QSqlDriver *driver)
{
    TSqlDriverExtension *extension = nullptr;

    loadKeys();
    QString k = key.toLower();
    if (k == MYSQL_KEY) {
        extension = new TMySQLDriverExtension(driver);
    } else if (k == PSQL_KEY) {
        extension = new TPostgreSQLDriverExtension(driver);
    }
    return extension;
}


void TSqlDriverExtensionFactory::destroy(const QString &key, TSqlDriverExtension *extension)
{
    if (!extension) {
        return;
    }

    loadKeys();
    QString k = key.toLower();
    if (k == MYSQL_KEY) {
        delete extension;
    } else if (k == PSQL_KEY) {
        delete extension;
    } else {
        delete extension;
    }
}
