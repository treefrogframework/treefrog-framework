#pragma once
#include "tabstractwebsocket.h"
#include "tepollsocket.h"
#include <QList>
#include <QObject>
#include <QPair>
#include <TGlobal>
#include <THttpResponseHeader>

class QHostAddress;
class TWebSocketWorker;
class TWebSocketFrame;
class TSession;
class THttpRequestHeader;


class T_CORE_EXPORT TEpollWebSocket : public QObject, public TEpollSocket, public TAbstractWebSocket {
    Q_OBJECT
public:
    virtual ~TEpollWebSocket();

    bool isTextRequest() const;
    bool isBinaryRequest() const;
    QList<QPair<int, QByteArray>> readAllBinaryRequest();
    virtual bool canReadRequest() override;
    virtual void process() override;
    virtual bool isProcessing() const override { return (bool)_worker; }
    void startWorkerForOpening(const TSession &session);
    void startWorkerForClosing();
    void disconnect() override;
    qintptr socketDescriptor() const override { return TEpollSocket::socketDescriptor(); }
    static TEpollWebSocket *searchSocket(int socket);

public slots:
    void releaseWorker();
    void sendTextForPublish(const QString &text, const QObject *except);
    void sendBinaryForPublish(const QByteArray &binary, const QObject *except);
    void sendPong(const QByteArray &data = QByteArray());

protected:
    virtual bool seekRecvBuffer(int pos) override;
    virtual QObject *thisObject() override { return this; }
    virtual int64_t writeRawData(const QByteArray &data) override;
    virtual QList<TWebSocketFrame> &websocketFrames() override { return _frames; }
    void timerEvent(QTimerEvent *event) override;
    void clear();

private:
    void startWorker(TWebSocketWorker *worker);
    TEpollWebSocket(int socketDescriptor, const QHostAddress &address, const THttpRequestHeader &header);

    QList<TWebSocketFrame> _frames;
    TWebSocketWorker *_worker {nullptr};

    friend class TEpoll;
    T_DISABLE_COPY(TEpollWebSocket)
    T_DISABLE_MOVE(TEpollWebSocket)
};
