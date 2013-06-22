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

    static QString fieldNameToVariableName(const QString &name);

protected:
    virtual TModelObject *modelData() { return 0; }
    virtual const TModelObject *modelData() const { return 0; }
};


inline QString TAbstractModel::fieldNameToVariableName(const QString &name)
{
    QString ret;
    bool existsLower = false;
    bool existsUnders = name.contains('_');
    const QLatin1Char Underscore('_');

    ret.reserve(name.length());

    for (int i = 0; i < name.length(); ++i) {
        const QChar &c = name[i];
        if (c == Underscore) {
            if (i > 0 && i + 1 < name.length()) {
                ret += name[++i].toUpper();
            }
        } else {
            if (!existsLower && c.isLower()) {
                existsLower = true;
            }
            ret += (existsLower && !existsUnders) ? c : c.toLower();
        }
    }
    return ret;
}

#endif // TABSTRACTMODEL_H
