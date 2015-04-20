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
    bool subscribe(const QObject *receiver);
    bool unsubscribe(const QObject *receiver);
    void publish(const QString &message);
    void publish(const QByteArray &binary);
    void publishLocal(const QString &message);
    void publishLocal(const QByteArray &binary);
    int subscriberCounter() const { return subscribers.count(); }
signals:
    void textPublished(const QString &);
    void binaryPublished(const QByteArray &);
private:
    QString topic;
    QSet<const void*> subscribers;
};
#include "tpublisher.moc"


bool Pub::subscribe(const QObject *receiver)
{
    tSystemDebug("Pub::subscribe");

    if (!receiver) {
        return false;
    }

    if (subscribers.contains(receiver))
        return true;

    connect(this, SIGNAL(textPublished(const QString&)),
            receiver, SLOT(sendText(const QString&)), Qt::QueuedConnection);
    connect(this, SIGNAL(binaryPublished(const QByteArray&)),
            receiver, SLOT(sendBinary(const QByteArray&)), Qt::QueuedConnection);

    subscribers.insert(receiver);
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


void Pub::publish(const QString &message)
{
    TSystemBus::instance()->send(Tf::WebSocketPublishText, topic, message.toUtf8());
    publishLocal(message);
}


void Pub::publish(const QByteArray &binary)
{
    TSystemBus::instance()->send(Tf::WebSocketPublishBinary, topic, binary);
    publishLocal(binary);
}


void Pub::publishLocal(const QString &message)
{
    emit textPublished(message);
}


void Pub::publishLocal(const QByteArray &binary)
{
    emit binaryPublished(binary);
}

/*!
  \class TPublisher
  \brief The TPublisher class provides a means of publish subscribe messaging.
*/

TPublisher::TPublisher()
    : pubobj()
{ }


void TPublisher::subscribe(const QString &topic, TAbstractWebSocket *socket)
{
    tSystemDebug("TPublisher::subscribe: %s", qPrintable(topic));
    QMutexLocker locker(&mutex);

    Pub *pub = get(topic);
    if (!pub) {
        pub = create(topic);
    }

    pub->subscribe(castToObject(socket));
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


bool TPublisher::publish(const QString &topic, const QString &text)
{
    QMutexLocker locker(&mutex);

    Pub *pub = get(topic);
    if (pub) {
        pub->publish(text);
    }
    return (bool)pub;
}


bool TPublisher::publish(const QString &topic, const QByteArray &binary)
{
    QMutexLocker locker(&mutex);

    Pub *pub = get(topic);
    if (pub) {
        pub->publish(binary);
    }
    return (bool)pub;
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
                pub->publishLocal(QString::fromUtf8(msg.data()));
            }
            break; }

        case Tf::WebSocketPublishBinary: {
            Pub *pub = get(msg.target());
            if (pub) {
                pub->publishLocal(msg.data());
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
