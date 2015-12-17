#ifndef TEPOLLHTTPSOCKET_H
#define TEPOLLHTTPSOCKET_H

#include <QElapsedTimer>
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
    int idleTime() const { return idleElapsed.elapsed() / 1000; }
    virtual void startWorker();
    virtual void deleteLater();
    static TEpollHttpSocket *searchSocket(const QByteArray &uuid);
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
    QElapsedTimer idleElapsed;

    TEpollHttpSocket(int socketDescriptor, const QHostAddress &address);

    friend class TEpollSocket;
    friend class TActionWorker;
    Q_DISABLE_COPY(TEpollHttpSocket)
};

#endif // TEPOLLHTTPSOCKET_H
