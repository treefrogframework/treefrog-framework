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

const QMap<int, QString> formatHash = {
    {TSql::Equal, "=%1"},
    {TSql::NotEqual, "<>%1"},
    {TSql::LessThan, "<%1"},
    {TSql::GreaterThan, ">%1"},
    {TSql::LessEqual, "<=%1"},
    {TSql::GreaterEqual, ">=%1"},
    {TSql::IsNull, " IS NULL"},
    {TSql::IsEmpty, "(%1 IS NULL OR %1='')"},
    {TSql::IsNotNull, " IS NOT NULL"},
    {TSql::IsNotEmpty, "%1 IS NOT NULL AND %1<>''"},
    {TSql::Like, " LIKE %1"},
    {TSql::NotLike, " NOT LIKE %1"},
    {TSql::LikeEscape, " LIKE %1 ESCAPE %2"},
    {TSql::NotLikeEscape, " NOT LIKE %1 ESCAPE %2"},
    {TSql::ILike, " ILIKE %1"},
    {TSql::NotILike, " NOT ILIKE %1"},
    {TSql::ILikeEscape, " ILIKE %1 ESCAPE %2"},
    {TSql::NotILikeEscape, " NOT ILIKE %1 ESCAPE %2"},
    {TSql::In, " IN (%1)"},
    {TSql::NotIn, " NOT IN (%1)"},
    {TSql::Between, " BETWEEN %1 AND %2"},
    {TSql::NotBetween, " NOT BETWEEN %1 AND %2"},
    {TSql::Any, "ANY (%1)"},
    {TSql::All, "ALL (%1)"},
};


QString TSql::formatArg(int op)
{
    return formatHash.value(op);
}


QString TSql::formatArg(int op, const QString &a)
{
    return formatHash.value(op).arg(a);
}


QString TSql::formatArg(int op, const QString &a1, const QString &a2)
{
    return formatHash.value(op).arg(a1, a2);
}
