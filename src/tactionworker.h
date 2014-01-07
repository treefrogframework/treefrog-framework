#ifndef TACTIONWORKER_H
#define TACTIONWORKER_H

#include <QThread>
#include <TActionContext>

class THttpRequest;
class THttpResponseHeader;
class TEpollSocket;
class QIODevice;


class T_CORE_EXPORT TActionWorker : public QThread, public TActionContext
{
    Q_OBJECT
public:
    ~TActionWorker();
    static int workerCount();
    static bool waitForAllDone(int msec);

protected:
    void run();
    qint64 writeResponse(THttpResponseHeader &header, QIODevice *body);
    void closeHttpSocket();

private:
    QByteArray httpRequest;
    QString clientAddr;
    int socketId;  // not socket descriptor

    TActionWorker(TEpollSocket *socket, QObject *parent = 0);

    Q_DISABLE_COPY(TActionWorker)
    friend class TEpollSocket;
};

#endif // TACTIONWORKER_H
