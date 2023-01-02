#pragma once
#include "tkvsdatabasepool.h"
#include "tsqldatabasepool.h"
#include <TActionThread>
#include <TSystemGlobal>
#include <TStdErrSystemLogger>
#include <TStdOutLogger>
#include <TWebApplication>
#include <TAppSettings>
#include <TLogger>
#include <QtCore>


#if QT_VERSION < 0x060000
#define TF_CLI_MAIN(STATICFUNCTION)                                                                                 \
    int main(int argc, char *argv[])                                                                                \
    {                                                                                                               \
        class Thread : public TActionThread {                                                                       \
        public:                                                                                                     \
            Thread() : TActionThread(0), returnCode(0) { }                                                          \
            volatile int returnCode;                                                                                \
                                                                                                                    \
        protected:                                                                                                  \
            virtual void run()                                                                                      \
            {                                                                                                       \
                returnCode = STATICFUNCTION();                                                                      \
                commitTransactions();                                                                               \
                for (auto it = sqlDatabases.begin(); it != sqlDatabases.end(); ++it) {                              \
                    it.value().database().close();  /* close SQL database */                                        \
                }                                                                                                   \
                for (auto it = kvsDatabases.begin(); it != kvsDatabases.end(); ++it) {                              \
                    it.value().close();  /* close KVS database */                                                   \
                }                                                                                                   \
                QEventLoop eventLoop;                                                                               \
                while (eventLoop.processEvents()) { }                                                               \
            }                                                                                                       \
        };                                                                                                          \
        TWebApplication app(argc, argv);                                                                            \
        QByteArray codecName = Tf::appSettings()->value(Tf::InternalEncoding).toByteArray();                        \
        QTextCodec *codec = QTextCodec::codecForName(codecName);                                                    \
        QTextCodec::setCodecForLocale(codec);                                                                       \
        Tf::setupSystemLogger(new TStdErrSystemLogger);                                                             \
        Tf::setupQueryLogger();                                                                                     \
        app.setMultiProcessingModule(TWebApplication::Thread);                                                      \
        int idx = QCoreApplication::arguments().indexOf("-e");                                                      \
        QString env = (idx > 0) ? QCoreApplication::arguments().value(idx + 1) : QString("product");                \
        app.setDatabaseEnvironment(env);                                                                            \
        Thread thread;                                                                                              \
        QObject::connect(&thread, SIGNAL(finished()), &app, SLOT(quit()));                                          \
        thread.start();                                                                                             \
        app.exec();                                                                                                 \
        Tf::releaseAppLoggers();                                                                                    \
        Tf::releaseQueryLogger();                                                                                   \
        return thread.returnCode;                                                                                   \
    }

#else

#define TF_CLI_MAIN(STATICFUNCTION)                                                                                 \
    int main(int argc, char *argv[])                                                                                \
    {                                                                                                               \
        class Thread : public TActionThread {                                                                       \
        public:                                                                                                     \
            Thread() : TActionThread(0), returnCode(0) { }                                                          \
            volatile int returnCode;                                                                                \
                                                                                                                    \
        protected:                                                                                                  \
            virtual void run()                                                                                      \
            {                                                                                                       \
                returnCode = STATICFUNCTION();                                                                      \
                commitTransactions();                                                                               \
                for (auto it = sqlDatabases.begin(); it != sqlDatabases.end(); ++it) {                              \
                    it.value().database().close(); /* close SQL database */                                         \
                }                                                                                                   \
                for (auto it = kvsDatabases.begin(); it != kvsDatabases.end(); ++it) {                              \
                    it.value().close(); /* close KVS database */                                                    \
                }                                                                                                   \
                QEventLoop eventLoop;                                                                               \
                while (eventLoop.processEvents()) { }                                                               \
            }                                                                                                       \
        };                                                                                                          \
        TWebApplication app(argc, argv);                                                                            \
        Tf::setupSystemLogger(new TStdErrSystemLogger);                                                             \
        Tf::setupQueryLogger();                                                                                     \
        app.setMultiProcessingModule(TWebApplication::Thread);                                                      \
        int idx = QCoreApplication::arguments().indexOf("-e");                                                      \
        QString env = (idx > 0) ? QCoreApplication::arguments().value(idx + 1) : QString("product");                \
        app.setDatabaseEnvironment(env);                                                                            \
        Thread thread;                                                                                              \
        QObject::connect(&thread, SIGNAL(finished()), &app, SLOT(quit()));                                          \
        thread.start();                                                                                             \
        app.exec();                                                                                                 \
        Tf::releaseAppLoggers();                                                                                    \
        Tf::releaseQueryLogger();                                                                                   \
        return thread.returnCode;                                                                                   \
    }
#endif
