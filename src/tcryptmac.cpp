/* Copyright (c) 2011-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QCryptographicHash>
#include <QMap>
#include <QScopedPointer>  // fix compilation error in Qt5.0
#if QT_VERSION >= 0x050100
# include <QMessageAuthenticationCode>
#endif
#include <TCryptMac>


#if QT_VERSION < 0x050100
class BlockSizeHash : public QMap<int, int>
{
public:
    BlockSizeHash() : QMap<int, int>()
    {
        insert(TCryptMac::Hmac_Md5, 64);
        insert(TCryptMac::Hmac_Sha1, 64);
        insert(TCryptMac::Hmac_Sha256, 64);
        insert(TCryptMac::Hmac_Sha384, 128);
        insert(TCryptMac::Hmac_Sha512, 128);
    }
};
Q_GLOBAL_STATIC(BlockSizeHash, blockSizeHash)
#endif


/*!
  \class TCryptMac
  \brief The TCryptMac class provides the functionality of a
  "Message Authentication Code" (MAC) algorithm.
*/

/*!
  Returns a cryptographic hash value generated from the given binary or
  text data \a data with \a key using \a method.
*/
QByteArray TCryptMac::hash(const QByteArray &data, const QByteArray &key, Algorithm method)
{
#if QT_VERSION < 0x050100
    int blockSize = blockSizeHash()->value(method);
    QByteArray tk = (key.length() > blockSize) ? QCryptographicHash::hash(key, (QCryptographicHash::Algorithm)method) : key;
    QByteArray k_ipad(blockSize, '\0');
    k_ipad.replace(0, tk.length(), tk);
    QByteArray k_opad = k_ipad;

    // XOR key with ipad and opad values
    for (int i = 0; i < blockSize; ++i) {
        k_ipad[i] = k_ipad[i] ^ 0x36;
        k_opad[i] = k_opad[i] ^ 0x5c;
    }

    k_ipad.append(data);
    QByteArray hash = QCryptographicHash::hash(k_ipad, (QCryptographicHash::Algorithm)method);
    k_opad.append(hash);
    return QCryptographicHash::hash(k_opad, (QCryptographicHash::Algorithm)method);

#else
    return QMessageAuthenticationCode::hash(data, key, (QCryptographicHash::Algorithm)method);
#endif
}
