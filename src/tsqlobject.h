#ifndef TSQLOBJECT_H
#define TSQLOBJECT_H

#include <QObject>
#include <QSqlRecord>
#include <QSqlError>
#include <QDateTime>
#include <QVariantHash>
#include <TGlobal>


class T_CORE_EXPORT TSqlObject : public QObject, public QSqlRecord
{
    Q_OBJECT
public:
    TSqlObject();
    TSqlObject(const TSqlObject &other);
    TSqlObject &operator=(const TSqlObject &other);
    virtual ~TSqlObject() { }

    virtual QString tableName() const;
    virtual int primaryKeyIndex() const { return -1; }
    virtual int autoValueIndex() const { return -1; }
    virtual int databaseId() const { return 0; }
    void setRecord(const QSqlRecord &record, const QSqlError &error);
    bool create();
    bool update();
    bool remove();
    bool reload();
    bool isNull() const { return isEmpty(); }
    bool isNew() const { return isEmpty(); }
    bool isModified() const;
    QSqlError error() const { return sqlError; }

    virtual QVariantHash properties() const;
    virtual void setProperties(const QVariantHash &values);

protected:
    void syncToSqlRecord();
    void syncToObject();

private:
    mutable QString tblName;
    QSqlError sqlError;
};

#endif // TSQLOBJECT_H
