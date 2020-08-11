#pragma once
#include <QByteArray>
#include <QCryptographicHash>
#include <TGlobal>


class T_CORE_EXPORT TCryptMac {
public:
    enum Algorithm {
        Hmac_Md5 = QCryptographicHash::Md5,
        Hmac_Sha1 = QCryptographicHash::Sha1,
        Hmac_Sha256 = QCryptographicHash::Sha256,
        Hmac_Sha384 = QCryptographicHash::Sha384,
        Hmac_Sha512 = QCryptographicHash::Sha512,
        Hmac_Sha3_224 = QCryptographicHash::Sha3_224,
        Hmac_Sha3_256 = QCryptographicHash::Sha3_256,
        Hmac_Sha3_384 = QCryptographicHash::Sha3_384,
        Hmac_Sha3_512 = QCryptographicHash::Sha3_512,
    };

    static QByteArray hash(const QByteArray &data, const QByteArray &key, Algorithm method);
};

