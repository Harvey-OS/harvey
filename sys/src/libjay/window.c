#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <jay.h>
#include "fns.h"

void mousethread(void* v);

const unsigned long jaystacksize = 8192;

Mousectl *m;

Widget *
initjayapp(char *name){
  if(initdraw(nil, nil, name) < 0){
    sysfatal("initdraw failed");
  }

  m = initmouse(nil, screen);
  if (m == nil){
    sysfatal("mouse error");
  }
  initdefaultconfig();
  Widget *w = createPanel1("main", screen->r, screen->r.min);
  if (w == nil){
    sysfatal("createPanel failed");
  }
  return w;
}

void
startjayapp(Widget * w){
  Channel *exitchan;
  if (w == nil){
    sysfatal("startjayapp nil widget");
  }
  exitchan = chancreate(sizeof(int), 0);
  w->_draw(w, screen);
  flushimage(display, 1);
  yield();
  threadcreate(mousethread, w, jaystacksize);
  recv(exitchan, nil);
}

void
mousethread(void* v){
  Widget *w = v;
  threadsetname("mousethread");
  enum {
		MReshape,
		MMouse,
		NALT
	};
	static Alt alts[NALT+1];
  alts[MReshape].c = m->resizec;
	alts[MReshape].v = nil;
	alts[MReshape].op = CHANRCV;
	alts[MMouse].c = m->c;
	alts[MMouse].v = (Mouse *)m;
	alts[MMouse].op = CHANRCV;
	alts[NALT].op = CHANEND;
  for(;;){
    switch(alt(alts)){
      case MMouse:
        w->_hover(w, m);
        break;
      case MReshape:
        if (getwindow(display, Refnone)<0){
          sysfatal("failed to re-attach window");
        }
        Point d = Pt(screen->r.min.x - w->p.x, screen->r.min.y - w->p.y);
        w->_resize(w, d);
        break;
    }
  }
}
