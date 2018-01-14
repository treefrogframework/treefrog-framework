#ifndef TDISPATCHER_H
#define TDISPATCHER_H

#include <QMetaType>
#include <QMetaMethod>
#include <QMetaObject>
#include <QStringList>
#include <TGlobal>
#include "tsystemglobal.h"


template <class T>
class TDispatcher
{
public:
    TDispatcher(const QString &metaTypeName);
    ~TDispatcher();

    bool invoke(const QByteArray &method, const QStringList &args = QStringList(), Qt::ConnectionType connectionType = Qt::AutoConnection);
    T *object();
    QString typeName() const { return metaType; }

private:
    QString metaType;
    int typeId {0};
    T *ptr {nullptr};

    T_DISABLE_COPY(TDispatcher)
    T_DISABLE_MOVE(TDispatcher)
};


template <class T>
inline TDispatcher<T>::TDispatcher(const QString &metaTypeName)
    : metaType(metaTypeName)
{ }

template <class T>
inline TDispatcher<T>::~TDispatcher()
{
    if (ptr) {
        if (typeId > 0) {
            QMetaType::destroy(typeId, ptr);
        } else {
            delete ptr;
        }
    }
}

template <class T>
inline bool TDispatcher<T>::invoke(const QByteArray &method, const QStringList &args, Qt::ConnectionType connectionType)
{
    T_TRACEFUNC("");
    const int NUM_PARAMS = 11;
    static const char *const params[NUM_PARAMS] = { "()", "(QString)",
                                                    "(QString,QString)",
                                                    "(QString,QString,QString)",
                                                    "(QString,QString,QString,QString)",
                                                    "(QString,QString,QString,QString,QString)",
                                                    "(QString,QString,QString,QString,QString,QString)",
                                                    "(QString,QString,QString,QString,QString,QString,QString)",
                                                    "(QString,QString,QString,QString,QString,QString,QString,QString)",
                                                    "(QString,QString,QString,QString,QString,QString,QString,QString,QString)",
                                                    "(QString,QString,QString,QString,QString,QString,QString,QString,QString,QString)" };

    object();
    if (Q_UNLIKELY(!ptr)) {
        tSystemDebug("Failed to invoke, no such class: %s", qPrintable(metaType));
        return false;
    }

    int argcnt = 0;
    int idx = -1;
    int narg = qMin(args.count(), NUM_PARAMS - 1);
    for (int i = narg; i >= 0; i--) {
        // Find method
        QByteArray mtd = method + params[i];
        idx = ptr->metaObject()->indexOfSlot(mtd.constData());
        if (idx >= 0) {
            argcnt = i;
            tSystemDebug("Found method: %s", mtd.constData());
            break;
        }
    }

    if (idx < 0) {
        for (int i = narg + 1; i < NUM_PARAMS - 1; i++) {
            // Find method
            QByteArray mtd = method + params[i];
            idx = ptr->metaObject()->indexOfSlot(mtd.constData());
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
        QMetaMethod mm = ptr->metaObject()->method(idx);
        tSystemDebug("Invoke method: %s", qPrintable(metaType + "." + method));
        switch (argcnt) {
        case 0:
            res = mm.invoke(ptr, connectionType);
            break;
        case 1:
            res = mm.invoke(ptr, connectionType, Q_ARG(QString, args.value(0)));
            break;
        case 2:
            res = mm.invoke(ptr, connectionType, Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)));
            break;
        case 3:
            res = mm.invoke(ptr, connectionType,
                            Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)));
            break;
        case 4:
            res = mm.invoke(ptr, connectionType,
                            Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                            Q_ARG(QString, args.value(3)));
            break;
        case 5:
            res = mm.invoke(ptr, connectionType,
                            Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                            Q_ARG(QString, args.value(3)), Q_ARG(QString, args.value(4)));
            break;
        case 6:
            res = mm.invoke(ptr, connectionType,
                            Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                            Q_ARG(QString, args.value(3)), Q_ARG(QString, args.value(4)), Q_ARG(QString, args.value(5)));
            break;
        case 7:
            res = mm.invoke(ptr, connectionType,
                            Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                            Q_ARG(QString, args.value(3)), Q_ARG(QString, args.value(4)), Q_ARG(QString, args.value(5)),
                            Q_ARG(QString, args.value(6)));
            break;
        case 8:
            res = mm.invoke(ptr, connectionType,
                            Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                            Q_ARG(QString, args.value(3)), Q_ARG(QString, args.value(4)), Q_ARG(QString, args.value(5)),
                            Q_ARG(QString, args.value(6)), Q_ARG(QString, args.value(7)));
            break;
        case 9:
            res = mm.invoke(ptr, connectionType,
                            Q_ARG(QString, args.value(0)), Q_ARG(QString, args.value(1)), Q_ARG(QString, args.value(2)),
                            Q_ARG(QString, args.value(3)), Q_ARG(QString, args.value(4)), Q_ARG(QString, args.value(5)),
                            Q_ARG(QString, args.value(6)), Q_ARG(QString, args.value(7)), Q_ARG(QString, args.value(8)));
            break;
        default:
            res = mm.invoke(ptr, connectionType,
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
    T_TRACEFUNC("");

    if (!ptr) {
        auto factory = Tf::objectFactories()->value(metaType.toLatin1().toLower());
        if (Q_LIKELY(factory)) {
            ptr = dynamic_cast<T*>(factory());
            if (ptr) {
                typeId = 0;
            }
        }
    }

    if (Q_UNLIKELY(!ptr)) {
        if (typeId <= 0 && !metaType.isEmpty()) {
            typeId = QMetaType::type(metaType.toLatin1().constData());
            if (Q_LIKELY(typeId > 0)) {
                ptr = static_cast<T *>(QMetaType::create(typeId));
                Q_CHECK_PTR(ptr);
                tSystemDebug("Constructs object, class: %s  typeId: %d", qPrintable(metaType), typeId);
            } else {
                tSystemDebug("No such object class : %s", qPrintable(metaType));
            }
        }
    }
    return ptr;
}

#endif // TDISPATCHER_H
