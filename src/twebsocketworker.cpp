/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QDataStream>
#include <TWebApplication>
#include <TDispatcher>
#include <TWebSocketEndpoint>
#include <THttpRequestHeader>
#include "twebsocketworker.h"
#include "tsystemglobal.h"
#include "turlroute.h"
#include "tabstractwebsocket.h"
#include "tpublisher.h"
#include "thttpsocket.h"
#ifdef Q_OS_LINUX
# include "tepollhttpsocket.h"
#endif


TWebSocketWorker::TWebSocketWorker(TWebSocketWorker::RunMode m, TAbstractWebSocket *s, const QByteArray &path, QObject *parent)
    : QThread(parent), TDatabaseContext(), _mode(m), _socket(s), _httpSession(),
      _requestPath(path), _payloads()
{ }


TWebSocketWorker::~TWebSocketWorker()
{
    tSystemDebug("TWebSocketWorker::~TWebSocketWorker");
}


void TWebSocketWorker::setPayload(TWebSocketFrame::OpCode opCode, const QByteArray &payload)
{
    _payloads << qMakePair((int)opCode, payload);
}


void TWebSocketWorker::setPayloads(QList<QPair<int, QByteArray>> payloads)
{
    _payloads = payloads;
}


void TWebSocketWorker::setSession(const TSession &session)
{
    _httpSession = session;
}


void TWebSocketWorker::run()
{
    if (_mode == Receiving) {
        for (QPair<int, QByteArray> &p : _payloads) {
            execute(p.first, p.second);
        }
        _payloads.clear();
    } else {
        execute();
    }
}


