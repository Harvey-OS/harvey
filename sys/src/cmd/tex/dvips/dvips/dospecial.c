/*
 *   This routine handles special commands;
 *   predospecial() is for the prescan, dospecial() for the real thing.
 */
#include "structures.h" /* The copyright notice in that file is included too! */

#include <ctype.h>
extern int atoi();
extern void fil2ps();
extern FILE *search();
extern int system();
/*
 *   These are the external routines called:
 */
/**/
#ifdef TPIC
/*
 * Fri Mar  9 1990  jourdan@minos.inria.fr (MJ)
 * Upgraded to accommodate tpic release 2.0 extended output language.
 * Should prove upward compatible!
 */
extern void setPenSize();
extern void flushPath();
extern void flushDashed();
extern void flushDashed();
extern void addPath();
extern void arc();
extern void flushSpline();
extern void shadeLast();
extern void whitenLast();
extern void blackenLast();
extern void SetShade() ;
#endif
extern shalfword dvibyte() ;
extern int add_header() ;
extern void hvpos() ;
extern void figcopyfile() ;
extern void nlcmdout() ;
extern void cmdout() ;
extern void numout() ;
extern void scout() ;
extern void stringend() ;
extern void error() ;
extern void psflush() ;
/* IBM: color - begin */
extern void pushcolor() ;
extern void popcolor() ;
extern void resetcolorstack() ;
extern void background() ;
/* IBM: color - end */
extern char errbuf[] ;
extern shalfword linepos;
extern Boolean usesspecial ;
extern Boolean usescolor ;   /* IBM: color */
extern int landscape ;
extern char *paperfmt ;
extern char *nextstring;
extern char *maxstring;
extern char *oname;
extern FILE *bitfile;
extern int quiet;
extern fontdesctype *curfnt ;
extern int actualdpi ;
extern int vactualdpi ;
extern integer hh, vv;
extern int lastfont ;
extern real conv ;
extern real vconv ;
extern integer hpapersize, vpapersize ;
extern Boolean pprescan ;
extern char *figpath ;

#ifdef DEBUG
extern integer debug_flag;
#endif
extern void scanfontcomments() ;
extern void handlepapersize() ;

static int specialerrors = 20 ;

struct bangspecial {
   struct bangspecial *next ;
   char actualstuff[1] ; /* more space will actually be allocated */
} *bangspecials = NULL ;

void specerror(s)
char *s ;
{
   if (specialerrors > 0) {
      error(s) ;
      specialerrors-- ;
   } else if (specialerrors == 0) {
      error("more errors in special, being ignored . . .") ;
      specialerrors-- ;
   }
}

#ifdef EMTEX
/* subset of emtex specials */

#define EMMAX 1613 /* maximum number of emtex special points */
#define TRUE 1
#define FALSE 0

struct empt {
   shalfword point;
   integer x, y;
};

struct empt *empoints = NULL;
boolean emused = FALSE;  /* true if em points used on this page */
integer emx, emy;

struct emunit {
   char *unit;
   float factor;
};
struct emunit emtable[] = {
  {"pt",72.27},
  {"pc",72.27/12},
  {"in",1.0},
  {"bp",72.0},
  {"cm",2.54},
  {"mm",25.4},
  {"dd",72.27/(1238/1157)},
  {"cc",72.27/12/(1238/1157)},
  {"sp",72.27*65536},
  {"",0.0}
};


/* clear the empoints array if necessary */
void
emclear()
{
int i;
   if (emused && empoints)
      for (i=0; i<EMMAX; i++)
         empoints[i].point = 0;
   emused = FALSE ;
}

/* put an empoint into the empoints array */
struct empt *emptput(point, x, y)
shalfword point;
integer x, y;
{
int i, start;

   emused = TRUE;
   start = point % EMMAX;
   i = start;
   while ( empoints[i].point != 0 ) {
      if ( empoints[i].point == point )
         break;
      i++;
      if (i >= EMMAX)
         i = 0;
      if (i == start) {
	 sprintf(errbuf,"!Too many em: special points");
	 specerror(errbuf);
      }
   }

   empoints[i].point = point;
   empoints[i].x = x;
   empoints[i].y = y;
   return(&empoints[i]);
}

