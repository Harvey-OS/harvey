initacc(void);	/* tile.c */
void arccontour(double xc, double yc, double r, double t0, double t1);	/* set.c */
int chadd(char *s);	/* font.c */
int checkdesc(char *path);	/* font.c */
int chindex(char *s);	/* font.c */
char *chname(int n);	/* font.c */
void conv(FILE *fp);	/* dpic.c */
void devcntrl(FILE *fp);	/* dpic.c */
void diceedge(double x0, double y0, double x1, double y1);	/* tile.c */
void drawarc(int dx1, int dy1, int dx2, int dy2);	/* draw.c */
void drawcirc(int d);	/* draw.c */
void drawellip(int a, int b);	/* draw.c */
void drawline(int dx, int dy);	/* draw.c */
void drawspline(FILE *fp, int flag);	/* draw.c */
void dumpfont(int n);	/* font.c */
void dumpmount(int m);	/* font.c */
void elcontour(double xc, double yc, double xr, double yr, double sense);	/* set.c */
void *emalloc(int n);	/* misc.c */
void error(char *mesg, ...);	/* misc.c */
int findfont(char *path);	/* font.c */
void freefonts(void);	/* font.c */
void freetypeface(Typeface *f);	/* rdtypeface.c */
int getdesc(char *path);	/* font.c */
int getfont(char *path, Fonthd *fpos);	/* font.c */
int hash(char *s, int l);	/* font.c */
int in_olist(int num);	/* misc.c */
void interior(void);	/* tile.c */
void loadfont(int m, char *f, char *name);	/* dpic.c */
void main(int ac, char *av[]);	/* dpic.c */
int mounted(int m);	/* font.c */
int mountfont(char *path, int m);	/* font.c */
int onfont(int c, int m);	/* font.c */
void oput(int c);	/* dpic.c */
void out_list(char *str);	/* misc.c */
void put1(int c);	/* dpic.c */
Typeface *rdtypeface(char *name);	/* rdtypeface.c */
void release(void *ptr);	/* font.c */
int right(Edge *a, Edge *b);	/* tile.c */
void setarc(int x0, int y0, int x1, int y1, int ixc, int iyc);	/* set.c */
void setbg(char *c);	/* set.c */
void setborder(char *buf);	/* set.c */
void setchar(int c, int x, int y);	/* set.c */
void setclear(void);	/* set.c */
void setclrwin(char *buf);	/* set.c */
void setcolor(char *c);	/* set.c */
void setellipse(int xl, int yl, int wid, int hgt);	/* set.c */
void setfont(int m);	/* dpic.c */
void setline(int ix0, int iy0, int ix1, int iy1);	/* set.c */
void setpage(int pagenum);	/* set.c */
void setpicfile(char *buf);	/* set.c */
void setspline(int ix0, int iy0, int ix1, int iy1, int ix2, int iy2);	/* set.c */
Rgba str2color(char *c);	/* set.c */
int str_convert(char **str, int err);	/* misc.c */
void t_charht(int n);	/* dpic.c */
int t_font(char *s);	/* dpic.c */
void t_init(void);	/* dpic.c */
void t_page(int pg);	/* dpic.c */
void t_slant(int n);	/* dpic.c */
void tileedge(double x0, double y0, double x1, double y1);	/* tile.c */
void tileend(void);	/* tile.c */
void tilestart(void);	/* tile.c */
dicedd(double x0, double y0, double x1, double y1);	/* tile.c */
dicedu(double x0, double y0, double x1, double y1);	/* tile.c */
diceud(double x0, double y0, double x1, double y1);	/* tile.c */
diceuu(double x0, double y0, double x1, double y1);	/* tile.c */
acctop(float x0, float x1, int y, int wnum);	/* tile.c */
accumulate(float x0, float y0, float x1, float y1, int x, int y);	/* tile.c */
void xymove(int x, int y);	/* dpic.c */
addspan(int y, int x0, int x1);	/* tile.c */
