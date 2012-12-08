/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QFileInfo>
#include <QRegExp>
#include <TViewHelper>
#include <TWebApplication>
#include <TActionView>
#include <THttpUtility>

#define ENABLE_CSRF_PROTECTION_MODULE "EnableCsrfProtectionModule"


/*!
  \class TViewHelper
  \brief The TViewHelper class provides some functionality for views.
*/

/*!
  Creates a link tag of the given \a text using a given \a url.
 */
QString TViewHelper::linkTo(const QString &text, const QUrl &url, Tf::HttpMethod method, const THtmlAttribute &attributes) const
{
    return linkTo(text, url, method, QString(), attributes);
}

/*!
  Creates a link tag of the given \a text using a given \a url.
 */
QString TViewHelper::linkTo(const QString &text, const QUrl &url, Tf::HttpMethod method, const QString &jsCondition, const THtmlAttribute &attributes) const
{
    QString string("<a href=\"");
    string.append(url.toString()).append("\"");

    if (method == Tf::Post) {
        string.append(" onclick=\"");
        if (!jsCondition.isEmpty()) {
            string.append("if (").append(jsCondition).append(") { ");
        }
        
        string += "var f = document.createElement('form'); document.body.appendChild(f); f.method = 'post'; f.action = this.href;";

        // Authenticity token
        QString token = actionView()->authenticityToken();
        if (!token.isEmpty()) {
            string += " var i = document.createElement('input'); f.appendChild(i); i.type = 'hidden'; i.name = 'authenticity_token'; i.value = '";
            string += token;
            string += "';";
        }
        string += " f.submit();";

        if (!jsCondition.isEmpty()) {
            string += " }";
        }
        string += " return false;\"";
    } else {
        if (!jsCondition.isEmpty()) {
            string.append(" onclick=\"return ").append(jsCondition).append(";\"");
        }
    }
    
    string.append(attributes.toString()).append(">").append(text).append("</a>");
    return string;
}

/*!
  Creates a link tag of the given \a text using a given \a url in a popup window.
 */
QString TViewHelper::linkToPopup(const QString &text, const QUrl &url, const QString &windowName,
                                 const QSize &size, const QPoint &topLeft, const QString &windowStyle,
                                 const QString &jsCondition, const THtmlAttribute &attributes) const
{
    QString string("<a href=\"");
    string.append(url.toString()).append("\"");

    string += " onclick=\"";
    if (!jsCondition.isEmpty()) {
        string.append("if (").append(jsCondition).append(") { ");
    }

    string += "window.open(this.href";
    if (!windowName.isEmpty()) {
        string.append(", '").append(windowName).append("'");
    }
    
    string += ", '";
    if (size.isValid()) {
        string.append("width=").append(QString::number(size.width())).append(",height=").append(QString::number(size.height()));
    }
    
    if (!topLeft.isNull()) {
        if (string.right(1) != "'")
            string += ",";
        string.append("top=").append(QString::number(topLeft.x())).append(",left=").append(QString::number(topLeft.y()));
    }

    if (!windowStyle.isEmpty()) {
        if (string.right(1) != "'")
            string += ",";
        string += windowStyle;
    }
    string += "');";

    if (!jsCondition.isEmpty()) {
        string += " }";
    }
    string += " return false;\"";

    string.append(attributes.toString()).append(">").append(text).append("</a>");
    return string;
}

/*!
  Creates a link tag whose onclick handler triggers the passed JavaScript.
 */
QString TViewHelper::linkToFunction(const QString &text, const QString &function,
                                    const THtmlAttribute &attributes) const
{
    QString string("<a href=\"#\" onclick=\"");
    QString func = function.trimmed();
    if (!func.isEmpty() && !func.endsWith(";")) {
        func += QLatin1Char(';');
    }
    string += func;
    string += QLatin1String(" return false;\"");
    string += attributes.toString();
    string += QLatin1Char('>');
    string += text;
    string += QLatin1String("</a>");
    return string;
}

/*!
  Creates a input tag (type="button") whose onclick handler triggers the passed JavaScript.
 */
QString TViewHelper::buttonToFunction(const QString &text, const QString &function,
                                      const THtmlAttribute &attributes) const
{
    QString onclick = function.trimmed();
    if (!onclick.isEmpty() && !onclick.endsWith(";")) {
        onclick += QLatin1Char(';');
    }
    onclick += QLatin1String(" return false;");

    THtmlAttribute attr = attributes;
    attr.prepend("onclick", onclick);
    attr.prepend("value", text);
    attr.prepend("type", "button");
    return tag("input", attr);
}

