#ifndef TKVSDATABASEPOOL2_H
#define TKVSDATABASEPOOL2_H

#include <QObject>
#include <QString>
#include <QBasicTimer>
#include <TKvsDatabase>
#include <TGlobal>

class QSettings;
class TAtomicSet;


class T_CORE_EXPORT TKvsDatabasePool2 : public QObject
{
    Q_OBJECT
public:
    ~TKvsDatabasePool2();
    TKvsDatabase database(TKvsDatabase::Type type);
    void pool(TKvsDatabase &database);

    static void instantiate(int maxConnections = 0);
    static TKvsDatabasePool2 *instance();

protected:
    void init();
    bool isKvsAvailable(TKvsDatabase::Type type) const;
    QSettings &kvsSettings(TKvsDatabase::Type type) const;
    bool setDatabaseSettings(TKvsDatabase &database, TKvsDatabase::Type type, const QString &env) const;
    void timerEvent(QTimerEvent *event);

    static QString driverName(TKvsDatabase::Type type);
    static int maxDbConnectionsPerProcess();

private:
    TKvsDatabasePool2(const QString &environment);

    struct DatabaseUse
    {
        QString dbName;
        uint lastUsed;
    };

    TAtomicSet *dbSet;
    int maxConnects;
    QString dbEnvironment;
    QBasicTimer timer;

    Q_DISABLE_COPY(TKvsDatabasePool2)
};

#endif // TKVSDATABASEPOOL2_H
