#define EXTERN extern
#include "mpd.h"

void 
#ifdef HAVE_PROTOTYPES
zpathintersection ( halfword h , halfword hh ) 
#else
zpathintersection ( h , hh ) 
  halfword h ;
  halfword hh ;
#endif
{
  /* 10 */ halfword p, pp  ;
  integer n, nn  ;
  if ( mem [h ].hhfield .b1 == 0 ) 
  {
    mem [h + 5 ].cint = mem [h + 1 ].cint ;
    mem [h + 3 ].cint = mem [h + 1 ].cint ;
    mem [h + 6 ].cint = mem [h + 2 ].cint ;
    mem [h + 4 ].cint = mem [h + 2 ].cint ;
    mem [h ].hhfield .b1 = 1 ;
  } 
  if ( mem [hh ].hhfield .b1 == 0 ) 
  {
    mem [hh + 5 ].cint = mem [hh + 1 ].cint ;
    mem [hh + 3 ].cint = mem [hh + 1 ].cint ;
    mem [hh + 6 ].cint = mem [hh + 2 ].cint ;
    mem [hh + 4 ].cint = mem [hh + 2 ].cint ;
    mem [hh ].hhfield .b1 = 1 ;
  } 
  tolstep = 0 ;
  do {
      n = -65536L ;
    p = h ;
    do {
	if ( mem [p ].hhfield .b1 != 0 ) 
      {
	nn = -65536L ;
	pp = hh ;
	do {
	    if ( mem [pp ].hhfield .b1 != 0 ) 
	  {
	    cubicintersection ( p , pp ) ;
	    if ( curt > 0 ) 
	    {
	      curt = curt + n ;
	      curtt = curtt + nn ;
	      goto lab10 ;
	    } 
	  } 
	  nn = nn + 65536L ;
	  pp = mem [pp ].hhfield .v.RH ;
	} while ( ! ( pp == hh ) ) ;
      } 
      n = n + 65536L ;
      p = mem [p ].hhfield .v.RH ;
    } while ( ! ( p == h ) ) ;
    tolstep = tolstep + 3 ;
  } while ( ! ( tolstep > 3 ) ) ;
  curt = -65536L ;
  curtt = -65536L ;
  lab10: ;
} 
fraction 
#ifdef HAVE_PROTOTYPES
zmaxcoef ( halfword p ) 
#else
zmaxcoef ( p ) 
  halfword p ;
#endif
{
  register fraction Result; fraction x  ;
  x = 0 ;
  while ( mem [p ].hhfield .lhfield != 0 ) {
      
    if ( abs ( mem [p + 1 ].cint ) > x ) 
    x = abs ( mem [p + 1 ].cint ) ;
    p = mem [p ].hhfield .v.RH ;
  } 
  Result = x ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zpplusq ( halfword p , halfword q , smallnumber t ) 
#else
zpplusq ( p , q , t ) 
  halfword p ;
  halfword q ;
  smallnumber t ;
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
      
    v = mem [p + 1 ].cint + mem [q + 1 ].cint ;
    mem [p + 1 ].cint = v ;
    s = p ;
    p = mem [p ].hhfield .v.RH ;
    pp = mem [p ].hhfield .lhfield ;
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
    q = mem [q ].hhfield .v.RH ;
    qq = mem [q ].hhfield .lhfield ;
  } 
  else if ( mem [pp + 1 ].cint < mem [qq + 1 ].cint ) 
  {
    s = getnode ( 2 ) ;
    mem [s ].hhfield .lhfield = qq ;
    mem [s + 1 ].cint = mem [q + 1 ].cint ;
    q = mem [q ].hhfield .v.RH ;
    qq = mem [q ].hhfield .lhfield ;
    mem [r ].hhfield .v.RH = s ;
    r = s ;
  } 
  else {
      
    mem [r ].hhfield .v.RH = p ;
    r = p ;
    p = mem [p ].hhfield .v.RH ;
    pp = mem [p ].hhfield .lhfield ;
  } 
  lab30: mem [p + 1 ].cint = slowadd ( mem [p + 1 ].cint , mem [q + 1 ]
  .cint ) ;
  mem [r ].hhfield .v.RH = p ;
  depfinal = p ;
  Result = mem [memtop - 1 ].hhfield .v.RH ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zptimesv ( halfword p , integer v , smallnumber t0 , smallnumber t1 , boolean 
visscaled ) 
#else
zptimesv ( p , v , t0 , t1 , visscaled ) 
  halfword p ;
  integer v ;
  smallnumber t0 ;
  smallnumber t1 ;
  boolean visscaled ;
