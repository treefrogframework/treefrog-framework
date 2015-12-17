#ifndef TGLOBAL_H
#define TGLOBAL_H

#include <QtGlobal>
#include <QMetaType>
#include <TfNamespace>
#include <cstdint>

#define TF_VERSION_STR "1.10.0"
#define TF_VERSION_NUMBER 0x0101000
#define TF_SRC_REVISION 984


#define T_DECLARE_CONTROLLER(TYPE, NAME)  \
    typedef TYPE NAME;                    \
    Q_DECLARE_METATYPE(NAME)

#define T_REGISTER_CONTROLLER(TYPE) T_REGISTER_METATYPE(TYPE)
#define T_REGISTER_VIEW(TYPE) T_REGISTER_METATYPE(TYPE)
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

#define T_DEFINE_PROPERTY(TYPE, PROPERTY)                          \
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


#define T_EXPORT(VAR)  do { QVariant ___##VAR##_; ___##VAR##_.setValue(VAR); exportVariant(QLatin1String(#VAR), (___##VAR##_), true); } while(0)
#define texport(VAR)  T_EXPORT(VAR)

#define T_EXPORT_UNLESS(VAR)  do { QVariant ___##VAR##_; ___##VAR##_.setValue(VAR); exportVariant(QLatin1String(#VAR), (___##VAR##_), false); } while(0)
#define texportUnless(VAR)  T_EXPORT_UNLESS(VAR)

#define T_FETCH(TYPE,VAR)  TYPE VAR = variant(QLatin1String(#VAR)).value<TYPE>()
#define tfetch(TYPE,VAR)  T_FETCH(TYPE,VAR)

#define T_EHEX(VAR)  eh(variant(QLatin1String(#VAR)))
#define tehex(VAR)  T_EHEX(VAR)

#define T_EHEX2(VAR,DEFAULT) do { QString ___##VAR##_ = variant(QLatin1String(#VAR)).toString(); if (___##VAR##_.isEmpty()) eh(DEFAULT); else eh(___##VAR##_); } while(0)
#define tehex2(VAR,DEFAULT)  T_EHEX2(VAR,DEFAULT)

#define T_ECHOEX(VAR)  echo(variant(QLatin1String(#VAR)))
#define techoex(VAR)  T_ECHOEX(VAR)

#define T_ECHOEX2(VAR,DEFAULT) do { QString ___##VAR##_ = variant(QLatin1String(#VAR)).toString(); if (___##VAR##_.isEmpty()) echo(DEFAULT); else echo(___##VAR##_); } while(0)
#define techoex2(VAR,DEFAULT)  T_ECHOEX2(VAR,DEFAULT)

#define T_FLASH(VAR)  do { QVariant ___##VAR##_; ___##VAR##_.setValue(VAR); setFlash(QLatin1String(#VAR), (___##VAR##_)); } while(0)
#define tflash(VAR)  T_FLASH(VAR)

#define T_VARIANT(VAR)  (variant(QLatin1String(#VAR)).toString())


class TLogger;
class TLog;

T_CORE_EXPORT void tSetupAppLoggers();   // internal use
T_CORE_EXPORT void tReleaseAppLoggers(); // internal use

T_CORE_EXPORT void tFatal(const char *, ...) // output fatal message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

T_CORE_EXPORT void tError(const char *, ...) // output error message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

T_CORE_EXPORT void tWarn(const char *, ...) // output warn message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

T_CORE_EXPORT void tInfo(const char *, ...) // output info message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

T_CORE_EXPORT void tDebug(const char *, ...) // output debug message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

T_CORE_EXPORT void tTrace(const char *, ...) // output trace message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

class TAppSettings;
class TActionContext;
class TDatabaseContext;
class QSqlDatabase;

namespace Tf
{
    T_CORE_EXPORT TWebApplication *app();
    T_CORE_EXPORT TAppSettings *appSettings();
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
}

/*!
  The TCheckChange class is for internal use only.
*/
template <class T>
class T_CORE_EXPORT TCheckChange
{
public:
    TCheckChange(const T &t, const char *f, int l) : ref(t), param(t), file(f), line(l) { }
    ~TCheckChange() { if (ref != param) tFatal("Changed parameter! (%s:%d)", file, line); }

private:
    const T &ref;
    T param;
    const char *file;
    int line;
};


#if QT_VERSION < 0x050000
# define TF_SET_CODEC_FOR_TR(codec)  do { QTextCodec::setCodecForTr(codec); QTextCodec::setCodecForCStrings(codec); } while (0)
# ifndef  Q_DECL_OVERRIDE
#  define Q_DECL_OVERRIDE
# endif
#else
# define TF_SET_CODEC_FOR_TR(codec)
#endif

#ifndef TF_NO_DEBUG
#  define T_CHECK_NO_CHANGE(val, type)  TCheckChange<type> ___Check ## val ## type (val, __FILE__, __LINE__)
#else
#  define tDebug(fmt, ...)
#  define tTrace(fmt, ...)
#  define T_CHECK_NO_CHANGE(val, type)
#endif  // TF_NO_DEBUG

#include "tfnamespace.h"
#include "tfexception.h"

#endif // TGLOBAL_H
