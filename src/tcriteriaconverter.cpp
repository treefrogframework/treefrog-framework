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

static QAtomicPointer<QHash<int, QString> > formatVector = 0;


const QHash<int, QString> &TCriteriaData::formats()
{
    QHash<int, QString> *ret = formatVector.fetchAndAddOrdered(0);

    if (!ret) {
        ret = new QHash<int, QString>();
        ret->insert(TSql::Equal, "=%1");
        ret->insert(TSql::NotEqual, "<>%1");
        ret->insert(TSql::LessThan, "<%1");
        ret->insert(TSql::GreaterThan, ">%1");
        ret->insert(TSql::LessEqual, "<=%1");
        ret->insert(TSql::GreaterEqual, ">=%1");
        ret->insert(TSql::IsNull, " IS NULL");
        ret->insert(TSql::IsNotNull, " IS NOT NULL");
        ret->insert(TSql::Like, " LIKE %1");
        ret->insert(TSql::NotLike, " NOT LIKE %1");
        ret->insert(TSql::LikeEscape, " LIKE %1 ESCAPE %2");
        ret->insert(TSql::NotLikeEscape, " NOT LIKE %1 ESCAPE %2");
        ret->insert(TSql::ILike, " ILIKE %1");
        ret->insert(TSql::NotILike, " NOT ILIKE %1");
        ret->insert(TSql::ILikeEscape, " ILIKE %1 ESCAPE %2");
        ret->insert(TSql::NotILikeEscape, " NOT ILIKE %1 ESCAPE %2");
        ret->insert(TSql::In, " IN (%1)");
        ret->insert(TSql::NotIn, " NOT IN (%1)");
        ret->insert(TSql::Between, " BETWEEN %1 AND %2");
        ret->insert(TSql::NotBetween, " NOT BETWEEN %1 AND %2");
        ret->insert(TSql::Any, "ANY (%1)");
        ret->insert(TSql::All, "ALL (%1)");

        if (!formatVector.testAndSetOrdered(0, ret)) {
            delete ret;
            ret = formatVector.fetchAndAddOrdered(0);
        }
    }
    return *ret;
}
