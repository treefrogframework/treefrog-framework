/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMetaMethod>
#include <QTextCodec>
#include <QCryptographicHash>
#include <TActionController>
#include <TWebApplication>
#include <TDispatcher>
#include <TActionView>
#include <TSession>
#include <TAbstractUser>
#include <TActionContext>
#include <TFormValidator>
#include "tsessionmanager.h"
#include "ttextview.h"

#define STORE_TYPE              "Session.StoreType"
#define FLASH_VARS_SESSION_KEY  "_flashVariants"
#define LOGIN_USER_NAME_KEY     "_loginUserName"
#define CSRF_PROTECTION_KEY     "Session.CsrfProtectionKey"

/*!
  \class TActionController
  \~english
  \brief The TActionController class is the base class of all action
  controllers.

  \~japanese
  \brief TActionControllerはアクションコントローラのベースとなるクラス
*/

/*!
  \~english
  \brief Constructor.

  \~japanese
  \brief コンストラクタ
 */
TActionController::TActionController()
    : QObject(),
      TAbstractController(),
      statCode(200),
      rendered(false),
      layoutEnable(true),
      rollback(false)
{
    // Default content type
    setContentType("text/html");
}

/*!
  \fn TActionController::~TActionController();
  \~english
  \brief Destructor.
  
  \~japanese
  \brief デストラクタ
*/

/*!
  \~english
  Returns the controller name.
  
  \~japanese
  コントローラ名を返す
*/
QString TActionController::name() const
{
    if (ctrlName.isEmpty()) {
        ctrlName = className().remove(QRegExp("Controller$"));
    }
    return ctrlName;
}


/*!
  \fn QString TActionController::className() const
  \~english
  Returns the class name.

  \~japanese
  クラス名を返す
*/

/*!
  \fn QString TActionController::activeAction() const
  \~english
  Returns the active action name.
  
  \~japanese
  アクティブなアクション名を返す
*/

/*!
  \fn const THttpRequest &TActionController::httpRequest() const;
  \~english
  Returns the HTTP request being executed.

  \~japanese
  HTTPリクエストへの参照を返す
*/

/*!
  \fn THttpRequest &TActionController::httpRequest();
  \~english
  Returns the HTTP request being executed.

  \~japanese
  HTTPリクエストへの参照を返す
*/

/*!
  \fn const THttpResponse &TActionController::httpResponse() const;
  \~english
  Returns a HTTP response to be sent.

  \~japanese
  送信するHTTPレスポンスへの参照を返す
*/

/*!
  \fn THttpResponse &TActionController::httpResponse();
  \~english
  Returns a HTTP response to be sent.

  \~japanese
  送信するHHTTPレスポンスへの参照を返す
*/

/*!
  \~english
  Sets the layout template to \a layout.

  \~japanese
  レイアウトテンプレート\a layout を設定する 
  \~
  \sa layout()
 */
void TActionController::setLayout(const QString &layout)
{
    if (!layout.isNull()) {
        layoutName = layout;
    }
}

/*!
  \~english
  Adds the cookie to the internal list of cookies.

  \~japanese
  クッキーをHTTPレスポンスに追加する
 */
bool TActionController::addCookie(const TCookie &cookie)
{
    QByteArray name = cookie.name();
    if (name.isEmpty() || name.contains(';') || name.contains(',') || name.contains(' ') || name.contains('\"')) {
        tError("Invalid cookie name: %s", name.data());     
        return false;
    }

    cookieJar.addCookie(cookie);
    response.header().removeAllRawHeaders("Set-Cookie");
    for (QListIterator<TCookie> i(cookieJar.allCookies()); i.hasNext(); ) {
        response.header().addRawHeader("Set-Cookie", i.next().toRawForm());
    }
    return true;
}

/*!
  \~english
  Adds the cookie to the internal list of cookies.

  \~japanese
  クッキーをHTTPレスポンスに追加する
 */
bool TActionController::addCookie(const QByteArray &name, const QByteArray &value, const QDateTime &expire,
                                  const QString &path, const QString &domain, bool secure, bool httpOnly)
{
    TCookie cookie(name, value);
    cookie.setExpirationDate(expire);
    cookie.setPath(path);
    cookie.setDomain(domain);
    cookie.setSecure(secure);
    cookie.setHttpOnly(httpOnly);
    return addCookie(cookie);
}

