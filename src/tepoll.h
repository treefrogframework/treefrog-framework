#ifndef TEPOLL_H
#define TEPOLL_H

#include <TGlobal>

class TEpollSocket;
struct epoll_event;


class TEpoll
{
public:
    ~TEpoll();

    int wait(int timeout);
    bool isPolling() const { return polling; }
    TEpollSocket *next();
    bool canSend() const;
    bool canReceive() const;

    bool addPoll(TEpollSocket *socket, int events);
    bool modifyPoll(TEpollSocket *socket, int events);
    bool deletePoll(TEpollSocket *socket);

    static TEpoll *instance();

protected:
    bool modifyPoll(int fd, int events);

private:
    int epollFd;
    int listenSocket;
    struct epoll_event *events;
    volatile bool polling;
    int numEvents;
    int eventIterator;

    TEpoll();
    Q_DISABLE_COPY(TEpoll);
};

#endif // TEPOLL_H
