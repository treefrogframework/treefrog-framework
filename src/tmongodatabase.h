#ifndef TMONGODATABASE_H
#define TMONGODATABASE_H

#include <QString>
#include <TGlobal>

class TMongoDriver;


class T_CORE_EXPORT TMongoDatabase
{
public:
    TMongoDatabase();
    TMongoDatabase(const TMongoDatabase &other)
        : host(other.host), d(other.d) { }
    ~TMongoDatabase() { }

    bool isValid () const;
    bool open();
    void close();
    QString hostName() const { return host; }
    void setHostName(const QString &hostName) { host = hostName; }
    bool isOpen() const;
    TMongoDriver *driver();
    const TMongoDriver *driver() const;
    TMongoDatabase &operator=(const TMongoDatabase &other);

    static TMongoDatabase addDatabase(const QString &host, const QString &connectionName = QString());
    static void removeDatabase(const QString &connectionName);
    static TMongoDatabase database(const QString &connectionName = QString());
    
private:
    QString host;
    TMongoDriver* d;  // pointer to a singleton object
};

#endif // TMONGODATABASE_H
