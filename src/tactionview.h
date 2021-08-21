#pragma once
#include <QObject>
#include <QTextStream>
#include <QVariant>
#include <TActionHelper>
#include <TGlobal>
#include <THttpRequest>
#include <THttpUtility>
#include <TPrototypeAjaxHelper>
#include <TViewHelper>

class TAbstractController;


class T_CORE_EXPORT TActionView : public QObject, public TActionHelper, public TViewHelper, public TPrototypeAjaxHelper {
public:
    TActionView();
    virtual ~TActionView() { }

    virtual QString toString() = 0;
    QString yield() const;
    QString renderPartial(const QString &templateName, const QVariantMap &vars = QVariantMap()) const;
    QString authenticityToken() const;
    QVariant variant(const QString &name) const;
    bool hasVariant(const QString &name) const;
    const QVariantMap &allVariants() const;
    const TAbstractController *controller() const override;
    const THttpRequest &httpRequest() const;

protected:
    QString echo(const QString &str);
    QString echo(const char *str);
    QString echo(const QByteArray &str);
    QString echo(int n, int base = 10);
    QString echo(long n, int base = 10);
    QString echo(ulong n, int base = 10);
    QString echo(qlonglong n, int base = 10);
    QString echo(qulonglong n, int base = 10);
    QString echo(double d, char format = 'g', int precision = 6);
    QString echo(const QJsonObject &object);
    QString echo(const QJsonArray &array);
    QString echo(const QJsonDocument &doc);
    QString echo(const THtmlAttribute &attr);
    QString echo(const QVariant &var);
    QString echo(const QVariantMap &map);
    QString eh(const QString &str);
    QString eh(const char *str);
    QString eh(const QByteArray &str);
    QString eh(int n, int base = 10);
    QString eh(long n, int base = 10);
    QString eh(ulong n, int base = 10);
    QString eh(qlonglong n, int base = 10);
    QString eh(qulonglong n, int base = 10);
    QString eh(double d, char format = 'g', int precision = 6);
    QString eh(const QJsonObject &object);
    QString eh(const QJsonArray &array);
    QString eh(const QJsonDocument &doc);
    QString eh(const THtmlAttribute &attr);
    QString eh(const QVariant &var);
    QString eh(const QVariantMap &map);
    QString renderReact(const QString &component);
    QString responsebody;

private:
    T_DISABLE_COPY(TActionView)
    T_DISABLE_MOVE(TActionView)

    void setVariantMap(const QVariantMap &vars);
    void setController(TAbstractController *controller);
    void setSubActionView(TActionView *actionView);
    virtual const TActionView *actionView() const override { return this; }

    TAbstractController *actionController {nullptr};
    TActionView *subView {nullptr};
    QVariantMap variantMap;

    friend class TActionController;
    friend class TActionMailer;
    friend class TDirectView;
};


inline void TActionView::setSubActionView(TActionView *actionView)
{
    subView = actionView;
}

inline const TAbstractController *TActionView::controller() const
{
    return actionController;
}

inline void TActionView::setVariantMap(const QVariantMap &vars)
{
    variantMap = vars;
}

inline QVariant TActionView::variant(const QString &name) const
{
    return variantMap.value(name);
}

inline bool TActionView::hasVariant(const QString &name) const
{
    return variantMap.contains(name);
}

inline const QVariantMap &TActionView::allVariants() const
{
    return variantMap;
}

inline QString TActionView::echo(const QString &str)
{
    responsebody += str;
    return QString();
}

inline QString TActionView::echo(const char *str)
{
    responsebody += QString(str);  // using codecForCStrings()
    return QString();
}

inline QString TActionView::echo(const QByteArray &str)
{
    responsebody += QString(str);  // using codecForCStrings()
    return QString();
}

inline QString TActionView::echo(int n, int base)
{
    responsebody += QString::number(n, base);
    return QString();
}

inline QString TActionView::echo(long n, int base)
{
    responsebody += QString::number(n, base);
    return QString();
}

inline QString TActionView::echo(ulong n, int base)
{
    responsebody += QString::number(n, base);
    return QString();
}

inline QString TActionView::echo(qlonglong n, int base)
{
    responsebody += QString::number(n, base);
    return QString();
}

inline QString TActionView::echo(qulonglong n, int base)
{
    responsebody += QString::number(n, base);
    return QString();
}

inline QString TActionView::echo(double d, char format, int precision)
{
    responsebody += QString::number(d, format, precision);
    return QString();
}

inline QString TActionView::echo(const QJsonObject &object)
{
    return echo(QJsonDocument(object));
}

inline QString TActionView::echo(const QJsonArray &array)
{
    return echo(QJsonDocument(array));
}

inline QString TActionView::echo(const QJsonDocument &doc)
{
    responsebody += doc.toJson(QJsonDocument::Compact);
    return QString();
}

inline QString TActionView::eh(const QString &str)
{
    return echo(THttpUtility::htmlEscape(str));
}

inline QString TActionView::eh(const char *str)
{
    return echo(THttpUtility::htmlEscape(str));
}

inline QString TActionView::eh(const QByteArray &str)
{
    return echo(THttpUtility::htmlEscape(str));
}

inline QString TActionView::eh(int n, int base)
{
    return echo(THttpUtility::htmlEscape(QString::number(n, base)));
}

inline QString TActionView::eh(long n, int base)
{
    return echo(THttpUtility::htmlEscape(QString::number(n, base)));
}

inline QString TActionView::eh(ulong n, int base)
{
    return echo(THttpUtility::htmlEscape(QString::number(n, base)));
}

inline QString TActionView::eh(qlonglong n, int base)
{
    return echo(THttpUtility::htmlEscape(QString::number(n, base)));
}

inline QString TActionView::eh(qulonglong n, int base)
{
    return echo(THttpUtility::htmlEscape(QString::number(n, base)));
}

inline QString TActionView::eh(double d, char format, int precision)
{
    return echo(THttpUtility::htmlEscape(QString::number(d, format, precision)));
}

inline QString TActionView::eh(const QJsonObject &object)
{
    return eh(QJsonDocument(object));
}

inline QString TActionView::eh(const QJsonArray &array)
{
    return eh(QJsonDocument(array));
}

inline QString TActionView::eh(const QJsonDocument &doc)
{
    return echo(THttpUtility::htmlEscape(doc.toJson(QJsonDocument::Compact)));
}

inline void TActionView::setController(TAbstractController *controller)
{
    actionController = controller;
}

