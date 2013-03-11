#ifndef TABSTRACTMODEL_H
#define TABSTRACTMODEL_H

#include <QVariant>
#include <TGlobal>

class TSqlObject;


class T_CORE_EXPORT TAbstractModel
{
public:
    virtual ~TAbstractModel() { }
    virtual bool create();
    virtual bool save();
    virtual bool update();
    virtual bool remove();
    virtual bool isNull() const;
    virtual bool isNew() const;
    virtual bool isSaved() const;
    virtual QVariantMap properties() const;  // obsolete function
    virtual void setProperties(const QVariantMap &properties);
    virtual QVariantMap toVariantMap() const;

protected:
    virtual TSqlObject *data() = 0;
    virtual const TSqlObject *data() const = 0;
};

#endif // TABSTRACTMODEL_H
