#ifndef TREDISDRIVER_H
#define TREDISDRIVER_H

#include <QString>
#include <QVariant>
#include <TGlobal>
#include <TKvsDriver>

class QTcpSocket;


class T_CORE_EXPORT TRedisDriver : public TKvsDriver
{
public:
    TRedisDriver();
    ~TRedisDriver();

    QString key() const override { return "REDIS"; }
    bool open(const QString &db, const QString &user = QString(), const QString &password = QString(), const QString &host = QString(), quint16 port = 0, const QString & options = QString()) override;
    void close() override;
    bool command(const QString &cmd) override;
    bool isOpen() const override;
    void moveToThread(QThread *thread) override;
    bool request(const QByteArrayList &command, QVariantList &response);

protected:
    enum DataType {
        SimpleString = '+',
        Error        = '-',
        Integer      = ':',
        BulkString   = '$',
        Array        = '*',
    };

    bool readReply();
    QByteArray parseBulkString(bool *ok);
    QVariantList parseArray(bool *ok);
    QByteArray getLine(bool *ok);
    int getNumber(bool *ok);
    void clearBuffer();
//    bool waitForState(int state, int msecs);

    static QByteArray toBulk(const QByteArray &data);
    static QByteArray toMultiBulk(const QByteArrayList &data);

private:
    bool connectToRedisServer();

    QTcpSocket *_client {nullptr};
    QByteArray _buffer;
    int _pos {0};
    QString _host;
    quint16 _port {0};

    T_DISABLE_COPY(TRedisDriver)
    T_DISABLE_MOVE(TRedisDriver)
};

#endif // TREDISDRIVER_H
