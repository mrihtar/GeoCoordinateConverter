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
// xgk-slo.cxx: Main GUI program for converting coordinates from XYZ files
//
#include "common.h"
#include "geo.h"

#include <pthread.h>
#ifdef _MSC_VER // Microsoft C
#define OLD_PTHREADS
#endif

#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Sys_Menu_Bar.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Help_Dialog.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>

#ifdef _WIN32
#include <Shellapi.h>
#else
#include <X11/xpm.h>
#include "globe.xpm"
#endif

#define SW_VERSION "1.36"
#define SW_BUILD   "May 11, 2016"

#define HELP "xgk-help.html"

// See https://en.wikipedia.org/wiki/Wikipedia:WikiProject_Geographical_coordinates#Parameters
#define SCALE 5000

#ifdef __cplusplus
extern "C" {
#endif

// global variables
char *prog;  // program name
int debug;
int tr;      // transformation
int rev;     // reverse xy/fila
int wdms;    // write DMS
extern int gid_wgs; // selected geoid on WGS 84 (in geo.c)
extern int hsel;    // output height calculation (in geo.c)
int ft;      // file type (XYZ/SHP)
int ddms;    // display DMS

char argv0[MAXS+1]; // original argv[0]
char path0[MAXS+1]; // path of argv[0], len > PATH_MAX

#ifdef __cplusplus
}
#endif

// thread variables
typedef struct ptid {
  // WIN32, OLD_PTHREADS { void *p; unsigned int x; }, UNIX: unsigned long
  pthread_t tid;
  int done;
  int sts;
  struct ptid *next;
} PTID;

PTID *threads, *threads0;
pthread_attr_t pattr;
pthread_mutex_t xlog_mutex; // xlog mutex
#define MAXTN 25 // maximum number of allowed active threads
pthread_mutex_t tn_mutex; // threads number mutex
int tn; // number of active treads

// thread arguments
typedef struct targ {
  char text[MAXL+1];
  Fl_Widget *w;
} TARG;

const char *browsers[] = { // Possible UNIX browsers
  "chrome", "firefox", "opera", "epiphany", "mozilla",
  "netscape", "konqueror", "nautilus", "safari"
};

#ifdef __cplusplus
extern "C" {
#endif
// External function prototypes
int convert_xyz_file(char *url, int outf, FILE *out, char *msg); // in conv.c
int convert_shp_file(TCHAR *inpurl, TCHAR *outurl, TCHAR *msg); // in conv.c
#ifdef __cplusplus
}
#endif

// FLTK callback function prototypes
void open_cb(Fl_Widget *w, void *p);
void quit_cb(Fl_Widget *w, void *p);
void help_cb(Fl_Widget *w, void *p);
void clear_cb(Fl_Widget *w, void *p);
void about_cb(Fl_Widget *w, void *p);
void ftchoice_cb(Fl_Widget *w, void *p);

// FLTK global variables
Fl_Menu_Item menubar_entries[] = {
  {"&File", 0, 0, 0, FL_SUBMENU},
    {"&Open", FL_ALT+'o',  open_cb, 0, FL_MENU_DIVIDER},
    {"&Quit", FL_ALT+'q',  quit_cb},
    {0},
  {"&Edit", 0, 0, 0, FL_SUBMENU},
    {"Undo",  FL_CTRL+'z', 0},
    {"Redo",  FL_CTRL+'r', 0, 0, FL_MENU_DIVIDER},
    {"Cut",   FL_CTRL+'x', 0},
    {"Copy",  FL_CTRL+'c', 0},
    {"Paste", FL_CTRL+'v', 0, 0, FL_MENU_DIVIDER},
    {"Clear", 0,           clear_cb},
    {0},
  {"&Help", 0, 0, 0, FL_SUBMENU},
    {"Help",  FL_ALT+'h',  help_cb, 0, FL_MENU_DIVIDER},
    {"About", 0,           about_cb},
    {0},
  {0}
}; /* menubar_entries */

Fl_Radio_Round_Button *rb_trans[MAXC];
Fl_Radio_Round_Button *rb_geoid[MAXC];
Fl_Radio_Round_Button *rb_height[MAXC];

Fl_Menu_Item ft_choices[] = {
  {"XYZ files", 0, ftchoice_cb, (void *)1},
  {"SHP files", 0, ftchoice_cb, (void *)2},
  {0}
}; /* ft_choices */

Fl_Group *tab1, *tab2;
Fl_Browser *brow;
Fl_Input *input[MAXC];
Fl_Output *output[MAXC];
Fl_Radio_Round_Button *rb_dms[MAXC];
Fl_Group *gtr1, *gtr2, *gtr3;

// ----------------------------------------------------------------------------
// Fl_DND_Box
// ----------------------------------------------------------------------------
class Fl_DND_Box : public Fl_Box
{
  public:
    Fl_DND_Box(int X, int Y, int W, int H, const char *L = 0) :
      Fl_Box(X,Y,W,H,L), evt(FL_NO_EVENT), evt_txt(0), evt_len(0)
    {
      labeltype(FL_NO_LABEL);
      box(FL_NO_BOX);
      clear_visible_focus();
    } /* Fl_DND_Box ctor */

    virtual ~Fl_DND_Box() {
      delete [] evt_txt;
    } /* Fl_DND_Box dtor */

    int event() {
      return evt;
    } /* event */

    const char *event_text() {
      return evt_txt;
    } /* event_text */

    int event_length() {
      return evt_len;
    } /* event_length */

    static void callback_deferred(void *w) {
      Fl_DND_Box *dnd = (Fl_DND_Box *)w;
      dnd->do_callback();
    } /* callback_deferred */

    int handle(int e) {
      switch (e) {
        // return 1 for these events to 'accept' DND
        case FL_DND_ENTER:
        case FL_DND_RELEASE:
        case FL_DND_LEAVE:
        case FL_DND_DRAG:
          evt = e;
          return 1;

        // handle actual drop (paste) operation
        case FL_PASTE:
          evt = e;

          // make a copy of the DND payload
          evt_len = Fl::event_length();

          delete [] evt_txt;
          evt_txt = new char[evt_len+1];
          xstrncpy(evt_txt, Fl::event_text(), evt_len);

          // If there is a callback registered, call it, but not directly.
          // The callback will be executed by the FLTK main-loop once we have
          // finished handling the DND event. This allows caller to popup
          // a window or change widget focus.
          if (callback() && ((when() & FL_WHEN_RELEASE) || (when() & FL_WHEN_CHANGED)))
            Fl::add_timeout(0.0, Fl_DND_Box::callback_deferred, (void *)this);
          return 1;
      } // switch

      // pass other events to default event handling of Fl_Box
      return Fl_Box::handle(e);
    } /* handle */

  protected:
    int evt; // The event which caused Fl_DND_Box to execute its callback
    char *evt_txt;
    int evt_len;
}; /* Fl_DND_Box */


// ----------------------------------------------------------------------------
// Fl_Arrow_Box
// ----------------------------------------------------------------------------
class Fl_Arrow_Box : public Fl_Box
{
  public:
    Fl_Arrow_Box(int X, int Y, int W, int H, const char *L = 0) :
      Fl_Box(X,Y,W,H,L)
    {
      labeltype(FL_NO_LABEL);
      box(FL_NO_BOX);
      clear_visible_focus();
    } /* Fl_Arrow_Box ctor */

  protected:
    void draw() {
      Fl_Color savec = fl_color();
      Fl_Box::draw();

      fl_color(FL_BLUE);
      fl_line_style(FL_SOLID | FL_CAP_ROUND | FL_JOIN_ROUND, 3);
      fl_line(x()+10, y()+h()/2-1, x()+w()-10, y()+h()/2-1);
      fl_line(x()+w()-20, y()+h()/2-7, x()+w()-10, y()+h()/2-1);
      fl_line(x()+w()-20, y()+h()/2+5, x()+w()-10, y()+h()/2-1);

      fl_line_style(0);
      fl_color(savec);
    } /* draw */
}; /* Fl_Arrow_Box */


