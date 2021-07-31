#define EXTERN extern
#include "texd.h"

void println ( ) 
{println_regmem 
  switch ( selector ) 
  {case 19 : 
    {
      (void) putc('\n',  stdout );
      (void) putc('\n',  logfile );
      termoffset = 0 ; 
      fileoffset = 0 ; 
    } 
    break ; 
  case 18 : 
    {
      (void) putc('\n',  logfile );
      fileoffset = 0 ; 
    } 
    break ; 
  case 17 : 
    {
      (void) putc('\n',  stdout );
      termoffset = 0 ; 
    } 
    break ; 
  case 16 : 
  case 20 : 
  case 21 : 
    ; 
    break ; 
    default: 
    (void) putc('\n',  writefile [ selector ] );
    break ; 
  } 
} 
void printchar ( ASCIIcode s ) 
{/* 10 */ printchar_regmem 
  if ( s == eqtb [ 12712 ] .cint ) 
  if ( selector < 20 ) 
  {
    println () ; 
    return ; 
  } 
  switch ( selector ) 
  {case 19 : 
    {
      (void) putc( Xchr ( s ) ,  stdout );
      (void) putc( Xchr ( s ) ,  logfile );
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
  case 18 : 
    {
      (void) putc( Xchr ( s ) ,  logfile );
      incr ( fileoffset ) ; 
      if ( fileoffset == maxprintline ) 
      println () ; 
    } 
    break ; 
  case 17 : 
    {
      (void) putc( Xchr ( s ) ,  stdout );
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
    trickbuf [ tally % errorline ] = s ; 
    break ; 
  case 21 : 
    {
      if ( poolptr < poolsize ) 
      {
	strpool [ poolptr ] = s ; 
	incr ( poolptr ) ; 
      } 
    } 
    break ; 
    default: 
    (void) putc( Xchr ( s ) ,  writefile [ selector ] );
    break ; 
  } 
  incr ( tally ) ; 
} 
void print ( integer s ) 
{/* 10 */ print_regmem 
  poolpointer j  ; 
  if ( s >= strptr ) 
  s = 259 ; 
  else if ( s < 256 ) 
  if ( s < 0 ) 
  s = 259 ; 
  else if ( ( s == eqtb [ 12712 ] .cint ) ) 
  if ( selector < 20 ) 
  {
    println () ; 
    return ; 
  } 
  j = strstart [ s ] ; 
  while ( j < strstart [ s + 1 ] ) {
      
    printchar ( strpool [ j ] ) ; 
    incr ( j ) ; 
  } 
} 
void slowprint ( integer s ) 
{/* 10 */ slowprint_regmem 
  poolpointer j  ; 
  if ( s >= strptr ) 
  s = 259 ; 
  else if ( s < 256 ) 
  if ( s < 0 ) 
  s = 259 ; 
  else if ( ( s == eqtb [ 12712 ] .cint ) ) 
  if ( selector < 20 ) 
  {
    println () ; 
    return ; 
  } 
  j = strstart [ s ] ; 
  while ( j < strstart [ s + 1 ] ) {
      
    print ( strpool [ j ] ) ; 
    incr ( j ) ; 
  } 
} 
void printnl ( strnumber s ) 
{printnl_regmem 
  if ( ( ( termoffset > 0 ) && ( odd ( selector ) ) ) || ( ( fileoffset > 0 ) 
  && ( selector >= 18 ) ) ) 
  println () ; 
  print ( s ) ; 
} 
void printesc ( strnumber s ) 
{printesc_regmem 
  integer c  ; 
  c = eqtb [ 12708 ] .cint ; 
  if ( c >= 0 ) 
  if ( c < 256 ) 
  print ( c ) ; 
  print ( s ) ; 
} 
void printthedigs ( eightbits k ) 
{printthedigs_regmem 
  while ( k > 0 ) {
      
    decr ( k ) ; 
    if ( dig [ k ] < 10 ) 
    printchar ( 48 + dig [ k ] ) ; 
    else printchar ( 55 + dig [ k ] ) ; 
  } 
} 
void printint ( integer n ) 
{printint_regmem 
  schar k  ; 
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
void printcs ( integer p ) 
{printcs_regmem 
  if ( p < 514 ) 
  if ( p >= 257 ) 
  if ( p == 513 ) 
  {
    printesc ( 500 ) ; 
    printesc ( 501 ) ; 
  } 
  else {
      
    printesc ( p - 257 ) ; 
    if ( eqtb [ 11383 + p - 257 ] .hh .v.RH == 11 ) 
    printchar ( 32 ) ; 
  } 
  else if ( p < 1 ) 
  printesc ( 502 ) ; 
  else print ( p - 1 ) ; 
  else if ( p >= 10281 ) 
  printesc ( 502 ) ; 
  else if ( ( hash [ p ] .v.RH >= strptr ) ) 
  printesc ( 503 ) ; 
  else {
      
    printesc ( 335 ) ; 
    slowprint ( hash [ p ] .v.RH ) ; 
    printchar ( 32 ) ; 
  } 
} 
void sprintcs ( halfword p ) 
{sprintcs_regmem 
  if ( p < 514 ) 
  if ( p < 257 ) 
  print ( p - 1 ) ; 
  else if ( p < 513 ) 
  printesc ( p - 257 ) ; 
  else {
      
    printesc ( 500 ) ; 
    printesc ( 501 ) ; 
  } 
  else {
      
    printesc ( 335 ) ; 
    slowprint ( hash [ p ] .v.RH ) ; 
  } 
} 
void printfilename ( integer n , integer a , integer e ) 
{printfilename_regmem 
  print ( a ) ; 
  print ( n ) ; 
  print ( e ) ; 
} 
void printsize ( integer s ) 
{printsize_regmem 
  if ( s == 0 ) 
  printesc ( 408 ) ; 
  else if ( s == 16 ) 
  printesc ( 409 ) ; 
  else printesc ( 410 ) ; 
} 
void printwritewhatsit ( strnumber s , halfword p ) 
{printwritewhatsit_regmem 
  printesc ( s ) ; 
  if ( mem [ p + 1 ] .hh .v.LH < 16 ) 
  printint ( mem [ p + 1 ] .hh .v.LH ) ; 
  else if ( mem [ p + 1 ] .hh .v.LH == 16 ) 
  printchar ( 42 ) ; 
  else printchar ( 45 ) ; 
} 
#ifdef DEBUG
#endif /* DEBUG */
void jumpout ( ) 
{jumpout_regmem 
  closefilesandterminate () ; 
  {
    termflush ( stdout ) ; 
    readyalready = 0 ; 
    if ( ( history != 0 ) && ( history != 1 ) ) 
    uexit ( 1 ) ; 
    else uexit ( 0 ) ; 
  } 
} 
void error ( ) 
{/* 22 10 */ error_regmem 
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
      wakeupterminal () ; 
      print ( 264 ) ; 
      terminput () ; 
    } 
    if ( last == first ) 
    return ; 
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
	s1 = curtok ; 
	s2 = curcmd ; 
	s3 = curchr ; 
	s4 = alignstate ; 
	alignstate = 1000000L ; 
	OKtointerrupt = false ; 
	if ( ( last > first + 1 ) && ( buffer [ first + 1 ] >= 48 ) && ( 
	buffer [ first + 1 ] <= 57 ) ) 
	c = c * 10 + buffer [ first + 1 ] - 48 * 11 ; 
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
	  helpline [ 1 ] = 277 ; 
	  helpline [ 0 ] = 278 ; 
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
      if ( baseptr > 0 ) 
      {
	editnamestart = strstart [ inputstack [ baseptr ] .namefield ] ; 
	editnamelength = strstart [ inputstack [ baseptr ] .namefield + 1 ] - 
	strstart [ inputstack [ baseptr ] .namefield ] ; 
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
	    helpline [ 1 ] = 279 ; 
	    helpline [ 0 ] = 280 ; 
	  } 
	  do {
	      decr ( helpptr ) ; 
	    print ( helpline [ helpptr ] ) ; 
	    println () ; 
	  } while ( ! ( helpptr == 0 ) ) ; 
	} 
	{
	  helpptr = 4 ; 
	  helpline [ 3 ] = 281 ; 
	  helpline [ 2 ] = 280 ; 
	  helpline [ 1 ] = 282 ; 
	  helpline [ 0 ] = 283 ; 
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
	    wakeupterminal () ; 
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
	termflush ( stdout ) ; 
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
    printnl ( helpline [ helpptr ] ) ; 
  } 
  println () ; 
  if ( interaction > 0 ) 
  incr ( selector ) ; 
  println () ; 
} 
void fatalerror ( strnumber s ) 
{fatalerror_regmem 
  normalizeselector () ; 
  {
    if ( interaction == 3 ) 
    wakeupterminal () ; 
    printnl ( 262 ) ; 
    print ( 285 ) ; 
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
void overflow ( strnumber s , integer n ) 
{overflow_regmem 
  normalizeselector () ; 
  {
    if ( interaction == 3 ) 
    wakeupterminal () ; 
    printnl ( 262 ) ; 
    print ( 286 ) ; 
  } 
  print ( s ) ; 
  printchar ( 61 ) ; 
  printint ( n ) ; 
  printchar ( 93 ) ; 
  {
    helpptr = 2 ; 
    helpline [ 1 ] = 287 ; 
    helpline [ 0 ] = 288 ; 
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
void confusion ( strnumber s ) 
{confusion_regmem 
  normalizeselector () ; 
  if ( history < 2 ) 
  {
    {
      if ( interaction == 3 ) 
      wakeupterminal () ; 
      printnl ( 262 ) ; 
      print ( 289 ) ; 
    } 
    print ( s ) ; 
    printchar ( 41 ) ; 
    {
      helpptr = 1 ; 
      helpline [ 0 ] = 290 ; 
    } 
  } 
  else {
      
    {
      if ( interaction == 3 ) 
      wakeupterminal () ; 
      printnl ( 262 ) ; 
      print ( 291 ) ; 
    } 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 292 ; 
      helpline [ 0 ] = 293 ; 
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
{/* 10 */ register boolean Result; initterminal_regmem 
  topenin () ; 
  if ( last > first ) 
  {
    curinput .locfield = first ; 
    while ( ( curinput .locfield < last ) && ( buffer [ curinput .locfield ] 
    == ' ' ) ) incr ( curinput .locfield ) ; 
    if ( curinput .locfield < last ) 
    {
      Result = true ; 
      return(Result) ; 
    } 
  } 
  while ( true ) {
      
    wakeupterminal () ; 
    (void) Fputs( stdout ,  "**" ) ; 
    termflush ( stdout ) ; 
    if ( ! inputln ( stdin , true ) ) 
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "! End of file on the terminal... why?" ) ; 
      Result = false ; 
      return(Result) ; 
    } 
    curinput .locfield = first ; 
    while ( ( curinput .locfield < last ) && ( buffer [ curinput .locfield ] 
    == 32 ) ) incr ( curinput .locfield ) ; 
    if ( curinput .locfield < last ) 
    {
      Result = true ; 
      return(Result) ; 
    } 
    (void) fprintf( stdout , "%s\n",  "Please type the name of your input file." ) ; 
  } 
  return(Result) ; 
} 
strnumber makestring ( ) 
{register strnumber Result; makestring_regmem 
  if ( strptr == maxstrings ) 
  overflow ( 258 , maxstrings - initstrptr ) ; 
  incr ( strptr ) ; 
  strstart [ strptr ] = poolptr ; 
  Result = strptr - 1 ; 
  return(Result) ; 
} 
boolean streqbuf ( strnumber s , integer k ) 
{/* 45 */ register boolean Result; streqbuf_regmem 
  poolpointer j  ; 
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
boolean streqstr ( strnumber s , strnumber t ) 
{/* 45 */ register boolean Result; streqstr_regmem 
  poolpointer j, k  ; 
  boolean result  ; 
  result = false ; 
  if ( ( strstart [ s + 1 ] - strstart [ s ] ) != ( strstart [ t + 1 ] - 
  strstart [ t ] ) ) 
  goto lab45 ; 
  j = strstart [ s ] ; 
  k = strstart [ t ] ; 
  while ( j < strstart [ s + 1 ] ) {
      
    if ( strpool [ j ] != strpool [ k ] ) 
    goto lab45 ; 
    incr ( j ) ; 
    incr ( k ) ; 
  } 
  result = true ; 
  lab45: Result = result ; 
  return(Result) ; 
} 
void printtwo ( integer n ) 
{printtwo_regmem 
  n = abs ( n ) % 100 ; 
  printchar ( 48 + ( n / 10 ) ) ; 
  printchar ( 48 + ( n % 10 ) ) ; 
} 
void printhex ( integer n ) 
{printhex_regmem 
  schar k  ; 
  k = 0 ; 
  printchar ( 34 ) ; 
  do {
      dig [ k ] = n % 16 ; 
    n = n / 16 ; 
    incr ( k ) ; 
  } while ( ! ( n == 0 ) ) ; 
  printthedigs ( k ) ; 
} 
void printromanint ( integer n ) 
{/* 10 */ printromanint_regmem 
  poolpointer j, k  ; 
  nonnegativeinteger u, v  ; 
  j = strstart [ 260 ] ; 
  v = 1000 ; 
  while ( true ) {
      
    while ( n >= v ) {
	
      printchar ( strpool [ j ] ) ; 
      n = n - v ; 
    } 
    if ( n <= 0 ) 
    return ; 
    k = j + 2 ; 
    u = v / ( strpool [ k - 1 ] - 48 ) ; 
    if ( strpool [ k - 1 ] == 50 ) 
    {
      k = k + 2 ; 
      u = u / ( strpool [ k - 1 ] - 48 ) ; 
    } 
    if ( n + u >= v ) 
    {
      printchar ( strpool [ k ] ) ; 
      n = n + u ; 
    } 
    else {
	
      j = j + 2 ; 
      v = v / ( strpool [ j - 1 ] - 48 ) ; 
    } 
  } 
} 
void printcurrentstring ( ) 
{printcurrentstring_regmem 
  poolpointer j  ; 
  j = strstart [ strptr ] ; 
  while ( j < poolptr ) {
      
    printchar ( strpool [ j ] ) ; 
    incr ( j ) ; 
  } 
} 
void terminput ( ) 
{terminput_regmem 
  integer k  ; 
  termflush ( stdout ) ; 
  if ( ! inputln ( stdin , true ) ) 
  fatalerror ( 261 ) ; 
  termoffset = 0 ; 
  decr ( selector ) ; 
  if ( last != first ) 
  {register integer for_end; k = first ; for_end = last - 1 ; if ( k <= 
  for_end) do 
    print ( buffer [ k ] ) ; 
  while ( k++ < for_end ) ; } 
  println () ; 
  incr ( selector ) ; 
} 
void interror ( integer n ) 
{interror_regmem 
  print ( 284 ) ; 
  printint ( n ) ; 
  printchar ( 41 ) ; 
  error () ; 
} 
void normalizeselector ( ) 
{normalizeselector_regmem 
  if ( logopened ) 
  selector = 19 ; 
  else selector = 17 ; 
  if ( jobname == 0 ) 
  openlogfile () ; 
  if ( interaction == 0 ) 
  decr ( selector ) ; 
} 
void pauseforinstructions ( ) 
{pauseforinstructions_regmem 
  if ( OKtointerrupt ) 
  {
    interaction = 3 ; 
    if ( ( selector == 18 ) || ( selector == 16 ) ) 
    incr ( selector ) ; 
    {
      if ( interaction == 3 ) 
      wakeupterminal () ; 
      printnl ( 262 ) ; 
      print ( 294 ) ; 
    } 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 295 ; 
      helpline [ 1 ] = 296 ; 
      helpline [ 0 ] = 297 ; 
    } 
    deletionsallowed = false ; 
    error () ; 
    deletionsallowed = true ; 
    interrupt = 0 ; 
  } 
} 
integer half ( integer x ) 
{register integer Result; half_regmem 
  if ( odd ( x ) ) 
  Result = ( x + 1 ) / 2 ; 
  else Result = x / 2 ; 
  return(Result) ; 
} 
scaled rounddecimals ( smallnumber k ) 
{register scaled Result; rounddecimals_regmem 
  integer a  ; 
  a = 0 ; 
  while ( k > 0 ) {
      
    decr ( k ) ; 
    a = ( a + dig [ k ] * 131072L ) / 10 ; 
  } 
  Result = ( a + 1 ) / 2 ; 
  return(Result) ; 
} 
void printscaled ( scaled s ) 
{printscaled_regmem 
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
scaled multandadd ( integer n , scaled x , scaled y , scaled maxanswer ) 
{register scaled Result; multandadd_regmem 
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
  return(Result) ; 
} 
scaled xovern ( scaled x , integer n ) 
{register scaled Result; xovern_regmem 
  boolean negative  ; 
  negative = false ; 
  if ( n == 0 ) 
  {
    aritherror = true ; 
    Result = 0 ; 
    remainder = x ; 
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
      remainder = x % n ; 
    } 
    else {
	
      Result = - (integer) ( ( - (integer) x ) / n ) ; 
      remainder = - (integer) ( ( - (integer) x ) % n ) ; 
    } 
  } 
  if ( negative ) 
  remainder = - (integer) remainder ; 
  return(Result) ; 
} 
scaled xnoverd ( scaled x , integer n , integer d ) 
{register scaled Result; xnoverd_regmem 
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
    remainder = v % d ; 
  } 
  else {
      
    Result = - (integer) u ; 
    remainder = - (integer) ( v % d ) ; 
  } 
  return(Result) ; 
} 
halfword badness ( scaled t , scaled s ) 
{register halfword Result; badness_regmem 
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
  return(Result) ; 
} 
#ifdef DEBUG
void printword ( memoryword w ) 
{printword_regmem 
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
#endif /* DEBUG */
void showtokenlist ( integer p , integer q , integer l ) 
{/* 10 */ showtokenlist_regmem 
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
    if ( mem [ p ] .hh .v.LH >= 4095 ) 
    printcs ( mem [ p ] .hh .v.LH - 4095 ) ; 
    else {
	
      m = mem [ p ] .hh .v.LH / 256 ; 
      c = mem [ p ] .hh .v.LH % 256 ; 
      if ( mem [ p ] .hh .v.LH < 0 ) 
      printesc ( 551 ) ; 
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
	print ( 552 ) ; 
	break ; 
	default: 
	printesc ( 551 ) ; 
	break ; 
      } 
    } 
    p = mem [ p ] .hh .v.RH ; 
  } 
  if ( p != 0 ) 
  printesc ( 550 ) ; 
} 
void runaway ( ) 
{runaway_regmem 
  halfword p  ; 
  if ( scannerstatus > 1 ) 
  {
    printnl ( 565 ) ; 
    switch ( scannerstatus ) 
    {case 2 : 
      {
	print ( 566 ) ; 
	p = defref ; 
      } 
      break ; 
    case 3 : 
      {
	print ( 567 ) ; 
	p = memtop - 3 ; 
      } 
      break ; 
    case 4 : 
      {
	print ( 568 ) ; 
	p = memtop - 4 ; 
      } 
      break ; 
    case 5 : 
      {
	print ( 569 ) ; 
	p = defref ; 
      } 
      break ; 
    } 
    printchar ( 63 ) ; 
    println () ; 
    showtokenlist ( mem [ p ] .hh .v.RH , 0 , errorline - 10 ) ; 
  } 
} 
halfword getavail ( ) 
{register halfword Result; getavail_regmem 
  halfword p  ; 
  p = avail ; 
  if ( p != 0 ) 
  avail = mem [ avail ] .hh .v.RH ; 
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
  mem [ p ] .hh .v.RH = 0 ; 
	;
#ifdef STAT
  incr ( dynused ) ; 
#endif /* STAT */
  Result = p ; 
  return(Result) ; 
} 
void flushlist ( halfword p ) 
{flushlist_regmem 
  halfword q, r  ; 
  if ( p != 0 ) 
  {
    r = p ; 
    do {
	q = r ; 
      r = mem [ r ] .hh .v.RH ; 
	;
#ifdef STAT
      decr ( dynused ) ; 
#endif /* STAT */
    } while ( ! ( r == 0 ) ) ; 
    mem [ q ] .hh .v.RH = avail ; 
    avail = p ; 
  } 
} 
halfword getnode ( integer s ) 
{/* 40 10 20 */ register halfword Result; getnode_regmem 
  halfword p  ; 
  halfword q  ; 
  integer r  ; 
  integer t  ; 
  lab20: p = rover ; 
  do {
      q = p + mem [ p ] .hh .v.LH ; 
    while ( ( mem [ q ] .hh .v.RH == 262143L ) ) {
	
      t = mem [ q + 1 ] .hh .v.RH ; 
      if ( q == rover ) 
      rover = t ; 
      mem [ t + 1 ] .hh .v.LH = mem [ q + 1 ] .hh .v.LH ; 
      mem [ mem [ q + 1 ] .hh .v.LH + 1 ] .hh .v.RH = t ; 
      q = q + mem [ q ] .hh .v.LH ; 
    } 
    r = q - s ; 
    if ( r > toint ( p + 1 ) ) 
    {
      mem [ p ] .hh .v.LH = r - p ; 
      rover = p ; 
      goto lab40 ; 
    } 
    if ( r == p ) 
    if ( mem [ p + 1 ] .hh .v.RH != p ) 
    {
      rover = mem [ p + 1 ] .hh .v.RH ; 
      t = mem [ p + 1 ] .hh .v.LH ; 
      mem [ rover + 1 ] .hh .v.LH = t ; 
      mem [ t + 1 ] .hh .v.RH = rover ; 
      goto lab40 ; 
    } 
    mem [ p ] .hh .v.LH = q - p ; 
    p = mem [ p + 1 ] .hh .v.RH ; 
  } while ( ! ( p == rover ) ) ; 
  if ( s == 1073741824L ) 
  {
    Result = 262143L ; 
    return(Result) ; 
  } 
  if ( lomemmax + 2 < himemmin ) 
  if ( lomemmax + 2 <= 262143L ) 
  {
    if ( himemmin - lomemmax >= 1998 ) 
    t = lomemmax + 1000 ; 
    else t = lomemmax + 1 + ( himemmin - lomemmax ) / 2 ; 
    p = mem [ rover + 1 ] .hh .v.LH ; 
    q = lomemmax ; 
    mem [ p + 1 ] .hh .v.RH = q ; 
    mem [ rover + 1 ] .hh .v.LH = q ; 
    if ( t > 262143L ) 
    t = 262143L ; 
    mem [ q + 1 ] .hh .v.RH = rover ; 
    mem [ q + 1 ] .hh .v.LH = p ; 
    mem [ q ] .hh .v.RH = 262143L ; 
    mem [ q ] .hh .v.LH = t - lomemmax ; 
    lomemmax = t ; 
    mem [ lomemmax ] .hh .v.RH = 0 ; 
    mem [ lomemmax ] .hh .v.LH = 0 ; 
    rover = q ; 
    goto lab20 ; 
  } 
  overflow ( 298 , memmax + 1 - memmin ) ; 
  lab40: mem [ r ] .hh .v.RH = 0 ; 
	;
#ifdef STAT
  varused = varused + s ; 
#endif /* STAT */
  Result = r ; 
  return(Result) ; 
} 
void freenode ( halfword p , halfword s ) 
{freenode_regmem 
  halfword q  ; 
  mem [ p ] .hh .v.LH = s ; 
  mem [ p ] .hh .v.RH = 262143L ; 
  q = mem [ rover + 1 ] .hh .v.LH ; 
  mem [ p + 1 ] .hh .v.LH = q ; 
  mem [ p + 1 ] .hh .v.RH = rover ; 
  mem [ rover + 1 ] .hh .v.LH = p ; 
  mem [ q + 1 ] .hh .v.RH = p ; 
	;
#ifdef STAT
  varused = varused - s ; 
#endif /* STAT */
} 
halfword newnullbox ( ) 
{register halfword Result; newnullbox_regmem 
  halfword p  ; 
  p = getnode ( 7 ) ; 
  mem [ p ] .hh.b0 = 0 ; 
  mem [ p ] .hh.b1 = 0 ; 
  mem [ p + 1 ] .cint = 0 ; 
  mem [ p + 2 ] .cint = 0 ; 
  mem [ p + 3 ] .cint = 0 ; 
  mem [ p + 4 ] .cint = 0 ; 
  mem [ p + 5 ] .hh .v.RH = 0 ; 
  mem [ p + 5 ] .hh.b0 = 0 ; 
  mem [ p + 5 ] .hh.b1 = 0 ; 
  mem [ p + 6 ] .gr = 0.0 ; 
  Result = p ; 
  return(Result) ; 
} 
halfword newrule ( ) 
{register halfword Result; newrule_regmem 
  halfword p  ; 
  p = getnode ( 4 ) ; 
  mem [ p ] .hh.b0 = 2 ; 
  mem [ p ] .hh.b1 = 0 ; 
  mem [ p + 1 ] .cint = -1073741824L ; 
  mem [ p + 2 ] .cint = -1073741824L ; 
  mem [ p + 3 ] .cint = -1073741824L ; 
  Result = p ; 
  return(Result) ; 
} 
halfword newligature ( quarterword f , quarterword c , halfword q ) 
{register halfword Result; newligature_regmem 
  halfword p  ; 
  p = getnode ( 2 ) ; 
  mem [ p ] .hh.b0 = 6 ; 
  mem [ p + 1 ] .hh.b0 = f ; 
  mem [ p + 1 ] .hh.b1 = c ; 
  mem [ p + 1 ] .hh .v.RH = q ; 
  mem [ p ] .hh.b1 = 0 ; 
  Result = p ; 
  return(Result) ; 
} 
halfword newligitem ( quarterword c ) 
{register halfword Result; newligitem_regmem 
  halfword p  ; 
  p = getnode ( 2 ) ; 
  mem [ p ] .hh.b1 = c ; 
  mem [ p + 1 ] .hh .v.RH = 0 ; 
  Result = p ; 
  return(Result) ; 
} 
halfword newdisc ( ) 
{register halfword Result; newdisc_regmem 
  halfword p  ; 
  p = getnode ( 2 ) ; 
  mem [ p ] .hh.b0 = 7 ; 
  mem [ p ] .hh.b1 = 0 ; 
  mem [ p + 1 ] .hh .v.LH = 0 ; 
  mem [ p + 1 ] .hh .v.RH = 0 ; 
  Result = p ; 
  return(Result) ; 
} 
halfword newmath ( scaled w , smallnumber s ) 
{register halfword Result; newmath_regmem 
  halfword p  ; 
  p = getnode ( 2 ) ; 
  mem [ p ] .hh.b0 = 9 ; 
  mem [ p ] .hh.b1 = s ; 
  mem [ p + 1 ] .cint = w ; 
  Result = p ; 
  return(Result) ; 
} 
halfword newspec ( halfword p ) 
{register halfword Result; newspec_regmem 
  halfword q  ; 
  q = getnode ( 4 ) ; 
  mem [ q ] = mem [ p ] ; 
  mem [ q ] .hh .v.RH = 0 ; 
  mem [ q + 1 ] .cint = mem [ p + 1 ] .cint ; 
  mem [ q + 2 ] .cint = mem [ p + 2 ] .cint ; 
  mem [ q + 3 ] .cint = mem [ p + 3 ] .cint ; 
  Result = q ; 
  return(Result) ; 
} 
halfword newparamglue ( smallnumber n ) 
{register halfword Result; newparamglue_regmem 
  halfword p  ; 
  halfword q  ; 
  p = getnode ( 2 ) ; 
  mem [ p ] .hh.b0 = 10 ; 
  mem [ p ] .hh.b1 = n + 1 ; 
  mem [ p + 1 ] .hh .v.RH = 0 ; 
  q = eqtb [ 10282 + n ] .hh .v.RH ; 
  mem [ p + 1 ] .hh .v.LH = q ; 
  incr ( mem [ q ] .hh .v.RH ) ; 
  Result = p ; 
  return(Result) ; 
} 
halfword newglue ( halfword q ) 
{register halfword Result; newglue_regmem 
  halfword p  ; 
  p = getnode ( 2 ) ; 
  mem [ p ] .hh.b0 = 10 ; 
  mem [ p ] .hh.b1 = 0 ; 
  mem [ p + 1 ] .hh .v.RH = 0 ; 
  mem [ p + 1 ] .hh .v.LH = q ; 
  incr ( mem [ q ] .hh .v.RH ) ; 
  Result = p ; 
  return(Result) ; 
} 
halfword newskipparam ( smallnumber n ) 
{register halfword Result; newskipparam_regmem 
  halfword p  ; 
  tempptr = newspec ( eqtb [ 10282 + n ] .hh .v.RH ) ; 
  p = newglue ( tempptr ) ; 
  mem [ tempptr ] .hh .v.RH = 0 ; 
  mem [ p ] .hh.b1 = n + 1 ; 
  Result = p ; 
  return(Result) ; 
} 
halfword newkern ( scaled w ) 
{register halfword Result; newkern_regmem 
  halfword p  ; 
  p = getnode ( 2 ) ; 
  mem [ p ] .hh.b0 = 11 ; 
  mem [ p ] .hh.b1 = 0 ; 
  mem [ p + 1 ] .cint = w ; 
  Result = p ; 
  return(Result) ; 
} 
halfword newpenalty ( integer m ) 
{register halfword Result; newpenalty_regmem 
  halfword p  ; 
  p = getnode ( 2 ) ; 
  mem [ p ] .hh.b0 = 12 ; 
  mem [ p ] .hh.b1 = 0 ; 
  mem [ p + 1 ] .cint = m ; 
  Result = p ; 
  return(Result) ; 
} 
#ifdef DEBUG
void checkmem ( boolean printlocs ) 
{/* 31 32 */ checkmem_regmem 
  halfword p, q  ; 
  boolean clobbered  ; 
  {register integer for_end; p = memmin ; for_end = lomemmax ; if ( p <= 
  for_end) do 
    freearr [ p ] = false ; 
  while ( p++ < for_end ) ; } 
  {register integer for_end; p = himemmin ; for_end = memend ; if ( p <= 
  for_end) do 
    freearr [ p ] = false ; 
  while ( p++ < for_end ) ; } 
  p = avail ; 
  q = 0 ; 
  clobbered = false ; 
  while ( p != 0 ) {
      
    if ( ( p > memend ) || ( p < himemmin ) ) 
    clobbered = true ; 
    else if ( freearr [ p ] ) 
    clobbered = true ; 
    if ( clobbered ) 
    {
      printnl ( 299 ) ; 
      printint ( q ) ; 
      goto lab31 ; 
    } 
    freearr [ p ] = true ; 
    q = p ; 
    p = mem [ q ] .hh .v.RH ; 
  } 
  lab31: ; 
  p = rover ; 
  q = 0 ; 
  clobbered = false ; 
  do {
      if ( ( p >= lomemmax ) || ( p < memmin ) ) 
    clobbered = true ; 
    else if ( ( mem [ p + 1 ] .hh .v.RH >= lomemmax ) || ( mem [ p + 1 ] .hh 
    .v.RH < memmin ) ) 
    clobbered = true ; 
    else if ( ! ( ( mem [ p ] .hh .v.RH == 262143L ) ) || ( mem [ p ] .hh 
    .v.LH < 2 ) || ( p + mem [ p ] .hh .v.LH > lomemmax ) || ( mem [ mem [ p + 
    1 ] .hh .v.RH + 1 ] .hh .v.LH != p ) ) 
    clobbered = true ; 
    if ( clobbered ) 
    {
      printnl ( 300 ) ; 
      printint ( q ) ; 
      goto lab32 ; 
    } 
    {register integer for_end; q = p ; for_end = p + mem [ p ] .hh .v.LH - 1 
    ; if ( q <= for_end) do 
      {
	if ( freearr [ q ] ) 
	{
	  printnl ( 301 ) ; 
	  printint ( q ) ; 
	  goto lab32 ; 
	} 
	freearr [ q ] = true ; 
      } 
    while ( q++ < for_end ) ; } 
    q = p ; 
    p = mem [ p + 1 ] .hh .v.RH ; 
  } while ( ! ( p == rover ) ) ; 
  lab32: ; 
  p = memmin ; 
  while ( p <= lomemmax ) {
      
    if ( ( mem [ p ] .hh .v.RH == 262143L ) ) 
    {
      printnl ( 302 ) ; 
      printint ( p ) ; 
    } 
    while ( ( p <= lomemmax ) && ! freearr [ p ] ) incr ( p ) ; 
    while ( ( p <= lomemmax ) && freearr [ p ] ) incr ( p ) ; 
  } 
  if ( printlocs ) 
  {
    printnl ( 303 ) ; 
    {register integer for_end; p = memmin ; for_end = lomemmax ; if ( p <= 
    for_end) do 
      if ( ! freearr [ p ] && ( ( p > waslomax ) || wasfree [ p ] ) ) 
      {
	printchar ( 32 ) ; 
	printint ( p ) ; 
      } 
    while ( p++ < for_end ) ; } 
    {register integer for_end; p = himemmin ; for_end = memend ; if ( p <= 
    for_end) do 
      if ( ! freearr [ p ] && ( ( p < washimin ) || ( p > wasmemend ) || 
      wasfree [ p ] ) ) 
      {
	printchar ( 32 ) ; 
	printint ( p ) ; 
      } 
    while ( p++ < for_end ) ; } 
  } 
  {register integer for_end; p = memmin ; for_end = lomemmax ; if ( p <= 
  for_end) do 
    wasfree [ p ] = freearr [ p ] ; 
  while ( p++ < for_end ) ; } 
  {register integer for_end; p = himemmin ; for_end = memend ; if ( p <= 
  for_end) do 
    wasfree [ p ] = freearr [ p ] ; 
  while ( p++ < for_end ) ; } 
  wasmemend = memend ; 
  waslomax = lomemmax ; 
  washimin = himemmin ; 
} 
#endif /* DEBUG */
#ifdef DEBUG
void searchmem ( halfword p ) 
{searchmem_regmem 
  integer q  ; 
  {register integer for_end; q = memmin ; for_end = lomemmax ; if ( q <= 
  for_end) do 
    {
      if ( mem [ q ] .hh .v.RH == p ) 
      {
	printnl ( 304 ) ; 
	printint ( q ) ; 
	printchar ( 41 ) ; 
      } 
      if ( mem [ q ] .hh .v.LH == p ) 
      {
	printnl ( 305 ) ; 
	printint ( q ) ; 
	printchar ( 41 ) ; 
      } 
    } 
  while ( q++ < for_end ) ; } 
  {register integer for_end; q = himemmin ; for_end = memend ; if ( q <= 
  for_end) do 
    {
      if ( mem [ q ] .hh .v.RH == p ) 
      {
	printnl ( 304 ) ; 
	printint ( q ) ; 
	printchar ( 41 ) ; 
      } 
      if ( mem [ q ] .hh .v.LH == p ) 
      {
	printnl ( 305 ) ; 
	printint ( q ) ; 
	printchar ( 41 ) ; 
      } 
    } 
  while ( q++ < for_end ) ; } 
  {register integer for_end; q = 1 ; for_end = 11333 ; if ( q <= for_end) do 
    {
      if ( eqtb [ q ] .hh .v.RH == p ) 
      {
	printnl ( 497 ) ; 
	printint ( q ) ; 
	printchar ( 41 ) ; 
      } 
    } 
  while ( q++ < for_end ) ; } 
  if ( saveptr > 0 ) 
  {register integer for_end; q = 0 ; for_end = saveptr - 1 ; if ( q <= 
  for_end) do 
    {
      if ( savestack [ q ] .hh .v.RH == p ) 
      {
	printnl ( 542 ) ; 
	printint ( q ) ; 
	printchar ( 41 ) ; 
      } 
    } 
  while ( q++ < for_end ) ; } 
  {register integer for_end; q = 0 ; for_end = 607 ; if ( q <= for_end) do 
    {
      if ( hyphlist [ q ] == p ) 
      {
	printnl ( 933 ) ; 
	printint ( q ) ; 
	printchar ( 41 ) ; 
      } 
    } 
  while ( q++ < for_end ) ; } 
} 
#endif /* DEBUG */
void shortdisplay ( integer p ) 
{shortdisplay_regmem 
  integer n  ; 
  while ( p > memmin ) {
      
    if ( ( p >= himemmin ) ) 
    {
      if ( p <= memend ) 
      {
	if ( mem [ p ] .hh.b0 != fontinshortdisplay ) 
	{
	  if ( ( mem [ p ] .hh.b0 > fontmax ) ) 
	  printchar ( 42 ) ; 
	  else printesc ( hash [ 10024 + mem [ p ] .hh.b0 ] .v.RH ) ; 
	  printchar ( 32 ) ; 
	  fontinshortdisplay = mem [ p ] .hh.b0 ; 
	} 
	print ( mem [ p ] .hh.b1 ) ; 
      } 
    } 
    else switch ( mem [ p ] .hh.b0 ) 
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
      if ( mem [ p + 1 ] .hh .v.LH != 0 ) 
      printchar ( 32 ) ; 
      break ; 
    case 9 : 
      printchar ( 36 ) ; 
      break ; 
    case 6 : 
      shortdisplay ( mem [ p + 1 ] .hh .v.RH ) ; 
      break ; 
    case 7 : 
      {
	shortdisplay ( mem [ p + 1 ] .hh .v.LH ) ; 
	shortdisplay ( mem [ p + 1 ] .hh .v.RH ) ; 
	n = mem [ p ] .hh.b1 ; 
	while ( n > 0 ) {
	    
	  if ( mem [ p ] .hh .v.RH != 0 ) 
	  p = mem [ p ] .hh .v.RH ; 
	  decr ( n ) ; 
	} 
      } 
      break ; 
      default: 
      ; 
      break ; 
    } 
    p = mem [ p ] .hh .v.RH ; 
  } 
} 
void printfontandchar ( integer p ) 
{printfontandchar_regmem 
  if ( p > memend ) 
  printesc ( 307 ) ; 
  else {
      
    if ( ( mem [ p ] .hh.b0 > fontmax ) ) 
    printchar ( 42 ) ; 
    else printesc ( hash [ 10024 + mem [ p ] .hh.b0 ] .v.RH ) ; 
    printchar ( 32 ) ; 
    print ( mem [ p ] .hh.b1 ) ; 
  } 
} 
void printmark ( integer p ) 
{printmark_regmem 
  printchar ( 123 ) ; 
  if ( ( p < himemmin ) || ( p > memend ) ) 
  printesc ( 307 ) ; 
  else showtokenlist ( mem [ p ] .hh .v.RH , 0 , maxprintline - 10 ) ; 
  printchar ( 125 ) ; 
} 
void printruledimen ( scaled d ) 
{printruledimen_regmem 
  if ( ( d == -1073741824L ) ) 
  printchar ( 42 ) ; 
  else printscaled ( d ) ; 
} 
void printglue ( scaled d , integer order , strnumber s ) 
{printglue_regmem 
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
void printspec ( integer p , strnumber s ) 
{printspec_regmem 
  if ( ( p < memmin ) || ( p >= lomemmax ) ) 
  printchar ( 42 ) ; 
  else {
      
    printscaled ( mem [ p + 1 ] .cint ) ; 
    if ( s != 0 ) 
    print ( s ) ; 
    if ( mem [ p + 2 ] .cint != 0 ) 
    {
      print ( 310 ) ; 
      printglue ( mem [ p + 2 ] .cint , mem [ p ] .hh.b0 , s ) ; 
    } 
    if ( mem [ p + 3 ] .cint != 0 ) 
    {
      print ( 311 ) ; 
      printglue ( mem [ p + 3 ] .cint , mem [ p ] .hh.b1 , s ) ; 
    } 
  } 
} 
void printfamandchar ( halfword p ) 
{printfamandchar_regmem 
  printesc ( 460 ) ; 
  printint ( mem [ p ] .hh.b0 ) ; 
  printchar ( 32 ) ; 
  print ( mem [ p ] .hh.b1 ) ; 
} 
void printdelimiter ( halfword p ) 
{printdelimiter_regmem 
  integer a  ; 
  a = mem [ p ] .qqqq .b0 * 256 + mem [ p ] .qqqq .b1 ; 
  a = a * 4096 + mem [ p ] .qqqq .b2 * 256 + mem [ p ] .qqqq .b3 ; 
  if ( a < 0 ) 
  printint ( a ) ; 
  else printhex ( a ) ; 
} 
void printsubsidiarydata ( halfword p , ASCIIcode c ) 
{printsubsidiarydata_regmem 
  if ( ( poolptr - strstart [ strptr ] ) >= depththreshold ) 
  {
    if ( mem [ p ] .hh .v.RH != 0 ) 
    print ( 312 ) ; 
  } 
  else {
      
    {
      strpool [ poolptr ] = c ; 
      incr ( poolptr ) ; 
    } 
    tempptr = p ; 
    switch ( mem [ p ] .hh .v.RH ) 
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
      if ( mem [ p ] .hh .v.LH == 0 ) 
      {
	println () ; 
	printcurrentstring () ; 
	print ( 853 ) ; 
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
void printstyle ( integer c ) 
{printstyle_regmem 
  switch ( c / 2 ) 
  {case 0 : 
    printesc ( 854 ) ; 
    break ; 
  case 1 : 
    printesc ( 855 ) ; 
    break ; 
  case 2 : 
    printesc ( 856 ) ; 
    break ; 
  case 3 : 
    printesc ( 857 ) ; 
    break ; 
    default: 
    print ( 858 ) ; 
    break ; 
  } 
} 
void printskipparam ( integer n ) 
{printskipparam_regmem 
  switch ( n ) 
  {case 0 : 
    printesc ( 372 ) ; 
    break ; 
  case 1 : 
    printesc ( 373 ) ; 
    break ; 
  case 2 : 
    printesc ( 374 ) ; 
    break ; 
  case 3 : 
    printesc ( 375 ) ; 
    break ; 
  case 4 : 
    printesc ( 376 ) ; 
    break ; 
  case 5 : 
    printesc ( 377 ) ; 
    break ; 
  case 6 : 
    printesc ( 378 ) ; 
    break ; 
  case 7 : 
    printesc ( 379 ) ; 
    break ; 
  case 8 : 
    printesc ( 380 ) ; 
    break ; 
  case 9 : 
    printesc ( 381 ) ; 
    break ; 
  case 10 : 
    printesc ( 382 ) ; 
    break ; 
  case 11 : 
    printesc ( 383 ) ; 
    break ; 
  case 12 : 
    printesc ( 384 ) ; 
    break ; 
  case 13 : 
    printesc ( 385 ) ; 
    break ; 
  case 14 : 
    printesc ( 386 ) ; 
    break ; 
  case 15 : 
    printesc ( 387 ) ; 
    break ; 
  case 16 : 
    printesc ( 388 ) ; 
    break ; 
  case 17 : 
    printesc ( 389 ) ; 
    break ; 
    default: 
    print ( 390 ) ; 
    break ; 
  } 
} 
void shownodelist ( integer p ) 
{/* 10 */ shownodelist_regmem 
  integer n  ; 
  real g  ; 
  if ( ( poolptr - strstart [ strptr ] ) > depththreshold ) 
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
    else switch ( mem [ p ] .hh.b0 ) 
    {case 0 : 
    case 1 : 
    case 13 : 
      {
	if ( mem [ p ] .hh.b0 == 0 ) 
	printesc ( 104 ) ; 
	else if ( mem [ p ] .hh.b0 == 1 ) 
	printesc ( 118 ) ; 
	else printesc ( 316 ) ; 
	print ( 317 ) ; 
	printscaled ( mem [ p + 3 ] .cint ) ; 
	printchar ( 43 ) ; 
	printscaled ( mem [ p + 2 ] .cint ) ; 
	print ( 318 ) ; 
	printscaled ( mem [ p + 1 ] .cint ) ; 
	if ( mem [ p ] .hh.b0 == 13 ) 
	{
	  if ( mem [ p ] .hh.b1 != 0 ) 
	  {
	    print ( 284 ) ; 
	    printint ( mem [ p ] .hh.b1 + 1 ) ; 
	    print ( 320 ) ; 
	  } 
	  if ( mem [ p + 6 ] .cint != 0 ) 
	  {
	    print ( 321 ) ; 
	    printglue ( mem [ p + 6 ] .cint , mem [ p + 5 ] .hh.b1 , 0 ) ; 
	  } 
	  if ( mem [ p + 4 ] .cint != 0 ) 
	  {
	    print ( 322 ) ; 
	    printglue ( mem [ p + 4 ] .cint , mem [ p + 5 ] .hh.b0 , 0 ) ; 
	  } 
	} 
	else {
	    
	  g = mem [ p + 6 ] .gr ; 
	  if ( ( g != 0.0 ) && ( mem [ p + 5 ] .hh.b0 != 0 ) ) 
	  {
	    print ( 323 ) ; 
	    if ( mem [ p + 5 ] .hh.b0 == 2 ) 
	    print ( 324 ) ; 
	    if ( fabs ( g ) > 20000.0 ) 
	    {
	      if ( g > 0.0 ) 
	      printchar ( 62 ) ; 
	      else print ( 325 ) ; 
	      printglue ( 20000 * 65536L , mem [ p + 5 ] .hh.b1 , 0 ) ; 
	    } 
	    else printglue ( round ( 65536L * g ) , mem [ p + 5 ] .hh.b1 , 0 ) 
	    ; 
	  } 
	  if ( mem [ p + 4 ] .cint != 0 ) 
	  {
	    print ( 319 ) ; 
	    printscaled ( mem [ p + 4 ] .cint ) ; 
	  } 
	} 
	{
	  {
	    strpool [ poolptr ] = 46 ; 
	    incr ( poolptr ) ; 
	  } 
	  shownodelist ( mem [ p + 5 ] .hh .v.RH ) ; 
	  decr ( poolptr ) ; 
	} 
      } 
      break ; 
    case 2 : 
      {
	printesc ( 326 ) ; 
	printruledimen ( mem [ p + 3 ] .cint ) ; 
	printchar ( 43 ) ; 
	printruledimen ( mem [ p + 2 ] .cint ) ; 
	print ( 318 ) ; 
	printruledimen ( mem [ p + 1 ] .cint ) ; 
      } 
      break ; 
    case 3 : 
      {
	printesc ( 327 ) ; 
	printint ( mem [ p ] .hh.b1 ) ; 
	print ( 328 ) ; 
	printscaled ( mem [ p + 3 ] .cint ) ; 
	print ( 329 ) ; 
	printspec ( mem [ p + 4 ] .hh .v.RH , 0 ) ; 
	printchar ( 44 ) ; 
	printscaled ( mem [ p + 2 ] .cint ) ; 
	print ( 330 ) ; 
	printint ( mem [ p + 1 ] .cint ) ; 
	{
	  {
	    strpool [ poolptr ] = 46 ; 
	    incr ( poolptr ) ; 
	  } 
	  shownodelist ( mem [ p + 4 ] .hh .v.LH ) ; 
	  decr ( poolptr ) ; 
	} 
      } 
      break ; 
    case 8 : 
      switch ( mem [ p ] .hh.b1 ) 
      {case 0 : 
	{
	  printwritewhatsit ( 1276 , p ) ; 
	  printchar ( 61 ) ; 
	  printfilename ( mem [ p + 1 ] .hh .v.RH , mem [ p + 2 ] .hh .v.LH , 
	  mem [ p + 2 ] .hh .v.RH ) ; 
	} 
	break ; 
      case 1 : 
	{
	  printwritewhatsit ( 590 , p ) ; 
	  printmark ( mem [ p + 1 ] .hh .v.RH ) ; 
	} 
	break ; 
      case 2 : 
	printwritewhatsit ( 1277 , p ) ; 
	break ; 
      case 3 : 
	{
	  printesc ( 1278 ) ; 
	  printmark ( mem [ p + 1 ] .hh .v.RH ) ; 
	} 
	break ; 
      case 4 : 
	{
	  printesc ( 1280 ) ; 
	  printint ( mem [ p + 1 ] .hh .v.RH ) ; 
	  print ( 362 ) ; 
	  printint ( mem [ p + 1 ] .hh.b0 ) ; 
	  printchar ( 44 ) ; 
	  printint ( mem [ p + 1 ] .hh.b1 ) ; 
	  printchar ( 41 ) ; 
	} 
	break ; 
	default: 
	print ( 1283 ) ; 
	break ; 
      } 
      break ; 
    case 10 : 
      if ( mem [ p ] .hh.b1 >= 100 ) 
      {
	printesc ( 335 ) ; 
	if ( mem [ p ] .hh.b1 == 101 ) 
	printchar ( 99 ) ; 
	else if ( mem [ p ] .hh.b1 == 102 ) 
	printchar ( 120 ) ; 
	print ( 336 ) ; 
	printspec ( mem [ p + 1 ] .hh .v.LH , 0 ) ; 
	{
	  {
	    strpool [ poolptr ] = 46 ; 
	    incr ( poolptr ) ; 
	  } 
	  shownodelist ( mem [ p + 1 ] .hh .v.RH ) ; 
	  decr ( poolptr ) ; 
	} 
      } 
      else {
	  
	printesc ( 331 ) ; 
	if ( mem [ p ] .hh.b1 != 0 ) 
	{
	  printchar ( 40 ) ; 
	  if ( mem [ p ] .hh.b1 < 98 ) 
	  printskipparam ( mem [ p ] .hh.b1 - 1 ) ; 
	  else if ( mem [ p ] .hh.b1 == 98 ) 
	  printesc ( 332 ) ; 
	  else printesc ( 333 ) ; 
	  printchar ( 41 ) ; 
	} 
	if ( mem [ p ] .hh.b1 != 98 ) 
	{
	  printchar ( 32 ) ; 
	  if ( mem [ p ] .hh.b1 < 98 ) 
	  printspec ( mem [ p + 1 ] .hh .v.LH , 0 ) ; 
	  else printspec ( mem [ p + 1 ] .hh .v.LH , 334 ) ; 
	} 
      } 
      break ; 
    case 11 : 
      if ( mem [ p ] .hh.b1 != 99 ) 
      {
	printesc ( 337 ) ; 
	if ( mem [ p ] .hh.b1 != 0 ) 
	printchar ( 32 ) ; 
	printscaled ( mem [ p + 1 ] .cint ) ; 
	if ( mem [ p ] .hh.b1 == 2 ) 
	print ( 338 ) ; 
      } 
      else {
	  
	printesc ( 339 ) ; 
	printscaled ( mem [ p + 1 ] .cint ) ; 
	print ( 334 ) ; 
      } 
      break ; 
    case 9 : 
      {
	printesc ( 340 ) ; 
	if ( mem [ p ] .hh.b1 == 0 ) 
	print ( 341 ) ; 
	else print ( 342 ) ; 
	if ( mem [ p + 1 ] .cint != 0 ) 
	{
	  print ( 343 ) ; 
	  printscaled ( mem [ p + 1 ] .cint ) ; 
	} 
      } 
      break ; 
    case 6 : 
      {
	printfontandchar ( p + 1 ) ; 
	print ( 344 ) ; 
	if ( mem [ p ] .hh.b1 > 1 ) 
	printchar ( 124 ) ; 
	fontinshortdisplay = mem [ p + 1 ] .hh.b0 ; 
	shortdisplay ( mem [ p + 1 ] .hh .v.RH ) ; 
	if ( odd ( mem [ p ] .hh.b1 ) ) 
	printchar ( 124 ) ; 
	printchar ( 41 ) ; 
      } 
      break ; 
    case 12 : 
      {
	printesc ( 345 ) ; 
	printint ( mem [ p + 1 ] .cint ) ; 
      } 
      break ; 
    case 7 : 
      {
	printesc ( 346 ) ; 
	if ( mem [ p ] .hh.b1 > 0 ) 
	{
	  print ( 347 ) ; 
	  printint ( mem [ p ] .hh.b1 ) ; 
	} 
	{
	  {
	    strpool [ poolptr ] = 46 ; 
	    incr ( poolptr ) ; 
	  } 
	  shownodelist ( mem [ p + 1 ] .hh .v.LH ) ; 
	  decr ( poolptr ) ; 
	} 
	{
	  strpool [ poolptr ] = 124 ; 
	  incr ( poolptr ) ; 
	} 
	shownodelist ( mem [ p + 1 ] .hh .v.RH ) ; 
	decr ( poolptr ) ; 
      } 
      break ; 
    case 4 : 
      {
	printesc ( 348 ) ; 
	printmark ( mem [ p + 1 ] .cint ) ; 
      } 
      break ; 
    case 5 : 
      {
	printesc ( 349 ) ; 
	{
	  {
	    strpool [ poolptr ] = 46 ; 
	    incr ( poolptr ) ; 
	  } 
	  shownodelist ( mem [ p + 1 ] .cint ) ; 
	  decr ( poolptr ) ; 
	} 
      } 
      break ; 
    case 14 : 
      printstyle ( mem [ p ] .hh.b1 ) ; 
      break ; 
    case 15 : 
      {
	printesc ( 521 ) ; 
	{
	  strpool [ poolptr ] = 68 ; 
	  incr ( poolptr ) ; 
	} 
	shownodelist ( mem [ p + 1 ] .hh .v.LH ) ; 
	decr ( poolptr ) ; 
	{
	  strpool [ poolptr ] = 84 ; 
	  incr ( poolptr ) ; 
	} 
	shownodelist ( mem [ p + 1 ] .hh .v.RH ) ; 
	decr ( poolptr ) ; 
	{
	  strpool [ poolptr ] = 83 ; 
	  incr ( poolptr ) ; 
	} 
	shownodelist ( mem [ p + 2 ] .hh .v.LH ) ; 
	decr ( poolptr ) ; 
	{
	  strpool [ poolptr ] = 115 ; 
	  incr ( poolptr ) ; 
	} 
	shownodelist ( mem [ p + 2 ] .hh .v.RH ) ; 
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
	switch ( mem [ p ] .hh.b0 ) 
	{case 16 : 
	  printesc ( 859 ) ; 
	  break ; 
	case 17 : 
	  printesc ( 860 ) ; 
	  break ; 
	case 18 : 
	  printesc ( 861 ) ; 
	  break ; 
	case 19 : 
	  printesc ( 862 ) ; 
	  break ; 
	case 20 : 
	  printesc ( 863 ) ; 
	  break ; 
	case 21 : 
	  printesc ( 864 ) ; 
	  break ; 
	case 22 : 
	  printesc ( 865 ) ; 
	  break ; 
	case 23 : 
	  printesc ( 866 ) ; 
	  break ; 
	case 27 : 
	  printesc ( 867 ) ; 
	  break ; 
	case 26 : 
	  printesc ( 868 ) ; 
	  break ; 
	case 29 : 
	  printesc ( 535 ) ; 
	  break ; 
	case 24 : 
	  {
	    printesc ( 529 ) ; 
	    printdelimiter ( p + 4 ) ; 
	  } 
	  break ; 
	case 28 : 
	  {
	    printesc ( 504 ) ; 
	    printfamandchar ( p + 4 ) ; 
	  } 
	  break ; 
	case 30 : 
	  {
	    printesc ( 869 ) ; 
	    printdelimiter ( p + 1 ) ; 
	  } 
	  break ; 
	case 31 : 
	  {
	    printesc ( 870 ) ; 
	    printdelimiter ( p + 1 ) ; 
	  } 
	  break ; 
	} 
	if ( mem [ p ] .hh.b1 != 0 ) 
	if ( mem [ p ] .hh.b1 == 1 ) 
	printesc ( 871 ) ; 
	else printesc ( 872 ) ; 
	if ( mem [ p ] .hh.b0 < 30 ) 
	printsubsidiarydata ( p + 1 , 46 ) ; 
	printsubsidiarydata ( p + 2 , 94 ) ; 
	printsubsidiarydata ( p + 3 , 95 ) ; 
      } 
      break ; 
    case 25 : 
      {
	printesc ( 873 ) ; 
	if ( mem [ p + 1 ] .cint == 1073741824L ) 
	print ( 874 ) ; 
	else printscaled ( mem [ p + 1 ] .cint ) ; 
	if ( ( mem [ p + 4 ] .qqqq .b0 != 0 ) || ( mem [ p + 4 ] .qqqq .b1 != 
	0 ) || ( mem [ p + 4 ] .qqqq .b2 != 0 ) || ( mem [ p + 4 ] .qqqq .b3 
	!= 0 ) ) 
	{
	  print ( 875 ) ; 
	  printdelimiter ( p + 4 ) ; 
	} 
	if ( ( mem [ p + 5 ] .qqqq .b0 != 0 ) || ( mem [ p + 5 ] .qqqq .b1 != 
	0 ) || ( mem [ p + 5 ] .qqqq .b2 != 0 ) || ( mem [ p + 5 ] .qqqq .b3 
	!= 0 ) ) 
	{
	  print ( 876 ) ; 
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
    p = mem [ p ] .hh .v.RH ; 
  } 
} 
