#include <u.h>
#include <libc.h>
#include <draw.h>
#include <mouse.h>
#include <jay.h>
#include "fns.h"

void _hoverPanel(Widget *w, Mousectl *m);
void _unhoverPanel(Widget *w);

Widget *
createPanel(char *id, Rectangle r, Point p){
  Panel *paux = malloc(sizeof(Panel));
  if (paux == nil){
    return nil;
  }
  Widget *w = createWidget(id, r, p, PANEL, paux);
  if (w == nil){
    return nil;
  }
  paux->w=w;
  paux->l = nil;
  paux->backColor = jayconfig->mainBackColor;
  w->_hover=_hoverPanel;
  w->_unhover=_unhoverPanel;
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
    for (WListElement *i=p->l; i != nil; i=i->next){
      if(ptinrect(m->xy, i->w->r)){
        if (!i->w->hovered){
          i->w->_hover(i->w, m);
          i->w->_unhover(i->w);
        }
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
  for (WListElement *e=p->l; e != nil; e=e->next){
    e->w->_draw(e->w, w->i);
  }
  draw(dst, w->r, w->i, nil, w->p);
}
