#define EXTERN extern
#include "mfd.h"

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
  {
    disperr ( 0 , 696 ) ;
    {
      helpptr = 1 ;
      helpline [0 ]= 991 ;
    } 
    putgeterror () ;
  } 
  else switch ( m ) 
  {case 0 : 
    {
      printnl ( 283 ) ;
      print ( curexp ) ;
    } 
    break ;
  case 1 : 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 261 ) ;
	print ( 283 ) ;
      } 
      print ( curexp ) ;
      if ( errhelp != 0 ) 
      useerrhelp = true ;
      else if ( longhelpseen ) 
      {
	helpptr = 1 ;
	helpline [0 ]= 992 ;
      } 
      else {
	  
	if ( interaction < 3 ) 
	longhelpseen = true ;
	{
	  helpptr = 4 ;
	  helpline [3 ]= 993 ;
	  helpline [2 ]= 994 ;
	  helpline [1 ]= 995 ;
	  helpline [0 ]= 996 ;
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
      if ( ( strstart [curexp + 1 ]- strstart [curexp ]) == 0 ) 
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
  if ( ( strstart [curexp + 1 ]- strstart [curexp ]) == 1 ) 
  {
    c = strpool [strstart [curexp ]];
    goto lab40 ;
  } 
  disperr ( 0 , 1005 ) ;
  {
    helpptr = 2 ;
    helpline [1 ]= 1006 ;
    helpline [0 ]= 1007 ;
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
      printnl ( 261 ) ;
      print ( 1008 ) ;
    } 
    if ( ( c > 32 ) && ( c < 127 ) ) 
    print ( c ) ;
    else if ( c == 256 ) 
    print ( 1009 ) ;
    else {
	
      print ( 1010 ) ;
      printint ( c ) ;
    } 
    print ( 1011 ) ;
    switch ( chartag [c ]) 
    {case 1 : 
      print ( 1012 ) ;
      break ;
    case 2 : 
      print ( 1013 ) ;
      break ;
    case 3 : 
      print ( 1002 ) ;
      break ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 1014 ;
      helpline [0 ]= 966 ;
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
      while ( curcmd == 81 ) {
	  
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
      if ( ( curcmd == 78 ) && lkstarted ) 
      {
	c = getcode () ;
	if ( nl - skiptable [c ]> 128 ) 
	{
	  {
	    {
	      if ( interaction == 3 ) 
	      ;
	      printnl ( 261 ) ;
	      print ( 1031 ) ;
	    } 
	    {
	      helpptr = 1 ;
	      helpline [0 ]= 1032 ;
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
      if ( curcmd == 79 ) 
      {
	c = 256 ;
	curcmd = 81 ;
      } 
      else {
	  
	backinput () ;
	c = getcode () ;
      } 
      if ( ( curcmd == 81 ) || ( curcmd == 80 ) ) 
      {
	if ( curcmd == 81 ) 
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
		  printnl ( 261 ) ;
		  print ( 1031 ) ;
		} 
		{
		  helpptr = 1 ;
		  helpline [0 ]= 1032 ;
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
      if ( curcmd == 76 ) 
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
	    disperr ( 0 , 1033 ) ;
	    {
	      helpptr = 2 ;
	      helpline [1 ]= 1034 ;
	      helpline [0 ]= 307 ;
	    } 
	    putgetflusherror ( 0 ) ;
	  } 
	  kern [nk ]= curexp ;
	  k = 0 ;
	  while ( kern [k ]!= curexp ) incr ( k ) ;
	  if ( k == nk ) 
	  {
	    if ( nk == maxkerns ) 
	    overflow ( 1030 , maxkerns ) ;
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
	  printnl ( 261 ) ;
	  print ( 1019 ) ;
	} 
	{
	  helpptr = 1 ;
	  helpline [0 ]= 1020 ;
	} 
	backerror () ;
	ligkern [nl ].b1 = 0 ;
	ligkern [nl ].b2 = 0 ;
	ligkern [nl ].b3 = 0 ;
	ligkern [nl ].b0 = 129 ;
      } 
      if ( nl == ligtablesize ) 
      overflow ( 1021 , ligtablesize ) ;
      incr ( nl ) ;
      if ( curcmd == 82 ) 
      goto lab22 ;
      if ( ligkern [nl - 1 ].b0 < 128 ) 
      ligkern [nl - 1 ].b0 = 128 ;
      lab30: ;
    } 
    break ;
  case 2 : 
    {
      if ( ne == 256 ) 
      overflow ( 1002 , 256 ) ;
      c = getcode () ;
      settag ( c , 3 , ne ) ;
      if ( curcmd != 81 ) 
      {
	missingerr ( 58 ) ;
	{
	  helpptr = 1 ;
	  helpline [0 ]= 1035 ;
	} 
	backerror () ;
      } 
      exten [ne ].b0 = getcode () ;
      if ( curcmd != 82 ) 
      {
	missingerr ( 44 ) ;
	{
	  helpptr = 1 ;
	  helpline [0 ]= 1035 ;
	} 
	backerror () ;
      } 
      exten [ne ].b1 = getcode () ;
      if ( curcmd != 82 ) 
      {
	missingerr ( 44 ) ;
	{
	  helpptr = 1 ;
	  helpline [0 ]= 1035 ;
	} 
	backerror () ;
      } 
      exten [ne ].b2 = getcode () ;
      if ( curcmd != 82 ) 
      {
	missingerr ( 44 ) ;
	{
	  helpptr = 1 ;
	  helpline [0 ]= 1035 ;
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
	disperr ( 0 , 1015 ) ;
	{
	  helpptr = 2 ;
	  helpline [1 ]= 1016 ;
	  helpline [0 ]= 1017 ;
	} 
	putgeterror () ;
      } 
      else {
	  
	j = roundunscaled ( curexp ) ;
	if ( curcmd != 81 ) 
	{
	  missingerr ( 58 ) ;
	  {
	    helpptr = 1 ;
	    helpline [0 ]= 1018 ;
	  } 
	  backerror () ;
	} 
	if ( c == 3 ) 
	do {
	    if ( j > headersize ) 
	  overflow ( 1003 , headersize ) ;
	  headerbyte [j ]= getcode () ;
	  incr ( j ) ;
	} while ( ! ( curcmd != 82 ) ) ;
	else do {
	    if ( j > maxfontdimen ) 
	  overflow ( 1004 , maxfontdimen ) ;
	  while ( j > np ) {
	      
	    incr ( np ) ;
	    param [np ]= 0 ;
	  } 
	  getxnext () ;
	  scanexpression () ;
	  if ( curtype != 16 ) 
	  {
	    disperr ( 0 , 1036 ) ;
	    {
	      helpptr = 1 ;
	      helpline [0 ]= 307 ;
	    } 
	    putgetflusherror ( 0 ) ;
	  } 
	  param [j ]= curexp ;
	  incr ( j ) ;
	} while ( ! ( curcmd != 82 ) ) ;
      } 
    } 
    break ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
dospecial ( void ) 
#else
dospecial ( ) 
#endif
{
  smallnumber m  ;
  m = curmod ;
  getxnext () ;
  scanexpression () ;
  if ( internal [34 ]>= 0 ) 
  if ( curtype != m ) 
  {
    disperr ( 0 , 1056 ) ;
    {
      helpptr = 1 ;
      helpline [0 ]= 1057 ;
    } 
    putgeterror () ;
  } 
  else {
      
    if ( outputfilename == 0 ) 
    initgf () ;
    if ( m == 4 ) 
    gfstring ( curexp , 0 ) ;
    else {
	
      {
	gfbuf [gfptr ]= 243 ;
	incr ( gfptr ) ;
	if ( gfptr == gflimit ) 
	gfswap () ;
      } 
      gffour ( curexp ) ;
    } 
  } 
  flushcurexp ( 0 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
dostatement ( void ) 
#else
dostatement ( ) 
#endif
{
  curtype = 1 ;
  getxnext () ;
  if ( curcmd > 43 ) 
  {
    if ( curcmd < 83 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 261 ) ;
	print ( 865 ) ;
      } 
      printcmdmod ( curcmd , curmod ) ;
      printchar ( 39 ) ;
      {
	helpptr = 5 ;
	helpline [4 ]= 866 ;
	helpline [3 ]= 867 ;
	helpline [2 ]= 868 ;
	helpline [1 ]= 869 ;
	helpline [0 ]= 870 ;
      } 
      backerror () ;
      getxnext () ;
    } 
  } 
  else if ( curcmd > 30 ) 
  {
    varflag = 77 ;
    scanexpression () ;
    if ( curcmd < 84 ) 
    {
      if ( curcmd == 51 ) 
      doequation () ;
      else if ( curcmd == 77 ) 
      doassignment () ;
      else if ( curtype == 4 ) 
      {
	if ( internal [1 ]> 0 ) 
	{
	  printnl ( 283 ) ;
	  print ( curexp ) ;
	  fflush ( stdout ) ;
	} 
	if ( internal [34 ]> 0 ) 
	{
	  if ( outputfilename == 0 ) 
	  initgf () ;
	  gfstring ( 1058 , curexp ) ;
	} 
      } 
      else if ( curtype != 1 ) 
      {
	disperr ( 0 , 875 ) ;
	{
	  helpptr = 3 ;
	  helpline [2 ]= 876 ;
	  helpline [1 ]= 877 ;
	  helpline [0 ]= 878 ;
	} 
	putgeterror () ;
      } 
      flushcurexp ( 0 ) ;
      curtype = 1 ;
    } 
  } 
  else {
      
    if ( internal [7 ]> 0 ) 
    showcmdmod ( curcmd , curmod ) ;
    switch ( curcmd ) 
    {case 30 : 
      dotypedeclaration () ;
      break ;
    case 16 : 
      if ( curmod > 2 ) 
      makeopdef () ;
      else if ( curmod > 0 ) 
      scandef () ;
      break ;
    case 24 : 
      dorandomseed () ;
      break ;
    case 23 : 
      {
	println () ;
	interaction = curmod ;
	if ( interaction == 0 ) 
	kpsemaketexdiscarderrors = 1 ;
	else kpsemaketexdiscarderrors = 0 ;
	if ( interaction == 0 ) 
	selector = 0 ;
	else selector = 1 ;
	if ( logopened ) 
	selector = selector + 2 ;
	getxnext () ;
      } 
      break ;
    case 21 : 
      doprotection () ;
      break ;
    case 27 : 
      defdelims () ;
      break ;
    case 12 : 
      do {
	  getsymbol () ;
	savevariable ( cursym ) ;
	getxnext () ;
      } while ( ! ( curcmd != 82 ) ) ;
      break ;
    case 13 : 
      dointerim () ;
      break ;
    case 14 : 
      dolet () ;
      break ;
    case 15 : 
      donewinternal () ;
      break ;
    case 22 : 
      doshowwhatever () ;
      break ;
    case 18 : 
      doaddto () ;
      break ;
    case 17 : 
      doshipout () ;
      break ;
    case 11 : 
      dodisplay () ;
      break ;
    case 28 : 
      doopenwindow () ;
      break ;
    case 19 : 
      docull () ;
      break ;
    case 26 : 
      {
	getsymbol () ;
	startsym = cursym ;
	getxnext () ;
      } 
      break ;
    case 25 : 
      domessage () ;
      break ;
    case 20 : 
      dotfmcommand () ;
      break ;
    case 29 : 
      dospecial () ;
      break ;
    } 
    curtype = 1 ;
  } 
  if ( curcmd < 83 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 261 ) ;
      print ( 871 ) ;
    } 
    {
      helpptr = 6 ;
      helpline [5 ]= 872 ;
      helpline [4 ]= 873 ;
      helpline [3 ]= 874 ;
      helpline [2 ]= 868 ;
      helpline [1 ]= 869 ;
      helpline [0 ]= 870 ;
    } 
    backerror () ;
    scannerstatus = 2 ;
    do {
	getnext () ;
      if ( curcmd == 39 ) 
      {
	if ( strref [curmod ]< 127 ) 
	if ( strref [curmod ]> 1 ) 
	decr ( strref [curmod ]) ;
	else flushstring ( curmod ) ;
      } 
    } while ( ! ( curcmd > 82 ) ) ;
    scannerstatus = 0 ;
  } 
  errorcount = 0 ;
} 
void 
#ifdef HAVE_PROTOTYPES
maincontrol ( void ) 
#else
maincontrol ( ) 
#endif
{
  do {
      dostatement () ;
    if ( curcmd == 84 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 261 ) ;
	print ( 906 ) ;
      } 
      {
	helpptr = 2 ;
	helpline [1 ]= 907 ;
	helpline [0 ]= 686 ;
      } 
      flusherror ( 0 ) ;
    } 
  } while ( ! ( curcmd == 85 ) ) ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zsortin ( scaled v ) 
#else
zsortin ( v ) 
  scaled v ;
#endif
{
  /* 40 */ register halfword Result; halfword p, q, r  ;
  p = memtop - 1 ;
  while ( true ) {
      
    q = mem [p ].hhfield .v.RH ;
    if ( v <= mem [q + 1 ].cint ) 
    goto lab40 ;
    p = q ;
  } 
  lab40: if ( v < mem [q + 1 ].cint ) 
  {
    r = getnode ( 2 ) ;
    mem [r + 1 ].cint = v ;
    mem [r ].hhfield .v.RH = q ;
    mem [p ].hhfield .v.RH = r ;
  } 
  Result = mem [p ].hhfield .v.RH ;
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
zmincover ( scaled d ) 
#else
zmincover ( d ) 
  scaled d ;
#endif
{
  register integer Result; halfword p  ;
  scaled l  ;
  integer m  ;
  m = 0 ;
  p = mem [memtop - 1 ].hhfield .v.RH ;
  perturbation = 2147483647L ;
  while ( p != 19 ) {
      
    incr ( m ) ;
    l = mem [p + 1 ].cint ;
    do {
	p = mem [p ].hhfield .v.RH ;
    } while ( ! ( mem [p + 1 ].cint > l + d ) ) ;
    if ( mem [p + 1 ].cint - l < perturbation ) 
    perturbation = mem [p + 1 ].cint - l ;
  } 
  Result = m ;
  return Result ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zthresholdfn ( integer m ) 
#else
zthresholdfn ( m ) 
  integer m ;
#endif
{
  register scaled Result; scaled d  ;
  excess = mincover ( 0 ) - m ;
  if ( excess <= 0 ) 
  Result = 0 ;
  else {
      
    do {
	d = perturbation ;
    } while ( ! ( mincover ( d + d ) <= m ) ) ;
    while ( mincover ( d ) > m ) d = perturbation ;
    Result = d ;
  } 
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
zskimp ( integer m ) 
#else
zskimp ( m ) 
  integer m ;
#endif
{
  register integer Result; scaled d  ;
  halfword p, q, r  ;
  scaled l  ;
  scaled v  ;
  d = thresholdfn ( m ) ;
  perturbation = 0 ;
  q = memtop - 1 ;
  m = 0 ;
  p = mem [memtop - 1 ].hhfield .v.RH ;
  while ( p != 19 ) {
      
    incr ( m ) ;
    l = mem [p + 1 ].cint ;
    mem [p ].hhfield .lhfield = m ;
    if ( mem [mem [p ].hhfield .v.RH + 1 ].cint <= l + d ) 
    {
      do {
	  p = mem [p ].hhfield .v.RH ;
	mem [p ].hhfield .lhfield = m ;
	decr ( excess ) ;
	if ( excess == 0 ) 
	d = 0 ;
      } while ( ! ( mem [mem [p ].hhfield .v.RH + 1 ].cint > l + d ) ) ;
      v = l + halfp ( mem [p + 1 ].cint - l ) ;
      if ( mem [p + 1 ].cint - v > perturbation ) 
      perturbation = mem [p + 1 ].cint - v ;
      r = q ;
      do {
	  r = mem [r ].hhfield .v.RH ;
	mem [r + 1 ].cint = v ;
      } while ( ! ( r == p ) ) ;
      mem [q ].hhfield .v.RH = p ;
    } 
    q = p ;
    p = mem [p ].hhfield .v.RH ;
  } 
  Result = m ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
ztfmwarning ( smallnumber m ) 
#else
ztfmwarning ( m ) 
  smallnumber m ;
#endif
{
  printnl ( 1037 ) ;
  print ( intname [m ]) ;
  print ( 1038 ) ;
  printscaled ( perturbation ) ;
  print ( 1039 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
fixdesignsize ( void ) 
#else
fixdesignsize ( ) 
#endif
{
  scaled d  ;
  d = internal [26 ];
  if ( ( d < 65536L ) || ( d >= 134217728L ) ) 
  {
    if ( d != 0 ) 
    printnl ( 1040 ) ;
    d = 8388608L ;
    internal [26 ]= d ;
  } 
  if ( headerbyte [5 ]< 0 ) 
  if ( headerbyte [6 ]< 0 ) 
  if ( headerbyte [7 ]< 0 ) 
  if ( headerbyte [8 ]< 0 ) 
  {
    headerbyte [5 ]= d / 1048576L ;
    headerbyte [6 ]= ( d / 4096 ) % 256 ;
    headerbyte [7 ]= ( d / 16 ) % 256 ;
    headerbyte [8 ]= ( d % 16 ) * 16 ;
  } 
  maxtfmdimen = 16 * internal [26 ]- internal [26 ]/ 2097152L ;
  if ( maxtfmdimen >= 134217728L ) 
  maxtfmdimen = 134217727L ;
} 
integer 
#ifdef HAVE_PROTOTYPES
zdimenout ( scaled x ) 
#else
zdimenout ( x ) 
  scaled x ;
#endif
{
  register integer Result; if ( abs ( x ) > maxtfmdimen ) 
  {
    incr ( tfmchanged ) ;
    if ( x > 0 ) 
    x = 16777215L ;
    else x = -16777215L ;
  } 
  else x = makescaled ( x * 16 , internal [26 ]) ;
  Result = x ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
fixchecksum ( void ) 
#else
fixchecksum ( ) 
#endif
{
  /* 10 */ eightbits k  ;
  eightbits lb1, lb2, lb3, b4  ;
  integer x  ;
  if ( headerbyte [1 ]< 0 ) 
  if ( headerbyte [2 ]< 0 ) 
  if ( headerbyte [3 ]< 0 ) 
  if ( headerbyte [4 ]< 0 ) 
  {
    lb1 = bc ;
    lb2 = ec ;
    lb3 = bc ;
    b4 = ec ;
    tfmchanged = 0 ;
    {register integer for_end; k = bc ;for_end = ec ; if ( k <= for_end) do 
      if ( charexists [k ]) 
      {
	x = dimenout ( mem [tfmwidth [k ]+ 1 ].cint ) + ( k + 4 ) * 
	4194304L ;
	lb1 = ( lb1 + lb1 + x ) % 255 ;
	lb2 = ( lb2 + lb2 + x ) % 253 ;
	lb3 = ( lb3 + lb3 + x ) % 251 ;
	b4 = ( b4 + b4 + x ) % 247 ;
      } 
    while ( k++ < for_end ) ;} 
    headerbyte [1 ]= lb1 ;
    headerbyte [2 ]= lb2 ;
    headerbyte [3 ]= lb3 ;
    headerbyte [4 ]= b4 ;
    goto lab10 ;
  } 
  {register integer for_end; k = 1 ;for_end = 4 ; if ( k <= for_end) do 
    if ( headerbyte [k ]< 0 ) 
    headerbyte [k ]= 0 ;
  while ( k++ < for_end ) ;} 
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
ztfmqqqq ( fourquarters x ) 
#else
ztfmqqqq ( x ) 
  fourquarters x ;
#endif
{
  putbyte ( x .b0 , tfmfile ) ;
  putbyte ( x .b1 , tfmfile ) ;
  putbyte ( x .b2 , tfmfile ) ;
  putbyte ( x .b3 , tfmfile ) ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
openbasefile ( void ) 
#else
openbasefile ( ) 
#endif
{
  /* 40 10 */ register boolean Result; integer j  ;
  j = curinput .locfield ;
  if ( buffer [curinput .locfield ]== 38 ) 
  {
    incr ( curinput .locfield ) ;
    j = curinput .locfield ;
    buffer [last ]= 32 ;
    while ( buffer [j ]!= 32 ) incr ( j ) ;
    packbufferedname ( 0 , curinput .locfield , j - 1 ) ;
    if ( wopenin ( basefile ) ) 
    goto lab40 ;
    Fputs( stdout ,  "Sorry, I can't find the base `" ) ;
    fputs ( (char*) nameoffile + 1 , stdout ) ;
    Fputs( stdout ,  "'; will try `" ) ;
    fputs ( MFbasedefault + 1 , stdout ) ;
    fprintf( stdout , "%s\n",  "'." ) ;
    fflush ( stdout ) ;
  } 
  packbufferedname ( basedefaultlength - 5 , 1 , 0 ) ;
  if ( ! wopenin ( basefile ) ) 
  {
    ;
    Fputs( stdout ,  "I can't find the base file `" ) ;
    fputs ( MFbasedefault + 1 , stdout ) ;
    fprintf( stdout , "%s\n",  "'!" ) ;
    Result = false ;
    goto lab10 ;
  } 
  lab40: curinput .locfield = j ;
  Result = true ;
  lab10: ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
scanprimary ( void ) 
#else
scanprimary ( ) 
#endif
{
  /* 20 30 31 32 */ halfword p, q, r  ;
  quarterword c  ;
  char myvarflag  ;
  halfword ldelim, rdelim  ;
  integer groupline  ;
  scaled num, denom  ;
  halfword prehead, posthead, tail  ;
  smallnumber tt  ;
  halfword t  ;
  halfword macroref  ;
  myvarflag = varflag ;
  varflag = 0 ;
  lab20: {
      
    if ( aritherror ) 
    cleararith () ;
  } 
	;
#ifdef TEXMF_DEBUG
  if ( panicking ) 
  checkmem ( false ) ;
#endif /* TEXMF_DEBUG */
  if ( interrupt != 0 ) 
  if ( OKtointerrupt ) 
  {
    backinput () ;
    {
      if ( interrupt != 0 ) 
      pauseforinstructions () ;
    } 
    getxnext () ;
  } 
  switch ( curcmd ) 
  {case 31 : 
    {
      ldelim = cursym ;
      rdelim = curmod ;
      getxnext () ;
      scanexpression () ;
      if ( ( curcmd == 82 ) && ( curtype >= 16 ) ) 
      {
	p = getnode ( 2 ) ;
	mem [p ].hhfield .b0 = 14 ;
	mem [p ].hhfield .b1 = 11 ;
	initbignode ( p ) ;
	q = mem [p + 1 ].cint ;
	stashin ( q ) ;
	getxnext () ;
	scanexpression () ;
	if ( curtype < 16 ) 
	{
	  disperr ( 0 , 771 ) ;
	  {
	    helpptr = 4 ;
	    helpline [3 ]= 772 ;
	    helpline [2 ]= 773 ;
	    helpline [1 ]= 774 ;
	    helpline [0 ]= 775 ;
	  } 
	  putgetflusherror ( 0 ) ;
	} 
	stashin ( q + 2 ) ;
	checkdelimiter ( ldelim , rdelim ) ;
	curtype = 14 ;
	curexp = p ;
      } 
      else checkdelimiter ( ldelim , rdelim ) ;
    } 
    break ;
  case 32 : 
    {
      groupline = line ;
      if ( internal [7 ]> 0 ) 
      showcmdmod ( curcmd , curmod ) ;
      {
	p = getavail () ;
	mem [p ].hhfield .lhfield = 0 ;
	mem [p ].hhfield .v.RH = saveptr ;
	saveptr = p ;
      } 
      do {
	  dostatement () ;
      } while ( ! ( curcmd != 83 ) ) ;
      if ( curcmd != 84 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 261 ) ;
	  print ( 776 ) ;
	} 
	printint ( groupline ) ;
	print ( 777 ) ;
	{
	  helpptr = 2 ;
	  helpline [1 ]= 778 ;
	  helpline [0 ]= 779 ;
	} 
	backerror () ;
	curcmd = 84 ;
      } 
      unsave () ;
      if ( internal [7 ]> 0 ) 
      showcmdmod ( curcmd , curmod ) ;
    } 
    break ;
  case 39 : 
    {
      curtype = 4 ;
      curexp = curmod ;
    } 
    break ;
  case 42 : 
    {
      curexp = curmod ;
      curtype = 16 ;
      getxnext () ;
      if ( curcmd != 54 ) 
      {
	num = 0 ;
	denom = 0 ;
      } 
      else {
	  
	getxnext () ;
	if ( curcmd != 42 ) 
	{
	  backinput () ;
	  curcmd = 54 ;
	  curmod = 72 ;
	  cursym = 9761 ;
	  goto lab30 ;
	} 
	num = curexp ;
	denom = curmod ;
	if ( denom == 0 ) 
	{
	  {
	    if ( interaction == 3 ) 
	    ;
	    printnl ( 261 ) ;
	    print ( 780 ) ;
	  } 
	  {
	    helpptr = 1 ;
	    helpline [0 ]= 781 ;
	  } 
	  error () ;
	} 
	else curexp = makescaled ( num , denom ) ;
	{
	  if ( aritherror ) 
	  cleararith () ;
	} 
	getxnext () ;
      } 
      if ( curcmd >= 30 ) 
      if ( curcmd < 42 ) 
      {
	p = stashcurexp () ;
	scanprimary () ;
	if ( ( abs ( num ) >= abs ( denom ) ) || ( curtype < 14 ) ) 
	dobinary ( p , 71 ) ;
	else {
	    
	  fracmult ( num , denom ) ;
	  freenode ( p , 2 ) ;
	} 
      } 
      goto lab30 ;
    } 
    break ;
  case 33 : 
    donullary ( curmod ) ;
    break ;
  case 34 : 
  case 30 : 
  case 36 : 
  case 43 : 
    {
      c = curmod ;
      getxnext () ;
      scanprimary () ;
      dounary ( c ) ;
      goto lab30 ;
    } 
    break ;
  case 37 : 
    {
      c = curmod ;
      getxnext () ;
      scanexpression () ;
      if ( curcmd != 69 ) 
      {
	missingerr ( 478 ) ;
	print ( 712 ) ;
	printcmdmod ( 37 , c ) ;
	{
	  helpptr = 1 ;
	  helpline [0 ]= 713 ;
	} 
	backerror () ;
      } 
      p = stashcurexp () ;
      getxnext () ;
      scanprimary () ;
      dobinary ( p , c ) ;
      goto lab30 ;
    } 
    break ;
  case 35 : 
    {
      getxnext () ;
      scansuffix () ;
      oldsetting = selector ;
      selector = 5 ;
      showtokenlist ( curexp , 0 , 100000L , 0 ) ;
      flushtokenlist ( curexp ) ;
      curexp = makestring () ;
      selector = oldsetting ;
      curtype = 4 ;
      goto lab30 ;
    } 
    break ;
  case 40 : 
    {
      q = curmod ;
      if ( myvarflag == 77 ) 
      {
	getxnext () ;
	if ( curcmd == 77 ) 
	{
	  curexp = getavail () ;
	  mem [curexp ].hhfield .lhfield = q + 9769 ;
	  curtype = 20 ;
	  goto lab30 ;
	} 
	backinput () ;
      } 
      curtype = 16 ;
      curexp = internal [q ];
    } 
    break ;
  case 38 : 
    makeexpcopy ( curmod ) ;
    break ;
  case 41 : 
    {
      {
	prehead = avail ;
	if ( prehead == 0 ) 
	prehead = getavail () ;
	else {
	    
	  avail = mem [prehead ].hhfield .v.RH ;
	  mem [prehead ].hhfield .v.RH = 0 ;
	;
#ifdef STAT
	  incr ( dynused ) ;
#endif /* STAT */
	} 
      } 
      tail = prehead ;
      posthead = 0 ;
      tt = 1 ;
      while ( true ) {
	  
	t = curtok () ;
	mem [tail ].hhfield .v.RH = t ;
	if ( tt != 0 ) 
	{
	  {
	    p = mem [prehead ].hhfield .v.RH ;
	    q = mem [p ].hhfield .lhfield ;
	    tt = 0 ;
	    if ( eqtb [q ].lhfield % 86 == 41 ) 
	    {
	      q = eqtb [q ].v.RH ;
	      if ( q == 0 ) 
	      goto lab32 ;
	      while ( true ) {
		  
		p = mem [p ].hhfield .v.RH ;
		if ( p == 0 ) 
		{
		  tt = mem [q ].hhfield .b0 ;
		  goto lab32 ;
		} 
		if ( mem [q ].hhfield .b0 != 21 ) 
		goto lab32 ;
		q = mem [mem [q + 1 ].hhfield .lhfield ].hhfield .v.RH ;
		if ( p >= himemmin ) 
		{
		  do {
		      q = mem [q ].hhfield .v.RH ;
		  } while ( ! ( mem [q + 2 ].hhfield .lhfield >= mem [p ]
		  .hhfield .lhfield ) ) ;
		  if ( mem [q + 2 ].hhfield .lhfield > mem [p ].hhfield 
		  .lhfield ) 
		  goto lab32 ;
		} 
	      } 
	    } 
	    lab32: ;
	  } 
	  if ( tt >= 22 ) 
	  {
	    mem [tail ].hhfield .v.RH = 0 ;
	    if ( tt > 22 ) 
	    {
	      posthead = getavail () ;
	      tail = posthead ;
	      mem [tail ].hhfield .v.RH = t ;
	      tt = 0 ;
	      macroref = mem [q + 1 ].cint ;
	      incr ( mem [macroref ].hhfield .lhfield ) ;
	    } 
	    else {
		
	      p = getavail () ;
	      mem [prehead ].hhfield .lhfield = mem [prehead ].hhfield 
	      .v.RH ;
	      mem [prehead ].hhfield .v.RH = p ;
	      mem [p ].hhfield .lhfield = t ;
	      macrocall ( mem [q + 1 ].cint , prehead , 0 ) ;
	      getxnext () ;
	      goto lab20 ;
	    } 
	  } 
	} 
	getxnext () ;
	tail = t ;
	if ( curcmd == 63 ) 
	{
	  getxnext () ;
	  scanexpression () ;
	  if ( curcmd != 64 ) 
	  {
	    backinput () ;
	    backexpr () ;
	    curcmd = 63 ;
	    curmod = 0 ;
	    cursym = 9760 ;
	  } 
	  else {
	      
	    if ( curtype != 16 ) 
	    badsubscript () ;
	    curcmd = 42 ;
	    curmod = curexp ;
	    cursym = 0 ;
	  } 
	} 
	if ( curcmd > 42 ) 
	goto lab31 ;
	if ( curcmd < 40 ) 
	goto lab31 ;
      } 
      lab31: if ( posthead != 0 ) 
      {
	backinput () ;
	p = getavail () ;
	q = mem [posthead ].hhfield .v.RH ;
	mem [prehead ].hhfield .lhfield = mem [prehead ].hhfield .v.RH ;
	mem [prehead ].hhfield .v.RH = posthead ;
	mem [posthead ].hhfield .lhfield = q ;
	mem [posthead ].hhfield .v.RH = p ;
	mem [p ].hhfield .lhfield = mem [q ].hhfield .v.RH ;
	mem [q ].hhfield .v.RH = 0 ;
	macrocall ( macroref , prehead , 0 ) ;
	decr ( mem [macroref ].hhfield .lhfield ) ;
	getxnext () ;
	goto lab20 ;
      } 
      q = mem [prehead ].hhfield .v.RH ;
      {
	mem [prehead ].hhfield .v.RH = avail ;
	avail = prehead ;
	;
#ifdef STAT
	decr ( dynused ) ;
#endif /* STAT */
      } 
      if ( curcmd == myvarflag ) 
      {
	curtype = 20 ;
	curexp = q ;
	goto lab30 ;
      } 
      p = findvariable ( q ) ;
      if ( p != 0 ) 
      makeexpcopy ( p ) ;
      else {
	  
	obliterated ( q ) ;
	helpline [2 ]= 793 ;
	helpline [1 ]= 794 ;
	helpline [0 ]= 795 ;
	putgetflusherror ( 0 ) ;
      } 
      flushnodelist ( q ) ;
      goto lab30 ;
    } 
    break ;
    default: 
    {
      badexp ( 765 ) ;
      goto lab20 ;
    } 
    break ;
  } 
  getxnext () ;
  lab30: if ( curcmd == 63 ) 
  if ( curtype >= 16 ) 
  {
    p = stashcurexp () ;
    getxnext () ;
    scanexpression () ;
    if ( curcmd != 82 ) 
    {
      {
	backinput () ;
	backexpr () ;
	curcmd = 63 ;
	curmod = 0 ;
	cursym = 9760 ;
      } 
      unstashcurexp ( p ) ;
    } 
    else {
	
      q = stashcurexp () ;
      getxnext () ;
      scanexpression () ;
      if ( curcmd != 64 ) 
      {
	missingerr ( 93 ) ;
	{
	  helpptr = 3 ;
	  helpline [2 ]= 797 ;
	  helpline [1 ]= 798 ;
	  helpline [0 ]= 694 ;
	} 
	backerror () ;
      } 
      r = stashcurexp () ;
      makeexpcopy ( q ) ;
      dobinary ( r , 70 ) ;
      dobinary ( p , 71 ) ;
      dobinary ( q , 69 ) ;
      getxnext () ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
scansuffix ( void ) 
#else
scansuffix ( ) 
#endif
{
  /* 30 */ halfword h, t  ;
  halfword p  ;
  h = getavail () ;
  t = h ;
  while ( true ) {
      
    if ( curcmd == 63 ) 
    {
      getxnext () ;
      scanexpression () ;
      if ( curtype != 16 ) 
      badsubscript () ;
      if ( curcmd != 64 ) 
      {
	missingerr ( 93 ) ;
	{
	  helpptr = 3 ;
	  helpline [2 ]= 799 ;
	  helpline [1 ]= 798 ;
	  helpline [0 ]= 694 ;
	} 
	backerror () ;
      } 
      curcmd = 42 ;
      curmod = curexp ;
    } 
    if ( curcmd == 42 ) 
    p = newnumtok ( curmod ) ;
    else if ( ( curcmd == 41 ) || ( curcmd == 40 ) ) 
    {
      p = getavail () ;
      mem [p ].hhfield .lhfield = cursym ;
    } 
    else goto lab30 ;
    mem [t ].hhfield .v.RH = p ;
    t = p ;
    getxnext () ;
  } 
  lab30: curexp = mem [h ].hhfield .v.RH ;
  {
    mem [h ].hhfield .v.RH = avail ;
    avail = h ;
	;
#ifdef STAT
    decr ( dynused ) ;
#endif /* STAT */
  } 
  curtype = 20 ;
} 
void 
#ifdef HAVE_PROTOTYPES
scansecondary ( void ) 
#else
scansecondary ( ) 
#endif
{
  /* 20 22 */ halfword p  ;
  halfword c, d  ;
  halfword macname  ;
  lab20: if ( ( curcmd < 30 ) || ( curcmd > 43 ) ) 
  badexp ( 800 ) ;
  scanprimary () ;
  lab22: if ( curcmd <= 55 ) 
  if ( curcmd >= 52 ) 
  {
    p = stashcurexp () ;
    c = curmod ;
    d = curcmd ;
    if ( d == 53 ) 
    {
      macname = cursym ;
      incr ( mem [c ].hhfield .lhfield ) ;
    } 
    getxnext () ;
    scanprimary () ;
    if ( d != 53 ) 
    dobinary ( p , c ) ;
    else {
	
      backinput () ;
      binarymac ( p , c , macname ) ;
      decr ( mem [c ].hhfield .lhfield ) ;
      getxnext () ;
      goto lab20 ;
    } 
    goto lab22 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
scantertiary ( void ) 
#else
scantertiary ( ) 
#endif
{
  /* 20 22 */ halfword p  ;
  halfword c, d  ;
  halfword macname  ;
  lab20: if ( ( curcmd < 30 ) || ( curcmd > 43 ) ) 
  badexp ( 801 ) ;
  scansecondary () ;
  if ( curtype == 8 ) 
  materializepen () ;
  lab22: if ( curcmd <= 45 ) 
  if ( curcmd >= 43 ) 
  {
    p = stashcurexp () ;
    c = curmod ;
    d = curcmd ;
    if ( d == 44 ) 
    {
      macname = cursym ;
      incr ( mem [c ].hhfield .lhfield ) ;
    } 
    getxnext () ;
    scansecondary () ;
    if ( d != 44 ) 
    dobinary ( p , c ) ;
    else {
	
      backinput () ;
      binarymac ( p , c , macname ) ;
      decr ( mem [c ].hhfield .lhfield ) ;
      getxnext () ;
      goto lab20 ;
    } 
    goto lab22 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
scanexpression ( void ) 
#else
scanexpression ( ) 
#endif
{
  /* 20 30 22 25 26 10 */ halfword p, q, r, pp, qq  ;
  halfword c, d  ;
  char myvarflag  ;
  halfword macname  ;
  boolean cyclehit  ;
  scaled x, y  ;
  char t  ;
  myvarflag = varflag ;
  lab20: if ( ( curcmd < 30 ) || ( curcmd > 43 ) ) 
  badexp ( 804 ) ;
  scantertiary () ;
  lab22: if ( curcmd <= 51 ) 
  if ( curcmd >= 46 ) 
  if ( ( curcmd != 51 ) || ( myvarflag != 77 ) ) 
  {
    p = stashcurexp () ;
    c = curmod ;
    d = curcmd ;
    if ( d == 49 ) 
    {
      macname = cursym ;
      incr ( mem [c ].hhfield .lhfield ) ;
    } 
    if ( ( d < 48 ) || ( ( d == 48 ) && ( ( mem [p ].hhfield .b0 == 14 ) || 
    ( mem [p ].hhfield .b0 == 9 ) ) ) ) 
    {
      cyclehit = false ;
      {
	unstashcurexp ( p ) ;
	if ( curtype == 14 ) 
	p = newknot () ;
	else if ( curtype == 9 ) 
	p = curexp ;
	else goto lab10 ;
	q = p ;
	while ( mem [q ].hhfield .v.RH != p ) q = mem [q ].hhfield .v.RH ;
	if ( mem [p ].hhfield .b0 != 0 ) 
	{
	  r = copyknot ( p ) ;
	  mem [q ].hhfield .v.RH = r ;
	  q = r ;
	} 
	mem [p ].hhfield .b0 = 4 ;
	mem [q ].hhfield .b1 = 4 ;
      } 
      lab25: if ( curcmd == 46 ) 
      {
	t = scandirection () ;
	if ( t != 4 ) 
	{
	  mem [q ].hhfield .b1 = t ;
	  mem [q + 5 ].cint = curexp ;
	  if ( mem [q ].hhfield .b0 == 4 ) 
	  {
	    mem [q ].hhfield .b0 = t ;
	    mem [q + 3 ].cint = curexp ;
	  } 
	} 
      } 
      d = curcmd ;
      if ( d == 47 ) 
      {
	getxnext () ;
	if ( curcmd == 58 ) 
	{
	  getxnext () ;
	  y = curcmd ;
	  if ( curcmd == 59 ) 
	  getxnext () ;
	  scanprimary () ;
	  if ( ( curtype != 16 ) || ( curexp < 49152L ) ) 
	  {
	    disperr ( 0 , 822 ) ;
	    {
	      helpptr = 1 ;
	      helpline [0 ]= 823 ;
	    } 
	    putgetflusherror ( 65536L ) ;
	  } 
	  if ( y == 59 ) 
	  curexp = - (integer) curexp ;
	  mem [q + 6 ].cint = curexp ;
	  if ( curcmd == 52 ) 
	  {
	    getxnext () ;
	    y = curcmd ;
	    if ( curcmd == 59 ) 
	    getxnext () ;
	    scanprimary () ;
	    if ( ( curtype != 16 ) || ( curexp < 49152L ) ) 
	    {
	      disperr ( 0 , 822 ) ;
	      {
		helpptr = 1 ;
		helpline [0 ]= 823 ;
	      } 
	      putgetflusherror ( 65536L ) ;
	    } 
	    if ( y == 59 ) 
	    curexp = - (integer) curexp ;
	  } 
	  y = curexp ;
	} 
	else if ( curcmd == 57 ) 
	{
	  mem [q ].hhfield .b1 = 1 ;
	  t = 1 ;
	  getxnext () ;
	  scanprimary () ;
	  knownpair () ;
	  mem [q + 5 ].cint = curx ;
	  mem [q + 6 ].cint = cury ;
	  if ( curcmd != 52 ) 
	  {
	    x = mem [q + 5 ].cint ;
	    y = mem [q + 6 ].cint ;
	  } 
	  else {
	      
	    getxnext () ;
	    scanprimary () ;
	    knownpair () ;
	    x = curx ;
	    y = cury ;
	  } 
	} 
	else {
	    
	  mem [q + 6 ].cint = 65536L ;
	  y = 65536L ;
	  backinput () ;
	  goto lab30 ;
	} 
	if ( curcmd != 47 ) 
	{
	  missingerr ( 407 ) ;
	  {
	    helpptr = 1 ;
	    helpline [0 ]= 821 ;
	  } 
	  backerror () ;
	} 
	lab30: ;
      } 
      else if ( d != 48 ) 
      goto lab26 ;
      getxnext () ;
      if ( curcmd == 46 ) 
      {
	t = scandirection () ;
	if ( mem [q ].hhfield .b1 != 1 ) 
	x = curexp ;
	else t = 1 ;
      } 
      else if ( mem [q ].hhfield .b1 != 1 ) 
      {
	t = 4 ;
	x = 0 ;
      } 
      if ( curcmd == 36 ) 
      {
	cyclehit = true ;
	getxnext () ;
	pp = p ;
	qq = p ;
	if ( d == 48 ) 
	if ( p == q ) 
	{
	  d = 47 ;
	  mem [q + 6 ].cint = 65536L ;
	  y = 65536L ;
	} 
      } 
      else {
	  
	scantertiary () ;
	{
	  if ( curtype != 9 ) 
	  pp = newknot () ;
	  else pp = curexp ;
	  qq = pp ;
	  while ( mem [qq ].hhfield .v.RH != pp ) qq = mem [qq ].hhfield 
	  .v.RH ;
	  if ( mem [pp ].hhfield .b0 != 0 ) 
	  {
	    r = copyknot ( pp ) ;
	    mem [qq ].hhfield .v.RH = r ;
	    qq = r ;
	  } 
	  mem [pp ].hhfield .b0 = 4 ;
	  mem [qq ].hhfield .b1 = 4 ;
	} 
      } 
      {
	if ( d == 48 ) 
	if ( ( mem [q + 1 ].cint != mem [pp + 1 ].cint ) || ( mem [q + 2 
	].cint != mem [pp + 2 ].cint ) ) 
	{
	  {
	    if ( interaction == 3 ) 
	    ;
	    printnl ( 261 ) ;
	    print ( 824 ) ;
	  } 
	  {
	    helpptr = 3 ;
	    helpline [2 ]= 825 ;
	    helpline [1 ]= 826 ;
	    helpline [0 ]= 827 ;
	  } 
	  putgeterror () ;
	  d = 47 ;
	  mem [q + 6 ].cint = 65536L ;
	  y = 65536L ;
	} 
	if ( mem [pp ].hhfield .b1 == 4 ) 
	if ( ( t == 3 ) || ( t == 2 ) ) 
	{
	  mem [pp ].hhfield .b1 = t ;
	  mem [pp + 5 ].cint = x ;
	} 
	if ( d == 48 ) 
	{
	  if ( mem [q ].hhfield .b0 == 4 ) 
	  if ( mem [q ].hhfield .b1 == 4 ) 
	  {
	    mem [q ].hhfield .b0 = 3 ;
	    mem [q + 3 ].cint = 65536L ;
	  } 
	  if ( mem [pp ].hhfield .b1 == 4 ) 
	  if ( t == 4 ) 
	  {
	    mem [pp ].hhfield .b1 = 3 ;
	    mem [pp + 5 ].cint = 65536L ;
	  } 
	  mem [q ].hhfield .b1 = mem [pp ].hhfield .b1 ;
	  mem [q ].hhfield .v.RH = mem [pp ].hhfield .v.RH ;
	  mem [q + 5 ].cint = mem [pp + 5 ].cint ;
	  mem [q + 6 ].cint = mem [pp + 6 ].cint ;
	  freenode ( pp , 7 ) ;
	  if ( qq == pp ) 
	  qq = q ;
	} 
	else {
	    
	  if ( mem [q ].hhfield .b1 == 4 ) 
	  if ( ( mem [q ].hhfield .b0 == 3 ) || ( mem [q ].hhfield .b0 == 
	  2 ) ) 
	  {
	    mem [q ].hhfield .b1 = mem [q ].hhfield .b0 ;
	    mem [q + 5 ].cint = mem [q + 3 ].cint ;
	  } 
	  mem [q ].hhfield .v.RH = pp ;
	  mem [pp + 4 ].cint = y ;
	  if ( t != 4 ) 
	  {
	    mem [pp + 3 ].cint = x ;
	    mem [pp ].hhfield .b0 = t ;
	  } 
	} 
	q = qq ;
      } 
      if ( curcmd >= 46 ) 
      if ( curcmd <= 48 ) 
      if ( ! cyclehit ) 
      goto lab25 ;
      lab26: if ( cyclehit ) 
      {
	if ( d == 48 ) 
	p = q ;
      } 
      else {
	  
	mem [p ].hhfield .b0 = 0 ;
	if ( mem [p ].hhfield .b1 == 4 ) 
	{
	  mem [p ].hhfield .b1 = 3 ;
	  mem [p + 5 ].cint = 65536L ;
	} 
	mem [q ].hhfield .b1 = 0 ;
	if ( mem [q ].hhfield .b0 == 4 ) 
	{
	  mem [q ].hhfield .b0 = 3 ;
	  mem [q + 3 ].cint = 65536L ;
	} 
	mem [q ].hhfield .v.RH = p ;
      } 
      makechoices ( p ) ;
      curtype = 9 ;
      curexp = p ;
    } 
    else {
	
      getxnext () ;
      scantertiary () ;
      if ( d != 49 ) 
      dobinary ( p , c ) ;
      else {
	  
	backinput () ;
	binarymac ( p , c , macname ) ;
	decr ( mem [c ].hhfield .lhfield ) ;
	getxnext () ;
	goto lab20 ;
      } 
    } 
    goto lab22 ;
  } 
  lab10: ;
} 
void 
#ifdef HAVE_PROTOTYPES
getboolean ( void ) 
#else
getboolean ( ) 
#endif
{
  getxnext () ;
  scanexpression () ;
  if ( curtype != 2 ) 
  {
    disperr ( 0 , 828 ) ;
    {
      helpptr = 2 ;
      helpline [1 ]= 829 ;
      helpline [0 ]= 830 ;
    } 
    putgetflusherror ( 31 ) ;
    curtype = 2 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
printcapsule ( void ) 
#else
printcapsule ( ) 
#endif
{
  printchar ( 40 ) ;
  printexp ( gpointer , 0 ) ;
  printchar ( 41 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
tokenrecycle ( void ) 
#else
tokenrecycle ( ) 
#endif
{
  recyclevalue ( gpointer ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
closefilesandterminate ( void ) 
#else
closefilesandterminate ( ) 
#endif
{
  integer k  ;
  integer lh  ;
  short lkoffset  ;
  halfword p  ;
  scaled x  ;
	;
#ifdef STAT
  if ( internal [12 ]> 0 ) 
  if ( logopened ) 
  {
    fprintf( logfile , "%c\n",  ' ' ) ;
    fprintf( logfile , "%s%s\n",  "Here is how much of METAFONT's memory" , " you used:"     ) ;
    fprintf( logfile , "%c%ld%s",  ' ' , (long)maxstrptr - initstrptr , " string" ) ;
    if ( maxstrptr != initstrptr + 1 ) 
    putc ( 's' ,  logfile );
    fprintf( logfile , "%s%ld\n",  " out of " , (long)maxstrings - initstrptr ) ;
    fprintf( logfile , "%c%ld%s%ld\n",  ' ' , (long)maxpoolptr - initpoolptr ,     " string characters out of " , (long)poolsize - initpoolptr ) ;
    fprintf( logfile , "%c%ld%s%ld\n",  ' ' , (long)lomemmax + 0 + memend - himemmin + 2 ,     " words of memory out of " , (long)memend + 1 ) ;
    fprintf( logfile , "%c%ld%s%ld\n",  ' ' , (long)stcount , " symbolic tokens out of " , (long)9500 ) ;
    fprintf( logfile , "%c%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%c\n",  ' ' , (long)maxinstack , "i," , (long)intptr , "n," ,     (long)maxroundingptr , "r," , (long)maxparamstack , "p," , (long)maxbufstack + 1 ,     "b stack positions out of " , (long)stacksize , "i," , (long)maxinternal , "n," ,     (long)maxwiggle , "r," , (long)150 , "p," , (long)bufsize , 'b' ) ;
  } 
#endif /* STAT */
  if ( ( gfprevptr > 0 ) || ( internal [33 ]> 0 ) ) 
  {
    rover = 23 ;
    mem [rover ].hhfield .v.RH = 268435455L ;
    lomemmax = himemmin - 1 ;
    if ( lomemmax - rover > 268435455L ) 
    lomemmax = 268435455L + rover ;
    mem [rover ].hhfield .lhfield = lomemmax - rover ;
    mem [rover + 1 ].hhfield .lhfield = rover ;
    mem [rover + 1 ].hhfield .v.RH = rover ;
    mem [lomemmax ].hhfield .v.RH = 0 ;
    mem [lomemmax ].hhfield .lhfield = 0 ;
    mem [memtop - 1 ].hhfield .v.RH = 19 ;
    {register integer for_end; k = bc ;for_end = ec ; if ( k <= for_end) do 
      if ( charexists [k ]) 
      tfmwidth [k ]= sortin ( tfmwidth [k ]) ;
    while ( k++ < for_end ) ;} 
    nw = skimp ( 255 ) + 1 ;
    dimenhead [1 ]= mem [memtop - 1 ].hhfield .v.RH ;
    if ( perturbation >= 4096 ) 
    tfmwarning ( 20 ) ;
    fixdesignsize () ;
    fixchecksum () ;
    if ( internal [33 ]> 0 ) 
    {
      mem [memtop - 1 ].hhfield .v.RH = 19 ;
      {register integer for_end; k = bc ;for_end = ec ; if ( k <= for_end) 
      do 
	if ( charexists [k ]) 
	if ( tfmheight [k ]== 0 ) 
	tfmheight [k ]= 15 ;
	else tfmheight [k ]= sortin ( tfmheight [k ]) ;
      while ( k++ < for_end ) ;} 
      nh = skimp ( 15 ) + 1 ;
      dimenhead [2 ]= mem [memtop - 1 ].hhfield .v.RH ;
      if ( perturbation >= 4096 ) 
      tfmwarning ( 21 ) ;
      mem [memtop - 1 ].hhfield .v.RH = 19 ;
      {register integer for_end; k = bc ;for_end = ec ; if ( k <= for_end) 
      do 
	if ( charexists [k ]) 
	if ( tfmdepth [k ]== 0 ) 
	tfmdepth [k ]= 15 ;
	else tfmdepth [k ]= sortin ( tfmdepth [k ]) ;
      while ( k++ < for_end ) ;} 
      nd = skimp ( 15 ) + 1 ;
      dimenhead [3 ]= mem [memtop - 1 ].hhfield .v.RH ;
      if ( perturbation >= 4096 ) 
      tfmwarning ( 22 ) ;
      mem [memtop - 1 ].hhfield .v.RH = 19 ;
      {register integer for_end; k = bc ;for_end = ec ; if ( k <= for_end) 
      do 
	if ( charexists [k ]) 
	if ( tfmitalcorr [k ]== 0 ) 
	tfmitalcorr [k ]= 15 ;
	else tfmitalcorr [k ]= sortin ( tfmitalcorr [k ]) ;
      while ( k++ < for_end ) ;} 
      ni = skimp ( 63 ) + 1 ;
      dimenhead [4 ]= mem [memtop - 1 ].hhfield .v.RH ;
      if ( perturbation >= 4096 ) 
      tfmwarning ( 23 ) ;
      internal [33 ]= 0 ;
      if ( jobname == 0 ) 
      openlogfile () ;
      packjobname ( 1041 ) ;
      while ( ! bopenout ( tfmfile ) ) promptfilename ( 1042 , 1041 ) ;
      metricfilename = bmakenamestring ( tfmfile ) ;
      k = headersize ;
      while ( headerbyte [k ]< 0 ) decr ( k ) ;
      lh = ( k + 3 ) / 4 ;
      if ( bc > ec ) 
      bc = 1 ;
      bchar = roundunscaled ( internal [41 ]) ;
      if ( ( bchar < 0 ) || ( bchar > 255 ) ) 
      {
	bchar = -1 ;
	lkstarted = false ;
	lkoffset = 0 ;
      } 
      else {
	  
	lkstarted = true ;
	lkoffset = 1 ;
      } 
      k = labelptr ;
      if ( labelloc [k ]+ lkoffset > 255 ) 
      {
	lkoffset = 0 ;
	lkstarted = false ;
	do {
	    charremainder [labelchar [k ]]= lkoffset ;
	  while ( labelloc [k - 1 ]== labelloc [k ]) {
	      
	    decr ( k ) ;
	    charremainder [labelchar [k ]]= lkoffset ;
	  } 
	  incr ( lkoffset ) ;
	  decr ( k ) ;
	} while ( ! ( lkoffset + labelloc [k ]< 256 ) ) ;
      } 
      if ( lkoffset > 0 ) 
      while ( k > 0 ) {
	  
	charremainder [labelchar [k ]]= charremainder [labelchar [k ]]
	+ lkoffset ;
	decr ( k ) ;
      } 
      if ( bchlabel < ligtablesize ) 
      {
	ligkern [nl ].b0 = 255 ;
	ligkern [nl ].b1 = 0 ;
	ligkern [nl ].b2 = ( ( bchlabel + lkoffset ) / 256 ) ;
	ligkern [nl ].b3 = ( ( bchlabel + lkoffset ) % 256 ) ;
	incr ( nl ) ;
      } 
      put2bytes ( tfmfile , 6 + lh + ( ec - bc + 1 ) + nw + nh + nd + ni + nl 
      + lkoffset + nk + ne + np ) ;
      put2bytes ( tfmfile , lh ) ;
      put2bytes ( tfmfile , bc ) ;
      put2bytes ( tfmfile , ec ) ;
      put2bytes ( tfmfile , nw ) ;
      put2bytes ( tfmfile , nh ) ;
      put2bytes ( tfmfile , nd ) ;
      put2bytes ( tfmfile , ni ) ;
      put2bytes ( tfmfile , nl + lkoffset ) ;
      put2bytes ( tfmfile , nk ) ;
      put2bytes ( tfmfile , ne ) ;
      put2bytes ( tfmfile , np ) ;
      {register integer for_end; k = 1 ;for_end = 4 * lh ; if ( k <= 
      for_end) do 
	{
	  if ( headerbyte [k ]< 0 ) 
	  headerbyte [k ]= 0 ;
	  putbyte ( headerbyte [k ], tfmfile ) ;
	} 
      while ( k++ < for_end ) ;} 
      {register integer for_end; k = bc ;for_end = ec ; if ( k <= for_end) 
      do 
	if ( ! charexists [k ]) 
	put4bytes ( tfmfile , 0 ) ;
	else {
	    
	  putbyte ( mem [tfmwidth [k ]].hhfield .lhfield , tfmfile ) ;
	  putbyte ( ( mem [tfmheight [k ]].hhfield .lhfield ) * 16 + mem [
	  tfmdepth [k ]].hhfield .lhfield , tfmfile ) ;
	  putbyte ( ( mem [tfmitalcorr [k ]].hhfield .lhfield ) * 4 + 
	  chartag [k ], tfmfile ) ;
	  putbyte ( charremainder [k ], tfmfile ) ;
	} 
      while ( k++ < for_end ) ;} 
      tfmchanged = 0 ;
      {register integer for_end; k = 1 ;for_end = 4 ; if ( k <= for_end) do 
	{
	  put4bytes ( tfmfile , 0 ) ;
	  p = dimenhead [k ];
	  while ( p != 19 ) {
	      
	    put4bytes ( tfmfile , dimenout ( mem [p + 1 ].cint ) ) ;
	    p = mem [p ].hhfield .v.RH ;
	  } 
	} 
      while ( k++ < for_end ) ;} 
      {register integer for_end; k = 0 ;for_end = 255 ; if ( k <= for_end) 
      do 
	if ( skiptable [k ]< ligtablesize ) 
	{
	  printnl ( 1044 ) ;
	  printint ( k ) ;
	  print ( 1045 ) ;
	  ll = skiptable [k ];
	  do {
	      lll = ligkern [ll ].b0 ;
	    ligkern [ll ].b0 = 128 ;
	    ll = ll - lll ;
	  } while ( ! ( lll == 0 ) ) ;
	} 
      while ( k++ < for_end ) ;} 
      if ( lkstarted ) 
      {
	putbyte ( 255 , tfmfile ) ;
	putbyte ( bchar , tfmfile ) ;
	put2bytes ( tfmfile , 0 ) ;
      } 
      else {
	  register integer for_end; k = 1 ;for_end = lkoffset ; if ( k <= 
      for_end) do 
	{
	  ll = labelloc [labelptr ];
	  if ( bchar < 0 ) 
	  {
	    putbyte ( 254 , tfmfile ) ;
	    putbyte ( 0 , tfmfile ) ;
	  } 
	  else {
	      
	    putbyte ( 255 , tfmfile ) ;
	    putbyte ( bchar , tfmfile ) ;
	  } 
	  put2bytes ( tfmfile , ll + lkoffset ) ;
	  do {
	      decr ( labelptr ) ;
	  } while ( ! ( labelloc [labelptr ]< ll ) ) ;
	} 
      while ( k++ < for_end ) ;} 
      {register integer for_end; k = 0 ;for_end = nl - 1 ; if ( k <= 
      for_end) do 
	tfmqqqq ( ligkern [k ]) ;
      while ( k++ < for_end ) ;} 
      {register integer for_end; k = 0 ;for_end = nk - 1 ; if ( k <= 
      for_end) do 
	put4bytes ( tfmfile , dimenout ( kern [k ]) ) ;
      while ( k++ < for_end ) ;} 
      {register integer for_end; k = 0 ;for_end = ne - 1 ; if ( k <= 
      for_end) do 
	tfmqqqq ( exten [k ]) ;
      while ( k++ < for_end ) ;} 
      {register integer for_end; k = 1 ;for_end = np ; if ( k <= for_end) do 
	if ( k == 1 ) 
	if ( abs ( param [1 ]) < 134217728L ) 
	put4bytes ( tfmfile , param [1 ]* 16 ) ;
	else {
	    
	  incr ( tfmchanged ) ;
	  if ( param [1 ]> 0 ) 
	  put4bytes ( tfmfile , 2147483647L ) ;
	  else put4bytes ( tfmfile , -2147483647L ) ;
	} 
	else put4bytes ( tfmfile , dimenout ( param [k ]) ) ;
      while ( k++ < for_end ) ;} 
      if ( tfmchanged > 0 ) 
      {
	if ( tfmchanged == 1 ) 
	printnl ( 1046 ) ;
	else {
	    
	  printnl ( 40 ) ;
	  printint ( tfmchanged ) ;
	  print ( 1047 ) ;
	} 
	print ( 1048 ) ;
      } 
	;
#ifdef STAT
      if ( internal [12 ]> 0 ) 
      {
	fprintf( logfile , "%c\n",  ' ' ) ;
	if ( bchlabel < ligtablesize ) 
	decr ( nl ) ;
	fprintf( logfile , "%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s\n",  "(You used " , (long)nw , "w," , (long)nh , "h," , (long)nd , "d," ,         (long)ni , "i," , (long)nl , "l," , (long)nk , "k," , (long)ne , "e," , (long)np ,         "p metric file positions" ) ;
	fprintf( logfile , "%s%s%ld%s%ld%s%ld%s\n",  "  out of " , "256w,16h,16d,64i," , (long)ligtablesize ,         "l," , (long)maxkerns , "k,256e," , (long)maxfontdimen , "p)" ) ;
      } 
#endif /* STAT */
      printnl ( 1043 ) ;
      print ( metricfilename ) ;
      printchar ( 46 ) ;
      bclose ( tfmfile ) ;
    } 
    if ( gfprevptr > 0 ) 
    {
      {
	gfbuf [gfptr ]= 248 ;
	incr ( gfptr ) ;
	if ( gfptr == gflimit ) 
	gfswap () ;
      } 
      gffour ( gfprevptr ) ;
      gfprevptr = gfoffset + gfptr - 5 ;
      gffour ( internal [26 ]* 16 ) ;
      {register integer for_end; k = 1 ;for_end = 4 ; if ( k <= for_end) do 
	{
	  gfbuf [gfptr ]= headerbyte [k ];
	  incr ( gfptr ) ;
	  if ( gfptr == gflimit ) 
	  gfswap () ;
	} 
      while ( k++ < for_end ) ;} 
      gffour ( internal [27 ]) ;
      gffour ( internal [28 ]) ;
      gffour ( gfminm ) ;
      gffour ( gfmaxm ) ;
      gffour ( gfminn ) ;
      gffour ( gfmaxn ) ;
      {register integer for_end; k = 0 ;for_end = 255 ; if ( k <= for_end) 
      do 
	if ( charexists [k ]) 
	{
	  x = gfdx [k ]/ 65536L ;
	  if ( ( gfdy [k ]== 0 ) && ( x >= 0 ) && ( x < 256 ) && ( gfdx [k 
	  ]== x * 65536L ) ) 
	  {
	    {
	      gfbuf [gfptr ]= 246 ;
	      incr ( gfptr ) ;
	      if ( gfptr == gflimit ) 
	      gfswap () ;
	    } 
	    {
	      gfbuf [gfptr ]= k ;
	      incr ( gfptr ) ;
	      if ( gfptr == gflimit ) 
	      gfswap () ;
	    } 
	    {
	      gfbuf [gfptr ]= x ;
	      incr ( gfptr ) ;
	      if ( gfptr == gflimit ) 
	      gfswap () ;
	    } 
	  } 
	  else {
	      
	    {
	      gfbuf [gfptr ]= 245 ;
	      incr ( gfptr ) ;
	      if ( gfptr == gflimit ) 
	      gfswap () ;
	    } 
	    {
	      gfbuf [gfptr ]= k ;
	      incr ( gfptr ) ;
	      if ( gfptr == gflimit ) 
	      gfswap () ;
	    } 
	    gffour ( gfdx [k ]) ;
	    gffour ( gfdy [k ]) ;
	  } 
	  x = mem [tfmwidth [k ]+ 1 ].cint ;
	  if ( abs ( x ) > maxtfmdimen ) 
	  if ( x > 0 ) 
	  x = 16777215L ;
	  else x = -16777215L ;
	  else x = makescaled ( x * 16 , internal [26 ]) ;
	  gffour ( x ) ;
	  gffour ( charptr [k ]) ;
	} 
      while ( k++ < for_end ) ;} 
      {
	gfbuf [gfptr ]= 249 ;
	incr ( gfptr ) ;
	if ( gfptr == gflimit ) 
	gfswap () ;
      } 
      gffour ( gfprevptr ) ;
      {
	gfbuf [gfptr ]= 131 ;
	incr ( gfptr ) ;
	if ( gfptr == gflimit ) 
	gfswap () ;
      } 
      k = 4 + ( ( gfbufsize - gfptr ) % 4 ) ;
      while ( k > 0 ) {
	  
	{
	  gfbuf [gfptr ]= 223 ;
	  incr ( gfptr ) ;
	  if ( gfptr == gflimit ) 
	  gfswap () ;
	} 
	decr ( k ) ;
      } 
      if ( gflimit == halfbuf ) 
      writegf ( halfbuf , gfbufsize - 1 ) ;
      if ( gfptr > 0 ) 
      writegf ( 0 , gfptr - 1 ) ;
      printnl ( 1059 ) ;
      print ( outputfilename ) ;
      print ( 557 ) ;
      printint ( totalchars ) ;
      print ( 1060 ) ;
      if ( totalchars != 1 ) 
      printchar ( 115 ) ;
      print ( 1061 ) ;
      printint ( gfoffset + gfptr ) ;
      print ( 1062 ) ;
      bclose ( gffile ) ;
    } 
  } 
  if ( logopened ) 
  {
    putc ('\n',  logfile );
    aclose ( logfile ) ;
    selector = selector - 2 ;
    if ( selector == 1 ) 
    {
      printnl ( 1070 ) ;
      print ( texmflogname ) ;
      printchar ( 46 ) ;
    } 
  } 
  println () ;
  if ( ( editnamestart != 0 ) && ( interaction > 0 ) ) 
  calledit ( strpool , editnamestart , editnamelength , editline ) ;
} 
#ifdef TEXMF_DEBUG
void 
#ifdef HAVE_PROTOTYPES
debughelp ( void ) 
#else
debughelp ( ) 
#endif
{
  /* 888 10 */ integer k, l, m, n  ;
  while ( true ) {
      
    ;
    printnl ( 1077 ) ;
    fflush ( stdout ) ;
    read ( stdin , m ) ;
    if ( m < 0 ) 
    goto lab10 ;
    else if ( m == 0 ) 
    {
      goto lab888 ;
      lab888: m = 0 ;
    } 
    else {
	
      read ( stdin , n ) ;
      switch ( m ) 
      {case 1 : 
	printword ( mem [n ]) ;
	break ;
      case 2 : 
	printint ( mem [n ].hhfield .lhfield ) ;
	break ;
      case 3 : 
	printint ( mem [n ].hhfield .v.RH ) ;
	break ;
      case 4 : 
	{
	  printint ( eqtb [n ].lhfield ) ;
	  printchar ( 58 ) ;
	  printint ( eqtb [n ].v.RH ) ;
	} 
	break ;
      case 5 : 
	printvariablename ( n ) ;
	break ;
      case 6 : 
	printint ( internal [n ]) ;
	break ;
      case 7 : 
	doshowdependencies () ;
	break ;
      case 9 : 
	showtokenlist ( n , 0 , 100000L , 0 ) ;
	break ;
      case 10 : 
	print ( n ) ;
	break ;
      case 11 : 
	checkmem ( n > 0 ) ;
	break ;
      case 12 : 
	searchmem ( n ) ;
	break ;
      case 13 : 
	{
	  read ( stdin , l ) ;
	  printcmdmod ( n , l ) ;
	} 
	break ;
      case 14 : 
	{register integer for_end; k = 0 ;for_end = n ; if ( k <= for_end) 
	do 
	  print ( buffer [k ]) ;
	while ( k++ < for_end ) ;} 
	break ;
      case 15 : 
	panicking = ! panicking ;
	break ;
	default: 
	print ( 63 ) ;
	break ;
      } 
    } 
  } 
  lab10: ;
} 
#endif /* TEXMF_DEBUG */
