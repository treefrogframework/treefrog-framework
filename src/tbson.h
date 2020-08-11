#pragma once
#include <QVariant>
#include <TGlobal>

using TBsonObject = void;


class T_CORE_EXPORT TBson {
public:
    TBson();
    ~TBson();
    TBson(const TBson &other);
    TBson(const TBsonObject *bson);

    bool insert(const QString &key, const QVariant &value);
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    TBsonObject *data() { return bsonData; }
    const TBsonObject *constData() const { return bsonData; }

    static QVariantMap fromBson(const TBson &bson);
    static TBson toBson(const QVariantMap &map);
    static TBson toBson(const QString &op, const QVariantMap &map);
    static TBson toBson(const QVariantMap &query, const QVariantMap &orderBy);
    static TBson toBson(const QStringList &lst);
    static QString generateObjectId();

protected:
    static QVariantMap fromBson(const TBsonObject *obj);

private:
    typedef struct _bson_t bson_t;
    bson_t *bsonData {nullptr};  // pointer to object of bson_t

    friend class TMongoDriver;
    friend class TMongoCursor;
    TBson &operator=(const TBson &other);
};

