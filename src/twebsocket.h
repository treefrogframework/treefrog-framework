#ifndef TWEBSOCKET_H
#define TWEBSOCKET_H

#include <QTcpSocket>
#include <QList>
#include <QByteArray>
#include <TGlobal>
#include "tatomic.h"
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

    qintptr socketDescriptor() const override { return QTcpSocket::socketDescriptor(); }
    int socketId() const override { return sid; }
    bool canReadRequest() const;
    void disconnect() override;
    static TAbstractWebSocket *searchSocket(int sid);

public slots:
    void sendTextForPublish(const QString &text, const QObject *except);
    void sendBinaryForPublish(const QByteArray &binary, const QObject *except);
    void sendPong(const QByteArray &data = QByteArray());
    void readRequest();
    void releaseWorker();
    void sendRawData(const QByteArray &data);
    void close() override;
    void deleteLater();

protected:
    void startWorkerForOpening(const TSession &session);
    void startWorkerForClosing();
    virtual QObject *thisObject() override { return this; }
    virtual qint64 writeRawData(const QByteArray &data) override;
    virtual QList<TWebSocketFrame> &websocketFrames() override { return frames; }
    void timerEvent(QTimerEvent *event) override;
    QList<TWebSocketFrame> frames;

signals:
    void sendByWorker(const QByteArray &data);
    void disconnectByWorker();

private:
    void startWorker(TWebSocketWorker *worker);

    int sid;
    QByteArray recvBuffer;
    TAtomic<int> myWorkerCounter;
    TAtomic<bool> deleting;

    friend class TActionThread;
    T_DISABLE_COPY(TWebSocket)
    T_DISABLE_MOVE(TWebSocket)
};

#endif // TWEBSOCKET_H
