/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QFileInfo>
#include <QRegularExpression>
#include <TActionView>
#include <TAppSettings>
#include <THttpUtility>
#include <TViewHelper>
#include <TWebApplication>


/*!
  \class TViewHelper
  \brief The TViewHelper class provides some functionality for views.
*/

/*!
  Creates a \<a\> link tag of the given \a text using the given URL
  \a url and HTML attributes \a attributes. If \a method is Tf::Post,
  the link submits a POST request instead of GET.
*/
QString TViewHelper::linkTo(const QString &text, const QUrl &url, Tf::HttpMethod method, const THtmlAttribute &attributes) const
{
    return linkTo(text, url, method, QString(), attributes);
}

/*!
  Creates a \<a\> link tag of the given \a text using the given \a url.
  This is an overloaded function.
  The \a jsCondition argument serves for for creating Javascript confirm
  alerts where if you pass 'confirm' => 'Are you sure?', the link will be
  guarded with a Javascript popup asking that question. If the user accepts,
  the link is processed, otherwise not.
*/
QString TViewHelper::linkTo(const QString &text, const QUrl &url, Tf::HttpMethod method, const QString &jsCondition, const THtmlAttribute &attributes) const
{
    QString string("<a href=\"");
    string.append(url.toString(QUrl::FullyEncoded)).append("\"");

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
  Creates a \<a\> link tag of the given \a text using a given URL
  \a url in a popup window  with the name \a windowName.
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
  Creates a \<a\> link tag whose onclick handler triggers the passed JavaScript.
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
  Creates a input tag (type="button") whose onclick handler triggers
  the passed JavaScript.
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
    return selfClosingTag("input", attr);
}

/*!
  Creates a form tag that points to an \a url. If \a multipart is true,
  this function creates a form tag of multipart/form-data.
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
    endTags << endTag("form");
    return string;
}

/*!
  Creates a end-tag of \a name.
*/
QString TViewHelper::endTag(const QString &name) const
{
    QString string = "</";
    string += name;
    string += QLatin1Char('>');
    return string;
}

/*!
  Returns an end tag if there is a buffered tags.
*/
QString TViewHelper::endTag()
{
    return (endTags.isEmpty()) ? QString() : endTags.takeLast();
}

/*!
  Returns all end tags if there are buffered tags.
*/
QString TViewHelper::allEndTags()
{
    QString tags = endTags.join("");
    endTags.clear();
    return tags;
}

/*!
  Creates a \<input\> input tag with type=\a "type", name=\a "name" and
  value=\a "value".
*/
QString TViewHelper::inputTag(const QString &type, const QString &name, const QVariant &value,
    const THtmlAttribute &attributes) const
{
    THtmlAttribute attr = attributes;
    if (!value.isNull()) {
        attr.prepend("value", value.toString());
    }
    attr.prepend("name", name);
    attr.prepend("type", type);
    return selfClosingTag("input", attr);
}

/*!
  Creates a input tag with type="checkbox", name=\a "name" and value=\a "value".
*/
QString TViewHelper::checkBoxTag(const QString &name, const QVariant &value, bool checked, const THtmlAttribute &attributes) const
{
    THtmlAttribute attr = attributes;
    if (checked)
        attr.append("checked", "checked");
    return inputTag("checkbox", name, value, attr);
}

/*!
  Creates a input tag with type="checkbox", name=\a "name" and value=\a "value".
  If the \a checkedValue parameter is equal to the \a value parameter, this checkbox is checked.
*/
QString TViewHelper::checkBoxTag(const QString &name, const QVariant &value, const QVariant &checkedValue, const THtmlAttribute &attributes) const
{
    return checkBoxTag(name, value, (!value.toString().isEmpty() && value == checkedValue), attributes);
}

/*!
  Creates a input tag with type="checkbox", name=\a "name" and value=\a "value".
  If the \a checkedValues parameter contains the \a value parameter, this checkbox is checked.
*/
QString TViewHelper::checkBoxTag(const QString &name, const QString &value, const QStringList &checkedValues, const THtmlAttribute &attributes) const
{
    return checkBoxTag(name, value, (!value.isEmpty() && checkedValues.contains(value)), attributes);
}

