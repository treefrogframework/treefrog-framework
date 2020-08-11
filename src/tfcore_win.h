#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <io.h>
#include <winbase.h>
#include "tfcore.h"

#ifndef Q_OS_WIN
#error "tfcore_win.h included on a non-Windows system"
#endif

namespace {

inline int tf_poll(pollfd *fds, int nfds, int timeout)
{
    return ::WSAPoll(fds, nfds, timeout);
}

/*!
  Waits for receive-event on a descriptor.
  Return: 0:timeout  1:event  -1:error
 */
inline int tf_poll_recv(int socket, int timeout)
{
    pollfd pfd = {(SOCKET)socket, POLLIN, 0};
    int ret = tf_poll(&pfd, 1, timeout);
    return ret;
}

/*!
  Waits for write-event on a descriptor.
  Return: 0:timeout  1:event  -1:error
 */
inline int tf_poll_send(int socket, int timeout)
{
    pollfd pfd = {(SOCKET)socket, POLLOUT, 0};
    int ret = tf_poll(&pfd, 1, timeout);
    return ret;
}


inline int tf_close(int fd)
{
    return ::_close(fd);
}


inline int tf_read(int fd, void *buf, size_t count)
{
    return ::_read(fd, buf, (uint)count);
}


inline int tf_write(int fd, const void *buf, size_t count)
{
    return ::_write(fd, buf, (uint)count);
}


inline int tf_send(int sockfd, const void *buf, size_t len, int flags = 0)
{
    Q_UNUSED(flags);
    return ::send((SOCKET)sockfd, (const char *)buf, (int)len, 0);
}


inline int tf_recv(int sockfd, void *buf, size_t len, int flags = 0)
{
    Q_UNUSED(flags);
    return ::recv((SOCKET)sockfd, (char *)buf, (int)len, 0);
}


inline int tf_close_socket(int sockfd)
{
    return ::closesocket((SOCKET)sockfd);
}


inline int tf_dup(int fd)
{
    return ::_dup(fd);
}


inline int tf_flock(int fd, int op)
{
    Q_UNUSED(fd);
    Q_UNUSED(op);
    return 0;
}

// advisory lock. exclusive:true=exclusive lock, false=shared lock
inline int tf_lockfile(int fd, bool exclusive, bool blocking)
{
    auto handle = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
    DWORD dwFlags = (exclusive) ? LOCKFILE_EXCLUSIVE_LOCK : 0;
    dwFlags |= (blocking) ? 0 : LOCKFILE_FAIL_IMMEDIATELY;
    OVERLAPPED ov;
    memset(&ov, 0, sizeof(OVERLAPPED));
    BOOL res = LockFileEx(handle, dwFlags, 0, 0, 0, &ov);
    return (res) ? 0 : -1;
}


inline int tf_unlink(const char *pathname)
{
    return ::_unlink(pathname);
}


inline int tf_fileno(FILE *stream)
{
    return ::_fileno(stream);
}

}  // namespace
