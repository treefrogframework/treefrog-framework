#pragma once
#include <QMap>
#include <QObject>
#include <QStringList>
#include <QVariant>
#include <TGlobal>


class T_CORE_EXPORT TModelObject : public QObject {
public:
    TModelObject() :
        QObject() { }
    virtual ~TModelObject() { }
    virtual bool isNull() const = 0;
    virtual bool create() = 0;
    virtual bool update() = 0;
    virtual bool save() = 0;
    virtual bool remove() = 0;
    virtual void setProperties(const QVariantMap &value);
    virtual void clear();
    virtual QVariantMap toVariantMap() const;
    virtual QStringList propertyNames() const;
};
