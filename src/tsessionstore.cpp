/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TAppSettings>
#include <TSessionStore>


qint64 TSessionStore::lifeTimeSecs()
{
    static qint64 lifetime = []() {
        qint64 time = Tf::appSettings()->value(Tf::SessionGcMaxLifeTime).toLongLong();
        return time;
    }();
    return lifetime;
}


/*!
  \class TSessionStore
  \brief The TSessionStore is an abstract class that stores HTTP sessions.
*/

/*!
  \fn virtual QString TSessionStore::key() const
  Returns the key i.e.\ the name of the sesseion store.
  This function should be called from any reimplementations of key().
*/

/*!
  \fn virtual TSession TSessionStore::find(const QByteArray &id, const QDateTime &expiration)
  Returns the session which has the ID \a id and is newer than or equal
  to the \a expiration datetime in the session store. If the store
  contains no such session, the function returns a empty session.
  This function should be called from any reimplementations of find().
*/

/*!
  \fn virtual bool TSessionStore::store(TSession &sesion)
  Stores the \a session in the session store.
  This function should be called from any reimplementations of store().
*/

/*!
  \fn virtual bool TSessionStore::remove(const QDateTime &expiration)
  Removes all sessions older than the \a expiration datetime.
  This function should be called from any reimplementations of remove().
*/

/*!
  \fn virtual bool TSessionStore::remove(const QByteArray &id)
  Removes the session with the ID \a id.
  This function should be called from any reimplementations of remove().
*/
