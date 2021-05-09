#pragma once
#include <QList>
#include <QPair>
#include <QSqlRecord>
#include <QString>
#include <QVariant>


class TableSchema {
public:
    TableSchema(const QString &table, const QString &env = "dev");
    bool exists() const;
    QList<QPair<QString, QString>> getFieldList() const;
    QList<QPair<QString, QMetaType::Type>> getFieldTypeList() const;
    int primaryKeyIndex() const;
    QString primaryKeyFieldName() const;
    int autoValueIndex() const;
    QString autoValueFieldName() const;
    QPair<QString, QString> getPrimaryKeyField() const;
    QPair<QString, QMetaType::Type> getPrimaryKeyFieldType() const;
    QString tableName() const { return tablename; }
    int lockRevisionIndex() const;
    bool hasLockRevisionField() const;
    static QStringList databaseDrivers();
    static QStringList tables(const QString &env = "dev");

protected:
    bool openDatabase(const QString &env) const;
    bool isOpen() const;

private:
    QString tablename;
    QSqlRecord tableFields;
};

