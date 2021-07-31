#define EXTERN extern
#include "texd.h"

void initialize ( ) 
{initialize_regmem 
  integer i  ; 
  integer k  ; 
  hyphpointer z  ; 
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
  interrupt = 0 ; 
  OKtointerrupt = true ; 
	;
#ifdef DEBUG
  wasmemend = memmin ; 
  waslomax = memmin ; 
  washimin = memmax ; 
  panicking = false ; 
#endif /* DEBUG */
  nestptr = 0 ; 
  maxneststack = 0 ; 
  curlist .modefield = 1 ; 
  curlist .headfield = memtop - 1 ; 
  curlist .tailfield = memtop - 1 ; 
  curlist .auxfield .cint = -65536000L ; 
  curlist .mlfield = 0 ; 
  curlist .lhmfield = 0 ; 
  curlist .rhmfield = 0 ; 
  curlist .pgfield = 0 ; 
  shownmode = 0 ; 
  pagecontents = 0 ; 
  pagetail = memtop - 2 ; 
  mem [ memtop - 2 ] .hh .v.RH = 0 ; 
  lastglue = 262143L ; 
  lastpenalty = 0 ; 
  lastkern = 0 ; 
  pagesofar [ 7 ] = 0 ; 
  pagemaxdepth = 0 ; 
  {register integer for_end; k = 12663 ; for_end = 13506 ; if ( k <= for_end) 
  do 
    xeqlevel [ k ] = 1 ; 
  while ( k++ < for_end ) ; } 
  nonewcontrolsequence = true ; 
  hash [ 514 ] .v.LH = 0 ; 
  hash [ 514 ] .v.RH = 0 ; 
  {register integer for_end; k = 515 ; for_end = 10280 ; if ( k <= for_end) 
  do 
    hash [ k ] = hash [ 514 ] ; 
  while ( k++ < for_end ) ; } 
  saveptr = 0 ; 
  curlevel = 1 ; 
  curgroup = 0 ; 
  curboundary = 0 ; 
  maxsavestack = 0 ; 
  magset = 0 ; 
  curmark [ 0 ] = 0 ; 
  curmark [ 1 ] = 0 ; 
  curmark [ 2 ] = 0 ; 
  curmark [ 3 ] = 0 ; 
  curmark [ 4 ] = 0 ; 
  curval = 0 ; 
  curvallevel = 0 ; 
  radix = 0 ; 
  curorder = 0 ; 
  {register integer for_end; k = 0 ; for_end = 16 ; if ( k <= for_end) do 
    readopen [ k ] = 2 ; 
  while ( k++ < for_end ) ; } 
  condptr = 0 ; 
  iflimit = 0 ; 
  curif = 0 ; 
  ifline = 0 ; 
  TEXformatdefault = " plain.fmt" ; 
  {register integer for_end; k = 0 ; for_end = fontmax ; if ( k <= for_end) 
  do 
    fontused [ k ] = false ; 
  while ( k++ < for_end ) ; } 
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
  {register integer for_end; z = 0 ; for_end = 607 ; if ( z <= for_end) do 
    {
      hyphword [ z ] = 0 ; 
      hyphlist [ z ] = 0 ; 
    } 
  while ( z++ < for_end ) ; } 
  hyphcount = 0 ; 
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
  {register integer for_end; k = 0 ; for_end = 17 ; if ( k <= for_end) do 
    writeopen [ k ] = false ; 
  while ( k++ < for_end ) ; } 
  editnamestart = 0 ; 
	;
#ifdef INITEX
  {register integer for_end; k = 1 ; for_end = 19 ; if ( k <= for_end) do 
    mem [ k ] .cint = 0 ; 
  while ( k++ < for_end ) ; } 
  k = 0 ; 
  while ( k <= 19 ) {
      
    mem [ k ] .hh .v.RH = 1 ; 
    mem [ k ] .hh.b0 = 0 ; 
    mem [ k ] .hh.b1 = 0 ; 
    k = k + 4 ; 
  } 
  mem [ 6 ] .cint = 65536L ; 
  mem [ 4 ] .hh.b0 = 1 ; 
  mem [ 10 ] .cint = 65536L ; 
  mem [ 8 ] .hh.b0 = 2 ; 
  mem [ 14 ] .cint = 65536L ; 
  mem [ 12 ] .hh.b0 = 1 ; 
  mem [ 15 ] .cint = 65536L ; 
  mem [ 12 ] .hh.b1 = 1 ; 
  mem [ 18 ] .cint = -65536L ; 
  mem [ 16 ] .hh.b0 = 1 ; 
  rover = 20 ; 
  mem [ rover ] .hh .v.RH = 262143L ; 
  mem [ rover ] .hh .v.LH = 1000 ; 
  mem [ rover + 1 ] .hh .v.LH = rover ; 
  mem [ rover + 1 ] .hh .v.RH = rover ; 
  lomemmax = rover + 1000 ; 
  mem [ lomemmax ] .hh .v.RH = 0 ; 
  mem [ lomemmax ] .hh .v.LH = 0 ; 
  {register integer for_end; k = memtop - 13 ; for_end = memtop ; if ( k <= 
  for_end) do 
    mem [ k ] = mem [ lomemmax ] ; 
  while ( k++ < for_end ) ; } 
  mem [ memtop - 10 ] .hh .v.LH = 14114 ; 
  mem [ memtop - 9 ] .hh .v.RH = 512 ; 
  mem [ memtop - 9 ] .hh .v.LH = 0 ; 
  mem [ memtop - 7 ] .hh.b0 = 1 ; 
  mem [ memtop - 6 ] .hh .v.LH = 262143L ; 
  mem [ memtop - 7 ] .hh.b1 = 0 ; 
  mem [ memtop ] .hh.b1 = 255 ; 
  mem [ memtop ] .hh.b0 = 1 ; 
  mem [ memtop ] .hh .v.RH = memtop ; 
  mem [ memtop - 2 ] .hh.b0 = 10 ; 
  mem [ memtop - 2 ] .hh.b1 = 0 ; 
  avail = 0 ; 
  memend = memtop ; 
  himemmin = memtop - 13 ; 
  varused = 20 ; 
  dynused = 14 ; 
  eqtb [ 10281 ] .hh.b0 = 101 ; 
  eqtb [ 10281 ] .hh .v.RH = 0 ; 
  eqtb [ 10281 ] .hh.b1 = 0 ; 
  {register integer for_end; k = 1 ; for_end = 10280 ; if ( k <= for_end) do 
    eqtb [ k ] = eqtb [ 10281 ] ; 
  while ( k++ < for_end ) ; } 
  eqtb [ 10282 ] .hh .v.RH = 0 ; 
  eqtb [ 10282 ] .hh.b1 = 1 ; 
  eqtb [ 10282 ] .hh.b0 = 117 ; 
  {register integer for_end; k = 10283 ; for_end = 10811 ; if ( k <= for_end) 
  do 
    eqtb [ k ] = eqtb [ 10282 ] ; 
  while ( k++ < for_end ) ; } 
  mem [ 0 ] .hh .v.RH = mem [ 0 ] .hh .v.RH + 530 ; 
  eqtb [ 10812 ] .hh .v.RH = 0 ; 
  eqtb [ 10812 ] .hh.b0 = 118 ; 
  eqtb [ 10812 ] .hh.b1 = 1 ; 
  {register integer for_end; k = 10813 ; for_end = 11077 ; if ( k <= for_end) 
  do 
    eqtb [ k ] = eqtb [ 10281 ] ; 
  while ( k++ < for_end ) ; } 
  eqtb [ 11078 ] .hh .v.RH = 0 ; 
  eqtb [ 11078 ] .hh.b0 = 119 ; 
  eqtb [ 11078 ] .hh.b1 = 1 ; 
  {register integer for_end; k = 11079 ; for_end = 11333 ; if ( k <= for_end) 
  do 
    eqtb [ k ] = eqtb [ 11078 ] ; 
  while ( k++ < for_end ) ; } 
  eqtb [ 11334 ] .hh .v.RH = 0 ; 
  eqtb [ 11334 ] .hh.b0 = 120 ; 
  eqtb [ 11334 ] .hh.b1 = 1 ; 
  {register integer for_end; k = 11335 ; for_end = 11382 ; if ( k <= for_end) 
  do 
    eqtb [ k ] = eqtb [ 11334 ] ; 
  while ( k++ < for_end ) ; } 
  eqtb [ 11383 ] .hh .v.RH = 0 ; 
  eqtb [ 11383 ] .hh.b0 = 120 ; 
  eqtb [ 11383 ] .hh.b1 = 1 ; 
  {register integer for_end; k = 11384 ; for_end = 12662 ; if ( k <= for_end) 
  do 
    eqtb [ k ] = eqtb [ 11383 ] ; 
  while ( k++ < for_end ) ; } 
  {register integer for_end; k = 0 ; for_end = 255 ; if ( k <= for_end) do 
    {
      eqtb [ 11383 + k ] .hh .v.RH = 12 ; 
      eqtb [ 12407 + k ] .hh .v.RH = k ; 
      eqtb [ 12151 + k ] .hh .v.RH = 1000 ; 
    } 
  while ( k++ < for_end ) ; } 
  eqtb [ 11396 ] .hh .v.RH = 5 ; 
  eqtb [ 11415 ] .hh .v.RH = 10 ; 
  eqtb [ 11475 ] .hh .v.RH = 0 ; 
  eqtb [ 11420 ] .hh .v.RH = 14 ; 
  eqtb [ 11510 ] .hh .v.RH = 15 ; 
  eqtb [ 11383 ] .hh .v.RH = 9 ; 
  {register integer for_end; k = 48 ; for_end = 57 ; if ( k <= for_end) do 
    eqtb [ 12407 + k ] .hh .v.RH = k + 28672 ; 
  while ( k++ < for_end ) ; } 
  {register integer for_end; k = 65 ; for_end = 90 ; if ( k <= for_end) do 
    {
      eqtb [ 11383 + k ] .hh .v.RH = 11 ; 
      eqtb [ 11383 + k + 32 ] .hh .v.RH = 11 ; 
      eqtb [ 12407 + k ] .hh .v.RH = k + 28928 ; 
      eqtb [ 12407 + k + 32 ] .hh .v.RH = k + 28960 ; 
      eqtb [ 11639 + k ] .hh .v.RH = k + 32 ; 
      eqtb [ 11639 + k + 32 ] .hh .v.RH = k + 32 ; 
      eqtb [ 11895 + k ] .hh .v.RH = k ; 
      eqtb [ 11895 + k + 32 ] .hh .v.RH = k ; 
      eqtb [ 12151 + k ] .hh .v.RH = 999 ; 
    } 
  while ( k++ < for_end ) ; } 
  {register integer for_end; k = 12663 ; for_end = 12973 ; if ( k <= for_end) 
  do 
    eqtb [ k ] .cint = 0 ; 
  while ( k++ < for_end ) ; } 
  eqtb [ 12680 ] .cint = 1000 ; 
  eqtb [ 12664 ] .cint = 10000 ; 
  eqtb [ 12704 ] .cint = 1 ; 
  eqtb [ 12703 ] .cint = 25 ; 
  eqtb [ 12708 ] .cint = 92 ; 
  eqtb [ 12711 ] .cint = 13 ; 
  {register integer for_end; k = 0 ; for_end = 255 ; if ( k <= for_end) do 
    eqtb [ 12974 + k ] .cint = -1 ; 
  while ( k++ < for_end ) ; } 
  eqtb [ 13020 ] .cint = 0 ; 
  {register integer for_end; k = 13230 ; for_end = 13506 ; if ( k <= for_end) 
  do 
    eqtb [ k ] .cint = 0 ; 
  while ( k++ < for_end ) ; } 
  hashused = 10014 ; 
  cscount = 0 ; 
  eqtb [ 10023 ] .hh.b0 = 116 ; 
  hash [ 10023 ] .v.RH = 498 ; 
  fontptr = 0 ; 
  fmemptr = 7 ; 
  fontname [ 0 ] = 794 ; 
  fontarea [ 0 ] = 335 ; 
  hyphenchar [ 0 ] = 45 ; 
  skewchar [ 0 ] = -1 ; 
  fontbc [ 0 ] = 1 ; 
  fontec [ 0 ] = 0 ; 
  fontsize [ 0 ] = 0 ; 
  fontdsize [ 0 ] = 0 ; 
  charbase [ 0 ] = 0 ; 
  widthbase [ 0 ] = 0 ; 
  heightbase [ 0 ] = 0 ; 
  depthbase [ 0 ] = 0 ; 
  italicbase [ 0 ] = 0 ; 
  ligkernbase [ 0 ] = 0 ; 
  kernbase [ 0 ] = 0 ; 
  extenbase [ 0 ] = 0 ; 
  fontglue [ 0 ] = 0 ; 
  fontparams [ 0 ] = 7 ; 
  parambase [ 0 ] = -1 ; 
  {register integer for_end; k = 0 ; for_end = 6 ; if ( k <= for_end) do 
    fontinfo [ k ] .cint = 0 ; 
  while ( k++ < for_end ) ; } 
  {register integer for_end; k = - (integer) trieopsize ; for_end = 
  trieopsize ; if ( k <= for_end) do 
    trieophash [ k ] = 0 ; 
  while ( k++ < for_end ) ; } 
  {register integer for_end; k = 0 ; for_end = 255 ; if ( k <= for_end) do 
    trieused [ k ] = 0 ; 
  while ( k++ < for_end ) ; } 
  trieopptr = 0 ; 
  trienotready = true ; 
  triel [ 0 ] = 0 ; 
  triec [ 0 ] = 0 ; 
  trieptr = 0 ; 
  hash [ 10014 ] .v.RH = 1183 ; 
  formatident = 1248 ; 
  hash [ 10022 ] .v.RH = 1286 ; 
  eqtb [ 10022 ] .hh.b1 = 1 ; 
  eqtb [ 10022 ] .hh.b0 = 113 ; 
  eqtb [ 10022 ] .hh .v.RH = 0 ; 
#endif /* INITEX */
} 
#ifdef INITEX
boolean getstringsstarted ( ) 
{/* 30 10 */ register boolean Result; getstringsstarted_regmem 
  unsigned char k, l  ; 
  ASCIIcode m, n  ; 
  strnumber g  ; 
  integer a  ; 
  boolean c  ; 
  poolptr = 0 ; 
  strptr = 0 ; 
  strstart [ 0 ] = 0 ; 
  {register integer for_end; k = 0 ; for_end = 255 ; if ( k <= for_end) do 
    {
      if ( ( ( k < 32 ) || ( k > 126 ) ) ) 
      {
	{
	  strpool [ poolptr ] = 94 ; 
	  incr ( poolptr ) ; 
	} 
	{
	  strpool [ poolptr ] = 94 ; 
	  incr ( poolptr ) ; 
	} 
	if ( k < 64 ) 
	{
	  strpool [ poolptr ] = k + 64 ; 
	  incr ( poolptr ) ; 
	} 
	else if ( k < 128 ) 
	{
	  strpool [ poolptr ] = k - 64 ; 
	  incr ( poolptr ) ; 
	} 
	else {
	    
	  l = k / 16 ; 
	  if ( l < 10 ) 
	  {
	    strpool [ poolptr ] = l + 48 ; 
	    incr ( poolptr ) ; 
	  } 
	  else {
	      
	    strpool [ poolptr ] = l + 87 ; 
	    incr ( poolptr ) ; 
	  } 
	  l = k % 16 ; 
	  if ( l < 10 ) 
	  {
	    strpool [ poolptr ] = l + 48 ; 
	    incr ( poolptr ) ; 
	  } 
	  else {
	      
	    strpool [ poolptr ] = l + 87 ; 
	    incr ( poolptr ) ; 
	  } 
	} 
      } 
      else {
	  
	strpool [ poolptr ] = k ; 
	incr ( poolptr ) ; 
      } 
      g = makestring () ; 
    } 
  while ( k++ < for_end ) ; } 
  vstrcpy ( nameoffile + 1 , poolname ) ; 
  if ( aopenin ( poolfile , poolpathspec ) ) 
  {
    c = false ; 
    do {
	{ 
	if ( eof ( poolfile ) ) 
	{
	  wakeupterminal () ; 
	  (void) fprintf( stdout , "%s\n",  "! tex.pool has no check sum." ) ; 
	  aclose ( poolfile ) ; 
	  Result = false ; 
	  return(Result) ; 
	} 
	read ( poolfile , m ) ; 
	read ( poolfile , n ) ; 
	if ( m == '*' ) 
	{
	  a = 0 ; 
	  k = 1 ; 
	  while ( true ) {
	      
	    if ( ( xord [ n ] < 48 ) || ( xord [ n ] > 57 ) ) 
	    {
	      wakeupterminal () ; 
	      (void) fprintf( stdout , "%s\n",                "! tex.pool check sum doesn't have nine digits." ) ; 
	      aclose ( poolfile ) ; 
	      Result = false ; 
	      return(Result) ; 
	    } 
	    a = 10 * a + xord [ n ] - 48 ; 
	    if ( k == 9 ) 
	    goto lab30 ; 
	    incr ( k ) ; 
	    read ( poolfile , n ) ; 
	  } 
	  lab30: if ( a != 127541235L ) 
	  {
	    wakeupterminal () ; 
	    (void) fprintf( stdout , "%s\n",  "! tex.pool doesn't match; tangle me again." ) 
	    ; 
	    aclose ( poolfile ) ; 
	    Result = false ; 
	    return(Result) ; 
	  } 
	  c = true ; 
	} 
	else {
	    
	  if ( ( xord [ m ] < 48 ) || ( xord [ m ] > 57 ) || ( xord [ n ] < 48 
	  ) || ( xord [ n ] > 57 ) ) 
	  {
	    wakeupterminal () ; 
	    (void) fprintf( stdout , "%s\n",              "! tex.pool line doesn't begin with two digits." ) ; 
	    aclose ( poolfile ) ; 
	    Result = false ; 
	    return(Result) ; 
	  } 
	  l = xord [ m ] * 10 + xord [ n ] - 48 * 11 ; 
	  if ( poolptr + l + stringvacancies > poolsize ) 
	  {
	    wakeupterminal () ; 
	    (void) fprintf( stdout , "%s\n",  "! You have to increase POOLSIZE." ) ; 
	    aclose ( poolfile ) ; 
	    Result = false ; 
	    return(Result) ; 
	  } 
	  {register integer for_end; k = 1 ; for_end = l ; if ( k <= for_end) 
	  do 
	    {
	      if ( eoln ( poolfile ) ) 
	      m = ' ' ; 
	      else read ( poolfile , m ) ; 
	      {
		strpool [ poolptr ] = xord [ m ] ; 
		incr ( poolptr ) ; 
	      } 
	    } 
	  while ( k++ < for_end ) ; } 
	  readln ( poolfile ) ; 
	  g = makestring () ; 
	} 
      } 
    } while ( ! ( c ) ) ; 
    aclose ( poolfile ) ; 
    Result = true ; 
  } 
  else {
      
    wakeupterminal () ; 
    (void) fprintf( stdout , "%s\n",  "! I can't read tex.pool." ) ; 
    Result = false ; 
    return(Result) ; 
  } 
  return(Result) ; 
} 
#endif /* INITEX */
#ifdef INITEX
void sortavail ( ) 
{sortavail_regmem 
  halfword p, q, r  ; 
  halfword oldrover  ; 
  p = getnode ( 1073741824L ) ; 
  p = mem [ rover + 1 ] .hh .v.RH ; 
  mem [ rover + 1 ] .hh .v.RH = 262143L ; 
  oldrover = rover ; 
  while ( p != oldrover ) if ( p < rover ) 
  {
    q = p ; 
    p = mem [ q + 1