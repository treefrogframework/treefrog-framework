#pragma once
#ifdef TF_USE_GUI_MODULE
#include <QApplication>
#else
#include <QCoreApplication>
#endif

#include "qplatformdefs.h"
#include <QBasicTimer>
#include <QVariant>
#include <QVector>
#include <TGlobal>

class QTextCodec;


class T_CORE_EXPORT TWebApplication
#ifdef TF_USE_GUI_MODULE
    : public QApplication
#else
    : public QCoreApplication
#endif
{
    Q_OBJECT
public:
    enum MultiProcessingModule {
        Invalid = 0,
        Thread = 1,
        Epoll = 2,
    };

    TWebApplication(int &argc, char **argv);
    ~TWebApplication();

    int exec();
    QString webRootPath() const { return _webRootAbsolutePath; }
    bool webRootExists() const;
    QString publicPath() const;
    QString configPath() const;
    QString libPath() const;
    QString logPath() const;
    QString pluginPath() const;
    QString tmpPath() const;
    QString databaseEnvironment() const { return _dbEnvironment; }
    void setDatabaseEnvironment(const QString &environment);

    bool appSettingsFileExists() const;
    QString appSettingsFilePath() const;
    const QVariantMap &sqlDatabaseSettings(int databaseId) const;
    int sqlDatabaseSettingsCount() const;
    const QVariantMap &kvsSettings(Tf::KvsEngine engine) const;
    bool isKvsAvailable(Tf::KvsEngine engine) const;
    bool cacheEnabled() const;
    QString cacheBackend() const;
    int databaseIdForCache() const;
    const QVariantMap &loggerSettings() const { return _loggerSetting; }
    const QVariantMap &validationSettings() const { return _validationSetting; }
    QString validationErrorMessage(int rule) const;
    QByteArray internetMediaType(const QString &ext, bool appendCharset = false);
    MultiProcessingModule multiProcessingModule() const;
    int maxNumberOfAppServers() const;
    int maxNumberOfThreadsPerAppServer() const;
    QString routesConfigFilePath() const;
    QString systemLogFilePath() const;
    QString accessLogFilePath() const;
    QString sqlQueryLogFilePath() const;
    QTextCodec *codecForInternal() const { return _codecInternal; }
    QTextCodec *codecForHttpOutput() const { return _codecHttp; }
    int applicationServerId() const { return _appServerId; }
    QThread *databaseContextMainThread() const;
    const QVariantMap &getConfig(const QString &configName);
    QVariant getConfigValue(const QString &configName, const QString &key, const QVariant &defaultValue = QVariant());

#if defined(Q_OS_UNIX)
    void watchUnixSignal(int sig, bool watch = true);
    void ignoreUnixSignal(int sig, bool ignore = true);
#endif

#if defined(Q_OS_WIN)
    void watchConsoleSignal();
    void ignoreConsoleSignal();
    void watchLocalSocket();
    static bool sendLocalCtrlMessage(const QByteArray &msg, int targetProcess);

private slots:
    void recvLocalSocket();
#endif  // Q_OS_WIN

protected:
    void timerEvent(QTimerEvent *event);
    static int signalNumber();

private:
    QString _webRootAbsolutePath;
    QString _dbEnvironment;
    QVector<QVariantMap> _sqlSettings;
    QVector<QVariantMap> _kvsSettings {(int)Tf::KvsEngine::Num};
    QVariantMap _loggerSetting;
    QVariantMap _validationSetting;
    QVariantMap _mediaTypes;
    QTextCodec *_codecInternal {nullptr};
    QTextCodec *_codecHttp {nullptr};
    int _appServerId {-1};
    QBasicTimer _timer;
    mutable MultiProcessingModule _mpm {Invalid};
    QMap<QString, QVariantMap> _configMap;
    int _cacheSqlDbIndex {-1};

    static void resetSignalNumber();

    T_DISABLE_COPY(TWebApplication)
    T_DISABLE_MOVE(TWebApplication)
};


/*!
  Sets the database environment to \a environment.
  \sa databaseEnvironment()
*/
inline void TWebApplication::setDatabaseEnvironment(const QString &environment)
{
    _dbEnvironment = environment;
}


#if defined(Q_OS_WIN)
#include <QAbstractNativeEventFilter>

class T_CORE_EXPORT TNativeEventFilter : public QAbstractNativeEventFilter {
public:
#if QT_VERSION < 0x060000
    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, long *);
#else
    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *);
#endif
};

#endif  // Q_OS_WIN

