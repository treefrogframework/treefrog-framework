#include <TfTest/TfTest>
#include <QDebug>
#include "../../turlroute.h"


class TestUrlRouter : public QObject, public TUrlRoute
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();

    void should_route_get_correctly();
    void should_route_post_correctly();
    void should_route_put_correctly();
    void should_route_delete_correctly();
    void should_route_patch_correctly();
    void should_not_route_get();
    void should_not_route_post();
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
    void should_not_accept_a_route_if_request_has_surplus_parameters();

    void should_not_create_route_if_destination_empty_and_route_does_not_accept_controller_and_action();
    void should_not_create_route_if_bad_param();
    // void should_not_create_route_if_it_does_not_accept_action_parameter_and_no_default_is_given();
    // void should_not_create_route_if_it_accepts_controller_but_not_action_and_no_default_given();
    // void should_create_route_if_it_accepts_controller_but_not_action_but_default_given();
    // void should_create_route_if_only_accepts_action();
    // void should_create_route_if_destination_does_not_include_action_but_it_accepts_as_parameter();
    // void should_create_route_if_destination_is_empty_but_controller_and_action_parameters_given();

    // void should_route_correctly_when_controller_and_action_given_as_parameters();
    // void should_default_to_destination_if_controller_and_action_parameters_are_empty();
    // void should_route_correctly_when_only_action_parameter_is_given();
    // void should_route_correctly_when_controller_parameter_empty_but_action_is_given();
    // void should_not_route_if_controller_given_but_action_is_not();
    // void should_route_correctly_when_controller_parameter_given_and_does_not_accept_action_parameter();
    // void should_parse_params_correctly_even_if_preceding_parameter_is_empty();
};

void TestUrlRouter::init()
{
    clear();
}

void TestUrlRouter::cleanup()
{ }


void TestUrlRouter::should_route_get_correctly()
{
    addRouteFromString("GET / 'dummy.hoge'");

    TRouting r = findRouting(Tf::Get, TUrlRoute::splitPath("/"));

    QCOMPARE(r.exists, true);
    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("hoge"));
    QCOMPARE(r.params, QStringList());
}

void TestUrlRouter::should_route_post_correctly()
{
    addRouteFromString("POST / 'dummy.foo'");

    TRouting r = findRouting(Tf::Post, TUrlRoute::splitPath("/"));

    QCOMPARE(r.exists, true);
    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("foo"));
    QCOMPARE(r.params, QStringList());
}

void TestUrlRouter::should_route_put_correctly()
{
    addRouteFromString("PUT / 'dummy.fuga'");

    TRouting r = findRouting(Tf::Put, TUrlRoute::splitPath("/"));

    QCOMPARE(r.exists, true);
    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("fuga"));
    QCOMPARE(r.params, QStringList());
}

void TestUrlRouter::should_route_patch_correctly()
{
    addRouteFromString("PATCH / 'dummy.hoge'");

    TRouting r = findRouting(Tf::Patch, TUrlRoute::splitPath("/"));

    QCOMPARE(r.exists, true);
    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("hoge"));
    QCOMPARE(r.params, QStringList());
}

void TestUrlRouter::should_not_route_get()
{
    addRouteFromString("POST / 'dummy.hoge'");

    TRouting r = findRouting(Tf::Get, TUrlRoute::splitPath("/"));

    QCOMPARE(r.exists, false);
}

void TestUrlRouter::should_not_route_post()
{
    addRouteFromString("GET / 'dummy.hoge'");

    TRouting r = findRouting(Tf::Post, TUrlRoute::splitPath("/"));

    QCOMPARE(r.exists, false);
}

void TestUrlRouter::should_route_delete_correctly()
{
    addRouteFromString("DELETE / 'dummy.foo'");

    TRouting r = findRouting(Tf::Delete, TUrlRoute::splitPath("/"));

    QCOMPARE(r.exists, true);
    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("foo"));
    QCOMPARE(r.params, QStringList());
}

void TestUrlRouter::should_route_to_empty_if_no_route_present()
{
    TRouting r = findRouting(Tf::Get, TUrlRoute::splitPath("/"));

    QCOMPARE(r.exists, false);
}

void TestUrlRouter::should_route_to_empty_if_no_method_matches()
{
    addRouteFromString("POST   / 'dummy.index'");
    addRouteFromString("PUT    / 'dummy.index'");
    addRouteFromString("PATCH  / 'dummy.index'");
    addRouteFromString("DELETE / 'dummy.index'");

    TRouting r = findRouting(Tf::Get, TUrlRoute::splitPath("/"));

    QCOMPARE(r.exists, false);
}

void TestUrlRouter::should_route_urls_with_parameters()
{
    addRouteFromString("GET  /:params 'dummy.index'");

    TRouting r = findRouting(Tf::Get, TUrlRoute::splitPath("/p1/p2/p3/"));

    QCOMPARE(r.exists, true);
    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList() << "p1" << "p2" << "p3");
}

