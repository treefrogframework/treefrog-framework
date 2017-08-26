#ifndef TSESSIONOBJECT_H
#define TSESSIONOBJECT_H

#include <TSqlObject>


class TSessionObject : public TSqlObject
{
public:
    QString id;
    QByteArray data;
    QDateTime updated_at;

    enum PropertyIndex {
        Id = 0,
        Data,
        UpdatedAt,
    };

    int primaryKeyIndex() const override { return Id; }
    int autoValueIndex() const override { return -1; }
    QString tableName() const override { return QLatin1String("session"); }

private:    /*** Don't modify below this line ***/
    Q_OBJECT
    Q_PROPERTY(QString id READ getid WRITE setid)
    T_DEFINE_PROPERTY(QString, id)
    Q_PROPERTY(QByteArray data READ getdata WRITE setdata)
    T_DEFINE_PROPERTY(QByteArray, data)
    Q_PROPERTY(QDateTime updated_at READ getupdated_at WRITE setupdated_at)
    T_DEFINE_PROPERTY(QDateTime, updated_at)
};

#endif // TSESSIONOBJECT_H
