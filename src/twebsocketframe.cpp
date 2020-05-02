/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "twebsocketframe.h"
#include <QDataStream>
#include <QIODevice>
#include <TSystemGlobal>


TWebSocketFrame::TWebSocketFrame()
{
}


TWebSocketFrame::TWebSocketFrame(const TWebSocketFrame &other) :
    _firstByte(other._firstByte),
    _maskKey(other._maskKey),
    _payloadLength(other._payloadLength),
    _payload(other._payload),
    _state(other._state),
    _valid(other._valid)
{
}


TWebSocketFrame &TWebSocketFrame::operator=(const TWebSocketFrame &other)
{
    _firstByte = other._firstByte;
    _maskKey = other._maskKey;
    _payloadLength = other._payloadLength;
    _payload = other._payload;
    _state = other._state;
    _valid = other._valid;
    return *this;
}


bool TWebSocketFrame::isControlFrame() const
{
    return (opCode() & 0x08);
}


void TWebSocketFrame::setFinBit(bool fin)
{
    if (fin) {
        _firstByte |= 0x80;
    } else {
        _firstByte &= ~0x80;
    }
}


void TWebSocketFrame::setOpCode(TWebSocketFrame::OpCode opCode)
{
    _firstByte &= ~0xF;
    _firstByte |= (quint8)opCode;
}


void TWebSocketFrame::setFirstByte(quint8 byte)
{
    _firstByte = byte;
}


void TWebSocketFrame::setMaskKey(quint32 maskKey)
{
    _maskKey = maskKey;
}


void TWebSocketFrame::setPayloadLength(quint64 length)
{
    _payloadLength = length;
}


void TWebSocketFrame::setPayload(const QByteArray &payload)
{
    _payload = payload;
    _payloadLength = payload.length();
}


void TWebSocketFrame::setState(ProcessingState state)
{
    _state = state;
}


void TWebSocketFrame::clear()
{
    _firstByte = 0x80;
    _maskKey = 0;
    _payloadLength = 0;
    _payload.truncate(0);
    _state = Empty;
    _valid = false;
}


QByteArray TWebSocketFrame::toByteArray() const
{
    QByteArray frame;
    int plen = _payload.length();
    frame.reserve(plen + 10);
    QDataStream ds(&frame, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);

    uchar b = _firstByte | 0x80;  // FIN bit
    if (!opCode()) {
        b |= 0x1;  // text frame
    }
    ds << b;

    b = 0;
    if (_maskKey) {
        b = 0x80;  // Mask bit
    }

    if (plen <= 125) {
        b |= (uchar)plen;
        ds << b;
    } else if (plen <= (int)0xFFFF) {
        b |= (uchar)126;
        ds << b << (quint16)plen;
    } else {
        b |= (uchar)127;
        ds << b << (quint64)plen;
    }

    // masking key
    if (_maskKey) {
        ds << _maskKey;
    }

    if (plen > 0) {
        ds.writeRawData(_payload.data(), plen);
    }
    return frame;
}


bool TWebSocketFrame::validate()
{
    if (_state != Completed) {
        return false;
    }

    _valid = true;
    _valid &= (rsv1Bit() == false);
    _valid &= (rsv2Bit() == false);
    _valid &= (rsv3Bit() == false);
    if (!_valid) {
        tSystemError("WebSocket frame validation error : Incorrect RSV bit  [%s:%d]", __FILE__, __LINE__);
        return _valid;
    }

    _valid &= ((opCode() >= TWebSocketFrame::Continuation && opCode() <= TWebSocketFrame::BinaryFrame)
        || (opCode() >= TWebSocketFrame::Close && opCode() <= TWebSocketFrame::Pong));
    if (!_valid) {
        tSystemError("WebSocket frame validation error : Incorrect opcode : %d  [%s:%d]", (int)opCode(), __FILE__, __LINE__);
        return _valid;
    }

    if (isControlFrame()) {
        _valid &= (payloadLength() <= 125);  // MUST have a payload length of 125 bytes or less
        _valid &= (finBit() == true);  // MUST NOT be fragmented
    }

    if (!_valid) {
        tSystemError("WebSocket frame validation error : Invalid control frame  [%s:%d]", __FILE__, __LINE__);
    }
    return _valid;
}
