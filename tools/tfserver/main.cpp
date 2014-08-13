/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QTextCodec>
#include <QStringList>
#include <QMap>
#include <TWebApplication>
#include <TThreadApplicationServer>
#include <TPreforkApplicationServer>
#include <TMultiplexingServer>
#include <TSystemGlobal>
#include <stdlib.h>
#include "tsystemglobal.h"
#include "signalhandler.h"
using namespace TreeFrog;

#define CTRL_C_OPTION     "--ctrlc-enable"
#define DEBUG_MODE_OPTION "--debug"
#define SOCKET_OPTION     "-s"


#if QT_VERSION >= 0x050000
static void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    QByteArray msg = message.toLocal8Bit();
    switch (type) {
    case QtFatalMsg:
    case QtCriticalMsg:
        tSystemError("%s (%s:%u %s)", msg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        tSystemWarn("%s (%s:%u %s)", msg.constData(), context.file, context.line, context.function);
        break;
    case QtDebugMsg:
        tSystemDebug("%s (%s:%u %s)", msg.constData(), context.file, context.line, context.function);
        break;
    default:
        break;
    }
}
#else
static void messageOutput(QtMsgType type, const char *msg)
{
    switch (type) {
    case QtFatalMsg:
    case QtCriticalMsg:
        tSystemError("%s", msg);
        break;
    case QtWarningMsg:
        tSystemWarn("%s", msg);
        break;
    case QtDebugMsg:
        tSystemDebug("%s", msg);
        break;
    default:
        break;
    }
}
#endif // QT_VERSION >= 0x050000

#if defined(Q_OS_UNIX)
static void writeFailure(const void *data, int size)
{
    tSystemError("%s", QByteArray((const char *)data, size).data());
}
#endif

static QMap<QString, QString> convertArgs(const QStringList &args)
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


int main(int argc, char *argv[])
{
    TWebApplication webapp(argc, argv);
    TApplicationServerBase *server = 0;
    int ret = -1;

    // Setup loggers
    tSetupSystemLogger();
    tSetupAccessLogger();
    tSetupQueryLogger();
    tSetupAppLoggers();

#if QT_VERSION >= 0x050000
    qInstallMessageHandler(messageOutput);
#else
    qInstallMsgHandler(messageOutput);
#endif
    QMap<QString, QString> args = convertArgs(QCoreApplication::arguments());
    int sock = args.value(SOCKET_OPTION).toInt();

#if defined(Q_OS_UNIX)
    webapp.watchUnixSignal(SIGTERM);
    if (!args.contains(CTRL_C_OPTION)) {
        webapp.ignoreUnixSignal(SIGINT);
    }

    // Setup signal handlers for SIGSEGV, SIGILL, SIGFPE, SIGABRT and SIGBUS
    setupFailureWriter(writeFailure);
    setupSignalHandler();

#elif defined(Q_OS_WIN)
    if (!args.contains(CTRL_C_OPTION)) {
        webapp.ignoreConsoleSignal();
    }
#endif

    // Sets the app locale
    QString loc = webapp.appSettings().value("Locale").toString().trimmed();
    if (!loc.isEmpty()) {
        QLocale locale(loc);
        QLocale::setDefault(locale);
        tSystemInfo("Application's default locale: %s", qPrintable(locale.name()));
    }

    // Sets codec
    QTextCodec *codec = webapp.codecForInternal();
    QTextCodec::setCodecForLocale(codec);

#if QT_VERSION < 0x050000
    QTextCodec::setCodecForTr(codec);
    QTextCodec::setCodecForCStrings(codec);
    tSystemDebug("setCodecForTr: %s", codec->name().data());
    tSystemDebug("setCodecForCStrings: %s", codec->name().data());
#endif

    if (!webapp.webRootExists()) {
        tSystemError("No such directory");
        goto finish;
    }
    tSystemDebug("Web Root: %s", qPrintable(webapp.webRootPath()));

    if (!webapp.appSettingsFileExists()) {
        tSystemError("Settings file not found");
        goto finish;
    }

#ifdef Q_OS_WIN
    if (sock <= 0)
#else
    if (sock <= 0 && args.contains(DEBUG_MODE_OPTION))
#endif
    {
        int port = webapp.appSettings().value("ListenPort").toInt();
        if (port <= 0 || port > USHRT_MAX) {
            tSystemError("Invalid port number: %d", port);
            goto finish;
        }
        TApplicationServerBase::nativeSocketInit();
        sock = TApplicationServerBase::nativeListen(QHostAddress::Any, port);
    }

    if (sock <= 0) {
        tSystemError("Invalid socket descriptor: %d", sock);
        goto finish;
    }

    switch (webapp.multiProcessingModule()) {
    case TWebApplication::Thread:
        server = new TThreadApplicationServer(sock, &webapp);
        break;

    case TWebApplication::Prefork: {
        TPreforkApplicationServer *svr = new TPreforkApplicationServer(&webapp);
        if (svr->setSocketDescriptor(sock)) {
            tSystemDebug("Set socket descriptor: %d", sock);
        } else {
            tSystemError("Failed to set socket descriptor: %d", sock);
            goto finish;
        }
        server = svr;
        break; }

    case TWebApplication::Hybrid:
#ifdef Q_OS_LINUX
        // Sets a listening socket descriptor
        TMultiplexingServer::instantiate(sock);
        tSystemDebug("Set socket descriptor: %d", sock);
        server = TMultiplexingServer::instance();
#else
        tFatal("Unsupported MPM: hybrid");
#endif
        break;

    default:
        break;
    }

    if (!server->start()) {
        tSystemError("Server open failed");
        goto finish;
    }

    ret = webapp.exec();
    server->stop();

finish:
    // Release loggers
    tReleaseAppLoggers();
    tReleaseQueryLogger();
    tReleaseAccessLogger();
    tReleaseSystemLogger();

    _exit(ret);
    return ret;
}
