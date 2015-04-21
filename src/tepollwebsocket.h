#ifndef TEPOLLWEBSOCKET_H
#define TEPOLLWEBSOCKET_H

#include <TGlobal>
#include <THttpRequestHeader>
#include <THttpResponseHeader>
#include "tepollsocket.h"
#include "tabstractwebsocket.h"

class QHostAddress;
class TWebSocketWorker;
class TWebSocketFrame;
class TSession;


class T_CORE_EXPORT TEpollWebSocket : public TEpollSocket, public TAbstractWebSocket
{
    Q_OBJECT
public:
    virtual ~TEpollWebSocket();

    bool isTextRequest() const;
    bool isBinaryRequest() const;
    QByteArray readBinaryRequest();
    virtual bool canReadRequest();
    virtual void startWorker();
    void startWorkerForOpening(const TSession &session);
    void startWorkerForClosing();
    void disconnect() Q_DECL_OVERRIDE;

public slots:
    void deleteLater();
    void releaseWorker();
    void sendText(const QString &text);
    void sendBinary(const QByteArray &binary);
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

    THttpRequestHeader reqHeader;
    QByteArray recvBuffer;
    QList<TWebSocketFrame> frames;

    TEpollWebSocket(int socketDescriptor, const QHostAddress &address, const THttpRequestHeader &header);

    friend class TEpoll;
    Q_DISABLE_COPY(TEpollWebSocket)
};

#endif // TEPOLLWEBSOCKET_H
