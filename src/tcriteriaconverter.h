#ifndef TCRITERIACONVERTER_H
#define TCRITERIACONVERTER_H

#include <QMetaObject>
#include <QVariant>
#include <QHash>
#include <TCriteria>
#include <TSqlQuery>
#include <TGlobal>
#include "tsystemglobal.h"

namespace TSql
{
    T_CORE_EXPORT const QHash<int, QString> &formats();
}

/*!
  TCriteriaData class is a class for criteria data objects.
  \sa TCriteria
 */
class T_CORE_EXPORT TCriteriaData
{
public:
    TCriteriaData();
    TCriteriaData(const TCriteriaData &other);
    TCriteriaData(int property, int op);
    TCriteriaData(int property, int op, const QVariant &val);
    TCriteriaData(int property, int op, const QVariant &val1, const QVariant &val2);
    TCriteriaData(int property, int op1, int op2, const QVariant &val);
    bool isEmpty() const;

    int property;
    int op1;
    int op2;
    QVariant val1;
    QVariant val2;
};


inline TCriteriaData::TCriteriaData()
    : property(-1), op1(TSql::Invalid), op2(TSql::Invalid)
{ }


inline TCriteriaData::TCriteriaData(const TCriteriaData &other)
    :  property(other.property), op1(other.op1), op2(other.op2), val1(other.val1), val2(other.val2)
{ }


inline TCriteriaData::TCriteriaData(int property, int op)
    : property(property), op1(op), op2(TSql::Invalid)
{ }


inline TCriteriaData::TCriteriaData(int property, int op, const QVariant &val)
    : property(property), op1(op), op2(TSql::Invalid), val1(val)
{ }


inline TCriteriaData::TCriteriaData(int property, int op, const QVariant &val1, const QVariant &val2)
    : property(property), op1(op), op2(TSql::Invalid), val1(val1), val2(val2)
{ }


inline TCriteriaData::TCriteriaData(int property, int op1, int op2, const QVariant &val)
    : property(property), op1(op1), op2(op2), val1(val)
{ }


inline bool TCriteriaData::isEmpty() const
{
    return (property < 0 || op1 == TSql::Invalid);
}

Q_DECLARE_METATYPE(TCriteriaData)


template <class T>
class TCriteriaConverter
{
public:
    TCriteriaConverter(const TCriteria &cri, const QSqlDatabase &db, const QString &aliasTableName = QString()) : criteria(cri), database(db), tableAlias(aliasTableName) { }
    QString toString() const;
    static QString propertyName(int property, const QSqlDriver *driver, const QString &aliasTableName = QString());


protected:
    QString criteriaToString(const QVariant &cri) const;
    static QString criteriaToString(const QString &propertyName, TSql::ComparisonOperator op, const QVariant &val1, const QVariant &val2, const QSqlDatabase &database);
    static QString criteriaToString(const QString &propertyName, TSql::ComparisonOperator op1, TSql::ComparisonOperator op2, const QVariant &val, const QSqlDatabase &database);
    static QString concat(const QString &s1, TCriteria::LogicalOperator op, const QString &s2);

private:
    TCriteria criteria;
    QSqlDatabase database;
    QString tableAlias;
};


template <class T>
inline QString TCriteriaConverter<T>::toString() const
{
    return criteriaToString(QVariant::fromValue(criteria));
}


template <class T>
inline QString TCriteriaConverter<T>::criteriaToString(const QVariant &var) const
{
    QString sqlString;
    if (var.isNull()) {
        return QString();
    }

    if (var.canConvert<TCriteria>()) {
        TCriteria cri = var.value<TCriteria>();
        if (cri.isEmpty()) {
            return QString();
        }
        sqlString = concat(criteriaToString(cri.first()), cri.logicalOperator(),
                           criteriaToString(cri.second()));

    } else if (var.canConvert<TCriteriaData>()) {
        TCriteriaData cri = var.value<TCriteriaData>();
        if (cri.isEmpty()) {
            return QString();
        }

        QString name = propertyName(cri.property, database.driver(), tableAlias);
        if (name.isEmpty()) {
            return QString();
        }

        if (cri.op1 != TSql::Invalid && cri.op2 != TSql::Invalid && !cri.val1.isNull()) {
            sqlString += criteriaToString(name, (TSql::ComparisonOperator)cri.op1, (TSql::ComparisonOperator)cri.op2, cri.val1, database);

        } else if (cri.op1 != TSql::Invalid && !cri.val1.isNull() && !cri.val2.isNull()) {
            sqlString += criteriaToString(name, (TSql::ComparisonOperator)cri.op1, cri.val1, cri.val2, database);

        } else if (cri.op1 != TSql::Invalid) {
            switch(cri.op1) {
            case TSql::Equal:
            case TSql::NotEqual:
            case TSql::LessThan:
            case TSql::GreaterThan:
            case TSql::LessEqual:
            case TSql::GreaterEqual:
            case TSql::Like:
            case TSql::NotLike:
            case TSql::ILike:
            case TSql::NotILike:
                sqlString += name + TSql::formats().value(cri.op1).arg(TSqlQuery::formatValue(cri.val1, database));
                break;

            case TSql::In:
            case TSql::NotIn: {
                QString str;
                QList<QVariant> lst = cri.val1.toList();
                for (auto &v : lst) {
                    QString s = TSqlQuery::formatValue(v, database);
                    if (!s.isEmpty()) {
                        str.append(s).append(',');
                    }
                }
                str.chop(1);
                if (!str.isEmpty()) {
                    sqlString += name + TSql::formats().value(cri.op1).arg(str);
                } else {
                    tWarn("error parameter");
                }
                break; }

            case TSql::LikeEscape:
            case TSql::NotLikeEscape:
            case TSql::ILikeEscape:
            case TSql::NotILikeEscape:
            case TSql::Between:
            case TSql::NotBetween: {
                QList<QVariant> lst = cri.val1.toList();
                if (lst.count() == 2) {
                    sqlString += criteriaToString(name, (TSql::ComparisonOperator)cri.op1, lst[0], lst[1], database);
                }
                break; }

            case TSql::IsNull:
            case TSql::IsNotNull:
                sqlString += name + TSql::formats().value(cri.op1);
                break;

            case TSql::IsEmpty:
            case TSql::IsNotEmpty:
                sqlString += TSql::formats().value(cri.op1).arg(name);
                break;

            default:
                tWarn("error parameter");
                break;
            }

        } else {
            tSystemError("Logic error: [%s:%d]", __FILE__, __LINE__);
        }

    } else {
        tSystemError("Logic error [%s:%d]", __FILE__, __LINE__);
    }
    return sqlString;
}


