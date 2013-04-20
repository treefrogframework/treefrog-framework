#ifndef TKVSDATABASEPOOL_H
#define TKVSDATABASEPOOL_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QMutex>
#include <QBasicTimer>
#include <TKvsDatabase>
#include <TGlobal>

class QSettings;


class T_CORE_EXPORT TKvsDatabasePool : public QObject
{
    Q_OBJECT
public:
    enum KvsType {
        MongoDB = 0,
    };

    ~TKvsDatabasePool();
    TKvsDatabase pop(KvsType type = MongoDB);
    void push(TKvsDatabase &database);

    static void instantiate();
    static TKvsDatabasePool *instance();

protected:
    void init();
    QSettings &kvsSettings(KvsType type) const;
    bool openDatabase(TKvsDatabase &database, KvsType type, const QString &env) const;
    void timerEvent(QTimerEvent *event);

    static QString driverName(KvsType type);
    static int maxConnectionsPerProcess();

private:
    Q_DISABLE_COPY(TKvsDatabasePool)
    TKvsDatabasePool(const QString &environment);

    QVector<QMap<QString, QDateTime> > pooledConnections;
    QMutex mutex;
    QString dbEnvironment;
    QBasicTimer timer;
};

#endif // TKVSDATABASEPOOL_H
