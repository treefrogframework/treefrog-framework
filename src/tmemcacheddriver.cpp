/* Copyright (c) 2022, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tmemcacheddriver.h"
#include "tsystemglobal.h"


TMemcachedDriver::TMemcachedDriver() :
    TKvsDriver()
{
}


bool TMemcachedDriver::command(const QByteArray &cmd)
{
    QByteArray response;
    tSystemDebug("memcached command: {}", cmd.data());
    request(cmd, 0);
    return true;
}

/*!
  Sends the \a command over this connection, waiting for a reply up to \a msecs
  milliseconds and returns the reply. If \a msecs is 0, this function will return
  immediately.
*/
QByteArray TMemcachedDriver::request(const QByteArray &command, int msecs)
{
    if (Q_UNLIKELY(!isOpen())) {
        tSystemError("Not open memcached session  [{}:{}]", __FILE__, __LINE__);
        return QByteArray();
    }

    if (!writeCommand(command)) {
        tSystemError("memcached write error  [{}:{}]", __FILE__, __LINE__);
        close();
        return QByteArray();
    }

    if (msecs <= 0) {
        return QByteArray();
    }

    return readReply(msecs);
}
