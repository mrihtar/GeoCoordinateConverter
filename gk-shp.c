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

#define SW_VERSION "1.07"
#define SW_BUILD   "Oct 25, 2016"

// global variables
char *prog;  // program name
int debug;
int tr;      // transformation
int rev;     // reverse xy/fila

extern int gid_wgs; // selected geoid on WGS 84 (in geo.c, via cmd line)
extern int hsel;    // output height calculation (in geo.c, via cmd line)

#ifdef _WIN32
#ifdef __MINGW32__
extern int _CRT_glob;
#else
int _CRT_glob = 1; // expand the wildcards in cmd line params
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif
// External function prototypes
int convert_shp_file(char *inpurl, char *outurl, char *msg);
#ifdef __cplusplus
}
#endif

// ----------------------------------------------------------------------------
// usage
// ----------------------------------------------------------------------------
void usage(int ver_only)
{
  fprintf(stderr, "%s %s  Copyright (c) 2014-2016 Matjaz Rihtar  (%s)\n",
          prog, SW_VERSION, SW_BUILD);
  if (ver_only) return;
  fprintf(stderr, "Usage: %s [<options>] <inpname> <outname>\n", prog);
  fprintf(stderr, "  -d                enable debug output\n");
  fprintf(stderr, "  -ht               calculate output height with 7-params Helmert trans.\n");
  fprintf(stderr, "  -hc               copy input height unchanged to output\n");
  fprintf(stderr, "  -hg               calculate output height from geoid model (default)\n");
  fprintf(stderr, "  -g slo|egm        select geoid model (Slo2000 or EGM2008)\n");
  fprintf(stderr, "                    default: Slo2000\n");
  fprintf(stderr, "  -t <n>            select transformation:\n");
  fprintf(stderr, "                     1: xy   (d96tm)  --> fila (etrs89), hg?, default\n");
  fprintf(stderr, "                     2: fila (etrs89) --> xy   (d96tm),  hg\n");
  fprintf(stderr, "                     3: xy   (d48gk)  --> fila (etrs89), ht\n");
  fprintf(stderr, "                     4: fila (etrs89) --> xy   (d48gk),  hg\n");
  fprintf(stderr, "                     5: xy   (d48gk)  --> xy   (d96tm),  hg(hc)\n");
  fprintf(stderr, "                     6: xy   (d96tm)  --> xy   (d48gk),  ht(hc)\n");
  fprintf(stderr, "                     7: xy   (d48gk)  --> xy   (d96tm),  hc, affine trans.\n");
  fprintf(stderr, "                     8: xy   (d96tm)  --> xy   (d48gk),  hc, affine trans.\n");
  fprintf(stderr, "                     9: xy   (d48gk)  --> fila (etrs89), hg, affine trans.\n");
  fprintf(stderr, "                    10: fila (etrs89) --> xy   (d48gk),  hg, affine trans.\n");
  fprintf(stderr, "  -r                reverse parsing order of xy/fila\n");
  fprintf(stderr, "                    (warning is displayed if y < 200000 or la > 17.0)\n");
  fprintf(stderr, "  <inpname>         parse and convert input data from <inpname>\n");
  fprintf(stderr, "  <outname>         write output data to <outname>\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Input data format:\n");
  fprintf(stderr, "ESRI Shapefile (ArcGIS)\n");
} /* usage */


// ----------------------------------------------------------------------------
// main
// ----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
#ifdef _WIN32
  wchar_t **wargv, **wenv; int si = 0;
#endif
  int ii, ac, opt;
  char *s, *av[MAXC], *errtxt;
  char geoid[MAXS+1];
  int value, warn;
  char inpname[MAXS+1], outname[MAXS+1], prjname[MAXS+1];
  static GEOGRA ifl, ofl; static GEOUTM ixy, oxy;
  struct timespec start, stop;
  double tdif;
  SHPHandle iSHP, oSHP;
  DBFHandle iDBF, oDBF;
  int nShapeType, nEntities; //, nVertices, nParts;
  int nEntity, nVertex, nPart;
  double adfMinBound[4], adfMaxBound[4];
  SHPObject *psShape;
  char *pszPartType, *pszPlus;
  char *iTuple, *oTuple;
  FILE *out; char *proj;
  int nPercentBefore, nPercent;

