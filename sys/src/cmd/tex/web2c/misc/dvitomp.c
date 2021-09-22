#define DVITOMP
#include "cpascal.h"
/* 9999 30 */ 
#define maxfonts ( 100 ) 
#define maxfnums ( 300 ) 
#define maxwidths ( 10000 ) 
#define virtualspace ( 10000 ) 
#define linelength ( 79 ) 
#define stacksize ( 100 ) 
#define namesize ( 1000 ) 
#define namelength ( 50 ) 
typedef unsigned char ASCIIcode  ;
typedef text /* of  ASCIIcode */ textfile  ;
typedef unsigned char eightbits  ;
typedef text /* of  eightbits */ bytefile  ;
typedef unsigned char quarterword  ;
char history  ;
textfile mpxfile  ;
ASCIIcode xchr[256]  ;
bytefile dvifile  ;
bytefile tfmfile  ;
bytefile vffile  ;
integer downthedrain  ;
char * curname  ;
eightbits b0, b1, b2, b3  ;
boolean vfreading  ;
quarterword cmdbuf[virtualspace + 1]  ;
integer bufptr  ;
integer fontnum[maxfnums + 1]  ;
integer internalnum[maxfnums + 1]  ;
boolean localonly[maxfonts + 1]  ;
integer fontname[maxfonts + 1]  ;
ASCIIcode names[namesize + 1]  ;
integer arealength[maxfonts + 1]  ;
real fontscaledsize[maxfonts + 1]  ;
real fontdesignsize[maxfonts + 1]  ;
integer fontchecksum[maxfonts + 1]  ;
integer fontbc[maxfonts + 1]  ;
integer fontec[maxfonts + 1]  ;
integer infobase[maxfonts + 1]  ;
integer width[maxwidths + 1]  ;
integer fbase[maxfonts + 1]  ;
integer ftop[maxfonts + 1]  ;
integer cmdptr[maxwidths + 1]  ;
integer nf  ;
integer vfptr  ;
integer infoptr  ;
integer ncmds  ;
integer curfbase, curftop  ;
real dviperfix  ;
integer inwidth[256]  ;
integer tfmchecksum  ;
char state  ;
integer printcol  ;
integer h, v  ;
real conv  ;
real mag  ;
boolean fontused[maxfonts + 1]  ;
boolean fontsused  ;
boolean rulesused  ;
integer strh1, strv  ;
integer strh2  ;
integer strf  ;
real strscale  ;
integer picdp, picht, picwd  ;
integer w, x, y, z  ;
integer hstack[stacksize + 1], vstack[stacksize + 1], wstack[stacksize + 1], 
xstack[stacksize + 1], ystack[stacksize + 1], zstack[stacksize + 1]  ;
integer stksiz  ;
real dviscale  ;
integer k, p  ;
integer numerator, denominator  ;
cstring dviname, mpxname  ;