// ----------------------------------------------------------------------------
// xlog
// ----------------------------------------------------------------------------
int xlog(const char *fmt, ...)
{
  va_list ap;
  time_t now;
  struct tm *tmnow;
  char stime[MAXS+1];
  char *msg;
  char logfile[MAXS+1];
  FILE *log;

  pthread_mutex_lock(&xlog_mutex);

  now = time(NULL);
  tmnow = localtime(&now);
//strftime(stime, MAXS, "%y-%m-%d %H:%M:%S", tmnow);
  strftime(stime, MAXS, "%H:%M:%S", tmnow);

  msg = new char[MAXL+1];

  va_start(ap, fmt);
  vsnprintf(msg, MAXL, fmt, ap);
  va_end(ap);

  snprintf(logfile, MAXS, "%s.log", prog);
  log = utf8_fopen(logfile, "a");
  if (log != NULL) {
    fprintf(log, "%s %s", stime, msg);
    fclose(log);
  }

  delete msg;

  pthread_mutex_unlock(&xlog_mutex);
  return 0;
} /* xlog */


// ----------------------------------------------------------------------------
// xpthread_create
// ----------------------------------------------------------------------------
int xpthread_create(void *(*worker)(void *), void *arg)
{
  PTID *pt;
  int rc;

  pt = new PTID;
  if (threads == NULL) { threads = pt; threads0 = threads; }
  else { threads->next = pt; threads = threads->next; }
  threads->done = 0; threads->sts = -1;
  threads->next = NULL;

  pthread_mutex_lock(&tn_mutex);
  rc = pthread_create(&threads->tid, &pattr, worker, arg);
  tn++;
  pthread_mutex_unlock(&tn_mutex);
  if (rc)
    if (rc == EAGAIN) // PTHREAD_THREADS_MAX = 2019
      xlog("Thread creation failed, max. number of threads exceeded\n");
    else xlog("Thread creation failed, rc = %d\n", rc);
  else
#ifdef OLD_PTHREADS
    xlog("Created thread %08x\n", threads->tid.p);
#else
    xlog("Created thread %08x\n", threads->tid);
#endif
  return rc;
} /* xpthread_create */


// ----------------------------------------------------------------------------
// THREAD: worker
// ----------------------------------------------------------------------------
void *worker(void *arg)
{
  Fl_Menu_Item *mi;
  time_t now;

  mi = (Fl_Menu_Item *)arg;
#ifdef OLD_PTHREADS
  xlog("THREAD %08x: worker: menu %s\n", pthread_self().p, mi->label());
#else
  xlog("THREAD %08x: worker: menu %s\n", pthread_self(), mi->label());
#endif

  now = time(NULL);

  pthread_mutex_lock(&tn_mutex);
  tn--;
  pthread_mutex_unlock(&tn_mutex);
  pthread_exit((void *)(intptr_t)now);
  return NULL;
} /* worker */

// ----------------------------------------------------------------------------
// THREAD: convert
// ----------------------------------------------------------------------------
void *convert(void *arg)
{
  TARG *targ;
  char *url, orig_url[MAXS+1];
  char line[MAXS+1], *linep;
  char *msg, *msgp, *s;
  int sts;

  targ = (TARG *)arg;
  url = targ->text;
#ifdef OLD_PTHREADS
  xlog("THREAD %08x: convert: url %s\n", pthread_self().p, url);
#else
  xlog("THREAD %08x: convert: url %s\n", pthread_self(), url);
#endif

  linep = line;

  snprintf(linep, MAXS, "Converting %s\n", url);
  Fl::lock();
  brow->add(linep); brow->bottomline(brow->size());
  Fl::unlock();
  Fl::awake((void *)NULL);

  msg = new char[MAXL+1];
  xstrncpy(orig_url, url, MAXS);

  if (ft == 1) // XYZ files
    sts = convert_xyz_file(url, 2, NULL, msg); // convert to separate files
  else { // SHP files
    if ((s = strrchr(url, '.')) != NULL) *s = '\0'; // clear current extension
    xstrncat(url, ".shp", MAXL); // look for <name>.shp

    sts = convert_shp_file(url, NULL, msg);
  }

  if (strlen(msg) > 0) {
    xlog("%s", msg); // must use %s here!

    Fl::lock();
    s = xstrtok_r(msg, "\r\n", &msgp);
    while (s != NULL) {
      snprintf(linep, MAXS, "@C1%s", s); // write errors in red
      brow->add(linep); brow->bottomline(brow->size());
      s = xstrtok_r(NULL, "\r\n", &msgp);
    }
    Fl::unlock();
    Fl::awake((void *)NULL);
  }

  if (sts == 0) {
    if (strlen(msg) > 0) // warning: dark magenta
      snprintf(linep, MAXS, "@C13Finished converting %s\n", orig_url);
    else // all OK: dark green
      snprintf(linep, MAXS, "@C10Finished converting %s\n", orig_url);
  }
  else // error: dark red
    snprintf(linep, MAXS, "@C9Finished converting %s\n", orig_url);

  Fl::lock();
  brow->add(linep); brow->bottomline(brow->size());
  Fl::unlock();
  Fl::awake((void *)NULL);

  delete msg;
  delete targ;
  pthread_mutex_lock(&tn_mutex);
  tn--;
  pthread_mutex_unlock(&tn_mutex);
  pthread_exit((void *)(intptr_t)sts);
  return NULL;
} /* convert */


// ----------------------------------------------------------------------------
// THREAD: convert_all
// ----------------------------------------------------------------------------
void *convert_all(void *arg)
{
  char *urls, *url, *sp, *s;
  int ii, len, rc;
  TARG *targs, *targ;
  struct timespec req, rem;
  int sts;
  unsigned int tid;

  targs = (TARG *)arg;
  urls = targs->text;
  len = strlen(urls);
#ifdef OLD_PTHREADS
  tid = PtrToInt(pthread_self().p);
#else
  tid = pthread_self();
#endif
  if (len > 256)
    xlog("THREAD %08x: convert_all: urls %.256s...\n", tid, urls);
  else
    xlog("THREAD %08x: convert_all: urls %s\n", tid, urls);

  brow->clear();

  ii = 0; sts = 0;
  url = xstrtok_r(urls, "\r\n", &sp);
  while (url != NULL) {
    while (tn > MAXTN) { // wait until some of the threads finish
#ifdef _WIN32
      Sleep(100); // msec
#else
      req.tv_sec = 0; req.tv_nsec = 100000000L; // 100 msec
      nanosleep(&req, &rem);
#endif
    }

    url = uri2path(url); // check drag&drop file syntax on Unix (URI)

    ii++;
    xlog("URL %d: %s\n", ii, url);
 // brow->add(url); brow->bottomline(brow->size());
 // Fl::flush();

    targ = new TARG;
    xstrncpy(targ->text, url, MAXL);
    if (url != NULL) free(url);

    sts += xpthread_create(convert, (void *)targ);

    url = xstrtok_r(NULL, "\r\n", &sp);
  }

  delete targs;
  pthread_mutex_lock(&tn_mutex);
  tn--;
  pthread_mutex_unlock(&tn_mutex);
  pthread_exit((void *)(intptr_t)sts);
  return NULL;
} /* convert_all */

// ----------------------------------------------------------------------------
// parse_dms
// ----------------------------------------------------------------------------
int parse_dms(const char *str, double *val)
{
  DMS dms;
  int n;

  dms.deg = 0.0; dms.min = 0.0; dms.sec = 0.0; // to keep compiler happy
  switch (ddms) {
    case 1: // Dec. Degrees
      n = sscanf(str, "%lf", &dms.deg);
      if (n != 1) {
        // error
        dms.deg = 0.0;
      }
      break;
    case 2: // Deg. Min.
      n = sscanf(str, "%lf%*[^0-9.-]%lf", &dms.deg, &dms.min);
      if (n != 2) {
        // error
        dms.min = 0.0;
        if (n != 1) dms.deg = 0.0;
      }
      dms.min = fabs(dms.min);
      break;
    case 3: // Deg. Min. Sec.
      n = sscanf(str, "%lf%*[^0-9.-]%lf%*[^0-9.-]%lf", &dms.deg, &dms.min, &dms.sec);
      if (n != 3) {
        // error
        dms.sec = 0.0;
        if (n != 2) dms.min = 0.0;
        if (n != 1) dms.deg = 0.0;
      }
      dms.min = fabs(dms.min);
      dms.sec = fabs(dms.sec);
      break;
  } // switch (ddms)

  dms2deg(dms, val);
  return 0;
} /* parse_dms */


// ----------------------------------------------------------------------------
// parse_double
// ----------------------------------------------------------------------------
int parse_double(const char *str, double *val)
{
  int n;

  n = sscanf(str, "%lf", val);
  if (n != 1) {
    *val = 0.0;
  }
  return 0;
} /* parse_double */


