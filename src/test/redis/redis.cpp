#include <TfTest/TfTest>
#include <TRedis>
#include <QDateTime>


static QString randomString(int length)
{
    static const QString str = QString::fromUtf8(
        u8"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-+/^=_[]@:;!#$%()~? \t\n"
        "あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよをぁぃぅぇぉっゃゅょ"
        "アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨヲァィゥェォッャュョヴ"
        "０１２３４５６７８９零一二三四五六七八九十百千万億兆京垓じょ穣溝澗正載極恒河沙阿僧祇那由他不可思議無量大数"
    );

    QString ret;
    for (int i = 0; i < length; ++i) {
        ret += str[(int)Tf::random(str.length() - 1)];
    }
    return ret;
}


class TestRedis : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase() {}
    void exists_data();
    void exists();
    void setGet_data();
    void setGet();
    void setexGet_data();
    void setexGet();
    void setnxGet_data();
    void setnxGet();
    void getSet_data();
    void getSet();

    void setsGet_data();
    void setsGet();
    void setsexGet_data();
    void setsexGet();
    void setsnxGet_data();
    void setsnxGet();
    void getSets_data();
    void getSets();
};


void TestRedis::initTestCase()
{
}


void TestRedis::exists_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << QByteArray("Hello world.");
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << QByteArray("こんにちは");
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << QByteArray(" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ");
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(256).toLatin1();
    QTest::newRow("5") << QUuid::createUuid().toByteArray()
                       << randomString(512).toLatin1();
    QTest::newRow("6") << QUuid::createUuid().toByteArray()
                       << randomString(1024).toLatin1();
}


void TestRedis::exists()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, value);

    TRedis redis;
    bool exists = redis.exists(key);
    QCOMPARE(exists, false);  // not exist

    bool ok = redis.set(key, value);
    QCOMPARE(ok, true);  // set ok
    Tf::msleep(Tf::random(50, 1000));  // sleep
    exists = redis.exists(key);
    QCOMPARE(exists, true);
}


void TestRedis::setGet_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << QByteArray("Hello world.");
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << QByteArray("こんにちは");
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << QByteArray(" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ");
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
                       << QByteArray("Hello world.")
                       << (int)Tf::random(2, 10);
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << QByteArray("こんにちは")
                       << (int)Tf::random(2, 10);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << QByteArray(" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ")
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
    Tf::msleep(Tf::random(50, 1700));  // sleep
    res = redis.get(key);  // get value
    QCOMPARE(res, value);
}


void TestRedis::setnxGet_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << QByteArray("Hello world.");
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << QByteArray("こんにちは");
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << QByteArray(" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ");
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(256).toLatin1();
    QTest::newRow("5") << QUuid::createUuid().toByteArray()
                       << randomString(512).toLatin1();
    QTest::newRow("6") << QUuid::createUuid().toByteArray()
                       << randomString(1024).toLatin1();
}


void TestRedis::setnxGet()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, value);

    TRedis redis;
    auto res = redis.get(key);
    QCOMPARE(res, QByteArray());  // empty
    bool ok = redis.setNx(key, value);
    QCOMPARE(ok, true);  // set ok
    Tf::msleep(Tf::random(50, 1900));  // sleep
    res = redis.get(key);  // get value
    QCOMPARE(res, value);

    Tf::msleep(Tf::random(50, 100));  // sleep
    ok = redis.setNx(key, "value");
    QCOMPARE(ok, false);  // set failure
    res = redis.get(key);  // get value
    QCOMPARE(res, value);
}


void TestRedis::getSet_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << QByteArray("Hello world.");
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << QByteArray("こんにちは");
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << QByteArray(" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ");
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(256).toLatin1();
    QTest::newRow("5") << QUuid::createUuid().toByteArray()
                       << randomString(512).toLatin1();
    QTest::newRow("6") << QUuid::createUuid().toByteArray()
                       << randomString(1024).toLatin1();
}


void TestRedis::getSet()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, value);

    TRedis redis;
    QByteArray res = redis.getSet(key, value);
    QCOMPARE(res, QByteArray());
    Tf::msleep(Tf::random(50, 200));  // sleep

    res = redis.getSet(key, value + value);
    QCOMPARE(res, value);
    Tf::msleep(Tf::random(50, 500));  // sleep
    res = redis.get(key);
    QCOMPARE(res, value + value);
}

