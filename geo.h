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
// geo.h: Include file for using coordinate conversion routines
//
#ifndef _GEO_H_DEFINED
#define _GEO_H_DEFINED

typedef struct dms {
  double deg, min, sec;
} DMS;

typedef double m33[3][3]; // matrix 3x3

typedef struct ellipsoid {
  double a, f; // given
  double a2, b, b2, n, c, M;
  double e2, e2_;
  // series' constants
  double A, B, C, D, E, F;
  double alfa, beta, gama, delta, epsilon;
} ELLIPSOID;

typedef struct helmert7 {
  double dX, dY, dZ;       // Cx, Cy, Cz
  double alfa, beta, gama; // rx, ry, rz
  double dm;               // s (scale)
  m33 *R;                  // rotation matrix
} HELMERT7;

typedef struct proj {
  double scale;
  double false_easting;
  double false_northing;
  double meridian;
  double lambda0; // meridian in radians
} PROJ;

typedef struct gklm {
  double fimin, fimax, lamin, lamax;
  double xmin, xmax, ymin, ymax;
} GKLM;

typedef struct geogra { // geodetic/geographic coordinates
  double fi; // fi, latitude
  double la; // lambda, longitude
  double h;  // elipsoidal height
  double Ng; // geoid height
} GEOGRA;

typedef struct geoutm { // geographic cartesian coordinates
  double x;  // easting
  double y;  // northing
  double H;  // ortometric/above sea level height
  double Ng; // geoid height
} GEOUTM;

typedef struct geocen { // geocentric cartesian coordinates
  double X;
  double Y;
  double Z;
} GEOCEN;

typedef struct aft { // affine transformation table
  GEOUTM src[3], dst[3];    // triangle points
  double cxy;              // src centroid x*y
  double a, b, c, d, e, f; // AFT parameters
} AFT;

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
void dms2deg(DMS dms, double *deg);
void dm2deg(DMS dms, double *deg);
void dms2rad(DMS dms, double *rad);
void dm2rad(DMS dms, double *rad);
void deg2dms(double deg, DMS *dms);
void deg2dm(double deg, DMS *dms);
void rad2dms(double rad, DMS *dms);
void rad2dm(double rad, DMS *dms);

void matrix33_mul(m33 R1, m33 R2, m33 *R12);

void ellipsoid_precalc(int oid);
void series_precalc(int oid);
void ellipsoid_init();
void h7_precalc(HELMERT7 *h7);
void params_init();

double geoid_height(double fi, double la, int gid);

int point_in_bounding_box(double x1, double y1, double x2, double y2, double x3, double y3, double x, double y);
double side(double x1, double y1, double x2, double y2, double x, double y);
int point_in_triangle(double x1, double y1, double x2, double y2, double x3, double y3, double x, double y);
double dist_to_segm(double x1, double y1, double x2, double y2, double x, double y);
int coord_in_triangle(GEOUTM in, AFT aft);

void xy2fila_ellips(GEOUTM in, GEOGRA *out, int oid);
void fila_ellips2xy(GEOGRA in, GEOUTM *out, int oid);
int xy2fila_ellips_loop(GEOUTM in, GEOGRA *out, int oid);
int fila_ellips2xy_loop(GEOGRA in, GEOUTM *out, int oid);
void xyz2fila_ellips(GEOCEN in, GEOGRA *out, int oid);
void fila_ellips2xyz(GEOGRA in, GEOCEN *out, int oid);
void xyz2xyz_helmert(GEOCEN in, GEOCEN *out, HELMERT7 h7);
void gkxy2fila_wgs(GEOUTM in, GEOGRA *out);
void fila_wgs2gkxy(GEOGRA in, GEOUTM *out);
void gkxy2tmxy(GEOUTM in, GEOUTM *out);
void tmxy2gkxy(GEOUTM in, GEOUTM *out);
int gkxy2tmxy_aft(GEOUTM in, GEOUTM *out);
int tmxy2gkxy_aft(GEOUTM in, GEOUTM *out);
void tmxy2fila_wgs(GEOUTM in, GEOGRA *out);
void fila_wgs2tmxy(GEOGRA in, GEOUTM *out);
void gkxy2fila_wgs_aft(GEOUTM in, GEOGRA *out);
void fila_wgs2gkxy_aft(GEOGRA in, GEOUTM *out);

#ifdef __cplusplus
}
#endif

#endif //_GEO_H_DEFINED
