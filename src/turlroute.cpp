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
            QStringList items = line.split(' ');
            if (items.count() == 3) {
                // Trimm quotes
                items[1] = THttpUtility::trimmedQuotes(items[1]);
                items[2] = THttpUtility::trimmedQuotes(items[2]);

                TRoute rt;

                rt.method = TRoute::methodFromString(items[0]);

                if (rt.method == TRoute::Invalid)
                {
                    tError("Invalid method, '%s'  [line : %d]", qPrintable(items[0]), cnt);
                    continue;
                }

                // parse path
                if (items[1].endsWith(":params")) {
                    rt.params = true;
                    rt.path = items[1].left(items[1].length() - 7);
                } else {
                    rt.params = false;
                    rt.path = items[1];
                }

                // parse controller and action
                QStringList list = items[2].split('#');
                if (list.count() == 2) {
                    rt.controller = list[0].toLower().toLatin1() + "controller";
                    rt.action = list[1].toLatin1();
                } else {
                    tError("Invalid action, '%s'  [line : %d]", qPrintable(items[2]), cnt);
                    continue;
                }

                if ((!rt.params) && (!rt.path.endsWith('/')))
                    rt.path += QLatin1Char('/');

                routes << rt;
                tSystemDebug("route: method:%d path:%s ctrl:%s action:%s params:%d",
                             rt.method, qPrintable(rt.path), rt.controller.data(),
                             rt.action.data(), rt.params);
            } else {
                tError("Invalid directive, '%s'  [line : %d]", qPrintable(line), cnt);
            }
        }
    }
    return true;
}


TRouting TUrlRoute::findRouting(Tf::HttpMethod method, const QString &path) const
{
    QStringList params;

    for (QListIterator<TRoute> i(routes); i.hasNext(); ) {
        const TRoute &rt = i.next();

        //If our target is not starting with the same string, there cannot be a match
        if (!path.startsWith(rt.path)) continue;

        //If we don't have the params set, we need to have an exact match.
        if (!rt.params && (path != rt.path)) continue;

        //Parse parameters
        if (rt.params)
        {
            int len = rt.path.length();
            QString paramstr = path.mid(len);
            params = paramstr.split('/', QString::SkipEmptyParts);
        }

        //Check if we have a good http verb
        switch(rt.method)
        {
            case TRoute::Match:
                return TRouting(rt.controller, rt.action, params);

            case TRoute::Get:
                if (method == Tf::Get) return TRouting(rt.controller, rt.action, params);
                continue;
            case TRoute::Post:
                if (method == Tf::Post) return TRouting(rt.controller, rt.action, params);
                continue;
            default:
                tSystemWarn("Unkown route method in findRouting: %d", rt.method);
                continue;
        }
    }

    return TRouting();  // Not found routing info
}
