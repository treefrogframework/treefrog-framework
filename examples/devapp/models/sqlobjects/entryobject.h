#ifndef ENTRYOBJECT_H
#define ENTRYOBJECT_H

#include <TSqlObject>
#include <QSharedData>


class T_MODEL_EXPORT EntryObject : public TSqlObject, public QSharedData
{
public:
    int id;
    QString name;
    QString address;
    QDateTime created_at;
    QDateTime updated_at;
    int revision;

    enum PropertyIndex {
        Id = 0,
        Name,
        Address,
        CreatedAt,
        UpdatedAt,
        Revision,
    };

    int primaryKeyIndex() const { return Id; }
    int autoValueIndex() const { return -1; }

private:    /*** Don't modify below this line ***/
    Q_OBJECT
    Q_PROPERTY(int id READ getid WRITE setid)
    T_DEFINE_PROPERTY(int, id)
    Q_PROPERTY(QString name READ getname WRITE setname)
    T_DEFINE_PROPERTY(QString, name)
    Q_PROPERTY(QString address READ getaddress WRITE setaddress)
    T_DEFINE_PROPERTY(QString, address)
    Q_PROPERTY(QDateTime created_at READ getcreated_at WRITE setcreated_at)
    T_DEFINE_PROPERTY(QDateTime, created_at)
    Q_PROPERTY(QDateTime updated_at READ getupdated_at WRITE setupdated_at)
    T_DEFINE_PROPERTY(QDateTime, updated_at)
    Q_PROPERTY(int revision READ getrevision WRITE setrevision)
    T_DEFINE_PROPERTY(int, revision)
};

#endif // ENTRYOBJECT_H
