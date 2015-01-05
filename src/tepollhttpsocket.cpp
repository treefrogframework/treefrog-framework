/* Copyright (c) 2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TSystemGlobal>
#include "tepollhttpsocket.h"
#include "tactionworker.h"


TEpollHttpSocket::TEpollHttpSocket(int socketDescriptor, int id, const QHostAddress &address)
    : TEpollSocket(socketDescriptor, id, address), recvBuf()
{ }


TEpollHttpSocket::~TEpollHttpSocket()
{ }


bool TEpollHttpSocket::canReadRequest()
{
    return recvBuf.canReadHttpRequest();
}


bool TEpollHttpSocket::upgradeConnectionReceived() const
{
    return false;
}


TEpollSocket *TEpollHttpSocket::switchProtocol()
{
    return NULL;
}


int TEpollHttpSocket::write(const char *data, int len)
{
    return recvBuf.write(data, len);
}


void TEpollHttpSocket::startWorker()
{
    TActionWorker *worker = new TActionWorker(this);
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    worker->start();
}
