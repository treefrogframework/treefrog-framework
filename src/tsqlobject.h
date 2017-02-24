#ifndef TSQLOBJECT_H
#define TSQLOBJECT_H

#include <QObject>
#include <QSqlRecord>
#include <QSqlError>
#include <QDateTime>
#include <QVariantMap>
#include <QStringList>
#include <TGlobal>
#include <TModelObject>


class T_CORE_EXPORT TSqlObject : public TModelObject, public QSqlRecord
{
    Q_OBJECT
public:
    TSqlObject();
    TSqlObject(const TSqlObject &other);
    TSqlObject &operator=(const TSqlObject &other);
    virtual ~TSqlObject() { }

    virtual QString tableName() const;
    virtual QList<int> primaryKeyIndex() const { QList<int> pkidxs;return pkidxs; }
    virtual int autoValueIndex() const { return -1; }
    virtual int databaseId() const { return 0; }
    void setRecord(const QSqlRecord &record, const QSqlError &error);
    bool create();
    bool update();
    bool remove();
    bool reload();
    bool isNull() const { return QSqlRecord::isEmpty(); }
    bool isNew() const { return QSqlRecord::isEmpty(); }
    bool isModified() const;
    void clear() { QSqlRecord::clear(); }
    QSqlError error() const { return sqlError; }

protected:
    void syncToSqlRecord();
    void syncToObject();
    QSqlError sqlError;
};

#endif // TSQLOBJECT_H
