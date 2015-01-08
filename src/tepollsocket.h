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
    TEpollSocket(int socketDescriptor, const QHostAddress &address);
    virtual ~TEpollSocket();

    void close();
    int socketDescriptor() const { return sd; }
    const QHostAddress &clientAddress() const { return clientAddr; }

    virtual bool canReadRequest() { return false; }
    virtual void startWorker() { }

    static TEpollSocket *accept(int listeningSocket);
    static TEpollSocket *create(int socketDescriptor, const QHostAddress &address);
    static bool waitSendData(int msec);
    static void dispatchSendData();

protected:
    //virtual void *getRecvBuffer(int size);
    virtual int write(const char *data, int len) = 0;
    void setSocketDescpriter(int socketDescriptor);

    void setSendData(const QByteArray &header, QIODevice *body, bool autoRemove, const TAccessLogger &accessLogger);
    void setDisconnect();
    void setSwitchProtocols(const QByteArray &header, TEpollSocket *target);

private:
    int recv();
    int send();

    int sd;
    QHostAddress clientAddr;
    QQueue<THttpSendBuffer*> sendBuf;

    static void initBuffer(int socketDescriptor);
    friend class TMultiplexingServer;
    Q_DISABLE_COPY(TEpollSocket)
};

#endif // TEPOLLSOCKET_H
