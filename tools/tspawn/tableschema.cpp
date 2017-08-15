/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QtCore>
#include <QtSql>
#include "tableschema.h"
#include "global.h"

QSettings *dbSettings = 0;


TableSchema::TableSchema(const QString &table, const QString &env)
    : tablename(table)
{
    if (!dbSettings) {
        QString path = QLatin1String("config") + QDir::separator() + "database.ini";
        if (!QFile::exists(path)) {
            qCritical("not found, %s", qPrintable(path));
        }
        dbSettings = new QSettings(path, QSettings::IniFormat);
    }

    if (openDatabase(env)) {
        if (!tablename.isEmpty()) {
            QSqlTableModel model;
            model.setTable(tablename);
            tableFields = model.record();

            if (model.database().driverName().toUpper() == "QPSQL") {
                // QPSQLResult doesn't call QSqlField::setAutoValue(), fix it
                for (int i = 0; i < tableFields.count(); ++i) {
                    QSqlField f = tableFields.field(i);
                    if (f.defaultValue().toString().startsWith(QLatin1String("nextval"))) {
                        f.setAutoValue(true);
                        tableFields.replace(i, f);
                    }
                }
            }
        } else {
            qCritical("Empty table name");
        }
    }
}


bool TableSchema::exists() const
{
    return (tableFields.count() > 0);
}


QList<QPair<QString, QString>> TableSchema::getFieldList() const
{
    QList<QPair<QString, QString>> fieldList;
    for (int i = 0; i < tableFields.count(); ++i) {
        QSqlField f = tableFields.field(i);
        fieldList << QPair<QString, QString>(f.name(), QString(QVariant::typeToName(f.type())));
    }
    return fieldList;
}


QList<QPair<QString, QVariant::Type>> TableSchema::getFieldTypeList() const
{
    QList<QPair<QString, QVariant::Type>> fieldList;
    for (int i = 0; i < tableFields.count(); ++i) {
        QSqlField f = tableFields.field(i);
        fieldList << QPair<QString, QVariant::Type>(f.name(), f.type());
    }
    return fieldList;
}


int TableSchema::primaryKeyIndex() const
{
    QSqlTableModel model;
    model.setTable(tablename);
    QSqlIndex index = model.primaryKey();
    if (index.isEmpty()) {
        return -1;
    }

    QSqlField fi = index.field(0);
    return model.record().indexOf(fi.name());
}


QString TableSchema::primaryKeyFieldName() const
{
    QSqlTableModel model;
    model.setTable(tablename);
    QSqlIndex index = model.primaryKey();
    if (index.isEmpty()) {
        return QString();
    }

    QSqlField fi = index.field(0);
    return fi.name();
}


int TableSchema::autoValueIndex() const
{
    for (int i = 0; i < tableFields.count(); ++i) {
        QSqlField f = tableFields.field(i);
        if (f.isAutoValue()) {
            return i;
        }
    }
    return -1;
}


QString TableSchema::autoValueFieldName() const
{
    for (int i = 0; i < tableFields.count(); ++i) {
        QSqlField f = tableFields.field(i);
        if (f.isAutoValue()) {
            return f.name();
        }
    }
    return QString();
}


QPair<QString, QString> TableSchema::getPrimaryKeyField() const
{
    QPair<QString, QString> pair;
    int index = primaryKeyIndex();
    if (index >= 0) {
        QSqlField f = tableFields.field(index);
        pair = QPair<QString, QString>(f.name(), QString(QVariant::typeToName(f.type())));
    }
    return pair;
}


QPair<QString, QVariant::Type> TableSchema::getPrimaryKeyFieldType() const
{
    QPair<QString, QVariant::Type> pair;
    int index = primaryKeyIndex();
    if (index >= 0) {
        QSqlField f = tableFields.field(index);
        pair = QPair<QString, QVariant::Type>(f.name(), f.type());
    }
    return pair;
}


int TableSchema::lockRevisionIndex() const
{
    for (int i = 0; i < tableFields.count(); ++i) {
        QSqlField f = tableFields.field(i);
        if (fieldNameToVariableName(f.name()) == "lockRevision") {
            return i;
        }
    }
    return -1;
}


bool TableSchema::hasLockRevisionField() const
{
    return lockRevisionIndex() >= 0;
}


bool TableSchema::openDatabase(const QString &env) const
{
    if (isOpen())
        return true;

    if (!dbSettings->childGroups().contains(env)) {
        qCritical("invalid environment: %s", qPrintable(env));
        return false;
    }

    dbSettings->beginGroup(env);

    QString driverType = dbSettings->value("DriverType").toString().trimmed();
    if (driverType.isEmpty()) {
        qWarning("Parameter 'DriverType' is empty");
    }
    printf("DriverType:   %s\n", qPrintable(driverType));

    QSqlDatabase db = QSqlDatabase::addDatabase(driverType);
    if (!db.isValid()) {
        qWarning("Parameter 'DriverType' is invalid or RDB client library not available.");
        return false;
    }

    QString databaseName = dbSettings->value("DatabaseName").toString().trimmed();
    printf("DatabaseName: %s\n", qPrintable(databaseName));
    if (!databaseName.isEmpty()) {
        db.setDatabaseName(databaseName);
    }

    QString hostName = dbSettings->value("HostName").toString().trimmed();
    printf("HostName:     %s\n", qPrintable(hostName));
    if (!hostName.isEmpty()) {
        db.setHostName(hostName);
    }

    int port = dbSettings->value("Port").toInt();
    if (port > 0) {
        db.setPort(port);
    }

    QString userName = dbSettings->value("UserName").toString().trimmed();
    if (!userName.isEmpty()) {
        db.setUserName(userName);
    }

    QString password = dbSettings->value("Password").toString().trimmed();
    if (!password.isEmpty()) {
        db.setPassword(password);
    }

    QString connectOptions = dbSettings->value("ConnectOptions").toString().trimmed();
    if (!connectOptions.isEmpty()) {
        db.setConnectOptions(connectOptions);
    }

    dbSettings->endGroup();

    if (!db.open()) {
        qWarning("Database open error");
        return false;
    }

    printf("Database opened successfully\n");
    return true;
}


bool TableSchema::isOpen() const
{
    return QSqlDatabase::database().isOpen();
}


QStringList TableSchema::databaseDrivers()
{
    return QSqlDatabase::drivers();
}


QStringList TableSchema::tables(const QString &env)
{
    QSet<QString> set;
    TableSchema dummy("dummy", env);  // to open database

    if (QSqlDatabase::database().isOpen()) {
        for (QStringListIterator i(QSqlDatabase::database().tables(QSql::Tables)); i.hasNext(); ) {
            TableSchema t(i.next());
            if (t.exists()) {
                set << t.tableName(); // If value already exists, the set is left unchanged
            }
        }
    }

    QStringList ret = set.toList();
    qSort(ret);
    return ret;
}
