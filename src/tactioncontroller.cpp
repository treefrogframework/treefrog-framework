/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsessionmanager.h"
#include "ttextview.h"
#include <QCryptographicHash>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QMetaMethod>
#include <QMetaType>
#include <QMutexLocker>
#include <QTextCodec>
#include <QTextStream>
#include <TAbstractUser>
#include <TActionContext>
#include <TActionController>
#include <TActionView>
#include <TAppSettings>
#include <TCache>
#include <TDispatcher>
#include <TFormValidator>
#include <TSession>
#include <TWebApplication>

const QString FLASH_VARS_SESSION_KEY("_flashVariants");
const QString LOGIN_USER_NAME_KEY("_loginUserName");
const QByteArray DEFAULT_CONTENT_TYPE("text/html");

/*!
  \class TActionController
  \brief The TActionController class is the base class of all action
  controllers.
*/

/*!
  \brief Constructor.
 */
TActionController::TActionController() :
    TAbstractController()
{
    // Default content type
    setContentType(DEFAULT_CONTENT_TYPE);
}

/*!
  \brief Destructor.
*/
TActionController::~TActionController()
{
}

/*!
  Returns the controller name.
*/
QString TActionController::name() const
{
    static const QString Postfix = "Controller";

    if (_ctrlName.isEmpty()) {
        _ctrlName = className();
        if (_ctrlName.endsWith(Postfix)) {
            _ctrlName.resize(_ctrlName.length() - Postfix.length());
        }
    }
    return _ctrlName;
}


/*!
  \fn QString TActionController::activeAction() const
  Returns the active action name.
*/

/*!
  Returns the HTTP request being executed.
*/
const THttpRequest &TActionController::request() const
{
    return Tf::currentContext()->httpRequest();
}

/*!
  Returns the HTTP request being executed.
*/
THttpRequest &TActionController::request()
{
    return Tf::currentContext()->httpRequest();
}

/*!
  \fn const THttpResponse &TActionController::httpResponse() const;
  Returns a HTTP response to be sent.
*/

/*!
  \fn THttpResponse &TActionController::httpResponse();
  Returns a HTTP response to be sent.
*/

/*!
  Sets the layout template to \a layout.
  \sa layout()
 */
void TActionController::setLayout(const QString &layout)
{
    if (!layout.isNull()) {
        _layoutName = layout;
    }
}

/*!
  Adds the cookie to the internal list of cookies.
 */
bool TActionController::addCookie(const TCookie &cookie)
{
    QByteArray name = cookie.name();
    if (name.isEmpty() || name.contains(';') || name.contains(',') || name.contains(' ') || name.contains('\"')) {
        tError("Invalid cookie name: %s", name.data());
        return false;
    }

    _cookieJar.addCookie(cookie);
    _response.header().removeAllRawHeaders("Set-Cookie");
    for (auto &ck : (const QList<TCookie> &)_cookieJar.allCookies()) {
        _response.header().addRawHeader("Set-Cookie", ck.toRawForm(QNetworkCookie::Full));
    }
    return true;
}

/*!
  Adds the cookie to the internal list of cookies.
 */
bool TActionController::addCookie(const QByteArray &name, const QByteArray &value, const QDateTime &expire,
    const QString &path, const QString &domain, bool secure, bool httpOnly,
    const QByteArray &sameSite)
{
    TCookie cookie(name, value);
    cookie.setExpirationDate(expire);
    cookie.setPath(path);
    cookie.setDomain(domain);
    cookie.setSecure(secure);
    cookie.setHttpOnly(httpOnly);
    cookie.setSameSite(sameSite);
    return addCookie(cookie);
}


bool TActionController::addCookie(const QByteArray &name, const QByteArray &value, qint64 maxAge, const QString &path,
    const QString &domain, bool secure, bool httpOnly, const QByteArray &sameSite)
{
    TCookie cookie(name, value);
    cookie.setMaxAge(maxAge);
    if (maxAge > 0) {
        cookie.setExpirationDate(QDateTime::currentDateTime().addSecs(maxAge));  // For IE11
    }
    cookie.setPath(path);
    cookie.setDomain(domain);
    cookie.setSecure(secure);
    cookie.setHttpOnly(httpOnly);
    cookie.setSameSite(sameSite);
    return addCookie(cookie);
}

