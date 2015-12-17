/* Copyright (c) 2010-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QtCore>
#include <QSysInfo>
#include <QHostInfo>
#include <TWebApplication>
#include <TAppSettings>
#include <TSystemGlobal>
#include <tprocessinfo.h>
#include <tfcore.h>
#include "servermanager.h"
#include "systembusdaemon.h"

#ifdef Q_OS_UNIX
# include <sys/utsname.h>
#endif
#ifdef Q_OS_WIN
# include <windows.h>
#endif

#ifdef Q_OS_WIN
# define TF_FLOCK(fd,op)
#else
# define TF_FLOCK(fd,op)  tf_flock((fd),(op))
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
    AutoReload,
};


#ifdef Q_OS_WIN
class WinVersion : public  QHash<int, QString>
{
public:
    WinVersion() : QHash<int, QString>()
    {
        insert(QSysInfo::WV_XP,         "Windows XP");
        insert(QSysInfo::WV_2003,       "Windows Server 2003");
        insert(QSysInfo::WV_VISTA,      "Windows Vista or Windows Server 2008");
        insert(QSysInfo::WV_WINDOWS7,   "Windows 7 or Windows Server 2008 R2");
# if QT_VERSION >= 0x050000
        insert(QSysInfo::WV_WINDOWS8,   "Windows 8 or Windows Server 2012");
# endif
# if QT_VERSION >= 0x050200
        insert(QSysInfo::WV_WINDOWS8_1, "Windows 8.1 or Windows Server 2012 R2");
# endif
# if QT_VERSION >= 0x050500
        insert(QSysInfo::WV_WINDOWS10,  "Windows 10");
# endif
    }
};
Q_GLOBAL_STATIC(WinVersion, winVersion)
#endif

#ifdef Q_OS_DARWIN
class MacxVersion : public QHash<int, QString>
{
public:
    MacxVersion() : QHash<int, QString>()
    {
        insert(QSysInfo::MV_10_3, "Mac OS X 10.3 Panther");
        insert(QSysInfo::MV_10_4, "Mac OS X 10.4 Tiger");
        insert(QSysInfo::MV_10_5, "Mac OS X 10.5 Leopard");
        insert(QSysInfo::MV_10_6, "Mac OS X 10.6 Snow Leopard");
# if QT_VERSION >= 0x040800
        insert(QSysInfo::MV_10_7, "Mac OS X 10.7 Lion");
        insert(QSysInfo::MV_10_8, "Mac OS X 10.8 Mountain Lion");
# endif
# if QT_VERSION >= 0x050100
        insert(QSysInfo::MV_10_9, "Mac OS X 10.9 Mavericks");
# endif
# if QT_VERSION >= 0x050400
        insert(QSysInfo::MV_10_10, "Mac OS X 10.10 Yosemite");
# endif
# if QT_VERSION >= 0x050500
        insert(QSysInfo::MV_10_11, "Mac OS X 10.11 El Capitan");
# endif
    }
};
Q_GLOBAL_STATIC(MacxVersion, macxVersion)
#endif

class OptionHash : public QHash<QString, int>
{
public:
    OptionHash() : QHash<QString, int>()
    {
        insert("-e", EnvironmentSpecified);
        insert("-s", SocketSpecified);
        insert("-v", PrintVersion);
        insert("-h", PrintUsage);
        insert("-l", ShowRunningAppList);
        insert("-d", DaemonMode);
        insert("-w", WindowsServiceMode);
        insert("-k", SendSignal);
        insert("-r", AutoReload);
    }
};
Q_GLOBAL_STATIC(OptionHash, options)


static void usage()
{
    char text[] =
        "Usage: %1 [-d] [-e environment] [application-directory]\n"     \
        "Usage: %1 [-k stop|abort|restart|status] [application-directory]\n" \
        "%2"                                                            \
        "Options:\n"                                                    \
        "  -d              : run as a daemon process\n"                 \
        "  -e environment  : specify an environment of the database settings\n" \
        "  -k              : send signal to a manager process\n"        \
        "%4"                                                            \
        "%3\n"                                                          \
        "Type '%1 -l' to show your running applications.\n"             \
        "Type '%1 -h' to show this information.\n"                      \
        "Type '%1 -v' to show the program version.";

    QString cmd = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
    QString text2, text3, text4;
#ifdef Q_OS_WIN
    text2 = QString("Usage: %1 -w [-e environment] application-directory\n").arg(cmd);
    text3 = "  -w              : run as Windows service mode\n";
#else
    text4 = "  -r              : reload app automatically for development\n";
#endif

    puts(qPrintable(QString(text).arg(cmd).arg(text2).arg(text3).arg(text4)));
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
    qtversion += QLatin1String(" / ") + macxVersion()->value(QSysInfo::MacintoshVersion, "Mac OS X");
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
        QString name = TProcessInfo(pid).processName().toLower();
        if (name == "treefrog" || name == "treefrogd")
            return pid;
    }
    return -1;
}


static QString runningApplicationsFilePath()
{
    QString home = QDir::homePath();
#ifdef Q_OS_LINUX
    if (home == QDir::rootPath()) {
        home = QLatin1String("/root");
    }
#endif
    return home + QDir::separator() + ".treefrog" + QDir::separator() + "runnings";
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
                url = QString("http://%1%2/").arg(QHostInfo::localHostName()).arg(port);
            }
            printf(" * %s\n    %s\n\n", qPrintable(s), qPrintable(url));
        }
    }
}


static int killTreeFrogProcess(const QString &cmd)
{
    qint64 pid = runningApplicationPid();

    if (cmd == "status") {  // status command
        if (pid > 0) {
            printf("TreeFrog server is running.  ( %s )\n", qPrintable(Tf::app()->webRootPath()));
        } else {
            printf("TreeFrog server is stopped.  ( %s )\n", qPrintable(Tf::app()->webRootPath()));
        }
        return (pid > 0) ? 0 : 1;
    }

    if (pid < 0) {
        printf("TreeFrog server not running\n");
        return 1;
    }

    TProcessInfo pi(pid);

    if (cmd == "stop") {  // stop command
        pi.terminate();
        if (pi.waitForTerminated()) {
            printf("TreeFrog application servers shutdown completed\n");
        } else {
            fprintf(stderr, "TreeFrog application servers shutdown failed\n");
        }

    } else if (cmd == "abort") {  // abort command
        QList<qint64> pids = pi.childProcessIds();

        pi.kill();  // kills the manager process
        SystemBusDaemon::releaseResource(pid);
        tf_unlink(pidFilePath().toLatin1().data());
        tSystemInfo("Killed TreeFrog manager process  pid:%ld", (long)pid);

        TProcessInfo::kill(pids);  // kills the server process
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
    bool autoReloadMode = false;
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

        case AutoReload:
            autoReloadMode = true;
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

    // Setup system loggers
    tSetupSystemLogger();

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
    QString svrname = Tf::appSettings()->value(Tf::ListenPort).toString();
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
        case TWebApplication::Thread:  // FALL THROUGH
        case TWebApplication::Hybrid: {
            int num = 1;
            if (!autoReloadMode) {
                num = qMax(app.maxNumberOfAppServers(), 1);
            }
            tSystemDebug("Max number of app servers: %d", num);
            manager = new ServerManager(num, num, 0, &app);
            break; }

        default:
            tSystemError("Invalid MPM specified");
            return 1;
        }

        // Startup
        writeStartupLog();
        SystemBusDaemon::instantiate();

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
        tSystemDebug("TreeFrog manager process caught a signal [code:%d]", ret);
        manager->stop();

        if (ret == 1) {  // means SIGHUP
            tSystemInfo("Restarts TreeFrog application servers");
            //continue;
        } else {
            break;
        }
    }

    // Close system bus
    SystemBusDaemon::instance()->close();

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
            SERVICE_TABLE_ENTRY entry[] = { { (LPTSTR)TEXT(""), (LPSERVICE_MAIN_FUNCTION)TreeFrog::winServiceMain },
                                            { NULL, NULL } };
            if (!StartServiceCtrlDispatcher(entry))
                return 1;
            return 0;
        }
    }
#endif
    int ret = TreeFrog::managerMain(argc, argv);
    tReleaseSystemLogger();
    return ret;
}
