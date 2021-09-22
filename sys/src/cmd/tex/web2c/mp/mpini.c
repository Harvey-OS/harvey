#define EXTERN extern
#include "mpd.h"

#ifdef INIMP
boolean 
#ifdef HAVE_PROTOTYPES
getstringsstarted ( void ) 
#else
getstringsstarted ( ) 
#endif
{
  /* 30 10 */ register boolean Result; unsigned char k, l  ;
  ASCIIcode m, n  ;
  strnumber g  ;
  integer a  ;
  boolean c  ;
  poolptr = 0 ;
  strptr = 0 ;
  maxpoolptr = 0 ;
  maxstrptr = 0 ;
  strstart [0 ]= 0 ;
  nextstr [0 ]= 1 ;
  stroverflowed = false ;
	;
#ifdef STAT
  poolinuse = 0 ;
  strsinuse = 0 ;
  maxplused = 0 ;
  maxstrsused = 0 ;
  pactcount = 0 ;
  pactchars = 0 ;
  pactstrs = 0 ;
#endif /* STAT */
  strsusedup = 0 ;
  {register integer for_end; k = 0 ;for_end = 255 ; if ( k <= for_end) do 
    {
      {
	strpool [poolptr ]= k ;
	incr ( poolptr ) ;
      } 
      g = makestring () ;
      strref [g ]= 127 ;
    } 
  while ( k++ < for_end ) ;} 
  namelength = strlen ( poolname ) ;
  nameoffile = xmalloc ( 1 + namelength + 1 ) ;
  strcpy ( (char*) nameoffile + 1 , poolname ) ;
  if ( aopenin ( poolfile , kpsemppoolformat ) ) 
  {
    c = false ;
    do {
	{ 
	if ( eof ( poolfile ) ) 
	{
	  ;
	  fprintf( stdout , "%s\n",  "! mp.pool has no check sum." ) ;
	  aclose ( poolfile ) ;
	  Result = false ;
	  goto lab10 ;
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
	      fprintf( stdout , "%s\n",                "! mp.pool check sum doesn't have nine digits." ) ;
	      aclose ( poolfile ) ;
	      Result = false ;
	      goto lab10 ;
	    } 
	    a = 10 * a + xord [n ]- 48 ;
	    if ( k == 9 ) 
	    goto lab30 ;
	    incr ( k ) ;
	    read ( poolfile , n ) ;
	  } 
	  lab30: if ( a != 136687108L ) 
	  {
	    ;
	    fprintf( stdout , "%s\n",              "! mp.pool doesn't match; tangle me again (or fix the path)." ) ;
	    aclose ( poolfile ) ;
	    Result = false ;
	    goto lab10 ;
	  } 
	  c = true ;
	} 
	else {
	    
	  if ( ( xord [m ]< 48 ) || ( xord [m ]> 57 ) || ( xord [n ]< 48 
	  ) || ( xord [n ]> 57 ) ) 
	  {
	    ;
	    fprintf( stdout , "%s\n",  "! mp.pool line doesn't begin with two digits."             ) ;
	    aclose ( poolfile ) ;
	    Result = false ;
	    goto lab10 ;
	  } 
	  l = xord [m ]* 10 + xord [n ]- 48 * 11 ;
	  if ( poolptr + l + stringvacancies > poolsize ) 
	  {
	    ;
	    fprintf( stdout , "%s\n",  "! You have to increase POOLSIZE." ) ;
	    aclose ( poolfile ) ;
	    Result = false ;
	    goto lab10 ;
	  } 
	  if ( strptr + stringsvacant >= maxstrings ) 
	  {
	    ;
	    fprintf( stdout , "%s\n",  "! You have to increase MAXSTRINGS." ) ;
	    aclose ( poolfile ) ;
	    Result = false ;
	    goto lab10 ;
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
	  strref [g ]= 127 ;
	} 
      } 
    } while ( ! ( c ) ) ;
    aclose ( poolfile ) ;
    Result = true ;
  } 
  else {
      
    ;
    fprintf( stdout , "%s\n",  "! I can't read mp.pool; bad path?" ) ;
    aclose ( poolfile ) ;
    Result = false ;
    goto lab10 ;
  } 
  lastfixedstr = strptr - 1 ;
  fixedstruse = strptr ;
  lab10: ;
  return Result ;
} 
#endif /* INIMP */
#ifdef INIMP
void 
#ifdef HAVE_PROTOTYPES
sortavail ( void ) 
#else
sortavail ( ) 
#endif
{
  halfword p, q, r  ;
  halfword oldrover  ;
  p = getnode ( 1073741824L ) ;
  p = mem [rover + 1 ].hhfield .v.RH ;
  mem [rover + 1 ].hhfield .v.RH = 268435455L ;
  oldrover = rover ;
  while ( p != oldrover ) if ( p < rover ) 
  {
    q = p ;
    p = mem [q + 1 ].hhfield .v.RH ;
    mem [q + 1 ].hhfield .v.RH = rover ;
    rover = q ;
  } 
  else {
      
    q = rover ;
    while ( mem [q + 1 ].hhfield .v.RH < p ) q = mem [q + 1 ].hhfield 
    .v.RH ;
    r = mem [p + 1 ].hhfield .v.RH ;
    mem [p + 1 ].hhfield .v.RH = mem [q + 1 ].hhfield .v.RH ;
    mem [q + 1 ].hhfield .v.RH = p ;
    p = r ;
  } 
  p = rover ;
  while ( mem [p + 1 ].hhfield .v.RH != 268435455L ) {
      
    mem [mem [p + 1 ].hhfield .v.RH + 1 ].hhfield .lhfield = p ;
    p = mem [p + 1 ].hhfield .v.RH ;
  } 
  mem [p + 1 ].hhfield .v.RH = rover ;
  mem [rover + 1 ].hhfield .lhfield = p ;
} 
#endif /* INIMP */
#ifdef INIMP
void 
#ifdef HAVE_PROTOTYPES
zprimitive ( strnumber s , halfword c , halfword o ) 
#else
zprimitive ( s , c , o ) 
  strnumber s ;
  halfword c ;
  halfword o ;
