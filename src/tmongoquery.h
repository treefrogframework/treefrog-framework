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
    virtual ~TMongoQuery() { }

    int limit() const;
    void setLimit(int limit);
    int offset() const;
    void setOffset(int offset);
    bool find(const QVariantMap &criteria = QVariantMap(), const QVariantMap &orderBy = QVariantMap(), const QStringList &fields = QStringList());
    bool next();
    QVariantMap value() const;

    QVariantMap findOne(const QVariantMap &criteria = QVariantMap(), const QStringList &fields = QStringList());
    QVariantMap findById(const QString &id, const QStringList &fields = QStringList());
    bool insert(QVariantMap &document);
    bool update(const QVariantMap &criteria, const QVariantMap &document, bool upsert = false);
    bool updateById(const QVariantMap &document);
    bool updateMulti(const QVariantMap &criteria, const QVariantMap &document);
    bool remove(const QVariantMap &criteria = QVariantMap());
    bool removeById(const QVariantMap &document);
    int count(const QVariantMap &criteria = QVariantMap());
    int numDocsAffected() const;
    QString lastErrorString() const;

    TMongoQuery &operator=(const TMongoQuery &other);

private:
    TMongoDriver *driver();
    const TMongoDriver *driver() const;

private:
    TKvsDatabase database;
    QString collection;
    int queryLimit;
    int queryOffset;
};


inline int TMongoQuery::limit() const
{
    return queryLimit;
}


inline void TMongoQuery::setLimit(int limit)
{
    queryLimit = limit;
}


inline int TMongoQuery::offset() const
{
    return queryOffset;
}


inline void TMongoQuery::setOffset(int offset)
{
    queryOffset = offset;
}

#endif // TMONGOQUERY_H
