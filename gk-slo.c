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
// gk-slo.c: Main cmd-line program for converting coordinates
//
// Compile on Unix:
//   gcc: gcc -O2 -Wall gk-slo.c util.c geo.c -o gk-slo -lm -lrt
// Compile on Windows:
//   MinGW: gcc -O2 -Wall gk-slo.c util.c geo.c -o gk-slo.exe
//   MS C:  cl /O2 /Wall gk-slo.c util.c geo.c
// Compile for profiler (gprof/AQTime):
//   gcc: gcc -g2 -gstabs+ -Wall gk-slo.c util.c geo.c -o gk-slo.exe -pg
//       (run gk-slo.exe --> creates file gmon.out)
//        gprof gk-slo.exe gmon.out > gprof.txt
//   MS C: cl /Zi /DEBUG /Wall gk-slo.c util.c geo.c
//
#include "common.h"
#include "geo.h"

#define SW_VERSION T("6.03")
#define SW_BUILD   T("Jan 11, 2015")

// global variables
TCHAR *prog;  // program name
int debug;

extern int gid_wgs; // selected geoid on WGS 84 (via cmd line)
extern int hsel;    // selected output height (via cmd line)

// ----------------------------------------------------------------------------
// reftest
// ----------------------------------------------------------------------------
void reftest()
{
  GEOGRA flref; GEOUTM d96ref, d48ref;
  GEOGRA fl; GEOUTM xy;
  DMS lat, lon;

//Lokacija: Zgonji Lehen na Pohorju
//WGS84: fi: 46.5375852874    la: 15.3015019823    h: 0.0
//       fi: 46 32 15.307035  la: 15 18 05.407136  h: 0.0
//SiTra ETRS89 -> D96/TM: x: 155370.642, y: 523125.803, H: -47.474
//SiTra ETRS89 -> D48/GK: x: 154885.259, y: 523494.788, H: -47.474

  flref.fi = 46.5375852874; flref.la = 15.3015019823; flref.h = 0.0;
  d96ref.x = 155370.642; d96ref.y = 523125.803; d96ref.H = -47.474;
  d48ref.x = 154885.259; d48ref.y = 523494.788; d48ref.H = -47.474;

  printf(T("---------- Reference point\n"));
  printf(T("WGS84 fi: %.10f,   la: %.10f,  h: %.3f\n"), flref.fi, flref.la, flref.h);
  deg2dms(flref.fi, &lat); deg2dms(flref.la, &lon);
  printf(T("     lat: %2.0f %2.0f %8.5f, lon: %2.0f %2.0f %8.5f, h: %.3f\n"),
         lat.deg, lat.min, lat.sec, lon.deg, lon.min, lon.sec, flref.h);
  printf(T("D96/TM x: %.3f, y: %.3f, H: %.3f\n"), d96ref.x, d96ref.y, d96ref.H);
  printf(T("D48/GK x: %.3f, y: %.3f, H: %.3f\n"), d48ref.x, d48ref.y, d48ref.H);


  printf(T("---------- Conversion D48/GK --> WGS84 (result)\n"));
  printf(T("<-- x: %.3f, y: %.3f, H: %.3f\n"), d48ref.x, d48ref.y, d48ref.H);
  gkxy2fila_wgs(d48ref, &fl);
  deg2dms(fl.fi, &lat); deg2dms(fl.la, &lon);
  printf(T("--> fi: %.10f,   la: %.10f,  h: %.3f\n"), fl.fi, fl.la, fl.h);
  printf(T("   lat: %2.0f %2.0f %8.5f, lon: %2.0f %2.0f %8.5f, h: %.3f\n"),
         lat.deg, lat.min, lat.sec, lon.deg, lon.min, lon.sec, fl.h);

  printf(T("---------- result --> D96/TM\n"));
  printf(T("<-- fi: %.10f, la: %.10f, h: %.3f\n"), fl.fi, fl.la, fl.h);
  fila_wgs2tmxy(fl, &xy);
  printf(T("--> x: %.3f, y: %.3f, H: %.3f\n"), xy.x, xy.y, xy.H);

  printf(T("---------- result --> D48/GK (inverse)\n"));
  printf(T("<-- fi: %.10f, la: %.10f, h: %.3f\n"), fl.fi, fl.la, fl.h);
  fila_wgs2gkxy(fl, &xy);
  printf(T("--> x: %.3f, y: %.3f, H: %.3f\n"), xy.x, xy.y, xy.H);

  printf(T("---------- Conversion WGS84 --> D48/GK (result)\n"));
  printf(T("<-- fi: %.10f, la: %.10f, h: %.3f\n"), flref.fi, flref.la, flref.h);
  fila_wgs2gkxy(flref, &xy);
  printf(T("--> x: %.3f, y: %.3f, H: %.3f\n"), xy.x, xy.y, xy.H);


  printf(T("---------- Conversion D96/TM --> WGS84 (result)\n"));
  printf(T("<-- x: %.3f, y: %.3f, H: %.3f\n"), d96ref.x, d96ref.y, d96ref.H);
  tmxy2fila_wgs(d96ref, &fl);
  deg2dms(fl.fi, &lat); deg2dms(fl.la, &lon);
  printf(T("--> fi: %.10f,   la: %.10f,  h: %.3f\n"), fl.fi, fl.la, fl.h);
  printf(T("   lat: %2.0f %2.0f %8.5f, lon: %2.0f %2.0f %8.5f, h: %.3f\n"),
         lat.deg, lat.min, lat.sec, lon.deg, lon.min, lon.sec, fl.h);

  printf(T("---------- result --> D48/GK\n"));
  printf(T("<-- fi: %.10f, la: %.10f, h: %.3f\n"), fl.fi, fl.la, fl.h);
  fila_wgs2gkxy(fl, &xy);
  printf(T("--> x: %.3f, y: %.3f, H: %.3f\n"), xy.x, xy.y, xy.H);

  printf(T("---------- result --> D96/TM (inverse)\n"));
  printf(T("<-- fi: %.10f, la: %.10f, h: %.3f\n"), fl.fi, fl.la, fl.h);
  fila_wgs2tmxy(fl, &xy);
  printf(T("--> x: %.3f, y: %.3f, H: %.3f\n"), xy.x, xy.y, xy.H);

  printf(T("---------- Conversion WGS84 --> D96/TM (result)\n"));
  printf(T("<-- fi: %.10f, la: %.10f, h: %.3f\n"), flref.fi, flref.la, flref.h);
  fila_wgs2tmxy(flref, &xy);
  printf(T("--> x: %.3f, y: %.3f, H: %.3f\n"), xy.x, xy.y, xy.H);


  printf(T("---------- Conversion D96/TM --> D48/GK (result)\n"));
  printf(T("<-- x: %.3f, y: %.3f, H: %.3f\n"), d96ref.x, d96ref.y, d96ref.H);
  tmxy2gkxy(d96ref, &xy);
  printf(T("--> x: %.3f, y: %.3f, H: %.3f\n"), xy.x, xy.y, xy.H);

  printf(T("---------- Conversion D48/GK --> D96/TM (result)\n"));
  printf(T("<-- x: %.3f, y: %.3f, H: %.3f\n"), d48ref.x, d48ref.y, d48ref.H);
  gkxy2tmxy(d48ref, &xy);
  printf(T("--> x: %.3f, y: %.3f, H: %.3f\n"), xy.x, xy.y, xy.H);

#if 1
  printf(T("---------- Conversion D48/GK --> BESSEL (loop)\n"));
  printf(T("<-- x: %.3f, y: %.3f, H: %.3f\n"), d48ref.x, d48ref.y, d48ref.H);
  xy2fila_ellips_loop(d48ref, &fl, 0);
  if (hsel == 1) fl.h = fl.h - fl.Ng;
  deg2dms(fl.fi, &lat); deg2dms(fl.la, &lon);
  printf(T("--> fi: %.10f,   la: %.10f,  h: %.3f (no geoid)\n"), fl.fi, fl.la, fl.h);
  printf(T("   lat: %2.0f %2.0f %8.5f, lon: %2.0f %2.0f %8.5f, h: %.3f (no geoid)\n"),
         lat.deg, lat.min, lat.sec, lon.deg, lon.min, lon.sec, fl.h);

  printf(T("---------- result --> D48/GK (loop, inverse)\n"));
  printf(T("<-- fi: %.10f, la: %.10f, h: %.3f (no geoid)\n"), fl.fi, fl.la, fl.h);
  fila_ellips2xy_loop(fl, &xy, 0);
  if (hsel == 1) xy.H = xy.H + xy.Ng;
  printf(T("--> x: %.3f, y: %.3f, H: %.3f (no geoid)\n"), xy.x, xy.y, xy.H);
#endif
#if 0
// Absolute geoid model limits
// fi: 45°15' - 47°00'
// la: 13°15' - 16°45'
  printf(T("---------- Conversion fi_min,la_min --> D96/TM\n"));
  lat.deg = 45; lat.min = 15; lat.sec = 0;
  lon.deg = 13; lon.min = 15; lon.sec = 0;
  dms2deg(lat, &fl.fi); dms2deg(lon, &fl.la); fl.h = 0.0;
  printf(T("<-- fi: %.10f, la: %.10f, h: %.3f\n"), fl.fi, fl.la, fl.h);
  fila_wgs2tmxy(fl, &xy);
  printf(T("--> x: %.3f, y: %.3f, H: %.3f\n"), xy.x, xy.y, xy.H);
  printf(T("---------- Conversion fi_max,la_max --> D96/TM\n"));
  lat.deg = 47; lat.min =  0; lat.sec = 0;
  lon.deg = 16; lon.min = 45; lon.sec = 0;
  dms2deg(lat, &fl.fi); dms2deg(lon, &fl.la); fl.h = 0.0;
  printf(T("<-- fi: %.10f, la: %.10f, h: %.3f\n"), fl.fi, fl.la, fl.h);
  fila_wgs2tmxy(fl, &xy);
  printf(T("--> x: %.3f, y: %.3f, H: %.3f\n"), xy.x, xy.y, xy.H);
  printf(T("---------- Conversion fi_min,la_min --> D48/GK\n"));
  lat.deg = 45; lat.min = 15; lat.sec = 0;
  lon.deg = 13; lon.min = 15; lon.sec = 0;
  dms2deg(lat, &fl.fi); dms2deg(lon, &fl.la); fl.h = 0.0;
  printf(T("<-- fi: %.10f, la: %.10f, h: %.3f\n"), fl.fi, fl.la, fl.h);
  fila_wgs2gkxy(fl, &xy);
  printf(T("--> x: %.3f, y: %.3f, H: %.3f\n"), xy.x, xy.y, xy.H);
  printf(T("---------- Conversion fi_max,la_max --> D48/GK\n"));
  lat.deg = 47; lat.min =  0; lat.sec = 0;
  lon.deg = 16; lon.min = 45; lon.sec = 0;
  dms2deg(lat, &fl.fi); dms2deg(lon, &fl.la); fl.h = 0.0;
  printf(T("<-- fi: %.10f, la: %.10f, h: %.3f\n"), fl.fi, fl.la, fl.h);
  fila_wgs2gkxy(fl, &xy);
  printf(T("--> x: %.3f, y: %.3f, H: %.3f\n"), xy.x, xy.y, xy.H);
#endif
#if 0
// Area completely inside Slovenia
  printf(T("---------- Conversion fi_min,la_min --> D96/TM\n"));
  fl.fi = 45.676000; fl.la = 13.920000; fl.h = 0.0;
  printf(T("<-- fi: %.10f, la: %.10f, h: %.3f\n"), fl.fi, fl.la, fl.h);
  fila_wgs2tmxy(fl, &xy);
  printf(T("--> x: %.3f, y: %.3f, H: %.3f\n"), xy.x, xy.y, xy.H);
  printf(T("---------- Conversion fi_max,la_max --> D96/TM\n"));
  fl.fi = 46.368000; fl.la = 15.235000; fl.h = 0.0;
  printf(T("<-- fi: %.10f, la: %.10f, h: %.3f\n"), fl.fi, fl.la, fl.h);
  fila_wgs2tmxy(fl, &xy);
  printf(T("--> x: %.3f, y: %.3f, H: %.3f\n"), xy.x, xy.y, xy.H);
  printf(T("---------- Conversion fi_min,la_min --> D48/GK\n"));
  fl.fi = 45.676000; fl.la = 13.920000; fl.h = 0.0;
  printf(T("<-- fi: %.10f, la: %.10f, h: %.3f\n"), fl.fi, fl.la, fl.h);
  fila_wgs2gkxy(fl, &xy);
  printf(T("--> x: %.3f, y: %.3f, H: %.3f\n"), xy.x, xy.y, xy.H);
  printf(T("---------- Conversion fi_max,la_max --> D48/GK\n"));
  fl.fi = 46.368000; fl.la = 15.235000; fl.h = 0.0;
  printf(T("<-- fi: %.10f, la: %.10f, h: %.3f\n"), fl.fi, fl.la, fl.h);
  fila_wgs2gkxy(fl, &xy);
  printf(T("--> x: %.3f, y: %.3f, H: %.3f\n"), xy.x, xy.y, xy.H);
#endif
} /* reftest */


