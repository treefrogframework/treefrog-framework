/* Copyright (c) 2010-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TWebApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <windows.h>
#include <winuser.h>

const QString LOCAL_SERVER_PREFIX = "treefrog_control_";
static volatile int ctrlSignal = -1;


static BOOL WINAPI signalHandler(DWORD ctrlType)
{
    switch (ctrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        ctrlSignal = ctrlType;
        break;
    default:
        return FALSE;
    }

    while (true)
        Sleep(1);

    return TRUE;
}



int TWebApplication::signalNumber()
{
    return ctrlSignal;
}


void TWebApplication::resetSignalNumber()
{
    ctrlSignal = -1;
}


void TWebApplication::watchConsoleSignal()
{
    SetConsoleCtrlHandler(signalHandler, TRUE);
    timer.start(500, this);
}


void TWebApplication::ignoreConsoleSignal()
{
    SetConsoleCtrlHandler(NULL, TRUE);
    timer.stop();
}

#if QT_VERSION < 0x050000

bool TWebApplication::winEventFilter(MSG *msg, long *result)
{
    if (msg->message == WM_CLOSE) {
        quit();
    } else if (msg->message == WM_APP) {
        exit(1);
    }
    return QCoreApplication::winEventFilter(msg, result);
}

#else

bool TNativeEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, long *)
{
    if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
        MSG *msg = static_cast<MSG *>(message);
        if (msg->message == WM_CLOSE) {
            Tf::app()->quit();
        } else if (msg->message == WM_APP) {
            Tf::app()->exit(1);
        }
    }
    return false;
}


void TWebApplication::watchLocalSocket()
{
    QLocalServer *server = new QLocalServer(this);
    server->setSocketOptions(QLocalServer::UserAccessOption);
    connect(server, SIGNAL(newConnection()), this, SLOT(recvLocalSocket()));
    server->listen(LOCAL_SERVER_PREFIX + QString::number(QCoreApplication::applicationPid()));
}


void TWebApplication::recvLocalSocket()
{
    QLocalServer *server = qobject_cast<QLocalServer *>(QObject::sender());
    if (server) {
        QLocalSocket *socket = server->nextPendingConnection();
        if (socket->waitForReadyRead(50)) {
            QByteArray data = socket->readAll();
            bool ok;
            int num = data.toInt(&ok);

            if (ok) {
                switch (num) {
                case WM_CLOSE:
                    quit();
                    break;

                case  WM_APP:
                    exit(1);
                    break;

                default:
                    break;
                }
            }
        }
    }
}


bool TWebApplication::sendLocalCtrlMessage(const QByteArray &msg,  int targetProcess)
{
    // Sends to the local socket
    bool ret = false;
    QLocalSocket *socket = new QLocalSocket();

    socket->connectToServer(LOCAL_SERVER_PREFIX + QString::number(targetProcess));
    if (socket->waitForConnected(1000)) {
        socket->write(msg);
        socket->flush();
        socket->close();
        ret = true;
    }

    delete socket;
    return ret;
}

#endif // QT_VERSION < 0x050000
