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


class T_CORE_EXPORT TRedisDriver : public TKvsDriver {
public:
    TRedisDriver();
    ~TRedisDriver();

    QString key() const override { return "REDIS"; }
    bool open(const QString &db, const QString &user = QString(), const QString &password = QString(), const QString &host = QString(), uint16_t port = 0, const QString &options = QString()) override;
    void close() override;
    bool command(const QByteArray &cmd) override;
    bool isOpen() const override;
    void moveToThread(QThread *thread) override;
    bool request(const QByteArrayList &command, QVariantList &response);

protected:
    enum DataType {
        SimpleString = '+',
        Error = '-',
        Integer = ':',
        BulkString = '$',
        Array = '*',
    };

    bool writeCommand(const QByteArray &command);
    bool readReply();
    QByteArray parseBulkString(bool *ok);
    QVariantList parseArray(bool *ok);
    QByteArray getLine(bool *ok);
    int getNumber(bool *ok);
    void clearBuffer();

    static QByteArray toBulk(const QByteArray &data);
    static QByteArray toMultiBulk(const QByteArrayList &data);

private:
#ifdef Q_OS_LINUX
    TTcpSocket *_client {nullptr};
#else
    QTcpSocket *_client {nullptr};
#endif
    QByteArray _buffer;
    int _pos {0};
    QString _host;
    uint16_t _port {0};

    T_DISABLE_COPY(TRedisDriver)
    T_DISABLE_MOVE(TRedisDriver)
};

