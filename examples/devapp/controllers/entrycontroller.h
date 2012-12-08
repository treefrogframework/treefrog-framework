#ifndef ENTRYCONTROLLER_H
#define ENTRYCONTROLLER_H

#include "applicationcontroller.h"


class T_CONTROLLER_EXPORT EntryController : public ApplicationController
{
    Q_OBJECT
public:
    EntryController() { }
    EntryController(const EntryController &other);

public slots:
    void index();
    void show(const QString &pk);
    void entry();
    void create();
    void edit(const QString &pk);
    void save(const QString &pk);
    void remove(const QString &pk);
};

T_DECLARE_CONTROLLER(EntryController, entrycontroller)

#endif // ENTRYCONTROLLER_H
