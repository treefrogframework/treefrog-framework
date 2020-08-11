#pragma once
#include <QDomDocument>
#include <QHostAddress>
#include <QtCore>
#include <TAbstractController>
#include <TAccessValidator>
#include <TActionHelper>
#include <TCookieJar>
#include <TGlobal>
#include <THttpRequest>
#include <THttpResponse>
#include <TSession>

class TActionView;
class TAbstractUser;
class TFormValidator;
class TCache;


class T_CORE_EXPORT TActionController : public QObject, public TAbstractController, public TActionHelper, protected TAccessValidator {
public:
    TActionController();
    virtual ~TActionController();

    QString className() const;
    QString name() const;
    QString activeAction() const;
    QStringList arguments() const;
    const THttpRequest &httpRequest() const;
    const THttpResponse &httpResponse() const { return response; }
    QString getRenderingData(const QString &templateName, const QVariantMap &vars = QVariantMap());
    const TSession &session() const { return sessionStore; }
    virtual bool sessionEnabled() const { return true; }
    virtual bool csrfProtectionEnabled() const { return true; }
    virtual QStringList exceptionActionsOfCsrfProtection() const { return QStringList(); }
    virtual bool transactionEnabled() const { return true; }
    QByteArray authenticityToken() const;
    QString flash(const QString &name) const;
    QHostAddress clientAddress() const;
    virtual bool isUserLoggedIn() const;
    virtual QString identityKeyOfLoginUser() const;

    static void setCsrfProtectionInto(TSession &session);
    static const QStringList &availableControllers();
    static const QStringList &disabledControllers();
    static QString loginUserNameKey();

protected:
    virtual bool preFilter() { return true; }
    virtual void postFilter() {}
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
    bool addCookie(const QByteArray &name, const QByteArray &value, const QDateTime &expire = QDateTime(), const QString &path = QString(), const QString &domain = QString(), bool secure = false, bool httpOnly = false, const QByteArray &sameSite = "Lax");
    bool addCookie(const QByteArray &name, const QByteArray &value, qint64 maxAge, const QString &path = QString(), const QString &domain = QString(), bool secure = false, bool httpOnly = false, const QByteArray &sameSite = "Lax");
    QByteArray contentType() const;
    void setContentType(const QByteArray &type);
    bool render(const QString &action = QString(), const QString &layout = QString());
    bool renderTemplate(const QString &templateName, const QString &layout = QString());
    bool renderText(const QString &text, bool layoutEnable = false, const QString &layout = QString());
    bool renderXml(const QDomDocument &document);
    bool renderXml(const QVariantMap &map);
    bool renderXml(const QVariantList &list);
    bool renderXml(const QStringList &list);
    bool renderJson(const QJsonDocument &document);
    bool renderJson(const QJsonObject &object);
    bool renderJson(const QJsonArray &array);
    bool renderJson(const QVariantMap &map);
    bool renderJson(const QVariantList &list);
    bool renderJson(const QStringList &list);
    bool renderAndCache(const QByteArray &key, int seconds, const QString &action = QString(), const QString &layout = QString());
    bool renderOnCache(const QByteArray &key);
    void removeCache(const QByteArray &key);
#if QT_VERSION >= 0x050c00  // 5.12.0
    bool renderCbor(const QVariant &variant, QCborValue::EncodingOptions opt = QCborValue::NoTransformation);
    bool renderCbor(const QVariantMap &map, QCborValue::EncodingOptions opt = QCborValue::NoTransformation);
    bool renderCbor(const QVariantHash &hash, QCborValue::EncodingOptions opt = QCborValue::NoTransformation);
    bool renderCbor(const QCborValue &value, QCborValue::EncodingOptions opt = QCborValue::NoTransformation);
    bool renderCbor(const QCborMap &map, QCborValue::EncodingOptions opt = QCborValue::NoTransformation);
    bool renderCbor(const QCborArray &array, QCborValue::EncodingOptions opt = QCborValue::NoTransformation);
#endif
    bool renderErrorResponse(int statusCode);
    void redirect(const QUrl &url, int statusCode = Tf::Found);
    bool sendFile(const QString &filePath, const QByteArray &contentType, const QString &name = QString(), bool autoRemove = false);
    bool sendData(const QByteArray &data, const QByteArray &contentType, const QString &name = QString());
    void rollbackTransaction() { rollback = true; }
    void setAutoRemove(const QString &filePath);
    bool validateAccess(const TAbstractUser *user);
    int socketId() const { return sockId; }
    // For WebSocket
    void sendTextToWebSocket(int socketId, const QString &text);
    void sendBinaryToWebSocket(int socketId, const QByteArray &binary);
    void closeWebSokcet(int socketId, int closeCode = Tf::NormalClosure);
    void publish(const QString &topic, const QString &text);
    void publish(const QString &topic, const QByteArray &binary);

    virtual bool userLogin(const TAbstractUser *user);
    virtual void userLogout();
    virtual void setAccessRules() {}

    THttpRequest &httpRequest();
    THttpResponse &httpResponse() { return response; }

private:
    enum WebSocketTaskType {
        SendTextTo,
        SendBinaryTo,
        SendCloseTo,
        PublishText,
        PublishBinary,
    };

    void setActionName(const QString &name);
    void setArguments(const QStringList &arguments) { args = arguments; }
    void setSocketId(int socketId) { sockId = socketId; }
    bool verifyRequest(const THttpRequest &request) const;
    QByteArray renderView(TActionView *view);
    void exportAllFlashVariants();
    const TActionController *controller() const { return this; }
    bool rollbackRequested() const { return rollback; }
    static QString layoutClassName(const QString &layout);
    static QString partialViewClassName(const QString &partial);

    mutable QString ctrlName;
    QString actName;
    QStringList args;
    int statCode {Tf::OK};  // 200 OK
    bool rendered {false};
    bool layoutEnable {true};
    QString layoutName;
    THttpResponse response;
    QVariantMap flashVars;
    TSession sessionStore;
    TCookieJar cookieJar;
    bool rollback {false};
    QStringList autoRemoveFiles;
    QList<QPair<int, QVariant>> taskList;
    int sockId {0};

    friend class TActionContext;
    friend class TSessionCookieStore;
    friend class TDirectView;
    T_DISABLE_COPY(TActionController)
    T_DISABLE_MOVE(TActionController)
};


inline QString TActionController::className() const
{
    return QString(metaObject()->className());
}

inline QString TActionController::activeAction() const
{
    return actName;
}

inline QStringList TActionController::arguments() const
{
    return args;
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

inline QByteArray TActionController::contentType() const
{
    return response.header().contentType();
}

inline void TActionController::setContentType(const QByteArray &type)
{
    response.header().setContentType(type);
}

