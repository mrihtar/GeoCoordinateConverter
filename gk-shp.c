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
// gk-shp.c: Main cmd-line program for converting coordinates from shapefiles
//
#include "common.h"
#include "geo.h"
#include "shapefil.h"

#define SW_VERSION T("1.02")
#define SW_BUILD   T("Nov 22, 2015")

// global variables
TCHAR *prog;  // program name
int debug;

extern int gid_wgs; // selected geoid on WGS 84 (via cmd line)
extern int hsel;    // selected output height (via cmd line)

#define EPSG_3787 0 // D48/GK
#define EPSG_3912 1 // D48/GK
#define EPSG_3794 2 // D96/TM
#define EPSG_4258 3 // ETRS89
#define EPSG_4326 4 // WGS84

TCHAR *prj[5] = {
  // prj[0] = EPSG:3787 (D48/GK)
  T("PROJCS[\"MGI / Slovene National Grid\",GEOGCS[\"MGI\",DATUM[\"D_MGI\",SPHEROID[\"Bessel_1841\",6377397.155,299.1528128]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.017453292519943295]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",15],PARAMETER[\"scale_factor\",0.9999],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",-5000000],UNIT[\"Meter\",1]]"),

  // prj[1] = EPSG:3912 (D48/GK)
  T("PROJCS[\"MGI 1901 / Slovene National Grid\",GEOGCS[\"MGI 1901\",DATUM[\"D_MGI_1901\",SPHEROID[\"Bessel_1841\",6377397.155,299.1528128]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.017453292519943295]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",15],PARAMETER[\"scale_factor\",0.9999],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",-5000000],UNIT[\"Meter\",1]]"),

  // prj[2] = EPSG:3794 (D96/TM)
  T("PROJCS[\"Slovenia 1996 / Slovene National Grid\",GEOGCS[\"Slovenia 1996\",DATUM[\"D_Slovenia_Geodetic_Datum_1996\",SPHEROID[\"GRS_1980\",6378137,298.257222101]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.017453292519943295]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",15],PARAMETER[\"scale_factor\",0.9999],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",-5000000],UNIT[\"Meter\",1]]"),

  // prj[3] = EPSG:4258 (ETRS89)
  T("GEOGCS[\"ETRS89\",DATUM[\"D_ETRS_1989\",SPHEROID[\"GRS_1980\",6378137,298.257222101]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.017453292519943295]]"),

  // prj[4] = EPSG:4326 (WGS84)
  T("GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137,298.257223563]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.017453292519943295]]")
};

// ----------------------------------------------------------------------------
// swapxy
// ----------------------------------------------------------------------------
void swapxy(GEOUTM *xy)
{
  double tmp;

  tmp = xy->x; xy->x = xy->y; xy->y = tmp;
} /* swapxy */


// ----------------------------------------------------------------------------
// swapfila
// ----------------------------------------------------------------------------
void swapfila(GEOGRA *fl)
{
  double tmp;

  tmp = fl->fi; fl->fi = fl->la; fl->la = tmp;
} /* swapfila */


