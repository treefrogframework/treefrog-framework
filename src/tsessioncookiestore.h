#ifndef TSESSIONCOOKIESTORE_H
#define TSESSIONCOOKIESTORE_H

#include <TSessionStore>


class TSessionCookieStore : public TSessionStore
{
public:
    QString key() const { return "cookie"; }
    TSession find(const QByteArray &id, const QDateTime &modified);
    bool store(TSession &session);
    bool remove(const QDateTime &garbageExpiration);
    bool remove(const QByteArray &id);
};

#endif // TSESSIONCOOKIESTORE_H
