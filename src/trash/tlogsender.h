#ifndef TLOGSENDER_H
#define TLOGSENDER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QBasicTimer>
#include <TLogWriter>

class QLocalSocket;


class TLogSender : public TAbstractLogWriter, public QObject
{
public:
    TLogSender(const QString &server);
    virtual ~TLogSender();

    void reopen();
    void writeLog(const QByteArray &log);
    bool waitForConnected(int msecs = 30000);

protected:
    int send(const QByteArray &log) const;
    void sendFromQueue();
    void timerEvent(QTimerEvent *event);

private:
    QString serverName;
    QByteArray queue;
    QLocalSocket *socket;
    QBasicTimer timer;
};

#endif // TLOGSENDER_H
