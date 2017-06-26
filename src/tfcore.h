#ifndef TFCORE_H
#define TFCORE_H

#include <QtGlobal>
#include <cstdio>
#include <cerrno>
#ifdef Q_OS_WIN
# include <windows.h>
# include <io.h>
# include <winbase.h>
#else
# include <unistd.h>
# include <fcntl.h>
# include <sys/file.h>
# include <sys/socket.h>
#endif

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


static inline int tf_dup(int fd)
{
#ifdef Q_OS_WIN
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

// advisory lock. exclusive:true=exclusive lock, false=shared lock
static inline int tf_lockfile(int fd, bool exclusive, bool blocking)
{
#ifdef Q_OS_WIN
    auto handle = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
    DWORD dwFlags = (exclusive) ? LOCKFILE_EXCLUSIVE_LOCK : 0;
    dwFlags |= (blocking) ? 0 : LOCKFILE_FAIL_IMMEDIATELY;
    OVERLAPPED ov;
    memset(&ov, 0, sizeof(OVERLAPPED));
    BOOL res = LockFileEx(handle, dwFlags, 0, 0, 0, &ov);
    return (res) ? 0 : -1;
#else
    struct flock lck;

    memset(&lck, 0, sizeof(struct flock));
    lck.l_type = (exclusive) ? F_WRLCK : F_RDLCK;
    lck.l_whence = SEEK_SET;
    auto cmd = (blocking) ? F_SETLKW : F_SETLK;
    TF_EINTR_LOOP(::fcntl(fd, cmd, &lck));
#endif
}


static inline int tf_unlink(const char *pathname)
{
#ifdef Q_OS_WIN
    return ::_unlink(pathname);
#else
    return ::unlink(pathname);
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
