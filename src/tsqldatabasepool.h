#pragma once
#include <TAtomic>
#include <TGlobal>
#include <QBasicTimer>
#include <QDateTime>
#include <QStack>
#include <QObject>
#include <QSqlDatabase>
#include <QString>

class TSqlDatabase;
template <class T> class TStack;


class T_CORE_EXPORT TSqlDatabasePool : public QObject {
    Q_OBJECT
public:
    ~TSqlDatabasePool();
    QSqlDatabase database(int databaseId = 0);
    void pool(QSqlDatabase &database, bool forceClose = false);

    static TSqlDatabasePool *instance();
    static bool setDatabaseSettings(TSqlDatabase &database, int databaseId);
    static int getDatabaseId(const QSqlDatabase &database);

protected:
    void init();
    void timerEvent(QTimerEvent *event);
    void closeDatabase(QSqlDatabase &database);

private:
    TSqlDatabasePool();

#if QT_VERSION < 0x060000
    mutable QMutex _mutex {QMutex::Recursive};
#else
    mutable QRecursiveMutex _mutex;
#endif
    QStack<QString> *cachedDatabase {nullptr};
    TAtomic<uint> *lastCachedTime {nullptr};
    QStack<QString> *availableNames {nullptr};
    int maxConnects {0};
    QBasicTimer timer;

    T_DISABLE_COPY(TSqlDatabasePool)
    T_DISABLE_MOVE(TSqlDatabasePool)
};
