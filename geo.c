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
// geo.c: Collection of coordinate conversion routines
//
#include "common.h"
#include "geo.h"

// Select meridian arc length (L) calculation algorithm
#define L1 //else L2
#undef  L3 // alternative

// Select fi0 calculation algorithm (geocentric to geodetic coordinates)
#define FI1 //else FI2

// Select Helmert transformation method
#define H71 // best
#undef  H72
#undef  H73
//else

// global variables
const double PI = M_PI; //= 4.0*atan(1.0);

// Ellipsoid data
// 0: Bessel 1841, 1: WGS 84, 2: ETRS89
ELLIPSOID ellips[3];
#define ellipsoid ellips[oid]

// Absolute geoid model of Slovenia
// fi: 45°15' - 47°00' = 105'/1.0' = 105
//     45.25° - 47.00°
// x:  13716.729 - 208212.911 (D96/TM)
// x:  13173.078 - 207659.160 (D48/GK)
// la: 13°15' - 16°45' = 210'/1.5' = 140
//     13.25° - 16.75°
// y: 362633.289 - 633083.271 (D96/TM)
// y: 363001.945 - 633454.830 (D48/GK)
// la: 13°15' - 16°45' = 210'/1.0' = 210
double geoid_bes[106][141];   // (doesn't exist) on Bessel 1841 (0)
//double geoid_slo[106][141]; // Slo2000 on WGS 84 (1)
#include "geoid_slo.h"
//double geoid_egm[106][211]; // EGM2008 on WGS 84 (2)
#include "geoid_egm.h"
double gfimin, gfimax, gfiinc1;
double glamin, glamax, glainc15, glainc1;

int gid_wgs; // selected geoid on WGS 84 (via cmd line)
int hsel;    // selected output height (via cmd line)
// transformed height(0), copied height(1) or geoid height(2)

// H = ortometric/above sea level height (what we normally use)
// h = elipsoidal height (from GPS)
// Ng = geoid height (from EGM96, EGM2008 or local absolute models)
// H = h - Ng
// h = H + Ng
// Ng for Slovenia:
//   0-5m for Bessel 1841
//   45-48m for WGS 84 (EGM96)

// German expressions:
// Breite = Latitude (N/S)
// Laenge = Longitude (E/W)
// Kennziffer = GK zone = {0°, 3°, 6°, ..., 351°, 354°, 357°} / 3°
// zone = xround(lon.deg / 3.0);

// Gauss-Krueger zones and their limits
GKLM gkzones[] = {
//  fimin fimax lamin lamax xmin xmax ymin ymax
  { 40.0, 55.0, -2.0,  2.0, 0.0, 0.0, 0.0, 0.0 },  // 0
  { 40.0, 55.0,  1.0,  5.0, 0.0, 0.0, 0.0, 0.0 },  // 1
  { 40.0, 55.0,  4.0,  8.0, 0.0, 0.0, 0.0, 0.0 },  // 2
  { 40.0, 55.0,  7.0, 11.0, 0.0, 0.0, 0.0, 0.0 },  // 3
  { 40.0, 55.0, 10.0, 14.0, 0.0, 0.0, 0.0, 0.0 },  // 4
  { 40.0, 55.0, 13.0, 17.0, -569400.0, 1097800.0, 329200.0, 628000.0 },  // 5 (Slovenia)
  { 40.0, 55.0, 16.0, 20.0, 0.0, 0.0, 0.0, 0.0 },  // 6
  { 40.0, 55.0, 19.0, 23.0, 0.0, 0.0, 0.0, 0.0 },  // 7
  { 40.0, 55.0, 22.0, 26.0, 0.0, 0.0, 0.0, 0.0 }   // 8
};

// Pre-calculated affine transformation tables
#define MAXAFT 1776
//AFT aft_gktm[MAXAFT];  // Affine transformation table from GK to TM for Slovenia
#include "aft_gktm.h"
//AFT aft_tmgk[MAXAFT];  // Affine transformation table from TM to GK for Slovenia
#include "aft_tmgk.h"
int last_tri = -1; // last found triangle

// Distance to triangle segment
#define EPSILON  0.001
#define EPSILON2 EPSILON*EPSILON

// Projection parameters
PROJ tm;

// Parameters for Helmert transformation
HELMERT7 slo7, slo7inv;

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
// dms2deg
// ----------------------------------------------------------------------------
void dms2deg(DMS dms, double *deg)
{
  *deg = dms.deg + (dms.min*60.0 + dms.sec)/3600.0;
} /* dms2deg */

// ----------------------------------------------------------------------------
// dm2deg
// ----------------------------------------------------------------------------
void dm2deg(DMS dms, double *deg)
{
  *deg = dms.deg + dms.min*60.0/3600.0;
} /* dm2deg */

// ----------------------------------------------------------------------------
// dms2rad
// ----------------------------------------------------------------------------
void dms2rad(DMS dms, double *rad)
{
  *rad = dms.deg + (dms.min*60.0 + dms.sec)/3600.0;
  *rad = *rad*PI/180.0; // convert degrees to radians
} /* dms2rad */

// ----------------------------------------------------------------------------
// dm2rad
// ----------------------------------------------------------------------------
void dm2rad(DMS dms, double *rad)
{
  *rad = dms.deg + dms.min*60.0/3600.0;
  *rad = *rad*PI/180.0; // convert degrees to radians
} /* dm2rad */

// ----------------------------------------------------------------------------
// deg2dms
// ----------------------------------------------------------------------------
void deg2dms(double deg, DMS *dms)
{
  dms->deg = xtrunc(deg);
  dms->sec = (deg - dms->deg)*60.0;
  dms->min = xtrunc(dms->sec);
  dms->sec = (dms->sec - dms->min)*60.0;
} /* deg2dms */

// ----------------------------------------------------------------------------
// deg2dm
// ----------------------------------------------------------------------------
void deg2dm(double deg, DMS *dms)
{
  dms->deg = xtrunc(deg);
  dms->min = (deg - dms->deg)*60.0;
  dms->sec = 0.0;
} /* deg2dm */

// ----------------------------------------------------------------------------
// rad2dms
// ----------------------------------------------------------------------------
void rad2dms(double rad, DMS *dms)
{
  rad = rad*180.0/PI; // convert radians to degrees
  dms->deg = xtrunc(rad);
  dms->sec = (rad - dms->deg)*60.0;
  dms->min = xtrunc(dms->sec);
  dms->sec = (dms->sec - dms->min)*60.0;
} /* rad2dms */

// ----------------------------------------------------------------------------
// rad2dm
// ----------------------------------------------------------------------------
void rad2dm(double rad, DMS *dms)
{
  rad = rad*180.0/PI; // convert radians to degrees
  dms->deg = xtrunc(rad);
  dms->min = (rad - dms->deg)*60.0;
  dms->sec = 0.0;
} /* rad2dm */


// ----------------------------------------------------------------------------
// matrix33_mul
// ----------------------------------------------------------------------------
void matrix33_mul(m33 R1, m33 R2, m33 *R12)
{
  (*R12)[0][0] = R1[0][0]*R2[0][0] + R1[0][1]*R2[1][0] + R1[0][2]*R2[2][0];
  (*R12)[0][1] = R1[0][0]*R2[0][1] + R1[0][1]*R2[1][1] + R1[0][2]*R2[2][1];
  (*R12)[0][2] = R1[0][0]*R2[0][2] + R1[0][1]*R2[1][2] + R1[0][2]*R2[2][2];

  (*R12)[1][0] = R1[1][0]*R2[0][0] + R1[1][1]*R2[1][0] + R1[1][2]*R2[2][0];
  (*R12)[1][1] = R1[1][0]*R2[0][1] + R1[1][1]*R2[1][1] + R1[1][2]*R2[2][1];
  (*R12)[1][2] = R1[1][0]*R2[0][2] + R1[1][1]*R2[1][2] + R1[1][2]*R2[2][2];

  (*R12)[2][0] = R1[2][0]*R2[0][0] + R1[2][1]*R2[1][0] + R1[2][2]*R2[2][0];
  (*R12)[2][1] = R1[2][0]*R2[0][1] + R1[2][1]*R2[1][1] + R1[2][2]*R2[2][1];
  (*R12)[2][2] = R1[2][0]*R2[0][2] + R1[2][1]*R2[1][2] + R1[2][2]*R2[2][2];
} /* matrix33_mul */


