#include <TGlobal>
#include <TreeFrogModel>
#include <TCriteria>
#include "entry2.h"
#include "entryobject.h"


Entry2::Entry2()
{
    sqlObject = new EntryObject();
}


Entry2::Entry2(const EntryObject &entry)
{
    sqlObject = new EntryObject(entry);
}


Entry2::~Entry2()
{
    delete sqlObject;
}


int Entry2::id()
{
    loadObject();
    return sqlObject->id;
}

void Entry2::setId(int id)
{
    loadObject();
    sqlObject->id = id;
}

QString Entry2::name()
{
    loadObject();
    return sqlObject->name;
}

void Entry2::setName(const QString &name)
{
    loadObject();
    sqlObject->name = name;
}

QString Entry2::address()
{
    loadObject();
    return sqlObject->address;
}

void Entry2::setAddress(const QString &address)
{
    loadObject();
    sqlObject->address = address;
}


Entry2 Entry2::load(int id)
{
    Entry2 e;
    e.setLazyLoadKey(id);
    return e;
}


Entry2 Entry2::get(int )
{
    TSqlORMapper<EntryObject> mapper;
    Entry2 entry(mapper.findFirst());
    return entry;
}


void printEntry(const EntryObject& e)
{
    tDebug("EntryObject id:%d name:%s address:%s", e.id, qPrintable(e.name), qPrintable(e.address));
}


void Entry2::hoge()
{
    TSqlORMapper<EntryObject> mapper0;
    TCriteria cri0(EntryObject::Id, TSql::IsNotNull);
    int res0 = mapper0.removeAll(cri0);
    tDebug("### Removed records: %d", res0);   
}


