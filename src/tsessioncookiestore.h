#ifndef TSESSIONCOOKIESTORE_H
#define TSESSIONCOOKIESTORE_H

#include <TSessionStore>


class TSessionCookieStore : public TSessionStore
{
public:
    QString key() const { return "cookie"; }
    TSession find(const QByteArray &id);
    bool store(TSession &session);
    bool remove(const QByteArray &id);
    int gc(const QDateTime &expire);
};

#endif // TSESSIONCOOKIESTORE_H