// ----------------------------------------------------------------------------
// ellipsoid_precalc
// Precalculate some ellipsoid constants
// ----------------------------------------------------------------------------
// http://en.wikipedia.org/wiki/Geodetic_datum
// a = semi-major axis (given)
// f = reciprocal of flattening (given)
// flattening: f = (a - b)/a = 1 - sqrt(1 - e^2)
// b = semi-minor axis
// n = third flattening
// e = first eccentricity
// e' = second eccentricity
// c = polar radius of curvature
// M = meridional radius of curvature
//
// b = a*(1 - f)
// n = (a - b)/(a + b) = f/(2 - f)
// c = a^2/b = a/(1 - f)
// M = b^2/a = a(1 - e^2) = a*(1 - f)^2
// e^2 = 1 - b/c = (a^2 - b^2)/a^2 = 1 - b^2/a^2 = 2f - f^2 = f*(2 - f)
// e'^2 = c/b - 1 = (a^2 - b^2)/b^2 = a^2/b^2 - 1 = e^2/(1 - e^2) = f*(2 - f)/(1 - f)^2
// ----------------------------------------------------------------------------
// Ellipsoid: 0: bessel, 1: wgs84, 2: etrs89
void ellipsoid_precalc(int oid)
{
  ellipsoid.a2  = pow(ellipsoid.a,2);
  ellipsoid.b   = ellipsoid.a*(1 - ellipsoid.f);
  ellipsoid.b2  = pow(ellipsoid.b,2);
  ellipsoid.n   = ellipsoid.f/(2 - ellipsoid.f);
  ellipsoid.c   = ellipsoid.a/(1 - ellipsoid.f);
  ellipsoid.M   = ellipsoid.a*pow(1 - ellipsoid.f,2);
  ellipsoid.e2  = ellipsoid.f*(2 - ellipsoid.f);
  ellipsoid.e2_ = ellipsoid.e2/pow(1 - ellipsoid.f,2);
} /* ellipsoid_precalc */

// ----------------------------------------------------------------------------
// series_precalc
// Precalculate some constants for series used in meridian distance (fi0)
// ----------------------------------------------------------------------------
// Ellipsoid: 0: bessel, 1: wgs84, 2: etrs89
void series_precalc(int oid)
{
  double E2, E4, E6, E8, E10;
  double N, N2, N3, N4, N5;

  // Coefficients for series to determine meridian arc length (L) on ellipsoid
  // See "Geodesy - Introduction to Geodetic Datum and Geodetic Systems (Zhiping, 2014), pg. 194"
  //   for cos() variant

#ifdef L1
  // See "Geometrical Geodesy (Hooijberg, 2008), pg. 165(pdf: 183)"
  // See "Bundeseinheitliche Transformation für ATKIS"
  //   for sin() variant (A, ...; more precise)
  E2 = ellipsoid.e2_;
  E4 = pow(E2,2); E6 = E4*E2; E8 = pow(E4,2); E10 = E6*E4;

  ellipsoid.A = 1.0 - 3.0/4.0*E2 + 45.0/64.0*E4 - 175.0/256.0*E6
    + 11025.0/16384.0*E8 - 43659.0/65536.0*E10;
  ellipsoid.B = -3.0/4.0*E2 + 15.0/16.0*E4 - 525.0/512.0*E6
    + 2205.0/2048.0*E8 - 72765.0/65536.0*E10;
  ellipsoid.C = 15.0/64.0*E4 - 105.0/256.0*E6 + 2205.0/4096.0*E8
    - 10395.0/16384.0*E10;
  ellipsoid.D = -35.0/512.0*E6 + 315.0/2048.0*E8 - 31185.0/131072.0*E10;
  ellipsoid.E = 315.0/16384.0*E8 - 3465.0/65536.0*E10;
  ellipsoid.F = -639.0/131072.0*E10;
#else //L2
  // See "Stara in nova drzavna kartografska projekcija (2008), pg. 8)"
  // See "Digitalni model reliefa (Podobnikar, 2001), pg. 100(pdf: 105)"
  //     "A General Formula for Calculating Meridian Arc Length (Kawase, 2011)"
  E2 = ellipsoid.e2;
  E4 = pow(E2,2); E6 = E4*E2; E8 = pow(E4,2); E10 = E6*E4;

  ellipsoid.A = 1.0 + 3.0/4.0*E2 + 45.0/64.0*E4 + 175.0/256.0*E6
    + 11025.0/16384.0*E8 + 43659.0/65536.0*E10;
  ellipsoid.B = 3.0/4.0*E2 + 15.0/16.0*E4 + 525.0/512.0*E6
    + 2205.0/2048.0*E8 + 72765.0/65536.0*E10;
  ellipsoid.C = 15.0/64.0*E4 + 105.0/256.0*E6 + 2205.0/4096.0*E8
    + 10395.0/16384.0*E10;
  ellipsoid.D = 35.0/512.0*E6 + 315.0/2048.0*E8 + 31185.0/131072.0*E10;
  ellipsoid.E = 315.0/16384.0*E8 + 3465.0/65536.0*E10;
  ellipsoid.F = 639.0/131072.0*E10;
#endif

  // Coefficients for series to determine fi0 on ellipsoid (alternative way)
  // See "Predavanje Geodezija (Univ. Zagreb, 2007), pg. 22-23"
  //   for sin() variant (alfa, ...; similar to above)

  //L3
  N = ellipsoid.n; N2 = pow(ellipsoid.n,2); N3 = pow(ellipsoid.n,3);
  N4 = pow(ellipsoid.n,4); N5 = pow(ellipsoid.n,5);

  ellipsoid.alfa = (ellipsoid.a + ellipsoid.b)/2.0*(1.0 + 1.0/4.0*N2
    + 1.0/64.0*N4);
  ellipsoid.beta = 3.0/2.0*N - 27.0/32.0*N3 + 269.0/512.0*N5;
  ellipsoid.gama = 21.0/16.0*N2 - 55.0/32.0*N4;
  ellipsoid.delta = 151.0/96.0*N3 - 417.0/128.0*N5;
  ellipsoid.epsilon = 1097.0/512.0*N4;
} /* series_precalc */


// ----------------------------------------------------------------------------
// ellipsoid_init
// ----------------------------------------------------------------------------
void ellipsoid_init()
{
  // http://en.wikipedia.org/wiki/Bessel_ellipsoid
  // For Slovenia: http://epsg.io/3911-3917
  ellips[0].a = 6377397.155;
  ellips[0].f = 1/299.1528128; //0.00334277318217481
//ellips[0].f = 1/299.152815351; // Japan
  ellipsoid_precalc(0);
  series_precalc(0);

  // http://en.wikipedia.org/wiki/World_Geodetic_System (wgs84)
  ellips[1].a = 6378137.0;
  ellips[1].f = 1/298.257223563; //0.00335281066474748
  ellipsoid_precalc(1);
  series_precalc(1);

  // http://en.wikipedia.org/wiki/European_Terrestrial_Reference_System_1989
  // Same as http://en.wikipedia.org/wiki/GRS_80
  ellips[2].a = 6378137.0;
  ellips[2].f = 1/298.257222101; //0.00335281068118232
  ellipsoid_precalc(2);
  series_precalc(2);
} /* ellipsoid_init */


// ----------------------------------------------------------------------------
// h7_precalc
// Precalculate rotation matrix for Helmert transformation
// ----------------------------------------------------------------------------
void h7_precalc(HELMERT7 *h7)
{
  double alfa, beta, gama;
  double sinAlfa, cosAlfa, sinBeta, cosBeta, sinGama, cosGama;
  m33 R1, R2, R3, R12;
  char *errtxt;

  // Angles specified in arc seconds, convert to radians
  alfa = (h7->alfa/3600.0)*PI/180.0;
  beta = (h7->beta/3600.0)*PI/180.0;
  gama = (h7->gama/3600.0)*PI/180.0;

  sinAlfa = sin(alfa); cosAlfa = cos(alfa);
  sinBeta = sin(beta); cosBeta = cos(beta);
  sinGama = sin(gama); cosGama = cos(gama);

#ifdef H71
  // See "GNSS - Global Navigation Satellite Systems (Hofmann-Wellenhof, 2008), pg: 294"
  // See "Digitalni model reliefa (Podobnikar, 2001), pg. 114(pdf: 119)"
  R1[0][0] = cosGama;  R1[0][1] = sinGama;  R1[0][2] = 0;
  R1[1][0] = -sinGama; R1[1][1] = cosGama;  R1[1][2] = 0;
  R1[2][0] = 0;        R1[2][1] = 0;        R1[2][2] = 1;

  R2[0][0] = cosBeta;  R2[0][1] = 0;        R2[0][2] = -sinBeta;
  R2[1][0] = 0;        R2[1][1] = 1;        R2[1][2] = 0;
  R2[2][0] = sinBeta;  R2[2][1] = 0;        R2[2][2] = cosBeta;

  R3[0][0] = 1;        R3[0][1] = 0;        R3[0][2] = 0;
  R3[1][0] = 0;        R3[1][1] = cosAlfa;  R3[1][2] = sinAlfa;
  R3[2][0] = 0;        R3[2][1] = -sinAlfa; R3[2][2] = cosAlfa;

  h7->R = (m33 *)malloc(sizeof(m33));
  if (h7->R == NULL) {
    errtxt = xstrerror();
    if (errtxt != NULL) {
      fprintf(stderr, "malloc(m33): %s\n", errtxt); free(errtxt);
    } else
      fprintf(stderr, "malloc(m33): Can't allocate memory\n");
    exit(3);
  }

  matrix33_mul(R1,  R2, &R12);
  matrix33_mul(R12, R3, h7->R);
#elif defined(H72)
  // See "Computing Helmert transformations (Watson, 2005)"
  // (different rotations?!)
  R1[0][0] = cosAlfa;  R1[0][1] = -sinAlfa; R1[0][2] = 0;
  R1[1][0] = sinAlfa;  R1[1][1] = cosAlfa;  R1[1][2] = 0;
  R1[2][0] = 0;        R1[2][1] = 0;        R1[2][2] = 1;

  R2[0][0] = cosBeta;  R2[0][1] = 0;        R2[0][2] = -sinBeta;
  R2[1][0] = 0;        R2[1][1] = 1;        R2[1][2] = 0;
  R2[2][0] = sinBeta;  R2[2][1] = 0;        R2[2][2] = cosBeta;

  R3[0][0] = 1;        R3[0][1] = 0;        R3[0][2] = 0;
  R3[1][0] = 0;        R3[1][1] = cosGama;  R3[1][2] = -sinGama;
  R3[2][0] = 0;        R3[2][1] = sinGama;  R3[2][2] = cosGama;

  h7->R = (m33 *)malloc(sizeof(m33));
  if (h7->R == NULL) {
    errtxt = xstrerror();
    if (errtxt != NULL) {
      fprintf(stderr, "malloc(m33): %s\n", errtxt); free(errtxt);
    } else
      fprintf(stderr, "malloc(m33): Can't allocate memory\n");
    exit(3);
  }

  matrix33_mul(R1,  R2, &R12);
  matrix33_mul(R12, R3, h7->R);
#endif
} /* h7_precalc */


