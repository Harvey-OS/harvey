#include "config.h"
/* 9999 */ 
#define maxfonts 100 
#define maxchars 10000 
#define maxwidths 3000 
#define maxpackets 5000 
#define maxbytes 30000 
#define maxrecursion 10 
#define stacksize 100 
#define terminallinelength 150 
typedef integer int31  ; 
typedef integer int24u  ; 
typedef integer int24  ; 
typedef integer int23  ; 
typedef unsigned short int16u  ; 
typedef integer int16  ; 
typedef short int15  ; 
typedef unsigned char int8u  ; 
typedef short int8  ; 
typedef schar int7  ; 
typedef unsigned char ASCIIcode  ; 
typedef file_ptr /* of  ASCIIcode */ textfile  ; 
typedef short signedbyte  ; 
typedef unsigned char eightbits  ; 
typedef integer signedpair  ; 
typedef unsigned short sixteenbits  ; 
typedef integer signedtrio  ; 
typedef integer twentyfourbits  ; 
typedef integer signedquad  ; 
typedef file_ptr /* of  eightbits */ bytefile  ; 
typedef eightbits packedbyte  ; 
typedef integer bytepointer  ; 
typedef integer pcktpointer  ; 
typedef short hashcode  ; 
typedef integer widthpointer  ; 
typedef integer charoffset  ; 
typedef integer charpointer  ; 
typedef schar ftype  ; 
typedef integer fontnumber  ; 
typedef schar typeflag  ; 
typedef schar cmdpar  ; 
typedef schar cmdcl  ; 
typedef boolean vfstate [2][2] ; 
typedef schar vftype  ; 
typedef integer stackpointer  ; 
typedef integer stackindex  ; 
typedef integer pair32 [2] ; 
typedef struct {
    integer hfield ; 
  integer vfield ; 
  pair32 wxfield ; 
  pair32 yzfield ; 
} stackrecord  ; 
typedef integer recurpointer  ; 
ASCIIcode xord[256]  ; 
ASCIIcode xchr[256]  ; 
schar history  ; 
packedbyte bytemem[maxbytes + 1]  ; 
bytepointer pcktstart[maxpackets + 1]  ; 
bytepointer byteptr  ; 
pcktpointer pcktptr  ; 
pcktpointer plink[maxpackets + 1]  ; 
pcktpointer phash[354]  ; 
pcktpointer strfonts, strchars, strwidths, strpackets, strbytes, strrecursion, 
strstack, strnamelength  ; 
pcktpointer curpckt  ; 
bytepointer curloc  ; 
bytepointer curlimit  ; 
char curname[PATHMAX + 1]  ; 
int15 curnamelength  ; 
integer widths[maxwidths + 1]  ; 
widthpointer wlink[maxwidths + 1]  ; 
widthpointer whash[354]  ; 
widthpointer nwidths  ; 
widthpointer charwidths[maxchars + 1]  ; 
pcktpointer charpackets[maxchars + 1]  ; 
charpointer nchars  ; 
fontnumber nf  ; 
integer fntcheck[maxfonts + 1]  ; 
int31 fntscaled[maxfonts + 1]  ; 
int31 fntdesign[maxfonts + 1]  ; 
pcktpointer fntname[maxfonts + 1]  ; 
eightbits fntbc[maxfonts + 1]  ; 
eightbits fntec[maxfonts + 1]  ; 
charoffset fntchars[maxfonts + 1]  ; 
ftype fnttype[maxfonts + 1]  ; 
fontnumber fntfont[maxfonts + 1]  ; 
fontnumber curfnt  ; 
int24 curext  ; 
int8u curres  ; 
typeflag curtype  ; 
int24 pcktext  ; 
int8u pcktres  ; 
boolean pcktdup  ; 
pcktpointer pcktprev  ; 
int7 pcktmmsg, pcktsmsg, pcktdmsg  ; 
bytefile tfmfile  ; 
pcktpointer tfmext  ; 
eightbits tfmb0, tfmb1, tfmb2, tfmb3  ; 
real tfmconv  ; 
bytefile dvifile  ; 
integer dviloc  ; 
ASCIIcode realnameoffile[PATHMAX + 1]  ; 
integer i  ; 
cmdpar dvipar[256]  ; 
cmdcl dvicl[256]  ; 
eightbits dvicharcmd[2]  ; 
eightbits dvirulecmd[2]  ; 
eightbits 
#define dvirightcmd (zzzaa -7)
  zzzaa[3]  ; 
eightbits 
#define dvidowncmd (zzzab -12)
  zzzab[3]  ; 
eightbits curcmd  ; 
integer curparm  ; 
cmdcl curclass  ; 
widthpointer curwp  ; 
boolean curupd  ; 
integer curvdimen  ; 
integer curhdimen  ; 
integer dviefnts[maxfonts + 1]  ; 
fontnumber dviifnts[maxfonts + 1]  ; 
fontnumber dvinf  ; 
bytefile vffile  ; 
integer vfloc  ; 
integer vflimit  ; 
pcktpointer vfext  ; 
fontnumber vfcurfnt  ; 
integer z  ; 
integer alpha  ; 
int15 beta  ; 
integer vfefnts[maxfonts + 1]  ; 
fontnumber vfifnts[maxfonts + 1]  ; 
fontnumber vfnf  ; 
fontnumber lclnf  ; 
vfstate vfmove[stacksize + 1]  ; 
bytepointer vfpushloc[stacksize + 1]  ; 
bytepointer vflastloc[stacksize + 1]  ; 
bytepointer vflastend[stacksize + 1]  ; 
eightbits vfpushnum[stacksize + 1]  ; 
vftype vflast[stacksize + 1]  ; 
stackpointer vfptr  ; 
stackpointer stackused  ; 
vftype vfchartype[2]  ; 
vftype vfruletype[2]  ; 
integer widthdimen  ; 
bytepointer scanptr  ; 
boolean typesetting  ; 
stackrecord stack[stacksize + 1]  ; 
stackrecord curstack  ; 
stackrecord zerostack  ; 
stackpointer stackptr  ; 
integer selectcount[10][10]  ; 
boolean selectthere[10][10]  ; 
schar selectvals[10]  ; 
integer selectmax[10]  ; 
integer outmag  ; 
integer count[10]  ; 
schar numselect  ; 
schar curselect  ; 
boolean selected  ; 
boolean alldone  ; 
pcktpointer strmag, strselect  ; 
fontnumber recurfnt[maxrecursion + 1]  ; 
int24 recurext[maxrecursion + 1]  ; 
eightbits recurres[maxrecursion + 1]  ; 
pcktpointer recurpckt[maxrecursion + 1]  ; 
bytepointer recurloc[maxrecursion + 1]  ; 
recurpointer nrecur  ; 
recurpointer recurused  ; 
int31 dvinum  ; 
int31 dviden  ; 
int31 dvimag  ; 
bytefile outfile  ; 
char outfname[PATHMAX + 1]  ; 
integer outloc  ; 
integer outback  ; 
int31 outmaxv  ; 
int31 outmaxh  ; 
int16u outstack  ; 
int16u outpages  ; 
fontnumber outfnts[maxfonts + 1]  ; 
fontnumber outnf  ; 
fontnumber outfnt  ; 

