#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <jay.h>
#include "fns.h"


static Widget *
genWidget(char *id, wtype t, void *w){
  Widget *wr = malloc(sizeof(Widget));
  if ( wr == nil){
    return nil;
  }
  wr->id = strdup(id);
  wr->p=ZP;
  wr->r=ZR;
  wr->t=t;
  wr->w=w;
  wr->i=nil;
  wr->lh=nil;
  wr->hovered=0;
  wr->draw=nil;
  wr->hover=nil;
  wr->unhover=nil;
  return wr;
}

Widget *
createWidget(char *id, int height, int width, wtype t, void *w){
  Widget *wr = genWidget(id, t, w);
  if ( wr == nil){
    return nil;
  }
  wr->r=ZR;
  wr->height = height;
  wr->width = width;
  if (height < 0 || width < 0){
    wr->autosize=1;
  } else {
    wr->autosize = 0;
  }
  return wr;
}

Widget *
createWidget1(char *id, Rectangle r, wtype t, void *w){
  Widget *wr = genWidget(id, t, w);
  if ( wr == nil){
    return nil;
  }
  wr->r=r;
  wr->height = r.max.y - r.min.y;
  wr->width = r.max.x - r.min.x;
  wr->autosize=0;
  return wr;
}
