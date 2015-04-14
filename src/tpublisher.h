#ifndef TPUBLISHER_H
#define TPUBLISHER_H

#include <QString>
#include <QMap>
#include <TGlobal>

class TAbstractWebSocket;
class Pub;


class T_CORE_EXPORT TPublisher
{
public:
    void subscribe(const QString &topic, TAbstractWebSocket *socket);
    void unsubscribe(const QString &topic, TAbstractWebSocket *socket);
    void unsubscribeFromAll(TAbstractWebSocket *socket);
    bool publish(const QString &topic, const QString &text);
    bool publish(const QString &topic, const QByteArray &binary);
   static TPublisher *instance();

protected:
    Pub *create(const QString &topic);
    Pub *get(const QString &topic);
    void release(const QString &topic);
    static QObject *castToObject(TAbstractWebSocket *socket);

private:
    TPublisher();
    QMap<QString, Pub*> pubobj;

    Q_DISABLE_COPY(TPublisher)
};

#endif // TPUBLISHER_H
