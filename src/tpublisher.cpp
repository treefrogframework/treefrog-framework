/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QMutex>
#include <QSet>
#include <TWebApplication>
#include "tsystemglobal.h"
#include "tpublisher.h"
#include "twebsocket.h"
#include "tsystembus.h"
#ifdef Q_OS_LINUX
# include "tepollwebsocket.h"
#endif

static QMutex mutex(QMutex::NonRecursive);
static TPublisher *globalInstance = nullptr;


class Pub : public QObject
{
    Q_OBJECT
public:
    Pub(const QString &t) : topic(t), subscribers() { }
    bool subscribe(const QObject *receiver, bool local);
    bool unsubscribe(const QObject *receiver);
    void publish(const QString &message, const QObject *sender);
    void publish(const QByteArray &binary, const QObject *sender);
    int subscriberCounter() const { return subscribers.count(); }
signals:
    void textPublished(const QString &, const QObject *sender);
    void binaryPublished(const QByteArray &, const QObject *sender);
private:
    QString topic;
    QMap<const QObject*, bool> subscribers;
};
#include "tpublisher.moc"


bool Pub::subscribe(const QObject *receiver, bool local)
{
    tSystemDebug("Pub::subscribe");

    if (!receiver) {
        return false;
    }

    if (subscribers.contains(receiver)) {
        subscribers[receiver] = local;
        return true;
    }

    connect(this, SIGNAL(textPublished(const QString&, const QObject*)),
            receiver, SLOT(sendTextForPublish(const QString&, const QObject*)), Qt::QueuedConnection);
    connect(this, SIGNAL(binaryPublished(const QByteArray&, const QObject*)),
            receiver, SLOT(sendBinaryForPublish(const QByteArray&, const QObject*)), Qt::QueuedConnection);

    subscribers.insert(receiver, local);
    tSystemDebug("subscriber counter: %d", subscriberCounter());
    return true;
}


bool Pub::unsubscribe(const QObject *receiver)
{
    tSystemDebug("Pub::unsubscribe");

    if (!receiver) {
        return false;
    }

    disconnect(this, nullptr, receiver, nullptr);
    subscribers.remove(receiver);
    tSystemDebug("subscriber counter: %d", subscriberCounter());
    return true;
}


void Pub::publish(const QString &message, const QObject *sender)
{
    const QObject *except = nullptr;
    bool local = subscribers.value(sender, true);
    if (!local) {
        except = sender;
    }
    emit textPublished(message, except);
}


void Pub::publish(const QByteArray &binary, const QObject *sender)
{
    const QObject *except = nullptr;
    bool local = subscribers.value(sender, true);
    if (!local) {
        except = sender;
    }
    emit binaryPublished(binary, except);
}


/*!
  \class TPublisher
  \brief The TPublisher class provides a means of publish subscribe messaging.
*/

TPublisher::TPublisher()
    : pubobj()
{ }


void TPublisher::subscribe(const QString &topic, bool local, TAbstractWebSocket *socket)
{
    tSystemDebug("TPublisher::subscribe: %s", qPrintable(topic));
    QMutexLocker locker(&mutex);

    Pub *pub = get(topic);
    if (!pub) {
        pub = create(topic);
    }

    pub->subscribe(castToObject(socket), local);
}


void TPublisher::unsubscribe(const QString &topic, TAbstractWebSocket *socket)
{
    tSystemDebug("TPublisher::unsubscribe: %s", qPrintable(topic));
    QMutexLocker locker(&mutex);

    Pub *pub = get(topic);
    if (pub) {
        pub->unsubscribe(castToObject(socket));
        if (pub->subscriberCounter() == 0) {
            release(topic);
        }
    }
}


void TPublisher::unsubscribeFromAll(TAbstractWebSocket *socket)
{
    tSystemDebug("TPublisher::unsubscribeFromAll");
    QMutexLocker locker(&mutex);

    for (QMutableMapIterator<QString, Pub*> it(pubobj); it.hasNext(); ) {
        it.next();
        Pub *pub = it.value();
        pub->unsubscribe(castToObject(socket));

        if (pub->subscriberCounter() == 0) {
            tSystemDebug("release topic: %s", qPrintable(it.key()));
            it.remove();
            delete pub;
        }
    }

    tSystemDebug("total topics: %d", pubobj.count());
}


QObject *TPublisher::castToObject(TAbstractWebSocket *socket)
{
    QObject *obj = nullptr;

    switch ( Tf::app()->multiProcessingModule() ) {
    case TWebApplication::Thread:
        obj = dynamic_cast<TWebSocket*>(socket);
        break;

    case TWebApplication::Hybrid:
#ifdef Q_OS_LINUX
        obj = dynamic_cast<TEpollWebSocket*>(socket);
#else
        tFatal("Unsupported MPM: hybrid");
#endif
        break;

    default:
        tFatal("Unsupported MPM: hybrid");
        break;
    }
    return obj;
}


void TPublisher::publish(const QString &topic, const QString &text, TAbstractWebSocket *socket)
{
    if (Tf::app()->maxNumberOfAppServers() > 1) {
        TSystemBus::instance()->send(Tf::WebSocketPublishText, topic, text.toUtf8());
    }

    QMutexLocker locker(&mutex);
    Pub *pub = get(topic);
    if (pub) {
        pub->publish(text, castToObject(socket));
    }
}


void TPublisher::publish(const QString &topic, const QByteArray &binary, TAbstractWebSocket *socket)
{
    if (Tf::app()->maxNumberOfAppServers() > 1) {
        TSystemBus::instance()->send(Tf::WebSocketPublishBinary, topic, binary);
    }

    QMutexLocker locker(&mutex);
    Pub *pub = get(topic);
    if (pub) {
        pub->publish(binary, castToObject(socket));
    }
}


TPublisher *TPublisher::instance()
{
    return globalInstance;
}


void TPublisher::instantiate()
{
    if (!globalInstance) {
        globalInstance = new TPublisher;
        connect(TSystemBus::instance(), SIGNAL(readyReceive()), globalInstance, SLOT(receiveSystemBus()));
    }
}


void TPublisher::receiveSystemBus()
{
    auto messages = TSystemBus::instance()->recvAll();

    for (auto &msg : messages) {
        switch (msg.opCode()) {
        case Tf::WebSocketSendText:
            break;

        case Tf::WebSocketSendBinary:
            break;

        case Tf::WebSocketPublishText: {
            Pub *pub = get(msg.target());
            if (pub) {
                pub->publish(QString::fromUtf8(msg.data()), nullptr);
            }
            break; }

        case Tf::WebSocketPublishBinary: {
            Pub *pub = get(msg.target());
            if (pub) {
                pub->publish(msg.data(), nullptr);
            }
            break; }

        default:
            tSystemError("Internal Error  [%s:%d]", __FILE__, __LINE__);
            break;
        }
    }
}


Pub *TPublisher::create(const QString &topic)
{
    auto *pub = new Pub(topic);
    pub->moveToThread(Tf::app()->thread());
    pubobj.insert(topic, pub);
    tSystemDebug("create topic: %s", qPrintable(topic));
    return pub;
}


Pub *TPublisher::get(const QString &topic)
{
    return pubobj.value(topic);
}


void TPublisher::release(const QString &topic)
{
    Pub *pub = pubobj.take(topic);
    if (pub) {
        delete pub;
        tSystemDebug("release topic: %s  (total topics:%d)", qPrintable(topic), pubobj.count());
    }
}
