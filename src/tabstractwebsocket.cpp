/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tabstractwebsocket.h"
#include "tdispatcher.h"
#include "turlroute.h"
#include "twebsocket.h"
#include "twebsocketendpoint.h"
#include "twebsocketframe.h"
#include <QCryptographicHash>
#include <QDataStream>
#include <QObject>
#include <THttpRequestHeader>
#include <THttpUtility>
#include <TWebApplication>
#ifdef Q_OS_LINUX
#include "tepollwebsocket.h"
#endif

const QByteArray saltToken = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";


TAbstractWebSocket::TAbstractWebSocket(const THttpRequestHeader &header) :
    reqHeader(header),
    mutexData(),
    sessionStore()
{
}


TAbstractWebSocket::~TAbstractWebSocket()
{
    if (!closing.load()) {
        tSystemWarn("Logic warning  [%s:%d]", __FILE__, __LINE__);
    }

    delete keepAliveTimer;
}


void TAbstractWebSocket::sendText(const QString &message)
{
    TWebSocketFrame frame;
    frame.setOpCode(TWebSocketFrame::TextFrame);
    frame.setPayload(message.toUtf8());
    writeRawData(frame.toByteArray());

    renewKeepAlive();  // Renew Keep-Alive interval
}


void TAbstractWebSocket::sendBinary(const QByteArray &data)
{
    TWebSocketFrame frame;
    frame.setOpCode(TWebSocketFrame::BinaryFrame);
    frame.setPayload(data);
    writeRawData(frame.toByteArray());

    renewKeepAlive();  // Renew Keep-Alive interval
}


void TAbstractWebSocket::sendPing(const QByteArray &data)
{
    TWebSocketFrame frame;
    frame.setOpCode(TWebSocketFrame::Ping);
    frame.setPayload(data);
    writeRawData(frame.toByteArray());
}


void TAbstractWebSocket::sendPong(const QByteArray &data)
{
    TWebSocketFrame frame;
    frame.setOpCode(TWebSocketFrame::Pong);
    frame.setPayload(data);
    writeRawData(frame.toByteArray());
}


void TAbstractWebSocket::sendClose(int code)
{
    if (!closeSent.exchange(true)) {
        TWebSocketFrame frame;
        frame.setOpCode(TWebSocketFrame::Close);
        QDataStream ds(&frame.payload(), QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::BigEndian);
        ds << (qint16)code;
        writeRawData(frame.toByteArray());

        stopKeepAlive();
    }
}


void TAbstractWebSocket::startKeepAlive(int interval)
{
    tSystemDebug("startKeepAlive");
    QMutexLocker locker(&mutexData);

    if (!keepAliveTimer) {
        keepAliveTimer = new TBasicTimer();
        keepAliveTimer->moveToThread(Tf::app()->thread());
        keepAliveTimer->setReceiver(thisObject());
    }

    keepAliveTimer->setInterval(interval * 1000);
    QMetaObject::invokeMethod(keepAliveTimer, "start", Qt::QueuedConnection);
}


void TAbstractWebSocket::stopKeepAlive()
{
    tSystemDebug("stopKeepAlive");
    QMutexLocker locker(&mutexData);

    if (keepAliveTimer) {
        QMetaObject::invokeMethod(keepAliveTimer, "stop", Qt::QueuedConnection);
    }
}


void TAbstractWebSocket::renewKeepAlive()
{
    tSystemDebug("renewKeepAlive");
    QMutexLocker locker(&mutexData);

    if (keepAliveTimer) {
        QMetaObject::invokeMethod(keepAliveTimer, "start", Qt::QueuedConnection);
    }
}


TWebSocketSession TAbstractWebSocket::session() const
{
    QMutexLocker locker(&mutexData);
    TWebSocketSession ret = sessionStore;
    return ret;
}


void TAbstractWebSocket::setSession(const TWebSocketSession &session)
{
    QMutexLocker locker(&mutexData);
    sessionStore = session;
}


