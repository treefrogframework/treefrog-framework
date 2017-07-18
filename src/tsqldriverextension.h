#ifndef TSQLDRIVEREXTENSION_H
#define TSQLDRIVEREXTENSION_H

#include <QString>
#include <TGlobal>

class QSqlRecord;


class T_CORE_EXPORT TSqlDriverExtension
{
public:
    virtual ~TSqlDriverExtension() { }
    virtual QString key() const = 0;
    virtual bool isUpsertSupported() const { return false; }
    virtual QString upsertStatement(const QString &tableName, const QSqlRecord &rec, const QString &uniqueKeyName) const;
};


inline QString TSqlDriverExtension::upsertStatement(const QString &tableName, const QSqlRecord &rec, const QString &uniqueKeyName) const
{
    Q_UNUSED(tableName);
    Q_UNUSED(rec);
    Q_UNUSED(uniqueKeyName);
    return QString();
}

#endif // TSQLDRIVEREXTENSION_H
