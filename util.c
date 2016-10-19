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
// util.c: Collection of utility functions for use in other parts of program
//
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
// xtrunc
// Round towards zero
// (trunc is missing from Microsoft C)
// ----------------------------------------------------------------------------
double xtrunc(double d)
{
  double r;

  if (d > 0) r = floor(d + 0.00000000000001);
  else r = ceil(d - 0.00000000000001);
  return r;
} /* xtrunc */


// ----------------------------------------------------------------------------
// xround
// Round half up if positive, round half down if negative (= round to nearest)
// (round is missing from Microsoft C)
// ----------------------------------------------------------------------------
int xround(double d)
{
  int i;

  if (d > 0) i = (int)floor(d + 0.50000000000001);
  else i = (int)ceil(d - 0.50000000000001);
  return i;
} /* xround */


// ----------------------------------------------------------------------------
// xllround
// Round half up if positive, round half down if negative (= round to nearest)
// (llround is missing from Microsoft C)
// ----------------------------------------------------------------------------
long long int xllround(double d)
{
  long long int i64;

  if (d > 0) i64 = (long long int)floor(d + 0.50000000000001);
  else i64 = (long long int)ceil(d - 0.50000000000001);
  return i64;
} /* xllround */


// ----------------------------------------------------------------------------
// xfmin
// Return the lesser of two doubles
// (fmin is missing from Microsoft C)
// ----------------------------------------------------------------------------
double xfmin(double d1, double d2)
{
  if (d1 < d2)
    return d1;
  else
    return d2;
} /* xfmin */


// ----------------------------------------------------------------------------
// xfmax
// Return the larger of two doubles
// (fmax is missing from Microsoft C)
// ----------------------------------------------------------------------------
double xfmax(double d1, double d2)
{
  if (d1 > d2)
    return d1;
  else
    return d2;
} /* xfmax */


// ----------------------------------------------------------------------------
// xstrncpy
// Copy string s2 into string s1 until s1 is full (n = size of s1)
// s1 needs size + 1 char for terminating null.
// ----------------------------------------------------------------------------
TCHAR *xstrncpy(TCHAR *s1, const TCHAR *s2, size_t n)
{
  size_t ii;

  if (s1 != NULL && s2 != NULL) {
    // Copy until length of s1 == n or s2 is empty
    for (ii = 0; ii < n && *s2; ii++)
      *s1++ = *s2++;
    *s1 = T('\0');
  }

  return s1;
} /* xstrncpy */


// ----------------------------------------------------------------------------
// xstrncat
// Appends string s2 to string s1 until s1 is full (n = size of s1)
// s1 needs size + 1 char for terminating null.
// ----------------------------------------------------------------------------
TCHAR *xstrncat(TCHAR *s1, const TCHAR *s2, size_t n)
{
  size_t ii;

  if (s1 != NULL && s2 != NULL) {
    for (ii = 0; ii < n && *s1; ii++) s1++;
    // Copy until length of s1 == n or s2 is empty
    for ( ; ii < n && *s2; ii++)
      *s1++ = *s2++;
    *s1 = T('\0');
  }

  return s1;
} /* xstrncat */


// ----------------------------------------------------------------------------
// xstrtrim
// Trims white spaces (defined by "isspace") from the end of str
// Returns pointer to beginning of non-white space in str
// ----------------------------------------------------------------------------
TCHAR *xstrtrim(TCHAR *str)
{
  TCHAR *beg;
  int ii;

  if (str == NULL) return NULL;

  // Trim white spaces at the end
  ii = strlen(str) - 1;
  while (ii >= 0 && isspace(str[ii]))
    str[ii--] = T('\0');

  // Trim white spaces at the beginning
  beg = str;
  while (isspace(*beg)) beg++;

  return beg;
} /* xstrtrim */