// ----------------------------------------------------------------------------
// convert_cb
// Widget w and parameter p can be NULL!
// ----------------------------------------------------------------------------
void convert_cb(Fl_Widget *w, void *p)
{
//Fl_Group *gtr;
  int n;
  char value1[MAXS+1], value2[MAXS+1], value3[MAXS+1];
  double fi, la, h, x, y, H;
  GEOGRA fl; GEOUTM xy, gkxy, tmxy;
  DMS lat, lon;

//gtr = (Fl_Group *)p;
  xy.x = 0.0; xy.y = 0.0; xy.H = 0.0; // to keep compiler happy
  fl.fi = 0.0; fl.la = 0.0; fl.h = 0.0;
  switch (tr) {
    case  1: // gtr1, xy (D96/TM) ==> fila (ETRS89)
    case  3: // gtr1, xy (D48/GK) ==> fila (ETRS89)
    case  9: // gtr1, xy (D48/GK) ==> fila (ETRS89), AFT
      parse_double(input[0]->value(), &y);
      parse_double(input[1]->value(), &x);
      parse_double(input[2]->value(), &H);
      xlog("convert_cb: input(tr = %d): x = %g, y = %g, H = %g\n", tr, x, y, H);
      if (y < 200000.0) {
        // warn: possibly reversed x/y
      }
      xy.x = x; xy.y = y; xy.H = H;
      break;

    case  2: // gtr2, fila (ETRS89) ==> xy (D96/TM)
    case  4: // gtr2, fila (ETRS89) ==> xy (D48/GK)
    case 10: // gtr2, fila (ETRS89) ==> xy (D48/GK), AFT
      parse_dms(input[3]->value(), &fi);
      if (fi > 90.0 || fi < -90.0) fi = 0.0;
      parse_dms(input[4]->value(), &la);
      if (la > 180.0 || la < -180.0) la = 0.0;
      parse_double(input[5]->value(), &h);
      xlog("convert_cb: input(tr = %d): fi = %g, la = %g, h = %g\n", tr, fi, la, h);
      if (la > 17.0) {
        // warn: possibly reversed fi/la
      }
      fl.fi = fi; fl.la = la; fl.h = h;
      break;

    case  5: // gtr3, xy (D48/GK) ==> xy (D96/TM)
    case  6: // gtr3, xy (D96/TM) ==> xy (D48/GK)
    case  7: // gtr3, xy (D48/GK) ==> xy (D96/TM), AFT
    case  8: // gtr3, xy (D96/TM) ==> xy (D48/GK), AFT
      parse_double(input[6]->value(), &y);
      parse_double(input[7]->value(), &x);
      parse_double(input[8]->value(), &H);
      xlog("convert_cb: input(tr = %d): x = %g, y = %g, H = %g\n", tr, x, y, H);
      if (y < 200000.0) {
        // warn: possibly reversed x/y
      }
      xy.x = x; xy.y = y; xy.H = H;
      break;
  } // switch (tr)

  switch (tr) {
    case  1: tmxy2fila_wgs(xy, &fl); break;              // gtr1, xy (D96/TM) ==> fila (ETRS89)
    case  2: fila_wgs2tmxy(fl, &xy); break;              // gtr2, fila (ETRS89) ==> xy (D96/TM)
    case  3: gkxy2fila_wgs(xy, &fl); break;              // gtr1, xy (D48/GK) ==> fila (ETRS89)
    case  4: fila_wgs2gkxy(fl, &xy); break;              // gtr2, fila (ETRS89) ==> xy (D48/GK)
    case  5: gkxy2tmxy(xy, &tmxy); xy = tmxy; break;     // gtr3, xy (D48/GK) ==> xy (D96/TM)
    case  6: tmxy2gkxy(xy, &gkxy); xy = gkxy; break;     // gtr3, xy (D96/TM) ==> xy (D48/GK)
    case  7: gkxy2tmxy_aft(xy, &tmxy); xy = tmxy; break; // gtr3, xy (D48/GK) ==> xy (D96/TM), AFT
    case  8: tmxy2gkxy_aft(xy, &gkxy); xy = gkxy; break; // gtr3, xy (D96/TM) ==> xy (D48/GK), AFT
    case  9: gkxy2fila_wgs_aft(xy, &fl); break;          // gtr1, xy (D48/GK) ==> fila (ETRS89), AFT
    case 10: fila_wgs2gkxy_aft(fl, &xy); break;          // gtr2, fila (ETRS89) ==> xy (D48/GK), AFT
  } // switch (tr)

  switch (tr) {
    case  1: // gtr1, xy (D96/TM) ==> fila (ETRS89)
    case  3: // gtr1, xy (D48/GK) ==> fila (ETRS89)
    case  9: // gtr1, xy (D48/GK) ==> fila (ETRS89), AFT
      xlog("convert_cb: output(tr = %d): fi = %.9f, la = %.9f, h = %.3f\n", tr, fl.fi, fl.la, fl.h);
      switch (ddms) {
        case 1: // Dec. Degrees
          snprintf(value1, MAXS, "%.9f", fl.fi);
          snprintf(value2, MAXS, "%.9f", fl.la);
          snprintf(value3, MAXS, "%.3f", fl.h);
          break;
        case 2: // Deg. Min.
          deg2dm(fl.fi, &lat); deg2dm(fl.la, &lon);
          snprintf(value1, MAXS, "%.0f\xB0 %.7f'", lat.deg, lat.min);
          snprintf(value2, MAXS, "%.0f\xB0 %.7f'", lon.deg, lon.min);
          snprintf(value3, MAXS, "%.3f", fl.h);
          break;
        case 3: // Deg. Min. Sec.
          deg2dms(fl.fi, &lat); deg2dms(fl.la, &lon);
          snprintf(value1, MAXS, "%.0f\xB0 %.0f' %.5f\"", lat.deg, lat.min, lat.sec);
          snprintf(value2, MAXS, "%.0f\xB0 %.0f' %.5f\"", lon.deg, lon.min, lon.sec);
          snprintf(value3, MAXS, "%.3f", fl.h);
          break;
      } // switch (ddms)
      output[0]->value(value1);
      output[1]->value(value2);
      output[2]->value(value3);
      break;

    case  2: // gtr2, fila (ETRS89) ==> xy (D96/TM)
    case  4: // gtr2, fila (ETRS89) ==> xy (D48/GK)
    case 10: // gtr2, fila (ETRS89) ==> xy (D48/GK), AFT
      xlog("convert_cb: output(tr = %d): x = %.9f, y = %.9f, H = %.3f\n", tr, xy.x, xy.y, xy.H);
      snprintf(value1, MAXS, "%.3f", xy.y);
      snprintf(value2, MAXS, "%.3f", xy.x);
      snprintf(value3, MAXS, "%.3f", xy.H);
      output[3]->value(value1);
      output[4]->value(value2);
      output[5]->value(value3);
      break;

    case  5: // gtr3, xy (D48/GK) ==> xy (D96/TM)
    case  6: // gtr3, xy (D96/TM) ==> xy (D48/GK)
    case  7: // gtr3, xy (D48/GK) ==> xy (D96/TM), AFT
    case  8: // gtr3, xy (D96/TM) ==> xy (D48/GK), AFT
      xlog("convert_cb: output(tr = %d): x = %.9f, y = %.9f, H = %.3f\n", tr, xy.x, xy.y, xy.H);
      snprintf(value1, MAXS, "%.3f", xy.y);
      snprintf(value2, MAXS, "%.3f", xy.x);
      snprintf(value3, MAXS, "%.3f", xy.H);
      output[6]->value(value1);
      output[7]->value(value2);
      output[8]->value(value3);
      break;
  } // switch (tr)
} /* convert_cb */


// ----------------------------------------------------------------------------
// open_url
// ----------------------------------------------------------------------------
int open_url(const char *url)
{
  int rc, ii, blen;
  char buf[MAXS+1], cmd[MAXS+1], *path;
#ifdef _WIN32
  HINSTANCE hi;
#endif

#ifdef _WIN32
  hi = ShellExecute(HWND_DESKTOP, "open", url, NULL, NULL, SW_SHOWDEFAULT);
  rc = PtrToInt(hi);
  if (rc > 32) rc = 0;
  else if (rc == 0) rc = ENOMEM;
#else
  rc = 1;
  blen = sizeof(browsers)/sizeof(*browsers);
  for (ii = 0; ii < blen; ii++) {
    if ((path = locate_exe(browsers[ii], buf, MAXS)) == NULL)
      continue;
    snprintf(cmd, MAXS, "\"%s\" '%s' >/dev/null 2>&1 &", path, url);
    rc = system(cmd);
    break;
  }
#endif
  return rc;
} /* open_url */


