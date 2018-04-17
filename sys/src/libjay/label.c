#include <u.h>
#include <libc.h>
#include <draw.h>
#include <mouse.h>
#include <jay.h>
#include "fns.h"

void _hoverLabel(Widget *w, Mousectl *m);
void _unhoverLabel(Widget *w);

Widget *
createLabel(char *id, Rectangle r, Point p){
  Label *laux = malloc(sizeof(Label));
  if (laux == nil){
    return nil;
  }
  Widget *w = createWidget(id, r, p, LABEL, laux);
  if (w == nil){
    return nil;
  }

  laux->t = nil;
  laux->backColor = jayconfig->mainBackColor;
  laux->textColor = jayconfig->mainTextColor;
  laux->border = 0;
  laux->d3 = 0;
  laux->up = 0;
  laux->w = w;
  w->_hover = _hoverLabel;
  w->_unhover = _unhoverLabel;
  return w;
}

void
_hoverLabel(Widget *w, Mousectl *m){
  if (w->t != LABEL){
    return;
  }
  Label *l = w->w;
  if (l == nil){
    return;
  }
  w->hovered = 1;
  if( w->hover != nil){
    w->hover(w);
  }
  while (ptinrect(m->xy, w->r)){}
}

void
_unhoverLabel(Widget *w){
  if (w->t != LABEL){
    return;
  }
  Label *l = w->w;
  if (l == nil){
    return;
  }
  w->hovered = 0;
  if (w->unhover != nil){
    w->unhover(w);
  }
}

void
_drawLabel(Widget *w, Image *dst){
  if (w->t != LABEL){
    return;
  }
  Label *l = w->w;
  if (l == nil){
    return;
  }

  if(w->i != nil){
    freeimage(w->i);
  }
  w->i = allocimage(display, w->r, RGB24, 1, l->backColor);
  if(l->t != nil){
    //TODO: Calculate correctly the points
    string(w->i, Pt(l->border + 2, l->border + 2), w->i, Pt(l->border + 2, l->border + 2), l->f, l->t);
  }
  if (l->border > 0){
    if (l->d3){
      if(l->up){
        border3d(w->i, w->r, l->border, display->white, display->black, ZP);
      } else {
        border3d(w->i, w->r, l->border, display->black, display->white, ZP);
      }
    } else {
      border(w->i, w->r, l->border, w->i, ZP);
    }
  }
  draw(dst, w->r, w->i, nil, w->p);
}
