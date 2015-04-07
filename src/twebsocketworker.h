#ifndef TWEBSOCKETWORKER_H
#define TWEBSOCKETWORKER_H

#include <QThread>
#include <TGlobal>
#include <TSession>
#include "tdatabasecontext.h"
#include "twebsocketframe.h"

class TAbstractWebSocket;


class T_CORE_EXPORT TWebSocketWorker : public QThread, public TDatabaseContext
{
    Q_OBJECT
public:
    TWebSocketWorker(TAbstractWebSocket *socket, const QByteArray &path, const TSession &session, QObject *parent = 0);
    TWebSocketWorker(TAbstractWebSocket *socket, const QByteArray &path, TWebSocketFrame::OpCode opCode,
                     const QByteArray &data, QObject *parent = 0);
    virtual ~TWebSocketWorker();

protected:
    void run();

private:
    TAbstractWebSocket *socket;
    TSession sessionStore;
    QByteArray requestPath;
    TWebSocketFrame::OpCode opcode;
    QByteArray requestData;
};

#endif // TWEBSOCKETWORKER_H
