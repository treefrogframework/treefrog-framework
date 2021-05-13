#pragma once
#include "tsystemglobal.h"
#include <QMetaObject>
#include <QVariant>
#include <TCriteria>
#include <TGlobal>
#include <TSqlQuery>

namespace TSql {
T_CORE_EXPORT QString formatArg(int op);
T_CORE_EXPORT QString formatArg(int op, const QString &a);
T_CORE_EXPORT QString formatArg(int op, const QString &a1, const QString &a2);
}

/*!
  TCriteriaData class is a class for criteria data objects.
  \sa TCriteria
 */
class T_CORE_EXPORT TCriteriaData {
public:
    TCriteriaData();
    TCriteriaData(const TCriteriaData &other);
    TCriteriaData(int property, int op);
    TCriteriaData(int property, int op, const QVariant &val);
    TCriteriaData(int property, int op, const QVariant &val1, const QVariant &val2);
    TCriteriaData(int property, int op1, int op2, const QVariant &val);
    bool isEmpty() const;

    int property {-1};
#if QT_VERSION < 0x060000
    QVariant::Type varType {QVariant::Invalid};
#else
    QMetaType varType {QMetaType::UnknownType};
#endif
    int op1 {TSql::Invalid};
    int op2 {TSql::Invalid};
    QVariant val1;
    QVariant val2;
};


inline TCriteriaData::TCriteriaData()
{
}


inline TCriteriaData::TCriteriaData(const TCriteriaData &other) :
    property(other.property), op1(other.op1), op2(other.op2), val1(other.val1), val2(other.val2)
{
}


inline TCriteriaData::TCriteriaData(int property, int op) :
    property(property), op1(op), op2(TSql::Invalid)
{
}


inline TCriteriaData::TCriteriaData(int property, int op, const QVariant &val) :
    property(property), op1(op), op2(TSql::Invalid), val1(val)
{
}


inline TCriteriaData::TCriteriaData(int property, int op, const QVariant &val1, const QVariant &val2) :
    property(property), op1(op), op2(TSql::Invalid), val1(val1), val2(val2)
{
}


inline TCriteriaData::TCriteriaData(int property, int op1, int op2, const QVariant &val) :
    property(property), op1(op1), op2(op2), val1(val)
{
}


inline bool TCriteriaData::isEmpty() const
{
    return (property < 0 || op1 == TSql::Invalid);
}

Q_DECLARE_METATYPE(TCriteriaData)


template <class T>
class TCriteriaConverter {
public:
    TCriteriaConverter(const TCriteria &cri, const QSqlDatabase &db, const QString &aliasTableName = QString()) :
        criteria(cri), database(db), tableAlias(aliasTableName) { }
    QString toString() const;
#if QT_VERSION < 0x060000
    QVariant::Type variantType(int property) const;
#else
    QMetaType variantType(int property) const;
#endif
    QString propertyName(int property, const QSqlDriver *driver, const QString &aliasTableName = QString()) const;
    static QString getPropertyName(int property, const QSqlDriver *driver, const QString &aliasTableName = QString());

protected:
    static QString getPropertyName(const QMetaObject *metaObject, int property, const QSqlDriver *driver, const QString &aliasTableName);
    QString criteriaToString(const QVariant &cri) const;
#if QT_VERSION < 0x060000
    static QString criteriaToString(const QString &propertyName, QVariant::Type varType, TSql::ComparisonOperator op, const QVariant &val1, const QVariant &val2, const QSqlDatabase &database);
    static QString criteriaToString(const QString &propertyName, QVariant::Type varType, TSql::ComparisonOperator op1, TSql::ComparisonOperator op2, const QVariant &val, const QSqlDatabase &database);
#else
    static QString criteriaToString(const QString &propertyName, const QMetaType &varType, TSql::ComparisonOperator op, const QVariant &val1, const QVariant &val2, const QSqlDatabase &database);
    static QString criteriaToString(const QString &propertyName, const QMetaType &varType, TSql::ComparisonOperator op1, TSql::ComparisonOperator op2, const QVariant &val, const QSqlDatabase &database);
#endif
    static QString concat(const QString &s1, TCriteria::LogicalOperator op, const QString &s2);

private:
    T obj;
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
    sqlString.reserve(1024);

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
        cri.varType = variantType(cri.property);

