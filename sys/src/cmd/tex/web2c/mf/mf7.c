#define EXTERN extern
#include "mfd.h"

void zbadbinary ( p , c ) 
halfword p ; 
quarterword c ; 
{disperr ( p , 283 ) ; 
  disperr ( 0 , 835 ) ; 
  if ( c >= 94 ) 
  printop ( c ) ; 
  printknownorunknowntype ( mem [ p ] .hhfield .b0 , p ) ; 
  if ( c >= 94 ) 
  print ( 478 ) ; 
  else printop ( c ) ; 
  printknownorunknowntype ( curtype , curexp ) ; 
  {
    helpptr = 3 ; 
    helpline [ 2 ] = 836 ; 
    helpline [ 1 ] = 845 ; 
    helpline [ 0 ] = 846 ; 
  } 
  putgeterror () ; 
} 
halfword ztarnished ( p ) 
halfword p ; 
{/* 10 */ register halfword Result; halfword q  ; 
  halfword r  ; 
  q = mem [ p + 1 ] .cint ; 
  r = q + bignodesize [ mem [ p ] .hhfield .b0 ] ; 
  do {
      r = r - 2 ; 
    if ( mem [ r ] .hhfield .b0 == 19 ) 
    {
      Result = 1 ; 
      goto lab10 ; 
    } 
  } while ( ! ( r == q ) ) ; 
  Result = 0 ; 
  lab10: ; 
  return(Result) ; 
} 
void zdepfinish ( v , q , t ) 
halfword v ; 
halfword q ; 
smallnumber t ; 
{halfword p  ; 
  scaled vv  ; 
  if ( q == 0 ) 
  p = curexp ; 
  else p = q ; 
  mem [ p + 1 ] .hhfield .v.RH = v ; 
  mem [ p ] .hhfield .b0 = t ; 
  if ( mem [ v ] .hhfield .lhfield == 0 ) 
  {
    vv = mem [ v + 1 ] .cint ; 
    if ( q == 0 ) 
    flushcurexp ( vv ) ; 
    else {
	
      recyclevalue ( p ) ; 
      mem [ q ] .hhfield .b0 = 16 ; 
      mem [ q + 1 ] .cint = vv ; 
    } 
  } 
  else if ( q == 0 ) 
  curtype = t ; 
  if ( fixneeded ) 
  fixdependencies () ; 
} 
void zaddorsubtract ( p , q , c ) 
halfword p ; 
halfword q ; 
quarterword c ; 
{/* 30 10 */ smallnumber s, t  ; 
  halfword r  ; 
  integer v  ; 
  if ( q == 0 ) 
  {
    t = curtype ; 
    if ( t < 17 ) 
    v = curexp ; 
    else v = mem [ curexp + 1 ] .hhfield .v.RH ; 
  } 
  else {
      
    t = mem [ q ] .hhfield .b0 ; 
    if ( t < 17 ) 
    v = mem [ q + 1 ] .cint ; 
    else v = mem [ q + 1 ] .hhfield .v.RH ; 
  } 
  if ( t == 16 ) 
  {
    if ( c == 70 ) 
    v = - (integer) v ; 
    if ( mem [ p ] .hhfield .b0 == 16 ) 
    {
      v = slowadd ( mem [ p + 1 ] .cint , v ) ; 
      if ( q == 0 ) 
      curexp = v ; 
      else mem [ q + 1 ] .cint = v ; 
      goto lab10 ; 
    } 
    r = mem [ p + 1 ] .hhfield .v.RH ; 
    while ( mem [ r ] .hhfield .lhfield != 0 ) r = mem [ r ] .hhfield .v.RH ; 
    mem [ r + 1 ] .cint = slowadd ( mem [ r + 1 ] .cint , v ) ; 
    if ( q == 0 ) 
    {
      q = getnode ( 2 ) ; 
      curexp = q ; 
      curtype = mem [ p ] .hhfield .b0 ; 
      mem [ q ] .hhfield .b1 = 11 ; 
    } 
    mem [ q + 1 ] .hhfield .v.RH = mem [ p + 1 ] .hhfield .v.RH ; 
    mem [ q ] .hhfield .b0 = mem [ p ] .hhfield .b0 ; 
    mem [ q + 1 ] .hhfield .lhfield = mem [ p + 1 ] .hhfield .lhfield ; 
    mem [ mem [ p + 1 ] .hhfield .lhfield ] .hhfield .v.RH = q ; 
    mem [ p ] .hhfield .b0 = 16 ; 
  } 
  else {
      
    if ( c == 70 ) 
    negatedeplist ( v ) ; 
    if ( mem [ p ] .hhfield .b0 == 16 ) 
    {
      while ( mem [ v ] .hhfield .lhfield != 0 ) v = mem [ v ] .hhfield .v.RH 
      ; 
      mem [ v + 1 ] .cint = slowadd ( mem [ p + 1 ] .cint , mem [ v + 1 ] 
      .cint ) ; 
    } 
    else {
	
      s = mem [ p ] .hhfield .b0 ; 
      r = mem [ p + 1 ] .hhfield .v.RH ; 
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
void zdepmult ( p , v , visscaled ) 
halfword p ; 
integer v ; 
boolean visscaled ; 
{/* 10 */ halfword q  ; 
  smallnumber s, t  ; 
  if ( p == 0 ) 
  q = curexp ; 
  else if ( mem [ p ] .hhfield .b0 != 16 ) 
  q = p ; 
  else {
      
    if ( visscaled ) 
    mem [ p + 1 ] .cint = takescaled ( mem [ p + 1 ] .cint , v ) ; 
    else mem [ p + 1 ] .cint = takefraction ( mem [ p + 1 ] .cint , v ) ; 
    goto lab10 ; 
  } 
  t = mem [ q ] .hhfield .b0 ; 
  q = mem [ q + 1 ] .hhfield .v.RH ; 
  s = t ; 
  if ( t == 17 ) 
  if ( visscaled ) 
  if ( abvscd ( maxcoef ( q ) , abs ( v ) , 626349396L , 65536L ) >= 0 ) 
  t = 18 ; 
  q = ptimesv ( q , v , s , t , visscaled ) ; 
  depfinish ( q , p , t ) ; 
  lab10: ; 
} 
void zhardtimes ( p ) 
halfword p ; 
{halfword q  ; 
  halfword r  ; 
  scaled u, v  ; 
  if ( mem [ p ] .hhfield .b0 == 14 ) 
  {
    q = stashcurexp () ; 
    unstashcurexp ( p ) ; 
    p = q ; 
  } 
  r = mem [ curexp + 1 ] .cint ; 
  u = mem [ r + 1 ] .cint ; 
  v = mem [ r + 3 ] .cint ; 
  mem [ r + 2 ] .hhfield .b0 = mem [ p ] .hhfield .b0 ; 
  newdep ( r + 2 , copydeplist ( mem [ p + 1 ] .hhfield .v.RH ) ) ; 
  mem [ r ] .hhfield .b0 = mem [ p ] .hhfield .b0 ; 
  mem [ r + 1 ] = mem [ p + 1 ] ; 
  mem [ mem [ p + 1 ] .hhfield .lhfield ] .hhfield .v.RH = r ; 
  freenode ( p , 2 ) ; 
  depmult ( r , u , true ) ; 
  depmult ( r + 2 , v , true ) ; 
} 
void zdepdiv ( p , v ) 
halfword p ; 
scaled v ; 
{/* 10 */ halfword q  ; 
  smallnumber s, t  ; 
  if ( p == 0 ) 
  q = curexp ; 
  else if ( mem [ p ] .hhfield .b0 != 16 ) 
  q = p ; 
  else {
      
    mem [ p + 1 ] .cint = makescaled ( mem [ p + 1 ] .cint , v ) ; 
    goto lab10 ; 
  } 
  t = mem [ q ] .hhfield .b0 ; 
  q = mem [ q + 1 ] .hhfield .v.RH ; 
  s = t ; 
  if ( t == 17 ) 
  if ( abvscd ( maxcoef ( q ) , 65536L , 626349396L , abs ( v ) ) >= 0 ) 
  t = 18 ; 
  q = poverv ( q , v , s , t ) ; 
  depfinish ( q , p , t ) ; 
  lab10: ; 
} 
void zsetuptrans ( c ) 
quarterword c ; 
{/* 30 10 */ halfword p, q, r  ; 
  if ( ( c != 88 ) || ( curtype != 13 ) ) 
  {
    p = stashcurexp () ; 
    curexp = idtransform () ; 
    curtype = 13 ; 
    q = mem [ curexp + 1 ] .cint ; 
    switch ( c ) 
    {case 84 : 
      if ( mem [ p ] .hhfield .b0 == 16 ) 
      {
	nsincos ( ( mem [ p + 1 ] .cint % 23592960L ) * 16 ) ; 
	mem [ q + 5 ] .cint = roundfraction ( ncos ) ; 
	mem [ q + 9 ] .cint = roundfraction ( nsin ) ; 
	mem [ q + 7 ] .cint = - (integer) mem [ q + 9 ] .cint ; 
	mem [ q + 11 ] .cint = mem [ q + 5 ] .cint ; 
	goto lab30 ; 
      } 
      break ; 
    case 85 : 
      if ( mem [ p ] .hhfield .b0 > 14 ) 
      {
	install ( q + 6 , p ) ; 
	goto lab30 ; 
      } 
      break ; 
    case 86 : 
      if ( mem [ p ] .hhfield .b0 > 14 ) 
      {
	install ( q + 4 , p ) ; 
	install ( q + 10 , p ) ; 
	goto lab30 ; 
      } 
      break ; 
    case 87 : 
      if ( mem [ p ] .hhfield .b0 == 14 ) 
      {
	r = mem [ p + 1 ] .cint ; 
	install ( q , r ) ; 
	install ( q + 2 , r + 2 ) ; 
	goto lab30 ; 
      } 
      break ; 
    case 89 : 
      if ( mem [ p ] .hhfield .b0 > 14 ) 
      {
	install ( q + 4 , p ) ; 
	goto lab30 ; 
      } 
      break ; 
    case 90 : 
      if ( mem [ p ] .hhfield .b0 > 14 ) 
      {
	install ( q + 10 , p ) ; 
	goto lab30 ; 
      } 
      break ; 
    case 91 : 
      if ( mem [ p ] .hhfield .b0 == 14 ) 
      {
	r = mem [ p + 1 ] .cint ; 
	install ( q + 4 , r ) ; 
	install ( q + 10 , r ) ; 
	install ( q + 8 , r + 2 ) ; 
	if ( mem [ r + 2 ] .hhfield .b0 == 16 ) 
	mem [ r + 3 ] .cint = - (integer) mem [ r + 3 ] .cint ; 
	else negatedeplist ( mem [ r + 3 ] .hhfield .v.RH ) ; 
	install ( q + 6 , r + 2 ) ; 
	goto lab30 ; 
      } 
      break ; 
    case 88 : 
      ; 
      break ; 
    } 
    disperr ( p , 855 ) ; 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 856 ; 
      helpline [ 1 ] = 857 ; 
      helpline [ 0 ] = 537 ; 
    } 
    putgeterror () ; 
    lab30: recyclevalue ( p ) ; 
    freenode ( p , 2 ) ; 
  } 
  q = mem [ curexp + 1 ] .cint ; 
  r = q + 12 ; 
  do {
      r = r - 2 ; 
    if ( mem [ r ] .hhfield .b0 != 16 ) 
    goto lab10 ; 
  } while ( ! ( r == q ) ) ; 
  txx = mem [ q + 5 ] .cint ; 
  txy = mem [ q + 7 ] .cint ; 
  tyx = mem [ q + 9 ] .cint ; 
  tyy = mem [ q + 11 ] .cint ; 
  tx = mem [ q + 1 ] .cint ; 
  ty = mem [ q + 3 ] .cint ; 
  flushcurexp ( 0 ) ; 
  lab10: ; 
} 
void zsetupknowntrans ( c ) 
quarterword c ; 
{setuptrans ( c ) ; 
  if ( curtype != 16 ) 
  {
    disperr ( 0 , 858 ) ; 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 859 ; 
      helpline [ 1 ] = 860 ; 
      helpline [ 0 ] = 537 ; 
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
void ztrans ( p , q ) 
halfword p ; 
halfword q ; 
{scaled v  ; 
  v = takescaled ( mem [ p ] .cint , txx ) + takescaled ( mem [ q ] .cint , 
  txy ) + tx ; 
  mem [ q ] .cint = takescaled ( mem [ p ] .cint , tyx ) + takescaled ( mem [ 
  q ] .cint , tyy ) + ty ; 
  mem [ p ] .cint = v ; 
} 
void zpathtrans ( p , c ) 
halfword p ; 
quarterword c ; 
{/* 10 */ halfword q  ; 
  setupknowntrans ( c ) ; 
  unstashcurexp ( p ) ; 
  if ( curtype == 6 ) 
  {
    if ( mem [ curexp + 9 ] .cint == 0 ) 
    if ( tx == 0 ) 
    if ( ty == 0 ) 
    goto lab10 ; 
    flushcurexp ( makepath ( curexp ) ) ; 
    curtype = 8 ; 
  } 
  q = curexp ; 
  do {
      if ( mem [ q ] .hhfield .b0 != 0 ) 
    trans ( q + 3 , q + 4 ) ; 
    trans ( q + 1 , q + 2 ) ; 
    if ( mem [ q ] .hhfield .b1 != 0 ) 
    trans ( q + 5 , q + 6 ) ; 
    q = mem [ q ] .hhfield .v.RH ; 
  } while ( ! ( q == curexp ) ) ; 
  lab10: ; 
} 
void zedgestrans ( p , c ) 
halfword p ; 
quarterword c ; 
{/* 10 */ setupknowntrans ( c ) ; 
  unstashcurexp ( p ) ; 
  curedges = curexp ; 
  if ( mem [ curedges ] .hhfield .v.RH == curedges ) 
  goto lab10 ; 
  if ( txx == 0 ) 
  if ( tyy == 0 ) 
  if ( txy % 65536L == 0 ) 
  if ( tyx % 65536L == 0 ) 
  {
    xyswapedges () ; 
    txx = txy ; 
    tyy = tyx ; 
    txy = 0 ; 
    tyx = 0 ; 
    if ( mem [ curedges ] .hhfield .v.RH == curedges ) 
    goto lab10 ; 
  } 
  if ( txy == 0 ) 
  if ( tyx == 0 ) 
  if ( txx % 65536L == 0 ) 
  if ( tyy % 65536L == 0 ) 
  {
    if ( ( txx == 0 ) || ( tyy == 0 ) ) 
    {
      tossedges ( curedges ) ; 
      curexp = getnode ( 6 ) ; 
      initedges ( curexp ) ; 
    } 
    else {
	
      if ( txx < 0 ) 
      {
	xreflectedges () ; 
	txx = - (integer) txx ; 
      } 
      if ( tyy < 0 ) 
      {
	yreflectedges () ; 
	tyy = - (integer) tyy ; 
      } 
      if ( txx != 65536L ) 
      xscaleedges ( txx / 65536L ) ; 
      if ( tyy != 65536L ) 
      yscaleedges ( tyy / 65536L ) ; 
      tx = roundunscaled ( tx ) ; 
      ty = roundunscaled ( ty ) ; 
      if ( ( toint ( mem [ curedges + 2 ] .hhfield .lhfield ) + tx <= 0 ) || ( 
      mem [ curedges + 2 ] .hhfield .v.RH + tx >= 8192 ) || ( toint ( mem [ 
      curedges + 1 ] .hhfield .lhfield ) + ty <= 0 ) || ( mem [ curedges + 1 ] 
      .hhfield .v.RH + ty >= 8191 ) || ( abs ( tx ) >= 4096 ) || ( abs ( ty ) 
      >= 4096 ) ) 
      {
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 261 ) ; 
	  print ( 864 ) ; 
	} 
	{
	  helpptr = 3 ; 
	  helpline [ 2 ] = 865 ; 
	  helpline [ 1 ] = 536 ; 
	  helpline [ 0 ] = 537 ; 
	} 
	putgeterror () ; 
      } 
      else {
	  
	if ( tx != 0 ) 
	{
	  if ( ! ( abs ( mem [ curedges + 3 ] .hhfield .lhfield - tx - 4096 ) 
	  < 4096 ) ) 
	  fixoffset () ; 
	  mem [ curedges + 2 ] .hhfield .lhfield = mem [ curedges + 2 ] 
	  .hhfield .lhfield + tx ; 
	  mem [ curedges + 2 ] .hhfield .v.RH = mem [ curedges + 2 ] .hhfield 
	  .v.RH + tx ; 
	  mem [ curedges + 3 ] .hhfield .lhfield = mem [ curedges + 3 ] 
	  .hhfield .lhfield - tx ; 
	  mem [ curedges + 4 ] .cint = 0 ; 
	} 
	if ( ty != 0 ) 
	{
	  mem [ curedges + 1 ] .hhfield .lhfield = mem [ curedges + 1 ] 
	  .hhfield .lhfield + ty ; 
	  mem [ curedges + 1 ] .hhfield .v.RH = mem [ curedges + 1 ] .hhfield 
	  .v.RH + ty ; 
	  mem [ curedges + 5 ] .hhfield .lhfield = mem [ curedges + 5 ] 
	  .hhfield .lhfield + ty ; 
	  mem [ curedges + 4 ] .cint = 0 ; 
	} 
      } 
    } 
    goto lab10 ; 
  } 
  {
    if ( interaction == 3 ) 
    ; 
    printnl ( 261 ) ; 
    print ( 861 ) ; 
  } 
  {
    helpptr = 3 ; 
    helpline [ 2 ] = 862 ; 
    helpline [ 1 ] = 863 ; 
    helpline [ 0 ] = 537 ; 
  } 
  putgeterror () ; 
  lab10: ; 
} 
void zbilin1 ( p , t , q , u , delta ) 
halfword p ; 
scaled t ; 
halfword q ; 
scaled u ; 
scaled delta ; 
{halfword r  ; 
  if ( t != 65536L ) 
  depmult ( p , t , true ) ; 
  if ( u != 0 ) 
  if ( mem [ q ] .hhfield .b0 == 16 ) 
  delta = delta + takescaled ( mem [ q + 1 ] .cint , u ) ; 
  else {
      
    if ( mem [ p ] .hhfield .b0 != 18 ) 
    {
      if ( mem [ p ] .hhfield .b0 == 16 ) 
      newdep ( p , constdependency ( mem [ p + 1 ] .cint ) ) ; 
      else mem [ p + 1 ] .hhfield .v.RH = ptimesv ( mem [ p + 1 ] .hhfield 
      .v.RH , 65536L , 17 , 18 , true ) ; 
      mem [ p ] .hhfield .b0 = 18 ; 
    } 
    mem [ p + 1 ] .hhfield .v.RH = pplusfq ( mem [ p + 1 ] .hhfield .v.RH , u 
    , mem [ q + 1 ] .hhfield .v.RH , 18 , mem [ q ] .hhfield .b0 ) ; 
  } 
  if ( mem [ p ] .hhfield .b0 == 16 ) 
  mem [ p + 1 ] .cint = mem [ p + 1 ] .cint + delta ; 
  else {
      
    r = mem [ p + 1 ] .hhfield .v.RH ; 
    while ( mem [ r ] .hhfield .lhfield != 0 ) r = mem [ r ] .hhfield .v.RH ; 
    delta = mem [ r + 1 ] .cint + delta ; 
    if ( r != mem [ p + 1 ] .hhfield .v.RH ) 
    mem [ r + 1 ] .cint = delta ; 
    else {
	
      recyclevalue ( p ) ; 
      mem [ p ] .hhfield .b0 = 16 ; 
      mem [ p + 1 ] .cint = delta ; 
    } 
  } 
  if ( fixneeded ) 
  fixdependencies () ; 
} 
void zaddmultdep ( p , v , r ) 
halfword p ; 
scaled v ; 
halfword r ; 
{if ( mem [ r ] .hhfield .b0 == 16 ) 
  mem [ depfinal + 1 ] .cint = mem [ depfinal + 1 ] .cint + takescaled ( mem [ 
  r + 1 ] .cint , v ) ; 
  else {
      
    mem [ p + 1 ] .hhfield .v.RH = pplusfq ( mem [ p + 1 ] .hhfield .v.RH , v 
    , mem [ r + 1 ] .hhfield .v.RH , 18 , mem [ r ] .hhfield .b0 ) ; 
    if ( fixneeded ) 
    fixdependencies () ; 
  } 
} 
void zbilin2 ( p , t , v , u , q ) 
halfword p ; 
halfword t ; 
scaled v ; 
halfword u ; 
halfword q ; 
{scaled vv  ; 
  vv = mem [ p + 1 ] .cint ; 
  mem [ p ] .hhfield .b0 = 18 ; 
  newdep ( p , constdependency ( 0 ) ) ; 
  if ( vv != 0 ) 
  addmultdep ( p , vv , t ) ; 
  if ( v != 0 ) 
  addmultdep ( p , v , u ) ; 
  if ( q != 0 ) 
  addmultdep ( p , 65536L , q ) ; 
  if ( mem [ p + 1 ] .hhfield .v.RH == depfinal ) 
  {
    vv = mem [ depfinal + 1 ] .cint ; 
    recyclevalue ( p ) ; 
    mem [ p ] .hhfield .b0 = 16 ; 
    mem [ p + 1 ] .cint = vv ; 
  } 
} 
void zbilin3 ( p , t , v , u , delta ) 
halfword p ; 
scaled t ; 
scaled v ; 
scaled u ; 
scaled delta ; 
{if ( t != 65536L ) 
  delta = delta + takescaled ( mem [ p + 1 ] .cint , t ) ; 
  else delta = delta + mem [ p + 1 ] .cint ; 
  if ( u != 0 ) 
  mem [ p + 1 ] .cint = delta + takescaled ( v , u ) ; 
  else mem [ p + 1 ] .cint = delta ; 
} 
void zbigtrans ( p , c ) 
halfword p ; 
quarterword c ; 
{/* 10 */ halfword q, r, pp, qq  ; 
  smallnumber s  ; 
  s = bignodesize [ mem [ p ] .hhfield .b0 ] ; 
  q = mem [ p + 1 ] .cint ; 
  r = q + s ; 
  do {
      r = r - 2 ; 
    if ( mem [ r ] .hhfield .b0 != 16 ) 
    {
      setupknowntrans ( c ) ; 
      makeexpcopy ( p ) ; 
      r = mem [ curexp + 1 ] .cint ; 
      if ( curtype == 13 ) 
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
    r = mem [ curexp + 1 ] .cint ; 
    if ( curtype == 13 ) 
    {
      bilin3 ( r + 10 , tyy , mem [ q + 7 ] .cint , tyx , 0 ) ; 
      bilin3 ( r + 8 , tyy , mem [ q + 5 ] .cint , tyx , 0 ) ; 
      bilin3 ( r + 6 , txx , mem [ q + 11 ] .cint , txy , 0 ) ; 
      bilin3 ( r + 4 , txx , mem [ q + 9 ] .cint , txy , 0 ) ; 
    } 
    bilin3 ( r + 2 , tyy , mem [ q + 1 ] .cint , tyx , ty ) ; 
    bilin3 ( r , txx , mem [ q + 3 ] .cint , txy , tx ) ; 
  } 
  else {
      
    pp = stashcurexp () ; 
    qq = mem [ pp + 1 ] .cint ; 
    makeexpcopy ( p ) ; 
    r = mem [ curexp + 1 ] .cint ; 
    if ( curtype == 13 ) 
    {
      bilin2 ( r + 10 , qq + 10 , mem [ q + 7 ] .cint , qq + 8 , 0 ) ; 
      bilin2 ( r + 8 , qq + 10 , mem [ q + 5 ] .cint , qq + 8 , 0 ) ; 
      bilin2 ( r + 6 , qq + 4 , mem [ q + 11 ] .cint , qq + 6 , 0 ) ; 
      bilin2 ( r + 4 , qq + 4 , mem [ q + 9 ] .cint , qq + 6 , 0 ) ; 
    } 
    bilin2 ( r + 2 , qq + 10 , mem [ q + 1 ] .cint , qq + 8 , qq + 2 ) ; 
    bilin2 ( r , qq + 4 , mem [ q + 3 ] .cint , qq + 6 , qq ) ; 
    recyclevalue ( pp ) ; 
    freenode ( pp , 2 ) ; 
  } 
  lab10: ; 
} 
void zcat ( p ) 
halfword p ; 
{strnumber a, b  ; 
  poolpointer k  ; 
  a = mem [ p + 1 ] .cint ; 
  b = curexp ; 
  {
    if ( poolptr + ( strstart [ a + 1 ] - strstart [ a ] ) + ( strstart [ b + 
    1 ] - strstart [ b ] ) > maxpoolptr ) 
    {
      if ( poolptr + ( strstart [ a + 1 ] - strstart [ a ] ) + ( strstart [ b 
      + 1 ] - strstart [ b ] ) > poolsize ) 
      overflow ( 257 , poolsize - initpoolptr ) ; 
      maxpoolptr = poolptr + ( strstart [ a + 1 ] - strstart [ a ] ) + ( 
      strstart [ b + 1 ] - strstart [ b ] ) ; 
    } 
  } 
  {register integer for_end; k = strstart [ a ] ; for_end = strstart [ a + 1 
  ] - 1 ; if ( k <= for_end) do 
    {
      strpool [ poolptr ] = strpool [ k ] ; 
      incr ( poolptr ) ; 
    } 
  while ( k++ < for_end ) ; } 
  {register integer for_end; k = strstart [ b ] ; for_end = strstart [ b + 1 
  ] - 1 ; if ( k <= for_end) do 
    {
      strpool [ poolptr ] = strpool [ k ] ; 
      incr ( poolptr ) ; 
    } 
  while ( k++ < for_end ) ; } 
  curexp = makestring () ; 
  {
    if ( strref [ b ] < 127 ) 
    if ( strref [ b ] > 1 ) 
    decr ( strref [ b ] ) ; 
    else flushstring ( b ) ; 
  } 
} 
void zchopstring ( p ) 
halfword p ; 
{integer a, b  ; 
  integer l  ; 
  integer k  ; 
  strnumber s  ; 
  boolean reversed  ; 
  a = roundunscaled ( mem [ p + 1 ] .cint ) ; 
  b = roundunscaled ( mem [ p + 3 ] .cint ) ; 
  if ( a <= b ) 
  reversed = false ; 
  else {
      
    reversed = true ; 
    k = a ; 
    a = b ; 
    b = k ; 
  } 
  s = curexp ; 
  l = ( strstart [ s + 1 ] - strstart [ s ] ) ; 
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
    {
      if ( poolptr + b - a > poolsize ) 
      overflow ( 257 , poolsize - initpoolptr ) ; 
      maxpoolptr = poolptr + b - a ; 
    } 
  } 
  if ( reversed ) 
  {register integer for_end; k = strstart [ s ] + b - 1 ; for_end = strstart 
  [ s ] + a ; if ( k >= for_end) do 
    {
      strpool [ poolptr ] = strpool [ k ] ; 
      incr ( poolptr ) ; 
    } 
  while ( k-- > for_end ) ; } 
  else {
      register integer for_end; k = strstart [ s ] + a ; for_end = strstart 
  [ s ] + b - 1 ; if ( k <= for_end) do 
    {
      strpool [ poolptr ] = strpool [ k ] ; 
      incr ( poolptr ) ; 
    } 
  while ( k++ < for_end ) ; } 
  curexp = makestring () ; 
  {
    if ( strref [ s ] < 127 ) 
    if ( strref [ s ] > 1 ) 
    decr ( strref [ s ] ) ; 
    else flushstring ( s ) ; 
  } 
} 
void zchoppath ( p ) 
halfword p ; 
{halfword q  ; 
  halfword pp, qq, rr, ss  ; 
  scaled a, b, k, l  ; 
  boolean reversed  ; 
  l = pathlength () ; 
  a = mem [ p + 1 ] .cint ; 
  b = mem [ p + 3 ] .cint ; 
  if ( a <= b ) 
  reversed = false ; 
  else {
      
    reversed = true ; 
    k = a ; 
    a = b ; 
    b = k ; 
  } 
  if ( a < 0 ) 
  if ( mem [ curexp ] .hhfield .b0 == 0 ) 
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
  if ( mem [ curexp ] .hhfield .b0 == 0 ) 
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
      
    q = mem [ q ] .hhfield .v.RH ; 
    a = a - 65536L ; 
    b = b - 65536L ; 
  } 
  if ( b == a ) 
  {
    if ( a > 0 ) 
    {
      qq = mem [ q ] .hhfield .v.RH ; 
      splitcubic ( q , a * 4096 , mem [ qq + 1 ] .cint , mem [ qq + 2 ] .cint 
      ) ; 
      q = mem [ q ] .hhfield .v.RH ; 
    } 
    pp = copyknot ( q ) ; 
    qq = pp ; 
  } 
  else {
      
    pp = copyknot ( q ) ; 
    qq = pp ; 
    do {
	q = mem [ q ] .hhfield .v.RH ; 
      rr = qq ; 
      qq = copyknot ( q ) ; 
      mem [ rr ] .hhfield .v.RH = qq ; 
      b = b - 65536L ; 
    } while ( ! ( b <= 0 ) ) ; 
    if ( a > 0 ) 
    {
      ss = pp ; 
      pp = mem [ pp ] .hhfield .v.RH ; 
      splitcubic ( ss , a * 4096 , mem [ pp + 1 ] .cint , mem [ pp + 2 ] .cint 
      ) ; 
      pp = mem [ ss ] .hhfield .v.RH ; 
      freenode ( ss , 7 ) ; 
      if ( rr == ss ) 
      {
	b = makescaled ( b , 65536L - a ) ; 
	rr = pp ; 
      } 
    } 
    if ( b < 0 ) 
    {
      splitcubic ( rr , ( b + 65536L ) * 4096 , mem [ qq + 1 ] .cint , mem [ 
      qq + 2 ] .cint ) ; 
      freenode ( qq , 7 ) ; 
      qq = mem [ rr ] .hhfield .v.RH ; 
    } 
  } 
  mem [ pp ] .hhfield .b0 = 0 ; 
  mem [ qq ] .hhfield .b1 = 0 ; 
  mem [ qq ] .hhfield .v.RH = pp ; 
  tossknotlist ( curexp ) ; 
  if ( reversed ) 
  {
    curexp = mem [ htapypoc ( pp ) ] .hhfield .v.RH ; 
    tossknotlist ( pp ) ; 
  } 
  else curexp = pp ; 
} 
void zpairvalue ( x , y ) 
scaled x ; 
scaled y ; 
{halfword p  ; 
  p = getnode ( 2 ) ; 
  flushcurexp ( p ) ; 
  curtype = 14 ; 
  mem [ p ] .hhfield .b0 = 14 ; 
  mem [ p ] .hhfield .b1 = 11 ; 
  initbignode ( p ) ; 
  p = mem [ p + 1 ] .cint ; 
  mem [ p ] .hhfield .b0 = 16 ; 
  mem [ p + 1 ] .cint = x ; 
  mem [ p + 2 ] .hhfield .b0 = 16 ; 
  mem [ p + 3 ] .cint = y ; 
} 
void zsetupoffset ( p ) 
halfword p ; 
{findoffset ( mem [ p + 1 ] .cint , mem [ p + 3 ] .cint , curexp ) ; 
  pairvalue ( curx , cury ) ; 
} 
void zsetupdirectiontime ( p ) 
halfword p ; 
{flushcurexp ( finddirectiontime ( mem [ p + 1 ] .cint , mem [ p + 3 ] .cint 
  , curexp ) ) ; 
} 
void zfindpoint ( v , c ) 
scaled v ; 
quarterword c ; 
{halfword p  ; 
  scaled n  ; 
  halfword q  ; 
  p = curexp ; 
  if ( mem [ p ] .hhfield .b0 == 0 ) 
  n = -65536L ; 
  else n = 0 ; 
  do {
      p = mem [ p ] .hhfield .v.RH ; 
    n = n + 65536L ; 
  } while ( ! ( p == curexp ) ) ; 
  if ( n == 0 ) 
  v = 0 ; 
  else if ( v < 0 ) 
  if ( mem [ p ] .hhfield .b0 == 0 ) 
  v = 0 ; 
  else v = n - 1 - ( ( - (integer) v - 1 ) % n ) ; 
  else if ( v > n ) 
  if ( mem [ p ] .hhfield .b0 == 0 ) 
  v = n ; 
  else v = v % n ; 
  p = curexp ; 
  while ( v >= 65536L ) {
      
    p = mem [ p ] .hhfield .v.RH ; 
    v = v - 65536L ; 
  } 
  if ( v != 0 ) 
  {
    q = mem [ p ] .hhfield .v.RH ; 
    splitcubic ( p , v * 4096 , mem [ q + 1 ] .cint , mem [ q + 2 ] .cint ) ; 
    p = mem [ p ] .hhfield .v.RH ; 
  } 
  switch ( c ) 
  {case 97 : 
    pairvalue ( mem [ p + 1 ] .cint , mem [ p + 2 ] .cint ) ; 
    break ; 
  case 98 : 
    if ( mem [ p ] .hhfield .b0 == 0 ) 
    pairvalue ( mem [ p + 1 ] .cint , mem [ p + 2 ] .cint ) ; 
    else pairvalue ( mem [ p + 3 ] .cint , mem [ p + 4 ] .cint ) ; 
    break ; 
  case 99 : 
    if ( mem [ p ] .hhfield .b1 == 0 ) 
    pairvalue ( mem [ p + 1 ] .cint , mem [ p + 2 ] .cint ) ; 
    else pairvalue ( mem [ p + 5 ] .cint , mem [ p + 6 ] .cint ) ; 
    break ; 
  } 
} 
void zdobinary ( p , c ) 
halfword p ; 
quarterword c ; 
{/* 30 31 10 */ halfword q, r, rr  ; 
  halfword oldp, oldexp  ; 
  integer v  ; 
  {
    if ( aritherror ) 
    cleararith () ; 
  } 
  if ( internal [ 7 ] > 131072L ) 
  {
    begindiagnostic () ; 
    printnl ( 847 ) ; 
    printexp ( p , 0 ) ; 
    printchar ( 41 ) ; 
    printop ( c ) ; 
    printchar ( 40 ) ; 
    printexp ( 0 , 0 ) ; 
    print ( 839 ) ; 
    enddiagnostic ( false ) ; 
  } 
  switch ( mem [ p ] .hhfield .b0 ) 
  {case 13 : 
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
  {case 13 : 
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
  {case 69 : 
  case 70 : 
    if ( ( curtype < 14 ) || ( mem [ p ] .hhfield .b0 < 14 ) ) 
    if ( ( curtype == 11 ) && ( mem [ p ] .hhfield .b0 == 11 ) ) 
    {
      if ( c == 70 ) 
      negateedges ( curexp ) ; 
      curedges = curexp ; 
      mergeedges ( mem [ p + 1 ] .cint ) ; 
    } 
    else badbinary ( p , c ) ; 
    else if ( curtype == 14 ) 
    if ( mem [ p ] .hhfield .b0 != 14 ) 
    badbinary ( p , c ) ; 
    else {
	
      q = mem [ p + 1 ] .cint ; 
      r = mem [ curexp + 1 ] .cint ; 
      addorsubtract ( q , r , c ) ; 
      addorsubtract ( q + 2 , r + 2 , c ) ; 
    } 
    else if ( mem [ p ] .hhfield .b0 == 14 ) 
    badbinary ( p , c ) ; 
    else addorsubtract ( p , 0 , c ) ; 
    break ; 
  case 77 : 
  case 78 : 
  case 79 : 
  case 80 : 
  case 81 : 
  case 82 : 
    {
      if ( ( curtype > 14 ) && ( mem [ p ] .hhfield .b0 > 14 ) ) 
      addorsubtract ( p , 0 , 70 ) ; 
      else if ( curtype != mem [ p ] .hhfield .b0 ) 
      {
	badbinary ( p , c ) ; 
	goto lab30 ; 
      } 
      else if ( curtype == 4 ) 
      flushcurexp ( strvsstr ( mem [ p + 1 ] .cint , curexp ) ) ; 
      else if ( ( curtype == 5 ) || ( curtype == 3 ) ) 
      {
	q = mem [ curexp + 1 ] .cint ; 
	while ( ( q != curexp ) && ( q != p ) ) q = mem [ q + 1 ] .cint ; 
	if ( q == p ) 
	flushcurexp ( 0 ) ; 
      } 
      else if ( ( curtype == 14 ) || ( curtype == 13 ) ) 
      {
	q = mem [ p + 1 ] .cint ; 
	r = mem [ curexp + 1 ] .cint ; 
	rr = r + bignodesize [ curtype ] - 2 ; 
	while ( true ) {
	    
	  addorsubtract ( q , r , 70 ) ; 
	  if ( mem [ r ] .hhfield .b0 != 16 ) 
	  goto lab31 ; 
	  if ( mem [ r + 1 ] .cint != 0 ) 
	  goto lab31 ; 
	  if ( r == rr ) 
	  goto lab31 ; 
	  q = q + 2 ; 
	  r = r + 2 ; 
	} 
	lab31: takepart ( 53 + ( r - mem [ curexp + 1 ] .cint ) / 2 ) ; 
      } 
      else if ( curtype == 2 ) 
      flushcurexp ( curexp - mem [ p + 1 ] .cint ) ; 
      else {
	  
	badbinary ( p , c ) ; 
	goto lab30 ; 
      } 
      if ( curtype != 16 ) 
      {
	if ( curtype < 16 ) 
	{
	  disperr ( p , 283 ) ; 
	  {
	    helpptr = 1 ; 
	    helpline [ 0 ] = 848 ; 
	  } 
	} 
	else {
	    
	  helpptr = 2 ; 
	  helpline [ 1 ] = 849 ; 
	  helpline [ 0 ] = 850 ; 
	} 
	disperr ( 0 , 851 ) ; 
	putgetflusherror ( 31 ) ; 
      } 
      else switch ( c ) 
      {case 77 : 
	if ( curexp < 0 ) 
	curexp = 30 ; 
	else curexp = 31 ; 
	break ; 
      case 78 : 
	if ( curexp <= 0 ) 
	curexp = 30 ; 
	else curexp = 31 ; 
	break ; 
      case 79 : 
	if ( curexp > 0 ) 
	curexp = 30 ; 
	else curexp = 31 ; 
	break ; 
      case 80 : 
	if ( curexp >= 0 ) 
	curexp = 30 ; 
	else curexp = 31 ; 
	break ; 
      case 81 : 
	if ( curexp == 0 ) 
	curexp = 30 ; 
	else curexp = 31 ; 
	break ; 
      case 82 : 
	if ( curexp != 0 ) 
	curexp = 30 ; 
	else curexp = 31 ; 
	break ; 
      } 
      curtype = 2 ; 
      lab30: ; 
    } 
    break ; 
  case 76 : 
  case 75 : 
    if ( ( mem [ p ] .hhfield .b0 != 2 ) || ( curtype != 2 ) ) 
    badbinary ( p , c ) ; 
    else if ( mem [ p + 1 ] .cint == c - 45 ) 
    curexp = mem [ p + 1 ] .cint ; 
    break ; 
  case 71 : 
    if ( ( curtype < 14 ) || ( mem [ p ] .hhfield .b0 < 14 ) ) 
    badbinary ( p , 71 ) ; 
    else if ( ( curtype == 16 ) || ( mem [ p ] .hhfield .b0 == 16 ) ) 
    {
      if ( mem [ p ] .hhfield .b0 == 16 ) 
      {
	v = mem [ p + 1 ] .cint ; 
	freenode ( p , 2 ) ; 
      } 
      else {
	  
	v = curexp ; 
	unstashcurexp ( p ) ; 
      } 
      if ( curtype == 16 ) 
      curexp = takescaled ( curexp , v ) ; 
      else if ( curtype == 14 ) 
      {
	p = mem [ curexp + 1 ] .cint ; 
	depmult ( p , v , true ) ; 
	depmult ( p + 2 , v , true ) ; 
      } 
      else depmult ( 0 , v , true ) ; 
      goto lab10 ; 
    } 
    else if ( ( nicepair ( p , mem [ p ] .hhfield .b0 ) && ( curtype > 14 ) ) 
    || ( nicepair ( curexp , curtype ) && ( mem [ p ] .hhfield .b0 > 14 ) ) ) 
    {
      hardtimes ( p ) ; 
      goto lab10 ; 
    } 
    else badbinary ( p , 71 ) ; 
    break ; 
  case 72 : 
    if ( ( curtype != 16 ) || ( mem [ p ] .hhfield .b0 < 14 ) ) 
    badbinary ( p , 72 ) ; 
    else {
	
      v = curexp ; 
      unstashcurexp ( p ) ; 
      if ( v == 0 ) 
      {
	disperr ( 0 , 781 ) ; 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 853 ; 
	  helpline [ 0 ] = 854 ; 
	} 
	putgeterror () ; 
      } 
      else {
	  
	if ( curtype == 16 ) 
	curexp = makescaled ( curexp , v ) ; 
	else if ( curtype == 14 ) 
	{
	  p = mem [ curexp + 1 ] .cint ; 
	  depdiv ( p , v ) ; 
	  depdiv ( p + 2 , v ) ; 
	} 
	else depdiv ( 0 , v ) ; 
      } 
      goto lab10 ; 
    } 
    break ; 
  case 73 : 
  case 74 : 
    if ( ( curtype == 16 ) && ( mem [ p ] .hhfield .b0 == 16 ) ) 
    if ( c == 73 ) 
    curexp = pythadd ( mem [ p + 1 ] .cint , curexp ) ; 
    else curexp = pythsub ( mem [ p + 1 ] .cint , curexp ) ; 
    else badbinary ( p , c ) ; 
    break ; 
  case 84 : 
  case 85 : 
  case 86 : 
  case 87 : 
  case 88 : 
  case 89 : 
  case 90 : 
  case 91 : 
    if ( ( mem [ p ] .hhfield .b0 == 9 ) || ( mem [ p ] .hhfield .b0 == 8 ) || 
    ( mem [ p ] .hhfield .b0 == 6 ) ) 
    {
      pathtrans ( p , c ) ; 
      goto lab10 ; 
    } 
    else if ( ( mem [ p ] .hhfield .b0 == 14 ) || ( mem [ p ] .hhfield .b0 == 
    13 ) ) 
    bigtrans ( p , c ) ; 
    else if ( mem [ p ] .hhfield .b0 == 11 ) 
    {
      edgestrans ( p , c ) ; 
      goto lab10 ; 
    } 
    else badbinary ( p , c ) ; 
    break ; 
  case 83 : 
    if ( ( curtype == 4 ) && ( mem [ p ] .hhfield .b0 == 4 ) ) 
    cat ( p ) ; 
    else badbinary ( p , 83 ) ; 
    break ; 
  case 94 : 
    if ( nicepair ( p , mem [ p ] .hhfield .b0 ) && ( curtype == 4 ) ) 
    chopstring ( mem [ p + 1 ] .cint ) ; 
    else badbinary ( p , 94 ) ; 
    break ; 
  case 95 : 
    {
      if ( curtype == 14 ) 
      pairtopath () ; 
      if ( nicepair ( p , mem [ p ] .hhfield .b0 ) && ( curtype == 9 ) ) 
      choppath ( mem [ p + 1 ] .cint ) ; 
      else badbinary ( p , 95 ) ; 
    } 
    break ; 
  case 97 : 
  case 98 : 
  case 99 : 
    {
      if ( curtype == 14 ) 
      pairtopath () ; 
      if ( ( curtype == 9 ) && ( mem [ p ] .hhfield .b0 == 16 ) ) 
      findpoint ( mem [ p + 1 ] .cint , c ) ; 
      else badbinary ( p , c ) ; 
    } 
    break ; 
  case 100 : 
    {
      if ( curtype == 8 ) 
      materializepen () ; 
      if ( ( curtype == 6 ) && nicepair ( p , mem [ p ] .hhfield .b0 ) ) 
      setupoffset ( mem [ p + 1 ] .cint ) ; 
      else badbinary ( p , 100 ) ; 
    } 
    break ; 
  case 96 : 
    {
      if ( curtype == 14 ) 
      pairtopath () ; 
      if ( ( curtype == 9 ) && nicepair ( p , mem [ p ] .hhfield .b0 ) ) 
      setupdirectiontime ( mem [ p + 1 ] .cint ) ; 
      else badbinary ( p , 96 ) ; 
    } 
    break ; 
  case 92 : 
    {
      if ( mem [ p ] .hhfield .b0 == 14 ) 
      {
	q = stashcurexp () ; 
	unstashcurexp ( p ) ; 
	pairtopath () ; 
	p = stashcurexp () ; 
	unstashcurexp ( q ) ; 
      } 
      if ( curtype == 14 ) 
      pairtopath () ; 
      if ( ( curtype == 9 ) && ( mem [ p ] .hhfield .b0 == 9 ) ) 
      {
	pathintersection ( mem [ p + 1 ] .cint , curexp ) ; 
	pairvalue ( curt , curtt ) ; 
      } 
      else badbinary ( p , 92 ) ; 
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
void zfracmult ( n , d ) 
scaled n ; 
scaled d ; 
{halfword p  ; 
  halfword oldexp  ; 
  fraction v  ; 
  if ( internal [ 7 ] > 131072L ) 
  {
    begindiagnostic () ; 
    printnl ( 847 ) ; 
    printscaled ( n ) ; 
    printchar ( 47 ) ; 
    printscaled ( d ) ; 
    print ( 852 ) ; 
    printexp ( 0 , 0 ) ; 
    print ( 839 ) ; 
    enddiagnostic ( false ) ; 
  } 
  switch ( curtype ) 
  {case 13 : 
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
  else if ( curtype == 14 ) 
  {
    p = mem [ curexp + 1 ] .cint ; 
    depmult ( p , v , false ) ; 
    depmult ( p + 2 , v , false ) ; 
  } 
  else depmult ( 0 , v , false ) ; 
  if ( oldexp != 0 ) 
  {
    recyclevalue ( oldexp ) ; 
    freenode ( oldexp , 2 ) ; 
  } 
} 
void gfswap ( ) 
{if ( gflimit == gfbufsize ) 
  {
    writegf ( 0 , halfbuf - 1 ) ; 
    gflimit = halfbuf ; 
    gfoffset = gfoffset + gfbufsize ; 
    gfptr = 0 ; 
  } 
  else {
      
    writegf ( halfbuf , gfbufsize - 1 ) ; 
    gflimit = gfbufsize ; 
  } 
} 
void zgffour ( x ) 
integer x ; 
{if ( x >= 0 ) 
  {
    gfbuf [ gfptr ] = x / 16777216L ; 
    incr ( gfptr ) ; 
    if ( gfptr == gflimit ) 
    gfswap () ; 
  } 
  else {
      
    x = x + 1073741824L ; 
    x = x + 1073741824L ; 
    {
      gfbuf [ gfptr ] = ( x / 16777216L ) + 128 ; 
      incr ( gfptr ) ; 
      if ( gfptr == gflimit ) 
      gfswap () ; 
    } 
  } 
  x = x % 16777216L ; 
  {
    gfbuf [ gfptr ] = x / 65536L ; 
    incr ( gfptr ) ; 
    if ( gfptr == gflimit ) 
    gfswap () ; 
  } 
  x = x % 65536L ; 
  {
    gfbuf [ gfptr ] = x / 256 ; 
    incr ( gfptr ) ; 
    if ( gfptr == gflimit ) 
    gfswap () ; 
  } 
  {
    gfbuf [ gfptr ] = x % 256 ; 
    incr ( gfptr ) ; 
    if ( gfptr == gflimit ) 
    gfswap () ; 
  } 
} 
void zgftwo ( x ) 
integer x ; 
{{
    
    gfbuf [ gfptr ] = x / 256 ; 
    incr ( gfptr ) ; 
    if ( gfptr == gflimit ) 
    gfswap () ; 
  } 
  {
    gfbuf [ gfptr ] = x % 256 ; 
    incr ( gfptr ) ; 
    if ( gfptr == gflimit ) 
    gfswap () ; 
  } 
} 
void zgfthree ( x ) 
integer x ; 
{{
    
    gfbuf [ gfptr ] = x / 65536L ; 
    incr ( gfptr ) ; 
    if ( gfptr == gflimit ) 
    gfswap () ; 
  } 
  {
    gfbuf [ gfptr ] = ( x % 65536L ) / 256 ; 
    incr ( gfptr ) ; 
    if ( gfptr == gflimit ) 
    gfswap () ; 
  } 
  {
    gfbuf [ gfptr ] = x % 256 ; 
    incr ( gfptr ) ; 
    if ( gfptr == gflimit ) 
    gfswap () ; 
  } 
} 
void zgfpaint ( d ) 
integer d ; 
{if ( d < 64 ) 
  {
    gfbuf [ gfptr ] = 0 + d ; 
    incr ( gfptr ) ; 
    if ( gfptr == gflimit ) 
    gfswap () ; 
  } 
  else if ( d < 256 ) 
  {
    {
      gfbuf [ gfptr ] = 64 ; 
      incr ( gfptr ) ; 
      if ( gfptr == gflimit ) 
      gfswap () ; 
    } 
    {
      gfbuf [ gfptr ] = d ; 
      incr ( gfptr ) ; 
      if ( gfptr == gflimit ) 
      gfswap () ; 
    } 
  } 
  else {
      
    {
      gfbuf [ gfptr ] = 65 ; 
      incr ( gfptr ) ; 
      if ( gfptr == gflimit ) 
      gfswap () ; 
    } 
    gftwo ( d ) ; 
  } 
} 
void zgfstring ( s , t ) 
strnumber s ; 
strnumber t ; 
{poolpointer k  ; 
  integer l  ; 
  if ( s != 0 ) 
  {
    l = ( strstart [ s + 1 ] - strstart [ s ] ) ; 
    if ( t != 0 ) 
    l = l + ( strstart [ t + 1 ] - strstart [ t ] ) ; 
    if ( l <= 255 ) 
    {
      {
	gfbuf [ gfptr ] = 239 ; 
	incr ( gfptr ) ; 
	if ( gfptr == gflimit ) 
	gfswap () ; 
      } 
      {
	gfbuf [ gfptr ] = l ; 
	incr ( gfptr ) ; 
	if ( gfptr == gflimit ) 
	gfswap () ; 
      } 
    } 
    else {
	
      {
	gfbuf [ gfptr ] = 241 ; 
	incr ( gfptr ) ; 
	if ( gfptr == gflimit ) 
	gfswap () ; 
      } 
      gfthree ( l ) ; 
    } 
    {register integer for_end; k = strstart [ s ] ; for_end = strstart [ s + 
    1 ] - 1 ; if ( k <= for_end) do 
      {
	gfbuf [ gfptr ] = strpool [ k ] ; 
	incr ( gfptr ) ; 
	if ( gfptr == gflimit ) 
	gfswap () ; 
      } 
    while ( k++ < for_end ) ; } 
  } 
  if ( t != 0 ) 
  {register integer for_end; k = strstart [ t ] ; for_end = strstart [ t + 1 
  ] - 1 ; if ( k <= for_end) do 
    {
      gfbuf [ gfptr ] = strpool [ k ] ; 
      incr ( gfptr ) ; 
      if ( gfptr == gflimit ) 
      gfswap () ; 
    } 
  while ( k++ < for_end ) ; } 
} 
void zgfboc ( minm , maxm , minn , maxn ) 
integer minm ; 
integer maxm ; 
integer minn ; 
integer maxn ; 
{/* 10 */ if ( minm < gfminm ) 
  gfminm = minm ; 
  if ( maxn > gfmaxn ) 
  gfmaxn = maxn ; 
  if ( bocp == -1 ) 
  if ( bocc >= 0 ) 
  if ( bocc < 256 ) 
  if ( maxm - minm >= 0 ) 
  if ( maxm - minm < 256 ) 
  if ( maxm >= 0 ) 
  if ( maxm < 256 ) 
  if ( maxn - minn >= 0 ) 
  if ( maxn - minn < 256 ) 
  if ( maxn >= 0 ) 
  if ( maxn < 256 ) 
  {
    {
      gfbuf [ gfptr ] = 68 ; 
      incr ( gfptr ) ; 
      if ( gfptr == gflimit ) 
      gfswap () ; 
    } 
    {
      gfbuf [ gfptr ] = bocc ; 
      incr ( gfptr ) ; 
      if ( gfptr == gflimit ) 
      gfswap () ; 
    } 
    {
      gfbuf [ gfptr ] = maxm - minm ; 
      incr ( gfptr ) ; 
      if ( gfptr == gflimit ) 
      gfswap () ; 
    } 
    {
      gfbuf [ gfptr ] = maxm ; 
      incr ( gfptr ) ; 
      if ( gfptr == gflimit ) 
      gfswap () ; 
    } 
    {
      gfbuf [ gfptr ] = maxn - minn ; 
      incr ( gfptr ) ; 
      if ( gfptr == gflimit ) 
      gfswap () ; 
    } 
    {
      gfbuf [ gfptr ] = maxn ; 
      incr ( gfptr ) ; 
      if ( gfptr == gflimit ) 
      gfswap () ; 
    } 
    goto lab10 ; 
  } 
  {
    gfbuf [ gfptr ] = 67 ; 
    incr ( gfptr ) ; 
    if ( gfptr == gflimit ) 
    gfswap () ; 
  } 
  gffour ( bocc ) ; 
  gffour ( bocp ) ; 
  gffour ( minm ) ; 
  gffour ( maxm ) ; 
  gffour ( minn ) ; 
  gffour ( maxn ) ; 
  lab10: ; 
} 
void initgf ( ) 
{short k  ; 
  integer t  ; 
  gfminm = 4096 ; 
  gfmaxm = -4096 ; 
  gfminn = 4096 ; 
  gfmaxn = -4096 ; 
  {register integer for_end; k = 0 ; for_end = 255 ; if ( k <= for_end) do 
    charptr [ k ] = -1 ; 
  while ( k++ < for_end ) ; } 
  if ( internal [ 27 ] <= 0 ) 
  gfext = 1051 ; 
  else {
      
    oldsetting = selector ; 
    selector = 5 ; 
    printchar ( 46 ) ; 
    printint ( makescaled ( internal [ 27 ] , 59429463L ) ) ; 
    print ( 1052 ) ; 
    gfext = makestring () ; 
    selector = oldsetting ; 
  } 
  {
    if ( jobname == 0 ) 
    openlogfile () ; 
    packjobname ( gfext ) ; 
    while ( ! bopenout ( gffile ) ) promptfilename ( 753 , gfext ) ; 
    outputfilename = bmakenamestring ( gffile ) ; 
  } 
  {
    gfbuf [ gfptr ] = 247 ; 
    incr ( gfptr ) ; 
    if ( gfptr == gflimit ) 
    gfswap () ; 
  } 
  {
    gfbuf [ gfptr ] = 131 ; 
    incr ( gfptr ) ; 
    if ( gfptr == gflimit ) 
    gfswap () ; 
  } 
  oldsetting = selector ; 
  selector = 5 ; 
  print ( 1050 ) ; 
  printint ( roundunscaled ( internal [ 14 ] ) ) ; 
  printchar ( 46 ) ; 
  printdd ( roundunscaled ( internal [ 15 ] ) ) ; 
  printchar ( 46 ) ; 
  printdd ( roundunscaled ( internal [ 16 ] ) ) ; 
  printchar ( 58 ) ; 
  t = roundunscaled ( internal [ 17 ] ) ; 
  printdd ( t / 60 ) ; 
  printdd ( t % 60 ) ; 
  selector = oldsetting ; 
  {
    gfbuf [ gfptr ] = ( poolptr - strstart [ strptr ] ) ; 
    incr ( gfptr ) ; 
    if ( gfptr == gflimit ) 
    gfswap () ; 
  } 
  strstart [ strptr + 1 ] = poolptr ; 
  gfstring ( 0 , strptr ) ; 
  poolptr = strstart [ strptr ] ; 
  gfprevptr = gfoffset + gfptr ; 
} 
void zshipout ( c ) 
eightbits c ; 
{/* 30 */ integer f  ; 
  integer prevm, m, mm  ; 
  integer prevn, n  ; 
  halfword p, q  ; 
  integer prevw, w, ww  ; 
  integer d  ; 
  integer delta  ; 
  integer curminm  ; 
  integer xoff, yoff  ; 
  if ( outputfilename == 0 ) 
  initgf () ; 
  f = roundunscaled ( internal [ 19 ] ) ; 
  xoff = roundunscaled ( internal [ 29 ] ) ; 
  yoff = roundunscaled ( internal [ 30 ] ) ; 
  if ( termoffset > maxprintline - 9 ) 
  println () ; 
  else if ( ( termoffset > 0 ) || ( fileoffset > 0 ) ) 
  printchar ( 32 ) ; 
  printchar ( 91 ) ; 
  printint ( c ) ; 
  if ( f != 0 ) 
  {
    printchar ( 46 ) ; 
    printint ( f ) ; 
  } 
  flush ( stdout ) ; 
  bocc = 256 * f + c ; 
  bocp = charptr [ c ] ; 
  charptr [ c ] = gfprevptr ; 
  if ( internal [ 34 ] > 0 ) 
  {
    if ( xoff != 0 ) 
    {
      gfstring ( 436 , 0 ) ; 
      {
	gfbuf [ gfptr ] = 243 ; 
	incr ( gfptr ) ; 
	if ( gfptr == gflimit ) 
	gfswap () ; 
      } 
      gffour ( xoff * 65536L ) ; 
    } 
    if ( yoff != 0 ) 
    {
      gfstring ( 437 , 0 ) ; 
      {
	gfbuf [ gfptr ] = 243 ; 
	incr ( gfptr ) ; 
	if ( gfptr == gflimit ) 
	gfswap () ; 
      } 
      gffour ( yoff * 65536L ) ; 
    } 
  } 
  prevn = 4096 ; 
  p = mem [ curedges ] .hhfield .lhfield ; 
  n = mem [ curedges + 1 ] .hhfield .v.RH - 4096 ; 
  while ( p != curedges ) {
      
    if ( mem [ p + 1 ] .hhfield .lhfield > 1 ) 
    sortedges ( p ) ; 
    q = mem [ p + 1 ] .hhfield .v.RH ; 
    w = 0 ; 
    prevm = -268435456L ; 
    ww = 0 ; 
    prevw = 0 ; 
    m = prevm ; 
    do {
	if ( q == memtop ) 
      mm = 268435456L ; 
      else {
	  
	d = mem [ q ] .hhfield .lhfield ; 
	mm = d / 8 ; 
	ww = ww + ( d % 8 ) - 4 ; 
      } 
      if ( mm != m ) 
      {
	if ( prevw <= 0 ) 
	{
	  if ( w > 0 ) 
	  {
	    if ( prevm == -268435456L ) 
	    {
	      if ( prevn == 4096 ) 
	      {
		gfboc ( mem [ curedges + 2 ] .hhfield .lhfield + xoff - 4096 , 
		mem [ curedges + 2 ] .hhfield .v.RH + xoff - 4096 , mem [ 
		curedges + 1 ] .hhfield .lhfield + yoff - 4096 , n + yoff ) ; 
		curminm = mem [ curedges + 2 ] .hhfield .lhfield - 4096 + mem 
		[ curedges + 3 ] .hhfield .lhfield ; 
	      } 
	      else if ( prevn > n + 1 ) 
	      {
		delta = prevn - n - 1 ; 
		if ( delta < 256 ) 
		{
		  {
		    gfbuf [ gfptr ] = 71 ; 
		    incr ( gfptr ) ; 
		    if ( gfptr == gflimit ) 
		    gfswap () ; 
		  } 
		  {
		    gfbuf [ gfptr ] = delta ; 
		    incr ( gfptr ) ; 
		    if ( gfptr == gflimit ) 
		    gfswap () ; 
		  } 
		} 
		else {
		    
		  {
		    gfbuf [ gfptr ] = 72 ; 
		    incr ( gfptr ) ; 
		    if ( gfptr == gflimit ) 
		    gfswap () ; 
		  } 
		  gftwo ( delta ) ; 
		} 
	      } 
	      else {
		  
		delta = m - curminm ; 
		if ( delta > 164 ) 
		{
		  gfbuf [ gfptr ] = 70 ; 
		  incr ( gfptr ) ; 
		  if ( gfptr == gflimit ) 
		  gfswap () ; 
		} 
		else {
		    
		  {
		    gfbuf [ gfptr ] = 74 + delta ; 
		    incr ( gfptr ) ; 
		    if ( gfptr == gflimit ) 
		    gfswap () ; 
		  } 
		  goto lab30 ; 
		} 
	      } 
	      gfpaint ( m - curminm ) ; 
	      lab30: prevn = n ; 
	    } 
	    else gfpaint ( m - prevm ) ; 
	    prevm = m ; 
	    prevw = w ; 
	  } 
	} 
	else if ( w <= 0 ) 
	{
	  gfpaint ( m - prevm ) ; 
	  prevm = m ; 
	  prevw = w ; 
	} 
	m = mm ; 
      } 
      w = ww ; 
      q = mem [ q ] .hhfield .v.RH ; 
    } while ( ! ( mm == 268435456L ) ) ; 
    if ( w != 0 ) 
    printnl ( 1054 ) ; 
    if ( prevm - toint ( mem [ curedges + 3 ] .hhfield .lhfield ) + xoff > 
    gfmaxm ) 
    gfmaxm = prevm - mem [ curedges + 3 ] .hhfield .lhfield + xoff ; 
    p = mem [ p ] .hhfield .lhfield ; 
    decr ( n ) ; 
  } 
  if ( prevn == 4096 ) 
  {
    gfboc ( 0 , 0 , 0 , 0 ) ; 
    if ( gfmaxm < 0 ) 
    gfmaxm = 0 ; 
    if ( gfminn > 0 ) 
    gfminn = 0 ; 
  } 
  else if ( prevn + yoff < gfminn ) 
  gfminn = prevn + yoff ; 
  {
    gfbuf [ gfptr ] = 69 ; 
    incr ( gfptr ) ; 
    if ( gfptr == gflimit ) 
    gfswap () ; 
  } 
  gfprevptr = gfoffset + gfptr ; 
  incr ( totalchars ) ; 
  printchar ( 93 ) ; 
  flush ( stdout ) ; 
  if ( internal [ 11 ] > 0 ) 
  printedges ( 1053 , true , xoff , yoff ) ; 
} 
void ztryeq ( l , r ) 
halfword l ; 
halfword r ; 
{/* 30 31 */ halfword p  ; 
  schar t  ; 
  halfword q  ; 
  halfword pp  ; 
  schar tt  ; 
  boolean copied  ; 
  t = mem [ l ] .hhfield .b0 ; 
  if ( t == 16 ) 
  {
    t = 17 ; 
    p = constdependency ( - (integer) mem [ l + 1 ] .cint ) ; 
    q = p ; 
  } 
  else if ( t == 19 ) 
  {
    t = 17 ; 
    p = singledependency ( l ) ; 
    mem [ p + 1 ] .cint = - (integer) mem [ p + 1 ] .cint ; 
    q = depfinal ; 
  } 
  else {
      
    p = mem [ l + 1 ] .hhfield .v.RH ; 
    q = p ; 
    while ( true ) {
	
      mem [ q + 1 ] .cint = - (integer) mem [ q + 1 ] .cint ; 
      if ( mem [ q ] .hhfield .lhfield == 0 ) 
      goto lab30 ; 
      q = mem [ q ] .hhfield .v.RH ; 
    } 
    lab30: mem [ mem [ l + 1 ] .hhfield .lhfield ] .hhfield .v.RH = mem [ q ] 
    .hhfield .v.RH ; 
    mem [ mem [ q ] .hhfield .v.RH + 1 ] .hhfield .lhfield = mem [ l + 1 ] 
    .hhfield .lhfield ; 
    mem [ l ] .hhfield .b0 = 16 ; 
  } 
  if ( r == 0 ) 
  if ( curtype == 16 ) 
  {
    mem [ q + 1 ] .cint = mem [ q + 1 ] .cint + curexp ; 
    goto lab31 ; 
  } 
  else {
      
    tt = curtype ; 
    if ( tt == 19 ) 
    pp = singledependency ( curexp ) ; 
    else pp = mem [ curexp + 1 ] .hhfield .v.RH ; 
  } 
  else if ( mem [ r ] .hhfield .b0 == 16 ) 
  {
    mem [ q + 1 ] .cint = mem [ q + 1 ] .cint + mem [ r + 1 ] .cint ; 
    goto lab31 ; 
  } 
  else {
      
    tt = mem [ r ] .hhfield .b0 ; 
    if ( tt == 19 ) 
    pp = singledependency ( r ) ; 
    else pp = mem [ r + 1 ] .hhfield .v.RH ; 
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
    while ( mem [ q ] .hhfield .lhfield != 0 ) {
	
      mem [ q + 1 ] .cint = roundfraction ( mem [ q + 1 ] .cint ) ; 
      q = mem [ q ] .hhfield .v.RH ; 
    } 
    t = 18 ; 
    p = pplusq ( p , pp , t ) ; 
  } 
  watchcoefs = true ; 
  if ( copied ) 
  flushnodelist ( pp ) ; 
  lab31: ; 
  if ( mem [ p ] .hhfield .lhfield == 0 ) 
  {
    if ( abs ( mem [ p + 1 ] .cint ) > 64 ) 
    {
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 261 ) ; 
	print ( 894 ) ; 
      } 
      print ( 896 ) ; 
      printscaled ( mem [ p + 1 ] .cint ) ; 
      printchar ( 41 ) ; 
      {
	helpptr = 2 ; 
	helpline [ 1 ] = 895 ; 
	helpline [ 0 ] = 893 ; 
      } 
      putgeterror () ; 
    } 
    else if ( r == 0 ) 
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
    freenode ( p , 2 ) ; 
  } 
  else {
      
    lineareq ( p , t ) ; 
    if ( r == 0 ) 
    if ( curtype != 16 ) 
    if ( mem [ curexp ] .hhfield .b0 == 16 ) 
    {
      pp = curexp ; 
      curexp = mem [ curexp + 1 ] .cint ; 
      curtype = 16 ; 
      freenode ( pp , 2 ) ; 
    } 
  } 
} 
