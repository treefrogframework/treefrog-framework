#ifndef APPLICATIONCONTROLLER_H
#define APPLICATIONCONTROLLER_H

#include <TActionController>
#include "applicationhelper.h"


class T_CONTROLLER_EXPORT ApplicationController : public TActionController
{
public:
    ApplicationController() : TActionController() { }
    virtual ~ApplicationController() { }
};

#endif // APPLICATIONCONTROLLER_H
