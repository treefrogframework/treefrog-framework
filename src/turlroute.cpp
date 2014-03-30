/* Copyright (c) 2011-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QFile>
#include <QTextStream>
#include <TWebApplication>
#include <TSystemGlobal>
#include <THttpUtility>
#include "turlroute.h"
#include "troute.h"

#define DIRECT_VIEW_RENDER_MODE  "DirectViewRenderMode"


static TUrlRoute *urlRoute = 0;

static void cleanup()
{
    if (urlRoute) {
        delete urlRoute;
        urlRoute = 0;
    }
}


/*!
 * Initializes.
 * Call this in main thread.
 */
void TUrlRoute::instantiate()
{
    if (!urlRoute) {
        urlRoute = new TUrlRoute;
        urlRoute->parseConfigFile();

        //Add default route
        if (Tf::app()->appSettings().value(DIRECT_VIEW_RENDER_MODE).toBool())
            urlRoute->addRouteFromString("MATCH /:params 'directcontroller#show'");
        else
        {
            urlRoute->addRouteFromString("MATCH /:controller/:params '#index'");
            urlRoute->addRouteFromString("MATCH /:controller/:action/:params");
        }

        qAddPostRoutine(::cleanup);
    }
}


const TUrlRoute &TUrlRoute::instance()
{
    Q_CHECK_PTR(urlRoute);
    return *urlRoute;
}

bool TUrlRoute::addRouteFromString(QString line)
{
    QStringList items = line.split(' ', QString::SkipEmptyParts);

    if (items.count() >= 2) {
        // Trimm quotes
        QString method = items[0];
        QString route = THttpUtility::trimmedQuotes(items[1]);
        QString destination;

        if (items.count() >= 3)
            destination = THttpUtility::trimmedQuotes(items[2]);

        TRoute rt;

        rt.method = TRoute::methodFromString(method);

        if (rt.method == TRoute::Invalid)
        {
            tError("Invalid method, '%s'", qPrintable(items[0]));
            return false;
        }

        // parse controller and action
        QStringList list = destination.split('#');

        switch(list.count())
        {
            case 2:
                rt.action = list[1].toLatin1();
                //fallthrough
            case 1:
                if (!list[0].isEmpty()) rt.controller = list[0].toLower().toLatin1();
                break;
            default:
                tError("Invalid destination, '%s'", qPrintable(destination));
                return false;
        }

        rt.components = route.split('/');
        if (route.startsWith('/')) rt.components.takeFirst();
        if (route.endsWith('/')) rt.components.takeLast();

        if ((rt.controller.isEmpty() && (rt.components.indexOf(":controller")) < 0))
        {
            tError("Can only create a route without a destionation if it accepts the :controller and :action parameters! [%s]", qPrintable(line));
            return false;
        }

        if ((rt.action.isEmpty() && (rt.components.indexOf(":action")) < 0))
        {
            tError("Can only create a route without a default action if it accepts the :action parameter! [%s]", qPrintable(line));
            return false;
        }

        if (rt.components.indexOf(":params") >= 0)
        {
            if (rt.components.indexOf(":params") != rt.components.length() - 1)
            {
                tError("Invalid route: :params must be at the end! [%s]",qPrintable(route));
                return false;
            }
            else
            {
                rt.components.takeLast();
                rt.has_variable_params = 1;
            }
        }

        routes << rt;
        tSystemDebug("added route: method:%d components:%s ctrl:%s action:%s, params:%d",
                     rt.method, qPrintable(rt.components.join('/')), rt.controller.data(),
                     rt.action.data(), rt.has_variable_params);
        return true;
    } else {
        tError("Invalid directive, '%s'", qPrintable(line));
        return false;
    }
}


bool TUrlRoute::parseConfigFile()
{
    QFile routesFile(Tf::app()->routesConfigFilePath());
    if (!routesFile.open(QIODevice::ReadOnly)) {
        tSystemError("failed to read file : %s", qPrintable(routesFile.fileName()));
        return false;
    }

    int cnt = 0;
    QTextStream ts(&routesFile);
    while (!ts.atEnd()) {
        QString line = ts.readLine().simplified();
        ++cnt;

        if (!line.isEmpty() && !line.startsWith('#')) {
            if (!addRouteFromString(line))
            {
                tError("Error parsing route %s [line: %d]", qPrintable(line), cnt);
            }
        }
    }
    return true;
}


TRouting TUrlRoute::findRouting(Tf::HttpMethod method, const QString &path) const
{
    QStringList params;
    QStringList components = path.split('/');
    QString controller;
    QString action;
    components.takeFirst();
    components.takeLast();

    for (QListIterator<TRoute> i(routes); i.hasNext(); ) {
        const TRoute &rt = i.next();

        //Check if we have a good http verb
        switch(rt.method)
        {
            case TRoute::Match:
                //We match anything here
                break;
            case TRoute::Get:
                if (method != Tf::Get) continue;
                break;
            case TRoute::Post:
                if (method != Tf::Post) continue;
                break;
            case TRoute::Patch:
                if (method != Tf::Patch) continue;
                break;
            case TRoute::Put:
                if (method != Tf::Put) continue;
                break;
            case TRoute::Delete:
                if (method != Tf::Delete) continue;
                break;
            default:
                tSystemWarn("Unkown route method in findRouting: %d", rt.method);
                continue;
                break;
        }

        //To short?
        if (components.length() < rt.components.length()) continue;

        //Parse any parameters
        for(int j=0; j < rt.components.length(); j++)
        {
            if (rt.components[j] == components[j]) continue;

            if (rt.components[j] == ":param") {
                params << components[j];
                continue;
            }

            if (rt.components[j] == ":controller")
            {
                controller = components[j];
                continue;
            }

            if (rt.components[j] == ":action")
            {
                action = components[j];
                continue;
            }

            goto trynext;
        }

        if (controller.isEmpty()) controller = rt.controller;
        if (action.isEmpty()) action = rt.action;

        if (controller.isEmpty() || action.isEmpty())
            goto trynext;

        //Add any variable params
        if (rt.has_variable_params)
            params << components.mid(rt.components.length());

        return TRouting(controller, action, params);

trynext:
        continue;
    }

    return TRouting();  // Not found routing info
}
