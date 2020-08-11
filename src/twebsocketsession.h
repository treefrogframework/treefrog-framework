#pragma once
#include <QVariant>
#include <TGlobal>

class TSession;


class T_CORE_EXPORT TWebSocketSession : public QVariantMap {
public:
    TWebSocketSession();
    TWebSocketSession(const TWebSocketSession &);
    TWebSocketSession &operator=(const TWebSocketSession &);

    iterator insert(const QString &key, const QVariant &value);
    const QVariant value(const QString &key) const;
    const QVariant value(const QString &key, const QVariant &defaultValue) const;
    TWebSocketSession &unite(const TSession &session);
    void reset();

private:
    void clear() { }  // disabled
};


inline TWebSocketSession::TWebSocketSession() :
    QVariantMap()
{
}

inline TWebSocketSession::TWebSocketSession(const TWebSocketSession &other) :
    QVariantMap(*static_cast<const QVariantMap *>(&other))
{
}

inline TWebSocketSession &TWebSocketSession::operator=(const TWebSocketSession &other)
{
    QVariantMap::operator=(*static_cast<const QVariantMap *>(&other));
    return *this;
}

inline TWebSocketSession::iterator TWebSocketSession::insert(const QString &key, const QVariant &value)
{
    return QVariantMap::insert(key, value);
}

inline const QVariant TWebSocketSession::value(const QString &key) const
{
    return QVariantMap::value(key);
}

inline const QVariant TWebSocketSession::value(const QString &key, const QVariant &defaultValue) const
{
    return QVariantMap::value(key, defaultValue);
}

inline void TWebSocketSession::reset()
{
    QVariantMap::clear();
}

