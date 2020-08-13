#pragma once
#include <QAbstractSocket>
#include <QByteArray>
#include <QHostAddress>
#include <QObject>
#include <TGlobal>
#include <THttpRequest>
#include <TTemporaryFile>


class T_CORE_EXPORT THttpSocket : public QObject {
    Q_OBJECT
public:
    THttpSocket(QByteArray &readBuffer, QObject *parent = 0);
    virtual ~THttpSocket();

    QList<THttpRequest> read();
    bool waitForReadyReadRequest(int msecs = 5000);
    bool canReadRequest() const;
    qint64 write(const THttpHeader *header, QIODevice *body);
    int idleTime() const;
    int socketId() const { return _sid; }
    void abort();
    void deleteLater();

    int socketDescriptor() const { return _socket; }
    void setSocketDescriptor(int socketDescriptor, QAbstractSocket::SocketState socketState);
    QHostAddress peerAddress() const { return _peerAddr; }
    ushort peerPort() const { return _peerPort; }
    QAbstractSocket::SocketState state() const { return _state; }

    static THttpSocket *searchSocket(int id);
    void writeRawDataFromWebSocket(const QByteArray &data);

protected:
    int readRawData(char *data, int size, int msecs);

protected slots:
    qint64 writeRawData(const char *data, qint64 size);
    qint64 writeRawData(const QByteArray &data);

signals:
    void requestWrite(const QByteArray &data);  // internal use

private:
    T_DISABLE_COPY(THttpSocket)
    T_DISABLE_MOVE(THttpSocket)

    int _sid {0};
    int _socket {0};
    QAbstractSocket::SocketState _state {QAbstractSocket::UnconnectedState};
    QHostAddress _peerAddr;
    ushort _peerPort {0};
    qint64 _lengthToRead {-1};
    QByteArray &_readBuffer;
    QByteArray _headerBuffer;
    TTemporaryFile _fileBuffer;
    quint64 _idleElapsed {0};

    friend class TActionThread;
};


inline bool THttpSocket::canReadRequest() const
{
    return (_lengthToRead == 0);
}