void Entry2::proc()
{
    TSqlORMapper<EntryObject> mapper0;
    TCriteria cri0(EntryObject::Id, TSql::IsNotNull);
    int res0 = mapper0.removeAll(cri0);
    tDebug("** Removed records: %d", res0);

    EntryObject e1;
    e1.id = 1000;
    e1.name = "aoyama kazuharu";
    e1.address = "oota-ku";
    Q_ASSERT(e1.isNew());
    bool res1 = e1.create();
    Q_ASSERT(!e1.isNew());
    Q_ASSERT(res1);

    TSqlORMapper<EntryObject> mapper1;
    mapper1.find();
    Q_ASSERT(mapper1.rowCount() == 1);
    EntryObject e2 = mapper1.value(0);
    Q_ASSERT(e2.id == e1.id && e2.name == e1.name && e2.address == e1.address);
    printEntry(e2);
    tDebug("*1 test - save & findAll ... success");

    TCriteria cri2(EntryObject::Id, TSql::Equal, e1.id);
    mapper1.find(cri2);
    Q_ASSERT(mapper1.rowCount() == 1);
    TCriteria cri2a(EntryObject::Id, TSql::NotEqual, e1.id);
    mapper1.find(cri2a);
    Q_ASSERT(mapper1.rowCount() == 0);
    mapper1.find(cri2 || cri2a);
    Q_ASSERT(mapper1.rowCount() == 1);
    cri2.add(cri2a);
    mapper1.find(cri2);
    Q_ASSERT(mapper1.rowCount() == 0);
    tDebug("*2 test - findAll ... success");

    EntryObject e3;
    e3.id = 1001;
    e3.name = "aoyama tomoe";
    e3.address = "itabashi-ku";
    bool res3 = e3.update();
    Q_ASSERT(!res3);
    res3 = e3.create();
    Q_ASSERT(res3);
    TSqlORMapper<EntryObject> mapper3;
    EntryObject e3a = mapper3.findFirst();
    Q_ASSERT(!e3a.isNew());
    printEntry(e3a);
    Q_ASSERT(!e3a.isEmpty());
    TCriteria cri3(EntryObject::Id, TSql::NotEqual, e3.id);
    EntryObject e3b = mapper3.findFirst(cri3);
    Q_ASSERT(!e3b.isEmpty());
    printEntry(e3b);
    Q_ASSERT(e3.id != e3b.id);
    TCriteria cri3c(EntryObject::Id, TSql::Equal, e3.id);
    EntryObject e3c = mapper3.findFirst(cri3c);
    Q_ASSERT(!e3c.isEmpty());
    Q_ASSERT(e3.name == e3c.name);
    Q_ASSERT(e3.address == e3c.address);
    TCriteria cri3d(EntryObject::Id, TSql::Equal, 1913236700);
    EntryObject e3d = mapper3.findFirst(cri3d);
    Q_ASSERT(e3d.isEmpty());
    tDebug("*3 test - findFirst ... success");

    TSqlORMapper<EntryObject> mapper4;
    EntryObject e4 = mapper4.findByPrimaryKey(e3.id);
    printEntry(e4);
    Q_ASSERT(!e4.isEmpty());
    EntryObject e4a = mapper4.findByPrimaryKey(e1.id);
    printEntry(e4a);
    Q_ASSERT(!e4a.isEmpty());
    EntryObject e4b = mapper4.findByPrimaryKey(1913236700);
    Q_ASSERT(e4b.isEmpty());
    tDebug("*4 test - findByPrimaryKey ... success");
  
    for (int i = 2000; i < 2300; ++i) {
        EntryObject e5;
        e5.id = i;
        e5.name = QString("Name-x") + QString::number(i);
        e5.address = QString("address") + QString::number(i*i);
        e5.create();
    }
    TSqlORMapper<EntryObject> mapper5;
    mapper5.setSort(EntryObject::Id, TSql::AscendingOrder);
    mapper5.setLimit(50);
    mapper5.setOffset(3);
    int res5 = mapper5.find();
    Q_ASSERT(res5 == 50);
    TSqlORMapperIterator<EntryObject> it5(mapper5);
    while (it5.hasNext()) {
        EntryObject e5a = it5.next();
        printEntry(e5a);
        if (e5a.id == 2050) {
            e5a.name = "--------";
            e5a.address = "********";
            bool res5a = e5a.update();
            Q_ASSERT(res5a);
            tDebug("update !!");
        }
    }
    
    TSqlORMapper<EntryObject> mapper5b;
    EntryObject e5b = mapper5b.findFirst(TCriteria(EntryObject::Id, 2050));
    Q_ASSERT(!e5b.isEmpty());
    Q_ASSERT(e5b.name == "--------");
    Q_ASSERT(e5b.address == "********");
    printEntry(e5b);
    tDebug("*5b test - find ... success");

    TSqlORMapper<EntryObject> mapper5c;
    EntryObject e5c = mapper5c.findFirst(TCriteria(EntryObject::Id, 2031));
    Q_ASSERT(!e5c.isEmpty());
    Q_ASSERT(!e5c.isNew());
    Q_ASSERT(!e5c.isModified());
    e5c.name = "hogehoge 5c";
    Q_ASSERT(e5c.isModified());
    bool res5c = e5c.update();
    Q_ASSERT(res5c);
    tDebug("*5c test - update ... success");

    TSqlORMapper<EntryObject> mapper5d;
    EntryObject e5d = mapper5d.findFirst(TCriteria(EntryObject::Id, 2031));
    Q_ASSERT(!e5d.isEmpty());
    Q_ASSERT(e5d.name == "hogehoge 5c");
    printEntry(e5d);
    tDebug("*5d test - find ... success");

    TSqlORMapper<EntryObject> mapper6;
    TCriteria  cri6;
    cri6.add(EntryObject::Address, TSql::Like, "%00%");
    cri6.addOr(TCriteria(EntryObject::Id, TSql::Between, 2100, 2233));
    int res6 = mapper6.find(cri6);
    Q_ASSERT(res6 >= 134);
    tDebug("*6 test - find ... success");

//     TSqlORMapper<EntryObject> mapper7;
//     TCriteria  cri7(EntryObject::Id, 2050);
//     EntryObject e7;
//     e7.id = 2050;
//     e7.name ="ssssssssssssssssssssssssssssssssssssssssssssssssssssssos";
//     e7.address = "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^";
//     int res7 = mapper7.updateWhere(e7, cri7);
//     Q_ASSERT(res7 == 1);
//     EntryObject e7a = mapper7.findFirst(cri7);
//     Q_ASSERT(e7.name == e7a.name);
//     Q_ASSERT(e7.address == e7a.address);
//     tDebug("*7 test - updateWhere ... success");

    TSqlORMapper<EntryObject> mapper8;
    EntryObject e8 = mapper8.findFirst(TCriteria(EntryObject::Id, 2051));
    e8.name ="**";
    e8.address = "!!";
    Q_ASSERT(e8.isModified());
    bool res8 = e8.update();
    Q_ASSERT(!e8.isModified());
    Q_ASSERT(res8);
    EntryObject e8a = mapper8.findByPrimaryKey(e8.id);
    Q_ASSERT(e8.name == e8a.name);
    Q_ASSERT(e8.address == e8a.address);
    tDebug("*8 test - updateByPrimaryKey ... success");

    TSqlORMapper<EntryObject> mapper9;
    TCriteria  cri9(EntryObject::Id, 2050);
    int res9 = mapper9.removeAll(cri9);
    Q_ASSERT(res9 == 1);
    EntryObject e9 = mapper9.findFirst(cri9);
    Q_ASSERT(e9.isEmpty());
    tDebug("*9 test - removeAll ... success");

    TSqlORMapper<EntryObject> mapper10;
    EntryObject e10 = mapper10.findByPrimaryKey(2222);
    Q_ASSERT(!e10.isEmpty());
    int res10 = e10.remove();
    Q_ASSERT(res10 == 1);
    EntryObject e10a = mapper10.findFirst(TCriteria(EntryObject::Id, e10.id));
    Q_ASSERT(e10a.isEmpty());
    tDebug("*10 test - removeAll ... success");

    TSqlORMapper<EntryObject> mapper11;
    EntryObject e11 = mapper11.findByPrimaryKey(2255);
    Q_ASSERT(!e11.isEmpty());
    int res11 = e11.remove();
    Q_ASSERT(res11 == 1);
    EntryObject e11a = mapper11.findByPrimaryKey(e11.id);
    Q_ASSERT(e11a.isEmpty());
    tDebug("*11 test - removeAll ... success");

    TSqlQueryORMapper<EntryObject> query12;
    query12.prepare("select * from entry WHERE address LIKE ?").addBind("%01%");
    bool res12 = query12.exec();
    Q_ASSERT(res12);
    while (query12.next()) {
        EntryObject e12 = query12.value();
        Q_ASSERT(!e12.isEmpty());
        printEntry(e12);
    }
    query12.clear();
    query12.prepare("select * from entry WHERE address = 'ade8si41__._94304'");
    res12 = query12.exec();
    Q_ASSERT(res12);
    tDebug("*12 test - TSqlQueryORMapper prepare ... success");

    TSqlORMapper<EntryObject> mapper13;
    int cnt13 = mapper13.find();
    Q_ASSERT(cnt13 > 0);
    TSqlQuery query13;
    query13.prepare("DELETE from entry WHERE address LIKE ?").addBind("%00%");
    bool res13 = query13.exec();
    Q_ASSERT(res13);
    mapper13.reset();
    int cnt13a = mapper13.find();
    Q_ASSERT(cnt13a > 0 && cnt13a < cnt13);
    tDebug("*13 test - TSqlORMapper prepare #2 ... success");

    TSqlQuery query14;
    query14.prepare("SELECT COUNT(1) FROM entry");
    bool res14 = query14.exec();
    Q_ASSERT(res14);
    int cnt14 = query14.getNextValue().toInt();
    tDebug("SELECT COUNT: %d", cnt14);
    Q_ASSERT(cnt14 > 10);
    tDebug("*14 test - TSqlQuery getNextValue ... success");
  
    TSqlQueryORMapper<EntryObject> query15;
    bool res15 = query15.load("select_from_entry.sql");
    Q_ASSERT(res15);
    query15.addBind(2015);
//   bool res15 = query15.exec();
//   Q_ASSERT(res15);
//   QString id15 = query15.getNextValue().toString();
//   tDebug("id: %s", qPrintable(id15));
    EntryObject e15 = query15.findFirst();
    Q_ASSERT(!e15.isEmpty());
    printEntry(e15);
    tDebug("*15 test - TSqlQuery load ... success");
}
