/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QDataStream>
#include <QCryptographicHash>
#include <TWebApplication>
#include <TSystemGlobal>
#include <TAppSettings>
#include <THttpRequestHeader>
#include <THttpUtility>
#include "tepoll.h"
#include "tepollwebsocket.h"
#include "twebsocketframe.h"
#include "twsactionworker.h"

const int BUFFER_RESERVE_SIZE = 127;
const QByteArray saltToken = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";


TEpollWebSocket::TEpollWebSocket(int socketDescriptor, const QHostAddress &address, const THttpRequestHeader &header)
    : TEpollSocket(socketDescriptor, address), reqHeader(header),
      recvBuffer(), frames()
{
    recvBuffer.reserve(BUFFER_RESERVE_SIZE);
}


TEpollWebSocket::~TEpollWebSocket()
{ }


bool TEpollWebSocket::canReadRequest()
{
    for (QListIterator<TWebSocketFrame> it(frames); it.hasNext(); ) {
        const TWebSocketFrame &frm = it.next();
        if (frm.isFinalFrame() && frm.state() == TWebSocketFrame::Completed) {
            return true;
        }
    }
    return false;
}


bool TEpollWebSocket::isTextRequest() const
{
    if (!frames.isEmpty()) {
        const TWebSocketFrame &frm = frames.first();
        return (frm.opCode() == TWebSocketFrame::TextFrame);
    }
    return false;
}


bool TEpollWebSocket::isBinaryRequest() const
{
    if (!frames.isEmpty()) {
        const TWebSocketFrame &frm = frames.first();
        return (frm.opCode() == TWebSocketFrame::BinaryFrame);
    }
    return false;
}


QString TEpollWebSocket::readTextRequest()
{
    return QString::fromUtf8(readBinaryRequest());
}


QByteArray TEpollWebSocket::readBinaryRequest()
{
    Q_ASSERT(canReadRequest());

    QByteArray ret;
    while (!frames.isEmpty()) {
        TWebSocketFrame frm = frames.takeFirst();
        ret += frm.payload();
        if (frm.isFinalFrame() && frm.state() == TWebSocketFrame::Completed) {
            break;
        }
    }
    tSystemDebug("readBinaryRequest: %s", ret.data());
    return ret;
}


void *TEpollWebSocket::getRecvBuffer(int size)
{
    int len = recvBuffer.size();
    recvBuffer.reserve(len + size);
    return recvBuffer.data() + len;
}


bool TEpollWebSocket::seekRecvBuffer(int pos)
{
    int size = recvBuffer.size();
    if (Q_UNLIKELY(pos <= 0 || size + pos > recvBuffer.capacity())) {
        clear();
        return false;
    }

    size += pos;
    recvBuffer.resize(size);
    int len = parse();
    if (len < 0) {
        tSystemError("WebSocket parse error [%s:%d]", __FILE__, __LINE__);
        close();
        return false;
    }
    recvBuffer.truncate(0);
    return true;
}


void TEpollWebSocket::startWorker()
{
    tSystemDebug("TEpollWebSocket::startWorker");
    Q_ASSERT(canReadRequest());

    do {
        TWebSocketFrame::OpCode opcode = frames.first().opCode();
        QByteArray binary = readBinaryRequest();
        TWsActionWorker *worker = new TWsActionWorker(socketUuid(), reqHeader.path(), opcode, binary);
        worker->moveToThread(Tf::app()->thread());
        connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
        worker->start();
    } while (canReadRequest());
}


void TEpollWebSocket::startWorkerForOpening(const TSession &session)
{
    TWsActionWorker *worker = new TWsActionWorker(socketUuid(), session);
    worker->moveToThread(Tf::app()->thread());
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    worker->start();
}


int TEpollWebSocket::parse()
{
    if (frames.isEmpty()) {
        frames.append(TWebSocketFrame());
    } else {
        const TWebSocketFrame &f = frames.last();
        if (f.state() == TWebSocketFrame::Completed) {
            frames.append(TWebSocketFrame());
        }
    }

    TWebSocketFrame &ref = frames.last();
    quint8  b;
    quint16 w;
    quint32 n;
    quint64 d;

    QDataStream ds(recvBuffer);
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
                if (frames.count() >= 2) {
                    const TWebSocketFrame &before = frames[frames.count() - 2];
                    if (before.isFinalFrame() || before.isControlFrame()) {
                        ref.clear();
                        continue;
                    }
                }
            }

            // In case of control frame, moves forward after previous control frames
            if (ref.isControlFrame()) {
                if (frames.count() >= 2) {
                    TWebSocketFrame frm = frames.takeLast();
                    QMutableListIterator<TWebSocketFrame> it(frames);
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
                        frames.prepend(frm);
                    }

                    ref = frames.last();
                }
            }

            if (!ds.atEnd() && ref.isFinalFrame()) {
                // Prepare next frame
                frames.append(TWebSocketFrame());
                ref = frames.last();
            }
        }
    }

    Q_ASSERT(dev->bytesAvailable() == 0);
    return recvBuffer.size() - dev->bytesAvailable();
}


void TEpollWebSocket::clear()
{
    recvBuffer.resize(BUFFER_RESERVE_SIZE);
    recvBuffer.squeeze();
    recvBuffer.truncate(0);
    frames.clear();
}


THttpResponseHeader TEpollWebSocket::handshakeResponse() const
{
    THttpResponseHeader response;
    response.setStatusLine(Tf::SwitchingProtocols, THttpUtility::getResponseReasonPhrase(Tf::SwitchingProtocols));
    response.setRawHeader("Upgrade", "websocket");
    response.setRawHeader("Connection", "Upgrade");

    QByteArray secAccept = QCryptographicHash::hash(reqHeader.rawHeader("Sec-WebSocket-Key").trimmed() + saltToken,
                                                    QCryptographicHash::Sha1).toBase64();
    response.setRawHeader("Sec-WebSocket-Accept", secAccept);
    return response;
}


void TEpollWebSocket::sendText(const QByteArray &socketUuid, const QString &message)
{
    TWebSocketFrame frame;
    frame.setOpCode(TWebSocketFrame::TextFrame);
    frame.setPayload(message.toUtf8());
    TEpoll::instance()->setSendData(socketUuid, frame.toByteArray());
}


void TEpollWebSocket::sendBinary(const QByteArray &socketUuid, const QByteArray &data)
{
    TWebSocketFrame frame;
    frame.setOpCode(TWebSocketFrame::BinaryFrame);
    frame.setPayload(data);
    TEpoll::instance()->setSendData(socketUuid, frame.toByteArray());
}


void TEpollWebSocket::sendPing(const QByteArray &socketUuid)
{
    TWebSocketFrame frame;
    frame.setOpCode(TWebSocketFrame::Ping);
    TEpoll::instance()->setSendData(socketUuid, frame.toByteArray());
}


void TEpollWebSocket::sendPong(const QByteArray &socketUuid)
{
    TWebSocketFrame frame;
    frame.setOpCode(TWebSocketFrame::Pong);
    TEpoll::instance()->setSendData(socketUuid, frame.toByteArray());
}


void TEpollWebSocket::disconnect(const QByteArray &socketUuid)
{
    TEpoll::instance()->setDisconnect(socketUuid);
}
