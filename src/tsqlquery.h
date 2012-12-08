#ifndef TSQLQUERY_H
#define TSQLQUERY_H

#include <QtSql>
#include <TGlobal>


class T_CORE_EXPORT TSqlQuery : public QSqlQuery
{
public:
    TSqlQuery(const QString &query = QString(), int databaseId = 0);
    TSqlQuery(int databaseId);

    TSqlQuery &prepare(const QString &query);
    bool load(const QString &filename);
    TSqlQuery &bind(const QString &placeholder, const QVariant &val);
    TSqlQuery &bind(int pos, const QVariant &val);
    TSqlQuery &addBind(const QVariant &val);
    QVariant getNextValue();
    QString queryDirPath() const;
    bool exec(const QString &query);
    bool exec();

    static void clearCachedQueries();
    static QString escapeIdentifier(const QString &identifier, QSqlDriver::IdentifierType type = QSqlDriver::FieldName, int databaseId = 0);
    static QString escapeIdentifier(const QString &identifier, QSqlDriver::IdentifierType type, const QSqlDatabase &database);
    static QString formatValue(const QVariant &val, int databaseId = 0);
    static QString formatValue(const QVariant &val, const QSqlDatabase &database);
};


inline TSqlQuery &TSqlQuery::prepare(const QString &query)
{
    QSqlQuery::prepare(query);
    return *this;
}

inline TSqlQuery &TSqlQuery::bind(const QString &placeholder, const QVariant &val)
{
    QSqlQuery::bindValue(placeholder, val);
    return *this;
}

inline TSqlQuery &TSqlQuery::bind(int pos, const QVariant &val)
{
    QSqlQuery::bindValue(pos, val);
    return *this;
}

inline TSqlQuery &TSqlQuery::addBind(const QVariant &val)
{
    QSqlQuery::addBindValue(val);
    return *this;
}

inline QVariant TSqlQuery::getNextValue()
{
    return (next()) ? record().value(0) : QVariant();
}

#endif // TSQLQUERY_H
