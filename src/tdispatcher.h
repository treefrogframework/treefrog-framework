#pragma once
#include "tsystemglobal.h"
#include <QMetaMethod>
#include <QMetaObject>
#include <QMetaType>
#include <QStringList>
#include <TGlobal>


template <class T>
class TDispatcher {
public:
    TDispatcher(const QString &metaTypeName);
    ~TDispatcher();

    bool invoke(const QByteArray &method, const QStringList &args = QStringList(), Qt::ConnectionType connectionType = Qt::AutoConnection);
    T *object();
    QString typeName() const { return _metaType; }

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
            QMetaType::destroy(_typeId, _ptr);
        } else {
            delete _ptr;
        }
    }
}

template <class T>
inline bool TDispatcher<T>::invoke(const QByteArray &method, const QStringList &args, Qt::ConnectionType connectionType)
{
    constexpr int NUM_PARAMS = 11;
    static const QByteArray params[NUM_PARAMS] = {
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
        tSystemDebug("Failed to invoke, no such class: %s", qPrintable(_metaType));
        return false;
    }

    int argcnt = 0;
    int idx = -1;
    int narg = qMin(args.count(), NUM_PARAMS - 1);
    for (int i = narg; i >= 0; i--) {
        // Find method
        QByteArray mtd = method;
        mtd += params[i];
        idx = _ptr->metaObject()->indexOfSlot(mtd.constData());
        if (idx >= 0) {
            argcnt = i;
            tSystemDebug("Found method: %s", mtd.constData());
            break;
        }
    }

    if (idx < 0) {
        for (int i = narg + 1; i < NUM_PARAMS - 1; i++) {
            // Find method
            QByteArray mtd = method;
            mtd += params[i];
            idx = _ptr->metaObject()->indexOfSlot(mtd.constData());
            if (idx >= 0) {
                argcnt = i;
                tSystemDebug("Found method: %s", mtd.constData());
                break;
            }
        }
    }

    bool res = false;
    if (Q_UNLIKELY(idx < 0)) {
        tSystemDebug("No such method: %s", qPrintable(method));
        return res;
    } else {
        QMetaMethod mm = _ptr->metaObject()->method(idx);
        tSystemDebug("Invoke method: %s", qPrintable(_metaType + "." + method));
        switch (argcnt) {
        case 0:
            res = mm.invoke(_ptr, connectionType);
            break;
        case 1:
            res = mm.invoke(_ptr, connectionType, Q_ARG(QString, args.value(0)));
            break;
        case 2:
            res = mm.invoke(_ptr, connectionType, Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)));
            break;
        case 3:
            res = mm.invoke(_ptr, connectionType,
                Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)));
            break;
        case 4:
            res = mm.invoke(_ptr, connectionType,
                Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                Q_ARG(QString, args.value(3)));
            break;
        case 5:
            res = mm.invoke(_ptr, connectionType,
                Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                Q_ARG(QString, args.value(3)), Q_ARG(QString, args.value(4)));
            break;
        case 6:
            res = mm.invoke(_ptr, connectionType,
                Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                Q_ARG(QString, args.value(3)), Q_ARG(QString, args.value(4)), Q_ARG(QString, args.value(5)));
            break;
        case 7:
            res = mm.invoke(_ptr, connectionType,
                Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                Q_ARG(QString, args.value(3)), Q_ARG(QString, args.value(4)), Q_ARG(QString, args.value(5)),
                Q_ARG(QString, args.value(6)));
            break;
        case 8:
            res = mm.invoke(_ptr, connectionType,
                Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                Q_ARG(QString, args.value(3)), Q_ARG(QString, args.value(4)), Q_ARG(QString, args.value(5)),
                Q_ARG(QString, args.value(6)), Q_ARG(QString, args.value(7)));
            break;
        case 9:
            res = mm.invoke(_ptr, connectionType,
                Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                Q_ARG(QString, args.value(3)), Q_ARG(QString, args.value(4)), Q_ARG(QString, args.value(5)),
                Q_ARG(QString, args.value(6)), Q_ARG(QString, args.value(7)), Q_ARG(QString, args.value(8)));
            break;
        default:
            res = mm.invoke(_ptr, connectionType,
                Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                Q_ARG(QString, args.value(3)), Q_ARG(QString, args.value(4)), Q_ARG(QString, args.value(5)),
                Q_ARG(QString, args.value(6)), Q_ARG(QString, args.value(7)), Q_ARG(QString, args.value(8)),
                Q_ARG(QString, args.value(9)));
            break;
        }
    }
    return res;
}

template <class T>
inline T *TDispatcher<T>::object()
{
    if (!_ptr) {
        auto factory = Tf::objectFactories()->value(_metaType.toLatin1().toLower());
        if (Q_LIKELY(factory)) {
            _ptr = dynamic_cast<T *>(factory());
            if (_ptr) {
                _typeId = 0;
            }
        }
    }

    if (Q_UNLIKELY(!_ptr)) {
        if (_typeId <= 0 && !_metaType.isEmpty()) {
            _typeId = QMetaType::type(_metaType.toLatin1().constData());
            if (Q_LIKELY(_typeId > 0)) {
                _ptr = static_cast<T *>(QMetaType::create(_typeId));
                Q_CHECK_PTR(_ptr);
                tSystemDebug("Constructs object, class: %s  typeId: %d", qPrintable(_metaType), _typeId);
            } else {
                tSystemDebug("No such object class : %s", qPrintable(_metaType));
            }
        }
    }
    return _ptr;
}

