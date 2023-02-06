#include "ttcpsocket.h"
#include "tepollsocket.h"
#include "tepoll.h"
#include "tsystemglobal.h"
#include "tfcore.h"
#include <QMutex>
#include <TWebApplication>

namespace {
#if QT_VERSION < 0x060000
QMutex epollMutex(QMutex::Recursive);
#else
QRecursiveMutex epollMutex;
#endif
}


TTcpSocket::TTcpSocket() :
    _esocket(new TEpollSocket)
{ }


TTcpSocket::~TTcpSocket()
{
    delete _esocket;
}


int TTcpSocket::socketDescriptor() const
{
    return _esocket->socketDescriptor();
}


bool TTcpSocket::setSocketOption(int level, int optname, int val)
{
    return _esocket->setSocketOption(level, optname, val);
}


void TTcpSocket::connectToHost(const QString &hostName, quint16 port)
{
    if (state() != Tf::SocketState::Unconnected) {
        tSystemWarn("Invalid socket state [%s:%d]", __FILE__, __LINE__);
        return;
    }

    _esocket->setAutoDelete(false);
    _esocket->connectToHost(QHostAddress(hostName), port);
}


bool TTcpSocket::waitUntil(bool (TEpollSocket::*method)(), int msecs)
{
    QElapsedTimer elapsed;
    elapsed.start();

    while (!(_esocket->*method)()) {
        int ms = msecs - elapsed.elapsed();
        if (ms <= 0) {
            break;
        }

        if (Tf::app()->multiProcessingModule() == TWebApplication::Epoll || epollMutex.tryLock()) {
            int ret = _esocket->waitUntil(method, msecs);
            if (Tf::app()->multiProcessingModule() != TWebApplication::Epoll) {
                epollMutex.unlock();
            }
            return ret;

        } else {
            Tf::msleep(1);  // context-switch
        }
    }
    return (_esocket->*method)();
}


bool TTcpSocket::waitForConnected(int msecs)
{
    if (state() == Tf::SocketState::Unconnected) {
        tSystemError("Invalid socket state [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    if (state() == Tf::SocketState::Connected) {
        return true;
    }

    return waitUntil((bool (TEpollSocket::*)())&TEpollSocket::isConnected, msecs);
}


bool TTcpSocket::waitForDataReceived(int msecs)
{
    if (state() != Tf::SocketState::Connected) {
        tSystemError("Invalid socket state [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    return waitUntil((bool (TEpollSocket::*)())&TEpollSocket::isDataReceived, msecs);
}


bool TTcpSocket::waitForDataSent(int msecs)
{
    if (state() != Tf::SocketState::Connected) {
        tSystemError("Invalid socket state [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    return waitUntil((bool (TEpollSocket::*)())&TEpollSocket::isDataSent, msecs);
}


Tf::SocketState TTcpSocket::state() const
{
    return _esocket->state();
}


qint64 TTcpSocket::receivedSize() const
{
    return _esocket->receivedSize();
}


qint64 TTcpSocket::receiveData(char *data, qint64 maxSize)
{
    if (state() != Tf::SocketState::Connected) {
        tSystemError("Invalid socket state [%s:%d]", __FILE__, __LINE__);
        return -1;
    }

    return _esocket->receiveData(data, maxSize);
}


QByteArray TTcpSocket::receiveAll()
{
    if (state() != Tf::SocketState::Connected) {
        tSystemError("Invalid socket state [%s:%d]", __FILE__, __LINE__);
        return QByteArray();
    }

    return _esocket->receiveAll();
}


qint64 TTcpSocket::sendData(const char *data, qint64 size)
{
    QByteArray buf(data, size);
    return sendData(buf);
}


qint64 TTcpSocket::sendData(const QByteArray &data)
{
    if (state() != Tf::SocketState::Connected) {
        tSystemError("Invalid socket state [%s:%d]", __FILE__, __LINE__);
        return -1;
    }

    _esocket->sendData(data);
    return data.length();
}


void TTcpSocket::close()
{
    tSystemDebug("TTcpSocket::close");
    _esocket->close();
}