// ----------------------------------------------------------------------------
// in_delim
// Returns true if character is in delimiter list.
// ----------------------------------------------------------------------------
int in_delim(TCHAR c, const TCHAR *delim)
{
  int found;
  TCHAR *s;

//if (delim == NULL) return 0;

  found = 0;
  for (s = (TCHAR *)delim; *s && !found; s++) {
    if (c == *s) found = 1;
  }
  return found;
} /* in_delim */


// ----------------------------------------------------------------------------
// xstrtok_r
// Extracts token from string str. Token delimiters are specified in string
// delim. Consecutive delimiters are ignored.
// (strtok_r is missing from Microsoft C & MinGW)
// ----------------------------------------------------------------------------
TCHAR *xstrtok_r(TCHAR *str, const TCHAR *delim, TCHAR **savep)
{
  TCHAR *ret;
  int dc, tc;

  if (str == NULL) {
    if (savep == NULL) return NULL;
    else str = *savep;
    if (str == NULL) return NULL;
  }
  if (delim == NULL) return str; // check this!

  // first clear all delimiters
  for (dc = 1; *str && dc; ) {
    dc = 0;
    if (in_delim(*str, delim)) {
      *str++ = T('\0'); dc = 1;
    }
  }

  ret = str;

  // token is until next delimiter, which is also cleared
  tc = 0;
  for (dc = 0; *str && !dc; str++) {
    if (in_delim(*str, delim)) {
      *str = T('\0'); dc = 1;
    }
    else tc++;
  }
  if (tc == 0) ret = NULL;
  *savep = str;

  return ret;
} /* xstrtok_r */


// ----------------------------------------------------------------------------
// xstrsep
// Extracts token from string *strp. Token delimiters are specified in string
// delim. Consecutive delimiters return empty tokens.
// Empty string *strp returns empty token (token after last delimiter).
// (strsep is missing from Microsoft C)
// ----------------------------------------------------------------------------
TCHAR *xstrsep(TCHAR **strp, const TCHAR *delim)
{
  TCHAR *ret;
  int dc;

  if (strp == NULL || *strp == NULL) return NULL;
  if (delim == NULL) return *strp; // check this!

  ret = *strp;

  for (dc = 0; **strp && !dc; (*strp)++) {
    if (in_delim(**strp, delim)) {
      **strp = T('\0'); dc = 1;
    }
  }
  if (!dc) *strp = NULL; // last token

  return ret;
} /* xstrsep */


// ----------------------------------------------------------------------------
// xstrstr (Boyer-Moore-Horspool)
// Finds the first occurrence of string s2 in string s1 using fast (aver. O(N))
// Boyer-Moore-Horspool algorithm.
// ----------------------------------------------------------------------------
TCHAR *xstrstr(TCHAR *s1, size_t n1, const TCHAR *s2, size_t n2)
{
  size_t ii, last;
  size_t skip[TSIZE];

  if (s1 == NULL || s2 == NULL || n2 > n1) return NULL;
  if (n2 == 0) return s1;

  for (ii = 0; ii < TSIZE; ii++)
    skip[ii] = n2;

  last = n2 - 1;

  for (ii = 0; ii < last; ii++)
    skip[s2[ii] & TSIZE] = last - ii;

  while (n1 >= n2) {
    for (ii = last; s1[ii] == s2[ii]; ii--)
      if (ii == 0) return s1;

    n1 -= skip[s1[last] & TSIZE];
    s1 += skip[s1[last] & TSIZE];
  }

  return NULL;
} /* xstrstr */


// ----------------------------------------------------------------------------
// xstrerror
// Returns string describing last occured error. Trims white spaces and dot
// from the end. Returned string must be freed after use outside the function.
// ----------------------------------------------------------------------------
TCHAR *xstrerror(void)
{
  TCHAR *msg;
  int error, len;

#ifdef _WIN32
  error = GetLastError();
  msg = (TCHAR *)calloc(MAXS+1, sizeof(TCHAR));
  if (msg == NULL) return NULL;
  FormatMessage(
    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, error,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  // default language
    (LPTSTR)msg, MAXS, NULL);
#else //not _WIN32
  error = errno;
  msg = (TCHAR *)calloc(MAXS+1, sizeof(TCHAR));
  if (msg == NULL) return NULL;
  strerror_r(error, msg, MAXS);
#endif

  // Trim white spaces and dot from the end
  len = strlen(msg) - 1;
  while (len >= 0 && (isspace(msg[len]) || msg[len] == T('.')))
    msg[len--] = T('\0');

  return msg;
} /* xstrerror */


