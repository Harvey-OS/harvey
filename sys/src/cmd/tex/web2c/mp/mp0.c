#define EXTERN extern
#include "mpd.h"

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
  {register integer for_end; k = 1 ;for_end = 33 ; if ( k <= for_end) do 
    internal [k ]= 0 ;
  while ( k++ < for_end ) ;} 
  intptr = 33 ;
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
  eqtb [1 ].lhfield = 43 ;
  eqtb [1 ].v.RH = 0 ;
  {register integer for_end; k = 2 ;for_end = 9771 ; if ( k <= for_end) do 
    {
      hash [k ]= hash [1 ];
      eqtb [k ]= eqtb [1 ];
    } 
  while ( k++ < for_end ) ;} 
  bignodesize [12 ]= 12 ;
  bignodesize [14 ]= 4 ;
  bignodesize [13 ]= 6 ;
  sector0 [12 ]= 5 ;
  sector0 [14 ]= 5 ;
  sector0 [13 ]= 11 ;
  {register integer for_end; k = 5 ;for_end = 10 ; if ( k <= for_end) do 
    sectoroffset [k ]= 2 * ( k - 5 ) ;
  while ( k++ < for_end ) ;} 
  {register integer for_end; k = 11 ;for_end = 13 ; if ( k <= for_end) do 
    sectoroffset [k ]= 2 * ( k - 11 ) ;
  while ( k++ < for_end ) ;} 
  saveptr = 0 ;
  halfcos [0 ]= 134217728L ;
  halfcos [1 ]= 94906266L ;
  halfcos [2 ]= 0 ;
  dcos [0 ]= 35596755L ;
  dcos [1 ]= 25170707L ;
  dcos [2 ]= 0 ;
  {register integer for_end; k = 3 ;for_end = 4 ; if ( k <= for_end) do 
    {
      halfcos [k ]= - (integer) halfcos [4 - k ];
      dcos [k ]= - (integer) dcos [4 - k ];
    } 
  while ( k++ < for_end ) ;} 
  {register integer for_end; k = 5 ;for_end = 7 ; if ( k <= for_end) do 
    {
      halfcos [k ]= halfcos [8 - k ];
      dcos [k ]= dcos [8 - k ];
    } 
  while ( k++ < for_end ) ;} 
  grobjectsize [1 ]= 6 ;
  grobjectsize [2 ]= 8 ;
  grobjectsize [3 ]= 14 ;
  grobjectsize [4 ]= 2 ;
  grobjectsize [6 ]= 2 ;
  grobjectsize [5 ]= 2 ;
  grobjectsize [7 ]= 2 ;
  specp1 = 0 ;
  specp2 = 0 ;
  fixneeded = false ;
  watchcoefs = true ;
  condptr = 0 ;
  iflimit = 0 ;
  curif = 0 ;
  ifline = 0 ;
  loopptr = 0 ;
  curname = 284 ;
  curarea = 284 ;
  curext = 284 ;
  readfiles = 0 ;
  writefiles = 0 ;
  curexp = 0 ;
  varflag = 0 ;
  eofline = 0 ;
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
  internal [31 ]= -65536L ;
  bchlabel = ligtablesize ;
  labelloc [0 ]= -1 ;
  labelptr = 0 ;
  firstfilename = 284 ;
  lastfilename = 284 ;
  firstoutputcode = 32768L ;
  lastoutputcode = -32768L ;
  totalshipped = 0 ;
  lastpending = memtop - 3 ;
  memident = 0 ;
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
  {case 10 : 
    {
      putc ('\n',  stdout );
      putc ('\n',  logfile );
      termoffset = 0 ;
      fileoffset = 0 ;
    } 
    break ;
  case 9 : 
    {
      putc ('\n',  logfile );
      fileoffset = 0 ;
    } 
    break ;
  case 8 : 
    {
      putc ('\n',  stdout );
      termoffset = 0 ;
    } 
    break ;
  case 5 : 
    {
      putc ('\n',  psfile );
      psoffset = 0 ;
    } 
    break ;
  case 7 : 
  case 6 : 
  case 4 : 
    ;
    break ;
    default: 
    putc ('\n',  wrfile [selector ]);
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
  /* 30 */ switch ( selector ) 
  {case 10 : 
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
  case 9 : 
    {
      putc ( xchr [s ],  logfile );
      incr ( fileoffset ) ;
      if ( fileoffset == maxprintline ) 
      println () ;
    } 
    break ;
  case 8 : 
    {
      putc ( xchr [s ],  stdout );
      incr ( termoffset ) ;
      if ( termoffset == maxprintline ) 
      println () ;
    } 
    break ;
  case 5 : 
    {
      putc ( xchr [s ],  psfile );
      incr ( psoffset ) ;
    } 
    break ;
  case 7 : 
    ;
    break ;
  case 6 : 
    if ( tally < trickcount ) 
    trickbuf [tally % errorline ]= s ;
    break ;
  case 4 : 
    {
      if ( poolptr >= maxpoolptr ) 
      {
	unitstrroom () ;
	if ( poolptr >= poolsize ) 
	goto lab30 ;
      } 
      {
	strpool [poolptr ]= s ;
	incr ( poolptr ) ;
      } 
    } 
    break ;
    default: 
    putc ( xchr [s ],  wrfile [selector ]);
    break ;
  } 
  lab30: incr ( tally ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintchar ( ASCIIcode k ) 
#else
zprintchar ( k ) 
  ASCIIcode k ;
#endif
{
  unsigned char l  ;
  if ( selector < 6 ) 
  printvisiblechar ( k ) ;
  else if ( ( k < 32 ) || ( k > 126 ) ) 
  {
    printvisiblechar ( 94 ) ;
    printvisiblechar ( 94 ) ;
    if ( k < 64 ) 
    printvisiblechar ( k + 64 ) ;
    else if ( k < 128 ) 
    printvisiblechar ( k - 64 ) ;
    else {
	
      l = k / 16 ;
      if ( l < 10 ) 
      printvisiblechar ( l + 48 ) ;
      else printvisiblechar ( l + 87 ) ;
      l = k % 16 ;
      if ( l < 10 ) 
      printvisiblechar ( l + 48 ) ;
      else printvisiblechar ( l + 87 ) ;
    } 
  } 
  else printvisiblechar ( k ) ;
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
  if ( ( s < 0 ) || ( s > maxstrptr ) ) 
  s = 260 ;
  j = strstart [s ];
  while ( j < strstart [nextstr [s ]]) {
      
    printchar ( strpool [j ]) ;
    incr ( j ) ;
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
  switch ( selector ) 
  {case 10 : 
    if ( ( termoffset > 0 ) || ( fileoffset > 0 ) ) 
    println () ;
    break ;
  case 9 : 
    if ( fileoffset > 0 ) 
    println () ;
    break ;
  case 8 : 
    if ( termoffset > 0 ) 
    println () ;
    break ;
  case 5 : 
    if ( psoffset > 0 ) 
    println () ;
    break ;
  case 7 : 
  case 6 : 
  case 4 : 
    ;
    break ;
  } 
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
    print ( 324 ) ;
    break ;
  case 2 : 
    print ( 325 ) ;
    break ;
  case 3 : 
    print ( 326 ) ;
    break ;
  case 4 : 
    print ( 259 ) ;
    break ;
  case 5 : 
    print ( 327 ) ;
    break ;
  case 6 : 
    print ( 328 ) ;
    break ;
  case 7 : 
    print ( 329 ) ;
    break ;
  case 8 : 
    print ( 330 ) ;
    break ;
  case 9 : 
    print ( 331 ) ;
    break ;
  case 10 : 
    print ( 332 ) ;
    break ;
  case 11 : 
    print ( 333 ) ;
    break ;
  case 12 : 
    print ( 334 ) ;
    break ;
  case 13 : 
    print ( 335 ) ;
    break ;
  case 14 : 
    print ( 336 ) ;
    break ;
  case 16 : 
    print ( 337 ) ;
    break ;
  case 17 : 
    print ( 338 ) ;
    break ;
  case 18 : 
    print ( 339 ) ;
    break ;
  case 15 : 
    print ( 340 ) ;
    break ;
  case 19 : 
    print ( 341 ) ;
    break ;
  case 20 : 
    print ( 342 ) ;
    break ;
  case 21 : 
    print ( 343 ) ;
    break ;
  case 22 : 
    print ( 344 ) ;
    break ;
  case 23 : 
    print ( 345 ) ;
    break ;
    default: 
    print ( 346 ) ;
    break ;
  } 
} 
integer 
#ifdef HAVE_PROTOTYPES
trueline ( void ) 
#else
trueline ( ) 
#endif
{
  register integer Result; integer k  ;
  if ( ( curinput .indexfield <= 15 ) && ( curinput .namefield > 2 ) ) 
  Result = linestack [curinput .indexfield ];
  else {
      
    k = inputptr ;
    while ( ( k > 0 ) && ( inputstack [k ].indexfield > 15 ) || ( inputstack 
    [k ].namefield <= 2 ) ) decr ( k ) ;
    Result = linestack [k ];
  } 
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
begindiagnostic ( void ) 
#else
begindiagnostic ( ) 
#endif
{
  oldsetting = selector ;
  if ( selector == 5 ) 
  selector = nonpssetting ;
  if ( ( internal [12 ]<= 0 ) && ( selector == 10 ) ) 
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
  printnl ( 284 ) ;
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
  print ( 463 ) ;
  printint ( trueline () ) ;
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
  
	;
#ifdef STAT
  poolinuse = poolinuse - ( strstart [nextstr [s ]]- strstart [s ]) ;
  decr ( strsinuse ) ;
#endif /* STAT */
  if ( nextstr [s ]!= strptr ) 
  strref [s ]= 0 ;
  else {
      
    strptr = s ;
    decr ( strsusedup ) ;
  } 
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
      print ( 264 ) ;
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
	  if ( curcmd == 41 ) 
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
	  helpline [1 ]= 277 ;
	  helpline [0 ]= 278 ;
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
	editnamelength = ( strstart [nextstr [inputstack [fileptr ]
	.namefield ]]- strstart [inputstack [fileptr ].namefield ]) ;
	editline = trueline () ;
	jumpout () ;
      } 
      break ;
    case 72 : 
      {
	if ( useerrhelp ) 
	{
	  j = strstart [errhelp ];
	  while ( j < strstart [nextstr [errhelp ]]) {
	      
	    if ( strpool [j ]!= 37 ) 
	    print ( strpool [j ]) ;
	    else if ( j + 1 == strstart [nextstr [errhelp ]]) 
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
	    helpline [1 ]= 279 ;
	    helpline [0 ]= 280 ;
	  } 
	  do {
	      decr ( helpptr ) ;
	    print ( helpline [helpptr ]) ;
	    println () ;
	  } while ( ! ( helpptr == 0 ) ) ;
	} 
	{
	  helpptr = 4 ;
	  helpline [3 ]= 281 ;
	  helpline [2 ]= 280 ;
	  helpline [1 ]= 282 ;
	  helpline [0 ]= 283 ;
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
	    print ( 276 ) ;
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
	print ( 271 ) ;
	switch ( c ) 
	{case 81 : 
	  {
	    print ( 272 ) ;
	    decr ( selector ) ;
	  } 
	  break ;
	case 82 : 
	  print ( 273 ) ;
	  break ;
	case 83 : 
	  print ( 274 ) ;
	  break ;
	} 
	print ( 275 ) ;
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
      print ( 265 ) ;
      printnl ( 266 ) ;
      printnl ( 267 ) ;
      if ( fileptr > 0 ) 
      print ( 268 ) ;
      if ( deletionsallowed ) 
      printnl ( 269 ) ;
      printnl ( 270 ) ;
    } 
  } 
  incr ( errorcount ) ;
  if ( errorcount == 100 ) 
  {
    printnl ( 263 ) ;
    history = 3 ;
    jumpout () ;
  } 
  if ( interaction > 0 ) 
  decr ( selector ) ;
  if ( useerrhelp ) 
  {
    printnl ( 284 ) ;
    j = strstart [errhelp ];
    while ( j < strstart [nextstr [errhelp ]]) {
	
      if ( strpool [j ]!= 37 ) 
      print ( strpool [j ]) ;
      else if ( j + 1 == strstart [nextstr [errhelp ]]) 
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
    printnl ( 262 ) ;
    print ( 285 ) ;
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
    printnl ( 262 ) ;
    print ( 286 ) ;
  } 
  print ( s ) ;
  printchar ( 61 ) ;
  printint ( n ) ;
  printchar ( 93 ) ;
  {
    helpptr = 2 ;
    helpline [1 ]= 287 ;
    helpline [0 ]= 288 ;
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
      printnl ( 262 ) ;
      print ( 289 ) ;
    } 
    print ( s ) ;
    printchar ( 41 ) ;
    {
      helpptr = 1 ;
      helpline [0 ]= 290 ;
    } 
  } 
  else {
      
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 291 ) ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 292 ;
      helpline [0 ]= 293 ;
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
void 
#ifdef HAVE_PROTOTYPES
zdocompaction ( poolpointer needed ) 
#else
zdocompaction ( needed ) 
  poolpointer needed ;
#endif
{
  /* 30 */ strnumber struse  ;
  strnumber r, s, t  ;
  poolpointer p, q  ;
  t = nextstr [lastfixedstr ];
  while ( ( strref [t ]== 127 ) && ( t != strptr ) ) {
      
    incr ( fixedstruse ) ;
    lastfixedstr = t ;
    t = nextstr [t ];
  } 
  struse = fixedstruse ;
  r = lastfixedstr ;
  s = nextstr [r ];
  p = strstart [s ];
  while ( s != strptr ) {
      
    while ( strref [s ]== 0 ) {
	
      t = s ;
      s = nextstr [s ];
      nextstr [r ]= s ;
      nextstr [t ]= nextstr [strptr ];
      nextstr [strptr ]= t ;
      if ( s == strptr ) 
      goto lab30 ;
    } 
    r = s ;
    s = nextstr [s ];
    incr ( struse ) ;
    q = strstart [r ];
    strstart [r ]= p ;
    while ( q < strstart [s ]) {
	
      strpool [p ]= strpool [q ];
      incr ( p ) ;
      incr ( q ) ;
    } 
  } 
  lab30: q = strstart [strptr ];
  strstart [strptr ]= p ;
  while ( q < poolptr ) {
      
    strpool [p ]= strpool [q ];
    incr ( p ) ;
    incr ( q ) ;
  } 
  poolptr = p ;
  if ( needed < poolsize ) 
  {
    if ( struse >= maxstrings - 1 ) 
    {
      stroverflowed = true ;
      overflow ( 257 , maxstrings - 1 - initstruse ) ;
    } 
    if ( poolptr + needed > maxpoolptr ) 
    if ( poolptr + needed > poolsize ) 
    {
      stroverflowed = true ;
      overflow ( 258 , poolsize - initpoolptr ) ;
    } 
    else maxpoolptr = poolptr + needed ;
  } 
	;
#ifdef STAT
  if ( ( strstart [strptr ]!= poolinuse ) || ( struse != strsinuse ) ) 
  confusion ( 259 ) ;
  incr ( pactcount ) ;
  pactchars = pactchars + poolptr - strstart [nextstr [lastfixedstr ]];
  pactstrs = pactstrs + struse - fixedstruse ;
	;
#ifdef TEXMF_DEBUG
  s = strptr ;
  t = struse ;
  while ( s <= maxstrptr ) {
      
    if ( t > maxstrptr ) 
    confusion ( 34 ) ;
    incr ( t ) ;
    s = nextstr [s ];
  } 
  if ( t <= maxstrptr ) 
  confusion ( 34 ) ;
#endif /* TEXMF_DEBUG */
#endif /* STAT */
  strsusedup = struse ;
} 
void 
#ifdef HAVE_PROTOTYPES
unitstrroom ( void ) 
#else
unitstrroom ( ) 
#endif
{
  if ( poolptr >= poolsize ) 
  docompaction ( poolsize ) ;
  if ( poolptr >= maxpoolptr ) 
  maxpoolptr = poolptr + 1 ;
} 
strnumber 
#ifdef HAVE_PROTOTYPES
makestring ( void ) 
#else
makestring ( ) 
#endif
{
  /* 20 */ register strnumber Result; strnumber s  ;
  lab20: s = strptr ;
  strptr = nextstr [s ];
  if ( strptr > maxstrptr ) 
  if ( strptr == maxstrings ) 
  {
    strptr = s ;
    docompaction ( 0 ) ;
    goto lab20 ;
  } 
  else {
      
	;
#ifdef TEXMF_DEBUG
    if ( strsusedup != maxstrptr ) 
    confusion ( 115 ) ;
#endif /* TEXMF_DEBUG */
    maxstrptr = strptr ;
    nextstr [strptr ]= maxstrptr + 1 ;
  } 
  strref [s ]= 1 ;
  strstart [strptr ]= poolptr ;
  incr ( strsusedup ) ;
	;
#ifdef STAT
  incr ( strsinuse ) ;
  poolinuse = poolinuse + ( strstart [nextstr [s ]]- strstart [s ]) ;
  if ( poolinuse > maxplused ) 
  maxplused = poolinuse ;
  if ( strsinuse > maxstrsused ) 
  maxstrsused = strsinuse ;
#endif /* STAT */
  Result = s ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zchoplaststring ( poolpointer p ) 
#else
zchoplaststring ( p ) 
  poolpointer p ;
#endif
{
  
	;
#ifdef STAT
  poolinuse = poolinuse - ( strstart [strptr ]- p ) ;
#endif /* STAT */
  strstart [strptr ]= p ;
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
  while ( j < strstart [nextstr [s ]]) {
      
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
  ls = ( strstart [nextstr [s ]]- strstart [s ]) ;
  lt = ( strstart [nextstr [t ]]- strstart [t ]) ;
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
  fatalerror ( 261 ) ;
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
  selector = 10 ;
  else selector = 8 ;
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
    if ( ( selector == 9 ) || ( selector == 7 ) ) 
    incr ( selector ) ;
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 294 ) ;
    } 
    {
      helpptr = 3 ;
      helpline [2 ]= 295 ;
      helpline [1 ]= 296 ;
      helpline [0 ]= 297 ;
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
    printnl ( 262 ) ;
    print ( 298 ) ;
  } 
  print ( s ) ;
  print ( 299 ) ;
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
    printnl ( 262 ) ;
    print ( 300 ) ;
  } 
  {
    helpptr = 4 ;
    helpline [3 ]= 301 ;
    helpline [2 ]= 302 ;
    helpline [1 ]= 303 ;
    helpline [0 ]= 304 ;
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
	printnl ( 262 ) ;
	print ( 305 ) ;
      } 
      printscaled ( x ) ;
      print ( 306 ) ;
      {
	helpptr = 2 ;
	helpline [1 ]= 307 ;
	helpline [0 ]= 308 ;
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
	printnl ( 262 ) ;
	print ( 309 ) ;
      } 
      printscaled ( a ) ;
      print ( 310 ) ;
      printscaled ( b ) ;
      print ( 306 ) ;
      {
	helpptr = 2 ;
	helpline [1 ]= 307 ;
	helpline [0 ]= 308 ;
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
      printnl ( 262 ) ;
      print ( 311 ) ;
    } 
    printscaled ( x ) ;
    print ( 306 ) ;
    {
      helpptr = 2 ;
      helpline [1 ]= 312 ;
      helpline [0 ]= 308 ;
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
      printnl ( 262 ) ;
      print ( 313 ) ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 314 ;
      helpline [0 ]= 308 ;
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
      print ( 505 ) ;
      goto lab10 ;
    } 
    if ( p < himemmin ) 
    if ( mem [p ].hhfield .b1 == 15 ) 
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
    print ( 508 ) ;
    else {
	
      printchar ( 34 ) ;
      print ( mem [p + 1 ].cint ) ;
      printchar ( 34 ) ;
      c = 4 ;
    } 
    else if ( ( mem [p ].hhfield .b1 != 14 ) || ( mem [p ].hhfield .b0 < 1 
    ) || ( mem [p ].hhfield .b0 > 19 ) ) 
    print ( 508 ) ;
    else {
	
      gpointer = p ;
      printcapsule () ;
      c = 8 ;
    } 
    else {
	
      r = mem [p ].hhfield .lhfield ;
      if ( r >= 9772 ) 
      {
	if ( r < 9922 ) 
	{
	  print ( 510 ) ;
	  r = r - ( 9772 ) ;
	} 
	else if ( r < 10072 ) 
	{
	  print ( 511 ) ;
	  r = r - ( 9922 ) ;
	} 
	else {
	    
	  print ( 512 ) ;
	  r = r - ( 10072 ) ;
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
	print ( 509 ) ;
	c = 18 ;
      } 
      else print ( 506 ) ;
      else {
	  
	r = hash [r ].v.RH ;
	if ( ( r < 0 ) || ( r > maxstrptr ) ) 
	print ( 507 ) ;
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
  print ( 504 ) ;
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
    printnl ( 658 ) ;
    switch ( scannerstatus ) 
    {case 3 : 
      print ( 659 ) ;
      break ;
    case 4 : 
    case 5 : 
      print ( 660 ) ;
      break ;
    case 6 : 
      print ( 661 ) ;
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
      overflow ( 315 , memmax + 1 ) ;
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
  overflow ( 315 , memmax + 1 ) ;
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
  /* 31 32 33 */ halfword p, q, r  ;
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
      printnl ( 316 ) ;
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
      printnl ( 317 ) ;
      printint ( q ) ;
      goto lab32 ;
    } 
    {register integer for_end; q = p ;for_end = p + mem [p ].hhfield 
    .lhfield - 1 ; if ( q <= for_end) do 
      {
	if ( freearr [q ]) 
	{
	  printnl ( 318 ) ;
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
      printnl ( 319 ) ;
      printint ( p ) ;
    } 
    while ( ( p <= lomemmax ) && ! freearr [p ]) incr ( p ) ;
    while ( ( p <= lomemmax ) && freearr [p ]) incr ( p ) ;
  } 
  q = 5 ;
  p = mem [q ].hhfield .v.RH ;
  while ( p != 5 ) {
      
    if ( mem [p + 1 ].hhfield .lhfield != q ) 
    {
      printnl ( 609 ) ;
      printint ( p ) ;
    } 
    p = mem [p + 1 ].hhfield .v.RH ;
    while ( true ) {
	
      r = mem [p ].hhfield .lhfield ;
      q = p ;
      p = mem [q ].hhfield .v.RH ;
      if ( r == 0 ) 
      goto lab33 ;
      if ( mem [mem [p ].hhfield .lhfield + 1 ].cint >= mem [r + 1 ]
      .cint ) 
      {
	printnl ( 610 ) ;
	printint ( p ) ;
      } 
    } 
    lab33: ;
  } 
  if ( printlocs ) 
  {
    q = memmax ;
    r = memmax ;
    printnl ( 320 ) ;
    {register integer for_end; p = 0 ;for_end = lomemmax ; if ( p <= 
    for_end) do 
      if ( ! freearr [p ]&& ( ( p > waslomax ) || wasfree [p ]) ) 
      {
	if ( p > q + 1 ) 
	{
	  if ( q > r ) 
	  {
	    print ( 321 ) ;
	    printint ( q ) ;
	  } 
	  printchar ( 32 ) ;
	  printint ( p ) ;
	  r = p ;
	} 
	q = p ;
      } 
    while ( p++ < for_end ) ;} 
    {register integer for_end; p = himemmin ;for_end = memend ; if ( p <= 
    for_end) do 
      if ( ! freearr [p ]&& ( ( p < washimin ) || ( p > wasmemend ) || 
      wasfree [p ]) ) 
      {
	if ( p > q + 1 ) 
	{
	  if ( q > r ) 
	  {
	    print ( 321 ) ;
	    printint ( q ) ;
	  } 
	  printchar ( 32 ) ;
	  printint ( p ) ;
	  r = p ;
	} 
	q = p ;
      } 
    while ( p++ < for_end ) ;} 
    if ( q > r ) 
    {
      print ( 321 ) ;
      printint ( q ) ;
    } 
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
	printnl ( 322 ) ;
	printint ( q ) ;
	printchar ( 41 ) ;
      } 
      if ( mem [q ].hhfield .lhfield == p ) 
      {
	printnl ( 323 ) ;
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
	printnl ( 322 ) ;
	printint ( q ) ;
	printchar ( 41 ) ;
      } 
      if ( mem [q ].hhfield .lhfield == p ) 
      {
	printnl ( 323 ) ;
	printint ( q ) ;
	printchar ( 41 ) ;
      } 
    } 
  while ( q++ < for_end ) ;} 
  {register integer for_end; q = 1 ;for_end = 9771 ; if ( q <= for_end) do 
    {
      if ( eqtb [q ].v.RH == p ) 
      {
	printnl ( 473 ) ;
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
    print ( 347 ) ;
    break ;
  case 31 : 
    print ( 348 ) ;
    break ;
  case 32 : 
    print ( 349 ) ;
    break ;
  case 33 : 
    print ( 350 ) ;
    break ;
  case 34 : 
    print ( 351 ) ;
    break ;
  case 35 : 
    print ( 352 ) ;
    break ;
  case 36 : 
    print ( 353 ) ;
    break ;
  case 37 : 
    print ( 354 ) ;
    break ;
  case 38 : 
    print ( 355 ) ;
    break ;
  case 39 : 
    print ( 356 ) ;
    break ;
  case 40 : 
    print ( 357 ) ;
    break ;
  case 41 : 
    print ( 358 ) ;
    break ;
  case 42 : 
    print ( 359 ) ;
    break ;
  case 43 : 
    print ( 360 ) ;
    break ;
  case 44 : 
    print ( 361 ) ;
    break ;
  case 45 : 
    print ( 362 ) ;
    break ;
  case 46 : 
    print ( 363 ) ;
    break ;
  case 47 : 
    print ( 364 ) ;
    break ;
  case 48 : 
    print ( 365 ) ;
    break ;
  case 49 : 
    print ( 366 ) ;
    break ;
  case 50 : 
    print ( 367 ) ;
    break ;
  case 51 : 
    print ( 368 ) ;
    break ;
  case 52 : 
    print ( 369 ) ;
    break ;
  case 53 : 
    print ( 370 ) ;
    break ;
  case 54 : 
    print ( 371 ) ;
    break ;
  case 55 : 
    print ( 372 ) ;
    break ;
  case 56 : 
    print ( 373 ) ;
    break ;
  case 57 : 
    print ( 374 ) ;
    break ;
  case 58 : 
    print ( 375 ) ;
    break ;
  case 59 : 
    print ( 376 ) ;
    break ;
  case 60 : 
    print ( 377 ) ;
    break ;
  case 61 : 
    print ( 378 ) ;
    break ;
  case 62 : 
    print ( 379 ) ;
    break ;
  case 63 : 
    print ( 380 ) ;
    break ;
  case 64 : 
    print ( 381 ) ;
    break ;
  case 65 : 
    print ( 382 ) ;
    break ;
  case 66 : 
    print ( 383 ) ;
    break ;
  case 67 : 
    print ( 384 ) ;
    break ;
  case 68 : 
    print ( 385 ) ;
    break ;
  case 69 : 
    print ( 386 ) ;
    break ;
  case 70 : 
    print ( 387 ) ;
    break ;
  case 71 : 
    print ( 388 ) ;
    break ;
  case 72 : 
    print ( 389 ) ;
    break ;
  case 73 : 
    print ( 390 ) ;
    break ;
  case 74 : 
    print ( 391 ) ;
    break ;
  case 75 : 
    print ( 392 ) ;
    break ;
  case 76 : 
    print ( 393 ) ;
    break ;
  case 77 : 
    print ( 394 ) ;
    break ;
  case 78 : 
    print ( 395 ) ;
    break ;
  case 79 : 
    print ( 396 ) ;
    break ;
  case 80 : 
    print ( 397 ) ;
    break ;
  case 81 : 
    print ( 398 ) ;
    break ;
  case 82 : 
    print ( 399 ) ;
    break ;
  case 83 : 
    print ( 400 ) ;
    break ;
  case 84 : 
    print ( 401 ) ;
    break ;
  case 85 : 
    print ( 402 ) ;
    break ;
  case 86 : 
    print ( 403 ) ;
    break ;
  case 87 : 
    print ( 404 ) ;
    break ;
  case 88 : 
    print ( 405 ) ;
    break ;
  case 89 : 
    printchar ( 43 ) ;
    break ;
  case 90 : 
    printchar ( 45 ) ;
    break ;
  case 91 : 
    printchar ( 42 ) ;
    break ;
  case 92 : 
    printchar ( 47 ) ;
    break ;
  case 93 : 
    print ( 406 ) ;
    break ;
  case 94 : 
    print ( 310 ) ;
    break ;
  case 95 : 
    print ( 407 ) ;
    break ;
  case 96 : 
    print ( 408 ) ;
    break ;
  case 97 : 
    printchar ( 60 ) ;
    break ;
  case 98 : 
    print ( 409 ) ;
    break ;
  case 99 : 
    printchar ( 62 ) ;
    break ;
  case 100 : 
    print ( 410 ) ;
    break ;
  case 101 : 
    printchar ( 61 ) ;
    break ;
  case 102 : 
    print ( 411 ) ;
    break ;
  case 103 : 
    print ( 38 ) ;
    break ;
  case 104 : 
    print ( 412 ) ;
    break ;
  case 105 : 
    print ( 413 ) ;
    break ;
  case 106 : 
    print ( 414 ) ;
    break ;
  case 107 : 
    print ( 415 ) ;
    break ;
  case 108 : 
    print ( 416 ) ;
    break ;
  case 109 : 
    print ( 417 ) ;
    break ;
  case 110 : 
    print ( 418 ) ;
    break ;
  case 111 : 
    print ( 419 ) ;
    break ;
  case 112 : 
    print ( 420 ) ;
    break ;
  case 113 : 
    print ( 421 ) ;
    break ;
  case 115 : 
    print ( 422 ) ;
    break ;
  case 116 : 
    print ( 423 ) ;
    break ;
  case 117 : 
    print ( 424 ) ;
    break ;
  case 118 : 
    print ( 425 ) ;
    break ;
  case 119 : 
    print ( 426 ) ;
    break ;
  case 120 : 
    print ( 427 ) ;
    break ;
  case 121 : 
    print ( 428 ) ;
    break ;
  case 122 : 
    print ( 429 ) ;
    break ;
    default: 
    print ( 321 ) ;
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
  dateandtime ( internal [16 ], internal [15 ], internal [14 ], 
  internal [13 ]) ;
  internal [16 ]= internal [16 ]* 65536L ;
  internal [15 ]= internal [15 ]* 65536L ;
  internal [14 ]= internal [14 ]* 65536L ;
  internal [13 ]= internal [13 ]* 65536L ;
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
    if ( ( strstart [nextstr [hash [p ].v.RH ]]- strstart [hash [p ]
    .v.RH ]) == l ) 
    if ( streqbuf ( hash [p ].v.RH , j ) ) 
    goto lab40 ;
    if ( hash [p ].lhfield == 0 ) 
    {
      if ( hash [p ].v.RH > 0 ) 
      {
	do {
	    if ( ( hashused == 257 ) ) 
	  overflow ( 472 , 9500 ) ;
	  decr ( hashused ) ;
	} while ( ! ( hash [hashused ].v.RH == 0 ) ) ;
	hash [p ].lhfield = hashused ;
	p = hashused ;
      } 
      {
	if ( poolptr + l > maxpoolptr ) 
	if ( poolptr + l > poolsize ) 
	docompaction ( l ) ;
	else maxpoolptr = poolptr + l ;
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
  mem [p ].hhfield .b1 = 15 ;
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
      case 11 : 
      case 9 : 
      case 6 : 
      case 8 : 
      case 10 : 
      case 14 : 
      case 13 : 
      case 12 : 
      case 17 : 
      case 18 : 
      case 19 : 
	{
	  gpointer = q ;
	  tokenrecycle () ;
	} 
	break ;
	default: 
	confusion ( 503 ) ;
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
  {case 20 : 
    print ( 477 ) ;
    break ;
  case 76 : 
    print ( 476 ) ;
    break ;
  case 61 : 
    print ( 478 ) ;
    break ;
  case 78 : 
    print ( 475 ) ;
    break ;
  case 34 : 
    print ( 479 ) ;
    break ;
  case 80 : 
    print ( 58 ) ;
    break ;
  case 81 : 
    print ( 44 ) ;
    break ;
  case 59 : 
    print ( 480 ) ;
    break ;
  case 62 : 
    print ( 481 ) ;
    break ;
  case 29 : 
    print ( 482 ) ;
    break ;
  case 79 : 
    print ( 474 ) ;
    break ;
  case 83 : 
    print ( 468 ) ;
    break ;
  case 28 : 
    print ( 483 ) ;
    break ;
  case 9 : 
    print ( 484 ) ;
    break ;
  case 12 : 
    print ( 485 ) ;
    break ;
  case 15 : 
    print ( 486 ) ;
    break ;
  case 48 : 
    print ( 123 ) ;
    break ;
  case 65 : 
    print ( 91 ) ;
    break ;
  case 16 : 
    print ( 487 ) ;
    break ;
  case 17 : 
    print ( 488 ) ;
    break ;
  case 70 : 
    print ( 489 ) ;
    break ;
  case 49 : 
    print ( 321 ) ;
    break ;
  case 26 : 
    print ( 490 ) ;
    break ;
  case 10 : 
    printchar ( 92 ) ;
    break ;
  case 67 : 
    print ( 125 ) ;
    break ;
  case 66 : 
    print ( 93 ) ;
    break ;
  case 14 : 
    print ( 491 ) ;
    break ;
  case 11 : 
    print ( 492 ) ;
    break ;
  case 82 : 
    print ( 59 ) ;
    break ;
  case 19 : 
    print ( 493 ) ;
    break ;
  case 77 : 
    print ( 494 ) ;
    break ;
  case 30 : 
    print ( 495 ) ;
    break ;
  case 72 : 
    print ( 496 ) ;
    break ;
  case 37 : 
    print ( 497 ) ;
    break ;
  case 60 : 
    print ( 498 ) ;
    break ;
  case 71 : 
    print ( 499 ) ;
    break ;
  case 73 : 
    print ( 500 ) ;
    break ;
  case 74 : 
    print ( 501 ) ;
    break ;
  case 31 : 
    print ( 502 ) ;
    break ;
  case 1 : 
    if ( m == 0 ) 
    print ( 681 ) ;
    else print ( 682 ) ;
    break ;
  case 2 : 
    print ( 465 ) ;
    break ;
  case 3 : 
    print ( 466 ) ;
    break ;
  case 18 : 
    if ( m <= 2 ) 
    if ( m == 1 ) 
    print ( 695 ) ;
    else if ( m < 1 ) 
    print ( 469 ) ;
    else print ( 696 ) ;
    else if ( m == 55 ) 
    print ( 697 ) ;
    else if ( m == 46 ) 
    print ( 698 ) ;
    else print ( 699 ) ;
    break ;
  case 7 : 
    if ( m <= 1 ) 
    if ( m == 1 ) 
    print ( 702 ) ;
    else print ( 470 ) ;
    else if ( m == 9772 ) 
    print ( 700 ) ;
    else print ( 701 ) ;
    break ;
  case 63 : 
    switch ( m ) 
    {case 1 : 
      print ( 704 ) ;
      break ;
    case 2 : 
      printchar ( 64 ) ;
      break ;
    case 3 : 
      print ( 705 ) ;
      break ;
      default: 
      print ( 703 ) ;
      break ;
    } 
    break ;
  case 58 : 
    if ( m >= 9772 ) 
    if ( m == 9772 ) 
    print ( 716 ) ;
    else if ( m == 9922 ) 
    print ( 717 ) ;
    else print ( 718 ) ;
    else if ( m < 2 ) 
    print ( 719 ) ;
    else if ( m == 2 ) 
    print ( 720 ) ;
    else print ( 721 ) ;
    break ;
  case 6 : 
    if ( m == 0 ) 
    print ( 731 ) ;
    else print ( 628 ) ;
    break ;
  case 4 : 
  case 5 : 
    switch ( m ) 
    {case 1 : 
      print ( 758 ) ;
      break ;
    case 2 : 
      print ( 467 ) ;
      break ;
    case 3 : 
      print ( 759 ) ;
      break ;
      default: 
      print ( 760 ) ;
      break ;
    } 
    break ;
  case 35 : 
  case 36 : 
  case 39 : 
  case 57 : 
  case 47 : 
  case 52 : 
  case 38 : 
  case 45 : 
  case 56 : 
  case 50 : 
  case 53 : 
  case 54 : 
    printop ( m ) ;
    break ;
  case 32 : 
    printtype ( m ) ;
    break ;
  case 84 : 
    if ( m == 0 ) 
    print ( 960 ) ;
    else print ( 961 ) ;
    break ;
  case 25 : 
    switch ( m ) 
    {case 0 : 
      print ( 272 ) ;
      break ;
    case 1 : 
      print ( 273 ) ;
      break ;
    case 2 : 
      print ( 274 ) ;
      break ;
      default: 
      print ( 967 ) ;
      break ;
    } 
    break ;
  case 23 : 
    if ( m == 0 ) 
    print ( 968 ) ;
    else print ( 969 ) ;
    break ;
  case 24 : 
    switch ( m ) 
    {case 0 : 
      print ( 983 ) ;
      break ;
    case 1 : 
      print ( 984 ) ;
      break ;
    case 2 : 
      print ( 985 ) ;
      break ;
    case 3 : 
      print ( 986 ) ;
      break ;
      default: 
      print ( 987 ) ;
      break ;
    } 
    break ;
  case 33 : 
  case 64 : 
    {
      if ( c == 33 ) 
      print ( 990 ) ;
      else print ( 991 ) ;
      print ( 992 ) ;
      print ( hash [m ].v.RH ) ;
    } 
    break ;
  case 43 : 
    if ( m == 0 ) 
    print ( 993 ) ;
    else print ( 994 ) ;
    break ;
  case 13 : 
    print ( 995 ) ;
    break ;
  case 55 : 
  case 46 : 
  case 51 : 
    {
      printcmdmod ( 18 , c ) ;
      print ( 996 ) ;
      println () ;
      showtokenlist ( mem [mem [m ].hhfield .v.RH ].hhfield .v.RH , 0 , 
      1000 , 0 ) ;
    } 
    break ;
  case 8 : 
    print ( 997 ) ;
    break ;
  case 42 : 
    print ( intname [m ]) ;
    break ;
  case 69 : 
    if ( m == 1 ) 
    print ( 1007 ) ;
    else if ( m == 0 ) 
    print ( 1006 ) ;
    else print ( 1008 ) ;
    break ;
  case 68 : 
    if ( m == 6 ) 
    print ( 1009 ) ;
    else if ( m == 13 ) 
    print ( 1011 ) ;
    else print ( 1010 ) ;
    break ;
  case 21 : 
    if ( m == 4 ) 
    print ( 1020 ) ;
    else print ( 1021 ) ;
    break ;
  case 27 : 
    if ( m < 1 ) 
    print ( 1034 ) ;
    else if ( m == 1 ) 
    print ( 1035 ) ;
    else print ( 1036 ) ;
    break ;
  case 22 : 
    switch ( m ) 
    {case 0 : 
      print ( 1051 ) ;
      break ;
    case 1 : 
      print ( 1052 ) ;
      break ;
    case 2 : 
      print ( 1053 ) ;
      break ;
    case 3 : 
      print ( 1054 ) ;
      break ;
      default: 
      print ( 1055 ) ;
      break ;
    } 
    break ;
  case 75 : 
    switch ( m ) 
    {case 0 : 
      print ( 1073 ) ;
      break ;
    case 1 : 
      print ( 1074 ) ;
      break ;
    case 2 : 
      print ( 1076 ) ;
      break ;
    case 3 : 
      print ( 1078 ) ;
      break ;
    case 5 : 
      print ( 1075 ) ;
      break ;
    case 6 : 
      print ( 1077 ) ;
      break ;
    case 7 : 
      print ( 1079 ) ;
      break ;
    case 11 : 
      print ( 1080 ) ;
      break ;
      default: 
      print ( 1081 ) ;
      break ;
    } 
    break ;
    default: 
    print ( 614 ) ;
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
    print ( 513 ) ;
    break ;
  case 1 : 
  case 2 : 
  case 3 : 
    {
      printchar ( 60 ) ;
      printcmdmod ( 58 , mem [p ].hhfield .lhfield ) ;
      print ( 514 ) ;
    } 
    break ;
  case 4 : 
    print ( 515 ) ;
    break ;
  case 5 : 
    print ( 516 ) ;
    break ;
  case 6 : 
    print ( 517 ) ;
    break ;
  case 7 : 
    print ( 518 ) ;
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
    mem [q + s ].hhfield .b1 = halfp ( s ) + sector0 [mem [p ].hhfield 
    .b0 ];
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
  mem [p ].hhfield .b0 = 12 ;
  mem [p ].hhfield .b1 = 14 ;
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
      print ( 521 ) ;
      break ;
    case 8 : 
      print ( 522 ) ;
      break ;
    case 9 : 
      print ( 523 ) ;
      break ;
    case 10 : 
      print ( 524 ) ;
      break ;
    case 11 : 
      print ( 525 ) ;
      break ;
    case 12 : 
      print ( 526 ) ;
      break ;
    case 13 : 
      print ( 527 ) ;
      break ;
    case 14 : 
      {
	print ( 528 ) ;
	printint ( p - 0 ) ;
	goto lab10 ;
      } 
      break ;
    } 
    print ( 529 ) ;
    p = mem [p - sectoroffset [mem [p ].hhfield .b1 ]].hhfield .v.RH ;
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
      confusion ( 520 ) ;
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
  print ( 519 ) ;
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
    if ( t != 14 ) 
    t = mem [mem [p - sectoroffset [t ]].hhfield .v.RH ].hhfield .b1 ;
    Result = ( t != 14 ) ;
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
    confusion ( 530 ) ;
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
  mem [q ].hhfield .v.RH = 9 ;
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
  if ( eqtb [p ].lhfield % 85 != 43 ) 
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
zprpath ( halfword h ) 
#else
zprpath ( h ) 
  halfword h ;
#endif
{
  /* 30 31 */ halfword p, q  ;
  p = h ;
  do {
      q = mem [p ].hhfield .v.RH ;
    if ( ( p == 0 ) || ( q == 0 ) ) 
    {
      printnl ( 260 ) ;
      goto lab30 ;
    } 
    printtwo ( mem [p + 1 ].cint , mem [p + 2 ].cint ) ;
    switch ( mem [p ].hhfield .b1 ) 
    {case 0 : 
      {
	if ( mem [p ].hhfield .b0 == 4 ) 
	print ( 532 ) ;
	if ( ( mem [q ].hhfield .b0 != 0 ) || ( q != h ) ) 
	q = 0 ;
	goto lab31 ;
      } 
      break ;
    case 1 : 
      {
	print ( 538 ) ;
	printtwo ( mem [p + 5 ].cint , mem [p + 6 ].cint ) ;
	print ( 537 ) ;
	if ( mem [q ].hhfield .b0 != 1 ) 
	print ( 539 ) ;
	else printtwo ( mem [q + 3 ].cint , mem [q + 4 ].cint ) ;
	goto lab31 ;
      } 
      break ;
    case 4 : 
      if ( ( mem [p ].hhfield .b0 != 1 ) && ( mem [p ].hhfield .b0 != 4 ) 
      ) 
      print ( 532 ) ;
      break ;
    case 3 : 
    case 2 : 
      {
	if ( mem [p ].hhfield .b0 == 4 ) 
	print ( 539 ) ;
	if ( mem [p ].hhfield .b1 == 3 ) 
	{
	  print ( 535 ) ;
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
      print ( 260 ) ;
      break ;
    } 
    if ( mem [q ].hhfield .b0 <= 1 ) 
    print ( 533 ) ;
    else if ( ( mem [p + 6 ].cint != 65536L ) || ( mem [q + 4 ].cint != 
    65536L ) ) 
    {
      print ( 536 ) ;
      if ( mem [p + 6 ].cint < 0 ) 
      print ( 478 ) ;
      printscaled ( abs ( mem [p + 6 ].cint ) ) ;
      if ( mem [p + 6 ].cint != mem [q + 4 ].cint ) 
      {
	print ( 537 ) ;
	if ( mem [q + 4 ].cint < 0 ) 
	print ( 478 ) ;
	printscaled ( abs ( mem [q + 4 ].cint ) ) ;
      } 
    } 
    lab31: ;
    p = q ;
    if ( ( p != h ) || ( mem [h ].hhfield .b0 != 0 ) ) 
    {
      printnl ( 534 ) ;
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
	print ( 535 ) ;
	printscaled ( mem [p + 3 ].cint ) ;
	printchar ( 125 ) ;
      } 
    } 
  } while ( ! ( p == h ) ) ;
  if ( mem [h ].hhfield .b0 != 0 ) 
  print ( 400 ) ;
  lab30: ;
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
  printdiagnostic ( 540 , s , nuline ) ;
  println () ;
  prpath ( h ) ;
  enddiagnostic ( true ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprpen ( halfword h ) 
#else
zprpen ( h ) 
  halfword h ;
#endif
{
  /* 30 */ halfword p, q  ;
  if ( ( h == mem [h ].hhfield .v.RH ) ) 
  {
    print ( 549 ) ;
    printscaled ( mem [h + 1 ].cint ) ;
    printchar ( 44 ) ;
    printscaled ( mem [h + 2 ].cint ) ;
    printchar ( 44 ) ;
    printscaled ( mem [h + 3 ].cint - mem [h + 1 ].cint ) ;
    printchar ( 44 ) ;
    printscaled ( mem [h + 5 ].cint - mem [h + 1 ].cint ) ;
    printchar ( 44 ) ;
    printscaled ( mem [h + 4 ].cint - mem [h + 2 ].cint ) ;
    printchar ( 44 ) ;
    printscaled ( mem [h + 6 ].cint - mem [h + 2 ].cint ) ;
    printchar ( 41 ) ;
  } 
  else {
      
    p = h ;
    do {
	printtwo ( mem [p + 1 ].cint , mem [p + 2 ].cint ) ;
      printnl ( 548 ) ;
      q = mem [p ].hhfield .v.RH ;
      if ( ( q == 0 ) || ( mem [q ].hhfield .lhfield != p ) ) 
      {
	printnl ( 260 ) ;
	goto lab30 ;
      } 
      p = q ;
    } while ( ! ( p == h ) ) ;
    print ( 400 ) ;
  } 
  lab30: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintpen ( halfword h , strnumber s , boolean nuline ) 
#else
zprintpen ( h , s , nuline ) 
  halfword h ;
  strnumber s ;
  boolean nuline ;
#endif
{
  printdiagnostic ( 550 , s , nuline ) ;
  println () ;
  prpen ( h ) ;
  enddiagnostic ( true ) ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zsqrtdet ( scaled a , scaled b , scaled c , scaled d ) 
#else
zsqrtdet ( a , b , c , d ) 
  scaled a ;
  scaled b ;
  scaled c ;
  scaled d ;
#endif
{
  register scaled Result; scaled maxabs  ;
  integer s  ;
  maxabs = abs ( a ) ;
  if ( abs ( b ) > maxabs ) 
  maxabs = abs ( b ) ;
  if ( abs ( c ) > maxabs ) 
  maxabs = abs ( c ) ;
  if ( abs ( d ) > maxabs ) 
  maxabs = abs ( d ) ;
  s = 64 ;
  while ( ( maxabs < 268435456L ) && ( s > 1 ) ) {
      
    a = a + a ;
    b = b + b ;
    c = c + c ;
    d = d + d ;
    maxabs = maxabs + maxabs ;
    s = halfp ( s ) ;
  } 
  Result = s * squarert ( abs ( takefraction ( a , d ) - takefraction ( b , c 
  ) ) ) ;
  return Result ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zgetpenscale ( halfword p ) 
#else
zgetpenscale ( p ) 
  halfword p ;
#endif
{
  register scaled Result; Result = sqrtdet ( mem [p + 3 ].cint - mem [p + 
  1 ].cint , mem [p + 5 ].cint - mem [p + 1 ].cint , mem [p + 4 ].cint 
  - mem [p + 2 ].cint , mem [p + 6 ].cint - mem [p + 2 ].cint ) ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintcompactnode ( halfword p , smallnumber k ) 
#else
zprintcompactnode ( p , k ) 
  halfword p ;
  smallnumber k ;
#endif
{
  halfword q  ;
  q = p + k - 1 ;
  printchar ( 40 ) ;
  while ( p <= q ) {
      
    printscaled ( mem [p ].cint ) ;
    if ( p < q ) 
    printchar ( 44 ) ;
    incr ( p ) ;
  } 
  printchar ( 41 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintobjcolor ( halfword p ) 
#else
zprintobjcolor ( p ) 
  halfword p ;
#endif
{
  if ( ( mem [p + 2 ].cint > 0 ) || ( mem [p + 3 ].cint > 0 ) || ( mem [
  p + 4 ].cint > 0 ) ) 
  {
    print ( 565 ) ;
    printcompactnode ( p + 2 , 3 ) ;
  } 
} 
scaled 
#ifdef HAVE_PROTOTYPES
zdashoffset ( halfword h ) 
#else
zdashoffset ( h ) 
  halfword h ;
#endif
{
  register scaled Result; scaled x  ;
  if ( ( mem [h ].hhfield .v.RH == 2 ) || ( mem [h + 1 ].cint < 0 ) ) 
  confusion ( 573 ) ;
  if ( mem [h + 1 ].cint == 0 ) 
  x = 0 ;
  else {
      
    x = - (integer) ( mem [mem [h ].hhfield .v.RH + 1 ].cint % mem [h + 1 
    ].cint ) ;
    if ( x < 0 ) 
    x = x + mem [h + 1 ].cint ;
  } 
  Result = x ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintedges ( halfword h , strnumber s , boolean nuline ) 
#else
zprintedges ( h , s , nuline ) 
  halfword h ;
  strnumber s ;
  boolean nuline ;
#endif
{
  halfword p  ;
  halfword hh, pp  ;
  scaled scf  ;
  boolean oktodash  ;
  printdiagnostic ( 552 , s , nuline ) ;
  p = h + 7 ;
  while ( mem [p ].hhfield .v.RH != 0 ) {
      
    p = mem [p ].hhfield .v.RH ;
    println () ;
    switch ( mem [p ].hhfield .b0 ) 
    {case 1 : 
      {
	print ( 555 ) ;
	printobjcolor ( p ) ;
	printchar ( 58 ) ;
	println () ;
	prpath ( mem [p + 1 ].hhfield .v.RH ) ;
	println () ;
	if ( ( mem [p + 1 ].hhfield .lhfield != 0 ) ) 
	{
	  switch ( mem [p ].hhfield .b1 ) 
	  {case 0 : 
	    {
	      print ( 557 ) ;
	      printscaled ( mem [p + 5 ].cint ) ;
	    } 
	    break ;
	  case 1 : 
	    print ( 558 ) ;
	    break ;
	  case 2 : 
	    print ( 559 ) ;
	    break ;
	    default: 
	    print ( 560 ) ;
	    break ;
	  } 
	  print ( 556 ) ;
	  println () ;
	  prpen ( mem [p + 1 ].hhfield .lhfield ) ;
	} 
      } 
      break ;
    case 2 : 
      {
	print ( 566 ) ;
	printobjcolor ( p ) ;
	printchar ( 58 ) ;
	println () ;
	prpath ( mem [p + 1 ].hhfield .v.RH ) ;
	if ( mem [p + 6 ].hhfield .v.RH != 0 ) 
	{
	  printnl ( 567 ) ;
	  oktodash = ( mem [p + 1 ].hhfield .lhfield == mem [mem [p + 1 ]
	  .hhfield .lhfield ].hhfield .v.RH ) ;
	  if ( ! oktodash ) 
	  scf = 65536L ;
	  else scf = mem [p + 7 ].cint ;
	  hh = mem [p + 6 ].hhfield .v.RH ;
	  pp = mem [hh ].hhfield .v.RH ;
	  if ( ( pp == 2 ) || ( mem [hh + 1 ].cint < 0 ) ) 
	  print ( 568 ) ;
	  else {
	      
	    mem [3 ].cint = mem [pp + 1 ].cint + mem [hh + 1 ].cint ;
	    while ( pp != 2 ) {
		
	      print ( 569 ) ;
	      printscaled ( takescaled ( mem [pp + 2 ].cint - mem [pp + 1 ]
	      .cint , scf ) ) ;
	      print ( 570 ) ;
	      printscaled ( takescaled ( mem [mem [pp ].hhfield .v.RH + 1 ]
	      .cint - mem [pp + 2 ].cint , scf ) ) ;
	      pp = mem [pp ].hhfield .v.RH ;
	      if ( pp != 2 ) 
	      printchar ( 32 ) ;
	    } 
	    print ( 571 ) ;
	    printscaled ( - (integer) takescaled ( dashoffset ( hh ) , scf ) ) 
	    ;
	    if ( ! oktodash || ( mem [hh + 1 ].cint == 0 ) ) 
	    print ( 572 ) ;
	  } 
	} 
	println () ;
	switch ( mem [p + 6 ].hhfield .b0 ) 
	{case 0 : 
	  print ( 561 ) ;
	  break ;
	case 1 : 
	  print ( 562 ) ;
	  break ;
	case 2 : 
	  print ( 563 ) ;
	  break ;
	  default: 
	  print ( 539 ) ;
	  break ;
	} 
	print ( 564 ) ;
	switch ( mem [p ].hhfield .b1 ) 
	{case 0 : 
	  {
	    print ( 557 ) ;
	    printscaled ( mem [p + 5 ].cint ) ;
	  } 
	  break ;
	case 1 : 
	  print ( 558 ) ;
	  break ;
	case 2 : 
	  print ( 559 ) ;
	  break ;
	  default: 
	  print ( 560 ) ;
	  break ;
	} 
	print ( 556 ) ;
	println () ;
	if ( mem [p + 1 ].hhfield .lhfield == 0 ) 
	print ( 260 ) ;
	else prpen ( mem [p + 1 ].hhfield .lhfield ) ;
      } 
      break ;
    case 3 : 
      {
	printchar ( 34 ) ;
	print ( mem [p + 1 ].hhfield .v.RH ) ;
	print ( 574 ) ;
	print ( fontname [mem [p + 1 ].hhfield .lhfield ]) ;
	printchar ( 34 ) ;
	println () ;
	printobjcolor ( p ) ;
	print ( 575 ) ;
	printcompactnode ( p + 8 , 6 ) ;
      } 
      break ;
    case 4 : 
      {
	print ( 576 ) ;
	println () ;
	prpath ( mem [p + 1 ].hhfield .v.RH ) ;
      } 
      break ;
    case 6 : 
      print ( 577 ) ;
      break ;
    case 5 : 
      {
	print ( 578 ) ;
	println () ;
	prpath ( mem [p + 1 ].hhfield .v.RH ) ;
      } 
      break ;
    case 7 : 
      print ( 579 ) ;
      break ;
      default: 
      {
	print ( 553 ) ;
      } 
      break ;
    } 
  } 
  printnl ( 554 ) ;
  if ( p != mem [h + 7 ].hhfield .lhfield ) 
  print ( 63 ) ;
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
    confusion ( 600 ) ;
    printvariablename ( q ) ;
    v = mem [q + 1 ].cint % 64 ;
    while ( v > 0 ) {
	
      print ( 601 ) ;
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
  else print ( 814 ) ;
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
  case 11 : 
  case 9 : 
  case 12 : 
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
      mem [p ].hhfield .b1 = 14 ;
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
  case 11 : 
  case 9 : 
  case 12 : 
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
    print ( 324 ) ;
    break ;
  case 2 : 
    if ( v == 30 ) 
    print ( 347 ) ;
    else print ( 348 ) ;
    break ;
  case 3 : 
  case 5 : 
  case 7 : 
  case 11 : 
  case 9 : 
  case 15 : 
    {
      printtype ( t ) ;
      if ( v != 0 ) 
      {
	printchar ( 32 ) ;
	while ( ( mem [v ].hhfield .b1 == 14 ) && ( v != p ) ) v = mem [v + 
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
  case 10 : 
    if ( verbosity <= 1 ) 
    printtype ( t ) ;
    else {
	
      if ( selector == 10 ) 
      if ( internal [12 ]<= 0 ) 
      {
	selector = 8 ;
	printtype ( t ) ;
	print ( 813 ) ;
	selector = 10 ;
      } 
      switch ( t ) 
      {case 6 : 
	printpen ( v , 284 , false ) ;
	break ;
      case 8 : 
	printpath ( v , 284 , false ) ;
	break ;
      case 10 : 
	printedges ( v , 284 , false ) ;
	break ;
      } 
    } 
    break ;
  case 12 : 
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
    confusion ( 812 ) ;
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
  printnl ( 804 ) ;
  printexp ( p , 1 ) ;
  if ( s != 284 ) 
  {
    printnl ( 262 ) ;
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
  if ( internal [30 ]> 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 602 ) ;
    } 
    printscaled ( x ) ;
    printchar ( 41 ) ;
    {
      helpptr = 4 ;
      helpline [3 ]= 603 ;
      helpline [2 ]= 604 ;
      helpline [1 ]= 605 ;
      helpline [0 ]= 606 ;
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
    printnl ( 607 ) ;
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
  r = mem [5 ].hhfield .v.RH ;
  s = 0 ;
  while ( r != 5 ) {
      
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
zflushdashlist ( halfword h ) 
#else
zflushdashlist ( h ) 
  halfword h ;
#endif
{
  halfword p, q  ;
  q = mem [h ].hhfield .v.RH ;
  while ( q != 2 ) {
      
    p = q ;
    q = mem [q ].hhfield .v.RH ;
    freenode ( p , 3 ) ;
  } 
  mem [h ].hhfield .v.RH = 2 ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
ztossgrobject ( halfword p ) 
#else
ztossgrobject ( p ) 
  halfword p ;
#endif
{
  register halfword Result; halfword e  ;
  e = 0 ;
  switch ( mem [p ].hhfield .b0 ) 
  {case 1 : 
    {
      tossknotlist ( mem [p + 1 ].hhfield .v.RH ) ;
      if ( mem [p + 1 ].hhfield .lhfield != 0 ) 
      tossknotlist ( mem [p + 1 ].hhfield .lhfield ) ;
    } 
    break ;
  case 2 : 
    {
      tossknotlist ( mem [p + 1 ].hhfield .v.RH ) ;
      if ( mem [p + 1 ].hhfield .lhfield != 0 ) 
      tossknotlist ( mem [p + 1 ].hhfield .lhfield ) ;
      e = mem [p + 6 ].hhfield .v.RH ;
    } 
    break ;
  case 3 : 
    {
      if ( strref [mem [p + 1 ].hhfield .v.RH ]< 127 ) 
      if ( strref [mem [p + 1 ].hhfield .v.RH ]> 1 ) 
      decr ( strref [mem [p + 1 ].hhfield .v.RH ]) ;
      else flushstring ( mem [p + 1 ].hhfield .v.RH ) ;
    } 
    break ;
  case 4 : 
  case 5 : 
    tossknotlist ( mem [p + 1 ].hhfield .v.RH ) ;
    break ;
  case 6 : 
  case 7 : 
    ;
    break ;
  } 
  freenode ( p , grobjectsize [mem [p ].hhfield .b0 ]) ;
  Result = e ;
  return Result ;
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
  halfword r  ;
  flushdashlist ( h ) ;
  q = mem [h + 7 ].hhfield .v.RH ;
  while ( ( q != 0 ) ) {
      
    p = q ;
    q = mem [q ].hhfield .v.RH ;
    r = tossgrobject ( p ) ;
    if ( r != 0 ) 
    if ( mem [r ].hhfield .lhfield == 0 ) 
    tossedges ( r ) ;
    else decr ( mem [r ].hhfield .lhfield ) ;
  } 
  freenode ( h , 8 ) ;
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
  case 11 : 
  case 9 : 
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
  case 8 : 
  case 6 : 
    tossknotlist ( v ) ;
    break ;
  case 10 : 
    if ( mem [v ].hhfield .lhfield == 0 ) 
    tossedges ( v ) ;
    else decr ( mem [v ].hhfield .lhfield ) ;
    break ;
  case 14 : 
  case 13 : 
  case 12 : 
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
      q = mem [5 ].hhfield .v.RH ;
      while ( q != 5 ) {
	  
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
	  printnl ( 816 ) ;
	  if ( v > 0 ) 
	  printchar ( 45 ) ;
	  if ( t == 17 ) 
	  vv = roundfraction ( maxc [17 ]) ;
	  else vv = maxc [18 ];
	  if ( vv != 65536L ) 
	  printscaled ( vv ) ;
	  printvariablename ( p ) ;
	  while ( mem [p + 1 ].cint % 64 > 0 ) {
	      
	    print ( 601 ) ;
	    mem [p + 1 ].cint = mem [p + 1 ].cint - 2 ;
	  } 
	  if ( t == 17 ) 
	  printchar ( 61 ) ;
	  else print ( 817 ) ;
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
    confusion ( 815 ) ;
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
  case 11 : 
  case 9 : 
  case 12 : 
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
  case 4 : 
    {
      if ( strref [curexp ]< 127 ) 
      if ( strref [curexp ]> 1 ) 
      decr ( strref [curexp ]) ;
      else flushstring ( curexp ) ;
    } 
    break ;
  case 6 : 
  case 8 : 
    tossknotlist ( curexp ) ;
    break ;
  case 10 : 
    if ( mem [curexp ].hhfield .lhfield == 0 ) 
    tossedges ( curexp ) ;
    else decr ( mem [curexp ].hhfield .lhfield ) ;
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
    } while ( ! ( q == 9 ) ) ;
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
    Result = 7 ;
    break ;
  case 8 : 
  case 9 : 
    Result = 9 ;
    break ;
  case 10 : 
  case 11 : 
    Result = 11 ;
    break ;
  case 12 : 
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
  switch ( eqtb [p ].lhfield % 85 ) 
  {case 13 : 
  case 55 : 
  case 46 : 
  case 51 : 
    if ( ! saving ) 
    deletemacref ( q ) ;
    break ;
  case 43 : 
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
  eqtb [p ]= eqtb [9771 ];
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
    mem [p ].hhfield .lhfield = 9771 + q ;
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
    if ( q > 9771 ) 
    {
      if ( internal [7 ]> 0 ) 
      {
	begindiagnostic () ;
	printnl ( 531 ) ;
	print ( intname [q - ( 9771 ) ]) ;
	printchar ( 61 ) ;
	printscaled ( mem [saveptr + 1 ].cint ) ;
	printchar ( 125 ) ;
	enddiagnostic ( false ) ;
      } 
      internal [q - ( 9771 ) ]= mem [saveptr + 1 ].cint ;
    } 
    else {
	
      if ( internal [7 ]> 0 ) 
      {
	begindiagnostic () ;
	printnl ( 531 ) ;
	print ( hash [q ].v.RH ) ;
	printchar ( 125 ) ;
	enddiagnostic ( false ) ;
      } 
      clearsymbol ( q , false ) ;
      eqtb [q ]= mem [saveptr + 1 ].hhfield ;
      if ( eqtb [q ].lhfield % 85 == 43 ) 
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
  register halfword Result; halfword q, pp, qq  ;
  q = copyknot ( p ) ;
  qq = q ;
  pp = mem [p ].hhfield .v.RH ;
  while ( pp != p ) {
      
    mem [qq ].hhfield .v.RH = copyknot ( pp ) ;
    qq = mem [qq ].hhfield .v.RH ;
    pp = mem [pp ].hhfield .v.RH ;
  } 
  mem [qq ].hhfield .v.RH = q ;
  Result = q ;
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
  printpath ( knots , 541 , true ) ;
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
	overflow ( 546 , pathsize ) ;
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
    else if ( mem [p ].hhfield .b1 == 0 ) 
    {
      mem [p + 5 ].cint = mem [p + 1 ].cint ;
      mem [p + 6 ].cint = mem [p + 2 ].cint ;
      mem [q + 3 ].cint = mem [q + 1 ].cint ;
      mem [q + 4 ].cint = mem [q + 2 ].cint ;
    } 
    p = q ;
  } while ( ! ( p == h ) ) ;
  if ( internal [4 ]> 0 ) 
  printpath ( knots , 542 , true ) ;
  if ( aritherror ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 543 ) ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 544 ;
      helpline [0 ]= 545 ;
    } 
    putgeterror () ;
    aritherror = false ;
  } 
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
scaled 
#ifdef HAVE_PROTOTYPES
zevalcubic ( halfword p , halfword q , fraction t ) 
#else
zevalcubic ( p , q , t ) 
  halfword p ;
  halfword q ;
  fraction t ;
#endif
{
  register scaled Result; scaled x1, x2, x3  ;
  x1 = mem [p ].cint - takefraction ( mem [p ].cint - mem [p + 4 ].cint 
  , t ) ;
  x2 = mem [p + 4 ].cint - takefraction ( mem [p + 4 ].cint - mem [q + 2 
  ].cint , t ) ;
  x3 = mem [q + 2 ].cint - takefraction ( mem [q + 2 ].cint - mem [q ]
  .cint , t ) ;
  x1 = x1 - takefraction ( x1 - x2 , t ) ;
  x2 = x2 - takefraction ( x2 - x3 , t ) ;
  Result = x1 - takefraction ( x1 - x2 , t ) ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zboundcubic ( halfword p , halfword q , smallnumber c ) 
#else
zboundcubic ( p , q , c ) 
  halfword p ;
  halfword q ;
  smallnumber c ;
#endif
{
  boolean wavy  ;
  scaled del1, del2, del3, del, dmax  ;
  fraction t, tt  ;
  scaled x  ;
  x = mem [q ].cint ;
  if ( x < bbmin [c ]) 
  bbmin [c ]= x ;
  if ( x > bbmax [c ]) 
  bbmax [c ]= x ;
  wavy = true ;
  if ( bbmin [c ]<= mem [p + 4 ].cint ) 
  if ( mem [p + 4 ].cint <= bbmax [c ]) 
  if ( bbmin [c ]<= mem [q + 2 ].cint ) 
  if ( mem [q + 2 ].cint <= bbmax [c ]) 
  wavy = false ;
  if ( wavy ) 
  {
    del1 = mem [p + 4 ].cint - mem [p ].cint ;
    del2 = mem [q + 2 ].cint - mem [p + 4 ].cint ;
    del3 = mem [q ].cint - mem [q + 2 ].cint ;
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
    if ( del < 0 ) 
    {
      del1 = - (integer) del1 ;
      del2 = - (integer) del2 ;
      del3 = - (integer) del3 ;
    } 
    t = crossingpoint ( del1 , del2 , del3 ) ;
    if ( t < 268435456L ) 
    {
      x = evalcubic ( p , q , t ) ;
      if ( x < bbmin [c ]) 
      bbmin [c ]= x ;
      if ( x > bbmax [c ]) 
      bbmax [c ]= x ;
      del2 = del2 - takefraction ( del2 - del3 , t ) ;
      if ( del2 > 0 ) 
      del2 = 0 ;
      tt = crossingpoint ( 0 , - (integer) del2 , - (integer) del3 ) ;
      if ( tt < 268435456L ) 
      {
	x = evalcubic ( p , q , tt - takefraction ( tt - 268435456L , t ) ) ;
	if ( x < bbmin [c ]) 
	bbmin [c ]= x ;
	if ( x > bbmax [c ]) 
	bbmax [c ]= x ;
      } 
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zpathbbox ( halfword h ) 
#else
zpathbbox ( h ) 
  halfword h ;
#endif
{
  /* 10 */ halfword p, q  ;
  bbmin [0 ]= mem [h + 1 ].cint ;
  bbmin [1 ]= mem [h + 2 ].cint ;
  bbmax [0 ]= bbmin [0 ];
  bbmax [1 ]= bbmin [1 ];
  p = h ;
  do {
      if ( mem [p ].hhfield .b1 == 0 ) 
    goto lab10 ;
    q = mem [p ].hhfield .v.RH ;
    boundcubic ( p + 1 , q + 1 , 0 ) ;
    boundcubic ( p + 2 , q + 2 , 1 ) ;
    p = q ;
  } while ( ! ( p == h ) ) ;
  lab10: ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zsolverisingcubic ( scaled a , scaled b , scaled c , scaled x ) 
#else
zsolverisingcubic ( a , b , c , x ) 
  scaled a ;
  scaled b ;
  scaled c ;
  scaled x ;
#endif
{
  register scaled Result; scaled ab, bc, ac  ;
  integer t  ;
  integer xx  ;
  if ( ( a < 0 ) || ( c < 0 ) ) 
  confusion ( 547 ) ;
  if ( x <= 0 ) 
  Result = 0 ;
  else if ( x >= a + b + c ) 
  Result = 65536L ;
  else {
      
    t = 1 ;
    while ( ( a > 715827882L ) || ( b > 715827882L ) || ( c > 715827882L ) ) {
	
      a = halfp ( a ) ;
      b = half ( b ) ;
      c = halfp ( c ) ;
      x = halfp ( x ) ;
    } 
    do {
	t = t + t ;
      ab = half ( a + b ) ;
      bc = half ( b + c ) ;
      ac = half ( ab + bc ) ;
      xx = x - a - ab - ac ;
      if ( xx < - (integer) x ) 
      {
	x = x + x ;
	b = ab ;
	c = ac ;
      } 
      else {
	  
	x = x + xx ;
	a = ac ;
	b = bc ;
	t = t + 1 ;
      } 
    } while ( ! ( t >= 65536L ) ) ;
    Result = t - 65536L ;
  } 
  return Result ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zarctest ( scaled dx0 , scaled dy0 , scaled dx1 , scaled dy1 , scaled dx2 , 
scaled dy2 , scaled v0 , scaled v02 , scaled v2 , scaled agoal , scaled tol ) 
#else
zarctest ( dx0 , dy0 , dx1 , dy1 , dx2 , dy2 , v0 , v02 , v2 , agoal , tol ) 
  scaled dx0 ;
  scaled dy0 ;
  scaled dx1 ;
  scaled dy1 ;
  scaled dx2 ;
  scaled dy2 ;
  scaled v0 ;
  scaled v02 ;
  scaled v2 ;
  scaled agoal ;
  scaled tol ;
#endif
{
  /* 10 */ register scaled Result; boolean simple  ;
  scaled dx01, dy01, dx12, dy12, dx02, dy02  ;
  scaled v002, v022  ;
  scaled arc  ;
  scaled a, b  ;
  scaled anew, aaux  ;
  scaled tmp, tmp2  ;
  scaled arc1  ;
  dx01 = half ( dx0 + dx1 ) ;
  dx12 = half ( dx1 + dx2 ) ;
  dx02 = half ( dx01 + dx12 ) ;
  dy01 = half ( dy0 + dy1 ) ;
  dy12 = half ( dy1 + dy2 ) ;
  dy02 = half ( dy01 + dy12 ) ;
  v002 = pythadd ( dx01 + half ( dx0 + dx02 ) , dy01 + half ( dy0 + dy02 ) ) ;
  v022 = pythadd ( dx12 + half ( dx02 + dx2 ) , dy12 + half ( dy02 + dy2 ) ) ;
  tmp = halfp ( v02 + 2 ) ;
  arc1 = v002 + half ( halfp ( v0 + tmp ) - v002 ) ;
  arc = v022 + half ( halfp ( v2 + tmp ) - v022 ) ;
  if ( ( arc < 2147483647L - arc1 ) ) 
  arc = arc + arc1 ;
  else {
      
    aritherror = true ;
    if ( agoal == 2147483647L ) 
    Result = 2147483647L ;
    else Result = -131072L ;
    goto lab10 ;
  } 
  simple = ( dx0 >= 0 ) && ( dx1 >= 0 ) && ( dx2 >= 0 ) || ( dx0 <= 0 ) && ( 
  dx1 <= 0 ) && ( dx2 <= 0 ) ;
  if ( simple ) 
  simple = ( dy0 >= 0 ) && ( dy1 >= 0 ) && ( dy2 >= 0 ) || ( dy0 <= 0 ) && ( 
  dy1 <= 0 ) && ( dy2 <= 0 ) ;
  if ( ! simple ) 
  {
    simple = ( dx0 >= dy0 ) && ( dx1 >= dy1 ) && ( dx2 >= dy2 ) || ( dx0 <= 
    dy0 ) && ( dx1 <= dy1 ) && ( dx2 <= dy2 ) ;
    if ( simple ) 
    simple = ( - (integer) dx0 >= dy0 ) && ( - (integer) dx1 >= dy1 ) && ( 
    - (integer) dx2 >= dy2 ) || ( - (integer) dx0 <= dy0 ) && ( - (integer) 
    dx1 <= dy1 ) && ( - (integer) dx2 <= dy2 ) ;
  } 
  if ( simple && ( abs ( arc - v02 - halfp ( v0 + v2 ) ) <= tol ) ) 
  if ( arc < agoal ) 
  Result = arc ;
  else {
      
    tmp = ( v02 + 2 ) / 4 ;
    if ( agoal <= arc1 ) 
    {
      tmp2 = halfp ( v0 ) ;
      Result = halfp ( solverisingcubic ( tmp2 , arc1 - tmp2 - tmp , tmp , 
      agoal ) ) - 131072L ;
    } 
    else {
	
      tmp2 = halfp ( v2 ) ;
      Result = ( -98304L ) + halfp ( solverisingcubic ( tmp , arc - arc1 - tmp 
      - tmp2 , tmp2 , agoal - arc1 ) ) ;
    } 
  } 
  else {
      
    aaux = 2147483647L - agoal ;
    if ( agoal > aaux ) 
    {
      aaux = agoal - aaux ;
      anew = 2147483647L ;
    } 
    else {
	
      anew = agoal + agoal ;
      aaux = 0 ;
    } 
    tol = tol + halfp ( tol ) ;
    a = arctest ( dx0 , dy0 , dx01 , dy01 , dx02 , dy02 , v0 , v002 , halfp ( 
    v02 ) , anew , tol ) ;
    if ( a < 0 ) 
    Result = - (integer) halfp ( 131072L - a ) ;
    else {
	
      if ( a > aaux ) 
      {
	aaux = aaux - a ;
	anew = anew + aaux ;
      } 
      b = arctest ( dx02 , dy02 , dx12 , dy12 , dx2 , dy2 , halfp ( v02 ) , 
      v022 , v2 , anew , tol ) ;
      if ( b < 0 ) 
      Result = - (integer) halfp ( - (integer) b ) - 32768L ;
      else Result = a + half ( b - a ) ;
    } 
  } 
  lab10: ;
  return Result ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zdoarctest ( scaled dx0 , scaled dy0 , scaled dx1 , scaled dy1 , scaled dx2 , 
scaled dy2 , scaled agoal ) 
#else
zdoarctest ( dx0 , dy0 , dx1 , dy1 , dx2 , dy2 , agoal ) 
  scaled dx0 ;
  scaled dy0 ;
  scaled dx1 ;
  scaled dy1 ;
  scaled dx2 ;
  scaled dy2 ;
  scaled agoal ;
#endif
{
  register scaled Result; scaled v0, v1, v2  ;
  scaled v02  ;
  v0 = pythadd ( dx0 , dy0 ) ;
  v1 = pythadd ( dx1 , dy1 ) ;
  v2 = pythadd ( dx2 , dy2 ) ;
  if ( ( v0 >= 1073741824L ) || ( v1 >= 1073741824L ) || ( v2 >= 1073741824L ) 
  ) 
  {
    aritherror = true ;
    if ( agoal == 2147483647L ) 
    Result = 2147483647L ;
    else Result = -131072L ;
  } 
  else {
      
    v02 = pythadd ( dx1 + half ( dx0 + dx2 ) , dy1 + half ( dy0 + dy2 ) ) ;
    Result = arctest ( dx0 , dy0 , dx1 , dy1 , dx2 , dy2 , v0 , v02 , v2 , 
    agoal , 16 ) ;
  } 
  return Result ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zgetarclength ( halfword h ) 
#else
zgetarclength ( h ) 
  halfword h ;
#endif
{
  /* 30 */ register scaled Result; halfword p, q  ;
  scaled a, atot  ;
  atot = 0 ;
  p = h ;
  while ( mem [p ].hhfield .b1 != 0 ) {
      
    q = mem [p ].hhfield .v.RH ;
    a = doarctest ( mem [p + 5 ].cint - mem [p + 1 ].cint , mem [p + 6 ]
    .cint - mem [p + 2 ].cint , mem [q + 3 ].cint - mem [p + 5 ].cint , 
    mem [q + 4 ].cint - mem [p + 6 ].cint , mem [q + 1 ].cint - mem [q 
    + 3 ].cint , mem [q + 2 ].cint - mem [q + 4 ].cint , 2147483647L ) ;
    atot = slowadd ( a , atot ) ;
    if ( q == h ) 
    goto lab30 ;
    else p = q ;
  } 
  lab30: {
      
    if ( aritherror ) 
    cleararith () ;
  } 
  Result = atot ;
  return Result ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zgetarctime ( halfword h , scaled arc0 ) 
#else
zgetarctime ( h , arc0 ) 
  halfword h ;
  scaled arc0 ;
#endif
{
  /* 30 */ register scaled Result; halfword p, q  ;
  scaled ttot  ;
  scaled t  ;
  scaled arc  ;
  integer n  ;
  if ( arc0 < 0 ) 
  {
    if ( mem [h ].hhfield .b0 == 0 ) 
    ttot = 0 ;
    else {
	
      p = htapypoc ( h ) ;
      ttot = - (integer) getarctime ( p , - (integer) arc0 ) ;
      tossknotlist ( p ) ;
    } 
    goto lab30 ;
  } 
  if ( arc0 == 2147483647L ) 
  decr ( arc0 ) ;
  ttot = 0 ;
  arc = arc0 ;
  p = h ;
  while ( ( mem [p ].hhfield .b1 != 0 ) && ( arc > 0 ) ) {
      
    q = mem [p ].hhfield .v.RH ;
    t = doarctest ( mem [p + 5 ].cint - mem [p + 1 ].cint , mem [p + 6 ]
    .cint - mem [p + 2 ].cint , mem [q + 3 ].cint - mem [p + 5 ].cint , 
    mem [q + 4 ].cint - mem [p + 6 ].cint , mem [q + 1 ].cint - mem [q 
    + 3 ].cint , mem [q + 2 ].cint - mem [q + 4 ].cint , arc ) ;
    if ( t < 0 ) 
    {
      ttot = ttot + t + 131072L ;
      arc = 0 ;
    } 
    else {
	
      ttot = ttot + 65536L ;
      arc = arc - t ;
    } 
    if ( q == h ) 
    if ( arc > 0 ) 
    {
      n = arc / ( arc0 - arc ) ;
      arc = arc - n * ( arc0 - arc ) ;
      if ( ttot > 2147483647L / ( n + 1 ) ) 
      {
	aritherror = true ;
	ttot = 2147483647L ;
	goto lab30 ;
      } 
      ttot = ( n + 1 ) * ttot ;
    } 
    p = q ;
  } 
  lab30: {
      
    if ( aritherror ) 
    cleararith () ;
  } 
  Result = ttot ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zmoveknot ( halfword p , halfword q ) 
#else
zmoveknot ( p , q ) 
  halfword p ;
  halfword q ;
#endif
{
  mem [mem [p ].hhfield .lhfield ].hhfield .v.RH = mem [p ].hhfield 
  .v.RH ;
  mem [mem [p ].hhfield .v.RH ].hhfield .lhfield = mem [p ].hhfield 
  .lhfield ;
  mem [p ].hhfield .lhfield = q ;
  mem [p ].hhfield .v.RH = mem [q ].hhfield .v.RH ;
  mem [q ].hhfield .v.RH = p ;
  mem [mem [p ].hhfield .v.RH ].hhfield .lhfield = p ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zconvexhull ( halfword h ) 
#else
zconvexhull ( h ) 
  halfword h ;
#endif
{
  /* 31 32 33 */ register halfword Result; halfword l, r  ;
  halfword p, q  ;
  halfword s  ;
  scaled dx, dy  ;
  if ( ( h == mem [h ].hhfield .v.RH ) ) 
  Result = h ;
  else {
      
    l = h ;
    p = mem [h ].hhfield .v.RH ;
    while ( p != h ) {
	
      if ( mem [p + 1 ].cint <= mem [l + 1 ].cint ) 
      if ( ( mem [p + 1 ].cint < mem [l + 1 ].cint ) || ( mem [p + 2 ]
      .cint < mem [l + 2 ].cint ) ) 
      l = p ;
      p = mem [p ].hhfield .v.RH ;
    } 
    r = h ;
    p = mem [h ].hhfield .v.RH ;
    while ( p != h ) {
	
      if ( mem [p + 1 ].cint >= mem [r + 1 ].cint ) 
      if ( ( mem [p + 1 ].cint > mem [r + 1 ].cint ) || ( mem [p + 2 ]
      .cint > mem [r + 2 ].cint ) ) 
      r = p ;
      p = mem [p ].hhfield .v.RH ;
    } 
    if ( l != r ) 
    {
      s = mem [r ].hhfield .v.RH ;
      dx = mem [r + 1 ].cint - mem [l + 1 ].cint ;
      dy = mem [r + 2 ].cint - mem [l + 2 ].cint ;
      p = mem [l ].hhfield .v.RH ;
      while ( p != r ) {
	  
	q = mem [p ].hhfield .v.RH ;
	if ( abvscd ( dx , mem [p + 2 ].cint - mem [l + 2 ].cint , dy , 
	mem [p + 1 ].cint - mem [l + 1 ].cint ) > 0 ) 
	moveknot ( p , r ) ;
	p = q ;
      } 
      p = s ;
      while ( p != l ) {
	  
	q = mem [p ].hhfield .v.RH ;
	if ( abvscd ( dx , mem [p + 2 ].cint - mem [l + 2 ].cint , dy , 
	mem [p + 1 ].cint - mem [l + 1 ].cint ) < 0 ) 
	moveknot ( p , l ) ;
	p = q ;
      } 
      p = mem [l ].hhfield .v.RH ;
      while ( p != r ) {
	  
	q = mem [p ].hhfield .lhfield ;
	while ( mem [q + 1 ].cint > mem [p + 1 ].cint ) q = mem [q ]
	.hhfield .lhfield ;
	while ( mem [q + 1 ].cint == mem [p + 1 ].cint ) if ( mem [q + 2 
	].cint > mem [p + 2 ].cint ) 
	q = mem [q ].hhfield .lhfield ;
	else goto lab31 ;
	lab31: if ( q == mem [p ].hhfield .lhfield ) 
	p = mem [p ].hhfield .v.RH ;
	else {
	    
	  p = mem [p ].hhfield .v.RH ;
	  moveknot ( mem [p ].hhfield .lhfield , q ) ;
	} 
      } 
      p = mem [r ].hhfield .v.RH ;
      while ( p != l ) {
	  
	q = mem [p ].hhfield .lhfield ;
	while ( mem [q + 1 ].cint < mem [p + 1 ].cint ) q = mem [q ]
	.hhfield .lhfield ;
	while ( mem [q + 1 ].cint == mem [p + 1 ].cint ) if ( mem [q + 2 
	].cint < mem [p + 2 ].cint ) 
	q = mem [q ].hhfield .lhfield ;
	else goto lab32 ;
	lab32: if ( q == mem [p ].hhfield .lhfield ) 
	p = mem [p ].hhfield .v.RH ;
	else {
	    
	  p = mem [p ].hhfield .v.RH ;
	  moveknot ( mem [p ].hhfield .lhfield , q ) ;
	} 
      } 
    } 
    if ( l != mem [l ].hhfield .v.RH ) 
    {
      p = l ;
      q = mem [l ].hhfield .v.RH ;
      while ( true ) {
	  
	dx = mem [q + 1 ].cint - mem [p + 1 ].cint ;
	dy = mem [q + 2 ].cint - mem [p + 2 ].cint ;
	p = q ;
	q = mem [q ].hhfield .v.RH ;
	if ( p == l ) 
	goto lab33 ;
	if ( p != r ) 
	if ( abvscd ( dx , mem [q + 2 ].cint - mem [p + 2 ].cint , dy , 
	mem [q + 1 ].cint - mem [p + 1 ].cint ) <= 0 ) 
	{
	  s = mem [p ].hhfield .lhfield ;
	  freenode ( p , 7 ) ;
	  mem [s ].hhfield .v.RH = q ;
	  mem [q ].hhfield .lhfield = s ;
	  if ( s == l ) 
	  p = s ;
	  else {
	      
	    p = mem [s ].hhfield .lhfield ;
	    q = s ;
	  } 
	} 
      } 
      lab33: ;
    } 
    Result = l ;
  } 
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zmakepen ( halfword h , boolean needhull ) 
#else
zmakepen ( h , needhull ) 
  halfword h ;
  boolean needhull ;
#endif
{
  register halfword Result; halfword p, q  ;
  q = h ;
  do {
      p = q ;
    q = mem [q ].hhfield .v.RH ;
    mem [q ].hhfield .lhfield = p ;
  } while ( ! ( q == h ) ) ;
  if ( needhull ) 
  {
    h = convexhull ( h ) ;
    if ( ( h == mem [h ].hhfield .v.RH ) ) 
    {
      mem [h + 3 ].cint = mem [h + 1 ].cint ;
      mem [h + 4 ].cint = mem [h + 2 ].cint ;
      mem [h + 5 ].cint = mem [h + 1 ].cint ;
      mem [h + 6 ].cint = mem [h + 2 ].cint ;
    } 
  } 
  Result = h ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zgetpencircle ( scaled diam ) 
#else
zgetpencircle ( diam ) 
  scaled diam ;
#endif
{
  register halfword Result; halfword h  ;
  h = getnode ( 7 ) ;
  mem [h ].hhfield .v.RH = h ;
  mem [h ].hhfield .lhfield = h ;
  mem [h + 1 ].cint = 0 ;
  mem [h + 2 ].cint = 0 ;
  mem [h + 3 ].cint = diam ;
  mem [h + 4 ].cint = 0 ;
  mem [h + 5 ].cint = 0 ;
  mem [h + 6 ].cint = diam ;
  Result = h ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zmakepath ( halfword h ) 
#else
zmakepath ( h ) 
  halfword h ;
#endif
{
  halfword p  ;
  smallnumber k  ;
  scaled centerx, centery  ;
  scaled widthx, widthy  ;
  scaled heightx, heighty  ;
  scaled dx, dy  ;
  integer kk  ;
  if ( ( h == mem [h ].hhfield .v.RH ) ) 
  {
    centerx = mem [h + 1 ].cint ;
    centery = mem [h + 2 ].cint ;
    widthx = mem [h + 3 ].cint - centerx ;
    widthy = mem [h + 4 ].cint - centery ;
    heightx = mem [h + 5 ].cint - centerx ;
    heighty = mem [h + 6 ].cint - centery ;
    p = h ;
    {register integer for_end; k = 0 ;for_end = 7 ; if ( k <= for_end) do 
      {
	kk = ( k + 6 ) % 8 ;
	mem [p + 1 ].cint = centerx + takefraction ( halfcos [k ], widthx 
	) + takefraction ( halfcos [kk ], heightx ) ;
	mem [p + 2 ].cint = centery + takefraction ( halfcos [k ], widthy 
	) + takefraction ( halfcos [kk ], heighty ) ;
	dx = - (integer) takefraction ( dcos [kk ], widthx ) + takefraction 
	( dcos [k ], heightx ) ;
	dy = - (integer) takefraction ( dcos [kk ], widthy ) + takefraction 
	( dcos [k ], heighty ) ;
	mem [p + 5 ].cint = mem [p + 1 ].cint + dx ;
	mem [p + 6 ].cint = mem [p + 2 ].cint + dy ;
	mem [p + 3 ].cint = mem [p + 1 ].cint - dx ;
	mem [p + 4 ].cint = mem [p + 2 ].cint - dy ;
	mem [p ].hhfield .b0 = 1 ;
	mem [p ].hhfield .b1 = 1 ;
	if ( k == 7 ) 
	mem [p ].hhfield .v.RH = h ;
	else mem [p ].hhfield .v.RH = getnode ( 7 ) ;
	p = mem [p ].hhfield .v.RH ;
      } 
    while ( k++ < for_end ) ;} 
  } 
  else {
      
    p = h ;
    do {
	mem [p ].hhfield .b0 = 1 ;
      mem [p ].hhfield .b1 = 1 ;
      mem [p + 3 ].cint = mem [p + 1 ].cint ;
      mem [p + 4 ].cint = mem [p + 2 ].cint ;
      mem [p + 5 ].cint = mem [p + 1 ].cint ;
      mem [p + 6 ].cint = mem [p + 2 ].cint ;
      p = mem [p ].hhfield .v.RH ;
    } while ( ! ( p == h ) ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zfindoffset ( scaled x , scaled y , halfword h ) 
#else
zfindoffset ( x , y , h ) 
  scaled x ;
  scaled y ;
  halfword h ;
#endif
{
  halfword p, q  ;
  scaled wx, wy, hx, hy  ;
  fraction xx, yy  ;
  fraction d  ;
  if ( ( h == mem [h ].hhfield .v.RH ) ) 
  if ( ( x == 0 ) && ( y == 0 ) ) 
  {
    curx = mem [h + 1 ].cint ;
    cury = mem [h + 2 ].cint ;
  } 
  else {
      
    wx = mem [h + 3 ].cint - mem [h + 1 ].cint ;
    wy = mem [h + 4 ].cint - mem [h + 2 ].cint ;
    hx = mem [h + 5 ].cint - mem [h + 1 ].cint ;
    hy = mem [h + 6 ].cint - mem [h + 2 ].cint ;
    while ( ( abs ( x ) < 134217728L ) && ( abs ( y ) < 134217728L ) ) {
	
      x = x + x ;
      y = y + y ;
    } 
    yy = - (integer) ( takefraction ( x , hy ) + takefraction ( y , 
    - (integer) hx ) ) ;
    xx = takefraction ( x , - (integer) wy ) + takefraction ( y , wx ) ;
    d = pythadd ( xx , yy ) ;
    if ( d > 0 ) 
    {
      xx = half ( makefraction ( xx , d ) ) ;
      yy = half ( makefraction ( yy , d ) ) ;
    } 
    curx = mem [h + 1 ].cint + takefraction ( xx , wx ) + takefraction ( yy 
    , hx ) ;
    cury = mem [h + 2 ].cint + takefraction ( xx , wy ) + takefraction ( yy 
    , hy ) ;
  } 
  else {
      
    q = h ;
    do {
	p = q ;
      q = mem [q ].hhfield .v.RH ;
    } while ( ! ( abvscd ( mem [q + 1 ].cint - mem [p + 1 ].cint , y , mem 
    [q + 2 ].cint - mem [p + 2 ].cint , x ) >= 0 ) ) ;
    do {
	p = q ;
      q = mem [q ].hhfield .v.RH ;
    } while ( ! ( abvscd ( mem [q + 1 ].cint - mem [p + 1 ].cint , y , mem 
    [q + 2 ].cint - mem [p + 2 ].cint , x ) <= 0 ) ) ;
    curx = mem [p + 1 ].cint ;
    cury = mem [p + 2 ].cint ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zpenbbox ( halfword h ) 
#else
zpenbbox ( h ) 
  halfword h ;
#endif
{
  halfword p  ;
  if ( ( h == mem [h ].hhfield .v.RH ) ) 
  {
    findoffset ( 0 , 268435456L , h ) ;
    bbmax [0 ]= curx ;
    bbmin [0 ]= 2 * mem [h + 1 ].cint - curx ;
    findoffset ( -268435456L , 0 , h ) ;
    bbmax [1 ]= cury ;
    bbmin [1 ]= 2 * mem [h + 2 ].cint - cury ;
  } 
  else {
      
    bbmin [0 ]= mem [h + 1 ].cint ;
    bbmax [0 ]= bbmin [0 ];
    bbmin [1 ]= mem [h + 2 ].cint ;
    bbmax [1 ]= bbmin [1 ];
    p = mem [h ].hhfield .v.RH ;
    while ( p != h ) {
	
      if ( mem [p + 1 ].cint < bbmin [0 ]) 
      bbmin [0 ]= mem [p + 1 ].cint ;
      if ( mem [p + 2 ].cint < bbmin [1 ]) 
      bbmin [1 ]= mem [p + 2 ].cint ;
      if ( mem [p + 1 ].cint > bbmax [0 ]) 
      bbmax [0 ]= mem [p + 1 ].cint ;
      if ( mem [p + 2 ].cint > bbmax [1 ]) 
      bbmax [1 ]= mem [p + 2 ].cint ;
      p = mem [p ].hhfield .v.RH ;
    } 
  } 
} 
halfword 
#ifdef HAVE_PROTOTYPES
znewfillnode ( halfword p ) 
#else
znewfillnode ( p ) 
  halfword p ;
#endif
{
  register halfword Result; halfword t  ;
  t = getnode ( 6 ) ;
  mem [t ].hhfield .b0 = 1 ;
  mem [t + 1 ].hhfield .v.RH = p ;
  mem [t + 1 ].hhfield .lhfield = 0 ;
  mem [t + 2 ].cint = 0 ;
  mem [t + 3 ].cint = 0 ;
  mem [t + 4 ].cint = 0 ;
  if ( internal [27 ]> 65536L ) 
  mem [t ].hhfield .b1 = 2 ;
  else if ( internal [27 ]> 0 ) 
  mem [t ].hhfield .b1 = 1 ;
  else mem [t ].hhfield .b1 = 0 ;
  if ( internal [29 ]< 65536L ) 
  mem [t + 5 ].cint = 65536L ;
  else mem [t + 5 ].cint = internal [29 ];
  Result = t ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
znewstrokednode ( halfword p ) 
#else
znewstrokednode ( p ) 
  halfword p ;
#endif
{
  register halfword Result; halfword t  ;
  t = getnode ( 8 ) ;
  mem [t ].hhfield .b0 = 2 ;
  mem [t + 1 ].hhfield .v.RH = p ;
  mem [t + 1 ].hhfield .lhfield = 0 ;
  mem [t + 6 ].hhfield .v.RH = 0 ;
  mem [t + 7 ].cint = 65536L ;
  mem [t + 2 ].cint = 0 ;
  mem [t + 3 ].cint = 0 ;
  mem [t + 4 ].cint = 0 ;
  if ( internal [27 ]> 65536L ) 
  mem [t ].hhfield .b1 = 2 ;
  else if ( internal [27 ]> 0 ) 
  mem [t ].hhfield .b1 = 1 ;
  else mem [t ].hhfield .b1 = 0 ;
  if ( internal [29 ]< 65536L ) 
  mem [t + 5 ].cint = 65536L ;
  else mem [t + 5 ].cint = internal [29 ];
  if ( internal [28 ]> 65536L ) 
  mem [t + 6 ].hhfield .b0 = 2 ;
  else if ( internal [28 ]> 0 ) 
  mem [t + 6 ].hhfield .b0 = 1 ;
  else mem [t + 6 ].hhfield .b0 = 0 ;
  Result = t ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
beginname ( void ) 
#else
beginname ( ) 
#endif
{
  {
    if ( strref [curname ]< 127 ) 
    if ( strref [curname ]> 1 ) 
    decr ( strref [curname ]) ;
    else flushstring ( curname ) ;
  } 
  {
    if ( strref [curarea ]< 127 ) 
    if ( strref [curarea ]> 1 ) 
    decr ( strref [curarea ]) ;
    else flushstring ( curarea ) ;
  } 
  {
    if ( strref [curext ]< 127 ) 
    if ( strref [curext ]> 1 ) 
    decr ( strref [curext ]) ;
    else flushstring ( curext ) ;
  } 
  areadelimiter = -1 ;
  extdelimiter = -1 ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
zmorename ( ASCIIcode c ) 
#else
zmorename ( c ) 
  ASCIIcode c ;
#endif
{
  register boolean Result; if ( ( c == 32 ) || ( c == 9 ) ) 
  Result = false ;
  else {
      
    if ( ISDIRSEP ( c ) ) 
    {
      areadelimiter = poolptr - strstart [strptr ];
      extdelimiter = -1 ;
    } 
    else if ( ( c == 46 ) ) 
    extdelimiter = poolptr - strstart [strptr ];
    {
      if ( poolptr + 1 > maxpoolptr ) 
      if ( poolptr + 1 > poolsize ) 
      docompaction ( 1 ) ;
      else maxpoolptr = poolptr + 1 ;
    } 
    {
      strpool [poolptr ]= c ;
      incr ( poolptr ) ;
    } 
    Result = true ;
  } 
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
endname ( void ) 
#else
endname ( ) 
#endif
{
  poolpointer a, n, e  ;
  e = poolptr - strstart [strptr ];
  if ( extdelimiter < 0 ) 
  extdelimiter = e ;
  a = areadelimiter + 1 ;
  n = extdelimiter - a ;
  e = e - extdelimiter ;
  if ( a == 0 ) 
  curarea = 284 ;
  else {
      
    curarea = makestring () ;
    choplaststring ( strstart [curarea ]+ a ) ;
  } 
  if ( n == 0 ) 
  curname = 284 ;
  else {
      
    curname = makestring () ;
    choplaststring ( strstart [curname ]+ n ) ;
  } 
  if ( e == 0 ) 
  curext = 284 ;
  else curext = makestring () ;
} 
void 
#ifdef HAVE_PROTOTYPES
zpackfilename ( strnumber n , strnumber a , strnumber e ) 
#else
zpackfilename ( n , a , e ) 
  strnumber n ;
  strnumber a ;
  strnumber e ;
#endif
{
  integer k  ;
  ASCIIcode c  ;
  poolpointer j  ;
  k = 0 ;
  if ( (char*) nameoffile ) 
  libcfree ( (char*) nameoffile ) ;
  nameoffile = xmalloc ( 1 + ( strstart [nextstr [a ]]- strstart [a ]) + 
  ( strstart [nextstr [n ]]- strstart [n ]) + ( strstart [nextstr [e ]
  ]- strstart [e ]) + 1 ) ;
  {register integer for_end; j = strstart [a ];for_end = strstart [
  nextstr [a ]]- 1 ; if ( j <= for_end) do 
    {
      c = strpool [j ];
      incr ( k ) ;
      if ( k <= maxint ) 
      nameoffile [k ]= xchr [c ];
    } 
  while ( j++ < for_end ) ;} 
  {register integer for_end; j = strstart [n ];for_end = strstart [
  nextstr [n ]]- 1 ; if ( j <= for_end) do 
    {
      c = strpool [j ];
      incr ( k ) ;
      if ( k <= maxint ) 
      nameoffile [k ]= xchr [c ];
    } 
  while ( j++ < for_end ) ;} 
  {register integer for_end; j = strstart [e ];for_end = strstart [
  nextstr [e ]]- 1 ; if ( j <= for_end) do 
    {
      c = strpool [j ];
      incr ( k ) ;
      if ( k <= maxint ) 
      nameoffile [k ]= xchr [c ];
    } 
  while ( j++ < for_end ) ;} 
  if ( k <= maxint ) 
  namelength = k ;
  else namelength = maxint ;
  nameoffile [namelength + 1 ]= 0 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zstrscanfile ( strnumber s ) 
#else
zstrscanfile ( s ) 
  strnumber s ;
#endif
{
  /* 30 */ poolpointer p, q  ;
  beginname () ;
  p = strstart [s ];
  q = strstart [nextstr [s ]];
  while ( p < q ) {
      
    if ( ! morename ( strpool [p ]) ) 
    goto lab30 ;
    incr ( p ) ;
  } 
  lab30: endname () ;
} 
fontnumber 
#ifdef HAVE_PROTOTYPES
zreadfontinfo ( strnumber fname ) 
#else
zreadfontinfo ( fname ) 
  strnumber fname ;
#endif
{
  /* 11 30 */ register fontnumber Result; boolean fileopened  ;
  fontnumber n  ;
  halfword lf, lh, bc, ec, nw, nh, nd  ;
  integer whdsize  ;
  integer i, ii  ;
  integer jj  ;
  scaled z  ;
  fraction d  ;
  eightbits handd  ;
  n = 0 ;
  fileopened = false ;
  strscanfile ( fname ) ;
  if ( curext == 284 ) 
  curext = 1092 ;
  packfilename ( curname , curarea , curext ) ;
  if ( ! bopenin ( tfminfile ) ) 
  goto lab11 ;
  fileopened = true ;
  {
    lf = tfmtemp ;
    if ( lf > 127 ) 
    goto lab11 ;
    tfmtemp = getc ( tfminfile ) ;
    lf = lf * 256 + tfmtemp ;
  } 
  tfmtemp = getc ( tfminfile ) ;
  {
    lh = tfmtemp ;
    if ( lh > 127 ) 
    goto lab11 ;
    tfmtemp = getc ( tfminfile ) ;
    lh = lh * 256 + tfmtemp ;
  } 
  tfmtemp = getc ( tfminfile ) ;
  {
    bc = tfmtemp ;
    if ( bc > 127 ) 
    goto lab11 ;
    tfmtemp = getc ( tfminfile ) ;
    bc = bc * 256 + tfmtemp ;
  } 
  tfmtemp = getc ( tfminfile ) ;
  {
    ec = tfmtemp ;
    if ( ec > 127 ) 
    goto lab11 ;
    tfmtemp = getc ( tfminfile ) ;
    ec = ec * 256 + tfmtemp ;
  } 
  if ( ( bc > 1 + ec ) || ( ec > 255 ) ) 
  goto lab11 ;
  tfmtemp = getc ( tfminfile ) ;
  {
    nw = tfmtemp ;
    if ( nw > 127 ) 
    goto lab11 ;
    tfmtemp = getc ( tfminfile ) ;
    nw = nw * 256 + tfmtemp ;
  } 
  tfmtemp = getc ( tfminfile ) ;
  {
    nh = tfmtemp ;
    if ( nh > 127 ) 
    goto lab11 ;
    tfmtemp = getc ( tfminfile ) ;
    nh = nh * 256 + tfmtemp ;
  } 
  tfmtemp = getc ( tfminfile ) ;
  {
    nd = tfmtemp ;
    if ( nd > 127 ) 
    goto lab11 ;
    tfmtemp = getc ( tfminfile ) ;
    nd = nd * 256 + tfmtemp ;
  } 
  whdsize = ( ec + 1 - bc ) + nw + nh + nd ;
  if ( lf < 6 + lh + whdsize ) 
  goto lab11 ;
  {register integer for_end; jj = 10 ;for_end = 1 ; if ( jj >= for_end) do 
    tfmtemp = getc ( tfminfile ) ;
  while ( jj-- > for_end ) ;} 
  if ( nextfmem < bc + 0 ) 
  nextfmem = bc + 0 ;
  if ( ( lastfnum == fontmax ) || ( nextfmem + whdsize >= fontmemsize ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 1100 ) ;
    } 
    print ( fname ) ;
    print ( 1107 ) ;
    {
      helpptr = 3 ;
      helpline [2 ]= 1108 ;
      helpline [1 ]= 1109 ;
      helpline [0 ]= 1110 ;
    } 
    error () ;
    goto lab30 ;
  } 
  incr ( lastfnum ) ;
  n = lastfnum ;
  fontbc [n ]= bc ;
  fontec [n ]= ec ;
  charbase [n ]= nextfmem - bc - 0 ;
  widthbase [n ]= nextfmem + ec - bc + 1 ;
  heightbase [n ]= widthbase [n ]+ 0 + nw ;
  depthbase [n ]= heightbase [n ]+ nh ;
  nextfmem = nextfmem + whdsize ;
  if ( lh < 2 ) 
  goto lab11 ;
  {register integer for_end; jj = 4 ;for_end = 1 ; if ( jj >= for_end) do 
    tfmtemp = getc ( tfminfile ) ;
  while ( jj-- > for_end ) ;} 
  tfmtemp = getc ( tfminfile ) ;
  {
    z = tfmtemp ;
    if ( z > 127 ) 
    goto lab11 ;
    tfmtemp = getc ( tfminfile ) ;
    z = z * 256 + tfmtemp ;
  } 
  tfmtemp = getc ( tfminfile ) ;
  z = z * 256 + tfmtemp ;
  tfmtemp = getc ( tfminfile ) ;
  z = z * 256 + tfmtemp ;
  fontdsize [n ]= takefraction ( z , 267432584L ) ;
  {register integer for_end; jj = 4 * ( lh - 2 ) ;for_end = 1 ; if ( jj >= 
  for_end) do 
    tfmtemp = getc ( tfminfile ) ;
  while ( jj-- > for_end ) ;} 
  ii = widthbase [n ]+ 0 ;
  i = charbase [n ]+ 0 + bc ;
  while ( i < ii ) {
      
    tfmtemp = getc ( tfminfile ) ;
    fontinfo [i ].qqqq .b0 = tfmtemp ;
    tfmtemp = getc ( tfminfile ) ;
    handd = tfmtemp ;
    fontinfo [i ].qqqq .b1 = handd / 16 ;
    fontinfo [i ].qqqq .b2 = handd % 16 ;
    tfmtemp = getc ( tfminfile ) ;
    tfmtemp = getc ( tfminfile ) ;
    incr ( i ) ;
  } 
  while ( i < nextfmem ) {
      
    tfmtemp = getc ( tfminfile ) ;
    d = tfmtemp ;
    if ( d >= 128 ) 
    d = d - 256 ;
    tfmtemp = getc ( tfminfile ) ;
    d = d * 256 + tfmtemp ;
    tfmtemp = getc ( tfminfile ) ;
    d = d * 256 + tfmtemp ;
    tfmtemp = getc ( tfminfile ) ;
    d = d * 256 + tfmtemp ;
    fontinfo [i ].cint = takefraction ( d * 16 , fontdsize [n ]) ;
    incr ( i ) ;
  } 
  if ( feof ( tfminfile ) ) 
  goto lab11 ;
  goto lab30 ;
  lab11: {
      
    if ( interaction == 3 ) 
    ;
    printnl ( 262 ) ;
    print ( 1100 ) ;
  } 
  print ( fname ) ;
  if ( fileopened ) 
  print ( 1101 ) ;
  else print ( 1102 ) ;
  {
    helpptr = 3 ;
    helpline [2 ]= 1103 ;
    helpline [1 ]= 1104 ;
    helpline [0 ]= 1105 ;
  } 
  if ( fileopened ) 
  helpline [0 ]= 1106 ;
  error () ;
  lab30: if ( fileopened ) 
  bclose ( tfminfile ) ;
  if ( n != 0 ) 
  {
    fontpsname [n ]= fname ;
    fontname [n ]= fname ;
    strref [fname ]= 127 ;
  } 
  Result = n ;
  return Result ;
} 
fontnumber 
#ifdef HAVE_PROTOTYPES
zfindfont ( strnumber f ) 
#else
zfindfont ( f ) 
  strnumber f ;
#endif
{
  /* 10 40 */ register fontnumber Result; fontnumber n  ;
  {register integer for_end; n = 0 ;for_end = lastfnum ; if ( n <= for_end) 
  do 
    if ( strvsstr ( f , fontname [n ]) == 0 ) 
    goto lab40 ;
  while ( n++ < for_end ) ;} 
  Result = readfontinfo ( f ) ;
  goto lab10 ;
  lab40: Result = n ;
  lab10: ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zlostwarning ( fontnumber f , poolpointer k ) 
#else
zlostwarning ( f , k ) 
  fontnumber f ;
  poolpointer k ;
#endif
{
  if ( internal [11 ]> 0 ) 
  {
    begindiagnostic () ;
    printnl ( 1111 ) ;
    print ( strpool [k ]) ;
    print ( 1112 ) ;
    print ( fontname [f ]) ;
    printchar ( 33 ) ;
    enddiagnostic ( false ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zsettextbox ( halfword p ) 
#else
zsettextbox ( p ) 
  halfword p ;
#endif
{
  fontnumber f  ;
  poolASCIIcode bc, ec  ;
  poolpointer k, kk  ;
  fourquarters cc  ;
  scaled h, d  ;
  mem [p + 5 ].cint = 0 ;
  mem [p + 6 ].cint = -2147483647L ;
  mem [p + 7 ].cint = -2147483647L ;
  f = mem [p + 1 ].hhfield .lhfield ;
  bc = fontbc [f ];
  ec = fontec [f ];
  kk = strstart [nextstr [mem [p + 1 ].hhfield .v.RH ]];
  k = strstart [mem [p + 1 ].hhfield .v.RH ];
  while ( k < kk ) {
      
    if ( ( strpool [k ]< bc ) || ( strpool [k ]> ec ) ) 
    lostwarning ( f , k ) ;
    else {
	
      cc = fontinfo [charbase [f ]+ strpool [k ]].qqqq ;
      if ( ! ( cc .b0 > 0 ) ) 
      lostwarning ( f , k ) ;
      else {
	  
	mem [p + 5 ].cint = mem [p + 5 ].cint + fontinfo [widthbase [f ]
	+ cc .b0 ].cint ;
	h = fontinfo [heightbase [f ]+ cc .b1 ].cint ;
	d = fontinfo [depthbase [f ]+ cc .b2 ].cint ;
	if ( h > mem [p + 6 ].cint ) 
	mem [p + 6 ].cint = h ;
	if ( d > mem [p + 7 ].cint ) 
	mem [p + 7 ].cint = d ;
      } 
    } 
    incr ( k ) ;
  } 
  if ( mem [p + 6 ].cint < - (integer) mem [p + 7 ].cint ) 
  {
    mem [p + 6 ].cint = 0 ;
    mem [p + 7 ].cint = 0 ;
  } 
} 
halfword 
#ifdef HAVE_PROTOTYPES
znewtextnode ( strnumber f , strnumber s ) 
#else
znewtextnode ( f , s ) 
  strnumber f ;
  strnumber s ;
#endif
{
  register halfword Result; halfword t  ;
  t = getnode ( 14 ) ;
  mem [t ].hhfield .b0 = 3 ;
  mem [t + 1 ].hhfield .v.RH = s ;
  mem [t + 1 ].hhfield .lhfield = findfont ( f ) ;
  mem [t + 2 ].cint = 0 ;
  mem [t + 3 ].cint = 0 ;
  mem [t + 4 ].cint = 0 ;
  mem [t + 8 ].cint = 0 ;
  mem [t + 9 ].cint = 0 ;
  mem [t + 10 ].cint = 65536L ;
  mem [t + 11 ].cint = 0 ;
  mem [t + 12 ].cint = 0 ;
  mem [t + 13 ].cint = 65536L ;
  settextbox ( t ) ;
  Result = t ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
znewboundsnode ( halfword p , smallnumber c ) 
#else
znewboundsnode ( p , c ) 
  halfword p ;
  smallnumber c ;
#endif
{
  register halfword Result; halfword t  ;
  t = getnode ( grobjectsize [c ]) ;
  mem [t ].hhfield .b0 = c ;
  mem [t + 1 ].hhfield .v.RH = p ;
  Result = t ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zinitbbox ( halfword h ) 
#else
zinitbbox ( h ) 
  halfword h ;
#endif
{
  mem [h + 6 ].hhfield .v.RH = h + 7 ;
  mem [h + 6 ].hhfield .lhfield = 0 ;
  mem [h + 2 ].cint = 2147483647L ;
  mem [h + 3 ].cint = 2147483647L ;
  mem [h + 4 ].cint = -2147483647L ;
  mem [h + 5 ].cint = -2147483647L ;
} 
void 
#ifdef HAVE_PROTOTYPES
zinitedges ( halfword h ) 
#else
zinitedges ( h ) 
  halfword h ;
#endif
{
  mem [h ].hhfield .v.RH = 2 ;
  mem [h + 7 ].hhfield .lhfield = h + 7 ;
  mem [h + 7 ].hhfield .v.RH = 0 ;
  mem [h ].hhfield .lhfield = 0 ;
  initbbox ( h ) ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zcopyobjects ( halfword p , halfword q ) 
#else
zcopyobjects ( p , q ) 
  halfword p ;
  halfword q ;
#endif
{
  register halfword Result; halfword hh  ;
  halfword pp  ;
  smallnumber k  ;
  hh = getnode ( 8 ) ;
  mem [hh ].hhfield .v.RH = 2 ;
  mem [hh ].hhfield .lhfield = 0 ;
  pp = hh + 7 ;
  while ( ( p != q ) ) {
      
    k = grobjectsize [mem [p ].hhfield .b0 ];
    mem [pp ].hhfield .v.RH = getnode ( k ) ;
    pp = mem [pp ].hhfield .v.RH ;
    while ( ( k > 0 ) ) {
	
      decr ( k ) ;
      mem [pp + k ]= mem [p + k ];
    } 
    switch ( mem [p ].hhfield .b0 ) 
    {case 4 : 
    case 5 : 
      mem [pp + 1 ].hhfield .v.RH = copypath ( mem [p + 1 ].hhfield .v.RH 
      ) ;
      break ;
    case 1 : 
      {
	mem [pp + 1 ].hhfield .v.RH = copypath ( mem [p + 1 ].hhfield 
	.v.RH ) ;
	if ( mem [p + 1 ].hhfield .lhfield != 0 ) 
	mem [pp + 1 ].hhfield .lhfield = makepen ( copypath ( mem [p + 1 ]
	.hhfield .lhfield ) , false ) ;
      } 
      break ;
    case 2 : 
      {
	mem [pp + 1 ].hhfield .v.RH = copypath ( mem [p + 1 ].hhfield 
	.v.RH ) ;
	mem [pp + 1 ].hhfield .lhfield = makepen ( copypath ( mem [p + 1 ]
	.hhfield .lhfield ) , false ) ;
	if ( mem [p + 6 ].hhfield .v.RH != 0 ) 
	incr ( mem [mem [pp + 6 ].hhfield .v.RH ].hhfield .lhfield ) ;
      } 
      break ;
    case 3 : 
      {
	if ( strref [mem [pp + 1 ].hhfield .v.RH ]< 127 ) 
	incr ( strref [mem [pp + 1 ].hhfield .v.RH ]) ;
      } 
      break ;
    case 6 : 
    case 7 : 
      ;
      break ;
    } 
    p = mem [p ].hhfield .v.RH ;
  } 
  mem [hh + 7 ].hhfield .lhfield = pp ;
  mem [pp ].hhfield .v.RH = 0 ;
  Result = hh ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zprivateedges ( halfword h ) 
#else
zprivateedges ( h ) 
  halfword h ;
#endif
{
  register halfword Result; halfword hh  ;
  halfword p, pp  ;
  if ( mem [h ].hhfield .lhfield == 0 ) 
  Result = h ;
  else {
      
    decr ( mem [h ].hhfield .lhfield ) ;
    hh = copyobjects ( mem [h + 7 ].hhfield .v.RH , 0 ) ;
    pp = hh ;
    p = mem [h ].hhfield .v.RH ;
    while ( ( p != 2 ) ) {
	
      mem [pp ].hhfield .v.RH = getnode ( 3 ) ;
      pp = mem [pp ].hhfield .v.RH ;
      mem [pp + 1 ].cint = mem [p + 1 ].cint ;
      mem [pp + 2 ].cint = mem [p + 2 ].cint ;
      p = mem [p ].hhfield .v.RH ;
    } 
    mem [pp ].hhfield .v.RH = 2 ;
    mem [hh + 1 ].cint = mem [h + 1 ].cint ;
    mem [hh + 2 ].cint = mem [h + 2 ].cint ;
    mem [hh + 3 ].cint = mem [h + 3 ].cint ;
    mem [hh + 4 ].cint = mem [h + 4 ].cint ;
    mem [hh + 5 ].cint = mem [h + 5 ].cint ;
    mem [hh + 6 ].hhfield .lhfield = mem [h + 6 ].hhfield .lhfield ;
    p = h + 7 ;
    pp = hh + 7 ;
    while ( ( p != mem [h + 6 ].hhfield .v.RH ) ) {
	
      if ( p == 0 ) 
      confusion ( 551 ) ;
      p = mem [p ].hhfield .v.RH ;
      pp = mem [pp ].hhfield .v.RH ;
    } 
    mem [hh + 6 ].hhfield .v.RH = pp ;
    Result = hh ;
  } 
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zskip1component ( halfword p ) 
#else
zskip1component ( p ) 
  halfword p ;
#endif
{
  register halfword Result; integer lev  ;
  lev = 0 ;
  do {
      if ( ( mem [p ].hhfield .b0 >= 4 ) ) 
    if ( ( mem [p ].hhfield .b0 >= 6 ) ) 
    decr ( lev ) ;
    else incr ( lev ) ;
    p = mem [p ].hhfield .v.RH ;
  } while ( ! ( lev == 0 ) ) ;
  Result = p ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
xretraceerror ( void ) 
#else
xretraceerror ( ) 
#endif
{
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 262 ) ;
    print ( 580 ) ;
  } 
  {
    helpptr = 3 ;
    helpline [2 ]= 584 ;
    helpline [1 ]= 585 ;
    helpline [0 ]= 583 ;
  } 
  putgeterror () ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zmakedashes ( halfword h ) 
#else
zmakedashes ( h ) 
  halfword h ;
#endif
{
  /* 10 40 45 */ register halfword Result; halfword p  ;
  scaled y0  ;
  halfword p0  ;
  halfword pp, qq, rr  ;
  halfword d, dd  ;
  scaled x0, x1, x2, x3  ;
  halfword dln  ;
  halfword hh  ;
  scaled hsf  ;
  halfword ds  ;
  scaled xoff  ;
  if ( mem [h ].hhfield .v.RH != 2 ) 
  goto lab40 ;
  p0 = 0 ;
  p = mem [h + 7 ].hhfield .v.RH ;
  while ( p != 0 ) {
      
    if ( mem [p ].hhfield .b0 != 2 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 580 ) ;
      } 
      {
	helpptr = 3 ;
	helpline [2 ]= 581 ;
	helpline [1 ]= 582 ;
	helpline [0 ]= 583 ;
      } 
      putgeterror () ;
      goto lab45 ;
    } 
    pp = mem [p + 1 ].hhfield .v.RH ;
    if ( p0 == 0 ) 
    {
      p0 = p ;
      y0 = mem [pp + 2 ].cint ;
    } 
    if ( ( mem [p + 2 ].cint != mem [p0 + 2 ].cint ) || ( mem [p + 3 ]
    .cint != mem [p0 + 3 ].cint ) || ( mem [p + 4 ].cint != mem [p0 + 4 ]
    .cint ) ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 580 ) ;
      } 
      {
	helpptr = 3 ;
	helpline [2 ]= 586 ;
	helpline [1 ]= 587 ;
	helpline [0 ]= 583 ;
      } 
      putgeterror () ;
      goto lab45 ;
    } 
    rr = pp ;
    if ( mem [pp ].hhfield .v.RH != pp ) 
    do {
	qq = rr ;
      rr = mem [rr ].hhfield .v.RH ;
      x0 = mem [qq + 1 ].cint ;
      x1 = mem [qq + 5 ].cint ;
      x2 = mem [rr + 3 ].cint ;
      x3 = mem [rr + 1 ].cint ;
      if ( ( x0 > x1 ) || ( x1 > x2 ) || ( x2 > x3 ) ) 
      if ( ( x0 < x1 ) || ( x1 < x2 ) || ( x2 < x3 ) ) 
      if ( abvscd ( x2 - x1 , x2 - x1 , x1 - x0 , x3 - x2 ) > 0 ) 
      {
	xretraceerror () ;
	goto lab45 ;
      } 
      if ( ( mem [pp + 1 ].cint > x0 ) || ( x0 > x3 ) ) 
      if ( ( mem [pp + 1 ].cint < x0 ) || ( x0 < x3 ) ) 
      {
	xretraceerror () ;
	goto lab45 ;
      } 
    } while ( ! ( mem [rr ].hhfield .b1 == 0 ) ) ;
    d = getnode ( 3 ) ;
    if ( mem [p + 6 ].hhfield .v.RH == 0 ) 
    mem [d ].hhfield .lhfield = 0 ;
    else mem [d ].hhfield .lhfield = p ;
    if ( mem [pp + 1 ].cint < mem [rr + 1 ].cint ) 
    {
      mem [d + 1 ].cint = mem [pp + 1 ].cint ;
      mem [d + 2 ].cint = mem [rr + 1 ].cint ;
    } 
    else {
	
      mem [d + 1 ].cint = mem [rr + 1 ].cint ;
      mem [d + 2 ].cint = mem [pp + 1 ].cint ;
    } 
    mem [3 ].cint = mem [d + 2 ].cint ;
    dd = h ;
    while ( mem [mem [dd ].hhfield .v.RH + 1 ].cint < mem [d + 2 ].cint 
    ) dd = mem [dd ].hhfield .v.RH ;
    if ( dd != h ) 
    if ( ( mem [dd + 2 ].cint > mem [d + 1 ].cint ) ) 
    {
      xretraceerror () ;
      goto lab45 ;
    } 
    mem [d ].hhfield .v.RH = mem [dd ].hhfield .v.RH ;
    mem [dd ].hhfield .v.RH = d ;
    p = mem [p ].hhfield .v.RH ;
  } 
  if ( mem [h ].hhfield .v.RH == 2 ) 
  goto lab45 ;
  d = h ;
  while ( mem [d ].hhfield .v.RH != 2 ) {
      
    ds = mem [mem [d ].hhfield .v.RH ].hhfield .lhfield ;
    if ( ds == 0 ) 
    d = mem [d ].hhfield .v.RH ;
    else {
	
      hh = mem [ds + 6 ].hhfield .v.RH ;
      hsf = mem [ds + 7 ].cint ;
      if ( ( hh == 0 ) ) 
      confusion ( 588 ) ;
      if ( mem [hh + 1 ].cint == 0 ) 
      d = mem [d ].hhfield .v.RH ;
      else {
	  
	if ( mem [hh ].hhfield .v.RH == 0 ) 
	confusion ( 588 ) ;
	dln = mem [d ].hhfield .v.RH ;
	dd = mem [hh ].hhfield .v.RH ;
	xoff = mem [dln + 1 ].cint - takescaled ( hsf , mem [dd + 1 ].cint 
	) - takescaled ( hsf , dashoffset ( hh ) ) ;
	mem [3 ].cint = takescaled ( hsf , mem [dd + 1 ].cint ) + 
	takescaled ( hsf , mem [hh + 1 ].cint ) ;
	mem [4 ].cint = mem [3 ].cint ;
	while ( xoff + takescaled ( hsf , mem [dd + 2 ].cint ) < mem [dln + 
	1 ].cint ) dd = mem [dd ].hhfield .v.RH ;
	while ( mem [dln + 1 ].cint <= mem [dln + 2 ].cint ) {
	    
	  if ( dd == 2 ) 
	  {
	    dd = mem [hh ].hhfield .v.RH ;
	    xoff = xoff + takescaled ( hsf , mem [hh + 1 ].cint ) ;
	  } 
	  if ( xoff + takescaled ( hsf , mem [dd + 1 ].cint ) <= mem [dln + 
	  2 ].cint ) 
	  {
	    mem [d ].hhfield .v.RH = getnode ( 3 ) ;
	    d = mem [d ].hhfield .v.RH ;
	    mem [d ].hhfield .v.RH = dln ;
	    if ( mem [dln + 1 ].cint > xoff + takescaled ( hsf , mem [dd + 
	    1 ].cint ) ) 
	    mem [d + 1 ].cint = mem [dln + 1 ].cint ;
	    else mem [d + 1 ].cint = xoff + takescaled ( hsf , mem [dd + 1 
	    ].cint ) ;
	    if ( mem [dln + 2 ].cint < xoff + takescaled ( hsf , mem [dd + 
	    2 ].cint ) ) 
	    mem [d + 2 ].cint = mem [dln + 2 ].cint ;
	    else mem [d + 2 ].cint = xoff + takescaled ( hsf , mem [dd + 2 
	    ].cint ) ;
	  } 
	  dd = mem [dd ].hhfield .v.RH ;
	  mem [dln + 1 ].cint = xoff + takescaled ( hsf , mem [dd + 1 ]
	  .cint ) ;
	} 
	mem [d ].hhfield .v.RH = mem [dln ].hhfield .v.RH ;
	freenode ( dln , 3 ) ;
      } 
    } 
  } 
  d = mem [h ].hhfield .v.RH ;
  while ( ( mem [d ].hhfield .v.RH != 2 ) ) d = mem [d ].hhfield .v.RH ;
  dd = mem [h ].hhfield .v.RH ;
  mem [h + 1 ].cint = mem [d + 2 ].cint - mem [dd + 1 ].cint ;
  if ( abs ( y0 ) > mem [h + 1 ].cint ) 
  mem [h + 1 ].cint = abs ( y0 ) ;
  else if ( d != dd ) 
  {
    mem [h ].hhfield .v.RH = mem [dd ].hhfield .v.RH ;
    mem [d + 2 ].cint = mem [dd + 2 ].cint + mem [h + 1 ].cint ;
    freenode ( dd , 3 ) ;
  } 
  lab40: Result = h ;
  goto lab10 ;
  lab45: flushdashlist ( h ) ;
  if ( mem [h ].hhfield .lhfield == 0 ) 
  tossedges ( h ) ;
  else decr ( mem [h ].hhfield .lhfield ) ;
  Result = 0 ;
  lab10: ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zadjustbbox ( halfword h ) 
#else
zadjustbbox ( h ) 
  halfword h ;
#endif
{
  if ( bbmin [0 ]< mem [h + 2 ].cint ) 
  mem [h + 2 ].cint = bbmin [0 ];
  if ( bbmin [1 ]< mem [h + 3 ].cint ) 
  mem [h + 3 ].cint = bbmin [1 ];
  if ( bbmax [0 ]> mem [h + 4 ].cint ) 
  mem [h + 4 ].cint = bbmax [0 ];
  if ( bbmax [1 ]> mem [h + 5 ].cint ) 
  mem [h + 5 ].cint = bbmax [1 ];
} 
void 
#ifdef HAVE_PROTOTYPES
zboxends ( halfword p , halfword pp , halfword h ) 
#else
zboxends ( p , pp , h ) 
  halfword p ;
  halfword pp ;
  halfword h ;
#endif
{
  /* 10 */ halfword q  ;
  fraction dx, dy  ;
  scaled d  ;
  scaled z  ;
  scaled xx, yy  ;
  integer i  ;
  if ( mem [p ].hhfield .b1 != 0 ) 
  {
    q = mem [p ].hhfield .v.RH ;
    while ( true ) {
	
      if ( q == mem [p ].hhfield .v.RH ) 
      {
	dx = mem [p + 1 ].cint - mem [p + 5 ].cint ;
	dy = mem [p + 2 ].cint - mem [p + 6 ].cint ;
	if ( ( dx == 0 ) && ( dy == 0 ) ) 
	{
	  dx = mem [p + 1 ].cint - mem [q + 3 ].cint ;
	  dy = mem [p + 2 ].cint - mem [q + 4 ].cint ;
	} 
      } 
      else {
	  
	dx = mem [p + 1 ].cint - mem [p + 3 ].cint ;
	dy = mem [p + 2 ].cint - mem [p + 4 ].cint ;
	if ( ( dx == 0 ) && ( dy == 0 ) ) 
	{
	  dx = mem [p + 1 ].cint - mem [q + 5 ].cint ;
	  dy = mem [p + 2 ].cint - mem [q + 6 ].cint ;
	} 
      } 
      dx = mem [p + 1 ].cint - mem [q + 1 ].cint ;
      dy = mem [p + 2 ].cint - mem [q + 2 ].cint ;
      d = pythadd ( dx , dy ) ;
      if ( d > 0 ) 
      {
	dx = makefraction ( dx , d ) ;
	dy = makefraction ( dy , d ) ;
	findoffset ( - (integer) dy , dx , pp ) ;
	xx = curx ;
	yy = cury ;
	{register integer for_end; i = 1 ;for_end = 2 ; if ( i <= for_end) 
	do 
	  {
	    findoffset ( dx , dy , pp ) ;
	    d = takefraction ( xx - curx , dx ) + takefraction ( yy - cury , 
	    dy ) ;
	    if ( ( d < 0 ) && ( i == 1 ) || ( d > 0 ) && ( i == 2 ) ) 
	    confusion ( 589 ) ;
	    z = mem [p + 1 ].cint + curx + takefraction ( d , dx ) ;
	    if ( z < mem [h + 2 ].cint ) 
	    mem [h + 2 ].cint = z ;
	    if ( z > mem [h + 4 ].cint ) 
	    mem [h + 4 ].cint = z ;
	    z = mem [p + 2 ].cint + cury + takefraction ( d , dy ) ;
	    if ( z < mem [h + 3 ].cint ) 
	    mem [h + 3 ].cint = z ;
	    if ( z > mem [h + 5 ].cint ) 
	    mem [h + 5 ].cint = z ;
	    dx = - (integer) dx ;
	    dy = - (integer) dy ;
	  } 
	while ( i++ < for_end ) ;} 
      } 
      if ( mem [p ].hhfield .b1 == 0 ) 
      goto lab10 ;
      else do {
	  q = p ;
	p = mem [p ].hhfield .v.RH ;
      } while ( ! ( mem [p ].hhfield .b1 == 0 ) ) ;
    } 
  } 
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zsetbbox ( halfword h , boolean toplevel ) 
#else
zsetbbox ( h , toplevel ) 
  halfword h ;
  boolean toplevel ;
#endif
{
  /* 10 */ halfword p  ;
  scaled sminx, sminy, smaxx, smaxy  ;
  scaled x0, x1, y0, y1  ;
  integer lev  ;
  switch ( mem [h + 6 ].hhfield .lhfield ) 
  {case 0 : 
    ;
    break ;
  case 1 : 
    if ( internal [33 ]> 0 ) 
    initbbox ( h ) ;
    break ;
  case 2 : 
    if ( internal [33 ]<= 0 ) 
    initbbox ( h ) ;
    break ;
  } 
  while ( mem [mem [h + 6 ].hhfield .v.RH ].hhfield .v.RH != 0 ) {
      
    p = mem [mem [h + 6 ].hhfield .v.RH ].hhfield .v.RH ;
    mem [h + 6 ].hhfield .v.RH = p ;
    switch ( mem [p ].hhfield .b0 ) 
    {case 6 : 
      if ( toplevel ) 
      confusion ( 590 ) ;
      else goto lab10 ;
      break ;
    case 1 : 
      {
	pathbbox ( mem [p + 1 ].hhfield .v.RH ) ;
	adjustbbox ( h ) ;
      } 
      break ;
    case 5 : 
      if ( internal [33 ]> 0 ) 
      mem [h + 6 ].hhfield .lhfield = 2 ;
      else {
	  
	mem [h + 6 ].hhfield .lhfield = 1 ;
	pathbbox ( mem [p + 1 ].hhfield .v.RH ) ;
	adjustbbox ( h ) ;
	lev = 1 ;
	while ( lev != 0 ) {
	    
	  if ( mem [p ].hhfield .v.RH == 0 ) 
	  confusion ( 591 ) ;
	  p = mem [p ].hhfield .v.RH ;
	  if ( mem [p ].hhfield .b0 == 5 ) 
	  incr ( lev ) ;
	  else if ( mem [p ].hhfield .b0 == 7 ) 
	  decr ( lev ) ;
	} 
	mem [h + 6 ].hhfield .v.RH = p ;
      } 
      break ;
    case 7 : 
      if ( internal [33 ]<= 0 ) 
      confusion ( 591 ) ;
      break ;
    case 2 : 
      {
	pathbbox ( mem [p + 1 ].hhfield .v.RH ) ;
	x0 = bbmin [0 ];
	y0 = bbmin [1 ];
	x1 = bbmax [0 ];
	y1 = bbmax [1 ];
	penbbox ( mem [p + 1 ].hhfield .lhfield ) ;
	bbmin [0 ]= bbmin [0 ]+ x0 ;
	bbmin [1 ]= bbmin [1 ]+ y0 ;
	bbmax [0 ]= bbmax [0 ]+ x1 ;
	bbmax [1 ]= bbmax [1 ]+ y1 ;
	adjustbbox ( h ) ;
	if ( ( mem [mem [p + 1 ].hhfield .v.RH ].hhfield .b0 == 0 ) && ( 
	mem [p + 6 ].hhfield .b0 == 2 ) ) 
	boxends ( mem [p + 1 ].hhfield .v.RH , mem [p + 1 ].hhfield 
	.lhfield , h ) ;
      } 
      break ;
    case 3 : 
      {
	x1 = takescaled ( mem [p + 10 ].cint , mem [p + 5 ].cint ) ;
	y0 = takescaled ( mem [p + 11 ].cint , - (integer) mem [p + 7 ]
	.cint ) ;
	y1 = takescaled ( mem [p + 11 ].cint , mem [p + 6 ].cint ) ;
	bbmin [0 ]= mem [p + 8 ].cint ;
	bbmax [0 ]= bbmin [0 ];
	if ( y0 < y1 ) 
	{
	  bbmin [0 ]= bbmin [0 ]+ y0 ;
	  bbmax [0 ]= bbmax [0 ]+ y1 ;
	} 
	else {
	    
	  bbmin [0 ]= bbmin [0 ]+ y1 ;
	  bbmax [0 ]= bbmax [0 ]+ y0 ;
	} 
	if ( x1 < 0 ) 
	bbmin [0 ]= bbmin [0 ]+ x1 ;
	else bbmax [0 ]= bbmax [0 ]+ x1 ;
	x1 = takescaled ( mem [p + 12 ].cint , mem [p + 5 ].cint ) ;
	y0 = takescaled ( mem [p + 13 ].cint , - (integer) mem [p + 7 ]
	.cint ) ;
	y1 = takescaled ( mem [p + 13 ].cint , mem [p + 6 ].cint ) ;
	bbmin [1 ]= mem [p + 9 ].cint ;
	bbmax [1 ]= bbmin [1 ];
	if ( y0 < y1 ) 
	{
	  bbmin [1 ]= bbmin [1 ]+ y0 ;
	  bbmax [1 ]= bbmax [1 ]+ y1 ;
	} 
	else {
	    
	  bbmin [1 ]= bbmin [1 ]+ y1 ;
	  bbmax [1 ]= bbmax [1 ]+ y0 ;
	} 
	if ( x1 < 0 ) 
	bbmin [1 ]= bbmin [1 ]+ x1 ;
	else bbmax [1 ]= bbmax [1 ]+ x1 ;
	adjustbbox ( h ) ;
      } 
      break ;
    case 4 : 
      {
	pathbbox ( mem [p + 1 ].hhfield .v.RH ) ;
	x0 = bbmin [0 ];
	y0 = bbmin [1 ];
	x1 = bbmax [0 ];
	y1 = bbmax [1 ];
	sminx = mem [h + 2 ].cint ;
	sminy = mem [h + 3 ].cint ;
	smaxx = mem [h + 4 ].cint ;
	smaxy = mem [h + 5 ].cint ;
	mem [h + 2 ].cint = 2147483647L ;
	mem [h + 3 ].cint = 2147483647L ;
	mem [h + 4 ].cint = -2147483647L ;
	mem [h + 5 ].cint = -2147483647L ;
	setbbox ( h , false ) ;
	if ( mem [h + 2 ].cint < x0 ) 
	mem [h + 2 ].cint = x0 ;
	if ( mem [h + 3 ].cint < y0 ) 
	mem [h + 3 ].cint = y0 ;
	if ( mem [h + 4 ].cint > x1 ) 
	mem [h + 4 ].cint = x1 ;
	if ( mem [h + 5 ].cint > y1 ) 
	mem [h + 5 ].cint = y1 ;
	bbmin [0 ]= sminx ;
	bbmin [1 ]= sminy ;
	bbmax [0 ]= smaxx ;
	bbmax [1 ]= smaxy ;
	adjustbbox ( h ) ;
      } 
      break ;
    } 
  } 
  if ( ! toplevel ) 
  confusion ( 590 ) ;
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zsplitcubic ( halfword p , fraction t ) 
#else
zsplitcubic ( p , t ) 
  halfword p ;
  fraction t ;
#endif
{
  scaled v  ;
  halfword q, r  ;
  q = mem [p ].hhfield .v.RH ;
  r = getnode ( 7 ) ;
  mem [p ].hhfield .v.RH = r ;
  mem [r ].hhfield .v.RH = q ;
  mem [r ].hhfield .b0 = 1 ;
  mem [r ].hhfield .b1 = 1 ;
  v = mem [p + 5 ].cint - takefraction ( mem [p + 5 ].cint - mem [q + 3 ]
  .cint , t ) ;
  mem [p + 5 ].cint = mem [p + 1 ].cint - takefraction ( mem [p + 1 ]
  .cint - mem [p + 5 ].cint , t ) ;
  mem [q + 3 ].cint = mem [q + 3 ].cint - takefraction ( mem [q + 3 ]
  .cint - mem [q + 1 ].cint , t ) ;
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
  .cint - mem [q + 2 ].cint , t ) ;
  mem [r + 4 ].cint = mem [p + 6 ].cint - takefraction ( mem [p + 6 ]
  .cint - v , t ) ;
  mem [r + 6 ].cint = v - takefraction ( v - mem [q + 4 ].cint , t ) ;
  mem [r + 2 ].cint = mem [r + 4 ].cint - takefraction ( mem [r + 4 ]
  .cint - mem [r + 6 ].cint , t ) ;
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
  mem [p ].hhfield .v.RH = mem [q ].hhfield .v.RH ;
  mem [p + 5 ].cint = mem [q + 5 ].cint ;
  mem [p + 6 ].cint = mem [q + 6 ].cint ;
  freenode ( q , 7 ) ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zpenwalk ( halfword w , integer k ) 
#else
zpenwalk ( w , k ) 
  halfword w ;
  integer k ;
#endif
{
  register halfword Result; while ( k > 0 ) {
      
    w = mem [w ].hhfield .v.RH ;
    decr ( k ) ;
  } 
  while ( k < 0 ) {
      
    w = mem [w ].hhfield .lhfield ;
    incr ( k ) ;
  } 
  Result = w ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zfinoffsetprep ( halfword p , halfword w , integer x0 , integer x1 , integer 
x2 , integer y0 , integer y1 , integer y2 , integer rise , integer turnamt ) 
#else
zfinoffsetprep ( p , w , x0 , x1 , x2 , y0 , y1 , y2 , rise , turnamt ) 
  halfword p ;
  halfword w ;
  integer x0 ;
  integer x1 ;
  integer x2 ;
  integer y0 ;
  integer y1 ;
  integer y2 ;
  integer rise ;
  integer turnamt ;
#endif
{
  /* 10 */ halfword ww  ;
  scaled du, dv  ;
  integer t0, t1, t2  ;
  fraction t  ;
  fraction s  ;
  integer v  ;
  halfword q  ;
  q = mem [p ].hhfield .v.RH ;
  while ( true ) {
      
    if ( rise > 0 ) 
    ww = mem [w ].hhfield .v.RH ;
    else ww = mem [w ].hhfield .lhfield ;
    du = mem [ww + 1 ].cint - mem [w + 1 ].cint ;
    dv = mem [ww + 2 ].cint - mem [w + 2 ].cint ;
    if ( abs ( du ) >= abs ( dv ) ) 
    {
      s = makefraction ( dv , du ) ;
      t0 = takefraction ( x0 , s ) - y0 ;
      t1 = takefraction ( x1 , s ) - y1 ;
      t2 = takefraction ( x2 , s ) - y2 ;
      if ( du < 0 ) 
      {
	t0 = - (integer) t0 ;
	t1 = - (integer) t1 ;
	t2 = - (integer) t2 ;
      } 
    } 
    else {
	
      s = makefraction ( du , dv ) ;
      t0 = x0 - takefraction ( y0 , s ) ;
      t1 = x1 - takefraction ( y1 , s ) ;
      t2 = x2 - takefraction ( y2 , s ) ;
      if ( dv < 0 ) 
      {
	t0 = - (integer) t0 ;
	t1 = - (integer) t1 ;
	t2 = - (integer) t2 ;
      } 
    } 
    if ( t0 < 0 ) 
    t0 = 0 ;
    t = crossingpoint ( t0 , t1 , t2 ) ;
    if ( t >= 268435456L ) 
    if ( turnamt > 0 ) 
    t = 268435456L ;
    else goto lab10 ;
    {
      splitcubic ( p , t ) ;
      p = mem [p ].hhfield .v.RH ;
      mem [p ].hhfield .lhfield = 16384 + rise ;
      decr ( turnamt ) ;
      v = x0 - takefraction ( x0 - x1 , t ) ;
      x1 = x1 - takefraction ( x1 - x2 , t ) ;
      x0 = v - takefraction ( v - x1 , t ) ;
      v = y0 - takefraction ( y0 - y1 , t ) ;
      y1 = y1 - takefraction ( y1 - y2 , t ) ;
      y0 = v - takefraction ( v - y1 , t ) ;
      if ( turnamt < 0 ) 
      {
	t1 = t1 - takefraction ( t1 - t2 , t ) ;
	if ( t1 > 0 ) 
	t1 = 0 ;
	t = crossingpoint ( 0 , - (integer) t1 , - (integer) t2 ) ;
	if ( t > 268435456L ) 
	t = 268435456L ;
	incr ( turnamt ) ;
	if ( ( t == 268435456L ) && ( mem [p ].hhfield .v.RH != q ) ) 
	mem [mem [p ].hhfield .v.RH ].hhfield .lhfield = mem [mem [p ]
	.hhfield .v.RH ].hhfield .lhfield - rise ;
	else {
	    
	  splitcubic ( p , t ) ;
	  mem [mem [p ].hhfield .v.RH ].hhfield .lhfield = 16384 - rise ;
	  v = x1 - takefraction ( x1 - x2 , t ) ;
	  x1 = x0 - takefraction ( x0 - x1 , t ) ;
	  x2 = x1 - takefraction ( x1 - v , t ) ;
	  v = y1 - takefraction ( y1 - y2 , t ) ;
	  y1 = y0 - takefraction ( y0 - y1 , t ) ;
	  y2 = y1 - takefraction ( y1 - v , t ) ;
	} 
      } 
    } 
    w = ww ;
  } 
  lab10: ;
} 
integer 
#ifdef HAVE_PROTOTYPES
zgetturnamt ( halfword w , scaled dx , scaled dy , boolean ccw ) 
#else
zgetturnamt ( w , dx , dy , ccw ) 
  halfword w ;
  scaled dx ;
  scaled dy ;
  boolean ccw ;
#endif
{
  /* 30 */ register integer Result; halfword ww  ;
  integer s  ;
  integer t  ;
  s = 0 ;
  if ( ccw ) 
  {
    ww = mem [w ].hhfield .v.RH ;
    do {
	t = abvscd ( dy , mem [ww + 1 ].cint - mem [w + 1 ].cint , dx , 
      mem [ww + 2 ].cint - mem [w + 2 ].cint ) ;
      if ( t < 0 ) 
      goto lab30 ;
      incr ( s ) ;
      w = ww ;
      ww = mem [ww ].hhfield .v.RH ;
    } while ( ! ( t <= 0 ) ) ;
    lab30: ;
  } 
  else {
      
    ww = mem [w ].hhfield .lhfield ;
    while ( abvscd ( dy , mem [w + 1 ].cint - mem [ww + 1 ].cint , dx , 
    mem [w + 2 ].cint - mem [ww + 2 ].cint ) < 0 ) {
	
      decr ( s ) ;
      w = ww ;
      ww = mem [ww ].hhfield .lhfield ;
    } 
  } 
  Result = s ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zoffsetprep ( halfword c , halfword h ) 
#else
zoffsetprep ( c , h ) 
  halfword c ;
  halfword h ;
#endif
{
  /* 45 */ register halfword Result; halfword n  ;
  halfword p, q, r, w, ww  ;
  integer kneeded  ;
  halfword w0  ;
  scaled dxin, dyin  ;
  integer turnamt  ;
  integer x0, x1, x2, y0, y1, y2  ;
  integer t0, t1, t2  ;
  integer du, dv, dx, dy  ;
  integer dx0, dy0  ;
  integer lmaxcoef  ;
  integer x0a, x1a, x2a, y0a, y1a, y2a  ;
  fraction t  ;
  fraction s  ;
  integer u0, u1, v0, v1  ;
  integer ss  ;
  schar dsign  ;
  n = 0 ;
  p = h ;
  do {
      incr ( n ) ;
    p = mem [p ].hhfield .v.RH ;
  } while ( ! ( p == h ) ) ;
  dxin = mem [mem [h ].hhfield .v.RH + 1 ].cint - mem [mem [h ].hhfield 
  .lhfield + 1 ].cint ;
  dyin = mem [mem [h ].hhfield .v.RH + 2 ].cint - mem [mem [h ].hhfield 
  .lhfield + 2 ].cint ;
  if ( ( dxin == 0 ) && ( dyin == 0 ) ) 
  {
    dxin = mem [mem [h ].hhfield .lhfield + 2 ].cint - mem [h + 2 ].cint 
    ;
    dyin = mem [h + 1 ].cint - mem [mem [h ].hhfield .lhfield + 1 ].cint 
    ;
  } 
  w0 = h ;
  p = c ;
  kneeded = 0 ;
  do {
      q = mem [p ].hhfield .v.RH ;
    mem [p ].hhfield .lhfield = 16384 + kneeded ;
    kneeded = 0 ;
    x0 = mem [p + 5 ].cint - mem [p + 1 ].cint ;
    x2 = mem [q + 1 ].cint - mem [q + 3 ].cint ;
    x1 = mem [q + 3 ].cint - mem [p + 5 ].cint ;
    y0 = mem [p + 6 ].cint - mem [p + 2 ].cint ;
    y2 = mem [q + 2 ].cint - mem [q + 4 ].cint ;
    y1 = mem [q + 4 ].cint - mem [p + 6 ].cint ;
    lmaxcoef = abs ( x0 ) ;
    if ( abs ( x1 ) > lmaxcoef ) 
    lmaxcoef = abs ( x1 ) ;
    if ( abs ( x2 ) > lmaxcoef ) 
    lmaxcoef = abs ( x2 ) ;
    if ( abs ( y0 ) > lmaxcoef ) 
    lmaxcoef = abs ( y0 ) ;
    if ( abs ( y1 ) > lmaxcoef ) 
    lmaxcoef = abs ( y1 ) ;
    if ( abs ( y2 ) > lmaxcoef ) 
    lmaxcoef = abs ( y2 ) ;
    if ( lmaxcoef == 0 ) 
    goto lab45 ;
    while ( lmaxcoef < 134217728L ) {
	
      lmaxcoef = lmaxcoef + lmaxcoef ;
      x0 = x0 + x0 ;
      x1 = x1 + x1 ;
      x2 = x2 + x2 ;
      y0 = y0 + y0 ;
      y1 = y1 + y1 ;
      y2 = y2 + y2 ;
    } 
    dx = x0 ;
    dy = y0 ;
    if ( dx == 0 ) 
    if ( dy == 0 ) 
    {
      dx = x1 ;
      dy = y1 ;
      if ( dx == 0 ) 
      if ( dy == 0 ) 
      {
	dx = x2 ;
	dy = y2 ;
      } 
    } 
    if ( p == c ) 
    {
      dx0 = dx ;
      dy0 = dy ;
    } 
    turnamt = getturnamt ( w0 , dx , dy , abvscd ( dy , dxin , dx , dyin ) >= 
    0 ) ;
    w = penwalk ( w0 , turnamt ) ;
    w0 = w ;
    mem [p ].hhfield .lhfield = mem [p ].hhfield .lhfield + turnamt ;
    dxin = x2 ;
    dyin = y2 ;
    if ( dxin == 0 ) 
    if ( dyin == 0 ) 
    {
      dxin = x1 ;
      dyin = y1 ;
      if ( dxin == 0 ) 
      if ( dyin == 0 ) 
      {
	dxin = x0 ;
	dyin = y0 ;
      } 
    } 
    dsign = abvscd ( dx , dyin , dxin , dy ) ;
    if ( dsign == 0 ) 
    if ( dx == 0 ) 
    if ( dy > 0 ) 
    dsign = 1 ;
    else dsign = -1 ;
    else if ( dx > 0 ) 
    dsign = 1 ;
    else dsign = -1 ;
    t0 = half ( takefraction ( x0 , y2 ) ) - half ( takefraction ( x2 , y0 ) ) 
    ;
    t1 = half ( takefraction ( x1 , y0 + y2 ) ) - half ( takefraction ( y1 , 
    x0 + x2 ) ) ;
    if ( t0 == 0 ) 
    t0 = dsign ;
    if ( t0 > 0 ) 
    {
      t = crossingpoint ( t0 , t1 , - (integer) t0 ) ;
      u0 = x0 - takefraction ( x0 - x1 , t ) ;
      u1 = x1 - takefraction ( x1 - x2 , t ) ;
      v0 = y0 - takefraction ( y0 - y1 , t ) ;
      v1 = y1 - takefraction ( y1 - y2 , t ) ;
    } 
    else {
	
      t = crossingpoint ( - (integer) t0 , t1 , t0 ) ;
      u0 = x2 - takefraction ( x2 - x1 , t ) ;
      u1 = x1 - takefraction ( x1 - x0 , t ) ;
      v0 = y2 - takefraction ( y2 - y1 , t ) ;
      v1 = y1 - takefraction ( y1 - y0 , t ) ;
    } 
    ss = takefraction ( x0 + x2 , u0 - takefraction ( u0 - u1 , t ) ) + 
    takefraction ( y0 + y2 , v0 - takefraction ( v0 - v1 , t ) ) ;
    turnamt = getturnamt ( w , dxin , dyin , dsign > 0 ) ;
    if ( ss < 0 ) 
    turnamt = turnamt - dsign * n ;
    ww = mem [w ].hhfield .lhfield ;
    du = mem [ww + 1 ].cint - mem [w + 1 ].cint ;
    dv = mem [ww + 2 ].cint - mem [w + 2 ].cint ;
    if ( abs ( du ) >= abs ( dv ) ) 
    {
      s = makefraction ( dv , du ) ;
      t0 = takefraction ( x0 , s ) - y0 ;
      t1 = takefraction ( x1 , s ) - y1 ;
      t2 = takefraction ( x2 , s ) - y2 ;
      if ( du < 0 ) 
      {
	t0 = - (integer) t0 ;
	t1 = - (integer) t1 ;
	t2 = - (integer) t2 ;
      } 
    } 
    else {
	
      s = makefraction ( du , dv ) ;
      t0 = x0 - takefraction ( y0 , s ) ;
      t1 = x1 - takefraction ( y1 , s ) ;
      t2 = x2 - takefraction ( y2 , s ) ;
      if ( dv < 0 ) 
      {
	t0 = - (integer) t0 ;
	t1 = - (integer) t1 ;
	t2 = - (integer) t2 ;
      } 
    } 
    if ( t0 < 0 ) 
    t0 = 0 ;
    t = crossingpoint ( t0 , t1 , t2 ) ;
    if ( turnamt >= 0 ) 
    if ( t2 < 0 ) 
    t = 268435457L ;
    else {
	
      u0 = x0 - takefraction ( x0 - x1 , t ) ;
      u1 = x1 - takefraction ( x1 - x2 , t ) ;
      ss = takefraction ( - (integer) du , u0 - takefraction ( u0 - u1 , t ) ) 
      ;
      v0 = y0 - takefraction ( y0 - y1 , t ) ;
      v1 = y1 - takefraction ( y1 - y2 , t ) ;
      ss = ss + takefraction ( - (integer) dv , v0 - takefraction ( v0 - v1 , 
      t ) ) ;
      if ( ss < 0 ) 
      t = 268435457L ;
    } 
    else if ( t > 268435456L ) 
    t = 268435456L ;
    if ( t > 268435456L ) 
    finoffsetprep ( p , w , x0 , x1 , x2 , y0 , y1 , y2 , 1 , turnamt ) ;
    else {
	
      splitcubic ( p , t ) ;
      r = mem [p ].hhfield .v.RH ;
      x1a = x0 - takefraction ( x0 - x1 , t ) ;
      x1 = x1 - takefraction ( x1 - x2 , t ) ;
      x2a = x1a - takefraction ( x1a - x1 , t ) ;
      y1a = y0 - takefraction ( y0 - y1 , t ) ;
      y1 = y1 - takefraction ( y1 - y2 , t ) ;
      y2a = y1a - takefraction ( y1a - y1 , t ) ;
      finoffsetprep ( p , w , x0 , x1a , x2a , y0 , y1a , y2a , 1 , 0 ) ;
      x0 = x2a ;
      y0 = y2a ;
      mem [r ].hhfield .lhfield = 16383 ;
      if ( turnamt >= 0 ) 
      {
	t1 = t1 - takefraction ( t1 - t2 , t ) ;
	if ( t1 > 0 ) 
	t1 = 0 ;
	t = crossingpoint ( 0 , - (integer) t1 , - (integer) t2 ) ;
	if ( t > 268435456L ) 
	t = 268435456L ;
	splitcubic ( r , t ) ;
	mem [mem [r ].hhfield .v.RH ].hhfield .lhfield = 16385 ;
	x1a = x1 - takefraction ( x1 - x2 , t ) ;
	x1 = x0 - takefraction ( x0 - x1 , t ) ;
	x0a = x1 - takefraction ( x1 - x1a , t ) ;
	y1a = y1 - takefraction ( y1 - y2 , t ) ;
	y1 = y0 - takefraction ( y0 - y1 , t ) ;
	y0a = y1 - takefraction ( y1 - y1a , t ) ;
	finoffsetprep ( mem [r ].hhfield .v.RH , w , x0a , x1a , x2 , y0a , 
	y1a , y2 , 1 , turnamt ) ;
	x2 = x0a ;
	y2 = y0a ;
	finoffsetprep ( r , ww , x0 , x1 , x2 , y0 , y1 , y2 , -1 , 0 ) ;
      } 
      else finoffsetprep ( r , ww , x0 , x1 , x2 , y0 , y1 , y2 , -1 , -1 - 
      turnamt ) ;
    } 
    w0 = penwalk ( w0 , turnamt ) ;
    lab45: ;
    do {
	r = mem [p ].hhfield .v.RH ;
      if ( mem [p + 1 ].cint == mem [p + 5 ].cint ) 
      if ( mem [p + 2 ].cint == mem [p + 6 ].cint ) 
      if ( mem [p + 1 ].cint == mem [r + 3 ].cint ) 
      if ( mem [p + 2 ].cint == mem [r + 4 ].cint ) 
      if ( mem [p + 1 ].cint == mem [r + 1 ].cint ) 
      if ( mem [p + 2 ].cint == mem [r + 2 ].cint ) 
      if ( r != p ) 
      {
	kneeded = mem [p ].hhfield .lhfield - 16384 ;
	if ( r == q ) 
	q = p ;
	else {
	    
	  mem [p ].hhfield .lhfield = kneeded + mem [r ].hhfield .lhfield 
	  ;
	  kneeded = 0 ;
	} 
	if ( r == c ) 
	{
	  mem [p ].hhfield .lhfield = mem [c ].hhfield .lhfield ;
	  c = p ;
	} 
	if ( r == specp1 ) 
	specp1 = p ;
	if ( r == specp2 ) 
	specp2 = p ;
	r = p ;
	removecubic ( p ) ;
      } 
      p = r ;
    } while ( ! ( p == q ) ) ;
  } while ( ! ( q == c ) ) ;
  specoffset = mem [c ].hhfield .lhfield - 16384 ;
  if ( mem [c ].hhfield .v.RH == c ) 
  mem [c ].hhfield .lhfield = 16384 + n ;
  else {
      
    mem [c ].hhfield .lhfield = mem [c ].hhfield .lhfield + kneeded ;
    while ( w0 != h ) {
	
      mem [c ].hhfield .lhfield = mem [c ].hhfield .lhfield + 1 ;
      w0 = mem [w0 ].hhfield .v.RH ;
    } 
    while ( mem [c ].hhfield .lhfield <= 16384 - n ) mem [c ].hhfield 
    .lhfield = mem [c ].hhfield .lhfield + n ;
    while ( mem [c ].hhfield .lhfield > 16384 ) mem [c ].hhfield .lhfield 
    = mem [c ].hhfield .lhfield - n ;
    if ( ( mem [c ].hhfield .lhfield != 16384 ) && ( abvscd ( dy0 , dxin , 
    dx0 , dyin ) >= 0 ) ) 
    mem [c ].hhfield .lhfield = mem [c ].hhfield .lhfield + n ;
  } 
  Result = c ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintspec ( halfword curspec , halfword curpen , strnumber s ) 
#else
zprintspec ( curspec , curpen , s ) 
  halfword curspec ;
  halfword curpen ;
  strnumber s ;
#endif
{
  halfword p, q  ;
  halfword w  ;
  printdiagnostic ( 592 , s , true ) ;
  p = curspec ;
  w = penwalk ( curpen , specoffset ) ;
  println () ;
  printtwo ( mem [curspec + 1 ].cint , mem [curspec + 2 ].cint ) ;
  print ( 593 ) ;
  printtwo ( mem [w + 1 ].cint , mem [w + 2 ].cint ) ;
  do {
      do { q = mem [p ].hhfield .v.RH ;
      {
	printnl ( 598 ) ;
	printtwo ( mem [p + 5 ].cint , mem [p + 6 ].cint ) ;
	print ( 537 ) ;
	printtwo ( mem [q + 3 ].cint , mem [q + 4 ].cint ) ;
	printnl ( 534 ) ;
	printtwo ( mem [q + 1 ].cint , mem [q + 2 ].cint ) ;
      } 
      p = q ;
    } while ( ! ( ( p == curspec ) || ( mem [p ].hhfield .lhfield != 16384 ) 
    ) ) ;
    if ( mem [p ].hhfield .lhfield != 16384 ) 
    {
      w = penwalk ( w , mem [p ].hhfield .lhfield - 16384 ) ;
      print ( 595 ) ;
      if ( mem [p ].hhfield .lhfield > 16384 ) 
      print ( 596 ) ;
      print ( 597 ) ;
      printtwo ( mem [w + 1 ].cint , mem [w + 2 ].cint ) ;
    } 
  } while ( ! ( p == curspec ) ) ;
  printnl ( 594 ) ;
  enddiagnostic ( true ) ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zinsertknot ( halfword q , scaled x , scaled y ) 
#else
zinsertknot ( q , x , y ) 
  halfword q ;
  scaled x ;
  scaled y ;
#endif
{
  register halfword Result; halfword r  ;
  r = getnode ( 7 ) ;
  mem [r ].hhfield .v.RH = mem [q ].hhfield .v.RH ;
  mem [q ].hhfield .v.RH = r ;
  mem [r + 5 ].cint = mem [q + 5 ].cint ;
  mem [r + 6 ].cint = mem [q + 6 ].cint ;
  mem [r + 1 ].cint = x ;
  mem [r + 2 ].cint = y ;
  mem [q + 5 ].cint = mem [q + 1 ].cint ;
  mem [q + 6 ].cint = mem [q + 2 ].cint ;
  mem [r + 3 ].cint = mem [r + 1 ].cint ;
  mem [r + 4 ].cint = mem [r + 2 ].cint ;
  mem [r ].hhfield .b0 = 1 ;
  mem [r ].hhfield .b1 = 1 ;
  Result = r ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zmakeenvelope ( halfword c , halfword h , smallnumber ljoin , smallnumber lcap 
, scaled miterlim ) 
#else
zmakeenvelope ( c , h , ljoin , lcap , miterlim ) 
  halfword c ;
  halfword h ;
  smallnumber ljoin ;
  smallnumber lcap ;
  scaled miterlim ;
#endif
{
  /* 30 */ register halfword Result; halfword p, q, r, q0  ;
  char jointype  ;
  halfword w, w0  ;
  scaled qx, qy  ;
  halfword k, k0  ;
  fraction dxin, dyin, dxout, dyout  ;
  scaled tmp  ;
  fraction det  ;
  fraction htx, hty  ;
  scaled maxht  ;
  halfword kk  ;
  halfword ww  ;
  specp1 = 0 ;
  specp2 = 0 ;
  if ( mem [c ].hhfield .b0 == 0 ) 
  {
    specp1 = htapypoc ( c ) ;
    specp2 = pathtail ;
    mem [specp2 ].hhfield .v.RH = mem [specp1 ].hhfield .v.RH ;
    mem [specp1 ].hhfield .v.RH = c ;
    removecubic ( specp1 ) ;
    c = specp1 ;
    if ( c != mem [c ].hhfield .v.RH ) 
    removecubic ( specp2 ) ;
    else {
	
      mem [c ].hhfield .b0 = 1 ;
      mem [c ].hhfield .b1 = 1 ;
      mem [c + 3 ].cint = mem [c + 1 ].cint ;
      mem [c + 4 ].cint = mem [c + 2 ].cint ;
      mem [c + 5 ].cint = mem [c + 1 ].cint ;
      mem [c + 6 ].cint = mem [c + 2 ].cint ;
    } 
  } 
  c = offsetprep ( c , h ) ;
  if ( internal [5 ]> 0 ) 
  printspec ( c , h , 284 ) ;
  h = penwalk ( h , specoffset ) ;
  w = h ;
  p = c ;
  do {
      q = mem [p ].hhfield .v.RH ;
    q0 = q ;
    qx = mem [q + 1 ].cint ;
    qy = mem [q + 2 ].cint ;
    k = mem [q ].hhfield .lhfield ;
    k0 = k ;
    w0 = w ;
    if ( k != 16384 ) 
    if ( k < 16384 ) 
    jointype = 2 ;
    else {
	
      if ( ( q != specp1 ) && ( q != specp2 ) ) 
      jointype = ljoin ;
      else if ( lcap == 2 ) 
      jointype = 3 ;
      else jointype = 2 - lcap ;
      if ( ( jointype == 0 ) || ( jointype == 3 ) ) 
      {
	dxin = mem [q + 1 ].cint - mem [q + 3 ].cint ;
	dyin = mem [q + 2 ].cint - mem [q + 4 ].cint ;
	if ( ( dxin == 0 ) && ( dyin == 0 ) ) 
	{
	  dxin = mem [q + 1 ].cint - mem [p + 5 ].cint ;
	  dyin = mem [q + 2 ].cint - mem [p + 6 ].cint ;
	  if ( ( dxin == 0 ) && ( dyin == 0 ) ) 
	  {
	    dxin = mem [q + 1 ].cint - mem [p + 1 ].cint ;
	    dyin = mem [q + 2 ].cint - mem [p + 2 ].cint ;
	    if ( p != c ) 
	    {
	      dxin = dxin + mem [w + 1 ].cint ;
	      dyin = dyin + mem [w + 2 ].cint ;
	    } 
	  } 
	} 
	tmp = pythadd ( dxin , dyin ) ;
	if ( tmp == 0 ) 
	jointype = 2 ;
	else {
	    
	  dxin = makefraction ( dxin , tmp ) ;
	  dyin = makefraction ( dyin , tmp ) ;
	  dxout = mem [q + 5 ].cint - mem [q + 1 ].cint ;
	  dyout = mem [q + 6 ].cint - mem [q + 2 ].cint ;
	  if ( ( dxout == 0 ) && ( dyout == 0 ) ) 
	  {
	    r = mem [q ].hhfield .v.RH ;
	    dxout = mem [r + 3 ].cint - mem [q + 1 ].cint ;
	    dyout = mem [r + 4 ].cint - mem [q + 2 ].cint ;
	    if ( ( dxout == 0 ) && ( dyout == 0 ) ) 
	    {
	      dxout = mem [r + 1 ].cint - mem [q + 1 ].cint ;
	      dyout = mem [r + 2 ].cint - mem [q + 2 ].cint ;
	    } 
	  } 
	  if ( q == c ) 
	  {
	    dxout = dxout - mem [h + 1 ].cint ;
	    dyout = dyout - mem [h + 2 ].cint ;
	  } 
	  tmp = pythadd ( dxout , dyout ) ;
	  if ( tmp == 0 ) 
	  confusion ( 599 ) ;
	  dxout = makefraction ( dxout , tmp ) ;
	  dyout = makefraction ( dyout , tmp ) ;
	} 
	if ( jointype == 0 ) 
	{
	  tmp = takefraction ( miterlim , 134217728L + half ( takefraction ( 
	  dxin , dxout ) + takefraction ( dyin , dyout ) ) ) ;
	  if ( tmp < 65536L ) 
	  if ( takescaled ( miterlim , tmp ) < 65536L ) 
	  jointype = 2 ;
	} 
      } 
    } 
    mem [p + 5 ].cint = mem [p + 5 ].cint + mem [w + 1 ].cint ;
    mem [p + 6 ].cint = mem [p + 6 ].cint + mem [w + 2 ].cint ;
    mem [q + 3 ].cint = mem [q + 3 ].cint + mem [w + 1 ].cint ;
    mem [q + 4 ].cint = mem [q + 4 ].cint + mem [w + 2 ].cint ;
    mem [q + 1 ].cint = mem [q + 1 ].cint + mem [w + 1 ].cint ;
    mem [q + 2 ].cint = mem [q + 2 ].cint + mem [w + 2 ].cint ;
    mem [q ].hhfield .b0 = 1 ;
    mem [q ].hhfield .b1 = 1 ;
    while ( k != 16384 ) {
	
      if ( k > 16384 ) 
      {
	w = mem [w ].hhfield .v.RH ;
	decr ( k ) ;
      } 
      else {
	  
	w = mem [w ].hhfield .lhfield ;
	incr ( k ) ;
      } 
      if ( ( jointype == 1 ) || ( k == 16384 ) ) 
      q = insertknot ( q , qx + mem [w + 1 ].cint , qy + mem [w + 2 ].cint 
      ) ;
    } 
    if ( q != mem [p ].hhfield .v.RH ) 
    {
      p = mem [p ].hhfield .v.RH ;
      if ( ( jointype == 0 ) || ( jointype == 3 ) ) 
      {
	if ( jointype == 0 ) 
	{
	  det = takefraction ( dyout , dxin ) - takefraction ( dxout , dyin ) 
	  ;
	  if ( abs ( det ) < 26844 ) 
	  r = 0 ;
	  else {
	      
	    tmp = takefraction ( mem [q + 1 ].cint - mem [p + 1 ].cint , 
	    dyout ) - takefraction ( mem [q + 2 ].cint - mem [p + 2 ].cint 
	    , dxout ) ;
	    tmp = makefraction ( tmp , det ) ;
	    r = insertknot ( p , mem [p + 1 ].cint + takefraction ( tmp , 
	    dxin ) , mem [p + 2 ].cint + takefraction ( tmp , dyin ) ) ;
	  } 
	} 
	else {
	    
	  htx = mem [w + 2 ].cint - mem [w0 + 2 ].cint ;
	  hty = mem [w0 + 1 ].cint - mem [w + 1 ].cint ;
	  while ( ( abs ( htx ) < 134217728L ) && ( abs ( hty ) < 134217728L ) 
	  ) {
	      
	    htx = htx + htx ;
	    hty = hty + hty ;
	  } 
	  maxht = 0 ;
	  kk = 16384 ;
	  ww = w ;
	  while ( true ) {
	      
	    if ( kk > k0 ) 
	    {
	      ww = mem [ww ].hhfield .v.RH ;
	      decr ( kk ) ;
	    } 
	    else {
		
	      ww = mem [ww ].hhfield .lhfield ;
	      incr ( kk ) ;
	    } 
	    if ( kk == k0 ) 
	    goto lab30 ;
	    tmp = takefraction ( mem [ww + 1 ].cint - mem [w0 + 1 ].cint , 
	    htx ) + takefraction ( mem [ww + 2 ].cint - mem [w0 + 2 ].cint 
	    , hty ) ;
	    if ( tmp > maxht ) 
	    maxht = tmp ;
	  } 
	  lab30: ;
	  tmp = makefraction ( maxht , takefraction ( dxin , htx ) + 
	  takefraction ( dyin , hty ) ) ;
	  r = insertknot ( p , mem [p + 1 ].cint + takefraction ( tmp , dxin 
	  ) , mem [p + 2 ].cint + takefraction ( tmp , dyin ) ) ;
	  tmp = makefraction ( maxht , takefraction ( dxout , htx ) + 
	  takefraction ( dyout , hty ) ) ;
	  r = insertknot ( r , mem [q + 1 ].cint + takefraction ( tmp , 
	  dxout ) , mem [q + 2 ].cint + takefraction ( tmp , dyout ) ) ;
	} 
	if ( r != 0 ) 
	{
	  mem [r + 5 ].cint = mem [r + 1 ].cint ;
	  mem [r + 6 ].cint = mem [r + 2 ].cint ;
	} 
      } 
    } 
    p = q ;
  } while ( ! ( q0 == c ) ) ;
  Result = c ;
  return Result ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zfinddirectiontime ( scaled x , scaled y , halfword h ) 
#else
zfinddirectiontime ( x , y , h ) 
  scaled x ;
  scaled y ;
  halfword h ;
#endif
{
  /* 10 40 45 30 */ register scaled Result; scaled max  ;
  halfword p, q  ;
  scaled n  ;
  scaled tt  ;
  scaled x1, x2, x3, y1, y2, y3  ;
  angle theta, phi  ;
  fraction t  ;
  if ( abs ( x ) < abs ( y ) ) 
  {
    x = makefraction ( x , abs ( y ) ) ;
    if ( y > 0 ) 
    y = 268435456L ;
    else y = -268435456L ;
  } 
  else if ( x == 0 ) 
  {
    Result = 0 ;
    goto lab10 ;
  } 
  else {
      
    y = makefraction ( y , abs ( x ) ) ;
    if ( x > 0 ) 
    x = 268435456L ;
    else x = -268435456L ;
  } 
  n = 0 ;
  p = h ;
  while ( true ) {
      
    if ( mem [p ].hhfield .b1 == 0 ) 
    goto lab45 ;
    q = mem [p ].hhfield .v.RH ;
    tt = 0 ;
    x1 = mem [p + 5 ].cint - mem [p + 1 ].cint ;
    x2 = mem [q + 3 ].cint - mem [p + 5 ].cint ;
    x3 = mem [q + 1 ].cint - mem [q + 3 ].cint ;
    y1 = mem [p + 6 ].cint - mem [p + 2 ].cint ;
    y2 = mem [q + 4 ].cint - mem [p + 6 ].cint ;
    y3 = mem [q + 2 ].cint - mem [q + 4 ].cint ;
    max = abs ( x1 ) ;
    if ( abs ( x2 ) > max ) 
    max = abs ( x2 ) ;
    if ( abs ( x3 ) > max ) 
    max = abs ( x3 ) ;
    if ( abs ( y1 ) > max ) 
    max = abs ( y1 ) ;
    if ( abs ( y2 ) > max ) 
    max = abs ( y2 ) ;
    if ( abs ( y3 ) > max ) 
    max = abs ( y3 ) ;
    if ( max == 0 ) 
    goto lab40 ;
    while ( max < 134217728L ) {
	
      max = max + max ;
      x1 = x1 + x1 ;
      x2 = x2 + x2 ;
      x3 = x3 + x3 ;
      y1 = y1 + y1 ;
      y2 = y2 + y2 ;
      y3 = y3 + y3 ;
    } 
    t = x1 ;
    x1 = takefraction ( x1 , x ) + takefraction ( y1 , y ) ;
    y1 = takefraction ( y1 , x ) - takefraction ( t , y ) ;
    t = x2 ;
    x2 = takefraction ( x2 , x ) + takefraction ( y2 , y ) ;
    y2 = takefraction ( y2 , x ) - takefraction ( t , y ) ;
    t = x3 ;
    x3 = takefraction ( x3 , x ) + takefraction ( y3 , y ) ;
    y3 = takefraction ( y3 , x ) - takefraction ( t , y ) ;
    if ( y1 == 0 ) 
    if ( x1 >= 0 ) 
    goto lab40 ;
    if ( n > 0 ) 
    {
      theta = narg ( x1 , y1 ) ;
      if ( theta >= 0 ) 
      if ( phi <= 0 ) 
      if ( phi >= theta - 188743680L ) 
      goto lab40 ;
      if ( theta <= 0 ) 
      if ( phi >= 0 ) 
      if ( phi <= theta + 188743680L ) 
      goto lab40 ;
      if ( p == h ) 
      goto lab45 ;
    } 
    if ( ( x3 != 0 ) || ( y3 != 0 ) ) 
    phi = narg ( x3 , y3 ) ;
    if ( x1 < 0 ) 
    if ( x2 < 0 ) 
    if ( x3 < 0 ) 
    goto lab30 ;
    if ( abvscd ( y1 , y3 , y2 , y2 ) == 0 ) 
    {
      if ( abvscd ( y1 , y2 , 0 , 0 ) < 0 ) 
      {
	t = makefraction ( y1 , y1 - y2 ) ;
	x1 = x1 - takefraction ( x1 - x2 , t ) ;
	x2 = x2 - takefraction ( x2 - x3 , t ) ;
	if ( x1 - takefraction ( x1 - x2 , t ) >= 0 ) 
	{
	  tt = ( t + 2048 ) / 4096 ;
	  goto lab40 ;
	} 
      } 
      else if ( y3 == 0 ) 
      if ( y1 == 0 ) 
      {
	t = crossingpoint ( - (integer) x1 , - (integer) x2 , - (integer) x3 ) 
	;
	if ( t <= 268435456L ) 
	{
	  tt = ( t + 2048 ) / 4096 ;
	  goto lab40 ;
	} 
	if ( abvscd ( x1 , x3 , x2 , x2 ) <= 0 ) 
	{
	  t = makefraction ( x1 , x1 - x2 ) ;
	  {
	    tt = ( t + 2048 ) / 4096 ;
	    goto lab40 ;
	  } 
	} 
      } 
      else if ( x3 >= 0 ) 
      {
	tt = 65536L ;
	goto lab40 ;
      } 
      goto lab30 ;
    } 
    if ( y1 <= 0 ) 
    if ( y1 < 0 ) 
    {
      y1 = - (integer) y1 ;
      y2 = - (integer) y2 ;
      y3 = - (integer) y3 ;
    } 
    else if ( y2 > 0 ) 
    {
      y2 = - (integer) y2 ;
      y3 = - (integer) y3 ;
    } 
    t = crossingpoint ( y1 , y2 , y3 ) ;
    if ( t > 268435456L ) 
    goto lab30 ;
    y2 = y2 - takefraction ( y2 - y3 , t ) ;
    x1 = x1 - takefraction ( x1 - x2 , t ) ;
    x2 = x2 - takefraction ( x2 - x3 , t ) ;
    x1 = x1 - takefraction ( x1 - x2 , t ) ;
    if ( x1 >= 0 ) 
    {
      tt = ( t + 2048 ) / 4096 ;
      goto lab40 ;
    } 
    if ( y2 > 0 ) 
    y2 = 0 ;
    tt = t ;
    t = crossingpoint ( 0 , - (integer) y2 , - (integer) y3 ) ;
    if ( t > 268435456L ) 
    goto lab30 ;
    x1 = x1 - takefraction ( x1 - x2 , t ) ;
    x2 = x2 - takefraction ( x2 - x3 , t ) ;
    if ( x1 - takefraction ( x1 - x2 , t ) >= 0 ) 
    {
      t = tt - takefraction ( tt - 268435456L , t ) ;
      {
	tt = ( t + 2048 ) / 4096 ;
	goto lab40 ;
      } 
    } 
    lab30: ;
    p = q ;
    n = n + 65536L ;
  } 
  lab45: Result = -65536L ;
  goto lab10 ;
  lab40: Result = n + tt ;
  lab10: ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zcubicintersection ( halfword p , halfword pp ) 
#else
zcubicintersection ( p , pp ) 
  halfword p ;
  halfword pp ;
#endif
{
  /* 22 45 10 */ halfword q, qq  ;
  timetogo = 5000 ;
  maxt = 2 ;
  q = mem [p ].hhfield .v.RH ;
  qq = mem [pp ].hhfield .v.RH ;
  bisectptr = 20 ;
  bisectstack [bisectptr - 5 ]= mem [p + 5 ].cint - mem [p + 1 ].cint ;
  bisectstack [bisectptr - 4 ]= mem [q + 3 ].cint - mem [p + 5 ].cint ;
  bisectstack [bisectptr - 3 ]= mem [q + 1 ].cint - mem [q + 3 ].cint ;
  if ( bisectstack [bisectptr - 5 ]< 0 ) 
  if ( bisectstack [bisectptr - 3 ]>= 0 ) 
  {
    if ( bisectstack [bisectptr - 4 ]< 0 ) 
    bisectstack [bisectptr - 2 ]= bisectstack [bisectptr - 5 ]+ 
    bisectstack [bisectptr - 4 ];
    else bisectstack [bisectptr - 2 ]= bisectstack [bisectptr - 5 ];
    bisectstack [bisectptr - 1 ]= bisectstack [bisectptr - 5 ]+ 
    bisectstack [bisectptr - 4 ]+ bisectstack [bisectptr - 3 ];
    if ( bisectstack [bisectptr - 1 ]< 0 ) 
    bisectstack [bisectptr - 1 ]= 0 ;
  } 
  else {
      
    bisectstack [bisectptr - 2 ]= bisectstack [bisectptr - 5 ]+ 
    bisectstack [bisectptr - 4 ]+ bisectstack [bisectptr - 3 ];
    if ( bisectstack [bisectptr - 2 ]> bisectstack [bisectptr - 5 ]) 
    bisectstack [bisectptr - 2 ]= bisectstack [bisectptr - 5 ];
    bisectstack [bisectptr - 1 ]= bisectstack [bisectptr - 5 ]+ 
    bisectstack [bisectptr - 4 ];
    if ( bisectstack [bisectptr - 1 ]< 0 ) 
    bisectstack [bisectptr - 1 ]= 0 ;
  } 
  else if ( bisectstack [bisectptr - 3 ]<= 0 ) 
  {
    if ( bisectstack [bisectptr - 4 ]> 0 ) 
    bisectstack [bisectptr - 1 ]= bisectstack [bisectptr - 5 ]+ 
    bisectstack [bisectptr - 4 ];
    else bisectstack [bisectptr - 1 ]= bisectstack [bisectptr - 5 ];
    bisectstack [bisectptr - 2 ]= bisectstack [bisectptr - 5 ]+ 
    bisectstack [bisectptr - 4 ]+ bisectstack [bisectptr - 3 ];
    if ( bisectstack [bisectptr - 2 ]> 0 ) 
    bisectstack [bisectptr - 2 ]= 0 ;
  } 
  else {
      
    bisectstack [bisectptr - 1 ]= bisectstack [bisectptr - 5 ]+ 
    bisectstack [bisectptr - 4 ]+ bisectstack [bisectptr - 3 ];
    if ( bisectstack [bisectptr - 1 ]< bisectstack [bisectptr - 5 ]) 
    bisectstack [bisectptr - 1 ]= bisectstack [bisectptr - 5 ];
    bisectstack [bisectptr - 2 ]= bisectstack [bisectptr - 5 ]+ 
    bisectstack [bisectptr - 4 ];
    if ( bisectstack [bisectptr - 2 ]> 0 ) 
    bisectstack [bisectptr - 2 ]= 0 ;
  } 
  bisectstack [bisectptr - 10 ]= mem [p + 6 ].cint - mem [p + 2 ].cint ;
  bisectstack [bisectptr - 9 ]= mem [q + 4 ].cint - mem [p + 6 ].cint ;
  bisectstack [bisectptr - 8 ]= mem [q + 2 ].cint - mem [q + 4 ].cint ;
  if ( bisectstack [bisectptr - 10 ]< 0 ) 
  if ( bisectstack [bisectptr - 8 ]>= 0 ) 
  {
    if ( bisectstack [bisectptr - 9 ]< 0 ) 
    bisectstack [bisectptr - 7 ]= bisectstack [bisectptr - 10 ]+ 
    bisectstack [bisectptr - 9 ];
    else bisectstack [bisectptr - 7 ]= bisectstack [bisectptr - 10 ];
    bisectstack [bisectptr - 6 ]= bisectstack [bisectptr - 10 ]+ 
    bisectstack [bisectptr - 9 ]+ bisectstack [bisectptr - 8 ];
    if ( bisectstack [bisectptr - 6 ]< 0 ) 
    bisectstack [bisectptr - 6 ]= 0 ;
  } 
  else {
      
    bisectstack [bisectptr - 7 ]= bisectstack [bisectptr - 10 ]+ 
    bisectstack [bisectptr - 9 ]+ bisectstack [bisectptr - 8 ];
    if ( bisectstack [bisectptr - 7 ]> bisectstack [bisectptr - 10 ]) 
    bisectstack [bisectptr - 7 ]= bisectstack [bisectptr - 10 ];
    bisectstack [bisectptr - 6 ]= bisectstack [bisectptr - 10 ]+ 
    bisectstack [bisectptr - 9 ];
    if ( bisectstack [bisectptr - 6 ]< 0 ) 
    bisectstack [bisectptr - 6 ]= 0 ;
  } 
  else if ( bisectstack [bisectptr - 8 ]<= 0 ) 
  {
    if ( bisectstack [bisectptr - 9 ]> 0 ) 
    bisectstack [bisectptr - 6 ]= bisectstack [bisectptr - 10 ]+ 
    bisectstack [bisectptr - 9 ];
    else bisectstack [bisectptr - 6 ]= bisectstack [bisectptr - 10 ];
    bisectstack [bisectptr - 7 ]= bisectstack [bisectptr - 10 ]+ 
    bisectstack [bisectptr - 9 ]+ bisectstack [bisectptr - 8 ];
    if ( bisectstack [bisectptr - 7 ]> 0 ) 
    bisectstack [bisectptr - 7 ]= 0 ;
  } 
  else {
      
    bisectstack [bisectptr - 6 ]= bisectstack [bisectptr - 10 ]+ 
    bisectstack [bisectptr - 9 ]+ bisectstack [bisectptr - 8 ];
    if ( bisectstack [bisectptr - 6 ]< bisectstack [bisectptr - 10 ]) 
    bisectstack [bisectptr - 6 ]= bisectstack [bisectptr - 10 ];
    bisectstack [bisectptr - 7 ]= bisectstack [bisectptr - 10 ]+ 
    bisectstack [bisectptr - 9 ];
    if ( bisectstack [bisectptr - 7 ]> 0 ) 
    bisectstack [bisectptr - 7 ]= 0 ;
  } 
  bisectstack [bisectptr - 15 ]= mem [pp + 5 ].cint - mem [pp + 1 ].cint 
  ;
  bisectstack [bisectptr - 14 ]= mem [qq + 3 ].cint - mem [pp + 5 ].cint 
  ;
  bisectstack [bisectptr - 13 ]= mem [qq + 1 ].cint - mem [qq + 3 ].cint 
  ;
  if ( bisectstack [bisectptr - 15 ]< 0 ) 
  if ( bisectstack [bisectptr - 13 ]>= 0 ) 
  {
    if ( bisectstack [bisectptr - 14 ]< 0 ) 
    bisectstack [bisectptr - 12 ]= bisectstack [bisectptr - 15 ]+ 
    bisectstack [bisectptr - 14 ];
    else bisectstack [bisectptr - 12 ]= bisectstack [bisectptr - 15 ];
    bisectstack [bisectptr - 11 ]= bisectstack [bisectptr - 15 ]+ 
    bisectstack [bisectptr - 14 ]+ bisectstack [bisectptr - 13 ];
    if ( bisectstack [bisectptr - 11 ]< 0 ) 
    bisectstack [bisectptr - 11 ]= 0 ;
  } 
  else {
      
    bisectstack [bisectptr - 12 ]= bisectstack [bisectptr - 15 ]+ 
    bisectstack [bisectptr - 14 ]+ bisectstack [bisectptr - 13 ];
    if ( bisectstack [bisectptr - 12 ]> bisectstack [bisectptr - 15 ]) 
    bisectstack [bisectptr - 12 ]= bisectstack [bisectptr - 15 ];
    bisectstack [bisectptr - 11 ]= bisectstack [bisectptr - 15 ]+ 
    bisectstack [bisectptr - 14 ];
    if ( bisectstack [bisectptr - 11 ]< 0 ) 
    bisectstack [bisectptr - 11 ]= 0 ;
  } 
  else if ( bisectstack [bisectptr - 13 ]<= 0 ) 
  {
    if ( bisectstack [bisectptr - 14 ]> 0 ) 
    bisectstack [bisectptr - 11 ]= bisectstack [bisectptr - 15 ]+ 
    bisectstack [bisectptr - 14 ];
    else bisectstack [bisectptr - 11 ]= bisectstack [bisectptr - 15 ];
    bisectstack [bisectptr - 12 ]= bisectstack [bisectptr - 15 ]+ 
    bisectstack [bisectptr - 14 ]+ bisectstack [bisectptr - 13 ];
    if ( bisectstack [bisectptr - 12 ]> 0 ) 
    bisectstack [bisectptr - 12 ]= 0 ;
  } 
  else {
      
    bisectstack [bisectptr - 11 ]= bisectstack [bisectptr - 15 ]+ 
    bisectstack [bisectptr - 14 ]+ bisectstack [bisectptr - 13 ];
    if ( bisectstack [bisectptr - 11 ]< bisectstack [bisectptr - 15 ]) 
    bisectstack [bisectptr - 11 ]= bisectstack [bisectptr - 15 ];
    bisectstack [bisectptr - 12 ]= bisectstack [bisectptr - 15 ]+ 
    bisectstack [bisectptr - 14 ];
    if ( bisectstack [bisectptr - 12 ]> 0 ) 
    bisectstack [bisectptr - 12 ]= 0 ;
  } 
  bisectstack [bisectptr - 20 ]= mem [pp + 6 ].cint - mem [pp + 2 ].cint 
  ;
  bisectstack [bisectptr - 19 ]= mem [qq + 4 ].cint - mem [pp + 6 ].cint 
  ;
  bisectstack [bisectptr - 18 ]= mem [qq + 2 ].cint - mem [qq + 4 ].cint 
  ;
  if ( bisectstack [bisectptr - 20 ]< 0 ) 
  if ( bisectstack [bisectptr - 18 ]>= 0 ) 
  {
    if ( bisectstack [bisectptr - 19 ]< 0 ) 
    bisectstack [bisectptr - 17 ]= bisectstack [bisectptr - 20 ]+ 
    bisectstack [bisectptr - 19 ];
    else bisectstack [bisectptr - 17 ]= bisectstack [bisectptr - 20 ];
    bisectstack [bisectptr - 16 ]= bisectstack [bisectptr - 20 ]+ 
    bisectstack [bisectptr - 19 ]+ bisectstack [bisectptr - 18 ];
    if ( bisectstack [bisectptr - 16 ]< 0 ) 
    bisectstack [bisectptr - 16 ]= 0 ;
  } 
  else {
      
    bisectstack [bisectptr - 17 ]= bisectstack [bisectptr - 20 ]+ 
    bisectstack [bisectptr - 19 ]+ bisectstack [bisectptr - 18 ];
    if ( bisectstack [bisectptr - 17 ]> bisectstack [bisectptr - 20 ]) 
    bisectstack [bisectptr - 17 ]= bisectstack [bisectptr - 20 ];
    bisectstack [bisectptr - 16 ]= bisectstack [bisectptr - 20 ]+ 
    bisectstack [bisectptr - 19 ];
    if ( bisectstack [bisectptr - 16 ]< 0 ) 
    bisectstack [bisectptr - 16 ]= 0 ;
  } 
  else if ( bisectstack [bisectptr - 18 ]<= 0 ) 
  {
    if ( bisectstack [bisectptr - 19 ]> 0 ) 
    bisectstack [bisectptr - 16 ]= bisectstack [bisectptr - 20 ]+ 
    bisectstack [bisectptr - 19 ];
    else bisectstack [bisectptr - 16 ]= bisectstack [bisectptr - 20 ];
    bisectstack [bisectptr - 17 ]= bisectstack [bisectptr - 20 ]+ 
    bisectstack [bisectptr - 19 ]+ bisectstack [bisectptr - 18 ];
    if ( bisectstack [bisectptr - 17 ]> 0 ) 
    bisectstack [bisectptr - 17 ]= 0 ;
  } 
  else {
      
    bisectstack [bisectptr - 16 ]= bisectstack [bisectptr - 20 ]+ 
    bisectstack [bisectptr - 19 ]+ bisectstack [bisectptr - 18 ];
    if ( bisectstack [bisectptr - 16 ]< bisectstack [bisectptr - 20 ]) 
    bisectstack [bisectptr - 16 ]= bisectstack [bisectptr - 20 ];
    bisectstack [bisectptr - 17 ]= bisectstack [bisectptr - 20 ]+ 
    bisectstack [bisectptr - 19 ];
    if ( bisectstack [bisectptr - 17 ]> 0 ) 
    bisectstack [bisectptr - 17 ]= 0 ;
  } 
  delx = mem [p + 1 ].cint - mem [pp + 1 ].cint ;
  dely = mem [p + 2 ].cint - mem [pp + 2 ].cint ;
  tol = 0 ;
  uv = bisectptr ;
  xy = bisectptr ;
  threel = 0 ;
  curt = 1 ;
  curtt = 1 ;
  while ( true ) {
      
    lab22: if ( delx - tol <= bisectstack [xy - 11 ]- bisectstack [uv - 2 ]
    ) 
    if ( delx + tol >= bisectstack [xy - 12 ]- bisectstack [uv - 1 ]) 
    if ( dely - tol <= bisectstack [xy - 16 ]- bisectstack [uv - 7 ]) 
    if ( dely + tol >= bisectstack [xy - 17 ]- bisectstack [uv - 6 ]) 
    {
      if ( curt >= maxt ) 
      {
	if ( maxt == 131072L ) 
	{
	  curt = halfp ( curt + 1 ) ;
	  curtt = halfp ( curtt + 1 ) ;
	  goto lab10 ;
	} 
	maxt = maxt + maxt ;
	apprt = curt ;
	apprtt = curtt ;
      } 
      bisectstack [bisectptr ]= delx ;
      bisectstack [bisectptr + 1 ]= dely ;
      bisectstack [bisectptr + 2 ]= tol ;
      bisectstack [bisectptr + 3 ]= uv ;
      bisectstack [bisectptr + 4 ]= xy ;
      bisectptr = bisectptr + 45 ;
      curt = curt + curt ;
      curtt = curtt + curtt ;
      bisectstack [bisectptr - 25 ]= bisectstack [uv - 5 ];
      bisectstack [bisectptr - 3 ]= bisectstack [uv - 3 ];
      bisectstack [bisectptr - 24 ]= half ( bisectstack [bisectptr - 25 ]+ 
      bisectstack [uv - 4 ]) ;
      bisectstack [bisectptr - 4 ]= half ( bisectstack [bisectptr - 3 ]+ 
      bisectstack [uv - 4 ]) ;
      bisectstack [bisectptr - 23 ]= half ( bisectstack [bisectptr - 24 ]+ 
      bisectstack [bisectptr - 4 ]) ;
      bisectstack [bisectptr - 5 ]= bisectstack [bisectptr - 23 ];
      if ( bisectstack [bisectptr - 25 ]< 0 ) 
      if ( bisectstack [bisectptr - 23 ]>= 0 ) 
      {
	if ( bisectstack [bisectptr - 24 ]< 0 ) 
	bisectstack [bisectptr - 22 ]= bisectstack [bisectptr - 25 ]+ 
	bisectstack [bisectptr - 24 ];
	else bisectstack [bisectptr - 22 ]= bisectstack [bisectptr - 25 ];
	bisectstack [bisectptr - 21 ]= bisectstack [bisectptr - 25 ]+ 
	bisectstack [bisectptr - 24 ]+ bisectstack [bisectptr - 23 ];
	if ( bisectstack [bisectptr - 21 ]< 0 ) 
	bisectstack [bisectptr - 21 ]= 0 ;
      } 
      else {
	  
	bisectstack [bisectptr - 22 ]= bisectstack [bisectptr - 25 ]+ 
	bisectstack [bisectptr - 24 ]+ bisectstack [bisectptr - 23 ];
	if ( bisectstack [bisectptr - 22 ]> bisectstack [bisectptr - 25 ]) 
	bisectstack [bisectptr - 22 ]= bisectstack [bisectptr - 25 ];
	bisectstack [bisectptr - 21 ]= bisectstack [bisectptr - 25 ]+ 
	bisectstack [bisectptr - 24 ];
	if ( bisectstack [bisectptr - 21 ]< 0 ) 
	bisectstack [bisectptr - 21 ]= 0 ;
      } 
      else if ( bisectstack [bisectptr - 23 ]<= 0 ) 
      {
	if ( bisectstack [bisectptr - 24 ]> 0 ) 
	bisectstack [bisectptr - 21 ]= bisectstack [bisectptr - 25 ]+ 
	bisectstack [bisectptr - 24 ];
	else bisectstack [bisectptr - 21 ]= bisectstack [bisectptr - 25 ];
	bisectstack [bisectptr - 22 ]= bisectstack [bisectptr - 25 ]+ 
	bisectstack [bisectptr - 24 ]+ bisectstack [bisectptr - 23 ];
	if ( bisectstack [bisectptr - 22 ]> 0 ) 
	bisectstack [bisectptr - 22 ]= 0 ;
      } 
      else {
	  
	bisectstack [bisectptr - 21 ]= bisectstack [bisectptr - 25 ]+ 
	bisectstack [bisectptr - 24 ]+ bisectstack [bisectptr - 23 ];
	if ( bisectstack [bisectptr - 21 ]< bisectstack [bisectptr - 25 ]) 
	bisectstack [bisectptr - 21 ]= bisectstack [bisectptr - 25 ];
	bisectstack [bisectptr - 22 ]= bisectstack [bisectptr - 25 ]+ 
	bisectstack [bisectptr - 24 ];
	if ( bisectstack [bisectptr - 22 ]> 0 ) 
	bisectstack [bisectptr - 22 ]= 0 ;
      } 
      if ( bisectstack [bisectptr - 5 ]< 0 ) 
      if ( bisectstack [bisectptr - 3 ]>= 0 ) 
      {
	if ( bisectstack [bisectptr - 4 ]< 0 ) 
	bisectstack [bisectptr - 2 ]= bisectstack [bisectptr - 5 ]+ 
	bisectstack [bisectptr - 4 ];
	else bisectstack [bisectptr - 2 ]= bisectstack [bisectptr - 5 ];
	bisectstack [bisectptr - 1 ]= bisectstack [bisectptr - 5 ]+ 
	bisectstack [bisectptr - 4 ]+ bisectstack [bisectptr - 3 ];
	if ( bisectstack [bisectptr - 1 ]< 0 ) 
	bisectstack [bisectptr - 1 ]= 0 ;
      } 
      else {
	  
	bisectstack [bisectptr - 2 ]= bisectstack [bisectptr - 5 ]+ 
	bisectstack [bisectptr - 4 ]+ bisectstack [bisectptr - 3 ];
	if ( bisectstack [bisectptr - 2 ]> bisectstack [bisectptr - 5 ]) 
	bisectstack [bisectptr - 2 ]= bisectstack [bisectptr - 5 ];
	bisectstack [bisectptr - 1 ]= bisectstack [bisectptr - 5 ]+ 
	bisectstack [bisectptr - 4 ];
	if ( bisectstack [bisectptr - 1 ]< 0 ) 
	bisectstack [bisectptr - 1 ]= 0 ;
      } 
      else if ( bisectstack [bisectptr - 3 ]<= 0 ) 
      {
	if ( bisectstack [bisectptr - 4 ]> 0 ) 
	bisectstack [bisectptr - 1 ]= bisectstack [bisectptr - 5 ]+ 
	bisectstack [bisectptr - 4 ];
	else bisectstack [bisectptr - 1 ]= bisectstack [bisectptr - 5 ];
	bisectstack [bisectptr - 2 ]= bisectstack [bisectptr - 5 ]+ 
	bisectstack [bisectptr - 4 ]+ bisectstack [bisectptr - 3 ];
	if ( bisectstack [bisectptr - 2 ]> 0 ) 
	bisectstack [bisectptr - 2 ]= 0 ;
      } 
      else {
	  
	bisectstack [bisectptr - 1 ]= bisectstack [bisectptr - 5 ]+ 
	bisectstack [bisectptr - 4 ]+ bisectstack [bisectptr - 3 ];
	if ( bisectstack [bisectptr - 1 ]< bisectstack [bisectptr - 5 ]) 
	bisectstack [bisectptr - 1 ]= bisectstack [bisectptr - 5 ];
	bisectstack [bisectptr - 2 ]= bisectstack [bisectptr - 5 ]+ 
	bisectstack [bisectptr - 4 ];
	if ( bisectstack [bisectptr - 2 ]> 0 ) 
	bisectstack [bisectptr - 2 ]= 0 ;
      } 
      bisectstack [bisectptr - 30 ]= bisectstack [uv - 10 ];
      bisectstack [bisectptr - 8 ]= bisectstack [uv - 8 ];
      bisectstack [bisectptr - 29 ]= half ( bisectstack [bisectptr - 30 ]+ 
      bisectstack [uv - 9 ]) ;
      bisectstack [bisectptr - 9 ]= half ( bisectstack [bisectptr - 8 ]+ 
      bisectstack [uv - 9 ]) ;
      bisectstack [bisectptr - 28 ]= half ( bisectstack [bisectptr - 29 ]+ 
      bisectstack [bisectptr - 9 ]) ;
      bisectstack [bisectptr - 10 ]= bisectstack [bisectptr - 28 ];
      if ( bisectstack [bisectptr - 30 ]< 0 ) 
      if ( bisectstack [bisectptr - 28 ]>= 0 ) 
      {
	if ( bisectstack [bisectptr - 29 ]< 0 ) 
	bisectstack [bisectptr - 27 ]= bisectstack [bisectptr - 30 ]+ 
	bisectstack [bisectptr - 29 ];
	else bisectstack [bisectptr - 27 ]= bisectstack [bisectptr - 30 ];
	bisectstack [bisectptr - 26 ]= bisectstack [bisectptr - 30 ]+ 
	bisectstack [bisectptr - 29 ]+ bisectstack [bisectptr - 28 ];
	if ( bisectstack [bisectptr - 26 ]< 0 ) 
	bisectstack [bisectptr - 26 ]= 0 ;
      } 
      else {
	  
	bisectstack [bisectptr - 27 ]= bisectstack [bisectptr - 30 ]+ 
	bisectstack [bisectptr - 29 ]+ bisectstack [bisectptr - 28 ];
	if ( bisectstack [bisectptr - 27 ]> bisectstack [bisectptr - 30 ]) 
	bisectstack [bisectptr - 27 ]= bisectstack [bisectptr - 30 ];
	bisectstack [bisectptr - 26 ]= bisectstack [bisectptr - 30 ]+ 
	bisectstack [bisectptr - 29 ];
	if ( bisectstack [bisectptr - 26 ]< 0 ) 
	bisectstack [bisectptr - 26 ]= 0 ;
      } 
      else if ( bisectstack [bisectptr - 28 ]<= 0 ) 
      {
	if ( bisectstack [bisectptr - 29 ]> 0 ) 
	bisectstack [bisectptr - 26 ]= bisectstack [bisectptr - 30 ]+ 
	bisectstack [bisectptr - 29 ];
	else bisectstack [bisectptr - 26 ]= bisectstack [bisectptr - 30 ];
	bisectstack [bisectptr - 27 ]= bisectstack [bisectptr - 30 ]+ 
	bisectstack [bisectptr - 29 ]+ bisectstack [bisectptr - 28 ];
	if ( bisectstack [bisectptr - 27 ]> 0 ) 
	bisectstack [bisectptr - 27 ]= 0 ;
      } 
      else {
	  
	bisectstack [bisectptr - 26 ]= bisectstack [bisectptr - 30 ]+ 
	bisectstack [bisectptr - 29 ]+ bisectstack [bisectptr - 28 ];
	if ( bisectstack [bisectptr - 26 ]< bisectstack [bisectptr - 30 ]) 
	bisectstack [bisectptr - 26 ]= bisectstack [bisectptr - 30 ];
	bisectstack [bisectptr - 27 ]= bisectstack [bisectptr - 30 ]+ 
	bisectstack [bisectptr - 29 ];
	if ( bisectstack [bisectptr - 27 ]> 0 ) 
	bisectstack [bisectptr - 27 ]= 0 ;
      } 
      if ( bisectstack [bisectptr - 10 ]< 0 ) 
      if ( bisectstack [bisectptr - 8 ]>= 0 ) 
      {
	if ( bisectstack [bisectptr - 9 ]< 0 ) 
	bisectstack [bisectptr - 7 ]= bisectstack [bisectptr - 10 ]+ 
	bisectstack [bisectptr - 9 ];
	else bisectstack [bisectptr - 7 ]= bisectstack [bisectptr - 10 ];
	bisectstack [bisectptr - 6 ]= bisectstack [bisectptr - 10 ]+ 
	bisectstack [bisectptr - 9 ]+ bisectstack [bisectptr - 8 ];
	if ( bisectstack [bisectptr - 6 ]< 0 ) 
	bisectstack [bisectptr - 6 ]= 0 ;
      } 
      else {
	  
	bisectstack [bisectptr - 7 ]= bisectstack [bisectptr - 10 ]+ 
	bisectstack [bisectptr - 9 ]+ bisectstack [bisectptr - 8 ];
	if ( bisectstack [bisectptr - 7 ]> bisectstack [bisectptr - 10 ]) 
	bisectstack [bisectptr - 7 ]= bisectstack [bisectptr - 10 ];
	bisectstack [bisectptr - 6 ]= bisectstack [bisectptr - 10 ]+ 
	bisectstack [bisectptr - 9 ];
	if ( bisectstack [bisectptr - 6 ]< 0 ) 
	bisectstack [bisectptr - 6 ]= 0 ;
      } 
      else if ( bisectstack [bisectptr - 8 ]<= 0 ) 
      {
	if ( bisectstack [bisectptr - 9 ]> 0 ) 
	bisectstack [bisectptr - 6 ]= bisectstack [bisectptr - 10 ]+ 
	bisectstack [bisectptr - 9 ];
	else bisectstack [bisectptr - 6 ]= bisectstack [bisectptr - 10 ];
	bisectstack [bisectptr - 7 ]= bisectstack [bisectptr - 10 ]+ 
	bisectstack [bisectptr - 9 ]+ bisectstack [bisectptr - 8 ];
	if ( bisectstack [bisectptr - 7 ]> 0 ) 
	bisectstack [bisectptr - 7 ]= 0 ;
      } 
      else {
	  
	bisectstack [bisectptr - 6 ]= bisectstack [bisectptr - 10 ]+ 
	bisectstack [bisectptr - 9 ]+ bisectstack [bisectptr - 8 ];
	if ( bisectstack [bisectptr - 6 ]< bisectstack [bisectptr - 10 ]) 
	bisectstack [bisectptr - 6 ]= bisectstack [bisectptr - 10 ];
	bisectstack [bisectptr - 7 ]= bisectstack [bisectptr - 10 ]+ 
	bisectstack [bisectptr - 9 ];
	if ( bisectstack [bisectptr - 7 ]> 0 ) 
	bisectstack [bisectptr - 7 ]= 0 ;
      } 
      bisectstack [bisectptr - 35 ]= bisectstack [xy - 15 ];
      bisectstack [bisectptr - 13 ]= bisectstack [xy - 13 ];
      bisectstack [bisectptr - 34 ]= half ( bisectstack [bisectptr - 35 ]+ 
      bisectstack [xy - 14 ]) ;
      bisectstack [bisectptr - 14 ]= half ( bisectstack [bisectptr - 13 ]+ 
      bisectstack [xy - 14 ]) ;
      bisectstack [bisectptr - 33 ]= half ( bisectstack [bisectptr - 34 ]+ 
      bisectstack [bisectptr - 14 ]) ;
      bisectstack [bisectptr - 15 ]= bisectstack [bisectptr - 33 ];
      if ( bisectstack [bisectptr - 35 ]< 0 ) 
      if ( bisectstack [bisectptr - 33 ]>= 0 ) 
      {
	if ( bisectstack [bisectptr - 34 ]< 0 ) 
	bisectstack [bisectptr - 32 ]= bisectstack [bisectptr - 35 ]+ 
	bisectstack [bisectptr - 34 ];
	else bisectstack [bisectptr - 32 ]= bisectstack [bisectptr - 35 ];
	bisectstack [bisectptr - 31 ]= bisectstack [bisectptr - 35 ]+ 
	bisectstack [bisectptr - 34 ]+ bisectstack [bisectptr - 33 ];
	if ( bisectstack [bisectptr - 31 ]< 0 ) 
	bisectstack [bisectptr - 31 ]= 0 ;
      } 
      else {
	  
	bisectstack [bisectptr - 32 ]= bisectstack [bisectptr - 35 ]+ 
	bisectstack [bisectptr - 34 ]+ bisectstack [bisectptr - 33 ];
	if ( bisectstack [bisectptr - 32 ]> bisectstack [bisectptr - 35 ]) 
	bisectstack [bisectptr - 32 ]= bisectstack [bisectptr - 35 ];
	bisectstack [bisectptr - 31 ]= bisectstack [bisectptr - 35 ]+ 
	bisectstack [bisectptr - 34 ];
	if ( bisectstack [bisectptr - 31 ]< 0 ) 
	bisectstack [bisectptr - 31 ]= 0 ;
      } 
      else if ( bisectstack [bisectptr - 33 ]<= 0 ) 
      {
	if ( bisectstack [bisectptr - 34 ]> 0 ) 
	bisectstack [bisectptr - 31 ]= bisectstack [bisectptr - 35 ]+ 
	bisectstack [bisectptr - 34 ];
	else bisectstack [bisectptr - 31 ]= bisectstack [bisectptr - 35 ];
	bisectstack [bisectptr - 32 ]= bisectstack [bisectptr - 35 ]+ 
	bisectstack [bisectptr - 34 ]+ bisectstack [bisectptr - 33 ];
	if ( bisectstack [bisectptr - 32 ]> 0 ) 
	bisectstack [bisectptr - 32 ]= 0 ;
      } 
      else {
	  
	bisectstack [bisectptr - 31 ]= bisectstack [bisectptr - 35 ]+ 
	bisectstack [bisectptr - 34 ]+ bisectstack [bisectptr - 33 ];
	if ( bisectstack [bisectptr - 31 ]< bisectstack [bisectptr - 35 ]) 
	bisectstack [bisectptr - 31 ]= bisectstack [bisectptr - 35 ];
	bisectstack [bisectptr - 32 ]= bisectstack [bisectptr - 35 ]+ 
	bisectstack [bisectptr - 34 ];
	if ( bisectstack [bisectptr - 32 ]> 0 ) 
	bisectstack [bisectptr - 32 ]= 0 ;
      } 
      if ( bisectstack [bisectptr - 15 ]< 0 ) 
      if ( bisectstack [bisectptr - 13 ]>= 0 ) 
      {
	if ( bisectstack [bisectptr - 14 ]< 0 ) 
	bisectstack [bisectptr - 12 ]= bisectstack [bisectptr - 15 ]+ 
	bisectstack [bisectptr - 14 ];
	else bisectstack [bisectptr - 12 ]= bisectstack [bisectptr - 15 ];
	bisectstack [bisectptr - 11 ]= bisectstack [bisectptr - 15 ]+ 
	bisectstack [bisectptr - 14 ]+ bisectstack [bisectptr - 13 ];
	if ( bisectstack [bisectptr - 11 ]< 0 ) 
	bisectstack [bisectptr - 11 ]= 0 ;
      } 
      else {
	  
	bisectstack [bisectptr - 12 ]= bisectstack [bisectptr - 15 ]+ 
	bisectstack [bisectptr - 14 ]+ bisectstack [bisectptr - 13 ];
	if ( bisectstack [bisectptr - 12 ]> bisectstack [bisectptr - 15 ]) 
	bisectstack [bisectptr - 12 ]= bisectstack [bisectptr - 15 ];
	bisectstack [bisectptr - 11 ]= bisectstack [bisectptr - 15 ]+ 
	bisectstack [bisectptr - 14 ];
	if ( bisectstack [bisectptr - 11 ]< 0 ) 
	bisectstack [bisectptr - 11 ]= 0 ;
      } 
      else if ( bisectstack [bisectptr - 13 ]<= 0 ) 
      {
	if ( bisectstack [bisectptr - 14 ]> 0 ) 
	bisectstack [bisectptr - 11 ]= bisectstack [bisectptr - 15 ]+ 
	bisectstack [bisectptr - 14 ];
	else bisectstack [bisectptr - 11 ]= bisectstack [bisectptr - 15 ];
	bisectstack [bisectptr - 12 ]= bisectstack [bisectptr - 15 ]+ 
	bisectstack [bisectptr - 14 ]+ bisectstack [bisectptr - 13 ];
	if ( bisectstack [bisectptr - 12 ]> 0 ) 
	bisectstack [bisectptr - 12 ]= 0 ;
      } 
      else {
	  
	bisectstack [bisectptr - 11 ]= bisectstack [bisectptr - 15 ]+ 
	bisectstack [bisectptr - 14 ]+ bisectstack [bisectptr - 13 ];
	if ( bisectstack [bisectptr - 11 ]< bisectstack [bisectptr - 15 ]) 
	bisectstack [bisectptr - 11 ]= bisectstack [bisectptr - 15 ];
	bisectstack [bisectptr - 12 ]= bisectstack [bisectptr - 15 ]+ 
	bisectstack [bisectptr - 14 ];
	if ( bisectstack [bisectptr - 12 ]> 0 ) 
	bisectstack [bisectptr - 12 ]= 0 ;
      } 
      bisectstack [bisectptr - 40 ]= bisectstack [xy - 20 ];
      bisectstack [bisectptr - 18 ]= bisectstack [xy - 18 ];
      bisectstack [bisectptr - 39 ]= half ( bisectstack [bisectptr - 40 ]+ 
      bisectstack [xy - 19 ]) ;
      bisectstack [bisectptr - 19 ]= half ( bisectstack [bisectptr - 18 ]+ 
      bisectstack [xy - 19 ]) ;
      bisectstack [bisectptr - 38 ]= half ( bisectstack [bisectptr - 39 ]+ 
      bisectstack [bisectptr - 19 ]) ;
      bisectstack [bisectptr - 20 ]= bisectstack [bisectptr - 38 ];
      if ( bisectstack [bisectptr - 40 ]< 0 ) 
      if ( bisectstack [bisectptr - 38 ]>= 0 ) 
      {
	if ( bisectstack [bisectptr - 39 ]< 0 ) 
	bisectstack [bisectptr - 37 ]= bisectstack [bisectptr - 40 ]+ 
	bisectstack [bisectptr - 39 ];
	else bisectstack [bisectptr - 37 ]= bisectstack [bisectptr - 40 ];
	bisectstack [bisectptr - 36 ]= bisectstack [bisectptr - 40 ]+ 
	bisectstack [bisectptr - 39 ]+ bisectstack [bisectptr - 38 ];
	if ( bisectstack [bisectptr - 36 ]< 0 ) 
	bisectstack [bisectptr - 36 ]= 0 ;
      } 
      else {
	  
	bisectstack [bisectptr - 37 ]= bisectstack [bisectptr - 40 ]+ 
	bisectstack [bisectptr - 39 ]+ bisectstack [bisectptr - 38 ];
	if ( bisectstack [bisectptr - 37 ]> bisectstack [bisectptr - 40 ]) 
	bisectstack [bisectptr - 37 ]= bisectstack [bisectptr - 40 ];
	bisectstack [bisectptr - 36 ]= bisectstack [bisectptr - 40 ]+ 
	bisectstack [bisectptr - 39 ];
	if ( bisectstack [bisectptr - 36 ]< 0 ) 
	bisectstack [bisectptr - 36 ]= 0 ;
      } 
      else if ( bisectstack [bisectptr - 38 ]<= 0 ) 
      {
	if ( bisectstack [bisectptr - 39 ]> 0 ) 
	bisectstack [bisectptr - 36 ]= bisectstack [bisectptr - 40 ]+ 
	bisectstack [bisectptr - 39 ];
	else bisectstack [bisectptr - 36 ]= bisectstack [bisectptr - 40 ];
	bisectstack [bisectptr - 37 ]= bisectstack [bisectptr - 40 ]+ 
	bisectstack [bisectptr - 39 ]+ bisectstack [bisectptr - 38 ];
	if ( bisectstack [bisectptr - 37 ]> 0 ) 
	bisectstack [bisectptr - 37 ]= 0 ;
      } 
      else {
	  
	bisectstack [bisectptr - 36 ]= bisectstack [bisectptr - 40 ]+ 
	bisectstack [bisectptr - 39 ]+ bisectstack [bisectptr - 38 ];
	if ( bisectstack [bisectptr - 36 ]< bisectstack [bisectptr - 40 ]) 
	bisectstack [bisectptr - 36 ]= bisectstack [bisectptr - 40 ];
	bisectstack [bisectptr - 37 ]= bisectstack [bisectptr - 40 ]+ 
	bisectstack [bisectptr - 39 ];
	if ( bisectstack [bisectptr - 37 ]> 0 ) 
	bisectstack [bisectptr - 37 ]= 0 ;
      } 
      if ( bisectstack [bisectptr - 20 ]< 0 ) 
      if ( bisectstack [bisectptr - 18 ]>= 0 ) 
      {
	if ( bisectstack [bisectptr - 19 ]< 0 ) 
	bisectstack [bisectptr - 17 ]= bisectstack [bisectptr - 20 ]+ 
	bisectstack [bisectptr - 19 ];
	else bisectstack [bisectptr - 17 ]= bisectstack [bisectptr - 20 ];
	bisectstack [bisectptr - 16 ]= bisectstack [bisectptr - 20 ]+ 
	bisectstack [bisectptr - 19 ]+ bisectstack [bisectptr - 18 ];
	if ( bisectstack [bisectptr - 16 ]< 0 ) 
	bisectstack [bisectptr - 16 ]= 0 ;
      } 
      else {
	  
	bisectstack [bisectptr - 17 ]= bisectstack [bisectptr - 20 ]+ 
	bisectstack [bisectptr - 19 ]+ bisectstack [bisectptr - 18 ];
	if ( bisectstack [bisectptr - 17 ]> bisectstack [bisectptr - 20 ]) 
	bisectstack [bisectptr - 17 ]= bisectstack [bisectptr - 20 ];
	bisectstack [bisectptr - 16 ]= bisectstack [bisectptr - 20 ]+ 
	bisectstack [bisectptr - 19 ];
	if ( bisectstack [bisectptr - 16 ]< 0 ) 
	bisectstack [bisectptr - 16 ]= 0 ;
      } 
      else if ( bisectstack [bisectptr - 18 ]<= 0 ) 
      {
	if ( bisectstack [bisectptr - 19 ]> 0 ) 
	bisectstack [bisectptr - 16 ]= bisectstack [bisectptr - 20 ]+ 
	bisectstack [bisectptr - 19 ];
	else bisectstack [bisectptr - 16 ]= bisectstack [bisectptr - 20 ];
	bisectstack [bisectptr - 17 ]= bisectstack [bisectptr - 20 ]+ 
	bisectstack [bisectptr - 19 ]+ bisectstack [bisectptr - 18 ];
	if ( bisectstack [bisectptr - 17 ]> 0 ) 
	bisectstack [bisectptr - 17 ]= 0 ;
      } 
      else {
	  
	bisectstack [bisectptr - 16 ]= bisectstack [bisectptr - 20 ]+ 
	bisectstack [bisectptr - 19 ]+ bisectstack [bisectptr - 18 ];
	if ( bisectstack [bisectptr - 16 ]< bisectstack [bisectptr - 20 ]) 
	bisectstack [bisectptr - 16 ]= bisectstack [bisectptr - 20 ];
	bisectstack [bisectptr - 17 ]= bisectstack [bisectptr - 20 ]+ 
	bisectstack [bisectptr - 19 ];
	if ( bisectstack [bisectptr - 17 ]> 0 ) 
	bisectstack [bisectptr - 17 ]= 0 ;
      } 
      uv = bisectptr - 20 ;
      xy = bisectptr - 20 ;
      delx = delx + delx ;
      dely = dely + dely ;
      tol = tol - threel + tolstep ;
      tol = tol + tol ;
      threel = threel + tolstep ;
      goto lab22 ;
    } 
    if ( timetogo > 0 ) 
    decr ( timetogo ) ;
    else {
	
      while ( apprt < 65536L ) {
	  
	apprt = apprt + apprt ;
	apprtt = apprtt + apprtt ;
      } 
      curt = apprt ;
      curtt = apprtt ;
      goto lab10 ;
    } 
    lab45: if ( odd ( curtt ) ) 
    if ( odd ( curt ) ) 
    {
      curt = halfp ( curt ) ;
      curtt = halfp ( curtt ) ;
      if ( curt == 0 ) 
      goto lab10 ;
      bisectptr = bisectptr - 45 ;
      threel = threel - tolstep ;
      delx = bisectstack [bisectptr ];
      dely = bisectstack [bisectptr + 1 ];
      tol = bisectstack [bisectptr + 2 ];
      uv = bisectstack [bisectptr + 3 ];
      xy = bisectstack [bisectptr + 4 ];
      goto lab45 ;
    } 
    else {
	
      incr ( curt ) ;
      delx = delx + bisectstack [uv - 5 ]+ bisectstack [uv - 4 ]+ 
      bisectstack [uv - 3 ];
      dely = dely + bisectstack [uv - 10 ]+ bisectstack [uv - 9 ]+ 
      bisectstack [uv - 8 ];
      uv = uv + 20 ;
      decr ( curtt ) ;
      xy = xy - 20 ;
      delx = delx + bisectstack [xy - 15 ]+ bisectstack [xy - 14 ]+ 
      bisectstack [xy - 13 ];
      dely = dely + bisectstack [xy - 20 ]+ bisectstack [xy - 19 ]+ 
      bisectstack [xy - 18 ];
    } 
    else {
	
      incr ( curtt ) ;
      tol = tol + threel ;
      delx = delx - bisectstack [xy - 15 ]- bisectstack [xy - 14 ]- 
      bisectstack [xy - 13 ];
      dely = dely - bisectstack [xy - 20 ]- bisectstack [xy - 19 ]- 
      bisectstack [xy - 18 ];
      xy = xy + 20 ;
    } 
  } 
  lab10: ;
} 
