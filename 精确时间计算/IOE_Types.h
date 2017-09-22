// IOE_Types.h

#ifndef __IOE_Types_H__
#define __IOE_Types_H__

//-----------------------------------------------------------------------------------
// ****** Operating system identification
//
// Try to use the most generic version of these defines as possible in order to achieve
// the simplest portable code. For example, instead of using #if (defined(IOE_OS_IPHONE) || defined(IOE_OS_MAC)),
// consider using #if defined(IOE_OS_APPLE).
//
// Type definitions exist for the following operating systems: (IOE_OS_x)
//
//    WIN32      - Win32 and Win64 (Windows XP and later) Does not include Microsoft phone and console platforms, despite that Microsoft's _WIN32 may be defined by the compiler for them.
//    WIN64      - Win64 (Windows XP and later)
//    MAC        - Mac OS X (may be defined in addition to BSD)
//    LINUX      - Linux
//    BSD        - BSD Unix
//    ANDROID    - Android (may be defined in addition to LINUX)
//    IPHONE     - iPhone
//    MS_MOBILE  - Microsoft mobile OS.
//
//  Meta platforms
//    MS        - Any OS by Microsoft (e.g. Win32, Win64, phone, console)
//    APPLE     - Any OS by Apple (e.g. iOS, OS X)
//    UNIX      - Linux, BSD, Mac OS X.
//    MOBILE    - iOS, Android, Microsoft phone
//    CONSOLE   - Console platforms.
//

#if (defined(__APPLE__) && (defined(__GNUC__) ||\
     defined(__xlC__) || defined(__xlc__))) || defined(__MACOS__)
#  if (defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) || defined(__IPHONE_OS_VERSION_MIN_REQUIRED))
#      if !defined(IOE_OS_IPHONE)
#        define IOE_OS_IPHONE
#      endif
#  else
#    if !defined(IOE_OS_MAC)
#      define IOE_OS_MAC
#    endif
#    if !defined(IOE_OS_DARWIN)
#      define IOE_OS_DARWIN
#    endif
#    if !defined(IOE_OS_BSD)
#      define IOE_OS_BSD
#    endif
#  endif
#elif (defined(WIN64) || defined(_WIN64) || defined(__WIN64__))
#  if !defined(IOE_OS_WIN64)
#      define IOE_OS_WIN64
#  endif
#  if !defined(IOE_OS_WIN32)
#      define IOE_OS_WIN32  //Can be a 32 bit Windows build or a WOW64 support for Win32.  In this case WOW64 support for Win32.
#  endif
#elif (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#  if !defined(IOE_OS_WIN32)
#      define IOE_OS_WIN32  //Can be a 32 bit Windows build or a WOW64 support for Win32.  In this case WOW64 support for Win32.
#  endif
#elif defined(ANDROID) || defined(__ANDROID__)
#  if !defined(IOE_OS_ANDROID)
#      define IOE_OS_ANDROID
#  endif
#  if !defined(IOE_OS_LINUX)
#      define IOE_OS_LINUX
#  endif
#elif defined(__linux__) || defined(__linux)
#  if !defined(IOE_OS_LINUX)
#      define IOE_OS_LINUX
#  endif
#elif defined(_BSD_) || defined(__FreeBSD__)
#  if !defined(IOE_OS_BSD)
#      define IOE_OS_BSD
#  endif
#else
#  if !defined(IOE_OS_OTHER)
#      define IOE_OS_OTHER
#  endif
#endif

#if !defined(IOE_OS_MS_MOBILE)
#   if (defined(_M_ARM) || defined(_M_IX86) || defined(_M_AMD64)) && !defined(IOE_OS_WIN32) && !defined(IOE_OS_CONSOLE)
#       define IOE_OS_MS_MOBILE
#   endif
#endif

#if !defined(IOE_OS_MS)
#   if defined(IOE_OS_WIN32) || defined(IOE_OS_WIN64) || defined(IOE_OS_MS_MOBILE)
#       define IOE_OS_MS
#   endif
#endif

#if !defined(IOE_OS_APPLE)
#   if defined(IOE_OS_MAC) || defined(IOE_OS_IPHONE)
#       define IOE_OS_APPLE
#   endif
#endif

#if !defined(IOE_OS_UNIX)
#   if defined(IOE_OS_ANDROID) || defined(IOE_OS_BSD) || defined(IOE_OS_LINUX) || defined(IOE_OS_MAC)
#       define IOE_OS_UNIX
#   endif
#endif

#if !defined(IOE_OS_MOBILE)
#   if defined(IOE_OS_ANDROID) || defined(IOE_OS_IPHONE) || defined(IOE_OS_MS_MOBILE)
#       define IOE_OS_MOBILE
#   endif
#endif

#if defined(IOE_OS_MS)
#define IOE_ASSERT(p)

#include <windows.h>
#include <tchar.h>

#endif

#endif // __IOE_Types_H__
