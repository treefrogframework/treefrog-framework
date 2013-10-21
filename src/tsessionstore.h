#ifndef TSESSIONSTORE_H
#define TSESSIONSTORE_H

#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <TSession>


class T_CORE_EXPORT TSessionStore
{
public:
    virtual ~TSessionStore() { }
#if QT_VERSION < 0x050000
    virtual QString key() const = 0;
#endif
    virtual TSession find(const QByteArray &id, const QDateTime &expiration) = 0;
    virtual bool store(TSession &sesion) = 0;
    virtual bool remove(const QDateTime &expiration) = 0;
    virtual bool remove(const QByteArray &id) = 0;
};

#endif // TSESSIONSTORE_H