/*!
  Creates a input tag with type="checkbox", name=\a "name" and value=\a "value".
  If the \a checkedValues parameter contains the \a value parameter, this checkbox is checked.
*/
QString TViewHelper::checkBoxTag(const QString &name, const QVariant &value, const QVariantList &checkedValues, const THtmlAttribute &attributes) const
{
    return checkBoxTag(name, value, (!value.toString().isEmpty() && checkedValues.contains(value)), attributes);
}

/*!
  Creates a input tag with type="radio", name=\a "name" and value=\a "value".
*/
QString TViewHelper::radioButtonTag(const QString &name, const QVariant &value, bool checked, const THtmlAttribute &attributes) const
{
    THtmlAttribute attr = attributes;
    if (checked)
        attr.append("checked", "checked");
    return inputTag("radio", name, value, attr);
}

/*!
  Creates a input tag with type="radio", name=\a "name" and value=\a "value".
  If the \a checkedValue parameter is equal to the \a value parameter, this radio button is checked.
*/
QString TViewHelper::radioButtonTag(const QString &name, const QVariant &value, const QVariant &checkedValue,
    const THtmlAttribute &attributes) const
{
    return radioButtonTag(name, value, (!value.toString().isEmpty() && value == checkedValue), attributes);
}

/*!
  Creates a select tag with name=\a "name".
 */
QString TViewHelper::selectTag(const QString &name, int size, bool multiple, const THtmlAttribute &attributes)
{
    THtmlAttribute attr = attributes;
    attr.prepend("size", QString::number(size));
    attr.prepend("name", name);

    if (multiple)
        attr.prepend("multiple", QString());

    return tag("select", attr);
}

/*!
  Creates a option tag for a select tag;
 */
QString TViewHelper::optionTag(const QString &text, const QVariant &value, bool selected,
    const THtmlAttribute &attributes) const
{
    QString ret;
    THtmlAttribute attr = attributes;

    if (selected)
        attr.prepend("selected", QString());

    attr.prepend("value", value.toString());
    return tag("option", attr, text);
}

/*!
  Creates option tags for a select tag;
  The option tag which value is equal to \a selectedValue parameter is selected.
 */
QString TViewHelper::optionTags(const QStringList &valueList, const QVariant &selectedValue, const THtmlAttribute &attributes) const
{
    QString ret;
    THtmlAttribute attr = attributes;

    for (auto &val : valueList) {
        if (!val.isEmpty() && val == selectedValue) {
            attr.prepend("selected", QString());
        }
        attr.prepend("value", val);
        ret += tag("option", attr, val);
        attr = attributes;
    }
    return ret;
}

/*!
  Creates option tags for a select tag;
  The option tag which value is equal to \a selectedValue parameter is selected.
 */
QString TViewHelper::optionTags(const QVariantList &valueList, const QVariant &selectedValue, const THtmlAttribute &attributes) const
{
    QString ret;
    THtmlAttribute attr = attributes;

    for (auto &val : valueList) {
        if (!val.isNull() && val == selectedValue) {
            attr.prepend("selected", QString());
        }
        attr.prepend("value", val.toString());
        ret += tag("option", attr, val.toString());
        attr = attributes;
    }
    return ret;
}

/*!
  Creates option tags for a select tag;
 */
QString TViewHelper::optionTags(const QList<QPair<QString, QVariant>> &valueList, const QVariant &selectedValue, const THtmlAttribute &attributes) const
{
    QString ret;
    THtmlAttribute attr = attributes;

    for (auto &val : valueList) {
        if (!val.second.isNull() && val.second == selectedValue) {
            attr.prepend("selected", QString());
        }
        attr.prepend("value", val.second.toString());
        ret += tag("option", attr, val.first);
        attr = attributes;
    }
    return ret;
}

/*!
  Creates a input tag with a authenticity token for CSRF protection.
*/
QString TViewHelper::inputAuthenticityTag() const
{
    QString tag;
    if (Tf::appSettings()->value(Tf::EnableCsrfProtectionModule, true).toBool()) {
        QString token = actionView()->authenticityToken();
        if (!token.isEmpty()) {
            tag = inputTag("hidden", "authenticity_token", token, a("id", "authenticity_token"));
        }
    }
    return tag;
}

