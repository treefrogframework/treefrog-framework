#ifndef SERVERMANAGER_H
#define SERVERMANAGER_H

#include <QObject>
#include <QHostAddress>
#include <QProcess>

namespace TreeFrog {


class ServerManager : public QObject
{
    Q_OBJECT
public:
    enum ManagerState {
        NotRunning = 0,
        Starting,
        Running,
        Stopping,
    };

    ServerManager(int min = 5, int max = 10, int spare = 5, QObject *parent = 0);
    virtual ~ServerManager();

    bool start(const QHostAddress &address = QHostAddress::Any, quint16 port = 0);
    bool start(const QString &fileDomain);  // For UNIX domain
    void stop();
    bool isRunning() const;
    ManagerState state() const { return managerState; }
    int serverCount() const;
    int spareServerCount() const;

protected:
    void ajustServers();
    void startServer(int id = -1) const;

protected slots:
    void updateServerStatus();
    void errorDetect(QProcess::ProcessError error);
    void serverFinish(int exitCode, QProcess::ExitStatus exitStatus);
    void readStandardError() const;

private:
    int listeningSocket;
    int maxServers;
    int minServers;
    int spareServers;
    volatile ManagerState managerState;

    Q_DISABLE_COPY(ServerManager)
};

}
#endif // SERVERMANAGER_H
