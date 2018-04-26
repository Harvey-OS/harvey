/*
 * labels: little program to test libjay
 */

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <jay.h>

void onHover_L1(Widget *w);
void onClick_L1(Widget *w, Mouse *m);

void onHover_L2(Widget *w);
void onClick_L2(Widget *w, Mouse *m);
void onDClick_L2(Widget *w, Mouse *m);

void onClick_L3(Widget *w, Mouse *m);

void onClick_L4(Widget *w, Mouse *m);
void onChange_L4(Widget *w);

void onPress(Widget *w, Mouse *m);
void onRelease(Widget *w, Mouse *m);

void
threadmain(int argc, char *argv[]) {
  Widget * w = initjayapp("labels");
  if (w==nil){
    sysfatal("Error initial panel");
  }

  Widget *l = createLabel("label1", -1, -1);
  Widget *l2 = createLabel("label2", 20, 120);
  Widget *l3 = createLabel("label3", 20, 200);
  Widget *l4 = createLabel("label4", 20, 270);

  Label *aux = l->w;
  aux->setText(aux, "This is a label with autosize");
  aux->border=createBorder(1, 0, 0);
  l->hover=onHover_L1;
  l->unhover=onHover_L1;
  l->click=onClick_L1;
  w->addWidget(w, l, Pt(30,30));

  aux = l2->w;
  aux->setText(aux, "Click me");
  aux->border=createBorder(1, 1, 0);
  l2->click=onClick_L2;
  l2->dclick=onDClick_L2;
  l2->hover = onHover_L2;
  l2->unhover = onHover_L2;
  w->addWidget(w, l2, Pt(30,60));

  aux = l3->w;
  aux->setText(aux, "Click on me to dissapear");
  l3->click=onClick_L3;
  w->addWidget(w, l3, Pt(30, 90));

  aux = l4->w;
  aux->setText(aux, "Click to change text and color");
  l4->change=onChange_L4;
  l4->click=onClick_L4;
  w->addWidget(w, l4, Pt(30, 120));

  startjayapp(w);
  threadexits(nil);
}


void onHover_L1(Widget *w){
  Label *l = w->w;
  if(w->hovered){
    l->border.size=3;
  } else {
    l->border.size=1;
  }
}

void onHover_L2(Widget *w){
  Label *l = w->w;
  if(w->hovered){
    l->border.up=1;
  } else {
    l->border.up=0;
  }
}

void onClick_L1(Widget *w, Mouse *m){
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

void onClick_L2(Widget *w, Mouse *m){
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

void onClick_L4(Widget *w, Mouse *m){
  static int i = 1;
  Label *l = w->w;
  if (i){
    l->setText(l, "Click me to change text and color");
    i=0;
  } else {
    l->setText(l, "Click to change text and color!");
    i=1;
  }
}

void onChange_L4(Widget *w){
  static int i=1;
  Label *l = w->w;
  if (i) {
    l->textColor=DRed;
    i=0;
  } else {
    l->textColor=DBlack;
    i=1;
  }
}

void onClick_L3(Widget *w, Mouse *m){
  w->setVisible(w, 0);
}

void onDClick_L2(Widget *w, Mouse *m){
  Label *l = w->w;
  l->setText(l, "Double click");
}

void onPress(Widget *w, Mouse *m){
  Label *l = w->w;
  l->border.size=0;
}

void onRelease(Widget *w, Mouse *m){
  Label *l = w->w;
  l->border.size=1;
}
