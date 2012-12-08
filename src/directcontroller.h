#ifndef DIRECTCONTROLLER_H
#define DIRECTCONTROLLER_H

#include <TActionController>


class DirectController : public TActionController
{
    Q_OBJECT
public:
    DirectController() : TActionController() { }
    DirectController(const DirectController &) : TActionController() { }

public slots:
    void show(const QString &view);
};

T_DECLARE_CONTROLLER(DirectController, directcontroller)

#endif // DIRECTCONTROLLER_H
