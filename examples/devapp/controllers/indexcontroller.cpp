#include <QtCore>
#include <TreeFrogController>
#include <TSqlORMapper>
#include "indexcontroller.h"


IndexController::IndexController()
    : ApplicationController()
{ }


IndexController::IndexController(const IndexController &)
    : ApplicationController()
{ }


void IndexController::index()
{
    T_TRACEFUNC();

    //render("index", "application");
    setLayoutEnabled(false);
    render();
    //renderTemplate("Hello/show");
    //renderText("hohoge foga", true, "Layout_application");
}


// Don't remove below
T_REGISTER_CONTROLLER(IndexController)
