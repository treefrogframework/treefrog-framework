#pragma once
#include <QtGlobal>
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


//  Some classes do not permit copies and moves to be made of an object.
#define T_DISABLE_COPY(Class)      \
    Class(const Class &) = delete; \
    Class &operator=(const Class &) = delete;

#define T_DISABLE_MOVE(Class) \
    Class(Class &&) = delete; \
    Class &operator=(Class &&) = delete;
