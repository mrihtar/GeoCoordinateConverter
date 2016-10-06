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
#include <pthread.h> // must be included before common.h
#include "common.h"
#include "geo.h"

#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Sys_Menu_Bar.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Help_Dialog.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>

#define SW_VERSION "1.20"
#define SW_BUILD   "Oct 6, 2016"

// global variables
char *prog;  // program name
int debug;
int tr;      // transformation
int rev;     // reverse xy/fila
int ddms;    // display DMS
extern int gid_wgs; // selected geoid on WGS 84 (in geo.c)
extern int hsel;    // output height calculation (in geo.c)

typedef struct ptid {
  // WIN32 { void *p; unsigned int x; }, UNIX: unsigned long
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

typedef struct targ {
  char text[MAXL+1];
  Fl_Browser *brow;
} TARG;

#ifdef __cplusplus
extern "C" {
#endif
// External function prototypes
int convert_file(char *url, int outf, FILE *out, char *msg); // in conv.c
#ifdef __cplusplus
}
#endif

// FLTK function prototypes
void quit_cb(Fl_Widget *w, void *p);
void help_cb(Fl_Widget *w, void *p);
void about_cb(Fl_Widget *w, void *p);

// FLTK global variables
Fl_Menu_Item menubar_entries[] = {
  {"&File", 0, 0, 0, FL_SUBMENU},
    {"&Open",   FL_ALT+'o',  0, 0, FL_MENU_INACTIVE},
    {"&Close",  0,           0, 0, FL_MENU_DIVIDER},
    {"&Quit",   FL_ALT+'q',  quit_cb},
    {0},
  {"&Edit", 0, 0, 0, FL_SUBMENU},
    {"Undo",    FL_CTRL+'z', 0},
    {"Redo",    FL_CTRL+'r', 0, 0, FL_MENU_DIVIDER},
    {"Cut",     FL_CTRL+'x', 0},
    {"Copy",    FL_CTRL+'c', 0},
    {"Paste",   FL_CTRL+'v', 0, 0, FL_MENU_DIVIDER},
    {"Clear",   0,           0},
    {0},
  {"&Help", 0, 0, 0, FL_SUBMENU},
    {"Help",    FL_ALT+'h',  help_cb, 0, FL_MENU_DIVIDER},
    {"About",   0,           about_cb},
    {0},
  {0}
}; /* menubar_entries */

Fl_Radio_Round_Button *rb_trans[MAXC];
Fl_Radio_Round_Button *rb_geoid[MAXC];
Fl_Radio_Round_Button *rb_height[MAXC];

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
  log = fopen(logfile, "a");
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
#ifdef _WIN32
    xlog("Created thread %08x\n", (unsigned int)threads->tid.p);
#else
    xlog("Created thread %08x\n", (unsigned int)threads->tid);
#endif
  return rc;
} /* xpthread_create */


// ----------------------------------------------------------------------------
// THREAD: worker
// ----------------------------------------------------------------------------
void *worker(void *arg) {
  Fl_Menu_Item *mi;
  unsigned int tid;
  time_t now;

  mi = (Fl_Menu_Item *)arg;
#ifdef _WIN32
  tid = (unsigned int)pthread_self().p;
#else
  tid = (unsigned int)pthread_self();
#endif
  xlog("THREAD %08x: worker: menu %s\n", tid, mi->label());

  now = time(NULL);

  pthread_mutex_lock(&tn_mutex);
  tn--;
  pthread_mutex_unlock(&tn_mutex);
  pthread_exit((void *)now);
  return NULL;
} /* worker */

// ----------------------------------------------------------------------------
// THREAD: convert
// ----------------------------------------------------------------------------
void *convert(void *arg) {
  TARG *targ;
  char *url;
  Fl_Browser *brow;
  unsigned int tid;
  char *msg, *line;
  long sts;

  targ = (TARG *)arg;
  url = targ->text; brow = targ->brow;
#ifdef _WIN32
  tid = (unsigned int)pthread_self().p;
#else
  tid = (unsigned int)pthread_self();
#endif
  xlog("THREAD %08x: convert: url %s\n", tid, url);

  msg = new char[MAXL+1];
  snprintf(msg, MAXL, "Converting %s\n", url);

  Fl::lock();
  brow->add(msg); brow->bottomline(brow->size());
  Fl::unlock();
  Fl::awake((void *)NULL);

  sts = convert_file(url, 2, NULL, msg); // convert to separate files
  if (strlen(msg) > 0) {
    xlog("%s", msg); // must use %s here!

    Fl::lock();
    line = strtok(msg, "\r\n");
    while (line != NULL) {
      brow->add(line); brow->bottomline(brow->size());
      line = strtok(NULL, "\r\n");
    }
    Fl::unlock();
    Fl::awake((void *)NULL);
  }

  delete msg;
  delete targ;
  pthread_mutex_lock(&tn_mutex);
  tn--;
  pthread_mutex_unlock(&tn_mutex);
  pthread_exit((void *)sts);
  return NULL;
} /* convert */


