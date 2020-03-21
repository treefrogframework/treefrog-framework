#include "../../tactioncontroller.h"
#include <TfTest/TfTest>


class BookController : public TActionController
{
    Q_OBJECT
protected:
    const TActionController *controller() const { return this; }
};


class TestUrl : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();

    void url_correctly_data();
    void url_correctly();
};

void TestUrl::init()
{ }

void TestUrl::cleanup()
{ }


void TestUrl::url_correctly_data()
{
    QTest::addColumn<QString>("controller");
    QTest::addColumn<QString>("action");
    QTest::addColumn<QStringList>("args");
    QTest::addColumn<QString>("url");

    QTest::newRow("1") << "Book" << "show" << QStringList() << "/Book/show";
    QTest::newRow("2") << "Book" << "show" << QStringList("1") << "/Book/1";
    QTest::newRow("3") << "Book" << "show" << QStringList({"1", "3"}) << "/Book/show/1/3";
    QTest::newRow("4") << "Book" << "index" << QStringList() << "/Book/index";
    QTest::newRow("5") << "Book" << "" << QStringList() << "/Book/";
}


void TestUrl::url_correctly()
{
    QFETCH(QString, controller);
    QFETCH(QString, action);
    QFETCH(QStringList, args);
    QFETCH(QString, url);

    QString path = TUrlRoute::instance().findUrl(controller, action, args);
    BookController contoller;
    QString res = contoller.url(controller, action, args).toString();

    QCOMPARE(res, url);
}

TF_TEST_MAIN(TestUrl)
#include "main.moc"
