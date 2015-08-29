#ifndef TSESSIONREDISSTORE_H
#define TSESSIONREDISSTORE_H

#include <TSessionStore>


class TSessionRedisStore : public TSessionStore
{
public:
    QString key() const { return "redis"; }
    TSession find(const QByteArray &id);
    bool store(TSession &session);
    bool remove(const QByteArray &id);
    int gc(const QDateTime &expire);
};

#endif // TSESSIONREDISSTORE_H
