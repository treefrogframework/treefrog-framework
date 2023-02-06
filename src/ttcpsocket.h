#pragma once
#include <QString>
#include <TGlobal>

class TEpollSocket;


class T_CORE_EXPORT TTcpSocket {
public:
    TTcpSocket();
    virtual ~TTcpSocket();

    Tf::SocketState state() const;
    int socketDescriptor() const;
    void close();
    bool setSocketOption(int level, int optname, int val);
    void connectToHost(const QString &hostName, quint16 port);
    bool waitForConnected(int msecs);
    bool waitForDataReceived(int msecs);
    bool waitForDataSent(int msecs);
    qint64 receivedSize() const;
    qint64 receiveData(char *data, qint64 maxSize);
    QByteArray receiveAll();
    qint64 sendData(const char *data, qint64 size);
    qint64 sendData(const QByteArray &data);

protected:
    bool waitUntil(bool (TEpollSocket::*method)(), int msecs);

private:
    TEpollSocket *_esocket {nullptr};
};
