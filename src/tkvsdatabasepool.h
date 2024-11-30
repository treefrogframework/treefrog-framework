#pragma once
#include <TAtomic>
#include <TKvsDatabase>
#include <TGlobal>
#include <QBasicTimer>
#include <QStack>
#include <QObject>
#include <QString>
#include <QMutex>

class QSettings;
template <class T> class TStack;


class T_CORE_EXPORT TKvsDatabasePool : public QObject {
    Q_OBJECT
public:
    ~TKvsDatabasePool();
    TKvsDatabase database(Tf::KvsEngine engine);
    void pool(TKvsDatabase &database);
    TKvsDatabaseData getDatabaseSettings(Tf::KvsEngine engine) const;

    static TKvsDatabasePool *instance();

protected:
    void init();
    bool setDatabaseSettings(TKvsDatabase &database, Tf::KvsEngine engine) const;
    void timerEvent(QTimerEvent *event);

    static QString driverName(Tf::KvsEngine engine);

private:
    TKvsDatabasePool();

    mutable QRecursiveMutex _mutex;
    QStack<QString> *cachedDatabase {nullptr};
    TAtomic<uint> *lastCachedTime {nullptr};
    QStack<QString> *availableNames {nullptr};
    int maxConnects {0};
    QBasicTimer timer;

    T_DISABLE_COPY(TKvsDatabasePool)
    T_DISABLE_MOVE(TKvsDatabasePool)
};
