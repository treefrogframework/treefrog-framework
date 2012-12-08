#ifndef ENTRYNAMEOBJECT_H
#define ENTRYNAMEOBJECT_H

#include <TSqlObject>
#include <QSharedData>


class T_MODEL_EXPORT EntryNameObject : public TSqlObject, public QSharedData
{
public:
    int id_index;
    QString full_name;
    QString address;
    QDateTime created_at;
    int entry_number;

    enum PropertyIndex {
        IdIndex = 0,
        FullName,
        Address,
        CreatedAt,
        EntryNumber,
    };

    int primaryKeyIndex() const { return -1; }

    /*** Don't modify below this line ***/
    Q_OBJECT
    Q_PROPERTY(int id_index READ getid_index WRITE setid_index)
    T_DEFINE_PROPERTY(int, id_index)
    Q_PROPERTY(QString full_name READ getfull_name WRITE setfull_name)
    T_DEFINE_PROPERTY(QString, full_name)
    Q_PROPERTY(QString address READ getaddress WRITE setaddress)
    T_DEFINE_PROPERTY(QString, address)
    Q_PROPERTY(QDateTime created_at READ getcreated_at WRITE setcreated_at)
    T_DEFINE_PROPERTY(QDateTime, created_at)
    Q_PROPERTY(int entry_number READ getentry_number WRITE setentry_number)
    T_DEFINE_PROPERTY(int, entry_number)
};

#endif // ENTRYNAMEOBJECT_H
