/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TSession>
#include <TWebApplication>
#include <TActionController>

/*!
  \class TSession
  \brief The TSession class holds information associated with individual
         visitors.

         This class inherits QHash<QString, QVariant> class.
         Use \a QHash::insert() to add data to the session.
  \sa http://qt-project.org/doc/qt-4.8/qvariant.html#QVariantHash-typedef
*/


/*!
  Resets the session.
 */
void TSession::reset()
{
    QVariantHash::clear();
    // Agsinst CSRF
    TActionController::setCsrfProtectionInto(*this);
}


/*!
  Returns the session name specified by the \a application.ini file.
 */
QByteArray TSession::sessionName()
{
    return Tf::app()->appSettings().value("Session.Name").toByteArray();
}
