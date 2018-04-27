#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <jay.h>
#include "fns.h"

WListElement *
createWListElement(Widget *w){
  WListElement *e = malloc(sizeof(WListElement));
  if (e == nil){
    return nil;
  }
  e->next=nil;
  e->prev=nil;
  e->w=w;
  return e;
}

WListElement *
getWListElementByID(WListElement *list, char *id){
  for(WListElement *e = list; e != nil ; e=e->next ){
    if(strcmp(id, e->w->id) == 0){
      return e;
    }
  }
  return nil;
}

WListElement *
getWListElementByPos(WListElement *list, int pos){
  int i=0;
  for(WListElement *e = list; e != nil ; e=e->next ){
    if(pos == i){
      return e;
    }
    i++;
  }
  return nil;
}

int
addWListElement(WListElement *list, Widget *w){
  if(list == nil){
    return 0;
  }
  if(getWListElementByID(list, w->id) != nil){
    return 0;
  }
  WListElement *le = createWListElement(w);
  if(le == nil){
    return 0;
  }
  for(WListElement *e = list; e != nil ; e=e->next ){
    if(e->next == nil){
      e->next = le;
      le->prev = e;
      return 1;
    }
  }
  return 0;
}

Widget *
extractWidgetByID(WListElement *list, char *id){
  WListElement *e = getWListElementByID(list, id);
  if (e == nil){
    return nil;
  }
  if(e->prev == nil){
    list = e->next;
  } else {
    e->prev->next = e->next;
  }
  if(e->next != nil){
    e->next->prev = e->prev;
  }
  Widget *w = e->w;
  free(e);
  return w;
}

Widget *
extractWidgetByPos(WListElement *list, int pos){
  WListElement *e = getWListElementByPos(list, pos);
  if (e == nil){
    return nil;
  }
  return extractWidgetByID(list, e->w->id);
}

void
freeWListElement(WListElement *e){
  if(e->w != nil){
    e->w->freeWidget(e->w);
    e->w=nil;
  }
  free(e);
}

void
destroyWList(WListElement *list){
  WListElement *e;
  for(e = list; e != nil ; e=e->next ){
    if(e->prev!=nil){
      freeWListElement(e->prev);
    }
  }
  freeWListElement(e);
}

int
countWListElements(WListElement *list){
  int i=0;
  for(WListElement *e = list; e != nil ; e=e->next ){
    i++;
  }
  return i;
}
