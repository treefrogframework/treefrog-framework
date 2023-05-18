#pragma once
#include <QList>
#include <QString>

namespace TreeFrog {


class ProcessInfo {
public:
    ProcessInfo(int64_t pid);

    int64_t pid() const { return processId; }
    int64_t ppid() const;
    QString processName() const;
    bool exists() const;

    void terminate();  // SIGTERM
    void kill();  // SIGKILL
    void restart();  // SIGHUP
    bool waitForTerminated(int msecs = 10000);
    QList<int64_t> childProcessIds() const;

    static void kill(int64_t ppid);
    static void kill(QList<int64_t> pids);
    static QList<int64_t> pidsOf(const QString &processName);
    static QList<int64_t> allConcurrentPids();

private:
    int64_t processId;
};

}
