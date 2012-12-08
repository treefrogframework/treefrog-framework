#include "imagecreatorcontroller.h"
#include <TWebApplication>
#include <QtCore>
#include <QImage>
#include <QPainter>


ImageCreatorController::ImageCreatorController(const ImageCreatorController &)
    : ApplicationController()
{ }

void ImageCreatorController::inputtext()
{
    QString string = httpRequest().parameter("text");
    //QImage image(QSize(500, 100), QImage::Format_ARGB32_Premultiplied);
    QImage image(QSize(500, 100), QImage::Format_RGB32);
    image.fill(0xcccccc);
    QPainter painter(&image);
    painter.setFont(QFont("IPAPGothic", 20));
    //painter.setFont(QFont("IPAPMincho", 20));
    painter.drawText(10, 40, string);
    painter.end();
    image.save(Tf::app()->publicPath() + "text.png");
    T_EXPORT(string);
    
    setLayoutEnabled(false);
    render();
}


void ImageCreatorController::inputtextajax()
{
    inputtext();
}


void ImageCreatorController::updateImage()
{
    inputtext();
}


// Don't remove below this line
T_REGISTER_CONTROLLER(imagecreatorcontroller)
