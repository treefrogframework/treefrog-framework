#pragma once
#include <QString>
#include <TGlobal>

class QSqlRecord;


class T_CORE_EXPORT TSqlDriverExtension {
public:
    virtual ~TSqlDriverExtension() { }
    virtual QString key() const = 0;
    virtual bool isUpsertSupported() const { return false; }
    virtual QString upsertStatement(const QString &tableName, const QSqlRecord &recordToInsert,
        const QSqlRecord &recordToUpdate, const QString &pkField, const QString &lockRevisionField) const;
};


inline QString TSqlDriverExtension::upsertStatement(const QString &tableName, const QSqlRecord &recordToInsert,
    const QSqlRecord &recordToUpdate, const QString &pkField, const QString &lockRevisionField) const
{
    Q_UNUSED(tableName);
    Q_UNUSED(recordToInsert);
    Q_UNUSED(recordToUpdate);
    Q_UNUSED(pkField);
    Q_UNUSED(lockRevisionField);
    return QString();
}