// ----------------------------------------------------------------------------
// params_init
// ----------------------------------------------------------------------------
void params_init()
{
  DMS lat, lon;
  double dlat, dlon;

  // http://en.wikipedia.org/wiki/Helmert_transformation
  // Parameters for Slovenia ETRS89 (D48/GK --> D96/TM)
  // http://193.2.92.129/SiTraNet2.10-navodila.htm#_Toc214285899 (SLO splosni)
  // (Coordinate Frame Transformation)
  slo7.dX   = 409.545088; // Cx
  slo7.dY   =  72.164092; // Cy
  slo7.dZ   = 486.871732; // Cz
  slo7.alfa =  -3.085957; // rx
  slo7.beta =  -5.469110; // ry
  slo7.gama =  11.020289; // rz
  slo7.dm   =  17.919665; // s (scale)

  // Parameters for Slovenia ETRS89 (D96/TM --> D48/GK, inverse)
  // http://193.2.92.129/SiTraNet2.10-navodila.htm#_Toc214285900 (SLO splosni)
  slo7inv.dX   = -409.520465; // Cx
  slo7inv.dY   =  -72.191827; // Cy
  slo7inv.dZ   = -486.872387; // Cz
  slo7inv.alfa =    3.086250; // rx
  slo7inv.beta =    5.468945; // ry
  slo7inv.gama =  -11.020370; // rz
  slo7inv.dm   =  -17.919456; // s (scale)

  // For additional parameters for Slovenia see:
  // http://www.transformacije.si/koristno/drzavni-parametri

  // Precalculate rotation matrices for Helmert transformation
  h7_precalc(&slo7);
  h7_precalc(&slo7inv);

  // Transverse Mercator projection parameters for Slovenia
  // (same for Gauss-Krueger projection)
  tm.scale = 0.9999;
  tm.false_easting = 500000;
  tm.false_northing = -5000000;
  tm.meridian = 15; // central meridian for Slovenia
  tm.lambda0 = tm.meridian*PI/180.0; //0.26179938779914941

  // Absolute geoid model limits
  // fi: 45°15' - 47°00', inc: 1.0'
  lat.deg = 45; lat.min = 15; lat.sec = 0;
  dms2deg(lat, &dlat); gfimin = dlat;
  lat.deg = 47; lat.min = 0; lat.sec = 0;
  dms2deg(lat, &dlat); gfimax = dlat;
  lat.deg = 0; lat.min = 1; lat.sec = 0;
  dms2deg(lat, &dlat); gfiinc1 = dlat;
  // la: 13°15' - 16°45', inc: 1.5'/1.0'
  lon.deg = 13; lon.min = 15; lon.sec = 0;
  dms2deg(lon, &dlon); glamin = dlon;
  lon.deg = 16; lon.min = 45; lon.sec = 0;
  dms2deg(lon, &dlon); glamax = dlon;
  lon.deg = 0; lon.min = 1; lon.sec = 30;
  dms2deg(lon, &dlon); glainc15 = dlon;
  lon.deg = 0; lon.min = 1; lon.sec = 0;
  dms2deg(lon, &dlon); glainc1 = dlon;

  // There's no data available for geoid on Bessel 1841!
  memset(geoid_bes, 0, sizeof(geoid_bes));
} /* params_init */


// ----------------------------------------------------------------------------
// geoid_height (depends on ellipsoid)
// ----------------------------------------------------------------------------
// Geoid: 0: bessel, 1: slo2000/wgs84, 2: egm2008/wgs84
double geoid_height(double fi, double la, int gid)
{
  double Ng = 0.0;
  double xi, yi; int ix, iy, gixmax, giymax;
  double x, y, x1, y1, x2, y2;
  double p1, p2, p3, p4;
  double R1, R2;

  if (fi < gfimin || fi > gfimax || la < glamin || la > glamax) {
    // Outside geoid model
    return Ng;
  }

  x = fi; y = la;

  xi = (fi - gfimin)/gfiinc1; ix = (int)xtrunc(xi);
  if (gid == 2) yi = (la - glamin)/glainc1; //egm2008
  else yi = (la - glamin)/glainc15; //slo2000, bessel
  iy = (int)xtrunc(yi);

  gixmax = 105; giymax = (gid == 2) ? 210 : 140;
  if (ix <= 0 || ix >= gixmax || iy <= 0 || iy >= giymax) {
    // On geoid model borders
    return Ng;
  }

  if (gid == 1) {
    x1 = gfimin + ix*gfiinc1; y1 = glamin + iy*glainc15;
    x2 = x1 + gfiinc1;        y2 = y1 + glainc15;

    p3 = geoid_slo[ix+1][iy]; p4 = geoid_slo[ix+1][iy+1];
    p1 = geoid_slo[ix][iy];   p2 = geoid_slo[ix][iy+1];
  }
  else if (gid == 2) {
    x1 = gfimin + ix*gfiinc1; y1 = glamin + iy*glainc15;
    x2 = x1 + gfiinc1;        y2 = y1 + glainc1;

    p3 = geoid_egm[ix+1][iy]; p4 = geoid_egm[ix+1][iy+1];
    p1 = geoid_egm[ix][iy];   p2 = geoid_egm[ix][iy+1];
  }
  else {
    x1 = gfimin + ix*gfiinc1; y1 = glamin + iy*glainc15;
    x2 = x1 + gfiinc1;        y2 = y1 + glainc15;

    p3 = geoid_bes[ix+1][iy]; p4 = geoid_bes[ix+1][iy+1];
    p1 = geoid_bes[ix][iy];   p2 = geoid_bes[ix][iy+1];
  }
  if (p1 == 0.0) {
    // No data in geoid model (outside Slovenia for slo2000)
    return Ng;
  }

  // Add missing values on Slovenia borders for slo2000
  if (p3 == 0.0) p3 = p1; if (p4 == 0.0) p4 = p1;
  /* p1 = base value */   if (p2 == 0.0) p2 = p1;

  // Bilinear interpolation (from Wiki)
  R1 = (y2 - y)/(y2 - y1)*p1 + (y - y1)/(y2 - y1)*p2;
  R2 = (y2 - y)/(y2 - y1)*p3 + (y - y1)/(y2 - y1)*p4;
  Ng = (x2 - x)/(x2 - x1)*R1 + (x - x1)/(x2 - x1)*R2;

  return Ng;
} /* geoid_height */


// ----------------------------------------------------------------------------
// point_in_bounding_box
// ----------------------------------------------------------------------------
int point_in_bounding_box(double x1, double y1, double x2, double y2, double x3, double y3, double x, double y)
{
  double xMin, xMax, yMin, yMax;

  xMin = xfmin(x1, xfmin(x2, x3)) - EPSILON;
  xMax = xfmax(x1, xfmax(x2, x3)) + EPSILON;
  yMin = xfmin(y1, xfmin(y2, y3)) - EPSILON;
  yMax = xfmax(y1, xfmax(y2, y3)) + EPSILON;
 
  if (x < xMin || x > xMax || y < yMin || y > yMax)
    return 0;
  else
    return 1;
} /* point_in_bounding_box */


