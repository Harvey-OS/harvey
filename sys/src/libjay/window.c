#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <jay.h>
#include "fns.h"

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
  if (w == nil){
    sysfatal("startjayapp nil widget");
  }
  w->_draw(w, screen);
  flushimage(display, 1);
  yield();
  for(;;){
    readmouse(m);
    w->_hover(w, m);
    w->_unhover(w);
  }
}