/*!
  Returns the authenticity token.
 */
QByteArray TActionController::authenticityToken() const
{
    if (Tf::appSettings()->value(Tf::SessionStoreType).toString().toLower() == QLatin1String("cookie")) {
        QString key = Tf::appSettings()->value(Tf::SessionCsrfProtectionKey).toString();
        QByteArray csrfId = session().value(key).toByteArray();

        if (csrfId.isEmpty()) {
            throw RuntimeException("CSRF protectionsession value is empty", __FILE__, __LINE__);
        }
        return csrfId;
    } else {
        return QCryptographicHash::hash(session().id() + Tf::appSettings()->value(Tf::SessionSecret).toByteArray(), QCryptographicHash::Sha1).toHex();
    }
}

/*!
  Sets the HTTP session to \a session.
 */
void TActionController::setSession(const TSession &session)
{
    _sessionStore = session;
}

/*!
  Sets CSRF protection informaion into \a session. Internal use.
*/
void TActionController::setCsrfProtectionInto(TSession &session)
{
    if (Tf::appSettings()->value(Tf::SessionStoreType).toString().toLower() == QLatin1String("cookie")) {
        QString key = Tf::appSettings()->value(Tf::SessionCsrfProtectionKey).toString();
        session.insert(key, TSessionManager::instance().generateId());  // it's just a random value
    }
}

/*!
  Returns the list of all available controllers.
*/
const QStringList &TActionController::availableControllers()
{
    static QStringList controllers = []() {
        QStringList controllers;
        for (auto it = Tf::objectFactories()->cbegin(); it != Tf::objectFactories()->cend(); ++it) {
            if (it.key().endsWith("controller")) {
                controllers << QString::fromLatin1(it.key());
            }
        }
        std::sort(controllers.begin(), controllers.end());
        return controllers;
    }();
    return controllers;
}


const QStringList &TActionController::disabledControllers()
{
    static const QStringList disabledNames = {"application"};
    return disabledNames;
}


QString TActionController::loginUserNameKey()
{
    return LOGIN_USER_NAME_KEY;
}

/*!
  Verifies the HTTP request \a request.
*/
bool TActionController::verifyRequest(const THttpRequest &request) const
{
    if (!csrfProtectionEnabled()) {
        return true;
    }

    if (Tf::appSettings()->value(Tf::SessionStoreType).toString().toLower() != QLatin1String("cookie")) {
        if (session().id().isEmpty()) {
            throw SecurityException("Request Forgery Protection requires a valid session", __FILE__, __LINE__);
        }
    }

    QByteArray postAuthToken = request.parameter("authenticity_token").toLatin1();
    if (postAuthToken.isEmpty()) {
        throw SecurityException("Authenticity token is empty", __FILE__, __LINE__);
    }

    tSystemDebug("postAuthToken: %s", postAuthToken.data());
    return (postAuthToken == authenticityToken());
}

/*!
  Renders the template of the action \a action with the layout \a layout.
 */
bool TActionController::render(const QString &action, const QString &layout)
{
    if (_rendered) {
        tWarn("Has rendered already: %s", qUtf8Printable(className() + '.' + activeAction()));
        return false;
    }
    _rendered = true;

    // Creates view-object and displays it
    TDispatcher<TActionView> viewDispatcher(viewClassName(action));
    setLayout(layout);
    _response.setBody(renderView(viewDispatcher.object()));
    return !response().isBodyNull();
}

/*!
  Renders the template given by \a templateName with the layout \a layout.
*/
bool TActionController::renderTemplate(const QString &templateName, const QString &layout)
{
    if (_rendered) {
        tWarn("Has rendered already: %s", qUtf8Printable(className() + '#' + activeAction()));
        return false;
    }
    _rendered = true;

    // Creates view-object and displays it
    QStringList names = templateName.split("/");
    if (names.count() != 2) {
        tError("Invalid patameter: %s", qUtf8Printable(templateName));
        return false;
    }
    TDispatcher<TActionView> viewDispatcher(viewClassName(names[0], names[1]));
    setLayout(layout);
    _response.setBody(renderView(viewDispatcher.object()));
    return (!response().isBodyNull());
}

