/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tabstractwebsocket.h"
#include "thttpsocket.h"
#include "tpublisher.h"
#include "tsessionmanager.h"
#include "tsystemglobal.h"
#include "turlroute.h"
#include <QHostAddress>
#include <QSet>
#include <QtCore>
#include <TActionContext>
#include <TfCore>
#include <TActionController>
#include <TAppSettings>
#include <TDispatcher>
#include <THttpRequest>
#include <THttpResponse>
#include <THttpUtility>
#include <TSessionStore>
#include <TWebApplication>

/*!
  \class TActionContext
  \brief The TActionContext class is the base class of contexts for
  action controllers.
*/

TActionContext::TActionContext() :
    TDatabaseContext()
{ }


TActionContext::~TActionContext()
{
    release();
    accessLogger.close();
}


static bool directViewRenderMode()
{
    static const int mode = (int)Tf::appSettings()->value(Tf::DirectViewRenderMode).toBool();
    return (bool)mode;
}


void TActionContext::execute(THttpRequest &request)
{
    // App parameters
    static const qint64 LimitRequestBodyBytes = Tf::appSettings()->value(Tf::LimitRequestBody).toLongLong();
    static const uint ListenPort = Tf::appSettings()->value(Tf::ListenPort).toUInt();
    static const bool EnableCsrfProtectionModuleFlag = Tf::appSettings()->value(Tf::EnableCsrfProtectionModule).toBool();
    static const bool SessionAutoIdRegeneration = Tf::appSettings()->value(Tf::SessionAutoIdRegeneration).toBool();

    THttpResponseHeader responseHeader;

    try {
        _httpRequest = &request;
        const THttpRequestHeader &reqHeader = _httpRequest->header();

        // Access log
        if (Tf::isAccessLoggerAvailable()) {
            accessLogger.open();
            QByteArray firstLine;
            firstLine.reserve(200);
            firstLine += reqHeader.method();
            firstLine += ' ';
            firstLine += reqHeader.path();
            firstLine += QStringLiteral(" HTTP/%1.%2").arg(reqHeader.majorVersion()).arg(reqHeader.minorVersion()).toLatin1();
            accessLogger.setTimestamp(QDateTime::currentDateTime());
            accessLogger.setRequest(firstLine);
            accessLogger.setRemoteHost((ListenPort > 0) ? originatingClientAddress().toString().toLatin1() : QByteArrayLiteral("(unix)"));
            accessLogger.startElapsedTimer();
        }

        tSystemDebug("method : %s", reqHeader.method().data());
        tSystemDebug("path : %s", reqHeader.path().data());

        // HTTP method
        Tf::HttpMethod method = _httpRequest->method();
        QString path = THttpUtility::fromUrlEncoding(reqHeader.path().mid(0, reqHeader.path().indexOf('?')));

        if (LimitRequestBodyBytes > 0 && reqHeader.contentLength() > (uint)LimitRequestBodyBytes) {
            throw ClientErrorException(Tf::RequestEntityTooLarge, __FILE__, __LINE__);  // Request Entity Too Large
        }

        // Routing info exists?
        QStringList components = TUrlRoute::splitPath(path);
        TRouting route = TUrlRoute::instance().findRouting(method, components);

        tSystemDebug("Routing: controller:%s  action:%s", route.controller.data(),
            route.action.data());

        if (!route.exists) {
            // Default URL routing
            if (Q_UNLIKELY(directViewRenderMode())) {  // Direct view render mode?
                // Direct view setting
                route.setRouting(QByteArrayLiteral("directcontroller"), QByteArrayLiteral("show"), components);
            } else {
                QByteArray c = components.value(0).toLatin1().toLower();
                if (Q_LIKELY(!c.isEmpty())) {
                    if (Q_LIKELY(!TActionController::disabledControllers().contains(c))) {  // Can not call 'ApplicationController'
                        // Default action: "index"
                        QByteArray action = components.value(1, QStringLiteral("index")).toLatin1();
                        route.setRouting(c + QByteArrayLiteral("controller"), action, components.mid(2));
                    }
                }
                tSystemDebug("Active Controller : %s", route.controller.data());
            }
        }

        // Call controller method
        TDispatcher<TActionController> ctlrDispatcher(route.controller);
        _currController = ctlrDispatcher.object();
        if (_currController) {
            _currController->setActionName(route.action);
            _currController->setArguments(route.params);
            _currController->setContext(this);

            // Session
            if (_currController->sessionEnabled()) {
                TSession session;
                QByteArray sessionId = _httpRequest->cookie(TSession::sessionName());
                if (!sessionId.isEmpty()) {
                    // Finds a session
                    session = TSessionManager::instance().findSession(sessionId);
                }
                _currController->setSession(session);

                // Exports flash-variant
                _currController->exportAllFlashVariants();
            }

            // Verify authenticity token
            if (EnableCsrfProtectionModuleFlag && _currController->csrfProtectionEnabled() && !_currController->exceptionActionsOfCsrfProtection().contains(route.action)) {
                if (method == Tf::Post || method == Tf::Put || method == Tf::Patch || method == Tf::Delete) {
                    if (!_currController->verifyRequest(*_httpRequest)) {
                        throw SecurityException("Invalid authenticity token", __FILE__, __LINE__);
                    }
                }
            }

            if (_currController->sessionEnabled()) {
                if (SessionAutoIdRegeneration || _currController->session().id().isEmpty()) {
                    TSessionManager::instance().remove(_currController->session().sessionId);  // Removes the old session
                    // Re-generate session ID
                    _currController->session().sessionId = TSessionManager::instance().generateId();
                    tSystemDebug("Re-generate session ID: %s", _currController->session().sessionId.data());
                }

                if (EnableCsrfProtectionModuleFlag && _currController->csrfProtectionEnabled()) {
                    // Sets CSRF protection information
                    TActionController::setCsrfProtectionInto(_currController->session());
                }
            }

            // Database Transaction
            for (int databaseId = 0; databaseId < Tf::app()->sqlDatabaseSettingsCount(); ++databaseId) {
                setTransactionEnabled(_currController->transactionEnabled(), databaseId);
            }

            // Do filters
            if (Q_LIKELY(_currController->preFilter())) {
                // Dispatches
                ctlrDispatcher.invoke(route.action, route.params);
            }

            // Flushes response
            flushResponse(_currController, false);

            // Session
            if (_currController->sessionEnabled()) {
                // Session GC
                TSessionManager::instance().collectGarbage();

                // Commits a transaction for session
                commitTransactions();
            }

        } else {
            accessLogger.setStatusCode(Tf::BadRequest);  // Set a default status code
            if (route.controller.startsWith('/')) {
                path = route.controller;
            }

            if (Q_LIKELY(method == Tf::Get)) {  // GET Method
                QString canonicalPath = QUrl(QStringLiteral(".")).resolved(QUrl(path)).toString().mid(1);
                QFile reqPath(Tf::app()->publicPath() + canonicalPath);
                QFileInfo fi(reqPath);
                tSystemDebug("canonicalPath : %s", qUtf8Printable(canonicalPath));

                if (fi.isFile() && fi.isReadable()) {
                    // Check "If-Modified-Since" header for caching
                    bool sendfile = true;
                    QByteArray ifModifiedSince = reqHeader.rawHeader(QByteArrayLiteral("If-Modified-Since"));

                    if (!ifModifiedSince.isEmpty()) {
                        QDateTime dt = THttpUtility::fromHttpDateTimeString(ifModifiedSince);
                        if (dt.isValid()) {
                            sendfile = (dt.toMSecsSinceEpoch() / 1000 != fi.lastModified().toMSecsSinceEpoch() / 1000);
                        }
                    }

                    if (sendfile) {
                        // Sends a request file
                        responseHeader.setRawHeader(QByteArrayLiteral("Last-Modified"), THttpUtility::toHttpDateTimeString(fi.lastModified()));
                        QByteArray type = Tf::app()->internetMediaType(fi.suffix());
                        int bytes = writeResponse(Tf::OK, responseHeader, type, &reqPath, reqPath.size());
                        accessLogger.setResponseBytes(bytes);
                    } else {
                        // Not send the data
                        int bytes = writeResponse(Tf::NotModified, responseHeader);
                        accessLogger.setResponseBytes(bytes);
                    }
                } else {
                    if (!route.exists) {
                        int bytes = writeResponse(Tf::NotFound, responseHeader);
                        accessLogger.setResponseBytes(bytes);
                    } else {
                        // Routing not empty, redirect.
                        responseHeader.setRawHeader(QByteArrayLiteral("Location"), QUrl(path).toEncoded());
                        responseHeader.setContentType(QByteArrayLiteral("text/html"));
                        int bytes = writeResponse(Tf::Found, responseHeader);
                        accessLogger.setResponseBytes(bytes);
                    }
                }
                accessLogger.setStatusCode(responseHeader.statusCode());

            } else if (method == Tf::Post) {
                // file upload?
            } else {
                // HEAD, DELETE, ...
            }
        }

    } catch (ClientErrorException &e) {
        tWarn("Caught %s: status code:%d", qUtf8Printable(e.className()), e.statusCode());
        tSystemWarn("Caught %s: status code:%d", qUtf8Printable(e.className()), e.statusCode());
        int bytes = writeResponse(e.statusCode(), responseHeader);
        accessLogger.setResponseBytes(bytes);
        accessLogger.setStatusCode(e.statusCode());
    } catch (TfException &e) {
        tError("Caught %s: %s  [%s:%d]", qUtf8Printable(e.className()), qUtf8Printable(e.message()), qUtf8Printable(e.fileName()), e.lineNumber());
        tSystemError("Caught %s: %s  [%s:%d]", qUtf8Printable(e.className()), qUtf8Printable(e.message()), qUtf8Printable(e.fileName()), e.lineNumber());
        closeSocket();
        accessLogger.setResponseBytes(0);
        accessLogger.setStatusCode(Tf::InternalServerError);
    } catch (std::exception &e) {
        tError("Caught Exception: %s", e.what());
        tSystemError("Caught Exception: %s", e.what());
        closeSocket();
        accessLogger.setResponseBytes(0);
        accessLogger.setStatusCode(Tf::InternalServerError);
    }

    accessLogger.write();  // Writes access log
    _currController = nullptr;
}


