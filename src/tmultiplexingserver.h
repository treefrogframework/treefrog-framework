#ifndef TMULTIPLEXINGSERVER_H
#define TMULTIPLEXINGSERVER_H

#include <QThread>
#include <QMap>
#include <QQueue>
#include <QList>
#include <QByteArray>
#include <QFileInfo>
#include <TGlobal>
#include <TApplicationServerBase>
#include <TAccessLog>
#include "tatomicqueue.h"

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
    bool start();
    void stop();

    static void instantiate(int listeningSocket);
    static TMultiplexingServer *instance();

protected:
    void run();

signals:
    bool incomingRequest(TEpollSocket *socket);

private:
    int maxWorkers;
    volatile bool stopped;
    int listenSocket;

    TMultiplexingServer(int listeningSocket, QObject *parent = 0);  // Constructor
    friend class TWorkerStarter;
    Q_DISABLE_COPY(TMultiplexingServer)
};


/*
 * WorkerStarter class declaration
 * This object creates worker threads in the main event loop.
 */
class TWorkerStarter : public QObject
{
    Q_OBJECT
public:
    TWorkerStarter(QObject *parent = 0) : QObject(parent) { }
    virtual ~TWorkerStarter();

public slots:
    void startWorker(TEpollSocket *);
};

#endif // TMULTIPLEXINGSERVER_H
