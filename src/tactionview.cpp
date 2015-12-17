/* Copyright (c) 2010-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TActionView>
#include <TActionController>
#include <THttpUtility>
#include <THtmlAttribute>

/*!
  \class TActionView
  \brief The TActionView class is the abstract base class of views,
  providing functionality common to view.
*/


/*!
  Constructor.
*/
TActionView::TActionView()
    : QObject(), TViewHelper(), TPrototypeAjaxHelper(), actionController(0), subView(0)
{ }

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
  Returns a authenticity token for CSRF protection.
*/
QString TActionView::authenticityToken() const
{
    return (actionController) ? QString::fromLatin1(actionController->authenticityToken().data()) : QString();
}

/*!
  Outputs the string of the HTML attribute \a attr to a view
  template.
*/
QString TActionView::echo(const THtmlAttribute &attr)
{
    responsebody += attr.toString().trimmed();
    return QString();
}

/*!
  Outputs a escaped string of the HTML attribute \a attr to
  a view template.
*/
QString TActionView::eh(const THtmlAttribute &attr)
{
    return echo(THttpUtility::htmlEscape(attr.toString().trimmed()));
}

/*!
  Returns the requested HTTP message.
*/
const THttpRequest &TActionView::httpRequest() const
{
    return controller()->httpRequest();
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
  \fn QString TActionView::echo(const QVariant &var)
  Outputs the variant variable \a var to a view template.
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
  \fn QString TActionView::eh(const QVariant &var)
  Outputs a escaped string of the variant variable \a var
  to a view template.
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
