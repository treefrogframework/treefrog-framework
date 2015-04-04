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
    TWebSocket(QObject *parent);
    virtual ~TWebSocket();

    void close();
    void readRequest();
    const QByteArray &socketUuid() const { return uuid; }
    int countWorkers() const;

public slots:
    void cleanup();
    void cleanupWorker();

protected:
    virtual void sendData(const QByteArray &data);
    virtual QList<TWebSocketFrame> &websocketFrames() { return frames; }
    QList<TWebSocketFrame> frames;

private:
    QByteArray uuid;
    THttpRequestHeader reqHeader;
    QByteArray recvBuffer;
    QAtomicInt workerCounter;
    volatile bool closing;

    Q_DISABLE_COPY(TWebSocket)
};


inline int TWebSocket::countWorkers() const
{
#if QT_VERSION >= 0x050000
    return workerCounter.load();
#else
    return (int)workerCounter;
#endif
}

#endif // TWEBSOCKET_H
