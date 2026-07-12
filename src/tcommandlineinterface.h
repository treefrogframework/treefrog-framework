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


#define TF_CLI_MAIN(STATICFUNCTION)                                                                                 \
    int main(int argc, char *argv[])                                                                                \
    {                                                                                                               \
        class Thread : public TActionThread {                                                                       \
        public:                                                                                                     \
            Thread() : TActionThread(0), returnCode(0) { }                                                          \
            volatile int returnCode;                                                                                \
        protected:                                                                                                  \
            virtual void run()                                                                                      \
            {                                                                                                       \
                returnCode = STATICFUNCTION();                                                                      \
                commitTransactions();                                                                               \
                QEventLoop eventLoop;                                                                               \
                while (eventLoop.processEvents()) { }                                                               \
            }                                                                                                       \
        };                                                                                                          \
        TWebApplication app(argc, argv);                                                                            \
        Tf::setupSystemLogger(new TStdErrSystemLogger);                                                             \
        Tf::setupQueryLogger();                                                                                     \
        app.setMultiProcessingModule(TWebApplication::MultiProcessingModule::Thread);                               \
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
