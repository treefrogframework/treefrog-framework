#include <TfTest/TfTest>
#include <TMemcached>
#include <QDateTime>


static QByteArray randomString(int length)
{
    constexpr auto ch = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-+/^=_[]@:;!#$%()~? \t\n";
    QByteArray ret;
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
    void getTimeout_data();
    void getTimeout();
    void addGet_data();
    void addGet();
    void addGetNumber_data();
    void addGetNumber();
    void replaceGet_data();
    void replaceGet();
    void replaceGetNumber_data();
    void replaceGetNumber();
    void appendGet_data();
    void appendGet();
    void prependGet_data();
    void prependGet();
    void remove_data();
    void remove();
    void incr_data();
    void incr();
    void decr_data();
    void decr();
    void version();
    void keyError_data();
    void keyError();
};


void TestMemcached::initTestCase()
{
}


void TestMemcached::setGet_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");
    QTest::addColumn<int>("secs");
    QTest::addColumn<uint>("flags");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << QByteArray("Hello world.")
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << QByteArray("こんにちは")
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << QByteArray(" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ")
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(256)
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("5") << QUuid::createUuid().toByteArray()
                       << randomString(512)
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("6") << QUuid::createUuid().toByteArray()
                       << randomString(1024)
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(1, UINT_MAX);
}


void TestMemcached::setGet()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, value);
    QFETCH(int, secs);
    QFETCH(uint, flags);

    TMemcached memcached;
    uint flg;
    QByteArray res = memcached.get(key);
    QCOMPARE(res, QByteArray());  // empty
    bool ok = memcached.set(key, value, secs, flags);
    QCOMPARE(ok, true);  // set ok
    Tf::msleep(Tf::random(50, 1900));  // sleep
    res = memcached.get(key, &flg);  // get value
    QCOMPARE(res, value);
    QCOMPARE(flg, flags);
}


void TestMemcached::setGetNumber_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<int64_t>("value");
    QTest::addColumn<int>("secs");
    QTest::addColumn<uint>("flags");

    QTest::newRow("1") << QUuid::createUuid().toByteArray()
                       << (int64_t)123456789
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(0, UINT_MAX);
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << (int64_t)-987654321
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(0, UINT_MAX);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << (int64_t)0
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(0, UINT_MAX);
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << (int64_t)Tf::random(1, INT64_MAX)
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(0, UINT_MAX);
    QTest::newRow("5") << QUuid::createUuid().toByteArray()
                       << (int64_t)-Tf::random(1, INT_MAX)
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(0, UINT_MAX);
}


void TestMemcached::setGetNumber()
{
    QFETCH(QByteArray, key);
    QFETCH(int64_t, value);
    QFETCH(int, secs);
    QFETCH(uint, flags);

    TMemcached memcached;
    bool ok;
    uint flg;
    auto res = memcached.getNumber(key, &ok);
    QCOMPARE(res, 0);
    QCOMPARE(ok, false);  // failire
    ok = memcached.set(key, value, secs, flags);
    //qDebug() << "set key:" << key << "value:" << value;
    QCOMPARE(ok, true);  // set ok
    Tf::msleep(Tf::random(50, 1900));  // sleep
    res = memcached.getNumber(key, &ok, &flg);  // get value
    //qDebug() << "got key:" << key << "value:" << res;
    QCOMPARE(res, value);
    QCOMPARE(ok, true);  // success
    QCOMPARE(flg, flags);
}


void TestMemcached::getTimeout_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<int>("secs");
    QTest::addColumn<uint>("flags");

    QTest::newRow("1") << QUuid::createUuid().toByteArray()
                       << (int)Tf::random(1, 5)
                       << (uint)Tf::random(0, UINT_MAX);
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << (int)Tf::random(5, 10)
                       << (uint)Tf::random(0, UINT_MAX);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << (int)Tf::random(8, 10)
                       << (uint)Tf::random(0, UINT_MAX);
}


void TestMemcached::getTimeout()
{
    QFETCH(QByteArray, key);
    QFETCH(int, secs);
    QFETCH(uint, flags);

    TMemcached memcached;
    uint flg;
    memcached.set(key, randomString(128), secs, flags);

    Tf::msleep(secs * 1100);
    auto res = memcached.get(key, &flg);  // get value
    QCOMPARE(res, QByteArray());  // timed out
}