/*!
  Renders the text \a text with the layout \a layout.
*/
bool TActionController::renderText(const QString &text, bool layoutEnable, const QString &layout)
{
    if (_rendered) {
        tWarn("Has rendered already: %s", qUtf8Printable(className() + '#' + activeAction()));
        return false;
    }
    _rendered = true;

    if (contentType() == DEFAULT_CONTENT_TYPE) {
        setContentType(QByteArrayLiteral("text/plain"));
    }

    // Creates TTextView object and displays it
    setLayout(layout);
    setLayoutEnabled(layoutEnable);
    TTextView *view = new TTextView(text);
    _response.setBody(renderView(view));
    delete view;
    return (!_response.isBodyNull());
}


static QDomElement createDomElement(const QString &name, const QVariantMap &map, QDomDocument &document)
{
    QDomElement element = document.createElement(name);

    for (auto it = map.begin(); it != map.end(); ++it) {
        QDomElement tag = document.createElement(it.key());
        element.appendChild(tag);

        QDomText text = document.createTextNode(it.value().toString());
        tag.appendChild(text);
    }
    return element;
}

/*!
  Renders the XML document \a document.
*/
bool TActionController::renderXml(const QDomDocument &document)
{
    QByteArray xml;
    QTextStream ts(&xml);

#if QT_VERSION < 0x060000
    ts.setCodec("UTF-8");
#else
    ts.setEncoding(QStringConverter::Utf8);
#endif
    document.save(ts, 1, QDomNode::EncodingFromTextStream);
    return sendData(xml, "text/xml");
}

/*!
  Renders the \a map as XML document.
*/
bool TActionController::renderXml(const QVariantMap &map)
{
    QDomDocument doc;
    QDomElement root = doc.createElement("map");

    doc.appendChild(root);
    root.appendChild(createDomElement("map", map, doc));
    return renderXml(doc);
}

/*!
  Renders the list of variants \a list as XML document.
*/
bool TActionController::renderXml(const QVariantList &list)
{
    QDomDocument doc;
    QDomElement root = doc.createElement("list");
    doc.appendChild(root);

    for (auto &var : list) {
        QVariantMap map = var.toMap();
        root.appendChild(createDomElement("map", map, doc));
    }
    return renderXml(doc);
}

/*!
  Renders the list of strings \a list as XML document.
*/
bool TActionController::renderXml(const QStringList &list)
{
    QDomDocument doc;
    QDomElement root = doc.createElement("list");
    doc.appendChild(root);

    for (auto &str : list) {
        QDomElement tag = doc.createElement("string");
        root.appendChild(tag);
        QDomText text = doc.createTextNode(str);
        tag.appendChild(text);
    }

    return renderXml(doc);
}

/*!
  Renders the template of the \a action with the \a layout and caches it with
  the \a key for \a seconds.
  \sa renderOnCache()
*/
bool TActionController::renderAndCache(const QByteArray &key, int seconds, const QString &action, const QString &layout)
{
    if (_rendered) {
        tWarn("Has rendered already: %s", qUtf8Printable(className() + '.' + activeAction()));
        return false;
    }

    render(action, layout);
    if (_rendered) {
        QByteArray responseMsg = response().body();
        Tf::cache()->set(key, responseMsg, seconds);
    }
    return _rendered;
}

/*!
  Renders the template cached with the \a key. If no item with the \a key
  found, returns false.
  \sa renderAndCache()
*/
bool TActionController::renderOnCache(const QByteArray &key)
{
    if (_rendered) {
        tWarn("Has rendered already: %s", qUtf8Printable(className() + '.' + activeAction()));
        return false;
    }

    auto responseMsg = Tf::cache()->get(key);
    if (responseMsg.isEmpty()) {
        return false;
    }

    _response.setBody(responseMsg);
    _rendered = true;
    return _rendered;
}

/*!
  Removes the template with the \a key from the cache.
*/
void TActionController::removeCache(const QByteArray &key)
{
    Tf::cache()->remove(key);
}

/*!
  Returns the rendering data of the partial template given by \a templateName.
*/
QString TActionController::getRenderingData(const QString &templateName, const QVariantMap &vars)
{

    // Creates view-object
    QStringList names = templateName.split("/");
    if (names.count() != 2) {
        tError("Invalid patameter: %s", qUtf8Printable(templateName));
        return QString();
    }

    TDispatcher<TActionView> viewDispatcher(viewClassName(names[0], names[1]));
    TActionView *view = viewDispatcher.object();
    if (!view) {
        return QString();
    }

    QVariantMap map = allVariants();
    for (auto it = vars.begin(); it != vars.end(); ++it) {
        map.insert(it.key(), it.value());  // item's value of same key is replaced
    }

    view->setController(this);
    view->setVariantMap(map);
    return view->toString();
}

