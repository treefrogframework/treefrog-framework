/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "twebsocketframe.h"
#include <TSystemGlobal>


TWebSocketFrame::TWebSocketFrame()
    : firstByte_(0x80), maskKey_(0), payloadLength_(0),
      payload_(), state_(Empty), valid_(false)
{ }


TWebSocketFrame::TWebSocketFrame(const TWebSocketFrame &other)
    : firstByte_(other.firstByte_),
      maskKey_(other.maskKey_),
      payloadLength_(other.payloadLength_),
      payload_(other.payload_),
      state_(other.state_),
      valid_(other.valid_)
{ }


TWebSocketFrame &TWebSocketFrame::operator=(const TWebSocketFrame &other)
{
    firstByte_ = other.firstByte_;
    maskKey_ = other.maskKey_;
    payloadLength_ = other.payloadLength_;
    payload_ = other.payload_;
    state_ = other.state_;
    valid_ = other.valid_;
    return *this;
}


bool TWebSocketFrame::isControlFrame() const
{
    return (opCode() & 0x08);
}


void TWebSocketFrame::setFinBit(bool fin)
{
    if (fin) {
        firstByte_ |= 0x80;
    } else {
        firstByte_ &= ~0x80;
    }
}


void TWebSocketFrame::setOpCode(TWebSocketFrame::OpCode opCode)
{
    firstByte_ &= ~0xF;
    firstByte_ |= (quint8)opCode;
}


void TWebSocketFrame::setFirstByte(quint8 byte)
{
    firstByte_ = byte;
}


void TWebSocketFrame::setMaskKey(quint32 maskKey)
{
    maskKey_ = maskKey;
}


void TWebSocketFrame::setPayloadLength(quint64 length)
{
    payloadLength_ = length;
}


void TWebSocketFrame::setPayload(const QByteArray &payload)
{
    payload_ = payload;
    payloadLength_ = payload.length();
}


void TWebSocketFrame::setState(ProcessingState state)
{
    state_ = state;
}


void TWebSocketFrame::clear()
{
    firstByte_ = 0x80;
    maskKey_ = 0;
    payloadLength_ = 0;
    payload_.truncate(0);
    state_ = Empty;
    valid_ = false;
}


QByteArray TWebSocketFrame::toByteArray() const
{
    QByteArray frame;
    int plen = payload_.length();
    frame.reserve(plen + 10);
    QDataStream ds(&frame, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);

    uchar b = firstByte_ | 0x80;  // FIN bit
    if (!opCode()) {
        b |= 0x1;  // text frame
    }
    ds << b;

    b = 0;
    if (maskKey_) {
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
    if (maskKey_) {
        ds << maskKey_;
    }

    if (plen > 0) {
        ds.writeRawData(payload_.data(), plen);
    }
    return frame;
}


bool TWebSocketFrame::validate()
{
    if (state_ != Completed) {
        return false;
    }

    valid_  = true;
    valid_ &= (rsv1Bit() == false);
    valid_ &= (rsv2Bit() == false);
    valid_ &= (rsv3Bit() == false);
    valid_ &= ((opCode() >= TWebSocketFrame::Continuation && opCode() <= TWebSocketFrame::BinaryFrame)
               || (opCode() >= TWebSocketFrame::Close && opCode() <= TWebSocketFrame::Pong));

    if (isControlFrame()) {
        valid_ &= (payloadLength() <= 125); // MUST have a payload length of 125 bytes or less
        valid_ &= (finBit() == true);  // MUST NOT be fragmented
    }

    if (!valid_) {
        tSystemError("WebSocket frame validation error  [%s:%d]", __FILE__, __LINE__);
    }
    return valid_;
}
