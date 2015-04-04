#ifndef TWEBSOCKETWORKER_H
#define TWEBSOCKETWORKER_H

#include <QThread>
#include <TGlobal>
#include <TSession>
#include "twebsocketframe.h"


class T_CORE_EXPORT TWebSocketWorker : public QThread
{
    Q_OBJECT
public:
    TWebSocketWorker(const QByteArray &socket, const TSession &session, QObject *parent = 0);
    TWebSocketWorker(const QByteArray &socket, const QByteArray &path, TWebSocketFrame::OpCode opCode, const QByteArray &data, QObject *parent = 0);
    virtual ~TWebSocketWorker();

protected:
    void run();

private:
    QByteArray socketUuid;
    TSession sessionStore;
    QByteArray requestPath;
    TWebSocketFrame::OpCode opcode;
    QByteArray requestData;
};

#endif // TWEBSOCKETWORKER_H
