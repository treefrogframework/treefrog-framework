/****************************************************************************
** Meta object code from reading C++ file 'tsessionobject.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "tsessionobject.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tsessionobject.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_TSessionObject_t {
    QByteArrayData data[4];
    char stringdata0[34];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TSessionObject_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TSessionObject_t qt_meta_stringdata_TSessionObject = {
    {
QT_MOC_LITERAL(0, 0, 14), // "TSessionObject"
QT_MOC_LITERAL(1, 15, 2), // "id"
QT_MOC_LITERAL(2, 18, 4), // "data"
QT_MOC_LITERAL(3, 23, 10) // "updated_at"

    },
    "TSessionObject\0id\0data\0updated_at"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TSessionObject[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       3,   14, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // properties: name, type, flags
       1, QMetaType::QString, 0x00095003,
       2, QMetaType::QByteArray, 0x00095003,
       3, QMetaType::QDateTime, 0x00095003,

       0        // eod
};

void TSessionObject::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{

#ifndef QT_NO_PROPERTIES
    if (_c == QMetaObject::ReadProperty) {
        TSessionObject *_t = static_cast<TSessionObject *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->getid(); break;
        case 1: *reinterpret_cast< QByteArray*>(_v) = _t->getdata(); break;
        case 2: *reinterpret_cast< QDateTime*>(_v) = _t->getupdated_at(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        TSessionObject *_t = static_cast<TSessionObject *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setid(*reinterpret_cast< QString*>(_v)); break;
        case 1: _t->setdata(*reinterpret_cast< QByteArray*>(_v)); break;
        case 2: _t->setupdated_at(*reinterpret_cast< QDateTime*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject TSessionObject::staticMetaObject = {
    { &TSqlObject::staticMetaObject, qt_meta_stringdata_TSessionObject.data,
      qt_meta_data_TSessionObject,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *TSessionObject::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TSessionObject::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_TSessionObject.stringdata0))
        return static_cast<void*>(const_cast< TSessionObject*>(this));
    return TSqlObject::qt_metacast(_clname);
}

int TSessionObject::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = TSqlObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    
#ifndef QT_NO_PROPERTIES
   if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 3;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}
QT_END_MOC_NAMESPACE
