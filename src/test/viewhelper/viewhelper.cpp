#include <TfTest/TfTest>
#include "../../tviewhelper.h"


class ViewHelper : public TViewHelper
{
    const TActionView *actionView() const { return 0; }
};


class TestViewHelper : public QObject
{
    Q_OBJECT
private slots:
    void buttonToFunction();
    void inputTag_data();
    void inputTag();
    void textAreaTag_data();
    void textAreaTag();
    void submitTag();
    void submitImageTag();
    void resetTag();
    void imageTag();
    void stylesheetTag();
    void selectTag();
};


void TestViewHelper::buttonToFunction()
{
    ViewHelper view;
    THtmlAttribute attr;
    attr.append("onclick", "return 0;");
    attr.append("style", "none");
    QString actual = view.buttonToFunction("hoge", "function", attr);
    QString result = "<input type=\"button\" value=\"hoge\" onclick=\"function; return false;\" onclick=\"return 0;\" style=\"none\" />";
    QCOMPARE(actual, result);
}


void TestViewHelper::inputTag_data()
{
    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("value");
    QTest::addColumn<THtmlAttribute>("attr");
    QTest::addColumn<QString>("result");

    THtmlAttribute attr;
    attr.append("onclick", "return 0;");
    attr.append("style", "none");
    QTest::newRow("1") << "hidden" << "hoge" << "50" << attr
                       << "<input type=\"hidden\" name=\"hoge\" value=\"50\" onclick=\"return 0;\" style=\"none\" />";
}


void TestViewHelper::inputTag()
{
    QFETCH(QString, type);
    QFETCH(QString, name);
    QFETCH(QString, value);
    QFETCH(THtmlAttribute, attr);
    QFETCH(QString, result);

    ViewHelper view;
    QString actual = view.inputTag(type, name, value, attr);
    QCOMPARE(actual, result);
}


void TestViewHelper::textAreaTag_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("columns");
    QTest::addColumn<QString>("content");
    QTest::addColumn<THtmlAttribute>("attr");
    QTest::addColumn<QString>("result");

    THtmlAttribute attr;
    attr.append("onclick", "return 0;");
    attr.append("style", "none");
    QTest::newRow("1") << "hoge" << 30 << 20 << "hello" << attr
                       << "<textarea name=\"hoge\" rows=\"30\" cols=\"20\" onclick=\"return 0;\" style=\"none\">hello</textarea>";
}


void TestViewHelper::textAreaTag()
{
    QFETCH(QString, name);
    QFETCH(int, rows);
    QFETCH(int, columns);
    QFETCH(QString, content);
    QFETCH(THtmlAttribute, attr);
    QFETCH(QString, result);

    ViewHelper view;
    QString actual = view.textAreaTag(name, rows, columns, content, attr);
    QCOMPARE(actual, result);
}


void TestViewHelper::submitTag()
{
    ViewHelper view;
    THtmlAttribute attr;
    attr.append("onclick", "return 0;");
    attr.append("style", "none");
    QString actual = view.submitTag("send", attr);
    QString result = "<input type=\"submit\" value=\"send\" onclick=\"return 0;\" style=\"none\" />";
    QCOMPARE(actual, result);
}


void TestViewHelper::submitImageTag()
{
    ViewHelper view;
    THtmlAttribute attr;
    attr.append("onclick", "return 0;");
    attr.append("style", "none");
    QString actual = view.submitImageTag("hoge.png", attr);
    QString result = "<input type=\"image\" src=\"/images/hoge.png\" onclick=\"return 0;\" style=\"none\" />";
    QCOMPARE(actual, result);
}


void TestViewHelper::resetTag()
{
    ViewHelper view;
    THtmlAttribute attr;
    attr.append("onclick", "return 0;");
    attr.append("style", "none");
    QString actual = view.resetTag("hoge", attr);
    QString result = "<input type=\"reset\" value=\"hoge\" onclick=\"return 0;\" style=\"none\" />";
    QCOMPARE(actual, result);
}

void TestViewHelper::imageTag()
{
    ViewHelper view;
    THtmlAttribute attr;
    attr.append("onclick", "return 0;");
    attr.append("style", "none");
    QString actual = view.imageTag("hoge.png", QSize(100, 200), "stop", attr);
    QString result = "<img src=\"/images/hoge.png\" width=\"100\" height=\"200\" alt=\"stop\" onclick=\"return 0;\" style=\"none\" />";
    QCOMPARE(actual, result);
}

void TestViewHelper::stylesheetTag()
{
    ViewHelper view;
    THtmlAttribute attr;
    attr.append("onclick", "return 0;");
    attr.append("style", "none");
    QString actual = view.styleSheetTag("hoge.png", attr);
    QString result = "<link href=\"/css/hoge.png\" rel=\"stylesheet\" type=\"text/css\" onclick=\"return 0;\" style=\"none\" />";
    QCOMPARE(actual, result);
}

void TestViewHelper::selectTag()
{
    ViewHelper view;
    QString actual1 = view.selectTag("book", 5);
    QString actual2 = view.endTag();
    QString result1 = "<select name=\"book\" size=\"5\">";
    QString result2 = "</select>";
    QCOMPARE(actual1, result1);
    QCOMPARE(actual2, result2);
}

//TF_TEST_SQLLESS_MAIN(TestViewHelper)
TF_TEST_MAIN(TestViewHelper)
#include "viewhelper.moc"