// ----------------------------------------------------------------------------
// side
// ----------------------------------------------------------------------------
double side(double x1, double y1, double x2, double y2, double x, double y)
{
  return (y2 - y1)*(x - x1) + (-x2 + x1)*(y - y1);
} /* side */

// ----------------------------------------------------------------------------
// point_in_triangle
// ----------------------------------------------------------------------------
int point_in_triangle(double x1, double y1, double x2, double y2, double x3, double y3, double x, double y)
{
  int checkSide1, checkSide2, checkSide3;

  checkSide1 = side(x1, y1, x2, y2, x, y) >= 0.0;
  checkSide2 = side(x2, y2, x3, y3, x, y) >= 0.0;
  checkSide3 = side(x3, y3, x1, y1, x, y) >= 0.0;

  return checkSide1 && checkSide2 && checkSide3;
} /* point_in_triangle */


// ----------------------------------------------------------------------------
// dist_to_segm
// ----------------------------------------------------------------------------
double dist_to_segm(double x1, double y1, double x2, double y2, double x, double y)
{
  double p1_p2_squareLen, p_p1_squareLen;
  double dotProduct;

  p1_p2_squareLen = (x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1);
  dotProduct = ((x - x1)*(x2 - x1) + (y - y1)*(y2 - y1)) / p1_p2_squareLen;
  if (dotProduct < 0.0)
    return (x - x1)*(x - x1) + (y - y1)*(y - y1);
  else if (dotProduct <= 1.0) {
    p_p1_squareLen = (x1 - x)*(x1 - x) + (y1 - y)*(y1 - y);
    return p_p1_squareLen - dotProduct * dotProduct * p1_p2_squareLen;
  }
  else
    return (x - x2)*(x - x2) + (y - y2)*(y - y2);
} /* dist_to_segm */


// ----------------------------------------------------------------------------
// coord_in_triangle
// ----------------------------------------------------------------------------
int coord_in_triangle(GEOUTM in, AFT aft)
{
  double x, y, x1, y1, x2, y2, x3, y3;

  x = in.x; y = in.y;
  x1 = aft.src[0].x; y1 = aft.src[0].y;
  x2 = aft.src[1].x; y2 = aft.src[1].y;
  x3 = aft.src[2].x; y3 = aft.src[2].y;

  if (!point_in_bounding_box(x1, y1, x2, y2, x3, y3, x, y))
    return 0;
 
  if (point_in_triangle(x1, y1, x2, y2, x3, y3, x, y))
    return 1;
 
  if (dist_to_segm(x1, y1, x2, y2, x, y) <= EPSILON2)
    return 1;
  if (dist_to_segm(x2, y2, x3, y3, x, y) <= EPSILON2)
    return 1;
  if (dist_to_segm(x3, y3, x1, y1, x, y) <= EPSILON2)
    return 1;
 
  return 0;
} /* coord_in_triangle */


// ----------------------------------------------------------------------------
// xy2fila_ellips (height calculated from geoid)
// ----------------------------------------------------------------------------
// Transform from GK/TM x,y,H coordinates to fi,la,h on specified ellipsoid
// ----------------------------------------------------------------------------
// Ellipsoid: 0: bessel, 1: wgs84, 2: etrs89
void xy2fila_ellips(GEOUTM in, GEOGRA *out, int oid)
{
  double ab, fi0, dif, L; int n;
  double sinFi0, sin2Fi0;
  double cosFi0, cos2Fi0;
  double tanFi0, tan2Fi0, tan4Fi0, tan6Fi0;
  double N, ni2, ni4, Ng;

  // Convert from relative to real coordinates
  in.x = (in.x - tm.false_northing)/tm.scale;
  in.y = (in.y - tm.false_easting)/tm.scale;

  // Calculate fi0 - footpoint latitude
  // See "Geometrical Geodesy (Hooijberg, 2008), pg. 165(pdf: 183)"
  ab = ellipsoid.a + ellipsoid.b;
  fi0 = 2.0*in.x/ab; // first estimate
  dif = 1.0; n = 15;
  while (fabs(dif) >= 1e-18 && n > 0) {
#ifdef L1
    L = ellipsoid.c*(ellipsoid.A*fi0 + ellipsoid.B/2.0*sin(2.0*fi0)
      + ellipsoid.C/4.0*sin(4.0*fi0) + ellipsoid.D/6.0*sin(6.0*fi0)
      + ellipsoid.E/8.0*sin(8.0*fi0) + ellipsoid.F/10.0*sin(10.0*fi0));
#else //L2
    L = ellipsoid.M*(ellipsoid.A*fi0 - ellipsoid.B/2.0*sin(2.0*fi0)
      + ellipsoid.C/4.0*sin(4.0*fi0) - ellipsoid.D/6.0*sin(6.0*fi0)
      + ellipsoid.E/8.0*sin(8.0*fi0) - ellipsoid.F/10.0*sin(10.0*fi0));
#endif
    dif = 2.0*(in.x - L)/ab;
    fi0 += dif;
    n--;
  }
#ifdef L3
  // Alternative way of calculating fi0 (similar to above, faster)
  // See "Predavanje Geodezija (Univ. Zagreb, 2007), pg. 22-23"
  double fiq;

  fiq = in.x/ellipsoid.alfa;
  fi0 = fiq + ellipsoid.beta*sin(2.0*fiq) + ellipsoid.gama*sin(4.0*fiq)
    + ellipsoid.delta*sin(6.0*fiq) + ellipsoid.epsilon*sin(8.0*fiq);
#endif

  sinFi0 = sin(fi0);
  sin2Fi0 = pow(sinFi0,2);
  cosFi0 = cos(fi0);
  cos2Fi0 = pow(cosFi0,2);
  tanFi0 = tan(fi0);
  tan2Fi0 = pow(tanFi0,2); tan4Fi0 = pow(tanFi0,4); tan6Fi0 = pow(tanFi0,6);

  // See "Predavanje Geodezija (Univ. Zagreb, 2007), pg. 22"
  N = ellipsoid.c/sqrt(1.0 + ellipsoid.e2_*cos2Fi0);
//N = ellipsoid.a/sqrt(1.0 - ellipsoid.e2*sin2Fi0); // alternative (from java)
//t = tanFi0;
  ni2 = ellipsoid.e2_*cos2Fi0;
  ni4 = pow(ni2,2);

#if 1
  // See "Predavanje Geodezija (Univ. Zagreb, 2007), pg. 41"
  // See "Bundeseinheitliche Transformation für ATKIS (BeTA2007), pg. 28"
  out->fi = fi0
    + tanFi0/(2.0*pow(N,2))*(-1.0 - ni2)*pow(in.y,2)
    + tanFi0/(24.0*pow(N,4))*(5.0 + 3.0*tan2Fi0 + 6.0*ni2 - 6.0*tan2Fi0*ni2
        - 3.0*ni4 - 9.0*tan2Fi0*ni4)*pow(in.y,4)
    + tanFi0/(720.0*pow(N,6))*(-61.0 - 90.0*tan2Fi0 - 45.0*tan4Fi0 - 107.0*ni2
        + 162.0*tan2Fi0*ni2 + 45.0*tan4Fi0*ni2)*pow(in.y,6)
    + tanFi0/(40320.0*pow(N,8))*(1385.0 + 3633.0*tan2Fi0 + 4095.0*tan4Fi0
        + 1575.0*tan6Fi0)*pow(in.y,8);
#else // really identical
  // See "Stara in nova drzavna kartografska projekcija (2008), pg. 8)"
  // See "Digitalni model reliefa (Podobnikar, 2001), pg. 109(pdf: 114)"
  out->fi = fi0
    - tanFi0/(2.0*pow(N,2))*(1.0 + ni2)*pow(in.y,2)
    + tanFi0/(24.0*pow(N,4))*(5.0 + 3.0*tan2Fi0 + 6.0*ni2 - 6.0*tan2Fi0*ni2
        - 3.0*ni4 - 9.0*tan2Fi0*ni4)*pow(in.y,4)
    - tanFi0/(720.0*pow(N,6))*(61.0 + 90.0*tan2Fi0 + 45.0*tan4Fi0 + 107.0*ni2
        - 162.0*tan2Fi0*ni2 - 45.0*tan4Fi0*ni2)*pow(in.y,6)
    + tanFi0/(40320.0*pow(N,8))*(1385.0 + 3633.0*tan2Fi0 + 4095.0*tan4Fi0
        + 1575.0*tan6Fi0)*pow(in.y,8);
#endif

#if 0 //xxx
#ifdef L1
  L = ellipsoid.c*(ellipsoid.A*out->fi + ellipsoid.B/2.0*sin(2.0*out->fi)
    + ellipsoid.C/4.0*sin(4.0*out->fi) + ellipsoid.D/6.0*sin(6.0*out->fi)
    + ellipsoid.E/8.0*sin(8.0*out->fi) + ellipsoid.F/10.0*sin(10.0*out->fi));
#else //L2
  L = ellipsoid.M*(ellipsoid.A*out->fi - ellipsoid.B/2.0*sin(2.0*out->fi)
    + ellipsoid.C/4.0*sin(4.0*out->fi) - ellipsoid.D/6.0*sin(6.0*out->fi)
    + ellipsoid.E/8.0*sin(8.0*out->fi) - ellipsoid.F/10.0*sin(10.0*out->fi));
#endif
  printf(">> L: %.10f\n", L);
#endif

#if 1
  // See "Predavanje Geodezija (Univ. Zagreb, 2007), pg. 41"
  // See "Bundeseinheitliche Transformation für ATKIS (BeTA2007), pg. 28"
  out->la = tm.lambda0
    + 1.0/(N*cosFi0)*in.y
    + 1.0/(6.0*pow(N,3)*cosFi0)*(-1.0 - 2.0*tan2Fi0 - ni2)*pow(in.y,3)
    + 1.0/(120.0*pow(N,5)*cosFi0)*(5.0 + 28.0*tan2Fi0 + 24.0*tan4Fi0
        + 8.0*tan2Fi0*ni2 + 6.0*ni2)*pow(in.y,5)
    + 1.0/(5040.0*pow(N,7)*cosFi0)*(-61.0 - 662.0*tan2Fi0 - 1320.0*tan4Fi0
        - 720.0*tan6Fi0)*pow(in.y,7);
#else // really identical
  // See "Stara in nova drzavna kartografska projekcija (2008), pg. 8)"
  // See "Digitalni model reliefa (Podobnikar, 2001), pg. 109(pdf: 114)"
  out->la = tm.lambda0
    + 1.0/(N*cosFi0)*in.y
    - 1.0/(6.0*pow(N,3)*cosFi0)*(1.0 + 2.0*tan2Fi0 + ni2)*pow(in.y,3)
    + 1.0/(120.0*pow(N,5)*cosFi0)*(5.0 + 28.0*tan2Fi0 + 24.0*tan4Fi0 + 6.0*ni2
        + 8.0*tan2Fi0*ni2)*pow(in.y,5)
    - 1.0/(5040.0*pow(N,7)*cosFi0)*(61.0 + 662.0*tan2Fi0 + 1320.0*tan4Fi0
        + 720.0*tan6Fi0)*pow(in.y,7);
#endif

  // Convert from radians to degrees
  out->fi = out->fi*180.0/PI;
  out->la = out->la*180.0/PI;

  if (oid == 1 || oid == 2) //wgs84/etrs89
    Ng = geoid_height(out->fi, out->la, gid_wgs); //slo2000/egm2008
  else //bessel
    Ng = geoid_height(out->fi, out->la, 0); //bessel
  out->Ng = Ng;

  // Geoid height
  out->h = in.H + Ng;
} /* xy2fila_ellips */