/* get an empoint from the empoints array */
struct empt *emptget(point)
shalfword point;
{
int i, start;

   start = point % EMMAX;
   i = start;
   if (emused == TRUE)
      while ( empoints[i].point != 0 ) {
         if (empoints[i].point == point)
            return(&empoints[i]);
         i++;
         if (i >= EMMAX)
            i = 0;
         if (i == start)
            break;
      }
   sprintf(errbuf,"!em: point %d not defined",point);
   specerror(errbuf);
   return(NULL); /* never returns due to error */
}


/* convert width into dpi units */
float emunits(width,unit)
float width;
char *unit;
{
struct emunit *p;
	for (p=emtable; *(p->unit)!='\0'; p++) {
	   if (strcmp(p->unit,unit)==0)
		return( width * actualdpi / p->factor );
	}
	return (-1.0); /* invalid unit */
}
#endif /* EMTEX */


static void trytobreakout(p)
register char *p ;
{
   register int i ;
   register int instring = 0 ;
   int lastc = 0 ;

   i = 0 ;
   (void)putc('\n', bitfile) ;
   while (*p) {
      if (i > 65 && *p == ' ' && instring == 0) {
         (void)putc('\n', bitfile) ;
         i = 0 ;
      } else {
         (void)putc(*p, bitfile) ;
         i++ ;
      }
      if (*p == '(' && lastc != '\\')
         instring = 1 ;
      else if (*p == ')' && lastc != '\\')
         instring = 0 ;
      lastc = *p ;
      p++ ;
   }
   (void)putc('\n', bitfile) ;
}

static void dobs(q)
register struct bangspecial *q ;
{
   if (q) {
      dobs(q->next) ;
      trytobreakout(q->actualstuff) ;
   }
}

void
outbangspecials() {
   if (bangspecials) {
      cmdout("TeXDict") ;
      cmdout("begin") ;
      cmdout("@defspecial\n") ;
      dobs(bangspecials) ;
      cmdout("\n@fedspecial") ;
      cmdout("end") ;
   }
}

/* We recommend that new specials be handled by the following general
 * (and extensible) scheme, in which the user specifies one or more
 * `key=value' pairs separated by spaces.
 * The known keys are given in KeyTab; they take values
 * of one of the following types:
 *
 * None: no value, just a keyword (in which case the = sign is omitted)
 * String: the value should be "<string without double-quotes"
 *                          or '<string without single-quotes'
 * Integer: the value should be a decimal integer (%d format)
 * Number: the value should be a decimal integer or real (%f format)
 * Dimension: like Number, but will be multiplied by the scaledsize
 *       of the current font and converted to default PostScript units
 * (Actually, strings are allowed in all cases; the delimiting quotes
 *  are simply stripped off if present.)
 *
 */

typedef enum {None, String, Integer, Number, Dimension} ValTyp;
typedef struct {
   char    *Entry;
   ValTyp  Type;
} KeyDesc;

#define NKEYS    (sizeof(KeyTab)/sizeof(KeyTab[0]))

KeyDesc KeyTab[] = {{"psfile",  String}, /* j==0 in the routine below */
                    {"ifffile", String}, /* j==1 */
                    {"tekfile", String}, /* j==2 */
                    {"hsize",   Number},
                    {"vsize",   Number},
                    {"hoffset", Number},
                    {"voffset", Number},
                    {"hscale",  Number},
                    {"vscale",  Number},
                    {"angle",   Number},
                    {"llx", Number},
                    {"lly", Number},
                    {"urx", Number},
                    {"ury", Number},
                    {"rwi", Number},
                    {"rhi", Number},
                    {"clip", None}};

#ifdef VMS
#define Tolower _tolower
#else
#ifdef VMCMS    /* IBM: VM/CMS */
#define Tolower __tolower
#else
/*
 * compare strings, ignore case
 */
