#pragma once
#include <QObject>
#include <QVariant>
#include <TGlobal>

class THttpRequest;
class TSession;
class TCookie;
class TFormValidator;


class T_CORE_EXPORT TAbstractController : public QObject {
public:
    TAbstractController();
    virtual ~TAbstractController() { }
    virtual QString className() const;
    virtual QString name() const = 0;
    virtual QString activeAction() const = 0;
    virtual QStringList arguments() const = 0;
    virtual const THttpRequest &httpRequest() const = 0;
    virtual const THttpRequest &request() const = 0;
    virtual const TSession &session() const = 0;
    virtual QString getRenderingData(const QString &templateName, const QVariantMap &vars = QVariantMap()) = 0;
    virtual QByteArray authenticityToken() const = 0;
    virtual void setFlash(const QString &name, const QVariant &value) = 0;
    void exportVariant(const QString &name, const QVariant &value, bool overwrite = true);

protected:
    virtual TSession &session() = 0;
    virtual bool addCookie(const TCookie &cookie) = 0;
    virtual bool addCookie(const QByteArray &name, const QByteArray &value, const QDateTime &expire = QDateTime(), const QString &path = QString(), const QString &domain = QString(), bool secure = false, bool httpOnly = false, const QByteArray &sameSite = "Lax") = 0;
    virtual bool addCookie(const QByteArray &name, const QByteArray &value, qint64 maxAge, const QString &path = QString(), const QString &domain = QString(), bool secure = false, bool httpOnly = false, const QByteArray &sameSite = "Lax") = 0;

    QVariant variant(const QString &name) const;
    void exportVariants(const QVariantMap &map);
    void exportValidationErrors(const TFormValidator &validator, const QString &prefix = QString("err_"));
    bool hasVariant(const QString &name) const;
    const QVariantMap &allVariants() const { return exportVars; }
    QString viewClassName(const QString &action = QString()) const;
    QString viewClassName(const QString &contoller, const QString &action) const;

private:
    QVariantMap exportVars;

    T_DISABLE_COPY(TAbstractController)
    T_DISABLE_MOVE(TAbstractController)
    friend class TDirectView;
};


/*!
  \fn QString TAbstractController::className() const
  Returns the class name.
*/
inline QString TAbstractController::className() const
{
    return QString(metaObject()->className());
}

inline QVariant TAbstractController::variant(const QString &name) const
{
    return exportVars.value(name);
}

inline bool TAbstractController::hasVariant(const QString &name) const
{
    return exportVars.contains(name);
}

