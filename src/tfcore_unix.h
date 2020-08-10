#ifndef TFCORE_UNIX_H
#define TFCORE_UNIX_H

#include "tfcore.h"
#include <aio.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef Q_OS_DARWIN
#include <pthread.h>
#endif

#ifndef Q_OS_UNIX
#error "tfcore_unix.h included on a non-Unix system"
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

#ifdef Q_OS_LINUX

inline int tf_ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *ts)
{
    TF_EAGAIN_LOOP(::ppoll(fds, nfds, ts, nullptr));
}


inline int tf_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    struct timespec ts = {timeout / 1000, (timeout % 1000) * 1000000L};
    return tf_ppoll(fds, nfds, &ts);
}

#endif

#ifdef Q_OS_DARWIN

inline int tf_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    TF_EAGAIN_LOOP(::poll(fds, nfds, timeout));
}

#endif

/*!
  Waits for receive-event on a descriptor.
  Return: 0:timeout  1:event  -1:error
 */
inline int tf_poll_recv(int socket, int timeout)
{
    struct pollfd pfd = {socket, POLLIN, 0};
    int ret = tf_poll(&pfd, 1, timeout);
    return ret;
}

/*!
  Waits for write-event on a descriptor.
  Return: 0:timeout  1:event  -1:error
 */
inline int tf_poll_send(int socket, int timeout)
{
    struct pollfd pfd = {socket, POLLOUT, 0};
    int ret = tf_poll(&pfd, 1, timeout);
    return ret;
}

}  // namespace


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

}  // namespace

#endif  // Q_OS_LINUX
#endif  // TFCORE_UNIX_H
