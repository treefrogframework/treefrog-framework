/* Copyright (c) 2016-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsystemglobal.h"
#include <TJSLoader>
#include <TReactComponent>

//#define tSystemError(fmt, ...)  printf(fmt "\n", ## __VA_ARGS__)
//#define tSystemDebug(fmt, ...)  printf(fmt "\n", ## __VA_ARGS__)


TReactComponent::TReactComponent(const QString &moduleName, const QStringList &searchPaths) :
    jsLoader(new TJSLoader(moduleName, TJSLoader::Jsx)), loadedTime()
{
    QStringList paths = searchPaths;
    paths << TJSLoader::defaultSearchPaths();
    jsLoader->setSearchPaths(searchPaths);
    jsLoader->import("React", "react-with-addons");
    jsLoader->import("ReactDOMServer", "react-dom-server");
}


void TReactComponent::import(const QString &moduleName)
{
    jsLoader->import(moduleName);
}


void TReactComponent::import(const QString &defaultMember, const QString &moduleName)
{
    jsLoader->import(defaultMember, moduleName);
}


QString TReactComponent::renderToString(const QString &component)
{
    auto *context = jsLoader->load();

    if (loadedTime.isNull()) {
        loadedTime = QDateTime::currentDateTime();
    } else {
        if (context) {
            QFileInfo fi(context->modulePath());
            if (context->modulePath().isEmpty() || (fi.exists() && fi.lastModified() > loadedTime)) {
                context = jsLoader->load(true);
                loadedTime = QDateTime::currentDateTime();
            }
        }
    }

    if (!context) {
        return QString();
    }

    QString func = QLatin1String("ReactDOMServer.renderToString(") + TJSLoader::compileJsx(component) + QLatin1String(");");
    tSystemDebug("TReactComponent func: %s", qUtf8Printable(func));
    return context->evaluate(func).toString();
}
