#include <u.h>
#include <libc.h>
#include <draw.h>
#include <mouse.h>
#include <jay.h>

Widget *
createWidget(char *id, Rectangle r, Point p, wtype t, void *w){
  Widget *wr = malloc(sizeof(Widget));
  if ( wr == nil){
    return nil;
  }
  wr->id = strdup(id);
  wr->p=p;
  wr->r=r;
  wr->t=t;
  wr->w=w;
  wr->i=nil;
  wr->draw=nil;
  wr->hover=nil;
  return wr;
}
