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
    tSystemDebug("memcached command: %s", cmd.data());
    return request(cmd, response);
}


bool TMemcachedDriver::request(const QByteArray &command, QByteArray &response)
{
    if (Q_UNLIKELY(!isOpen())) {
        tSystemError("Not open memcached session  [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    if (!writeCommand(command)) {
        tSystemError("memcached write error  [%s:%d]", __FILE__, __LINE__);
        close();
        return false;
    }

    response = readReply();
    return !response.isEmpty();
}
