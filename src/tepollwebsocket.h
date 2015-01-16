#ifndef TEPOLLWEBSOCKET_H
#define TEPOLLWEBSOCKET_H

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

    virtual bool canReadRequest();
    QByteArray readRequest();
    virtual void startWorker();

protected:
    virtual void *getRecvBuffer(int size);
    virtual bool seekRecvBuffer(int pos);
    void parse();
    void clear();

    THttpResponseHeader handshakeResponse() const;

private:
    THttpRequestHeader reqHeader;
    QByteArray recvBuffer;
    qint64 lengthToRead;

    TEpollWebSocket(int socketDescriptor, const QHostAddress &address, const THttpRequestHeader &header);

    friend class TEpoll;
    Q_DISABLE_COPY(TEpollWebSocket)
};

#endif // TEPOLLWEBSOCKET_H