#include "dvicopy.h"
void zprintpacket ( p ) 
pcktpointer p ; 
{bytepointer k  ; 
  {register integer for_end; k = pcktstart [ p ] ; for_end = pcktstart [ p + 
  1 ] - 1 ; if ( k <= for_end) do 
    (void) putc( xchr [ bytemem [ k ] ] ,  stdout );
  while ( k++ < for_end ) ; } 
} 
void zprintfont ( f ) 
fontnumber f ; 
{pcktpointer p  ; 
  bytepointer k  ; 
  int31 m  ; 
  (void) Fputs( stdout ,  " = " ) ; 
  p = fntname [ f ] ; 
  {register integer for_end; k = pcktstart [ p ] + 1 ; for_end = pcktstart [ 
  p + 1 ] - 1 ; if ( k <= for_end) do 
    (void) putc( xchr [ bytemem [ k ] ] ,  stdout );
  while ( k++ < for_end ) ; } 
  m = round ( ( fntscaled [ f ] / ((double) fntdesign [ f ] ) ) * outmag ) ; 
  if ( m != 1000 ) 
  (void) fprintf( stdout , "%s%ld",  " scaled " , (long)m ) ; 
} 
void jumpout ( ) 
{history = 3 ; 
  closefilesandterminate () ; 
  uexit ( 1 ) ; 
} 
void zconfusion ( p ) 
pcktpointer p ; 
{(void) Fputs( stdout ,  " !This can't happen (" ) ; 
  printpacket ( p ) ; 
  (void) fprintf( stdout , "%s\n",  ")." ) ; 
  jumpout () ; 
} 
void zoverflow ( p , n ) 
pcktpointer p ; 
int16u n ; 
{(void) fprintf( stdout , "%s%s%s",  " !Sorry, " , "DVIcopy" , " capacity exceeded [" ) ; 
  printpacket ( p ) ; 
  (void) fprintf( stdout , "%c%ld%s\n",  '=' , (long)n , "]." ) ; 
  jumpout () ; 
} 
void badtfm ( ) 
{(void) Fputs( stdout ,  "Bad TFM file" ) ; 
  printfont ( curfnt ) ; 
  (void) fprintf( stdout , "%c\n",  '!' ) ; 
  {
    (void) fprintf( stdout , "%c%s%c\n",  ' ' ,     "Use TFtoPL/PLtoTF to diagnose and correct the problem" , '.' ) ; 
    jumpout () ; 
  } 
} 
void badfont ( ) 
{(void) putc('\n',  stdout );
  switch ( fnttype [ curfnt ] ) 
  {case 0 : 
    badtfm () ; 
    break ; 
  case 1 : 
    {
      (void) Fputs( stdout ,  "Bad VF file" ) ; 
      printfont ( curfnt ) ; 
      (void) fprintf( stdout , "%s%ld\n",  " loc=" , (long)vfloc ) ; 
      {
	(void) fprintf( stdout , "%c%s%c\n",  ' ' ,         "Use VFtoVP/VPtoVF to diagnose and correct the problem" , '.' ) ; 
	jumpout () ; 
      } 
    } 
    break ; 
  } 
} 
void baddvi ( ) 
{(void) putc('\n',  stdout );
  (void) fprintf( stdout , "%s%ld%c\n",  "Bad DVI file: loc=" , (long)dviloc , '!' ) ; 
  (void) Fputs( stdout ,  " Use DVItype with output level" ) ; 
  if ( true ) 
  (void) Fputs( stdout ,  "=4" ) ; 
  else
  (void) Fputs( stdout ,  "<4" ) ; 
  {
    (void) fprintf( stdout , "%c%s%c\n",  ' ' , "to diagnose the problem" , '.' ) ; 
    jumpout () ; 
  } 
} 
void initialize ( ) 
{int16 i  ; 
  hashcode h  ; 
  (void) fprintf( stdout , "%s\n",  "This is DVIcopy, C Version 1.2" ) ; 
  (void) fprintf( stdout , "%s\n",  "Copyright (C) 1990,91 Peter Breitenlohner" ) ; 
  (void) fprintf( stdout , "%s\n",  "Distributed under terms of GNU General Public License" ) 
  ; 
  {register integer for_end; i = 0 ; for_end = 31 ; if ( i <= for_end) do 
    xchr [ i ] = '?' ; 
  while ( i++ < for_end ) ; } 
  xchr [ 32 ] = ' ' ; 
  xchr [ 33 ] = '!' ; 
  xchr [ 34 ] = '"' ; 
  xchr [ 35 ] = '#' ; 
  xchr [ 36 ] = '$' ; 
  xchr [ 37 ] = '%' ; 
  xchr [ 38 ] = '&' ; 
  xchr [ 39 ] = '\'' ; 
  xchr [ 40 ] = '(' ; 
  xchr [ 41 ] = ')' ; 
  xchr [ 42 ] = '*' ; 
  xchr [ 43 ] = '+' ; 
  xchr [ 44 ] = ',' ; 
  xchr [ 45 ] = '-' ; 
  xchr [ 46 ] = '.' ; 
  xchr [ 47 ] = '/' ; 
  xchr [ 48 ] = '0' ; 
  xchr [ 49 ] = '1' ; 
  xchr [ 50 ] = '2' ; 
  xchr [ 51 ] = '3' ; 
  xchr [ 52 ] = '4' ; 
  xchr [ 53 ] = '5' ; 
  xchr [ 54 ] = '6' ; 
  xchr [ 55 ] = '7' ; 
  xchr [ 56 ] = '8' ; 
  xchr [ 57 ] = '9' ; 
  xchr [ 58 ] = ':' ; 
  xchr [ 59 ] = ';' ; 
  xchr [ 60 ] = '<' ; 
  xchr [ 61 ] = '=' ; 
  xchr [ 62 ] = '>' ; 
  xchr [ 63 ] = '?' ; 
  xchr [ 64 ] = '@' ; 
  xchr [ 65 ] = 'A' ; 
  xchr [ 66 ] = 'B' ; 
  xchr [ 67 ] = 'C' ; 
  xchr [ 68 ] = 'D' ; 
  xchr [ 69 ] = 'E' ; 
  xchr [ 70 ] = 'F' ; 
  xchr [ 71 ] = 'G' ; 
  xchr [ 72 ] = 'H' ; 
  xchr [ 73 ] = 'I' ; 
  xchr [ 74 ] = 'J' ; 
  xchr [ 75 ] = 'K' ; 
  xchr [ 76 ] = 'L' ; 
  xchr [ 77 ] = 'M' ; 
  xchr [ 78 ] = 'N' ; 
  xchr [ 79 ] = 'O' ; 
  xchr [ 80 ] = 'P' ; 
  xchr [ 81 ] = 'Q' ; 
  xchr [ 82 ] = 'R' ; 
  xchr [ 83 ] = 'S' ; 
  xchr [ 84 ] = 'T' ; 
  xchr [ 85 ] = 'U' ; 
  xchr [ 86 ] = 'V' ; 
  xchr [ 87 ] = 'W' ; 
  xchr [ 88 ] = 'X' ; 
  xchr [ 89 ] = 'Y' ; 
  xchr [ 90 ] = 'Z' ; 
  xchr [ 91 ] = '[' ; 
  xchr [ 92 ] = '\\' ; 
  xchr [ 93 ] = ']' ; 
  xchr [ 94 ] = '^' ; 
  xchr [ 95 ] = '_' ; 
  xchr [ 96 ] = '`' ; 
  xchr [ 97 ] = 'a' ; 
  xchr [ 98 ] = 'b' ; 
  xchr [ 99 ] = 'c' ; 
  xchr [ 100 ] = 'd' ; 
  xchr [ 101 ] = 'e' ; 
  xchr [ 102 ] = 'f' ; 
  xchr [ 103 ] = 'g' ; 
  xchr [ 104 ] = 'h' ; 
  xchr [ 105 ] = 'i' ; 
  xchr [ 106 ] = 'j' ; 
  xchr [ 107 ] = 'k' ; 
  xchr [ 108 ] = 'l' ; 
  xchr [ 109 ] = 'm' ; 
  xchr [ 110 ] = 'n' ; 
  xchr [ 111 ] = 'o' ; 
  xchr [ 112 ] = 'p' ; 
  xchr [ 113 ] = 'q' ; 
  xchr [ 114 ] = 'r' ; 
  xchr [ 115 ] = 's' ; 
  xchr [ 116 ] = 't' ; 
  xchr [ 117 ] = 'u' ; 
  xchr [ 118 ] = 'v' ; 
  xchr [ 119 ] = 'w' ; 
  xchr [ 120 ] = 'x' ; 
  xchr [ 121 ] = 'y' ; 
  xchr [ 122 ] = 'z' ; 
  xchr [ 123 ] = '{' ; 
  xchr [ 124 ] = '|' ; 
  xchr [ 125 ] = '}' ; 
  xchr [ 126 ] = '~' ; 
  {register integer for_end; i = 127 ; for_end = 255 ; if ( i <= for_end) do 
    xchr [ i ] = '?' ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 0 ; for_end = 255 ; if ( i <= for_end) do 
    xord [ chr ( i ) ] = 32 ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 32 ; for_end = 126 ; if ( i <= for_end) do 
    xord [ xchr [ i ] ] = i ; 
  while ( i++ < for_end ) ; } 
  history = 0 ; 
  pcktptr = 1 ; 
  byteptr = 1 ; 
  pcktstart [ 0 ] = 1 ; 
  pcktstart [ 1 ] = 1 ; 
  {register integer for_end; h = 0 ; for_end = 352 ; if ( h <= for_end) do 
    phash [ h ] = 0 ; 
  while ( h++ < for_end ) ; } 
  whash [ 0 ] = 1 ; 
  wlink [ 1 ] = 0 ; 
  widths [ 0 ] = 0 ; 
  widths [ 1 ] = 0 ; 
  nwidths = 2 ; 
  {register integer for_end; h = 1 ; for_end = 352 ; if ( h <= for_end) do 
    whash [ h ] = 0 ; 
  while ( h++ < for_end ) ; } 
  nchars = 0 ; 
  nf = 0 ; 
  curfnt = maxfonts ; 
  pcktmmsg = 0 ; 
  pcktsmsg = 0 ; 
  pcktdmsg = 0 ; 
  {register integer for_end; i = 0 ; for_end = 136 ; if ( i <= for_end) do 
    dvipar [ i ] = 0 ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 138 ; for_end = 255 ; if ( i <= for_end) do 
    dvipar [ i ] = 1 ; 
  while ( i++ < for_end ) ; } 
  dvipar [ 132 ] = 11 ; 
  dvipar [ 137 ] = 11 ; 
  dvipar [ 143 ] = 2 ; 
  dvipar [ 144 ] = 4 ; 
  dvipar [ 145 ] = 6 ; 
  dvipar [ 146 ] = 8 ; 
  {register integer for_end; i = 171 ; for_end = 234 ; if ( i <= for_end) do 
    dvipar [ i ] = 12 ; 
  while ( i++ < for_end ) ; } 
  dvipar [ 235 ] = 3 ; 
  dvipar [ 236 ] = 5 ; 
  dvipar [ 237 ] = 7 ; 
  dvipar [ 238 ] = 9 ; 
  dvipar [ 239 ] = 3 ; 
  dvipar [ 240 ] = 5 ; 
  dvipar [ 241 ] = 7 ; 
  dvipar [ 242 ] = 10 ; 
  {register integer for_end; i = 0 ; for_end = 3 ; if ( i <= for_end) do 
    {
      dvipar [ i + 148 ] = dvipar [ i + 143 ] ; 
      dvipar [ i + 153 ] = dvipar [ i + 143 ] ; 
      dvipar [ i + 157 ] = dvipar [ i + 143 ] ; 
      dvipar [ i + 162 ] = dvipar [ i + 143 ] ; 
      dvipar [ i + 167 ] = dvipar [ i + 143 ] ; 
      dvipar [ i + 243 ] = dvipar [ i + 235 ] ; 
    } 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 0 ; for_end = 136 ; if ( i <= for_end) do 
    dvicl [ i ] = 0 ; 
  while ( i++ < for_end ) ; } 
  dvicl [ 132 ] = 1 ; 
  dvicl [ 137 ] = 1 ; 
  dvicl [ 138 ] = 17 ; 
  dvicl [ 139 ] = 17 ; 
  dvicl [ 140 ] = 17 ; 
  dvicl [ 141 ] = 3 ; 
  dvicl [ 142 ] = 4 ; 
  dvicl [ 147 ] = 5 ; 
  dvicl [ 152 ] = 6 ; 
  dvicl [ 161 ] = 10 ; 
  dvicl [ 166 ] = 11 ; 
  {register integer for_end; i = 0 ; for_end = 3 ; if ( i <= for_end) do 
    {
      dvicl [ i + 143 ] = 7 ; 
      dvicl [ i + 148 ] = 8 ; 
      dvicl [ i + 153 ] = 9 ; 
      dvicl [ i + 157 ] = 12 ; 
      dvicl [ i + 162 ] = 13 ; 
      dvicl [ i + 167 ] = 14 ; 
      dvicl [ i + 239 ] = 2 ; 
      dvicl [ i + 243 ] = 16 ; 
    } 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 171 ; for_end = 238 ; if ( i <= for_end) do 
    dvicl [ i ] = 15 ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 247 ; for_end = 255 ; if ( i <= for_end) do 
    dvicl [ i ] = 17 ; 
  while ( i++ < for_end ) ; } 
  dvicharcmd [ false ] = 133 ; 
  dvicharcmd [ true ] = 128 ; 
  dvirulecmd [ false ] = 137 ; 
  dvirulecmd [ true ] = 132 ; 
  dvirightcmd [ 7 ] = 143 ; 
  dvirightcmd [ 8 ] = 148 ; 
  dvirightcmd [ 9 ] = 153 ; 
  dvidowncmd [ 12 ] = 157 ; 
  dvidowncmd [ 13 ] = 162 ; 
  dvidowncmd [ 14 ] = 167 ; 
  dvinf = 0 ; 
  lclnf = 0 ; 
  vfmove [ 0 ] [ 0 ] [ 0 ] = false ; 
  vfmove [ 0 ] [ 0 ] [ 1 ] = false ; 
  vfmove [ 0 ] [ 1 ] [ 0 ] = false ; 
  vfmove [ 0 ] [ 1 ] [ 1 ] = false ; 
  stackused = 0 ; 
  vfchartype [ false ] = 3 ; 
  vfchartype [ true ] = 0 ; 
  vfruletype [ false ] = 4 ; 
  vfruletype [ true ] = 1 ; 
  widthdimen = -1073741824L ; 
  widthdimen = widthdimen - 1073741824L ; 
  typesetting = false ; 
  zerostack .hfield = 0 ; 
  zerostack .vfield = 0 ; 
  {register integer for_end; i = 0 ; for_end = 1 ; if ( i <= for_end) do 
    {
      zerostack .wxfield [ i ] = 0 ; 
      zerostack .yzfield [ i ] = 0 ; 
    } 
  while ( i++ < for_end ) ; } 
  nrecur = 0 ; 
  recurused = 0 ; 
  outloc = 0 ; 
  outback = -1 ; 
  outmaxv = 0 ; 
  outmaxh = 0 ; 
  outstack = 0 ; 
  outpages = 0 ; 
  outnf = 0 ; 
} 
pcktpointer makepacket ( ) 
{/* 31 */ register pcktpointer Result; bytepointer i, k  ; 
  hashcode h  ; 
  bytepointer s, l  ; 
  pcktpointer p  ; 
  s = pcktstart [ pcktptr ] ; 
  l = byteptr - s ; 
  if ( l == 0 ) 
  p = 0 ; 
  else {
      
    h = bytemem [ s ] ; 
    i = s + 1 ; 
    while ( i < byteptr ) {
	
      h = ( h + h + bytemem [ i ] ) % 353 ; 
      incr ( i ) ; 
    } 
    p = phash [ h ] ; 
    while ( p != 0 ) {
	
      if ( ( pcktstart [ p + 1 ] - pcktstart [ p ] ) == l ) 
      {
	i = s ; 
	k = pcktstart [ p ] ; 
	while ( ( i < byteptr ) && ( bytemem [ i ] == bytemem [ k ] ) ) {
	    
	  incr ( i ) ; 
	  incr ( k ) ; 
	} 
	if ( i == byteptr ) 
	{
	  byteptr = pcktstart [ pcktptr ] ; 
	  goto lab31 ; 
	} 
      } 
      p = plink [ p ] ; 
    } 
    p = pcktptr ; 
    plink [ p ] = phash [ h ] ; 
    phash [ h ] = p ; 
    if ( pcktptr == maxpackets ) 
    overflow ( strpackets , maxpackets ) ; 
    incr ( pcktptr ) ; 
    pcktstart [ pcktptr ] = byteptr ; 
  } 
  lab31: Result = p ; 
  return(Result) ; 
} 
pcktpointer newpacket ( ) 
{register pcktpointer Result; if ( pcktptr == maxpackets ) 
  overflow ( strpackets , maxpackets ) ; 
  Result = pcktptr ; 
  incr ( pcktptr ) ; 
  pcktstart [ pcktptr ] = byteptr ; 
  return(Result) ; 
} 
void flushpacket ( ) 
{decr ( pcktptr ) ; 
  byteptr = pcktstart [ pcktptr ] ; 
} 
int8 pcktsbyte ( ) 
{register int8 Result; eightbits a  ; 
  {
    a = bytemem [ curloc ] ; 
    incr ( curloc ) ; 
  } 
  if ( a < 128 ) 
  Result = a ; 
  else Result = a - 256 ; 
  return(Result) ; 
} 
int8u pcktubyte ( ) 
{register int8u Result; eightbits a  ; 
  {
    a = bytemem [ curloc ] ; 
    incr ( curloc ) ; 
  } 
  Result = a ; 
  return(Result) ; 
} 
int16 pcktspair ( ) 
{register int16 Result; eightbits a, b  ; 
  {
    a = bytemem [ curloc ] ; 
    incr ( curloc ) ; 
  } 
  {
    b = bytemem [ curloc ] ; 
    incr ( curloc ) ; 
  } 
  if ( a < 128 ) 
  Result = a * toint ( 256 ) + b ; 
  else Result = ( a - toint ( 256 ) ) * toint ( 256 ) + b ; 
  return(Result) ; 
} 
int16u pcktupair ( ) 
{register int16u Result; eightbits a, b  ; 
  {
    a = bytemem [ curloc ] ; 
    incr ( curloc ) ; 
  } 
  {
    b = bytemem [ curloc ] ; 
    incr ( curloc ) ; 
  } 
  Result = a * toint ( 256 ) + b ; 
  return(Result) ; 
} 
int24 pcktstrio ( ) 
{register int24 Result; eightbits a, b, c  ; 
  {
    a = bytemem [ curloc ] ; 
    incr ( curloc ) ; 
  } 
  {
    b = bytemem [ curloc ] ; 
    incr ( curloc ) ; 
  } 
  {
    c = bytemem [ curloc ] ; 
    incr ( curloc ) ; 
  } 
  if ( a < 128 ) 
  Result = ( a * toint ( 256 ) + b ) * toint ( 256 ) + c ; 
  else Result = ( ( a - toint ( 256 ) ) * toint ( 256 ) + b ) * toint ( 256 ) 
  + c ; 
  return(Result) ; 
} 
int24u pcktutrio ( ) 
{register int24u Result; eightbits a, b, c  ; 
  {
    a = bytemem [ curloc ] ; 
    incr ( curloc ) ; 
  } 
  {
    b = bytemem [ curloc ] ; 
    incr ( curloc ) ; 
  } 
  {
    c = bytemem [ curloc ] ; 
    incr ( curloc ) ; 
  } 
  Result = ( a * toint ( 256 ) + b ) * toint ( 256 ) + c ; 
  return(Result) ; 
} 
integer pcktsquad ( ) 
{register integer Result; eightbits a, b, c, d  ; 
  {
    a = bytemem [ curloc ] ; 
    incr ( curloc ) ; 
  } 
  {
    b = bytemem [ curloc ] ; 
    incr ( curloc ) ; 
  } 
  {
    c = bytemem [ curloc ] ; 
    incr ( curloc ) ; 
  } 
  {
    d = bytemem [ curloc ] ; 
    incr ( curloc ) ; 
  } 
  if ( a < 128 ) 
  Result = ( ( a * toint ( 256 ) + b ) * toint ( 256 ) + c ) * toint ( 256 ) + 
  d ; 
  else Result = ( ( ( a - toint ( 256 ) ) * toint ( 256 ) + b ) * toint ( 256 
  ) + c ) * toint ( 256 ) + d ; 
  return(Result) ; 
} 
void zpcktfour ( x ) 
integer x ; 
{; 
  if ( maxbytes - byteptr < 4 ) 
  overflow ( strbytes , maxbytes ) ; 
  if ( x >= 0 ) 
  {
    bytemem [ byteptr ] = x / 16777216L ; 
    incr ( byteptr ) ; 
  } 
  else {
      
    x = x + 1073741824L ; 
    x = x + 1073741824L ; 
    {
      bytemem [ byteptr ] = ( x / 16777216L ) + 128 ; 
      incr ( byteptr ) ; 
    } 
  } 
  x = x % 16777216L ; 
  {
    bytemem [ byteptr ] = x / 65536L ; 
    incr ( byteptr ) ; 
  } 
  x = x % 65536L ; 
  {
    bytemem [ byteptr ] = x / 256 ; 
    incr ( byteptr ) ; 
  } 
  {
    bytemem [ byteptr ] = x % 256 ; 
    incr ( byteptr ) ; 
  } 
} 
void zpcktchar ( upd , ext , res ) 
boolean upd ; 
integer ext ; 
eightbits res ; 
{eightbits o  ; 
  if ( maxbytes - byteptr < 5 ) 
  overflow ( strbytes , maxbytes ) ; 
  if ( ( ! upd ) || ( res > 127 ) || ( ext != 0 ) ) 
  {
    o = dvicharcmd [ upd ] ; 
    if ( ext < 0 ) 
    ext = ext + 16777216L ; 
    if ( ext == 0 ) 
    {
      bytemem [ byteptr ] = o ; 
      incr ( byteptr ) ; 
    } 
    else {
	
      if ( ext < 256 ) 
      {
	bytemem [ byteptr ] = o + 1 ; 
	incr ( byteptr ) ; 
      } 
      else {
	  
	if ( ext < 65536L ) 
	{
	  bytemem [ byteptr ] = o + 2 ; 
	  incr ( byteptr ) ; 
	} 
	else {
	    
	  {
	    bytemem [ byteptr ] = o + 3 ; 
	    incr ( byteptr ) ; 
	  } 
	  {
	    bytemem [ byteptr ] = ext / 65536L ; 
	    incr ( byteptr ) ; 
	  } 
	  ext = ext % 65536L ; 
	} 
	{
	  bytemem [ byteptr ] = ext / 256 ; 
	  incr ( byteptr ) ; 
	} 
	ext = ext % 256 ; 
      } 
      {
	bytemem [ byteptr ] = ext ; 
	incr ( byteptr ) ; 
      } 
    } 
  } 
  {
    bytemem [ byteptr ] = res ; 
    incr ( byteptr ) ; 
  } 
} 
void zpcktunsigned ( o , x ) 
eightbits o ; 
integer x ; 
{; 
  if ( maxbytes - byteptr < 5 ) 
  overflow ( strbytes , maxbytes ) ; 
  if ( ( x < 256 ) && ( x >= 0 ) ) 
  if ( ( o == 235 ) && ( x < 64 ) ) 
  x = x + 171 ; 
  else {
      
    bytemem [ byteptr ] = o ; 
    incr ( byteptr ) ; 
  } 
  else {
      
    if ( ( x < 65536L ) && ( x >= 0 ) ) 
    {
      bytemem [ byteptr ] = o + 1 ; 
      incr ( byteptr ) ; 
    } 
    else {
	
      if ( ( x < 16777216L ) && ( x >= 0 ) ) 
      {
	bytemem [ byteptr ] = o + 2 ; 
	incr ( byteptr ) ; 
      } 
      else {
	  
	{
	  bytemem [ byteptr ] = o + 3 ; 
	  incr ( byteptr ) ; 
	} 
	if ( x >= 0 ) 
	{
	  bytemem [ byteptr ] = x / 16777216L ; 
	  incr ( byteptr ) ; 
	} 
	else {
	    
	  x = x + 1073741824L ; 
	  x = x + 1073741824L ; 
	  {
	    bytemem [ byteptr ] = ( x / 16777216L ) + 128 ; 
	    incr ( byteptr ) ; 
	  } 
	} 
	x = x % 16777216L ; 
      } 
      {
	bytemem [ byteptr ] = x / 65536L ; 
	incr ( byteptr ) ; 
      } 
      x = x % 65536L ; 
    } 
    {
      bytemem [ byteptr ] = x / 256 ; 
      incr ( byteptr ) ; 
    } 
    x = x % 256 ; 
  } 
  {
    bytemem [ byteptr ] = x ; 
    incr ( byteptr ) ; 
  } 
} 
void zpcktsigned ( o , x ) 
eightbits o ; 
integer x ; 
{int31 xx  ; 
  if ( maxbytes - byteptr < 5 ) 
  overflow ( strbytes , maxbytes ) ; 
  if ( x >= 0 ) 
  xx = x ; 
  else xx = - (integer) ( x + 1 ) ; 
  if ( xx < 128 ) 
  {
    {
      bytemem [ byteptr ] = o ; 
      incr ( byteptr ) ; 
    } 
    if ( x < 0 ) 
    x = x + 256 ; 
  } 
  else {
      
    if ( xx < 32768L ) 
    {
      {
	bytemem [ byteptr ] = o + 1 ; 
	incr ( byteptr ) ; 
      } 
      if ( x < 0 ) 
      x = x + 65536L ; 
    } 
    else {
	
      if ( xx < 8388608L ) 
      {
	{
	  bytemem [ byteptr ] = o + 2 ; 
	  incr ( byteptr ) ; 
	} 
	if ( x < 0 ) 
	x = x + 16777216L ; 
      } 
      else {
	  
	{
	  bytemem [ byteptr ] = o + 3 ; 
	  incr ( byteptr ) ; 
	} 
	if ( x >= 0 ) 
	{
	  bytemem [ byteptr ] = x / 16777216L ; 
	  incr ( byteptr ) ; 
	} 
	else {
	    
	  x = 2147483647L - xx ; 
	  {
	    bytemem [ byteptr ] = ( x / 16777216L ) + 128 ; 
	    incr ( byteptr ) ; 
	  } 
	} 
	x = x % 16777216L ; 
      } 
      {
	bytemem [ byteptr ] = x / 65536L ; 
	incr ( byteptr ) ; 
      } 
      x = x % 65536L ; 
    } 
    {
      bytemem [ byteptr ] = x / 256 ; 
      incr ( byteptr ) ; 
    } 
    x = x % 256 ; 
  } 
  {
    bytemem [ byteptr ] = x ; 
    incr ( byteptr ) ; 
  } 
} 
void zmakename ( e ) 
pcktpointer e ; 
{eightbits b  ; 
  pcktpointer n  ; 
  bytepointer curloc, curlimit  ; 
  char c  ; 
  n = fntname [ curfnt ] ; 
  curloc = pcktstart [ n ] ; 
  curlimit = pcktstart [ n + 1 ] ; 
  {
    b = bytemem [ curloc ] ; 
    incr ( curloc ) ; 
  } 
  if ( b > 0 ) 
  curnamelength = 0 ; 
  while ( curloc < curlimit ) {
      
    {
      b = bytemem [ curloc ] ; 
      incr ( curloc ) ; 
    } 
    if ( curnamelength < PATHMAX ) 
    {
      incr ( curnamelength ) ; 
      curname [ curnamelength ] = xchr [ b ] ; 
    } 
    else overflow ( strnamelength , PATHMAX ) ; 
  } 
  curloc = pcktstart [ e ] ; 
  curlimit = pcktstart [ e + 1 ] ; 
  while ( curloc < curlimit ) {
      
    {
      b = bytemem [ curloc ] ; 
      incr ( curloc ) ; 
    } 
    {
      c = xchr [ b ] ; 
      if ( curnamelength < PATHMAX ) 
      {
	incr ( curnamelength ) ; 
	curname [ curnamelength ] = c ; 
      } 
      else overflow ( strnamelength , PATHMAX ) ; 
    } 
  } 
  while ( curnamelength < PATHMAX ) {
      
    incr ( curnamelength ) ; 
    curname [ curnamelength ] = ' ' ; 
  } 
} 
widthpointer zmakewidth ( w ) 
integer w ; 
{/* 31 */ register widthpointer Result; hashcode h  ; 
  widthpointer p  ; 
  int16 x  ; 
  widths [ nwidths ] = w ; 
  if ( w >= 0 ) 
  x = w / 16777216L ; 
  else {
      
    w = w + 1073741824L ; 
    w = w + 1073741824L ; 
    x = ( w / 16777216L ) + 128 ; 
  } 
  w = w % 16777216L ; 
  x = x + x + ( w / 65536L ) ; 
  w = w % 65536L ; 
  x = x + x + ( w / 256 ) ; 
  h = ( x + x + ( w % 256 ) ) % 353 ; 
  p = whash [ h ] ; 
  while ( p != 0 ) {
      
    if ( widths [ p ] == widths [ nwidths ] ) 
    goto lab31 ; 
    p = wlink [ p ] ; 
  } 
  p = nwidths ; 
  wlink [ p ] = whash [ h ] ; 
  whash [ h ] = p ; 
  if ( nwidths == maxwidths ) 
  overflow ( strwidths , maxwidths ) ; 
  incr ( nwidths ) ; 
  lab31: Result = p ; 
  return(Result) ; 
} 
boolean findpacket ( ) 
{/* 31 10 */ register boolean Result; pcktpointer p, q  ; 
  eightbits f  ; 
  int24 e  ; 
  q = charpackets [ fntchars [ curfnt ] + curres ] ; 
  while ( q != maxpackets ) {
      
    p = q ; 
    q = maxpackets ; 
    curloc = pcktstart [ p ] ; 
    curlimit = pcktstart [ p + 1 ] ; 
    if ( p == 0 ) 
    {
      e = 0 ; 
      f = 0 ; 
    } 
    else {
	
      {
	f = bytemem [ curloc ] ; 
	incr ( curloc ) ; 
      } 
      switch ( ( f / 64 ) ) 
      {case 0 : 
	e = 0 ; 
	break ; 
      case 1 : 
	e = pcktubyte () ; 
	break ; 
      case 2 : 
	e = pcktupair () ; 
	break ; 
      case 3 : 
	e = pcktstrio () ; 
	break ; 
      } 
      if ( ( f % 64 ) >= 32 ) 
      q = pcktupair () ; 
      f = f % 32 ; 
    } 
    if ( e == curext ) 
    goto lab31 ; 
  } 
  if ( charpackets [ fntchars [ curfnt ] + curres ] == maxpackets ) 
  {
    if ( pcktmmsg < 10 ) 
    {
      (void) fprintf( stdout , "%s%ld%s%ld\n",  "---missing character packet for character " , (long)curres       , " font " , (long)curfnt ) ; 
      incr ( pcktmmsg ) ; 
      history = 2 ; 
      if ( pcktmmsg == 10 ) 
      (void) fprintf( stdout , "%s\n",  "---further messages suppressed." ) ; 
    } 
    Result = false ; 
    goto lab10 ; 
  } 
  if ( pcktsmsg < 10 ) 
  {
    (void) fprintf( stdout , "%s%ld%s%ld%s%ld%s%ld\n",  "---substituted character packet with extension " , (long)e ,     " instead of " , (long)curext , " for character " , (long)curres , " font " , (long)curfnt ) 
    ; 
    incr ( pcktsmsg ) ; 
    history = 2 ; 
    if ( pcktsmsg == 10 ) 
    (void) fprintf( stdout , "%s\n",  "---further messages suppressed." ) ; 
  } 
  curext = e ; 
  lab31: curpckt = p ; 
  curtype = f ; 
  Result = true ; 
  lab10: ; 
  return(Result) ; 
} 
void zstartpacket ( t ) 
typeflag t ; 
{/* 31 32 */ pcktpointer p, q  ; 
  int8u f  ; 
  integer e  ; 
  bytepointer curloc  ; 
  bytepointer curlimit  ; 
  q = charpackets [ fntchars [ curfnt ] + curres ] ; 
  while ( q != maxpackets ) {
      
    p = q ; 
    q = maxpackets ; 
    curloc = pcktstart [ p ] ; 
    curlimit = pcktstart [ p + 1 ] ; 
    if ( p == 0 ) 
    {
      e = 0 ; 
      f = 0 ; 
    } 
    else {
	
      {
	f = bytemem [ curloc ] ; 
	incr ( curloc ) ; 
      } 
      switch ( ( f / 64 ) ) 
      {case 0 : 
	e = 0 ; 
	break ; 
      case 1 : 
	e = pcktubyte () ; 
	break ; 
      case 2 : 
	e = pcktupair () ; 
	break ; 
      case 3 : 
	e = pcktstrio () ; 
	break ; 
      } 
      if ( ( f % 64 ) >= 32 ) 
      q = pcktupair () ; 
      f = f % 32 ; 
    } 
    if ( e == curext ) 
    goto lab31 ; 
  } 
  q = charpackets [ fntchars [ curfnt ] + curres ] ; 
  pcktdup = false ; 
  goto lab32 ; 
  lab31: pcktdup = true ; 
  pcktprev = p ; 
  lab32: pcktext = curext ; 
  pcktres = curres ; 
  if ( maxbytes - byteptr < 6 ) 
  overflow ( strbytes , maxbytes ) ; 
  if ( q == maxpackets ) 
  f = t ; 
  else f = t + 32 ; 
  e = curext ; 
  if ( e < 0 ) 
  e = e + 16777216L ; 
  if ( e == 0 ) 
  {
    bytemem [ byteptr ] = f ; 
    incr ( byteptr ) ; 
  } 
  else {
      
    if ( e < 256 ) 
    {
      bytemem [ byteptr ] = f + 64 ; 
      incr ( byteptr ) ; 
    } 
    else {
	
      if ( e < 65536L ) 
      {
	bytemem [ byteptr ] = f + 128 ; 
	incr ( byteptr ) ; 
      } 
      else {
	  
	{
	  bytemem [ byteptr ] = f + 192 ; 
	  incr ( byteptr ) ; 
	} 
	{
	  bytemem [ byteptr ] = e / 65536L ; 
	  incr ( byteptr ) ; 
	} 
	e = e % 65536L ; 
      } 
      {
	bytemem [ byteptr ] = e / 256 ; 
	incr ( byteptr ) ; 
      } 
      e = e % 256 ; 
    } 
    {
      bytemem [ byteptr ] = e ; 
      incr ( byteptr ) ; 
    } 
  } 
  if ( q != maxpackets ) 
  {
    {
      bytemem [ byteptr ] = q / 256 ; 
      incr ( byteptr ) ; 
    } 
    {
      bytemem [ byteptr ] = q % 256 ; 
      incr ( byteptr ) ; 
    } 
  } 
} 
void buildpacket ( ) 
{bytepointer k, l  ; 
  if ( pcktdup ) 
  {
    k = pcktstart [ pcktprev + 1 ] ; 
    l = pcktstart [ pcktptr ] ; 
    if ( ( byteptr - l ) != ( k - pcktstart [ pcktprev ] ) ) 
    pcktdup = false ; 
    while ( pcktdup && ( byteptr > l ) ) {
	
      decr ( byteptr ) ; 
      decr ( k ) ; 
      if ( bytemem [ byteptr ] != bytemem [ k ] ) 
      pcktdup = false ; 
    } 
    if ( ( ! pcktdup ) && ( pcktdmsg < 10 ) ) 
    {
      (void) fprintf( stdout , "%s%ld",  "---duplicate packet for character " , (long)pcktres ) ; 
      if ( pcktext != 0 ) 
      (void) fprintf( stdout , "%c%ld",  '.' , (long)pcktext ) ; 
      (void) fprintf( stdout , "%s%ld\n",  " font " , (long)curfnt ) ; 
      incr ( pcktdmsg ) ; 
      history = 2 ; 
      if ( pcktdmsg == 10 ) 
      (void) fprintf( stdout , "%s\n",  "---further messages suppressed." ) ; 
    } 
    byteptr = l ; 
  } 
  else charpackets [ fntchars [ curfnt ] + pcktres ] = makepacket () ; 
} 
void readtfmword ( ) 
{read ( tfmfile , tfmb0 ) ; 
  read ( tfmfile , tfmb1 ) ; 
  read ( tfmfile , tfmb2 ) ; 
  read ( tfmfile , tfmb3 ) ; 
  if ( eof ( tfmfile ) ) 
  badfont () ; 
} 
void zcheckchecksum ( c , u ) 
integer c ; 
boolean u ; 
{if ( ( c != fntcheck [ curfnt ] ) && ( c != 0 ) ) 
  {
    if ( fntcheck [ curfnt ] != 0 ) 
    {
      (void) putc('\n',  stdout );
      (void) fprintf( stdout , "%s%ld%s%ld%c\n",  "---beware: check sums do not agree!   (" , (long)c ,       " vs. " , (long)fntcheck [ curfnt ] , ')' ) ; 
      history = 2 ; 
    } 
    if ( u ) 
    fntcheck [ curfnt ] = c ; 
  } 
} 
void zcheckdesignsize ( d ) 
integer d ; 
{if ( abs ( d - fntdesign [ curfnt ] ) > 2 ) 
  {
    (void) putc('\n',  stdout );
    (void) fprintf( stdout , "%s%ld%s%ld%c\n",  "---beware: design sizes do not agree!   (" , (long)d ,     " vs. " , (long)fntdesign [ curfnt ] , ')' ) ; 
    history = 2 ; 
  } 
} 
widthpointer zcheckwidth ( w ) 
integer w ; 
{register widthpointer Result; widthpointer wp  ; 
  if ( ( curres >= fntbc [ curfnt ] ) && ( curres <= fntec [ curfnt ] ) ) 
  wp = charwidths [ fntchars [ curfnt ] + curres ] ; 
  else wp = 0 ; 
  if ( wp == 0 ) 
  {
    {
      (void) putc('\n',  stdout );
      (void) fprintf( stdout , "%s%ld",  "Bad char " , (long)curres ) ; 
    } 
    if ( curext != 0 ) 
    (void) fprintf( stdout , "%c%ld",  '.' , (long)curext ) ; 
    (void) fprintf( stdout , "%s%ld",  " font " , (long)curfnt ) ; 
    printfont ( curfnt ) ; 
    {
      (void) fprintf( stdout , "%c%s%c\n",  ' ' , " (compare TFM file)" , '.' ) ; 
      jumpout () ; 
    } 
  } 
  if ( w != widths [ wp ] ) 
  {
    (void) putc('\n',  stdout );
    (void) fprintf( stdout , "%s%ld%s%ld%c\n",  "---beware: char widths do not agree!   (" , (long)w ,     " vs. " , (long)widths [ wp ] , ')' ) ; 
    history = 2 ; 
  } 
  Result = wp ; 
  return(Result) ; 
} 
fontnumber makefont ( ) 
{register fontnumber Result; int16 l  ; 
  charpointer p  ; 
  widthpointer q  ; 
  int15 bc, ec  ; 
  int15 lh  ; 
  int15 nw  ; 
  integer w  ; 
  fontnumber savefnt  ; 
  integer z  ; 
  integer alpha  ; 
  int15 beta  ; 
  savefnt = curfnt ; 
  curfnt = 0 ; 
  while ( ( fntname [ curfnt ] != fntname [ nf ] ) || ( fntscaled [ curfnt ] 
  != fntscaled [ nf ] ) ) incr ( curfnt ) ; 
  printfont ( curfnt ) ; 
  if ( curfnt < nf ) 
  {
    checkchecksum ( fntcheck [ nf ] , true ) ; 
    checkdesignsize ( fntdesign [ nf ] ) ; 
  } 
  else {
      
    if ( nf == maxfonts ) 
    overflow ( strfonts , maxfonts ) ; 
    fnttype [ curfnt ] = 0 ; 
    fntfont [ curfnt ] = maxfonts ; 
    curnamelength = 0 ; 
    makename ( tfmext ) ; 
    if ( testreadaccess ( curname , TFMFILEPATH ) ) 
    {
      reset ( tfmfile , curname ) ; 
    } 
    else {
	
      printpascalstring ( curname ) ; 
      {
	(void) fprintf( stdout , "%c%s%c\n",  ' ' , "---not loaded, TFM file can't be opened!" ,         '.' ) ; 
	jumpout () ; 
      } 
    } 
    readtfmword () ; 
    if ( tfmb2 > 127 ) 
    badfont () ; 
    else lh = tfmb2 * toint ( 256 ) + tfmb3 ; 
    readtfmword () ; 
    if ( tfmb0 > 127 ) 
    badfont () ; 
    else bc = tfmb0 * toint ( 256 ) + tfmb1 ; 
    if ( tfmb2 > 127 ) 
    badfont () ; 
    else ec = tfmb2 * toint ( 256 ) + tfmb3 ; 
    if ( ec < bc ) 
    {
      bc = 1 ; 
      ec = 0 ; 
    } 
    else if ( ec > 255 ) 
    badfont () ; 
    readtfmword () ; 
    if ( tfmb0 > 127 ) 
    badfont () ; 
    else nw = tfmb0 * toint ( 256 ) + tfmb1 ; 
    if ( ( nw == 0 ) || ( nw > 256 ) ) 
    badfont () ; 
    {register integer for_end; l = -2 ; for_end = lh ; if ( l <= for_end) do 
      {
	readtfmword () ; 
	if ( l == 1 ) 
	{
	  if ( tfmb0 < 128 ) 
	  w = ( ( tfmb0 * toint ( 256 ) + tfmb1 ) * toint ( 256 ) + tfmb2 ) * 
	  toint ( 256 ) + tfmb3 ; 
	  else w = ( ( ( tfmb0 - toint ( 256 ) ) * toint ( 256 ) + tfmb1 ) * 
	  toint ( 256 ) + tfmb2 ) * toint ( 256 ) + tfmb3 ; 
	  checkchecksum ( w , true ) ; 
	} 
	else if ( l == 2 ) 
	{
	  if ( tfmb0 > 127 ) 
	  badfont () ; 
	  checkdesignsize ( round ( tfmconv * ( ( ( tfmb0 * toint ( 256 ) + 
	  tfmb1 ) * toint ( 256 ) + tfmb2 ) * toint ( 256 ) + tfmb3 ) ) ) ; 
	} 
      } 
    while ( l++ < for_end ) ; } 
    readtfmword () ; 
    while ( ( tfmb0 == 0 ) && ( bc <= ec ) ) {
	
      incr ( bc ) ; 
      readtfmword () ; 
    } 
    fntbc [ curfnt ] = bc ; 
    fntchars [ curfnt ] = nchars - bc ; 
    if ( ec >= maxchars - fntchars [ curfnt ] ) 
    overflow ( strchars , maxchars ) ; 
    {register integer for_end; l = bc ; for_end = ec ; if ( l <= for_end) do 
      {
	charwidths [ nchars ] = tfmb0 ; 
	incr ( nchars ) ; 
	readtfmword () ; 
      } 
    while ( l++ < for_end ) ; } 
    while ( ( charwidths [ nchars - 1 ] == 0 ) && ( ec >= bc ) ) {
	
      decr ( nchars ) ; 
      decr ( ec ) ; 
    } 
    fntec [ curfnt ] = ec ; 
    if ( nw - 1 > maxchars - nchars ) 
    overflow ( strchars , maxchars ) ; 
    if ( ( tfmb0 != 0 ) || ( tfmb1 != 0 ) || ( tfmb2 != 0 ) || ( tfmb3 != 0 ) 
    ) 
    badfont () ; 
    else charwidths [ nchars ] = 0 ; 
    z = fntscaled [ curfnt ] ; 
    alpha = 16 ; 
    while ( z >= 8388608L ) {
	
      z = z / 2 ; 
      alpha = alpha + alpha ; 
    } 
    beta = 256 / alpha ; 
    alpha = alpha * z ; 
    {register integer for_end; p = nchars + 1 ; for_end = nchars + nw - 1 
    ; if ( p <= for_end) do 
      {
	readtfmword () ; 
	w = ( ( ( ( ( tfmb3 * z ) / 256 ) + ( tfmb2 * z ) ) / 256 ) + ( tfmb1 
	* z ) ) / beta ; 
	if ( tfmb0 > 0 ) 
	if ( tfmb0 == 255 ) 
	w = w - alpha ; 
	else badfont () ; 
	charwidths [ p ] = makewidth ( w ) ; 
      } 
    while ( p++ < for_end ) ; } 
    {register integer for_end; p = fntchars [ curfnt ] + bc ; for_end = 
    nchars - 1 ; if ( p <= for_end) do 
      {
	q = charwidths [ nchars + charwidths [ p ] ] ; 
	charwidths [ p ] = q ; 
	charpackets [ p ] = maxpackets ; 
      } 
    while ( p++ < for_end ) ; } 
    incr ( nf ) ; 
  } 
  (void) putc('\n',  stdout );
  Result = curfnt ; 
  curfnt = savefnt ; 
  return(Result) ; 
} 
integer dvilength ( ) 
{register integer Result; checkedfseek ( dvifile , 0 , 2 ) ; 
  dviloc = ftell ( dvifile ) ; 
  Result = dviloc ; 
  return(Result) ; 
} 
void zdvimove ( n ) 
integer n ; 
{checkedfseek ( dvifile , n , 0 ) ; 
  dviloc = n ; 
} 
int8 dvisbyte ( ) 
{register int8 Result; eightbits a  ; 
  if ( eof ( dvifile ) ) 
  baddvi () ; 
  else read ( dvifile , a ) ; 
  incr ( dviloc ) ; 
  if ( a < 128 ) 
  Result = a ; 
  else Result = a - 256 ; 
  return(Result) ; 
} 
int8u dviubyte ( ) 
{register int8u Result; eightbits a  ; 
  if ( eof ( dvifile ) ) 
  baddvi () ; 
  else read ( dvifile , a ) ; 
  incr ( dviloc ) ; 
  Result = a ; 
  return(Result) ; 
} 
int16 dvispair ( ) 
{register int16 Result; eightbits a, b  ; 
  if ( eof ( dvifile ) ) 
  baddvi () ; 
  else read ( dvifile , a ) ; 
  if ( eof ( dvifile ) ) 
  baddvi () ; 
  else read ( dvifile , b ) ; 
  dviloc = dviloc + 2 ; 
  if ( a < 128 ) 
  Result = a * toint ( 256 ) + b ; 
  else Result = ( a - toint ( 256 ) ) * toint ( 256 ) + b ; 
  return(Result) ; 
} 
int16u dviupair ( ) 
{register int16u Result; eightbits a, b  ; 
  if ( eof ( dvifile ) ) 
  baddvi () ; 
  else read ( dvifile , a ) ; 
  if ( eof ( dvifile ) ) 
  baddvi () ; 
  else read ( dvifile , b ) ; 
  dviloc = dviloc + 2 ; 
  Result = a * toint ( 256 ) + b ; 
  return(Result) ; 
} 
int24 dvistrio ( ) 
{register int24 Result; eightbits a, b, c  ; 
  if ( eof ( dvifile ) ) 
  baddvi () ; 
  else read ( dvifile , a ) ; 
  if ( eof ( dvifile ) ) 
  baddvi () ; 
  else read ( dvifile , b ) ; 
  if ( eof ( dvifile ) ) 
  baddvi () ; 
  else read ( dvifile , c ) ; 
  dviloc = dviloc + 3 ; 
  if ( a < 128 ) 
  Result = ( a * toint ( 256 ) + b ) * toint ( 256 ) + c ; 
  else Result = ( ( a - toint ( 256 ) ) * toint ( 256 ) + b ) * toint ( 256 ) 
  + c ; 
  return(Result) ; 
} 
int24u dviutrio ( ) 
{register int24u Result; eightbits a, b, c  ; 
  if ( eof ( dvifile ) ) 
  baddvi () ; 
  else read ( dvifile , a ) ; 
  if ( eof ( dvifile ) ) 
  baddvi () ; 
  else read ( dvifile , b ) ; 
  if ( eof ( dvifile ) ) 
  baddvi () ; 
  else read ( dvifile , c ) ; 
  dviloc = dviloc + 3 ; 
  Result = ( a * toint ( 256 ) + b ) * toint ( 256 ) + c ; 
  return(Result) ; 
} 
integer dvisquad ( ) 
{register integer Result; eightbits a, b, c, d  ; 
  if ( eof ( dvifile ) ) 
  baddvi () ; 
  else read ( dvifile , a ) ; 
  if ( eof ( dvifile ) ) 
  baddvi () ; 
  else read ( dvifile , b ) ; 
  if ( eof ( dvifile ) ) 
  baddvi () ; 
  else read ( dvifile , c ) ; 
  if ( eof ( dvifile ) ) 
  baddvi () ; 
  else read ( dvifile , d ) ; 
  dviloc = dviloc + 4 ; 
  if ( a < 128 ) 
  Result = ( ( a * toint ( 256 ) + b ) * toint ( 256 ) + c ) * toint ( 256 ) + 
  d ; 
  else Result = ( ( ( a - toint ( 256 ) ) * toint ( 256 ) + b ) * toint ( 256 
  ) + c ) * toint ( 256 ) + d ; 
  return(Result) ; 
} 
int31 dviuquad ( ) 
{register int31 Result; integer x  ; 
  x = dvisquad () ; 
  if ( x < 0 ) 
  baddvi () ; 
  else Result = x ; 
  return(Result) ; 
} 
int31 dvipquad ( ) 
{register int31 Result; integer x  ; 
  x = dvisquad () ; 
  if ( x <= 0 ) 
  baddvi () ; 
  else Result = x ; 
  return(Result) ; 
} 
integer dvipointer ( ) 
{register integer Result; integer x  ; 
  x = dvisquad () ; 
  if ( ( x <= 0 ) && ( x != -1 ) ) 
  baddvi () ; 
  else Result = x ; 
  return(Result) ; 
} 
void dvifirstpar ( ) 
{do {
    curcmd = dviubyte () ; 
  } while ( ! ( curcmd != 138 ) ) ; 
  switch ( dvipar [ curcmd ] ) 
  {case 0 : 
    {
      curext = 0 ; 
      if ( curcmd < 128 ) 
      {
	curres = curcmd ; 
	curupd = true ; 
      } 
      else {
	  
	curres = dviubyte () ; 
	curupd = ( curcmd < 133 ) ; 
	curcmd = curcmd - dvicharcmd [ curupd ] ; 
	while ( curcmd > 0 ) {
	    
	  if ( curcmd == 3 ) 
	  if ( curres > 127 ) 
	  curext = -1 ; 
	  curext = curext * 256 + curres ; 
	  curres = dviubyte () ; 
	  decr ( curcmd ) ; 
	} 
      } 
    } 
    break ; 
  case 1 : 
    ; 
    break ; 
  case 2 : 
    curparm = dvisbyte () ; 
    break ; 
  case 3 : 
    curparm = dviubyte () ; 
    break ; 
  case 4 : 
    curparm = dvispair () ; 
    break ; 
  case 5 : 
    curparm = dviupair () ; 
    break ; 
  case 6 : 
    curparm = dvistrio () ; 
    break ; 
  case 7 : 
    curparm = dviutrio () ; 
    break ; 
  case 8 : 
  case 9 : 
    curparm = dvisquad () ; 
    break ; 
  case 10 : 
    curparm = dviuquad () ; 
    break ; 
  case 11 : 
    {
      curvdimen = dvisquad () ; 
      curhdimen = dvisquad () ; 
      curupd = ( curcmd == 132 ) ; 
    } 
    break ; 
  case 12 : 
    curparm = curcmd - 171 ; 
    break ; 
  } 
  curclass = dvicl [ curcmd ] ; 
} 
void dvifont ( ) 
{fontnumber f  ; 
  f = 0 ; 
  dviefnts [ dvinf ] = curparm ; 
  while ( curparm != dviefnts [ f ] ) incr ( f ) ; 
  if ( f == dvinf ) 
  baddvi () ; 
  curfnt = dviifnts [ f ] ; 
} 
void zdvidofont ( second ) 
boolean second ; 
{fontnumber f  ; 
  int15 k  ; 
  (void) fprintf( stdout , "%s%ld",  "DVI: font " , (long)curparm ) ; 
  f = 0 ; 
  dviefnts [ dvinf ] = curparm ; 
  while ( curparm != dviefnts [ f ] ) incr ( f ) ; 
  if ( ( f == dvinf ) == second ) 
  baddvi () ; 
  fntcheck [ nf ] = dvisquad () ; 
  fntscaled [ nf ] = dvipquad () ; 
  fntdesign [ nf ] = dvipquad () ; 
  k = dviubyte () ; 
  if ( maxbytes - byteptr < 1 ) 
  overflow ( strbytes , maxbytes ) ; 
  {
    bytemem [ byteptr ] = k ; 
    incr ( byteptr ) ; 
  } 
  k = k + dviubyte () ; 
  if ( maxbytes - byteptr < k ) 
  overflow ( strbytes , maxbytes ) ; 
  while ( k > 0 ) {
      
    {
      bytemem [ byteptr ] = dviubyte () ; 
      incr ( byteptr ) ; 
    } 
    decr ( k ) ; 
  } 
  fntname [ nf ] = makepacket () ; 
  dviifnts [ dvinf ] = makefont () ; 
  if ( ! second ) 
  {
    if ( dvinf == maxfonts ) 
    overflow ( strfonts , maxfonts ) ; 
    incr ( dvinf ) ; 
  } 
  else if ( dviifnts [ f ] != dviifnts [ dvinf ] ) 
  baddvi () ; 
} 
int8u vfubyte ( ) 
{register int8u Result; eightbits a  ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , a ) ; 
  incr ( vfloc ) ; 
  Result = a ; 
  return(Result) ; 
} 
int16u vfupair ( ) 
{register int16u Result; eightbits a, b  ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , a ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , b ) ; 
  vfloc = vfloc + 2 ; 
  Result = a * toint ( 256 ) + b ; 
  return(Result) ; 
} 
int24 vfstrio ( ) 
{register int24 Result; eightbits a, b, c  ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , a ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , b ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , c ) ; 
  vfloc = vfloc + 3 ; 
  if ( a < 128 ) 
  Result = ( a * toint ( 256 ) + b ) * toint ( 256 ) + c ; 
  else Result = ( ( a - toint ( 256 ) ) * toint ( 256 ) + b ) * toint ( 256 ) 
  + c ; 
  return(Result) ; 
} 
int24u vfutrio ( ) 
{register int24u Result; eightbits a, b, c  ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , a ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , b ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , c ) ; 
  vfloc = vfloc + 3 ; 
  Result = ( a * toint ( 256 ) + b ) * toint ( 256 ) + c ; 
  return(Result) ; 
} 
integer vfsquad ( ) 
{register integer Result; eightbits a, b, c, d  ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , a ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , b ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , c ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , d ) ; 
  vfloc = vfloc + 4 ; 
  if ( a < 128 ) 
  Result = ( ( a * toint ( 256 ) + b ) * toint ( 256 ) + c ) * toint ( 256 ) + 
  d ; 
  else Result = ( ( ( a - toint ( 256 ) ) * toint ( 256 ) + b ) * toint ( 256 
  ) + c ) * toint ( 256 ) + d ; 
  return(Result) ; 
} 
integer vffix1 ( ) 
{register integer Result; integer x  ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , tfmb3 ) ; 
  incr ( vfloc ) ; 
  if ( tfmb3 > 127 ) 
  tfmb1 = 255 ; 
  else tfmb1 = 0 ; 
  tfmb2 = tfmb1 ; 
  x = ( ( ( ( ( tfmb3 * z ) / 256 ) + ( tfmb2 * z ) ) / 256 ) + ( tfmb1 * z ) 
  ) / beta ; 
  if ( tfmb1 > 127 ) 
  x = x - alpha ; 
  Result = x ; 
  return(Result) ; 
} 
integer vffix2 ( ) 
{register integer Result; integer x  ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , tfmb2 ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , tfmb3 ) ; 
  vfloc = vfloc + 2 ; 
  if ( tfmb2 > 127 ) 
  tfmb1 = 255 ; 
  else tfmb1 = 0 ; 
  x = ( ( ( ( ( tfmb3 * z ) / 256 ) + ( tfmb2 * z ) ) / 256 ) + ( tfmb1 * z ) 
  ) / beta ; 
  if ( tfmb1 > 127 ) 
  x = x - alpha ; 
  Result = x ; 
  return(Result) ; 
} 
integer vffix3 ( ) 
{register integer Result; integer x  ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , tfmb1 ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , tfmb2 ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , tfmb3 ) ; 
  vfloc = vfloc + 3 ; 
  x = ( ( ( ( ( tfmb3 * z ) / 256 ) + ( tfmb2 * z ) ) / 256 ) + ( tfmb1 * z ) 
  ) / beta ; 
  if ( tfmb1 > 127 ) 
  x = x - alpha ; 
  Result = x ; 
  return(Result) ; 
} 
integer vffix3u ( ) 
{register integer Result; if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , tfmb1 ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , tfmb2 ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , tfmb3 ) ; 
  vfloc = vfloc + 3 ; 
  Result = ( ( ( ( ( tfmb3 * z ) / 256 ) + ( tfmb2 * z ) ) / 256 ) + ( tfmb1 * 
  z ) ) / beta ; 
  return(Result) ; 
} 
integer vffix4 ( ) 
{register integer Result; integer x  ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , tfmb0 ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , tfmb1 ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , tfmb2 ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , tfmb3 ) ; 
  vfloc = vfloc + 4 ; 
  x = ( ( ( ( ( tfmb3 * z ) / 256 ) + ( tfmb2 * z ) ) / 256 ) + ( tfmb1 * z ) 
  ) / beta ; 
  if ( tfmb0 > 0 ) 
  if ( tfmb0 == 255 ) 
  x = x - alpha ; 
  else badfont () ; 
  Result = x ; 
  return(Result) ; 
} 
int31 vfuquad ( ) 
{register int31 Result; integer x  ; 
  x = vfsquad () ; 
  if ( x < 0 ) 
  badfont () ; 
  else Result = x ; 
  return(Result) ; 
} 
int31 vfpquad ( ) 
{register int31 Result; integer x  ; 
  x = vfsquad () ; 
  if ( x <= 0 ) 
  badfont () ; 
  else Result = x ; 
  return(Result) ; 
} 
int31 vffixp ( ) 
{register int31 Result; integer x  ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , tfmb0 ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , tfmb1 ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , tfmb2 ) ; 
  if ( eof ( vffile ) ) 
  badfont () ; 
  else read ( vffile , tfmb3 ) ; 
  vfloc = vfloc + 4 ; 
  if ( tfmb0 > 0 ) 
  badfont () ; 
  Result = ( ( ( ( ( tfmb3 * z ) / 256 ) + ( tfmb2 * z ) ) / 256 ) + ( tfmb1 * 
  z ) ) / beta ; 
  return(Result) ; 
} 
void vffirstpar ( ) 
{curcmd = vfubyte () ; 
  switch ( dvipar [ curcmd ] ) 
  {case 0 : 
    {
      {
	curext = 0 ; 
	if ( curcmd < 128 ) 
	{
	  curres = curcmd ; 
	  curupd = true ; 
	} 
	else {
	    
	  curres = vfubyte () ; 
	  curupd = ( curcmd < 133 ) ; 
	  curcmd = curcmd - dvicharcmd [ curupd ] ; 
	  while ( curcmd > 0 ) {
	      
	    if ( curcmd == 3 ) 
	    if ( curres > 127 ) 
	    curext = -1 ; 
	    curext = curext * 256 + curres ; 
	    curres = vfubyte () ; 
	    decr ( curcmd ) ; 
	  } 
	} 
      } 
      curwp = 0 ; 
      if ( vfcurfnt != maxfonts ) 
      if ( ( curres >= fntbc [ vfcurfnt ] ) && ( curres <= fntec [ vfcurfnt ] 
      ) ) 
      curwp = charwidths [ fntchars [ vfcurfnt ] + curres ] ; 
      if ( curwp == 0 ) 
      badfont () ; 
    } 
    break ; 
  case 1 : 
    ; 
    break ; 
  case 2 : 
    curparm = vffix1 () ; 
    break ; 
  case 3 : 
    curparm = vfubyte () ; 
    break ; 
  case 4 : 
    curparm = vffix2 () ; 
    break ; 
  case 5 : 
    curparm = vfupair () ; 
    break ; 
  case 6 : 
    curparm = vffix3 () ; 
    break ; 
  case 7 : 
    curparm = vfutrio () ; 
    break ; 
  case 8 : 
    curparm = vffix4 () ; 
    break ; 
  case 9 : 
    curparm = vfsquad () ; 
    break ; 
  case 10 : 
    curparm = vfuquad () ; 
    break ; 
  case 11 : 
    {
      curvdimen = vffix4 () ; 
      curhdimen = vffix4 () ; 
      curupd = ( curcmd == 132 ) ; 
    } 
    break ; 
  case 12 : 
    curparm = curcmd - 171 ; 
    break ; 
  } 
  curclass = dvicl [ curcmd ] ; 
} 
void vffont ( ) 
{fontnumber f  ; 
  f = 0 ; 
  vfefnts [ vfnf ] = curparm ; 
  while ( curparm != vfefnts [ f ] ) incr ( f ) ; 
  if ( f == vfnf ) 
  badfont () ; 
  vfcurfnt = vfifnts [ f ] ; 
} 
void vfdofont ( ) 
{fontnumber f  ; 
  int15 k  ; 
  (void) fprintf( stdout , "%s%ld",  "VF: font " , (long)curparm ) ; 
  f = 0 ; 
  vfefnts [ vfnf ] = curparm ; 
  while ( curparm != vfefnts [ f ] ) incr ( f ) ; 
  if ( f != vfnf ) 
  badfont () ; 
  fntcheck [ nf ] = vfsquad () ; 
  fntscaled [ nf ] = vffixp () ; 
  fntdesign [ nf ] = round ( tfmconv * vfpquad () ) ; 
  k = vfubyte () ; 
  if ( maxbytes - byteptr < 1 ) 
  overflow ( strbytes , maxbytes ) ; 
  {
    bytemem [ byteptr ] = k ; 
    incr ( byteptr ) ; 
  } 
  k = k + vfubyte () ; 
  if ( maxbytes - byteptr < k ) 
  overflow ( strbytes , maxbytes ) ; 
  while ( k > 0 ) {
      
    {
      bytemem [ byteptr ] = vfubyte () ; 
      incr ( byteptr ) ; 
    } 
    decr ( k ) ; 
  } 
  fntname [ nf ] = makepacket () ; 
  vfifnts [ vfnf ] = makefont () ; 
  if ( vfnf == lclnf ) 
  if ( lclnf == maxfonts ) 
  overflow ( strfonts , maxfonts ) ; 
  else incr ( lclnf ) ; 
  incr ( vfnf ) ; 
} 
boolean dovf ( ) 
{/* 21 30 32 10 */ register boolean Result; integer tempint  ; 
  int8u tempbyte  ; 
  bytepointer k  ; 
  int15 l  ; 
  int24 saveext  ; 
  int8u saveres  ; 
  widthpointer savewp  ; 
  boolean saveupd  ; 
  widthpointer vfwp  ; 
  fontnumber vffnt  ; 
  boolean movezero  ; 
  boolean lastpop  ; 
  curnamelength = 0 ; 
  makename ( vfext ) ; 
  if ( testreadaccess ( curname , VFFILEPATH ) ) 
  {
    reset ( vffile , curname ) ; 
  } 
  else goto lab32 ; 
  vfloc = 0 ; 
  saveext = curext ; 
  saveres = curres ; 
  savewp = curwp ; 
  saveupd = curupd ; 
  fnttype [ curfnt ] = 1 ; 
  if ( vfubyte () != 247 ) 
  badfont () ; 
  if ( vfubyte () != 202 ) 
  badfont () ; 
  tempbyte = vfubyte () ; 
  if ( maxbytes - byteptr < tempbyte ) 
  overflow ( strbytes , maxbytes ) ; 
  {register integer for_end; l = 1 ; for_end = tempbyte ; if ( l <= for_end) 
  do 
    {
      bytemem [ byteptr ] = vfubyte () ; 
      incr ( byteptr ) ; 
    } 
  while ( l++ < for_end ) ; } 
  (void) Fputs( stdout ,  "VF file: '" ) ; 
  printpacket ( newpacket () ) ; 
  (void) Fputs( stdout ,  "'," ) ; 
  flushpacket () ; 
  checkchecksum ( vfsquad () , false ) ; 
  checkdesignsize ( round ( tfmconv * vfpquad () ) ) ; 
  z = fntscaled [ curfnt ] ; 
  alpha = 16 ; 
  while ( z >= 8388608L ) {
      
    z = z / 2 ; 
    alpha = alpha + alpha ; 
  } 
  beta = 256 / alpha ; 
  alpha = alpha * z ; 
  {
    (void) putc('\n',  stdout );
    (void) fprintf( stdout , "%s%ld",  "   for font " , (long)curfnt ) ; 
  } 
  printfont ( curfnt ) ; 
  (void) fprintf( stdout , "%c\n",  '.' ) ; 
  vfifnts [ 0 ] = maxfonts ; 
  vfnf = 0 ; 
  curcmd = vfubyte () ; 
  while ( ( curcmd >= 243 ) && ( curcmd <= 246 ) ) {
      
    switch ( curcmd - 243 ) 
    {case 0 : 
      curparm = vfubyte () ; 
      break ; 
    case 1 : 
      curparm = vfupair () ; 
      break ; 
    case 2 : 
      curparm = vfutrio () ; 
      break ; 
    case 3 : 
      curparm = vfsquad () ; 
      break ; 
    } 
    vfdofont () ; 
    curcmd = vfubyte () ; 
  } 
  fntfont [ curfnt ] = vfifnts [ 0 ] ; 
  while ( curcmd <= 242 ) {
      
    if ( curcmd < 242 ) 
    {
      vflimit = curcmd ; 
      curext = 0 ; 
      curres = vfubyte () ; 
      vfwp = checkwidth ( vffix3u () ) ; 
    } 
    else {
	
      vflimit = vfuquad () ; 
      curext = vfstrio () ; 
      curres = vfubyte () ; 
      vfwp = checkwidth ( vffix4 () ) ; 
    } 
    vflimit = vflimit + vfloc ; 
    vfpushloc [ 0 ] = byteptr ; 
    vflastend [ 0 ] = byteptr ; 
    vflast [ 0 ] = 4 ; 
    vfptr = 0 ; 
    startpacket ( 1 ) ; 
    vfcurfnt = fntfont [ curfnt ] ; 
    vffnt = vfcurfnt ; 
    lastpop = false ; 
    curclass = 3 ; 
    while ( true ) {
	
      lab21: switch ( curclass ) 
      {case 0 : 
      case 1 : 
      case 2 : 
	{
	  if ( ( vfptr == 0 ) || ( byteptr > vfpushloc [ vfptr ] ) ) 
	  movezero = false ; 
	  else switch ( curclass ) 
	  {case 0 : 
	    movezero = ( ! curupd ) || ( vfcurfnt != vffnt ) ; 
	    break ; 
	  case 1 : 
	    movezero = ! curupd ; 
	    break ; 
	  case 2 : 
	    movezero = true ; 
	    break ; 
	  } 
	  if ( movezero ) 
	  {
	    decr ( byteptr ) ; 
	    decr ( vfptr ) ; 
	  } 
	  switch ( curclass ) 
	  {case 0 : 
	    {
	      if ( vfcurfnt != vffnt ) 
	      {
		vflast [ vfptr ] = 4 ; 
		pcktunsigned ( 235 , vfcurfnt ) ; 
		vffnt = vfcurfnt ; 
	      } 
	      if ( ( ! movezero ) || ( ! curupd ) ) 
	      {
		vflast [ vfptr ] = vfchartype [ curupd ] ; 
		vflastloc [ vfptr ] = byteptr ; 
		pcktchar ( curupd , curext , curres ) ; 
	      } 
	    } 
	    break ; 
	  case 1 : 
	    {
	      vflast [ vfptr ] = vfruletype [ curupd ] ; 
	      vflastloc [ vfptr ] = byteptr ; 
	      {
		if ( maxbytes - byteptr < 1 ) 
		overflow ( strbytes , maxbytes ) ; 
		{
		  bytemem [ byteptr ] = dvirulecmd [ curupd ] ; 
		  incr ( byteptr ) ; 
		} 
	      } 
	      pcktfour ( curvdimen ) ; 
	      pcktfour ( curhdimen ) ; 
	    } 
	    break ; 
	  case 2 : 
	    {
	      vflast [ vfptr ] = 4 ; 
	      pcktunsigned ( 239 , curparm ) ; 
	      if ( maxbytes - byteptr < curparm ) 
	      overflow ( strbytes , maxbytes ) ; 
	      while ( curparm > 0 ) {
		  
		{
		  bytemem [ byteptr ] = vfubyte () ; 
		  incr ( byteptr ) ; 
		} 
		decr ( curparm ) ; 
	      } 
	    } 
	    break ; 
	  } 
	  vflastend [ vfptr ] = byteptr ; 
	  if ( movezero ) 
	  {
	    incr ( vfptr ) ; 
	    {
	      if ( maxbytes - byteptr < 1 ) 
	      overflow ( strbytes , maxbytes ) ; 
	      {
		bytemem [ byteptr ] = 141 ; 
		incr ( byteptr ) ; 
	      } 
	    } 
	    vfpushloc [ vfptr ] = byteptr ; 
	    vflastend [ vfptr ] = byteptr ; 
	    if ( curclass == 0 ) 
	    if ( curupd ) 
	    goto lab21 ; 
	  } 
	} 
	break ; 
      case 3 : 
	if ( ( vfptr > 0 ) && ( vfpushloc [ vfptr ] == byteptr ) ) 
	{
	  if ( vfpushnum [ vfptr ] == 255 ) 
	  overflow ( strstack , 255 ) ; 
	  incr ( vfpushnum [ vfptr ] ) ; 
	} 
	else {
	    
	  if ( vfptr == stackused ) 
	  if ( stackused == stacksize ) 
	  overflow ( strstack , stacksize ) ; 
	  else incr ( stackused ) ; 
	  incr ( vfptr ) ; 
	  {
	    if ( maxbytes - byteptr < 1 ) 
	    overflow ( strbytes , maxbytes ) ; 
	    {
	      bytemem [ byteptr ] = 141 ; 
	      incr ( byteptr ) ; 
	    } 
	  } 
	  {
	    vfmove [ vfptr ] [ 0 ] [ 0 ] = vfmove [ vfptr - 1 ] [ 0 ] [ 0 ] ; 
	    vfmove [ vfptr ] [ 0 ] [ 1 ] = vfmove [ vfptr - 1 ] [ 0 ] [ 1 ] ; 
	    vfmove [ vfptr ] [ 1 ] [ 0 ] = vfmove [ vfptr - 1 ] [ 1 ] [ 0 ] ; 
	    vfmove [ vfptr ] [ 1 ] [ 1 ] = vfmove [ vfptr - 1 ] [ 1 ] [ 1 ] ; 
	  } 
	  vfpushloc [ vfptr ] = byteptr ; 
	  vflastend [ vfptr ] = byteptr ; 
	  vflast [ vfptr ] = 4 ; 
	  vfpushnum [ vfptr ] = 0 ; 
	} 
	break ; 
      case 4 : 
	{
	  if ( vfptr < 1 ) 
	  badfont () ; 
	  byteptr = vflastend [ vfptr ] ; 
	  if ( vflast [ vfptr ] <= 1 ) 
	  if ( vflastloc [ vfptr ] == vfpushloc [ vfptr ] ) 
	  {
	    curclass = vflast [ vfptr ] ; 
	    curupd = false ; 
	    byteptr = vfpushloc [ vfptr ] ; 
	  } 
	  if ( byteptr == vfpushloc [ vfptr ] ) 
	  {
	    if ( vfpushnum [ vfptr ] > 0 ) 
	    {
	      decr ( vfpushnum [ vfptr ] ) ; 
	      {
		vfmove [ vfptr ] [ 0 ] [ 0 ] = vfmove [ vfptr - 1 ] [ 0 ] [ 0 
		] ; 
		vfmove [ vfptr ] [ 0 ] [ 1 ] = vfmove [ vfptr - 1 ] [ 0 ] [ 1 
		] ; 
		vfmove [ vfptr ] [ 1 ] [ 0 ] = vfmove [ vfptr - 1 ] [ 1 ] [ 0 
		] ; 
		vfmove [ vfptr ] [ 1 ] [ 1 ] = vfmove [ vfptr - 1 ] [ 1 ] [ 1 
		] ; 
	      } 
	    } 
	    else {
		
	      decr ( byteptr ) ; 
	      decr ( vfptr ) ; 
	    } 
	    if ( curclass != 4 ) 
	    goto lab21 ; 
	  } 
	  else {
	      
	    if ( vflast [ vfptr ] == 2 ) 
	    {
	      byteptr = byteptr - 2 ; 
	      {register integer for_end; k = vflastloc [ vfptr ] + 1 ; 
	      for_end = byteptr ; if ( k <= for_end) do 
		bytemem [ k - 1 ] = bytemem [ k ] ; 
	      while ( k++ < for_end ) ; } 
	      vflast [ vfptr ] = 4 ; 
	      vflastend [ vfptr ] = byteptr ; 
	    } 
	    {
	      if ( maxbytes - byteptr < 1 ) 
	      overflow ( strbytes , maxbytes ) ; 
	      {
		bytemem [ byteptr ] = 142 ; 
		incr ( byteptr ) ; 
	      } 
	    } 
	    decr ( vfptr ) ; 
	    vflast [ vfptr ] = 2 ; 
	    vflastloc [ vfptr ] = vfpushloc [ vfptr + 1 ] - 1 ; 
	    vflastend [ vfptr ] = byteptr ; 
	    if ( vfpushnum [ vfptr + 1 ] > 0 ) 
	    {
	      incr ( vfptr ) ; 
	      {
		if ( maxbytes - byteptr < 1 ) 
		overflow ( strbytes , maxbytes ) ; 
		{
		  bytemem [ byteptr ] = 141 ; 
		  incr ( byteptr ) ; 
		} 
	      } 
	      {
		vfmove [ vfptr ] [ 0 ] [ 0 ] = vfmove [ vfptr - 1 ] [ 0 ] [ 0 
		] ; 
		vfmove [ vfptr ] [ 0 ] [ 1 ] = vfmove [ vfptr - 1 ] [ 0 ] [ 1 
		] ; 
		vfmove [ vfptr ] [ 1 ] [ 0 ] = vfmove [ vfptr - 1 ] [ 1 ] [ 0 
		] ; 
		vfmove [ vfptr ] [ 1 ] [ 1 ] = vfmove [ vfptr - 1 ] [ 1 ] [ 1 
		] ; 
	      } 
	      vfpushloc [ vfptr ] = byteptr ; 
	      vflastend [ vfptr ] = byteptr ; 
	      vflast [ vfptr ] = 4 ; 
	      decr ( vfpushnum [ vfptr ] ) ; 
	    } 
	  } 
	} 
	break ; 
      case 5 : 
      case 6 : 
	if ( vfmove [ vfptr ] [ 0 ] [ curclass - 5 ] ) 
	{
	  if ( maxbytes - byteptr < 1 ) 
	  overflow ( strbytes , maxbytes ) ; 
	  {
	    bytemem [ byteptr ] = curcmd ; 
	    incr ( byteptr ) ; 
	  } 
	} 
	break ; 
      case 7 : 
      case 8 : 
      case 9 : 
	{
	  pcktsigned ( dvirightcmd [ curclass ] , curparm ) ; 
	  if ( curclass >= 8 ) 
	  vfmove [ vfptr ] [ 0 ] [ curclass - 8 ] = true ; 
	} 
	break ; 
      case 10 : 
      case 11 : 
	if ( vfmove [ vfptr ] [ 1 ] [ curclass - 10 ] ) 
	{
	  if ( maxbytes - byteptr < 1 ) 
	  overflow ( strbytes , maxbytes ) ; 
	  {
	    bytemem [ byteptr ] = curcmd ; 
	    incr ( byteptr ) ; 
	  } 
	} 
	break ; 
      case 12 : 
      case 13 : 
      case 14 : 
	{
	  pcktsigned ( dvidowncmd [ curclass ] , curparm ) ; 
	  if ( curclass >= 13 ) 
	  vfmove [ vfptr ] [ 1 ] [ curclass - 13 ] = true ; 
	} 
	break ; 
      case 15 : 
	vffont () ; 
	break ; 
      case 16 : 
	badfont () ; 
	break ; 
      case 17 : 
	if ( curcmd != 138 ) 
	badfont () ; 
	break ; 
      } 
      if ( vfloc < vflimit ) 
      vffirstpar () ; 
      else if ( lastpop ) 
      goto lab30 ; 
      else {
	  
	curclass = 4 ; 
	lastpop = true ; 
      } 
    } 
    lab30: if ( ( vfptr != 0 ) || ( vfloc != vflimit ) ) 
    badfont () ; 
    k = pcktstart [ pcktptr ] ; 
    if ( vflast [ 0 ] == 3 ) 
    if ( curwp == vfwp ) 
    {
      decr ( bytemem [ k ] ) ; 
      if ( ( bytemem [ k ] == 0 ) && ( vfpushloc [ 0 ] == vflastloc [ 0 ] ) && 
      ( curext == 0 ) && ( curres == pcktres ) ) 
      byteptr = k ; 
    } 
    buildpacket () ; 
    curcmd = vfubyte () ; 
  } 
  if ( curcmd != 248 ) 
  badfont () ; 
  curext = saveext ; 
  curres = saveres ; 
  curwp = savewp ; 
  curupd = saveupd ; 
  Result = true ; 
  goto lab10 ; 
  lab32: Result = false ; 
  lab10: ; 
  return(Result) ; 
} 
void inputln ( ) 
{integer k  ; 
  (void) Fputs( stdout ,  "Enter option: " ) ; 
  flush ( stdout ) ; 
  k = 0 ; 
  if ( maxbytes - byteptr < terminallinelength ) 
  overflow ( strbytes , maxbytes ) ; 
  while ( ( k < terminallinelength ) && ! eoln ( stdin ) ) {
      
    {
      bytemem [ byteptr ] = xord [ getc ( stdin ) ] ; 
      incr ( byteptr ) ; 
    } 
    incr ( k ) ; 
  } 
} 
boolean zscankeyword ( p , l ) 
pcktpointer p ; 
int7 l ; 
{register boolean Result; bytepointer i, j, k  ; 
  i = pcktstart [ p ] ; 
  j = pcktstart [ p + 1 ] ; 
  k = scanptr ; 
  while ( ( i < j ) && ( ( bytemem [ k ] == bytemem [ i ] ) || ( bytemem [ k ] 
  == bytemem [ i ] - 32 ) ) ) {
      
    incr ( i ) ; 
    incr ( k ) ; 
  } 
  if ( ( bytemem [ k ] == 32 ) && ( i - pcktstart [ p ] >= l ) ) 
  {
    scanptr = k ; 
    while ( ( bytemem [ scanptr ] == 32 ) && ( scanptr < byteptr ) ) incr ( 
    scanptr ) ; 
    Result = true ; 
  } 
  else Result = false ; 
  return(Result) ; 
} 
integer scanint ( ) 
{register integer Result; integer x  ; 
  boolean negative  ; 
  if ( bytemem [ scanptr ] == 45 ) 
  {
    negative = true ; 
    incr ( scanptr ) ; 
  } 
  else negative = false ; 
  x = 0 ; 
  while ( ( bytemem [ scanptr ] >= 48 ) && ( bytemem [ scanptr ] <= 57 ) ) {
      
    x = 10 * x + bytemem [ scanptr ] - 48 ; 
    incr ( scanptr ) ; 
  } 
  while ( ( bytemem [ scanptr ] == 32 ) && ( scanptr < byteptr ) ) incr ( 
  scanptr ) ; 
  if ( negative ) 
  Result = - (integer) x ; 
  else Result = x ; 
  return(Result) ; 
} 
void scancount ( ) 
{if ( bytemem [ scanptr ] == 42 ) 
  {
    selectthere [ curselect ] [ selectvals [ curselect ] ] = false ; 
    incr ( scanptr ) ; 
    while ( ( bytemem [ scanptr ] == 32 ) && ( scanptr < byteptr ) ) incr ( 
    scanptr ) ; 
  } 
  else {
      
    selectthere [ curselect ] [ selectvals [ curselect ] ] = true ; 
    selectcount [ curselect ] [ selectvals [ curselect ] ] = scanint () ; 
    if ( curselect == 0 ) 
    selected = false ; 
  } 
} 
void dialog ( ) 
{/* 10 */ pcktpointer p  ; 
  outmag = 0 ; 
  curselect = 0 ; 
  selectmax [ curselect ] = 0 ; 
  selected = true ; 
  while ( true ) {
      
    inputln () ; 
    p = newpacket () ; 
    bytemem [ byteptr ] = 32 ; 
    scanptr = pcktstart [ pcktptr - 1 ] ; 
    while ( ( bytemem [ scanptr ] == 32 ) && ( scanptr < byteptr ) ) incr ( 
    scanptr ) ; 
    if ( scanptr == byteptr ) 
    {
      flushpacket () ; 
      goto lab10 ; 
    } 
    else if ( scankeyword ( strmag , 3 ) ) 
    outmag = scanint () ; 
    else if ( scankeyword ( strselect , 3 ) ) 
    if ( curselect == 10 ) 
    (void) fprintf( stdout , "%s\n",  "Too many page selections" ) ; 
    else {
	
      selectvals [ curselect ] = 0 ; 
      scancount () ; 
      while ( ( selectvals [ curselect ] < 9 ) && ( bytemem [ scanptr ] == 46 
      ) ) {
	  
	incr ( selectvals [ curselect ] ) ; 
	incr ( scanptr ) ; 
	scancount () ; 
      } 
      selectmax [ curselect ] = scanint () ; 
      incr ( curselect ) ; 
    } 
    else {
	
      (void) fprintf( stdout , "%s\n",  "Valid options are:" ) ; 
      (void) fprintf( stdout , "%s\n",  "  mag <mag>" ) ; 
      (void) fprintf( stdout , "%s\n",  "  select <first page> [<num pages>]" ) ; 
    } 
    if ( eoln ( stdin ) ) 
    readln ( stdin ) ; 
    flushpacket () ; 
  } 
  lab10: ; 
} 
void zoutpacket ( p ) 
pcktpointer p ; 
{bytepointer k  ; 
  outloc = outloc + ( pcktstart [ p + 1 ] - pcktstart [ p ] ) ; 
  {register integer for_end; k = pcktstart [ p ] ; for_end = pcktstart [ p + 
  1 ] - 1 ; if ( k <= for_end) do 
    putbyte ( bytemem [ k ] , outfile ) ; 
  while ( k++ < for_end ) ; } 
} 
void zoutfour ( x ) 
integer x ; 
{; 
  if ( x >= 0 ) 
  putbyte ( x / 16777216L , outfile ) ; 
  else {
      
    x = x + 1073741824L ; 
    x = x + 1073741824L ; 
    putbyte ( ( x / 16777216L ) + 128 , outfile ) ; 
  } 
  x = x % 16777216L ; 
  putbyte ( x / 65536L , outfile ) ; 
  x = x % 65536L ; 
  putbyte ( x / 256 , outfile ) ; 
  putbyte ( x % 256 , outfile ) ; 
  outloc = outloc + 4 ; 
} 
void zoutchar ( upd , ext , res ) 
boolean upd ; 
integer ext ; 
eightbits res ; 
{eightbits o  ; 
  if ( ( ! upd ) || ( res > 127 ) || ( ext != 0 ) ) 
  {
    o = dvicharcmd [ upd ] ; 
    if ( ext < 0 ) 
    ext = ext + 16777216L ; 
    if ( ext == 0 ) 
    {
      putbyte ( o , outfile ) ; 
      incr ( outloc ) ; 
    } 
    else {
	
      if ( ext < 256 ) 
      {
	putbyte ( o + 1 , outfile ) ; 
	incr ( outloc ) ; 
      } 
      else {
	  
	if ( ext < 65536L ) 
	{
	  putbyte ( o + 2 , outfile ) ; 
	  incr ( outloc ) ; 
	} 
	else {
	    
	  {
	    putbyte ( o + 3 , outfile ) ; 
	    incr ( outloc ) ; 
	  } 
	  {
	    putbyte ( ext / 65536L , outfile ) ; 
	    incr ( outloc ) ; 
	  } 
	  ext = ext % 65536L ; 
	} 
	{
	  putbyte ( ext / 256 , outfile ) ; 
	  incr ( outloc ) ; 
	} 
	ext = ext % 256 ; 
      } 
      {
	putbyte ( ext , outfile ) ; 
	incr ( outloc ) ; 
      } 
    } 
  } 
  {
    putbyte ( res , outfile ) ; 
    incr ( outloc ) ; 
  } 
} 
void zoutunsigned ( o , x ) 
eightbits o ; 
integer x ; 
{; 
  if ( ( x < 256 ) && ( x >= 0 ) ) 
  if ( ( o == 235 ) && ( x < 64 ) ) 
  x = x + 171 ; 
  else {
      
    putbyte ( o , outfile ) ; 
    incr ( outloc ) ; 
  } 
  else {
      
    if ( ( x < 65536L ) && ( x >= 0 ) ) 
    {
      putbyte ( o + 1 , outfile ) ; 
      incr ( outloc ) ; 
    } 
    else {
	
      if ( ( x < 16777216L ) && ( x >= 0 ) ) 
      {
	putbyte ( o + 2 , outfile ) ; 
	incr ( outloc ) ; 
      } 
      else {
	  
	{
	  putbyte ( o + 3 , outfile ) ; 
	  incr ( outloc ) ; 
	} 
	if ( x >= 0 ) 
	{
	  putbyte ( x / 16777216L , outfile ) ; 
	  incr ( outloc ) ; 
	} 
	else {
	    
	  x = x + 1073741824L ; 
	  x = x + 1073741824L ; 
	  {
	    putbyte ( ( x / 16777216L ) + 128 , outfile ) ; 
	    incr ( outloc ) ; 
	  } 
	} 
	x = x % 16777216L ; 
      } 
      {
	putbyte ( x / 65536L , outfile ) ; 
	incr ( outloc ) ; 
      } 
      x = x % 65536L ; 
    } 
    {
      putbyte ( x / 256 , outfile ) ; 
      incr ( outloc ) ; 
    } 
    x = x % 256 ; 
  } 
  {
    putbyte ( x , outfile ) ; 
    incr ( outloc ) ; 
  } 
} 
void zoutsigned ( o , x ) 
eightbits o ; 
integer x ; 
{int31 xx  ; 
  if ( x >= 0 ) 
  xx = x ; 
  else xx = - (integer) ( x + 1 ) ; 
  if ( xx < 128 ) 
  {
    {
      putbyte ( o , outfile ) ; 
      incr ( outloc ) ; 
    } 
    if ( x < 0 ) 
    x = x + 256 ; 
  } 
  else {
      
    if ( xx < 32768L ) 
    {
      {
	putbyte ( o + 1 , outfile ) ; 
	incr ( outloc ) ; 
      } 
      if ( x < 0 ) 
      x = x + 65536L ; 
    } 
    else {
	
      if ( xx < 8388608L ) 
      {
	{
	  putbyte ( o + 2 , outfile ) ; 
	  incr ( outloc ) ; 
	} 
	if ( x < 0 ) 
	x = x + 16777216L ; 
      } 
      else {
	  
	{
	  putbyte ( o + 3 , outfile ) ; 
	  incr ( outloc ) ; 
	} 
	if ( x >= 0 ) 
	{
	  putbyte ( x / 16777216L , outfile ) ; 
	  incr ( outloc ) ; 
	} 
	else {
	    
	  x = 2147483647L - xx ; 
	  {
	    putbyte ( ( x / 16777216L ) + 128 , outfile ) ; 
	    incr ( outloc ) ; 
	  } 
	} 
	x = x % 16777216L ; 
      } 
      {
	putbyte ( x / 65536L , outfile ) ; 
	incr ( outloc ) ; 
      } 
      x = x % 65536L ; 
    } 
    {
      putbyte ( x / 256 , outfile ) ; 
      incr ( outloc ) ; 
    } 
    x = x % 256 ; 
  } 
  {
    putbyte ( x , outfile ) ; 
    incr ( outloc ) ; 
  } 
} 
void zoutfntdef ( f ) 
fontnumber f ; 
{pcktpointer p  ; 
  bytepointer k, l  ; 
  eightbits a  ; 
  outunsigned ( 243 , fntfont [ f ] ) ; 
  outfour ( fntcheck [ f ] ) ; 
  outfour ( fntscaled [ f ] ) ; 
  outfour ( fntdesign [ f ] ) ; 
  p = fntname [ f ] ; 
  k = pcktstart [ p ] ; 
  l = pcktstart [ p + 1 ] - 1 ; 
  a = bytemem [ k ] ; 
  outloc = outloc + l - k + 2 ; 
  putbyte ( a , outfile ) ; 
  putbyte ( l - k - a , outfile ) ; 
  while ( k < l ) {
      
    incr ( k ) ; 
    putbyte ( bytemem [ k ] , outfile ) ; 
  } 
} 
boolean startmatch ( ) 
{register boolean Result; schar k  ; 
  boolean match  ; 
  match = true ; 
  {register integer for_end; k = 0 ; for_end = selectvals [ curselect ] 
  ; if ( k <= for_end) do 
    if ( selectthere [ curselect ] [ k ] && ( selectcount [ curselect ] [ k ] 
    != count [ k ] ) ) 
    match = false ; 
  while ( k++ < for_end ) ; } 
  Result = match ; 
  return(Result) ; 
} 
void dopre ( ) 
{int15 k  ; 
  bytepointer p, q, r  ; 
  ccharpointer comment  ; 
  alldone = false ; 
  numselect = curselect ; 
  curselect = 0 ; 
  if ( numselect == 0 ) 
  selectmax [ curselect ] = 0 ; 
  {
    putbyte ( 247 , outfile ) ; 
    incr ( outloc ) ; 
  } 
  {
    putbyte ( 2 , outfile ) ; 
    incr ( outloc ) ; 
  } 
  outfour ( dvinum ) ; 
  outfour ( dviden ) ; 
  outfour ( outmag ) ; 
  p = pcktstart [ pcktptr - 1 ] ; 
  q = byteptr ; 
  comment = " DVIcopy 1.2 output from " ; 
  if ( maxbytes - byteptr < 24 ) 
  overflow ( strbytes , maxbytes ) ; 
  {register integer for_end; k = 1 ; for_end = 24 ; if ( k <= for_end) do 
    {
      bytemem [ byteptr ] = xord [ comment [ k ] ] ; 
      incr ( byteptr ) ; 
    } 
  while ( k++ < for_end ) ; } 
  while ( bytemem [ p ] == 32 ) incr ( p ) ; 
  if ( p == q ) 
  byteptr = byteptr - 6 ; 
  else {
      
    k = 0 ; 
    while ( ( k < 24 ) && ( bytemem [ p + k ] == bytemem [ q + k ] ) ) incr ( 
    k ) ; 
    if ( k == 24 ) 
    p = p + 24 ; 
  } 
  k = byteptr - p ; 
  if ( k > 255 ) 
  {
    k = 255 ; 
    q = p + 231 ; 
  } 
  {
    putbyte ( k , outfile ) ; 
    incr ( outloc ) ; 
  } 
  outpacket ( newpacket () ) ; 
  flushpacket () ; 
  {register integer for_end; r = p ; for_end = q - 1 ; if ( r <= for_end) do 
    {
      putbyte ( bytemem [ r ] , outfile ) ; 
      incr ( outloc ) ; 
    } 
  while ( r++ < for_end ) ; } 
} 
void dobop ( ) 
{schar i, j  ; 
  if ( ! selected ) 
  selected = startmatch () ; 
  typesetting = selected ; 
  (void) Fputs( stdout ,  "DVI: " ) ; 
  if ( typesetting ) 
  (void) Fputs( stdout ,  "process" ) ; 
  else
  (void) Fputs( stdout ,  "skipp" ) ; 
  (void) fprintf( stdout , "%s%ld",  "ing page " , (long)count [ 0 ] ) ; 
  j = 9 ; 
  while ( ( j > 0 ) && ( count [ j ] == 0 ) ) decr ( j ) ; 
  {register integer for_end; i = 1 ; for_end = j ; if ( i <= for_end) do 
    (void) fprintf( stdout , "%c%ld",  '.' , (long)count [ i ] ) ; 
  while ( i++ < for_end ) ; } 
  (void) fprintf( stdout , "%c\n",  '.' ) ; 
  if ( typesetting ) 
  {
    stackptr = 0 ; 
    curstack = zerostack ; 
    curfnt = maxfonts ; 
    {
      putbyte ( 139 , outfile ) ; 
      incr ( outloc ) ; 
    } 
    incr ( outpages ) ; 
    {register integer for_end; i = 0 ; for_end = 9 ; if ( i <= for_end) do 
      outfour ( count [ i ] ) ; 
    while ( i++ < for_end ) ; } 
    outfour ( outback ) ; 
    outback = outloc - 45 ; 
    outfnt = maxfonts ; 
  } 
} 
void doeop ( ) 
{if ( stackptr != 0 ) 
  baddvi () ; 
  {
    putbyte ( 140 , outfile ) ; 
    incr ( outloc ) ; 
  } 
  if ( selectmax [ curselect ] > 0 ) 
  {
    decr ( selectmax [ curselect ] ) ; 
    if ( selectmax [ curselect ] == 0 ) 
    {
      selected = false ; 
      incr ( curselect ) ; 
      if ( curselect == numselect ) 
      alldone = true ; 
    } 
  } 
  typesetting = false ; 
} 
void dopush ( ) 
{if ( stackptr == stackused ) 
  if ( stackused == stacksize ) 
  overflow ( strstack , stacksize ) ; 
  else incr ( stackused ) ; 
  incr ( stackptr ) ; 
  stack [ stackptr ] = curstack ; 
  if ( stackptr > outstack ) 
  outstack = stackptr ; 
  {
    putbyte ( 141 , outfile ) ; 
    incr ( outloc ) ; 
  } 
} 
void dopop ( ) 
{if ( stackptr == 0 ) 
  baddvi () ; 
  {
    putbyte ( 142 , outfile ) ; 
    incr ( outloc ) ; 
  } 
  curstack = stack [ stackptr ] ; 
  decr ( stackptr ) ; 
} 
void doxxx ( ) 
{pcktpointer p  ; 
  p = newpacket () ; 
  outunsigned ( 239 , ( pcktstart [ p + 1 ] - pcktstart [ p ] ) ) ; 
  outpacket ( p ) ; 
  flushpacket () ; 
} 
void doright ( ) 
{if ( curclass >= 8 ) 
  curstack .wxfield [ curclass - 8 ] = curparm ; 
  else if ( curclass < 7 ) 
  curparm = curstack .wxfield [ curclass - 5 ] ; 
  if ( curclass < 7 ) 
  {
    putbyte ( curcmd , outfile ) ; 
    incr ( outloc ) ; 
  } 
  else outsigned ( dvirightcmd [ curclass ] , curparm ) ; 
  curstack .hfield = curstack .hfield + curparm ; 
  if ( abs ( curstack .hfield ) > outmaxh ) 
  outmaxh = abs ( curstack .hfield ) ; 
} 
void dodown ( ) 
{if ( curclass >= 13 ) 
  curstack .yzfield [ curclass - 13 ] = curparm ; 
  else if ( curclass < 12 ) 
  curparm = curstack .yzfield [ curclass - 10 ] ; 
  if ( curclass < 12 ) 
  {
    putbyte ( curcmd , outfile ) ; 
    incr ( outloc ) ; 
  } 
  else outsigned ( dvidowncmd [ curclass ] , curparm ) ; 
  curstack .vfield = curstack .vfield + curparm ; 
  if ( abs ( curstack .vfield ) > outmaxv ) 
  outmaxv = abs ( curstack .vfield ) ; 
} 
void dowidth ( ) 
{{
    
    putbyte ( 132 , outfile ) ; 
    incr ( outloc ) ; 
  } 
  outfour ( widthdimen ) ; 
  outfour ( curhdimen ) ; 
  curstack .hfield = curstack .hfield + curhdimen ; 
  if ( abs ( curstack .hfield ) > outmaxh ) 
  outmaxh = abs ( curstack .hfield ) ; 
} 
void dorule ( ) 
{boolean visible  ; 
  if ( ( curhdimen > 0 ) && ( curvdimen > 0 ) ) 
  {
    visible = true ; 
    {
      putbyte ( dvirulecmd [ curupd ] , outfile ) ; 
      incr ( outloc ) ; 
    } 
    outfour ( curvdimen ) ; 
    outfour ( curhdimen ) ; 
  } 
  else {
      
    visible = false ; 
    {
      putbyte ( dvirulecmd [ curupd ] , outfile ) ; 
      incr ( outloc ) ; 
    } 
    outfour ( curvdimen ) ; 
    outfour ( curhdimen ) ; 
  } 
  if ( curupd ) 
  {
    curstack .hfield = curstack .hfield + curhdimen ; 
    if ( abs ( curstack .hfield ) > outmaxh ) 
    outmaxh = abs ( curstack .hfield ) ; 
  } 
} 
void dochar ( ) 
{if ( curfnt != outfnt ) 
  {
    outunsigned ( 235 , fntfont [ curfnt ] ) ; 
    outfnt = curfnt ; 
  } 
  outchar ( curupd , curext , curres ) ; 
  if ( curupd ) 
  {
    curstack .hfield = curstack .hfield + widths [ curwp ] ; 
    if ( abs ( curstack .hfield ) > outmaxh ) 
    outmaxh = abs ( curstack .hfield ) ; 
  } 
} 
void dofont ( ) 
{/* 30 */ if ( dovf () ) 
  goto lab30 ; 
  if ( ( outnf >= maxfonts ) ) 
  overflow ( strfonts , maxfonts ) ; 
  (void) fprintf( stdout , "%s%ld",  "OUT: font " , (long)curfnt ) ; 
  printfont ( curfnt ) ; 
  (void) fprintf( stdout , "%c\n",  '.' ) ; 
  fnttype [ curfnt ] = 2 ; 
  fntfont [ curfnt ] = outnf ; 
  outfnts [ outnf ] = curfnt ; 
  incr ( outnf ) ; 
  outfntdef ( curfnt ) ; 
  lab30: ; 
} 
void pcktfirstpar ( ) 
{curcmd = pcktubyte () ; 
  switch ( dvipar [ curcmd ] ) 
  {case 0 : 
    {
      curext = 0 ; 
      if ( curcmd < 128 ) 
      {
	curres = curcmd ; 
	curupd = true ; 
      } 
      else {
	  
	curres = pcktubyte () ; 
	curupd = ( curcmd < 133 ) ; 
	curcmd = curcmd - dvicharcmd [ curupd ] ; 
	while ( curcmd > 0 ) {
	    
	  if ( curcmd == 3 ) 
	  if ( curres > 127 ) 
	  curext = -1 ; 
	  curext = curext * 256 + curres ; 
	  curres = pcktubyte () ; 
	  decr ( curcmd ) ; 
	} 
      } 
    } 
    break ; 
  case 1 : 
    ; 
    break ; 
  case 2 : 
    curparm = pcktsbyte () ; 
    break ; 
  case 3 : 
    curparm = pcktubyte () ; 
    break ; 
  case 4 : 
    curparm = pcktspair () ; 
    break ; 
  case 5 : 
    curparm = pcktupair () ; 
    break ; 
  case 6 : 
    curparm = pcktstrio () ; 
    break ; 
  case 7 : 
    curparm = pcktutrio () ; 
    break ; 
  case 8 : 
  case 9 : 
  case 10 : 
    curparm = pcktsquad () ; 
    break ; 
  case 11 : 
    {
      curvdimen = pcktsquad () ; 
      curhdimen = pcktsquad () ; 
      curupd = ( curcmd == 132 ) ; 
    } 
    break ; 
  case 12 : 
    curparm = curcmd - 171 ; 
    break ; 
  } 
  curclass = dvicl [ curcmd ] ; 
} 
void dovfpacket ( ) 
{/* 22 31 30 */ recurpointer k  ; 
  int8u f  ; 
  boolean saveupd  ; 
  widthpointer savewp  ; 
  bytepointer savelimit  ; 
  saveupd = curupd ; 
  savewp = curwp ; 
  recurfnt [ nrecur ] = curfnt ; 
  recurext [ nrecur ] = curext ; 
  recurres [ nrecur ] = curres ; 
  if ( findpacket () ) 
  f = curtype ; 
  else goto lab30 ; 
  recurpckt [ nrecur ] = curpckt ; 
  savelimit = curlimit ; 
  curfnt = fntfont [ curfnt ] ; 
  if ( curpckt == 0 ) 
  {
    curclass = 0 ; 
    goto lab31 ; 
  } 
  if ( curloc >= curlimit ) 
  goto lab30 ; 
  lab22: pcktfirstpar () ; 
  lab31: switch ( curclass ) 
  {case 0 : 
    {
      curwp = charwidths [ fntchars [ curfnt ] + curres ] ; 
      if ( fnttype [ curfnt ] == 0 ) 
      dofont () ; 
      if ( ( curloc == curlimit ) && ( f == 0 ) && saveupd ) 
      {
	saveupd = false ; 
	curupd = true ; 
      } 
      if ( fnttype [ curfnt ] == 1 ) 
      {
	recurloc [ nrecur ] = curloc ; 
	if ( curloc < curlimit ) 
	if ( bytemem [ curloc ] == 142 ) 
	curupd = false ; 
	if ( nrecur == recurused ) 
	if ( recurused == maxrecursion ) 
	{
	  (void) fprintf( stdout , "%s\n",  " !Infinite VF recursion?" ) ; 
	  {register integer for_end; k = maxrecursion ; for_end = 0 ; if ( k 
	  >= for_end) do 
	    {
	      (void) fprintf( stdout , "%s%ld%s",  "level=" , (long)k , " font" ) ; 
	      printfont ( recurfnt [ k ] ) ; 
	      (void) fprintf( stdout , "%s%ld",  " char=" , (long)recurres [ k ] ) ; 
	      if ( recurext [ k ] != 0 ) 
	      (void) fprintf( stdout , "%c%ld",  '.' , (long)recurext [ k ] ) ; 
	      (void) putc('\n',  stdout );
	    } 
	  while ( k-- > for_end ) ; } 
	  overflow ( strrecursion , maxrecursion ) ; 
	} 
	else incr ( recurused ) ; 
	incr ( nrecur ) ; 
	dovfpacket () ; 
	decr ( nrecur ) ; 
	curloc = recurloc [ nrecur ] ; 
	curlimit = savelimit ; 
      } 
      else dochar () ; 
    } 
    break ; 
  case 1 : 
    dorule () ; 
    break ; 
  case 2 : 
    {
      if ( maxbytes - byteptr < curparm ) 
      overflow ( strbytes , maxbytes ) ; 
      while ( curparm > 0 ) {
	  
	{
	  bytemem [ byteptr ] = pcktubyte () ; 
	  incr ( byteptr ) ; 
	} 
	decr ( curparm ) ; 
      } 
      doxxx () ; 
    } 
    break ; 
  case 3 : 
    dopush () ; 
    break ; 
  case 4 : 
    dopop () ; 
    break ; 
  case 5 : 
  case 6 : 
  case 7 : 
  case 8 : 
  case 9 : 
    doright () ; 
    break ; 
  case 10 : 
  case 11 : 
  case 12 : 
  case 13 : 
  case 14 : 
    dodown () ; 
    break ; 
  case 15 : 
    curfnt = curparm ; 
    break ; 
    default: 
    confusion ( strpackets ) ; 
    break ; 
  } 
  if ( curloc < curlimit ) 
  goto lab22 ; 
  lab30: if ( saveupd ) 
  {
    curhdimen = widths [ savewp ] ; 
    dowidth () ; 
  } 
  curfnt = recurfnt [ nrecur ] ; 
} 
void dodvi ( ) 
{/* 30 10 */ int8u tempbyte  ; 
  integer tempint  ; 
  integer dvistart  ; 
  integer dviboppost  ; 
  integer dviback  ; 
  int15 k  ; 
  if ( dviubyte () != 247 ) 
  baddvi () ; 
  if ( dviubyte () != 2 ) 
  baddvi () ; 
  dvinum = dvipquad () ; 
  dviden = dvipquad () ; 
  dvimag = dvipquad () ; 
  tfmconv = ( 25400000.0 / ((double) dvinum ) ) * ( dviden / ((double) 
  473628672L ) ) / ((double) 16.0 ) ; 
  tempbyte = dviubyte () ; 
  if ( maxbytes - byteptr < tempbyte ) 
  overflow ( strbytes , maxbytes ) ; 
  {register integer for_end; k = 1 ; for_end = tempbyte ; if ( k <= for_end) 
  do 
    {
      bytemem [ byteptr ] = dviubyte () ; 
      incr ( byteptr ) ; 
    } 
  while ( k++ < for_end ) ; } 
  (void) Fputs( stdout ,  "DVI file: '" ) ; 
  printpacket ( newpacket () ) ; 
  (void) fprintf( stdout , "%s\n",  "'," ) ; 
  (void) fprintf( stdout , "%s%ld%s%ld%s%ld",  "   num=" , (long)dvinum , ", den=" , (long)dviden , ", mag=" , (long)dvimag   ) ; 
  if ( outmag <= 0 ) 
  outmag = dvimag ; 
  else
  (void) fprintf( stdout , "%s%ld",  " => " , (long)outmag ) ; 
  (void) fprintf( stdout , "%c\n",  '.' ) ; 
  dopre () ; 
  flushpacket () ; 
  if ( true ) 
  {
    dvistart = dviloc ; 
    tempint = dvilength () - 5 ; 
    do {
	if ( tempint < 49 ) 
      baddvi () ; 
      dvimove ( tempint ) ; 
      tempbyte = dviubyte () ; 
      decr ( tempint ) ; 
    } while ( ! ( tempbyte != 223 ) ) ; 
    if ( tempbyte != 2 ) 
    baddvi () ; 
    dvimove ( tempint - 4 ) ; 
    if ( dviubyte () != 249 ) 
    baddvi () ; 
    dviboppost = dvipointer () ; 
    if ( ( dviboppost < 15 ) || ( dviboppost > dviloc - 34 ) ) 
    baddvi () ; 
    dvimove ( dviboppost ) ; 
    if ( dviubyte () != 248 ) 
    baddvi () ; 
    dviback = dvipointer () ; 
    if ( dvinum != dvipquad () ) 
    baddvi () ; 
    if ( dviden != dvipquad () ) 
    baddvi () ; 
    if ( dvimag != dvipquad () ) 
    baddvi () ; 
    tempint = dvisquad () ; 
    tempint = dvisquad () ; 
    if ( stacksize < dviupair () ) 
    overflow ( strstack , stacksize ) ; 
    tempint = dviupair () ; 
    dvifirstpar () ; 
    while ( curclass == 16 ) {
	
      dvidofont ( false ) ; 
      dvifirstpar () ; 
    } 
    if ( curcmd != 249 ) 
    baddvi () ; 
    if ( ! selected ) 
    {
      dvistart = dviboppost ; 
      while ( dviback != -1 ) {
	  
	if ( ( dviback < 15 ) || ( dviback > dviboppost - 46 ) ) 
	baddvi () ; 
	dviboppost = dviback ; 
	dvimove ( dviback ) ; 
	if ( dviubyte () != 139 ) 
	baddvi () ; 
	{register integer for_end; k = 0 ; for_end = 9 ; if ( k <= for_end) 
	do 
	  count [ k ] = dvisquad () ; 
	while ( k++ < for_end ) ; } 
	if ( startmatch () ) 
	dvistart = dviboppost ; 
	dviback = dvipointer () ; 
      } 
    } 
    dvimove ( dvistart ) ; 
  } 
  do {
      dvifirstpar () ; 
    while ( curclass == 16 ) {
	
      dvidofont ( true ) ; 
      dvifirstpar () ; 
    } 
    if ( curcmd == 139 ) 
    {
      {register integer for_end; k = 0 ; for_end = 9 ; if ( k <= for_end) do 
	count [ k ] = dvisquad () ; 
      while ( k++ < for_end ) ; } 
      tempint = dvipointer () ; 
      dobop () ; 
      dvifirstpar () ; 
      if ( typesetting ) 
      while ( true ) {
	  
	switch ( curclass ) 
	{case 0 : 
	  {
	    curwp = 0 ; 
	    if ( curfnt != maxfonts ) 
	    if ( ( curres >= fntbc [ curfnt ] ) && ( curres <= fntec [ curfnt 
	    ] ) ) 
	    curwp = charwidths [ fntchars [ curfnt ] + curres ] ; 
	    if ( curwp == 0 ) 
	    baddvi () ; 
	    if ( fnttype [ curfnt ] == 0 ) 
	    dofont () ; 
	    if ( fnttype [ curfnt ] == 1 ) 
	    dovfpacket () ; 
	    else dochar () ; 
	  } 
	  break ; 
	case 1 : 
	  if ( curupd && ( curvdimen == widthdimen ) ) 
	  {
	    dowidth () ; 
	  } 
	  else dorule () ; 
	  break ; 
	case 2 : 
	  {
	    if ( maxbytes - byteptr < curparm ) 
	    overflow ( strbytes , maxbytes ) ; 
	    while ( curparm > 0 ) {
		
	      {
		bytemem [ byteptr ] = dviubyte () ; 
		incr ( byteptr ) ; 
	      } 
	      decr ( curparm ) ; 
	    } 
	    doxxx () ; 
	  } 
	  break ; 
	case 3 : 
	  dopush () ; 
	  break ; 
	case 4 : 
	  dopop () ; 
	  break ; 
	case 5 : 
	case 6 : 
	case 7 : 
	case 8 : 
	case 9 : 
	  doright () ; 
	  break ; 
	case 10 : 
	case 11 : 
	case 12 : 
	case 13 : 
	case 14 : 
	  dodown () ; 
	  break ; 
	case 15 : 
	  dvifont () ; 
	  break ; 
	case 16 : 
	  dvidofont ( true ) ; 
	  break ; 
	case 17 : 
	  goto lab30 ; 
	  break ; 
	} 
	dvifirstpar () ; 
      } 
      else while ( true ) {
	  
	switch ( curclass ) 
	{case 2 : 
	  while ( curparm > 0 ) {
	      
	    tempbyte = dviubyte () ; 
	    decr ( curparm ) ; 
	  } 
	  break ; 
	case 16 : 
	  dvidofont ( true ) ; 
	  break ; 
	case 17 : 
	  goto lab30 ; 
	  break ; 
	  default: 
	  ; 
	  break ; 
	} 
	dvifirstpar () ; 
      } 
      lab30: if ( curcmd != 140 ) 
      baddvi () ; 
      if ( typesetting ) 
      {
	doeop () ; 
	if ( alldone ) 
	goto lab10 ; 
      } 
    } 
  } while ( ! ( curcmd != 140 ) ) ; 
  if ( curcmd != 248 ) 
  baddvi () ; 
  lab10: ; 
} 
void closefilesandterminate ( ) 
{int15 k  ; 
  if ( history < 3 ) 
  {
    if ( typesetting ) 
    {
      while ( stackptr > 0 ) {
	  
	{
	  putbyte ( 142 , outfile ) ; 
	  incr ( outloc ) ; 
	} 
	decr ( stackptr ) ; 
      } 
      {
	putbyte ( 140 , outfile ) ; 
	incr ( outloc ) ; 
      } 
    } 
    if ( outloc > 0 ) 
    {
      {
	putbyte ( 248 , outfile ) ; 
	incr ( outloc ) ; 
      } 
      outfour ( outback ) ; 
      outback = outloc - 5 ; 
      outfour ( dvinum ) ; 
      outfour ( dviden ) ; 
      outfour ( outmag ) ; 
      outfour ( outmaxv ) ; 
      outfour ( outmaxh ) ; 
      {
	putbyte ( outstack / 256 , outfile ) ; 
	incr ( outloc ) ; 
      } 
      {
	putbyte ( outstack % 256 , outfile ) ; 
	incr ( outloc ) ; 
      } 
      {
	putbyte ( outpages / 256 , outfile ) ; 
	incr ( outloc ) ; 
      } 
      {
	putbyte ( outpages % 256 , outfile ) ; 
	incr ( outloc ) ; 
      } 
      k = outnf ; 
      while ( k > 0 ) {
	  
	decr ( k ) ; 
	outfntdef ( outfnts [ k ] ) ; 
      } 
      {
	putbyte ( 249 , outfile ) ; 
	incr ( outloc ) ; 
      } 
      outfour ( outback ) ; 
      {
	putbyte ( 2 , outfile ) ; 
	incr ( outloc ) ; 
      } 
      k = 7 - ( ( outloc - 1 ) % 4 ) ; 
      while ( k > 0 ) {
	  
	{
	  putbyte ( 223 , outfile ) ; 
	  incr ( outloc ) ; 
	} 
	decr ( k ) ; 
      } 
      (void) fprintf( stdout , "%s%ld%s%ld%s",  "OUT file: " , (long)outloc , " bytes, " , (long)outpages , " page"       ) ; 
      if ( outpages != 1 ) 
      (void) putc( 's' ,  stdout );
    } 
    else
    (void) Fputs( stdout ,  "OUT file: no output" ) ; 
    (void) fprintf( stdout , "%s\n",  " written." ) ; 
    if ( outpages == 0 ) 
    if ( history == 0 ) 
    history = 1 ; 
  } 
  switch ( history ) 
  {case 0 : 
    (void) fprintf( stdout , "%s\n",  "(No errors were found.)" ) ; 
    break ; 
  case 1 : 
    (void) fprintf( stdout , "%s\n",  "(Did you see the warning message above?)" ) ; 
    break ; 
  case 2 : 
    (void) fprintf( stdout , "%s\n",  "(Pardon me, but I think I spotted something wrong.)" ) 
    ; 
    break ; 
  case 3 : 
    (void) fprintf( stdout , "%s\n",  "(That was a fatal error, my friend.)" ) ; 
    break ; 
  } 
} 
void main_body() {
    
  setpaths ( TFMFILEPATHBIT + VFFILEPATHBIT ) ; 
  initialize () ; 
  if ( argc != 3 ) 
  {
    (void) fprintf(stdout, "%s\n", "Usage: dvicopy <dvi file> <outfile>." ) ; 
    uexit ( 1 ) ; 
  } 
  byteptr = byteptr + 5 ; 
  bytemem [ byteptr - 5 ] = 102 ; 
  bytemem [ byteptr - 4 ] = 111 ; 
  bytemem [ byteptr - 3 ] = 110 ; 
  bytemem [ byteptr - 2 ] = 116 ; 
  bytemem [ byteptr - 1 ] = 115 ; 
  strfonts = makepacket () ; 
  byteptr = byteptr + 5 ; 
  bytemem [ byteptr - 5 ] = 99 ; 
  bytemem [ byteptr - 4 ] = 104 ; 
  bytemem [ byteptr - 3 ] = 97 ; 
  bytemem [ byteptr - 2 ] = 114 ; 
  bytemem [ byteptr - 1 ] = 115 ; 
  strchars = makepacket () ; 
  byteptr = byteptr + 6 ; 
  bytemem [ byteptr - 6 ] = 119 ; 
  bytemem [ byteptr - 5 ] = 105 ; 
  bytemem [ byteptr - 4 ] = 100 ; 
  bytemem [ byteptr - 3 ] = 116 ; 
  bytemem [ byteptr - 2 ] = 104 ; 
  bytemem [ byteptr - 1 ] = 115 ; 
  strwidths = makepacket () ; 
  byteptr = byteptr + 7 ; 
  bytemem [ byteptr - 7 ] = 112 ; 
  bytemem [ byteptr - 6 ] = 97 ; 
  bytemem [ byteptr - 5 ] = 99 ; 
  bytemem [ byteptr - 4 ] = 107 ; 
  bytemem [ byteptr - 3 ] = 101 ; 
  bytemem [ byteptr - 2 ] = 116 ; 
  bytemem [ byteptr - 1 ] = 115 ; 
  strpackets = makepacket () ; 
  byteptr = byteptr + 5 ; 
  bytemem [ byteptr - 5 ] = 98 ; 
  bytemem [ byteptr - 4 ] = 121 ; 
  bytemem [ byteptr - 3 ] = 116 ; 
  bytemem [ byteptr - 2 ] = 101 ; 
  bytemem [ byteptr - 1 ] = 115 ; 
  strbytes = makepacket () ; 
  byteptr = byteptr + 9 ; 
  bytemem [ byteptr - 9 ] = 114 ; 
  bytemem [ byteptr - 8 ] = 101 ; 
  bytemem [ byteptr - 7 ] = 99 ; 
  bytemem [ byteptr - 6 ] = 117 ; 
  bytemem [ byteptr - 5 ] = 114 ; 
  bytemem [ byteptr - 4 ] = 115 ; 
  bytemem [ byteptr - 3 ] = 105 ; 
  bytemem [ byteptr - 2 ] = 111 ; 
  bytemem [ byteptr - 1 ] = 110 ; 
  strrecursion = makepacket () ; 
  byteptr = byteptr + 5 ; 
  bytemem [ byteptr - 5 ] = 115 ; 
  bytemem [ byteptr - 4 ] = 116 ; 
  bytemem [ byteptr - 3 ] = 97 ; 
  bytemem [ byteptr - 2 ] = 99 ; 
  bytemem [ byteptr - 1 ] = 107 ; 
  strstack = makepacket () ; 
  byteptr = byteptr + 10 ; 
  bytemem [ byteptr - 10 ] = 110 ; 
  bytemem [ byteptr - 9 ] = 97 ; 
  bytemem [ byteptr - 8 ] = 109 ; 
  bytemem [ byteptr - 7 ] = 101 ; 
  bytemem [ byteptr - 6 ] = 108 ; 
  bytemem [ byteptr - 5 ] = 101 ; 
  bytemem [ byteptr - 4 ] = 110 ; 
  bytemem [ byteptr - 3 ] = 103 ; 
  bytemem [ byteptr - 2 ] = 116 ; 
  bytemem [ byteptr - 1 ] = 104 ; 
  strnamelength = makepacket () ; 
  byteptr = byteptr + 4 ; 
  bytemem [ byteptr - 4 ] = 46 ; 
  bytemem [ byteptr - 3 ] = 116 ; 
  bytemem [ byteptr - 2 ] = 102 ; 
  bytemem [ byteptr - 1 ] = 109 ; 
  tfmext = makepacket () ; 
  byteptr = byteptr + 3 ; 
  bytemem [ byteptr - 3 ] = 46 ; 
  bytemem [ byteptr - 2 ] = 118 ; 
  bytemem [ byteptr - 1 ] = 102 ; 
  vfext = makepacket () ; 
  byteptr = byteptr + 3 ; 
  bytemem [ byteptr - 3 ] = 109 ; 
  bytemem [ byteptr - 2 ] = 97 ; 
  bytemem [ byteptr - 1 ] = 103 ; 
  strmag = makepacket () ; 
  byteptr = byteptr + 6 ; 
  bytemem [ byteptr - 6 ] = 115 ; 
  bytemem [ byteptr - 5 ] = 101 ; 
  bytemem [ byteptr - 4 ] = 108 ; 
  bytemem [ byteptr - 3 ] = 101 ; 
  bytemem [ byteptr - 2 ] = 99 ; 
  bytemem [ byteptr - 1 ] = 116 ; 
  strselect = makepacket () ; 
  dialog () ; 
  argv ( toint ( 1 ) , curname ) ; 
  reset ( dvifile , curname ) ; 
  dviloc = 0 ; 
  argv ( toint ( 2 ) , outfname ) ; 
  rewrite ( outfile , outfname ) ; 
  dodvi () ; 
  closefilesandterminate () ; 
} 