// ----------------------------------------------------------------------------
// THREAD: convert_all
// ----------------------------------------------------------------------------
void *convert_all(void *arg) {
  char *urls, *url, *s;
  Fl_Browser *brow;
  unsigned int tid;
  int ii, len, rc;
  TARG *targs, *targ;
  struct timespec req, rem;
  long sts;

  targs = (TARG *)arg;
  urls = targs->text; brow = targs->brow;
#ifdef _WIN32
  tid = (unsigned int)pthread_self().p;
#else
  tid = (unsigned int)pthread_self();
#endif
  len = strlen(urls);
  if (len > 256)
    xlog("THREAD %08x: convert_all: urls %.256s...\n", tid, urls);
  else
    xlog("THREAD %08x: convert_all: urls %s\n", tid, urls);

  brow->clear();

  ii = 0; sts = 0;
  url = xstrtok_r(urls, "\r\n", &s);
  while (url != NULL) {
    while (tn > MAXTN) { // wait until some of the threads finish
#ifdef _WIN32
      Sleep(100); // msec
#else
      req.tv_sec = 0; req.tv_nsec = 100000000L; // 100 msec
      nanosleep(&req, &rem);
#endif
    }

    ii++;
    xlog("URL %d: %s\n", ii, url);
 // brow->add(url); brow->bottomline(brow->size());
 // Fl::flush();

    targ = new TARG;
    xstrncpy(targ->text, url, MAXL);
    targ->brow = brow;

    sts += xpthread_create(convert, (void *)targ);

    url = xstrtok_r(NULL, "\r\n", &s);
  }

  delete targs;
  pthread_mutex_lock(&tn_mutex);
  tn--;
  pthread_mutex_unlock(&tn_mutex);
  pthread_exit((void *)sts);
  return NULL;
} /* convert_all */


// ----------------------------------------------------------------------------
// mainwin_cb
// ----------------------------------------------------------------------------
void mainwin_cb(Fl_Widget *w, void *p) {
  Fl_Double_Window *mw;

  mw = (Fl_Double_Window *)w;
  xlog("mainwin_cb\n");
  mw->hide(); // not really needed (already default action)
} /* window_cb */


// ----------------------------------------------------------------------------
// menu_cb
// ----------------------------------------------------------------------------
void menu_cb(Fl_Widget *w, void *p) {
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
// quit_cb
// ----------------------------------------------------------------------------
void quit_cb(Fl_Widget *w, void *p) {
  Fl_Menu_ *m;

  m = (Fl_Menu_ *)w;
  xlog("quit_cb\n");
  // Get main window and hide it (=> close main)
  m->window()->hide();
} /* quit_cb */


// ----------------------------------------------------------------------------
// help_cb
// ----------------------------------------------------------------------------
void help_cb(Fl_Widget *w, void *p) {
  Fl_Menu_ *m;
  struct _stat fst;
  char fname[] = "xgk-help.html";
  Fl_Help_Dialog *help;

  m = (Fl_Menu_ *)w;
  xlog("help_cb\n");

  if (tstat(fname, &fst) < 0) { // help file not found
    fl_message_title("Help");
    fl_message("Help file \"%s\" not found", fname);
  }
  else {
    help = new Fl_Help_Dialog();
    help->load(fname);
    help->textsize(16);
    help->show();
  }
} /* help_cb */


// ----------------------------------------------------------------------------
// about_cb
// ----------------------------------------------------------------------------
void about_cb(Fl_Widget *w, void *p) {
  Fl_Menu_ *m;

  m = (Fl_Menu_ *)w;
  xlog("about_cb\n");

  fl_message_title("About");
  // copyright:  Unicode: 00A9, UTF8: A9
  fl_message("xgk-slo v%s (%s)\nCopyright \xA9 2014-2016 Matjaz Rihtar",
             SW_VERSION, SW_BUILD);
} /* about_cb */


// ----------------------------------------------------------------------------
// trans_cb
// ----------------------------------------------------------------------------
void trans_cb(Fl_Widget *w, void *p) {
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
    case 0: tr =  1; break; // xy (D96/TM) ==> fila (ETRS89)
    case 1: tr =  2; break; // fila (ETRS89) ==> xy (D96/TM)
    case 2: tr =  3; break; // xy (D48/GK) ==> fila (ETRS89)
    case 3: tr =  4; break; // fila (ETRS89) ==> xy (D48/GK)
    case 4: tr =  5; break; // xy (D48/GK) ==> xy (D96/TM)
    case 5: tr =  6; break; // xy (D96/TM) ==> xy (D48/GK)
    case 6: tr =  7; break; // xy (D48/GK) ==> xy (D96/TM), AFT
    case 7: tr =  8; break; // xy (D96/TM) ==> xy (D48/GK), AFT
    case 8: tr =  9; break; // xy (D48/GK) ==> fila (ETRS89), AFT
    case 9: tr = 10; break; // fila (ETRS89) ==> xy (D48/GK), AFT
    default: break;
  }
  xlog("trans_cb: button = %s, tr = %d\n", b->label(), tr);
} /* trans_cb */


