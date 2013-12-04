#ifndef FOOOBJECT_H
#define FOOOBJECT_H

#include <TMongoObject>
#include <QSharedData>


class T_MODEL_EXPORT FooObject : public TMongoObject, public QSharedData
{
public:
    QString _id;
    QString title;
    QString body;
    int length;
    QDateTime createdAt;
    QDateTime updatedAt;
    int lockRevision;

    enum PropertyIndex {
        Id = 0,
        Title,
        Body,
        Length,
        CreatedAt,
        UpdatedAt,
        LockRevision,
    };

    virtual QString collectionName() const { return "foo"; }
    virtual QString objectId() const { return _id; }
    virtual QString &objectId() { return _id; }

private:
    Q_OBJECT
    Q_PROPERTY(QString _id READ get_id WRITE set_id)
    T_DEFINE_PROPERTY(QString, _id)
    Q_PROPERTY(QString title READ gettitle WRITE settitle)
    T_DEFINE_PROPERTY(QString, title)
    Q_PROPERTY(QString body READ getbody WRITE setbody)
    T_DEFINE_PROPERTY(QString, body)
    Q_PROPERTY(int length READ getlength WRITE setlength)
    T_DEFINE_PROPERTY(int, length)
    Q_PROPERTY(QDateTime createdAt READ getcreatedAt WRITE setcreatedAt)
    T_DEFINE_PROPERTY(QDateTime, createdAt)
    Q_PROPERTY(QDateTime updatedAt READ getupdatedAt WRITE setupdatedAt)
    T_DEFINE_PROPERTY(QDateTime, updatedAt)
    Q_PROPERTY(int lockRevision READ getlockRevision WRITE setlockRevision)
    T_DEFINE_PROPERTY(int, lockRevision)
};
 
#endif // FOOOBJECT_H