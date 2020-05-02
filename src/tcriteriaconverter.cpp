/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QMap>
#include <TCriteriaConverter>

/*!
 * \class TCriteriaConverter<>
 * \brief The TCriteriaConverter class is a template class that converts
 * TCriteria objects to SQL strings.
 * This class is for internal use only.
 * \sa TCriteria
 */

class FormatHash : public QMap<int, QString> {
public:
    FormatHash() :
        QMap<int, QString>()
    {
        insert(TSql::Equal, "=%1");
        insert(TSql::NotEqual, "<>%1");
        insert(TSql::LessThan, "<%1");
        insert(TSql::GreaterThan, ">%1");
        insert(TSql::LessEqual, "<=%1");
        insert(TSql::GreaterEqual, ">=%1");
        insert(TSql::IsNull, " IS NULL");
        insert(TSql::IsEmpty, "(%1 IS NULL OR %1='')");
        insert(TSql::IsNotNull, " IS NOT NULL");
        insert(TSql::IsNotEmpty, "%1 IS NOT NULL AND %1<>''");
        insert(TSql::Like, " LIKE %1");
        insert(TSql::NotLike, " NOT LIKE %1");
        insert(TSql::LikeEscape, " LIKE %1 ESCAPE %2");
        insert(TSql::NotLikeEscape, " NOT LIKE %1 ESCAPE %2");
        insert(TSql::ILike, " ILIKE %1");
        insert(TSql::NotILike, " NOT ILIKE %1");
        insert(TSql::ILikeEscape, " ILIKE %1 ESCAPE %2");
        insert(TSql::NotILikeEscape, " NOT ILIKE %1 ESCAPE %2");
        insert(TSql::In, " IN (%1)");
        insert(TSql::NotIn, " NOT IN (%1)");
        insert(TSql::Between, " BETWEEN %1 AND %2");
        insert(TSql::NotBetween, " NOT BETWEEN %1 AND %2");
        insert(TSql::Any, "ANY (%1)");
        insert(TSql::All, "ALL (%1)");
    }
};
Q_GLOBAL_STATIC(FormatHash, formatHash)


QString TSql::formatArg(int op)
{
    return formatHash()->value(op);
}


QString TSql::formatArg(int op, const QString &a)
{
    return formatHash()->value(op).arg(a);
}


QString TSql::formatArg(int op, const QString &a1, const QString &a2)
{
    return formatHash()->value(op).arg(a1, a2);
}
