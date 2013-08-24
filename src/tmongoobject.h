#ifndef TMONGOOBJECT_H
#define TMONGOOBJECT_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVariantMap>
#include <QStringList>
#include <TGlobal>
#include <TModelObject>


class T_CORE_EXPORT TMongoObject : public TModelObject, protected QVariantMap
{
public:
    TMongoObject();
    TMongoObject(const TMongoObject &other);
    TMongoObject &operator=(const TMongoObject &other);
    virtual ~TMongoObject() { }

    virtual QString collectionName() const;
    virtual QString objectId() const { return QString(); }
    void setBsonData(const QVariantMap &bson);
    bool create();
    bool update();
    bool remove();
    bool reload();
    bool isNull() const { return objectId().isEmpty(); }
    bool isNew() const { return objectId().isEmpty(); }
    bool isModified() const;
    void clear();

protected:
    void syncToVariantMap();
    void syncToObject();
    virtual QString &objectId() = 0;
};

#endif // TMONGOOBJECT_H
