#ifndef TKVSDATABASEPOOL_H
#define TKVSDATABASEPOOL_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QBasicTimer>
#include <TKvsDatabase>
#include <TGlobal>
#include "tatomic.h"
#include "tstack.h"

class QSettings;


class T_CORE_EXPORT TKvsDatabasePool : public QObject
{
    Q_OBJECT
public:
    ~TKvsDatabasePool();
    TKvsDatabase database(TKvsDatabase::Type type);
    void pool(TKvsDatabase &database);

    static void instantiate();
    static TKvsDatabasePool *instance();

protected:
    void init();
    bool isKvsAvailable(TKvsDatabase::Type type) const;
    QSettings &kvsSettings(TKvsDatabase::Type type) const;
    bool setDatabaseSettings(TKvsDatabase &database, TKvsDatabase::Type type, const QString &env) const;
    void timerEvent(QTimerEvent *event);

    static QString driverName(TKvsDatabase::Type type);

private:
    T_DISABLE_COPY(TKvsDatabasePool)
    T_DISABLE_MOVE(TKvsDatabasePool)
    TKvsDatabasePool(const QString &environment);

    TStack<QString> *cachedDatabase {nullptr};
    TAtomic<uint> *lastCachedTime {nullptr};
    TStack<QString> *availableNames {nullptr};
    int maxConnects;
    QString dbEnvironment;
    QBasicTimer timer;
};

#endif // TKVSDATABASEPOOL_H
