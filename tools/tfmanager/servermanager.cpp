/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "servermanager.h"
#include "qplatformdefs.h"
#include "systembusdaemon.h"
#include "tfcore.h"
#include <QtCore>
#include <QtNetwork>
#include <TAppSettings>
#include <TApplicationServerBase>
#include <TGlobal>
#include <TLog>
#include <TSystemGlobal>
#include <TWebApplication>

namespace TreeFrog {

#if defined(Q_OS_WIN) && !defined(TF_NO_DEBUG)
constexpr auto TFSERVER_CMD = INSTALL_PATH "/tadpoled";
#else
constexpr auto TFSERVER_CMD = INSTALL_PATH "/tadpole";
#endif

static QMap<QProcess *, int> serversStatus;
static uint startCounter = 0;  // start-counter of treefrog servers


ServerManager::ServerManager(int max, int min, int spare, QObject *parent) :
    QObject(parent), listeningSocket(0), maxServers(max), minServers(min),
    spareServers(spare), managerState(NotRunning)
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


QString ServerManager::tfserverProgramPath()
{
    return QLatin1String(TFSERVER_CMD);
}


bool ServerManager::start(const QHostAddress &address, quint16 port)
{
    if (isRunning())
        return true;

    if (managerState == Stopping) {
        tSystemWarn("Manager stopping  [%s:%d]", __FILE__, __LINE__);
        return false;
    }

#ifdef Q_OS_UNIX
    int sd = TApplicationServerBase::nativeListen(address, port, TApplicationServerBase::NonCloseOnExec);
    if (sd <= 0) {
        tSystemError("Failed to create listening socket");
        std::fprintf(stderr, "Failed to create listening socket\n");
        return false;
    }
    listeningSocket = sd;
#else
    Q_UNUSED(address);
#endif

    managerState = Starting;
    tSystemDebug("TreeFrog application servers starting up.  port:%d", port);
    ajustServers();
    return true;
}


bool ServerManager::start(const QString &fileDomain)
{
    if (isRunning())
        return true;

    if (managerState == Stopping) {
        tSystemWarn("Manager stopping  [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    int sd = TApplicationServerBase::nativeListen(fileDomain, TApplicationServerBase::NonCloseOnExec);
    if (sd <= 0) {
        tSystemError("listening socket create failed  [%s:%d]", __FILE__, __LINE__);
        std::fprintf(stderr, "Failed to create listening socket of UNIX domain\n");
        return false;
    }

    listeningSocket = sd;
    managerState = Starting;
    tSystemDebug("TreeFrog application servers starting up.  Domain file name:%s", qUtf8Printable(fileDomain));
    ajustServers();
    return true;
}


void ServerManager::stop()
{
    if (!isRunning())
        return;

    managerState = Stopping;

    if (listeningSocket > 0) {
        tf_close(listeningSocket);
    }
    listeningSocket = 0;

    if (serverCount() > 0) {
        tSystemInfo("TreeFrog application servers shutting down");

        for (QMapIterator<QProcess *, int> i(serversStatus); i.hasNext();) {
            QProcess *tfserver = i.next().key();
            disconnect(tfserver, SIGNAL(finished(int, QProcess::ExitStatus)), 0, 0);
            disconnect(tfserver, SIGNAL(readyReadStandardError()), 0, 0);

            tfserver->terminate();
        }

        for (QMapIterator<QProcess *, int> i(serversStatus); i.hasNext();) {
            QProcess *tfserver = i.next().key();
            tfserver->waitForFinished(-1);
            delete tfserver;
        }
        serversStatus.clear();
        tSystemInfo("TreeFrog application servers shutdown completed");
    }

    startCounter = 0;
    managerState = NotRunning;
}


bool ServerManager::isRunning() const
{
    return managerState == Running || managerState == Starting;
}


int ServerManager::serverCount() const
{
    return serversStatus.count();
}


void ServerManager::ajustServers()
{
    if (isRunning()) {
        tSystemDebug("serverCount: %d", serverCount());
        if (serverCount() < maxServers && serverCount() < minServers) {
            startServer();
        } else {
            if (managerState != Running) {
                tSystemInfo("TreeFrog application servers started up.");
                managerState = Running;
            }
        }
    }
}


void ServerManager::setupEnvironment(QProcess *process)
{
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
    tSystemDebug("export %s=%s", "LD_LIBRARY_PATH", qUtf8Printable(ldpath));

    QString preload = Tf::appSettings()->value(Tf::LDPreload).toString();
    if (!preload.isEmpty()) {
        env.insert("LD_PRELOAD", preload);
        tSystemDebug("export %s=%s", "LD_PRELOAD", qUtf8Printable(preload));
    }
    process->setProcessEnvironment(env);
#else
    Q_UNUSED(process);
#endif
}


void ServerManager::startServer(int id) const
{
    QStringList args = QCoreApplication::arguments();
    args.removeFirst();

    if (id < 0) {
        id = startCounter;
    }

    TWebApplication::MultiProcessingModule mpm = Tf::app()->multiProcessingModule();
    if (mpm == TWebApplication::Epoll || mpm == TWebApplication::Thread) {
        if (id < maxServers) {
            args.prepend(QString::number(id));
            args.prepend("-i");  // give ID for app server
        }
    }

    if (listeningSocket > 0) {
        args.prepend(QString::number(listeningSocket));
        args.prepend("-s");
    }

    QProcess *tfserver = new QProcess;
    serversStatus.insert(tfserver, id);

    connect(tfserver, SIGNAL(started()), this, SLOT(updateServerStatus()));
    connect(tfserver, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(errorDetect(QProcess::ProcessError)));
    connect(tfserver, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(serverFinish(int, QProcess::ExitStatus)));
    connect(tfserver, SIGNAL(readyReadStandardError()), this, SLOT(readStandardError()));  // For error notification

    // Executes treefrog server
    setupEnvironment(tfserver);
    tfserver->start(tfserverProgramPath(), args, QIODevice::ReadOnly);
    tfserver->closeReadChannel(QProcess::StandardOutput);
    tfserver->closeWriteChannel();
    tSystemDebug("tfserver started");
    ++startCounter;
}


void ServerManager::updateServerStatus()
{
    QProcess *server = dynamic_cast<QProcess *>(sender());
    if (server) {
        ajustServers();
    }
}


void ServerManager::errorDetect(QProcess::ProcessError error)
{
    QProcess *server = dynamic_cast<QProcess *>(sender());
    if (server) {
        tSystemError("tfserver error detected(%d). [%s]", error, qUtf8Printable(tfserverProgramPath()));
        //server->close();  // long blocking..
        server->kill();
    }
}


void ServerManager::serverFinish(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess *server = dynamic_cast<QProcess *>(sender());
    if (server) {
        //server->close();  // long blocking..
        server->deleteLater();
        int id = serversStatus.take(server);

        if (isRunning()) {
            if (exitCode != 127) {  // 127 : for auto reloading
                tSystemError("Detected a server crashed. exitCode:%d  exitStatus:%d", exitCode, (int)exitStatus);
            }
            startServer(id);
        } else {
            tSystemDebug("Detected normal exit of server. exitCode:%d", exitCode);
            if (serversStatus.count() == 0) {
                Tf::app()->exit(-1);
            }
        }
    }
}


void ServerManager::readStandardError() const
{
    QProcess *server = dynamic_cast<QProcess *>(sender());
    if (server) {
        QByteArray buf = server->readAllStandardError();
        tSystemWarn("treefrog stderr: %s", buf.constData());
        std::fprintf(stderr, "treefrog stderr: %s", buf.constData());
    }
}

}  // namespace TreeFrog