// ----------------------------------------------------------------------------
// xprintf
// Writes an entry in previously opened log file, prepended by time of entry.
// ----------------------------------------------------------------------------
int xprintf(FILE *log, TCHAR *fmt, ...)
{
  va_list ap;
  time_t now;
  struct tm *tmnow;
  TCHAR stime[MAXS+1];
  TCHAR *msg;

  now = time(NULL);
  tmnow = localtime(&now);
//strftime(stime, MAXS, T("%y-%m-%d %H:%M:%S"), tmnow);
  strftime(stime, MAXS, T("%H:%M:%S"), tmnow);

  msg = (TCHAR *)calloc(MAXL+1, sizeof(TCHAR));

  va_start(ap, fmt);
  vsnprintf(msg, MAXL, fmt, ap);
  va_end(ap);

  fprintf(log, T("%s %s"), stime, msg);
//fflush(NULL);

  free(msg);
  return 0;
} /* xprintf */


// ----------------------------------------------------------------------------
// fefind
// Tries to find first free file name with specified extension (adding numbers
// to the name until 15).
// ----------------------------------------------------------------------------
TCHAR *fefind(TCHAR *fname, TCHAR *ext, TCHAR *newname)
{
  TCHAR name[MAXS+1], newext[MAXS+1], *s;
  struct _stat fst;
  int ii;

  xstrncpy(name, fname, MAXS);
  if ((s = strrchr(name, T('.'))) != NULL) *s = T('\0');

  xstrncpy(newname, name, MAXS);
  xstrncat(newname, ext, MAXS);
  for (ii = 0; ii <= 15; ii++) { // append 1..15 to output file name
    if (utf8_stat(newname, &fst) < 0) break; // file not found
    snprintf(newext, MAXS, T("-%d%s"), ii+1, ext);
    xstrncpy(newname, name, MAXS);
    xstrncat(newname, newext, MAXS);
  }
  if (ii > 15) {
    xstrncpy(newname, name, MAXS);
    xstrncat(newname, ext, MAXS);
    return NULL;
  }
  return newname;
} /* fefind */


#ifdef _WIN32
// ----------------------------------------------------------------------------
// getFILETIMEoffset
// Get time offset since January 1, 1970 for conversion to Unix epoch.
// ----------------------------------------------------------------------------
LARGE_INTEGER getFILETIMEoffset()
{
  SYSTEMTIME s;
  FILETIME f;
  LARGE_INTEGER t;

  s.wYear = 1970;
  s.wMonth = 1;
  s.wDay = 1;
  s.wHour = 0;
  s.wMinute = 0;
  s.wSecond = 0;
  s.wMilliseconds = 0;
  SystemTimeToFileTime(&s, &f);
  t.QuadPart = f.dwHighDateTime;
  t.QuadPart <<= 32;
  t.QuadPart |= f.dwLowDateTime;
  return t;
} /* getFILETIMEoffset */


