#define EXTERN extern
#include "texd.h"

void zcharwarning ( f , c ) 
internalfontnumber f ; 
eightbits c ; 
{charwarning_regmem 
  if ( eqtb [ 12698 ] .cint > 0 ) 
  {
    begindiagnostic () ; 
    printnl ( 818 ) ; 
    print ( c ) ; 
    print ( 819 ) ; 
    slowprint ( fontname [ f ] ) ; 
    printchar ( 33 ) ; 
    enddiagnostic ( false ) ; 
  } 
} 
halfword znewcharacter ( f , c ) 
internalfontnumber f ; 
eightbits c ; 
{/* 10 */ register halfword Result; newcharacter_regmem 
  halfword p  ; 
  if ( fontbc [ f ] <= c ) 
  if ( fontec [ f ] >= c ) 
  if ( ( fontinfo [ charbase [ f ] + c ] .qqqq .b0 > 0 ) ) 
  {
    p = getavail () ; 
    mem [ p ] .hh.b0 = f ; 
    mem [ p ] .hh.b1 = c ; 
    Result = p ; 
    return(Result) ; 
  } 
  charwarning ( f , c ) ; 
  Result = 0 ; 
  return(Result) ; 
} 
void dviswap ( ) 
{dviswap_regmem 
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
void zdvifour ( x ) 
integer x ; 
{dvifour_regmem 
  if ( x >= 0 ) 
  {
    dvibuf [ dviptr ] = x / 16777216L ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  else {
      
    x = x + 1073741824L ; 
    x = x + 1073741824L ; 
    {
      dvibuf [ dviptr ] = ( x / 16777216L ) + 128 ; 
      incr ( dviptr ) ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
  } 
  x = x % 16777216L ; 
  {
    dvibuf [ dviptr ] = x / 65536L ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  x = x % 65536L ; 
  {
    dvibuf [ dviptr ] = x / 256 ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {
    dvibuf [ dviptr ] = x % 256 ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
} 
void zdvipop ( l ) 
integer l ; 
{dvipop_regmem 
  if ( ( l == dvioffset + dviptr ) && ( dviptr > 0 ) ) 
  decr ( dviptr ) ; 
  else {
      
    dvibuf [ dviptr ] = 142 ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
} 
void zdvifontdef ( f ) 
internalfontnumber f ; 
{dvifontdef_regmem 
  poolpointer k  ; 
  {
    dvibuf [ dviptr ] = 243 ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {
    dvibuf [ dviptr ] = f - 1 ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {
    dvibuf [ dviptr ] = fontcheck [ f ] .b0 ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {
    dvibuf [ dviptr ] = fontcheck [ f ] .b1 ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {
    dvibuf [ dviptr ] = fontcheck [ f ] .b2 ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {
    dvibuf [ dviptr ] = fontcheck [ f ] .b3 ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  dvifour ( fontsize [ f ] ) ; 
  dvifour ( fontdsize [ f ] ) ; 
  {
    dvibuf [ dviptr ] = ( strstart [ fontarea [ f ] + 1 ] - strstart [ 
    fontarea [ f ] ] ) ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {
    dvibuf [ dviptr ] = ( strstart [ fontname [ f ] + 1 ] - strstart [ 
    fontname [ f ] ] ) ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {register integer for_end; k = strstart [ fontarea [ f ] ] ; for_end = 
  strstart [ fontarea [ f ] + 1 ] - 1 ; if ( k <= for_end) do 
    {
      dvibuf [ dviptr ] = strpool [ k ] ; 
      incr ( dviptr ) ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
  while ( k++ < for_end ) ; } 
  {register integer for_end; k = strstart [ fontname [ f ] ] ; for_end = 
  strstart [ fontname [ f ] + 1 ] - 1 ; if ( k <= for_end) do 
    {
      dvibuf [ dviptr ] = strpool [ k ] ; 
      incr ( dviptr ) ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
  while ( k++ < for_end ) ; } 
} 
void zmovement ( w , o ) 
scaled w ; 
eightbits o ; 
{/* 10 40 45 2 1 */ movement_regmem 
  smallnumber mstate  ; 
  halfword p, q  ; 
  integer k  ; 
  q = getnode ( 3 ) ; 
  mem [ q + 1 ] .cint = w ; 
  mem [ q + 2 ] .cint = dvioffset + dviptr ; 
  if ( o == 157 ) 
  {
    mem [ q ] .hh .v.RH = downptr ; 
    downptr = q ; 
  } 
  else {
      
    mem [ q ] .hh .v.RH = rightptr ; 
    rightptr = q ; 
  } 
  p = mem [ q ] .hh .v.RH ; 
  mstate = 0 ; 
  while ( p != 0 ) {
      
    if ( mem [ p + 1 ] .cint == w ) 
    switch ( mstate + mem [ p ] .hh .v.LH ) 
    {case 3 : 
    case 4 : 
    case 15 : 
    case 16 : 
      if ( mem [ p + 2 ] .cint < dvigone ) 
      goto lab45 ; 
      else {
	  
	k = mem [ p + 2 ] .cint - dvioffset ; 
	if ( k < 0 ) 
	k = k + dvibufsize ; 
	dvibuf [ k ] = dvibuf [ k ] + 5 ; 
	mem [ p ] .hh .v.LH = 1 ; 
	goto lab40 ; 
      } 
      break ; 
    case 5 : 
    case 9 : 
    case 11 : 
      if ( mem [ p + 2 ] .cint < dvigone ) 
      goto lab45 ; 
      else {
	  
	k = mem [ p + 2 ] .cint - dvioffset ; 
	if ( k < 0 ) 
	k = k + dvibufsize ; 
	dvibuf [ k ] = dvibuf [ k ] + 10 ; 
	mem [ p ] .hh .v.LH = 2 ; 
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
    else switch ( mstate + mem [ p ] .hh .v.LH ) 
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
    p = mem [ p ] .hh .v.RH ; 
  } 
  lab45: ; 
  mem [ q ] .hh .v.LH = 3 ; 
  if ( abs ( w ) >= 8388608L ) 
  {
    {
      dvibuf [ dviptr ] = o + 3 ; 
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
      dvibuf [ dviptr ] = o + 2 ; 
      incr ( dviptr ) ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
    if ( w < 0 ) 
    w = w + 16777216L ; 
    {
      dvibuf [ dviptr ] = w / 65536L ; 
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
      dvibuf [ dviptr ] = o + 1 ; 
      incr ( dviptr ) ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
    if ( w < 0 ) 
    w = w + 65536L ; 
    goto lab2 ; 
  } 
  {
    dvibuf [ dviptr ] = o ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  if ( w < 0 ) 
  w = w + 256 ; 
  goto lab1 ; 
  lab2: {
      
    dvibuf [ dviptr ] = w / 256 ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  lab1: {
      
    dvibuf [ dviptr ] = w % 256 ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  return ; 
  lab40: mem [ q ] .hh .v.LH = mem [ p ] .hh .v.LH ; 
  if ( mem [ q ] .hh .v.LH == 1 ) 
  {
    {
      dvibuf [ dviptr ] = o + 4 ; 
      incr ( dviptr ) ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
    while ( mem [ q ] .hh .v.RH != p ) {
	
      q = mem [ q ] .hh .v.RH ; 
      switch ( mem [ q ] .hh .v.LH ) 
      {case 3 : 
	mem [ q ] .hh .v.LH = 5 ; 
	break ; 
      case 4 : 
	mem [ q ] .hh .v.LH = 6 ; 
	break ; 
	default: 
	; 
	break ; 
      } 
    } 
  } 
  else {
      
    {
      dvibuf [ dviptr ] = o + 9 ; 
      incr ( dviptr ) ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
    while ( mem [ q ] .hh .v.RH != p ) {
	
      q = mem [ q ] .hh .v.RH ; 
      switch ( mem [ q ] .hh .v.LH ) 
      {case 3 : 
	mem [ q ] .hh .v.LH = 4 ; 
	break ; 
      case 5 : 
	mem [ q ] .hh .v.LH = 6 ; 
	break ; 
	default: 
	; 
	break ; 
      } 
    } 
  } 
} 
void zprunemovements ( l ) 
integer l ; 
{/* 30 10 */ prunemovements_regmem 
  halfword p  ; 
  while ( downptr != 0 ) {
      
    if ( mem [ downptr + 2 ] .cint < l ) 
    goto lab30 ; 
    p = downptr ; 
    downptr = mem [ p ] .hh .v.RH ; 
    freenode ( p , 3 ) ; 
  } 
  lab30: while ( rightptr != 0 ) {
      
    if ( mem [ rightptr + 2 ] .cint < l ) 
    return ; 
    p = rightptr ; 
    rightptr = mem [ p ] .hh .v.RH ; 
    freenode ( p , 3 ) ; 
  } 
} 
void zspecialout ( p ) 
halfword p ; 
{specialout_regmem 
  schar oldsetting  ; 
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
  showtokenlist ( mem [ mem [ p + 1 ] .hh .v.RH ] .hh .v.RH , 0 , poolsize - 
  poolptr ) ; 
  selector = oldsetting ; 
  {
    if ( poolptr + 1 > poolsize ) 
    overflow ( 257 , poolsize - initpoolptr ) ; 
  } 
  if ( ( poolptr - strstart [ strptr ] ) < 256 ) 
  {
    {
      dvibuf [ dviptr ] = 239 ; 
      incr ( dviptr ) ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
    {
      dvibuf [ dviptr ] = ( poolptr - strstart [ strptr ] ) ; 
      incr ( dviptr ) ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
  } 
  else {
      
    {
      dvibuf [ dviptr ] = 242 ; 
      incr ( dviptr ) ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
    dvifour ( ( poolptr - strstart [ strptr ] ) ) ; 
  } 
  {register integer for_end; k = strstart [ strptr ] ; for_end = poolptr - 1 
  ; if ( k <= for_end) do 
    {
      dvibuf [ dviptr ] = strpool [ k ] ; 
      incr ( dviptr ) ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
  while ( k++ < for_end ) ; } 
  poolptr = strstart [ strptr ] ; 
} 
void zwriteout ( p ) 
halfword p ; 
{writeout_regmem 
  schar oldsetting  ; 
  integer oldmode  ; 
  smallnumber j  ; 
  halfword q, r  ; 
  q = getavail () ; 
  mem [ q ] .hh .v.LH = 637 ; 
  r = getavail () ; 
  mem [ q ] .hh .v.RH = r ; 
  mem [ r ] .hh .v.LH = 14117 ; 
  begintokenlist ( q , 4 ) ; 
  begintokenlist ( mem [ p + 1 ] .hh .v.RH , 15 ) ; 
  q = getavail () ; 
  mem [ q ] .hh .v.LH = 379 ; 
  begintokenlist ( q , 4 ) ; 
  oldmode = curlist .modefield ; 
  curlist .modefield = 0 ; 
  curcs = writeloc ; 
  q = scantoks ( false , true ) ; 
  gettoken () ; 
  if ( curtok != 14117 ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 1289 ) ; 
    } 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 1290 ; 
      helpline [ 0 ] = 1005 ; 
    } 
    error () ; 
    do {
	gettoken () ; 
    } while ( ! ( curtok == 14117 ) ) ; 
  } 
  curlist .modefield = oldmode ; 
  endtokenlist () ; 
  oldsetting = selector ; 
  j = mem [ p + 1 ] .hh .v.LH ; 
  if ( writeopen [ j ] ) 
  selector = j ; 
  else {
      
    if ( ( j == 17 ) && ( selector == 19 ) ) 
    selector = 18 ; 
    printnl ( 335 ) ; 
  } 
  tokenshow ( defref ) ; 
  println () ; 
  flushlist ( defref ) ; 
  selector = oldsetting ; 
} 
void zoutwhat ( p ) 
halfword p ; 
{outwhat_regmem 
  smallnumber j  ; 
  switch ( mem [ p ] .hh.b1 ) 
  {case 0 : 
  case 1 : 
  case 2 : 
    if ( ! doingleaders ) 
    {
      j = mem [ p + 1 ] .hh .v.LH ; 
      if ( mem [ p ] .hh.b1 == 1 ) 
      writeout ( p ) ; 
      else {
	  
	if ( writeopen [ j ] ) 
	aclose ( writefile [ j ] ) ; 
	if ( mem [ p ] .hh.b1 == 2 ) 
	writeopen [ j ] = false ; 
	else if ( j < 16 ) 
	{
	  curname = mem [ p + 1 ] .hh .v.RH ; 
	  curarea = mem [ p + 2 ] .hh .v.LH ; 
	  curext = mem [ p + 2 ] .hh .v.RH ; 
	  if ( curext == 335 ) 
	  curext = 784 ; 
	  packfilename ( curname , curarea , curext ) ; 
	  while ( ! aopenout ( writefile [ j ] ) ) promptfilename ( 1292 , 784 
	  ) ; 
	  writeopen [ j ] = true ; 
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
    confusion ( 1291 ) ; 
    break ; 
  } 
} 
void hlistout ( ) 
{/* 21 13 14 15 */ hlistout_regmem 
  scaled baseline  ; 
  scaled leftedge  ; 
  scaled saveh, savev  ; 
  halfword thisbox  ; 
  glueord gorder  ; 
  schar gsign  ; 
  halfword p  ; 
  integer saveloc  ; 
  halfword leaderbox  ; 
  scaled leaderwd  ; 
  scaled lx  ; 
  boolean outerdoingleaders  ; 
  scaled edge  ; 
  thisbox = tempptr ; 
  gorder = mem [ thisbox + 5 ] .hh.b1 ; 
  gsign = mem [ thisbox + 5 ] .hh.b0 ; 
  p = mem [ thisbox + 5 ] .hh .v.RH ; 
  incr ( curs ) ; 
  if ( curs > 0 ) 
  {
    dvibuf [ dviptr ] = 141 ; 
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
	f = mem [ p ] .hh.b0 ; 
      c = mem [ p ] .hh.b1 ; 
      if ( f != dvif ) 
      {
	if ( ! fontused [ f ] ) 
	{
	  dvifontdef ( f ) ; 
	  fontused [ f ] = true ; 
	} 
	if ( f <= 64 ) 
	{
	  dvibuf [ dviptr ] = f + 170 ; 
	  incr ( dviptr ) ; 
	  if ( dviptr == dvilimit ) 
	  dviswap () ; 
	} 
	else {
	    
	  {
	    dvibuf [ dviptr ] = 235 ; 
	    incr ( dviptr ) ; 
	    if ( dviptr == dvilimit ) 
	    dviswap () ; 
	  } 
	  {
	    dvibuf [ dviptr ] = f - 1 ; 
	    incr ( dviptr ) ; 
	    if ( dviptr == dvilimit ) 
	    dviswap () ; 
	  } 
	} 
	dvif = f ; 
      } 
      if ( c >= 128 ) 
      {
	dvibuf [ dviptr ] = 128 ; 
	incr ( dviptr ) ; 
	if ( dviptr == dvilimit ) 
	dviswap () ; 
      } 
      {
	dvibuf [ dviptr ] = c ; 
	incr ( dviptr ) ; 
	if ( dviptr == dvilimit ) 
	dviswap () ; 
      } 
      curh = curh + fontinfo [ widthbase [ f ] + fontinfo [ charbase [ f ] + c 
      ] .qqqq .b0 ] .cint ; 
      p = mem [ p ] .hh .v.RH ; 
    } while ( ! ( ! ( p >= himemmin ) ) ) ; 
    dvih = curh ; 
  } 
  else {
      
    switch ( mem [ p ] .hh.b0 ) 
    {case 0 : 
    case 1 : 
      if ( mem [ p + 5 ] .hh .v.RH == 0 ) 
      curh = curh + mem [ p + 1 ] .cint ; 
      else {
	  
	saveh = dvih ; 
	savev = dviv ; 
	curv = baseline + mem [ p + 4 ] .cint ; 
	tempptr = p ; 
	edge = curh ; 
	if ( mem [ p ] .hh.b0 == 1 ) 
	vlistout () ; 
	else hlistout () ; 
	dvih = saveh ; 
	dviv = savev ; 
	curh = edge + mem [ p + 1 ] .cint ; 
	curv = baseline ; 
      } 
      break ; 
    case 2 : 
      {
	ruleht = mem [ p + 3 ] .cint ; 
	ruledp = mem [ p + 2 ] .cint ; 
	rulewd = mem [ p + 1 ] .cint ; 
	goto lab14 ; 
      } 
      break ; 
    case 8 : 
      outwhat ( p ) ; 
      break ; 
    case 10 : 
      {
	g = mem [ p + 1 ] .hh .v.LH ; 
	rulewd = mem [ g + 1 ] .cint ; 
	if ( gsign != 0 ) 
	{
	  if ( gsign == 1 ) 
	  {
	    if ( mem [ g ] .hh.b0 == gorder ) 
	    rulewd = rulewd + round ( mem [ thisbox + 6 ] .gr * mem [ g + 2 ] 
	    .cint ) ; 
	  } 
	  else {
	      
	    if ( mem [ g ] .hh.b1 == gorder ) 
	    rulewd = rulewd - round ( mem [ thisbox + 6 ] .gr * mem [ g + 3 ] 
	    .cint ) ; 
	  } 
	} 
	if ( mem [ p ] .hh.b1 >= 100 ) 
	{
	  leaderbox = mem [ p + 1 ] .hh .v.RH ; 
	  if ( mem [ leaderbox ] .hh.b0 == 2 ) 
	  {
	    ruleht = mem [ leaderbox + 3 ] .cint ; 
	    ruledp = mem [ leaderbox + 2 ] .cint ; 
	    goto lab14 ; 
	  } 
	  leaderwd = mem [ leaderbox + 1 ] .cint ; 
	  if ( ( leaderwd > 0 ) && ( rulewd > 0 ) ) 
	  {
	    rulewd = rulewd + 10 ; 
	    edge = curh + rulewd ; 
	    lx = 0 ; 
	    if ( mem [ p ] .hh.b1 == 100 ) 
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
	      if ( mem [ p ] .hh.b1 == 101 ) 
	      curh = curh + ( lr / 2 ) ; 
	      else {
		  
		lx = ( 2 * lr + lq + 1 ) / ( 2 * lq + 2 ) ; 
		curh = curh + ( ( lr - ( lq - 1 ) * lx ) / 2 ) ; 
	      } 
	    } 
	    while ( curh + leaderwd <= edge ) {
		
	      curv = baseline + mem [ leaderbox + 4 ] .cint ; 
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
	      if ( mem [ leaderbox ] .hh.b0 == 1 ) 
	      vlistout () ; 
	      else hlistout () ; 
	      doingleaders = outerdoingleaders ; 
	      dviv = savev ; 
	      dvih = saveh ; 
	      curv = savev ; 
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
      curh = curh + mem [ p + 1 ] .cint ; 
      break ; 
    case 6 : 
      {
	mem [ memtop - 12 ] = mem [ p + 1 ] ; 
	mem [ memtop - 12 ] .hh .v.RH = mem [ p ] .hh .v.RH ; 
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
    ruleht = mem [ thisbox + 3 ] .cint ; 
    if ( ( ruledp == -1073741824L ) ) 
    ruledp = mem [ thisbox + 2 ] .cint ; 
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
	dvibuf [ dviptr ] = 132 ; 
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
    lab15: p = mem [ p ] .hh .v.RH ; 
  } 
  prunemovements ( saveloc ) ; 
  if ( curs > 0 ) 
  dvipop ( saveloc ) ; 
  decr ( curs ) ; 
} 
void vlistout ( ) 
{/* 13 14 15 */ vlistout_regmem 
  scaled leftedge  ; 
  scaled topedge  ; 
  scaled saveh, savev  ; 
  halfword thisbox  ; 
  glueord gorder  ; 
  schar gsign  ; 
  halfword p  ; 
  integer saveloc  ; 
  halfword leaderbox  ; 
  scaled leaderht  ; 
  scaled lx  ; 
  boolean outerdoingleaders  ; 
  scaled edge  ; 
  thisbox = tempptr ; 
  gorder = mem [ thisbox + 5 ] .hh.b1 ; 
  gsign = mem [ thisbox + 5 ] .hh.b0 ; 
  p = mem [ thisbox + 5 ] .hh .v.RH ; 
  incr ( curs ) ; 
  if ( curs > 0 ) 
  {
    dvibuf [ dviptr ] = 141 ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  if ( curs > maxpush ) 
  maxpush = curs ; 
  saveloc = dvioffset + dviptr ; 
  leftedge = curh ; 
  curv = curv - mem [ thisbox + 3 ] .cint ; 
  topedge = curv ; 
  while ( p != 0 ) {
      
    if ( ( p >= himemmin ) ) 
    confusion ( 821 ) ; 
    else {
	
      switch ( mem [ p ] .hh.b0 ) 
      {case 0 : 
      case 1 : 
	if ( mem [ p + 5 ] .hh .v.RH == 0 ) 
	curv = curv + mem [ p + 3 ] .cint + mem [ p + 2 ] .cint ; 
	else {
	    
	  curv = curv + mem [ p + 3 ] .cint ; 
	  if ( curv != dviv ) 
	  {
	    movement ( curv - dviv , 157 ) ; 
	    dviv = curv ; 
	  } 
	  saveh = dvih ; 
	  savev = dviv ; 
	  curh = leftedge + mem [ p + 4 ] .cint ; 
	  tempptr = p ; 
	  if ( mem [ p ] .hh.b0 == 1 ) 
	  vlistout () ; 
	  else hlistout () ; 
	  dvih = saveh ; 
	  dviv = savev ; 
	  curv = savev + mem [ p + 2 ] .cint ; 
	  curh = leftedge ; 
	} 
	break ; 
      case 2 : 
	{
	  ruleht = mem [ p + 3 ] .cint ; 
	  ruledp = mem [ p + 2 ] .cint ; 
	  rulewd = mem [ p + 1 ] .cint ; 
	  goto lab14 ; 
	} 
	break ; 
      case 8 : 
	outwhat ( p ) ; 
	break ; 
      case 10 : 
	{
	  g = mem [ p + 1 ] .hh .v.LH ; 
	  ruleht = mem [ g + 1 ] .cint ; 
	  if ( gsign != 0 ) 
	  {
	    if ( gsign == 1 ) 
	    {
	      if ( mem [ g ] .hh.b0 == gorder ) 
	      ruleht = ruleht + round ( mem [ thisbox + 6 ] .gr * mem [ g + 2 
	      ] .cint ) ; 
	    } 
	    else {
		
	      if ( mem [ g ] .hh.b1 == gorder ) 
	      ruleht = ruleht - round ( mem [ thisbox + 6 ] .gr * mem [ g + 3 
	      ] .cint ) ; 
	    } 
	  } 
	  if ( mem [ p ] .hh.b1 >= 100 ) 
	  {
	    leaderbox = mem [ p + 1 ] .hh .v.RH ; 
	    if ( mem [ leaderbox ] .hh.b0 == 2 ) 
	    {
	      rulewd = mem [ leaderbox + 1 ] .cint ; 
	      ruledp = 0 ; 
	      goto lab14 ; 
	    } 
	    leaderht = mem [ leaderbox + 3 ] .cint + mem [ leaderbox + 2 ] 
	    .cint ; 
	    if ( ( leaderht > 0 ) && ( ruleht > 0 ) ) 
	    {
	      ruleht = ruleht + 10 ; 
	      edge = curv + ruleht ; 
	      lx = 0 ; 
	      if ( mem [ p ] .hh.b1 == 100 ) 
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
		if ( mem [ p ] .hh.b1 == 101 ) 
		curv = curv + ( lr / 2 ) ; 
		else {
		    
		  lx = ( 2 * lr + lq + 1 ) / ( 2 * lq + 2 ) ; 
		  curv = curv + ( ( lr - ( lq - 1 ) * lx ) / 2 ) ; 
		} 
	      } 
	      while ( curv + leaderht <= edge ) {
		  
		curh = leftedge + mem [ leaderbox + 4 ] .cint ; 
		if ( curh != dvih ) 
		{
		  movement ( curh - dvih , 143 ) ; 
		  dvih = curh ; 
		} 
		saveh = dvih ; 
		curv = curv + mem [ leaderbox + 3 ] .cint ; 
		if ( curv != dviv ) 
		{
		  movement ( curv - dviv , 157 ) ; 
		  dviv = curv ; 
		} 
		savev = dviv ; 
		tempptr = leaderbox ; 
		outerdoingleaders = doingleaders ; 
		doingleaders = true ; 
		if ( mem [ leaderbox ] .hh.b0 == 1 ) 
		vlistout () ; 
		else hlistout () ; 
		doingleaders = outerdoingleaders ; 
		dviv = savev ; 
		dvih = saveh ; 
		curh = saveh ; 
		curv = savev - mem [ leaderbox + 3 ] .cint + leaderht + lx ; 
	      } 
	      curv = edge - 10 ; 
	      goto lab15 ; 
	    } 
	  } 
	  goto lab13 ; 
	} 
	break ; 
      case 11 : 
	curv = curv + mem [ p + 1 ] .cint ; 
	break ; 
	default: 
	; 
	break ; 
      } 
      goto lab15 ; 
      lab14: if ( ( rulewd == -1073741824L ) ) 
      rulewd = mem [ thisbox + 1 ] .cint ; 
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
	  dvibuf [ dviptr ] = 137 ; 
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
    lab15: p = mem [ p ] .hh .v.RH ; 
  } 
  prunemovements ( saveloc ) ; 
  if ( curs > 0 ) 
  dvipop ( saveloc ) ; 
  decr ( curs ) ; 
} 
void zshipout ( p ) 
halfword p ; 
{/* 30 */ shipout_regmem 
  integer pageloc  ; 
  schar j, k  ; 
  poolpointer s  ; 
  schar oldsetting  ; 
  if ( eqtb [ 12697 ] .cint > 0 ) 
  {
    printnl ( 335 ) ; 
    println () ; 
    print ( 822 ) ; 
  } 
  if ( termoffset > maxprintline - 9 ) 
  println () ; 
  else if ( ( termoffset > 0 ) || ( fileoffset > 0 ) ) 
  printchar ( 32 ) ; 
  printchar ( 91 ) ; 
  j = 9 ; 
  while ( ( eqtb [ 12718 + j ] .cint == 0 ) && ( j > 0 ) ) decr ( j ) ; 
  {register integer for_end; k = 0 ; for_end = j ; if ( k <= for_end) do 
    {
      printint ( eqtb [ 12718 + k ] .cint ) ; 
      if ( k < j ) 
      printchar ( 46 ) ; 
    } 
  while ( k++ < for_end ) ; } 
  flush ( stdout ) ; 
  if ( eqtb [ 12697 ] .cint > 0 ) 
  {
    printchar ( 93 ) ; 
    begindiagnostic () ; 
    showbox ( p ) ; 
    enddiagnostic ( true ) ; 
  } 
  if ( ( mem [ p + 3 ] .cint > 1073741823L ) || ( mem [ p + 2 ] .cint > 
  1073741823L ) || ( mem [ p + 3 ] .cint + mem [ p + 2 ] .cint + eqtb [ 13249 
  ] .cint > 1073741823L ) || ( mem [ p + 1 ] .cint + eqtb [ 13248 ] .cint > 
  1073741823L ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 826 ) ; 
    } 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 827 ; 
      helpline [ 0 ] = 828 ; 
    } 
    error () ; 
    if ( eqtb [ 12697 ] .cint <= 0 ) 
    {
      begindiagnostic () ; 
      printnl ( 829 ) ; 
      showbox ( p ) ; 
      enddiagnostic ( true ) ; 
    } 
    goto lab30 ; 
  } 
  if ( mem [ p + 3 ] .cint + mem [ p + 2 ] .cint + eqtb [ 13249 ] .cint > maxv 
  ) 
  maxv = mem [ p + 3 ] .cint + mem [ p + 2 ] .cint + eqtb [ 13249 ] .cint ; 
  if ( mem [ p + 1 ] .cint + eqtb [ 13248 ] .cint > maxh ) 
  maxh = mem [ p + 1 ] .cint + eqtb [ 13248 ] .cint ; 
  dvih = 0 ; 
  dviv = 0 ; 
  curh = eqtb [ 13248 ] .cint ; 
  dvif = 0 ; 
  if ( outputfilename == 0 ) 
  {
    if ( jobname == 0 ) 
    openlogfile () ; 
    packjobname ( 787 ) ; 
    while ( ! bopenout ( dvifile ) ) promptfilename ( 788 , 787 ) ; 
    outputfilename = bmakenamestring ( dvifile ) ; 
  } 
  if ( totalpages == 0 ) 
  {
    {
      dvibuf [ dviptr ] = 247 ; 
      incr ( dviptr ) ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
    {
      dvibuf [ dviptr ] = 2 ; 
      incr ( dviptr ) ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
    dvifour ( 25400000L ) ; 
    dvifour ( 473628672L ) ; 
    preparemag () ; 
    dvifour ( eqtb [ 12680 ] .cint ) ; 
    oldsetting = selector ; 
    selector = 21 ; 
    print ( 820 ) ; 
    printint ( eqtb [ 12686 ] .cint ) ; 
    printchar ( 46 ) ; 
    printtwo ( eqtb [ 12685 ] .cint ) ; 
    printchar ( 46 ) ; 
    printtwo ( eqtb [ 12684 ] .cint ) ; 
    printchar ( 58 ) ; 
    printtwo ( eqtb [ 12683 ] .cint / 60 ) ; 
    printtwo ( eqtb [ 12683 ] .cint % 60 ) ; 
    selector = oldsetting ; 
    {
      dvibuf [ dviptr ] = ( poolptr - strstart [ strptr ] ) ; 
      incr ( dviptr ) ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
    {register integer for_end; s = strstart [ strptr ] ; for_end = poolptr - 
    1 ; if ( s <= for_end) do 
      {
	dvibuf [ dviptr ] = strpool [ s ] ; 
	incr ( dviptr ) ; 
	if ( dviptr == dvilimit ) 
	dviswap () ; 
      } 
    while ( s++ < for_end ) ; } 
    poolptr = strstart [ strptr ] ; 
  } 
  pageloc = dvioffset + dviptr ; 
  {
    dvibuf [ dviptr ] = 139 ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {register integer for_end; k = 0 ; for_end = 9 ; if ( k <= for_end) do 
    dvifour ( eqtb [ 12718 + k ] .cint ) ; 
  while ( k++ < for_end ) ; } 
  dvifour ( lastbop ) ; 
  lastbop = pageloc ; 
  curv = mem [ p + 3 ] .cint + eqtb [ 13249 ] .cint ; 
  tempptr = p ; 
  if ( mem [ p ] .hh.b0 == 1 ) 
  vlistout () ; 
  else hlistout () ; 
  {
    dvibuf [ dviptr ] = 140 ; 
    incr ( dviptr ) ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  incr ( totalpages ) ; 
  curs = -1 ; 
  lab30: ; 
  if ( eqtb [ 12697 ] .cint <= 0 ) 
  printchar ( 93 ) ; 
  deadcycles = 0 ; 
  flush ( stdout ) ; 
	;
#ifdef STAT
  if ( eqtb [ 12694 ] .cint > 1 ) 
  {
    printnl ( 823 ) ; 
    printint ( varused ) ; 
    printchar ( 38 ) ; 
    printint ( dynused ) ; 
    printchar ( 59 ) ; 
  } 
#endif /* STAT */
  flushnodelist ( p ) ; 
	;
#ifdef STAT
  if ( eqtb [ 12694 ] .cint > 1 ) 
  {
    print ( 824 ) ; 
    printint ( varused ) ; 
    printchar ( 38 ) ; 
    printint ( dynused ) ; 
    print ( 825 ) ; 
    printint ( himemmin - lomemmax - 1 ) ; 
    println () ; 
  } 
#endif /* STAT */
} 
void zscanspec ( c , threecodes ) 
groupcode c ; 
boolean threecodes ; 
{/* 40 */ scanspec_regmem 
  integer s  ; 
  schar speccode  ; 
  if ( threecodes ) 
  s = savestack [ saveptr + 0 ] .cint ; 
  if ( scankeyword ( 835 ) ) 
  speccode = 0 ; 
  else if ( scankeyword ( 836 ) ) 
  speccode = 1 ; 
  else {
      
    speccode = 1 ; 
    curval = 0 ; 
    goto lab40 ; 
  } 
  scandimen ( false , false , false ) ; 
  lab40: if ( threecodes ) 
  {
    savestack [ saveptr + 0 ] .cint = s ; 
    incr ( saveptr ) ; 
  } 
  savestack [ saveptr + 0 ] .cint = speccode ; 
  savestack [ saveptr + 1 ] .cint = curval ; 
  saveptr = saveptr + 2 ; 
  newsavelevel ( c ) ; 
  scanleftbrace () ; 
} 
halfword zhpack ( p , w , m ) 
halfword p ; 
scaled w ; 
smallnumber m ; 
{/* 21 50 10 */ register halfword Result; hpack_regmem 
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
  mem [ r ] .hh.b0 = 0 ; 
  mem [ r ] .hh.b1 = 0 ; 
  mem [ r + 4 ] .cint = 0 ; 
  q = r + 5 ; 
  mem [ q ] .hh .v.RH = p ; 
  h = 0 ; 
  d = 0 ; 
  x = 0 ; 
  totalstretch [ 0 ] = 0 ; 
  totalshrink [ 0 ] = 0 ; 
  totalstretch [ 1 ] = 0 ; 
  totalshrink [ 1 ] = 0 ; 
  totalstretch [ 2 ] = 0 ; 
  totalshrink [ 2 ] = 0 ; 
  totalstretch [ 3 ] = 0 ; 
  totalshrink [ 3 ] = 0 ; 
  while ( p != 0 ) {
      
    lab21: while ( ( p >= himemmin ) ) {
	
      f = mem [ p ] .hh.b0 ; 
      i = fontinfo [ charbase [ f ] + mem [ p ] .hh.b1 ] .qqqq ; 
      hd = i .b1 ; 
      x = x + fontinfo [ widthbase [ f ] + i .b0 ] .cint ; 
      s = fontinfo [ heightbase [ f ] + ( hd ) / 16 ] .cint ; 
      if ( s > h ) 
      h = s ; 
      s = fontinfo [ depthbase [ f ] + ( hd ) % 16 ] .cint ; 
      if ( s > d ) 
      d = s ; 
      p = mem [ p ] .hh .v.RH ; 
    } 
    if ( p != 0 ) 
    {
      switch ( mem [ p ] .hh.b0 ) 
      {case 0 : 
      case 1 : 
      case 2 : 
      case 13 : 
	{
	  x = x + mem [ p + 1 ] .cint ; 
	  if ( mem [ p ] .hh.b0 >= 2 ) 
	  s = 0 ; 
	  else s = mem [ p + 4 ] .cint ; 
	  if ( mem [ p + 3 ] .cint - s > h ) 
	  h = mem [ p + 3 ] .cint - s ; 
	  if ( mem [ p + 2 ] .cint + s > d ) 
	  d = mem [ p + 2 ] .cint + s ; 
	} 
	break ; 
      case 3 : 
      case 4 : 
      case 5 : 
	if ( adjusttail != 0 ) 
	{
	  while ( mem [ q ] .hh .v.RH != p ) q = mem [ q ] .hh .v.RH ; 
	  if ( mem [ p ] .hh.b0 == 5 ) 
	  {
	    mem [ adjusttail ] .hh .v.RH = mem [ p + 1 ] .cint ; 
	    while ( mem [ adjusttail ] .hh .v.RH != 0 ) adjusttail = mem [ 
	    adjusttail ] .hh .v.RH ; 
	    p = mem [ p ] .hh .v.RH ; 
	    freenode ( mem [ q ] .hh .v.RH , 2 ) ; 
	  } 
	  else {
	      
	    mem [ adjusttail ] .hh .v.RH = p ; 
	    adjusttail = p ; 
	    p = mem [ p ] .hh .v.RH ; 
	  } 
	  mem [ q ] .hh .v.RH = p ; 
	  p = q ; 
	} 
	break ; 
      case 8 : 
	; 
	break ; 
      case 10 : 
	{
	  g = mem [ p + 1 ] .hh .v.LH ; 
	  x = x + mem [ g + 1 ] .cint ; 
	  o = mem [ g ] .hh.b0 ; 
	  totalstretch [ o ] = totalstretch [ o ] + mem [ g + 2 ] .cint ; 
	  o = mem [ g ] .hh.b1 ; 
	  totalshrink [ o ] = totalshrink [ o ] + mem [ g + 3 ] .cint ; 
	  if ( mem [ p ] .hh.b1 >= 100 ) 
	  {
	    g = mem [ p + 1 ] .hh .v.RH ; 
	    if ( mem [ g + 3 ] .cint > h ) 
	    h = mem [ g + 3 ] .cint ; 
	    if ( mem [ g + 2 ] .cint > d ) 
	    d = mem [ g + 2 ] .cint ; 
	  } 
	} 
	break ; 
      case 11 : 
      case 9 : 
	x = x + mem [ p + 1 ] .cint ; 
	break ; 
      case 6 : 
	{
	  mem [ memtop - 12 ] = mem [ p + 1 ] ; 
	  mem [ memtop - 12 ] .hh .v.RH = mem [ p ] .hh .v.RH ; 
	  p = memtop - 12 ; 
	  goto lab21 ; 
	} 
	break ; 
	default: 
	; 
	break ; 
      } 
      p = mem [ p ] .hh .v.RH ; 
    } 
  } 
  if ( adjusttail != 0 ) 
  mem [ adjusttail ] .hh .v.RH = 0 ; 
  mem [ r + 3 ] .cint = h ; 
  mem [ r + 2 ] .cint = d ; 
  if ( m == 1 ) 
  w = x + w ; 
  mem [ r + 1 ] .cint = w ; 
  x = w - x ; 
  if ( x == 0 ) 
  {
    mem [ r + 5 ] .hh.b0 = 0 ; 
    mem [ r + 5 ] .hh.b1 = 0 ; 
    mem [ r + 6 ] .gr = 0.0 ; 
    goto lab10 ; 
  } 
  else if ( x > 0 ) 
  {
    if ( totalstretch [ 3 ] != 0 ) 
    o = 3 ; 
    else if ( totalstretch [ 2 ] != 0 ) 
    o = 2 ; 
    else if ( totalstretch [ 1 ] != 0 ) 
    o = 1 ; 
    else o = 0 ; 
    mem [ r + 5 ] .hh.b1 = o ; 
    mem [ r + 5 ] .hh.b0 = 1 ; 
    if ( totalstretch [ o ] != 0 ) 
    mem [ r + 6 ] .gr = x / ((double) totalstretch [ o ] ) ; 
    else {
	
      mem [ r + 5 ] .hh.b0 = 0 ; 
      mem [ r + 6 ] .gr = 0.0 ; 
    } 
    if ( o == 0 ) 
    if ( mem [ r + 5 ] .hh .v.RH != 0 ) 
    {
      lastbadness = badness ( x , totalstretch [ 0 ] ) ; 
      if ( lastbadness > eqtb [ 12689 ] .cint ) 
      {
	println () ; 
	if ( lastbadness > 100 ) 
	printnl ( 837 ) ; 
	else printnl ( 838 ) ; 
	print ( 839 ) ; 
	printint ( lastbadness ) ; 
	goto lab50 ; 
      } 
    } 
    goto lab10 ; 
  } 
  else {
      
    if ( totalshrink [ 3 ] != 0 ) 
    o = 3 ; 
    else if ( totalshrink [ 2 ] != 0 ) 
    o = 2 ; 
    else if ( totalshrink [ 1 ] != 0 ) 
    o = 1 ; 
    else o = 0 ; 
    mem [ r + 5 ] .hh.b1 = o ; 
    mem [ r + 5 ] .hh.b0 = 2 ; 
    if ( totalshrink [ o ] != 0 ) 
    mem [ r + 6 ] .gr = ( - (integer) x ) / ((double) totalshrink [ o ] ) ; 
    else {
	
      mem [ r + 5 ] .hh.b0 = 0 ; 
      mem [ r + 6 ] .gr = 0.0 ; 
    } 
    if ( ( totalshrink [ o ] < - (integer) x ) && ( o == 0 ) && ( mem [ r + 5 
    ] .hh .v.RH != 0 ) ) 
    {
      lastbadness = 1000000L ; 
      mem [ r + 6 ] .gr = 1.0 ; 
      if ( ( - (integer) x - totalshrink [ 0 ] > eqtb [ 13238 ] .cint ) || ( 
      eqtb [ 12689 ] .cint < 100 ) ) 
      {
	if ( ( eqtb [ 13246 ] .cint > 0 ) && ( - (integer) x - totalshrink [ 0 
	] > eqtb [ 13238 ] .cint ) ) 
	{
	  while ( mem [ q ] .hh .v.RH != 0 ) q = mem [ q ] .hh .v.RH ; 
	  mem [ q ] .hh .v.RH = newrule () ; 
	  mem [ mem [ q ] .hh .v.RH + 1 ] .cint = eqtb [ 13246 ] .cint ; 
	} 
	println () ; 
	printnl ( 845 ) ; 
	printscaled ( - (integer) x - totalshrink [ 0 ] ) ; 
	print ( 846 ) ; 
	goto lab50 ; 
      } 
    } 
    else if ( o == 0 ) 
    if ( mem [ r + 5 ] .hh .v.RH != 0 ) 
    {
      lastbadness = badness ( - (integer) x , totalshrink [ 0 ] ) ; 
      if ( lastbadness > eqtb [ 12689 ] .cint ) 
      {
	println () ; 
	printnl ( 847 ) ; 
	printint ( lastbadness ) ; 
	goto lab50 ; 
      } 
    } 
    goto lab10 ; 
  } 
  lab50: if ( outputactive ) 
  print ( 840 ) ; 
  else {
      
    if ( packbeginline != 0 ) 
    {
      if ( packbeginline > 0 ) 
      print ( 841 ) ; 
      else print ( 842 ) ; 
      printint ( abs ( packbeginline ) ) ; 
      print ( 843 ) ; 
    } 
    else print ( 844 ) ; 
    printint ( line ) ; 
  } 
  println () ; 
  fontinshortdisplay = 0 ; 
  shortdisplay ( mem [ r + 5 ] .hh .v.RH ) ; 
  println () ; 
  begindiagnostic () ; 
  showbox ( r ) ; 
  enddiagnostic ( true ) ; 
  lab10: Result = r ; 
  return(Result) ; 
} 
halfword zvpackage ( p , h , m , l ) 
halfword p ; 
scaled h ; 
smallnumber m ; 
scaled l ; 
{/* 50 10 */ register halfword Result; vpackage_regmem 
  halfword r  ; 
  scaled w, d, x  ; 
  scaled s  ; 
  halfword g  ; 
  glueord o  ; 
  lastbadness = 0 ; 
  r = getnode ( 7 ) ; 
  mem [ r ] .hh.b0 = 1 ; 
  mem [ r ] .hh.b1 = 0 ; 
  mem [ r + 4 ] .cint = 0 ; 
  mem [ r + 5 ] .hh .v.RH = p ; 
  w = 0 ; 
  d = 0 ; 
  x = 0 ; 
  totalstretch [ 0 ] = 0 ; 
  totalshrink [ 0 ] = 0 ; 
  totalstretch [ 1 ] = 0 ; 
  totalshrink [ 1 ] = 0 ; 
  totalstretch [ 2 ] = 0 ; 
  totalshrink [ 2 ] = 0 ; 
  totalstretch [ 3 ] = 0 ; 
  totalshrink [ 3 ] = 0 ; 
  while ( p != 0 ) {
      
    if ( ( p >= himemmin ) ) 
    confusion ( 848 ) ; 
    else switch ( mem [ p ] .hh.b0 ) 
    {case 0 : 
    case 1 : 
    case 2 : 
    case 13 : 
      {
	x = x + d + mem [ p + 3 ] .cint ; 
	d = mem [ p + 2 ] .cint ; 
	if ( mem [ p ] .hh.b0 >= 2 ) 
	s = 0 ; 
	else s = mem [ p + 4 ] .cint ; 
	if ( mem [ p + 1 ] .cint + s > w ) 
	w = mem [ p + 1 ] .cint + s ; 
      } 
      break ; 
    case 8 : 
      ; 
      break ; 
    case 10 : 
      {
	x = x + d ; 
	d = 0 ; 
	g = mem [ p + 1 ] .hh .v.LH ; 
	x = x + mem [ g + 1 ] .cint ; 
	o = mem [ g ] .hh.b0 ; 
	totalstretch [ o ] = totalstretch [ o ] + mem [ g + 2 ] .cint ; 
	o = mem [ g ] .hh.b1 ; 
	totalshrink [ o ] = totalshrink [ o ] + mem [ g + 3 ] .cint ; 
	if ( mem [ p ] .hh.b1 >= 100 ) 
	{
	  g = mem [ p + 1 ] .hh .v.RH ; 
	  if ( mem [ g + 1 ] .cint > w ) 
	  w = mem [ g + 1 ] .cint ; 
	} 
      } 
      break ; 
    case 11 : 
      {
	x = x + d + mem [ p + 1 ] .cint ; 
	d = 0 ; 
      } 
      break ; 
      default: 
      ; 
      break ; 
    } 
    p = mem [ p ] .hh .v.RH ; 
  } 
  mem [ r + 1 ] .cint = w ; 
  if ( d > l ) 
  {
    x = x + d - l ; 
    mem [ r + 2 ] .cint = l ; 
  } 
  else mem [ r + 2 ] .cint = d ; 
  if ( m == 1 ) 
  h = x + h ; 
  mem [ r + 3 ] .cint = h ; 
  x = h - x ; 
  if ( x == 0 ) 
  {
    mem [ r + 5 ] .hh.b0 = 0 ; 
    mem [ r + 5 ] .hh.b1 = 0 ; 
    mem [ r + 6 ] .gr = 0.0 ; 
    goto lab10 ; 
  } 
  else if ( x > 0 ) 
  {
    if ( totalstretch [ 3 ] != 0 ) 
    o = 3 ; 
    else if ( totalstretch [ 2 ] != 0 ) 
    o = 2 ; 
    else if ( totalstretch [ 1 ] != 0 ) 
    o = 1 ; 
    else o = 0 ; 
    mem [ r + 5 ] .hh.b1 = o ; 
    mem [ r + 5 ] .hh.b0 = 1 ; 
    if ( totalstretch [ o ] != 0 ) 
    mem [ r + 6 ] .gr = x / ((double) totalstretch [ o ] ) ; 
    else {
	
      mem [ r + 5 ] .hh.b0 = 0 ; 
      mem [ r + 6 ] .gr = 0.0 ; 
    } 
    if ( o == 0 ) 
    if ( mem [ r + 5 ] .hh .v.RH != 0 ) 
    {
      lastbadness = badness ( x , totalstretch [ 0 ] ) ; 
      if ( lastbadness > eqtb [ 12690 ] .cint ) 
      {
	println () ; 
	if ( lastbadness > 100 ) 
	printnl ( 837 ) ; 
	else printnl ( 838 ) ; 
	print ( 849 ) ; 
	printint ( lastbadness ) ; 
	goto lab50 ; 
      } 
    } 
    goto lab10 ; 
  } 
  else {
      
    if ( totalshrink [ 3 ] != 0 ) 
    o = 3 ; 
    else if ( totalshrink [ 2 ] != 0 ) 
    o = 2 ; 
    else if ( totalshrink [ 1 ] != 0 ) 
    o = 1 ; 
    else o = 0 ; 
    mem [ r + 5 ] .hh.b1 = o ; 
    mem [ r + 5 ] .hh.b0 = 2 ; 
    if ( totalshrink [ o ] != 0 ) 
    mem [ r + 6 ] .gr = ( - (integer) x ) / ((double) totalshrink [ o ] ) ; 
    else {
	
      mem [ r + 5 ] .hh.b0 = 0 ; 
      mem [ r + 6 ] .gr = 0.0 ; 
    } 
    if ( ( totalshrink [ o ] < - (integer) x ) && ( o == 0 ) && ( mem [ r + 5 
    ] .hh .v.RH != 0 ) ) 
    {
      lastbadness = 1000000L ; 
      mem [ r + 6 ] .gr = 1.0 ; 
      if ( ( - (integer) x - totalshrink [ 0 ] > eqtb [ 13239 ] .cint ) || ( 
      eqtb [ 12690 ] .cint < 100 ) ) 
      {
	println () ; 
	printnl ( 850 ) ; 
	printscaled ( - (integer) x - totalshrink [ 0 ] ) ; 
	print ( 851 ) ; 
	goto lab50 ; 
      } 
    } 
    else if ( o == 0 ) 
    if ( mem [ r + 5 ] .hh .v.RH != 0 ) 
    {
      lastbadness = badness ( - (integer) x , totalshrink [ 0 ] ) ; 
      if ( lastbadness > eqtb [ 12690 ] .cint ) 
      {
	println () ; 
	printnl ( 852 ) ; 
	printint ( lastbadness ) ; 
	goto lab50 ; 
      } 
    } 
    goto lab10 ; 
  } 
  lab50: if ( outputactive ) 
  print ( 840 ) ; 
  else {
      
    if ( packbeginline != 0 ) 
    {
      print ( 842 ) ; 
      printint ( abs ( packbeginline ) ) ; 
      print ( 843 ) ; 
    } 
    else print ( 844 ) ; 
    printint ( line ) ; 
    println () ; 
  } 
  begindiagnostic () ; 
  showbox ( r ) ; 
  enddiagnostic ( true ) ; 
  lab10: Result = r ; 
  return(Result) ; 
} 
void zappendtovlist ( b ) 
halfword b ; 
{appendtovlist_regmem 
  scaled d  ; 
  halfword p  ; 
  if ( curlist .auxfield .cint > -65536000L ) 
  {
    d = mem [ eqtb [ 10283 ] .hh .v.RH + 1 ] .cint - curlist .auxfield .cint - 
    mem [ b + 3 ] .cint ; 
    if ( d < eqtb [ 13232 ] .cint ) 
    p = newparamglue ( 0 ) ; 
    else {
	
      p = newskipparam ( 1 ) ; 
      mem [ tempptr + 1 ] .cint = d ; 
    } 
    mem [ curlist .tailfield ] .hh .v.RH = p ; 
    curlist .tailfield = p ; 
  } 
  mem [ curlist .tailfield ] .hh .v.RH = b ; 
  curlist .tailfield = b ; 
  curlist .auxfield .cint = mem [ b + 2 ] .cint ; 
} 
halfword newnoad ( ) 
{register halfword Result; newnoad_regmem 
  halfword p  ; 
  p = getnode ( 4 ) ; 
  mem [ p ] .hh.b0 = 16 ; 
  mem [ p ] .hh.b1 = 0 ; 
  mem [ p + 1 ] .hh = emptyfield ; 
  mem [ p + 3 ] .hh = emptyfield ; 
  mem [ p + 2 ] .hh = emptyfield ; 
  Result = p ; 
  return(Result) ; 
} 
halfword znewstyle ( s ) 
smallnumber s ; 
{register halfword Result; newstyle_regmem 
  halfword p  ; 
  p = getnode ( 3 ) ; 
  mem [ p ] .hh.b0 = 14 ; 
  mem [ p ] .hh.b1 = s ; 
  mem [ p + 1 ] .cint = 0 ; 
  mem [ p + 2 ] .cint = 0 ; 
  Result = p ; 
  return(Result) ; 
} 
halfword newchoice ( ) 
{register halfword Result; newchoice_regmem 
  halfword p  ; 
  p = getnode ( 3 ) ; 
  mem [ p ] .hh.b0 = 15 ; 
  mem [ p ] .hh.b1 = 0 ; 
  mem [ p + 1 ] .hh .v.LH = 0 ; 
  mem [ p + 1 ] .hh .v.RH = 0 ; 
  mem [ p + 2 ] .hh .v.LH = 0 ; 
  mem [ p + 2 ] .hh .v.RH = 0 ; 
  Result = p ; 
  return(Result) ; 
} 
void showinfo ( ) 
{showinfo_regmem 
  shownodelist ( mem [ tempptr ] .hh .v.LH ) ; 
} 
halfword zfractionrule ( t ) 
scaled t ; 
{register halfword Result; fractionrule_regmem 
  halfword p  ; 
  p = newrule () ; 
  mem [ p + 3 ] .cint = t ; 
  mem [ p + 2 ] .cint = 0 ; 
  Result = p ; 
  return(Result) ; 
} 
halfword zoverbar ( b , k , t ) 
halfword b ; 
scaled k ; 
scaled t ; 
{register halfword Result; overbar_regmem 
  halfword p, q  ; 
  p = newkern ( k ) ; 
  mem [ p ] .hh .v.RH = b ; 
  q = fractionrule ( t ) ; 
  mem [ q ] .hh .v.RH = p ; 
  p = newkern ( t ) ; 
  mem [ p ] .hh .v.RH = q ; 
  Result = vpackage ( p , 0 , 1 , 1073741823L ) ; 
  return(Result) ; 
} 
halfword zcharbox ( f , c ) 
internalfontnumber f ; 
quarterword c ; 
{register halfword Result; charbox_regmem 
  fourquarters q  ; 
  eightbits hd  ; 
  halfword b, p  ; 
  q = fontinfo [ charbase [ f ] + c ] .qqqq ; 
  hd = q .b1 ; 
  b = newnullbox () ; 
  mem [ b + 1 ] .cint = fontinfo [ widthbase [ f ] + q .b0 ] .cint + fontinfo 
  [ italicbase [ f ] + ( q .b2 ) / 4 ] .cint ; 
  mem [ b + 3 ] .cint = fontinfo [ heightbase [ f ] + ( hd ) / 16 ] .cint ; 
  mem [ b + 2 ] .cint = fontinfo [ depthbase [ f ] + ( hd ) % 16 ] .cint ; 
  p = getavail () ; 
  mem [ p ] .hh.b1 = c ; 
  mem [ p ] .hh.b0 = f ; 
  mem [ b + 5 ] .hh .v.RH = p ; 
  Result = b ; 
  return(Result) ; 
} 
void zstackintobox ( b , f , c ) 
halfword b ; 
internalfontnumber f ; 
quarterword c ; 
{stackintobox_regmem 
  halfword p  ; 
  p = charbox ( f , c ) ; 
  mem [ p ] .hh .v.RH = mem [ b + 5 ] .hh .v.RH ; 
  mem [ b + 5 ] .hh .v.RH = p ; 
  mem [ b + 3 ] .cint = mem [ p + 3 ] .cint ; 
} 
scaled zheightplusdepth ( f , c ) 
internalfontnumber f ; 
quarterword c ; 
{register scaled Result; heightplusdepth_regmem 
  fourquarters q  ; 
  eightbits hd  ; 
  q = fontinfo [ charbase [ f ] + c ] .qqqq ; 
  hd = q .b1 ; 
  Result = fontinfo [ heightbase [ f ] + ( hd ) / 16 ] .cint + fontinfo [ 
  depthbase [ f ] + ( hd ) % 16 ] .cint ; 
  return(Result) ; 
} 
halfword zvardelimiter ( d , s , v ) 
halfword d ; 
smallnumber s ; 
scaled v ; 
{/* 40 22 */ register halfword Result; vardelimiter_regmem 
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
  z = mem [ d ] .qqqq .b0 ; 
  x = mem [ d ] .qqqq .b1 ; 
  while ( true ) {
      
    if ( ( z != 0 ) || ( x != 0 ) ) 
    {
      z = z + s + 16 ; 
      do {
	  z = z - 16 ; 
	g = eqtb [ 11335 + z ] .hh .v.RH ; 
	if ( g != 0 ) 
	{
	  y = x ; 
	  if ( ( y >= fontbc [ g ] ) && ( y <= fontec [ g ] ) ) 
	  {
	    lab22: q = fontinfo [ charbase [ g ] + y ] .qqqq ; 
	    if ( ( q .b0 > 0 ) ) 
	    {
	      if ( ( ( q .b2 ) % 4 ) == 3 ) 
	      {
		f = g ; 
		c = y ; 
		goto lab40 ; 
	      } 
	      hd = q .b1 ; 
	      u = fontinfo [ heightbase [ g ] + ( hd ) / 16 ] .cint + fontinfo 
	      [ depthbase [ g ] + ( hd ) % 16 ] .cint ; 
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
    z = mem [ d ] .qqqq .b2 ; 
    x = mem [ d ] .qqqq .b3 ; 
  } 
  lab40: if ( f != 0 ) 
  if ( ( ( q .b2 ) % 4 ) == 3 ) 
  {
    b = newnullbox () ; 
    mem [ b ] .hh.b0 = 1 ; 
    r = fontinfo [ extenbase [ f ] + q .b3 ] .qqqq ; 
    c = r .b3 ; 
    u = heightplusdepth ( f , c ) ; 
    w = 0 ; 
    q = fontinfo [ charbase [ f ] + c ] .qqqq ; 
    mem [ b + 1 ] .cint = fontinfo [ widthbase [ f ] + q .b0 ] .cint + 
    fontinfo [ italicbase [ f ] + ( q .b2 ) / 4 ] .cint ; 
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
    {register integer for_end; m = 1 ; for_end = n ; if ( m <= for_end) do 
      stackintobox ( b , f , c ) ; 
    while ( m++ < for_end ) ; } 
    c = r .b1 ; 
    if ( c != 0 ) 
    {
      stackintobox ( b , f , c ) ; 
      c = r .b3 ; 
      {register integer for_end; m = 1 ; for_end = n ; if ( m <= for_end) do 
	stackintobox ( b , f , c ) ; 
      while ( m++ < for_end ) ; } 
    } 
    c = r .b0 ; 
    if ( c != 0 ) 
    stackintobox ( b , f , c ) ; 
    mem [ b + 2 ] .cint = w - mem [ b + 3 ] .cint ; 
  } 
  else b = charbox ( f , c ) ; 
  else {
      
    b = newnullbox () ; 
    mem [ b + 1 ] .cint = eqtb [ 13241 ] .cint ; 
  } 
  mem [ b + 4 ] .cint = half ( mem [ b + 3 ] .cint - mem [ b + 2 ] .cint ) - 
  fontinfo [ 22 + parambase [ eqtb [ 11337 + s ] .hh .v.RH ] ] .cint ; 
  Result = b ; 
  return(Result) ; 
} 
halfword zrebox ( b , w ) 
halfword b ; 
scaled w ; 
{register halfword Result; rebox_regmem 
  halfword p  ; 
  internalfontnumber f  ; 
  scaled v  ; 
  if ( ( mem [ b + 1 ] .cint != w ) && ( mem [ b + 5 ] .hh .v.RH != 0 ) ) 
  {
    if ( mem [ b ] .hh.b0 == 1 ) 
    b = hpack ( b , 0 , 1 ) ; 
    p = mem [ b + 5 ] .hh .v.RH ; 
    if ( ( ( p >= himemmin ) ) && ( mem [ p ] .hh .v.RH == 0 ) ) 
    {
      f = mem [ p ] .hh.b0 ; 
      v = fontinfo [ widthbase [ f ] + fontinfo [ charbase [ f ] + mem [ p ] 
      .hh.b1 ] .qqqq .b0 ] .cint ; 
      if ( v != mem [ b + 1 ] .cint ) 
      mem [ p ] .hh .v.RH = newkern ( mem [ b + 1 ] .cint - v ) ; 
    } 
    freenode ( b , 7 ) ; 
    b = newglue ( 12 ) ; 
    mem [ b ] .hh .v.RH = p ; 
    while ( mem [ p ] .hh .v.RH != 0 ) p = mem [ p ] .hh .v.RH ; 
    mem [ p ] .hh .v.RH = newglue ( 12 ) ; 
    Result = hpack ( b , w , 0 ) ; 
  } 
  else {
      
    mem [ b + 1 ] .cint = w ; 
    Result = b ; 
  } 
  return(Result) ; 
} 
