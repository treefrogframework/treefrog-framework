/****************************************************************************
** Meta object code from reading C++ file 'tepollwebsocket.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "tepollwebsocket.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tepollwebsocket.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_TEpollWebSocket_t {
    QByteArrayData data[11];
    char stringdata0[119];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TEpollWebSocket_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TEpollWebSocket_t qt_meta_stringdata_TEpollWebSocket = {
    {
QT_MOC_LITERAL(0, 0, 15), // "TEpollWebSocket"
QT_MOC_LITERAL(1, 16, 13), // "releaseWorker"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 18), // "sendTextForPublish"
QT_MOC_LITERAL(4, 50, 4), // "text"
QT_MOC_LITERAL(5, 55, 14), // "const QObject*"
QT_MOC_LITERAL(6, 70, 6), // "except"
QT_MOC_LITERAL(7, 77, 20), // "sendBinaryForPublish"
QT_MOC_LITERAL(8, 98, 6), // "binary"
QT_MOC_LITERAL(9, 105, 8), // "sendPong"
QT_MOC_LITERAL(10, 114, 4) // "data"

    },
    "TEpollWebSocket\0releaseWorker\0\0"
    "sendTextForPublish\0text\0const QObject*\0"
    "except\0sendBinaryForPublish\0binary\0"
    "sendPong\0data"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TEpollWebSocket[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   39,    2, 0x0a /* Public */,
       3,    2,   40,    2, 0x0a /* Public */,
       7,    2,   45,    2, 0x0a /* Public */,
       9,    1,   50,    2, 0x0a /* Public */,
       9,    0,   53,    2, 0x2a /* Public | MethodCloned */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 5,    4,    6,
    QMetaType::Void, QMetaType::QByteArray, 0x80000000 | 5,    8,    6,
    QMetaType::Void, QMetaType::QByteArray,   10,
    QMetaType::Void,

       0        // eod
};

void TEpollWebSocket::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        TEpollWebSocket *_t = static_cast<TEpollWebSocket *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->releaseWorker(); break;
        case 1: _t->sendTextForPublish((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QObject*(*)>(_a[2]))); break;
        case 2: _t->sendBinaryForPublish((*reinterpret_cast< const QByteArray(*)>(_a[1])),(*reinterpret_cast< const QObject*(*)>(_a[2]))); break;
        case 3: _t->sendPong((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 4: _t->sendPong(); break;
        default: ;
        }
    }
}

const QMetaObject TEpollWebSocket::staticMetaObject = {
    { &TEpollSocket::staticMetaObject, qt_meta_stringdata_TEpollWebSocket.data,
      qt_meta_data_TEpollWebSocket,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *TEpollWebSocket::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TEpollWebSocket::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_TEpollWebSocket.stringdata0))
        return static_cast<void*>(const_cast< TEpollWebSocket*>(this));
    if (!strcmp(_clname, "TAbstractWebSocket"))
        return static_cast< TAbstractWebSocket*>(const_cast< TEpollWebSocket*>(this));
    return TEpollSocket::qt_metacast(_clname);
}

int TEpollWebSocket::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = TEpollSocket::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
