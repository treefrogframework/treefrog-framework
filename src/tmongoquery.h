#ifndef TMONGOQUERY_H
#define TMONGOQUERY_H

#include <QVariant>
#include <QStringList>
#include <TGlobal>
#include <TKvsDatabase>

class TMongoDriver;


class T_CORE_EXPORT TMongoQuery
{
public:
    TMongoQuery(const QString &collection);
    TMongoQuery(const TMongoQuery &other);

    void setLimit(int limit);
    void setOffset(int offset);
    bool find(const QVariantMap &criteria, const QStringList &fields = QStringList());
    bool next();
    QVariantMap value() const;

    QVariantMap findOne(const QVariantMap &criteria, const QStringList &fields = QStringList());
    bool insert(const QVariantMap &document);
    bool update(const QVariantMap &criteria, const QVariantMap &document, bool upsert = false);
    bool updateById(const QVariantMap &document);
    bool updateMulti(const QVariantMap &criteria, const QVariantMap &document, bool upsert = false);
    bool remove(const QVariantMap &criteria);
    bool removeById(const QVariantMap &document);

    TMongoQuery &operator=(const TMongoQuery &other);

private:
    TMongoDriver *driver();
    const TMongoDriver *driver() const;

private:
    TKvsDatabase database;
    QString nameSpace;
    int queryLimit;
    int queryOffset;
};


inline void TMongoQuery::setLimit(int limit)
{
    queryLimit = limit;
}

inline void TMongoQuery::setOffset(int offset)
{
    queryOffset = offset;
}

#endif // TMONGOQUERY_H