/*!
  Creates a form tag that points to an \a url. If \a multipart is true, this function
  creates a form tag of multipart/form-data.
 */
QString TViewHelper::formTag(const QUrl &url, Tf::HttpMethod method, bool multipart,
                             const THtmlAttribute &attributes)
{
    QString string;
    string.append("<form action=\"").append(url.toString()).append("\"");
    
    if (multipart) {
        string += " enctype=\"multipart/form-data\"";
    }

    string += " method=";
    string += (method == Tf::Post) ? "\"post\"" : "\"get\"";

    string.append(attributes.toString()).append(">").append(inputAuthenticityTag());
    endTags << QLatin1String("</form>");
    return string;
}

/*!
  Creates an end tag such as form tag.
*/
QString TViewHelper::endTag()
{
    return (endTags.isEmpty()) ? QString() : endTags.takeLast();
}

/*!
  Creates an end tags such as form tags.
*/
QString TViewHelper::allEndTags()
{
    QString tags = endTags.join("");
    endTags.clear();
    return tags;
}

/*!
  Creates a input tag with name=\a "name" and value=\a "value".
 */
QString TViewHelper::inputTag(const QString &type, const QString &name, const QVariant &value,
                              const THtmlAttribute &attributes) const
{
    THtmlAttribute attr = attributes;
    attr.prepend("value", value.toString());
    attr.prepend("name", name);
    attr.prepend("type", type);
    return tag("input", attr);
}

/*!
  Creates a input tag with type="checkbox", name=\a "name" and value=\a "value".
 */
QString TViewHelper::checkBoxTag(const QString &name, const QString &value, bool checked, const THtmlAttribute &attributes) const
{
    THtmlAttribute attr = attributes;
    if (checked)
        attr.append("checked", "checked");
    return inputTag("checkbox", name, value, attr);
}

/*!
  Creates a input tag with type="radio", name=\a "name" and value=\a "value".
 */
QString TViewHelper::radioButtonTag(const QString &name, const QString &value, bool checked, const THtmlAttribute &attributes) const
{
    THtmlAttribute attr = attributes;
    if (checked)
        attr.append("checked", "checked");
    return inputTag("radio", name, value, attr);
}

/*!
  Creates a input tag with a authenticity token for CSRF protection.
 */
QString TViewHelper::inputAuthenticityTag() const
{
    QString tag;
    if (Tf::app()->appSettings().value(ENABLE_CSRF_PROTECTION_MODULE, true).toBool()) {
        QString token = actionView()->authenticityToken();
        if (!token.isEmpty())
            tag = inputTag("hidden", "authenticity_token", token);
    }
    return tag;
}

/*!
  Creates a textarea tag with rows=\a "rows" and cols=\a "cols".
 */
QString TViewHelper::textAreaTag(const QString &name, int rows, int cols, const QString &content, const THtmlAttribute &attributes) const
{
    THtmlAttribute attr = attributes;
    attr.prepend("cols", QString::number(cols));
    attr.prepend("rows", QString::number(rows));
    attr.prepend("name", name);
    return tag("textarea", attr, content);
}

/*!
  Creates a input tag with type=\a "submit" and value=\a "value".
 */
QString TViewHelper::submitTag(const QString &value, const THtmlAttribute &attributes) const
{
    THtmlAttribute attr = attributes;
    attr.prepend("value", value);
    attr.prepend("type", "submit");
    return tag("input", attr);
}

/*!
  Creates a input tag with type=\a "image" and src=\a "src". The \a src must
  be one of URL, a absolute path or a relative path. If \a src is a relative
  path, it must exist in the public/images directory.
 */
QString TViewHelper::submitImageTag(const QString &src, const THtmlAttribute &attributes) const
{
    THtmlAttribute attr = attributes;
    attr.prepend("src", imagePath(src));
    attr.prepend("type", "image");
    return tag("input", attr);
}

/*!
  Creates a input tag with type=\a "reset" and value=\a "value".
 */
QString TViewHelper::resetTag(const QString &value, const THtmlAttribute &attributes) const
{
    THtmlAttribute attr = attributes;
    attr.prepend("value", value);
    attr.prepend("type", "reset");
    return tag("input", attr);
}

/*!
  Creates a img tag with src=\a "src". The \a src must be one of URL, a
  absolute path or a relative path. If \a src is a relative path, it
  must exist in the public/images directory.
 */
