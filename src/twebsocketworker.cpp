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
    requestData_ = payload;
}


void TWebSocketWorker::setSession(const TSession &session)
{
    sessionStore_ = session;
}


void TWebSocketWorker::run()
{
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

        case Sending:
            break;

        case Closing:
            endpoint->onClose(Tf::GoingAway);
            goto transaction_cleanup;
            break;

        case Receiving: {

            switch (opcode_) {
            case TWebSocketFrame::TextFrame:
                endpoint->onTextReceived(QString::fromUtf8(requestData_));
                break;

            case TWebSocketFrame::BinaryFrame:
                endpoint->onBinaryReceived(requestData_);
                break;

            case TWebSocketFrame::Close: {
                quint16 closeCode = Tf::GoingAway;
                if (requestData_.length() >= 2) {
                    QDataStream ds(&requestData_, QIODevice::ReadOnly);
                    ds.setByteOrder(QDataStream::BigEndian);
                    ds >> closeCode;
                }

                endpoint->onClose(closeCode);
                endpoint->close(closeCode);  // close response
                break; }

            case TWebSocketFrame::Ping:
                endpoint->onPing();
                endpoint->sendPong();
                break;

            case TWebSocketFrame::Pong:
                endpoint->onPong();
                break;

            default:
                tSystemWarn("Invalid opcode: 0x%x  [%s:%d]", (int)opcode_, __FILE__, __LINE__);
                break;
            }
            break; }

        default:
            break;
        }

        // Sends payload
        for (const QVariant &var : endpoint->payloadList) {
            switch (var.type()) {
            case QVariant::String:
                socket_->sendText(var.toString());
                break;

            case QVariant::ByteArray:
                socket_->sendBinary(var.toByteArray());
                break;

            case QVariant::UInt:
                if (opcode_ == TWebSocketFrame::Close) {
                    socket_->closing = true;
                }

                if (socket_->closing && socket_->closeSent) {
                    // close-frame sent and received
                    socket_->disconnect();
                } else {
                    uint closeCode = var.toUInt();
                    socket_->sendClose(closeCode);
                }
                break;

            case QVariant::Int: {

                int rescode = var.toInt();
                switch (rescode) {
                case TWebSocketFrame::Ping:
                    socket_->sendPing();
                    break;

                case TWebSocketFrame::Pong:
                    socket_->sendPong();
                    break;

                default:
                    tSystemError("Invalid logic  [%s:%d]",  __FILE__, __LINE__);
                    break;
                }

                break; }

            default:
                tSystemError("Invalid logic  [%s:%d]",  __FILE__, __LINE__);
                break;
            }
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
