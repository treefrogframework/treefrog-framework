/* Copyright (c) 2010-2025, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsystemglobal.h"
#include <QDir>
#include <QMutex>
#include <QMutexLocker>
#include <TActionController>
#include <TActionView>
#include <THtmlAttribute>
#include <THttpUtility>
#include <TReactComponent>
#include <TWebApplication>

/*!
  \class TActionView
  \brief The TActionView class is the abstract base class of views,
  providing functionality common to view.
*/


/*!
  Constructor.
*/
TActionView::TActionView() :
    QObject(),
    TViewHelper()
{
}

/*!
  Returns a content processed by a action.
*/
QString TActionView::yield() const
{
    return (subView) ? subView->toString() : QString();
}

/*!
  Render the partial template given by \a templateName without layout.
*/
QString TActionView::renderPartial(const QString &templateName, const QVariantMap &vars) const
{
    QString temp = templateName;
    if (!temp.contains('/')) {
        temp = QLatin1String("partial/") + temp;
    }
    return (actionController) ? actionController->getRenderingData(temp, vars) : QString();
}

/*!
  Renders the React \a component on the server. Calls ReactDOMServer.renderToString()
  internally.
*/
QString TActionView::renderReact(const QString &component)
{
    QStringList path = {(Tf::app()->publicPath() + "js/components"),
        (Tf::app()->publicPath() + "js")};
    return TReactComponent(component, path).renderToString(component);
}

/*!
  Returns a authenticity token for CSRF protection.
*/
QString TActionView::authenticityToken() const
{
    return (actionController) ? QString::fromLatin1(actionController->authenticityToken().data()) : QString();
}

/*!
  Returns flash variants;
*/
QVariantMap TActionView::flashVariants() const
{
    static QVariantMap dummy;
    return (actionController) ? actionController->flashVariants() : dummy;
}

/*!
  \fn QString TActionView::echo(const THtmlAttribute &attr)
  Outputs the string of the HTML attribute \a attr to a view
  template.
*/

/*!
  \fn QString TActionView::echo(const QVariant &var)
  Outputs the variant variable \a var to a view template.
*/

/*!
  \fn QString TActionView::echo(const QVariantMap &map)
  Outputs the variantmap variable \a map to a view template.
*/

/*!
  \fn QString TActionView::eh(const THtmlAttribute &attr)
  Outputs a escaped string of the HTML attribute \a attr to
  a view template.
*/

/*!
  \fn QString TActionView::eh(const QVariant &var)
  Outputs a escaped string of the variant variable \a var
  to a view template.
*/

/*!
  \fn QString TActionView::eh(const QVariantMap &map)
  Outputs a escaped string of the variantmap variable \a map
  to a view template.
*/


QString TActionView::fromValue(int n, int base)
{
    return QString::number(n, base);
}


QString TActionView::fromValue(long n, int base)
{
    return QString::number(n, base);
}


QString TActionView::fromValue(ulong n, int base)
{
    return QString::number(n, base);
}


QString TActionView::fromValue(qlonglong n, int base)
{
    return QString::number(n, base);
}


QString TActionView::fromValue(qulonglong n, int base)
{
    return QString::number(n, base);
}


QString TActionView::fromValue(double d, char format, int precision)
{
    return QString::number(d, format, precision);
}


QString TActionView::fromValue(const QJsonObject &object)
{
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}


QString TActionView::fromValue(const QJsonArray &array)
{
    return QJsonDocument(array).toJson(QJsonDocument::Compact);
}


QString TActionView::fromValue(const QJsonDocument &doc)
{
    return doc.toJson(QJsonDocument::Compact);
}


QString TActionView::fromValue(const THtmlAttribute &attr)
{
    return attr.toString().trimmed();
}


QString TActionView::fromValue(const QVariant &var)
{
    if (var.userType() == QMetaType::QUrl) {
        return var.toUrl().toString(QUrl::FullyEncoded);
    } else {
        return var.toString();
    }
}


QString TActionView::fromValue(const QVariantMap &map)
{
    return QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
}

/*!
  Returns the requested HTTP message.
*/
const THttpRequest &TActionView::httpRequest() const
{
    return controller()->httpRequest();
}


void TActionView::reset()
{
    TViewHelper::clear();
    responsebody.resize(0);
    actionController = nullptr;
    subView = nullptr;
    variantMap.clear();
}


/*!
  \fn QString TActionView::echo	(const QString &str)
  Outputs the string \a str to a view template.
*/

/*!
  \fn QString TActionView::echo(const char *str)
  Outputs the string \a str to a view template.
*/

/*!
  \fn QString TActionView::echo(const QByteArray &str)
  Outputs the array \a str to a view template.
*/

/*!
  \fn QString TActionView::echo(int n, int base)
  Outputs the number \a n to a view template.
*/

/*!
  \fn QString TActionView::echo(double d, char format, int precision)
  Outputs the number \a d to a view template.
*/

/*!
  \fn QString TActionView::eh(const QString &str)
  Outputs a escaped string of the \a str to a view template.
*/

/*!
  \fn QString TActionView::eh(const char *str)
  Outputs a escaped string of the \a str to a view template.
*/

/*!
  \fn QString TActionView::eh(const QByteArray &str)
  Outputs a escaped array of the \a str to a view template.
*/

/*!
  \fn QString TActionView::eh(int n, int base)
  Outputs the number \a n to a view template.
*/

/*!
  \fn QString TActionView::eh(double d, char format, int precision)
  Outputs the number \a d to a view template.
*/

/*!
  \fn bool TActionView::hasVariant(const QString &name) const
  Returns true if the QVariantMap variable for a view contains
  an item with the \a name; otherwise returns false.
*/

/*!
  \fn virtual QString TActionView::toString()
  This function is reimplemented in subclasses to return a
  string to render a view.
*/

/*!
  \fn QVariant TActionView::variant(const QString &name) const
  Returns the value associated with the \a name in the QVariantMap
  variable for a view.
*/
