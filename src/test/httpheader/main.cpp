#include <TfTest/TfTest>
#include <THttpRequest>
#include "thttpheader.h"


class TestHttpHeader : public QObject
{
    Q_OBJECT
private slots:
    void parseParamExists_data();
    void parseParamExists();
    void parseHttpRequest_data();
    void parseHttpRequest();
    void parseRequestVariantList_data();
    void parseRequestVariantList();
    void parseRequestVariantMap_data();
    void parseRequestVariantMap();
};


void TestHttpHeader::parseParamExists_data()
{
    QTest::addColumn<QString>("header");
    QTest::addColumn<QString>("body");
    QTest::addColumn<QString>("key1");
    QTest::addColumn<bool>("ex1");
    QTest::addColumn<QString>("key2");
    QTest::addColumn<bool>("ex2");

    QTest::newRow("1") << "POST /generate HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 1000"
                       << "row=%E3%83%95%E3%82%A9%E3%83%AB%E3%83%80&excludeTags%5B%5D%5B%E5%85%A8%E4%BD%93%E7%97%85%E5%A4%89%5D=01normalstomach&excludeTags%5Baa%5D%5B%E3%83%95%E3%82%A9%E3%83%AB%E3%83%80%5D=AAA&excludeTags%5B33%5D%5B%E5%88%B6%E5%BE%A1%5D=trash&auth%5B%5D=&auth%5B%5D=unchecked"
                       << "row"
                       << true
                       << "excludeTags"
                       << true;
    QTest::newRow("2") << "POST /generate HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 1000"
                       << "row=%E3%83%95%E3%82%A9%E3%83%AB%E3%83%80&excludeTags%5B%5D%5B%E5%85%A8%E4%BD%93%E7%97%85%E5%A4%89%5D=01normalstomach&excludeTags%5Baa%5D%5B%E3%83%95%E3%82%A9%E3%83%AB%E3%83%80%5D=AAA&excludeTags%5B33%5D%5B%E5%88%B6%E5%BE%A1%5D=trash&auth%5B%5D=&auth%5B%5D=unchecked"
                       << "row1"
                       << false
                       << "excludeTags[aa]"
                       << true;
    QTest::newRow("3") << "POST /generate HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 1000"
                       << "row=%E3%83%95%E3%82%A9%E3%83%AB%E3%83%80&excludeTags%5B%5D%5B%E5%85%A8%E4%BD%93%E7%97%85%E5%A4%89%5D=01normalstomach&excludeTags%5Baa%5D%5B%E3%83%95%E3%82%A9%E3%83%AB%E3%83%80%5D=AAA&excludeTags%5B33%5D%5B%E5%88%B6%E5%BE%A1%5D=trash&auth%5B%5D=&auth%5B%5D=unchecked"
                       << "excludeTags[]"
                       << true
                       << u8"excludeTags[全体病変]"
                       << false;
}

void TestHttpHeader::parseParamExists()
{
    QFETCH(QString, header);
    QFETCH(QString, body);
    QFETCH(QString, key1);
    QFETCH(bool, ex1);
    QFETCH(QString, key2);
    QFETCH(bool, ex2);

    TActionThread *context = dynamic_cast<TActionThread *>(QThread::currentThread());
    THttpRequestHeader h(header.toUtf8());
    THttpRequest http(h, body.toUtf8(), QHostAddress(), context);
    QCOMPARE(http.hasFormItem(key1), ex1);
    QCOMPARE(http.hasFormItem(key2), ex2);
}

