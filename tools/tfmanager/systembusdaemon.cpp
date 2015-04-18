/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QLocalServer>
#include <QLocalSocket>
#include <QFile>
#include <TWebApplication>
#include <tsystembus.h>
#include "systembusdaemon.h"

static SystemBusDaemon *systemBusDaemon = nullptr;
static int maxServers = 0;


SystemBusDaemon::SystemBusDaemon()
    : QObject(), localServer(new QLocalServer), socketSet()
{
    connect(localServer, SIGNAL(newConnection()), this, SLOT(acceptConnection()));
}


SystemBusDaemon::~SystemBusDaemon()
{
    delete localServer;
}


bool SystemBusDaemon::open()
{
#ifdef Q_OS_UNIX
    // UNIX
    QFile file(QLatin1String("/tmp/") + TSystemBus::connectionName());
    if (file.exists()) {
        file.remove();
        tSystemWarn("File removed for UNIX domain socket : %s", qPrintable(file.fileName()));
    }
#endif
    return localServer->listen(TSystemBus::connectionName());
}


void SystemBusDaemon::close()
{
    tSystemDebug("close system bus daemon : %s", qPrintable(localServer->fullServerName()));
    localServer->close();
}


void SystemBusDaemon::acceptConnection()
{
    QLocalSocket *socket;
    while ( (socket = localServer->nextPendingConnection()) ) {
        tSystemDebug("acceptConnection");
        connect(socket, SIGNAL(readyRead()), this, SLOT(readSocket()));
        connect(socket, SIGNAL(disconnected()), this, SLOT(handleDisconnect()));
        socketSet.insert(socket);
    }
}


void SystemBusDaemon::readSocket()
{
    const int HEADER_LEN = 5;
    tSystemDebug("SystemBusDaemon::readSocket");

    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());
    if (!socket) {
        return;
    }

    QByteArray buf;
    for (;;) {
        buf += socket->readAll();
        if (maxServers <= 1) {
            // do nothing
            break;
        }

        QDataStream ds(buf);
        ds.setByteOrder(QDataStream::BigEndian);

        quint8 opcode;
        int  length;
        ds >> opcode >> length;

        if (buf.length() < HEADER_LEN || buf.length() < length + HEADER_LEN) {
            if (!socket->waitForReadyRead(1000)) {
                tSystemError("Manager frame too short  [%s:%d]", __FILE__, __LINE__);
                break;
            }
            continue;
        }

        length += HEADER_LEN;

        // Writes to other tfservers
        for (auto *tfserver : socketSet) {
            if (tfserver != socket) {
                int wrotelen = 0;
                for (;;) {
                    int len = tfserver->write(buf.data() + wrotelen, length - wrotelen);
                    if (len <= 0) {
                        tSystemError("PIPE write error  len:%d [%s:%d]", len,  __FILE__, __LINE__);
                        break;
                    }

                    wrotelen += len;
                    if (wrotelen == length) {
                        break;
                    }

                    if (!tfserver->waitForBytesWritten(1000)) {
                        tSystemError("PIPE wait error  [%s:%d]", __FILE__, __LINE__);
                        break;
                    }
                }
            }
        }

        buf.remove(0, length);
        if (buf.isEmpty())
            break;

        if (!socket->waitForReadyRead(1000)) {
            tSystemError("Invalid frame  [%s:%d]", __FILE__, __LINE__);
            break;
        }
    }
}

/* Data format
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +---------------+-----------------------------------------------+
 |   opcode (8)  |           Payload length (32)                 |
 |---------------+-----------------------------------------------+
 |               |           Payload data  ...                   |
*/


void SystemBusDaemon::handleDisconnect()
{
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());
    if (socket) {
        socketSet.remove(socket);
        tSystemDebug("disconnected local socket : %p", socket);
    }
}


SystemBusDaemon *SystemBusDaemon::instance()
{
    return systemBusDaemon;
}


void SystemBusDaemon::instantiate()
{
    if (!systemBusDaemon) {
        systemBusDaemon = new SystemBusDaemon;
        systemBusDaemon->open();

        maxServers = Tf::app()->maxNumberOfAppServers();
    }
}


void SystemBusDaemon::releaseResource(qint64 pid)
{
#ifdef Q_OS_UNIX
    // UNIX
    QFile file(QLatin1String("/tmp/") + TSystemBus::connectionName(pid));
    if (file.exists()) {
        file.remove();
        tSystemWarn("File removed for UNIX domain socket : %s", qPrintable(file.fileName()));
    }
#else
    Q_UNUSED(pid);
#endif
}
