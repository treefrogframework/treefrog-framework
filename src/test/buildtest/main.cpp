#include <TSqlORMapper>
#include <TSqlORMapperIterator>
#include <TSqlQueryORMapper>
#include <TSqlQueryORMapperIterator>
#include <TSqlJoin>
#include <TMongoODMapper>
#include <TModelUtil>
#if QT_VERSION >= 0x050000
# include <TJsonUtil>
#endif
#include <tglobal.h>
#include <tatomicptr.h>
#include <tatomic.h>
#include <tstack.h>
#include <tqueue.h>
#include "blog.h"
#include "blogobject.h"
#include "foo.h"
#include "fooobject.h"

/*!
  Build check of template class
 */

void build_check_TSqlORMapper()
{
    TCriteria crt;
    TSqlORMapper<BlogObject> mapper;

    mapper.setLimit(100);
    mapper.setOffset(1);
    mapper.setSortOrder(1, Tf::AscendingOrder);
    mapper.setSortOrder("id", Tf::AscendingOrder);
    mapper.reset();
    mapper.findFirst(crt);
    mapper.findFirstBy(BlogObject::Body, "hoge");
    mapper.findFirstBy(BlogObject::Body, QString("hoge"));
    mapper.findByPrimaryKey("hoge");
    mapper.findByPrimaryKey(QString("hoge"));
    mapper.findBy(0, "hoge");
    mapper.findBy(0, QString("hoge"));
    mapper.findIn(BlogObject::Id, QVariantList());
    mapper.rowCount();
    mapper.first();
    mapper.last();
    mapper.value(0);
    mapper.findCount(crt);
    mapper.findCountBy(BlogObject::Id, 123);
    mapper.findCountBy(BlogObject::Id, QString("123"));
    mapper.updateAll(crt, 1, 1);
    mapper.updateAll(crt, 1, QString("1"));
    mapper.updateAll(crt, QMap<int, QVariant>());
    mapper.removeAll(crt);
    mapper.removeAll(crt && crt);
    mapper.removeAll(crt || crt);
    mapper.removeAll(!crt);
    auto joinCri = TCriteria(BlogObject::Title, "hoge");
    mapper.setJoin(BlogObject::Id, TSqlJoin<BlogObject>(BlogObject::Title, joinCri));
    mapper.limit(10).offset(11).orderBy(1, Tf::AscendingOrder).join(BlogObject::Id, TSqlJoin<BlogObject>(TSql::LeftJoin, BlogObject::Title, joinCri));
    mapper.limit(10).orderBy("hoge").find();
    mapper.begin();
    mapper.end();
}

void build_check_TSqlORMapperIterator()
{
    TSqlORMapper<BlogObject> mapper;
    mapper.find();

    TSqlORMapperIterator<BlogObject> it(mapper);
    it.next();
    it.previous();
    it.toBack();
    it.toFront();
    it.value();
}

void build_check_TSqlORMapper_ConstIterator()
{
    TSqlORMapper<BlogObject> mapper;
    TSqlORMapper<BlogObject>::ConstIterator it = mapper.begin();
    auto i = it;
    it = i;
    auto val = *it;
    bool b = (i == it);
    b = (i != it);
    if (b) {
        it++;
        ++it;
    } else {
        it--;
        --it;
    }
    for (auto &o : mapper) {
        auto obj = o;
        Q_UNUSED(obj);
    }
}

void build_check_TSqlQueryORMapper()
{
    TSqlQueryORMapper<BlogObject> mapper;

    mapper.prepare("hoge");
    mapper.load("hoge");
    mapper.bind("hoge", 1);
    mapper.bind(1, "hoge");
    mapper.addBind("hoge");
    mapper.exec("hoge");
    mapper.exec();
    mapper.execFirst("hoge");
    mapper.execFirst();
    mapper.numRowsAffected();
    mapper.size();
    mapper.next();
    mapper.value();
    mapper.fieldName(0);
    mapper.begin();
    mapper.end();
}

void build_check_TSqlQueryORMapper_ConstIterator()
{
    TSqlQueryORMapper<BlogObject> mapper;
    TSqlQueryORMapper<BlogObject>::ConstIterator it = mapper.begin();
    auto i = it;
    it = i;
    auto val = *it;
    bool b = (i == it);
    b = (i != it);
    if (b) {
        ++it;
    }
    for (auto &o : mapper) {
        auto obj = o;
        Q_UNUSED(obj);
    }
}

