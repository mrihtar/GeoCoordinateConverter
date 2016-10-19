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
// gk-shp.c: Main cmd-line program for converting coordinates from shapefiles
//
#include "common.h"
#include "geo.h"
#include "shapefil.h"

#define SW_VERSION T("1.03")
#define SW_BUILD   T("Oct 6, 2016")

// global variables
TCHAR *prog;  // program name
int debug;
int tr;      // transformation
int rev;     // reverse xy/fila

extern int gid_wgs; // selected geoid on WGS 84 (in geo.c, via cmd line)
extern int hsel;    // output height calculation (in geo.c, via cmd line)

#ifdef __cplusplus
extern "C" {
#endif
// External function prototypes
int convert_shp_file(TCHAR *inpurl, TCHAR *outurl, TCHAR *msg);
#ifdef __cplusplus
}
#endif

// ----------------------------------------------------------------------------
// usage
// ----------------------------------------------------------------------------
void usage(TCHAR *prog, int ver_only)
{
  fprintf(stderr, T("%s %s  Copyright (c) 2014-2016 Matjaz Rihtar  (%s)\n"),
          prog, SW_VERSION, SW_BUILD);
  if (ver_only) return;
  fprintf(stderr, T("Usage: %s [<options>] <inpname> <outname>\n"), prog);
  fprintf(stderr, T("  -d                enable debug output\n"));
  fprintf(stderr, T("  -ht               calculate output height with 7-params Helmert trans.\n"));
  fprintf(stderr, T("  -hc               copy input height unchanged to output\n"));
  fprintf(stderr, T("  -hg               calculate output height from geoid model (default)\n"));
  fprintf(stderr, T("  -g slo|egm        select geoid model (Slo2000 or EGM2008)\n"));
  fprintf(stderr, T("                    default: Slo2000\n"));
  fprintf(stderr, T("  -t <n>            select transformation:\n"));
  fprintf(stderr, T("                     1: xy   (d96tm)  --> fila (etrs89), hg?, default\n"));
  fprintf(stderr, T("                     2: fila (etrs89) --> xy   (d96tm),  hg\n"));
  fprintf(stderr, T("                     3: xy   (d48gk)  --> fila (etrs89), ht\n"));
  fprintf(stderr, T("                     4: fila (etrs89) --> xy   (d48gk),  hg\n"));
  fprintf(stderr, T("                     5: xy   (d48gk)  --> xy   (d96tm),  hg(hc)\n"));
  fprintf(stderr, T("                     6: xy   (d96tm)  --> xy   (d48gk),  ht(hc)\n"));
  fprintf(stderr, T("                     7: xy   (d48gk)  --> xy   (d96tm),  hc, affine trans.\n"));
  fprintf(stderr, T("                     8: xy   (d96tm)  --> xy   (d48gk),  hc, affine trans.\n"));
  fprintf(stderr, T("                     9: xy   (d48gk)  --> fila (etrs89), hg, affine trans.\n"));
  fprintf(stderr, T("                    10: fila (etrs89) --> xy   (d48gk),  hg, affine trans.\n"));
  fprintf(stderr, T("  -r                reverse parsing order of xy/fila\n"));
  fprintf(stderr, T("                    (warning is displayed if y < 200000 or la > 17.0)\n"));
  fprintf(stderr, T("  <inpname>         parse and convert input data from <inpname>\n"));
  fprintf(stderr, T("  <outname>         write output data to <outname>\n"));
  fprintf(stderr, T("\n"));
  fprintf(stderr, T("Input data format:\n"));
  fprintf(stderr, T("ESRI Shapefile (ArcGIS)\n"));
} /* usage */


// ----------------------------------------------------------------------------
// mingw32 unicode wrapper
// ----------------------------------------------------------------------------
#ifdef __MINGW32__
extern int _CRT_glob;
#ifdef __cplusplus
extern "C"
#endif
void __wgetmainargs(int *, wchar_t ***, wchar_t ***, int, int *);
#endif