QString TViewHelper::imageTag(const QString &src, const QSize &size,
                              const QString &alt,
                              const THtmlAttribute &attributes) const
{
    return imageTag(src, false, size, alt, attributes);
}

/*!
  Creates a img tag with src=\a "src". The \a src must be one of URL, a
  absolute path or a relative path. If \a src is a relative path, it
  must exist in the public/images directory. If the \a withTimestamp is
  true, the timestamp of the image file is append to \a src as a query
  parameter.
 */
QString TViewHelper::imageTag(const QString &src, bool withTimestamp,
                              const QSize &size, const QString &alt,
                              const THtmlAttribute &attributes) const
{
    THtmlAttribute attr = attributes;
    if (!alt.isEmpty()) {
        attr.prepend("alt", alt);
    }

    if (!size.isEmpty()) {
        attr.prepend("height", QString::number(size.height()));
        attr.prepend("width", QString::number(size.width()));
    }

    attr.prepend("src", imagePath(src, withTimestamp));
    return tag("img", attr);
}

/*!
  Creates a stylesheet link tag with href=\a "src". The \a src must
  be one of URL, a absolute path or a relative path. If \a src is a
  relative path, it must exist in the public/css directory.
 */
QString TViewHelper::stylesheetTag(const QString &src, const THtmlAttribute &attributes) const
{
    THtmlAttribute attr = attributes;
    if (!attr.contains("type"))
        attr.prepend("type", "text/css");
  
    if (!attr.contains("rel"))
        attr.prepend("rel", "stylesheet");
    
    attr.prepend("href", cssPath(src));
    return tag("link", attr);
}

/*!
  Creates and returns a THtmlAttribute object with \a key =\a "value".
 */
THtmlAttribute TViewHelper::a(const QString &key, const QString &value) const
{
    THtmlAttribute attr;
    attr.append(key, value);
    return attr;
}

/*!
  Creates a tag of \a name with attributes \a attributes.
 */
QString TViewHelper::tag(const QString &name, const THtmlAttribute &attributes, bool selfClosing) const
{
    QString string = "<";
    string += name;
    string += attributes.toString();
    string += (selfClosing) ? QLatin1String(" />") : QLatin1String(">");
    return string;
}

/*!
  Creates an HTML element composed of a start-tag of \a name with
  \a attributes, a content \a content and a end-tag.
 */
QString TViewHelper::tag(const QString &name, const THtmlAttribute &attributes, const QString &content) const
{
    QString string = tag(name, attributes, false);
    string += content;
    string += QLatin1String("</");
    string += name;
    string += QLatin1Char('>');
    return string;
}

/*!
  Returns a image path to \a src. The \a src must be one of URL, a absolute
  path or a relative path. If \a src is a relative path, it must exist
  in the public/images directory.
 */
QString TViewHelper::imagePath(const QString &src, bool withTimestamp) const
{
    return srcPath(src, "/images/", withTimestamp);
}

/*!
  Returns a css path to \a src. The \a src must be one of URL, a absolute
  path or a relative path. If \a src is a relative path, it must exist
  in the public/css directory.
 */
QString TViewHelper::cssPath(const QString &src) const
{
    return srcPath(src, "/css/");
}

/*!
  Returns a javascript path to \a src. The \a src must be one of URL, a absolute
  path or a relative path. If \a src is a relative path, it must exist
  in the public/js directory.
 */
QString TViewHelper::jsPath(const QString &src) const
{
    return srcPath(src, "/js/");
}

/*!
  Returns a path to \a src. The \a src must be one of URL, a absolute
  path or a relative path. If \a src is a relative path, it must exist
  in the public directory.
 */
QString TViewHelper::srcPath(const QString &src, const QString &dir, bool withTimestamp) const
{
    if (src.contains(QRegExp("^[a-z]+://"))) {
        return src;
    }

    QString ret = (src.startsWith('/')) ? src : dir + src;

    if (withTimestamp) {
        QFileInfo fi(Tf::app()->publicPath() + ret);
        if (fi.exists()) {
            ret += QLatin1Char('?');
            ret += QString::number(fi.lastModified().toTime_t());
        }
    }
    return ret;
}


/*!
  \fn QString TViewHelper::imageLinkTo(const QString &src, const QUrl &url, const QSize &size, const QString &alt, const THtmlAttribute &attributes) const

  Creates a link tag of a given \a url with img tag with src=\a "src".
*/
