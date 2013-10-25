#ifndef TBSON_H
#define TBSON_H

#include <QVariant>
#include <TGlobal>

typedef void TBsonObject;


class T_CORE_EXPORT TBson
{
public:
    TBson();
    ~TBson();
    TBsonObject *data() { return bsonData; }
    const TBsonObject *constData() const { return bsonData; }

    static QVariantMap fromBson(const TBson &bson);
    static TBson toBson(const QVariantMap &map);
    static TBson toBson(const QVariantMap &query, const QVariantMap &orderBy);
    static TBson toBson(const QStringList &lst);
    static QString generateObjectId();

protected:
    static QVariantMap fromBson(const TBsonObject *obj);

private:
    TBsonObject *bsonData;   // pointer to object of struct bson

    friend class TMongoDriver;
    friend class TMongoCursor;
    //Q_DISABLE_COPY(TBson) /* You copy TBson objects several times in the code, then why forbid the copy?? Produces compile errors for me. */
};

#endif // TBSON_H
