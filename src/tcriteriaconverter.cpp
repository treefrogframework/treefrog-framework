/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TCriteriaConverter>
#include <QMutexLocker>

/*!
 * \class TCriteriaConverter<>
 * \brief The TCriteriaConverter class is a template class that converts
 * TCriteria objects to SQL strings.
 * This class is for internal use only.
 * \sa TCriteria
 */

static QHash<int, QString> formatVector;
static QMutex mutex;


const QHash<int, QString> &TCriteriaData::formats()
{
    if (formatVector.isEmpty()) {
        QMutexLocker lock(&mutex);
        formatVector.clear();
        formatVector.insert(TSql::Equal, "=%1");
        formatVector.insert(TSql::NotEqual, "<>%1");
        formatVector.insert(TSql::LessThan, "<%1");
        formatVector.insert(TSql::GreaterThan, ">%1");
        formatVector.insert(TSql::LessEqual, "<=%1");
        formatVector.insert(TSql::GreaterEqual, ">=%1");
        formatVector.insert(TSql::IsNull, " IS NULL");
        formatVector.insert(TSql::IsNotNull, " IS NOT NULL");
        formatVector.insert(TSql::Like, " LIKE %1");
        formatVector.insert(TSql::NotLike, " NOT LIKE %1");
        formatVector.insert(TSql::LikeEscape, " LIKE %1 ESCAPE %2");
        formatVector.insert(TSql::NotLikeEscape, " NOT LIKE %1 ESCAPE %2");
        formatVector.insert(TSql::ILike, " ILIKE %1");
        formatVector.insert(TSql::NotILike, " NOT ILIKE %1");
        formatVector.insert(TSql::ILikeEscape, " ILIKE %1 ESCAPE %2");
        formatVector.insert(TSql::NotILikeEscape, " NOT ILIKE %1 ESCAPE %2");
        formatVector.insert(TSql::In, " IN (%1)");
        formatVector.insert(TSql::NotIn, " NOT IN (%1)");
        formatVector.insert(TSql::Between, " BETWEEN %1 AND %2");
        formatVector.insert(TSql::NotBetween, " NOT BETWEEN %1 AND %2");
        formatVector.insert(TSql::Any, "ANY (%1)");
        formatVector.insert(TSql::All, "ALL (%1)");
    }
    return formatVector;
} 
