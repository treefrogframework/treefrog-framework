#ifndef TSESSIONSQLOBJECTSTORE_H
#define TSESSIONSQLOBJECTSTORE_H

#include <TSessionStore>


class TSessionSqlObjectStore : public TSessionStore
{
public:
    QString key() const { return "sqlobject"; }
    TSession find(const QByteArray &id);
    bool store(TSession &session);
    bool remove(const QByteArray &id);
    int gc(const QDateTime &expire);
};

#endif // TSESSIONSQLOBJECTSTORE_H
