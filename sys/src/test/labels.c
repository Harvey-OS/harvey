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

  Widget *l = createLabel("label1", 20, 50);
  Widget *l2 = createLabel("label2", 20, 70);

  Label *aux = l->w;
  aux->setText(aux, "This is a label!!");
  aux->border=1;
  w->addWidget(w, l, Pt(30,30));
  print("min: <%d><%d> Max:<%d><%d>\n", l->r.min.x, l->r.min.y, l->r.max.x, l->r.max.y);
  print("Point: <%d><%d>\n", l->p.x, l->p.y);

  aux = l2->w;
  aux->setText(aux, "Other label");
  aux->border=1;
  aux->d3=1;
  w->addWidget(w, l, Pt(60,60));

  startjayapp(w);
  threadexits(nil);
}
