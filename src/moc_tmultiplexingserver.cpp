/****************************************************************************
** Meta object code from reading C++ file 'tmultiplexingserver.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "tmultiplexingserver.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tmultiplexingserver.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_TMultiplexingServer_t {
    QByteArrayData data[5];
    char stringdata0[58];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TMultiplexingServer_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TMultiplexingServer_t qt_meta_stringdata_TMultiplexingServer = {
    {
QT_MOC_LITERAL(0, 0, 19), // "TMultiplexingServer"
QT_MOC_LITERAL(1, 20, 15), // "incomingRequest"
QT_MOC_LITERAL(2, 36, 0), // ""
QT_MOC_LITERAL(3, 37, 13), // "TEpollSocket*"
QT_MOC_LITERAL(4, 51, 6) // "socket"

    },
    "TMultiplexingServer\0incomingRequest\0"
    "\0TEpollSocket*\0socket"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TMultiplexingServer[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   19,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Bool, 0x80000000 | 3,    4,

       0        // eod
};

void TMultiplexingServer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        TMultiplexingServer *_t = static_cast<TMultiplexingServer *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: { bool _r = _t->incomingRequest((*reinterpret_cast< TEpollSocket*(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef bool (TMultiplexingServer::*_t)(TEpollSocket * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&TMultiplexingServer::incomingRequest)) {
                *result = 0;
            }
        }
    }
}

const QMetaObject TMultiplexingServer::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_TMultiplexingServer.data,
      qt_meta_data_TMultiplexingServer,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *TMultiplexingServer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TMultiplexingServer::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_TMultiplexingServer.stringdata0))
        return static_cast<void*>(const_cast< TMultiplexingServer*>(this));
    if (!strcmp(_clname, "TApplicationServerBase"))
        return static_cast< TApplicationServerBase*>(const_cast< TMultiplexingServer*>(this));
    return QThread::qt_metacast(_clname);
}

int TMultiplexingServer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
bool TMultiplexingServer::incomingRequest(TEpollSocket * _t1)
{
    bool _t0 = bool();
    void *_a[] = { const_cast<void*>(reinterpret_cast<const void*>(&_t0)), const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
    return _t0;
}
QT_END_MOC_NAMESPACE
