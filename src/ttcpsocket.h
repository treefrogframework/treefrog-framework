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
    void connectToHost(const QString &hostName, uint16_t port);
    bool waitForConnected(int msecs);
    bool waitForDataReceived(int msecs);
    bool waitForDataSent(int msecs);
    int64_t receivedSize() const;
    int64_t receiveData(char *data, int64_t maxSize);
    QByteArray receiveAll();
    int64_t sendData(const char *data, int64_t size);
    int64_t sendData(const QByteArray &data);

protected:
    bool waitUntil(bool (TEpollSocket::*method)(), int msecs);

private:
    TEpollSocket *_esocket {nullptr};
};
