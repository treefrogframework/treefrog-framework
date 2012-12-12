#ifndef TACTIONHELPER_H
#define TACTIONHELPER_H

#include <QStringList>
#include <QByteArray>
#include <QUrl>
#include <QPair>
#include <QVariant>
#include <TGlobal>

class TActionController;


class T_CORE_EXPORT TActionHelper
{
public:
    virtual ~TActionHelper() { }
    QUrl url(const QString &controller = QString(), const QString &action = QString(),
             const QStringList &args = QStringList(),
             const QVariantHash &query = QVariantHash()) const;
    QUrl url(const QString &controller, const QString &action, int arg) const;
    QUrl url(const QString &controller, const QString &action, uint arg) const;
    QUrl url(const QString &controller, const QString &action, qint64 arg) const;
    QUrl url(const QString &controller, const QString &action, quint64 arg) const;
    QUrl url(const QString &controller, const QString &action, const QString &arg) const;
    QUrl url(const QString &controller, const QString &action, const QVariant &arg) const;
    QUrl url(const QString &controller, const QString &action, const QVariantHash &query) const;
    
    QUrl urla(const QString &action = QString(), const QStringList &args = QStringList(),
              const QVariantHash &query = QVariantHash()) const;
    QUrl urla(const QString &action, int arg) const;
    QUrl urla(const QString &action, uint arg) const;
    QUrl urla(const QString &action, qint64 arg) const;
    QUrl urla(const QString &action, quint64 arg) const;
    QUrl urla(const QString &action, const QString &arg) const;
    QUrl urla(const QString &action, const QVariant &arg) const;
    QUrl urla(const QString &action, const QVariantHash &query) const;

protected:
    virtual const TActionController *controller() const = 0;
};


inline QUrl TActionHelper::url(const QString &controller, const QString &action, int arg) const
{
    return url(controller, action, QStringList(QString::number(arg)));
}

inline QUrl TActionHelper::url(const QString &controller, const QString &action, uint arg) const
{
    return url(controller, action, QStringList(QString::number(arg)));
}

inline QUrl TActionHelper::url(const QString &controller, const QString &action, qint64 arg) const
{
    return url(controller, action, QStringList(QString::number(arg)));
}

inline QUrl TActionHelper::url(const QString &controller, const QString &action, quint64 arg) const
{
    return url(controller, action, QStringList(QString::number(arg)));
}

inline QUrl TActionHelper::url(const QString &controller, const QString &action, const QString &arg) const
{
    return url(controller, action, QStringList(arg));
}

inline QUrl TActionHelper::url(const QString &controller, const QString &action, const QVariantHash &query) const
{
    return url(controller, action, QStringList(), query);
}

inline QUrl TActionHelper::urla(const QString &action, const QStringList &args, const QVariantHash &query) const
{
    return url(QString(), action, args, query);
}

inline QUrl TActionHelper::urla(const QString &action, const QString &arg) const
{
    return url(QString(), action, QStringList(arg));
}

inline QUrl TActionHelper::urla(const QString &action, int arg) const
{
    return url(QString(), action, QStringList(QString::number(arg)));
}

inline QUrl TActionHelper::urla(const QString &action, uint arg) const
{
    return url(QString(), action, QStringList(QString::number(arg)));
}

inline QUrl TActionHelper::urla(const QString &action, qint64 arg) const
{
    return url(QString(), action, QStringList(QString::number(arg)));
}

inline QUrl TActionHelper::urla(const QString &action, quint64 arg) const
{
    return url(QString(), action, QStringList(QString::number(arg)));
}

inline QUrl TActionHelper::urla(const QString &action, const QVariantHash &query) const
{
    return url(QString(), action, QStringList(), query);
}

#endif // TACTIONHELPER_H