// ----------------------------------------------------------------------------
// gendata_xy
// ----------------------------------------------------------------------------
void gendata_xy(int oid)
{
  double x_min, x_max, y_min, y_max;
  double x, y, x_inc, y_inc;
  int ii;

// Absolute geoid model limits
// fi: 45°15' - 47°00' (45.25° - 47.00°)
// la: 13°15' - 16°45' (13.25° - 16.75°)
// x:  13716.729 - 208212.911 (D96/TM)
// y: 362633.289 - 633083.271 (D96/TM)
// x:  13173.078 - 207659.160 (D48/GK)
// y: 363001.945 - 633454.830 (D48/GK)

// Old DMV model limits
// x:  28487.522 - 196483.316 (D96/TM)
// y: 373626.339 - 625633.019 (D96/TM)
// x:  28000.000 - 196000.000 (D48/GK)
// y: 374000.000 - 626000.000 (D48/GK)

// New DMV model limits
// x:  31090.000 - 194005.000 (D96/TM)
// y: 374590.000 - 623945.000 (D96/TM)
// x:  30602.469 - 193521.666 (D48/GK)
// y: 374963.589 - 624312.057 (D48/GK)

// Area completely inside Slovenia
//dlat_min = 45.676000; dlat_max = 46.368000;
//dlon_min = 13.920000; dlon_max = 15.235000;
  if (oid == 1 || oid == 2) {
    x_min =  60135.256; x_max = 136504.073; // D96/TM
    y_min = 415861.202; y_max = 518081.006; // D96/TM
  }
  else {
    x_min =  59589.464; x_max = 135954.408; // D48/GK
    y_min = 416230.629; y_max = 518451.587; // D48/GK
  }
  x_inc = 1500; y_inc = 1500; // in m

  x = x_min;
  for (ii = 1; x < x_max; ) {
    x += x_inc;

    y = y_min;
    for ( ; y < y_max; ) {
      y += y_inc;

      if (x < x_max && y < y_max)
        printf(T("%07d %.3f %.3f 0.000\n"), ii++, x, y);
    } // for y
  } // for x
} /* gendata_xy */