/*!
  \~english
  Returns the authenticity token.

  \~japanese
  HTTPリクエストの正当性を検証するためのトークンを返す
 */
QByteArray TActionController::authenticityToken() const
{
    if (Tf::app()->appSettings().value(STORE_TYPE).toString().toLower() == QLatin1String("cookie")) {
        QString key = Tf::app()->appSettings().value(CSRF_PROTECTION_KEY).toString();
        QByteArray csrfId = session().value(key).toByteArray();

        if (csrfId.isEmpty()) {
            throw RuntimeException("CSRF protectionsession value is empty", __FILE__, __LINE__);
        }
        return csrfId;
    } else {
        return QCryptographicHash::hash(session().id() + Tf::app()->appSettings().value("Session.Secret").toByteArray(), QCryptographicHash::Sha1).toHex();
    }
}

/*!
  \~english
  Sets the HTTP session to \a session.

  \~japanese
  セッション \a session を設定する
 */
void TActionController::setSession(const TSession &session)
{
    T_TRACEFUNC("");
    sessionStore = session;
}

/*!
  \~english
  Sets CSRF protection informaion into \a session. Internal use.

  \~japanese
  CSRF対策の情報をセッションに設定する (内部使用)
*/
void TActionController::setCsrfProtectionInto(TSession &session)
{
    if (Tf::app()->appSettings().value(STORE_TYPE).toString().toLower() == QLatin1String("cookie")) {
        QString key = Tf::app()->appSettings().value(CSRF_PROTECTION_KEY).toString();
        session.insert(key, TSessionManager::instance().generateId());  // it's just a random value
    }
}

