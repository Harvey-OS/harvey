#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <fcall.h>
#include <jay.h>
#include "dat.h"
#include "fns.h"

void
drawstartbutton(TPanel *p, int pressed) {
  if (p->sb == nil){
    int x = stringwidth(font, "Harvey");
    Rectangle r = Rect(p->r.min.x + Borderwidth, p->r.min.y + Borderwidth, p->r.min.x + 1.5*x + 2*Borderwidth, p->r.max.y - Borderwidth);
    Point pt;
    pt.x = r.min.x;
  	pt.x += (Dx(r)-x)/2;
  	pt.y = Borderwidth + r.min.y;
    p->sb = allocimage(display, r, CMAP8, 1, jayconfig->taskPanelColor);
    string(p->sb, pt, wht, pt, font, "Harvey");
  }

  if(pressed){
    border3d(p->sb, p->sb->r, 1, blk, wht, p->sb->r.min);
  } else {
    border3d(p->sb, p->sb->r, 1, wht, blk, p->sb->r.min);
  }
  draw(p->i, p->sb->r, p->sb, nil, p->sb->r.min);
}

void
freepanel() {
  freeimage(taskPanel->i);
  freeimage(taskPanel->sb);
  free(taskPanel);
  taskPanel=nil;
}

void
redrawpanel() {
  if (taskPanel != nil){
    freepanel();
  }
  taskPanel = malloc(sizeof(TPanel));
  taskPanel->position = Up;
  taskPanel->sb=nil;
  taskPanel->r = Rect(view->r.min.x, view->r.min.y, view->r.max.x, view->r.min.y + 30);
  windowspace = rectsubrect(view->r, taskPanel->r, taskPanel->position);
  taskPanel->i = allocwindow(wscreen, taskPanel->r, Refbackup, jayconfig->taskPanelColor);
  if (taskPanel->i == nil){
    error("can't allocate taskPanel");
  }
  drawstartbutton(taskPanel, 0);
  flushimage(display, 1);
}

void
initpanel() {
  redrawpanel();
}

void
pressstartbutton(TPanel *p) {
  int cleaned = 1;
  drawstartbutton(p, 1);
  StartMenu *menu = loadStartMenu(p);
  for(;;){
    readmouse(mousectl);
    if(mousectl->buttons && !ptinrect(mouse->xy, menu->i->r)){
      freeStartMenu(menu);
      drawstartbutton(p, 0);
      return;
    }
    if(ptinrect(mouse->xy, menu->i->r)){
      hoovermenu(menu, mouse->xy);
      cleaned = 0;
      if(mouse->buttons == 1){
        execmenu(menu, mouse->xy);
        freeStartMenu(menu);
        drawstartbutton(p, 0);
        return;
      }
    } else if(!cleaned){
      hoovermenu(menu, mouse->xy);
      cleaned = 1;
    }
  }
}

void
clickpanel(Point p, TPanel *tp){
  if (ptinrect(p, tp->sb->r)){
    pressstartbutton(tp);
  }
}
