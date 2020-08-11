#pragma once
#include "tatomic.h"
#include "tstack.h"
#include <QBasicTimer>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVector>
#include <TGlobal>
#include <TKvsDatabase>

class QSettings;


class T_CORE_EXPORT TKvsDatabasePool : public QObject {
    Q_OBJECT
public:
    ~TKvsDatabasePool();
    TKvsDatabase database(Tf::KvsEngine engine);
    void pool(TKvsDatabase &database);

    static TKvsDatabasePool *instance();

protected:
    void init();
    bool setDatabaseSettings(TKvsDatabase &database, Tf::KvsEngine engine) const;
    void timerEvent(QTimerEvent *event);

    static QString driverName(Tf::KvsEngine engine);

private:
    T_DISABLE_COPY(TKvsDatabasePool)
    T_DISABLE_MOVE(TKvsDatabasePool)
    TKvsDatabasePool();

    TStack<QString> *cachedDatabase {nullptr};
    TAtomic<uint> *lastCachedTime {nullptr};
    TStack<QString> *availableNames {nullptr};
    int maxConnects {0};
    QBasicTimer timer;
};