/*!
  \~english
  Verifies the HTTP request \a request.
  
  \~japanese
  HTTPリクエストを検証する
*/
bool TActionController::verifyRequest(const THttpRequest &request) const
{
    if (!csrfProtectionEnabled()) {
        return true;
    }

    if (Tf::app()->appSettings().value(STORE_TYPE).toString().toLower() != QLatin1String("cookie")) {
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
  \~english
  Renders the template of the action \a action with the layout \a layout.

  \~japanese
  レイアウト \a layout を適用し、アクション \a action のテンプレートを描画する
 */
bool TActionController::render(const QString &action, const QString &layout)
{
    T_TRACEFUNC("");

    if (rendered) {
        tWarn("Has rendered already: %s", qPrintable(className() + '#' + activeAction()));
        return false;
    }
    rendered = true;

    // Creates view-object and displays it
    TDispatcher<TActionView> viewDispatcher(viewClassName(action));
    setLayout(layout);
    response.setBody(renderView(viewDispatcher.object()));
    return !response.isBodyNull();
}


/*!
  \~english
  Renders the template given by \a templateName with the layout \a layout.
 
  \~japanese
  レイアウト \a layout を適用し、テンプレート \a templateName を描画する
*/
bool TActionController::renderTemplate(const QString &templateName, const QString &layout)
{
    T_TRACEFUNC("");

    if (rendered) {
        tWarn("Has rendered already: %s", qPrintable(className() + '#' + activeAction()));
        return false;
    }
    rendered = true;

    // Creates view-object and displays it
    QStringList names = templateName.split("/");
    if (names.count() != 2) {
        tError("Invalid patameter: %s", qPrintable(templateName));
        return false;
    }
    TDispatcher<TActionView> viewDispatcher(viewClassName(names[0], names[1]));
    setLayout(layout);
    response.setBody(renderView(viewDispatcher.object()));
    return (!response.isBodyNull());
}

/*!
  \~english
  Renders the text \a text with the layout \a layout.

  \~japanese
  レイアウト \a layout を適用し、テキストを描画する
*/
bool TActionController::renderText(const QString &text, bool layoutEnable, const QString &layout)
{
    T_TRACEFUNC("");

    if (rendered) {
        tWarn("Has rendered already: %s", qPrintable(className() + '#' + activeAction()));
        return false;
    }
    rendered = true;

    // Creates TTextView object and displays it
    setLayout(layout);
    setLayoutEnabled(layoutEnable);
    TTextView *view = new TTextView(text);
    response.setBody(renderView(view));
    delete view;
    return (!response.isBodyNull());
}


/*!
  \~english
  Returns the rendering data of the partial template given by \a templateName.

  \~japanese
  部分テンプレート \a templateName に変数 \a vars を設定した描画データを返す
*/
QString TActionController::getRenderingData(const QString &templateName, const QVariantHash &vars)
{
    T_TRACEFUNC("templateName: %s", qPrintable(templateName));

    // Creates view-object
    QStringList names = templateName.split("/");
    if (names.count() != 2) {
        tError("Invalid patameter: %s", qPrintable(templateName));
        return QString();
    }

    TDispatcher<TActionView> viewDispatcher(viewClassName(names[0], names[1]));
    TActionView *view = viewDispatcher.object();
    if (!view) {
        return QString();
    }

    QVariantHash hash = allVariants();
    for (QHashIterator<QString, QVariant> i(vars); i.hasNext(); ) {
        i.next();
        hash.insert(i.key(), i.value()); // item's value of same key is replaced
    }

    view->setController(this);
    view->setVariantHash(hash);
    return view->toString();  
}


/*!
  \~english
  Renders the \a view view.

  \~japanese
  ビューを描画する
*/
QByteArray TActionController::renderView(TActionView *view)
{
    T_TRACEFUNC("view: %p  layout: %s", view, qPrintable(layout()));

    if (!view) {
        tSystemError("view null pointer.  action:%s", qPrintable(activeAction()));
        return QByteArray();
    }
    view->setController(this);
    view->setVariantHash(allVariants());

    if (!layoutEnabled()) {
        // Renders without layout
        tSystemDebug("Renders without layout");
        return Tf::app()->codecForHttpOutput()->fromUnicode(view->toString());
    }
  
    // Displays with layout
    QString lay = (layout().isNull()) ? name().toLower() : layout().toLower();
    TDispatcher<TActionView> layoutDispatcher(layoutClassName(lay));
    TActionView *layoutView = layoutDispatcher.object();

    TDispatcher<TActionView> defLayoutDispatcher(layoutClassName("application"));
    if (!layoutView) {
        if (!layout().isNull()) {
            tSystemDebug("Not found layout: %s", qPrintable(layout()));
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
    layoutView->setVariantHash(allVariants());
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

    if (rendered) {
        tWarn("Has rendered already: %s", qPrintable(className() + '#' + activeAction()));
        return ret;
    }
    
    QString file = Tf::app()->publicPath() + QString::number(statusCode) + QLatin1String(".html");
    if (QFileInfo(file).exists())  {
        ret = sendFile(file, "text/html", "", false);
    } else {        
        response.setBody("");
    }
    setStatusCode(statusCode);
    rendered = true;
    return ret;
}

/*!
  \~english
  Returns the layout class name. Internal use.

  \~japanese
  レイアウトクラス名を返す
 */
QString TActionController::layoutClassName(const QString &layout)
{
    return QLatin1String("layouts_") + layout + "View";
}

/*!
  \~english
  Returns the class name of the partial view. Internal use.

  \~japanese
  部分ビューのクラス名を返す（内部使用）
 */
QString TActionController::partialViewClassName(const QString &partial)
{
    return QLatin1String("partial_") + partial + "View";
}

/*!
  \~english
  Redirects to the URL \a url.

  \~japanese
  URL \a url へリダイレクトする
 */
void TActionController::redirect(const QUrl &url, int statusCode)
{
    if (rendered) {
        tError("Unable to redirect. Has rendered already: %s", qPrintable(className() + '#' + activeAction()));
        return;
    }
    rendered = true;
    
    setStatusCode(statusCode);
    response.header().setRawHeader("Location", url.toEncoded());
    response.setBody(QByteArray("<html><body>redirected.</body></html>"));
    response.header().setContentType("text/html");
    
    // Enable flash-variants
    QVariant var;
    var.setValue(flashVars);
    sessionStore.insert(FLASH_VARS_SESSION_KEY, var);
}

/*!
  \~english
  Sends the file \a filePath as HTTP response.

  \~japanese
  HTTPレスポンスとして、ファイル \a filePath の内容を送信する
*/
bool TActionController::sendFile(const QString &filePath, const QByteArray &contentType, const QString &name, bool autoRemove)
{
    if (rendered) {
        tWarn("Has rendered already: %s", qPrintable(className() + '#' + activeAction()));
        return false;
    }
    rendered = true;

    setStatusCode(200);
    if (!name.isEmpty()) {
        QByteArray filename;
        filename += "attachment; filename=\"";
        filename += name.toUtf8();
        filename += '"';
        response.header().setRawHeader("Content-Disposition", filename);
    }

    response.setBodyFile(filePath);
    response.header().setContentType(contentType);

    if (autoRemove)
        setAutoRemove(filePath);

    return true;
}

/*!
  \~english
  Sends the data \a data as HTTP response.

  \~japanese
  HTTPレスポンスとして、データ \a data を送信する
*/
bool TActionController::sendData(const QByteArray &data, const QByteArray &contentType, const QString &name)
{
    if (rendered) {
        tWarn("Has rendered already: %s", qPrintable(className() + '#' + activeAction()));
        return false;
    }
    rendered = true;

    setStatusCode(200);

    if (!name.isEmpty()) {
        QByteArray filename;
        filename += "attachment; filename=\"";
        filename += name.toUtf8();
        filename += '"';
        response.header().setRawHeader("Content-Disposition", filename);
    }

    response.setBody(data);
    response.header().setContentType(contentType);
    return true;
}

/*!
  \~english
  Exports the all flash variants.
  
  \~japanese
  すべてのフラッシュオブジェクトをエクスポートする
*/
void TActionController::exportAllFlashVariants()
{
    QVariant var = sessionStore.take(FLASH_VARS_SESSION_KEY);
    if (!var.isNull()) {
        exportVariants(var.toHash());
    }
}

/*!
  \~english
  Logs the user \a user in to the system.
  
  This is a virtual function.
  \~japanese
  ユーザ \a user をシステムへログインさせる
  
  \~
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
        tSystemWarn("userLogin: Duplicate login detected. Force logout [user:%s]", qPrintable(identityKeyOfLoginUser()));
    }
    
    session().insert(LOGIN_USER_NAME_KEY, user->identityKey());
    return true;
}

/*!
  \~english
  Logs out of the system.
  
  This is a virtual function.
  \~japanese
  ユーザをログアウトさせる
  \~
  \sa userLogin()
*/
void TActionController::userLogout()
{
    session().take(LOGIN_USER_NAME_KEY);
}

/*!
  \~english
  Returns true if a user is logged in to the system; otherwise returns false.
  
  This is a virtual function.
  \~japanese
  ユーザがログインしている場合は true を返し、そうでない場合は false を返す
  \~
  \sa userLogin()
*/
bool TActionController::isUserLoggedIn() const
{
    return session().contains(LOGIN_USER_NAME_KEY);
}

/*!
  \~english
  Returns the identity key of the user, i.e., TAbstractUser object,
  logged in.
  
  This is a virtual function.
  \~japanese
  ログインユーザのアイデンティティキーを返す
  \~
  \sa userLogin()
*/
QString TActionController::identityKeyOfLoginUser() const
{
    return session().value(LOGIN_USER_NAME_KEY).toString();
}

/*!
  \~english
  Sets the automatically removing file.
  
  The file \a filePath is removed when the context is extinguished,
  after replied the HTTP response.

  \~japanese
  自動削除するファイルを設定する

  HTTPレスポンスを返却したあとコンテキストが消滅する際に、ファイル
  \a filePath が削除される
*/
void TActionController::setAutoRemove(const QString &filePath)
{
    if (!filePath.isEmpty() && !autoRemoveFiles.contains(filePath))
        autoRemoveFiles << filePath;
}

/*!
  \~english
  Returns the client address of the current session.

  \~japanese
  セッションをはっているクライアントのアドレスを返す
*/
QHostAddress TActionController::clientAddress() const
{
    return TActionContext::current()->clientAddress();
}

/*!
  \~english
  Sets the validation errors to flash variant.

  \~japanese
  バリデーションエラーとなったメッセージをフラッシュ変数へ設定する
*/
void TActionController::setFlashValidationErrors(const TFormValidator &v, const QString &prefix)
{
    for (QStringListIterator i(v.validationErrorKeys()); i.hasNext(); ) {
        const QString &key = i.next();
        QString msg = v.errorMessage(key);
        setFlash(prefix + key, msg);
    }
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

  Must be overridden by subclasses to disable a HTTP session. The
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
  \fn void TActionController::setFlash(const QString &name, const QVariant &value)

  Sets the flash message of \a name to \a value.
  \sa flash()
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