char Tolower(c)
register char c ;
{
   if ('A' <= c && c <= 'Z')
      return(c+32) ;
   else
      return(c) ;
}
#endif  /* IBM: VM/CMS */
#endif
int IsSame(a, b)
char *a, *b;
{
   for( ; *a != '\0'; ) {
      if( Tolower(*a) != Tolower(*b) ) 
         return( 0 );
      a++ ;
      b++ ;
   }
   return( *b == '\0' );
}

char *KeyStr, *ValStr ; /* Key and String values found */
long ValInt ; /* Integer value found */
float ValNum ; /* Number or Dimension value found */

char  *GetKeyVal(str,tno) /* returns NULL if none found, else next scan point */
   char *str ; /* starting point for scan */
   int  *tno ; /* table entry number of keyword, or -1 if keyword not found */
{
   register char *s ;
   register int i ;
   register char t ;

   for (s=str; *s <= ' ' && *s; s++) ; /* skip over blanks */
   if (*s == '\0')
      return (NULL) ;
   KeyStr = s ;
   while (*s>' ' && *s!='=') s++ ;
   if (t = *s)
      *s++ = 0 ;

   for(i=0; i<NKEYS; i++)
      if( IsSame(KeyStr, KeyTab[i].Entry) )
         goto found ;
   *tno = -1;
   return (s) ;

found: *tno = i ;
   if (KeyTab[i].Type == None)
      return (s) ;

   if (t && t <= ' ') {
      for (; *s <= ' ' && *s; s++) ; /* now look for the value part */
      if ((t = *s)=='=')
         s++ ;
   }
   ValStr = "" ;
   if ( t == '=' ) {
      while (*s <= ' ' && *s)
         s++ ;
      if (*s=='\'' || *s=='\"')
         t = *s++ ;               /* get string delimiter */
      else t = ' ' ;
      ValStr = s ;
      while (*s!=t && *s)
         s++ ;
      if (*s)
         *s++ = 0 ;
   }
   switch (KeyTab[i].Type) {
 case Integer:
      if(sscanf(ValStr,"%ld",&ValInt)!=1) {
          sprintf(errbuf,"Non-integer value (%s) given for keyword %s",
              ValStr, KeyStr) ;
          specerror(errbuf) ;
          ValInt = 0 ;
      }
      break ;
 case Number:
 case Dimension:
      if(sscanf(ValStr,"%f",&ValNum)!=1) {  
          sprintf(errbuf,"Non-numeric value (%s) given for keyword %s",
              ValStr, KeyStr) ;
          specerror(errbuf) ;
          ValNum = 0.0 ;
      }
      if (KeyTab[i].Type==Dimension) {
         if (curfnt==NULL)
            error("! No font selected") ;
         ValNum = ValNum * ((double)curfnt->scaledsize) * conv * 72 / DPI ;
      }
      break ;
 default: break ;
   }
   return (s) ;
}

/*
 *   Now our routines.  We get the number of bytes specified and place them
 *   into the string buffer, and then parse it. Numerous conventions are
 *   supported here for historical reasons.
 */

