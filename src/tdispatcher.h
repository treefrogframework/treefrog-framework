#pragma once
#include "tsystemglobal.h"
#include <QMetaMethod>
#include <QMetaObject>
#include <QMetaType>
#include <QStringList>
#include <TGlobal>

constexpr int NUM_METHOD_PARAMS = 11;


template <class T>
class TDispatcher {
public:
    TDispatcher(const QString &metaTypeName);
    ~TDispatcher();

    bool invoke(const QByteArray &method, const QStringList &args = QStringList(), Qt::ConnectionType connectionType = Qt::AutoConnection);
    T *object();
    QString typeName() const { return _metaType; }
    QMetaMethod method(const QByteArray &methodName, int argCount);
    bool hasMethod(const QByteArray &methodName);

private:
    QString _metaType;
    int _typeId {0};
    T *_ptr {nullptr};

    T_DISABLE_COPY(TDispatcher)
    T_DISABLE_MOVE(TDispatcher)
};


template <class T>
inline TDispatcher<T>::TDispatcher(const QString &metaTypeName) :
    _metaType(metaTypeName)
{
}

template <class T>
inline TDispatcher<T>::~TDispatcher()
{
    if (_ptr) {
        if (_typeId > 0) {
#if QT_VERSION < 0x060000
            QMetaType::destroy(_typeId, _ptr);
#else
            QMetaType(_typeId).destroy(_ptr);
#endif
        } else {
            delete _ptr;
        }
    }
}


template <class T>
inline QMetaMethod TDispatcher<T>::method(const QByteArray &methodName, int argCount)
{
    static const QByteArray params[NUM_METHOD_PARAMS] = {
        QByteArrayLiteral("()"),
        QByteArrayLiteral("(QString)"),
        QByteArrayLiteral("(QString,QString)"),
        QByteArrayLiteral("(QString,QString,QString)"),
        QByteArrayLiteral("(QString,QString,QString,QString)"),
        QByteArrayLiteral("(QString,QString,QString,QString,QString)"),
        QByteArrayLiteral("(QString,QString,QString,QString,QString,QString)"),
        QByteArrayLiteral("(QString,QString,QString,QString,QString,QString,QString)"),
        QByteArrayLiteral("(QString,QString,QString,QString,QString,QString,QString,QString)"),
        QByteArrayLiteral("(QString,QString,QString,QString,QString,QString,QString,QString,QString)"),
        QByteArrayLiteral("(QString,QString,QString,QString,QString,QString,QString,QString,QString,QString)")};

    object();
    if (Q_UNLIKELY(!_ptr)) {
        tSystemDebug("Failed to invoke, no such class: %s", qUtf8Printable(_metaType));
        return QMetaMethod();
    }

    int idx = -1;
    int narg = qMin(argCount, NUM_METHOD_PARAMS - 1);
    for (int i = narg; i >= 0; i--) {
        // Find method
        QByteArray mtd = methodName;
        mtd += params[i];
        idx = _ptr->metaObject()->indexOfSlot(mtd.constData());
        if (idx >= 0) {
            tSystemDebug("Found method: %s", mtd.constData());
            break;
        }
    }

    if (idx < 0) {
        for (int i = narg + 1; i < NUM_METHOD_PARAMS - 1; i++) {
            // Find method
            QByteArray mtd = methodName;
            mtd += params[i];
            idx = _ptr->metaObject()->indexOfSlot(mtd.constData());
            if (idx >= 0) {
                tSystemDebug("Found method: %s", mtd.constData());
                break;
            }
        }
    }

    if (Q_UNLIKELY(idx < 0)) {
        tSystemDebug("No such method: %s", qUtf8Printable(methodName));
        return QMetaMethod();
    }

    return _ptr->metaObject()->method(idx);
}


template <class T>
inline bool TDispatcher<T>::hasMethod(const QByteArray &methodName)
{
    for (int i = 0; i < NUM_METHOD_PARAMS; i++) {
        if (method(methodName, i).isValid()) {
            return true;
        }
    }
    return false;
}


template <class T>
inline bool TDispatcher<T>::invoke(const QByteArray &method, const QStringList &args, Qt::ConnectionType connectionType)
{
    bool ret = false;
    QMetaMethod mm = this->method(method, args.count());

    if (Q_UNLIKELY(!mm.isValid())) {
        tSystemDebug("No such method: %s", qUtf8Printable(method));
    } else {
        tSystemDebug("Invoke method: %s", qUtf8Printable(_metaType + "." + method));
        switch (args.count()) {
        case 0:
            ret = mm.invoke(_ptr, connectionType);
            break;
        case 1:
            ret = mm.invoke(_ptr, connectionType, Q_ARG(QString, args.value(0)));
            break;
        case 2:
            ret = mm.invoke(_ptr, connectionType, Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)));
            break;
        case 3:
            ret = mm.invoke(_ptr, connectionType,
                Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)));
            break;
        case 4:
            ret = mm.invoke(_ptr, connectionType,
                Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                Q_ARG(QString, args.value(3)));
            break;
        case 5:
            ret = mm.invoke(_ptr, connectionType,
                Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                Q_ARG(QString, args.value(3)), Q_ARG(QString, args.value(4)));
            break;
        case 6:
            ret = mm.invoke(_ptr, connectionType,
                Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                Q_ARG(QString, args.value(3)), Q_ARG(QString, args.value(4)), Q_ARG(QString, args.value(5)));
            break;
        case 7:
            ret = mm.invoke(_ptr, connectionType,
                Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                Q_ARG(QString, args.value(3)), Q_ARG(QString, args.value(4)), Q_ARG(QString, args.value(5)),
                Q_ARG(QString, args.value(6)));
            break;
        case 8:
            ret = mm.invoke(_ptr, connectionType,
                Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                Q_ARG(QString, args.value(3)), Q_ARG(QString, args.value(4)), Q_ARG(QString, args.value(5)),
                Q_ARG(QString, args.value(6)), Q_ARG(QString, args.value(7)));
            break;
        case 9:
            ret = mm.invoke(_ptr, connectionType,
                Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                Q_ARG(QString, args.value(3)), Q_ARG(QString, args.value(4)), Q_ARG(QString, args.value(5)),
                Q_ARG(QString, args.value(6)), Q_ARG(QString, args.value(7)), Q_ARG(QString, args.value(8)));
            break;
        default:
            ret = mm.invoke(_ptr, connectionType,
                Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                Q_ARG(QString, args.value(3)), Q_ARG(QString, args.value(4)), Q_ARG(QString, args.value(5)),
                Q_ARG(QString, args.value(6)), Q_ARG(QString, args.value(7)), Q_ARG(QString, args.value(8)),
                Q_ARG(QString, args.value(9)));
            break;
        }
    }
    return ret;
}


template <class T>
inline T *TDispatcher<T>::object()
{
    if (!_ptr) {
        auto factory = Tf::objectFactories()->value(_metaType.toLatin1().toLower());
        if (Q_LIKELY(factory)) {
            auto p = factory();
            _ptr = dynamic_cast<T *>(p);
            if (!_ptr) {
                delete p;
                _typeId = 0;
            }
        }
    }
    return _ptr;
}
