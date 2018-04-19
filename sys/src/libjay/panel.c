#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <jay.h>
#include "fns.h"

void _hoverPanel(Widget *w, Mousectl *m);
void _unhoverPanel(Widget *w);
void _drawPanel(Widget *w, Image *dst);
void _redrawPanel(Widget *w);
int _addWidgetToPanel(Widget *me, Widget *new, Point pos);

static void
genPanel(Widget *w, Panel *paux, Point p){
  paux->w=w;
  paux->l = nil;
  paux->backColor = jayconfig->mainBackColor;
  w->_hover=_hoverPanel;
  w->_unhover=_unhoverPanel;
  w->_draw=_drawPanel;
  w->_redraw=_redrawPanel;
  w->addWidget=_addWidgetToPanel;
  w->p=p;
}

Widget *
createPanel1(char *id, Rectangle r, Point p){
  Panel *paux = malloc(sizeof(Panel));
  if (paux == nil){
    return nil;
  }
  Widget *w = createWidget1(id, r, PANEL, paux);
  if (w == nil){
    return nil;
  }
  genPanel(w, paux, p);
  return w;
}

Widget *
createPanel(char *id, int height, int width, Point p){
  Panel *paux = malloc(sizeof(Panel));
  if (paux == nil){
    return nil;
  }
  Widget *w = createWidget(id, height, width, PANEL, paux);
  if (w == nil){
    return nil;
  }
  genPanel(w, paux, p);
  return w;
}

void
_hoverPanel(Widget *w, Mousectl *m){
  if (w->t != PANEL){
    return;
  }

  Panel *p = (Panel *)w->w;
  if (p == nil){
    return;
  }
  w->hovered=1;
  if(w->lh != nil){
    if(ptinrect(m->xy, w->lh->r)){
      if(w->lh->t == PANEL){
        //It's a container, so we need to send again the _hover event
        w->lh->_hover(w->lh, m);
      }
      return;
    }
    w->lh->_unhover(w->lh);
    w->lh=nil;
  }
  for (WListElement *i=p->l; i != nil; i=i->next){
    if(ptinrect(m->xy, i->w->r)){
      if (!i->w->hovered){
        i->w->_hover(i->w, m);
        w->lh = i->w;
        return;
      }
    }
  }
}

void
_unhoverPanel(Widget *w){
  w->hovered = 0;
  if (w->unhover != nil){
    w->unhover(w);
  }
}

void
_drawPanel(Widget *w, Image *dst) {
  if (w->t != PANEL){
    return;
  }

  Panel *p = (Panel *)w->w;
  if (p == nil){
    return;
  }
  if (w->i != nil){
    freeimage(w->i);
  }
  w->i = allocimage(display, w->r, RGB24, 1, p->backColor);
  if(w->i == nil){
    sysfatal("_drawPanel: %r");
  }

  for (WListElement *e=p->l; e != nil; e=e->next){
    e->w->_draw(e->w, w->i);
  }
  draw(dst, w->r, w->i, nil, w->p);
}

int
_addWidgetToPanel(Widget *me, Widget *new, Point pos){
  Point real;
  if (me->t != PANEL){
    return 0;
  }
  Panel *p = (Panel *)me->w;
  if (p == nil){
    return 0;
  }

  real.x = me->r.min.x + pos.x;
  real.y = me->r.min.y + pos.y;
  new->p = real;
  if (!new->autosize){
    new->r = Rect(real.x, real.y, real.x + new->width, real.y + new->height);
  }
  if(p->l == nil){
    p->l = createWListElement(new);
    if (p->l == nil){
      return 0;
    }
  } else if (!addWListElement(p->l, new)) {
    return 0;
  }
  new->father = me;
  return 1;
}

void
_redrawPanel(Widget *w){
  if (w->t != PANEL){
    return;
  }
  Panel *p = (Panel *)w->w;
  if (p == nil){
    return;
  }
  if (w->father==nil){
    //I'm the main panel
    w->_draw(w, screen);
  } else {
    w->father->_redraw(w->father);
  }
}
