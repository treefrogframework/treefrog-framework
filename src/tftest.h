#ifndef TFTEST_H
#define TFTEST_H

#include <QtTest/QtTest>
#include <QObject>
#include <QTextCodec>
#include <QByteArray>
#include <TWebApplication>
#include <TKvsDatabasePool>
#ifdef QT_SQL_LIB
# include <TActionThread>
# include <TSqlDatabasePool>
#endif

#if QT_VERSION < 0x050000
# define SET_CODEC_FOR_TR(codec)  do { QTextCodec::setCodecForTr(codec); QTextCodec::setCodecForCStrings(codec); } while (0)
#else
# define SET_CODEC_FOR_TR(codec)
#endif


#define TF_TEST_MAIN(TestObject) \
int main(int argc, char *argv[]) \
{ \
    class Thread : public TActionThread { \
    public: \
        Thread() : TActionThread(0), returnCode(0) { } \
        int returnCode; \
    protected: \
        virtual void run() \
        { \
            TestObject obj; \
            returnCode = QTest::qExec(&obj, QCoreApplication::arguments()); \
        } \
    }; \
    TWebApplication app(argc, argv); \
    QByteArray codecName = app.appSettings().value("InternalEncoding", "UTF-8").toByteArray(); \
    QTextCodec *codec = QTextCodec::codecForName(codecName); \
    QTextCodec::setCodecForLocale(codec); \
    SET_CODEC_FOR_TR(codec); \
    app.setDatabaseEnvironment("test"); \
    TSqlDatabasePool::instantiate(); \
    TKvsDatabasePool::instantiate(); \
    Thread thread; \
    thread.start(); \
    thread.wait(); \
    return thread.returnCode; \
}


#define TF_TEST_SQLLESS_MAIN(TestObject) \
int main(int argc, char *argv[]) \
{ \
    TWebApplication app(argc, argv); \
    QByteArray codecName = app.appSettings().value("InternalEncoding", "UTF-8").toByteArray(); \
    QTextCodec *codec = QTextCodec::codecForName(codecName); \
    QTextCodec::setCodecForLocale(codec); \
    SET_CODEC_FOR_TR(codec); \
    TestObject tc; \
    return QTest::qExec(&tc, argc, argv); \
}

#endif // TFTEST_H