// ----------------------------------------------------------------------------
// tmain
// ----------------------------------------------------------------------------
int tmain(int argc, TCHAR *argv[])
{
#ifdef _WIN32
  wchar_t **wargv, **wenpv; int si = 0;
#endif
  int ii, ac, opt;
  TCHAR *s, *av[MAXC], *errtxt;
  TCHAR geoid[MAXS+1];
  int value, tr, rev, warn;
  TCHAR inpname[MAXS+1], outname[MAXS+1], prjname[MAXS+1];
  static GEOGRA ifl, ofl; static GEOUTM ixy, oxy;
  struct timespec start, stop;
  double tdif;
  SHPHandle iSHP, oSHP;
  DBFHandle iDBF, oDBF;
  int nShapeType, nEntities; //, nVertices, nParts;
  int nEntity, nVertex, nPart;
  double adfMinBound[4], adfMaxBound[4];
  SHPObject *psShape;
  TCHAR *pszPartType, *pszPlus;
  TCHAR *iTuple, *oTuple;
  FILE *out; TCHAR *proj;
  int nPercentBefore, nPercent;

#ifdef _WIN32
  __wgetmainargs(&argc, &wargv, &wenpv, _CRT_glob, &si);
  argv[0] = wchar2utf8(wargv[0]); // convert to UTF-8
#endif

  // Get program name
  if ((prog = strrchr(argv[0], DIRSEP)) == NULL) prog = argv[0];
  else prog++;
  if ((s = strstr(prog, T(".exe"))) != NULL) *s = T('\0');
  if ((s = strstr(prog, T(".EXE"))) != NULL) *s = T('\0');

  // Default global flags
  debug = 0;   // no debug
  tr = 1;      // default transformation: xy (d96tm) --> fila (etrs89)
  rev = 0;     // don't reverse xy/fila
  geoid[0] = T('\0');
  gid_wgs = 1; // slo2000
  hsel = -1;   // no default height processing (use internal recommendations)

  // Parse command line
  ac = 0; opt = 1;
  for (ii = 1; ii < argc && ac < MAXC; ii++) {
#ifdef _WIN32
    argv[ii] = wchar2utf8(wargv[ii]); // convert to UTF-8
#endif
    if (opt && *argv[ii] == T('-')) {
      if (strcasecmp(argv[ii], T("-g")) == 0) { // geoid
        ii++; if (ii >= argc) goto usage;
        xstrncpy(geoid, argv[ii], MAXS);
        if (strlen(geoid) == 0) goto usage;
        if (strncasecmp(geoid, T("slo"), 1) == 0) gid_wgs = 1;      // slo2000
        else if (strncasecmp(geoid, T("egm"), 1) == 0) gid_wgs = 2; // egm2008
        else goto usage;
        continue;
      }
      else if (strcasecmp(argv[ii], T("-t")) == 0) { // transformation
        ii++; if (ii >= argc) goto usage;
        if (strlen(argv[ii]) == 0) goto usage;
        errno = 0; value = strtol(argv[ii], &s, 10);
        if (errno || *s) goto usage;
        if (value < 1 || value > 10) goto usage;
        tr = value;
        continue;
      }
      else if (strcasecmp(argv[ii], T("-ht")) == 0) { // transformed height
        hsel = 0;
        continue;
      }
      else if (strcasecmp(argv[ii], T("-hc")) == 0) { // copy height
        hsel = 1;
        continue;
      }
      else if (strcasecmp(argv[ii], T("-hg")) == 0) { // geoid height
        hsel = 2;
        continue;
      }
      else if (strcasecmp(argv[ii], T("--")) == 0) { // end of options
        opt = 0;
        continue;
      }
      s = argv[ii];
      while (*++s) {
        switch (*s) {
          case T('d'): // debug
            debug++;
            break;
          case T('r'): // reverse xy/fila
            rev = 1;
            break;
          case T('v'): // version
            usage(prog, 1); // show version only
            exit(0);
            break;
          default: // usage
usage:      usage(prog, 0);
            exit(1);
        }
      }
      continue;
    } // if opt
    av[ac] = (TCHAR *)malloc(MAXS+1);
    if (av[ac] == NULL) {
      errtxt = xstrerror();
      if (errtxt != NULL) {
        fprintf(stderr, T("malloc(av): %s\n"), errtxt); free(errtxt);
      } else
        fprintf(stderr, T("malloc(av): Unknown error\n"));
      exit(3);
    }
    xstrncpy(av[ac++], argv[ii], MAXS);
  } // for each argc
  av[ac] = NULL;

  ellipsoid_init();
  params_init();

  if (ac < 2) goto usage;

  if ((s = strrchr(av[0], T('.'))) != NULL) *s = T('\0'); // clear current extension
  xstrncat(av[0], T(".shp"), MAXS); // look for <name>.shp

  convert_shp_file(av[0], av[1], NULL);

  return 0;
} /* main */
