#include <QTest>
#include <QString>
#include "../../tabstractmodel.h"


class TestFieldnametovariablename : public QObject
{
    Q_OBJECT
private slots:
    void test_data();
    void test();
};


void TestFieldnametovariablename::test_data()
{
    QTest::addColumn<QString>("field");
    QTest::addColumn<QString>("result");

    QTest::newRow("1")  << "hoge"     << "hoge";
    QTest::newRow("2")  << "hoge_foo" << "hogeFoo";
    QTest::newRow("3")  << "HOGE"     << "hoge";
    QTest::newRow("4")  << "HOGE_FOO" << "hogeFoo";
    QTest::newRow("5")  << "hogeFoo"  << "hogeFoo";
    QTest::newRow("6")  << "hoge_FOO" << "hogeFoo";
    QTest::newRow("7")  << "HOGE12W"  << "hoge12w";
    QTest::newRow("8")  << "HOGE_12W" << "hoge12w";
    QTest::newRow("9")  << "_hoge12w" << "hoge12w";
    QTest::newRow("10") << "hoge12w_" << "hoge12w";
    QTest::newRow("11") << "hoge_Foo" << "hogeFoo";
    QTest::newRow("12") << "hoge_FOo" << "hogeFoo";
    QTest::newRow("13") << "hOge_Foo4" << "hogeFoo4";
    QTest::newRow("14") << "HOGE10"   << "hoge10";
    QTest::newRow("15") << "hogeFOsksdhfkhKJHKH_uHHo" << "hogefosksdhfkhkjhkhUhho";
}

void TestFieldnametovariablename::test()
{
    QFETCH(QString, field);
    QFETCH(QString, result);

    QString actual = TAbstractModel::fieldNameToVariableName(field);
    QCOMPARE(actual, result);

    // QBENCHMARK {
    //     QString s = fieldNameToVariableName(field);
    // }
}

QTEST_APPLESS_MAIN(TestFieldnametovariablename)
#include "main.moc"
