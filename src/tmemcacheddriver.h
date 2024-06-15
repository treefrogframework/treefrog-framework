#pragma once
#include <QString>
#include <QVariant>
#include <QtGlobal>
#include <TGlobal>
#include <TKvsDriver>

#ifdef Q_OS_LINUX
class TTcpSocket;
#else
class QTcpSocket;
#endif


class T_CORE_EXPORT TMemcachedDriver : public TKvsDriver {
public:
    TMemcachedDriver();
    ~TMemcachedDriver();

    QString key() const override { return "MEMCACHED"; }
    bool open(const QString &db, const QString &user = QString(), const QString &password = QString(), const QString &host = QString(), uint16_t port = 0, const QString &options = QString()) override;
    void close() override;
    bool command(const QByteArray &cmd) override;
    bool isOpen() const override;
    void moveToThread(QThread *thread) override;
    QByteArray request(const QByteArray &command, int msecs = 5000);

protected:
    bool writeCommand(const QByteArray &command);
    QByteArray readReply(int msecs);

private:
#ifdef Q_OS_LINUX
    TTcpSocket *_client {nullptr};
#else
    QTcpSocket *_client {nullptr};
#endif
    QString _host;
    uint16_t _port {0};

    static constexpr int DEFAULT_PORT = 11211;
    T_DISABLE_COPY(TMemcachedDriver)
    T_DISABLE_MOVE(TMemcachedDriver)
};
