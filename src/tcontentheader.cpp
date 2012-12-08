/* Copyright (c) 2010-2012, AOYAMA Kazuharu
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
TContentHeader::TContentHeader() : THttpHeader()
{ }

/*!
  Copy constructor.
*/
TContentHeader::TContentHeader(const TContentHeader &header)
    : THttpHeader(*static_cast<const THttpHeader *>(&header))
{ }

/*!
  Constructor.
*/
TContentHeader::TContentHeader(const QByteArray &str)
    : THttpHeader(str)
{ }

/*!
  Assigns other to this content header and returns a reference
  to this content header.
*/
TContentHeader &TContentHeader::operator=(const TContentHeader &h)
{
    THttpHeader::operator=(*static_cast<const THttpHeader *>(&h));
    return *this;
}
