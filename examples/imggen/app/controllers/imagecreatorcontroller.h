#ifndef IMAGECREATORCONTROLLER_H
#define IMAGECREATORCONTROLLER_H

#include "applicationcontroller.h"


class T_CONTROLLER_EXPORT ImageCreatorController : public ApplicationController
{
    Q_OBJECT
public:
    ImageCreatorController() { }
    ImageCreatorController(const ImageCreatorController &other);

public slots:
    void inputtext();
    void inputtextajax();
    void updateImage();
};

T_DECLARE_CONTROLLER(ImageCreatorController, imagecreatorcontroller)

#endif // IMAGECREATORCONTROLLER_H
