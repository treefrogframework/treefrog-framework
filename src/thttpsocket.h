#pragma once
#include <QAbstractSocket>
#include <QByteArray>
#include <QHostAddress>
#include <QObject>
#include <TGlobal>
#include <THttpRequest>
#include <TTemporaryFile>

class TActionContext;


class T_CORE_EXPORT THttpSocket : public QObject {
    Q_OBJECT
public:
    THttpSocket(QByteArray &readBuffer, TActionContext *context, QObject *parent = 0);
    virtual ~THttpSocket();

    QList<THttpRequest> read();
    bool waitForReadyReadRequest(int msecs = 5000);
    bool canReadRequest() const;
    int64_t write(const THttpHeader *header, QIODevice *body);
    int idleTime() const;
    void abort();
    void deleteLater();

    qintptr socketDescriptor() const { return _socket; }
    void setSocketDescriptor(qintptr socketDescriptor, QAbstractSocket::SocketState socketState);
    QHostAddress peerAddress() const { return _peerAddr; }
    ushort peerPort() const { return _peerPort; }
    QAbstractSocket::SocketState state() const { return _state; }
    void writeRawDataFromWebSocket(const QByteArray &data);

protected:
    int readRawData(char *data, int size, int msecs);

protected slots:
    int64_t writeRawData(const char *data, int64_t size);
    int64_t writeRawData(const QByteArray &data);

signals:
    void requestWrite(const QByteArray &data);  // internal use

private:
    qintptr _socket {0};
    QAbstractSocket::SocketState _state {QAbstractSocket::UnconnectedState};
    QHostAddress _peerAddr;
    ushort _peerPort {0};
    int64_t _lengthToRead {-1};
    QByteArray &_readBuffer;
    QByteArray _headerBuffer;
    TTemporaryFile _fileBuffer;
    uint64_t _idleElapsed {0};
    TActionContext *_context {nullptr};

    friend class TActionThread;
    T_DISABLE_COPY(THttpSocket)
    T_DISABLE_MOVE(THttpSocket)
};


inline bool THttpSocket::canReadRequest() const
{
    return (_lengthToRead == 0);
}