bool TAbstractWebSocket::searchEndpoint(const THttpRequestHeader &header)
{
    QString name = TUrlRoute::splitPath(header.path()).value(0).toLower();

    if (TWebSocketEndpoint::disabledEndpoints().contains(name)) {
        return false;
    }

    QString es = name + QLatin1String("endpoint");
    TDispatcher<TWebSocketEndpoint> dispatcher(es);
    TWebSocketEndpoint *endpoint = dispatcher.object();
    return endpoint;
}


int TAbstractWebSocket::parse(QByteArray &recvData)
{
    tSystemDebug("parse enter  data len:%lld  sid:%d", recvData.length(), socketId());
    if (websocketFrames().isEmpty()) {
        websocketFrames().append(TWebSocketFrame());
    } else {
        const TWebSocketFrame &f = websocketFrames().last();
        if (f.state() == TWebSocketFrame::Completed) {
            websocketFrames().append(TWebSocketFrame());
        }
    }

    TWebSocketFrame *pfrm = &websocketFrames().last();
    quint8 b;
    quint16 w;
    quint32 n;
    quint64 d;

    QDataStream ds(recvData);
    ds.setByteOrder(QDataStream::BigEndian);
    QIODevice *dev = ds.device();
    QByteArray hdr;

    while (!ds.atEnd()) {
        switch (pfrm->state()) {
        case TWebSocketFrame::Empty: {
            hdr = dev->peek(14);
            QDataStream dshdr(hdr);
            dshdr.setByteOrder(QDataStream::BigEndian);
            QIODevice *devhdr = dshdr.device();

            if (Q_UNLIKELY(devhdr->bytesAvailable() < 2)) {
                goto parse_end;
            }

            dshdr >> b;
            pfrm->setFirstByte(b);
            dshdr >> b;
            bool maskFlag = b & 0x80;
            quint8 len = b & 0x7f;

            // payload length
            switch (len) {
            case 126:
                if (Q_UNLIKELY(devhdr->bytesAvailable() < (int)sizeof(w))) {
                    goto parse_end;
                }
                dshdr >> w;
                if (Q_UNLIKELY(w < 126)) {
                    tSystemError("WebSocket protocol error  [%s:%d]", __FILE__, __LINE__);
                    return -1;
                }
                pfrm->setPayloadLength(w);
                break;

            case 127:
                if (Q_UNLIKELY(devhdr->bytesAvailable() < (int)sizeof(d))) {
                    goto parse_end;
                }
                dshdr >> d;
                if (Q_UNLIKELY(d <= 0xFFFF)) {
                    tSystemError("WebSocket protocol error  [%s:%d]", __FILE__, __LINE__);
                    return -1;
                }
                pfrm->setPayloadLength(d);
                break;

            default:
                pfrm->setPayloadLength(len);
                break;
            }

            // Mask key
            if (maskFlag) {
                if (Q_UNLIKELY(devhdr->bytesAvailable() < (int)sizeof(n))) {
                    goto parse_end;
                }
                dshdr >> n;
                pfrm->setMaskKey(n);
            }

            if (pfrm->payloadLength() == 0) {
                pfrm->setState(TWebSocketFrame::Completed);
            } else {
                pfrm->setState(TWebSocketFrame::HeaderParsed);
                if (pfrm->payloadLength() >= 2 * 1024 * 1024 * 1024ULL) {
                    tSystemError("Too big frame  [%s:%d]", __FILE__, __LINE__);
                    pfrm->clear();
                } else {
                    pfrm->payload().reserve(pfrm->payloadLength());
                }
            }

            tSystemDebug("WebSocket parse header pos: %lld", devhdr->pos());
            tSystemDebug("WebSocket payload length:%lld", pfrm->payloadLength());

            int hdrlen = hdr.length() - devhdr->bytesAvailable();
            ds.skipRawData(hdrlen);  // Forwards the pos
            break;
        }

        case TWebSocketFrame::HeaderParsed:  // fall through
        case TWebSocketFrame::MoreData: {
            tSystemDebug("WebSocket reading payload:  available length:%lld", dev->bytesAvailable());
            tSystemDebug("WebSocket parsing  length to read:%llu  current buf len:%lld", pfrm->payloadLength(), pfrm->payload().size());
            quint64 size = qMin((pfrm->payloadLength() - pfrm->payload().size()), (quint64)dev->bytesAvailable());
            if (Q_UNLIKELY(size == 0)) {
                Q_ASSERT(0);
                break;
            }

            char *p = pfrm->payload().data() + pfrm->payload().size();
            size = ds.readRawData(p, size);

            if (pfrm->maskKey()) {
                // Unmask
                const quint8 mask[4] = {quint8((pfrm->maskKey() & 0xFF000000) >> 24),
                    quint8((pfrm->maskKey() & 0x00FF0000) >> 16),
                    quint8((pfrm->maskKey() & 0x0000FF00) >> 8),
                    quint8((pfrm->maskKey() & 0x000000FF))};

                int i = pfrm->payload().size();
                const char *end = p + size;
                while (p < end) {
                    *p++ ^= mask[i++ % 4];
                }
            }
            pfrm->payload().resize(pfrm->payload().size() + size);
            tSystemDebug("WebSocket payload curent buf len: %lld", pfrm->payload().length());

            if ((quint64)pfrm->payload().size() == pfrm->payloadLength()) {
                pfrm->setState(TWebSocketFrame::Completed);
                tSystemDebug("Parse Completed   payload len: %lld", pfrm->payload().size());
            } else {
                pfrm->setState(TWebSocketFrame::MoreData);
                tSystemDebug("Parse MoreData   payload len: %lld", pfrm->payload().size());
            }
            break;
        }

        case TWebSocketFrame::Completed:  // fall through
        default:
            Q_ASSERT(0);
            break;
        }

        if (pfrm->state() == TWebSocketFrame::Completed) {
            if (Q_UNLIKELY(!pfrm->validate())) {
                pfrm->clear();
                continue;
            }

            // Fragmented message validation
            if (pfrm->opCode() == TWebSocketFrame::Continuation) {
                if (websocketFrames().count() >= 2) {
                    const TWebSocketFrame &before = websocketFrames()[websocketFrames().count() - 2];
                    if (before.isFinalFrame() || before.isControlFrame()) {
                        pfrm->clear();
                        tSystemWarn("Invalid continuation frame detected  [%s:%d]", __FILE__, __LINE__);
                        continue;
                    }
                }
            }

            // In case of control frame, moves forward after previous control frames
            if (pfrm->isControlFrame()) {
                if (websocketFrames().count() >= 2) {
                    TWebSocketFrame frm = websocketFrames().takeLast();
                    QMutableListIterator<TWebSocketFrame> it(websocketFrames());
                    while (it.hasNext()) {
                        TWebSocketFrame &f = it.next();
                        if (!f.isControlFrame()) {
                            break;
                        }
                    }

                    it.insert(frm);
                }
            }

            if (!ds.atEnd()) {
                // Prepare next frame
                websocketFrames().append(TWebSocketFrame());
                pfrm = &websocketFrames().last();
            } else {
                break;
            }
        }
    }

parse_end:
    int parsedlen = recvData.size() - dev->bytesAvailable();
    recvData.remove(0, parsedlen);
    return parsedlen;
}


void TAbstractWebSocket::sendHandshakeResponse()
{
    THttpResponseHeader response;
    response.setStatusLine(Tf::SwitchingProtocols, THttpUtility::getResponseReasonPhrase(Tf::SwitchingProtocols));
    response.setRawHeader("Upgrade", "websocket");
    response.setRawHeader("Connection", "Upgrade");

    QByteArray secAccept = QCryptographicHash::hash(reqHeader.rawHeader("Sec-WebSocket-Key").trimmed() + saltToken,
        QCryptographicHash::Sha1)
                               .toBase64();
    response.setRawHeader("Sec-WebSocket-Accept", secAccept);

    writeRawData(response.toByteArray());
}


TAbstractWebSocket *TAbstractWebSocket::searchWebSocket(int sid)
{
    TAbstractWebSocket *sock = nullptr;

    switch (Tf::app()->multiProcessingModule()) {
    case TWebApplication::Thread:
        sock = TWebSocket::searchSocket(sid);
        break;

    case TWebApplication::Epoll: {
#ifdef Q_OS_LINUX
        sock = TEpollWebSocket::searchSocket(sid);
#else
        tFatal("Unsupported MPM: epoll");
#endif
        break;
    }

    default:
        break;
    }

    return sock;
}
