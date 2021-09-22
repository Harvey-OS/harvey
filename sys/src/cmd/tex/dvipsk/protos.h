#ifndef _PROTOS_H_

#define _PROTOS_H_

/* some types need declaration before being used */
struct nlink;
struct hps_link;
struct Char;
struct String;
struct tcd;

/* prototypes for functions from bbox.c */
extern int nscalewidth P2H(int a, int b);
extern void bbtfmload P1H(int n);
extern void bbspecial P3H(int h, int v, int nbytes);
extern void bbdopage P1H(void);
extern void findbb P1H(int bop);

/* prototypes for functions from color.c */
extern void colorcmdout P1H(char *s);
extern void initcolor P1H(void);
extern void background P1H(char *bkgrnd);
extern void resetcolorstack P2H(char *p, int outtops);
extern void bopcolor P1H(int outtops);
extern void pushcolor P2H(char *p, Boolean outtops);
extern void popcolor P1H(Boolean outtops);


/* prototypes for functions from dopage.c */
extern void dopage P1H(void);

/* prototypes for functions from dosection.c */
extern void dosection P2H(sectiontype *s, int c);
extern int InPageList P1H(int i);
extern void InstallPL P2H(int pslow, int pshigh);
extern int ParsePages P1H(char *s);

/* prototypes for functions from dospecial.c */
extern void specerror P1H(char *s);
extern void outbangspecials P1H(void);
extern int IsSame P2H(char *a, char *b);
extern char *GetKeyVal P2H(char *str, int *tno);
extern void predospecial P2H(int numbytes, Boolean scanning);
extern int maccess P1H(char *s);
extern void dospecial P1H(int numbytes);
extern void fil2ps P2H(char *task, char *iname);
extern float *bbdospecial P1H(int nbytes);

/* prototypes for functions from download.c */
/* void downchar P2H(struct *c, short cc); */
extern void makepsname P2H(char *s, int n);
extern void lfontout P1H(int n);
/* void download P2H(struct *p, int psfont); */
/* void downpsfont P2H(struct *p, struct *all); */
extern void dopsfont P1H(sectiontype *fs);

/* prototypes for functions from dpicheck.c */
extern void addsiz P1H(int rhsize);
extern void adddpi P1H(int hsize);
extern unsigned short dpicheck P1H(unsigned short dpi);

/* prototypes for functions from drawPS.c */
#ifdef TPIC
extern void setPenSize P1H(char *cp);
extern void addPath P1H(char *cp);
extern void arc P2H(char *cp, int invis);
extern void flushDashedPath P2H(int dotted, double inchesPerDash);
extern void flushPath P1H(int invis);
extern void flushDashed P2H(char *cp, int dotted);
extern void flushSpline P1H(char *cp);
extern void SetShade P1H(char *cp);
extern void shadeLast P1H(char *cp);
extern void whitenLast P1H(void);
extern void blackenLast P1H(void);
#endif /* TPIC */

/* prototypes for functions from dviinput.c */
extern void abortpage P1H(void);
extern short dvibyte P1H(void);
extern unsigned short twobytes P1H(void);
extern int threebytes P1H(void);
extern short signedbyte P1H(void);
extern short signedpair P1H(void);
extern int signedtrio P1H(void);
extern int signedquad P1H(void);
extern void skipover P1H(int i);

/* prototypes for functions from dvips.c */
extern void help P1H(int status);
extern void error_with_perror P2H(char *s, char *fname);
extern void error P1H(char *s);
extern void check_checksum P3H(unsigned int c1, unsigned int c2, const char *name);
extern char *mymalloc P1H(int n);
extern void morestrings P1H(void);
extern void checkstrings P1H(void);
extern void initialize P1H(void);
extern char *newstring P1H(char *s);
extern void newoutname P1H(void);
extern void *revlist P1H(void *p);
extern void queryargs P1H(void);