// ----------------------------------------------------------------------------
// gendata_fila
// ----------------------------------------------------------------------------
void gendata_fila()
{
  DMS lat, lon;
  double dlat_min, dlat_max, dlon_min, dlon_max;
  double dlat, dlon, lat_inc, lon_inc;
  int ii;

// Absolute geoid model limits
// fi: 45°15' - 47°00' (45.25° - 47.00°)
// la: 13°15' - 16°45' (13.25° - 16.75°)
// x:  13716.729 - 208212.911 (D96/TM)
// y: 362633.289 - 633083.271 (D96/TM)
// x:  13173.078 - 207659.160 (D48/GK)
// y: 363001.945 - 633454.830 (D48/GK)

// Old DMV model limits
// x:  28487.522 - 196483.316 (D96/TM)
// y: 373626.339 - 625633.019 (D96/TM)
// x:  28000.000 - 196000.000 (D48/GK)
// y: 374000.000 - 626000.000 (D48/GK)

// New DMV model limits
// x:  31090.000 - 194005.000 (D96/TM)
// y: 374590.000 - 623945.000 (D96/TM)
// x:  30602.469 - 193521.666 (D48/GK)
// y: 374963.589 - 624312.057 (D48/GK)

// Area completely inside Slovenia
  dlat_min = 45.676000; dlat_max = 46.368000;
  dlon_min = 13.920000; dlon_max = 15.235000;
  lat_inc = 60; lon_inc = 60; // in seconds

  deg2dms(dlat_min, &lat);
//lat.deg = 45; lat.min = 15; lat.sec = 0;
  dms2deg(lat, &dlat);
  for (ii = 1; dlat < dlat_max; ) {
    lat.sec += lat_inc;
    if (lat.sec >= 60) {
      lat.sec = 0; lat.min++;
      if (lat.min >= 60) {
        lat.min = 0; lat.deg++;
        if (lat.deg >= 90) {
          lat.deg = 0;
        }
      }
    }

    deg2dms(dlon_min, &lon);
 // lon.deg = 13; lon.min = 15; lon.sec = 0;
    dms2deg(lon, &dlon);
    for ( ; dlon < dlon_max; ) {
      lon.sec += lon_inc;
      if (lon.sec >= 60) {
        lon.sec = 0; lon.min++;
        if (lon.min >= 60) {
          lon.min = 0; lon.deg++;
          if (lon.deg >= 180) {
            lon.deg = 0;
          }
        }
      }

      dms2deg(lat, &dlat); dms2deg(lon, &dlon);
      if (dlat < dlat_max && dlon < dlon_max)
        printf(T("%07d %.10f %.10f 0.000\n"), ii++, dlat, dlon);
    } // for lon
  } // for lat
} /* gendata_fila */


