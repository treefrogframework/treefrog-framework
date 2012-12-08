#include <QtTest/QtTest>
#include <QFile>
#include <THtmlParser>


class HtmlParser : public QObject
{
    Q_OBJECT
private slots:
    void mergeTags_data();
    void mergeTags();
    void append_data();
    void append();
    void prepend_data();
    void prepend();
};


void HtmlParser::mergeTags_data()
{
     QTest::addColumn<QString>("base");
     QTest::addColumn<QString>("add");
     QTest::addColumn<QString>("expect");

     QTest::newRow("1") << "<a href=\"hoge\"></a>"
                        << "<a href=\"foo\"></a>"
                        << "<a href=\"foo\"></a>";

     QTest::newRow("2") << "<a href=\"hoge\" id=\"1\"></a>"
                        << "<a href=\"foo\">bar</a>"
                        << "<a href=\"foo\" id=\"1\">bar</a>";
     
     QTest::newRow("3") << "<a href=\"hoge\"></a>"
                        << "<aa href=\"foo\"></a>"
                        << "<a href=\"hoge\"></a>";

     QTest::newRow("4") << "<a href=\"hoge\" id=\"1\">testtest!! \n</a>"
                        << "<a href=\"fuga\" data-hoge=\"sample\"><span>bar</span></a>"
                        << "<a href=\"fuga\" id=\"1\" data-hoge=\"sample\"><span>bar</span></a>";

     QTest::newRow("5") << "<a href=\"hoge\" id=\"1\">test! test...</a>"
                        << "<a href=\"fuga\" data-hoge=\"sample\"></a>"
                        << "<a href=\"fuga\" id=\"1\" data-hoge=\"sample\">test! test...</a>";

     QTest::newRow("6") << "<a href=\"hoge\" id=\"1\"><span>Thanks!!</span></a>"
                        << "<a href=\"fuga\"><p>It's OK.</p></a>"
                        << "<a href=\"fuga\" id=\"1\"><p>It's OK.</p><span>Thanks!!</span></a>";

     QTest::newRow("7") << "<a href=\"hoge\" id=\"1\"><span><b>Thanks!!</b></span></a>"
                        << "<a href=\"fuga\"><p><h3>It's OK.</h3></p></a>"
                        << "<a href=\"fuga\" id=\"1\"><p><h3>It's OK.</h3></p><span><b>Thanks!!</b></span></a>";

     QTest::newRow("8") << "<input value=\"hoge\" />"
                        << "<input value=\"\" />"
                        << "<input value=\"\" />";

     QTest::newRow("9") << "<input checked=\"1\" />"
                        << "<input checked />"
                        << "<input checked />";

     QTest::newRow("9") << "<input checked />"
                        << "<input checked=\"hoge\" />"
                        << "<input checked=\"hoge\" />";
}


void HtmlParser::mergeTags()
{
    QFETCH(QString, base);
    QFETCH(QString, add);
    QFETCH(QString, expect);

    THtmlParser actual = THtmlParser::mergeElements(base, add);
    QCOMPARE(actual.toString(), expect);
}


void HtmlParser::append_data()
{
     QTest::addColumn<QString>("base");
     QTest::addColumn<QString>("add");
     QTest::addColumn<QString>("expect");

     QTest::newRow("1") << "<a id=\"hoge\">hoge</a>"
                        << "<span id=\"foo\">foo</span>"
                        << "<a id=\"hoge\">hoge<span id=\"foo\">foo</span></a>";

     QTest::newRow("2") << "<a id=\"hoge\"><p>hoge</p></a>"
                        << "<span id=\"foo\">foo</span>"
                        << "<a id=\"hoge\"><p>hoge</p><span id=\"foo\">foo</span></a>";
     
     QTest::newRow("3") << "<a id=\"hoge\"><p>hoge</p></a>"
                        << "foo"
                        << "<a id=\"hoge\"><p>hoge</p></a>";

     QTest::newRow("4") << "<a id=\"hoge\"><p><b>hoge</b></p></a>"
                        << "<span id=\"foo\">foo</span>"
                        << "<a id=\"hoge\"><p><b>hoge</b></p><span id=\"foo\">foo</span></a>";

     QTest::newRow("5") << "<a id=\"hoge\"><p><b>hoge<b></p></aa>"
                        << "<span id=\"foo\">foo</span>"
                        << "<a id=\"hoge\"><p><b>hoge<b></p></aa><span id=\"foo\">foo</span>";
}


void HtmlParser::append()
{
    QFETCH(QString, base);
    QFETCH(QString, add);
    QFETCH(QString, expect);

    THtmlParser basep;
    basep.parse(base);
    THtmlParser addp;
    addp.parse(add);
    
    basep.append(1, addp);
    QString actual = basep.toString();
    QCOMPARE(actual, expect);
}


void HtmlParser::prepend_data()
{
     QTest::addColumn<QString>("base");
     QTest::addColumn<QString>("add");
     QTest::addColumn<QString>("expect");

     QTest::newRow("1") << "<a id=\"hoge\">hoge</a>"
                        << "<span id=\"foo\">foo</span>"
                        << "<a id=\"hoge\">hoge<span id=\"foo\">foo</span></a>";

     QTest::newRow("2") << "<a id=\"hoge\"><p>hoge</p></a>"
                        << "<span id=\"foo\">foo</span>"
                        << "<a id=\"hoge\"><span id=\"foo\">foo</span><p>hoge</p></a>";
     
     QTest::newRow("3") << "<a id=\"hoge\"><p>hoge</p></a>"
                        << "foo"
                        << "<a id=\"hoge\"><p>hoge</p></a>";

     QTest::newRow("4") << "<a id=\"hoge\"><p><b>hoge</b></p></a>"
                        << "<span id=\"foo\">foo</span>"
                        << "<a id=\"hoge\"><span id=\"foo\">foo</span><p><b>hoge</b></p></a>";

     QTest::newRow("5") << "<a id=\"hoge\"><p><b>hoge<b></p></aa>"
                        << "<span id=\"foo\">foo</span>"
                        << "<a id=\"hoge\"><span id=\"foo\">foo</span><p><b>hoge<b></p></aa>";
}


void HtmlParser::prepend()
{
    QFETCH(QString, base);
    QFETCH(QString, add);
    QFETCH(QString, expect);

    THtmlParser basep;
    basep.parse(base);
    THtmlParser addp;
    addp.parse(add);
    
    basep.prepend(1, addp);
    QString actual = basep.toString();
    QCOMPARE(actual, expect);
}

QTEST_MAIN(HtmlParser)
#include "htmlparser.moc"