        QString name = propertyName(cri.property, database.driver(), tableAlias);
        if (name.isEmpty()) {
            return QString();
        }

        if (cri.op1 != TSql::Invalid && cri.op2 != TSql::Invalid && !cri.val1.isNull()) {
            sqlString += criteriaToString(name, cri.varType, (TSql::ComparisonOperator)cri.op1, (TSql::ComparisonOperator)cri.op2, cri.val1, database);

        } else if (cri.op1 != TSql::Invalid && !cri.val1.isNull() && !cri.val2.isNull()) {
            sqlString += criteriaToString(name, cri.varType, (TSql::ComparisonOperator)cri.op1, cri.val1, cri.val2, database);

        } else if (cri.op1 != TSql::Invalid) {
            switch (cri.op1) {
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
                sqlString += name + TSql::formatArg(cri.op1, TSqlQuery::formatValue(cri.val1, cri.varType, database));
                break;

            case TSql::In:
            case TSql::NotIn: {
                auto inclause = [&](const QList<QVariant> &lst, int pos, int length) {
                    QString str;
                    length = qMin(length, lst.count() - pos);
                    for (int i = 0; i < length; i++) {
                        auto &v = lst[pos + i];
                        QString s = TSqlQuery::formatValue(v, cri.varType, database);
                        if (!s.isEmpty()) {
                            str.append(s).append(',');
                        }
                    }
                    str.chop(1);
                    return str;
                };

                constexpr int LIMIT_WORDS_IN_CLAUSE = 1000;
                const QList<QVariant> lst = cri.val1.toList();

                if (lst.isEmpty()) {
                    tWarn("error parameter");
                    break;
                }

#if QT_VERSION >= 0x050400
                if (lst.count() <= LIMIT_WORDS_IN_CLAUSE || database.driver()->dbmsType() == QSqlDriver::MySqlServer) {
#else
                if (lst.count() <= LIMIT_WORDS_IN_CLAUSE || database.driverName().toUpper() == QLatin1String("QMYSQL")) {
#endif
                    QString str = inclause(lst, 0, lst.count());
                    sqlString += name;
                    sqlString += TSql::formatArg(cri.op1, str);
                } else {
                    QString clause;
                    for (int i = 0; i < (lst.count() / LIMIT_WORDS_IN_CLAUSE) + 1; i++) {
                        QString str = inclause(lst, LIMIT_WORDS_IN_CLAUSE * i, LIMIT_WORDS_IN_CLAUSE);
                        clause += name;
                        clause += TSql::formatArg(cri.op1, str);
                        clause += QLatin1String(" OR ");
                    }
                    clause.chop(4);

                    sqlString += QLatin1Char('(');
                    sqlString += clause;
                    sqlString += QLatin1Char(')');
                }
                break;
            }

            case TSql::LikeEscape:
            case TSql::NotLikeEscape:
            case TSql::ILikeEscape:
            case TSql::NotILikeEscape:
            case TSql::Between:
            case TSql::NotBetween: {
                QList<QVariant> lst = cri.val1.toList();
                if (lst.count() == 2) {
                    sqlString += criteriaToString(name, cri.varType, (TSql::ComparisonOperator)cri.op1, lst[0], lst[1], database);
                }
                break;
            }

            case TSql::IsNull:
            case TSql::IsNotNull:
                sqlString += name;
                sqlString += TSql::formatArg(cri.op1);
                break;

            case TSql::IsEmpty:
            case TSql::IsNotEmpty:
                sqlString += TSql::formatArg(cri.op1, name);
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
inline QString TCriteriaConverter<T>::getPropertyName(const QMetaObject *metaObject, int property, const QSqlDriver *driver, const QString &aliasTableName)
{
    if (property < 0) {
        return QString();
    }

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
inline QString TCriteriaConverter<T>::propertyName(int property, const QSqlDriver *driver, const QString &aliasTableName) const
{
    return getPropertyName(obj.metaObject(), property, driver, aliasTableName);
}


template <class T>
inline QString TCriteriaConverter<T>::getPropertyName(int property, const QSqlDriver *driver, const QString &aliasTableName)
{
    return getPropertyName(T().metaObject(), property, driver, aliasTableName);
}

#if QT_VERSION < 0x060000
template <class T>
inline QVariant::Type TCriteriaConverter<T>::variantType(int property) const
{
    const QMetaObject *metaObject = obj.metaObject();
    return (metaObject) ? metaObject->property(metaObject->propertyOffset() + property).type() : QVariant::Invalid;
}
#else
template <class T>
inline QMetaType TCriteriaConverter<T>::variantType(int property) const
{
    const QMetaObject *metaObject = obj.metaObject();
    return (metaObject) ? metaObject->property(metaObject->propertyOffset() + property).metaType() : QMetaType(QMetaType::UnknownType);
}
#endif

#if QT_VERSION < 0x060000
template <class T>
inline QString TCriteriaConverter<T>::criteriaToString(const QString &propertyName, QVariant::Type varType, TSql::ComparisonOperator op, const QVariant &val1, const QVariant &val2, const QSqlDatabase &database)
#else
template <class T>
inline QString TCriteriaConverter<T>::criteriaToString(const QString &propertyName, const QMetaType &varType, TSql::ComparisonOperator op, const QVariant &val1, const QVariant &val2, const QSqlDatabase &database)
#endif
{
    QString sqlString;

#if QT_VERSION < 0x060000
    QString v1 = TSqlQuery::formatValue(val1, (QVariant::Type)varType, database);
    QString v2 = TSqlQuery::formatValue(val2, (QVariant::Type)varType, database);
#else
    QString v1 = TSqlQuery::formatValue(val1, varType, database);
    QString v2 = TSqlQuery::formatValue(val2, varType, database);
#endif

    if (!v1.isEmpty() && !v2.isEmpty()) {
        switch (op) {
        case TSql::LikeEscape:
        case TSql::NotLikeEscape:
        case TSql::ILikeEscape:
        case TSql::NotILikeEscape:
        case TSql::Between:
        case TSql::NotBetween:
            sqlString = QLatin1Char('(');
            sqlString += propertyName;
            sqlString += TSql::formatArg(op, v1, v2);
            sqlString += QLatin1Char(')');
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


#if QT_VERSION < 0x060000
template <class T>
inline QString TCriteriaConverter<T>::criteriaToString(const QString &propertyName, QVariant::Type varType, TSql::ComparisonOperator op1, TSql::ComparisonOperator op2, const QVariant &val, const QSqlDatabase &database)
#else
template <class T>
inline QString TCriteriaConverter<T>::criteriaToString(const QString &propertyName, const QMetaType &varType, TSql::ComparisonOperator op1, TSql::ComparisonOperator op2, const QVariant &val, const QSqlDatabase &database)
#endif
{
    QString sqlString;
    if (op1 != TSql::Invalid && op2 != TSql::Invalid && !val.isNull()) {
        switch (op2) {
        case TSql::Any:
        case TSql::All: {
            QString str;
            const QList<QVariant> lst = val.toList();
            for (auto &v : lst) {
                QString s = TSqlQuery::formatValue(v, varType, database);
                if (!s.isEmpty()) {
                    str.append(s).append(',');
                }
            }
            str.chop(1);
            str = TSql::formatArg(op2, str);
            if (!str.isEmpty()) {
                sqlString += propertyName;
                sqlString += TSql::formatArg(op1, str);
            }
            break;
        }

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