// ----------------------------------------------------------------------------
// show_cb
// ----------------------------------------------------------------------------
void show_cb(Fl_Widget *w, void *p)
{
  double fi, la, h, x, y, H;
  GEOGRA fl; GEOUTM xy;
  char fip, lap, url[MAXS+1];

  fi = 0.0; la = 0.0;
  fl.fi = 0.0; fl.la = 0.0;
  switch (tr) {
    case  1: // gtr1, xy (D96/TM) ==> fila (ETRS89)
    case  3: // gtr1, xy (D48/GK) ==> fila (ETRS89)
    case  9: // gtr1, xy (D48/GK) ==> fila (ETRS89), AFT
      parse_dms(output[0]->value(), &fi);
      if (fi > 90.0 || fi < -90.0) fi = 0.0;
      parse_dms(output[1]->value(), &la);
      if (la > 180.0 || la < -180.0) la = 0.0;

      if (fi == 0.0 || la == 0.0) {
        parse_double(input[0]->value(), &y);
        parse_double(input[1]->value(), &x);
        xy.x = x; xy.y = y; xy.H = 0;
        switch (tr) {
          case  1: tmxy2fila_wgs(xy, &fl); break;              // gtr1, xy (D96/TM) ==> fila (ETRS89)
          case  3: gkxy2fila_wgs(xy, &fl); break;              // gtr1, xy (D48/GK) ==> fila (ETRS89)
          case  9: gkxy2fila_wgs_aft(xy, &fl); break;          // gtr1, xy (D48/GK) ==> fila (ETRS89), AFT
        } // switch (tr)
        fi = fl.fi; la = fl.la;
      }
      break;

    case  2: // gtr2, fila (ETRS89) ==> xy (D96/TM)
    case  4: // gtr2, fila (ETRS89) ==> xy (D48/GK)
    case 10: // gtr2, fila (ETRS89) ==> xy (D48/GK), AFT
      parse_dms(input[3]->value(), &fi);
      if (fi > 90.0 || fi < -90.0) fi = 0.0;
      parse_dms(input[4]->value(), &la);
      if (la > 180.0 || la < -180.0) la = 0.0;
      break;

    case  5: // gtr3, xy (D48/GK) ==> xy (D96/TM)
    case  6: // gtr3, xy (D96/TM) ==> xy (D48/GK)
    case  7: // gtr3, xy (D48/GK) ==> xy (D96/TM), AFT
    case  8: // gtr3, xy (D96/TM) ==> xy (D48/GK), AFT
      parse_double(input[6]->value(), &y);
      parse_double(input[7]->value(), &x);
      xy.x = x; xy.y = y; xy.H = 0;
      switch (tr) {
        case  5: gkxy2fila_wgs_aft(xy, &fl); break;          // gtr3, xy (D48/GK) ==> fila (ETRS89), AFT
        case  6: tmxy2fila_wgs(xy, &fl); break;              // gtr3, xy (D96/TM) ==> fila (ETRS89)
        case  7: gkxy2fila_wgs_aft(xy, &fl); break;          // gtr3, xy (D48/GK) ==> fila (ETRS89), AFT
        case  8: tmxy2fila_wgs(xy, &fl); break;              // gtr3, xy (D96/TM) ==> fila (ETRS89)
      } // switch (tr)
      fi = fl.fi; la = fl.la;
      break;
  } // switch (tr)

  fip = 'N'; if (fi < 0.0) fip = 'S';
  lap = 'E'; if (la < 0.0) lap = 'W';
  snprintf(url, MAXS, "https://tools.wmflabs.org/geohack/geohack.php?params=%.9f_%c_%.9f_%c_scale:%d\n",
    fabs(fi), fip, fabs(la), lap, SCALE);

  open_url(url);
} /* show_cb */


// ----------------------------------------------------------------------------
// mainwin_cb
// ----------------------------------------------------------------------------
void mainwin_cb(Fl_Widget *w, void *p)
{
  Fl_Double_Window *mw;

  mw = (Fl_Double_Window *)w;
  xlog("mainwin_cb\n");
  mw->hide(); // not really needed (already default action)
} /* window_cb */


// ----------------------------------------------------------------------------
// menu_cb
// ----------------------------------------------------------------------------
void menu_cb(Fl_Widget *w, void *p)
{
  Fl_Menu_ *m;
  const Fl_Menu_Item *mi;
  int rc;

  m = (Fl_Menu_ *)w;
  mi = m->mvalue();
  if (!mi)
    xlog("menu_cb: mi = NULL\n");
  else if (mi->shortcut()) {
    xlog("menu_cb: %s (%s)\n", mi->label(), fl_shortcut_label(mi->shortcut()));
    rc = xpthread_create(worker, (void *)mi);
  }
  else {
    xlog("menu_cb: %s\n", mi->label());
    rc = xpthread_create(worker, (void *)mi);
  }
} /* menu_cb */


// ----------------------------------------------------------------------------
// open_cb
// ----------------------------------------------------------------------------
void open_cb(Fl_Widget *w, void *p)
{
  Fl_Native_File_Chooser *nfc;
  char *text;
  int ii, rc;
  TARG *targ;

  nfc = new Fl_Native_File_Chooser();
  nfc->title("Select files");
  nfc->type(Fl_Native_File_Chooser::BROWSE_MULTI_FILE);
  if (ft == 1) // XYZ files
    nfc->filter("XYZ Files\t*.{xyz,csv,txt}\n"
                "LIDAR Files\t*.asc\n"
                "ESRI Shapefiles\t*.shp\n");
  else // SHP files
    nfc->filter("ESRI Shapefiles\t*.shp\n"
                "XYZ Files\t*.{xyz,csv,txt}\n"
                "LIDAR Files\t*.asc\n");
//nfc->directory("/tmp");
//nfc->preset_file("test.xyz");

  switch (nfc->show()) {
    case -1: xlog("open_cb: error = %s\n", nfc->errmsg()); break;
    case  1: xlog("open_cb: cancel\n"); break;
    default:
      tab1->show(); // switch to drag & drop area tab

      text = new char[MAXL+1]; *text = '\0';
      for (ii = 0; ii < nfc->count(); ii++) {
        xlog("open_cb: selected = %s\n", nfc->filename(ii));
        xstrncat(text, nfc->filename(ii), MAXL);
        xstrncat(text, "\n", MAXL);
      }
      targ = new TARG;
      xstrncpy(targ->text, text, MAXL);
      delete text;

      rc = xpthread_create(convert_all, (void *)targ);

      break;
  }
} /* open_cb */


// ----------------------------------------------------------------------------
// quit_cb
// ----------------------------------------------------------------------------
void quit_cb(Fl_Widget *w, void *p)
{
  Fl_Menu_ *m;

  m = (Fl_Menu_ *)w;
  xlog("quit_cb\n");
  // Get main window and hide it (=> close main)
  m->window()->hide();
} /* quit_cb */


// ----------------------------------------------------------------------------
// help_cb
// ----------------------------------------------------------------------------
void help_cb(Fl_Widget *w, void *p)
{
  Fl_Menu_ *m;
  struct _stat fst;
  char fname[MAXS+1];
  Fl_Help_Dialog *help;

  m = (Fl_Menu_ *)w;
  xlog("help_cb\n");

  if (strlen(path0) > 0)
    snprintf(fname, MAXS, "%s%c%s", path0, DIRSEP, HELP);
  else 
    xstrncpy(fname, HELP, MAXS);

  if (utf8_stat(fname, &fst) < 0) { // help file not found
    fl_message_title("Help");
    fl_message("Help file \"%s\" not found\n"
               "See http://geocoordinateconverter.tk for help\n",
               fname);
  }
  else {
    help = new Fl_Help_Dialog();
    help->load(fname);
    help->textsize(16);
    help->show();
  }
} /* help_cb */


// ----------------------------------------------------------------------------
// clear_cb
// ----------------------------------------------------------------------------
void clear_cb(Fl_Widget *w, void *p)
{
  Fl_Menu_ *m;

  m = (Fl_Menu_ *)w;
  xlog("clear_cb\n");

  brow->clear();
} /* clear_cb */


// ----------------------------------------------------------------------------
// about_cb
// ----------------------------------------------------------------------------
void about_cb(Fl_Widget *w, void *p)
{
  Fl_Menu_ *m;

  m = (Fl_Menu_ *)w;
  xlog("about_cb\n");

  fl_message_title("About");
  // copyright:  Unicode: 00A9, UTF8: A9
  fl_message("Geo Coordinate Converter\n"
             "xgk-slo v%s (%s)\n"
             "Copyright \xA9 2014-2016 Matjaz Rihtar",
             SW_VERSION, SW_BUILD);
} /* about_cb */


