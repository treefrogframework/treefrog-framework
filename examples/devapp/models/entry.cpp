#include <TreeFrogModel>
#include "entry.h"
#include "entryobject.h"

Entry::Entry()
    : TAbstractModel(), d(new EntryObject)
{
    d->id = 0;
    d->revision = 0;
}

Entry::Entry(const Entry &other)
    : TAbstractModel(), d(new EntryObject(*other.d))
{ }

Entry::Entry(const EntryObject &object)
    : TAbstractModel(), d(new EntryObject(object))
{ }

Entry::~Entry()
{
    // If the reference count becomes 0,
    // the shared data object 'EntryObject' is deleted.
}

int Entry::id() const
{
    return d->id;
}

void Entry::setId(int id)
{
    d->id = id;
}

QString Entry::name() const
{
    return d->name;
}

void Entry::setName(const QString &name)
{
    d->name = name;
}

QString Entry::address() const
{
    return d->address;
}

void Entry::setAddress(const QString &address)
{
    d->address = address;
}

QDateTime Entry::createdAt() const
{
    return d->created_at;
}

QDateTime Entry::updatedAt() const
{
    return d->updated_at;
}

int Entry::revision() const
{
    return d->revision;
}

void Entry::setRevision(int revision)
{
    d->revision = revision;
}

Entry &Entry::operator=(const Entry &other)
{
    d = other.d;  // increments the reference count of the data
    return *this;
}

Entry Entry::create(int id, const QString &name, const QString &address, int revision)
{
    EntryObject obj;
    obj.id = id;
    obj.name = name;
    obj.address = address;
    obj.revision = revision;
    if (!obj.create()) {
        obj.clear();
    }
    return Entry(obj);
}

Entry Entry::create(const QVariantHash &values)
{
    Entry model;
    model.setProperties(values);
    if (!model.d->create()) {
        model.d->clear();
    }
    return model;
}

Entry Entry::get(int id)
{
    TSqlORMapper<EntryObject> mapper;
    return Entry(mapper.findByPrimaryKey(id));
}

QList<Entry> Entry::getAll()
{
    return tfGetModelListByCriteria<Entry, EntryObject>(TCriteria());
}

TSqlObject *Entry::data()
{
    return d.data();
}

const TSqlObject *Entry::data() const
{
    return d.data();
}
