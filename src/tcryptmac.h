#ifndef TCRYPTMAC_H
#define TCRYPTMAC_H

#include <QByteArray>
#include <QCryptographicHash>
#include <TGlobal>


class T_CORE_EXPORT TCryptMac
{
public:
    enum Algorithm {
        Hmac_Md5  = QCryptographicHash::Md5,        //< 128bit hash, MD5 is not suitable for applications like SSL
        Hmac_Sha1 = QCryptographicHash::Sha1,       //< 160bit hash, SHA-1 is known to have some weaknesses
#if QT_VERSION >= 0x050100
        Hmac_Sha3 = QCryptographicHash::Sha3_256,   //< 256bit hash, new secure hash algorithm from NIST
        Hmac_Sha3_512 = QCryptographicHash::Sha3_512    //< 512bit hash, new secure hash algorithm from NIST
#endif
    };
    
    static QByteArray mac(const QByteArray &data, const QByteArray &key, Algorithm method);
    static QByteArray macEx(const QByteArray &data, const QByteArray &key, Algorithm method);
};

#endif // TCRYPTMAC_H
