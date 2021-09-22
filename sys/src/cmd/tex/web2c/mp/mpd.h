#undef TRIP
#undef TRAP
#define STAT
#define INI
#define INIMP
#define MP
#ifdef TEXMF_DEBUG
#endif /* TEXMF_DEBUG */
#define MPCOERCE
#include "texmfmp.h"
/* 1 9998 9999 */ 
#define maxinternal ( 300 ) 
#define bufsize ( 3000 ) 
#define emergencylinelength ( 255 ) 
#define stacksize ( 300 ) 
#define maxreadfiles ( 30 ) 
#define stringsvacant ( 1000 ) 
#define fontmax ( 50 ) 
#define fontmemsize ( 10000 ) 
#define poolname ( "mp.pool" ) 
#define pstabname ( "psfonts.map" ) 
#define pathsize ( 1000 ) 
#define bistacksize ( 1500 ) 
#define headersize ( 100 ) 
#define ligtablesize ( 15000 ) 
#define maxkerns ( 2500 ) 
#define maxfontdimen ( 50 ) 
#define infmainmemory ( 2999 ) 
#define supmainmemory ( 8000000L ) 
#define infmaxstrings ( 2500 ) 
#define supmaxstrings ( 32767 ) 
#define infpoolsize ( 32000 ) 
#define suppoolsize ( 10000000L ) 
#define infpoolfree ( 1000 ) 
#define suppoolfree ( suppoolsize ) 
#define infstringvacancies ( 8000 ) 
#define supstringvacancies ( suppoolsize - 23000 ) 
typedef unsigned char ASCIIcode  ;
typedef unsigned char eightbits  ;
typedef text /* of  ASCIIcode */ alphafile  ;
typedef text /* of  eightbits */ bytefile  ;
typedef integer poolpointer  ;
typedef integer strnumber  ;
typedef unsigned char poolASCIIcode  ;
typedef integer scaled  ;
typedef char smallnumber  ;
typedef integer fraction  ;
typedef integer angle  ;
typedef unsigned char quarterword  ;
typedef integer halfword  ;
typedef char twochoices  ;
typedef char threechoices  ;
#include "texmfmem.h"
typedef text /* of  memoryword */ wordfile  ;
typedef char commandcode  ;
typedef struct {
    quarterword indexfield ;
  halfword startfield, locfield, limitfield, namefield ;
} instaterecord  ;
typedef integer readfindex  ;
typedef char writeindex  ;
typedef integer fontnumber  ;
typedef char strreftype  ;
EXTERN integer bad  ;
#ifdef INIMP
EXTERN boolean iniversion  ;
EXTERN boolean dumpoption  ;
EXTERN boolean dumpline  ;
#endif /* INIMP */
EXTERN integer bounddefault  ;
EXTERN char * boundname  ;
EXTERN integer mainmemory  ;
EXTERN integer memtop  ;
EXTERN integer memmax  ;
EXTERN integer errorline  ;
EXTERN integer halferrorline  ;
EXTERN integer maxprintline  ;
EXTERN integer poolsize  ;
EXTERN integer stringvacancies  ;
EXTERN integer poolfree  ;
EXTERN integer maxstrings  ;
EXTERN ASCIIcode xord[256]  ;
EXTERN ASCIIcode xchr[256]  ;
EXTERN ASCIIcode * nameoffile  ;
EXTERN integer namelength  ;
EXTERN ASCIIcode buffer[bufsize + 1]  ;
EXTERN integer first  ;
EXTERN integer last  ;
EXTERN integer maxbufstack  ;
EXTERN poolASCIIcode * strpool  ;
EXTERN poolpointer * strstart  ;
EXTERN strnumber * nextstr  ;
EXTERN poolpointer poolptr  ;
EXTERN strnumber strptr  ;
EXTERN poolpointer initpoolptr  ;
EXTERN strnumber initstruse  ;
EXTERN poolpointer maxpoolptr  ;
EXTERN strnumber maxstrptr  ;
EXTERN integer strsusedup  ;
EXTERN integer poolinuse  ;
EXTERN integer strsinuse  ;
EXTERN integer maxplused  ;
EXTERN integer maxstrsused  ;
EXTERN strreftype * strref  ;
EXTERN strnumber lastfixedstr  ;
EXTERN strnumber fixedstruse  ;
EXTERN boolean stroverflowed  ;
EXTERN integer pactcount  ;
EXTERN integer pactchars  ;
EXTERN integer pactstrs  ;
#ifdef INIMP
EXTERN alphafile poolfile  ;
#endif /* INIMP */
EXTERN alphafile logfile  ;
EXTERN alphafile psfile  ;
EXTERN char selector  ;
EXTERN char dig[23]  ;
EXTERN integer tally  ;
EXTERN integer termoffset  ;
EXTERN integer fileoffset  ;
EXTERN integer psoffset  ;
EXTERN ASCIIcode trickbuf[256]  ;
EXTERN integer trickcount  ;
EXTERN integer firstcount  ;
EXTERN char interaction  ;
EXTERN char interactionoption  ;
EXTERN boolean deletionsallowed  ;
EXTERN char history  ;
EXTERN schar errorcount  ;
EXTERN strnumber helpline[6]  ;
EXTERN char helpptr  ;
EXTERN boolean useerrhelp  ;
EXTERN strnumber errhelp  ;
EXTERN integer interrupt  ;
EXTERN boolean OKtointerrupt  ;
EXTERN boolean aritherror  ;
EXTERN integer twotothe[31]  ;
EXTERN integer speclog[29]  ;
EXTERN angle specatan[27]  ;
EXTERN fraction nsin, ncos  ;
EXTERN fraction randoms[55]  ;
EXTERN char jrandom  ;
EXTERN memoryword * mem  ;
EXTERN halfword lomemmax  ;
EXTERN halfword himemmin  ;
EXTERN integer varused, dynused  ;
EXTERN halfword avail  ;
EXTERN halfword memend  ;
EXTERN halfword rover  ;
#ifdef TEXMF_DEBUG
EXTERN boolean freearr[2]  ;
EXTERN boolean wasfree[2]  ;
EXTERN halfword wasmemend, waslomax, washimin  ;
EXTERN boolean panicking  ;
#endif /* TEXMF_DEBUG */
EXTERN scaled internal[maxinternal + 1]  ;
EXTERN strnumber intname[maxinternal + 1]  ;
EXTERN integer intptr  ;
EXTERN char oldsetting, nonpssetting  ;
EXTERN char charclass[256]  ;
EXTERN halfword hashused  ;
EXTERN integer stcount  ;
EXTERN twohalves hash[9772]  ;
EXTERN twohalves eqtb[9772]  ;
EXTERN halfword gpointer  ;
EXTERN smallnumber 
#define bignodesize (zzzaa -12)
  zzzaa[3]  ;
