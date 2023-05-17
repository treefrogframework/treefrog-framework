/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsendbuffer.h"
#include "tsystemglobal.h"
#include <TfCore>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QLocale>
#include <THttpResponseHeader>
#include <THttpUtility>
#include <TWebApplication>


TSendBuffer::TSendBuffer(const QByteArray &header, const QFileInfo &file, bool autoRemove, TAccessLogger &&logger) :
    _arrayBuffer(header),
    _fileRemove(autoRemove),
    _accesslogger(std::move(logger))
{
    if (file.exists() && file.isFile()) {
        _bodyFile = new QFile(file.absoluteFilePath());
        if (!_bodyFile->open(QIODevice::ReadOnly)) {
            tSystemWarn("file open failed: %s", qUtf8Printable(file.absoluteFilePath()));
            release();
        }
    }
}


TSendBuffer::TSendBuffer(const QByteArray &header) :
    _arrayBuffer(header)
{
}


TSendBuffer::TSendBuffer(int statusCode, const QHostAddress &address, const QByteArray &method)
{
    _accesslogger.open();
    _accesslogger.setStatusCode(statusCode);
    _accesslogger.setTimestamp(QDateTime::currentDateTime());
    _accesslogger.setRemoteHost(address.toString().toLatin1());
    _accesslogger.setRequest(method);
    _accesslogger.startElapsedTimer();

    THttpResponseHeader header;
    header.setStatusLine(statusCode, THttpUtility::getResponseReasonPhrase(statusCode));
    header.setRawHeader("Server", "TreeFrog server");
    header.setCurrentDate();

    _arrayBuffer += header.toByteArray();
}


TSendBuffer::~TSendBuffer()
{
    release();
}


void TSendBuffer::release()
{
    if (_bodyFile) {
        if (_fileRemove) {
            _bodyFile->remove();
        }
        delete _bodyFile;
        _bodyFile = nullptr;
    }
    _accesslogger.close();
}


void *TSendBuffer::getData(int &size)
{
    if (Q_UNLIKELY(size <= 0)) {
        tSystemError("Invalid data size. [%s:%d]", __FILE__, __LINE__);
        return nullptr;
    }

    if (_startPos < _arrayBuffer.length()) {
        size = std::min((int)_arrayBuffer.length() - _startPos, size);
        return _arrayBuffer.data() + _startPos;
    }

    if (!_bodyFile || _bodyFile->atEnd()) {
        size = 0;
        return nullptr;
    }

    _arrayBuffer.reserve(size);
    size = _bodyFile->read(_arrayBuffer.data(), size);
    if (Q_UNLIKELY(size < 0)) {
        tSystemError("file read error: %s", qUtf8Printable(_bodyFile->fileName()));
        size = 0;
        release();
        return nullptr;
    }

    _arrayBuffer.resize(size);
    _startPos = 0;
    return _arrayBuffer.data();
}


bool TSendBuffer::seekData(int pos)
{
    if (Q_UNLIKELY(pos < 0)) {
        return false;
    }

    if (_startPos + pos >= _arrayBuffer.length()) {
        _arrayBuffer.truncate(0);
        _startPos = 0;
    } else {
        _startPos += pos;
    }
    return true;
}


int TSendBuffer::prepend(const char *data, int maxSize)
{
    if (_startPos > 0) {
        _arrayBuffer.remove(0, _startPos);
    }
    _arrayBuffer.prepend(data, maxSize);
    _startPos = 0;
    return maxSize;
}


bool TSendBuffer::atEnd() const
{
    return _startPos >= _arrayBuffer.length() && (!_bodyFile || _bodyFile->atEnd());
}
