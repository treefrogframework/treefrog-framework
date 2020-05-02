/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QSqlDatabase>
#include <QSqlField>
#include <TCriteria>
#include <TCriteriaConverter>

/*!
  \class TCriteria
  \brief The TCriteria class represents a WHERE clause without SQL for
  the sake of database abstraction.
  \sa TSqlObject
*/

/*!
  Constructor.
*/
TCriteria::TCriteria() :
    logiOp(None)
{
}

/*!
  Copy constructor.
*/
TCriteria::TCriteria(const TCriteria &other) :
    cri1(other.cri1),
    cri2(other.cri2),
    logiOp(other.logiOp)
{
}

/*!
  Constructs a criteria initialized with a WHERE clause to
  which a property of ORM object with the index \a property is
  NULL value or NOT NULL value.

  @sa TCriteria &TCriteria::add(int property, TSql::ComparisonOperator op)
*/
TCriteria::TCriteria(int property, TSql::ComparisonOperator op) :
    logiOp(None)
{
    cri1 = QVariant::fromValue(TCriteriaData(property, op));
}

/*!
  Constructs a criteria initialized with a WHERE clause to which
  a property of ORM object with the index \a property equals the
  value \a val.

  @sa TCriteria &TCriteria::add(int property, const QVariant &val)
*/
TCriteria::TCriteria(int property, const QVariant &val) :
    logiOp(None)
{
    cri1 = QVariant::fromValue(TCriteriaData(property, TSql::Equal, val));
}

/*!
  Constructs a criteria initialized with a WHERE clause to which
  a property of ORM object with the index \a property is compared
  with the \a val value by a means of the \a op parameter.

  @sa TCriteria &TCriteria::add(int property, TSql::ComparisonOperator op, const QVariant &val)
*/
TCriteria::TCriteria(int property, TSql::ComparisonOperator op, const QVariant &val) :
    logiOp(None)
{
    cri1 = QVariant::fromValue(TCriteriaData(property, op, val));
}

/*!
  Constructs a criteria initialized with
  a WHERE clause to which a property of ORM object with
  the index \a property is compared with the \a val1 value and
  the \a val2 value by a means of the \a op parameter.

  @sa TCriteria &TCriteria::add(int property, TSql::ComparisonOperator op, const QVariant &val1, const QVariant &val2)
*/
TCriteria::TCriteria(int property, TSql::ComparisonOperator op, const QVariant &val1, const QVariant &val2) :
    logiOp(None)
{
    cri1 = QVariant::fromValue(TCriteriaData(property, op, val1, val2));
}

/*!
  Constructs a criteria initialized with
  a WHERE clause to which a property of ORM object with
  the index \a property is compared with the \a val value by
  a means of the \a op1 and \a op2 parameter.

  @sa TCriteria &TCriteria::add(int property, TSql::ComparisonOperator op1, TSql::ComparisonOperator op2, const QVariant &val)
*/
TCriteria::TCriteria(int property, TSql::ComparisonOperator op1, TSql::ComparisonOperator op2, const QVariant &val) :
    logiOp(None)
{
    cri1 = QVariant::fromValue(TCriteriaData(property, op1, op2, val));
}


TCriteria::TCriteria(int property, TMongo::ComparisonOperator op) :
    logiOp(None)
{
    cri1 = QVariant::fromValue(TCriteriaData(property, op));
}


TCriteria::TCriteria(int property, TMongo::ComparisonOperator op, const QVariant &val) :
    logiOp(None)
{
    cri1 = QVariant::fromValue(TCriteriaData(property, op, val));
}

/*!
  Adds a WHERE clause to which a property of ORM object with
  the index \a property is NULL value or NOT NULL value.
  The \a op parameter must be TSql::IsNull or TSql::IsNotNull.
*/
TCriteria &TCriteria::add(int property, TSql::ComparisonOperator op)
{
    return add(And, TCriteria(property, op));
}

/*!
  Adds a WHERE clause to which a property of ORM object with
  the index \a property equals the value \a val.
*/
TCriteria &TCriteria::add(int property, const QVariant &val)
{
    return add(And, TCriteria(property, val));
}

/*!
  Adds a WHERE clause to which a property of ORM object with
  the index \a property is compared with the \a val value by
  a means of the \a op parameter.
  The \a op parameter must be one of the following constants:\n
  TSql::Equal, TSql::NotEqual, TSql::LessThan, TSql::GreaterThan,
  TSql::LessEqual, TSql::GreaterEqual, TSql::Like, TSql::NotLike,
  TSql::ILike, TSql::NotILike.
*/
TCriteria &TCriteria::add(int property, TSql::ComparisonOperator op, const QVariant &val)
{
    return add(And, TCriteria(property, op, val));
}

/*!
  Adds a WHERE clause to which a property of ORM object with
  the index \a property is compared with the \a val1 value and
  the \a val2 value by a means of the \a op parameter.
  The \a op parameter must be one of the following constants:\n
  TSql::LikeEscape, TSql::NotLikeEscape, TSql::ILikeEscape,
  TSql::NotILikeEscape, TSql::Between, TSql::NotBetween.
 */
TCriteria &TCriteria::add(int property, TSql::ComparisonOperator op, const QVariant &val1, const QVariant &val2)
{
    return add(And, TCriteria(property, op, val1, val2));
}

/*!
  Adds a WHERE clause to which a property of ORM object with
  the index \a property is compared with the \a val value by
  a means of the \a op1 and \a op2 parameter. This function is
  used with \a TSql::Any or \a TSql::All constant as the
  \a op2 parameter to generate a WHERE clause such as
  "column >= any (10, 20, 50)".
*/
TCriteria &TCriteria::add(int property, TSql::ComparisonOperator op1, TSql::ComparisonOperator op2, const QVariant &val)
{
    return add(And, TCriteria(property, op1, op2, val));
}


