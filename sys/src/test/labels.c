#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <jay.h>

void
threadmain(int argc, char *argv[]) {
  print("Iniciando labels\n");
  Widget * w = initjayapp("labels");
  if (w==nil){
    sysfatal("Error initial panel");
  }
  print("min: <%d><%d> Max:<%d><%d>\n", w->r.min.x, w->r.min.y, w->r.max.x, w->r.max.y);
  Widget *l = createLabel("label1", Rect(0, 0, 30, 50));
  if (l==nil){
    sysfatal("Error l");
  }
  Label *aux = l->w;
  aux->setText(aux, "This is a label!!");
  aux->border=1;
  w->addWidget(w, l, Pt(30,30));
  print("min: <%d><%d> Max:<%d><%d>\n", l->r.min.x, l->r.min.y, l->r.max.x, l->r.max.y);
  print("Point: <%d><%d>", l->p.x, l->p.y);

  startjayapp(w);
  threadexits(nil);
}
