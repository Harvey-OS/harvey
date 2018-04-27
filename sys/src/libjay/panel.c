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
void _freePanel(Widget *w);
void _deleteWidgetPanel(Widget *me, char *id);
int _listWidgetsPanel(Widget *me, char ***list);
Widget * _extractWidgetPanel(Widget *me, char *id);
Widget * _getWidgetPanel(Widget *me, char *id);

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
  w->freeWidget=_freePanel;
  w->deleteWidget=_deleteWidgetPanel;
  w->listWidgets=_listWidgetsPanel;
  w->extractWidget=_extractWidgetPanel;
  w->getWidget=_getWidgetPanel;
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
  if(w->hover!=nil){
    w->hover(w);
    w->_redraw(w);
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
    w->_redraw(w);
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
  flushimage(display, 1);
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
  if(w->father==nil || !w->autosize){
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

  if (w->lh == nil){
    //Click on the panel
    if(w->click != nil){
      w->click(w, m);
      w->_redraw(w);
    }
  }else{
    //Click on a widget over the panel
    w->lh->_click(w->lh, m);
  }
}

void
_dclickPanel(Widget *w, Mouse *m){
  if(!checkPanel(w)){
    return;
  }

  w->_hover(w, m);

  if (w->lh == nil){
    //Click on the panel
    if(w->dclick != nil){
      w->dclick(w, m);
      w->_redraw(w);
    }
  }else{
    //Click on a widget over the panel
    w->lh->_dclick(w->lh, m);
  }
}

void
_mPressDownPanel(Widget *w, Mouse *m){
  if(!checkPanel(w)){
    return;
  }

  w->_hover(w, m);

  if (w->lh == nil){
    //Press down mouse button on the panel
    if(w->mpressdown != nil){
      w->mpressdown(w, m);
      w->_redraw(w);
    }
  }else{
    //Press down mouse button on a widget over the panel
    w->lh->_mpressdown(w->lh, m);
  }
}

void
_mPressUpPanel(Widget *w, Mouse *m){
  if(!checkPanel(w)){
    return;
  }

  w->_hover(w, m);

  if (w->lh == nil){
    //Press down mouse button on the panel
    if(w->mpressup != nil){
      w->mpressup(w, m);
      w->_redraw(w);
    }
  }else{
    //Press down mouse button on a widget over the panel
    w->lh->_mpressup(w->lh, m);
  }
}

void
_freePanel(Widget *w){
  if(!checkPanel(w)){
    return;
  }
  Panel *p = w->w;
  destroyWList(p->l);
  free(p);
  freeWidget(w);
}

Widget *
_getWidgetPanel(Widget *me, char *id){
  if(!checkPanel(me)){
    return nil;
  }
  Panel *p = me->w;
  WListElement *e = getWListElementByID(p->l, id);
  if (e == nil){
    return nil;
  }
  return e->w;
}

Widget *
_extractWidgetPanel(Widget *me, char *id){
  if(!checkPanel(me)){
    return nil;
  }
  Panel *p = me->w;
  return extractWidgetByID(p->l, id);
}

int
_listWidgetsPanel(Widget *me, char ***list){
  if(!checkPanel(me)){
    return 0;
  }
  Panel *p = me->w;
  int n = countWListElements(p->l);
  *list = malloc(n);
  int i=0;
  for(WListElement *e = p->l; e != nil ; e=e->next ){
    *((*list)+i) = strdup(e->w->id);
    i++;
  }
  return n;
}

void
_deleteWidgetPanel(Widget *me, char *id){
  if(!checkPanel(me)){
    return;
  }
  Panel *p = me->w;
  Widget *w = extractWidgetByID(p->l, id);
  w->freeWidget(w);
  me->_redraw(me);
}
