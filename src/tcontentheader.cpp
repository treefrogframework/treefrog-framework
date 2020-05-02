/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TContentHeader>

/*!
  \class TContentHeader
  \brief The TContentHeader class contains content header information
  for HTTP.
*/

/*!
  Constructor.
*/
TContentHeader::TContentHeader() :
    TInternetMessageHeader()
{
}

/*!
  Copy constructor.
*/
TContentHeader::TContentHeader(const TContentHeader &other) :
    TInternetMessageHeader(*static_cast<const TInternetMessageHeader *>(&other))
{
}

/*!
  Constructor.
*/
TContentHeader::TContentHeader(const QByteArray &str) :
    TInternetMessageHeader(str)
{
}

/*!
  Assigns \a other to this content header and returns a reference
  to this content header.
*/
TContentHeader &TContentHeader::operator=(const TContentHeader &other)
{
    TInternetMessageHeader::operator=(*static_cast<const TInternetMessageHeader *>(&other));
    return *this;
}
