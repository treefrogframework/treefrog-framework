
#if defined(__i386__) && __GNUC__ >= 2
# define STACKTRACE_H "stacktrace_x86-inl.h"
#elif defined(__x86_64__) && __GNUC__ >= 2
# define STACKTRACE_H "stacktrace_x86_64-inl.h"
#elif (defined(__ppc__) || defined(__PPC__)) && __GNUC__ >= 2
# define STACKTRACE_H "stacktrace_powerpc-inl.h"
#endif

#if !defined(STACKTRACE_H) && defined(HAVE_EXECINFO_H)
# define STACKTRACE_H "stacktrace_generic-inl.h"
#endif

#if defined(STACKTRACE_H)
# define HAVE_STACKTRACE
#endif

#ifdef STACKTRACE_H
# include STACKTRACE_H
#endif
