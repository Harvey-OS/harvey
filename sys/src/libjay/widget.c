#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <jay.h>
#include "fns.h"

void setVisible(Widget *w, int visible);

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
  wr->father=nil;
  wr->hovered=0;
  wr->visible=1;
  wr->draw=nil;
  wr->_draw=nil;
  wr->_redraw=nil;
  wr->hover=nil;
  wr->_hover=nil;
  wr->unhover=nil;
  wr->_unhover=nil;
  wr->resize=nil;
  wr->click=nil;
  wr->_click=nil;
  wr->dclick=nil;
  wr->_dclick=nil;
  wr->mpressdown=nil;
  wr->_mpressdown=nil;
  wr->mpressup=nil;
  wr->_mpressup=nil;
  wr->change=nil;
  wr->_change=nil;
  wr->setVisible=setVisible;
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

static int
checkWidget(Widget *w){
  if (w == nil || w->w == nil || w->father == nil){
    return 0;
  }
  return 1;
}

void
setVisible(Widget *w, int visible){
  if(!checkWidget(w)){
    return;
  }
  w->visible=visible;
  if (visible){
    w->_redraw(w);
  } else {
    //To dissapear we need to redraw the father
    w->father->_redraw(w->father);
  }
}
