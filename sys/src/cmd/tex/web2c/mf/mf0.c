#define EXTERN extern
#include "mfd.h"

void initialize ( ) 
{integer i  ; 
  integer k  ; 
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
  {register integer for_end; i = 0 ; for_end = 31 ; if ( i <= for_end) do 
    xchr [ i ] = chr ( i ) ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 127 ; for_end = 255 ; if ( i <= for_end) do 
    xchr [ i ] = chr ( i ) ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 0 ; for_end = 255 ; if ( i <= for_end) do 
    xord [ chr ( i ) ] = 127 ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 128 ; for_end = 255 ; if ( i <= for_end) do 
    xord [ xchr [ i ] ] = i ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 0 ; for_end = 126 ; if ( i <= for_end) do 
    xord [ xchr [ i ] ] = i ; 
  while ( i++ < for_end ) ; } 
  interaction = 3 ; 
  deletionsallowed = true ; 
  errorcount = 0 ; 
  helpptr = 0 ; 
  useerrhelp = false ; 
  errhelp = 0 ; 
  interrupt = 0 ; 
  OKtointerrupt = true ; 
  aritherror = false ; 
  twotothe [ 0 ] = 1 ; 
  {register integer for_end; k = 1 ; for_end = 30 ; if ( k <= for_end) do 
    twotothe [ k ] = 2 * twotothe [ k - 1 ] ; 
  while ( k++ < for_end ) ; } 
  speclog [ 1 ] = 93032640L ; 
  speclog [ 2 ] = 38612034L ; 
  speclog [ 3 ] = 17922280L ; 
  speclog [ 4 ] = 8662214L ; 
  speclog [ 5 ] = 4261238L ; 
  speclog [ 6 ] = 2113709L ; 
  speclog [ 7 ] = 1052693L ; 
  speclog [ 8 ] = 525315L ; 
  speclog [ 9 ] = 262400L ; 
  speclog [ 10 ] = 131136L ; 
  speclog [ 11 ] = 65552L ; 
  speclog [ 12 ] = 32772L ; 
  speclog [ 13 ] = 16385 ; 
  {register integer for_end; k = 14 ; for_end = 27 ; if ( k <= for_end) do 
    speclog [ k ] = twotothe [ 27 - k ] ; 
  while ( k++ < for_end ) ; } 
  speclog [ 28 ] = 1 ; 
  specatan [ 1 ] = 27855475L ; 
  specatan [ 2 ] = 14718068L ; 
  specatan [ 3 ] = 7471121L ; 
  specatan [ 4 ] = 3750058L ; 
  specatan [ 5 ] = 1876857L ; 
  specatan [ 6 ] = 938658L ; 
  specatan [ 7 ] = 469357L ; 
  specatan [ 8 ] = 234682L ; 
  specatan [ 9 ] = 117342L ; 
  specatan [ 10 ] = 58671L ; 
  specatan [ 11 ] = 29335 ; 
  specatan [ 12 ] = 14668 ; 
  specatan [ 13 ] = 7334 ; 
  specatan [ 14 ] = 3667 ; 
  specatan [ 15 ] = 1833 ; 
  specatan [ 16 ] = 917 ; 
  specatan [ 17 ] = 458 ; 
  specatan [ 18 ] = 229 ; 
  specatan [ 19 ] = 115 ; 
  specatan [ 20 ] = 57 ; 
  specatan [ 21 ] = 29 ; 
  specatan [ 22 ] = 14 ; 
  specatan [ 23 ] = 7 ; 
  specatan [ 24 ] = 4 ; 
  specatan [ 25 ] = 2 ; 
  specatan [ 26 ] = 1 ; 
	;
#ifdef DEBUG
  wasmemend = 0 ; 
  waslomax = 0 ; 
  washimin = memmax ; 
  panicking = false ; 
#endif /* DEBUG */
  {register integer for_end; k = 1 ; for_end = 41 ; if ( k <= for_end) do 
    internal [ k ] = 0 ; 
  while ( k++ < for_end ) ; } 
  intptr = 41 ; 
  {register integer for_end; k = 48 ; for_end = 57 ; if ( k <= for_end) do 
    charclass [ k ] = 0 ; 
  while ( k++ < for_end ) ; } 
  charclass [ 46 ] = 1 ; 
  charclass [ 32 ] = 2 ; 
  charclass [ 37 ] = 3 ; 
  charclass [ 34 ] = 4 ; 
  charclass [ 44 ] = 5 ; 
  charclass [ 59 ] = 6 ; 
  charclass [ 40 ] = 7 ; 
  charclass [ 41 ] = 8 ; 
  {register integer for_end; k = 65 ; for_end = 90 ; if ( k <= for_end) do 
    charclass [ k ] = 9 ; 
  while ( k++ < for_end ) ; } 
  {register integer for_end; k = 97 ; for_end = 122 ; if ( k <= for_end) do 
    charclass [ k ] = 9 ; 
  while ( k++ < for_end ) ; } 
  charclass [ 95 ] = 9 ; 
  charclass [ 60 ] = 10 ; 
  charclass [ 61 ] = 10 ; 
  charclass [ 62 ] = 10 ; 
  charclass [ 58 ] = 10 ; 
  charclass [ 124 ] = 10 ; 
  charclass [ 96 ] = 11 ; 
  charclass [ 39 ] = 11 ; 
  charclass [ 43 ] = 12 ; 
  charclass [ 45 ] = 12 ; 
  charclass [ 47 ] = 13 ; 
  charclass [ 42 ] = 13 ; 
  charclass [ 92 ] = 13 ; 
  charclass [ 33 ] = 14 ; 
  charclass [ 63 ] = 14 ; 
  charclass [ 35 ] = 15 ; 
  charclass [ 38 ] = 15 ; 
  charclass [ 64 ] = 15 ; 
  charclass [ 36 ] = 15 ; 
  charclass [ 94 ] = 16 ; 
  charclass [ 126 ] = 16 ; 
  charclass [ 91 ] = 17 ; 
  charclass [ 93 ] = 18 ; 
  charclass [ 123 ] = 19 ; 
  charclass [ 125 ] = 19 ; 
  {register integer for_end; k = 0 ; for_end = 31 ; if ( k <= for_end) do 
    charclass [ k ] = 20 ; 
  while ( k++ < for_end ) ; } 
  {register integer for_end; k = 127 ; for_end = 255 ; if ( k <= for_end) do 
    charclass [ k ] = 20 ; 
  while ( k++ < for_end ) ; } 
  charclass [ 9 ] = 2 ; 
  charclass [ 12 ] = 2 ; 
  hash [ 1 ] .lhfield = 0 ; 
  hash [ 1 ] .v.RH = 0 ; 
  eqtb [ 1 ] .lhfield = 41 ; 
  eqtb [ 1 ] .v.RH = 0 ; 
  {register integer for_end; k = 2 ; for_end = 9769 ; if ( k <= for_end) do 
    {
      hash [ k ] = hash [ 1 ] ; 
      eqtb [ k ] = eqtb [ 1 ] ; 
    } 
  while ( k++ < for_end ) ; } 
  bignodesize [ 13 ] = 12 ; 
  bignodesize [ 14 ] = 4 ; 
  saveptr = 0 ; 
  octantdir [ 1 ] = 547 ; 
  octantdir [ 5 ] = 548 ; 
  octantdir [ 6 ] = 549 ; 
  octantdir [ 2 ] = 550 ; 
  octantdir [ 4 ] = 551 ; 
  octantdir [ 8 ] = 552 ; 
  octantdir [ 7 ] = 553 ; 
  octantdir [ 3 ] = 554 ; 
  maxroundingptr = 0 ; 
  octantcode [ 1 ] = 1 ; 
  octantcode [ 2 ] = 5 ; 
  octantcode [ 3 ] = 6 ; 
  octantcode [ 4 ] = 2 ; 
  octantcode [ 5 ] = 4 ; 
  octantcode [ 6 ] = 8 ; 
  octantcode [ 7 ] = 7 ; 
  octantcode [ 8 ] = 3 ; 
  {register integer for_end; k = 1 ; for_end = 8 ; if ( k <= for_end) do 
    octantnumber [ octantcode [ k ] ] = k ; 
  while ( k++ < for_end ) ; } 
  revturns = false ; 
  xcorr [ 1 ] = 0 ; 
  ycorr [ 1 ] = 0 ; 
  xycorr [ 1 ] = 0 ; 
  xcorr [ 5 ] = 0 ; 
  ycorr [ 5 ] = 0 ; 
  xycorr [ 5 ] = 1 ; 
  xcorr [ 6 ] = -1 ; 
  ycorr [ 6 ] = 1 ; 
  xycorr [ 6 ] = 0 ; 
  xcorr [ 2 ] = 1 ; 
  ycorr [ 2 ] = 0 ; 
  xycorr [ 2 ] = 1 ; 
  xcorr [ 4 ] = 0 ; 
  ycorr [ 4 ] = 1 ; 
  xycorr [ 4 ] = 1 ; 
  xcorr [ 8 ] = 0 ; 
  ycorr [ 8 ] = 1 ; 
  xycorr [ 8 ] = 0 ; 
  xcorr [ 7 ] = 1 ; 
  ycorr [ 7 ] = 0 ; 
  xycorr [ 7 ] = 1 ; 
  xcorr [ 3 ] = -1 ; 
  ycorr [ 3 ] = 1 ; 
  xycorr [ 3 ] = 0 ; 
  {register integer for_end; k = 1 ; for_end = 8 ; if ( k <= for_end) do 
    zcorr [ k ] = xycorr [ k ] - xcorr [ k ] ; 
  while ( k++ < for_end ) ; } 
  screenstarted = false ; 
  screenOK = false ; 
  {register integer for_end; k = 0 ; for_end = 15 ; if ( k <= for_end) do 
    {
      windowopen [ k ] = false ; 
      windowtime [ k ] = 0 ; 
    } 
  while ( k++ < for_end ) ; } 
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
  {register integer for_end; k = 0 ; for_end = 255 ; if ( k <= for_end) do 
    {
      tfmwidth [ k ] = 0 ; 
      tfmheight [ k ] = 0 ; 
      tfmdepth [ k ] = 0 ; 
      tfmitalcorr [ k ] = 0 ; 
      charexists [ k ] = false ; 
      chartag [ k ] = 0 ; 
      charremainder [ k ] = 0 ; 
      skiptable [ k ] = ligtablesize ; 
    } 
  while ( k++ < for_end ) ; } 
  {register integer for_end; k = 1 ; for_end = headersize ; if ( k <= 
  for_end) do 
    headerbyte [ k ] = -1 ; 
  while ( k++ < for_end ) ; } 
  bc = 255 ; 
  ec = 0 ; 
  nl = 0 ; 
  nk = 0 ; 
  ne = 0 ; 
  np = 0 ; 
  internal [ 41 ] = -65536L ; 
  bchlabel = ligtablesize ; 
  labelloc [ 0 ] = -1 ; 
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
void println ( ) 
{switch ( selector ) 
  {case 3 : 
    {
      (void) putc('\n',  stdout );
      (void) putc('\n',  logfile );
      termoffset = 0 ; 
      fileoffset = 0 ; 
    } 
    break ; 
  case 2 : 
    {
      (void) putc('\n',  logfile );
      fileoffset = 0 ; 
    } 
    break ; 
  case 1 : 
    {
      (void) putc('\n',  stdout );
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
void zprintchar ( s ) 
ASCIIcode s ; 
{switch ( selector ) 
  {case 3 : 
    {
      (void) putc( xchr [ s ] ,  stdout );
      (void) putc( xchr [ s ] ,  logfile );
      incr ( termoffset ) ; 
      incr ( fileoffset ) ; 
      if ( termoffset == maxprintline ) 
      {
	(void) putc('\n',  stdout );
	termoffset = 0 ; 
      } 
      if ( fileoffset == maxprintline ) 
      {
	(void) putc('\n',  logfile );
	fileoffset = 0 ; 
      } 
    } 
    break ; 
  case 2 : 
    {
      (void) putc( xchr [ s ] ,  logfile );
      incr ( fileoffset ) ; 
      if ( fileoffset == maxprintline ) 
      println () ; 
    } 
    break ; 
  case 1 : 
    {
      (void) putc( xchr [ s ] ,  stdout );
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
    trickbuf [ tally % errorline ] = s ; 
    break ; 
  case 5 : 
    {
      if ( poolptr < poolsize ) 
      {
	strpool [ poolptr ] = s ; 
	incr ( poolptr ) ; 
      } 
    } 
    break ; 
  } 
  incr ( tally ) ; 
} 
void zprint ( s ) 
integer s ; 
{poolpointer j  ; 
  if ( ( s < 0 ) || ( s >= strptr ) ) 
  s = 259 ; 
  if ( ( s < 256 ) && ( selector > 4 ) ) 
  printchar ( s ) ; 
  else {
      
    j = strstart [ s ] ; 
    while ( j < strstart [ s + 1 ] ) {
	
      printchar ( strpool [ j ] ) ; 
      incr ( j ) ; 
    } 
  } 
} 
void zslowprint ( s ) 
integer s ; 
{poolpointer j  ; 
  if ( ( s < 0 ) || ( s >= strptr ) ) 
  s = 259 ; 
  if ( ( s < 256 ) && ( selector > 4 ) ) 
  printchar ( s ) ; 
  else {
      
    j = strstart [ s ] ; 
    while ( j < strstart [ s + 1 ] ) {
	
      print ( strpool [ j ] ) ; 
      incr ( j ) ; 
    } 
  } 
} 
void zprintnl ( s ) 
strnumber s ; 
{if ( ( ( termoffset > 0 ) && ( odd ( selector ) ) ) || ( ( fileoffset > 0 ) 
  && ( selector >= 2 ) ) ) 
  println () ; 
  print ( s ) ; 
} 
void zprintthedigs ( k ) 
eightbits k ; 
{while ( k > 0 ) {
    
    decr ( k ) ; 
    printchar ( 48 + dig [ k ] ) ; 
  } 
} 
void zprintint ( n ) 
integer n ; 
{schar k  ; 
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
      dig [ 0 ] = m ; 
      else {
	  
	dig [ 0 ] = 0 ; 
	incr ( n ) ; 
      } 
    } 
  } 
  do {
      dig [ k ] = n % 10 ; 
    n = n / 10 ; 
    incr ( k ) ; 
  } while ( ! ( n == 0 ) ) ; 
  printthedigs ( k ) ; 
} 
void zprintscaled ( s ) 
scaled s ; 
{scaled delta  ; 
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
void zprinttwo ( x , y ) 
scaled x ; 
scaled y ; 
{printchar ( 40 ) ; 
  printscaled ( x ) ; 
  printchar ( 44 ) ; 
  printscaled ( y ) ; 
  printchar ( 41 ) ; 
} 
void zprinttype ( t ) 
smallnumber t ; 
{switch ( t ) 
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
void begindiagnostic ( ) 
{oldsetting = selector ; 
  if ( ( internal [ 13 ] <= 0 ) && ( selector == 3 ) ) 
  {
    decr ( selector ) ; 
    if ( history == 0 ) 
    history = 1 ; 
  } 
} 
void zenddiagnostic ( blankline ) 
boolean blankline ; 
{printnl ( 283 ) ; 
  if ( blankline ) 
  println () ; 
  selector = oldsetting ; 
} 
void zprintdiagnostic ( s , t , nuline ) 
strnumber s ; 
strnumber t ; 
boolean nuline ; 
{begindiagnostic () ; 
  if ( nuline ) 
  printnl ( s ) ; 
  else print ( s ) ; 
  print ( 449 ) ; 
  printint ( line ) ; 
  print ( t ) ; 
  printchar ( 58 ) ; 
} 
void zprintfilename ( n , a , e ) 
integer n ; 
integer a ; 
integer e ; 
{slowprint ( a ) ; 
  slowprint ( n ) ; 
  slowprint ( e ) ; 
} 
#ifdef DEBUG
#endif /* DEBUG */
void zflushstring ( s ) 
strnumber s ; 
{if ( s < strptr - 1 ) 
  strref [ s ] = 0 ; 
  else do {
      decr ( strptr ) ; 
  } while ( ! ( strref [ strptr - 1 ] != 0 ) ) ; 
  poolptr = strstart [ strptr ] ; 
} 
void jumpout ( ) 
{closefilesandterminate () ; 
  {
    flush ( stdout ) ; 
    readyalready = 0 ; 
    if ( ( history != 0 ) && ( history != 1 ) ) 
    uexit ( 1 ) ; 
    else uexit ( 0 ) ; 
  } 
} 
void error ( ) 
{/* 22 10 */ ASCIIcode c  ; 
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
    c = buffer [ first ] ; 
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
	if ( ( last > first + 1 ) && ( buffer [ first + 1 ] >= 48 ) && ( 
	buffer [ first + 1 ] <= 57 ) ) 
	c = c * 10 + buffer [ first + 1 ] - 48 * 11 ; 
	else c = c - 48 ; 
	while ( c > 0 ) {
	    
	  getnext () ; 
	  if ( curcmd == 39 ) 
	  {
	    if ( strref [ curmod ] < 127 ) 
	    if ( strref [ curmod ] > 1 ) 
	    decr ( strref [ curmod ] ) ; 
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
	  helpline [ 1 ] = 276 ; 
	  helpline [ 0 ] = 277 ; 
	} 
	showcontext () ; 
	goto lab22 ; 
      } 
      break ; 
	;
#ifdef DEBUG
    case 68 : 
      {
	debughelp () ; 
	goto lab22 ; 
      } 
      break ; 
#endif /* DEBUG */
    case 69 : 
      if ( fileptr > 0 ) 
      {
	editnamestart = strstart [ inputstack [ fileptr ] .namefield ] ; 
	editnamelength = strstart [ inputstack [ fileptr ] .namefield + 1 ] - 
	strstart [ inputstack [ fileptr ] .namefield ] ; 
	editline = line ; 
	jumpout () ; 
      } 
      break ; 
    case 72 : 
      {
	if ( useerrhelp ) 
	{
	  j = strstart [ errhelp ] ; 
	  while ( j < strstart [ errhelp + 1 ] ) {
	      
	    if ( strpool [ j ] != 37 ) 
	    print ( strpool [ j ] ) ; 
	    else if ( j + 1 == strstart [ errhelp + 1 ] ) 
	    println () ; 
	    else if ( strpool [ j + 1 ] != 37 ) 
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
	    helpline [ 1 ] = 278 ; 
	    helpline [ 0 ] = 279 ; 
	  } 
	  do {
	      decr ( helpptr ) ; 
	    print ( helpline [ helpptr ] ) ; 
	    println () ; 
	  } while ( ! ( helpptr == 0 ) ) ; 
	} 
	{
	  helpptr = 4 ; 
	  helpline [ 3 ] = 280 ; 
	  helpline [ 2 ] = 279 ; 
	  helpline [ 1 ] = 281 ; 
	  helpline [ 0 ] = 282 ; 
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
	  buffer [ first ] = 32 ; 
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
	flush ( stdout ) ; 
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
    j = strstart [ errhelp ] ; 
    while ( j < strstart [ errhelp + 1 ] ) {
	
      if ( strpool [ j ] != 37 ) 
      print ( strpool [ j ] ) ; 
      else if ( j + 1 == strstart [ errhelp + 1 ] ) 
      println () ; 
      else if ( strpool [ j + 1 ] != 37 ) 
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
    printnl ( helpline [ helpptr ] ) ; 
  } 
  println () ; 
  if ( interaction > 0 ) 
  incr ( selector ) ; 
  println () ; 
  lab10: ; 
} 
void zfatalerror ( s ) 
strnumber s ; 
{normalizeselector () ; 
  {
    if ( interaction == 3 ) 
    ; 
    printnl ( 261 ) ; 
    print ( 284 ) ; 
  } 
  {
    helpptr = 1 ; 
    helpline [ 0 ] = s ; 
  } 
  {
    if ( interaction == 3 ) 
    interaction = 2 ; 
    if ( logopened ) 
    error () ; 
	;
#ifdef DEBUG
    if ( interaction > 0 ) 
    debughelp () ; 
#endif /* DEBUG */
    history = 3 ; 
    jumpout () ; 
  } 
} 
void zoverflow ( s , n ) 
strnumber s ; 
integer n ; 
{normalizeselector () ; 
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
    helpline [ 1 ] = 286 ; 
    helpline [ 0 ] = 287 ; 
  } 
  {
    if ( interaction == 3 ) 
    interaction = 2 ; 
    if ( logopened ) 
    error () ; 
	;
#ifdef DEBUG
    if ( interaction > 0 ) 
    debughelp () ; 
#endif /* DEBUG */
    history = 3 ; 
    jumpout () ; 
  } 
} 
void zconfusion ( s ) 
strnumber s ; 
{normalizeselector () ; 
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
      helpline [ 0 ] = 289 ; 
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
      helpline [ 1 ] = 291 ; 
      helpline [ 0 ] = 292 ; 
    } 
  } 
  {
    if ( interaction == 3 ) 
    interaction = 2 ; 
    if ( logopened ) 
    error () ; 
	;
#ifdef DEBUG
    if ( interaction > 0 ) 
    debughelp () ; 
#endif /* DEBUG */
    history = 3 ; 
    jumpout () ; 
  } 
} 
boolean initterminal ( ) 
{/* 10 */ register boolean Result; topenin () ; 
  if ( last > first ) 
  {
    curinput .locfield = first ; 
    while ( ( curinput .locfield < last ) && ( buffer [ curinput .locfield ] 
    == ' ' ) ) incr ( curinput .locfield ) ; 
    if ( curinput .locfield < last ) 
    {
      Result = true ; 
      goto lab10 ; 
    } 
  } 
  while ( true ) {
      
    ; 
    (void) Fputs( stdout ,  "**" ) ; 
    flush ( stdout ) ; 
    if ( ! inputln ( stdin , true ) ) 
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "! End of file on the terminal... why?" ) ; 
      Result = false ; 
      goto lab10 ; 
    } 
    curinput .locfield = first ; 
    while ( ( curinput .locfield < last ) && ( buffer [ curinput .locfield ] 
    == 32 ) ) incr ( curinput .locfield ) ; 
    if ( curinput .locfield < last ) 
    {
      Result = true ; 
      goto lab10 ; 
    } 
    (void) fprintf( stdout , "%s\n",  "Please type the name of your input file." ) ; 
  } 
  lab10: ; 
  return(Result) ; 
} 
strnumber makestring ( ) 
{register strnumber Result; if ( strptr == maxstrptr ) 
  {
    if ( strptr == maxstrings ) 
    overflow ( 258 , maxstrings - initstrptr ) ; 
    incr ( maxstrptr ) ; 
  } 
  strref [ strptr ] = 1 ; 
  incr ( strptr ) ; 
  strstart [ strptr ] = poolptr ; 
  Result = strptr - 1 ; 
  return(Result) ; 
} 
boolean zstreqbuf ( s , k ) 
strnumber s ; 
integer k ; 
{/* 45 */ register boolean Result; poolpointer j  ; 
  boolean result  ; 
  j = strstart [ s ] ; 
  while ( j < strstart [ s + 1 ] ) {
      
    if ( strpool [ j ] != buffer [ k ] ) 
    {
      result = false ; 
      goto lab45 ; 
    } 
    incr ( j ) ; 
    incr ( k ) ; 
  } 
  result = true ; 
  lab45: Result = result ; 
  return(Result) ; 
} 
integer zstrvsstr ( s , t ) 
strnumber s ; 
strnumber t ; 
{/* 10 */ register integer Result; poolpointer j, k  ; 
  integer ls, lt  ; 
  integer l  ; 
  ls = ( strstart [ s + 1 ] - strstart [ s ] ) ; 
  lt = ( strstart [ t + 1 ] - strstart [ t ] ) ; 
  if ( ls <= lt ) 
  l = ls ; 
  else l = lt ; 
  j = strstart [ s ] ; 
  k = strstart [ t ] ; 
  while ( l > 0 ) {
      
    if ( strpool [ j ] != strpool [ k ] ) 
    {
      Result = strpool [ j ] - strpool [ k ] ; 
      goto lab10 ; 
    } 
    incr ( j ) ; 
    incr ( k ) ; 
    decr ( l ) ; 
  } 
  Result = ls - lt ; 
  lab10: ; 
  return(Result) ; 
} 
void zprintdd ( n ) 
integer n ; 
{n = abs ( n ) % 100 ; 
  printchar ( 48 + ( n / 10 ) ) ; 
  printchar ( 48 + ( n % 10 ) ) ; 
} 
void terminput ( ) 
{integer k  ; 
  flush ( stdout ) ; 
  if ( ! inputln ( stdin , true ) ) 
  fatalerror ( 260 ) ; 
  termoffset = 0 ; 
  decr ( selector ) ; 
  if ( last != first ) 
  {register integer for_end; k = first ; for_end = last - 1 ; if ( k <= 
  for_end) do 
    print ( buffer [ k ] ) ; 
  while ( k++ < for_end ) ; } 
  println () ; 
  buffer [ last ] = 37 ; 
  incr ( selector ) ; 
} 
void normalizeselector ( ) 
{if ( logopened ) 
  selector = 3 ; 
  else selector = 1 ; 
  if ( jobname == 0 ) 
  openlogfile () ; 
  if ( interaction == 0 ) 
  decr ( selector ) ; 
} 
void pauseforinstructions ( ) 
{if ( OKtointerrupt ) 
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
      helpline [ 2 ] = 294 ; 
      helpline [ 1 ] = 295 ; 
      helpline [ 0 ] = 296 ; 
    } 
    deletionsallowed = false ; 
    error () ; 
    deletionsallowed = true ; 
    interrupt = 0 ; 
  } 
} 
void zmissingerr ( s ) 
strnumber s ; 
{{
    
    if ( interaction == 3 ) 
    ; 
    printnl ( 261 ) ; 
    print ( 297 ) ; 
  } 
  print ( s ) ; 
  print ( 298 ) ; 
} 
void cleararith ( ) 
{{
    
    if ( interaction == 3 ) 
    ; 
    printnl ( 261 ) ; 
    print ( 299 ) ; 
  } 
  {
    helpptr = 4 ; 
    helpline [ 3 ] = 300 ; 
    helpline [ 2 ] = 301 ; 
    helpline [ 1 ] = 302 ; 
    helpline [ 0 ] = 303 ; 
  } 
  error () ; 
  aritherror = false ; 
} 
integer zslowadd ( x , y ) 
integer x ; 
integer y ; 
{register integer Result; if ( x >= 0 ) 
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
  return(Result) ; 
} 
scaled zrounddecimals ( k ) 
smallnumber k ; 
{register scaled Result; integer a  ; 
  a = 0 ; 
  while ( k > 0 ) {
      
    decr ( k ) ; 
    a = ( a + dig [ k ] * 131072L ) / 10 ; 
  } 
  Result = ( a + 1 ) / 2 ; 
  return(Result) ; 
} 
fraction zmakefraction ( p , q ) 
integer p ; 
integer q ; 
{register fraction Result; integer f  ; 
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
#ifdef DEBUG
    if ( q == 0 ) 
    confusion ( 47 ) ; 
#endif /* DEBUG */
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
  return(Result) ; 
} 
integer ztakefraction ( q , f ) 
integer q ; 
fraction f ; 
{register integer Result; integer p  ; 
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
    p = ( p + q ) / 2 ; 
    else p = ( p ) / 2 ; 
    f = ( f ) / 2 ; 
  } while ( ! ( f == 1 ) ) ; 
  else do {
      if ( odd ( f ) ) 
    p = p + ( q - p ) / 2 ; 
    else p = ( p ) / 2 ; 
    f = ( f ) / 2 ; 
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
  return(Result) ; 
} 
integer ztakescaled ( q , f ) 
integer q ; 
scaled f ; 
{register integer Result; integer p  ; 
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
    p = ( p + q ) / 2 ; 
    else p = ( p ) / 2 ; 
    f = ( f ) / 2 ; 
  } while ( ! ( f == 1 ) ) ; 
  else do {
      if ( odd ( f ) ) 
    p = p + ( q - p ) / 2 ; 
    else p = ( p ) / 2 ; 
    f = ( f ) / 2 ; 
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
  return(Result) ; 
} 
scaled zmakescaled ( p , q ) 
integer p ; 
integer q ; 
{register scaled Result; integer f  ; 
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
#ifdef DEBUG
    if ( q == 0 ) 
    confusion ( 47 ) ; 
#endif /* DEBUG */
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
  return(Result) ; 
} 
fraction zvelocity ( st , ct , sf , cf , t ) 
fraction st ; 
fraction ct ; 
fraction sf ; 
fraction cf ; 
scaled t ; 
{register fraction Result; integer acc, num, denom  ; 
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
  return(Result) ; 
} 
integer zabvscd ( a , b , c , d ) 
integer a ; 
integer b ; 
integer c ; 
integer d ; 
{/* 10 */ register integer Result; integer q, r  ; 
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
  return(Result) ; 
} 
scaled zfloorscaled ( x ) 
scaled x ; 
{register scaled Result; integer becareful  ; 
  if ( x >= 0 ) 
  Result = x - ( x % 65536L ) ; 
  else {
      
    becareful = x + 1 ; 
    Result = x + ( ( - (integer) becareful ) % 65536L ) - 65535L ; 
  } 
  return(Result) ; 
} 
integer zfloorunscaled ( x ) 
scaled x ; 
{register integer Result; integer becareful  ; 
  if ( x >= 0 ) 
  Result = x / 65536L ; 
  else {
      
    becareful = x + 1 ; 
    Result = - (integer) ( 1 + ( ( - (integer) becareful ) / 65536L ) ) ; 
  } 
  return(Result) ; 
} 
integer zroundunscaled ( x ) 
scaled x ; 
{register integer Result; integer becareful  ; 
  if ( x >= 32768L ) 
  Result = 1 + ( ( x - 32768L ) / 65536L ) ; 
  else if ( x >= -32768L ) 
  Result = 0 ; 
  else {
      
    becareful = x + 1 ; 
    Result = - (integer) ( 1 + ( ( - (integer) becareful - 32768L ) / 65536L ) 
    ) ; 
  } 
  return(Result) ; 
} 
scaled zroundfraction ( x ) 
fraction x ; 
{register scaled Result; integer becareful  ; 
  if ( x >= 2048 ) 
  Result = 1 + ( ( x - 2048 ) / 4096 ) ; 
  else if ( x >= -2048 ) 
  Result = 0 ; 
  else {
      
    becareful = x + 1 ; 
    Result = - (integer) ( 1 + ( ( - (integer) becareful - 2048 ) / 4096 ) ) ; 
  } 
  return(Result) ; 
} 
scaled zsquarert ( x ) 
scaled x ; 
{register scaled Result; smallnumber k  ; 
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
	helpline [ 1 ] = 306 ; 
	helpline [ 0 ] = 307 ; 
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
    Result = ( q ) / 2 ; 
  } 
  return(Result) ; 
} 
integer zpythadd ( a , b ) 
integer a ; 
integer b ; 
{/* 30 */ register integer Result; fraction r  ; 
  boolean big  ; 
  a = abs ( a ) ; 
  b = abs ( b ) ; 
  if ( a < b ) 
  {
    r = b ; 
    b = a ; 
    a = r ; 
  } 
  if ( a > 0 ) 
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
  return(Result) ; 
} 
integer zpythsub ( a , b ) 
integer a ; 
integer b ; 
{/* 30 */ register integer Result; fraction r  ; 
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
	helpline [ 1 ] = 306 ; 
	helpline [ 0 ] = 307 ; 
      } 
      error () ; 
    } 
    a = 0 ; 
  } 
  else {
      
    if ( a < 1073741824L ) 
    big = false ; 
    else {
	
      a = ( a ) / 2 ; 
      b = ( b ) / 2 ; 
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
  return(Result) ; 
} 
scaled zmlog ( x ) 
scaled x ; 
{register scaled Result; integer y, z  ; 
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
      helpline [ 1 ] = 311 ; 
      helpline [ 0 ] = 307 ; 
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
	
      z = ( ( x - 1 ) / twotothe [ k ] ) + 1 ; 
      while ( x < 1073741824L + z ) {
	  
	z = ( z + 1 ) / 2 ; 
	k = k + 1 ; 
      } 
      y = y + speclog [ k ] ; 
      x = x - z ; 
    } 
    Result = y / 8 ; 
  } 
  return(Result) ; 
} 
scaled zmexp ( x ) 
scaled x ; 
{register scaled Result; smallnumber k  ; 
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
	
      while ( z >= speclog [ k ] ) {
	  
	z = z - speclog [ k ] ; 
	y = y - 1 - ( ( y - twotothe [ k - 1 ] ) / twotothe [ k ] ) ; 
      } 
      incr ( k ) ; 
    } 
    if ( x <= 127919879L ) 
    Result = ( y + 8 ) / 16 ; 
    else Result = y ; 
  } 
  return(Result) ; 
} 
angle znarg ( x , y ) 
integer x ; 
integer y ; 
{register angle Result; angle z  ; 
  integer t  ; 
  smallnumber k  ; 
  schar octant  ; 
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
      helpline [ 1 ] = 313 ; 
      helpline [ 0 ] = 307 ; 
    } 
    error () ; 
    Result = 0 ; 
  } 
  else {
      
    while ( x >= 536870912L ) {
	
      x = ( x ) / 2 ; 
      y = ( y ) / 2 ; 
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
	  z = z + specatan [ k ] ; 
	  t = x ; 
	  x = x + ( y / twotothe [ k + k ] ) ; 
	  y = y - t ; 
	} 
      } while ( ! ( k == 15 ) ) ; 
      do {
	  y = y + y ; 
	incr ( k ) ; 
	if ( y > x ) 
	{
	  z = z + specatan [ k ] ; 
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
  return(Result) ; 
} 
