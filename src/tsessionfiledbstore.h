#ifndef TSESSIONDBFILESTORE_H
#define TSESSIONDBFILESTORE_H

#include <TSessionStore>

class TCacheSQLiteStore;


class T_CORE_EXPORT TSessionFileDbStore : public TSessionStore
{
public:
    TSessionFileDbStore();
    ~TSessionFileDbStore();

    QString key() const { return QStringLiteral("cachedb"); }
    TSession find(const QByteArray &id) override;
    bool store(TSession &session) override;
    bool remove(const QByteArray &id) override;
    int gc(const QDateTime &expire) override;

private:
    TCacheSQLiteStore *_store {nullptr};
};

#endif // TSESSIONDBFILESTORE_H