void TestUrlRouter::should_route_urls_with_empty_parameters()
{
    addRouteFromString("GET  /:params 'dummy.index'");
    TRouting r = findRouting(Tf::Get, TUrlRoute::splitPath("/p1//p3/"));

    QCOMPARE(r.exists, true);
    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList() << "p1" << "" << "p3");
}

void TestUrlRouter::should_route_urls_with_no_parameters()
{
    addRouteFromString("GET  /:params 'dummy.index'");
    TRouting r = findRouting(Tf::Get, TUrlRoute::splitPath("/"));

    QCOMPARE(r.exists, true);
    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList());
}

void TestUrlRouter::should_route_urls_with_single_parameters_correctly()
{
    addRouteFromString("GET  /foo/:param/bar 'dummy.index'");
    TRouting r = findRouting(Tf::Get, TUrlRoute::splitPath("/foo/p1/bar/"));

    QCOMPARE(r.exists, true);
    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList() << "p1");
}

void TestUrlRouter::should_not_route_urls_with_multiple_parameters_instead_of_single()
{
    addRouteFromString("GET  /foo/:param/bar 'dummy.index'");
    TRouting r = findRouting(Tf::Get, TUrlRoute::splitPath("/foo/p1/p2/bar/"));

    QCOMPARE(r.exists, false);
}

void TestUrlRouter::should_route_urls_with_empty_single_parameters_correctly()
{
    addRouteFromString("GET  /foo/:param/bar 'dummy.index'");
    TRouting r = findRouting(Tf::Get, TUrlRoute::splitPath("/foo//bar/"));

    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList() << "");
}

void TestUrlRouter::should_route_urls_with_multiple_single_parameters_correctly()
{
    addRouteFromString("GET  /foo/:param/bar/:param/baz/:param 'dummy.index'");
    TRouting r = findRouting(Tf::Get, TUrlRoute::splitPath("/foo/p1/bar/p2/baz/p3/"));

    QCOMPARE(r.exists, true);
    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList() << "p1" << "p2" << "p3");
}

void TestUrlRouter::should_not_route_urls_with_missing_single_parameters()
{
    addRouteFromString("GET  /foo/:param/bar/ 'dummy.index'");
    TRouting r = findRouting(Tf::Get, TUrlRoute::splitPath("/foo/bar/"));

    QCOMPARE(r.exists, false);
}

void TestUrlRouter::should_not_route_urls_if_static_part_is_not_matching()
{
    addRouteFromString("GET  /foo/:param/bar/ 'dummy.index'");
    TRouting r = findRouting(Tf::Get, TUrlRoute::splitPath("/foo/p1/baz/"));

    QCOMPARE(r.exists, false);
}

void TestUrlRouter::should_route_urls_with_both_single_and_multiple_parameters_correctly_when_multiple_params_is_empty()
{
    addRouteFromString("GET  /foo/:param/baz/:params 'dummy.index'");
    TRouting r = findRouting(Tf::Get, TUrlRoute::splitPath("/foo/p1/baz/"));

    QCOMPARE(r.exists, true);
    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList() << "p1");
}

void TestUrlRouter::should_route_urls_with_both_single_and_multiple_parameters_correctly_when_multiple_params_is_not_empty()
{
    addRouteFromString("GET  /foo/:param/baz/:params 'dummy.index'");
    TRouting r = findRouting(Tf::Get, TUrlRoute::splitPath("/foo/p1/baz/p2/"));

    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList() << "p1" << "p2");
}

void TestUrlRouter::should_route_urls_with_both_single_and_multiple_parameters_correctly_when_multiple_params_has_multiple_items()
{
    addRouteFromString("GET  /foo/:param/baz/:params 'dummy.index'");
    TRouting r = findRouting(Tf::Get, TUrlRoute::splitPath("/foo/p1/baz/p2/p3"));

    QCOMPARE(r.exists, true);
    QCOMPARE(QString(r.controller), QString("dummycontroller"));
    QCOMPARE(QString(r.action), QString("index"));
    QCOMPARE(r.params, QStringList() << "p1" << "p2" << "p3");
}

void TestUrlRouter::should_not_accept_routes_with_params_in_middle()
{
    bool result = addRouteFromString("GET /foo/:params/bar 'dummy.index'");
    QCOMPARE(result, false);
}

void TestUrlRouter::should_not_accept_a_route_if_request_has_surplus_parameters()
{
    addRouteFromString("GET  / 'dummy.index'");
    TRouting r = findRouting(Tf::Get, TUrlRoute::splitPath("/foo/p1/baz/p2/p3/"));

    QCOMPARE(r.exists, false);
}

void TestUrlRouter::should_not_create_route_if_destination_empty_and_route_does_not_accept_controller_and_action()
{
    QString route = "GET /";
    bool result = addRouteFromString(route);

    QCOMPARE(result, false);
}

