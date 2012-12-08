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


class T_CORE_EXPORT TSqlDatabasePool : QObject
{
    Q_OBJECT
public:
    ~TSqlDatabasePool();
    QSqlDatabase pop(int databaseId = 0);
    void push(QSqlDatabase &database);
    const QString &environment() const { return dbEnvironment; }

    static void instantiate();
    static TSqlDatabasePool *instance();

    static QString driverType(const QString &env, int databaseId);
    static bool openDatabase(QSqlDatabase &database, const QString &env, int databaseId);

protected:
    void init();
    void timerEvent(QTimerEvent *event);

private:
    Q_DISABLE_COPY(TSqlDatabasePool)
    
    TSqlDatabasePool(const QString &environment);

    int maxConnections;
    QVector<QMap<QString, QDateTime> > pooledConnections;
    QMutex mutex;
    QString dbEnvironment;
    QBasicTimer timer;
};

#endif // TSQLDATABASEPOOL_H
