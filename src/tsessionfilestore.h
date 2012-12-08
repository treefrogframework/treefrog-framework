#ifndef TSESSIONFILESTORE_H
#define TSESSIONFILESTORE_H

#include <TSessionStore>


class T_CORE_EXPORT TSessionFileStore : public TSessionStore
{
public:
    QString key() const { return "file"; }
    TSession find(const QByteArray &id, const QDateTime &modified);
    bool store(TSession &session);
    bool remove(const QDateTime &garbageExpiration);
    bool remove(const QByteArray &id);

    static QString sessionDirPath();
};

#endif // TSESSIONFILESTORE_H
