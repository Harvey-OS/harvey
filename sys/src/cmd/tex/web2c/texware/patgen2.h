void initialize();
ASCIIcode zgetASCII();
#define getASCII(c) zgetASCII((textchar) (c))
void initpatterntrie();
triepointer firstfit();
void zunpack();
#define unpack(s) zunpack((triepointer) (s))
optype znewtrieop();
#define newtrieop(v, d, n) znewtrieop((valtype) (v), (dottype) (d), (optype) (n))
void zinsertpattern();
#define insertpattern(val, dot) zinsertpattern((valtype) (val), (dottype) (dot))
void initcounttrie();
triecpointer firstcfit();
void zunpackc();
#define unpackc(b) zunpackc((triecpointer) (b))
triecpointer zinsertcpat();
#define insertcpat(fpos) zinsertcpat((wordindex) (fpos))
void readtranslate();
void zfindletters();
#define findletters(b, i) zfindletters((triepointer) (b), (dottype) (i))
void ztraversecounttrie();
#define traversecounttrie(b, i) ztraversecounttrie((triecpointer) (b), (dottype) (i))
void collectcounttrie();
triepointer zdeletepatterns();
#define deletepatterns(s) zdeletepatterns((triepointer) (s))
void deletebadpatterns();
void zoutputpatterns();
#define outputpatterns(s, patlen) zoutputpatterns((triepointer) (s), (dottype) (patlen))
void readword();
void hyphenate();
void changedots();
void outputhyphenatedword();
void doword();
void dodictionary();
void readpatterns();
