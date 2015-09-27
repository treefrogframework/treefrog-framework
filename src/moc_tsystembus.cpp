/****************************************************************************
** Meta object code from reading C++ file 'tsystembus.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "tsystembus.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tsystembus.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_TSystemBus_t {
    QByteArrayData data[9];
    char stringdata0[104];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TSystemBus_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TSystemBus_t qt_meta_stringdata_TSystemBus = {
    {
QT_MOC_LITERAL(0, 0, 10), // "TSystemBus"
QT_MOC_LITERAL(1, 11, 12), // "readyReceive"
QT_MOC_LITERAL(2, 24, 0), // ""
QT_MOC_LITERAL(3, 25, 12), // "disconnected"
QT_MOC_LITERAL(4, 38, 7), // "readBus"
QT_MOC_LITERAL(5, 46, 8), // "writeBus"
QT_MOC_LITERAL(6, 55, 11), // "handleError"
QT_MOC_LITERAL(7, 67, 30), // "QLocalSocket::LocalSocketError"
QT_MOC_LITERAL(8, 98, 5) // "error"

    },
    "TSystemBus\0readyReceive\0\0disconnected\0"
    "readBus\0writeBus\0handleError\0"
    "QLocalSocket::LocalSocketError\0error"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TSystemBus[] = {

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
       3,    0,   40,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    0,   41,    2, 0x09 /* Protected */,
       5,    0,   42,    2, 0x09 /* Protected */,
       6,    1,   43,    2, 0x09 /* Protected */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 7,    8,

       0        // eod
};

void TSystemBus::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        TSystemBus *_t = static_cast<TSystemBus *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->readyReceive(); break;
        case 1: _t->disconnected(); break;
        case 2: _t->readBus(); break;
        case 3: _t->writeBus(); break;
        case 4: _t->handleError((*reinterpret_cast< QLocalSocket::LocalSocketError(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (TSystemBus::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&TSystemBus::readyReceive)) {
                *result = 0;
            }
        }
        {
            typedef void (TSystemBus::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&TSystemBus::disconnected)) {
                *result = 1;
            }
        }
    }
}

const QMetaObject TSystemBus::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_TSystemBus.data,
      qt_meta_data_TSystemBus,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *TSystemBus::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TSystemBus::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_TSystemBus.stringdata0))
        return static_cast<void*>(const_cast< TSystemBus*>(this));
    return QObject::qt_metacast(_clname);
}

int TSystemBus::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
void TSystemBus::readyReceive()
{
    QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}

// SIGNAL 1
void TSystemBus::disconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, Q_NULLPTR);
}
QT_END_MOC_NAMESPACE
