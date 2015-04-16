#ifndef TFCORE_H
#define TFCORE_H

#include <unistd.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <errno.h>
#ifdef Q_CC_MSVC
# include <io.h>
#endif

#define TF_EINTR_LOOP(func)                      \
    ssize_t ret;                                 \
    do {                                         \
        errno = 0;                               \
        ret = (func);                            \
    } while (ret == -1 && errno == EINTR);       \
    return ret;


static inline int tf_close(int fd)
{
#ifdef Q_CC_MSVC
    return ::_close(fd);
#else
    TF_EINTR_LOOP(::close(fd));
#endif
}


static inline ssize_t tf_read(int fd, void *buf, size_t count)
{
#ifdef Q_CC_MSVC
    return ::_read(fd, buf, count);
#else
    TF_EINTR_LOOP(::read(fd, buf, count));
#endif
}


static inline ssize_t tf_write(int fd, const void *buf, size_t count)
{
#ifdef Q_CC_MSVC
    return ::_write(fd, buf, count);
#else
    TF_EINTR_LOOP(::write(fd, buf, count));
#endif
}


static inline ssize_t tf_send(int sockfd, const void *buf, size_t len, int flags)
{
    TF_EINTR_LOOP(::send(sockfd, buf, len, flags));
}


static inline ssize_t tf_recv(int sockfd, void *buf, size_t len, int flags)
{
    TF_EINTR_LOOP(::recv(sockfd, buf, len, flags));
}


static inline int tf_flock(int fd, int op)
{
#ifdef Q_CC_MSVC
    Q_UNUSED(fd);
    Q_UNUSED(op);
    return 0;
#else
    TF_EINTR_LOOP(::flock(fd, op));
#endif
}


static inline int tf_fileno(FILE *stream)
{
#ifdef Q_CC_MSVC
    return ::_fileno(stream);
#else
    return ::fileno(stream);
#endif
}


#ifdef Q_OS_UNIX
# include "tfcore_unix.h"
#endif

#endif // TFCORE_H
