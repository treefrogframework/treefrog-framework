#ifndef TMULTIPLEXINGRECEIVER_H
#define TMULTIPLEXINGRECEIVER_H

#include <QObject>
#include <QThread>
#include <QByteArray>
#include <TGlobal>


class T_CORE_EXPORT TMultiplexingReceiver : public QThread
{
public:
    TMultiplexingReceiver(QObject *parent = 0);
    ~TMultiplexingReceiver();

    bool setSocketDescriptor(int socket);
    bool isListening() const { return listenSocket > 0; }
    void run();

protected:
    virtual void incomingConnection(int) { }
    virtual void incomingRequest(int, const QByteArray &) { }

private:
    int listenSocket;
};

#endif // TMULTIPLEXINGRECEIVER_H
