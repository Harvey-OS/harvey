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

void onClickL1(Widget *w, Mouse *m){
  static int i = 1;
  Label *l = w->w;
  if (i){
    l->setText(l, "A large text in the label");
    i=0;
  } else {
    l->setText(l, "Short text");
    i=1;
  }
}

void onClick(Widget *w, Mouse *m){
  static int i = 1;
  Label *l = w->w;
  if (i){
    l->setText(l, "One click");
    i=0;
  } else {
    l->setText(l, "Just one click");
    i=1;
  }
}

void onClickDissapear(Widget *w, Mouse *m){
  w->setVisible(w, 0);
}

void onDClick(Widget *w, Mouse *m){
  Label *l = w->w;
  l->setText(l, "Double click");
}

void onPress(Widget *w, Mouse *m){
  Label *l = w->w;
  l->border=0;
}

void onRelease(Widget *w, Mouse *m){
  Label *l = w->w;
  l->border=1;
}

void
threadmain(int argc, char *argv[]) {
  Widget * w = initjayapp("labels");
  if (w==nil){
    sysfatal("Error initial panel");
  }

  Widget *l = createLabel("label1", -1, -1);
  Widget *l2 = createLabel("label2", 20, 120);
  Widget *l3 = createLabel("label3", 20, 200);

  Label *aux = l->w;
  aux->setText(aux, "This is a label with autosize");
  aux->border=1;
  l->hover=onHover;
  l->unhover=onHover;
  l->click=onClickL1;
  w->addWidget(w, l, Pt(30,30));

  aux = l2->w;
  aux->setText(aux, "Click me");
  aux->border=1;
  aux->d3=1;
  l2->click=onClick;
  l2->dclick=onDClick;
  l2->hover = onHover2;
  l2->unhover = onHover2;
  w->addWidget(w, l2, Pt(30,60));

  aux = l3->w;
  aux->setText(aux, "Click on me to dissapear");
  l3->click=onClickDissapear;
  w->addWidget(w, l3, Pt(30, 90));

  startjayapp(w);
  threadexits(nil);
}
