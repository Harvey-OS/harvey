#define EXTERN extern
#include "texd.h"

void 
#ifdef HAVE_PROTOTYPES
initialize ( void ) 
#else
initialize ( ) 
#endif
{
  initialize_regmem 
  integer i  ;
  integer k  ;
  hyphpointer z  ;
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
  setboxallowed = true ;
  errorcount = 0 ;
  helpptr = 0 ;
  useerrhelp = false ;
  interrupt = 0 ;
  OKtointerrupt = true ;
	;
#ifdef TEXMF_DEBUG
  wasmemend = memmin ;
  waslomax = memmin ;
  washimin = memmax ;
  panicking = false ;
#endif /* TEXMF_DEBUG */
  nestptr = 0 ;
  maxneststack = 0 ;
  curlist .modefield = 1 ;
  curlist .headfield = memtop - 1 ;
  curlist .tailfield = memtop - 1 ;
  curlist .auxfield .cint = -65536000L ;
  curlist .mlfield = 0 ;
  curlist .pgfield = 0 ;
  shownmode = 0 ;
  pagecontents = 0 ;
  pagetail = memtop - 2 ;
  lastglue = 268435455L ;
  lastpenalty = 0 ;
  lastkern = 0 ;
  pagesofar [7 ]= 0 ;
  pagemaxdepth = 0 ;
  {register integer for_end; k = 15163 ;for_end = 16009 ; if ( k <= for_end) 
  do 
    xeqlevel [k ]= 1 ;
  while ( k++ < for_end ) ;} 
  nonewcontrolsequence = true ;
  saveptr = 0 ;
  curlevel = 1 ;
  curgroup = 0 ;
  curboundary = 0 ;
  maxsavestack = 0 ;
  magset = 0 ;
  curmark [0 ]= 0 ;
  curmark [1 ]= 0 ;
  curmark [2 ]= 0 ;
  curmark [3 ]= 0 ;
  curmark [4 ]= 0 ;
  curval = 0 ;
  curvallevel = 0 ;
  radix = 0 ;
  curorder = 0 ;
  {register integer for_end; k = 0 ;for_end = 16 ; if ( k <= for_end) do 
    readopen [k ]= 2 ;
  while ( k++ < for_end ) ;} 
  condptr = 0 ;
  iflimit = 0 ;
  curif = 0 ;
  ifline = 0 ;
  nullcharacter .b0 = 0 ;
  nullcharacter .b1 = 0 ;
  nullcharacter .b2 = 0 ;
  nullcharacter .b3 = 0 ;
  totalpages = 0 ;
  maxv = 0 ;
  maxh = 0 ;
  maxpush = 0 ;
  lastbop = -1 ;
  doingleaders = false ;
  deadcycles = 0 ;
  curs = -1 ;
  halfbuf = dvibufsize / 2 ;
  dvilimit = dvibufsize ;
  dviptr = 0 ;
  dvioffset = 0 ;
  dvigone = 0 ;
  downptr = 0 ;
  rightptr = 0 ;
  adjusttail = 0 ;
  lastbadness = 0 ;
  packbeginline = 0 ;
  emptyfield .v.RH = 0 ;
  emptyfield .v.LH = 0 ;
  nulldelimiter .b0 = 0 ;
  nulldelimiter .b1 = 0 ;
  nulldelimiter .b2 = 0 ;
  nulldelimiter .b3 = 0 ;
  alignptr = 0 ;
  curalign = 0 ;
  curspan = 0 ;
  curloop = 0 ;
  curhead = 0 ;
  curtail = 0 ;
  {register integer for_end; z = 0 ;for_end = hyphsize ; if ( z <= for_end) 
  do 
    {
      hyphword [z ]= 0 ;
      hyphlist [z ]= 0 ;
      hyphlink [z ]= 0 ;
    } 
  while ( z++ < for_end ) ;} 
  hyphcount = 0 ;
  hyphnext = 608 ;
  if ( hyphnext > hyphsize ) 
  hyphnext = 607 ;
  outputactive = false ;
  insertpenalties = 0 ;
  ligaturepresent = false ;
  cancelboundary = false ;
  lfthit = false ;
  rthit = false ;
  insdisc = false ;
  aftertoken = 0 ;
  longhelpseen = false ;
  formatident = 0 ;
  {register integer for_end; k = 0 ;for_end = 17 ; if ( k <= for_end) do 
    writeopen [k ]= false ;
  while ( k++ < for_end ) ;} 
  editnamestart = 0 ;
  mltexenabledp = false ;
	;
#ifdef INITEX
  if ( iniversion ) 
  {
    {register integer for_end; k = membot + 1 ;for_end = membot + 19 ; if ( 
    k <= for_end) do 
      mem [k ].cint = 0 ;
    while ( k++ < for_end ) ;} 
    k = membot ;
    while ( k <= membot + 19 ) {
	
      mem [k ].hh .v.RH = 1 ;
      mem [k ].hh.b0 = 0 ;
      mem [k ].hh.b1 = 0 ;
      k = k + 4 ;
    } 
    mem [membot + 6 ].cint = 65536L ;
    mem [membot + 4 ].hh.b0 = 1 ;
    mem [membot + 10 ].cint = 65536L ;
    mem [membot + 8 ].hh.b0 = 2 ;
    mem [membot + 14 ].cint = 65536L ;
    mem [membot + 12 ].hh.b0 = 1 ;
    mem [membot + 15 ].cint = 65536L ;
    mem [membot + 12 ].hh.b1 = 1 ;
    mem [membot + 18 ].cint = -65536L ;
    mem [membot + 16 ].hh.b0 = 1 ;
    rover = membot + 20 ;
    mem [rover ].hh .v.RH = 268435455L ;
    mem [rover ].hh .v.LH = 1000 ;
    mem [rover + 1 ].hh .v.LH = rover ;
    mem [rover + 1 ].hh .v.RH = rover ;
    lomemmax = rover + 1000 ;
    mem [lomemmax ].hh .v.RH = 0 ;
    mem [lomemmax ].hh .v.LH = 0 ;
    {register integer for_end; k = memtop - 13 ;for_end = memtop ; if ( k <= 
    for_end) do 
      mem [k ]= mem [lomemmax ];
    while ( k++ < for_end ) ;} 
    mem [memtop - 10 ].hh .v.LH = 14614 ;
    mem [memtop - 9 ].hh .v.RH = 256 ;
    mem [memtop - 9 ].hh .v.LH = 0 ;
    mem [memtop - 7 ].hh.b0 = 1 ;
    mem [memtop - 6 ].hh .v.LH = 268435455L ;
    mem [memtop - 7 ].hh.b1 = 0 ;
    mem [memtop ].hh.b1 = 255 ;
    mem [memtop ].hh.b0 = 1 ;
    mem [memtop ].hh .v.RH = memtop ;
    mem [memtop - 2 ].hh.b0 = 10 ;
    mem [memtop - 2 ].hh.b1 = 0 ;
    avail = 0 ;
    memend = memtop ;
    himemmin = memtop - 13 ;
    varused = membot + 20 - membot ;
    dynused = 14 ;
    eqtb [12525 ].hh.b0 = 101 ;
    eqtb [12525 ].hh .v.RH = 0 ;
    eqtb [12525 ].hh.b1 = 0 ;
    {register integer for_end; k = 1 ;for_end = eqtbtop ; if ( k <= for_end) 
    do 
      eqtb [k ]= eqtb [12525 ];
    while ( k++ < for_end ) ;} 
    eqtb [12526 ].hh .v.RH = membot ;
    eqtb [12526 ].hh.b1 = 1 ;
    eqtb [12526 ].hh.b0 = 117 ;
    {register integer for_end; k = 12527 ;for_end = 13055 ; if ( k <= 
    for_end) do 
      eqtb [k ]= eqtb [12526 ];
    while ( k++ < for_end ) ;} 
    mem [membot ].hh .v.RH = mem [membot ].hh .v.RH + 530 ;
    eqtb [13056 ].hh .v.RH = 0 ;
    eqtb [13056 ].hh.b0 = 118 ;
    eqtb [13056 ].hh.b1 = 1 ;
    {register integer for_end; k = 13057 ;for_end = 13321 ; if ( k <= 
    for_end) do 
      eqtb [k ]= eqtb [12525 ];
    while ( k++ < for_end ) ;} 
    eqtb [13322 ].hh .v.RH = 0 ;
    eqtb [13322 ].hh.b0 = 119 ;
    eqtb [13322 ].hh.b1 = 1 ;
    {register integer for_end; k = 13323 ;for_end = 13577 ; if ( k <= 
    for_end) do 
      eqtb [k ]= eqtb [13322 ];
    while ( k++ < for_end ) ;} 
    eqtb [13578 ].hh .v.RH = 0 ;
    eqtb [13578 ].hh.b0 = 120 ;
    eqtb [13578 ].hh.b1 = 1 ;
    {register integer for_end; k = 13579 ;for_end = 13626 ; if ( k <= 
    for_end) do 
      eqtb [k ]= eqtb [13578 ];
    while ( k++ < for_end ) ;} 
    eqtb [13627 ].hh .v.RH = 0 ;
    eqtb [13627 ].hh.b0 = 120 ;
    eqtb [13627 ].hh.b1 = 1 ;
    {register integer for_end; k = 13628 ;for_end = 15162 ; if ( k <= 
    for_end) do 
      eqtb [k ]= eqtb [13627 ];
    while ( k++ < for_end ) ;} 
    {register integer for_end; k = 0 ;for_end = 255 ; if ( k <= for_end) do 
      {
	eqtb [13627 + k ].hh .v.RH = 12 ;
	eqtb [14651 + k ].hh .v.RH = k ;
	eqtb [14395 + k ].hh .v.RH = 1000 ;
      } 
    while ( k++ < for_end ) ;} 
    eqtb [13640 ].hh .v.RH = 5 ;
    eqtb [13659 ].hh .v.RH = 10 ;
    eqtb [13719 ].hh .v.RH = 0 ;
    eqtb [13664 ].hh .v.RH = 14 ;
    eqtb [13754 ].hh .v.RH = 15 ;
    eqtb [13627 ].hh .v.RH = 9 ;
    {register integer for_end; k = 48 ;for_end = 57 ; if ( k <= for_end) do 
      eqtb [14651 + k ].hh .v.RH = k + 28672 ;
    while ( k++ < for_end ) ;} 
    {register integer for_end; k = 65 ;for_end = 90 ; if ( k <= for_end) do 
      {
	eqtb [13627 + k ].hh .v.RH = 11 ;
	eqtb [13627 + k + 32 ].hh .v.RH = 11 ;
	eqtb [14651 + k ].hh .v.RH = k + 28928 ;
	eqtb [14651 + k + 32 ].hh .v.RH = k + 28960 ;
	eqtb [13883 + k ].hh .v.RH = k + 32 ;
	eqtb [13883 + k + 32 ].hh .v.RH = k + 32 ;
	eqtb [14139 + k ].hh .v.RH = k ;
	eqtb [14139 + k + 32 ].hh .v.RH = k ;
	eqtb [14395 + k ].hh .v.RH = 999 ;
      } 
    while ( k++ < for_end ) ;} 
    {register integer for_end; k = 15163 ;for_end = 15476 ; if ( k <= 
    for_end) do 
      eqtb [k ].cint = 0 ;
    while ( k++ < for_end ) ;} 
    eqtb [15218 ].cint = 256 ;
    eqtb [15219 ].cint = -1 ;
    eqtb [15180 ].cint = 1000 ;
    eqtb [15164 ].cint = 10000 ;
    eqtb [15204 ].cint = 1 ;
    eqtb [15203 ].cint = 25 ;
    eqtb [15208 ].cint = 92 ;
    eqtb [15211 ].cint = 13 ;
    {register integer for_end; k = 0 ;for_end = 255 ; if ( k <= for_end) do 
      eqtb [15477 + k ].cint = -1 ;
    while ( k++ < for_end ) ;} 
    eqtb [15523 ].cint = 0 ;
    {register integer for_end; k = 15733 ;for_end = 16009 ; if ( k <= 
    for_end) do 
      eqtb [k ].cint = 0 ;
    while ( k++ < for_end ) ;} 
    hashused = 10514 ;
    hashhigh = 0 ;
    cscount = 0 ;
    eqtb [10523 ].hh.b0 = 116 ;
    hash [10523 ].v.RH = 502 ;
    {register integer for_end; k = - (integer) trieopsize ;for_end = 
    trieopsize ; if ( k <= for_end) do 
      trieophash [k ]= 0 ;
    while ( k++ < for_end ) ;} 
    {register integer for_end; k = 0 ;for_end = 255 ; if ( k <= for_end) do 
      trieused [k ]= mintrieop ;
    while ( k++ < for_end ) ;} 
    maxopused = mintrieop ;
    trieopptr = 0 ;
    trienotready = true ;
    hash [10514 ].v.RH = 1185 ;
    if ( iniversion ) 
    formatident = 1255 ;
    hash [10522 ].v.RH = 1294 ;
    eqtb [10522 ].hh.b1 = 1 ;
    eqtb [10522 ].hh.b0 = 113 ;
    eqtb [10522 ].hh .v.RH = 0 ;
  } 
#endif /* INITEX */
} 
#ifdef INITEX
boolean 
#ifdef HAVE_PROTOTYPES
getstringsstarted ( void ) 
#else
getstringsstarted ( ) 
#endif
{
  /* 30 10 */ register boolean Result; getstringsstarted_regmem 
  unsigned char k, l  ;
  ASCIIcode m, n  ;
  strnumber g  ;
  integer a  ;
  boolean c  ;
  poolptr = 0 ;
  strptr = 0 ;
  strstart [0 ]= 0 ;
  {register integer for_end; k = 0 ;for_end = 255 ; if ( k <= for_end) do 
    {
      {
	strpool [poolptr ]= k ;
	incr ( poolptr ) ;
      } 
      g = makestring () ;
    } 
  while ( k++ < for_end ) ;} 
  namelength = strlen ( poolname ) ;
  nameoffile = xmalloc ( 1 + namelength + 1 ) ;
  strcpy ( (char*) nameoffile + 1 , poolname ) ;
  if ( aopenin ( poolfile , kpsetexpoolformat ) ) 
  {
    c = false ;
    do {
	{ 
	if ( eof ( poolfile ) ) 
	{
	  ;
	  fprintf( stdout , "%s\n",  "! tex.pool has no check sum." ) ;
	  aclose ( poolfile ) ;
	  Result = false ;
	  return Result ;
	} 
	read ( poolfile , m ) ;
	read ( poolfile , n ) ;
	if ( m == '*' ) 
	{
	  a = 0 ;
	  k = 1 ;
	  while ( true ) {
	      
	    if ( ( xord [n ]< 48 ) || ( xord [n ]> 57 ) ) 
	    {
	      ;
	      fprintf( stdout , "%s\n",                "! tex.pool check sum doesn't have nine digits." ) ;
	      aclose ( poolfile ) ;
	      Result = false ;
	      return Result ;
	    } 
	    a = 10 * a + xord [n ]- 48 ;
	    if ( k == 9 ) 
	    goto lab30 ;
	    incr ( k ) ;
	    read ( poolfile , n ) ;
	  } 
	  lab30: if ( a != 294434737L ) 
	  {
	    ;
	    fprintf( stdout , "%s\n",              "! tex.pool doesn't match; tangle me again (or fix the path)." ) ;
	    aclose ( poolfile ) ;
	    Result = false ;
	    return Result ;
	  } 
	  c = true ;
	} 
	else {
	    
	  if ( ( xord [m ]< 48 ) || ( xord [m ]> 57 ) || ( xord [n ]< 48 
	  ) || ( xord [n ]> 57 ) ) 
	  {
	    ;
	    fprintf( stdout , "%s\n",              "! tex.pool line doesn't begin with two digits." ) ;
	    aclose ( poolfile ) ;
	    Result = false ;
	    return Result ;
	  } 
	  l = xord [m ]* 10 + xord [n ]- 48 * 11 ;
	  if ( poolptr + l + stringvacancies > poolsize ) 
	  {
	    ;
	    fprintf( stdout , "%s\n",  "! You have to increase POOLSIZE." ) ;
	    aclose ( poolfile ) ;
	    Result = false ;
	    return Result ;
	  } 
	  {register integer for_end; k = 1 ;for_end = l ; if ( k <= for_end) 
	  do 
	    {
	      if ( eoln ( poolfile ) ) 
	      m = ' ' ;
	      else read ( poolfile , m ) ;
	      {
		strpool [poolptr ]= xord [m ];
		incr ( poolptr ) ;
	      } 
	    } 
	  while ( k++ < for_end ) ;} 
	  readln ( poolfile ) ;
	  g = makestring () ;
	} 
      } 
    } while ( ! ( c ) ) ;
    aclose ( poolfile ) ;
    Result = true ;
  } 
  else {
      
    ;
    fprintf( stdout , "%s\n",  "! I can't read tex.pool; bad path?" ) ;
    aclose ( poolfile ) ;
    Result = false ;
    return Result ;
  } 
  return Result ;
} 
#endif /* INITEX */
#ifdef INITEX
void 
#ifdef HAVE_PROTOTYPES
sortavail ( void ) 
#else
sortavail ( ) 
#endif
{
  sortavail_regmem 
  halfword p, q, r  ;
  halfword oldrover  ;
  p = getnode ( 1073741824L ) ;
  p = mem [rover + 1 ].hh .v.RH ;
  mem [rover + 1 ].hh .v.RH = 268435455L ;
  oldrover = rover ;
  while ( p != oldrover ) if ( p < rover ) 
  {
    q = p ;
    p = mem [q + 1 ].hh .v.RH ;
    mem [q + 1 ].hh .v.RH = rover ;
    rover = q ;
  } 
  else {
      
    q = rover ;
    while ( mem [q + 1 ].hh .v.RH < p ) q = mem [q + 1 ].hh .v.RH ;
    r = mem [p + 1 ].hh .v.RH ;
    mem [p + 1 ].hh .v.RH = mem [q + 1 ].hh .v.RH ;
    mem [q + 1 ].hh .v.RH = p ;
    p = r ;
  } 
  p = rover ;
  while ( mem [p + 1 ].hh .v.RH != 268435455L ) {
      
    mem [mem [p + 1 ].hh .v.RH + 1 ].hh .v.LH = p ;
    p = mem [p + 1 ].hh .v.RH ;
  } 
  mem [p + 1 ].hh .v.RH = rover ;
  mem [rover + 1 ].hh .v.LH = p ;
} 
#endif /* INITEX */
#ifdef INITEX
void 
#ifdef HAVE_PROTOTYPES
zprimitive ( strnumber s , quarterword c , halfword o ) 
#else
zprimitive ( s , c , o ) 
  strnumber s ;
  quarterword c ;
  halfword o ;