// ----------------------------------------------------------------------------
// fields_show
// ----------------------------------------------------------------------------
void fields_show(int n)
{
  int ii;

  switch (n) {
    case 1:
      // set ddms to proper value on switched fields
      for (ii = 0; ii < 3; ii++)
        if (rb_dms[ii]->value()) ddms = ii % 3 + 1;
      gtr1->show(); gtr2->hide(); gtr3->hide();
      break;
    case 2:
      // set ddms to proper value on switched fields
      for (ii = 3; ii < 6; ii++)
        if (rb_dms[ii]->value()) ddms = ii % 3 + 1;
      gtr1->hide(); gtr2->show(); gtr3->hide();
      break;
    case 3:
      gtr1->hide(); gtr2->hide(); gtr3->show();
      break;
  }
  xlog("fields_show: ddms = %d\n", ddms);
} /* fields_show */


// ----------------------------------------------------------------------------
// trans_cb
// ----------------------------------------------------------------------------
void trans_cb(Fl_Widget *w, void *p)
{
  Fl_Radio_Round_Button *b;
  int ii, bsel;

  b = (Fl_Radio_Round_Button *)w;
  bsel = -1;
  for (ii = 0; ii < 10; ii++) {
    if (b == rb_trans[ii]) {
      rb_trans[ii]->labelfont(FL_BOLD);
      bsel = ii;
    }
    else rb_trans[ii]->labelfont(0);
    rb_trans[ii]->redraw_label();
  }
  switch (bsel) {
    case 0: tr =  1; fields_show(1); break; // xy (D96/TM) ==> fila (ETRS89)
    case 1: tr =  2; fields_show(2); break; // fila (ETRS89) ==> xy (D96/TM)
    case 2: tr =  3; fields_show(1); break; // xy (D48/GK) ==> fila (ETRS89)
    case 3: tr =  4; fields_show(2); break; // fila (ETRS89) ==> xy (D48/GK)
    case 4: tr =  5; fields_show(3); break; // xy (D48/GK) ==> xy (D96/TM)
    case 5: tr =  6; fields_show(3); break; // xy (D96/TM) ==> xy (D48/GK)
    case 6: tr =  7; fields_show(3); break; // xy (D48/GK) ==> xy (D96/TM), AFT
    case 7: tr =  8; fields_show(3); break; // xy (D96/TM) ==> xy (D48/GK), AFT
    case 8: tr =  9; fields_show(1); break; // xy (D48/GK) ==> fila (ETRS89), AFT
    case 9: tr = 10; fields_show(2); break; // fila (ETRS89) ==> xy (D48/GK), AFT
    default: break;
  }
  xlog("trans_cb: button = %s, tr = %d\n", b->label(), tr);
} /* trans_cb */


// ----------------------------------------------------------------------------
// geoid_cb
// ----------------------------------------------------------------------------
void geoid_cb(Fl_Widget *w, void *p)
{
  Fl_Radio_Round_Button *b;
  int ii, bsel;

  b = (Fl_Radio_Round_Button *)w;
  bsel = -1;
  for (ii = 0; ii < 2; ii++) {
    if (b == rb_geoid[ii]) {
      rb_geoid[ii]->labelfont(FL_BOLD);
      bsel = ii;
    }
    else rb_geoid[ii]->labelfont(0);
    rb_geoid[ii]->redraw_label();
  }
  switch (bsel) {
    case 0: gid_wgs = 1; break; // Slovenia 2000
    case 1: gid_wgs = 2; break; // EGM 2008
    default: break;
  }
  xlog("geoid_cb: button = %s, gid_wgs = %d\n", b->label(), gid_wgs);
} /* geoid_cb */


// ----------------------------------------------------------------------------
// height_cb
// ----------------------------------------------------------------------------
void height_cb(Fl_Widget *w, void *p)
{
  Fl_Radio_Round_Button *b;
  int ii, bsel;

  b = (Fl_Radio_Round_Button *)w;
  bsel = -1;
  for (ii = 0; ii < 4; ii++) {
    if (b == rb_height[ii]) {
      rb_height[ii]->labelfont(FL_BOLD);
      bsel = ii;
    }
    else rb_height[ii]->labelfont(0);
    rb_height[ii]->redraw_label();
  }
  switch (bsel) {
    case 0: hsel = -1; break; // Default (built-in recomm.)
    case 1: hsel =  2; break; // Calculate from geoid
    case 2: hsel =  0; break; // Use Helmert transformation
    case 3: hsel =  1; break; // Copy unchanged to output
    default: break;
  }
  xlog("height_cb: button = %s, hsel = %d\n", b->label(), hsel);
} /* height_cb */


// ----------------------------------------------------------------------------
// wdms_cb
// ----------------------------------------------------------------------------
void wdms_cb(Fl_Widget *w, void *p)
{
  Fl_Check_Button *b;

  b = (Fl_Check_Button *)w;
  if (b->value()) b->labelfont(FL_BOLD);
  else b->labelfont(0);
  b->redraw_label();

  wdms = b->value(); // Display fila in DMS format
  xlog("wdms_cb: button = %s, wdms = %d\n", b->label(), wdms);
} /* wdms_cb */


// ----------------------------------------------------------------------------
// rev_cb
// ----------------------------------------------------------------------------
void rev_cb(Fl_Widget *w, void *p)
{
  Fl_Check_Button *b;

  b = (Fl_Check_Button *)w;
  if (b->value()) b->labelfont(FL_BOLD);
  else b->labelfont(0);
  b->redraw_label();

  rev = b->value(); // Reverse parsing order of xy/fila
  xlog("rev_cb: button = %s, rev = %d\n", b->label(), rev);
} /* rev_cb */


// ----------------------------------------------------------------------------
// ftchoice_cb
// ----------------------------------------------------------------------------
void ftchoice_cb(Fl_Widget *w, void *p)
{
  Fl_Choice *c;

  c = (Fl_Choice *)w;
  ft = (fl_intptr_t)p;

  xlog("ftchoice_cb: ft = %d\n", ft);
} /* ftchoice_cb */


// ----------------------------------------------------------------------------
// dnd_cb
// ----------------------------------------------------------------------------
void dnd_cb(Fl_Widget *w, void *p)
{
  Fl_DND_Box *dnd;
  char *text;
  int len, rc;
  TARG *targ;

  dnd = (Fl_DND_Box *)w;
  xlog("dnd_cb\n");

  if (dnd->event() == FL_PASTE) { // process Paste event
    text = (char *)dnd->event_text();

    len = strlen(text);
    if (len > MAXL) // event text is truncated to MAXL!
      xlog("dnd_proc: len: %d, truncated text: %.256s...\n", len, text);
    else if (len > 256)
      xlog("dnd_proc: len: %d, text: %.256s...\n", len, text);
    else
      xlog("dnd_proc: len: %d, text: %s\n", len, text);

    targ = new TARG;
    xstrncpy(targ->text, text, MAXL);

    rc = xpthread_create(convert_all, (void *)targ);
  }
} /* dnd_cb */


// ----------------------------------------------------------------------------
// redisplay_dms
// ----------------------------------------------------------------------------
void redisplay_dms(int gtrn, int new_ddms)
{
  char value1[MAXS+1], value2[MAXS+1];
  double fi, la;
  DMS lat, lon;

  fi = 0.0; la = 0.0; // to keep compiler happy
  switch (gtrn) {
    case 1: // gtr1: xy ==> fila
      parse_dms(output[0]->value(), &fi);
      parse_dms(output[1]->value(), &la);
      break;
    case 2: // gtr2: fila ==> xy
      parse_dms(input[3]->value(), &fi);
      parse_dms(input[4]->value(), &la);
      break;
  }
  if (fi > 90.0 || fi < -90.0) fi = 0.0;
  if (la > 180.0 || la < -180.0) la = 0.0;

  switch (new_ddms) {
    case 1: // Dec. Degrees
      snprintf(value1, MAXS, "%.9f", fi);
      snprintf(value2, MAXS, "%.9f", la);
      break;
    case 2: // Deg. Min.
      deg2dm(fi, &lat); deg2dm(la, &lon);
      snprintf(value1, MAXS, "%.0f\xB0 %.7f'", lat.deg, lat.min);
      snprintf(value2, MAXS, "%.0f\xB0 %.7f'", lon.deg, lon.min);
      break;
    case 3: // Deg. Min. Sec.
      deg2dms(fi, &lat); deg2dms(la, &lon);
      snprintf(value1, MAXS, "%.0f\xB0 %.0f' %.5f\"", lat.deg, lat.min, lat.sec);
      snprintf(value2, MAXS, "%.0f\xB0 %.0f' %.5f\"", lon.deg, lon.min, lon.sec);
      break;
  } // switch (ddms)

  switch (gtrn) {
    case 1: // gtr1: xy ==> fila
      output[0]->value(value1);
      output[1]->value(value2);
      break;
    case 2: // gtr2: fila ==> xy
      input[3]->value(value1);
      input[4]->value(value2);
      break;
  }
} /* redisplay_dms */


