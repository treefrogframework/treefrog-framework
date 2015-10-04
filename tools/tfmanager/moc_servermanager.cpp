/****************************************************************************
** Meta object code from reading C++ file 'servermanager.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "servermanager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'servermanager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_TreeFrog__ServerManager_t {
    QByteArrayData data[11];
    char stringdata0[157];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TreeFrog__ServerManager_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TreeFrog__ServerManager_t qt_meta_stringdata_TreeFrog__ServerManager = {
    {
QT_MOC_LITERAL(0, 0, 23), // "TreeFrog::ServerManager"
QT_MOC_LITERAL(1, 24, 18), // "updateServerStatus"
QT_MOC_LITERAL(2, 43, 0), // ""
QT_MOC_LITERAL(3, 44, 11), // "errorDetect"
QT_MOC_LITERAL(4, 56, 22), // "QProcess::ProcessError"
QT_MOC_LITERAL(5, 79, 5), // "error"
QT_MOC_LITERAL(6, 85, 12), // "serverFinish"
QT_MOC_LITERAL(7, 98, 8), // "exitCode"
QT_MOC_LITERAL(8, 107, 20), // "QProcess::ExitStatus"
QT_MOC_LITERAL(9, 128, 10), // "exitStatus"
QT_MOC_LITERAL(10, 139, 17) // "readStandardError"

    },
    "TreeFrog::ServerManager\0updateServerStatus\0"
    "\0errorDetect\0QProcess::ProcessError\0"
    "error\0serverFinish\0exitCode\0"
    "QProcess::ExitStatus\0exitStatus\0"
    "readStandardError"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TreeFrog__ServerManager[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   34,    2, 0x09 /* Protected */,
       3,    1,   35,    2, 0x09 /* Protected */,
       6,    2,   38,    2, 0x09 /* Protected */,
      10,    0,   43,    2, 0x09 /* Protected */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4,    5,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 8,    7,    9,
    QMetaType::Void,

       0        // eod
};

void TreeFrog::ServerManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        ServerManager *_t = static_cast<ServerManager *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->updateServerStatus(); break;
        case 1: _t->errorDetect((*reinterpret_cast< QProcess::ProcessError(*)>(_a[1]))); break;
        case 2: _t->serverFinish((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QProcess::ExitStatus(*)>(_a[2]))); break;
        case 3: _t->readStandardError(); break;
        default: ;
        }
    }
}

const QMetaObject TreeFrog::ServerManager::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_TreeFrog__ServerManager.data,
      qt_meta_data_TreeFrog__ServerManager,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *TreeFrog::ServerManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TreeFrog::ServerManager::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_TreeFrog__ServerManager.stringdata0))
        return static_cast<void*>(const_cast< ServerManager*>(this));
    return QObject::qt_metacast(_clname);
}

int TreeFrog::ServerManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
