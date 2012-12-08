/* Copyright (c) 2010, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include "logserver.h"


LogServer::LogServer(const QString &webAppName, const QString &logFilePath, QObject *parent)
    : QObject(parent), server(new QLocalServer(this)), webApplicationName(webAppName), logFile(logFilePath)
      
{
    connect(server, SIGNAL(newConnection()), this, SLOT(accept()));
}


bool LogServer::start()
{
    if (logFile.fileName().isEmpty() || webApplicationName.isEmpty()) {
        return false;
    }

    if (!logFile.isOpen()) {
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            qCritical("LogServer: File open failed");
            return false;
        }
    }

    if (!server->isListening()) {
        QLocalServer::removeServer(serverName());
        if (!server->listen(serverName())) {
            qCritical("LogServer: Listen failed");
            return false;
        }
    }
    return true;
}


void LogServer::stop()
{
    server->close();
    logFile.close();
}


QString LogServer::serverName() const
{
    return QLatin1String(".tflogsvr_") + webApplicationName;
}


void LogServer::writeLog()
{
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());
    if (socket) {
        QByteArray log = socket->readAll();

        if (logFile.isOpen()) {
            int len = 0;
            len = logFile.write(log);
            logFile.flush();
            Q_ASSERT(len == log.length());
        }
    }
}


void LogServer::accept() const
{
    while (server->hasPendingConnections()) {
        QLocalSocket *socket = server->nextPendingConnection();
        connect(socket, SIGNAL(disconnected()), this, SLOT(cleanup()));
        connect(socket, SIGNAL(readyRead()), this, SLOT(writeLog()));
        qDebug("LogServer: accept");
    }
}


void LogServer::cleanup() const
{
    qDebug("LogServer: disconnected");
    sender()->deleteLater();
}
