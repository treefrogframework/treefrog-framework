/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsystemglobal.h"
#include <TAbstractController>
#include <TFormValidator>

/*!
  \class TAbstractController
  \brief The TAbstractController class is the abstract base class
  of controllers, providing functionality common to controllers.
*/

/*!
  Constructor.
*/
TAbstractController::TAbstractController() :
    QObject()
{
}

/*!
  Exports a new variable with the name \a name and a value of \a value for views.
  Internal use only.
*/
void TAbstractController::exportVariant(const QString &name, const QVariant &value, bool overwrite)
{
    if (!value.isValid()) {
        tSystemWarn("An invalid QVariant object for exportVariant(), name:{}", name);
        return;
    }

    if (overwrite || !_exportVars.contains(name)) {
        _exportVars.insert(name, value);
    }
}

/*!
  Exports the \a map.
  Internal use only.
*/
void TAbstractController::exportVariants(const QVariantMap &map)
{
    if (_exportVars.isEmpty()) {
        _exportVars = map;
    } else {
        auto tmp = _exportVars;
        _exportVars = map;
        _exportVars.insert(tmp);
    }
}

/*!
  Exports validation error messages with the prefix \a prefix each.
 */
void TAbstractController::exportValidationErrors(const TFormValidator &v, const QString &prefix)
{
    for (auto &key : (const QStringList &)v.validationErrorKeys()) {
        QString msg = v.errorMessage(key);
        exportVariant(prefix + key, QVariant(msg));
    }
}

/*!
  Returns a class name of a view for the action \a action of the
  active controller. Internal use only.
*/
QString TAbstractController::viewClassName(const QString &action) const
{
    return viewClassName("", action);
}

/*!
  Returns a class name of a view for the action \a action of the
  controller \a controller. Internal use only.
*/
QString TAbstractController::viewClassName(const QString &contoller, const QString &action) const
{
    // If action string is empty, active action name.
    // If controller string is empty, active contoller name.
    return (contoller.isEmpty() ? name().toLower() : contoller.toLower()) + '_' + (action.isEmpty() ? activeAction() : action) + QLatin1String("View");
}


void TAbstractController::setFlash(const QString &, const QVariant &)
{
    throw StandardException("Not implemented error", __FILE__, __LINE__);
}


QString TAbstractController::getRenderingData(const QString &, const QVariantMap &)
{
    throw StandardException("Not implemented error", __FILE__, __LINE__);
}


const THttpRequest &TAbstractController::httpRequest() const
{
    throw StandardException("Not implemented error", __FILE__, __LINE__);
}


const THttpRequest &TAbstractController::request() const
{
    throw StandardException("Not implemented error", __FILE__, __LINE__);
}


const TSession &TAbstractController::session() const
{
    throw StandardException("Not implemented error", __FILE__, __LINE__);
}


TSession &TAbstractController::session()
{
    throw StandardException("Not implemented error", __FILE__, __LINE__);
}


bool TAbstractController::addCookie(const TCookie &)
{
    throw StandardException("Not implemented error", __FILE__, __LINE__);
}


bool TAbstractController::addCookie(const QByteArray &, const QByteArray &, const QDateTime &, const QString &, const QString &, bool, bool, const QByteArray &)
{
    throw StandardException("Not implemented error", __FILE__, __LINE__);
}


bool TAbstractController::addCookie(const QByteArray &, const QByteArray &, int64_t , const QString &, const QString &, bool, bool, const QByteArray &)
{
    throw StandardException("Not implemented error", __FILE__, __LINE__);
}


/*!
  Returns true if a user is logged in to the system; otherwise returns false.
  This is a virtual function.
  \sa userLogin()
*/
bool TAbstractController::isUserLoggedIn() const
{
    throw StandardException("Not implemented error", __FILE__, __LINE__);
}

/*!
  \fn const QVariantMap &TAbstractController::allVariants() const

  Returns all the exported variables. Internal use only.
*/

/*!
  \fn virtual QString TAbstractController::name() const

  This function is reimplemented in subclasses to return a
  controller name.
*/

/*!
  \fn virtual QString TAbstractController::activeAction() const

  This function is reimplemented in subclasses to return a active
  action name.
*/

/*!
  \fn QVariant TAbstractController::variant(const QString &name) const

  Returns the exported variable with the value associated with the \a name.
*/

/*!
  \fn bool TAbstractController::hasVariant(const QString &name) const

  Returns true if a variable with the \a name is exported for views; otherwise returns false.
  Internal use only.
*/
