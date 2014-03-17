#include <QTest>
#include "turlroute.h"


class TestUrlRouter : public QObject
{
    Q_OBJECT
private slots:
    void test1();
};


void TestUrlRouter::test1()
{
    QVERIFY(1);
}


QTEST_MAIN(TestUrlRouter)
#include "main.moc"
