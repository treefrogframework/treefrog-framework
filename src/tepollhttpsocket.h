#ifndef TEPOLLHTTPSOCKET_H
#define TEPOLLHTTPSOCKET_H

#include <TGlobal>
#include "tepollsocket.h"

class QHostAddress;
class TActionWorker;


class T_CORE_EXPORT TEpollHttpSocket : public TEpollSocket
{
    Q_OBJECT
public:
    ~TEpollHttpSocket();

    virtual bool canReadRequest();
    QByteArray readRequest();
    int idleTime() const;
    virtual void startWorker();
    virtual void deleteLater();
    static TEpollHttpSocket *searchSocket(int sid);
    static QList<TEpollHttpSocket*> allSockets();

public slots:
    void releaseWorker();

protected:
    virtual int send();
    virtual int recv();
    virtual void *getRecvBuffer(int size);
    virtual bool seekRecvBuffer(int pos);
    void parse();
    void clear();

private:
    QByteArray httpBuffer;
    qint64 lengthToRead;
    uint idleElapsed {0};

    TEpollHttpSocket(int socketDescriptor, const QHostAddress &address);

    friend class TEpollSocket;
    friend class TActionWorker;
    T_DISABLE_COPY(TEpollHttpSocket)
    T_DISABLE_MOVE(TEpollHttpSocket)
};

#endif // TEPOLLHTTPSOCKET_H