// ----------------------------------------------------------------------------
// fila_ellips2xy (height calculated from geoid)
// ----------------------------------------------------------------------------
// Transform from fi,la,h to GK/TM x,y,H coordinates on specified ellipsoid
// ----------------------------------------------------------------------------
// Ellipsoid: 0: bessel, 1: wgs84, 2: etrs89
void fila_ellips2xy(GEOGRA in, GEOUTM *out, int oid)
{
  double dl, dl2, dl3, dl4, dl5, dl6, dl7, dl8;
  double sinFi, sin2Fi;
  double cosFi, cos2Fi, cos3Fi, cos4Fi, cos5Fi, cos6Fi, cos7Fi, cos8Fi;
  double tanFi, tan2Fi, tan4Fi, tan6Fi;
  double N, ni2, ni4, L, Ng;

  if (oid == 1 || oid == 2) //wgs84/etrs89
    Ng = geoid_height(in.fi, in.la, gid_wgs); //slo2000/egm2008
  else //bessel
    Ng = geoid_height(in.fi, in.la, 0); //bessel
  out->Ng = Ng;

  // Convert from degrees to radians
  in.fi = in.fi*PI/180.0;
  in.la = in.la*PI/180.0;

  dl = in.la - tm.lambda0;
  dl2 = pow(dl,2); dl3 = pow(dl,3); dl4 = pow(dl,4); dl5 = pow(dl,5);
  dl6 = pow(dl,6); dl7 = pow(dl,7); dl8 = pow(dl,8);

  sinFi = sin(in.fi);
  sin2Fi = pow(sinFi,2);
  cosFi = cos(in.fi);
  cos2Fi = pow(cosFi,2); cos3Fi = pow(cosFi,3); cos4Fi = pow(cosFi,4);
  cos5Fi = pow(cosFi,5); cos6Fi = pow(cosFi,6); cos7Fi = pow(cosFi,7);
  cos8Fi = pow(cosFi,8);
  tanFi = tan(in.fi);
  tan2Fi = pow(tanFi,2); tan4Fi = pow(tanFi,4); tan6Fi = pow(tanFi,6);

  // See "Predavanje Geodezija (Univ. Zagreb, 2007), pg. 22"
  N = ellipsoid.c/sqrt(1.0 + ellipsoid.e2_*cos2Fi);
//N = ellipsoid.a/sqrt(1.0 - ellipsoid.e2*sin2Fi); // alternative (from java)
//t = tanFi;
  ni2 = ellipsoid.e2_*cos2Fi;
  ni4 = pow(ni2,2);

#ifdef L1
  L = ellipsoid.c*(ellipsoid.A*in.fi + ellipsoid.B/2.0*sin(2.0*in.fi)
    + ellipsoid.C/4.0*sin(4.0*in.fi) + ellipsoid.D/6.0*sin(6.0*in.fi)
    + ellipsoid.E/8.0*sin(8.0*in.fi) + ellipsoid.F/10.0*sin(10.0*in.fi));
#else //L2
  L = ellipsoid.M*(ellipsoid.A*in.fi - ellipsoid.B/2.0*sin(2.0*in.fi)
    + ellipsoid.C/4.0*sin(4.0*in.fi) - ellipsoid.D/6.0*sin(6.0*in.fi)
    + ellipsoid.E/8.0*sin(8.0*in.fi) - ellipsoid.F/10.0*sin(10.0*in.fi));
#endif

#if 1
  // See "Stara in nova drzavna kartografska projekcija (2008), pg. 8)"
  // See "Predavanje Geodezija (Univ. Zagreb, 2007), pg. 40"
  out->x = L
    + tanFi/2.0*N*cos2Fi*dl2
    + tanFi/24.0*N*cos4Fi*(5.0 - tan2Fi + 9.0*ni2 + 4.0*ni4)*dl4
    + tanFi/720.0*N*cos6Fi*(61.0 - 58.0*tan2Fi + tan4Fi + 270.0*ni2
        - 330.0*tan2Fi*ni2)*dl6
    + tanFi/40320.0*N*cos8Fi*(1385.0 - 3111.0*tan2Fi + 543.0*tan4Fi
        - tan6Fi)*dl8;
#else // identical
  // See "Digitalni model reliefa (Podobnikar, 2001), pg. 110(pdf: 115)"
  out->x = L
    + 1.0/2.0*N*sinFi*cosFi*dl2
    + 1.0/24.0*N*sinFi*cos3Fi*(5.0 - tan2Fi + 9.0*ni2 + 4.0*ni4)*dl4
    + 1.0/720.0*N*sinFi*cos5Fi*(61.0 - 58.0*tan2Fi + tan4Fi)*dl6
    + 1.0/40320.0*N*sinFi*cos7Fi*(1385.0 - 3111.0*tan2Fi + 543.0*tan4Fi
        - tan6Fi)*dl8;
#endif

#if 1
  // See "Stara in nova drzavna kartografska projekcija (2008), pg. 8)"
  // See "Predavanje Geodezija (Univ. Zagreb, 2007), pg. 40"
  out->y = N*cosFi*dl
    + 1.0/6.0*N*cos3Fi*(1.0 - tan2Fi + ni2)*dl3
    + 1.0/120.0*N*cos5Fi*(5.0 - 18.0*tan2Fi + tan4Fi + 14.0*ni2
        - 58.0*tan2Fi*ni2)*dl5
    + 1.0/5040.0*N*cos7Fi*(61.0 - 479.0*tan2Fi + 179.0*tan4Fi - tan6Fi)*dl7;
#else // really identical
  // See "Digitalni model reliefa (Podobnikar, 2001), pg. 110(pdf: 115)"
  out->y = N*cosFi*dl
    + 1.0/6.0*N*cos3Fi*(1.0 - tan2Fi + ni2)*dl3
    + 1.0/120.0*N*cos5Fi*(5.0 - 18.0*tan2Fi + tan4Fi + 14.0*ni2
        - 58.0*tan2Fi*ni2)*dl5
    + 1.0/5040.0*N*cos7Fi*(61.0 - 479.0*tan2Fi + 179.0*tan4Fi - tan6Fi)*dl7;
#endif

  // Convert from real to relative coordinates
  out->x = out->x*tm.scale + tm.false_northing;
  out->y = out->y*tm.scale + tm.false_easting;

  // Geoid height
  out->H = in.h - Ng;
} /* fila_ellips2xy */


