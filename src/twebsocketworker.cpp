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


TWebSocketWorker::TWebSocketWorker(TAbstractWebSocket *s, const QByteArray &path, const TSession &session, QObject *parent)
    : QThread(parent), TDatabaseContext(), socket(s), sessionStore(session),
      requestPath(path), opcode(TWebSocketFrame::Continuation), requestData()
{
    tSystemDebug("TWebSocketWorker::TWebSocketWorker");
}


TWebSocketWorker::TWebSocketWorker(TAbstractWebSocket *s, const QByteArray &path, TWebSocketFrame::OpCode opCode, const QByteArray &data, QObject *parent)
    : QThread(parent), TDatabaseContext(), socket(s), sessionStore(), requestPath(path),
      opcode(opCode), requestData(data)
{
    tSystemDebug("TWebSocketWorker::TWebSocketWorker");
}

TWebSocketWorker::~TWebSocketWorker()
{
    tSystemDebug("TWebSocketWorker::~TWebSocketWorker");
}


void TWebSocketWorker::run()
{
    QString es = TUrlRoute::splitPath(requestPath).value(0).toLower() + "endpoint";
    TDispatcher<TWebSocketEndpoint> dispatcher(es);
    TWebSocketEndpoint *endpoint = dispatcher.object();
    try {
        if (endpoint) {
            tSystemDebug("Found endpoint: %s", qPrintable(es));
            tSystemDebug("TWebSocketWorker opcode: %d", opcode);

            // Database Transaction
            setTransactionEnabled(endpoint->transactionEnabled());

            switch (opcode) {
            case TWebSocketFrame::Continuation:   // means session opening
                if (sessionStore.id().isEmpty()) {
                    endpoint->onOpen(sessionStore);
                } else {
                    tError("Invalid logic  [%s:%d]",  __FILE__, __LINE__);
                }
                break;

            case TWebSocketFrame::TextFrame:
                endpoint->onTextReceived(QString::fromUtf8(requestData));
                break;

            case TWebSocketFrame::BinaryFrame:
                endpoint->onBinaryReceived(requestData);
                break;

            case TWebSocketFrame::Close:
                endpoint->onClose();
                endpoint->closeWebSocket();
                break;

            case TWebSocketFrame::Ping:
                endpoint->onPing();
                endpoint->sendPong();
                break;

            case TWebSocketFrame::Pong:
                endpoint->onPong();
                break;

            default:
                tWarn("Invalid opcode: 0x%x  [%s:%d]", (int)opcode, __FILE__, __LINE__);
                break;
            }

            // Sends payload
            for (const QVariant &var : endpoint->payloadList) {
                switch (var.type()) {
                case QVariant::String:
                    socket->sendText(var.toString());
                    break;

                case QVariant::ByteArray:
                    socket->sendBinary(var.toByteArray());
                    break;

                case QVariant::Int: {

                    int opcode = var.toInt();
                    switch (opcode) {
                    case TWebSocketFrame::Close:
                        socket->disconnect();
                        break;

                    case TWebSocketFrame::Ping:
                        socket->sendPing();
                        break;

                    case TWebSocketFrame::Pong:
                        socket->sendPong();
                        break;

                    default:
                        tError("Invalid logic  [%s:%d]",  __FILE__, __LINE__);
                        break;
                    }

                    break; }

                default:
                    tError("Invalid logic  [%s:%d]",  __FILE__, __LINE__);
                    break;
                }
            }

            // transaction
            if (Q_UNLIKELY(endpoint->rollbackRequested())) {
                rollbackTransactions();
            } else {
                // Commits a transaction to the database
                commitTransactions();
            }
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
