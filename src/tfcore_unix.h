#pragma once
#include "tfcore.h"
#include <aio.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef Q_OS_LINUX
#include <sys/epoll.h>
#endif
#ifdef Q_OS_DARWIN
#include <pthread.h>
#endif

#ifndef Q_OS_UNIX
#error "tfcore_unix.h included on a non-Unix system"
#endif

namespace {

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


inline pid_t tf_gettid()
{
    return syscall(SYS_gettid);
}


inline int tf_send(int sockfd, const void *buf, size_t len, int flags = 0)
{
    flags |= MSG_NOSIGNAL;
    TF_EINTR_LOOP(::send(sockfd, buf, len, flags));
}

#endif  // Q_OS_LINUX

#ifdef Q_OS_DARWIN

inline int tf_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    TF_EAGAIN_LOOP(::poll(fds, nfds, timeout));
}


inline pid_t tf_gettid()
{
    uint64_t tid;
    pthread_threadid_np(NULL, &tid);
    return tid;
}


inline int tf_send(int sockfd, const void *buf, size_t len, int flags = 0)
{
    TF_EINTR_LOOP(::send(sockfd, buf, len, flags));
}

#endif  // Q_OS_DARWIN

inline int tf_aio_write(struct aiocb *aiocbp)
{
    TF_EINTR_LOOP(::aio_write(aiocbp));
}


inline int tf_close(int fd)
{
    TF_EINTR_LOOP(::close(fd));
}


inline int tf_read(int fd, void *buf, size_t count)
{
    TF_EINTR_LOOP(::read(fd, buf, count));
}


inline int tf_write(int fd, const void *buf, size_t count)
{
    TF_EINTR_LOOP(::write(fd, buf, count));
}


inline int tf_recv(int sockfd, void *buf, size_t len, int flags = 0)
{
    TF_EINTR_LOOP(::recv(sockfd, buf, len, flags));
}


inline int tf_close_socket(int sockfd)
{
    TF_EINTR_LOOP(::close(sockfd));
}


inline int tf_dup(int fd)
{
    return ::fcntl(fd, F_DUPFD, 0);
}


inline int tf_flock(int fd, int op)
{
    TF_EINTR_LOOP(::flock(fd, op));
}

// advisory lock. exclusive:true=exclusive lock, false=shared lock
inline int tf_lockfile(int fd, bool exclusive, bool blocking)
{
    struct flock lck;

    memset(&lck, 0, sizeof(struct flock));
    lck.l_type = (exclusive) ? F_WRLCK : F_RDLCK;
    lck.l_whence = SEEK_SET;
    auto cmd = (blocking) ? F_SETLKW : F_SETLK;
    TF_EINTR_LOOP(::fcntl(fd, cmd, &lck));
}


inline int tf_unlink(const char *pathname)
{
    return ::unlink(pathname);
}


inline int tf_fileno(FILE *stream)
{
    return ::fileno(stream);
}

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
