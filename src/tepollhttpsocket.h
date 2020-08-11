#pragma once
#include "tepollsocket.h"
#include <TGlobal>

class QHostAddress;
class TActionWorker;


class T_CORE_EXPORT TEpollHttpSocket : public TEpollSocket {
public:
    ~TEpollHttpSocket();

    virtual bool canReadRequest();
    QByteArray readRequest();
    int idleTime() const;
    virtual void startWorker();
    void releaseWorker();
    static TEpollHttpSocket *searchSocket(int sid);
    static QList<TEpollHttpSocket *> allSockets();

protected:
    virtual int send();
    virtual int recv();
    virtual void *getRecvBuffer(int size);
    virtual bool seekRecvBuffer(int pos);
    void parse();
    void clear();

private:
    QByteArray httpBuffer;
    qint64 lengthToRead {0};
    uint idleElapsed {0};

    TEpollHttpSocket(int socketDescriptor, const QHostAddress &address);

    friend class TEpollSocket;
    T_DISABLE_COPY(TEpollHttpSocket)
    T_DISABLE_MOVE(TEpollHttpSocket)
};

