#ifndef TWEBSOCKETFRAME_H
#define TWEBSOCKETFRAME_H

#include <QByteArray>
#include <TGlobal>
#include "tepollwebsocket.h"


class T_CORE_EXPORT TWebSocketFrame
{
public:
    TWebSocketFrame();
    TWebSocketFrame(const TWebSocketFrame &other);
    TWebSocketFrame &operator=(const TWebSocketFrame &other);

    bool finBit() const { return firstByte_ & 0x80; }
    bool rsv1Bit() const { return firstByte_ & 0x40; }
    bool rsv2Bit() const { return firstByte_ & 0x20; }
    bool rsv3Bit() const { return firstByte_ & 0x10; }
    bool isFinalFrame() const { return finBit(); }
    TEpollWebSocket::OpCode opCode() const { return (TEpollWebSocket::OpCode)(firstByte_ & 0xF); }
    bool isControlFrame() const;
    quint32 maskKey() const { return maskKey_; }
    quint64 payloadLength() const { return payloadLength_; }
    const QByteArray &payload() const { return payload_; }
    bool isValid() const { return valid_; }
    void clear();

private:
    enum ProcessingState {
        Empty,
        HeaderParsed,
        MoreData,
        Completed,
    };

    void setFirstByte(quint8 byte);
    ProcessingState state() const { return state_; }
    void setState(ProcessingState state);
    void setMaskKey(quint32 maskKey);
    void setPayloadLength(quint64 length);
    QByteArray &payload() { return payload_; }
    bool validate();

    quint8 firstByte_;
    quint32 maskKey_;
    quint64 payloadLength_;
    QByteArray payload_;   // unmasked data stored
    ProcessingState state_;
    bool valid_;

    friend class TEpollWebSocket;
};

#endif // TWEBSOCKETFRAME_H
