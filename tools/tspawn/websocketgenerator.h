#ifndef WEBSOCKETGENERATOR_H
#define WEBSOCKETGENERATOR_H

#include <QtCore>


class WebSocketGenerator
{
public:
    WebSocketGenerator(const QString &name);
    bool generate(const QString &dst) const;

private:
    QString name;
};

#endif // WEBSOCKETGENERATOR_H

