#pragma once
#include "tepollsocket.h"
#include <TGlobal>

class QHostAddress;
class TActionWorker;


class T_CORE_EXPORT TEpollHttpSocket : public TEpollSocket {
public:
    ~TEpollHttpSocket();

    virtual bool canReadRequest() override;
    QByteArray readRequest();
    int idleTime() const;
    virtual void process() override;
    void releaseWorker();
    TActionWorker *worker() { return _worker; }
    bool isProcessing() const override { return (bool)_worker; }

    static TEpollHttpSocket *accept(int listeningSocket);
    static TEpollHttpSocket *create(int socketDescriptor, const QHostAddress &address, bool watch = true);
    static QList<TEpollHttpSocket *> allSockets();

protected:
    virtual int send() override;
    virtual int recv() override;
    virtual bool seekRecvBuffer(int pos) override;
    void parse();
    void clear();

private:
    int64_t _lengthToRead {0};
    uint _idleElapsed {0};
    TActionWorker *_worker {nullptr};

    TEpollHttpSocket(int socketDescriptor, const QHostAddress &address);

    friend class TEpollSocket;
    T_DISABLE_COPY(TEpollHttpSocket)
    T_DISABLE_MOVE(TEpollHttpSocket)
};
