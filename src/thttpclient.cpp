/* Copyright (c) 2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "thttpclient.h"
#include <QThread>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QElapsedTimer>


namespace {
    bool waitForReadyRead(QNetworkReply *reply, int msecs)
    {
        bool ret = true;
        QEventLoop eventLoop;
        QElapsedTimer idleTimer;
        int bytes = 0;

        idleTimer.start();
        while (!reply->isFinished()) {
            if (bytes != reply->bytesAvailable()) {
                idleTimer.restart();
                bytes = reply->bytesAvailable();
            }

            if (idleTimer.elapsed() >= msecs) {
                ret = false;
                break;
            }
            eventLoop.processEvents();
        }
        return ret;
    }
}


THttpClient::THttpClient() :
    _manager(new QNetworkAccessManager(QThread::currentThread()->parent()))
{ }


THttpClient::~THttpClient()
{
    _manager->deleteLater();
}


QNetworkReply *THttpClient::get(const QUrl &url, int msecs)
{
    return get(QNetworkRequest(url), msecs);
}


QNetworkReply *THttpClient::get(const QNetworkRequest &request, int msecs)
{
    QNetworkReply *reply = _manager->get(request);

    if (! waitForReadyRead(reply, msecs)) {
        reply->readAll();  // clear data
    }
    return reply;
}


QNetworkReply *THttpClient::post(const QUrl &url, const QByteArray &data, int msecs)
{
    return post(QNetworkRequest(url), data, msecs);
}


QNetworkReply *THttpClient::post(const QNetworkRequest &request, const QByteArray &data, int msecs)
{
    QNetworkReply *reply = _manager->post(request, data);

    if (! waitForReadyRead(reply, msecs)) {
        reply->readAll();  // clear data
    }
    return reply;
}


QNetworkReply *THttpClient::put(const QUrl &url, const QByteArray &data, int msecs)
{
    return put(QNetworkRequest(url), data, msecs);
}


QNetworkReply *THttpClient::put(const QNetworkRequest &request, const QByteArray &data, int msecs)
{
    QNetworkReply *reply = _manager->put(request, data);

    if (! waitForReadyRead(reply, msecs)) {
        reply->readAll();  // clear data
    }
    return reply;
}


QNetworkReply *THttpClient::deleteResource(const QUrl &url, int msecs)
{
    return deleteResource(QNetworkRequest(url), msecs);
}


QNetworkReply *THttpClient::deleteResource(const QNetworkRequest &request, int msecs)
{
    QNetworkReply *reply = _manager->deleteResource(request);

    if (! waitForReadyRead(reply, msecs)) {
        reply->readAll();  // clear data
    }
    return reply;
}
