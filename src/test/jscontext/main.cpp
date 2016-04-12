#include <TfTest/TfTest>
#include <QJSEngine>
#include <QJSValue>
#include "../../tjscontext.h"


class JSContext : public QObject
{
    Q_OBJECT
    QString jsxTransform(const QString &jsx);
    QString jsxTransformFile(const QString &file);

private slots:
    void eval_data();
    void eval();

#if QT_VERSION > 0x050400
    void callFunc_data();
    void callFunc();
    void callFunc1_data();
    void callFunc1();
    void transform_data();
    void transform();
    void load_data();
    void load();
    void react_data();
    void react();
    void reactjsx_data();
    void reactjsx();
    void benchmark();
#endif
};


void JSContext::eval_data()
{
    QTest::addColumn<QString>("expr");
    QTest::addColumn<QString>("output");

    QTest::newRow("01") << "1 + 2" << "3";
    QTest::newRow("02") << "'hello' + ' world!'" << "hello world!";
    QTest::newRow("03") << "(function(a){return a*a;})(5)" << "25";
    QTest::newRow("04") << "(function(s){return s+s;})('orange')" << "orangeorange";
}


void JSContext::eval()
{
    QFETCH(QString, expr);
    QFETCH(QString, output);

    TJSContext js;
    auto result = js.evaluate(expr).toString();
    QCOMPARE(result, output);
}

#if QT_VERSION > 0x050400

void JSContext::callFunc_data()
{
    QTest::addColumn<QString>("func");
    QTest::addColumn<QString>("output");

    QTest::newRow("01") << "function(){return 1+2;}" << "3";
    QTest::newRow("02") << "function(){return 'hello'+' world!';}" << "hello world!";
    QTest::newRow("03") << "function(){return \"hello\"+\" world.\";}" << "hello world.";
}


void JSContext::callFunc()
{
    QFETCH(QString, func);
    QFETCH(QString, output);

    TJSContext js;
    auto result = js.call(func).toString();
    QCOMPARE(result, output);
}

void JSContext::callFunc1_data()
{
    QTest::addColumn<QString>("func");
    QTest::addColumn<QString>("arg");
    QTest::addColumn<QString>("output");

    QTest::newRow("01") << "function(a){return a*a;}" << "4" << "16";
    QTest::newRow("02") << "function(s){return 'Hello '+s;}" << "world" << "Hello world";
}


void JSContext::callFunc1()
{
    QFETCH(QString, func);
    QFETCH(QString, arg);
    QFETCH(QString, output);

    TJSContext js;
    auto result = js.call(func, arg).toString();
    QCOMPARE(result, output);
}


void JSContext::transform_data()
{
    QTest::addColumn<QString>("jsx");
    QTest::addColumn<QString>("output");

    QTest::newRow("01") << "<HelloWorld />" << "React.createElement(HelloWorld, null)";
    QTest::newRow("02") << "<HelloWorld/>" << "React.createElement(HelloWorld, null)";
    QTest::newRow("03") << "<hello/>" << "React.createElement(\"hello\", null)";
}


void JSContext::transform()
{
    QFETCH(QString, jsx);
    QFETCH(QString, output);

    TJSContext js;
    js.load("JSXTransformer");

    auto result = js.call("JSXTransformer.transform", jsx).property("code").toString();
    QCOMPARE(result, output);
}


void JSContext::benchmark()
{
    TJSContext js;
    js.load("JSXTransformer");

    QBENCHMARK {
        auto res = js.call("JSXTransformer.transform", QString("<HelloWorld />"));
        //qDebug() << res.property("code").toString();
    }
}


void JSContext::load_data()
{
    QTest::addColumn<QStringList>("files");
    QTest::addColumn<QString>("variable");
    QTest::addColumn<QString>("result");

    QTest::newRow("01") << QStringList({"./js/main.js"}) << "sub('hello, world')" << "hello hello, world";
}


void JSContext::load()
{
    QFETCH(QStringList, files);
    QFETCH(QString, variable);
    QFETCH(QString, result);

    TJSContext js(true, files);  // commonJs mode
    QString output = js.evaluate(variable).toString();
    QCOMPARE(output, result);
}


