#ifndef THTTPBUFFER_H
#define THTTPBUFFER_H

#include <QByteArray>
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
    int write(const QByteArray &byteArray);
    int write(const char *data, int len);
    bool canReadHttpRequest() const;
    void clear();
    QByteArray &buffer() { return httpBuffer; }
    const QByteArray &buffer() const { return httpBuffer; }

private:
    void parse();

    QByteArray httpBuffer;
    int64_t lengthToRead;
};

#endif // THTTPBUFFER_H
