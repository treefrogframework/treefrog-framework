#pragma once
constexpr auto TF_VERSION_STR = "2.1.0";
constexpr auto TF_VERSION_NUMBER = 0x020100;
constexpr auto TF_SRC_REVISION = 2419;

#include <QMetaType>
#include <QIODevice>
#include <QtGlobal>


#define T_DEFINE_CONTROLLER(TYPE) T_DEFINE_TYPE(TYPE)
#define T_DEFINE_VIEW(TYPE) T_DEFINE_TYPE(TYPE)
#define T_DEFINE_TYPE(TYPE)                                                                          \
    class Static##TYPE##Definition {                                                                 \
    public:                                                                                          \
        Static##TYPE##Definition() noexcept                                                          \
        {                                                                                            \
            Tf::objectFactories()->insert(QByteArray(#TYPE).toLower(), []() { return new TYPE(); }); \
        }                                                                                            \
    };                                                                                               \
    static Static##TYPE##Definition _static##TYPE##Definition;

#if QT_VERSION < 0x060000
#define T_REGISTER_STREAM_OPERATORS(TYPE)                  \
    class Static##TYPE##Instance {                         \
    public:                                                \
        Static##TYPE##Instance() noexcept                  \
        {                                                  \
            qRegisterMetaTypeStreamOperators<TYPE>(#TYPE); \
        }                                                  \
    };                                                     \
    static Static##TYPE##Instance _static##TYPE##Instance;
#else
// do no longer exist in qt6, qRegisterMetaTypeStreamOperators().
#define T_REGISTER_STREAM_OPERATORS(TYPE)
#endif

#define T_DEFINE_PROPERTY(TYPE, PROPERTY)                                   \
    inline void set##PROPERTY(const TYPE &v__) noexcept { PROPERTY = v__; } \
    inline TYPE get##PROPERTY() const noexcept { return PROPERTY; }


//
// Create Treefrog DLL if TF_MAKEDLL is defined (Windows only)
//
#if defined(Q_OS_WIN)
#if defined(TF_MAKEDLL)  // Create a Treefrog DLL
#define T_CORE_EXPORT Q_DECL_EXPORT
#define T_MODEL_EXPORT
#define T_VIEW_EXPORT
#define T_CONTROLLER_EXPORT
#define T_HELPER_EXPORT
#elif defined(TF_DLL)  // Use a Treefrog DLL
#define T_CORE_EXPORT Q_DECL_IMPORT
#define T_MODEL_EXPORT Q_DECL_EXPORT
#define T_VIEW_EXPORT Q_DECL_EXPORT
#define T_CONTROLLER_EXPORT Q_DECL_EXPORT
#define T_HELPER_EXPORT Q_DECL_EXPORT
#else
#define T_CORE_EXPORT
#define T_MODEL_EXPORT
#define T_VIEW_EXPORT
#define T_CONTROLLER_EXPORT
#define T_HELPER_EXPORT
#endif
#else
#define T_CORE_EXPORT
#define T_MODEL_EXPORT
#define T_VIEW_EXPORT
#define T_CONTROLLER_EXPORT
#define T_HELPER_EXPORT
#endif

#if defined(Q_CC_GNU) && !defined(__INSURE__)
#if defined(Q_CC_MINGW) && !defined(Q_CC_CLANG)
#define T_ATTRIBUTE_FORMAT(A, B) __attribute__((format(gnu_printf, (A), (B))))
#else
#define T_ATTRIBUTE_FORMAT(A, B) __attribute__((format(printf, (A), (B))))
#endif
#else
#define T_ATTRIBUTE_FORMAT(A, B)
#endif


#define T_EXPORT(VAR)                                            \
    do {                                                         \
        QVariant ___##VAR##_;                                    \
        ___##VAR##_.setValue(VAR);                               \
        ((TAbstractController *)(Tf::currentContext())->currentController())->exportVariant(QLatin1String(#VAR), (___##VAR##_), true); \
    } while (0)
#define texport(VAR) T_EXPORT(VAR)

#define T_EXPORT_UNLESS(VAR)                                      \
    do {                                                          \
        QVariant ___##VAR##_;                                     \
        ___##VAR##_.setValue(VAR);                                \
        ((TAbstractController *)(Tf::currentContext())->currentController())->exportVariant(QLatin1String(#VAR), (___##VAR##_), false); \
    } while (0)
#define texportUnless(VAR) T_EXPORT_UNLESS(VAR)

#define T_FETCH(TYPE, VAR) TYPE VAR = variant(QLatin1String(#VAR)).value<TYPE>()
#define tfetch(TYPE, VAR) T_FETCH(TYPE, VAR)

#define T_FETCH_V(TYPE, VAR, DEFAULT) TYPE VAR = (hasVariant(QLatin1String(#VAR))) ? (variant(QLatin1String(#VAR)).value<TYPE>()) : (DEFAULT)
#define tfetchv(TYPE, VAR, DEFAULT) T_FETCH_V(TYPE, VAR, DEFAULT)

#if QT_VERSION < 0x060000
#define T_EHEX(VAR)                                      \
    do {                                                 \
        auto ___##VAR##_ = variant(QLatin1String(#VAR)); \
        int ___##VAR##_type = (___##VAR##_).type();      \
        switch (___##VAR##_type) {                       \
        case QMetaType::QJsonValue:                      \
            eh((___##VAR##_).toJsonValue());             \
            break;                                       \
        case QMetaType::QJsonObject:                     \
            eh((___##VAR##_).toJsonObject());            \
            break;                                       \
        case QMetaType::QJsonArray:                      \
            eh((___##VAR##_).toJsonArray());             \
            break;                                       \
        case QMetaType::QJsonDocument:                   \
            eh((___##VAR##_).toJsonDocument());          \
            break;                                       \
        case QMetaType::QVariantMap:                     \
            eh((___##VAR##_).toMap());                   \
            break;                                       \
        default:                                         \
            eh(___##VAR##_);                             \
        }                                                \
    } while (0)

#else
#define T_EHEX(VAR)                                      \
    do {                                                 \
        auto ___##VAR##_ = variant(QLatin1String(#VAR)); \
        int ___##VAR##_type = (___##VAR##_).typeId();    \
        switch (___##VAR##_type) {                       \
        case QMetaType::QJsonValue:                      \
            eh((___##VAR##_).toJsonValue());             \
            break;                                       \
        case QMetaType::QJsonObject:                     \
            eh((___##VAR##_).toJsonObject());            \
            break;                                       \
        case QMetaType::QJsonArray:                      \
            eh((___##VAR##_).toJsonArray());             \
            break;                                       \
        case QMetaType::QJsonDocument:                   \
            eh((___##VAR##_).toJsonDocument());          \
            break;                                       \
        case QMetaType::QVariantMap:                     \
            eh((___##VAR##_).toMap());                   \
            break;                                       \
        default:                                         \
            eh(___##VAR##_);                             \
        }                                                \
    } while (0)

#endif

#define tehex(VAR) T_EHEX(VAR)

#define T_EHEX_V(VAR, DEFAULT)                                         \
    do {                                                               \
        auto ___##VAR##_ = variant(QLatin1String(#VAR));               \
        if ((___##VAR##_).isNull()) {                                  \
            eh(DEFAULT);                                               \
        } else {                                                       \
            T_EHEX(VAR);                                               \
        }                                                              \
    } while (0)

#define tehexv(VAR, DEFAULT) T_EHEX_V(VAR, DEFAULT)

// alias of tehexv
#define T_EHEX2(VAR, DEFAULT) T_EHEX_V(VAR, DEFAULT)
#define tehex2(VAR, DEFAULT) T_EHEX2(VAR, DEFAULT)

#if QT_VERSION < 0x060000
#define T_ECHOEX(VAR)                                    \
    do {                                                 \
        auto ___##VAR##_ = variant(QLatin1String(#VAR)); \
        int ___##VAR##_type = (___##VAR##_).type();      \
        switch (___##VAR##_type) {                       \
        case QMetaType::QJsonValue:                      \
            echo((___##VAR##_).toJsonValue());           \
            break;                                       \
        case QMetaType::QJsonObject:                     \
            echo((___##VAR##_).toJsonObject());          \
            break;                                       \
        case QMetaType::QJsonArray:                      \
            echo((___##VAR##_).toJsonArray());           \
            break;                                       \
        case QMetaType::QJsonDocument:                   \
            echo((___##VAR##_).toJsonDocument());        \
            break;                                       \
        case QMetaType::QVariantMap:                     \
            echo((___##VAR##_).toMap());                 \
            break;                                       \
        default:                                         \
            echo(___##VAR##_);                           \
        }                                                \
    } while (0)

#else
#define T_ECHOEX(VAR)                                    \
    do {                                                 \
        auto ___##VAR##_ = variant(QLatin1String(#VAR)); \
        int ___##VAR##_type = (___##VAR##_).typeId();    \
        switch (___##VAR##_type) {                       \
        case QMetaType::QJsonValue:                      \
            echo((___##VAR##_).toJsonValue());           \
            break;                                       \
        case QMetaType::QJsonObject:                     \
            echo((___##VAR##_).toJsonObject());          \
            break;                                       \
        case QMetaType::QJsonArray:                      \
            echo((___##VAR##_).toJsonArray());           \
            break;                                       \
        case QMetaType::QJsonDocument:                   \
            echo((___##VAR##_).toJsonDocument());        \
            break;                                       \
        case QMetaType::QVariantMap:                     \
            echo((___##VAR##_).toMap());                 \
            break;                                       \
        default:                                         \
            echo(___##VAR##_);                           \
        }                                                \
    } while (0)

#endif


#define techoex(VAR) T_ECHOEX(VAR)

#define T_ECHOEX_V(VAR, DEFAULT)                                       \
    do {                                                               \
        auto ___##VAR##_ = variant(QLatin1String(#VAR));               \
        if ((___##VAR##_).isNull()) {                                  \
            echo(DEFAULT);                                             \
        } else {                                                       \
            T_ECHOEX(VAR);                                             \
        }                                                              \
    } while (0)

#define techoexv(VAR, DEFAULT) T_ECHOEX_V(VAR, DEFAULT)

// alias of techoexv
#define T_ECHOEX2(VAR, DEFAULT) T_ECHOEX_V(VAR, DEFAULT)
#define techoex2(VAR, DEFAULT) T_ECHOEX2(VAR, DEFAULT)

#define T_FLASH(VAR)                                  \
    do {                                              \
        QVariant ___##VAR##_;                         \
        ___##VAR##_.setValue(VAR);                    \
        ((TAbstractController *)(Tf::currentContext())->currentController())->setFlash(QLatin1String(#VAR), (___##VAR##_)); \
    } while (0)

#define tflash(VAR) T_FLASH(VAR)

#define T_VARIANT(VAR) (variant(QLatin1String(#VAR)).toString())

//  Some classes do not permit copies and moves to be made of an object.
#define T_DISABLE_COPY(Class)      \
    Class(const Class &) = delete; \
    Class &operator=(const Class &) = delete;

#define T_DISABLE_MOVE(Class) \
    Class(Class &&) = delete; \
    Class &operator=(Class &&) = delete;

#define tFatal TDebug(Tf::FatalLevel).fatal
#define tError TDebug(Tf::ErrorLevel).error
#define tWarn TDebug(Tf::WarnLevel).warn
#define tInfo TDebug(Tf::InfoLevel).info
#define tDebug TDebug(Tf::DebugLevel).debug
#define tTrace TDebug(Tf::TraceLevel).trace

namespace Tf {
#if QT_VERSION < 0x060000  // 6.0.0
constexpr auto ReadOnly = QIODevice::ReadOnly;
constexpr auto WriteOnly = QIODevice::WriteOnly;
#else
constexpr auto ReadOnly = QIODeviceBase::ReadOnly;
constexpr auto WriteOnly = QIODeviceBase::WriteOnly;
#endif
}

#include "tfexception.h"
#include "tfnamespace.h"
#include <TDebug>
#include <cstdint>
#include <functional>

class TWebApplication;
class TActionContext;
class TAbstractController;
class TAbstractActionContext;
class TAppSettings;
class TDatabaseContext;
class TCache;
class QSqlDatabase;

namespace Tf {
T_CORE_EXPORT TWebApplication *app() noexcept;
T_CORE_EXPORT TAppSettings *appSettings() noexcept;
T_CORE_EXPORT const QVariantMap &conf(const QString &configName) noexcept;
T_CORE_EXPORT void msleep(unsigned long msecs) noexcept;
T_CORE_EXPORT qint64 getMSecsSinceEpoch();

// Thread-safe std::random number generator
T_CORE_EXPORT uint32_t rand32_r() noexcept;
T_CORE_EXPORT uint64_t rand64_r() noexcept;
T_CORE_EXPORT uint64_t random(uint64_t min, uint64_t max) noexcept;
T_CORE_EXPORT uint64_t random(uint64_t max) noexcept;

T_CORE_EXPORT TCache *cache() noexcept;
T_CORE_EXPORT TActionContext *currentContext();
inline const TActionContext *constCurrentContext() { return currentContext(); }
T_CORE_EXPORT TDatabaseContext *currentDatabaseContext();
T_CORE_EXPORT QSqlDatabase &currentSqlDatabase(int id) noexcept;
T_CORE_EXPORT QMap<QByteArray, std::function<QObject *()>> *objectFactories() noexcept;

// LZ4 lossless compression algorithm
T_CORE_EXPORT QByteArray lz4Compress(const char *data, int nbytes, int compressionLevel = 1) noexcept;
T_CORE_EXPORT QByteArray lz4Compress(const QByteArray &data, int compressionLevel = 1) noexcept;
T_CORE_EXPORT QByteArray lz4Uncompress(const char *data, int nbytes) noexcept;
T_CORE_EXPORT QByteArray lz4Uncompress(const QByteArray &data) noexcept;

constexpr auto CRLFCRLF = "\x0d\x0a\x0d\x0a";
constexpr auto CRLF = "\x0d\x0a";
constexpr auto LF = "\x0a";
}
