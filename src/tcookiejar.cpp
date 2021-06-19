/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TCookieJar>

#ifdef Q_CC_MSVC
uint qHash(const TCookie &key)
{
    //calculate hash here
    return reinterpret_cast<quintptr>(&key);
}
#endif

/*!
  \class TCookieJar
  \brief The TCookieJar class holds network cookies.
  \sa TCookie
*/

/*!
  Copy constructor.
*/
TCookieJar::TCookieJar(const TCookieJar &jar) :
    QList<TCookie>(*static_cast<const QList<TCookie> *>(&jar))
{
}


/*!
  Adds the cookie \a cookie to the cookie jar.
*/
void TCookieJar::addCookie(const TCookie &cookie)
{
    for (QMutableListIterator<TCookie> it(*this); it.hasNext();) {
        if (it.next().name() == cookie.name()) {
            it.remove();
            break;
        }
    }
    append(cookie);
}


/*!
  \fn TCookieJar::TCookieJar()
  Constructor.
*/

/*!
  \fn TCookieJar &TCookieJar::operator=(const TCookieJar &jar)
  Assigns other to this cookie jar and returns a reference to this cookie jar.
*/

/*!
  \fn QList<TCookie> TCookieJar::allCookies() const
  Returns a list of all cookies in the cookie jar.
*/
