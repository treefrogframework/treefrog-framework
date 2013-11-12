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
#if QT_VERSION >= 0x050000
        Hmac_Sha256 = QCryptographicHash::Sha256,
        Hmac_Sha384 = QCryptographicHash::Sha384,
        Hmac_Sha512 = QCryptographicHash::Sha512,
#endif
    };

    static QByteArray mac(const QByteArray &data, const QByteArray &key, Algorithm method);
};

#endif // TCRYPTMAC_H