/*!
  Renders the \a view view.
*/
QByteArray TActionController::renderView(TActionView *view)
{
    if (!view) {
        tSystemError("view null pointer.  action:%s", qUtf8Printable(activeAction()));
        return QByteArray();
    }
    view->setController(this);
    view->setVariantMap(allVariants());

    if (!layoutEnabled()) {
        // Renders without layout
        tSystemDebug("Renders without layout");
        return Tf::app()->codecForHttpOutput()->fromUnicode(view->toString());
    }

    // Displays with layout
    QString lay = (layout().isNull()) ? name().toLower() : layout().toLower();
    TDispatcher<TActionView> layoutDispatcher(layoutClassName(lay));
    TActionView *layoutView = layoutDispatcher.object();

    TDispatcher<TActionView> defLayoutDispatcher(layoutClassName(QLatin1String("application")));
    if (!layoutView) {
        if (!layout().isNull()) {
            tSystemDebug("Not found layout: %s", qUtf8Printable(layout()));
            return QByteArray();
        } else {
            // Use default layout
            layoutView = defLayoutDispatcher.object();
            if (!layoutView) {
                tSystemDebug("Not found default layout. Renders without layout.");
                return Tf::app()->codecForHttpOutput()->fromUnicode(view->toString());
            }
        }
    }

    // Renders layout
    layoutView->setVariantMap(allVariants());
    layoutView->setController(this);
    layoutView->setSubActionView(view);
    return Tf::app()->codecForHttpOutput()->fromUnicode(layoutView->toString());
}

/*!
  Renders a static error page with the status code, which page is [statusCode].html
  in the \a public directory.
 */
bool TActionController::renderErrorResponse(int statusCode)
{
    bool ret = false;

    if (_rendered) {
        tWarn("Has rendered already: %s", qUtf8Printable(className() + '#' + activeAction()));
        return ret;
    }

    QString file = Tf::app()->publicPath() + QString::number(statusCode) + QLatin1String(".html");
    if (QFileInfo(file).exists()) {
        ret = sendFile(file, "text/html", "", false);
    } else {
        _response.setBody("");
    }
    setStatusCode(statusCode);
    _rendered = true;
    return ret;
}

/*!
  Returns the layout class name. Internal use.
 */
QString TActionController::layoutClassName(const QString &layout)
{
    return QLatin1String("layouts_") + layout + QLatin1String("View");
}

/*!
  Returns the class name of the partial view. Internal use.
 */
QString TActionController::partialViewClassName(const QString &partial)
{
    return QLatin1String("partial_") + partial + QLatin1String("View");
}

/*!
  Redirects to the URL \a url.
 */
void TActionController::redirect(const QUrl &url, int statusCode)
{
    if (_rendered) {
        tError("Unable to redirect. Has rendered already: %s", qUtf8Printable(className() + '#' + activeAction()));
        return;
    }
    _rendered = true;

    setStatusCode(statusCode);
    _response.header().setRawHeader("Location", url.toEncoded());
    _response.setBody(QByteArray("<html><body>redirected.</body></html>"));
    setContentType("text/html");

    // Enable flash-variants
    QVariant var;
    var.setValue(_flashVars);
    _sessionStore.insert(FLASH_VARS_SESSION_KEY, var);
}

/*!
  Sends the file \a filePath as HTTP response.
*/
bool TActionController::sendFile(const QString &filePath, const QByteArray &contentType, const QString &name, bool autoRemove)
{
    if (_rendered) {
        tWarn("Has rendered already: %s", qUtf8Printable(className() + '#' + activeAction()));
        return false;
    }
    _rendered = true;

    if (!name.isEmpty()) {
        QByteArray filename;
        filename += "attachment; filename=\"";
        filename += name.toUtf8();
        filename += '"';
        _response.header().setRawHeader("Content-Disposition", filename);
    }

    _response.setBodyFile(filePath);
    setContentType(contentType);

    if (autoRemove) {
        setAutoRemove(filePath);
    }
    return true;
}

