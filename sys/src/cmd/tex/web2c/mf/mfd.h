#undef TRIP
#undef TRAP
#define STAT
#define INI
#define INIMF
#define MF
#ifdef TEXMF_DEBUG
#endif /* TEXMF_DEBUG */
#define MFCOERCE
#include "texmfmp.h"
/* 1 9998 9999 */ 
#define maxinternal ( 300 ) 
#define bufsize ( 3000 ) 
#define screenwidth ( 1664 ) 
#define screendepth ( 1200 ) 
#define stacksize ( 300 ) 
#define maxstrings ( 7500 ) 
#define stringvacancies ( 74000L ) 
#define poolsize ( 100000L ) 
#define movesize ( 20000 ) 
#define maxwiggle ( 1000 ) 
#define poolname ( "mf.pool" ) 
#define pathsize ( 1000 ) 
#define bistacksize ( 1500 ) 
#define headersize ( 100 ) 
#define ligtablesize ( 15000 ) 
#define maxkerns ( 2500 ) 
#define maxfontdimen ( 60 ) 
#define infmainmemory ( 2999 ) 
#define supmainmemory ( 8000000L ) 
typedef unsigned char ASCIIcode  ;
typedef unsigned char eightbits  ;
typedef text /* of  ASCIIcode */ alphafile  ;
typedef text /* of  eightbits */ bytefile  ;
typedef integer poolpointer  ;
typedef integer strnumber  ;
typedef unsigned char packedASCIIcode  ;
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
typedef integer screenrow  ;
typedef integer screencol  ;
typedef screencol transspec [screenwidth + 1] ;
typedef char pixelcolor  ;
typedef char windownumber  ;
typedef struct {
    quarterword indexfield ;
  halfword startfield, locfield, limitfield, namefield ;
} instaterecord  ;
typedef integer gfindex  ;
EXTERN integer bad  ;
#ifdef INIMF
EXTERN boolean iniversion  ;
EXTERN boolean dumpoption  ;
EXTERN boolean dumpline  ;
#endif /* INIMF */
EXTERN integer bounddefault  ;
EXTERN char * boundname  ;
EXTERN integer mainmemory  ;
EXTERN integer memtop  ;
EXTERN integer memmax  ;
EXTERN integer errorline  ;
EXTERN integer halferrorline  ;
EXTERN integer maxprintline  ;
EXTERN integer gfbufsize  ;
EXTERN ASCIIcode xord[256]  ;
EXTERN ASCIIcode xchr[256]  ;
EXTERN ASCIIcode * nameoffile  ;
EXTERN integer namelength  ;
EXTERN ASCIIcode buffer[bufsize + 1]  ;
EXTERN integer first  ;
EXTERN integer last  ;
EXTERN integer maxbufstack  ;
EXTERN packedASCIIcode strpool[poolsize + 1]  ;
EXTERN poolpointer strstart[maxstrings + 1]  ;
EXTERN poolpointer poolptr  ;
EXTERN strnumber strptr  ;
EXTERN poolpointer initpoolptr  ;
EXTERN strnumber initstrptr  ;
EXTERN poolpointer maxpoolptr  ;
EXTERN strnumber maxstrptr  ;
EXTERN char strref[maxstrings + 1]  ;
#ifdef INIMF
EXTERN alphafile poolfile  ;
#endif /* INIMF */
EXTERN alphafile logfile  ;
EXTERN char selector  ;
EXTERN char dig[23]  ;
EXTERN integer tally  ;
EXTERN integer termoffset  ;
EXTERN integer fileoffset  ;
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
EXTERN char oldsetting  ;
EXTERN char charclass[256]  ;
EXTERN halfword hashused  ;
EXTERN integer stcount  ;
EXTERN twohalves hash[9770]  ;
EXTERN twohalves eqtb[9770]  ;
EXTERN halfword gpointer  ;
EXTERN smallnumber 
#define bignodesize (zzzaa -13)
  zzzaa[2]  ;