void TestHttpHeader::parseHttpRequest_data()
{
    QTest::addColumn<QString>("header");
    QTest::addColumn<QString>("body");
    QTest::addColumn<QString>("key1");
    QTest::addColumn<QString>("val1");
    QTest::addColumn<QString>("key2");
    QTest::addColumn<QStringList>("val2");

    QTest::newRow("1") << "POST /generate HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 1000"
                       << "email=aoyama%40example.com&password=pass&auto_login=on&login=%E3%83%AD%E3%82%B0%E3%82%A4%E3%83%B3"
                       << "email"
                       << "aoyama@example.com"
                       << QString()
                       << QStringList();
    QTest::newRow("2") << "POST /generate HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 10000"
                       << "rowGroup=%E3%83%95%E3%82%A9%E3%83%AB%E3%83%80&columnGroup=%E6%89%BF%E8%AA%8D%E7%8A%B6%E6%85%8B&excludeTags%5Bgroup%5D=&excludeTags%5Btag%5D=&authorizationRequiredColumns%5B%5D=unchecked&columnOrders%5B%5D=unchecked&columnOrders%5B%5D=checked&columnOrders%5B%5D=done&savedActionTags%5Be%5D=checked&savedActionTags%5Bd%5D=done"
                       << "columnGroup"
                       << u8"承認状態"
                       << "columnOrders"
                       << QStringList({ "unchecked", "checked", "done"});
    QTest::newRow("3") << "POST /generate HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 10000"
                       << "aa=zz&a%5B%5D=3&c%5B%5D=1&c%5B%5D=2&c%5B%5D=3&c%5B%5D=4&c%5B%5D=5&c%5B%5D=6&c%5B%5D=7&c%5B%5D=8&c%5B%5D=9&c%5B%5D=10&c%5B%5D=11&c%5B%5D=12&c%5B%5D=13&c%5B%5D=14&c%5B%5D=15&c%5B%5D=16&c%5B%5D=17&c%5B%5D=18&c%5B%5D=19&c%5B%5D=20&c%5B%5D=21&c%5B%5D=22&c%5B%5D=23&c%5B%5D=24&c%5B%5D=25&c%5B%5D=26&c%5B%5D=27&c%5B%5D=28&c%5B%5D=29&c%5B%5D=30&c1%5B%5D=c4&cc%5B%5D=1&8c%5B%5D=100"
                       << "aa"
                       << "zz"
                       << "c"
                       << QStringList({ "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27","28", "29", "30"});
}


void TestHttpHeader::parseHttpRequest()
{
    QFETCH(QString, header);
    QFETCH(QString, body);
    QFETCH(QString, key1);
    QFETCH(QString, val1);
    QFETCH(QString, key2);
    QFETCH(QStringList, val2);

    TActionThread *context = dynamic_cast<TActionThread *>(QThread::currentThread());
    THttpRequestHeader h(header.toUtf8());
    THttpRequest http(h, body.toUtf8(), QHostAddress(), context);
    // auto map = http.formItems();
    // auto it = map.begin();
    // while (it != map.end()) {
    //     qDebug() << it.value().toString();
    //     ++it;
    // }
    QCOMPARE(http.formItemValue(key1), val1);
    QCOMPARE(http.formItemList(key2), val2);
}


void TestHttpHeader::parseRequestVariantList_data()
{
    QTest::addColumn<QString>("header");
    QTest::addColumn<QString>("body");
    QTest::addColumn<QString>("key");
    QTest::addColumn<QString>("key0");
    QTest::addColumn<QString>("val0");
    QTest::addColumn<QString>("key1");
    QTest::addColumn<QString>("val1");

    QTest::newRow("1") << "POST /generate HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 1000"
                       << "row=%E3%83%95%E3%82%A9%E3%83%AB%E3%83%80&excludeTags%5B%5D%5B%E5%85%A8%E4%BD%93%E7%97%85%E5%A4%89%5D=01normalstomach&excludeTags%5B%5D%5B%E3%83%95%E3%82%A9%E3%83%AB%E3%83%80%5D=AAA&excludeTags%5B%5D%5B%E5%88%B6%E5%BE%A1%5D=trash&auth%5B%5D=&auth%5B%5D=unchecked"
                       << "excludeTags"
                       << u8"全体病変"
                       << "01normalstomach"
                       << u8"フォルダ"
                       << "AAA";
    QTest::newRow("2") << "POST /generate HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 1000"
                       << "row=%E3%83%95%E3%82%A9%E3%83%AB%E3%83%80&excludeTags%5B%5D%5B%E5%85%A8%E4%BD%93%E7%97%85%E5%A4%89%5D=01normalstomach&excludeTags%5B%5D%5B%E3%83%95%E3%82%A9%E3%83%AB%E3%83%80%5D=AAA&excludeTags%5B%5D%5B%E5%88%B6%E5%BE%A1%5D=trash&auth%5B%5D=&auth%5B%5D=unchecked"
                       << "excludeTags"
                       << "foo"
                       << QString()
                       << u8"全体病変"
                       << QString();
}


