#define EXTERN extern
#include "mfd.h"

void zmakeeq ( lhs ) 
halfword lhs ; 
{/* 20 30 45 */ smallnumber t  ; 
  integer v  ; 
  halfword p, q  ; 
  lab20: t = mem [ lhs ] .hhfield .b0 ; 
  if ( t <= 14 ) 
  v = mem [ lhs + 1 ] .cint ; 
  switch ( t ) 
  {case 2 : 
  case 4 : 
  case 6 : 
  case 9 : 
  case 11 : 
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
	    printnl ( 261 ) ; 
	    print ( 597 ) ; 
	  } 
	  {
	    helpptr = 2 ; 
	    helpline [ 1 ] = 598 ; 
	    helpline [ 0 ] = 599 ; 
	  } 
	  putgeterror () ; 
	} 
	goto lab30 ; 
      } 
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 261 ) ; 
	print ( 891 ) ; 
      } 
      {
	helpptr = 2 ; 
	helpline [ 1 ] = 892 ; 
	helpline [ 0 ] = 893 ; 
      } 
      putgeterror () ; 
      goto lab30 ; 
      lab45: {
	  
	if ( interaction == 3 ) 
	; 
	printnl ( 261 ) ; 
	print ( 894 ) ; 
      } 
      {
	helpptr = 2 ; 
	helpline [ 1 ] = 895 ; 
	helpline [ 0 ] = 893 ; 
      } 
      putgeterror () ; 
      goto lab30 ; 
    } 
    break ; 
  case 3 : 
  case 5 : 
  case 7 : 
  case 12 : 
  case 10 : 
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
    if ( t == 10 ) 
    {
      pairtopath () ; 
      goto lab20 ; 
    } 
    break ; 
  case 13 : 
  case 14 : 
    if ( curtype == t ) 
    {
      p = v + bignodesize [ t ] ; 
      q = mem [ curexp + 1 ] .cint + bignodesize [ t ] ; 
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
  disperr ( lhs , 283 ) ; 
  disperr ( 0 , 888 ) ; 
  if ( mem [ lhs ] .hhfield .b0 <= 14 ) 
  printtype ( mem [ lhs ] .hhfield .b0 ) ; 
  else print ( 339 ) ; 
  printchar ( 61 ) ; 
  if ( curtype <= 14 ) 
  printtype ( curtype ) ; 
  else print ( 339 ) ; 
  printchar ( 41 ) ; 
  {
    helpptr = 2 ; 
    helpline [ 1 ] = 889 ; 
    helpline [ 0 ] = 890 ; 
  } 
  putgeterror () ; 
  lab30: {
      
    if ( aritherror ) 
    cleararith () ; 
  } 
  recyclevalue ( lhs ) ; 
  freenode ( lhs , 2 ) ; 
} 
void doequation ( ) 
{halfword lhs  ; 
  halfword p  ; 
  lhs = stashcurexp () ; 
  getxnext () ; 
  varflag = 77 ; 
  scanexpression () ; 
  if ( curcmd == 51 ) 
  doequation () ; 
  else if ( curcmd == 77 ) 
  doassignment () ; 
  if ( internal [ 7 ] > 131072L ) 
  {
    begindiagnostic () ; 
    printnl ( 847 ) ; 
    printexp ( lhs , 0 ) ; 
    print ( 883 ) ; 
    printexp ( 0 , 0 ) ; 
    print ( 839 ) ; 
    enddiagnostic ( false ) ; 
  } 
  if ( curtype == 10 ) 
  if ( mem [ lhs ] .hhfield .b0 == 14 ) 
  {
    p = stashcurexp () ; 
    unstashcurexp ( lhs ) ; 
    lhs = p ; 
  } 
  makeeq ( lhs ) ; 
} 
void doassignment ( ) 
{halfword lhs  ; 
  halfword p  ; 
  halfword q  ; 
  if ( curtype != 20 ) 
  {
    disperr ( 0 , 880 ) ; 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 881 ; 
      helpline [ 0 ] = 882 ; 
    } 
    error () ; 
    doequation () ; 
  } 
  else {
      
    lhs = curexp ; 
    curtype = 1 ; 
    getxnext () ; 
    varflag = 77 ; 
    scanexpression () ; 
    if ( curcmd == 51 ) 
    doequation () ; 
    else if ( curcmd == 77 ) 
    doassignment () ; 
    if ( internal [ 7 ] > 131072L ) 
    {
      begindiagnostic () ; 
      printnl ( 123 ) ; 
      if ( mem [ lhs ] .hhfield .lhfield > 9769 ) 
      slowprint ( intname [ mem [ lhs ] .hhfield .lhfield - ( 9769 ) ] ) ; 
      else showtokenlist ( lhs , 0 , 1000 , 0 ) ; 
      print ( 460 ) ; 
      printexp ( 0 , 0 ) ; 
      printchar ( 125 ) ; 
      enddiagnostic ( false ) ; 
    } 
    if ( mem [ lhs ] .hhfield .lhfield > 9769 ) 
    if ( curtype == 16 ) 
    internal [ mem [ lhs ] .hhfield .lhfield - ( 9769 ) ] = curexp ; 
    else {
	
      disperr ( 0 , 884 ) ; 
      slowprint ( intname [ mem [ lhs ] .hhfield .lhfield - ( 9769 ) ] ) ; 
      print ( 885 ) ; 
      {
	helpptr = 2 ; 
	helpline [ 1 ] = 886 ; 
	helpline [ 0 ] = 887 ; 
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
	mem [ p ] .hhfield .b0 = curtype ; 
	mem [ p + 1 ] .cint = 0 ; 
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
void dotypedeclaration ( ) 
{smallnumber t  ; 
  halfword p  ; 
  halfword q  ; 
  if ( curmod >= 13 ) 
  t = curmod ; 
  else t = curmod + 1 ; 
  do {
      p = scandeclaredvariable () ; 
    flushvariable ( eqtb [ mem [ p ] .hhfield .lhfield ] .v.RH , mem [ p ] 
    .hhfield .v.RH , false ) ; 
    q = findvariable ( p ) ; 
    if ( q != 0 ) 
    {
      mem [ q ] .hhfield .b0 = t ; 
      mem [ q + 1 ] .cint = 0 ; 
    } 
    else {
	
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 261 ) ; 
	print ( 897 ) ; 
      } 
      {
	helpptr = 2 ; 
	helpline [ 1 ] = 898 ; 
	helpline [ 0 ] = 899 ; 
      } 
      putgeterror () ; 
    } 
    flushlist ( p ) ; 
    if ( curcmd < 82 ) 
    {
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 261 ) ; 
	print ( 900 ) ; 
      } 
      {
	helpptr = 5 ; 
	helpline [ 4 ] = 901 ; 
	helpline [ 3 ] = 902 ; 
	helpline [ 2 ] = 903 ; 
	helpline [ 1 ] = 904 ; 
	helpline [ 0 ] = 905 ; 
      } 
      if ( curcmd == 42 ) 
      helpline [ 2 ] = 906 ; 
      putgeterror () ; 
      scannerstatus = 2 ; 
      do {
	  getnext () ; 
	if ( curcmd == 39 ) 
	{
	  if ( strref [ curmod ] < 127 ) 
	  if ( strref [ curmod ] > 1 ) 
	  decr ( strref [ curmod ] ) ; 
	  else flushstring ( curmod ) ; 
	} 
      } while ( ! ( curcmd >= 82 ) ) ; 
      scannerstatus = 0 ; 
    } 
  } while ( ! ( curcmd > 82 ) ) ; 
} 
void dorandomseed ( ) 
{getxnext () ; 
  if ( curcmd != 77 ) 
  {
    missingerr ( 460 ) ; 
    {
      helpptr = 1 ; 
      helpline [ 0 ] = 911 ; 
    } 
    backerror () ; 
  } 
  getxnext () ; 
  scanexpression () ; 
  if ( curtype != 16 ) 
  {
    disperr ( 0 , 912 ) ; 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 913 ; 
      helpline [ 0 ] = 914 ; 
    } 
    putgetflusherror ( 0 ) ; 
  } 
  else {
      
    initrandoms ( curexp ) ; 
    if ( selector >= 2 ) 
    {
      oldsetting = selector ; 
      selector = 2 ; 
      printnl ( 915 ) ; 
      printscaled ( curexp ) ; 
      printchar ( 125 ) ; 
      printnl ( 283 ) ; 
      selector = oldsetting ; 
    } 
  } 
} 
void doprotection ( ) 
{schar m  ; 
  halfword t  ; 
  m = curmod ; 
  do {
      getsymbol () ; 
    t = eqtb [ cursym ] .lhfield ; 
    if ( m == 0 ) 
    {
      if ( t >= 86 ) 
      eqtb [ cursym ] .lhfield = t - 86 ; 
    } 
    else if ( t < 86 ) 
    eqtb [ cursym ] .lhfield = t + 86 ; 
    getxnext () ; 
  } while ( ! ( curcmd != 82 ) ) ; 
} 
void defdelims ( ) 
{halfword ldelim, rdelim  ; 
  getclearsymbol () ; 
  ldelim = cursym ; 
  getclearsymbol () ; 
  rdelim = cursym ; 
  eqtb [ ldelim ] .lhfield = 31 ; 
  eqtb [ ldelim ] .v.RH = rdelim ; 
  eqtb [ rdelim ] .lhfield = 62 ; 
  eqtb [ rdelim ] .v.RH = ldelim ; 
  getxnext () ; 
} 
void dointerim ( ) 
{getxnext () ; 
  if ( curcmd != 40 ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 261 ) ; 
      print ( 921 ) ; 
    } 
    if ( cursym == 0 ) 
    print ( 926 ) ; 
    else slowprint ( hash [ cursym ] .v.RH ) ; 
    print ( 927 ) ; 
    {
      helpptr = 1 ; 
      helpline [ 0 ] = 928 ; 
    } 
    backerror () ; 
  } 
  else {
      
    saveinternal ( curmod ) ; 
    backinput () ; 
  } 
  dostatement () ; 
} 
void dolet ( ) 
{halfword l  ; 
  getsymbol () ; 
  l = cursym ; 
  getxnext () ; 
  if ( curcmd != 51 ) 
  if ( curcmd != 77 ) 
  {
    missingerr ( 61 ) ; 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 929 ; 
      helpline [ 1 ] = 670 ; 
      helpline [ 0 ] = 930 ; 
    } 
    backerror () ; 
  } 
  getsymbol () ; 
  switch ( curcmd ) 
  {case 10 : 
  case 53 : 
  case 44 : 
  case 49 : 
    incr ( mem [ curmod ] .hhfield .lhfield ) ; 
    break ; 
    default: 
    ; 
    break ; 
  } 
  clearsymbol ( l , false ) ; 
  eqtb [ l ] .lhfield = curcmd ; 
  if ( curcmd == 41 ) 
  eqtb [ l ] .v.RH = 0 ; 
  else eqtb [ l ] .v.RH = curmod ; 
  getxnext () ; 
} 
void donewinternal ( ) 
{do {
    if ( intptr == maxinternal ) 
    overflow ( 931 , maxinternal ) ; 
    getclearsymbol () ; 
    incr ( intptr ) ; 
    eqtb [ cursym ] .lhfield = 40 ; 
    eqtb [ cursym ] .v.RH = intptr ; 
    intname [ intptr ] = hash [ cursym ] .v.RH ; 
    internal [ intptr ] = 0 ; 
    getxnext () ; 
  } while ( ! ( curcmd != 82 ) ) ; 
} 
void doshow ( ) 
{do {
    getxnext () ; 
    scanexpression () ; 
    printnl ( 762 ) ; 
    printexp ( 0 , 2 ) ; 
    flushcurexp ( 0 ) ; 
  } while ( ! ( curcmd != 82 ) ) ; 
} 
void disptoken ( ) 
{printnl ( 937 ) ; 
  if ( cursym == 0 ) 
  {
    if ( curcmd == 42 ) 
    printscaled ( curmod ) ; 
    else if ( curcmd == 38 ) 
    {
      gpointer = curmod ; 
      printcapsule () ; 
    } 
    else {
	
      printchar ( 34 ) ; 
      slowprint ( curmod ) ; 
      printchar ( 34 ) ; 
      {
	if ( strref [ curmod ] < 127 ) 
	if ( strref [ curmod ] > 1 ) 
	decr ( strref [ curmod ] ) ; 
	else flushstring ( curmod ) ; 
      } 
    } 
  } 
  else {
      
    slowprint ( hash [ cursym ] .v.RH ) ; 
    printchar ( 61 ) ; 
    if ( eqtb [ cursym ] .lhfield >= 86 ) 
    print ( 938 ) ; 
    printcmdmod ( curcmd , curmod ) ; 
    if ( curcmd == 10 ) 
    {
      println () ; 
      showmacro ( curmod , 0 , 100000L ) ; 
    } 
  } 
} 
void doshowtoken ( ) 
{do {
    getnext () ; 
    disptoken () ; 
    getxnext () ; 
  } while ( ! ( curcmd != 82 ) ) ; 
} 
void doshowstats ( ) 
{printnl ( 947 ) ; 
	;
#ifdef STAT
  printint ( varused ) ; 
  printchar ( 38 ) ; 
  printint ( dynused ) ; 
  if ( false ) 
#endif /* STAT */
  print ( 356 ) ; 
  print ( 557 ) ; 
  printint ( himemmin - lomemmax - 1 ) ; 
  print ( 948 ) ; 
  println () ; 
  printnl ( 949 ) ; 
  printint ( strptr - initstrptr ) ; 
  printchar ( 38 ) ; 
  printint ( poolptr - initpoolptr ) ; 
  print ( 557 ) ; 
  printint ( maxstrings - maxstrptr ) ; 
  printchar ( 38 ) ; 
  printint ( poolsize - maxpoolptr ) ; 
  print ( 948 ) ; 
  println () ; 
  getxnext () ; 
} 
void zdispvar ( p ) 
halfword p ; 
{halfword q  ; 
  integer n  ; 
  if ( mem [ p ] .hhfield .b0 == 21 ) 
  {
    q = mem [ p + 1 ] .hhfield .lhfield ; 
    do {
	dispvar ( q ) ; 
      q = mem [ q ] .hhfield .v.RH ; 
    } while ( ! ( q == 17 ) ) ; 
    q = mem [ p + 1 ] .hhfield .v.RH ; 
    while ( mem [ q ] .hhfield .b1 == 3 ) {
	
      dispvar ( q ) ; 
      q = mem [ q ] .hhfield .v.RH ; 
    } 
  } 
  else if ( mem [ p ] .hhfield .b0 >= 22 ) 
  {
    printnl ( 283 ) ; 
    printvariablename ( p ) ; 
    if ( mem [ p ] .hhfield .b0 > 22 ) 
    print ( 662 ) ; 
    print ( 950 ) ; 
    if ( fileoffset >= maxprintline - 20 ) 
    n = 5 ; 
    else n = maxprintline - fileoffset - 15 ; 
    showmacro ( mem [ p + 1 ] .cint , 0 , n ) ; 
  } 
  else if ( mem [ p ] .hhfield .b0 != 0 ) 
  {
    printnl ( 283 ) ; 
    printvariablename ( p ) ; 
    printchar ( 61 ) ; 
    printexp ( p , 0 ) ; 
  } 
} 
void doshowvar ( ) 
{/* 30 */ do {
    getnext () ; 
    if ( cursym > 0 ) 
    if ( cursym <= 9769 ) 
    if ( curcmd == 41 ) 
    if ( curmod != 0 ) 
    {
      dispvar ( curmod ) ; 
      goto lab30 ; 
    } 
    disptoken () ; 
    lab30: getxnext () ; 
  } while ( ! ( curcmd != 82 ) ) ; 
} 
void doshowdependencies ( ) 
{halfword p  ; 
  p = mem [ 13 ] .hhfield .v.RH ; 
  while ( p != 13 ) {
      
    if ( interesting ( p ) ) 
    {
      printnl ( 283 ) ; 
      printvariablename ( p ) ; 
      if ( mem [ p ] .hhfield .b0 == 17 ) 
      printchar ( 61 ) ; 
      else print ( 765 ) ; 
      printdependency ( mem [ p + 1 ] .hhfield .v.RH , mem [ p ] .hhfield .b0 
      ) ; 
    } 
    p = mem [ p + 1 ] .hhfield .v.RH ; 
    while ( mem [ p ] .hhfield .lhfield != 0 ) p = mem [ p ] .hhfield .v.RH ; 
    p = mem [ p ] .hhfield .v.RH ; 
  } 
  getxnext () ; 
} 
void doshowwhatever ( ) 
{if ( interaction == 3 ) 
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
  if ( internal [ 32 ] > 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 261 ) ; 
      print ( 951 ) ; 
    } 
    if ( interaction < 3 ) 
    {
      helpptr = 0 ; 
      decr ( errorcount ) ; 
    } 
    else {
	
      helpptr = 1 ; 
      helpline [ 0 ] = 952 ; 
    } 
    if ( curcmd == 83 ) 
    error () ; 
    else putgeterror () ; 
  } 
} 
boolean scanwith ( ) 
{register boolean Result; smallnumber t  ; 
  boolean result  ; 
  t = curmod ; 
  curtype = 1 ; 
  getxnext () ; 
  scanexpression () ; 
  result = false ; 
  if ( curtype != t ) 
  {
    disperr ( 0 , 960 ) ; 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 961 ; 
      helpline [ 0 ] = 962 ; 
    } 
    if ( t == 6 ) 
    helpline [ 1 ] = 963 ; 
    putgetflusherror ( 0 ) ; 
  } 
  else if ( curtype == 6 ) 
  result = true ; 
  else {
      
    curexp = roundunscaled ( curexp ) ; 
    if ( ( abs ( curexp ) < 4 ) && ( curexp != 0 ) ) 
    result = true ; 
    else {
	
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 261 ) ; 
	print ( 964 ) ; 
      } 
      {
	helpptr = 1 ; 
	helpline [ 0 ] = 962 ; 
      } 
      putgetflusherror ( 0 ) ; 
    } 
  } 
  Result = result ; 
  return(Result) ; 
} 
void zfindedgesvar ( t ) 
halfword t ; 
{halfword p  ; 
  p = findvariable ( t ) ; 
  curedges = 0 ; 
  if ( p == 0 ) 
  {
    obliterated ( t ) ; 
    putgeterror () ; 
  } 
  else if ( mem [ p ] .hhfield .b0 != 11 ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 261 ) ; 
      print ( 787 ) ; 
    } 
    showtokenlist ( t , 0 , 1000 , 0 ) ; 
    print ( 965 ) ; 
    printtype ( mem [ p ] .hhfield .b0 ) ; 
    printchar ( 41 ) ; 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 966 ; 
      helpline [ 0 ] = 967 ; 
    } 
    putgeterror () ; 
  } 
  else curedges = mem [ p + 1 ] .cint ; 
  flushnodelist ( t ) ; 
} 
void doaddto ( ) 
{/* 30 45 */ halfword lhs, rhs  ; 
  integer w  ; 
  halfword p  ; 
  halfword q  ; 
  schar addtotype  ; 
  getxnext () ; 
  varflag = 68 ; 
  scanprimary () ; 
  if ( curtype != 20 ) 
  {
    disperr ( 0 , 968 ) ; 
    {
      helpptr = 4 ; 
      helpline [ 3 ] = 969 ; 
      helpline [ 2 ] = 970 ; 
      helpline [ 1 ] = 971 ; 
      helpline [ 0 ] = 967 ; 
    } 
    putgetflusherror ( 0 ) ; 
  } 
  else {
      
    lhs = curexp ; 
    addtotype = curmod ; 
    curtype = 1 ; 
    getxnext () ; 
    scanexpression () ; 
    if ( addtotype == 2 ) 
    {
      findedgesvar ( lhs ) ; 
      if ( curedges == 0 ) 
      flushcurexp ( 0 ) ; 
      else if ( curtype != 11 ) 
      {
	disperr ( 0 , 972 ) ; 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 973 ; 
	  helpline [ 0 ] = 967 ; 
	} 
	putgetflusherror ( 0 ) ; 
      } 
      else {
	  
	mergeedges ( curexp ) ; 
	flushcurexp ( 0 ) ; 
      } 
    } 
    else {
	
      if ( curtype == 14 ) 
      pairtopath () ; 
      if ( curtype != 9 ) 
      {
	disperr ( 0 , 972 ) ; 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 974 ; 
	  helpline [ 0 ] = 967 ; 
	} 
	putgetflusherror ( 0 ) ; 
	flushtokenlist ( lhs ) ; 
      } 
      else {
	  
	rhs = curexp ; 
	w = 1 ; 
	curpen = 3 ; 
	while ( curcmd == 66 ) if ( scanwith () ) 
	if ( curtype == 16 ) 
	w = curexp ; 
	else {
	    
	  if ( mem [ curpen ] .hhfield .lhfield == 0 ) 
	  tosspen ( curpen ) ; 
	  else decr ( mem [ curpen ] .hhfield .lhfield ) ; 
	  curpen = curexp ; 
	} 
	findedgesvar ( lhs ) ; 
	if ( curedges == 0 ) 
	tossknotlist ( rhs ) ; 
	else {
	    
	  lhs = 0 ; 
	  curpathtype = addtotype ; 
	  if ( mem [ rhs ] .hhfield .b0 == 0 ) 
	  if ( curpathtype == 0 ) 
	  if ( mem [ rhs ] .hhfield .v.RH == rhs ) 
	  {
	    mem [ rhs + 5 ] .cint = mem [ rhs + 1 ] .cint ; 
	    mem [ rhs + 6 ] .cint = mem [ rhs + 2 ] .cint ; 
	    mem [ rhs + 3 ] .cint = mem [ rhs + 1 ] .cint ; 
	    mem [ rhs + 4 ] .cint = mem [ rhs + 2 ] .cint ; 
	    mem [ rhs ] .hhfield .b0 = 1 ; 
	    mem [ rhs ] .hhfield .b1 = 1 ; 
	  } 
	  else {
	      
	    p = htapypoc ( rhs ) ; 
	    q = mem [ p ] .hhfield .v.RH ; 
	    mem [ pathtail + 5 ] .cint = mem [ q + 5 ] .cint ; 
	    mem [ pathtail + 6 ] .cint = mem [ q + 6 ] .cint ; 
	    mem [ pathtail ] .hhfield .b1 = mem [ q ] .hhfield .b1 ; 
	    mem [ pathtail ] .hhfield .v.RH = mem [ q ] .hhfield .v.RH ; 
	    freenode ( q , 7 ) ; 
	    mem [ p + 5 ] .cint = mem [ rhs + 5 ] .cint ; 
	    mem [ p + 6 ] .cint = mem [ rhs + 6 ] .cint ; 
	    mem [ p ] .hhfield .b1 = mem [ rhs ] .hhfield .b1 ; 
	    mem [ p ] .hhfield .v.RH = mem [ rhs ] .hhfield .v.RH ; 
	    freenode ( rhs , 7 ) ; 
	    rhs = p ; 
	  } 
	  else {
	      
	    {
	      if ( interaction == 3 ) 
	      ; 
	      printnl ( 261 ) ; 
	      print ( 975 ) ; 
	    } 
	    {
	      helpptr = 2 ; 
	      helpline [ 1 ] = 976 ; 
	      helpline [ 0 ] = 967 ; 
	    } 
	    putgeterror () ; 
	    tossknotlist ( rhs ) ; 
	    goto lab45 ; 
	  } 
	  else if ( curpathtype == 0 ) 
	  lhs = htapypoc ( rhs ) ; 
	  curwt = w ; 
	  rhs = makespec ( rhs , mem [ curpen + 9 ] .cint , internal [ 5 ] ) ; 
	  if ( turningnumber <= 0 ) 
	  if ( curpathtype != 0 ) 
	  if ( internal [ 39 ] > 0 ) 
	  if ( ( turningnumber < 0 ) && ( mem [ curpen ] .hhfield .v.RH == 0 ) 
	  ) 
	  curwt = - (integer) curwt ; 
	  else {
	      
	    if ( turningnumber == 0 ) 
	    if ( ( internal [ 39 ] <= 65536L ) && ( mem [ curpen ] .hhfield 
	    .v.RH == 0 ) ) 
	    goto lab30 ; 
	    else printstrange ( 977 ) ; 
	    else printstrange ( 978 ) ; 
	    {
	      helpptr = 3 ; 
	      helpline [ 2 ] = 979 ; 
	      helpline [ 1 ] = 980 ; 
	      helpline [ 0 ] = 981 ; 
	    } 
	    putgeterror () ; 
	  } 
	  lab30: ; 
	  if ( mem [ curpen + 9 ] .cint == 0 ) 
	  fillspec ( rhs ) ; 
	  else fillenvelope ( rhs ) ; 
	  if ( lhs != 0 ) 
	  {
	    revturns = true ; 
	    lhs = makespec ( lhs , mem [ curpen + 9 ] .cint , internal [ 5 ] ) 
	    ; 
	    revturns = false ; 
	    if ( mem [ curpen + 9 ] .cint == 0 ) 
	    fillspec ( lhs ) ; 
	    else fillenvelope ( lhs ) ; 
	  } 
	  lab45: ; 
	} 
	if ( mem [ curpen ] .hhfield .lhfield == 0 ) 
	tosspen ( curpen ) ; 
	else decr ( mem [ curpen ] .hhfield .lhfield ) ; 
      } 
    } 
  } 
} 
scaled ztfmcheck ( m ) 
smallnumber m ; 
{register scaled Result; if ( abs ( internal [ m ] ) >= 134217728L ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 261 ) ; 
      print ( 998 ) ; 
    } 
    print ( intname [ m ] ) ; 
    print ( 999 ) ; 
    {
      helpptr = 1 ; 
      helpline [ 0 ] = 1000 ; 
    } 
    putgeterror () ; 
    if ( internal [ m ] > 0 ) 
    Result = 134217727L ; 
    else Result = -134217727L ; 
  } 
  else Result = internal [ m ] ; 
  return(Result) ; 
} 
void doshipout ( ) 
{/* 10 */ integer c  ; 
  getxnext () ; 
  varflag = 83 ; 
  scanexpression () ; 
  if ( curtype != 20 ) 
  if ( curtype == 11 ) 
  curedges = curexp ; 
  else {
      
    {
      disperr ( 0 , 968 ) ; 
      {
	helpptr = 4 ; 
	helpline [ 3 ] = 969 ; 
	helpline [ 2 ] = 970 ; 
	helpline [ 1 ] = 971 ; 
	helpline [ 0 ] = 967 ; 
      } 
      putgetflusherror ( 0 ) ; 
    } 
    goto lab10 ; 
  } 
  else {
      
    findedgesvar ( curexp ) ; 
    curtype = 1 ; 
  } 
  if ( curedges != 0 ) 
  {
    c = roundunscaled ( internal [ 18 ] ) % 256 ; 
    if ( c < 0 ) 
    c = c + 256 ; 
    if ( c < bc ) 
    bc = c ; 
    if ( c > ec ) 
    ec = c ; 
    charexists [ c ] = true ; 
    gfdx [ c ] = internal [ 24 ] ; 
    gfdy [ c ] = internal [ 25 ] ; 
    tfmwidth [ c ] = tfmcheck ( 20 ) ; 
    tfmheight [ c ] = tfmcheck ( 21 ) ; 
    tfmdepth [ c ] = tfmcheck ( 22 ) ; 
    tfmitalcorr [ c ] = tfmcheck ( 23 ) ; 
    if ( internal [ 34 ] >= 0 ) 
    shipout ( c ) ; 
  } 
  flushcurexp ( 0 ) ; 
  lab10: ; 
} 
void dodisplay ( ) 
{/* 45 50 10 */ halfword e  ; 
  getxnext () ; 
  varflag = 73 ; 
  scanprimary () ; 
  if ( curtype != 20 ) 
  {
    disperr ( 0 , 968 ) ; 
    {
      helpptr = 4 ; 
      helpline [ 3 ] = 969 ; 
      helpline [ 2 ] = 970 ; 
      helpline [ 1 ] = 971 ; 
      helpline [ 0 ] = 967 ; 
    } 
    putgetflusherror ( 0 ) ; 
  } 
  else {
      
    e = curexp ; 
    curtype = 1 ; 
    getxnext () ; 
    scanexpression () ; 
    if ( curtype != 16 ) 
    goto lab50 ; 
    curexp = roundunscaled ( curexp ) ; 
    if ( curexp < 0 ) 
    goto lab45 ; 
    if ( curexp > 15 ) 
    goto lab45 ; 
    if ( ! windowopen [ curexp ] ) 
    goto lab45 ; 
    findedgesvar ( e ) ; 
    if ( curedges != 0 ) 
    dispedges ( curexp ) ; 
    goto lab10 ; 
    lab45: curexp = curexp * 65536L ; 
    lab50: disperr ( 0 , 982 ) ; 
    {
      helpptr = 1 ; 
      helpline [ 0 ] = 983 ; 
    } 
    putgetflusherror ( 0 ) ; 
    flushtokenlist ( e ) ; 
  } 
  lab10: ; 
} 
boolean zgetpair ( c ) 
commandcode c ; 
{register boolean Result; halfword p  ; 
  boolean b  ; 
  if ( curcmd != c ) 
  Result = false ; 
  else {
      
    getxnext () ; 
    scanexpression () ; 
    if ( nicepair ( curexp , curtype ) ) 
    {
      p = mem [ curexp + 1 ] .cint ; 
      curx = mem [ p + 1 ] .cint ; 
      cury = mem [ p + 3 ] .cint ; 
      b = true ; 
    } 
    else b = false ; 
    flushcurexp ( 0 ) ; 
    Result = b ; 
  } 
  return(Result) ; 
} 
void doopenwindow ( ) 
{/* 45 10 */ integer k  ; 
  scaled r0, c0, r1, c1  ; 
  getxnext () ; 
  scanexpression () ; 
  if ( curtype != 16 ) 
  goto lab45 ; 
  k = roundunscaled ( curexp ) ; 
  if ( k < 0 ) 
  goto lab45 ; 
  if ( k > 15 ) 
  goto lab45 ; 
  if ( ! getpair ( 70 ) ) 
  goto lab45 ; 
  r0 = curx ; 
  c0 = cury ; 
  if ( ! getpair ( 71 ) ) 
  goto lab45 ; 
  r1 = curx ; 
  c1 = cury ; 
  if ( ! getpair ( 72 ) ) 
  goto lab45 ; 
  openawindow ( k , r0 , c0 , r1 , c1 , curx , cury ) ; 
  goto lab10 ; 
  lab45: {
      
    if ( interaction == 3 ) 
    ; 
    printnl ( 261 ) ; 
    print ( 984 ) ; 
  } 
  {
    helpptr = 2 ; 
    helpline [ 1 ] = 985 ; 
    helpline [ 0 ] = 986 ; 
  } 
  putgeterror () ; 
  lab10: ; 
} 
void docull ( ) 
{/* 45 10 */ halfword e  ; 
  schar keeping  ; 
  integer w, win, wout  ; 
  w = 1 ; 
  getxnext () ; 
  varflag = 67 ; 
  scanprimary () ; 
  if ( curtype != 20 ) 
  {
    disperr ( 0 , 968 ) ; 
    {
      helpptr = 4 ; 
      helpline [ 3 ] = 969 ; 
      helpline [ 2 ] = 970 ; 
      helpline [ 1 ] = 971 ; 
      helpline [ 0 ] = 967 ; 
    } 
    putgetflusherror ( 0 ) ; 
  } 
  else {
      
    e = curexp ; 
    curtype = 1 ; 
    keeping = curmod ; 
    if ( ! getpair ( 67 ) ) 
    goto lab45 ; 
    while ( ( curcmd == 66 ) && ( curmod == 16 ) ) if ( scanwith () ) 
    w = curexp ; 
    if ( curx > cury ) 
    goto lab45 ; 
    if ( keeping == 0 ) 
    {
      if ( ( curx > 0 ) || ( cury < 0 ) ) 
      goto lab45 ; 
      wout = w ; 
      win = 0 ; 
    } 
    else {
	
      if ( ( curx <= 0 ) && ( cury >= 0 ) ) 
      goto lab45 ; 
      wout = 0 ; 
      win = w ; 
    } 
    findedgesvar ( e ) ; 
    if ( curedges != 0 ) 
    culledges ( floorunscaled ( curx + 65535L ) , floorunscaled ( cury ) , 
    wout , win ) ; 
    goto lab10 ; 
    lab45: {
	
      if ( interaction == 3 ) 
      ; 
      printnl ( 261 ) ; 
      print ( 987 ) ; 
    } 
    {
      helpptr = 1 ; 
      helpline [ 0 ] = 988 ; 
    } 
    putgeterror () ; 
    flushtokenlist ( e ) ; 
  } 
  lab10: ; 
} 
void domessage ( ) 
{schar m  ; 
  m = curmod ; 
  getxnext () ; 
  scanexpression () ; 
  if ( curtype != 4 ) 
  {
    disperr ( 0 , 697 ) ; 
    {
      helpptr = 1 ; 
      helpline [ 0 ] = 992 ; 
    } 
    putgeterror () ; 
  } 
  else switch ( m ) 
  {case 0 : 
    {
      printnl ( 283 ) ; 
      slowprint ( curexp ) ; 
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
      slowprint ( curexp ) ; 
      if ( errhelp != 0 ) 
      useerrhelp = true ; 
      else if ( longhelpseen ) 
      {
	helpptr = 1 ; 
	helpline [ 0 ] = 993 ; 
      } 
      else {
	  
	if ( interaction < 3 ) 
	longhelpseen = true ; 
	{
	  helpptr = 4 ; 
	  helpline [ 3 ] = 994 ; 
	  helpline [ 2 ] = 995 ; 
	  helpline [ 1 ] = 996 ; 
	  helpline [ 0 ] = 997 ; 
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
	if ( strref [ errhelp ] < 127 ) 
	if ( strref [ errhelp ] > 1 ) 
	decr ( strref [ errhelp ] ) ; 
	else flushstring ( errhelp ) ; 
      } 
      if ( ( strstart [ curexp + 1 ] - strstart [ curexp ] ) == 0 ) 
      errhelp = 0 ; 
      else {
	  
	errhelp = curexp ; 
	{
	  if ( strref [ errhelp ] < 127 ) 
	  incr ( strref [ errhelp ] ) ; 
	} 
      } 
    } 
    break ; 
  } 
  flushcurexp ( 0 ) ; 
} 
eightbits getcode ( ) 
{/* 40 */ register eightbits Result; integer c  ; 
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
  if ( ( strstart [ curexp + 1 ] - strstart [ curexp ] ) == 1 ) 
  {
    c = strpool [ strstart [ curexp ] ] ; 
    goto lab40 ; 
  } 
  disperr ( 0 , 1006 ) ; 
  {
    helpptr = 2 ; 
    helpline [ 1 ] = 1007 ; 
    helpline [ 0 ] = 1008 ; 
  } 
  putgetflusherror ( 0 ) ; 
  c = 0 ; 
  lab40: Result = c ; 
  return(Result) ; 
} 
void zsettag ( c , t , r ) 
halfword c ; 
smallnumber t ; 
halfword r ; 
{if ( chartag [ c ] == 0 ) 
  {
    chartag [ c ] = t ; 
    charremainder [ c ] = r ; 
    if ( t == 1 ) 
    {
      incr ( labelptr ) ; 
      labelloc [ labelptr ] = r ; 
      labelchar [ labelptr ] = c ; 
    } 
  } 
  else {
      
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 261 ) ; 
      print ( 1009 ) ; 
    } 
    if ( ( c > 32 ) && ( c < 127 ) ) 
    print ( c ) ; 
    else if ( c == 256 ) 
    print ( 1010 ) ; 
    else {
	
      print ( 1011 ) ; 
      printint ( c ) ; 
    } 
    print ( 1012 ) ; 
    switch ( chartag [ c ] ) 
    {case 1 : 
      print ( 1013 ) ; 
      break ; 
    case 2 : 
      print ( 1014 ) ; 
      break ; 
    case 3 : 
      print ( 1003 ) ; 
      break ; 
    } 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 1015 ; 
      helpline [ 0 ] = 967 ; 
    } 
    putgeterror () ; 
  } 
} 
void dotfmcommand ( ) 
{/* 22 30 */ short c, cc  ; 
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
	if ( nl - skiptable [ c ] > 128 ) 
	{
	  {
	    {
	      if ( interaction == 3 ) 
	      ; 
	      printnl ( 261 ) ; 
	      print ( 1032 ) ; 
	    } 
	    {
	      helpptr = 1 ; 
	      helpline [ 0 ] = 1033 ; 
	    } 
	    error () ; 
	    ll = skiptable [ c ] ; 
	    do {
		lll = ligkern [ ll ] .b0 ; 
	      ligkern [ ll ] .b0 = 128 ; 
	      ll = ll - lll ; 
	    } while ( ! ( lll == 0 ) ) ; 
	  } 
	  skiptable [ c ] = ligtablesize ; 
	} 
	if ( skiptable [ c ] == ligtablesize ) 
	ligkern [ nl - 1 ] .b0 = 0 ; 
	else ligkern [ nl - 1 ] .b0 = nl - skiptable [ c ] - 1 ; 
	skiptable [ c ] = nl - 1 ; 
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
	else if ( skiptable [ c ] < ligtablesize ) 
	{
	  ll = skiptable [ c ] ; 
	  skiptable [ c ] = ligtablesize ; 
	  do {
	      lll = ligkern [ ll ] .b0 ; 
	    if ( nl - ll > 128 ) 
	    {
	      {
		{
		  if ( interaction == 3 ) 
		  ; 
		  printnl ( 261 ) ; 
		  print ( 1032 ) ; 
		} 
		{
		  helpptr = 1 ; 
		  helpline [ 0 ] = 1033 ; 
		} 
		error () ; 
		ll = ll ; 
		do {
		    lll = ligkern [ ll ] .b0 ; 
		  ligkern [ ll ] .b0 = 128 ; 
		  ll = ll - lll ; 
		} while ( ! ( lll == 0 ) ) ; 
	      } 
	      goto lab22 ; 
	    } 
	    ligkern [ ll ] .b0 = nl - ll - 1 ; 
	    ll = ll - lll ; 
	  } while ( ! ( lll == 0 ) ) ; 
	} 
	goto lab22 ; 
      } 
      if ( curcmd == 76 ) 
      {
	ligkern [ nl ] .b1 = c ; 
	ligkern [ nl ] .b0 = 0 ; 
	if ( curmod < 128 ) 
	{
	  ligkern [ nl ] .b2 = curmod ; 
	  ligkern [ nl ] .b3 = getcode () ; 
	} 
	else {
	    
	  getxnext () ; 
	  scanexpression () ; 
	  if ( curtype != 16 ) 
	  {
	    disperr ( 0 , 1034 ) ; 
	    {
	      helpptr = 2 ; 
	      helpline [ 1 ] = 1035 ; 
	      helpline [ 0 ] = 307 ; 
	    } 
	    putgetflusherror ( 0 ) ; 
	  } 
	  kern [ nk ] = curexp ; 
	  k = 0 ; 
	  while ( kern [ k ] != curexp ) incr ( k ) ; 
	  if ( k == nk ) 
	  {
	    if ( nk == maxkerns ) 
	    overflow ( 1031 , maxkerns ) ; 
	    incr ( nk ) ; 
	  } 
	  ligkern [ nl ] .b2 = 128 + ( k / 256 ) ; 
	  ligkern [ nl ] .b3 = ( k % 256 ) ; 
	} 
	lkstarted = true ; 
      } 
      else {
	  
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 261 ) ; 
	  print ( 1020 ) ; 
	} 
	{
	  helpptr = 1 ; 
	  helpline [ 0 ] = 1021 ; 
	} 
	backerror () ; 
	ligkern [ nl ] .b1 = 0 ; 
	ligkern [ nl ] .b2 = 0 ; 
	ligkern [ nl ] .b3 = 0 ; 
	ligkern [ nl ] .b0 = 129 ; 
      } 
      if ( nl == ligtablesize ) 
      overflow ( 1022 , ligtablesize ) ; 
      incr ( nl ) ; 
      if ( curcmd == 82 ) 
      goto lab22 ; 
      if ( ligkern [ nl - 1 ] .b0 < 128 ) 
      ligkern [ nl - 1 ] .b0 = 128 ; 
      lab30: ; 
    } 
    break ; 
  case 2 : 
    {
      if ( ne == 256 ) 
      overflow ( 1003 , 256 ) ; 
      c = getcode () ; 
      settag ( c , 3 , ne ) ; 
      if ( curcmd != 81 ) 
      {
	missingerr ( 58 ) ; 
	{
	  helpptr = 1 ; 
	  helpline [ 0 ] = 1036 ; 
	} 
	backerror () ; 
      } 
      exten [ ne ] .b0 = getcode () ; 
      if ( curcmd != 82 ) 
      {
	missingerr ( 44 ) ; 
	{
	  helpptr = 1 ; 
	  helpline [ 0 ] = 1036 ; 
	} 
	backerror () ; 
      } 
      exten [ ne ] .b1 = getcode () ; 
      if ( curcmd != 82 ) 
      {
	missingerr ( 44 ) ; 
	{
	  helpptr = 1 ; 
	  helpline [ 0 ] = 1036 ; 
	} 
	backerror () ; 
      } 
      exten [ ne ] .b2 = getcode () ; 
      if ( curcmd != 82 ) 
      {
	missingerr ( 44 ) ; 
	{
	  helpptr = 1 ; 
	  helpline [ 0 ] = 1036 ; 
	} 
	backerror () ; 
      } 
      exten [ ne ] .b3 = getcode () ; 
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
	disperr ( 0 , 1016 ) ; 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 1017 ; 
	  helpline [ 0 ] = 1018 ; 
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
	    helpline [ 0 ] = 1019 ; 
	  } 
	  backerror () ; 
	} 
	if ( c == 3 ) 
	do {
	    if ( j > headersize ) 
	  overflow ( 1004 , headersize ) ; 
	  headerbyte [ j ] = getcode () ; 
	  incr ( j ) ; 
	} while ( ! ( curcmd != 82 ) ) ; 
	else do {
	    if ( j > maxfontdimen ) 
	  overflow ( 1005 , maxfontdimen ) ; 
	  while ( j > np ) {
	      
	    incr ( np ) ; 
	    param [ np ] = 0 ; 
	  } 
	  getxnext () ; 
	  scanexpression () ; 
	  if ( curtype != 16 ) 
	  {
	    disperr ( 0 , 1037 ) ; 
	    {
	      helpptr = 1 ; 
	      helpline [ 0 ] = 307 ; 
	    } 
	    putgetflusherror ( 0 ) ; 
	  } 
	  param [ j ] = curexp ; 
	  incr ( j ) ; 
	} while ( ! ( curcmd != 82 ) ) ; 
      } 
    } 
    break ; 
  } 
} 
void dospecial ( ) 
{smallnumber m  ; 
  m = curmod ; 
  getxnext () ; 
  scanexpression () ; 
  if ( internal [ 34 ] >= 0 ) 
  if ( curtype != m ) 
  {
    disperr ( 0 , 1057 ) ; 
    {
      helpptr = 1 ; 
      helpline [ 0 ] = 1058 ; 
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
	gfbuf [ gfptr ] = 243 ; 
	incr ( gfptr ) ; 
	if ( gfptr == gflimit ) 
	gfswap () ; 
      } 
      gffour ( curexp ) ; 
    } 
  } 
  flushcurexp ( 0 ) ; 
} 
void dostatement ( ) 
{curtype = 1 ; 
  getxnext () ; 
  if ( curcmd > 43 ) 
  {
    if ( curcmd < 83 ) 
    {
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 261 ) ; 
	print ( 866 ) ; 
      } 
      printcmdmod ( curcmd , curmod ) ; 
      printchar ( 39 ) ; 
      {
	helpptr = 5 ; 
	helpline [ 4 ] = 867 ; 
	helpline [ 3 ] = 868 ; 
	helpline [ 2 ] = 869 ; 
	helpline [ 1 ] = 870 ; 
	helpline [ 0 ] = 871 ; 
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
	if ( internal [ 1 ] > 0 ) 
	{
	  printnl ( 283 ) ; 
	  slowprint ( curexp ) ; 
	  flush ( stdout ) ; 
	} 
	if ( internal [ 34 ] > 0 ) 
	{
	  if ( outputfilename == 0 ) 
	  initgf () ; 
	  gfstring ( 1059 , curexp ) ; 
	} 
      } 
      else if ( curtype != 1 ) 
      {
	disperr ( 0 , 876 ) ; 
	{
	  helpptr = 3 ; 
	  helpline [ 2 ] = 877 ; 
	  helpline [ 1 ] = 878 ; 
	  helpline [ 0 ] = 879 ; 
	} 
	putgeterror () ; 
      } 
      flushcurexp ( 0 ) ; 
      curtype = 1 ; 
    } 
  } 
  else {
      
    if ( internal [ 7 ] > 0 ) 
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
      print ( 872 ) ; 
    } 
    {
      helpptr = 6 ; 
      helpline [ 5 ] = 873 ; 
      helpline [ 4 ] = 874 ; 
      helpline [ 3 ] = 875 ; 
      helpline [ 2 ] = 869 ; 
      helpline [ 1 ] = 870 ; 
      helpline [ 0 ] = 871 ; 
    } 
    backerror () ; 
    scannerstatus = 2 ; 
    do {
	getnext () ; 
      if ( curcmd == 39 ) 
      {
	if ( strref [ curmod ] < 127 ) 
	if ( strref [ curmod ] > 1 ) 
	decr ( strref [ curmod ] ) ; 
	else flushstring ( curmod ) ; 
      } 
    } while ( ! ( curcmd > 82 ) ) ; 
    scannerstatus = 0 ; 
  } 
  errorcount = 0 ; 
} 
void maincontrol ( ) 
{do {
    dostatement () ; 
    if ( curcmd == 84 ) 
    {
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 261 ) ; 
	print ( 907 ) ; 
      } 
      {
	helpptr = 2 ; 
	helpline [ 1 ] = 908 ; 
	helpline [ 0 ] = 687 ; 
      } 
      flusherror ( 0 ) ; 
    } 
  } while ( ! ( curcmd == 85 ) ) ; 
} 
halfword zsortin ( v ) 
scaled v ; 
{/* 40 */ register halfword Result; halfword p, q, r  ; 
  p = memtop - 1 ; 
  while ( true ) {
      
    q = mem [ p ] .hhfield .v.RH ; 
    if ( v <= mem [ q + 1 ] .cint ) 
    goto lab40 ; 
    p = q ; 
  } 
  lab40: if ( v < mem [ q + 1 ] .cint ) 
  {
    r = getnode ( 2 ) ; 
    mem [ r + 1 ] .cint = v ; 
    mem [ r ] .hhfield .v.RH = q ; 
    mem [ p ] .hhfield .v.RH = r ; 
  } 
  Result = mem [ p ] .hhfield .v.RH ; 
  return(Result) ; 
} 
integer zmincover ( d ) 
scaled d ; 
{register integer Result; halfword p  ; 
  scaled l  ; 
  integer m  ; 
  m = 0 ; 
  p = mem [ memtop - 1 ] .hhfield .v.RH ; 
  perturbation = 2147483647L ; 
  while ( p != 19 ) {
      
    incr ( m ) ; 
    l = mem [ p + 1 ] .cint ; 
    do {
	p = mem [ p ] .hhfield .v.RH ; 
    } while ( ! ( mem [ p + 1 ] .cint > l + d ) ) ; 
    if ( mem [ p + 1 ] .cint - l < perturbation ) 
    perturbation = mem [ p + 1 ] .cint - l ; 
  } 
  Result = m ; 
  return(Result) ; 
} 
scaled zthresholdfn ( m ) 
integer m ; 
{register scaled Result; scaled d  ; 
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
  return(Result) ; 
} 
integer zskimp ( m ) 
integer m ; 
{register integer Result; scaled d  ; 
  halfword p, q, r  ; 
  scaled l  ; 
  scaled v  ; 
  d = thresholdfn ( m ) ; 
  perturbation = 0 ; 
  q = memtop - 1 ; 
  m = 0 ; 
  p = mem [ memtop - 1 ] .hhfield .v.RH ; 
  while ( p != 19 ) {
      
    incr ( m ) ; 
    l = mem [ p + 1 ] .cint ; 
    mem [ p ] .hhfield .lhfield = m ; 
    if ( mem [ mem [ p ] .hhfield .v.RH + 1 ] .cint <= l + d ) 
    {
      do {
	  p = mem [ p ] .hhfield .v.RH ; 
	mem [ p ] .hhfield .lhfield = m ; 
	decr ( excess ) ; 
	if ( excess == 0 ) 
	d = 0 ; 
      } while ( ! ( mem [ mem [ p ] .hhfield .v.RH + 1 ] .cint > l + d ) ) ; 
      v = l + ( mem [ p + 1 ] .cint - l ) / 2 ; 
      if ( mem [ p + 1 ] .cint - v > perturbation ) 
      perturbation = mem [ p + 1 ] .cint - v ; 
      r = q ; 
      do {
	  r = mem [ r ] .hhfield .v.RH ; 
	mem [ r + 1 ] .cint = v ; 
      } while ( ! ( r == p ) ) ; 
      mem [ q ] .hhfield .v.RH = p ; 
    } 
    q = p ; 
    p = mem [ p ] .hhfield .v.RH ; 
  } 
  Result = m ; 
  return(Result) ; 
} 
void ztfmwarning ( m ) 
smallnumber m ; 
{printnl ( 1038 ) ; 
  print ( intname [ m ] ) ; 
  print ( 1039 ) ; 
  printscaled ( perturbation ) ; 
  print ( 1040 ) ; 
} 
void fixdesignsize ( ) 
{scaled d  ; 
  d = internal [ 26 ] ; 
  if ( ( d < 65536L ) || ( d >= 134217728L ) ) 
  {
    if ( d != 0 ) 
    printnl ( 1041 ) ; 
    d = 8388608L ; 
    internal [ 26 ] = d ; 
  } 
  if ( headerbyte [ 5 ] < 0 ) 
  if ( headerbyte [ 6 ] < 0 ) 
  if ( headerbyte [ 7 ] < 0 ) 
  if ( headerbyte [ 8 ] < 0 ) 
  {
    headerbyte [ 5 ] = d / 1048576L ; 
    headerbyte [ 6 ] = ( d / 4096 ) % 256 ; 
    headerbyte [ 7 ] = ( d / 16 ) % 256 ; 
    headerbyte [ 8 ] = ( d % 16 ) * 16 ; 
  } 
  maxtfmdimen = 16 * internal [ 26 ] - internal [ 26 ] / 2097152L ; 
  if ( maxtfmdimen >= 134217728L ) 
  maxtfmdimen = 134217727L ; 
} 
integer zdimenout ( x ) 
scaled x ; 
{register integer Result; if ( abs ( x ) > maxtfmdimen ) 
  {
    incr ( tfmchanged ) ; 
    if ( x > 0 ) 
    x = 16777215L ; 
    else x = -16777215L ; 
  } 
  else x = makescaled ( x * 16 , internal [ 26 ] ) ; 
  Result = x ; 
  return(Result) ; 
} 
void fixchecksum ( ) 
{/* 10 */ eightbits k  ; 
  eightbits lb1, lb2, lb3, b4  ; 
  integer x  ; 
  if ( headerbyte [ 1 ] < 0 ) 
  if ( headerbyte [ 2 ] < 0 ) 
  if ( headerbyte [ 3 ] < 0 ) 
  if ( headerbyte [ 4 ] < 0 ) 
  {
    lb1 = bc ; 
    lb2 = ec ; 
    lb3 = bc ; 
    b4 = ec ; 
    tfmchanged = 0 ; 
    {register integer for_end; k = bc ; for_end = ec ; if ( k <= for_end) do 
      if ( charexists [ k ] ) 
      {
	x = dimenout ( mem [ tfmwidth [ k ] + 1 ] .cint ) + ( k + 4 ) * 
	4194304L ; 
	lb1 = ( lb1 + lb1 + x ) % 255 ; 
	lb2 = ( lb2 + lb2 + x ) % 253 ; 
	lb3 = ( lb3 + lb3 + x ) % 251 ; 
	b4 = ( b4 + b4 + x ) % 247 ; 
      } 
    while ( k++ < for_end ) ; } 
    headerbyte [ 1 ] = lb1 ; 
    headerbyte [ 2 ] = lb2 ; 
    headerbyte [ 3 ] = lb3 ; 
    headerbyte [ 4 ] = b4 ; 
    goto lab10 ; 
  } 
  {register integer for_end; k = 1 ; for_end = 4 ; if ( k <= for_end) do 
    if ( headerbyte [ k ] < 0 ) 
    headerbyte [ k ] = 0 ; 
  while ( k++ < for_end ) ; } 
  lab10: ; 
} 
