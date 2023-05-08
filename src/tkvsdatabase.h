#pragma once
#include <QString>
#include <TGlobal>

class TKvsDriver;
class TKvsDatabaseData;


class T_CORE_EXPORT TKvsDatabase {
public:
    TKvsDatabase() { }
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
    QStringList postOpenStatements() const;
    void setPostOpenStatements(const QStringList &statements);

    bool open();
    void close();
    bool command(const QString &cmd);
    bool isOpen() const;
    bool isValid() const;
    TKvsDriver *driver() { return drv; }
    const TKvsDriver *driver() const { return drv; }
    void moveToThread(QThread *targetThread);

    static const char *const defaultConnection;
    static TKvsDatabase database(const QString &connectionName = defaultConnection);
    static TKvsDatabase addDatabase(const QString &driver, const QString &connectionName = defaultConnection);
    static void removeDatabase(const QString &connectionName = defaultConnection);
    static TKvsDatabaseData settings(const QString &connectionName = defaultConnection);

private:
    QString connectName;
    TKvsDriver *drv {nullptr};

    TKvsDatabase(const QString &connectionName, TKvsDriver *driver);
    TKvsDatabase(const TKvsDatabaseData &data);
};


class T_CORE_EXPORT TKvsDatabaseData {
public:
    QString connectionName;
    QString databaseName;
    QString hostName;
    uint16_t port {0};
    QString userName;
    QString password;
    QString connectOptions;
    QStringList postOpenStatements;
    TKvsDriver *driver {nullptr};  // pointer to a singleton object

    TKvsDatabaseData() { }
};
