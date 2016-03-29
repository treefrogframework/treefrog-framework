#include <TfTest/TfTest>
#include <QJSEngine>
#include <QJSValue>
#include "../../tjscontext.h"
#include "../../treacthelper.h"


class BenchMark : public QObject
{
    Q_OBJECT
private slots:
    void benchCall();
    //void benchEval();
};


void BenchMark::benchCall()
{
    TJSContext js;
    js.load("JSXTransformer.js");

    QBENCHMARK {
        auto res = js.call("JSXTransformer.transform", QString("<HelloWorld />"));
        //qDebug() << res.property("code").toString();
    }
}


// void BenchMark::benchEval()
// {
//     TJSContext js;
//     js.load("JSXTransformer.js");
//     js.import("JSXTransformer");

//     QJSValueList args = { "<HelloWorld />" };
//     QString argstr;
//     for (int i = 0; i < args.count(); i++) {
//         argstr = QChar('a') + QString::number(i) + ",";
//     }
//     argstr.chop(1);

//     QString function = QLatin1String("function(") + argstr + "){return(JSXTransformer.transform(" + argstr + "));}";
//     QJSValue jsfunc = js.jsEngine->evaluate(function);

//     QBENCHMARK {
//         auto res = jsfunc.call(args);
//         //qDebug() << res.property("code").toString();
//     }
// }


TF_TEST_MAIN(BenchMark)
#include "main.moc"