/*!
  Creates a \<textarea\> text area tag with name=\a "name", rows=\a "rows"
  and cols=\a "cols".
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
  Creates a input tag with type="submit" and value=\a "value".
*/
QString TViewHelper::submitTag(const QString &value, const THtmlAttribute &attributes) const
{
    THtmlAttribute attr = attributes;
    attr.prepend("value", value);
    attr.prepend("type", "submit");
    return selfClosingTag("input", attr);
}

/*!
  Creates a input tag with type="image" and src=\a "src". The \a src must
  be one of URL, a absolute path or a relative path. If \a src is a relative
  path, it must exist in the public/images directory.
*/
QString TViewHelper::submitImageTag(const QString &src, const THtmlAttribute &attributes) const
{
    THtmlAttribute attr = attributes;
    attr.prepend("src", imagePath(src));
    attr.prepend("type", "image");
    return selfClosingTag("input", attr);
}

/*!
  Creates a input tag with type="reset" and value=\a "value".
*/
QString TViewHelper::resetTag(const QString &value, const THtmlAttribute &attributes) const
{
    THtmlAttribute attr = attributes;
    attr.prepend("value", value);
    attr.prepend("type", "reset");
    return selfClosingTag("input", attr);
}

/*!
  Creates a \<img\> image tag with src=\a "src". The \a src must be one
  of URL, a absolute path or a relative path. If \a src is a relative path,
  it must exist in the public/images directory.
*/
QString TViewHelper::imageTag(const QString &src, const QSize &size,
    const QString &alt,
    const THtmlAttribute &attributes) const
{
    return imageTag(src, false, size, alt, attributes);
}

/*!
  Creates a \<img\> image tag with src=\a "src". The \a src must be one
  of URL, a absolute path or a relative path. If \a src is a relative path,
  it must exist in the public/images directory. If \a withTimestamp is
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
    } else {
        attr.prepend("alt", "");  // output 'alt' always
    }

    if (size.height() > 0) {
        attr.prepend("height", QString::number(size.height()));
    }
    if (size.width() > 0) {
        attr.prepend("width", QString::number(size.width()));
    }

    attr.prepend("src", imagePath(src, withTimestamp));
    return selfClosingTag("img", attr);
}


QString TViewHelper::inlineImageTag(const QFileInfo &file, const QString &mediaType,
    const QSize &size, const QString &alt,
    const THtmlAttribute &attributes) const
{
    QByteArray data;
    QFile img(file.absoluteFilePath());
    if (img.open(QIODevice::ReadOnly)) {
        data = img.readAll();
        img.close();
    }
    return inlineImageTag(data, mediaType, size, alt, attributes);
}


QString TViewHelper::inlineImageTag(const QByteArray &data, const QString &mediaType,
    const QSize &size, const QString &alt,
    const THtmlAttribute &attributes) const
{
    THtmlAttribute attr = attributes;
    if (!alt.isEmpty()) {
        attr.prepend("alt", alt);
    } else {
        attr.prepend("alt", "");  // output 'alt' always
    }

    if (size.height() > 0) {
        attr.prepend("height", QString::number(size.height()));
    }
    if (size.width() > 0) {
        attr.prepend("width", QString::number(size.width()));
    }

    QByteArray dataUrl = "data:";
    dataUrl += mediaType.toLatin1() + ";base64,";
    dataUrl += data.toBase64();
    attr.prepend("src", dataUrl);
    return selfClosingTag("img", attr);
}

/*!
  Creates a \<link\> link tag for a style sheet with href=\a "src". The
  \a src must be one of URL, a absolute path or a relative path. If \a src
  is a relative path, it must exist in the public/css directory.
*/
QString TViewHelper::styleSheetTag(const QString &src, const THtmlAttribute &attributes) const
{
    return styleSheetTag(src, true, attributes);
}

