#ifndef TMULTIPLEXINGSERVER_H
#define TMULTIPLEXINGSERVER_H

#include <QThread>
#include <QMap>
#include <QList>
#include <QByteArray>
#include <QFileInfo>
#include <QBasicTimer>
#include <TGlobal>
#include <TApplicationServerBase>
#include <TAccessLog>
#include "tatomic.h"

class QIODevice;
class THttpHeader;
class THttpSendBuffer;
class TEpollSocket;


class T_CORE_EXPORT TMultiplexingServer : public QThread, public TApplicationServerBase
{
    Q_OBJECT
public:
    ~TMultiplexingServer();

    bool isListening() const { return listenSocket > 0; }
    bool start(bool debugMode) override;
    void stop() override;
    void setAutoReloadingEnabled(bool enable) override;
    bool isAutoReloadingEnabled() override;

    static void instantiate(int listeningSocket);
    static TMultiplexingServer *instance();

protected:
    void run() override;
    void timerEvent(QTimerEvent *event) override;

signals:
    bool incomingRequest(TEpollSocket *socket);

private:
    int maxWorkers;
    TAtomic<bool> stopped;
    int listenSocket;
    QBasicTimer reloadTimer;

    TMultiplexingServer(int listeningSocket, QObject *parent = 0);  // Constructor
    T_DISABLE_COPY(TMultiplexingServer)
    T_DISABLE_MOVE(TMultiplexingServer)
};


/*
 * WorkerStarter class declaration
 * This object creates worker threads in the main event loop.
 */
/*
class TWorkerStarter : public QObject
{
    Q_OBJECT
public:
    TWorkerStarter(QObject *parent = 0) : QObject(parent) { }
    virtual ~TWorkerStarter();

public slots:
    void startWorker(TEpollSocket *);
};
*/

#endif // TMULTIPLEXINGSERVER_H
