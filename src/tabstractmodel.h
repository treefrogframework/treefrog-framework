#pragma once
#include <QVariant>
#include <QtCore>
#include <TGlobal>

class TModelObject;


class T_CORE_EXPORT TAbstractModel {
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
    virtual QVariantMap toVariantMap(const QStringList &properties = QStringList()) const;
    virtual void setProperties(const QJsonDocument &properties);
    virtual QJsonObject toJsonObject(const QStringList &properties = QStringList()) const;
#if QT_VERSION >= 0x050c00  // 5.12.0
    virtual QCborMap toCborMap(const QStringList &properties = QStringList()) const;
#endif

    QString variableNameToFieldName(const QString &name) const;
    static QString fieldNameToVariableName(const QString &name);

protected:
    virtual TModelObject *modelData() { return nullptr; }
    virtual const TModelObject *modelData() const { return nullptr; }
};

