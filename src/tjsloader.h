#ifndef TJSLOADER_H
#define TJSLOADER_H

#include <QJSValue>
#include <TGlobal>
#include <TJSContext>
#include <TJSInstance>


class T_CORE_EXPORT TJSLoader
{
public:
    TJSLoader();

    TJSContext *loadJSModule(const QString &moduleName);
    TJSContext *loadJSModule(const QString &defaultMember, const QString &moduleName);
    TJSInstance loadJSModuleAsConstructor(const QString &moduleName, const QJSValue &arg);
    TJSInstance loadJSModuleAsConstructor(const QString &moduleName, const QJSValueList &args = QJSValueList());
};

#endif // TJSLOADER_H
