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
int _addWidgetToPanel(Widget *me, Widget *new, Point pos);

Widget *
createPanel(char *id, Rectangle r, Point p){
  Panel *paux = malloc(sizeof(Panel));
  if (paux == nil){
    return nil;
  }
  Widget *w = createWidget(id, r, PANEL, paux);
  if (w == nil){
    return nil;
  }
  paux->w=w;
  paux->l = nil;
  paux->backColor = jayconfig->mainBackColor;
  w->_hover=_hoverPanel;
  w->_unhover=_unhoverPanel;
  w->_draw=_drawPanel;
  w->addWidget=_addWidgetToPanel;
  w->p=p;
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
  if (w->hover != nil){
    w->hover(w);
  }
  while(ptinrect(m->xy, w->r)){
    readmouse(m);
    for (WListElement *i=p->l; i != nil; i=i->next){
      if(ptinrect(m->xy, i->w->r)){
        if (!i->w->hovered){
          i->w->_hover(i->w, m);
          i->w->_unhover(i->w);
        }
      }
    }
    yield();
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

  real.x = me->p.x + pos.x;
  real.y = me->p.y + pos.y;
  new->p = real;
  new->r = Rect(new->r.min.x + real.x, new->r.min.y + real.y, new->r.max.x + real.x, new->r.max.y+ real.y);

  if(p->l == nil){
    p->l = createWListElement(new);
    if (p->l == nil){
      return 0;
    }
    return 1;
  }

  return addWListElement(p->l, new);
}