/*!
  Sends the data \a data as HTTP response.
*/
bool TActionController::sendData(const QByteArray &data, const QByteArray &contentType, const QString &name)
{
    if (_rendered) {
        tWarn("Has rendered already: %s", qUtf8Printable(className() + '#' + activeAction()));
        return false;
    }
    _rendered = true;

    if (!name.isEmpty()) {
        QByteArray filename;
        filename += "attachment; filename=\"";
        filename += name.toUtf8();
        filename += '"';
        _response.header().setRawHeader("Content-Disposition", filename);
    }

    _response.setBody(data);
    setContentType(contentType);
    return true;
}

/*!
  Exports the all flash variants.
*/
void TActionController::exportAllFlashVariants()
{
    QVariant var = _sessionStore.take(FLASH_VARS_SESSION_KEY);
    if (!var.isNull()) {
        exportVariants(var.toMap());
    }
}

/*!
  Validates the access of the user \a user. Returns true if the user
  access is allowed by rule; otherwise returns false.
  @sa setAccessRules(), TAccessValidator::validate()
*/
bool TActionController::validateAccess(const TAbstractUser *user)
{
    if (TAccessValidator::accessRules.isEmpty()) {
        setAccessRules();
    }
    return TAccessValidator::validate(user);
}

/*!
  Logs the user \a user in to the system.

  This is a virtual function.
  \sa userLogout()
*/
bool TActionController::userLogin(const TAbstractUser *user)
{
    if (!user) {
        tSystemError("userLogin: null specified");
        return false;
    }

    if (user->identityKey().isEmpty()) {
        tSystemError("userLogin: identityKey empty");
        return false;
    }

    if (isUserLoggedIn()) {
        tSystemWarn("userLogin: Duplicate login detected. Force logout [user:%s]", qUtf8Printable(identityKeyOfLoginUser()));
    }

    session().insert(LOGIN_USER_NAME_KEY, user->identityKey());
    return true;
}

/*!
  Logs out of the system.

  This is a virtual function.
  \sa userLogin()
*/
void TActionController::userLogout()
{
    session().take(LOGIN_USER_NAME_KEY);
}

/*!
  Returns true if a user is logged in to the system; otherwise returns false.
  This is a virtual function.
  \sa userLogin()
*/
bool TActionController::isUserLoggedIn() const
{
    return session().contains(LOGIN_USER_NAME_KEY);
}

/*!
  Returns the identity key of the user, i.e., TAbstractUser object,
  logged in.

  This is a virtual function.
  \sa userLogin()
*/
QString TActionController::identityKeyOfLoginUser() const
{
    return session().value(LOGIN_USER_NAME_KEY).toString();
}

/*!
  Sets the automatically removing file.

  The file \a filePath is removed when the context is extinguished,
  after replied the HTTP response.
*/
void TActionController::setAutoRemove(const QString &filePath)
{
    if (!filePath.isEmpty() && !_autoRemoveFiles.contains(filePath)) {
        _autoRemoveFiles << filePath;
    }
}

/*!
  Returns the client address of the current session.
*/
QHostAddress TActionController::clientAddress() const
{
    return Tf::currentContext()->clientAddress();
}

/*!
  Sets the flash message of \a name to \a value.
  \sa flash()
*/
void TActionController::setFlash(const QString &name, const QVariant &value)
{
    if (value.isValid()) {
        _flashVars.insert(name, value);
    } else {
        tSystemWarn("An invalid QVariant object for setFlash(), name:%s", qUtf8Printable(name));
    }
}

/*!
  Sets the validation errors to flash variant.
*/
void TActionController::setFlashValidationErrors(const TFormValidator &v, const QString &prefix)
{
    for (auto &key : (const QStringList &)v.validationErrorKeys()) {
        QString msg = v.errorMessage(key);
        setFlash(prefix + key, msg);
    }
}


void TActionController::sendTextToWebSocket(int sid, const QString &text)
{
    QVariantList info;
    info << sid << text;
    _taskList << qMakePair((int)SendTextTo, QVariant(info));
}


void TActionController::sendBinaryToWebSocket(int sid, const QByteArray &binary)
{
    QVariantList info;
    info << sid << binary;
    _taskList << qMakePair((int)SendBinaryTo, QVariant(info));
}


