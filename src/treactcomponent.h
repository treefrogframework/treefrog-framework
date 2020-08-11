#pragma once
#include <QDateTime>
#include <QStringList>
#include <TGlobal>

class TJSLoader;


class T_CORE_EXPORT TReactComponent {
public:
    TReactComponent(const QString &moduleName, const QStringList &searchPaths = QStringList());
    virtual ~TReactComponent() { }

    void import(const QString &moduleName);
    void import(const QString &defaultMember, const QString &moduleName);
    QString renderToString(const QString &component);
    QDateTime loadedDateTime() const { return loadedTime; }

private:
    TJSLoader *jsLoader;
    QDateTime loadedTime;

    T_DISABLE_COPY(TReactComponent)
    T_DISABLE_MOVE(TReactComponent)
};

