/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tapplicationserverbase.h"
#include <TSystemGlobal>
#include <winsock2.h>


void TApplicationServerBase::nativeSocketInit()
{
    WSAData wsadata;
    if (WSAStartup(MAKEWORD(2, 0), &wsadata) != 0) {
        tSystemWarn("WinSock v2.0 initialization failed");
    }
}


void TApplicationServerBase::nativeSocketCleanup()
{
    WSACleanup();
}

/*!
  Listen a port with SO_REUSEADDR option.
  This function must be called in a tfserver process.
 */
int TApplicationServerBase::nativeListen(const QHostAddress &address, quint16 port, OpenFlag)
{
    int protocol = (address.protocol() == QAbstractSocket::IPv6Protocol) ? AF_INET6 : AF_INET;
    SOCKET sock = ::WSASocket(protocol, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (sock == INVALID_SOCKET) {
        tSystemError("WSASocket Error: %d", WSAGetLastError());
        return -1;
    }

    // ReuseAddr
    bool on = true;
    if (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) != 0) {
        tSystemError("setsockopt error: %d", WSAGetLastError());
        goto error_socket;
    }

    if (address.protocol() == QAbstractSocket::IPv6Protocol) {
        struct tf_in6_addr {
            quint8 tf_s6_addr[16];
        };
        struct tf_sockaddr_in6 {
            short sin6_family; /* AF_INET6 */
            quint16 sin6_port; /* Transport level port number */
            quint32 sin6_flowinfo; /* IPv6 flow information */
            struct tf_in6_addr sin6_addr; /* IPv6 address */
            quint32 sin6_scope_id; /* set of interfaces for a scope */
        } sa6;

        memset(&sa6, 0, sizeof(sa6));
        sa6.sin6_family = AF_INET6;
        WSAHtons(sock, port, &(sa6.sin6_port));
        Q_IPV6ADDR ipv6 = address.toIPv6Address();
        memcpy(&(sa6.sin6_addr.tf_s6_addr), &ipv6, sizeof(ipv6));
        if (::bind(sock, (struct sockaddr *)&sa6, sizeof(sa6)) != 0) {
            tSystemError("bind(v6) error: %d", WSAGetLastError());
            goto error_socket;
        }

    } else if (address.protocol() == QAbstractSocket::IPv4Protocol
        || address.protocol() == QAbstractSocket::AnyIPProtocol) {
        struct sockaddr_in sa;
        memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        WSAHtons(sock, port, &(sa.sin_port));
        WSAHtonl(sock, address.toIPv4Address(), &(sa.sin_addr.s_addr));
        if (::bind(sock, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
            tSystemError("bind error: %d", WSAGetLastError());
            goto error_socket;
        }
    } else {  // UnknownNetworkLayerProtocol
        goto error_socket;
    }

    if (::listen(sock, SOMAXCONN) != 0) {
        tSystemError("listen error: %d", WSAGetLastError());
        goto error_socket;
    }
    return sock;

error_socket:
    nativeClose(sock);
    return -1;
}


int TApplicationServerBase::nativeListen(const QString &, OpenFlag)
{
    // must not reach here
    Q_ASSERT(0);
    return 0;
}


void TApplicationServerBase::nativeClose(int socket)
{
    if (socket != (int)INVALID_SOCKET)
        closesocket(socket);
}


int TApplicationServerBase::duplicateSocket(int socketDescriptor)
{
    WSAPROTOCOL_INFO pi;
    ::WSADuplicateSocket(socketDescriptor, ::GetCurrentProcessId(), &pi);
    SOCKET newsock = ::WSASocket(pi.iAddressFamily, pi.iSocketType, pi.iProtocol, &pi, 0, 0);
    return newsock;
}
