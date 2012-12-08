#ifndef TCRYPTMAC_H
#define TCRYPTMAC_H

#include <QByteArray>
#include <QCryptographicHash>
#include <TGlobal>


class T_CORE_EXPORT TCryptMac
{
public:
    enum Algorithm {
        Hmac_Md5  = QCryptographicHash::Md5,
        Hmac_Sha1 = QCryptographicHash::Sha1,
    };
    
    static QByteArray mac(const QByteArray &data, const QByteArray &key, Algorithm method);
};

#endif // TCRYPTMAC_H