void TestMemcached::addGet_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");
    QTest::addColumn<int>("secs");
    QTest::addColumn<uint>("flags");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << QByteArray("Hello world.")
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(0, UINT_MAX);
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << QByteArray("こんにちは")
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(0, UINT_MAX);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << QByteArray(" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ")
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(0, UINT_MAX);
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(256)
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(0, UINT_MAX);
    QTest::newRow("5") << QUuid::createUuid().toByteArray()
                       << randomString(512)
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(0, UINT_MAX);
    QTest::newRow("6") << QUuid::createUuid().toByteArray()
                       << randomString(1024)
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(0, UINT_MAX);
}


void TestMemcached::addGet()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, value);
    QFETCH(int, secs);
    QFETCH(uint, flags);

    TMemcached memcached;
    uint flg;
    QByteArray res = memcached.get(key);
    QCOMPARE(res, QByteArray());  // empty
    bool ok = memcached.add(key, value, secs, flags);
    QCOMPARE(ok, true);  // add ok
    Tf::msleep(Tf::random(50, 1900));  // sleep
    res = memcached.get(key, &flg);  // get value
    QCOMPARE(res, value);
    QCOMPARE(flg, flags);
}


void TestMemcached::addGetNumber_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<int64_t>("value");
    QTest::addColumn<int>("secs");
    QTest::addColumn<uint>("flags");

    QTest::newRow("1") << QUuid::createUuid().toByteArray()
                       << (int64_t)123456789
                       << 10
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << (int64_t)-987654321
                       << 20
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << (int64_t)0
                       << 20
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << (int64_t)Tf::random(1, INT64_MAX)
                       << 10
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("5") << QUuid::createUuid().toByteArray()
                       << (int64_t)-Tf::random(1, INT_MAX)
                       << 20
                       << (uint)Tf::random(1, UINT_MAX);
}


void TestMemcached::addGetNumber()
{
    QFETCH(QByteArray, key);
    QFETCH(int64_t, value);
    QFETCH(int, secs);
    QFETCH(uint, flags);

    TMemcached memcached;
    bool ok;
    uint flg;
    auto res = memcached.getNumber(key, &ok);
    QCOMPARE(res, 0);
    QCOMPARE(ok, false);  // failire
    ok = memcached.add(key, value, secs, flags);
    qDebug() << "add key:" << key << "value:" << value;
    QCOMPARE(ok, true);  // set ok
    Tf::msleep(Tf::random(50, 1900));  // sleep
    res = memcached.getNumber(key, &ok, &flg);  // get value
    qDebug() << "got key:" << key << "value:" << res;
    QCOMPARE(res, value);
    QCOMPARE(ok, true);  // success
    QCOMPARE(flg, flags);
}


void TestMemcached::replaceGet_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");
    QTest::addColumn<int>("secs");
    QTest::addColumn<uint>("flags");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << QByteArray("Hello world.")
                       << 10
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << QByteArray("こんにちは")
                       << 20
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << QByteArray(" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ")
                       << 30
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(256)
                       << 10
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("5") << QUuid::createUuid().toByteArray()
                       << randomString(512)
                       << 20
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("6") << QUuid::createUuid().toByteArray()
                       << randomString(1024)
                       << 30
                       << (uint)Tf::random(1, UINT_MAX);
}


void TestMemcached::replaceGet()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, value);
    QFETCH(int, secs);
    QFETCH(uint, flags);

    TMemcached memcached;
    uint flg;
    memcached.set(key, randomString(50), secs * Tf::random(1, 10), 23);
    Tf::msleep(Tf::random(50, 1000));  // sleep

    bool ok = memcached.replace(key, value, secs, flags);
    QCOMPARE(ok, true);  // add ok
    Tf::msleep(Tf::random(50, 1900));  // sleep
    auto res = memcached.get(key, &flg);  // get value
    QCOMPARE(res, value);
    QCOMPARE(flg, flags);
}


void TestMemcached::replaceGetNumber_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<int64_t>("value");
    QTest::addColumn<int>("secs");
    QTest::addColumn<uint>("flags");

    QTest::newRow("1") << QUuid::createUuid().toByteArray()
                       << (int64_t)123456789
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << (int64_t)-987654321
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << (int64_t)0
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << (int64_t)Tf::random(1, INT64_MAX)
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("5") << QUuid::createUuid().toByteArray()
                       << (int64_t)-Tf::random(1, INT_MAX)
                       << (int)Tf::random(3, 10)
                       << (uint)Tf::random(1, UINT_MAX);
}


