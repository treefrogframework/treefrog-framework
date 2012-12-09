#include <QtCore>
#include <TreeFrogController>
#include <TSqlORMapper>
#include "hellocontroller.h"
//#include "entry2.h"
#include "hellohelper.h"
#include "entryname.h"

#if 0
# include <QImage>
# include <QPainter>
#endif


HelloController::HelloController()
    : ApplicationController()
{ }


HelloController::HelloController(const HelloController &)
    : ApplicationController()
{ }


static void recursive()
{
    recursive();
}


void HelloController::index()
{
    T_TRACEFUNC();

//     Entry2 ent2 = Entry2::load(0);
//     tDebug("findFirst  Entry id: %d  name: %s", ent2.id(), qPrintable(ent2.name()));
  
    // try it !!!!
    //Entry2::proc();
    //Entry2::hoge();

    QString hoge = "aoyama !!!!22233 -<-..>.. & ..\"..";
    T_EXPORT(hoge);

    QList<QVariant> entries;
    entries << (QStringList() << tr("Aoyama Kazuharu") << tr("七浦５６３"));
    entries << (QStringList() << tr("Aoyama 友恵") << tr("板橋区中台"));
    entries << (QStringList() << tr("青山 敏大") << tr("大田区下丸子 ブラウトリエ"));
    T_EXPORT(entries);

    HelloHelper helper;
    helper.setName(tr("青山 和玄"));
    helper.setAddress(tr("板橋区中台3丁目"));
    T_EXPORT(helper);

    QVariantHash values = httpRequest().allParameters();
    T_EXPORT(values);

    EntryName ent = EntryName::create(1, "aoyama", "urushiyama", 22);
    tDebug("isNull %d", ent.isNull());
    T_EXPORT(ent);

    //render("index", "application");
    render();
    //renderTemplate("Hello/show");
    //renderText("hohoge foga", true, "Layout_application");
    tDebug("##################### req: %s", httpRequest().cookie("hoge3").data());

    QByteArray ba = THttpUtility::toUrlEncoding("aoyama._-+*~");
    tDebug("############################  %s", ba.data());
    tDebug("############################  %s", qPrintable(THttpUtility::fromUrlEncoding(ba)));

    //httpResponse().setCookie("hoge3", tr("aoyamakkkkkkk!!! -._~;ハイパーテキスト").toUtf8(), QDateTime::currentDateTime().addSecs(3600));

    // session test
    foreach (QString key, static_cast<QVariantHash *>(&session())->keys()) {
        tDebug("****************  %s  %s", qPrintable(key), session().value(key).toByteArray().data());
    }

    session().insert("hogehoge", "tomoe!! thanks");

    //recursive();

//     char *pp = new char[10];
//     for (int i = 0; i < 20; ++i)
//         *pp++ = '\0';

//      int *p = 0;
//      int i = *p;


//     try {
     int h = 0;
     int f = 1 / h;
//     } catch (...) {
//         tDebug("==================");
//     }

//     QTime t;
//     t.start();
//     for (int i = 0; i < 1000; ++i)
//         tDebug("%d ############################ lsajdl;fss990980787kjhuujlljs", i);
    
//     tSystemDebug("Total: %d msec", t.elapsed());


//     QTime t;

//     QSystemSemaphore sema("hoge", 1);
//     t.restart();
//     for (int i = 0; i < 10000; ++i) {
//         sema.acquire();
//         sema.release();
//     }
//     tSystemDebug("Semaphore  Total: %d msec", t.elapsed());

//     QMutex mutex;
//     t.restart();
//     for (int i = 0; i < 10000; ++i) {
//         mutex.lock();
//         mutex.unlock();
//     }
//     tSystemDebug("Mutex  Total: %d msec", t.elapsed());
}


void HelloController::show()
{
    T_TRACEFUNC();
    render();
}


void HelloController::inputtext()
{
    T_TRACEFUNC();

    QString string = httpRequest().parameter("text");
#if 0
    //QImage image(QSize(500, 100), QImage::Format_ARGB32_Premultiplied);
    QImage image(QSize(500, 100), QImage::Format_RGB32);
    image.fill(0xcccccc);
    QPainter painter(&image);
    painter.setFont(QFont("IPAPGothic", 20));
    //painter.setFont(QFont("IPAPMincho", 20));
    painter.drawText(10, 40, string);
    painter.end();
    image.save(tWebApp->publicPath() + "text.png");
#endif
    T_EXPORT(string);
    
    setLayoutEnabled(false);
    render();
}


void HelloController::inputtextajax()
{
    inputtext();
}


void HelloController::updateImage()
{
    inputtext();
}


void HelloController::search()
{
    T_TRACEFUNC();
//    sleep(10);
}


void HelloController::upload()
{
    T_TRACEFUNC();
    
    httpRequest().multipartFormData().renameUploadedFile("FiletoUpload", Tf::app()->webRootPath() + "tmp" + QDir::separator() + "hogehoge.tmp", true);
}


// Don't remove below
T_REGISTER_CONTROLLER(hellocontroller)
