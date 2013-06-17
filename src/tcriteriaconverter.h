#ifndef TCRITERIACONVERTER_H
#define TCRITERIACONVERTER_H

#include <QMetaObject>
#include <QVariant>
#include <QHash>
#include <TCriteria>
#include <TSqlQuery>
#include <TGlobal>
#include "tsystemglobal.h"


/*!
  TCriteriaData class is a class for criteria data objects.
  \sa TCriteria
 */
class T_CORE_EXPORT TCriteriaData
{
public:
    TCriteriaData();
    TCriteriaData(const TCriteriaData &other);
    TCriteriaData(int property, TSql::ComparisonOperator op);
    TCriteriaData(int property, TSql::ComparisonOperator op, const QVariant &val);
    TCriteriaData(int property, TSql::ComparisonOperator op, const QVariant &val1, const QVariant &val2);
    TCriteriaData(int property, TSql::ComparisonOperator op1, TSql::ComparisonOperator op2, const QVariant &val);
    bool isEmpty() const;
    static const QHash<int, QString> &formats();

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


inline TCriteriaData::TCriteriaData(int property, TSql::ComparisonOperator op)
    : property(property), op1(op), op2(TSql::Invalid)
{ }


inline TCriteriaData::TCriteriaData(int property, TSql::ComparisonOperator op, const QVariant &val)
    : property(property), op1(op), op2(TSql::Invalid), val1(val)
{ }


inline TCriteriaData::TCriteriaData(int property, TSql::ComparisonOperator op, const QVariant &val1, const QVariant &val2)
    : property(property), op1(op), op2(TSql::Invalid), val1(val1), val2(val2)
{ }


inline TCriteriaData::TCriteriaData(int property, TSql::ComparisonOperator op1, TSql::ComparisonOperator op2, const QVariant &val)
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
    TCriteriaConverter(const TCriteria &cri, const QSqlDatabase &db) : criteria(cri), database(db) { }
    QString toString() const;
    static QString propertyName(int property);

protected:
    static QString criteriaToString(const QVariant &cri, const QSqlDatabase &database);
    static QString criteriaToString(const QString &propertyName, TSql::ComparisonOperator op, const QVariant &val1, const QVariant &val2, const QSqlDatabase &database);
    static QString criteriaToString(const QString &propertyName, TSql::ComparisonOperator op1, TSql::ComparisonOperator op2, const QVariant &val, const QSqlDatabase &database);
    static QString join(const QString &s1, TCriteria::LogicalOperator op, const QString &s2);

private:
    TCriteria criteria;
    const QSqlDatabase &database;
};


template <class T>
inline QString TCriteriaConverter<T>::toString() const
{
    return criteriaToString(QVariant::fromValue(criteria), database);
}


template <class T>
inline QString TCriteriaConverter<T>::criteriaToString(const QVariant &var, const QSqlDatabase &database)
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
        sqlString = join(criteriaToString(cri.first(), database), cri.logicalOperator(),
                         criteriaToString(cri.second(), database));
    
    } else if (var.canConvert<TCriteriaData>()) {
        TCriteriaData cri = var.value<TCriteriaData>();
        if (cri.isEmpty()) {
            return QString();
        }
        
        QString name = propertyName(cri.property);
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
                sqlString += name + TCriteriaData::formats().value(cri.op1).arg(TSqlQuery::formatValue(cri.val1, database));
                break;
                
            case TSql::In:
            case TSql::NotIn: {
                QString str;
                QList<QVariant> lst = cri.val1.toList();
                QListIterator<QVariant> i(lst);
                while (i.hasNext()) {
                    QString s = TSqlQuery::formatValue(i.next(), database);
                    if (!s.isEmpty()) {
                        str.append(s).append(',');
                    }
                }
                str.chop(1);
                if (!str.isEmpty()) {
                    sqlString += name + TCriteriaData::formats().value(cri.op1).arg(str);
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
                sqlString += name + TCriteriaData::formats().value(cri.op1);
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
inline QString TCriteriaConverter<T>::propertyName(int property)
{
    const QMetaObject *metaObject = T().metaObject();
    return (metaObject) ? metaObject->property(metaObject->propertyOffset() + property).name() : QString();
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
            sqlString = "(" + propertyName + TCriteriaData::formats().value(op).arg(v1, v2) + ")";
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
            QList<QVariant> list = val.toList();
            QListIterator<QVariant> i(list);
            while (i.hasNext()) {
                QString s = TSqlQuery::formatValue(i.next(), database);
                if (!s.isEmpty()) {
                    str.append(s).append(',');
                } 
            }
            str.chop(1);
            str = TCriteriaData::formats().value(op2).arg(str);
            if (!str.isEmpty()) {
                sqlString += propertyName + TCriteriaData::formats().value(op1).arg(str);
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
inline QString TCriteriaConverter<T>::join(const QString &s1, TCriteria::LogicalOperator op, const QString &s2)
{
    if (op == TCriteria::None || s2.isEmpty()) {
        return s1;
    }
    
    QString string;
    if (op == TCriteria::And) {
        string = s1 + " AND " + s2;
    } else if (op == TCriteria::Or) {
        string = QLatin1String("( ") + s1 + QLatin1String(" OR ") + s2 + QLatin1String(" )");
    } else {
        tSystemError("Logic error: [%s:%d]", __FILE__, __LINE__);
    }
    return string;
}

#endif // TCRITERIACONVERTER_H