void TActionContext::release()
{
    TDatabaseContext::release();

    for (auto temp : (const QList<TTemporaryFile *> &)_tempFiles) {
        delete temp;
    }
    _tempFiles.clear();

    for (auto &file : (const QStringList &)autoRemoveFiles) {
        QFile(file).remove();
    }
    autoRemoveFiles.clear();
}


void TActionContext::flushResponse(TActionController *controller, bool immediate)
{
    static const QString SessionCookiePath = Tf::appSettings()->value(Tf::SessionCookiePath).toString().trimmed();
    static const QString SessionCookieDomain = Tf::appSettings()->value(Tf::SessionCookieDomain).toString().trimmed();
    static const QByteArray SessionCookieSameSite = Tf::appSettings()->value(Tf::SessionCookieSameSite).toByteArray().trimmed();
    static const int SessionCookieMaxAge = ([]() -> int {
        QString maxagestr = Tf::appSettings()->value(Tf::SessionCookieMaxAge).toString().trimmed();
        return maxagestr.toInt();
    }());

    if (!controller || controller->_rendered != TActionController::RenderState::Rendered) {
        return;
    }

    autoRemoveFiles << controller->_autoRemoveFiles;  // Adds auto-remove files

    // Post filter
    controller->postFilter();

    if (Q_UNLIKELY(controller->rollbackRequested())) {
        rollbackTransactions();
    } else {
        // Commits a transaction to the database
        commitTransactions();
    }

    // Session store
    if (controller->sessionEnabled()) {
        bool stored = TSessionManager::instance().store(controller->session());
        if (Q_LIKELY(stored)) {
            controller->addCookie(TSession::sessionName(), controller->session().id(), SessionCookieMaxAge,
                SessionCookiePath, SessionCookieDomain, false, true, SessionCookieSameSite);

            // Commits a transaction for session
            commitTransactions();

        } else {
            tSystemError("Failed to store a session");
        }
    }

    // KVS pool
    releaseKvsDatabases();

    // WebSocket tasks
    if (Q_UNLIKELY(!controller->_taskList.isEmpty())) {
        QVariantList lst;
        for (auto &task : (const QList<QPair<int, QVariant>> &)controller->_taskList) {
            const QVariant &taskData = task.second;

            switch (task.first) {
            case TActionController::SendTextTo: {
                lst = taskData.toList();
                TAbstractWebSocket *websocket = TAbstractWebSocket::searchWebSocket(lst[0].toInt());
                if (websocket) {
                    websocket->sendText(lst[1].toString());
                }
            } break;

            case TActionController::SendBinaryTo: {
                lst = taskData.toList();
                TAbstractWebSocket *websocket = TAbstractWebSocket::searchWebSocket(lst[0].toInt());
                if (websocket) {
                    websocket->sendBinary(lst[1].toByteArray());
                }
            } break;

            case TActionController::SendCloseTo: {
                lst = taskData.toList();
                TAbstractWebSocket *websocket = TAbstractWebSocket::searchWebSocket(lst[0].toInt());
                if (websocket) {
                    websocket->sendClose(lst[1].toInt());
                }
            } break;

            case TActionController::PublishText: {
                lst = taskData.toList();
                QString topic = lst[0].toString();
                QString text = lst[1].toString();
                TPublisher::instance()->publish(topic, text, nullptr);
            } break;

            case TActionController::PublishBinary: {
                lst = taskData.toList();
                QString topic = lst[0].toString();
                QString binary = lst[1].toByteArray();
                TPublisher::instance()->publish(topic, binary, nullptr);
            } break;

            default:
                tSystemError("Invalid logic  [%s:%d]", __FILE__, __LINE__);
                break;
            }
        }
        controller->_taskList.clear();
    }

    // Sets charset to the content-type
    QByteArray ctype = controller->_response.header().contentType().toLower();
    if (ctype.startsWith("text") && !ctype.contains("charset")) {
        ctype += "; charset=";
#if QT_VERSION < 0x060000
        ctype += Tf::app()->codecForHttpOutput()->name();
#else
        ctype += QStringConverter::nameForEncoding(Tf::app()->encodingForHttpOutput());
#endif
        controller->_response.header().setContentType(ctype);
    }

    // Sets the default status code of HTTP response
    int bytes = 0;
    if (Q_UNLIKELY(controller->_response.isBodyNull())) {
        accessLogger.setStatusCode(Tf::NotFound);
        THttpResponseHeader header;
        bytes = writeResponse(Tf::NotFound, header);
    } else {
        accessLogger.setStatusCode(controller->statusCode());
        controller->_response.header().setStatusLine(controller->statusCode(), THttpUtility::getResponseReasonPhrase(controller->statusCode()));

        // Writes a response and access log
        qint64 bodyLength = (controller->_response.header().contentLength() > 0) ? controller->_response.header().contentLength() : controller->response().bodyLength();
        bytes = writeResponse(controller->_response.header(), controller->_response.bodyIODevice(), bodyLength);
    }
    accessLogger.setResponseBytes(bytes);

    if (immediate) {
        flushSocket();
        accessLogger.write();  // Writes access log
        accessLogger.close();
        closeSocket();
    }
}