void build_check_TSqlQueryORMapperIterator()
{
    TSqlQueryORMapper<BlogObject> mapper;
    mapper.exec();

    TSqlQueryORMapperIterator<BlogObject> it(mapper);
    it.hasNext();
    it.hasPrevious();
    it.next();
    it.previous();
    it.toBack();
    it.toFront();
    it.value();
}

void build_check_TMongoODMapper()
{
    TCriteria crt;
    TMongoODMapper<FooObject> mapper;

    mapper.setLimit(1);
    mapper.setOffset(100);
    mapper.setSortOrder(FooObject::Title, Tf::DescendingOrder);
    mapper.findOne(crt);
    mapper.findFirst(crt);
    mapper.findFirstBy(FooObject::Title, "hoge");
    mapper.findFirstBy(FooObject::Title, QString("hoge"));
    mapper.findFirstBy(FooObject::Title, 1);
    mapper.findByObjectId("hoge");
    mapper.find(crt);
    mapper.findBy(FooObject::Id, "hoge");
    mapper.findBy(FooObject::Id, QString("hoge"));
    mapper.findBy(FooObject::Id, 1);
    mapper.findIn(FooObject::Id, QVariantList());
    mapper.next();
    mapper.value();
    mapper.findCount(crt);
    mapper.findCountBy(FooObject::Id, "hoge");
    mapper.findCountBy(FooObject::Id, QString("hoge"));
    mapper.findCountBy(FooObject::Id, 1);
    mapper.updateAll(crt, FooObject::Id, "hoge");
    mapper.updateAll(crt, FooObject::Id, QString("hoge"));
    mapper.updateAll(crt, FooObject::Id, 1);
    mapper.updateAll(crt, QMap<int, QVariant>());
    mapper.removeAll(crt);
    mapper.limit(10).offset(1).orderBy("hoge").find();
    mapper.orderBy(1, Tf::DescendingOrder).findOne();
}

void build_check_TModelUtil()
{
    TCriteria crt;
    QList<QPair<QString, Tf::SortOrder>> sortColumns;
    QList<QPair<int, Tf::SortOrder>> sortColumns2;
    tfGetModelListByCriteria<Blog, BlogObject>(crt, sortColumns, 0, 0);
    tfGetModelListByCriteria<Blog, BlogObject>(crt, sortColumns2, 0, 0);
    tfGetModelListByCriteria<Blog, BlogObject>(crt, "hoge", Tf::DescendingOrder, 0, 0);
    tfGetModelListByCriteria<Blog, BlogObject>(crt, 0, Tf::DescendingOrder, 0, 0);
    tfGetModelListByCriteria<Blog, BlogObject>(crt, 0, 0);
    tfGetModelListByMongoCriteria<Foo, FooObject>(crt, 0, 0);

    QList<Blog> list;
    tfConvertToJsonArray(list);
#if QT_VERSION >= 0x050c00  // 5.12.0
    tfConvertToCborArray(list);
#endif
}

#if QT_VERSION >= 0x050000
void build_check_TJsonUtil()
{
    QList<Foo> fooList;
    tfModelListToJsonArray<Foo>(fooList);
}
#endif

void atomic_ptr()
{
    Foo *foo = new Foo();
    TAtomicPtr<Foo> ptr(foo);
    TAtomicPtr<Foo> ptr2(ptr);
    foo = ptr2;
    ptr2.load();
    ptr.store(foo);
    ptr.compareExchange(foo, nullptr);
    auto ptr3 = ptr.exchange(foo);
    ptr = ptr2;
    ptr2 = ptr3;
    Tf::threadFence();
}

void atomic_int()
{
    TAtomic<int> counter;
    TAtomic<int> counter2 {0};
    counter.fetchAdd(2);
    counter.fetchSub(2);
    counter++;
    ++counter;
    counter--;
    --counter;
    int tmp = counter.load();
    counter.store(3);
    tmp = counter.exchange(3);
    counter = 0;
    counter.compareExchange(tmp, 0);
    counter.compareExchangeStrong(tmp, 0);
    tmp = counter++ + --counter2;
}

void stack()
{
    TStack<QString> stack;
    stack.push(QString());
    QString s;
    stack.pop(s);
    stack.top(s);
}

void queue()
{
    TQueue<QString> queue;
    queue.enqueue(QString());
    QString s;
    queue.dequeue(s);
    queue.head(s);
    queue.count();
}

int main()
{
    return 0;
}
