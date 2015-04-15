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
    ServerManager(int min = 5, int max = 10, int spare = 5, QObject *parent = 0);
    virtual ~ServerManager();

    bool start(const QHostAddress &address = QHostAddress::Any, quint16 port = 0);
    bool start(const QString &fileDomain);  // For UNIX domain
    void stop();
    bool isRunning() const;
    int serverCount() const;
    int spareServerCount() const;

protected:
    enum ServerProcessState {
        NotRunning = 0,
        Listening,
        Running,
        Closing,
    };

    void ajustServers() const;
    void startServer() const;
    void readProcess(QProcess *process);

protected slots:
    void updateServerStatus();
    void errorDetect(QProcess::ProcessError error);
    void serverFinish(int exitCode, QProcess::ExitStatus exitStatus) const;
    void readStandardOutput();
    void readStandardOutputOfAll();
    void readStandardError() const;

private:
    int listeningSocket;
    int maxServers;
    int minServers;
    int spareServers;
    volatile bool running;
    volatile bool pipeReading;

    Q_DISABLE_COPY(ServerManager)
};

}
#endif // SERVERMANAGER_H
