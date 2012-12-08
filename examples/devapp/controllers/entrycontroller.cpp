#include "entrycontroller.h"
#include "entry.h"


EntryController::EntryController(const EntryController &)
    : ApplicationController()
{ }

void EntryController::index()
{
    QList<Entry> entryList = Entry::getAll();
    texport(entryList);
    render();
}

void EntryController::show(const QString &pk)
{
    Entry entry = Entry::get(pk.toInt());
    texport(entry);
    render();
}

void EntryController::entry()
{
    render();
}

void EntryController::create()
{
    if (httpRequest().method() != Tf::Post) {
        return;
    }
    
    Entry entry = Entry::create( httpRequest().formItems("entry") );
    if (!entry.isNull()) {
        QString notice = "Created successfully.";
        tflash(notice);
        redirect(urla("show", entry.id()));
    } else {
        QString error = "Failed to create.";
        texport(error);
        render("entry");
    }
}

void EntryController::edit(const QString &pk)
{
    Entry entry = Entry::get(pk.toInt());
    if (!entry.isNull()) {
        texport(entry);
        render();
    } else {
        redirect(urla("entry"));
    }
}

void EntryController::save(const QString &pk)
{
    if (httpRequest().method() != Tf::Post) {
        return;
    }

    QString error;
    Entry entry = Entry::get(pk.toInt());
    if (entry.isNull()) {
        error = "Original data not found. It may have been updated/removed by another transaction.";
        tflash(error);
        redirect(urla("edit", pk));
        return;
    } 
    
    entry.setProperties( httpRequest().formItems("entry") );
    if (entry.save()) {
        QString notice = "Updated successfully.";
        tflash(notice);
    } else {
        error = "Failed to update.";
        tflash(error);
    }
    redirect(urla("show", pk));
}

void EntryController::remove(const QString &pk)
{
    if (httpRequest().method() != Tf::Post) {
        return;
    }
    
    Entry entry = Entry::get(pk.toInt());
    entry.remove();
    redirect(urla("index"));
}


// Don't remove below this line
T_REGISTER_CONTROLLER(entrycontroller)
