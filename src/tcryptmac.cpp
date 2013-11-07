/* Copyright (c) 2011-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <TCryptMac>

/*!
  \class TCryptMac
  \brief The TCryptMac class provides the functionality of a
  "Message Authentication Code" (MAC) algorithm. 
*/

/*!
  Returns a cryptographic hash value generated from the given binary or
  text data \a data with \a key using \a method. Can only be used with MD5 and SHA-1.
*/
QByteArray TCryptMac::mac(const QByteArray &data, const QByteArray &key, Algorithm method)
{
    QByteArray tk = (key.length() > 64) ? QCryptographicHash::hash(key, (QCryptographicHash::Algorithm)method) : key;
    QByteArray k_ipad(64, '\0');
    k_ipad.replace(0, tk.length(), tk);
    QByteArray k_opad = k_ipad;    

    // XOR key with ipad and opad values
    for (int i = 0; i < 64; ++i) {
        k_ipad[i] = k_ipad[i] ^ 0x36;
        k_opad[i] = k_opad[i] ^ 0x5c;
    }

    k_ipad.append(data);
    QByteArray hash = QCryptographicHash::hash(k_ipad, (QCryptographicHash::Algorithm)method);
    k_opad.append(hash);
    return QCryptographicHash::hash(k_opad, (QCryptographicHash::Algorithm)method);
}

/*!
  Returns a cryptographic hash value generated from the given binary or
  text data \a data with \a key using \a method. Can be used with all hashes including SHA-3.
*/
QByteArray TCryptMac::macEx(const QByteArray &data, const QByteArray &key, Algorithm method)
{
#if QT_VERSION >= 0x050100
    QMessageAuthenticationCode code((QCryptographicHash::Algorithm)method);
    code.setKey(key);
    code.addData(data);
    return (code.result().toHex());
#else
    this->mac(data, key, method);
#endif
}

