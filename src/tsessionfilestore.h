#ifndef TSESSIONFILESTORE_H
#define TSESSIONFILESTORE_H

#include <TSessionStore>


class T_CORE_EXPORT TSessionFileStore : public TSessionStore
{
public:
    QString key() const { return "file"; }
    TSession find(const QByteArray &id) override;
    bool store(TSession &session) override;
    bool remove(const QByteArray &id) override;
    int gc(const QDateTime &expire) override;

    static QString sessionDirPath();
};

#endif // TSESSIONFILESTORE_H
