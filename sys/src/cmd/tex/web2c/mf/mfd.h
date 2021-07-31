#undef	TRIP
#undef	TRAP
#define	STAT
#undef	DEBUG
#include "mf.h"
/* 1 9998 9999 */ 
#define memmax 262140L 
#define maxinternal 300 
#define bufsize 3000 
#define errorline 79 
#define halferrorline 50 
#define maxprintline 79 
#define screenwidth 1664 
#define screendepth 1200 
#define stacksize 300 
#define maxstrings 7500 
#define stringvacancies 74000L 
#define poolsize 100000L 
#define movesize 20000 
#define maxwiggle 1000 
#define gfbufsize 16384 
#define poolname "mf.pool" 
#define pathsize 1000 
#define bistacksize 1500 
#define headersize 100 
#define ligtablesize 15000 
#define maxkerns 2500 
#define maxfontdimen 50 
#define memtop 262140L 
typedef unsigned char ASCIIcode  ; 
typedef unsigned char eightbits  ; 
typedef file_ptr /* of  ASCIIcode */ alphafile  ; 
typedef file_ptr /* of  eightbits */ bytefile  ; 
typedef integer poolpointer  ; 
typedef integer strnumber  ; 
typedef unsigned char packedASCIIcode  ; 
typedef integer scaled  ; 
typedef schar smallnumber  ; 
typedef integer fraction  ; 
typedef integer angle  ; 
typedef short quarterword  ; 
typedef integer halfword  ; 
typedef schar twochoices  ; 
typedef schar threechoices  ; 
#include "memory.h"
typedef file_ptr /* of  memoryword */ wordfile  ; 
typedef schar commandcode  ; 
typedef integer screenrow  ; 
typedef integer screencol  ; 
typedef screencol transspec [screenwidth + 1] ; 
typedef schar pixelcolor  ; 
typedef schar windownumber  ; 
typedef struct {
    quarterword indexfield ; 
  halfword startfield, locfield, limitfield, namefield ; 
} instaterecord  ; 
typedef integer gfindex  ; 
EXTERN integer bad  ; 
EXTERN ASCIIcode xord[256]  ; 
EXTERN ASCIIcode xchr[256]  ; 
EXTERN char nameoffile[FILENAMESIZE + 1], realnameoffile[FILENAMESIZE + 1]  ; 
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
EXTERN schar strref[maxstrings + 1]  ; 
#ifdef INIMF
EXTERN alphafile poolfile  ; 
#endif /* INIMF */
EXTERN alphafile logfile  ; 
EXTERN schar selector  ; 
EXTERN schar dig[23]  ; 
EXTERN integer tally  ; 
EXTERN integer termoffset  ; 
EXTERN integer fileoffset  ; 
EXTERN ASCIIcode trickbuf[errorline + 1]  ; 
EXTERN integer trickcount  ; 
EXTERN integer firstcount  ; 
EXTERN schar interaction  ; 
EXTERN boolean deletionsallowed  ; 
EXTERN schar history  ; 
EXTERN schar errorcount  ; 
EXTERN strnumber helpline[6]  ; 
EXTERN schar helpptr  ; 
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
EXTERN schar jrandom  ; 
EXTERN memoryword mem[memmax + 1]  ; 
EXTERN halfword lomemmax  ; 
EXTERN halfword himemmin  ; 
EXTERN integer varused, dynused  ; 
EXTERN halfword avail  ; 
EXTERN halfword memend  ; 
EXTERN halfword rover  ; 
#ifdef DEBUG
EXTERN boolean freearr[memmax + 1]  ; 
EXTERN boolean wasfree[memmax + 1]  ; 
EXTERN halfword wasmemend, waslomax, washimin  ; 
EXTERN boolean panicking  ; 
#endif /* DEBUG */
EXTERN scaled internal[maxinternal + 1]  ; 
EXTERN strnumber intname[maxinternal + 1]  ; 
EXTERN integer intptr  ; 
EXTERN schar oldsetting  ; 
EXTERN schar charclass[256]  ; 
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
EXTERN fraction uu