#ifdef _WIN32
  __wgetmainargs(&argc, &wargv, &wenv, _CRT_glob, &si);
  s = wchar2utf8(wargv[0]); // convert to UTF-8
  if (s != NULL) argv[0] = s;
#endif

  // Get program name
  if ((prog = strrchr(argv[0], DIRSEP)) == NULL) prog = argv[0];
  else prog++;
  if ((s = strstr(prog, ".exe")) != NULL) *s = '\0';
  if ((s = strstr(prog, ".EXE")) != NULL) *s = '\0';

  // Default global flags
  debug = 0;   // no debug
  tr = 1;      // default transformation: xy (d96tm) --> fila (etrs89)
  rev = 0;     // don't reverse xy/fila
  geoid[0] = '\0';
  gid_wgs = 1; // slo2000
  hsel = -1;   // default height processing (use internal recommendations)

  // Parse command line
  ac = 0; opt = 1;
  for (ii = 1; ii < argc && ac < MAXC; ii++) {
#ifdef _WIN32
    s = wchar2utf8(wargv[ii]); // convert to UTF-8
    if (s != NULL) argv[ii] = s;
#endif
    if (opt && *argv[ii] == '-') {
      if (strcasecmp(argv[ii], "-g") == 0) { // geoid
        ii++; if (ii >= argc) goto usage;
        xstrncpy(geoid, argv[ii], MAXS);
        if (strlen(geoid) == 0) goto usage;
        if (strncasecmp(geoid, "slo", 1) == 0) gid_wgs = 1;      // slo2000
        else if (strncasecmp(geoid, "egm", 1) == 0) gid_wgs = 2; // egm2008
        else goto usage;
        continue;
      }
      else if (strcasecmp(argv[ii], "-t") == 0) { // transformation
        ii++; if (ii >= argc) goto usage;
        if (strlen(argv[ii]) == 0) goto usage;
        errno = 0; value = strtol(argv[ii], &s, 10);
        if (errno || *s) goto usage;
        if (value < 1 || value > 10) goto usage;
        tr = value;
        continue;
      }
      else if (strcasecmp(argv[ii], "-ht") == 0) { // transformed height
        hsel = 0;
        continue;
      }
      else if (strcasecmp(argv[ii], "-hc") == 0) { // copy height
        hsel = 1;
        continue;
      }
      else if (strcasecmp(argv[ii], "-hg") == 0) { // geoid height
        hsel = 2;
        continue;
      }
      else if (strcasecmp(argv[ii], "--") == 0) { // end of options
        opt = 0;
        continue;
      }
      s = argv[ii];
      while (*++s) {
        switch (*s) {
          case 'd': // debug
            debug++;
            break;
          case 'r': // reverse xy/fila
            rev = 1;
            break;
          case 'v': // version
            usage(1); // show version only
            exit(0);
            break;
          default: // usage
usage:      usage(0);
            exit(1);
        }
      }
      continue;
    } // if opt
    av[ac] = (char *)malloc(MAXS+1);
    if (av[ac] == NULL) {
      errtxt = xstrerror();
      if (errtxt != NULL) {
        fprintf(stderr, "malloc(av): %s\n", errtxt); free(errtxt);
      } else
        fprintf(stderr, "malloc(av): Can't allocate memory\n");
      exit(3);
    }
    xstrncpy(av[ac++], argv[ii], MAXS);
  } // for each argc
//av[ac] = NULL;

  // geo.c initialization
  ellipsoid_init();
  params_init();

  if (ac < 2) goto usage;

  if ((s = strrchr(av[0], '.')) != NULL) *s = '\0'; // clear current extension
  xstrncat(av[0], ".shp", MAXS); // look for <name>.shp

  convert_shp_file(av[0], av[1], NULL);

  return 0;
} /* main */
