/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QtCore>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QHostAddress>
#include <QSet>
#include <TActionContext>
#include <TWebApplication>
#include <THttpRequest>
#include <THttpResponse>
#include <THttpUtility>
#include <TDispatcher>
#include <TActionController>
#include <TSessionStore>
#include "tsqldatabasepool2.h"
#include "tkvsdatabasepool2.h"
#include "tsystemglobal.h"
#include "thttpsocket.h"
#include "tsessionmanager.h"
#include "turlroute.h"
#ifdef Q_OS_UNIX
# include "tfcore_unix.h"
#endif

#define AUTO_ID_REGENERATION  "Session.AutoIdRegeneration"
#define DIRECT_VIEW_RENDER_MODE  "DirectViewRenderMode"
#define ENABLE_CSRF_PROTECTION_MODULE "EnableCsrfProtectionModule"
#define SESSION_COOKIE_PATH  "Session.CookiePath"
#define LISTEN_PORT  "ListenPort"

/*!
  \class TActionContext
  \brief The TActionContext class is the base class of contexts for
  action controllers.
*/

TActionContext::TActionContext()
    : transactions(),
      sqlDatabases(),
      kvsDatabases(),
      stopped(false),
      socketDesc(0),
      currController(0),
      httpReq(0)
{ }


TActionContext::~TActionContext()
{
    release();
}


QSqlDatabase &TActionContext::getSqlDatabase(int id)
{
    T_TRACEFUNC("id:%d", id);

    if (!Tf::app()->isSqlDatabaseAvailable()) {
        return sqlDatabases[0];  // invalid database
    }

    if (id < 0 || id >= Tf::app()->sqlDatabaseSettingsCount()) {
        throw RuntimeException("error database id", __FILE__, __LINE__);
    }

    QSqlDatabase &db = sqlDatabases[id];
    if (!db.isValid()) {
        db = TSqlDatabasePool2::instance()->database(id);
        beginTransaction(db);
    }
    return db;
}


void TActionContext::releaseSqlDatabases()
{
    rollbackTransactions();

    for (QMap<int, QSqlDatabase>::iterator it = sqlDatabases.begin(); it != sqlDatabases.end(); ++it) {
        TSqlDatabasePool2::instance()->pool(it.value());
    }
    sqlDatabases.clear();
}


TKvsDatabase &TActionContext::getKvsDatabase(TKvsDatabase::Type type)
{
    T_TRACEFUNC("type:%d", (int)type);

    TKvsDatabase &db = kvsDatabases[(int)type];
    if (!db.isValid()) {
        db = TKvsDatabasePool2::instance()->database(type);
    }
    return db;
}


void TActionContext::releaseKvsDatabases()
{
    for (QMap<int, TKvsDatabase>::iterator it = kvsDatabases.begin(); it != kvsDatabases.end(); ++it) {
        TKvsDatabasePool2::instance()->pool(it.value());
    }
    kvsDatabases.clear();
}


