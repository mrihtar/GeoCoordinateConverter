// GK - Converter between Gauss-Krueger/TM and WGS84 coordinates for Slovenia
// Copyright (c) 2014-2015 Matjaz Rihtar <matjaz@eunet.si>
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
#define _WIN32_WINNT 0x0501  // Windows XP & Windows Server 2003 or greater
#ifndef __MINGW32__
#pragma warning (disable:4996)  // disable POSIX warnings
#pragma warning (disable:4711)  // disable inline expansion warnings
#pragma warning (disable:4127)  // disable cond. expr. is constant warnings
#pragma warning (disable:4820)  // disable padding warnings
#pragma warning (disable:4668)  // disable not defined macro warnings
#pragma warning (disable:4255)  // disable no function prototype warnings
#pragma warning (disable:4100)  // disable unreferenced formal parameter
#pragma warning (disable:4101)  // disable unreferenced local variable
#define _USE_MATH_DEFINES  // for M_PI
#else
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

#else //not _WIN32
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef _WIN32
#define sleep Sleep
// simple wchar redefinitions (UNICODE support)
#define T(x) L##x
#define TCHAR wchar_t
#define TSIZE 0xFFFF
#define tmain wmain
#define DIRSEP T('\\')
#define DIRSEP_S T("\\")
#define strlen wcslen
#define isspace iswspace
#define printf wprintf
#define fprintf fwprintf
#define strftime wcsftime
#define snprintf _snwprintf
#define vsnprintf _vsnwprintf
#define strrchr wcsrchr
#define strstr wcsstr
#define strcmp wcscmp
#define strcasecmp _wcsicmp
#define strncasecmp _wcsnicmp
#define strtol wcstol
#define sscanf swscanf
#define fopen _wfopen
#define fgets fgetws
#define fgetc fgetwc
#define tstat _wstat

#else //not _WIN32
#define T(x) x
#define TCHAR char
#define TSIZE 0xFF
#define tmain main
#define DIRSEP T('/')
#define DIRSEP_S T("/")
#define _stat stat
#define tstat stat
#endif

#define MAXS 10240      // max string size
#define MAXC 5000       // max number of cmd line params
#define MAXB 131072     // max buffer size (also socket buffer size)
#define MAXL (MAXB+128) // max log file entry size

#ifdef _WIN32
#define CLOCK_REALTIME  0
#define CLOCK_MONOTONIC 1

struct timespec {
  time_t tv_sec;
//long   tv_usec; // microseconds
  long   tv_nsec; // nanoseconds
};
#endif
#define MICROSEC 1000000L
#define NANOSEC  1000000000L

// Forward declarations
double xtrunc(double d);
int xround(double d);
long long int xllround(double d);
TCHAR *xstrncpy(TCHAR *s1, const TCHAR *s2, size_t n);
TCHAR *xstrncat(TCHAR *s1, const TCHAR *s2, size_t n);
TCHAR *xstrtrim(TCHAR *str);
TCHAR *xstrsep(TCHAR **strp, const TCHAR *delim);
TCHAR *xstrstr(TCHAR *s1, size_t n1, const TCHAR *s2, size_t n2);
TCHAR *xstrerror(void);
int clock_gettime(int clk_id, struct timespec *tv);

#endif //_COMMON_H_DEFINED
