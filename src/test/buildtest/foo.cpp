#include <TreeFrogModel>
#include "foo.h"
#include "fooobject.h"

Foo::Foo()
    : TAbstractModel(), d(new FooObject)
{
    d->length = 0;
    d->lockRevision = 0;
}

Foo::Foo(const Foo &other)
    : TAbstractModel(), d(new FooObject(*other.d))
{ }

Foo::Foo(const FooObject &object)
    : TAbstractModel(), d(new FooObject(object))
{ }

Foo::~Foo()
{
    // If the reference count becomes 0,
    // the shared data object 'FooObject' is deleted.
}

QString Foo::id() const
{
    return d->_id;
}

QString Foo::title() const
{
    return d->title;
}

void Foo::setTitle(const QString &title)
{
    d->title = title;
}

QString Foo::body() const
{
    return d->body;
}

void Foo::setBody(const QString &body)
{
    d->body = body;
}

int Foo::length() const
{
    return d->length;
}

void Foo::setLength(int length)
{
    d->length = length;
}

QDateTime Foo::createdAt() const
{
    return d->createdAt;
}

QDateTime Foo::updatedAt() const
{
    return d->updatedAt;
}

int Foo::lockRevision() const
{
    return d->lockRevision;
}

Foo &Foo::operator=(const Foo &other)
{
    d = other.d;  // increments the reference count of the data
    return *this;
}

Foo Foo::create(const QString &title, const QString &body, int length)
{
    FooObject obj;
    obj.title = title;
    obj.body = body;
    obj.length = length;
    if (!obj.create()) {
        return Foo();
    }
    return Foo(obj);
}

Foo Foo::create(const QVariantMap &values)
{
    Foo model;
    model.setProperties(values);
    if (!model.d->create()) {
        model.d->clear();
    }
    return model;
}

Foo Foo::get(const QString &id)
{
    TMongoODMapper<FooObject> mapper;
    return Foo(mapper.findByObjectId(id));
}

Foo Foo::get(const QString &id, int lockRevision)
{
    TMongoODMapper<FooObject> mapper;
    TCriteria cri;
    cri.add(FooObject::Id, id);
    cri.add(FooObject::LockRevision, lockRevision);
    return Foo(mapper.findFirst(cri));
}

int Foo::count()
{
    TMongoODMapper<FooObject> mapper;
    return mapper.findCount();
}

QList<Foo> Foo::getAll()
{
    return tfGetModelListByMongoCriteria<Foo, FooObject>(TCriteria());
}

#if QT_VERSION >= 0x050000
QJsonArray Foo::getAllJson()
{
    QJsonArray array;
    TMongoODMapper<FooObject> mapper;

    if (mapper.find() > 0) {
        while (mapper.next()) {
            array.append(QJsonValue(QJsonObject::fromVariantMap(Foo(mapper.value()).toVariantMap())));
        }
    }
    return array;
}
#endif

TModelObject *Foo::modelData()
{
    return d.data();
}

const TModelObject *Foo::modelData() const
{
    return d.data();
}
