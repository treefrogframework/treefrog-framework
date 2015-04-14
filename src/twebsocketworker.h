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
    enum RunMode {
        Opening,
        Receiving,
        Closing,
    };

    TWebSocketWorker(RunMode mode, TAbstractWebSocket *socket, const QByteArray &path, QObject *parent = 0);
    virtual ~TWebSocketWorker();

    void setPayload(TWebSocketFrame::OpCode opCode, const QByteArray &data);
    void setSession(const TSession &session);

protected:
    void run();

private:
    RunMode mode_;
    TAbstractWebSocket *socket_;
    TSession sessionStore_;
    QByteArray requestPath_;
    TWebSocketFrame::OpCode opcode_;
    QByteArray requestData_;
};

#endif // TWEBSOCKETWORKER_H
