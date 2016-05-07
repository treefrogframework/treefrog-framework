#ifndef TWEBAPPLICATION_H
#define TWEBAPPLICATION_H

#ifdef TF_USE_GUI_MODULE
# include <QApplication>
#else
# include <QCoreApplication>
#endif

#include <QVector>
#include <QSettings>
#include <QBasicTimer>
#include <TGlobal>
#include "qplatformdefs.h"

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
        Thread,
        Hybrid,
    };

    TWebApplication(int &argc, char **argv);
    ~TWebApplication();

    int exec();
    QString webRootPath() const { return webRootAbsolutePath; }
    bool webRootExists() const;
    QString publicPath() const;
    QString configPath() const;
    QString libPath() const;
    QString logPath() const;
    QString pluginPath() const;
    QString tmpPath() const;
    QString databaseEnvironment() const { return dbEnvironment; }
    void setDatabaseEnvironment(const QString &environment);

    bool appSettingsFileExists() const;
    QString appSettingsFilePath() const;
    QSettings &sqlDatabaseSettings(int databaseId) const;
    int sqlDatabaseSettingsCount() const;
    bool isSqlDatabaseAvailable() const;
    QSettings &mongoDbSettings() const;
    bool isMongoDbAvailable() const;
    QSettings &redisSettings() const;
    bool isRedisAvailable() const;
    QSettings &loggerSettings() const { return *loggerSetting; }
    QSettings &validationSettings() const { return *validationSetting; }
    QString validationErrorMessage(int rule) const;
    QByteArray internetMediaType(const QString &ext, bool appendCharset = false);
    MultiProcessingModule multiProcessingModule() const;
    int maxNumberOfAppServers(int defaultValue = 1) const;
    QString routesConfigFilePath() const;
    QString systemLogFilePath() const;
    QString accessLogFilePath() const;
    QString sqlQueryLogFilePath() const;
    QTextCodec *codecForInternal() const { return codecInternal; }
    QTextCodec *codecForHttpOutput() const { return codecHttp; }
    int applicationServerId() const { return appServerId; }

#if defined(Q_OS_UNIX)
    void watchUnixSignal(int sig, bool watch = true);
    void ignoreUnixSignal(int sig, bool ignore = true);
#endif

#if defined(Q_OS_WIN)
    void watchConsoleSignal();
    void ignoreConsoleSignal();

# if QT_VERSION < 0x050000
    virtual bool winEventFilter(MSG *msg, long *result);
# else
    void watchLocalSocket();
    static bool sendLocalCtrlMessage(const QByteArray &msg,  int targetProcess);

private slots:
    void recvLocalSocket();
# endif
#endif // Q_OS_WIN

protected:
    void timerEvent(QTimerEvent *event);
    static int signalNumber();

private:
    Q_DISABLE_COPY(TWebApplication)

    QString webRootAbsolutePath;
    QString dbEnvironment;
    QVector<QSettings *> sqlSettings;
    QSettings *mongoSetting;
    QSettings *redisSetting;
    QSettings *loggerSetting;
    QSettings *validationSetting;
    QSettings *mediaTypes;
    QTextCodec *codecInternal;
    QTextCodec *codecHttp;
    int appServerId;
    QBasicTimer timer;
    mutable MultiProcessingModule mpm;

    static void resetSignalNumber();
};


/*!
  Sets the database environment to \a environment.
  \sa databaseEnvironment()
*/
inline void TWebApplication::setDatabaseEnvironment(const QString &environment)
{
    dbEnvironment = environment;
}


#if defined(Q_OS_WIN) && QT_VERSION >= 0x050000
#include <QAbstractNativeEventFilter>

class T_CORE_EXPORT TNativeEventFilter : public QAbstractNativeEventFilter
{
public:
    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, long *);
};

#endif // QT_VERSION >= 0x050000

#endif // TWEBAPPLICATION_H
