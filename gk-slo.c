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
// gk-slo.c: Main cmd-line program for converting coordinates from XYZ files
//
#include "common.h"
#include "geo.h"

#define SW_VERSION "9.05"
#define SW_BUILD   "Oct 25, 2016"

// global variables
char *prog; // program name
int debug;
int tr;      // transformation
int rev;     // reverse xy/fila
int wdms;    // write DMS

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
int convert_xyz_file(char *url, int outf, FILE *out, char *msg);
#ifdef __cplusplus
}
#endif

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

  printf("---------- Reference point\n");
  printf("WGS84 fi: %.10f,   la: %.10f,  h: %.3f\n", flref.fi, flref.la, flref.h);
  deg2dms(flref.fi, &lat); deg2dms(flref.la, &lon);
  printf("     lat: %2.0f %2.0f %8.5f, lon: %2.0f %2.0f %8.5f, h: %.3f\n",
         lat.deg, lat.min, lat.sec, lon.deg, lon.min, lon.sec, flref.h);
  printf("D96/TM x: %.3f, y: %.3f, H: %.3f\n", d96ref.x, d96ref.y, d96ref.H);
  printf("D48/GK x: %.3f, y: %.3f, H: %.3f\n", d48ref.x, d48ref.y, d48ref.H);


  printf("---------- Conversion D48/GK --> WGS84 (result)\n");
  printf("<-- x: %.3f, y: %.3f, H: %.3f\n", d48ref.x, d48ref.y, d48ref.H);
  gkxy2fila_wgs(d48ref, &fl);
  deg2dms(fl.fi, &lat); deg2dms(fl.la, &lon);
  printf("--> fi: %.10f,   la: %.10f,  h: %.3f\n", fl.fi, fl.la, fl.h);
  printf("   lat: %2.0f %2.0f %8.5f, lon: %2.0f %2.0f %8.5f, h: %.3f\n",
         lat.deg, lat.min, lat.sec, lon.deg, lon.min, lon.sec, fl.h);

  printf("---------- result --> D96/TM\n");
  printf("<-- fi: %.10f, la: %.10f, h: %.3f\n", fl.fi, fl.la, fl.h);
  fila_wgs2tmxy(fl, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);

  printf("---------- result --> D48/GK (inverse)\n");
  printf("<-- fi: %.10f, la: %.10f, h: %.3f\n", fl.fi, fl.la, fl.h);
  fila_wgs2gkxy(fl, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);

  printf("---------- Conversion WGS84 --> D48/GK (result)\n");
  printf("<-- fi: %.10f, la: %.10f, h: %.3f\n", flref.fi, flref.la, flref.h);
  fila_wgs2gkxy(flref, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);


  printf("---------- Conversion D96/TM --> WGS84 (result)\n");
  printf("<-- x: %.3f, y: %.3f, H: %.3f\n", d96ref.x, d96ref.y, d96ref.H);
  tmxy2fila_wgs(d96ref, &fl);
  deg2dms(fl.fi, &lat); deg2dms(fl.la, &lon);
  printf("--> fi: %.10f,   la: %.10f,  h: %.3f\n", fl.fi, fl.la, fl.h);
  printf("   lat: %2.0f %2.0f %8.5f, lon: %2.0f %2.0f %8.5f, h: %.3f\n",
         lat.deg, lat.min, lat.sec, lon.deg, lon.min, lon.sec, fl.h);

  printf("---------- result --> D48/GK\n");
  printf("<-- fi: %.10f, la: %.10f, h: %.3f\n", fl.fi, fl.la, fl.h);
  fila_wgs2gkxy(fl, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);

  printf("---------- result --> D96/TM (inverse)\n");
  printf("<-- fi: %.10f, la: %.10f, h: %.3f\n", fl.fi, fl.la, fl.h);
  fila_wgs2tmxy(fl, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);

  printf("---------- Conversion WGS84 --> D96/TM (result)\n");
  printf("<-- fi: %.10f, la: %.10f, h: %.3f\n", flref.fi, flref.la, flref.h);
  fila_wgs2tmxy(flref, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);


  printf("---------- Conversion D96/TM --> D48/GK (result)\n");
  printf("<-- x: %.3f, y: %.3f, H: %.3f\n", d96ref.x, d96ref.y, d96ref.H);
  tmxy2gkxy(d96ref, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);

  printf("---------- Conversion D96/TM --> D48/GK (affine trans.)\n");
  printf("<-- x: %.3f, y: %.3f, H: %.3f\n", d96ref.x, d96ref.y, d96ref.H);
  tmxy2gkxy_aft(d96ref, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);

  printf("---------- Conversion D48/GK --> D96/TM (result)\n");
  printf("<-- x: %.3f, y: %.3f, H: %.3f\n", d48ref.x, d48ref.y, d48ref.H);
  gkxy2tmxy(d48ref, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);

  printf("---------- Conversion D48/GK --> D96/TM (affine trans.)\n");
  printf("<-- x: %.3f, y: %.3f, H: %.3f\n", d48ref.x, d48ref.y, d48ref.H);
  gkxy2tmxy_aft(d48ref, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);

  printf("---------- Conversion D48/GK --> WGS84 (affine trans.)\n");
  printf("<-- x: %.3f, y: %.3f, H: %.3f\n", d48ref.x, d48ref.y, d48ref.H);
  gkxy2fila_wgs_aft(d48ref, &fl);
  deg2dms(fl.fi, &lat); deg2dms(fl.la, &lon);
  printf("--> fi: %.10f,   la: %.10f,  h: %.3f\n", fl.fi, fl.la, fl.h);
  printf("   lat: %2.0f %2.0f %8.5f, lon: %2.0f %2.0f %8.5f, h: %.3f\n",
         lat.deg, lat.min, lat.sec, lon.deg, lon.min, lon.sec, fl.h);

  printf("---------- Conversion WGS84 --> D48/GK (affine trans.)\n");
  printf("<-- fi: %.10f, la: %.10f, h: %.3f\n", flref.fi, flref.la, flref.h);
  fila_wgs2gkxy_aft(flref, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);