qint64 TActionContext::writeResponse(int statusCode, THttpResponseHeader &header)
{
    QByteArray body;

    if (statusCode == Tf::NotModified) {
        return writeResponse(statusCode, header, QByteArray(), nullptr, 0);
    }
    if (statusCode >= 400) {
        QFile html(Tf::app()->publicPath() + QString::number(statusCode) + QLatin1String(".html"));
        if (html.exists() && html.open(QIODevice::ReadOnly)) {
            body = html.readAll();
            html.close();
        }
    }
    if (body.isEmpty()) {
        body.reserve(1024);
        body += "<html><body>";
        body += THttpUtility::getResponseReasonPhrase(statusCode);
        body += " (";
        body += QByteArray::number(statusCode);
        body += ")</body></html>";
    }

    QBuffer buf(&body);
    return writeResponse(statusCode, header, QByteArrayLiteral("text/html"), &buf, body.length());
}


qint64 TActionContext::writeResponse(int statusCode, THttpResponseHeader &header, const QByteArray &contentType, QIODevice *body, qint64 length)
{

    header.setStatusLine(statusCode, THttpUtility::getResponseReasonPhrase(statusCode));
    if (!contentType.isEmpty()) {
        header.setContentType(contentType);
    }

    return writeResponse(header, body, length);
}


qint64 TActionContext::writeResponse(THttpResponseHeader &header, QIODevice *body, qint64 length)
{

    header.setContentLength(length);
    tSystemDebug("content-length: %lld", (qint64)header.contentLength());
    header.setRawHeader(QByteArrayLiteral("Server"), QByteArrayLiteral("TreeFrog server"));
    header.setCurrentDate();

    // Write data
    return writeResponse(header, body);
}


void TActionContext::emitError(int)
{
}


TTemporaryFile &TActionContext::createTemporaryFile()
{
    TTemporaryFile *file = new TTemporaryFile();
    _tempFiles << file;
    return *file;
}


QHostAddress TActionContext::clientAddress() const
{
    return _httpRequest->clientAddress();
}


QHostAddress TActionContext::originatingClientAddress() const
{
    return _httpRequest->originatingClientAddress();
}

/*!
  Returns the keep-alive timeout in seconds.
 */
int TActionContext::keepAliveTimeout()
{
    static int keepAliveTimeout = []() {
        int timeout = Tf::appSettings()->value(Tf::HttpKeepAliveTimeout).toInt();
        return qMax(timeout, 0);
    }();
    return keepAliveTimeout;
}
