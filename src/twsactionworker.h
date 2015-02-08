#ifndef TWSACTIONWORKER_H
#define TWSACTIONWORKER_H

#include <QThread>
#include <TGlobal>
#include "tepollwebsocket.h"

class T_CORE_EXPORT TWsActionWorker : public QThread
{
    Q_OBJECT
public:
    TWsActionWorker(TEpollWebSocket::OpCode opCode, const QByteArray &data, QObject *parent = 0);
    ~TWsActionWorker();

protected:
    void run();

private:
    TEpollWebSocket::OpCode opcode;
    QByteArray requestData;
};

#endif // TWSACTIONWORKER_H