TCriteria &TCriteria::add(int property, TMongo::ComparisonOperator op)
{
    return add(And, TCriteria(property, op));
}

TCriteria &TCriteria::add(int property, TMongo::ComparisonOperator op, const QVariant &val)
{
    return add(And, TCriteria(property, op, val));
}

/*!
  Adds a WHERE clause of the \a criteria parameter with the
  AND operator.
*/
TCriteria &TCriteria::add(const TCriteria &criteria)
{
    return add(And, criteria);
}

/*!
  Adds a WHERE clause with OR operator to which a property of
  ORM object with the index \a property is NULL value or NOT
  NULL value. The \a op parameter must be TSql::IsNull or
  TSql::IsNotNull.
*/
TCriteria &TCriteria::addOr(int property, TSql::ComparisonOperator op)
{
    return add(Or, TCriteria(property, op));
}

/*!
  Adds a WHERE clause with OR operator to which a property
  of ORM object with the index \a property equals the value
  \a val.
*/
TCriteria &TCriteria::addOr(int property, const QVariant &val)
{
    return add(Or, TCriteria(property, val));
}

/*!
  Adds a WHERE clause with OR operator to which a property
  of ORM object with the index \a property is compared with
  the \a val value by a means of the \a op parameter.
*/
TCriteria &TCriteria::addOr(int property, TSql::ComparisonOperator op, const QVariant &val)
{
    return add(Or, TCriteria(property, op, val));
}

/*!
  Adds a WHERE clause with OR operator to which a property
  of ORM object with the index \a property is compared with
  the \a val1 value and the \a val2 value by a means of the
  \a op parameter.
  The \a op parameter must be one of the following constants:\n
  TSql::LikeEscape, TSql::NotLikeEscape, TSql::ILikeEscape,
  TSql::NotILikeEscape, TSql::Between, TSql::NotBetween.
*/
TCriteria &TCriteria::addOr(int property, TSql::ComparisonOperator op, const QVariant &val1, const QVariant &val2)
{
    return add(Or, TCriteria(property, op, val1, val2));
}

/*!
  Adds a WHERE clause  with OR operator to which a property
  of ORM object with the index \a property is compared with
  the \a val value by a means of the \a op1 and \a op2
  parameter. This function is used with \a TSql::Any or
  \a TSql::All constant as the \a op2 parameter to generate
  a WHERE clause such as
  "column >= any (10, 20, 50)".
*/
TCriteria &TCriteria::addOr(int property, TSql::ComparisonOperator op1, TSql::ComparisonOperator op2, const QVariant &val)
{
    return add(Or, TCriteria(property, op1, op2, val));
}


TCriteria &TCriteria::addOr(int property, TMongo::ComparisonOperator op)
{
    return add(Or, TCriteria(property, op));
}

TCriteria &TCriteria::addOr(int property, TMongo::ComparisonOperator op, const QVariant &val)
{
    return add(Or, TCriteria(property, op, val));
}

/*!
  Adds a WHERE clause of the \a criteria parameter with the
  OR operator.
*/
TCriteria &TCriteria::addOr(const TCriteria &criteria)
{
    return add(Or, criteria);
}

/*!
  Adds a WHERE clause of the \a criteria parameter with the
  \a op operator.
*/
TCriteria &TCriteria::add(LogicalOperator op, const TCriteria &criteria)
{
    if (cri1.isNull()) {
        cri1 = QVariant::fromValue(criteria);
        logiOp = None;
        cri2.clear();
    } else {
        if (logiOp != None) {
            cri1 = QVariant::fromValue(*this);
        }
        logiOp = op;
        cri2 = QVariant::fromValue(criteria);
    }
    return *this;
}

/*!
  Adds a WHERE clause of the \a criteria parameter with the
  AND operator.
  @sa TCriteria &TCriteria::add(const TCriteria &criteria)
*/
const TCriteria TCriteria::operator&&(const TCriteria &criteria) const
{
    TCriteria res(*this);
    res.add(criteria);
    return res;
}

/*!
  Adds a WHERE clause of the \a criteria parameter with the
  OR operator.
  @sa TCriteria &TCriteria::addOr(const TCriteria &criteria)
*/
const TCriteria TCriteria::operator||(const TCriteria &criteria) const
{
    TCriteria res(*this);
    res.addOr(criteria);
    return res;
}

/*!
  Returns a WHERE clause that negated this criteria.
*/
const TCriteria TCriteria::operator!() const
{
    TCriteria cri(*this);
    cri.logiOp = Not;
    return cri;
}

/*!
  Assignment operator.
*/
TCriteria &TCriteria::operator=(const TCriteria &other)
{
    cri1 = other.cri1;
    cri2 = other.cri2;
    logiOp = other.logiOp;
    return *this;
}

/*!
  Returns true if the criteria has no data; otherwise returns false.
*/
bool TCriteria::isEmpty() const
{
    return cri1.isNull();
}

/*!
  Clears the criteria and makes it empty.
*/
void TCriteria::clear()
{
    cri1.clear();
    logiOp = None;
    cri2.clear();
}

/*!
  \fn const QVariant &TCriteria::first() const
  This function is for internal use only.
*/

/*!
  \fn const QVariant &TCriteria::second() const
  This function is for internal use only.
*/

/*!
  \fn LogicalOperator TCriteria::logicalOperator() const
  This function is for internal use only.
*/