void TWebSocketWorker::execute(int opcode, const QByteArray &payload)
{
    bool sendTask = false;
    QString es = TUrlRoute::splitPath(_requestPath).value(0).toLower() + "endpoint";
    TDispatcher<TWebSocketEndpoint> dispatcher(es);
    TWebSocketEndpoint *endpoint = dispatcher.object();

    if (!endpoint) {
        return;
    }

    try {
        tSystemDebug("Found endpoint: %s", qPrintable(es));
        tSystemDebug("TWebSocketWorker opcode: %d", opcode);

        endpoint->sessionStore = _socket->session(); // Sets websocket session
        endpoint->uuid = _socket->socketUuid();
        // Database Transaction
        setTransactionEnabled(endpoint->transactionEnabled());

        switch (_mode) {
        case Opening: {
            bool res = endpoint->onOpen(_httpSession);
            if (res) {
                // For switch response
                endpoint->taskList.prepend(qMakePair((int)TWebSocketEndpoint::OpenSuccess, QVariant()));

                if (endpoint->keepAliveInterval() > 0) {
                    endpoint->startKeepAlive(endpoint->keepAliveInterval());
                }
            } else {
                endpoint->taskList.prepend(qMakePair((int)TWebSocketEndpoint::OpenError, QVariant()));
            }
            break; }

        case Closing:
            if (!_socket->closing.exchange(true)) {
                endpoint->onClose(Tf::GoingAway);
                endpoint->unsubscribeFromAll();
            }
            break;

        case Receiving: {

            switch (opcode) {
            case TWebSocketFrame::TextFrame:
                endpoint->onTextReceived(QString::fromUtf8(payload));
                break;

            case TWebSocketFrame::BinaryFrame:
                endpoint->onBinaryReceived(payload);
                break;

            case TWebSocketFrame::Close: {
                quint16 closeCode = Tf::GoingAway;
                if (payload.length() >= 2) {
                    QDataStream ds(payload);
                    ds.setByteOrder(QDataStream::BigEndian);
                    ds >> closeCode;
                }

                if (!_socket->closing.exchange(true)) {
                    endpoint->onClose(closeCode);
                    endpoint->unsubscribeFromAll();
                }
                endpoint->close(closeCode);  // close response or disconnect
                break; }

            case TWebSocketFrame::Ping:
                endpoint->onPing(payload);
                break;

            case TWebSocketFrame::Pong:
                endpoint->onPong(payload);
                break;

            default:
                tSystemWarn("Invalid opcode: 0x%x  [%s:%d]", (int)opcode, __FILE__, __LINE__);
                break;
            }
            break; }

        default:
            break;
        }

        // Sets session to the websocket
        _socket->setSession(endpoint->session());

        for (auto &p : endpoint->taskList) {
            const QVariant &taskData = p.second;
            tSystemDebug("WebSocket Task: %d", p.first);

            switch (p.first) {
            case TWebSocketEndpoint::OpenSuccess:
                _socket->sendHandshakeResponse();
                break;

            case TWebSocketEndpoint::OpenError:
                _socket->closing = true;
                _socket->closeSent = true;
                _socket->disconnect();
                goto open_error;
                break;

            case TWebSocketEndpoint::SendText:
                _socket->sendText(taskData.toString());
                sendTask = true;
                break;

            case TWebSocketEndpoint::SendBinary:
                _socket->sendBinary(taskData.toByteArray());
                sendTask = true;
                break;

            case TWebSocketEndpoint::SendClose:
                if (_socket->closing.load() && _socket->closeSent.load()) {
                    // close-frame sent and received
                    _socket->disconnect();
                } else {
                    uint closeCode = taskData.toUInt();
                    _socket->sendClose(closeCode);
                    sendTask = true;
                }
                break;

            case TWebSocketEndpoint::SendPing:
                _socket->sendPing(taskData.toByteArray());
                sendTask = true;
                break;

            case TWebSocketEndpoint::SendPong:
                _socket->sendPong(taskData.toByteArray());
                sendTask = true;
                break;

            case TWebSocketEndpoint::SendTextTo: {
                QVariantList lst = taskData.toList();
                TAbstractWebSocket *websocket = TAbstractWebSocket::searchWebSocket(lst[0].toByteArray());
                if (websocket) {
                    websocket->sendText(lst[1].toString());
                }
                break; }

            case TWebSocketEndpoint::SendBinaryTo: {
                QVariantList lst = taskData.toList();
                TAbstractWebSocket *websocket = TAbstractWebSocket::searchWebSocket(lst[0].toByteArray());
                if (websocket) {
                    websocket->sendBinary(lst[1].toByteArray());
                }
                break; }

            case TWebSocketEndpoint::SendCloseTo: {
                QVariantList lst = taskData.toList();
                TAbstractWebSocket *websocket = TAbstractWebSocket::searchWebSocket(lst[0].toByteArray());
                if (websocket) {
                    websocket->sendClose(lst[1].toInt());
                }
                break; }

            case TWebSocketEndpoint::Subscribe: {
                QVariantList lst = taskData.toList();
                TPublisher::instance()->subscribe(lst[0].toString(), lst[1].toBool(), _socket);
                break; }

            case TWebSocketEndpoint::Unsubscribe:
                TPublisher::instance()->unsubscribe(taskData.toString(), _socket);
                break;

            case TWebSocketEndpoint::UnsubscribeFromAll:
                TPublisher::instance()->unsubscribeFromAll(_socket);
                break;

            case TWebSocketEndpoint::PublishText: {
                QVariantList lst = taskData.toList();
                TPublisher::instance()->publish(lst[0].toString(), lst[1].toString(), _socket);
                break; }

            case TWebSocketEndpoint::PublishBinary: {
                QVariantList lst = taskData.toList();
                TPublisher::instance()->publish(lst[0].toString(), lst[1].toByteArray(), _socket);
                break; }

            case TWebSocketEndpoint::StartKeepAlive:
                _socket->startKeepAlive(taskData.toInt());
                break;

            case TWebSocketEndpoint::StopKeepAlive:
                _socket->stopKeepAlive();
                break;

            case TWebSocketEndpoint::HttpSend: {
                QVariantList lst = taskData.toList();
                auto uuid = lst[0].toByteArray();

                switch ( Tf::app()->multiProcessingModule() ) {
                case TWebApplication::Thread: {
                    auto *sock = THttpSocket::searchSocket(uuid);
                    if (sock) {
                        sock->writeRawDataFromWebSocket(lst[1].toByteArray());
                    }
                    break; }

                case TWebApplication::Hybrid: {
#ifdef Q_OS_LINUX
                    auto *sock = TEpollHttpSocket::searchSocket(uuid);
                    if (sock) {
                        sock->sendData(lst[1].toByteArray());
                    }
#else
                    tFatal("Unsupported MPM: hybrid");
#endif
                    break; }

                default:
                    break;
                }
                break; }

            default:
                tSystemError("Invalid logic  [%s:%d]",  __FILE__, __LINE__);
                break;
            }
        }

        if (!sendTask) {
            // Receiving but not sending, so renew keep-alive
            _socket->renewKeepAlive();
        }

    open_error:
        // transaction
        if (Q_UNLIKELY(endpoint->rollbackRequested())) {
            rollbackTransactions();
        } else {
            // Commits a transaction to the database
            commitTransactions();
        }

    } catch (ClientErrorException &e) {
        tWarn("Caught ClientErrorException: status code:%d", e.statusCode());
        tSystemWarn("Caught ClientErrorException: status code:%d", e.statusCode());
    } catch (SqlException &e) {
        tError("Caught SqlException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
        tSystemError("Caught SqlException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
    } catch (KvsException &e) {
        tError("Caught KvsException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
        tSystemError("Caught KvsException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
    } catch (SecurityException &e) {
        tError("Caught SecurityException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
        tSystemError("Caught SecurityException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
    } catch (RuntimeException &e) {
        tError("Caught RuntimeException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
        tSystemError("Caught RuntimeException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
    } catch (StandardException &e) {
        tError("Caught StandardException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
        tSystemError("Caught StandardException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
    } catch (...) {
        tError("Caught Exception");
        tSystemError("Caught Exception");
    }
}
