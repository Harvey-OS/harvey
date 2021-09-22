void parsearguments AA((void));
void initialize AA((void));
ASCIIcode zgetASCII AA((textchar c));
#define getASCII(c) zgetASCII((textchar) (c))
void initpatterntrie AA((void));
triepointer firstfit AA((void));
void zunpack AA((triepointer s));
#define unpack(s) zunpack((triepointer) (s))
optype znewtrieop AA((valtype v,dottype d,optype n));
#define newtrieop(v, d, n) znewtrieop((valtype) (v), (dottype) (d), (optype) (n))
void zinsertpattern AA((valtype val,dottype dot));
#define insertpattern(val, dot) zinsertpattern((valtype) (val), (dottype) (dot))
void initcounttrie AA((void));
triecpointer firstcfit AA((void));
void zunpackc AA((triecpointer b));
#define unpackc(b) zunpackc((triecpointer) (b))
triecpointer zinsertcpat AA((wordindex fpos));
#define insertcpat(fpos) zinsertcpat((wordindex) (fpos))
void readtranslate AA((void));
void zfindletters AA((triepointer b,dottype i));
#define findletters(b, i) zfindletters((triepointer) (b), (dottype) (i))
void ztraversecounttrie AA((triecpointer b,dottype i));
#define traversecounttrie(b, i) ztraversecounttrie((triecpointer) (b), (dottype) (i))
void collectcounttrie AA((void));
triepointer zdeletepatterns AA((triepointer s));
#define deletepatterns(s) zdeletepatterns((triepointer) (s))
void deletebadpatterns AA((void));
void zoutputpatterns AA((triepointer s,dottype patlen));
#define outputpatterns(s, patlen) zoutputpatterns((triepointer) (s), (dottype) (patlen))
void readword AA((void));
void hyphenate AA((void));
void changedots AA((void));
void outputhyphenatedword AA((void));
void doword AA((void));
void dodictionary AA((void));
void readpatterns AA((void));
