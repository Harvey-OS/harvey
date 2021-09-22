#define DVITYPE
#include "cpascal.h"
/* 9999 30 */ 
#define maxfonts ( 500 ) 
#define maxwidths ( 25000 ) 
#define linelength ( 79 ) 
#define terminallinelength ( 150 ) 
#define stacksize ( 100 ) 
#define namesize ( 10000 ) 
typedef unsigned char ASCIIcode  ;
typedef text /* of  ASCIIcode */ textfile  ;
typedef unsigned char eightbits  ;
typedef text /* of  eightbits */ bytefile  ;
ASCIIcode xord[256]  ;
ASCIIcode xchr[256]  ;
bytefile dvifile  ;
bytefile tfmfile  ;
integer curloc  ;
char * curname  ;
eightbits b0, b1, b2, b3  ;
integer fontnum[maxfonts + 1]  ;
integer fontname[maxfonts + 1]  ;
ASCIIcode names[namesize + 1]  ;
integer fontchecksum[maxfonts + 1]  ;
integer fontscaledsize[maxfonts + 1]  ;
integer fontdesignsize[maxfonts + 1]  ;
integer fontspace[maxfonts + 1]  ;
integer fontbc[maxfonts + 1]  ;
integer fontec[maxfonts + 1]  ;
integer widthbase[maxfonts + 1]  ;
integer width[maxwidths + 1]  ;
integer nf  ;
integer widthptr  ;
integer inwidth[256]  ;
integer tfmchecksum  ;
integer tfmdesignsize  ;
real tfmconv  ;
integer pixelwidth[maxwidths + 1]  ;
real conv  ;
real trueconv  ;
integer numerator, denominator  ;
integer mag  ;
char outmode  ;
integer maxpages  ;
real resolution  ;
integer newmag  ;
integer startcount[10]  ;
boolean startthere[10]  ;
char startvals  ;
integer count[10]  ;
boolean inpostamble  ;
integer textptr  ;
ASCIIcode textbuf[linelength + 1]  ;
integer h, v, w, x, y, z, hh, vv  ;
integer hstack[stacksize + 1], vstack[stacksize + 1], wstack[stacksize + 1], 
xstack[stacksize + 1], ystack[stacksize + 1], zstack[stacksize + 1]  ;
integer hhstack[stacksize + 1], vvstack[stacksize + 1]  ;
integer maxv  ;
integer maxh  ;
integer maxs  ;
integer maxvsofar, maxhsofar, maxssofar  ;
integer totalpages  ;
integer pagecount  ;
integer s  ;
integer ss  ;
integer curfont  ;
boolean showing  ;
integer oldbackpointer  ;
integer newbackpointer  ;
boolean started  ;
integer postloc  ;
integer firstbackpointer  ;
integer startloc  ;
integer afterpre  ;
integer k, m, n, p, q  ;
cinttype showopcodes  ;

