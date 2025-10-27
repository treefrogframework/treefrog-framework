#pragma once
#include <QByteArray>
#include <QStringList>
#include <TGlobal>


class TRoute {
public:
    enum class RouteDirective {
        Match = 0,
        Get = (int)Tf::HttpMethod::Get,
        Head = (int)Tf::HttpMethod::Head,
        Post = (int)Tf::HttpMethod::Post,
        Options = (int)Tf::HttpMethod::Options,
        Put = (int)Tf::HttpMethod::Put,
        Delete = (int)Tf::HttpMethod::Delete,
        Trace = (int)Tf::HttpMethod::Trace,
        Connect = (int)Tf::HttpMethod::Connect,
        Patch = (int)Tf::HttpMethod::Patch,
        Invalid = 0xff,
    };

    RouteDirective method {RouteDirective::Invalid};
    QStringList componentList;
    QList<int> keywordIndexes;
    QByteArray controller;
    QByteArray action;
    int paramNum {0};
    bool hasVariableParams {false};
};


class TRouting {
public:
    bool exists {false};
    QByteArray controller;
    QByteArray action;
    QStringList params;

    TRouting() { }
    TRouting(const QByteArray &controller, const QByteArray &action, const QStringList &params = QStringList());

    void setRouting(const QByteArray &controller, const QByteArray &action, const QStringList &params = QStringList());
};


inline TRouting::TRouting(const QByteArray &ctrl, const QByteArray &act, const QStringList &p) :
    controller(ctrl),
    action(act),
    params(p)
{
}


inline void TRouting::setRouting(const QByteArray &ctrl, const QByteArray &act, const QStringList &p)
{
    controller = ctrl;
    action = act;
    params = p;
}


class T_CORE_EXPORT TUrlRoute {
public:
    static const TUrlRoute &instance();
    static QStringList splitPath(const QString &path);
    TRouting findRouting(Tf::HttpMethod method, const QStringList &components) const;
    QString findUrl(const QString &controller, const QString &action, const QStringList &params = QStringList()) const;
    QList<TRoute> allRoutes() const { return _routes; }

protected:
    TUrlRoute() { }
    bool parseConfigFile();
    bool addRouteFromString(const QString &line);
    void clear();

private:
    QList<TRoute> _routes;
};
