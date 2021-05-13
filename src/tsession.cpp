/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsystemglobal.h"
#include <TActionController>
#include <TAppSettings>
#include <TSession>

/*!
  \class TSession
  \brief The TSession class holds information associated with individual
         visitors.
         This class inherits QVariantMap class.
  \sa http://doc.qt.io/qt-5/qvariant.html#QVariantMap-typedef
*/


/*!
  Resets the session.
 */
void TSession::reset()
{
    QVariantMap::clear();
    // Agsinst CSRF
    TActionController::setCsrfProtectionInto(*this);
}


void TSession::clear()
{
    tSystemWarn("TSession::clear()  disabled");
}

/*!
  Returns the session name specified by the \a application.ini file.
 */
QByteArray TSession::sessionName()
{
    return Tf::appSettings()->value(Tf::SessionName).toByteArray();
}


/*!
  \fn TSession::TSession(const QByteArray &id = QByteArray())
  Constructs a empty session with the ID \a id.
*/

/*!
  \fn TSession::TSession(const TSession &other)
  Copy constructor.
*/

/*!
  \fn QByteArray TSession::id() const
  Returns the ID.
*/

/*!
  \fn iterator TSession::insert(const Key &key, const T &value)
  Inserts a new item with the \a key and a value of \a value.
  If there is already an item with the \a key, that item's value is
  replaced with \a value.
*/

/*!
  \fn const QVariant TSession::value(const QString &key) const
  Returns the value associated with the \a key.
*/

/*!
  \fn TSession::value(const QString &key, const QVariant &defaultValue) const
  This is an overloaded function.
  If the session contains no item with the given \a key, the function
  returns \a defaultValue.
*/
