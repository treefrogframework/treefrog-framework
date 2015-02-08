/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "twsactionworker.h"
#include "tsystemglobal.h"


TWsActionWorker::TWsActionWorker(TEpollWebSocket::OpCode opCode, const QByteArray &data, QObject *parent)
    : QThread(parent), opcode(opCode), requestData(data)
{
    tSystemDebug("TWsActionWorker::TWsActionWorker");
}


TWsActionWorker::~TWsActionWorker()
{
    tSystemDebug("TWsActionWorker::~TWsActionWorker");
}


void TWsActionWorker::run()
{
    tSystemWarn("## opcode: %d", opcode);
    if (opcode == TEpollWebSocket::TextFrame) {
        QString str = QString::fromUtf8(requestData);
        tSystemWarn("########### : %s", qPrintable(str));
    } else {
        tSystemWarn("##############");
    }
}