void TestMemcached::replaceGetNumber()
{
    QFETCH(QByteArray, key);
    QFETCH(int64_t, value);
    QFETCH(int, secs);
    QFETCH(uint, flags);

    TMemcached memcached;
    uint flg;
    memcached.set(key, Tf::random(1, INT_MAX), secs * Tf::random(1, 10), flags);
    Tf::msleep(Tf::random(50, 1000));  // sleep

    bool ok = memcached.replace(key, value, secs, flags);
    QCOMPARE(ok, true);  // add ok
    Tf::msleep(Tf::random(50, 1900));  // sleep
    auto res = memcached.getNumber(key, &ok, &flg);  // get value
    QCOMPARE(res, value);
    QCOMPARE(ok, true);  // success
    QCOMPARE(flg, flags);
}


void TestMemcached::appendGet_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value1");
    QTest::addColumn<QByteArray>("value2");
    QTest::addColumn<int>("secs");
    QTest::addColumn<uint>("flags");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << QByteArray("Hello world.")
                       << QByteArray("こんにちは")
                       << 10
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << QByteArray(" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ")
                       << QByteArray("\t\r\n")
                       << 20
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << randomString(126)
                       << randomString(256)
                       << 30
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(512)
                       << randomString(512)
                       << 10
                       << (uint)Tf::random(1, UINT_MAX);
}


void TestMemcached::appendGet()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, value1);
    QFETCH(QByteArray, value2);
    QFETCH(int, secs);
    QFETCH(uint, flags);

    TMemcached memcached;
    uint flg;
    memcached.set(key, value1, secs, flags);
    Tf::msleep(Tf::random(50, 500));  // sleep

    bool ok = memcached.append(key, value2, secs, 11);
    QCOMPARE(ok, true);  // set ok
    Tf::msleep(Tf::random(50, 1900));  // sleep
    auto res = memcached.get(key, &flg);  // get value
    QCOMPARE(res, value1 + value2);
    QCOMPARE(flg, flags);
}


void TestMemcached::prependGet_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value1");
    QTest::addColumn<QByteArray>("value2");
    QTest::addColumn<int>("secs");
    QTest::addColumn<uint>("flags");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << QByteArray("Hello world.")
                       << QByteArray("こんにちは")
                       << 10
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << QByteArray(" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ")
                       << QByteArray("\t\r\n")
                       << 20
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << randomString(126)
                       << randomString(256)
                       << 30
                       << (uint)Tf::random(1, UINT_MAX);
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(512)
                       << randomString(512)
                       << 10
                       << (uint)Tf::random(1, UINT_MAX);
}


void TestMemcached::prependGet()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, value1);
    QFETCH(QByteArray, value2);
    QFETCH(int, secs);
    QFETCH(uint, flags);

    TMemcached memcached;
    uint flg;
    memcached.set(key, value1, secs, flags);
    Tf::msleep(Tf::random(50, 500));  // sleep

    bool ok = memcached.prepend(key, value2, secs, 11);
    QCOMPARE(ok, true);  // set ok
    Tf::msleep(Tf::random(50, 1900));  // sleep
    auto res = memcached.get(key, &flg);  // get value
    QCOMPARE(res, value2 + value1);
    QCOMPARE(flg, flags);
}


void TestMemcached::remove_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");
    QTest::addColumn<int>("secs");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << QByteArray("Hello world.")
                       << (int)Tf::random(5, 3600);
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << QByteArray(" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ")
                       << (int)Tf::random(5, 3600);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << randomString(256)
                       << (int)Tf::random(5, 3600);
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(512)
                       << (int)Tf::random(5, 3600);
}


void TestMemcached::remove()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, value);
    QFETCH(int, secs);

    TMemcached memcached;
    memcached.set(key, value, secs);
    Tf::msleep(Tf::random(50, 500));  // sleep

    bool ok = memcached.remove(key);
    QCOMPARE(ok, true);  // set ok
    auto res = memcached.get(key);  // get value
    QCOMPARE(res, QByteArray());
}


void TestMemcached::incr_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<uint64_t>("value");
    QTest::addColumn<uint64_t>("incr");
    QTest::addColumn<int>("secs");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << (uint64_t)0 << (uint64_t)0
                       << (int)Tf::random(5, 3600);
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << (uint64_t)0 << (uint64_t)Tf::random(1, INT64_MAX)
                       << (int)Tf::random(5, 3600);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << (uint64_t)Tf::random(1, INT64_MAX) << (uint64_t)0
                       << (int)Tf::random(5, 3600);
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << (uint64_t)Tf::random(0, INT64_MAX / 2) << (uint64_t)Tf::random(0, INT64_MAX / 2)
                       << (int)Tf::random(5, 3600);
}