// ----------------------------------------------------------------------------
// clock_gettime
// Returns the time of the specified clock clk_id in nanoseconds precision.
// (clock_gettime is missing from Microsoft C)
// ----------------------------------------------------------------------------
int clock_gettime(int clk_id, struct timespec *tv)
{
  LARGE_INTEGER t;
  FILETIME f;
//double microseconds;
  double nanoseconds;
  static LARGE_INTEGER offset;
//static double frequencyToMicroseconds;
  static double frequencyToNanoseconds;
  static int initialized = 0;
  static int usePerformanceCounter = 0;

  if (!initialized) {
    LARGE_INTEGER performanceFrequency;
    initialized = 1;
    usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
    if (usePerformanceCounter) {
      QueryPerformanceCounter(&offset);
   // frequencyToMicroseconds = (double)performanceFrequency.QuadPart / MICROSEC;
      frequencyToNanoseconds = (double)performanceFrequency.QuadPart / NANOSEC;
    }
    else {
      offset = getFILETIMEoffset();
   // frequencyToMicroseconds = 10.0;
      frequencyToNanoseconds = 0.01;
    }
  }
  if (usePerformanceCounter) QueryPerformanceCounter(&t);
  else {
    GetSystemTimeAsFileTime(&f);
    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;
  }

  t.QuadPart -= offset.QuadPart;
//microseconds = (double)t.QuadPart / frequencyToMicroseconds;
  nanoseconds = (double)t.QuadPart / frequencyToNanoseconds;
//t.QuadPart = (LONGLONG)microseconds;
  t.QuadPart = (LONGLONG)nanoseconds;
//tv->tv_sec = t.QuadPart / MICROSEC;
  tv->tv_sec = t.QuadPart / NANOSEC;
//tv->tv_usec = t.QuadPart % MICROSEC;
  tv->tv_nsec = t.QuadPart % NANOSEC;

  return 0;
} /* clock_gettime */


// ----------------------------------------------------------------------------
// utf82wchar
// Converts UTF-8 encoded string to a wide character (UTF-16) string.
// ----------------------------------------------------------------------------
wchar_t *utf82wchar(const char *str)
{
  int wlen;
  wchar_t *wstr;

  wlen = MultiByteToWideChar(CP_UTF8, 0, str, -1, 0, 0);
  if (wlen == 0) return NULL;

  wstr = (wchar_t *)malloc(wlen * sizeof(wchar_t));
  if (wstr == NULL) return NULL;

  if (MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, wlen) == 0) {
    free(wstr);
    return NULL;
  }

  return wstr;
} /* utf82wchar */


// ----------------------------------------------------------------------------
// wchar2utf8
// Converts wide character (UTF-16) string to an UTF-8 encoded string.
// ----------------------------------------------------------------------------
char *wchar2utf8(const wchar_t *wstr)
{
  int len;
  char *str;

  len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, 0, 0, NULL, NULL);
  if (len == 0) return NULL;

  str = (char *)malloc(len);
  if (str == NULL) return NULL;

  if (WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL) == 0) {
    free(str);
    return NULL;
  }

  return str;
} /* wchar2utf8 */


#ifndef _WCHAR
// ----------------------------------------------------------------------------
// utf8_fopen
// Open UTF-8 encoded file name.
// ----------------------------------------------------------------------------
FILE *utf8_fopen(const char *fname, const char *mode)
{
  wchar_t *wfname, *wmode;
  FILE *file;
  int error;

  wfname = utf82wchar(fname);
  wmode = utf82wchar(mode);
  if (wfname == NULL || wmode == NULL) {
    if (wfname != NULL) free(wfname);
    if (wmode != NULL) free(wmode);
    SetLastError(ENOMEM);
    return NULL;
  }

  file = _wfopen(wfname, wmode);

  error = GetLastError();
  free(wfname);
  free(wmode);
  SetLastError(error);

  return file;
} /* utf8_fopen */


// ----------------------------------------------------------------------------
// utf8_stat
// Get status information for UTF-8 encoded file name.
// ----------------------------------------------------------------------------
int utf8_stat(const char *fname, struct _stat *fst)
{
  wchar_t *wfname;
  int rc, error;

  wfname = utf82wchar(fname);
  if (wfname == NULL) {
    SetLastError(ENOMEM);
    return -1;
  }

  rc = _wstat(wfname, fst);

  error = GetLastError();
  free(wfname);
  SetLastError(error);

  return rc;
} /* utf8_stat */
#endif
#endif //_WIN32

#ifdef __cplusplus
}
#endif
