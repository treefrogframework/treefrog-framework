#pragma once
#include <QDateTime>
#include <TGlobal>
#include <TSession>


class T_CORE_EXPORT TSessionManager {
public:
    ~TSessionManager();

    TSession findSession(const QByteArray &id);
    bool store(TSession &session);
    bool remove(const QByteArray &id);
    QString storeType() const;
    QByteArray generateId();
    void collectGarbage();

    static TSessionManager &instance();
    static int sessionLifeTime();

private:
    T_DISABLE_COPY(TSessionManager)
    T_DISABLE_MOVE(TSessionManager)
    TSessionManager();
};

