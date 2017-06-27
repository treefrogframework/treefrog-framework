#ifndef TSESSIONMONGOOBJECTSTORE_H
#define TSESSIONMONGOOBJECTSTORE_H

#include <TSessionStore>


class TSessionMongoObjectStore : public TSessionStore
{
public:
    QString key() const { return "mongoobject"; }
    TSession find(const QByteArray &id) override;
    bool store(TSession &session) override;
    bool remove(const QByteArray &id) override;
    int gc(const QDateTime &expire) override;
};

#endif // TSESSIONMONGOOBJECTSTORE_H
