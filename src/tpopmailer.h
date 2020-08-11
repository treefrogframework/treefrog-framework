#pragma once
#include <QByteArray>
#include <QObject>
#include <QString>
#include <TGlobal>

class QTcpSocket;


class T_CORE_EXPORT TPopMailer : public QObject {
    Q_OBJECT
public:
    TPopMailer(QObject *parent = 0);
    TPopMailer(const QString &hostName, quint16 port, QObject *parent = 0);
    ~TPopMailer();

    QString key() const { return "pop"; }
    QString hostName() const { return popHostName; }
    void setHostName(const QString &hostName);
    quint16 port() const { return popPort; }
    void setPort(quint16 port);
    void setUserName(const QByteArray &username);
    void setPassword(const QByteArray &password);
    void setApopEnabled(bool enable);

    bool connectToHost();
    QByteArray readMail(int index);
    void quit();

protected:
    bool cmdUser();
    bool cmdPass();
    bool cmdApop(const QByteArray &token);
    bool cmdStat();
    bool cmdRetr(int index, QByteArray &message);
    bool cmdQuit();

    bool cmd(const QByteArray &command, QByteArray *reply = 0);
    bool readResponse(QByteArray *reply = 0);
    bool write(const QByteArray &command);

private:
    T_DISABLE_COPY(TPopMailer)
    T_DISABLE_MOVE(TPopMailer)

    QTcpSocket *socket {nullptr};
    QString popHostName;
    quint16 popPort {0};
    QByteArray userName;
    QByteArray password;
    bool apopEnabled {false};
};


inline void TPopMailer::setUserName(const QByteArray &username)
{
    userName = username;
}


inline void TPopMailer::setPassword(const QByteArray &pass)
{
    password = pass;
}

