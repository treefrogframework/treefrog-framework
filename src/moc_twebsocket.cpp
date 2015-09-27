/****************************************************************************
** Meta object code from reading C++ file 'twebsocket.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "twebsocket.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'twebsocket.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_TWebSocket_t {
    QByteArrayData data[17];
    char stringdata0[188];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TWebSocket_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TWebSocket_t qt_meta_stringdata_TWebSocket = {
    {
QT_MOC_LITERAL(0, 0, 10), // "TWebSocket"
QT_MOC_LITERAL(1, 11, 12), // "sendByWorker"
QT_MOC_LITERAL(2, 24, 0), // ""
QT_MOC_LITERAL(3, 25, 4), // "data"
QT_MOC_LITERAL(4, 30, 18), // "disconnectByWorker"
QT_MOC_LITERAL(5, 49, 18), // "sendTextForPublish"
QT_MOC_LITERAL(6, 68, 4), // "text"
QT_MOC_LITERAL(7, 73, 14), // "const QObject*"
QT_MOC_LITERAL(8, 88, 6), // "except"
QT_MOC_LITERAL(9, 95, 20), // "sendBinaryForPublish"
QT_MOC_LITERAL(10, 116, 6), // "binary"
QT_MOC_LITERAL(11, 123, 8), // "sendPong"
QT_MOC_LITERAL(12, 132, 11), // "readRequest"
QT_MOC_LITERAL(13, 144, 13), // "releaseWorker"
QT_MOC_LITERAL(14, 158, 11), // "sendRawData"
QT_MOC_LITERAL(15, 170, 5), // "close"
QT_MOC_LITERAL(16, 176, 11) // "deleteLater"

    },
    "TWebSocket\0sendByWorker\0\0data\0"
    "disconnectByWorker\0sendTextForPublish\0"
    "text\0const QObject*\0except\0"
    "sendBinaryForPublish\0binary\0sendPong\0"
    "readRequest\0releaseWorker\0sendRawData\0"
    "close\0deleteLater"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TWebSocket[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   69,    2, 0x06 /* Public */,
       4,    0,   72,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       5,    2,   73,    2, 0x0a /* Public */,
       9,    2,   78,    2, 0x0a /* Public */,
      11,    1,   83,    2, 0x0a /* Public */,
      11,    0,   86,    2, 0x2a /* Public | MethodCloned */,
      12,    0,   87,    2, 0x0a /* Public */,
      13,    0,   88,    2, 0x0a /* Public */,
      14,    1,   89,    2, 0x0a /* Public */,
      15,    0,   92,    2, 0x0a /* Public */,
      16,    0,   93,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QByteArray,    3,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, 0x80000000 | 7,    6,    8,
    QMetaType::Void, QMetaType::QByteArray, 0x80000000 | 7,   10,    8,
    QMetaType::Void, QMetaType::QByteArray,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QByteArray,    3,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void TWebSocket::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        TWebSocket *_t = static_cast<TWebSocket *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->sendByWorker((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 1: _t->disconnectByWorker(); break;
        case 2: _t->sendTextForPublish((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QObject*(*)>(_a[2]))); break;
        case 3: _t->sendBinaryForPublish((*reinterpret_cast< const QByteArray(*)>(_a[1])),(*reinterpret_cast< const QObject*(*)>(_a[2]))); break;
        case 4: _t->sendPong((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 5: _t->sendPong(); break;
        case 6: _t->readRequest(); break;
        case 7: _t->releaseWorker(); break;
        case 8: _t->sendRawData((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 9: _t->close(); break;
        case 10: _t->deleteLater(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (TWebSocket::*_t)(const QByteArray & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&TWebSocket::sendByWorker)) {
                *result = 0;
            }
        }
        {
            typedef void (TWebSocket::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&TWebSocket::disconnectByWorker)) {
                *result = 1;
            }
        }
    }
}

const QMetaObject TWebSocket::staticMetaObject = {
    { &QTcpSocket::staticMetaObject, qt_meta_stringdata_TWebSocket.data,
      qt_meta_data_TWebSocket,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *TWebSocket::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TWebSocket::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_TWebSocket.stringdata0))
        return static_cast<void*>(const_cast< TWebSocket*>(this));
    if (!strcmp(_clname, "TAbstractWebSocket"))
        return static_cast< TAbstractWebSocket*>(const_cast< TWebSocket*>(this));
    return QTcpSocket::qt_metacast(_clname);
}

int TWebSocket::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTcpSocket::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void TWebSocket::sendByWorker(const QByteArray & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void TWebSocket::disconnectByWorker()
{
    QMetaObject::activate(this, &staticMetaObject, 1, Q_NULLPTR);
}
QT_END_MOC_NAMESPACE
