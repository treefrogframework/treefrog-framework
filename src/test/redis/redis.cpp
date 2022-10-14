#include <TfTest/TfTest>
#include <TRedis>
#include <QDateTime>


static QString randomString(int length)
{
    constexpr auto ch = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-+/^=_[]@:;!#$%()~? \t\n";
    QString ret;
    int max = (int)strlen(ch) - 1;

    for (int i = 0; i < length; ++i) {
        ret += ch[Tf::random(max)];
    }
    return ret;
}


class TestRedis : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase() {}
    void setGet_data();
    void setGet();
    void setexGet_data();
    void setexGet();
};


void TestRedis::initTestCase()
{
}


void TestRedis::setGet_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << QByteArray(u8"Hello world.");
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << QByteArray(u8"こんにちは");
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << QByteArray(u8" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ");
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(256).toLatin1();
    QTest::newRow("5") << QUuid::createUuid().toByteArray()
                       << randomString(512).toLatin1();
    QTest::newRow("6") << QUuid::createUuid().toByteArray()
                       << randomString(1024).toLatin1();
}


void TestRedis::setGet()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, value);

    TRedis redis;
    auto res = redis.get(key);
    QCOMPARE(res, QByteArray());  // empty
    bool ok = redis.set(key, value);
    QCOMPARE(ok, true);  // set ok
    Tf::msleep(Tf::random(50, 1900));  // sleep
    res = redis.get(key);  // get value
    QCOMPARE(res, value);
}


void TestRedis::setexGet_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");
    QTest::addColumn<int>("secs");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << QByteArray(u8"Hello world.")
                       << (int)Tf::random(2, 10);
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << QByteArray(u8"こんにちは")
                       << (int)Tf::random(2, 10);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << QByteArray(u8" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ")
                       << (int)Tf::random(2, 10);
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(256).toLatin1()
                       << (int)Tf::random(2, 10);
    QTest::newRow("5") << QUuid::createUuid().toByteArray()
                       << randomString(512).toLatin1()
                       << (int)Tf::random(2, 10);
    QTest::newRow("6") << QUuid::createUuid().toByteArray()
                       << randomString(1024).toLatin1()
                       << (int)Tf::random(2, 10);
}


void TestRedis::setexGet()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, value);
    QFETCH(int, secs);

    TRedis redis;
    auto res = redis.get(key);
    QCOMPARE(res, QByteArray());  // empty
    bool ok = redis.setEx(key, value, secs);
    QCOMPARE(ok, true);  // set ok
    Tf::msleep(Tf::random(50, 1900));  // sleep
    res = redis.get(key);  // get value
    QCOMPARE(res, value);
}


TF_TEST_MAIN(TestRedis)
#include "redis.moc"
