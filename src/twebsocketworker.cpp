/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TWebApplication>
#include <TDispatcher>
#include <TWebSocketEndpoint>
#include "twebsocketworker.h"
#include "tsystemglobal.h"
#include "turlroute.h"
#include "tabstractwebsocket.h"
#include "tpublisher.h"


TWebSocketWorker::TWebSocketWorker(TWebSocketWorker::RunMode m, TAbstractWebSocket *s, const QByteArray &path, QObject *parent)
    : QThread(parent), TDatabaseContext(), mode_(m), socket_(s), requestPath_(path)
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
    sessionStore_ = session;
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

        // Database Transaction
        setTransactionEnabled(endpoint->transactionEnabled());

        switch (mode_) {
        case Opening:
            endpoint->onOpen(sessionStore_);
            break;

        case Closing:
            endpoint->onClose(Tf::GoingAway);
            endpoint->unsubscribeFromAll();
            goto transaction_cleanup;
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

                endpoint->onClose(closeCode);
                endpoint->close(closeCode);  // close response
                endpoint->unsubscribeFromAll();
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

        for (auto &p : endpoint->taskList) {
            const QVariant &taskData = p.second;

            switch (p.first) {
            case TWebSocketEndpoint::SendText:
                socket_->sendText(taskData.toString());
                sendTask = true;
                break;

            case TWebSocketEndpoint::SendBinary:
                socket_->sendBinary(taskData.toByteArray());
                sendTask = true;
                break;

            case TWebSocketEndpoint::SendClose:
                if (opcode_ == TWebSocketFrame::Close) {
                    socket_->closing = true;
                }

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

            case TWebSocketEndpoint::Subscribe:
                TPublisher::instance()->subscribe(taskData.toString(), socket_);
                break;

            case TWebSocketEndpoint::Unsubscribe:
                TPublisher::instance()->unsubscribe(taskData.toString(), socket_);
                break;

            case TWebSocketEndpoint::UnsubscribeFromAll:
                TPublisher::instance()->unsubscribeFromAll(socket_);
                break;

            case TWebSocketEndpoint::PublishText: {
                QVariantList lst = taskData.toList();
                TPublisher::instance()->publish(lst[0].toString(), lst[1].toString());
                sendTask = true;
                break; }

            case TWebSocketEndpoint::PublishBinary: {
                QVariantList lst = taskData.toList();
                TPublisher::instance()->publish(lst[0].toString(), lst[1].toByteArray());
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

    transaction_cleanup:
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
