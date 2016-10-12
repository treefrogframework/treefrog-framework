#ifndef TABSTRACTCONTROLLER_H
#define TABSTRACTCONTROLLER_H

#include <QVariant>
#include <TGlobal>

class TFormValidator;


class T_CORE_EXPORT TAbstractController
{
public:
    TAbstractController();
    virtual ~TAbstractController() { }
    virtual QString name() const = 0;
    virtual QString activeAction() const = 0;

protected:
    QVariant variant(const QString &name) const;
    void exportVariant(const QString &name, const QVariant &value, bool overwrite = true);
    void exportValidationErrors(const TFormValidator &validator, const QString &prefix = QString("err_"));
    bool hasVariant(const QString &name) const;
    void exportVariants(const QVariantMap &map);
    const QVariantMap &allVariants() const { return exportVars; }
    QString viewClassName(const QString &action = QString()) const;
    QString viewClassName(const QString &contoller, const QString &action) const;

private:
    QVariantMap exportVars;
    T_DISABLE_COPY(TAbstractController)
    T_DISABLE_MOVE(TAbstractController)
};


inline QVariant TAbstractController::variant(const QString &name) const
{
    return exportVars.value(name);
}

inline bool TAbstractController::hasVariant(const QString &name) const
{
    return exportVars.contains(name);
}

#endif // TABSTRACTCONTROLLER_H
