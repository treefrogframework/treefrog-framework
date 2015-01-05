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
    virtual void startWorker();
    virtual bool upgradeConnectionReceived() const;
    virtual TEpollSocket *switchProtocol();

protected:
    //virtual void *getRecvBuffer(int size);
    virtual int write(const char *data, int len);
    void parse();
    void clear();

private:
    QByteArray httpBuffer;
    qint64 lengthToRead;

    TEpollHttpSocket(int socketDescriptor, int id, const QHostAddress &address);

    friend class TEpollSocket;
    friend class TActionWorker;
    Q_DISABLE_COPY(TEpollHttpSocket)
};

#endif // TEPOLLHTTPSOCKET_H
