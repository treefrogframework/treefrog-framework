#ifndef TABSTRACTMODEL_H
#define TABSTRACTMODEL_H

#include <QVariant>
#include <TGlobal>

class TSqlObject;
class TModelObject;


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
    virtual void setProperties(const QVariantMap &properties);
    virtual QVariantMap toVariantMap() const;

    QString variableNameToFieldName(const QString &name) const;
    static QString fieldNameToVariableName(const QString &name);

protected:
    virtual TModelObject *modelData() { return nullptr; }
    virtual const TModelObject *modelData() const { return nullptr; }
};

#endif // TABSTRACTMODEL_H
