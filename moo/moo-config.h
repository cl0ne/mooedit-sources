#pragma once

#if (defined(DEBUG) && ((!DEBUG) || !(MOO_DEBUG) || !(ENABLE_DEBUG))) || \
    (!defined(DEBUG) && (defined(MOO_DEBUG) || defined(ENABLE_DEBUG)))
#  error "DEBUG, MOO_DEBUG, and ENABLE_DEBUG must either be all defined to non-zero or all undefined"
#endif

#undef MOO_CL_GCC
#undef MOO_CL_MINGW
#undef MOO_CL_MSVC
#define MOO_GCC_CHECK_VERSION(maj,min) (0)

#if defined(_MSC_VER)
#  define MOO_CL_MSVC 1
#elif defined(__GNUC__)
#  define MOO_CL_GCC 1
#  ifdef __GNUC_MINOR__
#    undef MOO_GCC_CHECK_VERSION
#    define MOO_GCC_CHECK_VERSION(maj,min) (((__GNUC__ << 16) + __GNUC_MINOR__) >= (((maj) << 16) + (min)))
#  endif
#  if defined(__MINGW32__)
#    define MOO_CL_MINGW 1
#  endif
#endif

#undef MOO_OS_WIN32
#undef MOO_OS_WIN64
#undef MOO_OS_UNIX
#undef MOO_OS_DARWIN

#if defined(__APPLE__)
#  define MOO_OS_DARWIN 1
#endif

#if defined(_WIN32)
#  if !defined(UNICODE) || !defined(_UNICODE)
#    error "UNICODE and _UNICODE must be defined on windows"
#  endif
#  ifndef __WIN32__
#    error "__WIN32__ must be defined on windows"
#  endif
#  define MOO_OS_WIN32 1
#else
#  ifdef __WIN32__
#    error "__WIN32__ defined but _WIN32 is not"
#  endif
#endif
#if defined(_WIN64)
#  ifndef _WIN32
#    error "_WIN64 defined but _WIN32 is not"
#  endif
#  define MOO_OS_WIN64 1
#endif

#ifndef MOO_OS_WIN32
#  define MOO_OS_UNIX 1
#endif

#ifndef MOO_OS_WIN32
#  define MOO_CDECL
#  define MOO_STDCALL
#else
#  define MOO_CDECL __cdecl
#  define MOO_STDCALL __stdcall
#endif
