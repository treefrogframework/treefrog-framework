#ifndef TSESSIONSQLOBJECTSTORE_H
#define TSESSIONSQLOBJECTSTORE_H

#include <TSessionStore>


class TSessionSqlObjectStore : public TSessionStore
{
public:
    QString key() const { return "sqlobject"; }
    TSession find(const QByteArray &id) override;
    bool store(TSession &session) override;
    bool remove(const QByteArray &id) override;
    int gc(const QDateTime &expire) override;
};

#endif // TSESSIONSQLOBJECTSTORE_H