#include "dvitomp.h"
void 
#ifdef HAVE_PROTOTYPES
parsearguments ( void ) 
#else
parsearguments ( ) 
#endif
{
  
#define noptions ( 2 ) 
  getoptstruct longoptions[noptions + 1]  ;
  integer getoptreturnval  ;
  cinttype optionindex  ;
  integer currentoption  ;
  currentoption = 0 ;
  longoptions [currentoption ].name = "help" ;
  longoptions [currentoption ].hasarg = 0 ;
  longoptions [currentoption ].flag = 0 ;
  longoptions [currentoption ].val = 0 ;
  currentoption = currentoption + 1 ;
  longoptions [currentoption ].name = "version" ;
  longoptions [currentoption ].hasarg = 0 ;
  longoptions [currentoption ].flag = 0 ;
  longoptions [currentoption ].val = 0 ;
  currentoption = currentoption + 1 ;
  longoptions [currentoption ].name = 0 ;
  longoptions [currentoption ].hasarg = 0 ;
  longoptions [currentoption ].flag = 0 ;
  longoptions [currentoption ].val = 0 ;
  do {
      getoptreturnval = getoptlongonly ( argc , argv , "" , longoptions , 
    addressof ( optionindex ) ) ;
    if ( getoptreturnval == -1 ) 
    {
      ;
    } 
    else if ( getoptreturnval == 63 ) 
    {
      usage ( 1 , "dvitomp" ) ;
    } 
    else if ( ( strcmp ( longoptions [optionindex ].name , "help" ) == 0 ) ) 
    {
      usage ( 0 , DVITOMPHELP ) ;
    } 
    else if ( ( strcmp ( longoptions [optionindex ].name , "version" ) == 0 
    ) ) 
    {
      printversionandexit ( "This is DVItoMP, Version 0.64" , 
      "AT&T Bell Laboraties" , "John Hobby" ) ;
    } 
  } while ( ! ( getoptreturnval == -1 ) ) ;
  if ( ( optind + 1 != argc ) && ( optind + 2 != argc ) ) 
  {
    fprintf( stderr , "%s\n",  "dvitomp: Need one or two file arguments." ) ;
    usage ( 1 , "dvitomp" ) ;
  } 
  dviname = cmdline ( optind ) ;
  if ( optind + 2 <= argc ) 
  {
    mpxname = cmdline ( optind + 1 ) ;
  } 
  else {
      
    mpxname = basenamechangesuffix ( dviname , ".dvi" , ".mpx" ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
initialize ( void ) 
#else
initialize ( ) 
#endif
{
  integer i  ;
  kpsesetprogname ( argv [0 ]) ;
  parsearguments () ;
  history = 0 ;
  {register integer for_end; i = 0 ;for_end = 31 ; if ( i <= for_end) do 
    xchr [i ]= '?' ;
  while ( i++ < for_end ) ;} 
  xchr [32 ]= ' ' ;
  xchr [33 ]= '!' ;
  xchr [34 ]= '"' ;
  xchr [35 ]= '#' ;
  xchr [36 ]= '$' ;
  xchr [37 ]= '%' ;
  xchr [38 ]= '&' ;
  xchr [39 ]= '\'' ;
  xchr [40 ]= '(' ;
  xchr [41 ]= ')' ;
  xchr [42 ]= '*' ;
  xchr [43 ]= '+' ;
  xchr [44 ]= ',' ;
  xchr [45 ]= '-' ;
  xchr [46 ]= '.' ;
  xchr [47 ]= '/' ;
  xchr [48 ]= '0' ;
  xchr [49 ]= '1' ;
  xchr [50 ]= '2' ;
  xchr [51 ]= '3' ;
  xchr [52 ]= '4' ;
  xchr [53 ]= '5' ;
  xchr [54 ]= '6' ;
  xchr [55 ]= '7' ;
  xchr [56 ]= '8' ;
  xchr [57 ]= '9' ;
  xchr [58 ]= ':' ;
  xchr [59 ]= ';' ;
  xchr [60 ]= '<' ;
  xchr [61 ]= '=' ;
  xchr [62 ]= '>' ;
  xchr [63 ]= '?' ;
  xchr [64 ]= '@' ;
  xchr [65 ]= 'A' ;
  xchr [66 ]= 'B' ;
  xchr [67 ]= 'C' ;
  xchr [68 ]= 'D' ;
  xchr [69 ]= 'E' ;
  xchr [70 ]= 'F' ;
  xchr [71 ]= 'G' ;
  xchr [72 ]= 'H' ;
  xchr [73 ]= 'I' ;
  xchr [74 ]= 'J' ;
  xchr [75 ]= 'K' ;
  xchr [76 ]= 'L' ;
  xchr [77 ]= 'M' ;
  xchr [78 ]= 'N' ;
  xchr [79 ]= 'O' ;
  xchr [80 ]= 'P' ;
  xchr [81 ]= 'Q' ;
  xchr [82 ]= 'R' ;
  xchr [83 ]= 'S' ;
  xchr [84 ]= 'T' ;
  xchr [85 ]= 'U' ;
  xchr [86 ]= 'V' ;
  xchr [87 ]= 'W' ;
  xchr [88 ]= 'X' ;
  xchr [89 ]= 'Y' ;
  xchr [90 ]= 'Z' ;
  xchr [91 ]= '[' ;
  xchr [92 ]= '\\' ;
  xchr [93 ]= ']' ;
  xchr [94 ]= '^' ;
  xchr [95 ]= '_' ;
  xchr [96 ]= '`' ;
  xchr [97 ]= 'a' ;
  xchr [98 ]= 'b' ;
  xchr [99 ]= 'c' ;
  xchr [100 ]= 'd' ;
  xchr [101 ]= 'e' ;
  xchr [102 ]= 'f' ;
  xchr [103 ]= 'g' ;
  xchr [104 ]= 'h' ;
  xchr [105 ]= 'i' ;
  xchr [106 ]= 'j' ;
  xchr [107 ]= 'k' ;
  xchr [108 ]= 'l' ;
  xchr [109 ]= 'm' ;
  xchr [110 ]= 'n' ;
  xchr [111 ]= 'o' ;
  xchr [112 ]= 'p' ;
  xchr [113 ]= 'q' ;
  xchr [114 ]= 'r' ;
  xchr [115 ]= 's' ;
  xchr [116 ]= 't' ;
  xchr [117 ]= 'u' ;
  xchr [118 ]= 'v' ;
  xchr [119 ]= 'w' ;
  xchr [120 ]= 'x' ;
  xchr [121 ]= 'y' ;
  xchr [122 ]= 'z' ;
  xchr [123 ]= '{' ;
  xchr [124 ]= '|' ;
  xchr [125 ]= '}' ;
  xchr [126 ]= '~' ;
  {register integer for_end; i = 127 ;for_end = 255 ; if ( i <= for_end) do 
    xchr [i ]= '?' ;
  while ( i++ < for_end ) ;} 
  vfreading = false ;
  bufptr = virtualspace ;
  nf = 0 ;
  infoptr = 0 ;
  fontname [0 ]= 0 ;
  vfptr = maxfnums ;
  curfbase = 0 ;
  curftop = 0 ;
  state = 2 ;
} 
void 
#ifdef HAVE_PROTOTYPES
openmpxfile ( void ) 
#else
openmpxfile ( ) 
#endif
{
  curname = extendfilename ( mpxname , "mpx" ) ;
  rewrite ( mpxfile , curname ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
opendvifile ( void ) 
#else
opendvifile ( ) 
#endif
{
  curname = extendfilename ( dviname , "dvi" ) ;
  resetbin ( dvifile , curname ) ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
opentfmfile ( void ) 
#else
opentfmfile ( ) 
#endif
{
  register boolean Result; tfmfile = kpseopenfile ( curname , kpsetfmformat 
  ) ;
  free ( curname ) ;
  Result = true ;
  return Result ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
openvffile ( void ) 
#else
openvffile ( ) 
#endif
{
  register boolean Result; char * fullname  ;
  fullname = kpsefindvf ( curname ) ;
  if ( fullname ) 
  {
    resetbin ( vffile , fullname ) ;
    free ( curname ) ;
    free ( fullname ) ;
    Result = true ;
  } 
  else Result = false ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
readtfmword ( void ) 
#else
readtfmword ( ) 
#endif
{
  read ( tfmfile , b0 ) ;
  read ( tfmfile , b1 ) ;
  read ( tfmfile , b2 ) ;
  read ( tfmfile , b3 ) ;
} 
integer 
#ifdef HAVE_PROTOTYPES
getbyte ( void ) 
#else
getbyte ( ) 
#endif
{
  register integer Result; eightbits b  ;
  if ( vfreading ) 
  read ( vffile , b ) ;
  else if ( bufptr == virtualspace ) 
  read ( dvifile , b ) ;
  else {
      
    b = cmdbuf [bufptr ];
    bufptr = bufptr + 1 ;
  } 
  Result = b ;
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
signedbyte ( void ) 
#else
signedbyte ( ) 
#endif
{
  register integer Result; eightbits b  ;
  if ( vfreading ) 
  read ( vffile , b ) ;
  else if ( bufptr == virtualspace ) 
  read ( dvifile , b ) ;
  else {
      
    b = cmdbuf [bufptr ];
    bufptr = bufptr + 1 ;
  } 
  if ( b < 128 ) 
  Result = b ;
  else Result = b - 256 ;
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
gettwobytes ( void ) 
#else
gettwobytes ( ) 
#endif
{
  register integer Result; eightbits a, b  ;
  if ( vfreading ) 
  {
    read ( vffile , a ) ;
    read ( vffile , b ) ;
  } 
  else if ( bufptr == virtualspace ) 
  {
    read ( dvifile , a ) ;
    read ( dvifile , b ) ;
  } 
  else if ( bufptr + 2 > ncmds ) 
  {
    fprintf(stdout, "%s%s\n", "DVItoMP abort: " ,     "Error detected while interpreting a virtual font" ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  else {
      
    a = cmdbuf [bufptr ];
    b = cmdbuf [bufptr + 1 ];
    bufptr = bufptr + 2 ;
  } 
  Result = a * toint ( 256 ) + b ;
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
signedpair ( void ) 
#else
signedpair ( ) 
#endif
{
  register integer Result; eightbits a, b  ;
  if ( vfreading ) 
  {
    read ( vffile , a ) ;
    read ( vffile , b ) ;
  } 
  else if ( bufptr == virtualspace ) 
  {
    read ( dvifile , a ) ;
    read ( dvifile , b ) ;
  } 
  else if ( bufptr + 2 > ncmds ) 
  {
    fprintf(stdout, "%s%s\n", "DVItoMP abort: " ,     "Error detected while interpreting a virtual font" ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  else {
      
    a = cmdbuf [bufptr ];
    b = cmdbuf [bufptr + 1 ];
    bufptr = bufptr + 2 ;
  } 
  if ( a < 128 ) 
  Result = a * 256 + b ;
  else Result = ( a - 256 ) * 256 + b ;
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
getthreebytes ( void ) 
#else
getthreebytes ( ) 
#endif
{
  register integer Result; eightbits a, b, c  ;
  if ( vfreading ) 
  {
    read ( vffile , a ) ;
    read ( vffile , b ) ;
    read ( vffile , c ) ;
  } 
  else if ( bufptr == virtualspace ) 
  {
    read ( dvifile , a ) ;
    read ( dvifile , b ) ;
    read ( dvifile , c ) ;
  } 
  else if ( bufptr + 3 > ncmds ) 
  {
    fprintf(stdout, "%s%s\n", "DVItoMP abort: " ,     "Error detected while interpreting a virtual font" ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  else {
      
    a = cmdbuf [bufptr ];
    b = cmdbuf [bufptr + 1 ];
    c = cmdbuf [bufptr + 2 ];
    bufptr = bufptr + 3 ;
  } 
  Result = ( a * toint ( 256 ) + b ) * 256 + c ;
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
signedtrio ( void ) 
#else
signedtrio ( ) 
#endif
{
  register integer Result; eightbits a, b, c  ;
  if ( vfreading ) 
  {
    read ( vffile , a ) ;
    read ( vffile , b ) ;
    read ( vffile , c ) ;
  } 
  else if ( bufptr == virtualspace ) 
  {
    read ( dvifile , a ) ;
    read ( dvifile , b ) ;
    read ( dvifile , c ) ;
  } 
  else if ( bufptr + 3 > ncmds ) 
  {
    fprintf(stdout, "%s%s\n", "DVItoMP abort: " ,     "Error detected while interpreting a virtual font" ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  else {
      
    a = cmdbuf [bufptr ];
    b = cmdbuf [bufptr + 1 ];
    c = cmdbuf [bufptr + 2 ];
    bufptr = bufptr + 3 ;
  } 
  if ( a < 128 ) 
  Result = ( a * toint ( 256 ) + b ) * 256 + c ;
  else Result = ( ( a - toint ( 256 ) ) * 256 + b ) * 256 + c ;
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
signedquad ( void ) 
#else
signedquad ( ) 
#endif
{
  register integer Result; eightbits a, b, c, d  ;
  if ( vfreading ) 
  {
    read ( vffile , a ) ;
    read ( vffile , b ) ;
    read ( vffile , c ) ;
    read ( vffile , d ) ;
  } 
  else if ( bufptr == virtualspace ) 
  {
    read ( dvifile , a ) ;
    read ( dvifile , b ) ;
    read ( dvifile , c ) ;
    read ( dvifile , d ) ;
  } 
  else if ( bufptr + 4 > ncmds ) 
  {
    fprintf(stdout, "%s%s\n", "DVItoMP abort: " ,     "Error detected while interpreting a virtual font" ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  else {
      
    a = cmdbuf [bufptr ];
    b = cmdbuf [bufptr + 1 ];
    c = cmdbuf [bufptr + 2 ];
    d = cmdbuf [bufptr + 3 ];
    bufptr = bufptr + 4 ;
  } 
  if ( a < 128 ) 
  Result = ( ( a * toint ( 256 ) + b ) * 256 + c ) * 256 + d ;
  else Result = ( ( ( a - 256 ) * toint ( 256 ) + b ) * 256 + c ) * 256 + d ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintchar ( eightbits c ) 
#else
zprintchar ( c ) 
  eightbits c ;
#endif
{
  boolean printable  ;
  integer l  ;
  printable = ( c >= 32 ) && ( c <= 126 ) && ( c != 34 ) ;
  if ( printable ) 
  l = 1 ;
  else if ( c < 10 ) 
  l = 5 ;
  else if ( c < 100 ) 
  l = 6 ;
  else l = 7 ;
  if ( printcol + l > linelength - 2 ) 
  {
    if ( state == 1 ) 
    {
      putc ( '"' ,  mpxfile );
      state = 0 ;
    } 
    fprintf( mpxfile , "%c\n",  ' ' ) ;
    printcol = 0 ;
  } 
  if ( state == 1 ) 
  if ( printable ) 
  putc ( xchr [c ],  mpxfile );
  else {
      
    fprintf( mpxfile , "%s%ld",  "\"&char" , (long)c ) ;
    printcol = printcol + 2 ;
  } 
  else {
      
    if ( state == 0 ) 
    {
      putc ( '&' ,  mpxfile );
      printcol = printcol + 1 ;
    } 
    if ( printable ) 
    {
      fprintf( mpxfile , "%c%c",  '"' , xchr [c ]) ;
      printcol = printcol + 1 ;
    } 
    else
    fprintf( mpxfile , "%s%ld",  "char" , (long)c ) ;
  } 
  printcol = printcol + l ;
  if ( printable ) 
  state = 1 ;
  else state = 0 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zendcharstring ( integer l ) 
#else
zendcharstring ( l ) 
  integer l ;
#endif
{
  while ( state > 0 ) {
      
    putc ( '"' ,  mpxfile );
    printcol = printcol + 1 ;
    state = state - 1 ;
  } 
  if ( printcol + l > linelength ) 
  {
    fprintf( mpxfile , "%c\n",  ' ' ) ;
    printcol = 0 ;
  } 
  state = 2 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintfont ( integer f ) 
#else
zprintfont ( f ) 
  integer f ;
#endif
{
  integer k  ;
  if ( ( f < 0 ) || ( f >= nf ) ) 
  {
    fprintf(stdout, "%s%s%s%c\n", "DVItoMP abort: " , "Bad DVI file: " , "Undefined font" , '!' ) 
    ;
    history = 3 ;
    uexit ( history ) ;
  } 
  else {
      
    {register integer for_end; k = fontname [f ];for_end = fontname [f + 
    1 ]- 1 ; if ( k <= for_end) do 
      printchar ( names [k ]) ;
    while ( k++ < for_end ) ;} 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zerrprintfont ( integer f ) 
#else
zerrprintfont ( f ) 
  integer f ;
#endif
{
  integer k  ;
  {register integer for_end; k = fontname [f ];for_end = fontname [f + 1 
  ]- 1 ; if ( k <= for_end) do 
    putc (xchr [names [k ]], stdout);
  while ( k++ < for_end ) ;} 
  fprintf(stdout, "%c\n", ' ' ) ;
} 
integer 
#ifdef HAVE_PROTOTYPES
zmatchfont ( integer ff , boolean exact ) 
#else
zmatchfont ( ff , exact ) 
  integer ff ;
  boolean exact ;
#endif
{
  /* 30 99 */ register integer Result; integer f  ;
  integer ss, ll  ;
  integer k, s  ;
  ss = fontname [ff ];
  ll = fontname [ff + 1 ]- ss ;
  f = 0 ;
  while ( f < nf ) {
      
    if ( f != ff ) 
    {
      if ( ( arealength [f ]< arealength [ff ]) || ( ll != fontname [f + 
      1 ]- fontname [f ]) ) 
      goto lab99 ;
      s = fontname [f ];
      k = ll ;
      while ( k > 0 ) {
	  
	k = k - 1 ;
	if ( names [s + k ]!= names [ss + k ]) 
	goto lab99 ;
      } 
      if ( exact ) 
      {
	if ( fabs ( fontscaledsize [f ]- fontscaledsize [ff ]) <= 0.00001 
	) 
	{
	  if ( ! vfreading ) 
	  if ( localonly [f ]) 
	  {
	    fontnum [f ]= fontnum [ff ];
	    localonly [f ]= false ;
	  } 
	  else if ( fontnum [f ]!= fontnum [ff ]) 
	  goto lab99 ;
	  goto lab30 ;
	} 
      } 
      else if ( infobase [f ]!= maxwidths ) 
      goto lab30 ;
    } 
    lab99: f = f + 1 ;
  } 
  lab30: if ( f < nf ) 
  if ( fabs ( fontdesignsize [f ]- fontdesignsize [ff ]) > 0.00001 ) 
  {
    fprintf(stdout, "%s%s", "DVItoMP warning: " , "Inconsistent design sizes given for " ) ;
    errprintfont ( ff ) ;
    history = 2 ;
  } 
  else if ( fontchecksum [f ]!= fontchecksum [ff ]) 
  {
    fprintf(stdout, "%s%s", "DVItoMP warning: " , "Checksum mismatch for " ) ;
    errprintfont ( ff ) ;
    history = 2 ;
  } 
  Result = f ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdefinefont ( integer e ) 
#else
zdefinefont ( e ) 
  integer e ;
#endif
{
  integer i  ;
  integer n  ;
  integer k  ;
  integer x  ;
  if ( nf == maxfonts ) 
  {
    fprintf(stdout, "%s%s%ld%s\n", "DVItoMP abort: " , "DVItoMP capacity exceeded (max fonts=" ,     (long)maxfonts , ")!" ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  if ( vfptr == nf ) 
  {
    fprintf(stdout, "%s%s%ld%c\n", "DVItoMP abort: " ,     "DVItoMP capacity exceeded (max font numbers=" , (long)maxfnums , ')' ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  if ( vfreading ) 
  {
    fontnum [nf ]= 0 ;
    i = vfptr ;
    vfptr = vfptr - 1 ;
  } 
  else i = nf ;
  fontnum [i ]= e ;
  fontchecksum [nf ]= signedquad () ;
  x = signedquad () ;
  k = 1 ;
  while ( x > 8388608L ) {
      
    x = x / 2 ;
    k = k + k ;
  } 
  fontscaledsize [nf ]= x * k / ((double) 1048576.0 ) ;
  if ( vfreading ) 
  fontdesignsize [nf ]= signedquad () * dviperfix / ((double) 1048576.0 ) ;
  else fontdesignsize [nf ]= signedquad () / ((double) 1048576.0 ) ;
  n = getbyte () ;
  arealength [nf ]= n ;
  n = n + getbyte () ;
  if ( fontname [nf ]+ n > namesize ) 
  {
    fprintf(stdout, "%s%s%ld%s\n", "DVItoMP abort: " , "DVItoMP capacity exceeded (name size=" ,     (long)namesize , ")!" ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  fontname [nf + 1 ]= fontname [nf ]+ n ;
  {register integer for_end; k = fontname [nf ];for_end = fontname [nf + 
  1 ]- 1 ; if ( k <= for_end) do 
    names [k ]= getbyte () ;
  while ( k++ < for_end ) ;} 
  internalnum [i ]= matchfont ( nf , true ) ;
  if ( internalnum [i ]== nf ) 
  {
    infobase [nf ]= maxwidths ;
    localonly [nf ]= vfreading ;
    nf = nf + 1 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zinTFM ( integer f ) 
#else
zinTFM ( f ) 
  integer f ;
#endif
{
  /* 9997 9999 */ integer k  ;
  integer lh  ;
  integer nw  ;
  integer wp  ;
  readtfmword () ;
  lh = b2 * toint ( 256 ) + b3 ;
  readtfmword () ;
  fontbc [f ]= b0 * toint ( 256 ) + b1 ;
  fontec [f ]= b2 * toint ( 256 ) + b3 ;
  if ( fontec [f ]< fontbc [f ]) 
  fontbc [f ]= fontec [f ]+ 1 ;
  if ( infoptr + fontec [f ]- fontbc [f ]+ 1 > maxwidths ) 
  {
    fprintf(stdout, "%s%s%ld%s\n", "DVItoMP abort: " ,     "DVItoMP capacity exceeded (width table size=" , (long)maxwidths , ")!" ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  wp = infoptr + fontec [f ]- fontbc [f ]+ 1 ;
  readtfmword () ;
  nw = b0 * 256 + b1 ;
  if ( ( nw == 0 ) || ( nw > 256 ) ) 
  goto lab9997 ;
  {register integer for_end; k = 1 ;for_end = 3 + lh ; if ( k <= for_end) do 
    {
      if ( eof ( tfmfile ) ) 
      goto lab9997 ;
      readtfmword () ;
      if ( k == 4 ) 
      if ( b0 < 128 ) 
      tfmchecksum = ( ( b0 * toint ( 256 ) + b1 ) * 256 + b2 ) * 256 + b3 ;
      else tfmchecksum = ( ( ( b0 - 256 ) * toint ( 256 ) + b1 ) * 256 + b2 ) 
      * 256 + b3 ;
    } 
  while ( k++ < for_end ) ;} 
  if ( wp > 0 ) 
  {register integer for_end; k = infoptr ;for_end = wp - 1 ; if ( k <= 
  for_end) do 
    {
      readtfmword () ;
      if ( b0 > nw ) 
      goto lab9997 ;
      width [k ]= b0 ;
    } 
  while ( k++ < for_end ) ;} 
  {register integer for_end; k = 0 ;for_end = nw - 1 ; if ( k <= for_end) do 
    {
      readtfmword () ;
      if ( b0 > 127 ) 
      b0 = b0 - 256 ;
      inwidth [k ]= ( ( b0 * 256 + b1 ) * 256 + b2 ) * 256 + b3 ;
    } 
  while ( k++ < for_end ) ;} 
  if ( inwidth [0 ]!= 0 ) 
  goto lab9997 ;
  infobase [f ]= infoptr - fontbc [f ];
  if ( wp > 0 ) 
  {register integer for_end; k = infoptr ;for_end = wp - 1 ; if ( k <= 
  for_end) do 
    width [k ]= inwidth [width [k ]];
  while ( k++ < for_end ) ;} 
  fbase [f ]= 0 ;
  ftop [f ]= 0 ;
  infoptr = wp ;
  goto lab9999 ;
  lab9997: {
      
    fprintf(stdout, "%s%s", "DVItoMP abort: " , "Bad TFM file for " ) ;
    errprintfont ( f ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  lab9999: ;
} 
integer 
#ifdef HAVE_PROTOTYPES
zfirstpar ( eightbits o ) 
#else
zfirstpar ( o ) 
  eightbits o ;
#endif
{
  register integer Result; switch ( o ) 
  {case 0 : 
  case 1 : 
  case 2 : 
  case 3 : 
  case 4 : 
  case 5 : 
  case 6 : 
  case 7 : 
  case 8 : 
  case 9 : 
  case 10 : 
  case 11 : 
  case 12 : 
  case 13 : 
  case 14 : 
  case 15 : 
  case 16 : 
  case 17 : 
  case 18 : 
  case 19 : 
  case 20 : 
  case 21 : 
  case 22 : 
  case 23 : 
  case 24 : 
  case 25 : 
  case 26 : 
  case 27 : 
  case 28 : 
  case 29 : 
  case 30 : 
  case 31 : 
  case 32 : 
  case 33 : 
  case 34 : 
  case 35 : 
  case 36 : 
  case 37 : 
  case 38 : 
  case 39 : 
  case 40 : 
  case 41 : 
  case 42 : 
  case 43 : 
  case 44 : 
  case 45 : 
  case 46 : 
  case 47 : 
  case 48 : 
  case 49 : 
  case 50 : 
  case 51 : 
  case 52 : 
  case 53 : 
  case 54 : 
  case 55 : 
  case 56 : 
  case 57 : 
  case 58 : 
  case 59 : 
  case 60 : 
  case 61 : 
  case 62 : 
  case 63 : 
  case 64 : 
  case 65 : 
  case 66 : 
  case 67 : 
  case 68 : 
  case 69 : 
  case 70 : 
  case 71 : 
  case 72 : 
  case 73 : 
  case 74 : 
  case 75 : 
  case 76 : 
  case 77 : 
  case 78 : 
  case 79 : 
  case 80 : 
  case 81 : 
  case 82 : 
  case 83 : 
  case 84 : 
  case 85 : 
  case 86 : 
  case 87 : 
  case 88 : 
  case 89 : 
  case 90 : 
  case 91 : 
  case 92 : 
  case 93 : 
  case 94 : 
  case 95 : 
  case 96 : 
  case 97 : 
  case 98 : 
  case 99 : 
  case 100 : 
  case 101 : 
  case 102 : 
  case 103 : 
  case 104 : 
  case 105 : 
  case 106 : 
  case 107 : 
  case 108 : 
  case 109 : 
  case 110 : 
  case 111 : 
  case 112 : 
  case 113 : 
  case 114 : 
  case 115 : 
  case 116 : 
  case 117 : 
  case 118 : 
  case 119 : 
  case 120 : 
  case 121 : 
  case 122 : 
  case 123 : 
  case 124 : 
  case 125 : 
  case 126 : 
  case 127 : 
    Result = o - 0 ;
    break ;
  case 128 : 
  case 133 : 
  case 235 : 
  case 239 : 
  case 243 : 
    Result = getbyte () ;
    break ;
  case 129 : 
  case 134 : 
  case 236 : 
  case 240 : 
  case 244 : 
    Result = gettwobytes () ;
    break ;
  case 130 : 
  case 135 : 
  case 237 : 
  case 241 : 
  case 245 : 
    Result = getthreebytes () ;
    break ;
  case 143 : 
  case 148 : 
  case 153 : 
  case 157 : 
  case 162 : 
  case 167 : 
    Result = signedbyte () ;
    break ;
  case 144 : 
  case 149 : 
  case 154 : 
  case 158 : 
  case 163 : 
  case 168 : 
    Result = signedpair () ;
    break ;
  case 145 : 
  case 150 : 
  case 155 : 
  case 159 : 
  case 164 : 
  case 169 : 
    Result = signedtrio () ;
    break ;
  case 131 : 
  case 132 : 
  case 136 : 
  case 137 : 
  case 146 : 
  case 151 : 
  case 156 : 
  case 160 : 
  case 165 : 
  case 170 : 
  case 238 : 
  case 242 : 
  case 246 : 
    Result = signedquad () ;
    break ;
  case 138 : 
  case 139 : 
  case 140 : 
  case 141 : 
  case 142 : 
  case 247 : 
  case 248 : 
  case 249 : 
  case 250 : 
  case 251 : 
  case 252 : 
  case 253 : 
  case 254 : 
  case 255 : 
    Result = 0 ;
    break ;
  case 147 : 
    Result = w ;
    break ;
  case 152 : 
    Result = x ;
    break ;
  case 161 : 
    Result = y ;
    break ;
  case 166 : 
    Result = z ;
    break ;
  case 171 : 
  case 172 : 
  case 173 : 
  case 174 : 
  case 175 : 
  case 176 : 
  case 177 : 
  case 178 : 
  case 179 : 
  case 180 : 
  case 181 : 
  case 182 : 
  case 183 : 
  case 184 : 
  case 185 : 
  case 186 : 
  case 187 : 
  case 188 : 
  case 189 : 
  case 190 : 
  case 191 : 
  case 192 : 
  case 193 : 
  case 194 : 
  case 195 : 
  case 196 : 
  case 197 : 
  case 198 : 
  case 199 : 
  case 200 : 
  case 201 : 
  case 202 : 
  case 203 : 
  case 204 : 
  case 205 : 
  case 206 : 
  case 207 : 
  case 208 : 
  case 209 : 
  case 210 : 
  case 211 : 
  case 212 : 
  case 213 : 
  case 214 : 
  case 215 : 
  case 216 : 
  case 217 : 
  case 218 : 
  case 219 : 
  case 220 : 
  case 221 : 
  case 222 : 
  case 223 : 
  case 224 : 
  case 225 : 
  case 226 : 
  case 227 : 
  case 228 : 
  case 229 : 
  case 230 : 
  case 231 : 
  case 232 : 
  case 233 : 
  case 234 : 
    Result = o - 171 ;
    break ;
  } 
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zinVF ( integer f ) 
#else
zinVF ( f ) 
  integer f ;
#endif
{
  /* 9997 9999 */ integer p  ;
  boolean wasvfreading  ;
  integer c  ;
  integer limit  ;
  integer w  ;
  wasvfreading = vfreading ;
  vfreading = true ;
  p = getbyte () ;
  if ( p != 247 ) 
  goto lab9997 ;
  p = getbyte () ;
  if ( p != 202 ) 
  goto lab9997 ;
  p = getbyte () ;
  while ( p > 0 ) {
      
    p = p - 1 ;
    downthedrain = getbyte () ;
  } 
  tfmchecksum = signedquad () ;
  downthedrain = signedquad () ;
  ftop [f ]= vfptr ;
  if ( vfptr == nf ) 
  {
    fprintf(stdout, "%s%s%ld%c\n", "DVItoMP abort: " ,     "DVItoMP capacity exceeded (max font numbers=" , (long)maxfnums , ')' ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  vfptr = vfptr - 1 ;
  infobase [f ]= infoptr ;
  limit = maxwidths - infobase [f ];
  fontbc [f ]= limit ;
  fontec [f ]= 0 ;
  p = getbyte () ;
  while ( p >= 243 ) {
      
    if ( p > 246 ) 
    goto lab9997 ;
    definefont ( firstpar ( p ) ) ;
    p = getbyte () ;
  } 
  while ( p <= 242 ) {
      
    if ( eof ( vffile ) ) 
    goto lab9997 ;
    if ( p == 242 ) 
    {
      p = signedquad () ;
      c = signedquad () ;
      w = signedquad () ;
      if ( c < 0 ) 
      goto lab9997 ;
    } 
    else {
	
      c = getbyte () ;
      w = getthreebytes () ;
    } 
    if ( c >= limit ) 
    {
      fprintf(stdout, "%s%s%ld%s\n", "DVItoMP abort: " , "DVItoMP capacity exceeded (max widths=" ,       (long)maxwidths , ")!" ) ;
      history = 3 ;
      uexit ( history ) ;
    } 
    if ( c < fontbc [f ]) 
    fontbc [f ]= c ;
    if ( c > fontec [f ]) 
    fontec [f ]= c ;
    width [infobase [f ]+ c ]= w ;
    if ( ncmds + p >= virtualspace ) 
    {
      fprintf(stdout, "%s%s%ld%s\n", "DVItoMP abort: " ,       "DVItoMP capacity exceeded (virtual font space=" , (long)virtualspace , ")!" ) 
      ;
      history = 3 ;
      uexit ( history ) ;
    } 
    cmdptr [infobase [f ]+ c ]= ncmds ;
    while ( p > 0 ) {
	
      cmdbuf [ncmds ]= getbyte () ;
      ncmds = ncmds + 1 ;
      p = p - 1 ;
    } 
    cmdbuf [ncmds ]= 140 ;
    ncmds = ncmds + 1 ;
    p = getbyte () ;
  } 
  if ( p == 248 ) 
  {
    fbase [f ]= vfptr + 1 ;
    infoptr = infobase [f ]+ fontec [f ];
    goto lab9999 ;
  } 
  lab9997: {
      
    fprintf(stdout, "%s%s", "DVItoMP abort: " , "Bad VF file for " ) ;
    errprintfont ( f ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  lab9999: vfreading = wasvfreading ;
} 
integer 
#ifdef HAVE_PROTOTYPES
zselectfont ( integer e ) 
#else
zselectfont ( e ) 
  integer e ;
#endif
{
  register integer Result; integer f  ;
  integer ff  ;
  integer k, l  ;
  if ( curftop <= nf ) 
  curftop = nf ;
  fontnum [curftop ]= e ;
  k = curfbase ;
  while ( ( fontnum [k ]!= e ) || localonly [k ]) k = k + 1 ;
  if ( k == curftop ) 
  {
    fprintf(stdout, "%s%s\n", "DVItoMP abort: " , "Undefined font selected" ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  f = internalnum [k ];
  if ( infobase [f ]== maxwidths ) 
  {
    ff = matchfont ( f , false ) ;
    if ( ff < nf ) 
    {
      fontbc [f ]= fontbc [ff ];
      fontec [f ]= fontec [ff ];
      infobase [f ]= infobase [ff ];
      fbase [f ]= fbase [ff ];
      ftop [f ]= ftop [ff ];
    } 
    else {
	
      curname = xmalloc ( ( fontname [f + 1 ]- fontname [f ]) + 1 ) ;
      {register integer for_end; k = fontname [f ];for_end = fontname [f 
      + 1 ]- 1 ; if ( k <= for_end) do 
	{
	  curname [k - fontname [f ]]= xchr [names [k ]];
	} 
      while ( k++ < for_end ) ;} 
      curname [fontname [f + 1 ]- fontname [f ]]= 0 ;
      if ( openvffile () ) 
      inVF ( f ) ;
      else {
	  
	;
	if ( ! opentfmfile () ) 
	{
	  fprintf(stdout, "%s%s", "DVItoMP abort: " , "No TFM file found for " ) ;
	  errprintfont ( f ) ;
	  history = 3 ;
	  uexit ( history ) ;
	} 
	inTFM ( f ) ;
      } 
      {
	if ( ( fontchecksum [f ]!= 0 ) && ( tfmchecksum != 0 ) && ( 
	fontchecksum [f ]!= tfmchecksum ) ) 
	{
	  Fputs(stdout, "DVItoMP warning: Checksum mismatch for " ) ;
	  errprintfont ( f ) ;
	  if ( history == 0 ) 
	  history = 1 ;
	} 
      } 
    } 
    fontused [f ]= false ;
  } 
  Result = f ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
finishlastchar ( void ) 
#else
finishlastchar ( ) 
#endif
{
  real m, x, y  ;
  if ( strf >= 0 ) 
  {
    m = strscale * fontscaledsize [strf ]* mag / ((double) fontdesignsize [
    strf ]) ;
    x = conv * strh1 ;
    y = conv * ( - (integer) strv ) ;
    if ( ( fabs ( x ) >= 4096.0 ) || ( fabs ( y ) >= 4096.0 ) || ( m >= 4096.0 
    ) || ( m < 0 ) ) 
    {
      {
	fprintf(stdout, "%s%s\n", "DVItoMP warning: " , "text is out of range" ) ;
	history = 2 ;
      } 
      endcharstring ( 60 ) ;
    } 
    else endcharstring ( 40 ) ;
    fprintf( mpxfile , "%s%ld%c",  ",_n" , (long)strf , ',' ) ;
    fprintreal ( mpxfile , m , 1 , 5 ) ;
    putc ( ',' ,  mpxfile );
    fprintreal ( mpxfile , x , 1 , 4 ) ;
    putc ( ',' ,  mpxfile );
    fprintreal ( mpxfile , y , 1 , 4 ) ;
    fprintf( mpxfile , "%s\n",  ");" ) ;
    strf = -1 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zdosetchar ( integer f , integer c ) 
#else
zdosetchar ( f , c ) 
  integer f ;
  integer c ;
#endif
{
  if ( ( c < fontbc [f ]) || ( c > fontec [f ]) ) 
  {
    fprintf(stdout, "%s%s%ld\n", "DVItoMP abort: " , "attempt to typeset invalid character " , (long)c     ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  if ( ( h != strh2 ) || ( v != strv ) || ( f != strf ) || ( dviscale != 
  strscale ) ) 
  {
    if ( strf >= 0 ) 
    finishlastchar () ;
    else if ( ! fontsused ) 
    {
      k = 0 ;
      while ( ( k < nf ) ) {
	  
	fontused [k ]= false ;
	k = k + 1 ;
      } 
      fontsused = true ;
      fprintf( mpxfile , "%s\n",  "string _n[];" ) ;
      fprintf( mpxfile , "%s\n",  "vardef _s(expr _t,_f,_m,_x,_y)=" ) ;
      fprintf( mpxfile , "%s\n",        "  addto _p also _t infont _f scaled _m shifted (_x,_y); enddef;" ) ;
    } 
    if ( ! fontused [f ]) 
    {
      fontused [f ]= true ;
      fprintf( mpxfile , "%s%ld%c",  "_n" , (long)f , '=' ) ;
      printcol = 6 ;
      printfont ( f ) ;
      endcharstring ( 1 ) ;
      fprintf( mpxfile , "%c\n",  ';' ) ;
    } 
    Fputs( mpxfile ,  "_s(" ) ;
    printcol = 3 ;
    strscale = dviscale ;
    strf = f ;
    strv = v ;
    strh1 = h ;
  } 
  printchar ( c ) ;
  strh2 = h + round ( dviscale * fontscaledsize [f ]* width [infobase [f ]
  + c ]- 0.5 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdosetrule ( integer ht , integer wd ) 
#else
zdosetrule ( ht , wd ) 
  integer ht ;
  integer wd ;
#endif
{
  real xx1, yy1, xx2, yy2, ww  ;
  if ( wd == 1 ) 
  {
    picwd = h ;
    picdp = v ;
    picht = ht - v ;
  } 
  else if ( ( ht > 0 ) || ( wd > 0 ) ) 
  {
    if ( strf >= 0 ) 
    finishlastchar () ;
    if ( ! rulesused ) 
    {
      rulesused = true ;
      fprintf( mpxfile , "%s\n",  "interim linecap:=0;" ) ;
      fprintf( mpxfile , "%s\n",  "vardef _r(expr _a,_w) =" ) ;
      fprintf( mpxfile , "%s\n",        "  addto _p doublepath _a withpen pencircle scaled _w enddef;" ) ;
    } 
    xx1 = conv * h ;
    yy1 = conv * ( - (integer) v ) ;
    if ( wd > ht ) 
    {
      xx2 = xx1 + conv * wd ;
      ww = conv * ht ;
      yy1 = yy1 + 0.5 * ww ;
      yy2 = yy1 ;
    } 
    else {
	
      yy2 = yy1 + conv * ht ;
      ww = conv * wd ;
      xx1 = xx1 + 0.5 * ww ;
      xx2 = xx1 ;
    } 
    if ( ( fabs ( xx1 ) >= 4096.0 ) || ( fabs ( yy1 ) >= 4096.0 ) || ( fabs ( 
    xx2 ) >= 4096.0 ) || ( fabs ( yy2 ) >= 4096.0 ) || ( ww >= 4096.0 ) ) 
    {
      fprintf(stdout, "%s%s\n", "DVItoMP warning: " , "hrule or vrule is out of range" ) ;
      history = 2 ;
    } 
    Fputs( mpxfile ,  "_r((" ) ;
    fprintreal ( mpxfile , xx1 , 1 , 4 ) ;
    putc ( ',' ,  mpxfile );
    fprintreal ( mpxfile , yy1 , 1 , 4 ) ;
    Fputs( mpxfile ,  ")..(" ) ;
    fprintreal ( mpxfile , xx2 , 1 , 4 ) ;
    putc ( ',' ,  mpxfile );
    fprintreal ( mpxfile , yy2 , 1 , 4 ) ;
    Fputs( mpxfile ,  "), " ) ;
    fprintreal ( mpxfile , ww , 1 , 4 ) ;
    fprintf( mpxfile , "%s\n",  ");" ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
startpicture ( void ) 
#else
startpicture ( ) 
#endif
{
  fontsused = false ;
  rulesused = false ;
  strf = -1 ;
  strv = 0 ;
  strh2 = 0 ;
  strscale = 1.0 ;
  fprintf( mpxfile , "%s\n",    "begingroup save _p,_r,_s,_n; picture _p; _p=nullpicture;" ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
stoppicture ( void ) 
#else
stoppicture ( ) 
#endif
{
  real w, h, dd  ;
  if ( strf >= 0 ) 
  finishlastchar () ;
  dd = - (integer) picdp * conv ;
  w = conv * picwd ;
  h = conv * picht ;
  Fputs( mpxfile ,  "setbounds _p to (0," ) ;
  fprintreal ( mpxfile , dd , 1 , 4 ) ;
  Fputs( mpxfile ,  ")--(" ) ;
  fprintreal ( mpxfile , w , 1 , 4 ) ;
  putc ( ',' ,  mpxfile );
  fprintreal ( mpxfile , dd , 1 , 4 ) ;
  fprintf( mpxfile , "%s\n",  ")--" ) ;
  Fputs( mpxfile ,  " (" ) ;
  fprintreal ( mpxfile , w , 1 , 4 ) ;
  putc ( ',' ,  mpxfile );
  fprintreal ( mpxfile , h , 1 , 4 ) ;
  Fputs( mpxfile ,  ")--(0," ) ;
  fprintreal ( mpxfile , h , 1 , 4 ) ;
  fprintf( mpxfile , "%s\n",  ")--cycle;" ) ;
  fprintf( mpxfile , "%s\n",  "_p endgroup" ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
dopush ( void ) 
#else
dopush ( ) 
#endif
{
  if ( stksiz == stacksize ) 
  {
    fprintf(stdout, "%s%s%ld%c\n", "DVItoMP abort: " , "DVItoMP capacity exceeded (stack size=" ,     (long)stacksize , ')' ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  hstack [stksiz ]= h ;
  vstack [stksiz ]= v ;
  wstack [stksiz ]= w ;
  xstack [stksiz ]= x ;
  ystack [stksiz ]= y ;
  zstack [stksiz ]= z ;
  stksiz = stksiz + 1 ;
} 
void 
#ifdef HAVE_PROTOTYPES
dopop ( void ) 
#else
dopop ( ) 
#endif
{
  if ( stksiz == 0 ) 
  {
    fprintf(stdout, "%s%s%s%c\n", "DVItoMP abort: " , "Bad DVI file: " ,     "attempt to pop empty stack" , '!' ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  else {
      
    stksiz = stksiz - 1 ;
    h = hstack [stksiz ];
    v = vstack [stksiz ];
    w = wstack [stksiz ];
    x = xstack [stksiz ];
    y = ystack [stksiz ];
    z = zstack [stksiz ];
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zsetvirtualchar ( integer f , integer c ) 
#else
zsetvirtualchar ( f , c ) 
  integer f ;
  integer c ;
#endif
{
  real oldscale  ;
  integer oldbufptr  ;
  integer oldfbase, oldftop  ;
  if ( fbase [f ]== 0 ) 
  dosetchar ( f , c ) ;
  else {
      
    oldfbase = curfbase ;
    oldftop = curftop ;
    curfbase = fbase [f ];
    curftop = ftop [f ];
    oldscale = dviscale ;
    dviscale = dviscale * fontscaledsize [f ];
    oldbufptr = bufptr ;
    bufptr = cmdptr [infobase [f ]+ c ];
    dopush () ;
    dodvicommands () ;
    dopop () ;
    bufptr = oldbufptr ;
    dviscale = oldscale ;
    curfbase = oldfbase ;
    curftop = oldftop ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
dodvicommands ( void ) 
#else
dodvicommands ( ) 
#endif
{
  /* 9999 */ eightbits o  ;
  integer p, q  ;
  integer curfont  ;
  if ( ( curfbase < curftop ) && ( bufptr < virtualspace ) ) 
  curfont = selectfont ( fontnum [curftop - 1 ]) ;
  else curfont = maxfnums + 1 ;
  w = 0 ;
  x = 0 ;
  y = 0 ;
  z = 0 ;
  while ( true ) {
      
    o = getbyte () ;
    p = firstpar ( o ) ;
    if ( eof ( dvifile ) ) 
    {
      fprintf(stdout, "%s%s%s%c\n", "DVItoMP abort: " , "Bad DVI file: " ,       "the DVI file ended prematurely" , '!' ) ;
      history = 3 ;
      uexit ( history ) ;
    } 
    if ( o < 132 ) 
    {
      if ( curfont > maxfnums ) 
      if ( vfreading ) 
      {
	fprintf(stdout, "%s%s%ld%s\n", "DVItoMP abort: " , "no font selected for character " , (long)p ,         " in virtual font" ) ;
	history = 3 ;
	uexit ( history ) ;
      } 
      else {
	  
	fprintf(stdout, "%s%s%s%ld%c\n", "DVItoMP abort: " , "Bad DVI file: " ,         "no font selected for character " , (long)p , '!' ) ;
	history = 3 ;
	uexit ( history ) ;
      } 
      setvirtualchar ( curfont , p ) ;
      h = h + round ( dviscale * fontscaledsize [curfont ]* width [infobase 
      [curfont ]+ p ]- 0.5 ) ;
    } 
    else switch ( o ) 
    {case 133 : 
    case 134 : 
    case 135 : 
    case 136 : 
      setvirtualchar ( curfont , p ) ;
      break ;
    case 132 : 
      {
	q = trunc ( signedquad () * dviscale ) ;
	dosetrule ( trunc ( p * dviscale ) , q ) ;
	h = h + q ;
      } 
      break ;
    case 137 : 
      dosetrule ( trunc ( p * dviscale ) , trunc ( signedquad () * dviscale ) 
      ) ;
      break ;
    case 239 : 
    case 240 : 
    case 241 : 
    case 242 : 
      {register integer for_end; k = 1 ;for_end = p ; if ( k <= for_end) do 
	downthedrain = getbyte () ;
      while ( k++ < for_end ) ;} 
      break ;
    case 247 : 
    case 248 : 
    case 249 : 
      {
	fprintf(stdout, "%s%s%s%c\n", "DVItoMP abort: " , "Bad DVI file: " ,         "preamble or postamble within a page!" , '!' ) ;
	history = 3 ;
	uexit ( history ) ;
      } 
      break ;
    case 138 : 
      ;
      break ;
    case 139 : 
      {
	fprintf(stdout, "%s%s%s%c\n", "DVItoMP abort: " , "Bad DVI file: " ,         "bop occurred before eop" , '!' ) ;
	history = 3 ;
	uexit ( history ) ;
      } 
      break ;
    case 140 : 
      goto lab9999 ;
      break ;
    case 141 : 
      dopush () ;
      break ;
    case 142 : 
      dopop () ;
      break ;
    case 143 : 
    case 144 : 
    case 145 : 
    case 146 : 
      h = h + trunc ( p * dviscale ) ;
      break ;
    case 147 : 
    case 148 : 
    case 149 : 
    case 150 : 
    case 151 : 
      {
	w = trunc ( p * dviscale ) ;
	h = h + w ;
      } 
      break ;
    case 152 : 
    case 153 : 
    case 154 : 
    case 155 : 
    case 156 : 
      {
	x = trunc ( p * dviscale ) ;
	h = h + x ;
      } 
      break ;
    case 157 : 
    case 158 : 
    case 159 : 
    case 160 : 
      v = v + trunc ( p * dviscale ) ;
      break ;
    case 161 : 
    case 162 : 
    case 163 : 
    case 164 : 
    case 165 : 
      {
	y = trunc ( p * dviscale ) ;
	v = v + y ;
      } 
      break ;
    case 166 : 
    case 167 : 
    case 168 : 
    case 169 : 
    case 170 : 
      {
	z = trunc ( p * dviscale ) ;
	v = v + z ;
      } 
      break ;
    case 171 : 
    case 172 : 
    case 173 : 
    case 174 : 
    case 175 : 
    case 176 : 
    case 177 : 
    case 178 : 
    case 179 : 
    case 180 : 
    case 181 : 
    case 182 : 
    case 183 : 
    case 184 : 
    case 185 : 
    case 186 : 
    case 187 : 
    case 188 : 
    case 189 : 
    case 190 : 
    case 191 : 
    case 192 : 
    case 193 : 
    case 194 : 
    case 195 : 
    case 196 : 
    case 197 : 
    case 198 : 
    case 199 : 
    case 200 : 
    case 201 : 
    case 202 : 
    case 203 : 
    case 204 : 
    case 205 : 
    case 206 : 
    case 207 : 
    case 208 : 
    case 209 : 
    case 210 : 
    case 211 : 
    case 212 : 
    case 213 : 
    case 214 : 
    case 215 : 
    case 216 : 
    case 217 : 
    case 218 : 
    case 219 : 
    case 220 : 
    case 221 : 
    case 222 : 
    case 223 : 
    case 224 : 
    case 225 : 
    case 226 : 
    case 227 : 
    case 228 : 
    case 229 : 
    case 230 : 
    case 231 : 
    case 232 : 
    case 233 : 
    case 234 : 
    case 235 : 
    case 236 : 
    case 237 : 
    case 238 : 
      curfont = selectfont ( p ) ;
      break ;
    case 243 : 
    case 244 : 
    case 245 : 
    case 246 : 
      definefont ( p ) ;
      break ;
    case 250 : 
    case 251 : 
    case 252 : 
    case 253 : 
    case 254 : 
    case 255 : 
      {
	fprintf(stdout, "%s%s%s%ld%c\n", "DVItoMP abort: " , "Bad DVI file: " , "undefined command "         , (long)o , '!' ) ;
	history = 3 ;
	uexit ( history ) ;
      } 
      break ;
    } 
  } 
  lab9999: ;
} 
void mainbody() {
    
  initialize () ;
  opendvifile () ;
  p = getbyte () ;
  if ( p != 247 ) 
  {
    fprintf(stdout, "%s%s%s%c\n", "DVItoMP abort: " , "Bad DVI file: " ,     "First byte isn't start of preamble!" , '!' ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  p = getbyte () ;
  if ( p != 2 ) 
  {
    fprintf(stdout, "%s%s%ld%c\n", "DVItoMP warning: " , "identification in byte 1 should be " , (long)2     , '!' ) ;
    history = 2 ;
  } 
  numerator = signedquad () ;
  denominator = signedquad () ;
  if ( ( numerator <= 0 ) || ( denominator <= 0 ) ) 
  {
    fprintf(stdout, "%s%s%s%c\n", "DVItoMP abort: " , "Bad DVI file: " ,     "bad scale ratio in preamble" , '!' ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  mag = signedquad () / ((double) 1000.0 ) ;
  if ( mag <= 0.0 ) 
  {
    fprintf(stdout, "%s%s%s%c\n", "DVItoMP abort: " , "Bad DVI file: " ,     "magnification isn't positive" , '!' ) ;
    history = 3 ;
    uexit ( history ) ;
  } 
  conv = ( numerator / ((double) 254000.0 ) ) * ( 72.0 / ((double) denominator 
  ) ) * mag ;
  dviperfix = ( 254000.0 / ((double) numerator ) ) * ( denominator / ((double) 
  72.27 ) ) / ((double) 1048576.0 ) ;
  p = getbyte () ;
  while ( p > 0 ) {
      
    p = p - 1 ;
    downthedrain = getbyte () ;
  } 
  openmpxfile () ;
  Fputs( mpxfile ,  "% Written by DVItoMP, Version 0.64" ) ;
  fprintf( mpxfile , "%s\n",  versionstring ) ;
  {
    while ( true ) {
	
      do {
	  k = getbyte () ;
	if ( ( k >= 243 ) && ( k < 247 ) ) 
	{
	  p = firstpar ( k ) ;
	  definefont ( p ) ;
	  k = 138 ;
	} 
      } while ( ! ( k != 138 ) ) ;
      if ( k == 248 ) 
      goto lab30 ;
      if ( k != 139 ) 
      {
	fprintf(stdout, "%s%s%s%c\n", "DVItoMP abort: " , "Bad DVI file: " , "missing bop" , '!' ) 
	;
	history = 3 ;
	uexit ( history ) ;
      } 
      {register integer for_end; k = 0 ;for_end = 10 ; if ( k <= for_end) do 
	downthedrain = signedquad () ;
      while ( k++ < for_end ) ;} 
      dviscale = 1.0 ;
      stksiz = 0 ;
      h = 0 ;
      v = 0 ;
      startpicture () ;
      dodvicommands () ;
      if ( stksiz != 0 ) 
      {
	fprintf(stdout, "%s%s%s%c\n", "DVItoMP abort: " , "Bad DVI file: " ,         "stack not empty at end of page" , '!' ) ;
	history = 3 ;
	uexit ( history ) ;
      } 
      stoppicture () ;
      fprintf( mpxfile , "%s\n",  "mpxbreak" ) ;
    } 
    lab30: ;
  } 
  if ( history <= 1 ) 
  uexit ( 0 ) ;
  else uexit ( history ) ;
} 
