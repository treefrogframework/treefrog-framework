#pragma once
#include <QString>
#include <QVariant>
#include <QtGlobal>
#include <TGlobal>
#include <TKvsDriver>

class QTcpSocket;


class T_CORE_EXPORT TRedisDriver : public TKvsDriver {
public:
    TRedisDriver();
    ~TRedisDriver();

    QString key() const override { return "REDIS"; }
    bool open(const QString &db, const QString &user = QString(), const QString &password = QString(), const QString &host = QString(), quint16 port = 0, const QString &options = QString()) override;
    void close() override;
    bool command(const QString &cmd) override;
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
#ifdef Q_OS_UNIX
    int _socket {0};
#else
    QTcpSocket *_client {nullptr};
#endif
    QByteArray _buffer;
    int _pos {0};
    QString _host;
    quint16 _port {0};

    T_DISABLE_COPY(TRedisDriver)
    T_DISABLE_MOVE(TRedisDriver)
};

