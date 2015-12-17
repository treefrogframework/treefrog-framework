/* Copyright (c) 2010-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <QTcpServer>
#include <QFile>
#include <TSystemGlobal>
#include "tapplicationserverbase.h"
#include "tfcore_unix.h"


void TApplicationServerBase::nativeSocketInit()
{ }


void TApplicationServerBase::nativeSocketCleanup()
{ }

/*!
  Listen a port for connections on a socket.
  This function must be called in a tfmanager process.
 */
int TApplicationServerBase::nativeListen(const QHostAddress &address, quint16 port, OpenFlag flag)
{
    int sd = 0;
    QTcpServer server;

    if (!server.listen(address, port)) {
        tSystemError("Listen failed  port:%d", port);
        return sd;
    }

    sd = duplicateSocket(server.socketDescriptor()); // duplicate

    if (flag == CloseOnExec) {
        ::fcntl(sd, F_SETFD, ::fcntl(sd, F_GETFD) | FD_CLOEXEC);
    } else {
        ::fcntl(sd, F_SETFD, 0);  // clear
    }
    ::fcntl(sd, F_SETFL, ::fcntl(sd, F_GETFL) | O_NONBLOCK);  // non-block

    server.close();
    return sd;
}

/*!
  Listen for connections on UNIX domain.
 */
int TApplicationServerBase::nativeListen(const QString &fileDomain, OpenFlag flag)
{
    int sd = -1;
    struct sockaddr_un addr;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = PF_UNIX;
    if (sizeof(addr.sun_path) < (uint)fileDomain.toLatin1().size() + 1) {
        tSystemError("too long name for UNIX domain socket  [%s:%d]", __FILE__, __LINE__);
        return sd;
    }
    strncpy(addr.sun_path, fileDomain.toLatin1().data(), sizeof(addr.sun_path));

    // create unix domain socket
    sd = ::socket(PF_UNIX, SOCK_STREAM, 0);
    if (sd < 0) {
        tSystemError("Socket create failed  [%s:%d]", __FILE__, __LINE__);
        return sd;
    }

    if (flag == CloseOnExec) {
        ::fcntl(sd, F_SETFD, FD_CLOEXEC); // set close-on-exec flag
    }
    ::fcntl(sd, F_SETFL, ::fcntl(sd, F_GETFL) | O_NONBLOCK);  // non-block

    QFile file(fileDomain);
    if (file.exists()) {
        file.remove();
        tSystemWarn("File for UNIX domain socket removed: %s", qPrintable(fileDomain));
    }

    // Bind
    if (::bind(sd, (sockaddr *)&addr, sizeof(sockaddr_un)) < 0) {
        tSystemError("Bind failed  [%s:%d]", __FILE__, __LINE__);
        goto socket_error;
    }
    file.setPermissions((QFile::Permissions)0x777);

    // Listen
    if (::listen(sd, 50) < 0) {
        tSystemError("Listen failed  [%s:%d]", __FILE__, __LINE__);
        goto socket_error;
    }

    return sd;

socket_error:
    nativeClose(sd);
    return -1;
}


void TApplicationServerBase::nativeClose(int socket)
{
    if (socket > 0)
        tf_close(socket);
}


int TApplicationServerBase::duplicateSocket(int socketDescriptor)
{
    return ::fcntl(socketDescriptor, F_DUPFD, 0);  // duplicate
}
