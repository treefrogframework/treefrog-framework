/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TWebApplication>
#include <TDispatcher>
#include <TWebSocketController>
#include "twsactionworker.h"
#include "tepoll.h"
#include "tsystemglobal.h"
#include "turlroute.h"


TWsActionWorker::TWsActionWorker(const QByteArray &socket, const TSession &session, QObject *parent)
    : QThread(parent), socketUuid(socket), sessionStore(session), requestPath(),
      opcode(TWebSocketFrame::Continuation), requestData()
{
    tSystemDebug("TWsActionWorker::TWsActionWorker");
}


TWsActionWorker::TWsActionWorker(const QByteArray &socket, const QByteArray &path, TWebSocketFrame::OpCode opCode, const QByteArray &data, QObject *parent)
    : QThread(parent), socketUuid(socket), sessionStore(), requestPath(path), opcode(opCode), requestData(data)
{
    tSystemDebug("TWsActionWorker::TWsActionWorker");
}


TWsActionWorker::~TWsActionWorker()
{
    tSystemDebug("TWsActionWorker::~TWsActionWorker");
}


void TWsActionWorker::run()
{
    QString controller = TUrlRoute::splitPath(requestPath).value(0).toLower() + "controller";
    TDispatcher<TWebSocketController> ctlrDispatcher(controller);
    TWebSocketController *wscontroller = ctlrDispatcher.object();

    if (wscontroller) {
        tSystemDebug("Found WsController: %s", qPrintable(controller));
        tSystemDebug("TWsActionWorker opcode: %d", opcode);

        switch (opcode) {
        case TWebSocketFrame::Continuation:
            if (sessionStore.id().isEmpty()) {
                wscontroller->onOpen(sessionStore);
            } else {
                tError("Invalid logic  [%s:%d]",  __FILE__, __LINE__);
            }
            break;

        case TWebSocketFrame::TextFrame:
            wscontroller->onTextReceived(QString::fromUtf8(requestData));
            break;

        case TWebSocketFrame::BinaryFrame:
            wscontroller->onBinaryReceived(requestData);
            break;

        case TWebSocketFrame::Close:
            wscontroller->onClose();
            wscontroller->closeWebSocket();
            break;

        case TWebSocketFrame::Ping:
            wscontroller->onPing();
            wscontroller->sendPong();
            break;

        case TWebSocketFrame::Pong:
            wscontroller->onPong();
            break;

        default:
            tWarn("Invalid opcode: 0x%x  [%s:%d]", (int)opcode, __FILE__, __LINE__);
            break;
        }

        // Sends payload
        for (QListIterator<QVariant> it(wscontroller->payloadList); it.hasNext(); ) {
            const QVariant &var = it.next();
            switch (var.type()) {
            case QVariant::String:
                TEpollWebSocket::sendText(socketUuid, var.toString());
                break;

            case QVariant::ByteArray:
                TEpollWebSocket::sendBinary(socketUuid, var.toByteArray());
                break;

            case QVariant::Int: {

                int opcode = var.toInt();
                switch (opcode) {
                case TWebSocketFrame::Close:
                    TEpollWebSocket::disconnect(socketUuid);
                    break;

                case TWebSocketFrame::Ping:
                    TEpollWebSocket::sendPing(socketUuid);
                    break;

                case TWebSocketFrame::Pong:
                    TEpollWebSocket::sendPong(socketUuid);
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
    }
}
