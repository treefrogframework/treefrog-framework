#pragma once
#include "tqueue.h"
#include <QMap>
#include <TGlobal>
#include <sys/epoll.h>

class QIODevice;
class QByteArray;
class TEpollSocket;
class TAccessLogger;
class TSendData;
class THttpRequestHeader;
struct epoll_event;


class T_CORE_EXPORT TEpoll {
public:
    ~TEpoll();

    int wait(int timeout);
    bool isPolling() const { return polling; }
    TEpollSocket *next();
    bool canReceive() const;
    bool canSend() const;
    int recv(TEpollSocket *socket) const;
    int send(TEpollSocket *socket) const;

    bool addPoll(TEpollSocket *socket, int events);
    bool modifyPoll(TEpollSocket *socket, int events);
    bool deletePoll(TEpollSocket *socket);
    //bool waitSendData(int msec);
    void dispatchSendData();
    void releaseAllPollingSockets();

    // For action workers
    void setSendData(TEpollSocket *socket, const QByteArray &header, QIODevice *body, bool autoRemove, const TAccessLogger &accessLogger);
    void setSendData(TEpollSocket *socket, const QByteArray &data);
    void setDisconnect(TEpollSocket *socket);
    void setSwitchToWebSocket(TEpollSocket *socket, const THttpRequestHeader &header);

    static TEpoll *instance();

protected:
    bool modifyPoll(int fd, int events);

private:
    int epollFd {0};
    int listenSocket {0};
    struct epoll_event *events {nullptr};
    volatile bool polling {false};
    int numEvents {0};
    int eventIterator {0};
    QMap<TEpollSocket *, int> pollingSockets;
    TQueue<TSendData *> sendRequests;

    TEpoll();
    T_DISABLE_COPY(TEpoll)
    T_DISABLE_MOVE(TEpoll);
};

