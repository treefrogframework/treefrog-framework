#pragma once
#include <TSessionStore>


class TSessionMemcachedStore : public TSessionStore {
public:
    QString key() const { return "memcached"; }
    TSession find(const QByteArray &id) override;
    bool store(TSession &session) override;
    bool remove(const QByteArray &id) override;
    int gc(const QDateTime &expire) override;
};
