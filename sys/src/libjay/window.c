#include <u.h>
#include <libc.h>
#include <draw.h>
#include <mouse.h>
#include <jay.h>
#include "fns.h"

Widget *
initjayapp(char *name){
  if(initdraw(nil, nil, name) < 0){
    sysfatal("initdraw failed");
  }
  Widget *w = createPanel("main", screen->r, ZP);
  if (w == nil){
    sysfatal("createPanel failed");
  }
  return w;
}