/*!
  Creates a \<link\> link tag for a style sheet with href=\a "src". The
  \a src must be one of URL, a absolute path or a relative path. If \a src
  is a relative path, it must exist in the public/css directory.
*/
QString TViewHelper::styleSheetTag(const QString &src, bool withTimestamp, const THtmlAttribute &attributes) const
{
    THtmlAttribute attr = attributes;
    if (!attr.contains("type")) {
        attr.prepend("type", "text/css");
    }
    if (!attr.contains("rel")) {
        attr.prepend("rel", "stylesheet");
    }
    attr.prepend("href", cssPath(src, withTimestamp));
    return selfClosingTag("link", attr);
}

/*!
  Creates a \<script\> script tag with src=\a "src". The \a src must
  be one of URL, a absolute path or a relative path. If \a src is a
  relative path, it must exist in the public/js directory.
*/
QString TViewHelper::scriptTag(const QString &src, const THtmlAttribute &attributes) const
{
    return scriptTag(src, true, attributes);
}

/*!
  Creates a \<script\> script tag with src=\a "src". The \a src must
  be one of URL, a absolute path or a relative path. If \a src is a
  relative path, it must exist in the public/js directory.
*/
QString TViewHelper::scriptTag(const QString &src, bool withTimestamp, const THtmlAttribute &attributes) const
{
    THtmlAttribute attr = attributes;
    if (!attr.contains("type")) {
        attr.prepend("type", "text/javascript");
    }
    attr.prepend("src", jsPath(src, withTimestamp));
    return tag("script", attr, QString());
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
  Creates a start-tag of \a name with the given HTML attributes \a attributes.
*/
QString TViewHelper::tag(const QString &name, const THtmlAttribute &attributes)
{
    QString string = QLatin1String("<");
    string += name;
    string += attributes.toString();
    string += QLatin1Char('>');
    endTags << endTag(name);
    return string;
}

/*!
  Creates a start-tag of \a name with the given HTML attributes \a attributes.
*/
QString TViewHelper::tag(const QString &name, const THtmlAttribute &attributes, bool selfClose)
{
    return (selfClose) ? selfClosingTag(name, attributes) : tag(name, attributes);
}

/*!
  Creates an HTML element composed of a start-tag of \a name with
  HTML attributes \a attributes, a content \a content and an end-tag.
 */
QString TViewHelper::tag(const QString &name, const THtmlAttribute &attributes, const QString &content) const
{
    QString string = QLatin1String("<");
    string += name;
    string += attributes.toString();
    string += QLatin1Char('>');
    string += content;
    string += endTag(name);
    return string;
}

/*!
  Creates a self closing tag of \a name with the given HTML attributes
  \a attributes.
*/
QString TViewHelper::selfClosingTag(const QString &name, const THtmlAttribute &attributes) const
{
    QString string = QLatin1String("<");
    string += name;
    string += attributes.toString();
    string += QLatin1String(" />");
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
QString TViewHelper::cssPath(const QString &src, bool withTimestamp) const
{
    return srcPath(src, "/css/", withTimestamp);
}

/*!
  Returns a javascript path to \a src. The \a src must be one of URL, a absolute
  path or a relative path. If \a src is a relative path, it must exist
  in the public/js directory.
 */
QString TViewHelper::jsPath(const QString &src, bool withTimestamp) const
{
    return srcPath(src, "/js/", withTimestamp);
}

/*!
  Returns a path to \a src. The \a src must be one of URL, a absolute
  path or a relative path. If \a src is a relative path, it must exist
  in the public directory.
*/
QString TViewHelper::srcPath(const QString &src, const QString &dir, bool withTimestamp) const
{
    if (src.contains(QRegularExpression("^[a-z]+://"))) {
        return src;
    }

    QString ret = (src.startsWith('/')) ? src : dir + src;

    if (withTimestamp) {
        QFileInfo fi(Tf::app()->publicPath() + ret);
        if (fi.exists()) {
            ret += QLatin1Char('?');
            ret += QString::number(fi.lastModified().toSecsSinceEpoch());
        }
    }
    return ret;
}


/*!
  \fn THtmlAttribute TViewHelper::a() const
  Returns a null THtmlAttribute object.
*/

/*!
  \fn const TActionView *TViewHelper::actionView() const
  Must be overridden by subclasses to return the current action view.
*/
