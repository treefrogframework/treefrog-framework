/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TWebApplication>
#include <TDispatcher>
#include <TWebSocketEndpoint>
#include <THttpRequestHeader>
#include "twebsocketworker.h"
#include "tsystemglobal.h"
#include "turlroute.h"
#include "tabstractwebsocket.h"
#include "tpublisher.h"


TWebSocketWorker::TWebSocketWorker(TWebSocketWorker::RunMode m, TAbstractWebSocket *s, const QByteArray &path, QObject *parent)
    : QThread(parent), TDatabaseContext(), mode_(m), socket_(s), requestPath_(path),
      opcode_((TWebSocketFrame::OpCode)0), payload_()
{ }


TWebSocketWorker::~TWebSocketWorker()
{
    tSystemDebug("TWebSocketWorker::~TWebSocketWorker");
}


void TWebSocketWorker::setPayload(TWebSocketFrame::OpCode opCode, const QByteArray &payload)
{
    opcode_ = opCode;
    payload_ = payload;
}


void TWebSocketWorker::setSession(const TSession &session)
{
    httpSession_ = session;
}


void TWebSocketWorker::run()
{
    bool sendTask = false;
    QString es = TUrlRoute::splitPath(requestPath_).value(0).toLower() + "endpoint";
    TDispatcher<TWebSocketEndpoint> dispatcher(es);
    TWebSocketEndpoint *endpoint = dispatcher.object();

    if (!endpoint) {
        return;
    }

    try {
        tSystemDebug("Found endpoint: %s", qPrintable(es));
        tSystemDebug("TWebSocketWorker opcode: %d", opcode_);

        endpoint->sessionStore = socket_->session(); // Sets websocket session
        endpoint->uuid = socket_->socketUuid();
        // Database Transaction
        setTransactionEnabled(endpoint->transactionEnabled());

        switch (mode_) {
        case Opening: {
            bool res = endpoint->onOpen(httpSession_);
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
            if (!socket_->closing.exchange(true)) {
                endpoint->onClose(Tf::GoingAway);
                endpoint->unsubscribeFromAll();
            }
            break;

        case Receiving: {

            switch (opcode_) {
            case TWebSocketFrame::TextFrame:
                endpoint->onTextReceived(QString::fromUtf8(payload_));
                break;

            case TWebSocketFrame::BinaryFrame:
                endpoint->onBinaryReceived(payload_);
                break;

            case TWebSocketFrame::Close: {
                quint16 closeCode = Tf::GoingAway;
                if (payload_.length() >= 2) {
                    QDataStream ds(&payload_, QIODevice::ReadOnly);
                    ds.setByteOrder(QDataStream::BigEndian);
                    ds >> closeCode;
                }

                if (!socket_->closing.exchange(true)) {
                    endpoint->onClose(closeCode);
                    endpoint->unsubscribeFromAll();
                }
                endpoint->close(closeCode);  // close response or disconnect
                break; }

            case TWebSocketFrame::Ping:
                endpoint->onPing(payload_);
                break;

            case TWebSocketFrame::Pong:
                endpoint->onPong(payload_);
                break;

            default:
                tSystemWarn("Invalid opcode: 0x%x  [%s:%d]", (int)opcode_, __FILE__, __LINE__);
                break;
            }
            break; }

        default:
            break;
        }

        // Sets session to the websocket
        socket_->setSession(endpoint->session());

        for (auto &p : endpoint->taskList) {
            const QVariant &taskData = p.second;

            switch (p.first) {
            case TWebSocketEndpoint::OpenSuccess:
                socket_->sendHandshakeResponse();
                break;

            case TWebSocketEndpoint::OpenError:
                socket_->closing = true;
                socket_->closeSent = true;
                socket_->disconnect();
                goto open_error;
                break;

            case TWebSocketEndpoint::SendText:
                socket_->sendText(taskData.toString());
                sendTask = true;
                break;

            case TWebSocketEndpoint::SendBinary:
                socket_->sendBinary(taskData.toByteArray());
                sendTask = true;
                break;

            case TWebSocketEndpoint::SendClose:
                if (socket_->closing.load() && socket_->closeSent.load()) {
                    // close-frame sent and received
                    socket_->disconnect();
                } else {
                    uint closeCode = taskData.toUInt();
                    socket_->sendClose(closeCode);
                    sendTask = true;
                }
                break;

            case TWebSocketEndpoint::SendPing:
                socket_->sendPing(taskData.toByteArray());
                sendTask = true;
                break;

            case TWebSocketEndpoint::SendPong:
                socket_->sendPong(taskData.toByteArray());
                sendTask = true;
                break;

            case TWebSocketEndpoint::Subscribe: {
                QVariantList lst = taskData.toList();
                TPublisher::instance()->subscribe(lst[0].toString(), lst[1].toBool(), socket_);
                break; }

            case TWebSocketEndpoint::Unsubscribe:
                TPublisher::instance()->unsubscribe(taskData.toString(), socket_);
                break;

            case TWebSocketEndpoint::UnsubscribeFromAll:
                TPublisher::instance()->unsubscribeFromAll(socket_);
                break;

            case TWebSocketEndpoint::PublishText: {
                QVariantList lst = taskData.toList();
                TPublisher::instance()->publish(lst[0].toString(), lst[1].toString(), socket_);
                sendTask = true;
                break; }

            case TWebSocketEndpoint::PublishBinary: {
                QVariantList lst = taskData.toList();
                TPublisher::instance()->publish(lst[0].toString(), lst[1].toByteArray(), socket_);
                sendTask = true;
                break; }

            case TWebSocketEndpoint::StartKeepAlive:
                socket_->startKeepAlive(taskData.toInt());
                break;

            case TWebSocketEndpoint::StopKeepAlive:
                socket_->stopKeepAlive();
                break;

            default:
                tSystemError("Invalid logic  [%s:%d]",  __FILE__, __LINE__);
                break;
            }
        }

        if (!sendTask) {
            // Renew keep-alive
            socket_->renewKeepAlive();
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
