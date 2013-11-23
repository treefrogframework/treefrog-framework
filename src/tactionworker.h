#ifndef TACTIONWORKER_H
#define TACTIONWORKER_H

#include <QThread>
#include <TActionContext>

class THttpRequest;
class THttpResponseHeader;
class QIODevice;


class T_CORE_EXPORT TActionWorker : public QThread, public TActionContext
{
    Q_OBJECT
public:
    TActionWorker(int socket, const QByteArray &request, const QString &address, QObject *parent = 0);
    virtual ~TActionWorker();

protected:
    void run();
    qint64 writeResponse(THttpResponseHeader &header, QIODevice *body);
    void closeHttpSocket();

private:
    QByteArray httpRequest;
    QString clientAddr;

    Q_DISABLE_COPY(TActionWorker)
};

#endif // TACTIONWORKER_H
