#include <TreeFrogModel>
#include "entryname.h"
#include "entrynameobject.h"

EntryName::EntryName()
    : TAbstractModel(), d(new EntryNameObject)
{ }

EntryName::EntryName(const EntryName &other)
    : TAbstractModel(), d(new EntryNameObject(*other.d))
{ }

EntryName::EntryName(const EntryNameObject &object)
    : TAbstractModel(), d(new EntryNameObject(object))
{ }

EntryName::~EntryName()
{
    // If the reference count becomes 0,
    // the shared data object 'EntryNameObject' is deleted.
}

int EntryName::idIndex() const
{
    return d->id_index;
}

void EntryName::setIdIndex(int idIndex)
{
    d->id_index = idIndex;
}

QString EntryName::fullName() const
{
    return d->full_name;
}

void EntryName::setFullName(const QString &fullName)
{
    d->full_name = fullName;
}

QString EntryName::address() const
{
    return d->address;
}

void EntryName::setAddress(const QString &address)
{
    d->address = address;
}

QDateTime EntryName::createdAt() const
{
    return d->created_at;
}

int EntryName::entryNumber() const
{
    return d->entry_number;
}

void EntryName::setEntryNumber(int entryNumber)
{
    d->entry_number = entryNumber;
}

EntryName EntryName::create(int idIndex, const QString &fullName, const QString &address, int entryNumber)
{
    EntryNameObject obj;
    obj.id_index = idIndex;
    obj.full_name = fullName;
    obj.address = address;
    obj.entry_number = entryNumber;
    if (!obj.create())
        obj.clear();
    return EntryName(obj);
}

EntryName EntryName::get(int idIndex, const QString &fullName, const QString &address, int entryNumber)
{
    TCriteria cri;
    cri.add(EntryNameObject::IdIndex, idIndex);
    cri.add(EntryNameObject::FullName, fullName);
    cri.add(EntryNameObject::Address, address);
    cri.add(EntryNameObject::EntryNumber, entryNumber);
    TSqlORMapper<EntryNameObject> mapper;
    return EntryName(mapper.findFirst(cri));
}

TSqlObject *EntryName::data()
{
    return d.data();
}

const TSqlObject *EntryName::data() const
{
    return d.data();
}
