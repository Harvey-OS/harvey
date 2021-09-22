#define EXTERN extern
#include "texd.h"

void 
#ifdef HAVE_PROTOTYPES
println ( void ) 
#else
println ( ) 
#endif
{
  println_regmem 
  switch ( selector ) 
  {case 19 : 
    {
      putc ('\n',  stdout );
      putc ('\n',  logfile );
      termoffset = 0 ;
      fileoffset = 0 ;
    } 
    break ;
  case 18 : 
    {
      putc ('\n',  logfile );
      fileoffset = 0 ;
    } 
    break ;
  case 17 : 
    {
      putc ('\n',  stdout );
      termoffset = 0 ;
    } 
    break ;
  case 16 : 
  case 20 : 
  case 21 : 
    ;
    break ;
    default: 
    putc ('\n',  writefile [selector ]);
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
  /* 10 */ printvisiblechar_regmem 
  switch ( selector ) 
  {case 19 : 
    {
      putc ( Xchr (s ),  stdout );
      putc ( Xchr (s ),  logfile );
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
  case 18 : 
    {
      putc ( Xchr (s ),  logfile );
      incr ( fileoffset ) ;
      if ( fileoffset == maxprintline ) 
      println () ;
    } 
    break ;
  case 17 : 
    {
      putc ( Xchr (s ),  stdout );
      incr ( termoffset ) ;
      if ( termoffset == maxprintline ) 
      println () ;
    } 
    break ;
  case 16 : 
    ;
    break ;
  case 20 : 
    if ( tally < trickcount ) 
    trickbuf [tally % errorline ]= s ;
    break ;
  case 21 : 
    {
      if ( poolptr < poolsize ) 
      {
	strpool [poolptr ]= s ;
	incr ( poolptr ) ;
      } 
    } 
    break ;
    default: 
    putc ( Xchr (s ),  writefile [selector ]);
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
  /* 10 */ printchar_regmem 
  ASCIIcode k  ;
  unsigned char l  ;
  if ( selector > 20 ) 
  {
    printvisiblechar ( s ) ;
    return ;
  } 
  if ( s == eqtb [15212 ].cint ) 
  if ( selector < 20 ) 
  {
    println () ;
    return ;
  } 
  k = s ;
  if ( ( ( ( k < 32 ) || ( k > 126 ) ) && ! ( isprint ( xord [k ]) ) ) ) 
  {
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
} 
void 
#ifdef HAVE_PROTOTYPES
zprint ( integer s ) 
#else
zprint ( s ) 
  integer s ;
#endif
{
  /* 10 */ print_regmem 
  poolpointer j  ;
  if ( s >= strptr ) 
  s = 259 ;
  else if ( s < 256 ) 
  if ( s < 0 ) 
  s = 259 ;
  else {
      
    printchar ( s ) ;
    return ;
  } 
  j = strstart [s ];
  while ( j < strstart [s + 1 ]) {
      
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
  printnl_regmem 
  if ( ( ( termoffset > 0 ) && ( odd ( selector ) ) ) || ( ( fileoffset > 0 ) 
  && ( selector >= 18 ) ) ) 
  println () ;
  print ( s ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintesc ( strnumber s ) 
#else
zprintesc ( s ) 
  strnumber s ;
#endif
{
  printesc_regmem 
  integer c  ;
  c = eqtb [15208 ].cint ;
  if ( c >= 0 ) 
  if ( c < 256 ) 
  print ( c ) ;
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
  printthedigs_regmem 
  while ( k > 0 ) {
      
    decr ( k ) ;
    if ( dig [k ]< 10 ) 
    printchar ( 48 + dig [k ]) ;
    else printchar ( 55 + dig [k ]) ;
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
  printint_regmem 
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
zprintcs ( integer p ) 
#else
zprintcs ( p ) 
  integer p ;
#endif
{
  printcs_regmem 
  if ( p < 514 ) 
  if ( p >= 257 ) 
  if ( p == 513 ) 
  {
    printesc ( 504 ) ;
    printesc ( 505 ) ;
  } 
  else {
      
    printesc ( p - 257 ) ;
    if ( eqtb [13627 + p - 257 ].hh .v.RH == 11 ) 
    printchar ( 32 ) ;
  } 
  else if ( p < 1 ) 
  printesc ( 506 ) ;
  else print ( p - 1 ) ;
  else if ( ( ( p >= 12525 ) && ( p <= 16009 ) ) || ( p > eqtbtop ) ) 
  printesc ( 506 ) ;
  else if ( ( hash [p ].v.RH >= strptr ) ) 
  printesc ( 507 ) ;
  else {
      
    printesc ( hash [p ].v.RH ) ;
    printchar ( 32 ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zsprintcs ( halfword p ) 
#else
zsprintcs ( p ) 
  halfword p ;
#endif
{
  sprintcs_regmem 
  if ( p < 514 ) 
  if ( p < 257 ) 
  print ( p - 1 ) ;
  else if ( p < 513 ) 
  printesc ( p - 257 ) ;
  else {
      
    printesc ( 504 ) ;
    printesc ( 505 ) ;
  } 
  else printesc ( hash [p ].v.RH ) ;
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
  printfilename_regmem 
  print ( a ) ;
  print ( n ) ;
  print ( e ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintsize ( integer s ) 
#else
zprintsize ( s ) 
  integer s ;
#endif
{
  printsize_regmem 
  if ( s == 0 ) 
  printesc ( 409 ) ;
  else if ( s == 16 ) 
  printesc ( 410 ) ;
  else printesc ( 411 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintwritewhatsit ( strnumber s , halfword p ) 
#else
zprintwritewhatsit ( s , p ) 
  strnumber s ;
  halfword p ;
#endif
{
  printwritewhatsit_regmem 
  printesc ( s ) ;
  if ( mem [p + 1 ].hh .v.LH < 16 ) 
  printint ( mem [p + 1 ].hh .v.LH ) ;
  else if ( mem [p + 1 ].hh .v.LH == 16 ) 
  printchar ( 42 ) ;
  else printchar ( 45 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintcsnames ( integer hstart , integer hfinish ) 
#else
zprintcsnames ( hstart , hfinish ) 
  integer hstart ;
  integer hfinish ;
#endif
{
  printcsnames_regmem 
  integer c, h, where  ;
  fprintf( stderr , "%s%ld%s%ld%c\n",  "fmtdebug:csnames from " , (long)hstart , " to " , (long)hfinish ,   ':' ) ;
  {register integer for_end; h = hstart ;for_end = hfinish ; if ( h <= 
  for_end) do 
    {
      if ( hash [h ].v.RH > 0 ) 
      {
	where = h ;
	do {
	    { register integer for_end; c = strstart [hash [where ].v.RH ]
	  ;for_end = strstart [hash [where ].v.RH + 1 ]- 1 ; if ( c <= 
	  for_end) do 
	    {
	      putbyte ( strpool [c ], stderr ) ;
	    } 
	  while ( c++ < for_end ) ;} 
	  fprintf( stderr , "%s\n",  "" ) ;
	  where = hash [where ].v.LH ;
	} while ( ! ( where == 0 ) ) ;
      } 
    } 
  while ( h++ < for_end ) ;} 
} 
#ifdef TEXMF_DEBUG
#endif /* TEXMF_DEBUG */
void 
#ifdef HAVE_PROTOTYPES
jumpout ( void ) 
#else
jumpout ( ) 
#endif
{
  jumpout_regmem 
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
  /* 22 10 */ error_regmem 
  ASCIIcode c  ;
  integer s1, s2, s3, s4  ;
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
    return ;
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
	s1 = curtok ;
	s2 = curcmd ;
	s3 = curchr ;
	s4 = alignstate ;
	alignstate = 1000000L ;
	OKtointerrupt = false ;
	if ( ( last > first + 1 ) && ( buffer [first + 1 ]>= 48 ) && ( 
	buffer [first + 1 ]<= 57 ) ) 
	c = c * 10 + buffer [first + 1 ]- 48 * 11 ;
	else c = c - 48 ;
	while ( c > 0 ) {
	    
	  gettoken () ;
	  decr ( c ) ;
	} 
	curtok = s1 ;
	curcmd = s2 ;
	curchr = s3 ;
	alignstate = s4 ;
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
      if ( baseptr > 0 ) 
      {
	editnamestart = strstart [inputstack [baseptr ].namefield ];
	editnamelength = strstart [inputstack [baseptr ].namefield + 1 ]- 
	strstart [inputstack [baseptr ].namefield ];
	editline = line ;
	jumpout () ;
      } 
      break ;
    case 72 : 
      {
	if ( useerrhelp ) 
	{
	  giveerrhelp () ;
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
	first = last ;
	curinput .limitfield = last - 1 ;
	return ;
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
	    printesc ( 272 ) ;
	    decr ( selector ) ;
	  } 
	  break ;
	case 82 : 
	  printesc ( 273 ) ;
	  break ;
	case 83 : 
	  printesc ( 274 ) ;
	  break ;
	} 
	print ( 275 ) ;
	println () ;
	fflush ( stdout ) ;
	return ;
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
      if ( baseptr > 0 ) 
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
    println () ;
    giveerrhelp () ;
  } 
  else while ( helpptr > 0 ) {
      
    decr ( helpptr ) ;
    printnl ( helpline [helpptr ]) ;
  } 
  println () ;
  if ( interaction > 0 ) 
  incr ( selector ) ;
  println () ;
} 
void 
#ifdef HAVE_PROTOTYPES
zfatalerror ( strnumber s ) 
#else
zfatalerror ( s ) 
  strnumber s ;
#endif
{
  fatalerror_regmem 
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
  overflow_regmem 
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
  confusion_regmem 
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
  /* 10 */ register boolean Result; initterminal_regmem 
  topenin () ;
  if ( last > first ) 
  {
    curinput .locfield = first ;
    while ( ( curinput .locfield < last ) && ( buffer [curinput .locfield ]
    == ' ' ) ) incr ( curinput .locfield ) ;
    if ( curinput .locfield < last ) 
    {
      Result = true ;
      return Result ;
    } 
  } 
  while ( true ) {
      
    ;
    Fputs( stdout ,  "**" ) ;
    fflush ( stdout ) ;
    if ( ! inputln ( stdin , true ) ) 
    {
      putc ('\n',  stdout );
      fprintf( stdout , "%s\n",  "! End of file on the terminal... why?" ) ;
      Result = false ;
      return Result ;
    } 
    curinput .locfield = first ;
    while ( ( curinput .locfield < last ) && ( buffer [curinput .locfield ]
    == 32 ) ) incr ( curinput .locfield ) ;
    if ( curinput .locfield < last ) 
    {
      Result = true ;
      return Result ;
    } 
    fprintf( stdout , "%s\n",  "Please type the name of your input file." ) ;
  } 
  return Result ;
} 
strnumber 
#ifdef HAVE_PROTOTYPES
makestring ( void ) 
#else
makestring ( ) 
#endif
{
  register strnumber Result; makestring_regmem 
  if ( strptr == maxstrings ) 
  overflow ( 258 , maxstrings - initstrptr ) ;
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
  /* 45 */ register boolean Result; streqbuf_regmem 
  poolpointer j  ;
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
boolean 
#ifdef HAVE_PROTOTYPES
zstreqstr ( strnumber s , strnumber t ) 
#else
zstreqstr ( s , t ) 
  strnumber s ;
  strnumber t ;
#endif
{
  /* 45 */ register boolean Result; streqstr_regmem 
  poolpointer j, k  ;
  boolean result  ;
  result = false ;
  if ( ( strstart [s + 1 ]- strstart [s ]) != ( strstart [t + 1 ]- 
  strstart [t ]) ) 
  goto lab45 ;
  j = strstart [s ];
  k = strstart [t ];
  while ( j < strstart [s + 1 ]) {
      
    if ( strpool [j ]!= strpool [k ]) 
    goto lab45 ;
    incr ( j ) ;
    incr ( k ) ;
  } 
  result = true ;
  lab45: Result = result ;
  return Result ;
} 
strnumber 
#ifdef HAVE_PROTOTYPES
zsearchstring ( strnumber search ) 
#else
zsearchstring ( search ) 
  strnumber search ;
#endif
{
  /* 40 */ register strnumber Result; searchstring_regmem 
  strnumber result  ;
  strnumber s  ;
  integer len  ;
  result = 0 ;
  len = ( strstart [search + 1 ]- strstart [search ]) ;
  if ( len == 0 ) 
  {
    result = 335 ;
    goto lab40 ;
  } 
  else {
      
    s = search - 1 ;
    while ( s > 255 ) {
	
      if ( ( strstart [s + 1 ]- strstart [s ]) == len ) 
      if ( streqstr ( s , search ) ) 
      {
	result = s ;
	goto lab40 ;
      } 
      decr ( s ) ;
    } 
  } 
  lab40: Result = result ;
  return Result ;
} 
strnumber 
#ifdef HAVE_PROTOTYPES
slowmakestring ( void ) 
#else
slowmakestring ( ) 
#endif
{
  /* 10 */ register strnumber Result; slowmakestring_regmem 
  strnumber s  ;
  strnumber t  ;
  t = makestring () ;
  s = searchstring ( t ) ;
  if ( s > 0 ) 
  {
    {
      decr ( strptr ) ;
      poolptr = strstart [strptr ];
    } 
    Result = s ;
    return Result ;
  } 
  Result = t ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprinttwo ( integer n ) 
#else
zprinttwo ( n ) 
  integer n ;
#endif
{
  printtwo_regmem 
  n = abs ( n ) % 100 ;
  printchar ( 48 + ( n / 10 ) ) ;
  printchar ( 48 + ( n % 10 ) ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprinthex ( integer n ) 
#else
zprinthex ( n ) 
  integer n ;
#endif
{
  printhex_regmem 
  char k  ;
  k = 0 ;
  printchar ( 34 ) ;
  do {
      dig [k ]= n % 16 ;
    n = n / 16 ;
    incr ( k ) ;
  } while ( ! ( n == 0 ) ) ;
  printthedigs ( k ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintromanint ( integer n ) 
#else
zprintromanint ( n ) 
  integer n ;
#endif
{
  /* 10 */ printromanint_regmem 
  poolpointer j, k  ;
  nonnegativeinteger u, v  ;
  j = strstart [260 ];
  v = 1000 ;
  while ( true ) {
      
    while ( n >= v ) {
	
      printchar ( strpool [j ]) ;
      n = n - v ;
    } 
    if ( n <= 0 ) 
    return ;
    k = j + 2 ;
    u = v / ( strpool [k - 1 ]- 48 ) ;
    if ( strpool [k - 1 ]== 50 ) 
    {
      k = k + 2 ;
      u = u / ( strpool [k - 1 ]- 48 ) ;
    } 
    if ( n + u >= v ) 
    {
      printchar ( strpool [k ]) ;
      n = n + u ;
    } 
    else {
	
      j = j + 2 ;
      v = v / ( strpool [j - 1 ]- 48 ) ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
printcurrentstring ( void ) 
#else
printcurrentstring ( ) 
#endif
{
  printcurrentstring_regmem 
  poolpointer j  ;
  j = strstart [strptr ];
  while ( j < poolptr ) {
      
    printchar ( strpool [j ]) ;
    incr ( j ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
terminput ( void ) 
#else
terminput ( ) 
#endif
{
  terminput_regmem 
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
  incr ( selector ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zinterror ( integer n ) 
#else
zinterror ( n ) 
  integer n ;
#endif
{
  interror_regmem 
  print ( 284 ) ;
  printint ( n ) ;
  printchar ( 41 ) ;
  error () ;
} 
void 
#ifdef HAVE_PROTOTYPES
normalizeselector ( void ) 
#else
normalizeselector ( ) 
#endif
{
  normalizeselector_regmem 
  if ( logopened ) 
  selector = 19 ;
  else selector = 17 ;
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
  pauseforinstructions_regmem 
  if ( OKtointerrupt ) 
  {
    interaction = 3 ;
    if ( ( selector == 18 ) || ( selector == 16 ) ) 
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
integer 
#ifdef HAVE_PROTOTYPES
zhalf ( integer x ) 
#else
zhalf ( x ) 
  integer x ;
#endif
{
  register integer Result; half_regmem 
  if ( odd ( x ) ) 
  Result = ( x + 1 ) / 2 ;
  else Result = x / 2 ;
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
  register scaled Result; rounddecimals_regmem 
  integer a  ;
  a = 0 ;
  while ( k > 0 ) {
      
    decr ( k ) ;
    a = ( a + dig [k ]* 131072L ) / 10 ;
  } 
  Result = ( a + 1 ) / 2 ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintscaled ( scaled s ) 
#else
zprintscaled ( s ) 
  scaled s ;
#endif
{
  printscaled_regmem 
  scaled delta  ;
  if ( s < 0 ) 
  {
    printchar ( 45 ) ;
    s = - (integer) s ;
  } 
  printint ( s / 65536L ) ;
  printchar ( 46 ) ;
  s = 10 * ( s % 65536L ) + 5 ;
  delta = 10 ;
  do {
      if ( delta > 65536L ) 
    s = s - 17232 ;
    printchar ( 48 + ( s / 65536L ) ) ;
    s = 10 * ( s % 65536L ) ;
    delta = delta * 10 ;
  } while ( ! ( s <= delta ) ) ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zmultandadd ( integer n , scaled x , scaled y , scaled maxanswer ) 
#else
zmultandadd ( n , x , y , maxanswer ) 
  integer n ;
  scaled x ;
  scaled y ;
  scaled maxanswer ;
#endif
{
  register scaled Result; multandadd_regmem 
  if ( n < 0 ) 
  {
    x = - (integer) x ;
    n = - (integer) n ;
  } 
  if ( n == 0 ) 
  Result = y ;
  else if ( ( ( x <= ( maxanswer - y ) / n ) && ( - (integer) x <= ( maxanswer 
  + y ) / n ) ) ) 
  Result = n * x + y ;
  else {
      
    aritherror = true ;
    Result = 0 ;
  } 
  return Result ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zxovern ( scaled x , integer n ) 
#else
zxovern ( x , n ) 
  scaled x ;
  integer n ;
#endif
{
  register scaled Result; xovern_regmem 
  boolean negative  ;
  negative = false ;
  if ( n == 0 ) 
  {
    aritherror = true ;
    Result = 0 ;
    texremainder = x ;
  } 
  else {
      
    if ( n < 0 ) 
    {
      x = - (integer) x ;
      n = - (integer) n ;
      negative = true ;
    } 
    if ( x >= 0 ) 
    {
      Result = x / n ;
      texremainder = x % n ;
    } 
    else {
	
      Result = - (integer) ( ( - (integer) x ) / n ) ;
      texremainder = - (integer) ( ( - (integer) x ) % n ) ;
    } 
  } 
  if ( negative ) 
  texremainder = - (integer) texremainder ;
  return Result ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zxnoverd ( scaled x , integer n , integer d ) 
#else
zxnoverd ( x , n , d ) 
  scaled x ;
  integer n ;
  integer d ;
#endif
{
  register scaled Result; xnoverd_regmem 
  boolean positive  ;
  nonnegativeinteger t, u, v  ;
  if ( x >= 0 ) 
  positive = true ;
  else {
      
    x = - (integer) x ;
    positive = false ;
  } 
  t = ( x % 32768L ) * n ;
  u = ( x / 32768L ) * n + ( t / 32768L ) ;
  v = ( u % d ) * 32768L + ( t % 32768L ) ;
  if ( u / d >= 32768L ) 
  aritherror = true ;
  else u = 32768L * ( u / d ) + ( v / d ) ;
  if ( positive ) 
  {
    Result = u ;
    texremainder = v % d ;
  } 
  else {
      
    Result = - (integer) u ;
    texremainder = - (integer) ( v % d ) ;
  } 
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zbadness ( scaled t , scaled s ) 
#else
zbadness ( t , s ) 
  scaled t ;
  scaled s ;
#endif
{
  register halfword Result; badness_regmem 
  integer r  ;
  if ( t == 0 ) 
  Result = 0 ;
  else if ( s <= 0 ) 
  Result = 10000 ;
  else {
      
    if ( t <= 7230584L ) 
    r = ( t * 297 ) / s ;
    else if ( s >= 1663497L ) 
    r = t / ( s / 297 ) ;
    else r = t ;
    if ( r > 1290 ) 
    Result = 10000 ;
    else Result = ( r * r * r + 131072L ) / 262144L ;
  } 
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
  printword_regmem 
  printint ( w .cint ) ;
  printchar ( 32 ) ;
  printscaled ( w .cint ) ;
  printchar ( 32 ) ;
  printscaled ( round ( 65536L * w .gr ) ) ;
  println () ;
  printint ( w .hh .v.LH ) ;
  printchar ( 61 ) ;
  printint ( w .hh.b0 ) ;
  printchar ( 58 ) ;
  printint ( w .hh.b1 ) ;
  printchar ( 59 ) ;
  printint ( w .hh .v.RH ) ;
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
zshowtokenlist ( integer p , integer q , integer l ) 
#else
zshowtokenlist ( p , q , l ) 
  integer p ;
  integer q ;
  integer l ;
#endif
{
  /* 10 */ showtokenlist_regmem 
  integer m, c  ;
  ASCIIcode matchchr  ;
  ASCIIcode n  ;
  matchchr = 35 ;
  n = 48 ;
  tally = 0 ;
  while ( ( p != 0 ) && ( tally < l ) ) {
      
    if ( p == q ) 
    {
      firstcount = tally ;
      trickcount = tally + 1 + errorline - halferrorline ;
      if ( trickcount < errorline ) 
      trickcount = errorline ;
    } 
    if ( ( p < himemmin ) || ( p > memend ) ) 
    {
      printesc ( 307 ) ;
      return ;
    } 
    if ( mem [p ].hh .v.LH >= 4095 ) 
    printcs ( mem [p ].hh .v.LH - 4095 ) ;
    else {
	
      m = mem [p ].hh .v.LH / 256 ;
      c = mem [p ].hh .v.LH % 256 ;
      if ( mem [p ].hh .v.LH < 0 ) 
      printesc ( 555 ) ;
      else switch ( m ) 
      {case 1 : 
      case 2 : 
      case 3 : 
      case 4 : 
      case 7 : 
      case 8 : 
      case 10 : 
      case 11 : 
      case 12 : 
	print ( c ) ;
	break ;
      case 6 : 
	{
	  print ( c ) ;
	  print ( c ) ;
	} 
	break ;
      case 5 : 
	{
	  print ( matchchr ) ;
	  if ( c <= 9 ) 
	  printchar ( c + 48 ) ;
	  else {
	      
	    printchar ( 33 ) ;
	    return ;
	  } 
	} 
	break ;
      case 13 : 
	{
	  matchchr = c ;
	  print ( c ) ;
	  incr ( n ) ;
	  printchar ( n ) ;
	  if ( n > 57 ) 
	  return ;
	} 
	break ;
      case 14 : 
	print ( 556 ) ;
	break ;
	default: 
	printesc ( 555 ) ;
	break ;
      } 
    } 
    p = mem [p ].hh .v.RH ;
  } 
  if ( p != 0 ) 
  printesc ( 554 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
runaway ( void ) 
#else
runaway ( ) 
#endif
{
  runaway_regmem 
  halfword p  ;
  if ( scannerstatus > 1 ) 
  {
    printnl ( 569 ) ;
    switch ( scannerstatus ) 
    {case 2 : 
      {
	print ( 570 ) ;
	p = defref ;
      } 
      break ;
    case 3 : 
      {
	print ( 571 ) ;
	p = memtop - 3 ;
      } 
      break ;
    case 4 : 
      {
	print ( 572 ) ;
	p = memtop - 4 ;
      } 
      break ;
    case 5 : 
      {
	print ( 573 ) ;
	p = defref ;
      } 
      break ;
    } 
    printchar ( 63 ) ;
    println () ;
    showtokenlist ( mem [p ].hh .v.RH , 0 , errorline - 10 ) ;
  } 
} 
halfword 
#ifdef HAVE_PROTOTYPES
getavail ( void ) 
#else
getavail ( ) 
#endif
{
  register halfword Result; getavail_regmem 
  halfword p  ;
  p = avail ;
  if ( p != 0 ) 
  avail = mem [avail ].hh .v.RH ;
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
      overflow ( 298 , memmax + 1 - memmin ) ;
    } 
  } 
  mem [p ].hh .v.RH = 0 ;
	;
#ifdef STAT
  incr ( dynused ) ;
#endif /* STAT */
  Result = p ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zflushlist ( halfword p ) 
#else
zflushlist ( p ) 
  halfword p ;
#endif
{
  flushlist_regmem 
  halfword q, r  ;
  if ( p != 0 ) 
  {
    r = p ;
    do {
	q = r ;
      r = mem [r ].hh .v.RH ;
	;
#ifdef STAT
      decr ( dynused ) ;
#endif /* STAT */
    } while ( ! ( r == 0 ) ) ;
    mem [q ].hh .v.RH = avail ;
    avail = p ;
  } 
} 
halfword 
#ifdef HAVE_PROTOTYPES
zgetnode ( integer s ) 
#else
zgetnode ( s ) 
  integer s ;
#endif
{
  /* 40 10 20 */ register halfword Result; getnode_regmem 
  halfword p  ;
  halfword q  ;
  integer r  ;
  integer t  ;
  lab20: p = rover ;
  do {
      q = p + mem [p ].hh .v.LH ;
    while ( ( mem [q ].hh .v.RH == 268435455L ) ) {
	
      t = mem [q + 1 ].hh .v.RH ;
      if ( q == rover ) 
      rover = t ;
      mem [t + 1 ].hh .v.LH = mem [q + 1 ].hh .v.LH ;
      mem [mem [q + 1 ].hh .v.LH + 1 ].hh .v.RH = t ;
      q = q + mem [q ].hh .v.LH ;
    } 
    r = q - s ;
    if ( r > toint ( p + 1 ) ) 
    {
      mem [p ].hh .v.LH = r - p ;
      rover = p ;
      goto lab40 ;
    } 
    if ( r == p ) 
    if ( mem [p + 1 ].hh .v.RH != p ) 
    {
      rover = mem [p + 1 ].hh .v.RH ;
      t = mem [p + 1 ].hh .v.LH ;
      mem [rover + 1 ].hh .v.LH = t ;
      mem [t + 1 ].hh .v.RH = rover ;
      goto lab40 ;
    } 
    mem [p ].hh .v.LH = q - p ;
    p = mem [p + 1 ].hh .v.RH ;
  } while ( ! ( p == rover ) ) ;
  if ( s == 1073741824L ) 
  {
    Result = 268435455L ;
    return Result ;
  } 
  if ( lomemmax + 2 < himemmin ) 
  if ( lomemmax + 2 <= membot + 268435455L ) 
  {
    if ( himemmin - lomemmax >= 1998 ) 
    t = lomemmax + 1000 ;
    else t = lomemmax + 1 + ( himemmin - lomemmax ) / 2 ;
    p = mem [rover + 1 ].hh .v.LH ;
    q = lomemmax ;
    mem [p + 1 ].hh .v.RH = q ;
    mem [rover + 1 ].hh .v.LH = q ;
    if ( t > membot + 268435455L ) 
    t = membot + 268435455L ;
    mem [q + 1 ].hh .v.RH = rover ;
    mem [q + 1 ].hh .v.LH = p ;
    mem [q ].hh .v.RH = 268435455L ;
    mem [q ].hh .v.LH = t - lomemmax ;
    lomemmax = t ;
    mem [lomemmax ].hh .v.RH = 0 ;
    mem [lomemmax ].hh .v.LH = 0 ;
    rover = q ;
    goto lab20 ;
  } 
  overflow ( 298 , memmax + 1 - memmin ) ;
  lab40: mem [r ].hh .v.RH = 0 ;
	;
#ifdef STAT
  varused = varused + s ;
#endif /* STAT */
  Result = r ;
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
  freenode_regmem 
  halfword q  ;
  mem [p ].hh .v.LH = s ;
  mem [p ].hh .v.RH = 268435455L ;
  q = mem [rover + 1 ].hh .v.LH ;
  mem [p + 1 ].hh .v.LH = q ;
  mem [p + 1 ].hh .v.RH = rover ;
  mem [rover + 1 ].hh .v.LH = p ;
  mem [q + 1 ].hh .v.RH = p ;
	;
#ifdef STAT
  varused = varused - s ;
#endif /* STAT */
} 
halfword 
#ifdef HAVE_PROTOTYPES
newnullbox ( void ) 
#else
newnullbox ( ) 
#endif
{
  register halfword Result; newnullbox_regmem 
  halfword p  ;
  p = getnode ( 7 ) ;
  mem [p ].hh.b0 = 0 ;
  mem [p ].hh.b1 = 0 ;
  mem [p + 1 ].cint = 0 ;
  mem [p + 2 ].cint = 0 ;
  mem [p + 3 ].cint = 0 ;
  mem [p + 4 ].cint = 0 ;
  mem [p + 5 ].hh .v.RH = 0 ;
  mem [p + 5 ].hh.b0 = 0 ;
  mem [p + 5 ].hh.b1 = 0 ;
  mem [p + 6 ].gr = 0.0 ;
  Result = p ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
newrule ( void ) 
#else
newrule ( ) 
#endif
{
  register halfword Result; newrule_regmem 
  halfword p  ;
  p = getnode ( 4 ) ;
  mem [p ].hh.b0 = 2 ;
  mem [p ].hh.b1 = 0 ;
  mem [p + 1 ].cint = -1073741824L ;
  mem [p + 2 ].cint = -1073741824L ;
  mem [p + 3 ].cint = -1073741824L ;
  Result = p ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
znewligature ( internalfontnumber f , quarterword c , halfword q ) 
#else
znewligature ( f , c , q ) 
  internalfontnumber f ;
  quarterword c ;
  halfword q ;
#endif
{
  register halfword Result; newligature_regmem 
  halfword p  ;
  p = getnode ( 2 ) ;
  mem [p ].hh.b0 = 6 ;
  mem [p + 1 ].hh.b0 = f ;
  mem [p + 1 ].hh.b1 = c ;
  mem [p + 1 ].hh .v.RH = q ;
  mem [p ].hh.b1 = 0 ;
  Result = p ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
znewligitem ( quarterword c ) 
#else
znewligitem ( c ) 
  quarterword c ;
#endif
{
  register halfword Result; newligitem_regmem 
  halfword p  ;
  p = getnode ( 2 ) ;
  mem [p ].hh.b1 = c ;
  mem [p + 1 ].hh .v.RH = 0 ;
  Result = p ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
newdisc ( void ) 
#else
newdisc ( ) 
#endif
{
  register halfword Result; newdisc_regmem 
  halfword p  ;
  p = getnode ( 2 ) ;
  mem [p ].hh.b0 = 7 ;
  mem [p ].hh.b1 = 0 ;
  mem [p + 1 ].hh .v.LH = 0 ;
  mem [p + 1 ].hh .v.RH = 0 ;
  Result = p ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
znewmath ( scaled w , smallnumber s ) 
#else
znewmath ( w , s ) 
  scaled w ;
  smallnumber s ;
#endif
{
  register halfword Result; newmath_regmem 
  halfword p  ;
  p = getnode ( 2 ) ;
  mem [p ].hh.b0 = 9 ;
  mem [p ].hh.b1 = s ;
  mem [p + 1 ].cint = w ;
  Result = p ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
znewspec ( halfword p ) 
#else
znewspec ( p ) 
  halfword p ;
#endif
{
  register halfword Result; newspec_regmem 
  halfword q  ;
  q = getnode ( 4 ) ;
  mem [q ]= mem [p ];
  mem [q ].hh .v.RH = 0 ;
  mem [q + 1 ].cint = mem [p + 1 ].cint ;
  mem [q + 2 ].cint = mem [p + 2 ].cint ;
  mem [q + 3 ].cint = mem [p + 3 ].cint ;
  Result = q ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
znewparamglue ( smallnumber n ) 
#else
znewparamglue ( n ) 
  smallnumber n ;
#endif
{
  register halfword Result; newparamglue_regmem 
  halfword p  ;
  halfword q  ;
  p = getnode ( 2 ) ;
  mem [p ].hh.b0 = 10 ;
  mem [p ].hh.b1 = n + 1 ;
  mem [p + 1 ].hh .v.RH = 0 ;
  q = eqtb [12526 + n ].hh .v.RH ;
  mem [p + 1 ].hh .v.LH = q ;
  incr ( mem [q ].hh .v.RH ) ;
  Result = p ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
znewglue ( halfword q ) 
#else
znewglue ( q ) 
  halfword q ;
#endif
{
  register halfword Result; newglue_regmem 
  halfword p  ;
  p = getnode ( 2 ) ;
  mem [p ].hh.b0 = 10 ;
  mem [p ].hh.b1 = 0 ;
  mem [p + 1 ].hh .v.RH = 0 ;
  mem [p + 1 ].hh .v.LH = q ;
  incr ( mem [q ].hh .v.RH ) ;
  Result = p ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
znewskipparam ( smallnumber n ) 
#else
znewskipparam ( n ) 
  smallnumber n ;
#endif
{
  register halfword Result; newskipparam_regmem 
  halfword p  ;
  tempptr = newspec ( eqtb [12526 + n ].hh .v.RH ) ;
  p = newglue ( tempptr ) ;
  mem [tempptr ].hh .v.RH = 0 ;
  mem [p ].hh.b1 = n + 1 ;
  Result = p ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
znewkern ( scaled w ) 
#else
znewkern ( w ) 
  scaled w ;
#endif
{
  register halfword Result; newkern_regmem 
  halfword p  ;
  p = getnode ( 2 ) ;
  mem [p ].hh.b0 = 11 ;
  mem [p ].hh.b1 = 0 ;
  mem [p + 1 ].cint = w ;
  Result = p ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
znewpenalty ( integer m ) 
#else
znewpenalty ( m ) 
  integer m ;
#endif
{
  register halfword Result; newpenalty_regmem 
  halfword p  ;
  p = getnode ( 2 ) ;
  mem [p ].hh.b0 = 12 ;
  mem [p ].hh.b1 = 0 ;
  mem [p + 1 ].cint = m ;
  Result = p ;
  return Result ;
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
  /* 31 32 */ checkmem_regmem 
  halfword p, q  ;
  boolean clobbered  ;
  {register integer for_end; p = memmin ;for_end = lomemmax ; if ( p <= 
  for_end) do 
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
      printnl ( 299 ) ;
      printint ( q ) ;
      goto lab31 ;
    } 
    freearr [p ]= true ;
    q = p ;
    p = mem [q ].hh .v.RH ;
  } 
  lab31: ;
  p = rover ;
  q = 0 ;
  clobbered = false ;
  do {
      if ( ( p >= lomemmax ) || ( p < memmin ) ) 
    clobbered = true ;
    else if ( ( mem [p + 1 ].hh .v.RH >= lomemmax ) || ( mem [p + 1 ].hh 
    .v.RH < memmin ) ) 
    clobbered = true ;
    else if ( ! ( ( mem [p ].hh .v.RH == 268435455L ) ) || ( mem [p ].hh 
    .v.LH < 2 ) || ( p + mem [p ].hh .v.LH > lomemmax ) || ( mem [mem [p + 
    1 ].hh .v.RH + 1 ].hh .v.LH != p ) ) 
    clobbered = true ;
    if ( clobbered ) 
    {
      printnl ( 300 ) ;
      printint ( q ) ;
      goto lab32 ;
    } 
    {register integer for_end; q = p ;for_end = p + mem [p ].hh .v.LH - 1 
    ; if ( q <= for_end) do 
      {
	if ( freearr [q ]) 
	{
	  printnl ( 301 ) ;
	  printint ( q ) ;
	  goto lab32 ;
	} 
	freearr [q ]= true ;
      } 
    while ( q++ < for_end ) ;} 
    q = p ;
    p = mem [p + 1 ].hh .v.RH ;
  } while ( ! ( p == rover ) ) ;
  lab32: ;
  p = memmin ;
  while ( p <= lomemmax ) {
      
    if ( ( mem [p ].hh .v.RH == 268435455L ) ) 
    {
      printnl ( 302 ) ;
      printint ( p ) ;
    } 
    while ( ( p <= lomemmax ) && ! freearr [p ]) incr ( p ) ;
    while ( ( p <= lomemmax ) && freearr [p ]) incr ( p ) ;
  } 
  if ( printlocs ) 
  {
    printnl ( 303 ) ;
    {register integer for_end; p = memmin ;for_end = lomemmax ; if ( p <= 
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
  {register integer for_end; p = memmin ;for_end = lomemmax ; if ( p <= 
  for_end) do 
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
  searchmem_regmem 
  integer q  ;
  {register integer for_end; q = memmin ;for_end = lomemmax ; if ( q <= 
  for_end) do 
    {
      if ( mem [q ].hh .v.RH == p ) 
      {
	printnl ( 304 ) ;
	printint ( q ) ;
	printchar ( 41 ) ;
      } 
      if ( mem [q ].hh .v.LH == p ) 
      {
	printnl ( 305 ) ;
	printint ( q ) ;
	printchar ( 41 ) ;
      } 
    } 
  while ( q++ < for_end ) ;} 
  {register integer for_end; q = himemmin ;for_end = memend ; if ( q <= 
  for_end) do 
    {
      if ( mem [q ].hh .v.RH == p ) 
      {
	printnl ( 304 ) ;
	printint ( q ) ;
	printchar ( 41 ) ;
      } 
      if ( mem [q ].hh .v.LH == p ) 
      {
	printnl ( 305 ) ;
	printint ( q ) ;
	printchar ( 41 ) ;
      } 
    } 
  while ( q++ < for_end ) ;} 
  {register integer for_end; q = 1 ;for_end = 13577 ; if ( q <= for_end) do 
    {
      if ( eqtb [q ].hh .v.RH == p ) 
      {
	printnl ( 501 ) ;
	printint ( q ) ;
	printchar ( 41 ) ;
      } 
    } 
  while ( q++ < for_end ) ;} 
  if ( saveptr > 0 ) 
  {register integer for_end; q = 0 ;for_end = saveptr - 1 ; if ( q <= 
  for_end) do 
    {
      if ( savestack [q ].hh .v.RH == p ) 
      {
	printnl ( 546 ) ;
	printint ( q ) ;
	printchar ( 41 ) ;
      } 
    } 
  while ( q++ < for_end ) ;} 
  {register integer for_end; q = 0 ;for_end = hyphsize ; if ( q <= for_end) 
  do 
    {
      if ( hyphlist [q ]== p ) 
      {
	printnl ( 935 ) ;
	printint ( q ) ;
	printchar ( 41 ) ;
      } 
    } 
  while ( q++ < for_end ) ;} 
} 
#endif /* TEXMF_DEBUG */
void 
#ifdef HAVE_PROTOTYPES
zshortdisplay ( integer p ) 
#else
zshortdisplay ( p ) 
  integer p ;
#endif
{
  shortdisplay_regmem 
  integer n  ;
  while ( p > memmin ) {
      
    if ( ( p >= himemmin ) ) 
    {
      if ( p <= memend ) 
      {
	if ( mem [p ].hh.b0 != fontinshortdisplay ) 
	{
	  if ( ( mem [p ].hh.b0 > fontmax ) ) 
	  printchar ( 42 ) ;
	  else printesc ( hash [10524 + mem [p ].hh.b0 ].v.RH ) ;
	  printchar ( 32 ) ;
	  fontinshortdisplay = mem [p ].hh.b0 ;
	} 
	print ( mem [p ].hh.b1 ) ;
      } 
    } 
    else switch ( mem [p ].hh.b0 ) 
    {case 0 : 
    case 1 : 
    case 3 : 
    case 8 : 
    case 4 : 
    case 5 : 
    case 13 : 
      print ( 306 ) ;
      break ;
    case 2 : 
      printchar ( 124 ) ;
      break ;
    case 10 : 
      if ( mem [p + 1 ].hh .v.LH != membot ) 
      printchar ( 32 ) ;
      break ;
    case 9 : 
      printchar ( 36 ) ;
      break ;
    case 6 : 
      shortdisplay ( mem [p + 1 ].hh .v.RH ) ;
      break ;
    case 7 : 
      {
	shortdisplay ( mem [p + 1 ].hh .v.LH ) ;
	shortdisplay ( mem [p + 1 ].hh .v.RH ) ;
	n = mem [p ].hh.b1 ;
	while ( n > 0 ) {
	    
	  if ( mem [p ].hh .v.RH != 0 ) 
	  p = mem [p ].hh .v.RH ;
	  decr ( n ) ;
	} 
      } 
      break ;
      default: 
      ;
      break ;
    } 
    p = mem [p ].hh .v.RH ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zprintfontandchar ( integer p ) 
#else
zprintfontandchar ( p ) 
  integer p ;
#endif
{
  printfontandchar_regmem 
  if ( p > memend ) 
  printesc ( 307 ) ;
  else {
      
    if ( ( mem [p ].hh.b0 > fontmax ) ) 
    printchar ( 42 ) ;
    else printesc ( hash [10524 + mem [p ].hh.b0 ].v.RH ) ;
    printchar ( 32 ) ;
    print ( mem [p ].hh.b1 ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zprintmark ( integer p ) 
#else
zprintmark ( p ) 
  integer p ;
#endif
{
  printmark_regmem 
  printchar ( 123 ) ;
  if ( ( p < himemmin ) || ( p > memend ) ) 
  printesc ( 307 ) ;
  else showtokenlist ( mem [p ].hh .v.RH , 0 , maxprintline - 10 ) ;
  printchar ( 125 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintruledimen ( scaled d ) 
#else
zprintruledimen ( d ) 
  scaled d ;
#endif
{
  printruledimen_regmem 
  if ( ( d == -1073741824L ) ) 
  printchar ( 42 ) ;
  else printscaled ( d ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintglue ( scaled d , integer order , strnumber s ) 
#else
zprintglue ( d , order , s ) 
  scaled d ;
  integer order ;
  strnumber s ;
#endif
{
  printglue_regmem 
  printscaled ( d ) ;
  if ( ( order < 0 ) || ( order > 3 ) ) 
  print ( 308 ) ;
  else if ( order > 0 ) 
  {
    print ( 309 ) ;
    while ( order > 1 ) {
	
      printchar ( 108 ) ;
      decr ( order ) ;
    } 
  } 
  else if ( s != 0 ) 
  print ( s ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintspec ( integer p , strnumber s ) 
#else
zprintspec ( p , s ) 
  integer p ;
  strnumber s ;
#endif
{
  printspec_regmem 
  if ( ( p < memmin ) || ( p >= lomemmax ) ) 
  printchar ( 42 ) ;
  else {
      
    printscaled ( mem [p + 1 ].cint ) ;
    if ( s != 0 ) 
    print ( s ) ;
    if ( mem [p + 2 ].cint != 0 ) 
    {
      print ( 310 ) ;
      printglue ( mem [p + 2 ].cint , mem [p ].hh.b0 , s ) ;
    } 
    if ( mem [p + 3 ].cint != 0 ) 
    {
      print ( 311 ) ;
      printglue ( mem [p + 3 ].cint , mem [p ].hh.b1 , s ) ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zprintfamandchar ( halfword p ) 
#else
zprintfamandchar ( p ) 
  halfword p ;
#endif
{
  printfamandchar_regmem 
  printesc ( 461 ) ;
  printint ( mem [p ].hh.b0 ) ;
  printchar ( 32 ) ;
  print ( mem [p ].hh.b1 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintdelimiter ( halfword p ) 
#else
zprintdelimiter ( p ) 
  halfword p ;
#endif
{
  printdelimiter_regmem 
  integer a  ;
  a = mem [p ].qqqq .b0 * 256 + mem [p ].qqqq .b1 ;
  a = a * 4096 + mem [p ].qqqq .b2 * 256 + mem [p ].qqqq .b3 ;
  if ( a < 0 ) 
  printint ( a ) ;
  else printhex ( a ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintsubsidiarydata ( halfword p , ASCIIcode c ) 
#else
zprintsubsidiarydata ( p , c ) 
  halfword p ;
  ASCIIcode c ;
#endif
{
  printsubsidiarydata_regmem 
  if ( ( poolptr - strstart [strptr ]) >= depththreshold ) 
  {
    if ( mem [p ].hh .v.RH != 0 ) 
    print ( 312 ) ;
  } 
  else {
      
    {
      strpool [poolptr ]= c ;
      incr ( poolptr ) ;
    } 
    tempptr = p ;
    switch ( mem [p ].hh .v.RH ) 
    {case 1 : 
      {
	println () ;
	printcurrentstring () ;
	printfamandchar ( p ) ;
      } 
      break ;
    case 2 : 
      showinfo () ;
      break ;
    case 3 : 
      if ( mem [p ].hh .v.LH == 0 ) 
      {
	println () ;
	printcurrentstring () ;
	print ( 855 ) ;
      } 
      else showinfo () ;
      break ;
      default: 
      ;
      break ;
    } 
    decr ( poolptr ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zprintstyle ( integer c ) 
#else
zprintstyle ( c ) 
  integer c ;
#endif
{
  printstyle_regmem 
  switch ( c / 2 ) 
  {case 0 : 
    printesc ( 856 ) ;
    break ;
  case 1 : 
    printesc ( 857 ) ;
    break ;
  case 2 : 
    printesc ( 858 ) ;
    break ;
  case 3 : 
    printesc ( 859 ) ;
    break ;
    default: 
    print ( 860 ) ;
    break ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zprintskipparam ( integer n ) 
#else
zprintskipparam ( n ) 
  integer n ;
#endif
{
  printskipparam_regmem 
  switch ( n ) 
  {case 0 : 
    printesc ( 373 ) ;
    break ;
  case 1 : 
    printesc ( 374 ) ;
    break ;
  case 2 : 
    printesc ( 375 ) ;
    break ;
  case 3 : 
    printesc ( 376 ) ;
    break ;
  case 4 : 
    printesc ( 377 ) ;
    break ;
  case 5 : 
    printesc ( 378 ) ;
    break ;
  case 6 : 
    printesc ( 379 ) ;
    break ;
  case 7 : 
    printesc ( 380 ) ;
    break ;
  case 8 : 
    printesc ( 381 ) ;
    break ;
  case 9 : 
    printesc ( 382 ) ;
    break ;
  case 10 : 
    printesc ( 383 ) ;
    break ;
  case 11 : 
    printesc ( 384 ) ;
    break ;
  case 12 : 
    printesc ( 385 ) ;
    break ;
  case 13 : 
    printesc ( 386 ) ;
    break ;
  case 14 : 
    printesc ( 387 ) ;
    break ;
  case 15 : 
    printesc ( 388 ) ;
    break ;
  case 16 : 
    printesc ( 389 ) ;
    break ;
  case 17 : 
    printesc ( 390 ) ;
    break ;
    default: 
    print ( 391 ) ;
    break ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zshownodelist ( integer p ) 
#else
zshownodelist ( p ) 
  integer p ;
#endif
{
  /* 10 */ shownodelist_regmem 
  integer n  ;
  real g  ;
  if ( ( poolptr - strstart [strptr ]) > depththreshold ) 
  {
    if ( p > 0 ) 
    print ( 312 ) ;
    return ;
  } 
  n = 0 ;
  while ( p > memmin ) {
      
    println () ;
    printcurrentstring () ;
    if ( p > memend ) 
    {
      print ( 313 ) ;
      return ;
    } 
    incr ( n ) ;
    if ( n > breadthmax ) 
    {
      print ( 314 ) ;
      return ;
    } 
    if ( ( p >= himemmin ) ) 
    printfontandchar ( p ) ;
    else switch ( mem [p ].hh.b0 ) 
    {case 0 : 
    case 1 : 
    case 13 : 
      {
	if ( mem [p ].hh.b0 == 0 ) 
	printesc ( 104 ) ;
	else if ( mem [p ].hh.b0 == 1 ) 
	printesc ( 118 ) ;
	else printesc ( 316 ) ;
	print ( 317 ) ;
	printscaled ( mem [p + 3 ].cint ) ;
	printchar ( 43 ) ;
	printscaled ( mem [p + 2 ].cint ) ;
	print ( 318 ) ;
	printscaled ( mem [p + 1 ].cint ) ;
	if ( mem [p ].hh.b0 == 13 ) 
	{
	  if ( mem [p ].hh.b1 != 0 ) 
	  {
	    print ( 284 ) ;
	    printint ( mem [p ].hh.b1 + 1 ) ;
	    print ( 320 ) ;
	  } 
	  if ( mem [p + 6 ].cint != 0 ) 
	  {
	    print ( 321 ) ;
	    printglue ( mem [p + 6 ].cint , mem [p + 5 ].hh.b1 , 0 ) ;
	  } 
	  if ( mem [p + 4 ].cint != 0 ) 
	  {
	    print ( 322 ) ;
	    printglue ( mem [p + 4 ].cint , mem [p + 5 ].hh.b0 , 0 ) ;
	  } 
	} 
	else {
	    
	  g = mem [p + 6 ].gr ;
	  if ( ( g != 0.0 ) && ( mem [p + 5 ].hh.b0 != 0 ) ) 
	  {
	    print ( 323 ) ;
	    if ( mem [p + 5 ].hh.b0 == 2 ) 
	    print ( 324 ) ;
	    if ( fabs ( g ) > 20000.0 ) 
	    {
	      if ( g > 0.0 ) 
	      printchar ( 62 ) ;
	      else print ( 325 ) ;
	      printglue ( 20000 * 65536L , mem [p + 5 ].hh.b1 , 0 ) ;
	    } 
	    else printglue ( round ( 65536L * g ) , mem [p + 5 ].hh.b1 , 0 ) 
	    ;
	  } 
	  if ( mem [p + 4 ].cint != 0 ) 
	  {
	    print ( 319 ) ;
	    printscaled ( mem [p + 4 ].cint ) ;
	  } 
	} 
	{
	  {
	    strpool [poolptr ]= 46 ;
	    incr ( poolptr ) ;
	  } 
	  shownodelist ( mem [p + 5 ].hh .v.RH ) ;
	  decr ( poolptr ) ;
	} 
      } 
      break ;
    case 2 : 
      {
	printesc ( 326 ) ;
	printruledimen ( mem [p + 3 ].cint ) ;
	printchar ( 43 ) ;
	printruledimen ( mem [p + 2 ].cint ) ;
	print ( 318 ) ;
	printruledimen ( mem [p + 1 ].cint ) ;
      } 
      break ;
    case 3 : 
      {
	printesc ( 327 ) ;
	printint ( mem [p ].hh.b1 ) ;
	print ( 328 ) ;
	printscaled ( mem [p + 3 ].cint ) ;
	print ( 329 ) ;
	printspec ( mem [p + 4 ].hh .v.RH , 0 ) ;
	printchar ( 44 ) ;
	printscaled ( mem [p + 2 ].cint ) ;
	print ( 330 ) ;
	printint ( mem [p + 1 ].cint ) ;
	{
	  {
	    strpool [poolptr ]= 46 ;
	    incr ( poolptr ) ;
	  } 
	  shownodelist ( mem [p + 4 ].hh .v.LH ) ;
	  decr ( poolptr ) ;
	} 
      } 
      break ;
    case 8 : 
      switch ( mem [p ].hh.b1 ) 
      {case 0 : 
	{
	  printwritewhatsit ( 1283 , p ) ;
	  printchar ( 61 ) ;
	  printfilename ( mem [p + 1 ].hh .v.RH , mem [p + 2 ].hh .v.LH , 
	  mem [p + 2 ].hh .v.RH ) ;
	} 
	break ;
      case 1 : 
	{
	  printwritewhatsit ( 593 , p ) ;
	  printmark ( mem [p + 1 ].hh .v.RH ) ;
	} 
	break ;
      case 2 : 
	printwritewhatsit ( 1284 , p ) ;
	break ;
      case 3 : 
	{
	  printesc ( 1285 ) ;
	  printmark ( mem [p + 1 ].hh .v.RH ) ;
	} 
	break ;
      case 4 : 
	{
	  printesc ( 1287 ) ;
	  printint ( mem [p + 1 ].hh .v.RH ) ;
	  print ( 1290 ) ;
	  printint ( mem [p + 1 ].hh.b0 ) ;
	  printchar ( 44 ) ;
	  printint ( mem [p + 1 ].hh.b1 ) ;
	  printchar ( 41 ) ;
	} 
	break ;
	default: 
	print ( 1291 ) ;
	break ;
      } 
      break ;
    case 10 : 
      if ( mem [p ].hh.b1 >= 100 ) 
      {
	printesc ( 335 ) ;
	if ( mem [p ].hh.b1 == 101 ) 
	printchar ( 99 ) ;
	else if ( mem [p ].hh.b1 == 102 ) 
	printchar ( 120 ) ;
	print ( 336 ) ;
	printspec ( mem [p + 1 ].hh .v.LH , 0 ) ;
	{
	  {
	    strpool [poolptr ]= 46 ;
	    incr ( poolptr ) ;
	  } 
	  shownodelist ( mem [p + 1 ].hh .v.RH ) ;
	  decr ( poolptr ) ;
	} 
      } 
      else {
	  
	printesc ( 331 ) ;
	if ( mem [p ].hh.b1 != 0 ) 
	{
	  printchar ( 40 ) ;
	  if ( mem [p ].hh.b1 < 98 ) 
	  printskipparam ( mem [p ].hh.b1 - 1 ) ;
	  else if ( mem [p ].hh.b1 == 98 ) 
	  printesc ( 332 ) ;
	  else printesc ( 333 ) ;
	  printchar ( 41 ) ;
	} 
	if ( mem [p ].hh.b1 != 98 ) 
	{
	  printchar ( 32 ) ;
	  if ( mem [p ].hh.b1 < 98 ) 
	  printspec ( mem [p + 1 ].hh .v.LH , 0 ) ;
	  else printspec ( mem [p + 1 ].hh .v.LH , 334 ) ;
	} 
      } 
      break ;
    case 11 : 
      if ( mem [p ].hh.b1 != 99 ) 
      {
	printesc ( 337 ) ;
	if ( mem [p ].hh.b1 != 0 ) 
	printchar ( 32 ) ;
	printscaled ( mem [p + 1 ].cint ) ;
	if ( mem [p ].hh.b1 == 2 ) 
	print ( 338 ) ;
      } 
      else {
	  
	printesc ( 339 ) ;
	printscaled ( mem [p + 1 ].cint ) ;
	print ( 334 ) ;
      } 
      break ;
    case 9 : 
      {
	printesc ( 340 ) ;
	if ( mem [p ].hh.b1 == 0 ) 
	print ( 341 ) ;
	else print ( 342 ) ;
	if ( mem [p + 1 ].cint != 0 ) 
	{
	  print ( 343 ) ;
	  printscaled ( mem [p + 1 ].cint ) ;
	} 
      } 
      break ;
    case 6 : 
      {
	printfontandchar ( p + 1 ) ;
	print ( 344 ) ;
	if ( mem [p ].hh.b1 > 1 ) 
	printchar ( 124 ) ;
	fontinshortdisplay = mem [p + 1 ].hh.b0 ;
	shortdisplay ( mem [p + 1 ].hh .v.RH ) ;
	if ( odd ( mem [p ].hh.b1 ) ) 
	printchar ( 124 ) ;
	printchar ( 41 ) ;
      } 
      break ;
    case 12 : 
      {
	printesc ( 345 ) ;
	printint ( mem [p + 1 ].cint ) ;
      } 
      break ;
    case 7 : 
      {
	printesc ( 346 ) ;
	if ( mem [p ].hh.b1 > 0 ) 
	{
	  print ( 347 ) ;
	  printint ( mem [p ].hh.b1 ) ;
	} 
	{
	  {
	    strpool [poolptr ]= 46 ;
	    incr ( poolptr ) ;
	  } 
	  shownodelist ( mem [p + 1 ].hh .v.LH ) ;
	  decr ( poolptr ) ;
	} 
	{
	  strpool [poolptr ]= 124 ;
	  incr ( poolptr ) ;
	} 
	shownodelist ( mem [p + 1 ].hh .v.RH ) ;
	decr ( poolptr ) ;
      } 
      break ;
    case 4 : 
      {
	printesc ( 348 ) ;
	printmark ( mem [p + 1 ].cint ) ;
      } 
      break ;
    case 5 : 
      {
	printesc ( 349 ) ;
	{
	  {
	    strpool [poolptr ]= 46 ;
	    incr ( poolptr ) ;
	  } 
	  shownodelist ( mem [p + 1 ].cint ) ;
	  decr ( poolptr ) ;
	} 
      } 
      break ;
    case 14 : 
      printstyle ( mem [p ].hh.b1 ) ;
      break ;
    case 15 : 
      {
	printesc ( 525 ) ;
	{
	  strpool [poolptr ]= 68 ;
	  incr ( poolptr ) ;
	} 
	shownodelist ( mem [p + 1 ].hh .v.LH ) ;
	decr ( poolptr ) ;
	{
	  strpool [poolptr ]= 84 ;
	  incr ( poolptr ) ;
	} 
	shownodelist ( mem [p + 1 ].hh .v.RH ) ;
	decr ( poolptr ) ;
	{
	  strpool [poolptr ]= 83 ;
	  incr ( poolptr ) ;
	} 
	shownodelist ( mem [p + 2 ].hh .v.LH ) ;
	decr ( poolptr ) ;
	{
	  strpool [poolptr ]= 115 ;
	  incr ( poolptr ) ;
	} 
	shownodelist ( mem [p + 2 ].hh .v.RH ) ;
	decr ( poolptr ) ;
      } 
      break ;
    case 16 : 
    case 17 : 
    case 18 : 
    case 19 : 
    case 20 : 
    case 21 : 
    case 22 : 
    case 23 : 
    case 24 : 
    case 27 : 
    case 26 : 
    case 29 : 
    case 28 : 
    case 30 : 
    case 31 : 
      {
	switch ( mem [p ].hh.b0 ) 
	{case 16 : 
	  printesc ( 861 ) ;
	  break ;
	case 17 : 
	  printesc ( 862 ) ;
	  break ;
	case 18 : 
	  printesc ( 863 ) ;
	  break ;
	case 19 : 
	  printesc ( 864 ) ;
	  break ;
	case 20 : 
	  printesc ( 865 ) ;
	  break ;
	case 21 : 
	  printesc ( 866 ) ;
	  break ;
	case 22 : 
	  printesc ( 867 ) ;
	  break ;
	case 23 : 
	  printesc ( 868 ) ;
	  break ;
	case 27 : 
	  printesc ( 869 ) ;
	  break ;
	case 26 : 
	  printesc ( 870 ) ;
	  break ;
	case 29 : 
	  printesc ( 539 ) ;
	  break ;
	case 24 : 
	  {
	    printesc ( 533 ) ;
	    printdelimiter ( p + 4 ) ;
	  } 
	  break ;
	case 28 : 
	  {
	    printesc ( 508 ) ;
	    printfamandchar ( p + 4 ) ;
	  } 
	  break ;
	case 30 : 
	  {
	    printesc ( 871 ) ;
	    printdelimiter ( p + 1 ) ;
	  } 
	  break ;
	case 31 : 
	  {
	    printesc ( 872 ) ;
	    printdelimiter ( p + 1 ) ;
	  } 
	  break ;
	} 
	if ( mem [p ].hh.b1 != 0 ) 
	if ( mem [p ].hh.b1 == 1 ) 
	printesc ( 873 ) ;
	else printesc ( 874 ) ;
	if ( mem [p ].hh.b0 < 30 ) 
	printsubsidiarydata ( p + 1 , 46 ) ;
	printsubsidiarydata ( p + 2 , 94 ) ;
	printsubsidiarydata ( p + 3 , 95 ) ;
      } 
      break ;
    case 25 : 
      {
	printesc ( 875 ) ;
	if ( mem [p + 1 ].cint == 1073741824L ) 
	print ( 876 ) ;
	else printscaled ( mem [p + 1 ].cint ) ;
	if ( ( mem [p + 4 ].qqqq .b0 != 0 ) || ( mem [p + 4 ].qqqq .b1 != 
	0 ) || ( mem [p + 4 ].qqqq .b2 != 0 ) || ( mem [p + 4 ].qqqq .b3 
	!= 0 ) ) 
	{
	  print ( 877 ) ;
	  printdelimiter ( p + 4 ) ;
	} 
	if ( ( mem [p + 5 ].qqqq .b0 != 0 ) || ( mem [p + 5 ].qqqq .b1 != 
	0 ) || ( mem [p + 5 ].qqqq .b2 != 0 ) || ( mem [p + 5 ].qqqq .b3 
	!= 0 ) ) 
	{
	  print ( 878 ) ;
	  printdelimiter ( p + 5 ) ;
	} 
	printsubsidiarydata ( p + 2 , 92 ) ;
	printsubsidiarydata ( p + 3 , 47 ) ;
      } 
      break ;
      default: 
      print ( 315 ) ;
      break ;
    } 
    p = mem [p ].hh .v.RH ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zshowbox ( halfword p ) 
#else
zshowbox ( p ) 
  halfword p ;
#endif
{
  showbox_regmem 
  depththreshold = eqtb [15188 ].cint ;
  breadthmax = eqtb [15187 ].cint ;
  if ( breadthmax <= 0 ) 
  breadthmax = 5 ;
  if ( poolptr + depththreshold >= poolsize ) 
  depththreshold = poolsize - poolptr - 1 ;
  shownodelist ( p ) ;
  println () ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdeletetokenref ( halfword p ) 
#else
zdeletetokenref ( p ) 
  halfword p ;
#endif
{
  deletetokenref_regmem 
  if ( mem [p ].hh .v.LH == 0 ) 
  flushlist ( p ) ;
  else decr ( mem [p ].hh .v.LH ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdeleteglueref ( halfword p ) 
#else
zdeleteglueref ( p ) 
  halfword p ;
#endif
{
  deleteglueref_regmem 
  if ( mem [p ].hh .v.RH == 0 ) 
  freenode ( p , 4 ) ;
  else decr ( mem [p ].hh .v.RH ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zflushnodelist ( halfword p ) 
#else
zflushnodelist ( p ) 
  halfword p ;
#endif
{
  /* 30 */ flushnodelist_regmem 
  halfword q  ;
  while ( p != 0 ) {
      
    q = mem [p ].hh .v.RH ;
    if ( ( p >= himemmin ) ) 
    {
      mem [p ].hh .v.RH = avail ;
      avail = p ;
	;
#ifdef STAT
      decr ( dynused ) ;
#endif /* STAT */
    } 
    else {
	
      switch ( mem [p ].hh.b0 ) 
      {case 0 : 
      case 1 : 
      case 13 : 
	{
	  flushnodelist ( mem [p + 5 ].hh .v.RH ) ;
	  freenode ( p , 7 ) ;
	  goto lab30 ;
	} 
	break ;
      case 2 : 
	{
	  freenode ( p , 4 ) ;
	  goto lab30 ;
	} 
	break ;
      case 3 : 
	{
	  flushnodelist ( mem [p + 4 ].hh .v.LH ) ;
	  deleteglueref ( mem [p + 4 ].hh .v.RH ) ;
	  freenode ( p , 5 ) ;
	  goto lab30 ;
	} 
	break ;
      case 8 : 
	{
	  switch ( mem [p ].hh.b1 ) 
	  {case 0 : 
	    freenode ( p , 3 ) ;
	    break ;
	  case 1 : 
	  case 3 : 
	    {
	      deletetokenref ( mem [p + 1 ].hh .v.RH ) ;
	      freenode ( p , 2 ) ;
	      goto lab30 ;
	    } 
	    break ;
	  case 2 : 
	  case 4 : 
	    freenode ( p , 2 ) ;
	    break ;
	    default: 
	    confusion ( 1293 ) ;
	    break ;
	  } 
	  goto lab30 ;
	} 
	break ;
      case 10 : 
	{
	  {
	    if ( mem [mem [p + 1 ].hh .v.LH ].hh .v.RH == 0 ) 
	    freenode ( mem [p + 1 ].hh .v.LH , 4 ) ;
	    else decr ( mem [mem [p + 1 ].hh .v.LH ].hh .v.RH ) ;
	  } 
	  if ( mem [p + 1 ].hh .v.RH != 0 ) 
	  flushnodelist ( mem [p + 1 ].hh .v.RH ) ;
	} 
	break ;
      case 11 : 
      case 9 : 
      case 12 : 
	;
	break ;
      case 6 : 
	flushnodelist ( mem [p + 1 ].hh .v.RH ) ;
	break ;
      case 4 : 
	deletetokenref ( mem [p + 1 ].cint ) ;
	break ;
      case 7 : 
	{
	  flushnodelist ( mem [p + 1 ].hh .v.LH ) ;
	  flushnodelist ( mem [p + 1 ].hh .v.RH ) ;
	} 
	break ;
      case 5 : 
	flushnodelist ( mem [p + 1 ].cint ) ;
	break ;
      case 14 : 
	{
	  freenode ( p , 3 ) ;
	  goto lab30 ;
	} 
	break ;
      case 15 : 
	{
	  flushnodelist ( mem [p + 1 ].hh .v.LH ) ;
	  flushnodelist ( mem [p + 1 ].hh .v.RH ) ;
	  flushnodelist ( mem [p + 2 ].hh .v.LH ) ;
	  flushnodelist ( mem [p + 2 ].hh .v.RH ) ;
	  freenode ( p , 3 ) ;
	  goto lab30 ;
	} 
	break ;
      case 16 : 
      case 17 : 
      case 18 : 
      case 19 : 
      case 20 : 
      case 21 : 
      case 22 : 
      case 23 : 
      case 24 : 
      case 27 : 
      case 26 : 
      case 29 : 
      case 28 : 
	{
	  if ( mem [p + 1 ].hh .v.RH >= 2 ) 
	  flushnodelist ( mem [p + 1 ].hh .v.LH ) ;
	  if ( mem [p + 2 ].hh .v.RH >= 2 ) 
	  flushnodelist ( mem [p + 2 ].hh .v.LH ) ;
	  if ( mem [p + 3 ].hh .v.RH >= 2 ) 
	  flushnodelist ( mem [p + 3 ].hh .v.LH ) ;
	  if ( mem [p ].hh.b0 == 24 ) 
	  freenode ( p , 5 ) ;
	  else if ( mem [p ].hh.b0 == 28 ) 
	  freenode ( p , 5 ) ;
	  else freenode ( p , 4 ) ;
	  goto lab30 ;
	} 
	break ;
      case 30 : 
      case 31 : 
	{
	  freenode ( p , 4 ) ;
	  goto lab30 ;
	} 
	break ;
      case 25 : 
	{
	  flushnodelist ( mem [p + 2 ].hh .v.LH ) ;
	  flushnodelist ( mem [p + 3 ].hh .v.LH ) ;
	  freenode ( p , 6 ) ;
	  goto lab30 ;
	} 
	break ;
	default: 
	confusion ( 350 ) ;
	break ;
      } 
      freenode ( p , 2 ) ;
      lab30: ;
    } 
    p = q ;
  } 
} 
halfword 
#ifdef HAVE_PROTOTYPES
zcopynodelist ( halfword p ) 
#else
zcopynodelist ( p ) 
  halfword p ;
#endif
{
  register halfword Result; copynodelist_regmem 
  halfword h  ;
  halfword q  ;
  halfword r  ;
  char words  ;
  h = getavail () ;
  q = h ;
  while ( p != 0 ) {
      
    words = 1 ;
    if ( ( p >= himemmin ) ) 
    r = getavail () ;
    else switch ( mem [p ].hh.b0 ) 
    {case 0 : 
    case 1 : 
    case 13 : 
      {
	r = getnode ( 7 ) ;
	mem [r + 6 ]= mem [p + 6 ];
	mem [r + 5 ]= mem [p + 5 ];
	mem [r + 5 ].hh .v.RH = copynodelist ( mem [p + 5 ].hh .v.RH ) ;
	words = 5 ;
      } 
      break ;
    case 2 : 
      {
	r = getnode ( 4 ) ;
	words = 4 ;
      } 
      break ;
    case 3 : 
      {
	r = getnode ( 5 ) ;
	mem [r + 4 ]= mem [p + 4 ];
	incr ( mem [mem [p + 4 ].hh .v.RH ].hh .v.RH ) ;
	mem [r + 4 ].hh .v.LH = copynodelist ( mem [p + 4 ].hh .v.LH ) ;
	words = 4 ;
      } 
      break ;
    case 8 : 
      switch ( mem [p ].hh.b1 ) 
      {case 0 : 
	{
	  r = getnode ( 3 ) ;
	  words = 3 ;
	} 
	break ;
      case 1 : 
      case 3 : 
	{
	  r = getnode ( 2 ) ;
	  incr ( mem [mem [p + 1 ].hh .v.RH ].hh .v.LH ) ;
	  words = 2 ;
	} 
	break ;
      case 2 : 
      case 4 : 
	{
	  r = getnode ( 2 ) ;
	  words = 2 ;
	} 
	break ;
	default: 
	confusion ( 1292 ) ;
	break ;
      } 
      break ;
    case 10 : 
      {
	r = getnode ( 2 ) ;
	incr ( mem [mem [p + 1 ].hh .v.LH ].hh .v.RH ) ;
	mem [r + 1 ].hh .v.LH = mem [p + 1 ].hh .v.LH ;
	mem [r + 1 ].hh .v.RH = copynodelist ( mem [p + 1 ].hh .v.RH ) ;
      } 
      break ;
    case 11 : 
    case 9 : 
    case 12 : 
      {
	r = getnode ( 2 ) ;
	words = 2 ;
      } 
      break ;
    case 6 : 
      {
	r = getnode ( 2 ) ;
	mem [r + 1 ]= mem [p + 1 ];
	mem [r + 1 ].hh .v.RH = copynodelist ( mem [p + 1 ].hh .v.RH ) ;
      } 
      break ;
    case 7 : 
      {
	r = getnode ( 2 ) ;
	mem [r + 1 ].hh .v.LH = copynodelist ( mem [p + 1 ].hh .v.LH ) ;
	mem [r + 1 ].hh .v.RH = copynodelist ( mem [p + 1 ].hh .v.RH ) ;
      } 
      break ;
    case 4 : 
      {
	r = getnode ( 2 ) ;
	incr ( mem [mem [p + 1 ].cint ].hh .v.LH ) ;
	words = 2 ;
      } 
      break ;
    case 5 : 
      {
	r = getnode ( 2 ) ;
	mem [r + 1 ].cint = copynodelist ( mem [p + 1 ].cint ) ;
      } 
      break ;
      default: 
      confusion ( 351 ) ;
      break ;
    } 
    while ( words > 0 ) {
	
      decr ( words ) ;
      mem [r + words ]= mem [p + words ];
    } 
    mem [q ].hh .v.RH = r ;
    q = r ;
    p = mem [p ].hh .v.RH ;
  } 
  mem [q ].hh .v.RH = 0 ;
  q = mem [h ].hh .v.RH ;
  {
    mem [h ].hh .v.RH = avail ;
    avail = h ;
	;
#ifdef STAT
    decr ( dynused ) ;
#endif /* STAT */
  } 
  Result = q ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintmode ( integer m ) 
#else
zprintmode ( m ) 
  integer m ;
#endif
{
  printmode_regmem 
  if ( m > 0 ) 
  switch ( m / ( 101 ) ) 
  {case 0 : 
    print ( 352 ) ;
    break ;
  case 1 : 
    print ( 353 ) ;
    break ;
  case 2 : 
    print ( 354 ) ;
    break ;
  } 
  else if ( m == 0 ) 
  print ( 355 ) ;
  else switch ( ( - (integer) m ) / ( 101 ) ) 
  {case 0 : 
    print ( 356 ) ;
    break ;
  case 1 : 
    print ( 357 ) ;
    break ;
  case 2 : 
    print ( 340 ) ;
    break ;
  } 
  print ( 358 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
pushnest ( void ) 
#else
pushnest ( ) 
#endif
{
  pushnest_regmem 
  if ( nestptr > maxneststack ) 
  {
    maxneststack = nestptr ;
    if ( nestptr == nestsize ) 
    overflow ( 359 , nestsize ) ;
  } 
  nest [nestptr ]= curlist ;
  incr ( nestptr ) ;
  curlist .headfield = getavail () ;
  curlist .tailfield = curlist .headfield ;
  curlist .pgfield = 0 ;
  curlist .mlfield = line ;
} 
void 
#ifdef HAVE_PROTOTYPES
popnest ( void ) 
#else
popnest ( ) 
#endif
{
  popnest_regmem 
  {
    mem [curlist .headfield ].hh .v.RH = avail ;
    avail = curlist .headfield ;
	;
#ifdef STAT
    decr ( dynused ) ;
#endif /* STAT */
  } 
  decr ( nestptr ) ;
  curlist = nest [nestptr ];
} 
void 
#ifdef HAVE_PROTOTYPES
showactivities ( void ) 
#else
showactivities ( ) 
#endif
{
  showactivities_regmem 
  integer p  ;
  short m  ;
  memoryword a  ;
  halfword q, r  ;
  integer t  ;
  nest [nestptr ]= curlist ;
  printnl ( 335 ) ;
  println () ;
  {register integer for_end; p = nestptr ;for_end = 0 ; if ( p >= for_end) 
  do 
    {
      m = nest [p ].modefield ;
      a = nest [p ].auxfield ;
      printnl ( 360 ) ;
      printmode ( m ) ;
      print ( 361 ) ;
      printint ( abs ( nest [p ].mlfield ) ) ;
      if ( m == 102 ) 
      if ( nest [p ].pgfield != 8585216L ) 
      {
	print ( 362 ) ;
	printint ( nest [p ].pgfield % 65536L ) ;
	print ( 363 ) ;
	printint ( nest [p ].pgfield / 4194304L ) ;
	printchar ( 44 ) ;
	printint ( ( nest [p ].pgfield / 65536L ) % 64 ) ;
	printchar ( 41 ) ;
      } 
      if ( nest [p ].mlfield < 0 ) 
      print ( 364 ) ;
      if ( p == 0 ) 
      {
	if ( memtop - 2 != pagetail ) 
	{
	  printnl ( 975 ) ;
	  if ( outputactive ) 
	  print ( 976 ) ;
	  showbox ( mem [memtop - 2 ].hh .v.RH ) ;
	  if ( pagecontents > 0 ) 
	  {
	    printnl ( 977 ) ;
	    printtotals () ;
	    printnl ( 978 ) ;
	    printscaled ( pagesofar [0 ]) ;
	    r = mem [memtop ].hh .v.RH ;
	    while ( r != memtop ) {
		
	      println () ;
	      printesc ( 327 ) ;
	      t = mem [r ].hh.b1 ;
	      printint ( t ) ;
	      print ( 979 ) ;
	      t = xovern ( mem [r + 3 ].cint , 1000 ) * eqtb [15221 + t ]
	      .cint ;
	      printscaled ( t ) ;
	      if ( mem [r ].hh.b0 == 1 ) 
	      {
		q = memtop - 2 ;
		t = 0 ;
		do {
		    q = mem [q ].hh .v.RH ;
		  if ( ( mem [q ].hh.b0 == 3 ) && ( mem [q ].hh.b1 == mem 
		  [r ].hh.b1 ) ) 
		  incr ( t ) ;
		} while ( ! ( q == mem [r + 1 ].hh .v.LH ) ) ;
		print ( 980 ) ;
		printint ( t ) ;
		print ( 981 ) ;
	      } 
	      r = mem [r ].hh .v.RH ;
	    } 
	  } 
	} 
	if ( mem [memtop - 1 ].hh .v.RH != 0 ) 
	printnl ( 365 ) ;
      } 
      showbox ( mem [nest [p ].headfield ].hh .v.RH ) ;
      switch ( abs ( m ) / ( 101 ) ) 
      {case 0 : 
	{
	  printnl ( 366 ) ;
	  if ( a .cint <= -65536000L ) 
	  print ( 367 ) ;
	  else printscaled ( a .cint ) ;
	  if ( nest [p ].pgfield != 0 ) 
	  {
	    print ( 368 ) ;
	    printint ( nest [p ].pgfield ) ;
	    print ( 369 ) ;
	    if ( nest [p ].pgfield != 1 ) 
	    printchar ( 115 ) ;
	  } 
	} 
	break ;
      case 1 : 
	{
	  printnl ( 370 ) ;
	  printint ( a .hh .v.LH ) ;
	  if ( m > 0 ) 
	  if ( a .hh .v.RH > 0 ) 
	  {
	    print ( 371 ) ;
	    printint ( a .hh .v.RH ) ;
	  } 
	} 
	break ;
      case 2 : 
	if ( a .cint != 0 ) 
	{
	  print ( 372 ) ;
	  showbox ( a .cint ) ;
	} 
	break ;
      } 
    } 
  while ( p-- > for_end ) ;} 
} 
void 
#ifdef HAVE_PROTOTYPES
zprintparam ( integer n ) 
#else
zprintparam ( n ) 
  integer n ;
#endif
{
  printparam_regmem 
  switch ( n ) 
  {case 0 : 
    printesc ( 417 ) ;
    break ;
  case 1 : 
    printesc ( 418 ) ;
    break ;
  case 2 : 
    printesc ( 419 ) ;
    break ;
  case 3 : 
    printesc ( 420 ) ;
    break ;
  case 4 : 
    printesc ( 421 ) ;
    break ;
  case 5 : 
    printesc ( 422 ) ;
    break ;
  case 6 : 
    printesc ( 423 ) ;
    break ;
  case 7 : 
    printesc ( 424 ) ;
    break ;
  case 8 : 
    printesc ( 425 ) ;
    break ;
  case 9 : 
    printesc ( 426 ) ;
    break ;
  case 10 : 
    printesc ( 427 ) ;
    break ;
  case 11 : 
    printesc ( 428 ) ;
    break ;
  case 12 : 
    printesc ( 429 ) ;
    break ;
  case 13 : 
    printesc ( 430 ) ;
    break ;
  case 14 : 
    printesc ( 431 ) ;
    break ;
  case 15 : 
    printesc ( 432 ) ;
    break ;
  case 16 : 
    printesc ( 433 ) ;
    break ;
  case 17 : 
    printesc ( 434 ) ;
    break ;
  case 18 : 
    printesc ( 435 ) ;
    break ;
  case 19 : 
    printesc ( 436 ) ;
    break ;
  case 20 : 
    printesc ( 437 ) ;
    break ;
  case 21 : 
    printesc ( 438 ) ;
    break ;
  case 22 : 
    printesc ( 439 ) ;
    break ;
  case 23 : 
    printesc ( 440 ) ;
    break ;
  case 24 : 
    printesc ( 441 ) ;
    break ;
  case 25 : 
    printesc ( 442 ) ;
    break ;
  case 26 : 
    printesc ( 443 ) ;
    break ;
  case 27 : 
    printesc ( 444 ) ;
    break ;
  case 28 : 
    printesc ( 445 ) ;
    break ;
  case 29 : 
    printesc ( 446 ) ;
    break ;
  case 30 : 
    printesc ( 447 ) ;
    break ;
  case 31 : 
    printesc ( 448 ) ;
    break ;
  case 32 : 
    printesc ( 449 ) ;
    break ;
  case 33 : 
    printesc ( 450 ) ;
    break ;
  case 34 : 
    printesc ( 451 ) ;
    break ;
  case 35 : 
    printesc ( 452 ) ;
    break ;
  case 36 : 
    printesc ( 453 ) ;
    break ;
  case 37 : 
    printesc ( 454 ) ;
    break ;
  case 38 : 
    printesc ( 455 ) ;
    break ;
  case 39 : 
    printesc ( 456 ) ;
    break ;
  case 40 : 
    printesc ( 457 ) ;
    break ;
  case 41 : 
    printesc ( 458 ) ;
    break ;
  case 42 : 
    printesc ( 459 ) ;
    break ;
  case 43 : 
    printesc ( 460 ) ;
    break ;
  case 44 : 
    printesc ( 461 ) ;
    break ;
  case 45 : 
    printesc ( 462 ) ;
    break ;
  case 46 : 
    printesc ( 463 ) ;
    break ;
  case 47 : 
    printesc ( 464 ) ;
    break ;
  case 48 : 
    printesc ( 465 ) ;
    break ;
  case 49 : 
    printesc ( 466 ) ;
    break ;
  case 50 : 
    printesc ( 467 ) ;
    break ;
  case 51 : 
    printesc ( 468 ) ;
    break ;
  case 52 : 
    printesc ( 469 ) ;
    break ;
  case 53 : 
    printesc ( 470 ) ;
    break ;
  case 54 : 
    printesc ( 471 ) ;
    break ;
  case 55 : 
    printesc ( 472 ) ;
    break ;
  case 56 : 
    printesc ( 473 ) ;
    break ;
  case 57 : 
    printesc ( 474 ) ;
    break ;
    default: 
    print ( 475 ) ;
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
  begindiagnostic_regmem 
  oldsetting = selector ;
  if ( ( eqtb [15192 ].cint <= 0 ) && ( selector == 19 ) ) 
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
  enddiagnostic_regmem 
  printnl ( 335 ) ;
  if ( blankline ) 
  println () ;
  selector = oldsetting ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintlengthparam ( integer n ) 
#else
zprintlengthparam ( n ) 
  integer n ;
#endif
{
  printlengthparam_regmem 
  switch ( n ) 
  {case 0 : 
    printesc ( 478 ) ;
    break ;
  case 1 : 
    printesc ( 479 ) ;
    break ;
  case 2 : 
    printesc ( 480 ) ;
    break ;
  case 3 : 
    printesc ( 481 ) ;
    break ;
  case 4 : 
    printesc ( 482 ) ;
    break ;
  case 5 : 
    printesc ( 483 ) ;
    break ;
  case 6 : 
    printesc ( 484 ) ;
    break ;
  case 7 : 
    printesc ( 485 ) ;
    break ;
  case 8 : 
    printesc ( 486 ) ;
    break ;
  case 9 : 
    printesc ( 487 ) ;
    break ;
  case 10 : 
    printesc ( 488 ) ;
    break ;
  case 11 : 
    printesc ( 489 ) ;
    break ;
  case 12 : 
    printesc ( 490 ) ;
    break ;
  case 13 : 
    printesc ( 491 ) ;
    break ;
  case 14 : 
    printesc ( 492 ) ;
    break ;
  case 15 : 
    printesc ( 493 ) ;
    break ;
  case 16 : 
    printesc ( 494 ) ;
    break ;
  case 17 : 
    printesc ( 495 ) ;
    break ;
  case 18 : 
    printesc ( 496 ) ;
    break ;
  case 19 : 
    printesc ( 497 ) ;
    break ;
  case 20 : 
    printesc ( 498 ) ;
    break ;
    default: 
    print ( 499 ) ;
    break ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zprintcmdchr ( quarterword cmd , halfword chrcode ) 
#else
zprintcmdchr ( cmd , chrcode ) 
  quarterword cmd ;
  halfword chrcode ;
#endif
{
  printcmdchr_regmem 
  switch ( cmd ) 
  {case 1 : 
    {
      print ( 557 ) ;
      print ( chrcode ) ;
    } 
    break ;
  case 2 : 
    {
      print ( 558 ) ;
      print ( chrcode ) ;
    } 
    break ;
  case 3 : 
    {
      print ( 559 ) ;
      print ( chrcode ) ;
    } 
    break ;
  case 6 : 
    {
      print ( 560 ) ;
      print ( chrcode ) ;
    } 
    break ;
  case 7 : 
    {
      print ( 561 ) ;
      print ( chrcode ) ;
    } 
    break ;
  case 8 : 
    {
      print ( 562 ) ;
      print ( chrcode ) ;
    } 
    break ;
  case 9 : 
    print ( 563 ) ;
    break ;
  case 10 : 
    {
      print ( 564 ) ;
      print ( chrcode ) ;
    } 
    break ;
  case 11 : 
    {
      print ( 565 ) ;
      print ( chrcode ) ;
    } 
    break ;
  case 12 : 
    {
      print ( 566 ) ;
      print ( chrcode ) ;
    } 
    break ;
  case 75 : 
  case 76 : 
    if ( chrcode < 12544 ) 
    printskipparam ( chrcode - 12526 ) ;
    else if ( chrcode < 12800 ) 
    {
      printesc ( 392 ) ;
      printint ( chrcode - 12544 ) ;
    } 
    else {
	
      printesc ( 393 ) ;
      printint ( chrcode - 12800 ) ;
    } 
    break ;
  case 72 : 
    if ( chrcode >= 13066 ) 
    {
      printesc ( 404 ) ;
      printint ( chrcode - 13066 ) ;
    } 
    else switch ( chrcode ) 
    {case 13057 : 
      printesc ( 395 ) ;
      break ;
    case 13058 : 
      printesc ( 396 ) ;
      break ;
    case 13059 : 
      printesc ( 397 ) ;
      break ;
    case 13060 : 
      printesc ( 398 ) ;
      break ;
    case 13061 : 
      printesc ( 399 ) ;
      break ;
    case 13062 : 
      printesc ( 400 ) ;
      break ;
    case 13063 : 
      printesc ( 401 ) ;
      break ;
    case 13064 : 
      printesc ( 402 ) ;
      break ;
      default: 
      printesc ( 403 ) ;
      break ;
    } 
    break ;
  case 73 : 
    if ( chrcode < 15221 ) 
    printparam ( chrcode - 15163 ) ;
    else {
	
      printesc ( 476 ) ;
      printint ( chrcode - 15221 ) ;
    } 
    break ;
  case 74 : 
    if ( chrcode < 15754 ) 
    printlengthparam ( chrcode - 15733 ) ;
    else {
	
      printesc ( 500 ) ;
      printint ( chrcode - 15754 ) ;
    } 
    break ;
  case 45 : 
    printesc ( 508 ) ;
    break ;
  case 90 : 
    printesc ( 509 ) ;
    break ;
  case 40 : 
    printesc ( 510 ) ;
    break ;
  case 41 : 
    printesc ( 511 ) ;
    break ;
  case 77 : 
    printesc ( 519 ) ;
    break ;
  case 61 : 
    printesc ( 512 ) ;
    break ;
  case 42 : 
    printesc ( 531 ) ;
    break ;
  case 16 : 
    printesc ( 513 ) ;
    break ;
  case 107 : 
    printesc ( 504 ) ;
    break ;
  case 88 : 
    printesc ( 518 ) ;
    break ;
  case 15 : 
    printesc ( 514 ) ;
    break ;
  case 92 : 
    printesc ( 515 ) ;
    break ;
  case 67 : 
    printesc ( 505 ) ;
    break ;
  case 62 : 
    printesc ( 516 ) ;
    break ;
  case 64 : 
    printesc ( 32 ) ;
    break ;
  case 102 : 
    printesc ( 517 ) ;
    break ;
  case 32 : 
    printesc ( 520 ) ;
    break ;
  case 36 : 
    printesc ( 521 ) ;
    break ;
  case 39 : 
    printesc ( 522 ) ;
    break ;
  case 37 : 
    printesc ( 327 ) ;
    break ;
  case 44 : 
    printesc ( 47 ) ;
    break ;
  case 18 : 
    printesc ( 348 ) ;
    break ;
  case 46 : 
    printesc ( 523 ) ;
    break ;
  case 17 : 
    printesc ( 524 ) ;
    break ;
  case 54 : 
    printesc ( 525 ) ;
    break ;
  case 91 : 
    printesc ( 526 ) ;
    break ;
  case 34 : 
    printesc ( 527 ) ;
    break ;
  case 65 : 
    printesc ( 528 ) ;
    break ;
  case 103 : 
    printesc ( 529 ) ;
    break ;
  case 55 : 
    printesc ( 332 ) ;
    break ;
  case 63 : 
    printesc ( 530 ) ;
    break ;
  case 66 : 
    printesc ( 533 ) ;
    break ;
  case 96 : 
    printesc ( 534 ) ;
    break ;
  case 0 : 
    printesc ( 535 ) ;
    break ;
  case 98 : 
    printesc ( 536 ) ;
    break ;
  case 80 : 
    printesc ( 532 ) ;
    break ;
  case 84 : 
    printesc ( 405 ) ;
    break ;
  case 109 : 
    printesc ( 537 ) ;
    break ;
  case 71 : 
    printesc ( 404 ) ;
    break ;
  case 38 : 
    printesc ( 349 ) ;
    break ;
  case 33 : 
    printesc ( 538 ) ;
    break ;
  case 56 : 
    printesc ( 539 ) ;
    break ;
  case 35 : 
    printesc ( 540 ) ;
    break ;
  case 13 : 
    printesc ( 596 ) ;
    break ;
  case 104 : 
    if ( chrcode == 0 ) 
    printesc ( 628 ) ;
    else printesc ( 629 ) ;
    break ;
  case 110 : 
    switch ( chrcode ) 
    {case 1 : 
      printesc ( 631 ) ;
      break ;
    case 2 : 
      printesc ( 632 ) ;
      break ;
    case 3 : 
      printesc ( 633 ) ;
      break ;
    case 4 : 
      printesc ( 634 ) ;
      break ;
      default: 
      printesc ( 630 ) ;
      break ;
    } 
    break ;
  case 89 : 
    if ( chrcode == 0 ) 
    printesc ( 476 ) ;
    else if ( chrcode == 1 ) 
    printesc ( 500 ) ;
    else if ( chrcode == 2 ) 
    printesc ( 392 ) ;
    else printesc ( 393 ) ;
    break ;
  case 79 : 
    if ( chrcode == 1 ) 
    printesc ( 668 ) ;
    else printesc ( 667 ) ;
    break ;
  case 82 : 
    if ( chrcode == 0 ) 
    printesc ( 669 ) ;
    else printesc ( 670 ) ;
    break ;
  case 83 : 
    if ( chrcode == 1 ) 
    printesc ( 671 ) ;
    else if ( chrcode == 3 ) 
    printesc ( 672 ) ;
    else printesc ( 673 ) ;
    break ;
  case 70 : 
    switch ( chrcode ) 
    {case 0 : 
      printesc ( 674 ) ;
      break ;
    case 1 : 
      printesc ( 675 ) ;
      break ;
    case 2 : 
      printesc ( 676 ) ;
      break ;
    case 3 : 
      printesc ( 677 ) ;
      break ;
      default: 
      printesc ( 678 ) ;
      break ;
    } 
    break ;
  case 108 : 
    switch ( chrcode ) 
    {case 0 : 
      printesc ( 734 ) ;
      break ;
    case 1 : 
      printesc ( 735 ) ;
      break ;
    case 2 : 
      printesc ( 736 ) ;
      break ;
    case 3 : 
      printesc ( 737 ) ;
      break ;
    case 4 : 
      printesc ( 738 ) ;
      break ;
      default: 
      printesc ( 739 ) ;
      break ;
    } 
    break ;
  case 105 : 
    switch ( chrcode ) 
    {case 1 : 
      printesc ( 756 ) ;
      break ;
    case 2 : 
      printesc ( 757 ) ;
      break ;
    case 3 : 
      printesc ( 758 ) ;
      break ;
    case 4 : 
      printesc ( 759 ) ;
      break ;
    case 5 : 
      printesc ( 760 ) ;
      break ;
    case 6 : 
      printesc ( 761 ) ;
      break ;
    case 7 : 
      printesc ( 762 ) ;
      break ;
    case 8 : 
      printesc ( 763 ) ;
      break ;
    case 9 : 
      printesc ( 764 ) ;
      break ;
    case 10 : 
      printesc ( 765 ) ;
      break ;
    case 11 : 
      printesc ( 766 ) ;
      break ;
    case 12 : 
      printesc ( 767 ) ;
      break ;
    case 13 : 
      printesc ( 768 ) ;
      break ;
    case 14 : 
      printesc ( 769 ) ;
      break ;
    case 15 : 
      printesc ( 770 ) ;
      break ;
    case 16 : 
      printesc ( 771 ) ;
      break ;
      default: 
      printesc ( 755 ) ;
      break ;
    } 
    break ;
  case 106 : 
    if ( chrcode == 2 ) 
    printesc ( 772 ) ;
    else if ( chrcode == 4 ) 
    printesc ( 773 ) ;
    else printesc ( 774 ) ;
    break ;
  case 4 : 
    if ( chrcode == 256 ) 
    printesc ( 893 ) ;
    else {
	
      print ( 897 ) ;
      print ( chrcode ) ;
    } 
    break ;
  case 5 : 
    if ( chrcode == 257 ) 
    printesc ( 894 ) ;
    else printesc ( 895 ) ;
    break ;
  case 81 : 
    switch ( chrcode ) 
    {case 0 : 
      printesc ( 965 ) ;
      break ;
    case 1 : 
      printesc ( 966 ) ;
      break ;
    case 2 : 
      printesc ( 967 ) ;
      break ;
    case 3 : 
      printesc ( 968 ) ;
      break ;
    case 4 : 
      printesc ( 969 ) ;
      break ;
    case 5 : 
      printesc ( 970 ) ;
      break ;
    case 6 : 
      printesc ( 971 ) ;
      break ;
      default: 
      printesc ( 972 ) ;
      break ;
    } 
    break ;
  case 14 : 
    if ( chrcode == 1 ) 
    printesc ( 1021 ) ;
    else printesc ( 1020 ) ;
    break ;
  case 26 : 
    switch ( chrcode ) 
    {case 4 : 
      printesc ( 1022 ) ;
      break ;
    case 0 : 
      printesc ( 1023 ) ;
      break ;
    case 1 : 
      printesc ( 1024 ) ;
      break ;
    case 2 : 
      printesc ( 1025 ) ;
      break ;
      default: 
      printesc ( 1026 ) ;
      break ;
    } 
    break ;
  case 27 : 
    switch ( chrcode ) 
    {case 4 : 
      printesc ( 1027 ) ;
      break ;
    case 0 : 
      printesc ( 1028 ) ;
      break ;
    case 1 : 
      printesc ( 1029 ) ;
      break ;
    case 2 : 
      printesc ( 1030 ) ;
      break ;
      default: 
      printesc ( 1031 ) ;
      break ;
    } 
    break ;
  case 28 : 
    printesc ( 333 ) ;
    break ;
  case 29 : 
    printesc ( 337 ) ;
    break ;
  case 30 : 
    printesc ( 339 ) ;
    break ;
  case 21 : 
    if ( chrcode == 1 ) 
    printesc ( 1049 ) ;
    else printesc ( 1050 ) ;
    break ;
  case 22 : 
    if ( chrcode == 1 ) 
    printesc ( 1051 ) ;
    else printesc ( 1052 ) ;
    break ;
  case 20 : 
    switch ( chrcode ) 
    {case 0 : 
      printesc ( 406 ) ;
      break ;
    case 1 : 
      printesc ( 1053 ) ;
      break ;
    case 2 : 
      printesc ( 1054 ) ;
      break ;
    case 3 : 
      printesc ( 960 ) ;
      break ;
    case 4 : 
      printesc ( 1055 ) ;
      break ;
    case 5 : 
      printesc ( 962 ) ;
      break ;
      default: 
      printesc ( 1056 ) ;
      break ;
    } 
    break ;
  case 31 : 
    if ( chrcode == 100 ) 
    printesc ( 1058 ) ;
    else if ( chrcode == 101 ) 
    printesc ( 1059 ) ;
    else if ( chrcode == 102 ) 
    printesc ( 1060 ) ;
    else printesc ( 1057 ) ;
    break ;
  case 43 : 
    if ( chrcode == 0 ) 
    printesc ( 1076 ) ;
    else printesc ( 1075 ) ;
    break ;
  case 25 : 
    if ( chrcode == 10 ) 
    printesc ( 1087 ) ;
    else if ( chrcode == 11 ) 
    printesc ( 1086 ) ;
    else printesc ( 1085 ) ;
    break ;
  case 23 : 
    if ( chrcode == 1 ) 
    printesc ( 1089 ) ;
    else printesc ( 1088 ) ;
    break ;
  case 24 : 
    if ( chrcode == 1 ) 
    printesc ( 1091 ) ;
    else printesc ( 1090 ) ;
    break ;
  case 47 : 
    if ( chrcode == 1 ) 
    printesc ( 45 ) ;
    else printesc ( 346 ) ;
    break ;
  case 48 : 
    if ( chrcode == 1 ) 
    printesc ( 1123 ) ;
    else printesc ( 1122 ) ;
    break ;
  case 50 : 
    switch ( chrcode ) 
    {case 16 : 
      printesc ( 861 ) ;
      break ;
    case 17 : 
      printesc ( 862 ) ;
      break ;
    case 18 : 
      printesc ( 863 ) ;
      break ;
    case 19 : 
      printesc ( 864 ) ;
      break ;
    case 20 : 
      printesc ( 865 ) ;
      break ;
    case 21 : 
      printesc ( 866 ) ;
      break ;
    case 22 : 
      printesc ( 867 ) ;
      break ;
    case 23 : 
      printesc ( 868 ) ;
      break ;
    case 26 : 
      printesc ( 870 ) ;
      break ;
      default: 
      printesc ( 869 ) ;
      break ;
    } 
    break ;
  case 51 : 
    if ( chrcode == 1 ) 
    printesc ( 873 ) ;
    else if ( chrcode == 2 ) 
    printesc ( 874 ) ;
    else printesc ( 1124 ) ;
    break ;
  case 53 : 
    printstyle ( chrcode ) ;
    break ;
  case 52 : 
    switch ( chrcode ) 
    {case 1 : 
      printesc ( 1143 ) ;
      break ;
    case 2 : 
      printesc ( 1144 ) ;
      break ;
    case 3 : 
      printesc ( 1145 ) ;
      break ;
    case 4 : 
      printesc ( 1146 ) ;
      break ;
    case 5 : 
      printesc ( 1147 ) ;
      break ;
      default: 
      printesc ( 1142 ) ;
      break ;
    } 
    break ;
  case 49 : 
    if ( chrcode == 30 ) 
    printesc ( 871 ) ;
    else printesc ( 872 ) ;
    break ;
  case 93 : 
    if ( chrcode == 1 ) 
    printesc ( 1166 ) ;
    else if ( chrcode == 2 ) 
    printesc ( 1167 ) ;
    else printesc ( 1168 ) ;
    break ;
  case 97 : 
    if ( chrcode == 0 ) 
    printesc ( 1169 ) ;
    else if ( chrcode == 1 ) 
    printesc ( 1170 ) ;
    else if ( chrcode == 2 ) 
    printesc ( 1171 ) ;
    else printesc ( 1172 ) ;
    break ;
  case 94 : 
    if ( chrcode != 0 ) 
    printesc ( 1187 ) ;
    else printesc ( 1186 ) ;
    break ;
  case 95 : 
    switch ( chrcode ) 
    {case 0 : 
      printesc ( 1188 ) ;
      break ;
    case 1 : 
      printesc ( 1189 ) ;
      break ;
    case 2 : 
      printesc ( 1190 ) ;
      break ;
    case 3 : 
      printesc ( 1191 ) ;
      break ;
    case 4 : 
      printesc ( 1192 ) ;
      break ;
    case 5 : 
      printesc ( 1193 ) ;
      break ;
    case 7 : 
      printesc ( 1195 ) ;
      break ;
      default: 
      printesc ( 1194 ) ;
      break ;
    } 
    break ;
  case 68 : 
    {
      printesc ( 513 ) ;
      printhex ( chrcode ) ;
    } 
    break ;
  case 69 : 
    {
      printesc ( 524 ) ;
      printhex ( chrcode ) ;
    } 
    break ;
  case 85 : 
    if ( chrcode == 13627 ) 
    printesc ( 412 ) ;
    else if ( chrcode == 14651 ) 
    printesc ( 416 ) ;
    else if ( chrcode == 13883 ) 
    printesc ( 413 ) ;
    else if ( chrcode == 14139 ) 
    printesc ( 414 ) ;
    else if ( chrcode == 14395 ) 
    printesc ( 415 ) ;
    else printesc ( 477 ) ;
    break ;
  case 86 : 
    printsize ( chrcode - 13579 ) ;
    break ;
  case 99 : 
    if ( chrcode == 1 ) 
    printesc ( 948 ) ;
    else printesc ( 936 ) ;
    break ;
  case 78 : 
    if ( chrcode == 0 ) 
    printesc ( 1215 ) ;
    else printesc ( 1216 ) ;
    break ;
  case 87 : 
    {
      print ( 1224 ) ;
      print ( fontname [chrcode ]) ;
      if ( fontsize [chrcode ]!= fontdsize [chrcode ]) 
      {
	print ( 740 ) ;
	printscaled ( fontsize [chrcode ]) ;
	print ( 394 ) ;
      } 
    } 
    break ;
  case 100 : 
    switch ( chrcode ) 
    {case 0 : 
      printesc ( 272 ) ;
      break ;
    case 1 : 
      printesc ( 273 ) ;
      break ;
    case 2 : 
      printesc ( 274 ) ;
      break ;
      default: 
      printesc ( 1225 ) ;
      break ;
    } 
    break ;
  case 60 : 
    if ( chrcode == 0 ) 
    printesc ( 1227 ) ;
    else printesc ( 1226 ) ;
    break ;
  case 58 : 
    if ( chrcode == 0 ) 
    printesc ( 1228 ) ;
    else printesc ( 1229 ) ;
    break ;
  case 57 : 
    if ( chrcode == 13883 ) 
    printesc ( 1235 ) ;
    else printesc ( 1236 ) ;
    break ;
  case 19 : 
    switch ( chrcode ) 
    {case 1 : 
      printesc ( 1238 ) ;
      break ;
    case 2 : 
      printesc ( 1239 ) ;
      break ;
    case 3 : 
      printesc ( 1240 ) ;
      break ;
      default: 
      printesc ( 1237 ) ;
      break ;
    } 
    break ;
  case 101 : 
    print ( 1247 ) ;
    break ;
  case 111 : 
    print ( 1248 ) ;
    break ;
  case 112 : 
    printesc ( 1249 ) ;
    break ;
  case 113 : 
    printesc ( 1250 ) ;
    break ;
  case 114 : 
    {
      printesc ( 1166 ) ;
      printesc ( 1250 ) ;
    } 
    break ;
  case 115 : 
    printesc ( 1251 ) ;
    break ;
  case 59 : 
    switch ( chrcode ) 
    {case 0 : 
      printesc ( 1283 ) ;
      break ;
    case 1 : 
      printesc ( 593 ) ;
      break ;
    case 2 : 
      printesc ( 1284 ) ;
      break ;
    case 3 : 
      printesc ( 1285 ) ;
      break ;
    case 4 : 
      printesc ( 1286 ) ;
      break ;
    case 5 : 
      printesc ( 1287 ) ;
      break ;
      default: 
      print ( 1288 ) ;
      break ;
    } 
    break ;
    default: 
    print ( 567 ) ;
    break ;
  } 
} 
#ifdef STAT
void 
#ifdef HAVE_PROTOTYPES
zshoweqtb ( halfword n ) 
#else
zshoweqtb ( n ) 
  halfword n ;
#endif
{
  showeqtb_regmem 
  if ( n < 1 ) 
  printchar ( 63 ) ;
  else if ( ( n < 12526 ) || ( ( n > 16009 ) && ( n <= eqtbtop ) ) ) 
  {
    sprintcs ( n ) ;
    printchar ( 61 ) ;
    printcmdchr ( eqtb [n ].hh.b0 , eqtb [n ].hh .v.RH ) ;
    if ( eqtb [n ].hh.b0 >= 111 ) 
    {
      printchar ( 58 ) ;
      showtokenlist ( mem [eqtb [n ].hh .v.RH ].hh .v.RH , 0 , 32 ) ;
    } 
  } 
  else if ( n < 13056 ) 
  if ( n < 12544 ) 
  {
    printskipparam ( n - 12526 ) ;
    printchar ( 61 ) ;
    if ( n < 12541 ) 
    printspec ( eqtb [n ].hh .v.RH , 394 ) ;
    else printspec ( eqtb [n ].hh .v.RH , 334 ) ;
  } 
  else if ( n < 12800 ) 
  {
    printesc ( 392 ) ;
    printint ( n - 12544 ) ;
    printchar ( 61 ) ;
    printspec ( eqtb [n ].hh .v.RH , 394 ) ;
  } 
  else {
      
    printesc ( 393 ) ;
    printint ( n - 12800 ) ;
    printchar ( 61 ) ;
    printspec ( eqtb [n ].hh .v.RH , 334 ) ;
  } 
  else if ( n < 15163 ) 
  if ( n == 13056 ) 
  {
    printesc ( 405 ) ;
    printchar ( 61 ) ;
    if ( eqtb [13056 ].hh .v.RH == 0 ) 
    printchar ( 48 ) ;
    else printint ( mem [eqtb [13056 ].hh .v.RH ].hh .v.LH ) ;
  } 
  else if ( n < 13066 ) 
  {
    printcmdchr ( 72 , n ) ;
    printchar ( 61 ) ;
    if ( eqtb [n ].hh .v.RH != 0 ) 
    showtokenlist ( mem [eqtb [n ].hh .v.RH ].hh .v.RH , 0 , 32 ) ;
  } 
  else if ( n < 13322 ) 
  {
    printesc ( 404 ) ;
    printint ( n - 13066 ) ;
    printchar ( 61 ) ;
    if ( eqtb [n ].hh .v.RH != 0 ) 
    showtokenlist ( mem [eqtb [n ].hh .v.RH ].hh .v.RH , 0 , 32 ) ;
  } 
  else if ( n < 13578 ) 
  {
    printesc ( 406 ) ;
    printint ( n - 13322 ) ;
    printchar ( 61 ) ;
    if ( eqtb [n ].hh .v.RH == 0 ) 
    print ( 407 ) ;
    else {
	
      depththreshold = 0 ;
      breadthmax = 1 ;
      shownodelist ( eqtb [n ].hh .v.RH ) ;
    } 
  } 
  else if ( n < 13627 ) 
  {
    if ( n == 13578 ) 
    print ( 408 ) ;
    else if ( n < 13595 ) 
    {
      printesc ( 409 ) ;
      printint ( n - 13579 ) ;
    } 
    else if ( n < 13611 ) 
    {
      printesc ( 410 ) ;
      printint ( n - 13595 ) ;
    } 
    else {
	
      printesc ( 411 ) ;
      printint ( n - 13611 ) ;
    } 
    printchar ( 61 ) ;
    printesc ( hash [10524 + eqtb [n ].hh .v.RH ].v.RH ) ;
  } 
  else if ( n < 14651 ) 
  {
    if ( n < 13883 ) 
    {
      printesc ( 412 ) ;
      printint ( n - 13627 ) ;
    } 
    else if ( n < 14139 ) 
    {
      printesc ( 413 ) ;
      printint ( n - 13883 ) ;
    } 
    else if ( n < 14395 ) 
    {
      printesc ( 414 ) ;
      printint ( n - 14139 ) ;
    } 
    else {
	
      printesc ( 415 ) ;
      printint ( n - 14395 ) ;
    } 
    printchar ( 61 ) ;
    printint ( eqtb [n ].hh .v.RH ) ;
  } 
  else {
      
    printesc ( 416 ) ;
    printint ( n - 14651 ) ;
    printchar ( 61 ) ;
    printint ( eqtb [n ].hh .v.RH ) ;
  } 
  else if ( n < 15733 ) 
  {
    if ( n < 15221 ) 
    printparam ( n - 15163 ) ;
    else if ( n < 15477 ) 
    {
      printesc ( 476 ) ;
      printint ( n - 15221 ) ;
    } 
    else {
	
      printesc ( 477 ) ;
      printint ( n - 15477 ) ;
    } 
    printchar ( 61 ) ;
    printint ( eqtb [n ].cint ) ;
  } 
  else if ( n <= 16009 ) 
  {
    if ( n < 15754 ) 
    printlengthparam ( n - 15733 ) ;
    else {
	
      printesc ( 500 ) ;
      printint ( n - 15754 ) ;
    } 
    printchar ( 61 ) ;
    printscaled ( eqtb [n ].cint ) ;
    print ( 394 ) ;
  } 
  else printchar ( 63 ) ;
} 
#endif /* STAT */
halfword 
#ifdef HAVE_PROTOTYPES
zidlookup ( integer j , integer l ) 
#else
zidlookup ( j , l ) 
  integer j ;
  integer l ;
#endif
{
  /* 40 */ register halfword Result; idlookup_regmem 
  integer h  ;
  integer d  ;
  halfword p  ;
  halfword k  ;
  h = buffer [j ];
  {register integer for_end; k = j + 1 ;for_end = j + l - 1 ; if ( k <= 
  for_end) do 
    {
      h = h + h + buffer [k ];
      while ( h >= 8501 ) h = h - 8501 ;
    } 
  while ( k++ < for_end ) ;} 
  p = h + 514 ;
  while ( true ) {
      
    if ( hash [p ].v.RH > 0 ) 
    if ( ( strstart [hash [p ].v.RH + 1 ]- strstart [hash [p ].v.RH ]) 
    == l ) 
    if ( streqbuf ( hash [p ].v.RH , j ) ) 
    goto lab40 ;
    if ( hash [p ].v.LH == 0 ) 
    {
      if ( nonewcontrolsequence ) 
      p = 12525 ;
      else {
	  
	if ( hash [p ].v.RH > 0 ) 
	{
	  if ( hashhigh < hashextra ) 
	  {
	    incr ( hashhigh ) ;
	    hash [p ].v.LH = hashhigh + 16009 ;
	    p = hashhigh + 16009 ;
	  } 
	  else {
	      
	    do {
		if ( ( hashused == 514 ) ) 
	      overflow ( 503 , 10000 + hashextra ) ;
	      decr ( hashused ) ;
	    } while ( ! ( hash [hashused ].v.RH == 0 ) ) ;
	    hash [p ].v.LH = hashused ;
	    p = hashused ;
	  } 
	} 
	{
	  if ( poolptr + l > poolsize ) 
	  overflow ( 257 , poolsize - initpoolptr ) ;
	} 
	d = ( poolptr - strstart [strptr ]) ;
	while ( poolptr > strstart [strptr ]) {
	    
	  decr ( poolptr ) ;
	  strpool [poolptr + l ]= strpool [poolptr ];
	} 
	{register integer for_end; k = j ;for_end = j + l - 1 ; if ( k <= 
	for_end) do 
	  {
	    strpool [poolptr ]= buffer [k ];
	    incr ( poolptr ) ;
	  } 
	while ( k++ < for_end ) ;} 
	hash [p ].v.RH = makestring () ;
	poolptr = poolptr + d ;
	;
#ifdef STAT
	incr ( cscount ) ;
#endif /* STAT */
      } 
      goto lab40 ;
    } 
    p = hash [p ].v.LH ;
  } 
  lab40: Result = p ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
znewsavelevel ( groupcode c ) 
#else
znewsavelevel ( c ) 
  groupcode c ;
#endif
{
  newsavelevel_regmem 
  if ( saveptr > maxsavestack ) 
  {
    maxsavestack = saveptr ;
    if ( maxsavestack > savesize - 6 ) 
    overflow ( 541 , savesize ) ;
  } 
  savestack [saveptr ].hh.b0 = 3 ;
  savestack [saveptr ].hh.b1 = curgroup ;
  savestack [saveptr ].hh .v.RH = curboundary ;
  if ( curlevel == 255 ) 
  overflow ( 542 , 255 ) ;
  curboundary = saveptr ;
  incr ( curlevel ) ;
  incr ( saveptr ) ;
  curgroup = c ;
} 
void 
#ifdef HAVE_PROTOTYPES
zeqdestroy ( memoryword w ) 
#else
zeqdestroy ( w ) 
  memoryword w ;
#endif
{
  eqdestroy_regmem 
  halfword q  ;
  switch ( w .hh.b0 ) 
  {case 111 : 
  case 112 : 
  case 113 : 
  case 114 : 
    deletetokenref ( w .hh .v.RH ) ;
    break ;
  case 117 : 
    deleteglueref ( w .hh .v.RH ) ;
    break ;
  case 118 : 
    {
      q = w .hh .v.RH ;
      if ( q != 0 ) 
      freenode ( q , mem [q ].hh .v.LH + mem [q ].hh .v.LH + 1 ) ;
    } 
    break ;
  case 119 : 
    flushnodelist ( w .hh .v.RH ) ;
    break ;
    default: 
    ;
    break ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zeqsave ( halfword p , quarterword l ) 
#else
zeqsave ( p , l ) 
  halfword p ;
  quarterword l ;
#endif
{
  eqsave_regmem 
  if ( saveptr > maxsavestack ) 
  {
    maxsavestack = saveptr ;
    if ( maxsavestack > savesize - 6 ) 
    overflow ( 541 , savesize ) ;
  } 
  if ( l == 0 ) 
  savestack [saveptr ].hh.b0 = 1 ;
  else {
      
    savestack [saveptr ]= eqtb [p ];
    incr ( saveptr ) ;
    savestack [saveptr ].hh.b0 = 0 ;
  } 
  savestack [saveptr ].hh.b1 = l ;
  savestack [saveptr ].hh .v.RH = p ;
  incr ( saveptr ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zeqdefine ( halfword p , quarterword t , halfword e ) 
#else
zeqdefine ( p , t , e ) 
  halfword p ;
  quarterword t ;
  halfword e ;
#endif
{
  eqdefine_regmem 
  if ( eqtb [p ].hh.b1 == curlevel ) 
  eqdestroy ( eqtb [p ]) ;
  else if ( curlevel > 1 ) 
  eqsave ( p , eqtb [p ].hh.b1 ) ;
  eqtb [p ].hh.b1 = curlevel ;
  eqtb [p ].hh.b0 = t ;
  eqtb [p ].hh .v.RH = e ;
} 
void 
#ifdef HAVE_PROTOTYPES
zeqworddefine ( halfword p , integer w ) 
#else
zeqworddefine ( p , w ) 
  halfword p ;
  integer w ;
#endif
{
  eqworddefine_regmem 
  if ( xeqlevel [p ]!= curlevel ) 
  {
    eqsave ( p , xeqlevel [p ]) ;
    xeqlevel [p ]= curlevel ;
  } 
  eqtb [p ].cint = w ;
} 
void 
#ifdef HAVE_PROTOTYPES
zgeqdefine ( halfword p , quarterword t , halfword e ) 
#else
zgeqdefine ( p , t , e ) 
  halfword p ;
  quarterword t ;
  halfword e ;
#endif
{
  geqdefine_regmem 
  eqdestroy ( eqtb [p ]) ;
  eqtb [p ].hh.b1 = 1 ;
  eqtb [p ].hh.b0 = t ;
  eqtb [p ].hh .v.RH = e ;
} 
void 
#ifdef HAVE_PROTOTYPES
zgeqworddefine ( halfword p , integer w ) 
#else
zgeqworddefine ( p , w ) 
  halfword p ;
  integer w ;
#endif
{
  geqworddefine_regmem 
  eqtb [p ].cint = w ;
  xeqlevel [p ]= 1 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zsaveforafter ( halfword t ) 
#else
zsaveforafter ( t ) 
  halfword t ;
#endif
{
  saveforafter_regmem 
  if ( curlevel > 1 ) 
  {
    if ( saveptr > maxsavestack ) 
    {
      maxsavestack = saveptr ;
      if ( maxsavestack > savesize - 6 ) 
      overflow ( 541 , savesize ) ;
    } 
    savestack [saveptr ].hh.b0 = 2 ;
    savestack [saveptr ].hh.b1 = 0 ;
    savestack [saveptr ].hh .v.RH = t ;
    incr ( saveptr ) ;
  } 
} 
#ifdef STAT
void 
#ifdef HAVE_PROTOTYPES
zrestoretrace ( halfword p , strnumber s ) 
#else
zrestoretrace ( p , s ) 
  halfword p ;
  strnumber s ;
#endif
{
  restoretrace_regmem 
  begindiagnostic () ;
  printchar ( 123 ) ;
  print ( s ) ;
  printchar ( 32 ) ;
  showeqtb ( p ) ;
  printchar ( 125 ) ;
  enddiagnostic ( false ) ;
} 
#endif /* STAT */
void 
#ifdef HAVE_PROTOTYPES
unsave ( void ) 
#else
unsave ( ) 
#endif
{
  /* 30 */ unsave_regmem 
  halfword p  ;
  quarterword l  ;
  halfword t  ;
  if ( curlevel > 1 ) 
  {
    decr ( curlevel ) ;
    while ( true ) {
	
      decr ( saveptr ) ;
      if ( savestack [saveptr ].hh.b0 == 3 ) 
      goto lab30 ;
      p = savestack [saveptr ].hh .v.RH ;
      if ( savestack [saveptr ].hh.b0 == 2 ) 
      {
	t = curtok ;
	curtok = p ;
	backinput () ;
	curtok = t ;
      } 
      else {
	  
	if ( savestack [saveptr ].hh.b0 == 0 ) 
	{
	  l = savestack [saveptr ].hh.b1 ;
	  decr ( saveptr ) ;
	} 
	else savestack [saveptr ]= eqtb [12525 ];
	if ( ( p < 15163 ) || ( p > 16009 ) ) 
	if ( eqtb [p ].hh.b1 == 1 ) 
	{
	  eqdestroy ( savestack [saveptr ]) ;
	;
#ifdef STAT
	  if ( eqtb [15200 ].cint > 0 ) 
	  restoretrace ( p , 544 ) ;
#endif /* STAT */
	} 
	else {
	    
	  eqdestroy ( eqtb [p ]) ;
	  eqtb [p ]= savestack [saveptr ];
	;
#ifdef STAT
	  if ( eqtb [15200 ].cint > 0 ) 
	  restoretrace ( p , 545 ) ;
#endif /* STAT */
	} 
	else if ( xeqlevel [p ]!= 1 ) 
	{
	  eqtb [p ]= savestack [saveptr ];
	  xeqlevel [p ]= l ;
	;
#ifdef STAT
	  if ( eqtb [15200 ].cint > 0 ) 
	  restoretrace ( p , 545 ) ;
#endif /* STAT */
	} 
	else {
	    
	;
#ifdef STAT
	  if ( eqtb [15200 ].cint > 0 ) 
	  restoretrace ( p , 544 ) ;
#endif /* STAT */
	} 
      } 
    } 
    lab30: curgroup = savestack [saveptr ].hh.b1 ;
    curboundary = savestack [saveptr ].hh .v.RH ;
  } 
  else confusion ( 543 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
preparemag ( void ) 
#else
preparemag ( ) 
#endif
{
  preparemag_regmem 
  if ( ( magset > 0 ) && ( eqtb [15180 ].cint != magset ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 547 ) ;
    } 
    printint ( eqtb [15180 ].cint ) ;
    print ( 548 ) ;
    printnl ( 549 ) ;
    {
      helpptr = 2 ;
      helpline [1 ]= 550 ;
      helpline [0 ]= 551 ;
    } 
    interror ( magset ) ;
    geqworddefine ( 15180 , magset ) ;
  } 
  if ( ( eqtb [15180 ].cint <= 0 ) || ( eqtb [15180 ].cint > 32768L ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 552 ) ;
    } 
    {
      helpptr = 1 ;
      helpline [0 ]= 553 ;
    } 
    interror ( eqtb [15180 ].cint ) ;
    geqworddefine ( 15180 , 1000 ) ;
  } 
  magset = eqtb [15180 ].cint ;
} 
void 
#ifdef HAVE_PROTOTYPES
ztokenshow ( halfword p ) 
#else
ztokenshow ( p ) 
  halfword p ;
#endif
{
  tokenshow_regmem 
  if ( p != 0 ) 
  showtokenlist ( mem [p ].hh .v.RH , 0 , 10000000L ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
printmeaning ( void ) 
#else
printmeaning ( ) 
#endif
{
  printmeaning_regmem 
  printcmdchr ( curcmd , curchr ) ;
  if ( curcmd >= 111 ) 
  {
    printchar ( 58 ) ;
    println () ;
    tokenshow ( curchr ) ;
  } 
  else if ( curcmd == 110 ) 
  {
    printchar ( 58 ) ;
    println () ;
    tokenshow ( curmark [curchr ]) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
showcurcmdchr ( void ) 
#else
showcurcmdchr ( ) 
#endif
{
  showcurcmdchr_regmem 
  begindiagnostic () ;
  printnl ( 123 ) ;
  if ( curlist .modefield != shownmode ) 
  {
    printmode ( curlist .modefield ) ;
    print ( 568 ) ;
    shownmode = curlist .modefield ;
  } 
  printcmdchr ( curcmd , curchr ) ;
  printchar ( 125 ) ;
  enddiagnostic ( false ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
showcontext ( void ) 
#else
showcontext ( ) 
#endif
{
  /* 30 */ showcontext_regmem 
  char oldsetting  ;
  integer nn  ;
  boolean bottomline  ;
  integer i  ;
  integer j  ;
  integer l  ;
  integer m  ;
  integer n  ;
  integer p  ;
  integer q  ;
  baseptr = inputptr ;
  inputstack [baseptr ]= curinput ;
  nn = -1 ;
  bottomline = false ;
  while ( true ) {
      
    curinput = inputstack [baseptr ];
    if ( ( curinput .statefield != 0 ) ) 
    if ( ( curinput .namefield > 17 ) || ( baseptr == 0 ) ) 
    bottomline = true ;
    if ( ( baseptr == inputptr ) || bottomline || ( nn < eqtb [15217 ].cint 
    ) ) 
    {
      if ( ( baseptr == inputptr ) || ( curinput .statefield != 0 ) || ( 
      curinput .indexfield != 3 ) || ( curinput .locfield != 0 ) ) 
      {
	tally = 0 ;
	oldsetting = selector ;
	if ( curinput .statefield != 0 ) 
	{
	  if ( curinput .namefield <= 17 ) 
	  if ( ( curinput .namefield == 0 ) ) 
	  if ( baseptr == 0 ) 
	  printnl ( 574 ) ;
	  else printnl ( 575 ) ;
	  else {
	      
	    printnl ( 576 ) ;
	    if ( curinput .namefield == 17 ) 
	    printchar ( 42 ) ;
	    else printint ( curinput .namefield - 1 ) ;
	    printchar ( 62 ) ;
	  } 
	  else {
	      
	    printnl ( curinput .namefield ) ;
	    print ( 58 ) ;
	    printint ( line ) ;
	    print ( 58 ) ;
	  } 
	  printchar ( 32 ) ;
	  {
	    l = tally ;
	    tally = 0 ;
	    selector = 20 ;
	    trickcount = 1000000L ;
	  } 
	  if ( buffer [curinput .limitfield ]== eqtb [15211 ].cint ) 
	  j = curinput .limitfield ;
	  else j = curinput .limitfield + 1 ;
	  if ( j > 0 ) 
	  {register integer for_end; i = curinput .startfield ;for_end = j - 
	  1 ; if ( i <= for_end) do 
	    {
	      if ( i == curinput .locfield ) 
	      {
		firstcount = tally ;
		trickcount = tally + 1 + errorline - halferrorline ;
		if ( trickcount < errorline ) 
		trickcount = errorline ;
	      } 
	      print ( buffer [i ]) ;
	    } 
	  while ( i++ < for_end ) ;} 
	} 
	else {
	    
	  switch ( curinput .indexfield ) 
	  {case 0 : 
	    printnl ( 577 ) ;
	    break ;
	  case 1 : 
	  case 2 : 
	    printnl ( 578 ) ;
	    break ;
	  case 3 : 
	    if ( curinput .locfield == 0 ) 
	    printnl ( 579 ) ;
	    else printnl ( 580 ) ;
	    break ;
	  case 4 : 
	    printnl ( 581 ) ;
	    break ;
	  case 5 : 
	    {
	      println () ;
	      printcs ( curinput .namefield ) ;
	    } 
	    break ;
	  case 6 : 
	    printnl ( 582 ) ;
	    break ;
	  case 7 : 
	    printnl ( 583 ) ;
	    break ;
	  case 8 : 
	    printnl ( 584 ) ;
	    break ;
	  case 9 : 
	    printnl ( 585 ) ;
	    break ;
	  case 10 : 
	    printnl ( 586 ) ;
	    break ;
	  case 11 : 
	    printnl ( 587 ) ;
	    break ;
	  case 12 : 
	    printnl ( 588 ) ;
	    break ;
	  case 13 : 
	    printnl ( 589 ) ;
	    break ;
	  case 14 : 
	    printnl ( 590 ) ;
	    break ;
	  case 15 : 
	    printnl ( 591 ) ;
	    break ;
	    default: 
	    printnl ( 63 ) ;
	    break ;
	  } 
	  {
	    l = tally ;
	    tally = 0 ;
	    selector = 20 ;
	    trickcount = 1000000L ;
	  } 
	  if ( curinput .indexfield < 5 ) 
	  showtokenlist ( curinput .startfield , curinput .locfield , 100000L 
	  ) ;
	  else showtokenlist ( mem [curinput .startfield ].hh .v.RH , 
	  curinput .locfield , 100000L ) ;
	} 
	selector = oldsetting ;
	if ( trickcount == 1000000L ) 
	{
	  firstcount = tally ;
	  trickcount = tally + 1 + errorline - halferrorline ;
	  if ( trickcount < errorline ) 
	  trickcount = errorline ;
	} 
	if ( tally < trickcount ) 
	m = tally - firstcount ;
	else m = trickcount - firstcount ;
	if ( l + firstcount <= halferrorline ) 
	{
	  p = 0 ;
	  n = l + firstcount ;
	} 
	else {
	    
	  print ( 275 ) ;
	  p = l + firstcount - halferrorline + 3 ;
	  n = halferrorline ;
	} 
	{register integer for_end; q = p ;for_end = firstcount - 1 ; if ( q 
	<= for_end) do 
	  printchar ( trickbuf [q % errorline ]) ;
	while ( q++ < for_end ) ;} 
	println () ;
	{register integer for_end; q = 1 ;for_end = n ; if ( q <= for_end) 
	do 
	  printchar ( 32 ) ;
	while ( q++ < for_end ) ;} 
	if ( m + n <= errorline ) 
	p = firstcount + m ;
	else p = firstcount + ( errorline - n - 3 ) ;
	{register integer for_end; q = firstcount ;for_end = p - 1 ; if ( q 
	<= for_end) do 
	  printchar ( trickbuf [q % errorline ]) ;
	while ( q++ < for_end ) ;} 
	if ( m + n > errorline ) 
	print ( 275 ) ;
	incr ( nn ) ;
      } 
    } 
    else if ( nn == eqtb [15217 ].cint ) 
    {
      printnl ( 275 ) ;
      incr ( nn ) ;
    } 
    if ( bottomline ) 
    goto lab30 ;
    decr ( baseptr ) ;
  } 
  lab30: curinput = inputstack [inputptr ];
} 
void 
#ifdef HAVE_PROTOTYPES
zbegintokenlist ( halfword p , quarterword t ) 
#else
zbegintokenlist ( p , t ) 
  halfword p ;
  quarterword t ;
#endif
{
  begintokenlist_regmem 
  {
    if ( inputptr > maxinstack ) 
    {
      maxinstack = inputptr ;
      if ( inputptr == stacksize ) 
      overflow ( 592 , stacksize ) ;
    } 
    inputstack [inputptr ]= curinput ;
    incr ( inputptr ) ;
  } 
  curinput .statefield = 0 ;
  curinput .startfield = p ;
  curinput .indexfield = t ;
  if ( t >= 5 ) 
  {
    incr ( mem [p ].hh .v.LH ) ;
    if ( t == 5 ) 
    curinput .limitfield = paramptr ;
    else {
	
      curinput .locfield = mem [p ].hh .v.RH ;
      if ( eqtb [15193 ].cint > 1 ) 
      {
	begindiagnostic () ;
	printnl ( 335 ) ;
	switch ( t ) 
	{case 14 : 
	  printesc ( 348 ) ;
	  break ;
	case 15 : 
	  printesc ( 593 ) ;
	  break ;
	  default: 
	  printcmdchr ( 72 , t + 13051 ) ;
	  break ;
	} 
	print ( 556 ) ;
	tokenshow ( p ) ;
	enddiagnostic ( false ) ;
      } 
    } 
  } 
  else curinput .locfield = p ;
} 
void 
#ifdef HAVE_PROTOTYPES
endtokenlist ( void ) 
#else
endtokenlist ( ) 
#endif
{
  endtokenlist_regmem 
  if ( curinput .indexfield >= 3 ) 
  {
    if ( curinput .indexfield <= 4 ) 
    flushlist ( curinput .startfield ) ;
    else {
	
      deletetokenref ( curinput .startfield ) ;
      if ( curinput .indexfield == 5 ) 
      while ( paramptr > curinput .limitfield ) {
	  
	decr ( paramptr ) ;
	flushlist ( paramstack [paramptr ]) ;
      } 
    } 
  } 
  else if ( curinput .indexfield == 1 ) 
  if ( alignstate > 500000L ) 
  alignstate = 0 ;
  else fatalerror ( 594 ) ;
  {
    decr ( inputptr ) ;
    curinput = inputstack [inputptr ];
  } 
  {
    if ( interrupt != 0 ) 
    pauseforinstructions () ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
backinput ( void ) 
#else
backinput ( ) 
#endif
{
  backinput_regmem 
  halfword p  ;
  while ( ( curinput .statefield == 0 ) && ( curinput .locfield == 0 ) ) 
  endtokenlist () ;
  p = getavail () ;
  mem [p ].hh .v.LH = curtok ;
  if ( curtok < 768 ) 
  if ( curtok < 512 ) 
  decr ( alignstate ) ;
  else incr ( alignstate ) ;
  {
    if ( inputptr > maxinstack ) 
    {
      maxinstack = inputptr ;
      if ( inputptr == stacksize ) 
      overflow ( 592 , stacksize ) ;
    } 
    inputstack [inputptr ]= curinput ;
    incr ( inputptr ) ;
  } 
  curinput .statefield = 0 ;
  curinput .startfield = p ;
  curinput .indexfield = 3 ;
  curinput .locfield = p ;
} 
void 
#ifdef HAVE_PROTOTYPES
backerror ( void ) 
#else
backerror ( ) 
#endif
{
  backerror_regmem 
  OKtointerrupt = false ;
  backinput () ;
  OKtointerrupt = true ;
  error () ;
} 
void 
#ifdef HAVE_PROTOTYPES
inserror ( void ) 
#else
inserror ( ) 
#endif
{
  inserror_regmem 
  OKtointerrupt = false ;
  backinput () ;
  curinput .indexfield = 4 ;
  OKtointerrupt = true ;
  error () ;
} 
void 
#ifdef HAVE_PROTOTYPES
beginfilereading ( void ) 
#else
beginfilereading ( ) 
#endif
{
  beginfilereading_regmem 
  if ( inopen == maxinopen ) 
  overflow ( 595 , maxinopen ) ;
  if ( first == bufsize ) 
  overflow ( 256 , bufsize ) ;
  incr ( inopen ) ;
  {
    if ( inputptr > maxinstack ) 
    {
      maxinstack = inputptr ;
      if ( inputptr == stacksize ) 
      overflow ( 592 , stacksize ) ;
    } 
    inputstack [inputptr ]= curinput ;
    incr ( inputptr ) ;
  } 
  curinput .indexfield = inopen ;
  linestack [curinput .indexfield ]= line ;
  curinput .startfield = first ;
  curinput .statefield = 1 ;
  curinput .namefield = 0 ;
} 
void 
#ifdef HAVE_PROTOTYPES
endfilereading ( void ) 
#else
endfilereading ( ) 
#endif
{
  endfilereading_regmem 
  first = curinput .startfield ;
  line = linestack [curinput .indexfield ];
  if ( curinput .namefield > 17 ) 
  aclose ( inputfile [curinput .indexfield ]) ;
  {
    decr ( inputptr ) ;
    curinput = inputstack [inputptr ];
  } 
  decr ( inopen ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
clearforerrorprompt ( void ) 
#else
clearforerrorprompt ( ) 
#endif
{
  clearforerrorprompt_regmem 
  while ( ( curinput .statefield != 0 ) && ( curinput .namefield == 0 ) && ( 
  inputptr > 0 ) && ( curinput .locfield > curinput .limitfield ) ) 
  endfilereading () ;
  println () ;
} 
void 
#ifdef HAVE_PROTOTYPES
checkoutervalidity ( void ) 
#else
checkoutervalidity ( ) 
#endif
{
  checkoutervalidity_regmem 
  halfword p  ;
  halfword q  ;
  if ( scannerstatus != 0 ) 
  {
    deletionsallowed = false ;
    if ( curcs != 0 ) 
    {
      if ( ( curinput .statefield == 0 ) || ( curinput .namefield < 1 ) || ( 
      curinput .namefield > 17 ) ) 
      {
	p = getavail () ;
	mem [p ].hh .v.LH = 4095 + curcs ;
	begintokenlist ( p , 3 ) ;
      } 
      curcmd = 10 ;
      curchr = 32 ;
    } 
    if ( scannerstatus > 1 ) 
    {
      runaway () ;
      if ( curcs == 0 ) 
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 603 ) ;
      } 
      else {
	  
	curcs = 0 ;
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 604 ) ;
	} 
      } 
      print ( 605 ) ;
      p = getavail () ;
      switch ( scannerstatus ) 
      {case 2 : 
	{
	  print ( 570 ) ;
	  mem [p ].hh .v.LH = 637 ;
	} 
	break ;
      case 3 : 
	{
	  print ( 611 ) ;
	  mem [p ].hh .v.LH = partoken ;
	  longstate = 113 ;
	} 
	break ;
      case 4 : 
	{
	  print ( 572 ) ;
	  mem [p ].hh .v.LH = 637 ;
	  q = p ;
	  p = getavail () ;
	  mem [p ].hh .v.RH = q ;
	  mem [p ].hh .v.LH = 14610 ;
	  alignstate = -1000000L ;
	} 
	break ;
      case 5 : 
	{
	  print ( 573 ) ;
	  mem [p ].hh .v.LH = 637 ;
	} 
	break ;
      } 
      begintokenlist ( p , 4 ) ;
      print ( 606 ) ;
      sprintcs ( warningindex ) ;
      {
	helpptr = 4 ;
	helpline [3 ]= 607 ;
	helpline [2 ]= 608 ;
	helpline [1 ]= 609 ;
	helpline [0 ]= 610 ;
      } 
      error () ;
    } 
    else {
	
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 597 ) ;
      } 
      printcmdchr ( 105 , curif ) ;
      print ( 598 ) ;
      printint ( skipline ) ;
      {
	helpptr = 3 ;
	helpline [2 ]= 599 ;
	helpline [1 ]= 600 ;
	helpline [0 ]= 601 ;
      } 
      if ( curcs != 0 ) 
      curcs = 0 ;
      else helpline [2 ]= 602 ;
      curtok = 14613 ;
      inserror () ;
    } 
    deletionsallowed = true ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
getnext ( void ) 
#else
getnext ( ) 
#endif
{
  /* 20 25 21 26 40 10 */ getnext_regmem 
  integer k  ;
  halfword t  ;
  char cat  ;
  ASCIIcode c, cc  ;
  char d  ;
  lab20: curcs = 0 ;
  if ( curinput .statefield != 0 ) 
  {
    lab25: if ( curinput .locfield <= curinput .limitfield ) 
    {
      curchr = buffer [curinput .locfield ];
      incr ( curinput .locfield ) ;
      lab21: curcmd = eqtb [13627 + curchr ].hh .v.RH ;
      switch ( curinput .statefield + curcmd ) 
      {case 10 : 
      case 26 : 
      case 42 : 
      case 27 : 
      case 43 : 
	goto lab25 ;
	break ;
      case 1 : 
      case 17 : 
      case 33 : 
	{
	  if ( curinput .locfield > curinput .limitfield ) 
	  curcs = 513 ;
	  else {
	      
	    lab26: k = curinput .locfield ;
	    curchr = buffer [k ];
	    cat = eqtb [13627 + curchr ].hh .v.RH ;
	    incr ( k ) ;
	    if ( cat == 11 ) 
	    curinput .statefield = 17 ;
	    else if ( cat == 10 ) 
	    curinput .statefield = 17 ;
	    else curinput .statefield = 1 ;
	    if ( ( cat == 11 ) && ( k <= curinput .limitfield ) ) 
	    {
	      do {
		  curchr = buffer [k ];
		cat = eqtb [13627 + curchr ].hh .v.RH ;
		incr ( k ) ;
	      } while ( ! ( ( cat != 11 ) || ( k > curinput .limitfield ) ) ) 
	      ;
	      {
		if ( buffer [k ]== curchr ) 
		if ( cat == 7 ) 
		if ( k < curinput .limitfield ) 
		{
		  c = buffer [k + 1 ];
		  if ( c < 128 ) 
		  {
		    d = 2 ;
		    if ( ( ( ( c >= 48 ) && ( c <= 57 ) ) || ( ( c >= 97 ) && 
		    ( c <= 102 ) ) ) ) 
		    if ( k + 2 <= curinput .limitfield ) 
		    {
		      cc = buffer [k + 2 ];
		      if ( ( ( ( cc >= 48 ) && ( cc <= 57 ) ) || ( ( cc >= 97 
		      ) && ( cc <= 102 ) ) ) ) 
		      incr ( d ) ;
		    } 
		    if ( d > 2 ) 
		    {
		      if ( c <= 57 ) 
		      curchr = c - 48 ;
		      else curchr = c - 87 ;
		      if ( cc <= 57 ) 
		      curchr = 16 * curchr + cc - 48 ;
		      else curchr = 16 * curchr + cc - 87 ;
		      buffer [k - 1 ]= curchr ;
		    } 
		    else if ( c < 64 ) 
		    buffer [k - 1 ]= c + 64 ;
		    else buffer [k - 1 ]= c - 64 ;
		    curinput .limitfield = curinput .limitfield - d ;
		    first = first - d ;
		    while ( k <= curinput .limitfield ) {
			
		      buffer [k ]= buffer [k + d ];
		      incr ( k ) ;
		    } 
		    goto lab26 ;
		  } 
		} 
	      } 
	      if ( cat != 11 ) 
	      decr ( k ) ;
	      if ( k > curinput .locfield + 1 ) 
	      {
		curcs = idlookup ( curinput .locfield , k - curinput .locfield 
		) ;
		curinput .locfield = k ;
		goto lab40 ;
	      } 
	    } 
	    else {
		
	      if ( buffer [k ]== curchr ) 
	      if ( cat == 7 ) 
	      if ( k < curinput .limitfield ) 
	      {
		c = buffer [k + 1 ];
		if ( c < 128 ) 
		{
		  d = 2 ;
		  if ( ( ( ( c >= 48 ) && ( c <= 57 ) ) || ( ( c >= 97 ) && ( 
		  c <= 102 ) ) ) ) 
		  if ( k + 2 <= curinput .limitfield ) 
		  {
		    cc = buffer [k + 2 ];
		    if ( ( ( ( cc >= 48 ) && ( cc <= 57 ) ) || ( ( cc >= 97 ) 
		    && ( cc <= 102 ) ) ) ) 
		    incr ( d ) ;
		  } 
		  if ( d > 2 ) 
		  {
		    if ( c <= 57 ) 
		    curchr = c - 48 ;
		    else curchr = c - 87 ;
		    if ( cc <= 57 ) 
		    curchr = 16 * curchr + cc - 48 ;
		    else curchr = 16 * curchr + cc - 87 ;
		    buffer [k - 1 ]= curchr ;
		  } 
		  else if ( c < 64 ) 
		  buffer [k - 1 ]= c + 64 ;
		  else buffer [k - 1 ]= c - 64 ;
		  curinput .limitfield = curinput .limitfield - d ;
		  first = first - d ;
		  while ( k <= curinput .limitfield ) {
		      
		    buffer [k ]= buffer [k + d ];
		    incr ( k ) ;
		  } 
		  goto lab26 ;
		} 
	      } 
	    } 
	    curcs = 257 + buffer [curinput .locfield ];
	    incr ( curinput .locfield ) ;
	  } 
	  lab40: curcmd = eqtb [curcs ].hh.b0 ;
	  curchr = eqtb [curcs ].hh .v.RH ;
	  if ( curcmd >= 113 ) 
	  checkoutervalidity () ;
	} 
	break ;
      case 14 : 
      case 30 : 
      case 46 : 
	{
	  curcs = curchr + 1 ;
	  curcmd = eqtb [curcs ].hh.b0 ;
	  curchr = eqtb [curcs ].hh .v.RH ;
	  curinput .statefield = 1 ;
	  if ( curcmd >= 113 ) 
	  checkoutervalidity () ;
	} 
	break ;
      case 8 : 
      case 24 : 
      case 40 : 
	{
	  if ( curchr == buffer [curinput .locfield ]) 
	  if ( curinput .locfield < curinput .limitfield ) 
	  {
	    c = buffer [curinput .locfield + 1 ];
	    if ( c < 128 ) 
	    {
	      curinput .locfield = curinput .locfield + 2 ;
	      if ( ( ( ( c >= 48 ) && ( c <= 57 ) ) || ( ( c >= 97 ) && ( c <= 
	      102 ) ) ) ) 
	      if ( curinput .locfield <= curinput .limitfield ) 
	      {
		cc = buffer [curinput .locfield ];
		if ( ( ( ( cc >= 48 ) && ( cc <= 57 ) ) || ( ( cc >= 97 ) && ( 
		cc <= 102 ) ) ) ) 
		{
		  incr ( curinput .locfield ) ;
		  if ( c <= 57 ) 
		  curchr = c - 48 ;
		  else curchr = c - 87 ;
		  if ( cc <= 57 ) 
		  curchr = 16 * curchr + cc - 48 ;
		  else curchr = 16 * curchr + cc - 87 ;
		  goto lab21 ;
		} 
	      } 
	      if ( c < 64 ) 
	      curchr = c + 64 ;
	      else curchr = c - 64 ;
	      goto lab21 ;
	    } 
	  } 
	  curinput .statefield = 1 ;
	} 
	break ;
      case 16 : 
      case 32 : 
      case 48 : 
	{
	  {
	    if ( interaction == 3 ) 
	    ;
	    printnl ( 262 ) ;
	    print ( 612 ) ;
	  } 
	  {
	    helpptr = 2 ;
	    helpline [1 ]= 613 ;
	    helpline [0 ]= 614 ;
	  } 
	  deletionsallowed = false ;
	  error () ;
	  deletionsallowed = true ;
	  goto lab20 ;
	} 
	break ;
      case 11 : 
	{
	  curinput .statefield = 17 ;
	  curchr = 32 ;
	} 
	break ;
      case 6 : 
	{
	  curinput .locfield = curinput .limitfield + 1 ;
	  curcmd = 10 ;
	  curchr = 32 ;
	} 
	break ;
      case 22 : 
      case 15 : 
      case 31 : 
      case 47 : 
	{
	  curinput .locfield = curinput .limitfield + 1 ;
	  goto lab25 ;
	} 
	break ;
      case 38 : 
	{
	  curinput .locfield = curinput .limitfield + 1 ;
	  curcs = parloc ;
	  curcmd = eqtb [curcs ].hh.b0 ;
	  curchr = eqtb [curcs ].hh .v.RH ;
	  if ( curcmd >= 113 ) 
	  checkoutervalidity () ;
	} 
	break ;
      case 2 : 
	incr ( alignstate ) ;
	break ;
      case 18 : 
      case 34 : 
	{
	  curinput .statefield = 1 ;
	  incr ( alignstate ) ;
	} 
	break ;
      case 3 : 
	decr ( alignstate ) ;
	break ;
      case 19 : 
      case 35 : 
	{
	  curinput .statefield = 1 ;
	  decr ( alignstate ) ;
	} 
	break ;
      case 20 : 
      case 21 : 
      case 23 : 
      case 25 : 
      case 28 : 
      case 29 : 
      case 36 : 
      case 37 : 
      case 39 : 
      case 41 : 
      case 44 : 
      case 45 : 
	curinput .statefield = 1 ;
	break ;
	default: 
	;
	break ;
      } 
    } 
    else {
	
      curinput .statefield = 33 ;
      if ( curinput .namefield > 17 ) 
      {
	incr ( line ) ;
	first = curinput .startfield ;
	if ( ! forceeof ) 
	{
	  if ( inputln ( inputfile [curinput .indexfield ], true ) ) 
	  firmuptheline () ;
	  else forceeof = true ;
	} 
	if ( forceeof ) 
	{
	  printchar ( 41 ) ;
	  decr ( openparens ) ;
	  fflush ( stdout ) ;
	  forceeof = false ;
	  endfilereading () ;
	  checkoutervalidity () ;
	  goto lab20 ;
	} 
	if ( ( eqtb [15211 ].cint < 0 ) || ( eqtb [15211 ].cint > 255 ) ) 
	decr ( curinput .limitfield ) ;
	else buffer [curinput .limitfield ]= eqtb [15211 ].cint ;
	first = curinput .limitfield + 1 ;
	curinput .locfield = curinput .startfield ;
      } 
      else {
	  
	if ( ! ( curinput .namefield == 0 ) ) 
	{
	  curcmd = 0 ;
	  curchr = 0 ;
	  return ;
	} 
	if ( inputptr > 0 ) 
	{
	  endfilereading () ;
	  goto lab20 ;
	} 
	if ( selector < 18 ) 
	openlogfile () ;
	if ( interaction > 1 ) 
	{
	  if ( ( eqtb [15211 ].cint < 0 ) || ( eqtb [15211 ].cint > 255 ) 
	  ) 
	  incr ( curinput .limitfield ) ;
	  if ( curinput .limitfield == curinput .startfield ) 
	  printnl ( 615 ) ;
	  println () ;
	  first = curinput .startfield ;
	  {
	    ;
	    print ( 42 ) ;
	    terminput () ;
	  } 
	  curinput .limitfield = last ;
	  if ( ( eqtb [15211 ].cint < 0 ) || ( eqtb [15211 ].cint > 255 ) 
	  ) 
	  decr ( curinput .limitfield ) ;
	  else buffer [curinput .limitfield ]= eqtb [15211 ].cint ;
	  first = curinput .limitfield + 1 ;
	  curinput .locfield = curinput .startfield ;
	} 
	else fatalerror ( 616 ) ;
      } 
      {
	if ( interrupt != 0 ) 
	pauseforinstructions () ;
      } 
      goto lab25 ;
    } 
  } 
  else if ( curinput .locfield != 0 ) 
  {
    t = mem [curinput .locfield ].hh .v.LH ;
    curinput .locfield = mem [curinput .locfield ].hh .v.RH ;
    if ( t >= 4095 ) 
    {
      curcs = t - 4095 ;
      curcmd = eqtb [curcs ].hh.b0 ;
      curchr = eqtb [curcs ].hh .v.RH ;
      if ( curcmd >= 113 ) 
      if ( curcmd == 116 ) 
      {
	curcs = mem [curinput .locfield ].hh .v.LH - 4095 ;
	curinput .locfield = 0 ;
	curcmd = eqtb [curcs ].hh.b0 ;
	curchr = eqtb [curcs ].hh .v.RH ;
	if ( curcmd > 100 ) 
	{
	  curcmd = 0 ;
	  curchr = 257 ;
	} 
      } 
      else checkoutervalidity () ;
    } 
    else {
	
      curcmd = t / 256 ;
      curchr = t % 256 ;
      switch ( curcmd ) 
      {case 1 : 
	incr ( alignstate ) ;
	break ;
      case 2 : 
	decr ( alignstate ) ;
	break ;
      case 5 : 
	{
	  begintokenlist ( paramstack [curinput .limitfield + curchr - 1 ], 
	  0 ) ;
	  goto lab20 ;
	} 
	break ;
	default: 
	;
	break ;
      } 
    } 
  } 
  else {
      
    endtokenlist () ;
    goto lab20 ;
  } 
  if ( curcmd <= 5 ) 
  if ( curcmd >= 4 ) 
  if ( alignstate == 0 ) 
  {
    if ( scannerstatus == 4 ) 
    fatalerror ( 594 ) ;
    curcmd = mem [curalign + 5 ].hh .v.LH ;
    mem [curalign + 5 ].hh .v.LH = curchr ;
    if ( curcmd == 63 ) 
    begintokenlist ( memtop - 10 , 2 ) ;
    else begintokenlist ( mem [curalign + 2 ].cint , 2 ) ;
    alignstate = 1000000L ;
    goto lab20 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
firmuptheline ( void ) 
#else
firmuptheline ( ) 
#endif
{
  firmuptheline_regmem 
  integer k  ;
  curinput .limitfield = last ;
  if ( eqtb [15191 ].cint > 0 ) 
  if ( interaction > 1 ) 
  {
    ;
    println () ;
    if ( curinput .startfield < curinput .limitfield ) 
    {register integer for_end; k = curinput .startfield ;for_end = curinput 
    .limitfield - 1 ; if ( k <= for_end) do 
      print ( buffer [k ]) ;
    while ( k++ < for_end ) ;} 
    first = curinput .limitfield ;
    {
      ;
      print ( 617 ) ;
      terminput () ;
    } 
    if ( last > first ) 
    {
      {register integer for_end; k = first ;for_end = last - 1 ; if ( k <= 
      for_end) do 
	buffer [k + curinput .startfield - first ]= buffer [k ];
      while ( k++ < for_end ) ;} 
      curinput .limitfield = curinput .startfield + last - first ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
gettoken ( void ) 
#else
gettoken ( ) 
#endif
{
  gettoken_regmem 
  nonewcontrolsequence = false ;
  getnext () ;
  nonewcontrolsequence = true ;
  if ( curcs == 0 ) 
  curtok = ( curcmd * 256 ) + curchr ;
  else curtok = 4095 + curcs ;
} 
void 
#ifdef HAVE_PROTOTYPES
macrocall ( void ) 
#else
macrocall ( ) 
#endif
{
  /* 10 22 30 31 40 */ macrocall_regmem 
  halfword r  ;
  halfword p  ;
  halfword q  ;
  halfword s  ;
  halfword t  ;
  halfword u, v  ;
  halfword rbraceptr  ;
  smallnumber n  ;
  halfword unbalance  ;
  halfword m  ;
  halfword refcount  ;
  smallnumber savescannerstatus  ;
  halfword savewarningindex  ;
  ASCIIcode matchchr  ;
  savescannerstatus = scannerstatus ;
  savewarningindex = warningindex ;
  warningindex = curcs ;
  refcount = curchr ;
  r = mem [refcount ].hh .v.RH ;
  n = 0 ;
  if ( eqtb [15193 ].cint > 0 ) 
  {
    begindiagnostic () ;
    println () ;
    printcs ( warningindex ) ;
    tokenshow ( refcount ) ;
    enddiagnostic ( false ) ;
  } 
  if ( mem [r ].hh .v.LH != 3584 ) 
  {
    scannerstatus = 3 ;
    unbalance = 0 ;
    longstate = eqtb [curcs ].hh.b0 ;
    if ( longstate >= 113 ) 
    longstate = longstate - 2 ;
    do {
	mem [memtop - 3 ].hh .v.RH = 0 ;
      if ( ( mem [r ].hh .v.LH > 3583 ) || ( mem [r ].hh .v.LH < 3328 ) ) 
      s = 0 ;
      else {
	  
	matchchr = mem [r ].hh .v.LH - 3328 ;
	s = mem [r ].hh .v.RH ;
	r = s ;
	p = memtop - 3 ;
	m = 0 ;
      } 
      lab22: gettoken () ;
      if ( curtok == mem [r ].hh .v.LH ) 
      {
	r = mem [r ].hh .v.RH ;
	if ( ( mem [r ].hh .v.LH >= 3328 ) && ( mem [r ].hh .v.LH <= 3584 
	) ) 
	{
	  if ( curtok < 512 ) 
	  decr ( alignstate ) ;
	  goto lab40 ;
	} 
	else goto lab22 ;
      } 
      if ( s != r ) 
      if ( s == 0 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 649 ) ;
	} 
	sprintcs ( warningindex ) ;
	print ( 650 ) ;
	{
	  helpptr = 4 ;
	  helpline [3 ]= 651 ;
	  helpline [2 ]= 652 ;
	  helpline [1 ]= 653 ;
	  helpline [0 ]= 654 ;
	} 
	error () ;
	goto lab10 ;
      } 
      else {
	  
	t = s ;
	do {
	    { 
	    q = getavail () ;
	    mem [p ].hh .v.RH = q ;
	    mem [q ].hh .v.LH = mem [t ].hh .v.LH ;
	    p = q ;
	  } 
	  incr ( m ) ;
	  u = mem [t ].hh .v.RH ;
	  v = s ;
	  while ( true ) {
	      
	    if ( u == r ) 
	    if ( curtok != mem [v ].hh .v.LH ) 
	    goto lab30 ;
	    else {
		
	      r = mem [v ].hh .v.RH ;
	      goto lab22 ;
	    } 
	    if ( mem [u ].hh .v.LH != mem [v ].hh .v.LH ) 
	    goto lab30 ;
	    u = mem [u ].hh .v.RH ;
	    v = mem [v ].hh .v.RH ;
	  } 
	  lab30: t = mem [t ].hh .v.RH ;
	} while ( ! ( t == r ) ) ;
	r = s ;
      } 
      if ( curtok == partoken ) 
      if ( longstate != 112 ) 
      {
	if ( longstate == 111 ) 
	{
	  runaway () ;
	  {
	    if ( interaction == 3 ) 
	    ;
	    printnl ( 262 ) ;
	    print ( 644 ) ;
	  } 
	  sprintcs ( warningindex ) ;
	  print ( 645 ) ;
	  {
	    helpptr = 3 ;
	    helpline [2 ]= 646 ;
	    helpline [1 ]= 647 ;
	    helpline [0 ]= 648 ;
	  } 
	  backerror () ;
	} 
	pstack [n ]= mem [memtop - 3 ].hh .v.RH ;
	alignstate = alignstate - unbalance ;
	{register integer for_end; m = 0 ;for_end = n ; if ( m <= for_end) 
	do 
	  flushlist ( pstack [m ]) ;
	while ( m++ < for_end ) ;} 
	goto lab10 ;
      } 
      if ( curtok < 768 ) 
      if ( curtok < 512 ) 
      {
	unbalance = 1 ;
	while ( true ) {
	    
	  {
	    {
	      q = avail ;
	      if ( q == 0 ) 
	      q = getavail () ;
	      else {
		  
		avail = mem [q ].hh .v.RH ;
		mem [q ].hh .v.RH = 0 ;
	;
#ifdef STAT
		incr ( dynused ) ;
#endif /* STAT */
	      } 
	    } 
	    mem [p ].hh .v.RH = q ;
	    mem [q ].hh .v.LH = curtok ;
	    p = q ;
	  } 
	  gettoken () ;
	  if ( curtok == partoken ) 
	  if ( longstate != 112 ) 
	  {
	    if ( longstate == 111 ) 
	    {
	      runaway () ;
	      {
		if ( interaction == 3 ) 
		;
		printnl ( 262 ) ;
		print ( 644 ) ;
	      } 
	      sprintcs ( warningindex ) ;
	      print ( 645 ) ;
	      {
		helpptr = 3 ;
		helpline [2 ]= 646 ;
		helpline [1 ]= 647 ;
		helpline [0 ]= 648 ;
	      } 
	      backerror () ;
	    } 
	    pstack [n ]= mem [memtop - 3 ].hh .v.RH ;
	    alignstate = alignstate - unbalance ;
	    {register integer for_end; m = 0 ;for_end = n ; if ( m <= 
	    for_end) do 
	      flushlist ( pstack [m ]) ;
	    while ( m++ < for_end ) ;} 
	    goto lab10 ;
	  } 
	  if ( curtok < 768 ) 
	  if ( curtok < 512 ) 
	  incr ( unbalance ) ;
	  else {
	      
	    decr ( unbalance ) ;
	    if ( unbalance == 0 ) 
	    goto lab31 ;
	  } 
	} 
	lab31: rbraceptr = p ;
	{
	  q = getavail () ;
	  mem [p ].hh .v.RH = q ;
	  mem [q ].hh .v.LH = curtok ;
	  p = q ;
	} 
      } 
      else {
	  
	backinput () ;
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 636 ) ;
	} 
	sprintcs ( warningindex ) ;
	print ( 637 ) ;
	{
	  helpptr = 6 ;
	  helpline [5 ]= 638 ;
	  helpline [4 ]= 639 ;
	  helpline [3 ]= 640 ;
	  helpline [2 ]= 641 ;
	  helpline [1 ]= 642 ;
	  helpline [0 ]= 643 ;
	} 
	incr ( alignstate ) ;
	longstate = 111 ;
	curtok = partoken ;
	inserror () ;
      } 
      else {
	  
	if ( curtok == 2592 ) 
	if ( mem [r ].hh .v.LH <= 3584 ) 
	if ( mem [r ].hh .v.LH >= 3328 ) 
	goto lab22 ;
	{
	  q = getavail () ;
	  mem [p ].hh .v.RH = q ;
	  mem [q ].hh .v.LH = curtok ;
	  p = q ;
	} 
      } 
      incr ( m ) ;
      if ( mem [r ].hh .v.LH > 3584 ) 
      goto lab22 ;
      if ( mem [r ].hh .v.LH < 3328 ) 
      goto lab22 ;
      lab40: if ( s != 0 ) 
      {
	if ( ( m == 1 ) && ( mem [p ].hh .v.LH < 768 ) && ( p != memtop - 3 
	) ) 
	{
	  mem [rbraceptr ].hh .v.RH = 0 ;
	  {
	    mem [p ].hh .v.RH = avail ;
	    avail = p ;
	;
#ifdef STAT
	    decr ( dynused ) ;
#endif /* STAT */
	  } 
	  p = mem [memtop - 3 ].hh .v.RH ;
	  pstack [n ]= mem [p ].hh .v.RH ;
	  {
	    mem [p ].hh .v.RH = avail ;
	    avail = p ;
	;
#ifdef STAT
	    decr ( dynused ) ;
#endif /* STAT */
	  } 
	} 
	else pstack [n ]= mem [memtop - 3 ].hh .v.RH ;
	incr ( n ) ;
	if ( eqtb [15193 ].cint > 0 ) 
	{
	  begindiagnostic () ;
	  printnl ( matchchr ) ;
	  printint ( n ) ;
	  print ( 655 ) ;
	  showtokenlist ( pstack [n - 1 ], 0 , 1000 ) ;
	  enddiagnostic ( false ) ;
	} 
      } 
    } while ( ! ( mem [r ].hh .v.LH == 3584 ) ) ;
  } 
  while ( ( curinput .statefield == 0 ) && ( curinput .locfield == 0 ) ) 
  endtokenlist () ;
  begintokenlist ( refcount , 5 ) ;
  curinput .namefield = warningindex ;
  curinput .locfield = mem [r ].hh .v.RH ;
  if ( n > 0 ) 
  {
    if ( paramptr + n > maxparamstack ) 
    {
      maxparamstack = paramptr + n ;
      if ( maxparamstack > paramsize ) 
      overflow ( 635 , paramsize ) ;
    } 
    {register integer for_end; m = 0 ;for_end = n - 1 ; if ( m <= for_end) 
    do 
      paramstack [paramptr + m ]= pstack [m ];
    while ( m++ < for_end ) ;} 
    paramptr = paramptr + n ;
  } 
  lab10: scannerstatus = savescannerstatus ;
  warningindex = savewarningindex ;
} 
void 
#ifdef HAVE_PROTOTYPES
insertrelax ( void ) 
#else
insertrelax ( ) 
#endif
{
  insertrelax_regmem 
  curtok = 4095 + curcs ;
  backinput () ;
  curtok = 14616 ;
  backinput () ;
  curinput .indexfield = 4 ;
} 
void 
#ifdef HAVE_PROTOTYPES
expand ( void ) 
#else
expand ( ) 
#endif
{
  expand_regmem 
  halfword t  ;
  halfword p, q, r  ;
  integer j  ;
  integer cvbackup  ;
  smallnumber cvlbackup, radixbackup, cobackup  ;
  halfword backupbackup  ;
  smallnumber savescannerstatus  ;
  cvbackup = curval ;
  cvlbackup = curvallevel ;
  radixbackup = radix ;
  cobackup = curorder ;
  backupbackup = mem [memtop - 13 ].hh .v.RH ;
  if ( curcmd < 111 ) 
  {
    if ( eqtb [15199 ].cint > 1 ) 
    showcurcmdchr () ;
    switch ( curcmd ) 
    {case 110 : 
      {
	if ( curmark [curchr ]!= 0 ) 
	begintokenlist ( curmark [curchr ], 14 ) ;
      } 
      break ;
    case 102 : 
      {
	gettoken () ;
	t = curtok ;
	gettoken () ;
	if ( curcmd > 100 ) 
	expand () ;
	else backinput () ;
	curtok = t ;
	backinput () ;
      } 
      break ;
    case 103 : 
      {
	savescannerstatus = scannerstatus ;
	scannerstatus = 0 ;
	gettoken () ;
	scannerstatus = savescannerstatus ;
	t = curtok ;
	backinput () ;
	if ( t >= 4095 ) 
	{
	  p = getavail () ;
	  mem [p ].hh .v.LH = 14618 ;
	  mem [p ].hh .v.RH = curinput .locfield ;
	  curinput .startfield = p ;
	  curinput .locfield = p ;
	} 
      } 
      break ;
    case 107 : 
      {
	r = getavail () ;
	p = r ;
	do {
	    getxtoken () ;
	  if ( curcs == 0 ) 
	  {
	    q = getavail () ;
	    mem [p ].hh .v.RH = q ;
	    mem [q ].hh .v.LH = curtok ;
	    p = q ;
	  } 
	} while ( ! ( curcs != 0 ) ) ;
	if ( curcmd != 67 ) 
	{
	  {
	    if ( interaction == 3 ) 
	    ;
	    printnl ( 262 ) ;
	    print ( 624 ) ;
	  } 
	  printesc ( 505 ) ;
	  print ( 625 ) ;
	  {
	    helpptr = 2 ;
	    helpline [1 ]= 626 ;
	    helpline [0 ]= 627 ;
	  } 
	  backerror () ;
	} 
	j = first ;
	p = mem [r ].hh .v.RH ;
	while ( p != 0 ) {
	    
	  if ( j >= maxbufstack ) 
	  {
	    maxbufstack = j + 1 ;
	    if ( maxbufstack == bufsize ) 
	    overflow ( 256 , bufsize ) ;
	  } 
	  buffer [j ]= mem [p ].hh .v.LH % 256 ;
	  incr ( j ) ;
	  p = mem [p ].hh .v.RH ;
	} 
	if ( j > first + 1 ) 
	{
	  nonewcontrolsequence = false ;
	  curcs = idlookup ( first , j - first ) ;
	  nonewcontrolsequence = true ;
	} 
	else if ( j == first ) 
	curcs = 513 ;
	else curcs = 257 + buffer [first ];
	flushlist ( r ) ;
	if ( eqtb [curcs ].hh.b0 == 101 ) 
	{
	  eqdefine ( curcs , 0 , 256 ) ;
	} 
	curtok = curcs + 4095 ;
	backinput () ;
      } 
      break ;
    case 108 : 
      convtoks () ;
      break ;
    case 109 : 
      insthetoks () ;
      break ;
    case 105 : 
      conditional () ;
      break ;
    case 106 : 
      if ( curchr > iflimit ) 
      if ( iflimit == 1 ) 
      insertrelax () ;
      else {
	  
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 775 ) ;
	} 
	printcmdchr ( 106 , curchr ) ;
	{
	  helpptr = 1 ;
	  helpline [0 ]= 776 ;
	} 
	error () ;
      } 
      else {
	  
	while ( curchr != 2 ) passtext () ;
	{
	  p = condptr ;
	  ifline = mem [p + 1 ].cint ;
	  curif = mem [p ].hh.b1 ;
	  iflimit = mem [p ].hh.b0 ;
	  condptr = mem [p ].hh .v.RH ;
	  freenode ( p , 2 ) ;
	} 
      } 
      break ;
    case 104 : 
      if ( curchr > 0 ) 
      forceeof = true ;
      else if ( nameinprogress ) 
      insertrelax () ;
      else startinput () ;
      break ;
      default: 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 618 ) ;
	} 
	{
	  helpptr = 5 ;
	  helpline [4 ]= 619 ;
	  helpline [3 ]= 620 ;
	  helpline [2 ]= 621 ;
	  helpline [1 ]= 622 ;
	  helpline [0 ]= 623 ;
	} 
	error () ;
      } 
      break ;
    } 
  } 
  else if ( curcmd < 115 ) 
  macrocall () ;
  else {
      
    curtok = 14615 ;
    backinput () ;
  } 
  curval = cvbackup ;
  curvallevel = cvlbackup ;
  radix = radixbackup ;
  curorder = cobackup ;
  mem [memtop - 13 ].hh .v.RH = backupbackup ;
} 
void 
#ifdef HAVE_PROTOTYPES
getxtoken ( void ) 
#else
getxtoken ( ) 
#endif
{
  /* 20 30 */ getxtoken_regmem 
  lab20: getnext () ;
  if ( curcmd <= 100 ) 
  goto lab30 ;
  if ( curcmd >= 111 ) 
  if ( curcmd < 115 ) 
  macrocall () ;
  else {
      
    curcs = 10520 ;
    curcmd = 9 ;
    goto lab30 ;
  } 
  else expand () ;
  goto lab20 ;
  lab30: if ( curcs == 0 ) 
  curtok = ( curcmd * 256 ) + curchr ;
  else curtok = 4095 + curcs ;
} 
void 
#ifdef HAVE_PROTOTYPES
xtoken ( void ) 
#else
xtoken ( ) 
#endif
{
  xtoken_regmem 
  while ( curcmd > 100 ) {
      
    expand () ;
    getnext () ;
  } 
  if ( curcs == 0 ) 
  curtok = ( curcmd * 256 ) + curchr ;
  else curtok = 4095 + curcs ;
} 
void 
#ifdef HAVE_PROTOTYPES
scanleftbrace ( void ) 
#else
scanleftbrace ( ) 
#endif
{
  scanleftbrace_regmem 
  do {
      getxtoken () ;
  } while ( ! ( ( curcmd != 10 ) && ( curcmd != 0 ) ) ) ;
  if ( curcmd != 1 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 656 ) ;
    } 
    {
      helpptr = 4 ;
      helpline [3 ]= 657 ;
      helpline [2 ]= 658 ;
      helpline [1 ]= 659 ;
      helpline [0 ]= 660 ;
    } 
    backerror () ;
    curtok = 379 ;
    curcmd = 1 ;
    curchr = 123 ;
    incr ( alignstate ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
scanoptionalequals ( void ) 
#else
scanoptionalequals ( ) 
#endif
{
  scanoptionalequals_regmem 
  do {
      getxtoken () ;
  } while ( ! ( curcmd != 10 ) ) ;
  if ( curtok != 3133 ) 
  backinput () ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
zscankeyword ( strnumber s ) 
#else
zscankeyword ( s ) 
  strnumber s ;
#endif
{
  /* 10 */ register boolean Result; scankeyword_regmem 
  halfword p  ;
  halfword q  ;
  poolpointer k  ;
  p = memtop - 13 ;
  mem [p ].hh .v.RH = 0 ;
  k = strstart [s ];
  while ( k < strstart [s + 1 ]) {
      
    getxtoken () ;
    if ( ( curcs == 0 ) && ( ( curchr == strpool [k ]) || ( curchr == 
    strpool [k ]- 32 ) ) ) 
    {
      {
	q = getavail () ;
	mem [p ].hh .v.RH = q ;
	mem [q ].hh .v.LH = curtok ;
	p = q ;
      } 
      incr ( k ) ;
    } 
    else if ( ( curcmd != 10 ) || ( p != memtop - 13 ) ) 
    {
      backinput () ;
      if ( p != memtop - 13 ) 
      begintokenlist ( mem [memtop - 13 ].hh .v.RH , 3 ) ;
      Result = false ;
      return Result ;
    } 
  } 
  flushlist ( mem [memtop - 13 ].hh .v.RH ) ;
  Result = true ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
muerror ( void ) 
#else
muerror ( ) 
#endif
{
  muerror_regmem 
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 262 ) ;
    print ( 661 ) ;
  } 
  {
    helpptr = 1 ;
    helpline [0 ]= 662 ;
  } 
  error () ;
} 
void 
#ifdef HAVE_PROTOTYPES
scaneightbitint ( void ) 
#else
scaneightbitint ( ) 
#endif
{
  scaneightbitint_regmem 
  scanint () ;
  if ( ( curval < 0 ) || ( curval > 255 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 686 ) ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 687 ;
      helpline [0 ]= 688 ;
    } 
    interror ( curval ) ;
    curval = 0 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
scancharnum ( void ) 
#else
scancharnum ( ) 
#endif
{
  scancharnum_regmem 
  scanint () ;
  if ( ( curval < 0 ) || ( curval > 255 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 689 ) ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 690 ;
      helpline [0 ]= 688 ;
    } 
    interror ( curval ) ;
    curval = 0 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
scanfourbitint ( void ) 
#else
scanfourbitint ( ) 
#endif
{
  scanfourbitint_regmem 
  scanint () ;
  if ( ( curval < 0 ) || ( curval > 15 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 691 ) ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 692 ;
      helpline [0 ]= 688 ;
    } 
    interror ( curval ) ;
    curval = 0 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
scanfifteenbitint ( void ) 
#else
scanfifteenbitint ( ) 
#endif
{
  scanfifteenbitint_regmem 
  scanint () ;
  if ( ( curval < 0 ) || ( curval > 32767 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 693 ) ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 694 ;
      helpline [0 ]= 688 ;
    } 
    interror ( curval ) ;
    curval = 0 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
scantwentysevenbitint ( void ) 
#else
scantwentysevenbitint ( ) 
#endif
{
  scantwentysevenbitint_regmem 
  scanint () ;
  if ( ( curval < 0 ) || ( curval > 134217727L ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 695 ) ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 696 ;
      helpline [0 ]= 688 ;
    } 
    interror ( curval ) ;
    curval = 0 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
scanfontident ( void ) 
#else
scanfontident ( ) 
#endif
{
  scanfontident_regmem 
  internalfontnumber f  ;
  halfword m  ;
  do {
      getxtoken () ;
  } while ( ! ( curcmd != 10 ) ) ;
  if ( curcmd == 88 ) 
  f = eqtb [13578 ].hh .v.RH ;
  else if ( curcmd == 87 ) 
  f = curchr ;
  else if ( curcmd == 86 ) 
  {
    m = curchr ;
    scanfourbitint () ;
    f = eqtb [m + curval ].hh .v.RH ;
  } 
  else {
      
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 812 ) ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 813 ;
      helpline [0 ]= 814 ;
    } 
    backerror () ;
    f = 0 ;
  } 
  curval = f ;
} 
void 
#ifdef HAVE_PROTOTYPES
zfindfontdimen ( boolean writing ) 
#else
zfindfontdimen ( writing ) 
  boolean writing ;
#endif
{
  findfontdimen_regmem 
  internalfontnumber f  ;
  integer n  ;
  scanint () ;
  n = curval ;
  scanfontident () ;
  f = curval ;
  if ( n <= 0 ) 
  curval = fmemptr ;
  else {
      
    if ( writing && ( n <= 4 ) && ( n >= 2 ) && ( fontglue [f ]!= 0 ) ) 
    {
      deleteglueref ( fontglue [f ]) ;
      fontglue [f ]= 0 ;
    } 
    if ( n > fontparams [f ]) 
    if ( f < fontptr ) 
    curval = fmemptr ;
    else {
	
      do {
	  if ( fmemptr == fontmemsize ) 
	overflow ( 819 , fontmemsize ) ;
	fontinfo [fmemptr ].cint = 0 ;
	incr ( fmemptr ) ;
	incr ( fontparams [f ]) ;
      } while ( ! ( n == fontparams [f ]) ) ;
      curval = fmemptr - 1 ;
    } 
    else curval = n + parambase [f ];
  } 
  if ( curval == fmemptr ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 798 ) ;
    } 
    printesc ( hash [10524 + f ].v.RH ) ;
    print ( 815 ) ;
    printint ( fontparams [f ]) ;
    print ( 816 ) ;
    {
      helpptr = 2 ;
      helpline [1 ]= 817 ;
      helpline [0 ]= 818 ;
    } 
    error () ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zscansomethinginternal ( smallnumber level , boolean negative ) 
#else
zscansomethinginternal ( level , negative ) 
  smallnumber level ;
  boolean negative ;
#endif
{
  scansomethinginternal_regmem 
  halfword m  ;
  integer p  ;
  m = curchr ;
  switch ( curcmd ) 
  {case 85 : 
    {
      scancharnum () ;
      if ( m == 14651 ) 
      {
	curval = eqtb [14651 + curval ].hh .v.RH ;
	curvallevel = 0 ;
      } 
      else if ( m < 14651 ) 
      {
	curval = eqtb [m + curval ].hh .v.RH ;
	curvallevel = 0 ;
      } 
      else {
	  
	curval = eqtb [m + curval ].cint ;
	curvallevel = 0 ;
      } 
    } 
    break ;
  case 71 : 
  case 72 : 
  case 86 : 
  case 87 : 
  case 88 : 
    if ( level != 5 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 663 ) ;
      } 
      {
	helpptr = 3 ;
	helpline [2 ]= 664 ;
	helpline [1 ]= 665 ;
	helpline [0 ]= 666 ;
      } 
      backerror () ;
      {
	curval = 0 ;
	curvallevel = 1 ;
      } 
    } 
    else if ( curcmd <= 72 ) 
    {
      if ( curcmd < 72 ) 
      {
	scaneightbitint () ;
	m = 13066 + curval ;
      } 
      {
	curval = eqtb [m ].hh .v.RH ;
	curvallevel = 5 ;
      } 
    } 
    else {
	
      backinput () ;
      scanfontident () ;
      {
	curval = 10524 + curval ;
	curvallevel = 4 ;
      } 
    } 
    break ;
  case 73 : 
    {
      curval = eqtb [m ].cint ;
      curvallevel = 0 ;
    } 
    break ;
  case 74 : 
    {
      curval = eqtb [m ].cint ;
      curvallevel = 1 ;
    } 
    break ;
  case 75 : 
    {
      curval = eqtb [m ].hh .v.RH ;
      curvallevel = 2 ;
    } 
    break ;
  case 76 : 
    {
      curval = eqtb [m ].hh .v.RH ;
      curvallevel = 3 ;
    } 
    break ;
  case 79 : 
    if ( abs ( curlist .modefield ) != m ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 679 ) ;
      } 
      printcmdchr ( 79 , m ) ;
      {
	helpptr = 4 ;
	helpline [3 ]= 680 ;
	helpline [2 ]= 681 ;
	helpline [1 ]= 682 ;
	helpline [0 ]= 683 ;
      } 
      error () ;
      if ( level != 5 ) 
      {
	curval = 0 ;
	curvallevel = 1 ;
      } 
      else {
	  
	curval = 0 ;
	curvallevel = 0 ;
      } 
    } 
    else if ( m == 1 ) 
    {
      curval = curlist .auxfield .cint ;
      curvallevel = 1 ;
    } 
    else {
	
      curval = curlist .auxfield .hh .v.LH ;
      curvallevel = 0 ;
    } 
    break ;
  case 80 : 
    if ( curlist .modefield == 0 ) 
    {
      curval = 0 ;
      curvallevel = 0 ;
    } 
    else {
	
      nest [nestptr ]= curlist ;
      p = nestptr ;
      while ( abs ( nest [p ].modefield ) != 1 ) decr ( p ) ;
      {
	curval = nest [p ].pgfield ;
	curvallevel = 0 ;
      } 
    } 
    break ;
  case 82 : 
    {
      if ( m == 0 ) 
      curval = deadcycles ;
      else curval = insertpenalties ;
      curvallevel = 0 ;
    } 
    break ;
  case 81 : 
    {
      if ( ( pagecontents == 0 ) && ( ! outputactive ) ) 
      if ( m == 0 ) 
      curval = 1073741823L ;
      else curval = 0 ;
      else curval = pagesofar [m ];
      curvallevel = 1 ;
    } 
    break ;
  case 84 : 
    {
      if ( eqtb [13056 ].hh .v.RH == 0 ) 
      curval = 0 ;
      else curval = mem [eqtb [13056 ].hh .v.RH ].hh .v.LH ;
      curvallevel = 0 ;
    } 
    break ;
  case 83 : 
    {
      scaneightbitint () ;
      if ( eqtb [13322 + curval ].hh .v.RH == 0 ) 
      curval = 0 ;
      else curval = mem [eqtb [13322 + curval ].hh .v.RH + m ].cint ;
      curvallevel = 1 ;
    } 
    break ;
  case 68 : 
  case 69 : 
    {
      curval = curchr ;
      curvallevel = 0 ;
    } 
    break ;
  case 77 : 
    {
      findfontdimen ( false ) ;
      fontinfo [fmemptr ].cint = 0 ;
      {
	curval = fontinfo [curval ].cint ;
	curvallevel = 1 ;
      } 
    } 
    break ;
  case 78 : 
    {
      scanfontident () ;
      if ( m == 0 ) 
      {
	curval = hyphenchar [curval ];
	curvallevel = 0 ;
      } 
      else {
	  
	curval = skewchar [curval ];
	curvallevel = 0 ;
      } 
    } 
    break ;
  case 89 : 
    {
      scaneightbitint () ;
      switch ( m ) 
      {case 0 : 
	curval = eqtb [15221 + curval ].cint ;
	break ;
      case 1 : 
	curval = eqtb [15754 + curval ].cint ;
	break ;
      case 2 : 
	curval = eqtb [12544 + curval ].hh .v.RH ;
	break ;
      case 3 : 
	curval = eqtb [12800 + curval ].hh .v.RH ;
	break ;
      } 
      curvallevel = m ;
    } 
    break ;
  case 70 : 
    if ( curchr > 2 ) 
    {
      if ( curchr == 3 ) 
      curval = line ;
      else curval = lastbadness ;
      curvallevel = 0 ;
    } 
    else {
	
      if ( curchr == 2 ) 
      curval = membot ;
      else curval = 0 ;
      curvallevel = curchr ;
      if ( ! ( curlist .tailfield >= himemmin ) && ( curlist .modefield != 0 ) 
      ) 
      switch ( curchr ) 
      {case 0 : 
	if ( mem [curlist .tailfield ].hh.b0 == 12 ) 
	curval = mem [curlist .tailfield + 1 ].cint ;
	break ;
      case 1 : 
	if ( mem [curlist .tailfield ].hh.b0 == 11 ) 
	curval = mem [curlist .tailfield + 1 ].cint ;
	break ;
      case 2 : 
	if ( mem [curlist .tailfield ].hh.b0 == 10 ) 
	{
	  curval = mem [curlist .tailfield + 1 ].hh .v.LH ;
	  if ( mem [curlist .tailfield ].hh.b1 == 99 ) 
	  curvallevel = 3 ;
	} 
	break ;
      } 
      else if ( ( curlist .modefield == 1 ) && ( curlist .tailfield == curlist 
      .headfield ) ) 
      switch ( curchr ) 
      {case 0 : 
	curval = lastpenalty ;
	break ;
      case 1 : 
	curval = lastkern ;
	break ;
      case 2 : 
	if ( lastglue != 268435455L ) 
	curval = lastglue ;
	break ;
      } 
    } 
    break ;
    default: 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 684 ) ;
      } 
      printcmdchr ( curcmd , curchr ) ;
      print ( 685 ) ;
      printesc ( 537 ) ;
      {
	helpptr = 1 ;
	helpline [0 ]= 683 ;
      } 
      error () ;
      if ( level != 5 ) 
      {
	curval = 0 ;
	curvallevel = 1 ;
      } 
      else {
	  
	curval = 0 ;
	curvallevel = 0 ;
      } 
    } 
    break ;
  } 
  while ( curvallevel > level ) {
      
    if ( curvallevel == 2 ) 
    curval = mem [curval + 1 ].cint ;
    else if ( curvallevel == 3 ) 
    muerror () ;
    decr ( curvallevel ) ;
  } 
  if ( negative ) 
  if ( curvallevel >= 2 ) 
  {
    curval = newspec ( curval ) ;
    {
      mem [curval + 1 ].cint = - (integer) mem [curval + 1 ].cint ;
      mem [curval + 2 ].cint = - (integer) mem [curval + 2 ].cint ;
      mem [curval + 3 ].cint = - (integer) mem [curval + 3 ].cint ;
    } 
  } 
  else curval = - (integer) curval ;
  else if ( ( curvallevel >= 2 ) && ( curvallevel <= 3 ) ) 
  incr ( mem [curval ].hh .v.RH ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
scanint ( void ) 
#else
scanint ( ) 
#endif
{
  /* 30 */ scanint_regmem 
  boolean negative  ;
  integer m  ;
  smallnumber d  ;
  boolean vacuous  ;
  boolean OKsofar  ;
  radix = 0 ;
  OKsofar = true ;
  negative = false ;
  do {
      do { getxtoken () ;
    } while ( ! ( curcmd != 10 ) ) ;
    if ( curtok == 3117 ) 
    {
      negative = ! negative ;
      curtok = 3115 ;
    } 
  } while ( ! ( curtok != 3115 ) ) ;
  if ( curtok == 3168 ) 
  {
    gettoken () ;
    if ( curtok < 4095 ) 
    {
      curval = curchr ;
      if ( curcmd <= 2 ) 
      if ( curcmd == 2 ) 
      incr ( alignstate ) ;
      else decr ( alignstate ) ;
    } 
    else if ( curtok < 4352 ) 
    curval = curtok - 4096 ;
    else curval = curtok - 4352 ;
    if ( curval > 255 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 697 ) ;
      } 
      {
	helpptr = 2 ;
	helpline [1 ]= 698 ;
	helpline [0 ]= 699 ;
      } 
      curval = 48 ;
      backerror () ;
    } 
    else {
	
      getxtoken () ;
      if ( curcmd != 10 ) 
      backinput () ;
    } 
  } 
  else if ( ( curcmd >= 68 ) && ( curcmd <= 89 ) ) 
  scansomethinginternal ( 0 , false ) ;
  else {
      
    radix = 10 ;
    m = 214748364L ;
    if ( curtok == 3111 ) 
    {
      radix = 8 ;
      m = 268435456L ;
      getxtoken () ;
    } 
    else if ( curtok == 3106 ) 
    {
      radix = 16 ;
      m = 134217728L ;
      getxtoken () ;
    } 
    vacuous = true ;
    curval = 0 ;
    while ( true ) {
	
      if ( ( curtok < 3120 + radix ) && ( curtok >= 3120 ) && ( curtok <= 3129 
      ) ) 
      d = curtok - 3120 ;
      else if ( radix == 16 ) 
      if ( ( curtok <= 2886 ) && ( curtok >= 2881 ) ) 
      d = curtok - 2871 ;
      else if ( ( curtok <= 3142 ) && ( curtok >= 3137 ) ) 
      d = curtok - 3127 ;
      else goto lab30 ;
      else goto lab30 ;
      vacuous = false ;
      if ( ( curval >= m ) && ( ( curval > m ) || ( d > 7 ) || ( radix != 10 ) 
      ) ) 
      {
	if ( OKsofar ) 
	{
	  {
	    if ( interaction == 3 ) 
	    ;
	    printnl ( 262 ) ;
	    print ( 700 ) ;
	  } 
	  {
	    helpptr = 2 ;
	    helpline [1 ]= 701 ;
	    helpline [0 ]= 702 ;
	  } 
	  error () ;
	  curval = 2147483647L ;
	  OKsofar = false ;
	} 
      } 
      else curval = curval * radix + d ;
      getxtoken () ;
    } 
    lab30: ;
    if ( vacuous ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 663 ) ;
      } 
      {
	helpptr = 3 ;
	helpline [2 ]= 664 ;
	helpline [1 ]= 665 ;
	helpline [0 ]= 666 ;
      } 
      backerror () ;
    } 
    else if ( curcmd != 10 ) 
    backinput () ;
  } 
  if ( negative ) 
  curval = - (integer) curval ;
} 
void 
#ifdef HAVE_PROTOTYPES
zscandimen ( boolean mu , boolean inf , boolean shortcut ) 
#else
zscandimen ( mu , inf , shortcut ) 
  boolean mu ;
  boolean inf ;
  boolean shortcut ;
#endif
{
  /* 30 31 32 40 45 88 89 */ scandimen_regmem 
  boolean negative  ;
  integer f  ;
  integer num, denom  ;
  smallnumber k, kk  ;
  halfword p, q  ;
  scaled v  ;
  integer savecurval  ;
  f = 0 ;
  aritherror = false ;
  curorder = 0 ;
  negative = false ;
  if ( ! shortcut ) 
  {
    negative = false ;
    do {
	do { getxtoken () ;
      } while ( ! ( curcmd != 10 ) ) ;
      if ( curtok == 3117 ) 
      {
	negative = ! negative ;
	curtok = 3115 ;
      } 
    } while ( ! ( curtok != 3115 ) ) ;
    if ( ( curcmd >= 68 ) && ( curcmd <= 89 ) ) 
    if ( mu ) 
    {
      scansomethinginternal ( 3 , false ) ;
      if ( curvallevel >= 2 ) 
      {
	v = mem [curval + 1 ].cint ;
	deleteglueref ( curval ) ;
	curval = v ;
      } 
      if ( curvallevel == 3 ) 
      goto lab89 ;
      if ( curvallevel != 0 ) 
      muerror () ;
    } 
    else {
	
      scansomethinginternal ( 1 , false ) ;
      if ( curvallevel == 1 ) 
      goto lab89 ;
    } 
    else {
	
      backinput () ;
      if ( curtok == 3116 ) 
      curtok = 3118 ;
      if ( curtok != 3118 ) 
      scanint () ;
      else {
	  
	radix = 10 ;
	curval = 0 ;
      } 
      if ( curtok == 3116 ) 
      curtok = 3118 ;
      if ( ( radix == 10 ) && ( curtok == 3118 ) ) 
      {
	k = 0 ;
	p = 0 ;
	gettoken () ;
	while ( true ) {
	    
	  getxtoken () ;
	  if ( ( curtok > 3129 ) || ( curtok < 3120 ) ) 
	  goto lab31 ;
	  if ( k < 17 ) 
	  {
	    q = getavail () ;
	    mem [q ].hh .v.RH = p ;
	    mem [q ].hh .v.LH = curtok - 3120 ;
	    p = q ;
	    incr ( k ) ;
	  } 
	} 
	lab31: {
	    register integer for_end; kk = k ;for_end = 1 ; if ( kk >= 
	for_end) do 
	  {
	    dig [kk - 1 ]= mem [p ].hh .v.LH ;
	    q = p ;
	    p = mem [p ].hh .v.RH ;
	    {
	      mem [q ].hh .v.RH = avail ;
	      avail = q ;
	;
#ifdef STAT
	      decr ( dynused ) ;
#endif /* STAT */
	    } 
	  } 
	while ( kk-- > for_end ) ;} 
	f = rounddecimals ( k ) ;
	if ( curcmd != 10 ) 
	backinput () ;
      } 
    } 
  } 
  if ( curval < 0 ) 
  {
    negative = ! negative ;
    curval = - (integer) curval ;
  } 
  if ( inf ) 
  if ( scankeyword ( 309 ) ) 
  {
    curorder = 1 ;
    while ( scankeyword ( 108 ) ) {
	
      if ( curorder == 3 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 704 ) ;
	} 
	print ( 705 ) ;
	{
	  helpptr = 1 ;
	  helpline [0 ]= 706 ;
	} 
	error () ;
      } 
      else incr ( curorder ) ;
    } 
    goto lab88 ;
  } 
  savecurval = curval ;
  do {
      getxtoken () ;
  } while ( ! ( curcmd != 10 ) ) ;
  if ( ( curcmd < 68 ) || ( curcmd > 89 ) ) 
  backinput () ;
  else {
      
    if ( mu ) 
    {
      scansomethinginternal ( 3 , false ) ;
      if ( curvallevel >= 2 ) 
      {
	v = mem [curval + 1 ].cint ;
	deleteglueref ( curval ) ;
	curval = v ;
      } 
      if ( curvallevel != 3 ) 
      muerror () ;
    } 
    else scansomethinginternal ( 1 , false ) ;
    v = curval ;
    goto lab40 ;
  } 
  if ( mu ) 
  goto lab45 ;
  if ( scankeyword ( 707 ) ) 
  v = ( fontinfo [6 + parambase [eqtb [13578 ].hh .v.RH ]].cint ) ;
  else if ( scankeyword ( 708 ) ) 
  v = ( fontinfo [5 + parambase [eqtb [13578 ].hh .v.RH ]].cint ) ;
  else goto lab45 ;
  {
    getxtoken () ;
    if ( curcmd != 10 ) 
    backinput () ;
  } 
  lab40: curval = multandadd ( savecurval , v , xnoverd ( v , f , 65536L ) , 
  1073741823L ) ;
  goto lab89 ;
  lab45: ;
  if ( mu ) 
  if ( scankeyword ( 334 ) ) 
  goto lab88 ;
  else {
      
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 704 ) ;
    } 
    print ( 709 ) ;
    {
      helpptr = 4 ;
      helpline [3 ]= 710 ;
      helpline [2 ]= 711 ;
      helpline [1 ]= 712 ;
      helpline [0 ]= 713 ;
    } 
    error () ;
    goto lab88 ;
  } 
  if ( scankeyword ( 703 ) ) 
  {
    preparemag () ;
    if ( eqtb [15180 ].cint != 1000 ) 
    {
      curval = xnoverd ( curval , 1000 , eqtb [15180 ].cint ) ;
      f = ( 1000 * f + 65536L * texremainder ) / eqtb [15180 ].cint ;
      curval = curval + ( f / 65536L ) ;
      f = f % 65536L ;
    } 
  } 
  if ( scankeyword ( 394 ) ) 
  goto lab88 ;
  if ( scankeyword ( 714 ) ) 
  {
    num = 7227 ;
    denom = 100 ;
  } 
  else if ( scankeyword ( 715 ) ) 
  {
    num = 12 ;
    denom = 1 ;
  } 
  else if ( scankeyword ( 716 ) ) 
  {
    num = 7227 ;
    denom = 254 ;
  } 
  else if ( scankeyword ( 717 ) ) 
  {
    num = 7227 ;
    denom = 2540 ;
  } 
  else if ( scankeyword ( 718 ) ) 
  {
    num = 7227 ;
    denom = 7200 ;
  } 
  else if ( scankeyword ( 719 ) ) 
  {
    num = 1238 ;
    denom = 1157 ;
  } 
  else if ( scankeyword ( 720 ) ) 
  {
    num = 14856 ;
    denom = 1157 ;
  } 
  else if ( scankeyword ( 721 ) ) 
  goto lab30 ;
  else {
      
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 704 ) ;
    } 
    print ( 722 ) ;
    {
      helpptr = 6 ;
      helpline [5 ]= 723 ;
      helpline [4 ]= 724 ;
      helpline [3 ]= 725 ;
      helpline [2 ]= 711 ;
      helpline [1 ]= 712 ;
      helpline [0 ]= 713 ;
    } 
    error () ;
    goto lab32 ;
  } 
  curval = xnoverd ( curval , num , denom ) ;
  f = ( num * f + 65536L * texremainder ) / denom ;
  curval = curval + ( f / 65536L ) ;
  f = f % 65536L ;
  lab32: ;
  lab88: if ( curval >= 16384 ) 
  aritherror = true ;
  else curval = curval * 65536L + f ;
  lab30: ;
  {
    getxtoken () ;
    if ( curcmd != 10 ) 
    backinput () ;
  } 
  lab89: if ( aritherror || ( abs ( curval ) >= 1073741824L ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 726 ) ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 727 ;
      helpline [0 ]= 728 ;
    } 
    error () ;
    curval = 1073741823L ;
    aritherror = false ;
  } 
  if ( negative ) 
  curval = - (integer) curval ;
} 
void 
#ifdef HAVE_PROTOTYPES
zscanglue ( smallnumber level ) 
#else
zscanglue ( level ) 
  smallnumber level ;
#endif
{
  /* 10 */ scanglue_regmem 
  boolean negative  ;
  halfword q  ;
  boolean mu  ;
  mu = ( level == 3 ) ;
  negative = false ;
  do {
      do { getxtoken () ;
    } while ( ! ( curcmd != 10 ) ) ;
    if ( curtok == 3117 ) 
    {
      negative = ! negative ;
      curtok = 3115 ;
    } 
  } while ( ! ( curtok != 3115 ) ) ;
  if ( ( curcmd >= 68 ) && ( curcmd <= 89 ) ) 
  {
    scansomethinginternal ( level , negative ) ;
    if ( curvallevel >= 2 ) 
    {
      if ( curvallevel != level ) 
      muerror () ;
      return ;
    } 
    if ( curvallevel == 0 ) 
    scandimen ( mu , false , true ) ;
    else if ( level == 3 ) 
    muerror () ;
  } 
  else {
      
    backinput () ;
    scandimen ( mu , false , false ) ;
    if ( negative ) 
    curval = - (integer) curval ;
  } 
  q = newspec ( membot ) ;
  mem [q + 1 ].cint = curval ;
  if ( scankeyword ( 729 ) ) 
  {
    scandimen ( mu , true , false ) ;
    mem [q + 2 ].cint = curval ;
    mem [q ].hh.b0 = curorder ;
  } 
  if ( scankeyword ( 730 ) ) 
  {
    scandimen ( mu , true , false ) ;
    mem [q + 3 ].cint = curval ;
    mem [q ].hh.b1 = curorder ;
  } 
  curval = q ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
scanrulespec ( void ) 
#else
scanrulespec ( ) 
#endif
{
  /* 21 */ register halfword Result; scanrulespec_regmem 
  halfword q  ;
  q = newrule () ;
  if ( curcmd == 35 ) 
  mem [q + 1 ].cint = 26214 ;
  else {
      
    mem [q + 3 ].cint = 26214 ;
    mem [q + 2 ].cint = 0 ;
  } 
  lab21: if ( scankeyword ( 731 ) ) 
  {
    scandimen ( false , false , false ) ;
    mem [q + 1 ].cint = curval ;
    goto lab21 ;
  } 
  if ( scankeyword ( 732 ) ) 
  {
    scandimen ( false , false , false ) ;
    mem [q + 3 ].cint = curval ;
    goto lab21 ;
  } 
  if ( scankeyword ( 733 ) ) 
  {
    scandimen ( false , false , false ) ;
    mem [q + 2 ].cint = curval ;
    goto lab21 ;
  } 
  Result = q ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zstrtoks ( poolpointer b ) 
#else
zstrtoks ( b ) 
  poolpointer b ;
#endif
{
  register halfword Result; strtoks_regmem 
  halfword p  ;
  halfword q  ;
  halfword t  ;
  poolpointer k  ;
  {
    if ( poolptr + 1 > poolsize ) 
    overflow ( 257 , poolsize - initpoolptr ) ;
  } 
  p = memtop - 3 ;
  mem [p ].hh .v.RH = 0 ;
  k = b ;
  while ( k < poolptr ) {
      
    t = strpool [k ];
    if ( t == 32 ) 
    t = 2592 ;
    else t = 3072 + t ;
    {
      {
	q = avail ;
	if ( q == 0 ) 
	q = getavail () ;
	else {
	    
	  avail = mem [q ].hh .v.RH ;
	  mem [q ].hh .v.RH = 0 ;
	;
#ifdef STAT
	  incr ( dynused ) ;
#endif /* STAT */
	} 
      } 
      mem [p ].hh .v.RH = q ;
      mem [q ].hh .v.LH = t ;
      p = q ;
    } 
    incr ( k ) ;
  } 
  poolptr = b ;
  Result = p ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
thetoks ( void ) 
#else
thetoks ( ) 
#endif
{
  register halfword Result; thetoks_regmem 
  char oldsetting  ;
  halfword p, q, r  ;
  poolpointer b  ;
  getxtoken () ;
  scansomethinginternal ( 5 , false ) ;
  if ( curvallevel >= 4 ) 
  {
    p = memtop - 3 ;
    mem [p ].hh .v.RH = 0 ;
    if ( curvallevel == 4 ) 
    {
      q = getavail () ;
      mem [p ].hh .v.RH = q ;
      mem [q ].hh .v.LH = 4095 + curval ;
      p = q ;
    } 
    else if ( curval != 0 ) 
    {
      r = mem [curval ].hh .v.RH ;
      while ( r != 0 ) {
	  
	{
	  {
	    q = avail ;
	    if ( q == 0 ) 
	    q = getavail () ;
	    else {
		
	      avail = mem [q ].hh .v.RH ;
	      mem [q ].hh .v.RH = 0 ;
	;
#ifdef STAT
	      incr ( dynused ) ;
#endif /* STAT */
	    } 
	  } 
	  mem [p ].hh .v.RH = q ;
	  mem [q ].hh .v.LH = mem [r ].hh .v.LH ;
	  p = q ;
	} 
	r = mem [r ].hh .v.RH ;
      } 
    } 
    Result = p ;
  } 
  else {
      
    oldsetting = selector ;
    selector = 21 ;
    b = poolptr ;
    switch ( curvallevel ) 
    {case 0 : 
      printint ( curval ) ;
      break ;
    case 1 : 
      {
	printscaled ( curval ) ;
	print ( 394 ) ;
      } 
      break ;
    case 2 : 
      {
	printspec ( curval , 394 ) ;
	deleteglueref ( curval ) ;
      } 
      break ;
    case 3 : 
      {
	printspec ( curval , 334 ) ;
	deleteglueref ( curval ) ;
      } 
      break ;
    } 
    selector = oldsetting ;
    Result = strtoks ( b ) ;
  } 
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
insthetoks ( void ) 
#else
insthetoks ( ) 
#endif
{
  insthetoks_regmem 
  mem [memtop - 12 ].hh .v.RH = thetoks () ;
  begintokenlist ( mem [memtop - 3 ].hh .v.RH , 4 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
convtoks ( void ) 
#else
convtoks ( ) 
#endif
{
  convtoks_regmem 
  char oldsetting  ;
  char c  ;
  smallnumber savescannerstatus  ;
  poolpointer b  ;
  c = curchr ;
  switch ( c ) 
  {case 0 : 
  case 1 : 
    scanint () ;
    break ;
  case 2 : 
  case 3 : 
    {
      savescannerstatus = scannerstatus ;
      scannerstatus = 0 ;
      gettoken () ;
      scannerstatus = savescannerstatus ;
    } 
    break ;
  case 4 : 
    scanfontident () ;
    break ;
  case 5 : 
    if ( jobname == 0 ) 
    openlogfile () ;
    break ;
  } 
  oldsetting = selector ;
  selector = 21 ;
  b = poolptr ;
  switch ( c ) 
  {case 0 : 
    printint ( curval ) ;
    break ;
  case 1 : 
    printromanint ( curval ) ;
    break ;
  case 2 : 
    if ( curcs != 0 ) 
    sprintcs ( curcs ) ;
    else printchar ( curchr ) ;
    break ;
  case 3 : 
    printmeaning () ;
    break ;
  case 4 : 
    {
      print ( fontname [curval ]) ;
      if ( fontsize [curval ]!= fontdsize [curval ]) 
      {
	print ( 740 ) ;
	printscaled ( fontsize [curval ]) ;
	print ( 394 ) ;
      } 
    } 
    break ;
  case 5 : 
    print ( jobname ) ;
    break ;
  } 
  selector = oldsetting ;
  mem [memtop - 12 ].hh .v.RH = strtoks ( b ) ;
  begintokenlist ( mem [memtop - 3 ].hh .v.RH , 4 ) ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zscantoks ( boolean macrodef , boolean xpand ) 
#else
zscantoks ( macrodef , xpand ) 
  boolean macrodef ;
  boolean xpand ;
#endif
{
  /* 40 30 31 32 */ register halfword Result; scantoks_regmem 
  halfword t  ;
  halfword s  ;
  halfword p  ;
  halfword q  ;
  halfword unbalance  ;
  halfword hashbrace  ;
  if ( macrodef ) 
  scannerstatus = 2 ;
  else scannerstatus = 5 ;
  warningindex = curcs ;
  defref = getavail () ;
  mem [defref ].hh .v.LH = 0 ;
  p = defref ;
  hashbrace = 0 ;
  t = 3120 ;
  if ( macrodef ) 
  {
    while ( true ) {
	
      gettoken () ;
      if ( curtok < 768 ) 
      goto lab31 ;
      if ( curcmd == 6 ) 
      {
	s = 3328 + curchr ;
	gettoken () ;
	if ( curcmd == 1 ) 
	{
	  hashbrace = curtok ;
	  {
	    q = getavail () ;
	    mem [p ].hh .v.RH = q ;
	    mem [q ].hh .v.LH = curtok ;
	    p = q ;
	  } 
	  {
	    q = getavail () ;
	    mem [p ].hh .v.RH = q ;
	    mem [q ].hh .v.LH = 3584 ;
	    p = q ;
	  } 
	  goto lab30 ;
	} 
	if ( t == 3129 ) 
	{
	  {
	    if ( interaction == 3 ) 
	    ;
	    printnl ( 262 ) ;
	    print ( 743 ) ;
	  } 
	  {
	    helpptr = 1 ;
	    helpline [0 ]= 744 ;
	  } 
	  error () ;
	} 
	else {
	    
	  incr ( t ) ;
	  if ( curtok != t ) 
	  {
	    {
	      if ( interaction == 3 ) 
	      ;
	      printnl ( 262 ) ;
	      print ( 745 ) ;
	    } 
	    {
	      helpptr = 2 ;
	      helpline [1 ]= 746 ;
	      helpline [0 ]= 747 ;
	    } 
	    backerror () ;
	  } 
	  curtok = s ;
	} 
      } 
      {
	q = getavail () ;
	mem [p ].hh .v.RH = q ;
	mem [q ].hh .v.LH = curtok ;
	p = q ;
      } 
    } 
    lab31: {
	
      q = getavail () ;
      mem [p ].hh .v.RH = q ;
      mem [q ].hh .v.LH = 3584 ;
      p = q ;
    } 
    if ( curcmd == 2 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 656 ) ;
      } 
      incr ( alignstate ) ;
      {
	helpptr = 2 ;
	helpline [1 ]= 741 ;
	helpline [0 ]= 742 ;
      } 
      error () ;
      goto lab40 ;
    } 
    lab30: ;
  } 
  else scanleftbrace () ;
  unbalance = 1 ;
  while ( true ) {
      
    if ( xpand ) 
    {
      while ( true ) {
	  
	getnext () ;
	if ( curcmd <= 100 ) 
	goto lab32 ;
	if ( curcmd != 109 ) 
	expand () ;
	else {
	    
	  q = thetoks () ;
	  if ( mem [memtop - 3 ].hh .v.RH != 0 ) 
	  {
	    mem [p ].hh .v.RH = mem [memtop - 3 ].hh .v.RH ;
	    p = q ;
	  } 
	} 
      } 
      lab32: xtoken () ;
    } 
    else gettoken () ;
    if ( curtok < 768 ) 
    if ( curcmd < 2 ) 
    incr ( unbalance ) ;
    else {
	
      decr ( unbalance ) ;
      if ( unbalance == 0 ) 
      goto lab40 ;
    } 
    else if ( curcmd == 6 ) 
    if ( macrodef ) 
    {
      s = curtok ;
      if ( xpand ) 
      getxtoken () ;
      else gettoken () ;
      if ( curcmd != 6 ) 
      if ( ( curtok <= 3120 ) || ( curtok > t ) ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 748 ) ;
	} 
	sprintcs ( warningindex ) ;
	{
	  helpptr = 3 ;
	  helpline [2 ]= 749 ;
	  helpline [1 ]= 750 ;
	  helpline [0 ]= 751 ;
	} 
	backerror () ;
	curtok = s ;
      } 
      else curtok = 1232 + curchr ;
    } 
    {
      q = getavail () ;
      mem [p ].hh .v.RH = q ;
      mem [q ].hh .v.LH = curtok ;
      p = q ;
    } 
  } 
  lab40: scannerstatus = 0 ;
  if ( hashbrace != 0 ) 
  {
    q = getavail () ;
    mem [p ].hh .v.RH = q ;
    mem [q ].hh .v.LH = hashbrace ;
    p = q ;
  } 
  Result = p ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zreadtoks ( integer n , halfword r ) 
#else
zreadtoks ( n , r ) 
  integer n ;
  halfword r ;
#endif
{
  /* 30 */ readtoks_regmem 
  halfword p  ;
  halfword q  ;
  integer s  ;
  smallnumber m  ;
  scannerstatus = 2 ;
  warningindex = r ;
  defref = getavail () ;
  mem [defref ].hh .v.LH = 0 ;
  p = defref ;
  {
    q = getavail () ;
    mem [p ].hh .v.RH = q ;
    mem [q ].hh .v.LH = 3584 ;
    p = q ;
  } 
  if ( ( n < 0 ) || ( n > 15 ) ) 
  m = 16 ;
  else m = n ;
  s = alignstate ;
  alignstate = 1000000L ;
  do {
      beginfilereading () ;
    curinput .namefield = m + 1 ;
    if ( readopen [m ]== 2 ) 
    if ( interaction > 1 ) 
    if ( n < 0 ) 
    {
      ;
      print ( 335 ) ;
      terminput () ;
    } 
    else {
	
      ;
      println () ;
      sprintcs ( r ) ;
      {
	;
	print ( 61 ) ;
	terminput () ;
      } 
      n = -1 ;
    } 
    else fatalerror ( 752 ) ;
    else if ( readopen [m ]== 1 ) 
    if ( inputln ( readfile [m ], false ) ) 
    readopen [m ]= 0 ;
    else {
	
      aclose ( readfile [m ]) ;
      readopen [m ]= 2 ;
    } 
    else {
	
      if ( ! inputln ( readfile [m ], true ) ) 
      {
	aclose ( readfile [m ]) ;
	readopen [m ]= 2 ;
	if ( alignstate != 1000000L ) 
	{
	  runaway () ;
	  {
	    if ( interaction == 3 ) 
	    ;
	    printnl ( 262 ) ;
	    print ( 753 ) ;
	  } 
	  printesc ( 534 ) ;
	  {
	    helpptr = 1 ;
	    helpline [0 ]= 754 ;
	  } 
	  alignstate = 1000000L ;
	  error () ;
	} 
      } 
    } 
    curinput .limitfield = last ;
    if ( ( eqtb [15211 ].cint < 0 ) || ( eqtb [15211 ].cint > 255 ) ) 
    decr ( curinput .limitfield ) ;
    else buffer [curinput .limitfield ]= eqtb [15211 ].cint ;
    first = curinput .limitfield + 1 ;
    curinput .locfield = curinput .startfield ;
    curinput .statefield = 33 ;
    while ( true ) {
	
      gettoken () ;
      if ( curtok == 0 ) 
      goto lab30 ;
      if ( alignstate < 1000000L ) 
      {
	do {
	    gettoken () ;
	} while ( ! ( curtok == 0 ) ) ;
	alignstate = 1000000L ;
	goto lab30 ;
      } 
      {
	q = getavail () ;
	mem [p ].hh .v.RH = q ;
	mem [q ].hh .v.LH = curtok ;
	p = q ;
      } 
    } 
    lab30: endfilereading () ;
  } while ( ! ( alignstate == 1000000L ) ) ;
  curval = defref ;
  scannerstatus = 0 ;
  alignstate = s ;
} 
void 
#ifdef HAVE_PROTOTYPES
passtext ( void ) 
#else
passtext ( ) 
#endif
{
  /* 30 */ passtext_regmem 
  integer l  ;
  smallnumber savescannerstatus  ;
  savescannerstatus = scannerstatus ;
  scannerstatus = 1 ;
  l = 0 ;
  skipline = line ;
  while ( true ) {
      
    getnext () ;
    if ( curcmd == 106 ) 
    {
      if ( l == 0 ) 
      goto lab30 ;
      if ( curchr == 2 ) 
      decr ( l ) ;
    } 
    else if ( curcmd == 105 ) 
    incr ( l ) ;
  } 
  lab30: scannerstatus = savescannerstatus ;
} 
void 
#ifdef HAVE_PROTOTYPES
zchangeiflimit ( smallnumber l , halfword p ) 
#else
zchangeiflimit ( l , p ) 
  smallnumber l ;
  halfword p ;
#endif
{
  /* 10 */ changeiflimit_regmem 
  halfword q  ;
  if ( p == condptr ) 
  iflimit = l ;
  else {
      
    q = condptr ;
    while ( true ) {
	
      if ( q == 0 ) 
      confusion ( 755 ) ;
      if ( mem [q ].hh .v.RH == p ) 
      {
	mem [q ].hh.b0 = l ;
	return ;
      } 
      q = mem [q ].hh .v.RH ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
conditional ( void ) 
#else
conditional ( ) 
#endif
{
  /* 10 50 */ conditional_regmem 
  boolean b  ;
  char r  ;
  integer m, n  ;
  halfword p, q  ;
  smallnumber savescannerstatus  ;
  halfword savecondptr  ;
  smallnumber thisif  ;
  {
    p = getnode ( 2 ) ;
    mem [p ].hh .v.RH = condptr ;
    mem [p ].hh.b0 = iflimit ;
    mem [p ].hh.b1 = curif ;
    mem [p + 1 ].cint = ifline ;
    condptr = p ;
    curif = curchr ;
    iflimit = 1 ;
    ifline = line ;
  } 
  savecondptr = condptr ;
  thisif = curchr ;
  switch ( thisif ) 
  {case 0 : 
  case 1 : 
    {
      {
	getxtoken () ;
	if ( curcmd == 0 ) 
	if ( curchr == 257 ) 
	{
	  curcmd = 13 ;
	  curchr = curtok - 4096 ;
	} 
      } 
      if ( ( curcmd > 13 ) || ( curchr > 255 ) ) 
      {
	m = 0 ;
	n = 256 ;
      } 
      else {
	  
	m = curcmd ;
	n = curchr ;
      } 
      {
	getxtoken () ;
	if ( curcmd == 0 ) 
	if ( curchr == 257 ) 
	{
	  curcmd = 13 ;
	  curchr = curtok - 4096 ;
	} 
      } 
      if ( ( curcmd > 13 ) || ( curchr > 255 ) ) 
      {
	curcmd = 0 ;
	curchr = 256 ;
      } 
      if ( thisif == 0 ) 
      b = ( n == curchr ) ;
      else b = ( m == curcmd ) ;
    } 
    break ;
  case 2 : 
  case 3 : 
    {
      if ( thisif == 2 ) 
      scanint () ;
      else scandimen ( false , false , false ) ;
      n = curval ;
      do {
	  getxtoken () ;
      } while ( ! ( curcmd != 10 ) ) ;
      if ( ( curtok >= 3132 ) && ( curtok <= 3134 ) ) 
      r = curtok - 3072 ;
      else {
	  
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 779 ) ;
	} 
	printcmdchr ( 105 , thisif ) ;
	{
	  helpptr = 1 ;
	  helpline [0 ]= 780 ;
	} 
	backerror () ;
	r = 61 ;
      } 
      if ( thisif == 2 ) 
      scanint () ;
      else scandimen ( false , false , false ) ;
      switch ( r ) 
      {case 60 : 
	b = ( n < curval ) ;
	break ;
      case 61 : 
	b = ( n == curval ) ;
	break ;
      case 62 : 
	b = ( n > curval ) ;
	break ;
      } 
    } 
    break ;
  case 4 : 
    {
      scanint () ;
      b = odd ( curval ) ;
    } 
    break ;
  case 5 : 
    b = ( abs ( curlist .modefield ) == 1 ) ;
    break ;
  case 6 : 
    b = ( abs ( curlist .modefield ) == 102 ) ;
    break ;
  case 7 : 
    b = ( abs ( curlist .modefield ) == 203 ) ;
    break ;
  case 8 : 
    b = ( curlist .modefield < 0 ) ;
    break ;
  case 9 : 
  case 10 : 
  case 11 : 
    {
      scaneightbitint () ;
      p = eqtb [13322 + curval ].hh .v.RH ;
      if ( thisif == 9 ) 
      b = ( p == 0 ) ;
      else if ( p == 0 ) 
      b = false ;
      else if ( thisif == 10 ) 
      b = ( mem [p ].hh.b0 == 0 ) ;
      else b = ( mem [p ].hh.b0 == 1 ) ;
    } 
    break ;
  case 12 : 
    {
      savescannerstatus = scannerstatus ;
      scannerstatus = 0 ;
      getnext () ;
      n = curcs ;
      p = curcmd ;
      q = curchr ;
      getnext () ;
      if ( curcmd != p ) 
      b = false ;
      else if ( curcmd < 111 ) 
      b = ( curchr == q ) ;
      else {
	  
	p = mem [curchr ].hh .v.RH ;
	q = mem [eqtb [n ].hh .v.RH ].hh .v.RH ;
	if ( p == q ) 
	b = true ;
	else {
	    
	  while ( ( p != 0 ) && ( q != 0 ) ) if ( mem [p ].hh .v.LH != mem [
	  q ].hh .v.LH ) 
	  p = 0 ;
	  else {
	      
	    p = mem [p ].hh .v.RH ;
	    q = mem [q ].hh .v.RH ;
	  } 
	  b = ( ( p == 0 ) && ( q == 0 ) ) ;
	} 
      } 
      scannerstatus = savescannerstatus ;
    } 
    break ;
  case 13 : 
    {
      scanfourbitint () ;
      b = ( readopen [curval ]== 2 ) ;
    } 
    break ;
  case 14 : 
    b = true ;
    break ;
  case 15 : 
    b = false ;
    break ;
  case 16 : 
    {
      scanint () ;
      n = curval ;
      if ( eqtb [15199 ].cint > 1 ) 
      {
	begindiagnostic () ;
	print ( 781 ) ;
	printint ( n ) ;
	printchar ( 125 ) ;
	enddiagnostic ( false ) ;
      } 
      while ( n != 0 ) {
	  
	passtext () ;
	if ( condptr == savecondptr ) 
	if ( curchr == 4 ) 
	decr ( n ) ;
	else goto lab50 ;
	else if ( curchr == 2 ) 
	{
	  p = condptr ;
	  ifline = mem [p + 1 ].cint ;
	  curif = mem [p ].hh.b1 ;
	  iflimit = mem [p ].hh.b0 ;
	  condptr = mem [p ].hh .v.RH ;
	  freenode ( p , 2 ) ;
	} 
      } 
      changeiflimit ( 4 , savecondptr ) ;
      return ;
    } 
    break ;
  } 
  if ( eqtb [15199 ].cint > 1 ) 
  {
    begindiagnostic () ;
    if ( b ) 
    print ( 777 ) ;
    else print ( 778 ) ;
    enddiagnostic ( false ) ;
  } 
  if ( b ) 
  {
    changeiflimit ( 3 , savecondptr ) ;
    return ;
  } 
  while ( true ) {
      
    passtext () ;
    if ( condptr == savecondptr ) 
    {
      if ( curchr != 4 ) 
      goto lab50 ;
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 775 ) ;
      } 
      printesc ( 773 ) ;
      {
	helpptr = 1 ;
	helpline [0 ]= 776 ;
      } 
      error () ;
    } 
    else if ( curchr == 2 ) 
    {
      p = condptr ;
      ifline = mem [p + 1 ].cint ;
      curif = mem [p ].hh.b1 ;
      iflimit = mem [p ].hh.b0 ;
      condptr = mem [p ].hh .v.RH ;
      freenode ( p , 2 ) ;
    } 
  } 
  lab50: if ( curchr == 2 ) 
  {
    p = condptr ;
    ifline = mem [p + 1 ].cint ;
    curif = mem [p ].hh.b1 ;
    iflimit = mem [p ].hh.b0 ;
    condptr = mem [p ].hh .v.RH ;
    freenode ( p , 2 ) ;
  } 
  else iflimit = 2 ;
} 
void 
#ifdef HAVE_PROTOTYPES
beginname ( void ) 
#else
beginname ( ) 
#endif
{
  beginname_regmem 
  areadelimiter = 0 ;
  extdelimiter = 0 ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
zmorename ( ASCIIcode c ) 
#else
zmorename ( c ) 
  ASCIIcode c ;
#endif
{
  register boolean Result; morename_regmem 
  if ( c == 32 ) 
  Result = false ;
  else {
      
    {
      if ( poolptr + 1 > poolsize ) 
      overflow ( 257 , poolsize - initpoolptr ) ;
    } 
    {
      strpool [poolptr ]= c ;
      incr ( poolptr ) ;
    } 
    if ( ISDIRSEP ( c ) ) 
    {
      areadelimiter = ( poolptr - strstart [strptr ]) ;
      extdelimiter = 0 ;
    } 
    else if ( c == 46 ) 
    extdelimiter = ( poolptr - strstart [strptr ]) ;
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
  endname_regmem 
  strnumber tempstr  ;
  poolpointer j  ;
  if ( strptr + 3 > maxstrings ) 
  overflow ( 258 , maxstrings - initstrptr ) ;
  if ( areadelimiter == 0 ) 
  curarea = 335 ;
  else {
      
    curarea = strptr ;
    strstart [strptr + 1 ]= strstart [strptr ]+ areadelimiter ;
    incr ( strptr ) ;
    tempstr = searchstring ( curarea ) ;
    if ( tempstr > 0 ) 
    {
      curarea = tempstr ;
      decr ( strptr ) ;
      {register integer for_end; j = strstart [strptr + 1 ];for_end = 
      poolptr - 1 ; if ( j <= for_end) do 
	{
	  strpool [j - areadelimiter ]= strpool [j ];
	} 
      while ( j++ < for_end ) ;} 
      poolptr = poolptr - areadelimiter ;
    } 
  } 
  if ( extdelimiter == 0 ) 
  {
    curext = 335 ;
    curname = slowmakestring () ;
  } 
  else {
      
    curname = strptr ;
    strstart [strptr + 1 ]= strstart [strptr ]+ extdelimiter - 
    areadelimiter - 1 ;
    incr ( strptr ) ;
    curext = makestring () ;
    decr ( strptr ) ;
    tempstr = searchstring ( curname ) ;
    if ( tempstr > 0 ) 
    {
      curname = tempstr ;
      decr ( strptr ) ;
      {register integer for_end; j = strstart [strptr + 1 ];for_end = 
      poolptr - 1 ; if ( j <= for_end) do 
	{
	  strpool [j - extdelimiter + areadelimiter + 1 ]= strpool [j ];
	} 
      while ( j++ < for_end ) ;} 
      poolptr = poolptr - extdelimiter + areadelimiter + 1 ;
    } 
    curext = slowmakestring () ;
  } 
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
  packfilename_regmem 
  integer k  ;
  ASCIIcode c  ;
  poolpointer j  ;
  k = 0 ;
  if ( (char*) nameoffile ) 
  libcfree ( (char*) nameoffile ) ;
  nameoffile = xmalloc ( 1 + ( strstart [a + 1 ]- strstart [a ]) + ( 
  strstart [n + 1 ]- strstart [n ]) + ( strstart [e + 1 ]- strstart [e 
  ]) + 1 ) ;
  {register integer for_end; j = strstart [a ];for_end = strstart [a + 1 
  ]- 1 ; if ( j <= for_end) do 
    {
      c = strpool [j ];
      incr ( k ) ;
      if ( k <= maxint ) 
      nameoffile [k ]= xchr [c ];
    } 
  while ( j++ < for_end ) ;} 
  {register integer for_end; j = strstart [n ];for_end = strstart [n + 1 
  ]- 1 ; if ( j <= for_end) do 
    {
      c = strpool [j ];
      incr ( k ) ;
      if ( k <= maxint ) 
      nameoffile [k ]= xchr [c ];
    } 
  while ( j++ < for_end ) ;} 
  {register integer for_end; j = strstart [e ];for_end = strstart [e + 1 
  ]- 1 ; if ( j <= for_end) do 
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
zpackbufferedname ( smallnumber n , integer a , integer b ) 
#else
zpackbufferedname ( n , a , b ) 
  smallnumber n ;
  integer a ;
  integer b ;
#endif
{
  packbufferedname_regmem 
  integer k  ;
  ASCIIcode c  ;
  integer j  ;
  if ( n + b - a + 5 > maxint ) 
  b = a + maxint - n - 5 ;
  k = 0 ;
  if ( (char*) nameoffile ) 
  libcfree ( (char*) nameoffile ) ;
  nameoffile = xmalloc ( 1 + n + ( b - a + 1 ) + 5 ) ;
  {register integer for_end; j = 1 ;for_end = n ; if ( j <= for_end) do 
    {
      c = xord [TEXformatdefault [j ]];
      incr ( k ) ;
      if ( k <= maxint ) 
      nameoffile [k ]= xchr [c ];
    } 
  while ( j++ < for_end ) ;} 
  {register integer for_end; j = a ;for_end = b ; if ( j <= for_end) do 
    {
      c = buffer [j ];
      incr ( k ) ;
      if ( k <= maxint ) 
      nameoffile [k ]= xchr [c ];
    } 
  while ( j++ < for_end ) ;} 
  {register integer for_end; j = formatdefaultlength - 3 ;for_end = 
  formatdefaultlength ; if ( j <= for_end) do 
    {
      c = xord [TEXformatdefault [j ]];
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
strnumber 
#ifdef HAVE_PROTOTYPES
makenamestring ( void ) 
#else
makenamestring ( ) 
#endif
{
  register strnumber Result; makenamestring_regmem 
  integer k  ;
  if ( ( poolptr + namelength > poolsize ) || ( strptr == maxstrings ) || ( ( 
  poolptr - strstart [strptr ]) > 0 ) ) 
  Result = 63 ;
  else {
      
    {register integer for_end; k = 1 ;for_end = namelength ; if ( k <= 
    for_end) do 
      {
	strpool [poolptr ]= xord [nameoffile [k ]];
	incr ( poolptr ) ;
      } 
    while ( k++ < for_end ) ;} 
    Result = makestring () ;
  } 
  return Result ;
} 
strnumber 
#ifdef HAVE_PROTOTYPES
zamakenamestring ( alphafile f ) 
#else
zamakenamestring ( f ) 
  alphafile f ;
#endif
{
  register strnumber Result; amakenamestring_regmem 
  Result = makenamestring () ;
  return Result ;
} 
strnumber 
#ifdef HAVE_PROTOTYPES
zbmakenamestring ( bytefile f ) 
#else
zbmakenamestring ( f ) 
  bytefile f ;
#endif
{
  register strnumber Result; bmakenamestring_regmem 
  Result = makenamestring () ;
  return Result ;
} 
strnumber 
#ifdef HAVE_PROTOTYPES
zwmakenamestring ( wordfile f ) 
#else
zwmakenamestring ( f ) 
  wordfile f ;
#endif
{
  register strnumber Result; wmakenamestring_regmem 
  Result = makenamestring () ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
scanfilename ( void ) 
#else
scanfilename ( ) 
#endif
{
  /* 30 */ scanfilename_regmem 
  nameinprogress = true ;
  beginname () ;
  do {
      getxtoken () ;
  } while ( ! ( curcmd != 10 ) ) ;
  while ( true ) {
      
    if ( ( curcmd > 12 ) || ( curchr > 255 ) ) 
    {
      backinput () ;
      goto lab30 ;
    } 
    if ( ! morename ( curchr ) ) 
    goto lab30 ;
    getxtoken () ;
  } 
  lab30: endname () ;
  nameinprogress = false ;
} 
void 
#ifdef HAVE_PROTOTYPES
zpackjobname ( strnumber s ) 
#else
zpackjobname ( s ) 
  strnumber s ;
#endif
{
  packjobname_regmem 
  curarea = 335 ;
  curext = s ;
  curname = jobname ;
  packfilename ( curname , curarea , curext ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zpromptfilename ( strnumber s , strnumber e ) 
#else
zpromptfilename ( s , e ) 
  strnumber s ;
  strnumber e ;
#endif
{
  /* 30 */ promptfilename_regmem 
  integer k  ;
  if ( interaction == 2 ) 
  ;
  if ( s == 783 ) 
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 262 ) ;
    print ( 784 ) ;
  } 
  else {
      
    if ( interaction == 3 ) 
    ;
    printnl ( 262 ) ;
    print ( 785 ) ;
  } 
  printfilename ( curname , curarea , curext ) ;
  print ( 786 ) ;
  if ( ( e == 787 ) || ( e == 335 ) ) 
  showcontext () ;
  printnl ( 788 ) ;
  print ( s ) ;
  if ( interaction < 2 ) 
  fatalerror ( 789 ) ;
  {
    ;
    print ( 568 ) ;
    terminput () ;
  } 
  {
    beginname () ;
    k = first ;
    while ( ( buffer [k ]== 32 ) && ( k < last ) ) incr ( k ) ;
    while ( true ) {
	
      if ( k == last ) 
      goto lab30 ;
      if ( ! morename ( buffer [k ]) ) 
      goto lab30 ;
      incr ( k ) ;
    } 
    lab30: endname () ;
  } 
  if ( curext == 335 ) 
  curext = e ;
  packfilename ( curname , curarea , curext ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
openlogfile ( void ) 
#else
openlogfile ( ) 
#endif
{
  openlogfile_regmem 
  char oldsetting  ;
  integer k  ;
  integer l  ;
  char * months  ;
  oldsetting = selector ;
  if ( jobname == 0 ) 
  jobname = 792 ;
  packjobname ( 793 ) ;
  while ( ! aopenout ( logfile ) ) {
      
    selector = 17 ;
    promptfilename ( 795 , 793 ) ;
  } 
  texmflogname = amakenamestring ( logfile ) ;
  selector = 18 ;
  logopened = true ;
  {
    Fputs( logfile ,  "This is TeX, Version 3.14159" ) ;
    Fputs( logfile ,  versionstring ) ;
    print ( formatident ) ;
    print ( 796 ) ;
    printint ( eqtb [15184 ].cint ) ;
    printchar ( 32 ) ;
    months = " JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC" ;
    {register integer for_end; k = 3 * eqtb [15185 ].cint - 2 ;for_end = 3 
    * eqtb [15185 ].cint ; if ( k <= for_end) do 
      putc ( months [k ],  logfile );
    while ( k++ < for_end ) ;} 
    printchar ( 32 ) ;
    printint ( eqtb [15186 ].cint ) ;
    printchar ( 32 ) ;
    printtwo ( eqtb [15183 ].cint / 60 ) ;
    printchar ( 58 ) ;
    printtwo ( eqtb [15183 ].cint % 60 ) ;
  } 
  if ( mltexenabledp ) 
  {
    putc ('\n',  logfile );
    Fputs( logfile ,  "MLTeX v2.2 enabled" ) ;
  } 
  inputstack [inputptr ]= curinput ;
  printnl ( 794 ) ;
  l = inputstack [0 ].limitfield ;
  if ( buffer [l ]== eqtb [15211 ].cint ) 
  decr ( l ) ;
  {register integer for_end; k = 1 ;for_end = l ; if ( k <= for_end) do 
    print ( buffer [k ]) ;
  while ( k++ < for_end ) ;} 
  println () ;
  selector = oldsetting + 2 ;
} 
integer 
#ifdef HAVE_PROTOTYPES
zeffectivechar ( boolean errp , internalfontnumber f , quarterword c ) 
#else
zeffectivechar ( errp , f , c ) 
  boolean errp ;
  internalfontnumber f ;
  quarterword c ;
#endif
{
  /* 40 */ register integer Result; effectivechar_regmem 
  integer basec  ;
  integer result  ;
  result = c ;
  if ( ! mltexenabledp ) 
  goto lab40 ;
  if ( fontec [f ]>= c ) 
  if ( fontbc [f ]<= c ) 
  if ( ( fontinfo [charbase [f ]+ c ].qqqq .b0 > 0 ) ) 
  goto lab40 ;
  if ( c >= eqtb [15218 ].cint ) 
  if ( c <= eqtb [15219 ].cint ) 
  if ( ( eqtb [14907 + c ].hh .v.RH > 0 ) ) 
  {
    basec = ( eqtb [14907 + c ].hh .v.RH % 256 ) ;
    result = basec ;
    if ( ! errp ) 
    goto lab40 ;
    if ( fontec [f ]>= basec ) 
    if ( fontbc [f ]<= basec ) 
    if ( ( fontinfo [charbase [f ]+ basec ].qqqq .b0 > 0 ) ) 
    goto lab40 ;
  } 
  if ( errp ) 
  {
    begindiagnostic () ;
    printnl ( 820 ) ;
    print ( 1306 ) ;
    print ( c ) ;
    print ( 821 ) ;
    print ( fontname [f ]) ;
    printchar ( 33 ) ;
    enddiagnostic ( false ) ;
    result = fontbc [f ];
  } 
  lab40: Result = result ;
  return Result ;
} 
fourquarters 
#ifdef HAVE_PROTOTYPES
zeffectivecharinfo ( internalfontnumber f , quarterword c ) 
#else
zeffectivecharinfo ( f , c ) 
  internalfontnumber f ;
  quarterword c ;
#endif
{
  /* 10 */ register fourquarters Result; effectivecharinfo_regmem 
  fourquarters ci  ;
  integer basec  ;
  if ( ! mltexenabledp ) 
  {
    Result = fontinfo [charbase [f ]+ c ].qqqq ;
    return Result ;
  } 
  if ( fontec [f ]>= c ) 
  if ( fontbc [f ]<= c ) 
  {
    ci = fontinfo [charbase [f ]+ c ].qqqq ;
    if ( ( ci .b0 > 0 ) ) 
    {
      Result = ci ;
      return Result ;
    } 
  } 
  if ( c >= eqtb [15218 ].cint ) 
  if ( c <= eqtb [15219 ].cint ) 
  if ( ( eqtb [14907 + c ].hh .v.RH > 0 ) ) 
  {
    basec = ( eqtb [14907 + c ].hh .v.RH % 256 ) ;
    if ( fontec [f ]>= basec ) 
    if ( fontbc [f ]<= basec ) 
    {
      ci = fontinfo [charbase [f ]+ basec ].qqqq ;
      if ( ( ci .b0 > 0 ) ) 
      {
	Result = ci ;
	return Result ;
      } 
    } 
  } 
  Result = nullcharacter ;
  return Result ;
} 
internalfontnumber 
#ifdef HAVE_PROTOTYPES
zreadfontinfo ( halfword u , strnumber nom , strnumber aire , scaled s ) 
#else
zreadfontinfo ( u , nom , aire , s ) 
  halfword u ;
  strnumber nom ;
  strnumber aire ;
  scaled s ;
#endif
{
  /* 30 11 45 */ register internalfontnumber Result; readfontinfo_regmem 
  fontindex k  ;
  boolean fileopened  ;
  halfword lf, lh, bc, ec, nw, nh, nd, ni, nl, nk, ne, np  ;
  internalfontnumber f  ;
  internalfontnumber g  ;
  eightbits a, b, c, d  ;
  fourquarters qw  ;
  scaled sw  ;
  integer bchlabel  ;
  short bchar  ;
  scaled z  ;
  integer alpha  ;
  char beta  ;
  g = 0 ;
  fileopened = false ;
  packfilename ( nom , aire , 335 ) ;
  if ( ! bopenin ( tfmfile ) ) 
  goto lab11 ;
  fileopened = true ;
  {
    {
      lf = tfmtemp ;
      if ( lf > 127 ) 
      goto lab11 ;
      tfmtemp = getc ( tfmfile ) ;
      lf = lf * 256 + tfmtemp ;
    } 
    tfmtemp = getc ( tfmfile ) ;
    {
      lh = tfmtemp ;
      if ( lh > 127 ) 
      goto lab11 ;
      tfmtemp = getc ( tfmfile ) ;
      lh = lh * 256 + tfmtemp ;
    } 
    tfmtemp = getc ( tfmfile ) ;
    {
      bc = tfmtemp ;
      if ( bc > 127 ) 
      goto lab11 ;
      tfmtemp = getc ( tfmfile ) ;
      bc = bc * 256 + tfmtemp ;
    } 
    tfmtemp = getc ( tfmfile ) ;
    {
      ec = tfmtemp ;
      if ( ec > 127 ) 
      goto lab11 ;
      tfmtemp = getc ( tfmfile ) ;
      ec = ec * 256 + tfmtemp ;
    } 
    if ( ( bc > ec + 1 ) || ( ec > 255 ) ) 
    goto lab11 ;
    if ( bc > 255 ) 
    {
      bc = 1 ;
      ec = 0 ;
    } 
    tfmtemp = getc ( tfmfile ) ;
    {
      nw = tfmtemp ;
      if ( nw > 127 ) 
      goto lab11 ;
      tfmtemp = getc ( tfmfile ) ;
      nw = nw * 256 + tfmtemp ;
    } 
    tfmtemp = getc ( tfmfile ) ;
    {
      nh = tfmtemp ;
      if ( nh > 127 ) 
      goto lab11 ;
      tfmtemp = getc ( tfmfile ) ;
      nh = nh * 256 + tfmtemp ;
    } 
    tfmtemp = getc ( tfmfile ) ;
    {
      nd = tfmtemp ;
      if ( nd > 127 ) 
      goto lab11 ;
      tfmtemp = getc ( tfmfile ) ;
      nd = nd * 256 + tfmtemp ;
    } 
    tfmtemp = getc ( tfmfile ) ;
    {
      ni = tfmtemp ;
      if ( ni > 127 ) 
      goto lab11 ;
      tfmtemp = getc ( tfmfile ) ;
      ni = ni * 256 + tfmtemp ;
    } 
    tfmtemp = getc ( tfmfile ) ;
    {
      nl = tfmtemp ;
      if ( nl > 127 ) 
      goto lab11 ;
      tfmtemp = getc ( tfmfile ) ;
      nl = nl * 256 + tfmtemp ;
    } 
    tfmtemp = getc ( tfmfile ) ;
    {
      nk = tfmtemp ;
      if ( nk > 127 ) 
      goto lab11 ;
      tfmtemp = getc ( tfmfile ) ;
      nk = nk * 256 + tfmtemp ;
    } 
    tfmtemp = getc ( tfmfile ) ;
    {
      ne = tfmtemp ;
      if ( ne > 127 ) 
      goto lab11 ;
      tfmtemp = getc ( tfmfile ) ;
      ne = ne * 256 + tfmtemp ;
    } 
    tfmtemp = getc ( tfmfile ) ;
    {
      np = tfmtemp ;
      if ( np > 127 ) 
      goto lab11 ;
      tfmtemp = getc ( tfmfile ) ;
      np = np * 256 + tfmtemp ;
    } 
    if ( lf != 6 + lh + ( ec - bc + 1 ) + nw + nh + nd + ni + nl + nk + ne + 
    np ) 
    goto lab11 ;
  } 
  lf = lf - 6 - lh ;
  if ( np < 7 ) 
  lf = lf + 7 - np ;
  if ( ( fontptr == fontmax ) || ( fmemptr + lf > fontmemsize ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 798 ) ;
    } 
    sprintcs ( u ) ;
    printchar ( 61 ) ;
    printfilename ( nom , aire , 335 ) ;
    if ( s >= 0 ) 
    {
      print ( 740 ) ;
      printscaled ( s ) ;
      print ( 394 ) ;
    } 
    else if ( s != -1000 ) 
    {
      print ( 799 ) ;
      printint ( - (integer) s ) ;
    } 
    print ( 807 ) ;
    {
      helpptr = 4 ;
      helpline [3 ]= 808 ;
      helpline [2 ]= 809 ;
      helpline [1 ]= 810 ;
      helpline [0 ]= 811 ;
    } 
    error () ;
    goto lab30 ;
  } 
  f = fontptr + 1 ;
  charbase [f ]= fmemptr - bc ;
  widthbase [f ]= charbase [f ]+ ec + 1 ;
  heightbase [f ]= widthbase [f ]+ nw ;
  depthbase [f ]= heightbase [f ]+ nh ;
  italicbase [f ]= depthbase [f ]+ nd ;
  ligkernbase [f ]= italicbase [f ]+ ni ;
  kernbase [f ]= ligkernbase [f ]+ nl - 256 * ( 128 ) ;
  extenbase [f ]= kernbase [f ]+ 256 * ( 128 ) + nk ;
  parambase [f ]= extenbase [f ]+ ne ;
  {
    if ( lh < 2 ) 
    goto lab11 ;
    {
      tfmtemp = getc ( tfmfile ) ;
      a = tfmtemp ;
      qw .b0 = a ;
      tfmtemp = getc ( tfmfile ) ;
      b = tfmtemp ;
      qw .b1 = b ;
      tfmtemp = getc ( tfmfile ) ;
      c = tfmtemp ;
      qw .b2 = c ;
      tfmtemp = getc ( tfmfile ) ;
      d = tfmtemp ;
      qw .b3 = d ;
      fontcheck [f ]= qw ;
    } 
    tfmtemp = getc ( tfmfile ) ;
    {
      z = tfmtemp ;
      if ( z > 127 ) 
      goto lab11 ;
      tfmtemp = getc ( tfmfile ) ;
      z = z * 256 + tfmtemp ;
    } 
    tfmtemp = getc ( tfmfile ) ;
    z = z * 256 + tfmtemp ;
    tfmtemp = getc ( tfmfile ) ;
    z = ( z * 16 ) + ( tfmtemp / 16 ) ;
    if ( z < 65536L ) 
    goto lab11 ;
    while ( lh > 2 ) {
	
      tfmtemp = getc ( tfmfile ) ;
      tfmtemp = getc ( tfmfile ) ;
      tfmtemp = getc ( tfmfile ) ;
      tfmtemp = getc ( tfmfile ) ;
      decr ( lh ) ;
    } 
    fontdsize [f ]= z ;
    if ( s != -1000 ) 
    if ( s >= 0 ) 
    z = s ;
    else z = xnoverd ( z , - (integer) s , 1000 ) ;
    fontsize [f ]= z ;
  } 
  {register integer for_end; k = fmemptr ;for_end = widthbase [f ]- 1 
  ; if ( k <= for_end) do 
    {
      {
	tfmtemp = getc ( tfmfile ) ;
	a = tfmtemp ;
	qw .b0 = a ;
	tfmtemp = getc ( tfmfile ) ;
	b = tfmtemp ;
	qw .b1 = b ;
	tfmtemp = getc ( tfmfile ) ;
	c = tfmtemp ;
	qw .b2 = c ;
	tfmtemp = getc ( tfmfile ) ;
	d = tfmtemp ;
	qw .b3 = d ;
	fontinfo [k ].qqqq = qw ;
      } 
      if ( ( a >= nw ) || ( b / 16 >= nh ) || ( b % 16 >= nd ) || ( c / 4 >= 
      ni ) ) 
      goto lab11 ;
      switch ( c % 4 ) 
      {case 1 : 
	if ( d >= nl ) 
	goto lab11 ;
	break ;
      case 3 : 
	if ( d >= ne ) 
	goto lab11 ;
	break ;
      case 2 : 
	{
	  {
	    if ( ( d < bc ) || ( d > ec ) ) 
	    goto lab11 ;
	  } 
	  while ( d < k + bc - fmemptr ) {
	      
	    qw = fontinfo [charbase [f ]+ d ].qqqq ;
	    if ( ( ( qw .b2 ) % 4 ) != 2 ) 
	    goto lab45 ;
	    d = qw .b3 ;
	  } 
	  if ( d == k + bc - fmemptr ) 
	  goto lab11 ;
	  lab45: ;
	} 
	break ;
	default: 
	;
	break ;
      } 
    } 
  while ( k++ < for_end ) ;} 
  {
    {
      alpha = 16 ;
      while ( z >= 8388608L ) {
	  
	z = z / 2 ;
	alpha = alpha + alpha ;
      } 
      beta = 256 / alpha ;
      alpha = alpha * z ;
    } 
    {register integer for_end; k = widthbase [f ];for_end = ligkernbase [
    f ]- 1 ; if ( k <= for_end) do 
      {
	tfmtemp = getc ( tfmfile ) ;
	a = tfmtemp ;
	tfmtemp = getc ( tfmfile ) ;
	b = tfmtemp ;
	tfmtemp = getc ( tfmfile ) ;
	c = tfmtemp ;
	tfmtemp = getc ( tfmfile ) ;
	d = tfmtemp ;
	sw = ( ( ( ( ( d * z ) / 256 ) + ( c * z ) ) / 256 ) + ( b * z ) ) / 
	beta ;
	if ( a == 0 ) 
	fontinfo [k ].cint = sw ;
	else if ( a == 255 ) 
	fontinfo [k ].cint = sw - alpha ;
	else goto lab11 ;
      } 
    while ( k++ < for_end ) ;} 
    if ( fontinfo [widthbase [f ]].cint != 0 ) 
    goto lab11 ;
    if ( fontinfo [heightbase [f ]].cint != 0 ) 
    goto lab11 ;
    if ( fontinfo [depthbase [f ]].cint != 0 ) 
    goto lab11 ;
    if ( fontinfo [italicbase [f ]].cint != 0 ) 
    goto lab11 ;
  } 
  bchlabel = 32767 ;
  bchar = 256 ;
  if ( nl > 0 ) 
  {
    {register integer for_end; k = ligkernbase [f ];for_end = kernbase [f 
    ]+ 256 * ( 128 ) - 1 ; if ( k <= for_end) do 
      {
	{
	  tfmtemp = getc ( tfmfile ) ;
	  a = tfmtemp ;
	  qw .b0 = a ;
	  tfmtemp = getc ( tfmfile ) ;
	  b = tfmtemp ;
	  qw .b1 = b ;
	  tfmtemp = getc ( tfmfile ) ;
	  c = tfmtemp ;
	  qw .b2 = c ;
	  tfmtemp = getc ( tfmfile ) ;
	  d = tfmtemp ;
	  qw .b3 = d ;
	  fontinfo [k ].qqqq = qw ;
	} 
	if ( a > 128 ) 
	{
	  if ( 256 * c + d >= nl ) 
	  goto lab11 ;
	  if ( a == 255 ) 
	  if ( k == ligkernbase [f ]) 
	  bchar = b ;
	} 
	else {
	    
	  if ( b != bchar ) 
	  {
	    {
	      if ( ( b < bc ) || ( b > ec ) ) 
	      goto lab11 ;
	    } 
	    qw = fontinfo [charbase [f ]+ b ].qqqq ;
	    if ( ! ( qw .b0 > 0 ) ) 
	    goto lab11 ;
	  } 
	  if ( c < 128 ) 
	  {
	    {
	      if ( ( d < bc ) || ( d > ec ) ) 
	      goto lab11 ;
	    } 
	    qw = fontinfo [charbase [f ]+ d ].qqqq ;
	    if ( ! ( qw .b0 > 0 ) ) 
	    goto lab11 ;
	  } 
	  else if ( 256 * ( c - 128 ) + d >= nk ) 
	  goto lab11 ;
	  if ( a < 128 ) 
	  if ( k - ligkernbase [f ]+ a + 1 >= nl ) 
	  goto lab11 ;
	} 
      } 
    while ( k++ < for_end ) ;} 
    if ( a == 255 ) 
    bchlabel = 256 * c + d ;
  } 
  {register integer for_end; k = kernbase [f ]+ 256 * ( 128 ) ;for_end = 
  extenbase [f ]- 1 ; if ( k <= for_end) do 
    {
      tfmtemp = getc ( tfmfile ) ;
      a = tfmtemp ;
      tfmtemp = getc ( tfmfile ) ;
      b = tfmtemp ;
      tfmtemp = getc ( tfmfile ) ;
      c = tfmtemp ;
      tfmtemp = getc ( tfmfile ) ;
      d = tfmtemp ;
      sw = ( ( ( ( ( d * z ) / 256 ) + ( c * z ) ) / 256 ) + ( b * z ) ) / 
      beta ;
      if ( a == 0 ) 
      fontinfo [k ].cint = sw ;
      else if ( a == 255 ) 
      fontinfo [k ].cint = sw - alpha ;
      else goto lab11 ;
    } 
  while ( k++ < for_end ) ;} 
  {register integer for_end; k = extenbase [f ];for_end = parambase [f ]
  - 1 ; if ( k <= for_end) do 
    {
      {
	tfmtemp = getc ( tfmfile ) ;
	a = tfmtemp ;
	qw .b0 = a ;
	tfmtemp = getc ( tfmfile ) ;
	b = tfmtemp ;
	qw .b1 = b ;
	tfmtemp = getc ( tfmfile ) ;
	c = tfmtemp ;
	qw .b2 = c ;
	tfmtemp = getc ( tfmfile ) ;
	d = tfmtemp ;
	qw .b3 = d ;
	fontinfo [k ].qqqq = qw ;
      } 
      if ( a != 0 ) 
      {
	{
	  if ( ( a < bc ) || ( a > ec ) ) 
	  goto lab11 ;
	} 
	qw = fontinfo [charbase [f ]+ a ].qqqq ;
	if ( ! ( qw .b0 > 0 ) ) 
	goto lab11 ;
      } 
      if ( b != 0 ) 
      {
	{
	  if ( ( b < bc ) || ( b > ec ) ) 
	  goto lab11 ;
	} 
	qw = fontinfo [charbase [f ]+ b ].qqqq ;
	if ( ! ( qw .b0 > 0 ) ) 
	goto lab11 ;
      } 
      if ( c != 0 ) 
      {
	{
	  if ( ( c < bc ) || ( c > ec ) ) 
	  goto lab11 ;
	} 
	qw = fontinfo [charbase [f ]+ c ].qqqq ;
	if ( ! ( qw .b0 > 0 ) ) 
	goto lab11 ;
      } 
      {
	{
	  if ( ( d < bc ) || ( d > ec ) ) 
	  goto lab11 ;
	} 
	qw = fontinfo [charbase [f ]+ d ].qqqq ;
	if ( ! ( qw .b0 > 0 ) ) 
	goto lab11 ;
      } 
    } 
  while ( k++ < for_end ) ;} 
  {
    {register integer for_end; k = 1 ;for_end = np ; if ( k <= for_end) do 
      if ( k == 1 ) 
      {
	tfmtemp = getc ( tfmfile ) ;
	sw = tfmtemp ;
	if ( sw > 127 ) 
	sw = sw - 256 ;
	tfmtemp = getc ( tfmfile ) ;
	sw = sw * 256 + tfmtemp ;
	tfmtemp = getc ( tfmfile ) ;
	sw = sw * 256 + tfmtemp ;
	tfmtemp = getc ( tfmfile ) ;
	fontinfo [parambase [f ]].cint = ( sw * 16 ) + ( tfmtemp / 16 ) ;
      } 
      else {
	  
	tfmtemp = getc ( tfmfile ) ;
	a = tfmtemp ;
	tfmtemp = getc ( tfmfile ) ;
	b = tfmtemp ;
	tfmtemp = getc ( tfmfile ) ;
	c = tfmtemp ;
	tfmtemp = getc ( tfmfile ) ;
	d = tfmtemp ;
	sw = ( ( ( ( ( d * z ) / 256 ) + ( c * z ) ) / 256 ) + ( b * z ) ) / 
	beta ;
	if ( a == 0 ) 
	fontinfo [parambase [f ]+ k - 1 ].cint = sw ;
	else if ( a == 255 ) 
	fontinfo [parambase [f ]+ k - 1 ].cint = sw - alpha ;
	else goto lab11 ;
      } 
    while ( k++ < for_end ) ;} 
    if ( feof ( tfmfile ) ) 
    goto lab11 ;
    {register integer for_end; k = np + 1 ;for_end = 7 ; if ( k <= for_end) 
    do 
      fontinfo [parambase [f ]+ k - 1 ].cint = 0 ;
    while ( k++ < for_end ) ;} 
  } 
  if ( np >= 7 ) 
  fontparams [f ]= np ;
  else fontparams [f ]= 7 ;
  hyphenchar [f ]= eqtb [15209 ].cint ;
  skewchar [f ]= eqtb [15210 ].cint ;
  if ( bchlabel < nl ) 
  bcharlabel [f ]= bchlabel + ligkernbase [f ];
  else bcharlabel [f ]= 0 ;
  fontbchar [f ]= bchar ;
  fontfalsebchar [f ]= bchar ;
  if ( bchar <= ec ) 
  if ( bchar >= bc ) 
  {
    qw = fontinfo [charbase [f ]+ bchar ].qqqq ;
    if ( ( qw .b0 > 0 ) ) 
    fontfalsebchar [f ]= 256 ;
  } 
  fontname [f ]= nom ;
  fontarea [f ]= aire ;
  fontbc [f ]= bc ;
  fontec [f ]= ec ;
  fontglue [f ]= 0 ;
  charbase [f ]= charbase [f ];
  widthbase [f ]= widthbase [f ];
  ligkernbase [f ]= ligkernbase [f ];
  kernbase [f ]= kernbase [f ];
  extenbase [f ]= extenbase [f ];
  decr ( parambase [f ]) ;
  fmemptr = fmemptr + lf ;
  fontptr = f ;
  g = f ;
  goto lab30 ;
  lab11: {
      
    if ( interaction == 3 ) 
    ;
    printnl ( 262 ) ;
    print ( 798 ) ;
  } 
  sprintcs ( u ) ;
  printchar ( 61 ) ;
  printfilename ( nom , aire , 335 ) ;
  if ( s >= 0 ) 
  {
    print ( 740 ) ;
    printscaled ( s ) ;
    print ( 394 ) ;
  } 
  else if ( s != -1000 ) 
  {
    print ( 799 ) ;
    printint ( - (integer) s ) ;
  } 
  if ( fileopened ) 
  print ( 800 ) ;
  else print ( 801 ) ;
  {
    helpptr = 5 ;
    helpline [4 ]= 802 ;
    helpline [3 ]= 803 ;
    helpline [2 ]= 804 ;
    helpline [1 ]= 805 ;
    helpline [0 ]= 806 ;
  } 
  error () ;
  lab30: if ( fileopened ) 
  bclose ( tfmfile ) ;
  Result = g ;
  return Result ;
} 
