#pragma once
#include <QHostAddress>
#include <QObject>
#include <QProcess>
#include <TGlobal>

namespace TreeFrog {


class ServerManager : public QObject {
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

    bool start(const QHostAddress &address, quint16 port);
    bool start(const QString &fileDomain);  // For UNIX domain
    void stop();
    bool isRunning() const;
    ManagerState state() const { return managerState; }
    int serverCount() const;
    int spareServerCount() const;
    static QString tfserverProgramPath();
    static void setupEnvironment(QProcess *process);

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

    T_DISABLE_COPY(ServerManager)
    T_DISABLE_MOVE(ServerManager)
};

}
