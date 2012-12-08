#ifndef HELLOCONTROLLER_H
#define HELLOCONTROLLER_H

#include "applicationcontroller.h"


class T_CONTROLLER_EXPORT HelloController : public ApplicationController
{
    Q_OBJECT
public:
    HelloController();
    HelloController(const HelloController &);

public slots:
    void index();
    void show();
    void inputtext();
    void inputtextajax();
    void updateImage();
    void search();
    void upload();
};

T_DECLARE_CONTROLLER(HelloController, hellocontroller)

#endif // HELLOCONTROLLER_H
