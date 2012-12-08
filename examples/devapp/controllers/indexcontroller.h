#ifndef INDEXCONTROLLER_H
#define INDEXCONTROLLER_H

#include "applicationcontroller.h"


class T_CONTROLLER_EXPORT IndexController : public ApplicationController
{
    Q_OBJECT
public:
    IndexController();
    IndexController(const IndexController &);

public slots:
    void index();
};

T_DECLARE_CONTROLLER(IndexController, indexcontroller)

#endif // INDEXCONTROLLER_H
