#ifndef TEPOLLWEBSOCKET_H
#define TEPOLLWEBSOCKET_H

#include <QUuid>
#include <TGlobal>
#include <THttpRequestHeader>
#include <THttpResponseHeader>
#include "tepollsocket.h"

class QHostAddress;


class T_CORE_EXPORT TEpollWebSocket : public TEpollSocket
{
    Q_OBJECT
public:
    ~TEpollWebSocket();

    QByteArray socketUuid() const { return uuid.toByteArray(); }
    void sendText(const QString &message);
    void sendBinary(const QByteArray &data);

    QByteArray readRequest();
    virtual bool canReadRequest();
    virtual void startWorker();

    static void sendText(const QByteArray &socketUuid, const QString &message) { }
    static void sendBinary(const QByteArray &socketUuid, const QByteArray &data) { }

protected:
    virtual void *getRecvBuffer(int size);
    virtual bool seekRecvBuffer(int pos);
    void parse();
    void clear();
    THttpResponseHeader handshakeResponse() const;

private:
    QUuid uuid;
    THttpRequestHeader reqHeader;
    QByteArray recvBuffer;
    qint64 lengthToRead;

    TEpollWebSocket(int socketDescriptor, const QHostAddress &address, const THttpRequestHeader &header);

    friend class TEpoll;
    Q_DISABLE_COPY(TEpollWebSocket)
};

#endif // TEPOLLWEBSOCKET_H
