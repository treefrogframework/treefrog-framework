#ifndef TREDISDRIVER_H
#define TREDISDRIVER_H

#include <QString>
#include <QVariant>
#include <QTcpSocket>
#include <TGlobal>
#include <TKvsDriver>


class T_CORE_EXPORT TRedisDriver : public TKvsDriver, protected QTcpSocket
{
public:
    TRedisDriver();
    ~TRedisDriver();

    QString key() const { return "REDIS"; }
    bool open(const QString &db, const QString &user = QString(), const QString &password = QString(), const QString &host = QString(), quint16 port = 0, const QString & options = QString());
    void close();
    bool isOpen() const;

    bool request(const QList<QByteArray> &command, QVariantList &replay);

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
    QByteArray buffer;
    int pos;

    Q_DISABLE_COPY(TRedisDriver)
};

#endif // TREDISDRIVER_H