// ----------------------------------------------------------------------------
// xy2fila_ellips_loop (height calculated from geoid)
// ----------------------------------------------------------------------------
// Transform from GK/TM x,y,H coordinates to fi,la,h on specified ellipsoid
// using loop search over coordinates
// ----------------------------------------------------------------------------
// Ellipsoid: 0: bessel, 1: wgs84, 2: etrs89
int xy2fila_ellips_loop(GEOUTM in, GEOGRA *out, int oid)
{
  GEOGRA fl; GEOUTM xy;
  double fi_max, fi_inc;
  double la_max, la_inc;
  long long int rx, ry, rxx, ryy; double prec;
  int zone, found;
  double Ng;

  zone = 5; // Slovenia
  fl.fi = gkzones[zone].fimin; fi_max = gkzones[zone].fimax; fi_inc = 0.1;
  fl.la = gkzones[zone].lamin; la_max = gkzones[zone].lamax; la_inc = 0.1;
  fl.h = 0; // height will be calculated at the end

  xy.x = 0; xy.y = 0; xy.H = 0;
  prec = 100000000.0; // maximum precision (1e-8)
  rx = xllround(in.x*prec); ry = xllround(in.y*prec);
  rxx = xllround(xy.x*prec); ryy = xllround(xy.y*prec);

  found = 0;
  for ( ; ; ) {
    for ( ; fl.fi <= fi_max; fl.fi += fi_inc) {
      for ( ; fl.la <= la_max; fl.la += la_inc) {
        fila_ellips2xy(fl, &xy, oid);
        ryy = xllround(xy.y*prec);
        if ((ry - ryy) < 0) {
          fl.la -= la_inc;
          break;
        }
        else if (ry == ryy) break;
      } // for fl.la
      rxx = xllround(xy.x*prec);
      if ((rx - rxx) < 0) {
        fl.fi -= fi_inc;
        break;
      }
      else if (rx == rxx) break;
    } // for fl.fi
    if (ry == ryy && rx == rxx) { found = 1; break; }
    fi_inc /= 10.0; la_inc /= 10.0; // increase precision for next round
    if (fi_inc < 1e-18 || la_inc < 1e-18) break;
  } // outer loop
  fila_ellips2xy(fl, &xy, oid);

  out->fi = fl.fi; out->la = fl.la;

  if (oid == 1 || oid == 2) //wgs84/etrs89
    Ng = geoid_height(out->fi, out->la, gid_wgs); //slo2000/egm2008
  else //bessel
    Ng = geoid_height(out->fi, out->la, 0); //bessel
  out->Ng = Ng;

  // Geoid height
  out->h = in.H + Ng;

  return found;
} /* xy2fila_ellips_loop */


// ----------------------------------------------------------------------------
// fila_ellips2xy_loop (height calculated from geoid)
// ----------------------------------------------------------------------------
// Transform from fi,la,h to GK/TM x,y,H coordinates on specified ellipsoid
// using loop search over coordinates
// ----------------------------------------------------------------------------
// Ellipsoid: 0: bessel, 1: wgs84, 2: etrs89
int fila_ellips2xy_loop(GEOGRA in, GEOUTM *out, int oid)
{
  GEOUTM xy; GEOGRA fl;
  double x_max, x_inc;
  double y_max, y_inc;
  long long int rfi, rla, rxfi, rxla; double prec;
  int zone, found;
  double Ng;

  if (oid == 1 || oid == 2) //wgs84/etrs89
    Ng = geoid_height(in.fi, in.la, gid_wgs); //slo2000/egm2008
  else //bessel
    Ng = geoid_height(in.fi, in.la, 0); //bessel
  out->Ng = Ng;

  zone = 5; // Slovenia
  xy.x = gkzones[zone].xmin; x_max = gkzones[zone].xmax; x_inc = 10000;
  xy.y = gkzones[zone].ymin; y_max = gkzones[zone].ymax; y_inc = 10000;
  xy.H = 0; // height will be calculated at the end

  fl.fi = 0; fl.la = 0; fl.h = 0.0;
  prec = 100000000.0; // maximum precision (1e-8)
  rfi = xllround(in.fi*prec); rla = xllround(in.la*prec);
  rxfi = xllround(fl.fi*prec); rxla = xllround(fl.la*prec);

  found = 0;
  for ( ; ; ) {
    for ( ; xy.x <= x_max; xy.x += x_inc) {
      for ( ; xy.y <= y_max; xy.y += y_inc) {
        xy2fila_ellips(xy, &fl, oid);
        rxla = xllround(fl.la*prec);
        if ((rla - rxla) < 0) {
          xy.y -= y_inc;
          break;
        }
        else if (rla == rxla) break;
      } // for xy.y
      rxfi = xllround(fl.fi*prec);
      if ((rfi - rxfi) < 0) {
        xy.x -= x_inc;
        break;
      }
      else if (rfi == rxfi) break;
    } // for xy.x
    if (rla == rxla && rfi == rxfi) { found = 1; break; }
    x_inc /= 10.0; y_inc /= 10.0; // increase precision for next round
    if (x_inc < 1e-18 || y_inc < 1e-18) break;
  } // outer loop
  xy2fila_ellips(xy, &fl, oid);

  out->x = xy.x; out->y = xy.y;

  // Geoid height
  out->H = in.h - Ng;

  return found;
} /* fila_ellips2xy_loop */


// ----------------------------------------------------------------------------
// xyz2fila_ellips (height calculated from input)
// ----------------------------------------------------------------------------
// Transform from cart. X,Y,Z coordinates to fi,la,h on specified ellipsoid
// ----------------------------------------------------------------------------
// Ellipsoid: 0: bessel, 1: wgs84, 2: etrs89
void xyz2fila_ellips(GEOCEN in, GEOGRA *out, int oid)
{
  double p, N;
  double O, sinO, sin3O, cosO, cos3O;
  double fi0, dif; int n;

  // See "Comparison of different algorithms to transform
  //      geocentric to geodetic coordinates (Seemkooei, 2002)"
  p = sqrt(pow(in.X,2) + pow(in.Y,2));
#ifdef FI1
  // Bowring's algorithm (fastest)
  O = atan2(in.Z*ellipsoid.a, p*ellipsoid.b);
  sinO = sin(O);
  sin3O = pow(sinO,3);
  cosO = cos(O);
  cos3O = pow(cosO,3);

  out->fi = atan2(in.Z + ellipsoid.e2_*ellipsoid.b*sin3O,
                  p - ellipsoid.e2*ellipsoid.a*cos3O);

  N = ellipsoid.a/sqrt(1.0 - ellipsoid.e2*pow(sin(out->fi),2));
#else //FI2
  // Heiskanen and Moritz's algorithm (slower), more errors
  fi0 = atan2(in.Z*ellipsoid.a2, ellipsoid.b2*p); // first estimate
//fi0 = atan2(in.Z/(1.0 - ellipsoid.e2)*p); // identical (from Wiki)
  dif = 1.0; n = 15;
  while (fabs(dif) >= 1e-18 && n > 0) {
    N = ellipsoid.a/sqrt(1.0 - ellipsoid.e2*pow(sin(fi0),2));
    out->h = p/cos(fi0) - N;
    out->fi = atan2(in.Z, (1.0 - ellipsoid.e2*(N/(N + out->h)))*p);
    dif = fi0 - out->fi;
    fi0 = out->fi;
    n--;
  }
#endif
  out->la = atan2(in.Y, in.X);
  out->h = p/cos(out->fi) - N;

  // Convert from radians to degrees
  out->fi = out->fi*180.0/PI;
  out->la = out->la*180.0/PI;
} /* xyz2fila_ellips */


// ----------------------------------------------------------------------------
// fila_ellips2xyz (height included in calculation)
// ----------------------------------------------------------------------------
// Transform from fi,la,h to cart. X,Y,Z coordinates on specified ellipsoid
// ----------------------------------------------------------------------------
// Ellipsoid: 0: bessel, 1: wgs84, 2: etrs89
void fila_ellips2xyz(GEOGRA in, GEOCEN *out, int oid)
{
  double N;

  // Convert from degrees to radians
  in.fi = in.fi*PI/180.0;
  in.la = in.la*PI/180.0;

  N = ellipsoid.a/sqrt(1.0 - ellipsoid.e2*pow(sin(in.fi),2));

  // Hofmann-Wellenhof et al. 1994, ISO/DIS 19111 2001
  out->X = (N + in.h)*cos(in.fi)*cos(in.la);
  out->Y = (N + in.h)*cos(in.fi)*sin(in.la);
  out->Z = ((ellipsoid.b2/ellipsoid.a2)*N + in.h)*sin(in.fi);
//out->Z = ((1.0 - ellipsoid.e2)*N + in.h)*sin(in.fi); // identical (from Wiki)
} /* fila_ellips2xyz */


