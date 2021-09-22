void parsearguments AA((void));
void initialize AA((void));
void jumpout AA((void));
void inputln AA((void));
void opengffile AA((void));
void opentfmfile AA((void));
void opendvifile AA((void));
void readtfmword AA((void));
integer getbyte AA((void));
integer gettwobytes AA((void));
integer getthreebytes AA((void));
integer signedquad AA((void));
void zreadfontinfo AA((integer f,scaled s));
#define readfontinfo(f, s) zreadfontinfo((integer) (f), (scaled) (s))
strnumber makestring AA((void));
void zfirststring AA((integer c));
#define firststring(c) zfirststring((integer) (c))
keywordcode interpretxxx AA((void));
scaled getyyy AA((void));
void skipnop AA((void));
void beginname AA((void));
boolean zmorename AA((ASCIIcode c));
#define morename(c) zmorename((ASCIIcode) (c))
void endname AA((void));
void zpackfilename AA((strnumber n,strnumber a,strnumber e));
#define packfilename(n, a, e) zpackfilename((strnumber) (n), (strnumber) (a), (strnumber) (e))
void startgf AA((void));
void zwritedvi AA((dviindex a,dviindex b));
#define writedvi(a, b) zwritedvi((dviindex) (a), (dviindex) (b))
void dviswap AA((void));
void zdvifour AA((integer x));
#define dvifour(x) zdvifour((integer) (x))
void zdvifontdef AA((internalfontnumber f));
#define dvifontdef(f) zdvifontdef((internalfontnumber) (f))
void loadfonts AA((void));
void ztypeset AA((eightbits c));
#define typeset(c) ztypeset((eightbits) (c))
void zdviscaled AA((real x));
#define dviscaled(x) zdviscaled((real) (x))
void zhbox AA((strnumber s,internalfontnumber f,boolean sendit));
#define hbox(s, f, sendit) zhbox((strnumber) (s), (internalfontnumber) (f), (boolean) (sendit))
void zslantcomplaint AA((real r));
#define slantcomplaint(r) zslantcomplaint((real) (r))
nodepointer getavail AA((void));
void znodeins AA((nodepointer p,nodepointer q));
#define nodeins(p, q) znodeins((nodepointer) (p), (nodepointer) (q))
boolean zoverlap AA((nodepointer p,nodepointer q));
#define overlap(p, q) zoverlap((nodepointer) (p), (nodepointer) (q))
nodepointer znearestdot AA((nodepointer p,scaled d0));
#define nearestdot(p, d0) znearestdot((nodepointer) (p), (scaled) (d0))
void zconvert AA((scaled x,scaled y));
#define convert(x, y) zconvert((scaled) (x), (scaled) (y))
void zdvigoto AA((scaled x,scaled y));
#define dvigoto(x, y) zdvigoto((scaled) (x), (scaled) (y))
void ztopcoords AA((nodepointer p));
#define topcoords(p) ztopcoords((nodepointer) (p))
void zbotcoords AA((nodepointer p));
#define botcoords(p) zbotcoords((nodepointer) (p))
void zrightcoords AA((nodepointer p));
#define rightcoords(p) zrightcoords((nodepointer) (p))
void zleftcoords AA((nodepointer p));
#define leftcoords(p) zleftcoords((nodepointer) (p))
boolean zplacelabel AA((nodepointer p));
#define placelabel(p) zplacelabel((nodepointer) (p))
void dopixels AA((void));
