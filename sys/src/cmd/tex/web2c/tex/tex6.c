#define EXTERN extern
#include "texd.h"

void zpostlinebreak ( finalwidowpenalty ) 
integer finalwidowpenalty ; 
{/* 30 31 */ postlinebreak_regmem 
  halfword q, r, s  ; 
  boolean discbreak  ; 
  boolean postdiscbreak  ; 
  scaled curwidth  ; 
  scaled curindent  ; 
  quarterword t  ; 
  integer pen  ; 
  halfword curline  ; 
  q = mem [ bestbet + 1 ] .hh .v.RH ; 
  curp = 0 ; 
  do {
      r = q ; 
    q = mem [ q + 1 ] .hh .v.LH ; 
    mem [ r + 1 ] .hh .v.LH = curp ; 
    curp = r ; 
  } while ( ! ( q == 0 ) ) ; 
  curline = curlist .pgfield + 1 ; 
  do {
      q = mem [ curp + 1 ] .hh .v.RH ; 
    discbreak = false ; 
    postdiscbreak = false ; 
    if ( q != 0 ) 
    if ( mem [ q ] .hh.b0 == 10 ) 
    {
      deleteglueref ( mem [ q + 1 ] .hh .v.LH ) ; 
      mem [ q + 1 ] .hh .v.LH = eqtb [ 10290 ] .hh .v.RH ; 
      mem [ q ] .hh.b1 = 9 ; 
      incr ( mem [ eqtb [ 10290 ] .hh .v.RH ] .hh .v.RH ) ; 
      goto lab30 ; 
    } 
    else {
	
      if ( mem [ q ] .hh.b0 == 7 ) 
      {
	t = mem [ q ] .hh.b1 ; 
	if ( t == 0 ) 
	r = mem [ q ] .hh .v.RH ; 
	else {
	    
	  r = q ; 
	  while ( t > 1 ) {
	      
	    r = mem [ r ] .hh .v.RH ; 
	    decr ( t ) ; 
	  } 
	  s = mem [ r ] .hh .v.RH ; 
	  r = mem [ s ] .hh .v.RH ; 
	  mem [ s ] .hh .v.RH = 0 ; 
	  flushnodelist ( mem [ q ] .hh .v.RH ) ; 
	  mem [ q ] .hh.b1 = 0 ; 
	} 
	if ( mem [ q + 1 ] .hh .v.RH != 0 ) 
	{
	  s = mem [ q + 1 ] .hh .v.RH ; 
	  while ( mem [ s ] .hh .v.RH != 0 ) s = mem [ s ] .hh .v.RH ; 
	  mem [ s ] .hh .v.RH = r ; 
	  r = mem [ q + 1 ] .hh .v.RH ; 
	  mem [ q + 1 ] .hh .v.RH = 0 ; 
	  postdiscbreak = true ; 
	} 
	if ( mem [ q + 1 ] .hh .v.LH != 0 ) 
	{
	  s = mem [ q + 1 ] .hh .v.LH ; 
	  mem [ q ] .hh .v.RH = s ; 
	  while ( mem [ s ] .hh .v.RH != 0 ) s = mem [ s ] .hh .v.RH ; 
	  mem [ q + 1 ] .hh .v.LH = 0 ; 
	  q = s ; 
	} 
	mem [ q ] .hh .v.RH = r ; 
	discbreak = true ; 
      } 
      else if ( ( mem [ q ] .hh.b0 == 9 ) || ( mem [ q ] .hh.b0 == 11 ) ) 
      mem [ q + 1 ] .cint = 0 ; 
    } 
    else {
	
      q = memtop - 3 ; 
      while ( mem [ q ] .hh .v.RH != 0 ) q = mem [ q ] .hh .v.RH ; 
    } 
    r = newparamglue ( 8 ) ; 
    mem [ r ] .hh .v.RH = mem [ q ] .hh .v.RH ; 
    mem [ q ] .hh .v.RH = r ; 
    q = r ; 
    lab30: ; 
    r = mem [ q ] .hh .v.RH ; 
    mem [ q ] .hh .v.RH = 0 ; 
    q = mem [ memtop - 3 ] .hh .v.RH ; 
    mem [ memtop - 3 ] .hh .v.RH = r ; 
    if ( eqtb [ 10289 ] .hh .v.RH != 0 ) 
    {
      r = newparamglue ( 7 ) ; 
      mem [ r ] .hh .v.RH = q ; 
      q = r ; 
    } 
    if ( curline > lastspecialline ) 
    {
      curwidth = secondwidth ; 
      curindent = secondindent ; 
    } 
    else if ( eqtb [ 10812 ] .hh .v.RH == 0 ) 
    {
      curwidth = firstwidth ; 
      curindent = firstindent ; 
    } 
    else {
	
      curwidth = mem [ eqtb [ 10812 ] .hh .v.RH + 2 * curline ] .cint ; 
      curindent = mem [ eqtb [ 10812 ] .hh .v.RH + 2 * curline - 1 ] .cint ; 
    } 
    adjusttail = memtop - 5 ; 
    justbox = hpack ( q , curwidth , 0 ) ; 
    mem [ justbox + 4 ] .cint = curindent ; 
    appendtovlist ( justbox ) ; 
    if ( memtop - 5 != adjusttail ) 
    {
      mem [ curlist .tailfield ] .hh .v.RH = mem [ memtop - 5 ] .hh .v.RH ; 
      curlist .tailfield = adjusttail ; 
    } 
    adjusttail = 0 ; 
    if ( curline + 1 != bestline ) 
    {
      pen = eqtb [ 12676 ] .cint ; 
      if ( curline == curlist .pgfield + 1 ) 
      pen = pen + eqtb [ 12668 ] .cint ; 
      if ( curline + 2 == bestline ) 
      pen = pen + finalwidowpenalty ; 
      if ( discbreak ) 
      pen = pen + eqtb [ 12671 ] .cint ; 
      if ( pen != 0 ) 
      {
	r = newpenalty ( pen ) ; 
	mem [ curlist .tailfield ] .hh .v.RH = r ; 
	curlist .tailfield = r ; 
      } 
    } 
    incr ( curline ) ; 
    curp = mem [ curp + 1 ] .hh .v.LH ; 
    if ( curp != 0 ) 
    if ( ! postdiscbreak ) 
    {
      r = memtop - 3 ; 
      while ( true ) {
	  
	q = mem [ r ] .hh .v.RH ; 
	if ( q == mem [ curp + 1 ] .hh .v.RH ) 
	goto lab31 ; 
	if ( ( q >= himemmin ) ) 
	goto lab31 ; 
	if ( ( mem [ q ] .hh.b0 < 9 ) ) 
	goto lab31 ; 
	if ( mem [ q ] .hh.b1 == 2 ) 
	if ( mem [ q ] .hh.b0 == 11 ) 
	goto lab31 ; 
	r = q ; 
      } 
      lab31: if ( r != memtop - 3 ) 
      {
	mem [ r ] .hh .v.RH = 0 ; 
	flushnodelist ( mem [ memtop - 3 ] .hh .v.RH ) ; 
	mem [ memtop - 3 ] .hh .v.RH = q ; 
      } 
    } 
  } while ( ! ( curp == 0 ) ) ; 
  if ( ( curline != bestline ) || ( mem [ memtop - 3 ] .hh .v.RH != 0 ) ) 
  confusion ( 932 ) ; 
  curlist .pgfield = bestline - 1 ; 
} 
smallnumber zreconstitute ( j , n , bchar , hchar ) 
smallnumber j ; 
smallnumber n ; 
halfword bchar ; 
halfword hchar ; 
{/* 22 30 */ register smallnumber Result; reconstitute_regmem 
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
  mem [ memtop - 4 ] .hh .v.RH = 0 ; 
  curl = hu [ j ] ; 
  curq = t ; 
  if ( j == 0 ) 
  {
    ligaturepresent = initlig ; 
    p = initlist ; 
    if ( ligaturepresent ) 
    lfthit = initlft ; 
    while ( p > 0 ) {
	
      {
	mem [ t ] .hh .v.RH = getavail () ; 
	t = mem [ t ] .hh .v.RH ; 
	mem [ t ] .hh.b0 = hf ; 
	mem [ t ] .hh.b1 = mem [ p ] .hh.b1 ; 
      } 
      p = mem [ p ] .hh .v.RH ; 
    } 
  } 
  else if ( curl < 256 ) 
  {
    mem [ t ] .hh .v.RH = getavail () ; 
    t = mem [ t ] .hh .v.RH ; 
    mem [ t ] .hh.b0 = hf ; 
    mem [ t ] .hh.b1 = curl ; 
  } 
  ligstack = 0 ; 
  {
    if ( j < n ) 
    curr = hu [ j + 1 ] ; 
    else curr = bchar ; 
    if ( odd ( hyf [ j ] ) ) 
    currh = hchar ; 
    else currh = 256 ; 
  } 
  lab22: if ( curl == 256 ) 
  {
    k = bcharlabel [ hf ] ; 
    if ( k == fontmemsize ) 
    goto lab30 ; 
    else q = fontinfo [ k ] .qqqq ; 
  } 
  else {
      
    q = fontinfo [ charbase [ hf ] + curl ] .qqqq ; 
    if ( ( ( q .b2 ) % 4 ) != 1 ) 
    goto lab30 ; 
    k = ligkernbase [ hf ] + q .b3 ; 
    q = fontinfo [ k ] .qqqq ; 
    if ( q .b0 > 128 ) 
    {
      k = ligkernbase [ hf ] + 256 * q .b2 + q .b3 + 32768L - 256 * ( 128 ) ; 
      q = fontinfo [ k ] .qqqq ; 
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
      if ( odd ( hyf [ j ] ) ) 
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
	    mem [ ligstack ] .hh.b1 = curr ; 
	    else {
		
	      ligstack = newligitem ( curr ) ; 
	      if ( j == n ) 
	      bchar = 256 ; 
	      else {
		  
		p = getavail () ; 
		mem [ ligstack + 1 ] .hh .v.RH = p ; 
		mem [ p ] .hh.b1 = hu [ j + 1 ] ; 
		mem [ p ] .hh.b0 = hf ; 
	      } 
	    } 
	  } 
	  break ; 
	case 3 : 
	  {
	    curr = q .b3 ; 
	    p = ligstack ; 
	    ligstack = newligitem ( curr ) ; 
	    mem [ ligstack ] .hh .v.RH = p ; 
	  } 
	  break ; 
	case 7 : 
	case 11 : 
	  {
	    if ( ligaturepresent ) 
	    {
	      p = newligature ( hf , curl , mem [ curq ] .hh .v.RH ) ; 
	      if ( lfthit ) 
	      {
		mem [ p ] .hh.b1 = 2 ; 
		lfthit = false ; 
	      } 
	      if ( false ) 
	      if ( ligstack == 0 ) 
	      {
		incr ( mem [ p ] .hh.b1 ) ; 
		rthit = false ; 
	      } 
	      mem [ curq ] .hh .v.RH = p ; 
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
	      if ( mem [ ligstack + 1 ] .hh .v.RH > 0 ) 
	      {
		mem [ t ] .hh .v.RH = mem [ ligstack + 1 ] .hh .v.RH ; 
		t = mem [ t ] .hh .v.RH ; 
		incr ( j ) ; 
	      } 
	      p = ligstack ; 
	      ligstack = mem [ p ] .hh .v.RH ; 
	      freenode ( p , 2 ) ; 
	      if ( ligstack == 0 ) 
	      {
		if ( j < n ) 
		curr = hu [ j + 1 ] ; 
		else curr = bchar ; 
		if ( odd ( hyf [ j ] ) ) 
		currh = hchar ; 
		else currh = 256 ; 
	      } 
	      else curr = mem [ ligstack ] .hh.b1 ; 
	    } 
	    else if ( j == n ) 
	    goto lab30 ; 
	    else {
		
	      {
		mem [ t ] .hh .v.RH = getavail () ; 
		t = mem [ t ] .hh .v.RH ; 
		mem [ t ] .hh.b0 = hf ; 
		mem [ t ] .hh.b1 = curr ; 
	      } 
	      incr ( j ) ; 
	      {
		if ( j < n ) 
		curr = hu [ j + 1 ] ; 
		else curr = bchar ; 
		if ( odd ( hyf [ j ] ) ) 
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
      w = fontinfo [ kernbase [ hf ] + 256 * q .b2 + q .b3 ] .cint ; 
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
    q = fontinfo [ k ] .qqqq ; 
  } 
  lab30: ; 
  if ( ligaturepresent ) 
  {
    p = newligature ( hf , curl , mem [ curq ] .hh .v.RH ) ; 
    if ( lfthit ) 
    {
      mem [ p ] .hh.b1 = 2 ; 
      lfthit = false ; 
    } 
    if ( rthit ) 
    if ( ligstack == 0 ) 
    {
      incr ( mem [ p ] .hh.b1 ) ; 
      rthit = false ; 
    } 
    mem [ curq ] .hh .v.RH = p ; 
    t = p ; 
    ligaturepresent = false ; 
  } 
  if ( w != 0 ) 
  {
    mem [ t ] .hh .v.RH = newkern ( w ) ; 
    t = mem [ t ] .hh .v.RH ; 
    w = 0 ; 
  } 
  if ( ligstack > 0 ) 
  {
    curq = t ; 
    curl = mem [ ligstack ] .hh.b1 ; 
    ligaturepresent = true ; 
    {
      if ( mem [ ligstack + 1 ] .hh .v.RH > 0 ) 
      {
	mem [ t ] .hh .v.RH = mem [ ligstack + 1 ] .hh .v.RH ; 
	t = mem [ t ] .hh .v.RH ; 
	incr ( j ) ; 
      } 
      p = ligstack ; 
      ligstack = mem [ p ] .hh .v.RH ; 
      freenode ( p , 2 ) ; 
      if ( ligstack == 0 ) 
      {
	if ( j < n ) 
	curr = hu [ j + 1 ] ; 
	else curr = bchar ; 
	if ( odd ( hyf [ j ] ) ) 
	currh = hchar ; 
	else currh = 256 ; 
      } 
      else curr = mem [ ligstack ] .hh.b1 ; 
    } 
    goto lab22 ; 
  } 
  Result = j ; 
  return(Result) ; 
} 
void hyphenate ( ) 
{/* 50 30 40 41 42 45 10 */ hyphenate_regmem 
  schar i, j, l  ; 
  halfword q, r, s  ; 
  halfword bchar  ; 
  halfword majortail, minortail  ; 
  ASCIIcode c  ; 
  schar cloc  ; 
  integer rcount  ; 
  halfword hyfnode  ; 
  triepointer z  ; 
  integer v  ; 
  hyphpointer h  ; 
  strnumber k  ; 
  poolpointer u  ; 
  {register integer for_end; j = 0 ; for_end = hn ; if ( j <= for_end) do 
    hyf [ j ] = 0 ; 
  while ( j++ < for_end ) ; } 
  h = hc [ 1 ] ; 
  incr ( hn ) ; 
  hc [ hn ] = curlang ; 
  {register integer for_end; j = 2 ; for_end = hn ; if ( j <= for_end) do 
    h = ( h + h + hc [ j ] ) % 607 ; 
  while ( j++ < for_end ) ; } 
  while ( true ) {
      
    k = hyphword [ h ] ; 
    if ( k == 0 ) 
    goto lab45 ; 
    if ( ( strstart [ k + 1 ] - strstart [ k ] ) < hn ) 
    goto lab45 ; 
    if ( ( strstart [ k + 1 ] - strstart [ k ] ) == hn ) 
    {
      j = 1 ; 
      u = strstart [ k ] ; 
      do {
	  if ( strpool [ u ] < hc [ j ] ) 
	goto lab45 ; 
	if ( strpool [ u ] > hc [ j ] ) 
	goto lab30 ; 
	incr ( j ) ; 
	incr ( u ) ; 
      } while ( ! ( j > hn ) ) ; 
      s = hyphlist [ h ] ; 
      while ( s != 0 ) {
	  
	hyf [ mem [ s ] .hh .v.LH ] = 1 ; 
	s = mem [ s ] .hh .v.RH ; 
      } 
      decr ( hn ) ; 
      goto lab40 ; 
    } 
    lab30: ; 
    if ( h > 0 ) 
    decr ( h ) ; 
    else h = 607 ; 
  } 
  lab45: decr ( hn ) ; 
  if ( trietrc [ curlang + 1 ] != curlang ) 
  return ; 
  hc [ 0 ] = 0 ; 
  hc [ hn + 1 ] = 0 ; 
  hc [ hn + 2 ] = 256 ; 
  {register integer for_end; j = 0 ; for_end = hn - rhyf + 1 ; if ( j <= 
  for_end) do 
    {
      z = trietrl [ curlang + 1 ] + hc [ j ] ; 
      l = j ; 
      while ( hc [ l ] == trietrc [ z ] ) {
	  
	if ( trietro [ z ] != mintrieop ) 
	{
	  v = trietro [ z ] ; 
	  do {
	      v = v + opstart [ curlang ] ; 
	    i = l - hyfdistance [ v ] ; 
	    if ( hyfnum [ v ] > hyf [ i ] ) 
	    hyf [ i ] = hyfnum [ v ] ; 
	    v = hyfnext [ v ] ; 
	  } while ( ! ( v == mintrieop ) ) ; 
	} 
	incr ( l ) ; 
	z = trietrl [ z ] + hc [ l ] ; 
      } 
    } 
  while ( j++ < for_end ) ; } 
  lab40: {
      register integer for_end; j = 0 ; for_end = lhyf - 1 ; if ( j <= 
  for_end) do 
    hyf [ j ] = 0 ; 
  while ( j++ < for_end ) ; } 
  {register integer for_end; j = 0 ; for_end = rhyf - 1 ; if ( j <= for_end) 
  do 
    hyf [ hn - j ] = 0 ; 
  while ( j++ < for_end ) ; } 
  {register integer for_end; j = lhyf ; for_end = hn - rhyf ; if ( j <= 
  for_end) do 
    if ( odd ( hyf [ j ] ) ) 
    goto lab41 ; 
  while ( j++ < for_end ) ; } 
  return ; 
  lab41: ; 
  q = mem [ hb ] .hh .v.RH ; 
  mem [ hb ] .hh .v.RH = 0 ; 
  r = mem [ ha ] .hh .v.RH ; 
  mem [ ha ] .hh .v.RH = 0 ; 
  bchar = hyfbchar ; 
  if ( ( ha >= himemmin ) ) 
  if ( mem [ ha ] .hh.b0 != hf ) 
  goto lab42 ; 
  else {
      
    initlist = ha ; 
    initlig = false ; 
    hu [ 0 ] = mem [ ha ] .hh.b1 ; 
  } 
  else if ( mem [ ha ] .hh.b0 == 6 ) 
  if ( mem [ ha + 1 ] .hh.b0 != hf ) 
  goto lab42 ; 
  else {
      
    initlist = mem [ ha + 1 ] .hh .v.RH ; 
    initlig = true ; 
    initlft = ( mem [ ha ] .hh.b1 > 1 ) ; 
    hu [ 0 ] = mem [ ha + 1 ] .hh.b1 ; 
    if ( initlist == 0 ) 
    if ( initlft ) 
    {
      hu [ 0 ] = 256 ; 
      initlig = false ; 
    } 
    freenode ( ha , 2 ) ; 
  } 
  else {
      
    if ( ! ( r >= himemmin ) ) 
    if ( mem [ r ] .hh.b0 == 6 ) 
    if ( mem [ r ] .hh.b1 > 1 ) 
    goto lab42 ; 
    j = 1 ; 
    s = ha ; 
    initlist = 0 ; 
    goto lab50 ; 
  } 
  s = curp ; 
  while ( mem [ s ] .hh .v.RH != ha ) s = mem [ s ] .hh .v.RH ; 
  j = 0 ; 
  goto lab50 ; 
  lab42: s = ha ; 
  j = 0 ; 
  hu [ 0 ] = 256 ; 
  initlig = false ; 
  initlist = 0 ; 
  lab50: flushnodelist ( r ) ; 
  do {
      l = j ; 
    j = reconstitute ( j , hn , bchar , hyfchar ) + 1 ; 
    if ( hyphenpassed == 0 ) 
    {
      mem [ s ] .hh .v.RH = mem [ memtop - 4 ] .hh .v.RH ; 
      while ( mem [ s ] .hh .v.RH > 0 ) s = mem [ s ] .hh .v.RH ; 
      if ( odd ( hyf [ j - 1 ] ) ) 
      {
	l = j ; 
	hyphenpassed = j - 1 ; 
	mem [ memtop - 4 ] .hh .v.RH = 0 ; 
      } 
    } 
    if ( hyphenpassed > 0 ) 
    do {
	r = getnode ( 2 ) ; 
      mem [ r ] .hh .v.RH = mem [ memtop - 4 ] .hh .v.RH ; 
      mem [ r ] .hh.b0 = 7 ; 
      majortail = r ; 
      rcount = 0 ; 
      while ( mem [ majortail ] .hh .v.RH > 0 ) {
	  
	majortail = mem [ majortail ] .hh .v.RH ; 
	incr ( rcount ) ; 
      } 
      i = hyphenpassed ; 
      hyf [ i ] = 0 ; 
      minortail = 0 ; 
      mem [ r + 1 ] .hh .v.LH = 0 ; 
      hyfnode = newcharacter ( hf , hyfchar ) ; 
      if ( hyfnode != 0 ) 
      {
	incr ( i ) ; 
	c = hu [ i ] ; 
	hu [ i ] = hyfchar ; 
	{
	  mem [ hyfnode ] .hh .v.RH = avail ; 
	  avail = hyfnode ; 
	;
#ifdef STAT
	  decr ( dynused ) ; 
#endif /* STAT */
	} 
      } 
      while ( l <= i ) {
	  
	l = reconstitute ( l , i , fontbchar [ hf ] , 256 ) + 1 ; 
	if ( mem [ memtop - 4 ] .hh .v.RH > 0 ) 
	{
	  if ( minortail == 0 ) 
	  mem [ r + 1 ] .hh .v.LH = mem [ memtop - 4 ] .hh .v.RH ; 
	  else mem [ minortail ] .hh .v.RH = mem [ memtop - 4 ] .hh .v.RH ; 
	  minortail = mem [ memtop - 4 ] .hh .v.RH ; 
	  while ( mem [ minortail ] .hh .v.RH > 0 ) minortail = mem [ 
	  minortail ] .hh .v.RH ; 
	} 
      } 
      if ( hyfnode != 0 ) 
      {
	hu [ i ] = c ; 
	l = i ; 
	decr ( i ) ; 
      } 
      minortail = 0 ; 
      mem [ r + 1 ] .hh .v.RH = 0 ; 
      cloc = 0 ; 
      if ( bcharlabel [ hf ] < fontmemsize ) 
      {
	decr ( l ) ; 
	c = hu [ l ] ; 
	cloc = l ; 
	hu [ l ] = 256 ; 
      } 
      while ( l < j ) {
	  
	do {
	    l = reconstitute ( l , hn , bchar , 256 ) + 1 ; 
	  if ( cloc > 0 ) 
	  {
	    hu [ cloc ] = c ; 
	    cloc = 0 ; 
	  } 
	  if ( mem [ memtop - 4 ] .hh .v.RH > 0 ) 
	  {
	    if ( minortail == 0 ) 
	    mem [ r + 1 ] .hh .v.RH = mem [ memtop - 4 ] .hh .v.RH ; 
	    else mem [ minortail ] .hh .v.RH = mem [ memtop - 4 ] .hh .v.RH ; 
	    minortail = mem [ memtop - 4 ] .hh .v.RH ; 
	    while ( mem [ minortail ] .hh .v.RH > 0 ) minortail = mem [ 
	    minortail ] .hh .v.RH ; 
	  } 
	} while ( ! ( l >= j ) ) ; 
	while ( l > j ) {
	    
	  j = reconstitute ( j , hn , bchar , 256 ) + 1 ; 
	  mem [ majortail ] .hh .v.RH = mem [ memtop - 4 ] .hh .v.RH ; 
	  while ( mem [ majortail ] .hh .v.RH > 0 ) {
	      
	    majortail = mem [ majortail ] .hh .v.RH ; 
	    incr ( rcount ) ; 
	  } 
	} 
      } 
      if ( rcount > 127 ) 
      {
	mem [ s ] .hh .v.RH = mem [ r ] .hh .v.RH ; 
	mem [ r ] .hh .v.RH = 0 ; 
	flushnodelist ( r ) ; 
      } 
      else {
	  
	mem [ s ] .hh .v.RH = r ; 
	mem [ r ] .hh.b1 = rcount ; 
      } 
      s = majortail ; 
      hyphenpassed = j - 1 ; 
      mem [ memtop - 4 ] .hh .v.RH = 0 ; 
    } while ( ! ( ! odd ( hyf [ j - 1 ] ) ) ) ; 
  } while ( ! ( j > hn ) ) ; 
  mem [ s ] .hh .v.RH = q ; 
  flushlist ( initlist ) ; 
} 
void newhyphexceptions ( ) 
{/* 21 10 40 45 */ newhyphexceptions_regmem 
  smallnumber n  ; 
  smallnumber j  ; 
  hyphpointer h  ; 
  strnumber k  ; 
  halfword p  ; 
  halfword q  ; 
  strnumber s, t  ; 
  poolpointer u, v  ; 
  scanleftbrace () ; 
  if ( eqtb [ 12713 ] .cint <= 0 ) 
  curlang = 0 ; 
  else if ( eqtb [ 12713 ] .cint > 255 ) 
  curlang = 0 ; 
  else curlang = eqtb [ 12713 ] .cint ; 
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
	  mem [ q ] .hh .v.RH = p ; 
	  mem [ q ] .hh .v.LH = n ; 
	  p = q ; 
	} 
      } 
      else {
	  
	if ( eqtb [ 11639 + curchr ] .hh .v.RH == 0 ) 
	{
	  {
	    if ( interaction == 3 ) 
	    ; 
	    printnl ( 262 ) ; 
	    print ( 938 ) ; 
	  } 
	  {
	    helpptr = 2 ; 
	    helpline [ 1 ] = 939 ; 
	    helpline [ 0 ] = 940 ; 
	  } 
	  error () ; 
	} 
	else if ( n < 63 ) 
	{
	  incr ( n ) ; 
	  hc [ n ] = eqtb [ 11639 + curchr ] .hh .v.RH ; 
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
	  hc [ n ] = curlang ; 
	  {
	    if ( poolptr + n > poolsize ) 
	    overflow ( 257 , poolsize - initpoolptr ) ; 
	  } 
	  h = 0 ; 
	  {register integer for_end; j = 1 ; for_end = n ; if ( j <= for_end) 
	  do 
	    {
	      h = ( h + h + hc [ j ] ) % 607 ; 
	      {
		strpool [ poolptr ] = hc [ j ] ; 
		incr ( poolptr ) ; 
	      } 
	    } 
	  while ( j++ < for_end ) ; } 
	  s = makestring () ; 
	  if ( hyphcount == 607 ) 
	  overflow ( 941 , 607 ) ; 
	  incr ( hyphcount ) ; 
	  while ( hyphword [ h ] != 0 ) {
	      
	    k = hyphword [ h ] ; 
	    if ( ( strstart [ k + 1 ] - strstart [ k ] ) < ( strstart [ s + 1 
	    ] - strstart [ s ] ) ) 
	    goto lab40 ; 
	    if ( ( strstart [ k + 1 ] - strstart [ k ] ) > ( strstart [ s + 1 
	    ] - strstart [ s ] ) ) 
	    goto lab45 ; 
	    u = strstart [ k ] ; 
	    v = strstart [ s ] ; 
	    do {
		if ( strpool [ u ] < strpool [ v ] ) 
	      goto lab40 ; 
	      if ( strpool [ u ] > strpool [ v ] ) 
	      goto lab45 ; 
	      incr ( u ) ; 
	      incr ( v ) ; 
	    } while ( ! ( u == strstart [ k + 1 ] ) ) ; 
	    lab40: q = hyphlist [ h ] ; 
	    hyphlist [ h ] = p ; 
	    p = q ; 
	    t = hyphword [ h ] ; 
	    hyphword [ h ] = s ; 
	    s = t ; 
	    lab45: ; 
	    if ( h > 0 ) 
	    decr ( h ) ; 
	    else h = 607 ; 
	  } 
	  hyphword [ h ] = s ; 
	  hyphlist [ h ] = p ; 
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
	  print ( 676 ) ; 
	} 
	printesc ( 934 ) ; 
	print ( 935 ) ; 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 936 ; 
	  helpline [ 0 ] = 937 ; 
	} 
	error () ; 
      } 
      break ; 
    } 
  } 
} 
halfword zprunepagetop ( p ) 
halfword p ; 
{register halfword Result; prunepagetop_regmem 
  halfword prevp  ; 
  halfword q  ; 
  prevp = memtop - 3 ; 
  mem [ memtop - 3 ] .hh .v.RH = p ; 
  while ( p != 0 ) switch ( mem [ p ] .hh.b0 ) 
  {case 0 : 
  case 1 : 
  case 2 : 
    {
      q = newskipparam ( 10 ) ; 
      mem [ prevp ] .hh .v.RH = q ; 
      mem [ q ] .hh .v.RH = p ; 
      if ( mem [ tempptr + 1 ] .cint > mem [ p + 3 ] .cint ) 
      mem [ tempptr + 1 ] .cint = mem [ tempptr + 1 ] .cint - mem [ p + 3 ] 
      .cint ; 
      else mem [ tempptr + 1 ] .cint = 0 ; 
      p = 0 ; 
    } 
    break ; 
  case 8 : 
  case 4 : 
  case 3 : 
    {
      prevp = p ; 
      p = mem [ prevp ] .hh .v.RH ; 
    } 
    break ; 
  case 10 : 
  case 11 : 
  case 12 : 
    {
      q = p ; 
      p = mem [ q ] .hh .v.RH ; 
      mem [ q ] .hh .v.RH = 0 ; 
      mem [ prevp ] .hh .v.RH = p ; 
      flushnodelist ( q ) ; 
    } 
    break ; 
    default: 
    confusion ( 952 ) ; 
    break ; 
  } 
  Result = mem [ memtop - 3 ] .hh .v.RH ; 
  return(Result) ; 
} 
halfword zvertbreak ( p , h , d ) 
halfword p ; 
scaled h ; 
scaled d ; 
{/* 30 45 90 */ register halfword Result; vertbreak_regmem 
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
  activewidth [ 1 ] = 0 ; 
  activewidth [ 2 ] = 0 ; 
  activewidth [ 3 ] = 0 ; 
  activewidth [ 4 ] = 0 ; 
  activewidth [ 5 ] = 0 ; 
  activewidth [ 6 ] = 0 ; 
  prevdp = 0 ; 
  while ( true ) {
      
    if ( p == 0 ) 
    pi = -10000 ; 
    else switch ( mem [ p ] .hh.b0 ) 
    {case 0 : 
    case 1 : 
    case 2 : 
      {
	activewidth [ 1 ] = activewidth [ 1 ] + prevdp + mem [ p + 3 ] .cint ; 
	prevdp = mem [ p + 2 ] .cint ; 
	goto lab45 ; 
      } 
      break ; 
    case 8 : 
      goto lab45 ; 
      break ; 
    case 10 : 
      if ( ( mem [ prevp ] .hh.b0 < 9 ) ) 
      pi = 0 ; 
      else goto lab90 ; 
      break ; 
    case 11 : 
      {
	if ( mem [ p ] .hh .v.RH == 0 ) 
	t = 12 ; 
	else t = mem [ mem [ p ] .hh .v.RH ] .hh.b0 ; 
	if ( t == 10 ) 
	pi = 0 ; 
	else goto lab90 ; 
      } 
      break ; 
    case 12 : 
      pi = mem [ p + 1 ] .cint ; 
      break ; 
    case 4 : 
    case 3 : 
      goto lab45 ; 
      break ; 
      default: 
      confusion ( 953 ) ; 
      break ; 
    } 
    if ( pi < 10000 ) 
    {
      if ( activewidth [ 1 ] < h ) 
      if ( ( activewidth [ 3 ] != 0 ) || ( activewidth [ 4 ] != 0 ) || ( 
      activewidth [ 5 ] != 0 ) ) 
      b = 0 ; 
      else b = badness ( h - activewidth [ 1 ] , activewidth [ 2 ] ) ; 
      else if ( activewidth [ 1 ] - h > activewidth [ 6 ] ) 
      b = 1073741823L ; 
      else b = badness ( activewidth [ 1 ] - h , activewidth [ 6 ] ) ; 
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
	bestheightplusdepth = activewidth [ 1 ] + prevdp ; 
      } 
      if ( ( b == 1073741823L ) || ( pi <= -10000 ) ) 
      goto lab30 ; 
    } 
    if ( ( mem [ p ] .hh.b0 < 10 ) || ( mem [ p ] .hh.b0 > 11 ) ) 
    goto lab45 ; 
    lab90: if ( mem [ p ] .hh.b0 == 11 ) 
    q = p ; 
    else {
	
      q = mem [ p + 1 ] .hh .v.LH ; 
      activewidth [ 2 + mem [ q ] .hh.b0 ] = activewidth [ 2 + mem [ q ] 
      .hh.b0 ] + mem [ q + 2 ] .cint ; 
      activewidth [ 6 ] = activewidth [ 6 ] + mem [ q + 3 ] .cint ; 
      if ( ( mem [ q ] .hh.b1 != 0 ) && ( mem [ q + 3 ] .cint != 0 ) ) 
      {
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 954 ) ; 
	} 
	{
	  helpptr = 4 ; 
	  helpline [ 3 ] = 955 ; 
	  helpline [ 2 ] = 956 ; 
	  helpline [ 1 ] = 957 ; 
	  helpline [ 0 ] = 915 ; 
	} 
	error () ; 
	r = newspec ( q ) ; 
	mem [ r ] .hh.b1 = 0 ; 
	deleteglueref ( q ) ; 
	mem [ p + 1 ] .hh .v.LH = r ; 
	q = r ; 
      } 
    } 
    activewidth [ 1 ] = activewidth [ 1 ] + prevdp + mem [ q + 1 ] .cint ; 
    prevdp = 0 ; 
    lab45: if ( prevdp > d ) 
    {
      activewidth [ 1 ] = activewidth [ 1 ] + prevdp - d ; 
      prevdp = d ; 
    } 
    prevp = p ; 
    p = mem [ prevp ] .hh .v.RH ; 
  } 
  lab30: Result = bestplace ; 
  return(Result) ; 
} 
halfword zvsplit ( n , h ) 
eightbits n ; 
scaled h ; 
{/* 10 30 */ register halfword Result; vsplit_regmem 
  halfword v  ; 
  halfword p  ; 
  halfword q  ; 
  v = eqtb [ 11078 + n ] .hh .v.RH ; 
  if ( curmark [ 3 ] != 0 ) 
  {
    deletetokenref ( curmark [ 3 ] ) ; 
    curmark [ 3 ] = 0 ; 
    deletetokenref ( curmark [ 4 ] ) ; 
    curmark [ 4 ] = 0 ; 
  } 
  if ( v == 0 ) 
  {
    Result = 0 ; 
    return(Result) ; 
  } 
  if ( mem [ v ] .hh.b0 != 1 ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 335 ) ; 
    } 
    printesc ( 958 ) ; 
    print ( 959 ) ; 
    printesc ( 960 ) ; 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 961 ; 
      helpline [ 0 ] = 962 ; 
    } 
    error () ; 
    Result = 0 ; 
    return(Result) ; 
  } 
  q = vertbreak ( mem [ v + 5 ] .hh .v.RH , h , eqtb [ 13236 ] .cint ) ; 
  p = mem [ v + 5 ] .hh .v.RH ; 
  if ( p == q ) 
  mem [ v + 5 ] .hh .v.RH = 0 ; 
  else while ( true ) {
      
    if ( mem [ p ] .hh.b0 == 4 ) 
    if ( curmark [ 3 ] == 0 ) 
    {
      curmark [ 3 ] = mem [ p + 1 ] .cint ; 
      curmark [ 4 ] = curmark [ 3 ] ; 
      mem [ curmark [ 3 ] ] .hh .v.LH = mem [ curmark [ 3 ] ] .hh .v.LH + 2 ; 
    } 
    else {
	
      deletetokenref ( curmark [ 4 ] ) ; 
      curmark [ 4 ] = mem [ p + 1 ] .cint ; 
      incr ( mem [ curmark [ 4 ] ] .hh .v.LH ) ; 
    } 
    if ( mem [ p ] .hh .v.RH == q ) 
    {
      mem [ p ] .hh .v.RH = 0 ; 
      goto lab30 ; 
    } 
    p = mem [ p ] .hh .v.RH ; 
  } 
  lab30: ; 
  q = prunepagetop ( q ) ; 
  p = mem [ v + 5 ] .hh .v.RH ; 
  freenode ( v , 7 ) ; 
  if ( q == 0 ) 
  eqtb [ 11078 + n ] .hh .v.RH = 0 ; 
  else eqtb [ 11078 + n ] .hh .v.RH = vpackage ( q , 0 , 1 , 1073741823L ) ; 
  Result = vpackage ( p , h , 0 , eqtb [ 13236 ] .cint ) ; 
  return(Result) ; 
} 
void printtotals ( ) 
{printtotals_regmem 
  printscaled ( pagesofar [ 1 ] ) ; 
  if ( pagesofar [ 2 ] != 0 ) 
  {
    print ( 310 ) ; 
    printscaled ( pagesofar [ 2 ] ) ; 
    print ( 335 ) ; 
  } 
  if ( pagesofar [ 3 ] != 0 ) 
  {
    print ( 310 ) ; 
    printscaled ( pagesofar [ 3 ] ) ; 
    print ( 309 ) ; 
  } 
  if ( pagesofar [ 4 ] != 0 ) 
  {
    print ( 310 ) ; 
    printscaled ( pagesofar [ 4 ] ) ; 
    print ( 971 ) ; 
  } 
  if ( pagesofar [ 5 ] != 0 ) 
  {
    print ( 310 ) ; 
    printscaled ( pagesofar [ 5 ] ) ; 
    print ( 972 ) ; 
  } 
  if ( pagesofar [ 6 ] != 0 ) 
  {
    print ( 311 ) ; 
    printscaled ( pagesofar [ 6 ] ) ; 
  } 
} 
void zfreezepagespecs ( s ) 
smallnumber s ; 
{freezepagespecs_regmem 
  pagecontents = s ; 
  pagesofar [ 0 ] = eqtb [ 13234 ] .cint ; 
  pagemaxdepth = eqtb [ 13235 ] .cint ; 
  pagesofar [ 7 ] = 0 ; 
  pagesofar [ 1 ] = 0 ; 
  pagesofar [ 2 ] = 0 ; 
  pagesofar [ 3 ] = 0 ; 
  pagesofar [ 4 ] = 0 ; 
  pagesofar [ 5 ] = 0 ; 
  pagesofar [ 6 ] = 0 ; 
  leastpagecost = 1073741823L ; 
	;
#ifdef STAT
  if ( eqtb [ 12696 ] .cint > 0 ) 
  {
    begindiagnostic () ; 
    printnl ( 980 ) ; 
    printscaled ( pagesofar [ 0 ] ) ; 
    print ( 981 ) ; 
    printscaled ( pagemaxdepth ) ; 
    enddiagnostic ( false ) ; 
  } 
#endif /* STAT */
} 
void zboxerror ( n ) 
eightbits n ; 
{boxerror_regmem 
  error () ; 
  begindiagnostic () ; 
  printnl ( 829 ) ; 
  showbox ( eqtb [ 11078 + n ] .hh .v.RH ) ; 
  enddiagnostic ( true ) ; 
  flushnodelist ( eqtb [ 11078 + n ] .hh .v.RH ) ; 
  eqtb [ 11078 + n ] .hh .v.RH = 0 ; 
} 
void zensurevbox ( n ) 
eightbits n ; 
{ensurevbox_regmem 
  halfword p  ; 
  p = eqtb [ 11078 + n ] .hh .v.RH ; 
  if ( p != 0 ) 
  if ( mem [ p ] .hh.b0 == 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 982 ) ; 
    } 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 983 ; 
      helpline [ 1 ] = 984 ; 
      helpline [ 0 ] = 985 ; 
    } 
    boxerror ( n ) ; 
  } 
} 
void zfireup ( c ) 
halfword c ; 
{/* 10 */ fireup_regmem 
  halfword p, q, r, s  ; 
  halfword prevp  ; 
  unsigned char n  ; 
  boolean wait  ; 
  integer savevbadness  ; 
  scaled savevfuzz  ; 
  halfword savesplittopskip  ; 
  if ( mem [ bestpagebreak ] .hh.b0 == 12 ) 
  {
    geqworddefine ( 12702 , mem [ bestpagebreak + 1 ] .cint ) ; 
    mem [ bestpagebreak + 1 ] .cint = 10000 ; 
  } 
  else geqworddefine ( 12702 , 10000 ) ; 
  if ( curmark [ 2 ] != 0 ) 
  {
    if ( curmark [ 0 ] != 0 ) 
    deletetokenref ( curmark [ 0 ] ) ; 
    curmark [ 0 ] = curmark [ 2 ] ; 
    incr ( mem [ curmark [ 0 ] ] .hh .v.LH ) ; 
    deletetokenref ( curmark [ 1 ] ) ; 
    curmark [ 1 ] = 0 ; 
  } 
  if ( c == bestpagebreak ) 
  bestpagebreak = 0 ; 
  if ( eqtb [ 11333 ] .hh .v.RH != 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 335 ) ; 
    } 
    printesc ( 405 ) ; 
    print ( 996 ) ; 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 997 ; 
      helpline [ 0 ] = 985 ; 
    } 
    boxerror ( 255 ) ; 
  } 
  insertpenalties = 0 ; 
  savesplittopskip = eqtb [ 10292 ] .hh .v.RH ; 
  if ( eqtb [ 12716 ] .cint <= 0 ) 
  {
    r = mem [ memtop ] .hh .v.RH ; 
    while ( r != memtop ) {
	
      if ( mem [ r + 2 ] .hh .v.LH != 0 ) 
      {
	n = mem [ r ] .hh.b1 ; 
	ensurevbox ( n ) ; 
	if ( eqtb [ 11078 + n ] .hh .v.RH == 0 ) 
	eqtb [ 11078 + n ] .hh .v.RH = newnullbox () ; 
	p = eqtb [ 11078 + n ] .hh .v.RH + 5 ; 
	while ( mem [ p ] .hh .v.RH != 0 ) p = mem [ p ] .hh .v.RH ; 
	mem [ r + 2 ] .hh .v.RH = p ; 
      } 
      r = mem [ r ] .hh .v.RH ; 
    } 
  } 
  q = memtop - 4 ; 
  mem [ q ] .hh .v.RH = 0 ; 
  prevp = memtop - 2 ; 
  p = mem [ prevp ] .hh .v.RH ; 
  while ( p != bestpagebreak ) {
      
    if ( mem [ p ] .hh.b0 == 3 ) 
    {
      if ( eqtb [ 12716 ] .cint <= 0 ) 
      {
	r = mem [ memtop ] .hh .v.RH ; 
	while ( mem [ r ] .hh.b1 != mem [ p ] .hh.b1 ) r = mem [ r ] .hh .v.RH 
	; 
	if ( mem [ r + 2 ] .hh .v.LH == 0 ) 
	wait = true ; 
	else {
	    
	  wait = false ; 
	  s = mem [ r + 2 ] .hh .v.RH ; 
	  mem [ s ] .hh .v.RH = mem [ p + 4 ] .hh .v.LH ; 
	  if ( mem [ r + 2 ] .hh .v.LH == p ) 
	  {
	    if ( mem [ r ] .hh.b0 == 1 ) 
	    if ( ( mem [ r + 1 ] .hh .v.LH == p ) && ( mem [ r + 1 ] .hh .v.RH 
	    != 0 ) ) 
	    {
	      while ( mem [ s ] .hh .v.RH != mem [ r + 1 ] .hh .v.RH ) s = mem 
	      [ s ] .hh .v.RH ; 
	      mem [ s ] .hh .v.RH = 0 ; 
	      eqtb [ 10292 ] .hh .v.RH = mem [ p + 4 ] .hh .v.RH ; 
	      mem [ p + 4 ] .hh .v.LH = prunepagetop ( mem [ r + 1 ] .hh .v.RH 
	      ) ; 
	      if ( mem [ p + 4 ] .hh .v.LH != 0 ) 
	      {
		tempptr = vpackage ( mem [ p + 4 ] .hh .v.LH , 0 , 1 , 
		1073741823L ) ; 
		mem [ p + 3 ] .cint = mem [ tempptr + 3 ] .cint + mem [ 
		tempptr + 2 ] .cint ; 
		freenode ( tempptr , 7 ) ; 
		wait = true ; 
	      } 
	    } 
	    mem [ r + 2 ] .hh .v.LH = 0 ; 
	    n = mem [ r ] .hh.b1 ; 
	    tempptr = mem [ eqtb [ 11078 + n ] .hh .v.RH + 5 ] .hh .v.RH ; 
	    freenode ( eqtb [ 11078 + n ] .hh .v.RH , 7 ) ; 
	    eqtb [ 11078 + n ] .hh .v.RH = vpackage ( tempptr , 0 , 1 , 
	    1073741823L ) ; 
	  } 
	  else {
	      
	    while ( mem [ s ] .hh .v.RH != 0 ) s = mem [ s ] .hh .v.RH ; 
	    mem [ r + 2 ] .hh .v.RH = s ; 
	  } 
	} 
	mem [ prevp ] .hh .v.RH = mem [ p ] .hh .v.RH ; 
	mem [ p ] .hh .v.RH = 0 ; 
	if ( wait ) 
	{
	  mem [ q ] .hh .v.RH = p ; 
	  q = p ; 
	  incr ( insertpenalties ) ; 
	} 
	else {
	    
	  deleteglueref ( mem [ p + 4 ] .hh .v.RH ) ; 
	  freenode ( p , 5 ) ; 
	} 
	p = prevp ; 
      } 
    } 
    else if ( mem [ p ] .hh.b0 == 4 ) 
    {
      if ( curmark [ 1 ] == 0 ) 
      {
	curmark [ 1 ] = mem [ p + 1 ] .cint ; 
	incr ( mem [ curmark [ 1 ] ] .hh .v.LH ) ; 
      } 
      if ( curmark [ 2 ] != 0 ) 
      deletetokenref ( curmark [ 2 ] ) ; 
      curmark [ 2 ] = mem [ p + 1 ] .cint ; 
      incr ( mem [ curmark [ 2 ] ] .hh .v.LH ) ; 
    } 
    prevp = p ; 
    p = mem [ prevp ] .hh .v.RH ; 
  } 
  eqtb [ 10292 ] .hh .v.RH = savesplittopskip ; 
  if ( p != 0 ) 
  {
    if ( mem [ memtop - 1 ] .hh .v.RH == 0 ) 
    if ( nestptr == 0 ) 
    curlist .tailfield = pagetail ; 
    else nest [ 0 ] .tailfield = pagetail ; 
    mem [ pagetail ] .hh .v.RH = mem [ memtop - 1 ] .hh .v.RH ; 
    mem [ memtop - 1 ] .hh .v.RH = p ; 
    mem [ prevp ] .hh .v.RH = 0 ; 
  } 
  savevbadness = eqtb [ 12690 ] .cint ; 
  eqtb [ 12690 ] .cint = 10000 ; 
  savevfuzz = eqtb [ 13239 ] .cint ; 
  eqtb [ 13239 ] .cint = 1073741823L ; 
  eqtb [ 11333 ] .hh .v.RH = vpackage ( mem [ memtop - 2 ] .hh .v.RH , 
  bestsize , 0 , pagemaxdepth ) ; 
  eqtb [ 12690 ] .cint = savevbadness ; 
  eqtb [ 13239 ] .cint = savevfuzz ; 
  if ( lastglue != 262143L ) 
  deleteglueref ( lastglue ) ; 
  pagecontents = 0 ; 
  pagetail = memtop - 2 ; 
  mem [ memtop - 2 ] .hh .v.RH = 0 ; 
  lastglue = 262143L ; 
  lastpenalty = 0 ; 
  lastkern = 0 ; 
  pagesofar [ 7 ] = 0 ; 
  pagemaxdepth = 0 ; 
  if ( q != memtop - 4 ) 
  {
    mem [ memtop - 2 ] .hh .v.RH = mem [ memtop - 4 ] .hh .v.RH ; 
    pagetail = q ; 
  } 
  r = mem [ memtop ] .hh .v.RH ; 
  while ( r != memtop ) {
      
    q = mem [ r ] .hh .v.RH ; 
    freenode ( r , 4 ) ; 
    r = q ; 
  } 
  mem [ memtop ] .hh .v.RH = memtop ; 
  if ( ( curmark [ 0 ] != 0 ) && ( curmark [ 1 ] == 0 ) ) 
  {
    curmark [ 1 ] = curmark [ 0 ] ; 
    incr ( mem [ curmark [ 0 ] ] .hh .v.LH ) ; 
  } 
  if ( eqtb [ 10813 ] .hh .v.RH != 0 ) 
  if ( deadcycles >= eqtb [ 12703 ] .cint ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 998 ) ; 
    } 
    printint ( deadcycles ) ; 
    print ( 999 ) ; 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 1000 ; 
      helpline [ 1 ] = 1001 ; 
      helpline [ 0 ] = 1002 ; 
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
    begintokenlist ( eqtb [ 10813 ] .hh .v.RH , 6 ) ; 
    newsavelevel ( 8 ) ; 
    normalparagraph () ; 
    scanleftbrace () ; 
    return ; 
  } 
  {
    if ( mem [ memtop - 2 ] .hh .v.RH != 0 ) 
    {
      if ( mem [ memtop - 1 ] .hh .v.RH == 0 ) 
      if ( nestptr == 0 ) 
      curlist .tailfield = pagetail ; 
      else nest [ 0 ] .tailfield = pagetail ; 
      else mem [ pagetail ] .hh .v.RH = mem [ memtop - 1 ] .hh .v.RH ; 
      mem [ memtop - 1 ] .hh .v.RH = mem [ memtop - 2 ] .hh .v.RH ; 
      mem [ memtop - 2 ] .hh .v.RH = 0 ; 
      pagetail = memtop - 2 ; 
    } 
    shipout ( eqtb [ 11333 ] .hh .v.RH ) ; 
    eqtb [ 11333 ] .hh .v.RH = 0 ; 
  } 
} 
void buildpage ( ) 
{/* 10 30 31 22 80 90 */ buildpage_regmem 
  halfword p  ; 
  halfword q, r  ; 
  integer b, c  ; 
  integer pi  ; 
  unsigned char n  ; 
  scaled delta, h, w  ; 
  if ( ( mem [ memtop - 1 ] .hh .v.RH == 0 ) || outputactive ) 
  return ; 
  do {
      lab22: p = mem [ memtop - 1 ] .hh .v.RH ; 
    if ( lastglue != 262143L ) 
    deleteglueref ( lastglue ) ; 
    lastpenalty = 0 ; 
    lastkern = 0 ; 
    if ( mem [ p ] .hh.b0 == 10 ) 
    {
      lastglue = mem [ p + 1 ] .hh .v.LH ; 
      incr ( mem [ lastglue ] .hh .v.RH ) ; 
    } 
    else {
	
      lastglue = 262143L ; 
      if ( mem [ p ] .hh.b0 == 12 ) 
      lastpenalty = mem [ p + 1 ] .cint ; 
      else if ( mem [ p ] .hh.b0 == 11 ) 
      lastkern = mem [ p + 1 ] .cint ; 
    } 
    switch ( mem [ p ] .hh.b0 ) 
    {case 0 : 
    case 1 : 
    case 2 : 
      if ( pagecontents < 2 ) 
      {
	if ( pagecontents == 0 ) 
	freezepagespecs ( 2 ) ; 
	else pagecontents = 2 ; 
	q = newskipparam ( 9 ) ; 
	if ( mem [ tempptr + 1 ] .cint > mem [ p + 3 ] .cint ) 
	mem [ tempptr + 1 ] .cint = mem [ tempptr + 1 ] .cint - mem [ p + 3 ] 
	.cint ; 
	else mem [ tempptr + 1 ] .cint = 0 ; 
	mem [ q ] .hh .v.RH = p ; 
	mem [ memtop - 1 ] .hh .v.RH = q ; 
	goto lab22 ; 
      } 
      else {
	  
	pagesofar [ 1 ] = pagesofar [ 1 ] + pagesofar [ 7 ] + mem [ p + 3 ] 
	.cint ; 
	pagesofar [ 7 ] = mem [ p + 2 ] .cint ; 
	goto lab80 ; 
      } 
      break ; 
    case 8 : 
      goto lab80 ; 
      break ; 
    case 10 : 
      if ( pagecontents < 2 ) 
      goto lab31 ; 
      else if ( ( mem [ pagetail ] .hh.b0 < 9 ) ) 
      pi = 0 ; 
      else goto lab90 ; 
      break ; 
    case 11 : 
      if ( pagecontents < 2 ) 
      goto lab31 ; 
      else if ( mem [ p ] .hh .v.RH == 0 ) 
      return ; 
      else if ( mem [ mem [ p ] .hh .v.RH ] .hh.b0 == 10 ) 
      pi = 0 ; 
      else goto lab90 ; 
      break ; 
    case 12 : 
      if ( pagecontents < 2 ) 
      goto lab31 ; 
      else pi = mem [ p + 1 ] .cint ; 
      break ; 
    case 4 : 
      goto lab80 ; 
      break ; 
    case 3 : 
      {
	if ( pagecontents == 0 ) 
	freezepagespecs ( 1 ) ; 
	n = mem [ p ] .hh.b1 ; 
	r = memtop ; 
	while ( n >= mem [ mem [ r ] .hh .v.RH ] .hh.b1 ) r = mem [ r ] .hh 
	.v.RH ; 
	n = n ; 
	if ( mem [ r ] .hh.b1 != n ) 
	{
	  q = getnode ( 4 ) ; 
	  mem [ q ] .hh .v.RH = mem [ r ] .hh .v.RH ; 
	  mem [ r ] .hh .v.RH = q ; 
	  r = q ; 
	  mem [ r ] .hh.b1 = n ; 
	  mem [ r ] .hh.b0 = 0 ; 
	  ensurevbox ( n ) ; 
	  if ( eqtb [ 11078 + n ] .hh .v.RH == 0 ) 
	  mem [ r + 3 ] .cint = 0 ; 
	  else mem [ r + 3 ] .cint = mem [ eqtb [ 11078 + n ] .hh .v.RH + 3 ] 
	  .cint + mem [ eqtb [ 11078 + n ] .hh .v.RH + 2 ] .cint ; 
	  mem [ r + 2 ] .hh .v.LH = 0 ; 
	  q = eqtb [ 10300 + n ] .hh .v.RH ; 
	  if ( eqtb [ 12718 + n ] .cint == 1000 ) 
	  h = mem [ r + 3 ] .cint ; 
	  else h = xovern ( mem [ r + 3 ] .cint , 1000 ) * eqtb [ 12718 + n ] 
	  .cint ; 
	  pagesofar [ 0 ] = pagesofar [ 0 ] - h - mem [ q + 1 ] .cint ; 
	  pagesofar [ 2 + mem [ q ] .hh.b0 ] = pagesofar [ 2 + mem [ q ] 
	  .hh.b0 ] + mem [ q + 2 ] .cint ; 
	  pagesofar [ 6 ] = pagesofar [ 6 ] + mem [ q + 3 ] .cint ; 
	  if ( ( mem [ q ] .hh.b1 != 0 ) && ( mem [ q + 3 ] .cint != 0 ) ) 
	  {
	    {
	      if ( interaction == 3 ) 
	      ; 
	      printnl ( 262 ) ; 
	      print ( 991 ) ; 
	    } 
	    printesc ( 391 ) ; 
	    printint ( n ) ; 
	    {
	      helpptr = 3 ; 
	      helpline [ 2 ] = 992 ; 
	      helpline [ 1 ] = 993 ; 
	      helpline [ 0 ] = 915 ; 
	    } 
	    error () ; 
	  } 
	} 
	if ( mem [ r ] .hh.b0 == 1 ) 
	insertpenalties = insertpenalties + mem [ p + 1 ] .cint ; 
	else {
	    
	  mem [ r + 2 ] .hh .v.RH = p ; 
	  delta = pagesofar [ 0 ] - pagesofar [ 1 ] - pagesofar [ 7 ] + 
	  pagesofar [ 6 ] ; 
	  if ( eqtb [ 12718 + n ] .cint == 1000 ) 
	  h = mem [ p + 3 ] .cint ; 
	  else h = xovern ( mem [ p + 3 ] .cint , 1000 ) * eqtb [ 12718 + n ] 
	  .cint ; 
	  if ( ( ( h <= 0 ) || ( h <= delta ) ) && ( mem [ p + 3 ] .cint + mem 
	  [ r + 3 ] .cint <= eqtb [ 13251 + n ] .cint ) ) 
	  {
	    pagesofar [ 0 ] = pagesofar [ 0 ] - h ; 
	    mem [ r + 3 ] .cint = mem [ r + 3 ] .cint + mem [ p + 3 ] .cint ; 
	  } 
	  else {
	      
	    if ( eqtb [ 12718 + n ] .cint <= 0 ) 
	    w = 1073741823L ; 
	    else {
		
	      w = pagesofar [ 0 ] - pagesofar [ 1 ] - pagesofar [ 7 ] ; 
	      if ( eqtb [ 12718 + n ] .cint != 1000 ) 
	      w = xovern ( w , eqtb [ 12718 + n ] .cint ) * 1000 ; 
	    } 
	    if ( w > eqtb [ 13251 + n ] .cint - mem [ r + 3 ] .cint ) 
	    w = eqtb [ 13251 + n ] .cint - mem [ r + 3 ] .cint ; 
	    q = vertbreak ( mem [ p + 4 ] .hh .v.LH , w , mem [ p + 2 ] .cint 
	    ) ; 
	    mem [ r + 3 ] .cint = mem [ r + 3 ] .cint + bestheightplusdepth ; 
	;
#ifdef STAT
	    if ( eqtb [ 12696 ] .cint > 0 ) 
	    {
	      begindiagnostic () ; 
	      printnl ( 994 ) ; 
	      printint ( n ) ; 
	      print ( 995 ) ; 
	      printscaled ( w ) ; 
	      printchar ( 44 ) ; 
	      printscaled ( bestheightplusdepth ) ; 
	      print ( 924 ) ; 
	      if ( q == 0 ) 
	      printint ( -10000 ) ; 
	      else if ( mem [ q ] .hh.b0 == 12 ) 
	      printint ( mem [ q + 1 ] .cint ) ; 
	      else printchar ( 48 ) ; 
	      enddiagnostic ( false ) ; 
	    } 
#endif /* STAT */
	    if ( eqtb [ 12718 + n ] .cint != 1000 ) 
	    bestheightplusdepth = xovern ( bestheightplusdepth , 1000 ) * eqtb 
	    [ 12718 + n ] .cint ; 
	    pagesofar [ 0 ] = pagesofar [ 0 ] - bestheightplusdepth ; 
	    mem [ r ] .hh.b0 = 1 ; 
	    mem [ r + 1 ] .hh .v.RH = q ; 
	    mem [ r + 1 ] .hh .v.LH = p ; 
	    if ( q == 0 ) 
	    insertpenalties = insertpenalties - 10000 ; 
	    else if ( mem [ q ] .hh.b0 == 12 ) 
	    insertpenalties = insertpenalties + mem [ q + 1 ] .cint ; 
	  } 
	} 
	goto lab80 ; 
      } 
      break ; 
      default: 
      confusion ( 986 ) ; 
      break ; 
    } 
    if ( pi < 10000 ) 
    {
      if ( pagesofar [ 1 ] < pagesofar [ 0 ] ) 
      if ( ( pagesofar [ 3 ] != 0 ) || ( pagesofar [ 4 ] != 0 ) || ( pagesofar 
      [ 5 ] != 0 ) ) 
      b = 0 ; 
      else b = badness ( pagesofar [ 0 ] - pagesofar [ 1 ] , pagesofar [ 2 ] ) 
      ; 
      else if ( pagesofar [ 1 ] - pagesofar [ 0 ] > pagesofar [ 6 ] ) 
      b = 1073741823L ; 
      else b = badness ( pagesofar [ 1 ] - pagesofar [ 0 ] , pagesofar [ 6 ] ) 
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
      if ( eqtb [ 12696 ] .cint > 0 ) 
      {
	begindiagnostic () ; 
	printnl ( 37 ) ; 
	print ( 920 ) ; 
	printtotals () ; 
	print ( 989 ) ; 
	printscaled ( pagesofar [ 0 ] ) ; 
	print ( 923 ) ; 
	if ( b == 1073741823L ) 
	printchar ( 42 ) ; 
	else printint ( b ) ; 
	print ( 924 ) ; 
	printint ( pi ) ; 
	print ( 990 ) ; 
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
	bestsize = pagesofar [ 0 ] ; 
	leastpagecost = c ; 
	r = mem [ memtop ] .hh .v.RH ; 
	while ( r != memtop ) {
	    
	  mem [ r + 2 ] .hh .v.LH = mem [ r + 2 ] .hh .v.RH ; 
	  r = mem [ r ] .hh .v.RH ; 
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
    if ( ( mem [ p ] .hh.b0 < 10 ) || ( mem [ p ] .hh.b0 > 11 ) ) 
    goto lab80 ; 
    lab90: if ( mem [ p ] .hh.b0 == 11 ) 
    q = p ; 
    else {
	
      q = mem [ p + 1 ] .hh .v.LH ; 
      pagesofar [ 2 + mem [ q ] .hh.b0 ] = pagesofar [ 2 + mem [ q ] .hh.b0 ] 
      + mem [ q + 2 ] .cint ; 
      pagesofar [ 6 ] = pagesofar [ 6 ] + mem [ q + 3 ] .cint ; 
      if ( ( mem [ q ] .hh.b1 != 0 ) && ( mem [ q + 3 ] .cint != 0 ) ) 
      {
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 987 ) ; 
	} 
	{
	  helpptr = 4 ; 
	  helpline [ 3 ] = 988 ; 
	  helpline [ 2 ] = 956 ; 
	  helpline [ 1 ] = 957 ; 
	  helpline [ 0 ] = 915 ; 
	} 
	error () ; 
	r = newspec ( q ) ; 
	mem [ r ] .hh.b1 = 0 ; 
	deleteglueref ( q ) ; 
	mem [ p + 1 ] .hh .v.LH = r ; 
	q = r ; 
      } 
    } 
    pagesofar [ 1 ] = pagesofar [ 1 ] + pagesofar [ 7 ] + mem [ q + 1 ] .cint 
    ; 
    pagesofar [ 7 ] = 0 ; 
    lab80: if ( pagesofar [ 7 ] > pagemaxdepth ) 
    {
      pagesofar [ 1 ] = pagesofar [ 1 ] + pagesofar [ 7 ] - pagemaxdepth ; 
      pagesofar [ 7 ] = pagemaxdepth ; 
    } 
    mem [ pagetail ] .hh .v.RH = p ; 
    pagetail = p ; 
    mem [ memtop - 1 ] .hh .v.RH = mem [ p ] .hh .v.RH ; 
    mem [ p ] .hh .v.RH = 0 ; 
    goto lab30 ; 
    lab31: mem [ memtop - 1 ] .hh .v.RH = mem [ p ] .hh .v.RH ; 
    mem [ p ] .hh .v.RH = 0 ; 
    flushnodelist ( p ) ; 
    lab30: ; 
  } while ( ! ( mem [ memtop - 1 ] .hh .v.RH == 0 ) ) ; 
  if ( nestptr == 0 ) 
  curlist .tailfield = memtop - 1 ; 
  else nest [ 0 ] .tailfield = memtop - 1 ; 
} 
void appspace ( ) 
{appspace_regmem 
  halfword q  ; 
  if ( ( curlist .auxfield .hh .v.LH >= 2000 ) && ( eqtb [ 10295 ] .hh .v.RH 
  != 0 ) ) 
  q = newparamglue ( 13 ) ; 
  else {
      
    if ( eqtb [ 10294 ] .hh .v.RH != 0 ) 
    mainp = eqtb [ 10294 ] .hh .v.RH ; 
    else {
	
      mainp = fontglue [ eqtb [ 11334 ] .hh .v.RH ] ; 
      if ( mainp == 0 ) 
      {
	mainp = newspec ( 0 ) ; 
	maink = parambase [ eqtb [ 11334 ] .hh .v.RH ] + 2 ; 
	mem [ mainp + 1 ] .cint = fontinfo [ maink ] .cint ; 
	mem [ mainp + 2 ] .cint = fontinfo [ maink + 1 ] .cint ; 
	mem [ mainp + 3 ] .cint = fontinfo [ maink + 2 ] .cint ; 
	fontglue [ eqtb [ 11334 ] .hh .v.RH ] = mainp ; 
      } 
    } 
    mainp = newspec ( mainp ) ; 
    if ( curlist .auxfield .hh .v.LH >= 2000 ) 
    mem [ mainp + 1 ] .cint = mem [ mainp + 1 ] .cint + fontinfo [ 7 + 
    parambase [ eqtb [ 11334 ] .hh .v.RH ] ] .cint ; 
    mem [ mainp + 2 ] .cint = xnoverd ( mem [ mainp + 2 ] .cint , curlist 
    .auxfield .hh .v.LH , 1000 ) ; 
    mem [ mainp + 3 ] .cint = xnoverd ( mem [ mainp + 3 ] .cint , 1000 , 
    curlist .auxfield .hh .v.LH ) ; 
    q = newglue ( mainp ) ; 
    mem [ mainp ] .hh .v.RH = 0 ; 
  } 
  mem [ curlist .tailfield ] .hh .v.RH = q ; 
  curlist .tailfield = q ; 
} 
void insertdollarsign ( ) 
{insertdollarsign_regmem 
  backinput () ; 
  curtok = 804 ; 
  {
    if ( interaction == 3 ) 
    ; 
    printnl ( 262 ) ; 
    print ( 1010 ) ; 
  } 
  {
    helpptr = 2 ; 
    helpline [ 1 ] = 1011 ; 
    helpline [ 0 ] = 1012 ; 
  } 
  inserror () ; 
} 
void youcant ( ) 
{youcant_regmem 
  {
    if ( interaction == 3 ) 
    ; 
    printnl ( 262 ) ; 
    print ( 681 ) ; 
  } 
  printcmdchr ( curcmd , curchr ) ; 
  print ( 1013 ) ; 
  printmode ( curlist .modefield ) ; 
} 
void reportillegalcase ( ) 
{reportillegalcase_regmem 
  youcant () ; 
  {
    helpptr = 4 ; 
    helpline [ 3 ] = 1014 ; 
    helpline [ 2 ] = 1015 ; 
    helpline [ 1 ] = 1016 ; 
    helpline [ 0 ] = 1017 ; 
  } 
  error () ; 
} 
boolean privileged ( ) 
{register boolean Result; privileged_regmem 
  if ( curlist .modefield > 0 ) 
  Result = true ; 
  else {
      
    reportillegalcase () ; 
    Result = false ; 
  } 
  return(Result) ; 
} 
boolean itsallover ( ) 
{/* 10 */ register boolean Result; itsallover_regmem 
  if ( privileged () ) 
  {
    if ( ( memtop - 2 == pagetail ) && ( curlist .headfield == curlist 
    .tailfield ) && ( deadcycles == 0 ) ) 
    {
      Result = true ; 
      return(Result) ; 
    } 
    backinput () ; 
    {
      mem [ curlist .tailfield ] .hh .v.RH = newnullbox () ; 
      curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
    } 
    mem [ curlist .tailfield + 1 ] .cint = eqtb [ 13233 ] .cint ; 
    {
      mem [ curlist .tailfield ] .hh .v.RH = newglue ( 8 ) ; 
      curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
    } 
    {
      mem [ curlist .tailfield ] .hh .v.RH = newpenalty ( -1073741824L ) ; 
      curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
    } 
    buildpage () ; 
  } 
  Result = false ; 
  return(Result) ; 
} 
void appendglue ( ) 
{appendglue_regmem 
  smallnumber s  ; 
  s = curchr ; 
  switch ( s ) 
  {case 0 : 
    curval = 4 ; 
    break ; 
  case 1 : 
    curval = 8 ; 
    break ; 
  case 2 : 
    curval = 12 ; 
    break ; 
  case 3 : 
    curval = 16 ; 
    break ; 
  case 4 : 
    scanglue ( 2 ) ; 
    break ; 
  case 5 : 
    scanglue ( 3 ) ; 
    break ; 
  } 
  {
    mem [ curlist .tailfield ] .hh .v.RH = newglue ( curval ) ; 
    curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
  } 
  if ( s >= 4 ) 
  {
    decr ( mem [ curval ] .hh .v.RH ) ; 
    if ( s > 4 ) 
    mem [ curlist .tailfield ] .hh.b1 = 99 ; 
  } 
} 
void appendkern ( ) 
{appendkern_regmem 
  quarterword s  ; 
  s = curchr ; 
  scandimen ( s == 99 , false , false ) ; 
  {
    mem [ curlist .tailfield ] .hh .v.RH = newkern ( curval ) ; 
    curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
  } 
  mem [ curlist .tailfield ] .hh.b1 = s ; 
} 
void offsave ( ) 
{offsave_regmem 
  halfword p  ; 
  if ( curgroup == 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 772 ) ; 
    } 
    printcmdchr ( curcmd , curchr ) ; 
    {
      helpptr = 1 ; 
      helpline [ 0 ] = 1036 ; 
    } 
    error () ; 
  } 
  else {
      
    backinput () ; 
    p = getavail () ; 
    mem [ memtop - 3 ] .hh .v.RH = p ; 
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 621 ) ; 
    } 
    switch ( curgroup ) 
    {case 14 : 
      {
	mem [ p ] .hh .v.LH = 14111 ; 
	printesc ( 512 ) ; 
      } 
      break ; 
    case 15 : 
      {
	mem [ p ] .hh .v.LH = 804 ; 
	printchar ( 36 ) ; 
      } 
      break ; 
    case 16 : 
      {
	mem [ p ] .hh .v.LH = 14112 ; 
	mem [ p ] .hh .v.RH = getavail () ; 
	p = mem [ p ] .hh .v.RH ; 
	mem [ p ] .hh .v.LH = 3118 ; 
	printesc ( 1035 ) ; 
      } 
      break ; 
      default: 
      {
	mem [ p ] .hh .v.LH = 637 ; 
	printchar ( 125 ) ; 
      } 
      break ; 
    } 
    print ( 622 ) ; 
    begintokenlist ( mem [ memtop - 3 ] .hh .v.RH , 4 ) ; 
    {
      helpptr = 5 ; 
      helpline [ 4 ] = 1030 ; 
      helpline [ 3 ] = 1031 ; 
      helpline [ 2 ] = 1032 ; 
      helpline [ 1 ] = 1033 ; 
      helpline [ 0 ] = 1034 ; 
    } 
    error () ; 
  } 
} 
