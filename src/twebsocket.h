#ifndef TWEBSOCKET_H
#define TWEBSOCKET_H

#include <QTcpSocket>
#include <QList>
#include <QByteArray>
#include <QAtomicInt>
#include <TGlobal>
#include <THttpRequestHeader>
#include "tabstractwebsocket.h"

class TWebSocketFrame;


class T_CORE_EXPORT TWebSocket : public QTcpSocket, public TAbstractWebSocket
{
    Q_OBJECT
public:
    TWebSocket(QObject *parent = 0);
    virtual ~TWebSocket();

    void close();
    const QByteArray &socketUuid() const { return uuid; }
    int countWorkers() const;
    bool canReadRequest() const;

    void sendText(const QString &message) override { }
    void sendBinary(const QByteArray &data) override { }
    void sendPing() override { }
    void sendPong() override { }
    void disconnect() override { }

public slots:
    void readRequest();
    void deleteLater();
    void releaseWorker();

protected:
    virtual void sendData(const QByteArray &data);
    virtual QList<TWebSocketFrame> &websocketFrames() { return frames; }
    QList<TWebSocketFrame> frames;

private:
    QByteArray uuid;
    THttpRequestHeader reqHeader;
    QByteArray recvBuffer;
    QAtomicInt myWorkerCounter;
    volatile bool deleting;

    Q_DISABLE_COPY(TWebSocket)
};


inline int TWebSocket::countWorkers() const
{
#if QT_VERSION >= 0x050000
    return myWorkerCounter.load();
#else
    return (int)myWorkerCounter;
#endif
}

#endif // TWEBSOCKET_H
