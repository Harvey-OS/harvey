#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <jay.h>

void onHover(Widget *w){
  Label *l = w->w;
  if(w->hovered){
    l->border=3;
  } else {
    l->border=1;
  }
}

void onHover2(Widget *w){
  Label *l = w->w;
  if(w->hovered){
    l->up=1;
  } else {
    l->up=0;
  }
}

void
threadmain(int argc, char *argv[]) {
  Widget * w = initjayapp("labels");
  if (w==nil){
    sysfatal("Error initial panel");
  }

  Widget *l = createLabel("label1", -1, -1);
  Widget *l2 = createLabel("label2", 20, 100);

  Label *aux = l->w;
  aux->setText(aux, "This is a label!!");
  aux->border=1;
  l->hover=onHover;
  l->unhover=onHover;
  w->addWidget(w, l, Pt(30,30));

  aux = l2->w;
  aux->setText(aux, "Other label");
  aux->border=1;
  aux->d3=1;
  l2->hover = onHover2;
  l2->unhover = onHover2;
  w->addWidget(w, l2, Pt(30,60));

  startjayapp(w);
  threadexits(nil);
}