void TestUrlRouter::should_not_create_route_if_bad_param()
{
    QString route = "GET /foo/:cont 'dummy.index'";
    bool result = addRouteFromString(route);

    QCOMPARE(result, false);
}

// void TestUrlRouter::should_create_route_if_destination_is_empty_but_controller_and_action_parameters_given()
// {
//     QString route = "GET /:controller/:action";
//     bool result = addRouteFromString(route);

//     QCOMPARE(result, true);
// }

// void TestUrlRouter::should_not_create_route_if_it_does_not_accept_action_parameter_and_no_default_is_given()
// {
//     QString route = "GET /:controller 'dummy' ";
//     bool result = addRouteFromString(route);

//     QCOMPARE(result, false);
// }

// void TestUrlRouter::should_not_create_route_if_it_accepts_controller_but_not_action_and_no_default_given()
// {
//     QString route = "GET /:controller 'dummy' ";
//     bool result = addRouteFromString(route);

//     QCOMPARE(result, false);
// }

// void TestUrlRouter::should_create_route_if_it_accepts_controller_but_not_action_but_default_given()
// {
//     QString route = "GET /:controller '#defaultaction' ";
//     bool result = addRouteFromString(route);

//     QCOMPARE(result, true);
// }

// void TestUrlRouter::should_create_route_if_only_accepts_action()
// {
//     QString route = "GET /:action 'dummy.default' ";
//     bool result = addRouteFromString(route);

//     QCOMPARE(result, true);
// }

// void TestUrlRouter::should_create_route_if_destination_does_not_include_action_but_it_accepts_as_parameter()
// {
//     QString route = "GET /:action 'default' ";
//     bool result = addRouteFromString(route);

//     QCOMPARE(result, true);
// }

// void TestUrlRouter::should_route_correctly_when_controller_and_action_given_as_parameters()
// {
//     QString route = "GET /:controller/:action 'default#defaultaction' ";
//     addRouteFromString(route);

//     TRouting r = findRouting(Tf::Get, "/good/goodaction/");

//     QCOMPARE(QString(r.controller), QString("good"));
//     QCOMPARE(QString(r.action), QString("goodaction"));
// }

// void TestUrlRouter::should_default_to_destination_if_controller_and_action_parameters_are_empty()
// {
//     QString route = "GET /:controller/:action 'default#defaultaction' ";
//     addRouteFromString(route);

//     TRouting r = findRouting(Tf::Get, "///");

//     QCOMPARE(QString(r.controller), QString("default"));
//     QCOMPARE(QString(r.action), QString("defaultaction"));
// }

// void TestUrlRouter::should_route_correctly_when_only_action_parameter_is_given()
// {
//     QString route = "GET /:action 'default#defaultaction' ";
//     addRouteFromString(route);

//     TRouting r = findRouting(Tf::Get, "/newaction/");

//     QCOMPARE(QString(r.controller), QString("default"));
//     QCOMPARE(QString(r.action), QString("newaction"));
// }

// void TestUrlRouter::should_route_correctly_when_controller_parameter_empty_but_action_is_given()
// {
//     QString route = "GET /:controller/:action 'default#defaultaction' ";
//     addRouteFromString(route);

//     TRouting r = findRouting(Tf::Get, "//otheraction/");

//     QCOMPARE(QString(r.controller), QString("default"));
//     QCOMPARE(QString(r.action), QString("otheraction"));
// }

// void TestUrlRouter::should_not_route_if_controller_given_but_action_is_not()
// {
//     QString route = "GET /:controller/:action 'default#defaultaction' ";
//     addRouteFromString(route);

//     TRouting r = findRouting(Tf::Get, "/othercontroller/");

//     QCOMPARE(r.exists, false);
// }

// void TestUrlRouter::should_route_correctly_when_controller_parameter_given_and_does_not_accept_action_parameter()
// {
//     QString route = "GET /:controller '#defaultaction' ";
//     addRouteFromString(route);

//     TRouting r = findRouting(Tf::Get, "/other/");

//     QCOMPARE(QString(r.controller), QString("other"));
//     QCOMPARE(QString(r.action), QString("defaultaction"));
// }

// void TestUrlRouter::should_parse_params_correctly_even_if_preceding_parameter_is_empty()
// {
//     QString route = "GET /foo/:action/:params 'default#defaultaction' ";
//     addRouteFromString(route);

//     TRouting r = findRouting(Tf::Get, "/foo//p1/p2/p3/");

//     printf("p=%s\n",qUtf8Printable(r.params.join(",")));

//     QCOMPARE(QString(r.controller), QString("default"));
//     QCOMPARE(QString(r.action), QString("defaultaction"));
//     QCOMPARE(r.params, QStringList() << "p1" << "p2" << "p3");
// }


TF_TEST_MAIN(TestUrlRouter)
#include "urlrouter.moc"
