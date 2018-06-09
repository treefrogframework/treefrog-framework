#ifndef TGLOBAL_H
#define TGLOBAL_H

#define TF_VERSION_STR "1.22.0"
#define TF_VERSION_NUMBER 0x012200
#define TF_SRC_REVISION 1685

#include <QtGlobal>
#include <QMetaType>

#define T_DECLARE_CONTROLLER(TYPE,NAME)  \
    typedef TYPE NAME;                   \
    Q_DECLARE_METATYPE(NAME)

// Deprecated
#define T_REGISTER_CONTROLLER(TYPE) T_REGISTER_METATYPE(TYPE)
#define T_REGISTER_VIEW(TYPE)       T_REGISTER_METATYPE(TYPE)
#define T_REGISTER_METATYPE(TYPE)                               \
    class Static##TYPE##Instance                                \
    {                                                           \
    public:                                                     \
        Static##TYPE##Instance()                                \
        {                                                       \
            qRegisterMetaType<TYPE>();                          \
        }                                                       \
    };                                                          \
    static Static##TYPE##Instance _static##TYPE##Instance;

#define T_DEFINE_CONTROLLER(TYPE)  T_DEFINE_TYPE(TYPE)
#define T_DEFINE_VIEW(TYPE)        T_DEFINE_TYPE(TYPE)
#define T_DEFINE_TYPE(TYPE)                                     \
    class Static##TYPE##Definition                              \
    {                                                           \
    public:                                                     \
        Static##TYPE##Definition()                              \
        {                                                       \
            Tf::objectFactories()->insert(QByteArray(#TYPE).toLower(), [](){ return new TYPE(); }); \
        }                                                       \
    };                                                          \
    static Static##TYPE##Definition _static##TYPE##Definition;

#define T_REGISTER_STREAM_OPERATORS(TYPE)                       \
    class Static##TYPE##Instance                                \
    {                                                           \
    public:                                                     \
        Static##TYPE##Instance()                                \
        {                                                       \
            qRegisterMetaTypeStreamOperators<TYPE>(#TYPE);      \
        }                                                       \
    };                                                          \
    static Static##TYPE##Instance _static##TYPE##Instance;

#define T_DEFINE_PROPERTY(TYPE,PROPERTY)                           \
    inline void set##PROPERTY(const TYPE &v__) { PROPERTY = v__; } \
    inline TYPE get##PROPERTY() const { return PROPERTY; }


//
// Create Treefrog DLL if TF_MAKEDLL is defined (Windows only)
//
#if defined(Q_OS_WIN)
#  if defined(TF_MAKEDLL)   // Create a Treefrog DLL
#    define T_CORE_EXPORT Q_DECL_EXPORT
#    define T_MODEL_EXPORT
#    define T_VIEW_EXPORT
#    define T_CONTROLLER_EXPORT
#    define T_HELPER_EXPORT
#  elif defined(TF_DLL)   // Use a Treefrog DLL
#    define T_CORE_EXPORT Q_DECL_IMPORT
#    define T_MODEL_EXPORT Q_DECL_EXPORT
#    define T_VIEW_EXPORT Q_DECL_EXPORT
#    define T_CONTROLLER_EXPORT Q_DECL_EXPORT
#    define T_HELPER_EXPORT Q_DECL_EXPORT
#  else
#    define T_CORE_EXPORT
#    define T_MODEL_EXPORT
#    define T_VIEW_EXPORT
#    define T_CONTROLLER_EXPORT
#    define T_HELPER_EXPORT
#  endif
#else
#  define T_CORE_EXPORT
#  define T_MODEL_EXPORT
#  define T_VIEW_EXPORT
#  define T_CONTROLLER_EXPORT
#  define T_HELPER_EXPORT
#endif

#if defined(Q_CC_GNU) && !defined(__INSURE__)
#  if defined(Q_CC_MINGW) && !defined(Q_CC_CLANG)
#    define T_ATTRIBUTE_FORMAT(A,B)  __attribute__((format(gnu_printf,(A),(B))))
#  else
#    define T_ATTRIBUTE_FORMAT(A,B)  __attribute__((format(printf,(A),(B))))
#  endif
#else
#  define T_ATTRIBUTE_FORMAT(A,B)
#endif


#define T_EXPORT(VAR)  do { QVariant ___##VAR##_; ___##VAR##_.setValue(VAR); exportVariant(QLatin1String(#VAR), (___##VAR##_), true); } while(0)
#define texport(VAR)  T_EXPORT(VAR)

#define T_EXPORT_UNLESS(VAR)  do { QVariant ___##VAR##_; ___##VAR##_.setValue(VAR); exportVariant(QLatin1String(#VAR), (___##VAR##_), false); } while(0)
#define texportUnless(VAR)  T_EXPORT_UNLESS(VAR)

#define T_FETCH(TYPE,VAR)  TYPE VAR = variant(QLatin1String(#VAR)).value<TYPE>()
#define tfetch(TYPE,VAR)  T_FETCH(TYPE,VAR)

#define T_FETCH_V(TYPE,VAR,DEFAULT)  TYPE VAR = (hasVariant(QLatin1String(#VAR))) ? (variant(QLatin1String(#VAR)).value<TYPE>()) : (DEFAULT)
#define tfetchv(TYPE,VAR,DEFAULT)  T_FETCH_V(TYPE,VAR,DEFAULT)

#define T_EHEX(VAR)  eh(variant(QLatin1String(#VAR)))
#define tehex(VAR)  T_EHEX(VAR)

#define T_EHEX_V(VAR,DEFAULT) do { QString ___##VAR##_ = variant(QLatin1String(#VAR)).toString(); if (___##VAR##_.isEmpty()) eh(DEFAULT); else eh(___##VAR##_); } while(0)
#define tehexv(VAR,DEFAULT)  T_EHEX_V(VAR,DEFAULT)

// alias of tehexv
#define T_EHEX2(VAR,DEFAULT) do { QString ___##VAR##_ = variant(QLatin1String(#VAR)).toString(); if (___##VAR##_.isEmpty()) eh(DEFAULT); else eh(___##VAR##_); } while(0)
#define tehex2(VAR,DEFAULT)  T_EHEX2(VAR,DEFAULT)

#define T_ECHOEX(VAR)  echo(variant(QLatin1String(#VAR)))
#define techoex(VAR)  T_ECHOEX(VAR)

#define T_ECHOEX_V(VAR,DEFAULT) do { QString ___##VAR##_ = variant(QLatin1String(#VAR)).toString(); if (___##VAR##_.isEmpty()) echo(DEFAULT); else echo(___##VAR##_); } while(0)
#define techoexv(VAR,DEFAULT)  T_ECHOEX_V(VAR,DEFAULT)

// alias of techoexv
#define T_ECHOEX2(VAR,DEFAULT) do { QString ___##VAR##_ = variant(QLatin1String(#VAR)).toString(); if (___##VAR##_.isEmpty()) echo(DEFAULT); else echo(___##VAR##_); } while(0)
#define techoex2(VAR,DEFAULT)  T_ECHOEX2(VAR,DEFAULT)

#define T_FLASH(VAR)  do { QVariant ___##VAR##_; ___##VAR##_.setValue(VAR); setFlash(QLatin1String(#VAR), (___##VAR##_)); } while(0)
#define tflash(VAR)  T_FLASH(VAR)

#define T_VARIANT(VAR)  (variant(QLatin1String(#VAR)).toString())

//  Some classes do not permit copies and moves to be made of an object.
#define T_DISABLE_COPY(Class)                 \
    Class(const Class &) = delete;            \
    Class &operator=(const Class &) = delete;

#define T_DISABLE_MOVE(Class)                 \
    Class(Class &&) = delete;                 \
    Class &operator=(Class &&) = delete;


#define tFatal TDebug(Tf::FatalLevel).fatal
#define tError TDebug(Tf::ErrorLevel).error
#define tWarn  TDebug(Tf::WarnLevel).warn
#define tInfo  TDebug(Tf::InfoLevel).info
#define tDebug TDebug(Tf::DebugLevel).debug
#define tTrace TDebug(Tf::TraceLevel).trace


#include <TfNamespace>
#include <TDebug>
#include <TWebApplication>
#include "tfexception.h"
#include <cstdint>
#include <functional>

class TAppSettings;
class TActionContext;
class TDatabaseContext;
class QSqlDatabase;

namespace Tf
{
    T_CORE_EXPORT TWebApplication *app();
    T_CORE_EXPORT TAppSettings *appSettings();
    T_CORE_EXPORT const QVariantMap &conf(const QString &configName);
    T_CORE_EXPORT void msleep(unsigned long msecs);

    // Xorshift random number generator
    T_CORE_EXPORT void srandXor128(quint32 seed);  // obsolete
    T_CORE_EXPORT quint32 randXor128();            // obsolete

    // Thread-safe std::random number generator
    T_CORE_EXPORT uint32_t rand32_r();
    T_CORE_EXPORT uint64_t rand64_r();
    T_CORE_EXPORT uint64_t random(uint64_t min, uint64_t max);
    T_CORE_EXPORT uint64_t random(uint64_t max);

    T_CORE_EXPORT TActionContext *currentContext();
    T_CORE_EXPORT TDatabaseContext *currentDatabaseContext();
    T_CORE_EXPORT QSqlDatabase &currentSqlDatabase(int id);
    T_CORE_EXPORT QMap<QByteArray, std::function<QObject*()>> *objectFactories();

    static constexpr auto CRLFCRLF = "\x0d\x0a\x0d\x0a";
    static constexpr auto CRLF     = "\x0d\x0a";
    static constexpr auto LF       = "\x0a";
}

#endif // TGLOBAL_H
