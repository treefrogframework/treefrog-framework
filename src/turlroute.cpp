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

    if (items.count() == 3) {
        // Trimm quotes
        QString method = items[0];
        QString route = THttpUtility::trimmedQuotes(items[1]);
        QString destination = THttpUtility::trimmedQuotes(items[2]);

        TRoute rt;

        rt.method = TRoute::methodFromString(method);

        if (rt.method == TRoute::Invalid)
        {
            tError("Invalid method, '%s'", qPrintable(items[0]));
            return false;
        }

        // parse controller and action
        QStringList list = destination.split('#');
        if (list.count() == 2) {
            rt.controller = list[0].toLower().toLatin1() + "controller";
            rt.action = list[1].toLatin1();
        } else {
            tError("Invalid destination, '%s'", qPrintable(destination));
            return false;
        }

        rt.components = route.split('/');
        if (route.startsWith('/')) rt.components.takeFirst();
        if (route.endsWith('/')) rt.components.takeLast();

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

            goto trynext;
        }

        //Add any variable params
        if (rt.has_variable_params)
            params << components.mid(rt.components.length());

        return TRouting(rt.controller, rt.action, params);

trynext:
        continue;
    }

    return TRouting();  // Not found routing info
}
