#include <QtTest/QtTest>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <iostream>
#include <THtmlParser>
#include "otmparser.h"
#include "otamaconverter.h"
#include "erbconverter.h"
#include "erbparser.h"

extern int defaultTrimMode;


class TestTfpconverter: public QObject, public ErbConverter
{
    Q_OBJECT
public:
    TestTfpconverter() : ErbConverter(QDir("."), QDir(), QDir()) { }
private slots:
    void initTestCase();
    void parse_data();
    void parse();
    void otamaconvert_data();
    void otamaconvert();
    void otamaconvertStrong_data();
    void otamaconvertStrong();
    void erbparse_data();
    void erbparse();
    void erbparseStrong_data();
    void erbparseStrong();
};


void TestTfpconverter::initTestCase()
{
    defaultTrimMode = 1;
}


void TestTfpconverter::parse_data()
{
    QTest::addColumn<QString>("fileName");

    QTest::newRow("1") << "data1.phtm";
    QTest::newRow("2") << "data2.phtm";
    QTest::newRow("3") << "data3.phtm";
    QTest::newRow("4") << "data4.phtm";
    QTest::newRow("5") << "data5.phtm";
    QTest::newRow("6") << "data6.phtm";
    QTest::newRow("6-1") << "data6-1.phtm";
    QTest::newRow("7") << "data7.phtm";
    QTest::newRow("8") << "data8.phtm";
}


void TestTfpconverter::parse()
{
    QFETCH(QString, fileName);

    QFile file(fileName);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream ts(&file);
#if QT_VERSION < 0x060000
    ts.setCodec("UTF-8");
#endif
    QString html = ts.readAll();

    THtmlParser parser;
    parser.parse(html);
    QString result = parser.toString();

    QFile res("result.phtm");
    res.open(QIODevice::WriteOnly | QIODevice::Truncate);
    QTextStream rests(&res);
#if QT_VERSION < 0x060000
    rests.setCodec("UTF-8");
#endif
    rests << result;
    res.close();

    QCOMPARE(html, result);
}


void TestTfpconverter::otamaconvert_data()
{
    QTest::addColumn<QString>("htmlFileName");
    QTest::addColumn<QString>("olgFileName");
    QTest::addColumn<QString>("resultFileName");

    QTest::newRow("1")   << "index1.html"  << "logic1.olg" << "res1.html";
    QTest::newRow("1-2") << "index1-2.html" << "logic1.olg" << "res1-2.html";
    QTest::newRow("2")  << "index2.html"  << "logic1.olg" << "res2.html";
    QTest::newRow("3")  << "index3.html"  << "logic1.olg" << "res3.html";
    QTest::newRow("4")  << "index4.html"  << "logic1.olg" << "res4.html";
    QTest::newRow("5")  << "index5.html"  << "logic1.olg" << "res5.html";
    QTest::newRow("6")  << "index6.html"  << "logic1.olg" << "res6.html";
    QTest::newRow("7")  << "index7.html"  << "logic1.olg" << "res7.html";
    QTest::newRow("8")  << "index8.html"  << "logic1.olg" << "res8.html";
    QTest::newRow("9")  << "index9.html"  << "logic1.olg" << "res9.html";
    QTest::newRow("10") << "index10.html" << "logic1.olg" << "res10.html";
    QTest::newRow("11") << "index11.html" << "logic1.olg" << "res11.html";
    QTest::newRow("12") << "index12.html" << "logic1.olg" << "res12.html";
    QTest::newRow("13") << "index13.html" << "logic1.olg" << "res13.html";
    QTest::newRow("14") << "index14.html" << "logic1.olg" << "res14.html";
    QTest::newRow("14-2") << "index14-2.html" << "logic1.olg" << "res14-2.html";
    QTest::newRow("14-3") << "index14-3.html" << "logic1.olg" << "res14-3.html";
    QTest::newRow("14-4") << "index14-4.html" << "logic1.olg" << "res14-4.html";
    QTest::newRow("15") << "index15.html" << "logic1.olg" << "res15.html";
    QTest::newRow("16") << "index16.html" << "logic1.olg" << "res16.html";
    QTest::newRow("17") << "index17.html" << "logic1.olg" << "res17.html";
    QTest::newRow("18") << "index18.html" << "logic1.olg" << "res18.html";
    QTest::newRow("19") << "index19.html" << "logic1.olg" << "res19.html";
    QTest::newRow("19") << "index19.html" << "logic1.olg" << "res19.html";
    QTest::newRow("20") << "index20.html" << "logic1.olg" << "res20.html";

    QTest::newRow("c1") << "indexc1.html" << "logic1.olg" << "resc1.html";
    QTest::newRow("c2") << "indexc2.html" << "logic1.olg" << "resc2.html";
    QTest::newRow("c3") << "indexc3.html" << "logic1.olg" << "resc3.html";
    QTest::newRow("c4") << "indexc4.html" << "logic1.olg" << "resc4.html";

    QTest::newRow("dm") << "dummy.html"  << "logic1.olg" << "resdm.html";
}


