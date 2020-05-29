#ifndef TSMTPMAILER_H
#define TSMTPMAILER_H

#include <QByteArray>
#include <QMutex>
#include <QObject>
#include <QStringList>
#include <TMailMessage>

class QSslSocket;
class TPopMailer;

class T_CORE_EXPORT TSmtpMailer : public QObject {
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
    QString hostName() const { return _smtpHostName; }
    void setHostName(const QString &hostName);
    quint16 port() const { return _smtpPort; }
    void setPort(quint16 port);
    void setAuthenticationEnabled(bool enable);
    void setSslEnabled(bool enable);
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
    bool cmdRcpt(const QByteArrayList &to);
    bool cmdData(const QByteArray &message);
    bool cmdQuit();

    int cmd(const QByteArray &command, QByteArrayList *reply = nullptr);
    int read(QByteArrayList *reply = nullptr);
    bool write(const QByteArray &command);

private:
    T_DISABLE_COPY(TSmtpMailer)
    T_DISABLE_MOVE(TSmtpMailer)

    QSslSocket *_socket {nullptr};
    QMutex _sendMutex;
    QString _smtpHostName;
    quint16 _smtpPort {0};
    TMailMessage _mailMessage;
    QStringList _svrAuthMethods;
    bool _authEnabled {false};
    bool _sslEnabled {false};
    bool _startTlsEnabled {false};
    bool _startTlsAvailable {false};
    QByteArray _username;
    QByteArray _password;
    TPopMailer *_pop {nullptr};
    QByteArray _lastResponse;
};


inline void TSmtpMailer::setAuthenticationEnabled(bool enable)
{
    _authEnabled = enable;
}


inline void TSmtpMailer::setSslEnabled(bool enable)
{
    _sslEnabled = enable;
}


inline void TSmtpMailer::setStartTlsEnabled(bool enable)
{
    _startTlsEnabled = enable;
}


inline void TSmtpMailer::setUserName(const QByteArray &username)
{
    _username = username;
}


inline void TSmtpMailer::setPassword(const QByteArray &pass)
{
    _password = pass;
}


inline void TSmtpMailer::setHostName(const QString &hostName)
{
    _smtpHostName = hostName;
}


inline void TSmtpMailer::setPort(quint16 port)
{
    _smtpPort = port;
}

#endif  // TSMTPMAILER_H
