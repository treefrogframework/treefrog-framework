#ifndef TWEBSOCKET_H
#define TWEBSOCKET_H

#include <QTcpSocket>
#include <QList>
#include <QByteArray>
#include <TGlobal>
#include <atomic>
#include "tabstractwebsocket.h"

class TWebSocketFrame;
class TWebSocketWorker;
class TSession;
class THttpRequestHeader;


class T_CORE_EXPORT TWebSocket : public QTcpSocket, public TAbstractWebSocket
{
    Q_OBJECT
public:
    TWebSocket(int socketDescriptor, const QHostAddress &address, const THttpRequestHeader &header, QObject *parent = 0);
    virtual ~TWebSocket();

    QByteArray socketUuid() const { return uuid; }
    bool canReadRequest() const;
    void disconnect() Q_DECL_OVERRIDE;
    static TAbstractWebSocket *searchSocket(const QByteArray &uuid);

public slots:
    void sendTextForPublish(const QString &text, const QObject *except);
    void sendBinaryForPublish(const QByteArray &binary, const QObject *except);
    void sendPong(const QByteArray &data = QByteArray());
    void readRequest();
    void releaseWorker();
    void sendRawData(const QByteArray &data);
    void close();
    void deleteLater();

protected:
    void startWorkerForOpening(const TSession &session);
    void startWorkerForClosing();
    virtual QObject *thisObject() Q_DECL_OVERRIDE { return this; }
    virtual qint64 writeRawData(const QByteArray &data) Q_DECL_OVERRIDE;
    virtual QList<TWebSocketFrame> &websocketFrames() Q_DECL_OVERRIDE { return frames; }
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;
    QList<TWebSocketFrame> frames;

signals:
    void sendByWorker(const QByteArray &data);
    void disconnectByWorker();

private:
    void startWorker(TWebSocketWorker *worker);

    QByteArray uuid;
    QByteArray recvBuffer;
    std::atomic<int> myWorkerCounter;
    std::atomic<bool> deleting;

    friend class TActionThread;
    Q_DISABLE_COPY(TWebSocket)
};

#endif // TWEBSOCKET_H
