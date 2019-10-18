#ifndef TFCORE_UNIX_H
#define TFCORE_UNIX_H

#include "tfcore.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <aio.h>
#ifdef Q_OS_DARWIN
#include <pthread.h>
#endif

#ifndef Q_OS_UNIX
# error "tfcore_unix.h included on a non-Unix system"
#endif

namespace {

inline int tf_aio_write(struct aiocb *aiocbp)
{
    TF_EINTR_LOOP(::aio_write(aiocbp));
}


inline pid_t tf_gettid()
{
#if defined(Q_OS_LINUX)
    return syscall(SYS_gettid);
#elif defined(Q_OS_DARWIN)
    uint64_t tid;
    pthread_threadid_np(NULL, &tid);
    return tid;
    //return syscall(SYS_thread_selfid);
#else
    return 0;
#endif
}


inline int tf_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    TF_EINTR_LOOP(::poll(fds, nfds, timeout));
}


inline int tf_poll_recv(int socket, int timeout)
{
    struct pollfd pfd = { socket, POLLIN, 0 };
    int ret = tf_poll(&pfd, 1, timeout);

    if (ret < 0) {
        return ret;
    }

    return (pfd.revents & (POLLIN | POLLHUP | POLLERR)) ? 0 : 1;
}


inline int tf_poll_send(int socket, int timeout)
{
    struct pollfd pfd = { socket, POLLOUT, 0 };
    int ret = tf_poll(&pfd, 1, timeout);

    if (ret < 0) {
        return ret;
    }

    return (pfd.revents & (POLLOUT | POLLERR)) ? 0 : 1;
}

} // namespace


#ifdef Q_OS_LINUX
#include <sys/epoll.h>

namespace {

inline int tf_epoll_wait(int epfd, struct epoll_event *events,
                         int maxevents, int timeout)
{
    TF_EINTR_LOOP(::epoll_wait(epfd, events, maxevents, timeout));
}


inline int tf_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
    TF_EINTR_LOOP(::epoll_ctl(epfd, op, fd, event));
}


inline int tf_accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags)
{
    TF_EINTR_LOOP(::accept4(sockfd, addr, addrlen, flags));
}

} // namespace

#endif // Q_OS_LINUX
#endif // TFCORE_UNIX_H
