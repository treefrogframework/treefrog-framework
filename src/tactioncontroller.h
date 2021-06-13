#pragma once
#include <QHostAddress>
#include <QtCore>
#include <TAbstractController>
#include <TAccessValidator>
#include <TActionHelper>
#include <TCookieJar>
#include <THttpRequest>
#include <THttpResponse>
#include <TActionContext>
#include <TSession>
#include <TGlobal>

class TActionView;
class TAbstractUser;
class TFormValidator;
class TCache;
class QDomDocument;


class T_CORE_EXPORT TActionController : public TAbstractController, public TActionHelper, protected TAccessValidator {
public:
    TActionController();
    virtual ~TActionController();

    QString name() const override;
    QString activeAction() const override;
    QStringList arguments() const override;
    const THttpRequest &request() const override;
    const THttpRequest &httpRequest() const override { return request(); }
    const THttpResponse &response() const { return _response; }
    const THttpResponse &httpResponse() const { return response(); }
    const TSession &session() const override { return _sessionStore; }
    QString getRenderingData(const QString &templateName, const QVariantMap &vars = QVariantMap()) override;
    virtual bool sessionEnabled() const { return true; }
    virtual bool csrfProtectionEnabled() const { return true; }
    virtual QStringList exceptionActionsOfCsrfProtection() const { return QStringList(); }
    virtual bool transactionEnabled() const { return true; }
    QByteArray authenticityToken() const override;
    QString flash(const QString &name) const;
    QHostAddress clientAddress() const;
    virtual bool isUserLoggedIn() const;
    virtual QString identityKeyOfLoginUser() const;
    void setFlash(const QString &name, const QVariant &value) override;

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
    int statusCode() const { return _statCode; }
    void setFlashValidationErrors(const TFormValidator &validator, const QString &prefix = QString("err_"));
    TSession &session() override { return _sessionStore; }
    void setSession(const TSession &session);
    bool addCookie(const TCookie &cookie) override;
    bool addCookie(const QByteArray &name, const QByteArray &value, const QDateTime &expire = QDateTime(), const QString &path = QString(), const QString &domain = QString(), bool secure = false, bool httpOnly = false, const QByteArray &sameSite = "Lax") override;
    bool addCookie(const QByteArray &name, const QByteArray &value, qint64 maxAge, const QString &path = QString(), const QString &domain = QString(), bool secure = false, bool httpOnly = false, const QByteArray &sameSite = "Lax") override;
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
    void rollbackTransaction() { _rollback = true; }
    void setAutoRemove(const QString &filePath);
    bool validateAccess(const TAbstractUser *user);
    int socketId() const { return _sockId; }
    // For WebSocket
    void sendTextToWebSocket(int socketId, const QString &text);
    void sendBinaryToWebSocket(int socketId, const QByteArray &binary);
    void closeWebSokcet(int socketId, int closeCode = Tf::NormalClosure);
    void publish(const QString &topic, const QString &text);
    void publish(const QString &topic, const QByteArray &binary);

    virtual bool userLogin(const TAbstractUser *user);
    virtual void userLogout();
    virtual void setAccessRules() {}

    THttpRequest &request();
    THttpRequest &httpRequest() { return request(); }
    THttpResponse &httpResponse() { return _response; }

private:
    enum WebSocketTaskType {
        SendTextTo,
        SendBinaryTo,
        SendCloseTo,
        PublishText,
        PublishBinary,
    };

    void setActionName(const QString &name);
    void setArguments(const QStringList &arguments) { _args = arguments; }
    void setSocketId(int socketId) { _sockId = socketId; }
    bool verifyRequest(const THttpRequest &request) const;
    QByteArray renderView(TActionView *view);
    void exportAllFlashVariants();
    const TActionController *controller() const override { return this; }
    bool rollbackRequested() const { return _rollback; }
    static QString layoutClassName(const QString &layout);
    static QString partialViewClassName(const QString &partial);

    mutable QString _ctrlName;
    QString _actionName;
    QStringList _args;
    int _statCode {Tf::OK};  // 200 OK
    bool _rendered {false};
    bool _layoutEnable {true};
    QString _layoutName;
    THttpResponse _response;
    QVariantMap _flashVars;
    TSession _sessionStore;
    TCookieJar _cookieJar;
    bool _rollback {false};
    QStringList _autoRemoveFiles;
    QList<QPair<int, QVariant>> _taskList;
    int _sockId {0};

    friend class TActionContext;
    friend class TSessionCookieStore;
    T_DISABLE_COPY(TActionController)
    T_DISABLE_MOVE(TActionController)
};


inline QString TActionController::activeAction() const
{
    return _actionName;
}

inline QStringList TActionController::arguments() const
{
    return _args;
}

inline void TActionController::setActionName(const QString &name)
{
    _actionName = name;
}

inline void TActionController::setLayoutEnabled(bool enable)
{
    _layoutEnable = enable;
}

inline void TActionController::setLayoutDisabled(bool disable)
{
    _layoutEnable = !disable;
}

inline bool TActionController::layoutEnabled() const
{
    return _layoutEnable;
}

inline QString TActionController::layout() const
{
    return _layoutName;
}

inline void TActionController::setStatusCode(int code)
{
    _statCode = code;
}

inline QString TActionController::flash(const QString &name) const
{
    return _flashVars.value(name).toString();
}

inline QByteArray TActionController::contentType() const
{
    return _response.header().contentType();
}

inline void TActionController::setContentType(const QByteArray &type)
{
    _response.header().setContentType(type);
}

