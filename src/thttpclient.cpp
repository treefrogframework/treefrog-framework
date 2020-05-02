/* Copyright (c) 2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "thttpclient.h"
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>

/*!
  \class THttpClient
  \brief The THttpClient class can send HTTP requests to another server
  and receive replies.
  \sa https://doc.qt.io/qt-5/qnetworkaccessmanager.html
*/

namespace {
bool waitForReadyRead(QNetworkReply *reply, int msecs)
{
    QEventLoop eventLoop;
    QElapsedTimer idleTimer;
    int bytes = 0;

    idleTimer.start();
    do {
        if (bytes != reply->bytesAvailable()) {
            idleTimer.restart();
            bytes = reply->bytesAvailable();
        }

        eventLoop.processEvents();
    } while (!reply->isFinished() && idleTimer.elapsed() < msecs);

    return reply->isFinished();
}
}


THttpClient::THttpClient() :
    _manager(new QNetworkAccessManager(QThread::currentThread()->parent()))
{
}


THttpClient::~THttpClient()
{
    _manager->deleteLater();
}

/*!
  Posts a request to obtain the contents of the target request and returns a
  new QNetworkReply object opened for reading.
 */
QNetworkReply *THttpClient::get(const QUrl &url, int msecs)
{
    return get(QNetworkRequest(url), msecs);
}

/*!
  Posts a request to obtain the contents of the target request and returns a
  new QNetworkReply object opened for reading.
 */
QNetworkReply *THttpClient::get(const QNetworkRequest &request, int msecs)
{
    QNetworkReply *reply = _manager->get(request);

    if (!waitForReadyRead(reply, msecs)) {
        reply->readAll();  // clear data
    }
    return reply;
}

/*!
  Sends an HTTP POST request to the destination specified by \a url and
  returns a new QNetworkReply object opened for reading.
 */
QNetworkReply *THttpClient::post(const QUrl &url, const QJsonDocument &json, int msecs)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return post(request, json.toJson(QJsonDocument::Compact), msecs);
}

/*!
  Sends an HTTP POST request to the destination specified by \a request and
  returns a new QNetworkReply object opened for reading.
 */
QNetworkReply *THttpClient::post(const QNetworkRequest &request, const QByteArray &data, int msecs)
{
    QNetworkReply *reply = _manager->post(request, data);

    if (!waitForReadyRead(reply, msecs)) {
        reply->readAll();  // clear data
    }
    return reply;
}

/*!
  Uploads the contents of \a json to the destination \a url and returns a new
  QNetworkReply object that will be open for reply.
 */
QNetworkReply *THttpClient::put(const QUrl &url, const QJsonDocument &json, int msecs)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return put(request, json.toJson(QJsonDocument::Compact), msecs);
}

/*!
  Uploads the contents of \a data to the destination \a request and returns a
  new QNetworkReply object that will be open for reply.
 */
QNetworkReply *THttpClient::put(const QNetworkRequest &request, const QByteArray &data, int msecs)
{
    QNetworkReply *reply = _manager->put(request, data);

    if (!waitForReadyRead(reply, msecs)) {
        reply->readAll();  // clear data
    }
    return reply;
}

/*!
  Sends a request to delete the resource identified by the URL \a url.
 */
QNetworkReply *THttpClient::deleteResource(const QUrl &url, int msecs)
{
    return deleteResource(QNetworkRequest(url), msecs);
}

/*!
  Sends a request to delete the resource identified by the URL of \a request.
 */
QNetworkReply *THttpClient::deleteResource(const QNetworkRequest &request, int msecs)
{
    QNetworkReply *reply = _manager->deleteResource(request);

    if (!waitForReadyRead(reply, msecs)) {
        reply->readAll();  // clear data
    }
    return reply;
}
