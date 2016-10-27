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
extern int wdms;    // write DMS

extern int gid_wgs; // selected geoid on WGS 84 (in geo.c)
extern int hsel;    // output height calculation (in geo.c)

// ----------------------------------------------------------------------------
// convert_xyz_file
// ellipsoid_init() and params_init() must be called before this!
// ----------------------------------------------------------------------------
int convert_xyz_file(char *url, int outf, FILE *out, char *msg)
{
  char *s, err[MAXS+1], *errtxt;
  char inpname[MAXS+1], outname[MAXS+1];
  FILE *inp;
  char line[MAXS+1], col1[MAXS+1];
  int inpf, ln, n, warn;
  double fi, la, h, x, y, H, tmp;
  GEOGRA fl; GEOUTM xy, gkxy, tmxy;
  DMS lat, lon;
  struct timespec start, stop;
  double tdif;

  if (url == NULL) return 1;
  if (msg != NULL) msg[0] = '\0';

  // Open input file
  if (strcmp(url, "-") == 0) {
    xstrncpy(inpname, "<stdin>", MAXS);
    inp = stdin; inpf = 1;
  }
  else {
    xstrncpy(inpname, url, MAXS);
    inp = utf8_fopen(inpname, "r"); inpf = 2;
  }
  if (inp == NULL) {
    errtxt = xstrerror();
    if (errtxt != NULL) {
      snprintf(err, MAXS, "%s: %s\n", inpname, errtxt); free(errtxt);
    } else
      snprintf(err, MAXS, "%s: Can't open input file for reading\n", inpname);
    if (msg == NULL) fprintf(stderr, "%s", err);
    else xstrncat(msg, err, MAXL);
    return 2;
  }

  // Open output file
  if (outf == 2) { // convert to separate files
    if (fefind(url, ".out", outname) == NULL) {
      snprintf(err, MAXS, "%s: file already exists\n", outname);
      if (msg == NULL) fprintf(stderr, "%s", err);
      else xstrncat(msg, err, MAXL);
      if (inpf == 2) fclose(inp);
      return 3;
    }
    out = utf8_fopen(outname, "w");
    if (out == NULL) {
      errtxt = xstrerror();
      if (errtxt != NULL) {
        snprintf(err, MAXS, "%s: %s\n", outname, errtxt); free(errtxt);
      } else
        snprintf(err, MAXS, "%s: Can't open output file for writing\n", outname);
      if (msg == NULL) fprintf(stderr, "%s", err);
      else xstrncat(msg, err, MAXL);
      if (inpf == 2) fclose(inp);
      return 2;
    }
  }

  if (debug) fprintf(stderr, "Processing %s\n", inpname);
  clock_gettime(CLOCK_REALTIME, &start);

  ln = 0; warn = 1;
  for ( ; ; ) {
    // Read (next) line
    s = fgets(line, MAXS, inp);
    if (ferror(inp)) {
      errtxt = xstrerror();
      if (errtxt != NULL) {
        snprintf(err, MAXS, "%s: %s\n", inpname, errtxt); free(errtxt);
      } else
        snprintf(err, MAXS, "%s: Error reading from input file\n", inpname);
      if (msg == NULL) fprintf(stderr, "%s", err);
      else xstrncat(msg, err, MAXL);
      break;
    }
    if (feof(inp)) break;
    ln++;

    fi = 0.0; la = 0.0; h = 0.0; // to keep compiler happy
    x = 0.0; y = 0.0; H = 0.0;

    // Parse line
    s = xstrtrim(line);
    if (tr == 2 || tr == 4 || tr == 10) { // etrs89
      // try with blank (SiTra)
      n = sscanf(s, "%10240s %lf %lf %lf", col1, &fi, &la, &h);
      if (n != 4) {
	n = sscanf(s, "%lf %lf %lf", &fi, &la, &h);
	if (n != 3) n = 5;
	else col1[0] = '\0';
      }
      else xstrncat(col1, " ", MAXS);
      if (n != 4 && n != 3) {
	// try again with semicolon (LIDAR)
	n = sscanf(s, "%10240[^;];%lf;%lf;%lf", col1, &fi, &la, &h);
	if (n != 4) {
	  n = sscanf(s, "%lf;%lf;%lf", &fi, &la, &h);
	  if (n != 3) n = 5;
	  else col1[0] = '\0';
	}
	else xstrncat(col1, " ", MAXS);
	if (n != 4 && n != 3) {
	  snprintf(err, MAXS, "%s: line %d: %-.75s\n", inpname, ln, line);
          if (msg == NULL) fprintf(stderr, "%s", err);
          else xstrncat(msg, err, MAXL);
	  continue;
	}
      }

      if (rev) { tmp = fi; fi = la; la = tmp; }
      if (fi == 0.0 || la == 0.0) {
	snprintf(err, MAXS, "%s: line %d: %-.75s\n", inpname, ln, line);
        if (msg == NULL) fprintf(stderr, "%s", err);
        else xstrncat(msg, err, MAXL);
	continue;
      }
      if (la > 17.0) {
	if (warn) {
          snprintf(err, MAXS, "%s: possibly reversed fi/la\n", inpname);
          if (msg == NULL) fprintf(stderr, "%s", err);
          else xstrncat(msg, err, MAXL);
          warn = 0;
        }
      }
    }
    else { // tr == 1,3,5,6,7,8,9 // d96tm/d48gk
      // try with blank (SiTra)
      n = sscanf(s, "%10240s %lf %lf %lf", col1, &y, &x, &H);
      if (n != 4) {
	n = sscanf(s, "%lf %lf %lf", &y, &x, &H);
	if (n != 3) n = 5;
	else col1[0] = '\0';
      }
      else xstrncat(col1, " ", MAXS);
      if (n != 4 && n != 3) {
	// try again with semicolon (LIDAR)
	n = sscanf(s, "%10240[^;];%lf;%lf;%lf", col1, &y, &x, &H);
	if (n != 4) {
	  n = sscanf(s, "%lf;%lf;%lf", &y, &x, &H);
	  if (n != 3) n = 5;
	  else col1[0] = '\0';
	}
	else xstrncat(col1, " ", MAXS);
	if (n != 4 && n != 3) {
	  snprintf(err, MAXS, "%s: line %d: %-.75s\n", inpname, ln, line);
          if (msg == NULL) fprintf(stderr, "%s", err);
          else xstrncat(msg, err, MAXL);
	  continue;
	}
      }

      if (rev) { tmp = x; x = y; y = tmp; }
      if (x == 0.0 || y == 0.0) {
	snprintf(err, MAXS, "%s: line %d: %-.75s\n", inpname, ln, line);
        if (msg == NULL) fprintf(stderr, "%s", err);
        else xstrncat(msg, err, MAXL);
	continue;
      }
      if (y < 200000.0) {
	y += 500000.0;
	if (warn) {
          snprintf(err, MAXS, "%s: possibly reversed x/y\n", inpname);
          if (msg == NULL) fprintf(stderr, "%s", err);
          else xstrncat(msg, err, MAXL);
          warn = 0;
        }
      }
    }

    // Convert coordinates
    if (tr == 1) { // xy (d96tm) --> fila (etrs89)
      xy.x = x; xy.y = y; xy.H = H;
      tmxy2fila_wgs(xy, &fl);
      fprintf(out, "%s%.9f %.9f %.3f", col1, fl.fi, fl.la, fl.h);
      if (wdms) {
	deg2dms(fl.fi, &lat); deg2dms(fl.la, &lon);
	fprintf(out, " %.0f %2.0f %8.5f %.0f %2.0f %8.5f\n",
	  lat.deg, lat.min, lat.sec, lon.deg, lon.min, lon.sec);
      }
      else fprintf(out, "\n");
    }

    else if (tr == 2) { // fila (etrs89) --> xy (d96tm)
      fl.fi = fi; fl.la = la; fl.h = h;
      fila_wgs2tmxy(fl, &xy);
      fprintf(out, "%s%.3f %.3f %.3f\n", col1, xy.x, xy.y, xy.H);
    }

    else if (tr == 3) { // xy (d48gk) --> fila (etrs89)
      // SiTra: h,H/H(tr), gk-slo: hsel = ht ==> OK
      // SiTra: h=0,H=0/H(tr)=h-N, gk-slo: hsel = ht ==> OK (isti input!)
      // SiTra: h=0,H=0/H(tr)=h-N, gk-slo: hsel = hg ==> 40-70cm razlike
      xy.x = x; xy.y = y; xy.H = H;
      gkxy2fila_wgs(xy, &fl);
      fprintf(out, "%s%.9f %.9f %.3f", col1, fl.fi, fl.la, fl.h);
      if (wdms) {
	deg2dms(fl.fi, &lat); deg2dms(fl.la, &lon);
	fprintf(out, " %.0f %2.0f %8.5f %.0f %2.0f %8.5f\n",
	  lat.deg, lat.min, lat.sec, lon.deg, lon.min, lon.sec);
      }
      else fprintf(out, "\n");
    }

    else if (tr == 4) { // fila (etrs89) --> xy (d48gk)
      // SiTra: h,H/H(tr), gk-slo: hsel = ht ==> OK
      // SiTra: h=0,H=0/H(tr)=h-N, gk-slo: hsel = hg ==> OK
      // SiTra: h=0,H=0/H(tr)=h-N, gk-slo: hsel = ht ==> 40-70cm razlike
      fl.fi = fi; fl.la = la; fl.h = h;
      fila_wgs2gkxy(fl, &xy);
      fprintf(out, "%s%.3f %.3f %.3f\n", col1, xy.x, xy.y, xy.H);
    }

    else if (tr == 5) { // xy (d48gk) --> xy (d96tm)
      xy.x = x; xy.y = y; xy.H = H;
      gkxy2tmxy(xy, &tmxy);
      fprintf(out, "%s%.3f %.3f %.3f\n", col1, tmxy.x, tmxy.y, tmxy.H);
    }

    else if (tr == 6) { // xy (d96tm) --> xy (d48gk)
      xy.x = x; xy.y = y; xy.H = H;
      tmxy2gkxy(xy, &gkxy);
      fprintf(out, "%s%.3f %.3f %.3f\n", col1, gkxy.x, gkxy.y, gkxy.H);
    }

    else if (tr == 7) { // xy (d48gk) --> xy (d96tm), affine trans.
      xy.x = x; xy.y = y; xy.H = H;
      gkxy2tmxy_aft(xy, &tmxy);
      fprintf(out, "%s%.3f %.3f %.3f\n", col1, tmxy.x, tmxy.y, tmxy.H);
    }

    else if (tr == 8) { // xy (d96tm) --> xy (d48gk), affine trans.
      xy.x = x; xy.y = y; xy.H = H;
      tmxy2gkxy_aft(xy, &gkxy);
      fprintf(out, "%s%.3f %.3f %.3f\n", col1, gkxy.x, gkxy.y, gkxy.H);
    }

    else if (tr == 9) { // xy (d48gk) --> fila (etrs89), affine trans.
      xy.x = x; xy.y = y; xy.H = H;
      gkxy2fila_wgs_aft(xy, &fl);
      fprintf(out, "%s%.9f %.9f %.3f", col1, fl.fi, fl.la, fl.h);
      if (wdms) {
	deg2dms(fl.fi, &lat); deg2dms(fl.la, &lon);
	fprintf(out, " %.0f %2.0f %8.5f %.0f %2.0f %8.5f\n",
	  lat.deg, lat.min, lat.sec, lon.deg, lon.min, lon.sec);
      }
      else fprintf(out, "\n");
    }

    else if (tr == 10) { // fila (etrs89) --> xy (d48gk), affine trans.
      fl.fi = fi; fl.la = la; fl.h = h;
      fila_wgs2gkxy_aft(fl, &xy);
      fprintf(out, "%s%.3f %.3f %.3f\n", col1, xy.x, xy.y, xy.H);
    }
  } // while !eof

  clock_gettime(CLOCK_REALTIME, &stop);
  tdif = (stop.tv_sec - start.tv_sec)
         + (double)(stop.tv_nsec - start.tv_nsec)/NANOSEC;
  if (debug) fprintf(stderr, "Processing time: %f\n", tdif);

  if (inpf == 2) fclose(inp);
  if (outf == 2) fclose(out);

  return 0;
} /* convert_xyz_file */

#ifdef __cplusplus
}
#endif
