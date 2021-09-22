#define EXTERN extern
#include "mpd.h"

void 
#ifdef HAVE_PROTOTYPES
dospecial ( void ) 
#else
dospecial ( ) 
#endif
{
  getxnext () ;
  scanexpression () ;
  if ( curtype != 4 ) 
  {
    disperr ( 0 , 1148 ) ;
    {
      helpptr = 1 ;
      helpline [0 ]= 1149 ;
    } 
    putgeterror () ;
  } 
  else {
      
    mem [lastpending ].hhfield .v.RH = stashcurexp () ;
    lastpending = mem [lastpending ].hhfield .v.RH ;
    mem [lastpending ].hhfield .v.RH = 0 ;
  } 
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
  if ( curcmd > 45 ) 
  {
    if ( curcmd < 82 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 917 ) ;
      } 
      printcmdmod ( curcmd , curmod ) ;
      printchar ( 39 ) ;
      {
	helpptr = 5 ;
	helpline [4 ]= 918 ;
	helpline [3 ]= 919 ;
	helpline [2 ]= 920 ;
	helpline [1 ]= 921 ;
	helpline [0 ]= 922 ;
      } 
      backerror () ;
      getxnext () ;
    } 
  } 
  else if ( curcmd > 32 ) 
  {
    varflag = 76 ;
    scanexpression () ;
    if ( curcmd < 83 ) 
    {
      if ( curcmd == 53 ) 
      doequation () ;
      else if ( curcmd == 76 ) 
      doassignment () ;
      else if ( curtype == 4 ) 
      {
	if ( internal [1 ]> 0 ) 
	{
	  printnl ( 284 ) ;
	  print ( curexp ) ;
	  fflush ( stdout ) ;
	} 
      } 
      else if ( curtype != 1 ) 
      {
	disperr ( 0 , 927 ) ;
	{
	  helpptr = 3 ;
	  helpline [2 ]= 928 ;
	  helpline [1 ]= 929 ;
	  helpline [0 ]= 930 ;
	} 
	putgeterror () ;
      } 
      flushcurexp ( 0 ) ;
      curtype = 1 ;
    } 
  } 
  else {
      
    if ( internal [6 ]> 0 ) 
    showcmdmod ( curcmd , curmod ) ;
    switch ( curcmd ) 
    {case 32 : 
      dotypedeclaration () ;
      break ;
    case 18 : 
      if ( curmod > 2 ) 
      makeopdef () ;
      else if ( curmod > 0 ) 
      scandef () ;
      break ;
    case 26 : 
      dorandomseed () ;
      break ;
    case 25 : 
      {
	println () ;
	interaction = curmod ;
	if ( interaction == 0 ) 
	kpsemaketexdiscarderrors = 1 ;
	else kpsemaketexdiscarderrors = 0 ;
	if ( interaction == 0 ) 
	selector = 7 ;
	else selector = 8 ;
	if ( logopened ) 
	selector = selector + 2 ;
	getxnext () ;
      } 
      break ;
    case 23 : 
      doprotection () ;
      break ;
    case 29 : 
      defdelims () ;
      break ;
    case 14 : 
      do {
	  getsymbol () ;
	savevariable ( cursym ) ;
	getxnext () ;
      } while ( ! ( curcmd != 81 ) ) ;
      break ;
    case 15 : 
      dointerim () ;
      break ;
    case 16 : 
      dolet () ;
      break ;
    case 17 : 
      donewinternal () ;
      break ;
    case 24 : 
      doshowwhatever () ;
      break ;
    case 20 : 
      doaddto () ;
      break ;
    case 21 : 
      dobounds () ;
      break ;
    case 19 : 
      doshipout () ;
      break ;
    case 28 : 
      {
	getsymbol () ;
	startsym = cursym ;
	getxnext () ;
      } 
      break ;
    case 27 : 
      domessage () ;
      break ;
    case 31 : 
      dowrite () ;
      break ;
    case 22 : 
      dotfmcommand () ;
      break ;
    case 30 : 
      dospecial () ;
      break ;
    } 
    curtype = 1 ;
  } 
  if ( curcmd < 82 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 923 ) ;
    } 
    {
      helpptr = 6 ;
      helpline [5 ]= 924 ;
      helpline [4 ]= 925 ;
      helpline [3 ]= 926 ;
      helpline [2 ]= 920 ;
      helpline [1 ]= 921 ;
      helpline [0 ]= 922 ;
    } 
    backerror () ;
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
    } while ( ! ( curcmd > 81 ) ) ;
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
    if ( curcmd == 83 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 958 ) ;
      } 
      {
	helpptr = 2 ;
	helpline [1 ]= 959 ;
	helpline [0 ]= 730 ;
      } 
      flusherror ( 0 ) ;
    } 
  } while ( ! ( curcmd == 84 ) ) ;
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
  while ( p != 11 ) {
      
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
zcomputethreshold ( integer m ) 
#else
zcomputethreshold ( m ) 
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
  d = computethreshold ( m ) ;
  perturbation = 0 ;
  q = memtop - 1 ;
  m = 0 ;
  p = mem [memtop - 1 ].hhfield .v.RH ;
  while ( p != 11 ) {
      
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
  printnl ( 1088 ) ;
  print ( intname [m ]) ;
  print ( 1089 ) ;
  printscaled ( perturbation ) ;
  print ( 1090 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
fixdesignsize ( void ) 
#else
fixdesignsize ( ) 
#endif
{
  scaled d  ;
  d = internal [23 ];
  if ( ( d < 65536L ) || ( d >= 134217728L ) ) 
  {
    if ( d != 0 ) 
    printnl ( 1091 ) ;
    d = 8388608L ;
    internal [23 ]= d ;
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
  maxtfmdimen = 16 * internal [23 ]- internal [23 ]/ 2097152L ;
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
  else x = makescaled ( x * 16 , internal [23 ]) ;
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
openmemfile ( void ) 
#else
openmemfile ( ) 
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
    if ( wopenin ( memfile ) ) 
    goto lab40 ;
    Fputs( stdout ,  "Sorry, I can't find the mem file `" ) ;
    fputs ( (char*) nameoffile + 1 , stdout ) ;
    Fputs( stdout ,  "'; will try `" ) ;
    fputs ( MPmemdefault + 1 , stdout ) ;
    fprintf( stdout , "%s\n",  "'." ) ;
    fflush ( stdout ) ;
  } 
  packbufferedname ( memdefaultlength - 4 , 1 , 0 ) ;
  if ( ! wopenin ( memfile ) ) 
  {
    ;
    Fputs( stdout ,  "I can't find the mem file `" ) ;
    fputs ( MPmemdefault + 1 , stdout ) ;
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
  {case 33 : 
    {
      ldelim = cursym ;
      rdelim = curmod ;
      getxnext () ;
      scanexpression () ;
      if ( ( curcmd == 81 ) && ( curtype >= 16 ) ) 
      {
	p = stashcurexp () ;
	getxnext () ;
	scanexpression () ;
	if ( curtype < 16 ) 
	{
	  disperr ( 0 , 824 ) ;
	  {
	    helpptr = 4 ;
	    helpline [3 ]= 825 ;
	    helpline [2 ]= 826 ;
	    helpline [1 ]= 827 ;
	    helpline [0 ]= 828 ;
	  } 
	  putgetflusherror ( 0 ) ;
	} 
	q = getnode ( 2 ) ;
	mem [q ].hhfield .b1 = 14 ;
	if ( curcmd == 81 ) 
	mem [q ].hhfield .b0 = 13 ;
	else mem [q ].hhfield .b0 = 14 ;
	initbignode ( q ) ;
	r = mem [q + 1 ].cint ;
	stashin ( r + 2 ) ;
	unstashcurexp ( p ) ;
	stashin ( r ) ;
	if ( curcmd == 81 ) 
	{
	  getxnext () ;
	  scanexpression () ;
	  if ( curtype < 16 ) 
	  {
	    disperr ( 0 , 829 ) ;
	    {
	      helpptr = 3 ;
	      helpline [2 ]= 830 ;
	      helpline [1 ]= 827 ;
	      helpline [0 ]= 828 ;
	    } 
	    putgetflusherror ( 0 ) ;
	  } 
	  stashin ( r + 4 ) ;
	} 
	checkdelimiter ( ldelim , rdelim ) ;
	curtype = mem [q ].hhfield .b0 ;
	curexp = q ;
      } 
      else checkdelimiter ( ldelim , rdelim ) ;
    } 
    break ;
  case 34 : 
    {
      groupline = trueline () ;
      if ( internal [6 ]> 0 ) 
      showcmdmod ( curcmd , curmod ) ;
      {
	p = getavail () ;
	mem [p ].hhfield .lhfield = 0 ;
	mem [p ].hhfield .v.RH = saveptr ;
	saveptr = p ;
      } 
      do {
	  dostatement () ;
      } while ( ! ( curcmd != 82 ) ) ;
      if ( curcmd != 83 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 831 ) ;
	} 
	printint ( groupline ) ;
	print ( 832 ) ;
	{
	  helpptr = 2 ;
	  helpline [1 ]= 833 ;
	  helpline [0 ]= 834 ;
	} 
	backerror () ;
	curcmd = 83 ;
      } 
      unsave () ;
      if ( internal [6 ]> 0 ) 
      showcmdmod ( curcmd , curmod ) ;
    } 
    break ;
  case 41 : 
    {
      curtype = 4 ;
      curexp = curmod ;
    } 
    break ;
  case 44 : 
    {
      curexp = curmod ;
      curtype = 16 ;
      getxnext () ;
      if ( curcmd != 56 ) 
      {
	num = 0 ;
	denom = 0 ;
      } 
      else {
	  
	getxnext () ;
	if ( curcmd != 44 ) 
	{
	  backinput () ;
	  curcmd = 56 ;
	  curmod = 92 ;
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
	    printnl ( 262 ) ;
	    print ( 835 ) ;
	  } 
	  {
	    helpptr = 1 ;
	    helpline [0 ]= 836 ;
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
      if ( curcmd >= 32 ) 
      if ( curcmd < 44 ) 
      {
	p = stashcurexp () ;
	scanprimary () ;
	if ( ( abs ( num ) >= abs ( denom ) ) || ( curtype < 13 ) ) 
	dobinary ( p , 91 ) ;
	else {
	    
	  fracmult ( num , denom ) ;
	  freenode ( p , 2 ) ;
	} 
      } 
      goto lab30 ;
    } 
    break ;
  case 35 : 
    donullary ( curmod ) ;
    break ;
  case 36 : 
  case 32 : 
  case 38 : 
  case 45 : 
    {
      c = curmod ;
      getxnext () ;
      scanprimary () ;
      dounary ( c ) ;
      goto lab30 ;
    } 
    break ;
  case 39 : 
    {
      c = curmod ;
      getxnext () ;
      scanexpression () ;
      if ( curcmd != 70 ) 
      {
	missingerr ( 489 ) ;
	print ( 756 ) ;
	printcmdmod ( 39 , c ) ;
	{
	  helpptr = 1 ;
	  helpline [0 ]= 757 ;
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
  case 37 : 
    {
      getxnext () ;
      scansuffix () ;
      oldsetting = selector ;
      selector = 4 ;
      showtokenlist ( curexp , 0 , 100000L , 0 ) ;
      flushtokenlist ( curexp ) ;
      curexp = makestring () ;
      selector = oldsetting ;
      curtype = 4 ;
      goto lab30 ;
    } 
    break ;
  case 42 : 
    {
      q = curmod ;
      if ( myvarflag == 76 ) 
      {
	getxnext () ;
	if ( curcmd == 76 ) 
	{
	  curexp = getavail () ;
	  mem [curexp ].hhfield .lhfield = q + 9771 ;
	  curtype = 20 ;
	  goto lab30 ;
	} 
	backinput () ;
      } 
      curtype = 16 ;
      curexp = internal [q ];
    } 
    break ;
  case 40 : 
    makeexpcopy ( curmod ) ;
    break ;
  case 43 : 
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
	    if ( eqtb [q ].lhfield % 85 == 43 ) 
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
	if ( curcmd == 65 ) 
	{
	  getxnext () ;
	  scanexpression () ;
	  if ( curcmd != 66 ) 
	  {
	    backinput () ;
	    backexpr () ;
	    curcmd = 65 ;
	    curmod = 0 ;
	    cursym = 9760 ;
	  } 
	  else {
	      
	    if ( curtype != 16 ) 
	    badsubscript () ;
	    curcmd = 44 ;
	    curmod = curexp ;
	    cursym = 0 ;
	  } 
	} 
	if ( curcmd > 44 ) 
	goto lab31 ;
	if ( curcmd < 42 ) 
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
	helpline [2 ]= 848 ;
	helpline [1 ]= 849 ;
	helpline [0 ]= 850 ;
	putgetflusherror ( 0 ) ;
      } 
      flushnodelist ( q ) ;
      goto lab30 ;
    } 
    break ;
    default: 
    {
      badexp ( 818 ) ;
      goto lab20 ;
    } 
    break ;
  } 
  getxnext () ;
  lab30: if ( curcmd == 65 ) 
  if ( curtype >= 16 ) 
  {
    p = stashcurexp () ;
    getxnext () ;
    scanexpression () ;
    if ( curcmd != 81 ) 
    {
      {
	backinput () ;
	backexpr () ;
	curcmd = 65 ;
	curmod = 0 ;
	cursym = 9760 ;
      } 
      unstashcurexp ( p ) ;
    } 
    else {
	
      q = stashcurexp () ;
      getxnext () ;
      scanexpression () ;
      if ( curcmd != 66 ) 
      {
	missingerr ( 93 ) ;
	{
	  helpptr = 3 ;
	  helpline [2 ]= 852 ;
	  helpline [1 ]= 853 ;
	  helpline [0 ]= 738 ;
	} 
	backerror () ;
      } 
      r = stashcurexp () ;
      makeexpcopy ( q ) ;
      dobinary ( r , 90 ) ;
      dobinary ( p , 91 ) ;
      dobinary ( q , 89 ) ;
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
      
    if ( curcmd == 65 ) 
    {
      getxnext () ;
      scanexpression () ;
      if ( curtype != 16 ) 
      badsubscript () ;
      if ( curcmd != 66 ) 
      {
	missingerr ( 93 ) ;
	{
	  helpptr = 3 ;
	  helpline [2 ]= 854 ;
	  helpline [1 ]= 853 ;
	  helpline [0 ]= 738 ;
	} 
	backerror () ;
      } 
      curcmd = 44 ;
      curmod = curexp ;
    } 
    if ( curcmd == 44 ) 
    p = newnumtok ( curmod ) ;
    else if ( ( curcmd == 43 ) || ( curcmd == 42 ) ) 
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
  lab20: if ( ( curcmd < 32 ) || ( curcmd > 45 ) ) 
  badexp ( 855 ) ;
  scanprimary () ;
  lab22: if ( curcmd <= 57 ) 
  if ( curcmd >= 54 ) 
  {
    p = stashcurexp () ;
    c = curmod ;
    d = curcmd ;
    if ( d == 55 ) 
    {
      macname = cursym ;
      incr ( mem [c ].hhfield .lhfield ) ;
    } 
    getxnext () ;
    scanprimary () ;
    if ( d != 55 ) 
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
  lab20: if ( ( curcmd < 32 ) || ( curcmd > 45 ) ) 
  badexp ( 856 ) ;
  scansecondary () ;
  lab22: if ( curcmd <= 47 ) 
  if ( curcmd >= 45 ) 
  {
    p = stashcurexp () ;
    c = curmod ;
    d = curcmd ;
    if ( d == 46 ) 
    {
      macname = cursym ;
      incr ( mem [c ].hhfield .lhfield ) ;
    } 
    getxnext () ;
    scansecondary () ;
    if ( d != 46 ) 
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
  lab20: if ( ( curcmd < 32 ) || ( curcmd > 45 ) ) 
  badexp ( 857 ) ;
  scantertiary () ;
  lab22: if ( curcmd <= 53 ) 
  if ( curcmd >= 48 ) 
  if ( ( curcmd != 53 ) || ( myvarflag != 76 ) ) 
  {
    p = stashcurexp () ;
    c = curmod ;
    d = curcmd ;
    if ( d == 51 ) 
    {
      macname = cursym ;
      incr ( mem [c ].hhfield .lhfield ) ;
    } 
    if ( ( d < 50 ) || ( ( d == 50 ) && ( ( mem [p ].hhfield .b0 == 14 ) || 
    ( mem [p ].hhfield .b0 == 8 ) ) ) ) 
    {
      cyclehit = false ;
      {
	unstashcurexp ( p ) ;
	if ( curtype == 14 ) 
	p = newknot () ;
	else if ( curtype == 8 ) 
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
      lab25: if ( curcmd == 48 ) 
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
      if ( d == 49 ) 
      {
	getxnext () ;
	if ( curcmd == 60 ) 
	{
	  getxnext () ;
	  y = curcmd ;
	  if ( curcmd == 61 ) 
	  getxnext () ;
	  scanprimary () ;
	  if ( ( curtype != 16 ) || ( curexp < 49152L ) ) 
	  {
	    disperr ( 0 , 875 ) ;
	    {
	      helpptr = 1 ;
	      helpline [0 ]= 876 ;
	    } 
	    putgetflusherror ( 65536L ) ;
	  } 
	  if ( y == 61 ) 
	  curexp = - (integer) curexp ;
	  mem [q + 6 ].cint = curexp ;
	  if ( curcmd == 54 ) 
	  {
	    getxnext () ;
	    y = curcmd ;
	    if ( curcmd == 61 ) 
	    getxnext () ;
	    scanprimary () ;
	    if ( ( curtype != 16 ) || ( curexp < 49152L ) ) 
	    {
	      disperr ( 0 , 875 ) ;
	      {
		helpptr = 1 ;
		helpline [0 ]= 876 ;
	      } 
	      putgetflusherror ( 65536L ) ;
	    } 
	    if ( y == 61 ) 
	    curexp = - (integer) curexp ;
	  } 
	  y = curexp ;
	} 
	else if ( curcmd == 59 ) 
	{
	  mem [q ].hhfield .b1 = 1 ;
	  t = 1 ;
	  getxnext () ;
	  scanprimary () ;
	  knownpair () ;
	  mem [q + 5 ].cint = curx ;
	  mem [q + 6 ].cint = cury ;
	  if ( curcmd != 54 ) 
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
	if ( curcmd != 49 ) 
	{
	  missingerr ( 321 ) ;
	  {
	    helpptr = 1 ;
	    helpline [0 ]= 874 ;
	  } 
	  backerror () ;
	} 
	lab30: ;
      } 
      else if ( d != 50 ) 
      goto lab26 ;
      getxnext () ;
      if ( curcmd == 48 ) 
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
      if ( curcmd == 38 ) 
      {
	cyclehit = true ;
	getxnext () ;
	pp = p ;
	qq = p ;
	if ( d == 50 ) 
	if ( p == q ) 
	{
	  d = 49 ;
	  mem [q + 6 ].cint = 65536L ;
	  y = 65536L ;
	} 
      } 
      else {
	  
	scantertiary () ;
	{
	  if ( curtype != 8 ) 
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
	if ( d == 50 ) 
	if ( ( mem [q + 1 ].cint != mem [pp + 1 ].cint ) || ( mem [q + 2 
	].cint != mem [pp + 2 ].cint ) ) 
	{
	  {
	    if ( interaction == 3 ) 
	    ;
	    printnl ( 262 ) ;
	    print ( 877 ) ;
	  } 
	  {
	    helpptr = 3 ;
	    helpline [2 ]= 878 ;
	    helpline [1 ]= 879 ;
	    helpline [0 ]= 880 ;
	  } 
	  putgeterror () ;
	  d = 49 ;
	  mem [q + 6 ].cint = 65536L ;
	  y = 65536L ;
	} 
	if ( mem [pp ].hhfield .b1 == 4 ) 
	if ( ( t == 3 ) || ( t == 2 ) ) 
	{
	  mem [pp ].hhfield .b1 = t ;
	  mem [pp + 5 ].cint = x ;
	} 
	if ( d == 50 ) 
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
      if ( curcmd >= 48 ) 
      if ( curcmd <= 50 ) 
      if ( ! cyclehit ) 
      goto lab25 ;
      lab26: if ( cyclehit ) 
      {
	if ( d == 50 ) 
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
      curtype = 8 ;
      curexp = p ;
    } 
    else {
	
      getxnext () ;
      scantertiary () ;
      if ( d != 51 ) 
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
    disperr ( 0 , 881 ) ;
    {
      helpptr = 2 ;
      helpline [1 ]= 882 ;
      helpline [0 ]= 883 ;
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
  {register integer for_end; k = 0 ;for_end = readfiles - 1 ; if ( k <= 
  for_end) do 
    if ( rdfname [k ]!= 0 ) 
    aclose ( rdfile [k ]) ;
  while ( k++ < for_end ) ;} 
  {register integer for_end; k = 0 ;for_end = writefiles - 1 ; if ( k <= 
  for_end) do 
    if ( wrfname [k ]!= 0 ) 
    aclose ( wrfile [k ]) ;
  while ( k++ < for_end ) ;} 
	;
#ifdef STAT
  if ( internal [10 ]> 0 ) 
  if ( logopened ) 
  {
    fprintf( logfile , "%c\n",  ' ' ) ;
    fprintf( logfile , "%s%s\n",  "Here is how much of MetaPost's memory" , " you used:"     ) ;
    fprintf( logfile , "%c%ld%s",  ' ' , (long)maxstrsused - initstruse , " string" ) ;
    if ( maxstrsused != initstruse + 1 ) 
    putc ( 's' ,  logfile );
    fprintf( logfile , "%s%ld\n",  " out of " , (long)maxstrings - 1 - initstruse ) ;
    fprintf( logfile , "%c%ld%s%ld\n",  ' ' , (long)maxplused - initpoolptr ,     " string characters out of " , (long)poolsize - initpoolptr ) ;
    fprintf( logfile , "%c%ld%s%ld\n",  ' ' , (long)lomemmax + 0 + memend - himemmin + 2 ,     " words of memory out of " , (long)memend + 1 ) ;
    fprintf( logfile , "%c%ld%s%ld\n",  ' ' , (long)stcount , " symbolic tokens out of " , (long)9500 ) ;
    fprintf( logfile , "%c%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%c\n",  ' ' , (long)maxinstack , "i," , (long)intptr , "n," ,     (long)maxparamstack , "p," , (long)maxbufstack + 1 , "b stack positions out of " ,     (long)stacksize , "i," , (long)maxinternal , "n," , (long)150 , "p," , (long)bufsize , 'b' ) ;
    fprintf( logfile , "%c%ld%s%ld%s%ld%s\n",  ' ' , (long)pactcount , " string compactions (moved " ,     (long)pactchars , " characters, " , (long)pactstrs , " strings)" ) ;
  } 
#endif /* STAT */
  if ( internal [26 ]> 0 ) 
  {
    rover = 24 ;
    mem [rover ].hhfield .v.RH = 268435455L ;
    lomemmax = himemmin - 1 ;
    if ( lomemmax - rover > 268435455L ) 
    lomemmax = 268435455L + rover ;
    mem [rover ].hhfield .lhfield = lomemmax - rover ;
    mem [rover + 1 ].hhfield .lhfield = rover ;
    mem [rover + 1 ].hhfield .v.RH = rover ;
    mem [lomemmax ].hhfield .v.RH = 0 ;
    mem [lomemmax ].hhfield .lhfield = 0 ;
    mem [memtop - 1 ].hhfield .v.RH = 11 ;
    {register integer for_end; k = bc ;for_end = ec ; if ( k <= for_end) do 
      if ( charexists [k ]) 
      tfmwidth [k ]= sortin ( tfmwidth [k ]) ;
    while ( k++ < for_end ) ;} 
    nw = skimp ( 255 ) + 1 ;
    dimenhead [1 ]= mem [memtop - 1 ].hhfield .v.RH ;
    if ( perturbation >= 4096 ) 
    tfmwarning ( 19 ) ;
    fixdesignsize () ;
    fixchecksum () ;
    mem [memtop - 1 ].hhfield .v.RH = 11 ;
    {register integer for_end; k = bc ;for_end = ec ; if ( k <= for_end) do 
      if ( charexists [k ]) 
      if ( tfmheight [k ]== 0 ) 
      tfmheight [k ]= 7 ;
      else tfmheight [k ]= sortin ( tfmheight [k ]) ;
    while ( k++ < for_end ) ;} 
    nh = skimp ( 15 ) + 1 ;
    dimenhead [2 ]= mem [memtop - 1 ].hhfield .v.RH ;
    if ( perturbation >= 4096 ) 
    tfmwarning ( 20 ) ;
    mem [memtop - 1 ].hhfield .v.RH = 11 ;
    {register integer for_end; k = bc ;for_end = ec ; if ( k <= for_end) do 
      if ( charexists [k ]) 
      if ( tfmdepth [k ]== 0 ) 
      tfmdepth [k ]= 7 ;
      else tfmdepth [k ]= sortin ( tfmdepth [k ]) ;
    while ( k++ < for_end ) ;} 
    nd = skimp ( 15 ) + 1 ;
    dimenhead [3 ]= mem [memtop - 1 ].hhfield .v.RH ;
    if ( perturbation >= 4096 ) 
    tfmwarning ( 21 ) ;
    mem [memtop - 1 ].hhfield .v.RH = 11 ;
    {register integer for_end; k = bc ;for_end = ec ; if ( k <= for_end) do 
      if ( charexists [k ]) 
      if ( tfmitalcorr [k ]== 0 ) 
      tfmitalcorr [k ]= 7 ;
      else tfmitalcorr [k ]= sortin ( tfmitalcorr [k ]) ;
    while ( k++ < for_end ) ;} 
    ni = skimp ( 63 ) + 1 ;
    dimenhead [4 ]= mem [memtop - 1 ].hhfield .v.RH ;
    if ( perturbation >= 4096 ) 
    tfmwarning ( 22 ) ;
    internal [26 ]= 0 ;
    if ( jobname == 0 ) 
    openlogfile () ;
    packjobname ( 1092 ) ;
    while ( ! bopenout ( tfmfile ) ) promptfilename ( 1093 , 1092 ) ;
    metricfilename = bmakenamestring ( tfmfile ) ;
    k = headersize ;
    while ( headerbyte [k ]< 0 ) decr ( k ) ;
    lh = ( k + 3 ) / 4 ;
    if ( bc > ec ) 
    bc = 1 ;
    bchar = roundunscaled ( internal [31 ]) ;
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
	
      charremainder [labelchar [k ]]= charremainder [labelchar [k ]]+ 
      lkoffset ;
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
    put2bytes ( tfmfile , 6 + lh + ( ec - bc + 1 ) + nw + nh + nd + ni + nl + 
    lkoffset + nk + ne + np ) ;
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
    {register integer for_end; k = 1 ;for_end = 4 * lh ; if ( k <= for_end) 
    do 
      {
	if ( headerbyte [k ]< 0 ) 
	headerbyte [k ]= 0 ;
	putbyte ( headerbyte [k ], tfmfile ) ;
      } 
    while ( k++ < for_end ) ;} 
    {register integer for_end; k = bc ;for_end = ec ; if ( k <= for_end) do 
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
	while ( p != 11 ) {
	    
	  put4bytes ( tfmfile , dimenout ( mem [p + 1 ].cint ) ) ;
	  p = mem [p ].hhfield .v.RH ;
	} 
      } 
    while ( k++ < for_end ) ;} 
    {register integer for_end; k = 0 ;for_end = 255 ; if ( k <= for_end) do 
      if ( skiptable [k ]< ligtablesize ) 
      {
	printnl ( 1095 ) ;
	printint ( k ) ;
	print ( 1096 ) ;
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
    {register integer for_end; k = 0 ;for_end = nl - 1 ; if ( k <= for_end) 
    do 
      tfmqqqq ( ligkern [k ]) ;
    while ( k++ < for_end ) ;} 
    {register integer for_end; k = 0 ;for_end = nk - 1 ; if ( k <= for_end) 
    do 
      put4bytes ( tfmfile , dimenout ( kern [k ]) ) ;
    while ( k++ < for_end ) ;} 
    {register integer for_end; k = 0 ;for_end = ne - 1 ; if ( k <= for_end) 
    do 
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
      printnl ( 1097 ) ;
      else {
	  
	printnl ( 40 ) ;
	printint ( tfmchanged ) ;
	print ( 1098 ) ;
      } 
      print ( 1099 ) ;
    } 
	;
#ifdef STAT
    if ( internal [10 ]> 0 ) 
    {
      fprintf( logfile , "%c\n",  ' ' ) ;
      if ( bchlabel < ligtablesize ) 
      decr ( nl ) ;
      fprintf( logfile , "%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s\n",  "(You used " , (long)nw , "w," , (long)nh , "h," , (long)nd , "d," ,       (long)ni , "i," , (long)nl , "l," , (long)nk , "k," , (long)ne , "e," , (long)np ,       "p metric file positions" ) ;
      fprintf( logfile , "%s%s%ld%s%ld%s%ld%s\n",  "  out of " , "256w,16h,16d,64i," , (long)ligtablesize ,       "l," , (long)maxkerns , "k,256e," , (long)maxfontdimen , "p)" ) ;
    } 
#endif /* STAT */
    printnl ( 1094 ) ;
    print ( metricfilename ) ;
    printchar ( 46 ) ;
    bclose ( tfmfile ) ;
  } 
  if ( totalshipped > 0 ) 
  {
    printnl ( 284 ) ;
    printint ( totalshipped ) ;
    print ( 1116 ) ;
    if ( totalshipped > 1 ) 
    printchar ( 115 ) ;
    print ( 1117 ) ;
    print ( firstfilename ) ;
    if ( totalshipped > 1 ) 
    {
      if ( 31 + ( strstart [nextstr [firstfilename ]]- strstart [
      firstfilename ]) + ( strstart [nextstr [lastfilename ]]- strstart [
      lastfilename ]) > maxprintline ) 
      println () ;
      print ( 548 ) ;
      print ( lastfilename ) ;
    } 
  } 
  if ( logopened ) 
  {
    putc ('\n',  logfile );
    aclose ( logfile ) ;
    selector = selector - 2 ;
    if ( selector == 8 ) 
    {
      printnl ( 1181 ) ;
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
    printnl ( 1188 ) ;
    fflush ( stdout ) ;
    m = inputint ( stdin ) ;
    if ( m < 0 ) 
    goto lab10 ;
    else if ( m == 0 ) 
    {
      goto lab888 ;
      lab888: m = 0 ;
    } 
    else {
	
      n = inputint ( stdin ) ;
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
	  l = inputint ( stdin ) ;
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
