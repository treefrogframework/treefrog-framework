#pragma once
#include <QtSql>
#include <TGlobal>


class T_CORE_EXPORT TSqlQuery : public QSqlQuery {
public:
    TSqlQuery(int databaseId = 0);
    TSqlQuery(const QSqlDatabase &db);

    TSqlQuery &prepare(const QString &query);
    bool load(const QString &filename);
    bool loadPreparedQuery(const QString &filename) { return load(filename); }
    TSqlQuery &bind(const QString &placeholder, const QVariant &val);
    TSqlQuery &bind(int pos, const QVariant &val);
    TSqlQuery &addBind(const QVariant &val);
    QVariant boundValue(int pos) const;
    QVariantList boundValues() const;
    QVariant getNextValue();
    QString queryDirPath() const;
    bool exec(const QString &query);
    bool exec();
    int numRowsAffected() const;
    int size() const;
    bool next();
    QVariant value(int index) const;
    QVariant value(const QString &name) const;

    static void clearCachedQueries();
    static QString escapeIdentifier(const QString &identifier, QSqlDriver::IdentifierType type = QSqlDriver::FieldName, int databaseId = 0);
    static QString escapeIdentifier(const QString &identifier, QSqlDriver::IdentifierType type, const QSqlDriver *driver);
    static QString formatValue(const QVariant &val, const QMetaType &type, int databaseId = 0);
    static QString formatValue(const QVariant &val, const QMetaType &type, const QSqlDatabase &database);
    static QString formatValue(const QVariant &val, const QMetaType &type, const QSqlDriver *driver);
    static QString formatValue(const QVariant &val, const QSqlDriver *driver);
    static QString formatValue(const QVariant &val, const QSqlDatabase &database) { return formatValue(val, database.driver()); }

private:
    QString _connectionName;
    QVariantList _boundValues;  // For prepared query
};


inline QVariant TSqlQuery::getNextValue()
{
    return (next()) ? record().value(0) : QVariant();
}


inline int TSqlQuery::numRowsAffected() const
{
    return QSqlQuery::numRowsAffected();
}


inline int TSqlQuery::size() const
{
    return QSqlQuery::size();
}


inline bool TSqlQuery::next()
{
    return QSqlQuery::next();
}


inline QVariant TSqlQuery::value(int index) const
{
    return QSqlQuery::value(index);
}


inline QVariant TSqlQuery::value(const QString &name) const
{
    return QSqlQuery::value(name);
}