#endif
{
  primitive_regmem 
  poolpointer k  ;
  smallnumber j  ;
  smallnumber l  ;
  if ( s < 256 ) 
  curval = s + 257 ;
  else {
      
    k = strstart [s ];
    l = strstart [s + 1 ]- k ;
    {register integer for_end; j = 0 ;for_end = l - 1 ; if ( j <= for_end) 
    do 
      buffer [j ]= strpool [k + j ];
    while ( j++ < for_end ) ;} 
    curval = idlookup ( 0 , l ) ;
    {
      decr ( strptr ) ;
      poolptr = strstart [strptr ];
    } 
    hash [curval ].v.RH = s ;
  } 
  eqtb [curval ].hh.b1 = 1 ;
  eqtb [curval ].hh.b0 = c ;
  eqtb [curval ].hh .v.RH = o ;
} 
#endif /* INITEX */
void 
#ifdef HAVE_PROTOTYPES
startinput ( void ) 
#else
startinput ( ) 
#endif
{
  /* 30 */ startinput_regmem 
  strnumber tempstr  ;
  integer k  ;
  scanfilename () ;
  packfilename ( curname , curarea , curext ) ;
  while ( true ) {
      
    beginfilereading () ;
    texinputtype = 1 ;
    if ( aopenin ( inputfile [curinput .indexfield ], kpsetexformat ) ) 
    {
      k = 1 ;
      nameinprogress = true ;
      beginname () ;
      while ( ( k <= namelength ) && ( morename ( (char*) nameoffile [k ]) ) ) incr 
      ( k ) ;
      endname () ;
      nameinprogress = false ;
      goto lab30 ;
    } 
    endfilereading () ;
    promptfilename ( 783 , 335 ) ;
  } 
  lab30: curinput .namefield = amakenamestring ( inputfile [curinput 
  .indexfield ]) ;
  if ( curinput .namefield == strptr - 1 ) 
  {
    tempstr = searchstring ( curinput .namefield ) ;
    if ( tempstr > 0 ) 
    {
      curinput .namefield = tempstr ;
      {
	decr ( strptr ) ;
	poolptr = strstart [strptr ];
      } 
    } 
  } 
  if ( jobname == 0 ) 
  {
    jobname = curname ;
	;
#ifdef INITEX
    if ( iniversion ) 
    {
      if ( dumpoption ) 
      {
	{
	  if ( poolptr + formatdefaultlength > poolsize ) 
	  overflow ( 257 , poolsize - initpoolptr ) ;
	} 
	{register integer for_end; k = 1 ;for_end = formatdefaultlength - 4 
	; if ( k <= for_end) do 
	  {
	    strpool [poolptr ]= xord [TEXformatdefault [k ]];
	    incr ( poolptr ) ;
	  } 
	while ( k++ < for_end ) ;} 
	jobname = makestring () ;
      } 
    } 
#endif /* INITEX */
    openlogfile () ;
  } 
  if ( termoffset + ( strstart [curinput .namefield + 1 ]- strstart [
  curinput .namefield ]) > maxprintline - 2 ) 
  println () ;
  else if ( ( termoffset > 0 ) || ( fileoffset > 0 ) ) 
  printchar ( 32 ) ;
  printchar ( 40 ) ;
  incr ( openparens ) ;
  print ( curinput .namefield ) ;
  fflush ( stdout ) ;
  curinput .statefield = 33 ;
  {
    line = 1 ;
    if ( inputln ( inputfile [curinput .indexfield ], false ) ) 
    ;
    firmuptheline () ;
    if ( ( eqtb [15211 ].cint < 0 ) || ( eqtb [15211 ].cint > 255 ) ) 
    decr ( curinput .limitfield ) ;
    else buffer [curinput .limitfield ]= eqtb [15211 ].cint ;
    first = curinput .limitfield + 1 ;
    curinput .locfield = curinput .startfield ;
  } 
} 
#ifdef INITEX
trieopcode 
#ifdef HAVE_PROTOTYPES
znewtrieop ( smallnumber d , smallnumber n , trieopcode v ) 
#else
znewtrieop ( d , n , v ) 
  smallnumber d ;
  smallnumber n ;
  trieopcode v ;
#endif
{
  /* 10 */ register trieopcode Result; newtrieop_regmem 
  integer h  ;
  trieopcode u  ;
  integer l  ;
  h = abs ( toint ( n ) + 313 * toint ( d ) + 361 * toint ( v ) + 1009 * toint 
  ( curlang ) ) % ( trieopsize - negtrieopsize ) + negtrieopsize ;
  while ( true ) {
      
    l = trieophash [h ];
    if ( l == 0 ) 
    {
      if ( trieopptr == trieopsize ) 
      overflow ( 944 , trieopsize ) ;
      u = trieused [curlang ];
      if ( u == maxtrieop ) 
      overflow ( 945 , maxtrieop - mintrieop ) ;
      incr ( trieopptr ) ;
      incr ( u ) ;
      trieused [curlang ]= u ;
      if ( u > maxopused ) 
      maxopused = u ;
      hyfdistance [trieopptr ]= d ;
      hyfnum [trieopptr ]= n ;
      hyfnext [trieopptr ]= v ;
      trieoplang [trieopptr ]= curlang ;
      trieophash [h ]= trieopptr ;
      trieopval [trieopptr ]= u ;
      Result = u ;
      return Result ;
    } 
    if ( ( hyfdistance [l ]== d ) && ( hyfnum [l ]== n ) && ( hyfnext [l 
    ]== v ) && ( trieoplang [l ]== curlang ) ) 
    {
      Result = trieopval [l ];
      return Result ;
    } 
    if ( h > - (integer) trieopsize ) 
    decr ( h ) ;
    else h = trieopsize ;
  } 
  return Result ;
} 
triepointer 
#ifdef HAVE_PROTOTYPES
ztrienode ( triepointer p ) 
#else
ztrienode ( p ) 
  triepointer p ;
#endif
{
  /* 10 */ register triepointer Result; trienode_regmem 
  triepointer h  ;
  triepointer q  ;
  h = abs ( toint ( triec [p ]) + 1009 * toint ( trieo [p ]) + 2718 * 
  toint ( triel [p ]) + 3142 * toint ( trier [p ]) ) % triesize ;
  while ( true ) {
      
    q = triehash [h ];
    if ( q == 0 ) 
    {
      triehash [h ]= p ;
      Result = p ;
      return Result ;
    } 
    if ( ( triec [q ]== triec [p ]) && ( trieo [q ]== trieo [p ]) && ( 
    triel [q ]== triel [p ]) && ( trier [q ]== trier [p ]) ) 
    {
      Result = q ;
      return Result ;
    } 
    if ( h > 0 ) 
    decr ( h ) ;
    else h = triesize ;
  } 
  return Result ;
} 
triepointer 
#ifdef HAVE_PROTOTYPES
zcompresstrie ( triepointer p ) 
#else
zcompresstrie ( p ) 
  triepointer p ;
