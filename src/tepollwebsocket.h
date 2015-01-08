#ifndef TEPOLLWEBSOCKET_H
#define TEPOLLWEBSOCKET_H

#include <TGlobal>
#include "tepollsocket.h"

class QHostAddress;
//class TActionWorker;


class T_CORE_EXPORT TEpollWebSocket : public TEpollSocket
{
    Q_OBJECT
public:
    ~TEpollWebSocket();

    virtual bool canReadRequest();
    QByteArray readRequest();
    virtual void startWorker();

protected:
    //virtual void *getRecvBuffer(int size);
    virtual int write(const char *data, int len);
    void parse();
    void clear();

private:
    QByteArray httpBuffer;
    qint64 lengthToRead;

    TEpollWebSocket(int socketDescriptor, const QHostAddress &address);

    friend class TEpollHttpSocket;
    //friend class TActionWorker;
    Q_DISABLE_COPY(TEpollWebSocket)
};

#endif // TEPOLLWEBSOCKET_H
