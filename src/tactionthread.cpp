/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QCoreApplication>
#include <QEventLoop>
#include <TActionThread>
#include <TWebApplication>
#include <THttpRequest>
#include <TSession>
#include <TApplicationServerBase>
#include <TAppSettings>
#include <atomic>
#include "thttpsocket.h"
#include "twebsocket.h"
#include "tsystemglobal.h"
#include "tsessionmanager.h"
#include "tfcore.h"

static std::atomic<int> threadCounter(0);
static int keepAliveTimeout = -1;


int TActionThread::threadCount()
{
    return threadCounter.load();
}


bool TActionThread::waitForAllDone(int msec)
{
    int cnt;
    QTime time;
    time.start();

    while ((cnt = threadCount()) > 0) {
        if (time.elapsed() > msec) {
            break;
        }

        Tf::msleep(10);
        qApp->processEvents();
    }
    return cnt == 0;
}

/*!
  \class TActionThread
  \brief The TActionThread class provides a thread context.
*/

TActionThread::TActionThread(int socket)
    : QThread(), TActionContext(), httpSocket(nullptr)
{
    ++threadCounter;
    TActionContext::socketDesc = socket;

    if (keepAliveTimeout < 0) {
        int timeout = Tf::appSettings()->value(Tf::HttpKeepAliveTimeout, "10").toInt();
        keepAliveTimeout = qMax(timeout, 0);
    }
}


TActionThread::~TActionThread()
{
    if (httpSocket) {
        httpSocket->deleteLater();
    }

    if (TActionContext::socketDesc > 0)
        tf_close(TActionContext::socketDesc);

    --threadCounter;
}


void TActionThread::run()
{
    QList<THttpRequest> reqs;
    QEventLoop eventLoop;
    httpSocket = new THttpSocket();

    if (Q_UNLIKELY(!httpSocket->setSocketDescriptor(TActionContext::socketDesc))) {
        emitError(httpSocket->error());
        goto socket_error;
    }
    TActionContext::socketDesc = 0;

    for (;;) {
        reqs = readRequest(httpSocket);
        tSystemDebug("HTTP request count: %d", reqs.count());

        if (Q_UNLIKELY(reqs.isEmpty())) {
            break;
        }

        for (auto &req : reqs) {
            // WebSocket?
            QByteArray connectionHeader = req.header().rawHeader("Connection").toLower();
            if (Q_UNLIKELY(connectionHeader.contains("upgrade"))) {
                QByteArray upgradeHeader = req.header().rawHeader("Upgrade").toLower();
                tSystemDebug("Upgrade: %s", upgradeHeader.data());
                if (upgradeHeader == "websocket") {
                    // Switch to WebSocket
                    if (!handshakeForWebSocket(req.header())) {
                        goto socket_error;
                    }
                }

                goto socket_cleanup;
            }

            TActionContext::execute(req, httpSocket->socketId());

            httpSocket->flush();  // Flush socket
            TActionContext::release();
        }

        if (keepAliveTimeout == 0) {
            break;
        }

        // Next request
        while (!httpSocket->waitForReadyRead(100)) {
            if (httpSocket->state() != QAbstractSocket::ConnectedState) {
                if (httpSocket->error() != QAbstractSocket::RemoteHostClosedError) {
                    tSystemWarn("Error occurred : error:%d  socket:%d", httpSocket->error(), httpSocket->socketId());
                }
                goto receive_end;
            }

            if (httpSocket->idleTime() >= keepAliveTimeout) {
                tSystemDebug("KeepAlive timeout : socket:%d", httpSocket->socketId());
                goto receive_end;
            }

            while (eventLoop.processEvents(QEventLoop::ExcludeSocketNotifiers)) {}
        }
    }

receive_end:
    closeHttpSocket();  // disconnect

socket_cleanup:
    // For cleanup
    while (eventLoop.processEvents()) {}

socket_error:
    httpSocket->deleteLater();
    httpSocket = nullptr;
}


void TActionThread::emitError(int socketError)
{
    emit error(socketError);
}


QList<THttpRequest> TActionThread::readRequest(THttpSocket *socket)
{
    QList<THttpRequest> reqs;
    while (!socket->canReadRequest()) {
        // Check idle timeout
        if (Q_UNLIKELY(keepAliveTimeout > 0 && socket->idleTime() >= keepAliveTimeout)) {
            tSystemWarn("Reading a socket timed out after %d seconds. Descriptor:%d", keepAliveTimeout, (int)socket->socketDescriptor());
            break;
        }

        if (Q_UNLIKELY(socket->state() != QAbstractSocket::ConnectedState)) {
            tSystemWarn("Invalid descriptor (disconnected) : %d", (int)socket->socketDescriptor());
            break;
        }

        socket->waitForReadyRead(200);  // Repeats per 200 msecs
    }

    if (Q_UNLIKELY(!socket->canReadRequest())) {
        socket->abort();
    } else {
        reqs = socket->read();
    }

    return reqs;
}


qint64 TActionThread::writeResponse(THttpResponseHeader &header, QIODevice *body)
{
    if (keepAliveTimeout > 0) {
        header.setRawHeader("Connection", "Keep-Alive");
    }
    return httpSocket->write(static_cast<THttpHeader*>(&header), body);
}


void TActionThread::closeHttpSocket()
{
    httpSocket->close();
}


bool TActionThread::handshakeForWebSocket(const THttpRequestHeader &header)
{
    if (!TWebSocket::searchEndpoint(header)) {
        return false;
    }

    // Switch to WebSocket
    int sd = TApplicationServerBase::duplicateSocket(httpSocket->socketDescriptor());
    TWebSocket *ws = new TWebSocket(sd, httpSocket->peerAddress(), header);
    connect(ws, SIGNAL(disconnected()), ws, SLOT(deleteLater()));
    ws->moveToThread(Tf::app()->thread());

    // WebSocket opening
    TSession session;
    QByteArray sessionId = header.cookie(TSession::sessionName());
    if (!sessionId.isEmpty()) {
        // Finds a session
        session = TSessionManager::instance().findSession(sessionId);
    }

    ws->startWorkerForOpening(session);
    return true;
}
