#pragma once
#include <QObject>
#include <QVariant>
#include <TGlobal>

class THttpRequest;
class TSession;
class TCookie;
class TFormValidator;
class TActionContext;


class T_CORE_EXPORT TAbstractController : public QObject {
public:
    TAbstractController();
    virtual ~TAbstractController() { }
    virtual QString className() const;
    virtual QString name() const = 0;
    virtual QString activeAction() const = 0;
    virtual QStringList arguments() const { return QStringList(); }
    virtual const THttpRequest &httpRequest() const;
    virtual const THttpRequest &request() const;
    virtual const TSession &session() const;
    virtual QString getRenderingData(const QString &templateName, const QVariantMap &vars = QVariantMap());
    virtual QByteArray authenticityToken() const { return QByteArray(); }
    virtual QVariantMap flashVariants() const { return QVariantMap(); }
    virtual QVariant flashVariant(const QString &) const { return QVariant(); }
    virtual QJsonObject flashVariantsJson() const { return QJsonObject(); }
    virtual QJsonObject flashVariantJson(const QString &) const { return QJsonObject(); }
    virtual void setFlash(const QString &name, const QVariant &value);
    void exportVariant(const QString &name, const QVariant &value, bool overwrite = true);
    virtual bool isUserLoggedIn() const;
    const TActionContext *context() const { return _context; }
    TActionContext *context() { return _context; }
    void setContext(TActionContext *context) { _context = context; }
    static QThread *currentThread() { return QThread::currentThread(); }

    TAbstractController(const TAbstractController &) = delete;
    TAbstractController &operator=(const TAbstractController &) = delete;
    TAbstractController(TAbstractController &&) = delete;
    TAbstractController &operator=(TAbstractController &&) = delete;

protected:
    virtual TSession &session();
    virtual bool addCookie(const TCookie &cookie);
    virtual bool addCookie(const QByteArray &name, const QByteArray &value, const QDateTime &expire = QDateTime(), const QString &path = QString(), const QString &domain = QString(), bool secure = false, bool httpOnly = false, const QByteArray &sameSite = "Lax");
    virtual bool addCookie(const QByteArray &name, const QByteArray &value, int64_t maxAge, const QString &path = QString(), const QString &domain = QString(), bool secure = false, bool httpOnly = false, const QByteArray &sameSite = "Lax");
    virtual void reset() { }

    QVariant variant(const QString &name) const;
    void exportVariants(const QVariantMap &map);
    void exportValidationErrors(const TFormValidator &validator, const QString &prefix = QString("err_"));
    bool hasVariant(const QString &name) const;
    const QVariantMap &allVariants() const { return _exportVars; }
    QString viewClassName(const QString &action = QString()) const;
    QString viewClassName(const QString &contoller, const QString &action) const;

private:
    QVariantMap _exportVars;
    TActionContext *_context {nullptr};

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
    return _exportVars.value(name);
}

inline bool TAbstractController::hasVariant(const QString &name) const
{
    return _exportVars.contains(name);
}