/* prototypes for functions from emspecial.c */
extern void emclear P1H(void);
extern struct empt *emptput P3H(short point, int x, int y);
extern struct empt *emptget P1H(short point);
extern float emunits P2H(float width, char *unit);
extern void emspecial P1H(char *p);
extern int readinteger P1H(FILE *f);
extern unsigned short readhalfword P1H(FILE *f);
/* int PCXreadhead P2H(FILE *pcxf, struct *pcxh); */
extern int PCXreadline P3H(FILE *pcxf, unsigned char *pcxbuf, unsigned int byteperline);
/* void PCXgetpalette P5H(FILE *pcxf, struct *pcxh, unsigned char *r, unsigned char *g, unsigned char *b); */
extern void PCXshowpicture(FILE *pcxf,int wide,int high,int bytes,int cp,int bp,unsigned char *r,unsigned char *g,unsigned char *b);
extern void imagehead P5H(char *filename, int wide, int high, float emwidth, float emheight);
extern void imagetail P1H(void);
extern void pcxgraph P4H(FILE *pcxf, char *filename, float emwidth, float emheight);
extern void MSP_2_ps P3H(FILE *f, int wide, int high);
extern void MSP_1_ps P3H(FILE *f, int wide, int high);
extern void mspgraph P4H(FILE *f, char *filename, float emwidth, float emheight);
extern void rgbread P4H(FILE *f, int w, int b, char *s);
extern void rle4read P4H(FILE *f, int w, int b, char *s);
extern void rle8read P4H(FILE *f, int w, int b, char *s);
extern void bmpgraph P4H(FILE *f, char *filename, float emwidth, float emheight);
extern void emgraph P3H(char *filename, float emwidth, float emheight);

/* prototypes for functions from finclude.c */
extern fontdesctype *ifontdef P5H(char *name, char *area, int scsize, int dssize, char *scname);
extern void setfamily P1H(fontdesctype *f);
extern char *getname P1H(char *s);
extern void includechars P2H(fontdesctype *f, char *s);
extern void scan1fontcomment P1H(char *p);
extern int scanvm P1H(char *p);
extern void scan_fontnames P2H(char *str, char *psfile);
extern void scanfontusage P2H(char *p, char *psfile);
extern void scanfontcomments P1H(char *filename);
extern Boolean okascmd P1H(char *ss);
extern void nameout P2H(char *area, char *name);
extern void fonttableout P1H(void);

/* prototypes for functions from flib.c */
#ifdef FONTLIB
extern void fliload P1H(void);
#endif

/* prototypes for functions from fontdef.c */
extern fontdesctype *newfontdesc P5H(int cksum, int scsize, int dssize, char *name, char *area);
extern fontdesctype *matchfont P4H(char *name, char *area, int scsize, char *scname);
extern void fontdef P1H(int siz);
extern int skipnop P1H(void);

/* prototypes for functions from header.c */
extern int add_name P2H(char *s, struct header_list **what );
extern void checkhmem P1H(char *s);
extern int add_header P1H(char *s);
extern char *get_name P1H(struct header_list **what );
extern void send_headers P1H(void);

/* prototypes for functions from hps.c */
#ifdef HPS
extern void do_html P1H(char *s);
extern int href_or_name P1H(void);
extern int parseref P1H(void);
extern int get_string P1H(void);
extern int do_link P2H(char *s, int type);
extern unsigned int hash_string P1H(char *s);
extern struct nlist *lookup_link P2H(char *s, int type);
extern struct nlist *install_link P3H(char *name, struct hps_link *defn, int type);
extern struct hps_link *link_dup P1H(struct hps_link *s);
extern double dvi_to_hps_conv P2H(int i, int dir);
extern int vert_loc P1H(int i);
extern struct hps_link *dest_link P1H(char *s);
extern int count_targets P1H(void);
extern void do_targets P1H(void);
extern void do_target_dict P1H(void);
extern int href_name_match P2H(char *h, char *n);
extern void stamp_hps P1H(struct hps_link *pl);
extern void stamp_external P2H(char *s, struct hps_link *pl);
extern void finish_hps P1H(void);
extern void set_bitfile P2H(char *s, int mode);
extern void vertical_in_hps P1H(void);
extern void print_rect_list P1H(void);
extern void end_current_box P1H(void);
extern void start_new_box P1H(void);
#endif /* HPS */

