void initialize();
void jumpout();
void inputln();
void opengffile();
void opentfmfile();
void opendvifile();
void readtfmword();
integer getbyte();
integer gettwobytes();
integer getthreebytes();
integer signedquad();
void zreadfontinfo();
#define readfontinfo(f, s) zreadfontinfo((integer) (f), (scaled) (s))
strnumber makestring();
void zfirststring();
#define firststring(c) zfirststring((integer) (c))
keywordcode interpretxxx();
scaled getyyy();
void skipnop();
void beginname();
boolean zmorename();
#define morename(c) zmorename((ASCIIcode) (c))
void endname();
void zpackfilename();
#define packfilename(n, a, e) zpackfilename((strnumber) (n), (strnumber) (a), (strnumber) (e))
void startgf();
void zwritedvi();
#define writedvi(a, b) zwritedvi((dviindex) (a), (dviindex) (b))
void dviswap();
void zdvifour();
#define dvifour(x) zdvifour((integer) (x))
void zdvifontdef();
#define dvifontdef(f) zdvifontdef((internalfontnumber) (f))
void loadfonts();
void ztypeset();
#define typeset(c) ztypeset((eightbits) (c))
void zdviscaled();
#define dviscaled(x) zdviscaled((real) (x))
void zhbox();
#define hbox(s, f, sendit) zhbox((strnumber) (s), (internalfontnumber) (f), (boolean) (sendit))
void zslantcomplaint();
#define slantcomplaint(r) zslantcomplaint((real) (r))
nodepointer getavail();
void znodeins();
#define nodeins(p, q) znodeins((nodepointer) (p), (nodepointer) (q))
boolean zoverlap();
#define overlap(p, q) zoverlap((nodepointer) (p), (nodepointer) (q))
nodepointer znearestdot();
#define nearestdot(p, d0) znearestdot((nodepointer) (p), (scaled) (d0))
void zconvert();
#define convert(x, y) zconvert((scaled) (x), (scaled) (y))
void zdvigoto();
#define dvigoto(x, y) zdvigoto((scaled) (x), (scaled) (y))
void ztopcoords();
#define topcoords(p) ztopcoords((nodepointer) (p))
void zbotcoords();
#define botcoords(p) zbotcoords((nodepointer) (p))
void zrightcoords();
#define rightcoords(p) zrightcoords((nodepointer) (p))
void zleftcoords();
#define leftcoords(p) zleftcoords((nodepointer) (p))
boolean zplacelabel();
#define placelabel(p) zplacelabel((nodepointer) (p))
void dopixels();
