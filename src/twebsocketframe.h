#ifndef TWEBSOCKETFRAME_H
#define TWEBSOCKETFRAME_H

#include <QByteArray>
#include <TGlobal>


class T_CORE_EXPORT TWebSocketFrame
{
public:
    enum OpCode {
        Continuation = 0x0,
        TextFrame    = 0x1,
        BinaryFrame  = 0x2,
        Reserve3     = 0x3,
        Reserve4     = 0x4,
        Reserve5     = 0x5,
        Reserve6     = 0x6,
        Reserve7     = 0x7,
        Close        = 0x8,
        Ping         = 0x9,
        Pong         = 0xA,
        ReserveB     = 0xB,
        ReserveC     = 0xC,
        ReserveD     = 0xD,
        ReserveE     = 0xE,
        ReserveF     = 0xF,
    };

    TWebSocketFrame();
    TWebSocketFrame(const TWebSocketFrame &other);
    TWebSocketFrame &operator=(const TWebSocketFrame &other);

    bool finBit() const { return firstByte_ & 0x80; }
    bool rsv1Bit() const { return firstByte_ & 0x40; }
    bool rsv2Bit() const { return firstByte_ & 0x20; }
    bool rsv3Bit() const { return firstByte_ & 0x10; }
    bool isFinalFrame() const { return finBit(); }
    OpCode opCode() const { return (OpCode)(firstByte_ & 0xF); }
    bool isControlFrame() const;
    quint32 maskKey() const { return maskKey_; }
    quint64 payloadLength() const { return payloadLength_; }
    const QByteArray &payload() const { return payload_; }
    bool isValid() const { return valid_; }
    void clear();
    QByteArray toByteArray() const;

private:
    enum ProcessingState {
        Empty = 0,
        HeaderParsed,
        MoreData,
        Completed,
    };

    void setFinBit(bool fin);
    void setOpCode(OpCode opCode);
    void setFirstByte(quint8 byte);
    void setMaskKey(quint32 maskKey);
    void setPayloadLength(quint64 length);
    void setPayload(const QByteArray &payload);
    QByteArray &payload() { return payload_; }

    bool validate();
    ProcessingState state() const { return state_; }
    void setState(ProcessingState state);

    quint8 firstByte_;
    quint32 maskKey_;
    quint64 payloadLength_;
    QByteArray payload_;   // unmasked data stored
    ProcessingState state_;
    bool valid_;

    friend class TAbstractWebSocket;
    friend class TWebSocket;
    friend class TEpollWebSocket;
    friend class TWebSocketController;
};

#endif // TWEBSOCKETFRAME_H
