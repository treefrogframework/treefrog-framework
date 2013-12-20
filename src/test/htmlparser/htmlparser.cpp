#include <QtTest/QtTest>
#include <QFile>
#include <THtmlParser>


class HtmlParser : public QObject
{
    Q_OBJECT
private slots:
    void parse_data();
    void parse();
    void mergeTags_data();
    void mergeTags();
    void append_data();
    void append();
    void prepend_data();
    void prepend();
    void tagcheck_data();
    void tagcheck();
};


void HtmlParser::parse_data()
{
    QTest::addColumn<QString>("html");
    QTest::addColumn<int>("index");
    QTest::addColumn<QString>("tagstr");

    QTest::newRow("01") << "<a href=\"hoge\"></a>" << 1
                        << "<a href=\"hoge\">";
    QTest::newRow("02") << "<input checked /><div> </div>" << 1
                        << "<input checked />";
    QTest::newRow("03") << "<input checked /><div> </div>" << 2
                        << "<div> ";
    QTest::newRow("04") << "<a href=\"hoge\"><span style=\"color\">hoge</span></a>" << 2
                        << "<span style=\"color\">hoge";
    QTest::newRow("05") << "<select data-tf=\"@prefecture\">\n<!-- <option value=\"hoge\">hoge</option>--> </select>" << 1
                        << "<select data-tf=\"@prefecture\">\n<!-- ";
    QTest::newRow("06") << "<select data-tf=\"@prefecture\">\n<!-- <option value=\"hoge\">hoge</option>--> </select>" << 2
                        << "<option value=\"hoge\">hoge";
    QTest::newRow("07") << "<select data-tf=\"@prefecture\">\n<!-- <option value=\"hoge\">hoge</option>" << 3
                        << "";
}


void HtmlParser::parse()
{
    QFETCH(QString, html);
    QFETCH(int, index);
    QFETCH(QString, tagstr);

    THtmlParser parser;
    parser.parse(html);
    THtmlElement e = parser.at(index);
    QCOMPARE(e.toString(), tagstr);
}


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

     QTest::newRow("10") << "<div>\n<!-- Hello--></div>"
                         << "<div><span>Hi</span></div>"
                         << "<div><span>Hi</span></div>";
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


void HtmlParser::tagcheck_data()
{
    QTest::addColumn<QString>("tag");
    QTest::addColumn<bool>("ok");
    // OK
    QTest::newRow("1")  << "<html>" << true;
    QTest::newRow("2")  << "<html >" << true;
    QTest::newRow("3")  << "<html />" << true;
    QTest::newRow("4")  << "<HTML/>" << true;
    QTest::newRow("5")  << "<html name=\"hoge\">" << true;
    QTest::newRow("6")  << "<Html name='hoge'>" << true;
    QTest::newRow("7")  << "<html \"default\" >" << true;
    QTest::newRow("8")  << "<a name=\"hoge; return;\" >" << true;
    QTest::newRow("9")  << "</html>" << true;
    QTest::newRow("10") << "<hoge value=\"hoge\"/>" << true;
    QTest::newRow("11") << "<hoge value=\"hoge\" />" << true;
    QTest::newRow("12") << "<h1>" << true;
    QTest::newRow("13") << "</h1 >" << true;
    QTest::newRow("14") << "<h3/>" << true;
    QTest::newRow("15") << "<h4 value=\"hoge\" />" << true;
    QTest::newRow("16") << "<scr ' + 'i=tr' + '>" << true;

    // NG
    QTest::newRow("50") << "<!doctype html>" << false; // sepecial tag, not tag here
    QTest::newRow("51") << "<html '>" << false;
    QTest::newRow("52") << "<html \">" << false;
    QTest::newRow("53") << "<i; ++i){for (j = 0; j>" << false;
    QTest::newRow("54") << "<i; ++i)\nfor(j=0;j>" << false;
    QTest::newRow("55") << "<i; ++i) if(j>" << false;
    QTest::newRow("56") << "<!-->" << false;
    QTest::newRow("57") << "<!-- >" << false;
    QTest::newRow("58") << "<!>" << false;
    QTest::newRow("58") << "<? hoge ?>" << false;
    QTest::newRow("58") << "<% hoge %>" << false;
    QTest::newRow("59") << "<!--\n>" << false;
    QTest::newRow("60") << "<i) echo'>'" << false;
    QTest::newRow("61") << "<i) echo \">\"" << false;
    QTest::newRow("62") << "<'<scr' + 'i=tr' + '></s' //-->" << false;
    QTest::newRow("63") << "</s' //-->" << false;
    QTest::newRow("64") << "<'<scr' + 'i=tr' + '>"  << false;
    QTest::newRow("65") << "<scr' + 'i=tr' + '>"  << false;
    QTest::newRow("66") << "<scr ' + 'i=tr' >"  << false;
}

void HtmlParser::tagcheck()
{
    QFETCH(QString, tag);
    QFETCH(bool, ok);

    bool result = THtmlParser::isTag(tag);
    QCOMPARE(result, ok);
}

QTEST_MAIN(HtmlParser)
#include "htmlparser.moc"
