#ifndef TACTIONWORKER_H
#define TACTIONWORKER_H

#include <QThread>
#include <TActionContext>

class THttpRequest;
class THttpResponseHeader;
class TEpollHttpSocket;
class QIODevice;


class T_CORE_EXPORT TActionWorker : public QThread, public TActionContext
{
    Q_OBJECT
public:
    TActionWorker(TEpollHttpSocket *socket, QObject *parent = 0);
    virtual ~TActionWorker();

    static int workerCount();
    static bool waitForAllDone(int msec);

protected:
    void run();
    qint64 writeResponse(THttpResponseHeader &header, QIODevice *body) override;
    void closeHttpSocket();

private:
    QByteArray httpRequest;
    QString clientAddr;
    TEpollHttpSocket *socket;

    T_DISABLE_COPY(TActionWorker)
    T_DISABLE_MOVE(TActionWorker)
};

#endif // TACTIONWORKER_H
