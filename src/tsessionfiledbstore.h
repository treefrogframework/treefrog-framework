#ifndef TSESSIONDBFILESTORE_H
#define TSESSIONDBFILESTORE_H

#include <TSessionStore>


class T_CORE_EXPORT TSessionFileDbStore : public TSessionStore
{
public:
    QString key() const { return QStringLiteral("filedb"); }
    TSession find(const QByteArray &id) override;
    bool store(TSession &session) override;
    bool remove(const QByteArray &id) override;
    int gc(const QDateTime &expire) override;
};

#endif // TSESSIONDBFILESTORE_H
