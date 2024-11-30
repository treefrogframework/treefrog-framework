#pragma once
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QStringList>
#include <TGlobal>

class TSqlDriverExtension;


class T_CORE_EXPORT TSqlDatabase {
public:
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

    explicit TSqlDatabase(const QSqlDatabase &database = QSqlDatabase());
    TSqlDatabase(const TSqlDatabase &other);
    ~TSqlDatabase() { }
    TSqlDatabase &operator=(const TSqlDatabase &other);

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
    static TSqlDatabase &database(const QString &connectionName = QLatin1String(defaultConnection));
    static TSqlDatabase &addDatabase(const QString &driver, const QString &connectionName = QLatin1String(defaultConnection));
    static void removeDatabase(const QString &connectionName = QLatin1String(defaultConnection));
    static bool contains(const QString &connectionName = QLatin1String(defaultConnection));

private:
    TSqlDriverExtension *driverExtension() { return _driverExtension; }
    void setDriverExtension(TSqlDriverExtension *extension);

    QSqlDatabase _sqlDatabase;
    QStringList _postOpenStatements;
    bool _enableUpsert {false};
    TSqlDriverExtension *_driverExtension {nullptr};

    friend class TSqlDatabasePool;
};


inline TSqlDatabase::TSqlDatabase(const QSqlDatabase &database) :
    _sqlDatabase(database)
{
}

inline TSqlDatabase::TSqlDatabase(const TSqlDatabase &other) :
    _sqlDatabase(other._sqlDatabase),
    _postOpenStatements(other._postOpenStatements),
    _enableUpsert(other._enableUpsert),
    _driverExtension(other._driverExtension)
{
}

inline TSqlDatabase &TSqlDatabase::operator=(const TSqlDatabase &other)
{
    _sqlDatabase = other._sqlDatabase;
    _postOpenStatements = other._postOpenStatements;
    _enableUpsert = other._enableUpsert;
    _driverExtension = other._driverExtension;
    return *this;
}

