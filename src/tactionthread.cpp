/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tfcore.h"
#include "thttpsocket.h"
#include "tsessionmanager.h"
#include "tsystemglobal.h"
#include "twebsocket.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <TActionThread>
#include <TAppSettings>
#include <TApplicationServerBase>
#include <THttpRequest>
#include <TSession>
#include <TWebApplication>
#include <atomic>

constexpr int RECV_BUF_SIZE = 8 * 1024;

namespace {
std::atomic<int> threadCounter(0);
}


int TActionThread::threadCount()
{
    return threadCounter.load();
}


bool TActionThread::waitForAllDone(int msec)
{
    int cnt;
    QElapsedTimer time;
    time.start();

    while ((cnt = threadCount()) > 0) {
        if (time.elapsed() > msec) {
            break;
        }

        Tf::msleep(5);
        qApp->processEvents();
    }
    tSystemDebug("waitForAllDone : remaining:%d", cnt);
    return cnt == 0;
}

/*!
  \class TActionThread
  \brief The TActionThread class provides a thread context.
*/

TActionThread::TActionThread(int socket, int maxThreads) :
    QThread(),
    TActionContext(),
    _maxThreads(maxThreads)
{
    TActionContext::socketDesc = socket;
    _readBuffer.reserve(RECV_BUF_SIZE);
}


TActionThread::~TActionThread()
{
    if (_httpSocket) {
        _httpSocket->deleteLater();
    }

    if (TActionContext::socketDesc > 0) {
        tf_close_socket(TActionContext::socketDesc);
    }
}


void TActionThread::setSocketDescriptor(qintptr socket)
{
    if (TActionContext::socketDesc > 0) {
        tSystemWarn("Socket still open : %d   [%s:%d]", TActionContext::socketDesc, __FILE__, __LINE__);
        tf_close_socket(TActionContext::socketDesc);
    }
    TActionContext::socketDesc = (int)socket;
}


void TActionThread::run()
{
    class Counter {
        std::atomic<int> &_num;

    public:
        Counter(std::atomic<int> &n) :
            _num(n) { _num++; }
        ~Counter() { _num--; }
    };

    Counter counter(threadCounter);
    QEventLoop eventLoop;
    _httpSocket = new THttpSocket(_readBuffer);
    if (TActionContext::socketDesc > 0) {
        _httpSocket->setSocketDescriptor(TActionContext::socketDesc, QAbstractSocket::ConnectedState);
    }
    TActionContext::socketDesc = 0;
    TDatabaseContext::setCurrentDatabaseContext(this);

    try {
        for (;;) {
            QList<THttpRequest> requests = readRequest(_httpSocket);
            tSystemDebug("HTTP request count: %lld", (qint64)requests.count());

            if (requests.isEmpty()) {
                break;
            }

            // WebSocket?
            QByteArray connectionHeader = requests[0].header().rawHeader(QByteArrayLiteral("Connection")).toLower();
            if (Q_UNLIKELY(connectionHeader.contains("upgrade"))) {
                QByteArray upgradeHeader = requests[0].header().rawHeader(QByteArrayLiteral("Upgrade")).toLower();
                tSystemDebug("Upgrade: %s", upgradeHeader.data());
                if (upgradeHeader == "websocket") {
                    // Switch to WebSocket
                    if (!handshakeForWebSocket(requests[0].header())) {
                        goto socket_error;
                    }
                }
                goto socket_cleanup;
            }

            for (auto &req : requests) {
                TActionContext::execute(req, _httpSocket->socketId());
            }

            if (keepAliveTimeout() == 0) {
                break;
            }

            if (threadCount() >= _maxThreads && _maxThreads > 0) {
                // Do not keep-alive
                break;
            }

            if (_httpSocket->state() != QAbstractSocket::ConnectedState) {
                goto receive_end;
            }
        }

    } catch (ClientErrorException &e) {
        tWarn("Caught ClientErrorException: status code:%d", e.statusCode());
        tSystemWarn("Caught ClientErrorException: status code:%d", e.statusCode());
        THttpResponseHeader header;
        TActionContext::writeResponse(e.statusCode(), header);
    } catch (std::exception &e) {
        tError("Caught Exception: %s", e.what());
        tSystemError("Caught Exception: %s", e.what());
        THttpResponseHeader header;
        TActionContext::writeResponse(Tf::InternalServerError, header);
    }

receive_end:
    closeHttpSocket();  // disconnect

socket_cleanup:
    // For cleanup
    while (eventLoop.processEvents()) {
    }

socket_error:
    TActionContext::socketDesc = 0;
    TActionContext::release();
    TDatabaseContext::setCurrentDatabaseContext(nullptr);
    _httpSocket->deleteLater();
    _httpSocket = nullptr;
}


void TActionThread::emitError(int socketError)
{
    emit error(socketError);
}


QList<THttpRequest> TActionThread::readRequest(THttpSocket *socket)
{
    QList<THttpRequest> reqs;

    for (;;) {
        if (socket->waitForReadyReadRequest(500)) {
            reqs = socket->read();
            if (!reqs.isEmpty()) {
                return reqs;
            } else {
                break;
            }
        }

        if (Q_UNLIKELY(socket->state() != QAbstractSocket::ConnectedState)) {
            break;
        }

        // Check idle timeout
        if (Q_UNLIKELY(keepAliveTimeout() > 0 && socket->idleTime() >= keepAliveTimeout())) {
            tSystemWarn("Reading a socket timed out after %d seconds. Descriptor:%d", keepAliveTimeout(), (int)socket->socketDescriptor());
            break;
        }
    }

    socket->abort();
    return QList<THttpRequest>();
}


qint64 TActionThread::writeResponse(THttpResponseHeader &header, QIODevice *body)
{
    if (keepAliveTimeout() > 0) {
        header.setRawHeader(QByteArrayLiteral("Connection"), QByteArrayLiteral("Keep-Alive"));
    }
    return _httpSocket->write(static_cast<THttpHeader *>(&header), body);
}


void TActionThread::closeHttpSocket()
{
    _httpSocket->abort();
}


bool TActionThread::handshakeForWebSocket(const THttpRequestHeader &header)
{
    if (!TWebSocket::searchEndpoint(header)) {
        return false;
    }

    // Switch to WebSocket
    int sd = TApplicationServerBase::duplicateSocket(_httpSocket->socketDescriptor());
    TWebSocket *ws = new TWebSocket(sd, _httpSocket->peerAddress(), header);
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
