#include <QTest>

#if QT_VERSION >= 0x050000
class TestHttpHeader : public QObject
{
    Q_OBJECT
};
#else // QT_VERSION < 0x050000

#include <QHttpHeader>
#include "thttpheader.h"


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
//     qDebug("Qt: %s", qPrintable(qhttp.toString()));
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
//     qDebug("Qt: %s", qPrintable(qhttp.toString()));
//     qDebug("Tf: %s", thttp.toByteArray().data());
    QCOMPARE(qhttp.toString().toLatin1(), thttp.toByteArray());
    QCOMPARE(qhttp.majorVersion(), thttp.majorVersion());
    QCOMPARE(qhttp.minorVersion(), thttp.minorVersion());
}

#endif // QT_VERSION < 0x050000
QTEST_MAIN(TestHttpHeader)
#include "main.moc"


