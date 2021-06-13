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
        tSystemWarn("An invalid QVariant object for exportVariant(), name:%s", qUtf8Printable(name));
        return;
    }

    if (overwrite || !exportVars.contains(name)) {
        exportVars.insert(name, value);
    }
}

/*!
  Exports the \a map.
  Internal use only.
*/
void TAbstractController::exportVariants(const QVariantMap &map)
{
    if (exportVars.isEmpty()) {
        exportVars = map;
    } else {
#if QT_VERSION >= 0x050f00  // 5.15.0
        auto tmp = exportVars;
        exportVars = map;
        exportVars.insert(tmp);
#else
        exportVars.unite(map);
#endif
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
