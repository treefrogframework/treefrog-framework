#pragma once
#include <QByteArray>
#include <TGlobal>


class T_CORE_EXPORT TWebSocketFrame {
public:
    enum OpCode {
        Continuation = 0x0,
        TextFrame = 0x1,
        BinaryFrame = 0x2,
        Reserve3 = 0x3,
        Reserve4 = 0x4,
        Reserve5 = 0x5,
        Reserve6 = 0x6,
        Reserve7 = 0x7,
        Close = 0x8,
        Ping = 0x9,
        Pong = 0xA,
        ReserveB = 0xB,
        ReserveC = 0xC,
        ReserveD = 0xD,
        ReserveE = 0xE,
        ReserveF = 0xF,
    };

    TWebSocketFrame();
    TWebSocketFrame(const TWebSocketFrame &other);
    TWebSocketFrame &operator=(const TWebSocketFrame &other);

    bool finBit() const { return _firstByte & 0x80; }
    bool rsv1Bit() const { return _firstByte & 0x40; }
    bool rsv2Bit() const { return _firstByte & 0x20; }
    bool rsv3Bit() const { return _firstByte & 0x10; }
    bool isFinalFrame() const { return finBit(); }
    OpCode opCode() const { return (OpCode)(_firstByte & 0xF); }
    bool isControlFrame() const;
    uint32_t maskKey() const { return _maskKey; }
    uint64_t payloadLength() const { return _payloadLength; }
    const QByteArray &payload() const { return _payload; }
    bool isValid() const { return _valid; }
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
    void setFirstByte(uint8_t byte);
    void setMaskKey(uint32_t maskKey);
    void setPayloadLength(uint64_t length);
    void setPayload(const QByteArray &payload);
    QByteArray &payload() { return _payload; }

    bool validate();
    ProcessingState state() const { return _state; }
    void setState(ProcessingState state);

    uint8_t _firstByte {0x80};
    uint32_t _maskKey {0};
    uint64_t _payloadLength {0};
    QByteArray _payload;  // unmasked data stored
    ProcessingState _state {Empty};
    bool _valid {false};

    friend class TAbstractWebSocket;
    friend class TWebSocket;
    friend class TEpollWebSocket;
    friend class TWebSocketController;
};

