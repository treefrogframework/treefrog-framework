/* Copyright (c) 2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsqldriverextensionfactory.h"
#include "tsqldriverextension.h"
#include "tsystemglobal.h"
#include <QSqlRecord>
#include <QSqlField>
#include <QSqlDriver>


static QString prepareIdentifier(const QString &identifier, QSqlDriver::IdentifierType type,
                                 const QSqlDriver *driver)
{
    QString ret = identifier;
    if (! driver->isIdentifierEscaped(identifier, type)) {
        ret = driver->escapeIdentifier(identifier, type);
    }
    return ret;
}


class TMySQLDriverExtension : public TSqlDriverExtension
{
public:
    TMySQLDriverExtension(const QSqlDriver *drv = nullptr) : driver(drv) {}
    QString key() const override { return QLatin1String("QMYSQL"); }
    bool isUpsertSupported() const override { return true; }
    QString upsertStatement(const QString &tableName, const QSqlRecord &recordToInsert, const QSqlRecord &recordToUpdate, const QString &lockRevisionField) const override;

private:
    const QSqlDriver *driver {nullptr};
};


QString TMySQLDriverExtension::upsertStatement(const QString &tableName, const QSqlRecord &recordToInsert,
                                               const QSqlRecord &recordToUpdate, const QString &lockRevisionField) const
{
    QString statement;

    if (tableName.isEmpty() || recordToInsert.isEmpty() || recordToUpdate.isEmpty()) {
        return statement;
    }

    statement.reserve(256);
    statement.append(QLatin1String("INSERT INTO ")).append(tableName).append(QLatin1String(" ("));
    QString vals;

    for (int i = 0; i < recordToInsert.count(); ++i) {
        if (!recordToInsert.isGenerated(i)) {
            continue;
        }
        statement.append(prepareIdentifier(recordToInsert.fieldName(i), QSqlDriver::FieldName, driver)).append(QLatin1String(", "));
        vals.append(driver->formatValue(recordToInsert.field(i)));
        vals.append(QLatin1String(", "));
    }

    if (vals.isEmpty()) {
        statement.clear();
    } else {
        vals.chop(2); // remove trailing comma
        statement[statement.length() - 2] = QLatin1Char(')');
        statement.append(QLatin1String("VALUES (")).append(vals);
        statement.append(QLatin1String(") ON DUPLICATE KEY UPDATE "));
        vals.clear();

        for (int i = 0; i < recordToUpdate.count(); ++i) {
            if (!recordToUpdate.isGenerated(i)) {
                continue;
            }
            vals.append(prepareIdentifier(recordToUpdate.fieldName(i), QSqlDriver::FieldName, driver));
            vals.append(QLatin1Char('='));
            vals.append(driver->formatValue(recordToUpdate.field(i)));
            vals.append(QLatin1String(", "));
        }

        if (! lockRevisionField.isEmpty()) {
            auto str = prepareIdentifier(lockRevisionField, QSqlDriver::FieldName, driver);
            vals.append(str).append(QLatin1String("=1+")).append(str).append(QLatin1String(", "));
        }

        if (vals.isEmpty()) {
            statement.clear();
        } else {
            vals.chop(2); // remove trailing comma
            statement.append(vals);
        }
    }
    return statement;
}


QStringList TSqlDriverExtensionFactory::keys()
{
    QStringList ret;
    ret << TMySQLDriverExtension().key().toLower();
    return ret;
}

TSqlDriverExtension *TSqlDriverExtensionFactory::create(const QString &key, const QSqlDriver *driver)
{
    static const QString MYSQL_KEY = TMySQLDriverExtension().key().toLower();

    TSqlDriverExtension *extension = nullptr;
    QString k = key.toLower();
    if (k == MYSQL_KEY) {
        extension = new TMySQLDriverExtension(driver);
    }
    return extension;
}


void TSqlDriverExtensionFactory::destroy(const QString &key, TSqlDriverExtension *extension)
{
    static const QString MYSQL_KEY = TMySQLDriverExtension().key().toLower();

    if (! extension) {
        return;
    }

    QString k = key.toLower();
    if (k == MYSQL_KEY) {
        delete extension;
    }
}
