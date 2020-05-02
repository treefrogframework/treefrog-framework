/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QMessageAuthenticationCode>
#include <TCryptMac>

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
    return QMessageAuthenticationCode::hash(data, key, (QCryptographicHash::Algorithm)method);
}