EXTERN smallnumber 
#define sector0 (zzzab -12)
  zzzab[3]  ;
EXTERN smallnumber 
#define sectoroffset (zzzac -5)
  zzzac[9]  ;
EXTERN halfword saveptr  ;
EXTERN halfword pathtail  ;
EXTERN scaled deltax[pathsize + 1], deltay[pathsize + 1], delta[pathsize + 1]  ;
EXTERN angle psi[pathsize + 1]  ;
EXTERN angle theta[pathsize + 1]  ;
EXTERN fraction uu[pathsize + 1]  ;
EXTERN angle vv[pathsize + 1]  ;
EXTERN fraction ww[pathsize + 1]  ;
EXTERN fraction st, ct, sf, cf  ;
EXTERN scaled bbmin[2], bbmax[2]  ;
EXTERN fraction halfcos[8]  ;
EXTERN fraction dcos[8]  ;
EXTERN scaled curx, cury  ;
EXTERN smallnumber grobjectsize[8]  ;
EXTERN integer specoffset  ;
EXTERN halfword specp1, specp2  ;
EXTERN char tolstep  ;
EXTERN integer bisectstack[bistacksize + 1]  ;
EXTERN integer bisectptr  ;
EXTERN integer curt, curtt  ;
EXTERN integer timetogo  ;
EXTERN integer maxt  ;
EXTERN integer delx, dely  ;
EXTERN integer tol  ;
EXTERN integer uv, xy  ;
EXTERN integer threel  ;
EXTERN integer apprt, apprtt  ;
EXTERN integer serialno  ;
EXTERN boolean fixneeded  ;
EXTERN boolean watchcoefs  ;
EXTERN halfword depfinal  ;
EXTERN eightbits curcmd  ;
EXTERN integer curmod  ;
EXTERN halfword cursym  ;
EXTERN instaterecord inputstack[stacksize + 1]  ;
EXTERN integer inputptr  ;
EXTERN integer maxinstack  ;
EXTERN instaterecord curinput  ;
EXTERN char inopen  ;
EXTERN char openparens  ;
EXTERN alphafile inputfile[16]  ;
EXTERN integer linestack[16]  ;
EXTERN strnumber inamestack[16]  ;
EXTERN strnumber iareastack[16]  ;
EXTERN halfword mpxname[16]  ;
EXTERN halfword paramstack[151]  ;
EXTERN unsigned char paramptr  ;
EXTERN integer maxparamstack  ;
EXTERN integer fileptr  ;
EXTERN char scannerstatus  ;
EXTERN integer warninginfo  ;
EXTERN boolean forceeof  ;
EXTERN short bgloc, egloc  ;
EXTERN halfword condptr  ;
EXTERN char iflimit  ;
EXTERN smallnumber curif  ;
EXTERN integer ifline  ;
EXTERN halfword loopptr  ;
EXTERN strnumber curname  ;
EXTERN strnumber curarea  ;
EXTERN strnumber curext  ;
EXTERN integer areadelimiter  ;
EXTERN integer extdelimiter  ;
EXTERN integer memdefaultlength  ;
EXTERN char * MPmemdefault  ;
EXTERN boolean troffmode  ;
EXTERN strnumber jobname  ;
EXTERN boolean logopened  ;
EXTERN strnumber texmflogname  ;
EXTERN char * oldfilename  ;
EXTERN integer oldnamelength  ;
EXTERN alphafile rdfile[maxreadfiles + 1]  ;
EXTERN strnumber rdfname[maxreadfiles + 1]  ;
EXTERN readfindex readfiles  ;
EXTERN alphafile wrfile[5]  ;
EXTERN strnumber wrfname[5]  ;
EXTERN writeindex writefiles  ;
EXTERN smallnumber curtype  ;
EXTERN integer curexp  ;
EXTERN integer 
#define maxc (zzzad -17)
  zzzad[2]  ;
