#ifndef TPREFORKAPPLICATIONSERVER_H
#define TPREFORKAPPLICATIONSERVER_H

#include <QTcpServer>
#include <TGlobal>
#include "tapplicationserverbase.h"


class T_CORE_EXPORT TPreforkApplicationServer : public QTcpServer, public TApplicationServerBase
{
    Q_OBJECT
public:
    TPreforkApplicationServer(QObject *parent = 0);
    ~TPreforkApplicationServer();

    bool start();
    void stop();

protected:
#if QT_VERSION >= 0x050000
    void incomingConnection(qintptr socketDescriptor);
#else
    void incomingConnection(int socketDescriptor);
#endif

private:
    Q_DISABLE_COPY(TPreforkApplicationServer)
};

#endif // TPREFORKAPPLICATIONSERVER_H
