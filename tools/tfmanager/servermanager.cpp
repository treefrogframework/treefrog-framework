/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QtCore>
#include <QtNetwork>
#include <TGlobal>
#include <TWebApplication>
#include <TSystemGlobal>
#include <TApplicationServerBase>
#include <TLog>
#include "qplatformdefs.h"
#include "servermanager.h"
#include "tfcore.h"

namespace TreeFrog {

#if defined(Q_OS_WIN) && !defined(TF_NO_DEBUG)
#  define TFSERVER_CMD  INSTALL_PATH "/tadpoled"
#else
#  define TFSERVER_CMD  INSTALL_PATH "/tadpole"
#endif

static QMap<QProcess *, int> serversStatus;
static uint startCounter = 0;  // start-counter of treefrog servers


ServerManager::ServerManager(int max, int min, int spare, QObject *parent)
    : QObject(parent), listeningSocket(0), maxServers(max), minServers(min),
      spareServers(spare), running(false), pipeReading(false)
{
    spareServers = qMax(spareServers, 0);
    minServers = qMax(minServers, 1);
    maxServers = qMax(maxServers, minServers);

    TApplicationServerBase::nativeSocketInit();
}


ServerManager::~ServerManager()
{
    stop();

    TApplicationServerBase::nativeSocketCleanup();
}


bool ServerManager::start(const QHostAddress &address, quint16 port)
{
    if (isRunning())
        return true;

#ifdef Q_OS_UNIX
    int sd = TApplicationServerBase::nativeListen(address, port, TApplicationServerBase::NonCloseOnExec);
    if (sd <= 0) {
        tSystemError("Failed to create listening socket");
        fprintf(stderr, "Failed to create listening socket\n");
        return false;
    }
    listeningSocket = sd;
#else
    Q_UNUSED(address);
#endif

    running = true;
    ajustServers();
    tSystemInfo("TreeFrog application servers start up.  port:%d", port);
    return true;
}


bool ServerManager::start(const QString &fileDomain)
{
    if (isRunning())
        return true;

    int sd = TApplicationServerBase::nativeListen(fileDomain, TApplicationServerBase::NonCloseOnExec);
    if (sd <= 0) {
        tSystemError("listening socket create failed  [%s:%d]", __FILE__, __LINE__);
        fprintf(stderr, "Failed to create listening socket of UNIX domain\n");
        return false;
    }

    listeningSocket = sd;
    running = true;
    ajustServers();
    tSystemInfo("TreeFrog application servers start up.  Domain file name:%s", qPrintable(fileDomain));
    return true;
}


void ServerManager::stop()
{
    if (!isRunning())
        return;

    running = false;

    if (listeningSocket > 0) {
        tf_close(listeningSocket);
    }
    listeningSocket = 0;

    if (serverCount() > 0) {
        tSystemInfo("TreeFrog application servers shutting down");

        for (QMapIterator<QProcess *, int> i(serversStatus); i.hasNext(); ) {
            QProcess *tfserver = i.next().key();
            disconnect(tfserver, SIGNAL(finished(int, QProcess::ExitStatus)), 0, 0);
            disconnect(tfserver, SIGNAL(readyReadStandardError()), 0, 0);

            tfserver->terminate();
        }

        for (QMapIterator<QProcess *, int> i(serversStatus); i.hasNext(); ) {
            QProcess *tfserver = i.next().key();
            tfserver->waitForFinished(-1);
            delete tfserver;
        }
        serversStatus.clear();
        tSystemInfo("TreeFrog application servers shutdown completed");
    }

    startCounter = 0;
}


bool ServerManager::isRunning() const
{
    return running;
}


int ServerManager::serverCount() const
{
    return serversStatus.count();
}


int ServerManager::spareServerCount() const
{
    int count = 0;
    for (QMapIterator<QProcess *, int> i(serversStatus); i.hasNext(); ) {
        int state = i.next().value();
        if (state == Listening) {
            ++count;
        }
    }
    return count;
}


void ServerManager::ajustServers() const
{
    if (isRunning()) {
        tSystemDebug("serverCount: %d  spare: %d", serverCount(), spareServerCount());
        if (serverCount() < maxServers && (serverCount() < minServers || spareServerCount() < spareServers)) {
            startServer();
        }
    }
}


void ServerManager::startServer() const
{
    QStringList args = QCoreApplication::arguments();
    args.removeFirst();

    TWebApplication::MultiProcessingModule mpm = Tf::app()->multiProcessingModule();
    if (mpm == TWebApplication::Hybrid || mpm == TWebApplication::Thread) {
        if (startCounter < (uint)maxServers) {
            args.prepend(QString::number(startCounter));
            args.prepend("-i");  // give ID for app server
        }
    }

    if (listeningSocket > 0) {
        args.prepend(QString::number(listeningSocket));
        args.prepend("-s");
    }

    QProcess *tfserver = new QProcess;
    serversStatus.insert(tfserver, NotRunning);

    connect(tfserver, SIGNAL(started()), this, SLOT(updateServerStatus()));
    connect(tfserver, SIGNAL(error(QProcess::ProcessError)), this, SLOT(errorDetect(QProcess::ProcessError)));
    connect(tfserver, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(serverFinish(int, QProcess::ExitStatus)));
    connect(tfserver, SIGNAL(readyReadStandardOutput()), this, SLOT(readStandardOutput()));  // IPC between tfservers
    connect(tfserver, SIGNAL(readyReadStandardError()), this, SLOT(readStandardError()));    // For error notification

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    // Sets LD_LIBRARY_PATH environment variable
    QString ldpath = ".";  // means the lib dir
    QString sysldpath = QProcess::systemEnvironment().filter("LD_LIBRARY_PATH=", Qt::CaseSensitive).value(0).mid(16);
    if (!sysldpath.isEmpty()) {
        ldpath += ":";
        ldpath += sysldpath;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LD_LIBRARY_PATH", ldpath);
    tfserver->setProcessEnvironment(env);
    tSystemDebug("export %s=%s", "LD_LIBRARY_PATH", qPrintable(ldpath));
#endif

    // Executes treefrog server
    tfserver->start(TFSERVER_CMD, args, QIODevice::ReadWrite);
    //tfserver->closeWriteChannel();
    tSystemDebug("tfserver started");
    ++startCounter;
}


void ServerManager::updateServerStatus()
{
    QProcess *server = qobject_cast<QProcess *>(sender());
    if (server) {
        if (serversStatus.contains(server)) {
            serversStatus.insert(server, Listening);
        }

        ajustServers();
    }
}


void ServerManager::errorDetect(QProcess::ProcessError error)
{
    QProcess *server = qobject_cast<QProcess *>(sender());
    if (server) {
        tSystemError("tfserver error detected(%d). [%s]", error, TFSERVER_CMD);
        //server->close();  // long blocking..
        server->deleteLater();
        serversStatus.remove(server);

        ajustServers();
    }
}


void ServerManager::serverFinish(int exitCode, QProcess::ExitStatus exitStatus) const
{
    QProcess *server = qobject_cast<QProcess *>(sender());
    if (server) {
        //server->close();  // long blocking..
        server->deleteLater();
        serversStatus.remove(server);

        if (exitStatus == QProcess::CrashExit) {
            ajustServers();
        } else {
            tSystemDebug("Detected normal exit of server. exitCode:%d", exitCode);
            if (serversStatus.count() == 0) {
                Tf::app()->exit(-1);
            }
        }
    }
}


void ServerManager::readProcess(QProcess *server)
{
    const int HEADER_LEN = 5;
    QByteArray buf;

    for (;;) {
        buf += server->readAllStandardOutput();
        if (maxServers <= 1) {
            break;
        }

        QDataStream ds(buf);
        ds.setByteOrder(QDataStream::BigEndian);

        quint8 opcode;
        int  length;
        ds >> opcode >> length;

        if (buf.length() < HEADER_LEN || buf.length() < length + HEADER_LEN) {
            if (!server->waitForReadyRead(1000)) {
                tSystemError("Manager frame too short  [%s:%d]", __FILE__, __LINE__);
                break;
            }
            continue;
        }

        length += HEADER_LEN;

        // Writes to other tfservers
        for (QMapIterator<QProcess *, int> i(serversStatus); i.hasNext(); ) {
            QProcess *tfserver = i.next().key();
            if (tfserver != server) {
                int wrotelen = 0;
                for (;;) {
                    int len = tfserver->write(buf.data() + wrotelen, length - wrotelen);
                    if (len <= 0) {
                        tSystemError("PIPE write error  len:%d [%s:%d]", len,  __FILE__, __LINE__);
                        break;
                    }

                    wrotelen += len;
                    if (wrotelen >= length) {
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

        if (!server->waitForReadyRead(1000)) {
            tSystemError("Invalid frame  [%s:%d]", __FILE__, __LINE__);
            break;
        }
    }
}


void ServerManager::readStandardOutput()
{
    if (pipeReading) {
        QTimer::singleShot(1, this, SLOT(readStandardOutputOfAll()));
        return;
    }

    QProcess *server = qobject_cast<QProcess *>(sender());
    QByteArray buf;
    if (server) {
        pipeReading = true;
        readProcess(server);
        pipeReading = false;
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


void ServerManager::readStandardOutputOfAll()
{
    for (QMapIterator<QProcess *, int> i(serversStatus); i.hasNext(); ) {
        QProcess *tfserver = i.next().key();
        if (tfserver->waitForReadyRead(0)) {
            readProcess(tfserver);
        }
    }
}


void ServerManager::readStandardError() const
{
    QProcess *server = qobject_cast<QProcess *>(sender());
    if (server) {
        QByteArray buf = server->readAllStandardError();
        if (buf == "_accepted") {       // obsoleted here
            if (serversStatus.contains(server)) {
                serversStatus.insert(server, Running);
                ajustServers();
            }
        } else {
            tSystemWarn("treefrog stderr: %s", buf.constData());
            fprintf(stderr, "treefrog stderr: %s", buf.constData());
        }
    }
}

} // namespace TreeFrog
