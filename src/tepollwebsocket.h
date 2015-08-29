#ifndef TEPOLLWEBSOCKET_H
#define TEPOLLWEBSOCKET_H

#include <QList>
#include <QPair>
#include <TGlobal>
#include <THttpResponseHeader>
#include "tepollsocket.h"
#include "tabstractwebsocket.h"

class QHostAddress;
class TWebSocketWorker;
class TWebSocketFrame;
class TSession;
class THttpRequestHeader;


class T_CORE_EXPORT TEpollWebSocket : public TEpollSocket, public TAbstractWebSocket
{
    Q_OBJECT
public:
    virtual ~TEpollWebSocket();

    bool isTextRequest() const;
    bool isBinaryRequest() const;
    QList<QPair<int, QByteArray>> readAllBinaryRequest();
    virtual bool canReadRequest();
    virtual void startWorker();
    virtual void deleteLater();
    void startWorkerForOpening(const TSession &session);
    void startWorkerForClosing();
    void disconnect() Q_DECL_OVERRIDE;
    QByteArray socketUuid() const { return TEpollSocket::socketUuid(); }
    static TEpollWebSocket *searchSocket(const QByteArray &uuid);

public slots:
    void releaseWorker();
    void sendTextForPublish(const QString &text, const QObject *except);
    void sendBinaryForPublish(const QByteArray &binary, const QObject *except);
    void sendPong(const QByteArray &data = QByteArray());

protected:
    virtual void *getRecvBuffer(int size) Q_DECL_OVERRIDE;
    virtual bool seekRecvBuffer(int pos) Q_DECL_OVERRIDE;
    virtual QObject *thisObject() Q_DECL_OVERRIDE { return this; }
    virtual qint64 writeRawData(const QByteArray &data) Q_DECL_OVERRIDE;
    virtual QList<TWebSocketFrame> &websocketFrames() Q_DECL_OVERRIDE { return frames; }
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;
    void clear();

private:
    void startWorker(TWebSocketWorker *worker);

    QByteArray recvBuffer;
    QList<TWebSocketFrame> frames;

    TEpollWebSocket(int socketDescriptor, const QHostAddress &address, const THttpRequestHeader &header);

    friend class TEpoll;
    Q_DISABLE_COPY(TEpollWebSocket)
};

#endif // TEPOLLWEBSOCKET_H
