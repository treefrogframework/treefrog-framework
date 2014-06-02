#ifndef TURLROUTE_H
#define TURLROUTE_H

#include <QByteArray>
#include <QStringList>
#include <TGlobal>


class TRoute {
public:
    enum RouteDirective {
        Match   = 0,
        Get     = Tf::Get,
        Head    = Tf::Head,
        Post    = Tf::Post,
        Options = Tf::Options,
        Put     = Tf::Put,
        Delete  = Tf::Delete,
        Trace   = Tf::Trace,
        Connect = Tf::Connect,
        Patch   = Tf::Patch,
        Invalid = 0xff,
    };

    int     method;
    QStringList components;
    QList<int>  keywordIndexes;
    QByteArray controller;
    QByteArray action;
    bool    hasVariableParams;

    TRoute() : method(Invalid), hasVariableParams(false) { }
};


class TRouting {
public:
    bool empty;
    QByteArray controller;
    QByteArray action;
    QStringList params;

    TRouting();
    TRouting(const QByteArray &controller, const QByteArray &action, const QStringList &params = QStringList());
    bool isEmpty() const { return empty; }
    bool isDenied() const { return !empty && controller.isEmpty(); }
};


inline TRouting::TRouting()
    : empty(true) { }


inline TRouting::TRouting(const QByteArray &ctrl, const QByteArray &act, const QStringList &p)
    : empty(false), controller(ctrl), action(act), params(p) { }


class T_CORE_EXPORT TUrlRoute
{
public:
    static void instantiate();
    static const TUrlRoute &instance();
    TRouting findRouting(Tf::HttpMethod method, const QString &path) const;

protected:
    TUrlRoute() { }
    bool parseConfigFile();
    bool addRouteFromString(const QString &line);
    void clear();

private:
    QList<TRoute> routes;
};

#endif // TURLROUTE_H
