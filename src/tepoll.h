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
    bool isPolling() const { return _polling; }
    TEpollSocket *next();
    bool canReceive() const;
    bool canSend() const;
    int recv(TEpollSocket *socket) const;
    int send(TEpollSocket *socket) const;

    bool addPoll(TEpollSocket *socket, int events);
    bool modifyPoll(TEpollSocket *socket, int events);
    bool deletePoll(TEpollSocket *socket);
    void dispatchEvents();
    void releaseAllPollingSockets();

    // For action workers
    void setSendData(TEpollSocket *socket, const QByteArray &header, QIODevice *body, bool autoRemove, TAccessLogger &&accessLogger);
    void setSendData(TEpollSocket *socket, const QByteArray &data);
    void setDisconnect(TEpollSocket *socket);
    void setSwitchToWebSocket(TEpollSocket *socket, const THttpRequestHeader &header);

    static TEpoll *instance();

protected:
    bool modifyPoll(int fd, int events);

private:
    int _epollFd {0};
    int _listenSocket {0};
    struct epoll_event *_events {nullptr};
    volatile bool _polling {false};
    int _numEvents {0};
    int _eventIterator {0};
    TQueue<TSendData *> _sendRequests;

    TEpoll();
    T_DISABLE_COPY(TEpoll)
    T_DISABLE_MOVE(TEpoll);
};
