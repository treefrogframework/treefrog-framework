#pragma once

class TUringTask;


class TUringCoroutine {
public:
    explicit TUringCoroutine(int socketDescriptor) :
        _sd(socketDescriptor) {}
    virtual ~TUringCoroutine();

    TUringTask start();

private:
    int _sd {0};
    QByteArray _response;
    QString _fileName;
};
