/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TWebSocketSession>
#include <TSession>

/*!
  \class TWebSocketSession
  \brief The TWebSocketSession class holds information associated with
         individual visitors for WebSocket connection.
         This class inherits QVariantMap class.
  \sa http://doc.qt.io/qt-5/qvariant.html#QVariantMap-typedef
*/


TWebSocketSession &TWebSocketSession::unite(const TSession &session)
{
    QVariantMap::unite(*static_cast<const QVariantMap*>(&session));
    return *this;
}


/*!
  \fn TWebSocketSession::TWebSocketSession()
  Constructor.
*/

/*!
  \fn TWebSocketSession::TWebSocketSession(const TWebSocketSession &);
  Copy constructor.
*/

/*!
  \fn TWebSocketSession::TWebSocketSession &operator=(const TWebSocketSession &);
  Assignment operator.
*/

/*!
   \fn iterator TWebSocketSession::insert(const QString &key, const QVariant &value);
   Inserts a new item with the \a key and a value of \a value.
   If there is already an item with the \a key, that item's value is
   replaced with \a value.
*/

/*!
  \fn const QVariant TWebSocketSession::value(const QString &key) const;
  Returns the value associated with the \a key.
*/

/*!
  \fn const QVariant TWebSocketSession::value(const QString &key, const QVariant &defaultValue) const;
  This is an overloaded function.
  If the session contains no item with the given \a key, the function
  returns \a defaultValue.
*/

/*!
  \fn TWebSocketSession &TWebSocketSession::unite(const TSession &session);
  Inserts all the items in the other session into this session. If a key is
  common to both sessions, the resulting session will contain the key multiple
  times.
*/

/*!
  \fn void \fn TWebSocketSession::reset();
  Resets the session.
*/