// ----------------------------------------------------------------------------
// xyz2xyz_helmert
// ----------------------------------------------------------------------------
// Transform from cart. X,Y,Z to cart. X,Y,Z coordinates
// using Helmert 7-parameters transformation
// [Xo;Yo;Zo] = [Xt;Yt;Zt] + (1 + s)*R*[Xi;Yi;Zi]
// R: rotation matrix
// ----------------------------------------------------------------------------
void xyz2xyz_helmert(GEOCEN in, GEOCEN *out, HELMERT7 h7)
{
  double alfa, beta, gama, dm1;
  double (*R)[3] = (double (*)[3])h7.R;

#if 0
  if (inv < 0) { // for inverse transformation
    // not really aplicable, although mentioned in Wiki
    h7.dX = -h7.dX; h7.dY = -h7.dY; h7.dZ = -h7.dZ;
    h7.alfa = -h7.alfa; h7.beta = -h7.beta; h7.gama = -h7.gama;
    h7.dm = -h7.dm;
  }
#endif

  // Angles specified in arc seconds, convert to radians
  alfa = (h7.alfa/3600.0)*PI/180.0;
  beta = (h7.beta/3600.0)*PI/180.0;
  gama = (h7.gama/3600.0)*PI/180.0;

  dm1 = 1.0 + h7.dm*1e-6;
#ifdef H71
  // See "GNSS - Global Navigation Satellite Systems (Hofmann-Wellenhof, 2008), pg: 294"
  // See "Digitalni model reliefa (Podobnikar, 2001), pg. 114(pdf: 119)"
  out->X = h7.dX + dm1*(R[0][0]*in.X + R[0][1]*in.Y + R[0][2]*in.Z);
  out->Y = h7.dY + dm1*(R[1][0]*in.X + R[1][1]*in.Y + R[1][2]*in.Z);
  out->Z = h7.dZ + dm1*(R[2][0]*in.X + R[2][1]*in.Y + R[2][2]*in.Z);
#elif defined(H72)
  // See "Computing Helmert transformations (Watson, 2005)"
  // (different rotations?!)
  out->X = h7.dX + dm1*(R[0][0]*in.X + R[0][1]*in.Y + R[0][2]*in.Z);
  out->Y = h7.dY + dm1*(R[1][0]*in.X + R[1][1]*in.Y + R[1][2]*in.Z);
  out->Z = h7.dZ + dm1*(R[2][0]*in.X + R[2][1]*in.Y + R[2][2]*in.Z);
#elif defined(H73)
  // Simplified version (for small angles, from Wiki)
  out->X = h7.dX + dm1*(in.X       + gama*in.Y - beta*in.Z);
  out->Y = h7.dY + dm1*(-gama*in.X + in.Y      + alfa*in.Z);
  out->Z = h7.dZ + dm1*(beta*in.X  - alfa*in.Y + in.Z);
#else
  // See "A guide to coordinate systems in Great Britain (2013), pg. 30"
  // (not suitable for Slovenia?!)
  out->X = h7.dX + (dm1*in.X   - gama*in.Y + beta*in.Z);
  out->Y = h7.dY + (gama*in.X  + dm1*in.Y  - alfa*in.Z);
  out->Z = h7.dZ + (-beta*in.X + alfa*in.Y + dm1*in.Z);
#endif
} /* xyz2xyz_helmert */


// ----------------------------------------------------------------------------
// gkxy2fila_wgs
// ----------------------------------------------------------------------------
// Transform from GK x,y,H on Bessel 1841 to fi,la,h on WGS 84
// ----------------------------------------------------------------------------
void gkxy2fila_wgs(GEOUTM in, GEOGRA *out)
{
  GEOGRA flb;
  GEOCEN xyzb, xyzw;
  DMS lat, lon;
  double Ng, H;

  H = in.H; // in.H = 0.0;

  // Convert GK x,y,H to fi,la,h on Bessel 1841
  xy2fila_ellips(in, &flb, 0); // no change in height because of bessel

#if 0 //xxx
  deg2dms(flb.fi, &lat); deg2dms(flb.la, &lon);
  printf("> lat_b: %2.0f %2.0f %6.3f, lon_b: %2.0f %2.0f %6.3f\n",
         lat.deg, lat.min, lat.sec, lon.deg, lon.min, lon.sec);
#endif

  // Convert fi,la,h on Bessel 1841 to X,Y,Z on Bessel 1841
  fila_ellips2xyz(flb, &xyzb, 0); // height included in calculation of XYZ

  // Convert X,Y,Z on Bessel 1841 to X,Y,Z on WGS 84
  xyz2xyz_helmert(xyzb, &xyzw, slo7);

  // Convert X,Y,Z on WGS 84 to fi,la,h on WGS 84
  xyz2fila_ellips(xyzw, out, 1); // transformed height calculated from XYZ

//if (hsel < 0) out->h;             // default: transformed height (SiTra)
  if (hsel == 1) out->h = H;        // copied height
  else if (hsel == 2) {
    Ng = geoid_height(out->fi, out->la, gid_wgs); //slo2000/egm2008
    out->h = H + Ng;                // geoid height
  }
} /* gkxy2fila_wgs */


// ----------------------------------------------------------------------------
// fila_wgs2gkxy
// ----------------------------------------------------------------------------
// Transform from fi,la,h on WGS 84 to GK x,y,H on Bessel 1841
// ----------------------------------------------------------------------------
void fila_wgs2gkxy(GEOGRA in, GEOUTM *out)
{
  GEOGRA flb;
  GEOCEN xyzb, xyzw;
  DMS lat, lon;
  double Ng, h;

  h = in.h; // in.h = 0.0;
  if (hsel < 0 || hsel == 2)
    Ng = geoid_height(in.fi, in.la, gid_wgs); //slo2000/egm2008
  else Ng = 0.0; // to keep compiler happy

  // Convert fi,la,h on WGS 84 to X,Y,Z on WGS 84
  fila_ellips2xyz(in, &xyzw, 1); // height included in calculation of XYZ

  // Convert X,Y,Z on WGS 84 to X,Y,Z on Bessel 1841
  xyz2xyz_helmert(xyzw, &xyzb, slo7inv);

  // Convert X,Y,Z on Bessel 1841 to fi,la,h on Bessel 1841
  xyz2fila_ellips(xyzb, &flb, 0); // transformed height calculated from XYZ

#if 0 //xxx
  deg2dms(flb.fi, &lat); deg2dms(flb.la, &lon);
  printf("> lat_b: %2.0f %2.0f %6.3f, lon_b: %2.0f %2.0f %6.3f\n",
         lat.deg, lat.min, lat.sec, lon.deg, lon.min, lon.sec);
#endif

  // Convert fi,la,h to GK x,y,H on Bessel 1841
  fila_ellips2xy(flb, out, 0); // no change in height because of bessel

  if (hsel < 0) out->H = h - Ng;       // default: geoid height (SiTra)
  else if (hsel == 1) out->H = h;      // copied height
  else if (hsel == 2) out->H = h - Ng; // geoid height
} /* fila_wgs2gkxy */


// ----------------------------------------------------------------------------
// gkxy2tmxy
// ----------------------------------------------------------------------------
// Transform from GK x,y,H on Bessel 1841 to TM n,e,H on WGS 84
// ----------------------------------------------------------------------------
void gkxy2tmxy(GEOUTM in, GEOUTM *out)
{
  GEOGRA fl, flb;
  GEOCEN xyzb, xyzw;
  DMS lat, lon;
  double H, ht;

  H = in.H; // in.H = 0.0;

  // Convert GK x,y,H to fi,la,h on Bessel 1841
  xy2fila_ellips(in, &flb, 0); // no change in height because of bessel

#if 0 //xxx
  deg2dms(flb.fi, &lat); deg2dms(flb.la, &lon);
  printf("> lat_b: %2.0f %2.0f %6.3f, lon_b: %2.0f %2.0f %6.3f\n",
         lat.deg, lat.min, lat.sec, lon.deg, lon.min, lon.sec);
#endif

  // Convert fi,la,h on Bessel 1841 to X,Y,Z on Bessel 1841
  fila_ellips2xyz(flb, &xyzb, 0); // height included in calculation of XYZ

  // Convert X,Y,Z on Bessel 1841 to X,Y,Z on WGS 84
  xyz2xyz_helmert(xyzb, &xyzw, slo7);

  // Convert X,Y,Z on WGS 84 to fi,la,h on WGS 84
  xyz2fila_ellips(xyzw, &fl, 1); // transformed height calculated from XYZ

  ht = fl.h; // fl.h = 0.0;

  // Convert fi,la,h on WGS 84 to TM n,e,H on WGS 84
  fila_ellips2xy(fl, out, 1); // height calculated from geoid (H = h - Ng)

//if (hsel < 0) out->H;            // default: geoid height (closest)
  if (hsel == 1) out->H = H;       // copied height
  else if (hsel == 0) out->H = ht; // transformed height
} /* gkxy2tmxy */


