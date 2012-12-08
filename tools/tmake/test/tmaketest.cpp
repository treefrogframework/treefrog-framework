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

//static QString configPath;
extern int defaultTrimMode;


class TestTfpconverter: public QObject, public ErbConverter
{
    Q_OBJECT
public:
    TestTfpconverter() : ErbConverter(QDir("."), QDir()) { }
private slots:
    void initTestCase();
    void parse_data();
    void parse();
    void otamaconvert_data();
    void otamaconvert();
    void erbparse_data();
    void erbparse();
};


void TestTfpconverter::initTestCase()
{
    defaultTrimMode = 1;
//     configPath = QFileInfo(QCoreApplication::applicationFilePath()).absoluteDir().absolutePath() + QDir::separator() + ".." + QDir::separator() + ".." + QDir::separator() + "config" +  QDir::separator() + "treefrog.ini";
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
    QTest::newRow("7") << "data7.phtm";
    QTest::newRow("8") << "data8.phtm";
}


void TestTfpconverter::parse()
{
    QFETCH(QString, fileName);

    QFile file(fileName);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream ts(&file);
    ts.setCodec("UTF-8");
    QString html = ts.readAll();

    THtmlParser parser;
    parser.parse(html);
    QString result = parser.toString();
    
    QFile res("result.phtm");
    res.open(QIODevice::WriteOnly | QIODevice::Truncate);
    QTextStream rests(&res);
    rests.setCodec("UTF-8");
    rests << result;
    res.close();

    QCOMPARE(html, result);
}


// void TestTfpconverter::olgparser_data()
// {
//     QTest::addColumn<QString>("fileName");
//     QTest::addColumn<QString>("label");
//     QTest::addColumn<int>("count");

//     QTest::newRow("1") << "logic1.olg" << "#hoge" << 2;
// }


// void TestTfpconverter::olgparser()
// {
//     QFETCH(QString, fileName);
//     QFETCH(QString, label);
//     QFETCH(int, count);

//     QFile file(fileName);
//     QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
//     QTextStream ts(&file);
//     ts.setCodec("UTF-8");
//     QString olg = ts.readAll();


//     OlgParser parser("%%");
//     parser.parse(olg);
//     QStringList vals = parser.value(label);
//     qDebug() << vals.join(":");
//     QCOMPARE(vals.count(), count);
// }


void TestTfpconverter::otamaconvert_data()
{
    QTest::addColumn<QString>("htmlFileName");
    QTest::addColumn<QString>("olgFileName");
    QTest::addColumn<QString>("resultFileName");

    QTest::newRow("1") << "index1.html" << "logic1.olg" << "res1.html";
    QTest::newRow("2") << "index2.html" << "logic1.olg" << "res2.html";
    QTest::newRow("3") << "index3.html" << "logic1.olg" << "res3.html";
    QTest::newRow("4") << "index4.html" << "logic1.olg" << "res4.html";
    QTest::newRow("5") << "index5.html" << "logic1.olg" << "res5.html";
}


