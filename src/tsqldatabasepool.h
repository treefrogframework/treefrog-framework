#pragma once
#include "tatomic.h"
#include "tstack.h"
#include <QBasicTimer>
#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QVector>
#include <TGlobal>

class TSqlDatabase;


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

    TStack<QString> *cachedDatabase {nullptr};
    TAtomic<uint> *lastCachedTime {nullptr};
    TStack<QString> *availableNames {nullptr};
    int maxConnects {0};
    QBasicTimer timer;

    T_DISABLE_COPY(TSqlDatabasePool)
    T_DISABLE_MOVE(TSqlDatabasePool)
};

