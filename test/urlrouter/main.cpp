#include <QTest>
#include "../../src/turlroute.h"
#include <QDebug>


class TestUrlRouter : public QObject
{
    Q_OBJECT
private:
    TUrlRoute *ur;
private slots:
    void init();
    void cleanup();

    void should_route_get_correctly();
    void should_route_post_correctly();
    void should_route_put_correctly();
    void should_route_delete_correctly();
    void should_route_patch_correctly();
    void should_route_to_empty_if_no_route_present();
    void should_route_to_empty_if_no_method_matches();
    void should_route_urls_with_parameters();
    void should_route_urls_with_empty_parameters();
    void should_route_urls_with_no_parameters();
};

void TestUrlRouter::init()
{
    ur = new TUrlRoute();
}

void TestUrlRouter::cleanup()
{
    delete ur;
    ur = NULL;
}


void TestUrlRouter::should_route_get_correctly()
{
    ur->addRouteFromString("GET / 'dummy#get'");

    TRouting r = ur->findRouting(Tf::Get, "/");

    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("get"));
    QCOMPARE(r.params, QStringList());
}

void TestUrlRouter::should_route_post_correctly()
{
    ur->addRouteFromString("POST / 'dummy#post'");

    TRouting r = ur->findRouting(Tf::Post, "/");

    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("post"));
    QCOMPARE(r.params, QStringList());
}
void TestUrlRouter::should_route_put_correctly()
{
    ur->addRouteFromString("PUT / 'dummy#put'");

    TRouting r = ur->findRouting(Tf::Put, "/");

    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("put"));
    QCOMPARE(r.params, QStringList());
}
void TestUrlRouter::should_route_patch_correctly()
{
    ur->addRouteFromString("PATCH / 'dummy#patch'");

    TRouting r = ur->findRouting(Tf::Patch, "/");

    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("patch"));
    QCOMPARE(r.params, QStringList());
}
void TestUrlRouter::should_route_delete_correctly()
{
    ur->addRouteFromString("DELETE / 'dummy#del'");

    TRouting r = ur->findRouting(Tf::Delete, "/");

    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("del"));
    QCOMPARE(r.params, QStringList());
}

void TestUrlRouter::should_route_to_empty_if_no_route_present()
{
    TRouting r = ur->findRouting(Tf::Get, "/");

    QCOMPARE(r.isEmpty(), true);
}

void TestUrlRouter::should_route_to_empty_if_no_method_matches()
{
    ur->addRouteFromString("POST  / 'dummy#index'");
    ur->addRouteFromString("PUT / 'dummy#index'");
    ur->addRouteFromString("PATCH / 'dummy#index'");
    ur->addRouteFromString("DELETE / 'dummy#index'");

    TRouting r = ur->findRouting(Tf::Get, "/");

    QCOMPARE(r.isEmpty(), true);
}

void TestUrlRouter::should_route_urls_with_parameters()
{
    ur->addRouteFromString("GET  /:params 'dummy#index'");
    TRouting r = ur->findRouting(Tf::Get, "/p1/p2/p3");

    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList() << "p1" << "p2" << "p3");
}

void TestUrlRouter::should_route_urls_with_empty_parameters()
{
    ur->addRouteFromString("GET  /:params 'dummy#index'");
    TRouting r = ur->findRouting(Tf::Get, "/p1//p3");

    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList() << "p1" << "" << "p3");
}

void TestUrlRouter::should_route_urls_with_no_parameters()
{
    ur->addRouteFromString("GET  /:params 'dummy#index'");
    TRouting r = ur->findRouting(Tf::Get, "/");

    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList());
}

QTEST_MAIN(TestUrlRouter)
#include "main.moc"
