#ifndef TEPOLLSOCKET_H
#define TEPOLLSOCKET_H

#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QHostAddress>
#include <TGlobal>
#include "thttpbuffer.h"

class QHostAddress;
class THttpSendBuffer;
class THttpHeader;
class TAccessLogger;
class TActionWorker;


class T_CORE_EXPORT TEpollSocket : public QObject
{
    Q_OBJECT
public:
    ~TEpollSocket();

    int recv();
    int send();
    void close();
    int id() const { return identifier; }
    int socketDescriptor() const { return sd; }
    bool canReadHttpRequest();
    THttpBuffer &recvBuffer() { return recvBuf; }
    void startWorker();

    static TEpollSocket *accept(int listeningSocket);
    static TEpollSocket *create(int socketDescriptor, const QHostAddress &address);
    static void releaseAllSockets();
    static bool waitSendData(int msec);
    static void dispatchSendData();

private:
    static void setSendData(int id, const THttpHeader *header, QIODevice *body, bool autoRemove, const TAccessLogger &accessLogger);
    static void setDisconnect(int id);

private:
    int sd;
    int identifier;
    THttpBuffer recvBuf;
    QQueue<THttpSendBuffer*> sendBuf;

    static void initBuffer(int socketDescriptor);

    TEpollSocket(int socketDescriptor, int id, const QHostAddress &address);
    Q_DISABLE_COPY(TEpollSocket)
    friend class TActionWorker;
};

#endif // TEPOLLSOCKET_H
