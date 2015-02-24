#ifndef TWSACTIONWORKER_H
#define TWSACTIONWORKER_H

#include <QThread>
#include <TGlobal>
#include "tepollwebsocket.h"

class T_CORE_EXPORT TWsActionWorker : public QThread
{
    Q_OBJECT
public:
    TWsActionWorker(const QByteArray &socket, const QByteArray &path, TEpollWebSocket::OpCode opCode, const QByteArray &data, QObject *parent = 0);
    ~TWsActionWorker();

protected:
    void run();

private:
    QByteArray socketUuid;
    QByteArray requestPath;
    TEpollWebSocket::OpCode opcode;
    QByteArray requestData;
};

#endif // TWSACTIONWORKER_H