// ----------------------------------------------------------------------------
// fnfind
// ----------------------------------------------------------------------------
TCHAR *fnfind(TCHAR *fname, TCHAR *newname)
{
  TCHAR *s, name[MAXS+1], ext[MAXS+1];
  struct _stat fst;
  int ii;

  xstrncpy(name, fname, MAXS);
//if ((s = strrchr(name, T('.'))) != NULL) *s = T('\0');

  xstrncpy(newname, name, MAXS);
  xstrncat(newname, T(".out"), MAXS);
  for (ii = 0; ii <= 15; ii++) { // append 1..15 to output file name
    if (tstat(newname, &fst) < 0) break; // file not found
    snprintf(ext, MAXS, T(".out.%d"), ii+1);
    xstrncpy(newname, name, MAXS);
    xstrncat(newname, ext, MAXS);
  }
  if (ii > 15) {
    xstrncpy(newname, name, MAXS);
    xstrncat(newname, T(".out"), MAXS);
    return NULL;
  }
  return newname;
} /* fnfind */


// ----------------------------------------------------------------------------
// usage
// ----------------------------------------------------------------------------
void usage(TCHAR *prog, int ver_only)
{
  fprintf(stderr, T("%s %s  Copyright (c) 2014-2015 Matjaz Rihtar  (%s)\n"),
          prog, SW_VERSION, SW_BUILD);
  if (ver_only) return;
  fprintf(stderr, T("Usage: %s [<options>] [<inpname> ...]\n"), prog);
  fprintf(stderr, T("  -d                enable debug output\n"));
  fprintf(stderr, T("  -x                print reference test and exit\n"));
  fprintf(stderr, T("  -gd <n>           generate data (inside Slovenia) and exit\n"));
  fprintf(stderr, T("                    1: generate xy   (d96tm)  data\n"));
  fprintf(stderr, T("                    2: generate fila (etrs89) data\n"));
  fprintf(stderr, T("                    3: generate xy   (d48gk)  data\n"));
  fprintf(stderr, T("  -ht               calculate output height with 7-params Helmert trans.\n"));
  fprintf(stderr, T("  -hc               copy input height unchanged to output\n"));
  fprintf(stderr, T("  -hg               calculate output height from geoid model (default)\n"));
  fprintf(stderr, T("  -g slo|egm        select geoid model (Slo2000 or EGM2008)\n"));
  fprintf(stderr, T("                    default: Slo2000\n"));
  fprintf(stderr, T("  -dms              display fila in DMS format after height\n"));
  fprintf(stderr, T("  -t <n>            select transformation:\n"));
  fprintf(stderr, T("                    1: xy   (d96tm)  --> fila (etrs89), hg?, default\n"));
  fprintf(stderr, T("                    2: fila (etrs89) --> xy   (d96tm),  hg\n"));
  fprintf(stderr, T("                    3: xy   (d48gk)  --> fila (etrs89), ht\n"));
  fprintf(stderr, T("                    4: fila (etrs89) --> xy   (d48gk),  hg\n"));
  fprintf(stderr, T("                    5: xy   (d48gk)  --> xy   (d96tm),  hg(hc)\n"));
  fprintf(stderr, T("                    6: xy   (d96tm)  --> xy   (d48gk),  ht(hc)\n"));
  fprintf(stderr, T("  -r                reverse parsing order of xy/fila\n"));
  fprintf(stderr, T("                    (warning displayed if y < 200000)\n"));
  fprintf(stderr, T("  <inpname>         parse and convert input data from <inpname>\n"));
  fprintf(stderr, T("                    <inpname> \"-\" means stdin, use \"--\" before\n"));
  fprintf(stderr, T("  -o -|=|<outname>  write output data to:\n"));
  fprintf(stderr, T("                    -: stdout (default)\n"));
  fprintf(stderr, T("                    =: append \".out\" to each <inpname> and\n"));
  fprintf(stderr, T("                       write output to these separate files\n"));
  fprintf(stderr, T("                    <outname>: write all output to 1 file <outname>\n"));
  fprintf(stderr, T("\n"));
  fprintf(stderr, T("Typical input data format (SiTra):\n"));
  fprintf(stderr, T("[<label>]  <fi|x>  <la|y>  <h|H>\n"));
} /* usage */


