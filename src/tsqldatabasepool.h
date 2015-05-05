#ifndef TSQLDATABASEPOOL_H
#define TSQLDATABASEPOOL_H

#include <QObject>
#include <QSqlDatabase>
#include <QVector>
#include <QMap>
#include <QString>
#include <QMutex>
#include <QDateTime>
#include <QBasicTimer>
#include <TGlobal>


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
    static int maxDbConnectionsPerProcess();
    static bool setDatabaseSettings(QSqlDatabase &database, const QString &env, int databaseId);
    static int getDatabaseId(const QSqlDatabase &database);

protected:
    void init();
    void timerEvent(QTimerEvent *event);

private:
    Q_DISABLE_COPY(TSqlDatabasePool)
    TSqlDatabasePool(const QString &environment);

    QVector<QMap<QString, uint>> pooledConnections;
    QMutex mutex;
    int maxConnects;
    QString dbEnvironment;
    QBasicTimer timer;
};

#endif // TSQLDATABASEPOOL_H