/* prototypes for functions from loadfont.c */
extern void badpk P1H(char *s);
extern short pkbyte P1H(void);
extern int pkquad P1H(void);
extern int pktrio P1H(void);
extern void lectureuser P1H(void);
extern Boolean pkopen P1H(fontdesctype *fd);
extern void loadfont P1H(fontdesctype *curfnt);

/* prototypes for functions from makefont.c */
extern void makefont P3H(char *name, int dpi, int bdpi);

/* prototypes for functions from output.c */
extern void copyfile P1H(char *s);
extern void figcopyfile P2H(char *s, int systemtype);
extern void specialout P1H(char c);
extern void stringend P1H(void);
#ifdef SHIFTLOWCHARS
extern int T1Char P1H(int c);
#endif
extern void scout P1H(char c);
extern void cmdout P1H(char *s);
extern void floatout P1H(float n);
extern void doubleout P1H(double n);
extern void numout P1H(int n);
extern void mhexout P2H(void *p, long len);
extern void fontout P1H(int n);
extern void hvpos P1H(void);
extern void newline P1H(void);
extern void nlcmdout P1H(char *s);
extern int mlower P1H(int c);
extern int ncstrcmp P2H(char *a, char *b);
extern void findpapersize P1H(void);
extern void paperspec P2H(char *s, int hed);
extern char *epsftest P1H(int bop);
extern void open_output P1H(void);
extern void initprinter P1H(sectiontype *sect);
extern void setup P1H(void);
extern void cleanprinter P1H(void);
extern void psflush P1H(void);
extern void pageinit P1H(void);
extern void pageend P1H(void);
extern void drawrule P2H(int rw, int rh);

/* void drawchar P2H(struct *c, int cc); */
extern void tell_needed_fonts P1H(void);

/* prototypes for functions from papersiz.c */
extern long myatodim P1H(char **s );
extern void handlepapersize P3H(char *p, int *x, int *y);

/* prototypes for functions from pprescan.c */
extern void pprescanpages P1H(void);

/* prototypes for functions from prescan.c */
extern void readpreamble P1H(void);
extern void prescanpages P1H(void);

/* prototypes for functions from repack.c */
extern void was_putlong P2H(char *a, long i);
extern long getlong P1H(unsigned char *a);
extern void dochar P3H(unsigned char *from, short width, short height);
extern char *makecopy P3H(unsigned char *what, long len, unsigned char *p);
extern void repack P1H(struct tcd *cp);

/* prototypes for functions from resident.c */
extern int hash P1H(char *s);
extern void revpslists P1H(void);
extern void cleanres P1H(void);
extern struct resfont *lookup P1H(char *name);
extern struct resfont *findPSname P1H(char *name);
extern void add_entry P6H(char *TeXname, char *PSname, char *Fontfile, char *Vectfile, char *specinfo, char *downloadinfo);
extern int residentfont P1H(fontdesctype *curfnt);
extern void bad_config P1H(char *err);
extern char *configstring P2H(char *s, int nullok);
extern Boolean getdefaults P1H(char *s);
extern void getpsinfo P1H(char *name);
extern void checkenv P1H(int which);

/* prototypes for functions from scalewidth.c */
extern int scalewidth P2H(int a, int b);

/* prototypes for functions from scanpage.c */
/* int prescanchar P1H(struct *cd); */
extern Boolean preselectfont P1H(fontdesctype *f);
extern short scanpage P1H(void);

/* prototypes for functions from search.c */
#ifdef KPATHSEA
extern FILE *search P3H(kpse_file_format_type format, char *file, char *mode);
extern FILE *pksearch P6H(kpse_file_format_type format, char *file, char *mode, unsigned short dpi, char **name_ret , int *dpi_ret);
#else /* !KPATSHEA */
extern FILE *search P3H(char *path, char *file, char *mode);
extern FILE *pksearch P6H(char *path, char *file, char *mode, unsigned short dpi, char **name_ret , int *dpi_ret);
#endif /* KPATHSEA */
extern FILE *my_real_fopen P2H(char *n, char *t);
extern int close_file P1H(FILE *f);

/* prototypes for functions from skippage.c */
extern void skippage P1H(void);

/* prototypes for functions from squeeze.c */
extern void specialout P1H(char c);
extern void strout P1H(char *s);
extern void cmdout P1H(char *s);

