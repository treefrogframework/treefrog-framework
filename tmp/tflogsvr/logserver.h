#ifndef LOGSERVER_H
#define LOGSERVER_H

#include <QObject>
#include <QString>
#include <QFile>

class QLocalServer;


class LogServer : public QObject
{
    Q_OBJECT
public:
    LogServer(const QString &webAppName, const QString &logFileName, QObject *parent = 0);

    bool start();
    void stop();
    QString serverName() const;
    
protected slots:
    void accept() const;
    void cleanup() const;
    void writeLog();

private:
    QLocalServer *server;
    QString webApplicationName;
    QFile logFile;
};

#endif // LOGSERVER_H
