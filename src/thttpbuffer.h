#ifndef THTTPBUFFER_H
#define THTTPBUFFER_H

#include <QByteArray>
#include <QHostAddress>
#include <TGlobal>


class T_CORE_EXPORT THttpBuffer
{
public:
    THttpBuffer();
    ~THttpBuffer();
    THttpBuffer(const THttpBuffer &other);
    THttpBuffer &operator=(const THttpBuffer &other);

    QByteArray read(int maxSize);
    int read(char *data, int maxSize);
    int write(const char *data, int maxSize);
    int write(const QByteArray &byteArray);
    bool canReadHttpRequest() const;
    void clear();
    QByteArray &buffer() { return httpBuffer; }
    const QByteArray &buffer() const { return httpBuffer; }
    const QHostAddress &clientAddress() const { return clientAddr; }
    void setClientAddress(const QHostAddress &address) { clientAddr = address; }

private:
    void parse();

    QByteArray httpBuffer;
    qint64 lengthToRead;
    QHostAddress clientAddr;
};

#endif // THTTPBUFFER_H
