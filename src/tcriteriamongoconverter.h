#pragma once
#include "tsystemglobal.h"
#include <QMap>
#include <QMetaObject>
#include <QMetaProperty>
#include <QVariant>
#include <TCriteria>
#include <TCriteriaConverter>
#include <TGlobal>


template <class T>
class TCriteriaMongoConverter {
public:
    TCriteriaMongoConverter(const TCriteria &cri) :
        criteria(cri) { }
    QVariantMap toVariantMap() const;
    static QString propertyName(int property);

protected:
    static QVariantMap criteriaToVariantMap(const QVariant &cri);
    static QVariantMap join(const QVariantMap &v1, TCriteria::LogicalOperator op, const QVariantMap &v2);

private:
    TCriteria criteria;
};


template <class T>
inline QVariantMap TCriteriaMongoConverter<T>::toVariantMap() const
{
    return criteriaToVariantMap(QVariant::fromValue(criteria));
}


template <class T>
inline QVariantMap TCriteriaMongoConverter<T>::criteriaToVariantMap(const QVariant &var)
{
    QVariantMap ret;

    if (var.isNull()) {
        return ret;
    }

    if (var.canConvert<TCriteria>()) {
        TCriteria cri = var.value<TCriteria>();
        if (!cri.isEmpty()) {
            ret = join(criteriaToVariantMap(cri.first()), cri.logicalOperator(), criteriaToVariantMap(cri.second()));
        }

    } else if (var.canConvert<TCriteriaData>()) {
        TCriteriaData cri = var.value<TCriteriaData>();
        QString name = propertyName(cri.property);
        if (cri.isEmpty() || name.isEmpty()) {
            return ret;
        }

        switch (cri.op1) {
        case TMongo::Equal:
            ret.insert(name, cri.val1);
            break;

        case TMongo::NotEqual: {
            QVariantMap ne;
            ne.insert("$ne", cri.val1);
            ret.insert(name, QVariant(ne));
            break;
        }

        case TMongo::LessThan: {
            QVariantMap lt;
            lt.insert("$lt", cri.val1);
            ret.insert(name, QVariant(lt));
            break;
        }

        case TMongo::GreaterThan: {
            QVariantMap gt;
            gt.insert("$gt", cri.val1);
            ret.insert(name, QVariant(gt));
            break;
        }

        case TMongo::LessEqual: {
            QVariantMap le;
            le.insert("$lte", cri.val1);
            ret.insert(name, QVariant(le));
            break;
        }

        case TMongo::GreaterEqual: {
            QVariantMap ge;
            ge.insert("$gte", cri.val1);
            ret.insert(name, QVariant(ge));
            break;
        }

        case TMongo::Exists: {
            QVariantMap ex;
            ex.insert("$exists", true);
            ret.insert(name, ex);
            break;
        }

        case TMongo::NotExists: {
            QVariantMap nex;
            nex.insert("$exists", false);
            ret.insert(name, nex);
            break;
        }

        case TMongo::All: {
            QVariantMap all;
            all.insert("$all", cri.val1);
            ret.insert(name, all);
            break;
        }

        case TMongo::In: {
            QVariantMap in;
            in.insert("$in", cri.val1);
            ret.insert(name, in);
            break;
        }

        case TMongo::NotIn: {
            QVariantMap nin;
            nin.insert("$nin", cri.val1);
            ret.insert(name, nin);
            break;
        }

        case TMongo::Mod: {
            QVariantMap mod;
            mod.insert("$mod", cri.val1);
            ret.insert(name, mod);
            break;
        }

        case TMongo::Size: {
            QVariantMap sz;
            sz.insert("$size", cri.val1);
            ret.insert(name, sz);
            break;
        }

        case TMongo::Type: {
            QVariantMap ty;
            ty.insert("$type", cri.val1);
            ret.insert(name, ty);
            break;
        }

        default:
            tWarn("error parameter: %d", cri.op1);
            break;
        }

    } else {
        tSystemError("Logic error [%s:%d]", __FILE__, __LINE__);
    }
    return ret;
}


template <class T>
inline QString TCriteriaMongoConverter<T>::propertyName(int property)
{
    const QMetaObject *metaObject = T().metaObject();
    return (metaObject) ? metaObject->property(metaObject->propertyOffset() + property).name() : QString();
}


template <class T>
inline QVariantMap TCriteriaMongoConverter<T>::join(const QVariantMap &v1, TCriteria::LogicalOperator op, const QVariantMap &v2)
{
    if (op == TCriteria::None || v2.isEmpty()) {
        return v1;
    }

    QVariantMap ret;
    if (op == TCriteria::And) {
#if QT_VERSION >= 0x050f00  // 5.15.0
        ret = v2;
        ret.insert(v1);
#else
        ret = v1;
        ret.unite(v2);
#endif
    } else if (op == TCriteria::Or) {
        QVariantList lst;
        lst << v1 << v2;
        ret.insert("$or", lst);
    } else {
        tSystemError("Logic error: [%s:%d]", __FILE__, __LINE__);
    }
    return ret;
}
