#pragma once
#include <QtCore>


class WebSocketGenerator {
public:
    WebSocketGenerator(const QString &name);
    bool generate(const QString &dst) const;

private:
    QString name;
};