void TestMemcached::incr()
{
    QFETCH(QByteArray, key);
    QFETCH(uint64_t, value);
    QFETCH(uint64_t, incr);
    QFETCH(int, secs);

    TMemcached memcached;
    bool ok;
    memcached.set(key, value, secs);
    Tf::msleep(Tf::random(50, 500));  // sleep
    uint64_t res = memcached.incr(key + "hoge", incr, &ok);
    QCOMPARE(res, 0UL);
    QCOMPARE(ok, false);  // incr failure

    res = memcached.incr(key, incr, &ok);
    QCOMPARE(ok, true);  // incr ok
    QCOMPARE(res, value + incr);

    Tf::msleep(Tf::random(50, 1900));  // sleep
    res = memcached.getNumber(key, &ok);  // get value
    QCOMPARE(ok, true);  // get ok
    QCOMPARE(res, value + incr);
}


void TestMemcached::decr_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<uint64_t>("value");
    QTest::addColumn<uint64_t>("decr");
    QTest::addColumn<int>("secs");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << (uint64_t)0 << (uint64_t)0
                       << (int)Tf::random(5, 3600);
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << (uint64_t)0 << (uint64_t)Tf::random(1, INT64_MAX)
                       << (int)Tf::random(5, 3600);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << (uint64_t)Tf::random(1, INT64_MAX) << (uint64_t)0
                       << (int)Tf::random(5, 3600);
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << (uint64_t)Tf::random(INT64_MAX / 2, INT64_MAX) << (uint64_t)Tf::random(0, INT64_MAX / 2)
                       << (int)Tf::random(5, 3600);
    QTest::newRow("5") << QUuid::createUuid().toByteArray()
                       << (uint64_t)Tf::random(0, INT64_MAX / 2) << (uint64_t)Tf::random(INT64_MAX / 2, INT64_MAX)
                       << (int)Tf::random(5, 3600);
}


void TestMemcached::decr()
{
    QFETCH(QByteArray, key);
    QFETCH(uint64_t, value);
    QFETCH(uint64_t, decr);
    QFETCH(int, secs);

    TMemcached memcached;
    bool ok;
    memcached.set(key, value, secs);
    Tf::msleep(Tf::random(50, 500));  // sleep
    uint64_t res = memcached.decr(key + "foo", decr, &ok);
    QCOMPARE(res, 0UL);
    QCOMPARE(ok, false);  // decr failure

    res = memcached.decr(key, decr, &ok);
    QCOMPARE(ok, true);  // decr ok
    if (value > decr) {
        QCOMPARE(res, value - decr);
    } else {
        QCOMPARE(res, 0UL);
    }

    Tf::msleep(Tf::random(50, 1900));  // sleep
    res = memcached.getNumber(key, &ok);  // get value
    QCOMPARE(ok, true);  // get ok
    if (value > decr) {
        QCOMPARE(res, value - decr);
    } else {
        QCOMPARE(res, 0UL);
    }
}


void TestMemcached::version()
{
    TMemcached memcached;
    auto version = memcached.version();
    qDebug() << version;
    QVERIFY(!version.isEmpty());
}


void TestMemcached::keyError_data()
{
    // Invalid keys
    QTest::addColumn<QByteArray>("key");

    QTest::newRow("1") << QByteArray("");
    QTest::newRow("2") << QByteArray(" ");
    QTest::newRow("3") << QByteArray("\t");
    QTest::newRow("4") << QByteArray("\n");
    QTest::newRow("5") << QByteArray("\r\n");
    QTest::newRow("6") << QByteArray(" bad");
    QTest::newRow("7") << QByteArray("bad key");
    QTest::newRow("8") << QByteArray("hoge ");
    QTest::newRow("9") << QByteArray("foo\n");
}


void TestMemcached::keyError()
{
    QFETCH(QByteArray, key);

    TMemcached memcached;
    bool ok = memcached.set(key, randomString(256), Tf::random(50, 500));
    QCOMPARE(ok, false);  // key failure
    ok = memcached.set(key, 1, Tf::random(50, 500));
    QCOMPARE(ok, false);  // key failure
    ok = memcached.add(key, randomString(256), Tf::random(50, 500));
    QCOMPARE(ok, false);  // key failure
    ok = memcached.add(key, 2, Tf::random(50, 500));
    QCOMPARE(ok, false);  // key failure
}


TF_TEST_MAIN(TestMemcached)
#include "memcached.moc"
