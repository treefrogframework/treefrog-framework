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
    quint32 maskKey() const { return _maskKey; }
    quint64 payloadLength() const { return _payloadLength; }
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
    void setFirstByte(quint8 byte);
    void setMaskKey(quint32 maskKey);
    void setPayloadLength(quint64 length);
    void setPayload(const QByteArray &payload);
    QByteArray &payload() { return _payload; }

    bool validate();
    ProcessingState state() const { return _state; }
    void setState(ProcessingState state);

    quint8 _firstByte {0x80};
    quint32 _maskKey {0};
    quint64 _payloadLength {0};
    QByteArray _payload;  // unmasked data stored
    ProcessingState _state {Empty};
    bool _valid {false};

    friend class TAbstractWebSocket;
    friend class TWebSocket;
    friend class TEpollWebSocket;
    friend class TWebSocketController;
};

