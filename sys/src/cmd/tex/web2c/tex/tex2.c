#define EXTERN extern
#include "texd.h"

void 
#ifdef HAVE_PROTOTYPES
issuemessage ( void ) 
#else
issuemessage ( ) 
#endif
{
  issuemessage_regmem 
  char oldsetting  ;
  char c  ;
  strnumber s  ;
  c = curchr ;
  mem [memtop - 12 ].hh .v.RH = scantoks ( false , true ) ;
  oldsetting = selector ;
  selector = 21 ;
  tokenshow ( defref ) ;
  selector = oldsetting ;
  flushlist ( defref ) ;
  {
    if ( poolptr + 1 > poolsize ) 
    overflow ( 257 , poolsize - initpoolptr ) ;
  } 
  s = makestring () ;
  if ( c == 0 ) 
  {
    if ( termoffset + ( strstart [s + 1 ]- strstart [s ]) > maxprintline - 
    2 ) 
    println () ;
    else if ( ( termoffset > 0 ) || ( fileoffset > 0 ) ) 
    printchar ( 32 ) ;
    print ( s ) ;
    fflush ( stdout ) ;
  } 
  else {
      
    {
      if ( interaction == 3 ) 
      ;
      printnl ( 262 ) ;
      print ( 335 ) ;
    } 
    print ( s ) ;
    if ( eqtb [13065 ].hh .v.RH != 0 ) 
    useerrhelp = true ;
    else if ( longhelpseen ) 
    {
      helpptr = 1 ;
      helpline [0 ]= 1230 ;
    } 
    else {
	
      if ( interaction < 3 ) 
      longhelpseen = true ;
      {
	helpptr = 4 ;
	helpline [3 ]= 1231 ;
	helpline [2 ]= 1232 ;
	helpline [1 ]= 1233 ;
	helpline [0 ]= 1234 ;
      } 
    } 
    error () ;
    useerrhelp = false ;
  } 
  {
    decr ( strptr ) ;
    poolptr = strstart [strptr ];
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
shiftcase ( void ) 
#else
shiftcase ( ) 
#endif
{
  shiftcase_regmem 
  halfword b  ;
  halfword p  ;
  halfword t  ;
  eightbits c  ;
  b = curchr ;
  p = scantoks ( false , false ) ;
  p = mem [defref ].hh .v.RH ;
  while ( p != 0 ) {
      
    t = mem [p ].hh .v.LH ;
    if ( t < 4352 ) 
    {
      c = t % 256 ;
      if ( eqtb [b + c ].hh .v.RH != 0 ) 
      mem [p ].hh .v.LH = t - c + eqtb [b + c ].hh .v.RH ;
    } 
    p = mem [p ].hh .v.RH ;
  } 
  begintokenlist ( mem [defref ].hh .v.RH , 3 ) ;
  {
    mem [defref ].hh .v.RH = avail ;
    avail = defref ;
	;
#ifdef STAT
    decr ( dynused ) ;
#endif /* STAT */
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
showwhatever ( void ) 
#else
showwhatever ( ) 
#endif
{
  /* 50 */ showwhatever_regmem 
  halfword p  ;
  switch ( curchr ) 
  {case 3 : 
    {
      begindiagnostic () ;
      showactivities () ;
    } 
    break ;
  case 1 : 
    {
      scaneightbitint () ;
      begindiagnostic () ;
      printnl ( 1252 ) ;
      printint ( curval ) ;
      printchar ( 61 ) ;
      if ( eqtb [13322 + curval ].hh .v.RH == 0 ) 
      print ( 407 ) ;
      else showbox ( eqtb [13322 + curval ].hh .v.RH ) ;
    } 
    break ;
  case 0 : 
    {
      gettoken () ;
      if ( interaction == 3 ) 
      ;
      printnl ( 1246 ) ;
      if ( curcs != 0 ) 
      {
	sprintcs ( curcs ) ;
	printchar ( 61 ) ;
      } 
      printmeaning () ;
      goto lab50 ;
    } 
    break ;
    default: 
    {
      p = thetoks () ;
      if ( interaction == 3 ) 
      ;
      printnl ( 1246 ) ;
      tokenshow ( memtop - 3 ) ;
      flushlist ( mem [memtop - 3 ].hh .v.RH ) ;
      goto lab50 ;
    } 
    break ;
  } 
  enddiagnostic ( true ) ;
  {
    if ( interaction == 3 ) 
    ;
    printnl ( 262 ) ;
    print ( 1253 ) ;
  } 
  if ( selector == 19 ) 
  if ( eqtb [15192 ].cint <= 0 ) 
  {
    selector = 17 ;
    print ( 1254 ) ;
    selector = 19 ;
  } 
  lab50: if ( interaction < 3 ) 
  {
    helpptr = 0 ;
    decr ( errorcount ) ;
  } 
  else if ( eqtb [15192 ].cint > 0 ) 
  {
    {
      helpptr = 3 ;
      helpline [2 ]= 1241 ;
      helpline [1 ]= 1242 ;
      helpline [0 ]= 1243 ;
    } 
  } 
  else {
      
    {
      helpptr = 5 ;
      helpline [4 ]= 1241 ;
      helpline [3 ]= 1242 ;
      helpline [2 ]= 1243 ;
      helpline [1 ]= 1244 ;
      helpline [0 ]= 1245 ;
    } 
  } 
  error () ;
} 
void 
#ifdef HAVE_PROTOTYPES
znewwhatsit ( smallnumber s , smallnumber w ) 
#else
znewwhatsit ( s , w ) 
  smallnumber s ;
  smallnumber w ;
#endif
{
  newwhatsit_regmem 
  halfword p  ;
  p = getnode ( w ) ;
  mem [p ].hh.b0 = 8 ;
  mem [p ].hh.b1 = s ;
  mem [curlist .tailfield ].hh .v.RH = p ;
  curlist .tailfield = p ;
} 
void 
#ifdef HAVE_PROTOTYPES
znewwritewhatsit ( smallnumber w ) 
#else
znewwritewhatsit ( w ) 
  smallnumber w ;
#endif
{
  newwritewhatsit_regmem 
  newwhatsit ( curchr , w ) ;
  if ( w != 2 ) 
  scanfourbitint () ;
  else {
      
    scanint () ;
    if ( curval < 0 ) 
    curval = 17 ;
    else if ( ( curval > 15 ) && ( curval != 18 ) ) 
    curval = 16 ;
  } 
  mem [curlist .tailfield + 1 ].hh .v.LH = curval ;
} 
void 
#ifdef HAVE_PROTOTYPES
doextension ( void ) 
#else
doextension ( ) 
#endif
{
  doextension_regmem 
  integer i, j, k  ;
  halfword p, q, r  ;
  switch ( curchr ) 
  {case 0 : 
    {
      newwritewhatsit ( 3 ) ;
      scanoptionalequals () ;
      scanfilename () ;
      mem [curlist .tailfield + 1 ].hh .v.RH = curname ;
      mem [curlist .tailfield + 2 ].hh .v.LH = curarea ;
      mem [curlist .tailfield + 2 ].hh .v.RH = curext ;
    } 
    break ;
  case 1 : 
    {
      k = curcs ;
      newwritewhatsit ( 2 ) ;
      curcs = k ;
      p = scantoks ( false , false ) ;
      mem [curlist .tailfield + 1 ].hh .v.RH = defref ;
    } 
    break ;
  case 2 : 
    {
      newwritewhatsit ( 2 ) ;
      mem [curlist .tailfield + 1 ].hh .v.RH = 0 ;
    } 
    break ;
  case 3 : 
    {
      newwhatsit ( 3 , 2 ) ;
      mem [curlist .tailfield + 1 ].hh .v.LH = 0 ;
      p = scantoks ( false , true ) ;
      mem [curlist .tailfield + 1 ].hh .v.RH = defref ;
    } 
    break ;
  case 4 : 
    {
      getxtoken () ;
      if ( ( curcmd == 59 ) && ( curchr <= 2 ) ) 
      {
	p = curlist .tailfield ;
	doextension () ;
	outwhat ( curlist .tailfield ) ;
	flushnodelist ( curlist .tailfield ) ;
	curlist .tailfield = p ;
	mem [p ].hh .v.RH = 0 ;
      } 
      else backinput () ;
    } 
    break ;
  case 5 : 
    if ( abs ( curlist .modefield ) != 102 ) 
    reportillegalcase () ;
    else {
	
      newwhatsit ( 4 , 2 ) ;
      scanint () ;
      if ( curval <= 0 ) 
      curlist .auxfield .hh .v.RH = 0 ;
      else if ( curval > 255 ) 
      curlist .auxfield .hh .v.RH = 0 ;
      else curlist .auxfield .hh .v.RH = curval ;
      mem [curlist .tailfield + 1 ].hh .v.RH = curlist .auxfield .hh .v.RH ;
      mem [curlist .tailfield + 1 ].hh.b0 = normmin ( eqtb [15214 ].cint ) 
      ;
      mem [curlist .tailfield + 1 ].hh.b1 = normmin ( eqtb [15215 ].cint ) 
      ;
    } 
    break ;
    default: 
    confusion ( 1289 ) ;
    break ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
fixlanguage ( void ) 
#else
fixlanguage ( ) 
#endif
{
  fixlanguage_regmem 
  ASCIIcode l  ;
  if ( eqtb [15213 ].cint <= 0 ) 
  l = 0 ;
  else if ( eqtb [15213 ].cint > 255 ) 
  l = 0 ;
  else l = eqtb [15213 ].cint ;
  if ( l != curlist .auxfield .hh .v.RH ) 
  {
    newwhatsit ( 4 , 2 ) ;
    mem [curlist .tailfield + 1 ].hh .v.RH = l ;
    curlist .auxfield .hh .v.RH = l ;
    mem [curlist .tailfield + 1 ].hh.b0 = normmin ( eqtb [15214 ].cint ) ;
    mem [curlist .tailfield + 1 ].hh.b1 = normmin ( eqtb [15215 ].cint ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
handlerightbrace ( void ) 
#else
handlerightbrace ( ) 
#endif
{
  handlerightbrace_regmem 
  halfword p, q  ;
  scaled d  ;
  integer f  ;
  switch ( curgroup ) 
  {case 1 : 
    unsave () ;
    break ;
  case 0 : 
    {
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 1039 ) ;
      } 
      {
	helpptr = 2 ;
	helpline [1 ]= 1040 ;
	helpline [0 ]= 1041 ;
      } 
      error () ;
    } 
    break ;
  case 14 : 
  case 15 : 
  case 16 : 
    extrarightbrace () ;
    break ;
  case 2 : 
    package ( 0 ) ;
    break ;
  case 3 : 
    {
      adjusttail = memtop - 5 ;
      package ( 0 ) ;
    } 
    break ;
  case 4 : 
    {
      endgraf () ;
      package ( 0 ) ;
    } 
    break ;
  case 5 : 
    {
      endgraf () ;
      package ( 4 ) ;
    } 
    break ;
  case 11 : 
    {
      endgraf () ;
      q = eqtb [12536 ].hh .v.RH ;
      incr ( mem [q ].hh .v.RH ) ;
      d = eqtb [15739 ].cint ;
      f = eqtb [15205 ].cint ;
      unsave () ;
      decr ( saveptr ) ;
      p = vpackage ( mem [curlist .headfield ].hh .v.RH , 0 , 1 , 
      1073741823L ) ;
      popnest () ;
      if ( savestack [saveptr + 0 ].cint < 255 ) 
      {
	{
	  mem [curlist .tailfield ].hh .v.RH = getnode ( 5 ) ;
	  curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
	} 
	mem [curlist .tailfield ].hh.b0 = 3 ;
	mem [curlist .tailfield ].hh.b1 = savestack [saveptr + 0 ].cint ;
	mem [curlist .tailfield + 3 ].cint = mem [p + 3 ].cint + mem [p + 
	2 ].cint ;
	mem [curlist .tailfield + 4 ].hh .v.LH = mem [p + 5 ].hh .v.RH ;
	mem [curlist .tailfield + 4 ].hh .v.RH = q ;
	mem [curlist .tailfield + 2 ].cint = d ;
	mem [curlist .tailfield + 1 ].cint = f ;
      } 
      else {
	  
	{
	  mem [curlist .tailfield ].hh .v.RH = getnode ( 2 ) ;
	  curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
	} 
	mem [curlist .tailfield ].hh.b0 = 5 ;
	mem [curlist .tailfield ].hh.b1 = 0 ;
	mem [curlist .tailfield + 1 ].cint = mem [p + 5 ].hh .v.RH ;
	deleteglueref ( q ) ;
      } 
      freenode ( p , 7 ) ;
      if ( nestptr == 0 ) 
      buildpage () ;
    } 
    break ;
  case 8 : 
    {
      if ( ( curinput .locfield != 0 ) || ( ( curinput .indexfield != 6 ) && ( 
      curinput .indexfield != 3 ) ) ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 1005 ) ;
	} 
	{
	  helpptr = 2 ;
	  helpline [1 ]= 1006 ;
	  helpline [0 ]= 1007 ;
	} 
	error () ;
	do {
	    gettoken () ;
	} while ( ! ( curinput .locfield == 0 ) ) ;
      } 
      endtokenlist () ;
      endgraf () ;
      unsave () ;
      outputactive = false ;
      insertpenalties = 0 ;
      if ( eqtb [13577 ].hh .v.RH != 0 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ;
	  printnl ( 262 ) ;
	  print ( 1008 ) ;
	} 
	printesc ( 406 ) ;
	printint ( 255 ) ;
	{
	  helpptr = 3 ;
	  helpline [2 ]= 1009 ;
	  helpline [1 ]= 1010 ;
	  helpline [0 ]= 1011 ;
	} 
	boxerror ( 255 ) ;
      } 
      if ( curlist .tailfield != curlist .headfield ) 
      {
	mem [pagetail ].hh .v.RH = mem [curlist .headfield ].hh .v.RH ;
	pagetail = curlist .tailfield ;
      } 
      if ( mem [memtop - 2 ].hh .v.RH != 0 ) 
      {
	if ( mem [memtop - 1 ].hh .v.RH == 0 ) 
	nest [0 ].tailfield = pagetail ;
	mem [pagetail ].hh .v.RH = mem [memtop - 1 ].hh .v.RH ;
	mem [memtop - 1 ].hh .v.RH = mem [memtop - 2 ].hh .v.RH ;
	mem [memtop - 2 ].hh .v.RH = 0 ;
	pagetail = memtop - 2 ;
      } 
      popnest () ;
      buildpage () ;
    } 
    break ;
  case 10 : 
    builddiscretionary () ;
    break ;
  case 6 : 
    {
      backinput () ;
      curtok = 14610 ;
      {
	if ( interaction == 3 ) 
	;
	printnl ( 262 ) ;
	print ( 624 ) ;
      } 
      printesc ( 894 ) ;
      print ( 625 ) ;
      {
	helpptr = 1 ;
	helpline [0 ]= 1120 ;
      } 
      inserror () ;
    } 
    break ;
  case 7 : 
    {
      endgraf () ;
      unsave () ;
      alignpeek () ;
    } 
    break ;
  case 12 : 
    {
      endgraf () ;
      unsave () ;
      saveptr = saveptr - 2 ;
      p = vpackage ( mem [curlist .headfield ].hh .v.RH , savestack [
      saveptr + 1 ].cint , savestack [saveptr + 0 ].cint , 1073741823L ) ;
      popnest () ;
      {
	mem [curlist .tailfield ].hh .v.RH = newnoad () ;
	curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
      } 
      mem [curlist .tailfield ].hh.b0 = 29 ;
      mem [curlist .tailfield + 1 ].hh .v.RH = 2 ;
      mem [curlist .tailfield + 1 ].hh .v.LH = p ;
    } 
    break ;
  case 13 : 
    buildchoices () ;
    break ;
  case 9 : 
    {
      unsave () ;
      decr ( saveptr ) ;
      mem [savestack [saveptr + 0 ].cint ].hh .v.RH = 3 ;
      p = finmlist ( 0 ) ;
      mem [savestack [saveptr + 0 ].cint ].hh .v.LH = p ;
      if ( p != 0 ) 
      if ( mem [p ].hh .v.RH == 0 ) 
      if ( mem [p ].hh.b0 == 16 ) 
      {
	if ( mem [p + 3 ].hh .v.RH == 0 ) 
	if ( mem [p + 2 ].hh .v.RH == 0 ) 
	{
	  mem [savestack [saveptr + 0 ].cint ].hh = mem [p + 1 ].hh ;
	  freenode ( p , 4 ) ;
	} 
      } 
      else if ( mem [p ].hh.b0 == 28 ) 
      if ( savestack [saveptr + 0 ].cint == curlist .tailfield + 1 ) 
      if ( mem [curlist .tailfield ].hh.b0 == 16 ) 
      {
	q = curlist .headfield ;
	while ( mem [q ].hh .v.RH != curlist .tailfield ) q = mem [q ].hh 
	.v.RH ;
	mem [q ].hh .v.RH = p ;
	freenode ( curlist .tailfield , 4 ) ;
	curlist .tailfield = p ;
      } 
    } 
    break ;
    default: 
    confusion ( 1042 ) ;
    break ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
maincontrol ( void ) 
#else
maincontrol ( ) 
#endif
{
  /* 60 21 70 80 90 91 92 95 100 101 110 111 112 120 10 */ 
  maincontrol_regmem 
  integer t  ;
  if ( eqtb [13063 ].hh .v.RH != 0 ) 
  begintokenlist ( eqtb [13063 ].hh .v.RH , 12 ) ;
  lab60: getxtoken () ;
  lab21: if ( interrupt != 0 ) 
  if ( OKtointerrupt ) 
  {
    backinput () ;
    {
      if ( interrupt != 0 ) 
      pauseforinstructions () ;
    } 
    goto lab60 ;
  } 
	;
#ifdef TEXMF_DEBUG
  if ( panicking ) 
  checkmem ( false ) ;
#endif /* TEXMF_DEBUG */
  if ( eqtb [15199 ].cint > 0 ) 
  showcurcmdchr () ;
  switch ( abs ( curlist .modefield ) + curcmd ) 
  {case 113 : 
  case 114 : 
  case 170 : 
    goto lab70 ;
    break ;
  case 118 : 
    {
      scancharnum () ;
      curchr = curval ;
      goto lab70 ;
    } 
    break ;
  case 167 : 
    {
      getxtoken () ;
      if ( ( curcmd == 11 ) || ( curcmd == 12 ) || ( curcmd == 68 ) || ( 
      curcmd == 16 ) ) 
      cancelboundary = true ;
      goto lab21 ;
    } 
    break ;
  case 112 : 
    if ( curlist .auxfield .hh .v.LH == 1000 ) 
    goto lab120 ;
    else appspace () ;
    break ;
  case 166 : 
  case 267 : 
    goto lab120 ;
    break ;
  case 1 : 
  case 102 : 
  case 203 : 
  case 11 : 
  case 213 : 
  case 268 : 
    ;
    break ;
  case 40 : 
  case 141 : 
  case 242 : 
    {
      do {
	  getxtoken () ;
      } while ( ! ( curcmd != 10 ) ) ;
      goto lab21 ;
    } 
    break ;
  case 15 : 
    if ( itsallover () ) 
    return ;
    break ;
  case 23 : 
  case 123 : 
  case 224 : 
  case 71 : 
  case 172 : 
  case 273 : 
  case 39 : 
  case 45 : 
  case 49 : 
  case 150 : 
  case 7 : 
  case 108 : 
  case 209 : 
    reportillegalcase () ;
    break ;
  case 8 : 
  case 109 : 
  case 9 : 
  case 110 : 
  case 18 : 
  case 119 : 
  case 70 : 
  case 171 : 
  case 51 : 
  case 152 : 
  case 16 : 
  case 117 : 
  case 50 : 
  case 151 : 
  case 53 : 
  case 154 : 
  case 67 : 
  case 168 : 
  case 54 : 
  case 155 : 
  case 55 : 
  case 156 : 
  case 57 : 
  case 158 : 
  case 56 : 
  case 157 : 
  case 31 : 
  case 132 : 
  case 52 : 
  case 153 : 
  case 29 : 
  case 130 : 
  case 47 : 
  case 148 : 
  case 212 : 
  case 216 : 
  case 217 : 
  case 230 : 
  case 227 : 
  case 236 : 
  case 239 : 
    insertdollarsign () ;
    break ;
  case 37 : 
  case 137 : 
  case 238 : 
    {
      {
	mem [curlist .tailfield ].hh .v.RH = scanrulespec () ;
	curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
      } 
      if ( abs ( curlist .modefield ) == 1 ) 
      curlist .auxfield .cint = -65536000L ;
      else if ( abs ( curlist .modefield ) == 102 ) 
      curlist .auxfield .hh .v.LH = 1000 ;
    } 
    break ;
  case 28 : 
  case 128 : 
  case 229 : 
  case 231 : 
    appendglue () ;
    break ;
  case 30 : 
  case 131 : 
  case 232 : 
  case 233 : 
    appendkern () ;
    break ;
  case 2 : 
  case 103 : 
    newsavelevel ( 1 ) ;
    break ;
  case 62 : 
  case 163 : 
  case 264 : 
    newsavelevel ( 14 ) ;
    break ;
  case 63 : 
  case 164 : 
  case 265 : 
    if ( curgroup == 14 ) 
    unsave () ;
    else offsave () ;
    break ;
  case 3 : 
  case 104 : 
  case 205 : 
    handlerightbrace () ;
    break ;
  case 22 : 
  case 124 : 
  case 225 : 
    {
      t = curchr ;
      scandimen ( false , false , false ) ;
      if ( t == 0 ) 
      scanbox ( curval ) ;
      else scanbox ( - (integer) curval ) ;
    } 
    break ;
  case 32 : 
  case 133 : 
  case 234 : 
    scanbox ( 1073742237L + curchr ) ;
    break ;
  case 21 : 
  case 122 : 
  case 223 : 
    beginbox ( 0 ) ;
    break ;
  case 44 : 
    newgraf ( curchr > 0 ) ;
    break ;
  case 12 : 
  case 13 : 
  case 17 : 
  case 69 : 
  case 4 : 
  case 24 : 
  case 36 : 
  case 46 : 
  case 48 : 
  case 27 : 
  case 34 : 
  case 65 : 
  case 66 : 
    {
      backinput () ;
      newgraf ( true ) ;
    } 
    break ;
  case 145 : 
  case 246 : 
    indentinhmode () ;
    break ;
  case 14 : 
    {
      normalparagraph () ;
      if ( curlist .modefield > 0 ) 
      buildpage () ;
    } 
    break ;
  case 115 : 
    {
      if ( alignstate < 0 ) 
      offsave () ;
      endgraf () ;
      if ( curlist .modefield == 1 ) 
      buildpage () ;
    } 
    break ;
  case 116 : 
  case 129 : 
  case 138 : 
  case 126 : 
  case 134 : 
    headforvmode () ;
    break ;
  case 38 : 
  case 139 : 
  case 240 : 
  case 140 : 
  case 241 : 
    begininsertoradjust () ;
    break ;
  case 19 : 
  case 120 : 
  case 221 : 
    makemark () ;
    break ;
  case 43 : 
  case 144 : 
  case 245 : 
    appendpenalty () ;
    break ;
  case 26 : 
  case 127 : 
  case 228 : 
    deletelast () ;
    break ;
  case 25 : 
  case 125 : 
  case 226 : 
    unpackage () ;
    break ;
  case 146 : 
    appenditaliccorrection () ;
    break ;
  case 247 : 
    {
      mem [curlist .tailfield ].hh .v.RH = newkern ( 0 ) ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
    break ;
  case 149 : 
  case 250 : 
    appenddiscretionary () ;
    break ;
  case 147 : 
    makeaccent () ;
    break ;
  case 6 : 
  case 107 : 
  case 208 : 
  case 5 : 
  case 106 : 
  case 207 : 
    alignerror () ;
    break ;
  case 35 : 
  case 136 : 
  case 237 : 
    noalignerror () ;
    break ;
  case 64 : 
  case 165 : 
  case 266 : 
    omiterror () ;
    break ;
  case 33 : 
  case 135 : 
    initalign () ;
    break ;
  case 235 : 
    if ( privileged () ) 
    if ( curgroup == 15 ) 
    initalign () ;
    else offsave () ;
    break ;
  case 10 : 
  case 111 : 
    doendv () ;
    break ;
  case 68 : 
  case 169 : 
  case 270 : 
    cserror () ;
    break ;
  case 105 : 
    initmath () ;
    break ;
  case 251 : 
    if ( privileged () ) 
    if ( curgroup == 15 ) 
    starteqno () ;
    else offsave () ;
    break ;
  case 204 : 
    {
      {
	mem [curlist .tailfield ].hh .v.RH = newnoad () ;
	curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
      } 
      backinput () ;
      scanmath ( curlist .tailfield + 1 ) ;
    } 
    break ;
  case 214 : 
  case 215 : 
  case 271 : 
    setmathchar ( eqtb [14651 + curchr ].hh .v.RH ) ;
    break ;
  case 219 : 
    {
      scancharnum () ;
      curchr = curval ;
      setmathchar ( eqtb [14651 + curchr ].hh .v.RH ) ;
    } 
    break ;
  case 220 : 
    {
      scanfifteenbitint () ;
      setmathchar ( curval ) ;
    } 
    break ;
  case 272 : 
    setmathchar ( curchr ) ;
    break ;
  case 218 : 
    {
      scantwentysevenbitint () ;
      setmathchar ( curval / 4096 ) ;
    } 
    break ;
  case 253 : 
    {
      {
	mem [curlist .tailfield ].hh .v.RH = newnoad () ;
	curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
      } 
      mem [curlist .tailfield ].hh.b0 = curchr ;
      scanmath ( curlist .tailfield + 1 ) ;
    } 
    break ;
  case 254 : 
    mathlimitswitch () ;
    break ;
  case 269 : 
    mathradical () ;
    break ;
  case 248 : 
  case 249 : 
    mathac () ;
    break ;
  case 259 : 
    {
      scanspec ( 12 , false ) ;
      normalparagraph () ;
      pushnest () ;
      curlist .modefield = -1 ;
      curlist .auxfield .cint = -65536000L ;
      if ( eqtb [13062 ].hh .v.RH != 0 ) 
      begintokenlist ( eqtb [13062 ].hh .v.RH , 11 ) ;
    } 
    break ;
  case 256 : 
    {
      mem [curlist .tailfield ].hh .v.RH = newstyle ( curchr ) ;
      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
    } 
    break ;
  case 258 : 
    {
      {
	mem [curlist .tailfield ].hh .v.RH = newglue ( membot ) ;
	curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
      } 
      mem [curlist .tailfield ].hh.b1 = 98 ;
    } 
    break ;
  case 257 : 
    appendchoices () ;
    break ;
  case 211 : 
  case 210 : 
    subsup () ;
    break ;
  case 255 : 
    mathfraction () ;
    break ;
  case 252 : 
    mathleftright () ;
    break ;
  case 206 : 
    if ( curgroup == 15 ) 
    aftermath () ;
    else offsave () ;
    break ;
  case 72 : 
  case 173 : 
  case 274 : 
  case 73 : 
  case 174 : 
  case 275 : 
  case 74 : 
  case 175 : 
  case 276 : 
  case 75 : 
  case 176 : 
  case 277 : 
  case 76 : 
  case 177 : 
  case 278 : 
  case 77 : 
  case 178 : 
  case 279 : 
  case 78 : 
  case 179 : 
  case 280 : 
  case 79 : 
  case 180 : 
  case 281 : 
  case 80 : 
  case 181 : 
  case 282 : 
  case 81 : 
  case 182 : 
  case 283 : 
  case 82 : 
  case 183 : 
  case 284 : 
  case 83 : 
  case 184 : 
  case 285 : 
  case 84 : 
  case 185 : 
  case 286 : 
  case 85 : 
  case 186 : 
  case 287 : 
  case 86 : 
  case 187 : 
  case 288 : 
  case 87 : 
  case 188 : 
  case 289 : 
  case 88 : 
  case 189 : 
  case 290 : 
  case 89 : 
  case 190 : 
  case 291 : 
  case 90 : 
  case 191 : 
  case 292 : 
  case 91 : 
  case 192 : 
  case 293 : 
  case 92 : 
  case 193 : 
  case 294 : 
  case 93 : 
  case 194 : 
  case 295 : 
  case 94 : 
  case 195 : 
  case 296 : 
  case 95 : 
  case 196 : 
  case 297 : 
  case 96 : 
  case 197 : 
  case 298 : 
  case 97 : 
  case 198 : 
  case 299 : 
  case 98 : 
  case 199 : 
  case 300 : 
  case 99 : 
  case 200 : 
  case 301 : 
  case 100 : 
  case 201 : 
  case 302 : 
  case 101 : 
  case 202 : 
  case 303 : 
    prefixedcommand () ;
    break ;
  case 41 : 
  case 142 : 
  case 243 : 
    {
      gettoken () ;
      aftertoken = curtok ;
    } 
    break ;
  case 42 : 
  case 143 : 
  case 244 : 
    {
      gettoken () ;
      saveforafter ( curtok ) ;
    } 
    break ;
  case 61 : 
  case 162 : 
  case 263 : 
    openorclosein () ;
    break ;
  case 59 : 
  case 160 : 
  case 261 : 
    issuemessage () ;
    break ;
  case 58 : 
  case 159 : 
  case 260 : 
    shiftcase () ;
    break ;
  case 20 : 
  case 121 : 
  case 222 : 
    showwhatever () ;
    break ;
  case 60 : 
  case 161 : 
  case 262 : 
    doextension () ;
    break ;
  } 
  goto lab60 ;
  lab70: mains = eqtb [14395 + curchr ].hh .v.RH ;
  if ( mains == 1000 ) 
  curlist .auxfield .hh .v.LH = 1000 ;
  else if ( mains < 1000 ) 
  {
    if ( mains > 0 ) 
    curlist .auxfield .hh .v.LH = mains ;
  } 
  else if ( curlist .auxfield .hh .v.LH < 1000 ) 
  curlist .auxfield .hh .v.LH = 1000 ;
  else curlist .auxfield .hh .v.LH = mains ;
  mainf = eqtb [13578 ].hh .v.RH ;
  bchar = fontbchar [mainf ];
  falsebchar = fontfalsebchar [mainf ];
  if ( curlist .modefield > 0 ) 
  if ( eqtb [15213 ].cint != curlist .auxfield .hh .v.RH ) 
  fixlanguage () ;
  {
    ligstack = avail ;
    if ( ligstack == 0 ) 
    ligstack = getavail () ;
    else {
	
      avail = mem [ligstack ].hh .v.RH ;
      mem [ligstack ].hh .v.RH = 0 ;
	;
#ifdef STAT
      incr ( dynused ) ;
#endif /* STAT */
    } 
  } 
  mem [ligstack ].hh.b0 = mainf ;
  curl = curchr ;
  mem [ligstack ].hh.b1 = curl ;
  curq = curlist .tailfield ;
  if ( cancelboundary ) 
  {
    cancelboundary = false ;
    maink = 0 ;
  } 
  else maink = bcharlabel [mainf ];
  if ( maink == 0 ) 
  goto lab92 ;
  curr = curl ;
  curl = 256 ;
  goto lab111 ;
  lab80: if ( curl < 256 ) 
  {
    if ( mem [curlist .tailfield ].hh.b1 == hyphenchar [mainf ]) 
    if ( mem [curq ].hh .v.RH > 0 ) 
    insdisc = true ;
    if ( ligaturepresent ) 
    {
      mainp = newligature ( mainf , curl , mem [curq ].hh .v.RH ) ;
      if ( lfthit ) 
      {
	mem [mainp ].hh.b1 = 2 ;
	lfthit = false ;
      } 
      if ( rthit ) 
      if ( ligstack == 0 ) 
      {
	incr ( mem [mainp ].hh.b1 ) ;
	rthit = false ;
      } 
      mem [curq ].hh .v.RH = mainp ;
      curlist .tailfield = mainp ;
      ligaturepresent = false ;
    } 
    if ( insdisc ) 
    {
      insdisc = false ;
      if ( curlist .modefield > 0 ) 
      {
	mem [curlist .tailfield ].hh .v.RH = newdisc () ;
	curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
      } 
    } 
  } 
  lab90: if ( ligstack == 0 ) 
  goto lab21 ;
  curq = curlist .tailfield ;
  curl = mem [ligstack ].hh.b1 ;
  lab91: if ( ! ( ligstack >= himemmin ) ) 
  goto lab95 ;
  lab92: if ( ( effectivechar ( false , mainf , curchr ) > fontec [mainf ]) 
  || ( effectivechar ( false , mainf , curchr ) < fontbc [mainf ]) ) 
  {
    charwarning ( mainf , curchr ) ;
    {
      mem [ligstack ].hh .v.RH = avail ;
      avail = ligstack ;
	;
#ifdef STAT
      decr ( dynused ) ;
#endif /* STAT */
    } 
    goto lab60 ;
  } 
  maini = effectivecharinfo ( mainf , curl ) ;
  if ( ! ( maini .b0 > 0 ) ) 
  {
    charwarning ( mainf , curchr ) ;
    {
      mem [ligstack ].hh .v.RH = avail ;
      avail = ligstack ;
	;
#ifdef STAT
      decr ( dynused ) ;
#endif /* STAT */
    } 
    goto lab60 ;
  } 
  {
    mem [curlist .tailfield ].hh .v.RH = ligstack ;
    curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
  } 
  lab100: getnext () ;
  if ( curcmd == 11 ) 
  goto lab101 ;
  if ( curcmd == 12 ) 
  goto lab101 ;
  if ( curcmd == 68 ) 
  goto lab101 ;
  xtoken () ;
  if ( curcmd == 11 ) 
  goto lab101 ;
  if ( curcmd == 12 ) 
  goto lab101 ;
  if ( curcmd == 68 ) 
  goto lab101 ;
  if ( curcmd == 16 ) 
  {
    scancharnum () ;
    curchr = curval ;
    goto lab101 ;
  } 
  if ( curcmd == 65 ) 
  bchar = 256 ;
  curr = bchar ;
  ligstack = 0 ;
  goto lab110 ;
  lab101: mains = eqtb [14395 + curchr ].hh .v.RH ;
  if ( mains == 1000 ) 
  curlist .auxfield .hh .v.LH = 1000 ;
  else if ( mains < 1000 ) 
  {
    if ( mains > 0 ) 
    curlist .auxfield .hh .v.LH = mains ;
  } 
  else if ( curlist .auxfield .hh .v.LH < 1000 ) 
  curlist .auxfield .hh .v.LH = 1000 ;
  else curlist .auxfield .hh .v.LH = mains ;
  {
    ligstack = avail ;
    if ( ligstack == 0 ) 
    ligstack = getavail () ;
    else {
	
      avail = mem [ligstack ].hh .v.RH ;
      mem [ligstack ].hh .v.RH = 0 ;
	;
#ifdef STAT
      incr ( dynused ) ;
#endif /* STAT */
    } 
  } 
  mem [ligstack ].hh.b0 = mainf ;
  curr = curchr ;
  mem [ligstack ].hh.b1 = curr ;
  if ( curr == falsebchar ) 
  curr = 256 ;
  lab110: if ( ( ( maini .b2 ) % 4 ) != 1 ) 
  goto lab80 ;
  maink = ligkernbase [mainf ]+ maini .b3 ;
  mainj = fontinfo [maink ].qqqq ;
  if ( mainj .b0 <= 128 ) 
  goto lab112 ;
  maink = ligkernbase [mainf ]+ 256 * mainj .b2 + mainj .b3 + 32768L - 256 * 
  ( 128 ) ;
  lab111: mainj = fontinfo [maink ].qqqq ;
  lab112: if ( mainj .b1 == curr ) 
  if ( mainj .b0 <= 128 ) 
  {
    if ( mainj .b2 >= 128 ) 
    {
      if ( curl < 256 ) 
      {
	if ( mem [curlist .tailfield ].hh.b1 == hyphenchar [mainf ]) 
	if ( mem [curq ].hh .v.RH > 0 ) 
	insdisc = true ;
	if ( ligaturepresent ) 
	{
	  mainp = newligature ( mainf , curl , mem [curq ].hh .v.RH ) ;
	  if ( lfthit ) 
	  {
	    mem [mainp ].hh.b1 = 2 ;
	    lfthit = false ;
	  } 
	  if ( rthit ) 
	  if ( ligstack == 0 ) 
	  {
	    incr ( mem [mainp ].hh.b1 ) ;
	    rthit = false ;
	  } 
	  mem [curq ].hh .v.RH = mainp ;
	  curlist .tailfield = mainp ;
	  ligaturepresent = false ;
	} 
	if ( insdisc ) 
	{
	  insdisc = false ;
	  if ( curlist .modefield > 0 ) 
	  {
	    mem [curlist .tailfield ].hh .v.RH = newdisc () ;
	    curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
	  } 
	} 
      } 
      {
	mem [curlist .tailfield ].hh .v.RH = newkern ( fontinfo [kernbase [
	mainf ]+ 256 * mainj .b2 + mainj .b3 ].cint ) ;
	curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
      } 
      goto lab90 ;
    } 
    if ( curl == 256 ) 
    lfthit = true ;
    else if ( ligstack == 0 ) 
    rthit = true ;
    {
      if ( interrupt != 0 ) 
      pauseforinstructions () ;
    } 
    switch ( mainj .b2 ) 
    {case 1 : 
    case 5 : 
      {
	curl = mainj .b3 ;
	maini = fontinfo [charbase [mainf ]+ effectivechar ( true , mainf , 
	curl ) ].qqqq ;
	ligaturepresent = true ;
      } 
      break ;
    case 2 : 
    case 6 : 
      {
	curr = mainj .b3 ;
	if ( ligstack == 0 ) 
	{
	  ligstack = newligitem ( curr ) ;
	  bchar = 256 ;
	} 
	else if ( ( ligstack >= himemmin ) ) 
	{
	  mainp = ligstack ;
	  ligstack = newligitem ( curr ) ;
	  mem [ligstack + 1 ].hh .v.RH = mainp ;
	} 
	else mem [ligstack ].hh.b1 = curr ;
      } 
      break ;
    case 3 : 
      {
	curr = mainj .b3 ;
	mainp = ligstack ;
	ligstack = newligitem ( curr ) ;
	mem [ligstack ].hh .v.RH = mainp ;
      } 
      break ;
    case 7 : 
    case 11 : 
      {
	if ( curl < 256 ) 
	{
	  if ( mem [curlist .tailfield ].hh.b1 == hyphenchar [mainf ]) 
	  if ( mem [curq ].hh .v.RH > 0 ) 
	  insdisc = true ;
	  if ( ligaturepresent ) 
	  {
	    mainp = newligature ( mainf , curl , mem [curq ].hh .v.RH ) ;
	    if ( lfthit ) 
	    {
	      mem [mainp ].hh.b1 = 2 ;
	      lfthit = false ;
	    } 
	    if ( false ) 
	    if ( ligstack == 0 ) 
	    {
	      incr ( mem [mainp ].hh.b1 ) ;
	      rthit = false ;
	    } 
	    mem [curq ].hh .v.RH = mainp ;
	    curlist .tailfield = mainp ;
	    ligaturepresent = false ;
	  } 
	  if ( insdisc ) 
	  {
	    insdisc = false ;
	    if ( curlist .modefield > 0 ) 
	    {
	      mem [curlist .tailfield ].hh .v.RH = newdisc () ;
	      curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
	    } 
	  } 
	} 
	curq = curlist .tailfield ;
	curl = mainj .b3 ;
	maini = fontinfo [charbase [mainf ]+ effectivechar ( true , mainf , 
	curl ) ].qqqq ;
	ligaturepresent = true ;
      } 
      break ;
      default: 
      {
	curl = mainj .b3 ;
	ligaturepresent = true ;
	if ( ligstack == 0 ) 
	goto lab80 ;
	else goto lab91 ;
      } 
      break ;
    } 
    if ( mainj .b2 > 4 ) 
    if ( mainj .b2 != 7 ) 
    goto lab80 ;
    if ( curl < 256 ) 
    goto lab110 ;
    maink = bcharlabel [mainf ];
    goto lab111 ;
  } 
  if ( mainj .b0 == 0 ) 
  incr ( maink ) ;
  else {
      
    if ( mainj .b0 >= 128 ) 
    goto lab80 ;
    maink = maink + mainj .b0 + 1 ;
  } 
  goto lab111 ;
  lab95: mainp = mem [ligstack + 1 ].hh .v.RH ;
  if ( mainp > 0 ) 
  {
    mem [curlist .tailfield ].hh .v.RH = mainp ;
    curlist .tailfield = mem [curlist .tailfield ].hh .v.RH ;
  } 
  tempptr = ligstack ;
  ligstack = mem [tempptr ].hh .v.RH ;
  freenode ( tempptr , 2 ) ;
  maini = fontinfo [charbase [mainf ]+ effectivechar ( true , mainf , curl 
  ) ].qqqq ;
  ligaturepresent = true ;
  if ( ligstack == 0 ) 
  if ( mainp > 0 ) 
  goto lab100 ;
  else curr = bchar ;
  else curr = mem [ligstack ].hh.b1 ;
  goto lab110 ;
  lab120: if ( eqtb [12538 ].hh .v.RH == membot ) 
  {
    {
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
    tempptr = newglue ( mainp ) ;
  } 
  else tempptr = newparamglue ( 12 ) ;
  mem [curlist .tailfield ].hh .v.RH = tempptr ;
  curlist .tailfield = tempptr ;
  goto lab60 ;
} 
void 
#ifdef HAVE_PROTOTYPES
giveerrhelp ( void ) 
#else
giveerrhelp ( ) 
#endif
{
  giveerrhelp_regmem 
  tokenshow ( eqtb [13065 ].hh .v.RH ) ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
openfmtfile ( void ) 
#else
openfmtfile ( ) 
#endif
{
  /* 40 10 */ register boolean Result; openfmtfile_regmem 
  integer j  ;
  j = curinput .locfield ;
  if ( buffer [curinput .locfield ]== 38 ) 
  {
    incr ( curinput .locfield ) ;
    j = curinput .locfield ;
    buffer [last ]= 32 ;
    while ( buffer [j ]!= 32 ) incr ( j ) ;
    packbufferedname ( 0 , curinput .locfield , j - 1 ) ;
    if ( wopenin ( fmtfile ) ) 
    goto lab40 ;
    Fputs( stdout ,  "Sorry, I can't find the format `" ) ;
    fputs ( (char*) nameoffile + 1 , stdout ) ;
    Fputs( stdout ,  "'; will try `" ) ;
    fputs ( TEXformatdefault + 1 , stdout ) ;
    fprintf( stdout , "%s\n",  "'." ) ;
    fflush ( stdout ) ;
  } 
  packbufferedname ( formatdefaultlength - 4 , 1 , 0 ) ;
  if ( ! wopenin ( fmtfile ) ) 
  {
    ;
    Fputs( stdout ,  "I can't find the format file `" ) ;
    fputs ( TEXformatdefault + 1 , stdout ) ;
    fprintf( stdout , "%s\n",  "'!" ) ;
    Result = false ;
    return Result ;
  } 
  lab40: curinput .locfield = j ;
  Result = true ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
closefilesandterminate ( void ) 
#else
closefilesandterminate ( ) 
#endif
{
  closefilesandterminate_regmem 
  integer k  ;
  {register integer for_end; k = 0 ;for_end = 15 ; if ( k <= for_end) do 
    if ( writeopen [k ]) 
    aclose ( writefile [k ]) ;
  while ( k++ < for_end ) ;} 
	;
#ifdef STAT
  if ( eqtb [15194 ].cint > 0 ) 
  if ( logopened ) 
  {
    fprintf( logfile , "%c\n",  ' ' ) ;
    fprintf( logfile , "%s%s\n",  "Here is how much of TeX's memory" , " you used:" ) ;
    fprintf( logfile , "%c%ld%s",  ' ' , (long)strptr - initstrptr , " string" ) ;
    if ( strptr != initstrptr + 1 ) 
    putc ( 's' ,  logfile );
    fprintf( logfile , "%s%ld\n",  " out of " , (long)maxstrings - initstrptr ) ;
    fprintf( logfile , "%c%ld%s%ld\n",  ' ' , (long)poolptr - initpoolptr ,     " string characters out of " , (long)poolsize - initpoolptr ) ;
    fprintf( logfile , "%c%ld%s%ld\n",  ' ' , (long)lomemmax - memmin + memend - himemmin + 2 ,     " words of memory out of " , (long)memend + 1 - memmin ) ;
    fprintf( logfile , "%c%ld%s%ld%c%ld\n",  ' ' , (long)cscount ,     " multiletter control sequences out of " , (long)10000 , '+' , (long)hashextra ) ;
    fprintf( logfile , "%c%ld%s%ld%s",  ' ' , (long)fmemptr , " words of font info for " , (long)fontptr - 0     , " font" ) ;
    if ( fontptr != 1 ) 
    putc ( 's' ,  logfile );
    fprintf( logfile , "%s%ld%s%ld\n",  ", out of " , (long)fontmemsize , " for " , (long)fontmax - 0 ) ;
    fprintf( logfile , "%c%ld%s",  ' ' , (long)hyphcount , " hyphenation exception" ) ;
    if ( hyphcount != 1 ) 
    putc ( 's' ,  logfile );
    fprintf( logfile , "%s%ld\n",  " out of " , (long)hyphsize ) ;
    fprintf( logfile , "%c%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%c\n",  ' ' , (long)maxinstack , "i," , (long)maxneststack , "n," ,     (long)maxparamstack , "p," , (long)maxbufstack + 1 , "b," , (long)maxsavestack + 6 ,     "s stack positions out of " , (long)stacksize , "i," , (long)nestsize , "n," ,     (long)paramsize , "p," , (long)bufsize , "b," , (long)savesize , 's' ) ;
  } 
#endif /* STAT */
  while ( curs > -1 ) {
      
    if ( curs > 0 ) 
    {
      dvibuf [dviptr ]= 142 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    else {
	
      {
	dvibuf [dviptr ]= 140 ;
	incr ( dviptr ) ;
	if ( dviptr == dvilimit ) 
	dviswap () ;
      } 
      incr ( totalpages ) ;
    } 
    decr ( curs ) ;
  } 
  if ( totalpages == 0 ) 
  printnl ( 832 ) ;
  else {
      
    {
      dvibuf [dviptr ]= 248 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    dvifour ( lastbop ) ;
    lastbop = dvioffset + dviptr - 5 ;
    dvifour ( 25400000L ) ;
    dvifour ( 473628672L ) ;
    preparemag () ;
    dvifour ( eqtb [15180 ].cint ) ;
    dvifour ( maxv ) ;
    dvifour ( maxh ) ;
    {
      dvibuf [dviptr ]= maxpush / 256 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    {
      dvibuf [dviptr ]= maxpush % 256 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    {
      dvibuf [dviptr ]= ( totalpages / 256 ) % 256 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    {
      dvibuf [dviptr ]= totalpages % 256 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    while ( fontptr > 0 ) {
	
      if ( fontused [fontptr ]) 
      dvifontdef ( fontptr ) ;
      decr ( fontptr ) ;
    } 
    {
      dvibuf [dviptr ]= 249 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
    dvifour ( lastbop ) ;
    {
      dvibuf [dviptr ]= 2 ;
      incr ( dviptr ) ;
      if ( dviptr == dvilimit ) 
      dviswap () ;
    } 
	;
#ifdef IPC
    k = 7 - ( ( 3 + dvioffset + dviptr ) % 4 ) ;
#endif /* IPC */
	;
#ifndef IPC
    k = 4 + ( ( dvibufsize - dviptr ) % 4 ) ;
#endif /* not IPC */
    while ( k > 0 ) {
	
      {
	dvibuf [dviptr ]= 223 ;
	incr ( dviptr ) ;
	if ( dviptr == dvilimit ) 
	dviswap () ;
      } 
      decr ( k ) ;
    } 
    if ( dvilimit == halfbuf ) 
    writedvi ( halfbuf , dvibufsize - 1 ) ;
    if ( dviptr > 0 ) 
    writedvi ( 0 , dviptr - 1 ) ;
    printnl ( 833 ) ;
    print ( outputfilename ) ;
    print ( 284 ) ;
    printint ( totalpages ) ;
    print ( 834 ) ;
    if ( totalpages != 1 ) 
    printchar ( 115 ) ;
    print ( 835 ) ;
    printint ( dvioffset + dviptr ) ;
    print ( 836 ) ;
    bclose ( dvifile ) ;
  } 
  if ( logopened ) 
  {
    putc ('\n',  logfile );
    aclose ( logfile ) ;
    selector = selector - 2 ;
    if ( selector == 17 ) 
    {
      printnl ( 1273 ) ;
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
  /* 888 10 */ debughelp_regmem 
  integer k, l, m, n  ;
  while ( true ) {
      
    ;
    printnl ( 1282 ) ;
    fflush ( stdout ) ;
    read ( stdin , m ) ;
    if ( m < 0 ) 
    return ;
    else if ( m == 0 ) 
    dumpcore () ;
    else {
	
      read ( stdin , n ) ;
      switch ( m ) 
      {case 1 : 
	printword ( mem [n ]) ;
	break ;
      case 2 : 
	printint ( mem [n ].hh .v.LH ) ;
	break ;
      case 3 : 
	printint ( mem [n ].hh .v.RH ) ;
	break ;
      case 4 : 
	printword ( eqtb [n ]) ;
	break ;
      case 5 : 
	{
	  printscaled ( fontinfo [n ].cint ) ;
	  printchar ( 32 ) ;
	  printint ( fontinfo [n ].qqqq .b0 ) ;
	  printchar ( 58 ) ;
	  printint ( fontinfo [n ].qqqq .b1 ) ;
	  printchar ( 58 ) ;
	  printint ( fontinfo [n ].qqqq .b2 ) ;
	  printchar ( 58 ) ;
	  printint ( fontinfo [n ].qqqq .b3 ) ;
	} 
	break ;
      case 6 : 
	printword ( savestack [n ]) ;
	break ;
      case 7 : 
	showbox ( n ) ;
	break ;
      case 8 : 
	{
	  breadthmax = 10000 ;
	  depththreshold = poolsize - poolptr - 10 ;
	  shownodelist ( n ) ;
	} 
	break ;
      case 9 : 
	showtokenlist ( n , 0 , 1000 ) ;
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
	  printcmdchr ( n , l ) ;
	} 
	break ;
      case 14 : 
	{register integer for_end; k = 0 ;for_end = n ; if ( k <= for_end) 
	do 
	  print ( buffer [k ]) ;
	while ( k++ < for_end ) ;} 
	break ;
      case 15 : 
	{
	  fontinshortdisplay = 0 ;
	  shortdisplay ( n ) ;
	} 
	break ;
      case 16 : 
	panicking = ! panicking ;
	break ;
	default: 
	print ( 63 ) ;
	break ;
      } 
    } 
  } 
} 
#endif /* TEXMF_DEBUG */
