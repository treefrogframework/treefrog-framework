#pragma once
#include <QString>
#include <TGlobal>
#include <memory>

class TKvsDriver;
class TKvsDatabaseSettings;


class T_CORE_EXPORT TKvsDatabase {
public:
    using KvsDbPtr = std::unique_ptr<TKvsDatabase>;

    class Handle {
    public:
        Handle() = default;
        explicit Handle(KvsDbPtr ptr) : _dbptr(std::move(ptr)) {}
        Handle(const Handle &) = delete;
        Handle &operator=(const Handle &) = delete;
        Handle(Handle &&other) { *this = std::move(other); }
        Handle &operator=(Handle &&other);
        ~Handle();

        TKvsDatabase *operator->() { return _dbptr.get(); }
        TKvsDatabase &operator*() { return *(_dbptr.get()); }
        explicit operator bool() const noexcept { return (bool)_dbptr; }

    private:
        KvsDbPtr _dbptr;
    };


    TKvsDatabase() = default;
    ~TKvsDatabase() { /* do not delete driver pointer */ }
    TKvsDatabase(TKvsDatabase &&other) { *this = std::move(other); }
    TKvsDatabase &operator=(TKvsDatabase &&);

    QString driverName() const;
    QString connectionName() const { return _connectName; }
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
    TKvsDriver *driver() { return _driver; }
    const TKvsDriver *driver() const { return _driver; }
    void moveToThread(QThread *targetThread);

    static const char *const defaultConnection;
    static std::unique_ptr<TKvsDatabase> database(const QString &connectionName = defaultConnection);
    static bool addDatabase(const QString &driver, const QString &connectionName = defaultConnection);
    static void removeDatabase(const QString &connectionName = defaultConnection);
    static TKvsDatabaseSettings settings(const QString &connectionName = defaultConnection);

private:
    QString _connectName;
    TKvsDriver *_driver {nullptr};

    TKvsDatabase(const QString &connectionName, TKvsDriver *driver);
    TKvsDatabase(const TKvsDatabaseSettings &data);
    T_DISABLE_COPY(TKvsDatabase)
};


class T_CORE_EXPORT TKvsDatabaseSettings {
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

    TKvsDatabaseSettings() { }
};
