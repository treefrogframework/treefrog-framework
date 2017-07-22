#ifndef TSQLDATABASEPOOL_H
#define TSQLDATABASEPOOL_H

#include <QObject>
#include <QSqlDatabase>
#include <QVector>
#include <QMap>
#include <QString>
#include <QDateTime>
#include <QBasicTimer>
#include <TGlobal>
#include "tatomic.h"
#include "tstack.h"

class TSqlDatabase;


class T_CORE_EXPORT TSqlDatabasePool : public QObject
{
    Q_OBJECT
public:
    ~TSqlDatabasePool();
    QSqlDatabase database(int databaseId = 0);
    void pool(QSqlDatabase &database);

    static void instantiate();
    static TSqlDatabasePool *instance();

    static QString driverType(const QString &env, int databaseId);
    static bool setDatabaseSettings(TSqlDatabase &database, const QString &env, int databaseId);
    static int getDatabaseId(const QSqlDatabase &database);

protected:
    void init();
    void timerEvent(QTimerEvent *event);

private:
    T_DISABLE_COPY(TSqlDatabasePool)
    T_DISABLE_MOVE(TSqlDatabasePool)
    TSqlDatabasePool(const QString &environment);

    TStack<QString> *cachedDatabase {nullptr};
    TAtomic<uint> *lastCachedTime {nullptr};
    TStack<QString> *availableNames {nullptr};
    int maxConnects;
    QString dbEnvironment;
    QBasicTimer timer;
};

#endif // TSQLDATABASEPOOL_H
