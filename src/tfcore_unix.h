#ifndef TFCORE_UNIX_H
#define TFCORE_UNIX_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <aio.h>

#ifndef Q_OS_UNIX
# error "tfcore_unix.h included on a non-Unix system"
#endif

#define EINTR_LOOP(ret, cmd)                    \
    do {                                        \
        errno = 0;                              \
        ret = (cmd);                            \
    } while (ret == -1 && errno == EINTR)


static inline int tf_close(int fd)
{
    int ret;
    EINTR_LOOP(ret, ::close(fd));
    return ret;
}


static inline int tf_flock(int fd, int op)
{
    int ret;
    EINTR_LOOP(ret, ::flock(fd, op));
    return ret;
}


static inline int tf_aio_write(struct aiocb *aiocbp)
{
    int ret;
    EINTR_LOOP(ret, ::aio_write(aiocbp));
    return ret;
}


static inline pid_t gettid()
{
    return syscall(SYS_gettid);
}


// static inline int tf_setaffinity(int cpu)
// {
//     cpu_set_t cs;
//     CPU_ZERO(&cs);
//     CPU_SET(cpu, &cs);
//     return sched_setaffinity(0, sizeof(cs), &cs);
// }


#ifdef Q_OS_LINUX

#include <sys/epoll.h>

static inline int tf_epoll_wait(int epfd, struct epoll_event *events,
                                int maxevents, int timeout)
{
    int ret;
    EINTR_LOOP(ret, ::epoll_wait(epfd, events, maxevents, timeout));
    return ret;
}


static inline int tf_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
    int ret;
    EINTR_LOOP(ret, ::epoll_ctl(epfd, op, fd, event));
    return ret;
}


static inline int tf_accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags)
{
    int ret;
    EINTR_LOOP(ret, ::accept4(sockfd, addr, addrlen, flags));
    return ret;
}

#endif // Q_OS_LINUX

#endif // TFCORE_UNIX_H
