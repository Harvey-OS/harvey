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
  setboxallowed = true ; 
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
  mem [ memtop - 9 ] .hh .v.RH = 256 ; 
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
  bcharlabel [ 0 ] = fontmemsize ; 
  fontbchar [ 0 ] = 256 ; 
  fontfalsebchar [ 0 ] = 256 ; 
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
    trieused [ k ] = mintrieop ; 
  while ( k++ < for_end ) ; } 
  maxopused = mintrieop ; 
  trieopptr = 0 ; 
  trienotready = true ; 
  triel [ 0 ] = 0 ; 
  triec [ 0 ] = 0 ; 
  trieptr = 0 ; 
  hash [ 10014 ] .v.RH = 1183 ; 
  formatident = 1250 ; 
  hash [ 10022 ] .v.RH = 1288 ; 
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
  nameoffile [ 0 ] = ' ' ; 
  nameoffile [ strlen ( poolname ) + 1 ] = ' ' ; 
  namelength = strlen ( poolname ) ; 
  if ( aopenin ( poolfile , TEXPOOLPATH ) ) 
  {
    c = false ; 
    do {
	{ 
	if ( eof ( poolfile ) ) 
	{
	  ; 
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
	      ; 
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
	  lab30: if ( a != 353758006L ) 
	  {
	    ; 
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
	    ; 
	    (void) fprintf( stdout , "%s\n",              "! tex.pool line doesn't begin with two digits." ) ; 
	    aclose ( poolfile ) ; 
	    Result = false ; 
	    return(Result) ; 
	  } 
	  l = xord [ m ] * 10 + xord [ n ] - 48 * 11 ; 
	  if ( poolptr + l + stringvacancies > poolsize ) 
	  {
	    ; 
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
      
    ; 
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
    p = mem [ q + 1 ] .hh .v.RH ; 
    mem [ q + 1 ] .hh .v.RH = rover ; 
    rover = q ; 
  } 
  else {
      
    q = rover ; 
    while ( mem [ q + 1 ] .hh .v.RH < p ) q = mem [ q + 1 ] .hh .v.RH ; 
    r = mem [ p + 1 ] .hh .v.RH ; 
    mem [ p + 1 ] .hh .v.RH = mem [ q + 1 ] .hh .v.RH ; 
    mem [ q + 1 ] .hh .v.RH = p ; 
    p = r ; 
  } 
  p = rover ; 
  while ( mem [ p + 1 ] .hh .v.RH != 262143L ) {
      
    mem [ mem [ p + 1 ] .hh .v.RH + 1 ] .hh .v.LH = p ; 
    p = mem [ p + 1 ] .hh .v.RH ; 
  } 
  mem [ p + 1 ] .hh .v.RH = rover ; 
  mem [ rover + 1 ] .hh .v.LH = p ; 
} 
#endif /* INITEX */
#ifdef INITEX
void zprimitive ( s , c , o ) 
strnumber s ; 
quarterword c ; 
halfword o ; 
{primitive_regmem 
  poolpointer k  ; 
  smallnumber j  ; 
  smallnumber l  ; 
  if ( s < 256 ) 
  curval = s + 257 ; 
  else {
      
    k = strstart [ s ] ; 
    l = strstart [ s + 1 ] - k ; 
    {register integer for_end; j = 0 ; for_end = l - 1 ; if ( j <= for_end) 
    do 
      buffer [ j ] = strpool [ k + j ] ; 
    while ( j++ < for_end ) ; } 
    curval = idlookup ( 0 , l ) ; 
    {
      decr ( strptr ) ; 
      poolptr = strstart [ strptr ] ; 
    } 
    hash [ curval ] .v.RH = s ; 
  } 
  eqtb [ curval ] .hh.b1 = 1 ; 
  eqtb [ curval ] .hh.b0 = c ; 
  eqtb [ curval ] .hh .v.RH = o ; 
} 
#endif /* INITEX */
#ifdef INITEX
trieopcode znewtrieop ( d , n , v ) 
smallnumber d ; 
smallnumber n ; 
trieopcode v ; 
{/* 10 */ register trieopcode Result; newtrieop_regmem 
  integer h  ; 
  trieopcode u  ; 
  integer l  ; 
  h = abs ( toint ( n ) + 313 * toint ( d ) + 361 * toint ( v ) + 1009 * toint 
  ( curlang ) ) % ( trieopsize + trieopsize ) - trieopsize ; 
  while ( true ) {
      
    l = trieophash [ h ] ; 
    if ( l == 0 ) 
    {
      if ( trieopptr == trieopsize ) 
      overflow ( 942 , trieopsize ) ; 
      u = trieused [ curlang ] ; 
      if ( u == maxtrieop ) 
      overflow ( 943 , maxtrieop - mintrieop ) ; 
      incr ( trieopptr ) ; 
      incr ( u ) ; 
      trieused [ curlang ] = u ; 
      if ( u > maxopused ) 
      maxopused = u ; 
      hyfdistance [ trieopptr ] = d ; 
      hyfnum [ trieopptr ] = n ; 
      hyfnext [ trieopptr ] = v ; 
      trieoplang [ trieopptr ] = curlang ; 
      trieophash [ h ] = trieopptr ; 
      trieopval [ trieopptr ] = u ; 
      Result = u ; 
      return(Result) ; 
    } 
    if ( ( hyfdistance [ l ] == d ) && ( hyfnum [ l ] == n ) && ( hyfnext [ l 
    ] == v ) && ( trieoplang [ l ] == curlang ) ) 
    {
      Result = trieopval [ l ] ; 
      return(Result) ; 
    } 
    if ( h > - (integer) trieopsize ) 
    decr ( h ) ; 
    else h = trieopsize ; 
  } 
  return(Result) ; 
} 
triepointer ztrienode ( p ) 
triepointer p ; 
{/* 10 */ register triepointer Result; trienode_regmem 
  triepointer h  ; 
  triepointer q  ; 
  h = abs ( toint ( triec [ p ] ) + 1009 * toint ( trieo [ p ] ) + 2718 * 
  toint ( triel [ p ] ) + 3142 * toint ( trier [ p ] ) ) % triesize ; 
  while ( true ) {
      
    q = triehash [ h ] ; 
    if ( q == 0 ) 
    {
      triehash [ h ] = p ; 
      Result = p ; 
      return(Result) ; 
    } 
    if ( ( triec [ q ] == triec [ p ] ) && ( trieo [ q ] == trieo [ p ] ) && ( 
    triel [ q ] == triel [ p ] ) && ( trier [ q ] == trier [ p ] ) ) 
    {
      Result = q ; 
      return(Result) ; 
    } 
    if ( h > 0 ) 
    decr ( h ) ; 
    else h = triesize ; 
  } 
  return(Result) ; 
} 
triepointer zcompresstrie ( p ) 
triepointer p ; 
{register triepointer Result; compresstrie_regmem 
  if ( p == 0 ) 
  Result = 0 ; 
  else {
      
    triel [ p ] = compresstrie ( triel [ p ] ) ; 
    trier [ p ] = compresstrie ( trier [ p ] ) ; 
    Result = trienode ( p ) ; 
  } 
  return(Result) ; 
} 
void zfirstfit ( p ) 
triepointer p ; 
{/* 45 40 */ firstfit_regmem 
  triepointer h  ; 
  triepointer z  ; 
  triepointer q  ; 
  ASCIIcode c  ; 
  triepointer l, r  ; 
  short ll  ; 
  c = triec [ p ] ; 
  z = triemin [ c ] ; 
  while ( true ) {
      
    h = z - c ; 
    if ( triemax < h + 256 ) 
    {
      if ( triesize <= h + 256 ) 
      overflow ( 944 , triesize ) ; 
      do {
	  incr ( triemax ) ; 
	trietaken [ triemax ] = false ; 
	trietrl [ triemax ] = triemax + 1 ; 
	trietro [ triemax ] = triemax - 1 ; 
      } while ( ! ( triemax == h + 256 ) ) ; 
    } 
    if ( trietaken [ h ] ) 
    goto lab45 ; 
    q = trier [ p ] ; 
    while ( q > 0 ) {
	
      if ( trietrl [ h + triec [ q ] ] == 0 ) 
      goto lab45 ; 
      q = trier [ q ] ; 
    } 
    goto lab40 ; 
    lab45: z = trietrl [ z ] ; 
  } 
  lab40: trietaken [ h ] = true ; 
  triehash [ p ] = h ; 
  q = p ; 
  do {
      z = h + triec [ q ] ; 
    l = trietro [ z ] ; 
    r = trietrl [ z ] ; 
    trietro [ r ] = l ; 
    trietrl [ l ] = r ; 
    trietrl [ z ] = 0 ; 
    if ( l < 256 ) 
    {
      if ( z < 256 ) 
      ll = z ; 
      else ll = 256 ; 
      do {
	  triemin [ l ] = r ; 
	incr ( l ) ; 
      } while ( ! ( l == ll ) ) ; 
    } 
    q = trier [ q ] ; 
  } while ( ! ( q == 0 ) ) ; 
} 
void ztriepack ( p ) 
triepointer p ; 
{triepack_regmem 
  triepointer q  ; 
  do {
      q = triel [ p ] ; 
    if ( ( q > 0 ) && ( triehash [ q ] == 0 ) ) 
    {
      firstfit ( q ) ; 
      triepack ( q ) ; 
    } 
    p = trier [ p ] ; 
  } while ( ! ( p == 0 ) ) ; 
} 
void ztriefix ( p ) 
triepointer p ; 
{triefix_regmem 
  triepointer q  ; 
  ASCIIcode c  ; 
  triepointer z  ; 
  z = triehash [ p ] ; 
  do {
      q = triel [ p ] ; 
    c = triec [ p ] ; 
    trietrl [ z + c ] = triehash [ q ] ; 
    trietrc [ z + c ] = c ; 
    trietro [ z + c ] = trieo [ p ] ; 
    if ( q > 0 ) 
    triefix ( q ) ; 
    p = trier [ p ] ; 
  } while ( ! ( p == 0 ) ) ; 
} 
void newpatterns ( ) 
{/* 30 31 */ newpatterns_regmem 
  smallnumber k, l  ; 
  boolean digitsensed  ; 
  trieopcode v  ; 
  triepointer p, q  ; 
  boolean firstchild  ; 
  ASCIIcode c  ; 
  if ( trienotready ) 
  {
    if ( eqtb [ 12713 ] .cint <= 0 ) 
    curlang = 0 ; 
    else if ( eqtb [ 12713 ] .cint > 255 ) 
    curlang = 0 ; 
    else curlang = eqtb [ 12713 ] .cint ; 
    scanleftbrace () ; 
    k = 0 ; 
    hyf [ 0 ] = 0 ; 
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
	      
	    curchr = eqtb [ 11639 + curchr ] .hh .v.RH ; 
	    if ( curchr == 0 ) 
	    {
	      {
		if ( interaction == 3 ) 
		; 
		printnl ( 262 ) ; 
		print ( 950 ) ; 
	      } 
	      {
		helpptr = 1 ; 
		helpline [ 0 ] = 949 ; 
	      } 
	      error () ; 
	    } 
	  } 
	  if ( k < 63 ) 
	  {
	    incr ( k ) ; 
	    hc [ k ] = curchr ; 
	    hyf [ k ] = 0 ; 
	    digitsensed = false ; 
	  } 
	} 
	else if ( k < 63 ) 
	{
	  hyf [ k ] = curchr - 48 ; 
	  digitsensed = true ; 
	} 
	break ; 
      case 10 : 
      case 2 : 
	{
	  if ( k > 0 ) 
	  {
	    if ( hc [ 1 ] == 0 ) 
	    hyf [ 0 ] = 0 ; 
	    if ( hc [ k ] == 0 ) 
	    hyf [ k ] = 0 ; 
	    l = k ; 
	    v = mintrieop ; 
	    while ( true ) {
		
	      if ( hyf [ l ] != 0 ) 
	      v = newtrieop ( k - l , hyf [ l ] , v ) ; 
	      if ( l > 0 ) 
	      decr ( l ) ; 
	      else goto lab31 ; 
	    } 
	    lab31: ; 
	    q = 0 ; 
	    hc [ 0 ] = curlang ; 
	    while ( l <= k ) {
		
	      c = hc [ l ] ; 
	      incr ( l ) ; 
	      p = triel [ q ] ; 
	      firstchild = true ; 
	      while ( ( p > 0 ) && ( c > triec [ p ] ) ) {
		  
		q = p ; 
		p = trier [ q ] ; 
		firstchild = false ; 
	      } 
	      if ( ( p == 0 ) || ( c < triec [ p ] ) ) 
	      {
		if ( trieptr == triesize ) 
		overflow ( 944 , triesize ) ; 
		incr ( trieptr ) ; 
		trier [ trieptr ] = p ; 
		p = trieptr ; 
		triel [ p ] = 0 ; 
		if ( firstchild ) 
		triel [ q ] = p ; 
		else trier [ q ] = p ; 
		triec [ p ] = c ; 
		trieo [ p ] = mintrieop ; 
	      } 
	      q = p ; 
	    } 
	    if ( trieo [ q ] != mintrieop ) 
	    {
	      {
		if ( interaction == 3 ) 
		; 
		printnl ( 262 ) ; 
		print ( 951 ) ; 
	      } 
	      {
		helpptr = 1 ; 
		helpline [ 0 ] = 949 ; 
	      } 
	      error () ; 
	    } 
	    trieo [ q ] = v ; 
	  } 
	  if ( curcmd == 2 ) 
	  goto lab30 ; 
	  k = 0 ; 
	  hyf [ 0 ] = 0 ; 
	  digitsensed = false ; 
	} 
	break ; 
	default: 
	{
	  {
	    if ( interaction == 3 ) 
	    ; 
	    printnl ( 262 ) ; 
	    print ( 948 ) ; 
	  } 
	  printesc ( 946 ) ; 
	  {
	    helpptr = 1 ; 
	    helpline [ 0 ] = 949 ; 
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
      print ( 945 ) ; 
    } 
    printesc ( 946 ) ; 
    {
      helpptr = 1 ; 
      helpline [ 0 ] = 947 ; 
    } 
    error () ; 
    mem [ memtop - 12 ] .hh .v.RH = scantoks ( false , false ) ; 
    flushlist ( defref ) ; 
  } 
} 
void inittrie ( ) 
{inittrie_regmem 
  triepointer p  ; 
  integer j, k, t  ; 
  triepointer r, s  ; 
  opstart [ 0 ] = - (integer) mintrieop ; 
  {register integer for_end; j = 1 ; for_end = 255 ; if ( j <= for_end) do 
    opstart [ j ] = opstart [ j - 1 ] + trieused [ j - 1 ] ; 
  while ( j++ < for_end ) ; } 
  {register integer for_end; j = 1 ; for_end = trieopptr ; if ( j <= for_end) 
  do 
    trieophash [ j ] = opstart [ trieoplang [ j ] ] + trieopval [ j ] ; 
  while ( j++ < for_end ) ; } 
  {register integer for_end; j = 1 ; for_end = trieopptr ; if ( j <= for_end) 
  do 
    while ( trieophash [ j ] > j ) {
	
      k = trieophash [ j ] ; 
      t = hyfdistance [ k ] ; 
      hyfdistance [ k ] = hyfdistance [ j ] ; 
      hyfdistance [ j ] = t ; 
      t = hyfnum [ k ] ; 
      hyfnum [ k ] = hyfnum [ j ] ; 
      hyfnum [ j ] = t ; 
      t = hyfnext [ k ] ; 
      hyfnext [ k ] = hyfnext [ j ] ; 
      hyfnext [ j ] = t ; 
      trieophash [ j ] = trieophash [ k ] ; 
      trieophash [ k ] = k ; 
    } 
  while ( j++ < for_end ) ; } 
  {register integer for_end; p = 0 ; for_end = triesize ; if ( p <= for_end) 
  do 
    triehash [ p ] = 0 ; 
  while ( p++ < for_end ) ; } 
  triel [ 0 ] = compresstrie ( triel [ 0 ] ) ; 
  {register integer for_end; p = 0 ; for_end = trieptr ; if ( p <= for_end) 
  do 
    triehash [ p ] = 0 ; 
  while ( p++ < for_end ) ; } 
  {register integer for_end; p = 0 ; for_end = 255 ; if ( p <= for_end) do 
    triemin [ p ] = p + 1 ; 
  while ( p++ < for_end ) ; } 
  trietrl [ 0 ] = 1 ; 
  triemax = 0 ; 
  if ( triel [ 0 ] != 0 ) 
  {
    firstfit ( triel [ 0 ] ) ; 
    triepack ( triel [ 0 ] ) ; 
  } 
  if ( triel [ 0 ] == 0 ) 
  {
    {register integer for_end; r = 0 ; for_end = 256 ; if ( r <= for_end) do 
      {
	trietrl [ r ] = 0 ; 
	trietro [ r ] = mintrieop ; 
	trietrc [ r ] = 0 ; 
      } 
    while ( r++ < for_end ) ; } 
    triemax = 256 ; 
  } 
  else {
      
    triefix ( triel [ 0 ] ) ; 
    r = 0 ; 
    do {
	s = trietrl [ r ] ; 
      {
	trietrl [ r ] = 0 ; 
	trietro [ r ] = mintrieop ; 
	trietrc [ r ] = 0 ; 
      } 
      r = s ; 
    } while ( ! ( r > triemax ) ) ; 
  } 
  trietrc [ 0 ] = 63 ; 
  trienotready = false ; 
} 
#endif /* INITEX */
void zlinebreak ( finalwidowpenalty ) 
integer finalwidowpenalty ; 
{/* 30 31 32 33 34 35 22 */ linebreak_regmem 
  boolean autobreaking  ; 
  halfword prevp  ; 
  halfword q, r, s, prevs  ; 
  internalfontnumber f  ; 
  smallnumber j  ; 
  unsigned char c  ; 
  packbeginline = curlist .mlfield ; 
  mem [ memtop - 3 ] .hh .v.RH = mem [ curlist .headfield ] .hh .v.RH ; 
  if ( ( curlist .tailfield >= himemmin ) ) 
  {
    mem [ curlist .tailfield ] .hh .v.RH = newpenalty ( 10000 ) ; 
    curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
  } 
  else if ( mem [ curlist .tailfield ] .hh.b0 != 10 ) 
  {
    mem [ curlist .tailfield ] .hh .v.RH = newpenalty ( 10000 ) ; 
    curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
  } 
  else {
      
    mem [ curlist .tailfield ] .hh.b0 = 12 ; 
    deleteglueref ( mem [ curlist .tailfield + 1 ] .hh .v.LH ) ; 
    flushnodelist ( mem [ curlist .tailfield + 1 ] .hh .v.RH ) ; 
    mem [ curlist .tailfield + 1 ] .cint = 10000 ; 
  } 
  mem [ curlist .tailfield ] .hh .v.RH = newparamglue ( 14 ) ; 
  popnest () ; 
  noshrinkerroryet = true ; 
  if ( ( mem [ eqtb [ 10289 ] .hh .v.RH ] .hh.b1 != 0 ) && ( mem [ eqtb [ 
  10289 ] .hh .v.RH + 3 ] .cint != 0 ) ) 
  {
    eqtb [ 10289 ] .hh .v.RH = finiteshrink ( eqtb [ 10289 ] .hh .v.RH ) ; 
  } 
  if ( ( mem [ eqtb [ 10290 ] .hh .v.RH ] .hh.b1 != 0 ) && ( mem [ eqtb [ 
  10290 ] .hh .v.RH + 3 ] .cint != 0 ) ) 
  {
    eqtb [ 10290 ] .hh .v.RH = finiteshrink ( eqtb [ 10290 ] .hh .v.RH ) ; 
  } 
  q = eqtb [ 10289 ] .hh .v.RH ; 
  r = eqtb [ 10290 ] .hh .v.RH ; 
  background [ 1 ] = mem [ q + 1 ] .cint + mem [ r + 1 ] .cint ; 
  background [ 2 ] = 0 ; 
  background [ 3 ] = 0 ; 
  background [ 4 ] = 0 ; 
  background [ 5 ] = 0 ; 
  background [ 2 + mem [ q ] .hh.b0 ] = mem [ q + 2 ] .cint ; 
  background [ 2 + mem [ r ] .hh.b0 ] = background [ 2 + mem [ r ] .hh.b0 ] + 
  mem [ r + 2 ] .cint ; 
  background [ 6 ] = mem [ q + 3 ] .cint + mem [ r + 3 ] .cint ; 
  minimumdemerits = 1073741823L ; 
  minimaldemerits [ 3 ] = 1073741823L ; 
  minimaldemerits [ 2 ] = 1073741823L ; 
  minimaldemerits [ 1 ] = 1073741823L ; 
  minimaldemerits [ 0 ] = 1073741823L ; 
  if ( eqtb [ 10812 ] .hh .v.RH == 0 ) 
  if ( eqtb [ 13247 ] .cint == 0 ) 
  {
    lastspecialline = 0 ; 
    secondwidth = eqtb [ 13233 ] .cint ; 
    secondindent = 0 ; 
  } 
  else {
      
    lastspecialline = abs ( eqtb [ 12704 ] .cint ) ; 
    if ( eqtb [ 12704 ] .cint < 0 ) 
    {
      firstwidth = eqtb [ 13233 ] .cint - abs ( eqtb [ 13247 ] .cint ) ; 
      if ( eqtb [ 13247 ] .cint >= 0 ) 
      firstindent = eqtb [ 13247 ] .cint ; 
      else firstindent = 0 ; 
      secondwidth = eqtb [ 13233 ] .cint ; 
      secondindent = 0 ; 
    } 
    else {
	
      firstwidth = eqtb [ 13233 ] .cint ; 
      firstindent = 0 ; 
      secondwidth = eqtb [ 13233 ] .cint - abs ( eqtb [ 13247 ] .cint ) ; 
      if ( eqtb [ 13247 ] .cint >= 0 ) 
      secondindent = eqtb [ 13247 ] .cint ; 
      else secondindent = 0 ; 
    } 
  } 
  else {
      
    lastspecialline = mem [ eqtb [ 10812 ] .hh .v.RH ] .hh .v.LH - 1 ; 
    secondwidth = mem [ eqtb [ 10812 ] .hh .v.RH + 2 * ( lastspecialline + 1 ) 
    ] .cint ; 
    secondindent = mem [ eqtb [ 10812 ] .hh .v.RH + 2 * lastspecialline + 1 ] 
    .cint ; 
  } 
  if ( eqtb [ 12682 ] .cint == 0 ) 
  easyline = lastspecialline ; 
  else easyline = 262143L ; 
  threshold = eqtb [ 12663 ] .cint ; 
  if ( threshold >= 0 ) 
  {
	;
#ifdef STAT
    if ( eqtb [ 12695 ] .cint > 0 ) 
    {
      begindiagnostic () ; 
      printnl ( 926 ) ; 
    } 
#endif /* STAT */
    secondpass = false ; 
    finalpass = false ; 
  } 
  else {
      
    threshold = eqtb [ 12664 ] .cint ; 
    secondpass = true ; 
    finalpass = ( eqtb [ 13250 ] .cint <= 0 ) ; 
	;
#ifdef STAT
    if ( eqtb [ 12695 ] .cint > 0 ) 
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
      lhyf = curlist .lhmfield ; 
      rhyf = curlist .rhmfield ; 
      curlang = 0 ; 
    } 
    q = getnode ( 3 ) ; 
    mem [ q ] .hh.b0 = 0 ; 
    mem [ q ] .hh.b1 = 2 ; 
    mem [ q ] .hh .v.RH = memtop - 7 ; 
    mem [ q + 1 ] .hh .v.RH = 0 ; 
    mem [ q + 1 ] .hh .v.LH = curlist .pgfield + 1 ; 
    mem [ q + 2 ] .cint = 0 ; 
    mem [ memtop - 7 ] .hh .v.RH = q ; 
    activewidth [ 1 ] = background [ 1 ] ; 
    activewidth [ 2 ] = background [ 2 ] ; 
    activewidth [ 3 ] = background [ 3 ] ; 
    activewidth [ 4 ] = background [ 4 ] ; 
    activewidth [ 5 ] = background [ 5 ] ; 
    activewidth [ 6 ] = background [ 6 ] ; 
    passive = 0 ; 
    printednode = memtop - 3 ; 
    passnumber = 0 ; 
    fontinshortdisplay = 0 ; 
    curp = mem [ memtop - 3 ] .hh .v.RH ; 
    autobreaking = true ; 
    prevp = curp ; 
    while ( ( curp != 0 ) && ( mem [ memtop - 7 ] .hh .v.RH != memtop - 7 ) ) 
    {
      if ( ( curp >= himemmin ) ) 
      {
	prevp = curp ; 
	do {
	    f = mem [ curp ] .hh.b0 ; 
	  activewidth [ 1 ] = activewidth [ 1 ] + fontinfo [ widthbase [ f ] + 
	  fontinfo [ charbase [ f ] + mem [ curp ] .hh.b1 ] .qqqq .b0 ] .cint 
	  ; 
	  curp = mem [ curp ] .hh .v.RH ; 
	} while ( ! ( ! ( curp >= himemmin ) ) ) ; 
      } 
      switch ( mem [ curp ] .hh.b0 ) 
      {case 0 : 
      case 1 : 
      case 2 : 
	activewidth [ 1 ] = activewidth [ 1 ] + mem [ curp + 1 ] .cint ; 
	break ; 
      case 8 : 
	if ( mem [ curp ] .hh.b1 == 4 ) 
	{
	  curlang = mem [ curp + 1 ] .hh .v.RH ; 
	  lhyf = mem [ curp + 1 ] .hh.b0 ; 
	  rhyf = mem [ curp + 1 ] .hh.b1 ; 
	} 
	break ; 
      case 10 : 
	{
	  if ( autobreaking ) 
	  {
	    if ( ( prevp >= himemmin ) ) 
	    trybreak ( 0 , 0 ) ; 
	    else if ( ( mem [ prevp ] .hh.b0 < 9 ) ) 
	    trybreak ( 0 , 0 ) ; 
	  } 
	  if ( ( mem [ mem [ curp + 1 ] .hh .v.LH ] .hh.b1 != 0 ) && ( mem [ 
	  mem [ curp + 1 ] .hh .v.LH + 3 ] .cint != 0 ) ) 
	  {
	    mem [ curp + 1 ] .hh .v.LH = finiteshrink ( mem [ curp + 1 ] .hh 
	    .v.LH ) ; 
	  } 
	  q = mem [ curp + 1 ] .hh .v.LH ; 
	  activewidth [ 1 ] = activewidth [ 1 ] + mem [ q + 1 ] .cint ; 
	  activewidth [ 2 + mem [ q ] .hh.b0 ] = activewidth [ 2 + mem [ q ] 
	  .hh.b0 ] + mem [ q + 2 ] .cint ; 
	  activewidth [ 6 ] = activewidth [ 6 ] + mem [ q + 3 ] .cint ; 
	  if ( secondpass && autobreaking ) 
	  {
	    prevs = curp ; 
	    s = mem [ prevs ] .hh .v.RH ; 
	    if ( s != 0 ) 
	    {
	      while ( true ) {
		  
		if ( ( s >= himemmin ) ) 
		{
		  c = mem [ s ] .hh.b1 ; 
		  hf = mem [ s ] .hh.b0 ; 
		} 
		else if ( mem [ s ] .hh.b0 == 6 ) 
		if ( mem [ s + 1 ] .hh .v.RH == 0 ) 
		goto lab22 ; 
		else {
		    
		  q = mem [ s + 1 ] .hh .v.RH ; 
		  c = mem [ q ] .hh.b1 ; 
		  hf = mem [ q ] .hh.b0 ; 
		} 
		else if ( ( mem [ s ] .hh.b0 == 11 ) && ( mem [ s ] .hh.b1 == 
		0 ) ) 
		goto lab22 ; 
		else if ( mem [ s ] .hh.b0 == 8 ) 
		{
		  if ( mem [ s ] .hh.b1 == 4 ) 
		  {
		    curlang = mem [ s + 1 ] .hh .v.RH ; 
		    lhyf = mem [ s + 1 ] .hh.b0 ; 
		    rhyf = mem [ s + 1 ] .hh.b1 ; 
		  } 
		  goto lab22 ; 
		} 
		else goto lab31 ; 
		if ( eqtb [ 11639 + c ] .hh .v.RH != 0 ) 
		if ( ( eqtb [ 11639 + c ] .hh .v.RH == c ) || ( eqtb [ 12701 ] 
		.cint > 0 ) ) 
		goto lab32 ; 
		else goto lab31 ; 
		lab22: prevs = s ; 
		s = mem [ prevs ] .hh .v.RH ; 
	      } 
	      lab32: hyfchar = hyphenchar [ hf ] ; 
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
		  if ( mem [ s ] .hh.b0 != hf ) 
		  goto lab33 ; 
		  hyfbchar = mem [ s ] .hh.b1 ; 
		  c = hyfbchar ; 
		  if ( eqtb [ 11639 + c ] .hh .v.RH == 0 ) 
		  goto lab33 ; 
		  if ( hn == 63 ) 
		  goto lab33 ; 
		  hb = s ; 
		  incr ( hn ) ; 
		  hu [ hn ] = c ; 
		  hc [ hn ] = eqtb [ 11639 + c ] .hh .v.RH ; 
		  hyfbchar = 256 ; 
		} 
		else if ( mem [ s ] .hh.b0 == 6 ) 
		{
		  if ( mem [ s + 1 ] .hh.b0 != hf ) 
		  goto lab33 ; 
		  j = hn ; 
		  q = mem [ s + 1 ] .hh .v.RH ; 
		  if ( q > 0 ) 
		  hyfbchar = mem [ q ] .hh.b1 ; 
		  while ( q > 0 ) {
		      
		    c = mem [ q ] .hh.b1 ; 
		    if ( eqtb [ 11639 + c ] .hh .v.RH == 0 ) 
		    goto lab33 ; 
		    if ( j == 63 ) 
		    goto lab33 ; 
		    incr ( j ) ; 
		    hu [ j ] = c ; 
		    hc [ j ] = eqtb [ 11639 + c ] .hh .v.RH ; 
		    q = mem [ q ] .hh .v.RH ; 
		  } 
		  hb = s ; 
		  hn = j ; 
		  if ( odd ( mem [ s ] .hh.b1 ) ) 
		  hyfbchar = fontbchar [ hf ] ; 
		  else hyfbchar = 256 ; 
		} 
		else if ( ( mem [ s ] .hh.b0 == 11 ) && ( mem [ s ] .hh.b1 == 
		0 ) ) 
		hb = s ; 
		else goto lab33 ; 
		s = mem [ s ] .hh .v.RH ; 
	      } 
	      lab33: ; 
	      if ( hn < lhyf + rhyf ) 
	      goto lab31 ; 
	      while ( true ) {
		  
		if ( ! ( ( s >= himemmin ) ) ) 
		switch ( mem [ s ] .hh.b0 ) 
		{case 6 : 
		  ; 
		  break ; 
		case 11 : 
		  if ( mem [ s ] .hh.b1 != 0 ) 
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
		s = mem [ s ] .hh .v.RH ; 
	      } 
	      lab34: ; 
	      hyphenate () ; 
	    } 
	    lab31: ; 
	  } 
	} 
	break ; 
      case 11 : 
	{
	  if ( ! ( mem [ curp ] .hh .v.RH >= himemmin ) && autobreaking ) 
	  if ( mem [ mem [ curp ] .hh .v.RH ] .hh.b0 == 10 ) 
	  trybreak ( 0 , 0 ) ; 
	  activewidth [ 1 ] = activewidth [ 1 ] + mem [ curp + 1 ] .cint ; 
	} 
	break ; 
      case 6 : 
	{
	  f = mem [ curp + 1 ] .hh.b0 ; 
	  activewidth [ 1 ] = activewidth [ 1 ] + fontinfo [ widthbase [ f ] + 
	  fontinfo [ charbase [ f ] + mem [ curp + 1 ] .hh.b1 ] .qqqq .b0 ] 
	  .cint ; 
	} 
	break ; 
      case 7 : 
	{
	  s = mem [ curp + 1 ] .hh .v.LH ; 
	  discwidth = 0 ; 
	  if ( s == 0 ) 
	  trybreak ( eqtb [ 12667 ] .cint , 1 ) ; 
	  else {
	      
	    do {
		if ( ( s >= himemmin ) ) 
	      {
		f = mem [ s ] .hh.b0 ; 
		discwidth = discwidth + fontinfo [ widthbase [ f ] + fontinfo 
		[ charbase [ f ] + mem [ s ] .hh.b1 ] .qqqq .b0 ] .cint ; 
	      } 
	      else switch ( mem [ s ] .hh.b0 ) 
	      {case 6 : 
		{
		  f = mem [ s + 1 ] .hh.b0 ; 
		  discwidth = discwidth + fontinfo [ widthbase [ f ] + 
		  fontinfo [ charbase [ f ] + mem [ s + 1 ] .hh.b1 ] .qqqq .b0 
		  ] .cint ; 
		} 
		break ; 
	      case 0 : 
	      case 1 : 
	      case 2 : 
	      case 11 : 
		discwidth = discwidth + mem [ s + 1 ] .cint ; 
		break ; 
		default: 
		confusion ( 930 ) ; 
		break ; 
	      } 
	      s = mem [ s ] .hh .v.RH ; 
	    } while ( ! ( s == 0 ) ) ; 
	    activewidth [ 1 ] = activewidth [ 1 ] + discwidth ; 
	    trybreak ( eqtb [ 12666 ] .cint , 1 ) ; 
	    activewidth [ 1 ] = activewidth [ 1 ] - discwidth ; 
	  } 
	  r = mem [ curp ] .hh.b1 ; 
	  s = mem [ curp ] .hh .v.RH ; 
	  while ( r > 0 ) {
	      
	    if ( ( s >= himemmin ) ) 
	    {
	      f = mem [ s ] .hh.b0 ; 
	      activewidth [ 1 ] = activewidth [ 1 ] + fontinfo [ widthbase [ f 
	      ] + fontinfo [ charbase [ f ] + mem [ s ] .hh.b1 ] .qqqq .b0 ] 
	      .cint ; 
	    } 
	    else switch ( mem [ s ] .hh.b0 ) 
	    {case 6 : 
	      {
		f = mem [ s + 1 ] .hh.b0 ; 
		activewidth [ 1 ] = activewidth [ 1 ] + fontinfo [ widthbase [ 
		f ] + fontinfo [ charbase [ f ] + mem [ s + 1 ] .hh.b1 ] .qqqq 
		.b0 ] .cint ; 
	      } 
	      break ; 
	    case 0 : 
	    case 1 : 
	    case 2 : 
	    case 11 : 
	      activewidth [ 1 ] = activewidth [ 1 ] + mem [ s + 1 ] .cint ; 
	      break ; 
	      default: 
	      confusion ( 931 ) ; 
	      break ; 
	    } 
	    decr ( r ) ; 
	    s = mem [ s ] .hh .v.RH ; 
	  } 
	  prevp = curp ; 
	  curp = s ; 
	  goto lab35 ; 
	} 
	break ; 
      case 9 : 
	{
	  autobreaking = ( mem [ curp ] .hh.b1 == 1 ) ; 
	  {
	    if ( ! ( mem [ curp ] .hh .v.RH >= himemmin ) && autobreaking ) 
	    if ( mem [ mem [ curp ] .hh .v.RH ] .hh.b0 == 10 ) 
	    trybreak ( 0 , 0 ) ; 
	    activewidth [ 1 ] = activewidth [ 1 ] + mem [ curp + 1 ] .cint ; 
	  } 
	} 
	break ; 
      case 12 : 
	trybreak ( mem [ curp + 1 ] .cint , 0 ) ; 
	break ; 
      case 4 : 
      case 3 : 
      case 5 : 
	; 
	break ; 
	default: 
	confusion ( 929 ) ; 
	break ; 
      } 
      prevp = curp ; 
      curp = mem [ curp ] .hh .v.RH ; 
      lab35: ; 
    } 
    if ( curp == 0 ) 
    {
      trybreak ( -10000 , 1 ) ; 
      if ( mem [ memtop - 7 ] .hh .v.RH != memtop - 7 ) 
      {
	r = mem [ memtop - 7 ] .hh .v.RH ; 
	fewestdemerits = 1073741823L ; 
	do {
	    if ( mem [ r ] .hh.b0 != 2 ) 
	  if ( mem [ r + 2 ] .cint < fewestdemerits ) 
	  {
	    fewestdemerits = mem [ r + 2 ] .cint ; 
	    bestbet = r ; 
	  } 
	  r = mem [ r ] .hh .v.RH ; 
	} while ( ! ( r == memtop - 7 ) ) ; 
	bestline = mem [ bestbet + 1 ] .hh .v.LH ; 
	if ( eqtb [ 12682 ] .cint == 0 ) 
	goto lab30 ; 
	{
	  r = mem [ memtop - 7 ] .hh .v.RH ; 
	  actuallooseness = 0 ; 
	  do {
	      if ( mem [ r ] .hh.b0 != 2 ) 
	    {
	      linediff = toint ( mem [ r + 1 ] .hh .v.LH ) - toint ( bestline 
	      ) ; 
	      if ( ( ( linediff < actuallooseness ) && ( eqtb [ 12682 ] .cint 
	      <= linediff ) ) || ( ( linediff > actuallooseness ) && ( eqtb [ 
	      12682 ] .cint >= linediff ) ) ) 
	      {
		bestbet = r ; 
		actuallooseness = linediff ; 
		fewestdemerits = mem [ r + 2 ] .cint ; 
	      } 
	      else if ( ( linediff == actuallooseness ) && ( mem [ r + 2 ] 
	      .cint < fewestdemerits ) ) 
	      {
		bestbet = r ; 
		fewestdemerits = mem [ r + 2 ] .cint ; 
	      } 
	    } 
	    r = mem [ r ] .hh .v.RH ; 
	  } while ( ! ( r == memtop - 7 ) ) ; 
	  bestline = mem [ bestbet + 1 ] .hh .v.LH ; 
	} 
	if ( ( actuallooseness == eqtb [ 12682 ] .cint ) || finalpass ) 
	goto lab30 ; 
      } 
    } 
    q = mem [ memtop - 7 ] .hh .v.RH ; 
    while ( q != memtop - 7 ) {
	
      curp = mem [ q ] .hh .v.RH ; 
      if ( mem [ q ] .hh.b0 == 2 ) 
      freenode ( q , 7 ) ; 
      else freenode ( q , 3 ) ; 
      q = curp ; 
    } 
    q = passive ; 
    while ( q != 0 ) {
	
      curp = mem [ q ] .hh .v.RH ; 
      freenode ( q , 2 ) ; 
      q = curp ; 
    } 
    if ( ! secondpass ) 
    {
	;
#ifdef STAT
      if ( eqtb [ 12695 ] .cint > 0 ) 
      printnl ( 927 ) ; 
#endif /* STAT */
      threshold = eqtb [ 12664 ] .cint ; 
      secondpass = true ; 
      finalpass = ( eqtb [ 13250 ] .cint <= 0 ) ; 
    } 
    else {
	
	;
#ifdef STAT
      if ( eqtb [ 12695 ] .cint > 0 ) 
      printnl ( 928 ) ; 
#endif /* STAT */
      background [ 2 ] = background [ 2 ] + eqtb [ 13250 ] .cint ; 
      finalpass = true ; 
    } 
  } 
  lab30: 
	;
#ifdef STAT
  if ( eqtb [ 12695 ] .cint > 0 ) 
  {
    enddiagnostic ( true ) ; 
    normalizeselector () ; 
  } 
#endif /* STAT */
  postlinebreak ( finalwidowpenalty ) ; 
  q = mem [ memtop - 7 ] .hh .v.RH ; 
  while ( q != memtop - 7 ) {
      
    curp = mem [ q ] .hh .v.RH ; 
    if ( mem [ q ] .hh.b0 == 2 ) 
    freenode ( q , 7 ) ; 
    else freenode ( q , 3 ) ; 
    q = curp ; 
  } 
  q = passive ; 
  while ( q != 0 ) {
      
    curp = mem [ q ] .hh .v.RH ; 
    freenode ( q , 2 ) ; 
    q = curp ; 
  } 
  packbeginline = 0 ; 
} 
void prefixedcommand ( ) 
{/* 30 10 */ prefixedcommand_regmem 
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
	print ( 1172 ) ; 
      } 
      printcmdchr ( curcmd , curchr ) ; 
      printchar ( 39 ) ; 
      {
	helpptr = 1 ; 
	helpline [ 0 ] = 1173 ; 
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
      print ( 681 ) ; 
    } 
    printesc ( 1164 ) ; 
    print ( 1174 ) ; 
    printesc ( 1165 ) ; 
    print ( 1175 ) ; 
    printcmdchr ( curcmd , curchr ) ; 
    printchar ( 39 ) ; 
    {
      helpptr = 1 ; 
      helpline [ 0 ] = 1176 ; 
    } 
    error () ; 
  } 
  if ( eqtb [ 12706 ] .cint != 0 ) 
  if ( eqtb [ 12706 ] .cint < 0 ) 
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
    geqdefine ( 11334 , 120 , curchr ) ; 
    else eqdefine ( 11334 , 120 , curchr ) ; 
    break ; 
  case 97 : 
    {
      if ( odd ( curchr ) && ! ( a >= 4 ) && ( eqtb [ 12706 ] .cint >= 0 ) ) 
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
      incr ( mem [ curchr ] .hh .v.LH ) ; 
      if ( ( a >= 4 ) ) 
      geqdefine ( p , curcmd , curchr ) ; 
      else eqdefine ( p , curcmd , curchr ) ; 
    } 
    break ; 
  case 95 : 
    {
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
	    geqdefine ( p , 73 , 12718 + curval ) ; 
	    else eqdefine ( p , 73 , 12718 + curval ) ; 
	    break ; 
	  case 3 : 
	    if ( ( a >= 4 ) ) 
	    geqdefine ( p , 74 , 13251 + curval ) ; 
	    else eqdefine ( p , 74 , 13251 + curval ) ; 
	    break ; 
	  case 4 : 
	    if ( ( a >= 4 ) ) 
	    geqdefine ( p , 75 , 10300 + curval ) ; 
	    else eqdefine ( p , 75 , 10300 + curval ) ; 
	    break ; 
	  case 5 : 
	    if ( ( a >= 4 ) ) 
	    geqdefine ( p , 76 , 10556 + curval ) ; 
	    else eqdefine ( p , 76 , 10556 + curval ) ; 
	    break ; 
	  case 6 : 
	    if ( ( a >= 4 ) ) 
	    geqdefine ( p , 72 , 10822 + curval ) ; 
	    else eqdefine ( p , 72 , 10822 + curval ) ; 
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
      if ( ! scankeyword ( 835 ) ) 
      {
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 1066 ) ; 
	} 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 1193 ; 
	  helpline [ 0 ] = 1194 ; 
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
	p = 10822 + curval ; 
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
	  curchr = 10822 + curval ; 
	} 
	if ( curcmd == 72 ) 
	{
	  q = eqtb [ curchr ] .hh .v.RH ; 
	  if ( q == 0 ) 
	  if ( ( a >= 4 ) ) 
	  geqdefine ( p , 101 , 0 ) ; 
	  else eqdefine ( p , 101 , 0 ) ; 
	  else {
	      
	    incr ( mem [ q ] .hh .v.LH ) ; 
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
      if ( mem [ defref ] .hh .v.RH == 0 ) 
      {
	if ( ( a >= 4 ) ) 
	geqdefine ( p , 101 , 0 ) ; 
	else eqdefine ( p , 101 , 0 ) ; 
	{
	  mem [ defref ] .hh .v.RH = avail ; 
	  avail = defref ; 
	;
#ifdef STAT
	  decr ( dynused ) ; 
#endif /* STAT */
	} 
      } 
      else {
	  
	if ( p == 10813 ) 
	{
	  mem [ q ] .hh .v.RH = getavail () ; 
	  q = mem [ q ] .hh .v.RH ; 
	  mem [ q ] .hh .v.LH = 637 ; 
	  q = getavail () ; 
	  mem [ q ] .hh .v.LH = 379 ; 
	  mem [ q ] .hh .v.RH = mem [ defref ] .hh .v.RH ; 
	  mem [ defref ] .hh .v.RH = q ; 
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
      if ( curchr == 11383 ) 
      n = 15 ; 
      else if ( curchr == 12407 ) 
      n = 32768L ; 
      else if ( curchr == 12151 ) 
      n = 32767 ; 
      else if ( curchr == 12974 ) 
      n = 16777215L ; 
      else n = 255 ; 
      p = curchr ; 
      scancharnum () ; 
      p = p + curval ; 
      scanoptionalequals () ; 
      scanint () ; 
      if ( ( ( curval < 0 ) && ( p < 12974 ) ) || ( curval > n ) ) 
      {
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 1195 ) ; 
	} 
	printint ( curval ) ; 
	if ( p < 12974 ) 
	print ( 1196 ) ; 
	else print ( 1197 ) ; 
	printint ( n ) ; 
	{
	  helpptr = 1 ; 
	  helpline [ 0 ] = 1198 ; 
	} 
	error () ; 
	curval = 0 ; 
      } 
      if ( p < 12407 ) 
      if ( ( a >= 4 ) ) 
      geqdefine ( p , 120 , curval ) ; 
      else eqdefine ( p , 120 , curval ) ; 
      else if ( p < 12974 ) 
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
	  print ( 676 ) ; 
	} 
	printesc ( 532 ) ; 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 1204 ; 
	  helpline [ 0 ] = 1205 ; 
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
	mem [ p ] .hh .v.LH = n ; 
	{register integer for_end; j = 1 ; for_end = n ; if ( j <= for_end) 
	do 
	  {
	    scandimen ( false , false , false ) ; 
	    mem [ p + 2 * j - 1 ] .cint = curval ; 
	    scandimen ( false , false , false ) ; 
	    mem [ p + 2 * j ] .cint = curval ; 
	  } 
	while ( j++ < for_end ) ; } 
      } 
      if ( ( a >= 4 ) ) 
      geqdefine ( 10812 , 118 , p ) ; 
      else eqdefine ( 10812 , 118 , p ) ; 
    } 
    break ; 
  case 99 : 
    if ( curchr == 1 ) 
    {
	;
#ifdef INITEX
      newpatterns () ; 
      goto lab30 ; 
#endif /* INITEX */
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 262 ) ; 
	print ( 1209 ) ; 
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
      fontinfo [ k ] .cint = curval ; 
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
      hyphenchar [ f ] = curval ; 
      else skewchar [ f ] = curval ; 
    } 
    break ; 
  case 88 : 
    newfont ( a ) ; 
    break ; 
  case 100 : 
    newinteraction () ; 
    break ; 
    default: 
    confusion ( 1171 ) ; 
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
void storefmtfile ( ) 
{/* 41 42 31 32 */ storefmtfile_regmem 
  integer j, k, l  ; 
  halfword p, q  ; 
  integer x  ; 
  if ( saveptr != 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 1251 ) ; 
    } 
    {
      helpptr = 1 ; 
      helpline [ 0 ] = 1252 ; 
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
  selector = 21 ; 
  print ( 1265 ) ; 
  print ( jobname ) ; 
  printchar ( 32 ) ; 
  printint ( eqtb [ 12686 ] .cint % 100 ) ; 
  printchar ( 46 ) ; 
  printint ( eqtb [ 12685 ] .cint ) ; 
  printchar ( 46 ) ; 
  printint ( eqtb [ 12684 ] .cint ) ; 
  printchar ( 41 ) ; 
  if ( interaction == 0 ) 
  selector = 18 ; 
  else selector = 19 ; 
  {
    if ( poolptr + 1 > poolsize ) 
    overflow ( 257 , poolsize - initpoolptr ) ; 
  } 
  formatident = makestring () ; 
  packjobname ( 779 ) ; 
  while ( ! wopenout ( fmtfile ) ) promptfilename ( 1266 , 779 ) ; 
  printnl ( 1267 ) ; 
  slowprint ( wmakenamestring ( fmtfile ) ) ; 
  {
    decr ( strptr ) ; 
    poolptr = strstart [ strptr ] ; 
  } 
  printnl ( 335 ) ; 
  slowprint ( formatident ) ; 
  dumpint ( 353758006L ) ; 
  dumpint ( 0 ) ; 
  dumpint ( memtop ) ; 
  dumpint ( 13506 ) ; 
  dumpint ( 7919 ) ; 
  dumpint ( 607 ) ; 
  dumpint ( poolptr ) ; 
  dumpint ( strptr ) ; 
  dumpthings ( strstart [ 0 ] , strptr + 1 ) ; 
  dumpthings ( strpool [ 0 ] , poolptr ) ; 
  println () ; 
  printint ( strptr ) ; 
  print ( 1253 ) ; 
  printint ( poolptr ) ; 
  sortavail () ; 
  varused = 0 ; 
  dumpint ( lomemmax ) ; 
  dumpint ( rover ) ; 
  p = 0 ; 
  q = rover ; 
  x = 0 ; 
  do {
      dumpthings ( mem [ p ] , q + 2 - p ) ; 
    x = x + q + 2 - p ; 
    varused = varused + q - p ; 
    p = q + mem [ q ] .hh .v.LH ; 
    q = mem [ q + 1 ] .hh .v.RH ; 
  } while ( ! ( q == rover ) ) ; 
  varused = varused + lomemmax - p ; 
  dynused = memend + 1 - himemmin ; 
  dumpthings ( mem [ p ] , lomemmax + 1 - p ) ; 
  x = x + lomemmax + 1 - p ; 
  dumpint ( himemmin ) ; 
  dumpint ( avail ) ; 
  dumpthings ( mem [ himemmin ] , memend + 1 - himemmin ) ; 
  x = x + memend + 1 - himemmin ; 
  p = avail ; 
  while ( p != 0 ) {
      
    decr ( dynused ) ; 
    p = mem [ p ] .hh .v.RH ; 
  } 
  dumpint ( varused ) ; 
  dumpint ( dynused ) ; 
  println () ; 
  printint ( x ) ; 
  print ( 1254 ) ; 
  printint ( varused ) ; 
  printchar ( 38 ) ; 
  printint ( dynused ) ; 
  k = 1 ; 
  do {
      j = k ; 
    while ( j < 12662 ) {
	
      if ( ( eqtb [ j ] .hh .v.RH == eqtb [ j + 1 ] .hh .v.RH ) && ( eqtb [ j 
      ] .hh.b0 == eqtb [ j + 1 ] .hh.b0 ) && ( eqtb [ j ] .hh.b1 == eqtb [ j + 
      1 ] .hh.b1 ) ) 
      goto lab41 ; 
      incr ( j ) ; 
    } 
    l = 12663 ; 
    goto lab31 ; 
    lab41: incr ( j ) ; 
    l = j ; 
    while ( j < 12662 ) {
	
      if ( ( eqtb [ j ] .hh .v.RH != eqtb [ j + 1 ] .hh .v.RH ) || ( eqtb [ j 
      ] .hh.b0 != eqtb [ j + 1 ] .hh.b0 ) || ( eqtb [ j ] .hh.b1 != eqtb [ j + 
      1 ] .hh.b1 ) ) 
      goto lab31 ; 
      incr ( j ) ; 
    } 
    lab31: dumpint ( l - k ) ; 
    dumpthings ( eqtb [ k ] , l - k ) ; 
    k = j + 1 ; 
    dumpint ( k - l ) ; 
  } while ( ! ( k == 12663 ) ) ; 
  do {
      j = k ; 
    while ( j < 13506 ) {
	
      if ( eqtb [ j ] .cint == eqtb [ j + 1 ] .cint ) 
      goto lab42 ; 
      incr ( j ) ; 
    } 
    l = 13507 ; 
    goto lab32 ; 
    lab42: incr ( j ) ; 
    l = j ; 
    while ( j < 13506 ) {
	
      if ( eqtb [ j ] .cint != eqtb [ j + 1 ] .cint ) 
      goto lab32 ; 
      incr ( j ) ; 
    } 
    lab32: dumpint ( l - k ) ; 
    dumpthings ( eqtb [ k ] , l - k ) ; 
    k = j + 1 ; 
    dumpint ( k - l ) ; 
  } while ( ! ( k > 13506 ) ) ; 
  dumpint ( parloc ) ; 
  dumpint ( writeloc ) ; 
  dumpint ( hashused ) ; 
  cscount = 10013 - hashused ; 
  {register integer for_end; p = 514 ; for_end = hashused ; if ( p <= 
  for_end) do 
    if ( hash [ p ] .v.RH != 0 ) 
    {
      dumpint ( p ) ; 
      dumphh ( hash [ p ] ) ; 
      incr ( cscount ) ; 
    } 
  while ( p++ < for_end ) ; } 
  dumpthings ( hash [ hashused + 1 ] , 10280 - hashused ) ; 
  dumpint ( cscount ) ; 
  println () ; 
  printint ( cscount ) ; 
  print ( 1255 ) ; 
  dumpint ( fmemptr ) ; 
  {
    dumpthings ( fontinfo [ 0 ] , fmemptr ) ; 
    dumpint ( fontptr ) ; 
    dumpthings ( fontcheck [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( fontsize [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( fontdsize [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( fontparams [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( hyphenchar [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( skewchar [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( fontname [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( fontarea [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( fontbc [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( fontec [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( charbase [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( widthbase [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( heightbase [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( depthbase [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( italicbase [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( ligkernbase [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( kernbase [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( extenbase [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( parambase [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( fontglue [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( bcharlabel [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( fontbchar [ 0 ] , fontptr + 1 ) ; 
    dumpthings ( fontfalsebchar [ 0 ] , fontptr + 1 ) ; 
    {register integer for_end; k = 0 ; for_end = fontptr ; if ( k <= for_end) 
    do 
      {
	printnl ( 1258 ) ; 
	printesc ( hash [ 10024 + k ] .v.RH ) ; 
	printchar ( 61 ) ; 
	printfilename ( fontname [ k ] , fontarea [ k ] , 335 ) ; 
	if ( fontsize [ k ] != fontdsize [ k ] ) 
	{
	  print ( 737 ) ; 
	  printscaled ( fontsize [ k ] ) ; 
	  print ( 393 ) ; 
	} 
      } 
    while ( k++ < for_end ) ; } 
  } 
  println () ; 
  printint ( fmemptr - 7 ) ; 
  print ( 1256 ) ; 
  printint ( fontptr - 0 ) ; 
  print ( 1257 ) ; 
  if ( fontptr != 1 ) 
  printchar ( 115 ) ; 
  dumpint ( hyphcount ) ; 
  {register integer for_end; k = 0 ; for_end = 607 ; if ( k <= for_end) do 
    if ( hyphword [ k ] != 0 ) 
    {
      dumpint ( k ) ; 
      dumpint ( hyphword [ k ] ) ; 
      dumpint ( hyphlist [ k ] ) ; 
    } 
  while ( k++ < for_end ) ; } 
  println () ; 
  printint ( hyphcount ) ; 
  print ( 1259 ) ; 
  if ( hyphcount != 1 ) 
  printchar ( 115 ) ; 
  if ( trienotready ) 
  inittrie () ; 
  dumpint ( triemax ) ; 
  dumpthings ( trietrl [ 0 ] , triemax + 1 ) ; 
  dumpthings ( trietro [ 0 ] , triemax + 1 ) ; 
  dumpthings ( trietrc [ 0 ] , triemax + 1 ) ; 
  dumpint ( trieopptr ) ; 
  dumpthings ( hyfdistance [ 1 ] , trieopptr ) ; 
  dumpthings ( hyfnum [ 1 ] , trieopptr ) ; 
  dumpthings ( hyfnext [ 1 ] , trieopptr ) ; 
  printnl ( 1260 ) ; 
  printint ( triemax ) ; 
  print ( 1261 ) ; 
  printint ( trieopptr ) ; 
  print ( 1262 ) ; 
  if ( trieopptr != 1 ) 
  printchar ( 115 ) ; 
  print ( 1263 ) ; 
  printint ( trieopsize ) ; 
  {register integer for_end; k = 255 ; for_end = 0 ; if ( k >= for_end) do 
    if ( trieused [ k ] > 0 ) 
    {
      printnl ( 793 ) ; 
      printint ( trieused [ k ] ) ; 
      print ( 1264 ) ; 
      printint ( k ) ; 
      dumpint ( k ) ; 
      dumpint ( trieused [ k ] ) ; 
    } 
  while ( k-- > for_end ) ; } 
  dumpint ( interaction ) ; 
  dumpint ( formatident ) ; 
  dumpint ( 69069L ) ; 
  eqtb [ 12694 ] .cint = 0 ; 
  wclose ( fmtfile ) ; 
} 
#endif /* INITEX */
boolean loadfmtfile ( ) 
{/* 6666 10 */ register boolean Result; loadfmtfile_regmem 
  integer j, k  ; 
  halfword p, q  ; 
  integer x  ; 
  undumpint ( x ) ; 
  if ( x != 353758006L ) 
  goto lab6666 ; 
  undumpint ( x ) ; 
  if ( x != 0 ) 
  goto lab6666 ; 
  undumpint ( x ) ; 
  if ( x != memtop ) 
  goto lab6666 ; 
  undumpint ( x ) ; 
  if ( x != 13506 ) 
  goto lab6666 ; 
  undumpint ( x ) ; 
  if ( x != 7919 ) 
  goto lab6666 ; 
  undumpint ( x ) ; 
  if ( x != 607 ) 
  goto lab6666 ; 
  {
    undumpint ( x ) ; 
    if ( x < 0 ) 
    goto lab6666 ; 
    if ( x > poolsize ) 
    {
      ; 
      (void) fprintf( stdout , "%s%s\n",  "---! Must increase the " , "string pool size" ) ; 
      goto lab6666 ; 
    } 
    else poolptr = x ; 
  } 
  {
    undumpint ( x ) ; 
    if ( x < 0 ) 
    goto lab6666 ; 
    if ( x > maxstrings ) 
    {
      ; 
      (void) fprintf( stdout , "%s%s\n",  "---! Must increase the " , "max strings" ) ; 
      goto lab6666 ; 
    } 
    else strptr = x ; 
  } 
  undumpthings ( strstart [ 0 ] , strptr + 1 ) ; 
  undumpthings ( strpool [ 0 ] , poolptr ) ; 
  initstrptr = strptr ; 
  initpoolptr = poolptr ; 
  {
    undumpint ( x ) ; 
    if ( ( x < 1019 ) || ( x > memtop - 14 ) ) 
    goto lab6666 ; 
    else lomemmax = x ; 
  } 
  {
    undumpint ( x ) ; 
    if ( ( x < 20 ) || ( x > lomemmax ) ) 
    goto lab6666 ; 
    else rover = x ; 
  } 
  p = 0 ; 
  q = rover ; 
  do {
      undumpthings ( mem [ p ] , q + 2 - p ) ; 
    p = q + mem [ q ] .hh .v.LH ; 
    if ( ( p > lomemmax ) || ( ( q >= mem [ q + 1 ] .hh .v.RH ) && ( mem [ q + 
    1 ] .hh .v.RH != rover ) ) ) 
    goto lab6666 ; 
    q = mem [ q + 1 ] .hh .v.RH ; 
  } while ( ! ( q == rover ) ) ; 
  undumpthings ( mem [ p ] , lomemmax + 1 - p ) ; 
  if ( memmin < -2 ) 
  {
    p = mem [ rover + 1 ] .hh .v.LH ; 
    q = memmin + 1 ; 
    mem [ memmin ] .hh .v.RH = 0 ; 
    mem [ memmin ] .hh .v.LH = 0 ; 
    mem [ p + 1 ] .hh .v.RH = q ; 
    mem [ rover + 1 ] .hh .v.LH = q ; 
    mem [ q + 1 ] .hh .v.RH = rover ; 
    mem [ q + 1 ] .hh .v.LH = p ; 
    mem [ q ] .hh .v.RH = 262143L ; 
    mem [ q ] .hh .v.LH = -0 - q ; 
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
  undumpthings ( mem [ himemmin ] , memend + 1 - himemmin ) ; 
  undumpint ( varused ) ; 
  undumpint ( dynused ) ; 
  k = 1 ; 
  do {
      undumpint ( x ) ; 
    if ( ( x < 1 ) || ( k + x > 13507 ) ) 
    goto lab6666 ; 
    undumpthings ( eqtb [ k ] , x ) ; 
    k = k + x ; 
    undumpint ( x ) ; 
    if ( ( x < 0 ) || ( k + x > 13507 ) ) 
    goto lab6666 ; 
    {register integer for_end; j = k ; for_end = k + x - 1 ; if ( j <= 
    for_end) do 
      eqtb [ j ] = eqtb [ k - 1 ] ; 
    while ( j++ < for_end ) ; } 
    k = k + x ; 
  } while ( ! ( k > 13506 ) ) ; 
  {
    undumpint ( x ) ; 
    if ( ( x < 514 ) || ( x > 10014 ) ) 
    goto lab6666 ; 
    else parloc = x ; 
  } 
  partoken = 4095 + parloc ; 
  {
    undumpint ( x ) ; 
    if ( ( x < 514 ) || ( x > 10014 ) ) 
    goto lab6666 ; 
    else
    writeloc = x ; 
  } 
  {
    undumpint ( x ) ; 
    if ( ( x < 514 ) || ( x > 10014 ) ) 
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
    undumphh ( hash [ p ] ) ; 
  } while ( ! ( p == hashused ) ) ; 
  undumpthings ( hash [ hashused + 1 ] , 10280 - hashused ) ; 
  undumpint ( cscount ) ; 
  {
    undumpint ( x ) ; 
    if ( x < 7 ) 
    goto lab6666 ; 
    if ( x > fontmemsize ) 
    {
      ; 
      (void) fprintf( stdout , "%s%s\n",  "---! Must increase the " , "font mem size" ) ; 
      goto lab6666 ; 
    } 
    else fmemptr = x ; 
  } 
  {
    undumpthings ( fontinfo [ 0 ] , fmemptr ) ; 
    {
      undumpint ( x ) ; 
      if ( x < 0 ) 
      goto lab6666 ; 
      if ( x > fontmax ) 
      {
	; 
	(void) fprintf( stdout , "%s%s\n",  "---! Must increase the " , "font max" ) ; 
	goto lab6666 ; 
      } 
      else fontptr = x ; 
    } 
    undumpthings ( fontcheck [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( fontsize [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( fontdsize [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( fontparams [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( hyphenchar [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( skewchar [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( fontname [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( fontarea [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( fontbc [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( fontec [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( charbase [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( widthbase [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( heightbase [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( depthbase [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( italicbase [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( ligkernbase [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( kernbase [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( extenbase [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( parambase [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( fontglue [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( bcharlabel [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( fontbchar [ 0 ] , fontptr + 1 ) ; 
    undumpthings ( fontfalsebchar [ 0 ] , fontptr + 1 ) ; 
  } 
  {
    undumpint ( x ) ; 
    if ( ( x < 0 ) || ( x > 607 ) ) 
    goto lab6666 ; 
    else hyphcount = x ; 
  } 
  {register integer for_end; k = 1 ; for_end = hyphcount ; if ( k <= for_end) 
  do 
    {
      {
	undumpint ( x ) ; 
	if ( ( x < 0 ) || ( x > 607 ) ) 
	goto lab6666 ; 
	else j = x ; 
      } 
      {
	undumpint ( x ) ; 
	if ( ( x < 0 ) || ( x > strptr ) ) 
	goto lab6666 ; 
	else hyphword [ j ] = x ; 
      } 
      {
	undumpint ( x ) ; 
	if ( ( x < 0 ) || ( x > 262143L ) ) 
	goto lab6666 ; 
	else hyphlist [ j ] = x ; 
      } 
    } 
  while ( k++ < for_end ) ; } 
  {
    undumpint ( x ) ; 
    if ( x < 0 ) 
    goto lab6666 ; 
    if ( x > triesize ) 
    {
      ; 
      (void) fprintf( stdout , "%s%s\n",  "---! Must increase the " , "trie size" ) ; 
      goto lab6666 ; 
    } 
    else j = x ; 
  } 
	;
#ifdef INITEX
  triemax = j ; 
#endif /* INITEX */
  undumpthings ( trietrl [ 0 ] , j + 1 ) ; 
  undumpthings ( trietro [ 0 ] , j + 1 ) ; 
  undumpthings ( trietrc [ 0 ] , j + 1 ) ; 
  {
    undumpint ( x ) ; 
    if ( x < 0 ) 
    goto lab6666 ; 
    if ( x > trieopsize ) 
    {
      ; 
      (void) fprintf( stdout , "%s%s\n",  "---! Must increase the " , "trie op size" ) ; 
      goto lab6666 ; 
    } 
    else j = x ; 
  } 
	;
#ifdef INITEX
  trieopptr = j ; 
#endif /* INITEX */
  undumpthings ( hyfdistance [ 1 ] , j ) ; 
  undumpthings ( hyfnum [ 1 ] , j ) ; 
  undumpthings ( hyfnext [ 1 ] , j ) ; 
	;
#ifdef INITEX
  {register integer for_end; k = 0 ; for_end = 255 ; if ( k <= for_end) do 
    trieused [ k ] = 0 ; 
  while ( k++ < for_end ) ; } 
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
    trieused [ k ] = x ; 
#endif /* INITEX */
    j = j - x ; 
    opstart [ k ] = j ; 
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
  return(Result) ; 
  lab6666: ; 
  (void) fprintf( stdout , "%s\n",  "(Fatal format file error; I'm stymied)" ) ; 
  Result = false ; 
  return(Result) ; 
} 
void finalcleanup ( ) 
{/* 10 */ finalcleanup_regmem 
  smallnumber c  ; 
  c = curchr ; 
  if ( jobname == 0 ) 
  openlogfile () ; 
  while ( openparens > 0 ) {
      
    print ( 1269 ) ; 
    decr ( openparens ) ; 
  } 
  if ( curlevel > 1 ) 
  {
    printnl ( 40 ) ; 
    printesc ( 1270 ) ; 
    print ( 1271 ) ; 
    printint ( curlevel - 1 ) ; 
    printchar ( 41 ) ; 
  } 
  while ( condptr != 0 ) {
      
    printnl ( 40 ) ; 
    printesc ( 1270 ) ; 
    print ( 1272 ) ; 
    printcmdchr ( 105 , curif ) ; 
    if ( ifline != 0 ) 
    {
      print ( 1273 ) ; 
      printint ( ifline ) ; 
    } 
    print ( 1274 ) ; 
    ifline = mem [ condptr + 1 ] .cint ; 
    curif = mem [ condptr ] .hh.b1 ; 
    condptr = mem [ condptr ] .hh .v.RH ; 
  } 
  if ( history != 0 ) 
  if ( ( ( history == 1 ) || ( interaction < 3 ) ) ) 
  if ( selector == 19 ) 
  {
    selector = 17 ; 
    printnl ( 1275 ) ; 
    selector = 19 ; 
  } 
  if ( c == 1 ) 
  {
	;
#ifdef INITEX
    storefmtfile () ; 
    return ; 
#endif /* INITEX */
    printnl ( 1276 ) ; 
    return ; 
  } 
} 
#ifdef INITEX
void initprim ( ) 
{initprim_regmem 
  nonewcontrolsequence = false ; 
  primitive ( 372 , 75 , 10282 ) ; 
  primitive ( 373 , 75 , 10283 ) ; 
  primitive ( 374 , 75 , 10284 ) ; 
  primitive ( 375 , 75 , 10285 ) ; 
  primitive ( 376 , 75 , 10286 ) ; 
  primitive ( 377 , 75 , 10287 ) ; 
  primitive ( 378 , 75 , 10288 ) ; 
  primitive ( 379 , 75 , 10289 ) ; 
  primitive ( 380 , 75 , 10290 ) ; 
  primitive ( 381 , 75 , 10291 ) ; 
  primitive ( 382 , 75 , 10292 ) ; 
  primitive ( 383 , 75 , 10293 ) ; 
  primitive ( 384 , 75 , 10294 ) ; 
  primitive ( 385 , 75 , 10295 ) ; 
  primitive ( 386 , 75 , 10296 ) ; 
  primitive ( 387 , 76 , 10297 ) ; 
  primitive ( 388 , 76 , 10298 ) ; 
  primitive ( 389 , 76 , 10299 ) ; 
  primitive ( 394 , 72 , 10813 ) ; 
  primitive ( 395 , 72 , 10814 ) ; 
  primitive ( 396 , 72 , 10815 ) ; 
  primitive ( 397 , 72 , 10816 ) ; 
  primitive ( 398 , 72 , 10817 ) ; 
  primitive ( 399 , 72 , 10818 ) ; 
  primitive ( 400 , 72 , 10819 ) ; 
  primitive ( 401 , 72 , 10820 ) ; 
  primitive ( 402 , 72 , 10821 ) ; 
  primitive ( 416 , 73 , 12663 ) ; 
  primitive ( 417 , 73 , 12664 ) ; 
  primitive ( 418 , 73 , 12665 ) ; 
  primitive ( 419 , 73 , 12666 ) ; 
  primitive ( 420 , 73 , 12667 ) ; 
  primitive ( 421 , 73 , 12668 ) ; 
  primitive ( 422 , 73 , 12669 ) ; 
  primitive ( 423 , 73 , 12670 ) ; 
  primitive ( 424 , 73 , 12671 ) ; 
  primitive ( 425 , 73 , 12672 ) ; 
  primitive ( 426 , 73 , 12673 ) ; 
  primitive ( 427 , 73 , 12674 ) ; 
  primitive ( 428 , 73 , 12675 ) ; 
  primitive ( 429 , 73 , 12676 ) ; 
  primitive ( 430 , 73 , 12677 ) ; 
  primitive ( 431 , 73 , 12678 ) ; 
  primitive ( 432 , 73 , 12679 ) ; 
  primitive ( 433 , 73 , 12680 ) ; 
  primitive ( 434 , 73 , 12681 ) ; 
  primitive ( 435 , 73 , 12682 ) ; 
  primitive ( 436 , 73 , 12683 ) ; 
  primitive ( 437 , 73 , 12684 ) ; 
  primitive ( 438 , 73 , 12685 ) ; 
  primitive ( 439 , 73 , 12686 ) ; 
  primitive ( 440 , 73 , 12687 ) ; 
  primitive ( 441 , 73 , 12688 ) ; 
  primitive ( 442 , 73 , 12689 ) ; 
  primitive ( 443 , 73 , 12690 ) ; 
  primitive ( 444 , 73 , 12691 ) ; 
  primitive ( 445 , 73 , 12692 ) ; 
  primitive ( 446 , 73 , 12693 ) ; 
  primitive ( 447 , 73 , 12694 ) ; 
  primitive ( 448 , 73 , 12695 ) ; 
  primitive ( 449 , 73 , 12696 ) ; 
  primitive ( 450 , 73 , 12697 ) ; 
  primitive ( 451 , 73 , 12698 ) ; 
  primitive ( 452 , 73 , 12699 ) ; 
  primitive ( 453 , 73 , 12700 ) ; 
  primitive ( 454 , 73 , 12701 ) ; 
  primitive ( 455 , 73 , 12702 ) ; 
  primitive ( 456 , 73 , 12703 ) ; 
  primitive ( 457 , 73 , 12704 ) ; 
  primitive ( 458 , 73 , 12705 ) ; 
  primitive ( 459 , 73 , 12706 ) ; 
  primitive ( 460 , 73 , 12707 ) ; 
  primitive ( 461 , 73 , 12708 ) ; 
  primitive ( 462 , 73 , 12709 ) ; 
  primitive ( 463 , 73 , 12710 ) ; 
  primitive ( 464 , 73 , 12711 ) ; 
  primitive ( 465 , 73 , 12712 ) ; 
  primitive ( 466 , 73 , 12713 ) ; 
  primitive ( 467 , 73 , 12714 ) ; 
  primitive ( 468 , 73 , 12715 ) ; 
  primitive ( 469 , 73 , 12716 ) ; 
  primitive ( 470 , 73 , 12717 ) ; 
  primitive ( 474 , 74 , 13230 ) ; 
  primitive ( 475 , 74 , 13231 ) ; 
  primitive ( 476 , 74 , 13232 ) ; 
  primitive ( 477 , 74 , 13233 ) ; 
  primitive ( 478 , 74 , 13234 ) ; 
  primitive ( 479 , 74 , 13235 ) ; 
  primitive ( 480 , 74 , 13236 ) ; 
  primitive ( 481 , 74 , 13237 ) ; 
  primitive ( 482 , 74 , 13238 ) ; 
  primitive ( 483 , 74 , 13239 ) ; 
  primitive ( 484 , 74 , 13240 ) ; 
  primitive ( 485 , 74 , 13241 ) ; 
  primitive ( 486 , 74 , 13242 ) ; 
  primitive ( 487 , 74 , 13243 ) ; 
  primitive ( 488 , 74 , 13244 ) ; 
  primitive ( 489 , 74 , 13245 ) ; 
  primitive ( 490 , 74 , 13246 ) ; 
  primitive ( 491 , 74 , 13247 ) ; 
  primitive ( 492 , 74 , 13248 ) ; 
  primitive ( 493 , 74 , 13249 ) ; 
  primitive ( 494 , 74 , 13250 ) ; 
  primitive ( 32 , 64 , 0 ) ; 
  primitive ( 47 , 44 , 0 ) ; 
  primitive ( 504 , 45 , 0 ) ; 
  primitive ( 505 , 90 , 0 ) ; 
  primitive ( 506 , 40 , 0 ) ; 
  primitive ( 507 , 41 , 0 ) ; 
  primitive ( 508 , 61 , 0 ) ; 
  primitive ( 509 , 16 , 0 ) ; 
  primitive ( 500 , 107 , 0 ) ; 
  primitive ( 510 , 15 , 0 ) ; 
  primitive ( 511 , 92 , 0 ) ; 
  primitive ( 501 , 67 , 0 ) ; 
  primitive ( 512 , 62 , 0 ) ; 
  hash [ 10016 ] .v.RH = 512 ; 
  eqtb [ 10016 ] = eqtb [ curval ] ; 
  primitive ( 513 , 102 , 0 ) ; 
  primitive ( 514 , 88 , 0 ) ; 
  primitive ( 515 , 77 , 0 ) ; 
  primitive ( 516 , 32 , 0 ) ; 
  primitive ( 517 , 36 , 0 ) ; 
  primitive ( 518 , 39 , 0 ) ; 
  primitive ( 327 , 37 , 0 ) ; 
  primitive ( 348 , 18 , 0 ) ; 
  primitive ( 519 , 46 , 0 ) ; 
  primitive ( 520 , 17 , 0 ) ; 
  primitive ( 521 , 54 , 0 ) ; 
  primitive ( 522 , 91 , 0 ) ; 
  primitive ( 523 , 34 , 0 ) ; 
  primitive ( 524 , 65 , 0 ) ; 
  primitive ( 525 , 103 , 0 ) ; 
  primitive ( 332 , 55 , 0 ) ; 
  primitive ( 526 , 63 , 0 ) ; 
  primitive ( 404 , 84 , 0 ) ; 
  primitive ( 527 , 42 , 0 ) ; 
  primitive ( 528 , 80 , 0 ) ; 
  primitive ( 529 , 66 , 0 ) ; 
  primitive ( 530 , 96 , 0 ) ; 
  primitive ( 531 , 0 , 256 ) ; 
  hash [ 10021 ] .v.RH = 531 ; 
  eqtb [ 10021 ] = eqtb [ curval ] ; 
  primitive ( 532 , 98 , 0 ) ; 
  primitive ( 533 , 109 , 0 ) ; 
  primitive ( 403 , 71 , 0 ) ; 
  primitive ( 349 , 38 , 0 ) ; 
  primitive ( 534 , 33 , 0 ) ; 
  primitive ( 535 , 56 , 0 ) ; 
  primitive ( 536 , 35 , 0 ) ; 
  primitive ( 593 , 13 , 256 ) ; 
  parloc = curval ; 
  partoken = 4095 + parloc ; 
  primitive ( 625 , 104 , 0 ) ; 
  primitive ( 626 , 104 , 1 ) ; 
  primitive ( 627 , 110 , 0 ) ; 
  primitive ( 628 , 110 , 1 ) ; 
  primitive ( 629 , 110 , 2 ) ; 
  primitive ( 630 , 110 , 3 ) ; 
  primitive ( 631 , 110 , 4 ) ; 
  primitive ( 472 , 89 , 0 ) ; 
  primitive ( 496 , 89 , 1 ) ; 
  primitive ( 391 , 89 , 2 ) ; 
  primitive ( 392 , 89 , 3 ) ; 
  primitive ( 664 , 79 , 102 ) ; 
  primitive ( 665 , 79 , 1 ) ; 
  primitive ( 666 , 82 , 0 ) ; 
  primitive ( 667 , 82 , 1 ) ; 
  primitive ( 668 , 83 , 1 ) ; 
  primitive ( 669 , 83 , 3 ) ; 
  primitive ( 670 , 83 , 2 ) ; 
  primitive ( 671 , 70 , 0 ) ; 
  primitive ( 672 , 70 , 1 ) ; 
  primitive ( 673 , 70 , 2 ) ; 
  primitive ( 674 , 70 , 3 ) ; 
  primitive ( 675 , 70 , 4 ) ; 
  primitive ( 731 , 108 , 0 ) ; 
  primitive ( 732 , 108 , 1 ) ; 
  primitive ( 733 , 108 , 2 ) ; 
  primitive ( 734 , 108 , 3 ) ; 
  primitive ( 735 , 108 , 4 ) ; 
  primitive ( 736 , 108 , 5 ) ; 
  primitive ( 752 , 105 , 0 ) ; 
  primitive ( 753 , 105 , 1 ) ; 
  primitive ( 754 , 105 , 2 ) ; 
  primitive ( 755 , 105 , 3 ) ; 
  primitive ( 756 , 105 , 4 ) ; 
  primitive ( 757 , 105 , 5 ) ; 
  primitive ( 758 , 105 , 6 ) ; 
  primitive ( 759 , 105 , 7 ) ; 
  primitive ( 760 , 105 , 8 ) ; 
  primitive ( 761 , 105 , 9 ) ; 
  primitive ( 762 , 105 , 10 ) ; 
  primitive ( 763 , 105 , 11 ) ; 
  primitive ( 764 , 105 , 12 ) ; 
  primitive ( 765 , 105 , 13 ) ; 
  primitive ( 766 , 105 , 14 ) ; 
  primitive ( 767 , 105 , 15 ) ; 
  primitive ( 768 , 105 , 16 ) ; 
  primitive ( 769 , 106 , 2 ) ; 
  hash [ 10018 ] .v.RH = 769 ; 
  eqtb [ 10018 ] = eqtb [ curval ] ; 
  primitive ( 770 , 106 , 4 ) ; 
  primitive ( 771 , 106 , 3 ) ; 
  primitive ( 794 , 87 , 0 ) ; 
  hash [ 10024 ] .v.RH = 794 ; 
  eqtb [ 10024 ] = eqtb [ curval ] ; 
  primitive ( 891 , 4 , 256 ) ; 
  primitive ( 892 , 5 , 257 ) ; 
  hash [ 10015 ] .v.RH = 892 ; 
  eqtb [ 10015 ] = eqtb [ curval ] ; 
  primitive ( 893 , 5 , 258 ) ; 
  hash [ 10019 ] .v.RH = 894 ; 
  hash [ 10020 ] .v.RH = 894 ; 
  eqtb [ 10020 ] .hh.b0 = 9 ; 
  eqtb [ 10020 ] .hh .v.RH = memtop - 11 ; 
  eqtb [ 10020 ] .hh.b1 = 1 ; 
  eqtb [ 10019 ] = eqtb [ 10020 ] ; 
  eqtb [ 10019 ] .hh.b0 = 115 ; 
  primitive ( 963 , 81 , 0 ) ; 
  primitive ( 964 , 81 , 1 ) ; 
  primitive ( 965 , 81 , 2 ) ; 
  primitive ( 966 , 81 , 3 ) ; 
  primitive ( 967 , 81 , 4 ) ; 
  primitive ( 968 , 81 , 5 ) ; 
  primitive ( 969 , 81 , 6 ) ; 
  primitive ( 970 , 81 , 7 ) ; 
  primitive ( 1018 , 14 , 0 ) ; 
  primitive ( 1019 , 14 , 1 ) ; 
  primitive ( 1020 , 26 , 4 ) ; 
  primitive ( 1021 , 26 , 0 ) ; 
  primitive ( 1022 , 26 , 1 ) ; 
  primitive ( 1023 , 26 , 2 ) ; 
  primitive ( 1024 , 26 , 3 ) ; 
  primitive ( 1025 , 27 , 4 ) ; 
  primitive ( 1026 , 27 , 0 ) ; 
  primitive ( 1027 , 27 , 1 ) ; 
  primitive ( 1028 , 27 , 2 ) ; 
  primitive ( 1029 , 27 , 3 ) ; 
  primitive ( 333 , 28 , 5 ) ; 
  primitive ( 337 , 29 , 1 ) ; 
  primitive ( 339 , 30 , 99 ) ; 
  primitive ( 1047 , 21 , 1 ) ; 
  primitive ( 1048 , 21 , 0 ) ; 
  primitive ( 1049 , 22 , 1 ) ; 
  primitive ( 1050 , 22 , 0 ) ; 
  primitive ( 405 , 20 , 0 ) ; 
  primitive ( 1051 , 20 , 1 ) ; 
  primitive ( 1052 , 20 , 2 ) ; 
  primitive ( 958 , 20 , 3 ) ; 
  primitive ( 1053 , 20 , 4 ) ; 
  primitive ( 960 , 20 , 5 ) ; 
  primitive ( 1054 , 20 , 106 ) ; 
  primitive ( 1055 , 31 , 99 ) ; 
  primitive ( 1056 , 31 , 100 ) ; 
  primitive ( 1057 , 31 , 101 ) ; 
  primitive ( 1058 , 31 , 102 ) ; 
  primitive ( 1073 , 43 , 1 ) ; 
  primitive ( 1074 , 43 , 0 ) ; 
  primitive ( 1083 , 25 , 12 ) ; 
  primitive ( 1084 , 25 , 11 ) ; 
  primitive ( 1085 , 25 , 10 ) ; 
  primitive ( 1086 , 23 , 0 ) ; 
  primitive ( 1087 , 23 , 1 ) ; 
  primitive ( 1088 , 24 , 0 ) ; 
  primitive ( 1089 , 24 , 1 ) ; 
  primitive ( 45 , 47 , 1 ) ; 
  primitive ( 346 , 47 , 0 ) ; 
  primitive ( 1120 , 48 , 0 ) ; 
  primitive ( 1121 , 48 , 1 ) ; 
  primitive ( 859 , 50 , 16 ) ; 
  primitive ( 860 , 50 , 17 ) ; 
  primitive ( 861 , 50 , 18 ) ; 
  primitive ( 862 , 50 , 19 ) ; 
  primitive ( 863 , 50 , 20 ) ; 
  primitive ( 864 , 50 , 21 ) ; 
  primitive ( 865 , 50 , 22 ) ; 
  primitive ( 866 , 50 , 23 ) ; 
  primitive ( 868 , 50 , 26 ) ; 
  primitive ( 867 , 50 , 27 ) ; 
  primitive ( 1122 , 51 , 0 ) ; 
  primitive ( 871 , 51 , 1 ) ; 
  primitive ( 872 , 51 , 2 ) ; 
  primitive ( 854 , 53 , 0 ) ; 
  primitive ( 855 , 53 , 2 ) ; 
  primitive ( 856 , 53 , 4 ) ; 
  primitive ( 857 , 53 , 6 ) ; 
  primitive ( 1140 , 52 , 0 ) ; 
  primitive ( 1141 , 52 , 1 ) ; 
  primitive ( 1142 , 52 , 2 ) ; 
  primitive ( 1143 , 52 , 3 ) ; 
  primitive ( 1144 , 52 , 4 ) ; 
  primitive ( 1145 , 52 , 5 ) ; 
  primitive ( 869 , 49 , 30 ) ; 
  primitive ( 870 , 49 , 31 ) ; 
  hash [ 10017 ] .v.RH = 870 ; 
  eqtb [ 10017 ] = eqtb [ curval ] ; 
  primitive ( 1164 , 93 , 1 ) ; 
  primitive ( 1165 , 93 , 2 ) ; 
  primitive ( 1166 , 93 , 4 ) ; 
  primitive ( 1167 , 97 , 0 ) ; 
  primitive ( 1168 , 97 , 1 ) ; 
  primitive ( 1169 , 97 , 2 ) ; 
  primitive ( 1170 , 97 , 3 ) ; 
  primitive ( 1184 , 94 , 0 ) ; 
  primitive ( 1185 , 94 , 1 ) ; 
  primitive ( 1186 , 95 , 0 ) ; 
  primitive ( 1187 , 95 , 1 ) ; 
  primitive ( 1188 , 95 , 2 ) ; 
  primitive ( 1189 , 95 , 3 ) ; 
  primitive ( 1190 , 95 , 4 ) ; 
  primitive ( 1191 , 95 , 5 ) ; 
  primitive ( 1192 , 95 , 6 ) ; 
  primitive ( 411 , 85 , 11383 ) ; 
  primitive ( 415 , 85 , 12407 ) ; 
  primitive ( 412 , 85 , 11639 ) ; 
  primitive ( 413 , 85 , 11895 ) ; 
  primitive ( 414 , 85 , 12151 ) ; 
  primitive ( 473 , 85 , 12974 ) ; 
  primitive ( 408 , 86 , 11335 ) ; 
  primitive ( 409 , 86 , 11351 ) ; 
  primitive ( 410 , 86 , 11367 ) ; 
  primitive ( 934 , 99 , 0 ) ; 
  primitive ( 946 , 99 , 1 ) ; 
  primitive ( 1210 , 78 , 0 ) ; 
  primitive ( 1211 , 78 , 1 ) ; 
  primitive ( 272 , 100 , 0 ) ; 
  primitive ( 273 , 100 , 1 ) ; 
  primitive ( 274 , 100 , 2 ) ; 
  primitive ( 1220 , 100 , 3 ) ; 
  primitive ( 1221 , 60 , 1 ) ; 
  primitive ( 1222 , 60 , 0 ) ; 
  primitive ( 1223 , 58 , 0 ) ; 
  primitive ( 1224 , 58 , 1 ) ; 
  primitive ( 1230 , 57 , 11639 ) ; 
  primitive ( 1231 , 57 , 11895 ) ; 
  primitive ( 1232 , 19 , 0 ) ; 
  primitive ( 1233 , 19 , 1 ) ; 
  primitive ( 1234 , 19 , 2 ) ; 
  primitive ( 1235 , 19 , 3 ) ; 
  primitive ( 1278 , 59 , 0 ) ; 
  primitive ( 590 , 59 , 1 ) ; 
  writeloc = curval ; 
  primitive ( 1279 , 59 , 2 ) ; 
  primitive ( 1280 , 59 , 3 ) ; 
  primitive ( 1281 , 59 , 4 ) ; 
  primitive ( 1282 , 59 , 5 ) ; 
  nonewcontrolsequence = true ; 
} 
#endif /* INITEX */
void texbody ( ) 
{texbody_regmem 
  history = 3 ; 
  setpaths ( TEXFORMATPATHBIT + TEXINPUTPATHBIT + TEXPOOLPATHBIT + 
  TFMFILEPATHBIT ) ; 
  if ( readyalready == 314159L ) 
  goto lab1 ; 
  bad = 0 ; 
  if ( ( halferrorline < 30 ) || ( halferrorline > errorline - 15 ) ) 
  bad = 1 ; 
  if ( maxprintline < 60 ) 
  bad = 2 ; 
  if ( dvibufsize % 8 != 0 ) 
  bad = 3 ; 
  if ( 1100 > memtop ) 
  bad = 4 ; 
  if ( 7919 > 9500 ) 
  bad = 5 ; 
  if ( maxinopen >= 128 ) 
  bad = 6 ; 
  if ( memtop < 267 ) 
  bad = 7 ; 
	;
#ifdef INITEX
  if ( ( memmin != 0 ) || ( memmax != memtop ) ) 
  bad = 10 ; 
#endif /* INITEX */
  if ( ( memmin > 0 ) || ( memmax < memtop ) ) 
  bad = 10 ; 
  if ( ( 0 > 0 ) || ( 255 < 127 ) ) 
  bad = 11 ; 
  if ( ( 0 > 0 ) || ( 262143L < 32767 ) ) 
  bad = 12 ; 
  if ( ( 0 < 0 ) || ( 255 > 262143L ) ) 
  bad = 13 ; 
  if ( ( memmin < 0 ) || ( memmax >= 262143L ) || ( -0 - memmin > 262144L ) ) 
  bad = 14 ; 
  if ( ( 0 < 0 ) || ( fontmax > 255 ) ) 
  bad = 15 ; 
  if ( fontmax > 256 ) 
  bad = 16 ; 
  if ( ( savesize > 262143L ) || ( maxstrings > 262143L ) ) 
  bad = 17 ; 
  if ( bufsize > 262143L ) 
  bad = 18 ; 
  if ( 255 < 255 ) 
  bad = 19 ; 
  if ( 14376 > 262143L ) 
  bad = 21 ; 
  if ( formatdefaultlength > PATHMAX ) 
  bad = 31 ; 
  if ( 2 * 262143L < memtop - memmin ) 
  bad = 41 ; 
  if ( bad > 0 ) 
  {
    (void) fprintf( stdout , "%s%s%ld\n",  "Ouch---my internal constants have been clobbered!" ,     "---case " , (long)bad ) ; 
    goto lab9999 ; 
  } 
  initialize () ; 
	;
#ifdef INITEX
  if ( ! getstringsstarted () ) 
  goto lab9999 ; 
  initprim () ; 
  initstrptr = strptr ; 
  initpoolptr = poolptr ; 
  dateandtime ( eqtb [ 12683 ] .cint , eqtb [ 12684 ] .cint , eqtb [ 12685 ] 
  .cint , eqtb [ 12686 ] .cint ) ; 
#endif /* INITEX */
  readyalready = 314159L ; 
  lab1: selector = 17 ; 
  tally = 0 ; 
  termoffset = 0 ; 
  fileoffset = 0 ; 
  (void) Fputs( stdout ,  "This is TeX, C Version 3.141" ) ; 
  if ( formatident > 0 ) 
  slowprint ( formatident ) ; 
  println () ; 
  flush ( stdout ) ; 
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
	  buffer [ first ] = 0 ; 
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
    if ( ( formatident == 0 ) || ( buffer [ curinput .locfield ] == 38 ) ) 
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
      while ( ( curinput .locfield < curinput .limitfield ) && ( buffer [ 
      curinput .locfield ] == 32 ) ) incr ( curinput .locfield ) ; 
    } 
    if ( ( eqtb [ 12711 ] .cint < 0 ) || ( eqtb [ 12711 ] .cint > 255 ) ) 
    decr ( curinput .limitfield ) ; 
    else buffer [ curinput .limitfield ] = eqtb [ 12711 ] .cint ; 
    dateandtime ( eqtb [ 12683 ] .cint , eqtb [ 12684 ] .cint , eqtb [ 12685 ] 
    .cint , eqtb [ 12686 ] .cint ) ; 
    magicoffset = strstart [ 885 ] - 9 * 16 ; 
    if ( interaction == 0 ) 
    selector = 16 ; 
    else selector = 17 ; 
    if ( ( curinput .locfield < curinput .limitfield ) && ( eqtb [ 11383 + 
    buffer [ curinput .locfield ] ] .hh .v.RH != 0 ) ) 
    startinput () ; 
  } 
  history = 0 ; 
  maincontrol () ; 
  finalcleanup () ; 
  closefilesandterminate () ; 
  lab9999: {
      
    flush ( stdout ) ; 
    readyalready = 0 ; 
    if ( ( history != 0 ) && ( history != 1 ) ) 
    uexit ( 1 ) ; 
    else uexit ( 0 ) ; 
  } 
} 
