#pragma once
#include <TGlobal>
#include <TSystemGlobal>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QStringList>


class TSqlDriverExtension;


class T_CORE_EXPORT TSqlDatabase {
public:
    using SqlDbPtr = std::unique_ptr<TSqlDatabase>;

    enum DbmsType {
        UnknownDbms = QSqlDriver::UnknownDbms,
        MSSqlServer = QSqlDriver::MSSqlServer,
        MySqlServer = QSqlDriver::MySqlServer,
        PostgreSQL = QSqlDriver::PostgreSQL,
        Oracle = QSqlDriver::Oracle,
        Sybase = QSqlDriver::Sybase,
        SQLite = QSqlDriver::SQLite,
        Interbase = QSqlDriver::Interbase,
        DB2 = QSqlDriver::DB2
    };

    // TKvsDatabase handle
    class Handle {
    public:
        Handle() = default;
        Handle(SqlDbPtr ptr) : _dbptr(std::move(ptr)) {}
        Handle(const Handle &) = delete;
        Handle &operator=(const Handle &) = delete;
        Handle(Handle &&other) noexcept = default;
        Handle &operator=(Handle &&other) noexcept;
        ~Handle();

        TSqlDatabase *operator->() { return _dbptr.get(); }
        TSqlDatabase &operator*() { return *(_dbptr.get()); }
        explicit operator bool() const noexcept { return static_cast<bool>(_dbptr); }

    private:
        SqlDbPtr _dbptr;
    };


    ~TSqlDatabase();
    TSqlDatabase(TSqlDatabase &&other);
    TSqlDatabase &operator=(TSqlDatabase &&other);

    DbmsType dbmsType() const;
    bool isValid() const { return _sqlDatabase.isValid(); }
    QString connectionName() const { return _sqlDatabase.connectionName(); }
    const QSqlDatabase &sqlDatabase() const { return _sqlDatabase; }
    QSqlDatabase &sqlDatabase() { return _sqlDatabase; }
    QStringList postOpenStatements() const { return _postOpenStatements; }
    void setPostOpenStatements(const QStringList &statements) { _postOpenStatements = statements; }
    bool isUpsertEnabled() const { return _enableUpsert; }
    void setUpsertEnabled(bool enable) { _enableUpsert = enable; }
    bool isUpsertSupported() const;
    bool isPreparedStatementSupported() const;
    const TSqlDriverExtension *driverExtension() const { return _driverExtension.get(); }

    static const char *const defaultConnection;
    static std::unique_ptr<TSqlDatabase> database(const QString &connectionName = QLatin1String(defaultConnection));
    static bool addDatabase(const QString &driver, const QString &connectionName = QLatin1String(defaultConnection));
    static void removeDatabase(const QString &connectionName = QLatin1String(defaultConnection));
    static bool contains(const QString &connectionName = QLatin1String(defaultConnection));

private:
    explicit TSqlDatabase(const QSqlDatabase &database = QSqlDatabase());
    TSqlDriverExtension *driverExtension() { return _driverExtension.get(); }
    void setDriverExtension(std::unique_ptr<TSqlDriverExtension> extension);

    QSqlDatabase _sqlDatabase;
    QStringList _postOpenStatements;
    bool _enableUpsert {false};
    std::unique_ptr<TSqlDriverExtension> _driverExtension;

    friend class TSqlDatabasePool;
    friend class TSqlObject;
    T_DISABLE_COPY(TSqlDatabase)
};
