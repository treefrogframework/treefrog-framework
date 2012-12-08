#ifndef TACTIONCONTROLLER_H
#define TACTIONCONTROLLER_H

#include <QObject>
#include <QString>
#include <QHostAddress>
#include <TGlobal>
#include <TAbstractController>
#include <TActionHelper>
#include <THttpRequest>
#include <THttpResponse>
#include <TSession>
#include <TCookieJar>

class TActionView;
class TAbstractUser;
class TFormValidator;


class T_CORE_EXPORT TActionController : public QObject, public TAbstractController, public TActionHelper
{
    Q_OBJECT
public:
    TActionController();
    virtual ~TActionController() { }
  
    QString className() const;
    QString name() const;
    QString activeAction() const;
    const THttpRequest &httpRequest() const { return request; }
    const THttpResponse &httpResponse() const { return response; }
    QString getRenderingData(const QString &templateName, const QVariantHash &vars = QVariantHash());
    const TSession &session() const { return sessionStore; }
    virtual bool sessionEnabled() const { return true; }
    virtual bool csrfProtectionEnabled() const { return true; }
    virtual QStringList exceptionActionsOfCsrfProtection() const { return QStringList(); }
    virtual bool transactionEnabled() const { return true; }
    QByteArray authenticityToken() const;
    QString flash(const QString &name) const;
    QHostAddress clientAddress() const;
    virtual bool isUserLoggedIn() const;

    static void setCsrfProtectionInto(TSession &session);

protected:
    virtual bool preFilter() { return true; }
    virtual void postFilter() { }
    void setLayoutEnabled(bool enable);
    void setLayoutDisabled(bool disable);
    bool layoutEnabled() const;
    void setLayout(const QString &layout);
    QString layout() const;
    void setStatusCode(int code);
    int statusCode() const { return statCode; }
    void setFlash(const QString &name, const QVariant &value);
    void setFlashValidationErrors(const TFormValidator &validator, const QString &prefix = QString("err_"));
    TSession &session() { return sessionStore; }
    void setSession(const TSession &session);
    bool addCookie(const TCookie &cookie);
    bool addCookie(const QByteArray &name, const QByteArray &value, const QDateTime &expire = QDateTime(), const QString &path = QString(), const QString &domain = QString(), bool secure = false, bool httpOnly = false);
    QByteArray contentType() const;
    void setContentType(const QByteArray &type);
    bool render(const QString &action = QString(), const QString &layout = QString());
    bool renderTemplate(const QString &templateName, const QString &layout = QString());
    bool renderText(const QString &text, bool layoutEnable = false, const QString &layout = QString());
    bool renderErrorResponse(int statusCode);
    void redirect(const QUrl &url, int statusCode = Tf::Found);
    bool sendFile(const QString &filePath, const QByteArray &contentType, const QString &name = QString(), bool autoRemove = false);
    bool sendData(const QByteArray &data, const QByteArray &contentType, const QString &name = QString());
    void rollbackTransaction() { rollback = true; }
    void setAutoRemove(const QString &filePath);

    virtual bool userLogin(const TAbstractUser *user);
    virtual void userLogout();
    virtual QString identityKeyOfLoginUser() const;

    THttpRequest &httpRequest() { return request; }
    THttpResponse &httpResponse() { return response; }

private:
    void setActionName(const QString &name);
    void setHttpRequest(const THttpRequest &httpRequest);
    bool verifyRequest(const THttpRequest &request) const;
    QByteArray renderView(TActionView *view);
    void exportAllFlashVariants();
    const TActionController *controller() const { return this; }
    bool rollbackRequested() const { return rollback; }
    static QString layoutClassName(const QString &layout);
    static QString partialViewClassName(const QString &partial);

    mutable QString ctrlName;
    QString actName;
    int  statCode;
    bool rendered;
    bool layoutEnable;
    QString layoutName;
    THttpRequest request;
    THttpResponse response;
    QVariantHash flashVars;
    TSession sessionStore;
    TCookieJar cookieJar;
    bool rollback;
    QStringList autoRemoveFiles;

    friend class TActionContext;
    friend class TSessionCookieStore;
    friend class TDirectView;
    Q_DISABLE_COPY(TActionController)
};


inline QString TActionController::className() const
{
    return QString(metaObject()->className());
}

inline QString TActionController::activeAction() const
{
    return actName;
}

inline void TActionController::setHttpRequest(const THttpRequest &httpRequest)
{
    request = httpRequest;
}

inline void TActionController::setActionName(const QString &name)
{
    actName = name;
}

inline void TActionController::setLayoutEnabled(bool enable)
{
    layoutEnable = enable;
}

inline void TActionController::setLayoutDisabled(bool disable)
{
    layoutEnable = !disable;
}

inline bool TActionController::layoutEnabled() const
{
    return layoutEnable;
}

inline QString TActionController::layout() const
{
    return layoutName;
}

inline void TActionController::setStatusCode(int code)
{
    statCode = code;
}

inline QString TActionController::flash(const QString &name) const
{
    return flashVars.value(name).toString();
}

inline void TActionController::setFlash(const QString &name, const QVariant &value)
{
    flashVars.insert(name, value);
}

inline QByteArray TActionController::contentType() const
{
    return response.header().contentType();
}

inline void TActionController::setContentType(const QByteArray &type)
{
    response.header().setContentType(type);
}

#endif // TACTIONCONTROLLER_H
