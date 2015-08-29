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
        Redis,
    };

    TKvsDatabase();
    TKvsDatabase(const TKvsDatabase &other);
    ~TKvsDatabase() { }
    TKvsDatabase &operator=(const TKvsDatabase &other);

    QString driverName() const;
    QString connectionName() const { return connectName; }
    QString databaseName() const;
    void setDatabaseName(const QString &name);
    QString hostName() const;
    void setHostName(const QString &hostName);
    int port() const;
    void setPort(int port);
    QString userName() const;
    void setUserName(const QString &userName);
    QString password() const;
    void setPassword(const QString &password);
    QString connectOptions() const;
    void setConnectOptions(const QString &options = QString());

    bool open();
    bool isOpen() const;
    void close();
    bool isValid() const;
    TKvsDriver *driver() { return drv; }
    const TKvsDriver *driver() const { return drv; }

    static const char *const defaultConnection;
    static TKvsDatabase database(const QString &connectionName = QLatin1String(defaultConnection));
    static TKvsDatabase addDatabase(const QString &driver, const QString &connectionName = QLatin1String(defaultConnection));
    static void removeDatabase(const QString &connectionName = QLatin1String(defaultConnection));
    static void removeAllDatabases();
    static bool contains(const QString &connectionName = QLatin1String(defaultConnection));

private:
    QString connectName;
    TKvsDriver *drv;

    TKvsDatabase(const QString &connectionName, TKvsDriver *driver);
};

#endif // TKVSDATABASE_H