void TActionController::closeWebSokcet(int sid, int closeCode)
{
    QVariantList info;
    info << sid << closeCode;
    _taskList << qMakePair((int)SendCloseTo, QVariant(info));
}


void TActionController::publish(const QString &topic, const QString &text)
{
    QVariantList info;
    info << topic << text;
    _taskList << qMakePair((int)PublishText, QVariant(info));
}


void TActionController::publish(const QString &topic, const QByteArray &binary)
{
    QVariantList info;
    info << topic << binary;
    _taskList << qMakePair((int)PublishBinary, QVariant(info));
}


/*!
  \fn const TSession &TActionController::session() const;

  Returns the current HTTP session, allows associating information
  with individual visitors.
  \sa setSession(), sessionEnabled()
*/

/*!
  \fn TSession &TActionController::session();

  Returns the current HTTP session, allows associating information
  with individual visitors.
  \sa setSession(), sessionEnabled()
*/

/*!
  \fn virtual bool TActionController::sessionEnabled() const

  Must be overridden by subclasses to enable a HTTP session. The
  function must return \a false to disable a session. This function
  returns \a true.
  \sa session()
*/

/*!
  \fn virtual bool TActionController::csrfProtectionEnabled() const;

  Must be overridden by subclasses to disable CSRF protection. The
  function must return \a false to disable the protection. This function
  returns \a true.
  \sa exceptionActionsOfCsrfProtection()
*/

/*!
  \fn virtual QStringList TActionController::exceptionActionsOfCsrfProtection() const;

  Must be overridden by subclasses to return a string list of actions
  excluded from CSRF protection when the protection is enabled.
  \sa csrfProtectionEnabled()
*/

/*!
  \fn virtual bool TActionController::transactionEnabled() const;

  Must be overridden by subclasses to disable transaction mechanism.
  The function must return \a false to disable the mechanism. This function
  returns \a true.
*/

/*!
  \fn void TActionController::setLayoutEnabled(bool enable);

  Enables the layout mechanism if \a enable is true, otherwise disables it.
  By default the layout mechanism is enabled, and a HTML response is
  generated with using the current layout.
  \sa layoutEnabled()
*/

/*!
  \fn void TActionController::setLayoutDisabled(bool disable);

  Disables the layout mechanism if \a disable is true, otherwise enables it.
  By default the layout mechanism is enabled, and a HTML response is
  generated with using the current layout.
  \sa layoutEnabled()
*/

/*!
  \fn QString TActionController::layout() const

  Returns the name of the layout template.
  \sa setLayout()
 */

/*!
  \fn bool TActionController::layoutEnabled() const;

  Returns \a true if the layout mechanism is enabled; otherwise
  returns \a false.
  \sa setLayoutEnabled()
*/

/*!
  \fn QString TActionController::flash(const QString &name) const

  Returns the flash message for \a name.
  \sa setFlash()
*/

/*!
  \fn void TActionController::setStatusCode(int code);

  Sets the status code to \a code.
  \sa statusCode()
*/

/*!
  \fn int TActionController::statusCode() const

  Returns the status code of the HTTP response to be sent.
  \sa setStatusCode()
*/

/*!
  \fn bool TActionController::preFilter()

  This function is called before actions on the controller are performed,
  therefore can be overridden by subclasses (controllers) to filter a HTTP
  request. If the function returns false, a action on the controller is not
  executed.
*/

/*!
  \fn void TActionController::postFilter()

  This function is called after actions on the controller are performed.
  Can be overridden by subclasses (controllers) in order to post-processing
  of actions on the controller.
*/

/*!
  \fn void TActionController::rollbackTransaction()

  This function is called to rollback a transaction on the database.
*/

/*!
  \fn bool TActionController::rollbackRequested() const

  Returns \a true if a controller called rollbackTransaction function. Internal use.
*/

/*!
  \fn QByteArray TActionController::contentType() const

  Returns the content type for a response message.
*/

/*!
  \fn void TActionController::setContentType(const QByteArray &type)

  Sets the content type specified by \a type for a response message.
*/

/*!
 \fn virtual void TActionController::setAccessRules()

 Sets rules of access to this controller.
 @sa validateAccess(), TAccessValidator
*/
