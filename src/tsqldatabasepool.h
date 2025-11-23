#pragma once
#include <TAtomic>
#include <TGlobal>
#include <TSqlDatabase>
#include <TStack>
#include <QBasicTimer>
#include <QDateTime>
#include <QStack>
#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <memory>
#include <vector>
#include <deque>


class T_CORE_EXPORT TSqlDatabasePool : public QObject {
    Q_OBJECT
public:
    using SqlDbPtr = std::unique_ptr<TSqlDatabase>;

    ~TSqlDatabasePool();
    TSqlDatabase::Handle database(int databaseId = 0);

    static TSqlDatabasePool *instance();
    static bool setDatabaseSettings(TSqlDatabase &database, int databaseId);
    static int getDatabaseId(const QSqlDatabase &database);
    static int databaseIdFromName(const QString &name);

protected:
    void init();
    void timerEvent(QTimerEvent *event);

private:
    bool openDatabase(TSqlDatabase &database);
    void closeDatabase(TSqlDatabase &database);
    void pool(SqlDbPtr dbptr, bool forceClose = false);
    TSqlDatabasePool();

    //mutable QRecursiveMutex _mutex;
    TAtomic<uint> *lastCachedTime {nullptr};
    int maxConnects {0};
    QBasicTimer timer;
    // std::vector<MoveStack<SqlDbPtr>> availableDatabases;
    // std::vector<MoveStack<SqlDbPtr>> cachedDatabases;
    std::vector<TStack<SqlDbPtr>> availableDatabases;
    std::vector<TStack<SqlDbPtr>> cachedDatabases;

    T_DISABLE_COPY(TSqlDatabasePool)
    T_DISABLE_MOVE(TSqlDatabasePool)
    friend class TSqlDatabase;
};