void TestTfpconverter::otamaconvert()
{
    QFETCH(QString, htmlFileName);
    QFETCH(QString, olgFileName);
    QFETCH(QString, resultFileName);

    QFile htmlFile(htmlFileName);
    QVERIFY(htmlFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream tshtml(&htmlFile);
#if QT_VERSION < 0x060000
    tshtml.setCodec("UTF-8");
#endif

    QFile olgFile(olgFileName);
    QVERIFY(olgFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream tsolg(&olgFile);
#if QT_VERSION < 0x060000
    tsolg.setCodec("UTF-8");
#endif

    QFile resultFile(resultFileName);
    QVERIFY(resultFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream tsres(&resultFile);
#if QT_VERSION < 0x060000
    tsres.setCodec("UTF-8");
#endif

    QString result = OtamaConverter::convertToErb(tshtml.readAll(), tsolg.readAll(), 1);
    QString expect = tsres.readAll();
    QCOMPARE(result, expect);
}


void TestTfpconverter::otamaconvertStrong_data()
{
    QTest::addColumn<QString>("htmlFileName");
    QTest::addColumn<QString>("olgFileName");
    QTest::addColumn<QString>("resultFileName");

    QTest::newRow("1")   << "index1.html"   << "logic1.olg" << "res1st.html";
    QTest::newRow("1-2") << "index1-2.html" << "logic1.olg" << "res1-2st.html";
    QTest::newRow("2")   << "index2.html"   << "logic1.olg" << "res2st.html";
    QTest::newRow("3")   << "index3.html"   << "logic1.olg" << "res3st.html";
    QTest::newRow("4")   << "index4.html"   << "logic1.olg" << "res4st.html";
    QTest::newRow("5")   << "index5.html"   << "logic1.olg" << "res5st.html";
    QTest::newRow("6")   << "index6.html"   << "logic1.olg" << "res6st.html";
    QTest::newRow("7")   << "index7.html"   << "logic1.olg" << "res7st.html";
    QTest::newRow("8")   << "index8.html"   << "logic1.olg" << "res8st.html";
    QTest::newRow("9")   << "index9.html"   << "logic1.olg" << "res9st.html";
    QTest::newRow("10")  << "index10.html"  << "logic1.olg" << "res10st.html";

    QTest::newRow("20") << "index20.html" << "logic1.olg" << "res20st.html";
}


void TestTfpconverter::otamaconvertStrong()
{
    QFETCH(QString, htmlFileName);
    QFETCH(QString, olgFileName);
    QFETCH(QString, resultFileName);

    QFile htmlFile(htmlFileName);
    QVERIFY(htmlFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream tshtml(&htmlFile);
#if QT_VERSION < 0x060000
    tshtml.setCodec("UTF-8");
#endif

    QFile olgFile(olgFileName);
    QVERIFY(olgFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream tsolg(&olgFile);
#if QT_VERSION < 0x060000
    tsolg.setCodec("UTF-8");
#endif

    QFile resultFile(resultFileName);
    QVERIFY(resultFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream tsres(&resultFile);
#if QT_VERSION < 0x060000
    tsres.setCodec("UTF-8");
#endif

    QString result = OtamaConverter::convertToErb(tshtml.readAll(), tsolg.readAll(), 2);
    QString expect = tsres.readAll();
    QCOMPARE(result, expect);
}


void TestTfpconverter::erbparse_data()
{
    QTest::addColumn<QString>("erb");
    QTest::addColumn<QString>("expe");

    QTest::newRow("1") << "<body>Hello ... \n</body>"
                       << "  responsebody += QStringLiteral(\"<body>Hello ... \\n</body>\");\n";
    QTest::newRow("1-2") << "  <body>Hello ... \n</body> \t"
                         << "  responsebody += QStringLiteral(\"  <body>Hello ... \\n</body> \t\");\n";
    QTest::newRow("2") << "<body>Hello <%# this is comment!! %></body>"
                       << "  responsebody += QStringLiteral(\"<body>Hello \");\n  /* this is comment!! */\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("3") << "<body>Hello <%# this is comment!! %>   \n</body>"
                       << "  responsebody += QStringLiteral(\"<body>Hello \");\n  /* this is comment!! */\n  responsebody += QStringLiteral(\"</body>\");\n";

    QTest::newRow("4") << "<body>Hello <%# this is \"comment!!\" %></body>"
                       << "  responsebody += QStringLiteral(\"<body>Hello \");\n  /* this is \"comment!!\" */\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("5") << "<body>Hello <%# this is \"comment!!\" %>  \r\n</body>"
                       << "  responsebody += QStringLiteral(\"<body>Hello \");\n  /* this is \"comment!!\" */\n  responsebody += QStringLiteral(\"</body>\");\n";

    QTest::newRow("6") << "<body>Hello <% int i; %></body>"
                       << "  responsebody += QStringLiteral(\"<body>Hello \");\n  int i;\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("7") << "<body>Hello <% QString s(\"%>\"); %></body>"
                       << "  responsebody += QStringLiteral(\"<body>Hello \");\n  QString s(\"%>\");\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("8") << "<body>Hello <%== vvv %></body>"
                       << "  responsebody += QStringLiteral(\"<body>Hello \");\n  responsebody += QVariant(vvv).toString();\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("9") << "<body>Hello <%= vvv %> \n</body>"
                       << "  responsebody += QStringLiteral(\"<body>Hello \");\n  responsebody += THttpUtility::htmlEscape(vvv);\n  responsebody += QStringLiteral(\" \\n</body>\");\n";
    QTest::newRow("10") << "<body>Hello <%= vvv; -%> \n</body>"
                        << "  responsebody += QStringLiteral(\"<body>Hello \");\n  responsebody += THttpUtility::htmlEscape(vvv);\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("11") << "<body>Hello <% int i; -%> \r\n </body>"
                        << "  responsebody += QStringLiteral(\"<body>Hello \");\n  int i;\n  responsebody += QStringLiteral(\" </body>\");\n";
    QTest::newRow("12") << "<body>Hello <% int i; %> \r\n</body>"
                        << "  responsebody += QStringLiteral(\"<body>Hello \");\n  int i;\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("13") << "<body>Hello ... \r\n</body>"
                        << "  responsebody += QStringLiteral(\"<body>Hello ... \\r\\n</body>\");\n";
    QTest::newRow("14") << "<body>Hello <%= vvv; +%> \n</body>"
                        << "  responsebody += QStringLiteral(\"<body>Hello \");\n  responsebody += THttpUtility::htmlEscape(vvv);\n  responsebody += QStringLiteral(\" \\n</body>\");\n";
    QTest::newRow("15") << "<body>Hello <%= vvv; +%></body>\r\n"
                        << "  responsebody += QStringLiteral(\"<body>Hello \");\n  responsebody += THttpUtility::htmlEscape(vvv);\n  responsebody += QStringLiteral(\"</body>\\r\\n\");\n";
    QTest::newRow("16") << "<body>Hello <% int i; +%> \r\n </body>"
                        << "  responsebody += QStringLiteral(\"<body>Hello \");\n  int i;\n  responsebody += QStringLiteral(\" \\r\\n </body>\");\n";

    /** echo export object **/
    QTest::newRow("17") << "<body>Hello <%=$ hoge -%> \r\n </body>"
                        << "  responsebody += QStringLiteral(\"<body>Hello \");\n  tehex(hoge);\n  responsebody += QStringLiteral(\" </body>\");\n";
    QTest::newRow("18") << "<body>Hello <%==$ hoge %> \r\n </body>"
                        << "  responsebody += QStringLiteral(\"<body>Hello \");\n  techoex(hoge);\n  responsebody += QStringLiteral(\" \\r\\n </body>\");\n";

    /** Echo a default value on ERB **/
    QTest::newRow("19") << "<body><%# comment. %|% 33 %></body>"
                        << "  responsebody += QStringLiteral(\"<body>\");\n  /* comment. */\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("20") << "<body><%= number %|% 33 %></body>"
                        << "  responsebody += QStringLiteral(\"<body>\");\n  { QString ___s = QVariant(number).toString(); responsebody += (___s.isEmpty()) ? THttpUtility::htmlEscape(33) : THttpUtility::htmlEscape(___s); }\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("21") << "<body><%== number %|% 33 %></body>"
                        << "  responsebody += QStringLiteral(\"<body>\");\n  { QString ___s = QVariant(number).toString(); responsebody += (___s.isEmpty()) ? QVariant(33).toString() : ___s; }\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("22") << "<body><%=$number %|% 33 %></body>"
                        << "  responsebody += QStringLiteral(\"<body>\");\n  tehex2(number, (33));\n  responsebody += QStringLiteral(\"</body>\");\n";
    // Irregular pattern
    QTest::newRow("23") << "<body><%==$number %|% 33 -%>\t\n</body>"
                        << "  responsebody += QStringLiteral(\"<body>\");\n  techoex2(number, (33));\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("24") << "<body><%== \"  %|%\" %|% \"%|%\" -%> \t \n</body>"
                        << "  responsebody += QStringLiteral(\"<body>\");\n  { QString ___s = QVariant(\"  %|%\").toString(); responsebody += (___s.isEmpty()) ? QVariant(\"%|%\").toString() : ___s; }\n  responsebody += QStringLiteral(\"</body>\");\n";

    QTest::newRow("25") << "<body><script>function() { return '\\n'; }</script></body>"
                        << "  responsebody += QStringLiteral(\"<body><script>function() { return '\\\\n'; }</script></body>\");\n";
    QTest::newRow("26") << "<body><script>function() { return \"\\n\"; }</script></body>"
                        << "  responsebody += QStringLiteral(\"<body><script>function() { return \\\"\\\\n\\\"; }</script></body>\");\n";
}


void TestTfpconverter::erbparse()
{
    QFETCH(QString, erb);
    QFETCH(QString, expe);

    ErbParser parser(ErbParser::NormalTrim);
    parser.parse(erb);
    QString result = parser.sourceCode();
    QCOMPARE(result, expe);
}


void TestTfpconverter::erbparseStrong_data()
{
    QTest::addColumn<QString>("erb");
    QTest::addColumn<QString>("expe");

    QTest::newRow("1") << "<body>Hello ... \n</body>"
                       << "  responsebody += QStringLiteral(\"<body>Hello ...\\n</body>\");\n";
    QTest::newRow("1-2") << "<body>Hello ... \n \t</body>"
                         << "  responsebody += QStringLiteral(\"<body>Hello ...\\n</body>\");\n";
    QTest::newRow("2") << "<body>Hello <%# this is comment!! %></body>"
                       << "  responsebody += QStringLiteral(\"<body>Hello \");\n  /* this is comment!! */\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("3") << "<body>Hello <%# this is comment!! %>   \n</body>"
                       << "  responsebody += QStringLiteral(\"<body>Hello \");\n  /* this is comment!! */\n  responsebody += QStringLiteral(\"</body>\");\n";

    QTest::newRow("4") << "<body>Hello <%# this is \"comment!!\" %></body>"
                       << "  responsebody += QStringLiteral(\"<body>Hello \");\n  /* this is \"comment!!\" */\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("5") << "<body>Hello <%# this is \"comment!!\" %>  \r\n</body>"
                       << "  responsebody += QStringLiteral(\"<body>Hello \");\n  /* this is \"comment!!\" */\n  responsebody += QStringLiteral(\"</body>\");\n";

    QTest::newRow("6") << "<body>Hello <% int i; %></body>"
                       << "  responsebody += QStringLiteral(\"<body>Hello \");\n  int i;\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("7") << "<body>Hello <% QString s(\"%>\"); %></body>"
                       << "  responsebody += QStringLiteral(\"<body>Hello \");\n  QString s(\"%>\");\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("8") << "<body>Hello <%== vvv %></body>"
                       << "  responsebody += QStringLiteral(\"<body>Hello \");\n  responsebody += QVariant(vvv).toString();\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("9") << "<body>Hello <%= vvv %> \n</body>"
                       << "  responsebody += QStringLiteral(\"<body>Hello \");\n  responsebody += THttpUtility::htmlEscape(vvv);\n  responsebody += QStringLiteral(\"\\n</body>\");\n";
    QTest::newRow("9-2") << "<body>Hello <%= vvv %>　\n</body>"
                         << "  responsebody += QStringLiteral(\"<body>Hello \");\n  responsebody += THttpUtility::htmlEscape(vvv);\n  responsebody += tr(\"　\\n</body>\");\n";
    QTest::newRow("10") << "<body>Hello <%= vvv; -%>  \n</body>"
                        << "  responsebody += QStringLiteral(\"<body>Hello \");\n  responsebody += THttpUtility::htmlEscape(vvv);\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("11") << "  <body>Hello <% int i; -%> \r\n </body>  "
                        << "  responsebody += QStringLiteral(\"<body>Hello \");\n  int i;\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("12") << "<body>Hello <% int i; %> \r\n</body>"
                        << "  responsebody += QStringLiteral(\"<body>Hello \");\n  int i;\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("13") << "<body>Hello ... \t\r\n\t</body>"
                        << "  responsebody += QStringLiteral(\"<body>Hello ...\\n</body>\");\n";
    QTest::newRow("14") << "<body>Hello <%= vvv; +%> \n</body>"
                        << "  responsebody += QStringLiteral(\"<body>Hello \");\n  responsebody += THttpUtility::htmlEscape(vvv);\n  responsebody += QStringLiteral(\"\\n</body>\");\n";
    QTest::newRow("15") << "<body>Hello <%= vvv; +%></body>\t\r\n"
                        << "  responsebody += QStringLiteral(\"<body>Hello \");\n  responsebody += THttpUtility::htmlEscape(vvv);\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("16") << " \t<body>Hello <% int i; +%> \r\n </body>"
                        << "  responsebody += QStringLiteral(\"<body>Hello \");\n  int i;\n  responsebody += QStringLiteral(\"\\n</body>\");\n";

    /** echo export object **/
    QTest::newRow("17") << " \t <body>Hello <%=$ hoge -%> \r\n </body>"
                        << "  responsebody += QStringLiteral(\"<body>Hello \");\n  tehex(hoge);\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("18") << "<body>Hello <%==$ hoge %> \r\n </body>"
                        << "  responsebody += QStringLiteral(\"<body>Hello \");\n  techoex(hoge);\n  responsebody += QStringLiteral(\"\\n</body>\");\n";

    /** Echo a default value on ERB **/
    QTest::newRow("19") << "<body><%# comment. %|% 33 %></body>"
                        << "  responsebody += QStringLiteral(\"<body>\");\n  /* comment. */\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("20") << "<body><%= number %|% 33 %></body>"
                        << "  responsebody += QStringLiteral(\"<body>\");\n  { QString ___s = QVariant(number).toString(); responsebody += (___s.isEmpty()) ? THttpUtility::htmlEscape(33) : THttpUtility::htmlEscape(___s); }\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("21") << "<body><%== number %|% 33 %></body>"
                        << "  responsebody += QStringLiteral(\"<body>\");\n  { QString ___s = QVariant(number).toString(); responsebody += (___s.isEmpty()) ? QVariant(33).toString() : ___s; }\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("22") << "<body><%=$number %|% 33 %></body>"
                        << "  responsebody += QStringLiteral(\"<body>\");\n  tehex2(number, (33));\n  responsebody += QStringLiteral(\"</body>\");\n";
    // Irregular pattern
    QTest::newRow("23") << "<body><%==$number %|% 33 -%>\t\n</body>"
                        << "  responsebody += QStringLiteral(\"<body>\");\n  techoex2(number, (33));\n  responsebody += QStringLiteral(\"</body>\");\n";
    QTest::newRow("24") << "<body><%== \"  %|%\" %|% \"%|%\" -%> \t \n</body>"
                        << "  responsebody += QStringLiteral(\"<body>\");\n  { QString ___s = QVariant(\"  %|%\").toString(); responsebody += (___s.isEmpty()) ? QVariant(\"%|%\").toString() : ___s; }\n  responsebody += QStringLiteral(\"</body>\");\n";

    QTest::newRow("25") << "<body><script>function() { return '\\n'; }</script></body>"
                        << "  responsebody += QStringLiteral(\"<body><script>function() { return '\\\\n'; }</script></body>\");\n";
    QTest::newRow("26") << "<body><script>function() { return \"\\n\"; }</script></body>"
                        << "  responsebody += QStringLiteral(\"<body><script>function() { return \\\"\\\\n\\\"; }</script></body>\");\n";
}


void TestTfpconverter::erbparseStrong()
{
    QFETCH(QString, erb);
    QFETCH(QString, expe);

    ErbParser parser(ErbParser::StrongTrim);
    parser.parse(erb);
    QString result = parser.sourceCode();
    QCOMPARE(result, expe);
}


QTEST_MAIN(TestTfpconverter)
#include "tmaketest.moc"
