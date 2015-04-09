#include <TfTest/TfTest>
#include <QDebug>
#include "../../turlroute.h"


class TestUrlRouter2 : public QObject, public TUrlRoute
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase() { }
    void init();
    void cleanup();

    void should_route_get_correctly_data();
    void should_route_get_correctly();
};

void TestUrlRouter2::initTestCase()
{
    addRouteFromString("GET  /foo      'dummy1#foo'");
    addRouteFromString("GET  /         'dummy2#hoge'");
    addRouteFromString("GET  /fuga     'dummy3#fuga'");
    addRouteFromString("GET  /foo/hoge     'Dummy4#fuga1'");
    addRouteFromString("GET  /fuga/:params 'Dummy5#fuga2'");
    addRouteFromString("GET  /hoge/:param/hoge  'dummy6#fuga3'");
    addRouteFromString("GET  /hoge/:param/foo/:params  'dummY7#fuga4'");
    addRouteFromString("GET  /hoge/:param/fuga  'dummy8#fuga5'");
    addRouteFromString("GET  /hoge     'dummy9#hoge6'");
    addRouteFromString("POST /foo      'dummy10#hoge7'");
}

void TestUrlRouter2::init()
{ }

void TestUrlRouter2::cleanup()
{ }


void TestUrlRouter2::should_route_get_correctly_data()
{
    QTest::addColumn<int>("method");
    QTest::addColumn<QString>("path");

    QTest::addColumn<bool>("denied");
    QTest::addColumn<QString>("controller");
    QTest::addColumn<QString>("action");
    QTest::addColumn<QStringList>("params");

    QTest::newRow("1") << (int)Tf::Get << QString("/foo")
                       << false << QString("dummy1controller") << QString("foo") << QStringList();
    QTest::newRow("2") << (int)Tf::Get << QString("/")
                       << false << QString("dummy2controller") << QString("hoge") << QStringList();
    QTest::newRow("3") << (int)Tf::Get << QString("/fuga")
                       << false << QString("dummy3controller") << QString("fuga") << QStringList();
    QTest::newRow("4") << (int)Tf::Get << QString("/foo/hoge")
                       << false << QString("dummy4controller") << QString("fuga1") << QStringList();
    QTest::newRow("5") << (int)Tf::Get << QString("/fuga/hoge/")
                       << false << QString("dummy5controller") << QString("fuga2")
                       << (QStringList() << "hoge");
    QTest::newRow("6") << (int)Tf::Get << QString("/hoge/hoge/hoge")
                       << false << QString("dummy6controller") << QString("fuga3")
                       << (QStringList() << "hoge");
    QTest::newRow("7") << (int)Tf::Get << QString("/hoge/hoge/foo/foo")
                       << false << QString("dummy7controller") << QString("fuga4")
                       << (QStringList() << "hoge" << "foo");
    QTest::newRow("8") << (int)Tf::Get << QString("/hoge/foo/fuga")
                       << false << QString("dummy8controller") << QString("fuga5")
                       << (QStringList() << "foo");
    QTest::newRow("9") << (int)Tf::Get << QString("/hoge/")
                       << false << QString("dummy9controller") << QString("hoge6") << QStringList();
    QTest::newRow("10") << (int)Tf::Post << QString("/foo/")
                        << false << QString("dummy10controller") << QString("hoge7") << QStringList();
    // Deny
    QTest::newRow("11") << (int)Tf::Put << QString("/foo/")
                        << true << QString() << QString() << QStringList();
    QTest::newRow("12") << (int)Tf::Post << QString("/")
                        << true << QString() << QString() << QStringList();
    QTest::newRow("13") << (int)Tf::Delete << QString("/fuga")
                        << true << QString() << QString() << QStringList();
    // No entry
    QTest::newRow("21") << (int)Tf::Get << QString("/foo/fuga")
                        << false << QString() << QString() << QStringList();
    QTest::newRow("22") << (int)Tf::Get << QString("/foo/fuga/hoge")
                        << false << QString() << QString() << QStringList();
}


void TestUrlRouter2::should_route_get_correctly()
{
    QFETCH(int, method);
    QFETCH(QString, path);
    QFETCH(bool, denied);
    QFETCH(QString, controller);
    QFETCH(QString, action);
    QFETCH(QStringList, params);

    TRouting actual = findRouting((Tf::HttpMethod)method, TUrlRoute::splitPath(path));
    QCOMPARE(actual.isDenied(), denied);
    if (!denied) {
        QCOMPARE(QString(actual.controller), controller);
        QCOMPARE(QString(actual.action), action);
        QCOMPARE(actual.params, params);
    }
}


TF_TEST_MAIN(TestUrlRouter2)
#include "urlrouter2.moc"
