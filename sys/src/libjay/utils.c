#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <jay.h>
#include "fns.h"

void _simpleResize(Widget *w, Point d){
  w->p = Pt(w->p.x + d.x, w->p.y + d.y);
  if (w->height > 0 && w->width > 0){
    w->r = Rect(w->p.x, w->p.y, w->p.x + w->width, w->p.y + w->height);
  }
  if (w->resize != nil){
    w->resize(w);
  }
}
