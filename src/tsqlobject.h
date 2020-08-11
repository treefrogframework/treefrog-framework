#pragma once
#include <QDateTime>
#include <QObject>
#include <QSqlError>
#include <QSqlRecord>
#include <QStringList>
#include <QVariantMap>
#include <TGlobal>
#include <TModelObject>


class T_CORE_EXPORT TSqlObject : public TModelObject, public QSqlRecord {
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
    bool create() override;
    bool update() override;
    bool save() override;
    bool remove() override;
    bool reload();
    bool isNull() const override { return QSqlRecord::isEmpty(); }
    bool isNew() const { return QSqlRecord::isEmpty(); }
    bool isModified() const;
    void clear() override { QSqlRecord::clear(); }
    QSqlError error() const { return sqlError; }

protected:
    void syncToSqlRecord();
    void syncToObject();
    QSqlError sqlError;
};

