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
    QString upsertStatement(const QString &tableName, const QSqlRecord &rec, const QString &uniqueKeyName) const override;
private:
    const QSqlDriver *driver {nullptr};
};

QString TMySQLDriverExtension::upsertStatement(const QString &tableName, const QSqlRecord &rec,
                                               const QString &uniqueKeyName) const
{
    QString statement;

    auto sqlField = rec.field(uniqueKeyName);
    if (tableName.isEmpty() || uniqueKeyName.isEmpty() || sqlField.isNull()) {
        return statement;
    }

    statement.reserve(256);
    statement.append(QLatin1String("INSERT INTO ")).append(tableName).append(QLatin1String(" ("));
    QString vals;

    for (int i = 0; i < rec.count(); ++i) {
        if (!rec.isGenerated(i)) {
            continue;
        }
        statement.append(prepareIdentifier(rec.fieldName(i), QSqlDriver::FieldName, driver)).append(QLatin1String(", "));
        vals.append(driver->formatValue(rec.field(i)));
        vals.append(QLatin1String(", "));
    }

    if (vals.isEmpty()) {
        statement.clear();
    } else {
        vals.chop(2); // remove trailing comma
        statement[statement.length() - 2] = QLatin1Char(')');
        statement.append(QLatin1String("VALUES (")).append(vals);
        statement.append(QLatin1String(") ON DUPLICATE KEY UPDATE "));
        statement.append(prepareIdentifier(uniqueKeyName, QSqlDriver::FieldName, driver));
        statement.append(QLatin1Char('=')).append(driver->formatValue(sqlField));
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
