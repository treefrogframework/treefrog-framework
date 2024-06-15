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
    TPopMailer(const QString &hostName, uint16_t port, QObject *parent = 0);
    ~TPopMailer();

    QString key() const { return "pop"; }
    QString hostName() const { return _popHostName; }
    void setHostName(const QString &hostName);
    uint16_t port() const { return _popPort; }
    void setPort(uint16_t port);
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

    QTcpSocket *_socket {nullptr};
    QString _popHostName;
    uint16_t _popPort {0};
    QByteArray _userName;
    QByteArray _password;
    bool _apopEnabled {false};
};


inline void TPopMailer::setUserName(const QByteArray &username)
{
    _userName = username;
}


inline void TPopMailer::setPassword(const QByteArray &pass)
{
    _password = pass;
}
