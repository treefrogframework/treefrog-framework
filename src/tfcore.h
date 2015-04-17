#ifndef TFCORE_H
#define TFCORE_H

#include <QtGlobal>
#ifdef Q_OS_WIN
# include <io.h>
#else
# include <unistd.h>
# include <sys/file.h>
# include <sys/socket.h>
#endif
#include <stdio.h>
#include <errno.h>

#define TF_EINTR_LOOP(func)                       \
    int ret;                                      \
    do {                                          \
        errno = 0;                                \
        ret = (func);                             \
    } while (ret == -1 && errno == EINTR);        \
    return ret;


static inline int tf_close(int fd)
{
#ifdef Q_OS_WIN
    return ::_close(fd);
#else
    TF_EINTR_LOOP(::close(fd));
#endif
}


static inline int tf_read(int fd, void *buf, size_t count)
{
#ifdef Q_OS_WIN
    return ::_read(fd, buf, (uint)count);
#else
    TF_EINTR_LOOP(::read(fd, buf, count));
#endif
}


static inline int tf_write(int fd, const void *buf, size_t count)
{
#ifdef Q_OS_WIN
    return ::_write(fd, buf, (uint)count);
#else
    TF_EINTR_LOOP(::write(fd, buf, count));
#endif
}


static inline int tf_send(int sockfd, const void *buf, size_t len, int flags)
{
#ifdef Q_OS_WIN
    Q_ASSERT(0);
    Q_UNUSED(sockfd);
    Q_UNUSED(buf);
    Q_UNUSED(len);
    Q_UNUSED(flags);
    return 0;
#else
    TF_EINTR_LOOP(::send(sockfd, buf, len, flags));
#endif
}


static inline int tf_recv(int sockfd, void *buf, size_t len, int flags)
{
#ifdef Q_OS_WIN
    Q_ASSERT(0);
    Q_UNUSED(sockfd);
    Q_UNUSED(buf);
    Q_UNUSED(len);
    Q_UNUSED(flags);
    return 0;
#else
    TF_EINTR_LOOP(::recv(sockfd, buf, len, flags));
#endif
}


static inline int tf_select(int nfds, fd_set *readfds, fd_set *writefds,
                            fd_set *exceptfds, struct timeval *timeout)
{
    TF_EINTR_LOOP(::select(nfds, readfds, writefds, exceptfds, timeout));
}


static inline int tf_dup(int fd)
{
#if Q_OS_WIN
    return ::_dup(fd);
#else
    return ::fcntl(fd, F_DUPFD, 0);
#endif
}


static inline int tf_flock(int fd, int op)
{
#ifdef Q_OS_WIN
    Q_UNUSED(fd);
    Q_UNUSED(op);
    return 0;
#else
    TF_EINTR_LOOP(::flock(fd, op));
#endif
}


static inline int tf_fileno(FILE *stream)
{
#ifdef Q_OS_WIN
    return ::_fileno(stream);
#else
    return ::fileno(stream);
#endif
}


#ifdef Q_OS_UNIX
# include "tfcore_unix.h"
#endif

#endif // TFCORE_H
