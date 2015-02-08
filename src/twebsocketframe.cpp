/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "twebsocketframe.h"
#include <TSystemGlobal>


TWebSocketFrame::TWebSocketFrame()
    : firstByte_(0), maskKey_(0), payloadLength_(0),
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


void TWebSocketFrame::setFirstByte(quint8 byte)
{
    firstByte_ = byte;
}


void TWebSocketFrame::setState(ProcessingState state)
{
    state_ = state;
}


void TWebSocketFrame::setMaskKey(quint32 maskKey)
{
    maskKey_ = maskKey;
}


void TWebSocketFrame::setPayloadLength(quint64 length)
{
    payloadLength_ = length;
}


void TWebSocketFrame::clear()
{
    firstByte_ = 0;
    maskKey_ = 0;
    payloadLength_ = 0;
    payload_.truncate(0);
    state_ = Empty;
    valid_ = false;
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
    valid_ &= ((opCode() >= TEpollWebSocket::Continuation && opCode() <= TEpollWebSocket::BinaryFrame)
               || (opCode() >= TEpollWebSocket::Close && opCode() <= TEpollWebSocket::Pong));

    if (isControlFrame()) {
        valid_ &= (payloadLength() <= 125); // MUST have a payload length of 125 bytes or less
        valid_ &= (finBit() == true);  // MUST NOT be fragmented
    }

    if (!valid_) {
        tSystemError("WebSocket frame validation error  [%s:%d]", __FILE__, __LINE__);
    }
    return valid_;
}