void predospecial(numbytes, scanning)
integer numbytes ;
Boolean scanning ;
{
   register char *p = nextstring ;
   register int i = 0 ;
   int j ;

   if (nextstring + numbytes > maxstring)
      error("! out of string space in predospecial") ;
   for (i=numbytes; i>0; i--)
#ifdef VMCMS /* IBM: VM/CMS */
      *p++ = ascii2ebcdic[(char)dvibyte()] ;
#else
      *p++ = (char)dvibyte() ;
#endif /* IBM: VM/CMS */
   if (pprescan)
      return ;
   while (p[-1] <= ' ' && p > nextstring)
      p-- ; /* trim trailing blanks */
   if (p==nextstring) return ; /* all blank is no-op */
   *p = 0 ;
   p = nextstring ;
   while (*p <= ' ')
      p++ ;
#ifdef DEBUG
   if (dd(D_SPECIAL))
      (void)fprintf(stderr, "Preprocessing special: %s\n", p) ;
#endif

/*
 *   We use strncmp() here to also pass things like landscape()
 *   or landscape: or such.
 */

   if (strncmp(p, "landscape", 9)==0) {
      if (hpapersize || vpapersize)
         error(
             "both landscape and papersize specified:  ignoring landscape") ;
      else
         landscape = 1 ;
      return ;
   } else if (strncmp(p, "papersize", 9)==0) {
      p += 9 ;
      while (*p == '=' || *p == ' ')
         p++ ;
      if (hpapersize == 0 || vpapersize == 0) {
         if (landscape) {
            error(
             "both landscape and papersize specified:  ignoring landscape") ;
            landscape = 0 ;
         }
         handlepapersize(p, &hpapersize, &vpapersize) ;
      }
      return ;
   }
   if (strncmp(p, "xtex:", 5)==0) return ;
   usesspecial = 1 ;  /* now the special prolog will be sent */
   if (strncmp(p, "header", 6)==0) {
      char *q ;
      p += 6 ;
      while ((*p <= ' ' || *p == '=' || *p == '(') && *p != 0)
         p++ ;
      q = p ;  /* we will remove enclosing parentheses */
      p = p + strlen(p) - 1 ;
      while ((*p <= ' ' || *p == ')') && p >= q)
         p-- ;
      p[1] = 0 ;
      if (p >= q)
         (void)add_header(q) ;
   }
/* IBM: color - added section here for color header and color history */
   if (strncmp(p, "background", 10) == 0) {
      usescolor = 1 ;
      p +=11 ;
      while ( *p <= ' ' ) p++ ;
      background(p) ;
   }
   if (strncmp(p, "color", 5) == 0) {
      usescolor = 1 ;
      p += 6 ;
      while ( *p <= ' ' ) p++ ;
      if (strncmp(p, "push", 4) == 0 ) {
         p += 5 ;
         while ( *p <= ' ' ) p++ ;
         pushcolor(p, 0) ;
      } else if (strncmp(p, "pop", 3) == 0 ) {
         popcolor(0) ;
      } else {
         resetcolorstack(p,0) ;
      }
   }   /* IBM: color - end changes */
   else if (*p == '!') {
      register struct bangspecial *q ;
      p++ ;
      q = (struct bangspecial *)mymalloc((integer)
                         (sizeof(struct bangspecial) + strlen(p))) ;
      (void)strcpy(q->actualstuff, p) ;
      q->next = bangspecials ;
      bangspecials = q ;
   } else if (scanning && *p != '"' &&
          (p=GetKeyVal(p, &j)) != NULL && j==0)
      scanfontcomments(ValStr) ;
}

int maccess(s)
char *s ;
{
   FILE *f = search(figpath, s, "r") ;
   if (f)
      fclose(f) ;
   return (f != 0) ;
}

char *tasks[] = { 0, "iff2ps", "tek2ps" } ;