void TestTfpconverter::otamaconvert()
{
    QFETCH(QString, htmlFileName);
    QFETCH(QString, olgFileName);
    QFETCH(QString, resultFileName);

    QFile htmlFile(htmlFileName);
    QVERIFY(htmlFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream tshtml(&htmlFile);
    tshtml.setCodec("UTF-8");
  
    QFile olgFile(olgFileName);
    QVERIFY(olgFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream tsolg(&olgFile);
    tsolg.setCodec("UTF-8");

    QFile resultFile(resultFileName);
    QVERIFY(resultFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream tsres(&resultFile);
    tsres.setCodec("UTF-8");

    QString result = OtamaConverter::convertToErb(tshtml.readAll(), tsolg.readAll());
    QString expect = tsres.readAll();
    QCOMPARE(result, expect);
}


void TestTfpconverter::erbparse_data()
{
    QTest::addColumn<QString>("erb");
    QTest::addColumn<QString>("expe");

    QTest::newRow("1") << "<body>Hello ... \n</body>"
                       << "  responsebody += tr(\"<body>Hello ... \\n</body>\");\n";
    QTest::newRow("2") << "<body>Hello <%# this is comment!! %></body>"
                       << "  responsebody += tr(\"<body>Hello \");\n  /* this is comment!! */\n  responsebody += tr(\"</body>\");\n";
    QTest::newRow("3") << "<body>Hello <%# this is comment!! %>   \n</body>"
                       << "  responsebody += tr(\"<body>Hello \");\n  /* this is comment!! */\n  responsebody += tr(\"</body>\");\n";
    
    QTest::newRow("4") << "<body>Hello <%# this is \"comment!!\" %></body>"
                       << "  responsebody += tr(\"<body>Hello \");\n  /* this is \"comment!!\" */\n  responsebody += tr(\"</body>\");\n";
    QTest::newRow("5") << "<body>Hello <%# this is \"comment!!\" %>  \r\n</body>"
                       << "  responsebody += tr(\"<body>Hello \");\n  /* this is \"comment!!\" */\n  responsebody += tr(\"</body>\");\n";

    QTest::newRow("6") << "<body>Hello <% int i; %></body>"
                       << "  responsebody += tr(\"<body>Hello \");\n  int i;\n  responsebody += tr(\"</body>\");\n";
    QTest::newRow("7") << "<body>Hello <% QString s(\"%>\"); %></body>"
                       << "  responsebody += tr(\"<body>Hello \");\n  QString s(\"%>\");\n  responsebody += tr(\"</body>\");\n";
    QTest::newRow("8") << "<body>Hello <%== vvv %></body>"
                       << "  responsebody += tr(\"<body>Hello \");\n  responsebody += QVariant(vvv).toString();\n  responsebody += tr(\"</body>\");\n";
    QTest::newRow("9") << "<body>Hello <%= vvv %> \n</body>"
                       << "  responsebody += tr(\"<body>Hello \");\n  responsebody += THttpUtility::htmlEscape(vvv);\n  responsebody += tr(\" \\n</body>\");\n";
    QTest::newRow("10") << "<body>Hello <%= vvv; -%> \n</body>"
                        << "  responsebody += tr(\"<body>Hello \");\n  responsebody += THttpUtility::htmlEscape(vvv);\n  responsebody += tr(\"</body>\");\n";
    QTest::newRow("11") << "<body>Hello <% int i; -%> \r\n </body>"
                        << "  responsebody += tr(\"<body>Hello \");\n  int i;\n  responsebody += tr(\" </body>\");\n";
    QTest::newRow("12") << "<body>Hello <% int i; %> \r\n</body>"
                        << "  responsebody += tr(\"<body>Hello \");\n  int i;\n  responsebody += tr(\"</body>\");\n";
    QTest::newRow("13") << "<body>Hello ... \r\n</body>"
                        << "  responsebody += tr(\"<body>Hello ... \\r\\n</body>\");\n";

    /** echo export object **/
    QTest::newRow("14") << "<body>Hello <%=$ hoge -%> \r\n </body>"
                        << "  responsebody += tr(\"<body>Hello \");\n  tehex(hoge);\n  responsebody += tr(\" </body>\");\n";
    QTest::newRow("15") << "<body>Hello <%==$ hoge %> \r\n </body>"
                        << "  responsebody += tr(\"<body>Hello \");\n  techoex(hoge);\n  responsebody += tr(\" \\r\\n </body>\");\n";

    /** Echo a default value on ERB **/
    QTest::newRow("16") << "<body><%# comment. %|% 33 %></body>"
                        << "  responsebody += tr(\"<body>\");\n  /* comment. */\n  responsebody += tr(\"</body>\");\n";
    QTest::newRow("17") << "<body><%= number %|% 33 %></body>"
                        << "  responsebody += tr(\"<body>\");\n  { QString ___s = QVariant(number).toString(); responsebody += (___s.isEmpty()) ? THttpUtility::htmlEscape(33) : THttpUtility::htmlEscape(___s); }\n  responsebody += tr(\"</body>\");\n";
    QTest::newRow("18") << "<body><%== number %|% 33 %></body>"
                        << "  responsebody += tr(\"<body>\");\n  { QString ___s = QVariant(number).toString(); responsebody += (___s.isEmpty()) ? QVariant(33).toString() : ___s; }\n  responsebody += tr(\"</body>\");\n";
    QTest::newRow("19") << "<body><%=$number %|% 33 %></body>"
                        << "  responsebody += tr(\"<body>\");\n  tehex2(number, (33));\n  responsebody += tr(\"</body>\");\n";
    // Irregular pattern
    QTest::newRow("20") << "<body><%==$number %|% 33 -%>\t\n</body>"
                        << "  responsebody += tr(\"<body>\");\n  techoex2(number, (33));\n  responsebody += tr(\"</body>\");\n";
    QTest::newRow("21") << "<body><%== \"  %|%\" %|% \"%|%\" -%> \t \n</body>"
                        << "  responsebody += tr(\"<body>\");\n  { QString ___s = QVariant(\"  %|%\").toString(); responsebody += (___s.isEmpty()) ? QVariant(\"%|%\").toString() : ___s; }\n  responsebody += tr(\"</body>\");\n";
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


QTEST_MAIN(TestTfpconverter)
#include "tmaketest.moc"
