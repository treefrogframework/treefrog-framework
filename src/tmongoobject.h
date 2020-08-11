#pragma once
#include <QDateTime>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <TGlobal>
#include <TModelObject>


class T_CORE_EXPORT TMongoObject : public TModelObject, protected QVariantMap {
public:
    TMongoObject();
    TMongoObject(const TMongoObject &other);
    TMongoObject &operator=(const TMongoObject &other);
    virtual ~TMongoObject() { }

    virtual QString collectionName() const;
    virtual QString objectId() const { return QString(); }
    void setBsonData(const QVariantMap &bson);
    bool create() override;
    bool update() override;
    bool upsert(const QVariantMap &criteria);
    bool save() override;
    bool remove() override;
    bool reload();
    bool isNull() const override { return objectId().isEmpty(); }
    bool isNew() const { return objectId().isEmpty(); }
    bool isModified() const;
    void clear() override;

protected:
    void syncToVariantMap();
    void syncToObject();
    virtual QString &objectId() = 0;
};

