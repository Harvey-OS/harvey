/*
 *   This routine handles special commands;
 *   predospecial() is for the prescan, dospecial() for the real thing.
 */
#include "dvips.h" /* The copyright notice in that file is included too! */

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
extern void emspecial() ;
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
extern int prettycolumn ;
extern Boolean disablecomments ;

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
#ifndef __GNUC__	/* GNUC tolower is too simple */
#define Tolower tolower
#endif
#else
#ifdef VMCMS    /* IBM: VM/CMS */
#define Tolower __tolower
#else
#ifdef MVSXA    /* IBM: MVS/XA */
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
#endif
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
   if (0 != (t = *s))
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
 *
 *   To support GNUplot's horribly long specials, we go ahead and malloc a
 *   new string buffer if necessary.
 */

void predospecial(numbytes, scanning)
integer numbytes ;
Boolean scanning ;
{
   register char *p = nextstring ;
   register int i = 0 ;
   int j ;

   if (nextstring + numbytes > maxstring) {
      p = nextstring = mymalloc(1000 + 2 * numbytes) ;
      maxstring = nextstring + 2 * numbytes + 700 ;
   }
   for (i=numbytes; i>0; i--)
#ifdef VMCMS /* IBM: VM/CMS */
      *p++ = ascii2ebcdic[(char)dvibyte()] ;
#else
#ifdef MVSXA /* IBM: MVS/XA */
      *p++ = ascii2ebcdic[(char)dvibyte()] ;
#else
      *p++ = (char)dvibyte() ;
#endif /* IBM: VM/CMS */
#endif
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

   switch (*p) {
case 'l':
   if (strncmp(p, "landscape", 9)==0) {
      if (hpapersize || vpapersize)
         error(
             "both landscape and papersize specified:  ignoring landscape") ;
      else
         landscape = 1 ;
      return ;
   }
   break ;
case 'p':
   if (strncmp(p, "papersize", 9)==0) {
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
   break ;
case 'x':
   if (strncmp(p, "xtex:", 5)==0) return ;
   break ;
case 'h':
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
   break ;
/* IBM: color - added section here for color header and color history */
case 'b':
   if (strncmp(p, "background", 10) == 0) {
      usescolor = 1 ;
      p +=11 ;
      while ( *p <= ' ' ) p++ ;
      background(p) ;
      return ;
   }
   break ;
case 'c':
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
   break ;
case '!':
   {
      register struct bangspecial *q ;
      p++ ;
      q = (struct bangspecial *)mymalloc((integer)
                         (sizeof(struct bangspecial) + strlen(p))) ;
      (void)strcpy(q->actualstuff, p) ;
      q->next = bangspecials ;
      bangspecials = q ;
      usesspecial = 1 ;
      return ;
   }
   break ;
default:
   break ;
   }
   usesspecial = 1 ;  /* now the special prolog will be sent */
   if (scanning && *p != '"' && (p=GetKeyVal(p, &j)) != NULL && j==0)
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

static char psfile[511] ; 
void dospecial(numbytes)
integer numbytes ;
{
   register char *p = nextstring ;
   register int i = 0 ;
   int j, systemtype = 0 ;
   register char *q ;
   Boolean psfilewanted = 1 ;
   char *task = 0 ;
   char cmdbuf[111] ; 

   if (nextstring + i > maxstring)
      error("! out of string space in dospecial") ;
   for (i=numbytes; i>0; i--)
#ifdef VMCMS /* IBM: VM/CMS */
      *p++ = ascii2ebcdic[(char)dvibyte()] ;
#else
#ifdef MVSXA /* IBM: MVS/XA */
      *p++ = ascii2ebcdic[(char)dvibyte()] ;
#else
      *p++ = (char)dvibyte() ;
#endif  /* IBM: VM/CMS */
#endif
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

   switch (*p) {
case 'e':
   if (strncmp(p, "em:", 3)==0) {	/* emTeX specials in emspecial.c */
	emspecial(p);
	return;
   }
   break ;
case 'p':
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
           psflush() ;
           hvpos() ;
        }
        return;
   }
   if (strncmp(p, "papersize", 9) == 0)
      return ;
#ifdef TPIC
   if (strncmp(p, "pn ", 3) == 0) {setPenSize(p+2); return;}
   if (strncmp(p, "pa ", 3) == 0) {addPath(p+2); return;}
#endif
   break ;
case 'l':
   if (strncmp(p, "landscape", 9)==0) return ;
   break ;
case '!':
   return ;
case 'h':
   if (strncmp(p, "header", 6)==0) return ;
   if (strncmp(p, "html:", 5)==0) return ;
case 'w':
case 'W':
   if (strncmp(p+1, "arning", 6) == 0) {
      static int maxwarns = 50 ;
      if (maxwarns > 0) {
         error(p) ;
         maxwarns-- ;
      } else if (maxwarns == 0) {
         error(". . . rest of warnings suppressed") ;
         maxwarns-- ;
      }
      return ;
   }
#ifdef TPIC
   if (strcmp(p, "wh") == 0) {whitenLast(); return;}
#endif
   break ;
case 'b':
   if ( strncmp(p, "background", 10) == 0 )
      return ; /* already handled in prescan */
#ifdef TPIC
   if (strcmp(p, "bk") == 0) {blackenLast(); return;}
#endif
   break ;
case 'c':
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
   break ;
case 'f':
#ifdef TPIC
   if (strcmp(p, "fp") == 0) {flushPath(0); return;}
#endif
   break ;
case 'i':
#ifdef TPIC
   if (strcmp(p, "ip") == 0) {flushPath(1); return;} /* tpic 2.0 */
   if (strncmp(p, "ia ", 3) == 0) {arc(p+2, 1); return;} /* tpic 2.0 */
#endif
   break ;
case 'd':
#ifdef TPIC
   if (strncmp(p, "da ", 3) == 0) {flushDashed(p+2, 0); return;}
   if (strncmp(p, "dt ", 3) == 0) {flushDashed(p+2, 1); return;}
#endif
   break ;
case 's':
#ifdef TPIC
   if (strcmp(p, "sp") == 0) {flushSpline(p+2); return;} /* tpic 2.0 */
   if (strncmp(p, "sp ", 3) == 0) {flushSpline(p+3); return;} /* tpic 2.0 */
   if (strcmp(p, "sh") == 0) {shadeLast(p+2); return;} /* tpic 2.0 */
   if (strncmp(p, "sh ", 3) == 0) {shadeLast(p+3); return;} /* tpic 2.0 */
#endif
   break ;
case 'a':
#ifdef TPIC
   if (strncmp(p, "ar ", 3) == 0) {arc(p+2, 0); return;} /* tpic 2.0 */
#endif
   break ;
case 't':
#ifdef TPIC
   if (strncmp(p, "tx ", 3) == 0) {SetShade(p+3); return;}
#endif
   break ;
case '"':
   hvpos() ;
   cmdout("@beginspecial") ;
   cmdout("@setspecial") ;
   trytobreakout(p+1) ;
   cmdout("\n@endspecial") ;
   return ;
   break ;
default:
   break ;
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
            numout((integer)ValInt);
         else if (KeyTab[j].Type == String)
            for (q=ValStr; *q; q++)
               scout(*q) ;
         else if (KeyTab[j].Type == None) ;
         else { /* Number or Dimension */
            ValInt = (integer)(ValNum<0? ValNum-0.5 : ValNum+0.5) ;
            if (ValInt-ValNum < 0.001 && ValInt-ValNum > -0.001)
                numout((integer)ValInt) ;
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
   if (0 != (f=search(pictpath, iname, "r"))) {
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

