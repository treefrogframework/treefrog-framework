#ifndef TSESSION_H
#define TSESSION_H

#include <QVariant>
#include <QByteArray>
#include <TGlobal>


class T_CORE_EXPORT TSession : public QVariantHash
{
public:
    TSession(const QByteArray &id = QByteArray());
    TSession(const TSession &);
    TSession &operator=(const TSession &);

    QByteArray id() const { return sessionId; }
    void reset();
    iterator insert(const QString &key, const QVariant &value);
    const QVariant value(const QString &key) const;
    const QVariant value(const QString &key, const QVariant &defaultValue) const;

    static QByteArray sessionName();

private:
    QByteArray sessionId;

    void clear() {} // disabled
    friend class TSessionCookieStore;
    friend class TActionContext;
};


inline TSession::TSession(const QByteArray &id)
    : sessionId(id)
{ }

inline TSession::TSession(const TSession &session)
    : QVariantHash(*static_cast<const QVariantHash *>(&session)), sessionId(session.sessionId)
{ }

inline TSession &TSession::operator=(const TSession &session)
{
    QVariantHash::operator=(*static_cast<const QVariantHash *>(&session));
    sessionId = session.sessionId;
    return *this;
}

inline TSession::iterator TSession::insert(const QString &key, const QVariant &value)
{
    return QVariantHash::insert(key, value);
}

inline const QVariant TSession::value(const QString &key) const
{
    return QVariantHash::value(key);
}

inline const QVariant TSession::value(const QString &key, const QVariant &defaultValue) const
{
    return QVariantHash::value(key, defaultValue);
}

#endif // TSESSION_H
