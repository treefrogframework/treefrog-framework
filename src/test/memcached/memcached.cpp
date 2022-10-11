#include <TfTest/TfTest>
#include <TMemcached>
#include <QDateTime>


class TestMemcached : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase() {}
    void cleanupTestCase() {}
    void setGet_data();
    void setGet();
};


void TestMemcached::setGet_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QString>("value");
    QTest::addColumn<int>("msecs");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << QString::fromUtf8(u8"Hello world.")
                       << 1;
    QTest::newRow("2") << QByteArray::number(QDateTime::currentMSecsSinceEpoch())
                       << QString::fromUtf8(u8"こんにちは")
                       << 2;
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << QString::fromUtf8(u8" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ")
                       << 3;
}

void TestMemcached::setGet()
{
    QFETCH(QByteArray, key);
    QFETCH(QString, value);
    QFETCH(int, msecs);

    TMemcached memcached;
    QString res = memcached.get(key);
    QCOMPARE(res, QString());  // empty
    bool ok = memcached.set(key, value, msecs);
    QCOMPARE(ok, true);  // set ok
    Tf::msleep(100);
    res = memcached.get(key);  // get value
    QCOMPARE(res, value);
    Tf::msleep(msecs * 1000);
    res = memcached.get(key);  // empty
    QCOMPARE(res, QString());
}


TF_TEST_MAIN(TestMemcached)
#include "memcached.moc"