#endif
{
  register halfword Result; halfword r, s  ;
  integer w  ;
  integer threshold  ;
  boolean scalingdown  ;
  if ( t0 != t1 ) 
  scalingdown = true ;
  else scalingdown = ! visscaled ;
  if ( t1 == 17 ) 
  threshold = 1342 ;
  else threshold = 4 ;
  r = memtop - 1 ;
  while ( mem [p ].hhfield .lhfield != 0 ) {
      
    if ( scalingdown ) 
    w = takefraction ( v , mem [p + 1 ].cint ) ;
    else w = takescaled ( v , mem [p + 1 ].cint ) ;
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
  if ( visscaled ) 
  mem [p + 1 ].cint = takescaled ( mem [p + 1 ].cint , v ) ;
  else mem [p + 1 ].cint = takefraction ( mem [p + 1 ].cint , v ) ;
  Result = mem [memtop - 1 ].hhfield .v.RH ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zpwithxbecomingq ( halfword p , halfword x , halfword q , smallnumber t ) 
#else
zpwithxbecomingq ( p , x , q , t ) 
  halfword p ;
  halfword x ;
  halfword q ;
  smallnumber t ;
#endif
{
  register halfword Result; halfword r, s  ;
  integer v  ;
  integer sx  ;
  s = p ;
  r = memtop - 1 ;
  sx = mem [x + 1 ].cint ;
  while ( mem [mem [s ].hhfield .lhfield + 1 ].cint > sx ) {
      
    r = s ;
    s = mem [s ].hhfield .v.RH ;
  } 
  if ( mem [s ].hhfield .lhfield != x ) 
  Result = p ;
  else {
      
    mem [memtop - 1 ].hhfield .v.RH = p ;
    mem [r ].hhfield .v.RH = mem [s ].hhfield .v.RH ;
    v = mem [s + 1 ].cint ;
    freenode ( s , 2 ) ;
    Result = pplusfq ( mem [memtop - 1 ].hhfield .v.RH , v , q , t , 17 ) ;
  } 
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
znewdep ( halfword q , halfword p ) 
#else
znewdep ( q , p ) 
  halfword q ;
  halfword p ;
#endif
{
  halfword r  ;
  mem [q + 1 ].hhfield .v.RH = p ;
  mem [q + 1 ].hhfield .lhfield = 5 ;
  r = mem [5 ].hhfield .v.RH ;
  mem [depfinal ].hhfield .v.RH = r ;
  mem [r + 1 ].hhfield .lhfield = depfinal ;
  mem [5 ].hhfield .v.RH = q ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zconstdependency ( scaled v ) 
#else
zconstdependency ( v ) 
  scaled v ;
#endif
{
  register halfword Result; depfinal = getnode ( 2 ) ;
  mem [depfinal + 1 ].cint = v ;
  mem [depfinal ].hhfield .lhfield = 0 ;
  Result = depfinal ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zsingledependency ( halfword p ) 
#else
zsingledependency ( p ) 
  halfword p ;
#endif
{
  register halfword Result; halfword q  ;
  integer m  ;
  m = mem [p + 1 ].cint % 64 ;
  if ( m > 28 ) 
  Result = constdependency ( 0 ) ;
  else {
      
    q = getnode ( 2 ) ;
    mem [q + 1 ].cint = twotothe [28 - m ];
    mem [q ].hhfield .lhfield = p ;
    mem [q ].hhfield .v.RH = constdependency ( 0 ) ;
    Result = q ;
  } 
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zcopydeplist ( halfword p ) 
#else
zcopydeplist ( p ) 
  halfword p ;
#endif
{
  /* 30 */ register halfword Result; halfword q  ;
  q = getnode ( 2 ) ;
  depfinal = q ;
  while ( true ) {
      
    mem [depfinal ].hhfield .lhfield = mem [p ].hhfield .lhfield ;
    mem [depfinal + 1 ].cint = mem [p + 1 ].cint ;
    if ( mem [depfinal ].hhfield .lhfield == 0 ) 
    goto lab30 ;
    mem [depfinal ].hhfield .v.RH = getnode ( 2 ) ;
    depfinal = mem [depfinal ].hhfield .v.RH ;
    p = mem [p ].hhfield .v.RH ;
  } 
  lab30: Result = q ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zlineareq ( halfword p , smallnumber t ) 
#else
zlineareq ( p , t ) 
  halfword p ;
  smallnumber t ;
#endif
{
  halfword q, r, s  ;
  halfword x  ;
  integer n  ;
  integer v  ;
  halfword prevr  ;
  halfword finalnode  ;
  integer w  ;
  q = p ;
  r = mem [p ].hhfield .v.RH ;
  v = mem [q + 1 ].cint ;
  while ( mem [r ].hhfield .lhfield != 0 ) {
      
    if ( abs ( mem [r + 1 ].cint ) > abs ( v ) ) 
    {
      q = r ;
      v = mem [r + 1 ].cint ;
    } 
    r = mem [r ].hhfield .v.RH ;
  } 
  x = mem [q ].hhfield .lhfield ;
  n = mem [x + 1 ].cint % 64 ;
  s = memtop - 1 ;
  mem [s ].hhfield .v.RH = p ;
  r = p ;
  do {
      if ( r == q ) 
    {
      mem [s ].hhfield .v.RH = mem [r ].hhfield .v.RH ;
      freenode ( r , 2 ) ;
    } 
    else {
	
      w = makefraction ( mem [r + 1 ].cint , v ) ;
      if ( abs ( w ) <= 1342 ) 
      {
	mem [s ].hhfield .v.RH = mem [r ].hhfield .v.RH ;
	freenode ( r , 2 ) ;
      } 
      else {
	  
	mem [r + 1 ].cint = - (integer) w ;
	s = r ;
      } 
    } 
    r = mem [s ].hhfield .v.RH ;
  } while ( ! ( mem [r ].hhfield .lhfield == 0 ) ) ;
  if ( t == 18 ) 
  mem [r + 1 ].cint = - (integer) makescaled ( mem [r + 1 ].cint , v ) ;
  else if ( v != -268435456L ) 
  mem [r + 1 ].cint = - (integer) makefraction ( mem [r + 1 ].cint , v ) ;
  finalnode = r ;
  p = mem [memtop - 1 ].hhfield .v.RH ;
  if ( internal [2 ]> 0 ) 
  if ( interesting ( x ) ) 
  {
    begindiagnostic () ;
    printnl ( 608 ) ;
    printvariablename ( x ) ;
    w = n ;
    while ( w > 0 ) {
	
      print ( 601 ) ;
      w = w - 2 ;
    } 
    printchar ( 61 ) ;
    printdependency ( p , 17 ) ;
    enddiagnostic ( false ) ;
  } 
  prevr = 5 ;
  r = mem [5 ].hhfield .v.RH ;
  while ( r != 5 ) {
      
    s = mem [r + 1 ].hhfield .v.RH ;
    q = pwithxbecomingq ( s , x , p , mem [r ].hhfield .b0 ) ;
    if ( mem [q ].hhfield .lhfield == 0 ) 
    makeknown ( r , q ) ;
    else {
	
      mem [r + 1 ].hhfield .v.RH = q ;
      do {
	  q = mem [q ].hhfield .v.RH ;
      } while ( ! ( mem [q ].hhfield .lhfield == 0 ) ) ;
      prevr = q ;
    } 
    r = mem [prevr ].hhfield .v.RH ;
  } 
  if ( n > 0 ) 
  {
    s = memtop - 1 ;
    mem [memtop - 1 ].hhfield .v.RH = p ;
    r = p ;
    do {
	if ( n > 30 ) 
      w = 0 ;
      else w = mem [r + 1 ].cint / twotothe [n ];
      if ( ( abs ( w ) <= 1342 ) && ( mem [r ].hhfield .lhfield != 0 ) ) 
      {
	mem [s ].hhfield .v.RH = mem [r ].hhfield .v.RH ;
	freenode ( r , 2 ) ;
      } 
      else {
	  
	mem [r + 1 ].cint = w ;
	s = r ;
      } 
      r = mem [s ].hhfield .v.RH ;
    } while ( ! ( mem [s ].hhfield .lhfield == 0 ) ) ;
    p = mem [memtop - 1 ].hhfield .v.RH ;
  } 
  if ( mem [p ].hhfield .lhfield == 0 ) 
  {
    mem [x ].hhfield .b0 = 16 ;
    mem [x + 1 ].cint = mem [p + 1 ].cint ;
    if ( abs ( mem [x + 1 ].cint ) >= 268435456L ) 
    valtoobig ( mem [x + 1 ].cint ) ;
    freenode ( p , 2 ) ;
    if ( curexp == x ) 
    if ( curtype == 19 ) 
    {
      curexp = mem [x + 1 ].cint ;
      curtype = 16 ;
      freenode ( x , 2 ) ;
    } 
  } 
  else {
      
    mem [x ].hhfield .b0 = 17 ;
    depfinal = finalnode ;
    newdep ( x , p ) ;
    if ( curexp == x ) 
    if ( curtype == 19 ) 
    curtype = 17 ;
  } 
  if ( fixneeded ) 
  fixdependencies () ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
znewringentry ( halfword p ) 
#else
znewringentry ( p ) 
  halfword p ;
#endif
{
  register halfword Result; halfword q  ;
  q = getnode ( 2 ) ;
  mem [q ].hhfield .b1 = 14 ;
  mem [q ].hhfield .b0 = mem [p ].hhfield .b0 ;
  if ( mem [p + 1 ].cint == 0 ) 
  mem [q + 1 ].cint = p ;
  else mem [q + 1 ].cint = mem [p + 1 ].cint ;
  mem [p + 1 ].cint = q ;
  Result = q ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
znonlineareq ( integer v , halfword p , boolean flushp ) 
#else
znonlineareq ( v , p , flushp ) 
  integer v ;
  halfword p ;
  boolean flushp ;
#endif
{
  smallnumber t  ;
  halfword q, r  ;
  t = mem [p ].hhfield .b0 - 1 ;
  q = mem [p + 1 ].cint ;
  if ( flushp ) 
  mem [p ].hhfield .b0 = 1 ;
  else p = q ;
  do {
      r = mem [q + 1 ].cint ;
    mem [q ].hhfield .b0 = t ;
    switch ( t ) 
    {case 2 : 
      mem [q + 1 ].cint = v ;
      break ;
    case 4 : 
      {
	mem [q + 1 ].cint = v ;
	{
	  if ( strref [v ]< 127 ) 
	  incr ( strref [v ]) ;
	} 
      } 
      break ;
    case 6 : 
      mem [q + 1 ].cint = makepen ( copypath ( v ) , false ) ;
      break ;
    case 8 : 
      mem [q + 1 ].cint = copypath ( v ) ;
      break ;
    case 10 : 
      {
	mem [q + 1 ].cint = v ;
	incr ( mem [v ].hhfield .lhfield ) ;
      } 
      break ;
    } 
    q = r ;
  } while ( ! ( q == p ) ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zringmerge ( halfword p , halfword q ) 
#else
zringmerge ( p , q ) 
  halfword p ;
  halfword q ;
#endif
{
  /* 10 */ halfword r  ;
  r = mem [p + 1 ].cint ;
  while ( r != p ) {
      
    if ( r == q ) 
    {
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 611 ) ;
	} 
	{
	  helpptr = 2 ;
	  helpline [1 ]= 612 ;
	  helpline [0 ]= 613 ;
	} 
	putgeterror () ;
      } 
      goto lab10 ;
    } 
    r = mem [r + 1 ].cint ;
  } 
  r = mem [p + 1 ].cint ;
  mem [p + 1 ].cint = mem [q + 1 ].cint ;
  mem [q + 1 ].cint = r ;
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zshowcmdmod ( integer c , integer m ) 
#else
zshowcmdmod ( c , m ) 
  integer c ;
  integer m ;
#endif
{
  begindiagnostic () ;
  printnl ( 123 ) ;
  printcmdmod ( c , m ) ;
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
  /* 30 */ char oldsetting  ;
  integer i  ;
  integer l  ;
  integer m  ;
  integer n  ;
  integer p  ;
  integer q  ;
  fileptr = inputptr ;
  inputstack [fileptr ]= curinput ;
  while ( true ) {
      
    curinput = inputstack [fileptr ];
    if ( ( fileptr == inputptr ) || ( curinput .indexfield <= 15 ) || ( 
    curinput .indexfield != 19 ) || ( curinput .locfield != 0 ) ) 
    {
      tally = 0 ;
      oldsetting = selector ;
      if ( ( curinput .indexfield <= 15 ) ) 
      {
	if ( curinput .namefield > 2 ) 
	{
	  printnl ( curinput .namefield ) ;
	  print ( 58 ) ;
	  printint ( trueline () ) ;
	  print ( 58 ) ;
	} 
	else if ( ( curinput .namefield == 0 ) ) 
	if ( fileptr == 0 ) 
	printnl ( 615 ) ;
	else printnl ( 616 ) ;
	else if ( curinput .namefield == 2 ) 
	printnl ( 617 ) ;
	else printnl ( 618 ) ;
	printchar ( 32 ) ;
	{
	  l = tally ;
	  tally = 0 ;
	  selector = 6 ;
	  trickcount = 1000000L ;
	} 
	if ( curinput .limitfield > 0 ) 
	{register integer for_end; i = curinput .startfield ;for_end = 
	curinput .limitfield - 1 ; if ( i <= for_end) do 
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
	{case 16 : 
	  printnl ( 619 ) ;
	  break ;
	case 17 : 
	  {
	    printnl ( 624 ) ;
	    p = paramstack [curinput .limitfield ];
	    if ( p != 0 ) 
	    if ( mem [p ].hhfield .v.RH == 1 ) 
	    printexp ( p , 0 ) ;
	    else showtokenlist ( p , 0 , 20 , tally ) ;
	    print ( 625 ) ;
	  } 
	  break ;
	case 18 : 
	  printnl ( 620 ) ;
	  break ;
	case 19 : 
	  if ( curinput .locfield == 0 ) 
	  printnl ( 621 ) ;
	  else printnl ( 622 ) ;
	  break ;
	case 20 : 
	  printnl ( 623 ) ;
	  break ;
	case 21 : 
	  {
	    println () ;
	    if ( curinput .namefield != 0 ) 
	    print ( hash [curinput .namefield ].v.RH ) ;
	    else {
		
	      p = paramstack [curinput .limitfield ];
	      if ( p == 0 ) 
	      showtokenlist ( paramstack [curinput .limitfield + 1 ], 0 , 20 
	      , tally ) ;
	      else {
		  
		q = p ;
		while ( mem [q ].hhfield .v.RH != 0 ) q = mem [q ].hhfield 
		.v.RH ;
		mem [q ].hhfield .v.RH = paramstack [curinput .limitfield + 
		1 ];
		showtokenlist ( p , 0 , 20 , tally ) ;
		mem [q ].hhfield .v.RH = 0 ;
	      } 
	    } 
	    print ( 513 ) ;
	  } 
	  break ;
	  default: 
	  printnl ( 63 ) ;
	  break ;
	} 
	{
	  l = tally ;
	  tally = 0 ;
	  selector = 6 ;
	  trickcount = 1000000L ;
	} 
	if ( curinput .indexfield != 21 ) 
	showtokenlist ( curinput .startfield , curinput .locfield , 100000L , 
	0 ) ;
	else showmacro ( curinput .startfield , curinput .locfield , 100000L ) 
	;
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
      {register integer for_end; q = p ;for_end = firstcount - 1 ; if ( q <= 
      for_end) do 
	printchar ( trickbuf [q % errorline ]) ;
      while ( q++ < for_end ) ;} 
      println () ;
      {register integer for_end; q = 1 ;for_end = n ; if ( q <= for_end) do 
	printchar ( 32 ) ;
      while ( q++ < for_end ) ;} 
      if ( m + n <= errorline ) 
      p = firstcount + m ;
      else p = firstcount + ( errorline - n - 3 ) ;
      {register integer for_end; q = firstcount ;for_end = p - 1 ; if ( q <= 
      for_end) do 
	printchar ( trickbuf [q % errorline ]) ;
      while ( q++ < for_end ) ;} 
      if ( m + n > errorline ) 
      print ( 275 ) ;
    } 
    if ( ( curinput .indexfield <= 15 ) ) 
    if ( ( curinput .namefield > 2 ) || ( fileptr == 0 ) ) 
    goto lab30 ;
    decr ( fileptr ) ;
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
  {
    if ( inputptr > maxinstack ) 
    {
      maxinstack = inputptr ;
      if ( inputptr == stacksize ) 
      overflow ( 626 , stacksize ) ;
    } 
    inputstack [inputptr ]= curinput ;
    incr ( inputptr ) ;
  } 
  curinput .startfield = p ;
  curinput .indexfield = t ;
  curinput .limitfield = paramptr ;
  curinput .locfield = p ;
} 
void 
#ifdef HAVE_PROTOTYPES
endtokenlist ( void ) 
#else
endtokenlist ( ) 
#endif
{
  /* 30 */ halfword p  ;
  if ( curinput .indexfield >= 19 ) 
  if ( curinput .indexfield <= 20 ) 
  {
    flushtokenlist ( curinput .startfield ) ;
    goto lab30 ;
  } 
  else deletemacref ( curinput .startfield ) ;
  while ( paramptr > curinput .limitfield ) {
      
    decr ( paramptr ) ;
    p = paramstack [paramptr ];
    if ( p != 0 ) 
    if ( mem [p ].hhfield .v.RH == 1 ) 
    {
      recyclevalue ( p ) ;
      freenode ( p , 2 ) ;
    } 
    else flushtokenlist ( p ) ;
  } 
  lab30: {
      
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
zencapsulate ( halfword p ) 
#else
zencapsulate ( p ) 
  halfword p ;
#endif
{
  curexp = getnode ( 2 ) ;
  mem [curexp ].hhfield .b0 = curtype ;
  mem [curexp ].hhfield .b1 = 14 ;
  newdep ( curexp , p ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zinstall ( halfword r , halfword q ) 
#else
zinstall ( r , q ) 
  halfword r ;
  halfword q ;
#endif
{
  halfword p  ;
  if ( mem [q ].hhfield .b0 == 16 ) 
  {
    mem [r + 1 ].cint = mem [q + 1 ].cint ;
    mem [r ].hhfield .b0 = 16 ;
  } 
  else if ( mem [q ].hhfield .b0 == 19 ) 
  {
    p = singledependency ( q ) ;
    if ( p == depfinal ) 
    {
      mem [r ].hhfield .b0 = 16 ;
      mem [r + 1 ].cint = 0 ;
      freenode ( p , 2 ) ;
    } 
    else {
	
      mem [r ].hhfield .b0 = 17 ;
      newdep ( r , p ) ;
    } 
  } 
  else {
      
    mem [r ].hhfield .b0 = mem [q ].hhfield .b0 ;
    newdep ( r , copydeplist ( mem [q + 1 ].hhfield .v.RH ) ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zmakeexpcopy ( halfword p ) 
#else
zmakeexpcopy ( p ) 
  halfword p ;
#endif
{
  /* 20 */ halfword q, r, t  ;
  lab20: curtype = mem [p ].hhfield .b0 ;
  switch ( curtype ) 
  {case 1 : 
  case 2 : 
  case 16 : 
    curexp = mem [p + 1 ].cint ;
    break ;
  case 3 : 
  case 5 : 
  case 7 : 
  case 11 : 
  case 9 : 
    curexp = newringentry ( p ) ;
    break ;
  case 4 : 
    {
      curexp = mem [p + 1 ].cint ;
      {
	if ( strref [curexp ]< 127 ) 
	incr ( strref [curexp ]) ;
      } 
    } 
    break ;
  case 10 : 
    {
      curexp = mem [p + 1 ].cint ;
      incr ( mem [curexp ].hhfield .lhfield ) ;
    } 
    break ;
  case 6 : 
    curexp = makepen ( copypath ( mem [p + 1 ].cint ) , false ) ;
    break ;
  case 8 : 
    curexp = copypath ( mem [p + 1 ].cint ) ;
    break ;
  case 12 : 
  case 13 : 
  case 14 : 
    {
      if ( mem [p + 1 ].cint == 0 ) 
      initbignode ( p ) ;
      t = getnode ( 2 ) ;
      mem [t ].hhfield .b1 = 14 ;
      mem [t ].hhfield .b0 = curtype ;
      initbignode ( t ) ;
      q = mem [p + 1 ].cint + bignodesize [curtype ];
      r = mem [t + 1 ].cint + bignodesize [curtype ];
      do {
	  q = q - 2 ;
	r = r - 2 ;
	install ( r , q ) ;
      } while ( ! ( q == mem [p + 1 ].cint ) ) ;
      curexp = t ;
    } 
    break ;
  case 17 : 
  case 18 : 
    encapsulate ( copydeplist ( mem [p + 1 ].hhfield .v.RH ) ) ;
    break ;
  case 15 : 
    {
      {
	mem [p ].hhfield .b0 = 19 ;
	serialno = serialno + 64 ;
	mem [p + 1 ].cint = serialno ;
      } 
      goto lab20 ;
    } 
    break ;
  case 19 : 
    {
      q = singledependency ( p ) ;
      if ( q == depfinal ) 
      {
	curtype = 16 ;
	curexp = 0 ;
	freenode ( q , 2 ) ;
      } 
      else {
	  
	curtype = 17 ;
	encapsulate ( q ) ;
      } 
    } 
    break ;
    default: 
    confusion ( 851 ) ;
    break ;
  } 
} 
halfword 
#ifdef HAVE_PROTOTYPES
curtok ( void ) 
#else
curtok ( ) 
#endif
{
  register halfword Result; halfword p  ;
  smallnumber savetype  ;
  integer saveexp  ;
  if ( cursym == 0 ) 
  if ( curcmd == 40 ) 
  {
    savetype = curtype ;
    saveexp = curexp ;
    makeexpcopy ( curmod ) ;
    p = stashcurexp () ;
    mem [p ].hhfield .v.RH = 0 ;
    curtype = savetype ;
    curexp = saveexp ;
  } 
  else {
      
    p = getnode ( 2 ) ;
    mem [p + 1 ].cint = curmod ;
    mem [p ].hhfield .b1 = 15 ;
    if ( curcmd == 44 ) 
    mem [p ].hhfield .b0 = 16 ;
    else mem [p ].hhfield .b0 = 4 ;
  } 
  else {
      
    {
      p = avail ;
      if ( p == 0 ) 
      p = getavail () ;
      else {
	  
	avail = mem [p ].hhfield .v.RH ;
	mem [p ].hhfield .v.RH = 0 ;
	;
#ifdef STAT
	incr ( dynused ) ;
#endif /* STAT */
      } 
    } 
    mem [p ].hhfield .lhfield = cursym ;
  } 
  Result = p ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
backinput ( void ) 
#else
backinput ( ) 
#endif
{
  halfword p  ;
  p = curtok () ;
  while ( ( curinput .indexfield > 15 ) && ( curinput .locfield == 0 ) ) 
  endtokenlist () ;
  begintokenlist ( p , 19 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
backerror ( void ) 
#else
backerror ( ) 
#endif
{
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
  OKtointerrupt = false ;
  backinput () ;
  curinput .indexfield = 20 ;
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
  if ( inopen == 15 ) 
  overflow ( 627 , 15 ) ;
  if ( first == bufsize ) 
  overflow ( 256 , bufsize ) ;
  incr ( inopen ) ;
  {
    if ( inputptr > maxinstack ) 
    {
      maxinstack = inputptr ;
      if ( inputptr == stacksize ) 
      overflow ( 626 , stacksize ) ;
    } 
    inputstack [inputptr ]= curinput ;
    incr ( inputptr ) ;
  } 
  curinput .indexfield = inopen ;
  mpxname [curinput .indexfield ]= 1 ;
  curinput .startfield = first ;
  curinput .namefield = 0 ;
} 
void 
#ifdef HAVE_PROTOTYPES
endfilereading ( void ) 
#else
endfilereading ( ) 
#endif
{
  if ( inopen > curinput .indexfield ) 
  if ( ( mpxname [inopen ]== 1 ) || ( curinput .namefield <= 2 ) ) 
  confusion ( 628 ) ;
  else {
      
    aclose ( inputfile [inopen ]) ;
    {
      if ( strref [mpxname [inopen ]]< 127 ) 
      if ( strref [mpxname [inopen ]]> 1 ) 
      decr ( strref [mpxname [inopen ]]) ;
      else flushstring ( mpxname [inopen ]) ;
    } 
    decr ( inopen ) ;
  } 
  first = curinput .startfield ;
  if ( curinput .indexfield != inopen ) 
  confusion ( 628 ) ;
  if ( curinput .namefield > 2 ) 
  {
    aclose ( inputfile [curinput .indexfield ]) ;
    {
      if ( strref [curinput .namefield ]< 127 ) 
      if ( strref [curinput .namefield ]> 1 ) 
      decr ( strref [curinput .namefield ]) ;
      else flushstring ( curinput .namefield ) ;
    } 
    {
      if ( strref [inamestack [curinput .indexfield ]]< 127 ) 
      if ( strref [inamestack [curinput .indexfield ]]> 1 ) 
      decr ( strref [inamestack [curinput .indexfield ]]) ;
      else flushstring ( inamestack [curinput .indexfield ]) ;
    } 
    {
      if ( strref [iareastack [curinput .indexfield ]]< 127 ) 
      if ( strref [iareastack [curinput .indexfield ]]> 1 ) 
      decr ( strref [iareastack [curinput .indexfield ]]) ;
      else flushstring ( iareastack [curinput .indexfield ]) ;
    } 
  } 
  {
    decr ( inputptr ) ;
    curinput = inputstack [inputptr ];
  } 
  decr ( inopen ) ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
beginmpxreading ( void ) 
#else
beginmpxreading ( ) 
#endif
{
  register boolean Result; if ( inopen != curinput .indexfield + 1 ) 
  Result = false ;
  else {
      
    if ( mpxname [inopen ]<= 1 ) 
    confusion ( 629 ) ;
    if ( first == bufsize ) 
    overflow ( 256 , bufsize ) ;
    {
      if ( inputptr > maxinstack ) 
      {
	maxinstack = inputptr ;
	if ( inputptr == stacksize ) 
	overflow ( 626 , stacksize ) ;
      } 
      inputstack [inputptr ]= curinput ;
      incr ( inputptr ) ;
    } 
    curinput .indexfield = inopen ;
    curinput .startfield = first ;
    curinput .namefield = mpxname [inopen ];
    {
      if ( strref [curinput .namefield ]< 127 ) 
      incr ( strref [curinput .namefield ]) ;
    } 
    last = first ;
    curinput .limitfield = last ;
    buffer [curinput .limitfield ]= 37 ;
    first = curinput .limitfield + 1 ;
    curinput .locfield = curinput .startfield ;
    Result = true ;
  } 
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
endmpxreading ( void ) 
#else
endmpxreading ( ) 
#endif
{
  if ( inopen != curinput .indexfield ) 
  confusion ( 629 ) ;
  if ( curinput .locfield < curinput .limitfield ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 630 ) ;
    } 
    {
      helpptr = 4 ;
      helpline [3 ]= 631 ;
      helpline [2 ]= 632 ;
      helpline [1 ]= 633 ;
      helpline [0 ]= 634 ;
    } 
    error () ;
  } 
  first = curinput .startfield ;
  {
    decr ( inputptr ) ;
    curinput = inputstack [inputptr ];
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
clearforerrorprompt ( void ) 
#else
clearforerrorprompt ( ) 
#endif
{
  while ( ( curinput .indexfield <= 15 ) && ( curinput .namefield == 0 ) && 
  ( inputptr > 0 ) && ( curinput .locfield == curinput .limitfield ) ) 
  endfilereading () ;
  println () ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
checkoutervalidity ( void ) 
#else
checkoutervalidity ( ) 
#endif
{
  register boolean Result; halfword p  ;
  if ( scannerstatus == 0 ) 
  Result = true ;
  else if ( scannerstatus == 7 ) 
  if ( cursym != 0 ) 
  Result = true ;
  else {
      
    deletionsallowed = false ;
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 640 ) ;
    } 
    printint ( warninginfo ) ;
    {
      helpptr = 2 ;
      helpline [1 ]= 641 ;
      helpline [0 ]= 642 ;
    } 
    cursym = 9768 ;
    inserror () ;
    deletionsallowed = true ;
    Result = false ;
  } 
  else {
      
    deletionsallowed = false ;
    if ( cursym != 0 ) 
    {
      p = getavail () ;
      mem [p ].hhfield .lhfield = cursym ;
      begintokenlist ( p , 19 ) ;
    } 
    if ( scannerstatus > 1 ) 
    {
      runaway () ;
      if ( cursym == 0 ) 
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 643 ) ;
      } 
      else {
	  
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 644 ) ;
	} 
      } 
      print ( 645 ) ;
      {
	helpptr = 4 ;
	helpline [3 ]= 646 ;
	helpline [2 ]= 647 ;
	helpline [1 ]= 648 ;
	helpline [0 ]= 649 ;
      } 
      switch ( scannerstatus ) 
      {case 2 : 
	{
	  print ( 650 ) ;
	  helpline [3 ]= 651 ;
	  cursym = 9763 ;
	} 
	break ;
      case 3 : 
	{
	  print ( 652 ) ;
	  helpline [3 ]= 653 ;
	  if ( warninginfo == 0 ) 
	  cursym = 9767 ;
	  else {
	      
	    cursym = 9759 ;
	    eqtb [9759 ].v.RH = warninginfo ;
	  } 
	} 
	break ;
      case 4 : 
      case 5 : 
	{
	  print ( 654 ) ;
	  if ( scannerstatus == 5 ) 
	  print ( hash [warninginfo ].v.RH ) ;
	  else printvariablename ( warninginfo ) ;
	  cursym = 9765 ;
	} 
	break ;
      case 6 : 
	{
	  print ( 655 ) ;
	  print ( hash [warninginfo ].v.RH ) ;
	  print ( 656 ) ;
	  helpline [3 ]= 657 ;
	  cursym = 9764 ;
	} 
	break ;
      } 
      inserror () ;
    } 
    else {
	
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 635 ) ;
      } 
      printint ( warninginfo ) ;
      {
	helpptr = 3 ;
	helpline [2 ]= 636 ;
	helpline [1 ]= 637 ;
	helpline [0 ]= 638 ;
      } 
      if ( cursym == 0 ) 
      helpline [2 ]= 639 ;
      cursym = 9766 ;
      inserror () ;
    } 
    deletionsallowed = true ;
    Result = false ;
  } 
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
getnext ( void ) 
#else
getnext ( ) 
#endif
{
  /* 20 10 50 40 25 85 86 87 30 */ integer k  ;
  ASCIIcode c  ;
  ASCIIcode class  ;
  integer n, f  ;
  lab20: cursym = 0 ;
  if ( ( curinput .indexfield <= 15 ) ) 
  {
    lab25: c = buffer [curinput .locfield ];
    incr ( curinput .locfield ) ;
    class = charclass [c ];
    switch ( class ) 
    {case 0 : 
      goto lab85 ;
      break ;
    case 1 : 
      {
	class = charclass [buffer [curinput .locfield ]];
	if ( class > 1 ) 
	goto lab25 ;
	else if ( class < 1 ) 
	{
	  n = 0 ;
	  goto lab86 ;
	} 
      } 
      break ;
    case 2 : 
      goto lab25 ;
      break ;
    case 3 : 
      {
	if ( scannerstatus == 7 ) 
	if ( curinput .locfield < curinput .limitfield ) 
	goto lab25 ;
	if ( curinput .namefield > 2 ) 
	{
	  incr ( linestack [curinput .indexfield ]) ;
	  first = curinput .startfield ;
	  if ( ! forceeof ) 
	  {
	    if ( inputln ( inputfile [curinput .indexfield ], true ) ) 
	    firmuptheline () ;
	    else forceeof = true ;
	  } 
	  if ( forceeof ) 
	  {
	    forceeof = false ;
	    decr ( curinput .locfield ) ;
	    if ( ( mpxname [curinput .indexfield ]> 1 ) ) 
	    {
	      mpxname [curinput .indexfield ]= 0 ;
	      {
		if ( interaction == 3 ) 
		;
		printnl ( 262 ) ;
		print ( 676 ) ;
	      } 
	      {
		helpptr = 4 ;
		helpline [3 ]= 677 ;
		helpline [2 ]= 632 ;
		helpline [1 ]= 678 ;
		helpline [0 ]= 679 ;
	      } 
	      deletionsallowed = false ;
	      error () ;
	      deletionsallowed = true ;
	      cursym = 9769 ;
	      goto lab50 ;
	    } 
	    else {
		
	      printchar ( 41 ) ;
	      decr ( openparens ) ;
	      fflush ( stdout ) ;
	      endfilereading () ;
	      if ( checkoutervalidity () ) 
	      goto lab20 ;
	      else goto lab20 ;
	    } 
	  } 
	  buffer [curinput .limitfield ]= 37 ;
	  first = curinput .limitfield + 1 ;
	  curinput .locfield = curinput .startfield ;
	} 
	else {
	    
	  if ( inputptr > 0 ) 
	  {
	    endfilereading () ;
	    goto lab20 ;
	  } 
	  if ( selector < 9 ) 
	  openlogfile () ;
	  if ( interaction > 1 ) 
	  {
	    if ( curinput .limitfield == curinput .startfield ) 
	    printnl ( 674 ) ;
	    println () ;
	    first = curinput .startfield ;
	    {
	      ;
	      print ( 42 ) ;
	      terminput () ;
	    } 
	    curinput .limitfield = last ;
	    buffer [curinput .limitfield ]= 37 ;
	    first = curinput .limitfield + 1 ;
	    curinput .locfield = curinput .startfield ;
	  } 
	  else fatalerror ( 675 ) ;
	} 
	{
	  if ( interrupt != 0 ) 
	  pauseforinstructions () ;
	} 
	goto lab25 ;
      } 
      break ;
    case 4 : 
      if ( scannerstatus == 7 ) 
      goto lab25 ;
      else {
	  
	if ( buffer [curinput .locfield ]== 34 ) 
	curmod = 284 ;
	else {
	    
	  k = curinput .locfield ;
	  buffer [curinput .limitfield + 1 ]= 34 ;
	  do {
	      incr ( curinput .locfield ) ;
	  } while ( ! ( buffer [curinput .locfield ]== 34 ) ) ;
	  if ( curinput .locfield > curinput .limitfield ) 
	  {
	    curinput .locfield = curinput .limitfield ;
	    {
	      if ( interaction == 3 ) 
	      ;
	      printnl ( 262 ) ;
	      print ( 665 ) ;
	    } 
	    {
	      helpptr = 3 ;
	      helpline [2 ]= 666 ;
	      helpline [1 ]= 667 ;
	      helpline [0 ]= 668 ;
	    } 
	    deletionsallowed = false ;
	    error () ;
	    deletionsallowed = true ;
	    goto lab20 ;
	  } 
	  if ( curinput .locfield == k + 1 ) 
	  curmod = buffer [k ];
	  else {
	      
	    {
	      if ( poolptr + curinput .locfield - k > maxpoolptr ) 
	      if ( poolptr + curinput .locfield - k > poolsize ) 
	      docompaction ( curinput .locfield - k ) ;
	      else maxpoolptr = poolptr + curinput .locfield - k ;
	    } 
	    do {
		{ 
		strpool [poolptr ]= buffer [k ];
		incr ( poolptr ) ;
	      } 
	      incr ( k ) ;
	    } while ( ! ( k == curinput .locfield ) ) ;
	    curmod = makestring () ;
	  } 
	} 
	incr ( curinput .locfield ) ;
	curcmd = 41 ;
	goto lab10 ;
      } 
      break ;
    case 5 : 
    case 6 : 
    case 7 : 
    case 8 : 
      {
	k = curinput .locfield - 1 ;
	goto lab40 ;
      } 
      break ;
    case 20 : 
      if ( scannerstatus == 7 ) 
      goto lab25 ;
      else {
	  
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 662 ) ;
	} 
	{
	  helpptr = 2 ;
	  helpline [1 ]= 663 ;
	  helpline [0 ]= 664 ;
	} 
	deletionsallowed = false ;
	error () ;
	deletionsallowed = true ;
	goto lab20 ;
      } 
      break ;
      default: 
      ;
      break ;
    } 
    k = curinput .locfield - 1 ;
    while ( charclass [buffer [curinput .locfield ]]== class ) incr ( 
    curinput .locfield ) ;
    goto lab40 ;
    lab85: n = c - 48 ;
    while ( charclass [buffer [curinput .locfield ]]== 0 ) {
	
      if ( n < 32768L ) 
      n = 10 * n + buffer [curinput .locfield ]- 48 ;
      incr ( curinput .locfield ) ;
    } 
    if ( buffer [curinput .locfield ]== 46 ) 
    if ( charclass [buffer [curinput .locfield + 1 ]]== 0 ) 
    goto lab30 ;
    f = 0 ;
    goto lab87 ;
    lab30: incr ( curinput .locfield ) ;
    lab86: k = 0 ;
    do {
	if ( k < 17 ) 
      {
	dig [k ]= buffer [curinput .locfield ]- 48 ;
	incr ( k ) ;
      } 
      incr ( curinput .locfield ) ;
    } while ( ! ( charclass [buffer [curinput .locfield ]]!= 0 ) ) ;
    f = rounddecimals ( k ) ;
    if ( f == 65536L ) 
    {
      incr ( n ) ;
      f = 0 ;
    } 
    lab87: if ( n < 32768L ) 
    {
      curmod = n * 65536L + f ;
      if ( curmod >= 268435456L ) 
      if ( ( internal [30 ]> 0 ) && ( scannerstatus != 7 ) ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 672 ) ;
	} 
	printscaled ( curmod ) ;
	printchar ( 41 ) ;
	{
	  helpptr = 3 ;
	  helpline [2 ]= 673 ;
	  helpline [1 ]= 605 ;
	  helpline [0 ]= 606 ;
	} 
	error () ;
      } 
    } 
    else if ( scannerstatus != 7 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 669 ) ;
      } 
      {
	helpptr = 2 ;
	helpline [1 ]= 670 ;
	helpline [0 ]= 671 ;
      } 
      deletionsallowed = false ;
      error () ;
      deletionsallowed = true ;
      curmod = 2147483647L ;
    } 
    curcmd = 44 ;
    goto lab10 ;
    lab40: cursym = idlookup ( k , curinput .locfield - k ) ;
  } 
  else if ( curinput .locfield >= himemmin ) 
  {
    cursym = mem [curinput .locfield ].hhfield .lhfield ;
    curinput .locfield = mem [curinput .locfield ].hhfield .v.RH ;
    if ( cursym >= 9772 ) 
    if ( cursym >= 9922 ) 
    {
      if ( cursym >= 10072 ) 
      cursym = cursym - 150 ;
      begintokenlist ( paramstack [curinput .limitfield + cursym - ( 9922 ) ]
      , 18 ) ;
      goto lab20 ;
    } 
    else {
	
      curcmd = 40 ;
      curmod = paramstack [curinput .limitfield + cursym - ( 9772 ) ];
      cursym = 0 ;
      goto lab10 ;
    } 
  } 
  else if ( curinput .locfield > 0 ) 
  {
    if ( mem [curinput .locfield ].hhfield .b1 == 15 ) 
    {
      curmod = mem [curinput .locfield + 1 ].cint ;
      if ( mem [curinput .locfield ].hhfield .b0 == 16 ) 
      curcmd = 44 ;
      else {
	  
	curcmd = 41 ;
	{
	  if ( strref [curmod ]< 127 ) 
	  incr ( strref [curmod ]) ;
	} 
      } 
    } 
    else {
	
      curmod = curinput .locfield ;
      curcmd = 40 ;
    } 
    curinput .locfield = mem [curinput .locfield ].hhfield .v.RH ;
    goto lab10 ;
  } 
  else {
      
    endtokenlist () ;
    goto lab20 ;
  } 
  lab50: curcmd = eqtb [cursym ].lhfield ;
  curmod = eqtb [cursym ].v.RH ;
  if ( curcmd >= 85 ) 
  if ( checkoutervalidity () ) 
  curcmd = curcmd - 85 ;
  else goto lab20 ;
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
firmuptheline ( void ) 
#else
firmuptheline ( ) 
#endif
{
  integer k  ;
  curinput .limitfield = last ;
  if ( internal [24 ]> 0 ) 
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
      print ( 680 ) ;
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
tnext ( void ) 
#else
tnext ( ) 
#endif
{
  /* 65 50 */ char oldstatus  ;
  integer oldinfo  ;
  while ( curcmd <= 3 ) {
      
    if ( curcmd == 3 ) 
    if ( ! ( curinput .indexfield <= 15 ) || ( mpxname [curinput .indexfield 
    ]== 1 ) ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 690 ) ;
      } 
      {
	helpptr = 2 ;
	helpline [1 ]= 691 ;
	helpline [0 ]= 692 ;
      } 
      error () ;
    } 
    else {
	
      endmpxreading () ;
      goto lab65 ;
    } 
    else if ( curcmd == 1 ) 
    if ( ( curinput .indexfield > 15 ) || ( curinput .namefield <= 2 ) ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 686 ) ;
      } 
      {
	helpptr = 3 ;
	helpline [2 ]= 687 ;
	helpline [1 ]= 688 ;
	helpline [0 ]= 689 ;
      } 
      error () ;
    } 
    else if ( ( mpxname [curinput .indexfield ]> 1 ) ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 683 ) ;
      } 
      {
	helpptr = 4 ;
	helpline [3 ]= 631 ;
	helpline [2 ]= 632 ;
	helpline [1 ]= 684 ;
	helpline [0 ]= 685 ;
      } 
      error () ;
    } 
    else if ( ( curmod != 1 ) && ( mpxname [curinput .indexfield ]!= 0 ) ) 
    {
      if ( ! beginmpxreading () ) 
      startmpxinput () ;
    } 
    else goto lab65 ;
    else {
	
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 693 ) ;
      } 
      {
	helpptr = 1 ;
	helpline [0 ]= 694 ;
      } 
      error () ;
    } 
    goto lab50 ;
    lab65: oldstatus = scannerstatus ;
    oldinfo = warninginfo ;
    scannerstatus = 7 ;
    warninginfo = linestack [curinput .indexfield ];
    do {
	getnext () ;
    } while ( ! ( curcmd == 2 ) ) ;
    scannerstatus = oldstatus ;
    warninginfo = oldinfo ;
    lab50: getnext () ;
  } 
} 
halfword 
#ifdef HAVE_PROTOTYPES
zscantoks ( commandcode terminator , halfword substlist , halfword tailend , 
smallnumber suffixcount ) 
#else
zscantoks ( terminator , substlist , tailend , suffixcount ) 
  commandcode terminator ;
  halfword substlist ;
  halfword tailend ;
  smallnumber suffixcount ;
#endif
{
  /* 30 40 */ register halfword Result; halfword p  ;
  halfword q  ;
  integer balance  ;
  p = memtop - 2 ;
  balance = 1 ;
  mem [memtop - 2 ].hhfield .v.RH = 0 ;
  while ( true ) {
      
    {
      getnext () ;
      if ( curcmd <= 3 ) 
      tnext () ;
    } 
    if ( cursym > 0 ) 
    {
      {
	q = substlist ;
	while ( q != 0 ) {
	    
	  if ( mem [q ].hhfield .lhfield == cursym ) 
	  {
	    cursym = mem [q + 1 ].cint ;
	    curcmd = 10 ;
	    goto lab40 ;
	  } 
	  q = mem [q ].hhfield .v.RH ;
	} 
	lab40: ;
      } 
      if ( curcmd == terminator ) 
      if ( curmod > 0 ) 
      incr ( balance ) ;
      else {
	  
	decr ( balance ) ;
	if ( balance == 0 ) 
	goto lab30 ;
      } 
      else if ( curcmd == 63 ) 
      {
	if ( curmod == 0 ) 
	{
	  getnext () ;
	  if ( curcmd <= 3 ) 
	  tnext () ;
	} 
	else if ( curmod <= suffixcount ) 
	cursym = 9921 + curmod ;
      } 
    } 
    mem [p ].hhfield .v.RH = curtok () ;
    p = mem [p ].hhfield .v.RH ;
  } 
  lab30: mem [p ].hhfield .v.RH = tailend ;
  flushnodelist ( substlist ) ;
  Result = mem [memtop - 2 ].hhfield .v.RH ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
getsymbol ( void ) 
#else
getsymbol ( ) 
#endif
{
  /* 20 */ lab20: {
      
    getnext () ;
    if ( curcmd <= 3 ) 
    tnext () ;
  } 
  if ( ( cursym == 0 ) || ( cursym > 9757 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 706 ) ;
    } 
    {
      helpptr = 3 ;
      helpline [2 ]= 707 ;
      helpline [1 ]= 708 ;
      helpline [0 ]= 709 ;
    } 
    if ( cursym > 0 ) 
    helpline [2 ]= 710 ;
    else if ( curcmd == 41 ) 
    {
      if ( strref [curmod ]< 127 ) 
      if ( strref [curmod ]> 1 ) 
      decr ( strref [curmod ]) ;
      else flushstring ( curmod ) ;
    } 
    cursym = 9757 ;
    inserror () ;
    goto lab20 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
getclearsymbol ( void ) 
#else
getclearsymbol ( ) 
#endif
{
  getsymbol () ;
  clearsymbol ( cursym , false ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
checkequals ( void ) 
#else
checkequals ( ) 
#endif
{
  if ( curcmd != 53 ) 
  if ( curcmd != 76 ) 
  {
    missingerr ( 61 ) ;
    {
      helpptr = 5 ;
      helpline [4 ]= 711 ;
      helpline [3 ]= 712 ;
      helpline [2 ]= 713 ;
      helpline [1 ]= 714 ;
      helpline [0 ]= 715 ;
    } 
    backerror () ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
makeopdef ( void ) 
#else
makeopdef ( ) 
#endif
{
  commandcode m  ;
  halfword p, q, r  ;
  m = curmod ;
  getsymbol () ;
  q = getnode ( 2 ) ;
  mem [q ].hhfield .lhfield = cursym ;
  mem [q + 1 ].cint = 9772 ;
  getclearsymbol () ;
  warninginfo = cursym ;
  getsymbol () ;
  p = getnode ( 2 ) ;
  mem [p ].hhfield .lhfield = cursym ;
  mem [p + 1 ].cint = 9773 ;
  mem [p ].hhfield .v.RH = q ;
  {
    getnext () ;
    if ( curcmd <= 3 ) 
    tnext () ;
  } 
  checkequals () ;
  scannerstatus = 5 ;
  q = getavail () ;
  mem [q ].hhfield .lhfield = 0 ;
  r = getavail () ;
  mem [q ].hhfield .v.RH = r ;
  mem [r ].hhfield .lhfield = 0 ;
  mem [r ].hhfield .v.RH = scantoks ( 18 , p , 0 , 0 ) ;
  scannerstatus = 0 ;
  eqtb [warninginfo ].lhfield = m ;
  eqtb [warninginfo ].v.RH = q ;
  getxnext () ;
} 
void 
#ifdef HAVE_PROTOTYPES
zcheckdelimiter ( halfword ldelim , halfword rdelim ) 
#else
zcheckdelimiter ( ldelim , rdelim ) 
  halfword ldelim ;
  halfword rdelim ;
#endif
{
  /* 10 */ if ( curcmd == 64 ) 
  if ( curmod == ldelim ) 
  goto lab10 ;
  if ( cursym != rdelim ) 
  {
    missingerr ( hash [rdelim ].v.RH ) ;
    {
      helpptr = 2 ;
      helpline [1 ]= 970 ;
      helpline [0 ]= 971 ;
    } 
    backerror () ;
  } 
  else {
      
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 972 ) ;
    } 
    print ( hash [rdelim ].v.RH ) ;
    print ( 973 ) ;
    {
      helpptr = 3 ;
      helpline [2 ]= 974 ;
      helpline [1 ]= 975 ;
      helpline [0 ]= 976 ;
    } 
    error () ;
  } 
  lab10: ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
scandeclaredvariable ( void ) 
#else
scandeclaredvariable ( ) 
#endif
{
  /* 30 */ register halfword Result; halfword x  ;
  halfword h, t  ;
  halfword l  ;
  getsymbol () ;
  x = cursym ;
  if ( curcmd != 43 ) 
  clearsymbol ( x , false ) ;
  h = getavail () ;
  mem [h ].hhfield .lhfield = x ;
  t = h ;
  while ( true ) {
      
    getxnext () ;
    if ( cursym == 0 ) 
    goto lab30 ;
    if ( curcmd != 43 ) 
    if ( curcmd != 42 ) 
    if ( curcmd == 65 ) 
    {
      l = cursym ;
      getxnext () ;
      if ( curcmd != 66 ) 
      {
	backinput () ;
	cursym = l ;
	curcmd = 65 ;
	goto lab30 ;
      } 
      else cursym = 0 ;
    } 
    else goto lab30 ;
    mem [t ].hhfield .v.RH = getavail () ;
    t = mem [t ].hhfield .v.RH ;
    mem [t ].hhfield .lhfield = cursym ;
  } 
  lab30: if ( eqtb [x ].lhfield != 43 ) 
  clearsymbol ( x , false ) ;
  if ( eqtb [x ].v.RH == 0 ) 
  newroot ( x ) ;
  Result = h ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
scandef ( void ) 
#else
scandef ( ) 
#endif
{
  char m  ;
  char n  ;
  unsigned char k  ;
  char c  ;
  halfword r  ;
  halfword q  ;
  halfword p  ;
  halfword base  ;
  halfword ldelim, rdelim  ;
  m = curmod ;
  c = 0 ;
  mem [memtop - 2 ].hhfield .v.RH = 0 ;
  q = getavail () ;
  mem [q ].hhfield .lhfield = 0 ;
  r = 0 ;
  if ( m == 1 ) 
  {
    getclearsymbol () ;
    warninginfo = cursym ;
    {
      getnext () ;
      if ( curcmd <= 3 ) 
      tnext () ;
    } 
    scannerstatus = 5 ;
    n = 0 ;
    eqtb [warninginfo ].lhfield = 13 ;
    eqtb [warninginfo ].v.RH = q ;
  } 
  else {
      
    p = scandeclaredvariable () ;
    flushvariable ( eqtb [mem [p ].hhfield .lhfield ].v.RH , mem [p ]
    .hhfield .v.RH , true ) ;
    warninginfo = findvariable ( p ) ;
    flushlist ( p ) ;
    if ( warninginfo == 0 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 722 ) ;
      } 
      {
	helpptr = 2 ;
	helpline [1 ]= 723 ;
	helpline [0 ]= 724 ;
      } 
      error () ;
      warninginfo = 22 ;
    } 
    scannerstatus = 4 ;
    n = 2 ;
    if ( curcmd == 63 ) 
    if ( curmod == 3 ) 
    {
      n = 3 ;
      {
	getnext () ;
	if ( curcmd <= 3 ) 
	tnext () ;
      } 
    } 
    mem [warninginfo ].hhfield .b0 = 20 + n ;
    mem [warninginfo + 1 ].cint = q ;
  } 
  k = n ;
  if ( curcmd == 33 ) 
  do {
      ldelim = cursym ;
    rdelim = curmod ;
    {
      getnext () ;
      if ( curcmd <= 3 ) 
      tnext () ;
    } 
    if ( ( curcmd == 58 ) && ( curmod >= 9772 ) ) 
    base = curmod ;
    else {
	
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 725 ) ;
      } 
      {
	helpptr = 1 ;
	helpline [0 ]= 726 ;
      } 
      backerror () ;
      base = 9772 ;
    } 
    do {
	mem [q ].hhfield .v.RH = getavail () ;
      q = mem [q ].hhfield .v.RH ;
      mem [q ].hhfield .lhfield = base + k ;
      getsymbol () ;
      p = getnode ( 2 ) ;
      mem [p + 1 ].cint = base + k ;
      mem [p ].hhfield .lhfield = cursym ;
      if ( k == 150 ) 
      overflow ( 727 , 150 ) ;
      incr ( k ) ;
      mem [p ].hhfield .v.RH = r ;
      r = p ;
      {
	getnext () ;
	if ( curcmd <= 3 ) 
	tnext () ;
      } 
    } while ( ! ( curcmd != 81 ) ) ;
    checkdelimiter ( ldelim , rdelim ) ;
    {
      getnext () ;
      if ( curcmd <= 3 ) 
      tnext () ;
    } 
  } while ( ! ( curcmd != 33 ) ) ;
  if ( curcmd == 58 ) 
  {
    p = getnode ( 2 ) ;
    if ( curmod < 9772 ) 
    {
      c = curmod ;
      mem [p + 1 ].cint = 9772 + k ;
    } 
    else {
	
      mem [p + 1 ].cint = curmod + k ;
      if ( curmod == 9772 ) 
      c = 4 ;
      else if ( curmod == 9922 ) 
      c = 6 ;
      else c = 7 ;
    } 
    if ( k == 150 ) 
    overflow ( 727 , 150 ) ;
    incr ( k ) ;
    getsymbol () ;
    mem [p ].hhfield .lhfield = cursym ;
    mem [p ].hhfield .v.RH = r ;
    r = p ;
    {
      getnext () ;
      if ( curcmd <= 3 ) 
      tnext () ;
    } 
    if ( c == 4 ) 
    if ( curcmd == 70 ) 
    {
      c = 5 ;
      p = getnode ( 2 ) ;
      if ( k == 150 ) 
      overflow ( 727 , 150 ) ;
      mem [p + 1 ].cint = 9772 + k ;
      getsymbol () ;
      mem [p ].hhfield .lhfield = cursym ;
      mem [p ].hhfield .v.RH = r ;
      r = p ;
      {
	getnext () ;
	if ( curcmd <= 3 ) 
	tnext () ;
      } 
    } 
  } 
  checkequals () ;
  p = getavail () ;
  mem [p ].hhfield .lhfield = c ;
  mem [q ].hhfield .v.RH = p ;
  if ( m == 1 ) 
  mem [p ].hhfield .v.RH = scantoks ( 18 , r , 0 , n ) ;
  else {
      
    q = getavail () ;
    mem [q ].hhfield .lhfield = bgloc ;
    mem [p ].hhfield .v.RH = q ;
    p = getavail () ;
    mem [p ].hhfield .lhfield = egloc ;
    mem [q ].hhfield .v.RH = scantoks ( 18 , r , p , n ) ;
  } 
  if ( warninginfo == 22 ) 
  flushtokenlist ( mem [23 ].cint ) ;
  scannerstatus = 0 ;
  getxnext () ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintmacroname ( halfword a , halfword n ) 
#else
zprintmacroname ( a , n ) 
  halfword a ;
  halfword n ;
#endif
{
  halfword p, q  ;
  if ( n != 0 ) 
  print ( hash [n ].v.RH ) ;
  else {
      
    p = mem [a ].hhfield .lhfield ;
    if ( p == 0 ) 
    print ( hash [mem [mem [mem [a ].hhfield .v.RH ].hhfield .lhfield ]
    .hhfield .lhfield ].v.RH ) ;
    else {
	
      q = p ;
      while ( mem [q ].hhfield .v.RH != 0 ) q = mem [q ].hhfield .v.RH ;
      mem [q ].hhfield .v.RH = mem [mem [a ].hhfield .v.RH ].hhfield 
      .lhfield ;
      showtokenlist ( p , 0 , 1000 , 0 ) ;
      mem [q ].hhfield .v.RH = 0 ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zprintarg ( halfword q , integer n , halfword b ) 
#else
zprintarg ( q , n , b ) 
  halfword q ;
  integer n ;
  halfword b ;
#endif
{
  if ( mem [q ].hhfield .v.RH == 1 ) 
  printnl ( 510 ) ;
  else if ( ( b < 10072 ) && ( b != 7 ) ) 
  printnl ( 511 ) ;
  else printnl ( 512 ) ;
  printint ( n ) ;
  print ( 743 ) ;
  if ( mem [q ].hhfield .v.RH == 1 ) 
  printexp ( q , 1 ) ;
  else showtokenlist ( q , 0 , 1000 , 0 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zscantextarg ( halfword ldelim , halfword rdelim ) 
#else
zscantextarg ( ldelim , rdelim ) 
  halfword ldelim ;
  halfword rdelim ;
#endif
{
  /* 30 */ integer balance  ;
  halfword p  ;
  warninginfo = ldelim ;
  scannerstatus = 3 ;
  p = memtop - 2 ;
  balance = 1 ;
  mem [memtop - 2 ].hhfield .v.RH = 0 ;
  while ( true ) {
      
    {
      getnext () ;
      if ( curcmd <= 3 ) 
      tnext () ;
    } 
    if ( ldelim == 0 ) 
    {
      if ( curcmd > 81 ) 
      {
	if ( balance == 1 ) 
	goto lab30 ;
	else if ( curcmd == 83 ) 
	decr ( balance ) ;
      } 
      else if ( curcmd == 34 ) 
      incr ( balance ) ;
    } 
    else {
	
      if ( curcmd == 64 ) 
      {
	if ( curmod == ldelim ) 
	{
	  decr ( balance ) ;
	  if ( balance == 0 ) 
	  goto lab30 ;
	} 
      } 
      else if ( curcmd == 33 ) 
      if ( curmod == rdelim ) 
      incr ( balance ) ;
    } 
    mem [p ].hhfield .v.RH = curtok () ;
    p = mem [p ].hhfield .v.RH ;
  } 
  lab30: curexp = mem [memtop - 2 ].hhfield .v.RH ;
  curtype = 20 ;
  scannerstatus = 0 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zmacrocall ( halfword defref , halfword arglist , halfword macroname ) 
#else
zmacrocall ( defref , arglist , macroname ) 
  halfword defref ;
  halfword arglist ;
  halfword macroname ;
#endif
{
  /* 40 */ halfword r  ;
  halfword p, q  ;
  integer n  ;
  halfword ldelim, rdelim  ;
  halfword tail  ;
  r = mem [defref ].hhfield .v.RH ;
  incr ( mem [defref ].hhfield .lhfield ) ;
  if ( arglist == 0 ) 
  n = 0 ;
  else {
      
    n = 1 ;
    tail = arglist ;
    while ( mem [tail ].hhfield .v.RH != 0 ) {
	
      incr ( n ) ;
      tail = mem [tail ].hhfield .v.RH ;
    } 
  } 
  if ( internal [8 ]> 0 ) 
  {
    begindiagnostic () ;
    println () ;
    printmacroname ( arglist , macroname ) ;
    if ( n == 3 ) 
    print ( 705 ) ;
    showmacro ( defref , 0 , 100000L ) ;
    if ( arglist != 0 ) 
    {
      n = 0 ;
      p = arglist ;
      do {
	  q = mem [p ].hhfield .lhfield ;
	printarg ( q , n , 0 ) ;
	incr ( n ) ;
	p = mem [p ].hhfield .v.RH ;
      } while ( ! ( p == 0 ) ) ;
    } 
    enddiagnostic ( false ) ;
  } 
  curcmd = 82 ;
  while ( mem [r ].hhfield .lhfield >= 9772 ) {
      
    if ( curcmd != 81 ) 
    {
      getxnext () ;
      if ( curcmd != 33 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 749 ) ;
	} 
	printmacroname ( arglist , macroname ) ;
	{
	  helpptr = 3 ;
	  helpline [2 ]= 750 ;
	  helpline [1 ]= 751 ;
	  helpline [0 ]= 752 ;
	} 
	if ( mem [r ].hhfield .lhfield >= 9922 ) 
	{
	  curexp = 0 ;
	  curtype = 20 ;
	} 
	else {
	    
	  curexp = 0 ;
	  curtype = 16 ;
	} 
	backerror () ;
	curcmd = 64 ;
	goto lab40 ;
      } 
      ldelim = cursym ;
      rdelim = curmod ;
    } 
    if ( mem [r ].hhfield .lhfield >= 10072 ) 
    scantextarg ( ldelim , rdelim ) ;
    else {
	
      getxnext () ;
      if ( mem [r ].hhfield .lhfield >= 9922 ) 
      scansuffix () ;
      else scanexpression () ;
    } 
    if ( curcmd != 81 ) 
    if ( ( curcmd != 64 ) || ( curmod != ldelim ) ) 
    if ( mem [mem [r ].hhfield .v.RH ].hhfield .lhfield >= 9772 ) 
    {
      missingerr ( 44 ) ;
      {
	helpptr = 3 ;
	helpline [2 ]= 753 ;
	helpline [1 ]= 754 ;
	helpline [0 ]= 748 ;
      } 
      backerror () ;
      curcmd = 81 ;
    } 
    else {
	
      missingerr ( hash [rdelim ].v.RH ) ;
      {
	helpptr = 2 ;
	helpline [1 ]= 755 ;
	helpline [0 ]= 748 ;
      } 
      backerror () ;
    } 
    lab40: {
	
      p = getavail () ;
      if ( curtype == 20 ) 
      mem [p ].hhfield .lhfield = curexp ;
      else mem [p ].hhfield .lhfield = stashcurexp () ;
      if ( internal [8 ]> 0 ) 
      {
	begindiagnostic () ;
	printarg ( mem [p ].hhfield .lhfield , n , mem [r ].hhfield 
	.lhfield ) ;
	enddiagnostic ( false ) ;
      } 
      if ( arglist == 0 ) 
      arglist = p ;
      else mem [tail ].hhfield .v.RH = p ;
      tail = p ;
      incr ( n ) ;
    } 
    r = mem [r ].hhfield .v.RH ;
  } 
  if ( curcmd == 81 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 744 ) ;
    } 
    printmacroname ( arglist , macroname ) ;
    printchar ( 59 ) ;
    printnl ( 745 ) ;
    print ( hash [rdelim ].v.RH ) ;
    print ( 299 ) ;
    {
      helpptr = 3 ;
      helpline [2 ]= 746 ;
      helpline [1 ]= 747 ;
      helpline [0 ]= 748 ;
    } 
    error () ;
  } 
  if ( mem [r ].hhfield .lhfield != 0 ) 
  {
    if ( mem [r ].hhfield .lhfield < 7 ) 
    {
      getxnext () ;
      if ( mem [r ].hhfield .lhfield != 6 ) 
      if ( ( curcmd == 53 ) || ( curcmd == 76 ) ) 
      getxnext () ;
    } 
    switch ( mem [r ].hhfield .lhfield ) 
    {case 1 : 
      scanprimary () ;
      break ;
    case 2 : 
      scansecondary () ;
      break ;
    case 3 : 
      scantertiary () ;
      break ;
    case 4 : 
      scanexpression () ;
      break ;
    case 5 : 
      {
	scanexpression () ;
	p = getavail () ;
	mem [p ].hhfield .lhfield = stashcurexp () ;
	if ( internal [8 ]> 0 ) 
	{
	  begindiagnostic () ;
	  printarg ( mem [p ].hhfield .lhfield , n , 0 ) ;
	  enddiagnostic ( false ) ;
	} 
	if ( arglist == 0 ) 
	arglist = p ;
	else mem [tail ].hhfield .v.RH = p ;
	tail = p ;
	incr ( n ) ;
	if ( curcmd != 70 ) 
	{
	  missingerr ( 489 ) ;
	  print ( 756 ) ;
	  printmacroname ( arglist , macroname ) ;
	  {
	    helpptr = 1 ;
	    helpline [0 ]= 757 ;
	  } 
	  backerror () ;
	} 
	getxnext () ;
	scanprimary () ;
      } 
      break ;
    case 6 : 
      {
	if ( curcmd != 33 ) 
	ldelim = 0 ;
	else {
	    
	  ldelim = cursym ;
	  rdelim = curmod ;
	  getxnext () ;
	} 
	scansuffix () ;
	if ( ldelim != 0 ) 
	{
	  if ( ( curcmd != 64 ) || ( curmod != ldelim ) ) 
	  {
	    missingerr ( hash [rdelim ].v.RH ) ;
	    {
	      helpptr = 2 ;
	      helpline [1 ]= 755 ;
	      helpline [0 ]= 748 ;
	    } 
	    backerror () ;
	  } 
	  getxnext () ;
	} 
      } 
      break ;
    case 7 : 
      scantextarg ( 0 , 0 ) ;
      break ;
    } 
    backinput () ;
    {
      p = getavail () ;
      if ( curtype == 20 ) 
      mem [p ].hhfield .lhfield = curexp ;
      else mem [p ].hhfield .lhfield = stashcurexp () ;
      if ( internal [8 ]> 0 ) 
      {
	begindiagnostic () ;
	printarg ( mem [p ].hhfield .lhfield , n , mem [r ].hhfield 
	.lhfield ) ;
	enddiagnostic ( false ) ;
      } 
      if ( arglist == 0 ) 
      arglist = p ;
      else mem [tail ].hhfield .v.RH = p ;
      tail = p ;
      incr ( n ) ;
    } 
  } 
  r = mem [r ].hhfield .v.RH ;
  while ( ( curinput .indexfield > 15 ) && ( curinput .locfield == 0 ) ) 
  endtokenlist () ;
  if ( paramptr + n > maxparamstack ) 
  {
    maxparamstack = paramptr + n ;
    if ( maxparamstack > 150 ) 
    overflow ( 727 , 150 ) ;
  } 
  begintokenlist ( defref , 21 ) ;
  curinput .namefield = macroname ;
  curinput .locfield = r ;
  if ( n > 0 ) 
  {
    p = arglist ;
    do {
	paramstack [paramptr ]= mem [p ].hhfield .lhfield ;
      incr ( paramptr ) ;
      p = mem [p ].hhfield .v.RH ;
    } while ( ! ( p == 0 ) ) ;
    flushlist ( arglist ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
expand ( void ) 
#else
expand ( ) 
#endif
{
  halfword p  ;
  integer k  ;
  poolpointer j  ;
  if ( internal [6 ]> 65536L ) 
  if ( curcmd != 13 ) 
  showcmdmod ( curcmd , curmod ) ;
  switch ( curcmd ) 
  {case 4 : 
    conditional () ;
    break ;
  case 5 : 
    if ( curmod > iflimit ) 
    if ( iflimit == 1 ) 
    {
      missingerr ( 58 ) ;
      backinput () ;
      cursym = 9762 ;
      inserror () ;
    } 
    else {
	
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 764 ) ;
      } 
      printcmdmod ( 5 , curmod ) ;
      {
	helpptr = 1 ;
	helpline [0 ]= 765 ;
      } 
      error () ;
    } 
    else {
	
      while ( curmod != 2 ) passtext () ;
      {
	p = condptr ;
	ifline = mem [p + 1 ].cint ;
	curif = mem [p ].hhfield .b1 ;
	iflimit = mem [p ].hhfield .b0 ;
	condptr = mem [p ].hhfield .v.RH ;
	freenode ( p , 2 ) ;
      } 
    } 
    break ;
  case 6 : 
    if ( curmod > 0 ) 
    forceeof = true ;
    else startinput () ;
    break ;
  case 7 : 
    if ( curmod == 0 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 728 ) ;
      } 
      {
	helpptr = 2 ;
	helpline [1 ]= 729 ;
	helpline [0 ]= 730 ;
      } 
      error () ;
    } 
    else beginiteration () ;
    break ;
  case 8 : 
    {
      while ( ( curinput .indexfield > 15 ) && ( curinput .locfield == 0 ) ) 
      endtokenlist () ;
      if ( loopptr == 0 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 732 ) ;
	} 
	{
	  helpptr = 2 ;
	  helpline [1 ]= 733 ;
	  helpline [0 ]= 734 ;
	} 
	error () ;
      } 
      else resumeiteration () ;
    } 
    break ;
  case 9 : 
    {
      getboolean () ;
      if ( internal [6 ]> 65536L ) 
      showcmdmod ( 35 , curexp ) ;
      if ( curexp == 30 ) 
      if ( loopptr == 0 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 735 ) ;
	} 
	{
	  helpptr = 1 ;
	  helpline [0 ]= 736 ;
	} 
	if ( curcmd == 82 ) 
	error () ;
	else backerror () ;
      } 
      else {
	  
	p = 0 ;
	do {
	    if ( ( curinput .indexfield <= 15 ) ) 
	  endfilereading () ;
	  else {
	      
	    if ( curinput .indexfield <= 17 ) 
	    p = curinput .startfield ;
	    endtokenlist () ;
	  } 
	} while ( ! ( p != 0 ) ) ;
	if ( p != mem [loopptr ].hhfield .lhfield ) 
	fatalerror ( 739 ) ;
	stopiteration () ;
      } 
      else if ( curcmd != 82 ) 
      {
	missingerr ( 59 ) ;
	{
	  helpptr = 2 ;
	  helpline [1 ]= 737 ;
	  helpline [0 ]= 738 ;
	} 
	backerror () ;
      } 
    } 
    break ;
  case 10 : 
    ;
    break ;
  case 12 : 
    {
      {
	getnext () ;
	if ( curcmd <= 3 ) 
	tnext () ;
      } 
      p = curtok () ;
      {
	getnext () ;
	if ( curcmd <= 3 ) 
	tnext () ;
      } 
      if ( curcmd < 14 ) 
      expand () ;
      else backinput () ;
      begintokenlist ( p , 19 ) ;
    } 
    break ;
  case 11 : 
    {
      getxnext () ;
      scanprimary () ;
      if ( curtype != 4 ) 
      {
	disperr ( 0 , 740 ) ;
	{
	  helpptr = 2 ;
	  helpline [1 ]= 741 ;
	  helpline [0 ]= 742 ;
	} 
	putgetflusherror ( 0 ) ;
      } 
      else {
	  
	backinput () ;
	if ( ( strstart [nextstr [curexp ]]- strstart [curexp ]) > 0 ) 
	{
	  beginfilereading () ;
	  curinput .namefield = 2 ;
	  k = first + ( strstart [nextstr [curexp ]]- strstart [curexp ]
	  ) ;
	  if ( k >= maxbufstack ) 
	  {
	    if ( k >= bufsize ) 
	    {
	      maxbufstack = bufsize ;
	      overflow ( 256 , bufsize ) ;
	    } 
	    maxbufstack = k + 1 ;
	  } 
	  j = strstart [curexp ];
	  curinput .limitfield = k ;
	  while ( first < curinput .limitfield ) {
	      
	    buffer [first ]= strpool [j ];
	    incr ( j ) ;
	    incr ( first ) ;
	  } 
	  buffer [curinput .limitfield ]= 37 ;
	  first = curinput .limitfield + 1 ;
	  curinput .locfield = curinput .startfield ;
	  flushcurexp ( 0 ) ;
	} 
      } 
    } 
    break ;
  case 13 : 
    macrocall ( curmod , 0 , cursym ) ;
    break ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
getxnext ( void ) 
#else
getxnext ( ) 
#endif
{
  halfword saveexp  ;
  {
    getnext () ;
    if ( curcmd <= 3 ) 
    tnext () ;
  } 
  if ( curcmd < 14 ) 
  {
    saveexp = stashcurexp () ;
    do {
	if ( curcmd == 13 ) 
      macrocall ( curmod , 0 , cursym ) ;
      else expand () ;
      {
	getnext () ;
	if ( curcmd <= 3 ) 
	tnext () ;
      } 
    } while ( ! ( curcmd >= 14 ) ) ;
    unstashcurexp ( saveexp ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zstackargument ( halfword p ) 
#else
zstackargument ( p ) 
  halfword p ;
#endif
{
  if ( paramptr == maxparamstack ) 
  {
    incr ( maxparamstack ) ;
    if ( maxparamstack > 150 ) 
    overflow ( 727 , 150 ) ;
  } 
  paramstack [paramptr ]= p ;
  incr ( paramptr ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
passtext ( void ) 
#else
passtext ( ) 
#endif
{
  /* 30 */ integer l  ;
  scannerstatus = 1 ;
  l = 0 ;
  warninginfo = trueline () ;
  while ( true ) {
      
    {
      getnext () ;
      if ( curcmd <= 3 ) 
      tnext () ;
    } 
    if ( curcmd <= 5 ) 
    if ( curcmd < 5 ) 
    incr ( l ) ;
    else {
	
      if ( l == 0 ) 
      goto lab30 ;
      if ( curmod == 2 ) 
      decr ( l ) ;
    } 
    else if ( curcmd == 41 ) 
    {
      if ( strref [curmod ]< 127 ) 
      if ( strref [curmod ]> 1 ) 
      decr ( strref [curmod ]) ;
      else flushstring ( curmod ) ;
    } 
  } 
  lab30: scannerstatus = 0 ;
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
  /* 10 */ halfword q  ;
  if ( p == condptr ) 
  iflimit = l ;
  else {
      
    q = condptr ;
    while ( true ) {
	
      if ( q == 0 ) 
      confusion ( 758 ) ;
      if ( mem [q ].hhfield .v.RH == p ) 
      {
	mem [q ].hhfield .b0 = l ;
	goto lab10 ;
      } 
      q = mem [q ].hhfield .v.RH ;
    } 
  } 
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
checkcolon ( void ) 
#else
checkcolon ( ) 
#endif
{
  if ( curcmd != 80 ) 
  {
    missingerr ( 58 ) ;
    {
      helpptr = 2 ;
      helpline [1 ]= 761 ;
      helpline [0 ]= 738 ;
    } 
    backerror () ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
conditional ( void ) 
#else
conditional ( ) 
#endif
{
  /* 10 30 21 40 */ halfword savecondptr  ;
  char newiflimit  ;
  halfword p  ;
  {
    p = getnode ( 2 ) ;
    mem [p ].hhfield .v.RH = condptr ;
    mem [p ].hhfield .b0 = iflimit ;
    mem [p ].hhfield .b1 = curif ;
    mem [p + 1 ].cint = ifline ;
    condptr = p ;
    iflimit = 1 ;
    ifline = trueline () ;
    curif = 1 ;
  } 
  savecondptr = condptr ;
  lab21: getboolean () ;
  newiflimit = 4 ;
  if ( internal [6 ]> 65536L ) 
  {
    begindiagnostic () ;
    if ( curexp == 30 ) 
    print ( 762 ) ;
    else print ( 763 ) ;
    enddiagnostic ( false ) ;
  } 
  lab40: checkcolon () ;
  if ( curexp == 30 ) 
  {
    changeiflimit ( newiflimit , savecondptr ) ;
    goto lab10 ;
  } 
  while ( true ) {
      
    passtext () ;
    if ( condptr == savecondptr ) 
    goto lab30 ;
    else if ( curmod == 2 ) 
    {
      p = condptr ;
      ifline = mem [p + 1 ].cint ;
      curif = mem [p ].hhfield .b1 ;
      iflimit = mem [p ].hhfield .b0 ;
      condptr = mem [p ].hhfield .v.RH ;
      freenode ( p , 2 ) ;
    } 
  } 
  lab30: curif = curmod ;
  ifline = trueline () ;
  if ( curmod == 2 ) 
  {
    p = condptr ;
    ifline = mem [p + 1 ].cint ;
    curif = mem [p ].hhfield .b1 ;
    iflimit = mem [p ].hhfield .b0 ;
    condptr = mem [p ].hhfield .v.RH ;
    freenode ( p , 2 ) ;
  } 
  else if ( curmod == 4 ) 
  goto lab21 ;
  else {
      
    curexp = 30 ;
    newiflimit = 2 ;
    getxnext () ;
    goto lab40 ;
  } 
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zbadfor ( strnumber s ) 
#else
zbadfor ( s ) 
  strnumber s ;
#endif
{
  disperr ( 0 , 766 ) ;
  print ( s ) ;
  print ( 306 ) ;
  {
    helpptr = 4 ;
    helpline [3 ]= 767 ;
    helpline [2 ]= 768 ;
    helpline [1 ]= 769 ;
    helpline [0 ]= 308 ;
  } 
  putgetflusherror ( 0 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
beginiteration ( void ) 
#else
beginiteration ( ) 
#endif
{
  /* 22 30 */ halfword m  ;
  halfword n  ;
  halfword s  ;
  halfword p  ;
  halfword q  ;
  halfword pp  ;
  m = curmod ;
  n = cursym ;
  s = getnode ( 2 ) ;
  if ( m == 1 ) 
  {
    mem [s + 1 ].hhfield .lhfield = 1 ;
    p = 0 ;
    getxnext () ;
  } 
  else {
      
    getsymbol () ;
    p = getnode ( 2 ) ;
    mem [p ].hhfield .lhfield = cursym ;
    mem [p + 1 ].cint = m ;
    getxnext () ;
    if ( curcmd == 74 ) 
    {
      getxnext () ;
      scanexpression () ;
      if ( curtype != 10 ) 
      {
	disperr ( 0 , 782 ) ;
	{
	  helpptr = 1 ;
	  helpline [0 ]= 783 ;
	} 
	putgetflusherror ( getnode ( 8 ) ) ;
	initedges ( curexp ) ;
	curtype = 10 ;
      } 
      mem [s + 1 ].hhfield .lhfield = curexp ;
      curtype = 1 ;
      q = mem [curexp + 7 ].hhfield .v.RH ;
      if ( q != 0 ) 
      if ( ( mem [q ].hhfield .b0 >= 4 ) ) 
      if ( skip1component ( q ) == 0 ) 
      q = mem [q ].hhfield .v.RH ;
      mem [s + 1 ].hhfield .v.RH = q ;
    } 
    else {
	
      if ( ( curcmd != 53 ) && ( curcmd != 76 ) ) 
      {
	missingerr ( 61 ) ;
	{
	  helpptr = 3 ;
	  helpline [2 ]= 770 ;
	  helpline [1 ]= 713 ;
	  helpline [0 ]= 771 ;
	} 
	backerror () ;
      } 
      mem [s + 1 ].hhfield .lhfield = 0 ;
      q = s + 1 ;
      mem [q ].hhfield .v.RH = 0 ;
      do {
	  getxnext () ;
	if ( m != 9772 ) 
	scansuffix () ;
	else {
	    
	  if ( curcmd >= 80 ) 
	  if ( curcmd <= 81 ) 
	  goto lab22 ;
	  scanexpression () ;
	  if ( curcmd == 72 ) 
	  if ( q == s + 1 ) 
	  {
	    if ( curtype != 16 ) 
	    badfor ( 777 ) ;
	    pp = getnode ( 4 ) ;
	    mem [pp + 1 ].cint = curexp ;
	    getxnext () ;
	    scanexpression () ;
	    if ( curtype != 16 ) 
	    badfor ( 778 ) ;
	    mem [pp + 2 ].cint = curexp ;
	    if ( curcmd != 73 ) 
	    {
	      missingerr ( 500 ) ;
	      {
		helpptr = 2 ;
		helpline [1 ]= 779 ;
		helpline [0 ]= 780 ;
	      } 
	      backerror () ;
	    } 
	    getxnext () ;
	    scanexpression () ;
	    if ( curtype != 16 ) 
	    badfor ( 781 ) ;
	    mem [pp + 3 ].cint = curexp ;
	    mem [s + 1 ].hhfield .v.RH = pp ;
	    mem [s + 1 ].hhfield .lhfield = 2 ;
	    goto lab30 ;
	  } 
	  curexp = stashcurexp () ;
	} 
	mem [q ].hhfield .v.RH = getavail () ;
	q = mem [q ].hhfield .v.RH ;
	mem [q ].hhfield .lhfield = curexp ;
	curtype = 1 ;
	lab22: ;
      } while ( ! ( curcmd != 81 ) ) ;
      lab30: ;
    } 
  } 
  if ( curcmd != 80 ) 
  {
    missingerr ( 58 ) ;
    {
      helpptr = 3 ;
      helpline [2 ]= 772 ;
      helpline [1 ]= 773 ;
      helpline [0 ]= 774 ;
    } 
    backerror () ;
  } 
  q = getavail () ;
  mem [q ].hhfield .lhfield = 9758 ;
  scannerstatus = 6 ;
  warninginfo = n ;
  mem [s ].hhfield .lhfield = scantoks ( 7 , p , q , 0 ) ;
  scannerstatus = 0 ;
  mem [s ].hhfield .v.RH = loopptr ;
  loopptr = s ;
  resumeiteration () ;
} 
void 
#ifdef HAVE_PROTOTYPES
resumeiteration ( void ) 
#else
resumeiteration ( ) 
#endif
{
  /* 45 10 */ halfword p, q  ;
  p = mem [loopptr + 1 ].hhfield .lhfield ;
  if ( p == 2 ) 
  {
    p = mem [loopptr + 1 ].hhfield .v.RH ;
    curexp = mem [p + 1 ].cint ;
    if ( ( ( mem [p + 2 ].cint > 0 ) && ( curexp > mem [p + 3 ].cint ) ) 
    || ( ( mem [p + 2 ].cint < 0 ) && ( curexp < mem [p + 3 ].cint ) ) ) 
    goto lab45 ;
    curtype = 16 ;
    q = stashcurexp () ;
    mem [p + 1 ].cint = curexp + mem [p + 2 ].cint ;
  } 
  else if ( p == 0 ) 
  {
    p = mem [loopptr + 1 ].hhfield .v.RH ;
    if ( p == 0 ) 
    goto lab45 ;
    mem [loopptr + 1 ].hhfield .v.RH = mem [p ].hhfield .v.RH ;
    q = mem [p ].hhfield .lhfield ;
    {
      mem [p ].hhfield .v.RH = avail ;
      avail = p ;
	;
#ifdef STAT
      decr ( dynused ) ;
#endif /* STAT */
    } 
  } 
  else if ( p == 1 ) 
  {
    begintokenlist ( mem [loopptr ].hhfield .lhfield , 16 ) ;
    goto lab10 ;
  } 
  else {
      
    q = mem [loopptr + 1 ].hhfield .v.RH ;
    if ( q == 0 ) 
    goto lab45 ;
    if ( ! ( mem [q ].hhfield .b0 >= 4 ) ) 
    q = mem [q ].hhfield .v.RH ;
    else if ( ! ( mem [q ].hhfield .b0 >= 6 ) ) 
    q = skip1component ( q ) ;
    else goto lab45 ;
    curexp = copyobjects ( mem [loopptr + 1 ].hhfield .v.RH , q ) ;
    initbbox ( curexp ) ;
    curtype = 10 ;
    mem [loopptr + 1 ].hhfield .v.RH = q ;
    q = stashcurexp () ;
  } 
  begintokenlist ( mem [loopptr ].hhfield .lhfield , 17 ) ;
  stackargument ( q ) ;
  if ( internal [6 ]> 65536L ) 
  {
    begindiagnostic () ;
    printnl ( 776 ) ;
    if ( ( q != 0 ) && ( mem [q ].hhfield .v.RH == 1 ) ) 
    printexp ( q , 1 ) ;
    else showtokenlist ( q , 0 , 50 , 0 ) ;
    printchar ( 125 ) ;
    enddiagnostic ( false ) ;
  } 
  goto lab10 ;
  lab45: stopiteration () ;
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
stopiteration ( void ) 
#else
stopiteration ( ) 
#endif
{
  halfword p, q  ;
  p = mem [loopptr + 1 ].hhfield .lhfield ;
  if ( p == 2 ) 
  freenode ( mem [loopptr + 1 ].hhfield .v.RH , 4 ) ;
  else if ( p == 0 ) 
  {
    q = mem [loopptr + 1 ].hhfield .v.RH ;
    while ( q != 0 ) {
	
      p = mem [q ].hhfield .lhfield ;
      if ( p != 0 ) 
      if ( mem [p ].hhfield .v.RH == 1 ) 
      {
	recyclevalue ( p ) ;
	freenode ( p , 2 ) ;
      } 
      else flushtokenlist ( p ) ;
      p = q ;
      q = mem [q ].hhfield .v.RH ;
      {
	mem [p ].hhfield .v.RH = avail ;
	avail = p ;
	;
#ifdef STAT
	decr ( dynused ) ;
#endif /* STAT */
      } 
    } 
  } 
  else if ( p > 2 ) 
  if ( mem [p ].hhfield .lhfield == 0 ) 
  tossedges ( p ) ;
  else decr ( mem [p ].hhfield .lhfield ) ;
  p = loopptr ;
  loopptr = mem [p ].hhfield .v.RH ;
  flushtokenlist ( mem [p ].hhfield .lhfield ) ;
  freenode ( p , 2 ) ;
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
      c = xord [MPmemdefault [j ]];
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
  {register integer for_end; j = memdefaultlength - 3 ;for_end = 
  memdefaultlength ; if ( j <= for_end) do 
    {
      c = xord [MPmemdefault [j ]];
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
  register strnumber Result; integer k  ;
  if ( stroverflowed ) 
  Result = 63 ;
  else {
      
    {
      if ( poolptr + namelength > maxpoolptr ) 
      if ( poolptr + namelength > poolsize ) 
      docompaction ( namelength ) ;
      else maxpoolptr = poolptr + namelength ;
    } 
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
  register strnumber Result; Result = makenamestring () ;
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
  register strnumber Result; Result = makenamestring () ;
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
  register strnumber Result; Result = makenamestring () ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
scanfilename ( void ) 
#else
scanfilename ( ) 
#endif
{
  /* 30 */ beginname () ;
  while ( ( buffer [curinput .locfield ]== 32 ) || ( buffer [curinput 
  .locfield ]== 9 ) ) incr ( curinput .locfield ) ;
  while ( true ) {
      
    if ( ( buffer [curinput .locfield ]== 59 ) || ( buffer [curinput 
    .locfield ]== 37 ) ) 
    goto lab30 ;
    if ( ! morename ( buffer [curinput .locfield ]) ) 
    goto lab30 ;
    incr ( curinput .locfield ) ;
  } 
  lab30: endname () ;
} 
void 
#ifdef HAVE_PROTOTYPES
zpackjobname ( strnumber s ) 
#else
zpackjobname ( s ) 
  strnumber s ;
#endif
{
  {
    if ( strref [s ]< 127 ) 
    incr ( strref [s ]) ;
  } 
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
  curarea = 284 ;
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
  /* 30 */ integer k  ;
  if ( interaction == 2 ) 
  ;
  if ( s == 785 ) 
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 262 ) ;
    print ( 786 ) ;
  } 
  else {
      
    if ( interaction == 3 ) 
    ;
    printnl ( 262 ) ;
    print ( 787 ) ;
  } 
  printfilename ( curname , curarea , curext ) ;
  print ( 788 ) ;
  if ( e == 284 ) 
  showcontext () ;
  printnl ( 789 ) ;
  print ( s ) ;
  if ( interaction < 2 ) 
  fatalerror ( 790 ) ;
  {
    ;
    print ( 791 ) ;
    terminput () ;
  } 
  {
    beginname () ;
    k = first ;
    while ( ( ( buffer [k ]== 32 ) || ( buffer [k ]== 9 ) ) && ( k < last 
    ) ) incr ( k ) ;
    while ( true ) {
	
      if ( k == last ) 
      goto lab30 ;
      if ( ! morename ( buffer [k ]) ) 
      goto lab30 ;
      incr ( k ) ;
    } 
    lab30: endname () ;
  } 
  if ( curext == 284 ) 
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
  char oldsetting  ;
  integer k  ;
  integer l  ;
  integer m  ;
  char * months  ;
  oldsetting = selector ;
  if ( jobname == 0 ) 
  jobname = 792 ;
  packjobname ( 793 ) ;
  while ( ! aopenout ( logfile ) ) {
      
    selector = 8 ;
    promptfilename ( 795 , 793 ) ;
  } 
  texmflogname = amakenamestring ( logfile ) ;
  selector = 9 ;
  logopened = true ;
  {
    Fputs( logfile ,  "This is MetaPost, Version 0.641" ) ;
    Fputs( logfile ,  versionstring ) ;
    print ( memident ) ;
    print ( 796 ) ;
    printint ( roundunscaled ( internal [15 ]) ) ;
    printchar ( 32 ) ;
    months = " JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC" ;
    m = roundunscaled ( internal [14 ]) ;
    {register integer for_end; k = 3 * m - 2 ;for_end = 3 * m ; if ( k <= 
    for_end) do 
      putc ( months [k ],  logfile );
    while ( k++ < for_end ) ;} 
    printchar ( 32 ) ;
    printint ( roundunscaled ( internal [13 ]) ) ;
    printchar ( 32 ) ;
    m = roundunscaled ( internal [16 ]) ;
    printdd ( m / 60 ) ;
    printchar ( 58 ) ;
    printdd ( m % 60 ) ;
  } 
  inputstack [inputptr ]= curinput ;
  printnl ( 794 ) ;
  l = inputstack [0 ].limitfield - 1 ;
  {register integer for_end; k = 1 ;for_end = l ; if ( k <= for_end) do 
    print ( buffer [k ]) ;
  while ( k++ < for_end ) ;} 
  println () ;
  selector = oldsetting + 2 ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
ztryextension ( strnumber ext ) 
#else
ztryextension ( ext ) 
  strnumber ext ;
#endif
{
  register boolean Result; packfilename ( curname , curarea , curext ) ;
  inamestack [curinput .indexfield ]= curname ;
  iareastack [curinput .indexfield ]= curarea ;
  if ( strvsstr ( ext , 797 ) == 0 ) 
  Result = aopenin ( inputfile [curinput .indexfield ], kpsemfformat ) ;
  else Result = aopenin ( inputfile [curinput .indexfield ], kpsempformat ) 
  ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zcopyoldname ( strnumber s ) 
#else
zcopyoldname ( s ) 
  strnumber s ;
#endif
{
  integer k  ;
  poolpointer j  ;
  k = 0 ;
  if ( oldfilename ) 
  libcfree ( oldfilename ) ;
  oldfilename = xmalloc ( 1 + ( strstart [nextstr [s ]]- strstart [s ]) 
  + 1 ) ;
  {register integer for_end; j = strstart [s ];for_end = strstart [
  nextstr [s ]]- 1 ; if ( j <= for_end) do 
    {
      incr ( k ) ;
      if ( k <= maxint ) 
      oldfilename [k ]= xchr [strpool [j ]];
    } 
  while ( j++ < for_end ) ;} 
  if ( k <= maxint ) 
  oldnamelength = k ;
  else oldnamelength = maxint ;
  oldfilename [oldnamelength + 1 ]= 0 ;
} 
void 
#ifdef HAVE_PROTOTYPES
startmpxinput ( void ) 
#else
startmpxinput ( ) 
#endif
{
  /* 10 45 */ integer k  ;
  packfilename ( inamestack [curinput .indexfield ], iareastack [curinput 
  .indexfield ], 803 ) ;
  copyoldname ( curinput .namefield ) ;
  if ( ! callmakempx ( oldfilename + 1 , (char*) nameoffile + 1 ) ) 
  goto lab45 ;
  beginfilereading () ;
  if ( ! aopenin ( inputfile [curinput .indexfield ], -1 ) ) 
  {
    endfilereading () ;
    goto lab45 ;
  } 
  curinput .namefield = amakenamestring ( inputfile [curinput .indexfield ]) 
  ;
  mpxname [curinput .indexfield ]= curinput .namefield ;
  {
    if ( strref [curinput .namefield ]< 127 ) 
    incr ( strref [curinput .namefield ]) ;
  } 
  {
    linestack [curinput .indexfield ]= 1 ;
    if ( inputln ( inputfile [curinput .indexfield ], false ) ) 
    ;
    firmuptheline () ;
    buffer [curinput .limitfield ]= 37 ;
    first = curinput .limitfield + 1 ;
    curinput .locfield = curinput .startfield ;
  } 
  goto lab10 ;
  lab45: if ( interaction == 3 ) 
  ;
  printnl ( 804 ) ;
  {register integer for_end; k = 1 ;for_end = oldnamelength ; if ( k <= 
  for_end) do 
    print ( xord [oldfilename [k ]]) ;
  while ( k++ < for_end ) ;} 
  printnl ( 804 ) ;
  {register integer for_end; k = 1 ;for_end = namelength ; if ( k <= 
  for_end) do 
    print ( xord [nameoffile [k ]]) ;
  while ( k++ < for_end ) ;} 
  printnl ( 805 ) ;
  {
    helpptr = 4 ;
    helpline [3 ]= 806 ;
    helpline [2 ]= 807 ;
    helpline [1 ]= 808 ;
    helpline [0 ]= 809 ;
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
  lab10: ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
zstartreadinput ( strnumber s , readfindex n ) 
#else
zstartreadinput ( s , n ) 
  strnumber s ;
  readfindex n ;
#endif
{
  /* 10 45 */ register boolean Result; strscanfile ( s ) ;
  packfilename ( curname , curarea , curext ) ;
  beginfilereading () ;
  if ( ! aopenin ( rdfile [n ], -1 ) ) 
  goto lab45 ;
  if ( ! inputln ( rdfile [n ], false ) ) 
  {
    aclose ( rdfile [n ]) ;
    goto lab45 ;
  } 
  rdfname [n ]= s ;
  {
    if ( strref [s ]< 127 ) 
    incr ( strref [s ]) ;
  } 
  Result = true ;
  goto lab10 ;
  lab45: endfilereading () ;
  Result = false ;
  lab10: ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zopenwritefile ( strnumber s , readfindex n ) 
#else
zopenwritefile ( s , n ) 
  strnumber s ;
  readfindex n ;
#endif
{
  
#ifdef AMIGA
  /* 30 */ readfindex n0  ;
#endif /* AMIGA */
  strscanfile ( s ) ;
  packfilename ( curname , curarea , curext ) ;
	;
#ifdef AMIGA
  {register integer for_end; n0 = 0 ;for_end = readfiles - 1 ; if ( n0 <= 
  for_end) do 
    {
      if ( rdfname [n0 ]!= 0 ) 
      {
	if ( strvsstr ( s , rdfname [n0 ]) == 0 ) 
	{
	  aclose ( rdfile [n0 ]) ;
	  {
	    if ( strref [rdfname [n0 ]]< 127 ) 
	    if ( strref [rdfname [n0 ]]> 1 ) 
	    decr ( strref [rdfname [n0 ]]) ;
	    else flushstring ( rdfname [n0 ]) ;
	  } 
	  rdfname [n0 ]= 0 ;
	  if ( n0 == readfiles - 1 ) 
	  readfiles = n0 ;
	  goto lab30 ;
	} 
      } 
    } 
  while ( n0++ < for_end ) ;} 
  lab30: ;
#endif /* AMIGA */
  while ( ! openoutnameok ( (char*) nameoffile + 1 ) || ! aopenout ( wrfile [n ]) ) 
  promptfilename ( 810 , 284 ) ;
  wrfname [n ]= s ;
  {
    if ( strref [s ]< 127 ) 
    incr ( strref [s ]) ;
  } 
  if ( logopened ) 
  {
    oldsetting = selector ;
    if ( ( internal [12 ]<= 0 ) ) 
    selector = 9 ;
    else selector = 10 ;
    printnl ( 502 ) ;
    printint ( n ) ;
    print ( 811 ) ;
    printfilename ( curname , curarea , curext ) ;
    print ( 788 ) ;
    printnl ( 284 ) ;
    println () ;
    selector = oldsetting ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zbadexp ( strnumber s ) 
#else
zbadexp ( s ) 
  strnumber s ;
#endif
{
  char saveflag  ;
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 262 ) ;
    print ( s ) ;
  } 
  print ( 819 ) ;
  printcmdmod ( curcmd , curmod ) ;
  printchar ( 39 ) ;
  {
    helpptr = 4 ;
    helpline [3 ]= 820 ;
    helpline [2 ]= 821 ;
    helpline [1 ]= 822 ;
    helpline [0 ]= 823 ;
  } 
  backinput () ;
  cursym = 0 ;
  curcmd = 44 ;
  curmod = 0 ;
  inserror () ;
  saveflag = varflag ;
  varflag = 0 ;
  getxnext () ;
  varflag = saveflag ;
} 
void 
#ifdef HAVE_PROTOTYPES
zstashin ( halfword p ) 
#else
zstashin ( p ) 
  halfword p ;
#endif
{
  halfword q  ;
  mem [p ].hhfield .b0 = curtype ;
  if ( curtype == 16 ) 
  mem [p + 1 ].cint = curexp ;
  else {
      
    if ( curtype == 19 ) 
    {
      q = singledependency ( curexp ) ;
      if ( q == depfinal ) 
      {
	mem [p ].hhfield .b0 = 16 ;
	mem [p + 1 ].cint = 0 ;
	freenode ( q , 2 ) ;
      } 
      else {
	  
	mem [p ].hhfield .b0 = 17 ;
	newdep ( p , q ) ;
      } 
      recyclevalue ( curexp ) ;
    } 
    else {
	
      mem [p + 1 ]= mem [curexp + 1 ];
      mem [mem [p + 1 ].hhfield .lhfield ].hhfield .v.RH = p ;
    } 
    freenode ( curexp , 2 ) ;
  } 
  curtype = 1 ;
} 
void 
#ifdef HAVE_PROTOTYPES
backexpr ( void ) 
#else
backexpr ( ) 
#endif
{
  halfword p  ;
  p = stashcurexp () ;
  mem [p ].hhfield .v.RH = 0 ;
  begintokenlist ( p , 19 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
badsubscript ( void ) 
#else
badsubscript ( ) 
#endif
{
  disperr ( 0 , 837 ) ;
  {
    helpptr = 3 ;
    helpline [2 ]= 838 ;
    helpline [1 ]= 839 ;
    helpline [0 ]= 840 ;
  } 
  flusherror ( 0 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zobliterated ( halfword q ) 
#else
zobliterated ( q ) 
  halfword q ;
#endif
{
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 262 ) ;
    print ( 841 ) ;
  } 
  showtokenlist ( q , 0 , 1000 , 0 ) ;
  print ( 842 ) ;
  {
    helpptr = 5 ;
    helpline [4 ]= 843 ;
    helpline [3 ]= 844 ;
    helpline [2 ]= 845 ;
    helpline [1 ]= 846 ;
    helpline [0 ]= 847 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zbinarymac ( halfword p , halfword c , halfword n ) 
#else
zbinarymac ( p , c , n ) 
  halfword p ;
  halfword c ;
  halfword n ;
#endif
{
  halfword q, r  ;
  q = getavail () ;
  r = getavail () ;
  mem [q ].hhfield .v.RH = r ;
  mem [q ].hhfield .lhfield = p ;
  mem [r ].hhfield .lhfield = stashcurexp () ;
  macrocall ( c , q , n ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
knownpair ( void ) 
#else
knownpair ( ) 
#endif
{
  halfword p  ;
  if ( curtype != 14 ) 
  {
    disperr ( 0 , 858 ) ;
    {
      helpptr = 5 ;
      helpline [4 ]= 859 ;
      helpline [3 ]= 860 ;
      helpline [2 ]= 861 ;
      helpline [1 ]= 862 ;
      helpline [0 ]= 863 ;
    } 
    putgetflusherror ( 0 ) ;
    curx = 0 ;
    cury = 0 ;
  } 
  else {
      
    p = mem [curexp + 1 ].cint ;
    if ( mem [p ].hhfield .b0 == 16 ) 
    curx = mem [p + 1 ].cint ;
    else {
	
      disperr ( p , 864 ) ;
      {
	helpptr = 5 ;
	helpline [4 ]= 865 ;
	helpline [3 ]= 860 ;
	helpline [2 ]= 861 ;
	helpline [1 ]= 862 ;
	helpline [0 ]= 863 ;
      } 
      putgeterror () ;
      recyclevalue ( p ) ;
      curx = 0 ;
    } 
    if ( mem [p + 2 ].hhfield .b0 == 16 ) 
    cury = mem [p + 3 ].cint ;
    else {
	
      disperr ( p + 2 , 866 ) ;
      {
	helpptr = 5 ;
	helpline [4 ]= 867 ;
	helpline [3 ]= 860 ;
	helpline [2 ]= 861 ;
	helpline [1 ]= 862 ;
	helpline [0 ]= 863 ;
      } 
      putgeterror () ;
      recyclevalue ( p + 2 ) ;
      cury = 0 ;
    } 
    flushcurexp ( 0 ) ;
  } 
} 
halfword 
#ifdef HAVE_PROTOTYPES
newknot ( void ) 
#else
newknot ( ) 
#endif
{
  register halfword Result; halfword q  ;
  q = getnode ( 7 ) ;
  mem [q ].hhfield .b0 = 0 ;
  mem [q ].hhfield .b1 = 0 ;
  mem [q ].hhfield .v.RH = q ;
  knownpair () ;
  mem [q + 1 ].cint = curx ;
  mem [q + 2 ].cint = cury ;
  Result = q ;
  return Result ;
} 
smallnumber 
#ifdef HAVE_PROTOTYPES
scandirection ( void ) 
#else
scandirection ( ) 
#endif
{
  register smallnumber Result; char t  ;
  scaled x  ;
  getxnext () ;
  if ( curcmd == 62 ) 
  {
    getxnext () ;
    scanexpression () ;
    if ( ( curtype != 16 ) || ( curexp < 0 ) ) 
    {
      disperr ( 0 , 870 ) ;
      {
	helpptr = 1 ;
	helpline [0 ]= 871 ;
      } 
      putgetflusherror ( 65536L ) ;
    } 
    t = 3 ;
  } 
  else {
      
    scanexpression () ;
    if ( curtype > 14 ) 
    {
      if ( curtype != 16 ) 
      {
	disperr ( 0 , 864 ) ;
	{
	  helpptr = 5 ;
	  helpline [4 ]= 865 ;
	  helpline [3 ]= 860 ;
	  helpline [2 ]= 861 ;
	  helpline [1 ]= 862 ;
	  helpline [0 ]= 863 ;
	} 
	putgetflusherror ( 0 ) ;
      } 
      x = curexp ;
      if ( curcmd != 81 ) 
      {
	missingerr ( 44 ) ;
	{
	  helpptr = 2 ;
	  helpline [1 ]= 872 ;
	  helpline [0 ]= 873 ;
	} 
	backerror () ;
      } 
      getxnext () ;
      scanexpression () ;
      if ( curtype != 16 ) 
      {
	disperr ( 0 , 866 ) ;
	{
	  helpptr = 5 ;
	  helpline [4 ]= 867 ;
	  helpline [3 ]= 860 ;
	  helpline [2 ]= 861 ;
	  helpline [1 ]= 862 ;
	  helpline [0 ]= 863 ;
	} 
	putgetflusherror ( 0 ) ;
      } 
      cury = curexp ;
      curx = x ;
    } 
    else knownpair () ;
    if ( ( curx == 0 ) && ( cury == 0 ) ) 
    t = 4 ;
    else {
	
      t = 2 ;
      curexp = narg ( curx , cury ) ;
    } 
  } 
  if ( curcmd != 67 ) 
  {
    missingerr ( 125 ) ;
    {
      helpptr = 3 ;
      helpline [2 ]= 868 ;
      helpline [1 ]= 869 ;
      helpline [0 ]= 738 ;
    } 
    backerror () ;
  } 
  getxnext () ;
  Result = t ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
finishread ( void ) 
#else
finishread ( ) 
#endif
{
  poolpointer k  ;
  {
    if ( poolptr + last - curinput .startfield > maxpoolptr ) 
    if ( poolptr + last - curinput .startfield > poolsize ) 
    docompaction ( last - curinput .startfield ) ;
    else maxpoolptr = poolptr + last - curinput .startfield ;
  } 
  {register integer for_end; k = curinput .startfield ;for_end = last - 1 
  ; if ( k <= for_end) do 
    {
      strpool [poolptr ]= buffer [k ];
      incr ( poolptr ) ;
    } 
  while ( k++ < for_end ) ;} 
  endfilereading () ;
  curtype = 4 ;
  curexp = makestring () ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdonullary ( quarterword c ) 
#else
zdonullary ( c ) 
  quarterword c ;
#endif
{
  {
    if ( aritherror ) 
    cleararith () ;
  } 
  if ( internal [6 ]> 131072L ) 
  showcmdmod ( 35 , c ) ;
  switch ( c ) 
  {case 30 : 
  case 31 : 
    {
      curtype = 2 ;
      curexp = c ;
    } 
    break ;
  case 32 : 
    {
      curtype = 10 ;
      curexp = getnode ( 8 ) ;
      initedges ( curexp ) ;
    } 
    break ;
  case 33 : 
    {
      curtype = 6 ;
      curexp = getpencircle ( 0 ) ;
    } 
    break ;
  case 37 : 
    {
      curtype = 16 ;
      curexp = normrand () ;
    } 
    break ;
  case 36 : 
    {
      curtype = 6 ;
      curexp = getpencircle ( 65536L ) ;
    } 
    break ;
  case 34 : 
    {
      if ( jobname == 0 ) 
      openlogfile () ;
      curtype = 4 ;
      curexp = jobname ;
    } 
    break ;
  case 35 : 
    {
      if ( interaction <= 1 ) 
      fatalerror ( 884 ) ;
      beginfilereading () ;
      curinput .namefield = 1 ;
      curinput .limitfield = curinput .startfield ;
      {
	;
	print ( 284 ) ;
	terminput () ;
      } 
      finishread () ;
    } 
    break ;
  } 
  {
    if ( aritherror ) 
    cleararith () ;
  } 
} 
boolean 
#ifdef HAVE_PROTOTYPES
znicepair ( integer p , quarterword t ) 
#else
znicepair ( p , t ) 
  integer p ;
  quarterword t ;
#endif
{
  /* 10 */ register boolean Result; if ( t == 14 ) 
  {
    p = mem [p + 1 ].cint ;
    if ( mem [p ].hhfield .b0 == 16 ) 
    if ( mem [p + 2 ].hhfield .b0 == 16 ) 
    {
      Result = true ;
      goto lab10 ;
    } 
  } 
  Result = false ;
  lab10: ;
  return Result ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
znicecolororpair ( integer p , quarterword t ) 
#else
znicecolororpair ( p , t ) 
  integer p ;
  quarterword t ;
#endif
{
  /* 10 */ register boolean Result; halfword q, r  ;
  if ( ( t != 14 ) && ( t != 13 ) ) 
  Result = false ;
  else {
      
    q = mem [p + 1 ].cint ;
    r = q + bignodesize [mem [p ].hhfield .b0 ];
    do {
	r = r - 2 ;
      if ( mem [r ].hhfield .b0 != 16 ) 
      {
	Result = false ;
	goto lab10 ;
      } 
    } while ( ! ( r == q ) ) ;
    Result = true ;
  } 
  lab10: ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintknownorunknowntype ( smallnumber t , integer v ) 
#else
zprintknownorunknowntype ( t , v ) 
  smallnumber t ;
  integer v ;
#endif
{
  printchar ( 40 ) ;
  if ( t > 16 ) 
  print ( 885 ) ;
  else {
      
    if ( ( t == 14 ) || ( t == 13 ) ) 
    if ( ! nicecolororpair ( v , t ) ) 
    print ( 886 ) ;
    printtype ( t ) ;
  } 
  printchar ( 41 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zbadunary ( quarterword c ) 
#else
zbadunary ( c ) 
  quarterword c ;
#endif
{
  disperr ( 0 , 887 ) ;
  printop ( c ) ;
  printknownorunknowntype ( curtype , curexp ) ;
  {
    helpptr = 3 ;
    helpline [2 ]= 888 ;
    helpline [1 ]= 889 ;
    helpline [0 ]= 890 ;
  } 
  putgeterror () ;
} 
void 
#ifdef HAVE_PROTOTYPES
znegatedeplist ( halfword p ) 
#else
znegatedeplist ( p ) 
  halfword p ;
#endif
{
  /* 10 */ while ( true ) {
      
    mem [p + 1 ].cint = - (integer) mem [p + 1 ].cint ;
    if ( mem [p ].hhfield .lhfield == 0 ) 
    goto lab10 ;
    p = mem [p ].hhfield .v.RH ;
  } 
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
pairtopath ( void ) 
#else
pairtopath ( ) 
#endif
{
  curexp = newknot () ;
  curtype = 8 ;
} 
void 
#ifdef HAVE_PROTOTYPES
ztakepart ( quarterword c ) 
#else
ztakepart ( c ) 
  quarterword c ;
#endif
{
  halfword p  ;
  p = mem [curexp + 1 ].cint ;
  mem [10 ].cint = p ;
  mem [9 ].hhfield .b0 = curtype ;
  mem [p ].hhfield .v.RH = 9 ;
  freenode ( curexp , 2 ) ;
  makeexpcopy ( p + sectoroffset [c - 49 ]) ;
  recyclevalue ( 9 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
ztakepictpart ( quarterword c ) 
#else
ztakepictpart ( c ) 
  quarterword c ;
#endif
{
  /* 10 45 */ halfword p  ;
  p = mem [curexp + 7 ].hhfield .v.RH ;
  if ( p != 0 ) 
  {
    switch ( c ) 
    {case 54 : 
    case 55 : 
    case 56 : 
    case 57 : 
    case 58 : 
    case 59 : 
      if ( mem [p ].hhfield .b0 == 3 ) 
      flushcurexp ( mem [p + c - 46 ].cint ) ;
      else goto lab45 ;
      break ;
    case 60 : 
    case 61 : 
    case 62 : 
      if ( ( mem [p ].hhfield .b0 < 4 ) ) 
      flushcurexp ( mem [p + c - 58 ].cint ) ;
      else goto lab45 ;
      break ;
    case 64 : 
      if ( mem [p ].hhfield .b0 != 3 ) 
      goto lab45 ;
      else {
	  
	flushcurexp ( mem [p + 1 ].hhfield .v.RH ) ;
	{
	  if ( strref [curexp ]< 127 ) 
	  incr ( strref [curexp ]) ;
	} 
	curtype = 4 ;
      } 
      break ;
    case 63 : 
      if ( mem [p ].hhfield .b0 != 3 ) 
      goto lab45 ;
      else {
	  
	flushcurexp ( fontname [mem [p + 1 ].hhfield .lhfield ]) ;
	{
	  if ( strref [curexp ]< 127 ) 
	  incr ( strref [curexp ]) ;
	} 
	curtype = 4 ;
      } 
      break ;
    case 65 : 
      if ( mem [p ].hhfield .b0 == 3 ) 
      goto lab45 ;
      else if ( ( mem [p ].hhfield .b0 >= 6 ) ) 
      confusion ( 892 ) ;
      else {
	  
	flushcurexp ( copypath ( mem [p + 1 ].hhfield .v.RH ) ) ;
	curtype = 8 ;
      } 
      break ;
    case 66 : 
      if ( ! ( mem [p ].hhfield .b0 < 3 ) ) 
      goto lab45 ;
      else if ( mem [p + 1 ].hhfield .lhfield == 0 ) 
      goto lab45 ;
      else {
	  
	flushcurexp ( makepen ( copypath ( mem [p + 1 ].hhfield .lhfield ) , 
	false ) ) ;
	curtype = 6 ;
      } 
      break ;
    case 67 : 
      if ( mem [p ].hhfield .b0 != 2 ) 
      goto lab45 ;
      else if ( mem [p + 6 ].hhfield .v.RH == 0 ) 
      goto lab45 ;
      else {
	  
	incr ( mem [mem [p + 6 ].hhfield .v.RH ].hhfield .lhfield ) ;
	sesf = mem [p + 7 ].cint ;
	sepic = mem [p + 6 ].hhfield .v.RH ;
	scaleedges () ;
	flushcurexp ( sepic ) ;
	curtype = 10 ;
      } 
      break ;
    } 
    goto lab10 ;
  } 
  lab45: switch ( c ) 
  {case 64 : 
  case 63 : 
    {
      flushcurexp ( 284 ) ;
      curtype = 4 ;
    } 
    break ;
  case 65 : 
    {
      flushcurexp ( getnode ( 7 ) ) ;
      mem [curexp ].hhfield .b0 = 0 ;
      mem [curexp ].hhfield .b1 = 0 ;
      mem [curexp ].hhfield .v.RH = curexp ;
      mem [curexp + 1 ].cint = 0 ;
      mem [curexp + 2 ].cint = 0 ;
      curtype = 8 ;
    } 
    break ;
  case 66 : 
    {
      flushcurexp ( getpencircle ( 0 ) ) ;
      curtype = 6 ;
    } 
    break ;
  case 67 : 
    {
      flushcurexp ( getnode ( 8 ) ) ;
      initedges ( curexp ) ;
      curtype = 10 ;
    } 
    break ;
    default: 
    flushcurexp ( 0 ) ;
    break ;
  } 
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zstrtonum ( quarterword c ) 
#else
zstrtonum ( c ) 
  quarterword c ;
#endif
{
  integer n  ;
  ASCIIcode m  ;
  poolpointer k  ;
  char b  ;
  boolean badchar  ;
  if ( c == 50 ) 
  if ( ( strstart [nextstr [curexp ]]- strstart [curexp ]) == 0 ) 
  n = -1 ;
  else n = strpool [strstart [curexp ]];
  else {
      
    if ( c == 48 ) 
    b = 8 ;
    else b = 16 ;
    n = 0 ;
    badchar = false ;
    {register integer for_end; k = strstart [curexp ];for_end = strstart [
    nextstr [curexp ]]- 1 ; if ( k <= for_end) do 
      {
	m = strpool [k ];
	if ( ( m >= 48 ) && ( m <= 57 ) ) 
	m = m - 48 ;
	else if ( ( m >= 65 ) && ( m <= 70 ) ) 
	m = m - 55 ;
	else if ( ( m >= 97 ) && ( m <= 102 ) ) 
	m = m - 87 ;
	else {
	    
	  badchar = true ;
	  m = 0 ;
	} 
	if ( m >= b ) 
	{
	  badchar = true ;
	  m = 0 ;
	} 
	if ( n < 32768L / b ) 
	n = n * b + m ;
	else n = 32767 ;
      } 
    while ( k++ < for_end ) ;} 
    if ( badchar ) 
    {
      disperr ( 0 , 893 ) ;
      if ( c == 48 ) 
      {
	helpptr = 1 ;
	helpline [0 ]= 894 ;
      } 
      else {
	  
	helpptr = 1 ;
	helpline [0 ]= 895 ;
      } 
      putgeterror () ;
    } 
    if ( ( n > 4095 ) ) 
    if ( internal [30 ]> 0 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 896 ) ;
      } 
      printint ( n ) ;
      printchar ( 41 ) ;
      {
	helpptr = 2 ;
	helpline [1 ]= 897 ;
	helpline [0 ]= 606 ;
      } 
      putgeterror () ;
    } 
  } 
  flushcurexp ( n * 65536L ) ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
pathlength ( void ) 
#else
pathlength ( ) 
#endif
{
  register scaled Result; scaled n  ;
  halfword p  ;
  p = curexp ;
  if ( mem [p ].hhfield .b0 == 0 ) 
  n = -65536L ;
  else n = 0 ;
  do {
      p = mem [p ].hhfield .v.RH ;
    n = n + 65536L ;
  } while ( ! ( p == curexp ) ) ;
  Result = n ;
  return Result ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
pictlength ( void ) 
#else
pictlength ( ) 
#endif
{
  /* 40 */ register scaled Result; scaled n  ;
  halfword p  ;
  n = 0 ;
  p = mem [curexp + 7 ].hhfield .v.RH ;
  if ( p != 0 ) 
  {
    if ( ( mem [p ].hhfield .b0 >= 4 ) ) 
    if ( skip1component ( p ) == 0 ) 
    p = mem [p ].hhfield .v.RH ;
    while ( p != 0 ) {
	
      if ( ! ( mem [p ].hhfield .b0 >= 4 ) ) 
      p = mem [p ].hhfield .v.RH ;
      else if ( ! ( mem [p ].hhfield .b0 >= 6 ) ) 
      p = skip1component ( p ) ;
      else goto lab40 ;
      n = n + 65536L ;
    } 
  } 
  lab40: Result = n ;
  return Result ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zcountturns ( halfword c ) 
#else
zcountturns ( c ) 
  halfword c ;
#endif
{
  register scaled Result; halfword p  ;
  integer t  ;
  t = 0 ;
  p = c ;
  do {
      t = t + mem [p ].hhfield .lhfield - 16384 ;
    p = mem [p ].hhfield .v.RH ;
  } while ( ! ( p == c ) ) ;
  Result = ( t / 3 ) * 65536L ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
ztestknown ( quarterword c ) 
#else
ztestknown ( c ) 
  quarterword c ;
#endif
{
  /* 30 */ char b  ;
  halfword p, q  ;
  b = 31 ;
  switch ( curtype ) 
  {case 1 : 
  case 2 : 
  case 4 : 
  case 6 : 
  case 8 : 
  case 10 : 
  case 16 : 
    b = 30 ;
    break ;
  case 12 : 
  case 13 : 
  case 14 : 
    {
      p = mem [curexp + 1 ].cint ;
      q = p + bignodesize [curtype ];
      do {
	  q = q - 2 ;
	if ( mem [q ].hhfield .b0 != 16 ) 
	goto lab30 ;
      } while ( ! ( q == p ) ) ;
      b = 30 ;
      lab30: ;
    } 
    break ;
    default: 
    ;
    break ;
  } 
  if ( c == 41 ) 
  flushcurexp ( b ) ;
  else flushcurexp ( 61 - b ) ;
  curtype = 2 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zpairvalue ( scaled x , scaled y ) 
#else
zpairvalue ( x , y ) 
  scaled x ;
  scaled y ;
#endif
{
  halfword p  ;
  p = getnode ( 2 ) ;
  flushcurexp ( p ) ;
  curtype = 14 ;
  mem [p ].hhfield .b0 = 14 ;
  mem [p ].hhfield .b1 = 14 ;
  initbignode ( p ) ;
  p = mem [p + 1 ].cint ;
  mem [p ].hhfield .b0 = 16 ;
  mem [p + 1 ].cint = x ;
  mem [p + 2 ].hhfield .b0 = 16 ;
  mem [p + 3 ].cint = y ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
getcurbbox ( void ) 
#else
getcurbbox ( ) 
#endif
{
  /* 10 */ register boolean Result; switch ( curtype ) 
  {case 10 : 
    {
      setbbox ( curexp , true ) ;
      if ( mem [curexp + 2 ].cint > mem [curexp + 4 ].cint ) 
      {
	bbmin [0 ]= 0 ;
	bbmax [0 ]= 0 ;
	bbmin [1 ]= 0 ;
	bbmax [1 ]= 0 ;
      } 
      else {
	  
	bbmin [0 ]= mem [curexp + 2 ].cint ;
	bbmax [0 ]= mem [curexp + 4 ].cint ;
	bbmin [1 ]= mem [curexp + 3 ].cint ;
	bbmax [1 ]= mem [curexp + 5 ].cint ;
      } 
    } 
    break ;
  case 8 : 
    pathbbox ( curexp ) ;
    break ;
  case 6 : 
    penbbox ( curexp ) ;
    break ;
    default: 
    {
      Result = false ;
      goto lab10 ;
    } 
    break ;
  } 
  Result = true ;
  lab10: ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdoreadorclose ( quarterword c ) 
#else
zdoreadorclose ( c ) 
  quarterword c ;
#endif
{
  /* 10 22 40 45 46 */ readfindex n, n0  ;
  n = readfiles ;
  n0 = readfiles ;
  do {
      lab22: if ( n > 0 ) 
    decr ( n ) ;
    else if ( c == 39 ) 
    goto lab46 ;
    else {
	
      if ( n0 == readfiles ) 
      if ( readfiles < maxreadfiles ) 
      incr ( readfiles ) ;
      else overflow ( 899 , maxreadfiles ) ;
      n = n0 ;
      if ( startreadinput ( curexp , n ) ) 
      goto lab40 ;
      else goto lab45 ;
    } 
    if ( rdfname [n ]== 0 ) 
    {
      n0 = n ;
      goto lab22 ;
    } 
  } while ( ! ( strvsstr ( curexp , rdfname [n ]) == 0 ) ) ;
  if ( c == 39 ) 
  {
    aclose ( rdfile [n ]) ;
    goto lab45 ;
  } 
  beginfilereading () ;
  curinput .namefield = 1 ;
  if ( inputln ( rdfile [n ], true ) ) 
  goto lab40 ;
  endfilereading () ;
  lab45: {
      
    if ( strref [rdfname [n ]]< 127 ) 
    if ( strref [rdfname [n ]]> 1 ) 
    decr ( strref [rdfname [n ]]) ;
    else flushstring ( rdfname [n ]) ;
  } 
  rdfname [n ]= 0 ;
  if ( n == readfiles - 1 ) 
  readfiles = n ;
  if ( c == 39 ) 
  goto lab46 ;
  if ( eofline == 0 ) 
  {
    {
      strpool [poolptr ]= 0 ;
      incr ( poolptr ) ;
    } 
    eofline = makestring () ;
    strref [eofline ]= 127 ;
  } 
  flushcurexp ( eofline ) ;
  curtype = 4 ;
  goto lab10 ;
  lab46: flushcurexp ( 0 ) ;
  curtype = 1 ;
  goto lab10 ;
  lab40: flushcurexp ( 0 ) ;
  finishread () ;
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdounary ( quarterword c ) 
#else
zdounary ( c ) 
  quarterword c ;
#endif
{
  halfword p, q, r  ;
  integer x  ;
  {
    if ( aritherror ) 
    cleararith () ;
  } 
  if ( internal [6 ]> 131072L ) 
  {
    begindiagnostic () ;
    printnl ( 123 ) ;
    printop ( c ) ;
    printchar ( 40 ) ;
    printexp ( 0 , 0 ) ;
    print ( 891 ) ;
    enddiagnostic ( false ) ;
  } 
  switch ( c ) 
  {case 89 : 
    if ( curtype < 13 ) 
    badunary ( 89 ) ;
    break ;
  case 90 : 
    switch ( curtype ) 
    {case 13 : 
    case 14 : 
    case 19 : 
      {
	q = curexp ;
	makeexpcopy ( q ) ;
	if ( curtype == 17 ) 
	negatedeplist ( mem [curexp + 1 ].hhfield .v.RH ) ;
	else if ( curtype <= 14 ) 
	{
	  p = mem [curexp + 1 ].cint ;
	  r = p + bignodesize [curtype ];
	  do {
	      r = r - 2 ;
	    if ( mem [r ].hhfield .b0 == 16 ) 
	    mem [r + 1 ].cint = - (integer) mem [r + 1 ].cint ;
	    else negatedeplist ( mem [r + 1 ].hhfield .v.RH ) ;
	  } while ( ! ( r == p ) ) ;
	} 
	recyclevalue ( q ) ;
	freenode ( q , 2 ) ;
      } 
      break ;
    case 17 : 
    case 18 : 
      negatedeplist ( mem [curexp + 1 ].hhfield .v.RH ) ;
      break ;
    case 16 : 
      curexp = - (integer) curexp ;
      break ;
      default: 
      badunary ( 90 ) ;
      break ;
    } 
    break ;
  case 43 : 
    if ( curtype != 2 ) 
    badunary ( 43 ) ;
    else curexp = 61 - curexp ;
    break ;
  case 68 : 
  case 69 : 
  case 70 : 
  case 71 : 
  case 72 : 
  case 73 : 
  case 74 : 
  case 40 : 
  case 75 : 
    if ( curtype != 16 ) 
    badunary ( c ) ;
    else switch ( c ) 
    {case 68 : 
      curexp = squarert ( curexp ) ;
      break ;
    case 69 : 
      curexp = mexp ( curexp ) ;
      break ;
    case 70 : 
      curexp = mlog ( curexp ) ;
      break ;
    case 71 : 
    case 72 : 
      {
	nsincos ( ( curexp % 23592960L ) * 16 ) ;
	if ( c == 71 ) 
	curexp = roundfraction ( nsin ) ;
	else curexp = roundfraction ( ncos ) ;
      } 
      break ;
    case 73 : 
      curexp = floorscaled ( curexp ) ;
      break ;
    case 74 : 
      curexp = unifrand ( curexp ) ;
      break ;
    case 40 : 
      {
	if ( odd ( roundunscaled ( curexp ) ) ) 
	curexp = 30 ;
	else curexp = 31 ;
	curtype = 2 ;
      } 
      break ;
    case 75 : 
      {
	curexp = roundunscaled ( curexp ) % 256 ;
	if ( curexp < 0 ) 
	curexp = curexp + 256 ;
	if ( charexists [curexp ]) 
	curexp = 30 ;
	else curexp = 31 ;
	curtype = 2 ;
      } 
      break ;
    } 
    break ;
  case 82 : 
    if ( nicepair ( curexp , curtype ) ) 
    {
      p = mem [curexp + 1 ].cint ;
      x = narg ( mem [p + 1 ].cint , mem [p + 3 ].cint ) ;
      if ( x >= 0 ) 
      flushcurexp ( ( x + 8 ) / 16 ) ;
      else flushcurexp ( - (integer) ( ( - (integer) x + 8 ) / 16 ) ) ;
    } 
    else badunary ( 82 ) ;
    break ;
  case 54 : 
  case 55 : 
    if ( ( curtype == 14 ) || ( curtype == 12 ) ) 
    takepart ( c ) ;
    else if ( curtype == 10 ) 
    takepictpart ( c ) ;
    else badunary ( c ) ;
    break ;
  case 56 : 
  case 57 : 
  case 58 : 
  case 59 : 
    if ( curtype == 12 ) 
    takepart ( c ) ;
    else if ( curtype == 10 ) 
    takepictpart ( c ) ;
    else badunary ( c ) ;
    break ;
  case 60 : 
  case 61 : 
  case 62 : 
    if ( curtype == 13 ) 
    takepart ( c ) ;
    else if ( curtype == 10 ) 
    takepictpart ( c ) ;
    else badunary ( c ) ;
    break ;
  case 63 : 
  case 64 : 
  case 65 : 
  case 66 : 
  case 67 : 
    if ( curtype == 10 ) 
    takepictpart ( c ) ;
    else badunary ( c ) ;
    break ;
  case 51 : 
    if ( curtype != 16 ) 
    badunary ( 51 ) ;
    else {
	
      curexp = roundunscaled ( curexp ) % 256 ;
      curtype = 4 ;
      if ( curexp < 0 ) 
      curexp = curexp + 256 ;
    } 
    break ;
  case 44 : 
    if ( curtype != 16 ) 
    badunary ( 44 ) ;
    else {
	
      oldsetting = selector ;
      selector = 4 ;
      printscaled ( curexp ) ;
      curexp = makestring () ;
      selector = oldsetting ;
      curtype = 4 ;
    } 
    break ;
  case 48 : 
  case 49 : 
  case 50 : 
    if ( curtype != 4 ) 
    badunary ( c ) ;
    else strtonum ( c ) ;
    break ;
  case 76 : 
    if ( curtype != 4 ) 
    badunary ( 76 ) ;
    else flushcurexp ( ( fontdsize [findfont ( curexp ) ]+ 8 ) / 16 ) ;
    break ;
  case 52 : 
    switch ( curtype ) 
    {case 4 : 
      flushcurexp ( ( strstart [nextstr [curexp ]]- strstart [curexp ]) 
      * 65536L ) ;
      break ;
    case 8 : 
      flushcurexp ( pathlength () ) ;
      break ;
    case 16 : 
      curexp = abs ( curexp ) ;
      break ;
    case 10 : 
      flushcurexp ( pictlength () ) ;
      break ;
      default: 
      if ( nicepair ( curexp , curtype ) ) 
      flushcurexp ( pythadd ( mem [mem [curexp + 1 ].cint + 1 ].cint , mem 
      [mem [curexp + 1 ].cint + 3 ].cint ) ) ;
      else badunary ( c ) ;
      break ;
    } 
    break ;
  case 53 : 
    if ( curtype == 14 ) 
    flushcurexp ( 0 ) ;
    else if ( curtype != 8 ) 
    badunary ( 53 ) ;
    else if ( mem [curexp ].hhfield .b0 == 0 ) 
    flushcurexp ( 0 ) ;
    else {
	
      curexp = offsetprep ( curexp , 13 ) ;
      if ( internal [5 ]> 65536L ) 
      printspec ( curexp , 13 , 898 ) ;
      flushcurexp ( countturns ( curexp ) ) ;
    } 
    break ;
  case 2 : 
    {
      if ( ( curtype >= 2 ) && ( curtype <= 3 ) ) 
      flushcurexp ( 30 ) ;
      else flushcurexp ( 31 ) ;
      curtype = 2 ;
    } 
    break ;
  case 4 : 
    {
      if ( ( curtype >= 4 ) && ( curtype <= 5 ) ) 
      flushcurexp ( 30 ) ;
      else flushcurexp ( 31 ) ;
      curtype = 2 ;
    } 
    break ;
  case 6 : 
    {
      if ( ( curtype >= 6 ) && ( curtype <= 7 ) ) 
      flushcurexp ( 30 ) ;
      else flushcurexp ( 31 ) ;
      curtype = 2 ;
    } 
    break ;
  case 8 : 
    {
      if ( ( curtype >= 8 ) && ( curtype <= 9 ) ) 
      flushcurexp ( 30 ) ;
      else flushcurexp ( 31 ) ;
      curtype = 2 ;
    } 
    break ;
  case 10 : 
    {
      if ( ( curtype >= 10 ) && ( curtype <= 11 ) ) 
      flushcurexp ( 30 ) ;
      else flushcurexp ( 31 ) ;
      curtype = 2 ;
    } 
    break ;
  case 12 : 
  case 13 : 
  case 14 : 
    {
      if ( curtype == c ) 
      flushcurexp ( 30 ) ;
      else flushcurexp ( 31 ) ;
      curtype = 2 ;
    } 
    break ;
  case 15 : 
    {
      if ( ( curtype >= 16 ) && ( curtype <= 19 ) ) 
      flushcurexp ( 30 ) ;
      else flushcurexp ( 31 ) ;
      curtype = 2 ;
    } 
    break ;
  case 41 : 
  case 42 : 
    testknown ( c ) ;
    break ;
  case 83 : 
    {
      if ( curtype != 8 ) 
      flushcurexp ( 31 ) ;
      else if ( mem [curexp ].hhfield .b0 != 0 ) 
      flushcurexp ( 30 ) ;
      else flushcurexp ( 31 ) ;
      curtype = 2 ;
    } 
    break ;
  case 81 : 
    {
      if ( curtype == 14 ) 
      pairtopath () ;
      if ( curtype != 8 ) 
      badunary ( 81 ) ;
      else flushcurexp ( getarclength ( curexp ) ) ;
    } 
    break ;
  case 84 : 
  case 85 : 
  case 86 : 
  case 87 : 
  case 88 : 
    {
      if ( curtype != 10 ) 
      flushcurexp ( 31 ) ;
      else if ( mem [curexp + 7 ].hhfield .v.RH == 0 ) 
      flushcurexp ( 31 ) ;
      else if ( mem [mem [curexp + 7 ].hhfield .v.RH ].hhfield .b0 == c - 
      83 ) 
      flushcurexp ( 30 ) ;
      else flushcurexp ( 31 ) ;
      curtype = 2 ;
    } 
    break ;
  case 47 : 
    {
      if ( curtype == 14 ) 
      pairtopath () ;
      if ( curtype != 8 ) 
      badunary ( 47 ) ;
      else {
	  
	curtype = 6 ;
	curexp = makepen ( curexp , true ) ;
      } 
    } 
    break ;
  case 46 : 
    if ( curtype != 6 ) 
    badunary ( 46 ) ;
    else {
	
      curtype = 8 ;
      makepath ( curexp ) ;
    } 
    break ;
  case 45 : 
    if ( curtype == 8 ) 
    {
      p = htapypoc ( curexp ) ;
      if ( mem [p ].hhfield .b1 == 0 ) 
      p = mem [p ].hhfield .v.RH ;
      tossknotlist ( curexp ) ;
      curexp = p ;
    } 
    else if ( curtype == 14 ) 
    pairtopath () ;
    else badunary ( 45 ) ;
    break ;
  case 77 : 
    if ( ! getcurbbox () ) 
    badunary ( 77 ) ;
    else pairvalue ( bbmin [0 ], bbmin [1 ]) ;
    break ;
  case 78 : 
    if ( ! getcurbbox () ) 
    badunary ( 78 ) ;
    else pairvalue ( bbmax [0 ], bbmin [1 ]) ;
    break ;
  case 79 : 
    if ( ! getcurbbox () ) 
    badunary ( 79 ) ;
    else pairvalue ( bbmin [0 ], bbmax [1 ]) ;
    break ;
  case 80 : 
    if ( ! getcurbbox () ) 
    badunary ( 80 ) ;
    else pairvalue ( bbmax [0 ], bbmax [1 ]) ;
    break ;
  case 38 : 
  case 39 : 
    if ( curtype != 4 ) 
    badunary ( c ) ;
    else doreadorclose ( c ) ;
    break ;
  } 
  {
    if ( aritherror ) 
    cleararith () ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zbadbinary ( halfword p , quarterword c ) 
#else
zbadbinary ( p , c ) 
  halfword p ;
  quarterword c ;
#endif
{
  disperr ( p , 284 ) ;
  disperr ( 0 , 887 ) ;
  if ( c >= 115 ) 
  printop ( c ) ;
  printknownorunknowntype ( mem [p ].hhfield .b0 , p ) ;
  if ( c >= 115 ) 
  print ( 489 ) ;
  else printop ( c ) ;
  printknownorunknowntype ( curtype , curexp ) ;
  {
    helpptr = 3 ;
    helpline [2 ]= 888 ;
    helpline [1 ]= 900 ;
    helpline [0 ]= 901 ;
  } 
  putgeterror () ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
ztarnished ( halfword p ) 
#else
ztarnished ( p ) 
  halfword p ;
#endif
{
  /* 10 */ register halfword Result; halfword q  ;
  halfword r  ;
  q = mem [p + 1 ].cint ;
  r = q + bignodesize [mem [p ].hhfield .b0 ];
  do {
      r = r - 2 ;
    if ( mem [r ].hhfield .b0 == 19 ) 
    {
      Result = 1 ;
      goto lab10 ;
    } 
  } while ( ! ( r == q ) ) ;
  Result = 0 ;
  lab10: ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdepfinish ( halfword v , halfword q , smallnumber t ) 
#else
zdepfinish ( v , q , t ) 
  halfword v ;
  halfword q ;
  smallnumber t ;
#endif
{
  halfword p  ;
  scaled vv  ;
  if ( q == 0 ) 
  p = curexp ;
  else p = q ;
  mem [p + 1 ].hhfield .v.RH = v ;
  mem [p ].hhfield .b0 = t ;
  if ( mem [v ].hhfield .lhfield == 0 ) 
  {
    vv = mem [v + 1 ].cint ;
    if ( q == 0 ) 
    flushcurexp ( vv ) ;
    else {
	
      recyclevalue ( p ) ;
      mem [q ].hhfield .b0 = 16 ;
      mem [q + 1 ].cint = vv ;
    } 
  } 
  else if ( q == 0 ) 
  curtype = t ;
  if ( fixneeded ) 
  fixdependencies () ;
} 
void 
#ifdef HAVE_PROTOTYPES
zaddorsubtract ( halfword p , halfword q , quarterword c ) 
#else
zaddorsubtract ( p , q , c ) 
  halfword p ;
  halfword q ;
  quarterword c ;
#endif
{
  /* 30 10 */ smallnumber s, t  ;
  halfword r  ;
  integer v  ;
  if ( q == 0 ) 
  {
    t = curtype ;
    if ( t < 17 ) 
    v = curexp ;
    else v = mem [curexp + 1 ].hhfield .v.RH ;
  } 
  else {
      
    t = mem [q ].hhfield .b0 ;
    if ( t < 17 ) 
    v = mem [q + 1 ].cint ;
    else v = mem [q + 1 ].hhfield .v.RH ;
  } 
  if ( t == 16 ) 
  {
    if ( c == 90 ) 
    v = - (integer) v ;
    if ( mem [p ].hhfield .b0 == 16 ) 
    {
      v = slowadd ( mem [p + 1 ].cint , v ) ;
      if ( q == 0 ) 
      curexp = v ;
      else mem [q + 1 ].cint = v ;
      goto lab10 ;
    } 
    r = mem [p + 1 ].hhfield .v.RH ;
    while ( mem [r ].hhfield .lhfield != 0 ) r = mem [r ].hhfield .v.RH ;
    mem [r + 1 ].cint = slowadd ( mem [r + 1 ].cint , v ) ;
    if ( q == 0 ) 
    {
      q = getnode ( 2 ) ;
      curexp = q ;
      curtype = mem [p ].hhfield .b0 ;
      mem [q ].hhfield .b1 = 14 ;
    } 
    mem [q + 1 ].hhfield .v.RH = mem [p + 1 ].hhfield .v.RH ;
    mem [q ].hhfield .b0 = mem [p ].hhfield .b0 ;
    mem [q + 1 ].hhfield .lhfield = mem [p + 1 ].hhfield .lhfield ;
    mem [mem [p + 1 ].hhfield .lhfield ].hhfield .v.RH = q ;
    mem [p ].hhfield .b0 = 16 ;
  } 
  else {
      
    if ( c == 90 ) 
    negatedeplist ( v ) ;
    if ( mem [p ].hhfield .b0 == 16 ) 
    {
      while ( mem [v ].hhfield .lhfield != 0 ) v = mem [v ].hhfield .v.RH 
      ;
      mem [v + 1 ].cint = slowadd ( mem [p + 1 ].cint , mem [v + 1 ]
      .cint ) ;
    } 
    else {
	
      s = mem [p ].hhfield .b0 ;
      r = mem [p + 1 ].hhfield .v.RH ;
      if ( t == 17 ) 
      {
	if ( s == 17 ) 
	if ( maxcoef ( r ) + maxcoef ( v ) < 626349397L ) 
	{
	  v = pplusq ( v , r , 17 ) ;
	  goto lab30 ;
	} 
	t = 18 ;
	v = poverv ( v , 65536L , 17 , 18 ) ;
      } 
      if ( s == 18 ) 
      v = pplusq ( v , r , 18 ) ;
      else v = pplusfq ( v , 65536L , r , 18 , 17 ) ;
      lab30: if ( q != 0 ) 
      depfinish ( v , q , t ) ;
      else {
	  
	curtype = t ;
	depfinish ( v , 0 , t ) ;
      } 
    } 
  } 
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdepmult ( halfword p , integer v , boolean visscaled ) 
#else
zdepmult ( p , v , visscaled ) 
  halfword p ;
  integer v ;
  boolean visscaled ;
#endif
{
  /* 10 */ halfword q  ;
  smallnumber s, t  ;
  if ( p == 0 ) 
  q = curexp ;
  else if ( mem [p ].hhfield .b0 != 16 ) 
  q = p ;
  else {
      
    if ( visscaled ) 
    mem [p + 1 ].cint = takescaled ( mem [p + 1 ].cint , v ) ;
    else mem [p + 1 ].cint = takefraction ( mem [p + 1 ].cint , v ) ;
    goto lab10 ;
  } 
  t = mem [q ].hhfield .b0 ;
  q = mem [q + 1 ].hhfield .v.RH ;
  s = t ;
  if ( t == 17 ) 
  if ( visscaled ) 
  if ( abvscd ( maxcoef ( q ) , abs ( v ) , 626349396L , 65536L ) >= 0 ) 
  t = 18 ;
  q = ptimesv ( q , v , s , t , visscaled ) ;
  depfinish ( q , p , t ) ;
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zhardtimes ( halfword p ) 
#else
zhardtimes ( p ) 
  halfword p ;
#endif
{
  /* 30 */ halfword q  ;
  halfword r  ;
  scaled v  ;
  if ( mem [p ].hhfield .b0 <= 14 ) 
  {
    q = stashcurexp () ;
    unstashcurexp ( p ) ;
    p = q ;
  } 
  r = mem [curexp + 1 ].cint + bignodesize [curtype ];
  while ( true ) {
      
    r = r - 2 ;
    v = mem [r + 1 ].cint ;
    mem [r ].hhfield .b0 = mem [p ].hhfield .b0 ;
    if ( r == mem [curexp + 1 ].cint ) 
    goto lab30 ;
    newdep ( r , copydeplist ( mem [p + 1 ].hhfield .v.RH ) ) ;
    depmult ( r , v , true ) ;
  } 
  lab30: mem [r + 1 ]= mem [p + 1 ];
  mem [mem [p + 1 ].hhfield .lhfield ].hhfield .v.RH = r ;
  freenode ( p , 2 ) ;
  depmult ( r , v , true ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdepdiv ( halfword p , scaled v ) 
#else
zdepdiv ( p , v ) 
  halfword p ;
  scaled v ;
#endif
{
  /* 10 */ halfword q  ;
  smallnumber s, t  ;
  if ( p == 0 ) 
  q = curexp ;
  else if ( mem [p ].hhfield .b0 != 16 ) 
  q = p ;
  else {
      
    mem [p + 1 ].cint = makescaled ( mem [p + 1 ].cint , v ) ;
    goto lab10 ;
  } 
  t = mem [q ].hhfield .b0 ;
  q = mem [q + 1 ].hhfield .v.RH ;
  s = t ;
  if ( t == 17 ) 
  if ( abvscd ( maxcoef ( q ) , 65536L , 626349396L , abs ( v ) ) >= 0 ) 
  t = 18 ;
  q = poverv ( q , v , s , t ) ;
  depfinish ( q , p , t ) ;
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zsetuptrans ( quarterword c ) 
#else
zsetuptrans ( c ) 
  quarterword c ;
#endif
{
  /* 30 10 */ halfword p, q, r  ;
  if ( ( c != 108 ) || ( curtype != 12 ) ) 
  {
    p = stashcurexp () ;
    curexp = idtransform () ;
    curtype = 12 ;
    q = mem [curexp + 1 ].cint ;
    switch ( c ) 
    {case 104 : 
      if ( mem [p ].hhfield .b0 == 16 ) 
      {
	nsincos ( ( mem [p + 1 ].cint % 23592960L ) * 16 ) ;
	mem [q + 5 ].cint = roundfraction ( ncos ) ;
	mem [q + 9 ].cint = roundfraction ( nsin ) ;
	mem [q + 7 ].cint = - (integer) mem [q + 9 ].cint ;
	mem [q + 11 ].cint = mem [q + 5 ].cint ;
	goto lab30 ;
      } 
      break ;
    case 105 : 
      if ( mem [p ].hhfield .b0 > 14 ) 
      {
	install ( q + 6 , p ) ;
	goto lab30 ;
      } 
      break ;
    case 106 : 
      if ( mem [p ].hhfield .b0 > 14 ) 
      {
	install ( q + 4 , p ) ;
	install ( q + 10 , p ) ;
	goto lab30 ;
      } 
      break ;
    case 107 : 
      if ( mem [p ].hhfield .b0 == 14 ) 
      {
	r = mem [p + 1 ].cint ;
	install ( q , r ) ;
	install ( q + 2 , r + 2 ) ;
	goto lab30 ;
      } 
      break ;
    case 109 : 
      if ( mem [p ].hhfield .b0 > 14 ) 
      {
	install ( q + 4 , p ) ;
	goto lab30 ;
      } 
      break ;
    case 110 : 
      if ( mem [p ].hhfield .b0 > 14 ) 
      {
	install ( q + 10 , p ) ;
	goto lab30 ;
      } 
      break ;
    case 111 : 
      if ( mem [p ].hhfield .b0 == 14 ) 
      {
	r = mem [p + 1 ].cint ;
	install ( q + 4 , r ) ;
	install ( q + 10 , r ) ;
	install ( q + 8 , r + 2 ) ;
	if ( mem [r + 2 ].hhfield .b0 == 16 ) 
	mem [r + 3 ].cint = - (integer) mem [r + 3 ].cint ;
	else negatedeplist ( mem [r + 3 ].hhfield .v.RH ) ;
	install ( q + 6 , r + 2 ) ;
	goto lab30 ;
      } 
      break ;
    case 108 : 
      ;
      break ;
    } 
    disperr ( p , 910 ) ;
    {
      helpptr = 3 ;
      helpline [2 ]= 911 ;
      helpline [1 ]= 912 ;
      helpline [0 ]= 913 ;
    } 
    putgeterror () ;
    lab30: recyclevalue ( p ) ;
    freenode ( p , 2 ) ;
  } 
  q = mem [curexp + 1 ].cint ;
  r = q + 12 ;
  do {
      r = r - 2 ;
    if ( mem [r ].hhfield .b0 != 16 ) 
    goto lab10 ;
  } while ( ! ( r == q ) ) ;
  txx = mem [q + 5 ].cint ;
  txy = mem [q + 7 ].cint ;
  tyx = mem [q + 9 ].cint ;
  tyy = mem [q + 11 ].cint ;
  tx = mem [q + 1 ].cint ;
  ty = mem [q + 3 ].cint ;
  flushcurexp ( 0 ) ;
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zsetupknowntrans ( quarterword c ) 
#else
zsetupknowntrans ( c ) 
  quarterword c ;
#endif
{
  setuptrans ( c ) ;
  if ( curtype != 16 ) 
  {
    disperr ( 0 , 914 ) ;
    {
      helpptr = 3 ;
      helpline [2 ]= 915 ;
      helpline [1 ]= 916 ;
      helpline [0 ]= 913 ;
    } 
    putgetflusherror ( 0 ) ;
    txx = 65536L ;
    txy = 0 ;
    tyx = 0 ;
    tyy = 65536L ;
    tx = 0 ;
    ty = 0 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
ztrans ( halfword p , halfword q ) 
#else
ztrans ( p , q ) 
  halfword p ;
  halfword q ;
#endif
{
  scaled v  ;
  v = takescaled ( mem [p ].cint , txx ) + takescaled ( mem [q ].cint , 
  txy ) + tx ;
  mem [q ].cint = takescaled ( mem [p ].cint , tyx ) + takescaled ( mem [
  q ].cint , tyy ) + ty ;
  mem [p ].cint = v ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdopathtrans ( halfword p ) 
#else
zdopathtrans ( p ) 
  halfword p ;
#endif
{
  /* 10 */ halfword q  ;
  q = p ;
  do {
      if ( mem [q ].hhfield .b0 != 0 ) 
    trans ( q + 3 , q + 4 ) ;
    trans ( q + 1 , q + 2 ) ;
    if ( mem [q ].hhfield .b1 != 0 ) 
    trans ( q + 5 , q + 6 ) ;
    q = mem [q ].hhfield .v.RH ;
  } while ( ! ( q == p ) ) ;
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdopentrans ( halfword p ) 
#else
zdopentrans ( p ) 
  halfword p ;
#endif
{
  /* 10 */ halfword q  ;
  if ( ( p == mem [p ].hhfield .v.RH ) ) 
  {
    trans ( p + 3 , p + 4 ) ;
    trans ( p + 5 , p + 6 ) ;
  } 
  q = p ;
  do {
      trans ( q + 1 , q + 2 ) ;
    q = mem [q ].hhfield .v.RH ;
  } while ( ! ( q == p ) ) ;
  lab10: ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zedgestrans ( halfword h ) 
#else
zedgestrans ( h ) 
  halfword h ;
#endif
{
  /* 31 */ register halfword Result; halfword q  ;
  halfword r, s  ;
  scaled sx, sy  ;
  scaled sqdet  ;
  integer sgndet  ;
  scaled v  ;
  h = privateedges ( h ) ;
  sqdet = sqrtdet ( txx , txy , tyx , tyy ) ;
  sgndet = abvscd ( txx , tyy , txy , tyx ) ;
  if ( mem [h ].hhfield .v.RH != 2 ) 
  if ( ( txy != 0 ) || ( tyx != 0 ) || ( ty != 0 ) || ( abs ( txx ) != abs ( 
  tyy ) ) ) 
  flushdashlist ( h ) ;
  else {
      
    if ( txx < 0 ) 
    {
      r = mem [h ].hhfield .v.RH ;
      mem [h ].hhfield .v.RH = 2 ;
      while ( r != 2 ) {
	  
	s = r ;
	r = mem [r ].hhfield .v.RH ;
	v = mem [s + 1 ].cint ;
	mem [s + 1 ].cint = mem [s + 2 ].cint ;
	mem [s + 2 ].cint = v ;
	mem [s ].hhfield .v.RH = mem [h ].hhfield .v.RH ;
	mem [h ].hhfield .v.RH = s ;
      } 
    } 
    r = mem [h ].hhfield .v.RH ;
    while ( r != 2 ) {
	
      mem [r + 1 ].cint = takescaled ( mem [r + 1 ].cint , txx ) + tx ;
      mem [r + 2 ].cint = takescaled ( mem [r + 2 ].cint , txx ) + tx ;
      r = mem [r ].hhfield .v.RH ;
    } 
    mem [h + 1 ].cint = takescaled ( mem [h + 1 ].cint , abs ( tyy ) ) ;
  } 
  if ( ( txx == 0 ) && ( tyy == 0 ) ) 
  {
    v = mem [h + 2 ].cint ;
    mem [h + 2 ].cint = mem [h + 3 ].cint ;
    mem [h + 3 ].cint = v ;
    v = mem [h + 4 ].cint ;
    mem [h + 4 ].cint = mem [h + 5 ].cint ;
    mem [h + 5 ].cint = v ;
  } 
  else if ( ( txy != 0 ) || ( tyx != 0 ) ) 
  {
    initbbox ( h ) ;
    goto lab31 ;
  } 
  if ( mem [h + 2 ].cint <= mem [h + 4 ].cint ) 
  {
    mem [h + 2 ].cint = takescaled ( mem [h + 2 ].cint , txx + txy ) + tx 
    ;
    mem [h + 4 ].cint = takescaled ( mem [h + 4 ].cint , txx + txy ) + tx 
    ;
    mem [h + 3 ].cint = takescaled ( mem [h + 3 ].cint , tyx + tyy ) + ty 
    ;
    mem [h + 5 ].cint = takescaled ( mem [h + 5 ].cint , tyx + tyy ) + ty 
    ;
    if ( txx + txy < 0 ) 
    {
      v = mem [h + 2 ].cint ;
      mem [h + 2 ].cint = mem [h + 4 ].cint ;
      mem [h + 4 ].cint = v ;
    } 
    if ( tyx + tyy < 0 ) 
    {
      v = mem [h + 3 ].cint ;
      mem [h + 3 ].cint = mem [h + 5 ].cint ;
      mem [h + 5 ].cint = v ;
    } 
  } 
  lab31: ;
  q = mem [h + 7 ].hhfield .v.RH ;
  while ( q != 0 ) {
      
    switch ( mem [q ].hhfield .b0 ) 
    {case 1 : 
    case 2 : 
      {
	dopathtrans ( mem [q + 1 ].hhfield .v.RH ) ;
	if ( mem [q + 1 ].hhfield .lhfield != 0 ) 
	{
	  sx = tx ;
	  sy = ty ;
	  tx = 0 ;
	  ty = 0 ;
	  dopentrans ( mem [q + 1 ].hhfield .lhfield ) ;
	  if ( ( ( mem [q ].hhfield .b0 == 2 ) && ( mem [q + 6 ].hhfield 
	  .v.RH != 0 ) ) ) 
	  mem [q + 7 ].cint = takescaled ( mem [q + 7 ].cint , sqdet ) ;
	  if ( ! ( mem [q + 1 ].hhfield .lhfield == mem [mem [q + 1 ]
	  .hhfield .lhfield ].hhfield .v.RH ) ) 
	  if ( sgndet < 0 ) 
	  mem [q + 1 ].hhfield .lhfield = makepen ( copypath ( mem [q + 1 ]
	  .hhfield .lhfield ) , true ) ;
	  tx = sx ;
	  ty = sy ;
	} 
      } 
      break ;
    case 4 : 
    case 5 : 
      dopathtrans ( mem [q + 1 ].hhfield .v.RH ) ;
      break ;
    case 3 : 
      {
	r = q + 8 ;
	trans ( r , r + 1 ) ;
	sx = tx ;
	sy = ty ;
	tx = 0 ;
	ty = 0 ;
	trans ( r + 2 , r + 4 ) ;
	trans ( r + 3 , r + 5 ) ;
	tx = sx ;
	ty = sy ;
      } 
      break ;
    case 6 : 
    case 7 : 
      ;
      break ;
    } 
    q = mem [q ].hhfield .v.RH ;
  } 
  Result = h ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdoedgestrans ( halfword p , quarterword c ) 
#else
zdoedgestrans ( p , c ) 
  halfword p ;
  quarterword c ;
#endif
{
  setupknowntrans ( c ) ;
  mem [p + 1 ].cint = edgestrans ( mem [p + 1 ].cint ) ;
  unstashcurexp ( p ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
scaleedges ( void ) 
#else
scaleedges ( ) 
#endif
{
  txx = sesf ;
  tyy = sesf ;
  txy = 0 ;
  tyx = 0 ;
  tx = 0 ;
  ty = 0 ;
  sepic = edgestrans ( sepic ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zbilin1 ( halfword p , scaled t , halfword q , scaled u , scaled delta ) 
#else
zbilin1 ( p , t , q , u , delta ) 
  halfword p ;
  scaled t ;
  halfword q ;
  scaled u ;
  scaled delta ;
#endif
{
  halfword r  ;
  if ( t != 65536L ) 
  depmult ( p , t , true ) ;
  if ( u != 0 ) 
  if ( mem [q ].hhfield .b0 == 16 ) 
  delta = delta + takescaled ( mem [q + 1 ].cint , u ) ;
  else {
      
    if ( mem [p ].hhfield .b0 != 18 ) 
    {
      if ( mem [p ].hhfield .b0 == 16 ) 
      newdep ( p , constdependency ( mem [p + 1 ].cint ) ) ;
      else mem [p + 1 ].hhfield .v.RH = ptimesv ( mem [p + 1 ].hhfield 
      .v.RH , 65536L , 17 , 18 , true ) ;
      mem [p ].hhfield .b0 = 18 ;
    } 
    mem [p + 1 ].hhfield .v.RH = pplusfq ( mem [p + 1 ].hhfield .v.RH , u 
    , mem [q + 1 ].hhfield .v.RH , 18 , mem [q ].hhfield .b0 ) ;
  } 
  if ( mem [p ].hhfield .b0 == 16 ) 
  mem [p + 1 ].cint = mem [p + 1 ].cint + delta ;
  else {
      
    r = mem [p + 1 ].hhfield .v.RH ;
    while ( mem [r ].hhfield .lhfield != 0 ) r = mem [r ].hhfield .v.RH ;
    delta = mem [r + 1 ].cint + delta ;
    if ( r != mem [p + 1 ].hhfield .v.RH ) 
    mem [r + 1 ].cint = delta ;
    else {
	
      recyclevalue ( p ) ;
      mem [p ].hhfield .b0 = 16 ;
      mem [p + 1 ].cint = delta ;
    } 
  } 
  if ( fixneeded ) 
  fixdependencies () ;
} 
void 
#ifdef HAVE_PROTOTYPES
zaddmultdep ( halfword p , scaled v , halfword r ) 
#else
zaddmultdep ( p , v , r ) 
  halfword p ;
  scaled v ;
  halfword r ;
#endif
{
  if ( mem [r ].hhfield .b0 == 16 ) 
  mem [depfinal + 1 ].cint = mem [depfinal + 1 ].cint + takescaled ( mem [
  r + 1 ].cint , v ) ;
  else {
      
    mem [p + 1 ].hhfield .v.RH = pplusfq ( mem [p + 1 ].hhfield .v.RH , v 
    , mem [r + 1 ].hhfield .v.RH , 18 , mem [r ].hhfield .b0 ) ;
    if ( fixneeded ) 
    fixdependencies () ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zbilin2 ( halfword p , halfword t , scaled v , halfword u , halfword q ) 
#else
zbilin2 ( p , t , v , u , q ) 
  halfword p ;
  halfword t ;
  scaled v ;
  halfword u ;
  halfword q ;
#endif
{
  scaled vv  ;
  vv = mem [p + 1 ].cint ;
  mem [p ].hhfield .b0 = 18 ;
  newdep ( p , constdependency ( 0 ) ) ;
  if ( vv != 0 ) 
  addmultdep ( p , vv , t ) ;
  if ( v != 0 ) 
  addmultdep ( p , v , u ) ;
  if ( q != 0 ) 
  addmultdep ( p , 65536L , q ) ;
  if ( mem [p + 1 ].hhfield .v.RH == depfinal ) 
  {
    vv = mem [depfinal + 1 ].cint ;
    recyclevalue ( p ) ;
    mem [p ].hhfield .b0 = 16 ;
    mem [p + 1 ].cint = vv ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zbilin3 ( halfword p , scaled t , scaled v , scaled u , scaled delta ) 
#else
zbilin3 ( p , t , v , u , delta ) 
  halfword p ;
  scaled t ;
  scaled v ;
  scaled u ;
  scaled delta ;
#endif
{
  if ( t != 65536L ) 
  delta = delta + takescaled ( mem [p + 1 ].cint , t ) ;
  else delta = delta + mem [p + 1 ].cint ;
  if ( u != 0 ) 
  mem [p + 1 ].cint = delta + takescaled ( v , u ) ;
  else mem [p + 1 ].cint = delta ;
} 
void 
#ifdef HAVE_PROTOTYPES
zbigtrans ( halfword p , quarterword c ) 
#else
zbigtrans ( p , c ) 
  halfword p ;
  quarterword c ;
#endif
{
  /* 10 */ halfword q, r, pp, qq  ;
  smallnumber s  ;
  s = bignodesize [mem [p ].hhfield .b0 ];
  q = mem [p + 1 ].cint ;
  r = q + s ;
  do {
      r = r - 2 ;
    if ( mem [r ].hhfield .b0 != 16 ) 
    {
      setupknowntrans ( c ) ;
      makeexpcopy ( p ) ;
      r = mem [curexp + 1 ].cint ;
      if ( curtype == 12 ) 
      {
	bilin1 ( r + 10 , tyy , q + 6 , tyx , 0 ) ;
	bilin1 ( r + 8 , tyy , q + 4 , tyx , 0 ) ;
	bilin1 ( r + 6 , txx , q + 10 , txy , 0 ) ;
	bilin1 ( r + 4 , txx , q + 8 , txy , 0 ) ;
      } 
      bilin1 ( r + 2 , tyy , q , tyx , ty ) ;
      bilin1 ( r , txx , q + 2 , txy , tx ) ;
      goto lab10 ;
    } 
  } while ( ! ( r == q ) ) ;
  setuptrans ( c ) ;
  if ( curtype == 16 ) 
  {
    makeexpcopy ( p ) ;
    r = mem [curexp + 1 ].cint ;
    if ( curtype == 12 ) 
    {
      bilin3 ( r + 10 , tyy , mem [q + 7 ].cint , tyx , 0 ) ;
      bilin3 ( r + 8 , tyy , mem [q + 5 ].cint , tyx , 0 ) ;
      bilin3 ( r + 6 , txx , mem [q + 11 ].cint , txy , 0 ) ;
      bilin3 ( r + 4 , txx , mem [q + 9 ].cint , txy , 0 ) ;
    } 
    bilin3 ( r + 2 , tyy , mem [q + 1 ].cint , tyx , ty ) ;
    bilin3 ( r , txx , mem [q + 3 ].cint , txy , tx ) ;
  } 
  else {
      
    pp = stashcurexp () ;
    qq = mem [pp + 1 ].cint ;
    makeexpcopy ( p ) ;
    r = mem [curexp + 1 ].cint ;
    if ( curtype == 12 ) 
    {
      bilin2 ( r + 10 , qq + 10 , mem [q + 7 ].cint , qq + 8 , 0 ) ;
      bilin2 ( r + 8 , qq + 10 , mem [q + 5 ].cint , qq + 8 , 0 ) ;
      bilin2 ( r + 6 , qq + 4 , mem [q + 11 ].cint , qq + 6 , 0 ) ;
      bilin2 ( r + 4 , qq + 4 , mem [q + 9 ].cint , qq + 6 , 0 ) ;
    } 
    bilin2 ( r + 2 , qq + 10 , mem [q + 1 ].cint , qq + 8 , qq + 2 ) ;
    bilin2 ( r , qq + 4 , mem [q + 3 ].cint , qq + 6 , qq ) ;
    recyclevalue ( pp ) ;
    freenode ( pp , 2 ) ;
  } 
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zcat ( halfword p ) 
#else
zcat ( p ) 
  halfword p ;
#endif
{
  strnumber a, b  ;
  poolpointer k  ;
  a = mem [p + 1 ].cint ;
  b = curexp ;
  {
    if ( poolptr + ( strstart [nextstr [a ]]- strstart [a ]) + ( 
    strstart [nextstr [b ]]- strstart [b ]) > maxpoolptr ) 
    if ( poolptr + ( strstart [nextstr [a ]]- strstart [a ]) + ( 
    strstart [nextstr [b ]]- strstart [b ]) > poolsize ) 
    docompaction ( ( strstart [nextstr [a ]]- strstart [a ]) + ( 
    strstart [nextstr [b ]]- strstart [b ]) ) ;
    else maxpoolptr = poolptr + ( strstart [nextstr [a ]]- strstart [a ]
    ) + ( strstart [nextstr [b ]]- strstart [b ]) ;
  } 
  {register integer for_end; k = strstart [a ];for_end = strstart [
  nextstr [a ]]- 1 ; if ( k <= for_end) do 
    {
      strpool [poolptr ]= strpool [k ];
      incr ( poolptr ) ;
    } 
  while ( k++ < for_end ) ;} 
  {register integer for_end; k = strstart [b ];for_end = strstart [
  nextstr [b ]]- 1 ; if ( k <= for_end) do 
    {
      strpool [poolptr ]= strpool [k ];
      incr ( poolptr ) ;
    } 
  while ( k++ < for_end ) ;} 
  curexp = makestring () ;
  {
    if ( strref [b ]< 127 ) 
    if ( strref [b ]> 1 ) 
    decr ( strref [b ]) ;
    else flushstring ( b ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zchopstring ( halfword p ) 
#else
zchopstring ( p ) 
  halfword p ;
#endif
{
  integer a, b  ;
  integer l  ;
  integer k  ;
  strnumber s  ;
  boolean reversed  ;
  a = roundunscaled ( mem [p + 1 ].cint ) ;
  b = roundunscaled ( mem [p + 3 ].cint ) ;
  if ( a <= b ) 
  reversed = false ;
  else {
      
    reversed = true ;
    k = a ;
    a = b ;
    b = k ;
  } 
  s = curexp ;
  l = ( strstart [nextstr [s ]]- strstart [s ]) ;
  if ( a < 0 ) 
  {
    a = 0 ;
    if ( b < 0 ) 
    b = 0 ;
  } 
  if ( b > l ) 
  {
    b = l ;
    if ( a > l ) 
    a = l ;
  } 
  {
    if ( poolptr + b - a > maxpoolptr ) 
    if ( poolptr + b - a > poolsize ) 
    docompaction ( b - a ) ;
    else maxpoolptr = poolptr + b - a ;
  } 
  if ( reversed ) 
  {register integer for_end; k = strstart [s ]+ b - 1 ;for_end = strstart 
  [s ]+ a ; if ( k >= for_end) do 
    {
      strpool [poolptr ]= strpool [k ];
      incr ( poolptr ) ;
    } 
  while ( k-- > for_end ) ;} 
  else {
      register integer for_end; k = strstart [s ]+ a ;for_end = strstart 
  [s ]+ b - 1 ; if ( k <= for_end) do 
    {
      strpool [poolptr ]= strpool [k ];
      incr ( poolptr ) ;
    } 
  while ( k++ < for_end ) ;} 
  curexp = makestring () ;
  {
    if ( strref [s ]< 127 ) 
    if ( strref [s ]> 1 ) 
    decr ( strref [s ]) ;
    else flushstring ( s ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zchoppath ( halfword p ) 
#else
zchoppath ( p ) 
  halfword p ;
#endif
{
  halfword q  ;
  halfword pp, qq, rr, ss  ;
  scaled a, b, k, l  ;
  boolean reversed  ;
  l = pathlength () ;
  a = mem [p + 1 ].cint ;
  b = mem [p + 3 ].cint ;
  if ( a <= b ) 
  reversed = false ;
  else {
      
    reversed = true ;
    k = a ;
    a = b ;
    b = k ;
  } 
  if ( a < 0 ) 
  if ( mem [curexp ].hhfield .b0 == 0 ) 
  {
    a = 0 ;
    if ( b < 0 ) 
    b = 0 ;
  } 
  else do {
      a = a + l ;
    b = b + l ;
  } while ( ! ( a >= 0 ) ) ;
  if ( b > l ) 
  if ( mem [curexp ].hhfield .b0 == 0 ) 
  {
    b = l ;
    if ( a > l ) 
    a = l ;
  } 
  else while ( a >= l ) {
      
    a = a - l ;
    b = b - l ;
  } 
  q = curexp ;
  while ( a >= 65536L ) {
      
    q = mem [q ].hhfield .v.RH ;
    a = a - 65536L ;
    b = b - 65536L ;
  } 
  if ( b == a ) 
  {
    if ( a > 0 ) 
    {
      splitcubic ( q , a * 4096 ) ;
      q = mem [q ].hhfield .v.RH ;
    } 
    pp = copyknot ( q ) ;
    qq = pp ;
  } 
  else {
      
    pp = copyknot ( q ) ;
    qq = pp ;
    do {
	q = mem [q ].hhfield .v.RH ;
      rr = qq ;
      qq = copyknot ( q ) ;
      mem [rr ].hhfield .v.RH = qq ;
      b = b - 65536L ;
    } while ( ! ( b <= 0 ) ) ;
    if ( a > 0 ) 
    {
      ss = pp ;
      pp = mem [pp ].hhfield .v.RH ;
      splitcubic ( ss , a * 4096 ) ;
      pp = mem [ss ].hhfield .v.RH ;
      freenode ( ss , 7 ) ;
      if ( rr == ss ) 
      {
	b = makescaled ( b , 65536L - a ) ;
	rr = pp ;
      } 
    } 
    if ( b < 0 ) 
    {
      splitcubic ( rr , ( b + 65536L ) * 4096 ) ;
      freenode ( qq , 7 ) ;
      qq = mem [rr ].hhfield .v.RH ;
    } 
  } 
  mem [pp ].hhfield .b0 = 0 ;
  mem [qq ].hhfield .b1 = 0 ;
  mem [qq ].hhfield .v.RH = pp ;
  tossknotlist ( curexp ) ;
  if ( reversed ) 
  {
    curexp = mem [htapypoc ( pp ) ].hhfield .v.RH ;
    tossknotlist ( pp ) ;
  } 
  else curexp = pp ;
} 
void 
#ifdef HAVE_PROTOTYPES
zsetupoffset ( halfword p ) 
#else
zsetupoffset ( p ) 
  halfword p ;
#endif
{
  findoffset ( mem [p + 1 ].cint , mem [p + 3 ].cint , curexp ) ;
  pairvalue ( curx , cury ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zsetupdirectiontime ( halfword p ) 
#else
zsetupdirectiontime ( p ) 
  halfword p ;
#endif
{
  flushcurexp ( finddirectiontime ( mem [p + 1 ].cint , mem [p + 3 ]
  .cint , curexp ) ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zfindpoint ( scaled v , quarterword c ) 
#else
zfindpoint ( v , c ) 
  scaled v ;
  quarterword c ;
#endif
{
  halfword p  ;
  scaled n  ;
  p = curexp ;
  if ( mem [p ].hhfield .b0 == 0 ) 
  n = -65536L ;
  else n = 0 ;
  do {
      p = mem [p ].hhfield .v.RH ;
    n = n + 65536L ;
  } while ( ! ( p == curexp ) ) ;
  if ( n == 0 ) 
  v = 0 ;
  else if ( v < 0 ) 
  if ( mem [p ].hhfield .b0 == 0 ) 
  v = 0 ;
  else v = n - 1 - ( ( - (integer) v - 1 ) % n ) ;
  else if ( v > n ) 
  if ( mem [p ].hhfield .b0 == 0 ) 
  v = n ;
  else v = v % n ;
  p = curexp ;
  while ( v >= 65536L ) {
      
    p = mem [p ].hhfield .v.RH ;
    v = v - 65536L ;
  } 
  if ( v != 0 ) 
  {
    splitcubic ( p , v * 4096 ) ;
    p = mem [p ].hhfield .v.RH ;
  } 
  switch ( c ) 
  {case 118 : 
    pairvalue ( mem [p + 1 ].cint , mem [p + 2 ].cint ) ;
    break ;
  case 119 : 
    if ( mem [p ].hhfield .b0 == 0 ) 
    pairvalue ( mem [p + 1 ].cint , mem [p + 2 ].cint ) ;
    else pairvalue ( mem [p + 3 ].cint , mem [p + 4 ].cint ) ;
    break ;
  case 120 : 
    if ( mem [p ].hhfield .b1 == 0 ) 
    pairvalue ( mem [p + 1 ].cint , mem [p + 2 ].cint ) ;
    else pairvalue ( mem [p + 5 ].cint , mem [p + 6 ].cint ) ;
    break ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zdoinfont ( halfword p ) 
#else
zdoinfont ( p ) 
  halfword p ;
#endif
{
  halfword q  ;
  q = getnode ( 8 ) ;
  initedges ( q ) ;
  mem [mem [q + 7 ].hhfield .lhfield ].hhfield .v.RH = newtextnode ( 
  curexp , mem [p + 1 ].cint ) ;
  mem [q + 7 ].hhfield .lhfield = mem [mem [q + 7 ].hhfield .lhfield ]
  .hhfield .v.RH ;
  freenode ( p , 2 ) ;
  flushcurexp ( q ) ;
  curtype = 10 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdobinary ( halfword p , quarterword c ) 
#else
zdobinary ( p , c ) 
  halfword p ;
  quarterword c ;
#endif
{
  /* 30 31 10 */ halfword q, r, rr  ;
  halfword oldp, oldexp  ;
  integer v  ;
  {
    if ( aritherror ) 
    cleararith () ;
  } 
  if ( internal [6 ]> 131072L ) 
  {
    begindiagnostic () ;
    printnl ( 902 ) ;
    printexp ( p , 0 ) ;
    printchar ( 41 ) ;
    printop ( c ) ;
    printchar ( 40 ) ;
    printexp ( 0 , 0 ) ;
    print ( 891 ) ;
    enddiagnostic ( false ) ;
  } 
  switch ( mem [p ].hhfield .b0 ) 
  {case 12 : 
  case 13 : 
  case 14 : 
    oldp = tarnished ( p ) ;
    break ;
  case 19 : 
    oldp = 1 ;
    break ;
    default: 
    oldp = 0 ;
    break ;
  } 
  if ( oldp != 0 ) 
  {
    q = stashcurexp () ;
    oldp = p ;
    makeexpcopy ( oldp ) ;
    p = stashcurexp () ;
    unstashcurexp ( q ) ;
  } 
  switch ( curtype ) 
  {case 12 : 
  case 13 : 
  case 14 : 
    oldexp = tarnished ( curexp ) ;
    break ;
  case 19 : 
    oldexp = 1 ;
    break ;
    default: 
    oldexp = 0 ;
    break ;
  } 
  if ( oldexp != 0 ) 
  {
    oldexp = curexp ;
    makeexpcopy ( oldexp ) ;
  } 
  switch ( c ) 
  {case 89 : 
  case 90 : 
    if ( ( curtype < 13 ) || ( mem [p ].hhfield .b0 < 13 ) ) 
    badbinary ( p , c ) ;
    else if ( ( curtype > 14 ) && ( mem [p ].hhfield .b0 > 14 ) ) 
    addorsubtract ( p , 0 , c ) ;
    else if ( curtype != mem [p ].hhfield .b0 ) 
    badbinary ( p , c ) ;
    else {
	
      q = mem [p + 1 ].cint ;
      r = mem [curexp + 1 ].cint ;
      rr = r + bignodesize [curtype ];
      while ( r < rr ) {
	  
	addorsubtract ( q , r , c ) ;
	q = q + 2 ;
	r = r + 2 ;
      } 
    } 
    break ;
  case 97 : 
  case 98 : 
  case 99 : 
  case 100 : 
  case 101 : 
  case 102 : 
    {
      {
	if ( aritherror ) 
	cleararith () ;
      } 
      if ( ( curtype > 14 ) && ( mem [p ].hhfield .b0 > 14 ) ) 
      addorsubtract ( p , 0 , 90 ) ;
      else if ( curtype != mem [p ].hhfield .b0 ) 
      {
	badbinary ( p , c ) ;
	goto lab30 ;
      } 
      else if ( curtype == 4 ) 
      flushcurexp ( strvsstr ( mem [p + 1 ].cint , curexp ) ) ;
      else if ( ( curtype == 5 ) || ( curtype == 3 ) ) 
      {
	q = mem [curexp + 1 ].cint ;
	while ( ( q != curexp ) && ( q != p ) ) q = mem [q + 1 ].cint ;
	if ( q == p ) 
	flushcurexp ( 0 ) ;
      } 
      else if ( ( curtype <= 14 ) && ( curtype >= 12 ) ) 
      {
	q = mem [p + 1 ].cint ;
	r = mem [curexp + 1 ].cint ;
	rr = r + bignodesize [curtype ]- 2 ;
	while ( true ) {
	    
	  addorsubtract ( q , r , 90 ) ;
	  if ( mem [r ].hhfield .b0 != 16 ) 
	  goto lab31 ;
	  if ( mem [r + 1 ].cint != 0 ) 
	  goto lab31 ;
	  if ( r == rr ) 
	  goto lab31 ;
	  q = q + 2 ;
	  r = r + 2 ;
	} 
	lab31: takepart ( mem [r ].hhfield .b1 + 49 ) ;
      } 
      else if ( curtype == 2 ) 
      flushcurexp ( curexp - mem [p + 1 ].cint ) ;
      else {
	  
	badbinary ( p , c ) ;
	goto lab30 ;
      } 
      if ( curtype != 16 ) 
      {
	if ( curtype < 16 ) 
	{
	  disperr ( p , 284 ) ;
	  {
	    helpptr = 1 ;
	    helpline [0 ]= 903 ;
	  } 
	} 
	else {
	    
	  helpptr = 2 ;
	  helpline [1 ]= 904 ;
	  helpline [0 ]= 905 ;
	} 
	disperr ( 0 , 906 ) ;
	putgetflusherror ( 31 ) ;
      } 
      else switch ( c ) 
      {case 97 : 
	if ( curexp < 0 ) 
	curexp = 30 ;
	else curexp = 31 ;
	break ;
      case 98 : 
	if ( curexp <= 0 ) 
	curexp = 30 ;
	else curexp = 31 ;
	break ;
      case 99 : 
	if ( curexp > 0 ) 
	curexp = 30 ;
	else curexp = 31 ;
	break ;
      case 100 : 
	if ( curexp >= 0 ) 
	curexp = 30 ;
	else curexp = 31 ;
	break ;
      case 101 : 
	if ( curexp == 0 ) 
	curexp = 30 ;
	else curexp = 31 ;
	break ;
      case 102 : 
	if ( curexp != 0 ) 
	curexp = 30 ;
	else curexp = 31 ;
	break ;
      } 
      curtype = 2 ;
      lab30: aritherror = false ;
    } 
    break ;
  case 96 : 
  case 95 : 
    if ( ( mem [p ].hhfield .b0 != 2 ) || ( curtype != 2 ) ) 
    badbinary ( p , c ) ;
    else if ( mem [p + 1 ].cint == c - 65 ) 
    curexp = mem [p + 1 ].cint ;
    break ;
  case 91 : 
    if ( ( curtype < 13 ) || ( mem [p ].hhfield .b0 < 13 ) ) 
    badbinary ( p , 91 ) ;
    else if ( ( curtype == 16 ) || ( mem [p ].hhfield .b0 == 16 ) ) 
    {
      if ( mem [p ].hhfield .b0 == 16 ) 
      {
	v = mem [p + 1 ].cint ;
	freenode ( p , 2 ) ;
      } 
      else {
	  
	v = curexp ;
	unstashcurexp ( p ) ;
      } 
      if ( curtype == 16 ) 
      curexp = takescaled ( curexp , v ) ;
      else if ( ( curtype == 14 ) || ( curtype == 13 ) ) 
      {
	p = mem [curexp + 1 ].cint + bignodesize [curtype ];
	do {
	    p = p - 2 ;
	  depmult ( p , v , true ) ;
	} while ( ! ( p == mem [curexp + 1 ].cint ) ) ;
      } 
      else depmult ( 0 , v , true ) ;
      goto lab10 ;
    } 
    else if ( ( nicecolororpair ( p , mem [p ].hhfield .b0 ) && ( curtype > 
    14 ) ) || ( nicecolororpair ( curexp , curtype ) && ( mem [p ].hhfield 
    .b0 > 14 ) ) ) 
    {
      hardtimes ( p ) ;
      goto lab10 ;
    } 
    else badbinary ( p , 91 ) ;
    break ;
  case 92 : 
    if ( ( curtype != 16 ) || ( mem [p ].hhfield .b0 < 13 ) ) 
    badbinary ( p , 92 ) ;
    else {
	
      v = curexp ;
      unstashcurexp ( p ) ;
      if ( v == 0 ) 
      {
	disperr ( 0 , 835 ) ;
	{
	  helpptr = 2 ;
	  helpline [1 ]= 908 ;
	  helpline [0 ]= 909 ;
	} 
	putgeterror () ;
      } 
      else {
	  
	if ( curtype == 16 ) 
	curexp = makescaled ( curexp , v ) ;
	else if ( curtype <= 14 ) 
	{
	  p = mem [curexp + 1 ].cint + bignodesize [curtype ];
	  do {
	      p = p - 2 ;
	    depdiv ( p , v ) ;
	  } while ( ! ( p == mem [curexp + 1 ].cint ) ) ;
	} 
	else depdiv ( 0 , v ) ;
      } 
      goto lab10 ;
    } 
    break ;
  case 93 : 
  case 94 : 
    if ( ( curtype == 16 ) && ( mem [p ].hhfield .b0 == 16 ) ) 
    if ( c == 93 ) 
    curexp = pythadd ( mem [p + 1 ].cint , curexp ) ;
    else curexp = pythsub ( mem [p + 1 ].cint , curexp ) ;
    else badbinary ( p , c ) ;
    break ;
  case 104 : 
  case 105 : 
  case 106 : 
  case 107 : 
  case 108 : 
  case 109 : 
  case 110 : 
  case 111 : 
    if ( mem [p ].hhfield .b0 == 8 ) 
    {
      {
	setupknowntrans ( c ) ;
	unstashcurexp ( p ) ;
	dopathtrans ( curexp ) ;
      } 
      goto lab10 ;
    } 
    else if ( mem [p ].hhfield .b0 == 6 ) 
    {
      {
	setupknowntrans ( c ) ;
	unstashcurexp ( p ) ;
	dopentrans ( curexp ) ;
      } 
      curexp = convexhull ( curexp ) ;
      goto lab10 ;
    } 
    else if ( ( mem [p ].hhfield .b0 == 14 ) || ( mem [p ].hhfield .b0 == 
    12 ) ) 
    bigtrans ( p , c ) ;
    else if ( mem [p ].hhfield .b0 == 10 ) 
    {
      doedgestrans ( p , c ) ;
      goto lab10 ;
    } 
    else badbinary ( p , c ) ;
    break ;
  case 103 : 
    if ( ( curtype == 4 ) && ( mem [p ].hhfield .b0 == 4 ) ) 
    cat ( p ) ;
    else badbinary ( p , 103 ) ;
    break ;
  case 115 : 
    if ( nicepair ( p , mem [p ].hhfield .b0 ) && ( curtype == 4 ) ) 
    chopstring ( mem [p + 1 ].cint ) ;
    else badbinary ( p , 115 ) ;
    break ;
  case 116 : 
    {
      if ( curtype == 14 ) 
      pairtopath () ;
      if ( nicepair ( p , mem [p ].hhfield .b0 ) && ( curtype == 8 ) ) 
      choppath ( mem [p + 1 ].cint ) ;
      else badbinary ( p , 116 ) ;
    } 
    break ;
  case 118 : 
  case 119 : 
  case 120 : 
    {
      if ( curtype == 14 ) 
      pairtopath () ;
      if ( ( curtype == 8 ) && ( mem [p ].hhfield .b0 == 16 ) ) 
      findpoint ( mem [p + 1 ].cint , c ) ;
      else badbinary ( p , c ) ;
    } 
    break ;
  case 121 : 
    if ( ( curtype == 6 ) && nicepair ( p , mem [p ].hhfield .b0 ) ) 
    setupoffset ( mem [p + 1 ].cint ) ;
    else badbinary ( p , 121 ) ;
    break ;
  case 117 : 
    {
      if ( curtype == 14 ) 
      pairtopath () ;
      if ( ( curtype == 8 ) && nicepair ( p , mem [p ].hhfield .b0 ) ) 
      setupdirectiontime ( mem [p + 1 ].cint ) ;
      else badbinary ( p , 117 ) ;
    } 
    break ;
  case 122 : 
    {
      if ( curtype == 14 ) 
      pairtopath () ;
      if ( ( curtype == 8 ) && ( mem [p ].hhfield .b0 == 16 ) ) 
      flushcurexp ( getarctime ( curexp , mem [p + 1 ].cint ) ) ;
      else badbinary ( p , c ) ;
    } 
    break ;
  case 113 : 
    {
      if ( mem [p ].hhfield .b0 == 14 ) 
      {
	q = stashcurexp () ;
	unstashcurexp ( p ) ;
	pairtopath () ;
	p = stashcurexp () ;
	unstashcurexp ( q ) ;
      } 
      if ( curtype == 14 ) 
      pairtopath () ;
      if ( ( curtype == 8 ) && ( mem [p ].hhfield .b0 == 8 ) ) 
      {
	pathintersection ( mem [p + 1 ].cint , curexp ) ;
	pairvalue ( curt , curtt ) ;
      } 
      else badbinary ( p , 113 ) ;
    } 
    break ;
  case 112 : 
    if ( ( curtype != 4 ) || ( mem [p ].hhfield .b0 != 4 ) ) 
    badbinary ( p , 112 ) ;
    else {
	
      doinfont ( p ) ;
      goto lab10 ;
    } 
    break ;
  } 
  recyclevalue ( p ) ;
  freenode ( p , 2 ) ;
  lab10: {
      
    if ( aritherror ) 
    cleararith () ;
  } 
  if ( oldp != 0 ) 
  {
    recyclevalue ( oldp ) ;
    freenode ( oldp , 2 ) ;
  } 
  if ( oldexp != 0 ) 
  {
    recyclevalue ( oldexp ) ;
    freenode ( oldexp , 2 ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zfracmult ( scaled n , scaled d ) 
#else
zfracmult ( n , d ) 
  scaled n ;
  scaled d ;
#endif
{
  halfword p  ;
  halfword oldexp  ;
  fraction v  ;
  if ( internal [6 ]> 131072L ) 
  {
    begindiagnostic () ;
    printnl ( 902 ) ;
    printscaled ( n ) ;
    printchar ( 47 ) ;
    printscaled ( d ) ;
    print ( 907 ) ;
    printexp ( 0 , 0 ) ;
    print ( 891 ) ;
    enddiagnostic ( false ) ;
  } 
  switch ( curtype ) 
  {case 12 : 
  case 13 : 
  case 14 : 
    oldexp = tarnished ( curexp ) ;
    break ;
  case 19 : 
    oldexp = 1 ;
    break ;
    default: 
    oldexp = 0 ;
    break ;
  } 
  if ( oldexp != 0 ) 
  {
    oldexp = curexp ;
    makeexpcopy ( oldexp ) ;
  } 
  v = makefraction ( n , d ) ;
  if ( curtype == 16 ) 
  curexp = takefraction ( curexp , v ) ;
  else if ( curtype <= 14 ) 
  {
    p = mem [curexp + 1 ].cint + bignodesize [curtype ];
    do {
	p = p - 2 ;
      depmult ( p , v , false ) ;
    } while ( ! ( p == mem [curexp + 1 ].cint ) ) ;
  } 
  else depmult ( 0 , v , false ) ;
  if ( oldexp != 0 ) 
  {
    recyclevalue ( oldexp ) ;
    freenode ( oldexp , 2 ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
ztryeq ( halfword l , halfword r ) 
#else
ztryeq ( l , r ) 
  halfword l ;
  halfword r ;
#endif
{
  /* 30 31 */ halfword p  ;
  char t  ;
  halfword q  ;
  halfword pp  ;
  char tt  ;
  boolean copied  ;
  t = mem [l ].hhfield .b0 ;
  if ( t == 16 ) 
  {
    t = 17 ;
    p = constdependency ( - (integer) mem [l + 1 ].cint ) ;
    q = p ;
  } 
  else if ( t == 19 ) 
  {
    t = 17 ;
    p = singledependency ( l ) ;
    mem [p + 1 ].cint = - (integer) mem [p + 1 ].cint ;
    q = depfinal ;
  } 
  else {
      
    p = mem [l + 1 ].hhfield .v.RH ;
    q = p ;
    while ( true ) {
	
      mem [q + 1 ].cint = - (integer) mem [q + 1 ].cint ;
      if ( mem [q ].hhfield .lhfield == 0 ) 
      goto lab30 ;
      q = mem [q ].hhfield .v.RH ;
    } 
    lab30: mem [mem [l + 1 ].hhfield .lhfield ].hhfield .v.RH = mem [q ]
    .hhfield .v.RH ;
    mem [mem [q ].hhfield .v.RH + 1 ].hhfield .lhfield = mem [l + 1 ]
    .hhfield .lhfield ;
    mem [l ].hhfield .b0 = 16 ;
  } 
  if ( r == 0 ) 
  if ( curtype == 16 ) 
  {
    mem [q + 1 ].cint = mem [q + 1 ].cint + curexp ;
    goto lab31 ;
  } 
  else {
      
    tt = curtype ;
    if ( tt == 19 ) 
    pp = singledependency ( curexp ) ;
    else pp = mem [curexp + 1 ].hhfield .v.RH ;
  } 
  else if ( mem [r ].hhfield .b0 == 16 ) 
  {
    mem [q + 1 ].cint = mem [q + 1 ].cint + mem [r + 1 ].cint ;
    goto lab31 ;
  } 
  else {
      
    tt = mem [r ].hhfield .b0 ;
    if ( tt == 19 ) 
    pp = singledependency ( r ) ;
    else pp = mem [r + 1 ].hhfield .v.RH ;
  } 
  if ( tt != 19 ) 
  copied = false ;
  else {
      
    copied = true ;
    tt = 17 ;
  } 
  watchcoefs = false ;
  if ( t == tt ) 
  p = pplusq ( p , pp , t ) ;
  else if ( t == 18 ) 
  p = pplusfq ( p , 65536L , pp , 18 , 17 ) ;
  else {
      
    q = p ;
    while ( mem [q ].hhfield .lhfield != 0 ) {
	
      mem [q + 1 ].cint = roundfraction ( mem [q + 1 ].cint ) ;
      q = mem [q ].hhfield .v.RH ;
    } 
    t = 18 ;
    p = pplusq ( p , pp , t ) ;
  } 
  watchcoefs = true ;
  if ( copied ) 
  flushnodelist ( pp ) ;
  lab31: ;
  if ( mem [p ].hhfield .lhfield == 0 ) 
  {
    if ( abs ( mem [p + 1 ].cint ) > 64 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 945 ) ;
      } 
      print ( 947 ) ;
      printscaled ( mem [p + 1 ].cint ) ;
      printchar ( 41 ) ;
      {
	helpptr = 2 ;
	helpline [1 ]= 946 ;
	helpline [0 ]= 944 ;
      } 
      putgeterror () ;
    } 
    else if ( r == 0 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 611 ) ;
      } 
      {
	helpptr = 2 ;
	helpline [1 ]= 612 ;
	helpline [0 ]= 613 ;
      } 
      putgeterror () ;
    } 
    freenode ( p , 2 ) ;
  } 
  else {
      
    lineareq ( p , t ) ;
    if ( r == 0 ) 
    if ( curtype != 16 ) 
    if ( mem [curexp ].hhfield .b0 == 16 ) 
    {
      pp = curexp ;
      curexp = mem [curexp + 1 ].cint ;
      curtype = 16 ;
      freenode ( pp , 2 ) ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zmakeeq ( halfword lhs ) 
#else
zmakeeq ( lhs ) 
  halfword lhs ;
#endif
{
  /* 20 30 45 */ smallnumber t  ;
  integer v  ;
  halfword p, q  ;
  lab20: t = mem [lhs ].hhfield .b0 ;
  if ( t <= 14 ) 
  v = mem [lhs + 1 ].cint ;
  switch ( t ) 
  {case 2 : 
  case 4 : 
  case 6 : 
  case 8 : 
  case 10 : 
    if ( curtype == t + 1 ) 
    {
      nonlineareq ( v , curexp , false ) ;
      goto lab30 ;
    } 
    else if ( curtype == t ) 
    {
      if ( curtype <= 4 ) 
      {
	if ( curtype == 4 ) 
	{
	  if ( strvsstr ( v , curexp ) != 0 ) 
	  goto lab45 ;
	} 
	else if ( v != curexp ) 
	goto lab45 ;
	{
	  {
	    if ( interaction == 3 ) 
	    ;
	    printnl ( 262 ) ;
	    print ( 611 ) ;
	  } 
	  {
	    helpptr = 2 ;
	    helpline [1 ]= 612 ;
	    helpline [0 ]= 613 ;
	  } 
	  putgeterror () ;
	} 
	goto lab30 ;
      } 
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 942 ) ;
      } 
      {
	helpptr = 2 ;
	helpline [1 ]= 943 ;
	helpline [0 ]= 944 ;
      } 
      putgeterror () ;
      goto lab30 ;
      lab45: {
	  
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 945 ) ;
      } 
      {
	helpptr = 2 ;
	helpline [1 ]= 946 ;
	helpline [0 ]= 944 ;
      } 
      putgeterror () ;
      goto lab30 ;
    } 
    break ;
  case 3 : 
  case 5 : 
  case 7 : 
  case 11 : 
  case 9 : 
    if ( curtype == t - 1 ) 
    {
      nonlineareq ( curexp , lhs , true ) ;
      goto lab30 ;
    } 
    else if ( curtype == t ) 
    {
      ringmerge ( lhs , curexp ) ;
      goto lab30 ;
    } 
    else if ( curtype == 14 ) 
    if ( t == 9 ) 
    {
      pairtopath () ;
      goto lab20 ;
    } 
    break ;
  case 12 : 
  case 13 : 
  case 14 : 
    if ( curtype == t ) 
    {
      p = v + bignodesize [t ];
      q = mem [curexp + 1 ].cint + bignodesize [t ];
      do {
	  p = p - 2 ;
	q = q - 2 ;
	tryeq ( p , q ) ;
      } while ( ! ( p == v ) ) ;
      goto lab30 ;
    } 
    break ;
  case 16 : 
  case 17 : 
  case 18 : 
  case 19 : 
    if ( curtype >= 16 ) 
    {
      tryeq ( lhs , 0 ) ;
      goto lab30 ;
    } 
    break ;
  case 1 : 
    ;
    break ;
  } 
  disperr ( lhs , 284 ) ;
  disperr ( 0 , 939 ) ;
  if ( mem [lhs ].hhfield .b0 <= 14 ) 
  printtype ( mem [lhs ].hhfield .b0 ) ;
  else print ( 340 ) ;
  printchar ( 61 ) ;
  if ( curtype <= 14 ) 
  printtype ( curtype ) ;
  else print ( 340 ) ;
  printchar ( 41 ) ;
  {
    helpptr = 2 ;
    helpline [1 ]= 940 ;
    helpline [0 ]= 941 ;
  } 
  putgeterror () ;
  lab30: {
      
    if ( aritherror ) 
    cleararith () ;
  } 
  recyclevalue ( lhs ) ;
  freenode ( lhs , 2 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
doequation ( void ) 
#else
doequation ( ) 
#endif
{
  halfword lhs  ;
  halfword p  ;
  lhs = stashcurexp () ;
  getxnext () ;
  varflag = 76 ;
  scanexpression () ;
  if ( curcmd == 53 ) 
  doequation () ;
  else if ( curcmd == 76 ) 
  doassignment () ;
  if ( internal [6 ]> 131072L ) 
  {
    begindiagnostic () ;
    printnl ( 902 ) ;
    printexp ( lhs , 0 ) ;
    print ( 934 ) ;
    printexp ( 0 , 0 ) ;
    print ( 891 ) ;
    enddiagnostic ( false ) ;
  } 
  if ( curtype == 9 ) 
  if ( mem [lhs ].hhfield .b0 == 14 ) 
  {
    p = stashcurexp () ;
    unstashcurexp ( lhs ) ;
    lhs = p ;
  } 
  makeeq ( lhs ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
doassignment ( void ) 
#else
doassignment ( ) 
#endif
{
  halfword lhs  ;
  halfword p  ;
  halfword q  ;
  if ( curtype != 20 ) 
  {
    disperr ( 0 , 931 ) ;
    {
      helpptr = 2 ;
      helpline [1 ]= 932 ;
      helpline [0 ]= 933 ;
    } 
    error () ;
    doequation () ;
  } 
  else {
      
    lhs = curexp ;
    curtype = 1 ;
    getxnext () ;
    varflag = 76 ;
    scanexpression () ;
    if ( curcmd == 53 ) 
    doequation () ;
    else if ( curcmd == 76 ) 
    doassignment () ;
    if ( internal [6 ]> 131072L ) 
    {
      begindiagnostic () ;
      printnl ( 123 ) ;
      if ( mem [lhs ].hhfield .lhfield > 9771 ) 
      print ( intname [mem [lhs ].hhfield .lhfield - ( 9771 ) ]) ;
      else showtokenlist ( lhs , 0 , 1000 , 0 ) ;
      print ( 476 ) ;
      printexp ( 0 , 0 ) ;
      printchar ( 125 ) ;
      enddiagnostic ( false ) ;
    } 
    if ( mem [lhs ].hhfield .lhfield > 9771 ) 
    if ( curtype == 16 ) 
    internal [mem [lhs ].hhfield .lhfield - ( 9771 ) ]= curexp ;
    else {
	
      disperr ( 0 , 935 ) ;
      print ( intname [mem [lhs ].hhfield .lhfield - ( 9771 ) ]) ;
      print ( 936 ) ;
      {
	helpptr = 2 ;
	helpline [1 ]= 937 ;
	helpline [0 ]= 938 ;
      } 
      putgeterror () ;
    } 
    else {
	
      p = findvariable ( lhs ) ;
      if ( p != 0 ) 
      {
	q = stashcurexp () ;
	curtype = undtype ( p ) ;
	recyclevalue ( p ) ;
	mem [p ].hhfield .b0 = curtype ;
	mem [p + 1 ].cint = 0 ;
	makeexpcopy ( p ) ;
	p = stashcurexp () ;
	unstashcurexp ( q ) ;
	makeeq ( p ) ;
      } 
      else {
	  
	obliterated ( lhs ) ;
	putgeterror () ;
      } 
    } 
    flushnodelist ( lhs ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
dotypedeclaration ( void ) 
#else
dotypedeclaration ( ) 
#endif
{
  smallnumber t  ;
  halfword p  ;
  halfword q  ;
  if ( curmod >= 12 ) 
  t = curmod ;
  else t = curmod + 1 ;
  do {
      p = scandeclaredvariable () ;
    flushvariable ( eqtb [mem [p ].hhfield .lhfield ].v.RH , mem [p ]
    .hhfield .v.RH , false ) ;
    q = findvariable ( p ) ;
    if ( q != 0 ) 
    {
      mem [q ].hhfield .b0 = t ;
      mem [q + 1 ].cint = 0 ;
    } 
    else {
	
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 948 ) ;
      } 
      {
	helpptr = 2 ;
	helpline [1 ]= 949 ;
	helpline [0 ]= 950 ;
      } 
      putgeterror () ;
    } 
    flushlist ( p ) ;
    if ( curcmd < 81 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 951 ) ;
      } 
      {
	helpptr = 5 ;
	helpline [4 ]= 952 ;
	helpline [3 ]= 953 ;
	helpline [2 ]= 954 ;
	helpline [1 ]= 955 ;
	helpline [0 ]= 956 ;
      } 
      if ( curcmd == 44 ) 
      helpline [2 ]= 957 ;
      putgeterror () ;
      scannerstatus = 2 ;
      do {
	  { 
	  getnext () ;
	  if ( curcmd <= 3 ) 
	  tnext () ;
	} 
	if ( curcmd == 41 ) 
	{
	  if ( strref [curmod ]< 127 ) 
	  if ( strref [curmod ]> 1 ) 
	  decr ( strref [curmod ]) ;
	  else flushstring ( curmod ) ;
	} 
      } while ( ! ( curcmd >= 81 ) ) ;
      scannerstatus = 0 ;
    } 
  } while ( ! ( curcmd > 81 ) ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
dorandomseed ( void ) 
#else
dorandomseed ( ) 
#endif
{
  getxnext () ;
  if ( curcmd != 76 ) 
  {
    missingerr ( 476 ) ;
    {
      helpptr = 1 ;
      helpline [0 ]= 962 ;
    } 
    backerror () ;
  } 
  getxnext () ;
  scanexpression () ;
  if ( curtype != 16 ) 
  {
    disperr ( 0 , 963 ) ;
    {
      helpptr = 2 ;
      helpline [1 ]= 964 ;
      helpline [0 ]= 965 ;
    } 
    putgetflusherror ( 0 ) ;
  } 
  else {
      
    initrandoms ( curexp ) ;
    if ( selector >= 9 ) 
    {
      oldsetting = selector ;
      selector = 9 ;
      printnl ( 966 ) ;
      printscaled ( curexp ) ;
      printchar ( 125 ) ;
      printnl ( 284 ) ;
      selector = oldsetting ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
doprotection ( void ) 
#else
doprotection ( ) 
#endif
{
  char m  ;
  halfword t  ;
  m = curmod ;
  do {
      getsymbol () ;
    t = eqtb [cursym ].lhfield ;
    if ( m == 0 ) 
    {
      if ( t >= 85 ) 
      eqtb [cursym ].lhfield = t - 85 ;
    } 
    else if ( t < 85 ) 
    eqtb [cursym ].lhfield = t + 85 ;
    getxnext () ;
  } while ( ! ( curcmd != 81 ) ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
defdelims ( void ) 
#else
defdelims ( ) 
#endif
{
  halfword ldelim, rdelim  ;
  getclearsymbol () ;
  ldelim = cursym ;
  getclearsymbol () ;
  rdelim = cursym ;
  eqtb [ldelim ].lhfield = 33 ;
  eqtb [ldelim ].v.RH = rdelim ;
  eqtb [rdelim ].lhfield = 64 ;
  eqtb [rdelim ].v.RH = ldelim ;
  getxnext () ;
} 
void 
#ifdef HAVE_PROTOTYPES
dointerim ( void ) 
#else
dointerim ( ) 
#endif
{
  getxnext () ;
  if ( curcmd != 42 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 972 ) ;
    } 
    if ( cursym == 0 ) 
    print ( 977 ) ;
    else print ( hash [cursym ].v.RH ) ;
    print ( 978 ) ;
    {
      helpptr = 1 ;
      helpline [0 ]= 979 ;
    } 
    backerror () ;
  } 
  else {
      
    saveinternal ( curmod ) ;
    backinput () ;
  } 
  dostatement () ;
} 
void 
#ifdef HAVE_PROTOTYPES
dolet ( void ) 
#else
dolet ( ) 
#endif
{
  halfword l  ;
  getsymbol () ;
  l = cursym ;
  getxnext () ;
  if ( curcmd != 53 ) 
  if ( curcmd != 76 ) 
  {
    missingerr ( 61 ) ;
    {
      helpptr = 3 ;
      helpline [2 ]= 980 ;
      helpline [1 ]= 713 ;
      helpline [0 ]= 981 ;
    } 
    backerror () ;
  } 
  getsymbol () ;
  switch ( curcmd ) 
  {case 13 : 
  case 55 : 
  case 46 : 
  case 51 : 
    incr ( mem [curmod ].hhfield .lhfield ) ;
    break ;
    default: 
    ;
    break ;
  } 
  clearsymbol ( l , false ) ;
  eqtb [l ].lhfield = curcmd ;
  if ( curcmd == 43 ) 
  eqtb [l ].v.RH = 0 ;
  else eqtb [l ].v.RH = curmod ;
  getxnext () ;
} 
void 
#ifdef HAVE_PROTOTYPES
donewinternal ( void ) 
#else
donewinternal ( ) 
#endif
{
  do {
      if ( intptr == maxinternal ) 
    overflow ( 982 , maxinternal ) ;
    getclearsymbol () ;
    incr ( intptr ) ;
    eqtb [cursym ].lhfield = 42 ;
    eqtb [cursym ].v.RH = intptr ;
    intname [intptr ]= hash [cursym ].v.RH ;
    internal [intptr ]= 0 ;
    getxnext () ;
  } while ( ! ( curcmd != 81 ) ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
doshow ( void ) 
#else
doshow ( ) 
#endif
{
  do {
      getxnext () ;
    scanexpression () ;
    printnl ( 804 ) ;
    printexp ( 0 , 2 ) ;
    flushcurexp ( 0 ) ;
  } while ( ! ( curcmd != 81 ) ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
disptoken ( void ) 
#else
disptoken ( ) 
#endif
{
  printnl ( 988 ) ;
  if ( cursym == 0 ) 
  {
    if ( curcmd == 44 ) 
    printscaled ( curmod ) ;
    else if ( curcmd == 40 ) 
    {
      gpointer = curmod ;
      printcapsule () ;
    } 
    else {
	
      printchar ( 34 ) ;
      print ( curmod ) ;
      printchar ( 34 ) ;
      {
	if ( strref [curmod ]< 127 ) 
	if ( strref [curmod ]> 1 ) 
	decr ( strref [curmod ]) ;
	else flushstring ( curmod ) ;
      } 
    } 
  } 
  else {
      
    print ( hash [cursym ].v.RH ) ;
    printchar ( 61 ) ;
    if ( eqtb [cursym ].lhfield >= 85 ) 
    print ( 989 ) ;
    printcmdmod ( curcmd , curmod ) ;
    if ( curcmd == 13 ) 
    {
      println () ;
      showmacro ( curmod , 0 , 100000L ) ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
doshowtoken ( void ) 
#else
doshowtoken ( ) 
#endif
{
  do {
      { 
      getnext () ;
      if ( curcmd <= 3 ) 
      tnext () ;
    } 
    disptoken () ;
    getxnext () ;
  } while ( ! ( curcmd != 81 ) ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
doshowstats ( void ) 
#else
doshowstats ( ) 
#endif
{
  printnl ( 998 ) ;
	;
#ifdef STAT
  printint ( varused ) ;
  printchar ( 38 ) ;
  printint ( dynused ) ;
  if ( false ) 
#endif /* STAT */
  print ( 359 ) ;
  print ( 999 ) ;
  printint ( himemmin - lomemmax - 1 ) ;
  print ( 1000 ) ;
  println () ;
  printnl ( 1001 ) ;
	;
#ifdef STAT
  printint ( strsinuse - initstruse ) ;
  printchar ( 38 ) ;
  printint ( poolinuse - initpoolptr ) ;
  if ( false ) 
#endif /* STAT */
  print ( 359 ) ;
  print ( 999 ) ;
  printint ( maxstrings - 1 - strsusedup ) ;
  printchar ( 38 ) ;
  printint ( poolsize - poolptr ) ;
  print ( 1002 ) ;
  println () ;
  getxnext () ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdispvar ( halfword p ) 
#else
zdispvar ( p ) 
  halfword p ;
#endif
{
  halfword q  ;
  integer n  ;
  if ( mem [p ].hhfield .b0 == 21 ) 
  {
    q = mem [p + 1 ].hhfield .lhfield ;
    do {
	dispvar ( q ) ;
      q = mem [q ].hhfield .v.RH ;
    } while ( ! ( q == 9 ) ) ;
    q = mem [p + 1 ].hhfield .v.RH ;
    while ( mem [q ].hhfield .b1 == 3 ) {
	
      dispvar ( q ) ;
      q = mem [q ].hhfield .v.RH ;
    } 
  } 
  else if ( mem [p ].hhfield .b0 >= 22 ) 
  {
    printnl ( 284 ) ;
    printvariablename ( p ) ;
    if ( mem [p ].hhfield .b0 > 22 ) 
    print ( 705 ) ;
    print ( 1003 ) ;
    if ( fileoffset >= maxprintline - 20 ) 
    n = 5 ;
    else n = maxprintline - fileoffset - 15 ;
    showmacro ( mem [p + 1 ].cint , 0 , n ) ;
  } 
  else if ( mem [p ].hhfield .b0 != 0 ) 
  {
    printnl ( 284 ) ;
    printvariablename ( p ) ;
    printchar ( 61 ) ;
    printexp ( p , 0 ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
doshowvar ( void ) 
#else
doshowvar ( ) 
#endif
{
  /* 30 */ do {
      { 
      getnext () ;
      if ( curcmd <= 3 ) 
      tnext () ;
    } 
    if ( cursym > 0 ) 
    if ( cursym <= 9771 ) 
    if ( curcmd == 43 ) 
    if ( curmod != 0 ) 
    {
      dispvar ( curmod ) ;
      goto lab30 ;
    } 
    disptoken () ;
    lab30: getxnext () ;
  } while ( ! ( curcmd != 81 ) ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
doshowdependencies ( void ) 
#else
doshowdependencies ( ) 
#endif
{
  halfword p  ;
  p = mem [5 ].hhfield .v.RH ;
  while ( p != 5 ) {
      
    if ( interesting ( p ) ) 
    {
      printnl ( 284 ) ;
      printvariablename ( p ) ;
      if ( mem [p ].hhfield .b0 == 17 ) 
      printchar ( 61 ) ;
      else print ( 817 ) ;
      printdependency ( mem [p + 1 ].hhfield .v.RH , mem [p ].hhfield .b0 
      ) ;
    } 
    p = mem [p + 1 ].hhfield .v.RH ;
    while ( mem [p ].hhfield .lhfield != 0 ) p = mem [p ].hhfield .v.RH ;
    p = mem [p ].hhfield .v.RH ;
  } 
  getxnext () ;
} 
void 
#ifdef HAVE_PROTOTYPES
doshowwhatever ( void ) 
#else
doshowwhatever ( ) 
#endif
{
  if ( interaction == 3 ) 
  ;
  switch ( curmod ) 
  {case 0 : 
    doshowtoken () ;
    break ;
  case 1 : 
    doshowstats () ;
    break ;
  case 2 : 
    doshow () ;
    break ;
  case 3 : 
    doshowvar () ;
    break ;
  case 4 : 
    doshowdependencies () ;
    break ;
  } 
  if ( internal [25 ]> 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 1004 ) ;
    } 
    if ( interaction < 3 ) 
    {
      helpptr = 0 ;
      decr ( errorcount ) ;
    } 
    else {
	
      helpptr = 1 ;
      helpline [0 ]= 1005 ;
    } 
    if ( curcmd == 82 ) 
    error () ;
    else putgeterror () ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zscanwithlist ( halfword p ) 
#else
zscanwithlist ( p ) 
  halfword p ;
#endif
{
  /* 30 31 32 */ smallnumber t  ;
  halfword q  ;
  halfword cp, pp, dp  ;
  cp = 1 ;
  pp = 1 ;
  dp = 1 ;
  while ( curcmd == 68 ) {
      
    t = curmod ;
    getxnext () ;
    scanexpression () ;
    if ( curtype != t ) 
    {
      disperr ( 0 , 1012 ) ;
      {
	helpptr = 2 ;
	helpline [1 ]= 1013 ;
	helpline [0 ]= 1014 ;
      } 
      if ( t == 10 ) 
      helpline [1 ]= 1015 ;
      else if ( t == 13 ) 
      helpline [1 ]= 1016 ;
      putgetflusherror ( 0 ) ;
    } 
    else if ( t == 13 ) 
    {
      if ( cp == 1 ) 
      {
	cp = p ;
	while ( cp != 0 ) {
	    
	  if ( ( mem [cp ].hhfield .b0 < 4 ) ) 
	  goto lab30 ;
	  cp = mem [cp ].hhfield .v.RH ;
	} 
	lab30: ;
      } 
      if ( cp != 0 ) 
      {
	q = mem [curexp + 1 ].cint ;
	mem [cp + 2 ].cint = mem [q + 1 ].cint ;
	mem [cp + 3 ].cint = mem [q + 3 ].cint ;
	mem [cp + 4 ].cint = mem [q + 5 ].cint ;
	if ( mem [cp + 2 ].cint < 0 ) 
	mem [cp + 2 ].cint = 0 ;
	if ( mem [cp + 3 ].cint < 0 ) 
	mem [cp + 3 ].cint = 0 ;
	if ( mem [cp + 4 ].cint < 0 ) 
	mem [cp + 4 ].cint = 0 ;
	if ( mem [cp + 2 ].cint > 65536L ) 
	mem [cp + 2 ].cint = 65536L ;
	if ( mem [cp + 3 ].cint > 65536L ) 
	mem [cp + 3 ].cint = 65536L ;
	if ( mem [cp + 4 ].cint > 65536L ) 
	mem [cp + 4 ].cint = 65536L ;
      } 
      flushcurexp ( 0 ) ;
    } 
    else if ( t == 6 ) 
    {
      if ( pp == 1 ) 
      {
	pp = p ;
	while ( pp != 0 ) {
	    
	  if ( ( mem [pp ].hhfield .b0 < 3 ) ) 
	  goto lab31 ;
	  pp = mem [pp ].hhfield .v.RH ;
	} 
	lab31: ;
      } 
      if ( pp != 0 ) 
      {
	if ( mem [pp + 1 ].hhfield .lhfield != 0 ) 
	tossknotlist ( mem [pp + 1 ].hhfield .lhfield ) ;
	mem [pp + 1 ].hhfield .lhfield = curexp ;
	curtype = 1 ;
      } 
    } 
    else {
	
      if ( dp == 1 ) 
      {
	dp = p ;
	while ( dp != 0 ) {
	    
	  if ( mem [dp ].hhfield .b0 == 2 ) 
	  goto lab32 ;
	  dp = mem [dp ].hhfield .v.RH ;
	} 
	lab32: ;
      } 
      if ( dp != 0 ) 
      {
	if ( mem [dp + 6 ].hhfield .v.RH != 0 ) 
	if ( mem [mem [dp + 6 ].hhfield .v.RH ].hhfield .lhfield == 0 ) 
	tossedges ( mem [dp + 6 ].hhfield .v.RH ) ;
	else decr ( mem [mem [dp + 6 ].hhfield .v.RH ].hhfield .lhfield ) 
	;
	mem [dp + 6 ].hhfield .v.RH = makedashes ( curexp ) ;
	mem [dp + 7 ].cint = 65536L ;
	curtype = 1 ;
      } 
    } 
  } 
  if ( cp > 1 ) 
  {
    q = mem [cp ].hhfield .v.RH ;
    while ( q != 0 ) {
	
      if ( ( mem [q ].hhfield .b0 < 4 ) ) 
      {
	mem [q + 2 ].cint = mem [cp + 2 ].cint ;
	mem [q + 3 ].cint = mem [cp + 3 ].cint ;
	mem [q + 4 ].cint = mem [cp + 4 ].cint ;
      } 
      q = mem [q ].hhfield .v.RH ;
    } 
  } 
  if ( pp > 1 ) 
  {
    q = mem [pp ].hhfield .v.RH ;
    while ( q != 0 ) {
	
      if ( ( mem [q ].hhfield .b0 < 3 ) ) 
      {
	if ( mem [q + 1 ].hhfield .lhfield != 0 ) 
	tossknotlist ( mem [q + 1 ].hhfield .lhfield ) ;
	mem [q + 1 ].hhfield .lhfield = makepen ( copypath ( mem [pp + 1 ]
	.hhfield .lhfield ) , false ) ;
      } 
      q = mem [q ].hhfield .v.RH ;
    } 
  } 
  if ( dp > 1 ) 
  {
    q = mem [dp ].hhfield .v.RH ;
    while ( q != 0 ) {
	
      if ( mem [q ].hhfield .b0 == 2 ) 
      {
	if ( mem [q + 6 ].hhfield .v.RH != 0 ) 
	if ( mem [mem [q + 6 ].hhfield .v.RH ].hhfield .lhfield == 0 ) 
	tossedges ( mem [q + 6 ].hhfield .v.RH ) ;
	else decr ( mem [mem [q + 6 ].hhfield .v.RH ].hhfield .lhfield ) ;
	mem [q + 6 ].hhfield .v.RH = mem [dp + 6 ].hhfield .v.RH ;
	mem [q + 7 ].cint = 65536L ;
	if ( mem [q + 6 ].hhfield .v.RH != 0 ) 
	incr ( mem [mem [q + 6 ].hhfield .v.RH ].hhfield .lhfield ) ;
      } 
      q = mem [q ].hhfield .v.RH ;
    } 
  } 
} 
halfword 
#ifdef HAVE_PROTOTYPES
zfindedgesvar ( halfword t ) 
#else
zfindedgesvar ( t ) 
  halfword t ;
#endif
{
  register halfword Result; halfword p  ;
  halfword curedges  ;
  p = findvariable ( t ) ;
  curedges = 0 ;
  if ( p == 0 ) 
  {
    obliterated ( t ) ;
    putgeterror () ;
  } 
  else if ( mem [p ].hhfield .b0 != 10 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 841 ) ;
    } 
    showtokenlist ( t , 0 , 1000 , 0 ) ;
    print ( 1017 ) ;
    printtype ( mem [p ].hhfield .b0 ) ;
    printchar ( 41 ) ;
    {
      helpptr = 2 ;
      helpline [1 ]= 1018 ;
      helpline [0 ]= 1019 ;
    } 
    putgeterror () ;
  } 
  else {
      
    mem [p + 1 ].cint = privateedges ( mem [p + 1 ].cint ) ;
    curedges = mem [p + 1 ].cint ;
  } 
  flushnodelist ( t ) ;
  Result = curedges ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zstartdrawcmd ( quarterword sep ) 
#else
zstartdrawcmd ( sep ) 
  quarterword sep ;
#endif
{
  register halfword Result; halfword lhv  ;
  quarterword addtype  ;
  lhv = 0 ;
  getxnext () ;
  varflag = sep ;
  scanprimary () ;
  if ( curtype != 20 ) 
  {
    disperr ( 0 , 1022 ) ;
    {
      helpptr = 4 ;
      helpline [3 ]= 1023 ;
      helpline [2 ]= 1024 ;
      helpline [1 ]= 1025 ;
      helpline [0 ]= 1019 ;
    } 
    putgetflusherror ( 0 ) ;
  } 
  else {
      
    lhv = curexp ;
    addtype = curmod ;
    curtype = 1 ;
    getxnext () ;
    scanexpression () ;
  } 
  lastaddtype = addtype ;
  Result = lhv ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
dobounds ( void ) 
#else
dobounds ( ) 
#endif
{
  halfword lhv, lhe  ;
  halfword p  ;
  integer m  ;
  m = curmod ;
  lhv = startdrawcmd ( 71 ) ;
  if ( lhv != 0 ) 
  {
    lhe = findedgesvar ( lhv ) ;
    if ( lhe == 0 ) 
    flushcurexp ( 0 ) ;
    else if ( curtype != 8 ) 
    {
      disperr ( 0 , 1026 ) ;
      {
	helpptr = 2 ;
	helpline [1 ]= 1027 ;
	helpline [0 ]= 1019 ;
      } 
      putgetflusherror ( 0 ) ;
    } 
    else if ( mem [curexp ].hhfield .b0 == 0 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 1028 ) ;
      } 
      {
	helpptr = 2 ;
	helpline [1 ]= 1029 ;
	helpline [0 ]= 1019 ;
      } 
      putgeterror () ;
    } 
    else {
	
      p = newboundsnode ( curexp , m ) ;
      mem [p ].hhfield .v.RH = mem [lhe + 7 ].hhfield .v.RH ;
      mem [lhe + 7 ].hhfield .v.RH = p ;
      if ( mem [lhe + 7 ].hhfield .lhfield == lhe + 7 ) 
      mem [lhe + 7 ].hhfield .lhfield = p ;
      p = getnode ( grobjectsize [( m + 2 ) ]) ;
      mem [p ].hhfield .b0 = ( m + 2 ) ;
      mem [mem [lhe + 7 ].hhfield .lhfield ].hhfield .v.RH = p ;
      mem [lhe + 7 ].hhfield .lhfield = p ;
      initbbox ( lhe ) ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
doaddto ( void ) 
#else
doaddto ( ) 
#endif
{
  halfword lhv, lhe  ;
  halfword p  ;
  halfword e  ;
  quarterword addtype  ;
  lhv = startdrawcmd ( 69 ) ;
  addtype = lastaddtype ;
  if ( lhv != 0 ) 
  {
    if ( addtype == 2 ) 
    {
      p = 0 ;
      e = 0 ;
      if ( curtype != 10 ) 
      {
	disperr ( 0 , 1030 ) ;
	{
	  helpptr = 2 ;
	  helpline [1 ]= 1031 ;
	  helpline [0 ]= 1019 ;
	} 
	putgetflusherror ( 0 ) ;
      } 
      else {
	  
	e = privateedges ( curexp ) ;
	curtype = 1 ;
	p = mem [e + 7 ].hhfield .v.RH ;
      } 
    } 
    else {
	
      e = 0 ;
      p = 0 ;
      if ( curtype == 14 ) 
      pairtopath () ;
      if ( curtype != 8 ) 
      {
	disperr ( 0 , 1030 ) ;
	{
	  helpptr = 2 ;
	  helpline [1 ]= 1027 ;
	  helpline [0 ]= 1019 ;
	} 
	putgetflusherror ( 0 ) ;
      } 
      else if ( addtype == 1 ) 
      if ( mem [curexp ].hhfield .b0 == 0 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 1028 ) ;
	} 
	{
	  helpptr = 2 ;
	  helpline [1 ]= 1029 ;
	  helpline [0 ]= 1019 ;
	} 
	putgeterror () ;
      } 
      else {
	  
	p = newfillnode ( curexp ) ;
	curtype = 1 ;
      } 
      else {
	  
	p = newstrokednode ( curexp ) ;
	curtype = 1 ;
      } 
    } 
    scanwithlist ( p ) ;
    lhe = findedgesvar ( lhv ) ;
    if ( lhe == 0 ) 
    {
      if ( ( e == 0 ) && ( p != 0 ) ) 
      e = tossgrobject ( p ) ;
      if ( e != 0 ) 
      if ( mem [e ].hhfield .lhfield == 0 ) 
      tossedges ( e ) ;
      else decr ( mem [e ].hhfield .lhfield ) ;
    } 
    else if ( addtype == 2 ) 
    if ( e != 0 ) 
    {
      if ( mem [e + 7 ].hhfield .v.RH != 0 ) 
      {
	mem [mem [lhe + 7 ].hhfield .lhfield ].hhfield .v.RH = mem [e + 7 
	].hhfield .v.RH ;
	mem [lhe + 7 ].hhfield .lhfield = mem [e + 7 ].hhfield .lhfield ;
	mem [e + 7 ].hhfield .lhfield = e + 7 ;
	mem [e + 7 ].hhfield .v.RH = 0 ;
	flushdashlist ( lhe ) ;
      } 
      tossedges ( e ) ;
    } 
    else ;
    else if ( p != 0 ) 
    {
      mem [mem [lhe + 7 ].hhfield .lhfield ].hhfield .v.RH = p ;
      mem [lhe + 7 ].hhfield .lhfield = p ;
      if ( addtype == 0 ) 
      if ( mem [p + 1 ].hhfield .lhfield == 0 ) 
      mem [p + 1 ].hhfield .lhfield = getpencircle ( 0 ) ;
    } 
  } 
} 
scaled 
#ifdef HAVE_PROTOTYPES
ztfmcheck ( smallnumber m ) 
#else
ztfmcheck ( m ) 
  smallnumber m ;
#endif
{
  register scaled Result; if ( abs ( internal [m ]) >= 134217728L ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 1048 ) ;
    } 
    print ( intname [m ]) ;
    print ( 1049 ) ;
    {
      helpptr = 1 ;
      helpline [0 ]= 1050 ;
    } 
    putgeterror () ;
    if ( internal [m ]> 0 ) 
    Result = 134217727L ;
    else Result = -134217727L ;
  } 
  else Result = internal [m ];
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
readpsnametable ( void ) 
#else
readpsnametable ( ) 
#endif
{
  /* 50 30 */ fontnumber k  ;
  integer lmax  ;
  integer j  ;
  ASCIIcode c  ;
  strnumber s  ;
  namelength = strlen ( pstabname ) ;
  nameoffile = xmalloc ( 1 + namelength + 1 ) ;
  strcpy ( (char*) nameoffile + 1 , pstabname ) ;
  if ( aopenin ( pstabfile , kpsedvipsconfigformat ) ) 
  {
    lmax = 0 ;
    {register integer for_end; k = lastpsfnum + 1 ;for_end = lastfnum ; if ( 
    k <= for_end) do 
      if ( ( strstart [nextstr [fontname [k ]]]- strstart [fontname [k 
      ]]) > lmax ) 
      lmax = ( strstart [nextstr [fontname [k ]]]- strstart [fontname [
      k ]]) ;
    while ( k++ < for_end ) ;} 
    while ( ! eof ( pstabfile ) ) {
	
      {
	if ( poolptr + lmax > maxpoolptr ) 
	if ( poolptr + lmax > poolsize ) 
	docompaction ( lmax ) ;
	else maxpoolptr = poolptr + lmax ;
      } 
      j = lmax ;
      while ( true ) {
	  
	if ( eoln ( pstabfile ) ) 
	if ( j == lmax ) 
	{
	  poolptr = strstart [strptr ];
	  goto lab50 ;
	} 
	else fatalerror ( 1113 ) ;
	read ( pstabfile , c ) ;
	if ( ( ( c == '%' ) || ( c == '*' ) || ( c == ';' ) || ( c == '#' ) ) 
	) 
	{
	  poolptr = strstart [strptr ];
	  goto lab50 ;
	} 
	if ( ( ( c == ' ' ) || ( c == 9 ) ) ) 
	goto lab30 ;
	decr ( j ) ;
	if ( j >= 0 ) 
	{
	  strpool [poolptr ]= xord [c ];
	  incr ( poolptr ) ;
	} 
	else {
	    
	  poolptr = strstart [strptr ];
	  goto lab50 ;
	} 
      } 
      lab30: s = makestring () ;
      {register integer for_end; k = lastpsfnum + 1 ;for_end = lastfnum 
      ; if ( k <= for_end) do 
	if ( strvsstr ( s , fontname [k ]) == 0 ) 
	{
	  flushstring ( s ) ;
	  j = 32 ;
	  {
	    if ( poolptr + j > maxpoolptr ) 
	    if ( poolptr + j > poolsize ) 
	    docompaction ( j ) ;
	    else maxpoolptr = poolptr + j ;
	  } 
	  do {
	      if ( eoln ( pstabfile ) ) 
	    fatalerror ( 1113 ) ;
	    read ( pstabfile , c ) ;
	  } while ( ! ( ( ( c != ' ' ) && ( c != 9 ) ) ) ) ;
	  do {
	      decr ( j ) ;
	    if ( j < 0 ) 
	    fatalerror ( 1113 ) ;
	    {
	      strpool [poolptr ]= xord [c ];
	      incr ( poolptr ) ;
	    } 
	    if ( eoln ( pstabfile ) ) 
	    c = ' ' ;
	    else read ( pstabfile , c ) ;
	  } while ( ! ( ( ( c == ' ' ) || ( c == 9 ) ) ) ) ;
	  {
	    if ( strref [fontpsname [k ]]< 127 ) 
	    if ( strref [fontpsname [k ]]> 1 ) 
	    decr ( strref [fontpsname [k ]]) ;
	    else flushstring ( fontpsname [k ]) ;
	  } 
	  fontpsname [k ]= makestring () ;
	  goto lab50 ;
	} 
      while ( k++ < for_end ) ;} 
      flushstring ( s ) ;
      lab50: readln ( pstabfile ) ;
    } 
    lastpsfnum = lastfnum ;
    aclose ( pstabfile ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
openoutputfile ( void ) 
#else
openoutputfile ( ) 
#endif
{
  integer c  ;
  char oldsetting  ;
  strnumber s  ;
  if ( jobname == 0 ) 
  openlogfile () ;
  c = roundunscaled ( internal [17 ]) ;
  if ( c < 0 ) 
  s = 1114 ;
  else {
      
    oldsetting = selector ;
    selector = 4 ;
    printchar ( 46 ) ;
    printint ( c ) ;
    s = makestring () ;
    selector = oldsetting ;
  } 
  packjobname ( s ) ;
  while ( ! aopenout ( psfile ) ) promptfilename ( 1115 , s ) ;
  {
    if ( strref [s ]< 127 ) 
    if ( strref [s ]> 1 ) 
    decr ( strref [s ]) ;
    else flushstring ( s ) ;
  } 
  if ( ( c < firstoutputcode ) && ( firstoutputcode >= 0 ) ) 
  {
    firstoutputcode = c ;
    {
      if ( strref [firstfilename ]< 127 ) 
      if ( strref [firstfilename ]> 1 ) 
      decr ( strref [firstfilename ]) ;
      else flushstring ( firstfilename ) ;
    } 
    firstfilename = amakenamestring ( psfile ) ;
  } 
  if ( c >= lastoutputcode ) 
  {
    lastoutputcode = c ;
    {
      if ( strref [lastfilename ]< 127 ) 
      if ( strref [lastfilename ]> 1 ) 
      decr ( strref [lastfilename ]) ;
      else flushstring ( lastfilename ) ;
    } 
    lastfilename = amakenamestring ( psfile ) ;
  } 
  if ( termoffset > maxprintline - 6 ) 
  println () ;
  else if ( ( termoffset > 0 ) || ( fileoffset > 0 ) ) 
  printchar ( 32 ) ;
  printchar ( 91 ) ;
  if ( c >= 0 ) 
  printint ( c ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zpspairout ( scaled x , scaled y ) 
#else
zpspairout ( x , y ) 
  scaled x ;
  scaled y ;
#endif
{
  if ( psoffset + 26 > maxprintline ) 
  println () ;
  printscaled ( x ) ;
  printchar ( 32 ) ;
  printscaled ( y ) ;
  printchar ( 32 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zpsprint ( strnumber s ) 
#else
zpsprint ( s ) 
  strnumber s ;
#endif
{
  if ( psoffset + ( strstart [nextstr [s ]]- strstart [s ]) > 
  maxprintline ) 
  println () ;
  print ( s ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zpspathout ( halfword h ) 
#else
zpspathout ( h ) 
  halfword h ;
#endif
{
  /* 10 */ halfword p, q  ;
  scaled d  ;
  boolean curved  ;
  if ( psoffset + 40 > maxprintline ) 
  println () ;
  if ( neednewpath ) 
  print ( 1118 ) ;
  neednewpath = true ;
  pspairout ( mem [h + 1 ].cint , mem [h + 2 ].cint ) ;
  print ( 1119 ) ;
  p = h ;
  do {
      if ( mem [p ].hhfield .b1 == 0 ) 
    {
      if ( p == h ) 
      psprint ( 1120 ) ;
      goto lab10 ;
    } 
    q = mem [p ].hhfield .v.RH ;
    curved = true ;
    if ( mem [p + 5 ].cint == mem [p + 1 ].cint ) 
    if ( mem [p + 6 ].cint == mem [p + 2 ].cint ) 
    if ( mem [q + 3 ].cint == mem [q + 1 ].cint ) 
    if ( mem [q + 4 ].cint == mem [q + 2 ].cint ) 
    curved = false ;
    d = mem [q + 3 ].cint - mem [p + 5 ].cint ;
    if ( abs ( mem [p + 5 ].cint - mem [p + 1 ].cint - d ) <= 131 ) 
    if ( abs ( mem [q + 1 ].cint - mem [q + 3 ].cint - d ) <= 131 ) 
    {
      d = mem [q + 4 ].cint - mem [p + 6 ].cint ;
      if ( abs ( mem [p + 6 ].cint - mem [p + 2 ].cint - d ) <= 131 ) 
      if ( abs ( mem [q + 2 ].cint - mem [q + 4 ].cint - d ) <= 131 ) 
      curved = false ;
    } 
    println () ;
    if ( curved ) 
    {
      pspairout ( mem [p + 5 ].cint , mem [p + 6 ].cint ) ;
      pspairout ( mem [q + 3 ].cint , mem [q + 4 ].cint ) ;
      pspairout ( mem [q + 1 ].cint , mem [q + 2 ].cint ) ;
      psprint ( 1122 ) ;
    } 
    else if ( q != h ) 
    {
      pspairout ( mem [q + 1 ].cint , mem [q + 2 ].cint ) ;
      psprint ( 1123 ) ;
    } 
    p = q ;
  } while ( ! ( p == h ) ) ;
  psprint ( 1121 ) ;
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
zunknowngraphicsstate ( scaled c ) 
#else
zunknowngraphicsstate ( c ) 
  scaled c ;
#endif
{
  gsred = c ;
  gsgreen = c ;
  gsblue = c ;
  gsljoin = 3 ;
  gslcap = 3 ;
  gsmiterlim = 0 ;
  gsdashp = 1 ;
  gsdashsc = 0 ;
  gswidth = -1 ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
zcoordrangeOK ( halfword h , smallnumber zoff , scaled dz ) 
#else
zcoordrangeOK ( h , zoff , dz ) 
  halfword h ;
  smallnumber zoff ;
  scaled dz ;
#endif
{
  /* 40 45 10 */ register boolean Result; halfword p  ;
  scaled zlo, zhi  ;
  scaled z  ;
  zlo = mem [h + zoff ].cint ;
  zhi = zlo ;
  p = h ;
  while ( mem [p ].hhfield .b1 != 0 ) {
      
    z = mem [p + zoff + 4 ].cint ;
    if ( z < zlo ) 
    zlo = z ;
    else if ( z > zhi ) 
    zhi = z ;
    if ( zhi - zlo > dz ) 
    goto lab40 ;
    p = mem [p ].hhfield .v.RH ;
    z = mem [p + zoff + 2 ].cint ;
    if ( z < zlo ) 
    zlo = z ;
    else if ( z > zhi ) 
    zhi = z ;
    if ( zhi - zlo > dz ) 
    goto lab40 ;
    z = mem [p + zoff ].cint ;
    if ( z < zlo ) 
    zlo = z ;
    else if ( z > zhi ) 
    zhi = z ;
    if ( zhi - zlo > dz ) 
    goto lab40 ;
    if ( p == h ) 
    goto lab45 ;
  } 
  lab45: Result = true ;
  goto lab10 ;
  lab40: Result = false ;
  lab10: ;
  return Result ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
zsamedashes ( halfword h , halfword hh ) 
#else
zsamedashes ( h , hh ) 
  halfword h ;
  halfword hh ;
#endif
{
  /* 30 */ register boolean Result; halfword p, pp  ;
  if ( h == hh ) 
  Result = true ;
  else if ( ( h <= 1 ) || ( hh <= 1 ) ) 
  Result = false ;
  else if ( mem [h + 1 ].cint != mem [hh + 1 ].cint ) 
  Result = false ;
  else {
      
    p = mem [h ].hhfield .v.RH ;
    pp = mem [hh ].hhfield .v.RH ;
    while ( ( p != 2 ) && ( pp != 2 ) ) if ( ( mem [p + 1 ].cint != mem [pp 
    + 1 ].cint ) || ( mem [p + 2 ].cint != mem [pp + 2 ].cint ) ) 
    goto lab30 ;
    else {
	
      p = mem [p ].hhfield .v.RH ;
      pp = mem [pp ].hhfield .v.RH ;
    } 
    lab30: Result = p == pp ;
  } 
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zfixgraphicsstate ( halfword p ) 
#else
zfixgraphicsstate ( p ) 
  halfword p ;
#endif
{
  halfword hh, pp  ;
  scaled wx, wy, ww  ;
  boolean adjwx  ;
  integer tx, ty  ;
  scaled scf  ;
  if ( ( mem [p ].hhfield .b0 < 4 ) ) 
  if ( ( gsred != mem [p + 2 ].cint ) || ( gsgreen != mem [p + 3 ].cint ) 
  || ( gsblue != mem [p + 4 ].cint ) ) 
  {
    gsred = mem [p + 2 ].cint ;
    gsgreen = mem [p + 3 ].cint ;
    gsblue = mem [p + 4 ].cint ;
    if ( ( gsred == gsgreen ) && ( gsgreen == gsblue ) ) 
    {
      if ( psoffset + 16 > maxprintline ) 
      println () ;
      printchar ( 32 ) ;
      printscaled ( gsred ) ;
      print ( 1127 ) ;
    } 
    else {
	
      if ( psoffset + 36 > maxprintline ) 
      println () ;
      printchar ( 32 ) ;
      printscaled ( gsred ) ;
      printchar ( 32 ) ;
      printscaled ( gsgreen ) ;
      printchar ( 32 ) ;
      printscaled ( gsblue ) ;
      print ( 1128 ) ;
    } 
  } 
  if ( ( mem [p ].hhfield .b0 == 1 ) || ( mem [p ].hhfield .b0 == 2 ) ) 
  if ( mem [p + 1 ].hhfield .lhfield != 0 ) 
  if ( ( mem [p + 1 ].hhfield .lhfield == mem [mem [p + 1 ].hhfield 
  .lhfield ].hhfield .v.RH ) ) 
  {
    pp = mem [p + 1 ].hhfield .lhfield ;
    if ( ( mem [pp + 5 ].cint == mem [pp + 1 ].cint ) && ( mem [pp + 4 ]
    .cint == mem [pp + 2 ].cint ) ) 
    {
      wx = abs ( mem [pp + 3 ].cint - mem [pp + 1 ].cint ) ;
      wy = abs ( mem [pp + 6 ].cint - mem [pp + 2 ].cint ) ;
    } 
    else {
	
      wx = pythadd ( mem [pp + 3 ].cint - mem [pp + 1 ].cint , mem [pp + 
      5 ].cint - mem [pp + 1 ].cint ) ;
      wy = pythadd ( mem [pp + 4 ].cint - mem [pp + 2 ].cint , mem [pp + 
      6 ].cint - mem [pp + 2 ].cint ) ;
    } 
    tx = 1 ;
    ty = 1 ;
    if ( coordrangeOK ( mem [p + 1 ].hhfield .v.RH , 2 , wy ) ) 
    tx = 10 ;
    else if ( coordrangeOK ( mem [p + 1 ].hhfield .v.RH , 1 , wx ) ) 
    ty = 10 ;
    if ( wy / ty >= wx / tx ) 
    {
      ww = wy ;
      adjwx = false ;
    } 
    else {
	
      ww = wx ;
      adjwx = true ;
    } 
    if ( ( ww != gswidth ) || ( adjwx != gsadjwx ) ) 
    {
      if ( adjwx ) 
      {
	if ( psoffset + 13 > maxprintline ) 
	println () ;
	printchar ( 32 ) ;
	printscaled ( ww ) ;
	psprint ( 1129 ) ;
      } 
      else {
	  
	if ( psoffset + 15 > maxprintline ) 
	println () ;
	print ( 1130 ) ;
	printscaled ( ww ) ;
	psprint ( 1131 ) ;
      } 
      gswidth = ww ;
      gsadjwx = adjwx ;
    } 
    if ( mem [p ].hhfield .b0 == 1 ) 
    hh = 0 ;
    else {
	
      hh = mem [p + 6 ].hhfield .v.RH ;
      scf = getpenscale ( mem [p + 1 ].hhfield .lhfield ) ;
      if ( scf == 0 ) 
      if ( gswidth == 0 ) 
      scf = mem [p + 7 ].cint ;
      else hh = 0 ;
      else {
	  
	scf = makescaled ( gswidth , scf ) ;
	scf = takescaled ( scf , mem [p + 7 ].cint ) ;
      } 
    } 
    if ( hh == 0 ) 
    {
      if ( gsdashp != 0 ) 
      {
	psprint ( 1132 ) ;
	gsdashp = 0 ;
      } 
    } 
    else if ( ( gsdashsc != scf ) || ! samedashes ( gsdashp , hh ) ) 
    {
      gsdashp = hh ;
      gsdashsc = scf ;
      if ( ( mem [hh + 1 ].cint == 0 ) || ( abs ( mem [hh + 1 ].cint ) / 
      65536L >= 2147483647L / scf ) ) 
      psprint ( 1132 ) ;
      else {
	  
	pp = mem [hh ].hhfield .v.RH ;
	mem [3 ].cint = mem [pp + 1 ].cint + mem [hh + 1 ].cint ;
	if ( psoffset + 28 > maxprintline ) 
	println () ;
	print ( 1133 ) ;
	while ( pp != 2 ) {
	    
	  pspairout ( takescaled ( mem [pp + 2 ].cint - mem [pp + 1 ].cint 
	  , scf ) , takescaled ( mem [mem [pp ].hhfield .v.RH + 1 ].cint - 
	  mem [pp + 2 ].cint , scf ) ) ;
	  pp = mem [pp ].hhfield .v.RH ;
	} 
	if ( psoffset + 22 > maxprintline ) 
	println () ;
	print ( 1134 ) ;
	printscaled ( takescaled ( dashoffset ( hh ) , scf ) ) ;
	print ( 1135 ) ;
      } 
    } 
    if ( mem [p ].hhfield .b0 == 2 ) 
    if ( ( mem [mem [p + 1 ].hhfield .v.RH ].hhfield .b0 == 0 ) || ( mem [
    p + 6 ].hhfield .v.RH != 0 ) ) 
    if ( gslcap != mem [p + 6 ].hhfield .b0 ) 
    {
      if ( psoffset + 13 > maxprintline ) 
      println () ;
      printchar ( 32 ) ;
      printchar ( 48 + mem [p + 6 ].hhfield .b0 ) ;
      print ( 1124 ) ;
      gslcap = mem [p + 6 ].hhfield .b0 ;
    } 
    if ( gsljoin != mem [p ].hhfield .b1 ) 
    {
      if ( psoffset + 14 > maxprintline ) 
      println () ;
      printchar ( 32 ) ;
      printchar ( 48 + mem [p ].hhfield .b1 ) ;
      print ( 1125 ) ;
      gsljoin = mem [p ].hhfield .b1 ;
    } 
    if ( gsmiterlim != mem [p + 5 ].cint ) 
    {
      if ( psoffset + 27 > maxprintline ) 
      println () ;
      printchar ( 32 ) ;
      printscaled ( mem [p + 5 ].cint ) ;
      print ( 1126 ) ;
      gsmiterlim = mem [p + 5 ].cint ;
    } 
  } 
  if ( psoffset > 0 ) 
  println () ;
} 
void 
#ifdef HAVE_PROTOTYPES
zstrokeellipse ( halfword h , boolean fillalso ) 
#else
zstrokeellipse ( h , fillalso ) 
  halfword h ;
  boolean fillalso ;
#endif
{
  scaled txx, txy, tyx, tyy  ;
  halfword p  ;
  scaled d1, det  ;
  integer s  ;
  boolean transformed  ;
  transformed = false ;
  p = mem [h + 1 ].hhfield .lhfield ;
  txx = mem [p + 3 ].cint ;
  tyx = mem [p + 4 ].cint ;
  txy = mem [p + 5 ].cint ;
  tyy = mem [p + 6 ].cint ;
  if ( ( mem [p + 1 ].cint != 0 ) || ( mem [p + 2 ].cint != 0 ) ) 
  {
    printnl ( 1139 ) ;
    pspairout ( mem [p + 1 ].cint , mem [p + 2 ].cint ) ;
    psprint ( 1140 ) ;
    txx = txx - mem [p + 1 ].cint ;
    tyx = tyx - mem [p + 2 ].cint ;
    txy = txy - mem [p + 1 ].cint ;
    tyy = tyy - mem [p + 2 ].cint ;
    transformed = true ;
  } 
  else printnl ( 284 ) ;
  if ( gswidth != 65536L ) 
  if ( gswidth == 0 ) 
  {
    txx = 65536L ;
    tyy = 65536L ;
  } 
  else {
      
    txx = makescaled ( txx , gswidth ) ;
    txy = makescaled ( txy , gswidth ) ;
    tyx = makescaled ( tyx , gswidth ) ;
    tyy = makescaled ( tyy , gswidth ) ;
  } 
  if ( ( txy != 0 ) || ( tyx != 0 ) || ( txx != 65536L ) || ( tyy != 65536L ) 
  ) 
  if ( ( ! transformed ) ) 
  {
    psprint ( 1139 ) ;
    transformed = true ;
  } 
  det = takescaled ( txx , tyy ) - takescaled ( txy , tyx ) ;
  d1 = 4 * 10 + 1 ;
  if ( abs ( det ) < d1 ) 
  {
    if ( det >= 0 ) 
    {
      d1 = d1 - det ;
      s = 1 ;
    } 
    else {
	
      d1 = - (integer) d1 - det ;
      s = -1 ;
    } 
    d1 = d1 * 65536L ;
    if ( abs ( txx ) + abs ( tyy ) >= abs ( txy ) + abs ( tyy ) ) 
    if ( abs ( txx ) > abs ( tyy ) ) 
    tyy = tyy + ( d1 + s * abs ( txx ) ) / txx ;
    else txx = txx + ( d1 + s * abs ( tyy ) ) / tyy ;
    else if ( abs ( txy ) > abs ( tyx ) ) 
    tyx = tyx + ( d1 + s * abs ( txy ) ) / txy ;
    else txy = txy + ( d1 + s * abs ( tyx ) ) / tyx ;
  } 
  pspathout ( mem [h + 1 ].hhfield .v.RH ) ;
  if ( fillalso ) 
  printnl ( 1136 ) ;
  if ( ( txy != 0 ) || ( tyx != 0 ) ) 
  {
    println () ;
    printchar ( 91 ) ;
    pspairout ( txx , tyx ) ;
    pspairout ( txy , tyy ) ;
    psprint ( 1141 ) ;
  } 
  else if ( ( txx != 65536L ) || ( tyy != 65536L ) ) 
  {
    println () ;
    pspairout ( txx , tyy ) ;
    print ( 1142 ) ;
  } 
  psprint ( 1137 ) ;
  if ( transformed ) 
  psprint ( 1138 ) ;
  println () ;
} 
void 
#ifdef HAVE_PROTOTYPES
zpsfillout ( halfword p ) 
#else
zpsfillout ( p ) 
  halfword p ;
#endif
{
  pspathout ( p ) ;
  psprint ( 1143 ) ;
  println () ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdoouterenvelope ( halfword p , halfword h ) 
#else
zdoouterenvelope ( p , h ) 
  halfword p ;
  halfword h ;
#endif
{
  p = makeenvelope ( p , mem [h + 1 ].hhfield .lhfield , mem [h ]
  .hhfield .b1 , 0 , mem [h + 5 ].cint ) ;
  psfillout ( p ) ;
  tossknotlist ( p ) ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zchoosescale ( halfword p ) 
#else
zchoosescale ( p ) 
  halfword p ;
#endif
{
  register scaled Result; scaled a, b, c, d, ad, bc  ;
  a = mem [p + 10 ].cint ;
  b = mem [p + 11 ].cint ;
  c = mem [p + 12 ].cint ;
  d = mem [p + 13 ].cint ;
  if ( ( a < 0 ) ) 
  a = - (integer) a ;
  if ( ( b < 0 ) ) 
  b = - (integer) b ;
  if ( ( c < 0 ) ) 
  c = - (integer) c ;
  if ( ( d < 0 ) ) 
  d = - (integer) d ;
  ad = half ( a - d ) ;
  bc = half ( b - c ) ;
  Result = pythadd ( pythadd ( d + ad , ad ) , pythadd ( c + bc , bc ) ) ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zpsstringout ( strnumber s ) 
#else
zpsstringout ( s ) 
  strnumber s ;
#endif
{
  poolpointer i  ;
  ASCIIcode k  ;
  print ( 40 ) ;
  i = strstart [s ];
  while ( i < strstart [nextstr [s ]]) {
      
    if ( psoffset + 5 > maxprintline ) 
    {
      printchar ( 92 ) ;
      println () ;
    } 
    k = strpool [i ];
    if ( ( ( k < 32 ) || ( k > 126 ) ) ) 
    {
      printchar ( 92 ) ;
      printchar ( 48 + ( k / 64 ) ) ;
      printchar ( 48 + ( ( k / 8 ) % 8 ) ) ;
      printchar ( 48 + ( k % 8 ) ) ;
    } 
    else {
	
      if ( ( k == 40 ) || ( k == 41 ) || ( k == 92 ) ) 
      printchar ( 92 ) ;
      printchar ( k ) ;
    } 
    incr ( i ) ;
  } 
  print ( 41 ) ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
zispsname ( strnumber s ) 
#else
zispsname ( s ) 
  strnumber s ;
#endif
{
  /* 45 10 */ register boolean Result; poolpointer i  ;
  ASCIIcode k  ;
  i = strstart [s ];
  while ( i < strstart [nextstr [s ]]) {
      
    k = strpool [i ];
    if ( ( k <= 32 ) || ( k > 126 ) ) 
    goto lab45 ;
    if ( ( k == 40 ) || ( k == 41 ) || ( k == 60 ) || ( k == 62 ) || ( k == 
    123 ) || ( k == 125 ) || ( k == 47 ) || ( k == 37 ) ) 
    goto lab45 ;
    incr ( i ) ;
  } 
  Result = true ;
  goto lab10 ;
  lab45: Result = false ;
  lab10: ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zpsnameout ( strnumber s , boolean lit ) 
#else
zpsnameout ( s , lit ) 
  strnumber s ;
  boolean lit ;
#endif
{
  if ( psoffset + ( strstart [nextstr [s ]]- strstart [s ]) + 2 > 
  maxprintline ) 
  println () ;
  printchar ( 32 ) ;
  if ( ispsname ( s ) ) 
  {
    if ( lit ) 
    printchar ( 47 ) ;
    print ( s ) ;
  } 
  else {
      
    psstringout ( s ) ;
    if ( ! lit ) 
    psprint ( 1144 ) ;
    psprint ( 1145 ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zunmarkfont ( fontnumber f ) 
#else
zunmarkfont ( f ) 
  fontnumber f ;
#endif
{
  integer k  ;
  {register integer for_end; k = charbase [f ]+ fontbc [f ];for_end = 
  charbase [f ]+ fontec [f ]; if ( k <= for_end) do 
    fontinfo [k ].qqqq .b3 = 0 ;
  while ( k++ < for_end ) ;} 
} 
void 
#ifdef HAVE_PROTOTYPES
zmarkstringchars ( fontnumber f , strnumber s ) 
#else
zmarkstringchars ( f , s ) 
  fontnumber f ;
  strnumber s ;
#endif
{
  integer b  ;
  poolASCIIcode bc, ec  ;
  poolpointer k  ;
  b = charbase [f ];
  bc = fontbc [f ];
  ec = fontec [f ];
  k = strstart [nextstr [s ]];
  while ( k > strstart [s ]) {
      
    decr ( k ) ;
    if ( ( strpool [k ]>= bc ) && ( strpool [k ]<= ec ) ) 
    fontinfo [b + strpool [k ]].qqqq .b3 = 1 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zhexdigitout ( smallnumber d ) 
#else
zhexdigitout ( d ) 
  smallnumber d ;
#endif
{
  if ( d < 10 ) 
  printchar ( d + 48 ) ;
  else printchar ( d + 87 ) ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zpsmarksout ( fontnumber f , eightbits c ) 
#else
zpsmarksout ( f , c ) 
  fontnumber f ;
  eightbits c ;
#endif
{
  register halfword Result; eightbits bc, ec  ;
  integer lim  ;
  integer p  ;
  char d, b  ;
  lim = 4 * ( emergencylinelength - psoffset - 4 ) ;
  bc = fontbc [f ];
  ec = fontec [f ];
  if ( c > bc ) 
  bc = c ;
  p = charbase [f ]+ bc ;
  while ( ( fontinfo [p ].qqqq .b3 == 0 ) && ( bc < ec ) ) {
      
    incr ( p ) ;
    incr ( bc ) ;
  } 
  if ( ec >= bc + lim ) 
  ec = bc + lim - 1 ;
  p = charbase [f ]+ ec ;
  while ( ( fontinfo [p ].qqqq .b3 == 0 ) && ( bc < ec ) ) {
      
    decr ( p ) ;
    decr ( ec ) ;
  } 
  printchar ( 32 ) ;
  hexdigitout ( bc / 16 ) ;
  hexdigitout ( bc % 16 ) ;
  printchar ( 58 ) ;
  b = 8 ;
  d = 0 ;
  {register integer for_end; p = charbase [f ]+ bc ;for_end = charbase [f 
  ]+ ec ; if ( p <= for_end) do 
    {
      if ( b == 0 ) 
      {
	hexdigitout ( d ) ;
	d = 0 ;
	b = 8 ;
      } 
      if ( fontinfo [p ].qqqq .b3 != 0 ) 
      d = d + b ;
      b = halfp ( b ) ;
    } 
  while ( p++ < for_end ) ;} 
  hexdigitout ( d ) ;
  while ( ( ec < fontec [f ]) && ( fontinfo [p ].qqqq .b3 == 0 ) ) {
      
    incr ( p ) ;
    incr ( ec ) ;
  } 
  Result = ec + 1 ;
  return Result ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
zcheckpsmarks ( fontnumber f , integer c ) 
#else
zcheckpsmarks ( f , c ) 
  fontnumber f ;
  integer c ;
#endif
{
  /* 10 */ register boolean Result; integer p  ;
  {register integer for_end; p = charbase [f ]+ c ;for_end = charbase [f 
  ]+ fontec [f ]; if ( p <= for_end) do 
    if ( fontinfo [p ].qqqq .b3 == 1 ) 
    {
      Result = true ;
      goto lab10 ;
    } 
  while ( p++ < for_end ) ;} 
  Result = false ;
  lab10: ;
  return Result ;
} 
quarterword 
#ifdef HAVE_PROTOTYPES
zsizeindex ( fontnumber f , scaled s ) 
#else
zsizeindex ( f , s ) 
  fontnumber f ;
  scaled s ;
#endif
{
  /* 40 */ register quarterword Result; halfword p, q  ;
  quarterword i  ;
  q = fontsizes [f ];
  i = 0 ;
  while ( q != 0 ) {
      
    if ( abs ( s - mem [q + 1 ].cint ) <= 65 ) 
    goto lab40 ;
    else {
	
      p = q ;
      q = mem [q ].hhfield .v.RH ;
      incr ( i ) ;
    } 
    if ( i == 255 ) 
    overflow ( 1146 , 255 ) ;
  } 
  q = getnode ( 2 ) ;
  mem [q + 1 ].cint = s ;
  if ( i == 0 ) 
  fontsizes [f ]= q ;
  else mem [p ].hhfield .v.RH = q ;
  lab40: Result = i ;
  return Result ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zindexedsize ( fontnumber f , quarterword j ) 
#else
zindexedsize ( f , j ) 
  fontnumber f ;
  quarterword j ;
#endif
{
  register scaled Result; halfword p  ;
  quarterword i  ;
  p = fontsizes [f ];
  i = 0 ;
  if ( p == 0 ) 
  confusion ( 1147 ) ;
  while ( ( i != j ) ) {
      
    incr ( i ) ;
    p = mem [p ].hhfield .v.RH ;
    if ( p == 0 ) 
    confusion ( 1147 ) ;
  } 
  Result = mem [p + 1 ].cint ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
clearsizes ( void ) 
#else
clearsizes ( ) 
#endif
{
  fontnumber f  ;
  halfword p  ;
  {register integer for_end; f = 1 ;for_end = lastfnum ; if ( f <= for_end) 
  do 
    while ( fontsizes [f ]!= 0 ) {
	
      p = fontsizes [f ];
      fontsizes [f ]= mem [p ].hhfield .v.RH ;
      freenode ( p , 2 ) ;
    } 
  while ( f++ < for_end ) ;} 
} 
void 
#ifdef HAVE_PROTOTYPES
zshipout ( halfword h ) 
#else
zshipout ( h ) 
  halfword h ;
#endif
{
  /* 30 40 */ halfword p  ;
  halfword q  ;
  integer t  ;
  fontnumber f, ff  ;
  fontnumber ldf  ;
  boolean donefonts  ;
  quarterword nextsize  ;
  halfword curfsize[fontmax + 1]  ;
  scaled ds, scf  ;
  boolean transformed  ;
  openoutputfile () ;
  if ( ( internal [32 ]> 0 ) && ( lastpsfnum < lastfnum ) ) 
  readpsnametable () ;
  nonpssetting = selector ;
  selector = 5 ;
  print ( 1156 ) ;
  if ( internal [32 ]> 0 ) 
  print ( 1157 ) ;
  printnl ( 1158 ) ;
  setbbox ( h , true ) ;
  if ( mem [h + 2 ].cint > mem [h + 4 ].cint ) 
  print ( 1159 ) ;
  else if ( internal [32 ]< 0 ) 
  {
    pspairout ( mem [h + 2 ].cint , mem [h + 3 ].cint ) ;
    pspairout ( mem [h + 4 ].cint , mem [h + 5 ].cint ) ;
  } 
  else {
      
    pspairout ( floorscaled ( mem [h + 2 ].cint ) , floorscaled ( mem [h + 
    3 ].cint ) ) ;
    pspairout ( - (integer) floorscaled ( - (integer) mem [h + 4 ].cint ) , 
    - (integer) floorscaled ( - (integer) mem [h + 5 ].cint ) ) ;
  } 
  printnl ( 1160 ) ;
  printnl ( 1161 ) ;
  printint ( roundunscaled ( internal [13 ]) ) ;
  printchar ( 46 ) ;
  printdd ( roundunscaled ( internal [14 ]) ) ;
  printchar ( 46 ) ;
  printdd ( roundunscaled ( internal [15 ]) ) ;
  printchar ( 58 ) ;
  t = roundunscaled ( internal [16 ]) ;
  printdd ( t / 60 ) ;
  printdd ( t % 60 ) ;
  printnl ( 1162 ) ;
  {register integer for_end; f = 1 ;for_end = lastfnum ; if ( f <= for_end) 
  do 
    fontsizes [f ]= 0 ;
  while ( f++ < for_end ) ;} 
  p = mem [h + 7 ].hhfield .v.RH ;
  while ( p != 0 ) {
      
    if ( mem [p ].hhfield .b0 == 3 ) 
    if ( mem [p + 1 ].hhfield .lhfield != 0 ) 
    {
      f = mem [p + 1 ].hhfield .lhfield ;
      if ( internal [32 ]> 0 ) 
      fontsizes [f ]= 1 ;
      else {
	  
	if ( fontsizes [f ]== 0 ) 
	unmarkfont ( f ) ;
	mem [p ].hhfield .b1 = sizeindex ( f , choosescale ( p ) ) ;
	if ( mem [p ].hhfield .b1 == 0 ) 
	markstringchars ( f , mem [p + 1 ].hhfield .v.RH ) ;
      } 
    } 
    p = mem [p ].hhfield .v.RH ;
  } 
  if ( internal [32 ]> 0 ) 
  {
    ldf = 0 ;
    {register integer for_end; f = 1 ;for_end = lastfnum ; if ( f <= 
    for_end) do 
      if ( fontsizes [f ]!= 0 ) 
      {
	if ( ldf == 0 ) 
	printnl ( 1163 ) ;
	{register integer for_end; ff = ldf ;for_end = 0 ; if ( ff >= 
	for_end) do 
	  if ( fontsizes [ff ]!= 0 ) 
	  if ( strvsstr ( fontpsname [f ], fontpsname [ff ]) == 0 ) 
	  goto lab40 ;
	while ( ff-- > for_end ) ;} 
	if ( psoffset + 1 + ( strstart [nextstr [fontpsname [f ]]]- 
	strstart [fontpsname [f ]]) > maxprintline ) 
	printnl ( 1164 ) ;
	printchar ( 32 ) ;
	print ( fontpsname [f ]) ;
	ldf = f ;
	lab40: ;
      } 
    while ( f++ < for_end ) ;} 
  } 
  else {
      
    nextsize = 0 ;
    {register integer for_end; f = 1 ;for_end = lastfnum ; if ( f <= 
    for_end) do 
      curfsize [f ]= fontsizes [f ];
    while ( f++ < for_end ) ;} 
    do {
	donefonts = true ;
      {register integer for_end; f = 1 ;for_end = lastfnum ; if ( f <= 
      for_end) do 
	{
	  if ( curfsize [f ]!= 0 ) 
	  {
	    t = 0 ;
	    while ( checkpsmarks ( f , t ) ) {
		
	      printnl ( 1165 ) ;
	      if ( psoffset + ( strstart [nextstr [fontname [f ]]]- 
	      strstart [fontname [f ]]) + 12 > emergencylinelength ) 
	      goto lab30 ;
	      print ( fontname [f ]) ;
	      printchar ( 32 ) ;
	      ds = ( fontdsize [f ]+ 8 ) / 16 ;
	      printscaled ( takescaled ( ds , mem [curfsize [f ]+ 1 ].cint 
	      ) ) ;
	      if ( psoffset + 12 > emergencylinelength ) 
	      goto lab30 ;
	      printchar ( 32 ) ;
	      printscaled ( ds ) ;
	      if ( psoffset + 5 > emergencylinelength ) 
	      goto lab30 ;
	      t = psmarksout ( f , t ) ;
	    } 
	    lab30: curfsize [f ]= mem [curfsize [f ]].hhfield .v.RH ;
	  } 
	  if ( curfsize [f ]!= 0 ) 
	  {
	    unmarkfont ( f ) ;
	    donefonts = false ;
	  } 
	} 
      while ( f++ < for_end ) ;} 
      if ( ! donefonts ) 
      {
	incr ( nextsize ) ;
	p = mem [h + 7 ].hhfield .v.RH ;
	while ( p != 0 ) {
	    
	  if ( mem [p ].hhfield .b0 == 3 ) 
	  if ( mem [p + 1 ].hhfield .lhfield != 0 ) 
	  if ( mem [p ].hhfield .b1 == nextsize ) 
	  markstringchars ( mem [p + 1 ].hhfield .lhfield , mem [p + 1 ]
	  .hhfield .v.RH ) ;
	  p = mem [p ].hhfield .v.RH ;
	} 
      } 
    } while ( ! ( donefonts ) ) ;
  } 
  println () ;
  if ( internal [32 ]> 0 ) 
  {
    if ( ldf != 0 ) 
    {
      {register integer for_end; f = 1 ;for_end = lastfnum ; if ( f <= 
      for_end) do 
	if ( fontsizes [f ]!= 0 ) 
	{
	  psnameout ( fontname [f ], true ) ;
	  psnameout ( fontpsname [f ], true ) ;
	  psprint ( 1166 ) ;
	  println () ;
	} 
      while ( f++ < for_end ) ;} 
      print ( 1167 ) ;
      println () ;
    } 
  } 
  print ( 1151 ) ;
  printnl ( 1152 ) ;
  println () ;
  t = mem [memtop - 3 ].hhfield .v.RH ;
  while ( t != 0 ) {
      
    if ( ( strstart [nextstr [mem [t + 1 ].cint ]]- strstart [mem [t + 
    1 ].cint ]) <= emergencylinelength ) 
    print ( mem [t + 1 ].cint ) ;
    else overflow ( 1150 , emergencylinelength ) ;
    println () ;
    t = mem [t ].hhfield .v.RH ;
  } 
  flushtokenlist ( mem [memtop - 3 ].hhfield .v.RH ) ;
  mem [memtop - 3 ].hhfield .v.RH = 0 ;
  lastpending = memtop - 3 ;
  unknowngraphicsstate ( 0 ) ;
  neednewpath = true ;
  p = mem [h + 7 ].hhfield .v.RH ;
  while ( p != 0 ) {
      
    fixgraphicsstate ( p ) ;
    switch ( mem [p ].hhfield .b0 ) 
    {case 4 : 
      {
	printnl ( 1139 ) ;
	pspathout ( mem [p + 1 ].hhfield .v.RH ) ;
	psprint ( 1168 ) ;
	println () ;
      } 
      break ;
    case 6 : 
      {
	printnl ( 1169 ) ;
	println () ;
	unknowngraphicsstate ( -1 ) ;
      } 
      break ;
    case 1 : 
      if ( mem [p + 1 ].hhfield .lhfield == 0 ) 
      psfillout ( mem [p + 1 ].hhfield .v.RH ) ;
      else if ( ( mem [p + 1 ].hhfield .lhfield == mem [mem [p + 1 ]
      .hhfield .lhfield ].hhfield .v.RH ) ) 
      strokeellipse ( p , true ) ;
      else {
	  
	doouterenvelope ( copypath ( mem [p + 1 ].hhfield .v.RH ) , p ) ;
	doouterenvelope ( htapypoc ( mem [p + 1 ].hhfield .v.RH ) , p ) ;
      } 
      break ;
    case 2 : 
      if ( ( mem [p + 1 ].hhfield .lhfield == mem [mem [p + 1 ].hhfield 
      .lhfield ].hhfield .v.RH ) ) 
      strokeellipse ( p , false ) ;
      else {
	  
	q = copypath ( mem [p + 1 ].hhfield .v.RH ) ;
	t = mem [p + 6 ].hhfield .b0 ;
	if ( mem [q ].hhfield .b0 != 0 ) 
	{
	  mem [insertknot ( q , mem [q + 1 ].cint , mem [q + 2 ].cint ) ]
	  .hhfield .b0 = 0 ;
	  mem [q ].hhfield .b1 = 0 ;
	  q = mem [q ].hhfield .v.RH ;
	  t = 1 ;
	} 
	q = makeenvelope ( q , mem [p + 1 ].hhfield .lhfield , mem [p ]
	.hhfield .b1 , t , mem [p + 5 ].cint ) ;
	psfillout ( q ) ;
	tossknotlist ( q ) ;
      } 
      break ;
    case 3 : 
      if ( ( mem [p + 1 ].hhfield .lhfield != 0 ) && ( ( strstart [nextstr 
      [mem [p + 1 ].hhfield .v.RH ]]- strstart [mem [p + 1 ].hhfield 
      .v.RH ]) > 0 ) ) 
      {
	if ( internal [32 ]> 0 ) 
	scf = choosescale ( p ) ;
	else scf = indexedsize ( mem [p + 1 ].hhfield .lhfield , mem [p ]
	.hhfield .b1 ) ;
	transformed = ( mem [p + 10 ].cint != scf ) || ( mem [p + 13 ]
	.cint != scf ) || ( mem [p + 11 ].cint != 0 ) || ( mem [p + 12 ]
	.cint != 0 ) ;
	if ( transformed ) 
	{
	  print ( 1171 ) ;
	  pspairout ( makescaled ( mem [p + 10 ].cint , scf ) , makescaled ( 
	  mem [p + 12 ].cint , scf ) ) ;
	  pspairout ( makescaled ( mem [p + 11 ].cint , scf ) , makescaled ( 
	  mem [p + 13 ].cint , scf ) ) ;
	  pspairout ( mem [p + 8 ].cint , mem [p + 9 ].cint ) ;
	  psprint ( 1172 ) ;
	} 
	else {
	    
	  pspairout ( mem [p + 8 ].cint , mem [p + 9 ].cint ) ;
	  psprint ( 1119 ) ;
	} 
	println () ;
	psstringout ( mem [p + 1 ].hhfield .v.RH ) ;
	psnameout ( fontname [mem [p + 1 ].hhfield .lhfield ], false ) ;
	if ( psoffset + 18 > maxprintline ) 
	println () ;
	printchar ( 32 ) ;
	ds = ( fontdsize [mem [p + 1 ].hhfield .lhfield ]+ 8 ) / 16 ;
	printscaled ( takescaled ( ds , scf ) ) ;
	print ( 1170 ) ;
	if ( transformed ) 
	psprint ( 1138 ) ;
	println () ;
      } 
      break ;
    case 5 : 
    case 7 : 
      ;
      break ;
    } 
    p = mem [p ].hhfield .v.RH ;
  } 
  print ( 1153 ) ;
  println () ;
  print ( 1154 ) ;
  println () ;
  aclose ( psfile ) ;
  selector = nonpssetting ;
  if ( internal [32 ]<= 0 ) 
  clearsizes () ;
  printchar ( 93 ) ;
  fflush ( stdout ) ;
  incr ( totalshipped ) ;
  if ( internal [9 ]> 0 ) 
  printedges ( h , 1155 , true ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
doshipout ( void ) 
#else
doshipout ( ) 
#endif
{
  integer c  ;
  getxnext () ;
  scanexpression () ;
  if ( curtype != 10 ) 
  {
    disperr ( 0 , 1032 ) ;
    {
      helpptr = 1 ;
      helpline [0 ]= 1033 ;
    } 
    putgetflusherror ( 0 ) ;
  } 
  else {
      
    c = roundunscaled ( internal [17 ]) % 256 ;
    if ( c < 0 ) 
    c = c + 256 ;
    if ( c < bc ) 
    bc = c ;
    if ( c > ec ) 
    ec = c ;
    charexists [c ]= true ;
    tfmwidth [c ]= tfmcheck ( 19 ) ;
    tfmheight [c ]= tfmcheck ( 20 ) ;
    tfmdepth [c ]= tfmcheck ( 21 ) ;
    tfmitalcorr [c ]= tfmcheck ( 22 ) ;
    shipout ( curexp ) ;
    flushcurexp ( 0 ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
znostringerr ( strnumber s ) 
#else
znostringerr ( s ) 
  strnumber s ;
#endif
{
  disperr ( 0 , 740 ) ;
  {
    helpptr = 1 ;
    helpline [0 ]= s ;
  } 
  putgeterror () ;
} 
void 
#ifdef HAVE_PROTOTYPES
domessage ( void ) 
#else
domessage ( ) 
#endif
{
  char m  ;
  m = curmod ;
  getxnext () ;
  scanexpression () ;
  if ( curtype != 4 ) 
  nostringerr ( 1037 ) ;
  else switch ( m ) 
  {case 0 : 
    {
      printnl ( 284 ) ;
      print ( curexp ) ;
    } 
    break ;
  case 1 : 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 284 ) ;
      } 
      print ( curexp ) ;
      if ( errhelp != 0 ) 
      useerrhelp = true ;
      else if ( longhelpseen ) 
      {
	helpptr = 1 ;
	helpline [0 ]= 1038 ;
      } 
      else {
	  
	if ( interaction < 3 ) 
	longhelpseen = true ;
	{
	  helpptr = 4 ;
	  helpline [3 ]= 1039 ;
	  helpline [2 ]= 1040 ;
	  helpline [1 ]= 1041 ;
	  helpline [0 ]= 1042 ;
	} 
      } 
      putgeterror () ;
      useerrhelp = false ;
    } 
    break ;
  case 2 : 
    {
      if ( errhelp != 0 ) 
      {
	if ( strref [errhelp ]< 127 ) 
	if ( strref [errhelp ]> 1 ) 
	decr ( strref [errhelp ]) ;
	else flushstring ( errhelp ) ;
      } 
      if ( ( strstart [nextstr [curexp ]]- strstart [curexp ]) == 0 ) 
      errhelp = 0 ;
      else {
	  
	errhelp = curexp ;
	{
	  if ( strref [errhelp ]< 127 ) 
	  incr ( strref [errhelp ]) ;
	} 
      } 
    } 
    break ;
  } 
  flushcurexp ( 0 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
dowrite ( void ) 
#else
dowrite ( ) 
#endif
{
  /* 22 */ strnumber t  ;
  writeindex n, n0  ;
  char oldsetting  ;
  getxnext () ;
  scanexpression () ;
  if ( curtype != 4 ) 
  nostringerr ( 1043 ) ;
  else if ( curcmd != 71 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 1044 ) ;
    } 
    {
      helpptr = 1 ;
      helpline [0 ]= 1045 ;
    } 
    putgeterror () ;
  } 
  else {
      
    t = curexp ;
    curtype = 1 ;
    getxnext () ;
    scanexpression () ;
    if ( curtype != 4 ) 
    nostringerr ( 1046 ) ;
    else {
	
      n = writefiles ;
      n0 = writefiles ;
      do {
	  lab22: if ( n == 0 ) 
	{
	  if ( n0 == writefiles ) 
	  if ( writefiles < 4 ) 
	  incr ( writefiles ) ;
	  else overflow ( 1047 , 4 ) ;
	  n = n0 ;
	  openwritefile ( curexp , n ) ;
	} 
	else {
	    
	  decr ( n ) ;
	  if ( wrfname [n ]== 0 ) 
	  {
	    n0 = n ;
	    goto lab22 ;
	  } 
	} 
      } while ( ! ( strvsstr ( curexp , wrfname [n ]) == 0 ) ) ;
      if ( eofline == 0 ) 
      {
	{
	  strpool [poolptr ]= 0 ;
	  incr ( poolptr ) ;
	} 
	eofline = makestring () ;
	strref [eofline ]= 127 ;
      } 
      if ( strvsstr ( t , eofline ) == 0 ) 
      {
	aclose ( wrfile [n ]) ;
	{
	  if ( strref [wrfname [n ]]< 127 ) 
	  if ( strref [wrfname [n ]]> 1 ) 
	  decr ( strref [wrfname [n ]]) ;
	  else flushstring ( wrfname [n ]) ;
	} 
	wrfname [n ]= 0 ;
	if ( n == writefiles - 1 ) 
	writefiles = n ;
      } 
      else {
	  
	oldsetting = selector ;
	selector = n ;
	print ( t ) ;
	println () ;
	selector = oldsetting ;
      } 
    } 
    {
      if ( strref [t ]< 127 ) 
      if ( strref [t ]> 1 ) 
      decr ( strref [t ]) ;
      else flushstring ( t ) ;
    } 
  } 
  flushcurexp ( 0 ) ;
} 
eightbits 
#ifdef HAVE_PROTOTYPES
getcode ( void ) 
#else
getcode ( ) 
#endif
{
  /* 40 */ register eightbits Result; integer c  ;
  getxnext () ;
  scanexpression () ;
  if ( curtype == 16 ) 
  {
    c = roundunscaled ( curexp ) ;
    if ( c >= 0 ) 
    if ( c < 256 ) 
    goto lab40 ;
  } 
  else if ( curtype == 4 ) 
  if ( ( strstart [nextstr [curexp ]]- strstart [curexp ]) == 1 ) 
  {
    c = strpool [strstart [curexp ]];
    goto lab40 ;
  } 
  disperr ( 0 , 1056 ) ;
  {
    helpptr = 2 ;
    helpline [1 ]= 1057 ;
    helpline [0 ]= 1058 ;
  } 
  putgetflusherror ( 0 ) ;
  c = 0 ;
  lab40: Result = c ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zsettag ( halfword c , smallnumber t , halfword r ) 
#else
zsettag ( c , t , r ) 
  halfword c ;
  smallnumber t ;
  halfword r ;
#endif
{
  if ( chartag [c ]== 0 ) 
  {
    chartag [c ]= t ;
    charremainder [c ]= r ;
    if ( t == 1 ) 
    {
      incr ( labelptr ) ;
      labelloc [labelptr ]= r ;
      labelchar [labelptr ]= c ;
    } 
  } 
  else {
      
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 1059 ) ;
    } 
    if ( ( c > 32 ) && ( c < 127 ) ) 
    print ( c ) ;
    else if ( c == 256 ) 
    print ( 1060 ) ;
    else {
	
      print ( 1061 ) ;
      printint ( c ) ;
    } 
    print ( 1062 ) ;
    switch ( chartag [c ]) 
    {case 1 : 
      print ( 1063 ) ;
      break ;
    case 2 : 
      print ( 1064 ) ;
      break ;
    case 3 : 
      print ( 1053 ) ;
      break ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 1065 ;
      helpline [0 ]= 1019 ;
    } 
    putgeterror () ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
dotfmcommand ( void ) 
#else
dotfmcommand ( ) 
#endif
{
  /* 22 30 */ short c, cc  ;
  integer k  ;
  integer j  ;
  switch ( curmod ) 
  {case 0 : 
    {
      c = getcode () ;
      while ( curcmd == 80 ) {
	  
	cc = getcode () ;
	settag ( c , 2 , cc ) ;
	c = cc ;
      } 
    } 
    break ;
  case 1 : 
    {
      lkstarted = false ;
      lab22: getxnext () ;
      if ( ( curcmd == 77 ) && lkstarted ) 
      {
	c = getcode () ;
	if ( nl - skiptable [c ]> 128 ) 
	{
	  {
	    {
	      if ( interaction == 3 ) 
	      ;
	      printnl ( 262 ) ;
	      print ( 1082 ) ;
	    } 
	    {
	      helpptr = 1 ;
	      helpline [0 ]= 1083 ;
	    } 
	    error () ;
	    ll = skiptable [c ];
	    do {
		lll = ligkern [ll ].b0 ;
	      ligkern [ll ].b0 = 128 ;
	      ll = ll - lll ;
	    } while ( ! ( lll == 0 ) ) ;
	  } 
	  skiptable [c ]= ligtablesize ;
	} 
	if ( skiptable [c ]== ligtablesize ) 
	ligkern [nl - 1 ].b0 = 0 ;
	else ligkern [nl - 1 ].b0 = nl - skiptable [c ]- 1 ;
	skiptable [c ]= nl - 1 ;
	goto lab30 ;
      } 
      if ( curcmd == 78 ) 
      {
	c = 256 ;
	curcmd = 80 ;
      } 
      else {
	  
	backinput () ;
	c = getcode () ;
      } 
      if ( ( curcmd == 80 ) || ( curcmd == 79 ) ) 
      {
	if ( curcmd == 80 ) 
	if ( c == 256 ) 
	bchlabel = nl ;
	else settag ( c , 1 , nl ) ;
	else if ( skiptable [c ]< ligtablesize ) 
	{
	  ll = skiptable [c ];
	  skiptable [c ]= ligtablesize ;
	  do {
	      lll = ligkern [ll ].b0 ;
	    if ( nl - ll > 128 ) 
	    {
	      {
		{
		  if ( interaction == 3 ) 
		  ;
		  printnl ( 262 ) ;
		  print ( 1082 ) ;
		} 
		{
		  helpptr = 1 ;
		  helpline [0 ]= 1083 ;
		} 
		error () ;
		ll = ll ;
		do {
		    lll = ligkern [ll ].b0 ;
		  ligkern [ll ].b0 = 128 ;
		  ll = ll - lll ;
		} while ( ! ( lll == 0 ) ) ;
	      } 
	      goto lab22 ;
	    } 
	    ligkern [ll ].b0 = nl - ll - 1 ;
	    ll = ll - lll ;
	  } while ( ! ( lll == 0 ) ) ;
	} 
	goto lab22 ;
      } 
      if ( curcmd == 75 ) 
      {
	ligkern [nl ].b1 = c ;
	ligkern [nl ].b0 = 0 ;
	if ( curmod < 128 ) 
	{
	  ligkern [nl ].b2 = curmod ;
	  ligkern [nl ].b3 = getcode () ;
	} 
	else {
	    
	  getxnext () ;
	  scanexpression () ;
	  if ( curtype != 16 ) 
	  {
	    disperr ( 0 , 1084 ) ;
	    {
	      helpptr = 2 ;
	      helpline [1 ]= 1085 ;
	      helpline [0 ]= 308 ;
	    } 
	    putgetflusherror ( 0 ) ;
	  } 
	  kern [nk ]= curexp ;
	  k = 0 ;
	  while ( kern [k ]!= curexp ) incr ( k ) ;
	  if ( k == nk ) 
	  {
	    if ( nk == maxkerns ) 
	    overflow ( 1081 , maxkerns ) ;
	    incr ( nk ) ;
	  } 
	  ligkern [nl ].b2 = 128 + ( k / 256 ) ;
	  ligkern [nl ].b3 = ( k % 256 ) ;
	} 
	lkstarted = true ;
      } 
      else {
	  
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 1070 ) ;
	} 
	{
	  helpptr = 1 ;
	  helpline [0 ]= 1071 ;
	} 
	backerror () ;
	ligkern [nl ].b1 = 0 ;
	ligkern [nl ].b2 = 0 ;
	ligkern [nl ].b3 = 0 ;
	ligkern [nl ].b0 = 129 ;
      } 
      if ( nl == ligtablesize ) 
      overflow ( 1072 , ligtablesize ) ;
      incr ( nl ) ;
      if ( curcmd == 81 ) 
      goto lab22 ;
      if ( ligkern [nl - 1 ].b0 < 128 ) 
      ligkern [nl - 1 ].b0 = 128 ;
      lab30: ;
    } 
    break ;
  case 2 : 
    {
      if ( ne == 256 ) 
      overflow ( 1053 , 256 ) ;
      c = getcode () ;
      settag ( c , 3 , ne ) ;
      if ( curcmd != 80 ) 
      {
	missingerr ( 58 ) ;
	{
	  helpptr = 1 ;
	  helpline [0 ]= 1086 ;
	} 
	backerror () ;
      } 
      exten [ne ].b0 = getcode () ;
      if ( curcmd != 81 ) 
      {
	missingerr ( 44 ) ;
	{
	  helpptr = 1 ;
	  helpline [0 ]= 1086 ;
	} 
	backerror () ;
      } 
      exten [ne ].b1 = getcode () ;
      if ( curcmd != 81 ) 
      {
	missingerr ( 44 ) ;
	{
	  helpptr = 1 ;
	  helpline [0 ]= 1086 ;
	} 
	backerror () ;
      } 
      exten [ne ].b2 = getcode () ;
      if ( curcmd != 81 ) 
      {
	missingerr ( 44 ) ;
	{
	  helpptr = 1 ;
	  helpline [0 ]= 1086 ;
	} 
	backerror () ;
      } 
      exten [ne ].b3 = getcode () ;
      incr ( ne ) ;
    } 
    break ;
  case 3 : 
  case 4 : 
    {
      c = curmod ;
      getxnext () ;
      scanexpression () ;
      if ( ( curtype != 16 ) || ( curexp < 32768L ) ) 
      {
	disperr ( 0 , 1066 ) ;
	{
	  helpptr = 2 ;
	  helpline [1 ]= 1067 ;
	  helpline [0 ]= 1068 ;
	} 
	putgeterror () ;
      } 
      else {
	  
	j = roundunscaled ( curexp ) ;
	if ( curcmd != 80 ) 
	{
	  missingerr ( 58 ) ;
	  {
	    helpptr = 1 ;
	    helpline [0 ]= 1069 ;
	  } 
	  backerror () ;
	} 
	if ( c == 3 ) 
	do {
	    if ( j > headersize ) 
	  overflow ( 1054 , headersize ) ;
	  headerbyte [j ]= getcode () ;
	  incr ( j ) ;
	} while ( ! ( curcmd != 81 ) ) ;
	else do {
	    if ( j > maxfontdimen ) 
	  overflow ( 1055 , maxfontdimen ) ;
	  while ( j > np ) {
	      
	    incr ( np ) ;
	    param [np ]= 0 ;
	  } 
	  getxnext () ;
	  scanexpression () ;
	  if ( curtype != 16 ) 
	  {
	    disperr ( 0 , 1087 ) ;
	    {
	      helpptr = 1 ;
	      helpline [0 ]= 308 ;
	    } 
	    putgetflusherror ( 0 ) ;
	  } 
	  param [j ]= curexp ;
	  incr ( j ) ;
	} while ( ! ( curcmd != 81 ) ) ;
      } 
    } 
    break ;
  } 
} 
