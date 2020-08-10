#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include "tfcore.h"

#ifndef Q_OS_WINDOWS
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

}  // namespace
