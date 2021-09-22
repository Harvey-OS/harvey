#define EXTERN extern
#include "texd.h"

void 
#ifdef HAVE_PROTOTYPES
zcharwarning ( internalfontnumber f , eightbits c ) 
#else
zcharwarning ( f , c ) 
  internalfontnumber f ;
  eightbits c ;
#endif
{
  charwarning_regmem 
  if ( eqtb [15198 ].cint > 0 ) 
  {
    begindiagnostic () ;
    printnl ( 820 ) ;
    print ( c ) ;
    print ( 821 ) ;
    print ( fontname [f ]) ;
    printchar ( 33 ) ;
    enddiagnostic ( false ) ;
  } 
} 
halfword 
#ifdef HAVE_PROTOTYPES
znewcharacter ( internalfontnumber f , eightbits c ) 
#else
znewcharacter ( f , c ) 
  internalfontnumber f ;
  eightbits c ;
#endif
{
  /* 10 */ register halfword Result; newcharacter_regmem 
  halfword p  ;
  quarterword ec  ;
  ec = effectivechar ( false , f , c ) ;
  if ( fontbc [f ]<= ec ) 
  if ( fontec [f ]>= ec ) 
  if ( ( fontinfo [charbase [f ]+ ec ].qqqq .b0 > 0 ) ) 
  {
    p = getavail () ;
    mem [p ].hh.b0 = f ;
    mem [p ].hh.b1 = c ;
    Result = p ;
    return Result ;
  } 
  charwarning ( f , c ) ;
  Result = 0 ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
dviswap ( void ) 
#else
dviswap ( ) 
#endif
{
  dviswap_regmem 
  if ( dvilimit == dvibufsize ) 
  {
    writedvi ( 0 , halfbuf - 1 ) ;
    dvilimit = halfbuf ;
    dvioffset = dvioffset + dvibufsize ;
    dviptr = 0 ;
  } 
  else {
      
    writedvi ( halfbuf , dvibufsize - 1 ) ;
    dvilimit = dvibufsize ;
  } 
  dvigone = dvigone + halfbuf ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdvifour ( integer x ) 
#else
zdvifour ( x ) 
  integer x ;
#endif
{
  dvifour_regmem 
  if ( x >= 0 ) 
  {
    dvibuf [dviptr ]= x / 16777216L ;
    incr ( dviptr ) ;
    if ( dviptr == dvilimit ) 
    dviswap () ;
  } 
  else {
      
    x = x + 1073741824L ;
    x = x + 1073741824L ;
    {
      dvibuf [dviptr ]= ( x / 16777216L ) + 128 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
  } 
  x = x % 16777216L ;
  {
    dvibuf [dviptr ]= x / 65536L ;
    incr ( dviptr ) ;
    if ( dviptr == dvilimit ) 
    dviswap () ;
  } 
  x = x % 65536L ;
  {
    dvibuf [dviptr ]= x / 256 ;
    incr ( dviptr ) ;
    if ( dviptr == dvilimit ) 
    dviswap () ;
  } 
  {
    dvibuf [dviptr ]= x % 256 ;
    incr ( dviptr ) ;
    if ( dviptr == dvilimit ) 
    dviswap () ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zdvipop ( integer l ) 
#else
zdvipop ( l ) 
  integer l ;
#endif
{
  dvipop_regmem 
  if ( ( l == dvioffset + dviptr ) && ( dviptr > 0 ) ) 
  decr ( dviptr ) ;
  else {
      
    dvibuf [dviptr ]= 142 ;
    incr ( dviptr ) ;
    if ( dviptr == dvilimit ) 
    dviswap () ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zdvifontdef ( internalfontnumber f ) 
#else
zdvifontdef ( f ) 
  internalfontnumber f ;
#endif
{
  dvifontdef_regmem 
  poolpointer k  ;
  if ( f <= 256 ) 
  {
    {
      dvibuf [dviptr ]= 243 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    {
      dvibuf [dviptr ]= f - 1 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
  } 
  else {
      
    {
      dvibuf [dviptr ]= 244 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    {
      dvibuf [dviptr ]= ( f - 1 ) / 256 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    {
      dvibuf [dviptr ]= ( f - 1 ) % 256 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
  } 
  {
    dvibuf [dviptr ]= fontcheck [f ].b0 ;
    incr ( dviptr ) ;
    if ( dviptr == dvilimit ) 
    dviswap () ;
  } 
  {
    dvibuf [dviptr ]= fontcheck [f ].b1 ;
    incr ( dviptr ) ;
    if ( dviptr == dvilimit ) 
    dviswap () ;
  } 
  {
    dvibuf [dviptr ]= fontcheck [f ].b2 ;
    incr ( dviptr ) ;
    if ( dviptr == dvilimit ) 
    dviswap () ;
  } 
  {
    dvibuf [dviptr ]= fontcheck [f ].b3 ;
    incr ( dviptr ) ;
    if ( dviptr == dvilimit ) 
    dviswap () ;
  } 
  dvifour ( fontsize [f ]) ;
  dvifour ( fontdsize [f ]) ;
  {
    dvibuf [dviptr ]= ( strstart [fontarea [f ]+ 1 ]- strstart [
    fontarea [f ]]) ;
    incr ( dviptr ) ;
    if ( dviptr == dvilimit ) 
    dviswap () ;
  } 
  {
    dvibuf [dviptr ]= ( strstart [fontname [f ]+ 1 ]- strstart [
    fontname [f ]]) ;
    incr ( dviptr ) ;
    if ( dviptr == dvilimit ) 
    dviswap () ;
  } 
  {register integer for_end; k = strstart [fontarea [f ]];for_end = 
  strstart [fontarea [f ]+ 1 ]- 1 ; if ( k <= for_end) do 
    {
      dvibuf [dviptr ]= strpool [k ];
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
  while ( k++ < for_end ) ;} 
  {register integer for_end; k = strstart [fontname [f ]];for_end = 
  strstart [fontname [f ]+ 1 ]- 1 ; if ( k <= for_end) do 
    {
      dvibuf [dviptr ]= strpool [k ];
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
  while ( k++ < for_end ) ;} 
} 
void 
#ifdef HAVE_PROTOTYPES
zmovement ( scaled w , eightbits o ) 
#else
zmovement ( w , o ) 
  scaled w ;
  eightbits o ;
#endif
{
  /* 10 40 45 2 1 */ movement_regmem 
  smallnumber mstate  ;
  halfword p, q  ;
  integer k  ;
  q = getnode ( 3 ) ;
  mem [q + 1 ].cint = w ;
  mem [q + 2 ].cint = dvioffset + dviptr ;
  if ( o == 157 ) 
  {
    mem [q ].hh .v.RH = downptr ;
    downptr = q ;
  } 
  else {
      
    mem [q ].hh .v.RH = rightptr ;
    rightptr = q ;
  } 
  p = mem [q ].hh .v.RH ;
  mstate = 0 ;
  while ( p != 0 ) {
      
    if ( mem [p + 1 ].cint == w ) 
    switch ( mstate + mem [p ].hh .v.LH ) 
    {case 3 : 
    case 4 : 
    case 15 : 
    case 16 : 
      if ( mem [p + 2 ].cint < dvigone ) 
      goto lab45 ;
      else {
	  
	k = mem [p + 2 ].cint - dvioffset ;
	if ( k < 0 ) 
	k = k + dvibufsize ;
	dvibuf [k ]= dvibuf [k ]+ 5 ;
	mem [p ].hh .v.LH = 1 ;
	goto lab40 ;
      } 
      break ;
    case 5 : 
    case 9 : 
    case 11 : 
      if ( mem [p + 2 ].cint < dvigone ) 
      goto lab45 ;
      else {
	  
	k = mem [p + 2 ].cint - dvioffset ;
	if ( k < 0 ) 
	k = k + dvibufsize ;
	dvibuf [k ]= dvibuf [k ]+ 10 ;
	mem [p ].hh .v.LH = 2 ;
	goto lab40 ;
      } 
      break ;
    case 1 : 
    case 2 : 
    case 8 : 
    case 13 : 
      goto lab40 ;
      break ;
      default: 
      ;
      break ;
    } 
    else switch ( mstate + mem [p ].hh .v.LH ) 
    {case 1 : 
      mstate = 6 ;
      break ;
    case 2 : 
      mstate = 12 ;
      break ;
    case 8 : 
    case 13 : 
      goto lab45 ;
      break ;
      default: 
      ;
      break ;
    } 
    p = mem [p ].hh .v.RH ;
  } 
  lab45: ;
  mem [q ].hh .v.LH = 3 ;
  if ( abs ( w ) >= 8388608L ) 
  {
    {
      dvibuf [dviptr ]= o + 3 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    dvifour ( w ) ;
    return ;
  } 
  if ( abs ( w ) >= 32768L ) 
  {
    {
      dvibuf [dviptr ]= o + 2 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    if ( w < 0 ) 
    w = w + 16777216L ;
    {
      dvibuf [dviptr ]= w / 65536L ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    w = w % 65536L ;
    goto lab2 ;
  } 
  if ( abs ( w ) >= 128 ) 
  {
    {
      dvibuf [dviptr ]= o + 1 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    if ( w < 0 ) 
    w = w + 65536L ;
    goto lab2 ;
  } 
  {
    dvibuf [dviptr ]= o ;
    incr ( dviptr ) ;
    if ( dviptr == dvilimit ) 
    dviswap () ;
  } 
  if ( w < 0 ) 
  w = w + 256 ;
  goto lab1 ;
  lab2: {
      
    dvibuf [dviptr ]= w / 256 ;
    incr ( dviptr ) ;
    if ( dviptr == dvilimit ) 
    dviswap () ;
  } 
  lab1: {
      
    dvibuf [dviptr ]= w % 256 ;
    incr ( dviptr ) ;
    if ( dviptr == dvilimit ) 
    dviswap () ;
  } 
  return ;
  lab40: mem [q ].hh .v.LH = mem [p ].hh .v.LH ;
  if ( mem [q ].hh .v.LH == 1 ) 
  {
    {
      dvibuf [dviptr ]= o + 4 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    while ( mem [q ].hh .v.RH != p ) {
	
      q = mem [q ].hh .v.RH ;
      switch ( mem [q ].hh .v.LH ) 
      {case 3 : 
	mem [q ].hh .v.LH = 5 ;
	break ;
      case 4 : 
	mem [q ].hh .v.LH = 6 ;
	break ;
	default: 
	;
	break ;
      } 
    } 
  } 
  else {
      
    {
      dvibuf [dviptr ]= o + 9 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    while ( mem [q ].hh .v.RH != p ) {
	
      q = mem [q ].hh .v.RH ;
      switch ( mem [q ].hh .v.LH ) 
      {case 3 : 
	mem [q ].hh .v.LH = 4 ;
	break ;
      case 5 : 
	mem [q ].hh .v.LH = 6 ;
	break ;
	default: 
	;
	break ;
      } 
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zprunemovements ( integer l ) 
#else
zprunemovements ( l ) 
  integer l ;
#endif
{
  /* 30 10 */ prunemovements_regmem 
  halfword p  ;
  while ( downptr != 0 ) {
      
    if ( mem [downptr + 2 ].cint < l ) 
    goto lab30 ;
    p = downptr ;
    downptr = mem [p ].hh .v.RH ;
    freenode ( p , 3 ) ;
  } 
  lab30: while ( rightptr != 0 ) {
      
    if ( mem [rightptr + 2 ].cint < l ) 
    return ;
    p = rightptr ;
    rightptr = mem [p ].hh .v.RH ;
    freenode ( p , 3 ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zspecialout ( halfword p ) 
#else
zspecialout ( p ) 
  halfword p ;
#endif
{
  specialout_regmem 
  char oldsetting  ;
  poolpointer k  ;
  if ( curh != dvih ) 
  {
    movement ( curh - dvih , 143 ) ;
    dvih = curh ;
  } 
  if ( curv != dviv ) 
  {
    movement ( curv - dviv , 157 ) ;
    dviv = curv ;
  } 
  oldsetting = selector ;
  selector = 21 ;
  showtokenlist ( mem [mem [p + 1 ].hh .v.RH ].hh .v.RH , 0 , poolsize - 
  poolptr ) ;
  selector = oldsetting ;
  {
    if ( poolptr + 1 > poolsize ) 
    overflow ( 257 , poolsize - initpoolptr ) ;
  } 
  if ( ( poolptr - strstart [strptr ]) < 256 ) 
  {
    {
      dvibuf [dviptr ]= 239 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    {
      dvibuf [dviptr ]= ( poolptr - strstart [strptr ]) ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
  } 
  else {
      
    {
      dvibuf [dviptr ]= 242 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    dvifour ( ( poolptr - strstart [strptr ]) ) ;
  } 
  {register integer for_end; k = strstart [strptr ];for_end = poolptr - 1 
  ; if ( k <= for_end) do 
    {
      dvibuf [dviptr ]= strpool [k ];
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
  while ( k++ < for_end ) ;} 
  poolptr = strstart [strptr ];
} 
void 
#ifdef HAVE_PROTOTYPES
zwriteout ( halfword p ) 
#else
zwriteout ( p ) 
  halfword p ;
#endif
{
  writeout_regmem 
  char oldsetting  ;
  integer oldmode  ;
  smallnumber j  ;
  halfword q, r  ;
  integer d  ;
  boolean clobbered  ;
  q = getavail () ;
  mem [q ].hh .v.LH = 637 ;
  r = getavail () ;
  mem [q ].hh .v.RH = r ;
  mem [r ].hh .v.LH = 14617 ;
  begintokenlist ( q , 4 ) ;
  begintokenlist ( mem [p + 1 ].hh .v.RH , 15 ) ;
  q = getavail () ;
  mem [q ].hh .v.LH = 379 ;
  begintokenlist ( q , 4 ) ;
  oldmode = curlist .modefield ;
  curlist .modefield = 0 ;
  curcs = writeloc ;
  q = scantoks ( false , true ) ;
  gettoken () ;
  if ( curtok != 14617 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 1300 ) ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 1301 ;
      helpline [0 ]= 1007 ;
    } 
    error () ;
    do {
	gettoken () ;
    } while ( ! ( curtok == 14617 ) ) ;
  } 
  curlist .modefield = oldmode ;
  endtokenlist () ;
  oldsetting = selector ;
  j = mem [p + 1 ].hh .v.LH ;
  if ( shellenabledp && ( j == 18 ) ) 
  {
    selector = 21 ;
  } 
  else if ( writeopen [j ]) 
  selector = j ;
  else {
      
    if ( ( j == 17 ) && ( selector == 19 ) ) 
    selector = 18 ;
    printnl ( 335 ) ;
  } 
  tokenshow ( defref ) ;
  println () ;
  flushlist ( defref ) ;
  if ( j == 18 ) 
  {
    if ( ( eqtb [15192 ].cint <= 0 ) ) 
    selector = 18 ;
    else selector = 19 ;
    printnl ( 1295 ) ;
    {register integer for_end; d = 0 ;for_end = ( poolptr - strstart [
    strptr ]) - 1 ; if ( d <= for_end) do 
      {
	print ( strpool [strstart [strptr ]+ d ]) ;
      } 
    while ( d++ < for_end ) ;} 
    print ( 1296 ) ;
    if ( shellenabledp ) 
    {
      {
	if ( poolptr + 1 > poolsize ) 
	overflow ( 257 , poolsize - initpoolptr ) ;
      } 
      {
	strpool [poolptr ]= 0 ;
	incr ( poolptr ) ;
      } 
      clobbered = false ;
      {register integer for_end; d = 0 ;for_end = ( poolptr - strstart [
      strptr ]) - 1 ; if ( d <= for_end) do 
	{
	  strpool [strstart [strptr ]+ d ]= xchr [strpool [strstart [
	  strptr ]+ d ]];
	  if ( ( strpool [strstart [strptr ]+ d ]== 0 ) && ( d < ( poolptr 
	  - strstart [strptr ]) - 1 ) ) 
	  clobbered = true ;
	} 
      while ( d++ < for_end ) ;} 
      if ( clobbered ) 
      print ( 1297 ) ;
      else {
	  
	system ( (char*)addressof ( strpool [strstart [strptr ]]) ) ;
	print ( 1298 ) ;
      } 
      poolptr = strstart [strptr ];
    } 
    else {
	
      print ( 1299 ) ;
    } 
    printchar ( 46 ) ;
    printnl ( 335 ) ;
    println () ;
  } 
  selector = oldsetting ;
} 
void 
#ifdef HAVE_PROTOTYPES
zoutwhat ( halfword p ) 
#else
zoutwhat ( p ) 
  halfword p ;
#endif
{
  outwhat_regmem 
  smallnumber j  ;
  char oldsetting  ;
  switch ( mem [p ].hh.b1 ) 
  {case 0 : 
  case 1 : 
  case 2 : 
    if ( ! doingleaders ) 
    {
      j = mem [p + 1 ].hh .v.LH ;
      if ( mem [p ].hh.b1 == 1 ) 
      writeout ( p ) ;
      else {
	  
	if ( writeopen [j ]) 
	aclose ( writefile [j ]) ;
	if ( mem [p ].hh.b1 == 2 ) 
	writeopen [j ]= false ;
	else if ( j < 16 ) 
	{
	  curname = mem [p + 1 ].hh .v.RH ;
	  curarea = mem [p + 2 ].hh .v.LH ;
	  curext = mem [p + 2 ].hh .v.RH ;
	  if ( curext == 335 ) 
	  curext = 787 ;
	  packfilename ( curname , curarea , curext ) ;
	  while ( ! openoutnameok ( (char*) nameoffile + 1 ) || ! aopenout ( writefile 
	  [j ]) ) promptfilename ( 1303 , 787 ) ;
	  writeopen [j ]= true ;
	  if ( logopened ) 
	  {
	    oldsetting = selector ;
	    if ( ( eqtb [15192 ].cint <= 0 ) ) 
	    selector = 18 ;
	    else selector = 19 ;
	    printnl ( 1304 ) ;
	    printint ( j ) ;
	    print ( 1305 ) ;
	    printfilename ( curname , curarea , curext ) ;
	    print ( 786 ) ;
	    printnl ( 335 ) ;
	    println () ;
	    selector = oldsetting ;
	  } 
	} 
      } 
    } 
    break ;
  case 3 : 
    specialout ( p ) ;
    break ;
  case 4 : 
    ;
    break ;
    default: 
    confusion ( 1302 ) ;
    break ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
hlistout ( void ) 
#else
hlistout ( ) 
#endif
{
  /* 21 13 14 15 22 40 */ hlistout_regmem 
  scaled baseline  ;
  scaled leftedge  ;
  scaled saveh, savev  ;
  halfword thisbox  ;
  glueord gorder  ;
  char gsign  ;
  halfword p  ;
  integer saveloc  ;
  halfword leaderbox  ;
  scaled leaderwd  ;
  scaled lx  ;
  boolean outerdoingleaders  ;
  scaled edge  ;
  real gluetemp  ;
  thisbox = tempptr ;
  gorder = mem [thisbox + 5 ].hh.b1 ;
  gsign = mem [thisbox + 5 ].hh.b0 ;
  p = mem [thisbox + 5 ].hh .v.RH ;
  incr ( curs ) ;
  if ( curs > 0 ) 
  {
    dvibuf [dviptr ]= 141 ;
    incr ( dviptr ) ;
    if ( dviptr == dvilimit ) 
    dviswap () ;
  } 
  if ( curs > maxpush ) 
  maxpush = curs ;
  saveloc = dvioffset + dviptr ;
  baseline = curv ;
  leftedge = curh ;
  while ( p != 0 ) lab21: if ( ( p >= himemmin ) ) 
  {
    if ( curh != dvih ) 
    {
      movement ( curh - dvih , 143 ) ;
      dvih = curh ;
    } 
    if ( curv != dviv ) 
    {
      movement ( curv - dviv , 157 ) ;
      dviv = curv ;
    } 
    do {
	f = mem [p ].hh.b0 ;
      c = mem [p ].hh.b1 ;
      if ( f != dvif ) 
      {
	if ( ! fontused [f ]) 
	{
	  dvifontdef ( f ) ;
	  fontused [f ]= true ;
	} 
	if ( f <= 64 ) 
	{
	  dvibuf [dviptr ]= f + 170 ;
	  incr ( dviptr ) ;
	  if ( dviptr == dvilimit ) 
	  dviswap () ;
	} 
	else if ( f <= 256 ) 
	{
	  {
	    dvibuf [dviptr ]= 235 ;
	    incr ( dviptr ) ;
	    if ( dviptr == dvilimit ) 
	    dviswap () ;
	  } 
	  {
	    dvibuf [dviptr ]= f - 1 ;
	    incr ( dviptr ) ;
	    if ( dviptr == dvilimit ) 
	    dviswap () ;
	  } 
	} 
	else {
	    
	  {
	    dvibuf [dviptr ]= 236 ;
	    incr ( dviptr ) ;
	    if ( dviptr == dvilimit ) 
	    dviswap () ;
	  } 
	  {
	    dvibuf [dviptr ]= ( f - 1 ) / 256 ;
	    incr ( dviptr ) ;
	    if ( dviptr == dvilimit ) 
	    dviswap () ;
	  } 
	  {
	    dvibuf [dviptr ]= ( f - 1 ) % 256 ;
	    incr ( dviptr ) ;
	    if ( dviptr == dvilimit ) 
	    dviswap () ;
	  } 
	} 
	dvif = f ;
      } 
      if ( fontec [f ]>= c ) 
      if ( fontbc [f ]<= c ) 
      if ( ( fontinfo [charbase [f ]+ c ].qqqq .b0 > 0 ) ) 
      {
	if ( c >= 128 ) 
	{
	  dvibuf [dviptr ]= 128 ;
	  incr ( dviptr ) ;
	  if ( dviptr == dvilimit ) 
	  dviswap () ;
	} 
	{
	  dvibuf [dviptr ]= c ;
	  incr ( dviptr ) ;
	  if ( dviptr == dvilimit ) 
	  dviswap () ;
	} 
	curh = curh + fontinfo [widthbase [f ]+ fontinfo [charbase [f ]+ 
	c ].qqqq .b0 ].cint ;
	goto lab22 ;
      } 
      if ( mltexenabledp ) 
      {
	if ( c >= eqtb [15218 ].cint ) 
	if ( c <= eqtb [15219 ].cint ) 
	if ( ( eqtb [14907 + c ].hh .v.RH > 0 ) ) 
	{
	  basec = ( eqtb [14907 + c ].hh .v.RH % 256 ) ;
	  accentc = ( eqtb [14907 + c ].hh .v.RH / 256 ) ;
	  if ( ( fontec [f ]>= basec ) ) 
	  if ( ( fontbc [f ]<= basec ) ) 
	  if ( ( fontec [f ]>= accentc ) ) 
	  if ( ( fontbc [f ]<= accentc ) ) 
	  {
	    iac = fontinfo [charbase [f ]+ effectivechar ( true , f , 
	    accentc ) ].qqqq ;
	    ibc = fontinfo [charbase [f ]+ effectivechar ( true , f , basec 
	    ) ].qqqq ;
	    if ( ( ibc .b0 > 0 ) ) 
	    if ( ( iac .b0 > 0 ) ) 
	    goto lab40 ;
	  } 
	  begindiagnostic () ;
	  printnl ( 1307 ) ;
	  print ( c ) ;
	  print ( 1197 ) ;
	  print ( accentc ) ;
	  print ( 32 ) ;
	  print ( basec ) ;
	  print ( 821 ) ;
	  print ( fontname [f ]) ;
	  printchar ( 33 ) ;
	  enddiagnostic ( false ) ;
	  goto lab22 ;
	} 
	begindiagnostic () ;
	printnl ( 820 ) ;
	print ( 1306 ) ;
	print ( c ) ;
	print ( 821 ) ;
	print ( fontname [f ]) ;
	printchar ( 33 ) ;
	enddiagnostic ( false ) ;
	goto lab22 ;
	lab40: if ( eqtb [15198 ].cint > 99 ) 
	{
	  begindiagnostic () ;
	  printnl ( 1308 ) ;
	  print ( c ) ;
	  print ( 1197 ) ;
	  print ( accentc ) ;
	  print ( 32 ) ;
	  print ( basec ) ;
	  print ( 821 ) ;
	  print ( fontname [f ]) ;
	  printchar ( 46 ) ;
	  enddiagnostic ( false ) ;
	} 
	basexheight = fontinfo [5 + parambase [f ]].cint ;
	baseslant = fontinfo [1 + parambase [f ]].cint / ((double) 65536.0 
	) ;
	accentslant = baseslant ;
	basewidth = fontinfo [widthbase [f ]+ ibc .b0 ].cint ;
	baseheight = fontinfo [heightbase [f ]+ ( ibc .b1 ) / 16 ].cint ;
	accentwidth = fontinfo [widthbase [f ]+ iac .b0 ].cint ;
	accentheight = fontinfo [heightbase [f ]+ ( iac .b1 ) / 16 ].cint 
	;
	delta = round ( ( basewidth - accentwidth ) / ((double) 2.0 ) + 
	baseheight * baseslant - basexheight * accentslant ) ;
	dvih = curh ;
	curh = curh + delta ;
	if ( curh != dvih ) 
	{
	  movement ( curh - dvih , 143 ) ;
	  dvih = curh ;
	} 
	if ( ( ( baseheight != basexheight ) && ( accentheight > 0 ) ) ) 
	{
	  curv = baseline + ( basexheight - baseheight ) ;
	  if ( curv != dviv ) 
	  {
	    movement ( curv - dviv , 157 ) ;
	    dviv = curv ;
	  } 
	  if ( accentc >= 128 ) 
	  {
	    dvibuf [dviptr ]= 128 ;
	    incr ( dviptr ) ;
	    if ( dviptr == dvilimit ) 
	    dviswap () ;
	  } 
	  {
	    dvibuf [dviptr ]= accentc ;
	    incr ( dviptr ) ;
	    if ( dviptr == dvilimit ) 
	    dviswap () ;
	  } 
	  curv = baseline ;
	} 
	else {
	    
	  if ( curv != dviv ) 
	  {
	    movement ( curv - dviv , 157 ) ;
	    dviv = curv ;
	  } 
	  if ( accentc >= 128 ) 
	  {
	    dvibuf [dviptr ]= 128 ;
	    incr ( dviptr ) ;
	    if ( dviptr == dvilimit ) 
	    dviswap () ;
	  } 
	  {
	    dvibuf [dviptr ]= accentc ;
	    incr ( dviptr ) ;
	    if ( dviptr == dvilimit ) 
	    dviswap () ;
	  } 
	} 
	curh = curh + accentwidth ;
	dvih = curh ;
	curh = curh + ( - (integer) accentwidth - delta ) ;
	if ( curh != dvih ) 
	{
	  movement ( curh - dvih , 143 ) ;
	  dvih = curh ;
	} 
	if ( curv != dviv ) 
	{
	  movement ( curv - dviv , 157 ) ;
	  dviv = curv ;
	} 
	if ( basec >= 128 ) 
	{
	  dvibuf [dviptr ]= 128 ;
	  incr ( dviptr ) ;
	  if ( dviptr == dvilimit ) 
	  dviswap () ;
	} 
	{
	  dvibuf [dviptr ]= basec ;
	  incr ( dviptr ) ;
	  if ( dviptr == dvilimit ) 
	  dviswap () ;
	} 
	curh = curh + basewidth ;
	dvih = curh ;
      } 
      lab22: p = mem [p ].hh .v.RH ;
    } while ( ! ( ! ( p >= himemmin ) ) ) ;
    dvih = curh ;
  } 
  else {
      
    switch ( mem [p ].hh.b0 ) 
    {case 0 : 
    case 1 : 
      if ( mem [p + 5 ].hh .v.RH == 0 ) 
      curh = curh + mem [p + 1 ].cint ;
      else {
	  
	saveh = dvih ;
	savev = dviv ;
	curv = baseline + mem [p + 4 ].cint ;
	tempptr = p ;
	edge = curh ;
	if ( mem [p ].hh.b0 == 1 ) 
	vlistout () ;
	else hlistout () ;
	dvih = saveh ;
	dviv = savev ;
	curh = edge + mem [p + 1 ].cint ;
	curv = baseline ;
      } 
      break ;
    case 2 : 
      {
	ruleht = mem [p + 3 ].cint ;
	ruledp = mem [p + 2 ].cint ;
	rulewd = mem [p + 1 ].cint ;
	goto lab14 ;
      } 
      break ;
    case 8 : 
      outwhat ( p ) ;
      break ;
    case 10 : 
      {
	g = mem [p + 1 ].hh .v.LH ;
	rulewd = mem [g + 1 ].cint ;
	if ( gsign != 0 ) 
	{
	  if ( gsign == 1 ) 
	  {
	    if ( mem [g ].hh.b0 == gorder ) 
	    {
	      gluetemp = mem [thisbox + 6 ].gr * mem [g + 2 ].cint ;
	      if ( gluetemp > 1000000000.0 ) 
	      gluetemp = 1000000000.0 ;
	      else if ( gluetemp < -1000000000.0 ) 
	      gluetemp = -1000000000.0 ;
	      rulewd = rulewd + round ( gluetemp ) ;
	    } 
	  } 
	  else if ( mem [g ].hh.b1 == gorder ) 
	  {
	    gluetemp = mem [thisbox + 6 ].gr * mem [g + 3 ].cint ;
	    if ( gluetemp > 1000000000.0 ) 
	    gluetemp = 1000000000.0 ;
	    else if ( gluetemp < -1000000000.0 ) 
	    gluetemp = -1000000000.0 ;
	    rulewd = rulewd - round ( gluetemp ) ;
	  } 
	} 
	if ( mem [p ].hh.b1 >= 100 ) 
	{
	  leaderbox = mem [p + 1 ].hh .v.RH ;
	  if ( mem [leaderbox ].hh.b0 == 2 ) 
	  {
	    ruleht = mem [leaderbox + 3 ].cint ;
	    ruledp = mem [leaderbox + 2 ].cint ;
	    goto lab14 ;
	  } 
	  leaderwd = mem [leaderbox + 1 ].cint ;
	  if ( ( leaderwd > 0 ) && ( rulewd > 0 ) ) 
	  {
	    rulewd = rulewd + 10 ;
	    edge = curh + rulewd ;
	    lx = 0 ;
	    if ( mem [p ].hh.b1 == 100 ) 
	    {
	      saveh = curh ;
	      curh = leftedge + leaderwd * ( ( curh - leftedge ) / leaderwd ) 
	      ;
	      if ( curh < saveh ) 
	      curh = curh + leaderwd ;
	    } 
	    else {
		
	      lq = rulewd / leaderwd ;
	      lr = rulewd % leaderwd ;
	      if ( mem [p ].hh.b1 == 101 ) 
	      curh = curh + ( lr / 2 ) ;
	      else {
		  
		lx = ( 2 * lr + lq + 1 ) / ( 2 * lq + 2 ) ;
		curh = curh + ( ( lr - ( lq - 1 ) * lx ) / 2 ) ;
	      } 
	    } 
	    while ( curh + leaderwd <= edge ) {
		
	      curv = baseline + mem [leaderbox + 4 ].cint ;
	      if ( curv != dviv ) 
	      {
		movement ( curv - dviv , 157 ) ;
		dviv = curv ;
	      } 
	      savev = dviv ;
	      if ( curh != dvih ) 
	      {
		movement ( curh - dvih , 143 ) ;
		dvih = curh ;
	      } 
	      saveh = dvih ;
	      tempptr = leaderbox ;
	      outerdoingleaders = doingleaders ;
	      doingleaders = true ;
	      if ( mem [leaderbox ].hh.b0 == 1 ) 
	      vlistout () ;
	      else hlistout () ;
	      doingleaders = outerdoingleaders ;
	      dviv = savev ;
	      dvih = saveh ;
	      curv = baseline ;
	      curh = saveh + leaderwd + lx ;
	    } 
	    curh = edge - 10 ;
	    goto lab15 ;
	  } 
	} 
	goto lab13 ;
      } 
      break ;
    case 11 : 
    case 9 : 
      curh = curh + mem [p + 1 ].cint ;
      break ;
    case 6 : 
      {
	mem [memtop - 12 ]= mem [p + 1 ];
	mem [memtop - 12 ].hh .v.RH = mem [p ].hh .v.RH ;
	p = memtop - 12 ;
	goto lab21 ;
      } 
      break ;
      default: 
      ;
      break ;
    } 
    goto lab15 ;
    lab14: if ( ( ruleht == -1073741824L ) ) 
    ruleht = mem [thisbox + 3 ].cint ;
    if ( ( ruledp == -1073741824L ) ) 
    ruledp = mem [thisbox + 2 ].cint ;
    ruleht = ruleht + ruledp ;
    if ( ( ruleht > 0 ) && ( rulewd > 0 ) ) 
    {
      if ( curh != dvih ) 
      {
	movement ( curh - dvih , 143 ) ;
	dvih = curh ;
      } 
      curv = baseline + ruledp ;
      if ( curv != dviv ) 
      {
	movement ( curv - dviv , 157 ) ;
	dviv = curv ;
      } 
      {
	dvibuf [dviptr ]= 132 ;
	incr ( dviptr ) ;
	if ( dviptr == dvilimit ) 
	dviswap () ;
      } 
      dvifour ( ruleht ) ;
      dvifour ( rulewd ) ;
      curv = baseline ;
      dvih = dvih + rulewd ;
    } 
    lab13: curh = curh + rulewd ;
    lab15: p = mem [p ].hh .v.RH ;
  } 
  prunemovements ( saveloc ) ;
  if ( curs > 0 ) 
  dvipop ( saveloc ) ;
  decr ( curs ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
vlistout ( void ) 
#else
vlistout ( ) 
#endif
{
  /* 13 14 15 */ vlistout_regmem 
  scaled leftedge  ;
  scaled topedge  ;
  scaled saveh, savev  ;
  halfword thisbox  ;
  glueord gorder  ;
  char gsign  ;
  halfword p  ;
  integer saveloc  ;
  halfword leaderbox  ;
  scaled leaderht  ;
  scaled lx  ;
  boolean outerdoingleaders  ;
  scaled edge  ;
  real gluetemp  ;
  thisbox = tempptr ;
  gorder = mem [thisbox + 5 ].hh.b1 ;
  gsign = mem [thisbox + 5 ].hh.b0 ;
  p = mem [thisbox + 5 ].hh .v.RH ;
  incr ( curs ) ;
  if ( curs > 0 ) 
  {
    dvibuf [dviptr ]= 141 ;
    incr ( dviptr ) ;
    if ( dviptr == dvilimit ) 
    dviswap () ;
  } 
  if ( curs > maxpush ) 
  maxpush = curs ;
  saveloc = dvioffset + dviptr ;
  leftedge = curh ;
  curv = curv - mem [thisbox + 3 ].cint ;
  topedge = curv ;
  while ( p != 0 ) {
      
    if ( ( p >= himemmin ) ) 
    confusion ( 823 ) ;
    else {
	
      switch ( mem [p ].hh.b0 ) 
      {case 0 : 
      case 1 : 
	if ( mem [p + 5 ].hh .v.RH == 0 ) 
	curv = curv + mem [p + 3 ].cint + mem [p + 2 ].cint ;
	else {
	    
	  curv = curv + mem [p + 3 ].cint ;
	  if ( curv != dviv ) 
	  {
	    movement ( curv - dviv , 157 ) ;
	    dviv = curv ;
	  } 
	  saveh = dvih ;
	  savev = dviv ;
	  curh = leftedge + mem [p + 4 ].cint ;
	  tempptr = p ;
	  if ( mem [p ].hh.b0 == 1 ) 
	  vlistout () ;
	  else hlistout () ;
	  dvih = saveh ;
	  dviv = savev ;
	  curv = savev + mem [p + 2 ].cint ;
	  curh = leftedge ;
	} 
	break ;
      case 2 : 
	{
	  ruleht = mem [p + 3 ].cint ;
	  ruledp = mem [p + 2 ].cint ;
	  rulewd = mem [p + 1 ].cint ;
	  goto lab14 ;
	} 
	break ;
      case 8 : 
	outwhat ( p ) ;
	break ;
      case 10 : 
	{
	  g = mem [p + 1 ].hh .v.LH ;
	  ruleht = mem [g + 1 ].cint ;
	  if ( gsign != 0 ) 
	  {
	    if ( gsign == 1 ) 
	    {
	      if ( mem [g ].hh.b0 == gorder ) 
	      {
		gluetemp = mem [thisbox + 6 ].gr * mem [g + 2 ].cint ;
		if ( gluetemp > 1000000000.0 ) 
		gluetemp = 1000000000.0 ;
		else if ( gluetemp < -1000000000.0 ) 
		gluetemp = -1000000000.0 ;
		ruleht = ruleht + round ( gluetemp ) ;
	      } 
	    } 
	    else if ( mem [g ].hh.b1 == gorder ) 
	    {
	      gluetemp = mem [thisbox + 6 ].gr * mem [g + 3 ].cint ;
	      if ( gluetemp > 1000000000.0 ) 
	      gluetemp = 1000000000.0 ;
	      else if ( gluetemp < -1000000000.0 ) 
	      gluetemp = -1000000000.0 ;
	      ruleht = ruleht - round ( gluetemp ) ;
	    } 
	  } 
	  if ( mem [p ].hh.b1 >= 100 ) 
	  {
	    leaderbox = mem [p + 1 ].hh .v.RH ;
	    if ( mem [leaderbox ].hh.b0 == 2 ) 
	    {
	      rulewd = mem [leaderbox + 1 ].cint ;
	      ruledp = 0 ;
	      goto lab14 ;
	    } 
	    leaderht = mem [leaderbox + 3 ].cint + mem [leaderbox + 2 ]
	    .cint ;
	    if ( ( leaderht > 0 ) && ( ruleht > 0 ) ) 
	    {
	      ruleht = ruleht + 10 ;
	      edge = curv + ruleht ;
	      lx = 0 ;
	      if ( mem [p ].hh.b1 == 100 ) 
	      {
		savev = curv ;
		curv = topedge + leaderht * ( ( curv - topedge ) / leaderht ) 
		;
		if ( curv < savev ) 
		curv = curv + leaderht ;
	      } 
	      else {
		  
		lq = ruleht / leaderht ;
		lr = ruleht % leaderht ;
		if ( mem [p ].hh.b1 == 101 ) 
		curv = curv + ( lr / 2 ) ;
		else {
		    
		  lx = ( 2 * lr + lq + 1 ) / ( 2 * lq + 2 ) ;
		  curv = curv + ( ( lr - ( lq - 1 ) * lx ) / 2 ) ;
		} 
	      } 
	      while ( curv + leaderht <= edge ) {
		  
		curh = leftedge + mem [leaderbox + 4 ].cint ;
		if ( curh != dvih ) 
		{
		  movement ( curh - dvih , 143 ) ;
		  dvih = curh ;
		} 
		saveh = dvih ;
		curv = curv + mem [leaderbox + 3 ].cint ;
		if ( curv != dviv ) 
		{
		  movement ( curv - dviv , 157 ) ;
		  dviv = curv ;
		} 
		savev = dviv ;
		tempptr = leaderbox ;
		outerdoingleaders = doingleaders ;
		doingleaders = true ;
		if ( mem [leaderbox ].hh.b0 == 1 ) 
		vlistout () ;
		else hlistout () ;
		doingleaders = outerdoingleaders ;
		dviv = savev ;
		dvih = saveh ;
		curh = leftedge ;
		curv = savev - mem [leaderbox + 3 ].cint + leaderht + lx ;
	      } 
	      curv = edge - 10 ;
	      goto lab15 ;
	    } 
	  } 
	  goto lab13 ;
	} 
	break ;
      case 11 : 
	curv = curv + mem [p + 1 ].cint ;
	break ;
	default: 
	;
	break ;
      } 
      goto lab15 ;
      lab14: if ( ( rulewd == -1073741824L ) ) 
      rulewd = mem [thisbox + 1 ].cint ;
      ruleht = ruleht + ruledp ;
      curv = curv + ruleht ;
      if ( ( ruleht > 0 ) && ( rulewd > 0 ) ) 
      {
	if ( curh != dvih ) 
	{
	  movement ( curh - dvih , 143 ) ;
	  dvih = curh ;
	} 
	if ( curv != dviv ) 
	{
	  movement ( curv - dviv , 157 ) ;
	  dviv = curv ;
	} 
	{
	  dvibuf [dviptr ]= 137 ;
	  incr ( dviptr ) ;
	  if ( dviptr == dvilimit ) 
	  dviswap () ;
	} 
	dvifour ( ruleht ) ;
	dvifour ( rulewd ) ;
      } 
      goto lab15 ;
      lab13: curv = curv + ruleht ;
    } 
    lab15: p = mem [p ].hh .v.RH ;
  } 
  prunemovements ( saveloc ) ;
  if ( curs > 0 ) 
  dvipop ( saveloc ) ;
  decr ( curs ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zshipout ( halfword p ) 
#else
zshipout ( p ) 
  halfword p ;
#endif
{
  /* 30 */ shipout_regmem 
  integer pageloc  ;
  char j, k  ;
  poolpointer s  ;
  char oldsetting  ;
  if ( eqtb [15197 ].cint > 0 ) 
  {
    printnl ( 335 ) ;
    println () ;
    print ( 824 ) ;
  } 
  if ( termoffset > maxprintline - 9 ) 
  println () ;
  else if ( ( termoffset > 0 ) || ( fileoffset > 0 ) ) 
  printchar ( 32 ) ;
  printchar ( 91 ) ;
  j = 9 ;
  while ( ( eqtb [15221 + j ].cint == 0 ) && ( j > 0 ) ) decr ( j ) ;
  {register integer for_end; k = 0 ;for_end = j ; if ( k <= for_end) do 
    {
      printint ( eqtb [15221 + k ].cint ) ;
      if ( k < j ) 
      printchar ( 46 ) ;
    } 
  while ( k++ < for_end ) ;} 
  fflush ( stdout ) ;
  if ( eqtb [15197 ].cint > 0 ) 
  {
    printchar ( 93 ) ;
    begindiagnostic () ;
    showbox ( p ) ;
    enddiagnostic ( true ) ;
  } 
  if ( ( mem [p + 3 ].cint > 1073741823L ) || ( mem [p + 2 ].cint > 
  1073741823L ) || ( mem [p + 3 ].cint + mem [p + 2 ].cint + eqtb [15752 
  ].cint > 1073741823L ) || ( mem [p + 1 ].cint + eqtb [15751 ].cint > 
  1073741823L ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 828 ) ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 829 ;
      helpline [0 ]= 830 ;
    } 
    error () ;
    if ( eqtb [15197 ].cint <= 0 ) 
    {
      begindiagnostic () ;
      printnl ( 831 ) ;
      showbox ( p ) ;
      enddiagnostic ( true ) ;
    } 
    goto lab30 ;
  } 
  if ( mem [p + 3 ].cint + mem [p + 2 ].cint + eqtb [15752 ].cint > maxv 
  ) 
  maxv = mem [p + 3 ].cint + mem [p + 2 ].cint + eqtb [15752 ].cint ;
  if ( mem [p + 1 ].cint + eqtb [15751 ].cint > maxh ) 
  maxh = mem [p + 1 ].cint + eqtb [15751 ].cint ;
  dvih = 0 ;
  dviv = 0 ;
  curh = eqtb [15751 ].cint ;
  dvif = 0 ;
  if ( outputfilename == 0 ) 
  {
    if ( jobname == 0 ) 
    openlogfile () ;
    packjobname ( 790 ) ;
    while ( ! bopenout ( dvifile ) ) promptfilename ( 791 , 790 ) ;
    outputfilename = bmakenamestring ( dvifile ) ;
  } 
  if ( totalpages == 0 ) 
  {
    {
      dvibuf [dviptr ]= 247 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    {
      dvibuf [dviptr ]= 2 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    dvifour ( 25400000L ) ;
    dvifour ( 473628672L ) ;
    preparemag () ;
    dvifour ( eqtb [15180 ].cint ) ;
    if ( outputcomment ) 
    {
      l = strlen ( outputcomment ) ;
      {
	dvibuf [dviptr ]= l ;
	incr ( dviptr ) ;
	if ( dviptr == dvilimit ) 
	dviswap () ;
      } 
      {register integer for_end; s = 0 ;for_end = l - 1 ; if ( s <= for_end) 
      do 
	{
	  dvibuf [dviptr ]= outputcomment [s ];
	  incr ( dviptr ) ;
	  if ( dviptr == dvilimit ) 
	  dviswap () ;
	} 
      while ( s++ < for_end ) ;} 
    } 
    else {
	
      oldsetting = selector ;
      selector = 21 ;
      print ( 822 ) ;
      printint ( eqtb [15186 ].cint ) ;
      printchar ( 46 ) ;
      printtwo ( eqtb [15185 ].cint ) ;
      printchar ( 46 ) ;
      printtwo ( eqtb [15184 ].cint ) ;
      printchar ( 58 ) ;
      printtwo ( eqtb [15183 ].cint / 60 ) ;
      printtwo ( eqtb [15183 ].cint % 60 ) ;
      selector = oldsetting ;
      {
	dvibuf [dviptr ]= ( poolptr - strstart [strptr ]) ;
	incr ( dviptr ) ;
	if ( dviptr == dvilimit ) 
	dviswap () ;
      } 
      {register integer for_end; s = strstart [strptr ];for_end = poolptr 
      - 1 ; if ( s <= for_end) do 
	{
	  dvibuf [dviptr ]= strpool [s ];
	  incr ( dviptr ) ;
	  if ( dviptr == dvilimit ) 
	  dviswap () ;
	} 
      while ( s++ < for_end ) ;} 
      poolptr = strstart [strptr ];
    } 
  } 
  pageloc = dvioffset + dviptr ;
  {
    dvibuf [dviptr ]= 139 ;
    incr ( dviptr ) ;
    if ( dviptr == dvilimit ) 
    dviswap () ;
  } 
  {register integer for_end; k = 0 ;for_end = 9 ; if ( k <= for_end) do 
    dvifour ( eqtb [15221 + k ].cint ) ;
  while ( k++ < for_end ) ;} 
  dvifour ( lastbop ) ;
  lastbop = pageloc ;
  curv = mem [p + 3 ].cint + eqtb [15752 ].cint ;
  tempptr = p ;
  if ( mem [p ].hh.b0 == 1 ) 
  vlistout () ;
  else hlistout () ;
  {
    dvibuf [dviptr ]= 140 ;
    incr ( dviptr ) ;
    if ( dviptr == dvilimit ) 
    dviswap () ;
  } 
  incr ( totalpages ) ;
  curs = -1 ;
	;
#ifdef IPC
  if ( ipcon > 0 ) 
  {
    if ( dvilimit == halfbuf ) 
    {
      writedvi ( halfbuf , dvibufsize - 1 ) ;
      flushdvi () ;
      dvigone = dvigone + halfbuf ;
    } 
    if ( dviptr > 0 ) 
    {
      writedvi ( 0 , dviptr - 1 ) ;
      flushdvi () ;
      dvioffset = dvioffset + dviptr ;
      dvigone = dvigone + dviptr ;
    } 
    dviptr = 0 ;
    dvilimit = dvibufsize ;
    ipcpage ( dvigone ) ;
  } 
#endif /* IPC */
  lab30: ;
  if ( eqtb [15197 ].cint <= 0 ) 
  printchar ( 93 ) ;
  deadcycles = 0 ;
  fflush ( stdout ) ;
	;
#ifdef STAT
  if ( eqtb [15194 ].cint > 1 ) 
  {
    printnl ( 825 ) ;
    printint ( varused ) ;
    printchar ( 38 ) ;
    printint ( dynused ) ;
    printchar ( 59 ) ;
  } 
#endif /* STAT */
  flushnodelist ( p ) ;
	;
#ifdef STAT
  if ( eqtb [15194 ].cint > 1 ) 
  {
    print ( 826 ) ;
    printint ( varused ) ;
    printchar ( 38 ) ;
    printint ( dynused ) ;
    print ( 827 ) ;
    printint ( himemmin - lomemmax - 1 ) ;
    println () ;
  } 
#endif /* STAT */
} 
void 
#ifdef HAVE_PROTOTYPES
zscanspec ( groupcode c , boolean threecodes ) 
#else
zscanspec ( c , threecodes ) 
  groupcode c ;
  boolean threecodes ;
#endif
{
  /* 40 */ scanspec_regmem 
  integer s  ;
  char speccode  ;
  if ( threecodes ) 
  s = savestack [saveptr + 0 ].cint ;
  if ( scankeyword ( 837 ) ) 
  speccode = 0 ;
  else if ( scankeyword ( 838 ) ) 
  speccode = 1 ;
  else {
      
    speccode = 1 ;
    curval = 0 ;
    goto lab40 ;
  } 
  scandimen ( false , false , false ) ;
  lab40: if ( threecodes ) 
  {
    savestack [saveptr + 0 ].cint = s ;
    incr ( saveptr ) ;
  } 
  savestack [saveptr + 0 ].cint = speccode ;
  savestack [saveptr + 1 ].cint = curval ;
  saveptr = saveptr + 2 ;
  newsavelevel ( c ) ;
  scanleftbrace () ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zhpack ( halfword p , scaled w , smallnumber m ) 
#else
zhpack ( p , w , m ) 
  halfword p ;
  scaled w ;
  smallnumber m ;
#endif
{
  /* 21 50 10 */ register halfword Result; hpack_regmem 
  halfword r  ;
  halfword q  ;
  scaled h, d, x  ;
  scaled s  ;
  halfword g  ;
  glueord o  ;
  internalfontnumber f  ;
  fourquarters i  ;
  eightbits hd  ;
  lastbadness = 0 ;
  r = getnode ( 7 ) ;
  mem [r ].hh.b0 = 0 ;
  mem [r ].hh.b1 = 0 ;
  mem [r + 4 ].cint = 0 ;
  q = r + 5 ;
  mem [q ].hh .v.RH = p ;
  h = 0 ;
  d = 0 ;
  x = 0 ;
  totalstretch [0 ]= 0 ;
  totalshrink [0 ]= 0 ;
  totalstretch [1 ]= 0 ;
  totalshrink [1 ]= 0 ;
  totalstretch [2 ]= 0 ;
  totalshrink [2 ]= 0 ;
  totalstretch [3 ]= 0 ;
  totalshrink [3 ]= 0 ;
  while ( p != 0 ) {
      
    lab21: while ( ( p >= himemmin ) ) {
	
      f = mem [p ].hh.b0 ;
      i = fontinfo [charbase [f ]+ effectivechar ( true , f , mem [p ]
      .hh.b1 ) ].qqqq ;
      hd = i .b1 ;
      x = x + fontinfo [widthbase [f ]+ i .b0 ].cint ;
      s = fontinfo [heightbase [f ]+ ( hd ) / 16 ].cint ;
      if ( s > h ) 
      h = s ;
      s = fontinfo [depthbase [f ]+ ( hd ) % 16 ].cint ;
      if ( s > d ) 
      d = s ;
      p = mem [p ].hh .v.RH ;
    } 
    if ( p != 0 ) 
    {
      switch ( mem [p ].hh.b0 ) 
      {case 0 : 
      case 1 : 
      case 2 : 
      case 13 : 
	{
	  x = x + mem [p + 1 ].cint ;
	  if ( mem [p ].hh.b0 >= 2 ) 
	  s = 0 ;
	  else s = mem [p + 4 ].cint ;
	  if ( mem [p + 3 ].cint - s > h ) 
	  h = mem [p + 3 ].cint - s ;
	  if ( mem [p + 2 ].cint + s > d ) 
	  d = mem [p + 2 ].cint + s ;
	} 
	break ;
      case 3 : 
      case 4 : 
      case 5 : 
	if ( adjusttail != 0 ) 
	{
	  while ( mem [q ].hh .v.RH != p ) q = mem [q ].hh .v.RH ;
	  if ( mem [p ].hh.b0 == 5 ) 
	  {
	    mem [adjusttail ].hh .v.RH = mem [p + 1 ].cint ;
	    while ( mem [adjusttail ].hh .v.RH != 0 ) adjusttail = mem [
	    adjusttail ].hh .v.RH ;
	    p = mem [p ].hh .v.RH ;
	    freenode ( mem [q ].hh .v.RH , 2 ) ;
	  } 
	  else {
	      
	    mem [adjusttail ].hh .v.RH = p ;
	    adjusttail = p ;
	    p = mem [p ].hh .v.RH ;
	  } 
	  mem [q ].hh .v.RH = p ;
	  p = q ;
	} 
	break ;
      case 8 : 
	;
	break ;
      case 10 : 
	{
	  g = mem [p + 1 ].hh .v.LH ;
	  x = x + mem [g + 1 ].cint ;
	  o = mem [g ].hh.b0 ;
	  totalstretch [o ]= totalstretch [o ]+ mem [g + 2 ].cint ;
	  o = mem [g ].hh.b1 ;
	  totalshrink [o ]= totalshrink [o ]+ mem [g + 3 ].cint ;
	  if ( mem [p ].hh.b1 >= 100 ) 
	  {
	    g = mem [p + 1 ].hh .v.RH ;
	    if ( mem [g + 3 ].cint > h ) 
	    h = mem [g + 3 ].cint ;
	    if ( mem [g + 2 ].cint > d ) 
	    d = mem [g + 2 ].cint ;
	  } 
	} 
	break ;
      case 11 : 
      case 9 : 
	x = x + mem [p + 1 ].cint ;
	break ;
      case 6 : 
	{
	  mem [memtop - 12 ]= mem [p + 1 ];
	  mem [memtop - 12 ].hh .v.RH = mem [p ].hh .v.RH ;
	  p = memtop - 12 ;
	  goto lab21 ;
	} 
	break ;
	default: 
	;
	break ;
      } 
      p = mem [p ].hh .v.RH ;
    } 
  } 
  if ( adjusttail != 0 ) 
  mem [adjusttail ].hh .v.RH = 0 ;
  mem [r + 3 ].cint = h ;
  mem [r + 2 ].cint = d ;
  if ( m == 1 ) 
  w = x + w ;
  mem [r + 1 ].cint = w ;
  x = w - x ;
  if ( x == 0 ) 
  {
    mem [r + 5 ].hh.b0 = 0 ;
    mem [r + 5 ].hh.b1 = 0 ;
    mem [r + 6 ].gr = 0.0 ;
    goto lab10 ;
  } 
  else if ( x > 0 ) 
  {
    if ( totalstretch [3 ]!= 0 ) 
    o = 3 ;
    else if ( totalstretch [2 ]!= 0 ) 
    o = 2 ;
    else if ( totalstretch [1 ]!= 0 ) 
    o = 1 ;
    else o = 0 ;
    mem [r + 5 ].hh.b1 = o ;
    mem [r + 5 ].hh.b0 = 1 ;
    if ( totalstretch [o ]!= 0 ) 
    mem [r + 6 ].gr = x / ((double) totalstretch [o ]) ;
    else {
	
      mem [r + 5 ].hh.b0 = 0 ;
      mem [r + 6 ].gr = 0.0 ;
    } 
    if ( o == 0 ) 
    if ( mem [r + 5 ].hh .v.RH != 0 ) 
    {
      lastbadness = badness ( x , totalstretch [0 ]) ;
      if ( lastbadness > eqtb [15189 ].cint ) 
      {
	println () ;
	if ( lastbadness > 100 ) 
	printnl ( 839 ) ;
	else printnl ( 840 ) ;
	print ( 841 ) ;
	printint ( lastbadness ) ;
	goto lab50 ;
      } 
    } 
    goto lab10 ;
  } 
  else {
      
    if ( totalshrink [3 ]!= 0 ) 
    o = 3 ;
    else if ( totalshrink [2 ]!= 0 ) 
    o = 2 ;
    else if ( totalshrink [1 ]!= 0 ) 
    o = 1 ;
    else o = 0 ;
    mem [r + 5 ].hh.b1 = o ;
    mem [r + 5 ].hh.b0 = 2 ;
    if ( totalshrink [o ]!= 0 ) 
    mem [r + 6 ].gr = ( - (integer) x ) / ((double) totalshrink [o ]) ;
    else {
	
      mem [r + 5 ].hh.b0 = 0 ;
      mem [r + 6 ].gr = 0.0 ;
    } 
    if ( ( totalshrink [o ]< - (integer) x ) && ( o == 0 ) && ( mem [r + 5 
    ].hh .v.RH != 0 ) ) 
    {
      lastbadness = 1000000L ;
      mem [r + 6 ].gr = 1.0 ;
      if ( ( - (integer) x - totalshrink [0 ]> eqtb [15741 ].cint ) || ( 
      eqtb [15189 ].cint < 100 ) ) 
      {
	if ( ( eqtb [15749 ].cint > 0 ) && ( - (integer) x - totalshrink [0 
	]> eqtb [15741 ].cint ) ) 
	{
	  while ( mem [q ].hh .v.RH != 0 ) q = mem [q ].hh .v.RH ;
	  mem [q ].hh .v.RH = newrule () ;
	  mem [mem [q ].hh .v.RH + 1 ].cint = eqtb [15749 ].cint ;
	} 
	println () ;
	printnl ( 847 ) ;
	printscaled ( - (integer) x - totalshrink [0 ]) ;
	print ( 848 ) ;
	goto lab50 ;
      } 
    } 
    else if ( o == 0 ) 
    if ( mem [r + 5 ].hh .v.RH != 0 ) 
    {
      lastbadness = badness ( - (integer) x , totalshrink [0 ]) ;
      if ( lastbadness > eqtb [15189 ].cint ) 
      {
	println () ;
	printnl ( 849 ) ;
	printint ( lastbadness ) ;
	goto lab50 ;
      } 
    } 
    goto lab10 ;
  } 
  lab50: if ( outputactive ) 
  print ( 842 ) ;
  else {
      
    if ( packbeginline != 0 ) 
    {
      if ( packbeginline > 0 ) 
      print ( 843 ) ;
      else print ( 844 ) ;
      printint ( abs ( packbeginline ) ) ;
      print ( 845 ) ;
    } 
    else print ( 846 ) ;
    printint ( line ) ;
  } 
  println () ;
  fontinshortdisplay = 0 ;
  shortdisplay ( mem [r + 5 ].hh .v.RH ) ;
  println () ;
  begindiagnostic () ;
  showbox ( r ) ;
  enddiagnostic ( true ) ;
  lab10: Result = r ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zvpackage ( halfword p , scaled h , smallnumber m , scaled l ) 
#else
zvpackage ( p , h , m , l ) 
  halfword p ;
  scaled h ;
  smallnumber m ;
  scaled l ;
#endif
{
  /* 50 10 */ register halfword Result; vpackage_regmem 
  halfword r  ;
  scaled w, d, x  ;
  scaled s  ;
  halfword g  ;
  glueord o  ;
  lastbadness = 0 ;
  r = getnode ( 7 ) ;
  mem [r ].hh.b0 = 1 ;
  mem [r ].hh.b1 = 0 ;
  mem [r + 4 ].cint = 0 ;
  mem [r + 5 ].hh .v.RH = p ;
  w = 0 ;
  d = 0 ;
  x = 0 ;
  totalstretch [0 ]= 0 ;
  totalshrink [0 ]= 0 ;
  totalstretch [1 ]= 0 ;
  totalshrink [1 ]= 0 ;
  totalstretch [2 ]= 0 ;
  totalshrink [2 ]= 0 ;
  totalstretch [3 ]= 0 ;
  totalshrink [3 ]= 0 ;
  while ( p != 0 ) {
      
    if ( ( p >= himemmin ) ) 
    confusion ( 850 ) ;
    else switch ( mem [p ].hh.b0 ) 
    {case 0 : 
    case 1 : 
    case 2 : 
    case 13 : 
      {
	x = x + d + mem [p + 3 ].cint ;
	d = mem [p + 2 ].cint ;
	if ( mem [p ].hh.b0 >= 2 ) 
	s = 0 ;
	else s = mem [p + 4 ].cint ;
	if ( mem [p + 1 ].cint + s > w ) 
	w = mem [p + 1 ].cint + s ;
      } 
      break ;
    case 8 : 
      ;
      break ;
    case 10 : 
      {
	x = x + d ;
	d = 0 ;
	g = mem [p + 1 ].hh .v.LH ;
	x = x + mem [g + 1 ].cint ;
	o = mem [g ].hh.b0 ;
	totalstretch [o ]= totalstretch [o ]+ mem [g + 2 ].cint ;
	o = mem [g ].hh.b1 ;
	totalshrink [o ]= totalshrink [o ]+ mem [g + 3 ].cint ;
	if ( mem [p ].hh.b1 >= 100 ) 
	{
	  g = mem [p + 1 ].hh .v.RH ;
	  if ( mem [g + 1 ].cint > w ) 
	  w = mem [g + 1 ].cint ;
	} 
      } 
      break ;
    case 11 : 
      {
	x = x + d + mem [p + 1 ].cint ;
	d = 0 ;
      } 
      break ;
      default: 
      ;
      break ;
    } 
    p = mem [p ].hh .v.RH ;
  } 
  mem [r + 1 ].cint = w ;
  if ( d > l ) 
  {
    x = x + d - l ;
    mem [r + 2 ].cint = l ;
  } 
  else mem [r + 2 ].cint = d ;
  if ( m == 1 ) 
  h = x + h ;
  mem [r + 3 ].cint = h ;
  x = h - x ;
  if ( x == 0 ) 
  {
    mem [r + 5 ].hh.b0 = 0 ;
    mem [r + 5 ].hh.b1 = 0 ;
    mem [r + 6 ].gr = 0.0 ;
    goto lab10 ;
  } 
  else if ( x > 0 ) 
  {
    if ( totalstretch [3 ]!= 0 ) 
    o = 3 ;
    else if ( totalstretch [2 ]!= 0 ) 
    o = 2 ;
    else if ( totalstretch [1 ]!= 0 ) 
    o = 1 ;
    else o = 0 ;
    mem [r + 5 ].hh.b1 = o ;
    mem [r + 5 ].hh.b0 = 1 ;
    if ( totalstretch [o ]!= 0 ) 
    mem [r + 6 ].gr = x / ((double) totalstretch [o ]) ;
    else {
	
      mem [r + 5 ].hh.b0 = 0 ;
      mem [r + 6 ].gr = 0.0 ;
    } 
    if ( o == 0 ) 
    if ( mem [r + 5 ].hh .v.RH != 0 ) 
    {
      lastbadness = badness ( x , totalstretch [0 ]) ;
      if ( lastbadness > eqtb [15190 ].cint ) 
      {
	println () ;
	if ( lastbadness > 100 ) 
	printnl ( 839 ) ;
	else printnl ( 840 ) ;
	print ( 851 ) ;
	printint ( lastbadness ) ;
	goto lab50 ;
      } 
    } 
    goto lab10 ;
  } 
  else {
      
    if ( totalshrink [3 ]!= 0 ) 
    o = 3 ;
    else if ( totalshrink [2 ]!= 0 ) 
    o = 2 ;
    else if ( totalshrink [1 ]!= 0 ) 
    o = 1 ;
    else o = 0 ;
    mem [r + 5 ].hh.b1 = o ;
    mem [r + 5 ].hh.b0 = 2 ;
    if ( totalshrink [o ]!= 0 ) 
    mem [r + 6 ].gr = ( - (integer) x ) / ((double) totalshrink [o ]) ;
    else {
	
      mem [r + 5 ].hh.b0 = 0 ;
      mem [r + 6 ].gr = 0.0 ;
    } 
    if ( ( totalshrink [o ]< - (integer) x ) && ( o == 0 ) && ( mem [r + 5 
    ].hh .v.RH != 0 ) ) 
    {
      lastbadness = 1000000L ;
      mem [r + 6 ].gr = 1.0 ;
      if ( ( - (integer) x - totalshrink [0 ]> eqtb [15742 ].cint ) || ( 
      eqtb [15190 ].cint < 100 ) ) 
      {
	println () ;
	printnl ( 852 ) ;
	printscaled ( - (integer) x - totalshrink [0 ]) ;
	print ( 853 ) ;
	goto lab50 ;
      } 
    } 
    else if ( o == 0 ) 
    if ( mem [r + 5 ].hh .v.RH != 0 ) 
    {
      lastbadness = badness ( - (integer) x , totalshrink [0 ]) ;
      if ( lastbadness > eqtb [15190 ].cint ) 
      {
	println () ;
	printnl ( 854 ) ;
	printint ( lastbadness ) ;
	goto lab50 ;
      } 
    } 
    goto lab10 ;
  } 
  lab50: if ( outputactive ) 
  print ( 842 ) ;
  else {
      
    if ( packbeginline != 0 ) 
    {
      print ( 844 ) ;
      printint ( abs ( packbeginline ) ) ;
      print ( 845 ) ;
    } 
    else print ( 846 ) ;
    printint ( line ) ;
    println () ;
  } 
  begindiagnostic () ;
  showbox ( r ) ;
  enddiagnostic ( true ) ;
  lab10: Result = r ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zappendtovlist ( halfword b ) 
#else
zappendtovlist ( b ) 
  halfword b ;
#endif
{
  appendtovlist_regmem 
  scaled d  ;
  halfword p  ;
  if ( curlist .auxfield .cint > -65536000L ) 
  {
    d = mem [eqtb [12527 ].hh .v.RH + 1 ].cint - curlist .auxfield .cint - 
    mem [b + 3 ].cint ;
    if ( d < eqtb [15735 ].cint ) 
    p = newparamglue ( 0 ) ;
    else {
	
      p = newskipparam ( 1 ) ;
      mem [tempptr + 1 ].cint = d ;
    } 
    mem [curlist .tailfield ].hh .v.RH = p ;
    curlist .tailfield = p ;
  } 
  mem [curlist .tailfield ].hh .v.RH = b ;
  curlist .tailfield = b ;
  curlist .auxfield .cint = mem [b + 2 ].cint ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
newnoad ( void ) 
#else
newnoad ( ) 
#endif
{
  register halfword Result; newnoad_regmem 
  halfword p  ;
  p = getnode ( 4 ) ;
  mem [p ].hh.b0 = 16 ;
  mem [p ].hh.b1 = 0 ;
  mem [p + 1 ].hh = emptyfield ;
  mem [p + 3 ].hh = emptyfield ;
  mem [p + 2 ].hh = emptyfield ;
  Result = p ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
znewstyle ( smallnumber s ) 
#else
znewstyle ( s ) 
  smallnumber s ;
#endif
{
  register halfword Result; newstyle_regmem 
  halfword p  ;
  p = getnode ( 3 ) ;
  mem [p ].hh.b0 = 14 ;
  mem [p ].hh.b1 = s ;
  mem [p + 1 ].cint = 0 ;
  mem [p + 2 ].cint = 0 ;
  Result = p ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
newchoice ( void ) 
#else
newchoice ( ) 
#endif
{
  register halfword Result; newchoice_regmem 
  halfword p  ;
  p = getnode ( 3 ) ;
  mem [p ].hh.b0 = 15 ;
  mem [p ].hh.b1 = 0 ;
  mem [p + 1 ].hh .v.LH = 0 ;
  mem [p + 1 ].hh .v.RH = 0 ;
  mem [p + 2 ].hh .v.LH = 0 ;
  mem [p + 2 ].hh .v.RH = 0 ;
  Result = p ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
showinfo ( void ) 
#else
showinfo ( ) 
#endif
{
  showinfo_regmem 
  shownodelist ( mem [tempptr ].hh .v.LH ) ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zfractionrule ( scaled t ) 
#else
zfractionrule ( t ) 
  scaled t ;
#endif
{
  register halfword Result; fractionrule_regmem 
  halfword p  ;
  p = newrule () ;
  mem [p + 3 ].cint = t ;
  mem [p + 2 ].cint = 0 ;
  Result = p ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zoverbar ( halfword b , scaled k , scaled t ) 
#else
zoverbar ( b , k , t ) 
  halfword b ;
  scaled k ;
  scaled t ;
#endif
{
  register halfword Result; overbar_regmem 
  halfword p, q  ;
  p = newkern ( k ) ;
  mem [p ].hh .v.RH = b ;
  q = fractionrule ( t ) ;
  mem [q ].hh .v.RH = p ;
  p = newkern ( t ) ;
  mem [p ].hh .v.RH = q ;
  Result = vpackage ( p , 0 , 1 , 1073741823L ) ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zcharbox ( internalfontnumber f , quarterword c ) 
#else
zcharbox ( f , c ) 
  internalfontnumber f ;
  quarterword c ;
#endif
{
  register halfword Result; charbox_regmem 
  fourquarters q  ;
  eightbits hd  ;
  halfword b, p  ;
  q = fontinfo [charbase [f ]+ effectivechar ( true , f , c ) ].qqqq ;
  hd = q .b1 ;
  b = newnullbox () ;
  mem [b + 1 ].cint = fontinfo [widthbase [f ]+ q .b0 ].cint + fontinfo 
  [italicbase [f ]+ ( q .b2 ) / 4 ].cint ;
  mem [b + 3 ].cint = fontinfo [heightbase [f ]+ ( hd ) / 16 ].cint ;
  mem [b + 2 ].cint = fontinfo [depthbase [f ]+ ( hd ) % 16 ].cint ;
  p = getavail () ;
  mem [p ].hh.b1 = c ;
  mem [p ].hh.b0 = f ;
  mem [b + 5 ].hh .v.RH = p ;
  Result = b ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zstackintobox ( halfword b , internalfontnumber f , quarterword c ) 
#else
zstackintobox ( b , f , c ) 
  halfword b ;
  internalfontnumber f ;
  quarterword c ;
#endif
{
  stackintobox_regmem 
  halfword p  ;
  p = charbox ( f , c ) ;
  mem [p ].hh .v.RH = mem [b + 5 ].hh .v.RH ;
  mem [b + 5 ].hh .v.RH = p ;
  mem [b + 3 ].cint = mem [p + 3 ].cint ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zheightplusdepth ( internalfontnumber f , quarterword c ) 
#else
zheightplusdepth ( f , c ) 
  internalfontnumber f ;
  quarterword c ;
#endif
{
  register scaled Result; heightplusdepth_regmem 
  fourquarters q  ;
  eightbits hd  ;
  q = fontinfo [charbase [f ]+ effectivechar ( true , f , c ) ].qqqq ;
  hd = q .b1 ;
  Result = fontinfo [heightbase [f ]+ ( hd ) / 16 ].cint + fontinfo [
  depthbase [f ]+ ( hd ) % 16 ].cint ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zvardelimiter ( halfword d , smallnumber s , scaled v ) 
#else
zvardelimiter ( d , s , v ) 
  halfword d ;
  smallnumber s ;
  scaled v ;
#endif
{
  /* 40 22 */ register halfword Result; vardelimiter_regmem 
  halfword b  ;
  internalfontnumber f, g  ;
  quarterword c, x, y  ;
  integer m, n  ;
  scaled u  ;
  scaled w  ;
  fourquarters q  ;
  eightbits hd  ;
  fourquarters r  ;
  smallnumber z  ;
  boolean largeattempt  ;
  f = 0 ;
  w = 0 ;
  largeattempt = false ;
  z = mem [d ].qqqq .b0 ;
  x = mem [d ].qqqq .b1 ;
  while ( true ) {
      
    if ( ( z != 0 ) || ( x != 0 ) ) 
    {
      z = z + s + 16 ;
      do {
	  z = z - 16 ;
	g = eqtb [13579 + z ].hh .v.RH ;
	if ( g != 0 ) 
	{
	  y = x ;
	  if ( ( y >= fontbc [g ]) && ( y <= fontec [g ]) ) 
	  {
	    lab22: q = fontinfo [charbase [g ]+ y ].qqqq ;
	    if ( ( q .b0 > 0 ) ) 
	    {
	      if ( ( ( q .b2 ) % 4 ) == 3 ) 
	      {
		f = g ;
		c = y ;
		goto lab40 ;
	      } 
	      hd = q .b1 ;
	      u = fontinfo [heightbase [g ]+ ( hd ) / 16 ].cint + fontinfo 
	      [depthbase [g ]+ ( hd ) % 16 ].cint ;
	      if ( u > w ) 
	      {
		f = g ;
		c = y ;
		w = u ;
		if ( u >= v ) 
		goto lab40 ;
	      } 
	      if ( ( ( q .b2 ) % 4 ) == 2 ) 
	      {
		y = q .b3 ;
		goto lab22 ;
	      } 
	    } 
	  } 
	} 
      } while ( ! ( z < 16 ) ) ;
    } 
    if ( largeattempt ) 
    goto lab40 ;
    largeattempt = true ;
    z = mem [d ].qqqq .b2 ;
    x = mem [d ].qqqq .b3 ;
  } 
  lab40: if ( f != 0 ) 
  if ( ( ( q .b2 ) % 4 ) == 3 ) 
  {
    b = newnullbox () ;
    mem [b ].hh.b0 = 1 ;
    r = fontinfo [extenbase [f ]+ q .b3 ].qqqq ;
    c = r .b3 ;
    u = heightplusdepth ( f , c ) ;
    w = 0 ;
    q = fontinfo [charbase [f ]+ effectivechar ( true , f , c ) ].qqqq ;
    mem [b + 1 ].cint = fontinfo [widthbase [f ]+ q .b0 ].cint + 
    fontinfo [italicbase [f ]+ ( q .b2 ) / 4 ].cint ;
    c = r .b2 ;
    if ( c != 0 ) 
    w = w + heightplusdepth ( f , c ) ;
    c = r .b1 ;
    if ( c != 0 ) 
    w = w + heightplusdepth ( f , c ) ;
    c = r .b0 ;
    if ( c != 0 ) 
    w = w + heightplusdepth ( f , c ) ;
    n = 0 ;
    if ( u > 0 ) 
    while ( w < v ) {
	
      w = w + u ;
      incr ( n ) ;
      if ( r .b1 != 0 ) 
      w = w + u ;
    } 
    c = r .b2 ;
    if ( c != 0 ) 
    stackintobox ( b , f , c ) ;
    c = r .b3 ;
    {register integer for_end; m = 1 ;for_end = n ; if ( m <= for_end) do 
      stackintobox ( b , f , c ) ;
    while ( m++ < for_end ) ;} 
    c = r .b1 ;
    if ( c != 0 ) 
    {
      stackintobox ( b , f , c ) ;
      c = r .b3 ;
      {register integer for_end; m = 1 ;for_end = n ; if ( m <= for_end) do 
	stackintobox ( b , f , c ) ;
      while ( m++ < for_end ) ;} 
    } 
    c = r .b0 ;
    if ( c != 0 ) 
    stackintobox ( b , f , c ) ;
    mem [b + 2 ].cint = w - mem [b + 3 ].cint ;
  } 
  else b = charbox ( f , c ) ;
  else {
      
    b = newnullbox () ;
    mem [b + 1 ].cint = eqtb [15744 ].cint ;
  } 
  mem [b + 4 ].cint = half ( mem [b + 3 ].cint - mem [b + 2 ].cint ) - 
  fontinfo [22 + parambase [eqtb [13581 + s ].hh .v.RH ]].cint ;
  Result = b ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zrebox ( halfword b , scaled w ) 
#else
zrebox ( b , w ) 
  halfword b ;
  scaled w ;
#endif
{
  register halfword Result; rebox_regmem 
  halfword p  ;
  internalfontnumber f  ;
  scaled v  ;
  if ( ( mem [b + 1 ].cint != w ) && ( mem [b + 5 ].hh .v.RH != 0 ) ) 
  {
    if ( mem [b ].hh.b0 == 1 ) 
    b = hpack ( b , 0 , 1 ) ;
    p = mem [b + 5 ].hh .v.RH ;
    if ( ( ( p >= himemmin ) ) && ( mem [p ].hh .v.RH == 0 ) ) 
    {
      f = mem [p ].hh.b0 ;
      v = fontinfo [widthbase [f ]+ fontinfo [charbase [f ]+ 
      effectivechar ( true , f , mem [p ].hh.b1 ) ].qqqq .b0 ].cint ;
      if ( v != mem [b + 1 ].cint ) 
      mem [p ].hh .v.RH = newkern ( mem [b + 1 ].cint - v ) ;
    } 
    freenode ( b , 7 ) ;
    b = newglue ( membot + 12 ) ;
    mem [b ].hh .v.RH = p ;
    while ( mem [p ].hh .v.RH != 0 ) p = mem [p ].hh .v.RH ;
    mem [p ].hh .v.RH = newglue ( membot + 12 ) ;
    Result = hpack ( b , w , 0 ) ;
  } 
  else {
      
    mem [b + 1 ].cint = w ;
    Result = b ;
  } 
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zmathglue ( halfword g , scaled m ) 
#else
zmathglue ( g , m ) 
  halfword g ;
  scaled m ;
#endif
{
  register halfword Result; mathglue_regmem 
  halfword p  ;
  integer n  ;
  scaled f  ;
  n = xovern ( m , 65536L ) ;
  f = texremainder ;
  if ( f < 0 ) 
  {
    decr ( n ) ;
    f = f + 65536L ;
  } 
  p = getnode ( 4 ) ;
  mem [p + 1 ].cint = multandadd ( n , mem [g + 1 ].cint , xnoverd ( mem [
  g + 1 ].cint , f , 65536L ) , 1073741823L ) ;
  mem [p ].hh.b0 = mem [g ].hh.b0 ;
  if ( mem [p ].hh.b0 == 0 ) 
  mem [p + 2 ].cint = multandadd ( n , mem [g + 2 ].cint , xnoverd ( mem [
  g + 2 ].cint , f , 65536L ) , 1073741823L ) ;
  else mem [p + 2 ].cint = mem [g + 2 ].cint ;
  mem [p ].hh.b1 = mem [g ].hh.b1 ;
  if ( mem [p ].hh.b1 == 0 ) 
  mem [p + 3 ].cint = multandadd ( n , mem [g + 3 ].cint , xnoverd ( mem [
  g + 3 ].cint , f , 65536L ) , 1073741823L ) ;
  else mem [p + 3 ].cint = mem [g + 3 ].cint ;
  Result = p ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zmathkern ( halfword p , scaled m ) 
#else
zmathkern ( p , m ) 
  halfword p ;
  scaled m ;
#endif
{
  mathkern_regmem 
  integer n  ;
  scaled f  ;
  if ( mem [p ].hh.b1 == 99 ) 
  {
    n = xovern ( m , 65536L ) ;
    f = texremainder ;
    if ( f < 0 ) 
    {
      decr ( n ) ;
      f = f + 65536L ;
    } 
    mem [p + 1 ].cint = multandadd ( n , mem [p + 1 ].cint , xnoverd ( mem 
    [p + 1 ].cint , f , 65536L ) , 1073741823L ) ;
    mem [p ].hh.b1 = 1 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
flushmath ( void ) 
#else
flushmath ( ) 
#endif
{
  flushmath_regmem 
  flushnodelist ( mem [curlist .headfield ].hh .v.RH ) ;
  flushnodelist ( curlist .auxfield .cint ) ;
  mem [curlist .headfield ].hh .v.RH = 0 ;
  curlist .tailfield = curlist .headfield ;
  curlist .auxfield .cint = 0 ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zcleanbox ( halfword p , smallnumber s ) 
#else
zcleanbox ( p , s ) 
  halfword p ;
  smallnumber s ;
#endif
{
  /* 40 */ register halfword Result; cleanbox_regmem 
  halfword q  ;
  smallnumber savestyle  ;
  halfword x  ;
  halfword r  ;
  switch ( mem [p ].hh .v.RH ) 
  {case 1 : 
    {
      curmlist = newnoad () ;
      mem [curmlist + 1 ]= mem [p ];
    } 
    break ;
  case 2 : 
    {
      q = mem [p ].hh .v.LH ;
      goto lab40 ;
    } 
    break ;
  case 3 : 
    curmlist = mem [p ].hh .v.LH ;
    break ;
    default: 
    {
      q = newnullbox () ;
      goto lab40 ;
    } 
    break ;
  } 
  savestyle = curstyle ;
  curstyle = s ;
  mlistpenalties = false ;
  mlisttohlist () ;
  q = mem [memtop - 3 ].hh .v.RH ;
  curstyle = savestyle ;
  {
    if ( curstyle < 4 ) 
    cursize = 0 ;
    else cursize = 16 * ( ( curstyle - 2 ) / 2 ) ;
    curmu = xovern ( fontinfo [6 + parambase [eqtb [13581 + cursize ].hh 
    .v.RH ]].cint , 18 ) ;
  } 
  lab40: if ( ( q >= himemmin ) || ( q == 0 ) ) 
  x = hpack ( q , 0 , 1 ) ;
  else if ( ( mem [q ].hh .v.RH == 0 ) && ( mem [q ].hh.b0 <= 1 ) && ( mem 
  [q + 4 ].cint == 0 ) ) 
  x = q ;
  else x = hpack ( q , 0 , 1 ) ;
  q = mem [x + 5 ].hh .v.RH ;
  if ( ( q >= himemmin ) ) 
  {
    r = mem [q ].hh .v.RH ;
    if ( r != 0 ) 
    if ( mem [r ].hh .v.RH == 0 ) 
    if ( ! ( r >= himemmin ) ) 
    if ( mem [r ].hh.b0 == 11 ) 
    {
      freenode ( r , 2 ) ;
      mem [q ].hh .v.RH = 0 ;
    } 
  } 
  Result = x ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zfetch ( halfword a ) 
#else
zfetch ( a ) 
  halfword a ;
#endif
{
  fetch_regmem 
  curc = mem [a ].hh.b1 ;
  curf = eqtb [13579 + mem [a ].hh.b0 + cursize ].hh .v.RH ;
  if ( curf == 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 335 ) ;
    } 
    printsize ( cursize ) ;
    printchar ( 32 ) ;
    printint ( mem [a ].hh.b0 ) ;
    print ( 879 ) ;
    print ( curc ) ;
    printchar ( 41 ) ;
    {
      helpptr = 4 ;
      helpline [3 ]= 880 ;
      helpline [2 ]= 881 ;
      helpline [1 ]= 882 ;
      helpline [0 ]= 883 ;
    } 
    error () ;
    curi = nullcharacter ;
    mem [a ].hh .v.RH = 0 ;
  } 
  else {
      
    if ( ( curc >= fontbc [curf ]) && ( curc <= fontec [curf ]) ) 
    curi = fontinfo [charbase [curf ]+ curc ].qqqq ;
    else curi = nullcharacter ;
    if ( ! ( ( curi .b0 > 0 ) ) ) 
    {
      charwarning ( curf , curc ) ;
      mem [a ].hh .v.RH = 0 ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zmakeover ( halfword q ) 
#else
zmakeover ( q ) 
  halfword q ;
#endif
{
  makeover_regmem 
  mem [q + 1 ].hh .v.LH = overbar ( cleanbox ( q + 1 , 2 * ( curstyle / 2 ) 
  + 1 ) , 3 * fontinfo [8 + parambase [eqtb [13582 + cursize ].hh .v.RH ]
  ].cint , fontinfo [8 + parambase [eqtb [13582 + cursize ].hh .v.RH ]]
  .cint ) ;
  mem [q + 1 ].hh .v.RH = 2 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zmakeunder ( halfword q ) 
#else
zmakeunder ( q ) 
  halfword q ;
#endif
{
  makeunder_regmem 
  halfword p, x, y  ;
  scaled delta  ;
  x = cleanbox ( q + 1 , curstyle ) ;
  p = newkern ( 3 * fontinfo [8 + parambase [eqtb [13582 + cursize ].hh 
  .v.RH ]].cint ) ;
  mem [x ].hh .v.RH = p ;
  mem [p ].hh .v.RH = fractionrule ( fontinfo [8 + parambase [eqtb [13582 
  + cursize ].hh .v.RH ]].cint ) ;
  y = vpackage ( x , 0 , 1 , 1073741823L ) ;
  delta = mem [y + 3 ].cint + mem [y + 2 ].cint + fontinfo [8 + parambase 
  [eqtb [13582 + cursize ].hh .v.RH ]].cint ;
  mem [y + 3 ].cint = mem [x + 3 ].cint ;
  mem [y + 2 ].cint = delta - mem [y + 3 ].cint ;
  mem [q + 1 ].hh .v.LH = y ;
  mem [q + 1 ].hh .v.RH = 2 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zmakevcenter ( halfword q ) 
#else
zmakevcenter ( q ) 
  halfword q ;
#endif
{
  makevcenter_regmem 
  halfword v  ;
  scaled delta  ;
  v = mem [q + 1 ].hh .v.LH ;
  if ( mem [v ].hh.b0 != 1 ) 
  confusion ( 539 ) ;
  delta = mem [v + 3 ].cint + mem [v + 2 ].cint ;
  mem [v + 3 ].cint = fontinfo [22 + parambase [eqtb [13581 + cursize ]
  .hh .v.RH ]].cint + half ( delta ) ;
  mem [v + 2 ].cint = delta - mem [v + 3 ].cint ;
} 
void 
#ifdef HAVE_PROTOTYPES
zmakeradical ( halfword q ) 
#else
zmakeradical ( q ) 
  halfword q ;
#endif
{
  makeradical_regmem 
  halfword x, y  ;
  scaled delta, clr  ;
  x = cleanbox ( q + 1 , 2 * ( curstyle / 2 ) + 1 ) ;
  if ( curstyle < 2 ) 
  clr = fontinfo [8 + parambase [eqtb [13582 + cursize ].hh .v.RH ]]
  .cint + ( abs ( fontinfo [5 + parambase [eqtb [13581 + cursize ].hh 
  .v.RH ]].cint ) / 4 ) ;
  else {
      
    clr = fontinfo [8 + parambase [eqtb [13582 + cursize ].hh .v.RH ]]
    .cint ;
    clr = clr + ( abs ( clr ) / 4 ) ;
  } 
  y = vardelimiter ( q + 4 , cursize , mem [x + 3 ].cint + mem [x + 2 ]
  .cint + clr + fontinfo [8 + parambase [eqtb [13582 + cursize ].hh .v.RH 
  ]].cint ) ;
  delta = mem [y + 2 ].cint - ( mem [x + 3 ].cint + mem [x + 2 ].cint + 
  clr ) ;
  if ( delta > 0 ) 
  clr = clr + half ( delta ) ;
  mem [y + 4 ].cint = - (integer) ( mem [x + 3 ].cint + clr ) ;
  mem [y ].hh .v.RH = overbar ( x , clr , mem [y + 3 ].cint ) ;
  mem [q + 1 ].hh .v.LH = hpack ( y , 0 , 1 ) ;
  mem [q + 1 ].hh .v.RH = 2 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zmakemathaccent ( halfword q ) 
#else
zmakemathaccent ( q ) 
  halfword q ;
#endif
{
  /* 30 31 */ makemathaccent_regmem 
  halfword p, x, y  ;
  integer a  ;
  quarterword c  ;
  internalfontnumber f  ;
  fourquarters i  ;
  scaled s  ;
  scaled h  ;
  scaled delta  ;
  scaled w  ;
  fetch ( q + 4 ) ;
  if ( ( curi .b0 > 0 ) ) 
  {
    i = curi ;
    c = curc ;
    f = curf ;
    s = 0 ;
    if ( mem [q + 1 ].hh .v.RH == 1 ) 
    {
      fetch ( q + 1 ) ;
      if ( ( ( curi .b2 ) % 4 ) == 1 ) 
      {
	a = ligkernbase [curf ]+ curi .b3 ;
	curi = fontinfo [a ].qqqq ;
	if ( curi .b0 > 128 ) 
	{
	  a = ligkernbase [curf ]+ 256 * curi .b2 + curi .b3 + 32768L - 256 
	  * ( 128 ) ;
	  curi = fontinfo [a ].qqqq ;
	} 
	while ( true ) {
	    
	  if ( curi .b1 == skewchar [curf ]) 
	  {
	    if ( curi .b2 >= 128 ) 
	    if ( curi .b0 <= 128 ) 
	    s = fontinfo [kernbase [curf ]+ 256 * curi .b2 + curi .b3 ]
	    .cint ;
	    goto lab31 ;
	  } 
	  if ( curi .b0 >= 128 ) 
	  goto lab31 ;
	  a = a + curi .b0 + 1 ;
	  curi = fontinfo [a ].qqqq ;
	} 
      } 
    } 
    lab31: ;
    x = cleanbox ( q + 1 , 2 * ( curstyle / 2 ) + 1 ) ;
    w = mem [x + 1 ].cint ;
    h = mem [x + 3 ].cint ;
    while ( true ) {
	
      if ( ( ( i .b2 ) % 4 ) != 2 ) 
      goto lab30 ;
      y = i .b3 ;
      i = fontinfo [charbase [f ]+ y ].qqqq ;
      if ( ! ( i .b0 > 0 ) ) 
      goto lab30 ;
      if ( fontinfo [widthbase [f ]+ i .b0 ].cint > w ) 
      goto lab30 ;
      c = y ;
    } 
    lab30: ;
    if ( h < fontinfo [5 + parambase [f ]].cint ) 
    delta = h ;
    else delta = fontinfo [5 + parambase [f ]].cint ;
    if ( ( mem [q + 2 ].hh .v.RH != 0 ) || ( mem [q + 3 ].hh .v.RH != 0 ) 
    ) 
    if ( mem [q + 1 ].hh .v.RH == 1 ) 
    {
      flushnodelist ( x ) ;
      x = newnoad () ;
      mem [x + 1 ]= mem [q + 1 ];
      mem [x + 2 ]= mem [q + 2 ];
      mem [x + 3 ]= mem [q + 3 ];
      mem [q + 2 ].hh = emptyfield ;
      mem [q + 3 ].hh = emptyfield ;
      mem [q + 1 ].hh .v.RH = 3 ;
      mem [q + 1 ].hh .v.LH = x ;
      x = cleanbox ( q + 1 , curstyle ) ;
      delta = delta + mem [x + 3 ].cint - h ;
      h = mem [x + 3 ].cint ;
    } 
    y = charbox ( f , c ) ;
    mem [y + 4 ].cint = s + half ( w - mem [y + 1 ].cint ) ;
    mem [y + 1 ].cint = 0 ;
    p = newkern ( - (integer) delta ) ;
    mem [p ].hh .v.RH = x ;
    mem [y ].hh .v.RH = p ;
    y = vpackage ( y , 0 , 1 , 1073741823L ) ;
    mem [y + 1 ].cint = mem [x + 1 ].cint ;
    if ( mem [y + 3 ].cint < h ) 
    {
      p = newkern ( h - mem [y + 3 ].cint ) ;
      mem [p ].hh .v.RH = mem [y + 5 ].hh .v.RH ;
      mem [y + 5 ].hh .v.RH = p ;
      mem [y + 3 ].cint = h ;
    } 
    mem [q + 1 ].hh .v.LH = y ;
    mem [q + 1 ].hh .v.RH = 2 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zmakefraction ( halfword q ) 
#else
zmakefraction ( q ) 
  halfword q ;
#endif
{
  makefraction_regmem 
  halfword p, v, x, y, z  ;
  scaled delta, delta1, delta2, shiftup, shiftdown, clr  ;
  if ( mem [q + 1 ].cint == 1073741824L ) 
  mem [q + 1 ].cint = fontinfo [8 + parambase [eqtb [13582 + cursize ]
  .hh .v.RH ]].cint ;
  x = cleanbox ( q + 2 , curstyle + 2 - 2 * ( curstyle / 6 ) ) ;
  z = cleanbox ( q + 3 , 2 * ( curstyle / 2 ) + 3 - 2 * ( curstyle / 6 ) ) ;
  if ( mem [x + 1 ].cint < mem [z + 1 ].cint ) 
  x = rebox ( x , mem [z + 1 ].cint ) ;
  else z = rebox ( z , mem [x + 1 ].cint ) ;
  if ( curstyle < 2 ) 
  {
    shiftup = fontinfo [8 + parambase [eqtb [13581 + cursize ].hh .v.RH ]
    ].cint ;
    shiftdown = fontinfo [11 + parambase [eqtb [13581 + cursize ].hh .v.RH 
    ]].cint ;
  } 
  else {
      
    shiftdown = fontinfo [12 + parambase [eqtb [13581 + cursize ].hh .v.RH 
    ]].cint ;
    if ( mem [q + 1 ].cint != 0 ) 
    shiftup = fontinfo [9 + parambase [eqtb [13581 + cursize ].hh .v.RH ]
    ].cint ;
    else shiftup = fontinfo [10 + parambase [eqtb [13581 + cursize ].hh 
    .v.RH ]].cint ;
  } 
  if ( mem [q + 1 ].cint == 0 ) 
  {
    if ( curstyle < 2 ) 
    clr = 7 * fontinfo [8 + parambase [eqtb [13582 + cursize ].hh .v.RH ]
    ].cint ;
    else clr = 3 * fontinfo [8 + parambase [eqtb [13582 + cursize ].hh 
    .v.RH ]].cint ;
    delta = half ( clr - ( ( shiftup - mem [x + 2 ].cint ) - ( mem [z + 3 ]
    .cint - shiftdown ) ) ) ;
    if ( delta > 0 ) 
    {
      shiftup = shiftup + delta ;
      shiftdown = shiftdown + delta ;
    } 
  } 
  else {
      
    if ( curstyle < 2 ) 
    clr = 3 * mem [q + 1 ].cint ;
    else clr = mem [q + 1 ].cint ;
    delta = half ( mem [q + 1 ].cint ) ;
    delta1 = clr - ( ( shiftup - mem [x + 2 ].cint ) - ( fontinfo [22 + 
    parambase [eqtb [13581 + cursize ].hh .v.RH ]].cint + delta ) ) ;
    delta2 = clr - ( ( fontinfo [22 + parambase [eqtb [13581 + cursize ]
    .hh .v.RH ]].cint - delta ) - ( mem [z + 3 ].cint - shiftdown ) ) ;
    if ( delta1 > 0 ) 
    shiftup = shiftup + delta1 ;
    if ( delta2 > 0 ) 
    shiftdown = shiftdown + delta2 ;
  } 
  v = newnullbox () ;
  mem [v ].hh.b0 = 1 ;
  mem [v + 3 ].cint = shiftup + mem [x + 3 ].cint ;
  mem [v + 2 ].cint = mem [z + 2 ].cint + shiftdown ;
  mem [v + 1 ].cint = mem [x + 1 ].cint ;
  if ( mem [q + 1 ].cint == 0 ) 
  {
    p = newkern ( ( shiftup - mem [x + 2 ].cint ) - ( mem [z + 3 ].cint - 
    shiftdown ) ) ;
    mem [p ].hh .v.RH = z ;
  } 
  else {
      
    y = fractionrule ( mem [q + 1 ].cint ) ;
    p = newkern ( ( fontinfo [22 + parambase [eqtb [13581 + cursize ].hh 
    .v.RH ]].cint - delta ) - ( mem [z + 3 ].cint - shiftdown ) ) ;
    mem [y ].hh .v.RH = p ;
    mem [p ].hh .v.RH = z ;
    p = newkern ( ( shiftup - mem [x + 2 ].cint ) - ( fontinfo [22 + 
    parambase [eqtb [13581 + cursize ].hh .v.RH ]].cint + delta ) ) ;
    mem [p ].hh .v.RH = y ;
  } 
  mem [x ].hh .v.RH = p ;
  mem [v + 5 ].hh .v.RH = x ;
  if ( curstyle < 2 ) 
  delta = fontinfo [20 + parambase [eqtb [13581 + cursize ].hh .v.RH ]]
  .cint ;
  else delta = fontinfo [21 + parambase [eqtb [13581 + cursize ].hh .v.RH 
  ]].cint ;
  x = vardelimiter ( q + 4 , cursize , delta ) ;
  mem [x ].hh .v.RH = v ;
  z = vardelimiter ( q + 5 , cursize , delta ) ;
  mem [v ].hh .v.RH = z ;
  mem [q + 1 ].cint = hpack ( x , 0 , 1 ) ;
} 
scaled 
#ifdef HAVE_PROTOTYPES
zmakeop ( halfword q ) 
#else
zmakeop ( q ) 
  halfword q ;
#endif
{
  register scaled Result; makeop_regmem 
  scaled delta  ;
  halfword p, v, x, y, z  ;
  quarterword c  ;
  fourquarters i  ;
  scaled shiftup, shiftdown  ;
  if ( ( mem [q ].hh.b1 == 0 ) && ( curstyle < 2 ) ) 
  mem [q ].hh.b1 = 1 ;
  if ( mem [q + 1 ].hh .v.RH == 1 ) 
  {
    fetch ( q + 1 ) ;
    if ( ( curstyle < 2 ) && ( ( ( curi .b2 ) % 4 ) == 2 ) ) 
    {
      c = curi .b3 ;
      i = fontinfo [charbase [curf ]+ c ].qqqq ;
      if ( ( i .b0 > 0 ) ) 
      {
	curc = c ;
	curi = i ;
	mem [q + 1 ].hh.b1 = c ;
      } 
    } 
    delta = fontinfo [italicbase [curf ]+ ( curi .b2 ) / 4 ].cint ;
    x = cleanbox ( q + 1 , curstyle ) ;
    if ( ( mem [q + 3 ].hh .v.RH != 0 ) && ( mem [q ].hh.b1 != 1 ) ) 
    mem [x + 1 ].cint = mem [x + 1 ].cint - delta ;
    mem [x + 4 ].cint = half ( mem [x + 3 ].cint - mem [x + 2 ].cint ) - 
    fontinfo [22 + parambase [eqtb [13581 + cursize ].hh .v.RH ]].cint ;
    mem [q + 1 ].hh .v.RH = 2 ;
    mem [q + 1 ].hh .v.LH = x ;
  } 
  else delta = 0 ;
  if ( mem [q ].hh.b1 == 1 ) 
  {
    x = cleanbox ( q + 2 , 2 * ( curstyle / 4 ) + 4 + ( curstyle % 2 ) ) ;
    y = cleanbox ( q + 1 , curstyle ) ;
    z = cleanbox ( q + 3 , 2 * ( curstyle / 4 ) + 5 ) ;
    v = newnullbox () ;
    mem [v ].hh.b0 = 1 ;
    mem [v + 1 ].cint = mem [y + 1 ].cint ;
    if ( mem [x + 1 ].cint > mem [v + 1 ].cint ) 
    mem [v + 1 ].cint = mem [x + 1 ].cint ;
    if ( mem [z + 1 ].cint > mem [v + 1 ].cint ) 
    mem [v + 1 ].cint = mem [z + 1 ].cint ;
    x = rebox ( x , mem [v + 1 ].cint ) ;
    y = rebox ( y , mem [v + 1 ].cint ) ;
    z = rebox ( z , mem [v + 1 ].cint ) ;
    mem [x + 4 ].cint = half ( delta ) ;
    mem [z + 4 ].cint = - (integer) mem [x + 4 ].cint ;
    mem [v + 3 ].cint = mem [y + 3 ].cint ;
    mem [v + 2 ].cint = mem [y + 2 ].cint ;
    if ( mem [q + 2 ].hh .v.RH == 0 ) 
    {
      freenode ( x , 7 ) ;
      mem [v + 5 ].hh .v.RH = y ;
    } 
    else {
	
      shiftup = fontinfo [11 + parambase [eqtb [13582 + cursize ].hh .v.RH 
      ]].cint - mem [x + 2 ].cint ;
      if ( shiftup < fontinfo [9 + parambase [eqtb [13582 + cursize ].hh 
      .v.RH ]].cint ) 
      shiftup = fontinfo [9 + parambase [eqtb [13582 + cursize ].hh .v.RH 
      ]].cint ;
      p = newkern ( shiftup ) ;
      mem [p ].hh .v.RH = y ;
      mem [x ].hh .v.RH = p ;
      p = newkern ( fontinfo [13 + parambase [eqtb [13582 + cursize ].hh 
      .v.RH ]].cint ) ;
      mem [p ].hh .v.RH = x ;
      mem [v + 5 ].hh .v.RH = p ;
      mem [v + 3 ].cint = mem [v + 3 ].cint + fontinfo [13 + parambase [
      eqtb [13582 + cursize ].hh .v.RH ]].cint + mem [x + 3 ].cint + mem 
      [x + 2 ].cint + shiftup ;
    } 
    if ( mem [q + 3 ].hh .v.RH == 0 ) 
    freenode ( z , 7 ) ;
    else {
	
      shiftdown = fontinfo [12 + parambase [eqtb [13582 + cursize ].hh 
      .v.RH ]].cint - mem [z + 3 ].cint ;
      if ( shiftdown < fontinfo [10 + parambase [eqtb [13582 + cursize ]
      .hh .v.RH ]].cint ) 
      shiftdown = fontinfo [10 + parambase [eqtb [13582 + cursize ].hh 
      .v.RH ]].cint ;
      p = newkern ( shiftdown ) ;
      mem [y ].hh .v.RH = p ;
      mem [p ].hh .v.RH = z ;
      p = newkern ( fontinfo [13 + parambase [eqtb [13582 + cursize ].hh 
      .v.RH ]].cint ) ;
      mem [z ].hh .v.RH = p ;
      mem [v + 2 ].cint = mem [v + 2 ].cint + fontinfo [13 + parambase [
      eqtb [13582 + cursize ].hh .v.RH ]].cint + mem [z + 3 ].cint + mem 
      [z + 2 ].cint + shiftdown ;
    } 
    mem [q + 1 ].cint = v ;
  } 
  Result = delta ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zmakeord ( halfword q ) 
#else
zmakeord ( q ) 
  halfword q ;
#endif
{
  /* 20 10 */ makeord_regmem 
  integer a  ;
  halfword p, r  ;
  lab20: if ( mem [q + 3 ].hh .v.RH == 0 ) 
  if ( mem [q + 2 ].hh .v.RH == 0 ) 
  if ( mem [q + 1 ].hh .v.RH == 1 ) 
  {
    p = mem [q ].hh .v.RH ;
    if ( p != 0 ) 
    if ( ( mem [p ].hh.b0 >= 16 ) && ( mem [p ].hh.b0 <= 22 ) ) 
    if ( mem [p + 1 ].hh .v.RH == 1 ) 
    if ( mem [p + 1 ].hh.b0 == mem [q + 1 ].hh.b0 ) 
    {
      mem [q + 1 ].hh .v.RH = 4 ;
      fetch ( q + 1 ) ;
      if ( ( ( curi .b2 ) % 4 ) == 1 ) 
      {
	a = ligkernbase [curf ]+ curi .b3 ;
	curc = mem [p + 1 ].hh.b1 ;
	curi = fontinfo [a ].qqqq ;
	if ( curi .b0 > 128 ) 
	{
	  a = ligkernbase [curf ]+ 256 * curi .b2 + curi .b3 + 32768L - 256 
	  * ( 128 ) ;
	  curi = fontinfo [a ].qqqq ;
	} 
	while ( true ) {
	    
	  if ( curi .b1 == curc ) 
	  if ( curi .b0 <= 128 ) 
	  if ( curi .b2 >= 128 ) 
	  {
	    p = newkern ( fontinfo [kernbase [curf ]+ 256 * curi .b2 + curi 
	    .b3 ].cint ) ;
	    mem [p ].hh .v.RH = mem [q ].hh .v.RH ;
	    mem [q ].hh .v.RH = p ;
	    return ;
	  } 
	  else {
	      
	    {
	      if ( interrupt != 0 ) 
	      pauseforinstructions () ;
	    } 
	    switch ( curi .b2 ) 
	    {case 1 : 
	    case 5 : 
	      mem [q + 1 ].hh.b1 = curi .b3 ;
	      break ;
	    case 2 : 
	    case 6 : 
	      mem [p + 1 ].hh.b1 = curi .b3 ;
	      break ;
	    case 3 : 
	    case 7 : 
	    case 11 : 
	      {
		r = newnoad () ;
		mem [r + 1 ].hh.b1 = curi .b3 ;
		mem [r + 1 ].hh.b0 = mem [q + 1 ].hh.b0 ;
		mem [q ].hh .v.RH = r ;
		mem [r ].hh .v.RH = p ;
		if ( curi .b2 < 11 ) 
		mem [r + 1 ].hh .v.RH = 1 ;
		else mem [r + 1 ].hh .v.RH = 4 ;
	      } 
	      break ;
	      default: 
	      {
		mem [q ].hh .v.RH = mem [p ].hh .v.RH ;
		mem [q + 1 ].hh.b1 = curi .b3 ;
		mem [q + 3 ]= mem [p + 3 ];
		mem [q + 2 ]= mem [p + 2 ];
		freenode ( p , 4 ) ;
	      } 
	      break ;
	    } 
	    if ( curi .b2 > 3 ) 
	    return ;
	    mem [q + 1 ].hh .v.RH = 1 ;
	    goto lab20 ;
	  } 
	  if ( curi .b0 >= 128 ) 
	  return ;
	  a = a + curi .b0 + 1 ;
	  curi = fontinfo [a ].qqqq ;
	} 
      } 
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zmakescripts ( halfword q , scaled delta ) 
#else
zmakescripts ( q , delta ) 
  halfword q ;
  scaled delta ;
#endif
{
  makescripts_regmem 
  halfword p, x, y, z  ;
  scaled shiftup, shiftdown, clr  ;
  smallnumber t  ;
  p = mem [q + 1 ].cint ;
  if ( ( p >= himemmin ) ) 
  {
    shiftup = 0 ;
    shiftdown = 0 ;
  } 
  else {
      
    z = hpack ( p , 0 , 1 ) ;
    if ( curstyle < 4 ) 
    t = 16 ;
    else t = 32 ;
    shiftup = mem [z + 3 ].cint - fontinfo [18 + parambase [eqtb [13581 + 
    t ].hh .v.RH ]].cint ;
    shiftdown = mem [z + 2 ].cint + fontinfo [19 + parambase [eqtb [13581 
    + t ].hh .v.RH ]].cint ;
    freenode ( z , 7 ) ;
  } 
  if ( mem [q + 2 ].hh .v.RH == 0 ) 
  {
    x = cleanbox ( q + 3 , 2 * ( curstyle / 4 ) + 5 ) ;
    mem [x + 1 ].cint = mem [x + 1 ].cint + eqtb [15745 ].cint ;
    if ( shiftdown < fontinfo [16 + parambase [eqtb [13581 + cursize ].hh 
    .v.RH ]].cint ) 
    shiftdown = fontinfo [16 + parambase [eqtb [13581 + cursize ].hh .v.RH 
    ]].cint ;
    clr = mem [x + 3 ].cint - ( abs ( fontinfo [5 + parambase [eqtb [
    13581 + cursize ].hh .v.RH ]].cint * 4 ) / 5 ) ;
    if ( shiftdown < clr ) 
    shiftdown = clr ;
    mem [x + 4 ].cint = shiftdown ;
  } 
  else {
      
    {
      x = cleanbox ( q + 2 , 2 * ( curstyle / 4 ) + 4 + ( curstyle % 2 ) ) ;
      mem [x + 1 ].cint = mem [x + 1 ].cint + eqtb [15745 ].cint ;
      if ( odd ( curstyle ) ) 
      clr = fontinfo [15 + parambase [eqtb [13581 + cursize ].hh .v.RH ]]
      .cint ;
      else if ( curstyle < 2 ) 
      clr = fontinfo [13 + parambase [eqtb [13581 + cursize ].hh .v.RH ]]
      .cint ;
      else clr = fontinfo [14 + parambase [eqtb [13581 + cursize ].hh 
      .v.RH ]].cint ;
      if ( shiftup < clr ) 
      shiftup = clr ;
      clr = mem [x + 2 ].cint + ( abs ( fontinfo [5 + parambase [eqtb [
      13581 + cursize ].hh .v.RH ]].cint ) / 4 ) ;
      if ( shiftup < clr ) 
      shiftup = clr ;
    } 
    if ( mem [q + 3 ].hh .v.RH == 0 ) 
    mem [x + 4 ].cint = - (integer) shiftup ;
    else {
	
      y = cleanbox ( q + 3 , 2 * ( curstyle / 4 ) + 5 ) ;
      mem [y + 1 ].cint = mem [y + 1 ].cint + eqtb [15745 ].cint ;
      if ( shiftdown < fontinfo [17 + parambase [eqtb [13581 + cursize ]
      .hh .v.RH ]].cint ) 
      shiftdown = fontinfo [17 + parambase [eqtb [13581 + cursize ].hh 
      .v.RH ]].cint ;
      clr = 4 * fontinfo [8 + parambase [eqtb [13582 + cursize ].hh .v.RH 
      ]].cint - ( ( shiftup - mem [x + 2 ].cint ) - ( mem [y + 3 ].cint 
      - shiftdown ) ) ;
      if ( clr > 0 ) 
      {
	shiftdown = shiftdown + clr ;
	clr = ( abs ( fontinfo [5 + parambase [eqtb [13581 + cursize ].hh 
	.v.RH ]].cint * 4 ) / 5 ) - ( shiftup - mem [x + 2 ].cint ) ;
	if ( clr > 0 ) 
	{
	  shiftup = shiftup + clr ;
	  shiftdown = shiftdown - clr ;
	} 
      } 
      mem [x + 4 ].cint = delta ;
      p = newkern ( ( shiftup - mem [x + 2 ].cint ) - ( mem [y + 3 ].cint 
      - shiftdown ) ) ;
      mem [x ].hh .v.RH = p ;
      mem [p ].hh .v.RH = y ;
      x = vpackage ( x , 0 , 1 , 1073741823L ) ;
      mem [x + 4 ].cint = shiftdown ;
    } 
  } 
  if ( mem [q + 1 ].cint == 0 ) 
  mem [q + 1 ].cint = x ;
  else {
      
    p = mem [q + 1 ].cint ;
    while ( mem [p ].hh .v.RH != 0 ) p = mem [p ].hh .v.RH ;
    mem [p ].hh .v.RH = x ;
  } 
} 
smallnumber 
#ifdef HAVE_PROTOTYPES
zmakeleftright ( halfword q , smallnumber style , scaled maxd , scaled maxh ) 
#else
zmakeleftright ( q , style , maxd , maxh ) 
  halfword q ;
  smallnumber style ;
  scaled maxd ;
  scaled maxh ;
#endif
{
  register smallnumber Result; makeleftright_regmem 
  scaled delta, delta1, delta2  ;
  if ( style < 4 ) 
  cursize = 0 ;
  else cursize = 16 * ( ( style - 2 ) / 2 ) ;
  delta2 = maxd + fontinfo [22 + parambase [eqtb [13581 + cursize ].hh 
  .v.RH ]].cint ;
  delta1 = maxh + maxd - delta2 ;
  if ( delta2 > delta1 ) 
  delta1 = delta2 ;
  delta = ( delta1 / 500 ) * eqtb [15181 ].cint ;
  delta2 = delta1 + delta1 - eqtb [15743 ].cint ;
  if ( delta < delta2 ) 
  delta = delta2 ;
  mem [q + 1 ].cint = vardelimiter ( q + 1 , cursize , delta ) ;
  Result = mem [q ].hh.b0 - ( 10 ) ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
mlisttohlist ( void ) 
#else
mlisttohlist ( ) 
#endif
{
  /* 21 82 80 81 83 30 */ mlisttohlist_regmem 
  halfword mlist  ;
  boolean penalties  ;
  smallnumber style  ;
  smallnumber savestyle  ;
  halfword q  ;
  halfword r  ;
  smallnumber rtype  ;
  smallnumber t  ;
  halfword p, x, y, z  ;
  integer pen  ;
  smallnumber s  ;
  scaled maxh, maxd  ;
  scaled delta  ;
  mlist = curmlist ;
  penalties = mlistpenalties ;
  style = curstyle ;
  q = mlist ;
  r = 0 ;
  rtype = 17 ;
  maxh = 0 ;
  maxd = 0 ;
  {
    if ( curstyle < 4 ) 
    cursize = 0 ;
    else cursize = 16 * ( ( curstyle - 2 ) / 2 ) ;
    curmu = xovern ( fontinfo [6 + parambase [eqtb [13581 + cursize ].hh 
    .v.RH ]].cint , 18 ) ;
  } 
  while ( q != 0 ) {
      
    lab21: delta = 0 ;
    switch ( mem [q ].hh.b0 ) 
    {case 18 : 
      switch ( rtype ) 
      {case 18 : 
      case 17 : 
      case 19 : 
      case 20 : 
      case 22 : 
      case 30 : 
	{
	  mem [q ].hh.b0 = 16 ;
	  goto lab21 ;
	} 
	break ;
	default: 
	;
	break ;
      } 
      break ;
    case 19 : 
    case 21 : 
    case 22 : 
    case 31 : 
      {
	if ( rtype == 18 ) 
	mem [r ].hh.b0 = 16 ;
	if ( mem [q ].hh.b0 == 31 ) 
	goto lab80 ;
      } 
      break ;
    case 30 : 
      goto lab80 ;
      break ;
    case 25 : 
      {
	makefraction ( q ) ;
	goto lab82 ;
      } 
      break ;
    case 17 : 
      {
	delta = makeop ( q ) ;
	if ( mem [q ].hh.b1 == 1 ) 
	goto lab82 ;
      } 
      break ;
    case 16 : 
      makeord ( q ) ;
      break ;
    case 20 : 
    case 23 : 
      ;
      break ;
    case 24 : 
      makeradical ( q ) ;
      break ;
    case 27 : 
      makeover ( q ) ;
      break ;
    case 26 : 
      makeunder ( q ) ;
      break ;
    case 28 : 
      makemathaccent ( q ) ;
      break ;
    case 29 : 
      makevcenter ( q ) ;
      break ;
    case 14 : 
      {
	curstyle = mem [q ].hh.b1 ;
	{
	  if ( curstyle < 4 ) 
	  cursize = 0 ;
	  else cursize = 16 * ( ( curstyle - 2 ) / 2 ) ;
	  curmu = xovern ( fontinfo [6 + parambase [eqtb [13581 + cursize ]
	  .hh .v.RH ]].cint , 18 ) ;
	} 
	goto lab81 ;
      } 
      break ;
    case 15 : 
      {
	switch ( curstyle / 2 ) 
	{case 0 : 
	  {
	    p = mem [q + 1 ].hh .v.LH ;
	    mem [q + 1 ].hh .v.LH = 0 ;
	  } 
	  break ;
	case 1 : 
	  {
	    p = mem [q + 1 ].hh .v.RH ;
	    mem [q + 1 ].hh .v.RH = 0 ;
	  } 
	  break ;
	case 2 : 
	  {
	    p = mem [q + 2 ].hh .v.LH ;
	    mem [q + 2 ].hh .v.LH = 0 ;
	  } 
	  break ;
	case 3 : 
	  {
	    p = mem [q + 2 ].hh .v.RH ;
	    mem [q + 2 ].hh .v.RH = 0 ;
	  } 
	  break ;
	} 
	flushnodelist ( mem [q + 1 ].hh .v.LH ) ;
	flushnodelist ( mem [q + 1 ].hh .v.RH ) ;
	flushnodelist ( mem [q + 2 ].hh .v.LH ) ;
	flushnodelist ( mem [q + 2 ].hh .v.RH ) ;
	mem [q ].hh.b0 = 14 ;
	mem [q ].hh.b1 = curstyle ;
	mem [q + 1 ].cint = 0 ;
	mem [q + 2 ].cint = 0 ;
	if ( p != 0 ) 
	{
	  z = mem [q ].hh .v.RH ;
	  mem [q ].hh .v.RH = p ;
	  while ( mem [p ].hh .v.RH != 0 ) p = mem [p ].hh .v.RH ;
	  mem [p ].hh .v.RH = z ;
	} 
	goto lab81 ;
      } 
      break ;
    case 3 : 
    case 4 : 
    case 5 : 
    case 8 : 
    case 12 : 
    case 7 : 
      goto lab81 ;
      break ;
    case 2 : 
      {
	if ( mem [q + 3 ].cint > maxh ) 
	maxh = mem [q + 3 ].cint ;
	if ( mem [q + 2 ].cint > maxd ) 
	maxd = mem [q + 2 ].cint ;
	goto lab81 ;
      } 
      break ;
    case 10 : 
      {
	if ( mem [q ].hh.b1 == 99 ) 
	{
	  x = mem [q + 1 ].hh .v.LH ;
	  y = mathglue ( x , curmu ) ;
	  deleteglueref ( x ) ;
	  mem [q + 1 ].hh .v.LH = y ;
	  mem [q ].hh.b1 = 0 ;
	} 
	else if ( ( cursize != 0 ) && ( mem [q ].hh.b1 == 98 ) ) 
	{
	  p = mem [q ].hh .v.RH ;
	  if ( p != 0 ) 
	  if ( ( mem [p ].hh.b0 == 10 ) || ( mem [p ].hh.b0 == 11 ) ) 
	  {
	    mem [q ].hh .v.RH = mem [p ].hh .v.RH ;
	    mem [p ].hh .v.RH = 0 ;
	    flushnodelist ( p ) ;
	  } 
	} 
	goto lab81 ;
      } 
      break ;
    case 11 : 
      {
	mathkern ( q , curmu ) ;
	goto lab81 ;
      } 
      break ;
      default: 
      confusion ( 884 ) ;
      break ;
    } 
    switch ( mem [q + 1 ].hh .v.RH ) 
    {case 1 : 
    case 4 : 
      {
	fetch ( q + 1 ) ;
	if ( ( curi .b0 > 0 ) ) 
	{
	  delta = fontinfo [italicbase [curf ]+ ( curi .b2 ) / 4 ].cint ;
	  p = newcharacter ( curf , curc ) ;
	  if ( ( mem [q + 1 ].hh .v.RH == 4 ) && ( fontinfo [2 + parambase 
	  [curf ]].cint != 0 ) ) 
	  delta = 0 ;
	  if ( ( mem [q + 3 ].hh .v.RH == 0 ) && ( delta != 0 ) ) 
	  {
	    mem [p ].hh .v.RH = newkern ( delta ) ;
	    delta = 0 ;
	  } 
	} 
	else p = 0 ;
      } 
      break ;
    case 0 : 
      p = 0 ;
      break ;
    case 2 : 
      p = mem [q + 1 ].hh .v.LH ;
      break ;
    case 3 : 
      {
	curmlist = mem [q + 1 ].hh .v.LH ;
	savestyle = curstyle ;
	mlistpenalties = false ;
	mlisttohlist () ;
	curstyle = savestyle ;
	{
	  if ( curstyle < 4 ) 
	  cursize = 0 ;
	  else cursize = 16 * ( ( curstyle - 2 ) / 2 ) ;
	  curmu = xovern ( fontinfo [6 + parambase [eqtb [13581 + cursize ]
	  .hh .v.RH ]].cint , 18 ) ;
	} 
	p = hpack ( mem [memtop - 3 ].hh .v.RH , 0 , 1 ) ;
      } 
      break ;
      default: 
      confusion ( 885 ) ;
      break ;
    } 
    mem [q + 1 ].cint = p ;
    if ( ( mem [q + 3 ].hh .v.RH == 0 ) && ( mem [q + 2 ].hh .v.RH == 0 ) 
    ) 
    goto lab82 ;
    makescripts ( q , delta ) ;
    lab82: z = hpack ( mem [q + 1 ].cint , 0 , 1 ) ;
    if ( mem [z + 3 ].cint > maxh ) 
    maxh = mem [z + 3 ].cint ;
    if ( mem [z + 2 ].cint > maxd ) 
    maxd = mem [z + 2 ].cint ;
    freenode ( z , 7 ) ;
    lab80: r = q ;
    rtype = mem [r ].hh.b0 ;
    lab81: q = mem [q ].hh .v.RH ;
  } 
  if ( rtype == 18 ) 
  mem [r ].hh.b0 = 16 ;
  p = memtop - 3 ;
  mem [p ].hh .v.RH = 0 ;
  q = mlist ;
  rtype = 0 ;
  curstyle = style ;
  {
    if ( curstyle < 4 ) 
    cursize = 0 ;
    else cursize = 16 * ( ( curstyle - 2 ) / 2 ) ;
    curmu = xovern ( fontinfo [6 + parambase [eqtb [13581 + cursize ].hh 
    .v.RH ]].cint , 18 ) ;
  } 
  while ( q != 0 ) {
      
    t = 16 ;
    s = 4 ;
    pen = 10000 ;
    switch ( mem [q ].hh.b0 ) 
    {case 17 : 
    case 20 : 
    case 21 : 
    case 22 : 
    case 23 : 
      t = mem [q ].hh.b0 ;
      break ;
    case 18 : 
      {
	t = 18 ;
	pen = eqtb [15172 ].cint ;
      } 
      break ;
    case 19 : 
      {
	t = 19 ;
	pen = eqtb [15173 ].cint ;
      } 
      break ;
    case 16 : 
    case 29 : 
    case 27 : 
    case 26 : 
      ;
      break ;
    case 24 : 
      s = 5 ;
      break ;
    case 28 : 
      s = 5 ;
      break ;
    case 25 : 
      {
	t = 23 ;
	s = 6 ;
      } 
      break ;
    case 30 : 
    case 31 : 
      t = makeleftright ( q , style , maxd , maxh ) ;
      break ;
    case 14 : 
      {
	curstyle = mem [q ].hh.b1 ;
	s = 3 ;
	{
	  if ( curstyle < 4 ) 
	  cursize = 0 ;
	  else cursize = 16 * ( ( curstyle - 2 ) / 2 ) ;
	  curmu = xovern ( fontinfo [6 + parambase [eqtb [13581 + cursize ]
	  .hh .v.RH ]].cint , 18 ) ;
	} 
	goto lab83 ;
      } 
      break ;
    case 8 : 
    case 12 : 
    case 2 : 
    case 7 : 
    case 5 : 
    case 3 : 
    case 4 : 
    case 10 : 
    case 11 : 
      {
	mem [p ].hh .v.RH = q ;
	p = q ;
	q = mem [q ].hh .v.RH ;
	mem [p ].hh .v.RH = 0 ;
	goto lab30 ;
      } 
      break ;
      default: 
      confusion ( 886 ) ;
      break ;
    } 
    if ( rtype > 0 ) 
    {
      switch ( strpool [rtype * 8 + t + magicoffset ]) 
      {case 48 : 
	x = 0 ;
	break ;
      case 49 : 
	if ( curstyle < 4 ) 
	x = 15 ;
	else x = 0 ;
	break ;
      case 50 : 
	x = 15 ;
	break ;
      case 51 : 
	if ( curstyle < 4 ) 
	x = 16 ;
	else x = 0 ;
	break ;
      case 52 : 
	if ( curstyle < 4 ) 
	x = 17 ;
	else x = 0 ;
	break ;
	default: 
	confusion ( 888 ) ;
	break ;
      } 
      if ( x != 0 ) 
      {
	y = mathglue ( eqtb [12526 + x ].hh .v.RH , curmu ) ;
	z = newglue ( y ) ;
	mem [y ].hh .v.RH = 0 ;
	mem [p ].hh .v.RH = z ;
	p = z ;
	mem [z ].hh.b1 = x + 1 ;
      } 
    } 
    if ( mem [q + 1 ].cint != 0 ) 
    {
      mem [p ].hh .v.RH = mem [q + 1 ].cint ;
      do {
	  p = mem [p ].hh .v.RH ;
      } while ( ! ( mem [p ].hh .v.RH == 0 ) ) ;
    } 
    if ( penalties ) 
    if ( mem [q ].hh .v.RH != 0 ) 
    if ( pen < 10000 ) 
    {
      rtype = mem [mem [q ].hh .v.RH ].hh.b0 ;
      if ( rtype != 12 ) 
      if ( rtype != 19 ) 
      {
	z = newpenalty ( pen ) ;
	mem [p ].hh .v.RH = z ;
	p = z ;
      } 
    } 
    rtype = t ;
    lab83: r = q ;
    q = mem [q ].hh .v.RH ;
    freenode ( r , s ) ;
    lab30: ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
pushalignment ( void ) 
#else
pushalignment ( ) 
#endif
{
  pushalignment_regmem 
  halfword p  ;
  p = getnode ( 5 ) ;
  mem [p ].hh .v.RH = alignptr ;
  mem [p ].hh .v.LH = curalign ;
  mem [p + 1 ].hh .v.LH = mem [memtop - 8 ].hh .v.RH ;
  mem [p + 1 ].hh .v.RH = curspan ;
  mem [p + 2 ].cint = curloop ;
  mem [p + 3 ].cint = alignstate ;
  mem [p + 4 ].hh .v.LH = curhead ;
  mem [p + 4 ].hh .v.RH = curtail ;
  alignptr = p ;
  curhead = getavail () ;
} 
void 
#ifdef HAVE_PROTOTYPES
popalignment ( void ) 
#else
popalignment ( ) 
#endif
{
  popalignment_regmem 
  halfword p  ;
  {
    mem [curhead ].hh .v.RH = avail ;
    avail = curhead ;
	;
#ifdef STAT
    decr ( dynused ) ;
#endif /* STAT */
  } 
  p = alignptr ;
  curtail = mem [p + 4 ].hh .v.RH ;
  curhead = mem [p + 4 ].hh .v.LH ;
  alignstate = mem [p + 3 ].cint ;
  curloop = mem [p + 2 ].cint ;
  curspan = mem [p + 1 ].hh .v.RH ;
  mem [memtop - 8 ].hh .v.RH = mem [p + 1 ].hh .v.LH ;
  curalign = mem [p ].hh .v.LH ;
  alignptr = mem [p ].hh .v.RH ;
  freenode ( p , 5 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
getpreambletoken ( void ) 
#else
getpreambletoken ( ) 
#endif
{
  /* 20 */ getpreambletoken_regmem 
  lab20: gettoken () ;
  while ( ( curchr == 256 ) && ( curcmd == 4 ) ) {
      
    gettoken () ;
    if ( curcmd > 100 ) 
    {
      expand () ;
      gettoken () ;
    } 
  } 
  if ( curcmd == 9 ) 
  fatalerror ( 594 ) ;
  if ( ( curcmd == 75 ) && ( curchr == 12537 ) ) 
  {
    scanoptionalequals () ;
    scanglue ( 2 ) ;
    if ( eqtb [15206 ].cint > 0 ) 
    geqdefine ( 12537 , 117 , curval ) ;
    else eqdefine ( 12537 , 117 , curval ) ;
    goto lab20 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
initalign ( void ) 
#else
initalign ( ) 
#endif
{
  /* 30 31 32 22 */ initalign_regmem 
  halfword savecsptr  ;
  halfword p  ;
  savecsptr = curcs ;
  pushalignment () ;
  alignstate = -1000000L ;
  if ( ( curlist .modefield == 203 ) && ( ( curlist .tailfield != curlist 
  .headfield ) || ( curlist .auxfield .cint != 0 ) ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 679 ) ;
    } 
    printesc ( 520 ) ;
    print ( 889 ) ;
    {
      helpptr = 3 ;
      helpline [2 ]= 890 ;
      helpline [1 ]= 891 ;
      helpline [0 ]= 892 ;
    } 
    error () ;
    flushmath () ;
  } 
  pushnest () ;
  if ( curlist .modefield == 203 ) 
  {
    curlist .modefield = -1 ;
    curlist .auxfield .cint = nest [nestptr - 2 ].auxfield .cint ;
  } 
  else if ( curlist .modefield > 0 ) 
  curlist .modefield = - (integer) curlist .modefield ;
  scanspec ( 6 , false ) ;
  mem [memtop - 8 ].hh .v.RH = 0 ;
  curalign = memtop - 8 ;
  curloop = 0 ;
  scannerstatus = 4 ;
  warningindex = savecsptr ;
  alignstate = -1000000L ;
  while ( true ) {
      
    mem [curalign ].hh .v.RH = newparamglue ( 11 ) ;
    curalign = mem [curalign ].hh .v.RH ;
    if ( curcmd == 5 ) 
    goto lab30 ;
    p = memtop - 4 ;
    mem [p ].hh .v.RH = 0 ;
    while ( true ) {
	
      getpreambletoken () ;
      if ( curcmd == 6 ) 
      goto lab31 ;
      if ( ( curcmd <= 5 ) && ( curcmd >= 4 ) && ( alignstate == -1000000L ) ) 
      if ( ( p == memtop - 4 ) && ( curloop == 0 ) && ( curcmd == 4 ) ) 
      curloop = curalign ;
      else {
	  
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 898 ) ;
	} 
	{
	  helpptr = 3 ;
	  helpline [2 ]= 899 ;
	  helpline [1 ]= 900 ;
	  helpline [0 ]= 901 ;
	} 
	backerror () ;
	goto lab31 ;
      } 
      else if ( ( curcmd != 10 ) || ( p != memtop - 4 ) ) 
      {
	mem [p ].hh .v.RH = getavail () ;
	p = mem [p ].hh .v.RH ;
	mem [p ].hh .v.LH = curtok ;
      } 
    } 
    lab31: ;
    mem [curalign ].hh .v.RH = newnullbox () ;
    curalign = mem [curalign ].hh .v.RH ;
    mem [curalign ].hh .v.LH = memtop - 9 ;
    mem [curalign + 1 ].cint = -1073741824L ;
    mem [curalign + 3 ].cint = mem [memtop - 4 ].hh .v.RH ;
    p = memtop - 4 ;
    mem [p ].hh .v.RH = 0 ;
    while ( true ) {
	
      lab22: getpreambletoken () ;
      if ( ( curcmd <= 5 ) && ( curcmd >= 4 ) && ( alignstate == -1000000L ) ) 
      goto lab32 ;
      if ( curcmd == 6 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 902 ) ;
	} 
	{
	  helpptr = 3 ;
	  helpline [2 ]= 899 ;
	  helpline [1 ]= 900 ;
	  helpline [0 ]= 903 ;
	} 
	error () ;
	goto lab22 ;
      } 
      mem [p ].hh .v.RH = getavail () ;
      p = mem [p ].hh .v.RH ;
      mem [p ].hh .v.LH = curtok ;
    } 
    lab32: mem [p ].hh .v.RH = getavail () ;
    p = mem [p ].hh .v.RH ;
    mem [p ].hh .v.LH = 14614 ;
    mem [curalign + 2 ].cint = mem [memtop - 4 ].hh .v.RH ;
  } 
  lab30: scannerstatus = 0 ;
  newsavelevel ( 6 ) ;
  if ( eqtb [13064 ].hh .v.RH != 0 ) 
  begintokenlist ( eqtb [13064 ].hh .v.RH , 13 ) ;
  alignpeek () ;
} 
void 
#ifdef HAVE_PROTOTYPES
zinitspan ( halfword p ) 
#else
zinitspan ( p ) 
  halfword p ;
#endif
{
  initspan_regmem 
  pushnest () ;
  if ( curlist .modefield == -102 ) 
  curlist .auxfield .hh .v.LH = 1000 ;
  else {
      
    curlist .auxfield .cint = -65536000L ;
    normalparagraph () ;
  } 
  curspan = p ;
} 
void 
#ifdef HAVE_PROTOTYPES
initrow ( void ) 
#else
initrow ( ) 
#endif
{
  initrow_regmem 
  pushnest () ;
  curlist .modefield = ( -103 ) - curlist .modefield ;
  if ( curlist .modefield == -102 ) 
  curlist .auxfield .hh .v.LH = 0 ;
  else curlist .auxfield .cint = 0 ;
  {
    mem [curlist .tailfield ].hh .v.RH = newglue ( mem [mem [memtop - 8 ]
    .hh .v.RH + 1 ].hh .v.LH ) ;
    curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
  } 
  mem [curlist .tailfield ].hh.b1 = 12 ;
  curalign = mem [mem [memtop - 8 ].hh .v.RH ].hh .v.RH ;
  curtail = curhead ;
  initspan ( curalign ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
initcol ( void ) 
#else
initcol ( ) 
#endif
{
  initcol_regmem 
  mem [curalign + 5 ].hh .v.LH = curcmd ;
  if ( curcmd == 63 ) 
  alignstate = 0 ;
  else {
      
    backinput () ;
    begintokenlist ( mem [curalign + 3 ].cint , 1 ) ;
  } 
} 
boolean 
#ifdef HAVE_PROTOTYPES
fincol ( void ) 
#else
fincol ( ) 
#endif
{
  /* 10 */ register boolean Result; fincol_regmem 
  halfword p  ;
  halfword q, r  ;
  halfword s  ;
  halfword u  ;
  scaled w  ;
  glueord o  ;
  halfword n  ;
  if ( curalign == 0 ) 
  confusion ( 904 ) ;
  q = mem [curalign ].hh .v.RH ;
  if ( q == 0 ) 
  confusion ( 904 ) ;
  if ( alignstate < 500000L ) 
  fatalerror ( 594 ) ;
  p = mem [q ].hh .v.RH ;
  if ( ( p == 0 ) && ( mem [curalign + 5 ].hh .v.LH < 257 ) ) 
  if ( curloop != 0 ) 
  {
    mem [q ].hh .v.RH = newnullbox () ;
    p = mem [q ].hh .v.RH ;
    mem [p ].hh .v.LH = memtop - 9 ;
    mem [p + 1 ].cint = -1073741824L ;
    curloop = mem [curloop ].hh .v.RH ;
    q = memtop - 4 ;
    r = mem [curloop + 3 ].cint ;
    while ( r != 0 ) {
	
      mem [q ].hh .v.RH = getavail () ;
      q = mem [q ].hh .v.RH ;
      mem [q ].hh .v.LH = mem [r ].hh .v.LH ;
      r = mem [r ].hh .v.RH ;
    } 
    mem [q ].hh .v.RH = 0 ;
    mem [p + 3 ].cint = mem [memtop - 4 ].hh .v.RH ;
    q = memtop - 4 ;
    r = mem [curloop + 2 ].cint ;
    while ( r != 0 ) {
	
      mem [q ].hh .v.RH = getavail () ;
      q = mem [q ].hh .v.RH ;
      mem [q ].hh .v.LH = mem [r ].hh .v.LH ;
      r = mem [r ].hh .v.RH ;
    } 
    mem [q ].hh .v.RH = 0 ;
    mem [p + 2 ].cint = mem [memtop - 4 ].hh .v.RH ;
    curloop = mem [curloop ].hh .v.RH ;
    mem [p ].hh .v.RH = newglue ( mem [curloop + 1 ].hh .v.LH ) ;
  } 
  else {
      
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 905 ) ;
    } 
    printesc ( 894 ) ;
    {
      helpptr = 3 ;
      helpline [2 ]= 906 ;
      helpline [1 ]= 907 ;
      helpline [0 ]= 908 ;
    } 
    mem [curalign + 5 ].hh .v.LH = 257 ;
    error () ;
  } 
  if ( mem [curalign + 5 ].hh .v.LH != 256 ) 
  {
    unsave () ;
    newsavelevel ( 6 ) ;
    {
      if ( curlist .modefield == -102 ) 
      {
	adjusttail = curtail ;
	u = hpack ( mem [curlist .headfield ].hh .v.RH , 0 , 1 ) ;
	w = mem [u + 1 ].cint ;
	curtail = adjusttail ;
	adjusttail = 0 ;
      } 
      else {
	  
	u = vpackage ( mem [curlist .headfield ].hh .v.RH , 0 , 1 , 0 ) ;
	w = mem [u + 3 ].cint ;
      } 
      n = 0 ;
      if ( curspan != curalign ) 
      {
	q = curspan ;
	do {
	    incr ( n ) ;
	  q = mem [mem [q ].hh .v.RH ].hh .v.RH ;
	} while ( ! ( q == curalign ) ) ;
	if ( n > 255 ) 
	confusion ( 909 ) ;
	q = curspan ;
	while ( mem [mem [q ].hh .v.LH ].hh .v.RH < n ) q = mem [q ].hh 
	.v.LH ;
	if ( mem [mem [q ].hh .v.LH ].hh .v.RH > n ) 
	{
	  s = getnode ( 2 ) ;
	  mem [s ].hh .v.LH = mem [q ].hh .v.LH ;
	  mem [s ].hh .v.RH = n ;
	  mem [q ].hh .v.LH = s ;
	  mem [s + 1 ].cint = w ;
	} 
	else if ( mem [mem [q ].hh .v.LH + 1 ].cint < w ) 
	mem [mem [q ].hh .v.LH + 1 ].cint = w ;
      } 
      else if ( w > mem [curalign + 1 ].cint ) 
      mem [curalign + 1 ].cint = w ;
      mem [u ].hh.b0 = 13 ;
      mem [u ].hh.b1 = n ;
      if ( totalstretch [3 ]!= 0 ) 
      o = 3 ;
      else if ( totalstretch [2 ]!= 0 ) 
      o = 2 ;
      else if ( totalstretch [1 ]!= 0 ) 
      o = 1 ;
      else o = 0 ;
      mem [u + 5 ].hh.b1 = o ;
      mem [u + 6 ].cint = totalstretch [o ];
      if ( totalshrink [3 ]!= 0 ) 
      o = 3 ;
      else if ( totalshrink [2 ]!= 0 ) 
      o = 2 ;
      else if ( totalshrink [1 ]!= 0 ) 
      o = 1 ;
      else o = 0 ;
      mem [u + 5 ].hh.b0 = o ;
      mem [u + 4 ].cint = totalshrink [o ];
      popnest () ;
      mem [curlist .tailfield ].hh .v.RH = u ;
      curlist .tailfield = u ;
    } 
    {
      mem [curlist .tailfield ].hh .v.RH = newglue ( mem [mem [curalign ]
      .hh .v.RH + 1 ].hh .v.LH ) ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
    mem [curlist .tailfield ].hh.b1 = 12 ;
    if ( mem [curalign + 5 ].hh .v.LH >= 257 ) 
    {
      Result = true ;
      return Result ;
    } 
    initspan ( p ) ;
  } 
  alignstate = 1000000L ;
  do {
      getxtoken () ;
  } while ( ! ( curcmd != 10 ) ) ;
  curalign = p ;
  initcol () ;
  Result = false ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
finrow ( void ) 
#else
finrow ( ) 
#endif
{
  finrow_regmem 
  halfword p  ;
  if ( curlist .modefield == -102 ) 
  {
    p = hpack ( mem [curlist .headfield ].hh .v.RH , 0 , 1 ) ;
    popnest () ;
    appendtovlist ( p ) ;
    if ( curhead != curtail ) 
    {
      mem [curlist .tailfield ].hh .v.RH = mem [curhead ].hh .v.RH ;
      curlist .tailfield = curtail ;
    } 
  } 
  else {
      
    p = vpackage ( mem [curlist .headfield ].hh .v.RH , 0 , 1 , 1073741823L 
    ) ;
    popnest () ;
    mem [curlist .tailfield ].hh .v.RH = p ;
    curlist .tailfield = p ;
    curlist .auxfield .hh .v.LH = 1000 ;
  } 
  mem [p ].hh.b0 = 13 ;
  mem [p + 6 ].cint = 0 ;
  if ( eqtb [13064 ].hh .v.RH != 0 ) 
  begintokenlist ( eqtb [13064 ].hh .v.RH , 13 ) ;
  alignpeek () ;
} 
void 
#ifdef HAVE_PROTOTYPES
finalign ( void ) 
#else
finalign ( ) 
#endif
{
  finalign_regmem 
  halfword p, q, r, s, u, v  ;
  scaled t, w  ;
  scaled o  ;
  halfword n  ;
  scaled rulesave  ;
  memoryword auxsave  ;
  if ( curgroup != 6 ) 
  confusion ( 910 ) ;
  unsave () ;
  if ( curgroup != 6 ) 
  confusion ( 911 ) ;
  unsave () ;
  if ( nest [nestptr - 1 ].modefield == 203 ) 
  o = eqtb [15748 ].cint ;
  else o = 0 ;
  q = mem [mem [memtop - 8 ].hh .v.RH ].hh .v.RH ;
  do {
      flushlist ( mem [q + 3 ].cint ) ;
    flushlist ( mem [q + 2 ].cint ) ;
    p = mem [mem [q ].hh .v.RH ].hh .v.RH ;
    if ( mem [q + 1 ].cint == -1073741824L ) 
    {
      mem [q + 1 ].cint = 0 ;
      r = mem [q ].hh .v.RH ;
      s = mem [r + 1 ].hh .v.LH ;
      if ( s != membot ) 
      {
	incr ( mem [membot ].hh .v.RH ) ;
	deleteglueref ( s ) ;
	mem [r + 1 ].hh .v.LH = membot ;
      } 
    } 
    if ( mem [q ].hh .v.LH != memtop - 9 ) 
    {
      t = mem [q + 1 ].cint + mem [mem [mem [q ].hh .v.RH + 1 ].hh 
      .v.LH + 1 ].cint ;
      r = mem [q ].hh .v.LH ;
      s = memtop - 9 ;
      mem [s ].hh .v.LH = p ;
      n = 1 ;
      do {
	  mem [r + 1 ].cint = mem [r + 1 ].cint - t ;
	u = mem [r ].hh .v.LH ;
	while ( mem [r ].hh .v.RH > n ) {
	    
	  s = mem [s ].hh .v.LH ;
	  n = mem [mem [s ].hh .v.LH ].hh .v.RH + 1 ;
	} 
	if ( mem [r ].hh .v.RH < n ) 
	{
	  mem [r ].hh .v.LH = mem [s ].hh .v.LH ;
	  mem [s ].hh .v.LH = r ;
	  decr ( mem [r ].hh .v.RH ) ;
	  s = r ;
	} 
	else {
	    
	  if ( mem [r + 1 ].cint > mem [mem [s ].hh .v.LH + 1 ].cint ) 
	  mem [mem [s ].hh .v.LH + 1 ].cint = mem [r + 1 ].cint ;
	  freenode ( r , 2 ) ;
	} 
	r = u ;
      } while ( ! ( r == memtop - 9 ) ) ;
    } 
    mem [q ].hh.b0 = 13 ;
    mem [q ].hh.b1 = 0 ;
    mem [q + 3 ].cint = 0 ;
    mem [q + 2 ].cint = 0 ;
    mem [q + 5 ].hh.b1 = 0 ;
    mem [q + 5 ].hh.b0 = 0 ;
    mem [q + 6 ].cint = 0 ;
    mem [q + 4 ].cint = 0 ;
    q = p ;
  } while ( ! ( q == 0 ) ) ;
  saveptr = saveptr - 2 ;
  packbeginline = - (integer) curlist .mlfield ;
  if ( curlist .modefield == -1 ) 
  {
    rulesave = eqtb [15749 ].cint ;
    eqtb [15749 ].cint = 0 ;
    p = hpack ( mem [memtop - 8 ].hh .v.RH , savestack [saveptr + 1 ].cint 
    , savestack [saveptr + 0 ].cint ) ;
    eqtb [15749 ].cint = rulesave ;
  } 
  else {
      
    q = mem [mem [memtop - 8 ].hh .v.RH ].hh .v.RH ;
    do {
	mem [q + 3 ].cint = mem [q + 1 ].cint ;
      mem [q + 1 ].cint = 0 ;
      q = mem [mem [q ].hh .v.RH ].hh .v.RH ;
    } while ( ! ( q == 0 ) ) ;
    p = vpackage ( mem [memtop - 8 ].hh .v.RH , savestack [saveptr + 1 ]
    .cint , savestack [saveptr + 0 ].cint , 1073741823L ) ;
    q = mem [mem [memtop - 8 ].hh .v.RH ].hh .v.RH ;
    do {
	mem [q + 1 ].cint = mem [q + 3 ].cint ;
      mem [q + 3 ].cint = 0 ;
      q = mem [mem [q ].hh .v.RH ].hh .v.RH ;
    } while ( ! ( q == 0 ) ) ;
  } 
  packbeginline = 0 ;
  q = mem [curlist .headfield ].hh .v.RH ;
  s = curlist .headfield ;
  while ( q != 0 ) {
      
    if ( ! ( q >= himemmin ) ) 
    if ( mem [q ].hh.b0 == 13 ) 
    {
      if ( curlist .modefield == -1 ) 
      {
	mem [q ].hh.b0 = 0 ;
	mem [q + 1 ].cint = mem [p + 1 ].cint ;
      } 
      else {
	  
	mem [q ].hh.b0 = 1 ;
	mem [q + 3 ].cint = mem [p + 3 ].cint ;
      } 
      mem [q + 5 ].hh.b1 = mem [p + 5 ].hh.b1 ;
      mem [q + 5 ].hh.b0 = mem [p + 5 ].hh.b0 ;
      mem [q + 6 ].gr = mem [p + 6 ].gr ;
      mem [q + 4 ].cint = o ;
      r = mem [mem [q + 5 ].hh .v.RH ].hh .v.RH ;
      s = mem [mem [p + 5 ].hh .v.RH ].hh .v.RH ;
      do {
	  n = mem [r ].hh.b1 ;
	t = mem [s + 1 ].cint ;
	w = t ;
	u = memtop - 4 ;
	while ( n > 0 ) {
	    
	  decr ( n ) ;
	  s = mem [s ].hh .v.RH ;
	  v = mem [s + 1 ].hh .v.LH ;
	  mem [u ].hh .v.RH = newglue ( v ) ;
	  u = mem [u ].hh .v.RH ;
	  mem [u ].hh.b1 = 12 ;
	  t = t + mem [v + 1 ].cint ;
	  if ( mem [p + 5 ].hh.b0 == 1 ) 
	  {
	    if ( mem [v ].hh.b0 == mem [p + 5 ].hh.b1 ) 
	    t = t + round ( mem [p + 6 ].gr * mem [v + 2 ].cint ) ;
	  } 
	  else if ( mem [p + 5 ].hh.b0 == 2 ) 
	  {
	    if ( mem [v ].hh.b1 == mem [p + 5 ].hh.b1 ) 
	    t = t - round ( mem [p + 6 ].gr * mem [v + 3 ].cint ) ;
	  } 
	  s = mem [s ].hh .v.RH ;
	  mem [u ].hh .v.RH = newnullbox () ;
	  u = mem [u ].hh .v.RH ;
	  t = t + mem [s + 1 ].cint ;
	  if ( curlist .modefield == -1 ) 
	  mem [u + 1 ].cint = mem [s + 1 ].cint ;
	  else {
	      
	    mem [u ].hh.b0 = 1 ;
	    mem [u + 3 ].cint = mem [s + 1 ].cint ;
	  } 
	} 
	if ( curlist .modefield == -1 ) 
	{
	  mem [r + 3 ].cint = mem [q + 3 ].cint ;
	  mem [r + 2 ].cint = mem [q + 2 ].cint ;
	  if ( t == mem [r + 1 ].cint ) 
	  {
	    mem [r + 5 ].hh.b0 = 0 ;
	    mem [r + 5 ].hh.b1 = 0 ;
	    mem [r + 6 ].gr = 0.0 ;
	  } 
	  else if ( t > mem [r + 1 ].cint ) 
	  {
	    mem [r + 5 ].hh.b0 = 1 ;
	    if ( mem [r + 6 ].cint == 0 ) 
	    mem [r + 6 ].gr = 0.0 ;
	    else mem [r + 6 ].gr = ( t - mem [r + 1 ].cint ) / ((double) 
	    mem [r + 6 ].cint ) ;
	  } 
	  else {
	      
	    mem [r + 5 ].hh.b1 = mem [r + 5 ].hh.b0 ;
	    mem [r + 5 ].hh.b0 = 2 ;
	    if ( mem [r + 4 ].cint == 0 ) 
	    mem [r + 6 ].gr = 0.0 ;
	    else if ( ( mem [r + 5 ].hh.b1 == 0 ) && ( mem [r + 1 ].cint - 
	    t > mem [r + 4 ].cint ) ) 
	    mem [r + 6 ].gr = 1.0 ;
	    else mem [r + 6 ].gr = ( mem [r + 1 ].cint - t ) / ((double) 
	    mem [r + 4 ].cint ) ;
	  } 
	  mem [r + 1 ].cint = w ;
	  mem [r ].hh.b0 = 0 ;
	} 
	else {
	    
	  mem [r + 1 ].cint = mem [q + 1 ].cint ;
	  if ( t == mem [r + 3 ].cint ) 
	  {
	    mem [r + 5 ].hh.b0 = 0 ;
	    mem [r + 5 ].hh.b1 = 0 ;
	    mem [r + 6 ].gr = 0.0 ;
	  } 
	  else if ( t > mem [r + 3 ].cint ) 
	  {
	    mem [r + 5 ].hh.b0 = 1 ;
	    if ( mem [r + 6 ].cint == 0 ) 
	    mem [r + 6 ].gr = 0.0 ;
	    else mem [r + 6 ].gr = ( t - mem [r + 3 ].cint ) / ((double) 
	    mem [r + 6 ].cint ) ;
	  } 
	  else {
	      
	    mem [r + 5 ].hh.b1 = mem [r + 5 ].hh.b0 ;
	    mem [r + 5 ].hh.b0 = 2 ;
	    if ( mem [r + 4 ].cint == 0 ) 
	    mem [r + 6 ].gr = 0.0 ;
	    else if ( ( mem [r + 5 ].hh.b1 == 0 ) && ( mem [r + 3 ].cint - 
	    t > mem [r + 4 ].cint ) ) 
	    mem [r + 6 ].gr = 1.0 ;
	    else mem [r + 6 ].gr = ( mem [r + 3 ].cint - t ) / ((double) 
	    mem [r + 4 ].cint ) ;
	  } 
	  mem [r + 3 ].cint = w ;
	  mem [r ].hh.b0 = 1 ;
	} 
	mem [r + 4 ].cint = 0 ;
	if ( u != memtop - 4 ) 
	{
	  mem [u ].hh .v.RH = mem [r ].hh .v.RH ;
	  mem [r ].hh .v.RH = mem [memtop - 4 ].hh .v.RH ;
	  r = u ;
	} 
	r = mem [mem [r ].hh .v.RH ].hh .v.RH ;
	s = mem [mem [s ].hh .v.RH ].hh .v.RH ;
      } while ( ! ( r == 0 ) ) ;
    } 
    else if ( mem [q ].hh.b0 == 2 ) 
    {
      if ( ( mem [q + 1 ].cint == -1073741824L ) ) 
      mem [q + 1 ].cint = mem [p + 1 ].cint ;
      if ( ( mem [q + 3 ].cint == -1073741824L ) ) 
      mem [q + 3 ].cint = mem [p + 3 ].cint ;
      if ( ( mem [q + 2 ].cint == -1073741824L ) ) 
      mem [q + 2 ].cint = mem [p + 2 ].cint ;
      if ( o != 0 ) 
      {
	r = mem [q ].hh .v.RH ;
	mem [q ].hh .v.RH = 0 ;
	q = hpack ( q , 0 , 1 ) ;
	mem [q + 4 ].cint = o ;
	mem [q ].hh .v.RH = r ;
	mem [s ].hh .v.RH = q ;
      } 
    } 
    s = q ;
    q = mem [q ].hh .v.RH ;
  } 
  flushnodelist ( p ) ;
  popalignment () ;
  auxsave = curlist .auxfield ;
  p = mem [curlist .headfield ].hh .v.RH ;
  q = curlist .tailfield ;
  popnest () ;
  if ( curlist .modefield == 203 ) 
  {
    doassignments () ;
    if ( curcmd != 3 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 1165 ) ;
      } 
      {
	helpptr = 2 ;
	helpline [1 ]= 890 ;
	helpline [0 ]= 891 ;
      } 
      backerror () ;
    } 
    else {
	
      getxtoken () ;
      if ( curcmd != 3 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 1161 ) ;
	} 
	{
	  helpptr = 2 ;
	  helpline [1 ]= 1162 ;
	  helpline [0 ]= 1163 ;
	} 
	backerror () ;
      } 
    } 
    popnest () ;
    {
      mem [curlist .tailfield ].hh .v.RH = newpenalty ( eqtb [15174 ].cint 
      ) ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
    {
      mem [curlist .tailfield ].hh .v.RH = newparamglue ( 3 ) ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
    mem [curlist .tailfield ].hh .v.RH = p ;
    if ( p != 0 ) 
    curlist .tailfield = q ;
    {
      mem [curlist .tailfield ].hh .v.RH = newpenalty ( eqtb [15175 ].cint 
      ) ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
    {
      mem [curlist .tailfield ].hh .v.RH = newparamglue ( 4 ) ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
    curlist .auxfield .cint = auxsave .cint ;
    resumeafterdisplay () ;
  } 
  else {
      
    curlist .auxfield = auxsave ;
    mem [curlist .tailfield ].hh .v.RH = p ;
    if ( p != 0 ) 
    curlist .tailfield = q ;
    if ( curlist .modefield == 1 ) 
    buildpage () ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
alignpeek ( void ) 
#else
alignpeek ( ) 
#endif
{
  /* 20 */ alignpeek_regmem 
  lab20: alignstate = 1000000L ;
  do {
      getxtoken () ;
  } while ( ! ( curcmd != 10 ) ) ;
  if ( curcmd == 34 ) 
  {
    scanleftbrace () ;
    newsavelevel ( 7 ) ;
    if ( curlist .modefield == -1 ) 
    normalparagraph () ;
  } 
  else if ( curcmd == 2 ) 
  finalign () ;
  else if ( ( curcmd == 5 ) && ( curchr == 258 ) ) 
  goto lab20 ;
  else {
      
    initrow () ;
    initcol () ;
  } 
} 
halfword 
#ifdef HAVE_PROTOTYPES
zfiniteshrink ( halfword p ) 
#else
zfiniteshrink ( p ) 
  halfword p ;
#endif
{
  register halfword Result; finiteshrink_regmem 
  halfword q  ;
  if ( noshrinkerroryet ) 
  {
    noshrinkerroryet = false ;
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 912 ) ;
    } 
    {
      helpptr = 5 ;
      helpline [4 ]= 913 ;
      helpline [3 ]= 914 ;
      helpline [2 ]= 915 ;
      helpline [1 ]= 916 ;
      helpline [0 ]= 917 ;
    } 
    error () ;
  } 
  q = newspec ( p ) ;
  mem [q ].hh.b1 = 0 ;
  deleteglueref ( p ) ;
  Result = q ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
ztrybreak ( integer pi , smallnumber breaktype ) 
#else
ztrybreak ( pi , breaktype ) 
  integer pi ;
  smallnumber breaktype ;
#endif
{
  /* 10 30 31 22 60 */ trybreak_regmem 
  halfword r  ;
  halfword prevr  ;
  halfword oldl  ;
  boolean nobreakyet  ;
  halfword prevprevr  ;
  halfword s  ;
  halfword q  ;
  halfword v  ;
  integer t  ;
  internalfontnumber f  ;
  halfword l  ;
  boolean noderstaysactive  ;
  scaled linewidth  ;
  char fitclass  ;
  halfword b  ;
  integer d  ;
  boolean artificialdemerits  ;
  halfword savelink  ;
  scaled shortfall  ;
  if ( abs ( pi ) >= 10000 ) 
  if ( pi > 0 ) 
  goto lab10 ;
  else pi = -10000 ;
  nobreakyet = true ;
  prevr = memtop - 7 ;
  oldl = 0 ;
  curactivewidth [1 ]= activewidth [1 ];
  curactivewidth [2 ]= activewidth [2 ];
  curactivewidth [3 ]= activewidth [3 ];
  curactivewidth [4 ]= activewidth [4 ];
  curactivewidth [5 ]= activewidth [5 ];
  curactivewidth [6 ]= activewidth [6 ];
  while ( true ) {
      
    lab22: r = mem [prevr ].hh .v.RH ;
    if ( mem [r ].hh.b0 == 2 ) 
    {
      curactivewidth [1 ]= curactivewidth [1 ]+ mem [r + 1 ].cint ;
      curactivewidth [2 ]= curactivewidth [2 ]+ mem [r + 2 ].cint ;
      curactivewidth [3 ]= curactivewidth [3 ]+ mem [r + 3 ].cint ;
      curactivewidth [4 ]= curactivewidth [4 ]+ mem [r + 4 ].cint ;
      curactivewidth [5 ]= curactivewidth [5 ]+ mem [r + 5 ].cint ;
      curactivewidth [6 ]= curactivewidth [6 ]+ mem [r + 6 ].cint ;
      prevprevr = prevr ;
      prevr = r ;
      goto lab22 ;
    } 
    {
      l = mem [r + 1 ].hh .v.LH ;
      if ( l > oldl ) 
      {
	if ( ( minimumdemerits < 1073741823L ) && ( ( oldl != easyline ) || ( 
	r == memtop - 7 ) ) ) 
	{
	  if ( nobreakyet ) 
	  {
	    nobreakyet = false ;
	    breakwidth [1 ]= background [1 ];
	    breakwidth [2 ]= background [2 ];
	    breakwidth [3 ]= background [3 ];
	    breakwidth [4 ]= background [4 ];
	    breakwidth [5 ]= background [5 ];
	    breakwidth [6 ]= background [6 ];
	    s = curp ;
	    if ( breaktype > 0 ) 
	    if ( curp != 0 ) 
	    {
	      t = mem [curp ].hh.b1 ;
	      v = curp ;
	      s = mem [curp + 1 ].hh .v.RH ;
	      while ( t > 0 ) {
		  
		decr ( t ) ;
		v = mem [v ].hh .v.RH ;
		if ( ( v >= himemmin ) ) 
		{
		  f = mem [v ].hh.b0 ;
		  breakwidth [1 ]= breakwidth [1 ]- fontinfo [widthbase [
		  f ]+ fontinfo [charbase [f ]+ effectivechar ( true , f , 
		  mem [v ].hh.b1 ) ].qqqq .b0 ].cint ;
		} 
		else switch ( mem [v ].hh.b0 ) 
		{case 6 : 
		  {
		    f = mem [v + 1 ].hh.b0 ;
		    breakwidth [1 ]= breakwidth [1 ]- fontinfo [widthbase 
		    [f ]+ fontinfo [charbase [f ]+ effectivechar ( true , 
		    f , mem [v + 1 ].hh.b1 ) ].qqqq .b0 ].cint ;
		  } 
		  break ;
		case 0 : 
		case 1 : 
		case 2 : 
		case 11 : 
		  breakwidth [1 ]= breakwidth [1 ]- mem [v + 1 ].cint ;
		  break ;
		  default: 
		  confusion ( 918 ) ;
		  break ;
		} 
	      } 
	      while ( s != 0 ) {
		  
		if ( ( s >= himemmin ) ) 
		{
		  f = mem [s ].hh.b0 ;
		  breakwidth [1 ]= breakwidth [1 ]+ fontinfo [widthbase [
		  f ]+ fontinfo [charbase [f ]+ effectivechar ( true , f , 
		  mem [s ].hh.b1 ) ].qqqq .b0 ].cint ;
		} 
		else switch ( mem [s ].hh.b0 ) 
		{case 6 : 
		  {
		    f = mem [s + 1 ].hh.b0 ;
		    breakwidth [1 ]= breakwidth [1 ]+ fontinfo [widthbase 
		    [f ]+ fontinfo [charbase [f ]+ effectivechar ( true , 
		    f , mem [s + 1 ].hh.b1 ) ].qqqq .b0 ].cint ;
		  } 
		  break ;
		case 0 : 
		case 1 : 
		case 2 : 
		case 11 : 
		  breakwidth [1 ]= breakwidth [1 ]+ mem [s + 1 ].cint ;
		  break ;
		  default: 
		  confusion ( 919 ) ;
		  break ;
		} 
		s = mem [s ].hh .v.RH ;
	      } 
	      breakwidth [1 ]= breakwidth [1 ]+ discwidth ;
	      if ( mem [curp + 1 ].hh .v.RH == 0 ) 
	      s = mem [v ].hh .v.RH ;
	    } 
	    while ( s != 0 ) {
		
	      if ( ( s >= himemmin ) ) 
	      goto lab30 ;
	      switch ( mem [s ].hh.b0 ) 
	      {case 10 : 
		{
		  v = mem [s + 1 ].hh .v.LH ;
		  breakwidth [1 ]= breakwidth [1 ]- mem [v + 1 ].cint ;
		  breakwidth [2 + mem [v ].hh.b0 ]= breakwidth [2 + mem [
		  v ].hh.b0 ]- mem [v + 2 ].cint ;
		  breakwidth [6 ]= breakwidth [6 ]- mem [v + 3 ].cint ;
		} 
		break ;
	      case 12 : 
		;
		break ;
	      case 9 : 
		breakwidth [1 ]= breakwidth [1 ]- mem [s + 1 ].cint ;
		break ;
	      case 11 : 
		if ( mem [s ].hh.b1 != 1 ) 
		goto lab30 ;
		else breakwidth [1 ]= breakwidth [1 ]- mem [s + 1 ].cint 
		;
		break ;
		default: 
		goto lab30 ;
		break ;
	      } 
	      s = mem [s ].hh .v.RH ;
	    } 
	    lab30: ;
	  } 
	  if ( mem [prevr ].hh.b0 == 2 ) 
	  {
	    mem [prevr + 1 ].cint = mem [prevr + 1 ].cint - curactivewidth 
	    [1 ]+ breakwidth [1 ];
	    mem [prevr + 2 ].cint = mem [prevr + 2 ].cint - curactivewidth 
	    [2 ]+ breakwidth [2 ];
	    mem [prevr + 3 ].cint = mem [prevr + 3 ].cint - curactivewidth 
	    [3 ]+ breakwidth [3 ];
	    mem [prevr + 4 ].cint = mem [prevr + 4 ].cint - curactivewidth 
	    [4 ]+ breakwidth [4 ];
	    mem [prevr + 5 ].cint = mem [prevr + 5 ].cint - curactivewidth 
	    [5 ]+ breakwidth [5 ];
	    mem [prevr + 6 ].cint = mem [prevr + 6 ].cint - curactivewidth 
	    [6 ]+ breakwidth [6 ];
	  } 
	  else if ( prevr == memtop - 7 ) 
	  {
	    activewidth [1 ]= breakwidth [1 ];
	    activewidth [2 ]= breakwidth [2 ];
	    activewidth [3 ]= breakwidth [3 ];
	    activewidth [4 ]= breakwidth [4 ];
	    activewidth [5 ]= breakwidth [5 ];
	    activewidth [6 ]= breakwidth [6 ];
	  } 
	  else {
	      
	    q = getnode ( 7 ) ;
	    mem [q ].hh .v.RH = r ;
	    mem [q ].hh.b0 = 2 ;
	    mem [q ].hh.b1 = 0 ;
	    mem [q + 1 ].cint = breakwidth [1 ]- curactivewidth [1 ];
	    mem [q + 2 ].cint = breakwidth [2 ]- curactivewidth [2 ];
	    mem [q + 3 ].cint = breakwidth [3 ]- curactivewidth [3 ];
	    mem [q + 4 ].cint = breakwidth [4 ]- curactivewidth [4 ];
	    mem [q + 5 ].cint = breakwidth [5 ]- curactivewidth [5 ];
	    mem [q + 6 ].cint = breakwidth [6 ]- curactivewidth [6 ];
	    mem [prevr ].hh .v.RH = q ;
	    prevprevr = prevr ;
	    prevr = q ;
	  } 
	  if ( abs ( eqtb [15179 ].cint ) >= 1073741823L - minimumdemerits ) 
	  minimumdemerits = 1073741822L ;
	  else minimumdemerits = minimumdemerits + abs ( eqtb [15179 ].cint 
	  ) ;
	  {register integer for_end; fitclass = 0 ;for_end = 3 ; if ( 
	  fitclass <= for_end) do 
	    {
	      if ( minimaldemerits [fitclass ]<= minimumdemerits ) 
	      {
		q = getnode ( 2 ) ;
		mem [q ].hh .v.RH = passive ;
		passive = q ;
		mem [q + 1 ].hh .v.RH = curp ;
	;
#ifdef STAT
		incr ( passnumber ) ;
		mem [q ].hh .v.LH = passnumber ;
#endif /* STAT */
		mem [q + 1 ].hh .v.LH = bestplace [fitclass ];
		q = getnode ( 3 ) ;
		mem [q + 1 ].hh .v.RH = passive ;
		mem [q + 1 ].hh .v.LH = bestplline [fitclass ]+ 1 ;
		mem [q ].hh.b1 = fitclass ;
		mem [q ].hh.b0 = breaktype ;
		mem [q + 2 ].cint = minimaldemerits [fitclass ];
		mem [q ].hh .v.RH = r ;
		mem [prevr ].hh .v.RH = q ;
		prevr = q ;
	;
#ifdef STAT
		if ( eqtb [15195 ].cint > 0 ) 
		{
		  printnl ( 920 ) ;
		  printint ( mem [passive ].hh .v.LH ) ;
		  print ( 921 ) ;
		  printint ( mem [q + 1 ].hh .v.LH - 1 ) ;
		  printchar ( 46 ) ;
		  printint ( fitclass ) ;
		  if ( breaktype == 1 ) 
		  printchar ( 45 ) ;
		  print ( 922 ) ;
		  printint ( mem [q + 2 ].cint ) ;
		  print ( 923 ) ;
		  if ( mem [passive + 1 ].hh .v.LH == 0 ) 
		  printchar ( 48 ) ;
		  else printint ( mem [mem [passive + 1 ].hh .v.LH ].hh 
		  .v.LH ) ;
		} 
#endif /* STAT */
	      } 
	      minimaldemerits [fitclass ]= 1073741823L ;
	    } 
	  while ( fitclass++ < for_end ) ;} 
	  minimumdemerits = 1073741823L ;
	  if ( r != memtop - 7 ) 
	  {
	    q = getnode ( 7 ) ;
	    mem [q ].hh .v.RH = r ;
	    mem [q ].hh.b0 = 2 ;
	    mem [q ].hh.b1 = 0 ;
	    mem [q + 1 ].cint = curactivewidth [1 ]- breakwidth [1 ];
	    mem [q + 2 ].cint = curactivewidth [2 ]- breakwidth [2 ];
	    mem [q + 3 ].cint = curactivewidth [3 ]- breakwidth [3 ];
	    mem [q + 4 ].cint = curactivewidth [4 ]- breakwidth [4 ];
	    mem [q + 5 ].cint = curactivewidth [5 ]- breakwidth [5 ];
	    mem [q + 6 ].cint = curactivewidth [6 ]- breakwidth [6 ];
	    mem [prevr ].hh .v.RH = q ;
	    prevprevr = prevr ;
	    prevr = q ;
	  } 
	} 
	if ( r == memtop - 7 ) 
	goto lab10 ;
	if ( l > easyline ) 
	{
	  linewidth = secondwidth ;
	  oldl = 268435454L ;
	} 
	else {
	    
	  oldl = l ;
	  if ( l > lastspecialline ) 
	  linewidth = secondwidth ;
	  else if ( eqtb [13056 ].hh .v.RH == 0 ) 
	  linewidth = firstwidth ;
	  else linewidth = mem [eqtb [13056 ].hh .v.RH + 2 * l ].cint ;
	} 
      } 
    } 
    {
      artificialdemerits = false ;
      shortfall = linewidth - curactivewidth [1 ];
      if ( shortfall > 0 ) 
      if ( ( curactivewidth [3 ]!= 0 ) || ( curactivewidth [4 ]!= 0 ) || ( 
      curactivewidth [5 ]!= 0 ) ) 
      {
	b = 0 ;
	fitclass = 2 ;
      } 
      else {
	  
	if ( shortfall > 7230584L ) 
	if ( curactivewidth [2 ]< 1663497L ) 
	{
	  b = 10000 ;
	  fitclass = 0 ;
	  goto lab31 ;
	} 
	b = badness ( shortfall , curactivewidth [2 ]) ;
	if ( b > 12 ) 
	if ( b > 99 ) 
	fitclass = 0 ;
	else fitclass = 1 ;
	else fitclass = 2 ;
	lab31: ;
      } 
      else {
	  
	if ( - (integer) shortfall > curactivewidth [6 ]) 
	b = 10001 ;
	else b = badness ( - (integer) shortfall , curactivewidth [6 ]) ;
	if ( b > 12 ) 
	fitclass = 3 ;
	else fitclass = 2 ;
      } 
      if ( ( b > 10000 ) || ( pi == -10000 ) ) 
      {
	if ( finalpass && ( minimumdemerits == 1073741823L ) && ( mem [r ]
	.hh .v.RH == memtop - 7 ) && ( prevr == memtop - 7 ) ) 
	artificialdemerits = true ;
	else if ( b > threshold ) 
	goto lab60 ;
	noderstaysactive = false ;
      } 
      else {
	  
	prevr = r ;
	if ( b > threshold ) 
	goto lab22 ;
	noderstaysactive = true ;
      } 
      if ( artificialdemerits ) 
      d = 0 ;
      else {
	  
	d = eqtb [15165 ].cint + b ;
	if ( abs ( d ) >= 10000 ) 
	d = 100000000L ;
	else d = d * d ;
	if ( pi != 0 ) 
	if ( pi > 0 ) 
	d = d + pi * pi ;
	else if ( pi > -10000 ) 
	d = d - pi * pi ;
	if ( ( breaktype == 1 ) && ( mem [r ].hh.b0 == 1 ) ) 
	if ( curp != 0 ) 
	d = d + eqtb [15177 ].cint ;
	else d = d + eqtb [15178 ].cint ;
	if ( abs ( toint ( fitclass ) - toint ( mem [r ].hh.b1 ) ) > 1 ) 
	d = d + eqtb [15179 ].cint ;
      } 
	;
#ifdef STAT
      if ( eqtb [15195 ].cint > 0 ) 
      {
	if ( printednode != curp ) 
	{
	  printnl ( 335 ) ;
	  if ( curp == 0 ) 
	  shortdisplay ( mem [printednode ].hh .v.RH ) ;
	  else {
	      
	    savelink = mem [curp ].hh .v.RH ;
	    mem [curp ].hh .v.RH = 0 ;
	    printnl ( 335 ) ;
	    shortdisplay ( mem [printednode ].hh .v.RH ) ;
	    mem [curp ].hh .v.RH = savelink ;
	  } 
	  printednode = curp ;
	} 
	printnl ( 64 ) ;
	if ( curp == 0 ) 
	printesc ( 596 ) ;
	else if ( mem [curp ].hh.b0 != 10 ) 
	{
	  if ( mem [curp ].hh.b0 == 12 ) 
	  printesc ( 531 ) ;
	  else if ( mem [curp ].hh.b0 == 7 ) 
	  printesc ( 346 ) ;
	  else if ( mem [curp ].hh.b0 == 11 ) 
	  printesc ( 337 ) ;
	  else printesc ( 340 ) ;
	} 
	print ( 924 ) ;
	if ( mem [r + 1 ].hh .v.RH == 0 ) 
	printchar ( 48 ) ;
	else printint ( mem [mem [r + 1 ].hh .v.RH ].hh .v.LH ) ;
	print ( 925 ) ;
	if ( b > 10000 ) 
	printchar ( 42 ) ;
	else printint ( b ) ;
	print ( 926 ) ;
	printint ( pi ) ;
	print ( 927 ) ;
	if ( artificialdemerits ) 
	printchar ( 42 ) ;
	else printint ( d ) ;
      } 
#endif /* STAT */
      d = d + mem [r + 2 ].cint ;
      if ( d <= minimaldemerits [fitclass ]) 
      {
	minimaldemerits [fitclass ]= d ;
	bestplace [fitclass ]= mem [r + 1 ].hh .v.RH ;
	bestplline [fitclass ]= l ;
	if ( d < minimumdemerits ) 
	minimumdemerits = d ;
      } 
      if ( noderstaysactive ) 
      goto lab22 ;
      lab60: mem [prevr ].hh .v.RH = mem [r ].hh .v.RH ;
      freenode ( r , 3 ) ;
      if ( prevr == memtop - 7 ) 
      {
	r = mem [memtop - 7 ].hh .v.RH ;
	if ( mem [r ].hh.b0 == 2 ) 
	{
	  activewidth [1 ]= activewidth [1 ]+ mem [r + 1 ].cint ;
	  activewidth [2 ]= activewidth [2 ]+ mem [r + 2 ].cint ;
	  activewidth [3 ]= activewidth [3 ]+ mem [r + 3 ].cint ;
	  activewidth [4 ]= activewidth [4 ]+ mem [r + 4 ].cint ;
	  activewidth [5 ]= activewidth [5 ]+ mem [r + 5 ].cint ;
	  activewidth [6 ]= activewidth [6 ]+ mem [r + 6 ].cint ;
	  curactivewidth [1 ]= activewidth [1 ];
	  curactivewidth [2 ]= activewidth [2 ];
	  curactivewidth [3 ]= activewidth [3 ];
	  curactivewidth [4 ]= activewidth [4 ];
	  curactivewidth [5 ]= activewidth [5 ];
	  curactivewidth [6 ]= activewidth [6 ];
	  mem [memtop - 7 ].hh .v.RH = mem [r ].hh .v.RH ;
	  freenode ( r , 7 ) ;
	} 
      } 
      else if ( mem [prevr ].hh.b0 == 2 ) 
      {
	r = mem [prevr ].hh .v.RH ;
	if ( r == memtop - 7 ) 
	{
	  curactivewidth [1 ]= curactivewidth [1 ]- mem [prevr + 1 ]
	  .cint ;
	  curactivewidth [2 ]= curactivewidth [2 ]- mem [prevr + 2 ]
	  .cint ;
	  curactivewidth [3 ]= curactivewidth [3 ]- mem [prevr + 3 ]
	  .cint ;
	  curactivewidth [4 ]= curactivewidth [4 ]- mem [prevr + 4 ]
	  .cint ;
	  curactivewidth [5 ]= curactivewidth [5 ]- mem [prevr + 5 ]
	  .cint ;
	  curactivewidth [6 ]= curactivewidth [6 ]- mem [prevr + 6 ]
	  .cint ;
	  mem [prevprevr ].hh .v.RH = memtop - 7 ;
	  freenode ( prevr , 7 ) ;
	  prevr = prevprevr ;
	} 
	else if ( mem [r ].hh.b0 == 2 ) 
	{
	  curactivewidth [1 ]= curactivewidth [1 ]+ mem [r + 1 ].cint ;
	  curactivewidth [2 ]= curactivewidth [2 ]+ mem [r + 2 ].cint ;
	  curactivewidth [3 ]= curactivewidth [3 ]+ mem [r + 3 ].cint ;
	  curactivewidth [4 ]= curactivewidth [4 ]+ mem [r + 4 ].cint ;
	  curactivewidth [5 ]= curactivewidth [5 ]+ mem [r + 5 ].cint ;
	  curactivewidth [6 ]= curactivewidth [6 ]+ mem [r + 6 ].cint ;
	  mem [prevr + 1 ].cint = mem [prevr + 1 ].cint + mem [r + 1 ]
	  .cint ;
	  mem [prevr + 2 ].cint = mem [prevr + 2 ].cint + mem [r + 2 ]
	  .cint ;
	  mem [prevr + 3 ].cint = mem [prevr + 3 ].cint + mem [r + 3 ]
	  .cint ;
	  mem [prevr + 4 ].cint = mem [prevr + 4 ].cint + mem [r + 4 ]
	  .cint ;
	  mem [prevr + 5 ].cint = mem [prevr + 5 ].cint + mem [r + 5 ]
	  .cint ;
	  mem [prevr + 6 ].cint = mem [prevr + 6 ].cint + mem [r + 6 ]
	  .cint ;
	  mem [prevr ].hh .v.RH = mem [r ].hh .v.RH ;
	  freenode ( r , 7 ) ;
	} 
      } 
    } 
  } 
  lab10: 
	;
#ifdef STAT
  if ( curp == printednode ) 
  if ( curp != 0 ) 
  if ( mem [curp ].hh.b0 == 7 ) 
  {
    t = mem [curp ].hh.b1 ;
    while ( t > 0 ) {
	
      decr ( t ) ;
      printednode = mem [printednode ].hh .v.RH ;
    } 
  } 
#endif /* STAT */
} 
void 
#ifdef HAVE_PROTOTYPES
zpostlinebreak ( integer finalwidowpenalty ) 
#else
zpostlinebreak ( finalwidowpenalty ) 
  integer finalwidowpenalty ;
#endif
{
  /* 30 31 */ postlinebreak_regmem 
  halfword q, r, s  ;
  boolean discbreak  ;
  boolean postdiscbreak  ;
  scaled curwidth  ;
  scaled curindent  ;
  quarterword t  ;
  integer pen  ;
  halfword curline  ;
  q = mem [bestbet + 1 ].hh .v.RH ;
  curp = 0 ;
  do {
      r = q ;
    q = mem [q + 1 ].hh .v.LH ;
    mem [r + 1 ].hh .v.LH = curp ;
    curp = r ;
  } while ( ! ( q == 0 ) ) ;
  curline = curlist .pgfield + 1 ;
  do {
      q = mem [curp + 1 ].hh .v.RH ;
    discbreak = false ;
    postdiscbreak = false ;
    if ( q != 0 ) 
    if ( mem [q ].hh.b0 == 10 ) 
    {
      deleteglueref ( mem [q + 1 ].hh .v.LH ) ;
      mem [q + 1 ].hh .v.LH = eqtb [12534 ].hh .v.RH ;
      mem [q ].hh.b1 = 9 ;
      incr ( mem [eqtb [12534 ].hh .v.RH ].hh .v.RH ) ;
      goto lab30 ;
    } 
    else {
	
      if ( mem [q ].hh.b0 == 7 ) 
      {
	t = mem [q ].hh.b1 ;
	if ( t == 0 ) 
	r = mem [q ].hh .v.RH ;
	else {
	    
	  r = q ;
	  while ( t > 1 ) {
	      
	    r = mem [r ].hh .v.RH ;
	    decr ( t ) ;
	  } 
	  s = mem [r ].hh .v.RH ;
	  r = mem [s ].hh .v.RH ;
	  mem [s ].hh .v.RH = 0 ;
	  flushnodelist ( mem [q ].hh .v.RH ) ;
	  mem [q ].hh.b1 = 0 ;
	} 
	if ( mem [q + 1 ].hh .v.RH != 0 ) 
	{
	  s = mem [q + 1 ].hh .v.RH ;
	  while ( mem [s ].hh .v.RH != 0 ) s = mem [s ].hh .v.RH ;
	  mem [s ].hh .v.RH = r ;
	  r = mem [q + 1 ].hh .v.RH ;
	  mem [q + 1 ].hh .v.RH = 0 ;
	  postdiscbreak = true ;
	} 
	if ( mem [q + 1 ].hh .v.LH != 0 ) 
	{
	  s = mem [q + 1 ].hh .v.LH ;
	  mem [q ].hh .v.RH = s ;
	  while ( mem [s ].hh .v.RH != 0 ) s = mem [s ].hh .v.RH ;
	  mem [q + 1 ].hh .v.LH = 0 ;
	  q = s ;
	} 
	mem [q ].hh .v.RH = r ;
	discbreak = true ;
      } 
      else if ( ( mem [q ].hh.b0 == 9 ) || ( mem [q ].hh.b0 == 11 ) ) 
      mem [q + 1 ].cint = 0 ;
    } 
    else {
	
      q = memtop - 3 ;
      while ( mem [q ].hh .v.RH != 0 ) q = mem [q ].hh .v.RH ;
    } 
    r = newparamglue ( 8 ) ;
    mem [r ].hh .v.RH = mem [q ].hh .v.RH ;
    mem [q ].hh .v.RH = r ;
    q = r ;
    lab30: ;
    r = mem [q ].hh .v.RH ;
    mem [q ].hh .v.RH = 0 ;
    q = mem [memtop - 3 ].hh .v.RH ;
    mem [memtop - 3 ].hh .v.RH = r ;
    if ( eqtb [12533 ].hh .v.RH != membot ) 
    {
      r = newparamglue ( 7 ) ;
      mem [r ].hh .v.RH = q ;
      q = r ;
    } 
    if ( curline > lastspecialline ) 
    {
      curwidth = secondwidth ;
      curindent = secondindent ;
    } 
    else if ( eqtb [13056 ].hh .v.RH == 0 ) 
    {
      curwidth = firstwidth ;
      curindent = firstindent ;
    } 
    else {
	
      curwidth = mem [eqtb [13056 ].hh .v.RH + 2 * curline ].cint ;
      curindent = mem [eqtb [13056 ].hh .v.RH + 2 * curline - 1 ].cint ;
    } 
    adjusttail = memtop - 5 ;
    justbox = hpack ( q , curwidth , 0 ) ;
    mem [justbox + 4 ].cint = curindent ;
    appendtovlist ( justbox ) ;
    if ( memtop - 5 != adjusttail ) 
    {
      mem [curlist .tailfield ].hh .v.RH = mem [memtop - 5 ].hh .v.RH ;
      curlist .tailfield = adjusttail ;
    } 
    adjusttail = 0 ;
    if ( curline + 1 != bestline ) 
    {
      pen = eqtb [15176 ].cint ;
      if ( curline == curlist .pgfield + 1 ) 
      pen = pen + eqtb [15168 ].cint ;
      if ( curline + 2 == bestline ) 
      pen = pen + finalwidowpenalty ;
      if ( discbreak ) 
      pen = pen + eqtb [15171 ].cint ;
      if ( pen != 0 ) 
      {
	r = newpenalty ( pen ) ;
	mem [curlist .tailfield ].hh .v.RH = r ;
	curlist .tailfield = r ;
      } 
    } 
    incr ( curline ) ;
    curp = mem [curp + 1 ].hh .v.LH ;
    if ( curp != 0 ) 
    if ( ! postdiscbreak ) 
    {
      r = memtop - 3 ;
      while ( true ) {
	  
	q = mem [r ].hh .v.RH ;
	if ( q == mem [curp + 1 ].hh .v.RH ) 
	goto lab31 ;
	if ( ( q >= himemmin ) ) 
	goto lab31 ;
	if ( ( mem [q ].hh.b0 < 9 ) ) 
	goto lab31 ;
	if ( mem [q ].hh.b0 == 11 ) 
	if ( mem [q ].hh.b1 != 1 ) 
	goto lab31 ;
	r = q ;
      } 
      lab31: if ( r != memtop - 3 ) 
      {
	mem [r ].hh .v.RH = 0 ;
	flushnodelist ( mem [memtop - 3 ].hh .v.RH ) ;
	mem [memtop - 3 ].hh .v.RH = q ;
      } 
    } 
  } while ( ! ( curp == 0 ) ) ;
  if ( ( curline != bestline ) || ( mem [memtop - 3 ].hh .v.RH != 0 ) ) 
  confusion ( 934 ) ;
  curlist .pgfield = bestline - 1 ;
} 
smallnumber 
#ifdef HAVE_PROTOTYPES
zreconstitute ( smallnumber j , smallnumber n , halfword bchar , halfword 
hchar ) 
#else
zreconstitute ( j , n , bchar , hchar ) 
  smallnumber j ;
  smallnumber n ;
  halfword bchar ;
  halfword hchar ;
#endif
{
  /* 22 30 */ register smallnumber Result; reconstitute_regmem 
  halfword p  ;
  halfword t  ;
  fourquarters q  ;
  halfword currh  ;
  halfword testchar  ;
  scaled w  ;
  fontindex k  ;
  hyphenpassed = 0 ;
  t = memtop - 4 ;
  w = 0 ;
  mem [memtop - 4 ].hh .v.RH = 0 ;
  curl = hu [j ];
  curq = t ;
  if ( j == 0 ) 
  {
    ligaturepresent = initlig ;
    p = initlist ;
    if ( ligaturepresent ) 
    lfthit = initlft ;
    while ( p > 0 ) {
	
      {
	mem [t ].hh .v.RH = getavail () ;
	t = mem [t ].hh .v.RH ;
	mem [t ].hh.b0 = hf ;
	mem [t ].hh.b1 = mem [p ].hh.b1 ;
      } 
      p = mem [p ].hh .v.RH ;
    } 
  } 
  else if ( curl < 256 ) 
  {
    mem [t ].hh .v.RH = getavail () ;
    t = mem [t ].hh .v.RH ;
    mem [t ].hh.b0 = hf ;
    mem [t ].hh.b1 = curl ;
  } 
  ligstack = 0 ;
  {
    if ( j < n ) 
    curr = hu [j + 1 ];
    else curr = bchar ;
    if ( odd ( hyf [j ]) ) 
    currh = hchar ;
    else currh = 256 ;
  } 
  lab22: if ( curl == 256 ) 
  {
    k = bcharlabel [hf ];
    if ( k == 0 ) 
    goto lab30 ;
    else q = fontinfo [k ].qqqq ;
  } 
  else {
      
    q = fontinfo [charbase [hf ]+ effectivechar ( true , hf , curl ) ]
    .qqqq ;
    if ( ( ( q .b2 ) % 4 ) != 1 ) 
    goto lab30 ;
    k = ligkernbase [hf ]+ q .b3 ;
    q = fontinfo [k ].qqqq ;
    if ( q .b0 > 128 ) 
    {
      k = ligkernbase [hf ]+ 256 * q .b2 + q .b3 + 32768L - 256 * ( 128 ) ;
      q = fontinfo [k ].qqqq ;
    } 
  } 
  if ( currh < 256 ) 
  testchar = currh ;
  else testchar = curr ;
  while ( true ) {
      
    if ( q .b1 == testchar ) 
    if ( q .b0 <= 128 ) 
    if ( currh < 256 ) 
    {
      hyphenpassed = j ;
      hchar = 256 ;
      currh = 256 ;
      goto lab22 ;
    } 
    else {
	
      if ( hchar < 256 ) 
      if ( odd ( hyf [j ]) ) 
      {
	hyphenpassed = j ;
	hchar = 256 ;
      } 
      if ( q .b2 < 128 ) 
      {
	if ( curl == 256 ) 
	lfthit = true ;
	if ( j == n ) 
	if ( ligstack == 0 ) 
	rthit = true ;
	{
	  if ( interrupt != 0 ) 
	  pauseforinstructions () ;
	} 
	switch ( q .b2 ) 
	{case 1 : 
	case 5 : 
	  {
	    curl = q .b3 ;
	    ligaturepresent = true ;
	  } 
	  break ;
	case 2 : 
	case 6 : 
	  {
	    curr = q .b3 ;
	    if ( ligstack > 0 ) 
	    mem [ligstack ].hh.b1 = curr ;
	    else {
		
	      ligstack = newligitem ( curr ) ;
	      if ( j == n ) 
	      bchar = 256 ;
	      else {
		  
		p = getavail () ;
		mem [ligstack + 1 ].hh .v.RH = p ;
		mem [p ].hh.b1 = hu [j + 1 ];
		mem [p ].hh.b0 = hf ;
	      } 
	    } 
	  } 
	  break ;
	case 3 : 
	  {
	    curr = q .b3 ;
	    p = ligstack ;
	    ligstack = newligitem ( curr ) ;
	    mem [ligstack ].hh .v.RH = p ;
	  } 
	  break ;
	case 7 : 
	case 11 : 
	  {
	    if ( ligaturepresent ) 
	    {
	      p = newligature ( hf , curl , mem [curq ].hh .v.RH ) ;
	      if ( lfthit ) 
	      {
		mem [p ].hh.b1 = 2 ;
		lfthit = false ;
	      } 
	      if ( false ) 
	      if ( ligstack == 0 ) 
	      {
		incr ( mem [p ].hh.b1 ) ;
		rthit = false ;
	      } 
	      mem [curq ].hh .v.RH = p ;
	      t = p ;
	      ligaturepresent = false ;
	    } 
	    curq = t ;
	    curl = q .b3 ;
	    ligaturepresent = true ;
	  } 
	  break ;
	  default: 
	  {
	    curl = q .b3 ;
	    ligaturepresent = true ;
	    if ( ligstack > 0 ) 
	    {
	      if ( mem [ligstack + 1 ].hh .v.RH > 0 ) 
	      {
		mem [t ].hh .v.RH = mem [ligstack + 1 ].hh .v.RH ;
		t = mem [t ].hh .v.RH ;
		incr ( j ) ;
	      } 
	      p = ligstack ;
	      ligstack = mem [p ].hh .v.RH ;
	      freenode ( p , 2 ) ;
	      if ( ligstack == 0 ) 
	      {
		if ( j < n ) 
		curr = hu [j + 1 ];
		else curr = bchar ;
		if ( odd ( hyf [j ]) ) 
		currh = hchar ;
		else currh = 256 ;
	      } 
	      else curr = mem [ligstack ].hh.b1 ;
	    } 
	    else if ( j == n ) 
	    goto lab30 ;
	    else {
		
	      {
		mem [t ].hh .v.RH = getavail () ;
		t = mem [t ].hh .v.RH ;
		mem [t ].hh.b0 = hf ;
		mem [t ].hh.b1 = curr ;
	      } 
	      incr ( j ) ;
	      {
		if ( j < n ) 
		curr = hu [j + 1 ];
		else curr = bchar ;
		if ( odd ( hyf [j ]) ) 
		currh = hchar ;
		else currh = 256 ;
	      } 
	    } 
	  } 
	  break ;
	} 
	if ( q .b2 > 4 ) 
	if ( q .b2 != 7 ) 
	goto lab30 ;
	goto lab22 ;
      } 
      w = fontinfo [kernbase [hf ]+ 256 * q .b2 + q .b3 ].cint ;
      goto lab30 ;
    } 
    if ( q .b0 >= 128 ) 
    if ( currh == 256 ) 
    goto lab30 ;
    else {
	
      currh = 256 ;
      goto lab22 ;
    } 
    k = k + q .b0 + 1 ;
    q = fontinfo [k ].qqqq ;
  } 
  lab30: ;
  if ( ligaturepresent ) 
  {
    p = newligature ( hf , curl , mem [curq ].hh .v.RH ) ;
    if ( lfthit ) 
    {
      mem [p ].hh.b1 = 2 ;
      lfthit = false ;
    } 
    if ( rthit ) 
    if ( ligstack == 0 ) 
    {
      incr ( mem [p ].hh.b1 ) ;
      rthit = false ;
    } 
    mem [curq ].hh .v.RH = p ;
    t = p ;
    ligaturepresent = false ;
  } 
  if ( w != 0 ) 
  {
    mem [t ].hh .v.RH = newkern ( w ) ;
    t = mem [t ].hh .v.RH ;
    w = 0 ;
  } 
  if ( ligstack > 0 ) 
  {
    curq = t ;
    curl = mem [ligstack ].hh.b1 ;
    ligaturepresent = true ;
    {
      if ( mem [ligstack + 1 ].hh .v.RH > 0 ) 
      {
	mem [t ].hh .v.RH = mem [ligstack + 1 ].hh .v.RH ;
	t = mem [t ].hh .v.RH ;
	incr ( j ) ;
      } 
      p = ligstack ;
      ligstack = mem [p ].hh .v.RH ;
      freenode ( p , 2 ) ;
      if ( ligstack == 0 ) 
      {
	if ( j < n ) 
	curr = hu [j + 1 ];
	else curr = bchar ;
	if ( odd ( hyf [j ]) ) 
	currh = hchar ;
	else currh = 256 ;
      } 
      else curr = mem [ligstack ].hh.b1 ;
    } 
    goto lab22 ;
  } 
  Result = j ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
hyphenate ( void ) 
#else
hyphenate ( ) 
#endif
{
  /* 50 30 40 41 42 45 10 */ hyphenate_regmem 
  char i, j, l  ;
  halfword q, r, s  ;
  halfword bchar  ;
  halfword majortail, minortail  ;
  ASCIIcode c  ;
  char cloc  ;
  integer rcount  ;
  halfword hyfnode  ;
  triepointer z  ;
  integer v  ;
  hyphpointer h  ;
  strnumber k  ;
  poolpointer u  ;
  {register integer for_end; j = 0 ;for_end = hn ; if ( j <= for_end) do 
    hyf [j ]= 0 ;
  while ( j++ < for_end ) ;} 
  h = hc [1 ];
  incr ( hn ) ;
  hc [hn ]= curlang ;
  {register integer for_end; j = 2 ;for_end = hn ; if ( j <= for_end) do 
    h = ( h + h + hc [j ]) % 607 ;
  while ( j++ < for_end ) ;} 
  while ( true ) {
      
    k = hyphword [h ];
    if ( k == 0 ) 
    goto lab45 ;
    if ( ( strstart [k + 1 ]- strstart [k ]) == hn ) 
    {
      j = 1 ;
      u = strstart [k ];
      do {
	  if ( strpool [u ]!= hc [j ]) 
	goto lab30 ;
	incr ( j ) ;
	incr ( u ) ;
      } while ( ! ( j > hn ) ) ;
      s = hyphlist [h ];
      while ( s != 0 ) {
	  
	hyf [mem [s ].hh .v.LH ]= 1 ;
	s = mem [s ].hh .v.RH ;
      } 
      decr ( hn ) ;
      goto lab40 ;
    } 
    lab30: ;
    h = hyphlink [h ];
    if ( h == 0 ) 
    goto lab45 ;
    decr ( h ) ;
  } 
  lab45: decr ( hn ) ;
  if ( trietrc [curlang + 1 ]!= curlang ) 
  return ;
  hc [0 ]= 0 ;
  hc [hn + 1 ]= 0 ;
  hc [hn + 2 ]= 256 ;
  {register integer for_end; j = 0 ;for_end = hn - rhyf + 1 ; if ( j <= 
  for_end) do 
    {
      z = trietrl [curlang + 1 ]+ hc [j ];
      l = j ;
      while ( hc [l ]== trietrc [z ]) {
	  
	if ( trietro [z ]!= mintrieop ) 
	{
	  v = trietro [z ];
	  do {
	      v = v + opstart [curlang ];
	    i = l - hyfdistance [v ];
	    if ( hyfnum [v ]> hyf [i ]) 
	    hyf [i ]= hyfnum [v ];
	    v = hyfnext [v ];
	  } while ( ! ( v == mintrieop ) ) ;
	} 
	incr ( l ) ;
	z = trietrl [z ]+ hc [l ];
      } 
    } 
  while ( j++ < for_end ) ;} 
  lab40: {
      register integer for_end; j = 0 ;for_end = lhyf - 1 ; if ( j <= 
  for_end) do 
    hyf [j ]= 0 ;
  while ( j++ < for_end ) ;} 
  {register integer for_end; j = 0 ;for_end = rhyf - 1 ; if ( j <= for_end) 
  do 
    hyf [hn - j ]= 0 ;
  while ( j++ < for_end ) ;} 
  {register integer for_end; j = lhyf ;for_end = hn - rhyf ; if ( j <= 
  for_end) do 
    if ( odd ( hyf [j ]) ) 
    goto lab41 ;
  while ( j++ < for_end ) ;} 
  return ;
  lab41: ;
  q = mem [hb ].hh .v.RH ;
  mem [hb ].hh .v.RH = 0 ;
  r = mem [ha ].hh .v.RH ;
  mem [ha ].hh .v.RH = 0 ;
  bchar = hyfbchar ;
  if ( ( ha >= himemmin ) ) 
  if ( mem [ha ].hh.b0 != hf ) 
  goto lab42 ;
  else {
      
    initlist = ha ;
    initlig = false ;
    hu [0 ]= mem [ha ].hh.b1 ;
  } 
  else if ( mem [ha ].hh.b0 == 6 ) 
  if ( mem [ha + 1 ].hh.b0 != hf ) 
  goto lab42 ;
  else {
      
    initlist = mem [ha + 1 ].hh .v.RH ;
    initlig = true ;
    initlft = ( mem [ha ].hh.b1 > 1 ) ;
    hu [0 ]= mem [ha + 1 ].hh.b1 ;
    if ( initlist == 0 ) 
    if ( initlft ) 
    {
      hu [0 ]= 256 ;
      initlig = false ;
    } 
    freenode ( ha , 2 ) ;
  } 
  else {
      
    if ( ! ( r >= himemmin ) ) 
    if ( mem [r ].hh.b0 == 6 ) 
    if ( mem [r ].hh.b1 > 1 ) 
    goto lab42 ;
    j = 1 ;
    s = ha ;
    initlist = 0 ;
    goto lab50 ;
  } 
  s = curp ;
  while ( mem [s ].hh .v.RH != ha ) s = mem [s ].hh .v.RH ;
  j = 0 ;
  goto lab50 ;
  lab42: s = ha ;
  j = 0 ;
  hu [0 ]= 256 ;
  initlig = false ;
  initlist = 0 ;
  lab50: flushnodelist ( r ) ;
  do {
      l = j ;
    j = reconstitute ( j , hn , bchar , hyfchar ) + 1 ;
    if ( hyphenpassed == 0 ) 
    {
      mem [s ].hh .v.RH = mem [memtop - 4 ].hh .v.RH ;
      while ( mem [s ].hh .v.RH > 0 ) s = mem [s ].hh .v.RH ;
      if ( odd ( hyf [j - 1 ]) ) 
      {
	l = j ;
	hyphenpassed = j - 1 ;
	mem [memtop - 4 ].hh .v.RH = 0 ;
      } 
    } 
    if ( hyphenpassed > 0 ) 
    do {
	r = getnode ( 2 ) ;
      mem [r ].hh .v.RH = mem [memtop - 4 ].hh .v.RH ;
      mem [r ].hh.b0 = 7 ;
      majortail = r ;
      rcount = 0 ;
      while ( mem [majortail ].hh .v.RH > 0 ) {
	  
	majortail = mem [majortail ].hh .v.RH ;
	incr ( rcount ) ;
      } 
      i = hyphenpassed ;
      hyf [i ]= 0 ;
      minortail = 0 ;
      mem [r + 1 ].hh .v.LH = 0 ;
      hyfnode = newcharacter ( hf , hyfchar ) ;
      if ( hyfnode != 0 ) 
      {
	incr ( i ) ;
	c = hu [i ];
	hu [i ]= hyfchar ;
	{
	  mem [hyfnode ].hh .v.RH = avail ;
	  avail = hyfnode ;
	;
#ifdef STAT
	  decr ( dynused ) ;
#endif /* STAT */
	} 
      } 
      while ( l <= i ) {
	  
	l = reconstitute ( l , i , fontbchar [hf ], 256 ) + 1 ;
	if ( mem [memtop - 4 ].hh .v.RH > 0 ) 
	{
	  if ( minortail == 0 ) 
	  mem [r + 1 ].hh .v.LH = mem [memtop - 4 ].hh .v.RH ;
	  else mem [minortail ].hh .v.RH = mem [memtop - 4 ].hh .v.RH ;
	  minortail = mem [memtop - 4 ].hh .v.RH ;
	  while ( mem [minortail ].hh .v.RH > 0 ) minortail = mem [
	  minortail ].hh .v.RH ;
	} 
      } 
      if ( hyfnode != 0 ) 
      {
	hu [i ]= c ;
	l = i ;
	decr ( i ) ;
      } 
      minortail = 0 ;
      mem [r + 1 ].hh .v.RH = 0 ;
      cloc = 0 ;
      if ( bcharlabel [hf ]!= 0 ) 
      {
	decr ( l ) ;
	c = hu [l ];
	cloc = l ;
	hu [l ]= 256 ;
      } 
      while ( l < j ) {
	  
	do {
	    l = reconstitute ( l , hn , bchar , 256 ) + 1 ;
	  if ( cloc > 0 ) 
	  {
	    hu [cloc ]= c ;
	    cloc = 0 ;
	  } 
	  if ( mem [memtop - 4 ].hh .v.RH > 0 ) 
	  {
	    if ( minortail == 0 ) 
	    mem [r + 1 ].hh .v.RH = mem [memtop - 4 ].hh .v.RH ;
	    else mem [minortail ].hh .v.RH = mem [memtop - 4 ].hh .v.RH ;
	    minortail = mem [memtop - 4 ].hh .v.RH ;
	    while ( mem [minortail ].hh .v.RH > 0 ) minortail = mem [
	    minortail ].hh .v.RH ;
	  } 
	} while ( ! ( l >= j ) ) ;
	while ( l > j ) {
	    
	  j = reconstitute ( j , hn , bchar , 256 ) + 1 ;
	  mem [majortail ].hh .v.RH = mem [memtop - 4 ].hh .v.RH ;
	  while ( mem [majortail ].hh .v.RH > 0 ) {
	      
	    majortail = mem [majortail ].hh .v.RH ;
	    incr ( rcount ) ;
	  } 
	} 
      } 
      if ( rcount > 127 ) 
      {
	mem [s ].hh .v.RH = mem [r ].hh .v.RH ;
	mem [r ].hh .v.RH = 0 ;
	flushnodelist ( r ) ;
      } 
      else {
	  
	mem [s ].hh .v.RH = r ;
	mem [r ].hh.b1 = rcount ;
      } 
      s = majortail ;
      hyphenpassed = j - 1 ;
      mem [memtop - 4 ].hh .v.RH = 0 ;
    } while ( ! ( ! odd ( hyf [j - 1 ]) ) ) ;
  } while ( ! ( j > hn ) ) ;
  mem [s ].hh .v.RH = q ;
  flushlist ( initlist ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
newhyphexceptions ( void ) 
#else
newhyphexceptions ( ) 
#endif
{
  /* 21 10 40 45 */ newhyphexceptions_regmem 
  char n  ;
  char j  ;
  hyphpointer h  ;
  strnumber k  ;
  halfword p  ;
  halfword q  ;
  strnumber s, t  ;
  poolpointer u, v  ;
  scanleftbrace () ;
  if ( eqtb [15213 ].cint <= 0 ) 
  curlang = 0 ;
  else if ( eqtb [15213 ].cint > 255 ) 
  curlang = 0 ;
  else curlang = eqtb [15213 ].cint ;
  n = 0 ;
  p = 0 ;
  while ( true ) {
      
    getxtoken () ;
    lab21: switch ( curcmd ) 
    {case 11 : 
    case 12 : 
    case 68 : 
      if ( curchr == 45 ) 
      {
	if ( n < 63 ) 
	{
	  q = getavail () ;
	  mem [q ].hh .v.RH = p ;
	  mem [q ].hh .v.LH = n ;
	  p = q ;
	} 
      } 
      else {
	  
	if ( eqtb [13883 + curchr ].hh .v.RH == 0 ) 
	{
	  {
	    if ( interaction == 3 ) 
	    ;
	    printnl ( 262 ) ;
	    print ( 940 ) ;
	  } 
	  {
	    helpptr = 2 ;
	    helpline [1 ]= 941 ;
	    helpline [0 ]= 942 ;
	  } 
	  error () ;
	} 
	else if ( n < 63 ) 
	{
	  incr ( n ) ;
	  hc [n ]= eqtb [13883 + curchr ].hh .v.RH ;
	} 
      } 
      break ;
    case 16 : 
      {
	scancharnum () ;
	curchr = curval ;
	curcmd = 68 ;
	goto lab21 ;
      } 
      break ;
    case 10 : 
    case 2 : 
      {
	if ( n > 1 ) 
	{
	  incr ( n ) ;
	  hc [n ]= curlang ;
	  {
	    if ( poolptr + n > poolsize ) 
	    overflow ( 257 , poolsize - initpoolptr ) ;
	  } 
	  h = 0 ;
	  {register integer for_end; j = 1 ;for_end = n ; if ( j <= for_end) 
	  do 
	    {
	      h = ( h + h + hc [j ]) % 607 ;
	      {
		strpool [poolptr ]= hc [j ];
		incr ( poolptr ) ;
	      } 
	    } 
	  while ( j++ < for_end ) ;} 
	  s = makestring () ;
	  if ( hyphnext <= 607 ) 
	  while ( ( hyphnext > 0 ) && ( hyphword [hyphnext - 1 ]> 0 ) ) decr 
	  ( hyphnext ) ;
	  if ( ( hyphcount == hyphsize ) || ( hyphnext == 0 ) ) 
	  overflow ( 943 , hyphsize ) ;
	  incr ( hyphcount ) ;
	  while ( hyphword [h ]!= 0 ) {
	      
	    k = hyphword [h ];
	    if ( ( strstart [k + 1 ]- strstart [k ]) != ( strstart [s + 1 
	    ]- strstart [s ]) ) 
	    goto lab45 ;
	    u = strstart [k ];
	    v = strstart [s ];
	    do {
		if ( strpool [u ]!= strpool [v ]) 
	      goto lab45 ;
	      incr ( u ) ;
	      incr ( v ) ;
	    } while ( ! ( u == strstart [k + 1 ]) ) ;
	    {
	      decr ( strptr ) ;
	      poolptr = strstart [strptr ];
	    } 
	    s = hyphword [h ];
	    decr ( hyphcount ) ;
	    goto lab40 ;
	    lab45: ;
	    if ( hyphlink [h ]== 0 ) 
	    {
	      hyphlink [h ]= hyphnext ;
	      if ( hyphnext >= hyphsize ) 
	      hyphnext = 607 ;
	      if ( hyphnext > 607 ) 
	      incr ( hyphnext ) ;
	    } 
	    h = hyphlink [h ]- 1 ;
	  } 
	  lab40: hyphword [h ]= s ;
	  hyphlist [h ]= p ;
	} 
	if ( curcmd == 2 ) 
	return ;
	n = 0 ;
	p = 0 ;
      } 
      break ;
      default: 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 679 ) ;
	} 
	printesc ( 936 ) ;
	print ( 937 ) ;
	{
	  helpptr = 2 ;
	  helpline [1 ]= 938 ;
	  helpline [0 ]= 939 ;
	} 
	error () ;
      } 
      break ;
    } 
  } 
} 
halfword 
#ifdef HAVE_PROTOTYPES
zprunepagetop ( halfword p ) 
#else
zprunepagetop ( p ) 
  halfword p ;
#endif
{
  register halfword Result; prunepagetop_regmem 
  halfword prevp  ;
  halfword q  ;
  prevp = memtop - 3 ;
  mem [memtop - 3 ].hh .v.RH = p ;
  while ( p != 0 ) switch ( mem [p ].hh.b0 ) 
  {case 0 : 
  case 1 : 
  case 2 : 
    {
      q = newskipparam ( 10 ) ;
      mem [prevp ].hh .v.RH = q ;
      mem [q ].hh .v.RH = p ;
      if ( mem [tempptr + 1 ].cint > mem [p + 3 ].cint ) 
      mem [tempptr + 1 ].cint = mem [tempptr + 1 ].cint - mem [p + 3 ]
      .cint ;
      else mem [tempptr + 1 ].cint = 0 ;
      p = 0 ;
    } 
    break ;
  case 8 : 
  case 4 : 
  case 3 : 
    {
      prevp = p ;
      p = mem [prevp ].hh .v.RH ;
    } 
    break ;
  case 10 : 
  case 11 : 
  case 12 : 
    {
      q = p ;
      p = mem [q ].hh .v.RH ;
      mem [q ].hh .v.RH = 0 ;
      mem [prevp ].hh .v.RH = p ;
      flushnodelist ( q ) ;
    } 
    break ;
    default: 
    confusion ( 954 ) ;
    break ;
  } 
  Result = mem [memtop - 3 ].hh .v.RH ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zvertbreak ( halfword p , scaled h , scaled d ) 
#else
zvertbreak ( p , h , d ) 
  halfword p ;
  scaled h ;
  scaled d ;
#endif
{
  /* 30 45 90 */ register halfword Result; vertbreak_regmem 
  halfword prevp  ;
  halfword q, r  ;
  integer pi  ;
  integer b  ;
  integer leastcost  ;
  halfword bestplace  ;
  scaled prevdp  ;
  smallnumber t  ;
  prevp = p ;
  leastcost = 1073741823L ;
  activewidth [1 ]= 0 ;
  activewidth [2 ]= 0 ;
  activewidth [3 ]= 0 ;
  activewidth [4 ]= 0 ;
  activewidth [5 ]= 0 ;
  activewidth [6 ]= 0 ;
  prevdp = 0 ;
  while ( true ) {
      
    if ( p == 0 ) 
    pi = -10000 ;
    else switch ( mem [p ].hh.b0 ) 
    {case 0 : 
    case 1 : 
    case 2 : 
      {
	activewidth [1 ]= activewidth [1 ]+ prevdp + mem [p + 3 ].cint ;
	prevdp = mem [p + 2 ].cint ;
	goto lab45 ;
      } 
      break ;
    case 8 : 
      goto lab45 ;
      break ;
    case 10 : 
      if ( ( mem [prevp ].hh.b0 < 9 ) ) 
      pi = 0 ;
      else goto lab90 ;
      break ;
    case 11 : 
      {
	if ( mem [p ].hh .v.RH == 0 ) 
	t = 12 ;
	else t = mem [mem [p ].hh .v.RH ].hh.b0 ;
	if ( t == 10 ) 
	pi = 0 ;
	else goto lab90 ;
      } 
      break ;
    case 12 : 
      pi = mem [p + 1 ].cint ;
      break ;
    case 4 : 
    case 3 : 
      goto lab45 ;
      break ;
      default: 
      confusion ( 955 ) ;
      break ;
    } 
    if ( pi < 10000 ) 
    {
      if ( activewidth [1 ]< h ) 
      if ( ( activewidth [3 ]!= 0 ) || ( activewidth [4 ]!= 0 ) || ( 
      activewidth [5 ]!= 0 ) ) 
      b = 0 ;
      else b = badness ( h - activewidth [1 ], activewidth [2 ]) ;
      else if ( activewidth [1 ]- h > activewidth [6 ]) 
      b = 1073741823L ;
      else b = badness ( activewidth [1 ]- h , activewidth [6 ]) ;
      if ( b < 1073741823L ) 
      if ( pi <= -10000 ) 
      b = pi ;
      else if ( b < 10000 ) 
      b = b + pi ;
      else b = 100000L ;
      if ( b <= leastcost ) 
      {
	bestplace = p ;
	leastcost = b ;
	bestheightplusdepth = activewidth [1 ]+ prevdp ;
      } 
      if ( ( b == 1073741823L ) || ( pi <= -10000 ) ) 
      goto lab30 ;
    } 
    if ( ( mem [p ].hh.b0 < 10 ) || ( mem [p ].hh.b0 > 11 ) ) 
    goto lab45 ;
    lab90: if ( mem [p ].hh.b0 == 11 ) 
    q = p ;
    else {
	
      q = mem [p + 1 ].hh .v.LH ;
      activewidth [2 + mem [q ].hh.b0 ]= activewidth [2 + mem [q ]
      .hh.b0 ]+ mem [q + 2 ].cint ;
      activewidth [6 ]= activewidth [6 ]+ mem [q + 3 ].cint ;
      if ( ( mem [q ].hh.b1 != 0 ) && ( mem [q + 3 ].cint != 0 ) ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 956 ) ;
	} 
	{
	  helpptr = 4 ;
	  helpline [3 ]= 957 ;
	  helpline [2 ]= 958 ;
	  helpline [1 ]= 959 ;
	  helpline [0 ]= 917 ;
	} 
	error () ;
	r = newspec ( q ) ;
	mem [r ].hh.b1 = 0 ;
	deleteglueref ( q ) ;
	mem [p + 1 ].hh .v.LH = r ;
	q = r ;
      } 
    } 
    activewidth [1 ]= activewidth [1 ]+ prevdp + mem [q + 1 ].cint ;
    prevdp = 0 ;
    lab45: if ( prevdp > d ) 
    {
      activewidth [1 ]= activewidth [1 ]+ prevdp - d ;
      prevdp = d ;
    } 
    prevp = p ;
    p = mem [prevp ].hh .v.RH ;
  } 
  lab30: Result = bestplace ;
  return Result ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zvsplit ( eightbits n , scaled h ) 
#else
zvsplit ( n , h ) 
  eightbits n ;
  scaled h ;
#endif
{
  /* 10 30 */ register halfword Result; vsplit_regmem 
  halfword v  ;
  halfword p  ;
  halfword q  ;
  v = eqtb [13322 + n ].hh .v.RH ;
  if ( curmark [3 ]!= 0 ) 
  {
    deletetokenref ( curmark [3 ]) ;
    curmark [3 ]= 0 ;
    deletetokenref ( curmark [4 ]) ;
    curmark [4 ]= 0 ;
  } 
  if ( v == 0 ) 
  {
    Result = 0 ;
    return Result ;
  } 
  if ( mem [v ].hh.b0 != 1 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 335 ) ;
    } 
    printesc ( 960 ) ;
    print ( 961 ) ;
    printesc ( 962 ) ;
    {
      helpptr = 2 ;
      helpline [1 ]= 963 ;
      helpline [0 ]= 964 ;
    } 
    error () ;
    Result = 0 ;
    return Result ;
  } 
  q = vertbreak ( mem [v + 5 ].hh .v.RH , h , eqtb [15739 ].cint ) ;
  p = mem [v + 5 ].hh .v.RH ;
  if ( p == q ) 
  mem [v + 5 ].hh .v.RH = 0 ;
  else while ( true ) {
      
    if ( mem [p ].hh.b0 == 4 ) 
    if ( curmark [3 ]== 0 ) 
    {
      curmark [3 ]= mem [p + 1 ].cint ;
      curmark [4 ]= curmark [3 ];
      mem [curmark [3 ]].hh .v.LH = mem [curmark [3 ]].hh .v.LH + 2 ;
    } 
    else {
	
      deletetokenref ( curmark [4 ]) ;
      curmark [4 ]= mem [p + 1 ].cint ;
      incr ( mem [curmark [4 ]].hh .v.LH ) ;
    } 
    if ( mem [p ].hh .v.RH == q ) 
    {
      mem [p ].hh .v.RH = 0 ;
      goto lab30 ;
    } 
    p = mem [p ].hh .v.RH ;
  } 
  lab30: ;
  q = prunepagetop ( q ) ;
  p = mem [v + 5 ].hh .v.RH ;
  freenode ( v , 7 ) ;
  if ( q == 0 ) 
  eqtb [13322 + n ].hh .v.RH = 0 ;
  else eqtb [13322 + n ].hh .v.RH = vpackage ( q , 0 , 1 , 1073741823L ) ;
  Result = vpackage ( p , h , 0 , eqtb [15739 ].cint ) ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
printtotals ( void ) 
#else
printtotals ( ) 
#endif
{
  printtotals_regmem 
  printscaled ( pagesofar [1 ]) ;
  if ( pagesofar [2 ]!= 0 ) 
  {
    print ( 310 ) ;
    printscaled ( pagesofar [2 ]) ;
    print ( 335 ) ;
  } 
  if ( pagesofar [3 ]!= 0 ) 
  {
    print ( 310 ) ;
    printscaled ( pagesofar [3 ]) ;
    print ( 309 ) ;
  } 
  if ( pagesofar [4 ]!= 0 ) 
  {
    print ( 310 ) ;
    printscaled ( pagesofar [4 ]) ;
    print ( 973 ) ;
  } 
  if ( pagesofar [5 ]!= 0 ) 
  {
    print ( 310 ) ;
    printscaled ( pagesofar [5 ]) ;
    print ( 974 ) ;
  } 
  if ( pagesofar [6 ]!= 0 ) 
  {
    print ( 311 ) ;
    printscaled ( pagesofar [6 ]) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zfreezepagespecs ( smallnumber s ) 
#else
zfreezepagespecs ( s ) 
  smallnumber s ;
#endif
{
  freezepagespecs_regmem 
  pagecontents = s ;
  pagesofar [0 ]= eqtb [15737 ].cint ;
  pagemaxdepth = eqtb [15738 ].cint ;
  pagesofar [7 ]= 0 ;
  pagesofar [1 ]= 0 ;
  pagesofar [2 ]= 0 ;
  pagesofar [3 ]= 0 ;
  pagesofar [4 ]= 0 ;
  pagesofar [5 ]= 0 ;
  pagesofar [6 ]= 0 ;
  leastpagecost = 1073741823L ;
	;
#ifdef STAT
  if ( eqtb [15196 ].cint > 0 ) 
  {
    begindiagnostic () ;
    printnl ( 982 ) ;
    printscaled ( pagesofar [0 ]) ;
    print ( 983 ) ;
    printscaled ( pagemaxdepth ) ;
    enddiagnostic ( false ) ;
  } 
#endif /* STAT */
} 
void 
#ifdef HAVE_PROTOTYPES
zboxerror ( eightbits n ) 
#else
zboxerror ( n ) 
  eightbits n ;
#endif
{
  boxerror_regmem 
  error () ;
  begindiagnostic () ;
  printnl ( 831 ) ;
  showbox ( eqtb [13322 + n ].hh .v.RH ) ;
  enddiagnostic ( true ) ;
  flushnodelist ( eqtb [13322 + n ].hh .v.RH ) ;
  eqtb [13322 + n ].hh .v.RH = 0 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zensurevbox ( eightbits n ) 
#else
zensurevbox ( n ) 
  eightbits n ;
#endif
{
  ensurevbox_regmem 
  halfword p  ;
  p = eqtb [13322 + n ].hh .v.RH ;
  if ( p != 0 ) 
  if ( mem [p ].hh.b0 == 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 984 ) ;
    } 
    {
      helpptr = 3 ;
      helpline [2 ]= 985 ;
      helpline [1 ]= 986 ;
      helpline [0 ]= 987 ;
    } 
    boxerror ( n ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zfireup ( halfword c ) 
#else
zfireup ( c ) 
  halfword c ;
#endif
{
  /* 10 */ fireup_regmem 
  halfword p, q, r, s  ;
  halfword prevp  ;
  unsigned char n  ;
  boolean wait  ;
  integer savevbadness  ;
  scaled savevfuzz  ;
  halfword savesplittopskip  ;
  if ( mem [bestpagebreak ].hh.b0 == 12 ) 
  {
    geqworddefine ( 15202 , mem [bestpagebreak + 1 ].cint ) ;
    mem [bestpagebreak + 1 ].cint = 10000 ;
  } 
  else geqworddefine ( 15202 , 10000 ) ;
  if ( curmark [2 ]!= 0 ) 
  {
    if ( curmark [0 ]!= 0 ) 
    deletetokenref ( curmark [0 ]) ;
    curmark [0 ]= curmark [2 ];
    incr ( mem [curmark [0 ]].hh .v.LH ) ;
    deletetokenref ( curmark [1 ]) ;
    curmark [1 ]= 0 ;
  } 
  if ( c == bestpagebreak ) 
  bestpagebreak = 0 ;
  if ( eqtb [13577 ].hh .v.RH != 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 335 ) ;
    } 
    printesc ( 406 ) ;
    print ( 998 ) ;
    {
      helpptr = 2 ;
      helpline [1 ]= 999 ;
      helpline [0 ]= 987 ;
    } 
    boxerror ( 255 ) ;
  } 
  insertpenalties = 0 ;
  savesplittopskip = eqtb [12536 ].hh .v.RH ;
  if ( eqtb [15216 ].cint <= 0 ) 
  {
    r = mem [memtop ].hh .v.RH ;
    while ( r != memtop ) {
	
      if ( mem [r + 2 ].hh .v.LH != 0 ) 
      {
	n = mem [r ].hh.b1 ;
	ensurevbox ( n ) ;
	if ( eqtb [13322 + n ].hh .v.RH == 0 ) 
	eqtb [13322 + n ].hh .v.RH = newnullbox () ;
	p = eqtb [13322 + n ].hh .v.RH + 5 ;
	while ( mem [p ].hh .v.RH != 0 ) p = mem [p ].hh .v.RH ;
	mem [r + 2 ].hh .v.RH = p ;
      } 
      r = mem [r ].hh .v.RH ;
    } 
  } 
  q = memtop - 4 ;
  mem [q ].hh .v.RH = 0 ;
  prevp = memtop - 2 ;
  p = mem [prevp ].hh .v.RH ;
  while ( p != bestpagebreak ) {
      
    if ( mem [p ].hh.b0 == 3 ) 
    {
      if ( eqtb [15216 ].cint <= 0 ) 
      {
	r = mem [memtop ].hh .v.RH ;
	while ( mem [r ].hh.b1 != mem [p ].hh.b1 ) r = mem [r ].hh .v.RH 
	;
	if ( mem [r + 2 ].hh .v.LH == 0 ) 
	wait = true ;
	else {
	    
	  wait = false ;
	  s = mem [r + 2 ].hh .v.RH ;
	  mem [s ].hh .v.RH = mem [p + 4 ].hh .v.LH ;
	  if ( mem [r + 2 ].hh .v.LH == p ) 
	  {
	    if ( mem [r ].hh.b0 == 1 ) 
	    if ( ( mem [r + 1 ].hh .v.LH == p ) && ( mem [r + 1 ].hh .v.RH 
	    != 0 ) ) 
	    {
	      while ( mem [s ].hh .v.RH != mem [r + 1 ].hh .v.RH ) s = mem 
	      [s ].hh .v.RH ;
	      mem [s ].hh .v.RH = 0 ;
	      eqtb [12536 ].hh .v.RH = mem [p + 4 ].hh .v.RH ;
	      mem [p + 4 ].hh .v.LH = prunepagetop ( mem [r + 1 ].hh .v.RH 
	      ) ;
	      if ( mem [p + 4 ].hh .v.LH != 0 ) 
	      {
		tempptr = vpackage ( mem [p + 4 ].hh .v.LH , 0 , 1 , 
		1073741823L ) ;
		mem [p + 3 ].cint = mem [tempptr + 3 ].cint + mem [
		tempptr + 2 ].cint ;
		freenode ( tempptr , 7 ) ;
		wait = true ;
	      } 
	    } 
	    mem [r + 2 ].hh .v.LH = 0 ;
	    n = mem [r ].hh.b1 ;
	    tempptr = mem [eqtb [13322 + n ].hh .v.RH + 5 ].hh .v.RH ;
	    freenode ( eqtb [13322 + n ].hh .v.RH , 7 ) ;
	    eqtb [13322 + n ].hh .v.RH = vpackage ( tempptr , 0 , 1 , 
	    1073741823L ) ;
	  } 
	  else {
	      
	    while ( mem [s ].hh .v.RH != 0 ) s = mem [s ].hh .v.RH ;
	    mem [r + 2 ].hh .v.RH = s ;
	  } 
	} 
	mem [prevp ].hh .v.RH = mem [p ].hh .v.RH ;
	mem [p ].hh .v.RH = 0 ;
	if ( wait ) 
	{
	  mem [q ].hh .v.RH = p ;
	  q = p ;
	  incr ( insertpenalties ) ;
	} 
	else {
	    
	  deleteglueref ( mem [p + 4 ].hh .v.RH ) ;
	  freenode ( p , 5 ) ;
	} 
	p = prevp ;
      } 
    } 
    else if ( mem [p ].hh.b0 == 4 ) 
    {
      if ( curmark [1 ]== 0 ) 
      {
	curmark [1 ]= mem [p + 1 ].cint ;
	incr ( mem [curmark [1 ]].hh .v.LH ) ;
      } 
      if ( curmark [2 ]!= 0 ) 
      deletetokenref ( curmark [2 ]) ;
      curmark [2 ]= mem [p + 1 ].cint ;
      incr ( mem [curmark [2 ]].hh .v.LH ) ;
    } 
    prevp = p ;
    p = mem [prevp ].hh .v.RH ;
  } 
  eqtb [12536 ].hh .v.RH = savesplittopskip ;
  if ( p != 0 ) 
  {
    if ( mem [memtop - 1 ].hh .v.RH == 0 ) 
    if ( nestptr == 0 ) 
    curlist .tailfield = pagetail ;
    else nest [0 ].tailfield = pagetail ;
    mem [pagetail ].hh .v.RH = mem [memtop - 1 ].hh .v.RH ;
    mem [memtop - 1 ].hh .v.RH = p ;
    mem [prevp ].hh .v.RH = 0 ;
  } 
  savevbadness = eqtb [15190 ].cint ;
  eqtb [15190 ].cint = 10000 ;
  savevfuzz = eqtb [15742 ].cint ;
  eqtb [15742 ].cint = 1073741823L ;
  eqtb [13577 ].hh .v.RH = vpackage ( mem [memtop - 2 ].hh .v.RH , 
  bestsize , 0 , pagemaxdepth ) ;
  eqtb [15190 ].cint = savevbadness ;
  eqtb [15742 ].cint = savevfuzz ;
  if ( lastglue != 268435455L ) 
  deleteglueref ( lastglue ) ;
  pagecontents = 0 ;
  pagetail = memtop - 2 ;
  mem [memtop - 2 ].hh .v.RH = 0 ;
  lastglue = 268435455L ;
  lastpenalty = 0 ;
  lastkern = 0 ;
  pagesofar [7 ]= 0 ;
  pagemaxdepth = 0 ;
  if ( q != memtop - 4 ) 
  {
    mem [memtop - 2 ].hh .v.RH = mem [memtop - 4 ].hh .v.RH ;
    pagetail = q ;
  } 
  r = mem [memtop ].hh .v.RH ;
  while ( r != memtop ) {
      
    q = mem [r ].hh .v.RH ;
    freenode ( r , 4 ) ;
    r = q ;
  } 
  mem [memtop ].hh .v.RH = memtop ;
  if ( ( curmark [0 ]!= 0 ) && ( curmark [1 ]== 0 ) ) 
  {
    curmark [1 ]= curmark [0 ];
    incr ( mem [curmark [0 ]].hh .v.LH ) ;
  } 
  if ( eqtb [13057 ].hh .v.RH != 0 ) 
  if ( deadcycles >= eqtb [15203 ].cint ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 1000 ) ;
    } 
    printint ( deadcycles ) ;
    print ( 1001 ) ;
    {
      helpptr = 3 ;
      helpline [2 ]= 1002 ;
      helpline [1 ]= 1003 ;
      helpline [0 ]= 1004 ;
    } 
    error () ;
  } 
  else {
      
    outputactive = true ;
    incr ( deadcycles ) ;
    pushnest () ;
    curlist .modefield = -1 ;
    curlist .auxfield .cint = -65536000L ;
    curlist .mlfield = - (integer) line ;
    begintokenlist ( eqtb [13057 ].hh .v.RH , 6 ) ;
    newsavelevel ( 8 ) ;
    normalparagraph () ;
    scanleftbrace () ;
    return ;
  } 
  {
    if ( mem [memtop - 2 ].hh .v.RH != 0 ) 
    {
      if ( mem [memtop - 1 ].hh .v.RH == 0 ) 
      if ( nestptr == 0 ) 
      curlist .tailfield = pagetail ;
      else nest [0 ].tailfield = pagetail ;
      else mem [pagetail ].hh .v.RH = mem [memtop - 1 ].hh .v.RH ;
      mem [memtop - 1 ].hh .v.RH = mem [memtop - 2 ].hh .v.RH ;
      mem [memtop - 2 ].hh .v.RH = 0 ;
      pagetail = memtop - 2 ;
    } 
    shipout ( eqtb [13577 ].hh .v.RH ) ;
    eqtb [13577 ].hh .v.RH = 0 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
buildpage ( void ) 
#else
buildpage ( ) 
#endif
{
  /* 10 30 31 22 80 90 */ buildpage_regmem 
  halfword p  ;
  halfword q, r  ;
  integer b, c  ;
  integer pi  ;
  unsigned char n  ;
  scaled delta, h, w  ;
  if ( ( mem [memtop - 1 ].hh .v.RH == 0 ) || outputactive ) 
  return ;
  do {
      lab22: p = mem [memtop - 1 ].hh .v.RH ;
    if ( lastglue != 268435455L ) 
    deleteglueref ( lastglue ) ;
    lastpenalty = 0 ;
    lastkern = 0 ;
    if ( mem [p ].hh.b0 == 10 ) 
    {
      lastglue = mem [p + 1 ].hh .v.LH ;
      incr ( mem [lastglue ].hh .v.RH ) ;
    } 
    else {
	
      lastglue = 268435455L ;
      if ( mem [p ].hh.b0 == 12 ) 
      lastpenalty = mem [p + 1 ].cint ;
      else if ( mem [p ].hh.b0 == 11 ) 
      lastkern = mem [p + 1 ].cint ;
    } 
    switch ( mem [p ].hh.b0 ) 
    {case 0 : 
    case 1 : 
    case 2 : 
      if ( pagecontents < 2 ) 
      {
	if ( pagecontents == 0 ) 
	freezepagespecs ( 2 ) ;
	else pagecontents = 2 ;
	q = newskipparam ( 9 ) ;
	if ( mem [tempptr + 1 ].cint > mem [p + 3 ].cint ) 
	mem [tempptr + 1 ].cint = mem [tempptr + 1 ].cint - mem [p + 3 ]
	.cint ;
	else mem [tempptr + 1 ].cint = 0 ;
	mem [q ].hh .v.RH = p ;
	mem [memtop - 1 ].hh .v.RH = q ;
	goto lab22 ;
      } 
      else {
	  
	pagesofar [1 ]= pagesofar [1 ]+ pagesofar [7 ]+ mem [p + 3 ]
	.cint ;
	pagesofar [7 ]= mem [p + 2 ].cint ;
	goto lab80 ;
      } 
      break ;
    case 8 : 
      goto lab80 ;
      break ;
    case 10 : 
      if ( pagecontents < 2 ) 
      goto lab31 ;
      else if ( ( mem [pagetail ].hh.b0 < 9 ) ) 
      pi = 0 ;
      else goto lab90 ;
      break ;
    case 11 : 
      if ( pagecontents < 2 ) 
      goto lab31 ;
      else if ( mem [p ].hh .v.RH == 0 ) 
      return ;
      else if ( mem [mem [p ].hh .v.RH ].hh.b0 == 10 ) 
      pi = 0 ;
      else goto lab90 ;
      break ;
    case 12 : 
      if ( pagecontents < 2 ) 
      goto lab31 ;
      else pi = mem [p + 1 ].cint ;
      break ;
    case 4 : 
      goto lab80 ;
      break ;
    case 3 : 
      {
	if ( pagecontents == 0 ) 
	freezepagespecs ( 1 ) ;
	n = mem [p ].hh.b1 ;
	r = memtop ;
	while ( n >= mem [mem [r ].hh .v.RH ].hh.b1 ) r = mem [r ].hh 
	.v.RH ;
	n = n ;
	if ( mem [r ].hh.b1 != n ) 
	{
	  q = getnode ( 4 ) ;
	  mem [q ].hh .v.RH = mem [r ].hh .v.RH ;
	  mem [r ].hh .v.RH = q ;
	  r = q ;
	  mem [r ].hh.b1 = n ;
	  mem [r ].hh.b0 = 0 ;
	  ensurevbox ( n ) ;
	  if ( eqtb [13322 + n ].hh .v.RH == 0 ) 
	  mem [r + 3 ].cint = 0 ;
	  else mem [r + 3 ].cint = mem [eqtb [13322 + n ].hh .v.RH + 3 ]
	  .cint + mem [eqtb [13322 + n ].hh .v.RH + 2 ].cint ;
	  mem [r + 2 ].hh .v.LH = 0 ;
	  q = eqtb [12544 + n ].hh .v.RH ;
	  if ( eqtb [15221 + n ].cint == 1000 ) 
	  h = mem [r + 3 ].cint ;
	  else h = xovern ( mem [r + 3 ].cint , 1000 ) * eqtb [15221 + n ]
	  .cint ;
	  pagesofar [0 ]= pagesofar [0 ]- h - mem [q + 1 ].cint ;
	  pagesofar [2 + mem [q ].hh.b0 ]= pagesofar [2 + mem [q ]
	  .hh.b0 ]+ mem [q + 2 ].cint ;
	  pagesofar [6 ]= pagesofar [6 ]+ mem [q + 3 ].cint ;
	  if ( ( mem [q ].hh.b1 != 0 ) && ( mem [q + 3 ].cint != 0 ) ) 
	  {
	    {
	      if ( interaction == 3 ) 
	      ;
	      printnl ( 262 ) ;
	      print ( 993 ) ;
	    } 
	    printesc ( 392 ) ;
	    printint ( n ) ;
	    {
	      helpptr = 3 ;
	      helpline [2 ]= 994 ;
	      helpline [1 ]= 995 ;
	      helpline [0 ]= 917 ;
	    } 
	    error () ;
	  } 
	} 
	if ( mem [r ].hh.b0 == 1 ) 
	insertpenalties = insertpenalties + mem [p + 1 ].cint ;
	else {
	    
	  mem [r + 2 ].hh .v.RH = p ;
	  delta = pagesofar [0 ]- pagesofar [1 ]- pagesofar [7 ]+ 
	  pagesofar [6 ];
	  if ( eqtb [15221 + n ].cint == 1000 ) 
	  h = mem [p + 3 ].cint ;
	  else h = xovern ( mem [p + 3 ].cint , 1000 ) * eqtb [15221 + n ]
	  .cint ;
	  if ( ( ( h <= 0 ) || ( h <= delta ) ) && ( mem [p + 3 ].cint + mem 
	  [r + 3 ].cint <= eqtb [15754 + n ].cint ) ) 
	  {
	    pagesofar [0 ]= pagesofar [0 ]- h ;
	    mem [r + 3 ].cint = mem [r + 3 ].cint + mem [p + 3 ].cint ;
	  } 
	  else {
	      
	    if ( eqtb [15221 + n ].cint <= 0 ) 
	    w = 1073741823L ;
	    else {
		
	      w = pagesofar [0 ]- pagesofar [1 ]- pagesofar [7 ];
	      if ( eqtb [15221 + n ].cint != 1000 ) 
	      w = xovern ( w , eqtb [15221 + n ].cint ) * 1000 ;
	    } 
	    if ( w > eqtb [15754 + n ].cint - mem [r + 3 ].cint ) 
	    w = eqtb [15754 + n ].cint - mem [r + 3 ].cint ;
	    q = vertbreak ( mem [p + 4 ].hh .v.LH , w , mem [p + 2 ].cint 
	    ) ;
	    mem [r + 3 ].cint = mem [r + 3 ].cint + bestheightplusdepth ;
	;
#ifdef STAT
	    if ( eqtb [15196 ].cint > 0 ) 
	    {
	      begindiagnostic () ;
	      printnl ( 996 ) ;
	      printint ( n ) ;
	      print ( 997 ) ;
	      printscaled ( w ) ;
	      printchar ( 44 ) ;
	      printscaled ( bestheightplusdepth ) ;
	      print ( 926 ) ;
	      if ( q == 0 ) 
	      printint ( -10000 ) ;
	      else if ( mem [q ].hh.b0 == 12 ) 
	      printint ( mem [q + 1 ].cint ) ;
	      else printchar ( 48 ) ;
	      enddiagnostic ( false ) ;
	    } 
#endif /* STAT */
	    if ( eqtb [15221 + n ].cint != 1000 ) 
	    bestheightplusdepth = xovern ( bestheightplusdepth , 1000 ) * eqtb 
	    [15221 + n ].cint ;
	    pagesofar [0 ]= pagesofar [0 ]- bestheightplusdepth ;
	    mem [r ].hh.b0 = 1 ;
	    mem [r + 1 ].hh .v.RH = q ;
	    mem [r + 1 ].hh .v.LH = p ;
	    if ( q == 0 ) 
	    insertpenalties = insertpenalties - 10000 ;
	    else if ( mem [q ].hh.b0 == 12 ) 
	    insertpenalties = insertpenalties + mem [q + 1 ].cint ;
	  } 
	} 
	goto lab80 ;
      } 
      break ;
      default: 
      confusion ( 988 ) ;
      break ;
    } 
    if ( pi < 10000 ) 
    {
      if ( pagesofar [1 ]< pagesofar [0 ]) 
      if ( ( pagesofar [3 ]!= 0 ) || ( pagesofar [4 ]!= 0 ) || ( pagesofar 
      [5 ]!= 0 ) ) 
      b = 0 ;
      else b = badness ( pagesofar [0 ]- pagesofar [1 ], pagesofar [2 ]) 
      ;
      else if ( pagesofar [1 ]- pagesofar [0 ]> pagesofar [6 ]) 
      b = 1073741823L ;
      else b = badness ( pagesofar [1 ]- pagesofar [0 ], pagesofar [6 ]) 
      ;
      if ( b < 1073741823L ) 
      if ( pi <= -10000 ) 
      c = pi ;
      else if ( b < 10000 ) 
      c = b + pi + insertpenalties ;
      else c = 100000L ;
      else c = b ;
      if ( insertpenalties >= 10000 ) 
      c = 1073741823L ;
	;
#ifdef STAT
      if ( eqtb [15196 ].cint > 0 ) 
      {
	begindiagnostic () ;
	printnl ( 37 ) ;
	print ( 922 ) ;
	printtotals () ;
	print ( 991 ) ;
	printscaled ( pagesofar [0 ]) ;
	print ( 925 ) ;
	if ( b == 1073741823L ) 
	printchar ( 42 ) ;
	else printint ( b ) ;
	print ( 926 ) ;
	printint ( pi ) ;
	print ( 992 ) ;
	if ( c == 1073741823L ) 
	printchar ( 42 ) ;
	else printint ( c ) ;
	if ( c <= leastpagecost ) 
	printchar ( 35 ) ;
	enddiagnostic ( false ) ;
      } 
#endif /* STAT */
      if ( c <= leastpagecost ) 
      {
	bestpagebreak = p ;
	bestsize = pagesofar [0 ];
	leastpagecost = c ;
	r = mem [memtop ].hh .v.RH ;
	while ( r != memtop ) {
	    
	  mem [r + 2 ].hh .v.LH = mem [r + 2 ].hh .v.RH ;
	  r = mem [r ].hh .v.RH ;
	} 
      } 
      if ( ( c == 1073741823L ) || ( pi <= -10000 ) ) 
      {
	fireup ( p ) ;
	if ( outputactive ) 
	return ;
	goto lab30 ;
      } 
    } 
    if ( ( mem [p ].hh.b0 < 10 ) || ( mem [p ].hh.b0 > 11 ) ) 
    goto lab80 ;
    lab90: if ( mem [p ].hh.b0 == 11 ) 
    q = p ;
    else {
	
      q = mem [p + 1 ].hh .v.LH ;
      pagesofar [2 + mem [q ].hh.b0 ]= pagesofar [2 + mem [q ].hh.b0 ]
      + mem [q + 2 ].cint ;
      pagesofar [6 ]= pagesofar [6 ]+ mem [q + 3 ].cint ;
      if ( ( mem [q ].hh.b1 != 0 ) && ( mem [q + 3 ].cint != 0 ) ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 989 ) ;
	} 
	{
	  helpptr = 4 ;
	  helpline [3 ]= 990 ;
	  helpline [2 ]= 958 ;
	  helpline [1 ]= 959 ;
	  helpline [0 ]= 917 ;
	} 
	error () ;
	r = newspec ( q ) ;
	mem [r ].hh.b1 = 0 ;
	deleteglueref ( q ) ;
	mem [p + 1 ].hh .v.LH = r ;
	q = r ;
      } 
    } 
    pagesofar [1 ]= pagesofar [1 ]+ pagesofar [7 ]+ mem [q + 1 ].cint 
    ;
    pagesofar [7 ]= 0 ;
    lab80: if ( pagesofar [7 ]> pagemaxdepth ) 
    {
      pagesofar [1 ]= pagesofar [1 ]+ pagesofar [7 ]- pagemaxdepth ;
      pagesofar [7 ]= pagemaxdepth ;
    } 
    mem [pagetail ].hh .v.RH = p ;
    pagetail = p ;
    mem [memtop - 1 ].hh .v.RH = mem [p ].hh .v.RH ;
    mem [p ].hh .v.RH = 0 ;
    goto lab30 ;
    lab31: mem [memtop - 1 ].hh .v.RH = mem [p ].hh .v.RH ;
    mem [p ].hh .v.RH = 0 ;
    flushnodelist ( p ) ;
    lab30: ;
  } while ( ! ( mem [memtop - 1 ].hh .v.RH == 0 ) ) ;
  if ( nestptr == 0 ) 
  curlist .tailfield = memtop - 1 ;
  else nest [0 ].tailfield = memtop - 1 ;
} 
void 
#ifdef HAVE_PROTOTYPES
appspace ( void ) 
#else
appspace ( ) 
#endif
{
  appspace_regmem 
  halfword q  ;
  if ( ( curlist .auxfield .hh .v.LH >= 2000 ) && ( eqtb [12539 ].hh .v.RH 
  != membot ) ) 
  q = newparamglue ( 13 ) ;
  else {
      
    if ( eqtb [12538 ].hh .v.RH != membot ) 
    mainp = eqtb [12538 ].hh .v.RH ;
    else {
	
      mainp = fontglue [eqtb [13578 ].hh .v.RH ];
      if ( mainp == 0 ) 
      {
	mainp = newspec ( membot ) ;
	maink = parambase [eqtb [13578 ].hh .v.RH ]+ 2 ;
	mem [mainp + 1 ].cint = fontinfo [maink ].cint ;
	mem [mainp + 2 ].cint = fontinfo [maink + 1 ].cint ;
	mem [mainp + 3 ].cint = fontinfo [maink + 2 ].cint ;
	fontglue [eqtb [13578 ].hh .v.RH ]= mainp ;
      } 
    } 
    mainp = newspec ( mainp ) ;
    if ( curlist .auxfield .hh .v.LH >= 2000 ) 
    mem [mainp + 1 ].cint = mem [mainp + 1 ].cint + fontinfo [7 + 
    parambase [eqtb [13578 ].hh .v.RH ]].cint ;
    mem [mainp + 2 ].cint = xnoverd ( mem [mainp + 2 ].cint , curlist 
    .auxfield .hh .v.LH , 1000 ) ;
    mem [mainp + 3 ].cint = xnoverd ( mem [mainp + 3 ].cint , 1000 , 
    curlist .auxfield .hh .v.LH ) ;
    q = newglue ( mainp ) ;
    mem [mainp ].hh .v.RH = 0 ;
  } 
  mem [curlist .tailfield ].hh .v.RH = q ;
  curlist .tailfield = q ;
} 
void 
#ifdef HAVE_PROTOTYPES
insertdollarsign ( void ) 
#else
insertdollarsign ( ) 
#endif
{
  insertdollarsign_regmem 
  backinput () ;
  curtok = 804 ;
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 262 ) ;
    print ( 1012 ) ;
  } 
  {
    helpptr = 2 ;
    helpline [1 ]= 1013 ;
    helpline [0 ]= 1014 ;
  } 
  inserror () ;
} 
void 
#ifdef HAVE_PROTOTYPES
youcant ( void ) 
#else
youcant ( ) 
#endif
{
  youcant_regmem 
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 262 ) ;
    print ( 684 ) ;
  } 
  printcmdchr ( curcmd , curchr ) ;
  print ( 1015 ) ;
  printmode ( curlist .modefield ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
reportillegalcase ( void ) 
#else
reportillegalcase ( ) 
#endif
{
  reportillegalcase_regmem 
  youcant () ;
  {
    helpptr = 4 ;
    helpline [3 ]= 1016 ;
    helpline [2 ]= 1017 ;
    helpline [1 ]= 1018 ;
    helpline [0 ]= 1019 ;
  } 
  error () ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
privileged ( void ) 
#else
privileged ( ) 
#endif
{
  register boolean Result; privileged_regmem 
  if ( curlist .modefield > 0 ) 
  Result = true ;
  else {
      
    reportillegalcase () ;
    Result = false ;
  } 
  return Result ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
itsallover ( void ) 
#else
itsallover ( ) 
#endif
{
  /* 10 */ register boolean Result; itsallover_regmem 
  if ( privileged () ) 
  {
    if ( ( memtop - 2 == pagetail ) && ( curlist .headfield == curlist 
    .tailfield ) && ( deadcycles == 0 ) ) 
    {
      Result = true ;
      return Result ;
    } 
    backinput () ;
    {
      mem [curlist .tailfield ].hh .v.RH = newnullbox () ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
    mem [curlist .tailfield + 1 ].cint = eqtb [15736 ].cint ;
    {
      mem [curlist .tailfield ].hh .v.RH = newglue ( membot + 8 ) ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
    {
      mem [curlist .tailfield ].hh .v.RH = newpenalty ( -1073741824L ) ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
    buildpage () ;
  } 
  Result = false ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
appendglue ( void ) 
#else
appendglue ( ) 
#endif
{
  appendglue_regmem 
  smallnumber s  ;
  s = curchr ;
  switch ( s ) 
  {case 0 : 
    curval = membot + 4 ;
    break ;
  case 1 : 
    curval = membot + 8 ;
    break ;
  case 2 : 
    curval = membot + 12 ;
    break ;
  case 3 : 
    curval = membot + 16 ;
    break ;
  case 4 : 
    scanglue ( 2 ) ;
    break ;
  case 5 : 
    scanglue ( 3 ) ;
    break ;
  } 
  {
    mem [curlist .tailfield ].hh .v.RH = newglue ( curval ) ;
    curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
  } 
  if ( s >= 4 ) 
  {
    decr ( mem [curval ].hh .v.RH ) ;
    if ( s > 4 ) 
    mem [curlist .tailfield ].hh.b1 = 99 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
appendkern ( void ) 
#else
appendkern ( ) 
#endif
{
  appendkern_regmem 
  quarterword s  ;
  s = curchr ;
  scandimen ( s == 99 , false , false ) ;
  {
    mem [curlist .tailfield ].hh .v.RH = newkern ( curval ) ;
    curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
  } 
  mem [curlist .tailfield ].hh.b1 = s ;
} 
void 
#ifdef HAVE_PROTOTYPES
offsave ( void ) 
#else
offsave ( ) 
#endif
{
  offsave_regmem 
  halfword p  ;
  if ( curgroup == 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 775 ) ;
    } 
    printcmdchr ( curcmd , curchr ) ;
    {
      helpptr = 1 ;
      helpline [0 ]= 1038 ;
    } 
    error () ;
  } 
  else {
      
    backinput () ;
    p = getavail () ;
    mem [memtop - 3 ].hh .v.RH = p ;
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 624 ) ;
    } 
    switch ( curgroup ) 
    {case 14 : 
      {
	mem [p ].hh .v.LH = 14611 ;
	printesc ( 516 ) ;
      } 
      break ;
    case 15 : 
      {
	mem [p ].hh .v.LH = 804 ;
	printchar ( 36 ) ;
      } 
      break ;
    case 16 : 
      {
	mem [p ].hh .v.LH = 14612 ;
	mem [p ].hh .v.RH = getavail () ;
	p = mem [p ].hh .v.RH ;
	mem [p ].hh .v.LH = 3118 ;
	printesc ( 1037 ) ;
      } 
      break ;
      default: 
      {
	mem [p ].hh .v.LH = 637 ;
	printchar ( 125 ) ;
      } 
      break ;
    } 
    print ( 625 ) ;
    begintokenlist ( mem [memtop - 3 ].hh .v.RH , 4 ) ;
    {
      helpptr = 5 ;
      helpline [4 ]= 1032 ;
      helpline [3 ]= 1033 ;
      helpline [2 ]= 1034 ;
      helpline [1 ]= 1035 ;
      helpline [0 ]= 1036 ;
    } 
    error () ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
extrarightbrace ( void ) 
#else
extrarightbrace ( ) 
#endif
{
  extrarightbrace_regmem 
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 262 ) ;
    print ( 1043 ) ;
  } 
  switch ( curgroup ) 
  {case 14 : 
    printesc ( 516 ) ;
    break ;
  case 15 : 
    printchar ( 36 ) ;
    break ;
  case 16 : 
    printesc ( 872 ) ;
    break ;
  } 
  {
    helpptr = 5 ;
    helpline [4 ]= 1044 ;
    helpline [3 ]= 1045 ;
    helpline [2 ]= 1046 ;
    helpline [1 ]= 1047 ;
    helpline [0 ]= 1048 ;
  } 
  error () ;
  incr ( alignstate ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
normalparagraph ( void ) 
#else
normalparagraph ( ) 
#endif
{
  normalparagraph_regmem 
  if ( eqtb [15182 ].cint != 0 ) 
  eqworddefine ( 15182 , 0 ) ;
  if ( eqtb [15750 ].cint != 0 ) 
  eqworddefine ( 15750 , 0 ) ;
  if ( eqtb [15204 ].cint != 1 ) 
  eqworddefine ( 15204 , 1 ) ;
  if ( eqtb [13056 ].hh .v.RH != 0 ) 
  eqdefine ( 13056 , 118 , 0 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zboxend ( integer boxcontext ) 
#else
zboxend ( boxcontext ) 
  integer boxcontext ;
#endif
{
  boxend_regmem 
  halfword p  ;
  if ( boxcontext < 1073741824L ) 
  {
    if ( curbox != 0 ) 
    {
      mem [curbox + 4 ].cint = boxcontext ;
      if ( abs ( curlist .modefield ) == 1 ) 
      {
	appendtovlist ( curbox ) ;
	if ( adjusttail != 0 ) 
	{
	  if ( memtop - 5 != adjusttail ) 
	  {
	    mem [curlist .tailfield ].hh .v.RH = mem [memtop - 5 ].hh 
	    .v.RH ;
	    curlist .tailfield = adjusttail ;
	  } 
	  adjusttail = 0 ;
	} 
	if ( curlist .modefield > 0 ) 
	buildpage () ;
      } 
      else {
	  
	if ( abs ( curlist .modefield ) == 102 ) 
	curlist .auxfield .hh .v.LH = 1000 ;
	else {
	    
	  p = newnoad () ;
	  mem [p + 1 ].hh .v.RH = 2 ;
	  mem [p + 1 ].hh .v.LH = curbox ;
	  curbox = p ;
	} 
	mem [curlist .tailfield ].hh .v.RH = curbox ;
	curlist .tailfield = curbox ;
      } 
    } 
  } 
  else if ( boxcontext < 1073742336L ) 
  if ( boxcontext < 1073742080L ) 
  eqdefine ( -1073728502L + boxcontext , 119 , curbox ) ;
  else geqdefine ( -1073728758L + boxcontext , 119 , curbox ) ;
  else if ( curbox != 0 ) 
  if ( boxcontext > 1073742336L ) 
  {
    do {
	getxtoken () ;
    } while ( ! ( ( curcmd != 10 ) && ( curcmd != 0 ) ) ) ;
    if ( ( ( curcmd == 26 ) && ( abs ( curlist .modefield ) != 1 ) ) || ( ( 
    curcmd == 27 ) && ( abs ( curlist .modefield ) == 1 ) ) || ( ( curcmd == 
    28 ) && ( abs ( curlist .modefield ) == 203 ) ) ) 
    {
      appendglue () ;
      mem [curlist .tailfield ].hh.b1 = boxcontext - ( 1073742237L ) ;
      mem [curlist .tailfield + 1 ].hh .v.RH = curbox ;
    } 
    else {
	
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 1061 ) ;
      } 
      {
	helpptr = 3 ;
	helpline [2 ]= 1062 ;
	helpline [1 ]= 1063 ;
	helpline [0 ]= 1064 ;
      } 
      backerror () ;
      flushnodelist ( curbox ) ;
    } 
  } 
  else shipout ( curbox ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zbeginbox ( integer boxcontext ) 
#else
zbeginbox ( boxcontext ) 
  integer boxcontext ;
#endif
{
  /* 10 30 */ beginbox_regmem 
  halfword p, q  ;
  quarterword m  ;
  halfword k  ;
  eightbits n  ;
  switch ( curchr ) 
  {case 0 : 
    {
      scaneightbitint () ;
      curbox = eqtb [13322 + curval ].hh .v.RH ;
      eqtb [13322 + curval ].hh .v.RH = 0 ;
    } 
    break ;
  case 1 : 
    {
      scaneightbitint () ;
      curbox = copynodelist ( eqtb [13322 + curval ].hh .v.RH ) ;
    } 
    break ;
  case 2 : 
    {
      curbox = 0 ;
      if ( abs ( curlist .modefield ) == 203 ) 
      {
	youcant () ;
	{
	  helpptr = 1 ;
	  helpline [0 ]= 1065 ;
	} 
	error () ;
      } 
      else if ( ( curlist .modefield == 1 ) && ( curlist .headfield == curlist 
      .tailfield ) ) 
      {
	youcant () ;
	{
	  helpptr = 2 ;
	  helpline [1 ]= 1066 ;
	  helpline [0 ]= 1067 ;
	} 
	error () ;
      } 
      else {
	  
	if ( ! ( curlist .tailfield >= himemmin ) ) 
	if ( ( mem [curlist .tailfield ].hh.b0 == 0 ) || ( mem [curlist 
	.tailfield ].hh.b0 == 1 ) ) 
	{
	  q = curlist .headfield ;
	  do {
	      p = q ;
	    if ( ! ( q >= himemmin ) ) 
	    if ( mem [q ].hh.b0 == 7 ) 
	    {
	      {register integer for_end; m = 1 ;for_end = mem [q ].hh.b1 
	      ; if ( m <= for_end) do 
		p = mem [p ].hh .v.RH ;
	      while ( m++ < for_end ) ;} 
	      if ( p == curlist .tailfield ) 
	      goto lab30 ;
	    } 
	    q = mem [p ].hh .v.RH ;
	  } while ( ! ( q == curlist .tailfield ) ) ;
	  curbox = curlist .tailfield ;
	  mem [curbox + 4 ].cint = 0 ;
	  curlist .tailfield = p ;
	  mem [p ].hh .v.RH = 0 ;
	  lab30: ;
	} 
      } 
    } 
    break ;
  case 3 : 
    {
      scaneightbitint () ;
      n = curval ;
      if ( ! scankeyword ( 837 ) ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 1068 ) ;
	} 
	{
	  helpptr = 2 ;
	  helpline [1 ]= 1069 ;
	  helpline [0 ]= 1070 ;
	} 
	error () ;
      } 
      scandimen ( false , false , false ) ;
      curbox = vsplit ( n , curval ) ;
    } 
    break ;
    default: 
    {
      k = curchr - 4 ;
      savestack [saveptr + 0 ].cint = boxcontext ;
      if ( k == 102 ) 
      if ( ( boxcontext < 1073741824L ) && ( abs ( curlist .modefield ) == 1 ) 
      ) 
      scanspec ( 3 , true ) ;
      else scanspec ( 2 , true ) ;
      else {
	  
	if ( k == 1 ) 
	scanspec ( 4 , true ) ;
	else {
	    
	  scanspec ( 5 , true ) ;
	  k = 1 ;
	} 
	normalparagraph () ;
      } 
      pushnest () ;
      curlist .modefield = - (integer) k ;
      if ( k == 1 ) 
      {
	curlist .auxfield .cint = -65536000L ;
	if ( eqtb [13062 ].hh .v.RH != 0 ) 
	begintokenlist ( eqtb [13062 ].hh .v.RH , 11 ) ;
      } 
      else {
	  
	curlist .auxfield .hh .v.LH = 1000 ;
	if ( eqtb [13061 ].hh .v.RH != 0 ) 
	begintokenlist ( eqtb [13061 ].hh .v.RH , 10 ) ;
      } 
      return ;
    } 
    break ;
  } 
  boxend ( boxcontext ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
zscanbox ( integer boxcontext ) 
#else
zscanbox ( boxcontext ) 
  integer boxcontext ;
#endif
{
  scanbox_regmem 
  do {
      getxtoken () ;
  } while ( ! ( ( curcmd != 10 ) && ( curcmd != 0 ) ) ) ;
  if ( curcmd == 20 ) 
  beginbox ( boxcontext ) ;
  else if ( ( boxcontext >= 1073742337L ) && ( ( curcmd == 36 ) || ( curcmd == 
  35 ) ) ) 
  {
    curbox = scanrulespec () ;
    boxend ( boxcontext ) ;
  } 
  else {
      
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 1071 ) ;
    } 
    {
      helpptr = 3 ;
      helpline [2 ]= 1072 ;
      helpline [1 ]= 1073 ;
      helpline [0 ]= 1074 ;
    } 
    backerror () ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zpackage ( smallnumber c ) 
#else
zpackage ( c ) 
  smallnumber c ;
#endif
{
  package_regmem 
  scaled h  ;
  halfword p  ;
  scaled d  ;
  d = eqtb [15740 ].cint ;
  unsave () ;
  saveptr = saveptr - 3 ;
  if ( curlist .modefield == -102 ) 
  curbox = hpack ( mem [curlist .headfield ].hh .v.RH , savestack [saveptr 
  + 2 ].cint , savestack [saveptr + 1 ].cint ) ;
  else {
      
    curbox = vpackage ( mem [curlist .headfield ].hh .v.RH , savestack [
    saveptr + 2 ].cint , savestack [saveptr + 1 ].cint , d ) ;
    if ( c == 4 ) 
    {
      h = 0 ;
      p = mem [curbox + 5 ].hh .v.RH ;
      if ( p != 0 ) 
      if ( mem [p ].hh.b0 <= 2 ) 
      h = mem [p + 3 ].cint ;
      mem [curbox + 2 ].cint = mem [curbox + 2 ].cint - h + mem [curbox + 
      3 ].cint ;
      mem [curbox + 3 ].cint = h ;
    } 
  } 
  popnest () ;
  boxend ( savestack [saveptr + 0 ].cint ) ;
} 
smallnumber 
#ifdef HAVE_PROTOTYPES
znormmin ( integer h ) 
#else
znormmin ( h ) 
  integer h ;
#endif
{
  register smallnumber Result; normmin_regmem 
  if ( h <= 0 ) 
  Result = 1 ;
  else if ( h >= 63 ) 
  Result = 63 ;
  else Result = h ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
znewgraf ( boolean indented ) 
#else
znewgraf ( indented ) 
  boolean indented ;
#endif
{
  newgraf_regmem 
  curlist .pgfield = 0 ;
  if ( ( curlist .modefield == 1 ) || ( curlist .headfield != curlist 
  .tailfield ) ) 
  {
    mem [curlist .tailfield ].hh .v.RH = newparamglue ( 2 ) ;
    curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
  } 
  pushnest () ;
  curlist .modefield = 102 ;
  curlist .auxfield .hh .v.LH = 1000 ;
  if ( eqtb [15213 ].cint <= 0 ) 
  curlang = 0 ;
  else if ( eqtb [15213 ].cint > 255 ) 
  curlang = 0 ;
  else curlang = eqtb [15213 ].cint ;
  curlist .auxfield .hh .v.RH = curlang ;
  curlist .pgfield = ( normmin ( eqtb [15214 ].cint ) * 64 + normmin ( eqtb 
  [15215 ].cint ) ) * 65536L + curlang ;
  if ( indented ) 
  {
    curlist .tailfield = newnullbox () ;
    mem [curlist .headfield ].hh .v.RH = curlist .tailfield ;
    mem [curlist .tailfield + 1 ].cint = eqtb [15733 ].cint ;
  } 
  if ( eqtb [13058 ].hh .v.RH != 0 ) 
  begintokenlist ( eqtb [13058 ].hh .v.RH , 7 ) ;
  if ( nestptr == 1 ) 
  buildpage () ;
} 
void 
#ifdef HAVE_PROTOTYPES
indentinhmode ( void ) 
#else
indentinhmode ( ) 
#endif
{
  indentinhmode_regmem 
  halfword p, q  ;
  if ( curchr > 0 ) 
  {
    p = newnullbox () ;
    mem [p + 1 ].cint = eqtb [15733 ].cint ;
    if ( abs ( curlist .modefield ) == 102 ) 
    curlist .auxfield .hh .v.LH = 1000 ;
    else {
	
      q = newnoad () ;
      mem [q + 1 ].hh .v.RH = 2 ;
      mem [q + 1 ].hh .v.LH = p ;
      p = q ;
    } 
    {
      mem [curlist .tailfield ].hh .v.RH = p ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
headforvmode ( void ) 
#else
headforvmode ( ) 
#endif
{
  headforvmode_regmem 
  if ( curlist .modefield < 0 ) 
  if ( curcmd != 36 ) 
  offsave () ;
  else {
      
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 684 ) ;
    } 
    printesc ( 521 ) ;
    print ( 1077 ) ;
    {
      helpptr = 2 ;
      helpline [1 ]= 1078 ;
      helpline [0 ]= 1079 ;
    } 
    error () ;
  } 
  else {
      
    backinput () ;
    curtok = partoken ;
    backinput () ;
    curinput .indexfield = 4 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
endgraf ( void ) 
#else
endgraf ( ) 
#endif
{
  endgraf_regmem 
  if ( curlist .modefield == 102 ) 
  {
    if ( curlist .headfield == curlist .tailfield ) 
    popnest () ;
    else linebreak ( eqtb [15169 ].cint ) ;
    normalparagraph () ;
    errorcount = 0 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
begininsertoradjust ( void ) 
#else
begininsertoradjust ( ) 
#endif
{
  begininsertoradjust_regmem 
  if ( curcmd == 38 ) 
  curval = 255 ;
  else {
      
    scaneightbitint () ;
    if ( curval == 255 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 1080 ) ;
      } 
      printesc ( 327 ) ;
      printint ( 255 ) ;
      {
	helpptr = 1 ;
	helpline [0 ]= 1081 ;
      } 
      error () ;
      curval = 0 ;
    } 
  } 
  savestack [saveptr + 0 ].cint = curval ;
  incr ( saveptr ) ;
  newsavelevel ( 11 ) ;
  scanleftbrace () ;
  normalparagraph () ;
  pushnest () ;
  curlist .modefield = -1 ;
  curlist .auxfield .cint = -65536000L ;
} 
void 
#ifdef HAVE_PROTOTYPES
makemark ( void ) 
#else
makemark ( ) 
#endif
{
  makemark_regmem 
  halfword p  ;
  p = scantoks ( false , true ) ;
  p = getnode ( 2 ) ;
  mem [p ].hh.b0 = 4 ;
  mem [p ].hh.b1 = 0 ;
  mem [p + 1 ].cint = defref ;
  mem [curlist .tailfield ].hh .v.RH = p ;
  curlist .tailfield = p ;
} 
void 
#ifdef HAVE_PROTOTYPES
appendpenalty ( void ) 
#else
appendpenalty ( ) 
#endif
{
  appendpenalty_regmem 
  scanint () ;
  {
    mem [curlist .tailfield ].hh .v.RH = newpenalty ( curval ) ;
    curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
  } 
  if ( curlist .modefield == 1 ) 
  buildpage () ;
} 
void 
#ifdef HAVE_PROTOTYPES
deletelast ( void ) 
#else
deletelast ( ) 
#endif
{
  /* 10 */ deletelast_regmem 
  halfword p, q  ;
  quarterword m  ;
  if ( ( curlist .modefield == 1 ) && ( curlist .tailfield == curlist 
  .headfield ) ) 
  {
    if ( ( curchr != 10 ) || ( lastglue != 268435455L ) ) 
    {
      youcant () ;
      {
	helpptr = 2 ;
	helpline [1 ]= 1066 ;
	helpline [0 ]= 1082 ;
      } 
      if ( curchr == 11 ) 
      helpline [0 ]= ( 1083 ) ;
      else if ( curchr != 10 ) 
      helpline [0 ]= ( 1084 ) ;
      error () ;
    } 
  } 
  else {
      
    if ( ! ( curlist .tailfield >= himemmin ) ) 
    if ( mem [curlist .tailfield ].hh.b0 == curchr ) 
    {
      q = curlist .headfield ;
      do {
	  p = q ;
	if ( ! ( q >= himemmin ) ) 
	if ( mem [q ].hh.b0 == 7 ) 
	{
	  {register integer for_end; m = 1 ;for_end = mem [q ].hh.b1 
	  ; if ( m <= for_end) do 
	    p = mem [p ].hh .v.RH ;
	  while ( m++ < for_end ) ;} 
	  if ( p == curlist .tailfield ) 
	  return ;
	} 
	q = mem [p ].hh .v.RH ;
      } while ( ! ( q == curlist .tailfield ) ) ;
      mem [p ].hh .v.RH = 0 ;
      flushnodelist ( curlist .tailfield ) ;
      curlist .tailfield = p ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
unpackage ( void ) 
#else
unpackage ( ) 
#endif
{
  /* 10 */ unpackage_regmem 
  halfword p  ;
  char c  ;
  c = curchr ;
  scaneightbitint () ;
  p = eqtb [13322 + curval ].hh .v.RH ;
  if ( p == 0 ) 
  return ;
  if ( ( abs ( curlist .modefield ) == 203 ) || ( ( abs ( curlist .modefield ) 
  == 1 ) && ( mem [p ].hh.b0 != 1 ) ) || ( ( abs ( curlist .modefield ) == 
  102 ) && ( mem [p ].hh.b0 != 0 ) ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 1092 ) ;
    } 
    {
      helpptr = 3 ;
      helpline [2 ]= 1093 ;
      helpline [1 ]= 1094 ;
      helpline [0 ]= 1095 ;
    } 
    error () ;
    return ;
  } 
  if ( c == 1 ) 
  mem [curlist .tailfield ].hh .v.RH = copynodelist ( mem [p + 5 ].hh 
  .v.RH ) ;
  else {
      
    mem [curlist .tailfield ].hh .v.RH = mem [p + 5 ].hh .v.RH ;
    eqtb [13322 + curval ].hh .v.RH = 0 ;
    freenode ( p , 7 ) ;
  } 
  while ( mem [curlist .tailfield ].hh .v.RH != 0 ) curlist .tailfield = mem 
  [curlist .tailfield ].hh .v.RH ;
} 
void 
#ifdef HAVE_PROTOTYPES
appenditaliccorrection ( void ) 
#else
appenditaliccorrection ( ) 
#endif
{
  /* 10 */ appenditaliccorrection_regmem 
  halfword p  ;
  internalfontnumber f  ;
  if ( curlist .tailfield != curlist .headfield ) 
  {
    if ( ( curlist .tailfield >= himemmin ) ) 
    p = curlist .tailfield ;
    else if ( mem [curlist .tailfield ].hh.b0 == 6 ) 
    p = curlist .tailfield + 1 ;
    else return ;
    f = mem [p ].hh.b0 ;
    {
      mem [curlist .tailfield ].hh .v.RH = newkern ( fontinfo [italicbase [
      f ]+ ( fontinfo [charbase [f ]+ effectivechar ( true , f , mem [p ]
      .hh.b1 ) ].qqqq .b2 ) / 4 ].cint ) ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
    mem [curlist .tailfield ].hh.b1 = 1 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
appenddiscretionary ( void ) 
#else
appenddiscretionary ( ) 
#endif
{
  appenddiscretionary_regmem 
  integer c  ;
  {
    mem [curlist .tailfield ].hh .v.RH = newdisc () ;
    curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
  } 
  if ( curchr == 1 ) 
  {
    c = hyphenchar [eqtb [13578 ].hh .v.RH ];
    if ( c >= 0 ) 
    if ( c < 256 ) 
    mem [curlist .tailfield + 1 ].hh .v.LH = newcharacter ( eqtb [13578 ]
    .hh .v.RH , c ) ;
  } 
  else {
      
    incr ( saveptr ) ;
    savestack [saveptr - 1 ].cint = 0 ;
    newsavelevel ( 10 ) ;
    scanleftbrace () ;
    pushnest () ;
    curlist .modefield = -102 ;
    curlist .auxfield .hh .v.LH = 1000 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
builddiscretionary ( void ) 
#else
builddiscretionary ( ) 
#endif
{
  /* 30 10 */ builddiscretionary_regmem 
  halfword p, q  ;
  integer n  ;
  unsave () ;
  q = curlist .headfield ;
  p = mem [q ].hh .v.RH ;
  n = 0 ;
  while ( p != 0 ) {
      
    if ( ! ( p >= himemmin ) ) 
    if ( mem [p ].hh.b0 > 2 ) 
    if ( mem [p ].hh.b0 != 11 ) 
    if ( mem [p ].hh.b0 != 6 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 1102 ) ;
      } 
      {
	helpptr = 1 ;
	helpline [0 ]= 1103 ;
      } 
      error () ;
      begindiagnostic () ;
      printnl ( 1104 ) ;
      showbox ( p ) ;
      enddiagnostic ( true ) ;
      flushnodelist ( p ) ;
      mem [q ].hh .v.RH = 0 ;
      goto lab30 ;
    } 
    q = p ;
    p = mem [q ].hh .v.RH ;
    incr ( n ) ;
  } 
  lab30: ;
  p = mem [curlist .headfield ].hh .v.RH ;
  popnest () ;
  switch ( savestack [saveptr - 1 ].cint ) 
  {case 0 : 
    mem [curlist .tailfield + 1 ].hh .v.LH = p ;
    break ;
  case 1 : 
    mem [curlist .tailfield + 1 ].hh .v.RH = p ;
    break ;
  case 2 : 
    {
      if ( ( n > 0 ) && ( abs ( curlist .modefield ) == 203 ) ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 1096 ) ;
	} 
	printesc ( 346 ) ;
	{
	  helpptr = 2 ;
	  helpline [1 ]= 1097 ;
	  helpline [0 ]= 1098 ;
	} 
	flushnodelist ( p ) ;
	n = 0 ;
	error () ;
      } 
      else mem [curlist .tailfield ].hh .v.RH = p ;
      if ( n <= 255 ) 
      mem [curlist .tailfield ].hh.b1 = n ;
      else {
	  
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 1099 ) ;
	} 
	{
	  helpptr = 2 ;
	  helpline [1 ]= 1100 ;
	  helpline [0 ]= 1101 ;
	} 
	error () ;
      } 
      if ( n > 0 ) 
      curlist .tailfield = q ;
      decr ( saveptr ) ;
      return ;
    } 
    break ;
  } 
  incr ( savestack [saveptr - 1 ].cint ) ;
  newsavelevel ( 10 ) ;
  scanleftbrace () ;
  pushnest () ;
  curlist .modefield = -102 ;
  curlist .auxfield .hh .v.LH = 1000 ;
} 
void 
#ifdef HAVE_PROTOTYPES
makeaccent ( void ) 
#else
makeaccent ( ) 
#endif
{
  makeaccent_regmem 
  real s, t  ;
  halfword p, q, r  ;
  internalfontnumber f  ;
  scaled a, h, x, w, delta  ;
  fourquarters i  ;
  scancharnum () ;
  f = eqtb [13578 ].hh .v.RH ;
  p = newcharacter ( f , curval ) ;
  if ( p != 0 ) 
  {
    x = fontinfo [5 + parambase [f ]].cint ;
    s = fontinfo [1 + parambase [f ]].cint / ((double) 65536.0 ) ;
    a = fontinfo [widthbase [f ]+ fontinfo [charbase [f ]+ effectivechar 
    ( true , f , mem [p ].hh.b1 ) ].qqqq .b0 ].cint ;
    doassignments () ;
    q = 0 ;
    f = eqtb [13578 ].hh .v.RH ;
    if ( ( curcmd == 11 ) || ( curcmd == 12 ) || ( curcmd == 68 ) ) 
    q = newcharacter ( f , curchr ) ;
    else if ( curcmd == 16 ) 
    {
      scancharnum () ;
      q = newcharacter ( f , curval ) ;
    } 
    else backinput () ;
    if ( q != 0 ) 
    {
      t = fontinfo [1 + parambase [f ]].cint / ((double) 65536.0 ) ;
      i = fontinfo [charbase [f ]+ effectivechar ( true , f , mem [q ]
      .hh.b1 ) ].qqqq ;
      w = fontinfo [widthbase [f ]+ i .b0 ].cint ;
      h = fontinfo [heightbase [f ]+ ( i .b1 ) / 16 ].cint ;
      if ( h != x ) 
      {
	p = hpack ( p , 0 , 1 ) ;
	mem [p + 4 ].cint = x - h ;
      } 
      delta = round ( ( w - a ) / ((double) 2.0 ) + h * t - x * s ) ;
      r = newkern ( delta ) ;
      mem [r ].hh.b1 = 2 ;
      mem [curlist .tailfield ].hh .v.RH = r ;
      mem [r ].hh .v.RH = p ;
      curlist .tailfield = newkern ( - (integer) a - delta ) ;
      mem [curlist .tailfield ].hh.b1 = 2 ;
      mem [p ].hh .v.RH = curlist .tailfield ;
      p = q ;
    } 
    mem [curlist .tailfield ].hh .v.RH = p ;
    curlist .tailfield = p ;
    curlist .auxfield .hh .v.LH = 1000 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
alignerror ( void ) 
#else
alignerror ( ) 
#endif
{
  alignerror_regmem 
  if ( abs ( alignstate ) > 2 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 1109 ) ;
    } 
    printcmdchr ( curcmd , curchr ) ;
    if ( curtok == 1062 ) 
    {
      {
	helpptr = 6 ;
	helpline [5 ]= 1110 ;
	helpline [4 ]= 1111 ;
	helpline [3 ]= 1112 ;
	helpline [2 ]= 1113 ;
	helpline [1 ]= 1114 ;
	helpline [0 ]= 1115 ;
      } 
    } 
    else {
	
      {
	helpptr = 5 ;
	helpline [4 ]= 1110 ;
	helpline [3 ]= 1116 ;
	helpline [2 ]= 1113 ;
	helpline [1 ]= 1114 ;
	helpline [0 ]= 1115 ;
      } 
    } 
    error () ;
  } 
  else {
      
    backinput () ;
    if ( alignstate < 0 ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 656 ) ;
      } 
      incr ( alignstate ) ;
      curtok = 379 ;
    } 
    else {
	
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 1105 ) ;
      } 
      decr ( alignstate ) ;
      curtok = 637 ;
    } 
    {
      helpptr = 3 ;
      helpline [2 ]= 1106 ;
      helpline [1 ]= 1107 ;
      helpline [0 ]= 1108 ;
    } 
    inserror () ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
noalignerror ( void ) 
#else
noalignerror ( ) 
#endif
{
  noalignerror_regmem 
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 262 ) ;
    print ( 1109 ) ;
  } 
  printesc ( 527 ) ;
  {
    helpptr = 2 ;
    helpline [1 ]= 1117 ;
    helpline [0 ]= 1118 ;
  } 
  error () ;
} 
void 
#ifdef HAVE_PROTOTYPES
omiterror ( void ) 
#else
omiterror ( ) 
#endif
{
  omiterror_regmem 
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 262 ) ;
    print ( 1109 ) ;
  } 
  printesc ( 530 ) ;
  {
    helpptr = 2 ;
    helpline [1 ]= 1119 ;
    helpline [0 ]= 1118 ;
  } 
  error () ;
} 
void 
#ifdef HAVE_PROTOTYPES
doendv ( void ) 
#else
doendv ( ) 
#endif
{
  doendv_regmem 
  if ( curgroup == 6 ) 
  {
    endgraf () ;
    if ( fincol () ) 
    finrow () ;
  } 
  else offsave () ;
} 
void 
#ifdef HAVE_PROTOTYPES
cserror ( void ) 
#else
cserror ( ) 
#endif
{
  cserror_regmem 
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 262 ) ;
    print ( 775 ) ;
  } 
  printesc ( 505 ) ;
  {
    helpptr = 1 ;
    helpline [0 ]= 1121 ;
  } 
  error () ;
} 
void 
#ifdef HAVE_PROTOTYPES
zpushmath ( groupcode c ) 
#else
zpushmath ( c ) 
  groupcode c ;
#endif
{
  pushmath_regmem 
  pushnest () ;
  curlist .modefield = -203 ;
  curlist .auxfield .cint = 0 ;
  newsavelevel ( c ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
initmath ( void ) 
#else
initmath ( ) 
#endif
{
  /* 21 40 45 30 */ initmath_regmem 
  scaled w  ;
  scaled l  ;
  scaled s  ;
  halfword p  ;
  halfword q  ;
  internalfontnumber f  ;
  integer n  ;
  scaled v  ;
  scaled d  ;
  gettoken () ;
  if ( ( curcmd == 3 ) && ( curlist .modefield > 0 ) ) 
  {
    if ( curlist .headfield == curlist .tailfield ) 
    {
      popnest () ;
      w = -1073741823L ;
    } 
    else {
	
      linebreak ( eqtb [15170 ].cint ) ;
      v = mem [justbox + 4 ].cint + 2 * fontinfo [6 + parambase [eqtb [
      13578 ].hh .v.RH ]].cint ;
      w = -1073741823L ;
      p = mem [justbox + 5 ].hh .v.RH ;
      while ( p != 0 ) {
	  
	lab21: if ( ( p >= himemmin ) ) 
	{
	  f = mem [p ].hh.b0 ;
	  d = fontinfo [widthbase [f ]+ fontinfo [charbase [f ]+ 
	  effectivechar ( true , f , mem [p ].hh.b1 ) ].qqqq .b0 ].cint ;
	  goto lab40 ;
	} 
	switch ( mem [p ].hh.b0 ) 
	{case 0 : 
	case 1 : 
	case 2 : 
	  {
	    d = mem [p + 1 ].cint ;
	    goto lab40 ;
	  } 
	  break ;
	case 6 : 
	  {
	    mem [memtop - 12 ]= mem [p + 1 ];
	    mem [memtop - 12 ].hh .v.RH = mem [p ].hh .v.RH ;
	    p = memtop - 12 ;
	    goto lab21 ;
	  } 
	  break ;
	case 11 : 
	case 9 : 
	  d = mem [p + 1 ].cint ;
	  break ;
	case 10 : 
	  {
	    q = mem [p + 1 ].hh .v.LH ;
	    d = mem [q + 1 ].cint ;
	    if ( mem [justbox + 5 ].hh.b0 == 1 ) 
	    {
	      if ( ( mem [justbox + 5 ].hh.b1 == mem [q ].hh.b0 ) && ( mem 
	      [q + 2 ].cint != 0 ) ) 
	      v = 1073741823L ;
	    } 
	    else if ( mem [justbox + 5 ].hh.b0 == 2 ) 
	    {
	      if ( ( mem [justbox + 5 ].hh.b1 == mem [q ].hh.b1 ) && ( mem 
	      [q + 3 ].cint != 0 ) ) 
	      v = 1073741823L ;
	    } 
	    if ( mem [p ].hh.b1 >= 100 ) 
	    goto lab40 ;
	  } 
	  break ;
	case 8 : 
	  d = 0 ;
	  break ;
	  default: 
	  d = 0 ;
	  break ;
	} 
	if ( v < 1073741823L ) 
	v = v + d ;
	goto lab45 ;
	lab40: if ( v < 1073741823L ) 
	{
	  v = v + d ;
	  w = v ;
	} 
	else {
	    
	  w = 1073741823L ;
	  goto lab30 ;
	} 
	lab45: p = mem [p ].hh .v.RH ;
      } 
      lab30: ;
    } 
    if ( eqtb [13056 ].hh .v.RH == 0 ) 
    if ( ( eqtb [15750 ].cint != 0 ) && ( ( ( eqtb [15204 ].cint >= 0 ) && 
    ( curlist .pgfield + 2 > eqtb [15204 ].cint ) ) || ( curlist .pgfield + 
    1 < - (integer) eqtb [15204 ].cint ) ) ) 
    {
      l = eqtb [15736 ].cint - abs ( eqtb [15750 ].cint ) ;
      if ( eqtb [15750 ].cint > 0 ) 
      s = eqtb [15750 ].cint ;
      else s = 0 ;
    } 
    else {
	
      l = eqtb [15736 ].cint ;
      s = 0 ;
    } 
    else {
	
      n = mem [eqtb [13056 ].hh .v.RH ].hh .v.LH ;
      if ( curlist .pgfield + 2 >= n ) 
      p = eqtb [13056 ].hh .v.RH + 2 * n ;
      else p = eqtb [13056 ].hh .v.RH + 2 * ( curlist .pgfield + 2 ) ;
      s = mem [p - 1 ].cint ;
      l = mem [p ].cint ;
    } 
    pushmath ( 15 ) ;
    curlist .modefield = 203 ;
    eqworddefine ( 15207 , -1 ) ;
    eqworddefine ( 15746 , w ) ;
    eqworddefine ( 15747 , l ) ;
    eqworddefine ( 15748 , s ) ;
    if ( eqtb [13060 ].hh .v.RH != 0 ) 
    begintokenlist ( eqtb [13060 ].hh .v.RH , 9 ) ;
    if ( nestptr == 1 ) 
    buildpage () ;
  } 
  else {
      
    backinput () ;
    {
      pushmath ( 15 ) ;
      eqworddefine ( 15207 , -1 ) ;
      if ( eqtb [13059 ].hh .v.RH != 0 ) 
      begintokenlist ( eqtb [13059 ].hh .v.RH , 8 ) ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
starteqno ( void ) 
#else
starteqno ( ) 
#endif
{
  starteqno_regmem 
  savestack [saveptr + 0 ].cint = curchr ;
  incr ( saveptr ) ;
  {
    pushmath ( 15 ) ;
    eqworddefine ( 15207 , -1 ) ;
    if ( eqtb [13059 ].hh .v.RH != 0 ) 
    begintokenlist ( eqtb [13059 ].hh .v.RH , 8 ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zscanmath ( halfword p ) 
#else
zscanmath ( p ) 
  halfword p ;
#endif
{
  /* 20 21 10 */ scanmath_regmem 
  integer c  ;
  lab20: do {
      getxtoken () ;
  } while ( ! ( ( curcmd != 10 ) && ( curcmd != 0 ) ) ) ;
  lab21: switch ( curcmd ) 
  {case 11 : 
  case 12 : 
  case 68 : 
    {
      c = eqtb [14651 + curchr ].hh .v.RH ;
      if ( c == 32768L ) 
      {
	{
	  curcs = curchr + 1 ;
	  curcmd = eqtb [curcs ].hh.b0 ;
	  curchr = eqtb [curcs ].hh .v.RH ;
	  xtoken () ;
	  backinput () ;
	} 
	goto lab20 ;
      } 
    } 
    break ;
  case 16 : 
    {
      scancharnum () ;
      curchr = curval ;
      curcmd = 68 ;
      goto lab21 ;
    } 
    break ;
  case 17 : 
    {
      scanfifteenbitint () ;
      c = curval ;
    } 
    break ;
  case 69 : 
    c = curchr ;
    break ;
  case 15 : 
    {
      scantwentysevenbitint () ;
      c = curval / 4096 ;
    } 
    break ;
    default: 
    {
      backinput () ;
      scanleftbrace () ;
      savestack [saveptr + 0 ].cint = p ;
      incr ( saveptr ) ;
      pushmath ( 9 ) ;
      return ;
    } 
    break ;
  } 
  mem [p ].hh .v.RH = 1 ;
  mem [p ].hh.b1 = c % 256 ;
  if ( ( c >= 28672 ) && ( ( eqtb [15207 ].cint >= 0 ) && ( eqtb [15207 ]
  .cint < 16 ) ) ) 
  mem [p ].hh.b0 = eqtb [15207 ].cint ;
  else mem [p ].hh.b0 = ( c / 256 ) % 16 ;
} 
void 
#ifdef HAVE_PROTOTYPES
zsetmathchar ( integer c ) 
#else
zsetmathchar ( c ) 
  integer c ;
#endif
{
  setmathchar_regmem 
  halfword p  ;
  if ( c >= 32768L ) 
  {
    curcs = curchr + 1 ;
    curcmd = eqtb [curcs ].hh.b0 ;
    curchr = eqtb [curcs ].hh .v.RH ;
    xtoken () ;
    backinput () ;
  } 
  else {
      
    p = newnoad () ;
    mem [p + 1 ].hh .v.RH = 1 ;
    mem [p + 1 ].hh.b1 = c % 256 ;
    mem [p + 1 ].hh.b0 = ( c / 256 ) % 16 ;
    if ( c >= 28672 ) 
    {
      if ( ( ( eqtb [15207 ].cint >= 0 ) && ( eqtb [15207 ].cint < 16 ) ) 
      ) 
      mem [p + 1 ].hh.b0 = eqtb [15207 ].cint ;
      mem [p ].hh.b0 = 16 ;
    } 
    else mem [p ].hh.b0 = 16 + ( c / 4096 ) ;
    mem [curlist .tailfield ].hh .v.RH = p ;
    curlist .tailfield = p ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
mathlimitswitch ( void ) 
#else
mathlimitswitch ( ) 
#endif
{
  /* 10 */ mathlimitswitch_regmem 
  if ( curlist .headfield != curlist .tailfield ) 
  if ( mem [curlist .tailfield ].hh.b0 == 17 ) 
  {
    mem [curlist .tailfield ].hh.b1 = curchr ;
    return ;
  } 
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 262 ) ;
    print ( 1125 ) ;
  } 
  {
    helpptr = 1 ;
    helpline [0 ]= 1126 ;
  } 
  error () ;
} 
void 
#ifdef HAVE_PROTOTYPES
zscandelimiter ( halfword p , boolean r ) 
#else
zscandelimiter ( p , r ) 
  halfword p ;
  boolean r ;
#endif
{
  scandelimiter_regmem 
  if ( r ) 
  scantwentysevenbitint () ;
  else {
      
    do {
	getxtoken () ;
    } while ( ! ( ( curcmd != 10 ) && ( curcmd != 0 ) ) ) ;
    switch ( curcmd ) 
    {case 11 : 
    case 12 : 
      curval = eqtb [15477 + curchr ].cint ;
      break ;
    case 15 : 
      scantwentysevenbitint () ;
      break ;
      default: 
      curval = -1 ;
      break ;
    } 
  } 
  if ( curval < 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 1127 ) ;
    } 
    {
      helpptr = 6 ;
      helpline [5 ]= 1128 ;
      helpline [4 ]= 1129 ;
      helpline [3 ]= 1130 ;
      helpline [2 ]= 1131 ;
      helpline [1 ]= 1132 ;
      helpline [0 ]= 1133 ;
    } 
    backerror () ;
    curval = 0 ;
  } 
  mem [p ].qqqq .b0 = ( curval / 1048576L ) % 16 ;
  mem [p ].qqqq .b1 = ( curval / 4096 ) % 256 ;
  mem [p ].qqqq .b2 = ( curval / 256 ) % 16 ;
  mem [p ].qqqq .b3 = curval % 256 ;
} 
void 
#ifdef HAVE_PROTOTYPES
mathradical ( void ) 
#else
mathradical ( ) 
#endif
{
  mathradical_regmem 
  {
    mem [curlist .tailfield ].hh .v.RH = getnode ( 5 ) ;
    curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
  } 
  mem [curlist .tailfield ].hh.b0 = 24 ;
  mem [curlist .tailfield ].hh.b1 = 0 ;
  mem [curlist .tailfield + 1 ].hh = emptyfield ;
  mem [curlist .tailfield + 3 ].hh = emptyfield ;
  mem [curlist .tailfield + 2 ].hh = emptyfield ;
  scandelimiter ( curlist .tailfield + 4 , true ) ;
  scanmath ( curlist .tailfield + 1 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
mathac ( void ) 
#else
mathac ( ) 
#endif
{
  mathac_regmem 
  if ( curcmd == 45 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 1134 ) ;
    } 
    printesc ( 523 ) ;
    print ( 1135 ) ;
    {
      helpptr = 2 ;
      helpline [1 ]= 1136 ;
      helpline [0 ]= 1137 ;
    } 
    error () ;
  } 
  {
    mem [curlist .tailfield ].hh .v.RH = getnode ( 5 ) ;
    curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
  } 
  mem [curlist .tailfield ].hh.b0 = 28 ;
  mem [curlist .tailfield ].hh.b1 = 0 ;
  mem [curlist .tailfield + 1 ].hh = emptyfield ;
  mem [curlist .tailfield + 3 ].hh = emptyfield ;
  mem [curlist .tailfield + 2 ].hh = emptyfield ;
  mem [curlist .tailfield + 4 ].hh .v.RH = 1 ;
  scanfifteenbitint () ;
  mem [curlist .tailfield + 4 ].hh.b1 = curval % 256 ;
  if ( ( curval >= 28672 ) && ( ( eqtb [15207 ].cint >= 0 ) && ( eqtb [
  15207 ].cint < 16 ) ) ) 
  mem [curlist .tailfield + 4 ].hh.b0 = eqtb [15207 ].cint ;
  else mem [curlist .tailfield + 4 ].hh.b0 = ( curval / 256 ) % 16 ;
  scanmath ( curlist .tailfield + 1 ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
appendchoices ( void ) 
#else
appendchoices ( ) 
#endif
{
  appendchoices_regmem 
  {
    mem [curlist .tailfield ].hh .v.RH = newchoice () ;
    curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
  } 
  incr ( saveptr ) ;
  savestack [saveptr - 1 ].cint = 0 ;
  pushmath ( 13 ) ;
  scanleftbrace () ;
} 
halfword 
#ifdef HAVE_PROTOTYPES
zfinmlist ( halfword p ) 
#else
zfinmlist ( p ) 
  halfword p ;
#endif
{
  register halfword Result; finmlist_regmem 
  halfword q  ;
  if ( curlist .auxfield .cint != 0 ) 
  {
    mem [curlist .auxfield .cint + 3 ].hh .v.RH = 3 ;
    mem [curlist .auxfield .cint + 3 ].hh .v.LH = mem [curlist .headfield ]
    .hh .v.RH ;
    if ( p == 0 ) 
    q = curlist .auxfield .cint ;
    else {
	
      q = mem [curlist .auxfield .cint + 2 ].hh .v.LH ;
      if ( mem [q ].hh.b0 != 30 ) 
      confusion ( 872 ) ;
      mem [curlist .auxfield .cint + 2 ].hh .v.LH = mem [q ].hh .v.RH ;
      mem [q ].hh .v.RH = curlist .auxfield .cint ;
      mem [curlist .auxfield .cint ].hh .v.RH = p ;
    } 
  } 
  else {
      
    mem [curlist .tailfield ].hh .v.RH = p ;
    q = mem [curlist .headfield ].hh .v.RH ;
  } 
  popnest () ;
  Result = q ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
buildchoices ( void ) 
#else
buildchoices ( ) 
#endif
{
  /* 10 */ buildchoices_regmem 
  halfword p  ;
  unsave () ;
  p = finmlist ( 0 ) ;
  switch ( savestack [saveptr - 1 ].cint ) 
  {case 0 : 
    mem [curlist .tailfield + 1 ].hh .v.LH = p ;
    break ;
  case 1 : 
    mem [curlist .tailfield + 1 ].hh .v.RH = p ;
    break ;
  case 2 : 
    mem [curlist .tailfield + 2 ].hh .v.LH = p ;
    break ;
  case 3 : 
    {
      mem [curlist .tailfield + 2 ].hh .v.RH = p ;
      decr ( saveptr ) ;
      return ;
    } 
    break ;
  } 
  incr ( savestack [saveptr - 1 ].cint ) ;
  pushmath ( 13 ) ;
  scanleftbrace () ;
} 
void 
#ifdef HAVE_PROTOTYPES
subsup ( void ) 
#else
subsup ( ) 
#endif
{
  subsup_regmem 
  smallnumber t  ;
  halfword p  ;
  t = 0 ;
  p = 0 ;
  if ( curlist .tailfield != curlist .headfield ) 
  if ( ( mem [curlist .tailfield ].hh.b0 >= 16 ) && ( mem [curlist 
  .tailfield ].hh.b0 < 30 ) ) 
  {
    p = curlist .tailfield + 2 + curcmd - 7 ;
    t = mem [p ].hh .v.RH ;
  } 
  if ( ( p == 0 ) || ( t != 0 ) ) 
  {
    {
      mem [curlist .tailfield ].hh .v.RH = newnoad () ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
    p = curlist .tailfield + 2 + curcmd - 7 ;
    if ( t != 0 ) 
    {
      if ( curcmd == 7 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 1138 ) ;
	} 
	{
	  helpptr = 1 ;
	  helpline [0 ]= 1139 ;
	} 
      } 
      else {
	  
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 1140 ) ;
	} 
	{
	  helpptr = 1 ;
	  helpline [0 ]= 1141 ;
	} 
      } 
      error () ;
    } 
  } 
  scanmath ( p ) ;
} 
void 
#ifdef HAVE_PROTOTYPES
mathfraction ( void ) 
#else
mathfraction ( ) 
#endif
{
  mathfraction_regmem 
  smallnumber c  ;
  c = curchr ;
  if ( curlist .auxfield .cint != 0 ) 
  {
    if ( c >= 3 ) 
    {
      scandelimiter ( memtop - 12 , false ) ;
      scandelimiter ( memtop - 12 , false ) ;
    } 
    if ( c % 3 == 0 ) 
    scandimen ( false , false , false ) ;
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 1148 ) ;
    } 
    {
      helpptr = 3 ;
      helpline [2 ]= 1149 ;
      helpline [1 ]= 1150 ;
      helpline [0 ]= 1151 ;
    } 
    error () ;
  } 
  else {
      
    curlist .auxfield .cint = getnode ( 6 ) ;
    mem [curlist .auxfield .cint ].hh.b0 = 25 ;
    mem [curlist .auxfield .cint ].hh.b1 = 0 ;
    mem [curlist .auxfield .cint + 2 ].hh .v.RH = 3 ;
    mem [curlist .auxfield .cint + 2 ].hh .v.LH = mem [curlist .headfield ]
    .hh .v.RH ;
    mem [curlist .auxfield .cint + 3 ].hh = emptyfield ;
    mem [curlist .auxfield .cint + 4 ].qqqq = nulldelimiter ;
    mem [curlist .auxfield .cint + 5 ].qqqq = nulldelimiter ;
    mem [curlist .headfield ].hh .v.RH = 0 ;
    curlist .tailfield = curlist .headfield ;
    if ( c >= 3 ) 
    {
      scandelimiter ( curlist .auxfield .cint + 4 , false ) ;
      scandelimiter ( curlist .auxfield .cint + 5 , false ) ;
    } 
    switch ( c % 3 ) 
    {case 0 : 
      {
	scandimen ( false , false , false ) ;
	mem [curlist .auxfield .cint + 1 ].cint = curval ;
      } 
      break ;
    case 1 : 
      mem [curlist .auxfield .cint + 1 ].cint = 1073741824L ;
      break ;
    case 2 : 
      mem [curlist .auxfield .cint + 1 ].cint = 0 ;
      break ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
mathleftright ( void ) 
#else
mathleftright ( ) 
#endif
{
  mathleftright_regmem 
  smallnumber t  ;
  halfword p  ;
  t = curchr ;
  if ( ( t == 31 ) && ( curgroup != 16 ) ) 
  {
    if ( curgroup == 15 ) 
    {
      scandelimiter ( memtop - 12 , false ) ;
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 775 ) ;
      } 
      printesc ( 872 ) ;
      {
	helpptr = 1 ;
	helpline [0 ]= 1152 ;
      } 
      error () ;
    } 
    else offsave () ;
  } 
  else {
      
    p = newnoad () ;
    mem [p ].hh.b0 = t ;
    scandelimiter ( p + 1 , false ) ;
    if ( t == 30 ) 
    {
      pushmath ( 16 ) ;
      mem [curlist .headfield ].hh .v.RH = p ;
      curlist .tailfield = p ;
    } 
    else {
	
      p = finmlist ( p ) ;
      unsave () ;
      {
	mem [curlist .tailfield ].hh .v.RH = newnoad () ;
	curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
      } 
      mem [curlist .tailfield ].hh.b0 = 23 ;
      mem [curlist .tailfield + 1 ].hh .v.RH = 3 ;
      mem [curlist .tailfield + 1 ].hh .v.LH = p ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
aftermath ( void ) 
#else
aftermath ( ) 
#endif
{
  aftermath_regmem 
  boolean l  ;
  boolean danger  ;
  integer m  ;
  halfword p  ;
  halfword a  ;
  halfword b  ;
  scaled w  ;
  scaled z  ;
  scaled e  ;
  scaled q  ;
  scaled d  ;
  scaled s  ;
  smallnumber g1, g2  ;
  halfword r  ;
  halfword t  ;
  danger = false ;
  if ( ( fontparams [eqtb [13581 ].hh .v.RH ]< 22 ) || ( fontparams [eqtb 
  [13597 ].hh .v.RH ]< 22 ) || ( fontparams [eqtb [13613 ].hh .v.RH ]< 
  22 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 1153 ) ;
    } 
    {
      helpptr = 3 ;
      helpline [2 ]= 1154 ;
      helpline [1 ]= 1155 ;
      helpline [0 ]= 1156 ;
    } 
    error () ;
    flushmath () ;
    danger = true ;
  } 
  else if ( ( fontparams [eqtb [13582 ].hh .v.RH ]< 13 ) || ( fontparams [
  eqtb [13598 ].hh .v.RH ]< 13 ) || ( fontparams [eqtb [13614 ].hh .v.RH 
  ]< 13 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 1157 ) ;
    } 
    {
      helpptr = 3 ;
      helpline [2 ]= 1158 ;
      helpline [1 ]= 1159 ;
      helpline [0 ]= 1160 ;
    } 
    error () ;
    flushmath () ;
    danger = true ;
  } 
  m = curlist .modefield ;
  l = false ;
  p = finmlist ( 0 ) ;
  if ( curlist .modefield == - (integer) m ) 
  {
    {
      getxtoken () ;
      if ( curcmd != 3 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 1161 ) ;
	} 
	{
	  helpptr = 2 ;
	  helpline [1 ]= 1162 ;
	  helpline [0 ]= 1163 ;
	} 
	backerror () ;
      } 
    } 
    curmlist = p ;
    curstyle = 2 ;
    mlistpenalties = false ;
    mlisttohlist () ;
    a = hpack ( mem [memtop - 3 ].hh .v.RH , 0 , 1 ) ;
    unsave () ;
    decr ( saveptr ) ;
    if ( savestack [saveptr + 0 ].cint == 1 ) 
    l = true ;
    danger = false ;
    if ( ( fontparams [eqtb [13581 ].hh .v.RH ]< 22 ) || ( fontparams [
    eqtb [13597 ].hh .v.RH ]< 22 ) || ( fontparams [eqtb [13613 ].hh 
    .v.RH ]< 22 ) ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 1153 ) ;
      } 
      {
	helpptr = 3 ;
	helpline [2 ]= 1154 ;
	helpline [1 ]= 1155 ;
	helpline [0 ]= 1156 ;
      } 
      error () ;
      flushmath () ;
      danger = true ;
    } 
    else if ( ( fontparams [eqtb [13582 ].hh .v.RH ]< 13 ) || ( fontparams 
    [eqtb [13598 ].hh .v.RH ]< 13 ) || ( fontparams [eqtb [13614 ].hh 
    .v.RH ]< 13 ) ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 1157 ) ;
      } 
      {
	helpptr = 3 ;
	helpline [2 ]= 1158 ;
	helpline [1 ]= 1159 ;
	helpline [0 ]= 1160 ;
      } 
      error () ;
      flushmath () ;
      danger = true ;
    } 
    m = curlist .modefield ;
    p = finmlist ( 0 ) ;
  } 
  else a = 0 ;
  if ( m < 0 ) 
  {
    {
      mem [curlist .tailfield ].hh .v.RH = newmath ( eqtb [15734 ].cint , 
      0 ) ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
    curmlist = p ;
    curstyle = 2 ;
    mlistpenalties = ( curlist .modefield > 0 ) ;
    mlisttohlist () ;
    mem [curlist .tailfield ].hh .v.RH = mem [memtop - 3 ].hh .v.RH ;
    while ( mem [curlist .tailfield ].hh .v.RH != 0 ) curlist .tailfield = 
    mem [curlist .tailfield ].hh .v.RH ;
    {
      mem [curlist .tailfield ].hh .v.RH = newmath ( eqtb [15734 ].cint , 
      1 ) ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
    curlist .auxfield .hh .v.LH = 1000 ;
    unsave () ;
  } 
  else {
      
    if ( a == 0 ) 
    {
      getxtoken () ;
      if ( curcmd != 3 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 1161 ) ;
	} 
	{
	  helpptr = 2 ;
	  helpline [1 ]= 1162 ;
	  helpline [0 ]= 1163 ;
	} 
	backerror () ;
      } 
    } 
    curmlist = p ;
    curstyle = 0 ;
    mlistpenalties = false ;
    mlisttohlist () ;
    p = mem [memtop - 3 ].hh .v.RH ;
    adjusttail = memtop - 5 ;
    b = hpack ( p , 0 , 1 ) ;
    p = mem [b + 5 ].hh .v.RH ;
    t = adjusttail ;
    adjusttail = 0 ;
    w = mem [b + 1 ].cint ;
    z = eqtb [15747 ].cint ;
    s = eqtb [15748 ].cint ;
    if ( ( a == 0 ) || danger ) 
    {
      e = 0 ;
      q = 0 ;
    } 
    else {
	
      e = mem [a + 1 ].cint ;
      q = e + fontinfo [6 + parambase [eqtb [13581 ].hh .v.RH ]].cint ;
    } 
    if ( w + q > z ) 
    {
      if ( ( e != 0 ) && ( ( w - totalshrink [0 ]+ q <= z ) || ( totalshrink 
      [1 ]!= 0 ) || ( totalshrink [2 ]!= 0 ) || ( totalshrink [3 ]!= 0 ) 
      ) ) 
      {
	freenode ( b , 7 ) ;
	b = hpack ( p , z - q , 0 ) ;
      } 
      else {
	  
	e = 0 ;
	if ( w > z ) 
	{
	  freenode ( b , 7 ) ;
	  b = hpack ( p , z , 0 ) ;
	} 
      } 
      w = mem [b + 1 ].cint ;
    } 
    d = half ( z - w ) ;
    if ( ( e > 0 ) && ( d < 2 * e ) ) 
    {
      d = half ( z - w - e ) ;
      if ( p != 0 ) 
      if ( ! ( p >= himemmin ) ) 
      if ( mem [p ].hh.b0 == 10 ) 
      d = 0 ;
    } 
    {
      mem [curlist .tailfield ].hh .v.RH = newpenalty ( eqtb [15174 ].cint 
      ) ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
    if ( ( d + s <= eqtb [15746 ].cint ) || l ) 
    {
      g1 = 3 ;
      g2 = 4 ;
    } 
    else {
	
      g1 = 5 ;
      g2 = 6 ;
    } 
    if ( l && ( e == 0 ) ) 
    {
      mem [a + 4 ].cint = s ;
      appendtovlist ( a ) ;
      {
	mem [curlist .tailfield ].hh .v.RH = newpenalty ( 10000 ) ;
	curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
      } 
    } 
    else {
	
      mem [curlist .tailfield ].hh .v.RH = newparamglue ( g1 ) ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
    if ( e != 0 ) 
    {
      r = newkern ( z - w - e - d ) ;
      if ( l ) 
      {
	mem [a ].hh .v.RH = r ;
	mem [r ].hh .v.RH = b ;
	b = a ;
	d = 0 ;
      } 
      else {
	  
	mem [b ].hh .v.RH = r ;
	mem [r ].hh .v.RH = a ;
      } 
      b = hpack ( b , 0 , 1 ) ;
    } 
    mem [b + 4 ].cint = s + d ;
    appendtovlist ( b ) ;
    if ( ( a != 0 ) && ( e == 0 ) && ! l ) 
    {
      {
	mem [curlist .tailfield ].hh .v.RH = newpenalty ( 10000 ) ;
	curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
      } 
      mem [a + 4 ].cint = s + z - mem [a + 1 ].cint ;
      appendtovlist ( a ) ;
      g2 = 0 ;
    } 
    if ( t != memtop - 5 ) 
    {
      mem [curlist .tailfield ].hh .v.RH = mem [memtop - 5 ].hh .v.RH ;
      curlist .tailfield = t ;
    } 
    {
      mem [curlist .tailfield ].hh .v.RH = newpenalty ( eqtb [15175 ].cint 
      ) ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
    if ( g2 > 0 ) 
    {
      mem [curlist .tailfield ].hh .v.RH = newparamglue ( g2 ) ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
    resumeafterdisplay () ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
resumeafterdisplay ( void ) 
#else
resumeafterdisplay ( ) 
#endif
{
  resumeafterdisplay_regmem 
  if ( curgroup != 15 ) 
  confusion ( 1164 ) ;
  unsave () ;
  curlist .pgfield = curlist .pgfield + 3 ;
  pushnest () ;
  curlist .modefield = 102 ;
  curlist .auxfield .hh .v.LH = 1000 ;
  if ( eqtb [15213 ].cint <= 0 ) 
  curlang = 0 ;
  else if ( eqtb [15213 ].cint > 255 ) 
  curlang = 0 ;
  else curlang = eqtb [15213 ].cint ;
  curlist .auxfield .hh .v.RH = curlang ;
  curlist .pgfield = ( normmin ( eqtb [15214 ].cint ) * 64 + normmin ( eqtb 
  [15215 ].cint ) ) * 65536L + curlang ;
  {
    getxtoken () ;
    if ( curcmd != 10 ) 
    backinput () ;
  } 
  if ( nestptr == 1 ) 
  buildpage () ;
} 
void 
#ifdef HAVE_PROTOTYPES
getrtoken ( void ) 
#else
getrtoken ( ) 
#endif
{
  /* 20 */ getrtoken_regmem 
  lab20: do {
      gettoken () ;
  } while ( ! ( curtok != 2592 ) ) ;
  if ( ( curcs == 0 ) || ( curcs > eqtbtop ) || ( ( curcs > 10514 ) && ( curcs 
  <= 16009 ) ) ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 1179 ) ;
    } 
    {
      helpptr = 5 ;
      helpline [4 ]= 1180 ;
      helpline [3 ]= 1181 ;
      helpline [2 ]= 1182 ;
      helpline [1 ]= 1183 ;
      helpline [0 ]= 1184 ;
    } 
    if ( curcs == 0 ) 
    backinput () ;
    curtok = 14609 ;
    inserror () ;
    goto lab20 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
trapzeroglue ( void ) 
#else
trapzeroglue ( ) 
#endif
{
  trapzeroglue_regmem 
  if ( ( mem [curval + 1 ].cint == 0 ) && ( mem [curval + 2 ].cint == 0 ) 
  && ( mem [curval + 3 ].cint == 0 ) ) 
  {
    incr ( mem [membot ].hh .v.RH ) ;
    deleteglueref ( curval ) ;
    curval = membot ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zdoregistercommand ( smallnumber a ) 
#else
zdoregistercommand ( a ) 
  smallnumber a ;
#endif
{
  /* 40 10 */ doregistercommand_regmem 
  halfword l, q, r, s  ;
  char p  ;
  q = curcmd ;
  {
    if ( q != 89 ) 
    {
      getxtoken () ;
      if ( ( curcmd >= 73 ) && ( curcmd <= 76 ) ) 
      {
	l = curchr ;
	p = curcmd - 73 ;
	goto lab40 ;
      } 
      if ( curcmd != 89 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 684 ) ;
	} 
	printcmdchr ( curcmd , curchr ) ;
	print ( 685 ) ;
	printcmdchr ( q , 0 ) ;
	{
	  helpptr = 1 ;
	  helpline [0 ]= 1208 ;
	} 
	error () ;
	return ;
      } 
    } 
    p = curchr ;
    scaneightbitint () ;
    switch ( p ) 
    {case 0 : 
      l = curval + 15221 ;
      break ;
    case 1 : 
      l = curval + 15754 ;
      break ;
    case 2 : 
      l = curval + 12544 ;
      break ;
    case 3 : 
      l = curval + 12800 ;
      break ;
    } 
  } 
  lab40: ;
  if ( q == 89 ) 
  scanoptionalequals () ;
  else if ( scankeyword ( 1204 ) ) 
  ;
  aritherror = false ;
  if ( q < 91 ) 
  if ( p < 2 ) 
  {
    if ( p == 0 ) 
    scanint () ;
    else scandimen ( false , false , false ) ;
    if ( q == 90 ) 
    curval = curval + eqtb [l ].cint ;
  } 
  else {
      
    scanglue ( p ) ;
    if ( q == 90 ) 
    {
      q = newspec ( curval ) ;
      r = eqtb [l ].hh .v.RH ;
      deleteglueref ( curval ) ;
      mem [q + 1 ].cint = mem [q + 1 ].cint + mem [r + 1 ].cint ;
      if ( mem [q + 2 ].cint == 0 ) 
      mem [q ].hh.b0 = 0 ;
      if ( mem [q ].hh.b0 == mem [r ].hh.b0 ) 
      mem [q + 2 ].cint = mem [q + 2 ].cint + mem [r + 2 ].cint ;
      else if ( ( mem [q ].hh.b0 < mem [r ].hh.b0 ) && ( mem [r + 2 ]
      .cint != 0 ) ) 
      {
	mem [q + 2 ].cint = mem [r + 2 ].cint ;
	mem [q ].hh.b0 = mem [r ].hh.b0 ;
      } 
      if ( mem [q + 3 ].cint == 0 ) 
      mem [q ].hh.b1 = 0 ;
      if ( mem [q ].hh.b1 == mem [r ].hh.b1 ) 
      mem [q + 3 ].cint = mem [q + 3 ].cint + mem [r + 3 ].cint ;
      else if ( ( mem [q ].hh.b1 < mem [r ].hh.b1 ) && ( mem [r + 3 ]
      .cint != 0 ) ) 
      {
	mem [q + 3 ].cint = mem [r + 3 ].cint ;
	mem [q ].hh.b1 = mem [r ].hh.b1 ;
      } 
      curval = q ;
    } 
  } 
  else {
      
    scanint () ;
    if ( p < 2 ) 
    if ( q == 91 ) 
    if ( p == 0 ) 
    curval = multandadd ( eqtb [l ].cint , curval , 0 , 2147483647L ) ;
    else curval = multandadd ( eqtb [l ].cint , curval , 0 , 1073741823L ) ;
    else curval = xovern ( eqtb [l ].cint , curval ) ;
    else {
	
      s = eqtb [l ].hh .v.RH ;
      r = newspec ( s ) ;
      if ( q == 91 ) 
      {
	mem [r + 1 ].cint = multandadd ( mem [s + 1 ].cint , curval , 0 , 
	1073741823L ) ;
	mem [r + 2 ].cint = multandadd ( mem [s + 2 ].cint , curval , 0 , 
	1073741823L ) ;
	mem [r + 3 ].cint = multandadd ( mem [s + 3 ].cint , curval , 0 , 
	1073741823L ) ;
      } 
      else {
	  
	mem [r + 1 ].cint = xovern ( mem [s + 1 ].cint , curval ) ;
	mem [r + 2 ].cint = xovern ( mem [s + 2 ].cint , curval ) ;
	mem [r + 3 ].cint = xovern ( mem [s + 3 ].cint , curval ) ;
      } 
      curval = r ;
    } 
  } 
  if ( aritherror ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 1205 ) ;
    } 
    {
      helpptr = 2 ;
      helpline [1 ]= 1206 ;
      helpline [0 ]= 1207 ;
    } 
    error () ;
    return ;
  } 
  if ( p < 2 ) 
  if ( ( a >= 4 ) ) 
  geqworddefine ( l , curval ) ;
  else eqworddefine ( l , curval ) ;
  else {
      
    trapzeroglue () ;
    if ( ( a >= 4 ) ) 
    geqdefine ( l , 117 , curval ) ;
    else eqdefine ( l , 117 , curval ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
alteraux ( void ) 
#else
alteraux ( ) 
#endif
{
  alteraux_regmem 
  halfword c  ;
  if ( curchr != abs ( curlist .modefield ) ) 
  reportillegalcase () ;
  else {
      
    c = curchr ;
    scanoptionalequals () ;
    if ( c == 1 ) 
    {
      scandimen ( false , false , false ) ;
      curlist .auxfield .cint = curval ;
    } 
    else {
	
      scanint () ;
      if ( ( curval <= 0 ) || ( curval > 32767 ) ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 1211 ) ;
	} 
	{
	  helpptr = 1 ;
	  helpline [0 ]= 1212 ;
	} 
	interror ( curval ) ;
      } 
      else curlist .auxfield .hh .v.LH = curval ;
    } 
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
alterprevgraf ( void ) 
#else
alterprevgraf ( ) 
#endif
{
  alterprevgraf_regmem 
  integer p  ;
  nest [nestptr ]= curlist ;
  p = nestptr ;
  while ( abs ( nest [p ].modefield ) != 1 ) decr ( p ) ;
  scanoptionalequals () ;
  scanint () ;
  if ( curval < 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 950 ) ;
    } 
    printesc ( 532 ) ;
    {
      helpptr = 1 ;
      helpline [0 ]= 1213 ;
    } 
    interror ( curval ) ;
  } 
  else {
      
    nest [p ].pgfield = curval ;
    curlist = nest [nestptr ];
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
alterpagesofar ( void ) 
#else
alterpagesofar ( ) 
#endif
{
  alterpagesofar_regmem 
  char c  ;
  c = curchr ;
  scanoptionalequals () ;
  scandimen ( false , false , false ) ;
  pagesofar [c ]= curval ;
} 
void 
#ifdef HAVE_PROTOTYPES
alterinteger ( void ) 
#else
alterinteger ( ) 
#endif
{
  alterinteger_regmem 
  char c  ;
  c = curchr ;
  scanoptionalequals () ;
  scanint () ;
  if ( c == 0 ) 
  deadcycles = curval ;
  else insertpenalties = curval ;
} 
void 
#ifdef HAVE_PROTOTYPES
alterboxdimen ( void ) 
#else
alterboxdimen ( ) 
#endif
{
  alterboxdimen_regmem 
  smallnumber c  ;
  eightbits b  ;
  c = curchr ;
  scaneightbitint () ;
  b = curval ;
  scanoptionalequals () ;
  scandimen ( false , false , false ) ;
  if ( eqtb [13322 + b ].hh .v.RH != 0 ) 
  mem [eqtb [13322 + b ].hh .v.RH + c ].cint = curval ;
} 
void 
#ifdef HAVE_PROTOTYPES
znewfont ( smallnumber a ) 
#else
znewfont ( a ) 
  smallnumber a ;
#endif
{
  /* 50 */ newfont_regmem 
  halfword u  ;
  scaled s  ;
  internalfontnumber f  ;
  strnumber t  ;
  char oldsetting  ;
  strnumber flushablestring  ;
  if ( jobname == 0 ) 
  openlogfile () ;
  getrtoken () ;
  u = curcs ;
  if ( u >= 514 ) 
  t = hash [u ].v.RH ;
  else if ( u >= 257 ) 
  if ( u == 513 ) 
  t = 1217 ;
  else t = u - 257 ;
  else {
      
    oldsetting = selector ;
    selector = 21 ;
    print ( 1217 ) ;
    print ( u - 1 ) ;
    selector = oldsetting ;
    {
      if ( poolptr + 1 > poolsize ) 
      overflow ( 257 , poolsize - initpoolptr ) ;
    } 
    t = makestring () ;
  } 
  if ( ( a >= 4 ) ) 
  geqdefine ( u , 87 , 0 ) ;
  else eqdefine ( u , 87 , 0 ) ;
  scanoptionalequals () ;
  scanfilename () ;
  nameinprogress = true ;
  if ( scankeyword ( 1218 ) ) 
  {
    scandimen ( false , false , false ) ;
    s = curval ;
    if ( ( s <= 0 ) || ( s >= 134217728L ) ) 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 1220 ) ;
      } 
      printscaled ( s ) ;
      print ( 1221 ) ;
      {
	helpptr = 2 ;
	helpline [1 ]= 1222 ;
	helpline [0 ]= 1223 ;
      } 
      error () ;
      s = 10 * 65536L ;
    } 
  } 
  else if ( scankeyword ( 1219 ) ) 
  {
    scanint () ;
    s = - (integer) curval ;
    if ( ( curval <= 0 ) || ( curval > 32768L ) ) 
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
      interror ( curval ) ;
      s = -1000 ;
    } 
  } 
  else s = -1000 ;
  nameinprogress = false ;
  {register integer for_end; f = 1 ;for_end = fontptr ; if ( f <= for_end) 
  do 
    if ( streqstr ( fontname [f ], curname ) && streqstr ( fontarea [f ], 
    curarea ) ) 
    {
      if ( s > 0 ) 
      {
	if ( s == fontsize [f ]) 
	goto lab50 ;
      } 
      else if ( fontsize [f ]== xnoverd ( fontdsize [f ], - (integer) s , 
      1000 ) ) 
      goto lab50 ;
    } 
  while ( f++ < for_end ) ;} 
  f = readfontinfo ( u , curname , curarea , s ) ;
  lab50: eqtb [u ].hh .v.RH = f ;
  eqtb [10524 + f ]= eqtb [u ];
  hash [10524 + f ].v.RH = t ;
} 
void 
#ifdef HAVE_PROTOTYPES
newinteraction ( void ) 
#else
newinteraction ( ) 
#endif
{
  newinteraction_regmem 
  println () ;
  interaction = curchr ;
  if ( interaction == 0 ) 
  kpsemaketexdiscarderrors = 1 ;
  else kpsemaketexdiscarderrors = 0 ;
  if ( interaction == 0 ) 
  selector = 16 ;
  else selector = 17 ;
  if ( logopened ) 
  selector = selector + 2 ;
} 
void 
#ifdef HAVE_PROTOTYPES
doassignments ( void ) 
#else
doassignments ( ) 
#endif
{
  /* 10 */ doassignments_regmem 
  while ( true ) {
      
    do {
	getxtoken () ;
    } while ( ! ( ( curcmd != 10 ) && ( curcmd != 0 ) ) ) ;
    if ( curcmd <= 70 ) 
    return ;
    setboxallowed = false ;
    prefixedcommand () ;
    setboxallowed = true ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
openorclosein ( void ) 
#else
openorclosein ( ) 
#endif
{
  openorclosein_regmem 
  char c  ;
  char n  ;
  c = curchr ;
  scanfourbitint () ;
  n = curval ;
  if ( readopen [n ]!= 2 ) 
  {
    aclose ( readfile [n ]) ;
    readopen [n ]= 2 ;
  } 
  if ( c != 0 ) 
  {
    scanoptionalequals () ;
    scanfilename () ;
    packfilename ( curname , curarea , curext ) ;
    texinputtype = 0 ;
    if ( aopenin ( readfile [n ], kpsetexformat ) ) 
    readopen [n ]= 1 ;
  } 
} 
