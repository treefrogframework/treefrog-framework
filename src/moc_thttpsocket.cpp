/****************************************************************************
** Meta object code from reading C++ file 'thttpsocket.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "thttpsocket.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'thttpsocket.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_THttpSocket_t {
    QByteArrayData data[9];
    char stringdata0[84];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_THttpSocket_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_THttpSocket_t qt_meta_stringdata_THttpSocket = {
    {
QT_MOC_LITERAL(0, 0, 11), // "THttpSocket"
QT_MOC_LITERAL(1, 12, 10), // "newRequest"
QT_MOC_LITERAL(2, 23, 0), // ""
QT_MOC_LITERAL(3, 24, 12), // "requestWrite"
QT_MOC_LITERAL(4, 37, 4), // "data"
QT_MOC_LITERAL(5, 42, 11), // "readRequest"
QT_MOC_LITERAL(6, 54, 12), // "writeRawData"
QT_MOC_LITERAL(7, 67, 11), // "const char*"
QT_MOC_LITERAL(8, 79, 4) // "size"

    },
    "THttpSocket\0newRequest\0\0requestWrite\0"
    "data\0readRequest\0writeRawData\0const char*\0"
    "size"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_THttpSocket[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   39,    2, 0x06 /* Public */,
       3,    1,   40,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       5,    0,   43,    2, 0x09 /* Protected */,
       6,    2,   44,    2, 0x09 /* Protected */,
       6,    1,   49,    2, 0x09 /* Protected */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QByteArray,    4,

 // slots: parameters
    QMetaType::Void,
    QMetaType::LongLong, 0x80000000 | 7, QMetaType::LongLong,    4,    8,
    QMetaType::LongLong, QMetaType::QByteArray,    4,

       0        // eod
};

void THttpSocket::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        THttpSocket *_t = static_cast<THttpSocket *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->newRequest(); break;
        case 1: _t->requestWrite((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 2: _t->readRequest(); break;
        case 3: { qint64 _r = _t->writeRawData((*reinterpret_cast< const char*(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< qint64*>(_a[0]) = _r; }  break;
        case 4: { qint64 _r = _t->writeRawData((*reinterpret_cast< const QByteArray(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< qint64*>(_a[0]) = _r; }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (THttpSocket::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&THttpSocket::newRequest)) {
                *result = 0;
            }
        }
        {
            typedef void (THttpSocket::*_t)(const QByteArray & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&THttpSocket::requestWrite)) {
                *result = 1;
            }
        }
    }
}

const QMetaObject THttpSocket::staticMetaObject = {
    { &QTcpSocket::staticMetaObject, qt_meta_stringdata_THttpSocket.data,
      qt_meta_data_THttpSocket,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *THttpSocket::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *THttpSocket::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_THttpSocket.stringdata0))
        return static_cast<void*>(const_cast< THttpSocket*>(this));
    return QTcpSocket::qt_metacast(_clname);
}

int THttpSocket::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTcpSocket::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void THttpSocket::newRequest()
{
    QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}

// SIGNAL 1
void THttpSocket::requestWrite(const QByteArray & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
