#pragma once
#include <QString>
#include <QHostAddress>


class TUringTask;


class TUringCoroutine {
public:
    explicit TUringCoroutine(int socketDescriptor);
    virtual ~TUringCoroutine();

    TUringTask start();
    QHostAddress peerAddress() const { return _peer; }

private:
    int _sd {0};
    QHostAddress _peer;
    QByteArray _response;
    QString _fileName;
};
