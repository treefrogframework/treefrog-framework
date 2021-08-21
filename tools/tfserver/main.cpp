/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "signalhandler.h"
#include "tdispatcher.h"
#include "thazardptrmanager.h"
#include "tsystemglobal.h"
#include <QMap>
#include <QStringList>
#include <QTextCodec>
#include <TActionController>
#include <TAppSettings>
#include <TJSLoader>
#include <TMultiplexingServer>
#include <TSystemGlobal>
#include <TThreadApplicationServer>
#include <TUrlRoute>
#include <TWebApplication>
#include <cstdlib>
#include <cstdio>
using namespace TreeFrog;

constexpr auto DEBUG_MODE_OPTION = "--debug";
constexpr auto SOCKET_OPTION = "-s";
constexpr auto AUTO_RELOAD_OPTION = "-r";
constexpr auto PORT_OPTION = "-p";
constexpr auto SHOW_ROUTES_OPTION = "--show-routes";

namespace {

void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    QByteArray msg = message.toLocal8Bit();
    switch (type) {
    case QtFatalMsg:
        tFatal("%s (%s:%u %s)", msg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        tError("%s (%s:%u %s)", msg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        tWarn("%s (%s:%u %s)", msg.constData(), context.file, context.line, context.function);
        break;
    case QtDebugMsg:
        tDebug("%s (%s:%u %s)", msg.constData(), context.file, context.line, context.function);
        break;
    default:
        break;
    }
}


#if defined(Q_OS_UNIX)
void writeFailure(const void *data, int size)
{
    tSystemError("%s", QByteArray((const char *)data, size).data());
}
#endif

QMap<QString, QString> convertArgs(const QStringList &args)
{
    QMap<QString, QString> map;
    for (int i = 1; i < args.count(); ++i) {
        if (args.value(i).startsWith('-')) {
            if (args.value(i + 1).startsWith('-')) {
                map.insert(args.value(i), QString());
            } else {
                map.insert(args.value(i), args.value(i + 1));
                ++i;
            }
        }
    }
    return map;
}

class MethodDefinition : public QMap<int, QByteArray> {
public:
    MethodDefinition() :
        QMap<int, QByteArray>()
    {
        insert(TRoute::Match, QByteArray("match   "));
        insert(TRoute::Get, QByteArray("get     "));
        insert(TRoute::Head, QByteArray("head    "));
        insert(TRoute::Post, QByteArray("post    "));
        insert(TRoute::Options, QByteArray("options "));
        insert(TRoute::Put, QByteArray("put     "));
        insert(TRoute::Delete, QByteArray("delete  "));
        insert(TRoute::Trace, QByteArray("trace   "));
    }
};
Q_GLOBAL_STATIC(MethodDefinition, methodDef)


QByteArray createMethodString(const QString &controllerName, const QMetaMethod &method)
{
    QString str;

    if (method.isValid()) {
        str = controllerName;
        str += ".";
        str += method.name();
        str += "(";
        if (method.parameterCount() > 0) {
            for (auto &param : method.parameterNames()) {
                str += param;
                str += ",";
            }
            str.chop(1);
        }
        str += ")";
    }
    return str.toLatin1();
}


void showRoutes()
{
    static QStringList excludes = {"applicationcontroller", "directcontroller"};

    bool res = TApplicationServerBase::loadLibraries();
    if (!res) {
        return;
    }

    auto routes = TUrlRoute::instance().allRoutes();
    if (!routes.isEmpty()) {
        std::printf("Available routes:\n");

        for (auto &route : routes) {
            QByteArray action;
            QString path = QLatin1String("/") + route.componentList.join("/");
            auto routing = TUrlRoute::instance().findRouting((Tf::HttpMethod)route.method, route.componentList);

            TDispatcher<TActionController> dispatcher(routing.controller);
            auto method = dispatcher.method(routing.action, 0);
            if (method.isValid()) {
                action = createMethodString(dispatcher.typeName(), method);
            } else if (routing.controller.startsWith("/")) {
                action = routing.controller;
            } else if (route.hasVariableParams && dispatcher.hasMethod(routing.action)) {
                action = routing.controller + "." + routing.action + "(...)";
            } else {
                action = QByteArrayLiteral("(not found)");
            }
            std::printf("  %s%s  ->  %s\n", methodDef()->value(route.method).data(), qUtf8Printable(path), qUtf8Printable(action));
        }
        std::printf("\n");
    }

    std::printf("Available controllers:\n");
    auto keys = Tf::objectFactories()->keys();
    std::sort(keys.begin(), keys.end());

    for (const auto &key : keys) {
        if (key.endsWith("controller") && !excludes.contains(key)) {
            auto ctrl = key.mid(0, key.length() - 10);
            TDispatcher<TActionController> ctlrDispatcher(key);
            const QMetaObject *metaObject = ctlrDispatcher.object()->metaObject();

            for (int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i) {
                auto metaMethod = metaObject->method(i);
                QByteArray api = "match   /";
                api += ctrl;
                api += "/";
                api += metaMethod.name();
                for (int i = 0; i < metaMethod.parameterCount(); i++) {
                    api += "/:param";
                }

                QByteArray action = createMethodString(ctlrDispatcher.typeName(), metaMethod);
                std::printf("  %s  ->  %s\n", api.data(), action.data());
            }
        }
    }
}

}

int main(int argc, char *argv[])
{
    TWebApplication webapp(argc, argv);
    TApplicationServerBase *server = nullptr;
    int ret = -1;

    // No stdout buffering
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    // Setup loggers
    Tf::setupSystemLogger();
    Tf::setupAccessLogger();
    Tf::setupQueryLogger();
    Tf::setupAppLoggers();

    // Setup hazard pointer
    THazardPtrManager::instance().setGarbageCollectionBufferSize(Tf::app()->maxNumberOfThreadsPerAppServer());

    qInstallMessageHandler(messageOutput);
    QMap<QString, QString> args = convertArgs(QCoreApplication::arguments());
    int sock = args.value(SOCKET_OPTION).toInt();
    bool reload = args.contains(AUTO_RELOAD_OPTION);
    bool debug = args.contains(DEBUG_MODE_OPTION);
    bool showRoutesOption = args.contains(SHOW_ROUTES_OPTION);
    ushort portNumber = args.value(PORT_OPTION).toUShort();

#if defined(Q_OS_UNIX)
    webapp.watchUnixSignal(SIGTERM);
    if (!debug) {
        webapp.ignoreUnixSignal(SIGINT);
    }

    // Setup signal handlers for SIGSEGV, SIGILL, SIGFPE, SIGABRT and SIGBUS
    setupFailureWriter(writeFailure);
    setupSignalHandler();

#elif defined(Q_OS_WIN)
    if (!debug) {
        webapp.ignoreConsoleSignal();
    }
#endif

    // Sets the app locale
    QString loc = Tf::appSettings()->value(Tf::Locale).toString().trimmed();
    if (!loc.isEmpty()) {
        QLocale locale(loc);
        QLocale::setDefault(locale);
        tSystemInfo("Application's default locale: %s", qUtf8Printable(locale.name()));
    }

    // Sets codec
    QTextCodec *codec = webapp.codecForInternal();
    QTextCodec::setCodecForLocale(codec);

    if (!webapp.webRootExists()) {
        tSystemError("No such directory");
        std::fprintf(stderr, "No such directory\n");
        goto finish;
    }
    tSystemDebug("Web Root: %s", qUtf8Printable(webapp.webRootPath()));

    if (!webapp.appSettingsFileExists()) {
        tSystemError("Settings file not found");
        std::fprintf(stderr, "Settings file not found\n");
        goto finish;
    } else {
        // Sets search paths for JavaScript
        QStringList jpaths = Tf::appSettings()->value(Tf::JavaScriptPath, "script;node_modules").toString().split(';');
        TJSLoader::setDefaultSearchPaths(jpaths);
    }

    if (showRoutesOption) {
        showRoutes();
        ret = 0;
        goto end;
    }

#ifdef Q_OS_WIN
    if (sock <= 0)
#else
    if (sock <= 0 && debug)
#endif
    {
        int port = (portNumber > 0) ? portNumber : Tf::appSettings()->value(Tf::ListenPort).toInt();
        if (port <= 0 || port > USHRT_MAX) {
            tSystemError("Invalid port number: %d", port);
            std::fprintf(stderr, "Invalid port number: %d\n", port);
            goto finish;
        }
        // Listen address
        QString listenAddress = Tf::appSettings()->value(Tf::ListenAddress).toString();
        if (listenAddress.isEmpty()) {
            listenAddress = "0.0.0.0";
        }

        TApplicationServerBase::nativeSocketInit();
        sock = TApplicationServerBase::nativeListen(QHostAddress(listenAddress), port);
    }

    if (sock <= 0) {
        tSystemError("Invalid socket descriptor: %d", sock);
        std::fprintf(stderr, "Invalid option\n");
        goto finish;
    }

    switch (webapp.multiProcessingModule()) {
    case TWebApplication::Thread:
        server = new TThreadApplicationServer(sock, &webapp);
        break;

    case TWebApplication::Epoll:
#ifdef Q_OS_LINUX
        // Sets a listening socket descriptor
        TMultiplexingServer::instantiate(sock);
        tSystemDebug("Set socket descriptor: %d", sock);
        server = TMultiplexingServer::instance();
#else
        tFatal("Unsupported MPM: epoll");
#endif
        break;

    default:
        break;
    }

    server->setAutoReloadingEnabled(reload);
    if (!server->start(debug)) {
        tSystemError("Server open failed");
        std::fprintf(stderr, "Server open failed\n");
        goto finish;
    }

    QObject::connect(&webapp, &QCoreApplication::aboutToQuit, [=]() { server->stop(); });
    ret = webapp.exec();

finish:
    switch (webapp.multiProcessingModule()) {
    case TWebApplication::Thread:
        delete server;
        break;

    case TWebApplication::Epoll:
        break;

    default:
        break;
    }

    // Release loggers
    Tf::releaseAppLoggers();
    Tf::releaseQueryLogger();
    Tf::releaseAccessLogger();
    Tf::releaseSystemLogger();

end:
    _exit(ret);
    return ret;
}