// ----------------------------------------------------------------------------
// ddms_cb
// ----------------------------------------------------------------------------
void ddms_cb(Fl_Widget *w, void *p)
{
  Fl_Radio_Round_Button *b;
  int ii, bsel;

  b = (Fl_Radio_Round_Button *)w;
  bsel = -1;
  for (ii = 0; ii < 6; ii++) {
    if (b == rb_dms[ii]) {
      rb_dms[ii]->labelfont(FL_BOLD);
      bsel = ii;
    }
    else rb_dms[ii]->labelfont(0);
    rb_dms[ii]->redraw_label();
  }
  switch (bsel) {
    case 0: redisplay_dms(1, 1); ddms = 1; break; // gtr1: Dec. Degrees
    case 1: redisplay_dms(1, 2); ddms = 2; break; // gtr1: Deg. Min.
    case 2: redisplay_dms(1, 3); ddms = 3; break; // gtr1: Deg. Min. Sec.
    case 3: redisplay_dms(2, 1); ddms = 1; break; // gtr2: Dec. Degrees
    case 4: redisplay_dms(2, 2); ddms = 2; break; // gtr2: Deg. Min.
    case 5: redisplay_dms(2, 3); ddms = 3; break; // gtr2: Deg. Min. Sec.
    default: break;
  }
  xlog("ddms_cb: button = %s, ddms = %d\n", b->label(), ddms);
} /* ddms_cb */


// ----------------------------------------------------------------------------
// join_threads
// ----------------------------------------------------------------------------
void join_threads(void *p)
{
  int rc, sts;
  PTID *pt;

  for (pt = threads0; pt != NULL; pt = pt->next) {
    rc = pthread_join(pt->tid, (void **)&sts);
    if (!rc) {
      pt->done = 1; pt->sts = sts;
#ifdef OLD_PTHREADS
      xlog("Completed join with thread %08x, status = %d\n", pt->tid.p, pt->sts);
#else
      xlog("Completed join with thread %08x, status = %d\n", pt->tid, pt->sts);
#endif
    }
  }

//Fl::flush();
  Fl::repeat_timeout(0.5, join_threads);
} /* join_threads */


// ----------------------------------------------------------------------------
// main
// ----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  char *s;
  Fl_Double_Window *mainwin, *subwin1, *subwin2;
  Fl_Menu_Bar *menubar;
  Fl_Group *g1, *g2, *g3, *g4, *g5;
  Fl_Radio_Round_Button *rb;
  Fl_Check_Button *cb;
  Fl_Tabs *tabs;
  Fl_Text_Display *help;
  Fl_Text_Buffer *help_tb;
  Fl_Box *box; Fl_DND_Box *dnd;
  Fl_Choice *ch;
  Fl_Button *bt;
  Fl_Arrow_Box *ar;
  int x0, y0, xinc, yinc, w0, h0, x1;
  int ii, jj, rc, sts;
  PTID *pt;
#ifdef _WIN32
  HICON iconp;
#else
  Pixmap iconp, mask;
  XWMHints *hints;
#endif

  xstrncpy(argv0, argv[0], MAXS); // save original argv[0]
#ifdef _WIN32
  if (strstr(argv0, ".exe") == NULL && strstr(argv0, ".EXE") == NULL)
    xstrncat(argv0, ".exe", MAXS);
#endif

  // Get program name
  if ((prog = strrchr(argv[0], DIRSEP)) == NULL) prog = argv[0];
  else prog++;
  if ((s = strstr(prog, ".exe")) != NULL) *s = '\0';
  if ((s = strstr(prog, ".EXE")) != NULL) *s = '\0';

  // Initialize xlog mutex
  pthread_mutex_init(&xlog_mutex, NULL);

  locate_self(argv0, path0, MAXS);
  xlog("locate_self: path0 = |%s|\n", path0);

  // Default global flags
  debug = 0;   // no debug
  tr = 1;      // default transformation: xy (d96tm) --> fila (etrs89)
  rev = 0;     // don't reverse xy/fila
  wdms = 0;    // don't write DMS
  gid_wgs = 1; // default geoid: slo2000
  hsel = -1;   // no default height processing (use internal recommendations)
  ft = 1;      // file type: XYZ
  ddms = 1;    // display DMS: Dec. Degrees

  // geo.c initialization
  ellipsoid_init();
  params_init();

  threads = NULL; threads0 = threads;

  // Initialize and set thread attribute
  pthread_attr_init(&pattr);
//pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_JOINABLE);
  pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);
  pthread_mutex_init(&tn_mutex, NULL);
  tn = 0;

  // Create main window
  w0 = 815; h0 = 600;
  mainwin = new Fl_Double_Window(w0, h0, "Geo Coordinate Converter");
  mainwin->resizable(mainwin);
  mainwin->size_range(w0-300, h0, w0+300, h0+300);
  mainwin->callback(mainwin_cb, NULL);

  // Create menu bar
  menubar = new Fl_Menu_Bar(0, 0, w0, 30);
  menubar->menu(menubar_entries);
  menubar->callback(menu_cb, NULL);

  // ==========================================================================
  // Top window
  subwin1 = new Fl_Double_Window(0, menubar->y()+menubar->h(), mainwin->w(), 233);
  subwin1->box(FL_DOWN_BOX);
  // x,y is 0.0 from now on

  // Create radio buttons
  g1 = new Fl_Group(5, 22, 277, 207, "Select transformation");
  g1->labelsize(16); g1->labelfont(FL_BOLD + FL_ITALIC);
//g1->box(FL_DOWN_BOX);
//g1->clip_children(1);
  ii = 0;
  x0 = 5; y0 = 25; yinc = 20; w0 = g1->w()-4, h0 = 22;
  // rightwards double arrow:   Unicode: 21D2, UTF8: E28792
  // greek small letter phi:    Unicode: 03C6, UTF8: CF86
  // greek small letter lamda:  Unicode: 03BB, UTF8: CEBB
  rb = new Fl_Radio_Round_Button(x0, y0+0*yinc, w0, h0, "xy (D96/TM) \xE2\x87\x92 \xCF\x86\xCE\xBB (ETRS89)");
  rb->set(); rb->labelfont(FL_BOLD);
  rb->callback(trans_cb, NULL); rb_trans[ii++] = rb;
  rb = new Fl_Radio_Round_Button(x0, y0+1*yinc, w0, h0, "\xCF\x86\xCE\xBB (ETRS89) \xE2\x87\x92 xy (D96/TM)");
  rb->callback(trans_cb, NULL); rb_trans[ii++] = rb;
  rb = new Fl_Radio_Round_Button(x0, y0+2*yinc, w0, h0, "xy (D48/GK) \xE2\x87\x92 \xCF\x86\xCE\xBB (ETRS89)");
  rb->callback(trans_cb, NULL); rb_trans[ii++] = rb;
  rb = new Fl_Radio_Round_Button(x0, y0+3*yinc, w0, h0, "\xCF\x86\xCE\xBB (ETRS89) \xE2\x87\x92 xy (D48/GK)");
  rb->callback(trans_cb, NULL); rb_trans[ii++] = rb;
  rb = new Fl_Radio_Round_Button(x0, y0+4*yinc, w0, h0, "xy (D48/GK) \xE2\x87\x92 xy (D96/TM)");
  rb->callback(trans_cb, NULL); rb_trans[ii++] = rb;
  rb = new Fl_Radio_Round_Button(x0, y0+5*yinc, w0, h0, "xy (D96/TM) \xE2\x87\x92 xy (D48/GK)");
  rb->callback(trans_cb, NULL); rb_trans[ii++] = rb;
  rb = new Fl_Radio_Round_Button(x0, y0+6*yinc, w0, h0, "xy (D48/GK) \xE2\x87\x92 xy (D96/TM), AFT");
  rb->callback(trans_cb, NULL); rb_trans[ii++] = rb;
  rb = new Fl_Radio_Round_Button(x0, y0+7*yinc, w0, h0, "xy (D96/TM) \xE2\x87\x92 xy (D48/GK), AFT");
  rb->callback(trans_cb, NULL); rb_trans[ii++] = rb;
  rb = new Fl_Radio_Round_Button(x0, y0+8*yinc, w0, h0, "xy (D48/GK) \xE2\x87\x92 \xCF\x86\xCE\xBB (ETRS89), AFT");
  rb->callback(trans_cb, NULL); rb_trans[ii++] = rb;
  rb = new Fl_Radio_Round_Button(x0, y0+9*yinc, w0, h0, "\xCF\x86\xCE\xBB (ETRS89) \xE2\x87\x92 xy (D48/GK), AFT");
  rb->callback(trans_cb, NULL); rb_trans[ii++] = rb;
  g1->end();

  g2 = new Fl_Group(g1->w()+10, subwin1->y()-8, 248, 75, "Select geoid");
  g2->labelsize(16); g2->labelfont(FL_BOLD + FL_ITALIC);
