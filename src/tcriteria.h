#pragma once
#include <QVariant>
#include <TGlobal>


class T_CORE_EXPORT TCriteria {
public:
    TCriteria();
    TCriteria(const TCriteria &other);

    TCriteria(int property, const QVariant &val);
    TCriteria(int property, TSql::ComparisonOperator op);
    TCriteria(int property, TSql::ComparisonOperator op, const QVariant &val);
    TCriteria(int property, TSql::ComparisonOperator op, const QVariant &val1, const QVariant &val2);
    TCriteria(int property, TSql::ComparisonOperator op1, TSql::ComparisonOperator op2, const QVariant &val);
    TCriteria(int property, TMongo::ComparisonOperator op);
    TCriteria(int property, TMongo::ComparisonOperator op, const QVariant &val);
    ~TCriteria() { }

    TCriteria &add(int property, const QVariant &val);
    TCriteria &add(int property, TSql::ComparisonOperator op);
    TCriteria &add(int property, TSql::ComparisonOperator op, const QVariant &val);
    TCriteria &add(int property, TSql::ComparisonOperator op, const QVariant &val1, const QVariant &val2);
    TCriteria &add(int property, TSql::ComparisonOperator op1, TSql::ComparisonOperator op2, const QVariant &val);
    TCriteria &add(const TCriteria &criteria);
    TCriteria &addOr(int property, const QVariant &val);
    TCriteria &addOr(int property, TSql::ComparisonOperator op);
    TCriteria &addOr(int property, TSql::ComparisonOperator op, const QVariant &val);
    TCriteria &addOr(int property, TSql::ComparisonOperator op, const QVariant &val1, const QVariant &val2);
    TCriteria &addOr(int property, TSql::ComparisonOperator op1, TSql::ComparisonOperator op2, const QVariant &val);
    TCriteria &addOr(const TCriteria &criteria);

    // For MongoDB
    TCriteria &add(int property, TMongo::ComparisonOperator op);
    TCriteria &add(int property, TMongo::ComparisonOperator op, const QVariant &val);
    TCriteria &addOr(int property, TMongo::ComparisonOperator op);
    TCriteria &addOr(int property, TMongo::ComparisonOperator op, const QVariant &val);

    bool isEmpty() const;
    void clear();

    const TCriteria operator&&(const TCriteria &criteria) const;
    const TCriteria operator||(const TCriteria &criteria) const;
    const TCriteria operator!() const;
    TCriteria &operator=(const TCriteria &other);

protected:
    enum LogicalOperator {
        None = 0,
        And,
        Or,
        Not,
    };

    const QVariant &first() const { return cri1; }
    const QVariant &second() const { return cri2; }
    LogicalOperator logicalOperator() const { return (LogicalOperator)logiOp; }
    TCriteria &add(LogicalOperator op, const TCriteria &criteria);

private:
    QVariant cri1;
    QVariant cri2;
    int logiOp {None};

    template <class T>
    friend class TCriteriaConverter;
    template <class T>
    friend class TCriteriaMongoConverter;
};

Q_DECLARE_METATYPE(TCriteria)

