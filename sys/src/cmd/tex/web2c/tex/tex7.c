#define EXTERN extern
#include "texd.h"

void extrarightbrace ( ) 
{extrarightbrace_regmem 
  {
    if ( interaction == 3 ) 
    ; 
    printnl ( 262 ) ; 
    print ( 1041 ) ; 
  } 
  switch ( curgroup ) 
  {case 14 : 
    printesc ( 512 ) ; 
    break ; 
  case 15 : 
    printchar ( 36 ) ; 
    break ; 
  case 16 : 
    printesc ( 870 ) ; 
    break ; 
  } 
  {
    helpptr = 5 ; 
    helpline [ 4 ] = 1042 ; 
    helpline [ 3 ] = 1043 ; 
    helpline [ 2 ] = 1044 ; 
    helpline [ 1 ] = 1045 ; 
    helpline [ 0 ] = 1046 ; 
  } 
  error () ; 
  incr ( alignstate ) ; 
} 
void normalparagraph ( ) 
{normalparagraph_regmem 
  if ( eqtb [ 12682 ] .cint != 0 ) 
  eqworddefine ( 12682 , 0 ) ; 
  if ( eqtb [ 13247 ] .cint != 0 ) 
  eqworddefine ( 13247 , 0 ) ; 
  if ( eqtb [ 12704 ] .cint != 1 ) 
  eqworddefine ( 12704 , 1 ) ; 
  if ( eqtb [ 10812 ] .hh .v.RH != 0 ) 
  eqdefine ( 10812 , 118 , 0 ) ; 
} 
void zboxend ( boxcontext ) 
integer boxcontext ; 
{boxend_regmem 
  halfword p  ; 
  if ( boxcontext < 1073741824L ) 
  {
    if ( curbox != 0 ) 
    {
      mem [ curbox + 4 ] .cint = boxcontext ; 
      if ( abs ( curlist .modefield ) == 1 ) 
      {
	appendtovlist ( curbox ) ; 
	if ( adjusttail != 0 ) 
	{
	  if ( memtop - 5 != adjusttail ) 
	  {
	    mem [ curlist .tailfield ] .hh .v.RH = mem [ memtop - 5 ] .hh 
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
	  mem [ p + 1 ] .hh .v.RH = 2 ; 
	  mem [ p + 1 ] .hh .v.LH = curbox ; 
	  curbox = p ; 
	} 
	mem [ curlist .tailfield ] .hh .v.RH = curbox ; 
	curlist .tailfield = curbox ; 
      } 
    } 
  } 
  else if ( boxcontext < 1073742336L ) 
  if ( boxcontext < 1073742080L ) 
  eqdefine ( -1073730746L + boxcontext , 119 , curbox ) ; 
  else geqdefine ( -1073731002L + boxcontext , 119 , curbox ) ; 
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
      mem [ curlist .tailfield ] .hh.b1 = boxcontext - ( 1073742237L ) ; 
      mem [ curlist .tailfield + 1 ] .hh .v.RH = curbox ; 
    } 
    else {
	
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 262 ) ; 
	print ( 1059 ) ; 
      } 
      {
	helpptr = 3 ; 
	helpline [ 2 ] = 1060 ; 
	helpline [ 1 ] = 1061 ; 
	helpline [ 0 ] = 1062 ; 
      } 
      backerror () ; 
      flushnodelist ( curbox ) ; 
    } 
  } 
  else shipout ( curbox ) ; 
} 
void zbeginbox ( boxcontext ) 
integer boxcontext ; 
{/* 10 30 */ beginbox_regmem 
  halfword p, q  ; 
  quarterword m  ; 
  halfword k  ; 
  eightbits n  ; 
  switch ( curchr ) 
  {case 0 : 
    {
      scaneightbitint () ; 
      curbox = eqtb [ 11078 + curval ] .hh .v.RH ; 
      eqtb [ 11078 + curval ] .hh .v.RH = 0 ; 
    } 
    break ; 
  case 1 : 
    {
      scaneightbitint () ; 
      curbox = copynodelist ( eqtb [ 11078 + curval ] .hh .v.RH ) ; 
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
	  helpline [ 0 ] = 1063 ; 
	} 
	error () ; 
      } 
      else if ( ( curlist .modefield == 1 ) && ( curlist .headfield == curlist 
      .tailfield ) ) 
      {
	youcant () ; 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 1064 ; 
	  helpline [ 0 ] = 1065 ; 
	} 
	error () ; 
      } 
      else {
	  
	if ( ! ( curlist .tailfield >= himemmin ) ) 
	if ( ( mem [ curlist .tailfield ] .hh.b0 == 0 ) || ( mem [ curlist 
	.tailfield ] .hh.b0 == 1 ) ) 
	{
	  q = curlist .headfield ; 
	  do {
	      p = q ; 
	    if ( ! ( q >= himemmin ) ) 
	    if ( mem [ q ] .hh.b0 == 7 ) 
	    {
	      {register integer for_end; m = 1 ; for_end = mem [ q ] .hh.b1 
	      ; if ( m <= for_end) do 
		p = mem [ p ] .hh .v.RH ; 
	      while ( m++ < for_end ) ; } 
	      if ( p == curlist .tailfield ) 
	      goto lab30 ; 
	    } 
	    q = mem [ p ] .hh .v.RH ; 
	  } while ( ! ( q == curlist .tailfield ) ) ; 
	  curbox = curlist .tailfield ; 
	  mem [ curbox + 4 ] .cint = 0 ; 
	  curlist .tailfield = p ; 
	  mem [ p ] .hh .v.RH = 0 ; 
	  lab30: ; 
	} 
      } 
    } 
    break ; 
  case 3 : 
    {
      scaneightbitint () ; 
      n = curval ; 
      if ( ! scankeyword ( 835 ) ) 
      {
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 1066 ) ; 
	} 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 1067 ; 
	  helpline [ 0 ] = 1068 ; 
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
      savestack [ saveptr + 0 ] .cint = boxcontext ; 
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
	if ( eqtb [ 10818 ] .hh .v.RH != 0 ) 
	begintokenlist ( eqtb [ 10818 ] .hh .v.RH , 11 ) ; 
      } 
      else {
	  
	curlist .auxfield .hh .v.LH = 1000 ; 
	if ( eqtb [ 10817 ] .hh .v.RH != 0 ) 
	begintokenlist ( eqtb [ 10817 ] .hh .v.RH , 10 ) ; 
      } 
      return ; 
    } 
    break ; 
  } 
  boxend ( boxcontext ) ; 
} 
void zscanbox ( boxcontext ) 
integer boxcontext ; 
{scanbox_regmem 
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
      print ( 1069 ) ; 
    } 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 1070 ; 
      helpline [ 1 ] = 1071 ; 
      helpline [ 0 ] = 1072 ; 
    } 
    backerror () ; 
  } 
} 
void zpackage ( c ) 
smallnumber c ; 
{package_regmem 
  scaled h  ; 
  halfword p  ; 
  scaled d  ; 
  d = eqtb [ 13237 ] .cint ; 
  unsave () ; 
  saveptr = saveptr - 3 ; 
  if ( curlist .modefield == -102 ) 
  curbox = hpack ( mem [ curlist .headfield ] .hh .v.RH , savestack [ saveptr 
  + 2 ] .cint , savestack [ saveptr + 1 ] .cint ) ; 
  else {
      
    curbox = vpackage ( mem [ curlist .headfield ] .hh .v.RH , savestack [ 
    saveptr + 2 ] .cint , savestack [ saveptr + 1 ] .cint , d ) ; 
    if ( c == 4 ) 
    {
      h = 0 ; 
      p = mem [ curbox + 5 ] .hh .v.RH ; 
      if ( p != 0 ) 
      if ( mem [ p ] .hh.b0 <= 2 ) 
      h = mem [ p + 3 ] .cint ; 
      mem [ curbox + 2 ] .cint = mem [ curbox + 2 ] .cint - h + mem [ curbox + 
      3 ] .cint ; 
      mem [ curbox + 3 ] .cint = h ; 
    } 
  } 
  popnest () ; 
  boxend ( savestack [ saveptr + 0 ] .cint ) ; 
} 
smallnumber znormmin ( h ) 
integer h ; 
{register smallnumber Result; normmin_regmem 
  if ( h <= 0 ) 
  Result = 1 ; 
  else if ( h >= 63 ) 
  Result = 63 ; 
  else Result = h ; 
  return(Result) ; 
} 
void znewgraf ( indented ) 
boolean indented ; 
{newgraf_regmem 
  curlist .pgfield = 0 ; 
  if ( ( curlist .modefield == 1 ) || ( curlist .headfield != curlist 
  .tailfield ) ) 
  {
    mem [ curlist .tailfield ] .hh .v.RH = newparamglue ( 2 ) ; 
    curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
  } 
  curlist .lhmfield = normmin ( eqtb [ 12714 ] .cint ) ; 
  curlist .rhmfield = normmin ( eqtb [ 12715 ] .cint ) ; 
  pushnest () ; 
  curlist .modefield = 102 ; 
  curlist .auxfield .hh .v.LH = 1000 ; 
  curlist .auxfield .hh .v.RH = 0 ; 
  if ( indented ) 
  {
    curlist .tailfield = newnullbox () ; 
    mem [ curlist .headfield ] .hh .v.RH = curlist .tailfield ; 
    mem [ curlist .tailfield + 1 ] .cint = eqtb [ 13230 ] .cint ; 
  } 
  if ( eqtb [ 10814 ] .hh .v.RH != 0 ) 
  begintokenlist ( eqtb [ 10814 ] .hh .v.RH , 7 ) ; 
  if ( nestptr == 1 ) 
  buildpage () ; 
} 
void indentinhmode ( ) 
{indentinhmode_regmem 
  halfword p, q  ; 
  if ( curchr > 0 ) 
  {
    p = newnullbox () ; 
    mem [ p + 1 ] .cint = eqtb [ 13230 ] .cint ; 
    if ( abs ( curlist .modefield ) == 102 ) 
    curlist .auxfield .hh .v.LH = 1000 ; 
    else {
	
      q = newnoad () ; 
      mem [ q + 1 ] .hh .v.RH = 2 ; 
      mem [ q + 1 ] .hh .v.LH = p ; 
      p = q ; 
    } 
    {
      mem [ curlist .tailfield ] .hh .v.RH = p ; 
      curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
    } 
  } 
} 
void headforvmode ( ) 
{headforvmode_regmem 
  if ( curlist .modefield < 0 ) 
  if ( curcmd != 36 ) 
  offsave () ; 
  else {
      
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 681 ) ; 
    } 
    printesc ( 517 ) ; 
    print ( 1075 ) ; 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 1076 ; 
      helpline [ 0 ] = 1077 ; 
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
void endgraf ( ) 
{endgraf_regmem 
  if ( curlist .modefield == 102 ) 
  {
    if ( curlist .headfield == curlist .tailfield ) 
    popnest () ; 
    else linebreak ( eqtb [ 12669 ] .cint ) ; 
    normalparagraph () ; 
    errorcount = 0 ; 
  } 
} 
void begininsertoradjust ( ) 
{begininsertoradjust_regmem 
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
	print ( 1078 ) ; 
      } 
      printesc ( 327 ) ; 
      printint ( 255 ) ; 
      {
	helpptr = 1 ; 
	helpline [ 0 ] = 1079 ; 
      } 
      error () ; 
      curval = 0 ; 
    } 
  } 
  savestack [ saveptr + 0 ] .cint = curval ; 
  incr ( saveptr ) ; 
  newsavelevel ( 11 ) ; 
  scanleftbrace () ; 
  normalparagraph () ; 
  pushnest () ; 
  curlist .modefield = -1 ; 
  curlist .auxfield .cint = -65536000L ; 
} 
void makemark ( ) 
{makemark_regmem 
  halfword p  ; 
  p = scantoks ( false , true ) ; 
  p = getnode ( 2 ) ; 
  mem [ p ] .hh.b0 = 4 ; 
  mem [ p ] .hh.b1 = 0 ; 
  mem [ p + 1 ] .cint = defref ; 
  mem [ curlist .tailfield ] .hh .v.RH = p ; 
  curlist .tailfield = p ; 
} 
void appendpenalty ( ) 
{appendpenalty_regmem 
  scanint () ; 
  {
    mem [ curlist .tailfield ] .hh .v.RH = newpenalty ( curval ) ; 
    curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
  } 
  if ( curlist .modefield == 1 ) 
  buildpage () ; 
} 
void deletelast ( ) 
{/* 10 */ deletelast_regmem 
  halfword p, q  ; 
  quarterword m  ; 
  if ( ( curlist .modefield == 1 ) && ( curlist .tailfield == curlist 
  .headfield ) ) 
  {
    if ( ( curchr != 10 ) || ( lastglue != 262143L ) ) 
    {
      youcant () ; 
      {
	helpptr = 2 ; 
	helpline [ 1 ] = 1064 ; 
	helpline [ 0 ] = 1080 ; 
      } 
      if ( curchr == 11 ) 
      helpline [ 0 ] = ( 1081 ) ; 
      else if ( curchr != 10 ) 
      helpline [ 0 ] = ( 1082 ) ; 
      error () ; 
    } 
  } 
  else {
      
    if ( ! ( curlist .tailfield >= himemmin ) ) 
    if ( mem [ curlist .tailfield ] .hh.b0 == curchr ) 
    {
      q = curlist .headfield ; 
      do {
	  p = q ; 
	if ( ! ( q >= himemmin ) ) 
	if ( mem [ q ] .hh.b0 == 7 ) 
	{
	  {register integer for_end; m = 1 ; for_end = mem [ q ] .hh.b1 
	  ; if ( m <= for_end) do 
	    p = mem [ p ] .hh .v.RH ; 
	  while ( m++ < for_end ) ; } 
	  if ( p == curlist .tailfield ) 
	  return ; 
	} 
	q = mem [ p ] .hh .v.RH ; 
      } while ( ! ( q == curlist .tailfield ) ) ; 
      mem [ p ] .hh .v.RH = 0 ; 
      flushnodelist ( curlist .tailfield ) ; 
      curlist .tailfield = p ; 
    } 
  } 
} 
void unpackage ( ) 
{/* 10 */ unpackage_regmem 
  halfword p  ; 
  schar c  ; 
  c = curchr ; 
  scaneightbitint () ; 
  p = eqtb [ 11078 + curval ] .hh .v.RH ; 
  if ( p == 0 ) 
  return ; 
  if ( ( abs ( curlist .modefield ) == 203 ) || ( ( abs ( curlist .modefield ) 
  == 1 ) && ( mem [ p ] .hh.b0 != 1 ) ) || ( ( abs ( curlist .modefield ) == 
  102 ) && ( mem [ p ] .hh.b0 != 0 ) ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 1090 ) ; 
    } 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 1091 ; 
      helpline [ 1 ] = 1092 ; 
      helpline [ 0 ] = 1093 ; 
    } 
    error () ; 
    return ; 
  } 
  if ( c == 1 ) 
  mem [ curlist .tailfield ] .hh .v.RH = copynodelist ( mem [ p + 5 ] .hh 
  .v.RH ) ; 
  else {
      
    mem [ curlist .tailfield ] .hh .v.RH = mem [ p + 5 ] .hh .v.RH ; 
    eqtb [ 11078 + curval ] .hh .v.RH = 0 ; 
    freenode ( p , 7 ) ; 
  } 
  while ( mem [ curlist .tailfield ] .hh .v.RH != 0 ) curlist .tailfield = mem 
  [ curlist .tailfield ] .hh .v.RH ; 
} 
void appenditaliccorrection ( ) 
{/* 10 */ appenditaliccorrection_regmem 
  halfword p  ; 
  internalfontnumber f  ; 
  if ( curlist .tailfield != curlist .headfield ) 
  {
    if ( ( curlist .tailfield >= himemmin ) ) 
    p = curlist .tailfield ; 
    else if ( mem [ curlist .tailfield ] .hh.b0 == 6 ) 
    p = curlist .tailfield + 1 ; 
    else return ; 
    f = mem [ p ] .hh.b0 ; 
    {
      mem [ curlist .tailfield ] .hh .v.RH = newkern ( fontinfo [ italicbase [ 
      f ] + ( fontinfo [ charbase [ f ] + mem [ p ] .hh.b1 ] .qqqq .b2 ) / 4 ] 
      .cint ) ; 
      curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
    } 
    mem [ curlist .tailfield ] .hh.b1 = 1 ; 
  } 
} 
void appenddiscretionary ( ) 
{appenddiscretionary_regmem 
  integer c  ; 
  {
    mem [ curlist .tailfield ] .hh .v.RH = newdisc () ; 
    curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
  } 
  if ( curchr == 1 ) 
  {
    c = hyphenchar [ eqtb [ 11334 ] .hh .v.RH ] ; 
    if ( c >= 0 ) 
    if ( c < 256 ) 
    mem [ curlist .tailfield + 1 ] .hh .v.LH = newcharacter ( eqtb [ 11334 ] 
    .hh .v.RH , c ) ; 
  } 
  else {
      
    incr ( saveptr ) ; 
    savestack [ saveptr - 1 ] .cint = 0 ; 
    newsavelevel ( 10 ) ; 
    scanleftbrace () ; 
    pushnest () ; 
    curlist .modefield = -102 ; 
    curlist .auxfield .hh .v.LH = 1000 ; 
  } 
} 
void builddiscretionary ( ) 
{/* 30 10 */ builddiscretionary_regmem 
  halfword p, q  ; 
  integer n  ; 
  unsave () ; 
  q = curlist .headfield ; 
  p = mem [ q ] .hh .v.RH ; 
  n = 0 ; 
  while ( p != 0 ) {
      
    if ( ! ( p >= himemmin ) ) 
    if ( mem [ p ] .hh.b0 > 2 ) 
    if ( mem [ p ] .hh.b0 != 11 ) 
    if ( mem [ p ] .hh.b0 != 6 ) 
    {
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 262 ) ; 
	print ( 1100 ) ; 
      } 
      {
	helpptr = 1 ; 
	helpline [ 0 ] = 1101 ; 
      } 
      error () ; 
      begindiagnostic () ; 
      printnl ( 1102 ) ; 
      showbox ( p ) ; 
      enddiagnostic ( true ) ; 
      flushnodelist ( p ) ; 
      mem [ q ] .hh .v.RH = 0 ; 
      goto lab30 ; 
    } 
    q = p ; 
    p = mem [ q ] .hh .v.RH ; 
    incr ( n ) ; 
  } 
  lab30: ; 
  p = mem [ curlist .headfield ] .hh .v.RH ; 
  popnest () ; 
  switch ( savestack [ saveptr - 1 ] .cint ) 
  {case 0 : 
    mem [ curlist .tailfield + 1 ] .hh .v.LH = p ; 
    break ; 
  case 1 : 
    mem [ curlist .tailfield + 1 ] .hh .v.RH = p ; 
    break ; 
  case 2 : 
    {
      if ( ( n > 0 ) && ( abs ( curlist .modefield ) == 203 ) ) 
      {
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 1094 ) ; 
	} 
	printesc ( 346 ) ; 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 1095 ; 
	  helpline [ 0 ] = 1096 ; 
	} 
	flushnodelist ( p ) ; 
	n = 0 ; 
	error () ; 
      } 
      else mem [ curlist .tailfield ] .hh .v.RH = p ; 
      if ( n <= 255 ) 
      mem [ curlist .tailfield ] .hh.b1 = n ; 
      else {
	  
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 1097 ) ; 
	} 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 1098 ; 
	  helpline [ 0 ] = 1099 ; 
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
  incr ( savestack [ saveptr - 1 ] .cint ) ; 
  newsavelevel ( 10 ) ; 
  scanleftbrace () ; 
  pushnest () ; 
  curlist .modefield = -102 ; 
  curlist .auxfield .hh .v.LH = 1000 ; 
} 
void makeaccent ( ) 
{makeaccent_regmem 
  real s, t  ; 
  halfword p, q, r  ; 
  internalfontnumber f  ; 
  scaled a, h, x, w, delta  ; 
  fourquarters i  ; 
  scancharnum () ; 
  f = eqtb [ 11334 ] .hh .v.RH ; 
  p = newcharacter ( f , curval ) ; 
  if ( p != 0 ) 
  {
    x = fontinfo [ 5 + parambase [ f ] ] .cint ; 
    s = fontinfo [ 1 + parambase [ f ] ] .cint / ((double) 65536.0 ) ; 
    a = fontinfo [ widthbase [ f ] + fontinfo [ charbase [ f ] + mem [ p ] 
    .hh.b1 ] .qqqq .b0 ] .cint ; 
    doassignments () ; 
    q = 0 ; 
    f = eqtb [ 11334 ] .hh .v.RH ; 
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
      t = fontinfo [ 1 + parambase [ f ] ] .cint / ((double) 65536.0 ) ; 
      i = fontinfo [ charbase [ f ] + mem [ q ] .hh.b1 ] .qqqq ; 
      w = fontinfo [ widthbase [ f ] + i .b0 ] .cint ; 
      h = fontinfo [ heightbase [ f ] + ( i .b1 ) / 16 ] .cint ; 
      if ( h != x ) 
      {
	p = hpack ( p , 0 , 1 ) ; 
	mem [ p + 4 ] .cint = x - h ; 
      } 
      delta = round ( ( w - a ) / ((double) 2.0 ) + h * t - x * s ) ; 
      r = newkern ( delta ) ; 
      mem [ r ] .hh.b1 = 2 ; 
      mem [ curlist .tailfield ] .hh .v.RH = r ; 
      mem [ r ] .hh .v.RH = p ; 
      curlist .tailfield = newkern ( - (integer) a - delta ) ; 
      mem [ curlist .tailfield ] .hh.b1 = 2 ; 
      mem [ p ] .hh .v.RH = curlist .tailfield ; 
      p = q ; 
    } 
    mem [ curlist .tailfield ] .hh .v.RH = p ; 
    curlist .tailfield = p ; 
    curlist .auxfield .hh .v.LH = 1000 ; 
  } 
} 
void alignerror ( ) 
{alignerror_regmem 
  if ( abs ( alignstate ) > 2 ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 1107 ) ; 
    } 
    printcmdchr ( curcmd , curchr ) ; 
    if ( curtok == 1062 ) 
    {
      {
	helpptr = 6 ; 
	helpline [ 5 ] = 1108 ; 
	helpline [ 4 ] = 1109 ; 
	helpline [ 3 ] = 1110 ; 
	helpline [ 2 ] = 1111 ; 
	helpline [ 1 ] = 1112 ; 
	helpline [ 0 ] = 1113 ; 
      } 
    } 
    else {
	
      {
	helpptr = 5 ; 
	helpline [ 4 ] = 1108 ; 
	helpline [ 3 ] = 1114 ; 
	helpline [ 2 ] = 1111 ; 
	helpline [ 1 ] = 1112 ; 
	helpline [ 0 ] = 1113 ; 
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
	print ( 653 ) ; 
      } 
      incr ( alignstate ) ; 
      curtok = 379 ; 
    } 
    else {
	
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 262 ) ; 
	print ( 1103 ) ; 
      } 
      decr ( alignstate ) ; 
      curtok = 637 ; 
    } 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 1104 ; 
      helpline [ 1 ] = 1105 ; 
      helpline [ 0 ] = 1106 ; 
    } 
    inserror () ; 
  } 
} 
void noalignerror ( ) 
{noalignerror_regmem 
  {
    if ( interaction == 3 ) 
    ; 
    printnl ( 262 ) ; 
    print ( 1107 ) ; 
  } 
  printesc ( 523 ) ; 
  {
    helpptr = 2 ; 
    helpline [ 1 ] = 1115 ; 
    helpline [ 0 ] = 1116 ; 
  } 
  error () ; 
} 
void omiterror ( ) 
{omiterror_regmem 
  {
    if ( interaction == 3 ) 
    ; 
    printnl ( 262 ) ; 
    print ( 1107 ) ; 
  } 
  printesc ( 526 ) ; 
  {
    helpptr = 2 ; 
    helpline [ 1 ] = 1117 ; 
    helpline [ 0 ] = 1116 ; 
  } 
  error () ; 
} 
void doendv ( ) 
{doendv_regmem 
  if ( curgroup == 6 ) 
  {
    endgraf () ; 
    if ( fincol () ) 
    finrow () ; 
  } 
  else offsave () ; 
} 
void cserror ( ) 
{cserror_regmem 
  {
    if ( interaction == 3 ) 
    ; 
    printnl ( 262 ) ; 
    print ( 772 ) ; 
  } 
  printesc ( 501 ) ; 
  {
    helpptr = 1 ; 
    helpline [ 0 ] = 1119 ; 
  } 
  error () ; 
} 
void zpushmath ( c ) 
groupcode c ; 
{pushmath_regmem 
  pushnest () ; 
  curlist .modefield = -203 ; 
  curlist .auxfield .cint = 0 ; 
  newsavelevel ( c ) ; 
} 
void initmath ( ) 
{/* 21 40 45 30 */ initmath_regmem 
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
	
      linebreak ( eqtb [ 12670 ] .cint ) ; 
      v = mem [ justbox + 4 ] .cint + 2 * fontinfo [ 6 + parambase [ eqtb [ 
      11334 ] .hh .v.RH ] ] .cint ; 
      w = -1073741823L ; 
      p = mem [ justbox + 5 ] .hh .v.RH ; 
      while ( p != 0 ) {
	  
	lab21: if ( ( p >= himemmin ) ) 
	{
	  f = mem [ p ] .hh.b0 ; 
	  d = fontinfo [ widthbase [ f ] + fontinfo [ charbase [ f ] + mem [ p 
	  ] .hh.b1 ] .qqqq .b0 ] .cint ; 
	  goto lab40 ; 
	} 
	switch ( mem [ p ] .hh.b0 ) 
	{case 0 : 
	case 1 : 
	case 2 : 
	  {
	    d = mem [ p + 1 ] .cint ; 
	    goto lab40 ; 
	  } 
	  break ; 
	case 6 : 
	  {
	    mem [ memtop - 12 ] = mem [ p + 1 ] ; 
	    mem [ memtop - 12 ] .hh .v.RH = mem [ p ] .hh .v.RH ; 
	    p = memtop - 12 ; 
	    goto lab21 ; 
	  } 
	  break ; 
	case 11 : 
	case 9 : 
	  d = mem [ p + 1 ] .cint ; 
	  break ; 
	case 10 : 
	  {
	    q = mem [ p + 1 ] .hh .v.LH ; 
	    d = mem [ q + 1 ] .cint ; 
	    if ( mem [ justbox + 5 ] .hh.b0 == 1 ) 
	    {
	      if ( ( mem [ justbox + 5 ] .hh.b1 == mem [ q ] .hh.b0 ) && ( mem 
	      [ q + 2 ] .cint != 0 ) ) 
	      v = 1073741823L ; 
	    } 
	    else if ( mem [ justbox + 5 ] .hh.b0 == 2 ) 
	    {
	      if ( ( mem [ justbox + 5 ] .hh.b1 == mem [ q ] .hh.b1 ) && ( mem 
	      [ q + 3 ] .cint != 0 ) ) 
	      v = 1073741823L ; 
	    } 
	    if ( mem [ p ] .hh.b1 >= 100 ) 
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
	lab45: p = mem [ p ] .hh .v.RH ; 
      } 
      lab30: ; 
    } 
    if ( eqtb [ 10812 ] .hh .v.RH == 0 ) 
    if ( ( eqtb [ 13247 ] .cint != 0 ) && ( ( ( eqtb [ 12704 ] .cint >= 0 ) && 
    ( curlist .pgfield + 2 > eqtb [ 12704 ] .cint ) ) || ( curlist .pgfield + 
    1 < - (integer) eqtb [ 12704 ] .cint ) ) ) 
    {
      l = eqtb [ 13233 ] .cint - abs ( eqtb [ 13247 ] .cint ) ; 
      if ( eqtb [ 13247 ] .cint > 0 ) 
      s = eqtb [ 13247 ] .cint ; 
      else s = 0 ; 
    } 
    else {
	
      l = eqtb [ 13233 ] .cint ; 
      s = 0 ; 
    } 
    else {
	
      n = mem [ eqtb [ 10812 ] .hh .v.RH ] .hh .v.LH ; 
      if ( curlist .pgfield + 2 >= n ) 
      p = eqtb [ 10812 ] .hh .v.RH + 2 * n ; 
      else p = eqtb [ 10812 ] .hh .v.RH + 2 * ( curlist .pgfield + 2 ) ; 
      s = mem [ p - 1 ] .cint ; 
      l = mem [ p ] .cint ; 
    } 
    pushmath ( 15 ) ; 
    curlist .modefield = 203 ; 
    eqworddefine ( 12707 , -1 ) ; 
    eqworddefine ( 13243 , w ) ; 
    eqworddefine ( 13244 , l ) ; 
    eqworddefine ( 13245 , s ) ; 
    if ( eqtb [ 10816 ] .hh .v.RH != 0 ) 
    begintokenlist ( eqtb [ 10816 ] .hh .v.RH , 9 ) ; 
    if ( nestptr == 1 ) 
    buildpage () ; 
  } 
  else {
      
    backinput () ; 
    {
      pushmath ( 15 ) ; 
      eqworddefine ( 12707 , -1 ) ; 
      if ( eqtb [ 10815 ] .hh .v.RH != 0 ) 
      begintokenlist ( eqtb [ 10815 ] .hh .v.RH , 8 ) ; 
    } 
  } 
} 
void starteqno ( ) 
{starteqno_regmem 
  savestack [ saveptr + 0 ] .cint = curchr ; 
  incr ( saveptr ) ; 
  {
    pushmath ( 15 ) ; 
    eqworddefine ( 12707 , -1 ) ; 
    if ( eqtb [ 10815 ] .hh .v.RH != 0 ) 
    begintokenlist ( eqtb [ 10815 ] .hh .v.RH , 8 ) ; 
  } 
} 
void zscanmath ( p ) 
halfword p ; 
{/* 20 21 10 */ scanmath_regmem 
  integer c  ; 
  lab20: do {
      getxtoken () ; 
  } while ( ! ( ( curcmd != 10 ) && ( curcmd != 0 ) ) ) ; 
  lab21: switch ( curcmd ) 
  {case 11 : 
  case 12 : 
  case 68 : 
    {
      c = eqtb [ 12407 + curchr ] .hh .v.RH ; 
      if ( c == 32768L ) 
      {
	{
	  curcs = curchr + 1 ; 
	  curcmd = eqtb [ curcs ] .hh.b0 ; 
	  curchr = eqtb [ curcs ] .hh .v.RH ; 
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
      savestack [ saveptr + 0 ] .cint = p ; 
      incr ( saveptr ) ; 
      pushmath ( 9 ) ; 
      return ; 
    } 
    break ; 
  } 
  mem [ p ] .hh .v.RH = 1 ; 
  mem [ p ] .hh.b1 = c % 256 ; 
  if ( ( c >= 28672 ) && ( ( eqtb [ 12707 ] .cint >= 0 ) && ( eqtb [ 12707 ] 
  .cint < 16 ) ) ) 
  mem [ p ] .hh.b0 = eqtb [ 12707 ] .cint ; 
  else mem [ p ] .hh.b0 = ( c / 256 ) % 16 ; 
} 
void zsetmathchar ( c ) 
integer c ; 
{setmathchar_regmem 
  halfword p  ; 
  if ( c >= 32768L ) 
  {
    curcs = curchr + 1 ; 
    curcmd = eqtb [ curcs ] .hh.b0 ; 
    curchr = eqtb [ curcs ] .hh .v.RH ; 
    xtoken () ; 
    backinput () ; 
  } 
  else {
      
    p = newnoad () ; 
    mem [ p + 1 ] .hh .v.RH = 1 ; 
    mem [ p + 1 ] .hh.b1 = c % 256 ; 
    mem [ p + 1 ] .hh.b0 = ( c / 256 ) % 16 ; 
    if ( c >= 28672 ) 
    {
      if ( ( ( eqtb [ 12707 ] .cint >= 0 ) && ( eqtb [ 12707 ] .cint < 16 ) ) 
      ) 
      mem [ p + 1 ] .hh.b0 = eqtb [ 12707 ] .cint ; 
      mem [ p ] .hh.b0 = 16 ; 
    } 
    else mem [ p ] .hh.b0 = 16 + ( c / 4096 ) ; 
    mem [ curlist .tailfield ] .hh .v.RH = p ; 
    curlist .tailfield = p ; 
  } 
} 
void mathlimitswitch ( ) 
{/* 10 */ mathlimitswitch_regmem 
  if ( curlist .headfield != curlist .tailfield ) 
  if ( mem [ curlist .tailfield ] .hh.b0 == 17 ) 
  {
    mem [ curlist .tailfield ] .hh.b1 = curchr ; 
    return ; 
  } 
  {
    if ( interaction == 3 ) 
    ; 
    printnl ( 262 ) ; 
    print ( 1123 ) ; 
  } 
  {
    helpptr = 1 ; 
    helpline [ 0 ] = 1124 ; 
  } 
  error () ; 
} 
void zscandelimiter ( p , r ) 
halfword p ; 
boolean r ; 
{scandelimiter_regmem 
  if ( r ) 
  scantwentysevenbitint () ; 
  else {
      
    do {
	getxtoken () ; 
    } while ( ! ( ( curcmd != 10 ) && ( curcmd != 0 ) ) ) ; 
    switch ( curcmd ) 
    {case 11 : 
    case 12 : 
      curval = eqtb [ 12974 + curchr ] .cint ; 
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
      print ( 1125 ) ; 
    } 
    {
      helpptr = 6 ; 
      helpline [ 5 ] = 1126 ; 
      helpline [ 4 ] = 1127 ; 
      helpline [ 3 ] = 1128 ; 
      helpline [ 2 ] = 1129 ; 
      helpline [ 1 ] = 1130 ; 
      helpline [ 0 ] = 1131 ; 
    } 
    backerror () ; 
    curval = 0 ; 
  } 
  mem [ p ] .qqqq .b0 = ( curval / 1048576L ) % 16 ; 
  mem [ p ] .qqqq .b1 = ( curval / 4096 ) % 256 ; 
  mem [ p ] .qqqq .b2 = ( curval / 256 ) % 16 ; 
  mem [ p ] .qqqq .b3 = curval % 256 ; 
} 
void mathradical ( ) 
{mathradical_regmem 
  {
    mem [ curlist .tailfield ] .hh .v.RH = getnode ( 5 ) ; 
    curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
  } 
  mem [ curlist .tailfield ] .hh.b0 = 24 ; 
  mem [ curlist .tailfield ] .hh.b1 = 0 ; 
  mem [ curlist .tailfield + 1 ] .hh = emptyfield ; 
  mem [ curlist .tailfield + 3 ] .hh = emptyfield ; 
  mem [ curlist .tailfield + 2 ] .hh = emptyfield ; 
  scandelimiter ( curlist .tailfield + 4 , true ) ; 
  scanmath ( curlist .tailfield + 1 ) ; 
} 
void mathac ( ) 
{mathac_regmem 
  if ( curcmd == 45 ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 1132 ) ; 
    } 
    printesc ( 519 ) ; 
    print ( 1133 ) ; 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 1134 ; 
      helpline [ 0 ] = 1135 ; 
    } 
    error () ; 
  } 
  {
    mem [ curlist .tailfield ] .hh .v.RH = getnode ( 5 ) ; 
    curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
  } 
  mem [ curlist .tailfield ] .hh.b0 = 28 ; 
  mem [ curlist .tailfield ] .hh.b1 = 0 ; 
  mem [ curlist .tailfield + 1 ] .hh = emptyfield ; 
  mem [ curlist .tailfield + 3 ] .hh = emptyfield ; 
  mem [ curlist .tailfield + 2 ] .hh = emptyfield ; 
  mem [ curlist .tailfield + 4 ] .hh .v.RH = 1 ; 
  scanfifteenbitint () ; 
  mem [ curlist .tailfield + 4 ] .hh.b1 = curval % 256 ; 
  if ( ( curval >= 28672 ) && ( ( eqtb [ 12707 ] .cint >= 0 ) && ( eqtb [ 
  12707 ] .cint < 16 ) ) ) 
  mem [ curlist .tailfield + 4 ] .hh.b0 = eqtb [ 12707 ] .cint ; 
  else mem [ curlist .tailfield + 4 ] .hh.b0 = ( curval / 256 ) % 16 ; 
  scanmath ( curlist .tailfield + 1 ) ; 
} 
void appendchoices ( ) 
{appendchoices_regmem 
  {
    mem [ curlist .tailfield ] .hh .v.RH = newchoice () ; 
    curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
  } 
  incr ( saveptr ) ; 
  savestack [ saveptr - 1 ] .cint = 0 ; 
  pushmath ( 13 ) ; 
  scanleftbrace () ; 
} 
halfword zfinmlist ( p ) 
halfword p ; 
{register halfword Result; finmlist_regmem 
  halfword q  ; 
  if ( curlist .auxfield .cint != 0 ) 
  {
    mem [ curlist .auxfield .cint + 3 ] .hh .v.RH = 3 ; 
    mem [ curlist .auxfield .cint + 3 ] .hh .v.LH = mem [ curlist .headfield ] 
    .hh .v.RH ; 
    if ( p == 0 ) 
    q = curlist .auxfield .cint ; 
    else {
	
      q = mem [ curlist .auxfield .cint + 2 ] .hh .v.LH ; 
      if ( mem [ q ] .hh.b0 != 30 ) 
      confusion ( 870 ) ; 
      mem [ curlist .auxfield .cint + 2 ] .hh .v.LH = mem [ q ] .hh .v.RH ; 
      mem [ q ] .hh .v.RH = curlist .auxfield .cint ; 
      mem [ curlist .auxfield .cint ] .hh .v.RH = p ; 
    } 
  } 
  else {
      
    mem [ curlist .tailfield ] .hh .v.RH = p ; 
    q = mem [ curlist .headfield ] .hh .v.RH ; 
  } 
  popnest () ; 
  Result = q ; 
  return(Result) ; 
} 
void buildchoices ( ) 
{/* 10 */ buildchoices_regmem 
  halfword p  ; 
  unsave () ; 
  p = finmlist ( 0 ) ; 
  switch ( savestack [ saveptr - 1 ] .cint ) 
  {case 0 : 
    mem [ curlist .tailfield + 1 ] .hh .v.LH = p ; 
    break ; 
  case 1 : 
    mem [ curlist .tailfield + 1 ] .hh .v.RH = p ; 
    break ; 
  case 2 : 
    mem [ curlist .tailfield + 2 ] .hh .v.LH = p ; 
    break ; 
  case 3 : 
    {
      mem [ curlist .tailfield + 2 ] .hh .v.RH = p ; 
      decr ( saveptr ) ; 
      return ; 
    } 
    break ; 
  } 
  incr ( savestack [ saveptr - 1 ] .cint ) ; 
  pushmath ( 13 ) ; 
  scanleftbrace () ; 
} 
void subsup ( ) 
{subsup_regmem 
  smallnumber t  ; 
  halfword p  ; 
  t = 0 ; 
  p = 0 ; 
  if ( curlist .tailfield != curlist .headfield ) 
  if ( ( mem [ curlist .tailfield ] .hh.b0 >= 16 ) && ( mem [ curlist 
  .tailfield ] .hh.b0 < 30 ) ) 
  {
    p = curlist .tailfield + 2 + curcmd - 7 ; 
    t = mem [ p ] .hh .v.RH ; 
  } 
  if ( ( p == 0 ) || ( t != 0 ) ) 
  {
    {
      mem [ curlist .tailfield ] .hh .v.RH = newnoad () ; 
      curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
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
	  print ( 1136 ) ; 
	} 
	{
	  helpptr = 1 ; 
	  helpline [ 0 ] = 1137 ; 
	} 
      } 
      else {
	  
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 1138 ) ; 
	} 
	{
	  helpptr = 1 ; 
	  helpline [ 0 ] = 1139 ; 
	} 
      } 
      error () ; 
    } 
  } 
  scanmath ( p ) ; 
} 
void mathfraction ( ) 
{mathfraction_regmem 
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
      print ( 1146 ) ; 
    } 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 1147 ; 
      helpline [ 1 ] = 1148 ; 
      helpline [ 0 ] = 1149 ; 
    } 
    error () ; 
  } 
  else {
      
    curlist .auxfield .cint = getnode ( 6 ) ; 
    mem [ curlist .auxfield .cint ] .hh.b0 = 25 ; 
    mem [ curlist .auxfield .cint ] .hh.b1 = 0 ; 
    mem [ curlist .auxfield .cint + 2 ] .hh .v.RH = 3 ; 
    mem [ curlist .auxfield .cint + 2 ] .hh .v.LH = mem [ curlist .headfield ] 
    .hh .v.RH ; 
    mem [ curlist .auxfield .cint + 3 ] .hh = emptyfield ; 
    mem [ curlist .auxfield .cint + 4 ] .qqqq = nulldelimiter ; 
    mem [ curlist .auxfield .cint + 5 ] .qqqq = nulldelimiter ; 
    mem [ curlist .headfield ] .hh .v.RH = 0 ; 
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
	mem [ curlist .auxfield .cint + 1 ] .cint = curval ; 
      } 
      break ; 
    case 1 : 
      mem [ curlist .auxfield .cint + 1 ] .cint = 1073741824L ; 
      break ; 
    case 2 : 
      mem [ curlist .auxfield .cint + 1 ] .cint = 0 ; 
      break ; 
    } 
  } 
} 
void mathleftright ( ) 
{mathleftright_regmem 
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
	print ( 772 ) ; 
      } 
      printesc ( 870 ) ; 
      {
	helpptr = 1 ; 
	helpline [ 0 ] = 1150 ; 
      } 
      error () ; 
    } 
    else offsave () ; 
  } 
  else {
      
    p = newnoad () ; 
    mem [ p ] .hh.b0 = t ; 
    scandelimiter ( p + 1 , false ) ; 
    if ( t == 30 ) 
    {
      pushmath ( 16 ) ; 
      mem [ curlist .headfield ] .hh .v.RH = p ; 
      curlist .tailfield = p ; 
    } 
    else {
	
      p = finmlist ( p ) ; 
      unsave () ; 
      {
	mem [ curlist .tailfield ] .hh .v.RH = newnoad () ; 
	curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
      } 
      mem [ curlist .tailfield ] .hh.b0 = 23 ; 
      mem [ curlist .tailfield + 1 ] .hh .v.RH = 3 ; 
      mem [ curlist .tailfield + 1 ] .hh .v.LH = p ; 
    } 
  } 
} 
void aftermath ( ) 
{aftermath_regmem 
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
  if ( ( fontparams [ eqtb [ 11337 ] .hh .v.RH ] < 22 ) || ( fontparams [ eqtb 
  [ 11353 ] .hh .v.RH ] < 22 ) || ( fontparams [ eqtb [ 11369 ] .hh .v.RH ] < 
  22 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 1151 ) ; 
    } 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 1152 ; 
      helpline [ 1 ] = 1153 ; 
      helpline [ 0 ] = 1154 ; 
    } 
    error () ; 
    flushmath () ; 
    danger = true ; 
  } 
  else if ( ( fontparams [ eqtb [ 11338 ] .hh .v.RH ] < 13 ) || ( fontparams [ 
  eqtb [ 11354 ] .hh .v.RH ] < 13 ) || ( fontparams [ eqtb [ 11370 ] .hh .v.RH 
  ] < 13 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 1155 ) ; 
    } 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 1156 ; 
      helpline [ 1 ] = 1157 ; 
      helpline [ 0 ] = 1158 ; 
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
	  print ( 1159 ) ; 
	} 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 1160 ; 
	  helpline [ 0 ] = 1161 ; 
	} 
	backerror () ; 
      } 
    } 
    curmlist = p ; 
    curstyle = 2 ; 
    mlistpenalties = false ; 
    mlisttohlist () ; 
    a = hpack ( mem [ memtop - 3 ] .hh .v.RH , 0 , 1 ) ; 
    unsave () ; 
    decr ( saveptr ) ; 
    if ( savestack [ saveptr + 0 ] .cint == 1 ) 
    l = true ; 
    danger = false ; 
    if ( ( fontparams [ eqtb [ 11337 ] .hh .v.RH ] < 22 ) || ( fontparams [ 
    eqtb [ 11353 ] .hh .v.RH ] < 22 ) || ( fontparams [ eqtb [ 11369 ] .hh 
    .v.RH ] < 22 ) ) 
    {
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 262 ) ; 
	print ( 1151 ) ; 
      } 
      {
	helpptr = 3 ; 
	helpline [ 2 ] = 1152 ; 
	helpline [ 1 ] = 1153 ; 
	helpline [ 0 ] = 1154 ; 
      } 
      error () ; 
      flushmath () ; 
      danger = true ; 
    } 
    else if ( ( fontparams [ eqtb [ 11338 ] .hh .v.RH ] < 13 ) || ( fontparams 
    [ eqtb [ 11354 ] .hh .v.RH ] < 13 ) || ( fontparams [ eqtb [ 11370 ] .hh 
    .v.RH ] < 13 ) ) 
    {
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 262 ) ; 
	print ( 1155 ) ; 
      } 
      {
	helpptr = 3 ; 
	helpline [ 2 ] = 1156 ; 
	helpline [ 1 ] = 1157 ; 
	helpline [ 0 ] = 1158 ; 
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
      mem [ curlist .tailfield ] .hh .v.RH = newmath ( eqtb [ 13231 ] .cint , 
      0 ) ; 
      curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
    } 
    curmlist = p ; 
    curstyle = 2 ; 
    mlistpenalties = ( curlist .modefield > 0 ) ; 
    mlisttohlist () ; 
    mem [ curlist .tailfield ] .hh .v.RH = mem [ memtop - 3 ] .hh .v.RH ; 
    while ( mem [ curlist .tailfield ] .hh .v.RH != 0 ) curlist .tailfield = 
    mem [ curlist .tailfield ] .hh .v.RH ; 
    {
      mem [ curlist .tailfield ] .hh .v.RH = newmath ( eqtb [ 13231 ] .cint , 
      1 ) ; 
      curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
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
	  print ( 1159 ) ; 
	} 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 1160 ; 
	  helpline [ 0 ] = 1161 ; 
	} 
	backerror () ; 
      } 
    } 
    curmlist = p ; 
    curstyle = 0 ; 
    mlistpenalties = false ; 
    mlisttohlist () ; 
    p = mem [ memtop - 3 ] .hh .v.RH ; 
    adjusttail = memtop - 5 ; 
    b = hpack ( p , 0 , 1 ) ; 
    p = mem [ b + 5 ] .hh .v.RH ; 
    t = adjusttail ; 
    adjusttail = 0 ; 
    w = mem [ b + 1 ] .cint ; 
    z = eqtb [ 13244 ] .cint ; 
    s = eqtb [ 13245 ] .cint ; 
    if ( ( a == 0 ) || danger ) 
    {
      e = 0 ; 
      q = 0 ; 
    } 
    else {
	
      e = mem [ a + 1 ] .cint ; 
      q = e + fontinfo [ 6 + parambase [ eqtb [ 11337 ] .hh .v.RH ] ] .cint ; 
    } 
    if ( w + q > z ) 
    {
      if ( ( e != 0 ) && ( ( w - totalshrink [ 0 ] + q <= z ) || ( totalshrink 
      [ 1 ] != 0 ) || ( totalshrink [ 2 ] != 0 ) || ( totalshrink [ 3 ] != 0 ) 
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
      w = mem [ b + 1 ] .cint ; 
    } 
    d = half ( z - w ) ; 
    if ( ( e > 0 ) && ( d < 2 * e ) ) 
    {
      d = half ( z - w - e ) ; 
      if ( p != 0 ) 
      if ( ! ( p >= himemmin ) ) 
      if ( mem [ p ] .hh.b0 == 10 ) 
      d = 0 ; 
    } 
    {
      mem [ curlist .tailfield ] .hh .v.RH = newpenalty ( eqtb [ 12674 ] .cint 
      ) ; 
      curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
    } 
    if ( ( d + s <= eqtb [ 13243 ] .cint ) || l ) 
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
      mem [ a + 4 ] .cint = s ; 
      appendtovlist ( a ) ; 
      {
	mem [ curlist .tailfield ] .hh .v.RH = newpenalty ( 10000 ) ; 
	curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
      } 
    } 
    else {
	
      mem [ curlist .tailfield ] .hh .v.RH = newparamglue ( g1 ) ; 
      curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
    } 
    if ( e != 0 ) 
    {
      r = newkern ( z - w - e - d ) ; 
      if ( l ) 
      {
	mem [ a ] .hh .v.RH = r ; 
	mem [ r ] .hh .v.RH = b ; 
	b = a ; 
	d = 0 ; 
      } 
      else {
	  
	mem [ b ] .hh .v.RH = r ; 
	mem [ r ] .hh .v.RH = a ; 
      } 
      b = hpack ( b , 0 , 1 ) ; 
    } 
    mem [ b + 4 ] .cint = s + d ; 
    appendtovlist ( b ) ; 
    if ( ( a != 0 ) && ( e == 0 ) && ! l ) 
    {
      {
	mem [ curlist .tailfield ] .hh .v.RH = newpenalty ( 10000 ) ; 
	curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
      } 
      mem [ a + 4 ] .cint = s + z - mem [ a + 1 ] .cint ; 
      appendtovlist ( a ) ; 
      g2 = 0 ; 
    } 
    if ( t != memtop - 5 ) 
    {
      mem [ curlist .tailfield ] .hh .v.RH = mem [ memtop - 5 ] .hh .v.RH ; 
      curlist .tailfield = t ; 
    } 
    {
      mem [ curlist .tailfield ] .hh .v.RH = newpenalty ( eqtb [ 12675 ] .cint 
      ) ; 
      curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
    } 
    if ( g2 > 0 ) 
    {
      mem [ curlist .tailfield ] .hh .v.RH = newparamglue ( g2 ) ; 
      curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
    } 
    resumeafterdisplay () ; 
  } 
} 
void resumeafterdisplay ( ) 
{resumeafterdisplay_regmem 
  if ( curgroup != 15 ) 
  confusion ( 1162 ) ; 
  unsave () ; 
  curlist .pgfield = curlist .pgfield + 3 ; 
  pushnest () ; 
  curlist .modefield = 102 ; 
  curlist .auxfield .hh .v.LH = 1000 ; 
  curlist .auxfield .hh .v.RH = 0 ; 
  {
    getxtoken () ; 
    if ( curcmd != 10 ) 
    backinput () ; 
  } 
  if ( nestptr == 1 ) 
  buildpage () ; 
} 
void getrtoken ( ) 
{/* 20 */ getrtoken_regmem 
  lab20: do {
      gettoken () ; 
  } while ( ! ( curtok != 2592 ) ) ; 
  if ( ( curcs == 0 ) || ( curcs > 10014 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 1177 ) ; 
    } 
    {
      helpptr = 5 ; 
      helpline [ 4 ] = 1178 ; 
      helpline [ 3 ] = 1179 ; 
      helpline [ 2 ] = 1180 ; 
      helpline [ 1 ] = 1181 ; 
      helpline [ 0 ] = 1182 ; 
    } 
    if ( curcs == 0 ) 
    backinput () ; 
    curtok = 14109 ; 
    inserror () ; 
    goto lab20 ; 
  } 
} 
void trapzeroglue ( ) 
{trapzeroglue_regmem 
  if ( ( mem [ curval + 1 ] .cint == 0 ) && ( mem [ curval + 2 ] .cint == 0 ) 
  && ( mem [ curval + 3 ] .cint == 0 ) ) 
  {
    incr ( mem [ 0 ] .hh .v.RH ) ; 
    deleteglueref ( curval ) ; 
    curval = 0 ; 
  } 
} 
void zdoregistercommand ( a ) 
smallnumber a ; 
{/* 40 10 */ doregistercommand_regmem 
  halfword l, q, r, s  ; 
  schar p  ; 
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
	  print ( 681 ) ; 
	} 
	printcmdchr ( curcmd , curchr ) ; 
	print ( 682 ) ; 
	printcmdchr ( q , 0 ) ; 
	{
	  helpptr = 1 ; 
	  helpline [ 0 ] = 1203 ; 
	} 
	error () ; 
	return ; 
      } 
    } 
    p = curchr ; 
    scaneightbitint () ; 
    switch ( p ) 
    {case 0 : 
      l = curval + 12718 ; 
      break ; 
    case 1 : 
      l = curval + 13251 ; 
      break ; 
    case 2 : 
      l = curval + 10300 ; 
      break ; 
    case 3 : 
      l = curval + 10556 ; 
      break ; 
    } 
  } 
  lab40: ; 
  if ( q == 89 ) 
  scanoptionalequals () ; 
  else if ( scankeyword ( 1199 ) ) 
  ; 
  aritherror = false ; 
  if ( q < 91 ) 
  if ( p < 2 ) 
  {
    if ( p == 0 ) 
    scanint () ; 
    else scandimen ( false , false , false ) ; 
    if ( q == 90 ) 
    curval = curval + eqtb [ l ] .cint ; 
  } 
  else {
      
    scanglue ( p ) ; 
    if ( q == 90 ) 
    {
      q = newspec ( curval ) ; 
      r = eqtb [ l ] .hh .v.RH ; 
      deleteglueref ( curval ) ; 
      mem [ q + 1 ] .cint = mem [ q + 1 ] .cint + mem [ r + 1 ] .cint ; 
      if ( mem [ q + 2 ] .cint == 0 ) 
      mem [ q ] .hh.b0 = 0 ; 
      if ( mem [ q ] .hh.b0 == mem [ r ] .hh.b0 ) 
      mem [ q + 2 ] .cint = mem [ q + 2 ] .cint + mem [ r + 2 ] .cint ; 
      else if ( ( mem [ q ] .hh.b0 < mem [ r ] .hh.b0 ) && ( mem [ r + 2 ] 
      .cint != 0 ) ) 
      {
	mem [ q + 2 ] .cint = mem [ r + 2 ] .cint ; 
	mem [ q ] .hh.b0 = mem [ r ] .hh.b0 ; 
      } 
      if ( mem [ q + 3 ] .cint == 0 ) 
      mem [ q ] .hh.b1 = 0 ; 
      if ( mem [ q ] .hh.b1 == mem [ r ] .hh.b1 ) 
      mem [ q + 3 ] .cint = mem [ q + 3 ] .cint + mem [ r + 3 ] .cint ; 
      else if ( ( mem [ q ] .hh.b1 < mem [ r ] .hh.b1 ) && ( mem [ r + 3 ] 
      .cint != 0 ) ) 
      {
	mem [ q + 3 ] .cint = mem [ r + 3 ] .cint ; 
	mem [ q ] .hh.b1 = mem [ r ] .hh.b1 ; 
      } 
      curval = q ; 
    } 
  } 
  else {
      
    scanint () ; 
    if ( p < 2 ) 
    if ( q == 91 ) 
    if ( p == 0 ) 
    curval = multandadd ( eqtb [ l ] .cint , curval , 0 , 2147483647L ) ; 
    else curval = multandadd ( eqtb [ l ] .cint , curval , 0 , 1073741823L ) ; 
    else curval = xovern ( eqtb [ l ] .cint , curval ) ; 
    else {
	
      s = eqtb [ l ] .hh .v.RH ; 
      r = newspec ( s ) ; 
      if ( q == 91 ) 
      {
	mem [ r + 1 ] .cint = multandadd ( mem [ s + 1 ] .cint , curval , 0 , 
	1073741823L ) ; 
	mem [ r + 2 ] .cint = multandadd ( mem [ s + 2 ] .cint , curval , 0 , 
	1073741823L ) ; 
	mem [ r + 3 ] .cint = multandadd ( mem [ s + 3 ] .cint , curval , 0 , 
	1073741823L ) ; 
      } 
      else {
	  
	mem [ r + 1 ] .cint = xovern ( mem [ s + 1 ] .cint , curval ) ; 
	mem [ r + 2 ] .cint = xovern ( mem [ s + 2 ] .cint , curval ) ; 
	mem [ r + 3 ] .cint = xovern ( mem [ s + 3 ] .cint , curval ) ; 
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
      print ( 1200 ) ; 
    } 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 1201 ; 
      helpline [ 0 ] = 1202 ; 
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
