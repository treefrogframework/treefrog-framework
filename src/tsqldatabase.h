#pragma once
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QStringList>
#include <TGlobal>

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
        Handle(SqlDbPtr conn) : _conn(std::move(conn)) {}
        Handle(const Handle &) = delete;
        Handle &operator=(const Handle &) = delete;
        Handle(Handle &&other) noexcept = default;
        Handle &operator=(Handle &&other) noexcept = default;
        ~Handle();

        TSqlDatabase *operator->() { return _conn.get(); }
        TSqlDatabase &operator*() { return *(_conn.get()); }
        explicit operator bool() const noexcept { return (bool)_conn; }

    private:
        SqlDbPtr _conn;
    };


    TSqlDatabase(TSqlDatabase &&other);
    ~TSqlDatabase() { }
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
    const TSqlDriverExtension *driverExtension() const { return _driverExtension; }

    static const char *const defaultConnection;
    static std::unique_ptr<TSqlDatabase> database(const QString &connectionName = QLatin1String(defaultConnection));
    static bool addDatabase(const QString &driver, const QString &connectionName = QLatin1String(defaultConnection));
    static void removeDatabase(const QString &connectionName = QLatin1String(defaultConnection));
    static bool contains(const QString &connectionName = QLatin1String(defaultConnection));

private:
    explicit TSqlDatabase(const QSqlDatabase &database = QSqlDatabase());
    TSqlDriverExtension *driverExtension() { return _driverExtension; }
    void setDriverExtension(TSqlDriverExtension *extension);

    QSqlDatabase _sqlDatabase;
    QStringList _postOpenStatements;
    bool _enableUpsert {false};
    TSqlDriverExtension *_driverExtension {nullptr};

    friend class TSqlDatabasePool;
    friend class TSqlObject;
    T_DISABLE_COPY(TSqlDatabase)
};


inline TSqlDatabase::TSqlDatabase(const QSqlDatabase &database) :
    _sqlDatabase(database)
{
}

// inline TSqlDatabase::TSqlDatabase(const TSqlDatabase &other) :
//     _sqlDatabase(other._sqlDatabase),
//     _postOpenStatements(other._postOpenStatements),
//     _enableUpsert(other._enableUpsert),
//     _driverExtension(other._driverExtension)
// {
// }

inline TSqlDatabase::TSqlDatabase(TSqlDatabase &&other)
{
    *this = std::move(other);
}

// inline TSqlDatabase &TSqlDatabase::operator=(const TSqlDatabase &other)
// {
//     _sqlDatabase = other._sqlDatabase;
//     _postOpenStatements = other._postOpenStatements;
//     _enableUpsert = other._enableUpsert;
//     _driverExtension = other._driverExtension;
//     return *this;
// }

inline TSqlDatabase &TSqlDatabase::operator=(TSqlDatabase &&other)
{
    if (this != &other) {
        _sqlDatabase = other._sqlDatabase;
        _postOpenStatements = other._postOpenStatements;
        _enableUpsert = other._enableUpsert;
        _driverExtension = other._driverExtension;
        other._driverExtension = nullptr;
    }
    return *this;
}
