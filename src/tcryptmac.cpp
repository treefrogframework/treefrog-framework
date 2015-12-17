/* Copyright (c) 2011-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QCryptographicHash>
#include <QHash>
#include <QScopedPointer>  // fix compilation error in Qt5.0
#include <TCryptMac>


class BlockSizeHash : public QHash<int, int>
{
public:
    BlockSizeHash() : QHash<int, int>()
    {
        insert(TCryptMac::Hmac_Md5, 64);
        insert(TCryptMac::Hmac_Sha1, 64);
#if QT_VERSION >= 0x050000
        insert(TCryptMac::Hmac_Sha256, 64);
        insert(TCryptMac::Hmac_Sha384, 128);
        insert(TCryptMac::Hmac_Sha512, 128);
#endif
    }
};
Q_GLOBAL_STATIC(BlockSizeHash, blockSizeHash)


/*!
  \class TCryptMac
  \brief The TCryptMac class provides the functionality of a
  "Message Authentication Code" (MAC) algorithm.
*/

/*!
  Returns a cryptographic hash value generated from the given binary or
  text data \a data with \a key using \a method.
*/
QByteArray TCryptMac::mac(const QByteArray &data, const QByteArray &key, Algorithm method)
{
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
}
