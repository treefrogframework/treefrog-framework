#ifndef TKVSDATABASE_H
#define TKVSDATABASE_H

#include <QString>
#include <TGlobal>

class TKvsDriver;


class T_CORE_EXPORT TKvsDatabase
{
public:
    enum Type {
        MongoDB = 0,
    };

    TKvsDatabase();
    TKvsDatabase(const TKvsDatabase &other);
    ~TKvsDatabase() { }
    TKvsDatabase &operator=(const TKvsDatabase &other);

    QString driverName() const { return drvName; }
    QString connectionName() const { return connectName; }
    QString databaseName() const { return dbName; }
    void setDatabaseName(const QString &name) { dbName = name; }
    QString hostName() const { return host; }
    void setHostName(const QString &hostName) { host = hostName; }
    int port() const { return portNumber; }
    void setPort(int port) { portNumber = port; }
    QString userName() const { return user; }
    void setUserName(const QString &userName) { user = userName; }
    QString password() const { return pass; }
    void setPassword(const QString &password) { pass = password; }
    QString connectOptions() const { return opts; }
    void setConnectOptions(const QString &options = QString()) { opts = options; }

    bool open();
    bool isOpen() const;
    void close();
    bool isValid() const;
    TKvsDriver *driver() { return drv; }
    const TKvsDriver *driver() const { return drv; }

    static const char *const defaultConnection;
    static TKvsDatabase &database(const QString &connectionName = QLatin1String(defaultConnection));
    static TKvsDatabase &addDatabase(const QString &driver, const QString &connectionName = QLatin1String(defaultConnection));
    static void removeDatabase(const QString &connectionName = QLatin1String(defaultConnection));
    static void removeAllDatabases();
    static bool contains(const QString &connectionName = QLatin1String(defaultConnection));

protected:
    void createDriver(const QString &driverName);
    static TKvsDatabase &addDatabase(const TKvsDatabase &db, const QString &connectionName);

private:
    QString connectName;
    QString dbName;
    QString host;
    quint16 portNumber;
    QString user;
    QString pass;
    QString opts;
    QString drvName;
    TKvsDriver *drv;  // pointer to a singleton object
};

#endif // TKVSDATABASE_H
