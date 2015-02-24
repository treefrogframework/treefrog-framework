#ifndef TEPOLLSOCKET_H
#define TEPOLLSOCKET_H

#include <QObject>
#include <QByteArray>
#include <QQueue>
#include <QHostAddress>
#include <TGlobal>

class QHostAddress;
class QFileInfo;
class TSendBuffer;
class THttpHeader;
class TAccessLogger;


class T_CORE_EXPORT TEpollSocket : public QObject
{
    Q_OBJECT
public:
    TEpollSocket(int socketDescriptor, const QHostAddress &address);
    virtual ~TEpollSocket();

    void close();
    int socketDescriptor() const { return sd; }
    const QHostAddress &clientAddress() const { return clientAddr; }
    const QByteArray &socketUuid() const { return uuid; }

    virtual bool canReadRequest() { return false; }
    virtual void startWorker() { }

    static TEpollSocket *accept(int listeningSocket);
    static TEpollSocket *create(int socketDescriptor, const QHostAddress &address);
    static TSendBuffer *createSendBuffer(const QByteArray &header, const QFileInfo &file, bool autoRemove, const TAccessLogger &logger);

protected:
    int send();
    int recv();
    void enqueueSendData(const QByteArray &data);
    void enqueueSendData(TSendBuffer *buffer);
    void setSocketDescpriter(int socketDescriptor);
    virtual void *getRecvBuffer(int size) = 0;
    virtual bool seekRecvBuffer(int pos) = 0;

private:
    int sd;
    QByteArray uuid;
    QHostAddress clientAddr;
    QQueue<TSendBuffer*> sendBuf;

    static void initBuffer(int socketDescriptor);

    friend class TEpoll;
    Q_DISABLE_COPY(TEpollSocket)
};

#endif // TEPOLLSOCKET_H
