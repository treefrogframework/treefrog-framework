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


typedef QHash<int, QString> IntHash;
Q_GLOBAL_STATIC_WITH_INITIALIZER(IntHash, formatHash,
{
    x->insert(TSql::Equal, "=%1");
    x->insert(TSql::NotEqual, "<>%1");
    x->insert(TSql::LessThan, "<%1");
    x->insert(TSql::GreaterThan, ">%1");
    x->insert(TSql::LessEqual, "<=%1");
    x->insert(TSql::GreaterEqual, ">=%1");
    x->insert(TSql::IsNull, " IS NULL");
    x->insert(TSql::IsNotNull, " IS NOT NULL");
    x->insert(TSql::Like, " LIKE %1");
    x->insert(TSql::NotLike, " NOT LIKE %1");
    x->insert(TSql::LikeEscape, " LIKE %1 ESCAPE %2");
    x->insert(TSql::NotLikeEscape, " NOT LIKE %1 ESCAPE %2");
    x->insert(TSql::ILike, " ILIKE %1");
    x->insert(TSql::NotILike, " NOT ILIKE %1");
    x->insert(TSql::ILikeEscape, " ILIKE %1 ESCAPE %2");
    x->insert(TSql::NotILikeEscape, " NOT ILIKE %1 ESCAPE %2");
    x->insert(TSql::In, " IN (%1)");
    x->insert(TSql::NotIn, " NOT IN (%1)");
    x->insert(TSql::Between, " BETWEEN %1 AND %2");
    x->insert(TSql::NotBetween, " NOT BETWEEN %1 AND %2");
    x->insert(TSql::Any, "ANY (%1)");
    x->insert(TSql::All, "ALL (%1)");
})


const QHash<int, QString> &TSql::formats()
{
    return *formatHash();
}