void TestHttpHeader::parseRequestVariantList()
{
    QFETCH(QString, header);
    QFETCH(QString, body);
    QFETCH(QString, key);
    QFETCH(QString, key0);
    QFETCH(QString, val0);
    QFETCH(QString, key1);
    QFETCH(QString, val1);

    TActionThread *context = dynamic_cast<TActionThread *>(QThread::currentThread());
    THttpRequestHeader h(header.toUtf8());
    THttpRequest http(h, body.toUtf8(), QHostAddress(), context);
    auto vlist = http.formItemVariantList(key);
    QCOMPARE(vlist[0].toMap()[key0].toString(), val0);
    QCOMPARE(vlist[1].toMap()[key1].toString(), val1);
}

void TestHttpHeader::parseRequestVariantMap_data()
{
    QTest::addColumn<QString>("header");
    QTest::addColumn<QString>("body");
    QTest::addColumn<QString>("mapname");
    QTest::addColumn<QString>("key");
    QTest::addColumn<QString>("key0");
    QTest::addColumn<QString>("val0");
    QTest::addColumn<QString>("key1");
    QTest::addColumn<QString>("val1");

    QTest::newRow("1") << "POST /generate HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 1000"
                       << "row=%E3%83%95%E3%82%A9%E3%83%AB%E3%83%80&excludeTags%5Baa%5D%5B%E5%85%A8%E4%BD%93%E7%97%85%E5%A4%89%5D=01normalstomach&excludeTags%5Baa%5D%5B%E3%83%95%E3%82%A9%E3%83%AB%E3%83%80%5D=AAA&excludeTags%5B33%5D%5B%E5%88%B6%E5%BE%A1%5D=trash&auth%5B%5D=&auth%5B%5D=unchecked"
                       << "excludeTags"
                       << "aa"
                       << u8"全体病変"
                       << "01normalstomach"
                       << u8"フォルダ"
                       << "AAA";
    QTest::newRow("1") << "POST /generate HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 1000"
                       << "row=%E3%83%95%E3%82%A9%E3%83%AB%E3%83%80&excludeTags%5Baa%5D%5B%E5%85%A8%E4%BD%93%E7%97%85%E5%A4%89%5D=01normalstomach&excludeTags%5Baa%5D%5B%E3%83%95%E3%82%A9%E3%83%AB%E3%83%80%5D=AAA&excludeTags%5B33%5D%5B%E5%88%B6%E5%BE%A1%5D=trash&auth%5B%5D=&auth%5B%5D=unchecked"
                       << "excludeTags"
                       << "33"
                       << u8"全体病変"
                       << QString()
                       << u8"制御"
                       << "trash";
}

void TestHttpHeader::parseRequestVariantMap()
{
    QFETCH(QString, header);
    QFETCH(QString, body);
    QFETCH(QString, mapname);
    QFETCH(QString, key);
    QFETCH(QString, key0);
    QFETCH(QString, val0);
    QFETCH(QString, key1);
    QFETCH(QString, val1);

    TActionThread *context = dynamic_cast<TActionThread *>(QThread::currentThread());
    THttpRequestHeader h(header.toUtf8());
    THttpRequest http(h, body.toUtf8(), QHostAddress(), context);
    auto map = http.formItems(mapname);
    auto vmap = map.value(key).toMap();
    QCOMPARE(vmap[key0].toString(), val0);
    QCOMPARE(vmap[key1].toString(), val1);
}

TF_TEST_MAIN(TestHttpHeader)
#include "main.moc"