/* prototypes for functions from t1part.c */
extern int DefTypeFont P1H(unsigned char *name);
extern int GetZeroLine P1H(unsigned char *str);
extern int GetWord P1H(unsigned char *mem);
extern int GetToken P1H(void);
extern int GetNum P1H(void);
extern int PassToken P1H(void);
extern int PassString P1H(unsigned char flg);
extern void *getmem P1H(unsigned int size);
extern struct Char *AddChar P3H(struct Char *TmpChar, unsigned char *CharName, int num);
extern void AddStr P2H(unsigned char *name, int num);
extern void RevChar P1H(struct Char *TmpChar);
extern void OutChar P2H(struct Char *TmpChar, FILE *fout);
extern void Reverse P1H(struct String *TmpStr);
extern void OutStr P2H(struct String *TmpStr, FILE *fout);
extern void PrintChar P1H(struct Char *TmpChar);
extern int ClearB P1H(void);
extern int ChooseChar P2H(unsigned char *name, struct Char *TmpChar);
extern int FindSeac P1H(int num);
extern int FindCharW P2H(unsigned char *name, int length);
extern void ClearCW P1H(struct Char *ThisChar);
extern int WorkVect P1H(struct Char *TmpChar);
extern void UnDefineCharsW P1H(void);
extern struct Char *UnDefineChars P1H(struct Char *TmpChar);
extern void UnDefineStr P1H(void);
extern void ScanSubrs P1H(int i);
extern void ViewReturnCall P5H(int num_err, int top, int *pstack, int j, int depth);
extern int DeCodeStr P2H(int num, int numseac);
extern void ScanChars P1H(int i);
extern void LastLook P1H(void);
extern int FindKeyWord P2H(int First_Key, int lastkey);
extern int ScanBinary P1H(void);
extern unsigned char *itoasp P3H(int n, unsigned char *s, int len);
extern void SubstNum P1H(void);
extern unsigned long little4 P1H(unsigned char *buff);
extern unsigned char CDeCrypt P2H(unsigned char cipher, unsigned int *lcdr);
extern int EndOfEncoding P1H(int err_num);
extern void CorrectGrid P1H(void);
extern int CharEncoding P1H(void);
extern void FindEncoding P1H(void);
extern void CheckChoosing P1H(void);
extern void OutASCII P3H(FILE *fout, unsigned char *buff, unsigned long len);
extern void BinEDeCrypt P2H(unsigned char *buff, unsigned long len);
extern void HexEDeCrypt P1H(unsigned char *mem);
extern int PartialPFA P2H(FILE *fin, FILE *fout);
extern int PartialPFB P2H(FILE *fin, FILE *fout);
extern void OutHEX P1H(FILE *fout);
extern int Afm P1H(void);
extern int FontPart P3H(FILE *fout, unsigned char *fontfile, unsigned char *vectfile);
extern int LoadVector P2H(int num, struct Char *TmpChar);
extern int ChooseVect P1H(struct Char *tmpChar);
extern void ErrorOfScan P1H(int err);
extern void NameOfProgram P1H(void);

/* prototypes for functions from tfmload.c */
extern void tfmopen P1H(fontdesctype *fd);
extern short tfmbyte P1H(void);
extern unsigned short tfm16 P1H(void);
extern int tfm32 P1H(void);
extern int tfmload P1H(fontdesctype *curfnt);

/* prototypes for functions from unpack.c */
extern short getnyb P1H(void);
extern Boolean getbit P1H(void);
extern long pkpackednum P1H(void);
extern void flip P2H(char *s, long howmany);
extern long unpack P5H(unsigned char *pack, unsigned short *raster, unsigned short cwidth, unsigned short cheight, unsigned short cmd);

/* prototypes for functions from virtualfont.c */
extern void badvf P1H(char *s);
extern short vfbyte P1H(void);
extern int vfquad P1H(void);
extern int vftrio P1H(void);
extern Boolean vfopen P1H(fontdesctype *fd);
extern struct tft *vfontdef P2H(int s, int siz);
extern Boolean virtualfont P1H(fontdesctype *curfnt);

#endif