#endif
{
  poolpointer k  ;
  smallnumber j  ;
  smallnumber l  ;
  k = strstart [s ];
  l = strstart [nextstr [s ]]- k ;
  {register integer for_end; j = 0 ;for_end = l - 1 ; if ( j <= for_end) do 
    buffer [j ]= strpool [k + j ];
  while ( j++ < for_end ) ;} 
  cursym = idlookup ( 0 , l ) ;
  if ( s >= 256 ) 
  {
    flushstring ( hash [cursym ].v.RH ) ;
    hash [cursym ].v.RH = s ;
  } 
  eqtb [cursym ].lhfield = c ;
  eqtb [cursym ].v.RH = o ;
} 
#endif /* INIMP */
void 
#ifdef HAVE_PROTOTYPES
startinput ( void ) 
#else
startinput ( ) 
#endif
{
  /* 30 */ integer j  ;
  while ( ( curinput .indexfield > 15 ) && ( curinput .locfield == 0 ) ) 
  endtokenlist () ;
  if ( ( curinput .indexfield > 15 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 799 ) ;
    } 
    {
      helpptr = 3 ;
      helpline [2 ]= 800 ;
      helpline [1 ]= 801 ;
      helpline [0 ]= 802 ;
    } 
    error () ;
  } 
  if ( ( curinput .indexfield <= 15 ) ) 
  scanfilename () ;
  else {
      
    curname = 284 ;
    curext = 284 ;
    curarea = 284 ;
  } 
  while ( true ) {
      
    beginfilereading () ;
    if ( tryextension ( 798 ) ) 
    goto lab30 ;
    else if ( tryextension ( 797 ) ) 
    goto lab30 ;
    else ;
    endfilereading () ;
    promptfilename ( 785 , 284 ) ;
  } 
  lab30: curinput .namefield = amakenamestring ( inputfile [curinput 
  .indexfield ]) ;
  if ( inamestack [curinput .indexfield ]== curname ) 
  {
    if ( strref [curname ]< 127 ) 
    incr ( strref [curname ]) ;
  } 
  if ( iareastack [curinput .indexfield ]== curarea ) 
  {
    if ( strref [curarea ]< 127 ) 
    incr ( strref [curarea ]) ;
  } 
  if ( jobname == 0 ) 
  {
    j = 1 ;
    beginname () ;
    while ( ( j <= namelength ) && ( morename ( (char*) nameoffile [j ]) ) ) incr ( 
    j ) ;
    endname () ;
    jobname = curname ;
    strref [jobname ]= 127 ;
	;
#ifdef INIMP
    if ( iniversion && dumpoption ) 
    {
      {
	if ( poolptr + memdefaultlength > maxpoolptr ) 
	if ( poolptr + memdefaultlength > poolsize ) 
	docompaction ( memdefaultlength ) ;
	else maxpoolptr = poolptr + memdefaultlength ;
      } 
      {register integer for_end; j = 1 ;for_end = memdefaultlength - 4 
      ; if ( j <= for_end) do 
	{
	  strpool [poolptr ]= xord [MPmemdefault [j ]];
	  incr ( poolptr ) ;
	} 
      while ( j++ < for_end ) ;} 
      jobname = makestring () ;
      strref [jobname ]= 127 ;
    } 
#endif /* INIMP */
    openlogfile () ;
  } 
  if ( termoffset + ( strstart [nextstr [curinput .namefield ]]- strstart 
  [curinput .namefield ]) > maxprintline - 2 ) 
  println () ;
  else if ( ( termoffset > 0 ) || ( fileoffset > 0 ) ) 
  printchar ( 32 ) ;
  printchar ( 40 ) ;
  incr ( openparens ) ;
  print ( curinput .namefield ) ;
  fflush ( stdout ) ;
  {
    linestack [curinput .indexfield ]= 1 ;
    if ( inputln ( inputfile [curinput .indexfield ], false ) ) 
    ;
    firmuptheline () ;
    buffer [curinput .limitfield ]= 37 ;
    first = curinput .limitfield + 1 ;
    curinput .locfield = curinput .startfield ;
  } 
} 
#ifdef INIMP
void 
#ifdef HAVE_PROTOTYPES
storememfile ( void ) 
#else
storememfile ( ) 
#endif
{
  /* 30 */ integer k  ;
  halfword p, q  ;
  integer x  ;
  fourquarters w  ;
  strnumber s  ;
  selector = 4 ;
  print ( 1178 ) ;
  print ( jobname ) ;
  printchar ( 32 ) ;
  printint ( roundunscaled ( internal [13 ]) ) ;
  printchar ( 46 ) ;
  printint ( roundunscaled ( internal [14 ]) ) ;
  printchar ( 46 ) ;
  printint ( roundunscaled ( internal [15 ]) ) ;
  printchar ( 41 ) ;
  if ( interaction == 0 ) 
  selector = 9 ;
  else selector = 10 ;
  {
    if ( poolptr + 1 > maxpoolptr ) 
    if ( poolptr + 1 > poolsize ) 
    docompaction ( 1 ) ;
    else maxpoolptr = poolptr + 1 ;
  } 
  memident = makestring () ;
  strref [memident ]= 127 ;
  packjobname ( 784 ) ;
  while ( ! wopenout ( memfile ) ) promptfilename ( 1179 , 784 ) ;
  printnl ( 1180 ) ;
  s = wmakenamestring ( memfile ) ;
  print ( s ) ;
  flushstring ( s ) ;
  printnl ( memident ) ;
  dumpint ( 136687108L ) ;
  dumpint ( 0 ) ;
  dumpint ( memtop ) ;
  dumpint ( 9500 ) ;
  dumpint ( 7919 ) ;
  dumpint ( 15 ) ;
  docompaction ( poolsize ) ;
  dumpint ( poolptr ) ;
  dumpint ( maxstrptr ) ;
  dumpint ( strptr ) ;
  k = 0 ;
  while ( ( nextstr [k ]== k + 1 ) && ( k <= maxstrptr ) ) incr ( k ) ;
  dumpint ( k ) ;
  while ( k <= maxstrptr ) {
      
    dumpint ( nextstr [k ]) ;
    incr ( k ) ;
  } 
  k = 0 ;
  while ( true ) {
      
    dumpint ( strstart [k ]) ;
    if ( k == strptr ) 
    goto lab30 ;
    else k = nextstr [k ];
  } 
  lab30: k = 0 ;
  while ( k + 4 < poolptr ) {
      
    w .b0 = strpool [k ];
    w .b1 = strpool [k + 1 ];
    w .b2 = strpool [k + 2 ];
    w .b3 = strpool [k + 3 ];
    dumpqqqq ( w ) ;
    k = k + 4 ;
  } 
  k = poolptr - 4 ;
  w .b0 = strpool [k ];
  w .b1 = strpool [k + 1 ];
  w .b2 = strpool [k + 2 ];
  w .b3 = strpool [k + 3 ];
  dumpqqqq ( w ) ;
  println () ;
  print ( 1174 ) ;
  printint ( maxstrptr ) ;
  print ( 1175 ) ;
  printint ( poolptr ) ;
  sortavail () ;
  varused = 0 ;
  dumpint ( lomemmax ) ;
  dumpint ( rover ) ;
  p = 0 ;
  q = rover ;
  x = 0 ;
  do {
      { register integer for_end; k = p ;for_end = q + 1 ; if ( k <= 
    for_end) do 
      dumpwd ( mem [k ]) ;
    while ( k++ < for_end ) ;} 
    x = x + q + 2 - p ;
    varused = varused + q - p ;
    p = q + mem [q ].hhfield .lhfield ;
    q = mem [q + 1 ].hhfield .v.RH ;
  } while ( ! ( q == rover ) ) ;
  varused = varused + lomemmax - p ;
  dynused = memend + 1 - himemmin ;
  {register integer for_end; k = p ;for_end = lomemmax ; if ( k <= for_end) 
  do 
    dumpwd ( mem [k ]) ;
  while ( k++ < for_end ) ;} 
  x = x + lomemmax + 1 - p ;
  dumpint ( himemmin ) ;
  dumpint ( avail ) ;
  {register integer for_end; k = himemmin ;for_end = memend ; if ( k <= 
  for_end) do 
    dumpwd ( mem [k ]) ;
  while ( k++ < for_end ) ;} 
  x = x + memend + 1 - himemmin ;
  p = avail ;
  while ( p != 0 ) {
      
    decr ( dynused ) ;
    p = mem [p ].hhfield .v.RH ;
  } 
  dumpint ( varused ) ;
  dumpint ( dynused ) ;
  println () ;
  printint ( x ) ;
  print ( 1176 ) ;
  printint ( varused ) ;
  printchar ( 38 ) ;
  printint ( dynused ) ;
  dumpint ( hashused ) ;
  stcount = 9756 - hashused ;
  {register integer for_end; p = 1 ;for_end = hashused ; if ( p <= for_end) 
  do 
    if ( hash [p ].v.RH != 0 ) 
    {
      dumpint ( p ) ;
      dumphh ( hash [p ]) ;
      dumphh ( eqtb [p ]) ;
      incr ( stcount ) ;
    } 
  while ( p++ < for_end ) ;} 
  {register integer for_end; p = hashused + 1 ;for_end = 9771 ; if ( p <= 
  for_end) do 
    {
      dumphh ( hash [p ]) ;
      dumphh ( eqtb [p ]) ;
    } 
  while ( p++ < for_end ) ;} 
  dumpint ( stcount ) ;
  println () ;
  printint ( stcount ) ;
  print ( 1177 ) ;
  dumpint ( intptr ) ;
  {register integer for_end; k = 1 ;for_end = intptr ; if ( k <= for_end) do 
    {
      dumpint ( internal [k ]) ;
      dumpint ( intname [k ]) ;
    } 
  while ( k++ < for_end ) ;} 
  dumpint ( startsym ) ;
  dumpint ( interaction ) ;
  dumpint ( memident ) ;
  dumpint ( bgloc ) ;
  dumpint ( egloc ) ;
  dumpint ( serialno ) ;
  dumpint ( 69073L ) ;
  internal [10 ]= 0 ;
  wclose ( memfile ) ;
} 
#endif /* INIMP */
boolean 
#ifdef HAVE_PROTOTYPES
loadmemfile ( void ) 
#else
loadmemfile ( ) 
#endif
{
  /* 30 6666 10 */ register boolean Result; integer k  ;
  halfword p, q  ;
  integer x  ;
  strnumber s  ;
  fourquarters w  ;
  undumpint ( x ) ;
  if ( x != 136687108L ) 
  goto lab6666 ;
  undumpint ( x ) ;
  if ( x != 0 ) 
  goto lab6666 ;
	;
#ifdef INIMP
  if ( iniversion ) 
  {
    libcfree ( mem ) ;
    libcfree ( strref ) ;
    libcfree ( nextstr ) ;
    libcfree ( strstart ) ;
    libcfree ( strpool ) ;
  } 
#endif /* INIMP */
  undumpint ( memtop ) ;
  if ( memmax < memtop ) 
  memmax = memtop ;
  if ( 1100 > memtop ) 
  goto lab6666 ;
  xmallocarray ( mem , memmax - 0 ) ;
  undumpint ( x ) ;
  if ( x != 9500 ) 
  goto lab6666 ;
  undumpint ( x ) ;
  if ( x != 7919 ) 
  goto lab6666 ;
  undumpint ( x ) ;
  if ( x != 15 ) 
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
    else poolptr = x ;
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
      fprintf( stdout , "%s%s\n",  "---! Must increase the " , "max strings" ) ;
      goto lab6666 ;
    } 
    else maxstrptr = x ;
  } 
  xmallocarray ( strref , maxstrings ) ;
  xmallocarray ( nextstr , maxstrings ) ;
  xmallocarray ( strstart , maxstrings ) ;
  xmallocarray ( strpool , poolsize ) ;
  {
    undumpint ( x ) ;
    if ( ( x < 0 ) || ( x > maxstrptr ) ) 
    goto lab6666 ;
    else strptr = x ;
  } 
  {
    undumpint ( x ) ;
    if ( ( x < 0 ) || ( x > maxstrptr + 1 ) ) 
    goto lab6666 ;
    else s = x ;
  } 
  {register integer for_end; k = 0 ;for_end = s - 1 ; if ( k <= for_end) do 
    nextstr [k ]= k + 1 ;
  while ( k++ < for_end ) ;} 
  {register integer for_end; k = s ;for_end = maxstrptr ; if ( k <= for_end) 
  do 
    {
      undumpint ( x ) ;
      if ( ( x < s + 1 ) || ( x > maxstrptr + 1 ) ) 
      goto lab6666 ;
      else nextstr [k ]= x ;
    } 
  while ( k++ < for_end ) ;} 
  fixedstruse = 0 ;
  k = 0 ;
  while ( true ) {
      
    {
      undumpint ( x ) ;
      if ( ( x < 0 ) || ( x > poolptr ) ) 
      goto lab6666 ;
      else strstart [k ]= x ;
    } 
    if ( k == strptr ) 
    goto lab30 ;
    strref [k ]= 127 ;
    incr ( fixedstruse ) ;
    lastfixedstr = k ;
    k = nextstr [k ];
  } 
  lab30: k = 0 ;
  while ( k + 4 < poolptr ) {
      
    undumpqqqq ( w ) ;
    strpool [k ]= w .b0 ;
    strpool [k + 1 ]= w .b1 ;
    strpool [k + 2 ]= w .b2 ;
    strpool [k + 3 ]= w .b3 ;
    k = k + 4 ;
  } 
  k = poolptr - 4 ;
  undumpqqqq ( w ) ;
  strpool [k ]= w .b0 ;
  strpool [k + 1 ]= w .b1 ;
  strpool [k + 2 ]= w .b2 ;
  strpool [k + 3 ]= w .b3 ;
  initstruse = fixedstruse ;
  initpoolptr = poolptr ;
  maxpoolptr = poolptr ;
  strsusedup = fixedstruse ;
	;
