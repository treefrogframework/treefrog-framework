#pragma once
#include <QtSql>
#include <TGlobal>


class T_CORE_EXPORT TSqlQuery : public QSqlQuery {
public:
    TSqlQuery(int databaseId = 0);
    TSqlQuery(const QSqlDatabase &db);

    TSqlQuery &prepare(const QString &query);
    bool load(const QString &filename);
    TSqlQuery &bind(const QString &placeholder, const QVariant &val);
    TSqlQuery &bind(int pos, const QVariant &val);
    TSqlQuery &addBind(const QVariant &val);
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
#if QT_VERSION < 0x060000
    static QString formatValue(const QVariant &val, QVariant::Type type = QVariant::Invalid, int databaseId = 0);
    static QString formatValue(const QVariant &val, QVariant::Type type, const QSqlDatabase &database);
#else
    static QString formatValue(const QVariant &val, const QMetaType &type, int databaseId = 0);
    static QString formatValue(const QVariant &val, const QMetaType &type, const QSqlDatabase &database);
#endif
    static QString formatValue(const QVariant &val, const QSqlDatabase &database);
};


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

