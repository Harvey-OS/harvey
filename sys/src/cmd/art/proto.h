drawcurpt(void);	/* curpt.c */
Point D2P(Dpoint d);	/* geometry.c */
Flt Mangle(void);	/* alcmd.c */
Flt Mdist(void);	/* alcmd.c */
void Mgrid(void);	/* grid.c */
Flt Mslope(void);	/* alcmd.c */
void Oalign(int button, Flt (*measure)(void), int kind, ...);	/* alcmd.c */
void Oanchor(void);	/* command.c */
void Oangle(int n);	/* alcmd.c */
void Oarc(void);	/* edit.c */
void Obox(void);	/* edit.c */
void Ocirc(int n);	/* alcmd.c */
void Ocircle(void);	/* edit.c */
void Oclosegrp(void);	/* command.c */
void Ocoolall(void);	/* align.c */
void Ocopy(void);	/* edit.c */
void Odelete(void);	/* command.c */
void Oedit(void);	/* edit.c */
void Oexit(void);	/* command.c */
void Oflattengrp(void);	/* command.c */
void Ogravity(int n);	/* command.c */
void Ogrid(int n);	/* grid.c */
void Ogroup(void);	/* edit.c */
void Oheat(void);	/* align.c */
void Oheating(int n);	/* command.c */
void Oline(void);	/* edit.c */
void Oopengrp(void);	/* command.c */
void Oparallel(int n);	/* alcmd.c */
void Oselall(void);	/* command.c */
void Oslope(int n);	/* alcmd.c */
void Ospline(void);	/* edit.c */
Dpoint P2D(Point p);	/* geometry.c */
void activate(Item *ip);	/* align.c */
void activatearc(Item *ip);	/* arc.c */
void activatebox(Item *ip);	/* box.c */
void activatecircle(Item *ip);	/* circle.c */
void activategroup(Item *ip);	/* group.c */
void activatehead(Item *ip);	/* head.c */
void activateline(Item *ip);	/* line.c */
void activateprim(Item *ip, Item *op);	/* align.c */
void activatespline(Item *ip);	/* spline.c */
void activatetext(Item *ip);	/* text.c */
Item *addarc(Item *head, Dpoint p0, Dpoint p1, Dpoint p2);	/* arc.c */
Item *addbox(Item *head, Dpoint p0, Dpoint p1);	/* box.c */
Item *addcircle(Item *head, Dpoint p0, Flt r);	/* circle.c */
Item *addgroup(Item *head, int group, Dpoint offs);	/* group.c */
Item *addhead(void);	/* head.c */
Item *additem(Item *head, int type, Flt r, Typeface *face, char *text, int group, Itemfns *fn, int np, ...);	/* storage.c */
Item *additemv(Item *head, int type, Flt r, Typeface *face, char *text, int group, Itemfns *fn, int np, Dpoint *p, Dpoint offs);	/* storage.c */
Item *addline(Item *head, Dpoint p0, Dpoint p1);	/* line.c */
Item *addspline(Item *head, Dpoint p0, Dpoint p1);	/* spline.c */
Item *addtext(Item *head, Dpoint p, Typeface *face, char *text);	/* text.c */
void ainit(void);	/* alcmd.c */
void alcirc(Dpoint p, Flt r);	/* align.c */
void alline(Dpoint p, Flt t);	/* align.c */
Flt angle(Dpoint p, Dpoint q);	/* geometry.c */
int arcinterarc(Dpoint a0, Dpoint a1, Dpoint a2, Dpoint b0, Dpoint b1, Dpoint b2, Dpoint *i);	/* geometry.c */
void argument(void);	/* command.c */
Drectangle bboxarc(Item *ip);	/* arc.c */
Drectangle bboxbezier(Bezier b);	/* bezier.c */
Drectangle bboxbox(Item *ip);	/* box.c */
Drectangle bboxcircle(Item *ip);	/* circle.c */
Drectangle bboxgroup(Item *ip);	/* group.c */
Drectangle bboxhead(Item *ip);	/* head.c */
Drectangle bboxline(Item *ip);	/* line.c */
Drectangle bboxspline(Item *ip);	/* spline.c */
Drectangle bboxtext(Item *ip);	/* text.c */
int bezierinterbezier(Bezier b1, Bezier b2, Dpoint *i, int level);	/* bezier.c */
void bsplit(Bezier b, Bezier l, Bezier r);	/* bezier.c */
int bstraight(Bezier b);	/* bezier.c */
void bug(char *fmt, ...);	/* debug.c */
int button(int n);	/* input.c */
int button1(void);	/* input.c */
int button123(void);	/* input.c */
int button2(void);	/* input.c */
int button23(void);	/* input.c */
int button3(void);	/* input.c */
int circinterarc(Dpoint cen, Flt r, Dpoint a0, Dpoint a1, Dpoint a2, Dpoint *i);	/* geometry.c */
int circintercirc(Dpoint c0, Flt r0, Dpoint c1, Flt r1, Dpoint *i);	/* geometry.c */
Dpoint circumcenter(Dpoint p, Dpoint q, Dpoint r);	/* geometry.c */
Dpoint closevert(Bezier b, Dpoint testp);	/* bezier.c */
void clrb(Bitmap *dst, Rectangle r, int color);	/* text.c */
void command(void);	/* command.c */
int confirm(char *action);	/* command.c */
void cool(Item *a);	/* align.c */
Dpoint dadd(Dpoint p, Dpoint q);	/* geometry.c */
Flt datan2(Flt x, Flt y);	/* geometry.c */
int dclipline(Drectangle r, Dpoint *p0, Dpoint *p1);	/* align.c */
Dpoint dcomb2(Dpoint p, Flt wp, Dpoint q, Flt wq);	/* geometry.c */
Dpoint dcomb3(Dpoint p, Flt wp, Dpoint q, Flt wq, Dpoint r, Flt wr);	/* geometry.c */
void deletearc(Item *ip);	/* arc.c */
void deletebox(Item *ip);	/* box.c */
void deletecircle(Item *ip);	/* circle.c */
void deletegroup(Item *p);	/* group.c */
void deletehead(Item *p);	/* head.c */
void deleteline(Item *ip);	/* line.c */
void deletespline(Item *ip);	/* spline.c */
void deletetext(Item *ip);	/* text.c */
int deqpt(Dpoint p, Dpoint q);	/* geometry.c */
Flt dist(Dpoint p, Dpoint q);	/* geometry.c */
Flt dlen(Dpoint p);	/* geometry.c */
Dpoint dlerp(Dpoint p, Dpoint dp, Flt alpha);	/* geometry.c */
Dpoint dmidpt(Dpoint p, Dpoint q);	/* geometry.c */
Dpoint dmul(Dpoint p, Flt m);	/* geometry.c */
int dptinrect(Dpoint p, Drectangle r);	/* geometry.c */
Drectangle draddp(Drectangle r, Dpoint p);	/* geometry.c */
void draw(Item *ip, Dpoint offs, int bits, Fcode c);	/* draw.c */
void drawarc(Item *ip, int bits, Bitmap *b, Fcode c);	/* arc.c */
void drawbezier(Bezier p, int bits, Bitmap *b, Fcode c);	/* bezier.c */
void drawbox(Item *ip, int bits, Bitmap *b, Fcode c);	/* box.c */
void drawcircle(Item *ip, int bits, Bitmap *b, Fcode c);	/* circle.c */
void drawgrid(void);	/* grid.c */
void drawgroup(Item *ip, int bits, Bitmap *b, Fcode c);	/* group.c */
void drawhead(Item *ip, int bits, Bitmap *b, Fcode c);	/* head.c */
void drawline(Item *ip, int bits, Bitmap *b, Fcode c);	/* line.c */
Rectangle drawmenubar(void);	/* menubar.c */
void drawpart(Item *ip, int index, Dpoint offs);	/* spline.c */
void drawprim(Item *ip, Item *op);	/* draw.c */
void drawsel(void);	/* selection.c */
void drawselprim(Item *ip, Item *op);	/* selection.c */
void drawspline(Item *ip, int bits, Bitmap *b, Fcode c);	/* spline.c */
void drawtext(Item *ip, int bits, Bitmap *b, Fcode c);	/* text.c */
Drectangle drcanon(Drectangle r);	/* geometry.c */
int drectxrect(Drectangle r1, Drectangle r2);	/* geometry.c */
Dpoint dreflect(Dpoint p, Dpoint p0, Dpoint p1);	/* geometry.c */
Flt dsq(Dpoint p, Dpoint q);	/* bezier.c */
Dpoint dsub(Dpoint p, Dpoint q);	/* geometry.c */
Drectangle dunion(Drectangle r, Drectangle s);	/* geometry.c */
void echo(char *m);	/* msg.c */
void editarc(void);	/* arc.c */
void editbox(void);	/* box.c */
void editcircle(void);	/* circle.c */
void editgroup(void);	/* group.c */
void edithead(void);	/* head.c */
void editline(void);	/* line.c */
void editspline(void);	/* spline.c */
void edittext(void);	/* text.c */
void ereshaped(Rectangle r);	/* main.c */
void fatal(char *m);	/* msg.c */
int findcircle(Item *ip, Dpoint p, Flt r);	/* align.c */
int findline(Item *ip, Dpoint p, Dpoint q);	/* align.c */
void flatten(Item *ip, Item *op);	/* align.c */
void getarg(void);	/* command.c */
void getmouse(void);	/* input.c */
Dpoint getpt(Biobuf *f);	/* fileio.c */
char *getstring(Biobuf *f, int end);	/* fileio.c */
int goodgroup(int n);	/* group.c */
void heat(Item *a);	/* align.c */
int hitmenubar(void);	/* menubar.c */
void hotarc(Dpoint p, Dpoint q, Dpoint r);	/* align.c */
void hotline(Dpoint p, Dpoint q, int ep);	/* align.c */
void hotpoint(Dpoint p);	/* align.c */
int idchar(int c);	/* command.c */
int inboxarc(Item *ip, Drectangle r);	/* arc.c */
int inboxbezier(Bezier b, Drectangle box, int level);	/* bezier.c */
int inboxbox(Item *ip, Drectangle r);	/* box.c */
int inboxcircle(Item *ip, Drectangle r);	/* circle.c */
int inboxgroup(Item *ip, Drectangle r);	/* group.c */
int inboxhead(Item *ip, Drectangle r);	/* head.c */
int inboxline(Item *ip, Drectangle r);	/* line.c */
void inboxprim(Item *ip, Item *op);	/* edit.c */
int inboxspline(Item *ip, Drectangle r);	/* spline.c */
int inboxtext(Item *ip, Drectangle r);	/* text.c */
void input(char *name);	/* fileio.c */
int itemxitem(Item *p, Item *q, Dpoint *i);	/* align.c */
void main(int argc, char *argv[]);	/* main.c */
void mapgrps(Item *ip);	/* fileio.c */
void marks(void);	/* draw.c */
int mbarhit(Mouse *m);	/* menubar.c */
Rectangle mbarinit(char **menubar[]);	/* menubar.c */
int morespline(Item *ip, Dpoint p);	/* spline.c */
void moveall(Item *ip, int i, Dpoint p);	/* edit.c */
void movenone(Item *ip, int index, Dpoint p);	/* curpt.c */
void movep(Item *ip, int i, Dpoint p);	/* edit.c */
void mover(Item *ip, int i, Dpoint p);	/* edit.c */
void movespline(Item *ip, int i, Dpoint p);	/* spline.c */
void movex0(Item *ip, int i, Dpoint p);	/* edit.c */
void movex0y1(Item *ip, int i, Dpoint p);	/* edit.c */
void movex1(Item *ip, int i, Dpoint p);	/* edit.c */
void movex1y0(Item *ip, int i, Dpoint p);	/* edit.c */
void movey0(Item *ip, int i, Dpoint p);	/* edit.c */
void movey1(Item *ip, int i, Dpoint p);	/* edit.c */
void msg(char *fmt, ...);	/* msg.c */
void mvcurpt(Dpoint testp);	/* curpt.c */
void mwalk(Item *p, Dpoint tx, void (*f)(Item *, Item *));	/* walk.c */
Dpoint neararc(Item *ip, Dpoint testp);	/* arc.c */
Dpoint nearbezier(Bezier b, Dpoint p, Dpoint close, int level);	/* bezier.c */
Dpoint nearbox(Item *ip, Dpoint testp);	/* box.c */
Dpoint nearcircle(Item *ip, Dpoint testp);	/* circle.c */
Dpoint neargrid(Dpoint testp);	/* grid.c */
Dpoint neargroup(Item *ip, Dpoint testp);	/* group.c */
Dpoint nearhead(Item *ip, Dpoint testp);	/* head.c */
Dpoint nearline(Item *ip, Dpoint testp);	/* line.c */
Dpoint nearseg(Dpoint p0, Dpoint p1, Dpoint testp);	/* geometry.c */
Dpoint nearspline(Item *ip, Dpoint testp);	/* spline.c */
Dpoint neartext(Item *ip, Dpoint testp);	/* text.c */
Dpoint nearvertarc(Item *ip, Dpoint testp);	/* arc.c */
Dpoint nearvertbox(Item *ip, Dpoint testp);	/* box.c */
Dpoint nearvertcircle(Item *ip, Dpoint testp);	/* circle.c */
Dpoint nearvertgroup(Item *ip, Dpoint testp);	/* group.c */
Dpoint nearverthead(Item *ip, Dpoint testp);	/* head.c */
Dpoint nearvertline(Item *ip, Dpoint testp);	/* line.c */
Dpoint nearvertspline(Item *ip, Dpoint testp);	/* spline.c */
Dpoint nearverttext(Item *ip, Dpoint testp);	/* text.c */
void newgrid(int n);	/* grid.c */
void output(char *name);	/* fileio.c */
delete(Item *p);	/* storage.c */
Flt pldist(Dpoint p, Dpoint p0, Dpoint p1);	/* geometry.c */
void realign(void);	/* align.c */
void rectf(Bitmap *dp, Rectangle r, Fcode code);	/* output.c */
void redraw(void);	/* draw.c */
void reheat(void);	/* align.c */
void s2b(Item *ip, int i0, Bezier b);	/* spline.c */
int same(Flt a, Flt b);	/* geometry.c */
int seginterarc(Dpoint p0, Dpoint p1, Dpoint a0, Dpoint a1, Dpoint a2, Dpoint *i);	/* geometry.c */
int seginterbezier(Dpoint p0, Dpoint p1, Bezier b, Dpoint *i, int level);	/* bezier.c */
int segintercircle(Dpoint p0, Dpoint p1, Dpoint cen, Flt r, Dpoint *i);	/* geometry.c */
int seginterseg(Dpoint p0, Dpoint p1, Dpoint q0, Dpoint q1, Dpoint *i);	/* geometry.c */
int seginterspline(Dpoint p0, Dpoint p1, Item *s, Dpoint *i);	/* spline.c */
Item *select(void);	/* curpt.c */
void setbg(char *file);	/* draw.c */
void setface(char *s);	/* text.c */
void setselection(Item *p);	/* selection.c */
int splineinterspline(Item *s1, Item *s2, Dpoint *i);	/* spline.c */
void sweepbox(Item *ip, int i, Dpoint p);	/* edit.c */
void track(void (*rou)(Item *, int, Dpoint), int index, Item *ip);	/* curpt.c */
void translatearc(Item *ip, Dpoint delta);	/* arc.c */
void translatebox(Item *ip, Dpoint delta);	/* box.c */
void translatecircle(Item *ip, Dpoint delta);	/* circle.c */
void translategroup(Item *ip, Dpoint delta);	/* group.c */
void translatehead(Item *ip, Dpoint delta);	/* head.c */
void translateline(Item *ip, Dpoint delta);	/* line.c */
void translatespline(Item *ip, Dpoint delta);	/* spline.c */
void translatetext(Item *ip, Dpoint delta);	/* text.c */
Flt triarea(Dpoint p0, Dpoint p1, Dpoint p2);	/* geometry.c */
void typecmd(void);	/* command.c */
void typein(int c);	/* command.c */
int use1(char *why);	/* edit.c */
void walk(Item *p, Dpoint tx, void (*f)(Item *, Item *));	/* walk.c */
void walk1(Item *p, Dpoint tx, void (*f)(Item *, Item *), Item *top);	/* walk.c */
void writearc(Item *ip, int f);	/* arc.c */
void writebox(Item *ip, int f);	/* box.c */
void writecircle(Item *ip, int f);	/* circle.c */
void writegroup(Item *ip, int f);	/* group.c */
void writehead(Item *ip, int f);	/* head.c */
void writeline(Item *ip, int f);	/* line.c */
void writespline(Item *ip, int f);	/* spline.c */
void writetext(Item *ip, int f);	/* text.c */
