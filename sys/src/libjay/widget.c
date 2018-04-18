#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <jay.h>
#include "fns.h"

Widget *
createWidget(char *id, Rectangle r, wtype t, void *w){
  Widget *wr = malloc(sizeof(Widget));
  if ( wr == nil){
    return nil;
  }
  wr->id = strdup(id);
  wr->p=ZP;
  wr->r=r;
  wr->t=t;
  wr->w=w;
  wr->i=nil;
  wr->draw=nil;
  wr->hover=nil;
  wr->unhover=nil;
  return wr;
}
