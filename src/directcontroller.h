#pragma once
#include <TActionController>


class DirectController : public TActionController {
    Q_OBJECT
public:
    DirectController() :
        TActionController() { }

public slots:
    void show(const QString &view);
};

