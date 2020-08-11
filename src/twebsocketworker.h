#pragma once
#include "tdatabasecontextthread.h"
#include "twebsocketframe.h"
#include <QList>
#include <QPair>
#include <QThread>
#include <TGlobal>
#include <TSession>

class TAbstractWebSocket;


class T_CORE_EXPORT TWebSocketWorker : public TDatabaseContextThread {
    Q_OBJECT
public:
    enum RunMode {
        Opening = 0,
        Receiving,
        Closing,
    };

    TWebSocketWorker(RunMode mode, TAbstractWebSocket *socket, const QByteArray &path, QObject *parent = 0);
    virtual ~TWebSocketWorker();

    void setPayload(TWebSocketFrame::OpCode opCode, const QByteArray &data);
    void setPayloads(QList<QPair<int, QByteArray>> payloads);
    void setSession(const TSession &session);

protected:
    void run() override;
    void execute(int opcode = 0, const QByteArray &payload = QByteArray());

private:
    RunMode _mode {Opening};
    TAbstractWebSocket *_socket {nullptr};
    TSession _httpSession;
    QByteArray _requestPath;
    QList<QPair<int, QByteArray>> _payloads;
};

