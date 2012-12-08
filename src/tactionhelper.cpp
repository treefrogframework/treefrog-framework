/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TActionHelper>
#include <TActionController>
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
                        const QStringList &args,
                        const QVariantHash &query) const
{
    T_TRACEFUNC("%s : %s", qPrintable(controller), qPrintable(action));
    Q_ASSERT(this->controller());
    QString path;
    QString ctrl = (controller.isEmpty()) ? this->controller()->name() : controller;
    QString act = (action.isEmpty()) ? this->controller()->activeAction() : action;
    path.append('/').append(ctrl).append('/').append(act);
    
    for (QStringListIterator i(args); i.hasNext(); ) {
        path.append('/').append(THttpUtility::toUrlEncoding(i.next()));
    }

    // appends query items
    QString querystr;
    for (QHashIterator<QString, QVariant> it(query); it.hasNext(); ) {
        it.next();
        if (!it.key().isEmpty()) {
            querystr += it.key();
            querystr += QLatin1Char('=');
            querystr += it.value().toString();
            querystr += QLatin1Char('&');
        }
    }
    querystr.chop(1);

    if (!querystr.isEmpty()) {
        path += QLatin1Char('?');
        path += querystr;
    }
    return QUrl(path);
}

/*!
  This function overloads url(const QString &, const QString &, const QStringList &) const.
*/
QUrl TActionHelper::url(const QString &controller, const QString &action, const QVariant &arg) const
{
    if (arg.canConvert(QVariant::StringList)) {
        return url(controller, action, arg.toStringList());
    } else {
        return url(controller, action, arg.toString());
    }
}

/*!
  This function overloads url(const QString &, const QString &, const QStringList &) const.
*/
QUrl TActionHelper::urla(const QString &action, const QVariant &arg) const
{
    if (arg.canConvert(QVariant::StringList)) {
        return urla(action, arg.toStringList());
    } else {
        return urla(action, arg.toString());
    }  
}

/*!
  \fn QUrl TActionHelper::url(const QString &controller, const QString &action, int arg) const
  This function overloads url(const QString &, const QString &, const QStringList &) const.
*/

/*!
  \fn TActionHelper::url(const QString &controller, const QString &action, uint arg) const
  This function overloads url(const QString &, const QString &, const QStringList &) const.
*/

/*!
  \fn TActionHelper::url(const QString &controller, const QString &action, qint64 arg) const
  This function overloads url(const QString &, const QString &, const QStringList &) const.
*/

/*!
  \fn TActionHelper::url(const QString &controller, const QString &action, quint64 arg) const
  This function overloads url(const QString &, const QString &, const QStringList &) const.
*/

/*!
  \fn TActionHelper::url(const QString &controller, const QString &action, const QString &arg) const
  This function overloads url(const QString &, const QString &, const QStringList &) const.
*/

/*!
  \fn TActionHelper::urla(const QString &action=QString(), const QStringList &args=QStringList(), const QVariantHash &query = QVariantHash()) const
  Returns a QUrl to \a action of the current controller with arguments \a args.
  The current action name is used, if the \a action is a empty string.
  If \a query is not empty, sets the query string to an encoded version
  of \a query. 
 */

/*!
  \fn TActionHelper::urla(const QString &action, const QString &arg) const
  This function overloads urla(const QString &, const QStringList &) const.
*/

/*!
  \fn TActionHelper::urla(const QString &action, int arg) const
  This function overloads urla(const QString &, const QStringList &) const.
*/

/*!
  \fn TActionHelper::urla(const QString &action, uint arg) const
  This function overloads urla(const QString &, const QStringList &) const.
*/

/*!
  \fn TActionHelper::urla(const QString &action, qint64 arg) const
  This function overloads urla(const QString &, const QStringList &) const.
*/

/*!
  \fn TActionHelper::urla(const QString &action, quint64 arg) const
  This function overloads urla(const QString &, const QStringList &) const.
*/

