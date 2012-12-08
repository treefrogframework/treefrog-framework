#ifndef TCONTENTHEADER_H
#define TCONTENTHEADER_H

#include <THttpHeader>


class T_CORE_EXPORT TContentHeader : public THttpHeader
{
public:
    TContentHeader();
    TContentHeader(const TContentHeader &header);
    TContentHeader(const QByteArray &str);
    virtual ~TContentHeader() { }
    
    TContentHeader &operator=(const TContentHeader &h);
    
private:
    int majorVersion() const { return 0; }
    int minorVersion() const { return 0; }
};


Q_DECLARE_METATYPE(TContentHeader)

#endif // TCONTENTHEADER_H