///---------------------------------------------------

void TestRedis::setsGet_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QString>("value");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << QString(u8"Hello world.");
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << QString(u8"こんにちは");
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << QString(u8" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ");
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(256);
    QTest::newRow("5") << QUuid::createUuid().toByteArray()
                       << randomString(512);
    QTest::newRow("6") << QUuid::createUuid().toByteArray()
                       << randomString(1024);
}


void TestRedis::setsGet()
{
    QFETCH(QByteArray, key);
    QFETCH(QString, value);

    TRedis redis;
    auto res = redis.gets(key);
    QCOMPARE(res, QByteArray());  // empty
    bool ok = redis.sets(key, value);
    QCOMPARE(ok, true);  // set ok
    Tf::msleep(Tf::random(50, 1900));  // sleep
    res = redis.gets(key);  // get value
    QCOMPARE(res, value);
}


void TestRedis::setsexGet_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QString>("value");
    QTest::addColumn<int>("secs");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << QString(u8"Hello world.")
                       << (int)Tf::random(2, 10);
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << QString(u8"こんにちは")
                       << (int)Tf::random(2, 10);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << QString(u8" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ")
                       << (int)Tf::random(2, 10);
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(256)
                       << (int)Tf::random(2, 10);
    QTest::newRow("5") << QUuid::createUuid().toByteArray()
                       << randomString(512)
                       << (int)Tf::random(2, 10);
    QTest::newRow("6") << QUuid::createUuid().toByteArray()
                       << randomString(1024)
                       << (int)Tf::random(2, 10);
}


void TestRedis::setsexGet()
{
    QFETCH(QByteArray, key);
    QFETCH(QString, value);
    QFETCH(int, secs);

    TRedis redis;
    auto res = redis.gets(key);
    QCOMPARE(res, QByteArray());  // empty
    bool ok = redis.setsEx(key, value, secs);
    QCOMPARE(ok, true);  // set ok
    Tf::msleep(Tf::random(50, 1900));  // sleep
    res = redis.gets(key);  // get value
    QCOMPARE(res, value);
}


void TestRedis::setsnxGet_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QString>("value");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << QString(u8"Hello world.");
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << QString(u8"こんにちは");
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << QString(u8" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ");
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(256);
    QTest::newRow("5") << QUuid::createUuid().toByteArray()
                       << randomString(512);
    QTest::newRow("6") << QUuid::createUuid().toByteArray()
                       << randomString(1024);
}


void TestRedis::setsnxGet()
{
    QFETCH(QByteArray, key);
    QFETCH(QString, value);

    TRedis redis;
    auto res = redis.gets(key);
    QCOMPARE(res, QByteArray());  // empty
    bool ok = redis.setsNx(key, value);
    QCOMPARE(ok, true);  // set ok
    Tf::msleep(Tf::random(50, 1900));  // sleep
    res = redis.gets(key);  // get value
    QCOMPARE(res, value);

    Tf::msleep(Tf::random(50, 100));  // sleep
    ok = redis.setsNx(key, "value");
    QCOMPARE(ok, false);  // set failure
    res = redis.gets(key);  // get value
    QCOMPARE(res, value);
}


void TestRedis::getSets_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QString>("value");

    QTest::newRow("1") << QByteArray::number(QDateTime::currentSecsSinceEpoch())
                       << QString(u8"Hello world.");
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << QString(u8"こんにちは");
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << QString(u8" Hello world. \r\nこんにちは、\n\"世界\"\t!!! ");
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(256);
    QTest::newRow("5") << QUuid::createUuid().toByteArray()
                       << randomString(512);
    QTest::newRow("6") << QUuid::createUuid().toByteArray()
                       << randomString(1024);
}


void TestRedis::getSets()
{
    QFETCH(QByteArray, key);
    QFETCH(QString, value);

    TRedis redis;
    auto res = redis.getsSets(key, value);
    QCOMPARE(res, QString());  // empty
    Tf::msleep(Tf::random(50, 200));  // sleep

    res = redis.getsSets(key, value + value);
    QCOMPARE(res, value);
    Tf::msleep(Tf::random(50, 500));  // sleep
    res = redis.gets(key);
    QCOMPARE(res, value + value);
}


TF_TEST_MAIN(TestRedis)
#include "redis.moc"
