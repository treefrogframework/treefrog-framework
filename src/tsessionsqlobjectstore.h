#ifndef TSESSIONSQLOBJECTSTORE_H
#define TSESSIONSQLOBJECTSTORE_H

#include <TSessionStore>


class TSessionSqlObjectStore : public TSessionStore
{
public:
    QString key() const { return "sqlobject"; }
    TSession find(const QByteArray &id, const QDateTime &modified);
    bool store(TSession &session);
    bool remove(const QDateTime &garbageExpiration);
    bool remove(const QByteArray &id);
};

#endif // TSESSIONSQLOBJECTSTORE_H