void dospecial(numbytes)
integer numbytes ;
{
   register char *p = nextstring ;
   register int i = 0 ;
   int j, systemtype = 0 ;
   char psfile[100] ; 
   char cmdbuf[100] ; 
   register char *q ;
   Boolean psfilewanted = 1 ;
   char *task = 0 ;
#ifdef EMTEX
/* specials for emtex */
float emwidth;
shalfword empoint1, empoint2;
struct empt *empoint;
char emunit[3];
char emstr[80];
char *emp;
#endif /* EMTEX */

   if (nextstring + i > maxstring)
      error("! out of string space in dospecial") ;
   for (i=numbytes; i>0; i--)
#ifdef VMCMS /* IBM: VM/CMS */
      *p++ = ascii2ebcdic[(char)dvibyte()] ;
#else
      *p++ = (char)dvibyte() ;
#endif  /* IBM: VM/CMS */
   while (p[-1] <= ' ' && p > nextstring)
      p-- ; /* trim trailing blanks */
   if (p==nextstring) return ; /* all blank is no-op */
   *p = 0 ;
   p = nextstring ;
   while (*p <= ' ')
      p++ ;
#ifdef DEBUG
   if (dd(D_SPECIAL))
      (void)fprintf(stderr, "Processing special: %s\n", p) ;
#endif

#ifdef EMTEX
/* specials for emtex, added by rjl */
/* at present,
 * the line cut parameter is not supported (and is ignored)
 * em:graph is not supported
 */ 
   if (strncmp(p, "em:", 3)==0) {
        hvpos() ;
	for (emp = p+3; *emp && isspace(*emp); emp++); /* skip blanks */
	if (strncmp(emp, "linewidth", 9) == 0) {
	   /* code for linewidth */
	   for (emp = emp+9; *emp && isspace(*emp); emp++); /* skip blanks */
	   sscanf(emp, "%f%2s", &emwidth, emunit);
	   emwidth = emunits(emwidth,emunit);
	   if (emwidth!=-1.0) {
	      sprintf(emstr,"%.1f setlinewidth", emwidth);
	      cmdout(emstr);
#ifdef DEBUG
   if (dd(D_SPECIAL))
      (void)fprintf(stderr, "em special: Linewidth set to %.1f dots\n", 
		emwidth) ;
#endif
	   } else {
	      sprintf(errbuf,"Unknown em: special width");
	      specerror(errbuf);
	   }
	}
        else if (strncmp(emp, "moveto", 6) == 0) {
#ifdef DEBUG
   if (dd(D_SPECIAL))
#ifdef SHORTINT
      (void)fprintf(stderr, "em special: moveto %ld,%ld\n", hh, vv);
#else
      (void)fprintf(stderr, "em special: moveto %d,%d\n", hh, vv);
#endif
#endif
           emx = hh;
           emy = vv;
        }
        else if (strncmp(emp, "lineto", 6) == 0) {
#ifdef DEBUG
   if (dd(D_SPECIAL))
#ifdef SHORTINT
      (void)fprintf(stderr, "em special: lineto %ld,%ld\n", hh, vv);
#else
      (void)fprintf(stderr, "em special: lineto %d,%d\n", hh, vv);
#endif
#endif
	   cmdout("np");
	   numout(emx);
	   numout(emy);
	   cmdout("a");
	   numout(hh);
	   numout(vv);
	   cmdout("li");
	   cmdout("st");
           emx = hh;
           emy = vv;
        }
	else if (strncmp(emp, "point", 5) == 0) {
           if (empoints == NULL) {
              empoints = 
              (struct empt *)mymalloc((integer)EMMAX * sizeof(struct empt)) ;
              emused = TRUE;
              emclear();
           }
	   for (emp = emp+5; *emp && isspace(*emp); emp++); /* skip blanks */
           empoint1 = (shalfword)atoi(emp);
           empoint = emptput(empoint1,hh,vv);
#ifdef DEBUG
   if (dd(D_SPECIAL))
#ifdef SHORTINT
      (void)fprintf(stderr, "em special: Point %d is %ld,%ld\n",
#else
      (void)fprintf(stderr, "em special: Point %d is %d,%d\n",
#endif
		empoint->point, empoint->x, empoint->y) ;
#endif
	}
	else if (strncmp(emp, "line", 4) == 0) {
	   for (emp = emp+4; *emp && isspace(*emp); emp++); /* skip blanks */
           empoint1 = (shalfword)atoi(emp);
	   for (; *emp && isdigit(*emp); emp++); /* skip point 1 */
	   if ( *emp && strchr("hvp",*emp)!=0 )
	      emp++;  /* skip line cut */
	   for (; *emp && isspace(*emp); emp++); /* skip blanks */
	   if ( *emp && ispunct(*emp) )
	      emp++; /*  skip comma separator */
	   for (; *emp && isspace(*emp); emp++); /* skip blanks */
           empoint2 = (shalfword)atoi(emp);
	   for (; *emp && isdigit(*emp); emp++); /* skip point 2 */
	   if ( *emp && strchr("hvp",*emp)!=0 )
	      emp++;  /* skip line cut */
	   for (; *emp && isspace(*emp); emp++); /* skip blanks */
	   if ( *emp && ispunct(*emp) )
	      emp++; /*  skip comma separator */
	   emwidth = -1.0;
	   emunit[0]='\0';
	   sscanf(emp, "%f%2s", &emwidth, emunit);
	   emwidth = emunits(emwidth,emunit);
#ifdef DEBUG
   if (dd(D_SPECIAL))
      (void)fprintf(stderr, "em special: Line from point %d to point %d\n",
		empoint1, empoint2) ;
#endif
	   cmdout("np");
	   if (emwidth!=-1.0) {
#ifdef DEBUG
   if (dd(D_SPECIAL))
   (void)fprintf(stderr,"em special: Linewidth temporarily set to %.1f dots\n", 
		emwidth) ;
#endif
	   	strcpy(emstr,"currentlinewidth");
	   	cmdout(emstr);
	        sprintf(emstr,"%.1f setlinewidth", emwidth);
	        cmdout(emstr);
	   }
           empoint = emptget(empoint1);
	   numout(empoint->x);
	   numout(empoint->y);
	   cmdout("a");
           empoint = emptget(empoint2);
	   numout(empoint->x);
	   numout(empoint->y);
	   cmdout("li");
	   cmdout("st");
	   if (emwidth!=-1.0) {
	   	strcpy(emstr,"setlinewidth");
	   	cmdout(emstr);
	   }
	}
	else if (strncmp(emp, "message", 7) == 0) {
           (void)fprintf(stderr, "em message: %s\n", emp+7) ;
	}
	else {
           sprintf(errbuf, 
	      "Unknown em: command (%s) in \\special will be ignored", p);
           specerror(errbuf) ;
	}
	return;
   }
#endif /* EMTEX */

   if (strncmp(p, "ps:", 3)==0) {
        psflush() ; /* now anything can happen. */
        if (p[3]==':') {
           if (strncmp(p+4, "[begin]", 7) == 0) {
              hvpos() ;
              trytobreakout(&p[11]);
           } else if (strncmp(p+4, "[end]", 5) == 0)
              trytobreakout(&p[9]);
           else trytobreakout(&p[4]);
        } else if (strncmp(p+3, " plotfile ", 10) == 0) {
             char *sfp ;
             hvpos() ;
             p += 13;
           /*
            *  Fixed to allow popen input for plotfile
            *  TJD 10/20/91
            */
           while (*p == ' ') p++;
           if (*p == '"') {
             p++;
             for (sfp = p; *sfp && *sfp != '"'; sfp++) ;
           } else {
             for (sfp = p; *sfp && *sfp != ' '; sfp++) ;
           }
           *sfp = '\0';
           if (*p == '`') 
             figcopyfile(p+1, 1);
           else
             figcopyfile (p, 0);
           /* End TJD changes */
        } else {
           hvpos() ;
           trytobreakout(&p[3]);
        }
        return;
   }
   if (strncmp(p, "landscape", 9)==0 || strncmp(p, "header", 6)==0 ||
       strncmp(p, "papersize", 9)==0 || *p=='!')
      return ; /* already handled in prescan */
/* IBM: color - begin changes */
   if ( strncmp(p, "background", 10) == 0 )
      return ; /* already handled in prescan */
   if (strncmp(p, "color", 5) == 0) {
      p += 6 ;
      while ( *p <= ' ' ) p++ ;
      if (strncmp(p, "push", 4) == 0 ) {
         p += 4 ;
         while ( *p <= ' ' ) p++ ;
         pushcolor(p,1);
      } else if (strncmp(p, "pop", 3) == 0 ) {
         popcolor(1) ;
      } else {
         resetcolorstack(p,1) ;
      }
      return ;
   } /* IBM: color - end changes*/
#ifdef TPIC
/* ordered as in tpic 2.0 documentation for ease of cross-referencing */
   if (strncmp(p, "pn ", 3) == 0) {setPenSize(p+2); return;}
   if (strncmp(p, "pa ", 3) == 0) {addPath(p+2); return;}
   if (strcmp(p, "fp") == 0) {flushPath(0); return;}
   if (strcmp(p, "ip") == 0) {flushPath(1); return;} /* tpic 2.0 */
   if (strncmp(p, "da ", 3) == 0) {flushDashed(p+2, 0); return;}
   if (strncmp(p, "dt ", 3) == 0) {flushDashed(p+2, 1); return;}
   if (strcmp(p, "sp") == 0) {flushSpline(p+2); return;} /* tpic 2.0 */
   if (strncmp(p, "sp ", 3) == 0) {flushSpline(p+3); return;} /* tpic 2.0 */
   if (strncmp(p, "ar ", 3) == 0) {arc(p+2, 0); return;} /* tpic 2.0 */
   if (strncmp(p, "ia ", 3) == 0) {arc(p+2, 1); return;} /* tpic 2.0 */
   if (strcmp(p, "sh") == 0) {shadeLast(p+2); return;} /* tpic 2.0 */
   if (strncmp(p, "sh ", 3) == 0) {shadeLast(p+3); return;} /* tpic 2.0 */
   if (strcmp(p, "wh") == 0) {whitenLast(); return;}
   if (strcmp(p, "bk") == 0) {blackenLast(); return;}
   if (strncmp(p, "tx ", 3) == 0) {SetShade(p+3); return;}
#endif
   if (*p == '"') {
      hvpos();
      cmdout("@beginspecial") ;
      cmdout("@setspecial") ;
      trytobreakout(p+1) ;
      cmdout("\n@endspecial") ;
      return ;
   }

/* At last we get to the key/value conventions */
   psfile[0] = '\0';
   hvpos();
   cmdout("@beginspecial");

   while( (p=GetKeyVal(p,&j)) != NULL )
      switch (j) {
 case -1: /* for compatability with old conventions, we allow a file name
           * to be given without the 'psfile=' keyword */
         if (!psfile[0] && maccess(KeyStr)==0) /* yes we can read it */
             (void)strcpy(psfile,KeyStr) ;
         else {
           sprintf(errbuf, "Unknown keyword (%s) in \\special will be ignored",
                              KeyStr) ;
           specerror(errbuf) ;
         }
         break ;
 case 0: case 1: case 2: /* psfile */
         if (psfile[0]) {
           sprintf(errbuf, "More than one \\special %s given; %s ignored", 
                    "psfile",  ValStr) ;
           specerror(errbuf) ;
         }
         else (void)strcpy(psfile,ValStr) ;
         task = tasks[j] ;
         break ;
 default: /* most keywords are output as PostScript procedure calls */
         if (KeyTab[j].Type == Integer)
            numout(ValInt);
         else if (KeyTab[j].Type == String)
            for (q=ValStr; *q; q++)
               scout(*q) ;
         else if (KeyTab[j].Type == None) ;
         else { /* Number or Dimension */
            ValInt = (integer)(ValNum<0? ValNum-0.5 : ValNum+0.5) ;
            if (ValInt-ValNum < 0.001 && ValInt-ValNum > -0.001)
                numout(ValInt) ;
            else {
               (void)sprintf(cmdbuf, "%f", ValNum) ;
               cmdout(cmdbuf) ;
            }
         }
      (void)sprintf(cmdbuf, "@%s", KeyStr);
      cmdout(cmdbuf) ;
      }

   cmdout("@setspecial");

   if(psfile[0]) {
      if (task == 0) {
         systemtype = (psfile[0]=='`') ;
         figcopyfile(psfile+systemtype, systemtype);
      } else {
         fil2ps(task, psfile) ;
      }
   } else if (psfilewanted)
      specerror("No \\special psfile was given; figure will be blank") ;

   cmdout("@endspecial");
}

extern char realnameoffile[] ;
extern char *pictpath ;

void fil2ps(task, iname)
char *task, *iname ;
{
   char cmd[400] ;
   FILE *f ;
   if (f=search(pictpath, iname, "r")) {
      fclose(f) ;
   } else {
      fprintf(stderr, " couldn't open %s\n", iname) ;
      return ;
   }
   if (!quiet) {
      fprintf(stderr, " [%s", realnameoffile) ;
      fflush(stderr) ;
   }
   if (oname && oname[0] && oname[0] != '-') {
      putc(10, bitfile) ;
      fclose(bitfile) ;
      sprintf(cmd, "%s -f %s %s", task, realnameoffile, oname) ;
      system(cmd) ;
      if ((bitfile=fopen(oname, "a"))==NULL)
         error("! couldn't reopen PostScript file") ;
      linepos = 0 ;
   } else {
      sprintf(cmd, "%s -f %s", task, realnameoffile) ;
      system(cmd) ;
   }
   if (!quiet)
      fprintf(stderr, "]") ;
}
