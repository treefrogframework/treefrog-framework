#ifndef SYSTEMBUSDAEMON_H
#define SYSTEMBUSDAEMON_H

#include <QObject>
#include <QSet>
#include <TGlobal>

class QLocalServer;
class QLocalSocket;


class SystemBusDaemon : QObject
{
    Q_OBJECT
public:
    ~SystemBusDaemon();
    bool open();
    void close();

    static SystemBusDaemon *instance();
    static void instantiate();
    static void releaseResource(qint64 pid);

protected slots:
    void acceptConnection();
    void readSocket();
    void handleDisconnect();

private:
    QLocalServer *localServer;
    QSet<QLocalSocket *> socketSet;

    SystemBusDaemon();

    T_DISABLE_COPY(SystemBusDaemon)
    T_DISABLE_MOVE(SystemBusDaemon)
};

#endif // SYSTEMBUSDAEMON_H
