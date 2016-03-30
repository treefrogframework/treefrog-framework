#include <TfTest/TfTest>
#include <QJSEngine>
#include <QJSValue>
#include "../../tjscontext.h"


class BenchMark : public QObject
{
    Q_OBJECT
private slots:
    void eval_data();
    void eval();
    void callFunc_data();
    void callFunc();
    void callFunc1_data();
    void callFunc1();
    void transform_data();
    void transform();
    void benchCall();
};


void BenchMark::eval_data()
{
    QTest::addColumn<QString>("expr");
    QTest::addColumn<QString>("output");

    QTest::newRow("01") << "1 + 2" << "3";
    QTest::newRow("02") << "'hello' + ' world!'" << "hello world!";
    QTest::newRow("03") << "(function(a){return a*a;})(5)" << "25";
    QTest::newRow("04") << "(function(s){return s+s;})('orange')" << "orangeorange";
}


void BenchMark::eval()
{
    QFETCH(QString, expr);
    QFETCH(QString, output);

    TJSContext js;
    auto result = js.evaluate(expr).toString();
    QCOMPARE(result, output);
}


void BenchMark::callFunc_data()
{
    QTest::addColumn<QString>("func");
    QTest::addColumn<QString>("output");

    QTest::newRow("01") << "function(){return 1+2;}" << "3";
    QTest::newRow("02") << "function(){return 'hello'+' world!';}" << "hello world!";
    QTest::newRow("03") << "function(){return \"hello\"+\" world.\";}" << "hello world.";
}


void BenchMark::callFunc()
{
    QFETCH(QString, func);
    QFETCH(QString, output);

    TJSContext js;
    auto result = js.call(func).toString();
    QCOMPARE(result, output);
}

void BenchMark::callFunc1_data()
{
    QTest::addColumn<QString>("func");
    QTest::addColumn<QString>("arg");
    QTest::addColumn<QString>("output");

    QTest::newRow("01") << "function(a){return a*a;}" << "4" << "16";
    QTest::newRow("01") << "function(s){return 'Hello '+s;}" << "world" << "Hello world";
}


void BenchMark::callFunc1()
{
    QFETCH(QString, func);
    QFETCH(QString, arg);
    QFETCH(QString, output);

    TJSContext js;
    auto result = js.call(func, arg).toString();
    QCOMPARE(result, output);
}


void BenchMark::transform_data()
{
    QTest::addColumn<QString>("jsx");
    QTest::addColumn<QString>("output");

    QTest::newRow("01") << "<HelloWorld />" << "React.createElement(HelloWorld, null)";
    QTest::newRow("02") << "<HelloWorld/>" << "React.createElement(HelloWorld, null)";
    QTest::newRow("03") << "<hello/>" << "React.createElement(\"hello\", null)";
}


void BenchMark::transform()
{
    QFETCH(QString, jsx);
    QFETCH(QString, output);

    TJSContext js;
    js.load("JSXTransformer.js");

    auto result = js.call("JSXTransformer.transform", jsx).property("code").toString();
    QCOMPARE(result, output);
}


void BenchMark::benchCall()
{
    TJSContext js;
    js.load("JSXTransformer.js");

    QBENCHMARK {
        auto res = js.call("JSXTransformer.transform", QString("<HelloWorld />"));
        //qDebug() << res.property("code").toString();
    }
}

TF_TEST_MAIN(BenchMark)
#include "main.moc"
