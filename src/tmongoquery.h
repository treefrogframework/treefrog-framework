#pragma once
#include <QStringList>
#include <QVariant>
#include <TGlobal>
#include <TKvsDatabase>

class TMongoDriver;


class T_CORE_EXPORT TMongoQuery {
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
    int update(const QVariantMap &criteria, const QVariantMap &document, bool upsert = false);
    bool updateById(const QVariantMap &document);
    int updateMulti(const QVariantMap &criteria, const QVariantMap &document);
    int remove(const QVariantMap &criteria);
    bool removeById(const QVariantMap &document);
    int count(const QVariantMap &criteria = QVariantMap());
    QString lastErrorString() const;

    TMongoQuery &operator=(const TMongoQuery &other);

private:
    TMongoDriver *driver();
    const TMongoDriver *driver() const;

private:
    TMongoQuery(Tf::KvsEngine engine, const QString &collection);

    TKvsDatabase _database;
    QString _collection;
    int _queryLimit {0};
    int _queryOffset {0};

    friend class TCacheMongoStore;
};


inline int TMongoQuery::limit() const
{
    return _queryLimit;
}


inline void TMongoQuery::setLimit(int limit)
{
    _queryLimit = limit;
}


inline int TMongoQuery::offset() const
{
    return _queryOffset;
}


inline void TMongoQuery::setOffset(int offset)
{
    _queryOffset = offset;
}