// ----------------------------------------------------------------------------
// fefind
// ----------------------------------------------------------------------------
TCHAR *fefind(TCHAR *fname, TCHAR *ext, TCHAR *newname)
{
  TCHAR *s, name[MAXS+1], newext[MAXS+1];
  struct _stat fst;
  int ii;

  xstrncpy(name, fname, MAXS);
  if ((s = strrchr(name, T('.'))) != NULL) *s = T('\0');

  xstrncpy(newname, name, MAXS);
  xstrncat(newname, ext, MAXS);
  for (ii = 0; ii <= 15; ii++) { // append 1..15 to output file name
    if (tstat(newname, &fst) < 0) break; // file not found
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


// ----------------------------------------------------------------------------
// usage
// ----------------------------------------------------------------------------
void usage(TCHAR *prog, int ver_only)
{
  fprintf(stderr, T("%s %s  Copyright (c) 2014-2015 Matjaz Rihtar  (%s)\n"),
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
  fprintf(stderr, T("                     9: xy   (d48gk)  --> fila (etrs89), ht, affine trans.\n"));
  fprintf(stderr, T("                    10: fila (etrs89) --> xy   (d48gk),  hg, affine trans.\n"));
  fprintf(stderr, T("  -r                reverse parsing order of xy/fila\n"));
  fprintf(stderr, T("                    (warning displayed if y < 200000)\n"));
  fprintf(stderr, T("  <inpname>         parse and convert input data from <inpname>\n"));
  fprintf(stderr, T("  <outname>         write output data to <outname>\n"));
  fprintf(stderr, T("\n"));
  fprintf(stderr, T("Input data format:\n"));
  fprintf(stderr, T("ESRI Shapefile (ArcGIS)\n"));
} /* usage */


// ----------------------------------------------------------------------------
// main (mingw32 unicode wrapper)
// ----------------------------------------------------------------------------
#if defined(__MINGW32__) && defined(_WCHAR)
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
  int ii, ac, opt;
  TCHAR *s, *av[MAXC];
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
      fprintf(stderr, T("malloc(av): %s"), xstrerror()); exit(3);
    }
    xstrncpy(av[ac++], argv[ii], MAXS);
  } // for each argc
  av[ac] = NULL;

  ellipsoid_init();
  params_init();

  if (ac < 2) goto usage;

  xstrncpy(inpname, av[0], MAXS);
  if (fefind(av[1], T(".shp"), outname) == NULL) {
    fprintf(stderr, T("%s: file already exists\n"), outname);
    exit(2);
  }
  if (fefind(outname, T(".prj"), prjname) == NULL) {
    fprintf(stderr, T("%s: file already exists\n"), prjname);
    exit(2);
  }

  // Open input files (shp, dbf)
  iSHP = SHPOpen(inpname, "r+b");
  if (iSHP == NULL) {
    // error already displayed
    exit(2);
  }
  iDBF = DBFOpen(inpname, "r+b");
  if (iDBF == NULL) {
    // error already displayed
    exit(2);
  }

  if (debug) fprintf(stderr, T("Processing %s\n"), inpname);
  clock_gettime(CLOCK_REALTIME, &start);

  SHPGetInfo(iSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound);
  if (debug)
    fprintf(stderr, T("Shapefile type: %s, number of shapes: %d\n"),
            SHPTypeName(nShapeType), nEntities);
  if (debug > 3)
    fprintf(stderr, T("File bounds: (%.15g, %.15g, %.15g, %.15g)\n"
                      "         to: (%.15g, %.15g, %.15g, %.15g)\n"),
            adfMinBound[0], adfMinBound[1], adfMinBound[2], adfMinBound[3],
            adfMaxBound[0], adfMaxBound[1], adfMaxBound[2], adfMaxBound[3]);

  // create output files (shp, dbf)
  oSHP = SHPCreate(outname, nShapeType);
  if (oSHP == NULL) {
    // error already displayed
    exit(2);
  }
  oDBF = DBFCloneEmpty(iDBF, av[1]);
  if (oDBF == NULL) {
    // error already displayed
    exit(2);
  }

  oTuple = (TCHAR *)malloc(oDBF->nRecordLength + 15);
  if (oTuple == NULL) {
    fprintf(stderr, T("malloc(oTuple): %s"), xstrerror()); exit(3);
  }

  // determine output projection
  switch (tr) {
    default:
    case 1: // xy (d96tm) --> fila (etrs89)
      proj = prj[EPSG_4326]; break; // WGS84
    case 2: // fila (etrs89) --> xy (d96tm)
      proj = prj[EPSG_3794]; break; // D96/TM
    case 3: // xy (d48gk) --> fila (etrs89)
      proj = prj[EPSG_4326]; break; // WGS84
    case 4: // fila (etrs89) --> xy (d48gk)
      proj = prj[EPSG_3787]; break; // D48/GK
    case 5: // xy (d48gk) --> xy (d96tm)
      proj = prj[EPSG_3794]; break; // D96/TM
    case 6: // xy (d96tm) --> xy (d48gk)
      proj = prj[EPSG_3787]; break; // D48/GK
    case 7: // xy (d48gk) --> xy (d96tm), affine trans.
      proj = prj[EPSG_3794]; break; // D96/TM
    case 8: // xy (d96tm) --> xy (d48gk), affine trans.
      proj = prj[EPSG_3787]; break; // D48/GK
    case 9: // xy (d48gk) --> fila (etrs89), affine trans.
      proj = prj[EPSG_4326]; break; // WGS84
    case 10: // fila (etrs89) --> xy (d48gk), affine trans.
      proj = prj[EPSG_3787]; break; // D48/GK
  }
  // write selected projection to .prj file (for GIS programs)
  out = fopen(prjname, "w");
  if (out == NULL) {
    fprintf(stderr, T("%s: %s"), prjname, xstrerror()); // ignore
  }
  else {
    fprintf(out, "%s\n", proj);
    fclose(out);
  }

  // process entitites (shapes) in input shapefile
  int nPercentBefore = -1;
  warn = 1;
  for (nEntity = 0; nEntity < nEntities; nEntity++) {
    psShape = SHPReadObject(iSHP, nEntity);
    if (psShape == NULL) {
      fprintf(stderr, T("%s: unable to read shape %d, terminate reading\n"),
              inpname, nEntity);
      break;
    }

    int nPercent = (long)nEntity*10000/nEntities;
    if (debug > 2) {
      fprintf(stderr, T("Shape: %d (%s), Vertices: %d, Parts: %d\n"),
              nEntity, SHPTypeName(psShape->nSHPType),
              psShape->nVertices, psShape->nParts);
      if (debug > 3) {
        if (psShape->bMeasureIsUsed)
          fprintf(stderr, T("Shape bounds: (%.15g, %.15g, %.15g, %.15g)\n"
                            "          to: (%.15g, %.15g, %.15g, %.15g)\n"),
            psShape->dfXMin, psShape->dfYMin, psShape->dfZMin, psShape->dfMMin,
            psShape->dfXMax, psShape->dfYMax, psShape->dfZMax, psShape->dfMMax);
        else
          fprintf(stderr, T("Shape bounds: (%.15g, %.15g, %.15g)\n"
                            "          to: (%.15g, %.15g, %.15g)\n"),
            psShape->dfXMin, psShape->dfYMin, psShape->dfZMin,
            psShape->dfXMax, psShape->dfYMax, psShape->dfZMax);
      }
    }
    else if (debug == 2 && (nPercent > nPercentBefore || nEntity == nEntities))
      fprintf(stderr, T("Shape: %d (%.2f%%)\r"), nEntity, (double)nPercent/100.0);

    if (psShape->nParts > 0 && psShape->panPartStart[0] != 0)
      fprintf(stderr, T("%s: panPartStart[0] = %d, not zero as expected\n"),
              inpname, psShape->panPartStart[0]); // ignore

    // process vertices in a shape
    nPart = 1;
    for (nVertex = 0; nVertex < psShape->nVertices; nVertex++) {
      pszPartType = "";
      if (nVertex == 0 && psShape->nParts > 0)
        pszPartType = (TCHAR *)SHPPartTypeName(psShape->panPartType[0]);

      if (nPart < psShape->nParts && psShape->panPartStart[nPart] == nVertex) {
        pszPartType = (TCHAR *)SHPPartTypeName(psShape->panPartType[nPart]);
        nPart++; pszPlus = T("+");
      }
      else pszPlus = T(" ");

      // prepare data for transformation
      if (tr == 2 || tr == 4 || tr == 10) { // etrs89
        ifl.la = psShape->padfX[nVertex]; ifl.fi = psShape->padfY[nVertex]; // reverse!
        ifl.h = psShape->padfZ[nVertex];
        if (rev) swapfila(&ifl);
        if (ifl.la > 17.0) {
          if (warn) { fprintf(stderr, T("%s: possibly reversed fi/la\n"), inpname); warn = 0; }
        }
      }
      else { // tr == 1,3,5,6,7,8,9 // d96tm/d48gk
        ixy.y = psShape->padfX[nVertex]; ixy.x = psShape->padfY[nVertex]; // reverse!
        ixy.H = psShape->padfZ[nVertex];
        if (rev) swapxy(&ixy);
        if (ixy.y < 200000.0) {
          ixy.y += 500000.0;
          if (warn) { fprintf(stderr, T("%s: possibly reversed x/y\n"), inpname); warn = 0; }
        }
      }

      // transform data
      switch (tr) {
        default:
        case 1: // xy (d96tm) --> fila (etrs89)
          tmxy2fila_wgs(ixy, &ofl); break;
        case 2: // fila (etrs89) --> xy (d96tm)
          fila_wgs2tmxy(ifl, &oxy); break;
        case 3: // xy (d48gk) --> fila (etrs89)
          gkxy2fila_wgs(ixy, &ofl); break;
        case 4: // fila (etrs89) --> xy (d48gk)
          fila_wgs2gkxy(ifl, &oxy); break;
        case 5: // xy (d48gk) --> xy (d96tm)
          gkxy2tmxy(ixy, &oxy); break;
        case 6: // xy (d96tm) --> xy (d48gk)
          tmxy2gkxy(ixy, &oxy); break;
        case 7: // xy (d48gk) --> xy (d96tm), affine trans.
          gkxy2tmxy_aft(ixy, &oxy); break;
        case 8: // xy (d96tm) --> xy (d48gk), affine trans.
          tmxy2gkxy_aft(ixy, &oxy); break;
        case 9: // xy (d48gk) --> fila (etrs89), affine trans.
          gkxy2fila_wgs_aft(ixy, &ofl); break;
        case 10: // fila (etrs89) --> xy (d48gk), affine trans.
          fila_wgs2gkxy_aft(ifl, &oxy); break;
      }

      // save transformed data
      if (tr == 1 || tr == 3 || tr == 9) { // etrs89
        if (rev) swapfila(&ofl);
        psShape->padfX[nVertex] = ofl.la; psShape->padfY[nVertex] = ofl.fi; // reverse!
        psShape->padfZ[nVertex] = ofl.h;
      }
      else { // tr == 2,4,5,6,7,8,10 // d96tm/d48gk
        if (rev) swapxy(&oxy);
        psShape->padfX[nVertex] = oxy.y; psShape->padfY[nVertex] = oxy.x; // reverse!
        psShape->padfZ[nVertex] = oxy.H;
      }

      if (debug > 4) {
        if (psShape->bMeasureIsUsed)
          fprintf(stderr, T("%s (%.15g, %.15g, %.15g, %.15g) %s\n"),
                  pszPlus, psShape->padfX[nVertex], psShape->padfY[nVertex],
                  psShape->padfZ[nVertex], psShape->padfM[nVertex],
                  pszPartType);
        else
          fprintf(stderr, T("%s (%.15g, %.15g, %.15g) %s\n"),
                  pszPlus, psShape->padfX[nVertex], psShape->padfY[nVertex],
                  psShape->padfZ[nVertex], pszPartType);
      }
    } // for each Vertice

    SHPComputeExtents(psShape);

    SHPWriteObject(oSHP, -1, psShape);
    SHPDestroyObject(psShape);

    iTuple = (TCHAR *)DBFReadTuple(iDBF, nEntity);
    if (iTuple == NULL) {
      // error already displayed, ignore
    }
    else {
      memcpy(oTuple, iTuple, iDBF->nRecordLength);
      DBFWriteTuple(oDBF, oDBF->nRecords, oTuple);
    }

    nPercentBefore = nPercent;
  } // for each Entity

  clock_gettime(CLOCK_REALTIME, &stop);
  tdif = (stop.tv_sec - start.tv_sec)
         + (double)(stop.tv_nsec - start.tv_nsec)/NANOSEC;
  if (debug) fprintf(stderr, T("Processing time: %f\n"), tdif);

  SHPClose(iSHP); DBFClose(iDBF);
  SHPClose(oSHP); DBFClose(oDBF);

  return 0;
} /* main */
