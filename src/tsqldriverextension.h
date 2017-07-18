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
    virtual QString upsertStatement(const QString &tableName, const QSqlRecord &rec) const;
};


inline QString TSqlDriverExtension::upsertStatement(const QString &tableName, const QSqlRecord &rec) const
{
    Q_UNUSED(tableName);
    Q_UNUSED(rec);
    return QString();
}

#endif // TSQLDRIVEREXTENSION_H