// ----------------------------------------------------------------------------
// main (mingw32 unicode wrapper)
// ----------------------------------------------------------------------------
#ifdef __MINGW32__
extern int _CRT_glob;
extern
#ifdef __cplusplus
"C"
#endif
void __wgetmainargs(int*, TCHAR***, TCHAR***, int, int*);
int tmain(int argc, TCHAR *argv[]);

int main() {
  TCHAR **argv, **enpv;
  int argc, si = 0;

  __wgetmainargs(&argc, &argv, &enpv, _CRT_glob, &si);
  return tmain(argc, argv);
}
#endif


// ----------------------------------------------------------------------------
// tmain
// ----------------------------------------------------------------------------
int tmain(int argc, TCHAR *argv[])
{
  int ii, ac, opt, sts;
  TCHAR *s, *av[MAXC];
  TCHAR geoid[MAXS+1];
  int value, test, gd, tr, rev, warn, ddms;
  TCHAR inpname[MAXS+1], outname[MAXS+1];
  int inpf, outf;
  FILE *inp, *out;
  TCHAR line[MAXS+1], col1[MAXS+1];
  int ln, n;
  double fi, la, h, x, y, H;
  GEOGRA fl; GEOUTM xy, gkxy, tmxy;
  DMS lat, lon;
  double dlat, dlon, tmp;
  struct timespec start, stop;
  double tdif;

  // Get program name
  if ((prog = strrchr(argv[0], DIRSEP)) == NULL) prog = argv[0];
  else prog++;
  if ((s = strstr(prog, T(".exe"))) != NULL) *s = T('\0');
  if ((s = strstr(prog, T(".EXE"))) != NULL) *s = T('\0');

  // Default global flags
  debug = 0;   // no debug
  test = 0;    // no reference test
  gd = 0;      // no gendata
  tr = 1;      // default transformation: xy (d96tm) --> fila (etrs89)
  rev = 0;     // don't reverse xy/fila
  ddms = 0;    // don't display DMS
  geoid[0] = T('\0');
  gid_wgs = 1; // slo2000
  inpf = 1;    // stdin
  outf = 1;    // stdout
  outname[0] = T('\0');
  hsel = -1;   // no default height processing (use internal recommendations)

  // Parse command line
  ac = 0; opt = 1;
  for (ii = 1; ii < argc && ac < MAXC; ii++) {
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
        if (value < 1 || value > 6) goto usage;
        tr = value;
        continue;
      }
      else if (strcasecmp(argv[ii], T("-gd")) == 0) { // generate data
        ii++; if (ii >= argc) goto usage;
        if (strlen(argv[ii]) == 0) goto usage;
        errno = 0; value = strtol(argv[ii], &s, 10);
        if (errno || *s) goto usage;
        if (value < 1 || value > 3) goto usage;
        gd = value;
        continue;
      }
      else if (strcasecmp(argv[ii], T("-o")) == 0) { // output
        ii++; if (ii >= argc) goto usage;
        xstrncpy(outname, argv[ii], MAXS);
        if (strlen(outname) == 0) goto usage;
        if (strncasecmp(outname, T("-"), 1) == 0) outf = 1; // stdout
        else if (strncasecmp(outname, T("="), 1) == 0) outf = 2; // separate files
        else outf = 3; // specified file
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
      else if (strcasecmp(argv[ii], T("-dms")) == 0) { // display DMS
        ddms = 1;
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
          case T('x'): // test
            test = 1;
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
      fprintf(stderr, T("malloc(av): %s"), xstrerror()); exit(3);
    }
    xstrncpy(av[ac++], argv[ii], MAXS);
  } // for each argc
  av[ac] = NULL;

  ellipsoid_init();
  params_init();

  if (test) {
    reftest();
    exit(0);
  }
  else if (gd) {
    if (gd == 1) gendata_xy(1);
    else if (gd == 2) gendata_fila();
    else gendata_xy(0);
    exit(0);
  }

  if (ac == 0) goto usage;

  out = stdout;
  if (outf == 3) { // specified file
    out = fopen(outname, T("w"));
    if (out == NULL) {
      fprintf(stderr, T("%s: %s\n"), outname, xstrerror()); exit(2);
    }
  }

  for (ii = 0; ii < ac; ii++) {
    // Open (next) input file
    if (strcmp(av[ii], T("-")) == 0) {
      xstrncpy(inpname, T("<stdin>"), MAXS);
      inp = stdin; inpf = 1;
    }
    else {
      xstrncpy(inpname, av[ii], MAXS);
      inp = fopen(inpname, T("r")); inpf = 3;
    }
    if (inp == NULL) {
      fprintf(stderr, T("%s: %s\n"), inpname, xstrerror());
      continue;
    }

    // Open (next) output file
    if (outf == 2) { // separate files
      if (fnfind(av[ii], outname) == NULL) {
        fprintf(stderr, T("%s: file already exists\n"), outname);
        if (inpf == 3) fclose(inp);
        continue;
      }
      out = fopen(outname, T("w"));
      if (out == NULL) {
        fprintf(stderr, T("%s: %s\n"), outname, xstrerror());
        if (inpf == 3) fclose(inp);
        continue;
      }
    }

    if (debug) fprintf(stderr, T("Processing %s\n"), inpname);
    clock_gettime(CLOCK_REALTIME, &start);

    ln = 0; warn = 1;
    for ( ; ; ) {
      // Read (next) line
      s = fgets(line, MAXS, inp);
      if (ferror(inp)) {
        fprintf(stderr, T("%s: %s\n"), inpname, xstrerror());
        break;
      }
      if (feof(inp)) break;
      ln++;

      fi = 0.0; la = 0.0; h = 0.0; // to keep compiler happy
      x = 0.0; y = 0.0; H = 0.0;

      // Parse line
      s = xstrtrim(line);
      if (tr == 2 || tr == 4) { // etrs89
        n = sscanf(s, T("%10240s %lf %lf %lf"), col1, &fi, &la, &h);
        if (n != 4) {
          n = sscanf(s, T("%lf %lf %lf"), &fi, &la, &h);
          if (n != 3) n = 5;
          else col1[0] = T('\0');
        }
        else xstrncat(col1, T(" "), MAXS);
        if (n != 4 && n != 3) {
          fprintf(stderr, T("%s: line %d: %-.75s\n"), inpname, ln, line);
          continue;
        }

        if (rev) { tmp = fi; fi = la; la = tmp; }
        if (fi == 0.0 || la == 0.0) {
          fprintf(stderr, T("%s: line %d: %-.75s\n"), inpname, ln, line);
          continue;
        }
      }
      else { // tr == 1,3,5,6 // d96tm/d48gk
        n = sscanf(s, T("%10240s %lf %lf %lf"), col1, &x, &y, &H);
        if (n != 4) {
          n = sscanf(s, T("%lf %lf %lf"), &x, &y, &H);
          if (n != 3) n = 5;
          else col1[0] = T('\0');
        }
        else xstrncat(col1, T(" "), MAXS);
        if (n != 4 && n != 3) {
          fprintf(stderr, T("%s: line %d: %-.75s\n"), inpname, ln, line);
          continue;
        }

        if (rev) { tmp = x; x = y; y = tmp; }
        if (x == 0.0 || y == 0.0) {
          fprintf(stderr, T("%s: line %d: %-.75s\n"), inpname, ln, line);
          continue;
        }
        if (y < 200000.0) { 
          y += 500000.0;
          if (warn) { fprintf(stderr, T("%s: possibly reversed xy\n"), inpname); warn = 0; }
        }
      }

      // Convert coordinates
      if (tr == 1) { // xy (d96tm) --> fila (etrs89)
        xy.x = x; xy.y = y; xy.H = H;
        tmxy2fila_wgs(xy, &fl);
        fprintf(out, T("%s%.10f %.10f %.3f"), col1, fl.fi, fl.la, fl.h);
        if (ddms) {
          deg2dms(fl.fi, &lat); deg2dms(fl.la, &lon);
          fprintf(out, T(" %.0f %2.0f %8.5f %.0f %2.0f %8.5f\n"),
            lat.deg, lat.min, lat.sec, lon.deg, lon.min, lon.sec);
        }
        else fprintf(out, T("\n"));
      }

      else if (tr == 2) { // fila (etrs89) --> xy (d96tm)
        fl.fi = fi; fl.la = la; fl.h = h;
        fila_wgs2tmxy(fl, &xy);
        fprintf(out, T("%s%.3f %.3f %.3f\n"), col1, xy.x, xy.y, xy.H);
      }

      else if (tr == 3) { // xy (d48gk) --> fila (etrs89)
        // SiTra: h,H/H(tr), gk-slo: hsel = ht ==> OK
        // SiTra: h=0,H=0/H(tr)=h-N, gk-slo: hsel = ht ==> OK (isti input!)
        // SiTra: h=0,H=0/H(tr)=h-N, gk-slo: hsel = hg ==> 40-70cm razlike
        xy.x = x; xy.y = y; xy.H = H;
        gkxy2fila_wgs(xy, &fl);
        fprintf(out, T("%s%.10f %.10f %.3f"), col1, fl.fi, fl.la, fl.h);
        if (ddms) {
          deg2dms(fl.fi, &lat); deg2dms(fl.la, &lon);
          fprintf(out, T(" %.0f %2.0f %8.5f %.0f %2.0f %8.5f\n"),
            lat.deg, lat.min, lat.sec, lon.deg, lon.min, lon.sec);
        }
        else fprintf(out, T("\n"));
      }

      else if (tr == 4) { // fila (etrs89) --> xy (d48gk)
        // SiTra: h,H/H(tr), gk-slo: hsel = ht ==> OK
        // SiTra: h=0,H=0/H(tr)=h-N, gk-slo: hsel = hg ==> OK
        // SiTra: h=0,H=0/H(tr)=h-N, gk-slo: hsel = ht ==> 40-70cm razlike
        fl.fi = fi; fl.la = la; fl.h = h;
        fila_wgs2gkxy(fl, &xy);
        fprintf(out, T("%s%.3f %.3f %.3f\n"), col1, xy.x, xy.y, xy.H);
      }

      else if (tr == 5) { // xy (d48gk) --> xy (d96tm)
        xy.x = x; xy.y = y; xy.H = H;
        gkxy2tmxy(xy, &tmxy);
        fprintf(out, T("%s%.3f %.3f %.3f\n"), col1, tmxy.x, tmxy.y, tmxy.H);
      }

      else if (tr == 6) { // xy (d96tm) --> xy (d48gk)
        xy.x = x; xy.y = y; xy.H = H;
        tmxy2gkxy(xy, &gkxy);
        fprintf(out, T("%s%.3f %.3f %.3f\n"), col1, gkxy.x, gkxy.y, gkxy.H);
      }
    } // while !eof

    clock_gettime(CLOCK_REALTIME, &stop);
    tdif = (stop.tv_sec - start.tv_sec)
           + (double)(stop.tv_nsec - start.tv_nsec)/NANOSEC;
    if (debug) fprintf(stderr, T("Processing time: %f\n"), tdif);

    if (inpf == 3) fclose(inp);
    if (outf == 2) fclose(out);
  } // for ac

  if (outf == 3) fclose(out);

  return 0;
} /* main */
