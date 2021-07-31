#define EXTERN extern
#include "mfd.h"

void ztfmqqqq ( x ) 
fourquarters x ; 
{bwritebyte ( tfmfile , x .b0 ) ; 
  bwritebyte ( tfmfile , x .b1 ) ; 
  bwritebyte ( tfmfile , x .b2 ) ; 
  bwritebyte ( tfmfile , x .b3 ) ; 
} 
boolean openbasefile ( ) 
{/* 40 10 */ register boolean Result; integer j  ; 
  j = curinput .locfield ; 
  if ( buffer [ curinput .locfield ] == 38 ) 
  {
    incr ( curinput .locfield ) ; 
    j = curinput .locfield ; 
    buffer [ last ] = 32 ; 
    while ( buffer [ j ] != 32 ) incr ( j ) ; 
    packbufferedname ( 0 , curinput .locfield , j - 1 ) ; 
    if ( wopenin ( basefile ) ) 
    goto lab40 ; 
    (void) fprintf( stdout , "%s%s\n",  "Sorry, I can't find that base;" ,     " will try the default." ) ; 
    flush ( stdout ) ; 
  } 
  packbufferedname ( basedefaultlength - 5 , 1 , 0 ) ; 
  if ( ! wopenin ( basefile ) ) 
  {
    ; 
    (void) fprintf( stdout , "%s\n",  "I can't find the default base file!" ) ; 
    Result = false ; 
    goto lab10 ; 
  } 
  lab40: curinput .locfield = j ; 
  Result = true ; 
  lab10: ; 
  return(Result) ; 
} 
boolean loadbasefile ( ) 
{/* 6666 10 */ register boolean Result; integer k  ; 
  halfword p, q  ; 
  integer x  ; 
  fourquarters w  ; 
  undumpint ( x ) ; 
  if ( x != 426472440L ) 
  goto lab6666 ; 
  undumpint ( x ) ; 
  if ( x != 0 ) 
  goto lab6666 ; 
  undumpint ( x ) ; 
  if ( x != memtop ) 
  goto lab6666 ; 
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
  {register integer for_end; k = 0 ; for_end = strptr ; if ( k <= for_end) do 
    {
      {
	undumpint ( x ) ; 
	if ( ( x < 0 ) || ( x > poolptr ) ) 
	goto lab6666 ; 
	else strstart [ k ] = x ; 
      } 
      strref [ k ] = 127 ; 
    } 
  while ( k++ < for_end ) ; } 
  k = 0 ; 
  while ( k + 4 < poolptr ) {
      
    undumpqqqq ( w ) ; 
    strpool [ k ] = w .b0 ; 
    strpool [ k + 1 ] = w .b1 ; 
    strpool [ k + 2 ] = w .b2 ; 
    strpool [ k + 3 ] = w .b3 ; 
    k = k + 4 ; 
  } 
  k = poolptr - 4 ; 
  undumpqqqq ( w ) ; 
  strpool [ k ] = w .b0 ; 
  strpool [ k + 1 ] = w .b1 ; 
  strpool [ k + 2 ] = w .b2 ; 
  strpool [ k + 3 ] = w .b3 ; 
  initstrptr = strptr ; 
  initpoolptr = poolptr ; 
  maxstrptr = strptr ; 
  maxpoolptr = poolptr ; 
  {
    undumpint ( x ) ; 
    if ( ( x < 1022 ) || ( x > memtop - 3 ) ) 
    goto lab6666 ; 
    else lomemmax = x ; 
  } 
  {
    undumpint ( x ) ; 
    if ( ( x < 23 ) || ( x > lomemmax ) ) 
    goto lab6666 ; 
    else rover = x ; 
  } 
  p = 0 ; 
  q = rover ; 
  do {
      { register integer for_end; k = p ; for_end = q + 1 ; if ( k <= 
    for_end) do 
      undumpwd ( mem [ k ] ) ; 
    while ( k++ < for_end ) ; } 
    p = q + mem [ q ] .hhfield .lhfield ; 
    if ( ( p > lomemmax ) || ( ( q >= mem [ q + 1 ] .hhfield .v.RH ) && ( mem 
    [ q + 1 ] .hhfield .v.RH != rover ) ) ) 
    goto lab6666 ; 
    q = mem [ q + 1 ] .hhfield .v.RH ; 
  } while ( ! ( q == rover ) ) ; 
  {register integer for_end; k = p ; for_end = lomemmax ; if ( k <= for_end) 
  do 
    undumpwd ( mem [ k ] ) ; 
  while ( k++ < for_end ) ; } 
  {
    undumpint ( x ) ; 
    if ( ( x < lomemmax + 1 ) || ( x > memtop - 2 ) ) 
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
  {register integer for_end; k = himemmin ; for_end = memend ; if ( k <= 
  for_end) do 
    undumpwd ( mem [ k ] ) ; 
  while ( k++ < for_end ) ; } 
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
    undumphh ( hash [ p ] ) ; 
    undumphh ( eqtb [ p ] ) ; 
  } while ( ! ( p == hashused ) ) ; 
  {register integer for_end; p = hashused + 1 ; for_end = 9769 ; if ( p <= 
  for_end) do 
    {
      undumphh ( hash [ p ] ) ; 
      undumphh ( eqtb [ p ] ) ; 
    } 
  while ( p++ < for_end ) ; } 
  undumpint ( stcount ) ; 
  {
    undumpint ( x ) ; 
    if ( ( x < 41 ) || ( x > maxinternal ) ) 
    goto lab6666 ; 
    else intptr = x ; 
  } 
  {register integer for_end; k = 1 ; for_end = intptr ; if ( k <= for_end) do 
    {
      undumpint ( internal [ k ] ) ; 
      {
	undumpint ( x ) ; 
	if ( ( x < 0 ) || ( x > strptr ) ) 
	goto lab6666 ; 
	else intname [ k ] = x ; 
      } 
    } 
  while ( k++ < for_end ) ; } 
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
  {
    undumpint ( x ) ; 
    if ( ( x < 0 ) || ( x > strptr ) ) 
    goto lab6666 ; 
    else baseident = x ; 
  } 
  {
    undumpint ( x ) ; 
    if ( ( x < 1 ) || ( x > 9769 ) ) 
    goto lab6666 ; 
    else bgloc = x ; 
  } 
  {
    undumpint ( x ) ; 
    if ( ( x < 1 ) || ( x > 9769 ) ) 
    goto lab6666 ; 
    else egloc = x ; 
  } 
  undumpint ( serialno ) ; 
  undumpint ( x ) ; 
  if ( ( x != 69069L ) || feof ( basefile ) ) 
  goto lab6666 ; 
  Result = true ; 
  goto lab10 ; 
  lab6666: ; 
  (void) fprintf( stdout , "%s\n",  "(Fatal base file error; I'm stymied)" ) ; 
  Result = false ; 
  lab10: ; 
  return(Result) ; 
} 
void scanprimary ( ) 
{/* 20 30 31 32 */ halfword p, q, r  ; 
  quarterword c  ; 
  schar myvarflag  ; 
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
#ifdef DEBUG
  if ( panicking ) 
  checkmem ( false ) ; 
#endif /* DEBUG */
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
	mem [ p ] .hhfield .b0 = 14 ; 
	mem [ p ] .hhfield .b1 = 11 ; 
	initbignode ( p ) ; 
	q = mem [ p + 1 ] .cint ; 
	stashin ( q ) ; 
	getxnext () ; 
	scanexpression () ; 
	if ( curtype < 16 ) 
	{
	  disperr ( 0 , 772 ) ; 
	  {
	    helpptr = 4 ; 
	    helpline [ 3 ] = 773 ; 
	    helpline [ 2 ] = 774 ; 
	    helpline [ 1 ] = 775 ; 
	    helpline [ 0 ] = 776 ; 
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
      if ( internal [ 7 ] > 0 ) 
      showcmdmod ( curcmd , curmod ) ; 
      {
	p = getavail () ; 
	mem [ p ] .hhfield .lhfield = 0 ; 
	mem [ p ] .hhfield .v.RH = saveptr ; 
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
	  print ( 777 ) ; 
	} 
	printint ( groupline ) ; 
	print ( 778 ) ; 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 779 ; 
	  helpline [ 0 ] = 780 ; 
	} 
	backerror () ; 
	curcmd = 84 ; 
      } 
      unsave () ; 
      if ( internal [ 7 ] > 0 ) 
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
	    print ( 781 ) ; 
	  } 
	  {
	    helpptr = 1 ; 
	    helpline [ 0 ] = 782 ; 
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
	print ( 713 ) ; 
	printcmdmod ( 37 , c ) ; 
	{
	  helpptr = 1 ; 
	  helpline [ 0 ] = 714 ; 
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
	  mem [ curexp ] .hhfield .lhfield = q + 9769 ; 
	  curtype = 20 ; 
	  goto lab30 ; 
	} 
	backinput () ; 
      } 
      curtype = 16 ; 
      curexp = internal [ q ] ; 
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
	    
	  avail = mem [ prehead ] .hhfield .v.RH ; 
	  mem [ prehead ] .hhfield .v.RH = 0 ; 
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
	mem [ tail ] .hhfield .v.RH = t ; 
	if ( tt != 0 ) 
	{
	  {
	    p = mem [ prehead ] .hhfield .v.RH ; 
	    q = mem [ p ] .hhfield .lhfield ; 
	    tt = 0 ; 
	    if ( eqtb [ q ] .lhfield % 86 == 41 ) 
	    {
	      q = eqtb [ q ] .v.RH ; 
	      if ( q == 0 ) 
	      goto lab32 ; 
	      while ( true ) {
		  
		p = mem [ p ] .hhfield .v.RH ; 
		if ( p == 0 ) 
		{
		  tt = mem [ q ] .hhfield .b0 ; 
		  goto lab32 ; 
		} 
		if ( mem [ q ] .hhfield .b0 != 21 ) 
		goto lab32 ; 
		q = mem [ mem [ q + 1 ] .hhfield .lhfield ] .hhfield .v.RH ; 
		if ( p >= himemmin ) 
		{
		  do {
		      q = mem [ q ] .hhfield .v.RH ; 
		  } while ( ! ( mem [ q + 2 ] .hhfield .lhfield >= mem [ p ] 
		  .hhfield .lhfield ) ) ; 
		  if ( mem [ q + 2 ] .hhfield .lhfield > mem [ p ] .hhfield 
		  .lhfield ) 
		  goto lab32 ; 
		} 
	      } 
	    } 
	    lab32: ; 
	  } 
	  if ( tt >= 22 ) 
	  {
	    mem [ tail ] .hhfield .v.RH = 0 ; 
	    if ( tt > 22 ) 
	    {
	      posthead = getavail () ; 
	      tail = posthead ; 
	      mem [ tail ] .hhfield .v.RH = t ; 
	      tt = 0 ; 
	      macroref = mem [ q + 1 ] .cint ; 
	      incr ( mem [ macroref ] .hhfield .lhfield ) ; 
	    } 
	    else {
		
	      p = getavail () ; 
	      mem [ prehead ] .hhfield .lhfield = mem [ prehead ] .hhfield 
	      .v.RH ; 
	      mem [ prehead ] .hhfield .v.RH = p ; 
	      mem [ p ] .hhfield .lhfield = t ; 
	      macrocall ( mem [ q + 1 ] .cint , prehead , 0 ) ; 
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
	q = mem [ posthead ] .hhfield .v.RH ; 
	mem [ prehead ] .hhfield .lhfield = mem [ prehead ] .hhfield .v.RH ; 
	mem [ prehead ] .hhfield .v.RH = posthead ; 
	mem [ posthead ] .hhfield .lhfield = q ; 
	mem [ posthead ] .hhfield .v.RH = p ; 
	mem [ p ] .hhfield .lhfield = mem [ q ] .hhfield .v.RH ; 
	mem [ q ] .hhfield .v.RH = 0 ; 
	macrocall ( macroref , prehead , 0 ) ; 
	decr ( mem [ macroref ] .hhfield .lhfield ) ; 
	getxnext () ; 
	goto lab20 ; 
      } 
      q = mem [ prehead ] .hhfield .v.RH ; 
      {
	mem [ prehead ] .hhfield .v.RH = avail ; 
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
	helpline [ 2 ] = 794 ; 
	helpline [ 1 ] = 795 ; 
	helpline [ 0 ] = 796 ; 
	putgetflusherror ( 0 ) ; 
      } 
      flushnodelist ( q ) ; 
      goto lab30 ; 
    } 
    break ; 
    default: 
    {
      badexp ( 766 ) ; 
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
	  helpline [ 2 ] = 798 ; 
	  helpline [ 1 ] = 799 ; 
	  helpline [ 0 ] = 695 ; 
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
void scansuffix ( ) 
{/* 30 */ halfword h, t  ; 
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
	  helpline [ 2 ] = 800 ; 
	  helpline [ 1 ] = 799 ; 
	  helpline [ 0 ] = 695 ; 
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
      mem [ p ] .hhfield .lhfield = cursym ; 
    } 
    else goto lab30 ; 
    mem [ t ] .hhfield .v.RH = p ; 
    t = p ; 
    getxnext () ; 
  } 
  lab30: curexp = mem [ h ] .hhfield .v.RH ; 
  {
    mem [ h ] .hhfield .v.RH = avail ; 
    avail = h ; 
	;
#ifdef STAT
    decr ( dynused ) ; 
#endif /* STAT */
  } 
  curtype = 20 ; 
} 
void scansecondary ( ) 
{/* 20 22 */ halfword p  ; 
  halfword c, d  ; 
  halfword macname  ; 
  lab20: if ( ( curcmd < 30 ) || ( curcmd > 43 ) ) 
  badexp ( 801 ) ; 
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
      incr ( mem [ c ] .hhfield .lhfield ) ; 
    } 
    getxnext () ; 
    scanprimary () ; 
    if ( d != 53 ) 
    dobinary ( p , c ) ; 
    else {
	
      backinput () ; 
      binarymac ( p , c , macname ) ; 
      decr ( mem [ c ] .hhfield .lhfield ) ; 
      getxnext () ; 
      goto lab20 ; 
    } 
    goto lab22 ; 
  } 
} 
void scantertiary ( ) 
{/* 20 22 */ halfword p  ; 
  halfword c, d  ; 
  halfword macname  ; 
  lab20: if ( ( curcmd < 30 ) || ( curcmd > 43 ) ) 
  badexp ( 802 ) ; 
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
      incr ( mem [ c ] .hhfield .lhfield ) ; 
    } 
    getxnext () ; 
    scansecondary () ; 
    if ( d != 44 ) 
    dobinary ( p , c ) ; 
    else {
	
      backinput () ; 
      binarymac ( p , c , macname ) ; 
      decr ( mem [ c ] .hhfield .lhfield ) ; 
      getxnext () ; 
      goto lab20 ; 
    } 
    goto lab22 ; 
  } 
} 
void scanexpression ( ) 
{/* 20 30 22 25 26 10 */ halfword p, q, r, pp, qq  ; 
  halfword c, d  ; 
  schar myvarflag  ; 
  halfword macname  ; 
  boolean cyclehit  ; 
  scaled x, y  ; 
  schar t  ; 
  myvarflag = varflag ; 
  lab20: if ( ( curcmd < 30 ) || ( curcmd > 43 ) ) 
  badexp ( 805 ) ; 
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
      incr ( mem [ c ] .hhfield .lhfield ) ; 
    } 
    if ( ( d < 48 ) || ( ( d == 48 ) && ( ( mem [ p ] .hhfield .b0 == 14 ) || 
    ( mem [ p ] .hhfield .b0 == 9 ) ) ) ) 
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
	while ( mem [ q ] .hhfield .v.RH != p ) q = mem [ q ] .hhfield .v.RH ; 
	if ( mem [ p ] .hhfield .b0 != 0 ) 
	{
	  r = copyknot ( p ) ; 
	  mem [ q ] .hhfield .v.RH = r ; 
	  q = r ; 
	} 
	mem [ p ] .hhfield .b0 = 4 ; 
	mem [ q ] .hhfield .b1 = 4 ; 
      } 
      lab25: if ( curcmd == 46 ) 
      {
	t = scandirection () ; 
	if ( t != 4 ) 
	{
	  mem [ q ] .hhfield .b1 = t ; 
	  mem [ q + 5 ] .cint = curexp ; 
	  if ( mem [ q ] .hhfield .b0 == 4 ) 
	  {
	    mem [ q ] .hhfield .b0 = t ; 
	    mem [ q + 3 ] .cint = curexp ; 
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
	    disperr ( 0 , 823 ) ; 
	    {
	      helpptr = 1 ; 
	      helpline [ 0 ] = 824 ; 
	    } 
	    putgetflusherror ( 65536L ) ; 
	  } 
	  if ( y == 59 ) 
	  curexp = - (integer) curexp ; 
	  mem [ q + 6 ] .cint = curexp ; 
	  if ( curcmd == 52 ) 
	  {
	    getxnext () ; 
	    y = curcmd ; 
	    if ( curcmd == 59 ) 
	    getxnext () ; 
	    scanprimary () ; 
	    if ( ( curtype != 16 ) || ( curexp < 49152L ) ) 
	    {
	      disperr ( 0 , 823 ) ; 
	      {
		helpptr = 1 ; 
		helpline [ 0 ] = 824 ; 
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
	  mem [ q ] .hhfield .b1 = 1 ; 
	  t = 1 ; 
	  getxnext () ; 
	  scanprimary () ; 
	  knownpair () ; 
	  mem [ q + 5 ] .cint = curx ; 
	  mem [ q + 6 ] .cint = cury ; 
	  if ( curcmd != 52 ) 
	  {
	    x = mem [ q + 5 ] .cint ; 
	    y = mem [ q + 6 ] .cint ; 
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
	    
	  mem [ q + 6 ] .cint = 65536L ; 
	  y = 65536L ; 
	  backinput () ; 
	  goto lab30 ; 
	} 
	if ( curcmd != 47 ) 
	{
	  missingerr ( 407 ) ; 
	  {
	    helpptr = 1 ; 
	    helpline [ 0 ] = 822 ; 
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
	if ( mem [ q ] .hhfield .b1 != 1 ) 
	x = curexp ; 
	else t = 1 ; 
      } 
      else if ( mem [ q ] .hhfield .b1 != 1 ) 
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
	  mem [ q + 6 ] .cint = 65536L ; 
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
	  while ( mem [ qq ] .hhfield .v.RH != pp ) qq = mem [ qq ] .hhfield 
	  .v.RH ; 
	  if ( mem [ pp ] .hhfield .b0 != 0 ) 
	  {
	    r = copyknot ( pp ) ; 
	    mem [ qq ] .hhfield .v.RH = r ; 
	    qq = r ; 
	  } 
	  mem [ pp ] .hhfield .b0 = 4 ; 
	  mem [ qq ] .hhfield .b1 = 4 ; 
	} 
      } 
      {
	if ( d == 48 ) 
	if ( ( mem [ q + 1 ] .cint != mem [ pp + 1 ] .cint ) || ( mem [ q + 2 
	] .cint != mem [ pp + 2 ] .cint ) ) 
	{
	  {
	    if ( interaction == 3 ) 
	    ; 
	    printnl ( 261 ) ; 
	    print ( 825 ) ; 
	  } 
	  {
	    helpptr = 3 ; 
	    helpline [ 2 ] = 826 ; 
	    helpline [ 1 ] = 827 ; 
	    helpline [ 0 ] = 828 ; 
	  } 
	  putgeterror () ; 
	  d = 47 ; 
	  mem [ q + 6 ] .cint = 65536L ; 
	  y = 65536L ; 
	} 
	if ( mem [ pp ] .hhfield .b1 == 4 ) 
	if ( ( t == 3 ) || ( t == 2 ) ) 
	{
	  mem [ pp ] .hhfield .b1 = t ; 
	  mem [ pp + 5 ] .cint = x ; 
	} 
	if ( d == 48 ) 
	{
	  if ( mem [ q ] .hhfield .b0 == 4 ) 
	  if ( mem [ q ] .hhfield .b1 == 4 ) 
	  {
	    mem [ q ] .hhfield .b0 = 3 ; 
	    mem [ q + 3 ] .cint = 65536L ; 
	  } 
	  if ( mem [ pp ] .hhfield .b1 == 4 ) 
	  if ( t == 4 ) 
	  {
	    mem [ pp ] .hhfield .b1 = 3 ; 
	    mem [ pp + 5 ] .cint = 65536L ; 
	  } 
	  mem [ q ] .hhfield .b1 = mem [ pp ] .hhfield .b1 ; 
	  mem [ q ] .hhfield .v.RH = mem [ pp ] .hhfield .v.RH ; 
	  mem [ q + 5 ] .cint = mem [ pp + 5 ] .cint ; 
	  mem [ q + 6 ] .cint = mem [ pp + 6 ] .cint ; 
	  freenode ( pp , 7 ) ; 
	  if ( qq == pp ) 
	  qq = q ; 
	} 
	else {
	    
	  if ( mem [ q ] .hhfield .b1 == 4 ) 
	  if ( ( mem [ q ] .hhfield .b0 == 3 ) || ( mem [ q ] .hhfield .b0 == 
	  2 ) ) 
	  {
	    mem [ q ] .hhfield .b1 = mem [ q ] .hhfield .b0 ; 
	    mem [ q + 5 ] .cint = mem [ q + 3 ] .cint ; 
	  } 
	  mem [ q ] .hhfield .v.RH = pp ; 
	  mem [ pp + 4 ] .cint = y ; 
	  if ( t != 4 ) 
	  {
	    mem [ pp + 3 ] .cint = x ; 
	    mem [ pp ] .hhfield .b0 = t ; 
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
	  
	mem [ p ] .hhfield .b0 = 0 ; 
	if ( mem [ p ] .hhfield .b1 == 4 ) 
	{
	  mem [ p ] .hhfield .b1 = 3 ; 
	  mem [ p + 5 ] .cint = 65536L ; 
	} 
	mem [ q ] .hhfield .b1 = 0 ; 
	if ( mem [ q ] .hhfield .b0 == 4 ) 
	{
	  mem [ q ] .hhfield .b0 = 3 ; 
	  mem [ q + 3 ] .cint = 65536L ; 
	} 
	mem [ q ] .hhfield .v.RH = p ; 
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
	decr ( mem [ c ] .hhfield .lhfield ) ; 
	getxnext () ; 
	goto lab20 ; 
      } 
    } 
    goto lab22 ; 
  } 
  lab10: ; 
} 
void getboolean ( ) 
{getxnext () ; 
  scanexpression () ; 
  if ( curtype != 2 ) 
  {
    disperr ( 0 , 829 ) ; 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 830 ; 
      helpline [ 0 ] = 831 ; 
    } 
    putgetflusherror ( 31 ) ; 
    curtype = 2 ; 
  } 
} 
void printcapsule ( ) 
{printchar ( 40 ) ; 
  printexp ( gpointer , 0 ) ; 
  printchar ( 41 ) ; 
} 
void tokenrecycle ( ) 
{recyclevalue ( gpointer ) ; 
} 
void closefilesandterminate ( ) 
{integer k  ; 
  integer lh  ; 
  short lkoffset  ; 
  halfword p  ; 
  scaled x  ; 
	;
#ifdef STAT
  if ( internal [ 12 ] > 0 ) 
  if ( logopened ) 
  {
    (void) fprintf( logfile , "%c\n",  ' ' ) ; 
    (void) fprintf( logfile , "%s%s\n",  "Here is how much of METAFONT's memory" , " you used:"     ) ; 
    (void) fprintf( logfile , "%c%ld%s",  ' ' , (long)maxstrptr - initstrptr , " string" ) ; 
    if ( maxstrptr != initstrptr + 1 ) 
    (void) putc( 's' ,  logfile );
    (void) fprintf( logfile , "%s%ld\n",  " out of " , (long)maxstrings - initstrptr ) ; 
    (void) fprintf( logfile , "%c%ld%s%ld\n",  ' ' , (long)maxpoolptr - initpoolptr ,     " string characters out of " , (long)poolsize - initpoolptr ) ; 
    (void) fprintf( logfile , "%c%ld%s%ld\n",  ' ' , (long)lomemmax + 0 + memend - himemmin + 2 ,     " words of memory out of " , (long)memend + 1 ) ; 
    (void) fprintf( logfile , "%c%ld%s%ld\n",  ' ' , (long)stcount , " symbolic tokens out of " , (long)9500 ) ; 
    (void) fprintf( logfile , "%c%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%c\n",  ' ' , (long)maxinstack , "i," , (long)intptr , "n," ,     (long)maxroundingptr , "r," , (long)maxparamstack , "p," , (long)maxbufstack + 1 ,     "b stack positions out of " , (long)stacksize , "i," , (long)maxinternal , "n," ,     (long)maxwiggle , "r," , (long)150 , "p," , (long)bufsize , 'b' ) ; 
  } 
#endif /* STAT */
  if ( ( gfprevptr > 0 ) || ( internal [ 33 ] > 0 ) ) 
  {
    rover = 23 ; 
    mem [ rover ] .hhfield .v.RH = 262143L ; 
    lomemmax = himemmin - 1 ; 
    if ( lomemmax - rover > 262143L ) 
    lomemmax = 262143L + rover ; 
    mem [ rover ] .hhfield .lhfield = lomemmax - rover ; 
    mem [ rover + 1 ] .hhfield .lhfield = rover ; 
    mem [ rover + 1 ] .hhfield .v.RH = rover ; 
    mem [ lomemmax ] .hhfield .v.RH = 0 ; 
    mem [ lomemmax ] .hhfield .lhfield = 0 ; 
    mem [ memtop - 1 ] .hhfield .v.RH = 19 ; 
    {register integer for_end; k = bc ; for_end = ec ; if ( k <= for_end) do 
      if ( charexists [ k ] ) 
      tfmwidth [ k ] = sortin ( tfmwidth [ k ] ) ; 
    while ( k++ < for_end ) ; } 
    nw = skimp ( 255 ) + 1 ; 
    dimenhead [ 1 ] = mem [ memtop - 1 ] .hhfield .v.RH ; 
    if ( perturbation >= 4096 ) 
    tfmwarning ( 20 ) ; 
    fixdesignsize () ; 
    fixchecksum () ; 
    if ( internal [ 33 ] > 0 ) 
    {
      mem [ memtop - 1 ] .hhfield .v.RH = 19 ; 
      {register integer for_end; k = bc ; for_end = ec ; if ( k <= for_end) 
      do 
	if ( charexists [ k ] ) 
	if ( tfmheight [ k ] == 0 ) 
	tfmheight [ k ] = 15 ; 
	else tfmheight [ k ] = sortin ( tfmheight [ k ] ) ; 
      while ( k++ < for_end ) ; } 
      nh = skimp ( 15 ) + 1 ; 
      dimenhead [ 2 ] = mem [ memtop - 1 ] .hhfield .v.RH ; 
      if ( perturbation >= 4096 ) 
      tfmwarning ( 21 ) ; 
      mem [ memtop - 1 ] .hhfield .v.RH = 19 ; 
      {register integer for_end; k = bc ; for_end = ec ; if ( k <= for_end) 
      do 
	if ( charexists [ k ] ) 
	if ( tfmdepth [ k ] == 0 ) 
	tfmdepth [ k ] = 15 ; 
	else tfmdepth [ k ] = sortin ( tfmdepth [ k ] ) ; 
      while ( k++ < for_end ) ; } 
      nd = skimp ( 15 ) + 1 ; 
      dimenhead [ 3 ] = mem [ memtop - 1 ] .hhfield .v.RH ; 
      if ( perturbation >= 4096 ) 
      tfmwarning ( 22 ) ; 
      mem [ memtop - 1 ] .hhfield .v.RH = 19 ; 
      {register integer for_end; k = bc ; for_end = ec ; if ( k <= for_end) 
      do 
	if ( charexists [ k ] ) 
	if ( tfmitalcorr [ k ] == 0 ) 
	tfmitalcorr [ k ] = 15 ; 
	else tfmitalcorr [ k ] = sortin ( tfmitalcorr [ k ] ) ; 
      while ( k++ < for_end ) ; } 
      ni = skimp ( 63 ) + 1 ; 
      dimenhead [ 4 ] = mem [ memtop - 1 ] .hhfield .v.RH ; 
      if ( perturbation >= 4096 ) 
      tfmwarning ( 23 ) ; 
      internal [ 33 ] = 0 ; 
      if ( jobname == 0 ) 
      openlogfile () ; 
      packjobname ( 1042 ) ; 
      while ( ! bopenout ( tfmfile ) ) promptfilename ( 1043 , 1042 ) ; 
      metricfilename = bmakenamestring ( tfmfile ) ; 
      k = headersize ; 
      while ( headerbyte [ k ] < 0 ) decr ( k ) ; 
      lh = ( k + 3 ) / 4 ; 
      if ( bc > ec ) 
      bc = 1 ; 
      bchar = roundunscaled ( internal [ 41 ] ) ; 
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
      if ( labelloc [ k ] + lkoffset > 255 ) 
      {
	lkoffset = 0 ; 
	lkstarted = false ; 
	do {
	    charremainder [ labelchar [ k ] ] = lkoffset ; 
	  while ( labelloc [ k - 1 ] == labelloc [ k ] ) {
	      
	    decr ( k ) ; 
	    charremainder [ labelchar [ k ] ] = lkoffset ; 
	  } 
	  incr ( lkoffset ) ; 
	  decr ( k ) ; 
	} while ( ! ( lkoffset + labelloc [ k ] < 256 ) ) ; 
      } 
      if ( lkoffset > 0 ) 
      while ( k > 0 ) {
	  
	charremainder [ labelchar [ k ] ] = charremainder [ labelchar [ k ] ] 
	+ lkoffset ; 
	decr ( k ) ; 
      } 
      if ( bchlabel < ligtablesize ) 
      {
	ligkern [ nl ] .b0 = 255 ; 
	ligkern [ nl ] .b1 = 0 ; 
	ligkern [ nl ] .b2 = ( ( bchlabel + lkoffset ) / 256 ) ; 
	ligkern [ nl ] .b3 = ( ( bchlabel + lkoffset ) % 256 ) ; 
	incr ( nl ) ; 
      } 
      bwrite2bytes ( tfmfile , 6 + lh + ( ec - bc + 1 ) + nw + nh + nd + ni + 
      nl + lkoffset + nk + ne + np ) ; 
      bwrite2bytes ( tfmfile , lh ) ; 
      bwrite2bytes ( tfmfile , bc ) ; 
      bwrite2bytes ( tfmfile , ec ) ; 
      bwrite2bytes ( tfmfile , nw ) ; 
      bwrite2bytes ( tfmfile , nh ) ; 
      bwrite2bytes ( tfmfile , nd ) ; 
      bwrite2bytes ( tfmfile , ni ) ; 
      bwrite2bytes ( tfmfile , nl + lkoffset ) ; 
      bwrite2bytes ( tfmfile , nk ) ; 
      bwrite2bytes ( tfmfile , ne ) ; 
      bwrite2bytes ( tfmfile , np ) ; 
      {register integer for_end; k = 1 ; for_end = 4 * lh ; if ( k <= 
      for_end) do 
	{
	  if ( headerbyte [ k ] < 0 ) 
	  headerbyte [ k ] = 0 ; 
	  bwritebyte ( tfmfile , headerbyte [ k ] ) ; 
	} 
      while ( k++ < for_end ) ; } 
      {register integer for_end; k = bc ; for_end = ec ; if ( k <= for_end) 
      do 
	if ( ! charexists [ k ] ) 
	bwrite4bytes ( tfmfile , 0 ) ; 
	else {
	    
	  bwritebyte ( tfmfile , mem [ tfmwidth [ k ] ] .hhfield .lhfield ) ; 
	  bwritebyte ( tfmfile , ( mem [ tfmheight [ k ] ] .hhfield .lhfield ) 
	  * 16 + mem [ tfmdepth [ k ] ] .hhfield .lhfield ) ; 
	  bwritebyte ( tfmfile , ( mem [ tfmitalcorr [ k ] ] .hhfield .lhfield 
	  ) * 4 + chartag [ k ] ) ; 
	  bwritebyte ( tfmfile , charremainder [ k ] ) ; 
	} 
      while ( k++ < for_end ) ; } 
      tfmchanged = 0 ; 
      {register integer for_end; k = 1 ; for_end = 4 ; if ( k <= for_end) do 
	{
	  bwrite4bytes ( tfmfile , 0 ) ; 
	  p = dimenhead [ k ] ; 
	  while ( p != 19 ) {
	      
	    bwrite4bytes ( tfmfile , dimenout ( mem [ p + 1 ] .cint ) ) ; 
	    p = mem [ p ] .hhfield .v.RH ; 
	  } 
	} 
      while ( k++ < for_end ) ; } 
      {register integer for_end; k = 0 ; for_end = 255 ; if ( k <= for_end) 
      do 
	if ( skiptable [ k ] < ligtablesize ) 
	{
	  printnl ( 1045 ) ; 
	  printint ( k ) ; 
	  print ( 1046 ) ; 
	  ll = skiptable [ k ] ; 
	  do {
	      lll = ligkern [ ll ] .b0 ; 
	    ligkern [ ll ] .b0 = 128 ; 
	    ll = ll - lll ; 
	  } while ( ! ( lll == 0 ) ) ; 
	} 
      while ( k++ < for_end ) ; } 
      if ( lkstarted ) 
      {
	bwritebyte ( tfmfile , 255 ) ; 
	bwritebyte ( tfmfile , bchar ) ; 
	bwrite2bytes ( tfmfile , 0 ) ; 
      } 
      else {
	  register integer for_end; k = 1 ; for_end = lkoffset ; if ( k <= 
      for_end) do 
	{
	  ll = labelloc [ labelptr ] ; 
	  if ( bchar < 0 ) 
	  {
	    bwritebyte ( tfmfile , 254 ) ; 
	    bwritebyte ( tfmfile , 0 ) ; 
	  } 
	  else {
	      
	    bwritebyte ( tfmfile , 255 ) ; 
	    bwritebyte ( tfmfile , bchar ) ; 
	  } 
	  bwrite2bytes ( tfmfile , ll + lkoffset ) ; 
	  do {
	      decr ( labelptr ) ; 
	  } while ( ! ( labelloc [ labelptr ] < ll ) ) ; 
	} 
      while ( k++ < for_end ) ; } 
      {register integer for_end; k = 0 ; for_end = nl - 1 ; if ( k <= 
      for_end) do 
	tfmqqqq ( ligkern [ k ] ) ; 
      while ( k++ < for_end ) ; } 
      {register integer for_end; k = 0 ; for_end = nk - 1 ; if ( k <= 
      for_end) do 
	bwrite4bytes ( tfmfile , dimenout ( kern [ k ] ) ) ; 
      while ( k++ < for_end ) ; } 
      {register integer for_end; k = 0 ; for_end = ne - 1 ; if ( k <= 
      for_end) do 
	tfmqqqq ( exten [ k ] ) ; 
      while ( k++ < for_end ) ; } 
      {register integer for_end; k = 1 ; for_end = np ; if ( k <= for_end) do 
	if ( k == 1 ) 
	if ( abs ( param [ 1 ] ) < 134217728L ) 
	bwrite4bytes ( tfmfile , param [ 1 ] * 16 ) ; 
	else {
	    
	  incr ( tfmchanged ) ; 
	  if ( param [ 1 ] > 0 ) 
	  bwrite4bytes ( tfmfile , 2147483647L ) ; 
	  else bwrite4bytes ( tfmfile , -2147483647L ) ; 
	} 
	else bwrite4bytes ( tfmfile , dimenout ( param [ k ] ) ) ; 
      while ( k++ < for_end ) ; } 
      if ( tfmchanged > 0 ) 
      {
	if ( tfmchanged == 1 ) 
	printnl ( 1047 ) ; 
	else {
	    
	  printnl ( 40 ) ; 
	  printint ( tfmchanged ) ; 
	  print ( 1048 ) ; 
	} 
	print ( 1049 ) ; 
      } 
	;
#ifdef STAT
      if ( internal [ 12 ] > 0 ) 
      {
	(void) fprintf( logfile , "%c\n",  ' ' ) ; 
	if ( bchlabel < ligtablesize ) 
	decr ( nl ) ; 
	(void) fprintf( logfile , "%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s\n",  "(You used " , (long)nw , "w," , (long)nh , "h," , (long)nd , "d," ,         (long)ni , "i," , (long)nl , "l," , (long)nk , "k," , (long)ne , "e," , (long)np ,         "p metric file positions" ) ; 
	(void) fprintf( logfile , "%s%s%ld%s%ld%s%ld%s\n",  "  out of " , "256w,16h,16d,64i," , (long)ligtablesize ,         "l," , (long)maxkerns , "k,256e," , (long)maxfontdimen , "p)" ) ; 
      } 
#endif /* STAT */
      printnl ( 1044 ) ; 
      slowprint ( metricfilename ) ; 
      printchar ( 46 ) ; 
      bclose ( tfmfile ) ; 
    } 
    if ( gfprevptr > 0 ) 
    {
      {
	gfbuf [ gfptr ] = 248 ; 
	incr ( gfptr ) ; 
	if ( gfptr == gflimit ) 
	gfswap () ; 
      } 
      gffour ( gfprevptr ) ; 
      gfprevptr = gfoffset + gfptr - 5 ; 
      gffour ( internal [ 26 ] * 16 ) ; 
      {register integer for_end; k = 1 ; for_end = 4 ; if ( k <= for_end) do 
	{
	  gfbuf [ gfptr ] = headerbyte [ k ] ; 
	  incr ( gfptr ) ; 
	  if ( gfptr == gflimit ) 
	  gfswap () ; 
	} 
      while ( k++ < for_end ) ; } 
      gffour ( internal [ 27 ] ) ; 
      gffour ( internal [ 28 ] ) ; 
      gffour ( gfminm ) ; 
      gffour ( gfmaxm ) ; 
      gffour ( gfminn ) ; 
      gffour ( gfmaxn ) ; 
      {register integer for_end; k = 0 ; for_end = 255 ; if ( k <= for_end) 
      do 
	if ( charexists [ k ] ) 
	{
	  x = gfdx [ k ] / 65536L ; 
	  if ( ( gfdy [ k ] == 0 ) && ( x >= 0 ) && ( x < 256 ) && ( gfdx [ k 
	  ] == x * 65536L ) ) 
	  {
	    {
	      gfbuf [ gfptr ] = 246 ; 
	      incr ( gfptr ) ; 
	      if ( gfptr == gflimit ) 
	      gfswap () ; 
	    } 
	    {
	      gfbuf [ gfptr ] = k ; 
	      incr ( gfptr ) ; 
	      if ( gfptr == gflimit ) 
	      gfswap () ; 
	    } 
	    {
	      gfbuf [ gfptr ] = x ; 
	      incr ( gfptr ) ; 
	      if ( gfptr == gflimit ) 
	      gfswap () ; 
	    } 
	  } 
	  else {
	      
	    {
	      gfbuf [ gfptr ] = 245 ; 
	      incr ( gfptr ) ; 
	      if ( gfptr == gflimit ) 
	      gfswap () ; 
	    } 
	    {
	      gfbuf [ gfptr ] = k ; 
	      incr ( gfptr ) ; 
	      if ( gfptr == gflimit ) 
	      gfswap () ; 
	    } 
	    gffour ( gfdx [ k ] ) ; 
	    gffour ( gfdy [ k ] ) ; 
	  } 
	  x = mem [ tfmwidth [ k ] + 1 ] .cint ; 
	  if ( abs ( x ) > maxtfmdimen ) 
	  if ( x > 0 ) 
	  x = 16777215L ; 
	  else x = -16777215L ; 
	  else x = makescaled ( x * 16 , internal [ 26 ] ) ; 
	  gffour ( x ) ; 
	  gffour ( charptr [ k ] ) ; 
	} 
      while ( k++ < for_end ) ; } 
      {
	gfbuf [ gfptr ] = 249 ; 
	incr ( gfptr ) ; 
	if ( gfptr == gflimit ) 
	gfswap () ; 
      } 
      gffour ( gfprevptr ) ; 
      {
	gfbuf [ gfptr ] = 131 ; 
	incr ( gfptr ) ; 
	if ( gfptr == gflimit ) 
	gfswap () ; 
      } 
      k = 4 + ( ( gfbufsize - gfptr ) % 4 ) ; 
      while ( k > 0 ) {
	  
	{
	  gfbuf [ gfptr ] = 223 ; 
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
      printnl ( 1060 ) ; 
      slowprint ( outputfilename ) ; 
      print ( 557 ) ; 
      printint ( totalchars ) ; 
      print ( 1061 ) ; 
      if ( totalchars != 1 ) 
      printchar ( 115 ) ; 
      print ( 1062 ) ; 
      printint ( gfoffset + gfptr ) ; 
      print ( 1063 ) ; 
      bclose ( gffile ) ; 
    } 
  } 
  if ( logopened ) 
  {
    (void) putc('\n',  logfile );
    aclose ( logfile ) ; 
    selector = selector - 2 ; 
    if ( selector == 1 ) 
    {
      printnl ( 1071 ) ; 
      slowprint ( texmflogname ) ; 
      printchar ( 46 ) ; 
    } 
  } 
  println () ; 
  if ( ( editnamestart != 0 ) && ( interaction > 0 ) ) 
  calledit ( strpool , editnamestart , editnamelength , editline ) ; 
} 
#ifdef DEBUG
void debughelp ( ) 
{/* 888 10 */ integer k, l, m, n  ; 
  while ( true ) {
      
    ; 
    printnl ( 1078 ) ; 
    flush ( stdout ) ; 
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
	printword ( mem [ n ] ) ; 
	break ; 
      case 2 : 
	printint ( mem [ n ] .hhfield .lhfield ) ; 
	break ; 
      case 3 : 
	printint ( mem [ n ] .hhfield .v.RH ) ; 
	break ; 
      case 4 : 
	{
	  printint ( eqtb [ n ] .lhfield ) ; 
	  printchar ( 58 ) ; 
	  printint ( eqtb [ n ] .v.RH ) ; 
	} 
	break ; 
      case 5 : 
	printvariablename ( n ) ; 
	break ; 
      case 6 : 
	printint ( internal [ n ] ) ; 
	break ; 
      case 7 : 
	doshowdependencies () ; 
	break ; 
      case 9 : 
	showtokenlist ( n , 0 , 100000L , 0 ) ; 
	break ; 
      case 10 : 
	slowprint ( n ) ; 
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
	{register integer for_end; k = 0 ; for_end = n ; if ( k <= for_end) 
	do 
	  print ( buffer [ k ] ) ; 
	while ( k++ < for_end ) ; } 
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
#endif /* DEBUG */
