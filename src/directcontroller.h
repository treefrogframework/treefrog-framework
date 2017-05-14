#ifndef DIRECTCONTROLLER_H
#define DIRECTCONTROLLER_H

#include <TActionController>


class DirectController : public TActionController
{
    Q_OBJECT
public:
    Q_INVOKABLE
    DirectController() : TActionController() { }

public slots:
    void show(const QString &view);
};

#endif // DIRECTCONTROLLER_H
