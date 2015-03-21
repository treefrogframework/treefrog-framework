/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TCriteriaConverter>
#include <QAtomicPointer>

/*!
 * \class TCriteriaConverter<>
 * \brief The TCriteriaConverter class is a template class that converts
 * TCriteria objects to SQL strings.
 * This class is for internal use only.
 * \sa TCriteria
 */

static const QHash<int, QString> formatHash = {
    { TSql::Equal, "=%1" },
    { TSql::NotEqual, "<>%1" },
    { TSql::LessThan, "<%1" },
    { TSql::GreaterThan, ">%1" },
    { TSql::LessEqual, "<=%1" },
    { TSql::GreaterEqual, ">=%1" },
    { TSql::IsNull, " IS NULL" },
    { TSql::IsNotNull, " IS NOT NULL" },
    { TSql::Like, " LIKE %1" },
    { TSql::NotLike, " NOT LIKE %1" },
    { TSql::LikeEscape, " LIKE %1 ESCAPE %2" },
    { TSql::NotLikeEscape, " NOT LIKE %1 ESCAPE %2" },
    { TSql::ILike, " ILIKE %1" },
    { TSql::NotILike, " NOT ILIKE %1" },
    { TSql::ILikeEscape, " ILIKE %1 ESCAPE %2" },
    { TSql::NotILikeEscape, " NOT ILIKE %1 ESCAPE %2" },
    { TSql::In, " IN (%1)" },
    { TSql::NotIn, " NOT IN (%1)" },
    { TSql::Between, " BETWEEN %1 AND %2" },
    { TSql::NotBetween, " NOT BETWEEN %1 AND %2" },
    { TSql::Any, "ANY (%1)" },
    { TSql::All, "ALL (%1)" },
};


const QHash<int, QString> &TSql::formats()
{
    return formatHash;
}
