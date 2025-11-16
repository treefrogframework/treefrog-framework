#pragma once
#include <TAtomic>
#include <TGlobal>
#include <TSqlDatabase>
#include <QBasicTimer>
#include <QDateTime>
#include <QStack>
#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <memory>
#include <vector>
#include <stack>


class T_CORE_EXPORT TSqlDatabasePool : public QObject {
    Q_OBJECT
public:
    using SqlDbPtr = std::unique_ptr<TSqlDatabase>;

    template <class T>
    class MoveStack : public std::deque<T>
    {
    public:
        MoveStack() = default;
        MoveStack(const MoveStack &) = delete;
        MoveStack &operator=(const MoveStack &) = delete;
        MoveStack(MoveStack &&) noexcept = default;
        MoveStack &operator=(MoveStack &&) noexcept = default;
        void push(T value) { std::deque<T>::push_back(std::move(value)); }
        T pop()
        {
            T v = std::move(std::deque<T>::back());
            std::deque<T>::pop_back();
            return v;
        }
        T &top() { return std::deque<T>::back(); }
        const T &top() const { return std::deque<T>::back(); }
    };

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

    mutable QRecursiveMutex _mutex;
    TAtomic<uint> *lastCachedTime {nullptr};
    int maxConnects {0};
    QBasicTimer timer;

    std::vector<MoveStack<SqlDbPtr>> availableDatabases;
    std::vector<MoveStack<SqlDbPtr>> cachedDatabases;

    T_DISABLE_COPY(TSqlDatabasePool)
    T_DISABLE_MOVE(TSqlDatabasePool)
    friend class TSqlDatabase;
};