// ----------------------------------------------------------------------------
// geoid_cb
// ----------------------------------------------------------------------------
void geoid_cb(Fl_Widget *w, void *p) {
  Fl_Radio_Round_Button *b;
  int ii, bsel;

  b = (Fl_Radio_Round_Button *)w;
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
void height_cb(Fl_Widget *w, void *p) {
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
// dms_cb
// ----------------------------------------------------------------------------
void dms_cb(Fl_Widget *w, void *p) {
  Fl_Check_Button *b;

  b = (Fl_Check_Button *)w;
  if (b->value()) b->labelfont(FL_BOLD);
  else b->labelfont(0);
  b->redraw_label();

  ddms = b->value(); // Display fila in DMS format
  xlog("dms_cb: button = %s, ddms = %d\n", b->label(), ddms);
} /* dms_cb */


// ----------------------------------------------------------------------------
// rev_cb
// ----------------------------------------------------------------------------
void rev_cb(Fl_Widget *w, void *p) {
  Fl_Check_Button *b;

  b = (Fl_Check_Button *)w;
  if (b->value()) b->labelfont(FL_BOLD);
  else b->labelfont(0);
  b->redraw_label();

  rev = b->value(); // Reverse parsing order of xy/fila
  xlog("rev_cb: button = %s, rev = %d\n", b->label(), rev);
} /* rev_cb */


// ----------------------------------------------------------------------------
// dnd_proc
// ----------------------------------------------------------------------------
void dnd_proc(Fl_DND_Box *dnd, Fl_Browser *brow) {
  char *text;
  int len, rc;
  TARG *targ;

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
  targ->brow = brow;

  rc = xpthread_create(convert_all, (void *)targ);
} /* dnd_proc */


// ----------------------------------------------------------------------------
// dnd_cb
// ----------------------------------------------------------------------------
void dnd_cb(Fl_Widget *w, void *p) {
  Fl_DND_Box *dnd;
  Fl_Browser *brow;

  dnd = (Fl_DND_Box *)w;
  brow = (Fl_Browser *)p;
  xlog("dnd_cb\n");

  if (dnd->event() == FL_PASTE)
    dnd_proc(dnd, brow);
} /* dnd_cb */


// ----------------------------------------------------------------------------
// join_threads
// ----------------------------------------------------------------------------
void join_threads(void *p) {
  int rc, sts;
  PTID *pt;

  for (pt = threads0; pt != NULL; pt = pt->next) {
    rc = pthread_join(pt->tid, (void **)&sts);
    if (!rc) {
      pt->done = 1; pt->sts = sts;
#ifdef _WIN32
      xlog("Completed join with thread %08x, status = %d\n",
           (unsigned int)pt->tid.p, pt->sts);
#else
      xlog("Completed join with thread %08x, status = %d\n",
           (unsigned int)pt->tid, pt->sts);
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
  Fl_Browser *brow;
  Fl_Box *box; Fl_DND_Box *dnd;
  int x0, y0, xinc, yinc, w0, h0;
  int ii, jj, rc, sts;
  PTID *pt;

  // Get program name
  if ((prog = strrchr(argv[0], DIRSEP)) == NULL) prog = argv[0];
  else prog++;
  if ((s = strstr(prog, ".exe")) != NULL) *s = '\0';
  if ((s = strstr(prog, ".EXE")) != NULL) *s = '\0';

  // Default global flags
  debug = 0;   // no debug
  tr = 1;      // default transformation: xy (d96tm) --> fila (etrs89)
  rev = 0;     // don't reverse xy/fila
  ddms = 0;    // don't display DMS
  gid_wgs = 1; // default geoid: slo2000
  hsel = -1;   // no default height processing (use internal recommendations)

  // geo.c initialization
  ellipsoid_init();
  params_init();

  threads = NULL; threads0 = threads;

  // Initialize and set thread attribute
  pthread_attr_init(&pattr);
//pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_JOINABLE);
  pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);
  // Initialize xlog mutex
  pthread_mutex_init(&xlog_mutex, NULL);
  pthread_mutex_init(&tn_mutex, NULL);
  tn = 0;

  // Create main window
  w0 = 815; h0 = 400;
  mainwin = new Fl_Double_Window(w0, h0, prog);
  mainwin->resizable(mainwin);
  mainwin->size_range(w0-300, h0, w0+300, h0+300);
  mainwin->callback(mainwin_cb);

  // Create menu bar
  menubar = new Fl_Menu_Bar(0, 0, w0, 30);
  menubar->menu(menubar_entries);
  menubar->callback(menu_cb);

  // Top window
  subwin1 = new Fl_Double_Window(0, menubar->y()+menubar->h(), mainwin->w(), 233);
  subwin1->box(FL_DOWN_BOX);
//xlog("subwin1-x: %d, subwin1-y: %d\n", subwin1->x(), subwin1->y());
  // x,y is 0.0 from now on

  // Create radio buttons
  g1 = new Fl_Group(5, 22, 277, 207, "Select transformation");
  g1->labelsize(16); g1->labelfont(FL_BOLD + FL_ITALIC);
  g1->box(FL_DOWN_BOX);
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
  g2->box(FL_DOWN_BOX);
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
  g3->box(FL_DOWN_BOX);
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

  g4 = new Fl_Group(g1->w()+g2->w()+15, subwin1->y()-8, 270, 55, "Input syntax");
  g4->labelsize(16); g4->labelfont(FL_BOLD + FL_ITALIC);
  g4->box(FL_DOWN_BOX);
//g4->clip_children(1);
  x0 = g1->w()+g2->w()+15; y0 = 25; yinc = 20; w0 = g4->w()-4, h0 = 22;
  cb = new Fl_Check_Button(x0, y0+0*yinc, w0, h0, "Reverse parsing order of xy/\xCF\x86\xCE\xBB");
  cb->callback(rev_cb, NULL);
  g4->end();

  g5 = new Fl_Group(g1->w()+g2->w()+15, subwin1->y()-8+55, g4->w(), 55, "Output format");
  g5->labelsize(16); g5->labelfont(FL_BOLD + FL_ITALIC);
  g5->box(FL_DOWN_BOX);
//g5->clip_children(1);
  x0 = g1->w()+g2->w()+15; y0 = g4->h()+25; yinc = 20; w0 = g5->w()-4, h0 = 22;
  // degree sign:  Unicode: 00B0, UTF8: B0
  cb = new Fl_Check_Button(x0, y0+0*yinc, w0, h0, "Display \xCF\x86\xCE\xBB in D\xB0M'S\" format");
  cb->callback(dms_cb, NULL);
  g5->end();

  // No more widgets in subwin1
  subwin1->resizable(subwin1);
  subwin1->end();

  // Bottom window
  subwin2 = new Fl_Double_Window(0, menubar->y()+menubar->h()+subwin1->h(), mainwin->w(), 137);
  subwin2->box(FL_DOWN_BOX);
//xlog("subwin2-x: %d, subwin2-y: %d\n", subwin2->x(), subwin2->y());
  // x,y is 0.0 from now on

  // Create drag & drop area
  g1 = new Fl_Group(5, 22, subwin2->w()-10, 110, "Drag & drop files to transform");
  g1->labelsize(16); g1->labelfont(FL_BOLD + FL_ITALIC);
  g1->box(FL_DOWN_BOX);
//g1->clip_children(1);

  brow = new Fl_Browser(g1->x(), g1->y(), g1->w(), g1->h());
  dnd = new Fl_DND_Box(brow->x(), brow->y(), brow->w(), brow->h());
  dnd->callback(dnd_cb, brow);

  g1->end();

  // No more widgets in subwin2
  subwin2->resizable(subwin2);
  subwin2->end();

  // No more widgets in mainwin
  mainwin->end();

  // Show main window
  Fl::scheme("gtk+");
  Fl::visual(FL_DOUBLE | FL_INDEX);
#ifdef _WIN32
  mainwin->icon((const void *)LoadIcon(fl_display, "IDI_ICON1"));
#endif
  mainwin->show(argc, argv);

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
#ifdef _WIN32
      xlog("Completed join with thread %08x, status = %d\n",
           (unsigned int)pt->tid.p, pt->sts);
#else
      xlog("Completed join with thread %08x, status = %d\n",
           (unsigned int)pt->tid, pt->sts);
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