#if 1
  printf("---------- Conversion D48/GK --> BESSEL (loop)\n");
  printf("<-- x: %.3f, y: %.3f, H: %.3f\n", d48ref.x, d48ref.y, d48ref.H);
  xy2fila_ellips_loop(d48ref, &fl, 0);
  if (hsel == 1) fl.h = fl.h - fl.Ng;
  deg2dms(fl.fi, &lat); deg2dms(fl.la, &lon);
  printf("--> fi: %.10f,   la: %.10f,  h: %.3f (no geoid)\n", fl.fi, fl.la, fl.h);
  printf("   lat: %2.0f %2.0f %8.5f, lon: %2.0f %2.0f %8.5f, h: %.3f (no geoid)\n",
         lat.deg, lat.min, lat.sec, lon.deg, lon.min, lon.sec, fl.h);

  printf("---------- result --> D48/GK (loop, inverse)\n");
  printf("<-- fi: %.10f, la: %.10f, h: %.3f (no geoid)\n", fl.fi, fl.la, fl.h);
  fila_ellips2xy_loop(fl, &xy, 0);
  if (hsel == 1) xy.H = xy.H + xy.Ng;
  printf("--> x: %.3f, y: %.3f, H: %.3f (no geoid)\n", xy.x, xy.y, xy.H);
#endif
#if 0
// Absolute geoid model limits
// fi: 45°15' - 47°00'
// la: 13°15' - 16°45'
  printf("---------- Conversion fi_min,la_min --> D96/TM\n");
  lat.deg = 45; lat.min = 15; lat.sec = 0;
  lon.deg = 13; lon.min = 15; lon.sec = 0;
  dms2deg(lat, &fl.fi); dms2deg(lon, &fl.la); fl.h = 0.0;
  printf("<-- fi: %.10f, la: %.10f, h: %.3f\n", fl.fi, fl.la, fl.h);
  fila_wgs2tmxy(fl, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);
  printf("---------- Conversion fi_max,la_max --> D96/TM\n");
  lat.deg = 47; lat.min =  0; lat.sec = 0;
  lon.deg = 16; lon.min = 45; lon.sec = 0;
  dms2deg(lat, &fl.fi); dms2deg(lon, &fl.la); fl.h = 0.0;
  printf("<-- fi: %.10f, la: %.10f, h: %.3f\n", fl.fi, fl.la, fl.h);
  fila_wgs2tmxy(fl, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);
  printf("---------- Conversion fi_min,la_min --> D48/GK\n");
  lat.deg = 45; lat.min = 15; lat.sec = 0;
  lon.deg = 13; lon.min = 15; lon.sec = 0;
  dms2deg(lat, &fl.fi); dms2deg(lon, &fl.la); fl.h = 0.0;
  printf("<-- fi: %.10f, la: %.10f, h: %.3f\n", fl.fi, fl.la, fl.h);
  fila_wgs2gkxy(fl, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);
  printf("---------- Conversion fi_max,la_max --> D48/GK\n");
  lat.deg = 47; lat.min =  0; lat.sec = 0;
  lon.deg = 16; lon.min = 45; lon.sec = 0;
  dms2deg(lat, &fl.fi); dms2deg(lon, &fl.la); fl.h = 0.0;
  printf("<-- fi: %.10f, la: %.10f, h: %.3f\n", fl.fi, fl.la, fl.h);
  fila_wgs2gkxy(fl, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);
