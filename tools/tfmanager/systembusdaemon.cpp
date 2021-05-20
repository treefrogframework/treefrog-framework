/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "systembusdaemon.h"
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <TWebApplication>
#include <tsystembus.h>

static SystemBusDaemon *systemBusDaemon = nullptr;
static int maxServers = 0;


#ifdef Q_OS_UNIX
static QString unixDomainServerDir()
{
    return QDir::cleanPath(QDir::tempPath()) + "/";
}
#endif

SystemBusDaemon::SystemBusDaemon() :
    QObject(), localServer(new QLocalServer), socketSet()
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
    QFile file(unixDomainServerDir() + TSystemBus::connectionName());
    if (file.exists()) {
        file.remove();
        tSystemWarn("File removed for UNIX domain socket : %s", qUtf8Printable(file.fileName()));
    }
#endif

    bool ret = localServer->listen(TSystemBus::connectionName());
    if (ret) {
        tSystemDebug("system bus open : %s", qUtf8Printable(localServer->fullServerName()));
    } else {
        tSystemError("system bus open error  [%s:%d]", __FILE__, __LINE__);
    }
    return ret;
}


void SystemBusDaemon::close()
{
    QObject::disconnect(localServer, nullptr, nullptr, nullptr);
    localServer->close();

    for (auto *socket : socketSet) {
        disconnect(socket, nullptr, nullptr, nullptr);
        socket->abort();
        delete socket;
    }

    tSystemDebug("close system bus daemon : %s", qUtf8Printable(localServer->fullServerName()));
}


void SystemBusDaemon::acceptConnection()
{
    QLocalSocket *socket;
    while ((socket = localServer->nextPendingConnection())) {
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

    QLocalSocket *socket = dynamic_cast<QLocalSocket *>(sender());
    if (!socket) {
        return;
    }

    QByteArray buf = socket->readAll();
    tSystemDebug("SystemBusDaemon::read len : %lld", buf.size());
    if (maxServers <= 1) {
        // do nothing
        return;
    }

    quint8 opcode;
    quint32 length;

    for (;;) {
        QDataStream ds(buf);
        ds.setByteOrder(QDataStream::BigEndian);
        ds >> opcode >> length;

        if ((uint)buf.length() < length + HEADER_LEN) {
            if (!socket->waitForReadyRead(100)) {
                tSystemError("Manager frame too short  [%s:%d]", __FILE__, __LINE__);
                break;
            }
            buf += socket->readAll();
            continue;
        }

        length += HEADER_LEN;

        // Writes to other tfservers
        for (auto *tfserver : socketSet) {
            if (tfserver != socket) {
                uint wrotelen = 0;
                for (;;) {
                    int len = tfserver->write(buf.data() + wrotelen, length - wrotelen);
                    if (len <= 0) {
                        tSystemError("PIPE write error  len:%d [%s:%d]", len, __FILE__, __LINE__);
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
        if (buf.isEmpty()) {
            break;
        }
    }
}


void SystemBusDaemon::handleDisconnect()
{
    QLocalSocket *socket = dynamic_cast<QLocalSocket *>(sender());
    if (socket) {
        socketSet.remove(socket);
        disconnect(socket, nullptr, nullptr, nullptr);
        socket->deleteLater();
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
    QFile file(unixDomainServerDir() + TSystemBus::connectionName(pid));
    if (file.exists()) {
        file.remove();
        tSystemWarn("File removed for UNIX domain socket : %s", qUtf8Printable(file.fileName()));
    }
#else
    Q_UNUSED(pid);
#endif
}
