#pragma once
#include <QByteArray>
#include <QVariant>
#include <TGlobal>


class T_CORE_EXPORT TSession : public QVariantMap {
public:
    TSession(const QByteArray &id = QByteArray());
    TSession(const TSession &other);
    TSession &operator=(const TSession &other);

    QByteArray id() const { return sessionId; }
    void reset();
    iterator insert(const QString &key, const QVariant &value);
    int remove(const QString &key);
    QVariant take(const QString &key);
    const QVariant value(const QString &key) const;
    const QVariant value(const QString &key, const QVariant &defaultValue) const;
    static QByteArray sessionName();

private:
    QByteArray sessionId;

    void clear();  // disabled
    friend class TSessionCookieStore;
    friend class TActionContext;
};


inline TSession::TSession(const QByteArray &id) :
    sessionId(id)
{
}

inline TSession::TSession(const TSession &other) :
    QVariantMap(*static_cast<const QVariantMap *>(&other)), sessionId(other.sessionId)
{
}

inline TSession &TSession::operator=(const TSession &other)
{
    QVariantMap::operator=(*static_cast<const QVariantMap *>(&other));
    sessionId = other.sessionId;
    return *this;
}

inline TSession::iterator TSession::insert(const QString &key, const QVariant &value)
{
    return QVariantMap::insert(key, value);
}

inline int TSession::remove(const QString &key)
{
    return QVariantMap::remove(key);
}

inline QVariant TSession::take(const QString &key)
{
    return QVariantMap::take(key);
}

inline const QVariant TSession::value(const QString &key) const
{
    return QVariantMap::value(key);
}

inline const QVariant TSession::value(const QString &key, const QVariant &defaultValue) const
{
    return QVariantMap::value(key, defaultValue);
}

