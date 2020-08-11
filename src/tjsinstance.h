#pragma once
#include <QJSValue>
#include <QMutex>
#include <TGlobal>


class T_CORE_EXPORT TJSInstance : public QJSValue {
public:
    TJSInstance();
    TJSInstance(const TJSInstance &other);
    TJSInstance(const QJSValue &other);
    ~TJSInstance();

    QJSValue call(const QString &method, const QJSValue &arg);
    QJSValue call(const QString &method, const QJSValueList &args = QJSValueList());
};

