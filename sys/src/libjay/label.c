#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <jay.h>
#include "fns.h"

void _hoverLabel(Widget *w, Mouse *m);
void _unhoverLabel(Widget *w);
void _drawLabel(Widget *w, Image *dst);
void _redrawLabel(Widget *w);
void _setTextLabel(Label *l, const char *t);
char *_getTextLabel(Label *l);
static Rectangle autosizeLabel(Widget *w);
void _clickLabel(Widget *w, Mouse *m);
void _dclickLabel(Widget *w, Mouse *m);
void _mPressDownLabel(Widget *w, Mouse *m);
void _mPressUpLabel(Widget *w, Mouse *m);

Widget *
createLabel(char *id, int height, int width){
  Label *laux = malloc(sizeof(Label));
  if (laux == nil){
    return nil;
  }
  Widget *w = createWidget(id, height, width, LABEL, laux);
  if (w == nil){
    return nil;
  }
  laux->f = openfont(display, jayconfig->fontPath);
  if (laux->f == nil){
    return nil;
  }
  laux->t = nil;
  laux->backColor = jayconfig->mainBackColor;
  laux->textColor = jayconfig->mainTextColor;
  laux->border = 0;
  laux->d3 = 0;
  laux->up = 0;
  laux->w = w;
  laux->gettext=_getTextLabel;
  laux->setText=_setTextLabel;
  w->_hover = _hoverLabel;
  w->_unhover = _unhoverLabel;
  w->_draw=_drawLabel;
  w->_redraw=_redrawLabel;
  w->_resize=_simpleResize;
  w->_click=_clickLabel;
  w->_dclick=_dclickLabel;
  w->_mpressdown=_mPressDownLabel;
  w->_mpressup=_mPressUpLabel;
  return w;
}

static int
checkLabel(Widget *w){
  if (w == nil || w->t != LABEL || w->w == nil){
    return 0;
  }
  return 1;
}

void
_hoverLabel(Widget *w, Mouse *m){
  if(!checkLabel(w)){
    return;
  }

  w->hovered=1;
  if( w->hover != nil){
    w->hover(w);
    w->_redraw(w);
  }
}

void
_unhoverLabel(Widget *w){
  if(!checkLabel(w)){
    return;
  }

  w->hovered = 0;
  if (w->unhover != nil){
    w->unhover(w);
    w->_redraw(w);
  }
}

void
_drawLabel(Widget *w, Image *dst){
  if(!checkLabel(w) || w->father == nil || !w->visible || !w->father->visible){
    return;
  }

  Label *l = w->w;

  if(w->i != nil){
    freeimage(w->i);
  }
  if (w->height == 0 || w->width == 0){
    return;
  }
  if(w->height<0 || w->width<0){
    w->r = autosizeLabel(w);
  }
  w->i = allocimage(display, w->r, RGB24, 1, l->backColor);
  if (w->i == nil){
    sysfatal("_drawLabel: %r");
  }

  if(l->t != nil){
    Image *i = allocimage(display, Rect(0,0,1,1), RGB24, 1, l->textColor);
    Rectangle r = insetrect(w->r, l->border);
    Point s = stringsize(l->f, l->t);
    Point p = Pt(r.min.x, r.min.y + ((Dy(r)/2) - (s.y/2) + 1) );
    string(w->i, p, i, p, l->f, l->t);
    freeimage(i);
  }

  if (l->border > 0){
    if (l->d3){
      Image *i=allocimage(display, Rect(0,0,1,1), RGB24, 1, 0xCCCCCCFF);
      if(l->up){
        border3d(w->i, w->r, l->border, i, display->black, ZP);
      } else {
        border3d(w->i, w->r, l->border, display->black, i, ZP);
      }
      freeimage(i);
    } else {
      border(w->i, w->r, l->border, display->black, ZP);
    }
  }
  draw(dst, w->r, w->i, nil, w->p);
  if(w->draw != nil){
    w->draw(w);
  }
  flushimage(display, 1);
}

void
_redrawLabel(Widget *w){
  if(!checkLabel(w) || w->father == nil || !w->visible || !w->father->visible){
    return;
  }
  if (w->autosize){
    Rectangle r = autosizeLabel(w);
    if(!eqrect(r, w->r)){
      // The geometry has changed, so we need to redraw all
      w->father->_redraw(w->father);
    }
  }
  w->_draw(w, screen);
}

void
_setTextLabel(Label *l, const char *t){
  if (l->t != nil){
    free(l->t);
  }
  l->t=strdup(t);
  l->w->_redraw(l->w);
}

char *
_getTextLabel(Label *l){
  return strdup(l->t);
}

static Rectangle
autosizeLabel(Widget *w){
  int fh, fw; //Final heigh, final width
  if(!checkLabel(w)){
    return ZR;
  }
  Label *l = w->w;

  fh = w->height;
  fw = w->width;
  if (l->t == nil || strcmp("", l->t) == 0){
    if (fh<0){
      fh=20;
    }
    if (fw<0){
      fw=12;
    }
  } else {
    Point p = stringsize(l->f, l->t);
    if (fh<0){
      fh = p.y + (2*l->border); //2*border because is a box, we have 2 sides in this axis.
    }
    if(fw<0){
      fw = p.x + (2*l->border); //2*border because is a box, we have 2 sides in this axis.
    }
  }
  return Rect(w->p.x, w->p.y, w->p.x + fw, w->p.y + fh);
}

void
_clickLabel(Widget *w, Mouse *m){
  if(!checkLabel(w)){
    return;
  }

  if(w->click != nil){
    w->click(w, m);
    w->_redraw(w);
  }
}

void
_dclickLabel(Widget *w, Mouse *m){
  if(!checkLabel(w)){
    return;
  }

  if(w->dclick != nil){
    w->dclick(w, m);
    w->_redraw(w);
  }
}

void
_mPressDownLabel(Widget *w, Mouse *m){
  if(!checkLabel(w)){
    return;
  }

  if(w->mpressdown != nil){
    w->mpressdown(w, m);
    w->_redraw(w);
  }
}

void
_mPressUpLabel(Widget *w, Mouse *m){
  if(!checkLabel(w)){
    return;
  }

  if(w->mpressup != nil){
    w->mpressup(w, m);
    w->_redraw(w);
  }
}
