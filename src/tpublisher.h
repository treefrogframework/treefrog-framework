#pragma once
#include <QMap>
#include <QObject>
#include <QString>
#include <TGlobal>

class TAbstractWebSocket;
class Pub;


class T_CORE_EXPORT TPublisher : public QObject {
    Q_OBJECT
public:
    void subscribe(const QString &topic, bool local, TAbstractWebSocket *socket);
    void unsubscribe(const QString &topic, TAbstractWebSocket *socket);
    void unsubscribeFromAll(TAbstractWebSocket *socket);
    void publish(const QString &topic, const QString &text, TAbstractWebSocket *socket);
    void publish(const QString &topic, const QByteArray &binary, TAbstractWebSocket *socket);
    static TPublisher *instance();

protected:
    Pub *create(const QString &topic);
    Pub *get(const QString &topic);
    void release(const QString &topic);
    static QObject *castToObject(TAbstractWebSocket *socket);

protected slots:
    void receiveSystemBus();

private:
    TPublisher();
    QMap<QString, Pub *> pubobj;

    T_DISABLE_COPY(TPublisher)
    T_DISABLE_MOVE(TPublisher)
};

