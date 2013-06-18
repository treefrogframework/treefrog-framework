#ifndef TABLESCHEMA_H
#define TABLESCHEMA_H

#include <QString>
#include <QList>
#include <QPair>
#include <QSqlRecord>


class TableSchema
{
public:
    TableSchema(const QString &table, const QString &env = "dev");
    bool exists() const;
    QList<QPair<QString, QString> > getFieldList() const;
    QList<QPair<QString, int> > getFieldTypeList() const;
    int primaryKeyIndex() const;
    QString primaryKeyFieldName() const;
    int autoValueIndex() const;
    QString autoValueFieldName() const;
    QPair<QString, QString> getPrimaryKeyField() const;
    QPair<QString, int> getPrimaryKeyFieldType() const;
    QString tableName() const { return tablename; }
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

#endif // TABLESCHEMA_H
