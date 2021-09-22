#define EXTERN extern
#include "mfd.h"

void 
#ifdef HAVE_PROTOTYPES
initialize ( void ) 
#else
initialize ( ) 
#endif
{
  integer i  ;
  integer k  ;
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
  {register integer for_end; i = 0 ;for_end = 31 ; if ( i <= for_end) do 
    xchr [i ]= chr ( i ) ;
  while ( i++ < for_end ) ;} 
  {register integer for_end; i = 127 ;for_end = 255 ; if ( i <= for_end) do 
    xchr [i ]= chr ( i ) ;
  while ( i++ < for_end ) ;} 
  {register integer for_end; i = 0 ;for_end = 255 ; if ( i <= for_end) do 
    xord [chr ( i ) ]= 127 ;
  while ( i++ < for_end ) ;} 
  {register integer for_end; i = 128 ;for_end = 255 ; if ( i <= for_end) do 
    xord [xchr [i ]]= i ;
  while ( i++ < for_end ) ;} 
  {register integer for_end; i = 0 ;for_end = 126 ; if ( i <= for_end) do 
    xord [xchr [i ]]= i ;
  while ( i++ < for_end ) ;} 
  if ( interactionoption == 4 ) 
  interaction = 3 ;
  else interaction = interactionoption ;
  deletionsallowed = true ;
  errorcount = 0 ;
  helpptr = 0 ;
  useerrhelp = false ;
  errhelp = 0 ;
  interrupt = 0 ;
  OKtointerrupt = true ;
  aritherror = false ;
  twotothe [0 ]= 1 ;
  {register integer for_end; k = 1 ;for_end = 30 ; if ( k <= for_end) do 
    twotothe [k ]= 2 * twotothe [k - 1 ];
  while ( k++ < for_end ) ;} 
  speclog [1 ]= 93032640L ;
  speclog [2 ]= 38612034L ;
  speclog [3 ]= 17922280L ;
  speclog [4 ]= 8662214L ;
  speclog [5 ]= 4261238L ;
  speclog [6 ]= 2113709L ;
  speclog [7 ]= 1052693L ;
  speclog [8 ]= 525315L ;
  speclog [9 ]= 262400L ;
  speclog [10 ]= 131136L ;
  speclog [11 ]= 65552L ;
  speclog [12 ]= 32772L ;
  speclog [13 ]= 16385 ;
  {register integer for_end; k = 14 ;for_end = 27 ; if ( k <= for_end) do 
    speclog [k ]= twotothe [27 - k ];
  while ( k++ < for_end ) ;} 
  speclog [28 ]= 1 ;
  specatan [1 ]= 27855475L ;
  specatan [2 ]= 14718068L ;
  specatan [3 ]= 7471121L ;
  specatan [4 ]= 3750058L ;
  specatan [5 ]= 1876857L ;
  specatan [6 ]= 938658L ;
  specatan [7 ]= 469357L ;
  specatan [8 ]= 234682L ;
  specatan [9 ]= 117342L ;
  specatan [10 ]= 58671L ;
  specatan [11 ]= 29335 ;
  specatan [12 ]= 14668 ;
  specatan [13 ]= 7334 ;
  specatan [14 ]= 3667 ;
  specatan [15 ]= 1833 ;
  specatan [16 ]= 917 ;
  specatan [17 ]= 458 ;
  specatan [18 ]= 229 ;
  specatan [19 ]= 115 ;
  specatan [20 ]= 57 ;
  specatan [21 ]= 29 ;
  specatan [22 ]= 14 ;
  specatan [23 ]= 7 ;
  specatan [24 ]= 4 ;
  specatan [25 ]= 2 ;
  specatan [26 ]= 1 ;
	;
#ifdef TEXMF_DEBUG
  wasmemend = 0 ;
  waslomax = 0 ;
  washimin = memmax ;
  panicking = false ;
#endif /* TEXMF_DEBUG */
  {register integer for_end; k = 1 ;for_end = 41 ; if ( k <= for_end) do 
    internal [k ]= 0 ;
  while ( k++ < for_end ) ;} 
  intptr = 41 ;
  {register integer for_end; k = 48 ;for_end = 57 ; if ( k <= for_end) do 
    charclass [k ]= 0 ;
  while ( k++ < for_end ) ;} 
  charclass [46 ]= 1 ;
  charclass [32 ]= 2 ;
  charclass [37 ]= 3 ;
  charclass [34 ]= 4 ;
  charclass [44 ]= 5 ;
  charclass [59 ]= 6 ;
  charclass [40 ]= 7 ;
  charclass [41 ]= 8 ;
  {register integer for_end; k = 65 ;for_end = 90 ; if ( k <= for_end) do 
    charclass [k ]= 9 ;
  while ( k++ < for_end ) ;} 
  {register integer for_end; k = 97 ;for_end = 122 ; if ( k <= for_end) do 
    charclass [k ]= 9 ;
  while ( k++ < for_end ) ;} 
  charclass [95 ]= 9 ;
  charclass [60 ]= 10 ;
  charclass [61 ]= 10 ;
  charclass [62 ]= 10 ;
  charclass [58 ]= 10 ;
  charclass [124 ]= 10 ;
  charclass [96 ]= 11 ;
  charclass [39 ]= 11 ;
  charclass [43 ]= 12 ;
  charclass [45 ]= 12 ;
  charclass [47 ]= 13 ;
  charclass [42 ]= 13 ;
  charclass [92 ]= 13 ;
  charclass [33 ]= 14 ;
  charclass [63 ]= 14 ;
  charclass [35 ]= 15 ;
  charclass [38 ]= 15 ;
  charclass [64 ]= 15 ;
  charclass [36 ]= 15 ;
  charclass [94 ]= 16 ;
  charclass [126 ]= 16 ;
  charclass [91 ]= 17 ;
  charclass [93 ]= 18 ;
  charclass [123 ]= 19 ;
  charclass [125 ]= 19 ;
  {register integer for_end; k = 0 ;for_end = 31 ; if ( k <= for_end) do 
    charclass [k ]= 20 ;
  while ( k++ < for_end ) ;} 
  {register integer for_end; k = 127 ;for_end = 255 ; if ( k <= for_end) do 
    charclass [k ]= 20 ;
  while ( k++ < for_end ) ;} 
  charclass [9 ]= 2 ;
  charclass [12 ]= 2 ;
  hash [1 ].lhfield = 0 ;
  hash [1 ].v.RH = 0 ;
  eqtb [1 ].lhfield = 41 ;
  eqtb [1 ].v.RH = 0 ;
  {register integer for_end; k = 2 ;for_end = 9769 ; if ( k <= for_end) do 
    {
      hash [k ]= hash [1 ];
      eqtb [k ]= eqtb [1 ];
    } 
  while ( k++ < for_end ) ;} 
  bignodesize [13 ]= 12 ;
  bignodesize [14 ]= 4 ;
  saveptr = 0 ;
  octantdir [1 ]= 547 ;
  octantdir [5 ]= 548 ;
  octantdir [6 ]= 549 ;
  octantdir [2 ]= 550 ;
  octantdir [4 ]= 551 ;
  octantdir [8 ]= 552 ;
  octantdir [7 ]= 553 ;
  octantdir [3 ]= 554 ;
  maxroundingptr = 0 ;
  octantcode [1 ]= 1 ;
  octantcode [2 ]= 5 ;
  octantcode [3 ]= 6 ;
  octantcode [4 ]= 2 ;
  octantcode [5 ]= 4 ;
  octantcode [6 ]= 8 ;
  octantcode [7 ]= 7 ;
  octantcode [8 ]= 3 ;
  {register integer for_end; k = 1 ;for_end = 8 ; if ( k <= for_end) do 
    octantnumber [octantcode [k ]]= k ;
  while ( k++ < for_end ) ;} 
  revturns = false ;
  xcorr [1 ]= 0 ;
  ycorr [1 ]= 0 ;
  xycorr [1 ]= 0 ;
  xcorr [5 ]= 0 ;
  ycorr [5 ]= 0 ;
  xycorr [5 ]= 1 ;
  xcorr [6 ]= -1 ;
  ycorr [6 ]= 1 ;
  xycorr [6 ]= 0 ;
  xcorr [2 ]= 1 ;
  ycorr [2 ]= 0 ;
  xycorr [2 ]= 1 ;
  xcorr [4 ]= 0 ;
  ycorr [4 ]= 1 ;
  xycorr [4 ]= 1 ;
  xcorr [8 ]= 0 ;
  ycorr [8 ]= 1 ;
  xycorr [8 ]= 0 ;
  xcorr [7 ]= 1 ;
  ycorr [7 ]= 0 ;
  xycorr [7 ]= 1 ;
  xcorr [3 ]= -1 ;
  ycorr [3 ]= 1 ;
  xycorr [3 ]= 0 ;
  {register integer for_end; k = 1 ;for_end = 8 ; if ( k <= for_end) do 
    zcorr [k ]= xycorr [k ]- xcorr [k ];
  while ( k++ < for_end ) ;} 
  screenstarted = false ;
  screenOK = false ;
  {register integer for_end; k = 0 ;for_end = 15 ; if ( k <= for_end) do 
    {
      windowopen [k ]= false ;
      windowtime [k ]= 0 ;
    } 
  while ( k++ < for_end ) ;} 
  fixneeded = false ;
  watchcoefs = true ;
  condptr = 0 ;
  iflimit = 0 ;
  curif = 0 ;
  ifline = 0 ;
  loopptr = 0 ;
  curexp = 0 ;
  varflag = 0 ;
  startsym = 0 ;
  longhelpseen = false ;
  {register integer for_end; k = 0 ;for_end = 255 ; if ( k <= for_end) do 
    {
      tfmwidth [k ]= 0 ;
      tfmheight [k ]= 0 ;
      tfmdepth [k ]= 0 ;
      tfmitalcorr [k ]= 0 ;
      charexists [k ]= false ;
      chartag [k ]= 0 ;
      charremainder [k ]= 0 ;
      skiptable [k ]= ligtablesize ;
    } 
  while ( k++ < for_end ) ;} 
  {register integer for_end; k = 1 ;for_end = headersize ; if ( k <= 
  for_end) do 
    headerbyte [k ]= -1 ;
  while ( k++ < for_end ) ;} 
  bc = 255 ;
  ec = 0 ;
  nl = 0 ;
  nk = 0 ;
  ne = 0 ;
  np = 0 ;
  internal [41 ]= -65536L ;
  bchlabel = ligtablesize ;
  labelloc [0 ]= -1 ;
  labelptr = 0 ;
  gfprevptr = 0 ;
  totalchars = 0 ;
  halfbuf = gfbufsize / 2 ;
  gflimit = gfbufsize ;
  gfptr = 0 ;
  gfoffset = 0 ;
  baseident = 0 ;
  editnamestart = 0 ;
} 
void 
#ifdef HAVE_PROTOTYPES
println ( void ) 
#else
println ( ) 
#endif
{
  switch ( selector ) 
  {case 3 : 
    {
      putc ('\n',  stdout );
      putc ('\n',  logfile );
      termoffset = 0 ;
      fileoffset = 0 ;
    } 
    break ;
  case 2 : 
    {
      putc ('\n',  logfile );
      fileoffset = 0 ;
    } 
    break ;
  case 1 : 
    {
      putc ('\n',  stdout );
      termoffset = 0 ;
    } 
    break ;
  case 0 : 
  case 4 : 
  case 5 : 
    ;
    break ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zprintvisiblechar ( ASCIIcode s ) 
#else
zprintvisiblechar ( s ) 
  ASCIIcode s ;
#endif
{
  switch ( selector ) 
  {case 3 : 
    {
      putc ( xchr [s ],  stdout );
      putc ( xchr [s ],  logfile );
      incr ( termoffset ) ;
      incr ( fileoffset ) ;
      if ( termoffset == maxprintline ) 
      {
	putc ('\n',  stdout );
	termoffset = 0 ;
      } 
      if ( fileoffset == maxprintline ) 
      {
	putc ('\n',  logfile );
	fileoffset = 0 ;
      } 
    } 
    break ;
  case 2 : 
    {
      putc ( xchr [s ],  logfile );
      incr ( fileoffset ) ;
      if ( fileoffset == maxprintline ) 
      println () ;
    } 
    break ;
  case 1 : 
    {
      putc ( xchr [s ],  stdout );
      incr ( termoffset ) ;
      if ( termoffset == maxprintline ) 
      println () ;
    } 
    break ;
  case 0 : 
    ;
    break ;
  case 4 : 
    if ( tally < trickcount ) 
    trickbuf [tally % errorline ]= s ;
    break ;
  case 5 : 
    {
      if ( poolptr < poolsize ) 
      {
	strpool [poolptr ]= s ;
	incr ( poolptr ) ;
      } 
    } 
    break ;
  } 
  incr ( tally ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintchar ( ASCIIcode s ) 
#else
zprintchar ( s ) 
  ASCIIcode s ;
#endif
{
  /* 10 */ ASCIIcode k  ;
  unsigned char l  ;
  k = s ;
  if ( ( k < 32 ) || ( k > 126 ) ) 
  {
    if ( selector > 4 ) 
    {
      printvisiblechar ( s ) ;
      goto lab10 ;
    } 
    printvisiblechar ( 94 ) ;
    printvisiblechar ( 94 ) ;
    if ( s < 64 ) 
    printvisiblechar ( s + 64 ) ;
    else if ( s < 128 ) 
    printvisiblechar ( s - 64 ) ;
    else {
	
      l = s / 16 ;
      if ( l < 10 ) 
      printvisiblechar ( l + 48 ) ;
      else printvisiblechar ( l + 87 ) ;
      l = s % 16 ;
      if ( l < 10 ) 
      printvisiblechar ( l + 48 ) ;
      else printvisiblechar ( l + 87 ) ;
    } 
  } 
  else printvisiblechar ( s ) ;
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprint ( integer s ) 
#else
zprint ( s ) 
  integer s ;
#endif
{
  poolpointer j  ;
  if ( ( s < 0 ) || ( s >= strptr ) ) 
  s = 259 ;
  if ( ( s < 256 ) ) 
  printchar ( s ) ;
  else {
      
    j = strstart [s ];
    while ( j < strstart [s + 1 ]) {
	
      printchar ( strpool [j ]) ;
      incr ( j ) ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zprintnl ( strnumber s ) 
#else
zprintnl ( s ) 
  strnumber s ;
#endif
{
  if ( ( ( termoffset > 0 ) && ( odd ( selector ) ) ) || ( ( fileoffset > 0 
  ) && ( selector >= 2 ) ) ) 
  println () ;
  print ( s ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintthedigs ( eightbits k ) 
#else
zprintthedigs ( k ) 
  eightbits k ;
#endif
{
  while ( k > 0 ) {
      
    decr ( k ) ;
    printchar ( 48 + dig [k ]) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zprintint ( integer n ) 
#else
zprintint ( n ) 
  integer n ;
#endif
{
  char k  ;
  integer m  ;
  k = 0 ;
  if ( n < 0 ) 
  {
    printchar ( 45 ) ;
    if ( n > -100000000L ) 
    n = - (integer) n ;
    else {
	
      m = -1 - n ;
      n = m / 10 ;
      m = ( m % 10 ) + 1 ;
      k = 1 ;
      if ( m < 10 ) 
      dig [0 ]= m ;
      else {
	  
	dig [0 ]= 0 ;
	incr ( n ) ;
      } 
    } 
  } 
  do {
      dig [k ]= n % 10 ;
    n = n / 10 ;
    incr ( k ) ;
  } while ( ! ( n == 0 ) ) ;
  printthedigs ( k ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintscaled ( scaled s ) 
#else
zprintscaled ( s ) 
  scaled s ;
#endif
{
  scaled delta  ;
  if ( s < 0 ) 
  {
    printchar ( 45 ) ;
    s = - (integer) s ;
  } 
  printint ( s / 65536L ) ;
  s = 10 * ( s % 65536L ) + 5 ;
  if ( s != 5 ) 
  {
    delta = 10 ;
    printchar ( 46 ) ;
    do {
	if ( delta > 65536L ) 
      s = s + 32768L - ( delta / 2 ) ;
      printchar ( 48 + ( s / 65536L ) ) ;
      s = 10 * ( s % 65536L ) ;
      delta = delta * 10 ;
    } while ( ! ( s <= delta ) ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zprinttwo ( scaled x , scaled y ) 
#else
zprinttwo ( x , y ) 
  scaled x ;
  scaled y ;
#endif
{
  printchar ( 40 ) ;
  printscaled ( x ) ;
  printchar ( 44 ) ;
  printscaled ( y ) ;
  printchar ( 41 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprinttype ( smallnumber t ) 
#else
zprinttype ( t ) 
  smallnumber t ;
#endif
{
  switch ( t ) 
  {case 1 : 
    print ( 322 ) ;
    break ;
  case 2 : 
    print ( 323 ) ;
    break ;
  case 3 : 
    print ( 324 ) ;
    break ;
  case 4 : 
    print ( 325 ) ;
    break ;
  case 5 : 
    print ( 326 ) ;
    break ;
  case 6 : 
    print ( 327 ) ;
    break ;
  case 7 : 
    print ( 328 ) ;
    break ;
  case 8 : 
    print ( 329 ) ;
    break ;
  case 9 : 
    print ( 330 ) ;
    break ;
  case 10 : 
    print ( 331 ) ;
    break ;
  case 11 : 
    print ( 332 ) ;
    break ;
  case 12 : 
    print ( 333 ) ;
    break ;
  case 13 : 
    print ( 334 ) ;
    break ;
  case 14 : 
    print ( 335 ) ;
    break ;
  case 16 : 
    print ( 336 ) ;
    break ;
  case 17 : 
    print ( 337 ) ;
    break ;
  case 18 : 
    print ( 338 ) ;
    break ;
  case 15 : 
    print ( 339 ) ;
    break ;
  case 19 : 
    print ( 340 ) ;
    break ;
  case 20 : 
    print ( 341 ) ;
    break ;
  case 21 : 
    print ( 342 ) ;
    break ;
  case 22 : 
    print ( 343 ) ;
    break ;
  case 23 : 
    print ( 344 ) ;
    break ;
    default: 
    print ( 345 ) ;
    break ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
begindiagnostic ( void ) 
#else
begindiagnostic ( ) 
#endif
{
  oldsetting = selector ;
  if ( ( internal [13 ]<= 0 ) && ( selector == 3 ) ) 
  {
    decr ( selector ) ;
    if ( history == 0 ) 
    history = 1 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zenddiagnostic ( boolean blankline ) 
#else
zenddiagnostic ( blankline ) 
  boolean blankline ;
#endif
{
  printnl ( 283 ) ;
  if ( blankline ) 
  println () ;
  selector = oldsetting ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintdiagnostic ( strnumber s , strnumber t , boolean nuline ) 
#else
zprintdiagnostic ( s , t , nuline ) 
  strnumber s ;
  strnumber t ;
  boolean nuline ;
#endif
{
  begindiagnostic () ;
  if ( nuline ) 
  printnl ( s ) ;
  else print ( s ) ;
  print ( 449 ) ;
  printint ( line ) ;
  print ( t ) ;
  printchar ( 58 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintfilename ( integer n , integer a , integer e ) 
#else
zprintfilename ( n , a , e ) 
  integer n ;
  integer a ;
  integer e ;
#endif
{
  print ( a ) ;
  print ( n ) ;
  print ( e ) ;
} 
#ifdef TEXMF_DEBUG
#endif /* TEXMF_DEBUG */
void 
#ifdef HAVE_PROTOTYPES
zflushstring ( strnumber s ) 
#else
zflushstring ( s ) 
  strnumber s ;
#endif
{
  if ( s < strptr - 1 ) 
  strref [s ]= 0 ;
  else do {
      decr ( strptr ) ;
  } while ( ! ( strref [strptr - 1 ]!= 0 ) ) ;
  poolptr = strstart [strptr ];
} 
void 
#ifdef HAVE_PROTOTYPES
jumpout ( void ) 
#else
jumpout ( ) 
#endif
{
  closefilesandterminate () ;
  {
    fflush ( stdout ) ;
    readyalready = 0 ;
    if ( ( history != 0 ) && ( history != 1 ) ) 
    uexit ( 1 ) ;
    else uexit ( 0 ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
error ( void ) 
#else
error ( ) 
#endif
{
  /* 22 10 */ ASCIIcode c  ;
  integer s1, s2, s3  ;
  poolpointer j  ;
  if ( history < 2 ) 
  history = 2 ;
  printchar ( 46 ) ;
  showcontext () ;
  if ( interaction == 3 ) 
  while ( true ) {
      
    lab22: clearforerrorprompt () ;
    {
      ;
      print ( 263 ) ;
      terminput () ;
    } 
    if ( last == first ) 
    goto lab10 ;
    c = buffer [first ];
    if ( c >= 97 ) 
    c = c - 32 ;
    switch ( c ) 
    {case 48 : 
    case 49 : 
    case 50 : 
    case 51 : 
    case 52 : 
    case 53 : 
    case 54 : 
    case 55 : 
    case 56 : 
    case 57 : 
      if ( deletionsallowed ) 
      {
	s1 = curcmd ;
	s2 = curmod ;
	s3 = cursym ;
	OKtointerrupt = false ;
	if ( ( last > first + 1 ) && ( buffer [first + 1 ]>= 48 ) && ( 
	buffer [first + 1 ]<= 57 ) ) 
	c = c * 10 + buffer [first + 1 ]- 48 * 11 ;
	else c = c - 48 ;
	while ( c > 0 ) {
	    
	  getnext () ;
	  if ( curcmd == 39 ) 
	  {
	    if ( strref [curmod ]< 127 ) 
	    if ( strref [curmod ]> 1 ) 
	    decr ( strref [curmod ]) ;
	    else flushstring ( curmod ) ;
	  } 
	  decr ( c ) ;
	} 
	curcmd = s1 ;
	curmod = s2 ;
	cursym = s3 ;
	OKtointerrupt = true ;
	{
	  helpptr = 2 ;
	  helpline [1 ]= 276 ;
	  helpline [0 ]= 277 ;
	} 
	showcontext () ;
	goto lab22 ;
      } 
      break ;
	;
#ifdef TEXMF_DEBUG
    case 68 : 
      {
	debughelp () ;
	goto lab22 ;
      } 
      break ;
#endif /* TEXMF_DEBUG */
    case 69 : 
      if ( fileptr > 0 ) 
      {
	editnamestart = strstart [inputstack [fileptr ].namefield ];
	editnamelength = strstart [inputstack [fileptr ].namefield + 1 ]- 
	strstart [inputstack [fileptr ].namefield ];
	editline = line ;
	jumpout () ;
      } 
      break ;
    case 72 : 
      {
	if ( useerrhelp ) 
	{
	  j = strstart [errhelp ];
	  while ( j < strstart [errhelp + 1 ]) {
	      
	    if ( strpool [j ]!= 37 ) 
	    print ( strpool [j ]) ;
	    else if ( j + 1 == strstart [errhelp + 1 ]) 
	    println () ;
	    else if ( strpool [j + 1 ]!= 37 ) 
	    println () ;
	    else {
		
	      incr ( j ) ;
	      printchar ( 37 ) ;
	    } 
	    incr ( j ) ;
	  } 
	  useerrhelp = false ;
	} 
	else {
	    
	  if ( helpptr == 0 ) 
	  {
	    helpptr = 2 ;
	    helpline [1 ]= 278 ;
	    helpline [0 ]= 279 ;
	  } 
	  do {
	      decr ( helpptr ) ;
	    print ( helpline [helpptr ]) ;
	    println () ;
	  } while ( ! ( helpptr == 0 ) ) ;
	} 
	{
	  helpptr = 4 ;
	  helpline [3 ]= 280 ;
	  helpline [2 ]= 279 ;
	  helpline [1 ]= 281 ;
	  helpline [0 ]= 282 ;
	} 
	goto lab22 ;
      } 
      break ;
    case 73 : 
      {
	beginfilereading () ;
	if ( last > first + 1 ) 
	{
	  curinput .locfield = first + 1 ;
	  buffer [first ]= 32 ;
	} 
	else {
	    
	  {
	    ;
	    print ( 275 ) ;
	    terminput () ;
	  } 
	  curinput .locfield = first ;
	} 
	first = last + 1 ;
	curinput .limitfield = last ;
	goto lab10 ;
      } 
      break ;
    case 81 : 
    case 82 : 
    case 83 : 
      {
	errorcount = 0 ;
	interaction = 0 + c - 81 ;
	print ( 270 ) ;
	switch ( c ) 
	{case 81 : 
	  {
	    print ( 271 ) ;
	    decr ( selector ) ;
	  } 
	  break ;
	case 82 : 
	  print ( 272 ) ;
	  break ;
	case 83 : 
	  print ( 273 ) ;
	  break ;
	} 
	print ( 274 ) ;
	println () ;
	fflush ( stdout ) ;
	goto lab10 ;
      } 
      break ;
    case 88 : 
      {
	interaction = 2 ;
	jumpout () ;
      } 
      break ;
      default: 
      ;
      break ;
    } 
    {
      print ( 264 ) ;
      printnl ( 265 ) ;
      printnl ( 266 ) ;
      if ( fileptr > 0 ) 
      print ( 267 ) ;
      if ( deletionsallowed ) 
      printnl ( 268 ) ;
      printnl ( 269 ) ;
    } 
  } 
  incr ( errorcount ) ;
  if ( errorcount == 100 ) 
  {
    printnl ( 262 ) ;
    history = 3 ;
    jumpout () ;
  } 
  if ( interaction > 0 ) 
  decr ( selector ) ;
  if ( useerrhelp ) 
  {
    printnl ( 283 ) ;
    j = strstart [errhelp ];
    while ( j < strstart [errhelp + 1 ]) {
	
      if ( strpool [j ]!= 37 ) 
      print ( strpool [j ]) ;
      else if ( j + 1 == strstart [errhelp + 1 ]) 
      println () ;
      else if ( strpool [j + 1 ]!= 37 ) 
      println () ;
      else {
	  
	incr ( j ) ;
	printchar ( 37 ) ;
      } 
      incr ( j ) ;
    } 
  } 
  else while ( helpptr > 0 ) {
      
    decr ( helpptr ) ;
    printnl ( helpline [helpptr ]) ;
  } 
  println () ;
  if ( interaction > 0 ) 
  incr ( selector ) ;
  println () ;
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zfatalerror ( strnumber s ) 
#else
zfatalerror ( s ) 
  strnumber s ;
#endif
{
  normalizeselector () ;
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 261 ) ;
    print ( 284 ) ;
  } 
  {
    helpptr = 1 ;
    helpline [0 ]= s ;
  } 
  {
    if ( interaction == 3 ) 
    interaction = 2 ;
    if ( logopened ) 
    error () ;
	;
#ifdef TEXMF_DEBUG
    if ( interaction > 0 ) 
    debughelp () ;
#endif /* TEXMF_DEBUG */
    history = 3 ;
    jumpout () ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zoverflow ( strnumber s , integer n ) 
#else
zoverflow ( s , n ) 
  strnumber s ;
  integer n ;
#endif
{
  normalizeselector () ;
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 261 ) ;
    print ( 285 ) ;
  } 
  print ( s ) ;
  printchar ( 61 ) ;
  printint ( n ) ;
  printchar ( 93 ) ;
  {
    helpptr = 2 ;
    helpline [1 ]= 286 ;
    helpline [0 ]= 287 ;
  } 
  {
    if ( interaction == 3 ) 
    interaction = 2 ;
    if ( logopened ) 
    error () ;
	;
#ifdef TEXMF_DEBUG
    if ( interaction > 0 ) 
    debughelp () ;
#endif /* TEXMF_DEBUG */
    history = 3 ;
    jumpout () ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zconfusion ( strnumber s ) 
#else
zconfusion ( s ) 
  strnumber s ;
#endif
{
  normalizeselector () ;
  if ( history < 2 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 261 ) ;
      print ( 288 ) ;
    } 
    print ( s ) ;
    printchar ( 41 ) ;
    {
      helpptr = 1 ;
      helpline [0 ]= 289 ;
    } 
  } 
  else {
      
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 261 ) ;
      print ( 290 ) ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 291 ;
      helpline [0 ]= 292 ;
    } 
  } 
  {
    if ( interaction == 3 ) 
    interaction = 2 ;
    if ( logopened ) 
    error () ;
	;
#ifdef TEXMF_DEBUG
    if ( interaction > 0 ) 
    debughelp () ;
#endif /* TEXMF_DEBUG */
    history = 3 ;
    jumpout () ;
  } 
} 
boolean 
#ifdef HAVE_PROTOTYPES
initterminal ( void ) 
#else
initterminal ( ) 
#endif
{
  /* 10 */ register boolean Result; topenin () ;
  if ( last > first ) 
  {
    curinput .locfield = first ;
    while ( ( curinput .locfield < last ) && ( buffer [curinput .locfield ]
    == ' ' ) ) incr ( curinput .locfield ) ;
    if ( curinput .locfield < last ) 
    {
      Result = true ;
      goto lab10 ;
    } 
  } 
  while ( true ) {
      
    ;
    Fputs( stdout ,  "**" ) ;
    fflush ( stdout ) ;
    if ( ! inputln ( stdin , true ) ) 
    {
      putc ('\n',  stdout );
      Fputs( stdout ,  "! End of file on the terminal... why?" ) ;
      Result = false ;
      goto lab10 ;
    } 
    curinput .locfield = first ;
    while ( ( curinput .locfield < last ) && ( buffer [curinput .locfield ]
    == 32 ) ) incr ( curinput .locfield ) ;
    if ( curinput .locfield < last ) 
    {
      Result = true ;
      goto lab10 ;
    } 
    fprintf( stdout , "%s\n",  "Please type the name of your input file." ) ;
  } 
  lab10: ;
  return Result ;
} 
strnumber 
#ifdef HAVE_PROTOTYPES
makestring ( void ) 
#else
makestring ( ) 
#endif
{
  register strnumber Result; if ( strptr == maxstrptr ) 
  {
    if ( strptr == maxstrings ) 
    overflow ( 258 , maxstrings - initstrptr ) ;
    incr ( maxstrptr ) ;
  } 
  strref [strptr ]= 1 ;
  incr ( strptr ) ;
  strstart [strptr ]= poolptr ;
  Result = strptr - 1 ;
  return Result ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
zstreqbuf ( strnumber s , integer k ) 
#else
zstreqbuf ( s , k ) 
  strnumber s ;
  integer k ;
#endif
{
  /* 45 */ register boolean Result; poolpointer j  ;
  boolean result  ;
  j = strstart [s ];
  while ( j < strstart [s + 1 ]) {
      
    if ( strpool [j ]!= buffer [k ]) 
    {
      result = false ;
      goto lab45 ;
    } 
    incr ( j ) ;
    incr ( k ) ;
  } 
  result = true ;
  lab45: Result = result ;
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
zstrvsstr ( strnumber s , strnumber t ) 
#else
zstrvsstr ( s , t ) 
  strnumber s ;
  strnumber t ;
#endif
{
  /* 10 */ register integer Result; poolpointer j, k  ;
  integer ls, lt  ;
  integer l  ;
  ls = ( strstart [s + 1 ]- strstart [s ]) ;
  lt = ( strstart [t + 1 ]- strstart [t ]) ;
  if ( ls <= lt ) 
  l = ls ;
  else l = lt ;
  j = strstart [s ];
  k = strstart [t ];
  while ( l > 0 ) {
      
    if ( strpool [j ]!= strpool [k ]) 
    {
      Result = strpool [j ]- strpool [k ];
      goto lab10 ;
    } 
    incr ( j ) ;
    incr ( k ) ;
    decr ( l ) ;
  } 
  Result = ls - lt ;
  lab10: ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintdd ( integer n ) 
#else
zprintdd ( n ) 
  integer n ;
#endif
{
  n = abs ( n ) % 100 ;
  printchar ( 48 + ( n / 10 ) ) ;
  printchar ( 48 + ( n % 10 ) ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
terminput ( void ) 
#else
terminput ( ) 
#endif
{
  integer k  ;
  fflush ( stdout ) ;
  if ( ! inputln ( stdin , true ) ) 
  fatalerror ( 260 ) ;
  termoffset = 0 ;
  decr ( selector ) ;
  if ( last != first ) 
  {register integer for_end; k = first ;for_end = last - 1 ; if ( k <= 
  for_end) do 
    print ( buffer [k ]) ;
  while ( k++ < for_end ) ;} 
  println () ;
  buffer [last ]= 37 ;
  incr ( selector ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
normalizeselector ( void ) 
#else
normalizeselector ( ) 
#endif
{
  if ( logopened ) 
  selector = 3 ;
  else selector = 1 ;
  if ( jobname == 0 ) 
  openlogfile () ;
  if ( interaction == 0 ) 
  decr ( selector ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
pauseforinstructions ( void ) 
#else
pauseforinstructions ( ) 
#endif
{
  if ( OKtointerrupt ) 
  {
    interaction = 3 ;
    if ( ( selector == 2 ) || ( selector == 0 ) ) 
    incr ( selector ) ;
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 261 ) ;
      print ( 293 ) ;
    } 
    {
      helpptr = 3 ;
      helpline [2 ]= 294 ;
      helpline [1 ]= 295 ;
      helpline [0 ]= 296 ;
    } 
    deletionsallowed = false ;
    error () ;
    deletionsallowed = true ;
    interrupt = 0 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zmissingerr ( strnumber s ) 
#else
zmissingerr ( s ) 
  strnumber s ;
#endif
{
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 261 ) ;
    print ( 297 ) ;
  } 
  print ( s ) ;
  print ( 298 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
cleararith ( void ) 
#else
cleararith ( ) 
#endif
{
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 261 ) ;
    print ( 299 ) ;
  } 
  {
    helpptr = 4 ;
    helpline [3 ]= 300 ;
    helpline [2 ]= 301 ;
    helpline [1 ]= 302 ;
    helpline [0 ]= 303 ;
  } 
  error () ;
  aritherror = false ;
} 
integer 
#ifdef HAVE_PROTOTYPES
zslowadd ( integer x , integer y ) 
#else
zslowadd ( x , y ) 
  integer x ;
  integer y ;
#endif
{
  register integer Result; if ( x >= 0 ) 
  if ( y <= 2147483647L - x ) 
  Result = x + y ;
  else {
      
    aritherror = true ;
    Result = 2147483647L ;
  } 
  else if ( - (integer) y <= 2147483647L + x ) 
  Result = x + y ;
  else {
      
    aritherror = true ;
    Result = -2147483647L ;
  } 
  return Result ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zrounddecimals ( smallnumber k ) 
#else
zrounddecimals ( k ) 
  smallnumber k ;
#endif
{
  register scaled Result; integer a  ;
  a = 0 ;
  while ( k > 0 ) {
      
    decr ( k ) ;
    a = ( a + dig [k ]* 131072L ) / 10 ;
  } 
  Result = halfp ( a + 1 ) ;
  return Result ;
} 
#ifdef FIXPT
fraction 
#ifdef HAVE_PROTOTYPES
zmakefraction ( integer p , integer q ) 
#else
zmakefraction ( p , q ) 
  integer p ;
  integer q ;
#endif
{
  register fraction Result; integer f  ;
  integer n  ;
  boolean negative  ;
  integer becareful  ;
  if ( p >= 0 ) 
  negative = false ;
  else {
      
    p = - (integer) p ;
    negative = true ;
  } 
  if ( q <= 0 ) 
  {
	;
#ifdef TEXMF_DEBUG
    if ( q == 0 ) 
    confusion ( 47 ) ;
#endif /* TEXMF_DEBUG */
    q = - (integer) q ;
    negative = ! negative ;
  } 
  n = p / q ;
  p = p % q ;
  if ( n >= 8 ) 
  {
    aritherror = true ;
    if ( negative ) 
    Result = -2147483647L ;
    else Result = 2147483647L ;
  } 
  else {
      
    n = ( n - 1 ) * 268435456L ;
    f = 1 ;
    do {
	becareful = p - q ;
      p = becareful + p ;
      if ( p >= 0 ) 
      f = f + f + 1 ;
      else {
	  
	f = f + f ;
	p = p + q ;
      } 
    } while ( ! ( f >= 268435456L ) ) ;
    becareful = p - q ;
    if ( becareful + p >= 0 ) 
    incr ( f ) ;
    if ( negative ) 
    Result = - (integer) ( f + n ) ;
    else Result = f + n ;
  } 
  return Result ;
} 
#endif /* FIXPT */
#ifdef FIXPT
integer 
#ifdef HAVE_PROTOTYPES
ztakefraction ( integer q , fraction f ) 
#else
ztakefraction ( q , f ) 
  integer q ;
  fraction f ;
#endif
{
  register integer Result; integer p  ;
  boolean negative  ;
  integer n  ;
  integer becareful  ;
  if ( f >= 0 ) 
  negative = false ;
  else {
      
    f = - (integer) f ;
    negative = true ;
  } 
  if ( q < 0 ) 
  {
    q = - (integer) q ;
    negative = ! negative ;
  } 
  if ( f < 268435456L ) 
  n = 0 ;
  else {
      
    n = f / 268435456L ;
    f = f % 268435456L ;
    if ( q <= 2147483647L / n ) 
    n = n * q ;
    else {
	
      aritherror = true ;
      n = 2147483647L ;
    } 
  } 
  f = f + 268435456L ;
  p = 134217728L ;
  if ( q < 1073741824L ) 
  do {
      if ( odd ( f ) ) 
    p = halfp ( p + q ) ;
    else p = halfp ( p ) ;
    f = halfp ( f ) ;
  } while ( ! ( f == 1 ) ) ;
  else do {
      if ( odd ( f ) ) 
    p = p + halfp ( q - p ) ;
    else p = halfp ( p ) ;
    f = halfp ( f ) ;
  } while ( ! ( f == 1 ) ) ;
  becareful = n - 2147483647L ;
  if ( becareful + p > 0 ) 
  {
    aritherror = true ;
    n = 2147483647L - p ;
  } 
  if ( negative ) 
  Result = - (integer) ( n + p ) ;
  else Result = n + p ;
  return Result ;
} 
#endif /* FIXPT */
#ifdef FIXPT
integer 
#ifdef HAVE_PROTOTYPES
ztakescaled ( integer q , scaled f ) 
#else
ztakescaled ( q , f ) 
  integer q ;
  scaled f ;
#endif
{
  register integer Result; integer p  ;
  boolean negative  ;
  integer n  ;
  integer becareful  ;
  if ( f >= 0 ) 
  negative = false ;
  else {
      
    f = - (integer) f ;
    negative = true ;
  } 
  if ( q < 0 ) 
  {
    q = - (integer) q ;
    negative = ! negative ;
  } 
  if ( f < 65536L ) 
  n = 0 ;
  else {
      
    n = f / 65536L ;
    f = f % 65536L ;
    if ( q <= 2147483647L / n ) 
    n = n * q ;
    else {
	
      aritherror = true ;
      n = 2147483647L ;
    } 
  } 
  f = f + 65536L ;
  p = 32768L ;
  if ( q < 1073741824L ) 
  do {
      if ( odd ( f ) ) 
    p = halfp ( p + q ) ;
    else p = halfp ( p ) ;
    f = halfp ( f ) ;
  } while ( ! ( f == 1 ) ) ;
  else do {
      if ( odd ( f ) ) 
    p = p + halfp ( q - p ) ;
    else p = halfp ( p ) ;
    f = halfp ( f ) ;
  } while ( ! ( f == 1 ) ) ;
  becareful = n - 2147483647L ;
  if ( becareful + p > 0 ) 
  {
    aritherror = true ;
    n = 2147483647L - p ;
  } 
  if ( negative ) 
  Result = - (integer) ( n + p ) ;
  else Result = n + p ;
  return Result ;
} 
#endif /* FIXPT */
#ifdef FIXPT
scaled 
#ifdef HAVE_PROTOTYPES
zmakescaled ( integer p , integer q ) 
#else
zmakescaled ( p , q ) 
  integer p ;
  integer q ;
#endif
{
  register scaled Result; integer f  ;
  integer n  ;
  boolean negative  ;
  integer becareful  ;
  if ( p >= 0 ) 
  negative = false ;
  else {
      
    p = - (integer) p ;
    negative = true ;
  } 
  if ( q <= 0 ) 
  {
	;
#ifdef TEXMF_DEBUG
    if ( q == 0 ) 
    confusion ( 47 ) ;
#endif /* TEXMF_DEBUG */
    q = - (integer) q ;
    negative = ! negative ;
  } 
  n = p / q ;
  p = p % q ;
  if ( n >= 32768L ) 
  {
    aritherror = true ;
    if ( negative ) 
    Result = -2147483647L ;
    else Result = 2147483647L ;
  } 
  else {
      
    n = ( n - 1 ) * 65536L ;
    f = 1 ;
    do {
	becareful = p - q ;
      p = becareful + p ;
      if ( p >= 0 ) 
      f = f + f + 1 ;
      else {
	  
	f = f + f ;
	p = p + q ;
      } 
    } while ( ! ( f >= 65536L ) ) ;
    becareful = p - q ;
    if ( becareful + p >= 0 ) 
    incr ( f ) ;
    if ( negative ) 
    Result = - (integer) ( f + n ) ;
    else Result = f + n ;
  } 
  return Result ;
} 
#endif /* FIXPT */
fraction 
#ifdef HAVE_PROTOTYPES
zvelocity ( fraction st , fraction ct , fraction sf , fraction cf , scaled t ) 
#else
zvelocity ( st , ct , sf , cf , t ) 
  fraction st ;
  fraction ct ;
  fraction sf ;
  fraction cf ;
  scaled t ;
#endif
{
  register fraction Result; integer acc, num, denom  ;
  acc = takefraction ( st - ( sf / 16 ) , sf - ( st / 16 ) ) ;
  acc = takefraction ( acc , ct - cf ) ;
  num = 536870912L + takefraction ( acc , 379625062L ) ;
  denom = 805306368L + takefraction ( ct , 497706707L ) + takefraction ( cf , 
  307599661L ) ;
  if ( t != 65536L ) 
  num = makescaled ( num , t ) ;
  if ( num / 4 >= denom ) 
  Result = 1073741824L ;
  else Result = makefraction ( num , denom ) ;
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
zabvscd ( integer a , integer b , integer c , integer d ) 
#else
zabvscd ( a , b , c , d ) 
  integer a ;
  integer b ;
  integer c ;
  integer d ;
#endif
{
  /* 10 */ register integer Result; integer q, r  ;
  if ( a < 0 ) 
  {
    a = - (integer) a ;
    b = - (integer) b ;
  } 
  if ( c < 0 ) 
  {
    c = - (integer) c ;
    d = - (integer) d ;
  } 
  if ( d <= 0 ) 
  {
    if ( b >= 0 ) 
    if ( ( ( a == 0 ) || ( b == 0 ) ) && ( ( c == 0 ) || ( d == 0 ) ) ) 
    {
      Result = 0 ;
      goto lab10 ;
    } 
    else {
	
      Result = 1 ;
      goto lab10 ;
    } 
    if ( d == 0 ) 
    if ( a == 0 ) 
    {
      Result = 0 ;
      goto lab10 ;
    } 
    else {
	
      Result = -1 ;
      goto lab10 ;
    } 
    q = a ;
    a = c ;
    c = q ;
    q = - (integer) b ;
    b = - (integer) d ;
    d = q ;
  } 
  else if ( b <= 0 ) 
  {
    if ( b < 0 ) 
    if ( a > 0 ) 
    {
      Result = -1 ;
      goto lab10 ;
    } 
    if ( c == 0 ) 
    {
      Result = 0 ;
      goto lab10 ;
    } 
    else {
	
      Result = -1 ;
      goto lab10 ;
    } 
  } 
  while ( true ) {
      
    q = a / d ;
    r = c / b ;
    if ( q != r ) 
    if ( q > r ) 
    {
      Result = 1 ;
      goto lab10 ;
    } 
    else {
	
      Result = -1 ;
      goto lab10 ;
    } 
    q = a % d ;
    r = c % b ;
    if ( r == 0 ) 
    if ( q == 0 ) 
    {
      Result = 0 ;
      goto lab10 ;
    } 
    else {
	
      Result = 1 ;
      goto lab10 ;
    } 
    if ( q == 0 ) 
    {
      Result = -1 ;
      goto lab10 ;
    } 
    a = b ;
    b = q ;
    c = d ;
    d = r ;
  } 
  lab10: ;
  return Result ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zsquarert ( scaled x ) 
#else
zsquarert ( x ) 
  scaled x ;
#endif
{
  register scaled Result; smallnumber k  ;
  integer y, q  ;
  if ( x <= 0 ) 
  {
    if ( x < 0 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 261 ) ;
	print ( 304 ) ;
      } 
      printscaled ( x ) ;
      print ( 305 ) ;
      {
	helpptr = 2 ;
	helpline [1 ]= 306 ;
	helpline [0 ]= 307 ;
      } 
      error () ;
    } 
    Result = 0 ;
  } 
  else {
      
    k = 23 ;
    q = 2 ;
    while ( x < 536870912L ) {
	
      decr ( k ) ;
      x = x + x + x + x ;
    } 
    if ( x < 1073741824L ) 
    y = 0 ;
    else {
	
      x = x - 1073741824L ;
      y = 1 ;
    } 
    do {
	x = x + x ;
      y = y + y ;
      if ( x >= 1073741824L ) 
      {
	x = x - 1073741824L ;
	incr ( y ) ;
      } 
      x = x + x ;
      y = y + y - q ;
      q = q + q ;
      if ( x >= 1073741824L ) 
      {
	x = x - 1073741824L ;
	incr ( y ) ;
      } 
      if ( y > q ) 
      {
	y = y - q ;
	q = q + 2 ;
      } 
      else if ( y <= 0 ) 
      {
	q = q - 2 ;
	y = y + q ;
      } 
      decr ( k ) ;
    } while ( ! ( k == 0 ) ) ;
    Result = halfp ( q ) ;
  } 
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
zpythadd ( integer a , integer b ) 
#else
zpythadd ( a , b ) 
  integer a ;
  integer b ;
#endif
{
  /* 30 */ register integer Result; fraction r  ;
  boolean big  ;
  a = abs ( a ) ;
  b = abs ( b ) ;
  if ( a < b ) 
  {
    r = b ;
    b = a ;
    a = r ;
  } 
  if ( b > 0 ) 
  {
    if ( a < 536870912L ) 
    big = false ;
    else {
	
      a = a / 4 ;
      b = b / 4 ;
      big = true ;
    } 
    while ( true ) {
	
      r = makefraction ( b , a ) ;
      r = takefraction ( r , r ) ;
      if ( r == 0 ) 
      goto lab30 ;
      r = makefraction ( r , 1073741824L + r ) ;
      a = a + takefraction ( a + a , r ) ;
      b = takefraction ( b , r ) ;
    } 
    lab30: ;
    if ( big ) 
    if ( a < 536870912L ) 
    a = a + a + a + a ;
    else {
	
      aritherror = true ;
      a = 2147483647L ;
    } 
  } 
  Result = a ;
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
zpythsub ( integer a , integer b ) 
#else
zpythsub ( a , b ) 
  integer a ;
  integer b ;
#endif
{
  /* 30 */ register integer Result; fraction r  ;
  boolean big  ;
  a = abs ( a ) ;
  b = abs ( b ) ;
  if ( a <= b ) 
  {
    if ( a < b ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 261 ) ;
	print ( 308 ) ;
      } 
      printscaled ( a ) ;
      print ( 309 ) ;
      printscaled ( b ) ;
      print ( 305 ) ;
      {
	helpptr = 2 ;
	helpline [1 ]= 306 ;
	helpline [0 ]= 307 ;
      } 
      error () ;
    } 
    a = 0 ;
  } 
  else {
      
    if ( a < 1073741824L ) 
    big = false ;
    else {
	
      a = halfp ( a ) ;
      b = halfp ( b ) ;
      big = true ;
    } 
    while ( true ) {
	
      r = makefraction ( b , a ) ;
      r = takefraction ( r , r ) ;
      if ( r == 0 ) 
      goto lab30 ;
      r = makefraction ( r , 1073741824L - r ) ;
      a = a - takefraction ( a + a , r ) ;
      b = takefraction ( b , r ) ;
    } 
    lab30: ;
    if ( big ) 
    a = a + a ;
  } 
  Result = a ;
  return Result ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zmlog ( scaled x ) 
#else
zmlog ( x ) 
  scaled x ;
#endif
{
  register scaled Result; integer y, z  ;
  integer k  ;
  if ( x <= 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 261 ) ;
      print ( 310 ) ;
    } 
    printscaled ( x ) ;
    print ( 305 ) ;
    {
      helpptr = 2 ;
      helpline [1 ]= 311 ;
      helpline [0 ]= 307 ;
    } 
    error () ;
    Result = 0 ;
  } 
  else {
      
    y = 1302456860L ;
    z = 6581195L ;
    while ( x < 1073741824L ) {
	
      x = x + x ;
      y = y - 93032639L ;
      z = z - 48782L ;
    } 
    y = y + ( z / 65536L ) ;
    k = 2 ;
    while ( x > 1073741828L ) {
	
      z = ( ( x - 1 ) / twotothe [k ]) + 1 ;
      while ( x < 1073741824L + z ) {
	  
	z = halfp ( z + 1 ) ;
	k = k + 1 ;
      } 
      y = y + speclog [k ];
      x = x - z ;
    } 
    Result = y / 8 ;
  } 
  return Result ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zmexp ( scaled x ) 
#else
zmexp ( x ) 
  scaled x ;
#endif
{
  register scaled Result; smallnumber k  ;
  integer y, z  ;
  if ( x > 174436200L ) 
  {
    aritherror = true ;
    Result = 2147483647L ;
  } 
  else if ( x < -197694359L ) 
  Result = 0 ;
  else {
      
    if ( x <= 0 ) 
    {
      z = -8 * x ;
      y = 1048576L ;
    } 
    else {
	
      if ( x <= 127919879L ) 
      z = 1023359037L - 8 * x ;
      else z = 8 * ( 174436200L - x ) ;
      y = 2147483647L ;
    } 
    k = 1 ;
    while ( z > 0 ) {
	
      while ( z >= speclog [k ]) {
	  
	z = z - speclog [k ];
	y = y - 1 - ( ( y - twotothe [k - 1 ]) / twotothe [k ]) ;
      } 
      incr ( k ) ;
    } 
    if ( x <= 127919879L ) 
    Result = ( y + 8 ) / 16 ;
    else Result = y ;
  } 
  return Result ;
} 
angle 
#ifdef HAVE_PROTOTYPES
znarg ( integer x , integer y ) 
#else
znarg ( x , y ) 
  integer x ;
  integer y ;
#endif
{
  register angle Result; angle z  ;
  integer t  ;
  smallnumber k  ;
  char octant  ;
  if ( x >= 0 ) 
  octant = 1 ;
  else {
      
    x = - (integer) x ;
    octant = 2 ;
  } 
  if ( y < 0 ) 
  {
    y = - (integer) y ;
    octant = octant + 2 ;
  } 
  if ( x < y ) 
  {
    t = y ;
    y = x ;
    x = t ;
    octant = octant + 4 ;
  } 
  if ( x == 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 261 ) ;
      print ( 312 ) ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 313 ;
      helpline [0 ]= 307 ;
    } 
    error () ;
    Result = 0 ;
  } 
  else {
      
    while ( x >= 536870912L ) {
	
      x = halfp ( x ) ;
      y = halfp ( y ) ;
    } 
    z = 0 ;
    if ( y > 0 ) 
    {
      while ( x < 268435456L ) {
	  
	x = x + x ;
	y = y + y ;
      } 
      k = 0 ;
      do {
	  y = y + y ;
	incr ( k ) ;
	if ( y > x ) 
	{
	  z = z + specatan [k ];
	  t = x ;
	  x = x + ( y / twotothe [k + k ]) ;
	  y = y - t ;
	} 
      } while ( ! ( k == 15 ) ) ;
      do {
	  y = y + y ;
	incr ( k ) ;
	if ( y > x ) 
	{
	  z = z + specatan [k ];
	  y = y - x ;
	} 
      } while ( ! ( k == 26 ) ) ;
    } 
    switch ( octant ) 
    {case 1 : 
      Result = z ;
      break ;
    case 5 : 
      Result = 94371840L - z ;
      break ;
    case 6 : 
      Result = 94371840L + z ;
      break ;
    case 2 : 
      Result = 188743680L - z ;
      break ;
    case 4 : 
      Result = z - 188743680L ;
      break ;
    case 8 : 
      Result = - (integer) z - 94371840L ;
      break ;
    case 7 : 
      Result = z - 94371840L ;
      break ;
    case 3 : 
      Result = - (integer) z ;
      break ;
    } 
  } 
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
znsincos ( angle z ) 
#else
znsincos ( z ) 
  angle z ;
#endif
{
  smallnumber k  ;
  char q  ;
  fraction r  ;
  integer x, y, t  ;
  while ( z < 0 ) z = z + 377487360L ;
  z = z % 377487360L ;
  q = z / 47185920L ;
  z = z % 47185920L ;
  x = 268435456L ;
  y = x ;
  if ( ! odd ( q ) ) 
  z = 47185920L - z ;
  k = 1 ;
  while ( z > 0 ) {
      
    if ( z >= specatan [k ]) 
    {
      z = z - specatan [k ];
      t = x ;
      x = t + y / twotothe [k ];
      y = y - t / twotothe [k ];
    } 
    incr ( k ) ;
  } 
  if ( y < 0 ) 
  y = 0 ;
  switch ( q ) 
  {case 0 : 
    ;
    break ;
  case 1 : 
    {
      t = x ;
      x = y ;
      y = t ;
    } 
    break ;
  case 2 : 
    {
      t = x ;
      x = - (integer) y ;
      y = t ;
    } 
    break ;
  case 3 : 
    x = - (integer) x ;
    break ;
  case 4 : 
    {
      x = - (integer) x ;
      y = - (integer) y ;
    } 
    break ;
  case 5 : 
    {
      t = x ;
      x = - (integer) y ;
      y = - (integer) t ;
    } 
    break ;
  case 6 : 
    {
      t = x ;
      x = y ;
      y = - (integer) t ;
    } 
    break ;
  case 7 : 
    y = - (integer) y ;
    break ;
  } 
  r = pythadd ( x , y ) ;
  ncos = makefraction ( x , r ) ;
  nsin = makefraction ( y , r ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
newrandoms ( void ) 
#else
newrandoms ( ) 
#endif
{
  char k  ;
  fraction x  ;
  {register integer for_end; k = 0 ;for_end = 23 ; if ( k <= for_end) do 
    {
      x = randoms [k ]- randoms [k + 31 ];
      if ( x < 0 ) 
      x = x + 268435456L ;
      randoms [k ]= x ;
    } 
  while ( k++ < for_end ) ;} 
  {register integer for_end; k = 24 ;for_end = 54 ; if ( k <= for_end) do 
    {
      x = randoms [k ]- randoms [k - 24 ];
      if ( x < 0 ) 
      x = x + 268435456L ;
      randoms [k ]= x ;
    } 
  while ( k++ < for_end ) ;} 
  jrandom = 54 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zinitrandoms ( scaled seed ) 
#else
zinitrandoms ( seed ) 
  scaled seed ;
#endif
{
  fraction j, jj, k  ;
  char i  ;
  j = abs ( seed ) ;
  while ( j >= 268435456L ) j = halfp ( j ) ;
  k = 1 ;
  {register integer for_end; i = 0 ;for_end = 54 ; if ( i <= for_end) do 
    {
      jj = k ;
      k = j - k ;
      j = jj ;
      if ( k < 0 ) 
      k = k + 268435456L ;
      randoms [( i * 21 ) % 55 ]= j ;
    } 
  while ( i++ < for_end ) ;} 
  newrandoms () ;
  newrandoms () ;
  newrandoms () ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zunifrand ( scaled x ) 
#else
zunifrand ( x ) 
  scaled x ;
#endif
{
  register scaled Result; scaled y  ;
  if ( jrandom == 0 ) 
  newrandoms () ;
  else decr ( jrandom ) ;
  y = takefraction ( abs ( x ) , randoms [jrandom ]) ;
  if ( y == abs ( x ) ) 
  Result = 0 ;
  else if ( x > 0 ) 
  Result = y ;
  else Result = - (integer) y ;
  return Result ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
normrand ( void ) 
#else
normrand ( ) 
#endif
{
  register scaled Result; integer x, u, l  ;
  do {
      do { if ( jrandom == 0 ) 
      newrandoms () ;
      else decr ( jrandom ) ;
      x = takefraction ( 112429L , randoms [jrandom ]- 134217728L ) ;
      if ( jrandom == 0 ) 
      newrandoms () ;
      else decr ( jrandom ) ;
      u = randoms [jrandom ];
    } while ( ! ( abs ( x ) < u ) ) ;
    x = makefraction ( x , u ) ;
    l = 139548960L - mlog ( u ) ;
  } while ( ! ( abvscd ( 1024 , l , x , x ) >= 0 ) ) ;
  Result = x ;
  return Result ;
} 
#ifdef TEXMF_DEBUG
void 
#ifdef HAVE_PROTOTYPES
zprintword ( memoryword w ) 
#else
zprintword ( w ) 
  memoryword w ;
#endif
{
  printint ( w .cint ) ;
  printchar ( 32 ) ;
  printscaled ( w .cint ) ;
  printchar ( 32 ) ;
  printscaled ( w .cint / 4096 ) ;
  println () ;
  printint ( w .hhfield .lhfield ) ;
  printchar ( 61 ) ;
  printint ( w .hhfield .b0 ) ;
  printchar ( 58 ) ;
  printint ( w .hhfield .b1 ) ;
  printchar ( 59 ) ;
  printint ( w .hhfield .v.RH ) ;
  printchar ( 32 ) ;
  printint ( w .qqqq .b0 ) ;
  printchar ( 58 ) ;
  printint ( w .qqqq .b1 ) ;
  printchar ( 58 ) ;
  printint ( w .qqqq .b2 ) ;
  printchar ( 58 ) ;
  printint ( w .qqqq .b3 ) ;
} 
#endif /* TEXMF_DEBUG */
void 
#ifdef HAVE_PROTOTYPES
zshowtokenlist ( integer p , integer q , integer l , integer nulltally ) 
#else
zshowtokenlist ( p , q , l , nulltally ) 
  integer p ;
  integer q ;
  integer l ;
  integer nulltally ;
#endif
{
  /* 10 */ smallnumber class, c  ;
  integer r, v  ;
  class = 3 ;
  tally = nulltally ;
  while ( ( p != 0 ) && ( tally < l ) ) {
      
    if ( p == q ) 
    {
      firstcount = tally ;
      trickcount = tally + 1 + errorline - halferrorline ;
      if ( trickcount < errorline ) 
      trickcount = errorline ;
    } 
    c = 9 ;
    if ( ( p < 0 ) || ( p > memend ) ) 
    {
      print ( 492 ) ;
      goto lab10 ;
    } 
    if ( p < himemmin ) 
    if ( mem [p ].hhfield .b1 == 12 ) 
    if ( mem [p ].hhfield .b0 == 16 ) 
    {
      if ( class == 0 ) 
      printchar ( 32 ) ;
      v = mem [p + 1 ].cint ;
      if ( v < 0 ) 
      {
	if ( class == 17 ) 
	printchar ( 32 ) ;
	printchar ( 91 ) ;
	printscaled ( v ) ;
	printchar ( 93 ) ;
	c = 18 ;
      } 
      else {
	  
	printscaled ( v ) ;
	c = 0 ;
      } 
    } 
    else if ( mem [p ].hhfield .b0 != 4 ) 
    print ( 495 ) ;
    else {
	
      printchar ( 34 ) ;
      print ( mem [p + 1 ].cint ) ;
      printchar ( 34 ) ;
      c = 4 ;
    } 
    else if ( ( mem [p ].hhfield .b1 != 11 ) || ( mem [p ].hhfield .b0 < 1 
    ) || ( mem [p ].hhfield .b0 > 19 ) ) 
    print ( 495 ) ;
    else {
	
      gpointer = p ;
      printcapsule () ;
      c = 8 ;
    } 
    else {
	
      r = mem [p ].hhfield .lhfield ;
      if ( r >= 9770 ) 
      {
	if ( r < 9920 ) 
	{
	  print ( 497 ) ;
	  r = r - ( 9770 ) ;
	} 
	else if ( r < 10070 ) 
	{
	  print ( 498 ) ;
	  r = r - ( 9920 ) ;
	} 
	else {
	    
	  print ( 499 ) ;
	  r = r - ( 10070 ) ;
	} 
	printint ( r ) ;
	printchar ( 41 ) ;
	c = 8 ;
      } 
      else if ( r < 1 ) 
      if ( r == 0 ) 
      {
	if ( class == 17 ) 
	printchar ( 32 ) ;
	print ( 496 ) ;
	c = 18 ;
      } 
      else print ( 493 ) ;
      else {
	  
	r = hash [r ].v.RH ;
	if ( ( r < 0 ) || ( r >= strptr ) ) 
	print ( 494 ) ;
	else {
	    
	  c = charclass [strpool [strstart [r ]]];
	  if ( c == class ) 
	  switch ( c ) 
	  {case 9 : 
	    printchar ( 46 ) ;
	    break ;
	  case 5 : 
	  case 6 : 
	  case 7 : 
	  case 8 : 
	    ;
	    break ;
	    default: 
	    printchar ( 32 ) ;
	    break ;
	  } 
	  print ( r ) ;
	} 
      } 
    } 
    class = c ;
    p = mem [p ].hhfield .v.RH ;
  } 
  if ( p != 0 ) 
  print ( 491 ) ;
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
runaway ( void ) 
#else
runaway ( ) 
#endif
{
  if ( scannerstatus > 2 ) 
  {
    printnl ( 634 ) ;
    switch ( scannerstatus ) 
    {case 3 : 
      print ( 635 ) ;
      break ;
    case 4 : 
    case 5 : 
      print ( 636 ) ;
      break ;
    case 6 : 
      print ( 637 ) ;
      break ;
    } 
    println () ;
    showtokenlist ( mem [memtop - 2 ].hhfield .v.RH , 0 , errorline - 10 , 0 
    ) ;
  } 
} 
halfword 
#ifdef HAVE_PROTOTYPES
getavail ( void ) 
#else
getavail ( ) 
#endif
{
  register halfword Result; halfword p  ;
  p = avail ;
  if ( p != 0 ) 
  avail = mem [avail ].hhfield .v.RH ;
  else if ( memend < memmax ) 
  {
    incr ( memend ) ;
    p = memend ;
  } 
  else {
      
    decr ( himemmin ) ;
    p = himemmin ;
    if ( himemmin <= lomemmax ) 
    {
      runaway () ;
      overflow ( 314 , memmax + 1 ) ;
    } 
  } 
  mem [p ].hhfield .v.RH = 0 ;
	;
#ifdef STAT
  incr ( dynused ) ;
#endif /* STAT */
  Result = p ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zgetnode ( integer s ) 
#else
zgetnode ( s ) 
  integer s ;
#endif
{
  /* 40 10 20 */ register halfword Result; halfword p  ;
  halfword q  ;
  integer r  ;
  integer t, tt  ;
  lab20: p = rover ;
  do {
      q = p + mem [p ].hhfield .lhfield ;
    while ( ( mem [q ].hhfield .v.RH == 268435455L ) ) {
	
      t = mem [q + 1 ].hhfield .v.RH ;
      tt = mem [q + 1 ].hhfield .lhfield ;
      if ( q == rover ) 
      rover = t ;
      mem [t + 1 ].hhfield .lhfield = tt ;
      mem [tt + 1 ].hhfield .v.RH = t ;
      q = q + mem [q ].hhfield .lhfield ;
    } 
    r = q - s ;
    if ( r > toint ( p + 1 ) ) 
    {
      mem [p ].hhfield .lhfield = r - p ;
      rover = p ;
      goto lab40 ;
    } 
    if ( r == p ) 
    if ( mem [p + 1 ].hhfield .v.RH != p ) 
    {
      rover = mem [p + 1 ].hhfield .v.RH ;
      t = mem [p + 1 ].hhfield .lhfield ;
      mem [rover + 1 ].hhfield .lhfield = t ;
      mem [t + 1 ].hhfield .v.RH = rover ;
      goto lab40 ;
    } 
    mem [p ].hhfield .lhfield = q - p ;
    p = mem [p + 1 ].hhfield .v.RH ;
  } while ( ! ( p == rover ) ) ;
  if ( s == 1073741824L ) 
  {
    Result = 268435455L ;
    goto lab10 ;
  } 
  if ( lomemmax + 2 < himemmin ) 
  if ( lomemmax + 2 <= 268435455L ) 
  {
    if ( himemmin - lomemmax >= 1998 ) 
    t = lomemmax + 1000 ;
    else t = lomemmax + 1 + ( himemmin - lomemmax ) / 2 ;
    if ( t > 268435455L ) 
    t = 268435455L ;
    p = mem [rover + 1 ].hhfield .lhfield ;
    q = lomemmax ;
    mem [p + 1 ].hhfield .v.RH = q ;
    mem [rover + 1 ].hhfield .lhfield = q ;
    mem [q + 1 ].hhfield .v.RH = rover ;
    mem [q + 1 ].hhfield .lhfield = p ;
    mem [q ].hhfield .v.RH = 268435455L ;
    mem [q ].hhfield .lhfield = t - lomemmax ;
    lomemmax = t ;
    mem [lomemmax ].hhfield .v.RH = 0 ;
    mem [lomemmax ].hhfield .lhfield = 0 ;
    rover = q ;
    goto lab20 ;
  } 
  overflow ( 314 , memmax + 1 ) ;
  lab40: mem [r ].hhfield .v.RH = 0 ;
	;
#ifdef STAT
  varused = varused + s ;
#endif /* STAT */
  Result = r ;
  lab10: ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zfreenode ( halfword p , halfword s ) 
#else
zfreenode ( p , s ) 
  halfword p ;
  halfword s ;
#endif
{
  halfword q  ;
  mem [p ].hhfield .lhfield = s ;
  mem [p ].hhfield .v.RH = 268435455L ;
  q = mem [rover + 1 ].hhfield .lhfield ;
  mem [p + 1 ].hhfield .lhfield = q ;
  mem [p + 1 ].hhfield .v.RH = rover ;
  mem [rover + 1 ].hhfield .lhfield = p ;
  mem [q + 1 ].hhfield .v.RH = p ;
	;
#ifdef STAT
  varused = varused - s ;
#endif /* STAT */
} 
void 
#ifdef HAVE_PROTOTYPES
zflushlist ( halfword p ) 
#else
zflushlist ( p ) 
  halfword p ;
#endif
{
  /* 30 */ halfword q, r  ;
  if ( p >= himemmin ) 
  if ( p != memtop ) 
  {
    r = p ;
    do {
	q = r ;
      r = mem [r ].hhfield .v.RH ;
	;
#ifdef STAT
      decr ( dynused ) ;
#endif /* STAT */
      if ( r < himemmin ) 
      goto lab30 ;
    } while ( ! ( r == memtop ) ) ;
    lab30: mem [q ].hhfield .v.RH = avail ;
    avail = p ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zflushnodelist ( halfword p ) 
#else
zflushnodelist ( p ) 
  halfword p ;
#endif
{
  halfword q  ;
  while ( p != 0 ) {
      
    q = p ;
    p = mem [p ].hhfield .v.RH ;
    if ( q < himemmin ) 
    freenode ( q , 2 ) ;
    else {
	
      mem [q ].hhfield .v.RH = avail ;
      avail = q ;
	;
#ifdef STAT
      decr ( dynused ) ;
#endif /* STAT */
    } 
  } 
} 
#ifdef TEXMF_DEBUG
void 
#ifdef HAVE_PROTOTYPES
zcheckmem ( boolean printlocs ) 
#else
zcheckmem ( printlocs ) 
  boolean printlocs ;
#endif
{
  /* 31 32 */ halfword p, q, r  ;
  boolean clobbered  ;
  {register integer for_end; p = 0 ;for_end = lomemmax ; if ( p <= for_end) 
  do 
    freearr [p ]= false ;
  while ( p++ < for_end ) ;} 
  {register integer for_end; p = himemmin ;for_end = memend ; if ( p <= 
  for_end) do 
    freearr [p ]= false ;
  while ( p++ < for_end ) ;} 
  p = avail ;
  q = 0 ;
  clobbered = false ;
  while ( p != 0 ) {
      
    if ( ( p > memend ) || ( p < himemmin ) ) 
    clobbered = true ;
    else if ( freearr [p ]) 
    clobbered = true ;
    if ( clobbered ) 
    {
      printnl ( 315 ) ;
      printint ( q ) ;
      goto lab31 ;
    } 
    freearr [p ]= true ;
    q = p ;
    p = mem [q ].hhfield .v.RH ;
  } 
  lab31: ;
  p = rover ;
  q = 0 ;
  clobbered = false ;
  do {
      if ( ( p >= lomemmax ) ) 
    clobbered = true ;
    else if ( ( mem [p + 1 ].hhfield .v.RH >= lomemmax ) ) 
    clobbered = true ;
    else if ( ! ( ( mem [p ].hhfield .v.RH == 268435455L ) ) || ( mem [p ]
    .hhfield .lhfield < 2 ) || ( p + mem [p ].hhfield .lhfield > lomemmax ) 
    || ( mem [mem [p + 1 ].hhfield .v.RH + 1 ].hhfield .lhfield != p ) ) 
    clobbered = true ;
    if ( clobbered ) 
    {
      printnl ( 316 ) ;
      printint ( q ) ;
      goto lab32 ;
    } 
    {register integer for_end; q = p ;for_end = p + mem [p ].hhfield 
    .lhfield - 1 ; if ( q <= for_end) do 
      {
	if ( freearr [q ]) 
	{
	  printnl ( 317 ) ;
	  printint ( q ) ;
	  goto lab32 ;
	} 
	freearr [q ]= true ;
      } 
    while ( q++ < for_end ) ;} 
    q = p ;
    p = mem [p + 1 ].hhfield .v.RH ;
  } while ( ! ( p == rover ) ) ;
  lab32: ;
  p = 0 ;
  while ( p <= lomemmax ) {
      
    if ( ( mem [p ].hhfield .v.RH == 268435455L ) ) 
    {
      printnl ( 318 ) ;
      printint ( p ) ;
    } 
    while ( ( p <= lomemmax ) && ! freearr [p ]) incr ( p ) ;
    while ( ( p <= lomemmax ) && freearr [p ]) incr ( p ) ;
  } 
  q = 13 ;
  p = mem [q ].hhfield .v.RH ;
  while ( p != 13 ) {
      
    if ( mem [p + 1 ].hhfield .lhfield != q ) 
    {
      printnl ( 595 ) ;
      printint ( p ) ;
    } 
    p = mem [p + 1 ].hhfield .v.RH ;
    r = 19 ;
    do {
	if ( mem [mem [p ].hhfield .lhfield + 1 ].cint >= mem [r + 1 ]
      .cint ) 
      {
	printnl ( 596 ) ;
	printint ( p ) ;
      } 
      r = mem [p ].hhfield .lhfield ;
      q = p ;
      p = mem [q ].hhfield .v.RH ;
    } while ( ! ( r == 0 ) ) ;
  } 
  if ( printlocs ) 
  {
    printnl ( 319 ) ;
    {register integer for_end; p = 0 ;for_end = lomemmax ; if ( p <= 
    for_end) do 
      if ( ! freearr [p ]&& ( ( p > waslomax ) || wasfree [p ]) ) 
      {
	printchar ( 32 ) ;
	printint ( p ) ;
      } 
    while ( p++ < for_end ) ;} 
    {register integer for_end; p = himemmin ;for_end = memend ; if ( p <= 
    for_end) do 
      if ( ! freearr [p ]&& ( ( p < washimin ) || ( p > wasmemend ) || 
      wasfree [p ]) ) 
      {
	printchar ( 32 ) ;
	printint ( p ) ;
      } 
    while ( p++ < for_end ) ;} 
  } 
  {register integer for_end; p = 0 ;for_end = lomemmax ; if ( p <= for_end) 
  do 
    wasfree [p ]= freearr [p ];
  while ( p++ < for_end ) ;} 
  {register integer for_end; p = himemmin ;for_end = memend ; if ( p <= 
  for_end) do 
    wasfree [p ]= freearr [p ];
  while ( p++ < for_end ) ;} 
  wasmemend = memend ;
  waslomax = lomemmax ;
  washimin = himemmin ;
} 
#endif /* TEXMF_DEBUG */
#ifdef TEXMF_DEBUG
void 
#ifdef HAVE_PROTOTYPES
zsearchmem ( halfword p ) 
#else
zsearchmem ( p ) 
  halfword p ;
#endif
{
  integer q  ;
  {register integer for_end; q = 0 ;for_end = lomemmax ; if ( q <= for_end) 
  do 
    {
      if ( mem [q ].hhfield .v.RH == p ) 
      {
	printnl ( 320 ) ;
	printint ( q ) ;
	printchar ( 41 ) ;
      } 
      if ( mem [q ].hhfield .lhfield == p ) 
      {
	printnl ( 321 ) ;
	printint ( q ) ;
	printchar ( 41 ) ;
      } 
    } 
  while ( q++ < for_end ) ;} 
  {register integer for_end; q = himemmin ;for_end = memend ; if ( q <= 
  for_end) do 
    {
      if ( mem [q ].hhfield .v.RH == p ) 
      {
	printnl ( 320 ) ;
	printint ( q ) ;
	printchar ( 41 ) ;
      } 
      if ( mem [q ].hhfield .lhfield == p ) 
      {
	printnl ( 321 ) ;
	printint ( q ) ;
	printchar ( 41 ) ;
      } 
    } 
  while ( q++ < for_end ) ;} 
  {register integer for_end; q = 1 ;for_end = 9769 ; if ( q <= for_end) do 
    {
      if ( eqtb [q ].v.RH == p ) 
      {
	printnl ( 457 ) ;
	printint ( q ) ;
	printchar ( 41 ) ;
      } 
    } 
  while ( q++ < for_end ) ;} 
} 
#endif /* TEXMF_DEBUG */
void 
#ifdef HAVE_PROTOTYPES
zprintop ( quarterword c ) 
#else
zprintop ( c ) 
  quarterword c ;
#endif
{
  if ( c <= 15 ) 
  printtype ( c ) ;
  else switch ( c ) 
  {case 30 : 
    print ( 346 ) ;
    break ;
  case 31 : 
    print ( 347 ) ;
    break ;
  case 32 : 
    print ( 348 ) ;
    break ;
  case 33 : 
    print ( 349 ) ;
    break ;
  case 34 : 
    print ( 350 ) ;
    break ;
  case 35 : 
    print ( 351 ) ;
    break ;
  case 36 : 
    print ( 352 ) ;
    break ;
  case 37 : 
    print ( 353 ) ;
    break ;
  case 38 : 
    print ( 354 ) ;
    break ;
  case 39 : 
    print ( 355 ) ;
    break ;
  case 40 : 
    print ( 356 ) ;
    break ;
  case 41 : 
    print ( 357 ) ;
    break ;
  case 42 : 
    print ( 358 ) ;
    break ;
  case 43 : 
    print ( 359 ) ;
    break ;
  case 44 : 
    print ( 360 ) ;
    break ;
  case 45 : 
    print ( 361 ) ;
    break ;
  case 46 : 
    print ( 362 ) ;
    break ;
  case 47 : 
    print ( 363 ) ;
    break ;
  case 48 : 
    print ( 364 ) ;
    break ;
  case 49 : 
    print ( 365 ) ;
    break ;
  case 50 : 
    print ( 366 ) ;
    break ;
  case 51 : 
    print ( 367 ) ;
    break ;
  case 52 : 
    print ( 368 ) ;
    break ;
  case 53 : 
    print ( 369 ) ;
    break ;
  case 54 : 
    print ( 370 ) ;
    break ;
  case 55 : 
    print ( 371 ) ;
    break ;
  case 56 : 
    print ( 372 ) ;
    break ;
  case 57 : 
    print ( 373 ) ;
    break ;
  case 58 : 
    print ( 374 ) ;
    break ;
  case 59 : 
    print ( 375 ) ;
    break ;
  case 60 : 
    print ( 376 ) ;
    break ;
  case 61 : 
    print ( 377 ) ;
    break ;
  case 62 : 
    print ( 378 ) ;
    break ;
  case 63 : 
    print ( 379 ) ;
    break ;
  case 64 : 
    print ( 380 ) ;
    break ;
  case 65 : 
    print ( 381 ) ;
    break ;
  case 66 : 
    print ( 382 ) ;
    break ;
  case 67 : 
    print ( 383 ) ;
    break ;
  case 68 : 
    print ( 384 ) ;
    break ;
  case 69 : 
    printchar ( 43 ) ;
    break ;
  case 70 : 
    printchar ( 45 ) ;
    break ;
  case 71 : 
    printchar ( 42 ) ;
    break ;
  case 72 : 
    printchar ( 47 ) ;
    break ;
  case 73 : 
    print ( 385 ) ;
    break ;
  case 74 : 
    print ( 309 ) ;
    break ;
  case 75 : 
    print ( 386 ) ;
    break ;
  case 76 : 
    print ( 387 ) ;
    break ;
  case 77 : 
    printchar ( 60 ) ;
    break ;
  case 78 : 
    print ( 388 ) ;
    break ;
  case 79 : 
    printchar ( 62 ) ;
    break ;
  case 80 : 
    print ( 389 ) ;
    break ;
  case 81 : 
    printchar ( 61 ) ;
    break ;
  case 82 : 
    print ( 390 ) ;
    break ;
  case 83 : 
    print ( 38 ) ;
    break ;
  case 84 : 
    print ( 391 ) ;
    break ;
  case 85 : 
    print ( 392 ) ;
    break ;
  case 86 : 
    print ( 393 ) ;
    break ;
  case 87 : 
    print ( 394 ) ;
    break ;
  case 88 : 
    print ( 395 ) ;
    break ;
  case 89 : 
    print ( 396 ) ;
    break ;
  case 90 : 
    print ( 397 ) ;
    break ;
  case 91 : 
    print ( 398 ) ;
    break ;
  case 92 : 
    print ( 399 ) ;
    break ;
  case 94 : 
    print ( 400 ) ;
    break ;
  case 95 : 
    print ( 401 ) ;
    break ;
  case 96 : 
    print ( 402 ) ;
    break ;
  case 97 : 
    print ( 403 ) ;
    break ;
  case 98 : 
    print ( 404 ) ;
    break ;
  case 99 : 
    print ( 405 ) ;
    break ;
  case 100 : 
    print ( 406 ) ;
    break ;
    default: 
    print ( 407 ) ;
    break ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
fixdateandtime ( void ) 
#else
fixdateandtime ( ) 
#endif
{
  dateandtime ( internal [17 ], internal [16 ], internal [15 ], 
  internal [14 ]) ;
  internal [17 ]= internal [17 ]* 65536L ;
  internal [16 ]= internal [16 ]* 65536L ;
  internal [15 ]= internal [15 ]* 65536L ;
  internal [14 ]= internal [14 ]* 65536L ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zidlookup ( integer j , integer l ) 
#else
zidlookup ( j , l ) 
  integer j ;
  integer l ;
#endif
{
  /* 40 */ register halfword Result; integer h  ;
  halfword p  ;
  halfword k  ;
  if ( l == 1 ) 
  {
    p = buffer [j ]+ 1 ;
    hash [p ].v.RH = p - 1 ;
    goto lab40 ;
  } 
  h = buffer [j ];
  {register integer for_end; k = j + 1 ;for_end = j + l - 1 ; if ( k <= 
  for_end) do 
    {
      h = h + h + buffer [k ];
      while ( h >= 7919 ) h = h - 7919 ;
    } 
  while ( k++ < for_end ) ;} 
  p = h + 257 ;
  while ( true ) {
      
    if ( hash [p ].v.RH > 0 ) 
    if ( ( strstart [hash [p ].v.RH + 1 ]- strstart [hash [p ].v.RH ]) 
    == l ) 
    if ( streqbuf ( hash [p ].v.RH , j ) ) 
    goto lab40 ;
    if ( hash [p ].lhfield == 0 ) 
    {
      if ( hash [p ].v.RH > 0 ) 
      {
	do {
	    if ( ( hashused == 257 ) ) 
	  overflow ( 456 , 9500 ) ;
	  decr ( hashused ) ;
	} while ( ! ( hash [hashused ].v.RH == 0 ) ) ;
	hash [p ].lhfield = hashused ;
	p = hashused ;
      } 
      {
	if ( poolptr + l > maxpoolptr ) 
	{
	  if ( poolptr + l > poolsize ) 
	  overflow ( 257 , poolsize - initpoolptr ) ;
	  maxpoolptr = poolptr + l ;
	} 
      } 
      {register integer for_end; k = j ;for_end = j + l - 1 ; if ( k <= 
      for_end) do 
	{
	  strpool [poolptr ]= buffer [k ];
	  incr ( poolptr ) ;
	} 
      while ( k++ < for_end ) ;} 
      hash [p ].v.RH = makestring () ;
      strref [hash [p ].v.RH ]= 127 ;
	;
#ifdef STAT
      incr ( stcount ) ;
#endif /* STAT */
      goto lab40 ;
    } 
    p = hash [p ].lhfield ;
  } 
  lab40: Result = p ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
znewnumtok ( scaled v ) 
#else
znewnumtok ( v ) 
  scaled v ;
#endif
{
  register halfword Result; halfword p  ;
  p = getnode ( 2 ) ;
  mem [p + 1 ].cint = v ;
  mem [p ].hhfield .b0 = 16 ;
  mem [p ].hhfield .b1 = 12 ;
  Result = p ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zflushtokenlist ( halfword p ) 
#else
zflushtokenlist ( p ) 
  halfword p ;
#endif
{
  halfword q  ;
  while ( p != 0 ) {
      
    q = p ;
    p = mem [p ].hhfield .v.RH ;
    if ( q >= himemmin ) 
    {
      mem [q ].hhfield .v.RH = avail ;
      avail = q ;
	;
#ifdef STAT
      decr ( dynused ) ;
#endif /* STAT */
    } 
    else {
	
      switch ( mem [q ].hhfield .b0 ) 
      {case 1 : 
      case 2 : 
      case 16 : 
	;
	break ;
      case 4 : 
	{
	  if ( strref [mem [q + 1 ].cint ]< 127 ) 
	  if ( strref [mem [q + 1 ].cint ]> 1 ) 
	  decr ( strref [mem [q + 1 ].cint ]) ;
	  else flushstring ( mem [q + 1 ].cint ) ;
	} 
	break ;
      case 3 : 
      case 5 : 
      case 7 : 
      case 12 : 
      case 10 : 
      case 6 : 
      case 9 : 
      case 8 : 
      case 11 : 
      case 14 : 
      case 13 : 
      case 17 : 
      case 18 : 
      case 19 : 
	{
	  gpointer = q ;
	  tokenrecycle () ;
	} 
	break ;
	default: 
	confusion ( 490 ) ;
	break ;
      } 
      freenode ( q , 2 ) ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zdeletemacref ( halfword p ) 
#else
zdeletemacref ( p ) 
  halfword p ;
#endif
{
  if ( mem [p ].hhfield .lhfield == 0 ) 
  flushtokenlist ( p ) ;
  else decr ( mem [p ].hhfield .lhfield ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintcmdmod ( integer c , integer m ) 
#else
zprintcmdmod ( c , m ) 
  integer c ;
  integer m ;
#endif
{
  switch ( c ) 
  {case 18 : 
    print ( 461 ) ;
    break ;
  case 77 : 
    print ( 460 ) ;
    break ;
  case 59 : 
    print ( 463 ) ;
    break ;
  case 72 : 
    print ( 462 ) ;
    break ;
  case 79 : 
    print ( 459 ) ;
    break ;
  case 32 : 
    print ( 464 ) ;
    break ;
  case 81 : 
    print ( 58 ) ;
    break ;
  case 82 : 
    print ( 44 ) ;
    break ;
  case 57 : 
    print ( 465 ) ;
    break ;
  case 19 : 
    print ( 466 ) ;
    break ;
  case 60 : 
    print ( 467 ) ;
    break ;
  case 27 : 
    print ( 468 ) ;
    break ;
  case 11 : 
    print ( 469 ) ;
    break ;
  case 80 : 
    print ( 458 ) ;
    break ;
  case 84 : 
    print ( 452 ) ;
    break ;
  case 26 : 
    print ( 470 ) ;
    break ;
  case 6 : 
    print ( 471 ) ;
    break ;
  case 9 : 
    print ( 472 ) ;
    break ;
  case 70 : 
    print ( 473 ) ;
    break ;
  case 73 : 
    print ( 474 ) ;
    break ;
  case 13 : 
    print ( 475 ) ;
    break ;
  case 46 : 
    print ( 123 ) ;
    break ;
  case 63 : 
    print ( 91 ) ;
    break ;
  case 14 : 
    print ( 476 ) ;
    break ;
  case 15 : 
    print ( 477 ) ;
    break ;
  case 69 : 
    print ( 478 ) ;
    break ;
  case 28 : 
    print ( 479 ) ;
    break ;
  case 47 : 
    print ( 407 ) ;
    break ;
  case 24 : 
    print ( 480 ) ;
    break ;
  case 7 : 
    printchar ( 92 ) ;
    break ;
  case 65 : 
    print ( 125 ) ;
    break ;
  case 64 : 
    print ( 93 ) ;
    break ;
  case 12 : 
    print ( 481 ) ;
    break ;
  case 8 : 
    print ( 482 ) ;
    break ;
  case 83 : 
    print ( 59 ) ;
    break ;
  case 17 : 
    print ( 483 ) ;
    break ;
  case 78 : 
    print ( 484 ) ;
    break ;
  case 74 : 
    print ( 485 ) ;
    break ;
  case 35 : 
    print ( 486 ) ;
    break ;
  case 58 : 
    print ( 487 ) ;
    break ;
  case 71 : 
    print ( 488 ) ;
    break ;
  case 75 : 
    print ( 489 ) ;
    break ;
  case 16 : 
    if ( m <= 2 ) 
    if ( m == 1 ) 
    print ( 651 ) ;
    else if ( m < 1 ) 
    print ( 453 ) ;
    else print ( 652 ) ;
    else if ( m == 53 ) 
    print ( 653 ) ;
    else if ( m == 44 ) 
    print ( 654 ) ;
    else print ( 655 ) ;
    break ;
  case 4 : 
    if ( m <= 1 ) 
    if ( m == 1 ) 
    print ( 658 ) ;
    else print ( 454 ) ;
    else if ( m == 9770 ) 
    print ( 656 ) ;
    else print ( 657 ) ;
    break ;
  case 61 : 
    switch ( m ) 
    {case 1 : 
      print ( 660 ) ;
      break ;
    case 2 : 
      printchar ( 64 ) ;
      break ;
    case 3 : 
      print ( 661 ) ;
      break ;
      default: 
      print ( 659 ) ;
      break ;
    } 
    break ;
  case 56 : 
    if ( m >= 9770 ) 
    if ( m == 9770 ) 
    print ( 672 ) ;
    else if ( m == 9920 ) 
    print ( 673 ) ;
    else print ( 674 ) ;
    else if ( m < 2 ) 
    print ( 675 ) ;
    else if ( m == 2 ) 
    print ( 676 ) ;
    else print ( 677 ) ;
    break ;
  case 3 : 
    if ( m == 0 ) 
    print ( 687 ) ;
    else print ( 613 ) ;
    break ;
  case 1 : 
  case 2 : 
    switch ( m ) 
    {case 1 : 
      print ( 714 ) ;
      break ;
    case 2 : 
      print ( 451 ) ;
      break ;
    case 3 : 
      print ( 715 ) ;
      break ;
      default: 
      print ( 716 ) ;
      break ;
    } 
    break ;
  case 33 : 
  case 34 : 
  case 37 : 
  case 55 : 
  case 45 : 
  case 50 : 
  case 36 : 
  case 43 : 
  case 54 : 
  case 48 : 
  case 51 : 
  case 52 : 
    printop ( m ) ;
    break ;
  case 30 : 
    printtype ( m ) ;
    break ;
  case 85 : 
    if ( m == 0 ) 
    print ( 908 ) ;
    else print ( 909 ) ;
    break ;
  case 23 : 
    switch ( m ) 
    {case 0 : 
      print ( 271 ) ;
      break ;
    case 1 : 
      print ( 272 ) ;
      break ;
    case 2 : 
      print ( 273 ) ;
      break ;
      default: 
      print ( 915 ) ;
      break ;
    } 
    break ;
  case 21 : 
    if ( m == 0 ) 
    print ( 916 ) ;
    else print ( 917 ) ;
    break ;
  case 22 : 
    switch ( m ) 
    {case 0 : 
      print ( 931 ) ;
      break ;
    case 1 : 
      print ( 932 ) ;
      break ;
    case 2 : 
      print ( 933 ) ;
      break ;
    case 3 : 
      print ( 934 ) ;
      break ;
      default: 
      print ( 935 ) ;
      break ;
    } 
    break ;
  case 31 : 
  case 62 : 
    {
      if ( c == 31 ) 
      print ( 938 ) ;
      else print ( 939 ) ;
      print ( 940 ) ;
      print ( hash [m ].v.RH ) ;
    } 
    break ;
  case 41 : 
    if ( m == 0 ) 
    print ( 941 ) ;
    else print ( 942 ) ;
    break ;
  case 10 : 
    print ( 943 ) ;
    break ;
  case 53 : 
  case 44 : 
  case 49 : 
    {
      printcmdmod ( 16 , c ) ;
      print ( 944 ) ;
      println () ;
      showtokenlist ( mem [mem [m ].hhfield .v.RH ].hhfield .v.RH , 0 , 
      1000 , 0 ) ;
    } 
    break ;
  case 5 : 
    print ( 945 ) ;
    break ;
  case 40 : 
    print ( intname [m ]) ;
    break ;
  case 68 : 
    if ( m == 1 ) 
    print ( 952 ) ;
    else if ( m == 0 ) 
    print ( 953 ) ;
    else print ( 954 ) ;
    break ;
  case 66 : 
    if ( m == 6 ) 
    print ( 955 ) ;
    else print ( 956 ) ;
    break ;
  case 67 : 
    if ( m == 0 ) 
    print ( 957 ) ;
    else print ( 958 ) ;
    break ;
  case 25 : 
    if ( m < 1 ) 
    print ( 988 ) ;
    else if ( m == 1 ) 
    print ( 989 ) ;
    else print ( 990 ) ;
    break ;
  case 20 : 
    switch ( m ) 
    {case 0 : 
      print ( 1000 ) ;
      break ;
    case 1 : 
      print ( 1001 ) ;
      break ;
    case 2 : 
      print ( 1002 ) ;
      break ;
    case 3 : 
      print ( 1003 ) ;
      break ;
      default: 
      print ( 1004 ) ;
      break ;
    } 
    break ;
  case 76 : 
    switch ( m ) 
    {case 0 : 
      print ( 1022 ) ;
      break ;
    case 1 : 
      print ( 1023 ) ;
      break ;
    case 2 : 
      print ( 1025 ) ;
      break ;
    case 3 : 
      print ( 1027 ) ;
      break ;
    case 5 : 
      print ( 1024 ) ;
      break ;
    case 6 : 
      print ( 1026 ) ;
      break ;
    case 7 : 
      print ( 1028 ) ;
      break ;
    case 11 : 
      print ( 1029 ) ;
      break ;
      default: 
      print ( 1030 ) ;
      break ;
    } 
    break ;
  case 29 : 
    if ( m == 16 ) 
    print ( 1055 ) ;
    else print ( 1054 ) ;
    break ;
    default: 
    print ( 600 ) ;
    break ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zshowmacro ( halfword p , integer q , integer l ) 
#else
zshowmacro ( p , q , l ) 
  halfword p ;
  integer q ;
  integer l ;
#endif
{
  /* 10 */ halfword r  ;
  p = mem [p ].hhfield .v.RH ;
  while ( mem [p ].hhfield .lhfield > 7 ) {
      
    r = mem [p ].hhfield .v.RH ;
    mem [p ].hhfield .v.RH = 0 ;
    showtokenlist ( p , 0 , l , 0 ) ;
    mem [p ].hhfield .v.RH = r ;
    p = r ;
    if ( l > 0 ) 
    l = l - tally ;
    else goto lab10 ;
  } 
  tally = 0 ;
  switch ( mem [p ].hhfield .lhfield ) 
  {case 0 : 
    print ( 500 ) ;
    break ;
  case 1 : 
  case 2 : 
  case 3 : 
    {
      printchar ( 60 ) ;
      printcmdmod ( 56 , mem [p ].hhfield .lhfield ) ;
      print ( 501 ) ;
    } 
    break ;
  case 4 : 
    print ( 502 ) ;
    break ;
  case 5 : 
    print ( 503 ) ;
    break ;
  case 6 : 
    print ( 504 ) ;
    break ;
  case 7 : 
    print ( 505 ) ;
    break ;
  } 
  showtokenlist ( mem [p ].hhfield .v.RH , q , l - tally , 0 ) ;
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zinitbignode ( halfword p ) 
#else
zinitbignode ( p ) 
  halfword p ;
#endif
{
  halfword q  ;
  smallnumber s  ;
  s = bignodesize [mem [p ].hhfield .b0 ];
  q = getnode ( s ) ;
  do {
      s = s - 2 ;
    {
      mem [q + s ].hhfield .b0 = 19 ;
      serialno = serialno + 64 ;
      mem [q + s + 1 ].cint = serialno ;
    } 
    mem [q + s ].hhfield .b1 = halfp ( s ) + 5 ;
    mem [q + s ].hhfield .v.RH = 0 ;
  } while ( ! ( s == 0 ) ) ;
  mem [q ].hhfield .v.RH = p ;
  mem [p + 1 ].cint = q ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
idtransform ( void ) 
#else
idtransform ( ) 
#endif
{
  register halfword Result; halfword p, q, r  ;
  p = getnode ( 2 ) ;
  mem [p ].hhfield .b0 = 13 ;
  mem [p ].hhfield .b1 = 11 ;
  mem [p + 1 ].cint = 0 ;
  initbignode ( p ) ;
  q = mem [p + 1 ].cint ;
  r = q + 12 ;
  do {
      r = r - 2 ;
    mem [r ].hhfield .b0 = 16 ;
    mem [r + 1 ].cint = 0 ;
  } while ( ! ( r == q ) ) ;
  mem [q + 5 ].cint = 65536L ;
  mem [q + 11 ].cint = 65536L ;
  Result = p ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
znewroot ( halfword x ) 
#else
znewroot ( x ) 
  halfword x ;
#endif
{
  halfword p  ;
  p = getnode ( 2 ) ;
  mem [p ].hhfield .b0 = 0 ;
  mem [p ].hhfield .b1 = 0 ;
  mem [p ].hhfield .v.RH = x ;
  eqtb [x ].v.RH = p ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintvariablename ( halfword p ) 
#else
zprintvariablename ( p ) 
  halfword p ;
#endif
{
  /* 40 10 */ halfword q  ;
  halfword r  ;
  while ( mem [p ].hhfield .b1 >= 5 ) {
      
    switch ( mem [p ].hhfield .b1 ) 
    {case 5 : 
      printchar ( 120 ) ;
      break ;
    case 6 : 
      printchar ( 121 ) ;
      break ;
    case 7 : 
      print ( 508 ) ;
      break ;
    case 8 : 
      print ( 509 ) ;
      break ;
    case 9 : 
      print ( 510 ) ;
      break ;
    case 10 : 
      print ( 511 ) ;
      break ;
    case 11 : 
      {
	print ( 512 ) ;
	printint ( p - 0 ) ;
	goto lab10 ;
      } 
      break ;
    } 
    print ( 513 ) ;
    p = mem [p - 2 * ( mem [p ].hhfield .b1 - 5 ) ].hhfield .v.RH ;
  } 
  q = 0 ;
  while ( mem [p ].hhfield .b1 > 1 ) {
      
    if ( mem [p ].hhfield .b1 == 3 ) 
    {
      r = newnumtok ( mem [p + 2 ].cint ) ;
      do {
	  p = mem [p ].hhfield .v.RH ;
      } while ( ! ( mem [p ].hhfield .b1 == 4 ) ) ;
    } 
    else if ( mem [p ].hhfield .b1 == 2 ) 
    {
      p = mem [p ].hhfield .v.RH ;
      goto lab40 ;
    } 
    else {
	
      if ( mem [p ].hhfield .b1 != 4 ) 
      confusion ( 507 ) ;
      r = getavail () ;
      mem [r ].hhfield .lhfield = mem [p + 2 ].hhfield .lhfield ;
    } 
    mem [r ].hhfield .v.RH = q ;
    q = r ;
    lab40: p = mem [p + 2 ].hhfield .v.RH ;
  } 
  r = getavail () ;
  mem [r ].hhfield .lhfield = mem [p ].hhfield .v.RH ;
  mem [r ].hhfield .v.RH = q ;
  if ( mem [p ].hhfield .b1 == 1 ) 
  print ( 506 ) ;
  showtokenlist ( r , 0 , 2147483647L , tally ) ;
  flushtokenlist ( r ) ;
  lab10: ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
zinteresting ( halfword p ) 
#else
zinteresting ( p ) 
  halfword p ;
#endif
{
  register boolean Result; smallnumber t  ;
  if ( internal [3 ]> 0 ) 
  Result = true ;
  else {
      
    t = mem [p ].hhfield .b1 ;
    if ( t >= 5 ) 
    if ( t != 11 ) 
    t = mem [mem [p - 2 * ( t - 5 ) ].hhfield .v.RH ].hhfield .b1 ;
    Result = ( t != 11 ) ;
  } 
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
znewstructure ( halfword p ) 
#else
znewstructure ( p ) 
  halfword p ;
#endif
{
  register halfword Result; halfword q, r  ;
  switch ( mem [p ].hhfield .b1 ) 
  {case 0 : 
    {
      q = mem [p ].hhfield .v.RH ;
      r = getnode ( 2 ) ;
      eqtb [q ].v.RH = r ;
    } 
    break ;
  case 3 : 
    {
      q = p ;
      do {
	  q = mem [q ].hhfield .v.RH ;
      } while ( ! ( mem [q ].hhfield .b1 == 4 ) ) ;
      q = mem [q + 2 ].hhfield .v.RH ;
      r = q + 1 ;
      do {
	  q = r ;
	r = mem [r ].hhfield .v.RH ;
      } while ( ! ( r == p ) ) ;
      r = getnode ( 3 ) ;
      mem [q ].hhfield .v.RH = r ;
      mem [r + 2 ].cint = mem [p + 2 ].cint ;
    } 
    break ;
  case 4 : 
    {
      q = mem [p + 2 ].hhfield .v.RH ;
      r = mem [q + 1 ].hhfield .lhfield ;
      do {
	  q = r ;
	r = mem [r ].hhfield .v.RH ;
      } while ( ! ( r == p ) ) ;
      r = getnode ( 3 ) ;
      mem [q ].hhfield .v.RH = r ;
      mem [r + 2 ]= mem [p + 2 ];
      if ( mem [p + 2 ].hhfield .lhfield == 0 ) 
      {
	q = mem [p + 2 ].hhfield .v.RH + 1 ;
	while ( mem [q ].hhfield .v.RH != p ) q = mem [q ].hhfield .v.RH ;
	mem [q ].hhfield .v.RH = r ;
      } 
    } 
    break ;
    default: 
    confusion ( 514 ) ;
    break ;
  } 
  mem [r ].hhfield .v.RH = mem [p ].hhfield .v.RH ;
  mem [r ].hhfield .b0 = 21 ;
  mem [r ].hhfield .b1 = mem [p ].hhfield .b1 ;
  mem [r + 1 ].hhfield .lhfield = p ;
  mem [p ].hhfield .b1 = 2 ;
  q = getnode ( 3 ) ;
  mem [p ].hhfield .v.RH = q ;
  mem [r + 1 ].hhfield .v.RH = q ;
  mem [q + 2 ].hhfield .v.RH = r ;
  mem [q ].hhfield .b0 = 0 ;
  mem [q ].hhfield .b1 = 4 ;
  mem [q ].hhfield .v.RH = 17 ;
  mem [q + 2 ].hhfield .lhfield = 0 ;
  Result = r ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zfindvariable ( halfword t ) 
#else
zfindvariable ( t ) 
  halfword t ;
#endif
{
  /* 10 */ register halfword Result; halfword p, q, r, s  ;
  halfword pp, qq, rr, ss  ;
  integer n  ;
  memoryword saveword  ;
  p = mem [t ].hhfield .lhfield ;
  t = mem [t ].hhfield .v.RH ;
  if ( eqtb [p ].lhfield % 86 != 41 ) 
  {
    Result = 0 ;
    goto lab10 ;
  } 
  if ( eqtb [p ].v.RH == 0 ) 
  newroot ( p ) ;
  p = eqtb [p ].v.RH ;
  pp = p ;
  while ( t != 0 ) {
      
    if ( mem [pp ].hhfield .b0 != 21 ) 
    {
      if ( mem [pp ].hhfield .b0 > 21 ) 
      {
	Result = 0 ;
	goto lab10 ;
      } 
      ss = newstructure ( pp ) ;
      if ( p == pp ) 
      p = ss ;
      pp = ss ;
    } 
    if ( mem [p ].hhfield .b0 != 21 ) 
    p = newstructure ( p ) ;
    if ( t < himemmin ) 
    {
      n = mem [t + 1 ].cint ;
      pp = mem [mem [pp + 1 ].hhfield .lhfield ].hhfield .v.RH ;
      q = mem [mem [p + 1 ].hhfield .lhfield ].hhfield .v.RH ;
      saveword = mem [q + 2 ];
      mem [q + 2 ].cint = 2147483647L ;
      s = p + 1 ;
      do {
	  r = s ;
	s = mem [s ].hhfield .v.RH ;
      } while ( ! ( n <= mem [s + 2 ].cint ) ) ;
      if ( n == mem [s + 2 ].cint ) 
      p = s ;
      else {
	  
	p = getnode ( 3 ) ;
	mem [r ].hhfield .v.RH = p ;
	mem [p ].hhfield .v.RH = s ;
	mem [p + 2 ].cint = n ;
	mem [p ].hhfield .b1 = 3 ;
	mem [p ].hhfield .b0 = 0 ;
      } 
      mem [q + 2 ]= saveword ;
    } 
    else {
	
      n = mem [t ].hhfield .lhfield ;
      ss = mem [pp + 1 ].hhfield .lhfield ;
      do {
	  rr = ss ;
	ss = mem [ss ].hhfield .v.RH ;
      } while ( ! ( n <= mem [ss + 2 ].hhfield .lhfield ) ) ;
      if ( n < mem [ss + 2 ].hhfield .lhfield ) 
      {
	qq = getnode ( 3 ) ;
	mem [rr ].hhfield .v.RH = qq ;
	mem [qq ].hhfield .v.RH = ss ;
	mem [qq + 2 ].hhfield .lhfield = n ;
	mem [qq ].hhfield .b1 = 4 ;
	mem [qq ].hhfield .b0 = 0 ;
	mem [qq + 2 ].hhfield .v.RH = pp ;
	ss = qq ;
      } 
      if ( p == pp ) 
      {
	p = ss ;
	pp = ss ;
      } 
      else {
	  
	pp = ss ;
	s = mem [p + 1 ].hhfield .lhfield ;
	do {
	    r = s ;
	  s = mem [s ].hhfield .v.RH ;
	} while ( ! ( n <= mem [s + 2 ].hhfield .lhfield ) ) ;
	if ( n == mem [s + 2 ].hhfield .lhfield ) 
	p = s ;
	else {
	    
	  q = getnode ( 3 ) ;
	  mem [r ].hhfield .v.RH = q ;
	  mem [q ].hhfield .v.RH = s ;
	  mem [q + 2 ].hhfield .lhfield = n ;
	  mem [q ].hhfield .b1 = 4 ;
	  mem [q ].hhfield .b0 = 0 ;
	  mem [q + 2 ].hhfield .v.RH = p ;
	  p = q ;
	} 
      } 
    } 
    t = mem [t ].hhfield .v.RH ;
  } 
  if ( mem [pp ].hhfield .b0 >= 21 ) 
  if ( mem [pp ].hhfield .b0 == 21 ) 
  pp = mem [pp + 1 ].hhfield .lhfield ;
  else {
      
    Result = 0 ;
    goto lab10 ;
  } 
  if ( mem [p ].hhfield .b0 == 21 ) 
  p = mem [p + 1 ].hhfield .lhfield ;
  if ( mem [p ].hhfield .b0 == 0 ) 
  {
    if ( mem [pp ].hhfield .b0 == 0 ) 
    {
      mem [pp ].hhfield .b0 = 15 ;
      mem [pp + 1 ].cint = 0 ;
    } 
    mem [p ].hhfield .b0 = mem [pp ].hhfield .b0 ;
    mem [p + 1 ].cint = 0 ;
  } 
  Result = p ;
  lab10: ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintpath ( halfword h , strnumber s , boolean nuline ) 
#else
zprintpath ( h , s , nuline ) 
  halfword h ;
  strnumber s ;
  boolean nuline ;
#endif
{
  /* 30 31 */ halfword p, q  ;
  printdiagnostic ( 516 , s , nuline ) ;
  println () ;
  p = h ;
  do {
      q = mem [p ].hhfield .v.RH ;
    if ( ( p == 0 ) || ( q == 0 ) ) 
    {
      printnl ( 259 ) ;
      goto lab30 ;
    } 
    printtwo ( mem [p + 1 ].cint , mem [p + 2 ].cint ) ;
    switch ( mem [p ].hhfield .b1 ) 
    {case 0 : 
      {
	if ( mem [p ].hhfield .b0 == 4 ) 
	print ( 517 ) ;
	if ( ( mem [q ].hhfield .b0 != 0 ) || ( q != h ) ) 
	q = 0 ;
	goto lab31 ;
      } 
      break ;
    case 1 : 
      {
	print ( 523 ) ;
	printtwo ( mem [p + 5 ].cint , mem [p + 6 ].cint ) ;
	print ( 522 ) ;
	if ( mem [q ].hhfield .b0 != 1 ) 
	print ( 524 ) ;
	else printtwo ( mem [q + 3 ].cint , mem [q + 4 ].cint ) ;
	goto lab31 ;
      } 
      break ;
    case 4 : 
      if ( ( mem [p ].hhfield .b0 != 1 ) && ( mem [p ].hhfield .b0 != 4 ) 
      ) 
      print ( 517 ) ;
      break ;
    case 3 : 
    case 2 : 
      {
	if ( mem [p ].hhfield .b0 == 4 ) 
	print ( 524 ) ;
	if ( mem [p ].hhfield .b1 == 3 ) 
	{
	  print ( 520 ) ;
	  printscaled ( mem [p + 5 ].cint ) ;
	} 
	else {
	    
	  nsincos ( mem [p + 5 ].cint ) ;
	  printchar ( 123 ) ;
	  printscaled ( ncos ) ;
	  printchar ( 44 ) ;
	  printscaled ( nsin ) ;
	} 
	printchar ( 125 ) ;
      } 
      break ;
      default: 
      print ( 259 ) ;
      break ;
    } 
    if ( mem [q ].hhfield .b0 <= 1 ) 
    print ( 518 ) ;
    else if ( ( mem [p + 6 ].cint != 65536L ) || ( mem [q + 4 ].cint != 
    65536L ) ) 
    {
      print ( 521 ) ;
      if ( mem [p + 6 ].cint < 0 ) 
      print ( 463 ) ;
      printscaled ( abs ( mem [p + 6 ].cint ) ) ;
      if ( mem [p + 6 ].cint != mem [q + 4 ].cint ) 
      {
	print ( 522 ) ;
	if ( mem [q + 4 ].cint < 0 ) 
	print ( 463 ) ;
	printscaled ( abs ( mem [q + 4 ].cint ) ) ;
      } 
    } 
    lab31: ;
    p = q ;
    if ( ( p != h ) || ( mem [h ].hhfield .b0 != 0 ) ) 
    {
      printnl ( 519 ) ;
      if ( mem [p ].hhfield .b0 == 2 ) 
      {
	nsincos ( mem [p + 3 ].cint ) ;
	printchar ( 123 ) ;
	printscaled ( ncos ) ;
	printchar ( 44 ) ;
	printscaled ( nsin ) ;
	printchar ( 125 ) ;
      } 
      else if ( mem [p ].hhfield .b0 == 3 ) 
      {
	print ( 520 ) ;
	printscaled ( mem [p + 3 ].cint ) ;
	printchar ( 125 ) ;
      } 
    } 
  } while ( ! ( p == h ) ) ;
  if ( mem [h ].hhfield .b0 != 0 ) 
  print ( 384 ) ;
  lab30: enddiagnostic ( true ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintweight ( halfword q , integer xoff ) 
#else
zprintweight ( q , xoff ) 
  halfword q ;
  integer xoff ;
#endif
{
  integer w, m  ;
  integer d  ;
  d = mem [q ].hhfield .lhfield ;
  w = d % 8 ;
  m = ( d / 8 ) - mem [curedges + 3 ].hhfield .lhfield ;
  if ( fileoffset > maxprintline - 9 ) 
  printnl ( 32 ) ;
  else printchar ( 32 ) ;
  printint ( m + xoff ) ;
  while ( w > 4 ) {
      
    printchar ( 43 ) ;
    decr ( w ) ;
  } 
  while ( w < 4 ) {
      
    printchar ( 45 ) ;
    incr ( w ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zprintedges ( strnumber s , boolean nuline , integer xoff , integer yoff ) 
#else
zprintedges ( s , nuline , xoff , yoff ) 
  strnumber s ;
  boolean nuline ;
  integer xoff ;
  integer yoff ;
#endif
{
  halfword p, q, r  ;
  integer n  ;
  printdiagnostic ( 531 , s , nuline ) ;
  p = mem [curedges ].hhfield .lhfield ;
  n = mem [curedges + 1 ].hhfield .v.RH - 4096 ;
  while ( p != curedges ) {
      
    q = mem [p + 1 ].hhfield .lhfield ;
    r = mem [p + 1 ].hhfield .v.RH ;
    if ( ( q > 1 ) || ( r != memtop ) ) 
    {
      printnl ( 532 ) ;
      printint ( n + yoff ) ;
      printchar ( 58 ) ;
      while ( q > 1 ) {
	  
	printweight ( q , xoff ) ;
	q = mem [q ].hhfield .v.RH ;
      } 
      print ( 533 ) ;
      while ( r != memtop ) {
	  
	printweight ( r , xoff ) ;
	r = mem [r ].hhfield .v.RH ;
      } 
    } 
    p = mem [p ].hhfield .lhfield ;
    decr ( n ) ;
  } 
  enddiagnostic ( true ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zunskew ( scaled x , scaled y , smallnumber octant ) 
#else
zunskew ( x , y , octant ) 
  scaled x ;
  scaled y ;
  smallnumber octant ;
#endif
{
  switch ( octant ) 
  {case 1 : 
    {
      curx = x + y ;
      cury = y ;
    } 
    break ;
  case 5 : 
    {
      curx = y ;
      cury = x + y ;
    } 
    break ;
  case 6 : 
    {
      curx = - (integer) y ;
      cury = x + y ;
    } 
    break ;
  case 2 : 
    {
      curx = - (integer) x - y ;
      cury = y ;
    } 
    break ;
  case 4 : 
    {
      curx = - (integer) x - y ;
      cury = - (integer) y ;
    } 
    break ;
  case 8 : 
    {
      curx = - (integer) y ;
      cury = - (integer) x - y ;
    } 
    break ;
  case 7 : 
    {
      curx = y ;
      cury = - (integer) x - y ;
    } 
    break ;
  case 3 : 
    {
      curx = x + y ;
      cury = - (integer) y ;
    } 
    break ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zprintpen ( halfword p , strnumber s , boolean nuline ) 
#else
zprintpen ( p , s , nuline ) 
  halfword p ;
  strnumber s ;
  boolean nuline ;
#endif
{
  boolean nothingprinted  ;
  char k  ;
  halfword h  ;
  integer m, n  ;
  halfword w, ww  ;
  printdiagnostic ( 568 , s , nuline ) ;
  nothingprinted = true ;
  println () ;
  {register integer for_end; k = 1 ;for_end = 8 ; if ( k <= for_end) do 
    {
      octant = octantcode [k ];
      h = p + octant ;
      n = mem [h ].hhfield .lhfield ;
      w = mem [h ].hhfield .v.RH ;
      if ( ! odd ( k ) ) 
      w = mem [w ].hhfield .lhfield ;
      {register integer for_end; m = 1 ;for_end = n + 1 ; if ( m <= for_end) 
      do 
	{
	  if ( odd ( k ) ) 
	  ww = mem [w ].hhfield .v.RH ;
	  else ww = mem [w ].hhfield .lhfield ;
	  if ( ( mem [ww + 1 ].cint != mem [w + 1 ].cint ) || ( mem [ww + 
	  2 ].cint != mem [w + 2 ].cint ) ) 
	  {
	    if ( nothingprinted ) 
	    nothingprinted = false ;
	    else printnl ( 570 ) ;
	    unskew ( mem [ww + 1 ].cint , mem [ww + 2 ].cint , octant ) ;
	    printtwo ( curx , cury ) ;
	  } 
	  w = ww ;
	} 
      while ( m++ < for_end ) ;} 
    } 
  while ( k++ < for_end ) ;} 
  if ( nothingprinted ) 
  {
    w = mem [p + 1 ].hhfield .v.RH ;
    printtwo ( mem [w + 1 ].cint + mem [w + 2 ].cint , mem [w + 2 ].cint 
    ) ;
  } 
  printnl ( 569 ) ;
  enddiagnostic ( true ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintdependency ( halfword p , smallnumber t ) 
#else
zprintdependency ( p , t ) 
  halfword p ;
  smallnumber t ;
#endif
{
  /* 10 */ integer v  ;
  halfword pp, q  ;
  pp = p ;
  while ( true ) {
      
    v = abs ( mem [p + 1 ].cint ) ;
    q = mem [p ].hhfield .lhfield ;
    if ( q == 0 ) 
    {
      if ( ( v != 0 ) || ( p == pp ) ) 
      {
	if ( mem [p + 1 ].cint > 0 ) 
	if ( p != pp ) 
	printchar ( 43 ) ;
	printscaled ( mem [p + 1 ].cint ) ;
      } 
      goto lab10 ;
    } 
    if ( mem [p + 1 ].cint < 0 ) 
    printchar ( 45 ) ;
    else if ( p != pp ) 
    printchar ( 43 ) ;
    if ( t == 17 ) 
    v = roundfraction ( v ) ;
    if ( v != 65536L ) 
    printscaled ( v ) ;
    if ( mem [q ].hhfield .b0 != 19 ) 
    confusion ( 586 ) ;
    printvariablename ( q ) ;
    v = mem [q + 1 ].cint % 64 ;
    while ( v > 0 ) {
	
      print ( 587 ) ;
      v = v - 2 ;
    } 
    p = mem [p ].hhfield .v.RH ;
  } 
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintdp ( smallnumber t , halfword p , smallnumber verbosity ) 
#else
zprintdp ( t , p , verbosity ) 
  smallnumber t ;
  halfword p ;
  smallnumber verbosity ;
#endif
{
  halfword q  ;
  q = mem [p ].hhfield .v.RH ;
  if ( ( mem [q ].hhfield .lhfield == 0 ) || ( verbosity > 0 ) ) 
  printdependency ( p , t ) ;
  else print ( 760 ) ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
stashcurexp ( void ) 
#else
stashcurexp ( ) 
#endif
{
  register halfword Result; halfword p  ;
  switch ( curtype ) 
  {case 3 : 
  case 5 : 
  case 7 : 
  case 12 : 
  case 10 : 
  case 13 : 
  case 14 : 
  case 17 : 
  case 18 : 
  case 19 : 
    p = curexp ;
    break ;
    default: 
    {
      p = getnode ( 2 ) ;
      mem [p ].hhfield .b1 = 11 ;
      mem [p ].hhfield .b0 = curtype ;
      mem [p + 1 ].cint = curexp ;
    } 
    break ;
  } 
  curtype = 1 ;
  mem [p ].hhfield .v.RH = 1 ;
  Result = p ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zunstashcurexp ( halfword p ) 
#else
zunstashcurexp ( p ) 
  halfword p ;
#endif
{
  curtype = mem [p ].hhfield .b0 ;
  switch ( curtype ) 
  {case 3 : 
  case 5 : 
  case 7 : 
  case 12 : 
  case 10 : 
  case 13 : 
  case 14 : 
  case 17 : 
  case 18 : 
  case 19 : 
    curexp = p ;
    break ;
    default: 
    {
      curexp = mem [p + 1 ].cint ;
      freenode ( p , 2 ) ;
    } 
    break ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zprintexp ( halfword p , smallnumber verbosity ) 
#else
zprintexp ( p , verbosity ) 
  halfword p ;
  smallnumber verbosity ;
#endif
{
  boolean restorecurexp  ;
  smallnumber t  ;
  integer v  ;
  halfword q  ;
  if ( p != 0 ) 
  restorecurexp = false ;
  else {
      
    p = stashcurexp () ;
    restorecurexp = true ;
  } 
  t = mem [p ].hhfield .b0 ;
  if ( t < 17 ) 
  v = mem [p + 1 ].cint ;
  else if ( t < 19 ) 
  v = mem [p + 1 ].hhfield .v.RH ;
  switch ( t ) 
  {case 1 : 
    print ( 322 ) ;
    break ;
  case 2 : 
    if ( v == 30 ) 
    print ( 346 ) ;
    else print ( 347 ) ;
    break ;
  case 3 : 
  case 5 : 
  case 7 : 
  case 12 : 
  case 10 : 
  case 15 : 
    {
      printtype ( t ) ;
      if ( v != 0 ) 
      {
	printchar ( 32 ) ;
	while ( ( mem [v ].hhfield .b1 == 11 ) && ( v != p ) ) v = mem [v + 
	1 ].cint ;
	printvariablename ( v ) ;
      } 
    } 
    break ;
  case 4 : 
    {
      printchar ( 34 ) ;
      print ( v ) ;
      printchar ( 34 ) ;
    } 
    break ;
  case 6 : 
  case 8 : 
  case 9 : 
  case 11 : 
    if ( verbosity <= 1 ) 
    printtype ( t ) ;
    else {
	
      if ( selector == 3 ) 
      if ( internal [13 ]<= 0 ) 
      {
	selector = 1 ;
	printtype ( t ) ;
	print ( 758 ) ;
	selector = 3 ;
      } 
      switch ( t ) 
      {case 6 : 
	printpen ( v , 283 , false ) ;
	break ;
      case 8 : 
	printpath ( v , 759 , false ) ;
	break ;
      case 9 : 
	printpath ( v , 283 , false ) ;
	break ;
      case 11 : 
	{
	  curedges = v ;
	  printedges ( 283 , false , 0 , 0 ) ;
	} 
	break ;
      } 
    } 
    break ;
  case 13 : 
  case 14 : 
    if ( v == 0 ) 
    printtype ( t ) ;
    else {
	
      printchar ( 40 ) ;
      q = v + bignodesize [t ];
      do {
	  if ( mem [v ].hhfield .b0 == 16 ) 
	printscaled ( mem [v + 1 ].cint ) ;
	else if ( mem [v ].hhfield .b0 == 19 ) 
	printvariablename ( v ) ;
	else printdp ( mem [v ].hhfield .b0 , mem [v + 1 ].hhfield .v.RH , 
	verbosity ) ;
	v = v + 2 ;
	if ( v != q ) 
	printchar ( 44 ) ;
      } while ( ! ( v == q ) ) ;
      printchar ( 41 ) ;
    } 
    break ;
  case 16 : 
    printscaled ( v ) ;
    break ;
  case 17 : 
  case 18 : 
    printdp ( t , v , verbosity ) ;
    break ;
  case 19 : 
    printvariablename ( p ) ;
    break ;
    default: 
    confusion ( 757 ) ;
    break ;
  } 
  if ( restorecurexp ) 
  unstashcurexp ( p ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdisperr ( halfword p , strnumber s ) 
#else
zdisperr ( p , s ) 
  halfword p ;
  strnumber s ;
#endif
{
  if ( interaction == 3 ) 
  ;
  printnl ( 761 ) ;
  printexp ( p , 1 ) ;
  if ( s != 283 ) 
  {
    printnl ( 261 ) ;
    print ( s ) ;
  } 
} 
halfword 
#ifdef HAVE_PROTOTYPES
zpplusfq ( halfword p , integer f , halfword q , smallnumber t , smallnumber 
tt ) 
#else
zpplusfq ( p , f , q , t , tt ) 
  halfword p ;
  integer f ;
  halfword q ;
  smallnumber t ;
  smallnumber tt ;
#endif
{
  /* 30 */ register halfword Result; halfword pp, qq  ;
  halfword r, s  ;
  integer threshold  ;
  integer v  ;
  if ( t == 17 ) 
  threshold = 2685 ;
  else threshold = 8 ;
  r = memtop - 1 ;
  pp = mem [p ].hhfield .lhfield ;
  qq = mem [q ].hhfield .lhfield ;
  while ( true ) if ( pp == qq ) 
  if ( pp == 0 ) 
  goto lab30 ;
  else {
      
    if ( tt == 17 ) 
    v = mem [p + 1 ].cint + takefraction ( f , mem [q + 1 ].cint ) ;
    else v = mem [p + 1 ].cint + takescaled ( f , mem [q + 1 ].cint ) ;
    mem [p + 1 ].cint = v ;
    s = p ;
    p = mem [p ].hhfield .v.RH ;
    if ( abs ( v ) < threshold ) 
    freenode ( s , 2 ) ;
    else {
	
      if ( abs ( v ) >= 626349397L ) 
      if ( watchcoefs ) 
      {
	mem [qq ].hhfield .b0 = 0 ;
	fixneeded = true ;
      } 
      mem [r ].hhfield .v.RH = s ;
      r = s ;
    } 
    pp = mem [p ].hhfield .lhfield ;
    q = mem [q ].hhfield .v.RH ;
    qq = mem [q ].hhfield .lhfield ;
  } 
  else if ( mem [pp + 1 ].cint < mem [qq + 1 ].cint ) 
  {
    if ( tt == 17 ) 
    v = takefraction ( f , mem [q + 1 ].cint ) ;
    else v = takescaled ( f , mem [q + 1 ].cint ) ;
    if ( abs ( v ) > halfp ( threshold ) ) 
    {
      s = getnode ( 2 ) ;
      mem [s ].hhfield .lhfield = qq ;
      mem [s + 1 ].cint = v ;
      if ( abs ( v ) >= 626349397L ) 
      if ( watchcoefs ) 
      {
	mem [qq ].hhfield .b0 = 0 ;
	fixneeded = true ;
      } 
      mem [r ].hhfield .v.RH = s ;
      r = s ;
    } 
    q = mem [q ].hhfield .v.RH ;
    qq = mem [q ].hhfield .lhfield ;
  } 
  else {
      
    mem [r ].hhfield .v.RH = p ;
    r = p ;
    p = mem [p ].hhfield .v.RH ;
    pp = mem [p ].hhfield .lhfield ;
  } 
  lab30: if ( t == 17 ) 
  mem [p + 1 ].cint = slowadd ( mem [p + 1 ].cint , takefraction ( mem [q 
  + 1 ].cint , f ) ) ;
  else mem [p + 1 ].cint = slowadd ( mem [p + 1 ].cint , takescaled ( mem 
  [q + 1 ].cint , f ) ) ;
  mem [r ].hhfield .v.RH = p ;
  depfinal = p ;
  Result = mem [memtop - 1 ].hhfield .v.RH ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zpoverv ( halfword p , scaled v , smallnumber t0 , smallnumber t1 ) 
#else
zpoverv ( p , v , t0 , t1 ) 
  halfword p ;
  scaled v ;
  smallnumber t0 ;
  smallnumber t1 ;
#endif
{
  register halfword Result; halfword r, s  ;
  integer w  ;
  integer threshold  ;
  boolean scalingdown  ;
  if ( t0 != t1 ) 
  scalingdown = true ;
  else scalingdown = false ;
  if ( t1 == 17 ) 
  threshold = 1342 ;
  else threshold = 4 ;
  r = memtop - 1 ;
  while ( mem [p ].hhfield .lhfield != 0 ) {
      
    if ( scalingdown ) 
    if ( abs ( v ) < 524288L ) 
    w = makescaled ( mem [p + 1 ].cint , v * 4096 ) ;
    else w = makescaled ( roundfraction ( mem [p + 1 ].cint ) , v ) ;
    else w = makescaled ( mem [p + 1 ].cint , v ) ;
    if ( abs ( w ) <= threshold ) 
    {
      s = mem [p ].hhfield .v.RH ;
      freenode ( p , 2 ) ;
      p = s ;
    } 
    else {
	
      if ( abs ( w ) >= 626349397L ) 
      {
	fixneeded = true ;
	mem [mem [p ].hhfield .lhfield ].hhfield .b0 = 0 ;
      } 
      mem [r ].hhfield .v.RH = p ;
      r = p ;
      mem [p + 1 ].cint = w ;
      p = mem [p ].hhfield .v.RH ;
    } 
  } 
  mem [r ].hhfield .v.RH = p ;
  mem [p + 1 ].cint = makescaled ( mem [p + 1 ].cint , v ) ;
  Result = mem [memtop - 1 ].hhfield .v.RH ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zvaltoobig ( scaled x ) 
#else
zvaltoobig ( x ) 
  scaled x ;
#endif
{
  if ( internal [40 ]> 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 261 ) ;
      print ( 588 ) ;
    } 
    printscaled ( x ) ;
    printchar ( 41 ) ;
    {
      helpptr = 4 ;
      helpline [3 ]= 589 ;
      helpline [2 ]= 590 ;
      helpline [1 ]= 591 ;
      helpline [0 ]= 592 ;
    } 
    error () ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zmakeknown ( halfword p , halfword q ) 
#else
zmakeknown ( p , q ) 
  halfword p ;
  halfword q ;
#endif
{
  char t  ;
  mem [mem [q ].hhfield .v.RH + 1 ].hhfield .lhfield = mem [p + 1 ]
  .hhfield .lhfield ;
  mem [mem [p + 1 ].hhfield .lhfield ].hhfield .v.RH = mem [q ].hhfield 
  .v.RH ;
  t = mem [p ].hhfield .b0 ;
  mem [p ].hhfield .b0 = 16 ;
  mem [p + 1 ].cint = mem [q + 1 ].cint ;
  freenode ( q , 2 ) ;
  if ( abs ( mem [p + 1 ].cint ) >= 268435456L ) 
  valtoobig ( mem [p + 1 ].cint ) ;
  if ( internal [2 ]> 0 ) 
  if ( interesting ( p ) ) 
  {
    begindiagnostic () ;
    printnl ( 593 ) ;
    printvariablename ( p ) ;
    printchar ( 61 ) ;
    printscaled ( mem [p + 1 ].cint ) ;
    enddiagnostic ( false ) ;
  } 
  if ( curexp == p ) 
  if ( curtype == t ) 
  {
    curtype = 16 ;
    curexp = mem [p + 1 ].cint ;
    freenode ( p , 2 ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
fixdependencies ( void ) 
#else
fixdependencies ( ) 
#endif
{
  /* 30 */ halfword p, q, r, s, t  ;
  halfword x  ;
  r = mem [13 ].hhfield .v.RH ;
  s = 0 ;
  while ( r != 13 ) {
      
    t = r ;
    r = t + 1 ;
    while ( true ) {
	
      q = mem [r ].hhfield .v.RH ;
      x = mem [q ].hhfield .lhfield ;
      if ( x == 0 ) 
      goto lab30 ;
      if ( mem [x ].hhfield .b0 <= 1 ) 
      {
	if ( mem [x ].hhfield .b0 < 1 ) 
	{
	  p = getavail () ;
	  mem [p ].hhfield .v.RH = s ;
	  s = p ;
	  mem [s ].hhfield .lhfield = x ;
	  mem [x ].hhfield .b0 = 1 ;
	} 
	mem [q + 1 ].cint = mem [q + 1 ].cint / 4 ;
	if ( mem [q + 1 ].cint == 0 ) 
	{
	  mem [r ].hhfield .v.RH = mem [q ].hhfield .v.RH ;
	  freenode ( q , 2 ) ;
	  q = r ;
	} 
      } 
      r = q ;
    } 
    lab30: ;
    r = mem [q ].hhfield .v.RH ;
    if ( q == mem [t + 1 ].hhfield .v.RH ) 
    makeknown ( t , q ) ;
  } 
  while ( s != 0 ) {
      
    p = mem [s ].hhfield .v.RH ;
    x = mem [s ].hhfield .lhfield ;
    {
      mem [s ].hhfield .v.RH = avail ;
      avail = s ;
	;
#ifdef STAT
      decr ( dynused ) ;
#endif /* STAT */
    } 
    s = p ;
    mem [x ].hhfield .b0 = 19 ;
    mem [x + 1 ].cint = mem [x + 1 ].cint + 2 ;
  } 
  fixneeded = false ;
} 
void 
#ifdef HAVE_PROTOTYPES
ztossknotlist ( halfword p ) 
#else
ztossknotlist ( p ) 
  halfword p ;
#endif
{
  halfword q  ;
  halfword r  ;
  q = p ;
  do {
      r = mem [q ].hhfield .v.RH ;
    freenode ( q , 7 ) ;
    q = r ;
  } while ( ! ( q == p ) ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
ztossedges ( halfword h ) 
#else
ztossedges ( h ) 
  halfword h ;
#endif
{
  halfword p, q  ;
  q = mem [h ].hhfield .v.RH ;
  while ( q != h ) {
      
    flushlist ( mem [q + 1 ].hhfield .v.RH ) ;
    if ( mem [q + 1 ].hhfield .lhfield > 1 ) 
    flushlist ( mem [q + 1 ].hhfield .lhfield ) ;
    p = q ;
    q = mem [q ].hhfield .v.RH ;
    freenode ( p , 2 ) ;
  } 
  freenode ( h , 6 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
ztosspen ( halfword p ) 
#else
ztosspen ( p ) 
  halfword p ;
#endif
{
  char k  ;
  halfword w, ww  ;
  if ( p != 3 ) 
  {
    {register integer for_end; k = 1 ;for_end = 8 ; if ( k <= for_end) do 
      {
	w = mem [p + k ].hhfield .v.RH ;
	do {
	    ww = mem [w ].hhfield .v.RH ;
	  freenode ( w , 3 ) ;
	  w = ww ;
	} while ( ! ( w == mem [p + k ].hhfield .v.RH ) ) ;
      } 
    while ( k++ < for_end ) ;} 
    freenode ( p , 10 ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zringdelete ( halfword p ) 
#else
zringdelete ( p ) 
  halfword p ;
#endif
{
  halfword q  ;
  q = mem [p + 1 ].cint ;
  if ( q != 0 ) 
  if ( q != p ) 
  {
    while ( mem [q + 1 ].cint != p ) q = mem [q + 1 ].cint ;
    mem [q + 1 ].cint = mem [p + 1 ].cint ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zrecyclevalue ( halfword p ) 
#else
zrecyclevalue ( p ) 
  halfword p ;
#endif
{
  /* 30 */ smallnumber t  ;
  integer v  ;
  integer vv  ;
  halfword q, r, s, pp  ;
  t = mem [p ].hhfield .b0 ;
  if ( t < 17 ) 
  v = mem [p + 1 ].cint ;
  switch ( t ) 
  {case 0 : 
  case 1 : 
  case 2 : 
  case 16 : 
  case 15 : 
    ;
    break ;
  case 3 : 
  case 5 : 
  case 7 : 
  case 12 : 
  case 10 : 
    ringdelete ( p ) ;
    break ;
  case 4 : 
    {
      if ( strref [v ]< 127 ) 
      if ( strref [v ]> 1 ) 
      decr ( strref [v ]) ;
      else flushstring ( v ) ;
    } 
    break ;
  case 6 : 
    if ( mem [v ].hhfield .lhfield == 0 ) 
    tosspen ( v ) ;
    else decr ( mem [v ].hhfield .lhfield ) ;
    break ;
  case 9 : 
  case 8 : 
    tossknotlist ( v ) ;
    break ;
  case 11 : 
    tossedges ( v ) ;
    break ;
  case 14 : 
  case 13 : 
    if ( v != 0 ) 
    {
      q = v + bignodesize [t ];
      do {
	  q = q - 2 ;
	recyclevalue ( q ) ;
      } while ( ! ( q == v ) ) ;
      freenode ( v , bignodesize [t ]) ;
    } 
    break ;
  case 17 : 
  case 18 : 
    {
      q = mem [p + 1 ].hhfield .v.RH ;
      while ( mem [q ].hhfield .lhfield != 0 ) q = mem [q ].hhfield .v.RH 
      ;
      mem [mem [p + 1 ].hhfield .lhfield ].hhfield .v.RH = mem [q ]
      .hhfield .v.RH ;
      mem [mem [q ].hhfield .v.RH + 1 ].hhfield .lhfield = mem [p + 1 ]
      .hhfield .lhfield ;
      mem [q ].hhfield .v.RH = 0 ;
      flushnodelist ( mem [p + 1 ].hhfield .v.RH ) ;
    } 
    break ;
  case 19 : 
    {
      maxc [17 ]= 0 ;
      maxc [18 ]= 0 ;
      maxlink [17 ]= 0 ;
      maxlink [18 ]= 0 ;
      q = mem [13 ].hhfield .v.RH ;
      while ( q != 13 ) {
	  
	s = q + 1 ;
	while ( true ) {
	    
	  r = mem [s ].hhfield .v.RH ;
	  if ( mem [r ].hhfield .lhfield == 0 ) 
	  goto lab30 ;
	  if ( mem [r ].hhfield .lhfield != p ) 
	  s = r ;
	  else {
	      
	    t = mem [q ].hhfield .b0 ;
	    mem [s ].hhfield .v.RH = mem [r ].hhfield .v.RH ;
	    mem [r ].hhfield .lhfield = q ;
	    if ( abs ( mem [r + 1 ].cint ) > maxc [t ]) 
	    {
	      if ( maxc [t ]> 0 ) 
	      {
		mem [maxptr [t ]].hhfield .v.RH = maxlink [t ];
		maxlink [t ]= maxptr [t ];
	      } 
	      maxc [t ]= abs ( mem [r + 1 ].cint ) ;
	      maxptr [t ]= r ;
	    } 
	    else {
		
	      mem [r ].hhfield .v.RH = maxlink [t ];
	      maxlink [t ]= r ;
	    } 
	  } 
	} 
	lab30: q = mem [r ].hhfield .v.RH ;
      } 
      if ( ( maxc [17 ]> 0 ) || ( maxc [18 ]> 0 ) ) 
      {
	if ( ( maxc [17 ]/ 4096 >= maxc [18 ]) ) 
	t = 17 ;
	else t = 18 ;
	s = maxptr [t ];
	pp = mem [s ].hhfield .lhfield ;
	v = mem [s + 1 ].cint ;
	if ( t == 17 ) 
	mem [s + 1 ].cint = -268435456L ;
	else mem [s + 1 ].cint = -65536L ;
	r = mem [pp + 1 ].hhfield .v.RH ;
	mem [s ].hhfield .v.RH = r ;
	while ( mem [r ].hhfield .lhfield != 0 ) r = mem [r ].hhfield 
	.v.RH ;
	q = mem [r ].hhfield .v.RH ;
	mem [r ].hhfield .v.RH = 0 ;
	mem [q + 1 ].hhfield .lhfield = mem [pp + 1 ].hhfield .lhfield ;
	mem [mem [pp + 1 ].hhfield .lhfield ].hhfield .v.RH = q ;
	{
	  mem [pp ].hhfield .b0 = 19 ;
	  serialno = serialno + 64 ;
	  mem [pp + 1 ].cint = serialno ;
	} 
	if ( curexp == pp ) 
	if ( curtype == t ) 
	curtype = 19 ;
	if ( internal [2 ]> 0 ) 
	if ( interesting ( p ) ) 
	{
	  begindiagnostic () ;
	  printnl ( 763 ) ;
	  if ( v > 0 ) 
	  printchar ( 45 ) ;
	  if ( t == 17 ) 
	  vv = roundfraction ( maxc [17 ]) ;
	  else vv = maxc [18 ];
	  if ( vv != 65536L ) 
	  printscaled ( vv ) ;
	  printvariablename ( p ) ;
	  while ( mem [p + 1 ].cint % 64 > 0 ) {
	      
	    print ( 587 ) ;
	    mem [p + 1 ].cint = mem [p + 1 ].cint - 2 ;
	  } 
	  if ( t == 17 ) 
	  printchar ( 61 ) ;
	  else print ( 764 ) ;
	  printdependency ( s , t ) ;
	  enddiagnostic ( false ) ;
	} 
	t = 35 - t ;
	if ( maxc [t ]> 0 ) 
	{
	  mem [maxptr [t ]].hhfield .v.RH = maxlink [t ];
	  maxlink [t ]= maxptr [t ];
	} 
	if ( t != 17 ) 
	{register integer for_end; t = 17 ;for_end = 18 ; if ( t <= for_end) 
	do 
	  {
	    r = maxlink [t ];
	    while ( r != 0 ) {
		
	      q = mem [r ].hhfield .lhfield ;
	      mem [q + 1 ].hhfield .v.RH = pplusfq ( mem [q + 1 ].hhfield 
	      .v.RH , makefraction ( mem [r + 1 ].cint , - (integer) v ) , s 
	      , t , 17 ) ;
	      if ( mem [q + 1 ].hhfield .v.RH == depfinal ) 
	      makeknown ( q , depfinal ) ;
	      q = r ;
	      r = mem [r ].hhfield .v.RH ;
	      freenode ( q , 2 ) ;
	    } 
	  } 
	while ( t++ < for_end ) ;} 
	else {
	    register integer for_end; t = 17 ;for_end = 18 ; if ( t <= 
	for_end) do 
	  {
	    r = maxlink [t ];
	    while ( r != 0 ) {
		
	      q = mem [r ].hhfield .lhfield ;
	      if ( t == 17 ) 
	      {
		if ( curexp == q ) 
		if ( curtype == 17 ) 
		curtype = 18 ;
		mem [q + 1 ].hhfield .v.RH = poverv ( mem [q + 1 ].hhfield 
		.v.RH , 65536L , 17 , 18 ) ;
		mem [q ].hhfield .b0 = 18 ;
		mem [r + 1 ].cint = roundfraction ( mem [r + 1 ].cint ) ;
	      } 
	      mem [q + 1 ].hhfield .v.RH = pplusfq ( mem [q + 1 ].hhfield 
	      .v.RH , makescaled ( mem [r + 1 ].cint , - (integer) v ) , s , 
	      18 , 18 ) ;
	      if ( mem [q + 1 ].hhfield .v.RH == depfinal ) 
	      makeknown ( q , depfinal ) ;
	      q = r ;
	      r = mem [r ].hhfield .v.RH ;
	      freenode ( q , 2 ) ;
	    } 
	  } 
	while ( t++ < for_end ) ;} 
	flushnodelist ( s ) ;
	if ( fixneeded ) 
	fixdependencies () ;
	{
	  if ( aritherror ) 
	  cleararith () ;
	} 
      } 
    } 
    break ;
  case 20 : 
  case 21 : 
    confusion ( 762 ) ;
    break ;
  case 22 : 
  case 23 : 
    deletemacref ( mem [p + 1 ].cint ) ;
    break ;
  } 
  mem [p ].hhfield .b0 = 0 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zflushcurexp ( scaled v ) 
#else
zflushcurexp ( v ) 
  scaled v ;
#endif
{
  switch ( curtype ) 
  {case 3 : 
  case 5 : 
  case 7 : 
  case 12 : 
  case 10 : 
  case 13 : 
  case 14 : 
  case 17 : 
  case 18 : 
  case 19 : 
    {
      recyclevalue ( curexp ) ;
      freenode ( curexp , 2 ) ;
    } 
    break ;
  case 6 : 
    if ( mem [curexp ].hhfield .lhfield == 0 ) 
    tosspen ( curexp ) ;
    else decr ( mem [curexp ].hhfield .lhfield ) ;
    break ;
  case 4 : 
    {
      if ( strref [curexp ]< 127 ) 
      if ( strref [curexp ]> 1 ) 
      decr ( strref [curexp ]) ;
      else flushstring ( curexp ) ;
    } 
    break ;
  case 8 : 
  case 9 : 
    tossknotlist ( curexp ) ;
    break ;
  case 11 : 
    tossedges ( curexp ) ;
    break ;
    default: 
    ;
    break ;
  } 
  curtype = 16 ;
  curexp = v ;
} 
void 
#ifdef HAVE_PROTOTYPES
zflusherror ( scaled v ) 
#else
zflusherror ( v ) 
  scaled v ;
#endif
{
  error () ;
  flushcurexp ( v ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
putgeterror ( void ) 
#else
putgeterror ( ) 
#endif
{
  backerror () ;
  getxnext () ;
} 
void 
#ifdef HAVE_PROTOTYPES
zputgetflusherror ( scaled v ) 
#else
zputgetflusherror ( v ) 
  scaled v ;
#endif
{
  putgeterror () ;
  flushcurexp ( v ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zflushbelowvariable ( halfword p ) 
#else
zflushbelowvariable ( p ) 
  halfword p ;
#endif
{
  halfword q, r  ;
  if ( mem [p ].hhfield .b0 != 21 ) 
  recyclevalue ( p ) ;
  else {
      
    q = mem [p + 1 ].hhfield .v.RH ;
    while ( mem [q ].hhfield .b1 == 3 ) {
	
      flushbelowvariable ( q ) ;
      r = q ;
      q = mem [q ].hhfield .v.RH ;
      freenode ( r , 3 ) ;
    } 
    r = mem [p + 1 ].hhfield .lhfield ;
    q = mem [r ].hhfield .v.RH ;
    recyclevalue ( r ) ;
    if ( mem [p ].hhfield .b1 <= 1 ) 
    freenode ( r , 2 ) ;
    else freenode ( r , 3 ) ;
    do {
	flushbelowvariable ( q ) ;
      r = q ;
      q = mem [q ].hhfield .v.RH ;
      freenode ( r , 3 ) ;
    } while ( ! ( q == 17 ) ) ;
    mem [p ].hhfield .b0 = 0 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zflushvariable ( halfword p , halfword t , boolean discardsuffixes ) 
#else
zflushvariable ( p , t , discardsuffixes ) 
  halfword p ;
  halfword t ;
  boolean discardsuffixes ;
#endif
{
  /* 10 */ halfword q, r  ;
  halfword n  ;
  while ( t != 0 ) {
      
    if ( mem [p ].hhfield .b0 != 21 ) 
    goto lab10 ;
    n = mem [t ].hhfield .lhfield ;
    t = mem [t ].hhfield .v.RH ;
    if ( n == 0 ) 
    {
      r = p + 1 ;
      q = mem [r ].hhfield .v.RH ;
      while ( mem [q ].hhfield .b1 == 3 ) {
	  
	flushvariable ( q , t , discardsuffixes ) ;
	if ( t == 0 ) 
	if ( mem [q ].hhfield .b0 == 21 ) 
	r = q ;
	else {
	    
	  mem [r ].hhfield .v.RH = mem [q ].hhfield .v.RH ;
	  freenode ( q , 3 ) ;
	} 
	else r = q ;
	q = mem [r ].hhfield .v.RH ;
      } 
    } 
    p = mem [p + 1 ].hhfield .lhfield ;
    do {
	r = p ;
      p = mem [p ].hhfield .v.RH ;
    } while ( ! ( mem [p + 2 ].hhfield .lhfield >= n ) ) ;
    if ( mem [p + 2 ].hhfield .lhfield != n ) 
    goto lab10 ;
  } 
  if ( discardsuffixes ) 
  flushbelowvariable ( p ) ;
  else {
      
    if ( mem [p ].hhfield .b0 == 21 ) 
    p = mem [p + 1 ].hhfield .lhfield ;
    recyclevalue ( p ) ;
  } 
  lab10: ;
} 
smallnumber 
#ifdef HAVE_PROTOTYPES
zundtype ( halfword p ) 
#else
zundtype ( p ) 
  halfword p ;
#endif
{
  register smallnumber Result; switch ( mem [p ].hhfield .b0 ) 
  {case 0 : 
  case 1 : 
    Result = 0 ;
    break ;
  case 2 : 
  case 3 : 
    Result = 3 ;
    break ;
  case 4 : 
  case 5 : 
    Result = 5 ;
    break ;
  case 6 : 
  case 7 : 
  case 8 : 
    Result = 7 ;
    break ;
  case 9 : 
  case 10 : 
    Result = 10 ;
    break ;
  case 11 : 
  case 12 : 
    Result = 12 ;
    break ;
  case 13 : 
  case 14 : 
  case 15 : 
    Result = mem [p ].hhfield .b0 ;
    break ;
  case 16 : 
  case 17 : 
  case 18 : 
  case 19 : 
    Result = 15 ;
    break ;
  } 
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zclearsymbol ( halfword p , boolean saving ) 
#else
zclearsymbol ( p , saving ) 
  halfword p ;
  boolean saving ;
#endif
{
  halfword q  ;
  q = eqtb [p ].v.RH ;
  switch ( eqtb [p ].lhfield % 86 ) 
  {case 10 : 
  case 53 : 
  case 44 : 
  case 49 : 
    if ( ! saving ) 
    deletemacref ( q ) ;
    break ;
  case 41 : 
    if ( q != 0 ) 
    if ( saving ) 
    mem [q ].hhfield .b1 = 1 ;
    else {
	
      flushbelowvariable ( q ) ;
      freenode ( q , 2 ) ;
    } 
    break ;
    default: 
    ;
    break ;
  } 
  eqtb [p ]= eqtb [9769 ];
} 
void 
#ifdef HAVE_PROTOTYPES
zsavevariable ( halfword q ) 
#else
zsavevariable ( q ) 
  halfword q ;
#endif
{
  halfword p  ;
  if ( saveptr != 0 ) 
  {
    p = getnode ( 2 ) ;
    mem [p ].hhfield .lhfield = q ;
    mem [p ].hhfield .v.RH = saveptr ;
    mem [p + 1 ].hhfield = eqtb [q ];
    saveptr = p ;
  } 
  clearsymbol ( q , ( saveptr != 0 ) ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zsaveinternal ( halfword q ) 
#else
zsaveinternal ( q ) 
  halfword q ;
#endif
{
  halfword p  ;
  if ( saveptr != 0 ) 
  {
    p = getnode ( 2 ) ;
    mem [p ].hhfield .lhfield = 9769 + q ;
    mem [p ].hhfield .v.RH = saveptr ;
    mem [p + 1 ].cint = internal [q ];
    saveptr = p ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
unsave ( void ) 
#else
unsave ( ) 
#endif
{
  halfword q  ;
  halfword p  ;
  while ( mem [saveptr ].hhfield .lhfield != 0 ) {
      
    q = mem [saveptr ].hhfield .lhfield ;
    if ( q > 9769 ) 
    {
      if ( internal [8 ]> 0 ) 
      {
	begindiagnostic () ;
	printnl ( 515 ) ;
	print ( intname [q - ( 9769 ) ]) ;
	printchar ( 61 ) ;
	printscaled ( mem [saveptr + 1 ].cint ) ;
	printchar ( 125 ) ;
	enddiagnostic ( false ) ;
      } 
      internal [q - ( 9769 ) ]= mem [saveptr + 1 ].cint ;
    } 
    else {
	
      if ( internal [8 ]> 0 ) 
      {
	begindiagnostic () ;
	printnl ( 515 ) ;
	print ( hash [q ].v.RH ) ;
	printchar ( 125 ) ;
	enddiagnostic ( false ) ;
      } 
      clearsymbol ( q , false ) ;
      eqtb [q ]= mem [saveptr + 1 ].hhfield ;
      if ( eqtb [q ].lhfield % 86 == 41 ) 
      {
	p = eqtb [q ].v.RH ;
	if ( p != 0 ) 
	mem [p ].hhfield .b1 = 0 ;
      } 
    } 
    p = mem [saveptr ].hhfield .v.RH ;
    freenode ( saveptr , 2 ) ;
    saveptr = p ;
  } 
  p = mem [saveptr ].hhfield .v.RH ;
  {
    mem [saveptr ].hhfield .v.RH = avail ;
    avail = saveptr ;
	;
#ifdef STAT
    decr ( dynused ) ;
#endif /* STAT */
  } 
  saveptr = p ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zcopyknot ( halfword p ) 
#else
zcopyknot ( p ) 
  halfword p ;
#endif
{
  register halfword Result; halfword q  ;
  char k  ;
  q = getnode ( 7 ) ;
  {register integer for_end; k = 0 ;for_end = 6 ; if ( k <= for_end) do 
    mem [q + k ]= mem [p + k ];
  while ( k++ < for_end ) ;} 
  Result = q ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zcopypath ( halfword p ) 
#else
zcopypath ( p ) 
  halfword p ;
#endif
{
  /* 10 */ register halfword Result; halfword q, pp, qq  ;
  q = getnode ( 7 ) ;
  qq = q ;
  pp = p ;
  while ( true ) {
      
    mem [qq ].hhfield .b0 = mem [pp ].hhfield .b0 ;
    mem [qq ].hhfield .b1 = mem [pp ].hhfield .b1 ;
    mem [qq + 1 ].cint = mem [pp + 1 ].cint ;
    mem [qq + 2 ].cint = mem [pp + 2 ].cint ;
    mem [qq + 3 ].cint = mem [pp + 3 ].cint ;
    mem [qq + 4 ].cint = mem [pp + 4 ].cint ;
    mem [qq + 5 ].cint = mem [pp + 5 ].cint ;
    mem [qq + 6 ].cint = mem [pp + 6 ].cint ;
    if ( mem [pp ].hhfield .v.RH == p ) 
    {
      mem [qq ].hhfield .v.RH = q ;
      Result = q ;
      goto lab10 ;
    } 
    mem [qq ].hhfield .v.RH = getnode ( 7 ) ;
    qq = mem [qq ].hhfield .v.RH ;
    pp = mem [pp ].hhfield .v.RH ;
  } 
  lab10: ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zhtapypoc ( halfword p ) 
#else
zhtapypoc ( p ) 
  halfword p ;
#endif
{
  /* 10 */ register halfword Result; halfword q, pp, qq, rr  ;
  q = getnode ( 7 ) ;
  qq = q ;
  pp = p ;
  while ( true ) {
      
    mem [qq ].hhfield .b1 = mem [pp ].hhfield .b0 ;
    mem [qq ].hhfield .b0 = mem [pp ].hhfield .b1 ;
    mem [qq + 1 ].cint = mem [pp + 1 ].cint ;
    mem [qq + 2 ].cint = mem [pp + 2 ].cint ;
    mem [qq + 5 ].cint = mem [pp + 3 ].cint ;
    mem [qq + 6 ].cint = mem [pp + 4 ].cint ;
    mem [qq + 3 ].cint = mem [pp + 5 ].cint ;
    mem [qq + 4 ].cint = mem [pp + 6 ].cint ;
    if ( mem [pp ].hhfield .v.RH == p ) 
    {
      mem [q ].hhfield .v.RH = qq ;
      pathtail = pp ;
      Result = q ;
      goto lab10 ;
    } 
    rr = getnode ( 7 ) ;
    mem [rr ].hhfield .v.RH = qq ;
    qq = rr ;
    pp = mem [pp ].hhfield .v.RH ;
  } 
  lab10: ;
  return Result ;
} 
fraction 
#ifdef HAVE_PROTOTYPES
zcurlratio ( scaled gamma , scaled atension , scaled btension ) 
#else
zcurlratio ( gamma , atension , btension ) 
  scaled gamma ;
  scaled atension ;
  scaled btension ;
#endif
{
  register fraction Result; fraction alpha, beta, num, denom, ff  ;
  alpha = makefraction ( 65536L , atension ) ;
  beta = makefraction ( 65536L , btension ) ;
  if ( alpha <= beta ) 
  {
    ff = makefraction ( alpha , beta ) ;
    ff = takefraction ( ff , ff ) ;
    gamma = takefraction ( gamma , ff ) ;
    beta = beta / 4096 ;
    denom = takefraction ( gamma , alpha ) + 196608L - beta ;
    num = takefraction ( gamma , 805306368L - alpha ) + beta ;
  } 
  else {
      
    ff = makefraction ( beta , alpha ) ;
    ff = takefraction ( ff , ff ) ;
    beta = takefraction ( beta , ff ) / 4096 ;
    denom = takefraction ( gamma , alpha ) + ( ff / 1365 ) - beta ;
    num = takefraction ( gamma , 805306368L - alpha ) + beta ;
  } 
  if ( num >= denom + denom + denom + denom ) 
  Result = 1073741824L ;
  else Result = makefraction ( num , denom ) ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zsetcontrols ( halfword p , halfword q , integer k ) 
#else
zsetcontrols ( p , q , k ) 
  halfword p ;
  halfword q ;
  integer k ;
#endif
{
  fraction rr, ss  ;
  scaled lt, rt  ;
  fraction sine  ;
  lt = abs ( mem [q + 4 ].cint ) ;
  rt = abs ( mem [p + 6 ].cint ) ;
  rr = velocity ( st , ct , sf , cf , rt ) ;
  ss = velocity ( sf , cf , st , ct , lt ) ;
  if ( ( mem [p + 6 ].cint < 0 ) || ( mem [q + 4 ].cint < 0 ) ) 
  if ( ( ( st >= 0 ) && ( sf >= 0 ) ) || ( ( st <= 0 ) && ( sf <= 0 ) ) ) 
  {
    sine = takefraction ( abs ( st ) , cf ) + takefraction ( abs ( sf ) , ct ) 
    ;
    if ( sine > 0 ) 
    {
      sine = takefraction ( sine , 268500992L ) ;
      if ( mem [p + 6 ].cint < 0 ) 
      if ( abvscd ( abs ( sf ) , 268435456L , rr , sine ) < 0 ) 
      rr = makefraction ( abs ( sf ) , sine ) ;
      if ( mem [q + 4 ].cint < 0 ) 
      if ( abvscd ( abs ( st ) , 268435456L , ss , sine ) < 0 ) 
      ss = makefraction ( abs ( st ) , sine ) ;
    } 
  } 
  mem [p + 5 ].cint = mem [p + 1 ].cint + takefraction ( takefraction ( 
  deltax [k ], ct ) - takefraction ( deltay [k ], st ) , rr ) ;
  mem [p + 6 ].cint = mem [p + 2 ].cint + takefraction ( takefraction ( 
  deltay [k ], ct ) + takefraction ( deltax [k ], st ) , rr ) ;
  mem [q + 3 ].cint = mem [q + 1 ].cint - takefraction ( takefraction ( 
  deltax [k ], cf ) + takefraction ( deltay [k ], sf ) , ss ) ;
  mem [q + 4 ].cint = mem [q + 2 ].cint - takefraction ( takefraction ( 
  deltay [k ], cf ) - takefraction ( deltax [k ], sf ) , ss ) ;
  mem [p ].hhfield .b1 = 1 ;
  mem [q ].hhfield .b0 = 1 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zsolvechoices ( halfword p , halfword q , halfword n ) 
#else
zsolvechoices ( p , q , n ) 
  halfword p ;
  halfword q ;
  halfword n ;
#endif
{
  /* 40 10 */ integer k  ;
  halfword r, s, t  ;
  fraction aa, bb, cc, ff, acc  ;
  scaled dd, ee  ;
  scaled lt, rt  ;
  k = 0 ;
  s = p ;
  while ( true ) {
      
    t = mem [s ].hhfield .v.RH ;
    if ( k == 0 ) 
    switch ( mem [s ].hhfield .b1 ) 
    {case 2 : 
      if ( mem [t ].hhfield .b0 == 2 ) 
      {
	aa = narg ( deltax [0 ], deltay [0 ]) ;
	nsincos ( mem [p + 5 ].cint - aa ) ;
	ct = ncos ;
	st = nsin ;
	nsincos ( mem [q + 3 ].cint - aa ) ;
	cf = ncos ;
	sf = - (integer) nsin ;
	setcontrols ( p , q , 0 ) ;
	goto lab10 ;
      } 
      else {
	  
	vv [0 ]= mem [s + 5 ].cint - narg ( deltax [0 ], deltay [0 ]) 
	;
	if ( abs ( vv [0 ]) > 188743680L ) 
	if ( vv [0 ]> 0 ) 
	vv [0 ]= vv [0 ]- 377487360L ;
	else vv [0 ]= vv [0 ]+ 377487360L ;
	uu [0 ]= 0 ;
	ww [0 ]= 0 ;
      } 
      break ;
    case 3 : 
      if ( mem [t ].hhfield .b0 == 3 ) 
      {
	mem [p ].hhfield .b1 = 1 ;
	mem [q ].hhfield .b0 = 1 ;
	lt = abs ( mem [q + 4 ].cint ) ;
	rt = abs ( mem [p + 6 ].cint ) ;
	if ( rt == 65536L ) 
	{
	  if ( deltax [0 ]>= 0 ) 
	  mem [p + 5 ].cint = mem [p + 1 ].cint + ( ( deltax [0 ]+ 1 ) / 
	  3 ) ;
	  else mem [p + 5 ].cint = mem [p + 1 ].cint + ( ( deltax [0 ]- 
	  1 ) / 3 ) ;
	  if ( deltay [0 ]>= 0 ) 
	  mem [p + 6 ].cint = mem [p + 2 ].cint + ( ( deltay [0 ]+ 1 ) / 
	  3 ) ;
	  else mem [p + 6 ].cint = mem [p + 2 ].cint + ( ( deltay [0 ]- 
	  1 ) / 3 ) ;
	} 
	else {
	    
	  ff = makefraction ( 65536L , 3 * rt ) ;
	  mem [p + 5 ].cint = mem [p + 1 ].cint + takefraction ( deltax [
	  0 ], ff ) ;
	  mem [p + 6 ].cint = mem [p + 2 ].cint + takefraction ( deltay [
	  0 ], ff ) ;
	} 
	if ( lt == 65536L ) 
	{
	  if ( deltax [0 ]>= 0 ) 
	  mem [q + 3 ].cint = mem [q + 1 ].cint - ( ( deltax [0 ]+ 1 ) / 
	  3 ) ;
	  else mem [q + 3 ].cint = mem [q + 1 ].cint - ( ( deltax [0 ]- 
	  1 ) / 3 ) ;
	  if ( deltay [0 ]>= 0 ) 
	  mem [q + 4 ].cint = mem [q + 2 ].cint - ( ( deltay [0 ]+ 1 ) / 
	  3 ) ;
	  else mem [q + 4 ].cint = mem [q + 2 ].cint - ( ( deltay [0 ]- 
	  1 ) / 3 ) ;
	} 
	else {
	    
	  ff = makefraction ( 65536L , 3 * lt ) ;
	  mem [q + 3 ].cint = mem [q + 1 ].cint - takefraction ( deltax [
	  0 ], ff ) ;
	  mem [q + 4 ].cint = mem [q + 2 ].cint - takefraction ( deltay [
	  0 ], ff ) ;
	} 
	goto lab10 ;
      } 
      else {
	  
	cc = mem [s + 5 ].cint ;
	lt = abs ( mem [t + 4 ].cint ) ;
	rt = abs ( mem [s + 6 ].cint ) ;
	if ( ( rt == 65536L ) && ( lt == 65536L ) ) 
	uu [0 ]= makefraction ( cc + cc + 65536L , cc + 131072L ) ;
	else uu [0 ]= curlratio ( cc , rt , lt ) ;
	vv [0 ]= - (integer) takefraction ( psi [1 ], uu [0 ]) ;
	ww [0 ]= 0 ;
      } 
      break ;
    case 4 : 
      {
	uu [0 ]= 0 ;
	vv [0 ]= 0 ;
	ww [0 ]= 268435456L ;
      } 
      break ;
    } 
    else switch ( mem [s ].hhfield .b0 ) 
    {case 5 : 
    case 4 : 
      {
	if ( abs ( mem [r + 6 ].cint ) == 65536L ) 
	{
	  aa = 134217728L ;
	  dd = 2 * delta [k ];
	} 
	else {
	    
	  aa = makefraction ( 65536L , 3 * abs ( mem [r + 6 ].cint ) - 
	  65536L ) ;
	  dd = takefraction ( delta [k ], 805306368L - makefraction ( 65536L 
	  , abs ( mem [r + 6 ].cint ) ) ) ;
	} 
	if ( abs ( mem [t + 4 ].cint ) == 65536L ) 
	{
	  bb = 134217728L ;
	  ee = 2 * delta [k - 1 ];
	} 
	else {
	    
	  bb = makefraction ( 65536L , 3 * abs ( mem [t + 4 ].cint ) - 
	  65536L ) ;
	  ee = takefraction ( delta [k - 1 ], 805306368L - makefraction ( 
	  65536L , abs ( mem [t + 4 ].cint ) ) ) ;
	} 
	cc = 268435456L - takefraction ( uu [k - 1 ], aa ) ;
	dd = takefraction ( dd , cc ) ;
	lt = abs ( mem [s + 4 ].cint ) ;
	rt = abs ( mem [s + 6 ].cint ) ;
	if ( lt != rt ) 
	if ( lt < rt ) 
	{
	  ff = makefraction ( lt , rt ) ;
	  ff = takefraction ( ff , ff ) ;
	  dd = takefraction ( dd , ff ) ;
	} 
	else {
	    
	  ff = makefraction ( rt , lt ) ;
	  ff = takefraction ( ff , ff ) ;
	  ee = takefraction ( ee , ff ) ;
	} 
	ff = makefraction ( ee , ee + dd ) ;
	uu [k ]= takefraction ( ff , bb ) ;
	acc = - (integer) takefraction ( psi [k + 1 ], uu [k ]) ;
	if ( mem [r ].hhfield .b1 == 3 ) 
	{
	  ww [k ]= 0 ;
	  vv [k ]= acc - takefraction ( psi [1 ], 268435456L - ff ) ;
	} 
	else {
	    
	  ff = makefraction ( 268435456L - ff , cc ) ;
	  acc = acc - takefraction ( psi [k ], ff ) ;
	  ff = takefraction ( ff , aa ) ;
	  vv [k ]= acc - takefraction ( vv [k - 1 ], ff ) ;
	  if ( ww [k - 1 ]== 0 ) 
	  ww [k ]= 0 ;
	  else ww [k ]= - (integer) takefraction ( ww [k - 1 ], ff ) ;
	} 
	if ( mem [s ].hhfield .b0 == 5 ) 
	{
	  aa = 0 ;
	  bb = 268435456L ;
	  do {
	      decr ( k ) ;
	    if ( k == 0 ) 
	    k = n ;
	    aa = vv [k ]- takefraction ( aa , uu [k ]) ;
	    bb = ww [k ]- takefraction ( bb , uu [k ]) ;
	  } while ( ! ( k == n ) ) ;
	  aa = makefraction ( aa , 268435456L - bb ) ;
	  theta [n ]= aa ;
	  vv [0 ]= aa ;
	  {register integer for_end; k = 1 ;for_end = n - 1 ; if ( k <= 
	  for_end) do 
	    vv [k ]= vv [k ]+ takefraction ( aa , ww [k ]) ;
	  while ( k++ < for_end ) ;} 
	  goto lab40 ;
	} 
      } 
      break ;
    case 3 : 
      {
	cc = mem [s + 3 ].cint ;
	lt = abs ( mem [s + 4 ].cint ) ;
	rt = abs ( mem [r + 6 ].cint ) ;
	if ( ( rt == 65536L ) && ( lt == 65536L ) ) 
	ff = makefraction ( cc + cc + 65536L , cc + 131072L ) ;
	else ff = curlratio ( cc , lt , rt ) ;
	theta [n ]= - (integer) makefraction ( takefraction ( vv [n - 1 ], 
	ff ) , 268435456L - takefraction ( ff , uu [n - 1 ]) ) ;
	goto lab40 ;
      } 
      break ;
    case 2 : 
      {
	theta [n ]= mem [s + 3 ].cint - narg ( deltax [n - 1 ], deltay [
	n - 1 ]) ;
	if ( abs ( theta [n ]) > 188743680L ) 
	if ( theta [n ]> 0 ) 
	theta [n ]= theta [n ]- 377487360L ;
	else theta [n ]= theta [n ]+ 377487360L ;
	goto lab40 ;
      } 
      break ;
    } 
    r = s ;
    s = t ;
    incr ( k ) ;
  } 
  lab40: {
      register integer for_end; k = n - 1 ;for_end = 0 ; if ( k >= 
  for_end) do 
    theta [k ]= vv [k ]- takefraction ( theta [k + 1 ], uu [k ]) ;
  while ( k-- > for_end ) ;} 
  s = p ;
  k = 0 ;
  do {
      t = mem [s ].hhfield .v.RH ;
    nsincos ( theta [k ]) ;
    st = nsin ;
    ct = ncos ;
    nsincos ( - (integer) psi [k + 1 ]- theta [k + 1 ]) ;
    sf = nsin ;
    cf = ncos ;
    setcontrols ( s , t , k ) ;
    incr ( k ) ;
    s = t ;
  } while ( ! ( k == n ) ) ;
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zmakechoices ( halfword knots ) 
#else
zmakechoices ( knots ) 
  halfword knots ;
#endif
{
  /* 30 */ halfword h  ;
  halfword p, q  ;
  integer k, n  ;
  halfword s, t  ;
  scaled delx, dely  ;
  fraction sine, cosine  ;
  {
    if ( aritherror ) 
    cleararith () ;
  } 
  if ( internal [4 ]> 0 ) 
  printpath ( knots , 525 , true ) ;
  p = knots ;
  do {
      q = mem [p ].hhfield .v.RH ;
    if ( mem [p + 1 ].cint == mem [q + 1 ].cint ) 
    if ( mem [p + 2 ].cint == mem [q + 2 ].cint ) 
    if ( mem [p ].hhfield .b1 > 1 ) 
    {
      mem [p ].hhfield .b1 = 1 ;
      if ( mem [p ].hhfield .b0 == 4 ) 
      {
	mem [p ].hhfield .b0 = 3 ;
	mem [p + 3 ].cint = 65536L ;
      } 
      mem [q ].hhfield .b0 = 1 ;
      if ( mem [q ].hhfield .b1 == 4 ) 
      {
	mem [q ].hhfield .b1 = 3 ;
	mem [q + 5 ].cint = 65536L ;
      } 
      mem [p + 5 ].cint = mem [p + 1 ].cint ;
      mem [q + 3 ].cint = mem [p + 1 ].cint ;
      mem [p + 6 ].cint = mem [p + 2 ].cint ;
      mem [q + 4 ].cint = mem [p + 2 ].cint ;
    } 
    p = q ;
  } while ( ! ( p == knots ) ) ;
  h = knots ;
  while ( true ) {
      
    if ( mem [h ].hhfield .b0 != 4 ) 
    goto lab30 ;
    if ( mem [h ].hhfield .b1 != 4 ) 
    goto lab30 ;
    h = mem [h ].hhfield .v.RH ;
    if ( h == knots ) 
    {
      mem [h ].hhfield .b0 = 5 ;
      goto lab30 ;
    } 
  } 
  lab30: ;
  p = h ;
  do {
      q = mem [p ].hhfield .v.RH ;
    if ( mem [p ].hhfield .b1 >= 2 ) 
    {
      while ( ( mem [q ].hhfield .b0 == 4 ) && ( mem [q ].hhfield .b1 == 4 
      ) ) q = mem [q ].hhfield .v.RH ;
      k = 0 ;
      s = p ;
      n = pathsize ;
      do {
	  t = mem [s ].hhfield .v.RH ;
	deltax [k ]= mem [t + 1 ].cint - mem [s + 1 ].cint ;
	deltay [k ]= mem [t + 2 ].cint - mem [s + 2 ].cint ;
	delta [k ]= pythadd ( deltax [k ], deltay [k ]) ;
	if ( k > 0 ) 
	{
	  sine = makefraction ( deltay [k - 1 ], delta [k - 1 ]) ;
	  cosine = makefraction ( deltax [k - 1 ], delta [k - 1 ]) ;
	  psi [k ]= narg ( takefraction ( deltax [k ], cosine ) + 
	  takefraction ( deltay [k ], sine ) , takefraction ( deltay [k ], 
	  cosine ) - takefraction ( deltax [k ], sine ) ) ;
	} 
	incr ( k ) ;
	s = t ;
	if ( k == pathsize ) 
	overflow ( 530 , pathsize ) ;
	if ( s == q ) 
	n = k ;
      } while ( ! ( ( k >= n ) && ( mem [s ].hhfield .b0 != 5 ) ) ) ;
      if ( k == n ) 
      psi [n ]= 0 ;
      else psi [k ]= psi [1 ];
      if ( mem [q ].hhfield .b0 == 4 ) 
      {
	delx = mem [q + 5 ].cint - mem [q + 1 ].cint ;
	dely = mem [q + 6 ].cint - mem [q + 2 ].cint ;
	if ( ( delx == 0 ) && ( dely == 0 ) ) 
	{
	  mem [q ].hhfield .b0 = 3 ;
	  mem [q + 3 ].cint = 65536L ;
	} 
	else {
	    
	  mem [q ].hhfield .b0 = 2 ;
	  mem [q + 3 ].cint = narg ( delx , dely ) ;
	} 
      } 
      if ( ( mem [p ].hhfield .b1 == 4 ) && ( mem [p ].hhfield .b0 == 1 ) 
      ) 
      {
	delx = mem [p + 1 ].cint - mem [p + 3 ].cint ;
	dely = mem [p + 2 ].cint - mem [p + 4 ].cint ;
	if ( ( delx == 0 ) && ( dely == 0 ) ) 
	{
	  mem [p ].hhfield .b1 = 3 ;
	  mem [p + 5 ].cint = 65536L ;
	} 
	else {
	    
	  mem [p ].hhfield .b1 = 2 ;
	  mem [p + 5 ].cint = narg ( delx , dely ) ;
	} 
      } 
      solvechoices ( p , q , n ) ;
    } 
    p = q ;
  } while ( ! ( p == h ) ) ;
  if ( internal [4 ]> 0 ) 
  printpath ( knots , 526 , true ) ;
  if ( aritherror ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 261 ) ;
      print ( 527 ) ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 528 ;
      helpline [0 ]= 529 ;
    } 
    putgeterror () ;
    aritherror = false ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zmakemoves ( scaled xx0 , scaled xx1 , scaled xx2 , scaled xx3 , scaled yy0 , 
scaled yy1 , scaled yy2 , scaled yy3 , smallnumber xicorr , smallnumber 
etacorr ) 
#else
zmakemoves ( xx0 , xx1 , xx2 , xx3 , yy0 , yy1 , yy2 , yy3 , xicorr , etacorr 
) 
  scaled xx0 ;
  scaled xx1 ;
  scaled xx2 ;
  scaled xx3 ;
  scaled yy0 ;
  scaled yy1 ;
  scaled yy2 ;
  scaled yy3 ;
  smallnumber xicorr ;
  smallnumber etacorr ;
#endif
{
  /* 22 30 10 */ integer x1, x2, x3, m, r, y1, y2, y3, n, s, l  ;
  integer q, t, u, x2a, x3a, y2a, y3a  ;
  if ( ( xx3 < xx0 ) || ( yy3 < yy0 ) ) 
  confusion ( 109 ) ;
  l = 16 ;
  bisectptr = 0 ;
  x1 = xx1 - xx0 ;
  x2 = xx2 - xx1 ;
  x3 = xx3 - xx2 ;
  if ( xx0 >= xicorr ) 
  r = ( xx0 - xicorr ) % 65536L ;
  else r = 65535L - ( ( - (integer) xx0 + xicorr - 1 ) % 65536L ) ;
  m = ( xx3 - xx0 + r ) / 65536L ;
  y1 = yy1 - yy0 ;
  y2 = yy2 - yy1 ;
  y3 = yy3 - yy2 ;
  if ( yy0 >= etacorr ) 
  s = ( yy0 - etacorr ) % 65536L ;
  else s = 65535L - ( ( - (integer) yy0 + etacorr - 1 ) % 65536L ) ;
  n = ( yy3 - yy0 + s ) / 65536L ;
  if ( ( xx3 - xx0 >= 268435456L ) || ( yy3 - yy0 >= 268435456L ) ) 
  {
    x1 = half ( x1 + xicorr ) ;
    x2 = half ( x2 + xicorr ) ;
    x3 = half ( x3 + xicorr ) ;
    r = half ( r + xicorr ) ;
    y1 = half ( y1 + etacorr ) ;
    y2 = half ( y2 + etacorr ) ;
    y3 = half ( y3 + etacorr ) ;
    s = half ( s + etacorr ) ;
    l = 15 ;
  } 
  while ( true ) {
      
    lab22: if ( m == 0 ) 
    while ( n > 0 ) {
	
      incr ( moveptr ) ;
      move [moveptr ]= 1 ;
      decr ( n ) ;
    } 
    else if ( n == 0 ) 
    move [moveptr ]= move [moveptr ]+ m ;
    else if ( m + n == 2 ) 
    {
      r = twotothe [l ]- r ;
      s = twotothe [l ]- s ;
      while ( l < 30 ) {
	  
	x3a = x3 ;
	x2a = half ( x2 + x3 + xicorr ) ;
	x2 = half ( x1 + x2 + xicorr ) ;
	x3 = half ( x2 + x2a + xicorr ) ;
	t = x1 + x2 + x3 ;
	r = r + r - xicorr ;
	y3a = y3 ;
	y2a = half ( y2 + y3 + etacorr ) ;
	y2 = half ( y1 + y2 + etacorr ) ;
	y3 = half ( y2 + y2a + etacorr ) ;
	u = y1 + y2 + y3 ;
	s = s + s - etacorr ;
	if ( t < r ) 
	if ( u < s ) 
	{
	  x1 = x3 ;
	  x2 = x2a ;
	  x3 = x3a ;
	  r = r - t ;
	  y1 = y3 ;
	  y2 = y2a ;
	  y3 = y3a ;
	  s = s - u ;
	} 
	else {
	    
	  {
	    incr ( moveptr ) ;
	    move [moveptr ]= 2 ;
	  } 
	  goto lab30 ;
	} 
	else if ( u < s ) 
	{
	  {
	    incr ( move [moveptr ]) ;
	    incr ( moveptr ) ;
	    move [moveptr ]= 1 ;
	  } 
	  goto lab30 ;
	} 
	incr ( l ) ;
      } 
      r = r - xicorr ;
      s = s - etacorr ;
      if ( abvscd ( x1 + x2 + x3 , s , y1 + y2 + y3 , r ) - xicorr >= 0 ) 
      {
	incr ( move [moveptr ]) ;
	incr ( moveptr ) ;
	move [moveptr ]= 1 ;
      } 
      else {
	  
	incr ( moveptr ) ;
	move [moveptr ]= 2 ;
      } 
      lab30: ;
    } 
    else {
	
      incr ( l ) ;
      bisectstack [bisectptr + 10 ]= l ;
      bisectstack [bisectptr + 2 ]= x3 ;
      bisectstack [bisectptr + 1 ]= half ( x2 + x3 + xicorr ) ;
      x2 = half ( x1 + x2 + xicorr ) ;
      x3 = half ( x2 + bisectstack [bisectptr + 1 ]+ xicorr ) ;
      bisectstack [bisectptr ]= x3 ;
      r = r + r + xicorr ;
      t = x1 + x2 + x3 + r ;
      q = t / twotothe [l ];
      bisectstack [bisectptr + 3 ]= t % twotothe [l ];
      bisectstack [bisectptr + 4 ]= m - q ;
      m = q ;
      bisectstack [bisectptr + 7 ]= y3 ;
      bisectstack [bisectptr + 6 ]= half ( y2 + y3 + etacorr ) ;
      y2 = half ( y1 + y2 + etacorr ) ;
      y3 = half ( y2 + bisectstack [bisectptr + 6 ]+ etacorr ) ;
      bisectstack [bisectptr + 5 ]= y3 ;
      s = s + s + etacorr ;
      u = y1 + y2 + y3 + s ;
      q = u / twotothe [l ];
      bisectstack [bisectptr + 8 ]= u % twotothe [l ];
      bisectstack [bisectptr + 9 ]= n - q ;
      n = q ;
      bisectptr = bisectptr + 11 ;
      goto lab22 ;
    } 
    if ( bisectptr == 0 ) 
    goto lab10 ;
    bisectptr = bisectptr - 11 ;
    x1 = bisectstack [bisectptr ];
    x2 = bisectstack [bisectptr + 1 ];
    x3 = bisectstack [bisectptr + 2 ];
    r = bisectstack [bisectptr + 3 ];
    m = bisectstack [bisectptr + 4 ];
    y1 = bisectstack [bisectptr + 5 ];
    y2 = bisectstack [bisectptr + 6 ];
    y3 = bisectstack [bisectptr + 7 ];
    s = bisectstack [bisectptr + 8 ];
    n = bisectstack [bisectptr + 9 ];
    l = bisectstack [bisectptr + 10 ];
  } 
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zsmoothmoves ( integer b , integer t ) 
#else
zsmoothmoves ( b , t ) 
  integer b ;
  integer t ;
#endif
{
  integer k  ;
  integer a, aa, aaa  ;
  if ( t - b >= 3 ) 
  {
    k = b + 2 ;
    aa = move [k - 1 ];
    aaa = move [k - 2 ];
    do {
	a = move [k ];
      if ( abs ( a - aa ) > 1 ) 
      if ( a > aa ) 
      {
	if ( aaa >= aa ) 
	if ( a >= move [k + 1 ]) 
	{
	  incr ( move [k - 1 ]) ;
	  move [k ]= a - 1 ;
	} 
      } 
      else {
	  
	if ( aaa <= aa ) 
	if ( a <= move [k + 1 ]) 
	{
	  decr ( move [k - 1 ]) ;
	  move [k ]= a + 1 ;
	} 
      } 
      incr ( k ) ;
      aaa = aa ;
      aa = a ;
    } while ( ! ( k == t ) ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zinitedges ( halfword h ) 
#else
zinitedges ( h ) 
  halfword h ;
#endif
{
  mem [h ].hhfield .lhfield = h ;
  mem [h ].hhfield .v.RH = h ;
  mem [h + 1 ].hhfield .lhfield = 8191 ;
  mem [h + 1 ].hhfield .v.RH = 1 ;
  mem [h + 2 ].hhfield .lhfield = 8191 ;
  mem [h + 2 ].hhfield .v.RH = 1 ;
  mem [h + 3 ].hhfield .lhfield = 4096 ;
  mem [h + 3 ].hhfield .v.RH = 0 ;
  mem [h + 4 ].cint = 0 ;
  mem [h + 5 ].hhfield .v.RH = h ;
  mem [h + 5 ].hhfield .lhfield = 0 ;
} 
void 
#ifdef HAVE_PROTOTYPES
fixoffset ( void ) 
#else
fixoffset ( ) 
#endif
{
  halfword p, q  ;
  integer delta  ;
  delta = 8 * ( mem [curedges + 3 ].hhfield .lhfield - 4096 ) ;
  mem [curedges + 3 ].hhfield .lhfield = 4096 ;
  q = mem [curedges ].hhfield .v.RH ;
  while ( q != curedges ) {
      
    p = mem [q + 1 ].hhfield .v.RH ;
    while ( p != memtop ) {
	
      mem [p ].hhfield .lhfield = mem [p ].hhfield .lhfield - delta ;
      p = mem [p ].hhfield .v.RH ;
    } 
    p = mem [q + 1 ].hhfield .lhfield ;
    while ( p > 1 ) {
	
      mem [p ].hhfield .lhfield = mem [p ].hhfield .lhfield - delta ;
      p = mem [p ].hhfield .v.RH ;
    } 
    q = mem [q ].hhfield .v.RH ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zedgeprep ( integer ml , integer mr , integer nl , integer nr ) 
#else
zedgeprep ( ml , mr , nl , nr ) 
  integer ml ;
  integer mr ;
  integer nl ;
  integer nr ;
#endif
{
  halfword delta  ;
  integer temp  ;
  halfword p, q  ;
  ml = ml + 4096 ;
  mr = mr + 4096 ;
  nl = nl + 4096 ;
  nr = nr + 4095 ;
  if ( ml < mem [curedges + 2 ].hhfield .lhfield ) 
  mem [curedges + 2 ].hhfield .lhfield = ml ;
  if ( mr > mem [curedges + 2 ].hhfield .v.RH ) 
  mem [curedges + 2 ].hhfield .v.RH = mr ;
  temp = mem [curedges + 3 ].hhfield .lhfield - 4096 ;
  if ( ! ( abs ( mem [curedges + 2 ].hhfield .lhfield + temp - 4096 ) < 4096 
  ) || ! ( abs ( mem [curedges + 2 ].hhfield .v.RH + temp - 4096 ) < 4096 ) 
  ) 
  fixoffset () ;
  if ( mem [curedges ].hhfield .v.RH == curedges ) 
  {
    mem [curedges + 1 ].hhfield .lhfield = nr + 1 ;
    mem [curedges + 1 ].hhfield .v.RH = nr ;
  } 
  if ( nl < mem [curedges + 1 ].hhfield .lhfield ) 
  {
    delta = mem [curedges + 1 ].hhfield .lhfield - nl ;
    mem [curedges + 1 ].hhfield .lhfield = nl ;
    p = mem [curedges ].hhfield .v.RH ;
    do {
	q = getnode ( 2 ) ;
      mem [q + 1 ].hhfield .v.RH = memtop ;
      mem [q + 1 ].hhfield .lhfield = 1 ;
      mem [p ].hhfield .lhfield = q ;
      mem [q ].hhfield .v.RH = p ;
      p = q ;
      decr ( delta ) ;
    } while ( ! ( delta == 0 ) ) ;
    mem [p ].hhfield .lhfield = curedges ;
    mem [curedges ].hhfield .v.RH = p ;
    if ( mem [curedges + 5 ].hhfield .v.RH == curedges ) 
    mem [curedges + 5 ].hhfield .lhfield = nl - 1 ;
  } 
  if ( nr > mem [curedges + 1 ].hhfield .v.RH ) 
  {
    delta = nr - mem [curedges + 1 ].hhfield .v.RH ;
    mem [curedges + 1 ].hhfield .v.RH = nr ;
    p = mem [curedges ].hhfield .lhfield ;
    do {
	q = getnode ( 2 ) ;
      mem [q + 1 ].hhfield .v.RH = memtop ;
      mem [q + 1 ].hhfield .lhfield = 1 ;
      mem [p ].hhfield .v.RH = q ;
      mem [q ].hhfield .lhfield = p ;
      p = q ;
      decr ( delta ) ;
    } while ( ! ( delta == 0 ) ) ;
    mem [p ].hhfield .v.RH = curedges ;
    mem [curedges ].hhfield .lhfield = p ;
    if ( mem [curedges + 5 ].hhfield .v.RH == curedges ) 
    mem [curedges + 5 ].hhfield .lhfield = nr + 1 ;
  } 
} 
halfword 
#ifdef HAVE_PROTOTYPES
zcopyedges ( halfword h ) 
#else
zcopyedges ( h ) 
  halfword h ;
#endif
{
  register halfword Result; halfword p, r  ;
  halfword hh, pp, qq, rr, ss  ;
  hh = getnode ( 6 ) ;
  mem [hh + 1 ]= mem [h + 1 ];
  mem [hh + 2 ]= mem [h + 2 ];
  mem [hh + 3 ]= mem [h + 3 ];
  mem [hh + 4 ]= mem [h + 4 ];
  mem [hh + 5 ].hhfield .lhfield = mem [hh + 1 ].hhfield .v.RH + 1 ;
  mem [hh + 5 ].hhfield .v.RH = hh ;
  p = mem [h ].hhfield .v.RH ;
  qq = hh ;
  while ( p != h ) {
      
    pp = getnode ( 2 ) ;
    mem [qq ].hhfield .v.RH = pp ;
    mem [pp ].hhfield .lhfield = qq ;
    r = mem [p + 1 ].hhfield .v.RH ;
    rr = pp + 1 ;
    while ( r != memtop ) {
	
      ss = getavail () ;
      mem [rr ].hhfield .v.RH = ss ;
      rr = ss ;
      mem [rr ].hhfield .lhfield = mem [r ].hhfield .lhfield ;
      r = mem [r ].hhfield .v.RH ;
    } 
    mem [rr ].hhfield .v.RH = memtop ;
    r = mem [p + 1 ].hhfield .lhfield ;
    rr = memtop - 1 ;
    while ( r > 1 ) {
	
      ss = getavail () ;
      mem [rr ].hhfield .v.RH = ss ;
      rr = ss ;
      mem [rr ].hhfield .lhfield = mem [r ].hhfield .lhfield ;
      r = mem [r ].hhfield .v.RH ;
    } 
    mem [rr ].hhfield .v.RH = r ;
    mem [pp + 1 ].hhfield .lhfield = mem [memtop - 1 ].hhfield .v.RH ;
    p = mem [p ].hhfield .v.RH ;
    qq = pp ;
  } 
  mem [qq ].hhfield .v.RH = hh ;
  mem [hh ].hhfield .lhfield = qq ;
  Result = hh ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
yreflectedges ( void ) 
#else
yreflectedges ( ) 
#endif
{
  halfword p, q, r  ;
  p = mem [curedges + 1 ].hhfield .lhfield ;
  mem [curedges + 1 ].hhfield .lhfield = 8191 - mem [curedges + 1 ]
  .hhfield .v.RH ;
  mem [curedges + 1 ].hhfield .v.RH = 8191 - p ;
  mem [curedges + 5 ].hhfield .lhfield = 8191 - mem [curedges + 5 ]
  .hhfield .lhfield ;
  p = mem [curedges ].hhfield .v.RH ;
  q = curedges ;
  do {
      r = mem [p ].hhfield .v.RH ;
    mem [p ].hhfield .v.RH = q ;
    mem [q ].hhfield .lhfield = p ;
    q = p ;
    p = r ;
  } while ( ! ( q == curedges ) ) ;
  mem [curedges + 4 ].cint = 0 ;
} 
void 
#ifdef HAVE_PROTOTYPES
xreflectedges ( void ) 
#else
xreflectedges ( ) 
#endif
{
  halfword p, q, r, s  ;
  integer m  ;
  p = mem [curedges + 2 ].hhfield .lhfield ;
  mem [curedges + 2 ].hhfield .lhfield = 8192 - mem [curedges + 2 ]
  .hhfield .v.RH ;
  mem [curedges + 2 ].hhfield .v.RH = 8192 - p ;
  m = ( 4096 + mem [curedges + 3 ].hhfield .lhfield ) * 8 + 8 ;
  mem [curedges + 3 ].hhfield .lhfield = 4096 ;
  p = mem [curedges ].hhfield .v.RH ;
  do {
      q = mem [p + 1 ].hhfield .v.RH ;
    r = memtop ;
    while ( q != memtop ) {
	
      s = mem [q ].hhfield .v.RH ;
      mem [q ].hhfield .v.RH = r ;
      r = q ;
      mem [r ].hhfield .lhfield = m - mem [q ].hhfield .lhfield ;
      q = s ;
    } 
    mem [p + 1 ].hhfield .v.RH = r ;
    q = mem [p + 1 ].hhfield .lhfield ;
    while ( q > 1 ) {
	
      mem [q ].hhfield .lhfield = m - mem [q ].hhfield .lhfield ;
      q = mem [q ].hhfield .v.RH ;
    } 
    p = mem [p ].hhfield .v.RH ;
  } while ( ! ( p == curedges ) ) ;
  mem [curedges + 4 ].cint = 0 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zyscaleedges ( integer s ) 
#else
zyscaleedges ( s ) 
  integer s ;
#endif
{
  halfword p, q, pp, r, rr, ss  ;
  integer t  ;
  if ( ( s * ( mem [curedges + 1 ].hhfield .v.RH - 4095 ) >= 4096 ) || ( s * 
  ( mem [curedges + 1 ].hhfield .lhfield - 4096 ) <= -4096 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 261 ) ;
      print ( 534 ) ;
    } 
    {
      helpptr = 3 ;
      helpline [2 ]= 535 ;
      helpline [1 ]= 536 ;
      helpline [0 ]= 537 ;
    } 
    putgeterror () ;
  } 
  else {
      
    mem [curedges + 1 ].hhfield .v.RH = s * ( mem [curedges + 1 ].hhfield 
    .v.RH - 4095 ) + 4095 ;
    mem [curedges + 1 ].hhfield .lhfield = s * ( mem [curedges + 1 ]
    .hhfield .lhfield - 4096 ) + 4096 ;
    p = curedges ;
    do {
	q = p ;
      p = mem [p ].hhfield .v.RH ;
      {register integer for_end; t = 2 ;for_end = s ; if ( t <= for_end) do 
	{
	  pp = getnode ( 2 ) ;
	  mem [q ].hhfield .v.RH = pp ;
	  mem [p ].hhfield .lhfield = pp ;
	  mem [pp ].hhfield .v.RH = p ;
	  mem [pp ].hhfield .lhfield = q ;
	  q = pp ;
	  r = mem [p + 1 ].hhfield .v.RH ;
	  rr = pp + 1 ;
	  while ( r != memtop ) {
	      
	    ss = getavail () ;
	    mem [rr ].hhfield .v.RH = ss ;
	    rr = ss ;
	    mem [rr ].hhfield .lhfield = mem [r ].hhfield .lhfield ;
	    r = mem [r ].hhfield .v.RH ;
	  } 
	  mem [rr ].hhfield .v.RH = memtop ;
	  r = mem [p + 1 ].hhfield .lhfield ;
	  rr = memtop - 1 ;
	  while ( r > 1 ) {
	      
	    ss = getavail () ;
	    mem [rr ].hhfield .v.RH = ss ;
	    rr = ss ;
	    mem [rr ].hhfield .lhfield = mem [r ].hhfield .lhfield ;
	    r = mem [r ].hhfield .v.RH ;
	  } 
	  mem [rr ].hhfield .v.RH = r ;
	  mem [pp + 1 ].hhfield .lhfield = mem [memtop - 1 ].hhfield .v.RH 
	  ;
	} 
      while ( t++ < for_end ) ;} 
    } while ( ! ( mem [p ].hhfield .v.RH == curedges ) ) ;
    mem [curedges + 4 ].cint = 0 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zxscaleedges ( integer s ) 
#else
zxscaleedges ( s ) 
  integer s ;
#endif
{
  halfword p, q  ;
  unsigned short t  ;
  char w  ;
  integer delta  ;
  if ( ( s * ( mem [curedges + 2 ].hhfield .v.RH - 4096 ) >= 4096 ) || ( s * 
  ( mem [curedges + 2 ].hhfield .lhfield - 4096 ) <= -4096 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 261 ) ;
      print ( 534 ) ;
    } 
    {
      helpptr = 3 ;
      helpline [2 ]= 538 ;
      helpline [1 ]= 536 ;
      helpline [0 ]= 537 ;
    } 
    putgeterror () ;
  } 
  else if ( ( mem [curedges + 2 ].hhfield .v.RH != 4096 ) || ( mem [
  curedges + 2 ].hhfield .lhfield != 4096 ) ) 
  {
    mem [curedges + 2 ].hhfield .v.RH = s * ( mem [curedges + 2 ].hhfield 
    .v.RH - 4096 ) + 4096 ;
    mem [curedges + 2 ].hhfield .lhfield = s * ( mem [curedges + 2 ]
    .hhfield .lhfield - 4096 ) + 4096 ;
    delta = 8 * ( 4096 - s * mem [curedges + 3 ].hhfield .lhfield ) + 0 ;
    mem [curedges + 3 ].hhfield .lhfield = 4096 ;
    q = mem [curedges ].hhfield .v.RH ;
    do {
	p = mem [q + 1 ].hhfield .v.RH ;
      while ( p != memtop ) {
	  
	t = mem [p ].hhfield .lhfield ;
	w = t % 8 ;
	mem [p ].hhfield .lhfield = ( t - w ) * s + w + delta ;
	p = mem [p ].hhfield .v.RH ;
      } 
      p = mem [q + 1 ].hhfield .lhfield ;
      while ( p > 1 ) {
	  
	t = mem [p ].hhfield .lhfield ;
	w = t % 8 ;
	mem [p ].hhfield .lhfield = ( t - w ) * s + w + delta ;
	p = mem [p ].hhfield .v.RH ;
      } 
      q = mem [q ].hhfield .v.RH ;
    } while ( ! ( q == curedges ) ) ;
    mem [curedges + 4 ].cint = 0 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
znegateedges ( halfword h ) 
#else
znegateedges ( h ) 
  halfword h ;
#endif
{
  /* 30 */ halfword p, q, r, s, t, u  ;
  p = mem [h ].hhfield .v.RH ;
  while ( p != h ) {
      
    q = mem [p + 1 ].hhfield .lhfield ;
    while ( q > 1 ) {
	
      mem [q ].hhfield .lhfield = 8 - 2 * ( ( mem [q ].hhfield .lhfield ) 
      % 8 ) + mem [q ].hhfield .lhfield ;
      q = mem [q ].hhfield .v.RH ;
    } 
    q = mem [p + 1 ].hhfield .v.RH ;
    if ( q != memtop ) 
    {
      do {
	  mem [q ].hhfield .lhfield = 8 - 2 * ( ( mem [q ].hhfield 
	.lhfield ) % 8 ) + mem [q ].hhfield .lhfield ;
	q = mem [q ].hhfield .v.RH ;
      } while ( ! ( q == memtop ) ) ;
      u = p + 1 ;
      q = mem [u ].hhfield .v.RH ;
      r = q ;
      s = mem [r ].hhfield .v.RH ;
      while ( true ) if ( mem [s ].hhfield .lhfield > mem [r ].hhfield 
      .lhfield ) 
      {
	mem [u ].hhfield .v.RH = q ;
	if ( s == memtop ) 
	goto lab30 ;
	u = r ;
	q = s ;
	r = q ;
	s = mem [r ].hhfield .v.RH ;
      } 
      else {
	  
	t = s ;
	s = mem [t ].hhfield .v.RH ;
	mem [t ].hhfield .v.RH = q ;
	q = t ;
      } 
      lab30: mem [r ].hhfield .v.RH = memtop ;
    } 
    p = mem [p ].hhfield .v.RH ;
  } 
  mem [h + 4 ].cint = 0 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zsortedges ( halfword h ) 
#else
zsortedges ( h ) 
  halfword h ;
#endif
{
  /* 30 */ halfword k  ;
  halfword p, q, r, s  ;
  r = mem [h + 1 ].hhfield .lhfield ;
  mem [h + 1 ].hhfield .lhfield = 0 ;
  p = mem [r ].hhfield .v.RH ;
  mem [r ].hhfield .v.RH = memtop ;
  mem [memtop - 1 ].hhfield .v.RH = r ;
  while ( p > 1 ) {
      
    k = mem [p ].hhfield .lhfield ;
    q = memtop - 1 ;
    do {
	r = q ;
      q = mem [r ].hhfield .v.RH ;
    } while ( ! ( k <= mem [q ].hhfield .lhfield ) ) ;
    mem [r ].hhfield .v.RH = p ;
    r = mem [p ].hhfield .v.RH ;
    mem [p ].hhfield .v.RH = q ;
    p = r ;
  } 
  {
    r = h + 1 ;
    q = mem [r ].hhfield .v.RH ;
    p = mem [memtop - 1 ].hhfield .v.RH ;
    while ( true ) {
	
      k = mem [p ].hhfield .lhfield ;
      while ( k > mem [q ].hhfield .lhfield ) {
	  
	r = q ;
	q = mem [r ].hhfield .v.RH ;
      } 
      mem [r ].hhfield .v.RH = p ;
      s = mem [p ].hhfield .v.RH ;
      mem [p ].hhfield .v.RH = q ;
      if ( s == memtop ) 
      goto lab30 ;
      r = p ;
      p = s ;
    } 
    lab30: ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zculledges ( integer wlo , integer whi , integer wout , integer win ) 
#else
zculledges ( wlo , whi , wout , win ) 
  integer wlo ;
  integer whi ;
  integer wout ;
  integer win ;
#endif
{
  /* 30 */ halfword p, q, r, s  ;
  integer w  ;
  integer d  ;
  integer m  ;
  integer mm  ;
  integer ww  ;
  integer prevw  ;
  halfword n, minn, maxn  ;
  halfword mind, maxd  ;
  mind = 268435455L ;
  maxd = 0 ;
  minn = 268435455L ;
  maxn = 0 ;
  p = mem [curedges ].hhfield .v.RH ;
  n = mem [curedges + 1 ].hhfield .lhfield ;
  while ( p != curedges ) {
      
    if ( mem [p + 1 ].hhfield .lhfield > 1 ) 
    sortedges ( p ) ;
    if ( mem [p + 1 ].hhfield .v.RH != memtop ) 
    {
      r = memtop - 1 ;
      q = mem [p + 1 ].hhfield .v.RH ;
      ww = 0 ;
      m = 1000000L ;
      prevw = 0 ;
      while ( true ) {
	  
	if ( q == memtop ) 
	mm = 1000000L ;
	else {
	    
	  d = mem [q ].hhfield .lhfield ;
	  mm = d / 8 ;
	  ww = ww + ( d % 8 ) - 4 ;
	} 
	if ( mm > m ) 
	{
	  if ( w != prevw ) 
	  {
	    s = getavail () ;
	    mem [r ].hhfield .v.RH = s ;
	    mem [s ].hhfield .lhfield = 8 * m + 4 + w - prevw ;
	    r = s ;
	    prevw = w ;
	  } 
	  if ( q == memtop ) 
	  goto lab30 ;
	} 
	m = mm ;
	if ( ww >= wlo ) 
	if ( ww <= whi ) 
	w = win ;
	else w = wout ;
	else w = wout ;
	s = mem [q ].hhfield .v.RH ;
	{
	  mem [q ].hhfield .v.RH = avail ;
	  avail = q ;
	;
#ifdef STAT
	  decr ( dynused ) ;
#endif /* STAT */
	} 
	q = s ;
      } 
      lab30: mem [r ].hhfield .v.RH = memtop ;
      mem [p + 1 ].hhfield .v.RH = mem [memtop - 1 ].hhfield .v.RH ;
      if ( r != memtop - 1 ) 
      {
	if ( minn == 268435455L ) 
	minn = n ;
	maxn = n ;
	if ( mind > mem [mem [memtop - 1 ].hhfield .v.RH ].hhfield 
	.lhfield ) 
	mind = mem [mem [memtop - 1 ].hhfield .v.RH ].hhfield .lhfield ;
	if ( maxd < mem [r ].hhfield .lhfield ) 
	maxd = mem [r ].hhfield .lhfield ;
      } 
    } 
    p = mem [p ].hhfield .v.RH ;
    incr ( n ) ;
  } 
  if ( minn > maxn ) 
  {
    p = mem [curedges ].hhfield .v.RH ;
    while ( p != curedges ) {
	
      q = mem [p ].hhfield .v.RH ;
      freenode ( p , 2 ) ;
      p = q ;
    } 
    initedges ( curedges ) ;
  } 
  else {
      
    n = mem [curedges + 1 ].hhfield .lhfield ;
    mem [curedges + 1 ].hhfield .lhfield = minn ;
    while ( minn > n ) {
	
      p = mem [curedges ].hhfield .v.RH ;
      mem [curedges ].hhfield .v.RH = mem [p ].hhfield .v.RH ;
      mem [mem [p ].hhfield .v.RH ].hhfield .lhfield = curedges ;
      freenode ( p , 2 ) ;
      incr ( n ) ;
    } 
    n = mem [curedges + 1 ].hhfield .v.RH ;
    mem [curedges + 1 ].hhfield .v.RH = maxn ;
    mem [curedges + 5 ].hhfield .lhfield = maxn + 1 ;
    mem [curedges + 5 ].hhfield .v.RH = curedges ;
    while ( maxn < n ) {
	
      p = mem [curedges ].hhfield .lhfield ;
      mem [curedges ].hhfield .lhfield = mem [p ].hhfield .lhfield ;
      mem [mem [p ].hhfield .lhfield ].hhfield .v.RH = curedges ;
      freenode ( p , 2 ) ;
      decr ( n ) ;
    } 
    mem [curedges + 2 ].hhfield .lhfield = ( ( mind ) / 8 ) - mem [curedges 
    + 3 ].hhfield .lhfield + 4096 ;
    mem [curedges + 2 ].hhfield .v.RH = ( ( maxd ) / 8 ) - mem [curedges + 
    3 ].hhfield .lhfield + 4096 ;
  } 
  mem [curedges + 4 ].cint = 0 ;
} 
void 
#ifdef HAVE_PROTOTYPES
xyswapedges ( void ) 
#else
xyswapedges ( ) 
#endif
{
  /* 30 */ integer mmagic, nmagic  ;
  halfword p, q, r, s  ;
  integer mspread  ;
  integer j, jj  ;
  integer m, mm  ;
  integer pd, rd  ;
  integer pm, rm  ;
  integer w  ;
  integer ww  ;
  integer dw  ;
  integer extras  ;
  schar xw  ;
  integer k  ;
  mspread = mem [curedges + 2 ].hhfield .v.RH - mem [curedges + 2 ]
  .hhfield .lhfield ;
  if ( mspread > movesize ) 
  overflow ( 539 , movesize ) ;
  {register integer for_end; j = 0 ;for_end = mspread ; if ( j <= for_end) 
  do 
    move [j ]= memtop ;
  while ( j++ < for_end ) ;} 
  p = getnode ( 2 ) ;
  mem [p + 1 ].hhfield .v.RH = memtop ;
  mem [p + 1 ].hhfield .lhfield = 0 ;
  mem [p ].hhfield .lhfield = curedges ;
  mem [mem [curedges ].hhfield .v.RH ].hhfield .lhfield = p ;
  p = getnode ( 2 ) ;
  mem [p + 1 ].hhfield .v.RH = memtop ;
  mem [p ].hhfield .lhfield = mem [curedges ].hhfield .lhfield ;
  mmagic = mem [curedges + 2 ].hhfield .lhfield + mem [curedges + 3 ]
  .hhfield .lhfield - 4096 ;
  nmagic = 8 * mem [curedges + 1 ].hhfield .v.RH + 12 ;
  do {
      q = mem [p ].hhfield .lhfield ;
    if ( mem [q + 1 ].hhfield .lhfield > 1 ) 
    sortedges ( q ) ;
    r = mem [p + 1 ].hhfield .v.RH ;
    freenode ( p , 2 ) ;
    p = r ;
    pd = mem [p ].hhfield .lhfield ;
    pm = pd / 8 ;
    r = mem [q + 1 ].hhfield .v.RH ;
    rd = mem [r ].hhfield .lhfield ;
    rm = rd / 8 ;
    w = 0 ;
    while ( true ) {
	
      if ( pm < rm ) 
      mm = pm ;
      else mm = rm ;
      if ( w != 0 ) 
      if ( m != mm ) 
      {
	if ( mm - mmagic >= movesize ) 
	confusion ( 509 ) ;
	extras = ( abs ( w ) - 1 ) / 3 ;
	if ( extras > 0 ) 
	{
	  if ( w > 0 ) 
	  xw = 3 ;
	  else xw = -3 ;
	  ww = w - extras * xw ;
	} 
	else ww = w ;
	do {
	    j = m - mmagic ;
	  {register integer for_end; k = 1 ;for_end = extras ; if ( k <= 
	  for_end) do 
	    {
	      s = getavail () ;
	      mem [s ].hhfield .lhfield = nmagic + xw ;
	      mem [s ].hhfield .v.RH = move [j ];
	      move [j ]= s ;
	    } 
	  while ( k++ < for_end ) ;} 
	  s = getavail () ;
	  mem [s ].hhfield .lhfield = nmagic + ww ;
	  mem [s ].hhfield .v.RH = move [j ];
	  move [j ]= s ;
	  incr ( m ) ;
	} while ( ! ( m == mm ) ) ;
      } 
      if ( pd < rd ) 
      {
	dw = ( pd % 8 ) - 4 ;
	s = mem [p ].hhfield .v.RH ;
	{
	  mem [p ].hhfield .v.RH = avail ;
	  avail = p ;
	;
#ifdef STAT
	  decr ( dynused ) ;
#endif /* STAT */
	} 
	p = s ;
	pd = mem [p ].hhfield .lhfield ;
	pm = pd / 8 ;
      } 
      else {
	  
	if ( r == memtop ) 
	goto lab30 ;
	dw = - (integer) ( ( rd % 8 ) - 4 ) ;
	r = mem [r ].hhfield .v.RH ;
	rd = mem [r ].hhfield .lhfield ;
	rm = rd / 8 ;
      } 
      m = mm ;
      w = w + dw ;
    } 
    lab30: ;
    p = q ;
    nmagic = nmagic - 8 ;
  } while ( ! ( mem [p ].hhfield .lhfield == curedges ) ) ;
  freenode ( p , 2 ) ;
  move [mspread ]= 0 ;
  j = 0 ;
  while ( move [j ]== memtop ) incr ( j ) ;
  if ( j == mspread ) 
  initedges ( curedges ) ;
  else {
      
    mm = mem [curedges + 2 ].hhfield .lhfield ;
    mem [curedges + 2 ].hhfield .lhfield = mem [curedges + 1 ].hhfield 
    .lhfield ;
    mem [curedges + 2 ].hhfield .v.RH = mem [curedges + 1 ].hhfield .v.RH 
    + 1 ;
    mem [curedges + 3 ].hhfield .lhfield = 4096 ;
    jj = mspread - 1 ;
    while ( move [jj ]== memtop ) decr ( jj ) ;
    mem [curedges + 1 ].hhfield .lhfield = j + mm ;
    mem [curedges + 1 ].hhfield .v.RH = jj + mm ;
    q = curedges ;
    do {
	p = getnode ( 2 ) ;
      mem [q ].hhfield .v.RH = p ;
      mem [p ].hhfield .lhfield = q ;
      mem [p + 1 ].hhfield .v.RH = move [j ];
      mem [p + 1 ].hhfield .lhfield = 0 ;
      incr ( j ) ;
      q = p ;
    } while ( ! ( j > jj ) ) ;
    mem [q ].hhfield .v.RH = curedges ;
    mem [curedges ].hhfield .lhfield = q ;
    mem [curedges + 5 ].hhfield .lhfield = mem [curedges + 1 ].hhfield 
    .v.RH + 1 ;
    mem [curedges + 5 ].hhfield .v.RH = curedges ;
    mem [curedges + 4 ].cint = 0 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zmergeedges ( halfword h ) 
#else
zmergeedges ( h ) 
  halfword h ;
#endif
{
  /* 30 */ halfword p, q, r, pp, qq, rr  ;
  integer n  ;
  halfword k  ;
  integer delta  ;
  if ( mem [h ].hhfield .v.RH != h ) 
  {
    if ( ( mem [h + 2 ].hhfield .lhfield < mem [curedges + 2 ].hhfield 
    .lhfield ) || ( mem [h + 2 ].hhfield .v.RH > mem [curedges + 2 ]
    .hhfield .v.RH ) || ( mem [h + 1 ].hhfield .lhfield < mem [curedges + 1 
    ].hhfield .lhfield ) || ( mem [h + 1 ].hhfield .v.RH > mem [curedges + 
    1 ].hhfield .v.RH ) ) 
    edgeprep ( mem [h + 2 ].hhfield .lhfield - 4096 , mem [h + 2 ].hhfield 
    .v.RH - 4096 , mem [h + 1 ].hhfield .lhfield - 4096 , mem [h + 1 ]
    .hhfield .v.RH - 4095 ) ;
    if ( mem [h + 3 ].hhfield .lhfield != mem [curedges + 3 ].hhfield 
    .lhfield ) 
    {
      pp = mem [h ].hhfield .v.RH ;
      delta = 8 * ( mem [curedges + 3 ].hhfield .lhfield - mem [h + 3 ]
      .hhfield .lhfield ) ;
      do {
	  qq = mem [pp + 1 ].hhfield .v.RH ;
	while ( qq != memtop ) {
	    
	  mem [qq ].hhfield .lhfield = mem [qq ].hhfield .lhfield + delta 
	  ;
	  qq = mem [qq ].hhfield .v.RH ;
	} 
	qq = mem [pp + 1 ].hhfield .lhfield ;
	while ( qq > 1 ) {
	    
	  mem [qq ].hhfield .lhfield = mem [qq ].hhfield .lhfield + delta 
	  ;
	  qq = mem [qq ].hhfield .v.RH ;
	} 
	pp = mem [pp ].hhfield .v.RH ;
      } while ( ! ( pp == h ) ) ;
    } 
    n = mem [curedges + 1 ].hhfield .lhfield ;
    p = mem [curedges ].hhfield .v.RH ;
    pp = mem [h ].hhfield .v.RH ;
    while ( n < mem [h + 1 ].hhfield .lhfield ) {
	
      incr ( n ) ;
      p = mem [p ].hhfield .v.RH ;
    } 
    do {
	qq = mem [pp + 1 ].hhfield .lhfield ;
      if ( qq > 1 ) 
      if ( mem [p + 1 ].hhfield .lhfield <= 1 ) 
      mem [p + 1 ].hhfield .lhfield = qq ;
      else {
	  
	while ( mem [qq ].hhfield .v.RH > 1 ) qq = mem [qq ].hhfield .v.RH 
	;
	mem [qq ].hhfield .v.RH = mem [p + 1 ].hhfield .lhfield ;
	mem [p + 1 ].hhfield .lhfield = mem [pp + 1 ].hhfield .lhfield ;
      } 
      mem [pp + 1 ].hhfield .lhfield = 0 ;
      qq = mem [pp + 1 ].hhfield .v.RH ;
      if ( qq != memtop ) 
      {
	if ( mem [p + 1 ].hhfield .lhfield == 1 ) 
	mem [p + 1 ].hhfield .lhfield = 0 ;
	mem [pp + 1 ].hhfield .v.RH = memtop ;
	r = p + 1 ;
	q = mem [r ].hhfield .v.RH ;
	if ( q == memtop ) 
	mem [p + 1 ].hhfield .v.RH = qq ;
	else while ( true ) {
	    
	  k = mem [qq ].hhfield .lhfield ;
	  while ( k > mem [q ].hhfield .lhfield ) {
	      
	    r = q ;
	    q = mem [r ].hhfield .v.RH ;
	  } 
	  mem [r ].hhfield .v.RH = qq ;
	  rr = mem [qq ].hhfield .v.RH ;
	  mem [qq ].hhfield .v.RH = q ;
	  if ( rr == memtop ) 
	  goto lab30 ;
	  r = qq ;
	  qq = rr ;
	} 
      } 
      lab30: ;
      pp = mem [pp ].hhfield .v.RH ;
      p = mem [p ].hhfield .v.RH ;
    } while ( ! ( pp == h ) ) ;
  } 
} 
integer 
#ifdef HAVE_PROTOTYPES
ztotalweight ( halfword h ) 
#else
ztotalweight ( h ) 
  halfword h ;
#endif
{
  register integer Result; halfword p, q  ;
  integer n  ;
  unsigned short m  ;
  n = 0 ;
  p = mem [h ].hhfield .v.RH ;
  while ( p != h ) {
      
    q = mem [p + 1 ].hhfield .v.RH ;
    while ( q != memtop ) {
	
      m = mem [q ].hhfield .lhfield ;
      n = n - ( ( m % 8 ) - 4 ) * ( m / 8 ) ;
      q = mem [q ].hhfield .v.RH ;
    } 
    q = mem [p + 1 ].hhfield .lhfield ;
    while ( q > 1 ) {
	
      m = mem [q ].hhfield .lhfield ;
      n = n - ( ( m % 8 ) - 4 ) * ( m / 8 ) ;
      q = mem [q ].hhfield .v.RH ;
    } 
    p = mem [p ].hhfield .v.RH ;
  } 
  Result = n ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
beginedgetracing ( void ) 
#else
beginedgetracing ( ) 
#endif
{
  printdiagnostic ( 540 , 283 , true ) ;
  print ( 541 ) ;
  printint ( curwt ) ;
  printchar ( 41 ) ;
  tracex = -4096 ;
} 
void 
#ifdef HAVE_PROTOTYPES
traceacorner ( void ) 
#else
traceacorner ( ) 
#endif
{
  if ( fileoffset > maxprintline - 13 ) 
  printnl ( 283 ) ;
  printchar ( 40 ) ;
  printint ( tracex ) ;
  printchar ( 44 ) ;
  printint ( traceyy ) ;
  printchar ( 41 ) ;
  tracey = traceyy ;
} 
void 
#ifdef HAVE_PROTOTYPES
endedgetracing ( void ) 
#else
endedgetracing ( ) 
#endif
{
  if ( tracex == -4096 ) 
  printnl ( 542 ) ;
  else {
      
    traceacorner () ;
    printchar ( 46 ) ;
  } 
  enddiagnostic ( true ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
ztracenewedge ( halfword r , integer n ) 
#else
ztracenewedge ( r , n ) 
  halfword r ;
  integer n ;
#endif
{
  integer d  ;
  schar w  ;
  integer m, n0, n1  ;
  d = mem [r ].hhfield .lhfield ;
  w = ( d % 8 ) - 4 ;
  m = ( d / 8 ) - mem [curedges + 3 ].hhfield .lhfield ;
  if ( w == curwt ) 
  {
    n0 = n + 1 ;
    n1 = n ;
  } 
  else {
      
    n0 = n ;
    n1 = n + 1 ;
  } 
  if ( m != tracex ) 
  {
    if ( tracex == -4096 ) 
    {
      printnl ( 283 ) ;
      traceyy = n0 ;
    } 
    else if ( traceyy != n0 ) 
    printchar ( 63 ) ;
    else traceacorner () ;
    tracex = m ;
    traceacorner () ;
  } 
  else {
      
    if ( n0 != traceyy ) 
    printchar ( 33 ) ;
    if ( ( ( n0 < n1 ) && ( tracey > traceyy ) ) || ( ( n0 > n1 ) && ( tracey 
    < traceyy ) ) ) 
    traceacorner () ;
  } 
  traceyy = n1 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zlineedges ( scaled x0 , scaled y0 , scaled x1 , scaled y1 ) 
#else
zlineedges ( x0 , y0 , x1 , y1 ) 
  scaled x0 ;
  scaled y0 ;
  scaled x1 ;
  scaled y1 ;
#endif
{
  /* 30 31 */ integer m0, n0, m1, n1  ;
  scaled delx, dely  ;
  scaled yt  ;
  scaled tx  ;
  halfword p, r  ;
  integer base  ;
  integer n  ;
  n0 = roundunscaled ( y0 ) ;
  n1 = roundunscaled ( y1 ) ;
  if ( n0 != n1 ) 
  {
    m0 = roundunscaled ( x0 ) ;
    m1 = roundunscaled ( x1 ) ;
    delx = x1 - x0 ;
    dely = y1 - y0 ;
    yt = n0 * 65536L - 32768L ;
    y0 = y0 - yt ;
    y1 = y1 - yt ;
    if ( n0 < n1 ) 
    {
      base = 8 * mem [curedges + 3 ].hhfield .lhfield + 4 - curwt ;
      if ( m0 <= m1 ) 
      edgeprep ( m0 , m1 , n0 , n1 ) ;
      else edgeprep ( m1 , m0 , n0 , n1 ) ;
      n = mem [curedges + 5 ].hhfield .lhfield - 4096 ;
      p = mem [curedges + 5 ].hhfield .v.RH ;
      if ( n != n0 ) 
      if ( n < n0 ) 
      do {
	  incr ( n ) ;
	p = mem [p ].hhfield .v.RH ;
      } while ( ! ( n == n0 ) ) ;
      else do {
	  decr ( n ) ;
	p = mem [p ].hhfield .lhfield ;
      } while ( ! ( n == n0 ) ) ;
      y0 = 65536L - y0 ;
      while ( true ) {
	  
	r = getavail () ;
	mem [r ].hhfield .v.RH = mem [p + 1 ].hhfield .lhfield ;
	mem [p + 1 ].hhfield .lhfield = r ;
	tx = takefraction ( delx , makefraction ( y0 , dely ) ) ;
	if ( abvscd ( delx , y0 , dely , tx ) < 0 ) 
	decr ( tx ) ;
	mem [r ].hhfield .lhfield = 8 * roundunscaled ( x0 + tx ) + base ;
	y1 = y1 - 65536L ;
	if ( internal [10 ]> 0 ) 
	tracenewedge ( r , n ) ;
	if ( y1 < 65536L ) 
	goto lab30 ;
	p = mem [p ].hhfield .v.RH ;
	y0 = y0 + 65536L ;
	incr ( n ) ;
      } 
      lab30: ;
    } 
    else {
	
      base = 8 * mem [curedges + 3 ].hhfield .lhfield + 4 + curwt ;
      if ( m0 <= m1 ) 
      edgeprep ( m0 , m1 , n1 , n0 ) ;
      else edgeprep ( m1 , m0 , n1 , n0 ) ;
      decr ( n0 ) ;
      n = mem [curedges + 5 ].hhfield .lhfield - 4096 ;
      p = mem [curedges + 5 ].hhfield .v.RH ;
      if ( n != n0 ) 
      if ( n < n0 ) 
      do {
	  incr ( n ) ;
	p = mem [p ].hhfield .v.RH ;
      } while ( ! ( n == n0 ) ) ;
      else do {
	  decr ( n ) ;
	p = mem [p ].hhfield .lhfield ;
      } while ( ! ( n == n0 ) ) ;
      while ( true ) {
	  
	r = getavail () ;
	mem [r ].hhfield .v.RH = mem [p + 1 ].hhfield .lhfield ;
	mem [p + 1 ].hhfield .lhfield = r ;
	tx = takefraction ( delx , makefraction ( y0 , dely ) ) ;
	if ( abvscd ( delx , y0 , dely , tx ) < 0 ) 
	incr ( tx ) ;
	mem [r ].hhfield .lhfield = 8 * roundunscaled ( x0 - tx ) + base ;
	y1 = y1 + 65536L ;
	if ( internal [10 ]> 0 ) 
	tracenewedge ( r , n ) ;
	if ( y1 >= 0 ) 
	goto lab31 ;
	p = mem [p ].hhfield .lhfield ;
	y0 = y0 + 65536L ;
	decr ( n ) ;
      } 
      lab31: ;
    } 
    mem [curedges + 5 ].hhfield .v.RH = p ;
    mem [curedges + 5 ].hhfield .lhfield = n + 4096 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zmovetoedges ( integer m0 , integer n0 , integer m1 , integer n1 ) 
#else
zmovetoedges ( m0 , n0 , m1 , n1 ) 
  integer m0 ;
  integer n0 ;
  integer m1 ;
  integer n1 ;
#endif
{
  /* 60 61 62 63 30 */ integer delta  ;
  integer k  ;
  halfword p, r  ;
  integer dx  ;
  integer edgeandweight  ;
  integer j  ;
  integer n  ;
#ifdef TEXMF_DEBUG
  integer sum  ;
#endif /* TEXMF_DEBUG */
  delta = n1 - n0 ;
	;
#ifdef TEXMF_DEBUG
  sum = move [0 ];
  {register integer for_end; k = 1 ;for_end = delta ; if ( k <= for_end) do 
    sum = sum + abs ( move [k ]) ;
  while ( k++ < for_end ) ;} 
  if ( sum != m1 - m0 ) 
  confusion ( 48 ) ;
#endif /* TEXMF_DEBUG */
  switch ( octant ) 
  {case 1 : 
    {
      dx = 8 ;
      edgeprep ( m0 , m1 , n0 , n1 ) ;
      goto lab60 ;
    } 
    break ;
  case 5 : 
    {
      dx = 8 ;
      edgeprep ( n0 , n1 , m0 , m1 ) ;
      goto lab62 ;
    } 
    break ;
  case 6 : 
    {
      dx = -8 ;
      edgeprep ( - (integer) n1 , - (integer) n0 , m0 , m1 ) ;
      n0 = - (integer) n0 ;
      goto lab62 ;
    } 
    break ;
  case 2 : 
    {
      dx = -8 ;
      edgeprep ( - (integer) m1 , - (integer) m0 , n0 , n1 ) ;
      m0 = - (integer) m0 ;
      goto lab60 ;
    } 
    break ;
  case 4 : 
    {
      dx = -8 ;
      edgeprep ( - (integer) m1 , - (integer) m0 , - (integer) n1 , 
      - (integer) n0 ) ;
      m0 = - (integer) m0 ;
      goto lab61 ;
    } 
    break ;
  case 8 : 
    {
      dx = -8 ;
      edgeprep ( - (integer) n1 , - (integer) n0 , - (integer) m1 , 
      - (integer) m0 ) ;
      n0 = - (integer) n0 ;
      goto lab63 ;
    } 
    break ;
  case 7 : 
    {
      dx = 8 ;
      edgeprep ( n0 , n1 , - (integer) m1 , - (integer) m0 ) ;
      goto lab63 ;
    } 
    break ;
  case 3 : 
    {
      dx = 8 ;
      edgeprep ( m0 , m1 , - (integer) n1 , - (integer) n0 ) ;
      goto lab61 ;
    } 
    break ;
  } 
  lab60: n = mem [curedges + 5 ].hhfield .lhfield - 4096 ;
  p = mem [curedges + 5 ].hhfield .v.RH ;
  if ( n != n0 ) 
  if ( n < n0 ) 
  do {
      incr ( n ) ;
    p = mem [p ].hhfield .v.RH ;
  } while ( ! ( n == n0 ) ) ;
  else do {
      decr ( n ) ;
    p = mem [p ].hhfield .lhfield ;
  } while ( ! ( n == n0 ) ) ;
  if ( delta > 0 ) 
  {
    k = 0 ;
    edgeandweight = 8 * ( m0 + mem [curedges + 3 ].hhfield .lhfield ) + 4 - 
    curwt ;
    do {
	edgeandweight = edgeandweight + dx * move [k ];
      {
	r = avail ;
	if ( r == 0 ) 
	r = getavail () ;
	else {
	    
	  avail = mem [r ].hhfield .v.RH ;
	  mem [r ].hhfield .v.RH = 0 ;
	;
#ifdef STAT
	  incr ( dynused ) ;
#endif /* STAT */
	} 
      } 
      mem [r ].hhfield .v.RH = mem [p + 1 ].hhfield .lhfield ;
      mem [r ].hhfield .lhfield = edgeandweight ;
      if ( internal [10 ]> 0 ) 
      tracenewedge ( r , n ) ;
      mem [p + 1 ].hhfield .lhfield = r ;
      p = mem [p ].hhfield .v.RH ;
      incr ( k ) ;
      incr ( n ) ;
    } while ( ! ( k == delta ) ) ;
  } 
  goto lab30 ;
  lab61: n0 = - (integer) n0 - 1 ;
  n = mem [curedges + 5 ].hhfield .lhfield - 4096 ;
  p = mem [curedges + 5 ].hhfield .v.RH ;
  if ( n != n0 ) 
  if ( n < n0 ) 
  do {
      incr ( n ) ;
    p = mem [p ].hhfield .v.RH ;
  } while ( ! ( n == n0 ) ) ;
  else do {
      decr ( n ) ;
    p = mem [p ].hhfield .lhfield ;
  } while ( ! ( n == n0 ) ) ;
  if ( delta > 0 ) 
  {
    k = 0 ;
    edgeandweight = 8 * ( m0 + mem [curedges + 3 ].hhfield .lhfield ) + 4 + 
    curwt ;
    do {
	edgeandweight = edgeandweight + dx * move [k ];
      {
	r = avail ;
	if ( r == 0 ) 
	r = getavail () ;
	else {
	    
	  avail = mem [r ].hhfield .v.RH ;
	  mem [r ].hhfield .v.RH = 0 ;
	;
#ifdef STAT
	  incr ( dynused ) ;
#endif /* STAT */
	} 
      } 
      mem [r ].hhfield .v.RH = mem [p + 1 ].hhfield .lhfield ;
      mem [r ].hhfield .lhfield = edgeandweight ;
      if ( internal [10 ]> 0 ) 
      tracenewedge ( r , n ) ;
      mem [p + 1 ].hhfield .lhfield = r ;
      p = mem [p ].hhfield .lhfield ;
      incr ( k ) ;
      decr ( n ) ;
    } while ( ! ( k == delta ) ) ;
  } 
  goto lab30 ;
  lab62: edgeandweight = 8 * ( n0 + mem [curedges + 3 ].hhfield .lhfield ) + 
  4 - curwt ;
  n0 = m0 ;
  k = 0 ;
  n = mem [curedges + 5 ].hhfield .lhfield - 4096 ;
  p = mem [curedges + 5 ].hhfield .v.RH ;
  if ( n != n0 ) 
  if ( n < n0 ) 
  do {
      incr ( n ) ;
    p = mem [p ].hhfield .v.RH ;
  } while ( ! ( n == n0 ) ) ;
  else do {
      decr ( n ) ;
    p = mem [p ].hhfield .lhfield ;
  } while ( ! ( n == n0 ) ) ;
  do {
      j = move [k ];
    while ( j > 0 ) {
	
      {
	r = avail ;
	if ( r == 0 ) 
	r = getavail () ;
	else {
	    
	  avail = mem [r ].hhfield .v.RH ;
	  mem [r ].hhfield .v.RH = 0 ;
	;
#ifdef STAT
	  incr ( dynused ) ;
#endif /* STAT */
	} 
      } 
      mem [r ].hhfield .v.RH = mem [p + 1 ].hhfield .lhfield ;
      mem [r ].hhfield .lhfield = edgeandweight ;
      if ( internal [10 ]> 0 ) 
      tracenewedge ( r , n ) ;
      mem [p + 1 ].hhfield .lhfield = r ;
      p = mem [p ].hhfield .v.RH ;
      decr ( j ) ;
      incr ( n ) ;
    } 
    edgeandweight = edgeandweight + dx ;
    incr ( k ) ;
  } while ( ! ( k > delta ) ) ;
  goto lab30 ;
  lab63: edgeandweight = 8 * ( n0 + mem [curedges + 3 ].hhfield .lhfield ) + 
  4 + curwt ;
  n0 = - (integer) m0 - 1 ;
  k = 0 ;
  n = mem [curedges + 5 ].hhfield .lhfield - 4096 ;
  p = mem [curedges + 5 ].hhfield .v.RH ;
  if ( n != n0 ) 
  if ( n < n0 ) 
  do {
      incr ( n ) ;
    p = mem [p ].hhfield .v.RH ;
  } while ( ! ( n == n0 ) ) ;
  else do {
      decr ( n ) ;
    p = mem [p ].hhfield .lhfield ;
  } while ( ! ( n == n0 ) ) ;
  do {
      j = move [k ];
    while ( j > 0 ) {
	
      {
	r = avail ;
	if ( r == 0 ) 
	r = getavail () ;
	else {
	    
	  avail = mem [r ].hhfield .v.RH ;
	  mem [r ].hhfield .v.RH = 0 ;
	;
#ifdef STAT
	  incr ( dynused ) ;
#endif /* STAT */
	} 
      } 
      mem [r ].hhfield .v.RH = mem [p + 1 ].hhfield .lhfield ;
      mem [r ].hhfield .lhfield = edgeandweight ;
      if ( internal [10 ]> 0 ) 
      tracenewedge ( r , n ) ;
      mem [p + 1 ].hhfield .lhfield = r ;
      p = mem [p ].hhfield .lhfield ;
      decr ( j ) ;
      decr ( n ) ;
    } 
    edgeandweight = edgeandweight + dx ;
    incr ( k ) ;
  } while ( ! ( k > delta ) ) ;
  goto lab30 ;
  lab30: mem [curedges + 5 ].hhfield .lhfield = n + 4096 ;
  mem [curedges + 5 ].hhfield .v.RH = p ;
} 
void 
#ifdef HAVE_PROTOTYPES
zskew ( scaled x , scaled y , smallnumber octant ) 
#else
zskew ( x , y , octant ) 
  scaled x ;
  scaled y ;
  smallnumber octant ;
#endif
{
  switch ( octant ) 
  {case 1 : 
    {
      curx = x - y ;
      cury = y ;
    } 
    break ;
  case 5 : 
    {
      curx = y - x ;
      cury = x ;
    } 
    break ;
  case 6 : 
    {
      curx = y + x ;
      cury = - (integer) x ;
    } 
    break ;
  case 2 : 
    {
      curx = - (integer) x - y ;
      cury = y ;
    } 
    break ;
  case 4 : 
    {
      curx = - (integer) x + y ;
      cury = - (integer) y ;
    } 
    break ;
  case 8 : 
    {
      curx = - (integer) y + x ;
      cury = - (integer) x ;
    } 
    break ;
  case 7 : 
    {
      curx = - (integer) y - x ;
      cury = x ;
    } 
    break ;
  case 3 : 
    {
      curx = x + y ;
      cury = - (integer) y ;
    } 
    break ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zabnegate ( scaled x , scaled y , smallnumber octantbefore , smallnumber 
octantafter ) 
#else
zabnegate ( x , y , octantbefore , octantafter ) 
  scaled x ;
  scaled y ;
  smallnumber octantbefore ;
  smallnumber octantafter ;
#endif
{
  if ( odd ( octantbefore ) == odd ( octantafter ) ) 
  curx = x ;
  else curx = - (integer) x ;
  if ( ( octantbefore > 2 ) == ( octantafter > 2 ) ) 
  cury = y ;
  else cury = - (integer) y ;
} 
fraction 
#ifdef HAVE_PROTOTYPES
zcrossingpoint ( integer a , integer b , integer c ) 
#else
zcrossingpoint ( a , b , c ) 
  integer a ;
  integer b ;
  integer c ;
#endif
{
  /* 10 */ register fraction Result; integer d  ;
  integer x, xx, x0, x1, x2  ;
  if ( a < 0 ) 
  {
    Result = 0 ;
    goto lab10 ;
  } 
  if ( c >= 0 ) 
  {
    if ( b >= 0 ) 
    if ( c > 0 ) 
    {
      Result = 268435457L ;
      goto lab10 ;
    } 
    else if ( ( a == 0 ) && ( b == 0 ) ) 
    {
      Result = 268435457L ;
      goto lab10 ;
    } 
    else {
	
      Result = 268435456L ;
      goto lab10 ;
    } 
    if ( a == 0 ) 
    {
      Result = 0 ;
      goto lab10 ;
    } 
  } 
  else if ( a == 0 ) 
  if ( b <= 0 ) 
  {
    Result = 0 ;
    goto lab10 ;
  } 
  d = 1 ;
  x0 = a ;
  x1 = a - b ;
  x2 = b - c ;
  do {
      x = half ( x1 + x2 ) ;
    if ( x1 - x0 > x0 ) 
    {
      x2 = x ;
      x0 = x0 + x0 ;
      d = d + d ;
    } 
    else {
	
      xx = x1 + x - x0 ;
      if ( xx > x0 ) 
      {
	x2 = x ;
	x0 = x0 + x0 ;
	d = d + d ;
      } 
      else {
	  
	x0 = x0 - xx ;
	if ( x <= x0 ) 
	if ( x + x2 <= x0 ) 
	{
	  Result = 268435457L ;
	  goto lab10 ;
	} 
	x1 = x ;
	d = d + d + 1 ;
      } 
    } 
  } while ( ! ( d >= 268435456L ) ) ;
  Result = d - 268435456L ;
  lab10: ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintspec ( strnumber s ) 
#else
zprintspec ( s ) 
  strnumber s ;
#endif
{
  /* 45 30 */ halfword p, q  ;
  smallnumber octant  ;
  printdiagnostic ( 543 , s , true ) ;
  p = curspec ;
  octant = mem [p + 3 ].cint ;
  println () ;
  unskew ( mem [curspec + 1 ].cint , mem [curspec + 2 ].cint , octant ) ;
  printtwo ( curx , cury ) ;
  print ( 544 ) ;
  while ( true ) {
      
    print ( octantdir [octant ]) ;
    printchar ( 39 ) ;
    while ( true ) {
	
      q = mem [p ].hhfield .v.RH ;
      if ( mem [p ].hhfield .b1 == 0 ) 
      goto lab45 ;
      {
	printnl ( 555 ) ;
	unskew ( mem [p + 5 ].cint , mem [p + 6 ].cint , octant ) ;
	printtwo ( curx , cury ) ;
	print ( 522 ) ;
	unskew ( mem [q + 3 ].cint , mem [q + 4 ].cint , octant ) ;
	printtwo ( curx , cury ) ;
	printnl ( 519 ) ;
	unskew ( mem [q + 1 ].cint , mem [q + 2 ].cint , octant ) ;
	printtwo ( curx , cury ) ;
	print ( 556 ) ;
	printint ( mem [q ].hhfield .b0 - 1 ) ;
      } 
      p = q ;
    } 
    lab45: if ( q == curspec ) 
    goto lab30 ;
    p = q ;
    octant = mem [p + 3 ].cint ;
    printnl ( 545 ) ;
  } 
  lab30: printnl ( 546 ) ;
  enddiagnostic ( true ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintstrange ( strnumber s ) 
#else
zprintstrange ( s ) 
  strnumber s ;
#endif
{
  halfword p  ;
  halfword f  ;
  halfword q  ;
  integer t  ;
  if ( interaction == 3 ) 
  ;
  printnl ( 62 ) ;
  p = curspec ;
  t = 256 ;
  do {
      p = mem [p ].hhfield .v.RH ;
    if ( mem [p ].hhfield .b0 != 0 ) 
    {
      if ( mem [p ].hhfield .b0 < t ) 
      f = p ;
      t = mem [p ].hhfield .b0 ;
    } 
  } while ( ! ( p == curspec ) ) ;
  p = curspec ;
  q = p ;
  do {
      p = mem [p ].hhfield .v.RH ;
    if ( mem [p ].hhfield .b0 == 0 ) 
    q = p ;
  } while ( ! ( p == f ) ) ;
  t = 0 ;
  do {
      if ( mem [p ].hhfield .b0 != 0 ) 
    {
      if ( mem [p ].hhfield .b0 != t ) 
      {
	t = mem [p ].hhfield .b0 ;
	printchar ( 32 ) ;
	printint ( t - 1 ) ;
      } 
      if ( q != 0 ) 
      {
	if ( mem [mem [q ].hhfield .v.RH ].hhfield .b0 == 0 ) 
	{
	  print ( 557 ) ;
	  print ( octantdir [mem [q + 3 ].cint ]) ;
	  q = mem [q ].hhfield .v.RH ;
	  while ( mem [mem [q ].hhfield .v.RH ].hhfield .b0 == 0 ) {
	      
	    printchar ( 32 ) ;
	    print ( octantdir [mem [q + 3 ].cint ]) ;
	    q = mem [q ].hhfield .v.RH ;
	  } 
	  printchar ( 41 ) ;
	} 
	printchar ( 32 ) ;
	print ( octantdir [mem [q + 3 ].cint ]) ;
	q = 0 ;
      } 
    } 
    else if ( q == 0 ) 
    q = p ;
    p = mem [p ].hhfield .v.RH ;
  } while ( ! ( p == f ) ) ;
  printchar ( 32 ) ;
  printint ( mem [p ].hhfield .b0 - 1 ) ;
  if ( q != 0 ) 
  if ( mem [mem [q ].hhfield .v.RH ].hhfield .b0 == 0 ) 
  {
    print ( 557 ) ;
    print ( octantdir [mem [q + 3 ].cint ]) ;
    q = mem [q ].hhfield .v.RH ;
    while ( mem [mem [q ].hhfield .v.RH ].hhfield .b0 == 0 ) {
	
      printchar ( 32 ) ;
      print ( octantdir [mem [q + 3 ].cint ]) ;
      q = mem [q ].hhfield .v.RH ;
    } 
    printchar ( 41 ) ;
  } 
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 261 ) ;
    print ( s ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zremovecubic ( halfword p ) 
#else
zremovecubic ( p ) 
  halfword p ;
#endif
{
  halfword q  ;
  q = mem [p ].hhfield .v.RH ;
  mem [p ].hhfield .b1 = mem [q ].hhfield .b1 ;
  mem [p ].hhfield .v.RH = mem [q ].hhfield .v.RH ;
  mem [p + 1 ].cint = mem [q + 1 ].cint ;
  mem [p + 2 ].cint = mem [q + 2 ].cint ;
  mem [p + 5 ].cint = mem [q + 5 ].cint ;
  mem [p + 6 ].cint = mem [q + 6 ].cint ;
  freenode ( q , 7 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zsplitcubic ( halfword p , fraction t , scaled xq , scaled yq ) 
#else
zsplitcubic ( p , t , xq , yq ) 
  halfword p ;
  fraction t ;
  scaled xq ;
  scaled yq ;
#endif
{
  scaled v  ;
  halfword q, r  ;
  q = mem [p ].hhfield .v.RH ;
  r = getnode ( 7 ) ;
  mem [p ].hhfield .v.RH = r ;
  mem [r ].hhfield .v.RH = q ;
  mem [r ].hhfield .b0 = mem [q ].hhfield .b0 ;
  mem [r ].hhfield .b1 = mem [p ].hhfield .b1 ;
  v = mem [p + 5 ].cint - takefraction ( mem [p + 5 ].cint - mem [q + 3 ]
  .cint , t ) ;
  mem [p + 5 ].cint = mem [p + 1 ].cint - takefraction ( mem [p + 1 ]
  .cint - mem [p + 5 ].cint , t ) ;
  mem [q + 3 ].cint = mem [q + 3 ].cint - takefraction ( mem [q + 3 ]
  .cint - xq , t ) ;
  mem [r + 3 ].cint = mem [p + 5 ].cint - takefraction ( mem [p + 5 ]
  .cint - v , t ) ;
  mem [r + 5 ].cint = v - takefraction ( v - mem [q + 3 ].cint , t ) ;
  mem [r + 1 ].cint = mem [r + 3 ].cint - takefraction ( mem [r + 3 ]
  .cint - mem [r + 5 ].cint , t ) ;
  v = mem [p + 6 ].cint - takefraction ( mem [p + 6 ].cint - mem [q + 4 ]
  .cint , t ) ;
  mem [p + 6 ].cint = mem [p + 2 ].cint - takefraction ( mem [p + 2 ]
  .cint - mem [p + 6 ].cint , t ) ;
  mem [q + 4 ].cint = mem [q + 4 ].cint - takefraction ( mem [q + 4 ]
  .cint - yq , t ) ;
  mem [r + 4 ].cint = mem [p + 6 ].cint - takefraction ( mem [p + 6 ]
  .cint - v , t ) ;
  mem [r + 6 ].cint = v - takefraction ( v - mem [q + 4 ].cint , t ) ;
  mem [r + 2 ].cint = mem [r + 4 ].cint - takefraction ( mem [r + 4 ]
  .cint - mem [r + 6 ].cint , t ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
quadrantsubdivide ( void ) 
#else
quadrantsubdivide ( ) 
#endif
{
  /* 22 10 */ halfword p, q, r, s, pp, qq  ;
  scaled firstx, firsty  ;
  scaled del1, del2, del3, del, dmax  ;
  fraction t  ;
  scaled destx, desty  ;
  boolean constantx  ;
  p = curspec ;
  firstx = mem [curspec + 1 ].cint ;
  firsty = mem [curspec + 2 ].cint ;
  do {
      lab22: q = mem [p ].hhfield .v.RH ;
    if ( q == curspec ) 
    {
      destx = firstx ;
      desty = firsty ;
    } 
    else {
	
      destx = mem [q + 1 ].cint ;
      desty = mem [q + 2 ].cint ;
    } 
    del1 = mem [p + 5 ].cint - mem [p + 1 ].cint ;
    del2 = mem [q + 3 ].cint - mem [p + 5 ].cint ;
    del3 = destx - mem [q + 3 ].cint ;
    if ( del1 != 0 ) 
    del = del1 ;
    else if ( del2 != 0 ) 
    del = del2 ;
    else del = del3 ;
    if ( del != 0 ) 
    {
      dmax = abs ( del1 ) ;
      if ( abs ( del2 ) > dmax ) 
      dmax = abs ( del2 ) ;
      if ( abs ( del3 ) > dmax ) 
      dmax = abs ( del3 ) ;
      while ( dmax < 134217728L ) {
	  
	dmax = dmax + dmax ;
	del1 = del1 + del1 ;
	del2 = del2 + del2 ;
	del3 = del3 + del3 ;
      } 
    } 
    if ( del == 0 ) 
    constantx = true ;
    else {
	
      constantx = false ;
      if ( del < 0 ) 
      {
	mem [p + 1 ].cint = - (integer) mem [p + 1 ].cint ;
	mem [p + 5 ].cint = - (integer) mem [p + 5 ].cint ;
	mem [q + 3 ].cint = - (integer) mem [q + 3 ].cint ;
	del1 = - (integer) del1 ;
	del2 = - (integer) del2 ;
	del3 = - (integer) del3 ;
	destx = - (integer) destx ;
	mem [p ].hhfield .b1 = 2 ;
      } 
      t = crossingpoint ( del1 , del2 , del3 ) ;
      if ( t < 268435456L ) 
      {
	splitcubic ( p , t , destx , desty ) ;
	r = mem [p ].hhfield .v.RH ;
	if ( mem [r ].hhfield .b1 > 1 ) 
	mem [r ].hhfield .b1 = 1 ;
	else mem [r ].hhfield .b1 = 2 ;
	if ( mem [r + 1 ].cint < mem [p + 1 ].cint ) 
	mem [r + 1 ].cint = mem [p + 1 ].cint ;
	mem [r + 3 ].cint = mem [r + 1 ].cint ;
	if ( mem [p + 5 ].cint > mem [r + 1 ].cint ) 
	mem [p + 5 ].cint = mem [r + 1 ].cint ;
	mem [r + 1 ].cint = - (integer) mem [r + 1 ].cint ;
	mem [r + 5 ].cint = mem [r + 1 ].cint ;
	mem [q + 3 ].cint = - (integer) mem [q + 3 ].cint ;
	destx = - (integer) destx ;
	del2 = del2 - takefraction ( del2 - del3 , t ) ;
	if ( del2 > 0 ) 
	del2 = 0 ;
	t = crossingpoint ( 0 , - (integer) del2 , - (integer) del3 ) ;
	if ( t < 268435456L ) 
	{
	  splitcubic ( r , t , destx , desty ) ;
	  s = mem [r ].hhfield .v.RH ;
	  if ( mem [s + 1 ].cint < destx ) 
	  mem [s + 1 ].cint = destx ;
	  if ( mem [s + 1 ].cint < mem [r + 1 ].cint ) 
	  mem [s + 1 ].cint = mem [r + 1 ].cint ;
	  mem [s ].hhfield .b1 = mem [p ].hhfield .b1 ;
	  mem [s + 3 ].cint = mem [s + 1 ].cint ;
	  if ( mem [q + 3 ].cint < destx ) 
	  mem [q + 3 ].cint = - (integer) destx ;
	  else if ( mem [q + 3 ].cint > mem [s + 1 ].cint ) 
	  mem [q + 3 ].cint = - (integer) mem [s + 1 ].cint ;
	  else mem [q + 3 ].cint = - (integer) mem [q + 3 ].cint ;
	  mem [s + 1 ].cint = - (integer) mem [s + 1 ].cint ;
	  mem [s + 5 ].cint = mem [s + 1 ].cint ;
	} 
	else {
	    
	  if ( mem [r + 1 ].cint > destx ) 
	  {
	    mem [r + 1 ].cint = destx ;
	    mem [r + 3 ].cint = - (integer) mem [r + 1 ].cint ;
	    mem [r + 5 ].cint = mem [r + 1 ].cint ;
	  } 
	  if ( mem [q + 3 ].cint > destx ) 
	  mem [q + 3 ].cint = destx ;
	  else if ( mem [q + 3 ].cint < mem [r + 1 ].cint ) 
	  mem [q + 3 ].cint = mem [r + 1 ].cint ;
	} 
      } 
    } 
    pp = p ;
    do {
	qq = mem [pp ].hhfield .v.RH ;
      abnegate ( mem [qq + 1 ].cint , mem [qq + 2 ].cint , mem [qq ]
      .hhfield .b1 , mem [pp ].hhfield .b1 ) ;
      destx = curx ;
      desty = cury ;
      del1 = mem [pp + 6 ].cint - mem [pp + 2 ].cint ;
      del2 = mem [qq + 4 ].cint - mem [pp + 6 ].cint ;
      del3 = desty - mem [qq + 4 ].cint ;
      if ( del1 != 0 ) 
      del = del1 ;
      else if ( del2 != 0 ) 
      del = del2 ;
      else del = del3 ;
      if ( del != 0 ) 
      {
	dmax = abs ( del1 ) ;
	if ( abs ( del2 ) > dmax ) 
	dmax = abs ( del2 ) ;
	if ( abs ( del3 ) > dmax ) 
	dmax = abs ( del3 ) ;
	while ( dmax < 134217728L ) {
	    
	  dmax = dmax + dmax ;
	  del1 = del1 + del1 ;
	  del2 = del2 + del2 ;
	  del3 = del3 + del3 ;
	} 
      } 
      if ( del != 0 ) 
      {
	if ( del < 0 ) 
	{
	  mem [pp + 2 ].cint = - (integer) mem [pp + 2 ].cint ;
	  mem [pp + 6 ].cint = - (integer) mem [pp + 6 ].cint ;
	  mem [qq + 4 ].cint = - (integer) mem [qq + 4 ].cint ;
	  del1 = - (integer) del1 ;
	  del2 = - (integer) del2 ;
	  del3 = - (integer) del3 ;
	  desty = - (integer) desty ;
	  mem [pp ].hhfield .b1 = mem [pp ].hhfield .b1 + 2 ;
	} 
	t = crossingpoint ( del1 , del2 , del3 ) ;
	if ( t < 268435456L ) 
	{
	  splitcubic ( pp , t , destx , desty ) ;
	  r = mem [pp ].hhfield .v.RH ;
	  if ( mem [r ].hhfield .b1 > 2 ) 
	  mem [r ].hhfield .b1 = mem [r ].hhfield .b1 - 2 ;
	  else mem [r ].hhfield .b1 = mem [r ].hhfield .b1 + 2 ;
	  if ( mem [r + 2 ].cint < mem [pp + 2 ].cint ) 
	  mem [r + 2 ].cint = mem [pp + 2 ].cint ;
	  mem [r + 4 ].cint = mem [r + 2 ].cint ;
	  if ( mem [pp + 6 ].cint > mem [r + 2 ].cint ) 
	  mem [pp + 6 ].cint = mem [r + 2 ].cint ;
	  mem [r + 2 ].cint = - (integer) mem [r + 2 ].cint ;
	  mem [r + 6 ].cint = mem [r + 2 ].cint ;
	  mem [qq + 4 ].cint = - (integer) mem [qq + 4 ].cint ;
	  desty = - (integer) desty ;
	  if ( mem [r + 1 ].cint < mem [pp + 1 ].cint ) 
	  mem [r + 1 ].cint = mem [pp + 1 ].cint ;
	  else if ( mem [r + 1 ].cint > destx ) 
	  mem [r + 1 ].cint = destx ;
	  if ( mem [r + 3 ].cint > mem [r + 1 ].cint ) 
	  {
	    mem [r + 3 ].cint = mem [r + 1 ].cint ;
	    if ( mem [pp + 5 ].cint > mem [r + 1 ].cint ) 
	    mem [pp + 5 ].cint = mem [r + 1 ].cint ;
	  } 
	  if ( mem [r + 5 ].cint < mem [r + 1 ].cint ) 
	  {
	    mem [r + 5 ].cint = mem [r + 1 ].cint ;
	    if ( mem [qq + 3 ].cint < mem [r + 1 ].cint ) 
	    mem [qq + 3 ].cint = mem [r + 1 ].cint ;
	  } 
	  del2 = del2 - takefraction ( del2 - del3 , t ) ;
	  if ( del2 > 0 ) 
	  del2 = 0 ;
	  t = crossingpoint ( 0 , - (integer) del2 , - (integer) del3 ) ;
	  if ( t < 268435456L ) 
	  {
	    splitcubic ( r , t , destx , desty ) ;
	    s = mem [r ].hhfield .v.RH ;
	    if ( mem [s + 2 ].cint < desty ) 
	    mem [s + 2 ].cint = desty ;
	    if ( mem [s + 2 ].cint < mem [r + 2 ].cint ) 
	    mem [s + 2 ].cint = mem [r + 2 ].cint ;
	    mem [s ].hhfield .b1 = mem [pp ].hhfield .b1 ;
	    mem [s + 4 ].cint = mem [s + 2 ].cint ;
	    if ( mem [qq + 4 ].cint < desty ) 
	    mem [qq + 4 ].cint = - (integer) desty ;
	    else if ( mem [qq + 4 ].cint > mem [s + 2 ].cint ) 
	    mem [qq + 4 ].cint = - (integer) mem [s + 2 ].cint ;
	    else mem [qq + 4 ].cint = - (integer) mem [qq + 4 ].cint ;
	    mem [s + 2 ].cint = - (integer) mem [s + 2 ].cint ;
	    mem [s + 6 ].cint = mem [s + 2 ].cint ;
	    if ( mem [s + 1 ].cint < mem [r + 1 ].cint ) 
	    mem [s + 1 ].cint = mem [r + 1 ].cint ;
	    else if ( mem [s + 1 ].cint > destx ) 
	    mem [s + 1 ].cint = destx ;
	    if ( mem [s + 3 ].cint > mem [s + 1 ].cint ) 
	    {
	      mem [s + 3 ].cint = mem [s + 1 ].cint ;
	      if ( mem [r + 5 ].cint > mem [s + 1 ].cint ) 
	      mem [r + 5 ].cint = mem [s + 1 ].cint ;
	    } 
	    if ( mem [s + 5 ].cint < mem [s + 1 ].cint ) 
	    {
	      mem [s + 5 ].cint = mem [s + 1 ].cint ;
	      if ( mem [qq + 3 ].cint < mem [s + 1 ].cint ) 
	      mem [qq + 3 ].cint = mem [s + 1 ].cint ;
	    } 
	  } 
	  else {
	      
	    if ( mem [r + 2 ].cint > desty ) 
	    {
	      mem [r + 2 ].cint = desty ;
	      mem [r + 4 ].cint = - (integer) mem [r + 2 ].cint ;
	      mem [r + 6 ].cint = mem [r + 2 ].cint ;
	    } 
	    if ( mem [qq + 4 ].cint > desty ) 
	    mem [qq + 4 ].cint = desty ;
	    else if ( mem [qq + 4 ].cint < mem [r + 2 ].cint ) 
	    mem [qq + 4 ].cint = mem [r + 2 ].cint ;
	  } 
	} 
      } 
      else if ( constantx ) 
      {
	if ( q != p ) 
	{
	  removecubic ( p ) ;
	  if ( curspec != q ) 
	  goto lab22 ;
	  else {
	      
	    curspec = p ;
	    goto lab10 ;
	  } 
	} 
      } 
      else if ( ! odd ( mem [pp ].hhfield .b1 ) ) 
      {
	mem [pp + 2 ].cint = - (integer) mem [pp + 2 ].cint ;
	mem [pp + 6 ].cint = - (integer) mem [pp + 6 ].cint ;
	mem [qq + 4 ].cint = - (integer) mem [qq + 4 ].cint ;
	del1 = - (integer) del1 ;
	del2 = - (integer) del2 ;
	del3 = - (integer) del3 ;
	desty = - (integer) desty ;
	mem [pp ].hhfield .b1 = mem [pp ].hhfield .b1 + 2 ;
      } 
      pp = qq ;
    } while ( ! ( pp == q ) ) ;
    if ( constantx ) 
    {
      pp = p ;
      do {
	  qq = mem [pp ].hhfield .v.RH ;
	if ( mem [pp ].hhfield .b1 > 2 ) 
	{
	  mem [pp ].hhfield .b1 = mem [pp ].hhfield .b1 + 1 ;
	  mem [pp + 1 ].cint = - (integer) mem [pp + 1 ].cint ;
	  mem [pp + 5 ].cint = - (integer) mem [pp + 5 ].cint ;
	  mem [qq + 3 ].cint = - (integer) mem [qq + 3 ].cint ;
	} 
	pp = qq ;
      } while ( ! ( pp == q ) ) ;
    } 
    p = q ;
  } while ( ! ( p == curspec ) ) ;
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
octantsubdivide ( void ) 
#else
octantsubdivide ( ) 
#endif
{
  halfword p, q, r, s  ;
  scaled del1, del2, del3, del, dmax  ;
  fraction t  ;
  scaled destx, desty  ;
  p = curspec ;
  do {
      q = mem [p ].hhfield .v.RH ;
    mem [p + 1 ].cint = mem [p + 1 ].cint - mem [p + 2 ].cint ;
    mem [p + 5 ].cint = mem [p + 5 ].cint - mem [p + 6 ].cint ;
    mem [q + 3 ].cint = mem [q + 3 ].cint - mem [q + 4 ].cint ;
    if ( q == curspec ) 
    {
      unskew ( mem [q + 1 ].cint , mem [q + 2 ].cint , mem [q ].hhfield 
      .b1 ) ;
      skew ( curx , cury , mem [p ].hhfield .b1 ) ;
      destx = curx ;
      desty = cury ;
    } 
    else {
	
      abnegate ( mem [q + 1 ].cint , mem [q + 2 ].cint , mem [q ]
      .hhfield .b1 , mem [p ].hhfield .b1 ) ;
      destx = curx - cury ;
      desty = cury ;
    } 
    del1 = mem [p + 5 ].cint - mem [p + 1 ].cint ;
    del2 = mem [q + 3 ].cint - mem [p + 5 ].cint ;
    del3 = destx - mem [q + 3 ].cint ;
    if ( del1 != 0 ) 
    del = del1 ;
    else if ( del2 != 0 ) 
    del = del2 ;
    else del = del3 ;
    if ( del != 0 ) 
    {
      dmax = abs ( del1 ) ;
      if ( abs ( del2 ) > dmax ) 
      dmax = abs ( del2 ) ;
      if ( abs ( del3 ) > dmax ) 
      dmax = abs ( del3 ) ;
      while ( dmax < 134217728L ) {
	  
	dmax = dmax + dmax ;
	del1 = del1 + del1 ;
	del2 = del2 + del2 ;
	del3 = del3 + del3 ;
      } 
    } 
    if ( del != 0 ) 
    {
      if ( del < 0 ) 
      {
	mem [p + 2 ].cint = mem [p + 1 ].cint + mem [p + 2 ].cint ;
	mem [p + 1 ].cint = - (integer) mem [p + 1 ].cint ;
	mem [p + 6 ].cint = mem [p + 5 ].cint + mem [p + 6 ].cint ;
	mem [p + 5 ].cint = - (integer) mem [p + 5 ].cint ;
	mem [q + 4 ].cint = mem [q + 3 ].cint + mem [q + 4 ].cint ;
	mem [q + 3 ].cint = - (integer) mem [q + 3 ].cint ;
	del1 = - (integer) del1 ;
	del2 = - (integer) del2 ;
	del3 = - (integer) del3 ;
	desty = destx + desty ;
	destx = - (integer) destx ;
	mem [p ].hhfield .b1 = mem [p ].hhfield .b1 + 4 ;
      } 
      t = crossingpoint ( del1 , del2 , del3 ) ;
      if ( t < 268435456L ) 
      {
	splitcubic ( p , t , destx , desty ) ;
	r = mem [p ].hhfield .v.RH ;
	if ( mem [r ].hhfield .b1 > 4 ) 
	mem [r ].hhfield .b1 = mem [r ].hhfield .b1 - 4 ;
	else mem [r ].hhfield .b1 = mem [r ].hhfield .b1 + 4 ;
	if ( mem [r + 2 ].cint < mem [p + 2 ].cint ) 
	mem [r + 2 ].cint = mem [p + 2 ].cint ;
	else if ( mem [r + 2 ].cint > desty ) 
	mem [r + 2 ].cint = desty ;
	if ( mem [p + 1 ].cint + mem [r + 2 ].cint > destx + desty ) 
	mem [r + 2 ].cint = destx + desty - mem [p + 1 ].cint ;
	if ( mem [r + 4 ].cint > mem [r + 2 ].cint ) 
	{
	  mem [r + 4 ].cint = mem [r + 2 ].cint ;
	  if ( mem [p + 6 ].cint > mem [r + 2 ].cint ) 
	  mem [p + 6 ].cint = mem [r + 2 ].cint ;
	} 
	if ( mem [r + 6 ].cint < mem [r + 2 ].cint ) 
	{
	  mem [r + 6 ].cint = mem [r + 2 ].cint ;
	  if ( mem [q + 4 ].cint < mem [r + 2 ].cint ) 
	  mem [q + 4 ].cint = mem [r + 2 ].cint ;
	} 
	if ( mem [r + 1 ].cint < mem [p + 1 ].cint ) 
	mem [r + 1 ].cint = mem [p + 1 ].cint ;
	else if ( mem [r + 1 ].cint + mem [r + 2 ].cint > destx + desty ) 
	mem [r + 1 ].cint = destx + desty - mem [r + 2 ].cint ;
	mem [r + 3 ].cint = mem [r + 1 ].cint ;
	if ( mem [p + 5 ].cint > mem [r + 1 ].cint ) 
	mem [p + 5 ].cint = mem [r + 1 ].cint ;
	mem [r + 2 ].cint = mem [r + 2 ].cint + mem [r + 1 ].cint ;
	mem [r + 6 ].cint = mem [r + 6 ].cint + mem [r + 1 ].cint ;
	mem [r + 1 ].cint = - (integer) mem [r + 1 ].cint ;
	mem [r + 5 ].cint = mem [r + 1 ].cint ;
	mem [q + 4 ].cint = mem [q + 4 ].cint + mem [q + 3 ].cint ;
	mem [q + 3 ].cint = - (integer) mem [q + 3 ].cint ;
	desty = desty + destx ;
	destx = - (integer) destx ;
	if ( mem [r + 6 ].cint < mem [r + 2 ].cint ) 
	{
	  mem [r + 6 ].cint = mem [r + 2 ].cint ;
	  if ( mem [q + 4 ].cint < mem [r + 2 ].cint ) 
	  mem [q + 4 ].cint = mem [r + 2 ].cint ;
	} 
	del2 = del2 - takefraction ( del2 - del3 , t ) ;
	if ( del2 > 0 ) 
	del2 = 0 ;
	t = crossingpoint ( 0 , - (integer) del2 , - (integer) del3 ) ;
	if ( t < 268435456L ) 
	{
	  splitcubic ( r , t , destx , desty ) ;
	  s = mem [r ].hhfield .v.RH ;
	  if ( mem [s + 2 ].cint < mem [r + 2 ].cint ) 
	  mem [s + 2 ].cint = mem [r + 2 ].cint ;
	  else if ( mem [s + 2 ].cint > desty ) 
	  mem [s + 2 ].cint = desty ;
	  if ( mem [r + 1 ].cint + mem [s + 2 ].cint > destx + desty ) 
	  mem [s + 2 ].cint = destx + desty - mem [r + 1 ].cint ;
	  if ( mem [s + 4 ].cint > mem [s + 2 ].cint ) 
	  {
	    mem [s + 4 ].cint = mem [s + 2 ].cint ;
	    if ( mem [r + 6 ].cint > mem [s + 2 ].cint ) 
	    mem [r + 6 ].cint = mem [s + 2 ].cint ;
	  } 
	  if ( mem [s + 6 ].cint < mem [s + 2 ].cint ) 
	  {
	    mem [s + 6 ].cint = mem [s + 2 ].cint ;
	    if ( mem [q + 4 ].cint < mem [s + 2 ].cint ) 
	    mem [q + 4 ].cint = mem [s + 2 ].cint ;
	  } 
	  if ( mem [s + 1 ].cint + mem [s + 2 ].cint > destx + desty ) 
	  mem [s + 1 ].cint = destx + desty - mem [s + 2 ].cint ;
	  else {
	      
	    if ( mem [s + 1 ].cint < destx ) 
	    mem [s + 1 ].cint = destx ;
	    if ( mem [s + 1 ].cint < mem [r + 1 ].cint ) 
	    mem [s + 1 ].cint = mem [r + 1 ].cint ;
	  } 
	  mem [s ].hhfield .b1 = mem [p ].hhfield .b1 ;
	  mem [s + 3 ].cint = mem [s + 1 ].cint ;
	  if ( mem [q + 3 ].cint < destx ) 
	  {
	    mem [q + 4 ].cint = mem [q + 4 ].cint + destx ;
	    mem [q + 3 ].cint = - (integer) destx ;
	  } 
	  else if ( mem [q + 3 ].cint > mem [s + 1 ].cint ) 
	  {
	    mem [q + 4 ].cint = mem [q + 4 ].cint + mem [s + 1 ].cint ;
	    mem [q + 3 ].cint = - (integer) mem [s + 1 ].cint ;
	  } 
	  else {
	      
	    mem [q + 4 ].cint = mem [q + 4 ].cint + mem [q + 3 ].cint ;
	    mem [q + 3 ].cint = - (integer) mem [q + 3 ].cint ;
	  } 
	  mem [s + 2 ].cint = mem [s + 2 ].cint + mem [s + 1 ].cint ;
	  mem [s + 6 ].cint = mem [s + 6 ].cint + mem [s + 1 ].cint ;
	  mem [s + 1 ].cint = - (integer) mem [s + 1 ].cint ;
	  mem [s + 5 ].cint = mem [s + 1 ].cint ;
	  if ( mem [s + 6 ].cint < mem [s + 2 ].cint ) 
	  {
	    mem [s + 6 ].cint = mem [s + 2 ].cint ;
	    if ( mem [q + 4 ].cint < mem [s + 2 ].cint ) 
	    mem [q + 4 ].cint = mem [s + 2 ].cint ;
	  } 
	} 
	else {
	    
	  if ( mem [r + 1 ].cint > destx ) 
	  {
	    mem [r + 1 ].cint = destx ;
	    mem [r + 3 ].cint = - (integer) mem [r + 1 ].cint ;
	    mem [r + 5 ].cint = mem [r + 1 ].cint ;
	  } 
	  if ( mem [q + 3 ].cint > destx ) 
	  mem [q + 3 ].cint = destx ;
	  else if ( mem [q + 3 ].cint < mem [r + 1 ].cint ) 
	  mem [q + 3 ].cint = mem [r + 1 ].cint ;
	} 
      } 
    } 
    p = q ;
  } while ( ! ( p == curspec ) ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
makesafe ( void ) 
#else
makesafe ( ) 
#endif
{
  integer k  ;
  boolean allsafe  ;
  scaled nexta  ;
  scaled deltaa, deltab  ;
  before [curroundingptr ]= before [0 ];
  nodetoround [curroundingptr ]= nodetoround [0 ];
  do {
      after [curroundingptr ]= after [0 ];
    allsafe = true ;
    nexta = after [0 ];
    {register integer for_end; k = 0 ;for_end = curroundingptr - 1 ; if ( k 
    <= for_end) do 
      {
	deltab = before [k + 1 ]- before [k ];
	if ( deltab >= 0 ) 
	deltaa = after [k + 1 ]- nexta ;
	else deltaa = nexta - after [k + 1 ];
	nexta = after [k + 1 ];
	if ( ( deltaa < 0 ) || ( deltaa > abs ( deltab + deltab ) ) ) 
	{
	  allsafe = false ;
	  after [k ]= before [k ];
	  if ( k == curroundingptr - 1 ) 
	  after [0 ]= before [0 ];
	  else after [k + 1 ]= before [k + 1 ];
	} 
      } 
    while ( k++ < for_end ) ;} 
  } while ( ! ( allsafe ) ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zbeforeandafter ( scaled b , scaled a , halfword p ) 
#else
zbeforeandafter ( b , a , p ) 
  scaled b ;
  scaled a ;
  halfword p ;
#endif
{
  if ( curroundingptr == maxroundingptr ) 
  if ( maxroundingptr < maxwiggle ) 
  incr ( maxroundingptr ) ;
  else overflow ( 567 , maxwiggle ) ;
  after [curroundingptr ]= a ;
  before [curroundingptr ]= b ;
  nodetoround [curroundingptr ]= p ;
  incr ( curroundingptr ) ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zgoodval ( scaled b , scaled o ) 
#else
zgoodval ( b , o ) 
  scaled b ;
  scaled o ;
#endif
{
  register scaled Result; scaled a  ;
  a = b + o ;
  if ( a >= 0 ) 
  a = a - ( a % curgran ) - o ;
  else a = a + ( ( - (integer) ( a + 1 ) ) % curgran ) - curgran + 1 - o ;
  if ( b - a < a + curgran - b ) 
  Result = a ;
  else Result = a + curgran ;
  return Result ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zcompromise ( scaled u , scaled v ) 
#else
zcompromise ( u , v ) 
  scaled u ;
  scaled v ;
#endif
{
  register scaled Result; Result = half ( goodval ( u + u , - (integer) u - 
  v ) ) ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
xyround ( void ) 
#else
xyround ( ) 
#endif
{
  halfword p, q  ;
  scaled b, a  ;
  scaled penedge  ;
  fraction alpha  ;
  curgran = abs ( internal [37 ]) ;
  if ( curgran == 0 ) 
  curgran = 65536L ;
  p = curspec ;
  curroundingptr = 0 ;
  do {
      q = mem [p ].hhfield .v.RH ;
    if ( odd ( mem [p ].hhfield .b1 ) != odd ( mem [q ].hhfield .b1 ) ) 
    {
      if ( odd ( mem [q ].hhfield .b1 ) ) 
      b = mem [q + 1 ].cint ;
      else b = - (integer) mem [q + 1 ].cint ;
      if ( ( abs ( mem [q + 1 ].cint - mem [q + 5 ].cint ) < 655 ) || ( 
      abs ( mem [q + 1 ].cint + mem [q + 3 ].cint ) < 655 ) ) 
      {
	if ( curpen == 3 ) 
	penedge = 0 ;
	else if ( curpathtype == 0 ) 
	penedge = compromise ( mem [mem [curpen + 5 ].hhfield .v.RH + 2 ]
	.cint , mem [mem [curpen + 7 ].hhfield .v.RH + 2 ].cint ) ;
	else if ( odd ( mem [q ].hhfield .b1 ) ) 
	penedge = mem [mem [curpen + 7 ].hhfield .v.RH + 2 ].cint ;
	else penedge = mem [mem [curpen + 5 ].hhfield .v.RH + 2 ].cint ;
	a = goodval ( b , penedge ) ;
      } 
      else a = b ;
      if ( abs ( a ) > maxallowed ) 
      if ( a > 0 ) 
      a = maxallowed ;
      else a = - (integer) maxallowed ;
      beforeandafter ( b , a , q ) ;
    } 
    p = q ;
  } while ( ! ( p == curspec ) ) ;
  if ( curroundingptr > 0 ) 
  {
    makesafe () ;
    do {
	decr ( curroundingptr ) ;
      if ( ( after [curroundingptr ]!= before [curroundingptr ]) || ( 
      after [curroundingptr + 1 ]!= before [curroundingptr + 1 ]) ) 
      {
	p = nodetoround [curroundingptr ];
	if ( odd ( mem [p ].hhfield .b1 ) ) 
	{
	  b = before [curroundingptr ];
	  a = after [curroundingptr ];
	} 
	else {
	    
	  b = - (integer) before [curroundingptr ];
	  a = - (integer) after [curroundingptr ];
	} 
	if ( before [curroundingptr ]== before [curroundingptr + 1 ]) 
	alpha = 268435456L ;
	else alpha = makefraction ( after [curroundingptr + 1 ]- after [
	curroundingptr ], before [curroundingptr + 1 ]- before [
	curroundingptr ]) ;
	do {
	    mem [p + 1 ].cint = takefraction ( alpha , mem [p + 1 ].cint 
	  - b ) + a ;
	  mem [p + 5 ].cint = takefraction ( alpha , mem [p + 5 ].cint - b 
	  ) + a ;
	  p = mem [p ].hhfield .v.RH ;
	  mem [p + 3 ].cint = takefraction ( alpha , mem [p + 3 ].cint - b 
	  ) + a ;
	} while ( ! ( p == nodetoround [curroundingptr + 1 ]) ) ;
      } 
    } while ( ! ( curroundingptr == 0 ) ) ;
  } 
  p = curspec ;
  curroundingptr = 0 ;
  do {
      q = mem [p ].hhfield .v.RH ;
    if ( ( mem [p ].hhfield .b1 > 2 ) != ( mem [q ].hhfield .b1 > 2 ) ) 
    {
      if ( mem [q ].hhfield .b1 <= 2 ) 
      b = mem [q + 2 ].cint ;
      else b = - (integer) mem [q + 2 ].cint ;
      if ( ( abs ( mem [q + 2 ].cint - mem [q + 6 ].cint ) < 655 ) || ( 
      abs ( mem [q + 2 ].cint + mem [q + 4 ].cint ) < 655 ) ) 
      {
	if ( curpen == 3 ) 
	penedge = 0 ;
	else if ( curpathtype == 0 ) 
	penedge = compromise ( mem [mem [curpen + 2 ].hhfield .v.RH + 2 ]
	.cint , mem [mem [curpen + 1 ].hhfield .v.RH + 2 ].cint ) ;
	else if ( mem [q ].hhfield .b1 <= 2 ) 
	penedge = mem [mem [curpen + 1 ].hhfield .v.RH + 2 ].cint ;
	else penedge = mem [mem [curpen + 2 ].hhfield .v.RH + 2 ].cint ;
	a = goodval ( b , penedge ) ;
      } 
      else a = b ;
      if ( abs ( a ) > maxallowed ) 
      if ( a > 0 ) 
      a = maxallowed ;
      else a = - (integer) maxallowed ;
      beforeandafter ( b , a , q ) ;
    } 
    p = q ;
  } while ( ! ( p == curspec ) ) ;
  if ( curroundingptr > 0 ) 
  {
    makesafe () ;
    do {
	decr ( curroundingptr ) ;
      if ( ( after [curroundingptr ]!= before [curroundingptr ]) || ( 
      after [curroundingptr + 1 ]!= before [curroundingptr + 1 ]) ) 
      {
	p = nodetoround [curroundingptr ];
	if ( mem [p ].hhfield .b1 <= 2 ) 
	{
	  b = before [curroundingptr ];
	  a = after [curroundingptr ];
	} 
	else {
	    
	  b = - (integer) before [curroundingptr ];
	  a = - (integer) after [curroundingptr ];
	} 
	if ( before [curroundingptr ]== before [curroundingptr + 1 ]) 
	alpha = 268435456L ;
	else alpha = makefraction ( after [curroundingptr + 1 ]- after [
	curroundingptr ], before [curroundingptr + 1 ]- before [
	curroundingptr ]) ;
	do {
	    mem [p + 2 ].cint = takefraction ( alpha , mem [p + 2 ].cint 
	  - b ) + a ;
	  mem [p + 6 ].cint = takefraction ( alpha , mem [p + 6 ].cint - b 
	  ) + a ;
	  p = mem [p ].hhfield .v.RH ;
	  mem [p + 4 ].cint = takefraction ( alpha , mem [p + 4 ].cint - b 
	  ) + a ;
	} while ( ! ( p == nodetoround [curroundingptr + 1 ]) ) ;
      } 
    } while ( ! ( curroundingptr == 0 ) ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
diaground ( void ) 
#else
diaground ( ) 
#endif
{
  halfword p, q, pp  ;
  scaled b, a, bb, aa, d, c, dd, cc  ;
  scaled penedge  ;
  fraction alpha, beta  ;
  scaled nexta  ;
  boolean allsafe  ;
  integer k  ;
  scaled firstx, firsty  ;
  p = curspec ;
  curroundingptr = 0 ;
  do {
      q = mem [p ].hhfield .v.RH ;
    if ( mem [p ].hhfield .b1 != mem [q ].hhfield .b1 ) 
    {
      if ( mem [q ].hhfield .b1 > 4 ) 
      b = - (integer) mem [q + 1 ].cint ;
      else b = mem [q + 1 ].cint ;
      if ( abs ( mem [q ].hhfield .b1 - mem [p ].hhfield .b1 ) == 4 ) 
      if ( ( abs ( mem [q + 1 ].cint - mem [q + 5 ].cint ) < 655 ) || ( 
      abs ( mem [q + 1 ].cint + mem [q + 3 ].cint ) < 655 ) ) 
      {
	if ( curpen == 3 ) 
	penedge = 0 ;
	else if ( curpathtype == 0 ) 
	switch ( mem [q ].hhfield .b1 ) 
	{case 1 : 
	case 5 : 
	  penedge = compromise ( mem [mem [mem [curpen + 1 ].hhfield .v.RH 
	  ].hhfield .lhfield + 1 ].cint , - (integer) mem [mem [mem [
	  curpen + 4 ].hhfield .v.RH ].hhfield .lhfield + 1 ].cint ) ;
	  break ;
	case 4 : 
	case 8 : 
	  penedge = - (integer) compromise ( mem [mem [mem [curpen + 1 ]
	  .hhfield .v.RH ].hhfield .lhfield + 1 ].cint , - (integer) mem [
	  mem [mem [curpen + 4 ].hhfield .v.RH ].hhfield .lhfield + 1 ]
	  .cint ) ;
	  break ;
	case 6 : 
	case 2 : 
	  penedge = compromise ( mem [mem [mem [curpen + 2 ].hhfield .v.RH 
	  ].hhfield .lhfield + 1 ].cint , - (integer) mem [mem [mem [
	  curpen + 3 ].hhfield .v.RH ].hhfield .lhfield + 1 ].cint ) ;
	  break ;
	case 7 : 
	case 3 : 
	  penedge = - (integer) compromise ( mem [mem [mem [curpen + 2 ]
	  .hhfield .v.RH ].hhfield .lhfield + 1 ].cint , - (integer) mem [
	  mem [mem [curpen + 3 ].hhfield .v.RH ].hhfield .lhfield + 1 ]
	  .cint ) ;
	  break ;
	} 
	else if ( mem [q ].hhfield .b1 <= 4 ) 
	penedge = mem [mem [mem [curpen + mem [q ].hhfield .b1 ].hhfield 
	.v.RH ].hhfield .lhfield + 1 ].cint ;
	else penedge = - (integer) mem [mem [mem [curpen + mem [q ]
	.hhfield .b1 ].hhfield .v.RH ].hhfield .lhfield + 1 ].cint ;
	if ( odd ( mem [q ].hhfield .b1 ) ) 
	a = goodval ( b , penedge + halfp ( curgran ) ) ;
	else a = goodval ( b - 1 , penedge + halfp ( curgran ) ) ;
      } 
      else a = b ;
      else a = b ;
      beforeandafter ( b , a , q ) ;
    } 
    p = q ;
  } while ( ! ( p == curspec ) ) ;
  if ( curroundingptr > 0 ) 
  {
    p = nodetoround [0 ];
    firstx = mem [p + 1 ].cint ;
    firsty = mem [p + 2 ].cint ;
    before [curroundingptr ]= before [0 ];
    nodetoround [curroundingptr ]= nodetoround [0 ];
    do {
	after [curroundingptr ]= after [0 ];
      allsafe = true ;
      nexta = after [0 ];
      {register integer for_end; k = 0 ;for_end = curroundingptr - 1 ; if ( 
      k <= for_end) do 
	{
	  a = nexta ;
	  b = before [k ];
	  nexta = after [k + 1 ];
	  aa = nexta ;
	  bb = before [k + 1 ];
	  if ( ( a != b ) || ( aa != bb ) ) 
	  {
	    p = nodetoround [k ];
	    pp = nodetoround [k + 1 ];
	    if ( aa == bb ) 
	    {
	      if ( pp == nodetoround [0 ]) 
	      unskew ( firstx , firsty , mem [pp ].hhfield .b1 ) ;
	      else unskew ( mem [pp + 1 ].cint , mem [pp + 2 ].cint , mem 
	      [pp ].hhfield .b1 ) ;
	      skew ( curx , cury , mem [p ].hhfield .b1 ) ;
	      bb = curx ;
	      aa = bb ;
	      dd = cury ;
	      cc = dd ;
	      if ( mem [p ].hhfield .b1 > 4 ) 
	      {
		b = - (integer) b ;
		a = - (integer) a ;
	      } 
	    } 
	    else {
		
	      if ( mem [p ].hhfield .b1 > 4 ) 
	      {
		bb = - (integer) bb ;
		aa = - (integer) aa ;
		b = - (integer) b ;
		a = - (integer) a ;
	      } 
	      if ( pp == nodetoround [0 ]) 
	      dd = firsty - bb ;
	      else dd = mem [pp + 2 ].cint - bb ;
	      if ( odd ( aa - bb ) ) 
	      if ( mem [p ].hhfield .b1 > 4 ) 
	      cc = dd - half ( aa - bb + 1 ) ;
	      else cc = dd - half ( aa - bb - 1 ) ;
	      else cc = dd - half ( aa - bb ) ;
	    } 
	    d = mem [p + 2 ].cint ;
	    if ( odd ( a - b ) ) 
	    if ( mem [p ].hhfield .b1 > 4 ) 
	    c = d - half ( a - b - 1 ) ;
	    else c = d - half ( a - b + 1 ) ;
	    else c = d - half ( a - b ) ;
	    if ( ( aa < a ) || ( cc < c ) || ( aa - a > 2 * ( bb - b ) ) || ( 
	    cc - c > 2 * ( dd - d ) ) ) 
	    {
	      allsafe = false ;
	      after [k ]= before [k ];
	      if ( k == curroundingptr - 1 ) 
	      after [0 ]= before [0 ];
	      else after [k + 1 ]= before [k + 1 ];
	    } 
	  } 
	} 
      while ( k++ < for_end ) ;} 
    } while ( ! ( allsafe ) ) ;
    {register integer for_end; k = 0 ;for_end = curroundingptr - 1 ; if ( k 
    <= for_end) do 
      {
	a = after [k ];
	b = before [k ];
	aa = after [k + 1 ];
	bb = before [k + 1 ];
	if ( ( a != b ) || ( aa != bb ) ) 
	{
	  p = nodetoround [k ];
	  pp = nodetoround [k + 1 ];
	  if ( aa == bb ) 
	  {
	    if ( pp == nodetoround [0 ]) 
	    unskew ( firstx , firsty , mem [pp ].hhfield .b1 ) ;
	    else unskew ( mem [pp + 1 ].cint , mem [pp + 2 ].cint , mem [
	    pp ].hhfield .b1 ) ;
	    skew ( curx , cury , mem [p ].hhfield .b1 ) ;
	    bb = curx ;
	    aa = bb ;
	    dd = cury ;
	    cc = dd ;
	    if ( mem [p ].hhfield .b1 > 4 ) 
	    {
	      b = - (integer) b ;
	      a = - (integer) a ;
	    } 
	  } 
	  else {
	      
	    if ( mem [p ].hhfield .b1 > 4 ) 
	    {
	      bb = - (integer) bb ;
	      aa = - (integer) aa ;
	      b = - (integer) b ;
	      a = - (integer) a ;
	    } 
	    if ( pp == nodetoround [0 ]) 
	    dd = firsty - bb ;
	    else dd = mem [pp + 2 ].cint - bb ;
	    if ( odd ( aa - bb ) ) 
	    if ( mem [p ].hhfield .b1 > 4 ) 
	    cc = dd - half ( aa - bb + 1 ) ;
	    else cc = dd - half ( aa - bb - 1 ) ;
	    else cc = dd - half ( aa - bb ) ;
	  } 
	  d = mem [p + 2 ].cint ;
	  if ( odd ( a - b ) ) 
	  if ( mem [p ].hhfield .b1 > 4 ) 
	  c = d - half ( a - b - 1 ) ;
	  else c = d - half ( a - b + 1 ) ;
	  else c = d - half ( a - b ) ;
	  if ( b == bb ) 
	  alpha = 268435456L ;
	  else alpha = makefraction ( aa - a , bb - b ) ;
	  if ( d == dd ) 
	  beta = 268435456L ;
	  else beta = makefraction ( cc - c , dd - d ) ;
	  do {
	      mem [p + 1 ].cint = takefraction ( alpha , mem [p + 1 ]
	    .cint - b ) + a ;
	    mem [p + 2 ].cint = takefraction ( beta , mem [p + 2 ].cint - 
	    d ) + c ;
	    mem [p + 5 ].cint = takefraction ( alpha , mem [p + 5 ].cint - 
	    b ) + a ;
	    mem [p + 6 ].cint = takefraction ( beta , mem [p + 6 ].cint - 
	    d ) + c ;
	    p = mem [p ].hhfield .v.RH ;
	    mem [p + 3 ].cint = takefraction ( alpha , mem [p + 3 ].cint - 
	    b ) + a ;
	    mem [p + 4 ].cint = takefraction ( beta , mem [p + 4 ].cint - 
	    d ) + c ;
	  } while ( ! ( p == pp ) ) ;
	} 
      } 
    while ( k++ < for_end ) ;} 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
znewboundary ( halfword p , smallnumber octant ) 
#else
znewboundary ( p , octant ) 
  halfword p ;
  smallnumber octant ;
#endif
{
  halfword q, r  ;
  q = mem [p ].hhfield .v.RH ;
  r = getnode ( 7 ) ;
  mem [r ].hhfield .v.RH = q ;
  mem [p ].hhfield .v.RH = r ;
  mem [r ].hhfield .b0 = mem [q ].hhfield .b0 ;
  mem [r + 3 ].cint = mem [q + 3 ].cint ;
  mem [r + 4 ].cint = mem [q + 4 ].cint ;
  mem [r ].hhfield .b1 = 0 ;
  mem [q ].hhfield .b0 = 0 ;
  mem [r + 5 ].cint = octant ;
  mem [q + 3 ].cint = mem [q ].hhfield .b1 ;
  unskew ( mem [q + 1 ].cint , mem [q + 2 ].cint , mem [q ].hhfield .b1 
  ) ;
  skew ( curx , cury , octant ) ;
  mem [r + 1 ].cint = curx ;
  mem [r + 2 ].cint = cury ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zmakespec ( halfword h , scaled safetymargin , integer tracing ) 
#else
zmakespec ( h , safetymargin , tracing ) 
  halfword h ;
  scaled safetymargin ;
  integer tracing ;
#endif
{
  /* 22 30 */ register halfword Result; halfword p, q, r, s  ;
  integer k  ;
  boolean chopped  ;
  smallnumber o1, o2  ;
  boolean clockwise  ;
  integer dx1, dy1, dx2, dy2  ;
  integer dmax, del  ;
  curspec = h ;
  if ( tracing > 0 ) 
  printpath ( curspec , 558 , true ) ;
  maxallowed = 268402687L - safetymargin ;
  p = curspec ;
  k = 1 ;
  chopped = false ;
  do {
      if ( abs ( mem [p + 3 ].cint ) > maxallowed ) 
    {
      chopped = true ;
      if ( mem [p + 3 ].cint > 0 ) 
      mem [p + 3 ].cint = maxallowed ;
      else mem [p + 3 ].cint = - (integer) maxallowed ;
    } 
    if ( abs ( mem [p + 4 ].cint ) > maxallowed ) 
    {
      chopped = true ;
      if ( mem [p + 4 ].cint > 0 ) 
      mem [p + 4 ].cint = maxallowed ;
      else mem [p + 4 ].cint = - (integer) maxallowed ;
    } 
    if ( abs ( mem [p + 1 ].cint ) > maxallowed ) 
    {
      chopped = true ;
      if ( mem [p + 1 ].cint > 0 ) 
      mem [p + 1 ].cint = maxallowed ;
      else mem [p + 1 ].cint = - (integer) maxallowed ;
    } 
    if ( abs ( mem [p + 2 ].cint ) > maxallowed ) 
    {
      chopped = true ;
      if ( mem [p + 2 ].cint > 0 ) 
      mem [p + 2 ].cint = maxallowed ;
      else mem [p + 2 ].cint = - (integer) maxallowed ;
    } 
    if ( abs ( mem [p + 5 ].cint ) > maxallowed ) 
    {
      chopped = true ;
      if ( mem [p + 5 ].cint > 0 ) 
      mem [p + 5 ].cint = maxallowed ;
      else mem [p + 5 ].cint = - (integer) maxallowed ;
    } 
    if ( abs ( mem [p + 6 ].cint ) > maxallowed ) 
    {
      chopped = true ;
      if ( mem [p + 6 ].cint > 0 ) 
      mem [p + 6 ].cint = maxallowed ;
      else mem [p + 6 ].cint = - (integer) maxallowed ;
    } 
    p = mem [p ].hhfield .v.RH ;
    mem [p ].hhfield .b0 = k ;
    if ( k < 255 ) 
    incr ( k ) ;
    else k = 1 ;
  } while ( ! ( p == curspec ) ) ;
  if ( chopped ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 261 ) ;
      print ( 562 ) ;
    } 
    {
      helpptr = 4 ;
      helpline [3 ]= 563 ;
      helpline [2 ]= 564 ;
      helpline [1 ]= 565 ;
      helpline [0 ]= 566 ;
    } 
    putgeterror () ;
  } 
  quadrantsubdivide () ;
  if ( internal [36 ]> 0 ) 
  xyround () ;
  octantsubdivide () ;
  if ( internal [36 ]> 65536L ) 
  diaground () ;
  p = curspec ;
  do {
      lab22: q = mem [p ].hhfield .v.RH ;
    if ( p != q ) 
    {
      if ( mem [p + 1 ].cint == mem [p + 5 ].cint ) 
      if ( mem [p + 2 ].cint == mem [p + 6 ].cint ) 
      if ( mem [p + 1 ].cint == mem [q + 3 ].cint ) 
      if ( mem [p + 2 ].cint == mem [q + 4 ].cint ) 
      {
	unskew ( mem [q + 1 ].cint , mem [q + 2 ].cint , mem [q ]
	.hhfield .b1 ) ;
	skew ( curx , cury , mem [p ].hhfield .b1 ) ;
	if ( mem [p + 1 ].cint == curx ) 
	if ( mem [p + 2 ].cint == cury ) 
	{
	  removecubic ( p ) ;
	  if ( q != curspec ) 
	  goto lab22 ;
	  curspec = p ;
	  q = p ;
	} 
      } 
    } 
    p = q ;
  } while ( ! ( p == curspec ) ) ;
  turningnumber = 0 ;
  p = curspec ;
  q = mem [p ].hhfield .v.RH ;
  do {
      r = mem [q ].hhfield .v.RH ;
    if ( ( mem [p ].hhfield .b1 != mem [q ].hhfield .b1 ) || ( q == r ) ) 
    {
      newboundary ( p , mem [p ].hhfield .b1 ) ;
      s = mem [p ].hhfield .v.RH ;
      o1 = octantnumber [mem [p ].hhfield .b1 ];
      o2 = octantnumber [mem [q ].hhfield .b1 ];
      switch ( o2 - o1 ) 
      {case 1 : 
      case -7 : 
      case 7 : 
      case -1 : 
	goto lab30 ;
	break ;
      case 2 : 
      case -6 : 
	clockwise = false ;
	break ;
      case 3 : 
      case -5 : 
      case 4 : 
      case -4 : 
      case 5 : 
      case -3 : 
	{
	  dx1 = mem [s + 1 ].cint - mem [s + 3 ].cint ;
	  dy1 = mem [s + 2 ].cint - mem [s + 4 ].cint ;
	  if ( dx1 == 0 ) 
	  if ( dy1 == 0 ) 
	  {
	    dx1 = mem [s + 1 ].cint - mem [p + 5 ].cint ;
	    dy1 = mem [s + 2 ].cint - mem [p + 6 ].cint ;
	    if ( dx1 == 0 ) 
	    if ( dy1 == 0 ) 
	    {
	      dx1 = mem [s + 1 ].cint - mem [p + 1 ].cint ;
	      dy1 = mem [s + 2 ].cint - mem [p + 2 ].cint ;
	    } 
	  } 
	  dmax = abs ( dx1 ) ;
	  if ( abs ( dy1 ) > dmax ) 
	  dmax = abs ( dy1 ) ;
	  while ( dmax < 268435456L ) {
	      
	    dmax = dmax + dmax ;
	    dx1 = dx1 + dx1 ;
	    dy1 = dy1 + dy1 ;
	  } 
	  dx2 = mem [q + 5 ].cint - mem [q + 1 ].cint ;
	  dy2 = mem [q + 6 ].cint - mem [q + 2 ].cint ;
	  if ( dx2 == 0 ) 
	  if ( dy2 == 0 ) 
	  {
	    dx2 = mem [r + 3 ].cint - mem [q + 1 ].cint ;
	    dy2 = mem [r + 4 ].cint - mem [q + 2 ].cint ;
	    if ( dx2 == 0 ) 
	    if ( dy2 == 0 ) 
	    {
	      if ( mem [r ].hhfield .b1 == 0 ) 
	      {
		curx = mem [r + 1 ].cint ;
		cury = mem [r + 2 ].cint ;
	      } 
	      else {
		  
		unskew ( mem [r + 1 ].cint , mem [r + 2 ].cint , mem [r ]
		.hhfield .b1 ) ;
		skew ( curx , cury , mem [q ].hhfield .b1 ) ;
	      } 
	      dx2 = curx - mem [q + 1 ].cint ;
	      dy2 = cury - mem [q + 2 ].cint ;
	    } 
	  } 
	  dmax = abs ( dx2 ) ;
	  if ( abs ( dy2 ) > dmax ) 
	  dmax = abs ( dy2 ) ;
	  while ( dmax < 268435456L ) {
	      
	    dmax = dmax + dmax ;
	    dx2 = dx2 + dx2 ;
	    dy2 = dy2 + dy2 ;
	  } 
	  unskew ( dx1 , dy1 , mem [p ].hhfield .b1 ) ;
	  del = pythadd ( curx , cury ) ;
	  dx1 = makefraction ( curx , del ) ;
	  dy1 = makefraction ( cury , del ) ;
	  unskew ( dx2 , dy2 , mem [q ].hhfield .b1 ) ;
	  del = pythadd ( curx , cury ) ;
	  dx2 = makefraction ( curx , del ) ;
	  dy2 = makefraction ( cury , del ) ;
	  del = takefraction ( dx1 , dy2 ) - takefraction ( dx2 , dy1 ) ;
	  if ( del > 4684844L ) 
	  clockwise = false ;
	  else if ( del < -4684844L ) 
	  clockwise = true ;
	  else clockwise = revturns ;
	} 
	break ;
      case 6 : 
      case -2 : 
	clockwise = true ;
	break ;
      case 0 : 
	clockwise = revturns ;
	break ;
      } 
      while ( true ) {
	  
	if ( clockwise ) 
	if ( o1 == 1 ) 
	o1 = 8 ;
	else decr ( o1 ) ;
	else if ( o1 == 8 ) 
	o1 = 1 ;
	else incr ( o1 ) ;
	if ( o1 == o2 ) 
	goto lab30 ;
	newboundary ( s , octantcode [o1 ]) ;
	s = mem [s ].hhfield .v.RH ;
	mem [s + 3 ].cint = mem [s + 5 ].cint ;
      } 
      lab30: if ( q == r ) 
      {
	q = mem [q ].hhfield .v.RH ;
	r = q ;
	p = s ;
	mem [s ].hhfield .v.RH = q ;
	mem [q + 3 ].cint = mem [q + 5 ].cint ;
	mem [q ].hhfield .b0 = 0 ;
	freenode ( curspec , 7 ) ;
	curspec = q ;
      } 
      p = mem [p ].hhfield .v.RH ;
      do {
	  s = mem [p ].hhfield .v.RH ;
	o1 = octantnumber [mem [p + 5 ].cint ];
	o2 = octantnumber [mem [s + 3 ].cint ];
	if ( abs ( o1 - o2 ) == 1 ) 
	{
	  if ( o2 < o1 ) 
	  o2 = o1 ;
	  if ( odd ( o2 ) ) 
	  mem [p + 6 ].cint = 0 ;
	  else mem [p + 6 ].cint = 1 ;
	} 
	else {
	    
	  if ( o1 == 8 ) 
	  incr ( turningnumber ) ;
	  else decr ( turningnumber ) ;
	  mem [p + 6 ].cint = 0 ;
	} 
	mem [s + 4 ].cint = mem [p + 6 ].cint ;
	p = s ;
      } while ( ! ( p == q ) ) ;
    } 
    p = q ;
    q = r ;
  } while ( ! ( p == curspec ) ) ;
  while ( mem [curspec ].hhfield .b0 != 0 ) curspec = mem [curspec ]
  .hhfield .v.RH ;
  if ( tracing > 0 ) 
  if ( internal [36 ]<= 0 ) 
  printspec ( 559 ) ;
  else if ( internal [36 ]> 65536L ) 
  printspec ( 560 ) ;
  else printspec ( 561 ) ;
  Result = curspec ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zendround ( scaled x , scaled y ) 
#else
zendround ( x , y ) 
  scaled x ;
  scaled y ;
#endif
{
  y = y + 32768L - ycorr [octant ];
  x = x + y - xcorr [octant ];
  m1 = floorunscaled ( x ) ;
  n1 = floorunscaled ( y ) ;
  if ( x - 65536L * m1 >= y - 65536L * n1 + zcorr [octant ]) 
  d1 = 1 ;
  else d1 = 0 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zfillspec ( halfword h ) 
#else
zfillspec ( h ) 
  halfword h ;
#endif
{
  halfword p, q, r, s  ;
  if ( internal [10 ]> 0 ) 
  beginedgetracing () ;
  p = h ;
  do {
      octant = mem [p + 3 ].cint ;
    q = p ;
    while ( mem [q ].hhfield .b1 != 0 ) q = mem [q ].hhfield .v.RH ;
    if ( q != p ) 
    {
      endround ( mem [p + 1 ].cint , mem [p + 2 ].cint ) ;
      m0 = m1 ;
      n0 = n1 ;
      d0 = d1 ;
      endround ( mem [q + 1 ].cint , mem [q + 2 ].cint ) ;
      if ( n1 - n0 >= movesize ) 
      overflow ( 539 , movesize ) ;
      move [0 ]= d0 ;
      moveptr = 0 ;
      r = p ;
      do {
	  s = mem [r ].hhfield .v.RH ;
	makemoves ( mem [r + 1 ].cint , mem [r + 5 ].cint , mem [s + 3 ]
	.cint , mem [s + 1 ].cint , mem [r + 2 ].cint + 32768L , mem [r + 
	6 ].cint + 32768L , mem [s + 4 ].cint + 32768L , mem [s + 2 ]
	.cint + 32768L , xycorr [octant ], ycorr [octant ]) ;
	r = s ;
      } while ( ! ( r == q ) ) ;
      move [moveptr ]= move [moveptr ]- d1 ;
      if ( internal [35 ]> 0 ) 
      smoothmoves ( 0 , moveptr ) ;
      movetoedges ( m0 , n0 , m1 , n1 ) ;
    } 
    p = mem [q ].hhfield .v.RH ;
  } while ( ! ( p == h ) ) ;
  tossknotlist ( h ) ;
  if ( internal [10 ]> 0 ) 
  endedgetracing () ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdupoffset ( halfword w ) 
#else
zdupoffset ( w ) 
  halfword w ;
#endif
{
  halfword r  ;
  r = getnode ( 3 ) ;
  mem [r + 1 ].cint = mem [w + 1 ].cint ;
  mem [r + 2 ].cint = mem [w + 2 ].cint ;
  mem [r ].hhfield .v.RH = mem [w ].hhfield .v.RH ;
  mem [mem [w ].hhfield .v.RH ].hhfield .lhfield = r ;
  mem [r ].hhfield .lhfield = w ;
  mem [w ].hhfield .v.RH = r ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zmakepen ( halfword h ) 
#else
zmakepen ( h ) 
  halfword h ;
#endif
{
  /* 30 31 45 40 */ register halfword Result; smallnumber o, oo, k  ;
  halfword p  ;
  halfword q, r, s, w, hh  ;
  integer n  ;
  scaled dx, dy  ;
  scaled mc  ;
  q = h ;
  r = mem [q ].hhfield .v.RH ;
  mc = abs ( mem [h + 1 ].cint ) ;
  if ( q == r ) 
  {
    hh = h ;
    mem [h ].hhfield .b1 = 0 ;
    if ( mc < abs ( mem [h + 2 ].cint ) ) 
    mc = abs ( mem [h + 2 ].cint ) ;
  } 
  else {
      
    o = 0 ;
    hh = 0 ;
    while ( true ) {
	
      s = mem [r ].hhfield .v.RH ;
      if ( mc < abs ( mem [r + 1 ].cint ) ) 
      mc = abs ( mem [r + 1 ].cint ) ;
      if ( mc < abs ( mem [r + 2 ].cint ) ) 
      mc = abs ( mem [r + 2 ].cint ) ;
      dx = mem [r + 1 ].cint - mem [q + 1 ].cint ;
      dy = mem [r + 2 ].cint - mem [q + 2 ].cint ;
      if ( dx == 0 ) 
      if ( dy == 0 ) 
      goto lab45 ;
      if ( abvscd ( dx , mem [s + 2 ].cint - mem [r + 2 ].cint , dy , mem 
      [s + 1 ].cint - mem [r + 1 ].cint ) < 0 ) 
      goto lab45 ;
      if ( dx > 0 ) 
      octant = 1 ;
      else if ( dx == 0 ) 
      if ( dy > 0 ) 
      octant = 1 ;
      else octant = 2 ;
      else {
	  
	dx = - (integer) dx ;
	octant = 2 ;
      } 
      if ( dy < 0 ) 
      {
	dy = - (integer) dy ;
	octant = octant + 2 ;
      } 
      else if ( dy == 0 ) 
      if ( octant > 1 ) 
      octant = 4 ;
      if ( dx < dy ) 
      octant = octant + 4 ;
      mem [q ].hhfield .b1 = octant ;
      oo = octantnumber [octant ];
      if ( o > oo ) 
      {
	if ( hh != 0 ) 
	goto lab45 ;
	hh = q ;
      } 
      o = oo ;
      if ( ( q == h ) && ( hh != 0 ) ) 
      goto lab30 ;
      q = r ;
      r = s ;
    } 
    lab30: ;
  } 
  if ( mc >= 268402688L ) 
  goto lab45 ;
  p = getnode ( 10 ) ;
  q = hh ;
  mem [p + 9 ].cint = mc ;
  mem [p ].hhfield .lhfield = 0 ;
  if ( mem [q ].hhfield .v.RH != q ) 
  mem [p ].hhfield .v.RH = 1 ;
  {register integer for_end; k = 1 ;for_end = 8 ; if ( k <= for_end) do 
    {
      octant = octantcode [k ];
      n = 0 ;
      h = p + octant ;
      while ( true ) {
	  
	r = getnode ( 3 ) ;
	skew ( mem [q + 1 ].cint , mem [q + 2 ].cint , octant ) ;
	mem [r + 1 ].cint = curx ;
	mem [r + 2 ].cint = cury ;
	if ( n == 0 ) 
	mem [h ].hhfield .v.RH = r ;
	else if ( odd ( k ) ) 
	{
	  mem [w ].hhfield .v.RH = r ;
	  mem [r ].hhfield .lhfield = w ;
	} 
	else {
	    
	  mem [w ].hhfield .lhfield = r ;
	  mem [r ].hhfield .v.RH = w ;
	} 
	w = r ;
	if ( mem [q ].hhfield .b1 != octant ) 
	goto lab31 ;
	q = mem [q ].hhfield .v.RH ;
	incr ( n ) ;
      } 
      lab31: r = mem [h ].hhfield .v.RH ;
      if ( odd ( k ) ) 
      {
	mem [w ].hhfield .v.RH = r ;
	mem [r ].hhfield .lhfield = w ;
      } 
      else {
	  
	mem [w ].hhfield .lhfield = r ;
	mem [r ].hhfield .v.RH = w ;
	mem [h ].hhfield .v.RH = w ;
	r = w ;
      } 
      if ( ( mem [r + 2 ].cint != mem [mem [r ].hhfield .v.RH + 2 ].cint 
      ) || ( n == 0 ) ) 
      {
	dupoffset ( r ) ;
	incr ( n ) ;
      } 
      r = mem [r ].hhfield .lhfield ;
      if ( mem [r + 1 ].cint != mem [mem [r ].hhfield .lhfield + 1 ]
      .cint ) 
      dupoffset ( r ) ;
      else decr ( n ) ;
      if ( n >= 255 ) 
      overflow ( 578 , 255 ) ;
      mem [h ].hhfield .lhfield = n ;
    } 
  while ( k++ < for_end ) ;} 
  goto lab40 ;
  lab45: p = 3 ;
  if ( mc >= 268402688L ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 261 ) ;
      print ( 572 ) ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 573 ;
      helpline [0 ]= 574 ;
    } 
  } 
  else {
      
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 261 ) ;
      print ( 575 ) ;
    } 
    {
      helpptr = 3 ;
      helpline [2 ]= 576 ;
      helpline [1 ]= 577 ;
      helpline [0 ]= 574 ;
    } 
  } 
  putgeterror () ;
  lab40: if ( internal [6 ]> 0 ) 
  printpen ( p , 571 , true ) ;
  Result = p ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
ztrivialknot ( scaled x , scaled y ) 
#else
ztrivialknot ( x , y ) 
  scaled x ;
  scaled y ;
#endif
{
  register halfword Result; halfword p  ;
  p = getnode ( 7 ) ;
  mem [p ].hhfield .b0 = 1 ;
  mem [p ].hhfield .b1 = 1 ;
  mem [p + 1 ].cint = x ;
  mem [p + 3 ].cint = x ;
  mem [p + 5 ].cint = x ;
  mem [p + 2 ].cint = y ;
  mem [p + 4 ].cint = y ;
  mem [p + 6 ].cint = y ;
  Result = p ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zmakepath ( halfword penhead ) 
#else
zmakepath ( penhead ) 
  halfword penhead ;
#endif
{
  register halfword Result; halfword p  ;
  char k  ;
  halfword h  ;
  integer m, n  ;
  halfword w, ww  ;
  p = memtop - 1 ;
  {register integer for_end; k = 1 ;for_end = 8 ; if ( k <= for_end) do 
    {
      octant = octantcode [k ];
      h = penhead + octant ;
      n = mem [h ].hhfield .lhfield ;
      w = mem [h ].hhfield .v.RH ;
      if ( ! odd ( k ) ) 
      w = mem [w ].hhfield .lhfield ;
      {register integer for_end; m = 1 ;for_end = n + 1 ; if ( m <= for_end) 
      do 
	{
	  if ( odd ( k ) ) 
	  ww = mem [w ].hhfield .v.RH ;
	  else ww = mem [w ].hhfield .lhfield ;
	  if ( ( mem [ww + 1 ].cint != mem [w + 1 ].cint ) || ( mem [ww + 
	  2 ].cint != mem [w + 2 ].cint ) ) 
	  {
	    unskew ( mem [ww + 1 ].cint , mem [ww + 2 ].cint , octant ) ;
	    mem [p ].hhfield .v.RH = trivialknot ( curx , cury ) ;
	    p = mem [p ].hhfield .v.RH ;
	  } 
	  w = ww ;
	} 
      while ( m++ < for_end ) ;} 
    } 
  while ( k++ < for_end ) ;} 
  if ( p == memtop - 1 ) 
  {
    w = mem [penhead + 1 ].hhfield .v.RH ;
    p = trivialknot ( mem [w + 1 ].cint + mem [w + 2 ].cint , mem [w + 2 
    ].cint ) ;
    mem [memtop - 1 ].hhfield .v.RH = p ;
  } 
  mem [p ].hhfield .v.RH = mem [memtop - 1 ].hhfield .v.RH ;
  Result = mem [memtop - 1 ].hhfield .v.RH ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zfindoffset ( scaled x , scaled y , halfword p ) 
#else
zfindoffset ( x , y , p ) 
  scaled x ;
  scaled y ;
  halfword p ;
#endif
{
  /* 30 10 */ char octant  ;
  schar s  ;
  integer n  ;
  halfword h, w, ww  ;
  if ( x > 0 ) 
  octant = 1 ;
  else if ( x == 0 ) 
  if ( y <= 0 ) 
  if ( y == 0 ) 
  {
    curx = 0 ;
    cury = 0 ;
    goto lab10 ;
  } 
  else octant = 2 ;
  else octant = 1 ;
  else {
      
    x = - (integer) x ;
    if ( y == 0 ) 
    octant = 4 ;
    else octant = 2 ;
  } 
  if ( y < 0 ) 
  {
    octant = octant + 2 ;
    y = - (integer) y ;
  } 
  if ( x >= y ) 
  x = x - y ;
  else {
      
    octant = octant + 4 ;
    x = y - x ;
    y = y - x ;
  } 
  if ( odd ( octantnumber [octant ]) ) 
  s = -1 ;
  else s = 1 ;
  h = p + octant ;
  w = mem [mem [h ].hhfield .v.RH ].hhfield .v.RH ;
  ww = mem [w ].hhfield .v.RH ;
  n = mem [h ].hhfield .lhfield ;
  while ( n > 1 ) {
      
    if ( abvscd ( x , mem [ww + 2 ].cint - mem [w + 2 ].cint , y , mem [
    ww + 1 ].cint - mem [w + 1 ].cint ) != s ) 
    goto lab30 ;
    w = ww ;
    ww = mem [w ].hhfield .v.RH ;
    decr ( n ) ;
  } 
  lab30: unskew ( mem [w + 1 ].cint , mem [w + 2 ].cint , octant ) ;
  lab10: ;
} 
