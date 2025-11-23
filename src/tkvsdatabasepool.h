#pragma once
#include <TAtomic>
#include <TKvsDatabase>
#include <TStack>
#include <TGlobal>
#include <QBasicTimer>
#include <QStack>
#include <QObject>
#include <QString>
#include <QMutex>
#include <vector>
#include <deque>


class T_CORE_EXPORT TKvsDatabasePool : public QObject {
    Q_OBJECT
public:
    using KvsDbPtr = std::unique_ptr<TKvsDatabase>;

    ~TKvsDatabasePool();
    TKvsDatabase::Handle database(Tf::KvsEngine engine);
    static TKvsDatabasePool *instance();

protected:
    void init();
    bool setDatabaseSettings(TKvsDatabase &database, Tf::KvsEngine engine) const;
    void timerEvent(QTimerEvent *event);

    static QString driverName(Tf::KvsEngine engine);

private:
    TKvsDatabasePool();
    void pool(KvsDbPtr dbptr);

    //mutable QRecursiveMutex _mutex;
    TAtomic<uint> *lastCachedTime {nullptr};
    int maxConnects {0};
    QBasicTimer timer;
    //std::vector<MoveStack<KvsDbPtr>> availableDatabases;
    //std::vector<MoveStack<KvsDbPtr>> cachedDatabases;
    std::vector<TStack<KvsDbPtr>> availableDatabases;
    std::vector<TStack<KvsDbPtr>> cachedDatabases;

    T_DISABLE_COPY(TKvsDatabasePool)
    T_DISABLE_MOVE(TKvsDatabasePool)
    friend class TKvsDatabase;
};
