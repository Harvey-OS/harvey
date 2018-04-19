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
