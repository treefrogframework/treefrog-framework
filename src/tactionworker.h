#pragma once
#include <QHostAddress>
#include <TActionContext>

class THttpRequest;
class THttpResponseHeader;
class TEpollHttpSocket;
class QIODevice;


class T_CORE_EXPORT TActionWorker : public QObject, public TActionContext {
    Q_OBJECT
public:
    TActionWorker() { }
    virtual ~TActionWorker() { }
    void start(TEpollHttpSocket *socket);

    //static TActionWorker *instance();
    //static int workerCount();

protected:
    void run();
    qint64 writeResponse(THttpResponseHeader &header, QIODevice *body) override;
    void closeHttpSocket() override;

private:

    QByteArray _httpRequest;
    QHostAddress _clientAddr;
    TEpollHttpSocket *_socket {nullptr};

    T_DISABLE_COPY(TActionWorker)
    T_DISABLE_MOVE(TActionWorker)
};