void JSContext::react_data()
{
    QTest::addColumn<QString>("variable");
    QTest::addColumn<QString>("result");

    QTest::newRow("01") << "JSXTransformer.transform('<HelloWorld />')['code']"
                        << "React.createElement(HelloWorld, null)";
    QTest::newRow("02") << "ReactDOMServer.renderToString(React.createElement('div'))"
                        << "<div data-reactroot=\"\" data-reactid=\"1\" data-react-checksum=\"2058293082\"></div>";
}


void JSContext::react()
{
    QFETCH(QString, variable);
    QFETCH(QString, result);

    TJSContext js;
    js.load("JSXTransformer");
    js.load("react");
    js.load("react-dom-server");

    QString output = js.evaluate(variable).toString();
    QCOMPARE(output, result);
}

void JSContext::reactjsx_data()
{
    QTest::addColumn<QStringList>("jsxfiles");
    QTest::addColumn<QString>("func");
    QTest::addColumn<QString>("result");

    QTest::newRow("01") << QStringList()
                        << "ReactDOMServer.renderToString(<div/>)"
                        << "<div data-reactroot=\"\" data-reactid=\"1\" data-react-checksum=\"2058293082\"></div>";
    QTest::newRow("02") << QStringList("./js/react_samlple.jsx")
                        << "ReactDOMServer.renderToString(React.createElement(MyComponent, null))"
                        << "<div data-reactroot=\"\" data-reactid=\"1\" data-react-checksum=\"1078334326\">Hello World</div>";
    QTest::newRow("03") << QStringList("./js/react_samlple.jsx")
                        << "ReactDOMServer.renderToString(<MyComponent/>)"
                        << "<div data-reactroot=\"\" data-reactid=\"1\" data-react-checksum=\"1078334326\">Hello World</div>";
    QTest::newRow("04") << QStringList()
                        << "ReactDOMServer.renderToString(<ReactBootstrap.Button/>)"
                        << "<button class=\"btn btn-default\" type=\"button\" data-reactroot=\"\" data-reactid=\"1\" data-react-checksum=\"-1068949636\"></button>";
    QTest::newRow("05") << QStringList({"js/react_bootstrap_sample.jsx"})
                        << "ReactDOMServer.renderToString(<Button/>)"
                        << "<button class=\"btn btn-default\" type=\"button\" data-reactroot=\"\" data-reactid=\"1\" data-react-checksum=\"-1068949636\"></button>";
    QTest::newRow("06") << QStringList({"js/react_bootstrap_sample.jsx"})
                        << "ReactDOMServer.renderToString(SimpleButton)"
                        << "<button class=\"btn btn-default\" type=\"button\" data-reactroot=\"\" data-reactid=\"1\" data-react-checksum=\"1694638448\">Hello</button>";
    QTest::newRow("07") << QStringList({"js/react_bootstrap_sample.jsx"})
                        << "ReactDOMServer.renderToString(HelloButton)"
                        << "<button class=\"btn btn-lg btn-primary\" type=\"button\" data-reactroot=\"\" data-reactid=\"1\" data-react-checksum=\"1572807667\">Hello</button>";
}


void JSContext::reactjsx()
{
    QFETCH(QStringList, jsxfiles);
    QFETCH(QString, func);
    QFETCH(QString, result);

    TJSContext js;
    js.load("react");
    js.load("react-dom-server");
    js.load("react-bootstrap");

    // Loads JSX
    if (!jsxfiles.isEmpty()) {
        for (auto &f : jsxfiles) {
            js.evaluate(jsxTransformFile(f), f);
        }
    }
    QString fn = jsxTransform(func);
    QString output = js.evaluate(fn).toString();
    QCOMPARE(output, result);
}

#endif

QString JSContext::jsxTransform(const QString &jsx)
{
    TJSContext js;
    js.load("JSXTransformer");
    auto val = js.call("JSXTransformer.transform", jsx);
    return val.property("code").toString();
}

QString JSContext::jsxTransformFile(const QString &file)
{
    QFile script(file);
    if (!script.open(QIODevice::ReadOnly)) {
        // open error
        printf("open error: %s\n", qPrintable(file));
        return QString();
    }

    QTextStream stream(&script);
    QString contents = stream.readAll();
    script.close();
    return jsxTransform(contents);
}

TF_TEST_MAIN(JSContext)
#include "main.moc"
