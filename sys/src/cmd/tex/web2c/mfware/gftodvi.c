#include "config.h"
#define maxlabels 2000 
#define poolsize 10000 
#define maxstrings 1100 
#define terminallinelength 150 
#define fontmemsize 2000 
#define dvibufsize 800 
#define widestrow 8192 
#define liglookahead 20 
#define noptions 2 
#define argoptions 1 
typedef integer scaled  ; 
typedef unsigned char ASCIIcode  ; 
typedef file_ptr /* of  ASCIIcode */ textfile  ; 
typedef unsigned char eightbits  ; 
typedef file_ptr /* of  eightbits */ bytefile  ; 
typedef integer fontindex  ; 
typedef unsigned char quarterword  ; 
typedef struct {
    quarterword B0 ; 
  quarterword B1 ; 
  quarterword B2 ; 
  quarterword B3 ; 
} fourquarters  ; 
#include "gftodmem.h"
typedef schar internalfontnumber  ; 
typedef integer poolpointer  ; 
typedef integer strnumber  ; 
typedef schar keywordcode  ; 
typedef integer dviindex  ; 
typedef integer nodepointer  ; 
ASCIIcode xord[256]  ; 
ASCIIcode xchr[256]  ; 
unsigned char buffer[terminallinelength + 1]  ; 
integer bufptr  ; 
integer linelength  ; 
short lf, lh, bc, ec, nw, nh, nd, ni, nl, nk, ne, np  ; 
bytefile gffile  ; 
bytefile dvifile  ; 
bytefile tfmfile  ; 
integer curloc  ; 
char nameoffile[PATHMAX + 1]  ; 
eightbits b0, b1, b2, b3  ; 
memoryword fontinfo[fontmemsize + 1]  ; 
fontindex fmemptr  ; 
fourquarters fontcheck[6]  ; 
scaled fontsize[6]  ; 
scaled fontdsize[6]  ; 
eightbits fontbc[6]  ; 
eightbits fontec[6]  ; 
integer charbase[6]  ; 
integer widthbase[6]  ; 
integer heightbase[6]  ; 
integer depthbase[6]  ; 
integer italicbase[6]  ; 
integer ligkernbase[6]  ; 
integer kernbase[6]  ; 
integer extenbase[6]  ; 
integer parambase[6]  ; 
fontindex bcharlabel[6]  ; 
short fontbchar[6]  ; 
ASCIIcode strpool[poolsize + 1]  ; 
poolpointer strstart[maxstrings + 1]  ; 
poolpointer poolptr  ; 
strnumber strptr  ; 
strnumber initstrptr  ; 
integer l  ; 
eightbits curgf  ; 
strnumber curstring  ; 
eightbits labeltype  ; 
strnumber curname  ; 
strnumber curarea  ; 
strnumber curext  ; 
poolpointer areadelimiter  ; 
poolpointer extdelimiter  ; 
strnumber jobname  ; 
boolean interaction  ; 
boolean fontsnotloaded  ; 
strnumber fontname[6]  ; 
strnumber fontarea[6]  ; 
scaled fontat[6]  ; 
integer totalpages  ; 
scaled maxv  ; 
scaled maxh  ; 
integer lastbop  ; 
eightbits dvibuf[dvibufsize + 1]  ; 
dviindex halfbuf  ; 
dviindex dvilimit  ; 
dviindex dviptr  ; 
integer dvioffset  ; 
scaled boxwidth  ; 
scaled boxheight  ; 
scaled boxdepth  ; 
quarterword ligstack[liglookahead + 1]  ; 
fourquarters dummyinfo  ; 
boolean suppresslig  ; 
short c[121]  ; 
short d[121]  ; 
short twotothe[14]  ; 
real ruleslant  ; 
integer slantn  ; 
real slantunit  ; 
real slantreported  ; 
scaled xl[maxlabels + 1], xr[maxlabels + 1], yt[maxlabels + 1], 
yb[maxlabels + 1]  ; 
scaled xx[maxlabels + 1], yy[maxlabels + 1]  ; 
nodepointer prev[maxlabels + 1], next[maxlabels + 1]  ; 
strnumber info[maxlabels + 1]  ; 
nodepointer maxnode  ; 
scaled maxheight  ; 
scaled maxdepth  ; 
nodepointer firstdot  ; 
boolean twin  ; 
scaled rulethickness  ; 
scaled offsetx, offsety  ; 
scaled xoffset, yoffset  ; 
scaled preminx, premaxx, preminy, premaxy  ; 
nodepointer ruleptr  ; 
nodepointer labeltail  ; 
nodepointer titlehead, titletail  ; 
integer charcode, ext  ; 
integer minx, maxx, miny, maxy  ; 
integer x, y  ; 
integer z  ; 
real xratio, yratio, slantratio  ; 
real unscxratio, unscyratio, unscslantratio  ; 
real fudgefactor  ; 
scaled deltax, deltay  ; 
scaled dvix, dviy  ; 
scaled overcol  ; 
scaled pageheight, pagewidth  ; 
scaled grayrulethickness  ; 
scaled tempx, tempy  ; 
integer overflowline  ; 
scaled delta  ; 
scaled halfxheight  ; 
scaled thricexheight  ; 
scaled dotwidth, dotheight  ; 
schar b[4096]  ; 
short rho[4096]  ; 
short a[widestrow + 1]  ; 
integer blankrows  ; 
integer k, m, p, q, r, s, t, dx, dy  ; 
strnumber timestamp  ; 
boolean uselogo  ; 
integer verbose  ; 
integer overflowlabeloffset  ; 
real offsetinpoints  ; 

