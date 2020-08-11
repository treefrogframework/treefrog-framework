#pragma once
#include <TMongoObject>


class T_MODEL_EXPORT TSessionMongoObject : public TMongoObject {
public:
    QString _id;
    QString sessionId;
    QByteArray data;
    QDateTime updatedAt;

    enum PropertyIndex {
        Id = 0,
        SessionId,
        Data,
        UpdatedAt,
    };

    virtual QString collectionName() const override { return QLatin1String("session"); }
    virtual QString objectId() const override { return _id; }
    virtual QString &objectId() override { return _id; }

private:
    Q_OBJECT
    Q_PROPERTY(QString _id READ get_id WRITE set_id)
    T_DEFINE_PROPERTY(QString, _id)
    Q_PROPERTY(QString sessionId READ getsessionId WRITE setsessionId)
    T_DEFINE_PROPERTY(QString, sessionId)
    Q_PROPERTY(QByteArray data READ getdata WRITE setdata)
    T_DEFINE_PROPERTY(QByteArray, data)
    Q_PROPERTY(QDateTime updatedAt READ getupdatedAt WRITE setupdatedAt)
    T_DEFINE_PROPERTY(QDateTime, updatedAt)
};

