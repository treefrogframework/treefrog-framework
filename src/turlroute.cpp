/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "turlroute.h"
#include <QFile>
#include <QMap>
#include <QRegularExpression>
#include <QTextStream>
#include <THttpUtility>
#include <TSystemGlobal>
#include <TWebApplication>


class RouteDirectiveHash : public QMap<QString, int> {
public:
    RouteDirectiveHash() :
        QMap<QString, int>()
    {
        insert("match", TRoute::Match);
        insert("get", TRoute::Get);
        insert("post", TRoute::Post);
        insert("put", TRoute::Put);
        insert("patch", TRoute::Patch);
        insert("delete", TRoute::Delete);
        insert("trace", TRoute::Trace);
        insert("connect", TRoute::Connect);
        insert("patch", TRoute::Patch);
    }
};
Q_GLOBAL_STATIC(RouteDirectiveHash, directiveHash)


const TUrlRoute &TUrlRoute::instance()
{
    static TUrlRoute *urlRoute = []() {
        auto *route = new TUrlRoute();
        route->parseConfigFile();
        return route;
    }();
    return *urlRoute;
}


bool TUrlRoute::parseConfigFile()
{
    QFile routesFile(Tf::app()->routesConfigFilePath());
    if (!routesFile.open(QIODevice::ReadOnly)) {
        tSystemError("failed to read file : %s", qUtf8Printable(routesFile.fileName()));
        return false;
    }

    int cnt = 0;
    QTextStream ts(&routesFile);
    while (!ts.atEnd()) {
        QString line = ts.readLine().simplified();
        ++cnt;

        if (!line.isEmpty() && !line.startsWith('#')) {
            if (!addRouteFromString(line)) {
                tError("Error parsing route %s [line: %d]", qUtf8Printable(line), cnt);
            }
        }
    }
    return true;
}


bool TUrlRoute::addRouteFromString(const QString &line)
{
    QStringList items = line.simplified().split(' ');
    if (items.count() != 3) {
        tError("Invalid directive, '%s'", qUtf8Printable(line));
        return false;
    }

    // Trimm quotes
    items[1] = THttpUtility::trimmedQuotes(items[1]);
    items[2] = THttpUtility::trimmedQuotes(items[2]);
    const QString &path = items[1];

    if (path.contains(":params") && !path.endsWith(":params")) {
        tError(":params must be specified as last directive.");
        return false;
    }

    TRoute rt;

    // Check method
    rt.method = directiveHash()->value(items[0].toLower(), TRoute::Invalid);
    if (rt.method == TRoute::Invalid) {
        tError("Invalid directive, '%s'", qUtf8Printable(items[0]));
        return false;
    }

    // parse path
    rt.componentList = splitPath(path);
    rt.paramNum = rt.componentList.count(":param");
    rt.hasVariableParams = rt.componentList.contains(":params");

    for (int i = 0; i < rt.componentList.count(); ++i) {
        const QString &c = rt.componentList[i];
        if (c.startsWith(":")) {
            if (c != ":param" && c != ":params") {
                return false;
            }
        } else {
            rt.keywordIndexes << i;
        }
    }

    if (items[2].startsWith("/")) {
        // static file
        rt.controller = items[2].toUtf8();
    } else {
        // parse controller and action
        QStringList list = items[2].split(QRegularExpression("[#\\.]"));
        if (list.count() == 2) {
            rt.controller = list[0].toLower().toLatin1() + "controller";
            rt.action = list[1].toLatin1();
        } else {
            tError("Invalid action, '%s'", qUtf8Printable(items[2]));
            return false;
        }
    }

    _routes << rt;
    tSystemDebug("route: method:%d path:%s  ctrl:%s action:%s params:%d",
        rt.method, qUtf8Printable(QLatin1String("/") + rt.componentList.join("/")), rt.controller.data(),
        rt.action.data(), rt.hasVariableParams);
    return true;
}


TRouting TUrlRoute::findRouting(Tf::HttpMethod method, const QStringList &components) const
{
    if (_routes.isEmpty()) {
        return TRouting();
    }

    for (const auto &rt : _routes) {
        // Too long or short?
        if (rt.hasVariableParams) {
            if (components.length() < rt.componentList.length() - 1) {
                continue;
            }
        } else {
            if (components.length() != rt.componentList.length()) {
                continue;
            }
        }

        for (int idx : (const QList<int> &)rt.keywordIndexes) {
            if (components.value(idx) != rt.componentList[idx]) {
                goto continue_next;
            }
        }

        if (rt.method == TRoute::Match || rt.method == method) {
            // Generates parameters for action
            QStringList params = components;

            if (params.count() == 1 && params[0].isEmpty()) {  // means path="/"
                params.clear();
            } else {
                // Erases non-parameters
                QListIterator<int> it(rt.keywordIndexes);
                it.toBack();
                while (it.hasPrevious()) {
                    int idx = it.previous();
                    params.removeAt(idx);
                }
            }

            TRouting routing(rt.controller, rt.action, params);
            routing.exists = true;
            return routing;
        }
    continue_next:
        continue;
    }

    return TRouting() /* Not found routing info */;
}


static QString generatePath(const QStringList &components, const QStringList &params)
{
    QString ret = QLatin1String("/");
    int cnt = 0;
    for (auto &c : components) {
        if (c == QLatin1String(":param")) {
            ret += params.value(cnt++);
        } else if (c == QLatin1String(":params")) {
            ret += QStringList(params.mid(cnt)).join("/") + QLatin1Char('/');
            break;
        } else {
            ret += c;
        }
        ret += QLatin1Char('/');
    }

    if (ret.length() > 1) {
        ret.chop(1);
    }
    return ret;
}


QString TUrlRoute::findUrl(const QString &controller, const QString &action, const QStringList &params) const
{
    if (_routes.isEmpty()) {
        return QString();
    }

    for (const auto &rt : _routes) {
        const QByteArray ctrl = controller.toLower().toLatin1() + "controller";
        const QByteArray act = action.toLower().toLatin1();

        if (rt.controller == ctrl && rt.action == act) {
            if ((rt.paramNum == params.count() && !rt.hasVariableParams)
                || (rt.paramNum <= params.count() && rt.hasVariableParams)) {
                return generatePath(rt.componentList, params);
            }
        }
    }

    return QString();
}


void TUrlRoute::clear()
{
    _routes.clear();
}


QStringList TUrlRoute::splitPath(const QString &path)
{
    const QLatin1Char Slash('/');

    int s = (path.startsWith(Slash)) ? 1 : 0;
    int len = path.length();

    if (len > 1 && path.endsWith(Slash)) {
        --len;
    }
    return path.mid(s, len - s).split(Slash);
}
