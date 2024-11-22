#pragma once
#include <QDir>
#include <QJSValue>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>
#include <TGlobal>

class QJSEngine;
class TJSInstance;


class T_CORE_EXPORT TJSModule : public QObject {
public:
    TJSModule(QObject *parent = nullptr);
    virtual ~TJSModule();

    QJSValue evaluate(const QString &program, const QString &fileName = QString(), int lineNumber = 1);
    QJSValue call(const QString &func, const QJSValue &arg);
    QJSValue call(const QString &func, const QJSValueList &args = QJSValueList());
    TJSInstance callAsConstructor(const QString &constructorName, const QJSValue &arg);
    TJSInstance callAsConstructor(const QString &constructorName, const QJSValueList &args = QJSValueList());
    QString modulePath() const { return _modulePath; }

    QJSValue import(const QString &moduleName);
    QJSValue import(const QString &defaultMember, const QString &moduleName);

private:
    QJSEngine *_jsEngine;
    QMap<QString, QString> _loadedFiles;
    QJSValue *_funcObj;
    QString _lastFunc;
    QString _modulePath;
    QRecursiveMutex _mutex;

    T_DISABLE_COPY(TJSModule)
    T_DISABLE_MOVE(TJSModule);

    friend class TJSLoader;
    friend class TReactComponent;
};
