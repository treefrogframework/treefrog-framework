/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QtCore>
#include <QHostAddress>
#include <QSet>
#include <TActionContext>
#include <TWebApplication>
#include <TAppSettings>
#include <THttpRequest>
#include <THttpResponse>
#include <THttpUtility>
#include <TDispatcher>
#include <TActionController>
#include <TSessionStore>
#include "tsystemglobal.h"
#include "thttpsocket.h"
#include "tsessionmanager.h"
#include "turlroute.h"
#include "tabstractwebsocket.h"

/*!
  \class TActionContext
  \brief The TActionContext class is the base class of contexts for
  action controllers.
*/

TActionContext::TActionContext()
    : TDatabaseContext(),
      stopped(false),
      socketDesc(0),
      currController(nullptr),
      httpReq(nullptr)
{ }


TActionContext::~TActionContext()
{
    release();
}


static bool directViewRenderMode()
{
    static int mode = -1;
    if (mode < 0) {
        mode = (int)Tf::appSettings()->value(Tf::DirectViewRenderMode).toBool();
    }
    return (bool)mode;
}


void TActionContext::execute(THttpRequest &request, int sid)
{
    T_TRACEFUNC("");

    THttpResponseHeader responseHeader;
    accessLogger.open();

    try {
        httpReq = &request;
        const THttpRequestHeader &reqHeader = httpReq->header();

        // Access log
        accessLogger.setTimestamp(QDateTime::currentDateTime());
        QByteArray firstLine = reqHeader.method() + ' ' + reqHeader.path();
        firstLine += QString(" HTTP/%1.%2").arg(reqHeader.majorVersion()).arg(reqHeader.minorVersion()).toLatin1();
        accessLogger.setRequest(firstLine);
        accessLogger.setRemoteHost( (Tf::appSettings()->value(Tf::ListenPort).toUInt() > 0) ? clientAddress().toString().toLatin1() : QByteArray("(unix)") );

        tSystemDebug("method : %s", reqHeader.method().data());
        tSystemDebug("path : %s", reqHeader.path().data());

        // HTTP method
        Tf::HttpMethod method = httpReq->method();
        QString path = THttpUtility::fromUrlEncoding(reqHeader.path().mid(0, reqHeader.path().indexOf('?')));

        // Routing info exists?
        QStringList components = TUrlRoute::splitPath(path);
        TRouting route = TUrlRoute::instance().findRouting(method, components);

        tSystemDebug("Routing: controller:%s  action:%s", route.controller.data(),
                     route.action.data());

        if (! route.exists) {
            // Default URL routing

            if (Q_UNLIKELY(directViewRenderMode())) { // Direct view render mode?
                // Direct view setting
                route.setRouting("directcontroller", "show", components);
            } else {
                QByteArray c = components.value(0).toLatin1().toLower();
                if (Q_LIKELY(!c.isEmpty())) {
                    if (Q_LIKELY(!TActionController::disabledControllers().contains(c))) { // Can not call 'ApplicationController'
                        // Default action: "index"
                        QByteArray action = components.value(1, QLatin1String("index")).toLatin1();
                        route.setRouting(c + "controller", action, components.mid(2));
                    }
                }
                tSystemDebug("Active Controller : %s", route.controller.data());
            }
        }

        // Call controller method
        TDispatcher<TActionController> ctlrDispatcher(route.controller);
        currController = ctlrDispatcher.object();
        if (currController) {
            currController->setActionName(route.action);
            currController->setArguments(route.params);
            currController->setSocketId(sid);

            // Session
            if (currController->sessionEnabled()) {
                TSession session;
                QByteArray sessionId = httpReq->cookie(TSession::sessionName());
                if (!sessionId.isEmpty()) {
                    // Finds a session
                    session = TSessionManager::instance().findSession(sessionId);
                }
                currController->setSession(session);

                // Exports flash-variant
                currController->exportAllFlashVariants();
            }

            // Verify authenticity token
            if (Tf::appSettings()->value(Tf::EnableCsrfProtectionModule, true).toBool()
                && currController->csrfProtectionEnabled() && !currController->exceptionActionsOfCsrfProtection().contains(route.action)) {

                if (method == Tf::Post || method == Tf::Put || method == Tf::Patch || method == Tf::Delete) {
                    if (!currController->verifyRequest(*httpReq)) {
                        throw SecurityException("Invalid authenticity token", __FILE__, __LINE__);
                    }
                }
            }

            if (currController->sessionEnabled()) {
                if (currController->session().id().isEmpty() || Tf::appSettings()->value(Tf::SessionAutoIdRegeneration).toBool()) {
                    TSessionManager::instance().remove(currController->session().sessionId); // Removes the old session
                    // Re-generate session ID
                    currController->session().sessionId = TSessionManager::instance().generateId();
                    tSystemDebug("Re-generate session ID: %s", currController->session().sessionId.data());
                }
                // Sets CSRF protection informaion
                TActionController::setCsrfProtectionInto(currController->session());
            }

            // Database Transaction
            setTransactionEnabled(currController->transactionEnabled());

            // Do filters
            bool dispatched = false;
            if (Q_LIKELY(currController->preFilter())) {

                // Dispathes
                dispatched = ctlrDispatcher.invoke(route.action, route.params);
                if (Q_LIKELY(dispatched)) {
                    autoRemoveFiles << currController->autoRemoveFiles;  // Adds auto-remove files

                    // Post fileter
                    currController->postFilter();

                    if (Q_UNLIKELY(currController->rollbackRequested())) {
                        rollbackTransactions();
                    } else {
                        // Commits a transaction to the database
                        commitTransactions();
                    }

                    // Session store
                    if (currController->sessionEnabled()) {
                        bool stored = TSessionManager::instance().store(currController->session());
                        if (Q_LIKELY(stored)) {
                            static int cookielifetime = -1;
                            QDateTime expire;

                            if (cookielifetime < 0) {
                                cookielifetime = Tf::appSettings()->value(Tf::SessionLifeTime).toInt();
                            }
                            if (cookielifetime > 0) {
                                expire = QDateTime::currentDateTime().addSecs(cookielifetime);
                            }

                            // Sets the path in the session cookie
                            QString cookiePath = Tf::appSettings()->value(Tf::SessionCookiePath).toString();
                            currController->addCookie(TSession::sessionName(), currController->session().id(), expire, cookiePath, QString(), false, true);
                        } else {
                            tSystemError("Failed to store a session");
                        }
                    }

                    // WebSocket tasks
                    if (!currController->taskList.isEmpty()) {
                        QVariantList lst;
                        for (auto &task : (const QList<QPair<int, QVariant>>&)currController->taskList) {
                            const QVariant &taskData = task.second;

                            switch (task.first) {
                            case TActionController::SendTextTo: {
                                lst = taskData.toList();
                                TAbstractWebSocket *websocket = TAbstractWebSocket::searchWebSocket(lst[0].toInt());
                                if (websocket) {
                                    websocket->sendText(lst[1].toString());
                                }
                                break; }

                            case TActionController::SendBinaryTo: {
                                lst = taskData.toList();
                                TAbstractWebSocket *websocket = TAbstractWebSocket::searchWebSocket(lst[0].toInt());
                                if (websocket) {
                                    websocket->sendBinary(lst[1].toByteArray());
                                }
                                break; }

                            case TActionController::SendCloseTo: {
                                lst = taskData.toList();
                                TAbstractWebSocket *websocket = TAbstractWebSocket::searchWebSocket(lst[0].toInt());
                                if (websocket) {
                                    websocket->sendClose(lst[1].toInt());
                                }
                                break; }

                            default:
                                tSystemError("Invalid logic  [%s:%d]",  __FILE__, __LINE__);
                                break;
                            }
                        }
                    }
                }
            }

            // Sets charset to the content-type
            QByteArray ctype = currController->response.header().contentType().toLower();
            if (ctype.startsWith("text") && !ctype.contains("charset")) {
                ctype += "; charset=";
                ctype += Tf::app()->codecForHttpOutput()->name();
                currController->response.header().setContentType(ctype);
            }

            // Sets the default status code of HTTP response
            int bytes = 0;
            if (Q_UNLIKELY(currController->response.isBodyNull())) {
                accessLogger.setStatusCode((dispatched) ? Tf::InternalServerError : Tf::NotFound);
                bytes = writeResponse(accessLogger.statusCode(), responseHeader);
            } else {
                accessLogger.setStatusCode(currController->statusCode());
                currController->response.header().setStatusLine(currController->statusCode(), THttpUtility::getResponseReasonPhrase(currController->statusCode()));

                // Writes a response and access log
                qint64 bodyLength = (currController->response.header().contentLength() > 0) ? currController->response.header().contentLength() : currController->response.bodyLength();
                bytes = writeResponse(currController->response.header(), currController->response.bodyIODevice(),
                                      bodyLength);
            }
            accessLogger.setResponseBytes(bytes);

            // Session GC
            TSessionManager::instance().collectGarbage();

        } else {
            accessLogger.setStatusCode( Tf::BadRequest );  // Set a default status code
            if (route.controller.startsWith("/")) {
                path = route.controller;
            }

            if (Q_LIKELY(method == Tf::Get)) {  // GET Method
                QString canonicalPath = QUrl(".").resolved(QUrl(path)).toString().mid(1);
                QFile reqPath(Tf::app()->publicPath() + canonicalPath);
                QFileInfo fi(reqPath);
                tSystemDebug("canonicalPath : %s", qPrintable(canonicalPath));

                if (fi.isFile() && fi.isReadable()) {
                    // Check "If-Modified-Since" header for caching
                    bool sendfile = true;
                    QByteArray ifModifiedSince = reqHeader.rawHeader("If-Modified-Since");

                    if (!ifModifiedSince.isEmpty()) {
                        QDateTime dt = THttpUtility::fromHttpDateTimeString(ifModifiedSince);
                        if (dt.isValid()) {
                            sendfile = (dt.toMSecsSinceEpoch() / 1000 != fi.lastModified().toMSecsSinceEpoch() / 1000);
                        }
                    }

                    if (sendfile) {
                        // Sends a request file
                        responseHeader.setRawHeader("Last-Modified", THttpUtility::toHttpDateTimeString(fi.lastModified()));
                        QByteArray type = Tf::app()->internetMediaType(fi.suffix());
                        int bytes = writeResponse(Tf::OK, responseHeader, type, &reqPath, reqPath.size());
                        accessLogger.setResponseBytes( bytes );
                    } else {
                        // Not send the data
                        int bytes = writeResponse(Tf::NotModified, responseHeader);
                        accessLogger.setResponseBytes( bytes );
                    }
                } else {
                    if (! route.exists) {
                        int bytes = writeResponse(Tf::NotFound, responseHeader);
                        accessLogger.setResponseBytes( bytes );
                    } else {
                        // Routing not empty, redirect.
                        responseHeader.setRawHeader("Location", QUrl(path).toEncoded());
                        responseHeader.setContentType("text/html");
                        int bytes = writeResponse(Tf::Found, responseHeader);
                        accessLogger.setResponseBytes( bytes );
                    }
                }
                accessLogger.setStatusCode( responseHeader.statusCode() );

            } else if (method == Tf::Post) {
                // file upload?
            } else {
                // HEAD, DELETE, ...
            }
        }

    } catch (ClientErrorException &e) {
        tWarn("Caught ClientErrorException: status code:%d", e.statusCode());
        tSystemWarn("Caught ClientErrorException: status code:%d", e.statusCode());
        int bytes = writeResponse(e.statusCode(), responseHeader);
        accessLogger.setResponseBytes( bytes );
        accessLogger.setStatusCode( e.statusCode() );
    } catch (SqlException &e) {
        tError("Caught SqlException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
        tSystemError("Caught SqlException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
        closeHttpSocket();
    } catch (KvsException &e) {
        tError("Caught KvsException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
        tSystemError("Caught KvsException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
        closeHttpSocket();
    } catch (SecurityException &e) {
        tError("Caught SecurityException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
        tSystemError("Caught SecurityException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
        closeHttpSocket();
    } catch (RuntimeException &e) {
        tError("Caught RuntimeException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
        tSystemError("Caught RuntimeException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
        closeHttpSocket();
    } catch (StandardException &e) {
        tError("Caught StandardException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
        tSystemError("Caught StandardException: %s  [%s:%d]", qPrintable(e.message()), qPrintable(e.fileName()), e.lineNumber());
        closeHttpSocket();
    } catch (...) {
        tError("Caught Exception");
        tSystemError("Caught Exception");
        closeHttpSocket();
    }

    TActionContext::accessLogger.write();  // Writes access log
}


void TActionContext::release()
{
    TDatabaseContext::release();

    for (auto temp : (const QList<TTemporaryFile*> &)tempFiles) {
        delete temp;
    }
    tempFiles.clear();

    for (auto &file : (const QStringList&)autoRemoveFiles) {
        QFile(file).remove();
    }
    autoRemoveFiles.clear();
}


qint64 TActionContext::writeResponse(int statusCode, THttpResponseHeader &header)
{
    T_TRACEFUNC("statusCode:%d", statusCode);
    QByteArray body;

    if (statusCode == Tf::NotModified) {
        return writeResponse(statusCode, header, QByteArray(), nullptr, 0);
    }
    if (statusCode >= 400) {
        QFile html(Tf::app()->publicPath() + QString::number(statusCode) + ".html");
        if (html.exists() && html.open(QIODevice::ReadOnly)) {
            body = html.readAll();
            html.close();
        }
    }
    if (body.isEmpty()) {
        body  = "<html><body>";
        body += THttpUtility::getResponseReasonPhrase(statusCode);
        body += " (";
        body += QByteArray::number(statusCode);
        body += ")</body></html>";
    }

    QBuffer buf(&body);
    return writeResponse(statusCode, header, "text/html", &buf, body.length());
}


qint64 TActionContext::writeResponse(int statusCode, THttpResponseHeader &header, const QByteArray &contentType, QIODevice *body, qint64 length)
{
    T_TRACEFUNC("statusCode:%d  contentType:%s  length:%s", statusCode, contentType.data(), qPrintable(QString::number(length)));

    header.setStatusLine(statusCode, THttpUtility::getResponseReasonPhrase(statusCode));
    if (!contentType.isEmpty()) {
        header.setContentType(contentType);
    }

    return writeResponse(header, body, length);
}


qint64 TActionContext::writeResponse(THttpResponseHeader &header, QIODevice *body, qint64 length)
{
    T_TRACEFUNC("length:%s", qPrintable(QString::number(length)));

    header.setContentLength(length);
    header.setRawHeader("Server", "TreeFrog server");
    header.setCurrentDate();

    // Write data
    return writeResponse(header, body);
}


void TActionContext::emitError(int )
{ }


TTemporaryFile &TActionContext::createTemporaryFile()
{
    TTemporaryFile *file = new TTemporaryFile();
    tempFiles << file;
    return *file;
}


QHostAddress TActionContext::clientAddress() const
{
    return httpReq->clientAddress();
}
