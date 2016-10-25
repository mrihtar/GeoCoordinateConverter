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
// conv.c: Conversion routines for main GUI and cmd-line program
//
#include "common.h"
#include "geo.h"
#include "shapefil.h"

#ifdef __cplusplus
extern "C" {
#endif

// global variables
extern char *prog; // program name

extern int debug;
extern int tr;      // transformation
extern int rev;     // reverse xy/fila

extern int gid_wgs; // selected geoid on WGS 84 (in geo.c)
extern int hsel;    // output height calculation (in geo.c)

#define EPSG_3787 0 // D48/GK
#define EPSG_3912 1 // D48/GK
#define EPSG_3794 2 // D96/TM
#define EPSG_4258 3 // ETRS89
#define EPSG_4326 4 // WGS84

char *prj[5] = {
  // prj[0] = EPSG:3787 (D48/GK)
  "PROJCS[\"MGI / Slovene National Grid\",GEOGCS[\"MGI\",DATUM[\"D_MGI\",SPHEROID[\"Bessel_1841\",6377397.155,299.1528128]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.017453292519943295]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",15],PARAMETER[\"scale_factor\",0.9999],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",-5000000],UNIT[\"Meter\",1]]",

  // prj[1] = EPSG:3912 (D48/GK)
  "PROJCS[\"MGI 1901 / Slovene National Grid\",GEOGCS[\"MGI 1901\",DATUM[\"D_MGI_1901\",SPHEROID[\"Bessel_1841\",6377397.155,299.1528128]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.017453292519943295]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",15],PARAMETER[\"scale_factor\",0.9999],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",-5000000],UNIT[\"Meter\",1]]",

  // prj[2] = EPSG:3794 (D96/TM)
  "PROJCS[\"Slovenia 1996 / Slovene National Grid\",GEOGCS[\"Slovenia 1996\",DATUM[\"D_Slovenia_Geodetic_Datum_1996\",SPHEROID[\"GRS_1980\",6378137,298.257222101]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.017453292519943295]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",15],PARAMETER[\"scale_factor\",0.9999],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",-5000000],UNIT[\"Meter\",1]]",

  // prj[3] = EPSG:4258 (ETRS89)
  "GEOGCS[\"ETRS89\",DATUM[\"D_ETRS_1989\",SPHEROID[\"GRS_1980\",6378137,298.257222101]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.017453292519943295]]",

  // prj[4] = EPSG:4326 (WGS84)
  "GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137,298.257223563]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.017453292519943295]]"
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
// convert_shp_file
// ellipsoid_init() and params_init() must be called before this!
// ----------------------------------------------------------------------------
int convert_shp_file(char *inpurl, char *outurl, char *msg)
{
  char *s, err[MAXS+1], *errtxt, buf[BUFSIZ];
  int warn;
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

  if (inpurl == NULL) return 1;
  if (msg != NULL) msg[0] = '\0';

  xstrncpy(inpname, inpurl, MAXS);
  if (outurl == NULL) outurl = inpurl;

  // Check output files
  if (fefind(outurl, ".shp", outname) == NULL) {
    snprintf(err, MAXS, "%s: file already exists\n", outname);
    if (msg == NULL) fprintf(stderr, "%s", err);
    else xstrncat(msg, err, MAXL);
    return 3;
  }
  if (fefind(outname, ".prj", prjname) == NULL) {
    snprintf(err, MAXS, "%s: file already exists\n", prjname);
    if (msg == NULL) fprintf(stderr, "%s", err);
    else xstrncat(msg, err, MAXL);
    return 3;
  }

  // Open input files (shp, dbf)
  setvbuf(stderr, buf, _IOFBF, sizeof(buf));
  iSHP = SHPOpen(inpname, "r+b");
  if (iSHP == NULL) {
    if (msg == NULL) { /* error already displayed */ }
    else xstrncat(msg, buf, MAXL);
    setvbuf(stderr, NULL, _IONBF, 0);
    return 2;
  }
  iDBF = DBFOpen(inpname, "r+b");
  if (iDBF == NULL) {
    if (msg == NULL) { /* error already displayed */ }
    else xstrncat(msg, buf, MAXL);
    setvbuf(stderr, NULL, _IONBF, 0);
    return 2;
  }
  setvbuf(stderr, NULL, _IONBF, 0);

  if (debug) fprintf(stderr, "Processing %s\n", inpname);
  clock_gettime(CLOCK_REALTIME, &start);

  SHPGetInfo(iSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound);
  if (debug)
    fprintf(stderr, "Shapefile type: %s, number of shapes: %d\n",
            SHPTypeName(nShapeType), nEntities);
  if (debug > 3)
    fprintf(stderr, "File bounds: (%.15g, %.15g, %.15g, %.15g)\n"
                    "         to: (%.15g, %.15g, %.15g, %.15g)\n",
            adfMinBound[0], adfMinBound[1], adfMinBound[2], adfMinBound[3],
            adfMaxBound[0], adfMaxBound[1], adfMaxBound[2], adfMaxBound[3]);

  // create output files (shp, dbf)
  setvbuf(stderr, buf, _IOFBF, sizeof(buf));
  oSHP = SHPCreate(outname, nShapeType);
  if (oSHP == NULL) {
    if (msg == NULL) { /* error already displayed */ }
    else xstrncat(msg, buf, MAXL);
    setvbuf(stderr, NULL, _IONBF, 0);
    return 2;
  }
  oDBF = DBFCloneEmpty(iDBF, outname);
  if (oDBF == NULL) {
    if (msg == NULL) { /* error already displayed */ }
    else xstrncat(msg, buf, MAXL);
    setvbuf(stderr, NULL, _IONBF, 0);
    return 2;
  }
  setvbuf(stderr, NULL, _IONBF, 0);

  oTuple = (char *)malloc(oDBF->nRecordLength + 15);
  if (oTuple == NULL) {
    errtxt = xstrerror();
    if (errtxt != NULL) {
      snprintf(err, MAXS, "malloc(oTuple): %s\n", errtxt); free(errtxt);
    } else
      snprintf(err, MAXS, "malloc(oTuple): Can't allocate memory\n");
    if (msg == NULL) fprintf(stderr, "%s", err);
    else xstrncat(msg, err, MAXL);
    return 4;
  }

  // determine output projection
  switch (tr) {
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
    default: // xy (d96tm) --> fila (etrs89)
      proj = prj[EPSG_4326]; break; // WGS84
  }
  // write selected projection to .prj file (for GIS programs)
  out = utf8_fopen(prjname, "w");
  if (out == NULL) {
    errtxt = xstrerror();
    if (errtxt != NULL) {
      snprintf(err, MAXS, "%s: %s\n", prjname, errtxt); free(errtxt);
    } else
      snprintf(err, MAXS, "%s: Can't open PRJ file for writing\n", prjname);
    if (msg == NULL) fprintf(stderr, "%s", err);
    else xstrncat(msg, err, MAXL);
    // ignore
  }
  else {
    fprintf(out, "%s\n", proj);
    fclose(out);
  }

  // process entitites (shapes) in input shapefile
  warn = 1; nPercentBefore = -1;
  for (nEntity = 0; nEntity < nEntities; nEntity++) {
    psShape = SHPReadObject(iSHP, nEntity);
    if (psShape == NULL) {
      snprintf(err, MAXS, "%s: unable to read shape %d, terminate reading\n",
               inpname, nEntity);
      if (msg == NULL) fprintf(stderr, "%s", err);
      else xstrncat(msg, err, MAXL);
      break;
    }

    nPercent = (long long int)nEntity*10000/nEntities;
    if (debug > 2) {
      fprintf(stderr, "Shape: %d (%s), Vertices: %d, Parts: %d\n",
              nEntity, SHPTypeName(psShape->nSHPType),
              psShape->nVertices, psShape->nParts);
      if (debug > 3) {
        if (psShape->bMeasureIsUsed)
          fprintf(stderr, "Shape bounds: (%.15g, %.15g, %.15g, %.15g)\n"
                          "          to: (%.15g, %.15g, %.15g, %.15g)\n",
            psShape->dfXMin, psShape->dfYMin, psShape->dfZMin, psShape->dfMMin,
            psShape->dfXMax, psShape->dfYMax, psShape->dfZMax, psShape->dfMMax);
        else
          fprintf(stderr, "Shape bounds: (%.15g, %.15g, %.15g)\n"
                          "          to: (%.15g, %.15g, %.15g)\n",
            psShape->dfXMin, psShape->dfYMin, psShape->dfZMin,
            psShape->dfXMax, psShape->dfYMax, psShape->dfZMax);
      }
    }
    else if (debug == 2 && nPercent > nPercentBefore)
      fprintf(stderr, "Shape: %d (%.2f%%)\r", nEntity, (double)nPercent/100.0);
    nPercentBefore = nPercent;

    if (psShape->nParts > 0 && psShape->panPartStart[0] != 0) {
      snprintf(err, MAXS, "%s: panPartStart[0] = %d, not zero as expected\n",
               inpname, psShape->panPartStart[0]);
      if (msg == NULL) fprintf(stderr, "%s", err);
      else xstrncat(msg, err, MAXL);
      // ignore
    }

    // process vertices in a shape
    nPart = 1;
    for (nVertex = 0; nVertex < psShape->nVertices; nVertex++) {
      pszPartType = "";
      if (nVertex == 0 && psShape->nParts > 0)
        pszPartType = (char *)SHPPartTypeName(psShape->panPartType[0]);

      if (nPart < psShape->nParts && psShape->panPartStart[nPart] == nVertex) {
        pszPartType = (char *)SHPPartTypeName(psShape->panPartType[nPart]);
        nPart++; pszPlus = "+";
      }
      else pszPlus = " ";

      // prepare data for transformation
      if (tr == 2 || tr == 4 || tr == 10) { // etrs89
        ifl.fi = psShape->padfY[nVertex]; ifl.la = psShape->padfX[nVertex]; // reverse!
        ifl.h = psShape->padfZ[nVertex];
        if (rev) swapfila(&ifl);
        if (ifl.la > 17.0) {
          if (warn) {
            snprintf(err, MAXS, "%s: possibly reversed fi/la\n", inpname);
            if (msg == NULL) fprintf(stderr, "%s", err);
            else xstrncat(msg, err, MAXL);
            warn = 0;
          }
        }
      }
      else { // tr == 1,3,5,6,7,8,9 // d96tm/d48gk
        ixy.x = psShape->padfY[nVertex]; ixy.y = psShape->padfX[nVertex]; // reverse!
        ixy.H = psShape->padfZ[nVertex];
        if (rev) swapxy(&ixy);
        if (ixy.y < 200000.0) {
          ixy.y += 500000.0;
          if (warn) {
            snprintf(err, MAXS, "%s: possibly reversed x/y\n", inpname);
            if (msg == NULL) fprintf(stderr, "%s", err);
            else xstrncat(msg, err, MAXL);
            warn = 0;
          }
        }
      }

      // transform data
      switch (tr) {
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
        default: // xy (d96tm) --> fila (etrs89)
          tmxy2fila_wgs(ixy, &ofl); break;
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
          fprintf(stderr, "%s (%.15g, %.15g, %.15g, %.15g) %s\n",
                  pszPlus, psShape->padfX[nVertex], psShape->padfY[nVertex],
                  psShape->padfZ[nVertex], psShape->padfM[nVertex],
                  pszPartType);
        else
          fprintf(stderr, "%s (%.15g, %.15g, %.15g) %s\n",
                  pszPlus, psShape->padfX[nVertex], psShape->padfY[nVertex],
                  psShape->padfZ[nVertex], pszPartType);
      }
    } // for each Vertice

    SHPComputeExtents(psShape);

    SHPWriteObject(oSHP, -1, psShape);
    SHPDestroyObject(psShape);

    setvbuf(stderr, buf, _IOFBF, sizeof(buf));
    iTuple = (char *)DBFReadTuple(iDBF, nEntity);
    if (iTuple == NULL) {
      if (msg == NULL) { /* error already displayed */ }
      else xstrncat(msg, buf, MAXL);
      // ignore
    }
    else {
      memcpy(oTuple, iTuple, iDBF->nRecordLength);
      DBFWriteTuple(oDBF, oDBF->nRecords, oTuple);
    }
    setvbuf(stderr, NULL, _IONBF, 0);
  } // for each Entity

  clock_gettime(CLOCK_REALTIME, &stop);
  tdif = (stop.tv_sec - start.tv_sec)
         + (double)(stop.tv_nsec - start.tv_nsec)/NANOSEC;
  if (debug) fprintf(stderr, "Processing time: %f\n", tdif);

  SHPClose(iSHP); DBFClose(iDBF);
  SHPClose(oSHP); DBFClose(oDBF);

  return 0;
} /* convert_shp_file */

#ifdef __cplusplus
}
#endif
