/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QtCore>
#include <QSysInfo>
#include <QHostInfo>
#include <TWebApplication>
#include <TSystemGlobal>
#include "servermanager.h"
#include "processinfo.h"

#ifdef Q_OS_UNIX
# include <sys/utsname.h>
# include <tfcore_unix.h>
#else
# define TF_FLOCK(fd,op)
#endif

namespace TreeFrog {

#ifdef Q_OS_WIN
extern void WINAPI winServiceMain(DWORD argc, LPTSTR *argv);
#endif

#define PID_FILENAME  "treefrog.pid"

enum CommandOption {
    Invalid = 0,
    EnvironmentSpecified,
    SocketSpecified,
    PrintVersion,
    PrintUsage,
    ShowRunningAppList,
    DaemonMode,
    WindowsServiceMode,
    SendSignal,
};

typedef QHash<int, QString> VersionHash;

#ifdef Q_OS_WIN
Q_GLOBAL_STATIC_WITH_INITIALIZER(VersionHash, winVersion,
{
    x->insert(QSysInfo::WV_XP,       "Windows XP");
    x->insert(QSysInfo::WV_2003,     "Windows Server 2003");
    x->insert(QSysInfo::WV_VISTA,    "Windows Vista or Windows Server 2008");
    x->insert(QSysInfo::WV_WINDOWS7, "Windows 7 or Windows Server 2008 R2");
#if QT_VERSION >= 0x050000
    x->insert(QSysInfo::WV_WINDOWS8, "Windows 8");
#endif
})
#endif

#ifdef Q_OS_DARWIN
Q_GLOBAL_STATIC_WITH_INITIALIZER(VersionHash, macVersion,
{
    x->insert(QSysInfo::MV_10_3, "Mac OS X 10.3 Panther");
    x->insert(QSysInfo::MV_10_4, "Mac OS X 10.4 Tiger");
    x->insert(QSysInfo::MV_10_5, "Mac OS X 10.5 Leopard");
    x->insert(QSysInfo::MV_10_6, "Mac OS X 10.6 Snow Leopard");
#if QT_VERSION >= 0x040800
    x->insert(QSysInfo::MV_10_7, "Mac OS X 10.7 Lion");
    x->insert(QSysInfo::MV_10_8, "Mac OS X 10.8 Mountain Lion");
#if QT_VERSION >= 0x050100
    x->insert(QSysInfo::MV_10_9, "Mac OS X 10.9 Mavericks");
#endif
#endif
})
#endif

typedef QHash<QString, int> OptionHash;
Q_GLOBAL_STATIC_WITH_INITIALIZER(OptionHash, options,
{
    x->insert("-e", EnvironmentSpecified);
    x->insert("-s", SocketSpecified);
    x->insert("-v", PrintVersion);
    x->insert("-h", PrintUsage);
    x->insert("-l", ShowRunningAppList);
    x->insert("-d", DaemonMode);
    x->insert("-w", WindowsServiceMode);
    x->insert("-k", SendSignal);
})


static void usage()
{
    char text[] =
        "Usage: %1 [-d] [-e environment] [application-directory]\n"     \
        "Usage: %1 [-k stop|abort|restart] [application-directory]\n"   \
        "Options:\n"                                                    \
        "  -d              : run as a daemon process\n"                 \
        "  -e environment  : specify an environment of the database settings\n" \
        "  -k              : send signal to a manager process\n\n"      \
        "Type '%1 -l' to show your running applications.\n"             \
        "Type '%1 -h' to show this information.\n"                      \
        "Type '%1 -v' to show the program version.";

    QString cmd = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
    puts(qPrintable(QString(text).arg(cmd)));
}


static bool checkArguments()
{
    for (QStringListIterator i(QCoreApplication::arguments()); i.hasNext(); ) {
        const QString &arg = i.next();
        if (arg.startsWith('-') && options()->value(arg, Invalid) == Invalid) {
            fprintf(stderr, "invalid argument\n");
            return false;
        }
    }
    return true;
}


static bool startDaemon()
{
    bool success;
    QStringList args = QCoreApplication::arguments();
    args.removeAll("-d");
#ifdef Q_OS_WIN
    PROCESS_INFORMATION pinfo;
    STARTUPINFOW startupInfo = { sizeof(STARTUPINFO), 0, 0, 0,
                                 (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
                                 (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    success = CreateProcess(0, (wchar_t*)args.join(" ").utf16(),
                            0, 0, FALSE, CREATE_UNICODE_ENVIRONMENT, 0,
                            (wchar_t*)QDir::currentPath().utf16(),
                            &startupInfo, &pinfo);
    if (success) {
        CloseHandle(pinfo.hThread);
        CloseHandle(pinfo.hProcess);
    }
#else
    args.removeFirst();  // Removes the program name
    success = QProcess::startDetached(QCoreApplication::applicationFilePath(), args, QDir::currentPath());
#endif

    return success;
}


static void writeStartupLog()
{
    tSystemInfo("TreeFrog Framework version " TF_VERSION_STR);

    QString qtversion = QLatin1String("Qt ") + qVersion();
#if defined(Q_OS_WIN)
    qtversion += QLatin1String(" / ") + winVersion()->value(QSysInfo::WindowsVersion, "Windows");
#elif defined(Q_OS_DARWIN)
    qtversion += QLatin1String(" / ") + macVersion()->value(QSysInfo::MacintoshVersion, "Mac OS X");
#elif defined(Q_OS_UNIX)
    struct utsname uts;
    if (uname(&uts) == 0) {
        qtversion += QString(" / %1 %2").arg(uts.sysname).arg(uts.release);
    }
#endif
    tSystemInfo("%s", qtversion.toLatin1().data());
}


static QString pidFilePath(const QString &appRoot = QString())
{
    return (appRoot.isEmpty()) ? Tf::app()->tmpPath() + PID_FILENAME
        : appRoot + QDir::separator() + "tmp" + QDir::separator() +  PID_FILENAME;
}


static qint64 readPidFileOfApplication(const QString &appRoot = QString())
{
    QFile pidf(pidFilePath(appRoot));
    if (pidf.open(QIODevice::ReadOnly)) {
        qint64 pid = pidf.readLine(100).toLongLong();
        if (pid > 0) {
            return pid;
        }
    }
    return -1;
}


static qint64 runningApplicationPid(const QString &appRoot = QString())
{
    qint64 pid = readPidFileOfApplication(appRoot);
    if (pid > 0) {
        QString name = ProcessInfo(pid).processName().toLower();
        if (name == "treefrog" || name == "treefrogd")
            return pid;
    }
    return -1;
}


static QString runningApplicationsFilePath()
{
    return QDir::homePath() + QDir::separator() + ".treefrog" + QDir::separator() + "runnings";
}


static bool addRunningApplication(const QString &rootPath)
{
    QFile file(runningApplicationsFilePath());
    QDir dir = QFileInfo(file).dir();

    if (!dir.exists()) {
        dir.mkpath(".");
        // set permissions of the dir
        QFile(dir.absolutePath()).setPermissions(QFile::ReadUser | QFile::WriteUser | QFile::ExeUser);
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        return false;
    }

    TF_FLOCK(file.handle(), LOCK_EX); // lock
    file.write(rootPath.toLatin1());
    file.write("\n");
    TF_FLOCK(file.handle(), LOCK_UN); // unlock
    file.close();
    return true;
}


static QStringList runningApplicationPathList()
{
    QStringList paths;
    QFile file(runningApplicationsFilePath());

    if (file.open(QIODevice::ReadOnly)) {
        TF_FLOCK(file.handle(), LOCK_SH); // lock
        QList<QByteArray> lst = file.readAll().split('\n');
        TF_FLOCK(file.handle(), LOCK_UN); // unlock
        file.close();

        for (QListIterator<QByteArray> it(lst); it.hasNext(); ) {
            // Checks the running
            const QByteArray &approot = it.next().trimmed();
            if (!approot.isEmpty()) {
                if (!paths.contains(approot) && runningApplicationPid(approot) > 0) {
                    paths << approot;
                }
            }
        }
    }
    return paths;
}


static void cleanupRunningApplicationList()
{
    QStringList runapps = runningApplicationPathList();
    QFile file(runningApplicationsFilePath());

    if (runapps.isEmpty()) {
        file.remove();
        return;
    }

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        TF_FLOCK(file.handle(), LOCK_EX); // lock
        file.write(runapps.join("\n").toLatin1());
        file.write("\n");
        TF_FLOCK(file.handle(), LOCK_UN); // unlock
        file.close();
    }
}


static QVariant applicationSettingValue(const QString &appRoot, const QString &key)
{
    QString appIni = appRoot + QDir::separator() + QLatin1String("config") + QDir::separator() + "application.ini";
    return QSettings(appIni, QSettings::IniFormat).value(key);
}


static void showRunningAppList()
{
    QStringList apps = runningApplicationPathList();
    if (apps.isEmpty()) {
        printf("no running application\n");
    } else {
        int cnt = apps.count();
        printf(" %d application%s running:\n", cnt, (cnt > 1 ? "s" : ""));

        foreach (const QString &s, apps) {
            QString url = applicationSettingValue(s, "ListenPort").toString().trimmed();
            if (!url.startsWith("unix:", Qt::CaseInsensitive)) {
                QString port = (url == "80") ? QString("") : (QString(":") + url);
                url = QString("url: http://%1%2/").arg(QHostInfo::localHostName()).arg(port);
            }
            printf(" * %s\n   %s\n\n", qPrintable(s), qPrintable(url));
        }
    }
}


static int killTreeFrogProcess(const QString &cmd)
{
    qint64 pid = runningApplicationPid();
    if (pid < 0) {
        printf("TreeFrog server not running\n");
        return 1;
    }

    ProcessInfo pi(pid);

    if (cmd == "stop") {  // stop command
        pi.terminate();
        if (pi.waitForTerminated()) {
            printf("TreeFrog application servers shutdown completed\n");
        } else {
            fprintf(stderr, "TreeFrog application servers shutdown failed\n");
        }

    } else if (cmd == "abort") {  // abort command
        pi.kill();
        tSystemInfo("Killed TreeFrog manager process  pid:%ld", (long)pid);
        ::unlink(pidFilePath().toLatin1().data());

        // kill all 'tadpole' processes
#if defined(Q_OS_WIN) && !defined(TF_NO_DEBUG)
        ProcessInfo::killProcesses("tadpoled");
#else
        ProcessInfo::killProcesses("tadpole");
#endif
        tSystemInfo("Killed TreeFrog application server processes");
        printf("Killed TreeFrog application server processes\n");

    } else if (cmd == "restart") {  // restart command
        pi.restart();
        printf("Sent a restart request\n");

    } else {
        usage();
        return 1;
    }
    return 0;
}


int managerMain(int argc, char *argv[])
{
    TWebApplication app(argc, argv);

    // Sets codec
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(codec);

    if (!checkArguments()) {
        return 1;
    }

    bool daemonMode = false;
    QString signalCmd;

    QStringList args = QCoreApplication::arguments();
    args.removeFirst();
    for (QStringListIterator i(args); i.hasNext(); ) {
        const QString &arg = i.next();
        int cmd = options()->value(arg, Invalid);
        switch (cmd) {
        case PrintVersion:
            printf("%s version " TF_VERSION_STR " (r%d) built on %s / Qt %s\n", qPrintable(QFileInfo(argv[0]).baseName()), TF_SRC_REVISION, __DATE__, qVersion());
            return 0;
            break;

        case PrintUsage:
            usage();
            return 0;
            break;

        case ShowRunningAppList:
            showRunningAppList();
            return 0;
            break;

        case DaemonMode:
            daemonMode = true;
            break;

        case WindowsServiceMode:
            // ignore
            break;

        case SendSignal:
            signalCmd = i.next(); // assign a command
            break;

        default:
            // do nothing
            break;
        }
    }

    if (!app.appSettingsFileExists()) {
        fprintf(stderr, "INI file not found [%s]\n\n", qPrintable(app.appSettingsFilePath()));
        return 1;
    }

    // Creates the semaphore for system log
    QSystemSemaphore semaphore("TreeFrogSystemLog", 1, QSystemSemaphore::Create);
    tSetupSystemLoggers();

#if defined(Q_OS_UNIX)
    app.watchUnixSignal(SIGTERM);
    app.watchUnixSignal(SIGINT);
    app.watchUnixSignal(SIGHUP);

#elif defined(Q_OS_WIN)
    app.watchConsoleSignal();
# if QT_VERSION >= 0x050000
    app.watchLocalSocket();
# endif
#endif

    if (!signalCmd.isEmpty()) {
        return killTreeFrogProcess(signalCmd);
    }

    // Check TreeFrog processes are running
    qint64 pid = runningApplicationPid();
    if (pid > 0) {
        fprintf(stderr, "Already running  pid:%ld\n", (long)pid);
        return 1;
    }

    // Check a port number
    quint16 listenPort = 0;
    QString svrname = app.appSettings().value("ListenPort").toString();
    if (svrname.startsWith("unix:", Qt::CaseInsensitive)) {
        svrname.remove(0, 5);
    } else {
        int port = svrname.toInt();
        if (port <= 0 || port > USHRT_MAX) {
            tSystemError("Invalid port number: %d", port);
            return 1;
        }
        listenPort = port;
        svrname.clear();
    }

    // start daemon process
    if (daemonMode) {
        if ( !startDaemon() ) {
            fprintf(stderr, "Failed to create process\n\n");
            return 1;
        }
        return 0;
    }

    int ret = 0;
    QFile pidfile;

    for (;;) {
        ServerManager *manager = 0;
        switch ( app.multiProcessingModule() ) {
        case TWebApplication::Thread:
            manager = new ServerManager(1, 1, 0, &app);
            break;

        case TWebApplication::Prefork: {
            int max = app.appSettings().value("MPM.prefork.MaxServers").toInt();
            int min = app.appSettings().value("MPM.prefork.MinServers").toInt();
            int spare = app.appSettings().value("MPM.prefork.SpareServers").toInt();
            manager = new ServerManager(max, min, spare, &app);
            break; }

        case TWebApplication::Hybrid: {
            int num = QThread::idealThreadCount();
            manager = new ServerManager(num, num, 0, &app);
            break; }

        default:
            tSystemError("Invalid MPM specified");
            return 1;
        }

        // Startup
        writeStartupLog();
        bool started;
        if (listenPort > 0) {
            // TCP/IP
            started = manager->start(QHostAddress::Any, listenPort);
        } else {
            // UNIX domain
            started = manager->start(svrname);
        }

        if (!started) {
            tSystemError("TreeFrog application server startup failed");
            fprintf(stderr, "TreeFrog application server startup failed\n\n");
            return 1;
        }

        // tmp directory
        QDir tmpDir(app.tmpPath());
        if (!tmpDir.exists()) {
            tmpDir.mkpath(".");
        }

        // Adds this running app
        addRunningApplication(app.webRootPath());

        // Writes the PID
        pidfile.setFileName(pidFilePath());
        if (pidfile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            pid = QCoreApplication::applicationPid();
            pidfile.write( QString::number(pid).toLatin1() );
            pidfile.close();
        } else {
            tSystemError("File open failed: %s", qPrintable(pidfile.fileName()));
        }

        ret = app.exec();
        tSystemDebug("tfmanager returnCode:%d", ret);
        manager->stop();

        if (ret == 1) {  // means SIGHUP
            tSystemDebug("Restarts TreeFrog application servers");
            //continue;
        } else {
            break;
        }
    }

    if (!svrname.isEmpty()) {  // UNIX domain file
        QFile(svrname).remove();
    }

    pidfile.remove();  // Removes the PID file
    cleanupRunningApplicationList();  // Cleanup running apps
    return 0;
}

} // namespace TreeFrog


int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-w") == 0) {
            // Windows service mode
            SERVICE_TABLE_ENTRY entry[] = { { (LPTSTR)TEXT(""), TreeFrog::winServiceMain }, { 0, 0 } };
            StartServiceCtrlDispatcher(entry);
            return 0;
        }
    }
#endif
    return TreeFrog::managerMain(argc, argv);
}