#include "dvitype.h"
void 
#ifdef HAVE_PROTOTYPES
parsearguments ( void ) 
#else
parsearguments ( ) 
#endif
{
  
#define noptions ( 8 ) 
  getoptstruct longoptions[noptions + 1]  ;
  integer getoptreturnval  ;
  cinttype optionindex  ;
  integer currentoption  ;
  char * endnum  ;
  currentoption = 0 ;
  longoptions [currentoption ].name = "help" ;
  longoptions [currentoption ].hasarg = 0 ;
  longoptions [currentoption ].flag = 0 ;
  longoptions [currentoption ].val = 0 ;
  currentoption = currentoption + 1 ;
  longoptions [currentoption ].name = "version" ;
  longoptions [currentoption ].hasarg = 0 ;
  longoptions [currentoption ].flag = 0 ;
  longoptions [currentoption ].val = 0 ;
  currentoption = currentoption + 1 ;
  longoptions [currentoption ].name = "output-level" ;
  longoptions [currentoption ].hasarg = 1 ;
  longoptions [currentoption ].flag = 0 ;
  longoptions [currentoption ].val = 0 ;
  currentoption = currentoption + 1 ;
  outmode = 4 ;
  longoptions [currentoption ].name = "page-start" ;
  longoptions [currentoption ].hasarg = 1 ;
  longoptions [currentoption ].flag = 0 ;
  longoptions [currentoption ].val = 0 ;
  currentoption = currentoption + 1 ;
  longoptions [currentoption ].name = "max-pages" ;
  longoptions [currentoption ].hasarg = 1 ;
  longoptions [currentoption ].flag = 0 ;
  longoptions [currentoption ].val = 0 ;
  currentoption = currentoption + 1 ;
  maxpages = 1000000L ;
  longoptions [currentoption ].name = "dpi" ;
  longoptions [currentoption ].hasarg = 1 ;
  longoptions [currentoption ].flag = 0 ;
  longoptions [currentoption ].val = 0 ;
  currentoption = currentoption + 1 ;
  resolution = 300.0 ;
  longoptions [currentoption ].name = "magnification" ;
  longoptions [currentoption ].hasarg = 1 ;
  longoptions [currentoption ].flag = 0 ;
  longoptions [currentoption ].val = 0 ;
  currentoption = currentoption + 1 ;
  newmag = 0 ;
  longoptions [currentoption ].name = "show-opcodes" ;
  longoptions [currentoption ].hasarg = 0 ;
  longoptions [currentoption ].flag = addressof ( showopcodes ) ;
  longoptions [currentoption ].val = 1 ;
  currentoption = currentoption + 1 ;
  newmag = 0 ;
  longoptions [currentoption ].name = 0 ;
  longoptions [currentoption ].hasarg = 0 ;
  longoptions [currentoption ].flag = 0 ;
  longoptions [currentoption ].val = 0 ;
  do {
      getoptreturnval = getoptlongonly ( argc , argv , "" , longoptions , 
    addressof ( optionindex ) ) ;
    if ( getoptreturnval == -1 ) 
    {
      ;
    } 
    else if ( getoptreturnval == 63 ) 
    {
      usage ( 1 , "dvitype" ) ;
    } 
    else if ( ( strcmp ( longoptions [optionindex ].name , "help" ) == 0 ) ) 
    {
      usage ( 0 , DVITYPEHELP ) ;
    } 
    else if ( ( strcmp ( longoptions [optionindex ].name , "version" ) == 0 
    ) ) 
    {
      printversionandexit ( "This is DVItype, Version 3.6" , nil , 
      "D.E. Knuth" ) ;
    } 
    else if ( ( strcmp ( longoptions [optionindex ].name , "output-level" ) 
    == 0 ) ) 
    {
      outmode = atou ( optarg ) ;
      if ( ( outmode == 0 ) || ( outmode > 4 ) ) 
      {
	fprintf( stderr , "%s\n",  "Value for --output-level must be >= 1 and <= 4." ) 
	;
	uexit ( 1 ) ;
      } 
    } 
    else if ( ( strcmp ( longoptions [optionindex ].name , "page-start" ) == 
    0 ) ) 
    {
      k = 0 ;
      m = 0 ;
      while ( optarg [m ]) {
	  
	if ( optarg [m ]== 42 ) 
	{
	  startthere [k ]= false ;
	  m = m + 1 ;
	} 
	else if ( optarg [m ]== 46 ) 
	{
	  k = k + 1 ;
	  if ( k >= 10 ) 
	  {
	    fprintf( stderr , "%s\n",              "dvitype: More than ten count registers specified." ) ;
	    uexit ( 1 ) ;
	  } 
	  m = m + 1 ;
	} 
	else {
	    
	  startcount [k ]= strtol ( optarg + m , addressof ( endnum ) , 10 ) 
	  ;
	  if ( endnum == optarg + m ) 
	  {
	    fprintf( stderr , "%s\n",              "dvitype: -page-start values must be numeric or *." ) ;
	    uexit ( 1 ) ;
	  } 
	  startthere [k ]= true ;
	  m = m + endnum - ( optarg + m ) ;
	} 
      } 
      startvals = k ;
    } 
    else if ( ( strcmp ( longoptions [optionindex ].name , "max-pages" ) == 
    0 ) ) 
    {
      maxpages = atou ( optarg ) ;
    } 
    else if ( ( strcmp ( longoptions [optionindex ].name , "dpi" ) == 0 ) ) 
    {
      resolution = atof ( optarg ) ;
    } 
    else if ( ( strcmp ( longoptions [optionindex ].name , "magnification" ) 
    == 0 ) ) 
    {
      newmag = atou ( optarg ) ;
    } 
  } while ( ! ( getoptreturnval == -1 ) ) ;
  if ( ( optind + 1 != argc ) ) 
  {
    fprintf( stderr , "%s\n",  "dvitype: Need exactly one file argument." ) ;
    usage ( 1 , "dvitype" ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
initialize ( void ) 
#else
initialize ( ) 
#endif
{
  integer i  ;
  kpsesetprogname ( argv [0 ]) ;
  parsearguments () ;
  Fputs( stdout ,  "This is DVItype, Version 3.6" ) ;
  fprintf( stdout , "%s\n",  versionstring ) ;
  {register integer for_end; i = 0 ;for_end = 31 ; if ( i <= for_end) do 
    xchr [i ]= '?' ;
  while ( i++ < for_end ) ;} 
  xchr [32 ]= ' ' ;
  xchr [33 ]= '!' ;
  xchr [34 ]= '"' ;
  xchr [35 ]= '#' ;
  xchr [36 ]= '$' ;
  xchr [37 ]= '%' ;
  xchr [38 ]= '&' ;
  xchr [39 ]= '\'' ;
  xchr [40 ]= '(' ;
  xchr [41 ]= ')' ;
  xchr [42 ]= '*' ;
  xchr [43 ]= '+' ;
  xchr [44 ]= ',' ;
  xchr [45 ]= '-' ;
  xchr [46 ]= '.' ;
  xchr [47 ]= '/' ;
  xchr [48 ]= '0' ;
  xchr [49 ]= '1' ;
  xchr [50 ]= '2' ;
  xchr [51 ]= '3' ;
  xchr [52 ]= '4' ;
  xchr [53 ]= '5' ;
  xchr [54 ]= '6' ;
  xchr [55 ]= '7' ;
  xchr [56 ]= '8' ;
  xchr [57 ]= '9' ;
  xchr [58 ]= ':' ;
  xchr [59 ]= ';' ;
  xchr [60 ]= '<' ;
  xchr [61 ]= '=' ;
  xchr [62 ]= '>' ;
  xchr [63 ]= '?' ;
  xchr [64 ]= '@' ;
  xchr [65 ]= 'A' ;
  xchr [66 ]= 'B' ;
  xchr [67 ]= 'C' ;
  xchr [68 ]= 'D' ;
  xchr [69 ]= 'E' ;
  xchr [70 ]= 'F' ;
  xchr [71 ]= 'G' ;
  xchr [72 ]= 'H' ;
  xchr [73 ]= 'I' ;
  xchr [74 ]= 'J' ;
  xchr [75 ]= 'K' ;
  xchr [76 ]= 'L' ;
  xchr [77 ]= 'M' ;
  xchr [78 ]= 'N' ;
  xchr [79 ]= 'O' ;
  xchr [80 ]= 'P' ;
  xchr [81 ]= 'Q' ;
  xchr [82 ]= 'R' ;
  xchr [83 ]= 'S' ;
  xchr [84 ]= 'T' ;
  xchr [85 ]= 'U' ;
  xchr [86 ]= 'V' ;
  xchr [87 ]= 'W' ;
  xchr [88 ]= 'X' ;
  xchr [89 ]= 'Y' ;
  xchr [90 ]= 'Z' ;
  xchr [91 ]= '[' ;
  xchr [92 ]= '\\' ;
  xchr [93 ]= ']' ;
  xchr [94 ]= '^' ;
  xchr [95 ]= '_' ;
  xchr [96 ]= '`' ;
  xchr [97 ]= 'a' ;
  xchr [98 ]= 'b' ;
  xchr [99 ]= 'c' ;
  xchr [100 ]= 'd' ;
  xchr [101 ]= 'e' ;
  xchr [102 ]= 'f' ;
  xchr [103 ]= 'g' ;
  xchr [104 ]= 'h' ;
  xchr [105 ]= 'i' ;
  xchr [106 ]= 'j' ;
  xchr [107 ]= 'k' ;
  xchr [108 ]= 'l' ;
  xchr [109 ]= 'm' ;
  xchr [110 ]= 'n' ;
  xchr [111 ]= 'o' ;
  xchr [112 ]= 'p' ;
  xchr [113 ]= 'q' ;
  xchr [114 ]= 'r' ;
  xchr [115 ]= 's' ;
  xchr [116 ]= 't' ;
  xchr [117 ]= 'u' ;
  xchr [118 ]= 'v' ;
  xchr [119 ]= 'w' ;
  xchr [120 ]= 'x' ;
  xchr [121 ]= 'y' ;
  xchr [122 ]= 'z' ;
  xchr [123 ]= '{' ;
  xchr [124 ]= '|' ;
  xchr [125 ]= '}' ;
  xchr [126 ]= '~' ;
  {register integer for_end; i = 127 ;for_end = 255 ; if ( i <= for_end) do 
    xchr [i ]= '?' ;
  while ( i++ < for_end ) ;} 
  {register integer for_end; i = 0 ;for_end = 255 ; if ( i <= for_end) do 
    xord [chr ( i ) ]= 32 ;
  while ( i++ < for_end ) ;} 
  {register integer for_end; i = 32 ;for_end = 126 ; if ( i <= for_end) do 
    xord [xchr [i ]]= i ;
  while ( i++ < for_end ) ;} 
  nf = 0 ;
  widthptr = 0 ;
  fontname [0 ]= 1 ;
  fontspace [maxfonts ]= 0 ;
  fontbc [maxfonts ]= 1 ;
  fontec [maxfonts ]= 0 ;
  inpostamble = false ;
  textptr = 0 ;
  maxv = 2147483548L ;
  maxh = 2147483548L ;
  maxs = stacksize + 1 ;
  maxvsofar = 0 ;
  maxhsofar = 0 ;
  maxssofar = 0 ;
  pagecount = 0 ;
  oldbackpointer = -1 ;
  started = false ;
} 
void 
#ifdef HAVE_PROTOTYPES
opendvifile ( void ) 
#else
opendvifile ( ) 
#endif
{
  curname = extendfilename ( cmdline ( optind ) , "dvi" ) ;
  resetbin ( dvifile , curname ) ;
  curloc = 0 ;
} 
void 
#ifdef HAVE_PROTOTYPES
opentfmfile ( void ) 
#else
opentfmfile ( ) 
#endif
{
  char * fullname  ;
  fullname = kpsefindtfm ( curname ) ;
  if ( fullname ) 
  {
    tfmfile = fopen ( fullname , FOPENRBINMODE ) ;
  } 
  else {
      
    tfmfile = nil ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
readtfmword ( void ) 
#else
readtfmword ( ) 
#endif
{
  read ( tfmfile , b0 ) ;
  read ( tfmfile , b1 ) ;
  read ( tfmfile , b2 ) ;
  read ( tfmfile , b3 ) ;
} 
integer 
#ifdef HAVE_PROTOTYPES
getbyte ( void ) 
#else
getbyte ( ) 
#endif
{
  register integer Result; eightbits b  ;
  if ( eof ( dvifile ) ) 
  Result = 0 ;
  else {
      
    read ( dvifile , b ) ;
    curloc = curloc + 1 ;
    Result = b ;
  } 
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
signedbyte ( void ) 
#else
signedbyte ( ) 
#endif
{
  register integer Result; eightbits b  ;
  read ( dvifile , b ) ;
  curloc = curloc + 1 ;
  if ( b < 128 ) 
  Result = b ;
  else Result = b - 256 ;
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
gettwobytes ( void ) 
#else
gettwobytes ( ) 
#endif
{
  register integer Result; eightbits a, b  ;
  read ( dvifile , a ) ;
  read ( dvifile , b ) ;
  curloc = curloc + 2 ;
  Result = a * toint ( 256 ) + b ;
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
signedpair ( void ) 
#else
signedpair ( ) 
#endif
{
  register integer Result; eightbits a, b  ;
  read ( dvifile , a ) ;
  read ( dvifile , b ) ;
  curloc = curloc + 2 ;
  if ( a < 128 ) 
  Result = a * 256 + b ;
  else Result = ( a - 256 ) * 256 + b ;
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
getthreebytes ( void ) 
#else
getthreebytes ( ) 
#endif
{
  register integer Result; eightbits a, b, c  ;
  read ( dvifile , a ) ;
  read ( dvifile , b ) ;
  read ( dvifile , c ) ;
  curloc = curloc + 3 ;
  Result = ( a * toint ( 256 ) + b ) * 256 + c ;
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
signedtrio ( void ) 
#else
signedtrio ( ) 
#endif
{
  register integer Result; eightbits a, b, c  ;
  read ( dvifile , a ) ;
  read ( dvifile , b ) ;
  read ( dvifile , c ) ;
  curloc = curloc + 3 ;
  if ( a < 128 ) 
  Result = ( a * toint ( 256 ) + b ) * 256 + c ;
  else Result = ( ( a - toint ( 256 ) ) * 256 + b ) * 256 + c ;
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
signedquad ( void ) 
#else
signedquad ( ) 
#endif
{
  register integer Result; eightbits a, b, c, d  ;
  read ( dvifile , a ) ;
  read ( dvifile , b ) ;
  read ( dvifile , c ) ;
  read ( dvifile , d ) ;
  curloc = curloc + 4 ;
  if ( a < 128 ) 
  Result = ( ( a * toint ( 256 ) + b ) * 256 + c ) * 256 + d ;
  else Result = ( ( ( a - 256 ) * toint ( 256 ) + b ) * 256 + c ) * 256 + d ;
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
dvilength ( void ) 
#else
dvilength ( ) 
#endif
{
  register integer Result; xfseek ( dvifile , 0 , 2 , "dvitype" ) ;
  curloc = xftell ( dvifile , "dvitype" ) ;
  Result = curloc ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zmovetobyte ( integer n ) 
#else
zmovetobyte ( n ) 
  integer n ;
#endif
{
  xfseek ( dvifile , n , 0 , "dvitype" ) ;
  curloc = n ;
} 
void 
#ifdef HAVE_PROTOTYPES
zprintfont ( integer f ) 
#else
zprintfont ( f ) 
  integer f ;
#endif
{
  integer k  ;
  if ( f == maxfonts ) 
  Fputs( stdout ,  "UNDEFINED!" ) ;
  else {
      
    {register integer for_end; k = fontname [f ];for_end = fontname [f + 
    1 ]- 1 ; if ( k <= for_end) do 
      putc ( xchr [names [k ]],  stdout );
    while ( k++ < for_end ) ;} 
  } 
} 
boolean 
#ifdef HAVE_PROTOTYPES
zinTFM ( integer z ) 
#else
zinTFM ( z ) 
  integer z ;
#endif
{
  /* 9997 9998 9999 */ register boolean Result; integer k  ;
  integer lh  ;
  integer nw  ;
  integer wp  ;
  integer alpha, beta  ;
  readtfmword () ;
  lh = b2 * toint ( 256 ) + b3 ;
  readtfmword () ;
  fontbc [nf ]= b0 * toint ( 256 ) + b1 ;
  fontec [nf ]= b2 * toint ( 256 ) + b3 ;
  if ( fontec [nf ]< fontbc [nf ]) 
  fontbc [nf ]= fontec [nf ]+ 1 ;
  if ( widthptr + fontec [nf ]- fontbc [nf ]+ 1 > maxwidths ) 
  {
    fprintf( stdout , "%s\n",  "---not loaded, DVItype needs larger width table" ) ;
    goto lab9998 ;
  } 
  wp = widthptr + fontec [nf ]- fontbc [nf ]+ 1 ;
  readtfmword () ;
  nw = b0 * 256 + b1 ;
  if ( ( nw == 0 ) || ( nw > 256 ) ) 
  goto lab9997 ;
  {register integer for_end; k = 1 ;for_end = 3 + lh ; if ( k <= for_end) do 
    {
      if ( eof ( tfmfile ) ) 
      goto lab9997 ;
      readtfmword () ;
      if ( k == 4 ) 
      if ( b0 < 128 ) 
      tfmchecksum = ( ( b0 * toint ( 256 ) + b1 ) * 256 + b2 ) * 256 + b3 ;
      else tfmchecksum = ( ( ( b0 - 256 ) * toint ( 256 ) + b1 ) * 256 + b2 ) 
      * 256 + b3 ;
      else if ( k == 5 ) 
      if ( b0 < 128 ) 
      tfmdesignsize = round ( tfmconv * ( ( ( b0 * 256 + b1 ) * 256 + b2 ) * 
      256 + b3 ) ) ;
      else goto lab9997 ;
    } 
  while ( k++ < for_end ) ;} 
  if ( wp > 0 ) 
  {register integer for_end; k = widthptr ;for_end = wp - 1 ; if ( k <= 
  for_end) do 
    {
      readtfmword () ;
      if ( b0 > nw ) 
      goto lab9997 ;
      width [k ]= b0 ;
    } 
  while ( k++ < for_end ) ;} 
  {
    alpha = 16 ;
    while ( z >= 8388608L ) {
	
      z = z / 2 ;
      alpha = alpha + alpha ;
    } 
    beta = 256 / alpha ;
    alpha = alpha * z ;
  } 
  {register integer for_end; k = 0 ;for_end = nw - 1 ; if ( k <= for_end) do 
    {
      readtfmword () ;
      inwidth [k ]= ( ( ( ( ( b3 * z ) / 256 ) + ( b2 * z ) ) / 256 ) + ( b1 
      * z ) ) / beta ;
      if ( b0 > 0 ) 
      if ( b0 < 255 ) 
      goto lab9997 ;
      else inwidth [k ]= inwidth [k ]- alpha ;
    } 
  while ( k++ < for_end ) ;} 
  if ( inwidth [0 ]!= 0 ) 
  goto lab9997 ;
  widthbase [nf ]= widthptr - fontbc [nf ];
  if ( wp > 0 ) 
  {register integer for_end; k = widthptr ;for_end = wp - 1 ; if ( k <= 
  for_end) do 
    if ( width [k ]== 0 ) 
    {
      width [k ]= 2147483647L ;
      pixelwidth [k ]= 0 ;
    } 
    else {
	
      width [k ]= inwidth [width [k ]];
      pixelwidth [k ]= round ( conv * ( width [k ]) ) ;
    } 
  while ( k++ < for_end ) ;} 
  widthptr = wp ;
  Result = true ;
  goto lab9999 ;
  lab9997: fprintf( stdout , "%s\n",  "---not loaded, TFM file is bad" ) ;
  lab9998: Result = false ;
  lab9999: ;
  return Result ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
startmatch ( void ) 
#else
startmatch ( ) 
#endif
{
  register boolean Result; char k  ;
  boolean match  ;
  match = true ;
  {register integer for_end; k = 0 ;for_end = startvals ; if ( k <= for_end) 
  do 
    if ( startthere [k ]&& ( startcount [k ]!= count [k ]) ) 
    match = false ;
  while ( k++ < for_end ) ;} 
  Result = match ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
zdefinefont ( integer e ) 
#else
zdefinefont ( e ) 
  integer e ;
#endif
{
  integer f  ;
  integer p  ;
  integer n  ;
  integer c, q, d, m  ;
  integer r  ;
  integer j, k  ;
  boolean mismatch  ;
  if ( nf == maxfonts ) 
  {
    fprintf( stderr , "%s%ld%s\n",  "DVItype capacity exceeded (max fonts=" , (long)maxfonts ,     ")!" ) ;
    uexit ( 1 ) ;
  } 
  fontnum [nf ]= e ;
  f = 0 ;
  while ( fontnum [f ]!= e ) f = f + 1 ;
  c = signedquad () ;
  fontchecksum [nf ]= c ;
  q = signedquad () ;
  fontscaledsize [nf ]= q ;
  d = signedquad () ;
  fontdesignsize [nf ]= d ;
  if ( ( q <= 0 ) || ( d <= 0 ) ) 
  m = 1000 ;
  else m = round ( ( 1000.0 * conv * q ) / ((double) ( trueconv * d ) ) ) ;
  p = getbyte () ;
  n = getbyte () ;
  if ( fontname [nf ]+ n + p > namesize ) 
  {
    fprintf( stderr , "%s%ld%s\n",  "DVItype capacity exceeded (name size=" , (long)namesize ,     ")!" ) ;
    uexit ( 1 ) ;
  } 
  fontname [nf + 1 ]= fontname [nf ]+ n + p ;
  if ( showing ) 
  Fputs( stdout ,  ": " ) ;
  else
  fprintf( stdout , "%s%ld%s",  "Font " , (long)e , ": " ) ;
  if ( n + p == 0 ) 
  Fputs( stdout ,  "null font name!" ) ;
  else {
      register integer for_end; k = fontname [nf ];for_end = fontname [
  nf + 1 ]- 1 ; if ( k <= for_end) do 
    names [k ]= getbyte () ;
  while ( k++ < for_end ) ;} 
  printfont ( nf ) ;
  if ( ! showing ) 
  if ( m != 1000 ) 
  fprintf( stdout , "%s%ld",  " scaled " , (long)m ) ;
  if ( ( ( outmode == 4 ) && inpostamble ) || ( ( outmode < 4 ) && ! 
  inpostamble ) ) 
  {
    if ( f < nf ) 
    fprintf( stdout , "%s\n",  "---this font was already defined!" ) ;
  } 
  else {
      
    if ( f == nf ) 
    fprintf( stdout , "%s\n",  "---this font wasn't loaded before!" ) ;
  } 
  if ( f == nf ) 
  {
    r = fontname [nf + 1 ]- fontname [nf ];
    curname = xmalloc ( r + 1 ) ;
    {register integer for_end; k = fontname [nf ];for_end = fontname [nf 
    + 1 ]; if ( k <= for_end) do 
      {
	curname [k - fontname [nf ]]= xchr [names [k ]];
      } 
    while ( k++ < for_end ) ;} 
    curname [r ]= 0 ;
    opentfmfile () ;
    if ( eof ( tfmfile ) ) 
    Fputs( stdout ,  "---not loaded, TFM file can't be opened!" ) ;
    else {
	
      if ( ( q <= 0 ) || ( q >= 134217728L ) ) 
      fprintf( stdout , "%s%ld%s",  "---not loaded, bad scale (" , (long)q , ")!" ) ;
      else if ( ( d <= 0 ) || ( d >= 134217728L ) ) 
      fprintf( stdout , "%s%ld%s",  "---not loaded, bad design size (" , (long)d , ")!" ) ;
      else if ( inTFM ( q ) ) 
      {
	fontspace [nf ]= q / 6 ;
	if ( ( c != 0 ) && ( tfmchecksum != 0 ) && ( c != tfmchecksum ) ) 
	{
	  fprintf( stdout , "%s\n",  "---beware: check sums do not agree!" ) ;
	  fprintf( stdout , "%s%ld%s%ld%c\n",  "   (" , (long)c , " vs. " , (long)tfmchecksum , ')' ) ;
	  Fputs( stdout ,  "   " ) ;
	} 
	if ( abs ( tfmdesignsize - d ) > 2 ) 
	{
	  fprintf( stdout , "%s\n",  "---beware: design sizes do not agree!" ) ;
	  fprintf( stdout , "%s%ld%s%ld%c\n",  "   (" , (long)d , " vs. " , (long)tfmdesignsize , ')' ) ;
	  Fputs( stdout ,  "   " ) ;
	} 
	fprintf( stdout , "%s%ld%s",  "---loaded at size " , (long)q , " DVI units" ) ;
	d = round ( ( 100.0 * conv * q ) / ((double) ( trueconv * d ) ) ) ;
	if ( d != 100 ) 
	{
	  fprintf( stdout , "%c\n",  ' ' ) ;
	  fprintf( stdout , "%s%ld%s",  " (this font is magnified " , (long)d , "%)" ) ;
	} 
	nf = nf + 1 ;
      } 
    } 
    if ( outmode == 0 ) 
    fprintf( stdout , "%c\n",  ' ' ) ;
    if ( tfmfile ) 
    xfclose ( tfmfile , curname ) ;
    free ( curname ) ;
  } 
  else {
      
    if ( fontchecksum [f ]!= c ) 
    fprintf( stdout , "%s\n",  "---check sum doesn't match previous definition!" ) ;
    if ( fontscaledsize [f ]!= q ) 
    fprintf( stdout , "%s\n",  "---scaled size doesn't match previous definition!" ) ;
    if ( fontdesignsize [f ]!= d ) 
    fprintf( stdout , "%s\n",  "---design size doesn't match previous definition!" ) ;
    j = fontname [f ];
    k = fontname [nf ];
    if ( fontname [f + 1 ]- j != fontname [nf + 1 ]- k ) 
    mismatch = true ;
    else {
	
      mismatch = false ;
      while ( j < fontname [f + 1 ]) {
	  
	if ( names [j ]!= names [k ]) 
	mismatch = true ;
	j = j + 1 ;
	k = k + 1 ;
      } 
    } 
    if ( mismatch ) 
    fprintf( stdout , "%s\n",  "---font name doesn't match previous definition!" ) ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
flushtext ( void ) 
#else
flushtext ( ) 
#endif
{
  integer k  ;
  if ( textptr > 0 ) 
  {
    if ( outmode > 0 ) 
    {
      putc ( '[' ,  stdout );
      {register integer for_end; k = 1 ;for_end = textptr ; if ( k <= 
      for_end) do 
	putc ( xchr [textbuf [k ]],  stdout );
      while ( k++ < for_end ) ;} 
      fprintf( stdout , "%c\n",  ']' ) ;
    } 
    textptr = 0 ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zouttext ( ASCIIcode c ) 
#else
zouttext ( c ) 
  ASCIIcode c ;
#endif
{
  if ( textptr == linelength - 2 ) 
  flushtext () ;
  textptr = textptr + 1 ;
  textbuf [textptr ]= c ;
} 
integer 
#ifdef HAVE_PROTOTYPES
zfirstpar ( eightbits o ) 
#else
zfirstpar ( o ) 
  eightbits o ;
#endif
{
  register integer Result; switch ( o ) 
  {case 0 : 
  case 1 : 
  case 2 : 
  case 3 : 
  case 4 : 
  case 5 : 
  case 6 : 
  case 7 : 
  case 8 : 
  case 9 : 
  case 10 : 
  case 11 : 
  case 12 : 
  case 13 : 
  case 14 : 
  case 15 : 
  case 16 : 
  case 17 : 
  case 18 : 
  case 19 : 
  case 20 : 
  case 21 : 
  case 22 : 
  case 23 : 
  case 24 : 
  case 25 : 
  case 26 : 
  case 27 : 
  case 28 : 
  case 29 : 
  case 30 : 
  case 31 : 
  case 32 : 
  case 33 : 
  case 34 : 
  case 35 : 
  case 36 : 
  case 37 : 
  case 38 : 
  case 39 : 
  case 40 : 
  case 41 : 
  case 42 : 
  case 43 : 
  case 44 : 
  case 45 : 
  case 46 : 
  case 47 : 
  case 48 : 
  case 49 : 
  case 50 : 
  case 51 : 
  case 52 : 
  case 53 : 
  case 54 : 
  case 55 : 
  case 56 : 
  case 57 : 
  case 58 : 
  case 59 : 
  case 60 : 
  case 61 : 
  case 62 : 
  case 63 : 
  case 64 : 
  case 65 : 
  case 66 : 
  case 67 : 
  case 68 : 
  case 69 : 
  case 70 : 
  case 71 : 
  case 72 : 
  case 73 : 
  case 74 : 
  case 75 : 
  case 76 : 
  case 77 : 
  case 78 : 
  case 79 : 
  case 80 : 
  case 81 : 
  case 82 : 
  case 83 : 
  case 84 : 
  case 85 : 
  case 86 : 
  case 87 : 
  case 88 : 
  case 89 : 
  case 90 : 
  case 91 : 
  case 92 : 
  case 93 : 
  case 94 : 
  case 95 : 
  case 96 : 
  case 97 : 
  case 98 : 
  case 99 : 
  case 100 : 
  case 101 : 
  case 102 : 
  case 103 : 
  case 104 : 
  case 105 : 
  case 106 : 
  case 107 : 
  case 108 : 
  case 109 : 
  case 110 : 
  case 111 : 
  case 112 : 
  case 113 : 
  case 114 : 
  case 115 : 
  case 116 : 
  case 117 : 
  case 118 : 
  case 119 : 
  case 120 : 
  case 121 : 
  case 122 : 
  case 123 : 
  case 124 : 
  case 125 : 
  case 126 : 
  case 127 : 
    Result = o - 0 ;
    break ;
  case 128 : 
  case 133 : 
  case 235 : 
  case 239 : 
  case 243 : 
    Result = getbyte () ;
    break ;
  case 129 : 
  case 134 : 
  case 236 : 
  case 240 : 
  case 244 : 
    Result = gettwobytes () ;
    break ;
  case 130 : 
  case 135 : 
  case 237 : 
  case 241 : 
  case 245 : 
    Result = getthreebytes () ;
    break ;
  case 143 : 
  case 148 : 
  case 153 : 
  case 157 : 
  case 162 : 
  case 167 : 
    Result = signedbyte () ;
    break ;
  case 144 : 
  case 149 : 
  case 154 : 
  case 158 : 
  case 163 : 
  case 168 : 
    Result = signedpair () ;
    break ;
  case 145 : 
  case 150 : 
  case 155 : 
  case 159 : 
  case 164 : 
  case 169 : 
    Result = signedtrio () ;
    break ;
  case 131 : 
  case 132 : 
  case 136 : 
  case 137 : 
  case 146 : 
  case 151 : 
  case 156 : 
  case 160 : 
  case 165 : 
  case 170 : 
  case 238 : 
  case 242 : 
  case 246 : 
    Result = signedquad () ;
    break ;
  case 138 : 
  case 139 : 
  case 140 : 
  case 141 : 
  case 142 : 
  case 247 : 
  case 248 : 
  case 249 : 
  case 250 : 
  case 251 : 
  case 252 : 
  case 253 : 
  case 254 : 
  case 255 : 
    Result = 0 ;
    break ;
  case 147 : 
    Result = w ;
    break ;
  case 152 : 
    Result = x ;
    break ;
  case 161 : 
    Result = y ;
    break ;
  case 166 : 
    Result = z ;
    break ;
  case 171 : 
  case 172 : 
  case 173 : 
  case 174 : 
  case 175 : 
  case 176 : 
  case 177 : 
  case 178 : 
  case 179 : 
  case 180 : 
  case 181 : 
  case 182 : 
  case 183 : 
  case 184 : 
  case 185 : 
  case 186 : 
  case 187 : 
  case 188 : 
  case 189 : 
  case 190 : 
  case 191 : 
  case 192 : 
  case 193 : 
  case 194 : 
  case 195 : 
  case 196 : 
  case 197 : 
  case 198 : 
  case 199 : 
  case 200 : 
  case 201 : 
  case 202 : 
  case 203 : 
  case 204 : 
  case 205 : 
  case 206 : 
  case 207 : 
  case 208 : 
  case 209 : 
  case 210 : 
  case 211 : 
  case 212 : 
  case 213 : 
  case 214 : 
  case 215 : 
  case 216 : 
  case 217 : 
  case 218 : 
  case 219 : 
  case 220 : 
  case 221 : 
  case 222 : 
  case 223 : 
  case 224 : 
  case 225 : 
  case 226 : 
  case 227 : 
  case 228 : 
  case 229 : 
  case 230 : 
  case 231 : 
  case 232 : 
  case 233 : 
  case 234 : 
    Result = o - 171 ;
    break ;
  } 
  return Result ;
} 
integer 
#ifdef HAVE_PROTOTYPES
zrulepixels ( integer x ) 
#else
zrulepixels ( x ) 
  integer x ;
#endif
{
  register integer Result; integer n  ;
  n = trunc ( conv * x ) ;
  if ( n < conv * x ) 
  Result = n + 1 ;
  else Result = n ;
  return Result ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
zspecialcases ( eightbits o , integer p , integer a ) 
#else
zspecialcases ( o , p , a ) 
  eightbits o ;
  integer p ;
  integer a ;
#endif
{
  /* 46 44 30 9998 */ register boolean Result; integer q  ;
  integer k  ;
  boolean badchar  ;
  boolean pure  ;
  integer vvv  ;
  pure = true ;
  switch ( o ) 
  {case 157 : 
  case 158 : 
  case 159 : 
  case 160 : 
    {
      if ( abs ( p ) >= 5 * fontspace [curfont ]) 
      vv = round ( conv * ( v + p ) ) ;
      else vv = vv + round ( conv * ( p ) ) ;
      if ( outmode > 0 ) 
      {
	flushtext () ;
	showing = true ;
	fprintf( stdout , "%ld%s%s%ld%c%ld",  (long)a , ": " , "down" , (long)o - 156 , ' ' , (long)p ) ;
	if ( showopcodes && ( o >= 128 ) ) 
	fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
      } 
      goto lab44 ;
    } 
    break ;
  case 161 : 
  case 162 : 
  case 163 : 
  case 164 : 
  case 165 : 
    {
      y = p ;
      if ( abs ( p ) >= 5 * fontspace [curfont ]) 
      vv = round ( conv * ( v + p ) ) ;
      else vv = vv + round ( conv * ( p ) ) ;
      if ( outmode > 0 ) 
      {
	flushtext () ;
	showing = true ;
	fprintf( stdout , "%ld%s%c%ld%c%ld",  (long)a , ": " , 'y' , (long)o - 161 , ' ' , (long)p ) ;
	if ( showopcodes && ( o >= 128 ) ) 
	fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
      } 
      goto lab44 ;
    } 
    break ;
  case 166 : 
  case 167 : 
  case 168 : 
  case 169 : 
  case 170 : 
    {
      z = p ;
      if ( abs ( p ) >= 5 * fontspace [curfont ]) 
      vv = round ( conv * ( v + p ) ) ;
      else vv = vv + round ( conv * ( p ) ) ;
      if ( outmode > 0 ) 
      {
	flushtext () ;
	showing = true ;
	fprintf( stdout , "%ld%s%c%ld%c%ld",  (long)a , ": " , 'z' , (long)o - 166 , ' ' , (long)p ) ;
	if ( showopcodes && ( o >= 128 ) ) 
	fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
      } 
      goto lab44 ;
    } 
    break ;
  case 171 : 
  case 172 : 
  case 173 : 
  case 174 : 
  case 175 : 
  case 176 : 
  case 177 : 
  case 178 : 
  case 179 : 
  case 180 : 
  case 181 : 
  case 182 : 
  case 183 : 
  case 184 : 
  case 185 : 
  case 186 : 
  case 187 : 
  case 188 : 
  case 189 : 
  case 190 : 
  case 191 : 
  case 192 : 
  case 193 : 
  case 194 : 
  case 195 : 
  case 196 : 
  case 197 : 
  case 198 : 
  case 199 : 
  case 200 : 
  case 201 : 
  case 202 : 
  case 203 : 
  case 204 : 
  case 205 : 
  case 206 : 
  case 207 : 
  case 208 : 
  case 209 : 
  case 210 : 
  case 211 : 
  case 212 : 
  case 213 : 
  case 214 : 
  case 215 : 
  case 216 : 
  case 217 : 
  case 218 : 
  case 219 : 
  case 220 : 
  case 221 : 
  case 222 : 
  case 223 : 
  case 224 : 
  case 225 : 
  case 226 : 
  case 227 : 
  case 228 : 
  case 229 : 
  case 230 : 
  case 231 : 
  case 232 : 
  case 233 : 
  case 234 : 
    {
      if ( outmode > 0 ) 
      {
	flushtext () ;
	showing = true ;
	fprintf( stdout , "%ld%s%s%ld",  (long)a , ": " , "fntnum" , (long)p ) ;
	if ( showopcodes && ( o >= 128 ) ) 
	fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
      } 
      goto lab46 ;
    } 
    break ;
  case 235 : 
  case 236 : 
  case 237 : 
  case 238 : 
    {
      if ( outmode > 0 ) 
      {
	flushtext () ;
	showing = true ;
	fprintf( stdout , "%ld%s%s%ld%c%ld",  (long)a , ": " , "fnt" , (long)o - 234 , ' ' , (long)p ) ;
	if ( showopcodes && ( o >= 128 ) ) 
	fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
      } 
      goto lab46 ;
    } 
    break ;
  case 243 : 
  case 244 : 
  case 245 : 
  case 246 : 
    {
      if ( outmode > 0 ) 
      {
	flushtext () ;
	showing = true ;
	fprintf( stdout , "%ld%s%s%ld%c%ld",  (long)a , ": " , "fntdef" , (long)o - 242 , ' ' , (long)p ) ;
	if ( showopcodes && ( o >= 128 ) ) 
	fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
      } 
      definefont ( p ) ;
      goto lab30 ;
    } 
    break ;
  case 239 : 
  case 240 : 
  case 241 : 
  case 242 : 
    {
      if ( outmode > 0 ) 
      {
	flushtext () ;
	showing = true ;
	fprintf( stdout , "%ld%s%s",  (long)a , ": " , "xxx '" ) ;
	if ( showopcodes && ( o >= 128 ) ) 
	fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
      } 
      badchar = false ;
      if ( p < 0 ) 
      if ( ! showing ) 
      {
	flushtext () ;
	showing = true ;
	fprintf( stdout , "%ld%s%s",  (long)a , ": " , "string of negative length!" ) ;
	if ( showopcodes && ( o >= 128 ) ) 
	fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
      } 
      else
      fprintf( stdout , "%c%s",  ' ' , "string of negative length!" ) ;
      {register integer for_end; k = 1 ;for_end = p ; if ( k <= for_end) do 
	{
	  q = getbyte () ;
	  if ( ( q < 32 ) || ( q > 126 ) ) 
	  badchar = true ;
	  if ( showing ) 
	  putc ( xchr [q ],  stdout );
	} 
      while ( k++ < for_end ) ;} 
      if ( showing ) 
      putc ( '\'' ,  stdout );
      if ( badchar ) 
      if ( ! showing ) 
      {
	flushtext () ;
	showing = true ;
	fprintf( stdout , "%ld%s%s",  (long)a , ": " , "non-ASCII character in xxx command!" ) ;
	if ( showopcodes && ( o >= 128 ) ) 
	fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
      } 
      else
      fprintf( stdout , "%c%s",  ' ' , "non-ASCII character in xxx command!" ) ;
      goto lab30 ;
    } 
    break ;
  case 247 : 
    {
      if ( ! showing ) 
      {
	flushtext () ;
	showing = true ;
	fprintf( stdout , "%ld%s%s",  (long)a , ": " , "preamble command within a page!" ) ;
	if ( showopcodes && ( o >= 128 ) ) 
	fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
      } 
      else
      fprintf( stdout , "%c%s",  ' ' , "preamble command within a page!" ) ;
      goto lab9998 ;
    } 
    break ;
  case 248 : 
  case 249 : 
    {
      if ( ! showing ) 
      {
	flushtext () ;
	showing = true ;
	fprintf( stdout , "%ld%s%s",  (long)a , ": " , "postamble command within a page!" ) ;
	if ( showopcodes && ( o >= 128 ) ) 
	fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
      } 
      else
      fprintf( stdout , "%c%s",  ' ' , "postamble command within a page!" ) ;
      goto lab9998 ;
    } 
    break ;
    default: 
    {
      if ( ! showing ) 
      {
	flushtext () ;
	showing = true ;
	fprintf( stdout , "%ld%s%s%ld%c",  (long)a , ": " , "undefined command " , (long)o , '!' ) ;
	if ( showopcodes && ( o >= 128 ) ) 
	fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
      } 
      else
      fprintf( stdout , "%c%s%ld%c",  ' ' , "undefined command " , (long)o , '!' ) ;
      goto lab30 ;
    } 
    break ;
  } 
  lab44: if ( ( v > 0 ) && ( p > 0 ) ) 
  if ( v > 2147483647L - p ) 
  {
    if ( ! showing ) 
    {
      flushtext () ;
      showing = true ;
      fprintf( stdout , "%ld%s%s%ld%s%ld",  (long)a , ": " ,       "arithmetic overflow! parameter changed from " , (long)p , " to " ,       (long)2147483647L - v ) ;
      if ( showopcodes && ( o >= 128 ) ) 
      fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
    } 
    else
    fprintf( stdout , "%c%s%ld%s%ld",  ' ' , "arithmetic overflow! parameter changed from "     , (long)p , " to " , (long)2147483647L - v ) ;
    p = 2147483647L - v ;
  } 
  if ( ( v < 0 ) && ( p < 0 ) ) 
  if ( - (integer) v > p + 2147483647L ) 
  {
    if ( ! showing ) 
    {
      flushtext () ;
      showing = true ;
      fprintf( stdout , "%ld%s%s%ld%s%ld",  (long)a , ": " ,       "arithmetic overflow! parameter changed from " , (long)p , " to " , (long)(       - (integer) v ) - 2147483647L ) ;
      if ( showopcodes && ( o >= 128 ) ) 
      fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
    } 
    else
    fprintf( stdout , "%c%s%ld%s%ld",  ' ' , "arithmetic overflow! parameter changed from "     , (long)p , " to " , (long)( - (integer) v ) - 2147483647L ) ;
    p = ( - (integer) v ) - 2147483647L ;
  } 
  vvv = round ( conv * ( v + p ) ) ;
  if ( abs ( vvv - vv ) > 2 ) 
  if ( vvv > vv ) 
  vv = vvv - 2 ;
  else vv = vvv + 2 ;
  if ( showing ) 
  if ( outmode > 2 ) 
  {
    fprintf( stdout , "%s%ld",  " v:=" , (long)v ) ;
    if ( p >= 0 ) 
    putc ( '+' ,  stdout );
    fprintf( stdout , "%ld%c%ld%s%ld",  (long)p , '=' , (long)v + p , ", vv:=" , (long)vv ) ;
  } 
  v = v + p ;
  if ( abs ( v ) > maxvsofar ) 
  {
    if ( abs ( v ) > maxv + 99 ) 
    {
      if ( ! showing ) 
      {
	flushtext () ;
	showing = true ;
	fprintf( stdout , "%ld%s%s%ld%c",  (long)a , ": " , "warning: |v|>" , (long)maxv , '!' ) ;
	if ( showopcodes && ( o >= 128 ) ) 
	fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
      } 
      else
      fprintf( stdout , "%c%s%ld%c",  ' ' , "warning: |v|>" , (long)maxv , '!' ) ;
      maxv = abs ( v ) ;
    } 
    maxvsofar = abs ( v ) ;
  } 
  goto lab30 ;
  lab46: fontnum [nf ]= p ;
  curfont = 0 ;
  while ( fontnum [curfont ]!= p ) curfont = curfont + 1 ;
  if ( curfont == nf ) 
  {
    curfont = maxfonts ;
    if ( ! showing ) 
    {
      flushtext () ;
      showing = true ;
      fprintf( stdout , "%ld%s%s%ld%s",  (long)a , ": " , "invalid font selection: font " , (long)p ,       " was never defined!" ) ;
      if ( showopcodes && ( o >= 128 ) ) 
      fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
    } 
    else
    fprintf( stdout , "%c%s%ld%s",  ' ' , "invalid font selection: font " , (long)p ,     " was never defined!" ) ;
  } 
  if ( showing ) 
  if ( outmode > 2 ) 
  {
    Fputs( stdout ,  " current font is " ) ;
    printfont ( curfont ) ;
  } 
  goto lab30 ;
  lab9998: pure = false ;
  lab30: Result = pure ;
  return Result ;
} 
boolean 
#ifdef HAVE_PROTOTYPES
dopage ( void ) 
#else
dopage ( ) 
#endif
{
  /* 41 42 43 45 30 9998 9999 */ register boolean Result; eightbits o  ;
  integer p, q  ;
  integer a  ;
  integer hhh  ;
  curfont = maxfonts ;
  s = 0 ;
  h = 0 ;
  v = 0 ;
  w = 0 ;
  x = 0 ;
  y = 0 ;
  z = 0 ;
  hh = 0 ;
  vv = 0 ;
  while ( true ) {
      
    a = curloc ;
    showing = false ;
    o = getbyte () ;
    p = firstpar ( o ) ;
    if ( eof ( dvifile ) ) 
    {
      fprintf( stderr , "%s%s%c\n",  "Bad DVI file: " , "the file ended prematurely" , '!'       ) ;
      uexit ( 1 ) ;
    } 
    if ( o < 128 ) 
    {
      if ( ( o > 32 ) && ( o <= 126 ) ) 
      {
	outtext ( p ) ;
	if ( outmode > 1 ) 
	{
	  showing = true ;
	  fprintf( stdout , "%ld%s%s%ld",  (long)a , ": " , "setchar" , (long)p ) ;
	  if ( showopcodes && ( o >= 128 ) ) 
	  fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
	} 
      } 
      else if ( outmode > 0 ) 
      {
	flushtext () ;
	showing = true ;
	fprintf( stdout , "%ld%s%s%ld",  (long)a , ": " , "setchar" , (long)p ) ;
	if ( showopcodes && ( o >= 128 ) ) 
	fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
      } 
      goto lab41 ;
    } 
    else switch ( o ) 
    {case 128 : 
    case 129 : 
    case 130 : 
    case 131 : 
      {
	if ( outmode > 0 ) 
	{
	  flushtext () ;
	  showing = true ;
	  fprintf( stdout , "%ld%s%s%ld%c%ld",  (long)a , ": " , "set" , (long)o - 127 , ' ' , (long)p ) ;
	  if ( showopcodes && ( o >= 128 ) ) 
	  fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
	} 
	goto lab41 ;
      } 
      break ;
    case 133 : 
    case 134 : 
    case 135 : 
    case 136 : 
      {
	if ( outmode > 0 ) 
	{
	  flushtext () ;
	  showing = true ;
	  fprintf( stdout , "%ld%s%s%ld%c%ld",  (long)a , ": " , "put" , (long)o - 132 , ' ' , (long)p ) ;
	  if ( showopcodes && ( o >= 128 ) ) 
	  fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
	} 
	goto lab41 ;
      } 
      break ;
    case 132 : 
      {
	if ( outmode > 0 ) 
	{
	  flushtext () ;
	  showing = true ;
	  fprintf( stdout , "%ld%s%s",  (long)a , ": " , "setrule" ) ;
	  if ( showopcodes && ( o >= 128 ) ) 
	  fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
	} 
	goto lab42 ;
      } 
      break ;
    case 137 : 
      {
	if ( outmode > 0 ) 
	{
	  flushtext () ;
	  showing = true ;
	  fprintf( stdout , "%ld%s%s",  (long)a , ": " , "putrule" ) ;
	  if ( showopcodes && ( o >= 128 ) ) 
	  fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
	} 
	goto lab42 ;
      } 
      break ;
    case 138 : 
      {
	if ( outmode > 1 ) 
	{
	  showing = true ;
	  fprintf( stdout , "%ld%s%s",  (long)a , ": " , "nop" ) ;
	  if ( showopcodes && ( o >= 128 ) ) 
	  fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
	} 
	goto lab30 ;
      } 
      break ;
    case 139 : 
      {
	if ( ! showing ) 
	{
	  flushtext () ;
	  showing = true ;
	  fprintf( stdout , "%ld%s%s",  (long)a , ": " , "bop occurred before eop!" ) ;
	  if ( showopcodes && ( o >= 128 ) ) 
	  fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
	} 
	else
	fprintf( stdout , "%c%s",  ' ' , "bop occurred before eop!" ) ;
	goto lab9998 ;
      } 
      break ;
    case 140 : 
      {
	if ( outmode > 0 ) 
	{
	  flushtext () ;
	  showing = true ;
	  fprintf( stdout , "%ld%s%s",  (long)a , ": " , "eop" ) ;
	  if ( showopcodes && ( o >= 128 ) ) 
	  fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
	} 
	if ( s != 0 ) 
	if ( ! showing ) 
	{
	  flushtext () ;
	  showing = true ;
	  fprintf( stdout , "%ld%s%s%ld%s",  (long)a , ": " , "stack not empty at end of page (level "           , (long)s , ")!" ) ;
	  if ( showopcodes && ( o >= 128 ) ) 
	  fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
	} 
	else
	fprintf( stdout , "%c%s%ld%s",  ' ' , "stack not empty at end of page (level " ,         (long)s , ")!" ) ;
	Result = true ;
	fprintf( stdout , "%c\n",  ' ' ) ;
	goto lab9999 ;
      } 
      break ;
    case 141 : 
      {
	if ( outmode > 0 ) 
	{
	  flushtext () ;
	  showing = true ;
	  fprintf( stdout , "%ld%s%s",  (long)a , ": " , "push" ) ;
	  if ( showopcodes && ( o >= 128 ) ) 
	  fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
	} 
	if ( s == maxssofar ) 
	{
	  maxssofar = s + 1 ;
	  if ( s == maxs ) 
	  if ( ! showing ) 
	  {
	    flushtext () ;
	    showing = true ;
	    fprintf( stdout , "%ld%s%s",  (long)a , ": " , "deeper than claimed in postamble!" ) 
	    ;
	    if ( showopcodes && ( o >= 128 ) ) 
	    fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
	  } 
	  else
	  fprintf( stdout , "%c%s",  ' ' , "deeper than claimed in postamble!" ) ;
	  if ( s == stacksize ) 
	  {
	    if ( ! showing ) 
	    {
	      flushtext () ;
	      showing = true ;
	      fprintf( stdout , "%ld%s%s%ld%c",  (long)a , ": " ,               "DVItype capacity exceeded (stack size=" , (long)stacksize , ')' ) ;
	      if ( showopcodes && ( o >= 128 ) ) 
	      fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
	    } 
	    else
	    fprintf( stdout , "%c%s%ld%c",  ' ' ,             "DVItype capacity exceeded (stack size=" , (long)stacksize , ')' ) ;
	    goto lab9998 ;
	  } 
	} 
	hstack [s ]= h ;
	vstack [s ]= v ;
	wstack [s ]= w ;
	xstack [s ]= x ;
	ystack [s ]= y ;
	zstack [s ]= z ;
	hhstack [s ]= hh ;
	vvstack [s ]= vv ;
	s = s + 1 ;
	ss = s - 1 ;
	goto lab45 ;
      } 
      break ;
    case 142 : 
      {
	if ( outmode > 0 ) 
	{
	  flushtext () ;
	  showing = true ;
	  fprintf( stdout , "%ld%s%s",  (long)a , ": " , "pop" ) ;
	  if ( showopcodes && ( o >= 128 ) ) 
	  fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
	} 
	if ( s == 0 ) 
	if ( ! showing ) 
	{
	  flushtext () ;
	  showing = true ;
	  fprintf( stdout , "%ld%s%s",  (long)a , ": " , "(illegal at level zero)!" ) ;
	  if ( showopcodes && ( o >= 128 ) ) 
	  fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
	} 
	else
	fprintf( stdout , "%c%s",  ' ' , "(illegal at level zero)!" ) ;
	else {
	    
	  s = s - 1 ;
	  hh = hhstack [s ];
	  vv = vvstack [s ];
	  h = hstack [s ];
	  v = vstack [s ];
	  w = wstack [s ];
	  x = xstack [s ];
	  y = ystack [s ];
	  z = zstack [s ];
	} 
	ss = s ;
	goto lab45 ;
      } 
      break ;
    case 143 : 
    case 144 : 
    case 145 : 
    case 146 : 
      {
	if ( ( p >= fontspace [curfont ]) || ( p <= -4 * fontspace [curfont 
	]) ) 
	{
	  outtext ( 32 ) ;
	  hh = round ( conv * ( h + p ) ) ;
	} 
	else hh = hh + round ( conv * ( p ) ) ;
	if ( outmode > 1 ) 
	{
	  showing = true ;
	  fprintf( stdout , "%ld%s%s%ld%c%ld",  (long)a , ": " , "right" , (long)o - 142 , ' ' , (long)p ) ;
	  if ( showopcodes && ( o >= 128 ) ) 
	  fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
	} 
	q = p ;
	goto lab43 ;
      } 
      break ;
    case 147 : 
    case 148 : 
    case 149 : 
    case 150 : 
    case 151 : 
      {
	w = p ;
	if ( ( p >= fontspace [curfont ]) || ( p <= -4 * fontspace [curfont 
	]) ) 
	{
	  outtext ( 32 ) ;
	  hh = round ( conv * ( h + p ) ) ;
	} 
	else hh = hh + round ( conv * ( p ) ) ;
	if ( outmode > 1 ) 
	{
	  showing = true ;
	  fprintf( stdout , "%ld%s%c%ld%c%ld",  (long)a , ": " , 'w' , (long)o - 147 , ' ' , (long)p ) ;
	  if ( showopcodes && ( o >= 128 ) ) 
	  fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
	} 
	q = p ;
	goto lab43 ;
      } 
      break ;
    case 152 : 
    case 153 : 
    case 154 : 
    case 155 : 
    case 156 : 
      {
	x = p ;
	if ( ( p >= fontspace [curfont ]) || ( p <= -4 * fontspace [curfont 
	]) ) 
	{
	  outtext ( 32 ) ;
	  hh = round ( conv * ( h + p ) ) ;
	} 
	else hh = hh + round ( conv * ( p ) ) ;
	if ( outmode > 1 ) 
	{
	  showing = true ;
	  fprintf( stdout , "%ld%s%c%ld%c%ld",  (long)a , ": " , 'x' , (long)o - 152 , ' ' , (long)p ) ;
	  if ( showopcodes && ( o >= 128 ) ) 
	  fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
	} 
	q = p ;
	goto lab43 ;
      } 
      break ;
      default: 
      if ( specialcases ( o , p , a ) ) 
      goto lab30 ;
      else goto lab9998 ;
      break ;
    } 
    lab41: if ( p < 0 ) 
    p = 255 - ( ( -1 - p ) % 256 ) ;
    else if ( p >= 256 ) 
    p = p % 256 ;
    if ( ( p < fontbc [curfont ]) || ( p > fontec [curfont ]) ) 
    q = 2147483647L ;
    else q = width [widthbase [curfont ]+ p ];
    if ( q == 2147483647L ) 
    {
      if ( ! showing ) 
      {
	flushtext () ;
	showing = true ;
	fprintf( stdout , "%ld%s%s%ld%s",  (long)a , ": " , "character " , (long)p , " invalid in font " ) ;
	if ( showopcodes && ( o >= 128 ) ) 
	fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
      } 
      else
      fprintf( stdout , "%c%s%ld%s",  ' ' , "character " , (long)p , " invalid in font " ) ;
      printfont ( curfont ) ;
      if ( curfont != maxfonts ) 
      putc ( '!' ,  stdout );
    } 
    if ( o >= 133 ) 
    goto lab30 ;
    if ( q == 2147483647L ) 
    q = 0 ;
    else hh = hh + pixelwidth [widthbase [curfont ]+ p ];
    goto lab43 ;
    lab42: q = signedquad () ;
    if ( showing ) 
    {
      fprintf( stdout , "%s%ld%s%ld",  " height " , (long)p , ", width " , (long)q ) ;
      if ( outmode > 2 ) 
      if ( ( p <= 0 ) || ( q <= 0 ) ) 
      Fputs( stdout ,  " (invisible)" ) ;
      else
      fprintf( stdout , "%s%ld%c%ld%s",  " (" , (long)rulepixels ( p ) , 'x' , (long)rulepixels ( q ) ,       " pixels)" ) ;
    } 
    if ( o == 137 ) 
    goto lab30 ;
    if ( showing ) 
    if ( outmode > 2 ) 
    fprintf( stdout , "%c\n",  ' ' ) ;
    hh = hh + rulepixels ( q ) ;
    goto lab43 ;
    lab43: if ( ( h > 0 ) && ( q > 0 ) ) 
    if ( h > 2147483647L - q ) 
    {
      if ( ! showing ) 
      {
	flushtext () ;
	showing = true ;
	fprintf( stdout , "%ld%s%s%ld%s%ld",  (long)a , ": " ,         "arithmetic overflow! parameter changed from " , (long)q , " to " ,         (long)2147483647L - h ) ;
	if ( showopcodes && ( o >= 128 ) ) 
	fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
      } 
      else
      fprintf( stdout , "%c%s%ld%s%ld",  ' ' ,       "arithmetic overflow! parameter changed from " , (long)q , " to " ,       (long)2147483647L - h ) ;
      q = 2147483647L - h ;
    } 
    if ( ( h < 0 ) && ( q < 0 ) ) 
    if ( - (integer) h > q + 2147483647L ) 
    {
      if ( ! showing ) 
      {
	flushtext () ;
	showing = true ;
	fprintf( stdout , "%ld%s%s%ld%s%ld",  (long)a , ": " ,         "arithmetic overflow! parameter changed from " , (long)q , " to " , (long)(         - (integer) h ) - 2147483647L ) ;
	if ( showopcodes && ( o >= 128 ) ) 
	fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
      } 
      else
      fprintf( stdout , "%c%s%ld%s%ld",  ' ' ,       "arithmetic overflow! parameter changed from " , (long)q , " to " , (long)(       - (integer) h ) - 2147483647L ) ;
      q = ( - (integer) h ) - 2147483647L ;
    } 
    hhh = round ( conv * ( h + q ) ) ;
    if ( abs ( hhh - hh ) > 2 ) 
    if ( hhh > hh ) 
    hh = hhh - 2 ;
    else hh = hhh + 2 ;
    if ( showing ) 
    if ( outmode > 2 ) 
    {
      fprintf( stdout , "%s%ld",  " h:=" , (long)h ) ;
      if ( q >= 0 ) 
      putc ( '+' ,  stdout );
      fprintf( stdout , "%ld%c%ld%s%ld",  (long)q , '=' , (long)h + q , ", hh:=" , (long)hh ) ;
    } 
    h = h + q ;
    if ( abs ( h ) > maxhsofar ) 
    {
      if ( abs ( h ) > maxh + 99 ) 
      {
	if ( ! showing ) 
	{
	  flushtext () ;
	  showing = true ;
	  fprintf( stdout , "%ld%s%s%ld%c",  (long)a , ": " , "warning: |h|>" , (long)maxh , '!' ) ;
	  if ( showopcodes && ( o >= 128 ) ) 
	  fprintf( stdout , "%s%ld%c",  " {" , (long)o , '}' ) ;
	} 
	else
	fprintf( stdout , "%c%s%ld%c",  ' ' , "warning: |h|>" , (long)maxh , '!' ) ;
	maxh = abs ( h ) ;
      } 
      maxhsofar = abs ( h ) ;
    } 
    goto lab30 ;
    lab45: if ( showing ) 
    if ( outmode > 2 ) 
    {
      fprintf( stdout , "%c\n",  ' ' ) ;
      fprintf( stdout , "%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%s%ld%c",  "level " , (long)ss , ":(h=" , (long)h , ",v=" , (long)v , ",w=" , (long)w ,       ",x=" , (long)x , ",y=" , (long)y , ",z=" , (long)z , ",hh=" , (long)hh , ",vv=" , (long)vv , ')' ) ;
    } 
    goto lab30 ;
    lab30: if ( showing ) 
    fprintf( stdout , "%c\n",  ' ' ) ;
  } 
  lab9998: fprintf( stdout , "%c\n",  '!' ) ;
  Result = false ;
  lab9999: ;
  return Result ;
} 
void 
#ifdef HAVE_PROTOTYPES
scanbop ( void ) 
#else
scanbop ( ) 
#endif
{
  unsigned char k  ;
  do {
      if ( eof ( dvifile ) ) 
    {
      fprintf( stderr , "%s%s%c\n",  "Bad DVI file: " , "the file ended prematurely" , '!'       ) ;
      uexit ( 1 ) ;
    } 
    k = getbyte () ;
    if ( ( k >= 243 ) && ( k < 247 ) ) 
    {
      definefont ( firstpar ( k ) ) ;
      k = 138 ;
    } 
  } while ( ! ( k != 138 ) ) ;
  if ( k == 248 ) 
  inpostamble = true ;
  else {
      
    if ( k != 139 ) 
    {
      fprintf( stderr , "%s%s%ld%s%c\n",  "Bad DVI file: " , "byte " , (long)curloc - 1 ,       " is not bop" , '!' ) ;
      uexit ( 1 ) ;
    } 
    newbackpointer = curloc - 1 ;
    pagecount = pagecount + 1 ;
    {register integer for_end; k = 0 ;for_end = 9 ; if ( k <= for_end) do 
      count [k ]= signedquad () ;
    while ( k++ < for_end ) ;} 
    if ( signedquad () != oldbackpointer ) 
    fprintf( stdout , "%s%ld%s%ld%c\n",  "backpointer in byte " , (long)curloc - 4 , " should be " ,     (long)oldbackpointer , '!' ) ;
    oldbackpointer = newbackpointer ;
  } 
} 
void 
#ifdef HAVE_PROTOTYPES
zskippages ( boolean bopseen ) 
#else
zskippages ( bopseen ) 
  boolean bopseen ;
#endif
{
  /* 9999 */ integer p  ;
  unsigned char k  ;
  integer downthedrain  ;
  showing = false ;
  while ( true ) {
      
    if ( ! bopseen ) 
    {
      scanbop () ;
      if ( inpostamble ) 
      goto lab9999 ;
      if ( ! started ) 
      if ( startmatch () ) 
      {
	started = true ;
	goto lab9999 ;
      } 
    } 
    do {
	if ( eof ( dvifile ) ) 
      {
	fprintf( stderr , "%s%s%c\n",  "Bad DVI file: " , "the file ended prematurely" ,         '!' ) ;
	uexit ( 1 ) ;
      } 
      k = getbyte () ;
      p = firstpar ( k ) ;
      switch ( k ) 
      {case 132 : 
      case 137 : 
	downthedrain = signedquad () ;
	break ;
      case 243 : 
      case 244 : 
      case 245 : 
      case 246 : 
	{
	  definefont ( p ) ;
	  fprintf( stdout , "%c\n",  ' ' ) ;
	} 
	break ;
      case 239 : 
      case 240 : 
      case 241 : 
      case 242 : 
	while ( p > 0 ) {
	    
	  downthedrain = getbyte () ;
	  p = p - 1 ;
	} 
	break ;
      case 139 : 
      case 247 : 
      case 248 : 
      case 249 : 
      case 250 : 
      case 251 : 
      case 252 : 
      case 253 : 
      case 254 : 
      case 255 : 
	{
	  fprintf( stderr , "%s%s%ld%c\n",  "Bad DVI file: " , "illegal command at byte " ,           (long)curloc - 1 , '!' ) ;
	  uexit ( 1 ) ;
	} 
	break ;
	default: 
	;
	break ;
      } 
    } while ( ! ( k == 140 ) ) ;
    bopseen = false ;
  } 
  lab9999: ;
} 
void 
#ifdef HAVE_PROTOTYPES
readpostamble ( void ) 
#else
readpostamble ( ) 
#endif
{
  integer k  ;
  integer p, q, m  ;
  showing = false ;
  postloc = curloc - 5 ;
  fprintf( stdout , "%s%ld%c\n",  "Postamble starts at byte " , (long)postloc , '.' ) ;
  if ( signedquad () != numerator ) 
  fprintf( stdout , "%s\n",  "numerator doesn't match the preamble!" ) ;
  if ( signedquad () != denominator ) 
  fprintf( stdout , "%s\n",  "denominator doesn't match the preamble!" ) ;
  if ( signedquad () != mag ) 
  if ( newmag == 0 ) 
  fprintf( stdout , "%s\n",  "magnification doesn't match the preamble!" ) ;
  maxv = signedquad () ;
  maxh = signedquad () ;
  fprintf( stdout , "%s%ld%s%ld",  "maxv=" , (long)maxv , ", maxh=" , (long)maxh ) ;
  maxs = gettwobytes () ;
  totalpages = gettwobytes () ;
  fprintf( stdout , "%s%ld%s%ld\n",  ", maxstackdepth=" , (long)maxs , ", totalpages=" , (long)totalpages   ) ;
  if ( outmode < 4 ) 
  {
    if ( maxv + 99 < maxvsofar ) 
    fprintf( stdout , "%s%ld\n",  "warning: observed maxv was " , (long)maxvsofar ) ;
    if ( maxh + 99 < maxhsofar ) 
    fprintf( stdout , "%s%ld\n",  "warning: observed maxh was " , (long)maxhsofar ) ;
    if ( maxs < maxssofar ) 
    fprintf( stdout , "%s%ld\n",  "warning: observed maxstackdepth was " , (long)maxssofar ) ;
    if ( pagecount != totalpages ) 
    fprintf( stdout , "%s%ld%s%ld%c\n",  "there are really " , (long)pagecount , " pages, not " ,     (long)totalpages , '!' ) ;
  } 
  do {
      k = getbyte () ;
    if ( ( k >= 243 ) && ( k < 247 ) ) 
    {
      p = firstpar ( k ) ;
      definefont ( p ) ;
      fprintf( stdout , "%c\n",  ' ' ) ;
      k = 138 ;
    } 
  } while ( ! ( k != 138 ) ) ;
  if ( k != 249 ) 
  fprintf( stdout , "%s%ld%s\n",  "byte " , (long)curloc - 1 , " is not postpost!" ) ;
  q = signedquad () ;
  if ( q != postloc ) 
  fprintf( stdout , "%s%ld%c\n",  "bad postamble pointer in byte " , (long)curloc - 4 , '!' ) ;
  m = getbyte () ;
  if ( m != 2 ) 
  fprintf( stdout , "%s%ld%s%ld%c\n",  "identification in byte " , (long)curloc - 1 , " should be " ,   (long)2 , '!' ) ;
  k = curloc ;
  m = 223 ;
  while ( ( m == 223 ) && ! eof ( dvifile ) ) m = getbyte () ;
  if ( ! eof ( dvifile ) ) 
  {
    fprintf( stderr , "%s%s%ld%s%c\n",  "Bad DVI file: " , "signature in byte " , (long)curloc - 1 ,     " should be 223" , '!' ) ;
    uexit ( 1 ) ;
  } 
  else if ( curloc < k + 4 ) 
  fprintf( stdout , "%s%ld%c\n",  "not enough signature bytes at end of file (" , (long)curloc -   k , ')' ) ;
} 
void mainbody() {
    
  initialize () ;
  fprintf( stdout , "%s\n",  "Options selected:" ) ;
  Fputs( stdout ,  "  Starting page = " ) ;
  {register integer for_end; k = 0 ;for_end = startvals ; if ( k <= for_end) 
  do 
    {
      if ( startthere [k ]) 
      fprintf( stdout , "%ld",  (long)startcount [k ]) ;
      else
      putc ( '*' ,  stdout );
      if ( k < startvals ) 
      putc ( '.' ,  stdout );
      else
      fprintf( stdout , "%c\n",  ' ' ) ;
    } 
  while ( k++ < for_end ) ;} 
  fprintf( stdout , "%s%ld\n",  "  Maximum number of pages = " , (long)maxpages ) ;
  fprintf( stdout , "%s%ld",  "  Output level = " , (long)outmode ) ;
  switch ( outmode ) 
  {case 0 : 
    fprintf( stdout , "%s\n",  " (showing bops, fonts, and error messages only)" ) ;
    break ;
  case 1 : 
    fprintf( stdout , "%s\n",  " (terse)" ) ;
    break ;
  case 2 : 
    fprintf( stdout , "%s\n",  " (mnemonics)" ) ;
    break ;
  case 3 : 
    fprintf( stdout , "%s\n",  " (verbose)" ) ;
    break ;
  case 4 : 
    if ( true ) 
    fprintf( stdout , "%s\n",  " (the works)" ) ;
    else {
	
      outmode = 3 ;
      fprintf( stdout , "%s\n",  " (the works: same as level 3 in this DVItype)" ) ;
    } 
    break ;
  } 
  Fputs( stdout ,  "  Resolution = " ) ;
  printreal ( resolution , 12 , 8 ) ;
  fprintf( stdout , "%s\n",  " pixels per inch" ) ;
  if ( newmag > 0 ) 
  {
    Fputs( stdout ,  "  New magnification factor = " ) ;
    printreal ( newmag / ((double) 1000.0 ) , 8 , 3 ) ;
    fprintf( stdout , "%s\n",  "" ) ;
  } 
  opendvifile () ;
  p = getbyte () ;
  if ( p != 247 ) 
  {
    fprintf( stderr , "%s%s%c\n",  "Bad DVI file: " ,     "First byte isn't start of preamble!" , '!' ) ;
    uexit ( 1 ) ;
  } 
  p = getbyte () ;
  if ( p != 2 ) 
  fprintf( stdout , "%s%ld%c\n",  "identification in byte 1 should be " , (long)2 , '!' ) ;
  numerator = signedquad () ;
  denominator = signedquad () ;
  if ( numerator <= 0 ) 
  {
    fprintf( stderr , "%s%s%ld%c\n",  "Bad DVI file: " , "numerator is " , (long)numerator , '!' ) 
    ;
    uexit ( 1 ) ;
  } 
  if ( denominator <= 0 ) 
  {
    fprintf( stderr , "%s%s%ld%c\n",  "Bad DVI file: " , "denominator is " , (long)denominator ,     '!' ) ;
    uexit ( 1 ) ;
  } 
  fprintf( stdout , "%s%ld%c%ld\n",  "numerator/denominator=" , (long)numerator , '/' , (long)denominator   ) ;
  tfmconv = ( 25400000.0 / ((double) numerator ) ) * ( denominator / ((double) 
  473628672L ) ) / ((double) 16.0 ) ;
  conv = ( numerator / ((double) 254000.0 ) ) * ( resolution / ((double) 
  denominator ) ) ;
  mag = signedquad () ;
  if ( newmag > 0 ) 
  mag = newmag ;
  else if ( mag <= 0 ) 
  {
    fprintf( stderr , "%s%s%ld%c\n",  "Bad DVI file: " , "magnification is " , (long)mag , '!' ) ;
    uexit ( 1 ) ;
  } 
  trueconv = conv ;
  conv = trueconv * ( mag / ((double) 1000.0 ) ) ;
  fprintf( stdout , "%s%ld%s",  "magnification=" , (long)mag , "; " ) ;
  printreal ( conv , 16 , 8 ) ;
  fprintf( stdout , "%s\n",  " pixels per DVI unit" ) ;
  p = getbyte () ;
  putc ( '\'' ,  stdout );
  while ( p > 0 ) {
      
    p = p - 1 ;
    putc ( xchr [getbyte () ],  stdout );
  } 
  fprintf( stdout , "%c\n",  '\'' ) ;
  afterpre = curloc ;
  if ( outmode == 4 ) 
  {
    n = dvilength () ;
    if ( n < 53 ) 
    {
      fprintf( stderr , "%s%s%ld%s%c\n",  "Bad DVI file: " , "only " , (long)n , " bytes long" , '!'       ) ;
      uexit ( 1 ) ;
    } 
    m = n - 4 ;
    do {
	if ( m == 0 ) 
      {
	fprintf( stderr , "%s%s%c\n",  "Bad DVI file: " , "all 223s" , '!' ) ;
	uexit ( 1 ) ;
      } 
      movetobyte ( m ) ;
      k = getbyte () ;
      m = m - 1 ;
    } while ( ! ( k != 223 ) ) ;
    if ( k != 2 ) 
    {
      fprintf( stderr , "%s%s%ld%c\n",  "Bad DVI file: " , "ID byte is " , (long)k , '!' ) ;
      uexit ( 1 ) ;
    } 
    movetobyte ( m - 3 ) ;
    q = signedquad () ;
    if ( ( q < 0 ) || ( q > m - 33 ) ) 
    {
      fprintf( stderr , "%s%s%ld%s%ld%c\n",  "Bad DVI file: " , "post pointer " , (long)q , " at byte "       , (long)m - 3 , '!' ) ;
      uexit ( 1 ) ;
    } 
    movetobyte ( q ) ;
    k = getbyte () ;
    if ( k != 248 ) 
    {
      fprintf( stderr , "%s%s%ld%s%c\n",  "Bad DVI file: " , "byte " , (long)q , " is not post" , '!'       ) ;
      uexit ( 1 ) ;
    } 
    postloc = q ;
    firstbackpointer = signedquad () ;
    inpostamble = true ;
    readpostamble () ;
    inpostamble = false ;
    q = postloc ;
    p = firstbackpointer ;
    startloc = -1 ;
    if ( p < 0 ) 
    inpostamble = true ;
    else {
	
      do {
	  if ( p > q - 46 ) 
	{
	  fprintf( stderr , "%s%s%ld%s%ld%c\n",  "Bad DVI file: " , "page link " , (long)p ,           " after byte " , (long)q , '!' ) ;
	  uexit ( 1 ) ;
	} 
	q = p ;
	movetobyte ( q ) ;
	k = getbyte () ;
	if ( k == 139 ) 
	pagecount = pagecount + 1 ;
	else {
	    
	  fprintf( stderr , "%s%s%ld%s%c\n",  "Bad DVI file: " , "byte " , (long)q , " is not bop" ,           '!' ) ;
	  uexit ( 1 ) ;
	} 
	{register integer for_end; k = 0 ;for_end = 9 ; if ( k <= for_end) 
	do 
	  count [k ]= signedquad () ;
	while ( k++ < for_end ) ;} 
	p = signedquad () ;
	if ( startmatch () ) 
	{
	  startloc = q ;
	  oldbackpointer = p ;
	} 
      } while ( ! ( p < 0 ) ) ;
      if ( startloc < 0 ) 
      {
	fprintf( stderr , "%s\n",  "starting page number could not be found!" ) ;
	uexit ( 1 ) ;
      } 
      if ( oldbackpointer < 0 ) 
      startloc = afterpre ;
      movetobyte ( startloc ) ;
    } 
    if ( pagecount != totalpages ) 
    fprintf( stdout , "%s%ld%s%ld%c\n",  "there are really " , (long)pagecount , " pages, not " ,     (long)totalpages , '!' ) ;
  } 
  skippages ( false ) ;
  if ( ! inpostamble ) 
  {
    while ( maxpages > 0 ) {
	
      maxpages = maxpages - 1 ;
      fprintf( stdout , "%c\n",  ' ' ) ;
      fprintf( stdout , "%ld%s",  (long)curloc - 45 , ": beginning of page " ) ;
      {register integer for_end; k = 0 ;for_end = startvals ; if ( k <= 
      for_end) do 
	{
	  fprintf( stdout , "%ld",  (long)count [k ]) ;
	  if ( k < startvals ) 
	  putc ( '.' ,  stdout );
	  else
	  fprintf( stdout , "%c\n",  ' ' ) ;
	} 
      while ( k++ < for_end ) ;} 
      if ( ! dopage () ) 
      {
	fprintf( stderr , "%s%s%c\n",  "Bad DVI file: " , "page ended unexpectedly" , '!'         ) ;
	uexit ( 1 ) ;
      } 
      scanbop () ;
      if ( inpostamble ) 
      goto lab30 ;
    } 
    lab30: ;
  } 
  if ( outmode < 4 ) 
  {
    if ( ! inpostamble ) 
    skippages ( true ) ;
    if ( signedquad () != oldbackpointer ) 
    fprintf( stdout , "%s%ld%s%ld%c\n",  "backpointer in byte " , (long)curloc - 4 , " should be " ,     (long)oldbackpointer , '!' ) ;
    readpostamble () ;
  } 
} 
