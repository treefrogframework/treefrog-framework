#ifndef TSMTPMAILER_H
#define TSMTPMAILER_H

#include <QObject>
#include <QMutex>
#include <QStringList>
#include <QByteArray>
#include <TMailMessage>

class QSslSocket;
class TPopMailer;

class T_CORE_EXPORT TSmtpMailer : public QObject
{
    Q_OBJECT
public:
    enum AuthenticationType {
        None = 0,
        CRAM_MD5,
        Login,
        Plain,
    };

    TSmtpMailer(QObject *parent = nullptr);
    TSmtpMailer(const QString &hostName, quint16 port, QObject *parent = nullptr);
    ~TSmtpMailer();

    QString key() const { return "smtp"; }
    QString hostName() const { return smtpHostName; }
    void setHostName(const QString &hostName);
    quint16 port() const { return smtpPort; }
    void setPort(quint16 port);
    void setAuthenticationEnabled(bool enable);
    void setStartTlsEnabled(bool enable);
    void setPopBeforeSmtpAuthEnabled(const QString &popServer, quint16 port, bool apop, bool enable);
    void setUserName(const QByteArray &username);
    void setPassword(const QByteArray &password);
    QString lastServerResponse() const;

    void moveToThread(QThread *targetThread);
    bool send(const TMailMessage &message);
    void sendLater(const TMailMessage &message);

    static QByteArray authCramMd5(const QByteArray &in, const QByteArray &username, const QByteArray &password);

protected slots:
    void sendAndDeleteLater();

protected:
    bool send();
    bool connectToHost(const QString &hostName, quint16 port);
    bool cmdEhlo();
    bool cmdHelo();
    bool cmdStartTls();
    bool cmdAuth();
    bool cmdRset();
    bool cmdMail(const QByteArray &from);
    bool cmdRcpt(const QList<QByteArray> &to);
    bool cmdData(const QByteArray &message);
    bool cmdQuit();

    int  cmd(const QByteArray &command, QList<QByteArray> *reply = nullptr);
    int  read(QList<QByteArray> *reply = nullptr);
    bool write(const QByteArray &command);

private:
    T_DISABLE_COPY(TSmtpMailer)
    T_DISABLE_MOVE(TSmtpMailer)

    QSslSocket *socket {nullptr};
    QMutex sendMutex;
    QString smtpHostName;
    quint16 smtpPort {0};
    TMailMessage mailMessage;
    QStringList  svrAuthMethods;
    bool authEnable {false};
    bool tlsEnable {false};
    bool tlsAvailable {false};
    QByteArray userName;
    QByteArray password;
    TPopMailer *pop {nullptr};
    QByteArray lastResponse;
};


inline void TSmtpMailer::setAuthenticationEnabled(bool enable)
{
    authEnable = enable;
}


inline void TSmtpMailer::setStartTlsEnabled(bool enable)
{
    tlsEnable = enable;
}


inline void TSmtpMailer::setUserName(const QByteArray &username)
{
    userName = username;
}


inline void TSmtpMailer::setPassword(const QByteArray &pass)
{
    password = pass;
}

#endif // TSMTPMAILER_H