void TActionContext::execute(THttpRequest &request)
{
    T_TRACEFUNC("");

    THttpResponseHeader responseHeader;
    accessLogger.open();

    try {
        httpReq = &request;
        const THttpRequestHeader &hdr = httpReq->header();

        // Access log
        accessLogger.setTimestamp(Tf::currentDateTimeSec());
        QByteArray firstLine = hdr.method() + ' ' + hdr.path();
        firstLine += QString(" HTTP/%1.%2").arg(hdr.majorVersion()).arg(hdr.minorVersion()).toLatin1();
        accessLogger.setRequest(firstLine);
        accessLogger.setRemoteHost( (Tf::app()->appSettings().value(LISTEN_PORT).toUInt() > 0) ? clientAddress().toString().toLatin1() : QByteArray("(unix)") );

        tSystemDebug("method : %s", hdr.method().data());
        tSystemDebug("path : %s", hdr.path().data());

        Tf::HttpMethod method = httpReq->method();

        //Make sure we have a valid HTTP method.
        if (method == Tf::Invalid)
        {
            int bytes = writeResponse(Tf::MethodNotAllowed, responseHeader);
            accessLogger.setResponseBytes( bytes );
            accessLogger.setStatusCode( responseHeader.statusCode() );
        }
        else
        {
            QString path = THttpUtility::fromUrlEncoding(hdr.path().mid(0, hdr.path().indexOf('?')));
            if (!path.endsWith('/')) path+= QLatin1Char('/');

            // Routing info exists?
            TRouting rt = TUrlRoute::instance().findRouting(method, path);
            tSystemDebug("Routing: controller:%s  action:%s", rt.controller.data(),
                         rt.action.data());

            if (rt.isEmpty()) {
                // Default URL routing
                rt.params = path.split('/', QString::SkipEmptyParts);

                // Direct view render mode?
                if (Tf::app()->appSettings().value(DIRECT_VIEW_RENDER_MODE).toBool()) {
                    // Direct view setting
                    rt.controller = "directcontroller";
                    rt.action = "show";
                } else {
                    if (!rt.params.value(0).isEmpty()) {
                        rt.controller = rt.params.takeFirst().toLower().toLatin1() + "controller";

                        if (rt.controller == "applicationcontroller") {
                            rt.controller.clear();  // Can not call 'ApplicationController'
                        }

                        // Default action: index
                        rt.action = rt.params.value(0, QLatin1String("index")).toLatin1();
                        if (!rt.params.isEmpty()) {
                            rt.params.takeFirst();
                        }
                    }
                    tSystemDebug("Active Controller : %s", rt.controller.data());
                }
            }

            // Call controller method
            TDispatcher<TActionController> ctlrDispatcher(rt.controller);
            currController = ctlrDispatcher.object();
            if (currController) {
                currController->setActionName(rt.action);

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
                if (Tf::app()->appSettings().value(ENABLE_CSRF_PROTECTION_MODULE, true).toBool()
                    && currController->csrfProtectionEnabled() && !currController->exceptionActionsOfCsrfProtection().contains(rt.action)) {

                    if (method == Tf::Post || method == Tf::Put || method == Tf::Delete) {
                        if (!currController->verifyRequest(*httpReq)) {
                            throw SecurityException("Invalid authenticity token", __FILE__, __LINE__);
                        }
                    }
                }

                if (currController->sessionEnabled()) {
                    if (currController->session().id().isEmpty() || Tf::app()->appSettings().value(AUTO_ID_REGENERATION).toBool()) {
                        TSessionManager::instance().remove(currController->session().sessionId); // Removes the old session
                        // Re-generate session ID
                        currController->session().sessionId = TSessionManager::instance().generateId();
                        tSystemDebug("Re-generate session ID: %s", currController->session().sessionId.data());
                    }
                    // Sets CSRF protection informaion
                    TActionController::setCsrfProtectionInto(currController->session());
                }

                // Database Transaction
                transactions.setEnabled(currController->transactionEnabled());

                // Do filters
                if (currController->preFilter()) {

                    // Dispathes
                    bool dispatched = ctlrDispatcher.invoke(rt.action, rt.params);
                    if (dispatched) {
                        autoRemoveFiles << currController->autoRemoveFiles;  // Adds auto-remove files

                        // Post fileter
                        currController->postFilter();

                        if (currController->rollbackRequested()) {
                            rollbackTransactions();
                        } else {
                            // Commits a transaction to the database
                            commitTransactions();
                        }

                        // Session store
                        if (currController->sessionEnabled()) {
                            bool stored = TSessionManager::instance().store(currController->session());
                            if (stored) {
                                QDateTime expire;
                                if (TSessionManager::sessionLifeTime() > 0) {
                                    expire = Tf::currentDateTimeSec().addSecs(TSessionManager::sessionLifeTime());
                                }

                                // Sets the path in the session cookie
                                QString cookiePath = Tf::app()->appSettings().value(SESSION_COOKIE_PATH).toString();
                                currController->addCookie(TSession::sessionName(), currController->session().id(), expire, cookiePath);
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
                accessLogger.setStatusCode( (!currController->response.isBodyNull()) ? currController->statusCode() : Tf::InternalServerError );
                currController->response.header().setStatusLine(accessLogger.statusCode(), THttpUtility::getResponseReasonPhrase(accessLogger.statusCode()));

                // Writes a response and access log
                int bytes = writeResponse(currController->response.header(), currController->response.bodyIODevice(),
                                          currController->response.bodyLength());
                accessLogger.setResponseBytes(bytes);

                // Session GC
                TSessionManager::instance().collectGarbage();

            } else {
                //Controller not found.
                //Check it's a static file

                switch(method)
                {
                    case Tf::Get:
                    {
                        path.remove(0, 1);
                        QFile reqPath(Tf::app()->publicPath() + path);
                        QFileInfo fi(reqPath);

                        if (fi.isFile() && fi.isReadable()) {
                            // Check "If-Modified-Since" header for caching
                            bool sendfile = true;
                            QByteArray ifModifiedSince = hdr.rawHeader("If-Modified-Since");

                            if (!ifModifiedSince.isEmpty()) {
                                QDateTime dt = THttpUtility::fromHttpDateTimeString(ifModifiedSince);
                                sendfile = (!dt.isValid() || dt != fi.lastModified());
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
                            int bytes = writeResponse(Tf::NotFound, responseHeader);
                            accessLogger.setResponseBytes( bytes );
                        }
                    }
                    break;

                    case Tf::Post: //Handle upload maybe?
                    default:
                    {
                        int bytes = writeResponse(Tf::NotFound, responseHeader);
                        accessLogger.setResponseBytes( bytes );
                    }
                }

                accessLogger.setStatusCode( responseHeader.statusCode() );
            }
        }

    } catch (ClientErrorException &e) {
        tWarn("Caught ClientErrorException: status code:%d", e.statusCode());
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
    // Releases all SQL database sessions
    TActionContext::releaseSqlDatabases();

    // Releases all KVS database sessions
    TActionContext::releaseKvsDatabases();

    for (QListIterator<TTemporaryFile *> i(tempFiles); i.hasNext(); ) {
        delete i.next();
    }
    tempFiles.clear();

    for (QStringListIterator i(autoRemoveFiles); i.hasNext(); ) {
        QFile(i.next()).remove();
    }
    autoRemoveFiles.clear();
}


qint64 TActionContext::writeResponse(int statusCode, THttpResponseHeader &header)
{
    T_TRACEFUNC("statusCode:%d", statusCode);
    QByteArray body;
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
    if (!contentType.isEmpty())
        header.setContentType(contentType);

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


bool TActionContext::beginTransaction(QSqlDatabase &database)
{
    bool ret = true;
    if (database.driver()->hasFeature(QSqlDriver::Transactions)) {
        ret = transactions.begin(database);
    }
    return ret;
}


void TActionContext::commitTransactions()
{
    transactions.commit();
}


void TActionContext::rollbackTransactions()
{
    transactions.rollback();
}


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
