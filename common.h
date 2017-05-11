// GK - Converter between Gauss-Krueger/TM and WGS84 coordinates for Slovenia
// Copyright (c) 2014-2016 Matjaz Rihtar <matjaz@eunet.si>
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see http://www.gnu.org/licenses/
//
// common.h: Include file with common definitions for Windows and Unix
//           environment
//
#ifndef _COMMON_H_DEFINED
#define _COMMON_H_DEFINED

#ifdef _WIN32
//ifdef _MSC_VER = Microsoft Compiler
//ifdef __MINGW32__ = MinGW Compiler
//ifdef __MINGW64__ = MinGW-w64 64-bit Compiler
// 0x0501 = Windows XP / Windows Server 2003 (minimum supported version)
// 0x0502 = Windows XP SP2 / Windows Server 2003 SP1
// 0x0600 = Windows Vista / Windows Server 2008
// 0x0601 = Windows 7 / Windows Server 2008 R2
#undef WINVER
#undef _WIN32_WINNT
#undef _WIN32_WINDOWS
#define _WINVER        0x0502  // Windows XP SP2 & Windows Server 2003 SP1 or greater
#define _WIN32_WINNT   0x0502
#define _WIN32_WINDOWS 0x0502

// exclude MFC stuff from headers
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRA_LEAN
#define WIN32_EXTRA_LEAN

#define _CRT_NON_CONFORMING_SWPRINTFS
#ifndef __MINGW32__ // Microsoft C
#pragma warning (disable:4995)  // W3: name was marked as #pragma deprecated
#pragma warning (disable:4996)  // W3: deprecated declaration (_CRT_SECURE_NO_WARNINGS)
#pragma warning (disable:4711)  // W1: function selected for inline expansion
#pragma warning (disable:4710)  // W4: function not inlined
#pragma warning (disable:4127)  // W4: conditional expression is constant
#pragma warning (disable:4820)  // W4: bytes padding added
#pragma warning (disable:4668)  // W4: symbol not defined as a macro
#pragma warning (disable:4255)  // W4: no function prototype given
#pragma warning (disable:4100)  // W4: unreferenced formal parameter
#pragma warning (disable:4101)  // W3: unreferenced local variable
// fltk
#pragma warning (disable:4365)  // W4: conversion -> signed/unsigned mismatch
#pragma warning (disable:4242)  // W4: conversion -> possible loss of data
#pragma warning (disable:4244)  // W3,W4: conversion -> possible loss of data
#pragma warning (disable:4625)  // W4: copy constructor could not be generated
#pragma warning (disable:4626)  // W4: assignment operator could not be generated
#pragma warning (disable:4266)  // W4: hidden function -> no override available
// deelx
#pragma warning (disable:4505)  // W4: unreferenced local function removed
// Windows SDK
#pragma warning (disable:4917)  // W1: GUID can only be associated with a class
#pragma warning (disable:4987)  // W4: nonstandard extension used
#define _USE_MATH_DEFINES  // for M_PI
#else // MinGW32
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-result"
#pragma GCC diagnostic ignored "-Wwrite-strings"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif

#else //not _WIN32
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-result"
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <math.h>
// math.h includes mathcalls.h (on Unix), which defines y1()
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#endif

#ifdef _WIN32
#ifdef _WCHAR
// simple wchar redefinitions (UNICODE support)
#define T(x) L##x
#define TCHAR wchar_t
#define TSIZE 0xFFFF
#define DIRSEP T('\\')
#define DIRSEP_S T("\\")
#define PATHSEP T(';')
#define PATHSEP_S T(";")
#define tmain wmain
#define fgetc fgetwc
#define fgets fgetws
#define fopen _wfopen
#define fprintf fwprintf
#define getcwd _wgetcwd
#define isspace iswspace
#define printf wprintf
#define snprintf _snwprintf
#define sscanf swscanf
#define strcasecmp _wcsicmp
#define strcmp wcscmp
#define strftime wcsftime
#define strlen wcslen
#define strncasecmp _wcsnicmp
#define strrchr wcsrchr
#define strstr wcsstr
#define strtol wcstol
#define tstat _wstat
#define vsnprintf _vsnwprintf
#undef FormatMessage
#define FormatMessage FormatMessageW
#define LPTSTR LPWSTR

#else //not _WCHAR
#define T(x) x
#define TCHAR char
#define TSIZE 0xFF
#define DIRSEP T('\\')
#define DIRSEP_S T("\\")
#define PATHSEP T(';')
#define PATHSEP_S T(";")
#define tmain main
#define getcwd _getcwd
#define snprintf _snprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define tstat _stat
#endif

#else //not _WIN32
#define T(x) x
#define TCHAR char
#define TSIZE 0xFF
#define DIRSEP T('/')
#define DIRSEP_S T("/")
#define PATHSEP T(':')
#define PATHSEP_S T(":")
#define tmain main
#define _stat stat
#define tstat stat
#endif

#define MAXS 10240      // max string size (check conv.c for hard-coded value!)
#define MAXC 5000       // max number of cmd line params
#define MAXB 131072     // max buffer size (also socket buffer size)
#define MAXL (MAXB+128) // max log file entry size

#ifdef _WIN32
#define CLOCK_REALTIME  0
#define CLOCK_MONOTONIC 1

#ifdef _MSC_VER // Microsoft C is missing timespec, but not if pthreads.h is included
#ifndef HAVE_STRUCT_TIMESPEC
#define HAVE_STRUCT_TIMESPEC
struct timespec {
  time_t tv_sec;
//long   tv_usec; // microseconds
  long   tv_nsec; // nanoseconds
};
#endif
#endif
#endif
#define MICROSEC 1000000L
#define NANOSEC  1000000000L

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
double xtrunc(double d);
int xround(double d);
long long int xllround(double d);
double xfmin(double d1, double d2);
double xfmax(double d1, double d2);

TCHAR *xstrncpy(TCHAR *s1, const TCHAR *s2, size_t n);
TCHAR *xstrncat(TCHAR *s1, const TCHAR *s2, size_t n);
TCHAR *xstrtrim(TCHAR *str);
TCHAR *xstrtok_r(TCHAR *str, const TCHAR *delim, TCHAR **savep);
TCHAR *xstrsep(TCHAR **strp, const TCHAR *delim);
TCHAR *xstrstr(TCHAR *s1, size_t n1, const TCHAR *s2, size_t n2);
TCHAR *xstrerror(void);

int xprintf(FILE *out, TCHAR *fmt, ...);

int clock_gettime(int clk_id, struct timespec *tv);

TCHAR *fefind(TCHAR *fname, TCHAR *ext, TCHAR *newname);

TCHAR *search_path(const TCHAR *exe, TCHAR *path, int len);
TCHAR *locate_self(const TCHAR *argv0, TCHAR *path0, int len);
TCHAR *locate_exe(const TCHAR *exe, TCHAR *path, int len);

TCHAR *uri2path(const TCHAR *uri);

// Windows UTF-8 functions
#ifdef _WIN32
int __wgetmainargs(int *argc, wchar_t ***wargv, wchar_t ***wenv, int glob, int *si);

wchar_t *utf82wchar(const char *str);
char *wchar2utf8(const wchar_t *wstr);
FILE *utf8_fopen(const char *fname, const char *mode);
int utf8_stat(const char *fname, struct _stat *fst);

#else //not _WIN32
#define utf8_fopen fopen
#define utf8_stat stat
#endif

#ifdef __cplusplus
}
#endif

#endif //_COMMON_H_DEFINED
