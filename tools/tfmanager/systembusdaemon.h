#ifndef SYSTEMBUSDAEMON_H
#define SYSTEMBUSDAEMON_H

#include <QObject>
#include <QSet>

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
    Q_DISABLE_COPY(SystemBusDaemon)
};

#endif // SYSTEMBUSDAEMON_H
