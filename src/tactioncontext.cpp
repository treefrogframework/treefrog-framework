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
    static const int64_t LimitRequestBodyBytes = Tf::appSettings()->value(Tf::LimitRequestBody).toLongLong();
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

        tSystemDebug("method : {}", reqHeader.method().data());
        tSystemDebug("path : {}", reqHeader.path().data());

        // HTTP method
        Tf::HttpMethod method = _httpRequest->method();
        QString path = THttpUtility::fromUrlEncoding(reqHeader.path().mid(0, reqHeader.path().indexOf('?')));

        if (LimitRequestBodyBytes > 0 && reqHeader.contentLength() > (uint)LimitRequestBodyBytes) {
            throw ClientErrorException(Tf::RequestEntityTooLarge, __FILE__, __LINE__);  // Request Entity Too Large
        }

        // Routing info exists?
        QStringList components = TUrlRoute::splitPath(path);
        TRouting route = TUrlRoute::instance().findRouting(method, components);

        tSystemDebug("Routing: controller:{}  action:{}", (const char*)route.controller.data(),
            (const char*)route.action.data());

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
                tSystemDebug("Active Controller : {}", (const char*)route.controller.data());
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
                    tSystemDebug("Re-generate session ID: {}", (const char*)_currController->session().sessionId.data());
                }

                // Sets CSRF protection information
                TActionController::setCsrfProtectionInto(_currController->session());
            }

            // Database Transaction
            for (int databaseId = 0; databaseId < Tf::app()->sqlDatabaseSettingsCount(); ++databaseId) {
                setTransactionEnabled(_currController->transactionEnabled(), databaseId);
            }

            // Do filters
            if (Q_LIKELY(_currController->preFilter())) {
                // Dispatches
                bool inv = ctlrDispatcher.invoke(route.action, route.params);
                if (!inv) {
                    _currController->setStatusCode(Tf::NotFound);
                }
            }

            // Flushes response and sets access log
            flushResponse(_currController, false);

            // Session
            if (_currController->sessionEnabled()) {
                // Session GC
                TSessionManager::instance().collectGarbage();

                // Commits a transaction for session
                commitTransactions();
            }

        } else {
            // If no controller
            int responseBytes = 0;

            if (route.controller.startsWith('/')) {
                path = route.controller;
            }

            if (Q_LIKELY(method == Tf::Get)) {  // GET Method
                QString canonicalPath = QUrl(QStringLiteral(".")).resolved(QUrl(path)).toString().mid(1);
                QFile reqPath(Tf::app()->publicPath() + canonicalPath);
                QFileInfo fi(reqPath);
                tSystemDebug("canonicalPath : {}", canonicalPath);

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
                        responseBytes = writeResponse(Tf::OK, responseHeader, type, &reqPath, reqPath.size());
                    } else {
                        // Not send the data
                        responseBytes = writeResponse(Tf::NotModified, responseHeader);
                    }
                } else {
                    if (!route.exists) {
                        responseBytes = writeResponse(Tf::NotFound, responseHeader);
                    } else {
                        // Routing not empty, redirect.
                        responseHeader.setRawHeader(QByteArrayLiteral("Location"), QUrl(path).toEncoded());
                        responseHeader.setContentType(QByteArrayLiteral("text/html"));
                        responseBytes = writeResponse(Tf::Found, responseHeader);
                    }
                }

            } else if (method == Tf::Post) {
                responseBytes = writeResponse(Tf::BadRequest, responseHeader);
            } else {
                responseBytes = writeResponse(Tf::BadRequest, responseHeader);
            }

            accessLogger.setResponseBytes(responseBytes);
            accessLogger.setStatusCode(responseHeader.statusCode());
        }

    } catch (ClientErrorException &e) {
        Tf::warn("Caught {}: status code:{}", e.className(), e.statusCode());
        tSystemWarn("Caught {}: status code:{}", e.className(), e.statusCode());
        int responseBytes = writeResponse(e.statusCode(), responseHeader);
        accessLogger.setResponseBytes(responseBytes);
        accessLogger.setStatusCode(e.statusCode());
    } catch (TfException &e) {
        Tf::error("Caught {}: {}  [{}:{}]", e.className(), e.message(), e.fileName(), e.lineNumber());
        tSystemError("Caught {}: {}  [{}:{}]", e.className(), e.message(), e.fileName(), e.lineNumber());
        closeSocket();
        accessLogger.setResponseBytes(0);
        accessLogger.setStatusCode(Tf::InternalServerError);
    } catch (std::exception &e) {
        Tf::error("Caught Exception: {}", e.what());
        tSystemError("Caught Exception: {}", e.what());
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

    if (!controller) {
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
                tSystemError("Invalid logic  [{}:{}]", __FILE__, __LINE__);
                break;
            }
        }
        controller->_taskList.clear();
    }

    // Sets charset to the content-type
    QByteArray ctype = controller->_response.header().contentType().toLower();
    if (ctype.startsWith("text") && !ctype.contains("charset")) {
        ctype += "; charset=";
        ctype += QStringConverter::nameForEncoding(Tf::app()->encodingForHttpOutput());
        controller->_response.header().setContentType(ctype);
    }

    // Sets the default status code of HTTP response
    int responseBytes = 0;
    if (Q_UNLIKELY(controller->_response.isBodyNull())) {
        THttpResponseHeader header;
        responseBytes = writeResponse(Tf::NotFound, header);
        accessLogger.setStatusCode(header.statusCode());

    } else {
        controller->_response.header().setStatusLine(controller->statusCode(), THttpUtility::getResponseReasonPhrase(controller->statusCode()));

        // Writes a response and access log
        int64_t bodyLength = (controller->_response.header().contentLength() > 0) ? controller->_response.header().contentLength() : controller->response().bodyLength();
        responseBytes = writeResponse(controller->_response.header(), controller->_response.bodyIODevice(), bodyLength);
        accessLogger.setStatusCode(controller->statusCode());
    }
    accessLogger.setResponseBytes(responseBytes);

    if (immediate) {
        flushSocket();
        accessLogger.write();  // Writes access log
        accessLogger.close();
        closeSocket();
    }
}


int64_t TActionContext::writeResponse(int statusCode, THttpResponseHeader &header)
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


int64_t TActionContext::writeResponse(int statusCode, THttpResponseHeader &header, const QByteArray &contentType, QIODevice *body, int64_t length)
{

    header.setStatusLine(statusCode, THttpUtility::getResponseReasonPhrase(statusCode));
    if (!contentType.isEmpty()) {
        header.setContentType(contentType);
    }

    return writeResponse(header, body, length);
}


int64_t TActionContext::writeResponse(THttpResponseHeader &header, QIODevice *body, int64_t length)
{

    header.setContentLength(length);
    tSystemDebug("content-length: {}", (qint64)header.contentLength());
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
        return std::max(timeout, 0);
    }();
    return keepAliveTimeout;
}
