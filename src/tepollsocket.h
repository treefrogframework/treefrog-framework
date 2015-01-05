#ifndef TEPOLLSOCKET_H
#define TEPOLLSOCKET_H

#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QHostAddress>
#include <TGlobal>

class QHostAddress;
class THttpSendBuffer;
class THttpHeader;
class TAccessLogger;
class TAbstractRecvBuffer;


class T_CORE_EXPORT TEpollSocket : public QObject
{
    Q_OBJECT
public:
    TEpollSocket(int socketDescriptor, int id, const QHostAddress &address);
    virtual ~TEpollSocket();

    int recv();
    int send();
    void close();
    int id() const { return identifier; }
    int socketDescriptor() const { return sd; }
    const QHostAddress &clientAddress() const { return clientAddr; }

    virtual bool canReadRequest() { return false; }
    virtual void startWorker() { }
    virtual bool upgradeConnectionReceived() const { return false; }
    virtual TEpollSocket *switchProtocol() { return NULL; }

    static TEpollSocket *accept(int listeningSocket);
    static TEpollSocket *create(int socketDescriptor, const QHostAddress &address);
    static void releaseAllSockets();
    static bool waitSendData(int msec);
    static void dispatchSendData();

protected:
    //virtual void *getRecvBuffer(int size);
    virtual int write(const char *data, int len) = 0;

    static void setSendData(int id, const QByteArray &header, QIODevice *body, bool autoRemove, const TAccessLogger &accessLogger);
    static void setDisconnect(int id);

private:
    int sd;
    int identifier;
    QHostAddress clientAddr;
    QQueue<THttpSendBuffer*> sendBuf;

    static void initBuffer(int socketDescriptor);

    Q_DISABLE_COPY(TEpollSocket)
};

#endif // TEPOLLSOCKET_H
