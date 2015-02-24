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


TWsActionWorker::TWsActionWorker(const QByteArray &socket, const QByteArray &path, TEpollWebSocket::OpCode opCode, const QByteArray &data, QObject *parent)
    : QThread(parent), socketUuid(socket), requestPath(path), opcode(opCode), requestData(data)
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
        tSystemWarn("TWsActionWorker opcode: %d", opcode);

        switch (opcode) {
        case TEpollWebSocket::TextFrame:
            wscontroller->onTextReceived(QString::fromUtf8(requestData));
            break;

        case TEpollWebSocket::BinaryFrame:
            wscontroller->onBinaryReceived(requestData);
            break;

        case TEpollWebSocket::Close:
            wscontroller->onClose();
            TEpoll::instance()->setDisconnect(socketUuid);
            break;

        case TEpollWebSocket::Ping:
            wscontroller->onPing();
            break;

        case TEpollWebSocket::Pong:
            wscontroller->onPong();
            break;

        default:
            tWarn("Invalid opcode: 0x%x  [%s:%d]", (int)opcode, __FILE__, __LINE__);
            break;
        }
    }
}