EXTERN halfword 
#define maxptr (zzzae -17)
  zzzae[2]  ;
EXTERN halfword 
#define maxlink (zzzaf -17)
  zzzaf[2]  ;
EXTERN char varflag  ;
EXTERN halfword sepic  ;
EXTERN scaled sesf  ;
EXTERN strnumber eofline  ;
EXTERN scaled txx, txy, tyx, tyy, tx, ty  ;
EXTERN quarterword lastaddtype  ;
EXTERN halfword startsym  ;
EXTERN boolean longhelpseen  ;
EXTERN bytefile tfmfile  ;
EXTERN strnumber metricfilename  ;
EXTERN eightbits bc, ec  ;
EXTERN scaled tfmwidth[256]  ;
EXTERN scaled tfmheight[256]  ;
EXTERN scaled tfmdepth[256]  ;
EXTERN scaled tfmitalcorr[256]  ;
EXTERN boolean charexists[256]  ;
EXTERN char chartag[256]  ;
EXTERN integer charremainder[256]  ;
EXTERN short headerbyte[headersize + 1]  ;
EXTERN fourquarters ligkern[ligtablesize + 1]  ;
EXTERN short nl  ;
EXTERN scaled kern[maxkerns + 1]  ;
EXTERN integer nk  ;
EXTERN fourquarters exten[256]  ;
EXTERN short ne  ;
EXTERN scaled param[maxfontdimen + 1]  ;
EXTERN integer np  ;
EXTERN short nw, nh, nd, ni  ;
EXTERN integer skiptable[256]  ;
EXTERN boolean lkstarted  ;
EXTERN integer bchar  ;
EXTERN integer bchlabel  ;
EXTERN integer ll, lll  ;
EXTERN integer labelloc[257]  ;
EXTERN eightbits labelchar[257]  ;
EXTERN short labelptr  ;
EXTERN scaled perturbation  ;
EXTERN integer excess  ;
EXTERN halfword dimenhead[5]  ;
EXTERN scaled maxtfmdimen  ;
EXTERN integer tfmchanged  ;
EXTERN bytefile tfminfile  ;
EXTERN memoryword fontinfo[fontmemsize + 1]  ;
EXTERN integer nextfmem  ;
EXTERN fontnumber lastfnum  ;
EXTERN scaled fontdsize[fontmax + 1]  ;
EXTERN strnumber fontname[fontmax + 1]  ;
EXTERN strnumber fontpsname[fontmax + 1]  ;
EXTERN fontnumber lastpsfnum  ;
EXTERN eightbits fontbc[fontmax + 1], fontec[fontmax + 1]  ;
EXTERN integer charbase[fontmax + 1]  ;
EXTERN integer widthbase[fontmax + 1]  ;
EXTERN integer heightbase[fontmax + 1]  ;
EXTERN integer depthbase[fontmax + 1]  ;
EXTERN alphafile pstabfile  ;
EXTERN strnumber firstfilename, lastfilename  ;
EXTERN integer firstoutputcode, lastoutputcode  ;
EXTERN integer totalshipped  ;
EXTERN boolean neednewpath  ;
EXTERN scaled gsred, gsgreen, gsblue  ;
EXTERN quarterword gsljoin, gslcap  ;
EXTERN scaled gsmiterlim  ;
EXTERN halfword gsdashp  ;
EXTERN scaled gsdashsc  ;
EXTERN scaled gswidth  ;
EXTERN boolean gsadjwx  ;
EXTERN halfword fontsizes[fontmax + 1]  ;
EXTERN halfword lastpending  ;
EXTERN strnumber memident  ;
EXTERN wordfile memfile  ;
EXTERN integer readyalready  ;
EXTERN poolpointer editnamestart  ;
EXTERN integer editnamelength, editline  ;

#include "mpcoerce.h"