EXTERN halfword saveptr  ;
EXTERN halfword pathtail  ;
EXTERN scaled deltax[pathsize + 1], deltay[pathsize + 1], delta[pathsize + 1]  ;
EXTERN angle psi[pathsize + 1]  ;
EXTERN angle theta[pathsize + 1]  ;
EXTERN fraction uu[pathsize + 1]  ;
EXTERN angle vv[pathsize + 1]  ;
EXTERN fraction ww[pathsize + 1]  ;
EXTERN fraction st, ct, sf, cf  ;
EXTERN integer move[movesize + 1]  ;
EXTERN integer moveptr  ;
EXTERN integer bisectstack[bistacksize + 1]  ;
EXTERN integer bisectptr  ;
EXTERN halfword curedges  ;
EXTERN integer curwt  ;
EXTERN integer tracex  ;
EXTERN integer tracey  ;
EXTERN integer traceyy  ;
EXTERN char octant  ;
EXTERN scaled curx, cury  ;
EXTERN strnumber octantdir[9]  ;
EXTERN halfword curspec  ;
EXTERN integer turningnumber  ;
EXTERN halfword curpen  ;
EXTERN char curpathtype  ;
EXTERN scaled maxallowed  ;
EXTERN scaled before[maxwiggle + 1], after[maxwiggle + 1]  ;
EXTERN halfword nodetoround[maxwiggle + 1]  ;
EXTERN integer curroundingptr  ;
EXTERN integer maxroundingptr  ;
EXTERN scaled curgran  ;
EXTERN char octantnumber[9]  ;
EXTERN char octantcode[9]  ;
EXTERN boolean revturns  ;
EXTERN char ycorr[9], xycorr[9], zcorr[9]  ;
EXTERN schar xcorr[9]  ;
EXTERN integer m0, n0, m1, n1  ;
EXTERN char d0, d1  ;
EXTERN integer envmove[movesize + 1]  ;
EXTERN char tolstep  ;
EXTERN integer curt, curtt  ;
EXTERN integer timetogo  ;
EXTERN integer maxt  ;
EXTERN integer delx, dely  ;
EXTERN integer tol  ;
EXTERN integer uv, xy  ;
EXTERN integer threel  ;
EXTERN integer apprt, apprtt  ;
EXTERN boolean screenstarted  ;
EXTERN boolean screenOK  ;
EXTERN boolean windowopen[16]  ;
EXTERN screencol leftcol[16]  ;
EXTERN screencol rightcol[16]  ;
EXTERN screenrow toprow[16]  ;
EXTERN screenrow botrow[16]  ;
EXTERN integer mwindow[16]  ;
EXTERN integer nwindow[16]  ;
EXTERN integer windowtime[16]  ;
EXTERN transspec rowtransition  ;
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
EXTERN integer line  ;
EXTERN integer linestack[16]  ;
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
EXTERN poolpointer areadelimiter  ;
EXTERN poolpointer extdelimiter  ;
EXTERN integer basedefaultlength  ;
EXTERN char * MFbasedefault  ;
EXTERN strnumber jobname  ;
EXTERN boolean logopened  ;
EXTERN strnumber texmflogname  ;
EXTERN strnumber gfext  ;
EXTERN bytefile gffile  ;
EXTERN strnumber outputfilename  ;
EXTERN smallnumber curtype  ;
EXTERN integer curexp  ;
EXTERN integer 
#define maxc (zzzab -17)
  zzzab[2]  ;
EXTERN halfword 
#define maxptr (zzzac -17)
  zzzac[2]  ;
EXTERN halfword 
#define maxlink (zzzad -17)
  zzzad[2]  ;
EXTERN char varflag  ;
EXTERN scaled txx, txy, tyx, tyy, tx, ty  ;
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
EXTERN integer gfminm, gfmaxm, gfminn, gfmaxn  ;
EXTERN integer gfprevptr  ;
EXTERN integer totalchars  ;
EXTERN integer charptr[256]  ;
EXTERN integer gfdx[256], gfdy[256]  ;
EXTERN eightbits * gfbuf  ;
EXTERN gfindex halfbuf  ;
EXTERN gfindex gflimit  ;
EXTERN gfindex gfptr  ;
EXTERN integer gfoffset  ;
EXTERN integer bocc, bocp  ;
EXTERN strnumber baseident  ;
EXTERN wordfile basefile  ;
EXTERN integer readyalready  ;
EXTERN poolpointer editnamestart  ;
EXTERN integer editnamelength, editline  ;

#include "mfcoerce.h"