#include "gftodvi.h"
void initialize ( ) 
{integer i, j, m, n  ; 
  getoptstruct longoptions[noptions + 1]  ; 
  integer getoptreturnval  ; 
  integer optionindex  ; 
  integer currentoption  ; 
  if ( argc > noptions + argoptions + 2 ) 
  {
    (void) fprintf( stdout , "%s\n",      "Usage: gftodvi [-verbose] [-overflow-label-offset=<real>] <gf file>." ) ; 
    uexit ( 1 ) ; 
  } 
  verbose = false ; 
  overflowlabeloffset = 10000000L ; 
  {
    currentoption = 0 ; 
    longoptions [ 0 ] .name = "verbose" ; 
    longoptions [ 0 ] .hasarg = 0 ; 
    longoptions [ 0 ] .flag = addressofint ( verbose ) ; 
    longoptions [ 0 ] .val = 1 ; 
    currentoption = currentoption + 1 ; 
    longoptions [ currentoption ] .name = "overflow-label-offset" ; 
    longoptions [ currentoption ] .hasarg = 1 ; 
    longoptions [ currentoption ] .flag = 0 ; 
    longoptions [ currentoption ] .val = 0 ; 
    currentoption = currentoption + 1 ; 
    longoptions [ currentoption ] .name = 0 ; 
    longoptions [ currentoption ] .hasarg = 0 ; 
    longoptions [ currentoption ] .flag = 0 ; 
    longoptions [ currentoption ] .val = 0 ; 
    do {
	getoptreturnval = getoptlongonly ( argc , gargv , "" , longoptions , 
      addressofint ( optionindex ) ) ; 
      if ( getoptreturnval != -1 ) 
      {
	if ( getoptreturnval == 63 ) 
	uexit ( 1 ) ; 
	if ( ( strcmp ( longoptions [ optionindex ] .name , 
	"overflow-label-offset" ) == 0 ) ) 
	{
	  offsetinpoints = atof ( optarg ) ; 
	  overflowlabeloffset = round ( offsetinpoints * 65536L ) ; 
	} 
	else ; 
      } 
    } while ( ! ( getoptreturnval == -1 ) ) ; 
  } 
  if ( verbose ) 
  (void) fprintf( stdout , "%s\n",  "This is GFtoDVI, C Version 3.0" ) ; 
  xchr [ 32 ] = ' ' ; 
  xchr [ 33 ] = '!' ; 
  xchr [ 34 ] = '"' ; 
  xchr [ 35 ] = '#' ; 
  xchr [ 36 ] = '$' ; 
  xchr [ 37 ] = '%' ; 
  xchr [ 38 ] = '&' ; 
  xchr [ 39 ] = '\'' ; 
  xchr [ 40 ] = '(' ; 
  xchr [ 41 ] = ')' ; 
  xchr [ 42 ] = '*' ; 
  xchr [ 43 ] = '+' ; 
  xchr [ 44 ] = ',' ; 
  xchr [ 45 ] = '-' ; 
  xchr [ 46 ] = '.' ; 
  xchr [ 47 ] = '/' ; 
  xchr [ 48 ] = '0' ; 
  xchr [ 49 ] = '1' ; 
  xchr [ 50 ] = '2' ; 
  xchr [ 51 ] = '3' ; 
  xchr [ 52 ] = '4' ; 
  xchr [ 53 ] = '5' ; 
  xchr [ 54 ] = '6' ; 
  xchr [ 55 ] = '7' ; 
  xchr [ 56 ] = '8' ; 
  xchr [ 57 ] = '9' ; 
  xchr [ 58 ] = ':' ; 
  xchr [ 59 ] = ';' ; 
  xchr [ 60 ] = '<' ; 
  xchr [ 61 ] = '=' ; 
  xchr [ 62 ] = '>' ; 
  xchr [ 63 ] = '?' ; 
  xchr [ 64 ] = '@' ; 
  xchr [ 65 ] = 'A' ; 
  xchr [ 66 ] = 'B' ; 
  xchr [ 67 ] = 'C' ; 
  xchr [ 68 ] = 'D' ; 
  xchr [ 69 ] = 'E' ; 
  xchr [ 70 ] = 'F' ; 
  xchr [ 71 ] = 'G' ; 
  xchr [ 72 ] = 'H' ; 
  xchr [ 73 ] = 'I' ; 
  xchr [ 74 ] = 'J' ; 
  xchr [ 75 ] = 'K' ; 
  xchr [ 76 ] = 'L' ; 
  xchr [ 77 ] = 'M' ; 
  xchr [ 78 ] = 'N' ; 
  xchr [ 79 ] = 'O' ; 
  xchr [ 80 ] = 'P' ; 
  xchr [ 81 ] = 'Q' ; 
  xchr [ 82 ] = 'R' ; 
  xchr [ 83 ] = 'S' ; 
  xchr [ 84 ] = 'T' ; 
  xchr [ 85 ] = 'U' ; 
  xchr [ 86 ] = 'V' ; 
  xchr [ 87 ] = 'W' ; 
  xchr [ 88 ] = 'X' ; 
  xchr [ 89 ] = 'Y' ; 
  xchr [ 90 ] = 'Z' ; 
  xchr [ 91 ] = '[' ; 
  xchr [ 92 ] = '\\' ; 
  xchr [ 93 ] = ']' ; 
  xchr [ 94 ] = '^' ; 
  xchr [ 95 ] = '_' ; 
  xchr [ 96 ] = '`' ; 
  xchr [ 97 ] = 'a' ; 
  xchr [ 98 ] = 'b' ; 
  xchr [ 99 ] = 'c' ; 
  xchr [ 100 ] = 'd' ; 
  xchr [ 101 ] = 'e' ; 
  xchr [ 102 ] = 'f' ; 
  xchr [ 103 ] = 'g' ; 
  xchr [ 104 ] = 'h' ; 
  xchr [ 105 ] = 'i' ; 
  xchr [ 106 ] = 'j' ; 
  xchr [ 107 ] = 'k' ; 
  xchr [ 108 ] = 'l' ; 
  xchr [ 109 ] = 'm' ; 
  xchr [ 110 ] = 'n' ; 
  xchr [ 111 ] = 'o' ; 
  xchr [ 112 ] = 'p' ; 
  xchr [ 113 ] = 'q' ; 
  xchr [ 114 ] = 'r' ; 
  xchr [ 115 ] = 's' ; 
  xchr [ 116 ] = 't' ; 
  xchr [ 117 ] = 'u' ; 
  xchr [ 118 ] = 'v' ; 
  xchr [ 119 ] = 'w' ; 
  xchr [ 120 ] = 'x' ; 
  xchr [ 121 ] = 'y' ; 
  xchr [ 122 ] = 'z' ; 
  xchr [ 123 ] = '{' ; 
  xchr [ 124 ] = '|' ; 
  xchr [ 125 ] = '}' ; 
  xchr [ 126 ] = '~' ; 
  {register integer for_end; i = 1 ; for_end = 31 ; if ( i <= for_end) do 
    xchr [ i ] = chr ( i ) ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 127 ; for_end = 255 ; if ( i <= for_end) do 
    xchr [ i ] = chr ( i ) ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 0 ; for_end = 255 ; if ( i <= for_end) do 
    xord [ chr ( i ) ] = 32 ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 1 ; for_end = 255 ; if ( i <= for_end) do 
    xord [ xchr [ i ] ] = i ; 
  while ( i++ < for_end ) ; } 
  xord [ '?' ] = 63 ; 
  fmemptr = 0 ; 
  interaction = false ; 
  fontsnotloaded = true ; 
  fontname [ 1 ] = 29 ; 
  fontname [ 2 ] = 30 ; 
  fontname [ 3 ] = 31 ; 
  fontname [ 4 ] = 0 ; 
  fontname [ 5 ] = 32 ; 
  {register integer for_end; k = 1 ; for_end = 5 ; if ( k <= for_end) do 
    {
      fontarea [ k ] = 0 ; 
      fontat [ k ] = 0 ; 
    } 
  while ( k++ < for_end ) ; } 
  totalpages = 0 ; 
  maxv = 0 ; 
  maxh = 0 ; 
  lastbop = -1 ; 
  halfbuf = dvibufsize / 2 ; 
  dvilimit = dvibufsize ; 
  dviptr = 0 ; 
  dvioffset = 0 ; 
  dummyinfo .B0 = 0 ; 
  dummyinfo .B1 = 0 ; 
  dummyinfo .B2 = 0 ; 
  dummyinfo .B3 = 0 ; 
  c [ 1 ] = 1 ; 
  d [ 1 ] = 2 ; 
  twotothe [ 0 ] = 1 ; 
  m = 1 ; 
  {register integer for_end; k = 1 ; for_end = 13 ; if ( k <= for_end) do 
    twotothe [ k ] = 2 * twotothe [ k - 1 ] ; 
  while ( k++ < for_end ) ; } 
  {register integer for_end; k = 2 ; for_end = 6 ; if ( k <= for_end) do 
    {
      n = twotothe [ k - 1 ] ; 
      {register integer for_end; j = 0 ; for_end = n - 1 ; if ( j <= for_end) 
      do 
	{
	  m = m + 1 ; 
	  c [ m ] = m ; 
	  d [ m ] = n + n ; 
	} 
      while ( j++ < for_end ) ; } 
    } 
  while ( k++ < for_end ) ; } 
  {register integer for_end; k = 7 ; for_end = 12 ; if ( k <= for_end) do 
    {
      n = twotothe [ k - 1 ] ; 
      {register integer for_end; j = k ; for_end = 1 ; if ( j >= for_end) do 
	{
	  m = m + 1 ; 
	  d [ m ] = n + n ; 
	  if ( j == k ) 
	  c [ m ] = n ; 
	  else c [ m ] = c [ m - 1 ] + twotothe [ j - 1 ] ; 
	} 
      while ( j-- > for_end ) ; } 
    } 
  while ( k++ < for_end ) ; } 
  yy [ 0 ] = -1073741824L ; 
  yy [ maxlabels ] = 1073741824L ; 
} 
void jumpout ( ) 
{uexit ( 0 ) ; 
} 
void inputln ( ) 
{flush ( stdout ) ; 
  if ( eoln ( stdin ) ) 
  readln ( stdin ) ; 
  linelength = 0 ; 
  while ( ( linelength < terminallinelength ) && ! eoln ( stdin ) ) {
      
    buffer [ linelength ] = xord [ getc ( stdin ) ] ; 
    linelength = linelength + 1 ; 
  } 
} 
void opengffile ( ) 
{if ( testreadaccess ( nameoffile , GFFILEPATH ) ) 
  {
    reset ( gffile , nameoffile ) ; 
  } 
  else {
      
    printpascalstring ( nameoffile ) ; 
    {
      (void) fprintf( stdout , "%s\n",  ": GF file not found." ) ; 
      uexit ( 1 ) ; 
    } 
  } 
  curloc = 0 ; 
} 
void opentfmfile ( ) 
{if ( testreadaccess ( nameoffile , TFMFILEPATH ) ) 
  {
    reset ( tfmfile , nameoffile ) ; 
  } 
  else {
      
    printpascalstring ( nameoffile ) ; 
    {
      (void) fprintf( stdout , "%s\n",  ": TFM file not found." ) ; 
      uexit ( 1 ) ; 
    } 
  } 
} 
void opendvifile ( ) 
{rewrite ( dvifile , nameoffile ) ; 
} 
void readtfmword ( ) 
{read ( tfmfile , b0 ) ; 
  read ( tfmfile , b1 ) ; 
  read ( tfmfile , b2 ) ; 
  read ( tfmfile , b3 ) ; 
} 
integer getbyte ( ) 
{register integer Result; eightbits b  ; 
  if ( eof ( gffile ) ) 
  Result = 0 ; 
  else {
      
    read ( gffile , b ) ; 
    curloc = curloc + 1 ; 
    Result = b ; 
  } 
  return(Result) ; 
} 
integer gettwobytes ( ) 
{register integer Result; eightbits a, b  ; 
  read ( gffile , a ) ; 
  read ( gffile , b ) ; 
  curloc = curloc + 2 ; 
  Result = a * toint ( 256 ) + b ; 
  return(Result) ; 
} 
integer getthreebytes ( ) 
{register integer Result; eightbits a, b, c  ; 
  read ( gffile , a ) ; 
  read ( gffile , b ) ; 
  read ( gffile , c ) ; 
  curloc = curloc + 3 ; 
  Result = ( a * toint ( 256 ) + b ) * 256 + c ; 
  return(Result) ; 
} 
integer signedquad ( ) 
{register integer Result; eightbits a, b, c, d  ; 
  read ( gffile , a ) ; 
  read ( gffile , b ) ; 
  read ( gffile , c ) ; 
  read ( gffile , d ) ; 
  curloc = curloc + 4 ; 
  if ( a < 128 ) 
  Result = ( ( a * toint ( 256 ) + b ) * 256 + c ) * 256 + d ; 
  else Result = ( ( ( a - 256 ) * toint ( 256 ) + b ) * 256 + c ) * 256 + d ; 
  return(Result) ; 
} 
void zreadfontinfo ( f , s ) 
integer f ; 
scaled s ; 
{/* 30 11 */ fontindex k  ; 
  unsigned short lf, lh, bc, ec, nw, nh, nd, ni, nl, nk, ne, np  ; 
  integer bchlabel  ; 
  short bchar  ; 
  fourquarters qw  ; 
  scaled sw  ; 
  scaled z  ; 
  integer alpha  ; 
  schar beta  ; 
  {
    readtfmword () ; 
    lf = b0 * toint ( 256 ) + b1 ; 
    lh = b2 * toint ( 256 ) + b3 ; 
    readtfmword () ; 
    bc = b0 * toint ( 256 ) + b1 ; 
    ec = b2 * toint ( 256 ) + b3 ; 
    if ( ( bc > ec + 1 ) || ( ec > 255 ) ) 
    goto lab11 ; 
    if ( bc > 255 ) 
    {
      bc = 1 ; 
      ec = 0 ; 
    } 
    readtfmword () ; 
    nw = b0 * toint ( 256 ) + b1 ; 
    nh = b2 * toint ( 256 ) + b3 ; 
    readtfmword () ; 
    nd = b0 * toint ( 256 ) + b1 ; 
    ni = b2 * toint ( 256 ) + b3 ; 
    readtfmword () ; 
    nl = b0 * toint ( 256 ) + b1 ; 
    nk = b2 * toint ( 256 ) + b3 ; 
    readtfmword () ; 
    ne = b0 * toint ( 256 ) + b1 ; 
    np = b2 * toint ( 256 ) + b3 ; 
    if ( lf != 6 + lh + ( ec - bc + 1 ) + nw + nh + nd + ni + nl + nk + ne + 
    np ) 
    goto lab11 ; 
  } 
  lf = lf - 6 - lh ; 
  if ( np < 8 ) 
  lf = lf + 8 - np ; 
  if ( fmemptr + lf > fontmemsize ) 
  {
    (void) fprintf( stdout , "%s\n",  "No room for TFM file!" ) ; 
    uexit ( 1 ) ; 
  } 
  charbase [ f ] = fmemptr - bc ; 
  widthbase [ f ] = charbase [ f ] + ec + 1 ; 
  heightbase [ f ] = widthbase [ f ] + nw ; 
  depthbase [ f ] = heightbase [ f ] + nh ; 
  italicbase [ f ] = depthbase [ f ] + nd ; 
  ligkernbase [ f ] = italicbase [ f ] + ni ; 
  kernbase [ f ] = ligkernbase [ f ] + nl ; 
  extenbase [ f ] = kernbase [ f ] + nk ; 
  parambase [ f ] = extenbase [ f ] + ne ; 
  {
    if ( lh < 2 ) 
    goto lab11 ; 
    {
      readtfmword () ; 
      qw .B0 = b0 + 0 ; 
      qw .B1 = b1 + 0 ; 
      qw .B2 = b2 + 0 ; 
      qw .B3 = b3 + 0 ; 
      fontcheck [ f ] = qw ; 
    } 
    readtfmword () ; 
    if ( b0 > 127 ) 
    goto lab11 ; 
    z = ( ( b0 * toint ( 256 ) + b1 ) * toint ( 256 ) + b2 ) * 16 + ( b3 / 16 
    ) ; 
    if ( z < 65536L ) 
    goto lab11 ; 
    while ( lh > 2 ) {
	
      readtfmword () ; 
      lh = lh - 1 ; 
    } 
    fontdsize [ f ] = z ; 
    if ( s > 0 ) 
    z = s ; 
    fontsize [ f ] = z ; 
  } 
  {register integer for_end; k = fmemptr ; for_end = widthbase [ f ] - 1 
  ; if ( k <= for_end) do 
    {
      {
	readtfmword () ; 
	qw .B0 = b0 + 0 ; 
	qw .B1 = b1 + 0 ; 
	qw .B2 = b2 + 0 ; 
	qw .B3 = b3 + 0 ; 
	fontinfo [ k ] .qqqq = qw ; 
      } 
      if ( ( b0 >= nw ) || ( b1 / 16 >= nh ) || ( b1 % 16 >= nd ) || ( b2 / 4 
      >= ni ) ) 
      goto lab11 ; 
      switch ( b2 % 4 ) 
      {case 1 : 
	if ( b3 >= nl ) 
	goto lab11 ; 
	break ; 
      case 3 : 
	if ( b3 >= ne ) 
	goto lab11 ; 
	break ; 
      case 0 : 
      case 2 : 
	; 
	break ; 
      } 
    } 
  while ( k++ < for_end ) ; } 
  {
    {
      alpha = 16 * z ; 
      beta = 16 ; 
      while ( z >= 8388608L ) {
	  
	z = z / 2 ; 
	beta = beta / 2 ; 
      } 
    } 
    {register integer for_end; k = widthbase [ f ] ; for_end = ligkernbase [ 
    f ] - 1 ; if ( k <= for_end) do 
      {
	readtfmword () ; 
	sw = ( ( ( ( ( b3 * z ) / 256 ) + ( b2 * z ) ) / 256 ) + ( b1 * z ) ) 
	/ beta ; 
	if ( b0 == 0 ) 
	fontinfo [ k ] .sc = sw ; 
	else if ( b0 == 255 ) 
	fontinfo [ k ] .sc = sw - alpha ; 
	else goto lab11 ; 
      } 
    while ( k++ < for_end ) ; } 
    if ( fontinfo [ widthbase [ f ] ] .sc != 0 ) 
    goto lab11 ; 
    if ( fontinfo [ heightbase [ f ] ] .sc != 0 ) 
    goto lab11 ; 
    if ( fontinfo [ depthbase [ f ] ] .sc != 0 ) 
    goto lab11 ; 
    if ( fontinfo [ italicbase [ f ] ] .sc != 0 ) 
    goto lab11 ; 
  } 
  {
    bchlabel = 32767 ; 
    bchar = 256 ; 
    if ( nl > 0 ) 
    {
      {register integer for_end; k = ligkernbase [ f ] ; for_end = kernbase [ 
      f ] - 1 ; if ( k <= for_end) do 
	{
	  {
	    readtfmword () ; 
	    qw .B0 = b0 + 0 ; 
	    qw .B1 = b1 + 0 ; 
	    qw .B2 = b2 + 0 ; 
	    qw .B3 = b3 + 0 ; 
	    fontinfo [ k ] .qqqq = qw ; 
	  } 
	  if ( b0 > 128 ) 
	  {
	    if ( 256 * b2 + b3 >= nl ) 
	    goto lab11 ; 
	    if ( b0 == 255 ) 
	    if ( k == ligkernbase [ f ] ) 
	    bchar = b1 ; 
	  } 
	  else {
	      
	    if ( b1 != bchar ) 
	    {
	      if ( ( b1 < bc ) || ( b1 > ec ) ) 
	      goto lab11 ; 
	    } 
	    if ( b2 < 128 ) 
	    {
	      if ( ( b3 < bc ) || ( b3 > ec ) ) 
	      goto lab11 ; 
	    } 
	    else if ( toint ( 256 ) * ( b2 - 128 ) + b3 >= nk ) 
	    goto lab11 ; 
	  } 
	} 
      while ( k++ < for_end ) ; } 
      if ( b0 == 255 ) 
      bchlabel = 256 * b2 + b3 ; 
    } 
    {register integer for_end; k = kernbase [ f ] ; for_end = extenbase [ f ] 
    - 1 ; if ( k <= for_end) do 
      {
	readtfmword () ; 
	sw = ( ( ( ( ( b3 * z ) / 256 ) + ( b2 * z ) ) / 256 ) + ( b1 * z ) ) 
	/ beta ; 
	if ( b0 == 0 ) 
	fontinfo [ k ] .sc = sw ; 
	else if ( b0 == 255 ) 
	fontinfo [ k ] .sc = sw - alpha ; 
	else goto lab11 ; 
      } 
    while ( k++ < for_end ) ; } 
  } 
  {register integer for_end; k = extenbase [ f ] ; for_end = parambase [ f ] 
  - 1 ; if ( k <= for_end) do 
    {
      {
	readtfmword () ; 
	qw .B0 = b0 + 0 ; 
	qw .B1 = b1 + 0 ; 
	qw .B2 = b2 + 0 ; 
	qw .B3 = b3 + 0 ; 
	fontinfo [ k ] .qqqq = qw ; 
      } 
      if ( b0 != 0 ) 
      {
	if ( ( b0 < bc ) || ( b0 > ec ) ) 
	goto lab11 ; 
      } 
      if ( b1 != 0 ) 
      {
	if ( ( b1 < bc ) || ( b1 > ec ) ) 
	goto lab11 ; 
      } 
      if ( b2 != 0 ) 
      {
	if ( ( b2 < bc ) || ( b2 > ec ) ) 
	goto lab11 ; 
      } 
      {
	if ( ( b3 < bc ) || ( b3 > ec ) ) 
	goto lab11 ; 
      } 
    } 
  while ( k++ < for_end ) ; } 
  {
    {register integer for_end; k = 1 ; for_end = np ; if ( k <= for_end) do 
      if ( k == 1 ) 
      {
	readtfmword () ; 
	if ( b0 > 127 ) 
	sw = b0 - 256 ; 
	else sw = b0 ; 
	sw = sw * 256 + b1 ; 
	sw = sw * 256 + b2 ; 
	fontinfo [ parambase [ f ] ] .sc = ( sw * 16 ) + ( b3 / 16 ) ; 
      } 
      else {
	  
	readtfmword () ; 
	sw = ( ( ( ( ( b3 * z ) / 256 ) + ( b2 * z ) ) / 256 ) + ( b1 * z ) ) 
	/ beta ; 
	if ( b0 == 0 ) 
	fontinfo [ parambase [ f ] + k - 1 ] .sc = sw ; 
	else if ( b0 == 255 ) 
	fontinfo [ parambase [ f ] + k - 1 ] .sc = sw - alpha ; 
	else goto lab11 ; 
      } 
    while ( k++ < for_end ) ; } 
    {register integer for_end; k = np + 1 ; for_end = 8 ; if ( k <= for_end) 
    do 
      fontinfo [ parambase [ f ] + k - 1 ] .sc = 0 ; 
    while ( k++ < for_end ) ; } 
  } 
  fontbc [ f ] = bc ; 
  fontec [ f ] = ec ; 
  if ( bchlabel < nl ) 
  bcharlabel [ f ] = bchlabel + ligkernbase [ f ] ; 
  else bcharlabel [ f ] = fontmemsize ; 
  fontbchar [ f ] = bchar + 0 ; 
  widthbase [ f ] = widthbase [ f ] - 0 ; 
  ligkernbase [ f ] = ligkernbase [ f ] - 0 ; 
  kernbase [ f ] = kernbase [ f ] - 0 ; 
  extenbase [ f ] = extenbase [ f ] - 0 ; 
  parambase [ f ] = parambase [ f ] - 1 ; 
  fmemptr = fmemptr + lf ; 
  goto lab30 ; 
  lab11: {
      
    (void) putc('\n',  stdout );
    (void) Fputs( stdout ,  "Bad TFM file for" ) ; 
  } 
  switch ( f ) 
  {case 1 : 
    {
      (void) fprintf( stdout , "%s\n",  "titles!" ) ; 
      uexit ( 1 ) ; 
    } 
    break ; 
  case 2 : 
    {
      (void) fprintf( stdout , "%s\n",  "labels!" ) ; 
      uexit ( 1 ) ; 
    } 
    break ; 
  case 3 : 
    {
      (void) fprintf( stdout , "%s\n",  "pixels!" ) ; 
      uexit ( 1 ) ; 
    } 
    break ; 
  case 4 : 
    {
      (void) fprintf( stdout , "%s\n",  "slants!" ) ; 
      uexit ( 1 ) ; 
    } 
    break ; 
  case 5 : 
    {
      (void) fprintf( stdout , "%s\n",  "METAFONT logo!" ) ; 
      uexit ( 1 ) ; 
    } 
    break ; 
  } 
  lab30: ; 
} 
strnumber makestring ( ) 
{register strnumber Result; if ( strptr == maxstrings ) 
  {
    (void) fprintf( stdout , "%s\n",  "Too many labels!" ) ; 
    uexit ( 1 ) ; 
  } 
  strptr = strptr + 1 ; 
  strstart [ strptr ] = poolptr ; 
  Result = strptr - 1 ; 
  return(Result) ; 
} 
void zfirststring ( c ) 
integer c ; 
{if ( strptr != c ) 
  {
    (void) fprintf( stdout , "%c\n",  '?' ) ; 
    uexit ( 1 ) ; 
  } 
  while ( l > 0 ) {
      
    {
      strpool [ poolptr ] = buffer [ l ] ; 
      poolptr = poolptr + 1 ; 
    } 
    l = l - 1 ; 
  } 
  strptr = strptr + 1 ; 
  strstart [ strptr ] = poolptr ; 
} 
keywordcode interpretxxx ( ) 
{/* 30 31 45 */ register keywordcode Result; integer k  ; 
  integer j  ; 
  schar l  ; 
  keywordcode m  ; 
  schar n1  ; 
  poolpointer n2  ; 
  keywordcode c  ; 
  c = 19 ; 
  curstring = 0 ; 
  switch ( curgf ) 
  {case 244 : 
    goto lab30 ; 
    break ; 
  case 243 : 
    {
      k = signedquad () ; 
      goto lab30 ; 
    } 
    break ; 
  case 239 : 
    k = getbyte () ; 
    break ; 
  case 240 : 
    k = gettwobytes () ; 
    break ; 
  case 241 : 
    k = getthreebytes () ; 
    break ; 
  case 242 : 
    k = signedquad () ; 
    break ; 
  } 
  j = 0 ; 
  if ( k < 2 ) 
  goto lab45 ; 
  while ( true ) {
      
    l = j ; 
    if ( j == k ) 
    goto lab31 ; 
    if ( j == 13 ) 
    goto lab45 ; 
    j = j + 1 ; 
    buffer [ j ] = getbyte () ; 
    if ( buffer [ j ] == 32 ) 
    goto lab31 ; 
  } 
  lab31: {
      register integer for_end; m = 0 ; for_end = 18 ; if ( m <= for_end) 
  do 
    if ( ( strstart [ m + 1 ] - strstart [ m ] ) == l ) 
    {
      n1 = 0 ; 
      n2 = strstart [ m ] ; 
      while ( ( n1 < l ) && ( buffer [ n1 + 1 ] == strpool [ n2 ] ) ) {
	  
	n1 = n1 + 1 ; 
	n2 = n2 + 1 ; 
      } 
      if ( n1 == l ) 
      {
	c = m ; 
	if ( m == 0 ) 
	{
	  j = j + 1 ; 
	  labeltype = getbyte () ; 
	} 
	{
	  if ( poolptr + k - j > poolsize ) 
	  {
	    (void) fprintf( stdout , "%s\n",  "Too many strings!" ) ; 
	    uexit ( 1 ) ; 
	  } 
	} 
	while ( j < k ) {
	    
	  j = j + 1 ; 
	  {
	    strpool [ poolptr ] = getbyte () ; 
	    poolptr = poolptr + 1 ; 
	  } 
	} 
	curstring = makestring () ; 
	goto lab30 ; 
      } 
    } 
  while ( m++ < for_end ) ; } 
  lab45: while ( j < k ) {
      
    j = j + 1 ; 
    curgf = getbyte () ; 
  } 
  lab30: curgf = getbyte () ; 
  Result = c ; 
  return(Result) ; 
} 
scaled getyyy ( ) 
{register scaled Result; scaled v  ; 
  if ( curgf != 243 ) 
  Result = 0 ; 
  else {
      
    v = signedquad () ; 
    curgf = getbyte () ; 
    Result = v ; 
  } 
  return(Result) ; 
} 
void skipnop ( ) 
{/* 30 */ integer k  ; 
  integer j  ; 
  switch ( curgf ) 
  {case 244 : 
    goto lab30 ; 
    break ; 
  case 243 : 
    {
      k = signedquad () ; 
      goto lab30 ; 
    } 
    break ; 
  case 239 : 
    k = getbyte () ; 
    break ; 
  case 240 : 
    k = gettwobytes () ; 
    break ; 
  case 241 : 
    k = getthreebytes () ; 
    break ; 
  case 242 : 
    k = signedquad () ; 
    break ; 
  } 
  {register integer for_end; j = 1 ; for_end = k ; if ( j <= for_end) do 
    curgf = getbyte () ; 
  while ( j++ < for_end ) ; } 
  lab30: curgf = getbyte () ; 
} 
void beginname ( ) 
{areadelimiter = 0 ; 
  extdelimiter = 0 ; 
} 
boolean zmorename ( c ) 
ASCIIcode c ; 
{register boolean Result; if ( c == 32 ) 
  Result = false ; 
  else {
      
    if ( ( c == 47 ) ) 
    {
      areadelimiter = poolptr ; 
      extdelimiter = 0 ; 
    } 
    else if ( c == 46 ) 
    extdelimiter = poolptr ; 
    {
      if ( poolptr + 1 > poolsize ) 
      {
	(void) fprintf( stdout , "%s\n",  "Too many strings!" ) ; 
	uexit ( 1 ) ; 
      } 
    } 
    {
      strpool [ poolptr ] = c ; 
      poolptr = poolptr + 1 ; 
    } 
    Result = true ; 
  } 
  return(Result) ; 
} 
void endname ( ) 
{if ( strptr + 3 > maxstrings ) 
  {
    (void) fprintf( stdout , "%s\n",  "Too many strings!" ) ; 
    uexit ( 1 ) ; 
  } 
  if ( areadelimiter == 0 ) 
  curarea = 0 ; 
  else {
      
    curarea = strptr ; 
    strptr = strptr + 1 ; 
    strstart [ strptr ] = areadelimiter + 1 ; 
  } 
  if ( extdelimiter == 0 ) 
  {
    curext = 0 ; 
    curname = makestring () ; 
  } 
  else {
      
    curname = strptr ; 
    strptr = strptr + 1 ; 
    strstart [ strptr ] = extdelimiter ; 
    curext = makestring () ; 
  } 
} 
void zpackfilename ( n , a , e ) 
strnumber n ; 
strnumber a ; 
strnumber e ; 
{integer k  ; 
  ASCIIcode c  ; 
  integer j  ; 
  integer namelength  ; 
  k = 0 ; 
  {register integer for_end; j = strstart [ a ] ; for_end = strstart [ a + 1 
  ] - 1 ; if ( j <= for_end) do 
    {
      c = strpool [ j ] ; 
      k = k + 1 ; 
      if ( k <= PATHMAX ) 
      nameoffile [ k ] = xchr [ c ] ; 
    } 
  while ( j++ < for_end ) ; } 
  {register integer for_end; j = strstart [ n ] ; for_end = strstart [ n + 1 
  ] - 1 ; if ( j <= for_end) do 
    {
      c = strpool [ j ] ; 
      k = k + 1 ; 
      if ( k <= PATHMAX ) 
      nameoffile [ k ] = xchr [ c ] ; 
    } 
  while ( j++ < for_end ) ; } 
  {register integer for_end; j = strstart [ e ] ; for_end = strstart [ e + 1 
  ] - 1 ; if ( j <= for_end) do 
    {
      c = strpool [ j ] ; 
      k = k + 1 ; 
      if ( k <= PATHMAX ) 
      nameoffile [ k ] = xchr [ c ] ; 
    } 
  while ( j++ < for_end ) ; } 
  if ( k <= PATHMAX ) 
  namelength = k ; 
  else namelength = PATHMAX ; 
  {register integer for_end; k = namelength + 1 ; for_end = PATHMAX ; if ( k 
  <= for_end) do 
    nameoffile [ k ] = ' ' ; 
  while ( k++ < for_end ) ; } 
} 
void startgf ( ) 
{/* 30 */ char argbuffer[PATHMAX + 1]  ; 
  integer argbufptr  ; 
  if ( optind == argc ) 
  {
    (void) Fputs( stdout ,  "GF file name: " ) ; 
    inputln () ; 
  } 
  else {
      
    argv ( optind , argbuffer ) ; 
    argbuffer [ PATHMAX ] = ' ' ; 
    argbufptr = 1 ; 
    linelength = 0 ; 
    while ( ( argbufptr < PATHMAX ) && ( argbuffer [ argbufptr ] == ' ' ) ) 
    argbufptr = argbufptr + 1 ; 
    while ( ( argbufptr < PATHMAX ) && ( linelength < terminallinelength ) && 
    ( argbuffer [ argbufptr ] != ' ' ) ) {
	
      buffer [ linelength ] = xord [ argbuffer [ argbufptr ] ] ; 
      linelength = linelength + 1 ; 
      argbufptr = argbufptr + 1 ; 
    } 
  } 
  bufptr = 0 ; 
  buffer [ linelength ] = 63 ; 
  while ( buffer [ bufptr ] == 32 ) bufptr = bufptr + 1 ; 
  if ( bufptr < linelength ) 
  {
    if ( buffer [ linelength - 1 ] == 47 ) 
    {
      interaction = true ; 
      linelength = linelength - 1 ; 
    } 
    beginname () ; 
    while ( true ) {
	
      if ( bufptr == linelength ) 
      goto lab30 ; 
      if ( ! morename ( buffer [ bufptr ] ) ) 
      goto lab30 ; 
      bufptr = bufptr + 1 ; 
    } 
    lab30: endname () ; 
    if ( curext == 0 ) 
    curext = 19 ; 
    packfilename ( curname , curarea , curext ) ; 
    opengffile () ; 
  } 
  jobname = curname ; 
  packfilename ( jobname , 0 , 20 ) ; 
  opendvifile () ; 
} 
void zwritedvi ( a , b ) 
dviindex a ; 
dviindex b ; 
{writechunk ( dvifile , dvibuf , a , b ) ; 
} 
void dviswap ( ) 
{if ( dvilimit == dvibufsize ) 
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
} 
void zdvifour ( x ) 
integer x ; 
{if ( x >= 0 ) 
  {
    dvibuf [ dviptr ] = x / 16777216L ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  else {
      
    x = x + 1073741824L ; 
    x = x + 1073741824L ; 
    {
      dvibuf [ dviptr ] = ( x / 16777216L ) + 128 ; 
      dviptr = dviptr + 1 ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
  } 
  x = x % 16777216L ; 
  {
    dvibuf [ dviptr ] = x / 65536L ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  x = x % 65536L ; 
  {
    dvibuf [ dviptr ] = x / 256 ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {
    dvibuf [ dviptr ] = x % 256 ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
} 
void zdvifontdef ( f ) 
internalfontnumber f ; 
{integer k  ; 
  {
    dvibuf [ dviptr ] = 243 ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {
    dvibuf [ dviptr ] = f ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {
    dvibuf [ dviptr ] = fontcheck [ f ] .B0 - 0 ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {
    dvibuf [ dviptr ] = fontcheck [ f ] .B1 - 0 ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {
    dvibuf [ dviptr ] = fontcheck [ f ] .B2 - 0 ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {
    dvibuf [ dviptr ] = fontcheck [ f ] .B3 - 0 ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  dvifour ( fontsize [ f ] ) ; 
  dvifour ( fontdsize [ f ] ) ; 
  {
    dvibuf [ dviptr ] = ( strstart [ fontarea [ f ] + 1 ] - strstart [ 
    fontarea [ f ] ] ) ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {
    dvibuf [ dviptr ] = ( strstart [ fontname [ f ] + 1 ] - strstart [ 
    fontname [ f ] ] ) ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {register integer for_end; k = strstart [ fontarea [ f ] ] ; for_end = 
  strstart [ fontarea [ f ] + 1 ] - 1 ; if ( k <= for_end) do 
    {
      dvibuf [ dviptr ] = strpool [ k ] ; 
      dviptr = dviptr + 1 ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
  while ( k++ < for_end ) ; } 
  {register integer for_end; k = strstart [ fontname [ f ] ] ; for_end = 
  strstart [ fontname [ f ] + 1 ] - 1 ; if ( k <= for_end) do 
    {
      dvibuf [ dviptr ] = strpool [ k ] ; 
      dviptr = dviptr + 1 ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
  while ( k++ < for_end ) ; } 
} 
void loadfonts ( ) 
{/* 30 22 40 45 */ internalfontnumber f  ; 
  fourquarters i  ; 
  integer j, k, v  ; 
  schar m  ; 
  schar n1  ; 
  poolpointer n2  ; 
  if ( interaction ) 
  while ( true ) {
      
    lab45: {
	
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "Special font substitution: " ) ; 
    } 
    lab22: inputln () ; 
    if ( linelength == 0 ) 
    goto lab30 ; 
    bufptr = 0 ; 
    buffer [ linelength ] = 32 ; 
    while ( buffer [ bufptr ] != 32 ) bufptr = bufptr + 1 ; 
    {register integer for_end; m = 1 ; for_end = 8 ; if ( m <= for_end) do 
      if ( ( strstart [ m + 1 ] - strstart [ m ] ) == bufptr ) 
      {
	n1 = 0 ; 
	n2 = strstart [ m ] ; 
	while ( ( n1 < bufptr ) && ( buffer [ n1 ] == strpool [ n2 ] ) ) {
	    
	  n1 = n1 + 1 ; 
	  n2 = n2 + 1 ; 
	} 
	if ( n1 == bufptr ) 
	goto lab40 ; 
      } 
    while ( m++ < for_end ) ; } 
    (void) Fputs( stdout ,      "Please say, e.g., \"grayfont foo\" or \"slantfontarea baz\"." ) ; 
    goto lab45 ; 
    lab40: bufptr = bufptr + 1 ; 
    {
      if ( poolptr + linelength - bufptr > poolsize ) 
      {
	(void) fprintf( stdout , "%s\n",  "Too many strings!" ) ; 
	uexit ( 1 ) ; 
      } 
    } 
    while ( bufptr < linelength ) {
	
      {
	strpool [ poolptr ] = buffer [ bufptr ] ; 
	poolptr = poolptr + 1 ; 
      } 
      bufptr = bufptr + 1 ; 
    } 
    if ( m > 4 ) 
    fontarea [ m - 4 ] = makestring () ; 
    else {
	
      fontname [ m ] = makestring () ; 
      fontarea [ m ] = 0 ; 
      fontat [ m ] = 0 ; 
    } 
    initstrptr = strptr ; 
    (void) Fputs( stdout ,  "OK; any more? " ) ; 
    goto lab22 ; 
  } 
  lab30: ; 
  fontsnotloaded = false ; 
  {register integer for_end; f = 1 ; for_end = 5 ; if ( f <= for_end) do 
    if ( ( f != 4 ) || ( ( strstart [ fontname [ f ] + 1 ] - strstart [ 
    fontname [ f ] ] ) > 0 ) ) 
    {
      if ( ( strstart [ fontarea [ f ] + 1 ] - strstart [ fontarea [ f ] ] ) 
      == 0 ) 
      fontarea [ f ] = 34 ; 
      packfilename ( fontname [ f ] , fontarea [ f ] , 21 ) ; 
      opentfmfile () ; 
      readfontinfo ( f , fontat [ f ] ) ; 
      if ( fontarea [ f ] == 34 ) 
      fontarea [ f ] = 0 ; 
      dvifontdef ( f ) ; 
    } 
  while ( f++ < for_end ) ; } 
  if ( ( strstart [ fontname [ 4 ] + 1 ] - strstart [ fontname [ 4 ] ] ) == 0 
  ) 
  ruleslant = 0.0 ; 
  else {
      
    ruleslant = fontinfo [ 1 + parambase [ 4 ] ] .sc / ((double) 65536L ) ; 
    slantn = fontec [ 4 ] ; 
    i = fontinfo [ charbase [ 4 ] + slantn ] .qqqq ; 
    slantunit = fontinfo [ heightbase [ 4 ] + ( i .B1 - 0 ) / 16 ] .sc 
    / ((double) slantn ) ; 
  } 
  slantreported = 0.0 ; 
  i = fontinfo [ charbase [ 3 ] + 1 ] .qqqq ; 
  if ( ! ( i .B0 > 0 ) ) 
  {
    (void) fprintf( stdout , "%s\n",  "Missing pixel char!" ) ; 
    uexit ( 1 ) ; 
  } 
  unscxratio = fontinfo [ widthbase [ 3 ] + i .B0 ] .sc ; 
  xratio = unscxratio / ((double) 65536L ) ; 
  unscyratio = fontinfo [ heightbase [ 3 ] + ( i .B1 - 0 ) / 16 ] .sc ; 
  yratio = unscyratio / ((double) 65536L ) ; 
  unscslantratio = fontinfo [ 1 + parambase [ 3 ] ] .sc * yratio ; 
  slantratio = unscslantratio / ((double) 65536L ) ; 
  if ( xratio * yratio == 0 ) 
  {
    (void) fprintf( stdout , "%s\n",  "Vanishing pixel size!" ) ; 
    uexit ( 1 ) ; 
  } 
  fudgefactor = ( slantratio / ((double) xratio ) ) / ((double) yratio ) ; 
  grayrulethickness = fontinfo [ 8 + parambase [ 3 ] ] .sc ; 
  if ( grayrulethickness == 0 ) 
  grayrulethickness = 26214 ; 
  i = fontinfo [ charbase [ 3 ] + 0 ] .qqqq ; 
  if ( ! ( i .B0 > 0 ) ) 
  {
    (void) fprintf( stdout , "%s\n",  "Missing dot char!" ) ; 
    uexit ( 1 ) ; 
  } 
  dotwidth = fontinfo [ widthbase [ 3 ] + i .B0 ] .sc ; 
  dotheight = fontinfo [ heightbase [ 3 ] + ( i .B1 - 0 ) / 16 ] .sc ; 
  delta = fontinfo [ 2 + parambase [ 2 ] ] .sc / 2 ; 
  thricexheight = 3 * fontinfo [ 5 + parambase [ 2 ] ] .sc ; 
  halfxheight = thricexheight / 6 ; 
  {register integer for_end; k = 0 ; for_end = 4095 ; if ( k <= for_end) do 
    b [ k ] = 0 ; 
  while ( k++ < for_end ) ; } 
  {register integer for_end; k = fontbc [ 3 ] ; for_end = fontec [ 3 ] ; if ( 
  k <= for_end) do 
    if ( k >= 1 ) 
    if ( k <= 120 ) 
    if ( ( fontinfo [ charbase [ 3 ] + k ] .qqqq .B0 > 0 ) ) 
    {
      v = c [ k ] ; 
      do {
	  b [ v ] = k ; 
	v = v + d [ k ] ; 
      } while ( ! ( v > 4095 ) ) ; 
    } 
  while ( k++ < for_end ) ; } 
  {register integer for_end; j = 0 ; for_end = 11 ; if ( j <= for_end) do 
    {
      k = twotothe [ j ] ; 
      v = k ; 
      do {
	  rho [ v ] = k ; 
	v = v + k + k ; 
      } while ( ! ( v > 4095 ) ) ; 
    } 
  while ( j++ < for_end ) ; } 
  rho [ 0 ] = 4096 ; 
} 
void ztypeset ( c ) 
eightbits c ; 
{if ( c >= 128 ) 
  {
    dvibuf [ dviptr ] = 128 ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {
    dvibuf [ dviptr ] = c ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
} 
void zdviscaled ( x ) 
real x ; 
{integer n  ; 
  integer m  ; 
  integer k  ; 
  n = round ( x / ((double) 6553.6 ) ) ; 
  if ( n < 0 ) 
  {
    {
      dvibuf [ dviptr ] = 45 ; 
      dviptr = dviptr + 1 ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
    n = - (integer) n ; 
  } 
  m = n / 10 ; 
  k = 0 ; 
  do {
      k = k + 1 ; 
    buffer [ k ] = ( m % 10 ) + 48 ; 
    m = m / 10 ; 
  } while ( ! ( m == 0 ) ) ; 
  do {
      { 
      dvibuf [ dviptr ] = buffer [ k ] ; 
      dviptr = dviptr + 1 ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
    k = k - 1 ; 
  } while ( ! ( k == 0 ) ) ; 
  if ( n % 10 != 0 ) 
  {
    {
      dvibuf [ dviptr ] = 46 ; 
      dviptr = dviptr + 1 ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
    {
      dvibuf [ dviptr ] = ( n % 10 ) + 48 ; 
      dviptr = dviptr + 1 ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
  } 
} 
void zhbox ( s , f , sendit ) 
strnumber s ; 
internalfontnumber f ; 
boolean sendit ; 
{/* 22 30 */ poolpointer k, endk, maxk  ; 
  fourquarters i, j  ; 
  short curl  ; 
  short curr  ; 
  short bchar  ; 
  integer stackptr  ; 
  fontindex l  ; 
  scaled kernamount  ; 
  eightbits hd  ; 
  scaled x  ; 
  ASCIIcode savec  ; 
  boxwidth = 0 ; 
  boxheight = 0 ; 
  boxdepth = 0 ; 
  k = strstart [ s ] ; 
  maxk = strstart [ s + 1 ] ; 
  savec = strpool [ maxk ] ; 
  strpool [ maxk ] = 32 ; 
  while ( k < maxk ) {
      
    if ( strpool [ k ] == 32 ) 
    {
      boxwidth = boxwidth + fontinfo [ 2 + parambase [ f ] ] .sc ; 
      if ( sendit ) 
      {
	{
	  dvibuf [ dviptr ] = 146 ; 
	  dviptr = dviptr + 1 ; 
	  if ( dviptr == dvilimit ) 
	  dviswap () ; 
	} 
	dvifour ( fontinfo [ 2 + parambase [ f ] ] .sc ) ; 
      } 
      k = k + 1 ; 
    } 
    else {
	
      endk = k ; 
      do {
	  endk = endk + 1 ; 
      } while ( ! ( strpool [ endk ] == 32 ) ) ; 
      kernamount = 0 ; 
      curl = 256 ; 
      stackptr = 0 ; 
      bchar = fontbchar [ f ] ; 
      if ( k < endk ) 
      curr = strpool [ k ] + 0 ; 
      else curr = bchar ; 
      suppresslig = false ; 
      lab22: if ( ( curl < fontbc [ f ] ) || ( curl > fontec [ f ] ) ) 
      {
	i = dummyinfo ; 
	if ( curl == 256 ) 
	l = bcharlabel [ f ] ; 
	else l = fontmemsize ; 
      } 
      else {
	  
	i = fontinfo [ charbase [ f ] + curl ] .qqqq ; 
	if ( ( ( i .B2 - 0 ) % 4 ) != 1 ) 
	l = fontmemsize ; 
	else {
	    
	  l = ligkernbase [ f ] + i .B3 ; 
	  j = fontinfo [ l ] .qqqq ; 
	  if ( j .B0 - 0 > 128 ) 
	  l = ligkernbase [ f ] + 256 * ( j .B2 - 0 ) + j .B3 ; 
	} 
      } 
      if ( suppresslig ) 
      suppresslig = false ; 
      else while ( l < kernbase [ f ] + 0 ) {
	  
	j = fontinfo [ l ] .qqqq ; 
	if ( j .B1 == curr ) 
	if ( j .B0 - 0 <= 128 ) 
	if ( j .B2 - 0 >= 128 ) 
	{
	  kernamount = fontinfo [ kernbase [ f ] + 256 * ( j .B2 - 128 ) + j 
	  .B3 ] .sc ; 
	  goto lab30 ; 
	} 
	else {
	    
	  switch ( j .B2 - 0 ) 
	  {case 1 : 
	  case 5 : 
	    curl = j .B3 - 0 ; 
	    break ; 
	  case 2 : 
	  case 6 : 
	    {
	      curr = j .B3 ; 
	      if ( stackptr == 0 ) 
	      {
		stackptr = 1 ; 
		if ( k < endk ) 
		k = k + 1 ; 
		else bchar = 256 ; 
	      } 
	      ligstack [ stackptr ] = curr ; 
	    } 
	    break ; 
	  case 3 : 
	  case 7 : 
	  case 11 : 
	    {
	      curr = j .B3 ; 
	      stackptr = stackptr + 1 ; 
	      ligstack [ stackptr ] = curr ; 
	      if ( j .B2 - 0 == 11 ) 
	      suppresslig = true ; 
	    } 
	    break ; 
	    default: 
	    {
	      curl = j .B3 - 0 ; 
	      if ( stackptr > 0 ) 
	      {
		stackptr = stackptr - 1 ; 
		if ( stackptr > 0 ) 
		curr = ligstack [ stackptr ] ; 
		else if ( k < endk ) 
		curr = strpool [ k ] + 0 ; 
		else curr = bchar ; 
	      } 
	      else if ( k == endk ) 
	      goto lab30 ; 
	      else {
		  
		k = k + 1 ; 
		if ( k < endk ) 
		curr = strpool [ k ] + 0 ; 
		else curr = bchar ; 
	      } 
	    } 
	    break ; 
	  } 
	  if ( j .B2 - 0 > 3 ) 
	  goto lab30 ; 
	  goto lab22 ; 
	} 
	if ( j .B0 - 0 >= 128 ) 
	goto lab30 ; 
	l = l + j .B0 + 1 ; 
      } 
      lab30: ; 
      if ( ( i .B0 > 0 ) ) 
      {
	boxwidth = boxwidth + fontinfo [ widthbase [ f ] + i .B0 ] .sc + 
	kernamount ; 
	hd = i .B1 - 0 ; 
	x = fontinfo [ heightbase [ f ] + ( hd ) / 16 ] .sc ; 
	if ( x > boxheight ) 
	boxheight = x ; 
	x = fontinfo [ depthbase [ f ] + hd % 16 ] .sc ; 
	if ( x > boxdepth ) 
	boxdepth = x ; 
	if ( sendit ) 
	{
	  typeset ( curl ) ; 
	  if ( kernamount != 0 ) 
	  {
	    {
	      dvibuf [ dviptr ] = 146 ; 
	      dviptr = dviptr + 1 ; 
	      if ( dviptr == dvilimit ) 
	      dviswap () ; 
	    } 
	    dvifour ( kernamount ) ; 
	  } 
	} 
	kernamount = 0 ; 
      } 
      curl = curr - 0 ; 
      if ( stackptr > 0 ) 
      {
	{
	  stackptr = stackptr - 1 ; 
	  if ( stackptr > 0 ) 
	  curr = ligstack [ stackptr ] ; 
	  else if ( k < endk ) 
	  curr = strpool [ k ] + 0 ; 
	  else curr = bchar ; 
	} 
	goto lab22 ; 
      } 
      if ( k < endk ) 
      {
	k = k + 1 ; 
	if ( k < endk ) 
	curr = strpool [ k ] + 0 ; 
	else curr = bchar ; 
	goto lab22 ; 
      } 
    } 
  } 
  strpool [ maxk ] = savec ; 
} 
void zslantcomplaint ( r ) 
real r ; 
{if ( abs ( r - slantreported ) > 0.001 ) 
  {
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "Sorry, I can't make diagonal rules of slant " ) ; 
    } 
    printreal ( r , 10 , 5 ) ; 
    (void) putc( '!' ,  stdout );
    slantreported = r ; 
  } 
} 
nodepointer getavail ( ) 
{register nodepointer Result; maxnode = maxnode + 1 ; 
  if ( maxnode == maxlabels ) 
  {
    (void) fprintf( stdout , "%s\n",  "Too many labels and/or rules!" ) ; 
    uexit ( 1 ) ; 
  } 
  Result = maxnode ; 
  return(Result) ; 
} 
void znodeins ( p , q ) 
nodepointer p ; 
nodepointer q ; 
{nodepointer r  ; 
  if ( yy [ p ] >= yy [ q ] ) 
  {
    do {
	r = q ; 
      q = next [ q ] ; 
    } while ( ! ( yy [ p ] <= yy [ q ] ) ) ; 
    next [ r ] = p ; 
    prev [ p ] = r ; 
    next [ p ] = q ; 
    prev [ q ] = p ; 
  } 
  else {
      
    do {
	r = q ; 
      q = prev [ q ] ; 
    } while ( ! ( yy [ p ] >= yy [ q ] ) ) ; 
    prev [ r ] = p ; 
    next [ p ] = r ; 
    prev [ p ] = q ; 
    next [ q ] = p ; 
  } 
  if ( yy [ p ] - yt [ p ] > maxheight ) 
  maxheight = yy [ p ] - yt [ p ] ; 
  if ( yb [ p ] - yy [ p ] > maxdepth ) 
  maxdepth = yb [ p ] - yy [ p ] ; 
} 
boolean zoverlap ( p , q ) 
nodepointer p ; 
nodepointer q ; 
{/* 10 */ register boolean Result; scaled ythresh  ; 
  scaled xleft, xright, ytop, ybot  ; 
  nodepointer r  ; 
  xleft = xl [ p ] ; 
  xright = xr [ p ] ; 
  ytop = yt [ p ] ; 
  ybot = yb [ p ] ; 
  ythresh = ybot + maxheight ; 
  r = next [ q ] ; 
  while ( yy [ r ] < ythresh ) {
      
    if ( ybot > yt [ r ] ) 
    if ( xleft < xr [ r ] ) 
    if ( xright > xl [ r ] ) 
    if ( ytop < yb [ r ] ) 
    {
      Result = true ; 
      goto lab10 ; 
    } 
    r = next [ r ] ; 
  } 
  ythresh = ytop - maxdepth ; 
  r = q ; 
  while ( yy [ r ] > ythresh ) {
      
    if ( ybot > yt [ r ] ) 
    if ( xleft < xr [ r ] ) 
    if ( xright > xl [ r ] ) 
    if ( ytop < yb [ r ] ) 
    {
      Result = true ; 
      goto lab10 ; 
    } 
    r = prev [ r ] ; 
  } 
  Result = false ; 
  lab10: ; 
  return(Result) ; 
} 
nodepointer znearestdot ( p , d0 ) 
nodepointer p ; 
scaled d0 ; 
{register nodepointer Result; nodepointer bestq  ; 
  scaled dmin, d  ; 
  twin = false ; 
  bestq = 0 ; 
  dmin = 268435456L ; 
  q = next [ p ] ; 
  while ( yy [ q ] < yy [ p ] + dmin ) {
      
    d = abs ( xx [ q ] - xx [ p ] ) ; 
    if ( d < yy [ q ] - yy [ p ] ) 
    d = yy [ q ] - yy [ p ] ; 
    if ( d < d0 ) 
    twin = true ; 
    else if ( d < dmin ) 
    {
      dmin = d ; 
      bestq = q ; 
    } 
    q = next [ q ] ; 
  } 
  q = prev [ p ] ; 
  while ( yy [ q ] > yy [ p ] - dmin ) {
      
    d = abs ( xx [ q ] - xx [ p ] ) ; 
    if ( d < yy [ p ] - yy [ q ] ) 
    d = yy [ p ] - yy [ q ] ; 
    if ( d < d0 ) 
    twin = true ; 
    else if ( d < dmin ) 
    {
      dmin = d ; 
      bestq = q ; 
    } 
    q = prev [ q ] ; 
  } 
  Result = bestq ; 
  return(Result) ; 
} 
void zconvert ( x , y ) 
scaled x ; 
scaled y ; 
{x = x + xoffset ; 
  y = y + yoffset ; 
  dviy = - (integer) round ( yratio * y ) + deltay ; 
  dvix = round ( xratio * x + slantratio * y ) + deltax ; 
} 
void zdvigoto ( x , y ) 
scaled x ; 
scaled y ; 
{{
    
    dvibuf [ dviptr ] = 141 ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  if ( x != 0 ) 
  {
    {
      dvibuf [ dviptr ] = 146 ; 
      dviptr = dviptr + 1 ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
    dvifour ( x ) ; 
  } 
  if ( y != 0 ) 
  {
    {
      dvibuf [ dviptr ] = 160 ; 
      dviptr = dviptr + 1 ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
    dvifour ( y ) ; 
  } 
} 
void ztopcoords ( p ) 
nodepointer p ; 
{xx [ p ] = dvix - ( boxwidth / 2 ) ; 
  xl [ p ] = xx [ p ] - delta ; 
  xr [ p ] = xx [ p ] + boxwidth + delta ; 
  yb [ p ] = dviy - dotheight ; 
  yy [ p ] = yb [ p ] - boxdepth ; 
  yt [ p ] = yy [ p ] - boxheight - delta ; 
} 
void zbotcoords ( p ) 
nodepointer p ; 
{xx [ p ] = dvix - ( boxwidth / 2 ) ; 
  xl [ p ] = xx [ p ] - delta ; 
  xr [ p ] = xx [ p ] + boxwidth + delta ; 
  yt [ p ] = dviy + dotheight ; 
  yy [ p ] = yt [ p ] + boxheight ; 
  yb [ p ] = yy [ p ] + boxdepth + delta ; 
} 
void zrightcoords ( p ) 
nodepointer p ; 
{xl [ p ] = dvix + dotwidth ; 
  xx [ p ] = xl [ p ] ; 
  xr [ p ] = xx [ p ] + boxwidth + delta ; 
  yy [ p ] = dviy + halfxheight ; 
  yb [ p ] = yy [ p ] + boxdepth + delta ; 
  yt [ p ] = yy [ p ] - boxheight - delta ; 
} 
void zleftcoords ( p ) 
nodepointer p ; 
{xr [ p ] = dvix - dotwidth ; 
  xx [ p ] = xr [ p ] - boxwidth ; 
  xl [ p ] = xx [ p ] - delta ; 
  yy [ p ] = dviy + halfxheight ; 
  yb [ p ] = yy [ p ] + boxdepth + delta ; 
  yt [ p ] = yy [ p ] - boxheight - delta ; 
} 
boolean zplacelabel ( p ) 
nodepointer p ; 
{/* 10 40 */ register boolean Result; schar oct  ; 
  nodepointer dfl  ; 
  hbox ( info [ p ] , 2 , false ) ; 
  dvix = xx [ p ] ; 
  dviy = yy [ p ] ; 
  dfl = xl [ p ] ; 
  oct = xr [ p ] ; 
  switch ( oct ) 
  {case 0 : 
  case 4 : 
  case 9 : 
  case 13 : 
    leftcoords ( p ) ; 
    break ; 
  case 1 : 
  case 2 : 
  case 8 : 
  case 11 : 
    botcoords ( p ) ; 
    break ; 
  case 3 : 
  case 7 : 
  case 10 : 
  case 14 : 
    rightcoords ( p ) ; 
    break ; 
  case 6 : 
  case 5 : 
  case 15 : 
  case 12 : 
    topcoords ( p ) ; 
    break ; 
  } 
  if ( ! overlap ( p , dfl ) ) 
  goto lab40 ; 
  switch ( oct ) 
  {case 0 : 
  case 3 : 
  case 15 : 
  case 12 : 
    botcoords ( p ) ; 
    break ; 
  case 1 : 
  case 5 : 
  case 10 : 
  case 14 : 
    leftcoords ( p ) ; 
    break ; 
  case 2 : 
  case 6 : 
  case 9 : 
  case 13 : 
    rightcoords ( p ) ; 
    break ; 
  case 7 : 
  case 4 : 
  case 8 : 
  case 11 : 
    topcoords ( p ) ; 
    break ; 
  } 
  if ( ! overlap ( p , dfl ) ) 
  goto lab40 ; 
  switch ( oct ) 
  {case 0 : 
  case 3 : 
  case 14 : 
  case 13 : 
    topcoords ( p ) ; 
    break ; 
  case 1 : 
  case 5 : 
  case 11 : 
  case 15 : 
    rightcoords ( p ) ; 
    break ; 
  case 2 : 
  case 6 : 
  case 8 : 
  case 12 : 
    leftcoords ( p ) ; 
    break ; 
  case 7 : 
  case 4 : 
  case 9 : 
  case 10 : 
    botcoords ( p ) ; 
    break ; 
  } 
  if ( ! overlap ( p , dfl ) ) 
  goto lab40 ; 
  switch ( oct ) 
  {case 0 : 
  case 4 : 
  case 8 : 
  case 12 : 
    rightcoords ( p ) ; 
    break ; 
  case 1 : 
  case 2 : 
  case 9 : 
  case 10 : 
    topcoords ( p ) ; 
    break ; 
  case 3 : 
  case 7 : 
  case 11 : 
  case 15 : 
    leftcoords ( p ) ; 
    break ; 
  case 6 : 
  case 5 : 
  case 14 : 
  case 13 : 
    botcoords ( p ) ; 
    break ; 
  } 
  if ( ! overlap ( p , dfl ) ) 
  goto lab40 ; 
  xx [ p ] = dvix ; 
  yy [ p ] = dviy ; 
  xl [ p ] = dfl ; 
  Result = false ; 
  goto lab10 ; 
  lab40: nodeins ( p , dfl ) ; 
  dvigoto ( xx [ p ] , yy [ p ] ) ; 
  hbox ( info [ p ] , 2 , true ) ; 
  {
    dvibuf [ dviptr ] = 142 ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  Result = true ; 
  lab10: ; 
  return(Result) ; 
} 
void dopixels ( ) 
{/* 30 31 21 22 10 */ boolean paintblack  ; 
  integer startingcol, finishingcol  ; 
  integer j  ; 
  integer l  ; 
  fourquarters i  ; 
  eightbits v  ; 
  {
    dvibuf [ dviptr ] = 174 ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  deltax = deltax + round ( unscxratio * minx ) ; 
  {register integer for_end; j = 0 ; for_end = maxx - minx ; if ( j <= 
  for_end) do 
    a [ j ] = 0 ; 
  while ( j++ < for_end ) ; } 
  l = 1 ; 
  z = 0 ; 
  startingcol = 0 ; 
  finishingcol = 0 ; 
  y = maxy + 12 ; 
  paintblack = false ; 
  blankrows = 0 ; 
  curgf = getbyte () ; 
  while ( true ) {
      
    do {
	if ( blankrows > 0 ) 
      blankrows = blankrows - 1 ; 
      else if ( curgf != 69 ) 
      {
	x = z ; 
	if ( startingcol > x ) 
	startingcol = x ; 
	while ( true ) {
	    
	  lab22: if ( ( curgf >= 74 ) && ( curgf <= 238 ) ) 
	  {
	    z = curgf - 74 ; 
	    paintblack = true ; 
	    curgf = getbyte () ; 
	    goto lab31 ; 
	  } 
	  else switch ( curgf ) 
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
	    k = curgf ; 
	    break ; 
	  case 64 : 
	    k = getbyte () ; 
	    break ; 
	  case 65 : 
	    k = gettwobytes () ; 
	    break ; 
	  case 66 : 
	    k = getthreebytes () ; 
	    break ; 
	  case 69 : 
	    goto lab31 ; 
	    break ; 
	  case 70 : 
	    {
	      blankrows = 0 ; 
	      z = 0 ; 
	      paintblack = false ; 
	      curgf = getbyte () ; 
	      goto lab31 ; 
	    } 
	    break ; 
	  case 71 : 
	    {
	      blankrows = getbyte () ; 
	      z = 0 ; 
	      paintblack = false ; 
	      curgf = getbyte () ; 
	      goto lab31 ; 
	    } 
	    break ; 
	  case 72 : 
	    {
	      blankrows = gettwobytes () ; 
	      z = 0 ; 
	      paintblack = false ; 
	      curgf = getbyte () ; 
	      goto lab31 ; 
	    } 
	    break ; 
	  case 73 : 
	    {
	      blankrows = getthreebytes () ; 
	      z = 0 ; 
	      paintblack = false ; 
	      curgf = getbyte () ; 
	      goto lab31 ; 
	    } 
	    break ; 
	  case 239 : 
	  case 240 : 
	  case 241 : 
	  case 242 : 
	  case 243 : 
	  case 244 : 
	    {
	      skipnop () ; 
	      goto lab22 ; 
	    } 
	    break ; 
	    default: 
	    {
	      (void) fprintf( stdout , "%s%s%s%ld%c\n",  "Bad GF file: " , "Improper opcode" ,               "! (at byte " , (long)curloc - 1 , ')' ) ; 
	      uexit ( 1 ) ; 
	    } 
	    break ; 
	  } 
	  if ( x + k > finishingcol ) 
	  finishingcol = x + k ; 
	  if ( paintblack ) 
	  {register integer for_end; j = x ; for_end = x + k - 1 ; if ( j <= 
	  for_end) do 
	    a [ j ] = a [ j ] + l ; 
	  while ( j++ < for_end ) ; } 
	  paintblack = ! paintblack ; 
	  x = x + k ; 
	  curgf = getbyte () ; 
	} 
	lab31: ; 
      } 
      l = l + l ; 
      y = y - 1 ; 
    } while ( ! ( l == 4096 ) ) ; 
    dvigoto ( 0 , deltay - round ( unscyratio * y ) ) ; 
    j = startingcol ; 
    while ( true ) {
	
      while ( ( j <= finishingcol ) && ( b [ a [ j ] ] == 0 ) ) j = j + 1 ; 
      if ( j > finishingcol ) 
      goto lab30 ; 
      {
	dvibuf [ dviptr ] = 141 ; 
	dviptr = dviptr + 1 ; 
	if ( dviptr == dvilimit ) 
	dviswap () ; 
      } 
      {
	dvibuf [ dviptr ] = 146 ; 
	dviptr = dviptr + 1 ; 
	if ( dviptr == dvilimit ) 
	dviswap () ; 
      } 
      dvifour ( round ( unscxratio * j + unscslantratio * y ) + deltax ) ; 
      do {
	  v = b [ a [ j ] ] ; 
	a [ j ] = a [ j ] - c [ v ] ; 
	k = j ; 
	j = j + 1 ; 
	while ( b [ a [ j ] ] == v ) {
	    
	  a [ j ] = a [ j ] - c [ v ] ; 
	  j = j + 1 ; 
	} 
	k = j - k ; 
	lab21: if ( k == 1 ) 
	typeset ( v ) ; 
	else {
	    
	  i = fontinfo [ charbase [ 3 ] + v ] .qqqq ; 
	  if ( ( ( i .B2 - 0 ) % 4 ) == 2 ) 
	  {
	    if ( odd ( k ) ) 
	    typeset ( v ) ; 
	    k = k / 2 ; 
	    v = i .B3 - 0 ; 
	    goto lab21 ; 
	  } 
	  else do {
	      typeset ( v ) ; 
	    k = k - 1 ; 
	  } while ( ! ( k == 0 ) ) ; 
	} 
      } while ( ! ( b [ a [ j ] ] == 0 ) ) ; 
      {
	dvibuf [ dviptr ] = 142 ; 
	dviptr = dviptr + 1 ; 
	if ( dviptr == dvilimit ) 
	dviswap () ; 
      } 
    } 
    lab30: ; 
    {
      dvibuf [ dviptr ] = 142 ; 
      dviptr = dviptr + 1 ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
    l = rho [ a [ startingcol ] ] ; 
    {register integer for_end; j = startingcol + 1 ; for_end = finishingcol 
    ; if ( j <= for_end) do 
      if ( l > rho [ a [ j ] ] ) 
      l = rho [ a [ j ] ] ; 
    while ( j++ < for_end ) ; } 
    if ( l == 4096 ) 
    if ( curgf == 69 ) 
    goto lab10 ; 
    else {
	
      y = y - blankrows ; 
      blankrows = 0 ; 
      l = 1 ; 
      startingcol = z ; 
      finishingcol = z ; 
    } 
    else {
	
      while ( a [ startingcol ] == 0 ) startingcol = startingcol + 1 ; 
      while ( a [ finishingcol ] == 0 ) finishingcol = finishingcol - 1 ; 
      {register integer for_end; j = startingcol ; for_end = finishingcol 
      ; if ( j <= for_end) do 
	a [ j ] = a [ j ] / l ; 
      while ( j++ < for_end ) ; } 
      l = 4096 / l ; 
    } 
  } 
  lab10: ; 
} 
void main_body() {
    
  initialize () ; 
  strptr = 0 ; 
  poolptr = 0 ; 
  strstart [ 0 ] = 0 ; 
  l = 0 ; 
  firststring ( 0 ) ; 
  l = 9 ; 
  buffer [ 9 ] = 116 ; 
  buffer [ 8 ] = 105 ; 
  buffer [ 7 ] = 116 ; 
  buffer [ 6 ] = 108 ; 
  buffer [ 5 ] = 101 ; 
  buffer [ 4 ] = 102 ; 
  buffer [ 3 ] = 111 ; 
  buffer [ 2 ] = 110 ; 
  buffer [ 1 ] = 116 ; 
  firststring ( 1 ) ; 
  l = 9 ; 
  buffer [ 9 ] = 108 ; 
  buffer [ 8 ] = 97 ; 
  buffer [ 7 ] = 98 ; 
  buffer [ 6 ] = 101 ; 
  buffer [ 5 ] = 108 ; 
  buffer [ 4 ] = 102 ; 
  buffer [ 3 ] = 111 ; 
  buffer [ 2 ] = 110 ; 
  buffer [ 1 ] = 116 ; 
  firststring ( 2 ) ; 
  l = 8 ; 
  buffer [ 8 ] = 103 ; 
  buffer [ 7 ] = 114 ; 
  buffer [ 6 ] = 97 ; 
  buffer [ 5 ] = 121 ; 
  buffer [ 4 ] = 102 ; 
  buffer [ 3 ] = 111 ; 
  buffer [ 2 ] = 110 ; 
  buffer [ 1 ] = 116 ; 
  firststring ( 3 ) ; 
  l = 9 ; 
  buffer [ 9 ] = 115 ; 
  buffer [ 8 ] = 108 ; 
  buffer [ 7 ] = 97 ; 
  buffer [ 6 ] = 110 ; 
  buffer [ 5 ] = 116 ; 
  buffer [ 4 ] = 102 ; 
  buffer [ 3 ] = 111 ; 
  buffer [ 2 ] = 110 ; 
  buffer [ 1 ] = 116 ; 
  firststring ( 4 ) ; 
  l = 13 ; 
  buffer [ 13 ] = 116 ; 
  buffer [ 12 ] = 105 ; 
  buffer [ 11 ] = 116 ; 
  buffer [ 10 ] = 108 ; 
  buffer [ 9 ] = 101 ; 
  buffer [ 8 ] = 102 ; 
  buffer [ 7 ] = 111 ; 
  buffer [ 6 ] = 110 ; 
  buffer [ 5 ] = 116 ; 
  buffer [ 4 ] = 97 ; 
  buffer [ 3 ] = 114 ; 
  buffer [ 2 ] = 101 ; 
  buffer [ 1 ] = 97 ; 
  firststring ( 5 ) ; 
  l = 13 ; 
  buffer [ 13 ] = 108 ; 
  buffer [ 12 ] = 97 ; 
  buffer [ 11 ] = 98 ; 
  buffer [ 10 ] = 101 ; 
  buffer [ 9 ] = 108 ; 
  buffer [ 8 ] = 102 ; 
  buffer [ 7 ] = 111 ; 
  buffer [ 6 ] = 110 ; 
  buffer [ 5 ] = 116 ; 
  buffer [ 4 ] = 97 ; 
  buffer [ 3 ] = 114 ; 
  buffer [ 2 ] = 101 ; 
  buffer [ 1 ] = 97 ; 
  firststring ( 6 ) ; 
  l = 12 ; 
  buffer [ 12 ] = 103 ; 
  buffer [ 11 ] = 114 ; 
  buffer [ 10 ] = 97 ; 
  buffer [ 9 ] = 121 ; 
  buffer [ 8 ] = 102 ; 
  buffer [ 7 ] = 111 ; 
  buffer [ 6 ] = 110 ; 
  buffer [ 5 ] = 116 ; 
  buffer [ 4 ] = 97 ; 
  buffer [ 3 ] = 114 ; 
  buffer [ 2 ] = 101 ; 
  buffer [ 1 ] = 97 ; 
  firststring ( 7 ) ; 
  l = 13 ; 
  buffer [ 13 ] = 115 ; 
  buffer [ 12 ] = 108 ; 
  buffer [ 11 ] = 97 ; 
  buffer [ 10 ] = 110 ; 
  buffer [ 9 ] = 116 ; 
  buffer [ 8 ] = 102 ; 
  buffer [ 7 ] = 111 ; 
  buffer [ 6 ] = 110 ; 
  buffer [ 5 ] = 116 ; 
  buffer [ 4 ] = 97 ; 
  buffer [ 3 ] = 114 ; 
  buffer [ 2 ] = 101 ; 
  buffer [ 1 ] = 97 ; 
  firststring ( 8 ) ; 
  l = 11 ; 
  buffer [ 11 ] = 116 ; 
  buffer [ 10 ] = 105 ; 
  buffer [ 9 ] = 116 ; 
  buffer [ 8 ] = 108 ; 
  buffer [ 7 ] = 101 ; 
  buffer [ 6 ] = 102 ; 
  buffer [ 5 ] = 111 ; 
  buffer [ 4 ] = 110 ; 
  buffer [ 3 ] = 116 ; 
  buffer [ 2 ] = 97 ; 
  buffer [ 1 ] = 116 ; 
  firststring ( 9 ) ; 
  l = 11 ; 
  buffer [ 11 ] = 108 ; 
  buffer [ 10 ] = 97 ; 
  buffer [ 9 ] = 98 ; 
  buffer [ 8 ] = 101 ; 
  buffer [ 7 ] = 108 ; 
  buffer [ 6 ] = 102 ; 
  buffer [ 5 ] = 111 ; 
  buffer [ 4 ] = 110 ; 
  buffer [ 3 ] = 116 ; 
  buffer [ 2 ] = 97 ; 
  buffer [ 1 ] = 116 ; 
  firststring ( 10 ) ; 
  l = 10 ; 
  buffer [ 10 ] = 103 ; 
  buffer [ 9 ] = 114 ; 
  buffer [ 8 ] = 97 ; 
  buffer [ 7 ] = 121 ; 
  buffer [ 6 ] = 102 ; 
  buffer [ 5 ] = 111 ; 
  buffer [ 4 ] = 110 ; 
  buffer [ 3 ] = 116 ; 
  buffer [ 2 ] = 97 ; 
  buffer [ 1 ] = 116 ; 
  firststring ( 11 ) ; 
  l = 11 ; 
  buffer [ 11 ] = 115 ; 
  buffer [ 10 ] = 108 ; 
  buffer [ 9 ] = 97 ; 
  buffer [ 8 ] = 110 ; 
  buffer [ 7 ] = 116 ; 
  buffer [ 6 ] = 102 ; 
  buffer [ 5 ] = 111 ; 
  buffer [ 4 ] = 110 ; 
  buffer [ 3 ] = 116 ; 
  buffer [ 2 ] = 97 ; 
  buffer [ 1 ] = 116 ; 
  firststring ( 12 ) ; 
  l = 4 ; 
  buffer [ 4 ] = 114 ; 
  buffer [ 3 ] = 117 ; 
  buffer [ 2 ] = 108 ; 
  buffer [ 1 ] = 101 ; 
  firststring ( 13 ) ; 
  l = 5 ; 
  buffer [ 5 ] = 116 ; 
  buffer [ 4 ] = 105 ; 
  buffer [ 3 ] = 116 ; 
  buffer [ 2 ] = 108 ; 
  buffer [ 1 ] = 101 ; 
  firststring ( 14 ) ; 
  l = 13 ; 
  buffer [ 13 ] = 114 ; 
  buffer [ 12 ] = 117 ; 
  buffer [ 11 ] = 108 ; 
  buffer [ 10 ] = 101 ; 
  buffer [ 9 ] = 116 ; 
  buffer [ 8 ] = 104 ; 
  buffer [ 7 ] = 105 ; 
  buffer [ 6 ] = 99 ; 
  buffer [ 5 ] = 107 ; 
  buffer [ 4 ] = 110 ; 
  buffer [ 3 ] = 101 ; 
  buffer [ 2 ] = 115 ; 
  buffer [ 1 ] = 115 ; 
  firststring ( 15 ) ; 
  l = 6 ; 
  buffer [ 6 ] = 111 ; 
  buffer [ 5 ] = 102 ; 
  buffer [ 4 ] = 102 ; 
  buffer [ 3 ] = 115 ; 
  buffer [ 2 ] = 101 ; 
  buffer [ 1 ] = 116 ; 
  firststring ( 16 ) ; 
  l = 7 ; 
  buffer [ 7 ] = 120 ; 
  buffer [ 6 ] = 111 ; 
  buffer [ 5 ] = 102 ; 
  buffer [ 4 ] = 102 ; 
  buffer [ 3 ] = 115 ; 
  buffer [ 2 ] = 101 ; 
  buffer [ 1 ] = 116 ; 
  firststring ( 17 ) ; 
  l = 7 ; 
  buffer [ 7 ] = 121 ; 
  buffer [ 6 ] = 111 ; 
  buffer [ 5 ] = 102 ; 
  buffer [ 4 ] = 102 ; 
  buffer [ 3 ] = 115 ; 
  buffer [ 2 ] = 101 ; 
  buffer [ 1 ] = 116 ; 
  firststring ( 18 ) ; 
  l = 7 ; 
  buffer [ 7 ] = 46 ; 
  buffer [ 6 ] = 50 ; 
  buffer [ 5 ] = 54 ; 
  buffer [ 4 ] = 48 ; 
  buffer [ 3 ] = 50 ; 
  buffer [ 2 ] = 103 ; 
  buffer [ 1 ] = 102 ; 
  firststring ( 19 ) ; 
  l = 4 ; 
  buffer [ 4 ] = 46 ; 
  buffer [ 3 ] = 100 ; 
  buffer [ 2 ] = 118 ; 
  buffer [ 1 ] = 105 ; 
  firststring ( 20 ) ; 
  l = 4 ; 
  buffer [ 4 ] = 46 ; 
  buffer [ 3 ] = 116 ; 
  buffer [ 2 ] = 102 ; 
  buffer [ 1 ] = 109 ; 
  firststring ( 21 ) ; 
  l = 7 ; 
  buffer [ 7 ] = 32 ; 
  buffer [ 6 ] = 32 ; 
  buffer [ 5 ] = 80 ; 
  buffer [ 4 ] = 97 ; 
  buffer [ 3 ] = 103 ; 
  buffer [ 2 ] = 101 ; 
  buffer [ 1 ] = 32 ; 
  firststring ( 22 ) ; 
  l = 12 ; 
  buffer [ 12 ] = 32 ; 
  buffer [ 11 ] = 32 ; 
  buffer [ 10 ] = 67 ; 
  buffer [ 9 ] = 104 ; 
  buffer [ 8 ] = 97 ; 
  buffer [ 7 ] = 114 ; 
  buffer [ 6 ] = 97 ; 
  buffer [ 5 ] = 99 ; 
  buffer [ 4 ] = 116 ; 
  buffer [ 3 ] = 101 ; 
  buffer [ 2 ] = 114 ; 
  buffer [ 1 ] = 32 ; 
  firststring ( 23 ) ; 
  l = 6 ; 
  buffer [ 6 ] = 32 ; 
  buffer [ 5 ] = 32 ; 
  buffer [ 4 ] = 69 ; 
  buffer [ 3 ] = 120 ; 
  buffer [ 2 ] = 116 ; 
  buffer [ 1 ] = 32 ; 
  firststring ( 24 ) ; 
  l = 4 ; 
  buffer [ 4 ] = 32 ; 
  buffer [ 3 ] = 32 ; 
  buffer [ 2 ] = 96 ; 
  buffer [ 1 ] = 96 ; 
  firststring ( 25 ) ; 
  l = 2 ; 
  buffer [ 2 ] = 39 ; 
  buffer [ 1 ] = 39 ; 
  firststring ( 26 ) ; 
  l = 3 ; 
  buffer [ 3 ] = 32 ; 
  buffer [ 2 ] = 61 ; 
  buffer [ 1 ] = 32 ; 
  firststring ( 27 ) ; 
  l = 4 ; 
  buffer [ 4 ] = 32 ; 
  buffer [ 3 ] = 43 ; 
  buffer [ 2 ] = 32 ; 
  buffer [ 1 ] = 40 ; 
  firststring ( 28 ) ; 
  l = 4 ; 
  buffer [ 4 ] = 99 ; 
  buffer [ 3 ] = 109 ; 
  buffer [ 2 ] = 114 ; 
  buffer [ 1 ] = 56 ; 
  firststring ( 29 ) ; 
  l = 6 ; 
  buffer [ 6 ] = 99 ; 
  buffer [ 5 ] = 109 ; 
  buffer [ 4 ] = 116 ; 
  buffer [ 3 ] = 116 ; 
  buffer [ 2 ] = 49 ; 
  buffer [ 1 ] = 48 ; 
  firststring ( 30 ) ; 
  l = 4 ; 
  buffer [ 4 ] = 103 ; 
  buffer [ 3 ] = 114 ; 
  buffer [ 2 ] = 97 ; 
  buffer [ 1 ] = 121 ; 
  firststring ( 31 ) ; 
  l = 5 ; 
  buffer [ 5 ] = 108 ; 
  buffer [ 4 ] = 111 ; 
  buffer [ 3 ] = 103 ; 
  buffer [ 2 ] = 111 ; 
  buffer [ 1 ] = 56 ; 
  firststring ( 32 ) ; 
  l = 8 ; 
  buffer [ 8 ] = 77 ; 
  buffer [ 7 ] = 69 ; 
  buffer [ 6 ] = 84 ; 
  buffer [ 5 ] = 65 ; 
  buffer [ 4 ] = 70 ; 
  buffer [ 3 ] = 79 ; 
  buffer [ 2 ] = 78 ; 
  buffer [ 1 ] = 84 ; 
  firststring ( 33 ) ; 
  l = 0 ; 
  firststring ( 34 ) ; 
  setpaths ( GFFILEPATHBIT + TFMFILEPATHBIT ) ; 
  startgf () ; 
  if ( getbyte () != 247 ) 
  {
    (void) fprintf( stdout , "%s%s%s%ld%c\n",  "Bad GF file: " , "No preamble" , "! (at byte " ,     (long)curloc - 1 , ')' ) ; 
    uexit ( 1 ) ; 
  } 
  if ( getbyte () != 131 ) 
  {
    (void) fprintf( stdout , "%s%s%s%ld%c\n",  "Bad GF file: " , "Wrong ID" , "! (at byte " , (long)curloc -     1 , ')' ) ; 
    uexit ( 1 ) ; 
  } 
  k = getbyte () ; 
  {register integer for_end; m = 1 ; for_end = k ; if ( m <= for_end) do 
    {
      strpool [ poolptr ] = getbyte () ; 
      poolptr = poolptr + 1 ; 
    } 
  while ( m++ < for_end ) ; } 
  {
    dvibuf [ dviptr ] = 247 ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  {
    dvibuf [ dviptr ] = 2 ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  dvifour ( 25400000L ) ; 
  dvifour ( 473628672L ) ; 
  dvifour ( 1000 ) ; 
  {
    dvibuf [ dviptr ] = k ; 
    dviptr = dviptr + 1 ; 
    if ( dviptr == dvilimit ) 
    dviswap () ; 
  } 
  uselogo = false ; 
  s = strstart [ strptr ] ; 
  {register integer for_end; m = 1 ; for_end = k ; if ( m <= for_end) do 
    {
      dvibuf [ dviptr ] = strpool [ s + m - 1 ] ; 
      dviptr = dviptr + 1 ; 
      if ( dviptr == dvilimit ) 
      dviswap () ; 
    } 
  while ( m++ < for_end ) ; } 
  if ( strpool [ s ] == 32 ) 
  if ( strpool [ s + 1 ] == 77 ) 
  if ( strpool [ s + 2 ] == 69 ) 
  if ( strpool [ s + 3 ] == 84 ) 
  if ( strpool [ s + 4 ] == 65 ) 
  if ( strpool [ s + 5 ] == 70 ) 
  if ( strpool [ s + 6 ] == 79 ) 
  if ( strpool [ s + 7 ] == 78 ) 
  if ( strpool [ s + 8 ] == 84 ) 
  {
    strptr = strptr + 1 ; 
    strstart [ strptr ] = s + 9 ; 
    uselogo = true ; 
  } 
  timestamp = makestring () ; 
  curgf = getbyte () ; 
  initstrptr = strptr ; 
  while ( true ) {
      
    maxnode = 0 ; 
    next [ 0 ] = maxlabels ; 
    prev [ maxlabels ] = 0 ; 
    maxheight = 0 ; 
    maxdepth = 0 ; 
    rulethickness = 0 ; 
    offsetx = 0 ; 
    offsety = 0 ; 
    xoffset = 0 ; 
    yoffset = 0 ; 
    preminx = 268435456L ; 
    premaxx = -268435456L ; 
    preminy = 268435456L ; 
    premaxy = -268435456L ; 
    ruleptr = 0 ; 
    titlehead = 0 ; 
    titletail = 0 ; 
    next [ maxlabels ] = 0 ; 
    labeltail = maxlabels ; 
    firstdot = maxlabels ; 
    while ( ( curgf >= 239 ) && ( curgf <= 244 ) ) {
	
      k = interpretxxx () ; 
      switch ( k ) 
      {case 19 : 
	; 
	break ; 
      case 1 : 
      case 2 : 
      case 3 : 
      case 4 : 
	if ( fontsnotloaded ) 
	{
	  fontname [ k ] = curstring ; 
	  fontarea [ k ] = 0 ; 
	  fontat [ k ] = 0 ; 
	  initstrptr = strptr ; 
	} 
	else {
	    
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%s%ld%s",  "(Tardy font change will be ignored (byte " ,           (long)curloc , ")!)" ) ; 
	} 
	break ; 
      case 5 : 
      case 6 : 
      case 7 : 
      case 8 : 
	if ( fontsnotloaded ) 
	{
	  fontarea [ k - 4 ] = curstring ; 
	  initstrptr = strptr ; 
	} 
	else {
	    
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%s%ld%s",  "(Tardy font change will be ignored (byte " ,           (long)curloc , ")!)" ) ; 
	} 
	break ; 
      case 9 : 
      case 10 : 
      case 11 : 
      case 12 : 
	if ( fontsnotloaded ) 
	{
	  fontat [ k - 8 ] = getyyy () ; 
	  initstrptr = strptr ; 
	} 
	else {
	    
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%s%ld%s",  "(Tardy font change will be ignored (byte " ,           (long)curloc , ")!)" ) ; 
	} 
	break ; 
      case 15 : 
	rulethickness = getyyy () ; 
	break ; 
      case 13 : 
	{
	  p = getavail () ; 
	  next [ p ] = ruleptr ; 
	  ruleptr = p ; 
	  xl [ p ] = getyyy () ; 
	  yt [ p ] = getyyy () ; 
	  xr [ p ] = getyyy () ; 
	  yb [ p ] = getyyy () ; 
	  if ( xl [ p ] < preminx ) 
	  preminx = xl [ p ] ; 
	  if ( xl [ p ] > premaxx ) 
	  premaxx = xl [ p ] ; 
	  if ( yt [ p ] < preminy ) 
	  preminy = yt [ p ] ; 
	  if ( yt [ p ] > premaxy ) 
	  premaxy = yt [ p ] ; 
	  if ( xr [ p ] < preminx ) 
	  preminx = xr [ p ] ; 
	  if ( xr [ p ] > premaxx ) 
	  premaxx = xr [ p ] ; 
	  if ( yb [ p ] < preminy ) 
	  preminy = yb [ p ] ; 
	  if ( yb [ p ] > premaxy ) 
	  premaxy = yb [ p ] ; 
	  xx [ p ] = rulethickness ; 
	} 
	break ; 
      case 16 : 
	{
	  offsetx = getyyy () ; 
	  offsety = getyyy () ; 
	} 
	break ; 
      case 17 : 
	xoffset = getyyy () ; 
	break ; 
      case 18 : 
	yoffset = getyyy () ; 
	break ; 
      case 14 : 
	{
	  p = getavail () ; 
	  info [ p ] = curstring ; 
	  if ( titlehead == 0 ) 
	  titlehead = p ; 
	  else next [ titletail ] = p ; 
	  titletail = p ; 
	} 
	break ; 
      case 0 : 
	if ( ( labeltype < 47 ) || ( labeltype > 56 ) ) 
	{
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%s%ld%c",  "Bad label type precedes byte " , (long)curloc , '!' ) ; 
	} 
	else {
	    
	  p = getavail () ; 
	  next [ labeltail ] = p ; 
	  labeltail = p ; 
	  prev [ p ] = labeltype ; 
	  info [ p ] = curstring ; 
	  xx [ p ] = getyyy () ; 
	  yy [ p ] = getyyy () ; 
	  if ( xx [ p ] < preminx ) 
	  preminx = xx [ p ] ; 
	  if ( xx [ p ] > premaxx ) 
	  premaxx = xx [ p ] ; 
	  if ( yy [ p ] < preminy ) 
	  preminy = yy [ p ] ; 
	  if ( yy [ p ] > premaxy ) 
	  premaxy = yy [ p ] ; 
	} 
	break ; 
      } 
    } 
    if ( curgf == 248 ) 
    {
      {
	dvibuf [ dviptr ] = 248 ; 
	dviptr = dviptr + 1 ; 
	if ( dviptr == dvilimit ) 
	dviswap () ; 
      } 
      dvifour ( lastbop ) ; 
      lastbop = dvioffset + dviptr - 5 ; 
      dvifour ( 25400000L ) ; 
      dvifour ( 473628672L ) ; 
      dvifour ( 1000 ) ; 
      dvifour ( maxv ) ; 
      dvifour ( maxh ) ; 
      {
	dvibuf [ dviptr ] = 0 ; 
	dviptr = dviptr + 1 ; 
	if ( dviptr == dvilimit ) 
	dviswap () ; 
      } 
      {
	dvibuf [ dviptr ] = 3 ; 
	dviptr = dviptr + 1 ; 
	if ( dviptr == dvilimit ) 
	dviswap () ; 
      } 
      {
	dvibuf [ dviptr ] = totalpages / 256 ; 
	dviptr = dviptr + 1 ; 
	if ( dviptr == dvilimit ) 
	dviswap () ; 
      } 
      {
	dvibuf [ dviptr ] = totalpages % 256 ; 
	dviptr = dviptr + 1 ; 
	if ( dviptr == dvilimit ) 
	dviswap () ; 
      } 
      if ( ! fontsnotloaded ) 
      {register integer for_end; k = 1 ; for_end = 5 ; if ( k <= for_end) do 
	if ( ( strstart [ fontname [ k ] + 1 ] - strstart [ fontname [ k ] ] ) 
	> 0 ) 
	dvifontdef ( k ) ; 
      while ( k++ < for_end ) ; } 
      {
	dvibuf [ dviptr ] = 249 ; 
	dviptr = dviptr + 1 ; 
	if ( dviptr == dvilimit ) 
	dviswap () ; 
      } 
      dvifour ( lastbop ) ; 
      {
	dvibuf [ dviptr ] = 2 ; 
	dviptr = dviptr + 1 ; 
	if ( dviptr == dvilimit ) 
	dviswap () ; 
      } 
      k = 4 + ( ( dvibufsize - dviptr ) % 4 ) ; 
      while ( k > 0 ) {
	  
	{
	  dvibuf [ dviptr ] = 223 ; 
	  dviptr = dviptr + 1 ; 
	  if ( dviptr == dvilimit ) 
	  dviswap () ; 
	} 
	k = k - 1 ; 
      } 
      if ( dvilimit == halfbuf ) 
      writedvi ( halfbuf , dvibufsize - 1 ) ; 
      if ( dviptr > 0 ) 
      writedvi ( 0 , dviptr - 1 ) ; 
      if ( verbose ) 
      (void) fprintf( stdout , "%c\n",  ' ' ) ; 
      uexit ( 0 ) ; 
    } 
    if ( curgf != 67 ) 
    if ( curgf != 68 ) 
    {
      (void) fprintf( stdout , "%s\n",  "Missing boc!" ) ; 
      uexit ( 1 ) ; 
    } 
    {
      if ( fontsnotloaded ) 
      loadfonts () ; 
      if ( curgf == 67 ) 
      {
	ext = signedquad () ; 
	charcode = ext % 256 ; 
	if ( charcode < 0 ) 
	charcode = charcode + 256 ; 
	ext = ( ext - charcode ) / 256 ; 
	k = signedquad () ; 
	minx = signedquad () ; 
	maxx = signedquad () ; 
	miny = signedquad () ; 
	maxy = signedquad () ; 
      } 
      else {
	  
	ext = 0 ; 
	charcode = getbyte () ; 
	minx = getbyte () ; 
	maxx = getbyte () ; 
	minx = maxx - minx ; 
	miny = getbyte () ; 
	maxy = getbyte () ; 
	miny = maxy - miny ; 
      } 
      if ( maxx - minx > widestrow ) 
      {
	(void) fprintf( stdout , "%s\n",  "Character too wide!" ) ; 
	uexit ( 1 ) ; 
      } 
      if ( preminx < minx * 65536L ) 
      offsetx = offsetx + minx * 65536L - preminx ; 
      if ( premaxy > maxy * 65536L ) 
      offsety = offsety + maxy * 65536L - premaxy ; 
      if ( premaxx > maxx * 65536L ) 
      premaxx = premaxx / 65536L ; 
      else premaxx = maxx ; 
      if ( preminy < miny * 65536L ) 
      preminy = preminy / 65536L ; 
      else preminy = miny ; 
      deltay = round ( unscyratio * ( maxy + 1 ) - yratio * offsety ) + 
      3276800L ; 
      deltax = round ( xratio * offsetx - unscxratio * minx ) ; 
      if ( slantratio >= 0 ) 
      overcol = round ( unscxratio * premaxx + unscslantratio * maxy ) ; 
      else overcol = round ( unscxratio * premaxx + unscslantratio * miny ) ; 
      overcol = overcol + deltax + overflowlabeloffset ; 
      pageheight = round ( unscyratio * ( maxy + 1 - preminy ) ) + 3276800L - 
      offsety ; 
      if ( pageheight > maxv ) 
      maxv = pageheight ; 
      pagewidth = overcol - 10000000L ; 
      {
	dvibuf [ dviptr ] = 139 ; 
	dviptr = dviptr + 1 ; 
	if ( dviptr == dvilimit ) 
	dviswap () ; 
      } 
      totalpages = totalpages + 1 ; 
      dvifour ( totalpages ) ; 
      dvifour ( charcode ) ; 
      dvifour ( ext ) ; 
      {register integer for_end; k = 3 ; for_end = 9 ; if ( k <= for_end) do 
	dvifour ( 0 ) ; 
      while ( k++ < for_end ) ; } 
      dvifour ( lastbop ) ; 
      lastbop = dvioffset + dviptr - 45 ; 
      dvigoto ( 0 , 655360L ) ; 
      if ( uselogo ) 
      {
	{
	  dvibuf [ dviptr ] = 176 ; 
	  dviptr = dviptr + 1 ; 
	  if ( dviptr == dvilimit ) 
	  dviswap () ; 
	} 
	hbox ( 33 , 5 , true ) ; 
      } 
      {
	dvibuf [ dviptr ] = 172 ; 
	dviptr = dviptr + 1 ; 
	if ( dviptr == dvilimit ) 
	dviswap () ; 
      } 
      hbox ( timestamp , 1 , true ) ; 
      hbox ( 22 , 1 , true ) ; 
      dviscaled ( totalpages * 65536.0 ) ; 
      if ( ( charcode != 0 ) || ( ext != 0 ) ) 
      {
	hbox ( 23 , 1 , true ) ; 
	dviscaled ( charcode * 65536.0 ) ; 
	if ( ext != 0 ) 
	{
	  hbox ( 24 , 1 , true ) ; 
	  dviscaled ( ext * 65536.0 ) ; 
	} 
      } 
      if ( titlehead != 0 ) 
      {
	next [ titletail ] = 0 ; 
	do {
	    hbox ( 25 , 1 , true ) ; 
	  hbox ( info [ titlehead ] , 1 , true ) ; 
	  hbox ( 26 , 1 , true ) ; 
	  titlehead = next [ titlehead ] ; 
	} while ( ! ( titlehead == 0 ) ) ; 
      } 
      {
	dvibuf [ dviptr ] = 142 ; 
	dviptr = dviptr + 1 ; 
	if ( dviptr == dvilimit ) 
	dviswap () ; 
      } 
      if ( verbose ) 
      {
	(void) fprintf( stdout , "%c%ld",  '[' , (long)totalpages ) ; 
	flush ( stdout ) ; 
      } 
      if ( ruleslant != 0 ) 
      {
	dvibuf [ dviptr ] = 175 ; 
	dviptr = dviptr + 1 ; 
	if ( dviptr == dvilimit ) 
	dviswap () ; 
      } 
      while ( ruleptr != 0 ) {
	  
	p = ruleptr ; 
	ruleptr = next [ p ] ; 
	if ( xx [ p ] == 0 ) 
	xx [ p ] = grayrulethickness ; 
	if ( xx [ p ] > 0 ) 
	{
	  convert ( xl [ p ] , yt [ p ] ) ; 
	  tempx = dvix ; 
	  tempy = dviy ; 
	  convert ( xr [ p ] , yb [ p ] ) ; 
	  if ( abs ( tempx - dvix ) < 6554 ) 
	  {
	    if ( tempy > dviy ) 
	    {
	      k = tempy ; 
	      tempy = dviy ; 
	      dviy = k ; 
	    } 
	    dvigoto ( dvix - ( xx [ p ] / 2 ) , dviy ) ; 
	    {
	      dvibuf [ dviptr ] = 137 ; 
	      dviptr = dviptr + 1 ; 
	      if ( dviptr == dvilimit ) 
	      dviswap () ; 
	    } 
	    dvifour ( dviy - tempy ) ; 
	    dvifour ( xx [ p ] ) ; 
	    {
	      dvibuf [ dviptr ] = 142 ; 
	      dviptr = dviptr + 1 ; 
	      if ( dviptr == dvilimit ) 
	      dviswap () ; 
	    } 
	  } 
	  else if ( abs ( tempy - dviy ) < 6554 ) 
	  {
	    if ( tempx < dvix ) 
	    {
	      k = tempx ; 
	      tempx = dvix ; 
	      dvix = k ; 
	    } 
	    dvigoto ( dvix , dviy + ( xx [ p ] / 2 ) ) ; 
	    {
	      dvibuf [ dviptr ] = 137 ; 
	      dviptr = dviptr + 1 ; 
	      if ( dviptr == dvilimit ) 
	      dviswap () ; 
	    } 
	    dvifour ( xx [ p ] ) ; 
	    dvifour ( tempx - dvix ) ; 
	    {
	      dvibuf [ dviptr ] = 142 ; 
	      dviptr = dviptr + 1 ; 
	      if ( dviptr == dvilimit ) 
	      dviswap () ; 
	    } 
	  } 
	  else if ( ( ruleslant == 0 ) || ( abs ( tempx + ruleslant * ( tempy 
	  - dviy ) - dvix ) > xx [ p ] ) ) 
	  slantcomplaint ( ( dvix - tempx ) / ((double) ( tempy - dviy ) ) ) ; 
	  else {
	      
	    if ( tempy > dviy ) 
	    {
	      k = tempy ; 
	      tempy = dviy ; 
	      dviy = k ; 
	      k = tempx ; 
	      tempx = dvix ; 
	      dvix = k ; 
	    } 
	    m = round ( ( dviy - tempy ) / ((double) slantunit ) ) ; 
	    if ( m > 0 ) 
	    {
	      dvigoto ( dvix , dviy ) ; 
	      q = ( ( m - 1 ) / slantn ) + 1 ; 
	      k = m / q ; 
	      p = m % q ; 
	      q = q - p ; 
	      typeset ( k ) ; 
	      dy = round ( k * slantunit ) ; 
	      {
		dvibuf [ dviptr ] = 170 ; 
		dviptr = dviptr + 1 ; 
		if ( dviptr == dvilimit ) 
		dviswap () ; 
	      } 
	      dvifour ( - (integer) dy ) ; 
	      while ( q > 1 ) {
		  
		typeset ( k ) ; 
		{
		  dvibuf [ dviptr ] = 166 ; 
		  dviptr = dviptr + 1 ; 
		  if ( dviptr == dvilimit ) 
		  dviswap () ; 
		} 
		q = q - 1 ; 
	      } 
	      if ( p > 0 ) 
	      {
		k = k + 1 ; 
		typeset ( k ) ; 
		dy = round ( k * slantunit ) ; 
		{
		  dvibuf [ dviptr ] = 170 ; 
		  dviptr = dviptr + 1 ; 
		  if ( dviptr == dvilimit ) 
		  dviswap () ; 
		} 
		dvifour ( - (integer) dy ) ; 
		while ( p > 1 ) {
		    
		  typeset ( k ) ; 
		  {
		    dvibuf [ dviptr ] = 166 ; 
		    dviptr = dviptr + 1 ; 
		    if ( dviptr == dvilimit ) 
		    dviswap () ; 
		  } 
		  p = p - 1 ; 
		} 
	      } 
	      {
		dvibuf [ dviptr ] = 142 ; 
		dviptr = dviptr + 1 ; 
		if ( dviptr == dvilimit ) 
		dviswap () ; 
	      } 
	    } 
	  } 
	} 
      } 
      overflowline = 1 ; 
      if ( next [ maxlabels ] != 0 ) 
      {
	next [ labeltail ] = 0 ; 
	{
	  dvibuf [ dviptr ] = 174 ; 
	  dviptr = dviptr + 1 ; 
	  if ( dviptr == dvilimit ) 
	  dviswap () ; 
	} 
	p = next [ maxlabels ] ; 
	firstdot = maxnode + 1 ; 
	while ( p != 0 ) {
	    
	  convert ( xx [ p ] , yy [ p ] ) ; 
	  xx [ p ] = dvix ; 
	  yy [ p ] = dviy ; 
	  if ( prev [ p ] < 53 ) 
	  {
	    q = getavail () ; 
	    xl [ p ] = q ; 
	    info [ q ] = p ; 
	    xx [ q ] = dvix ; 
	    xl [ q ] = dvix - dotwidth ; 
	    xr [ q ] = dvix + dotwidth ; 
	    yy [ q ] = dviy ; 
	    yt [ q ] = dviy - dotheight ; 
	    yb [ q ] = dviy + dotheight ; 
	    nodeins ( q , 0 ) ; 
	    dvigoto ( xx [ q ] , yy [ q ] ) ; 
	    {
	      dvibuf [ dviptr ] = 0 ; 
	      dviptr = dviptr + 1 ; 
	      if ( dviptr == dvilimit ) 
	      dviswap () ; 
	    } 
	    {
	      dvibuf [ dviptr ] = 142 ; 
	      dviptr = dviptr + 1 ; 
	      if ( dviptr == dvilimit ) 
	      dviswap () ; 
	    } 
	  } 
	  p = next [ p ] ; 
	} 
	p = next [ maxlabels ] ; 
	while ( p != 0 ) {
	    
	  if ( prev [ p ] <= 48 ) 
	  {
	    r = xl [ p ] ; 
	    q = nearestdot ( r , 10 ) ; 
	    if ( twin ) 
	    xr [ p ] = 8 ; 
	    else xr [ p ] = 0 ; 
	    if ( q != 0 ) 
	    {
	      dx = xx [ q ] - xx [ r ] ; 
	      dy = yy [ q ] - yy [ r ] ; 
	      if ( dy > 0 ) 
	      xr [ p ] = xr [ p ] + 4 ; 
	      if ( dx < 0 ) 
	      xr [ p ] = xr [ p ] + 1 ; 
	      if ( dy > dx ) 
	      xr [ p ] = xr [ p ] + 1 ; 
	      if ( - (integer) dy > dx ) 
	      xr [ p ] = xr [ p ] + 1 ; 
	    } 
	  } 
	  p = next [ p ] ; 
	} 
	{
	  dvibuf [ dviptr ] = 173 ; 
	  dviptr = dviptr + 1 ; 
	  if ( dviptr == dvilimit ) 
	  dviswap () ; 
	} 
	q = maxlabels ; 
	while ( next [ q ] != 0 ) {
	    
	  p = next [ q ] ; 
	  if ( prev [ p ] > 48 ) 
	  {
	    next [ q ] = next [ p ] ; 
	    {
	      hbox ( info [ p ] , 2 , false ) ; 
	      dvix = xx [ p ] ; 
	      dviy = yy [ p ] ; 
	      if ( prev [ p ] < 53 ) 
	      r = xl [ p ] ; 
	      else r = 0 ; 
	      switch ( prev [ p ] ) 
	      {case 49 : 
	      case 53 : 
		topcoords ( p ) ; 
		break ; 
	      case 50 : 
	      case 54 : 
		leftcoords ( p ) ; 
		break ; 
	      case 51 : 
	      case 55 : 
		rightcoords ( p ) ; 
		break ; 
	      case 52 : 
	      case 56 : 
		botcoords ( p ) ; 
		break ; 
	      } 
	      nodeins ( p , r ) ; 
	      dvigoto ( xx [ p ] , yy [ p ] ) ; 
	      hbox ( info [ p ] , 2 , true ) ; 
	      {
		dvibuf [ dviptr ] = 142 ; 
		dviptr = dviptr + 1 ; 
		if ( dviptr == dvilimit ) 
		dviswap () ; 
	      } 
	    } 
	  } 
	  else q = next [ q ] ; 
	} 
	q = maxlabels ; 
	while ( next [ q ] != 0 ) {
	    
	  p = next [ q ] ; 
	  r = next [ p ] ; 
	  s = xl [ p ] ; 
	  if ( placelabel ( p ) ) 
	  next [ q ] = r ; 
	  else {
	      
	    info [ s ] = 0 ; 
	    if ( prev [ p ] == 47 ) 
	    next [ q ] = r ; 
	    else q = p ; 
	  } 
	} 
	p = next [ 0 ] ; 
	while ( p != maxlabels ) {
	    
	  q = next [ p ] ; 
	  if ( ( p < firstdot ) || ( info [ p ] == 0 ) ) 
	  {
	    r = prev [ p ] ; 
	    next [ r ] = q ; 
	    prev [ q ] = r ; 
	    next [ p ] = r ; 
	  } 
	  p = q ; 
	} 
	p = next [ maxlabels ] ; 
	while ( p != 0 ) {
	    
	  {
	    r = next [ xl [ p ] ] ; 
	    s = next [ r ] ; 
	    t = next [ p ] ; 
	    next [ p ] = s ; 
	    prev [ s ] = p ; 
	    next [ r ] = p ; 
	    prev [ p ] = r ; 
	    q = nearestdot ( p , 0 ) ; 
	    next [ r ] = s ; 
	    prev [ s ] = r ; 
	    next [ p ] = t ; 
	    overflowline = overflowline + 1 ; 
	    dvigoto ( overcol , overflowline * thricexheight + 655360L ) ; 
	    hbox ( info [ p ] , 2 , true ) ; 
	    if ( q != 0 ) 
	    {
	      hbox ( 27 , 2 , true ) ; 
	      hbox ( info [ info [ q ] ] , 2 , true ) ; 
	      hbox ( 28 , 2 , true ) ; 
	      dviscaled ( ( xx [ p ] - xx [ q ] ) / ((double) xratio ) + ( yy 
	      [ p ] - yy [ q ] ) * fudgefactor ) ; 
	      {
		dvibuf [ dviptr ] = 44 ; 
		dviptr = dviptr + 1 ; 
		if ( dviptr == dvilimit ) 
		dviswap () ; 
	      } 
	      dviscaled ( ( yy [ q ] - yy [ p ] ) / ((double) yratio ) ) ; 
	      {
		dvibuf [ dviptr ] = 41 ; 
		dviptr = dviptr + 1 ; 
		if ( dviptr == dvilimit ) 
		dviswap () ; 
	      } 
	    } 
	    {
	      dvibuf [ dviptr ] = 142 ; 
	      dviptr = dviptr + 1 ; 
	      if ( dviptr == dvilimit ) 
	      dviswap () ; 
	    } 
	  } 
	  p = next [ p ] ; 
	} 
      } 
      dopixels () ; 
      {
	dvibuf [ dviptr ] = 140 ; 
	dviptr = dviptr + 1 ; 
	if ( dviptr == dvilimit ) 
	dviswap () ; 
      } 
      if ( overflowline > 1 ) 
      pagewidth = overcol + 10000000L ; 
      if ( pagewidth > maxh ) 
      maxh = pagewidth ; 
      if ( verbose ) 
      {
	(void) putc( ']' ,  stdout );
	if ( totalpages % 13 == 0 ) 
	(void) fprintf( stdout , "%c\n",  ' ' ) ; 
	else
	(void) putc( ' ' ,  stdout );
	flush ( stdout ) ; 
      } 
    } 
    curgf = getbyte () ; 
    strptr = initstrptr ; 
    poolptr = strstart [ strptr ] ; 
  } 
  if ( verbose && ( totalpages % 13 != 0 ) ) 
  (void) fprintf( stdout , "%c\n",  ' ' ) ; 
} 
