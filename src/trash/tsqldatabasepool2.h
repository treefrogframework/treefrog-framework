#ifndef TSQLDATABASEPOOL2_H
#define TSQLDATABASEPOOL2_H

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QBasicTimer>
#include <TGlobal>

class TAtomicSet;


class T_CORE_EXPORT TSqlDatabasePool2 : public QObject
{
    Q_OBJECT
public:
    ~TSqlDatabasePool2();
    QSqlDatabase database(int databaseId = 0);
    void pool(QSqlDatabase &database);

    static void instantiate(int maxConnections = 0);
    static TSqlDatabasePool2 *instance();

    static QString driverType(const QString &env, int databaseId);
    static int maxDbConnectionsPerProcess();
    static bool setDatabaseSettings(QSqlDatabase &database, const QString &env, int databaseId);
    static int getDatabaseId(const QSqlDatabase &database);

protected:
    void init();
    void timerEvent(QTimerEvent *event);

private:
    TSqlDatabasePool2(const QString &environment);

    struct DatabaseUse
    {
        QString dbName;
        uint lastUsed;
    };

    TAtomicSet *dbSet;
    int maxConnects;
    QString dbEnvironment;
    QBasicTimer timer;

    Q_DISABLE_COPY(TSqlDatabasePool2)
};

#endif // TSQLDATABASEPOOL2_H
