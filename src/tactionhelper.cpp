/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "turlroute.h"
#include <TActionController>
#include <TActionHelper>
#include <THttpUtility>
#include <TSystemGlobal>

/*!
  \class TActionHelper
  \brief The TActionHelper class is the base class of all helpers.
*/


/*!
  Returns a QUrl to \a action of \a controller with arguments \a args.
  The current controller name is used, if the \a controller is a empty string.
  The current action name is used, if the \a action is a empty string.
  If \a query is not empty, sets the query string to an encoded version
  of \a query.
*/
QUrl TActionHelper::url(const QString &controller, const QString &action,
    const QStringList &args, const QVariantMap &query) const
{
    QString querystr;
    for (auto it = query.begin(); it != query.end(); ++it) {
        if (!it.key().isEmpty()) {
            querystr += it.key();
            querystr += QLatin1Char('=');
            querystr += THttpUtility::toUrlEncoding(it.value().toString());
            querystr += QLatin1Char('&');
        }
    }
    querystr.chop(1);

    return url(controller, action, args, querystr);
}

/*!
  Returns a QUrl to \a action of \a controller with arguments \a args.
  The current controller name is used, if the \a controller is a empty string.
  The current action name is used, if the \a action is a empty string.
  If \a query is not empty, sets the query string to \a query.
*/
QUrl TActionHelper::url(const QString &controller, const QString &action,
    const QStringList &arguments, const QString &query) const
{
    Q_ASSERT(this->controller());
    const QString ctrl = (controller.isEmpty()) ? this->controller()->name() : controller;
    const QString act = (action.isEmpty()) ? this->controller()->activeAction() : action;
    const QStringList args = (controller.isEmpty() && action.isEmpty() && arguments.isEmpty()) ? this->controller()->arguments() : arguments;
    QString path = TUrlRoute::instance().findUrl(ctrl, act, args);

    if (path.isEmpty()) {
        path.append('/').append(ctrl).append('/').append(act);

        for (auto &a : args) {
            path.append('/').append(a);
        }
    }

    // appends query items
    if (!query.isEmpty()) {
        if (!query.startsWith('?')) {
            path += QLatin1Char('?');
        }
        path += query;
    }

    return QUrl(path);
}

/*!
  This function overloads url(const QString &, const QString &, const QStringList &, const QVariantMap &) const.
*/
QUrl TActionHelper::url(const QString &controller, const QString &action, const QVariant &arg) const
{
    if (arg.canConvert<QStringList>()) {
        return url(controller, action, arg.toStringList());
    } else {
        return url(controller, action, arg.toString());
    }
}

/*!
  This function overloads url(const QString &, const QString &, const QStringList &, const QVariantMap &) const.
*/
QUrl TActionHelper::urla(const QString &action, const QVariant &arg) const
{
    if (arg.canConvert<QStringList>()) {
        return urla(action, arg.toStringList());
    } else {
        return urla(action, arg.toString());
    }
}

/*!
  \fn QUrl TActionHelper::url(const QString &controller, const QString &action, int arg) const
  This function overloads url(const QString &, const QString &, const QStringList &, const QVariantMap &) const.
*/

/*!
  \fn TActionHelper::url(const QString &controller, const QString &action, uint arg) const
  This function overloads url(const QString &, const QString &, const QStringList &, const QVariantMap &) const.
*/

/*!
  \fn TActionHelper::url(const QString &controller, const QString &action, qint64 arg) const
  This function overloads url(const QString &, const QString &, const QStringList &, const QVariantMap &) const.
*/

/*!
  \fn TActionHelper::url(const QString &controller, const QString &action, quint64 arg) const
  This function overloads url(const QString &, const QString &, const QStringList &, const QVariantMap &) const.
*/

/*!
  \fn TActionHelper::url(const QString &controller, const QString &action, const QString &arg) const
  This function overloads url(const QString &, const QString &, const QStringList &, const QVariantMap &) const.
*/

/*!
  \fn TActionHelper::url(const QString &controller, const QString &action, const QString &arg, const QVariantMap &) const
  This function overloads url(const QString &, const QString &, const QStringList &, const QVariantMap &) const.
*/

/*!
  \fn TActionHelper::urla(const QString &action=QString(), const QStringList &args=QStringList(), const QVariantMap &query = QVariantMap()) const
  Returns a QUrl to \a action of the current controller with arguments \a args.
  The current action name is used, if the \a action is a empty string.
  If \a query is not empty, sets the query string to an encoded version
  of \a query.
*/

/*!
  \fn TActionHelper::urla(const QString &action, const QStringList &args, const QString &query) const
  Returns a QUrl to \a action of the current controller with arguments \a args.
  The current action name is used, if the \a action is a empty string.
  If \a query is not empty, sets the query string to an encoded version
  of \a query.
 */

/*!
  \fn TActionHelper::urla(const QString &action, const QString &arg) const
  This function overloads urla(const QString &, const QStringList &, const QVariantMap &) const.
*/

/*!
  \fn TActionHelper::urla(const QString &action, int arg) const
  This function overloads urla(const QString &, const QStringList &, const QVariantMap &) const.
*/

/*!
  \fn TActionHelper::urla(const QString &action, uint arg) const
  This function overloads urla(const QString &, const QStringList &, const QVariantMap &) const.
*/

/*!
  \fn TActionHelper::urla(const QString &action, qint64 arg) const
  This function overloads urla(const QString &, const QStringList &, const QVariantMap &) const.
*/

/*!
  \fn TActionHelper::urla(const QString &action, quint64 arg) const
  This function overloads urla(const QString &, const QStringList &, const QVariantMap &) const.
*/

/*!
  \fn TActionHelper::urla(const QString &action, const QVariantMap &query) const
  This function overloads urla(const QString &, const QStringList &, const QVariantMap &) const.
*/

/*!
  \fn TActionHelper::urlq(const QVariantMap &query) const
  This function overloads url(const QString &, const QString &, const QStringList &, const QVariantMap &) const.
  Equivalent to url(QString(), QString(), QStringList(), query).
*/

/*!
  \fn TActionHelper::urlq(const QString &query) const
  This function overloads url(const QString &, const QString &, const QStringList &, const QString &) const.
  Equivalent to url(QString(), QString(), QStringList(), query).
*/
