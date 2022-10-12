#include <TfTest/TfTest>
#include <TMemcached>
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


class TestMemcached : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase() {}
    void setGet_data();
    void setGet();
    void setGetNumber_data();
    void setGetNumber();
};


void TestMemcached::initTestCase()
{
    TMemcached memcached;
    auto ok = memcached.flushAll();
    qDebug() << "flush_all" << ok;
    Tf::msleep(1000);
    // Note that after flushing, you have to wait a certain amount of time
    // (in my case < 1s) to be able to write to Memcached again.
}


void TestMemcached::setGet_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QString>("value");
    QTest::addColumn<int>("secs");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << QString::fromUtf8(u8"Hello world.")
                       << 1;
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << QString::fromUtf8(u8"こんにちは")
                       << 2;
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << QString::fromUtf8(u8" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ")
                       << 3;
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(256)
                       << 1;
    QTest::newRow("5") << QUuid::createUuid().toByteArray()
                       << randomString(512)
                       << 2;
    QTest::newRow("6") << QUuid::createUuid().toByteArray()
                       << randomString(1024)
                       << 3;
}

void TestMemcached::setGet()
{
    QFETCH(QByteArray, key);
    QFETCH(QString, value);
    QFETCH(int, secs);

    TMemcached memcached;
    QString res = memcached.get(key);
    QCOMPARE(res, QString());  // empty
    bool ok = memcached.set(key, value, secs);
    QCOMPARE(ok, true);  // set ok
    Tf::msleep(Tf::random(50, 800));  // sleep
    res = memcached.get(key);  // get value
    QCOMPARE(res, value);
    Tf::msleep(secs * 1000);
    res = memcached.get(key);  // empty
    QCOMPARE(res, QString());
}


void TestMemcached::setGetNumber_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<qint64>("value");
    QTest::addColumn<int>("secs");

    QTest::newRow("1") << QUuid::createUuid().toByteArray()
                       << 123456789LL
                       << 2;
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << -987654321LL
                       << 3;
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << 0LL
                       << 1;
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << (qint64)Tf::random(1, INT64_MAX)
                       << 1;
    QTest::newRow("5") << QUuid::createUuid().toByteArray()
                       << (qint64)-Tf::random(1, INT_MAX)
                       << 2;
}


void TestMemcached::setGetNumber()
{
    QFETCH(QByteArray, key);
    QFETCH(qint64, value);
    QFETCH(int, secs);

    TMemcached memcached;
    bool ok;
    auto res = memcached.getNumber(key, &ok);
    QCOMPARE(res, 0);
    QCOMPARE(ok, false);  // failire
    ok = memcached.set(key, value, secs);
    qDebug() << "set key:" << key << "value:" << value;
    QCOMPARE(ok, true);  // set ok
    Tf::msleep(Tf::random(50, 800));  // sleep
    res = memcached.getNumber(key, &ok);  // get value
    qDebug() << "got key:" << key << "value:" << res;
    QCOMPARE(res, value);
    QCOMPARE(ok, true);  // success
    Tf::msleep(secs * 1000);
    res = memcached.getNumber(key, &ok);  // empty
    QCOMPARE(res, 0LL);
    QCOMPARE(ok, false);  // failire
}

TF_TEST_MAIN(TestMemcached)
#include "memcached.moc"
