/* Copyright (c) 2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TWebApplication>
#include <THttpRequestHeader>
#include "thttpbuffer.h"
#include "tsystemglobal.h"


THttpBuffer::THttpBuffer()
    : lengthToRead(-1)
{
    httpBuffer.reserve(1024);
}


THttpBuffer::~THttpBuffer()
{ }


THttpBuffer::THttpBuffer(const THttpBuffer &other)
    : httpBuffer(other.httpBuffer),
      lengthToRead(other.lengthToRead),
      clientAddr(other.clientAddr)
{ }


THttpBuffer &THttpBuffer::operator=(const THttpBuffer &other)
{
    httpBuffer = other.httpBuffer;
    lengthToRead = other.lengthToRead;
    clientAddr = other.clientAddr;
    return *this;
}


QByteArray THttpBuffer::read(int maxSize)
{
    int size = qMin(httpBuffer.length(), maxSize);
    QByteArray res = httpBuffer.left(size);
    httpBuffer.remove(0, size);
    return res;
}


int THttpBuffer::read(char *data, int maxSize)
{
    int size = qMin(httpBuffer.length(), maxSize);
    memcpy(data, httpBuffer.data(), size);
    httpBuffer.remove(0, size);
    return size;
}


int THttpBuffer::write(const char *data, int maxSize)
{
    httpBuffer.append(data, maxSize);
    parse();
    return maxSize;
}


int THttpBuffer::write(const QByteArray &byteArray)
{
    int len = byteArray.length();
    httpBuffer += byteArray;
    parse();
    return len;
}


void THttpBuffer::parse()
{
    uint limitBodyBytes = Tf::app()->appSettings().value("LimitRequestBody", "0").toUInt();

    if (lengthToRead > 0) {
        int idx = httpBuffer.indexOf("\r\n\r\n") + 4;
        int len = qMin(httpBuffer.length() - idx, (int)lengthToRead);
        lengthToRead -= len;

    } else if (lengthToRead < 0) {
        int idx = httpBuffer.indexOf("\r\n\r\n");
        if (idx > 0) {
            THttpRequestHeader header(httpBuffer.left(idx + 4));
            tSystemDebug("content-length: %d", header.contentLength());

            if (limitBodyBytes > 0 && header.contentLength() > limitBodyBytes) {
                throw ClientErrorException(413);  // Request Entity Too Large
            }

            lengthToRead = qMax(idx + 4 + (qint64)header.contentLength() - httpBuffer.length(), 0LL);
        }
    } else {
        tSystemWarn("Not reachable");
    }
}


bool THttpBuffer::canReadHttpRequest() const
{
    return (lengthToRead == 0);
}


void THttpBuffer::clear()
{
    lengthToRead = -1;
    httpBuffer.truncate(0);
    httpBuffer.reserve(1024);
    clientAddr.clear();
}