#ifdef STAT
  poolinuse = strstart [strptr ];
  strsinuse = fixedstruse ;
  maxplused = poolinuse ;
  maxstrsused = strsinuse ;
  pactcount = 0 ;
  pactchars = 0 ;
  pactstrs = 0 ;
#endif /* STAT */
  {
    undumpint ( x ) ;
    if ( ( x < 1023 ) || ( x > memtop - 4 ) ) 
    goto lab6666 ;
    else lomemmax = x ;
  } 
  {
    undumpint ( x ) ;
    if ( ( x < 24 ) || ( x > lomemmax ) ) 
    goto lab6666 ;
    else rover = x ;
  } 
  p = 0 ;
  q = rover ;
  do {
      { register integer for_end; k = p ;for_end = q + 1 ; if ( k <= 
    for_end) do 
      undumpwd ( mem [k ]) ;
    while ( k++ < for_end ) ;} 
    p = q + mem [q ].hhfield .lhfield ;
    if ( ( p > lomemmax ) || ( ( q >= mem [q + 1 ].hhfield .v.RH ) && ( mem 
    [q + 1 ].hhfield .v.RH != rover ) ) ) 
    goto lab6666 ;
    q = mem [q + 1 ].hhfield .v.RH ;
  } while ( ! ( q == rover ) ) ;
  {register integer for_end; k = p ;for_end = lomemmax ; if ( k <= for_end) 
  do 
    undumpwd ( mem [k ]) ;
  while ( k++ < for_end ) ;} 
  {
    undumpint ( x ) ;
    if ( ( x < lomemmax + 1 ) || ( x > memtop - 3 ) ) 
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
  {register integer for_end; k = himemmin ;for_end = memend ; if ( k <= 
  for_end) do 
    undumpwd ( mem [k ]) ;
  while ( k++ < for_end ) ;} 
  undumpint ( varused ) ;
  undumpint ( dynused ) ;
  {
    undumpint ( x ) ;
    if ( ( x < 1 ) || ( x > 9757 ) ) 
    goto lab6666 ;
    else hashused = x ;
  } 
  p = 0 ;
  do {
      { 
      undumpint ( x ) ;
      if ( ( x < p + 1 ) || ( x > hashused ) ) 
      goto lab6666 ;
      else p = x ;
    } 
    undumphh ( hash [p ]) ;
    undumphh ( eqtb [p ]) ;
  } while ( ! ( p == hashused ) ) ;
  {register integer for_end; p = hashused + 1 ;for_end = 9771 ; if ( p <= 
  for_end) do 
    {
      undumphh ( hash [p ]) ;
      undumphh ( eqtb [p ]) ;
    } 
  while ( p++ < for_end ) ;} 
  undumpint ( stcount ) ;
  {
    undumpint ( x ) ;
    if ( ( x < 33 ) || ( x > maxinternal ) ) 
    goto lab6666 ;
    else intptr = x ;
  } 
  {register integer for_end; k = 1 ;for_end = intptr ; if ( k <= for_end) do 
    {
      undumpint ( internal [k ]) ;
      {
	undumpint ( x ) ;
	if ( ( x < 0 ) || ( x > strptr ) ) 
	goto lab6666 ;
	else intname [k ]= x ;
      } 
    } 
  while ( k++ < for_end ) ;} 
  {
    undumpint ( x ) ;
    if ( ( x < 0 ) || ( x > 9757 ) ) 
    goto lab6666 ;
    else startsym = x ;
  } 
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
    else memident = x ;
  } 
  {
    undumpint ( x ) ;
    if ( ( x < 1 ) || ( x > 9771 ) ) 
    goto lab6666 ;
    else bgloc = x ;
  } 
  {
    undumpint ( x ) ;
    if ( ( x < 1 ) || ( x > 9771 ) ) 
    goto lab6666 ;
    else egloc = x ;
  } 
  undumpint ( serialno ) ;
  undumpint ( x ) ;
  if ( ( x != 69073L ) || feof ( memfile ) ) 
  goto lab6666 ;
  Result = true ;
  goto lab10 ;
  lab6666: ;
  fprintf( stdout , "%s\n",  "(Fatal mem file error; I'm stymied)" ) ;
  Result = false ;
  lab10: ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
