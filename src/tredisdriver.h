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

    QString key() const { return "REDIS"; }
    bool open(const QString &db, const QString &user = QString(), const QString &password = QString(), const QString &host = QString(), quint16 port = 0, const QString & options = QString());
    void close();
    bool isOpen() const;
    void moveToThread(QThread *thread);
    bool request(const QList<QByteArray> &command, QVariantList &response);

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
    bool waitForState(int state, int msecs);

    static QByteArray toBulk(const QByteArray &data);
    static QByteArray toMultiBulk(const QList<QByteArray> &data);

private:
    QTcpSocket *client;
    QByteArray buffer;
    int pos;

    T_DISABLE_COPY(TRedisDriver)
    T_DISABLE_MOVE(TRedisDriver)
};

#endif // TREDISDRIVER_H