//g2->box(FL_DOWN_BOX);
//g2->clip_children(1);
  ii = 0;
  x0 = g1->w()+10; y0 = 25; yinc = 20; w0 = g2->w()-4, h0 = 22;
  rb = new Fl_Radio_Round_Button(x0, y0+0*yinc, w0, h0, "Slovenia 2000");
  rb->set(); rb->labelfont(FL_BOLD);
  rb->callback(geoid_cb, NULL); rb_geoid[ii++] = rb;
  rb = new Fl_Radio_Round_Button(x0, y0+1*yinc, w0, h0, "EGM 2008");
  rb->callback(geoid_cb, NULL); rb_geoid[ii++] = rb;
  g2->end();

  g3 = new Fl_Group(g1->w()+10, subwin1->y()-8+75, g2->w(), 100, "Select height calculation");
  g3->labelsize(16); g3->labelfont(FL_BOLD + FL_ITALIC);
//g3->box(FL_DOWN_BOX);
//g3->clip_children(1);
  ii = 0;
  x0 = g1->w()+10; y0 = g3->h(); yinc = 20; w0 = g3->w()-4, h0 = 22;
  rb = new Fl_Radio_Round_Button(x0, y0+0*yinc, w0, h0, "Default (built-in recomm.)");
  rb->set(); rb->labelfont(FL_BOLD);
  rb->callback(height_cb, NULL); rb_height[ii++] = rb;
  rb = new Fl_Radio_Round_Button(x0, y0+1*yinc, w0, h0, "Calculate from geoid");
  rb->callback(height_cb, NULL); rb_height[ii++] = rb;
  rb = new Fl_Radio_Round_Button(x0, y0+2*yinc, w0, h0, "Use Helmert transformation");
  rb->callback(height_cb, NULL); rb_height[ii++] = rb;
  rb = new Fl_Radio_Round_Button(x0, y0+3*yinc, w0, h0, "Copy unchanged to output");
  rb->callback(height_cb, NULL); rb_height[ii++] = rb;
  g3->end();

  g4 = new Fl_Group(g1->w()+g2->w()+15, subwin1->y()-8, 270, 55, "Input file syntax");
  g4->labelsize(16); g4->labelfont(FL_BOLD + FL_ITALIC);
//g4->box(FL_DOWN_BOX);
//g4->clip_children(1);
  x0 = g1->w()+g2->w()+15; y0 = 25; yinc = 20; w0 = g4->w()-4, h0 = 22;
  cb = new Fl_Check_Button(x0, y0+0*yinc, w0, h0, "Reverse parsing order of xy/\xCF\x86\xCE\xBB");
  cb->callback(rev_cb, NULL);
  g4->end();

  g5 = new Fl_Group(g1->w()+g2->w()+15, subwin1->y()-8+55, g4->w(), 45, "Output file format");
  g5->labelsize(16); g5->labelfont(FL_BOLD + FL_ITALIC);
//g5->box(FL_DOWN_BOX);
//g5->clip_children(1);
  x0 = g1->w()+g2->w()+15; y0 = g4->h()+25; yinc = 20; w0 = g5->w()-4, h0 = 22;
  // degree sign:  Unicode: 00B0, UTF8: B0
  cb = new Fl_Check_Button(x0, y0+0*yinc, w0, h0, "Display \xCF\x86\xCE\xBB in D\xB0M'S\" format");
  cb->callback(wdms_cb, NULL);
  g5->end();

  help = new Fl_Text_Display(g1->w()+g2->w()+15, subwin1->y()-8+100, g4->w(), 110);
  help->box(FL_NO_BOX);
  help_tb = new Fl_Text_Buffer();
  help->buffer(help_tb);
  help->textsize(11); help->textcolor(FL_BLUE);
  help_tb->text("y = Northing\n"
                "x = Easting\n"
                "H = Ortometric (above sea level) height\n"
                "\xCF\x86 = phi, Latitude (N/S)\n"
                "\xCE\xBB = lambda, Longitude (E/W)\n"
                "h = Ellipsoidal height\n"
                "AFT = Affine Transformation");

  // No more widgets in subwin1
  subwin1->resizable(subwin1);
  subwin1->end();

  // ==========================================================================
  // Bottom window
  subwin2 = new Fl_Double_Window(0, menubar->y()+menubar->h()+subwin1->h(), mainwin->w(), mainwin->h()-subwin1->h()-6);
//subwin2->box(FL_DOWN_BOX);
//xlog("subwin2-x: %d, subwin2-y: %d\n", subwin2->x(), subwin2->y());
  // x,y is 0.0 from now on

  tabs = new Fl_Tabs(0, 22, subwin2->w(), subwin2->h()-46, "Operation");
  tabs->labelsize(16); tabs->labelfont(FL_BOLD);
//tabs->box(FL_DOWN_BOX);
  // get available size for children tabs
  tabs->client_area((int &)x0, (int &)y0, (int &)w0, (int &)h0, 25);

  // --------------------------------------------------------------------------
  // Create drag & drop area tab
  tab1 = new Fl_Group(x0+5, y0, w0-10, h0-5, "Drag && drop files");
  tab1->labelsize(16); tab1->labelfont(FL_BOLD);
  tab1->box(FL_DOWN_BOX);
//tab1->clip_children(1);

#ifdef _WIN32
  x1 = tab1->x()+85; // different font sizes on Windows & Unix
#else
  x1 = tab1->x()+100;
#endif
  ch = new Fl_Choice(x1, tab1->y()+8, 100, 25, "File type: ");
  ch->labelsize(16); ch->labelfont(FL_BOLD + FL_ITALIC);
  ch->menu(ft_choices);
  ch->callback(ftchoice_cb, NULL);
  ch->when(FL_WHEN_CHANGED);

  brow = new Fl_Browser(tab1->x(), tab1->y()+40, tab1->w(), tab1->h()-40);
  dnd = new Fl_DND_Box(brow->x(), brow->y(), brow->w(), brow->h());
  dnd->callback(dnd_cb, NULL);

  brow->add("@C4You can drag & drop many files of the same type or open them from the File menu.");
  brow->bottomline(brow->size());
  brow->add("@C4They will be processed in parallel.");
  brow->bottomline(brow->size());

  tab1->end();

  // --------------------------------------------------------------------------
  // Create interactive area tab
  tab2 = new Fl_Group(x0+5, y0, w0-10, h0-5, "Interactive");
  tab2->labelsize(16); tab2->labelfont(FL_BOLD);
  tab2->box(FL_DOWN_BOX);
//tab2->clip_children(1);

  gtr1 = new Fl_Group(tab2->x(), tab2->y(), tab2->w(), tab2->h());
//gtr1->box(FL_DOWN_BOX);
//gtr1->clip_children(1);

  input[0] = new Fl_Input(tab2->x()+30, tab2->y()+20, 150, 25, "Y:");
  input[0]->tooltip("Enter northing (Y)");
  input[1] = new Fl_Input(tab2->x()+30, tab2->y()+55, 150, 25, "X:");
  input[1]->tooltip("Enter easting (X)");
  input[2] = new Fl_Input(tab2->x()+30, tab2->y()+90, 150, 25, "H:");
  input[2]->tooltip("Enter ortometric (above sea level) height (H)");

  bt = new Fl_Button(input[0]->x()+input[0]->w()+20, input[1]->y(), 70, 25, "Convert");
  bt->callback(convert_cb, gtr1);
  ar = new Fl_Arrow_Box(bt->x(), bt->y()+bt->h(), bt->w(), 30);

  output[0] = new Fl_Output(bt->x()+bt->w()+70, bt->y()-35, 150, 25, "Lat (\xCF\x86):");
  output[0]->tooltip("Latitude (\xCF\x86, N/S)");
  output[1] = new Fl_Output(bt->x()+bt->w()+70, bt->y(), 150, 25, "Lon (\xCE\xBB):");
  output[1]->tooltip("Longitude (\xCE\xBB, E/W)");
  output[2] = new Fl_Output(bt->x()+bt->w()+70, bt->y()+35, 150, 25, "h:");
  output[2]->tooltip("Ellipsoidal height (h)");

  g1 = new Fl_Group(output[0]->x()+output[0]->w()+20, output[0]->y(), 130, 64);