template <class T>
inline QString TCriteriaConverter<T>::propertyName(int property, const QSqlDriver *driver, const QString &aliasTableName)
{
    const QMetaObject *metaObject = T().metaObject();
    QString name = (metaObject) ? metaObject->property(metaObject->propertyOffset() + property).name() : QString();
    if (name.isEmpty()) {
        return name;
    }

    name = TSqlQuery::escapeIdentifier(name, QSqlDriver::FieldName, driver);
    if (!aliasTableName.isEmpty()) {
        name = aliasTableName + QLatin1Char('.') + name;
    }
    return name;
}


template <class T>
inline QString TCriteriaConverter<T>::criteriaToString(const QString &propertyName, TSql::ComparisonOperator op, const QVariant &val1, const QVariant &val2, const QSqlDatabase &database)
{
    QString sqlString;
    QString v1 = TSqlQuery::formatValue(val1, database);
    QString v2 = TSqlQuery::formatValue(val2, database);

    if (!v1.isEmpty() && !v2.isEmpty()) {
        switch(op) {
        case TSql::LikeEscape:
        case TSql::NotLikeEscape:
        case TSql::ILikeEscape:
        case TSql::NotILikeEscape:
        case TSql::Between:
        case TSql::NotBetween:
            sqlString = QLatin1Char('(') + propertyName + TSql::formats().value(op).arg(v1, v2) + QLatin1Char(')');
            break;

        default:
            tWarn("Invalid parameters  [%s:%d]", __FILE__, __LINE__);
            break;
        }
    } else {
        tWarn("Invalid parameters  [%s:%d]", __FILE__, __LINE__);
    }
    return sqlString;
}


template <class T>
inline QString TCriteriaConverter<T>::criteriaToString(const QString &propertyName, TSql::ComparisonOperator op1, TSql::ComparisonOperator op2, const QVariant &val, const QSqlDatabase &database)
{
    QString sqlString;
    if (op1 != TSql::Invalid && op2 != TSql::Invalid && !val.isNull()) {
        switch(op2) {
        case TSql::Any:
        case TSql::All: {
            QString str;
            QList<QVariant> lst = val.toList();
            for (auto &v : lst) {
                QString s = TSqlQuery::formatValue(v, database);
                if (!s.isEmpty()) {
                    str.append(s).append(',');
                }
            }
            str.chop(1);
            str = TSql::formats().value(op2).arg(str);
            if (!str.isEmpty()) {
                sqlString += propertyName + TSql::formats().value(op1).arg(str);
            }
            break; }

        default:
            tWarn("Invalid parameters  [%s:%d]", __FILE__, __LINE__);
        }
    } else {
        tWarn("Invalid parameters  [%s:%d]", __FILE__, __LINE__);
    }
    return sqlString;
}


template <class T>
inline QString TCriteriaConverter<T>::concat(const QString &s1, TCriteria::LogicalOperator op, const QString &s2)
{
    if (op == TCriteria::None || (s2.isEmpty() && op != TCriteria::Not)) {
        return s1;
    }

    QString string;
    switch (op) {
    case TCriteria::And:
        string = s1 + " AND " + s2;
        break;

    case TCriteria::Or:
        string = QLatin1Char('(') + s1 + QLatin1String(" OR ") + s2 + QLatin1Char(')');
        break;

    case TCriteria::Not:
        string = QLatin1String("(NOT ") + s1 + QLatin1Char(')');
        break;

    default:
        tSystemError("Logic error: [%s:%d]", __FILE__, __LINE__);
        break;
    }

    return string;
}

#endif // TCRITERIACONVERTER_H
