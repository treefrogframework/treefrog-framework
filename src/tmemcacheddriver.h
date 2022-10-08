#pragma once
#include <QString>
#include <QVariant>
#include <QtGlobal>
#include <TGlobal>
#include <TKvsDriver>

#ifdef Q_OS_UNIX
class TTcpSocket;
#else
class QTcpSocket;
#endif


class T_CORE_EXPORT TMemcachedDriver : public TKvsDriver {
public:
    TMemcachedDriver();
    ~TMemcachedDriver();

    QString key() const override { return "MEMCACHED"; }
    bool open(const QString &db, const QString &user = QString(), const QString &password = QString(), const QString &host = QString(), quint16 port = 0, const QString &options = QString()) override;
    void close() override;
    bool command(const QByteArray &cmd) override;
    bool isOpen() const override;
    void moveToThread(QThread *thread) override;
    bool request(const QByteArrayList &command, QVariantList &response);

protected:
    bool writeCommand(const QByteArray &command);
    bool readReply();
    //QByteArray parseBulkString(bool *ok);
    //QVariantList parseArray(bool *ok);
    QByteArray getLine(bool *ok);
    //int getNumber(bool *ok);
    void clearBuffer();

private:
#ifdef Q_OS_LINUX
    TTcpSocket *_client {nullptr};
#else
    QTcpSocket *_client {nullptr};
#endif
    QByteArray _buffer;
    int _pos {0};
    QString _host;
    quint16 _port {0};

    T_DISABLE_COPY(TMemcachedDriver)
    T_DISABLE_MOVE(TMemcachedDriver)
};

