#ifndef TFCORE_UNIX_H
#define TFCORE_UNIX_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <aio.h>
#include "tfcore.h"

#ifndef Q_OS_UNIX
# error "tfcore_unix.h included on a non-Unix system"
#endif


static inline int tf_aio_write(struct aiocb *aiocbp)
{
    TF_EINTR_LOOP(::aio_write(aiocbp));
}


static inline pid_t gettid()
{
#if defined(Q_OS_LINUX)
    return syscall(SYS_gettid);
#elif defined(Q_OS_DARWIN)
    return syscall(SYS_thread_selfid);
#else
    return 0;
#endif
}


#ifdef Q_OS_LINUX
#include <sys/epoll.h>

static inline int tf_epoll_wait(int epfd, struct epoll_event *events,
                                int maxevents, int timeout)
{
    TF_EINTR_LOOP(::epoll_wait(epfd, events, maxevents, timeout));
}


static inline int tf_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
    TF_EINTR_LOOP(::epoll_ctl(epfd, op, fd, event));
}


static inline int tf_accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags)
{
    TF_EINTR_LOOP(::accept4(sockfd, addr, addrlen, flags));
}

#endif // Q_OS_LINUX

#endif // TFCORE_UNIX_H
