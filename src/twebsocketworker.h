#ifndef TWEBSOCKETWORKER_H
#define TWEBSOCKETWORKER_H

#include <QThread>
#include <TGlobal>
#include <TSession>
#include "twebsocketframe.h"

class TEpollSocket;


class T_CORE_EXPORT TWebSocketWorker : public QThread
{
    Q_OBJECT
public:
    TWebSocketWorker(TEpollWebSocket *socket, const TSession &session, QObject *parent = 0);
    TWebSocketWorker(TEpollWebSocket *socket, const QByteArray &path, TWebSocketFrame::OpCode opCode, const QByteArray &data, QObject *parent = 0);
    virtual ~TWebSocketWorker();

protected:
    void run();

private:
    TEpollWebSocket *socket;
    TSession sessionStore;
    QByteArray requestPath;
    TWebSocketFrame::OpCode opcode;
    QByteArray requestData;
};

#endif // TWEBSOCKETWORKER_H