//g1->box(FL_DOWN_BOX);
//g1->clip_children(1);
  ii = 0;
  x0 = g1->x(); y0 = g1->y(); yinc = 20; w0 = g1->w()-4, h0 = 22;
  rb = new Fl_Radio_Round_Button(x0, y0+0*yinc, w0, h0, "Dec. Degrees");
  rb->set(); rb->labelfont(FL_BOLD);
  rb->callback(ddms_cb, NULL); rb_dms[ii++] = rb;
  rb = new Fl_Radio_Round_Button(x0, y0+1*yinc, w0, h0, "Deg. Min.");
  rb->callback(ddms_cb, NULL); rb_dms[ii++] = rb;
  rb = new Fl_Radio_Round_Button(x0, y0+2*yinc, w0, h0, "Deg. Min. Sec.");
  rb->callback(ddms_cb, NULL); rb_dms[ii++] = rb;
  g1->end();

  bt = new Fl_Button(g1->x()+g1->w()+20, output[1]->y(), 100, 25, "Show on map");
  bt->callback(show_cb, gtr1);

  gtr1->end();
//gtr1->hide();

  gtr2 = new Fl_Group(tab2->x(), tab2->y(), tab2->w(), tab2->h());
//gtr2->box(FL_DOWN_BOX);
//gtr2->clip_children(1);

  g2 = new Fl_Group(tab2->x()+15, tab2->y()+20, 130, 64);
//g2->box(FL_DOWN_BOX);
//g2->clip_children(1);
//ii = 0;
  x0 = g2->x(); y0 = g2->y(); yinc = 20; w0 = g2->w()-4, h0 = 22;
  rb = new Fl_Radio_Round_Button(x0, y0+0*yinc, w0, h0, "Dec. Degrees");
  rb->set(); rb->labelfont(FL_BOLD);
  rb->callback(ddms_cb, NULL); rb_dms[ii++] = rb;
  rb = new Fl_Radio_Round_Button(x0, y0+1*yinc, w0, h0, "Deg. Min.");
  rb->callback(ddms_cb, NULL); rb_dms[ii++] = rb;
  rb = new Fl_Radio_Round_Button(x0, y0+2*yinc, w0, h0, "Deg. Min. Sec.");
  rb->callback(ddms_cb, NULL); rb_dms[ii++] = rb;
  g2->end();

  input[3] = new Fl_Input(g2->x()+g2->w()+70, g2->y(), 150, 25, "Lat (\xCF\x86):");
  input[3]->tooltip("Enter Latitude (\xCF\x86, N/S)");
  input[4] = new Fl_Input(g2->x()+g2->w()+70, g2->y()+35, 150, 25, "Lon (\xCE\xBB):");
  input[4]->tooltip("Enter Longitude (\xCE\xBB, E/W)");
  input[5] = new Fl_Input(g2->x()+g2->w()+70, g2->y()+70, 150, 25, "h:");
  input[5]->tooltip("Enter ellipsoidal height (h)");

  bt = new Fl_Button(input[3]->x()+input[3]->w()+20, input[4]->y(), 70, 25, "Convert");
  bt->callback(convert_cb, gtr2);
  ar = new Fl_Arrow_Box(bt->x(), bt->y()+bt->h(), bt->w(), 30);

  output[3] = new Fl_Output(bt->x()+bt->w()+35, bt->y()-35, 150, 25, "Y:");
  output[3]->tooltip("Northing (Y)");
  output[4] = new Fl_Output(bt->x()+bt->w()+35, bt->y(), 150, 25, "X:");
  output[4]->tooltip("Easting (X)");
  output[5] = new Fl_Output(bt->x()+bt->w()+35, bt->y()+35, 150, 25, "H:");
  output[5]->tooltip("Ortometric (above sea level) height (H)");

  bt = new Fl_Button(output[4]->x()+output[4]->w()+20, output[4]->y(), 100, 25, "Show on map");
  bt->callback(show_cb, gtr2);

  gtr2->end();
  gtr2->hide();

  gtr3 = new Fl_Group(tab2->x(), tab2->y(), tab2->w(), tab2->h());
//gtr3->box(FL_DOWN_BOX);
//gtr3->clip_children(1);

  input[6] = new Fl_Input(tab2->x()+30, tab2->y()+20, 150, 25, "Y:");
  input[6]->tooltip("Enter northing (Y)");
  input[7] = new Fl_Input(tab2->x()+30, tab2->y()+55, 150, 25, "X:");
  input[7]->tooltip("Enter easting (X)");
  input[8] = new Fl_Input(tab2->x()+30, tab2->y()+90, 150, 25, "H:");
  input[8]->tooltip("Enter ortometric (above sea level) height (H)");

  bt = new Fl_Button(input[6]->x()+input[6]->w()+20, input[7]->y(), 70, 25, "Convert");
  bt->callback(convert_cb, gtr3);
  ar = new Fl_Arrow_Box(bt->x(), bt->y()+bt->h(), bt->w(), 30);

  output[6] = new Fl_Output(bt->x()+bt->w()+35, bt->y()-35, 150, 25, "Y:");
  output[6]->tooltip("Northing (Y)");
  output[7] = new Fl_Output(bt->x()+bt->w()+35, bt->y(), 150, 25, "X:");
  output[7]->tooltip("Easting (X)");
  output[8] = new Fl_Output(bt->x()+bt->w()+35, bt->y()+35, 150, 25, "H:");
  output[8]->tooltip("Ortometric (above sea level) height (H)");

  bt = new Fl_Button(output[7]->x()+output[7]->w()+20, output[7]->y(), 100, 25, "Show on map");
  bt->callback(show_cb, gtr3);

  gtr3->end();
  gtr3->hide();

  tab2->end();

  tabs->end();

  // No more widgets in subwin2
  subwin2->resizable(subwin2);
  subwin2->end();

  // ==========================================================================
  // No more widgets in mainwin
  mainwin->end();

  // Show main window
  Fl::scheme("gtk+");
  Fl::visual(FL_DOUBLE | FL_INDEX);
  mainwin->xclass("XGkSlo"); // no non-alphanumeric chars!

#ifdef _WIN32
  // Add window icon
  iconp = LoadIcon(fl_display, "IDI_ICON1");
  mainwin->icon((const void *)iconp);
  mainwin->show(argc, argv);
#else
  XpmCreatePixmapFromData(fl_display, DefaultRootWindow(fl_display),
                          globe_xpm, &iconp, &mask, NULL);
//mainwin->icon((const void *)iconp); // This creates non-transparent window icon!
  mainwin->show(argc, argv);

  // Add window icon with transparent background (using mask)
  hints = XGetWMHints(fl_display, fl_xid(mainwin));
  hints->icon_pixmap = iconp;
  hints->icon_mask = mask;
  hints->flags |= IconPixmapHint | IconMaskHint;
  XSetWMHints(fl_display, fl_xid(mainwin), hints);
#endif

//Fl::add_timeout(0.5, join_threads); // join finished threads every 0.5 sec

  // Run in loop until main window closes
  Fl::run();

  // Free thread attribute and wait for all threads to finish
  pthread_attr_destroy(&pattr);
#if 0
  for (pt = threads0; pt != NULL; pt = pt->next) {
    rc = pthread_join(pt->tid, (void **)&sts);
    if (!rc) {
      pt->done = 1; pt->sts = sts;
#ifdef OLD_PTHREADS
      xlog("Completed join with thread %08x, status = %d\n", pt->tid.p, pt->sts);
#else
      xlog("Completed join with thread %08x, status = %d\n", pt->tid, pt->sts);
#endif
    }
  }
#endif

  xlog("Main program completed (threads: %d)\n", tn);
  pthread_mutex_destroy(&xlog_mutex);
  pthread_mutex_destroy(&tn_mutex);
  pthread_exit(NULL);
  return 0;
} /* main */
