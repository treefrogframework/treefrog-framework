#ifndef TMONGOOBJECT_H
#define TMONGOOBJECT_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <TGlobal>


class T_CORE_EXPORT TMongoObject : public QObject, protected QVariantMap
{
public:
    TMongoObject();
    TMongoObject(const TMongoObject &other);
    TMongoObject &operator=(const TMongoObject &other);
    virtual ~TMongoObject() { }

    virtual QString collectionName() const;
    virtual QString objectId() const { return QString(); }

    bool create();
    bool update();
    bool remove();
    bool reload();
    bool isNull() const { return objectId().isEmpty(); }
    bool isNew() const { return objectId().isEmpty(); }
    bool isModified() const;

    QVariantMap toVariantMap() const;
    void setProperties(const QVariantMap &values);
    QStringList propertyNames() const;

protected:
    void setBsonData(const QVariantMap &bson);
    void syncToVariantMap();
    void syncToObject();
};

#endif // TMONGOOBJECT_H
