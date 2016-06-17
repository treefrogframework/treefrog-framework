#ifndef TSESSIONSTORE_H
#define TSESSIONSTORE_H

#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <TSession>


class T_CORE_EXPORT TSessionStore
{
public:
    TSessionStore() : lifeTimeSecs(0) { }
    virtual ~TSessionStore() { }

#if QT_VERSION < 0x050000
    virtual QString key() const = 0;
#endif
    void setLifeTime(int seconds) { lifeTimeSecs = seconds; }
    virtual TSession find(const QByteArray &id) = 0;
    virtual bool store(TSession &sesion) = 0;
    virtual bool remove(const QByteArray &id) = 0;
    virtual int gc(const QDateTime &expire) = 0;

protected:
    int lifeTimeSecs;
};

#endif // TSESSIONSTORE_H