// ----------------------------------------------------------------------------
// tmxy2gkxy
// ----------------------------------------------------------------------------
// Transform from TM n,e,H on WGS 84 to GK x,y,H on Bessel 1841
// ----------------------------------------------------------------------------
void tmxy2gkxy(GEOUTM in, GEOUTM *out)
{
  GEOGRA fl, flb;
  GEOCEN xyzb, xyzw;
  DMS lat, lon;
  double hg, H;

  H = in.H; // in.H = 0.0;

  // Convert TM n,e,H on WGS 84 to fi,la,h on WGS 84
  xy2fila_ellips(in, &fl, 1); // height calculated from geoid (h = H + Ng)

  hg = fl.h; // fl.h = 0.0;

  // Convert fi,la,h on WGS 84 to X,Y,Z on WGS 84
  fila_ellips2xyz(fl, &xyzw, 1); // height included in calculation of XYZ

  // Convert X,Y,Z on WGS 84 to X,Y,Z on Bessel 1841
  xyz2xyz_helmert(xyzw, &xyzb, slo7inv);

  // Convert X,Y,Z on Bessel 1841 to fi,la,h on Bessel 1841
  xyz2fila_ellips(xyzb, &flb, 0); // transformed height calculated from XYZ

#if 0 //xxx
  deg2dms(flb.fi, &lat); deg2dms(flb.la, &lon);
  printf("> lat_b: %2.0f %2.0f %6.3f, lon_b: %2.0f %2.0f %6.3f\n",
         lat.deg, lat.min, lat.sec, lon.deg, lon.min, lon.sec);
#endif

  // Convert fi,la,h to GK x,y,H on Bessel 1841
  fila_ellips2xy(flb, out, 0); // no change in height because of bessel

//if (hsel < 0) out->H;            // default: transformed height (closest)
  if (hsel == 1) out->H = H;       // copied height
  else if (hsel == 2) out->H = hg; // geoid height (already calculated)
} /* tmxy2gkxy */


// ----------------------------------------------------------------------------
// gkxy2tmxy_aft (height copied)
// ----------------------------------------------------------------------------
// Transform from GK x,y,H on Bessel 1841 to TM n,e,H on WGS 84
// using pre-calculated affine transformation table
// ----------------------------------------------------------------------------
int gkxy2tmxy_aft(GEOUTM in, GEOUTM *out)
{
  double H;
  int ii, jj, found;

  H = in.H;

  out->x = 0.0; out->y = 0.0;

  found = 0;
  // check if point is in last found triangle
  if (last_tri >= 0) {
    ii = last_tri;
    if (coord_in_triangle(in, aft_gktm[ii])) {
      out->x = aft_gktm[ii].a*in.x + aft_gktm[ii].b*in.y + aft_gktm[ii].c;
      out->y = aft_gktm[ii].d*in.x + aft_gktm[ii].e*in.y + aft_gktm[ii].f;
      found = 1;
    }
    else last_tri = -1;
  }

  // if not found, search from the middle of the table in both directions
  ii = MAXAFT / 2; jj = ii - 1;
  for ( ; !found; ) {
    if (ii < MAXAFT) {
      if (coord_in_triangle(in, aft_gktm[ii])) {
        out->x = aft_gktm[ii].a*in.x + aft_gktm[ii].b*in.y + aft_gktm[ii].c;
        out->y = aft_gktm[ii].d*in.x + aft_gktm[ii].e*in.y + aft_gktm[ii].f;
        last_tri = ii;
        found = 1; break;
      }
      ii++;
    }
    if (jj >= 0) {
      if (coord_in_triangle(in, aft_gktm[jj])) {
        out->x = aft_gktm[jj].a*in.x + aft_gktm[jj].b*in.y + aft_gktm[jj].c;
        out->y = aft_gktm[jj].d*in.x + aft_gktm[jj].e*in.y + aft_gktm[jj].f;
        last_tri = jj;
        found = 1; break;
      }
      jj--;
    }
    if (ii >= MAXAFT && jj < 0) break;
  }

  out->H = H;  // default: copied height

  return found;
} /* gkxy2tmxy_aft */


// ----------------------------------------------------------------------------
// tmxy2gkxy_aft (height copied)
// ----------------------------------------------------------------------------
// Transform from TM n,e,H on WGS 84 to GK x,y,H on Bessel 1841
// using pre-calculated affine transformation table
// ----------------------------------------------------------------------------
int tmxy2gkxy_aft(GEOUTM in, GEOUTM *out)
{
  double H;
  int ii, jj, found;

  H = in.H;

  out->x = 0.0; out->y = 0.0;

  found = 0;
  // check if point is in last found triangle
  if (last_tri >= 0) {
    ii = last_tri;
    if (coord_in_triangle(in, aft_tmgk[ii])) {
      out->x = aft_tmgk[ii].a*in.x + aft_tmgk[ii].b*in.y + aft_tmgk[ii].c;
      out->y = aft_tmgk[ii].d*in.x + aft_tmgk[ii].e*in.y + aft_tmgk[ii].f;
      found = 1;
    }
    else last_tri = -1;
  }

  // if not found, search from the middle of the table in both directions
  ii = MAXAFT / 2; jj = ii - 1;
  for ( ; !found; ) {
    if (ii < MAXAFT) {
      if (coord_in_triangle(in, aft_tmgk[ii])) {
        out->x = aft_tmgk[ii].a*in.x + aft_tmgk[ii].b*in.y + aft_tmgk[ii].c;
        out->y = aft_tmgk[ii].d*in.x + aft_tmgk[ii].e*in.y + aft_tmgk[ii].f;
        last_tri = ii;
        found = 1; break;
      }
      ii++;
    }
    if (jj >= 0) {
      if (coord_in_triangle(in, aft_tmgk[jj])) {
        out->x = aft_tmgk[jj].a*in.x + aft_tmgk[jj].b*in.y + aft_tmgk[jj].c;
        out->y = aft_tmgk[jj].d*in.x + aft_tmgk[jj].e*in.y + aft_tmgk[jj].f;
        last_tri = jj;
        found = 1; break;
      }
      jj--;
    }
    if (ii >= MAXAFT && jj < 0) break;
  }

  out->H = H;  // default: copied height

  return found;
} /* tmxy2gkxy_aft */


// ----------------------------------------------------------------------------
// tmxy2fila_wgs
// ----------------------------------------------------------------------------
// Transform from TM n,e,H on WGS 84 to fi,la,h on WGS 84
// ----------------------------------------------------------------------------
void tmxy2fila_wgs(GEOUTM in, GEOGRA *out)
{
  double H;

  H = in.H; // in.H = 0.0;

  // Convert TM n,e,H on WGS 84 to fi,la,h on WGS 84
  xy2fila_ellips(in, out, 1); // height calculated from geoid (h = H + Ng)

//if (hsel < 0) out->h;             // default: geoid height
  if (hsel == 1) out->h = H;        // copied height
  // no transformed height possible
} /* tmxy2fila_wgs */


// ----------------------------------------------------------------------------
// fila_wgs2tmxy
// ----------------------------------------------------------------------------
// Transform from fi,la,h on WGS 84 to TM n,e,H on WGS 84
// ----------------------------------------------------------------------------
void fila_wgs2tmxy(GEOGRA in, GEOUTM *out)
{
  double h;

  h = in.h; // in.h = 0.0;

  // Convert fi,la,h on WGS 84 to TM n,e,H on WGS 84
  fila_ellips2xy(in, out, 1); // height calculated from geoid (H = h - Ng)

//if (hsel < 0) out->H;            // default: geoid height
  if (hsel == 1) out->H = h;       // copied height
  // no transformed height possible
} /* fila_wgs2tmxy */


// ----------------------------------------------------------------------------
// gkxy2fila_wgs_aft
// ----------------------------------------------------------------------------
// Transform from GK x,y,H on Bessel 1841 to fi,la,h on WGS 84
// using pre-calculated affine transformation table
// ----------------------------------------------------------------------------
void gkxy2fila_wgs_aft(GEOUTM in, GEOGRA *out)
{
  GEOUTM tmxy;

  gkxy2tmxy_aft(in, &tmxy);
  tmxy2fila_wgs(tmxy, out);
} /* gkxy2fila_wgs_aft */


// ----------------------------------------------------------------------------
// fila_wgs2gkxy_aft
// ----------------------------------------------------------------------------
// Transform from fi,la,h on WGS 84 to GK x,y,H on Bessel 1841
// using pre-calculated affine transformation table
// ----------------------------------------------------------------------------
void fila_wgs2gkxy_aft(GEOGRA in, GEOUTM *out)
{
  GEOUTM tmxy;

  fila_wgs2tmxy(in, &tmxy);
  tmxy2gkxy_aft(tmxy, out);
} /* fila_wgs2gkxy_aft */

#ifdef __cplusplus
}
#endif
