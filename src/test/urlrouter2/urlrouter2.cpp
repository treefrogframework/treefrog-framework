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
    void find_url_data();
    void find_url();
};

void TestUrlRouter2::initTestCase()
{
    addRouteFromString("GET   /foo               'dummy1.foo'");
    addRouteFromString("GET   /                  'dummy2.hoge'");
    addRouteFromString("GET   /fuga              'dummy3.fuga'");
    addRouteFromString("GET   /foo/hoge          'Dummy4.fuga1'");
    addRouteFromString("GET   /fuga/:params      'Dummy5.fuga2'");
    addRouteFromString("GET   /hoge/:param/hoge  'dummy6.fuga3'");
    addRouteFromString("GET   /hoge/:param/foo/:params  'dummY7.fuga4'");
    addRouteFromString("GET   /hoge/:param/fuga  'dummy8.fuga5'");
    addRouteFromString("GET   /hoge              'dummy9.hoge6'");
    addRouteFromString("GET   /hoge/foo/fuga     'dummy9.function'");
    addRouteFromString("GET   /hoge/:param       'dummy11.fuga8'");
    addRouteFromString("POST  /foo               'dummy10.hoge7'");
    addRouteFromString("POST  /hoge              'dummy1.foo'");  // same to top item
    addRouteFromString("MATCH /foo/foo           '/index2.html'");  // redirect to the file
}

void TestUrlRouter2::init()
{ }

void TestUrlRouter2::cleanup()
{ }


void TestUrlRouter2::should_route_get_correctly_data()
{
    QTest::addColumn<int>("method");
    QTest::addColumn<QString>("path");

    QTest::addColumn<bool>("exists");
    QTest::addColumn<QString>("controller");
    QTest::addColumn<QString>("action");
    QTest::addColumn<QStringList>("params");

    QTest::newRow("1") << (int)Tf::Get << QString("/foo")
                       << true << QString("dummy1controller") << QString("foo") << QStringList();
    QTest::newRow("2") << (int)Tf::Get << QString("/")
                       << true << QString("dummy2controller") << QString("hoge") << QStringList();
    QTest::newRow("3") << (int)Tf::Get << QString("/fuga")
                       << true << QString("dummy3controller") << QString("fuga") << QStringList();
    QTest::newRow("4") << (int)Tf::Get << QString("/foo/hoge")
                       << true << QString("dummy4controller") << QString("fuga1") << QStringList();
    QTest::newRow("5") << (int)Tf::Get << QString("/fuga/hoge/")
                       << true << QString("dummy5controller") << QString("fuga2")
                       << (QStringList() << "hoge");
    QTest::newRow("6") << (int)Tf::Get << QString("/hoge/hoge/hoge")
                       << true << QString("dummy6controller") << QString("fuga3")
                       << (QStringList() << "hoge");
    QTest::newRow("7") << (int)Tf::Get << QString("/hoge/hoge/foo/foo")
                       << true << QString("dummy7controller") << QString("fuga4")
                       << (QStringList() << "hoge" << "foo");
    QTest::newRow("8") << (int)Tf::Get << QString("/hoge/foo/fuga")
                       << true << QString("dummy8controller") << QString("fuga5")
                       << (QStringList() << "foo");
    QTest::newRow("9") << (int)Tf::Get << QString("/hoge/")
                       << true << QString("dummy9controller") << QString("hoge6") << QStringList();
    QTest::newRow("10") << (int)Tf::Post << QString("/foo/")
                        << true << QString("dummy10controller") << QString("hoge7") << QStringList();
    QTest::newRow("11") << (int)Tf::Get << QString("/hoge/1")
                        << true << QString("dummy11controller") << QString("fuga8") << QStringList("1");

    // No entry
    QTest::newRow("30") << (int)Tf::Put << QString("/foo/")
                        << false << QString() << QString() << QStringList();
    QTest::newRow("31") << (int)Tf::Post << QString("/")
                        << false << QString() << QString() << QStringList();
    QTest::newRow("32") << (int)Tf::Delete << QString("/fuga")
                        << false << QString() << QString() << QStringList();
    QTest::newRow("33") << (int)Tf::Get << QString("/foo/fuga")
                        << false << QString() << QString() << QStringList();
    QTest::newRow("34") << (int)Tf::Get << QString("/foo/fuga/hoge")
                        << false << QString() << QString() << QStringList();
    // Redirect
    QTest::newRow("50") << (int)Tf::Get << QString("/foo/foo")
                        << true << QString("/index2.html") << QString() << QStringList();
    QTest::newRow("51") << (int)Tf::Get << QString("/foo/foo/")
                        << true << QString("/index2.html") << QString() << QStringList();
    QTest::newRow("52") << (int)Tf::Put << QString("/foo/foo")
                        << true << QString("/index2.html") << QString() << QStringList();
}


void TestUrlRouter2::should_route_get_correctly()
{
    QFETCH(int, method);
    QFETCH(QString, path);
    QFETCH(bool, exists);
    QFETCH(QString, controller);
    QFETCH(QString, action);
    QFETCH(QStringList, params);

    TRouting actual = findRouting((Tf::HttpMethod)method, TUrlRoute::splitPath(path));
    QCOMPARE(actual.exists, exists);
    if (exists) {
        QCOMPARE(QString(actual.controller), controller);
        QCOMPARE(QString(actual.action), action);
        QCOMPARE(actual.params, params);
    }
}


void TestUrlRouter2::find_url_data()
{
    QTest::addColumn<QStringList>("func");
    QTest::addColumn<QString>("output");

    QTest::newRow("1")  << QStringList({ "hogehoge", "hoge" })
                        << QString();
    QTest::newRow("2")  << QStringList({ "dummy2", "hoge" })
                        << QString("/");
    QTest::newRow("3")  << QStringList({ "dummy1", "foo" })
                        << QString("/foo");
    QTest::newRow("4")  << QStringList({ "dummy9", "function" })
                        << QString("/hoge/foo/fuga");
    QTest::newRow("5")  << QStringList({ "dummy6", "fuga3", "0" })
                        << QString("/hoge/0/hoge");
    QTest::newRow("6")  << QStringList({ "Dummy5", "fuga2", "00", "11" })
                        << QString("/fuga/00/11");
    QTest::newRow("7")  << QStringList({ "dummY7", "fuga4" })
                        << QString();
    QTest::newRow("8")  << QStringList({ "dummY7", "fuga4", "0" })
                        << QString("/hoge/0/foo/");
    QTest::newRow("9")  << QStringList({ "dummY7", "fuga4", "0", "1" })
                        << QString("/hoge/0/foo/1");
    QTest::newRow("10") << QStringList({ "dummY7", "fuga4", "0", "1", "2" })
                        << QString("/hoge/0/foo/1/2");
    QTest::newRow("11") << QStringList({ "dummy8", "fuga5", "0" })
                        << QString("/hoge/0/fuga");
    QTest::newRow("12") << QStringList({ "dummy8", "fuga5", "0", "1" })
                        << QString();
}


void TestUrlRouter2::find_url()
{
    QFETCH(QStringList, func);
    QFETCH(QString, output);

    QStringList params = func.mid(2);
    QString result = findUrl(func[0], func[1], params);
    QCOMPARE(result, output);
}

TF_TEST_MAIN(TestUrlRouter2)
#include "urlrouter2.moc"
