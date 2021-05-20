#include <TfTest/TfTest>
#include <THttpRequest>
#include "thttpheader.h"

#if QT_VERSION >= 0x050000
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

    THttpRequestHeader h(header.toUtf8());
    THttpRequest http(h, body.toUtf8(), QHostAddress());
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

    THttpRequestHeader h(header.toUtf8());
    THttpRequest http(h, body.toUtf8(), QHostAddress());
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

    THttpRequestHeader h(header.toUtf8());
    THttpRequest http(h, body.toUtf8(), QHostAddress());
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

    THttpRequestHeader h(header.toUtf8());
    THttpRequest http(h, body.toUtf8(), QHostAddress());
    auto map = http.formItems(mapname);
    auto vmap = map.value(key).toMap();
    QCOMPARE(vmap[key0].toString(), val0);
    QCOMPARE(vmap[key1].toString(), val1);
}


#else // QT_VERSION < 0x050000

#include <QHttpHeader>


class TestHttpHeader : public QObject
{
    Q_OBJECT
private slots:
    void parseHttpRequestHeader_data();
    void parseHttpRequestHeader();
    void parseHttpResponseHeader_data();
    void parseHttpResponseHeader();
};


void TestHttpHeader::parseHttpRequestHeader_data()
{
    QTest::addColumn<QString>("data");
    QTest::newRow("1") <<
        "GET /generate_204 HTTP/2.2\r\n"
        "Content-Length: 0\r\n"
        "Content-Type: text/html\r\n"
        "Date: Sun, 27 Mar 2011 11:48:42 GMT\r\n"
        "Server: GFE/2.0\r\n";

    QTest::newRow("2") <<
        "GET /generate_204 HTTP/2.2\r\n"
        "Content-Length: 0";

    QTest::newRow("3") <<
        "GET /ge HTTP/1.1\r\n"
        "Accept-Encoding: gzip,deflate\r\n"
        "Accept-Charset: UTF-8,*\r\n"
        "Keep-Alive: 115\r\n"
        "Connection: alive\r\n";

    QTest::newRow("4") <<
        "GET /ge HTTP/1.1\r\n"
        "Accept-Encoding: gzip,deflate\r\n"
        "Accept-Charset: UTF-8,*\r\n"
        "Keep-Alive: 115\r\n"
        "Connection: kdd\r\n"
        "Received: fr\r\n"
        " by hoge.hoge.123.com\r\n"
        " by hoge.hoga.com";
}


void TestHttpHeader::parseHttpRequestHeader()
{
    QFETCH(QString, data);
    QHttpRequestHeader qhttp(data);
    THttpRequestHeader thttp(data.toLatin1());
//     qDebug("Qt: %s", qUtf8Printable(qhttp.toString()));
//     qDebug("Tf: %s", thttp.toByteArray().data());

    QCOMPARE(qhttp.toString().toLatin1(), thttp.toByteArray());
    QCOMPARE(qhttp.contentType().toLatin1(), thttp.contentType());
    QCOMPARE(qhttp.contentLength(), thttp.contentLength());
    QCOMPARE(qhttp.hasKey("date"), thttp.hasRawHeader("date"));
    QCOMPARE(qhttp.hasKey("Date"), thttp.hasRawHeader("Date"));
    QCOMPARE(qhttp.hasKey("hoge"), thttp.hasRawHeader("hoge"));
    QCOMPARE(qhttp.value("server").toLatin1(), thttp.rawHeader("server"));
    QCOMPARE(qhttp.keys().count(), thttp.rawHeaderList().count());

    qhttp.setValue("Connection", "keep-alive");
    thttp.setRawHeader("Connection", "keep-alive");
    QCOMPARE(qhttp.toString().toLatin1(), thttp.toByteArray());

    qhttp.addValue("Connection", "keep-alive222");
    thttp.addRawHeader("Connection", "keep-alive222");
    QCOMPARE(qhttp.toString().toLatin1(), thttp.toByteArray());

    qhttp.setValue("Hoge", "hoge");
    thttp.setRawHeader("Hoge", "hoge");
    QCOMPARE(qhttp.toString().toLatin1(), thttp.toByteArray());

    qhttp.setValue("Hoge", "");
    thttp.setRawHeader("Hoge", "");
    QCOMPARE(qhttp.toString().toLatin1(), thttp.toByteArray());

    qhttp.removeAllValues("connection");
    thttp.removeAllRawHeaders("connection");
    QCOMPARE(qhttp.toString().toLatin1(), thttp.toByteArray());
    QCOMPARE(qhttp.majorVersion(), thttp.majorVersion());
    QCOMPARE(qhttp.minorVersion(), thttp.minorVersion());
}


void TestHttpHeader::parseHttpResponseHeader_data()
{
    QTest::addColumn<QString>("data");
    QTest::newRow("1") <<
        "HTTP/1.1 204 No Content\r\n"
        "Content-Length: 0\r\n";

    QTest::newRow("2") <<
        "HTTP/3.3 204 No Content\r\n"
        "Content-Length: 0\r\n"
        "Content-Type: text/html\r\n"
        "Date: Sun, 27 Mar 2011 11:48:42 GMT\r\n"
        "Server: GFE/2.0\r\n\r\n";
}


void TestHttpHeader::parseHttpResponseHeader()
{
    QFETCH(QString, data);
    QHttpResponseHeader qhttp(data);
    THttpResponseHeader thttp(data.toLatin1());
//     qDebug("Qt: %s", qUtf8Printable(qhttp.toString()));
//     qDebug("Tf: %s", thttp.toByteArray().data());
    QCOMPARE(qhttp.toString().toLatin1(), thttp.toByteArray());
    QCOMPARE(qhttp.majorVersion(), thttp.majorVersion());
    QCOMPARE(qhttp.minorVersion(), thttp.minorVersion());
}

#endif // QT_VERSION < 0x050000

TF_TEST_MAIN(TestHttpHeader)
#include "main.moc"
