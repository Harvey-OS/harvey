#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

static void
addEntry(StartMenu *sm, char *name, char *action, StartMenu *submenu, Point p){
  if (name == nil)
    return;
  MenuEntry *e = sm->entries[sm->numEntries] = malloc(sizeof(MenuEntry));
  sm->numEntries++;

  e->name = estrdup(name);
  if (action != nil)
    e->action = estrdup(action);
  else
    e->action = nil;
  e->submenu = submenu;

  Point ssize = stringsize(font, e->name);

  Rectangle r = Rect(p.x, p.y, p.x + ssize.x + 2*Borderwidth, p.y + 2*Borderwidth + ssize.y);
  e->i = allocimage(display, r, CMAP8, 1, jayconfig->mainMenuColor);
  e->ih = allocimage(display, r, CMAP8, 1, jayconfig->mainMenuHooverColor);

  Point pt = Pt(p.x + Borderwidth, p.y+Borderwidth);
  string(e->i, pt, wht, pt, font, e->name);
  string(e->ih, pt, wht, pt, font, e->name);
}

static void
resizeImageEntry(MenuEntry *e, int sizex){
  Image *aux;
  aux = e->i;
  e->i = allocimage(display, Rect(aux->r.min.x, aux->r.min.y, aux->r.min.x + sizex, aux->r.max.y), CMAP8, 1, jayconfig->mainMenuColor);
  draw(e->i, aux->r, aux, nil, e->i->r.min);
  freeimage(aux);

  aux = e->ih;
  e->ih = allocimage(display, Rect(aux->r.min.x, aux->r.min.y, aux->r.min.x + sizex, aux->r.max.y), e->ih->chan, 1, jayconfig->mainMenuHooverColor);
  draw(e->ih, aux->r, aux, nil, e->ih->r.min);
  freeimage(aux);
}

static void
printEntries(StartMenu *sm) {
  MenuEntry *e;
  int maxx = 0;
  for (int i=0; i<sm->numEntries; i++){
    e = sm->entries[i];
    if (Dx(e->i->r) > maxx){
      maxx = Dx(e->i->r);
    }
  }

  if (maxx < Dx(sm->i->r)){
    maxx = Dx(sm->i->r);
  }


  for (int i=0; i<sm->numEntries; i++){
    e = sm->entries[i];
    resizeImageEntry(e, maxx);
    draw(sm->i, e->i->r, e->i, nil, e->i->r.min);
  }
}

void
hoovermenu(StartMenu *sm, Point p) {
  MenuEntry *e;
  for(int i=0; i<sm->numEntries; i++){
    e = sm->entries[i];
    if(ptinrect(p, e->i->r)){
      draw(sm->i, e->i->r, e->ih, nil, e->i->r.min);
    } else {
      draw(sm->i, e->i->r, e->i, nil, e->i->r.min);
    }
  }
}

void
execmenu(StartMenu *sm, Point p) {
  MenuEntry *e;
  for(int i=0; i<sm->numEntries; i++){
    e = sm->entries[i];
    if(ptinrect(p, e->i->r) && e->action != nil){
      new(wcenter(1.6*400, 400), FALSE, scrolling, 0, nil, e->action, nil);
    }
  }
}

void
freeStartMenu(StartMenu *sm) {
  if(sm == nil)
    return;
  MenuEntry *e;
  for (int i=0; i<sm->numEntries; i++){
    e = sm->entries[i];
    freeimage(e->i);
    freeimage(e->ih);
    free(e->name);
    free(e->action);
    freeStartMenu(e->submenu);
    free(e);
  }
  sm->numEntries = 0;
  freeimage(sm->i);
  free(sm);
}

static char *
getcharproperty(char *start, char *end, char *property){
  char *s, *aux, *r;
  int size;
  s = start;
  while (s < end && *s!='}' && strncmp(s, property, strlen(property))){
    if(*s == '['){
      //ignore submenu
      int o = 1;
      s++;
      while ( s < end && o > 0 ){
        while (s < end && *s != ']'){
          if(*s == '['){
            o++;
          }
          s++;
        }
        o--;
      }
    }
    s++;
  }
  while(s < end && *s!='}' && *s!='"'){
    s++;
  }
  if (s==end)
    return nil;
  aux = ++s;
  while(s < end && *s!='}' && *s!='"'){
    s++;
  }
  if (s==end || *s == '}' || s == aux)
    return nil;
  size = s - aux;
  r = malloc(size + 1);
  strncpy(r, aux, size);
  aux=r;
  aux += size;
  *aux = '\0';
  return r;
}

static StartMenu *
parsesubmenuproperty(char *start, char *end){
  //TODO
  return nil;
}

static void
parseentry(StartMenu *menu, char *start, char *end) {
  char *name, *action;
  Point p;
  if (menu->numEntries==0) {
    p = menu->i->r.min;
  } else {
    p = Pt(menu->i->r.min.x, menu->entries[menu->numEntries - 1]->i->r.max.y);
  }
  name = getcharproperty(start, end, "name");
  action = getcharproperty(start, end, "action");
  addEntry(menu, name, action, parsesubmenuproperty(start, end), p);
  free(name);
  free(action);
}


static StartMenu *
parsemenu(char *start, char *end, Point p, int minSize){
  StartMenu *sm;
  char *s, *e;
  int o;
  s = start;
  if (*s != '['){
    return nil;
  }
  sm = malloc(sizeof(StartMenu));
  sm->numEntries=0;
  sm->i = allocwindow(wscreen, Rect(p.x +2, p.y + 2, p.x + minSize, p.y + 1.6*minSize), Refnone, jayconfig->mainMenuColor);

  while(s < end){
    while (s < end && *s !='{'){
      if(*s == ']')
        return sm;
      s++;
    }
    if(*s != '{')
      return nil;

    e = s + 1;
    o = 1;
    while (e < end && o > 0){
      while (e < end && *e !='}'){
        if (*e == '{')
          o++;
        e++;
      }
      o--;
    }
    if (e == end){
      freeStartMenu(sm);
      return nil;
    }
    parseentry(sm, s, e);
    s = ++e;
  }
  return sm;
}

static StartMenu *
parsemenufile(TPanel *p, const char *path){
  int fd;
  Dir *d;
  char *s;

  fd = open(path, OREAD);
  if (fd < 0){
    error("openning menu file");
    return nil;
  }
  d = dirfstat(fd);
  s = malloc(sizeof(char) * d->length);
  if (read(fd, s, d->length)>0){
    close(fd);
    return parsemenu(s, s + d->length - 1, Pt(p->r.min.x, p->r.max.y), 100);
  }
  return nil;
}


StartMenu *
loadStartMenu(TPanel *p){
  StartMenu *sm = parsemenufile(p, "/usr/harvey/lib/menu.conf");
  printEntries(sm);
  return sm;
}