#endif
{
  register triepointer Result; compresstrie_regmem 
  if ( p == 0 ) 
  Result = 0 ;
  else {
      
    triel [p ]= compresstrie ( triel [p ]) ;
    trier [p ]= compresstrie ( trier [p ]) ;
    Result = trienode ( p ) ;
  } 
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zfirstfit ( triepointer p ) 
#else
zfirstfit ( p ) 
  triepointer p ;
#endif
{
  /* 45 40 */ firstfit_regmem 
  triepointer h  ;
  triepointer z  ;
  triepointer q  ;
  ASCIIcode c  ;
  triepointer l, r  ;
  short ll  ;
  c = triec [p ];
  z = triemin [c ];
  while ( true ) {
      
    h = z - c ;
    if ( triemax < h + 256 ) 
    {
      if ( triesize <= h + 256 ) 
      overflow ( 946 , triesize ) ;
      do {
	  incr ( triemax ) ;
	trietaken [triemax ]= false ;
	trietrl [triemax ]= triemax + 1 ;
	trietro [triemax ]= triemax - 1 ;
      } while ( ! ( triemax == h + 256 ) ) ;
    } 
    if ( trietaken [h ]) 
    goto lab45 ;
    q = trier [p ];
    while ( q > 0 ) {
	
      if ( trietrl [h + triec [q ]]== 0 ) 
      goto lab45 ;
      q = trier [q ];
    } 
    goto lab40 ;
    lab45: z = trietrl [z ];
  } 
  lab40: trietaken [h ]= true ;
  triehash [p ]= h ;
  q = p ;
  do {
      z = h + triec [q ];
    l = trietro [z ];
    r = trietrl [z ];
    trietro [r ]= l ;
    trietrl [l ]= r ;
    trietrl [z ]= 0 ;
    if ( l < 256 ) 
    {
      if ( z < 256 ) 
      ll = z ;
      else ll = 256 ;
      do {
	  triemin [l ]= r ;
	incr ( l ) ;
      } while ( ! ( l == ll ) ) ;
    } 
    q = trier [q ];
  } while ( ! ( q == 0 ) ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
ztriepack ( triepointer p ) 
#else
ztriepack ( p ) 
  triepointer p ;
#endif
{
  triepack_regmem 
  triepointer q  ;
  do {
      q = triel [p ];
    if ( ( q > 0 ) && ( triehash [q ]== 0 ) ) 
    {
      firstfit ( q ) ;
      triepack ( q ) ;
    } 
    p = trier [p ];
  } while ( ! ( p == 0 ) ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
ztriefix ( triepointer p ) 
#else
ztriefix ( p ) 
  triepointer p ;
#endif
{
  triefix_regmem 
  triepointer q  ;
  ASCIIcode c  ;
  triepointer z  ;
  z = triehash [p ];
  do {
      q = triel [p ];
    c = triec [p ];
    trietrl [z + c ]= triehash [q ];
    trietrc [z + c ]= c ;
    trietro [z + c ]= trieo [p ];
    if ( q > 0 ) 
    triefix ( q ) ;
    p = trier [p ];
  } while ( ! ( p == 0 ) ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
newpatterns ( void ) 
#else
newpatterns ( ) 
#endif
{
  /* 30 31 */ newpatterns_regmem 
  char k, l  ;
  boolean digitsensed  ;
  trieopcode v  ;
  triepointer p, q  ;
  boolean firstchild  ;
  ASCIIcode c  ;
  if ( trienotready ) 
  {
    if ( eqtb [15213 ].cint <= 0 ) 
    curlang = 0 ;
    else if ( eqtb [15213 ].cint > 255 ) 
    curlang = 0 ;
    else curlang = eqtb [15213 ].cint ;
    scanleftbrace () ;
    k = 0 ;
    hyf [0 ]= 0 ;
    digitsensed = false ;
    while ( true ) {
	
      getxtoken () ;
      switch ( curcmd ) 
      {case 11 : 
      case 12 : 
	if ( digitsensed || ( curchr < 48 ) || ( curchr > 57 ) ) 
	{
	  if ( curchr == 46 ) 
	  curchr = 0 ;
	  else {
	      
	    curchr = eqtb [13883 + curchr ].hh .v.RH ;
	    if ( curchr == 0 ) 
	    {
	      {
		if ( interaction == 3 ) 
		;
		printnl ( 262 ) ;
		print ( 952 ) ;
	      } 
	      {
		helpptr = 1 ;
		helpline [0 ]= 951 ;
	      } 
	      error () ;
	    } 
	  } 
	  if ( k < 63 ) 
	  {
	    incr ( k ) ;
	    hc [k ]= curchr ;
	    hyf [k ]= 0 ;
	    digitsensed = false ;
	  } 
	} 
	else if ( k < 63 ) 
	{
	  hyf [k ]= curchr - 48 ;
	  digitsensed = true ;
	} 
	break ;
      case 10 : 
      case 2 : 
	{
	  if ( k > 0 ) 
	  {
	    if ( hc [1 ]== 0 ) 
	    hyf [0 ]= 0 ;
	    if ( hc [k ]== 0 ) 
	    hyf [k ]= 0 ;
	    l = k ;
	    v = mintrieop ;
	    while ( true ) {
		
	      if ( hyf [l ]!= 0 ) 
	      v = newtrieop ( k - l , hyf [l ], v ) ;
	      if ( l > 0 ) 
	      decr ( l ) ;
	      else goto lab31 ;
	    } 
	    lab31: ;
	    q = 0 ;
	    hc [0 ]= curlang ;
	    while ( l <= k ) {
		
	      c = hc [l ];
	      incr ( l ) ;
	      p = triel [q ];
	      firstchild = true ;
	      while ( ( p > 0 ) && ( c > triec [p ]) ) {
		  
		q = p ;
		p = trier [q ];
		firstchild = false ;
	      } 
	      if ( ( p == 0 ) || ( c < triec [p ]) ) 
	      {
		if ( trieptr == triesize ) 
		overflow ( 946 , triesize ) ;
		incr ( trieptr ) ;
		trier [trieptr ]= p ;
		p = trieptr ;
		triel [p ]= 0 ;
		if ( firstchild ) 
		triel [q ]= p ;
		else trier [q ]= p ;
		triec [p ]= c ;
		trieo [p ]= mintrieop ;
	      } 
	      q = p ;
	    } 
	    if ( trieo [q ]!= mintrieop ) 
	    {
	      {
		if ( interaction == 3 ) 
		;
		printnl ( 262 ) ;
		print ( 953 ) ;
	      } 
	      {
		helpptr = 1 ;
		helpline [0 ]= 951 ;
	      } 
	      error () ;
	    } 
	    trieo [q ]= v ;
	  } 
	  if ( curcmd == 2 ) 
	  goto lab30 ;
	  k = 0 ;
	  hyf [0 ]= 0 ;
	  digitsensed = false ;
	} 
	break ;
	default: 
	{
	  {
	    if ( interaction == 3 ) 
	    ;
	    printnl ( 262 ) ;
	    print ( 950 ) ;
	  } 
	  printesc ( 948 ) ;
	  {
	    helpptr = 1 ;
	    helpline [0 ]= 951 ;
	  } 
	  error () ;
	} 
	break ;
      } 
    } 
    lab30: ;
  } 
  else {
      
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 947 ) ;
    } 
    printesc ( 948 ) ;
    {
      helpptr = 1 ;
      helpline [0 ]= 949 ;
    } 
    error () ;
    mem [memtop - 12 ].hh .v.RH = scantoks ( false , false ) ;
    flushlist ( defref ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
inittrie ( void ) 
#else
inittrie ( ) 
#endif
{
  inittrie_regmem 
  triepointer p  ;
  integer j, k, t  ;
  triepointer r, s  ;
  opstart [0 ]= - (integer) mintrieop ;
  {register integer for_end; j = 1 ;for_end = 255 ; if ( j <= for_end) do 
    opstart [j ]= opstart [j - 1 ]+ trieused [j - 1 ];
  while ( j++ < for_end ) ;} 
  {register integer for_end; j = 1 ;for_end = trieopptr ; if ( j <= for_end) 
  do 
    trieophash [j ]= opstart [trieoplang [j ]]+ trieopval [j ];
  while ( j++ < for_end ) ;} 
  {register integer for_end; j = 1 ;for_end = trieopptr ; if ( j <= for_end) 
  do 
    while ( trieophash [j ]> j ) {
	
      k = trieophash [j ];
      t = hyfdistance [k ];
      hyfdistance [k ]= hyfdistance [j ];
      hyfdistance [j ]= t ;
      t = hyfnum [k ];
      hyfnum [k ]= hyfnum [j ];
      hyfnum [j ]= t ;
      t = hyfnext [k ];
      hyfnext [k ]= hyfnext [j ];
      hyfnext [j ]= t ;
      trieophash [j ]= trieophash [k ];
      trieophash [k ]= k ;
    } 
  while ( j++ < for_end ) ;} 
  {register integer for_end; p = 0 ;for_end = triesize ; if ( p <= for_end) 
  do 
    triehash [p ]= 0 ;
  while ( p++ < for_end ) ;} 
  triel [0 ]= compresstrie ( triel [0 ]) ;
  {register integer for_end; p = 0 ;for_end = trieptr ; if ( p <= for_end) 
  do 
    triehash [p ]= 0 ;
  while ( p++ < for_end ) ;} 
  {register integer for_end; p = 0 ;for_end = 255 ; if ( p <= for_end) do 
    triemin [p ]= p + 1 ;
  while ( p++ < for_end ) ;} 
  trietrl [0 ]= 1 ;
  triemax = 0 ;
  if ( triel [0 ]!= 0 ) 
  {
    firstfit ( triel [0 ]) ;
    triepack ( triel [0 ]) ;
  } 
  if ( triel [0 ]== 0 ) 
  {
    {register integer for_end; r = 0 ;for_end = 256 ; if ( r <= for_end) do 
      {
	trietrl [r ]= 0 ;
	trietro [r ]= mintrieop ;
	trietrc [r ]= 0 ;
      } 
    while ( r++ < for_end ) ;} 
    triemax = 256 ;
  } 
  else {
      
    triefix ( triel [0 ]) ;
    r = 0 ;
    do {
	s = trietrl [r ];
      {
	trietrl [r ]= 0 ;
	trietro [r ]= mintrieop ;
	trietrc [r ]= 0 ;
      } 
      r = s ;
    } while ( ! ( r > triemax ) ) ;
  } 
  trietrc [0 ]= 63 ;
  trienotready = false ;
} 
#endif /* INITEX */
void 
#ifdef HAVE_PROTOTYPES
zlinebreak ( integer finalwidowpenalty ) 
#else
zlinebreak ( finalwidowpenalty ) 
  integer finalwidowpenalty ;
#endif
{
  /* 30 31 32 33 34 35 22 */ linebreak_regmem 
  boolean autobreaking  ;
  halfword prevp  ;
  halfword q, r, s, prevs  ;
  internalfontnumber f  ;
  smallnumber j  ;
  unsigned char c  ;
  packbeginline = curlist .mlfield ;
  mem [memtop - 3 ].hh .v.RH = mem [curlist .headfield ].hh .v.RH ;
  if ( ( curlist .tailfield >= himemmin ) ) 
  {
    mem [curlist .tailfield ].hh .v.RH = newpenalty ( 10000 ) ;
    curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
  } 
  else if ( mem [curlist .tailfield ].hh.b0 != 10 ) 
  {
    mem [curlist .tailfield ].hh .v.RH = newpenalty ( 10000 ) ;
    curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
  } 
  else {
      
    mem [curlist .tailfield ].hh.b0 = 12 ;
    deleteglueref ( mem [curlist .tailfield + 1 ].hh .v.LH ) ;
    flushnodelist ( mem [curlist .tailfield + 1 ].hh .v.RH ) ;
    mem [curlist .tailfield + 1 ].cint = 10000 ;
  } 
  mem [curlist .tailfield ].hh .v.RH = newparamglue ( 14 ) ;
  initcurlang = curlist .pgfield % 65536L ;
  initlhyf = curlist .pgfield / 4194304L ;
  initrhyf = ( curlist .pgfield / 65536L ) % 64 ;
  popnest () ;
  noshrinkerroryet = true ;
  if ( ( mem [eqtb [12533 ].hh .v.RH ].hh.b1 != 0 ) && ( mem [eqtb [
  12533 ].hh .v.RH + 3 ].cint != 0 ) ) 
  {
    eqtb [12533 ].hh .v.RH = finiteshrink ( eqtb [12533 ].hh .v.RH ) ;
  } 
  if ( ( mem [eqtb [12534 ].hh .v.RH ].hh.b1 != 0 ) && ( mem [eqtb [
  12534 ].hh .v.RH + 3 ].cint != 0 ) ) 
  {
    eqtb [12534 ].hh .v.RH = finiteshrink ( eqtb [12534 ].hh .v.RH ) ;
  } 
  q = eqtb [12533 ].hh .v.RH ;
  r = eqtb [12534 ].hh .v.RH ;
  background [1 ]= mem [q + 1 ].cint + mem [r + 1 ].cint ;
  background [2 ]= 0 ;
  background [3 ]= 0 ;
  background [4 ]= 0 ;
  background [5 ]= 0 ;
  background [2 + mem [q ].hh.b0 ]= mem [q + 2 ].cint ;
  background [2 + mem [r ].hh.b0 ]= background [2 + mem [r ].hh.b0 ]+ 
  mem [r + 2 ].cint ;
  background [6 ]= mem [q + 3 ].cint + mem [r + 3 ].cint ;
  minimumdemerits = 1073741823L ;
  minimaldemerits [3 ]= 1073741823L ;
  minimaldemerits [2 ]= 1073741823L ;
  minimaldemerits [1 ]= 1073741823L ;
  minimaldemerits [0 ]= 1073741823L ;
  if ( eqtb [13056 ].hh .v.RH == 0 ) 
  if ( eqtb [15750 ].cint == 0 ) 
  {
    lastspecialline = 0 ;
    secondwidth = eqtb [15736 ].cint ;
    secondindent = 0 ;
  } 
  else {
      
    lastspecialline = abs ( eqtb [15204 ].cint ) ;
    if ( eqtb [15204 ].cint < 0 ) 
    {
      firstwidth = eqtb [15736 ].cint - abs ( eqtb [15750 ].cint ) ;
      if ( eqtb [15750 ].cint >= 0 ) 
      firstindent = eqtb [15750 ].cint ;
      else firstindent = 0 ;
      secondwidth = eqtb [15736 ].cint ;
      secondindent = 0 ;
    } 
    else {
	
      firstwidth = eqtb [15736 ].cint ;
      firstindent = 0 ;
      secondwidth = eqtb [15736 ].cint - abs ( eqtb [15750 ].cint ) ;
      if ( eqtb [15750 ].cint >= 0 ) 
      secondindent = eqtb [15750 ].cint ;
      else secondindent = 0 ;
    } 
  } 
  else {
      
    lastspecialline = mem [eqtb [13056 ].hh .v.RH ].hh .v.LH - 1 ;
    secondwidth = mem [eqtb [13056 ].hh .v.RH + 2 * ( lastspecialline + 1 ) 
    ].cint ;
    secondindent = mem [eqtb [13056 ].hh .v.RH + 2 * lastspecialline + 1 ]
    .cint ;
  } 
  if ( eqtb [15182 ].cint == 0 ) 
  easyline = lastspecialline ;
  else easyline = 268435455L ;
  threshold = eqtb [15163 ].cint ;
  if ( threshold >= 0 ) 
  {
	;
#ifdef STAT
    if ( eqtb [15195 ].cint > 0 ) 
    {
      begindiagnostic () ;
      printnl ( 928 ) ;
    } 
#endif /* STAT */
    secondpass = false ;
    finalpass = false ;
  } 
  else {
      
    threshold = eqtb [15164 ].cint ;
    secondpass = true ;
    finalpass = ( eqtb [15753 ].cint <= 0 ) ;
	;
#ifdef STAT
    if ( eqtb [15195 ].cint > 0 ) 
    begindiagnostic () ;
#endif /* STAT */
  } 
  while ( true ) {
      
    if ( threshold > 10000 ) 
    threshold = 10000 ;
    if ( secondpass ) 
    {
	;
#ifdef INITEX
      if ( trienotready ) 
      inittrie () ;
#endif /* INITEX */
      curlang = initcurlang ;
      lhyf = initlhyf ;
      rhyf = initrhyf ;
    } 
    q = getnode ( 3 ) ;
    mem [q ].hh.b0 = 0 ;
    mem [q ].hh.b1 = 2 ;
    mem [q ].hh .v.RH = memtop - 7 ;
    mem [q + 1 ].hh .v.RH = 0 ;
    mem [q + 1 ].hh .v.LH = curlist .pgfield + 1 ;
    mem [q + 2 ].cint = 0 ;
    mem [memtop - 7 ].hh .v.RH = q ;
    activewidth [1 ]= background [1 ];
    activewidth [2 ]= background [2 ];
    activewidth [3 ]= background [3 ];
    activewidth [4 ]= background [4 ];
    activewidth [5 ]= background [5 ];
    activewidth [6 ]= background [6 ];
    passive = 0 ;
    printednode = memtop - 3 ;
    passnumber = 0 ;
    fontinshortdisplay = 0 ;
    curp = mem [memtop - 3 ].hh .v.RH ;
    autobreaking = true ;
    prevp = curp ;
    while ( ( curp != 0 ) && ( mem [memtop - 7 ].hh .v.RH != memtop - 7 ) ) 
    {
      if ( ( curp >= himemmin ) ) 
      {
	prevp = curp ;
	do {
	    f = mem [curp ].hh.b0 ;
	  activewidth [1 ]= activewidth [1 ]+ fontinfo [widthbase [f ]+ 
	  fontinfo [charbase [f ]+ effectivechar ( true , f , mem [curp ]
	  .hh.b1 ) ].qqqq .b0 ].cint ;
	  curp = mem [curp ].hh .v.RH ;
	} while ( ! ( ! ( curp >= himemmin ) ) ) ;
      } 
      switch ( mem [curp ].hh.b0 ) 
      {case 0 : 
      case 1 : 
      case 2 : 
	activewidth [1 ]= activewidth [1 ]+ mem [curp + 1 ].cint ;
	break ;
      case 8 : 
	if ( mem [curp ].hh.b1 == 4 ) 
	{
	  curlang = mem [curp + 1 ].hh .v.RH ;
	  lhyf = mem [curp + 1 ].hh.b0 ;
	  rhyf = mem [curp + 1 ].hh.b1 ;
	} 
	break ;
      case 10 : 
	{
	  if ( autobreaking ) 
	  {
	    if ( ( prevp >= himemmin ) ) 
	    trybreak ( 0 , 0 ) ;
	    else if ( ( mem [prevp ].hh.b0 < 9 ) ) 
	    trybreak ( 0 , 0 ) ;
	    else if ( ( mem [prevp ].hh.b0 == 11 ) && ( mem [prevp ].hh.b1 
	    != 1 ) ) 
	    trybreak ( 0 , 0 ) ;
	  } 
	  if ( ( mem [mem [curp + 1 ].hh .v.LH ].hh.b1 != 0 ) && ( mem [
	  mem [curp + 1 ].hh .v.LH + 3 ].cint != 0 ) ) 
	  {
	    mem [curp + 1 ].hh .v.LH = finiteshrink ( mem [curp + 1 ].hh 
	    .v.LH ) ;
	  } 
	  q = mem [curp + 1 ].hh .v.LH ;
	  activewidth [1 ]= activewidth [1 ]+ mem [q + 1 ].cint ;
	  activewidth [2 + mem [q ].hh.b0 ]= activewidth [2 + mem [q ]
	  .hh.b0 ]+ mem [q + 2 ].cint ;
	  activewidth [6 ]= activewidth [6 ]+ mem [q + 3 ].cint ;
	  if ( secondpass && autobreaking ) 
	  {
	    prevs = curp ;
	    s = mem [prevs ].hh .v.RH ;
	    if ( s != 0 ) 
	    {
	      while ( true ) {
		  
		if ( ( s >= himemmin ) ) 
		{
		  c = mem [s ].hh.b1 ;
		  hf = mem [s ].hh.b0 ;
		} 
		else if ( mem [s ].hh.b0 == 6 ) 
		if ( mem [s + 1 ].hh .v.RH == 0 ) 
		goto lab22 ;
		else {
		    
		  q = mem [s + 1 ].hh .v.RH ;
		  c = mem [q ].hh.b1 ;
		  hf = mem [q ].hh.b0 ;
		} 
		else if ( ( mem [s ].hh.b0 == 11 ) && ( mem [s ].hh.b1 == 
		0 ) ) 
		goto lab22 ;
		else if ( mem [s ].hh.b0 == 8 ) 
		{
		  if ( mem [s ].hh.b1 == 4 ) 
		  {
		    curlang = mem [s + 1 ].hh .v.RH ;
		    lhyf = mem [s + 1 ].hh.b0 ;
		    rhyf = mem [s + 1 ].hh.b1 ;
		  } 
		  goto lab22 ;
		} 
		else goto lab31 ;
		if ( eqtb [13883 + c ].hh .v.RH != 0 ) 
		if ( ( eqtb [13883 + c ].hh .v.RH == c ) || ( eqtb [15201 ]
		.cint > 0 ) ) 
		goto lab32 ;
		else goto lab31 ;
		lab22: prevs = s ;
		s = mem [prevs ].hh .v.RH ;
	      } 
	      lab32: hyfchar = hyphenchar [hf ];
	      if ( hyfchar < 0 ) 
	      goto lab31 ;
	      if ( hyfchar > 255 ) 
	      goto lab31 ;
	      ha = prevs ;
	      if ( lhyf + rhyf > 63 ) 
	      goto lab31 ;
	      hn = 0 ;
	      while ( true ) {
		  
		if ( ( s >= himemmin ) ) 
		{
		  if ( mem [s ].hh.b0 != hf ) 
		  goto lab33 ;
		  hyfbchar = mem [s ].hh.b1 ;
		  c = hyfbchar ;
		  if ( eqtb [13883 + c ].hh .v.RH == 0 ) 
		  goto lab33 ;
		  if ( hn == 63 ) 
		  goto lab33 ;
		  hb = s ;
		  incr ( hn ) ;
		  hu [hn ]= c ;
		  hc [hn ]= eqtb [13883 + c ].hh .v.RH ;
		  hyfbchar = 256 ;
		} 
		else if ( mem [s ].hh.b0 == 6 ) 
		{
		  if ( mem [s + 1 ].hh.b0 != hf ) 
		  goto lab33 ;
		  j = hn ;
		  q = mem [s + 1 ].hh .v.RH ;
		  if ( q > 0 ) 
		  hyfbchar = mem [q ].hh.b1 ;
		  while ( q > 0 ) {
		      
		    c = mem [q ].hh.b1 ;
		    if ( eqtb [13883 + c ].hh .v.RH == 0 ) 
		    goto lab33 ;
		    if ( j == 63 ) 
		    goto lab33 ;
		    incr ( j ) ;
		    hu [j ]= c ;
		    hc [j ]= eqtb [13883 + c ].hh .v.RH ;
		    q = mem [q ].hh .v.RH ;
		  } 
		  hb = s ;
		  hn = j ;
		  if ( odd ( mem [s ].hh.b1 ) ) 
		  hyfbchar = fontbchar [hf ];
		  else hyfbchar = 256 ;
		} 
		else if ( ( mem [s ].hh.b0 == 11 ) && ( mem [s ].hh.b1 == 
		0 ) ) 
		{
		  hb = s ;
		  hyfbchar = fontbchar [hf ];
		} 
		else goto lab33 ;
		s = mem [s ].hh .v.RH ;
	      } 
	      lab33: ;
	      if ( hn < lhyf + rhyf ) 
	      goto lab31 ;
	      while ( true ) {
		  
		if ( ! ( ( s >= himemmin ) ) ) 
		switch ( mem [s ].hh.b0 ) 
		{case 6 : 
		  ;
		  break ;
		case 11 : 
		  if ( mem [s ].hh.b1 != 0 ) 
		  goto lab34 ;
		  break ;
		case 8 : 
		case 10 : 
		case 12 : 
		case 3 : 
		case 5 : 
		case 4 : 
		  goto lab34 ;
		  break ;
		  default: 
		  goto lab31 ;
		  break ;
		} 
		s = mem [s ].hh .v.RH ;
	      } 
	      lab34: ;
	      hyphenate () ;
	    } 
	    lab31: ;
	  } 
	} 
	break ;
      case 11 : 
	if ( mem [curp ].hh.b1 == 1 ) 
	{
	  if ( ! ( mem [curp ].hh .v.RH >= himemmin ) && autobreaking ) 
	  if ( mem [mem [curp ].hh .v.RH ].hh.b0 == 10 ) 
	  trybreak ( 0 , 0 ) ;
	  activewidth [1 ]= activewidth [1 ]+ mem [curp + 1 ].cint ;
	} 
	else activewidth [1 ]= activewidth [1 ]+ mem [curp + 1 ].cint ;
	break ;
      case 6 : 
	{
	  f = mem [curp + 1 ].hh.b0 ;
	  activewidth [1 ]= activewidth [1 ]+ fontinfo [widthbase [f ]+ 
	  fontinfo [charbase [f ]+ effectivechar ( true , f , mem [curp + 
	  1 ].hh.b1 ) ].qqqq .b0 ].cint ;
	} 
	break ;
      case 7 : 
	{
	  s = mem [curp + 1 ].hh .v.LH ;
	  discwidth = 0 ;
	  if ( s == 0 ) 
	  trybreak ( eqtb [15167 ].cint , 1 ) ;
	  else {
	      
	    do {
		if ( ( s >= himemmin ) ) 
	      {
		f = mem [s ].hh.b0 ;
		discwidth = discwidth + fontinfo [widthbase [f ]+ fontinfo 
		[charbase [f ]+ effectivechar ( true , f , mem [s ].hh.b1 
		) ].qqqq .b0 ].cint ;
	      } 
	      else switch ( mem [s ].hh.b0 ) 
	      {case 6 : 
		{
		  f = mem [s + 1 ].hh.b0 ;
		  discwidth = discwidth + fontinfo [widthbase [f ]+ 
		  fontinfo [charbase [f ]+ effectivechar ( true , f , mem [
		  s + 1 ].hh.b1 ) ].qqqq .b0 ].cint ;
		} 
		break ;
	      case 0 : 
	      case 1 : 
	      case 2 : 
	      case 11 : 
		discwidth = discwidth + mem [s + 1 ].cint ;
		break ;
		default: 
		confusion ( 932 ) ;
		break ;
	      } 
	      s = mem [s ].hh .v.RH ;
	    } while ( ! ( s == 0 ) ) ;
	    activewidth [1 ]= activewidth [1 ]+ discwidth ;
	    trybreak ( eqtb [15166 ].cint , 1 ) ;
	    activewidth [1 ]= activewidth [1 ]- discwidth ;
	  } 
	  r = mem [curp ].hh.b1 ;
	  s = mem [curp ].hh .v.RH ;
	  while ( r > 0 ) {
	      
	    if ( ( s >= himemmin ) ) 
	    {
	      f = mem [s ].hh.b0 ;
	      activewidth [1 ]= activewidth [1 ]+ fontinfo [widthbase [f 
	      ]+ fontinfo [charbase [f ]+ effectivechar ( true , f , mem [
	      s ].hh.b1 ) ].qqqq .b0 ].cint ;
	    } 
	    else switch ( mem [s ].hh.b0 ) 
	    {case 6 : 
	      {
		f = mem [s + 1 ].hh.b0 ;
		activewidth [1 ]= activewidth [1 ]+ fontinfo [widthbase [
		f ]+ fontinfo [charbase [f ]+ effectivechar ( true , f , 
		mem [s + 1 ].hh.b1 ) ].qqqq .b0 ].cint ;
	      } 
	      break ;
	    case 0 : 
	    case 1 : 
	    case 2 : 
	    case 11 : 
	      activewidth [1 ]= activewidth [1 ]+ mem [s + 1 ].cint ;
	      break ;
	      default: 
	      confusion ( 933 ) ;
	      break ;
	    } 
	    decr ( r ) ;
	    s = mem [s ].hh .v.RH ;
	  } 
	  prevp = curp ;
	  curp = s ;
	  goto lab35 ;
	} 
	break ;
      case 9 : 
	{
	  autobreaking = ( mem [curp ].hh.b1 == 1 ) ;
	  {
	    if ( ! ( mem [curp ].hh .v.RH >= himemmin ) && autobreaking ) 
	    if ( mem [mem [curp ].hh .v.RH ].hh.b0 == 10 ) 
	    trybreak ( 0 , 0 ) ;
	    activewidth [1 ]= activewidth [1 ]+ mem [curp + 1 ].cint ;
	  } 
	} 
	break ;
      case 12 : 
	trybreak ( mem [curp + 1 ].cint , 0 ) ;
	break ;
      case 4 : 
      case 3 : 
      case 5 : 
	;
	break ;
	default: 
	confusion ( 931 ) ;
	break ;
      } 
      prevp = curp ;
      curp = mem [curp ].hh .v.RH ;
      lab35: ;
    } 
    if ( curp == 0 ) 
    {
      trybreak ( -10000 , 1 ) ;
      if ( mem [memtop - 7 ].hh .v.RH != memtop - 7 ) 
      {
	r = mem [memtop - 7 ].hh .v.RH ;
	fewestdemerits = 1073741823L ;
	do {
	    if ( mem [r ].hh.b0 != 2 ) 
	  if ( mem [r + 2 ].cint < fewestdemerits ) 
	  {
	    fewestdemerits = mem [r + 2 ].cint ;
	    bestbet = r ;
	  } 
	  r = mem [r ].hh .v.RH ;
	} while ( ! ( r == memtop - 7 ) ) ;
	bestline = mem [bestbet + 1 ].hh .v.LH ;
	if ( eqtb [15182 ].cint == 0 ) 
	goto lab30 ;
	{
	  r = mem [memtop - 7 ].hh .v.RH ;
	  actuallooseness = 0 ;
	  do {
	      if ( mem [r ].hh.b0 != 2 ) 
	    {
	      linediff = toint ( mem [r + 1 ].hh .v.LH ) - toint ( bestline 
	      ) ;
	      if ( ( ( linediff < actuallooseness ) && ( eqtb [15182 ].cint 
	      <= linediff ) ) || ( ( linediff > actuallooseness ) && ( eqtb [
	      15182 ].cint >= linediff ) ) ) 
	      {
		bestbet = r ;
		actuallooseness = linediff ;
		fewestdemerits = mem [r + 2 ].cint ;
	      } 
	      else if ( ( linediff == actuallooseness ) && ( mem [r + 2 ]
	      .cint < fewestdemerits ) ) 
	      {
		bestbet = r ;
		fewestdemerits = mem [r + 2 ].cint ;
	      } 
	    } 
	    r = mem [r ].hh .v.RH ;
	  } while ( ! ( r == memtop - 7 ) ) ;
	  bestline = mem [bestbet + 1 ].hh .v.LH ;
	} 
	if ( ( actuallooseness == eqtb [15182 ].cint ) || finalpass ) 
	goto lab30 ;
      } 
    } 
    q = mem [memtop - 7 ].hh .v.RH ;
    while ( q != memtop - 7 ) {
	
      curp = mem [q ].hh .v.RH ;
      if ( mem [q ].hh.b0 == 2 ) 
      freenode ( q , 7 ) ;
      else freenode ( q , 3 ) ;
      q = curp ;
    } 
    q = passive ;
    while ( q != 0 ) {
	
      curp = mem [q ].hh .v.RH ;
      freenode ( q , 2 ) ;
      q = curp ;
    } 
    if ( ! secondpass ) 
    {
	;
#ifdef STAT
      if ( eqtb [15195 ].cint > 0 ) 
      printnl ( 929 ) ;
#endif /* STAT */
      threshold = eqtb [15164 ].cint ;
      secondpass = true ;
      finalpass = ( eqtb [15753 ].cint <= 0 ) ;
    } 
    else {
	
	;
#ifdef STAT
      if ( eqtb [15195 ].cint > 0 ) 
      printnl ( 930 ) ;
#endif /* STAT */
      background [2 ]= background [2 ]+ eqtb [15753 ].cint ;
      finalpass = true ;
    } 
  } 
  lab30: 
	;
#ifdef STAT
  if ( eqtb [15195 ].cint > 0 ) 
  {
    enddiagnostic ( true ) ;
    normalizeselector () ;
  } 
#endif /* STAT */
  postlinebreak ( finalwidowpenalty ) ;
  q = mem [memtop - 7 ].hh .v.RH ;
  while ( q != memtop - 7 ) {
      
    curp = mem [q ].hh .v.RH ;
    if ( mem [q ].hh.b0 == 2 ) 
    freenode ( q , 7 ) ;
    else freenode ( q , 3 ) ;
    q = curp ;
  } 
  q = passive ;
  while ( q != 0 ) {
      
    curp = mem [q ].hh .v.RH ;
    freenode ( q , 2 ) ;
    q = curp ;
  } 
  packbeginline = 0 ;
} 
void 
#ifdef HAVE_PROTOTYPES
prefixedcommand ( void ) 
#else
prefixedcommand ( ) 
#endif
{
  /* 30 10 */ prefixedcommand_regmem 
  smallnumber a  ;
  internalfontnumber f  ;
  halfword j  ;
  fontindex k  ;
  halfword p, q  ;
  integer n  ;
  boolean e  ;
  a = 0 ;
  while ( curcmd == 93 ) {
      
    if ( ! odd ( a / curchr ) ) 
    a = a + curchr ;
    do {
	getxtoken () ;
    } while ( ! ( ( curcmd != 10 ) && ( curcmd != 0 ) ) ) ;
    if ( curcmd <= 70 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 1174 ) ;
      } 
      printcmdchr ( curcmd , curchr ) ;
      printchar ( 39 ) ;
      {
	helpptr = 1 ;
	helpline [0 ]= 1175 ;
      } 
      backerror () ;
      return ;
    } 
  } 
  if ( ( curcmd != 97 ) && ( a % 4 != 0 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 684 ) ;
    } 
    printesc ( 1166 ) ;
    print ( 1176 ) ;
    printesc ( 1167 ) ;
    print ( 1177 ) ;
    printcmdchr ( curcmd , curchr ) ;
    printchar ( 39 ) ;
    {
      helpptr = 1 ;
      helpline [0 ]= 1178 ;
    } 
    error () ;
  } 
  if ( eqtb [15206 ].cint != 0 ) 
  if ( eqtb [15206 ].cint < 0 ) 
  {
    if ( ( a >= 4 ) ) 
    a = a - 4 ;
  } 
  else {
      
    if ( ! ( a >= 4 ) ) 
    a = a + 4 ;
  } 
  switch ( curcmd ) 
  {case 87 : 
    if ( ( a >= 4 ) ) 
    geqdefine ( 13578 , 120 , curchr ) ;
    else eqdefine ( 13578 , 120 , curchr ) ;
    break ;
  case 97 : 
    {
      if ( odd ( curchr ) && ! ( a >= 4 ) && ( eqtb [15206 ].cint >= 0 ) ) 
      a = a + 4 ;
      e = ( curchr >= 2 ) ;
      getrtoken () ;
      p = curcs ;
      q = scantoks ( true , e ) ;
      if ( ( a >= 4 ) ) 
      geqdefine ( p , 111 + ( a % 4 ) , defref ) ;
      else eqdefine ( p , 111 + ( a % 4 ) , defref ) ;
    } 
    break ;
  case 94 : 
    {
      n = curchr ;
      getrtoken () ;
      p = curcs ;
      if ( n == 0 ) 
      {
	do {
	    gettoken () ;
	} while ( ! ( curcmd != 10 ) ) ;
	if ( curtok == 3133 ) 
	{
	  gettoken () ;
	  if ( curcmd == 10 ) 
	  gettoken () ;
	} 
      } 
      else {
	  
	gettoken () ;
	q = curtok ;
	gettoken () ;
	backinput () ;
	curtok = q ;
	backinput () ;
      } 
      if ( curcmd >= 111 ) 
      incr ( mem [curchr ].hh .v.LH ) ;
      if ( ( a >= 4 ) ) 
      geqdefine ( p , curcmd , curchr ) ;
      else eqdefine ( p , curcmd , curchr ) ;
    } 
    break ;
  case 95 : 
    if ( curchr == 7 ) 
    {
      scancharnum () ;
      p = 14907 + curval ;
      scanoptionalequals () ;
      scancharnum () ;
      n = curval ;
      scancharnum () ;
      if ( ( eqtb [15220 ].cint > 0 ) ) 
      {
	begindiagnostic () ;
	printnl ( 1196 ) ;
	print ( p - 14907 ) ;
	print ( 1197 ) ;
	print ( n ) ;
	printchar ( 32 ) ;
	print ( curval ) ;
	enddiagnostic ( false ) ;
      } 
      n = n * 256 + curval ;
      if ( ( a >= 4 ) ) 
      geqdefine ( p , 120 , n ) ;
      else eqdefine ( p , 120 , n ) ;
      if ( ( p - 14907 ) < eqtb [15218 ].cint ) 
      if ( ( a >= 4 ) ) 
      geqworddefine ( 15218 , p - 14907 ) ;
      else eqworddefine ( 15218 , p - 14907 ) ;
      if ( ( p - 14907 ) > eqtb [15219 ].cint ) 
      if ( ( a >= 4 ) ) 
      geqworddefine ( 15219 , p - 14907 ) ;
      else eqworddefine ( 15219 , p - 14907 ) ;
    } 
    else {
	
      n = curchr ;
      getrtoken () ;
      p = curcs ;
      if ( ( a >= 4 ) ) 
      geqdefine ( p , 0 , 256 ) ;
      else eqdefine ( p , 0 , 256 ) ;
      scanoptionalequals () ;
      switch ( n ) 
      {case 0 : 
	{
	  scancharnum () ;
	  if ( ( a >= 4 ) ) 
	  geqdefine ( p , 68 , curval ) ;
	  else eqdefine ( p , 68 , curval ) ;
	} 
	break ;
      case 1 : 
	{
	  scanfifteenbitint () ;
	  if ( ( a >= 4 ) ) 
	  geqdefine ( p , 69 , curval ) ;
	  else eqdefine ( p , 69 , curval ) ;
	} 
	break ;
	default: 
	{
	  scaneightbitint () ;
	  switch ( n ) 
	  {case 2 : 
	    if ( ( a >= 4 ) ) 
	    geqdefine ( p , 73 , 15221 + curval ) ;
	    else eqdefine ( p , 73 , 15221 + curval ) ;
	    break ;
	  case 3 : 
	    if ( ( a >= 4 ) ) 
	    geqdefine ( p , 74 , 15754 + curval ) ;
	    else eqdefine ( p , 74 , 15754 + curval ) ;
	    break ;
	  case 4 : 
	    if ( ( a >= 4 ) ) 
	    geqdefine ( p , 75 , 12544 + curval ) ;
	    else eqdefine ( p , 75 , 12544 + curval ) ;
	    break ;
	  case 5 : 
	    if ( ( a >= 4 ) ) 
	    geqdefine ( p , 76 , 12800 + curval ) ;
	    else eqdefine ( p , 76 , 12800 + curval ) ;
	    break ;
	  case 6 : 
	    if ( ( a >= 4 ) ) 
	    geqdefine ( p , 72 , 13066 + curval ) ;
	    else eqdefine ( p , 72 , 13066 + curval ) ;
	    break ;
	  } 
	} 
	break ;
      } 
    } 
    break ;
  case 96 : 
    {
      scanint () ;
      n = curval ;
      if ( ! scankeyword ( 837 ) ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 1068 ) ;
	} 
	{
	  helpptr = 2 ;
	  helpline [1 ]= 1198 ;
	  helpline [0 ]= 1199 ;
	} 
	error () ;
      } 
      getrtoken () ;
      p = curcs ;
      readtoks ( n , p ) ;
      if ( ( a >= 4 ) ) 
      geqdefine ( p , 111 , curval ) ;
      else eqdefine ( p , 111 , curval ) ;
    } 
    break ;
  case 71 : 
  case 72 : 
    {
      q = curcs ;
      if ( curcmd == 71 ) 
      {
	scaneightbitint () ;
	p = 13066 + curval ;
      } 
      else p = curchr ;
      scanoptionalequals () ;
      do {
	  getxtoken () ;
      } while ( ! ( ( curcmd != 10 ) && ( curcmd != 0 ) ) ) ;
      if ( curcmd != 1 ) 
      {
	if ( curcmd == 71 ) 
	{
	  scaneightbitint () ;
	  curcmd = 72 ;
	  curchr = 13066 + curval ;
	} 
	if ( curcmd == 72 ) 
	{
	  q = eqtb [curchr ].hh .v.RH ;
	  if ( q == 0 ) 
	  if ( ( a >= 4 ) ) 
	  geqdefine ( p , 101 , 0 ) ;
	  else eqdefine ( p , 101 , 0 ) ;
	  else {
	      
	    incr ( mem [q ].hh .v.LH ) ;
	    if ( ( a >= 4 ) ) 
	    geqdefine ( p , 111 , q ) ;
	    else eqdefine ( p , 111 , q ) ;
	  } 
	  goto lab30 ;
	} 
      } 
      backinput () ;
      curcs = q ;
      q = scantoks ( false , false ) ;
      if ( mem [defref ].hh .v.RH == 0 ) 
      {
	if ( ( a >= 4 ) ) 
	geqdefine ( p , 101 , 0 ) ;
	else eqdefine ( p , 101 , 0 ) ;
	{
	  mem [defref ].hh .v.RH = avail ;
	  avail = defref ;
	;
#ifdef STAT
	  decr ( dynused ) ;
#endif /* STAT */
	} 
      } 
      else {
	  
	if ( p == 13057 ) 
	{
	  mem [q ].hh .v.RH = getavail () ;
	  q = mem [q ].hh .v.RH ;
	  mem [q ].hh .v.LH = 637 ;
	  q = getavail () ;
	  mem [q ].hh .v.LH = 379 ;
	  mem [q ].hh .v.RH = mem [defref ].hh .v.RH ;
	  mem [defref ].hh .v.RH = q ;
	} 
	if ( ( a >= 4 ) ) 
	geqdefine ( p , 111 , defref ) ;
	else eqdefine ( p , 111 , defref ) ;
      } 
    } 
    break ;
  case 73 : 
    {
      p = curchr ;
      scanoptionalequals () ;
      scanint () ;
      if ( ( a >= 4 ) ) 
      geqworddefine ( p , curval ) ;
      else eqworddefine ( p , curval ) ;
    } 
    break ;
  case 74 : 
    {
      p = curchr ;
      scanoptionalequals () ;
      scandimen ( false , false , false ) ;
      if ( ( a >= 4 ) ) 
      geqworddefine ( p , curval ) ;
      else eqworddefine ( p , curval ) ;
    } 
    break ;
  case 75 : 
  case 76 : 
    {
      p = curchr ;
      n = curcmd ;
      scanoptionalequals () ;
      if ( n == 76 ) 
      scanglue ( 3 ) ;
      else scanglue ( 2 ) ;
      trapzeroglue () ;
      if ( ( a >= 4 ) ) 
      geqdefine ( p , 117 , curval ) ;
      else eqdefine ( p , 117 , curval ) ;
    } 
    break ;
  case 85 : 
    {
      if ( curchr == 13627 ) 
      n = 15 ;
      else if ( curchr == 14651 ) 
      n = 32768L ;
      else if ( curchr == 14395 ) 
      n = 32767 ;
      else if ( curchr == 15477 ) 
      n = 16777215L ;
      else n = 255 ;
      p = curchr ;
      scancharnum () ;
      p = p + curval ;
      scanoptionalequals () ;
      scanint () ;
      if ( ( ( curval < 0 ) && ( p < 15477 ) ) || ( curval > n ) ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 1200 ) ;
	} 
	printint ( curval ) ;
	if ( p < 15477 ) 
	print ( 1201 ) ;
	else print ( 1202 ) ;
	printint ( n ) ;
	{
	  helpptr = 1 ;
	  helpline [0 ]= 1203 ;
	} 
	error () ;
	curval = 0 ;
      } 
      if ( p < 14651 ) 
      if ( ( a >= 4 ) ) 
      geqdefine ( p , 120 , curval ) ;
      else eqdefine ( p , 120 , curval ) ;
      else if ( p < 15477 ) 
      if ( ( a >= 4 ) ) 
      geqdefine ( p , 120 , curval ) ;
      else eqdefine ( p , 120 , curval ) ;
      else if ( ( a >= 4 ) ) 
      geqworddefine ( p , curval ) ;
      else eqworddefine ( p , curval ) ;
    } 
    break ;
  case 86 : 
    {
      p = curchr ;
      scanfourbitint () ;
      p = p + curval ;
      scanoptionalequals () ;
      scanfontident () ;
      if ( ( a >= 4 ) ) 
      geqdefine ( p , 120 , curval ) ;
      else eqdefine ( p , 120 , curval ) ;
    } 
    break ;
  case 89 : 
  case 90 : 
  case 91 : 
  case 92 : 
    doregistercommand ( a ) ;
    break ;
  case 98 : 
    {
      scaneightbitint () ;
      if ( ( a >= 4 ) ) 
      n = 256 + curval ;
      else n = curval ;
      scanoptionalequals () ;
      if ( setboxallowed ) 
      scanbox ( 1073741824L + n ) ;
      else {
	  
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 679 ) ;
	} 
	printesc ( 536 ) ;
	{
	  helpptr = 2 ;
	  helpline [1 ]= 1209 ;
	  helpline [0 ]= 1210 ;
	} 
	error () ;
      } 
    } 
    break ;
  case 79 : 
    alteraux () ;
    break ;
  case 80 : 
    alterprevgraf () ;
    break ;
  case 81 : 
    alterpagesofar () ;
    break ;
  case 82 : 
    alterinteger () ;
    break ;
  case 83 : 
    alterboxdimen () ;
    break ;
  case 84 : 
    {
      scanoptionalequals () ;
      scanint () ;
      n = curval ;
      if ( n <= 0 ) 
      p = 0 ;
      else {
	  
	p = getnode ( 2 * n + 1 ) ;
	mem [p ].hh .v.LH = n ;
	{register integer for_end; j = 1 ;for_end = n ; if ( j <= for_end) 
	do 
	  {
	    scandimen ( false , false , false ) ;
	    mem [p + 2 * j - 1 ].cint = curval ;
	    scandimen ( false , false , false ) ;
	    mem [p + 2 * j ].cint = curval ;
	  } 
	while ( j++ < for_end ) ;} 
      } 
      if ( ( a >= 4 ) ) 
      geqdefine ( 13056 , 118 , p ) ;
      else eqdefine ( 13056 , 118 , p ) ;
    } 
    break ;
  case 99 : 
    if ( curchr == 1 ) 
    {
	;
#ifdef INITEX
      if ( iniversion ) 
      {
	newpatterns () ;
	goto lab30 ;
      } 
#endif /* INITEX */
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 1214 ) ;
      } 
      helpptr = 0 ;
      error () ;
      do {
	  gettoken () ;
      } while ( ! ( curcmd == 2 ) ) ;
      return ;
    } 
    else {
	
      newhyphexceptions () ;
      goto lab30 ;
    } 
    break ;
  case 77 : 
    {
      findfontdimen ( true ) ;
      k = curval ;
      scanoptionalequals () ;
      scandimen ( false , false , false ) ;
      fontinfo [k ].cint = curval ;
    } 
    break ;
  case 78 : 
    {
      n = curchr ;
      scanfontident () ;
      f = curval ;
      scanoptionalequals () ;
      scanint () ;
      if ( n == 0 ) 
      hyphenchar [f ]= curval ;
      else skewchar [f ]= curval ;
    } 
    break ;
  case 88 : 
    newfont ( a ) ;
    break ;
  case 100 : 
    newinteraction () ;
    break ;
    default: 
    confusion ( 1173 ) ;
    break ;
  } 
  lab30: if ( aftertoken != 0 ) 
  {
    curtok = aftertoken ;
    backinput () ;
    aftertoken = 0 ;
  } 
} 
#ifdef INITEX
void 
#ifdef HAVE_PROTOTYPES
storefmtfile ( void ) 
#else
storefmtfile ( ) 
#endif
{
  /* 41 42 31 32 */ storefmtfile_regmem 
  integer j, k, l  ;
  halfword p, q  ;
  integer x  ;
  if ( saveptr != 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 1256 ) ;
    } 
    {
      helpptr = 1 ;
      helpline [0 ]= 1257 ;
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
  selector = 21 ;
  print ( 1270 ) ;
  print ( jobname ) ;
  printchar ( 32 ) ;
  printint ( eqtb [15186 ].cint ) ;
  printchar ( 46 ) ;
  printint ( eqtb [15185 ].cint ) ;
  printchar ( 46 ) ;
  printint ( eqtb [15184 ].cint ) ;
  printchar ( 41 ) ;
  if ( interaction == 0 ) 
  selector = 18 ;
  else selector = 19 ;
  {
    if ( poolptr + 1 > poolsize ) 
    overflow ( 257 , poolsize - initpoolptr ) ;
  } 
  formatident = makestring () ;
  packjobname ( 782 ) ;
  while ( ! wopenout ( fmtfile ) ) promptfilename ( 1271 , 782 ) ;
  printnl ( 1272 ) ;
  print ( wmakenamestring ( fmtfile ) ) ;
  {
    decr ( strptr ) ;
    poolptr = strstart [strptr ];
  } 
  printnl ( 335 ) ;
  print ( formatident ) ;
  dumpint ( 294434737L ) ;
  dumpint ( 268435455L ) ;
  dumpint ( hashhigh ) ;
  dumpint ( membot ) ;
  dumpint ( memtop ) ;
  dumpint ( 16009 ) ;
  dumpint ( 8501 ) ;
  dumpint ( 607 ) ;
  dumpint ( 1296847960L ) ;
  if ( mltexp ) 
  dumpint ( 1 ) ;
  else dumpint ( 0 ) ;
  dumpint ( poolptr ) ;
  dumpint ( strptr ) ;
  dumpthings ( strstart [0 ], strptr + 1 ) ;
  dumpthings ( strpool [0 ], poolptr ) ;
  println () ;
  printint ( strptr ) ;
  print ( 1258 ) ;
  printint ( poolptr ) ;
  sortavail () ;
  varused = 0 ;
  dumpint ( lomemmax ) ;
  dumpint ( rover ) ;
  p = membot ;
  q = rover ;
  x = 0 ;
  do {
      dumpthings ( mem [p ], q + 2 - p ) ;
    x = x + q + 2 - p ;
    varused = varused + q - p ;
    p = q + mem [q ].hh .v.LH ;
    q = mem [q + 1 ].hh .v.RH ;
  } while ( ! ( q == rover ) ) ;
  varused = varused + lomemmax - p ;
  dynused = memend + 1 - himemmin ;
  dumpthings ( mem [p ], lomemmax + 1 - p ) ;
  x = x + lomemmax + 1 - p ;
  dumpint ( himemmin ) ;
  dumpint ( avail ) ;
  dumpthings ( mem [himemmin ], memend + 1 - himemmin ) ;
  x = x + memend + 1 - himemmin ;
  p = avail ;
  while ( p != 0 ) {
      
    decr ( dynused ) ;
    p = mem [p ].hh .v.RH ;
  } 
  dumpint ( varused ) ;
  dumpint ( dynused ) ;
  println () ;
  printint ( x ) ;
  print ( 1259 ) ;
  printint ( varused ) ;
  printchar ( 38 ) ;
  printint ( dynused ) ;
  k = 1 ;
  do {
      j = k ;
    while ( j < 15162 ) {
	
      if ( ( eqtb [j ].hh .v.RH == eqtb [j + 1 ].hh .v.RH ) && ( eqtb [j 
      ].hh.b0 == eqtb [j + 1 ].hh.b0 ) && ( eqtb [j ].hh.b1 == eqtb [j + 
      1 ].hh.b1 ) ) 
      goto lab41 ;
      incr ( j ) ;
    } 
    l = 15163 ;
    goto lab31 ;
    lab41: incr ( j ) ;
    l = j ;
    while ( j < 15162 ) {
	
      if ( ( eqtb [j ].hh .v.RH != eqtb [j + 1 ].hh .v.RH ) || ( eqtb [j 
      ].hh.b0 != eqtb [j + 1 ].hh.b0 ) || ( eqtb [j ].hh.b1 != eqtb [j + 
      1 ].hh.b1 ) ) 
      goto lab31 ;
      incr ( j ) ;
    } 
    lab31: dumpint ( l - k ) ;
    dumpthings ( eqtb [k ], l - k ) ;
    k = j + 1 ;
    dumpint ( k - l ) ;
  } while ( ! ( k == 15163 ) ) ;
  do {
      j = k ;
    while ( j < 16009 ) {
	
      if ( eqtb [j ].cint == eqtb [j + 1 ].cint ) 
      goto lab42 ;
      incr ( j ) ;
    } 
    l = 16010 ;
    goto lab32 ;
    lab42: incr ( j ) ;
    l = j ;
    while ( j < 16009 ) {
	
      if ( eqtb [j ].cint != eqtb [j + 1 ].cint ) 
      goto lab32 ;
      incr ( j ) ;
    } 
    lab32: dumpint ( l - k ) ;
    dumpthings ( eqtb [k ], l - k ) ;
    k = j + 1 ;
    dumpint ( k - l ) ;
  } while ( ! ( k > 16009 ) ) ;
  if ( hashhigh > 0 ) 
  dumpthings ( eqtb [16010 ], hashhigh ) ;
  dumpint ( parloc ) ;
  dumpint ( writeloc ) ;
  dumpint ( hashused ) ;
  cscount = 10513 - hashused + hashhigh ;
  {register integer for_end; p = 514 ;for_end = hashused ; if ( p <= 
  for_end) do 
    if ( hash [p ].v.RH != 0 ) 
    {
      dumpint ( p ) ;
      dumphh ( hash [p ]) ;
      incr ( cscount ) ;
    } 
  while ( p++ < for_end ) ;} 
  dumpthings ( hash [hashused + 1 ], 12524 - hashused ) ;
  if ( hashhigh > 0 ) 
  dumpthings ( hash [16010 ], hashhigh ) ;
  dumpint ( cscount ) ;
  println () ;
  printint ( cscount ) ;
  print ( 1260 ) ;
  dumpint ( fmemptr ) ;
  dumpthings ( fontinfo [0 ], fmemptr ) ;
  dumpint ( fontptr ) ;
  {
    dumpthings ( fontcheck [0 ], fontptr + 1 ) ;
    dumpthings ( fontsize [0 ], fontptr + 1 ) ;
    dumpthings ( fontdsize [0 ], fontptr + 1 ) ;
    dumpthings ( fontparams [0 ], fontptr + 1 ) ;
    dumpthings ( hyphenchar [0 ], fontptr + 1 ) ;
    dumpthings ( skewchar [0 ], fontptr + 1 ) ;
    dumpthings ( fontname [0 ], fontptr + 1 ) ;
    dumpthings ( fontarea [0 ], fontptr + 1 ) ;
    dumpthings ( fontbc [0 ], fontptr + 1 ) ;
    dumpthings ( fontec [0 ], fontptr + 1 ) ;
    dumpthings ( charbase [0 ], fontptr + 1 ) ;
    dumpthings ( widthbase [0 ], fontptr + 1 ) ;
    dumpthings ( heightbase [0 ], fontptr + 1 ) ;
    dumpthings ( depthbase [0 ], fontptr + 1 ) ;
    dumpthings ( italicbase [0 ], fontptr + 1 ) ;
    dumpthings ( ligkernbase [0 ], fontptr + 1 ) ;
    dumpthings ( kernbase [0 ], fontptr + 1 ) ;
    dumpthings ( extenbase [0 ], fontptr + 1 ) ;
    dumpthings ( parambase [0 ], fontptr + 1 ) ;
    dumpthings ( fontglue [0 ], fontptr + 1 ) ;
    dumpthings ( bcharlabel [0 ], fontptr + 1 ) ;
    dumpthings ( fontbchar [0 ], fontptr + 1 ) ;
    dumpthings ( fontfalsebchar [0 ], fontptr + 1 ) ;
    {register integer for_end; k = 0 ;for_end = fontptr ; if ( k <= for_end) 
    do 
      {
	printnl ( 1263 ) ;
	printesc ( hash [10524 + k ].v.RH ) ;
	printchar ( 61 ) ;
	printfilename ( fontname [k ], fontarea [k ], 335 ) ;
	if ( fontsize [k ]!= fontdsize [k ]) 
	{
	  print ( 740 ) ;
	  printscaled ( fontsize [k ]) ;
	  print ( 394 ) ;
	} 
      } 
    while ( k++ < for_end ) ;} 
  } 
  println () ;
  printint ( fmemptr - 7 ) ;
  print ( 1261 ) ;
  printint ( fontptr - 0 ) ;
  print ( 1262 ) ;
  if ( fontptr != 1 ) 
  printchar ( 115 ) ;
  dumpint ( hyphcount ) ;
  if ( hyphnext <= 607 ) 
  hyphnext = hyphsize ;
  dumpint ( hyphnext ) ;
  {register integer for_end; k = 0 ;for_end = hyphsize ; if ( k <= for_end) 
  do 
    if ( hyphword [k ]!= 0 ) 
    {
      dumpint ( k + 65536L * hyphlink [k ]) ;
      dumpint ( hyphword [k ]) ;
      dumpint ( hyphlist [k ]) ;
    } 
  while ( k++ < for_end ) ;} 
  println () ;
  printint ( hyphcount ) ;
  print ( 1264 ) ;
  if ( hyphcount != 1 ) 
  printchar ( 115 ) ;
  if ( trienotready ) 
  inittrie () ;
  dumpint ( triemax ) ;
  dumpthings ( trietrl [0 ], triemax + 1 ) ;
  dumpthings ( trietro [0 ], triemax + 1 ) ;
  dumpthings ( trietrc [0 ], triemax + 1 ) ;
  dumpint ( trieopptr ) ;
  dumpthings ( hyfdistance [1 ], trieopptr ) ;
  dumpthings ( hyfnum [1 ], trieopptr ) ;
  dumpthings ( hyfnext [1 ], trieopptr ) ;
  printnl ( 1265 ) ;
  printint ( triemax ) ;
  print ( 1266 ) ;
  printint ( trieopptr ) ;
  print ( 1267 ) ;
  if ( trieopptr != 1 ) 
  printchar ( 115 ) ;
  print ( 1268 ) ;
  printint ( trieopsize ) ;
  {register integer for_end; k = 255 ;for_end = 0 ; if ( k >= for_end) do 
    if ( trieused [k ]> 0 ) 
    {
      printnl ( 796 ) ;
      printint ( trieused [k ]) ;
      print ( 1269 ) ;
      printint ( k ) ;
      dumpint ( k ) ;
      dumpint ( trieused [k ]) ;
    } 
  while ( k-- > for_end ) ;} 
  dumpint ( interaction ) ;
  dumpint ( formatident ) ;
  dumpint ( 69069L ) ;
  eqtb [15194 ].cint = 0 ;
  wclose ( fmtfile ) ;
} 
#endif /* INITEX */
boolean 
#ifdef HAVE_PROTOTYPES
loadfmtfile ( void ) 
#else
loadfmtfile ( ) 
#endif
{
  /* 6666 10 */ register boolean Result; loadfmtfile_regmem 
  integer j, k  ;
  halfword p, q  ;
  integer x  ;
	;
#ifdef INITEX
  if ( iniversion ) 
  {
    libcfree ( fontinfo ) ;
    libcfree ( strpool ) ;
    libcfree ( strstart ) ;
    libcfree ( yhash ) ;
    libcfree ( zeqtb ) ;
    libcfree ( yzmem ) ;
  } 
#endif /* INITEX */
  undumpint ( x ) ;
  if ( debugformatfile ) 
  {
    fprintf( stderr , "%s%s",  "fmtdebug:" , "string pool checksum" ) ;
    fprintf( stderr , "%s%ld\n",  " = " , (long)x ) ;
  } 
  if ( x != 294434737L ) 
  goto lab6666 ;
  undumpint ( x ) ;
  if ( x != 268435455L ) 
  goto lab6666 ;
  undumpint ( hashhigh ) ;
  if ( ( hashhigh < 0 ) || ( hashhigh > suphashextra ) ) 
  goto lab6666 ;
  if ( hashextra < hashhigh ) 
  hashextra = hashhigh ;
  eqtbtop = 16009 + hashextra ;
  if ( hashextra == 0 ) 
  hashtop = 12525 ;
  else hashtop = eqtbtop ;
  xmallocarray ( yhash , 1 + hashtop - hashoffset ) ;
  hash = yhash - hashoffset ;
  hash [514 ].v.LH = 0 ;
  hash [514 ].v.RH = 0 ;
  {register integer for_end; x = 515 ;for_end = hashtop ; if ( x <= for_end) 
  do 
    hash [x ]= hash [514 ];
  while ( x++ < for_end ) ;} 
  xmallocarray ( zeqtb , eqtbtop + 1 ) ;
  eqtb = zeqtb ;
  eqtb [12525 ].hh.b0 = 101 ;
  eqtb [12525 ].hh .v.RH = 0 ;
  eqtb [12525 ].hh.b1 = 0 ;
  {register integer for_end; x = 16010 ;for_end = eqtbtop ; if ( x <= 
  for_end) do 
    eqtb [x ]= eqtb [12525 ];
  while ( x++ < for_end ) ;} 
  undumpint ( x ) ;
  if ( debugformatfile ) 
  {
    fprintf( stderr , "%s%s",  "fmtdebug:" , "mem_bot" ) ;
    fprintf( stderr , "%s%ld\n",  " = " , (long)x ) ;
  } 
  if ( x != membot ) 
  goto lab6666 ;
  undumpint ( memtop ) ;
  if ( debugformatfile ) 
  {
    fprintf( stderr , "%s%s",  "fmtdebug:" , "mem_top" ) ;
    fprintf( stderr , "%s%ld\n",  " = " , (long)memtop ) ;
  } 
  if ( membot + 1100 > memtop ) 
  goto lab6666 ;
  curlist .headfield = memtop - 1 ;
  curlist .tailfield = memtop - 1 ;
  pagetail = memtop - 2 ;
  memmin = membot - extramembot ;
  memmax = memtop + extramemtop ;
  xmallocarray ( yzmem , memmax - memmin ) ;
  zmem = yzmem - memmin ;
  mem = zmem ;
  undumpint ( x ) ;
  if ( x != 16009 ) 
  goto lab6666 ;
  undumpint ( x ) ;
  if ( x != 8501 ) 
  goto lab6666 ;
  undumpint ( x ) ;
  if ( x != 607 ) 
  goto lab6666 ;
  undumpint ( x ) ;
  if ( x != 1296847960L ) 
  goto lab6666 ;
  undumpint ( x ) ;
  if ( x == 1 ) 
  mltexenabledp = true ;
  else if ( x != 0 ) 
  goto lab6666 ;
  {
    undumpint ( x ) ;
    if ( x < 0 ) 
    goto lab6666 ;
    if ( x > suppoolsize - poolfree ) 
    {
      ;
      fprintf( stdout , "%s%s\n",  "---! Must increase the " , "string pool size" ) ;
      goto lab6666 ;
    } 
    else if ( debugformatfile ) 
    {
      fprintf( stderr , "%s%s",  "fmtdebug:" , "string pool size" ) ;
      fprintf( stderr , "%s%ld\n",  " = " , (long)x ) ;
    } 
    poolptr = x ;
  } 
  if ( poolsize < poolptr + poolfree ) 
  poolsize = poolptr + poolfree ;
  {
    undumpint ( x ) ;
    if ( x < 0 ) 
    goto lab6666 ;
    if ( x > supmaxstrings ) 
    {
      ;
      fprintf( stdout , "%s%s\n",  "---! Must increase the " , "sup strings" ) ;
      goto lab6666 ;
    } 
    else if ( debugformatfile ) 
    {
      fprintf( stderr , "%s%s",  "fmtdebug:" , "sup strings" ) ;
      fprintf( stderr , "%s%ld\n",  " = " , (long)x ) ;
    } 
    strptr = x ;
  } 
  xmallocarray ( strstart , maxstrings ) ;
  undumpcheckedthings ( 0 , poolptr , strstart [0 ], strptr + 1 ) ;
  xmallocarray ( strpool , poolsize ) ;
  undumpthings ( strpool [0 ], poolptr ) ;
  initstrptr = strptr ;
  initpoolptr = poolptr ;
  {
    undumpint ( x ) ;
    if ( ( x < membot + 1019 ) || ( x > memtop - 14 ) ) 
    goto lab6666 ;
    else lomemmax = x ;
  } 
  {
    undumpint ( x ) ;
    if ( ( x < membot + 20 ) || ( x > lomemmax ) ) 
    goto lab6666 ;
    else rover = x ;
  } 
  p = membot ;
  q = rover ;
  do {
      undumpthings ( mem [p ], q + 2 - p ) ;
    p = q + mem [q ].hh .v.LH ;
    if ( ( p > lomemmax ) || ( ( q >= mem [q + 1 ].hh .v.RH ) && ( mem [q + 
    1 ].hh .v.RH != rover ) ) ) 
    goto lab6666 ;
    q = mem [q + 1 ].hh .v.RH ;
  } while ( ! ( q == rover ) ) ;
  undumpthings ( mem [p ], lomemmax + 1 - p ) ;
  if ( memmin < membot - 2 ) 
  {
    p = mem [rover + 1 ].hh .v.LH ;
    q = memmin + 1 ;
    mem [memmin ].hh .v.RH = 0 ;
    mem [memmin ].hh .v.LH = 0 ;
    mem [p + 1 ].hh .v.RH = q ;
    mem [rover + 1 ].hh .v.LH = q ;
    mem [q + 1 ].hh .v.RH = rover ;
    mem [q + 1 ].hh .v.LH = p ;
    mem [q ].hh .v.RH = 268435455L ;
    mem [q ].hh .v.LH = membot - q ;
  } 
  {
    undumpint ( x ) ;
    if ( ( x < lomemmax + 1 ) || ( x > memtop - 13 ) ) 
    goto lab6666 ;
    else himemmin = x ;
  } 
  {
    undumpint ( x ) ;
    if ( ( x < 0 ) || ( x > memtop ) ) 
    goto lab6666 ;
    else avail = x ;
  } 
  memend = memtop ;
  undumpthings ( mem [himemmin ], memend + 1 - himemmin ) ;
  undumpint ( varused ) ;
  undumpint ( dynused ) ;
  k = 1 ;
  do {
      undumpint ( x ) ;
    if ( ( x < 1 ) || ( k + x > 16010 ) ) 
    goto lab6666 ;
    undumpthings ( eqtb [k ], x ) ;
    k = k + x ;
    undumpint ( x ) ;
    if ( ( x < 0 ) || ( k + x > 16010 ) ) 
    goto lab6666 ;
    {register integer for_end; j = k ;for_end = k + x - 1 ; if ( j <= 
    for_end) do 
      eqtb [j ]= eqtb [k - 1 ];
    while ( j++ < for_end ) ;} 
    k = k + x ;
  } while ( ! ( k > 16009 ) ) ;
  if ( hashhigh > 0 ) 
  undumpthings ( eqtb [16010 ], hashhigh ) ;
  {
    undumpint ( x ) ;
    if ( ( x < 514 ) || ( x > hashtop ) ) 
    goto lab6666 ;
    else parloc = x ;
  } 
  partoken = 4095 + parloc ;
  {
    undumpint ( x ) ;
    if ( ( x < 514 ) || ( x > hashtop ) ) 
    goto lab6666 ;
    else
    writeloc = x ;
  } 
  {
    undumpint ( x ) ;
    if ( ( x < 514 ) || ( x > 10514 ) ) 
    goto lab6666 ;
    else hashused = x ;
  } 
  p = 513 ;
  do {
      { 
      undumpint ( x ) ;
      if ( ( x < p + 1 ) || ( x > hashused ) ) 
      goto lab6666 ;
      else p = x ;
    } 
    undumphh ( hash [p ]) ;
  } while ( ! ( p == hashused ) ) ;
  undumpthings ( hash [hashused + 1 ], 12524 - hashused ) ;
  if ( debugformatfile ) 
  {
    printcsnames ( 514 , 12524 ) ;
  } 
  if ( hashhigh > 0 ) 
  {
    undumpthings ( hash [16010 ], hashhigh ) ;
    if ( debugformatfile ) 
    {
      printcsnames ( 16010 , hashhigh - ( 16010 ) ) ;
    } 
  } 
  undumpint ( cscount ) ;
  {
    undumpint ( x ) ;
    if ( x < 7 ) 
    goto lab6666 ;
    if ( x > supfontmemsize ) 
    {
      ;
      fprintf( stdout , "%s%s\n",  "---! Must increase the " , "font mem size" ) ;
      goto lab6666 ;
    } 
    else if ( debugformatfile ) 
    {
      fprintf( stderr , "%s%s",  "fmtdebug:" , "font mem size" ) ;
      fprintf( stderr , "%s%ld\n",  " = " , (long)x ) ;
    } 
    fmemptr = x ;
  } 
  if ( fmemptr > fontmemsize ) 
  fontmemsize = fmemptr ;
  xmallocarray ( fontinfo , fontmemsize ) ;
  undumpthings ( fontinfo [0 ], fmemptr ) ;
  {
    undumpint ( x ) ;
    if ( x < 0 ) 
    goto lab6666 ;
    if ( x > 2000 ) 
    {
      ;
      fprintf( stdout , "%s%s\n",  "---! Must increase the " , "font max" ) ;
      goto lab6666 ;
    } 
    else if ( debugformatfile ) 
    {
      fprintf( stderr , "%s%s",  "fmtdebug:" , "font max" ) ;
      fprintf( stderr , "%s%ld\n",  " = " , (long)x ) ;
    } 
    fontptr = x ;
  } 
  {
    xmallocarray ( fontcheck , fontmax ) ;
    xmallocarray ( fontsize , fontmax ) ;
    xmallocarray ( fontdsize , fontmax ) ;
    xmallocarray ( fontparams , fontmax ) ;
    xmallocarray ( fontname , fontmax ) ;
    xmallocarray ( fontarea , fontmax ) ;
    xmallocarray ( fontbc , fontmax ) ;
    xmallocarray ( fontec , fontmax ) ;
    xmallocarray ( fontglue , fontmax ) ;
    xmallocarray ( hyphenchar , fontmax ) ;
    xmallocarray ( skewchar , fontmax ) ;
    xmallocarray ( bcharlabel , fontmax ) ;
    xmallocarray ( fontbchar , fontmax ) ;
    xmallocarray ( fontfalsebchar , fontmax ) ;
    xmallocarray ( charbase , fontmax ) ;
    xmallocarray ( widthbase , fontmax ) ;
    xmallocarray ( heightbase , fontmax ) ;
    xmallocarray ( depthbase , fontmax ) ;
    xmallocarray ( italicbase , fontmax ) ;
    xmallocarray ( ligkernbase , fontmax ) ;
    xmallocarray ( kernbase , fontmax ) ;
    xmallocarray ( extenbase , fontmax ) ;
    xmallocarray ( parambase , fontmax ) ;
    undumpthings ( fontcheck [0 ], fontptr + 1 ) ;
    undumpthings ( fontsize [0 ], fontptr + 1 ) ;
    undumpthings ( fontdsize [0 ], fontptr + 1 ) ;
    undumpcheckedthings ( 0 , 268435455L , fontparams [0 ], fontptr + 1 ) ;
    undumpthings ( hyphenchar [0 ], fontptr + 1 ) ;
    undumpthings ( skewchar [0 ], fontptr + 1 ) ;
    undumpuppercheckthings ( strptr , fontname [0 ], fontptr + 1 ) ;
    undumpuppercheckthings ( strptr , fontarea [0 ], fontptr + 1 ) ;
    undumpthings ( fontbc [0 ], fontptr + 1 ) ;
    undumpthings ( fontec [0 ], fontptr + 1 ) ;
    undumpthings ( charbase [0 ], fontptr + 1 ) ;
    undumpthings ( widthbase [0 ], fontptr + 1 ) ;
    undumpthings ( heightbase [0 ], fontptr + 1 ) ;
    undumpthings ( depthbase [0 ], fontptr + 1 ) ;
    undumpthings ( italicbase [0 ], fontptr + 1 ) ;
    undumpthings ( ligkernbase [0 ], fontptr + 1 ) ;
    undumpthings ( kernbase [0 ], fontptr + 1 ) ;
    undumpthings ( extenbase [0 ], fontptr + 1 ) ;
    undumpthings ( parambase [0 ], fontptr + 1 ) ;
    undumpcheckedthings ( 0 , lomemmax , fontglue [0 ], fontptr + 1 ) ;
    undumpcheckedthings ( 0 , fmemptr - 1 , bcharlabel [0 ], fontptr + 1 ) ;
    undumpcheckedthings ( 0 , 256 , fontbchar [0 ], fontptr + 1 ) ;
    undumpcheckedthings ( 0 , 256 , fontfalsebchar [0 ], fontptr + 1 ) ;
  } 
  {
    undumpint ( x ) ;
    if ( x < 0 ) 
    goto lab6666 ;
    if ( x > hyphsize ) 
    {
      ;
      fprintf( stdout , "%s%s\n",  "---! Must increase the " , "hyph_size" ) ;
      goto lab6666 ;
    } 
    else if ( debugformatfile ) 
    {
      fprintf( stderr , "%s%s",  "fmtdebug:" , "hyph_size" ) ;
      fprintf( stderr , "%s%ld\n",  " = " , (long)x ) ;
    } 
    hyphcount = x ;
  } 
  {
    undumpint ( x ) ;
    if ( x < 607 ) 
    goto lab6666 ;
    if ( x > hyphsize ) 
    {
      ;
      fprintf( stdout , "%s%s\n",  "---! Must increase the " , "hyph_size" ) ;
      goto lab6666 ;
    } 
    else if ( debugformatfile ) 
    {
      fprintf( stderr , "%s%s",  "fmtdebug:" , "hyph_size" ) ;
      fprintf( stderr , "%s%ld\n",  " = " , (long)x ) ;
    } 
    hyphnext = x ;
  } 
  j = 0 ;
  {register integer for_end; k = 1 ;for_end = hyphcount ; if ( k <= for_end) 
  do 
    {
      undumpint ( j ) ;
      if ( j < 0 ) 
      goto lab6666 ;
      if ( j > 65535L ) 
      {
	hyphnext = j / 65536L ;
	j = j - hyphnext * 65536L ;
      } 
      else hyphnext = 0 ;
      if ( ( j >= hyphsize ) || ( hyphnext > hyphsize ) ) 
      goto lab6666 ;
      hyphlink [j ]= hyphnext ;
      {
	undumpint ( x ) ;
	if ( ( x < 0 ) || ( x > strptr ) ) 
	goto lab6666 ;
	else hyphword [j ]= x ;
      } 
      {
	undumpint ( x ) ;
	if ( ( x < 0 ) || ( x > 268435455L ) ) 
	goto lab6666 ;
	else hyphlist [j ]= x ;
      } 
    } 
  while ( k++ < for_end ) ;} 
  incr ( j ) ;
  if ( j < 607 ) 
  j = 607 ;
  hyphnext = j ;
  if ( hyphnext >= hyphsize ) 
  hyphnext = 607 ;
  else if ( hyphnext >= 607 ) 
  incr ( hyphnext ) ;
  {
    undumpint ( x ) ;
    if ( x < 0 ) 
    goto lab6666 ;
    if ( x > triesize ) 
    {
      ;
      fprintf( stdout , "%s%s\n",  "---! Must increase the " , "trie size" ) ;
      goto lab6666 ;
    } 
    else if ( debugformatfile ) 
    {
      fprintf( stderr , "%s%s",  "fmtdebug:" , "trie size" ) ;
      fprintf( stderr , "%s%ld\n",  " = " , (long)x ) ;
    } 
    j = x ;
  } 
	;
#ifdef INITEX
  triemax = j ;
#endif /* INITEX */
  if ( ! trietrl ) 
  xmallocarray ( trietrl , j + 1 ) ;
  undumpthings ( trietrl [0 ], j + 1 ) ;
  if ( ! trietro ) 
  xmallocarray ( trietro , j + 1 ) ;
  undumpthings ( trietro [0 ], j + 1 ) ;
  if ( ! trietrc ) 
  xmallocarray ( trietrc , j + 1 ) ;
  undumpthings ( trietrc [0 ], j + 1 ) ;
  {
    undumpint ( x ) ;
    if ( x < 0 ) 
    goto lab6666 ;
    if ( x > trieopsize ) 
    {
      ;
      fprintf( stdout , "%s%s\n",  "---! Must increase the " , "trie op size" ) ;
      goto lab6666 ;
    } 
    else if ( debugformatfile ) 
    {
      fprintf( stderr , "%s%s",  "fmtdebug:" , "trie op size" ) ;
      fprintf( stderr , "%s%ld\n",  " = " , (long)x ) ;
    } 
    j = x ;
  } 
	;
#ifdef INITEX
  trieopptr = j ;
#endif /* INITEX */
  undumpthings ( hyfdistance [1 ], j ) ;
  undumpthings ( hyfnum [1 ], j ) ;
  undumpuppercheckthings ( maxtrieop , hyfnext [1 ], j ) ;
	;
#ifdef INITEX
  {register integer for_end; k = 0 ;for_end = 255 ; if ( k <= for_end) do 
    trieused [k ]= 0 ;
  while ( k++ < for_end ) ;} 
#endif /* INITEX */
  k = 256 ;
  while ( j > 0 ) {
      
    {
      undumpint ( x ) ;
      if ( ( x < 0 ) || ( x > k - 1 ) ) 
      goto lab6666 ;
      else k = x ;
    } 
    {
      undumpint ( x ) ;
      if ( ( x < 1 ) || ( x > j ) ) 
      goto lab6666 ;
      else x = x ;
    } 
	;
#ifdef INITEX
    trieused [k ]= x ;
#endif /* INITEX */
    j = j - x ;
    opstart [k ]= j ;
  } 
	;
#ifdef INITEX
  trienotready = false 
#endif /* INITEX */
  ;
  {
    undumpint ( x ) ;
    if ( ( x < 0 ) || ( x > 3 ) ) 
    goto lab6666 ;
    else interaction = x ;
  } 
  if ( interactionoption != 4 ) 
  interaction = interactionoption ;
  {
    undumpint ( x ) ;
    if ( ( x < 0 ) || ( x > strptr ) ) 
    goto lab6666 ;
    else formatident = x ;
  } 
  undumpint ( x ) ;
  if ( ( x != 69069L ) || feof ( fmtfile ) ) 
  goto lab6666 ;
  Result = true ;
  return Result ;
  lab6666: ;
  fprintf( stdout , "%s\n",  "(Fatal format file error; I'm stymied)" ) ;
  Result = false ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
finalcleanup ( void ) 
#else
finalcleanup ( ) 
#endif
{
  /* 10 */ finalcleanup_regmem 
  smallnumber c  ;
  c = curchr ;
  if ( jobname == 0 ) 
  openlogfile () ;
  while ( inputptr > 0 ) if ( curinput .statefield == 0 ) 
  endtokenlist () ;
  else endfilereading () ;
  while ( openparens > 0 ) {
      
    print ( 1274 ) ;
    decr ( openparens ) ;
  } 
  if ( curlevel > 1 ) 
  {
    printnl ( 40 ) ;
    printesc ( 1275 ) ;
    print ( 1276 ) ;
    printint ( curlevel - 1 ) ;
    printchar ( 41 ) ;
  } 
  while ( condptr != 0 ) {
      
    printnl ( 40 ) ;
    printesc ( 1275 ) ;
    print ( 1277 ) ;
    printcmdchr ( 105 , curif ) ;
    if ( ifline != 0 ) 
    {
      print ( 1278 ) ;
      printint ( ifline ) ;
    } 
    print ( 1279 ) ;
    ifline = mem [condptr + 1 ].cint ;
    curif = mem [condptr ].hh.b1 ;
    tempptr = condptr ;
    condptr = mem [condptr ].hh .v.RH ;
    freenode ( tempptr , 2 ) ;
  } 
  if ( history != 0 ) 
  if ( ( ( history == 1 ) || ( interaction < 3 ) ) ) 
  if ( selector == 19 ) 
  {
    selector = 17 ;
    printnl ( 1280 ) ;
    selector = 19 ;
  } 
  if ( c == 1 ) 
  {
	;
#ifdef INITEX
    if ( iniversion ) 
    {
      {register integer for_end; c = 0 ;for_end = 4 ; if ( c <= for_end) do 
	if ( curmark [c ]!= 0 ) 
	deletetokenref ( curmark [c ]) ;
      while ( c++ < for_end ) ;} 
      storefmtfile () ;
      return ;
    } 
#endif /* INITEX */
    printnl ( 1281 ) ;
    return ;
  } 
} 
#ifdef INITEX
void 
#ifdef HAVE_PROTOTYPES
initprim ( void ) 
#else
initprim ( ) 
#endif
{
  initprim_regmem 
  nonewcontrolsequence = false ;
  primitive ( 373 , 75 , 12526 ) ;
  primitive ( 374 , 75 , 12527 ) ;
  primitive ( 375 , 75 , 12528 ) ;
  primitive ( 376 , 75 , 12529 ) ;
  primitive ( 377 , 75 , 12530 ) ;
  primitive ( 378 , 75 , 12531 ) ;
  primitive ( 379 , 75 , 12532 ) ;
  primitive ( 380 , 75 , 12533 ) ;
  primitive ( 381 , 75 , 12534 ) ;
  primitive ( 382 , 75 , 12535 ) ;
  primitive ( 383 , 75 , 12536 ) ;
  primitive ( 384 , 75 , 12537 ) ;
  primitive ( 385 , 75 , 12538 ) ;
  primitive ( 386 , 75 , 12539 ) ;
  primitive ( 387 , 75 , 12540 ) ;
  primitive ( 388 , 76 , 12541 ) ;
  primitive ( 389 , 76 , 12542 ) ;
  primitive ( 390 , 76 , 12543 ) ;
  primitive ( 395 , 72 , 13057 ) ;
  primitive ( 396 , 72 , 13058 ) ;
  primitive ( 397 , 72 , 13059 ) ;
  primitive ( 398 , 72 , 13060 ) ;
  primitive ( 399 , 72 , 13061 ) ;
  primitive ( 400 , 72 , 13062 ) ;
  primitive ( 401 , 72 , 13063 ) ;
  primitive ( 402 , 72 , 13064 ) ;
  primitive ( 403 , 72 , 13065 ) ;
  primitive ( 417 , 73 , 15163 ) ;
  primitive ( 418 , 73 , 15164 ) ;
  primitive ( 419 , 73 , 15165 ) ;
  primitive ( 420 , 73 , 15166 ) ;
  primitive ( 421 , 73 , 15167 ) ;
  primitive ( 422 , 73 , 15168 ) ;
  primitive ( 423 , 73 , 15169 ) ;
  primitive ( 424 , 73 , 15170 ) ;
  primitive ( 425 , 73 , 15171 ) ;
  primitive ( 426 , 73 , 15172 ) ;
  primitive ( 427 , 73 , 15173 ) ;
  primitive ( 428 , 73 , 15174 ) ;
  primitive ( 429 , 73 , 15175 ) ;
  primitive ( 430 , 73 , 15176 ) ;
  primitive ( 431 , 73 , 15177 ) ;
  primitive ( 432 , 73 , 15178 ) ;
  primitive ( 433 , 73 , 15179 ) ;
  primitive ( 434 , 73 , 15180 ) ;
  primitive ( 435 , 73 , 15181 ) ;
  primitive ( 436 , 73 , 15182 ) ;
  primitive ( 437 , 73 , 15183 ) ;
  primitive ( 438 , 73 , 15184 ) ;
  primitive ( 439 , 73 , 15185 ) ;
  primitive ( 440 , 73 , 15186 ) ;
  primitive ( 441 , 73 , 15187 ) ;
  primitive ( 442 , 73 , 15188 ) ;
  primitive ( 443 , 73 , 15189 ) ;
  primitive ( 444 , 73 , 15190 ) ;
  primitive ( 445 , 73 , 15191 ) ;
  primitive ( 446 , 73 , 15192 ) ;
  primitive ( 447 , 73 , 15193 ) ;
  primitive ( 448 , 73 , 15194 ) ;
  primitive ( 449 , 73 , 15195 ) ;
  primitive ( 450 , 73 , 15196 ) ;
  primitive ( 451 , 73 , 15197 ) ;
  primitive ( 452 , 73 , 15198 ) ;
  primitive ( 453 , 73 , 15199 ) ;
  primitive ( 454 , 73 , 15200 ) ;
  primitive ( 455 , 73 , 15201 ) ;
  primitive ( 456 , 73 , 15202 ) ;
  primitive ( 457 , 73 , 15203 ) ;
  primitive ( 458 , 73 , 15204 ) ;
  primitive ( 459 , 73 , 15205 ) ;
  primitive ( 460 , 73 , 15206 ) ;
  primitive ( 461 , 73 , 15207 ) ;
  primitive ( 462 , 73 , 15208 ) ;
  primitive ( 463 , 73 , 15209 ) ;
  primitive ( 464 , 73 , 15210 ) ;
  primitive ( 465 , 73 , 15211 ) ;
  primitive ( 466 , 73 , 15212 ) ;
  primitive ( 467 , 73 , 15213 ) ;
  primitive ( 468 , 73 , 15214 ) ;
  primitive ( 469 , 73 , 15215 ) ;
  primitive ( 470 , 73 , 15216 ) ;
  primitive ( 471 , 73 , 15217 ) ;
  if ( mltexp ) 
  {
    mltexenabledp = true ;
    if ( false ) 
    primitive ( 472 , 73 , 15218 ) ;
    primitive ( 473 , 73 , 15219 ) ;
    primitive ( 474 , 73 , 15220 ) ;
  } 
  primitive ( 478 , 74 , 15733 ) ;
  primitive ( 479 , 74 , 15734 ) ;
  primitive ( 480 , 74 , 15735 ) ;
  primitive ( 481 , 74 , 15736 ) ;
  primitive ( 482 , 74 , 15737 ) ;
  primitive ( 483 , 74 , 15738 ) ;
  primitive ( 484 , 74 , 15739 ) ;
  primitive ( 485 , 74 , 15740 ) ;
  primitive ( 486 , 74 , 15741 ) ;
  primitive ( 487 , 74 , 15742 ) ;
  primitive ( 488 , 74 , 15743 ) ;
  primitive ( 489 , 74 , 15744 ) ;
  primitive ( 490 , 74 , 15745 ) ;
  primitive ( 491 , 74 , 15746 ) ;
  primitive ( 492 , 74 , 15747 ) ;
  primitive ( 493 , 74 , 15748 ) ;
  primitive ( 494 , 74 , 15749 ) ;
  primitive ( 495 , 74 , 15750 ) ;
  primitive ( 496 , 74 , 15751 ) ;
  primitive ( 497 , 74 , 15752 ) ;
  primitive ( 498 , 74 , 15753 ) ;
  primitive ( 32 , 64 , 0 ) ;
  primitive ( 47 , 44 , 0 ) ;
  primitive ( 508 , 45 , 0 ) ;
  primitive ( 509 , 90 , 0 ) ;
  primitive ( 510 , 40 , 0 ) ;
  primitive ( 511 , 41 , 0 ) ;
  primitive ( 512 , 61 , 0 ) ;
  primitive ( 513 , 16 , 0 ) ;
  primitive ( 504 , 107 , 0 ) ;
  primitive ( 514 , 15 , 0 ) ;
  primitive ( 515 , 92 , 0 ) ;
  primitive ( 505 , 67 , 0 ) ;
  primitive ( 516 , 62 , 0 ) ;
  hash [10516 ].v.RH = 516 ;
  eqtb [10516 ]= eqtb [curval ];
  primitive ( 517 , 102 , 0 ) ;
  primitive ( 518 , 88 , 0 ) ;
  primitive ( 519 , 77 , 0 ) ;
  primitive ( 520 , 32 , 0 ) ;
  primitive ( 521 , 36 , 0 ) ;
  primitive ( 522 , 39 , 0 ) ;
  primitive ( 327 , 37 , 0 ) ;
  primitive ( 348 , 18 , 0 ) ;
  primitive ( 523 , 46 , 0 ) ;
  primitive ( 524 , 17 , 0 ) ;
  primitive ( 525 , 54 , 0 ) ;
  primitive ( 526 , 91 , 0 ) ;
  primitive ( 527 , 34 , 0 ) ;
  primitive ( 528 , 65 , 0 ) ;
  primitive ( 529 , 103 , 0 ) ;
  primitive ( 332 , 55 , 0 ) ;
  primitive ( 530 , 63 , 0 ) ;
  primitive ( 405 , 84 , 0 ) ;
  primitive ( 531 , 42 , 0 ) ;
  primitive ( 532 , 80 , 0 ) ;
  primitive ( 533 , 66 , 0 ) ;
  primitive ( 534 , 96 , 0 ) ;
  primitive ( 535 , 0 , 256 ) ;
  hash [10521 ].v.RH = 535 ;
  eqtb [10521 ]= eqtb [curval ];
  primitive ( 536 , 98 , 0 ) ;
  primitive ( 537 , 109 , 0 ) ;
  primitive ( 404 , 71 , 0 ) ;
  primitive ( 349 , 38 , 0 ) ;
  primitive ( 538 , 33 , 0 ) ;
  primitive ( 539 , 56 , 0 ) ;
  primitive ( 540 , 35 , 0 ) ;
  primitive ( 596 , 13 , 256 ) ;
  parloc = curval ;
  partoken = 4095 + parloc ;
  primitive ( 628 , 104 , 0 ) ;
  primitive ( 629 , 104 , 1 ) ;
  primitive ( 630 , 110 , 0 ) ;
  primitive ( 631 , 110 , 1 ) ;
  primitive ( 632 , 110 , 2 ) ;
  primitive ( 633 , 110 , 3 ) ;
  primitive ( 634 , 110 , 4 ) ;
  primitive ( 476 , 89 , 0 ) ;
  primitive ( 500 , 89 , 1 ) ;
  primitive ( 392 , 89 , 2 ) ;
  primitive ( 393 , 89 , 3 ) ;
  primitive ( 667 , 79 , 102 ) ;
  primitive ( 668 , 79 , 1 ) ;
  primitive ( 669 , 82 , 0 ) ;
  primitive ( 670 , 82 , 1 ) ;
  primitive ( 671 , 83 , 1 ) ;
  primitive ( 672 , 83 , 3 ) ;
  primitive ( 673 , 83 , 2 ) ;
  primitive ( 674 , 70 , 0 ) ;
  primitive ( 675 , 70 , 1 ) ;
  primitive ( 676 , 70 , 2 ) ;
  primitive ( 677 , 70 , 3 ) ;
  primitive ( 678 , 70 , 4 ) ;
  primitive ( 734 , 108 , 0 ) ;
  primitive ( 735 , 108 , 1 ) ;
  primitive ( 736 , 108 , 2 ) ;
  primitive ( 737 , 108 , 3 ) ;
  primitive ( 738 , 108 , 4 ) ;
  primitive ( 739 , 108 , 5 ) ;
  primitive ( 755 , 105 , 0 ) ;
  primitive ( 756 , 105 , 1 ) ;
  primitive ( 757 , 105 , 2 ) ;
  primitive ( 758 , 105 , 3 ) ;
  primitive ( 759 , 105 , 4 ) ;
  primitive ( 760 , 105 , 5 ) ;
  primitive ( 761 , 105 , 6 ) ;
  primitive ( 762 , 105 , 7 ) ;
  primitive ( 763 , 105 , 8 ) ;
  primitive ( 764 , 105 , 9 ) ;
  primitive ( 765 , 105 , 10 ) ;
  primitive ( 766 , 105 , 11 ) ;
  primitive ( 767 , 105 , 12 ) ;
  primitive ( 768 , 105 , 13 ) ;
  primitive ( 769 , 105 , 14 ) ;
  primitive ( 770 , 105 , 15 ) ;
  primitive ( 771 , 105 , 16 ) ;
  primitive ( 772 , 106 , 2 ) ;
  hash [10518 ].v.RH = 772 ;
  eqtb [10518 ]= eqtb [curval ];
  primitive ( 773 , 106 , 4 ) ;
  primitive ( 774 , 106 , 3 ) ;
  primitive ( 797 , 87 , 0 ) ;
  hash [10524 ].v.RH = 797 ;
  eqtb [10524 ]= eqtb [curval ];
  primitive ( 893 , 4 , 256 ) ;
  primitive ( 894 , 5 , 257 ) ;
  hash [10515 ].v.RH = 894 ;
  eqtb [10515 ]= eqtb [curval ];
  primitive ( 895 , 5 , 258 ) ;
  hash [10519 ].v.RH = 896 ;
  hash [10520 ].v.RH = 896 ;
  eqtb [10520 ].hh.b0 = 9 ;
  eqtb [10520 ].hh .v.RH = memtop - 11 ;
  eqtb [10520 ].hh.b1 = 1 ;
  eqtb [10519 ]= eqtb [10520 ];
  eqtb [10519 ].hh.b0 = 115 ;
  primitive ( 965 , 81 , 0 ) ;
  primitive ( 966 , 81 , 1 ) ;
  primitive ( 967 , 81 , 2 ) ;
  primitive ( 968 , 81 , 3 ) ;
  primitive ( 969 , 81 , 4 ) ;
  primitive ( 970 , 81 , 5 ) ;
  primitive ( 971 , 81 , 6 ) ;
  primitive ( 972 , 81 , 7 ) ;
  primitive ( 1020 , 14 , 0 ) ;
  primitive ( 1021 , 14 , 1 ) ;
  primitive ( 1022 , 26 , 4 ) ;
  primitive ( 1023 , 26 , 0 ) ;
  primitive ( 1024 , 26 , 1 ) ;
  primitive ( 1025 , 26 , 2 ) ;
  primitive ( 1026 , 26 , 3 ) ;
  primitive ( 1027 , 27 , 4 ) ;
  primitive ( 1028 , 27 , 0 ) ;
  primitive ( 1029 , 27 , 1 ) ;
  primitive ( 1030 , 27 , 2 ) ;
  primitive ( 1031 , 27 , 3 ) ;
  primitive ( 333 , 28 , 5 ) ;
  primitive ( 337 , 29 , 1 ) ;
  primitive ( 339 , 30 , 99 ) ;
  primitive ( 1049 , 21 , 1 ) ;
  primitive ( 1050 , 21 , 0 ) ;
  primitive ( 1051 , 22 , 1 ) ;
  primitive ( 1052 , 22 , 0 ) ;
  primitive ( 406 , 20 , 0 ) ;
  primitive ( 1053 , 20 , 1 ) ;
  primitive ( 1054 , 20 , 2 ) ;
  primitive ( 960 , 20 , 3 ) ;
  primitive ( 1055 , 20 , 4 ) ;
  primitive ( 962 , 20 , 5 ) ;
  primitive ( 1056 , 20 , 106 ) ;
  primitive ( 1057 , 31 , 99 ) ;
  primitive ( 1058 , 31 , 100 ) ;
  primitive ( 1059 , 31 , 101 ) ;
  primitive ( 1060 , 31 , 102 ) ;
  primitive ( 1075 , 43 , 1 ) ;
  primitive ( 1076 , 43 , 0 ) ;
  primitive ( 1085 , 25 , 12 ) ;
  primitive ( 1086 , 25 , 11 ) ;
  primitive ( 1087 , 25 , 10 ) ;
  primitive ( 1088 , 23 , 0 ) ;
  primitive ( 1089 , 23 , 1 ) ;
  primitive ( 1090 , 24 , 0 ) ;
  primitive ( 1091 , 24 , 1 ) ;
  primitive ( 45 , 47 , 1 ) ;
  primitive ( 346 , 47 , 0 ) ;
  primitive ( 1122 , 48 , 0 ) ;
  primitive ( 1123 , 48 , 1 ) ;
  primitive ( 861 , 50 , 16 ) ;
  primitive ( 862 , 50 , 17 ) ;
  primitive ( 863 , 50 , 18 ) ;
  primitive ( 864 , 50 , 19 ) ;
  primitive ( 865 , 50 , 20 ) ;
  primitive ( 866 , 50 , 21 ) ;
  primitive ( 867 , 50 , 22 ) ;
  primitive ( 868 , 50 , 23 ) ;
  primitive ( 870 , 50 , 26 ) ;
  primitive ( 869 , 50 , 27 ) ;
  primitive ( 1124 , 51 , 0 ) ;
  primitive ( 873 , 51 , 1 ) ;
  primitive ( 874 , 51 , 2 ) ;
  primitive ( 856 , 53 , 0 ) ;
  primitive ( 857 , 53 , 2 ) ;
  primitive ( 858 , 53 , 4 ) ;
  primitive ( 859 , 53 , 6 ) ;
  primitive ( 1142 , 52 , 0 ) ;
  primitive ( 1143 , 52 , 1 ) ;
  primitive ( 1144 , 52 , 2 ) ;
  primitive ( 1145 , 52 , 3 ) ;
  primitive ( 1146 , 52 , 4 ) ;
  primitive ( 1147 , 52 , 5 ) ;
  primitive ( 871 , 49 , 30 ) ;
  primitive ( 872 , 49 , 31 ) ;
  hash [10517 ].v.RH = 872 ;
  eqtb [10517 ]= eqtb [curval ];
  primitive ( 1166 , 93 , 1 ) ;
  primitive ( 1167 , 93 , 2 ) ;
  primitive ( 1168 , 93 , 4 ) ;
  primitive ( 1169 , 97 , 0 ) ;
  primitive ( 1170 , 97 , 1 ) ;
  primitive ( 1171 , 97 , 2 ) ;
  primitive ( 1172 , 97 , 3 ) ;
  primitive ( 1186 , 94 , 0 ) ;
  primitive ( 1187 , 94 , 1 ) ;
  primitive ( 1188 , 95 , 0 ) ;
  primitive ( 1189 , 95 , 1 ) ;
  primitive ( 1190 , 95 , 2 ) ;
  primitive ( 1191 , 95 , 3 ) ;
  primitive ( 1192 , 95 , 4 ) ;
  primitive ( 1193 , 95 , 5 ) ;
  primitive ( 1194 , 95 , 6 ) ;
  if ( mltexp ) 
  {
    primitive ( 1195 , 95 , 7 ) ;
  } 
  primitive ( 412 , 85 , 13627 ) ;
  primitive ( 416 , 85 , 14651 ) ;
  primitive ( 413 , 85 , 13883 ) ;
  primitive ( 414 , 85 , 14139 ) ;
  primitive ( 415 , 85 , 14395 ) ;
  primitive ( 477 , 85 , 15477 ) ;
  primitive ( 409 , 86 , 13579 ) ;
  primitive ( 410 , 86 , 13595 ) ;
  primitive ( 411 , 86 , 13611 ) ;
  primitive ( 936 , 99 , 0 ) ;
  primitive ( 948 , 99 , 1 ) ;
  primitive ( 1215 , 78 , 0 ) ;
  primitive ( 1216 , 78 , 1 ) ;
  primitive ( 272 , 100 , 0 ) ;
  primitive ( 273 , 100 , 1 ) ;
  primitive ( 274 , 100 , 2 ) ;
  primitive ( 1225 , 100 , 3 ) ;
  primitive ( 1226 , 60 , 1 ) ;
  primitive ( 1227 , 60 , 0 ) ;
  primitive ( 1228 , 58 , 0 ) ;
  primitive ( 1229 , 58 , 1 ) ;
  primitive ( 1235 , 57 , 13883 ) ;
  primitive ( 1236 , 57 , 14139 ) ;
  primitive ( 1237 , 19 , 0 ) ;
  primitive ( 1238 , 19 , 1 ) ;
  primitive ( 1239 , 19 , 2 ) ;
  primitive ( 1240 , 19 , 3 ) ;
  primitive ( 1283 , 59 , 0 ) ;
  primitive ( 593 , 59 , 1 ) ;
  writeloc = curval ;
  primitive ( 1284 , 59 , 2 ) ;
  primitive ( 1285 , 59 , 3 ) ;
  primitive ( 1286 , 59 , 4 ) ;
  primitive ( 1287 , 59 , 5 ) ;
  nonewcontrolsequence = true ;
} 
#endif /* INITEX */
void 
#ifdef HAVE_PROTOTYPES
mainbody ( void ) 
#else
mainbody ( ) 
#endif
{
  mainbody_regmem 
  bounddefault = 250000L ;
  boundname = "main_memory" ;
  setupboundvariable ( addressof ( mainmemory ) , boundname , bounddefault ) ;
  bounddefault = 0 ;
  boundname = "extra_mem_top" ;
  setupboundvariable ( addressof ( extramemtop ) , boundname , bounddefault ) 
  ;
  bounddefault = 0 ;
  boundname = "extra_mem_bot" ;
  setupboundvariable ( addressof ( extramembot ) , boundname , bounddefault ) 
  ;
  bounddefault = 100000L ;
  boundname = "pool_size" ;
  setupboundvariable ( addressof ( poolsize ) , boundname , bounddefault ) ;
  bounddefault = 75000L ;
  boundname = "string_vacancies" ;
  setupboundvariable ( addressof ( stringvacancies ) , boundname , 
  bounddefault ) ;
  bounddefault = 5000 ;
  boundname = "pool_free" ;
  setupboundvariable ( addressof ( poolfree ) , boundname , bounddefault ) ;
  bounddefault = 15000 ;
  boundname = "max_strings" ;
  setupboundvariable ( addressof ( maxstrings ) , boundname , bounddefault ) ;
  bounddefault = 100000L ;
  boundname = "font_mem_size" ;
  setupboundvariable ( addressof ( fontmemsize ) , boundname , bounddefault ) 
  ;
  bounddefault = 500 ;
  boundname = "font_max" ;
  setupboundvariable ( addressof ( fontmax ) , boundname , bounddefault ) ;
  bounddefault = 20000 ;
  boundname = "trie_size" ;
  setupboundvariable ( addressof ( triesize ) , boundname , bounddefault ) ;
  bounddefault = 659 ;
  boundname = "hyph_size" ;
  setupboundvariable ( addressof ( hyphsize ) , boundname , bounddefault ) ;
  bounddefault = 3000 ;
  boundname = "buf_size" ;
  setupboundvariable ( addressof ( bufsize ) , boundname , bounddefault ) ;
  bounddefault = 50 ;
  boundname = "nest_size" ;
  setupboundvariable ( addressof ( nestsize ) , boundname , bounddefault ) ;
  bounddefault = 15 ;
  boundname = "max_in_open" ;
  setupboundvariable ( addressof ( maxinopen ) , boundname , bounddefault ) ;
  bounddefault = 60 ;
  boundname = "param_size" ;
  setupboundvariable ( addressof ( paramsize ) , boundname , bounddefault ) ;
  bounddefault = 4000 ;
  boundname = "save_size" ;
  setupboundvariable ( addressof ( savesize ) , boundname , bounddefault ) ;
  bounddefault = 300 ;
  boundname = "stack_size" ;
  setupboundvariable ( addressof ( stacksize ) , boundname , bounddefault ) ;
  bounddefault = 16384 ;
  boundname = "dvi_buf_size" ;
  setupboundvariable ( addressof ( dvibufsize ) , boundname , bounddefault ) ;
  bounddefault = 79 ;
  boundname = "error_line" ;
  setupboundvariable ( addressof ( errorline ) , boundname , bounddefault ) ;
  bounddefault = 50 ;
  boundname = "half_error_line" ;
  setupboundvariable ( addressof ( halferrorline ) , boundname , bounddefault 
  ) ;
  bounddefault = 79 ;
  boundname = "max_print_line" ;
  setupboundvariable ( addressof ( maxprintline ) , boundname , bounddefault ) 
  ;
  bounddefault = 0 ;
  boundname = "hash_extra" ;
  setupboundvariable ( addressof ( hashextra ) , boundname , bounddefault ) ;
  {
    if ( mainmemory < infmainmemory ) 
    mainmemory = infmainmemory ;
    else if ( mainmemory > supmainmemory ) 
    mainmemory = supmainmemory ;
  } 
	;
#ifdef INITEX
  if ( iniversion ) 
  {
    extramemtop = 0 ;
    extramembot = 0 ;
  } 
#endif /* INITEX */
  if ( extramembot > membot ) 
  extramembot = membot ;
  if ( extramembot > supmainmemory ) 
  extramembot = supmainmemory ;
  if ( extramemtop > supmainmemory ) 
  extramemtop = supmainmemory ;
  memtop = membot + mainmemory ;
  memmin = membot ;
  memmax = memtop ;
  {
    if ( triesize < inftriesize ) 
    triesize = inftriesize ;
    else if ( triesize > suptriesize ) 
    triesize = suptriesize ;
  } 
  {
    if ( hyphsize < infhyphsize ) 
    hyphsize = infhyphsize ;
    else if ( hyphsize > suphyphsize ) 
    hyphsize = suphyphsize ;
  } 
  {
    if ( bufsize < infbufsize ) 
    bufsize = infbufsize ;
    else if ( bufsize > supbufsize ) 
    bufsize = supbufsize ;
  } 
  {
    if ( nestsize < infnestsize ) 
    nestsize = infnestsize ;
    else if ( nestsize > supnestsize ) 
    nestsize = supnestsize ;
  } 
  {
    if ( maxinopen < infmaxinopen ) 
    maxinopen = infmaxinopen ;
    else if ( maxinopen > supmaxinopen ) 
    maxinopen = supmaxinopen ;
  } 
  {
    if ( paramsize < infparamsize ) 
    paramsize = infparamsize ;
    else if ( paramsize > supparamsize ) 
    paramsize = supparamsize ;
  } 
  {
    if ( savesize < infsavesize ) 
    savesize = infsavesize ;
    else if ( savesize > supsavesize ) 
    savesize = supsavesize ;
  } 
  {
    if ( stacksize < infstacksize ) 
    stacksize = infstacksize ;
    else if ( stacksize > supstacksize ) 
    stacksize = supstacksize ;
  } 
  {
    if ( dvibufsize < infdvibufsize ) 
    dvibufsize = infdvibufsize ;
    else if ( dvibufsize > supdvibufsize ) 
    dvibufsize = supdvibufsize ;
  } 
  {
    if ( poolsize < infpoolsize ) 
    poolsize = infpoolsize ;
    else if ( poolsize > suppoolsize ) 
    poolsize = suppoolsize ;
  } 
  {
    if ( stringvacancies < infstringvacancies ) 
    stringvacancies = infstringvacancies ;
    else if ( stringvacancies > supstringvacancies ) 
    stringvacancies = supstringvacancies ;
  } 
  {
    if ( poolfree < infpoolfree ) 
    poolfree = infpoolfree ;
    else if ( poolfree > suppoolfree ) 
    poolfree = suppoolfree ;
  } 
  {
    if ( maxstrings < infmaxstrings ) 
    maxstrings = infmaxstrings ;
    else if ( maxstrings > supmaxstrings ) 
    maxstrings = supmaxstrings ;
  } 
  {
    if ( fontmemsize < inffontmemsize ) 
    fontmemsize = inffontmemsize ;
    else if ( fontmemsize > supfontmemsize ) 
    fontmemsize = supfontmemsize ;
  } 
  {
    if ( fontmax < inffontmax ) 
    fontmax = inffontmax ;
    else if ( fontmax > supfontmax ) 
    fontmax = supfontmax ;
  } 
  {
    if ( hashextra < infhashextra ) 
    hashextra = infhashextra ;
    else if ( hashextra > suphashextra ) 
    hashextra = suphashextra ;
  } 
  if ( errorline > 255 ) 
  errorline = 255 ;
  xmallocarray ( buffer , bufsize ) ;
  xmallocarray ( nest , nestsize ) ;
  xmallocarray ( savestack , savesize ) ;
  xmallocarray ( inputstack , stacksize ) ;
  xmallocarray ( inputfile , maxinopen ) ;
  xmallocarray ( linestack , maxinopen ) ;
  xmallocarray ( paramstack , paramsize ) ;
  xmallocarray ( dvibuf , dvibufsize ) ;
  xmallocarray ( hyphword , hyphsize ) ;
  xmallocarray ( hyphlist , hyphsize ) ;
  xmallocarray ( hyphlink , hyphsize ) ;
	;
#ifdef INITEX
  if ( iniversion ) 
  {
    xmallocarray ( yzmem , memtop - membot ) ;
    zmem = yzmem - membot ;
    eqtbtop = 16009 + hashextra ;
    if ( hashextra == 0 ) 
    hashtop = 12525 ;
    else hashtop = eqtbtop ;
    xmallocarray ( yhash , 1 + hashtop - hashoffset ) ;
    hash = yhash - hashoffset ;
    hash [514 ].v.LH = 0 ;
    hash [514 ].v.RH = 0 ;
    {register integer for_end; hashused = 515 ;for_end = hashtop ; if ( 
    hashused <= for_end) do 
      hash [hashused ]= hash [514 ];
    while ( hashused++ < for_end ) ;} 
    xmallocarray ( zeqtb , eqtbtop ) ;
    eqtb = zeqtb ;
    xmallocarray ( strstart , maxstrings ) ;
    xmallocarray ( strpool , poolsize ) ;
    xmallocarray ( fontinfo , fontmemsize ) ;
  } 
#endif /* INITEX */
  history = 3 ;
  if ( readyalready == 314159L ) 
  goto lab1 ;
  bad = 0 ;
  if ( ( halferrorline < 30 ) || ( halferrorline > errorline - 15 ) ) 
  bad = 1 ;
  if ( maxprintline < 60 ) 
  bad = 2 ;
  if ( dvibufsize % 8 != 0 ) 
  bad = 3 ;
  if ( membot + 1100 > memtop ) 
  bad = 4 ;
  if ( 8501 > 10000 ) 
  bad = 5 ;
  if ( maxinopen >= 128 ) 
  bad = 6 ;
  if ( memtop < 267 ) 
  bad = 7 ;
	;
#ifdef INITEX
  if ( ( memmin != membot ) || ( memmax != memtop ) ) 
  bad = 10 ;
#endif /* INITEX */
  if ( ( memmin > membot ) || ( memmax < memtop ) ) 
  bad = 10 ;
  if ( ( 0 > 0 ) || ( 255 < 127 ) ) 
  bad = 11 ;
  if ( ( 0 > 0 ) || ( 268435455L < 32767 ) ) 
  bad = 12 ;
  if ( ( 0 < 0 ) || ( 255 > 268435455L ) ) 
  bad = 13 ;
  if ( ( memmin < 0 ) || ( memmax >= 268435455L ) || ( membot - memmin > 
  268435456L ) ) 
  bad = 14 ;
  if ( ( 2000 < 0 ) || ( 2000 > 268435455L ) ) 
  bad = 15 ;
  if ( fontmax > 2000 ) 
  bad = 16 ;
  if ( ( savesize > 268435455L ) || ( maxstrings > 268435455L ) ) 
  bad = 17 ;
  if ( bufsize > 268435455L ) 
  bad = 18 ;
  if ( 255 < 255 ) 
  bad = 19 ;
  if ( 20104 + hashextra > 268435455L ) 
  bad = 21 ;
  if ( ( hashoffset < 0 ) || ( hashoffset > 514 ) ) 
  bad = 42 ;
  if ( formatdefaultlength > maxint ) 
  bad = 31 ;
  if ( 2 * 268435455L < memtop - memmin ) 
  bad = 41 ;
  if ( bad > 0 ) 
  {
    fprintf( stdout , "%s%s%ld\n",  "Ouch---my internal constants have been clobbered!" ,     "---case " , (long)bad ) ;
    goto lab9999 ;
  } 
  initialize () ;
	;
#ifdef INITEX
  if ( iniversion ) 
  {
    if ( ! getstringsstarted () ) 
    goto lab9999 ;
    initprim () ;
    initstrptr = strptr ;
    initpoolptr = poolptr ;
    dateandtime ( eqtb [15183 ].cint , eqtb [15184 ].cint , eqtb [15185 ]
    .cint , eqtb [15186 ].cint ) ;
  } 
#endif /* INITEX */
  readyalready = 314159L ;
  lab1: selector = 17 ;
  tally = 0 ;
  termoffset = 0 ;
  fileoffset = 0 ;
  Fputs( stdout ,  "This is TeX, Version 3.14159" ) ;
  Fputs( stdout ,  versionstring ) ;
  if ( formatident > 0 ) 
  print ( formatident ) ;
  println () ;
  fflush ( stdout ) ;
  jobname = 0 ;
  nameinprogress = false ;
  logopened = false ;
  outputfilename = 0 ;
  {
    {
      inputptr = 0 ;
      maxinstack = 0 ;
      inopen = 0 ;
      openparens = 0 ;
      maxbufstack = 0 ;
      paramptr = 0 ;
      maxparamstack = 0 ;
      first = bufsize ;
      do {
	  buffer [first ]= 0 ;
	decr ( first ) ;
      } while ( ! ( first == 0 ) ) ;
      scannerstatus = 0 ;
      warningindex = 0 ;
      first = 1 ;
      curinput .statefield = 33 ;
      curinput .startfield = 1 ;
      curinput .indexfield = 0 ;
      line = 0 ;
      curinput .namefield = 0 ;
      forceeof = false ;
      alignstate = 1000000L ;
      if ( ! initterminal () ) 
      goto lab9999 ;
      curinput .limitfield = last ;
      first = last + 1 ;
    } 
    if ( ( formatident == 0 ) || ( buffer [curinput .locfield ]== 38 ) || 
    dumpline ) 
    {
      if ( formatident != 0 ) 
      initialize () ;
      if ( ! openfmtfile () ) 
      goto lab9999 ;
      if ( ! loadfmtfile () ) 
      {
	wclose ( fmtfile ) ;
	goto lab9999 ;
      } 
      wclose ( fmtfile ) ;
      eqtb = zeqtb ;
      while ( ( curinput .locfield < curinput .limitfield ) && ( buffer [
      curinput .locfield ]== 32 ) ) incr ( curinput .locfield ) ;
    } 
    if ( ( eqtb [15211 ].cint < 0 ) || ( eqtb [15211 ].cint > 255 ) ) 
    decr ( curinput .limitfield ) ;
    else buffer [curinput .limitfield ]= eqtb [15211 ].cint ;
    if ( mltexenabledp ) 
    {
      fprintf( stdout , "%s\n",  "MLTeX v2.2 enabled" ) ;
    } 
    dateandtime ( eqtb [15183 ].cint , eqtb [15184 ].cint , eqtb [15185 ]
    .cint , eqtb [15186 ].cint ) ;
	;
#ifdef notdef
    if ( charssavedbycharset > 0 ) 
    {
      savestrptr = strptr ;
      savepoolptr = poolptr ;
      strptr = 0 ;
      poolptr = charssavedbycharset ;
      strstart [0 ]= poolptr ;
      {register integer for_end; k = 0 ;for_end = 255 ; if ( k <= for_end) 
      do 
	{
	  {
	    strpool [poolptr ]= k ;
	    incr ( poolptr ) ;
	  } 
	  g = makestring () ;
	} 
      while ( k++ < for_end ) ;} 
      strptr = savestrptr ;
      poolptr = savepoolptr ;
    } 
#endif /* notdef */
	;
#ifdef INITEX
    if ( trienotready ) 
    {
      xmallocarray ( trietrl , triesize ) ;
      xmallocarray ( trietro , triesize ) ;
      xmallocarray ( trietrc , triesize ) ;
      xmallocarray ( triec , triesize ) ;
      xmallocarray ( trieo , triesize ) ;
      xmallocarray ( triel , triesize ) ;
      xmallocarray ( trier , triesize ) ;
      xmallocarray ( triehash , triesize ) ;
      xmallocarray ( trietaken , triesize ) ;
      triel [0 ]= 0 ;
      triec [0 ]= 0 ;
      trieptr = 0 ;
      xmallocarray ( fontcheck , fontmax ) ;
      xmallocarray ( fontsize , fontmax ) ;
      xmallocarray ( fontdsize , fontmax ) ;
      xmallocarray ( fontparams , fontmax ) ;
      xmallocarray ( fontname , fontmax ) ;
      xmallocarray ( fontarea , fontmax ) ;
      xmallocarray ( fontbc , fontmax ) ;
      xmallocarray ( fontec , fontmax ) ;
      xmallocarray ( fontglue , fontmax ) ;
      xmallocarray ( hyphenchar , fontmax ) ;
      xmallocarray ( skewchar , fontmax ) ;
      xmallocarray ( bcharlabel , fontmax ) ;
      xmallocarray ( fontbchar , fontmax ) ;
      xmallocarray ( fontfalsebchar , fontmax ) ;
      xmallocarray ( charbase , fontmax ) ;
      xmallocarray ( widthbase , fontmax ) ;
      xmallocarray ( heightbase , fontmax ) ;
      xmallocarray ( depthbase , fontmax ) ;
      xmallocarray ( italicbase , fontmax ) ;
      xmallocarray ( ligkernbase , fontmax ) ;
      xmallocarray ( kernbase , fontmax ) ;
      xmallocarray ( extenbase , fontmax ) ;
      xmallocarray ( parambase , fontmax ) ;
      fontptr = 0 ;
      fmemptr = 7 ;
      fontname [0 ]= 797 ;
      fontarea [0 ]= 335 ;
      hyphenchar [0 ]= 45 ;
      skewchar [0 ]= -1 ;
      bcharlabel [0 ]= 0 ;
      fontbchar [0 ]= 256 ;
      fontfalsebchar [0 ]= 256 ;
      fontbc [0 ]= 1 ;
      fontec [0 ]= 0 ;
      fontsize [0 ]= 0 ;
      fontdsize [0 ]= 0 ;
      charbase [0 ]= 0 ;
      widthbase [0 ]= 0 ;
      heightbase [0 ]= 0 ;
      depthbase [0 ]= 0 ;
      italicbase [0 ]= 0 ;
      ligkernbase [0 ]= 0 ;
      kernbase [0 ]= 0 ;
      extenbase [0 ]= 0 ;
      fontglue [0 ]= 0 ;
      fontparams [0 ]= 7 ;
      parambase [0 ]= -1 ;
      {register integer for_end; fontk = 0 ;for_end = 6 ; if ( fontk <= 
      for_end) do 
	fontinfo [fontk ].cint = 0 ;
      while ( fontk++ < for_end ) ;} 
    } 
#endif /* INITEX */
    xmallocarray ( fontused , fontmax ) ;
    {register integer for_end; fontk = 0 ;for_end = fontmax ; if ( fontk <= 
    for_end) do 
      fontused [fontk ]= false ;
    while ( fontk++ < for_end ) ;} 
    magicoffset = strstart [887 ]- 9 * 16 ;
    if ( interaction == 0 ) 
    selector = 16 ;
    else selector = 17 ;
    if ( ( curinput .locfield < curinput .limitfield ) && ( eqtb [13627 + 
    buffer [curinput .locfield ]].hh .v.RH != 0 ) ) 
    startinput () ;
  } 
  history = 0 ;
  maincontrol () ;
  finalcleanup () ;
  closefilesandterminate () ;
  lab9999: {
      
    fflush ( stdout ) ;
    readyalready = 0 ;
    if ( ( history != 0 ) && ( history != 1 ) ) 
    uexit ( 1 ) ;
    else uexit ( 0 ) ;
  } 
} 
