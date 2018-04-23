#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <jay.h>
#include "fns.h"

void _hoverPanel(Widget *w, Mouse *m);
void _unhoverPanel(Widget *w);
void _drawPanel(Widget *w, Image *dst);
void _redrawPanel(Widget *w);
int _addWidgetToPanel(Widget *me, Widget *new, Point pos);
void _resizePanel(Widget *w, Point d);
void _clickPanel(Widget *w, Mouse *m);
void _dclickPanel(Widget *w, Mouse *m);
void _mPressDownPanel(Widget *w, Mouse *m);
void _mPressUpPanel(Widget *w, Mouse *m);

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
  w->_resize=_resizePanel;
  w->_click=_clickPanel;
  w->_dclick=_dclickPanel;
  w->_mpressdown=_mPressDownPanel;
  w->_mpressup=_mPressUpPanel;
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

static int
checkPanel(Widget *w){
  if (w == nil || w->t != PANEL || w->w == nil || !w->visible){
    return 0;
  }
  return 1;
}

void
_hoverPanel(Widget *w, Mouse *m){
  if(!checkPanel(w)){
    return;
  }
  Panel *p = (Panel *)w->w;
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
  if(!checkPanel(w)){
    return;
  }
  w->hovered = 0;
  if (w->unhover != nil){
    w->unhover(w);
  }
}

void
_drawPanel(Widget *w, Image *dst) {
  if(!checkPanel(w)){
    return;
  }
  Panel *p = (Panel *)w->w;

  if (w->i != nil){
    freeimage(w->i);
  }
  w->i = allocimage(display, w->r, RGB24, 1, p->backColor);
  if(w->i == nil){
    sysfatal("_drawPanel: %r");
  }

  for (WListElement *e=p->l; e != nil; e=e->next){
    if(e->w->visible){
      e->w->_draw(e->w, w->i);
    }
  }
  draw(dst, w->r, w->i, nil, w->p);
}

int
_addWidgetToPanel(Widget *me, Widget *new, Point pos){
  Point real;
  if (me == nil || me->t != PANEL || me->w == nil){
    return 0;
  }
  Panel *p = (Panel *)me->w;

  real.x = me->r.min.x + pos.x;
  real.y = me->r.min.y + pos.y;
  new->p = real;
  new->pos = pos;
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
  if(!checkPanel(w)){
    return;
  }
  if (w->father==nil){
    //I'm the main panel
    w->_draw(w, screen);
  } else {
    w->father->_redraw(w->father);
  }
}

void
_resizePanel(Widget *w, Point d){
  if(!checkPanel(w)){
    return;
  }
  Panel *p = (Panel *)w->w;

  _simpleResize(w, d);
  for (WListElement *e=p->l; e != nil; e=e->next){
    e->w->_resize(e->w, d);
  }
  if(w->father == nil){
    //I'm the main panel
    w->_draw(w, screen);
  }
  flushimage(display, 1);
}

void
_clickPanel(Widget *w, Mouse *m){
  if(!checkPanel(w)){
    return;
  }

  w->_hover(w, m);

  if(w->click != nil){
    w->click(w, m);
  }

  if (w->lh != nil){
    w->lh->_click(w->lh, m);
  }
  w->_redraw(w);
  flushimage(display, 1);
}

void
_dclickPanel(Widget *w, Mouse *m){
  if(!checkPanel(w)){
    return;
  }

  w->_hover(w, m);

  if(w->dclick != nil){
    w->dclick(w, m);
  }

  if (w->lh != nil){
    w->lh->_dclick(w->lh, m);
  }
  w->_redraw(w);
  flushimage(display, 1);
}

void
_mPressDownPanel(Widget *w, Mouse *m){
  if(!checkPanel(w)){
    return;
  }

  w->_hover(w, m);

  if(w->mpressdown != nil){
    w->mpressdown(w, m);
  }

  if (w->lh != nil){
    w->lh->_mpressdown(w->lh, m);
  }
  w->_redraw(w);
  flushimage(display, 1);
}

void
_mPressUpPanel(Widget *w, Mouse *m){
  if(!checkPanel(w)){
    return;
  }

  w->_hover(w, m);

  if(w->mpressup != nil){
    w->mpressup(w, m);
  }

  if (w->lh != nil){
    w->lh->_mpressup(w->lh, m);
  }
  w->_redraw(w);
  flushimage(display, 1);
}
