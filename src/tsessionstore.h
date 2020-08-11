#pragma once
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <TSession>


class T_CORE_EXPORT TSessionStore {
public:
    TSessionStore() { }
    virtual ~TSessionStore() { }
    virtual TSession find(const QByteArray &id) = 0;
    virtual bool store(TSession &sesion) = 0;
    virtual bool remove(const QByteArray &id) = 0;
    virtual int gc(const QDateTime &expire) = 0;

    static qint64 lifeTimeSecs();
};