finalcleanup ( void ) 
#else
finalcleanup ( ) 
#endif
{
  /* 10 */ smallnumber c  ;
  c = curmod ;
  if ( jobname == 0 ) 
  openlogfile () ;
  while ( inputptr > 0 ) if ( ( curinput .indexfield > 15 ) ) 
  endtokenlist () ;
  else endfilereading () ;
  while ( loopptr != 0 ) stopiteration () ;
  while ( openparens > 0 ) {
      
    print ( 1182 ) ;
    decr ( openparens ) ;
  } 
  while ( condptr != 0 ) {
      
    printnl ( 1183 ) ;
    printcmdmod ( 5 , curif ) ;
    if ( ifline != 0 ) 
    {
      print ( 1184 ) ;
      printint ( ifline ) ;
    } 
    print ( 1185 ) ;
    ifline = mem [condptr + 1 ].cint ;
    curif = mem [condptr ].hhfield .b1 ;
    condptr = mem [condptr ].hhfield .v.RH ;
  } 
  if ( history != 0 ) 
  if ( ( ( history == 1 ) || ( interaction < 3 ) ) ) 
  if ( selector == 10 ) 
  {
    selector = 8 ;
    printnl ( 1186 ) ;
    selector = 10 ;
  } 
  if ( c == 1 ) 
  {
	;
#ifdef INIMP
    if ( iniversion ) 
    {
      storememfile () ;
      goto lab10 ;
    } 
#endif /* INIMP */
    printnl ( 1187 ) ;
    goto lab10 ;
  } 
  lab10: ;
} 
#ifdef INIMP
void 
#ifdef HAVE_PROTOTYPES
initprim ( void ) 
#else
initprim ( ) 
#endif
{
  primitive ( 430 , 42 , 1 ) ;
  primitive ( 431 , 42 , 2 ) ;
  primitive ( 432 , 42 , 3 ) ;
  primitive ( 433 , 42 , 4 ) ;
  primitive ( 434 , 42 , 5 ) ;
  primitive ( 435 , 42 , 6 ) ;
  primitive ( 436 , 42 , 7 ) ;
  primitive ( 437 , 42 , 8 ) ;
  primitive ( 438 , 42 , 9 ) ;
  primitive ( 439 , 42 , 10 ) ;
  primitive ( 440 , 42 , 11 ) ;
  primitive ( 441 , 42 , 12 ) ;
  primitive ( 442 , 42 , 13 ) ;
  primitive ( 443 , 42 , 14 ) ;
  primitive ( 444 , 42 , 15 ) ;
  primitive ( 445 , 42 , 16 ) ;
  primitive ( 446 , 42 , 17 ) ;
  primitive ( 447 , 42 , 18 ) ;
  primitive ( 448 , 42 , 19 ) ;
  primitive ( 449 , 42 , 20 ) ;
  primitive ( 450 , 42 , 21 ) ;
  primitive ( 451 , 42 , 22 ) ;
  primitive ( 452 , 42 , 23 ) ;
  primitive ( 453 , 42 , 24 ) ;
  primitive ( 454 , 42 , 25 ) ;
  primitive ( 455 , 42 , 26 ) ;
  primitive ( 456 , 42 , 27 ) ;
  primitive ( 457 , 42 , 28 ) ;
  primitive ( 458 , 42 , 29 ) ;
  primitive ( 459 , 42 , 30 ) ;
  primitive ( 460 , 42 , 31 ) ;
  primitive ( 461 , 42 , 32 ) ;
  primitive ( 462 , 42 , 33 ) ;
  primitive ( 321 , 49 , 0 ) ;
  primitive ( 91 , 65 , 0 ) ;
  eqtb [9760 ]= eqtb [cursym ];
  primitive ( 93 , 66 , 0 ) ;
  primitive ( 125 , 67 , 0 ) ;
  primitive ( 123 , 48 , 0 ) ;
  primitive ( 58 , 80 , 0 ) ;
  eqtb [9762 ]= eqtb [cursym ];
  primitive ( 474 , 79 , 0 ) ;
  primitive ( 475 , 78 , 0 ) ;
  primitive ( 476 , 76 , 0 ) ;
  primitive ( 44 , 81 , 0 ) ;
  primitive ( 59 , 82 , 0 ) ;
  eqtb [9763 ]= eqtb [cursym ];
  primitive ( 92 , 10 , 0 ) ;
  primitive ( 477 , 20 , 0 ) ;
  primitive ( 478 , 61 , 0 ) ;
  primitive ( 479 , 34 , 0 ) ;
  bgloc = cursym ;
  primitive ( 480 , 59 , 0 ) ;
  primitive ( 481 , 62 , 0 ) ;
  primitive ( 482 , 29 , 0 ) ;
  primitive ( 468 , 83 , 0 ) ;
  eqtb [9767 ]= eqtb [cursym ];
  egloc = cursym ;
  primitive ( 483 , 28 , 0 ) ;
  primitive ( 484 , 9 , 0 ) ;
  primitive ( 485 , 12 , 0 ) ;
  primitive ( 486 , 15 , 0 ) ;
  primitive ( 487 , 16 , 0 ) ;
  primitive ( 488 , 17 , 0 ) ;
  primitive ( 489 , 70 , 0 ) ;
  primitive ( 490 , 26 , 0 ) ;
  primitive ( 491 , 14 , 0 ) ;
  primitive ( 492 , 11 , 0 ) ;
  primitive ( 493 , 19 , 0 ) ;
  primitive ( 494 , 77 , 0 ) ;
  primitive ( 495 , 30 , 0 ) ;
  primitive ( 496 , 72 , 0 ) ;
  primitive ( 497 , 37 , 0 ) ;
  primitive ( 498 , 60 , 0 ) ;
  primitive ( 499 , 71 , 0 ) ;
  primitive ( 500 , 73 , 0 ) ;
  primitive ( 501 , 74 , 0 ) ;
  primitive ( 502 , 31 , 0 ) ;
  primitive ( 681 , 1 , 0 ) ;
  primitive ( 682 , 1 , 1 ) ;
  primitive ( 465 , 2 , 0 ) ;
  eqtb [9768 ]= eqtb [cursym ];
  primitive ( 466 , 3 , 0 ) ;
  eqtb [9769 ]= eqtb [cursym ];
  primitive ( 695 , 18 , 1 ) ;
  primitive ( 696 , 18 , 2 ) ;
  primitive ( 697 , 18 , 55 ) ;
  primitive ( 698 , 18 , 46 ) ;
  primitive ( 699 , 18 , 51 ) ;
  primitive ( 469 , 18 , 0 ) ;
  eqtb [9765 ]= eqtb [cursym ];
  primitive ( 700 , 7 , 9772 ) ;
  primitive ( 701 , 7 , 9922 ) ;
  primitive ( 702 , 7 , 1 ) ;
  primitive ( 470 , 7 , 0 ) ;
  eqtb [9764 ]= eqtb [cursym ];
  primitive ( 703 , 63 , 0 ) ;
  primitive ( 704 , 63 , 1 ) ;
  primitive ( 64 , 63 , 2 ) ;
  primitive ( 705 , 63 , 3 ) ;
  primitive ( 716 , 58 , 9772 ) ;
  primitive ( 717 , 58 , 9922 ) ;
  primitive ( 718 , 58 , 10072 ) ;
  primitive ( 719 , 58 , 1 ) ;
  primitive ( 720 , 58 , 2 ) ;
  primitive ( 721 , 58 , 3 ) ;
  primitive ( 731 , 6 , 0 ) ;
  primitive ( 628 , 6 , 1 ) ;
  primitive ( 758 , 4 , 1 ) ;
  primitive ( 467 , 5 , 2 ) ;
  eqtb [9766 ]= eqtb [cursym ];
  primitive ( 759 , 5 , 3 ) ;
  primitive ( 760 , 5 , 4 ) ;
  primitive ( 347 , 35 , 30 ) ;
  primitive ( 348 , 35 , 31 ) ;
  primitive ( 349 , 35 , 32 ) ;
  primitive ( 350 , 35 , 33 ) ;
  primitive ( 351 , 35 , 34 ) ;
  primitive ( 352 , 35 , 35 ) ;
  primitive ( 353 , 35 , 36 ) ;
  primitive ( 354 , 35 , 37 ) ;
  primitive ( 355 , 36 , 38 ) ;
  primitive ( 356 , 36 , 39 ) ;
  primitive ( 357 , 36 , 40 ) ;
  primitive ( 358 , 36 , 41 ) ;
  primitive ( 359 , 36 , 42 ) ;
  primitive ( 360 , 36 , 43 ) ;
  primitive ( 361 , 36 , 44 ) ;
  primitive ( 362 , 36 , 45 ) ;
  primitive ( 363 , 36 , 46 ) ;
  primitive ( 364 , 36 , 47 ) ;
  primitive ( 365 , 36 , 48 ) ;
  primitive ( 366 , 36 , 49 ) ;
  primitive ( 367 , 36 , 50 ) ;
  primitive ( 368 , 36 , 51 ) ;
  primitive ( 369 , 36 , 52 ) ;
  primitive ( 370 , 36 , 53 ) ;
  primitive ( 371 , 36 , 54 ) ;
  primitive ( 372 , 36 , 55 ) ;
  primitive ( 373 , 36 , 56 ) ;
  primitive ( 374 , 36 , 57 ) ;
  primitive ( 375 , 36 , 58 ) ;
  primitive ( 376 , 36 , 59 ) ;
  primitive ( 377 , 36 , 60 ) ;
  primitive ( 378 , 36 , 61 ) ;
  primitive ( 379 , 36 , 62 ) ;
  primitive ( 380 , 36 , 63 ) ;
  primitive ( 381 , 36 , 64 ) ;
  primitive ( 382 , 36 , 65 ) ;
  primitive ( 383 , 36 , 66 ) ;
  primitive ( 384 , 36 , 67 ) ;
  primitive ( 385 , 36 , 68 ) ;
  primitive ( 386 , 36 , 69 ) ;
  primitive ( 387 , 36 , 70 ) ;
  primitive ( 388 , 36 , 71 ) ;
  primitive ( 389 , 36 , 72 ) ;
  primitive ( 390 , 36 , 73 ) ;
  primitive ( 391 , 36 , 74 ) ;
  primitive ( 392 , 36 , 75 ) ;
  primitive ( 393 , 36 , 76 ) ;
  primitive ( 394 , 36 , 77 ) ;
  primitive ( 395 , 36 , 78 ) ;
  primitive ( 396 , 36 , 79 ) ;
  primitive ( 397 , 36 , 80 ) ;
  primitive ( 398 , 36 , 81 ) ;
  primitive ( 399 , 36 , 82 ) ;
  primitive ( 400 , 38 , 83 ) ;
  primitive ( 402 , 36 , 85 ) ;
  primitive ( 401 , 36 , 84 ) ;
  primitive ( 403 , 36 , 86 ) ;
  primitive ( 404 , 36 , 87 ) ;
  primitive ( 405 , 36 , 88 ) ;
  primitive ( 43 , 45 , 89 ) ;
  primitive ( 45 , 45 , 90 ) ;
  primitive ( 42 , 57 , 91 ) ;
  primitive ( 47 , 56 , 92 ) ;
  eqtb [9761 ]= eqtb [cursym ];
  primitive ( 406 , 47 , 93 ) ;
  primitive ( 310 , 47 , 94 ) ;
  primitive ( 407 , 47 , 95 ) ;
  primitive ( 408 , 54 , 96 ) ;
  primitive ( 60 , 52 , 97 ) ;
  primitive ( 409 , 52 , 98 ) ;
  primitive ( 62 , 52 , 99 ) ;
  primitive ( 410 , 52 , 100 ) ;
  primitive ( 61 , 53 , 101 ) ;
  primitive ( 411 , 52 , 102 ) ;
  primitive ( 422 , 39 , 115 ) ;
  primitive ( 423 , 39 , 116 ) ;
  primitive ( 424 , 39 , 117 ) ;
  primitive ( 425 , 39 , 118 ) ;
  primitive ( 426 , 39 , 119 ) ;
  primitive ( 427 , 39 , 120 ) ;
  primitive ( 428 , 39 , 121 ) ;
  primitive ( 429 , 39 , 122 ) ;
  primitive ( 38 , 50 , 103 ) ;
  primitive ( 412 , 57 , 104 ) ;
  primitive ( 413 , 57 , 105 ) ;
  primitive ( 414 , 57 , 106 ) ;
  primitive ( 415 , 57 , 107 ) ;
  primitive ( 416 , 57 , 108 ) ;
  primitive ( 417 , 57 , 109 ) ;
  primitive ( 418 , 57 , 110 ) ;
  primitive ( 419 , 57 , 111 ) ;
  primitive ( 420 , 57 , 112 ) ;
  primitive ( 421 , 47 , 113 ) ;
  primitive ( 340 , 32 , 15 ) ;
  primitive ( 259 , 32 , 4 ) ;
  primitive ( 325 , 32 , 2 ) ;
  primitive ( 330 , 32 , 8 ) ;
  primitive ( 328 , 32 , 6 ) ;
  primitive ( 332 , 32 , 10 ) ;
  primitive ( 334 , 32 , 12 ) ;
  primitive ( 335 , 32 , 13 ) ;
  primitive ( 336 , 32 , 14 ) ;
  primitive ( 960 , 84 , 0 ) ;
  primitive ( 961 , 84 , 1 ) ;
  primitive ( 272 , 25 , 0 ) ;
  primitive ( 273 , 25 , 1 ) ;
  primitive ( 274 , 25 , 2 ) ;
  primitive ( 967 , 25 , 3 ) ;
  primitive ( 968 , 23 , 0 ) ;
  primitive ( 969 , 23 , 1 ) ;
  primitive ( 983 , 24 , 0 ) ;
  primitive ( 984 , 24 , 1 ) ;
  primitive ( 985 , 24 , 2 ) ;
  primitive ( 986 , 24 , 3 ) ;
  primitive ( 987 , 24 , 4 ) ;
  primitive ( 1006 , 69 , 0 ) ;
  primitive ( 1007 , 69 , 1 ) ;
  primitive ( 1008 , 69 , 2 ) ;
  primitive ( 1009 , 68 , 6 ) ;
  primitive ( 1010 , 68 , 10 ) ;
  primitive ( 1011 , 68 , 13 ) ;
  primitive ( 1020 , 21 , 4 ) ;
  primitive ( 1021 , 21 , 5 ) ;
  primitive ( 1034 , 27 , 0 ) ;
  primitive ( 1035 , 27 , 1 ) ;
  primitive ( 1036 , 27 , 2 ) ;
  primitive ( 1051 , 22 , 0 ) ;
  primitive ( 1052 , 22 , 1 ) ;
  primitive ( 1053 , 22 , 2 ) ;
  primitive ( 1054 , 22 , 3 ) ;
  primitive ( 1055 , 22 , 4 ) ;
  primitive ( 1073 , 75 , 0 ) ;
  primitive ( 1074 , 75 , 1 ) ;
  primitive ( 1075 , 75 , 5 ) ;
  primitive ( 1076 , 75 , 2 ) ;
  primitive ( 1077 , 75 , 6 ) ;
  primitive ( 1078 , 75 , 3 ) ;
  primitive ( 1079 , 75 , 7 ) ;
  primitive ( 1080 , 75 , 11 ) ;
  primitive ( 1081 , 75 , 128 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
inittab ( void ) 
#else
inittab ( ) 
#endif
{
  integer k  ;
  rover = 24 ;
  mem [rover ].hhfield .v.RH = 268435455L ;
  mem [rover ].hhfield .lhfield = 1000 ;
  mem [rover + 1 ].hhfield .lhfield = rover ;
  mem [rover + 1 ].hhfield .v.RH = rover ;
  lomemmax = rover + 1000 ;
  mem [lomemmax ].hhfield .v.RH = 0 ;
  mem [lomemmax ].hhfield .lhfield = 0 ;
  {register integer for_end; k = memtop - 3 ;for_end = memtop ; if ( k <= 
  for_end) do 
    mem [k ]= mem [lomemmax ];
  while ( k++ < for_end ) ;} 
  avail = 0 ;
  memend = memtop ;
  himemmin = memtop - 3 ;
  varused = 24 ;
  dynused = memtop + 1 - ( memtop - 3 ) ;
  mem [14 ].cint = -32768L ;
  mem [15 ].cint = 0 ;
  mem [17 ].cint = 32768L ;
  mem [18 ].cint = 0 ;
  mem [20 ].cint = 0 ;
  mem [21 ].cint = 65536L ;
  mem [13 ].hhfield .v.RH = 16 ;
  mem [16 ].hhfield .v.RH = 19 ;
  mem [19 ].hhfield .v.RH = 13 ;
  mem [13 ].hhfield .lhfield = 19 ;
  mem [16 ].hhfield .lhfield = 13 ;
  mem [19 ].hhfield .lhfield = 16 ;
  intname [1 ]= 430 ;
  intname [2 ]= 431 ;
  intname [3 ]= 432 ;
  intname [4 ]= 433 ;
  intname [5 ]= 434 ;
  intname [6 ]= 435 ;
  intname [7 ]= 436 ;
  intname [8 ]= 437 ;
  intname [9 ]= 438 ;
  intname [10 ]= 439 ;
  intname [11 ]= 440 ;
  intname [12 ]= 441 ;
  intname [13 ]= 442 ;
  intname [14 ]= 443 ;
  intname [15 ]= 444 ;
  intname [16 ]= 445 ;
  intname [17 ]= 446 ;
  intname [18 ]= 447 ;
  intname [19 ]= 448 ;
  intname [20 ]= 449 ;
  intname [21 ]= 450 ;
  intname [22 ]= 451 ;
  intname [23 ]= 452 ;
  intname [24 ]= 453 ;
  intname [25 ]= 454 ;
  intname [26 ]= 455 ;
  intname [27 ]= 456 ;
  intname [28 ]= 457 ;
  intname [29 ]= 458 ;
  intname [30 ]= 459 ;
  intname [31 ]= 460 ;
  intname [32 ]= 461 ;
  intname [33 ]= 462 ;
  hashused = 9757 ;
  stcount = 0 ;
  hash [9770 ].v.RH = 464 ;
  hash [9768 ].v.RH = 465 ;
  hash [9769 ].v.RH = 466 ;
  hash [9766 ].v.RH = 467 ;
  hash [9767 ].v.RH = 468 ;
  hash [9765 ].v.RH = 469 ;
  hash [9764 ].v.RH = 470 ;
  hash [9763 ].v.RH = 59 ;
  hash [9762 ].v.RH = 58 ;
  hash [9761 ].v.RH = 47 ;
  hash [9760 ].v.RH = 91 ;
  hash [9759 ].v.RH = 41 ;
  hash [9757 ].v.RH = 471 ;
  eqtb [9759 ].lhfield = 64 ;
  mem [0 ].hhfield .v.RH = 0 ;
  mem [1 ].cint = 0 ;
  mem [11 ].hhfield .lhfield = 9772 ;
  mem [11 ].hhfield .v.RH = 0 ;
  serialno = 0 ;
  mem [5 ].hhfield .v.RH = 5 ;
  mem [6 ].hhfield .lhfield = 5 ;
  mem [5 ].hhfield .lhfield = 0 ;
  mem [6 ].hhfield .v.RH = 0 ;
  mem [22 ].hhfield .b1 = 0 ;
  mem [22 ].hhfield .v.RH = 9770 ;
  eqtb [9770 ].v.RH = 22 ;
  eqtb [9770 ].lhfield = 43 ;
  eqtb [9758 ].lhfield = 93 ;
  hash [9758 ].v.RH = 775 ;
  mem [9 ].hhfield .b1 = 14 ;
  mem [12 ].cint = 1073741824L ;
  mem [8 ].cint = 0 ;
  mem [7 ].hhfield .lhfield = 0 ;
  fontdsize [0 ]= 0 ;
  fontname [0 ]= 284 ;
  fontpsname [0 ]= 284 ;
  fontbc [0 ]= 1 ;
  fontec [0 ]= 0 ;
  charbase [0 ]= 0 ;
  widthbase [0 ]= 0 ;
  heightbase [0 ]= 0 ;
  depthbase [0 ]= 0 ;
  nextfmem = 0 ;
  lastfnum = 0 ;
  lastpsfnum = 0 ;
  if ( iniversion ) 
  memident = 1173 ;
} 
#endif /* INIMP */
void mainbody() {
    
  bounddefault = 250000L ;
  boundname = "main_memory" ;
  setupboundvariable ( addressof ( mainmemory ) , boundname , bounddefault ) ;
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
  if ( errorline > 255 ) 
  errorline = 255 ;
  {
    if ( mainmemory < infmainmemory ) 
    mainmemory = infmainmemory ;
    else if ( mainmemory > supmainmemory ) 
    mainmemory = supmainmemory ;
  } 
  memtop = 0 + mainmemory ;
  memmax = memtop ;
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
#ifdef INIMP
  if ( iniversion ) 
  {
    xmallocarray ( mem , memtop - 0 ) ;
    xmallocarray ( strref , maxstrings ) ;
    xmallocarray ( nextstr , maxstrings ) ;
    xmallocarray ( strstart , maxstrings ) ;
    xmallocarray ( strpool , poolsize ) ;
  } 
#endif /* INIMP */
  history = 3 ;
  if ( readyalready == 314159L ) 
  goto lab1 ;
  bad = 0 ;
  if ( ( halferrorline < 30 ) || ( halferrorline > errorline - 15 ) ) 
  bad = 1 ;
  if ( maxprintline < 60 ) 
  bad = 2 ;
  if ( emergencylinelength < maxprintline ) 
  bad = 3 ;
  if ( 1100 > memtop ) 
  bad = 4 ;
  if ( 7919 > 9500 ) 
  bad = 5 ;
  if ( headersize % 4 != 0 ) 
  bad = 6 ;
  if ( ( ligtablesize < 255 ) || ( ligtablesize > 32510 ) ) 
  bad = 7 ;
#ifdef INIMP
  if ( memmax != memtop ) 
  bad = 8 ;
#endif /* INIMP */
  if ( memmax < memtop ) 
  bad = 8 ;
  if ( ( 0 > 0 ) || ( 255 < 127 ) ) 
  bad = 9 ;
  if ( ( 0 > 0 ) || ( 268435455L < 32767 ) ) 
  bad = 10 ;
  if ( ( 0 < 0 ) || ( 255 > 268435455L ) ) 
  bad = 11 ;
  if ( ( 0 < 0 ) || ( memmax >= 268435455L ) ) 
  bad = 12 ;
  if ( maxstrings > 268435455L ) 
  bad = 13 ;
  if ( bufsize > 268435455L ) 
  bad = 14 ;
  if ( fontmax > 268435455L ) 
  bad = 15 ;
  if ( ( 255 < 255 ) || ( 268435455L < 65535L ) ) 
  bad = 16 ;
  if ( 9771 + maxinternal > 268435455L ) 
  bad = 17 ;
  if ( 10222 > 268435455L ) 
  bad = 18 ;
  if ( 20 + 17 * 45 > bistacksize ) 
  bad = 19 ;
  if ( memdefaultlength > maxint ) 
  bad = 20 ;
  if ( bad > 0 ) 
  {
    fprintf( stdout , "%s%s%ld\n",  "Ouch---my internal constants have been clobbered!" ,     "---case " , (long)bad ) ;
    goto lab9999 ;
  } 
  initialize () ;
#ifdef INIMP
  if ( iniversion ) 
  {
    if ( ! getstringsstarted () ) 
    goto lab9999 ;
    inittab () ;
    initprim () ;
    initstruse = strptr ;
    initpoolptr = poolptr ;
    maxstrptr = strptr ;
    maxpoolptr = poolptr ;
    fixdateandtime () ;
  } 
#endif /* INIMP */
  readyalready = 314159L ;
  lab1: selector = 8 ;
  tally = 0 ;
  termoffset = 0 ;
  fileoffset = 0 ;
  psoffset = 0 ;
  Fputs( stdout ,  "This is MetaPost, Version 0.641" ) ;
  Fputs( stdout ,  versionstring ) ;
  if ( memident > 0 ) 
  print ( memident ) ;
  println () ;
  fflush ( stdout ) ;
  jobname = 0 ;
  logopened = false ;
  {
    {
      inputptr = 0 ;
      maxinstack = 0 ;
      inopen = 0 ;
      openparens = 0 ;
      maxbufstack = 0 ;
      paramptr = 0 ;
      maxparamstack = 0 ;
      first = 1 ;
      curinput .startfield = 1 ;
      curinput .indexfield = 0 ;
      linestack [curinput .indexfield ]= 0 ;
      curinput .namefield = 0 ;
      mpxname [0 ]= 1 ;
      forceeof = false ;
      if ( ! initterminal () ) 
      goto lab9999 ;
      curinput .limitfield = last ;
      first = last + 1 ;
    } 
    scannerstatus = 0 ;
    if ( ( memident == 0 ) || ( buffer [curinput .locfield ]== 38 ) || 
    dumpline ) 
    {
      if ( memident != 0 ) 
      initialize () ;
      if ( ! openmemfile () ) 
      goto lab9999 ;
      if ( ! loadmemfile () ) 
      {
	wclose ( memfile ) ;
	goto lab9999 ;
      } 
      wclose ( memfile ) ;
      while ( ( curinput .locfield < curinput .limitfield ) && ( buffer [
      curinput .locfield ]== 32 ) ) incr ( curinput .locfield ) ;
    } 
    buffer [curinput .limitfield ]= 37 ;
    fixdateandtime () ;
    initrandoms ( ( internal [16 ]/ 65536L ) + internal [15 ]) ;
    if ( interaction == 0 ) 
    selector = 7 ;
    else selector = 8 ;
    if ( curinput .locfield < curinput .limitfield ) 
    if ( buffer [curinput .locfield ]!= 92 ) 
    startinput () ;
  } 
  history = 0 ;
  if ( troffmode ) 
  internal [32 ]= 65536L ;
  if ( startsym > 0 ) 
  {
    cursym = startsym ;
    backinput () ;
  } 
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
