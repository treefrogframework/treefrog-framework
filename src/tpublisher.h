#ifndef TPUBLISHER_H
#define TPUBLISHER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <TGlobal>

class TAbstractWebSocket;
class Pub;


class T_CORE_EXPORT TPublisher : public QObject
{
    Q_OBJECT
public:
    void subscribe(const QString &topic, TAbstractWebSocket *socket);
    void unsubscribe(const QString &topic, TAbstractWebSocket *socket);
    void unsubscribeFromAll(TAbstractWebSocket *socket);
    void publish(const QString &topic, const QString &text);
    void publish(const QString &topic, const QByteArray &binary);
    static TPublisher *instance();
    static void instantiate();

protected:
    Pub *create(const QString &topic);
    Pub *get(const QString &topic);
    void release(const QString &topic);
    static QObject *castToObject(TAbstractWebSocket *socket);

protected slots:
    void receiveSystemBus();

private:
    TPublisher();
    QMap<QString, Pub*> pubobj;

    Q_DISABLE_COPY(TPublisher)
};

#endif // TPUBLISHER_H
