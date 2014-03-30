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
    void should_route_urls_with_single_parameters_correctly();
    void should_not_route_urls_with_multiple_parameters_instead_of_single();
    void should_route_urls_with_empty_single_parameters_correctly();
    void should_route_urls_with_multiple_single_parameters_correctly();
    void should_not_route_urls_with_missing_single_parameters();
    void should_not_route_urls_if_static_part_is_not_matching();
    void should_route_urls_with_both_single_and_multiple_parameters_correctly_when_multiple_params_is_empty();
    void should_route_urls_with_both_single_and_multiple_parameters_correctly_when_multiple_params_is_not_empty();
    void should_route_urls_with_both_single_and_multiple_parameters_correctly_when_multiple_params_has_multiple_items();
    void should_not_accept_routes_with_params_in_middle();
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
    TRouting r = ur->findRouting(Tf::Get, "/p1/p2/p3/");

    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList() << "p1" << "p2" << "p3");
}

void TestUrlRouter::should_route_urls_with_empty_parameters()
{
    ur->addRouteFromString("GET  /:params 'dummy#index'");
    TRouting r = ur->findRouting(Tf::Get, "/p1//p3/");

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

void TestUrlRouter::should_route_urls_with_single_parameters_correctly()
{
    ur->addRouteFromString("GET  /foo/:param/bar 'dummy#index'");
    TRouting r = ur->findRouting(Tf::Get, "/foo/p1/bar/");

    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList() << "p1");
}

void TestUrlRouter::should_not_route_urls_with_multiple_parameters_instead_of_single()
{
    ur->addRouteFromString("GET  /foo/:param/bar 'dummy#index'");
    TRouting r = ur->findRouting(Tf::Get, "/foo/p1/p2/bar/");

    QCOMPARE(r.isEmpty(), true);
}

void TestUrlRouter::should_route_urls_with_empty_single_parameters_correctly()
{
    ur->addRouteFromString("GET  /foo/:param/bar 'dummy#index'");
    TRouting r = ur->findRouting(Tf::Get, "/foo//bar/");

    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList() << "");
}

void TestUrlRouter::should_route_urls_with_multiple_single_parameters_correctly()
{
    ur->addRouteFromString("GET  /foo/:param/bar/:param/baz/:param 'dummy#index'");
    TRouting r = ur->findRouting(Tf::Get, "/foo/p1/bar/p2/baz/p3/");

    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList() << "p1" << "p2" << "p3");
}

void TestUrlRouter::should_not_route_urls_with_missing_single_parameters()
{
    ur->addRouteFromString("GET  /foo/:param/bar/ 'dummy#index'");
    TRouting r = ur->findRouting(Tf::Get, "/foo/bar/");

    QCOMPARE(r.isEmpty(), true);
}

void TestUrlRouter::should_not_route_urls_if_static_part_is_not_matching()
{
    ur->addRouteFromString("GET  /foo/:param/bar/ 'dummy#index'");
    TRouting r = ur->findRouting(Tf::Get, "/foo/p1/baz/");

    QCOMPARE(r.isEmpty(), true);
}

void TestUrlRouter::should_route_urls_with_both_single_and_multiple_parameters_correctly_when_multiple_params_is_empty()
{
    ur->addRouteFromString("GET  /foo/:param/baz/:params 'dummy#index'");
    TRouting r = ur->findRouting(Tf::Get, "/foo/p1/baz/");

    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList() << "p1");
}

void TestUrlRouter::should_route_urls_with_both_single_and_multiple_parameters_correctly_when_multiple_params_is_not_empty()
{
    ur->addRouteFromString("GET  /foo/:param/baz/:params 'dummy#index'");
    TRouting r = ur->findRouting(Tf::Get, "/foo/p1/baz/p2/");

    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList() << "p1" << "p2");
}

void TestUrlRouter::should_route_urls_with_both_single_and_multiple_parameters_correctly_when_multiple_params_has_multiple_items()
{
    ur->addRouteFromString("GET  /foo/:param/baz/:params/ 'dummy#index'");
    TRouting r = ur->findRouting(Tf::Get, "/foo/p1/baz/p2/p3/");

    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList() << "p1" << "p2" << "p3");
}

void TestUrlRouter::should_not_accept_routes_with_params_in_middle()
{
    QCOMPARE(ur->addRouteFromString("GET /foo/:params/bar 'dummy#index'"), false);
}


QTEST_MAIN(TestUrlRouter)
#include "urlrouter.moc"
