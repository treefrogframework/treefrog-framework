#pragma once
#include <TSessionStore>


class TSessionMongoStore : public TSessionStore {
public:
    QString key() const { return "mongodb"; }
    TSession find(const QByteArray &id) override;
    bool store(TSession &session) override;
    bool remove(const QByteArray &id) override;
    int gc(const QDateTime &expire) override;
};

