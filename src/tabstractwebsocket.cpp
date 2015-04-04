/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QCryptographicHash>
#include <THttpRequestHeader>
#include <THttpUtility>
#include "tabstractwebsocket.h"
#include "twebsocketframe.h"
#include "twebsocketendpoint.h"
#include "turlroute.h"
#include "tdispatcher.h"

const QByteArray saltToken = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";


TAbstractWebSocket::TAbstractWebSocket()
{ }


TAbstractWebSocket::~TAbstractWebSocket()
{ }


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
    if (websocketFrames().isEmpty()) {
        websocketFrames().append(TWebSocketFrame());
    } else {
        const TWebSocketFrame &f = websocketFrames().last();
        if (f.state() == TWebSocketFrame::Completed) {
            websocketFrames().append(TWebSocketFrame());
        }
    }

    TWebSocketFrame &ref = websocketFrames().last();
    quint8  b;
    quint16 w;
    quint32 n;
    quint64 d;

    QDataStream ds(recvData);
    ds.setByteOrder(QDataStream::BigEndian);
    QIODevice *dev = ds.device();

    while (!ds.atEnd()) {
        switch (ref.state()) {
        case TWebSocketFrame::Empty: {
            if (Q_UNLIKELY(dev->bytesAvailable() < 4)) {
                tSystemError("WebSocket header too short  [%s:%d]", __FILE__, __LINE__);
                return -1;
            }

            ds >> b;
            ref.setFirstByte(b);

            ds >> b;
            bool maskFlag = b & 0x80;
            quint8 len = b & 0x7f;

            // payload length
            switch (len) {
            case 126:
                if (Q_UNLIKELY(dev->bytesAvailable() < (int)sizeof(w))) {
                    tSystemError("WebSocket header too short  [%s:%d]", __FILE__, __LINE__);
                    return -1;
                }
                ds >> w;
                if (Q_UNLIKELY(w < 126)) {
                    tSystemError("WebSocket protocol error  [%s:%d]", __FILE__, __LINE__);
                    return -1;
                }
                ref.setPayloadLength( w );
                break;

            case 127:
                if (Q_UNLIKELY(dev->bytesAvailable() < (int)sizeof(d))) {
                    tSystemError("WebSocket header too short  [%s:%d]", __FILE__, __LINE__);
                    return -1;
                }
                ds >> d;
                if (Q_UNLIKELY(d <= 0xFFFF)) {
                    tSystemError("WebSocket protocol error  [%s:%d]", __FILE__, __LINE__);
                    return -1;
                }
                ref.setPayloadLength( d );
                break;

            default:
                ref.setPayloadLength( len );
                break;
            }

            // Mask key
            if (maskFlag) {
                if (Q_UNLIKELY(dev->bytesAvailable() < (int)sizeof(n))) {
                    tSystemError("WebSocket parse error  [%s:%d]", __FILE__, __LINE__);
                    return -1;
                }
                ds >> n;
                ref.setMaskKey( n );
            }

            if (ref.payloadLength() == 0) {
                ref.setState(TWebSocketFrame::Completed);
            } else {
                ref.setState(TWebSocketFrame::HeaderParsed);
                ref.payload().reserve(ref.payloadLength());
            }

            tSystemDebug("WebSocket header read length: %lld", dev->pos());
            tSystemDebug("WebSocket payload length:%lld", ref.payloadLength());
            break; }

        case TWebSocketFrame::HeaderParsed:  // fall through
        case TWebSocketFrame::MoreData: {
            tSystemDebug("WebSocket reading payload:  available length:%lld", dev->bytesAvailable());
            int size = qMin((qint64)(ref.payloadLength() - ref.payload().size()), dev->bytesAvailable());
            if (Q_UNLIKELY(size <= 0)) {
                Q_ASSERT(0);
                break;
            }

            char *p = ref.payload().data() + ref.payload().size();
            size = ds.readRawData(p, size);

            if (ref.maskKey()) {
                // Unmask
                const quint8 mask[4] = { quint8((ref.maskKey() & 0xFF000000) >> 24),
                                         quint8((ref.maskKey() & 0x00FF0000) >> 16),
                                         quint8((ref.maskKey() & 0x0000FF00) >> 8),
                                         quint8((ref.maskKey() & 0x000000FF)) };

                int i = ref.payload().size();
                const char *end = p + size;
                while (p < end) {
                    *p++ ^= mask[i++ % 4];
                }
            }
            ref.payload().resize( ref.payload().size() + size );
            tSystemDebug("WebSocket payload length read: %d", ref.payload().length());

            if ((quint64)ref.payload().size() == ref.payloadLength()) {
                ref.setState(TWebSocketFrame::Completed);
            } else {
                ref.setState(TWebSocketFrame::MoreData);
            }
            break; }

        case TWebSocketFrame::Completed:
            break;

        default:
            Q_ASSERT(0);
            return -1;
            break;
        }

        if (ref.state() == TWebSocketFrame::Completed) {
            if (Q_UNLIKELY(!ref.validate())) {
                ref.clear();
                continue;
            }

            // Fragmented message validation
            if (ref.opCode() == TWebSocketFrame::Continuation) {
                if (websocketFrames().count() >= 2) {
                    const TWebSocketFrame &before = websocketFrames()[websocketFrames().count() - 2];
                    if (before.isFinalFrame() || before.isControlFrame()) {
                        ref.clear();
                        continue;
                    }
                }
            }

            // In case of control frame, moves forward after previous control frames
            if (ref.isControlFrame()) {
                if (websocketFrames().count() >= 2) {
                    TWebSocketFrame frm = websocketFrames().takeLast();
                    QMutableListIterator<TWebSocketFrame> it(websocketFrames());
                    it.toBack();
                    while (it.hasPrevious()) {
                        TWebSocketFrame &f = it.previous();
                        if (f.isControlFrame()) {
                            // Inserts after control frame
                            it.next();
                            it.insert(frm);
                        }
                    }

                    if (!it.hasPrevious()) {
                        websocketFrames().prepend(frm);
                    }

                    ref = websocketFrames().last();
                }
            }

            if (!ds.atEnd() && ref.isFinalFrame()) {
                // Prepare next frame
                websocketFrames().append(TWebSocketFrame());
                ref = websocketFrames().last();
            }
        }
    }

    Q_ASSERT(dev->bytesAvailable() == 0);
    int sz = recvData.size();
    recvData.truncate(0);
    return sz - dev->bytesAvailable();
}


THttpResponseHeader TAbstractWebSocket::handshakeResponse(const THttpRequestHeader &header)
{
    THttpResponseHeader response;
    response.setStatusLine(Tf::SwitchingProtocols, THttpUtility::getResponseReasonPhrase(Tf::SwitchingProtocols));
    response.setRawHeader("Upgrade", "websocket");
    response.setRawHeader("Connection", "Upgrade");

    QByteArray secAccept = QCryptographicHash::hash(header.rawHeader("Sec-WebSocket-Key").trimmed() + saltToken,
                                                    QCryptographicHash::Sha1).toBase64();
    response.setRawHeader("Sec-WebSocket-Accept", secAccept);
    return response;
}
