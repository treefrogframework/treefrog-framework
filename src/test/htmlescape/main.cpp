#include <QtTest/QtTest>
#include <QFile>
#include <THttpUtility>


class HtmlParser : public QObject
{
    Q_OBJECT
private slots:
    void escapeCompat_data();
    void escapeCompat();
    void escapeQuotes_data();
    void escapeQuotes();
    void escapeNoQuotes_data();
    void escapeNoQuotes();
};


void HtmlParser::escapeCompat_data()
{
     QTest::addColumn<QString>("string");
     QTest::addColumn<QString>("correct");

     QTest::newRow("1") << tr(u8"こんにちは")
                        << tr(u8"こんにちは");
     QTest::newRow("2") << tr(u8"asdfma;lsdfjine^-~][]:+_?.1")
                        << tr(u8"asdfma;lsdfjine^-~][]:+_?.1");
     QTest::newRow("3") << "<a href=\"hoge\">a & b</a>"
                        << "&lt;a href=&quot;hoge&quot;&gt;a &amp; b&lt;/a&gt;";
     QTest::newRow("4") << "A 'quote' is <b>bold</b>"
                        << "A 'quote' is &lt;b&gt;bold&lt;/b&gt;";
}

void HtmlParser::escapeCompat()
{
    QFETCH(QString, string);
    QFETCH(QString, correct);
    QString actualStr = THttpUtility::htmlEscape(string, Tf::Compatible);
    QCOMPARE(actualStr, correct);
}

void HtmlParser::escapeQuotes_data()
{
     QTest::addColumn<QString>("string");
     QTest::addColumn<QString>("correct");

     QTest::newRow("1") << tr(u8"こんにちは")
                        << tr(u8"こんにちは");
     QTest::newRow("2") << tr(u8"asdfma;lsdfjine^-~][]:+_?.1")
                        << tr(u8"asdfma;lsdfjine^-~][]:+_?.1");
     QTest::newRow("3") << "<a href=\"hoge\">a & b</a>"
                        << "&lt;a href=&quot;hoge&quot;&gt;a &amp; b&lt;/a&gt;";;
     QTest::newRow("4") << "A 'quote' is <b>bold</b>"
                        << "A &#039;quote&#039; is &lt;b&gt;bold&lt;/b&gt;";
}

void HtmlParser::escapeQuotes()
{
    QFETCH(QString, string);
    QFETCH(QString, correct);
    QString actualStr = THttpUtility::htmlEscape(string, Tf::Quotes);
    QCOMPARE(actualStr, correct);
}

void HtmlParser::escapeNoQuotes_data()
{
     QTest::addColumn<QString>("string");
     QTest::addColumn<QString>("correct");

     QTest::newRow("1") << tr(u8"こんにちは")
                        << tr(u8"こんにちは");
     QTest::newRow("2") << tr(u8"asdfma;lsdfjine^-~][]:+_?.1")
                        << tr(u8"asdfma;lsdfjine^-~][]:+_?.1");
     QTest::newRow("3") << "<a href=\"hoge\">a & b</a>"
                        << "&lt;a href=\"hoge\"&gt;a &amp; b&lt;/a&gt;";
     QTest::newRow("4") << "A 'quote' is <b>bold</b>"
                        << "A 'quote' is &lt;b&gt;bold&lt;/b&gt;";
}

void HtmlParser::escapeNoQuotes()
{
    QFETCH(QString, string);
    QFETCH(QString, correct);
    QString actualStr = THttpUtility::htmlEscape(string, Tf::NoQuotes);
    QCOMPARE(actualStr, correct);
}

QTEST_MAIN(HtmlParser)
#include "main.moc"
