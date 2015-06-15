#ifndef THTTPSOCKET_H
#define THTTPSOCKET_H

#include <QTcpSocket>
#include <QByteArray>
#include <QElapsedTimer>
#include <THttpRequest>
#include <TTemporaryFile>
#include <TGlobal>
#ifdef Q_OS_UNIX
# include "tfcore_unix.h"
#endif


class T_CORE_EXPORT THttpSocket : public QTcpSocket
{
    Q_OBJECT
public:
    THttpSocket(QObject *parent = 0);
    virtual ~THttpSocket();

    QList<THttpRequest> read();
    bool canReadRequest() const;
    qint64 write(const THttpHeader *header, QIODevice *body);
    int idleTime() const { return idleElapsed.elapsed() / 1000; }
    QByteArray socketUuid() const { return uuid; }
    void deleteLater();

#if QT_VERSION >= 0x050000
    bool setSocketDescriptor(qintptr socketDescriptor, SocketState socketState = ConnectedState, OpenMode openMode = ReadWrite);
#else
    bool setSocketDescriptor(int socketDescriptor, SocketState socketState = ConnectedState, OpenMode openMode = ReadWrite);
#endif

    static THttpSocket *searchSocket(const QByteArray &uuid);
    void writeRawDataFromWebSocket(const QByteArray &data);

protected slots:
    void readRequest();
    qint64 writeRawData(const char *data, qint64 size);
    qint64 writeRawData(const QByteArray &data);

signals:
    void newRequest();
    void requestWrite(const QByteArray &data);  // internal use

private:
    Q_DISABLE_COPY(THttpSocket)

    QByteArray uuid;
    qint64 lengthToRead;
    QByteArray readBuffer;
    TTemporaryFile fileBuffer;
    QElapsedTimer idleElapsed;

    friend class TActionThread;
};

#endif // THTTPSOCKET_H