#endif
#if 0
// Area completely inside Slovenia
  printf("---------- Conversion fi_min,la_min --> D96/TM\n");
  fl.fi = 45.676000; fl.la = 13.920000; fl.h = 0.0;
  printf("<-- fi: %.10f, la: %.10f, h: %.3f\n", fl.fi, fl.la, fl.h);
  fila_wgs2tmxy(fl, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);
  printf("---------- Conversion fi_max,la_max --> D96/TM\n");
  fl.fi = 46.368000; fl.la = 15.235000; fl.h = 0.0;
  printf("<-- fi: %.10f, la: %.10f, h: %.3f\n", fl.fi, fl.la, fl.h);
  fila_wgs2tmxy(fl, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);
  printf("---------- Conversion fi_min,la_min --> D48/GK\n");
  fl.fi = 45.676000; fl.la = 13.920000; fl.h = 0.0;
  printf("<-- fi: %.10f, la: %.10f, h: %.3f\n", fl.fi, fl.la, fl.h);
  fila_wgs2gkxy(fl, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);
  printf("---------- Conversion fi_max,la_max --> D48/GK\n");
  fl.fi = 46.368000; fl.la = 15.235000; fl.h = 0.0;
  printf("<-- fi: %.10f, la: %.10f, h: %.3f\n", fl.fi, fl.la, fl.h);
  fila_wgs2gkxy(fl, &xy);
  printf("--> x: %.3f, y: %.3f, H: %.3f\n", xy.x, xy.y, xy.H);
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
        printf("%07d %.3f %.3f 0.000\n", ii++, x, y);
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
        printf("%07d %.10f %.10f 0.000\n", ii++, dlat, dlon);
    } // for lon
  } // for lat
} /* gendata_fila */


// ----------------------------------------------------------------------------
// usage
// ----------------------------------------------------------------------------
void usage(int ver_only)
{
  fprintf(stderr, "%s %s  Copyright (c) 2014-2016 Matjaz Rihtar  (%s)\n",
          prog, SW_VERSION, SW_BUILD);
  if (ver_only) return;
  fprintf(stderr, "Usage: %s [<options>] [<inpname> ...]\n", prog);
  fprintf(stderr, "  -d                enable debug output\n");
  fprintf(stderr, "  -x                print reference test and exit\n");
  fprintf(stderr, "  -gd <n>           generate data (inside Slovenia) and exit\n");
  fprintf(stderr, "                    1: generate xy   (d96tm)  data\n");
  fprintf(stderr, "                    2: generate fila (etrs89) data\n");
  fprintf(stderr, "                    3: generate xy   (d48gk)  data\n");
  fprintf(stderr, "  -ht               calculate output height with 7-params Helmert trans.\n");
  fprintf(stderr, "  -hc               copy input height unchanged to output\n");
  fprintf(stderr, "  -hg               calculate output height from geoid model (default)\n");
  fprintf(stderr, "  -g slo|egm        select geoid model (Slo2000 or EGM2008)\n");
  fprintf(stderr, "                    default: Slo2000\n");
  fprintf(stderr, "  -dms              display fila in DMS format after height\n");
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
  fprintf(stderr, "                    <inpname> \"-\" means stdin, use \"--\" before\n");
  fprintf(stderr, "  -o -|=|<outname>  write output data to:\n");
  fprintf(stderr, "                    -: stdout (default)\n");
  fprintf(stderr, "                    =: append \".out\" to each <inpname> and\n");
  fprintf(stderr, "                       write output to these separate files\n");
  fprintf(stderr, "                    <outname>: write all output to 1 file <outname>\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Typical input data format (SiTra .xyz or LIDAR .asc):\n");
  fprintf(stderr, "[<label> ]<fi|y> <la|x> <h|H>\n");
  fprintf(stderr, "[<label>;]<fi|y>;<la|x>;<h|H>\n");
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
  int value, test, gd;
  char outname[MAXS+1];
  int inpf, outf;
  FILE *out;

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
  test = 0;    // no reference test
  gd = 0;      // no gendata
  tr = 1;      // default transformation: xy (d96tm) --> fila (etrs89)
  rev = 0;     // don't reverse xy/fila
  wdms = 0;    // don't write DMS
  geoid[0] = '\0';
  gid_wgs = 1; // slo2000
  inpf = 1;    // stdin
  outf = 1;    // stdout
  outname[0] = '\0';
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
      else if (strcasecmp(argv[ii], "-gd") == 0) { // generate data
        ii++; if (ii >= argc) goto usage;
        if (strlen(argv[ii]) == 0) goto usage;
        errno = 0; value = strtol(argv[ii], &s, 10);
        if (errno || *s) goto usage;
        if (value < 1 || value > 3) goto usage;
        gd = value;
        continue;
      }
      else if (strcasecmp(argv[ii], "-o") == 0) { // output
        ii++; if (ii >= argc) goto usage;
        xstrncpy(outname, argv[ii], MAXS);
        if (strlen(outname) == 0) goto usage;
        if (strncasecmp(outname, "-", 1) == 0) outf = 1; // stdout
        else if (strncasecmp(outname, "=", 1) == 0) outf = 2; // separate files
        else outf = 3; // specified file
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
      else if (strcasecmp(argv[ii], "-dms") == 0) { // write DMS
        wdms = 1;
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
          case 'x': // test
            test = 1;
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
    out = fopen(outname, "w");
    if (out == NULL) {
      errtxt = xstrerror();
      if (errtxt != NULL) {
        fprintf(stderr, "%s: %s\n", outname, errtxt); free(errtxt);
      } else
        fprintf(stderr, "%s: Can't open output file for writing\n", outname);
      exit(2);
    }
  }

  for (ii = 0; ii < ac; ii++)
    convert_xyz_file(av[ii], outf, out, NULL);

  if (outf == 3) fclose(out);

  return 0;
} /* main */
