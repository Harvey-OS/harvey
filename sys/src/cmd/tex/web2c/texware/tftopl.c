#include "config.h"
/* 9999 */ 
#define tfmsize 30000 
#define ligsize 5000 
#define hashsize 5003 
#define charcodeascii 0 
#define charcodeoctal 1 
#define charcodedefault 2 
#define noptions 2 
#define argoptions 1 
typedef unsigned char byte  ; 
typedef integer index  ; 
typedef integer charcodeformattype  ; 
file_ptr /* of  unsigned char */ tfmfile  ; 
char tfmname[PATHMAX + 1]  ; 
short lf, lh, bc, ec, nw, nh, nd, ni, nl, nk, ne, np  ; 
text plfile  ; 
char plname[PATHMAX + 1]  ; 
#define tfm (tfmfilearray + 1001)
pointertobyte tfmfilearray  ; 
integer charbase, widthbase, heightbase, depthbase, italicbase, ligkernbase, 
kernbase, extenbase, parambase  ; 
schar fonttype  ; 
ccharpointer ASCII04, ASCII10, ASCII14  ; 
char ASCIIall[257]  ; 
ccharpointer MBLstring, RIstring, RCEstring  ; 
schar dig[12]  ; 
schar level  ; 
schar charsonline  ; 
boolean perfect  ; 
short i  ; 
short c  ; 
schar d  ; 
index k  ; 
unsigned short r  ; 
schar count  ; 
struct {
    short cc ; 
  integer rr ; 
} labeltable[259]  ; 
short labelptr  ; 
short sortptr  ; 
short boundarychar  ; 
short bcharlabel  ; 
schar activity[ligsize + 1]  ; 
integer ai, acti  ; 
integer hash[hashsize + 1]  ; 
schar class[hashsize + 1]  ; 
short ligz[hashsize + 1]  ; 
integer hashptr  ; 
integer hashlist[hashsize + 1]  ; 
integer h, hh  ; 
short xligcycle, yligcycle  ; 
integer verbose  ; 
charcodeformattype charcodeformat  ; 

#include "tftopl.h"
void initialize ( ) 
{getoptstruct longoptions[noptions + 1]  ; 
  integer getoptreturnval  ; 
  integer optionindex  ; 
  integer currentoption  ; 
  if ( ( argc < 2 ) || ( argc > noptions + argoptions + 3 ) ) 
  {
    (void) Fputs(stdout, "Usage: tftopl " ) ; 
    (void) Fputs(stdout, "[-verbose] " ) ; 
    (void) Fputs(stdout, "[-charcode-format=<format>] " ) ; 
    (void) fprintf(stdout, "%s\n", "<tfm file> [<property list file>]." ) ; 
    uexit ( 1 ) ; 
  } 
  tfmfilearray = casttobytepointer ( xmalloc ( 1002 ) ) ; 
  verbose = false ; 
  charcodeformat = charcodedefault ; 
  {
    currentoption = 0 ; 
    longoptions [ 0 ] .name = "verbose" ; 
    longoptions [ 0 ] .hasarg = 0 ; 
    longoptions [ 0 ] .flag = addressofint ( verbose ) ; 
    longoptions [ 0 ] .val = 1 ; 
    currentoption = currentoption + 1 ; 
    longoptions [ currentoption ] .name = "charcode-format" ; 
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
	if ( ( strcmp ( longoptions [ optionindex ] .name , "charcode-format" 
	) == 0 ) ) 
	{
	  if ( strcmp ( optarg , "ascii" ) == 0 ) 
	  charcodeformat = charcodeascii ; 
	  else if ( strcmp ( optarg , "octal" ) == 0 ) 
	  charcodeformat = charcodeoctal ; 
	  else
	  (void) fprintf(stdout, "%s%ld%c", "Bad character code format" , (long)optarg , '.' ) ; 
	} 
	else ; 
      } 
    } while ( ! ( getoptreturnval == -1 ) ) ; 
  } 
  setpaths ( TFMFILEPATHBIT ) ; 
  argv ( optind , tfmname ) ; 
  if ( testreadaccess ( tfmname , TFMFILEPATH ) ) 
  {
    reset ( tfmfile , tfmname ) ; 
  } 
  else {
      
    printpascalstring ( tfmname ) ; 
    (void) fprintf(stdout, "%s\n", ": TFM file not found." ) ; 
    uexit ( 1 ) ; 
  } 
  if ( verbose ) 
  (void) fprintf(stdout, "%s\n", "This is TFtoPL, C Version 3.1" ) ; 
  if ( optind + 1 == argc ) 
  plfile = stdout ; 
  else {
      
    argv ( optind + 1 , plname ) ; 
    rewrite ( plfile , plname ) ; 
  } 
  ASCII04 = "  !\"#$%&'()*+,-./0123456789:;<=>?" ; 
  ASCII10 = " @ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_" ; 
  ASCII14 = " `abcdefghijklmnopqrstuvwxyz{|}~ " ; 
  vstrcpy ( ASCIIall , ASCII04 ) ; 
  vstrcat ( ASCIIall , "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_" ) ; 
  vstrcat ( ASCIIall , "`abcdefghijklmnopqrstuvwxyz{|}~" ) ; 
  MBLstring = " MBL" ; 
  RIstring = " RI " ; 
  RCEstring = " RCE" ; 
  level = 0 ; 
  charsonline = 0 ; 
  perfect = true ; 
  boundarychar = 256 ; 
  bcharlabel = 32767 ; 
  labelptr = 0 ; 
  labeltable [ 0 ] .rr = 0 ; 
} 
void zoutdigs ( j ) 
integer j ; 
{do {
    j = j - 1 ; 
    (void) fprintf( plfile , "%ld",  (long)dig [ j ] ) ; 
  } while ( ! ( j == 0 ) ) ; 
} 
void zprintdigs ( j ) 
integer j ; 
{do {
    j = j - 1 ; 
    (void) fprintf(stdout, "%ld", (long)dig [ j ] ) ; 
  } while ( ! ( j == 0 ) ) ; 
} 
void zprintoctal ( c ) 
byte c ; 
{schar j  ; 
  (void) putc('\'' , stdout);
  {register integer for_end; j = 0 ; for_end = 2 ; if ( j <= for_end) do 
    {
      dig [ j ] = c % 8 ; 
      c = c / 8 ; 
    } 
  while ( j++ < for_end ) ; } 
  printdigs ( 3 ) ; 
} 
void outln ( ) 
{schar l  ; 
  (void) putc('\n',  plfile );
  {register integer for_end; l = 1 ; for_end = level ; if ( l <= for_end) do 
    (void) Fputs( plfile ,  "   " ) ; 
  while ( l++ < for_end ) ; } 
} 
void left ( ) 
{level = level + 1 ; 
  (void) putc( '(' ,  plfile );
} 
void right ( ) 
{level = level - 1 ; 
  (void) putc( ')' ,  plfile );
  outln () ; 
} 
void zoutBCPL ( k ) 
index k ; 
{schar l  ; 
  (void) putc( ' ' ,  plfile );
  l = tfm [ k ] ; 
  while ( l > 0 ) {
      
    k = k + 1 ; 
    l = l - 1 ; 
    switch ( tfm [ k ] / 32 ) 
    {case 1 : 
      (void) putc( ASCII04 [ 1 + ( tfm [ k ] % 32 ) ] ,  plfile );
      break ; 
    case 2 : 
      (void) putc( ASCII10 [ 1 + ( tfm [ k ] % 32 ) ] ,  plfile );
      break ; 
    case 3 : 
      (void) putc( ASCII14 [ 1 + ( tfm [ k ] % 32 ) ] ,  plfile );
      break ; 
    } 
  } 
} 
void zoutoctal ( k , l ) 
index k ; 
index l ; 
{short a  ; 
  schar b  ; 
  schar j  ; 
  (void) Fputs( plfile ,  " O " ) ; 
  a = 0 ; 
  b = 0 ; 
  j = 0 ; 
  while ( l > 0 ) {
      
    l = l - 1 ; 
    if ( tfm [ k + l ] != 0 ) 
    {
      while ( b > 2 ) {
	  
	dig [ j ] = a % 8 ; 
	a = a / 8 ; 
	b = b - 3 ; 
	j = j + 1 ; 
      } 
      switch ( b ) 
      {case 0 : 
	a = tfm [ k + l ] ; 
	break ; 
      case 1 : 
	a = a + 2 * tfm [ k + l ] ; 
	break ; 
      case 2 : 
	a = a + 4 * tfm [ k + l ] ; 
	break ; 
      } 
    } 
    b = b + 8 ; 
  } 
  while ( ( a > 0 ) || ( j == 0 ) ) {
      
    dig [ j ] = a % 8 ; 
    a = a / 8 ; 
    j = j + 1 ; 
  } 
  outdigs ( j ) ; 
} 
void zoutchar ( c ) 
byte c ; 
{if ( ( fonttype > 0 ) || ( charcodeformat == charcodeoctal ) ) 
  {
    tfm [ 0 ] = c ; 
    outoctal ( 0 , 1 ) ; 
  } 
  else if ( ( charcodeformat == charcodeascii ) && ( c > 32 ) && ( c <= 126 ) 
  && ( c != 40 ) && ( c != 41 ) ) 
  (void) fprintf( plfile , "%s%c",  " C " , ASCIIall [ c - 31 ] ) ; 
  else if ( ( c >= 48 ) && ( c <= 57 ) ) 
  (void) fprintf( plfile , "%s%ld",  " C " , (long)c - 48 ) ; 
  else if ( ( c >= 65 ) && ( c <= 90 ) ) 
  (void) fprintf( plfile , "%s%c",  " C " , ASCII10 [ c - 63 ] ) ; 
  else if ( ( c >= 97 ) && ( c <= 122 ) ) 
  (void) fprintf( plfile , "%s%c",  " C " , ASCII14 [ c - 95 ] ) ; 
  else {
      
    tfm [ 0 ] = c ; 
    outoctal ( 0 , 1 ) ; 
  } 
} 
void zoutface ( k ) 
index k ; 
{schar s  ; 
  schar b  ; 
  if ( tfm [ k ] >= 18 ) 
  outoctal ( k , 1 ) ; 
  else {
      
    (void) Fputs( plfile ,  " F " ) ; 
    s = tfm [ k ] % 2 ; 
    b = tfm [ k ] / 2 ; 
    putbyte ( MBLstring [ 1 + ( b % 3 ) ] , plfile ) ; 
    putbyte ( RIstring [ 1 + s ] , plfile ) ; 
    putbyte ( RCEstring [ 1 + ( b / 3 ) ] , plfile ) ; 
  } 
} 
void zoutfix ( k ) 
index k ; 
{short a  ; 
  integer f  ; 
  schar j  ; 
  integer delta  ; 
  (void) Fputs( plfile ,  " R " ) ; 
  a = ( tfm [ k ] * 16 ) + ( tfm [ k + 1 ] / 16 ) ; 
  f = ( ( tfm [ k + 1 ] % 16 ) * toint ( 256 ) + tfm [ k + 2 ] ) * 256 + tfm [ 
  k + 3 ] ; 
  if ( a > 2047 ) 
  {
    (void) putc( '-' ,  plfile );
    a = 4096 - a ; 
    if ( f > 0 ) 
    {
      f = 1048576L - f ; 
      a = a - 1 ; 
    } 
  } 
  {
    j = 0 ; 
    do {
	dig [ j ] = a % 10 ; 
      a = a / 10 ; 
      j = j + 1 ; 
    } while ( ! ( a == 0 ) ) ; 
    outdigs ( j ) ; 
  } 
  {
    (void) putc( '.' ,  plfile );
    f = 10 * f + 5 ; 
    delta = 10 ; 
    do {
	if ( delta > 1048576L ) 
      f = f + 524288L - ( delta / 2 ) ; 
      (void) fprintf( plfile , "%ld",  (long)f / 1048576L ) ; 
      f = 10 * ( f % 1048576L ) ; 
      delta = delta * 10 ; 
    } while ( ! ( f <= delta ) ) ; 
  } 
} 
void zcheckBCPL ( k , l ) 
index k ; 
index l ; 
{index j  ; 
  byte c  ; 
  if ( tfm [ k ] >= l ) 
  {
    {
      perfect = false ; 
      if ( charsonline > 0 ) 
      (void) fprintf(stdout, "%c\n", ' ' ) ; 
      charsonline = 0 ; 
      (void) fprintf(stdout, "%s%s\n", "Bad TFM file: " ,       "String is too long; I've shortened it drastically." ) ; 
    } 
    tfm [ k ] = 1 ; 
  } 
  {register integer for_end; j = k + 1 ; for_end = k + tfm [ k ] ; if ( j <= 
  for_end) do 
    {
      c = tfm [ j ] ; 
      if ( ( c == 40 ) || ( c == 41 ) ) 
      {
	{
	  perfect = false ; 
	  if ( charsonline > 0 ) 
	  (void) fprintf(stdout, "%c\n", ' ' ) ; 
	  charsonline = 0 ; 
	  (void) fprintf(stdout, "%s%s\n", "Bad TFM file: " ,           "Parenthesis in string has been changed to slash." ) ; 
	} 
	tfm [ j ] = 47 ; 
      } 
      else if ( ( c < 32 ) || ( c > 126 ) ) 
      {
	{
	  perfect = false ; 
	  if ( charsonline > 0 ) 
	  (void) fprintf(stdout, "%c\n", ' ' ) ; 
	  charsonline = 0 ; 
	  (void) fprintf(stdout, "%s%s\n", "Bad TFM file: " ,           "Nonstandard ASCII code has been blotted out." ) ; 
	} 
	tfm [ j ] = 63 ; 
      } 
      else if ( ( c >= 97 ) && ( c <= 122 ) ) 
      tfm [ j ] = c - 32 ; 
    } 
  while ( j++ < for_end ) ; } 
} 
void hashinput ( ) 
{/* 30 */ schar cc  ; 
  unsigned char zz  ; 
  unsigned char y  ; 
  integer key  ; 
  integer t  ; 
  if ( hashptr == hashsize ) 
  goto lab30 ; 
  k = 4 * ( ligkernbase + ( i ) ) ; 
  y = tfm [ k + 1 ] ; 
  t = tfm [ k + 2 ] ; 
  cc = 0 ; 
  zz = tfm [ k + 3 ] ; 
  if ( t >= 128 ) 
  zz = y ; 
  else {
      
    switch ( t ) 
    {case 0 : 
    case 6 : 
      ; 
      break ; 
    case 5 : 
    case 11 : 
      zz = y ; 
      break ; 
    case 1 : 
    case 7 : 
      cc = 1 ; 
      break ; 
    case 2 : 
      cc = 2 ; 
      break ; 
    case 3 : 
      cc = 3 ; 
      break ; 
    } 
  } 
  key = 256 * c + y + 1 ; 
  h = ( 1009 * key ) % hashsize ; 
  while ( hash [ h ] > 0 ) {
      
    if ( hash [ h ] <= key ) 
    {
      if ( hash [ h ] == key ) 
      goto lab30 ; 
      t = hash [ h ] ; 
      hash [ h ] = key ; 
      key = t ; 
      t = class [ h ] ; 
      class [ h ] = cc ; 
      cc = t ; 
      t = ligz [ h ] ; 
      ligz [ h ] = zz ; 
      zz = t ; 
    } 
    if ( h > 0 ) 
    h = h - 1 ; 
    else h = hashsize ; 
  } 
  hash [ h ] = key ; 
  class [ h ] = cc ; 
  ligz [ h ] = zz ; 
  hashptr = hashptr + 1 ; 
  hashlist [ hashptr ] = h ; 
  lab30: ; 
} 
#ifdef notdef
index zffn ( h , x , y ) 
index h ; 
index x ; 
index y ; 
{register index Result; ; 
  return(Result) ; 
} 
#endif /* notdef */
index zeval ( x , y ) 
index x ; 
index y ; 
{register index Result; integer key  ; 
  key = 256 * x + y + 1 ; 
  h = ( 1009 * key ) % hashsize ; 
  while ( hash [ h ] > key ) if ( h > 0 ) 
  h = h - 1 ; 
  else h = hashsize ; 
  if ( hash [ h ] < key ) 
  Result = y ; 
  else Result = ffn ( h , x , y ) ; 
  return(Result) ; 
} 
index zffn ( h , x , y ) 
index h ; 
index x ; 
index y ; 
{register index Result; switch ( class [ h ] ) 
  {case 0 : 
    ; 
    break ; 
  case 1 : 
    {
      class [ h ] = 4 ; 
      ligz [ h ] = eval ( ligz [ h ] , y ) ; 
      class [ h ] = 0 ; 
    } 
    break ; 
  case 2 : 
    {
      class [ h ] = 4 ; 
      ligz [ h ] = eval ( x , ligz [ h ] ) ; 
      class [ h ] = 0 ; 
    } 
    break ; 
  case 3 : 
    {
      class [ h ] = 4 ; 
      ligz [ h ] = eval ( eval ( x , ligz [ h ] ) , y ) ; 
      class [ h ] = 0 ; 
    } 
    break ; 
  case 4 : 
    {
      xligcycle = x ; 
      yligcycle = y ; 
      ligz [ h ] = 257 ; 
      class [ h ] = 0 ; 
    } 
    break ; 
  } 
  Result = ligz [ h ] ; 
  return(Result) ; 
} 
boolean organize ( ) 
{/* 9999 30 */ register boolean Result; index tfmptr  ; 
  read ( tfmfile , tfm [ 0 ] ) ; 
  if ( tfm [ 0 ] > 127 ) 
  {
    (void) fprintf(stdout, "%s\n", "The first byte of the input file exceeds 127!" ) ; 
    (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
    goto lab9999 ; 
  } 
  if ( eof ( tfmfile ) ) 
  {
    (void) fprintf(stdout, "%s\n", "The input file is only one byte long!" ) ; 
    (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
    goto lab9999 ; 
  } 
  read ( tfmfile , tfm [ 1 ] ) ; 
  lf = tfm [ 0 ] * 256 + tfm [ 1 ] ; 
  if ( lf == 0 ) 
  {
    (void) fprintf(stdout, "%s\n", "The file claims to have length zero, but that's impossible!" ) 
    ; 
    (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
    goto lab9999 ; 
  } 
  tfmfilearray = casttobytepointer ( xrealloc ( tfmfilearray , 4 * lf + 1001 ) 
  ) ; 
  {register integer for_end; tfmptr = 2 ; for_end = 4 * lf - 1 ; if ( tfmptr 
  <= for_end) do 
    {
      if ( eof ( tfmfile ) ) 
      {
	(void) fprintf(stdout, "%s\n", "The file has fewer bytes than it claims!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	goto lab9999 ; 
      } 
      read ( tfmfile , tfm [ tfmptr ] ) ; 
    } 
  while ( tfmptr++ < for_end ) ; } 
  if ( ! eof ( tfmfile ) ) 
  {
    (void) fprintf(stdout, "%s\n", "There's some extra junk at the end of the TFM file," ) ; 
    (void) fprintf(stdout, "%s\n", "but I'll proceed as if it weren't there." ) ; 
  } 
  {
    tfmptr = 2 ; 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	goto lab9999 ; 
      } 
      lh = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	goto lab9999 ; 
      } 
      bc = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	goto lab9999 ; 
      } 
      ec = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	goto lab9999 ; 
      } 
      nw = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	goto lab9999 ; 
      } 
      nh = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	goto lab9999 ; 
      } 
      nd = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	goto lab9999 ; 
      } 
      ni = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	goto lab9999 ; 
      } 
      nl = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	goto lab9999 ; 
      } 
      nk = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	goto lab9999 ; 
      } 
      ne = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	goto lab9999 ; 
      } 
      np = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    if ( lh < 2 ) 
    {
      (void) fprintf(stdout, "%s%ld%c\n", "The header length is only " , (long)lh , '!' ) ; 
      (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
      goto lab9999 ; 
    } 
    if ( nl > 4 * ligsize ) 
    {
      (void) fprintf(stdout, "%s\n", "The lig/kern program is longer than I can handle!" ) ; 
      (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
      goto lab9999 ; 
    } 
    if ( ( bc > ec + 1 ) || ( ec > 255 ) ) 
    {
      (void) fprintf(stdout, "%s%ld%s%ld%s\n", "The character code range " , (long)bc , ".." , (long)ec , "is illegal!" ) 
      ; 
      (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
      goto lab9999 ; 
    } 
    if ( ( nw == 0 ) || ( nh == 0 ) || ( nd == 0 ) || ( ni == 0 ) ) 
    {
      (void) fprintf(stdout, "%s\n", "Incomplete subfiles for character dimensions!" ) ; 
      (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
      goto lab9999 ; 
    } 
    if ( ne > 256 ) 
    {
      (void) fprintf(stdout, "%s%ld%s\n", "There are " , (long)ne , " extensible recipes!" ) ; 
      (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
      goto lab9999 ; 
    } 
    if ( lf != 6 + lh + ( ec - bc + 1 ) + nw + nh + nd + ni + nl + nk + ne + 
    np ) 
    {
      (void) fprintf(stdout, "%s\n", "Subfile sizes don't add up to the stated total!" ) ; 
      (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
      goto lab9999 ; 
    } 
  } 
  {
    charbase = 6 + lh - bc ; 
    widthbase = charbase + ec + 1 ; 
    heightbase = widthbase + nw ; 
    depthbase = heightbase + nh ; 
    italicbase = depthbase + nd ; 
    ligkernbase = italicbase + ni ; 
    kernbase = ligkernbase + nl ; 
    extenbase = kernbase + nk ; 
    parambase = extenbase + ne - 1 ; 
  } 
  Result = true ; 
  goto lab30 ; 
  lab9999: Result = false ; 
  lab30: ; 
  return(Result) ; 
} 
void dosimplethings ( ) 
{short i  ; 
  {
    fonttype = 0 ; 
    if ( lh >= 12 ) 
    {
      {
	checkBCPL ( 32 , 40 ) ; 
	if ( ( tfm [ 32 ] >= 11 ) && ( tfm [ 33 ] == 84 ) && ( tfm [ 34 ] == 
	69 ) && ( tfm [ 35 ] == 88 ) && ( tfm [ 36 ] == 32 ) && ( tfm [ 37 ] 
	== 77 ) && ( tfm [ 38 ] == 65 ) && ( tfm [ 39 ] == 84 ) && ( tfm [ 40 
	] == 72 ) && ( tfm [ 41 ] == 32 ) ) 
	{
	  if ( ( tfm [ 42 ] == 83 ) && ( tfm [ 43 ] == 89 ) ) 
	  fonttype = 1 ; 
	  else if ( ( tfm [ 42 ] == 69 ) && ( tfm [ 43 ] == 88 ) ) 
	  fonttype = 2 ; 
	} 
      } 
      if ( lh >= 17 ) 
      {
	left () ; 
	(void) Fputs( plfile ,  "FAMILY" ) ; 
	checkBCPL ( 72 , 20 ) ; 
	outBCPL ( 72 ) ; 
	right () ; 
	if ( lh >= 18 ) 
	{
	  left () ; 
	  (void) Fputs( plfile ,  "FACE" ) ; 
	  outface ( 95 ) ; 
	  right () ; 
	  {register integer for_end; i = 18 ; for_end = lh - 1 ; if ( i <= 
	  for_end) do 
	    {
	      left () ; 
	      (void) fprintf( plfile , "%s%ld",  "HEADER D " , (long)i ) ; 
	      outoctal ( 24 + 4 * i , 4 ) ; 
	      right () ; 
	    } 
	  while ( i++ < for_end ) ; } 
	} 
      } 
      left () ; 
      (void) Fputs( plfile ,  "CODINGSCHEME" ) ; 
      outBCPL ( 32 ) ; 
      right () ; 
    } 
    left () ; 
    (void) Fputs( plfile ,  "DESIGNSIZE" ) ; 
    if ( tfm [ 28 ] > 127 ) 
    {
      {
	perfect = false ; 
	if ( charsonline > 0 ) 
	(void) fprintf(stdout, "%c\n", ' ' ) ; 
	charsonline = 0 ; 
	(void) fprintf(stdout, "%s%s%s%c\n", "Bad TFM file: " , "Design size " , "negative" , '!' ) ; 
      } 
      (void) fprintf(stdout, "%s\n", "I've set it to 10 points." ) ; 
      (void) Fputs( plfile ,  " D 10" ) ; 
    } 
    else if ( ( tfm [ 28 ] == 0 ) && ( tfm [ 29 ] < 16 ) ) 
    {
      {
	perfect = false ; 
	if ( charsonline > 0 ) 
	(void) fprintf(stdout, "%c\n", ' ' ) ; 
	charsonline = 0 ; 
	(void) fprintf(stdout, "%s%s%s%c\n", "Bad TFM file: " , "Design size " , "too small" , '!' ) ; 
      } 
      (void) fprintf(stdout, "%s\n", "I've set it to 10 points." ) ; 
      (void) Fputs( plfile ,  " D 10" ) ; 
    } 
    else outfix ( 28 ) ; 
    right () ; 
    (void) Fputs( plfile ,  "(COMMENT DESIGNSIZE IS IN POINTS)" ) ; 
    outln () ; 
    (void) Fputs( plfile ,  "(COMMENT OTHER SIZES ARE MULTIPLES OF DESIGNSIZE)" ) ; 
    outln () ; 
    left () ; 
    (void) Fputs( plfile ,  "CHECKSUM" ) ; 
    outoctal ( 24 , 4 ) ; 
    right () ; 
    if ( ( lh > 17 ) && ( tfm [ 92 ] > 127 ) ) 
    {
      left () ; 
      (void) Fputs( plfile ,  "SEVENBITSAFEFLAG TRUE" ) ; 
      right () ; 
    } 
  } 
  if ( np > 0 ) 
  {
    left () ; 
    (void) Fputs( plfile ,  "FONTDIMEN" ) ; 
    outln () ; 
    {register integer for_end; i = 1 ; for_end = np ; if ( i <= for_end) do 
      {
	left () ; 
	if ( i == 1 ) 
	(void) Fputs( plfile ,  "SLANT" ) ; 
	else {
	    
	  if ( ( tfm [ 4 * ( parambase + i ) ] > 0 ) && ( tfm [ 4 * ( 
	  parambase + i ) ] < 255 ) ) 
	  {
	    tfm [ 4 * ( parambase + i ) ] = 0 ; 
	    tfm [ ( 4 * ( parambase + i ) ) + 1 ] = 0 ; 
	    tfm [ ( 4 * ( parambase + i ) ) + 2 ] = 0 ; 
	    tfm [ ( 4 * ( parambase + i ) ) + 3 ] = 0 ; 
	    {
	      perfect = false ; 
	      if ( charsonline > 0 ) 
	      (void) fprintf(stdout, "%c\n", ' ' ) ; 
	      charsonline = 0 ; 
	      (void) fprintf(stdout, "%s%s%c%ld%s\n", "Bad TFM file: " , "Parameter" , ' ' , (long)i ,               " is too big;" ) ; 
	    } 
	    (void) fprintf(stdout, "%s\n", "I have set it to zero." ) ; 
	  } 
	  if ( i <= 7 ) 
	  switch ( i ) 
	  {case 2 : 
	    (void) Fputs( plfile ,  "SPACE" ) ; 
	    break ; 
	  case 3 : 
	    (void) Fputs( plfile ,  "STRETCH" ) ; 
	    break ; 
	  case 4 : 
	    (void) Fputs( plfile ,  "SHRINK" ) ; 
	    break ; 
	  case 5 : 
	    (void) Fputs( plfile ,  "XHEIGHT" ) ; 
	    break ; 
	  case 6 : 
	    (void) Fputs( plfile ,  "QUAD" ) ; 
	    break ; 
	  case 7 : 
	    (void) Fputs( plfile ,  "EXTRASPACE" ) ; 
	    break ; 
	  } 
	  else if ( ( i <= 22 ) && ( fonttype == 1 ) ) 
	  switch ( i ) 
	  {case 8 : 
	    (void) Fputs( plfile ,  "NUM1" ) ; 
	    break ; 
	  case 9 : 
	    (void) Fputs( plfile ,  "NUM2" ) ; 
	    break ; 
	  case 10 : 
	    (void) Fputs( plfile ,  "NUM3" ) ; 
	    break ; 
	  case 11 : 
	    (void) Fputs( plfile ,  "DENOM1" ) ; 
	    break ; 
	  case 12 : 
	    (void) Fputs( plfile ,  "DENOM2" ) ; 
	    break ; 
	  case 13 : 
	    (void) Fputs( plfile ,  "SUP1" ) ; 
	    break ; 
	  case 14 : 
	    (void) Fputs( plfile ,  "SUP2" ) ; 
	    break ; 
	  case 15 : 
	    (void) Fputs( plfile ,  "SUP3" ) ; 
	    break ; 
	  case 16 : 
	    (void) Fputs( plfile ,  "SUB1" ) ; 
	    break ; 
	  case 17 : 
	    (void) Fputs( plfile ,  "SUB2" ) ; 
	    break ; 
	  case 18 : 
	    (void) Fputs( plfile ,  "SUPDROP" ) ; 
	    break ; 
	  case 19 : 
	    (void) Fputs( plfile ,  "SUBDROP" ) ; 
	    break ; 
	  case 20 : 
	    (void) Fputs( plfile ,  "DELIM1" ) ; 
	    break ; 
	  case 21 : 
	    (void) Fputs( plfile ,  "DELIM2" ) ; 
	    break ; 
	  case 22 : 
	    (void) Fputs( plfile ,  "AXISHEIGHT" ) ; 
	    break ; 
	  } 
	  else if ( ( i <= 13 ) && ( fonttype == 2 ) ) 
	  if ( i == 8 ) 
	  (void) Fputs( plfile ,  "DEFAULTRULETHICKNESS" ) ; 
	  else
	  (void) fprintf( plfile , "%s%ld",  "BIGOPSPACING" , (long)i - 8 ) ; 
	  else
	  (void) fprintf( plfile , "%s%ld",  "PARAMETER D " , (long)i ) ; 
	} 
	outfix ( 4 * ( parambase + i ) ) ; 
	right () ; 
      } 
    while ( i++ < for_end ) ; } 
    right () ; 
  } 
  if ( ( fonttype == 1 ) && ( np != 22 ) ) 
  (void) fprintf(stdout, "%s%ld%s\n", "Unusual number of fontdimen parameters for a math symbols font ("   , (long)np , " not 22)." ) ; 
  else if ( ( fonttype == 2 ) && ( np != 13 ) ) 
  (void) fprintf(stdout, "%s%ld%s\n", "Unusual number of fontdimen parameters for an extension font (" ,   (long)np , " not 13)." ) ; 
  if ( ( tfm [ 4 * widthbase ] > 0 ) || ( tfm [ 4 * widthbase + 1 ] > 0 ) || ( 
  tfm [ 4 * widthbase + 2 ] > 0 ) || ( tfm [ 4 * widthbase + 3 ] > 0 ) ) 
  {
    perfect = false ; 
    if ( charsonline > 0 ) 
    (void) fprintf(stdout, "%c\n", ' ' ) ; 
    charsonline = 0 ; 
    (void) fprintf(stdout, "%s%s\n", "Bad TFM file: " , "width[0] should be zero." ) ; 
  } 
  if ( ( tfm [ 4 * heightbase ] > 0 ) || ( tfm [ 4 * heightbase + 1 ] > 0 ) || 
  ( tfm [ 4 * heightbase + 2 ] > 0 ) || ( tfm [ 4 * heightbase + 3 ] > 0 ) ) 
  {
    perfect = false ; 
    if ( charsonline > 0 ) 
    (void) fprintf(stdout, "%c\n", ' ' ) ; 
    charsonline = 0 ; 
    (void) fprintf(stdout, "%s%s\n", "Bad TFM file: " , "height[0] should be zero." ) ; 
  } 
  if ( ( tfm [ 4 * depthbase ] > 0 ) || ( tfm [ 4 * depthbase + 1 ] > 0 ) || ( 
  tfm [ 4 * depthbase + 2 ] > 0 ) || ( tfm [ 4 * depthbase + 3 ] > 0 ) ) 
  {
    perfect = false ; 
    if ( charsonline > 0 ) 
    (void) fprintf(stdout, "%c\n", ' ' ) ; 
    charsonline = 0 ; 
    (void) fprintf(stdout, "%s%s\n", "Bad TFM file: " , "depth[0] should be zero." ) ; 
  } 
  if ( ( tfm [ 4 * italicbase ] > 0 ) || ( tfm [ 4 * italicbase + 1 ] > 0 ) || 
  ( tfm [ 4 * italicbase + 2 ] > 0 ) || ( tfm [ 4 * italicbase + 3 ] > 0 ) ) 
  {
    perfect = false ; 
    if ( charsonline > 0 ) 
    (void) fprintf(stdout, "%c\n", ' ' ) ; 
    charsonline = 0 ; 
    (void) fprintf(stdout, "%s%s\n", "Bad TFM file: " , "italic[0] should be zero." ) ; 
  } 
  {register integer for_end; i = 0 ; for_end = nw - 1 ; if ( i <= for_end) do 
    if ( ( tfm [ 4 * ( widthbase + i ) ] > 0 ) && ( tfm [ 4 * ( widthbase + i 
    ) ] < 255 ) ) 
    {
      tfm [ 4 * ( widthbase + i ) ] = 0 ; 
      tfm [ ( 4 * ( widthbase + i ) ) + 1 ] = 0 ; 
      tfm [ ( 4 * ( widthbase + i ) ) + 2 ] = 0 ; 
      tfm [ ( 4 * ( widthbase + i ) ) + 3 ] = 0 ; 
      {
	perfect = false ; 
	if ( charsonline > 0 ) 
	(void) fprintf(stdout, "%c\n", ' ' ) ; 
	charsonline = 0 ; 
	(void) fprintf(stdout, "%s%s%c%ld%s\n", "Bad TFM file: " , "Width" , ' ' , (long)i , " is too big;" ) ; 
      } 
      (void) fprintf(stdout, "%s\n", "I have set it to zero." ) ; 
    } 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 0 ; for_end = nh - 1 ; if ( i <= for_end) do 
    if ( ( tfm [ 4 * ( heightbase + i ) ] > 0 ) && ( tfm [ 4 * ( heightbase + 
    i ) ] < 255 ) ) 
    {
      tfm [ 4 * ( heightbase + i ) ] = 0 ; 
      tfm [ ( 4 * ( heightbase + i ) ) + 1 ] = 0 ; 
      tfm [ ( 4 * ( heightbase + i ) ) + 2 ] = 0 ; 
      tfm [ ( 4 * ( heightbase + i ) ) + 3 ] = 0 ; 
      {
	perfect = false ; 
	if ( charsonline > 0 ) 
	(void) fprintf(stdout, "%c\n", ' ' ) ; 
	charsonline = 0 ; 
	(void) fprintf(stdout, "%s%s%c%ld%s\n", "Bad TFM file: " , "Height" , ' ' , (long)i , " is too big;" ) ; 
      } 
      (void) fprintf(stdout, "%s\n", "I have set it to zero." ) ; 
    } 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 0 ; for_end = nd - 1 ; if ( i <= for_end) do 
    if ( ( tfm [ 4 * ( depthbase + i ) ] > 0 ) && ( tfm [ 4 * ( depthbase + i 
    ) ] < 255 ) ) 
    {
      tfm [ 4 * ( depthbase + i ) ] = 0 ; 
      tfm [ ( 4 * ( depthbase + i ) ) + 1 ] = 0 ; 
      tfm [ ( 4 * ( depthbase + i ) ) + 2 ] = 0 ; 
      tfm [ ( 4 * ( depthbase + i ) ) + 3 ] = 0 ; 
      {
	perfect = false ; 
	if ( charsonline > 0 ) 
	(void) fprintf(stdout, "%c\n", ' ' ) ; 
	charsonline = 0 ; 
	(void) fprintf(stdout, "%s%s%c%ld%s\n", "Bad TFM file: " , "Depth" , ' ' , (long)i , " is too big;" ) ; 
      } 
      (void) fprintf(stdout, "%s\n", "I have set it to zero." ) ; 
    } 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 0 ; for_end = ni - 1 ; if ( i <= for_end) do 
    if ( ( tfm [ 4 * ( italicbase + i ) ] > 0 ) && ( tfm [ 4 * ( italicbase + 
    i ) ] < 255 ) ) 
    {
      tfm [ 4 * ( italicbase + i ) ] = 0 ; 
      tfm [ ( 4 * ( italicbase + i ) ) + 1 ] = 0 ; 
      tfm [ ( 4 * ( italicbase + i ) ) + 2 ] = 0 ; 
      tfm [ ( 4 * ( italicbase + i ) ) + 3 ] = 0 ; 
      {
	perfect = false ; 
	if ( charsonline > 0 ) 
	(void) fprintf(stdout, "%c\n", ' ' ) ; 
	charsonline = 0 ; 
	(void) fprintf(stdout, "%s%s%c%ld%s\n", "Bad TFM file: " , "Italic correction" , ' ' , (long)i ,         " is too big;" ) ; 
      } 
      (void) fprintf(stdout, "%s\n", "I have set it to zero." ) ; 
    } 
  while ( i++ < for_end ) ; } 
  if ( nk > 0 ) 
  {register integer for_end; i = 0 ; for_end = nk - 1 ; if ( i <= for_end) do 
    if ( ( tfm [ 4 * ( kernbase + i ) ] > 0 ) && ( tfm [ 4 * ( kernbase + i ) 
    ] < 255 ) ) 
    {
      tfm [ 4 * ( kernbase + i ) ] = 0 ; 
      tfm [ ( 4 * ( kernbase + i ) ) + 1 ] = 0 ; 
      tfm [ ( 4 * ( kernbase + i ) ) + 2 ] = 0 ; 
      tfm [ ( 4 * ( kernbase + i ) ) + 3 ] = 0 ; 
      {
	perfect = false ; 
	if ( charsonline > 0 ) 
	(void) fprintf(stdout, "%c\n", ' ' ) ; 
	charsonline = 0 ; 
	(void) fprintf(stdout, "%s%s%c%ld%s\n", "Bad TFM file: " , "Kern" , ' ' , (long)i , " is too big;" ) ; 
      } 
      (void) fprintf(stdout, "%s\n", "I have set it to zero." ) ; 
    } 
  while ( i++ < for_end ) ; } 
} 
void docharacters ( ) 
{byte c  ; 
  index k  ; 
  integer ai  ; 
  sortptr = 0 ; 
  {register integer for_end; c = bc ; for_end = ec ; if ( c <= for_end) do 
    if ( tfm [ 4 * ( charbase + c ) ] > 0 ) 
    {
      if ( charsonline == 8 ) 
      {
	(void) fprintf(stdout, "%c\n", ' ' ) ; 
	charsonline = 1 ; 
      } 
      else {
	  
	if ( charsonline > 0 ) 
	(void) putc(' ' , stdout);
	if ( verbose ) 
	charsonline = charsonline + 1 ; 
      } 
      if ( verbose ) 
      printoctal ( c ) ; 
      left () ; 
      (void) Fputs( plfile ,  "CHARACTER" ) ; 
      outchar ( c ) ; 
      outln () ; 
      {
	left () ; 
	(void) Fputs( plfile ,  "CHARWD" ) ; 
	if ( tfm [ 4 * ( charbase + c ) ] >= nw ) 
	{
	  perfect = false ; 
	  (void) fprintf(stdout, "%c\n", ' ' ) ; 
	  (void) fprintf(stdout, "%s%s", "Width" , " index for character " ) ; 
	  printoctal ( c ) ; 
	  (void) fprintf(stdout, "%s\n", " is too large;" ) ; 
	  (void) fprintf(stdout, "%s\n", "so I reset it to zero." ) ; 
	} 
	else outfix ( 4 * ( widthbase + tfm [ 4 * ( charbase + c ) ] ) ) ; 
	right () ; 
      } 
      if ( ( tfm [ 4 * ( charbase + c ) + 1 ] / 16 ) > 0 ) 
      if ( ( tfm [ 4 * ( charbase + c ) + 1 ] / 16 ) >= nh ) 
      {
	perfect = false ; 
	(void) fprintf(stdout, "%c\n", ' ' ) ; 
	(void) fprintf(stdout, "%s%s", "Height" , " index for character " ) ; 
	printoctal ( c ) ; 
	(void) fprintf(stdout, "%s\n", " is too large;" ) ; 
	(void) fprintf(stdout, "%s\n", "so I reset it to zero." ) ; 
      } 
      else {
	  
	left () ; 
	(void) Fputs( plfile ,  "CHARHT" ) ; 
	outfix ( 4 * ( heightbase + ( tfm [ 4 * ( charbase + c ) + 1 ] / 16 ) 
	) ) ; 
	right () ; 
      } 
      if ( ( tfm [ 4 * ( charbase + c ) + 1 ] % 16 ) > 0 ) 
      if ( ( tfm [ 4 * ( charbase + c ) + 1 ] % 16 ) >= nd ) 
      {
	perfect = false ; 
	(void) fprintf(stdout, "%c\n", ' ' ) ; 
	(void) fprintf(stdout, "%s%s", "Depth" , " index for character " ) ; 
	printoctal ( c ) ; 
	(void) fprintf(stdout, "%s\n", " is too large;" ) ; 
	(void) fprintf(stdout, "%s\n", "so I reset it to zero." ) ; 
      } 
      else {
	  
	left () ; 
	(void) Fputs( plfile ,  "CHARDP" ) ; 
	outfix ( 4 * ( depthbase + ( tfm [ 4 * ( charbase + c ) + 1 ] % 16 ) ) 
	) ; 
	right () ; 
      } 
      if ( ( tfm [ 4 * ( charbase + c ) + 2 ] / 4 ) > 0 ) 
      if ( ( tfm [ 4 * ( charbase + c ) + 2 ] / 4 ) >= ni ) 
      {
	perfect = false ; 
	(void) fprintf(stdout, "%c\n", ' ' ) ; 
	(void) fprintf(stdout, "%s%s", "Italic correction" , " index for character " ) ; 
	printoctal ( c ) ; 
	(void) fprintf(stdout, "%s\n", " is too large;" ) ; 
	(void) fprintf(stdout, "%s\n", "so I reset it to zero." ) ; 
      } 
      else {
	  
	left () ; 
	(void) Fputs( plfile ,  "CHARIC" ) ; 
	outfix ( 4 * ( italicbase + ( tfm [ 4 * ( charbase + c ) + 2 ] / 4 ) ) 
	) ; 
	right () ; 
      } 
      switch ( ( tfm [ 4 * ( charbase + c ) + 2 ] % 4 ) ) 
      {case 0 : 
	; 
	break ; 
      case 1 : 
	{
	  left () ; 
	  (void) Fputs( plfile ,  "COMMENT" ) ; 
	  outln () ; 
	  i = tfm [ 4 * ( charbase + c ) + 3 ] ; 
	  r = 4 * ( ligkernbase + ( i ) ) ; 
	  if ( tfm [ r ] > 128 ) 
	  i = 256 * tfm [ r + 2 ] + tfm [ r + 3 ] ; 
	  do {
	      { 
	      k = 4 * ( ligkernbase + ( i ) ) ; 
	      if ( tfm [ k ] > 128 ) 
	      {
		if ( 256 * tfm [ k + 2 ] + tfm [ k + 3 ] >= nl ) 
		{
		  perfect = false ; 
		  if ( charsonline > 0 ) 
		  (void) fprintf(stdout, "%c\n", ' ' ) ; 
		  charsonline = 0 ; 
		  (void) fprintf(stdout, "%s%s\n", "Bad TFM file: " ,                   "Ligature unconditional stop command address is too big." ) 
		  ; 
		} 
	      } 
	      else if ( tfm [ k + 2 ] >= 128 ) 
	      {
		if ( ( ( tfm [ k + 1 ] < bc ) || ( tfm [ k + 1 ] > ec ) || ( 
		tfm [ 4 * ( charbase + tfm [ k + 1 ] ) ] == 0 ) ) ) 
		if ( tfm [ k + 1 ] != boundarychar ) 
		{
		  perfect = false ; 
		  if ( charsonline > 0 ) 
		  (void) fprintf(stdout, "%c\n", ' ' ) ; 
		  charsonline = 0 ; 
		  (void) fprintf(stdout, "%s%s%s", "Bad TFM file: " , "Kern step for" ,                   " nonexistent character " ) ; 
		  printoctal ( tfm [ k + 1 ] ) ; 
		  (void) fprintf(stdout, "%c\n", '.' ) ; 
		  tfm [ k + 1 ] = bc ; 
		} 
		left () ; 
		(void) Fputs( plfile ,  "KRN" ) ; 
		outchar ( tfm [ k + 1 ] ) ; 
		r = 256 * ( tfm [ k + 2 ] - 128 ) + tfm [ k + 3 ] ; 
		if ( r >= nk ) 
		{
		  {
		    perfect = false ; 
		    if ( charsonline > 0 ) 
		    (void) fprintf(stdout, "%c\n", ' ' ) ; 
		    charsonline = 0 ; 
		    (void) fprintf(stdout, "%s%s\n", "Bad TFM file: " , "Kern index too large." ) ; 
		  } 
		  (void) Fputs( plfile ,  " R 0.0" ) ; 
		} 
		else outfix ( 4 * ( kernbase + r ) ) ; 
		right () ; 
	      } 
	      else {
		  
		if ( ( ( tfm [ k + 1 ] < bc ) || ( tfm [ k + 1 ] > ec ) || ( 
		tfm [ 4 * ( charbase + tfm [ k + 1 ] ) ] == 0 ) ) ) 
		if ( tfm [ k + 1 ] != boundarychar ) 
		{
		  perfect = false ; 
		  if ( charsonline > 0 ) 
		  (void) fprintf(stdout, "%c\n", ' ' ) ; 
		  charsonline = 0 ; 
		  (void) fprintf(stdout, "%s%s%s", "Bad TFM file: " , "Ligature step for" ,                   " nonexistent character " ) ; 
		  printoctal ( tfm [ k + 1 ] ) ; 
		  (void) fprintf(stdout, "%c\n", '.' ) ; 
		  tfm [ k + 1 ] = bc ; 
		} 
		if ( ( ( tfm [ k + 3 ] < bc ) || ( tfm [ k + 3 ] > ec ) || ( 
		tfm [ 4 * ( charbase + tfm [ k + 3 ] ) ] == 0 ) ) ) 
		{
		  perfect = false ; 
		  if ( charsonline > 0 ) 
		  (void) fprintf(stdout, "%c\n", ' ' ) ; 
		  charsonline = 0 ; 
		  (void) fprintf(stdout, "%s%s%s", "Bad TFM file: " , "Ligature step produces the" ,                   " nonexistent character " ) ; 
		  printoctal ( tfm [ k + 3 ] ) ; 
		  (void) fprintf(stdout, "%c\n", '.' ) ; 
		  tfm [ k + 3 ] = bc ; 
		} 
		left () ; 
		r = tfm [ k + 2 ] ; 
		if ( ( r == 4 ) || ( ( r > 7 ) && ( r != 11 ) ) ) 
		{
		  (void) fprintf(stdout, "%s\n", "Ligature step with nonstandard code changed to LIG" ) ; 
		  r = 0 ; 
		  tfm [ k + 2 ] = 0 ; 
		} 
		if ( r % 4 > 1 ) 
		(void) putc( '/' ,  plfile );
		(void) Fputs( plfile ,  "LIG" ) ; 
		if ( odd ( r ) ) 
		(void) putc( '/' ,  plfile );
		while ( r > 3 ) {
		    
		  (void) putc( '>' ,  plfile );
		  r = r - 4 ; 
		} 
		outchar ( tfm [ k + 1 ] ) ; 
		outchar ( tfm [ k + 3 ] ) ; 
		right () ; 
	      } 
	      if ( tfm [ k ] > 0 ) 
	      if ( level == 1 ) 
	      {
		if ( tfm [ k ] >= 128 ) 
		(void) Fputs( plfile ,  "(STOP)" ) ; 
		else {
		    
		  count = 0 ; 
		  {register integer for_end; ai = i + 1 ; for_end = i + tfm [ 
		  k ] ; if ( ai <= for_end) do 
		    if ( activity [ ai ] == 2 ) 
		    count = count + 1 ; 
		  while ( ai++ < for_end ) ; } 
		  (void) fprintf( plfile , "%s%ld%c",  "(SKIP D " , (long)count , ')' ) ; 
		} 
		outln () ; 
	      } 
	    } 
	    if ( tfm [ k ] >= 128 ) 
	    i = nl ; 
	    else i = i + 1 + tfm [ k ] ; 
	  } while ( ! ( i >= nl ) ) ; 
	  right () ; 
	} 
	break ; 
      case 2 : 
	{
	  r = tfm [ 4 * ( charbase + c ) + 3 ] ; 
	  if ( ( ( r < bc ) || ( r > ec ) || ( tfm [ 4 * ( charbase + r ) ] == 
	  0 ) ) ) 
	  {
	    {
	      perfect = false ; 
	      if ( charsonline > 0 ) 
	      (void) fprintf(stdout, "%c\n", ' ' ) ; 
	      charsonline = 0 ; 
	      (void) fprintf(stdout, "%s%s%s", "Bad TFM file: " , "Character list link to" ,               " nonexistent character " ) ; 
	      printoctal ( r ) ; 
	      (void) fprintf(stdout, "%c\n", '.' ) ; 
	    } 
	    tfm [ 4 * ( charbase + c ) + 2 ] = 4 * ( tfm [ 4 * ( charbase + c 
	    ) + 2 ] / 4 ) + 0 ; 
	  } 
	  else {
	      
	    while ( ( r < c ) && ( ( tfm [ 4 * ( charbase + r ) + 2 ] % 4 ) == 
	    2 ) ) r = tfm [ 4 * ( charbase + r ) + 3 ] ; 
	    if ( r == c ) 
	    {
	      {
		perfect = false ; 
		if ( charsonline > 0 ) 
		(void) fprintf(stdout, "%c\n", ' ' ) ; 
		charsonline = 0 ; 
		(void) fprintf(stdout, "%s%s\n", "Bad TFM file: " , "Cycle in a character list!" ) ; 
	      } 
	      (void) Fputs(stdout, "Character " ) ; 
	      printoctal ( c ) ; 
	      (void) fprintf(stdout, "%s\n", " now ends the list." ) ; 
	      tfm [ 4 * ( charbase + c ) + 2 ] = 4 * ( tfm [ 4 * ( charbase + 
	      c ) + 2 ] / 4 ) + 0 ; 
	    } 
	    else {
		
	      left () ; 
	      (void) Fputs( plfile ,  "NEXTLARGER" ) ; 
	      outchar ( tfm [ 4 * ( charbase + c ) + 3 ] ) ; 
	      right () ; 
	    } 
	  } 
	} 
	break ; 
      case 3 : 
	if ( tfm [ 4 * ( charbase + c ) + 3 ] >= ne ) 
	{
	  {
	    perfect = false ; 
	    (void) fprintf(stdout, "%c\n", ' ' ) ; 
	    (void) fprintf(stdout, "%s%s", "Extensible" , " index for character " ) ; 
	    printoctal ( c ) ; 
	    (void) fprintf(stdout, "%s\n", " is too large;" ) ; 
	    (void) fprintf(stdout, "%s\n", "so I reset it to zero." ) ; 
	  } 
	  tfm [ 4 * ( charbase + c ) + 2 ] = 4 * ( tfm [ 4 * ( charbase + c ) 
	  + 2 ] / 4 ) + 0 ; 
	} 
	else {
	    
	  left () ; 
	  (void) Fputs( plfile ,  "VARCHAR" ) ; 
	  outln () ; 
	  {register integer for_end; k = 0 ; for_end = 3 ; if ( k <= for_end) 
	  do 
	    if ( ( k == 3 ) || ( tfm [ 4 * ( extenbase + tfm [ 4 * ( charbase 
	    + c ) + 3 ] ) + k ] > 0 ) ) 
	    {
	      left () ; 
	      switch ( k ) 
	      {case 0 : 
		(void) Fputs( plfile ,  "TOP" ) ; 
		break ; 
	      case 1 : 
		(void) Fputs( plfile ,  "MID" ) ; 
		break ; 
	      case 2 : 
		(void) Fputs( plfile ,  "BOT" ) ; 
		break ; 
	      case 3 : 
		(void) Fputs( plfile ,  "REP" ) ; 
		break ; 
	      } 
	      if ( ( ( tfm [ 4 * ( extenbase + tfm [ 4 * ( charbase + c ) + 3 
	      ] ) + k ] < bc ) || ( tfm [ 4 * ( extenbase + tfm [ 4 * ( 
	      charbase + c ) + 3 ] ) + k ] > ec ) || ( tfm [ 4 * ( charbase + 
	      tfm [ 4 * ( extenbase + tfm [ 4 * ( charbase + c ) + 3 ] ) + k ] 
	      ) ] == 0 ) ) ) 
	      outchar ( c ) ; 
	      else outchar ( tfm [ 4 * ( extenbase + tfm [ 4 * ( charbase + c 
	      ) + 3 ] ) + k ] ) ; 
	      right () ; 
	    } 
	  while ( k++ < for_end ) ; } 
	  right () ; 
	} 
	break ; 
      } 
      right () ; 
    } 
  while ( c++ < for_end ) ; } 
} 
void main_body() {
    
  initialize () ; 
  if ( ! organize () ) 
  goto lab9999 ; 
  dosimplethings () ; 
  if ( nl > 0 ) 
  {
    {register integer for_end; ai = 0 ; for_end = nl - 1 ; if ( ai <= 
    for_end) do 
      activity [ ai ] = 0 ; 
    while ( ai++ < for_end ) ; } 
    if ( tfm [ 4 * ( ligkernbase + ( 0 ) ) ] == 255 ) 
    {
      left () ; 
      (void) Fputs( plfile ,  "BOUNDARYCHAR" ) ; 
      boundarychar = tfm [ 4 * ( ligkernbase + ( 0 ) ) + 1 ] ; 
      outchar ( boundarychar ) ; 
      right () ; 
      activity [ 0 ] = 1 ; 
    } 
    if ( tfm [ 4 * ( ligkernbase + ( nl - 1 ) ) ] == 255 ) 
    {
      r = 256 * tfm [ 4 * ( ligkernbase + ( nl - 1 ) ) + 2 ] + tfm [ 4 * ( 
      ligkernbase + ( nl - 1 ) ) + 3 ] ; 
      if ( r >= nl ) 
      {
	perfect = false ; 
	(void) fprintf(stdout, "%c\n", ' ' ) ; 
	(void) Fputs(stdout, "Ligature/kern starting index for boundarychar is too large;"         ) ; 
	(void) fprintf(stdout, "%s\n", "so I removed it." ) ; 
      } 
      else {
	  
	labelptr = 1 ; 
	labeltable [ 1 ] .cc = 256 ; 
	labeltable [ 1 ] .rr = r ; 
	bcharlabel = r ; 
	activity [ r ] = 2 ; 
      } 
      activity [ nl - 1 ] = 1 ; 
    } 
  } 
  {register integer for_end; c = bc ; for_end = ec ; if ( c <= for_end) do 
    if ( ( tfm [ 4 * ( charbase + c ) + 2 ] % 4 ) == 1 ) 
    {
      r = tfm [ 4 * ( charbase + c ) + 3 ] ; 
      if ( r < nl ) 
      {
	if ( tfm [ 4 * ( ligkernbase + ( r ) ) ] > 128 ) 
	{
	  r = 256 * tfm [ 4 * ( ligkernbase + ( r ) ) + 2 ] + tfm [ 4 * ( 
	  ligkernbase + ( r ) ) + 3 ] ; 
	  if ( r < nl ) 
	  if ( activity [ tfm [ 4 * ( charbase + c ) + 3 ] ] == 0 ) 
	  activity [ tfm [ 4 * ( charbase + c ) + 3 ] ] = 1 ; 
	} 
      } 
      if ( r >= nl ) 
      {
	perfect = false ; 
	(void) fprintf(stdout, "%c\n", ' ' ) ; 
	(void) Fputs(stdout, "Ligature/kern starting index for character " ) ; 
	printoctal ( c ) ; 
	(void) fprintf(stdout, "%s\n", " is too large;" ) ; 
	(void) fprintf(stdout, "%s\n", "so I removed it." ) ; 
	tfm [ 4 * ( charbase + c ) + 2 ] = 4 * ( tfm [ 4 * ( charbase + c ) + 
	2 ] / 4 ) + 0 ; 
      } 
      else {
	  
	sortptr = labelptr ; 
	while ( labeltable [ sortptr ] .rr > r ) {
	    
	  labeltable [ sortptr + 1 ] = labeltable [ sortptr ] ; 
	  sortptr = sortptr - 1 ; 
	} 
	labeltable [ sortptr + 1 ] .cc = c ; 
	labeltable [ sortptr + 1 ] .rr = r ; 
	labelptr = labelptr + 1 ; 
	activity [ r ] = 2 ; 
      } 
    } 
  while ( c++ < for_end ) ; } 
  labeltable [ labelptr + 1 ] .rr = ligsize ; 
  if ( nl > 0 ) 
  {
    left () ; 
    (void) Fputs( plfile ,  "LIGTABLE" ) ; 
    outln () ; 
    {register integer for_end; ai = 0 ; for_end = nl - 1 ; if ( ai <= 
    for_end) do 
      if ( activity [ ai ] == 2 ) 
      {
	r = tfm [ 4 * ( ligkernbase + ( ai ) ) ] ; 
	if ( r < 128 ) 
	{
	  r = r + ai + 1 ; 
	  if ( r >= nl ) 
	  {
	    {
	      perfect = false ; 
	      if ( charsonline > 0 ) 
	      (void) fprintf(stdout, "%c\n", ' ' ) ; 
	      charsonline = 0 ; 
	      (void) fprintf(stdout, "%s%s%ld%s\n", "Bad TFM file: " , "Ligature/kern step " , (long)ai ,               " skips too far;" ) ; 
	    } 
	    (void) fprintf(stdout, "%s\n", "I made it stop." ) ; 
	    tfm [ 4 * ( ligkernbase + ( ai ) ) ] = 128 ; 
	  } 
	  else activity [ r ] = 2 ; 
	} 
      } 
    while ( ai++ < for_end ) ; } 
    sortptr = 1 ; 
    {register integer for_end; acti = 0 ; for_end = nl - 1 ; if ( acti <= 
    for_end) do 
      if ( activity [ acti ] != 1 ) 
      {
	i = acti ; 
	if ( activity [ i ] == 0 ) 
	{
	  if ( level == 1 ) 
	  {
	    left () ; 
	    (void) Fputs( plfile ,  "COMMENT THIS PART OF THE PROGRAM IS NEVER USED!"             ) ; 
	    outln () ; 
	  } 
	} 
	else if ( level == 2 ) 
	right () ; 
	while ( i == labeltable [ sortptr ] .rr ) {
	    
	  left () ; 
	  (void) Fputs( plfile ,  "LABEL" ) ; 
	  if ( labeltable [ sortptr ] .cc == 256 ) 
	  (void) Fputs( plfile ,  " BOUNDARYCHAR" ) ; 
	  else outchar ( labeltable [ sortptr ] .cc ) ; 
	  right () ; 
	  sortptr = sortptr + 1 ; 
	} 
	{
	  k = 4 * ( ligkernbase + ( i ) ) ; 
	  if ( tfm [ k ] > 128 ) 
	  {
	    if ( 256 * tfm [ k + 2 ] + tfm [ k + 3 ] >= nl ) 
	    {
	      perfect = false ; 
	      if ( charsonline > 0 ) 
	      (void) fprintf(stdout, "%c\n", ' ' ) ; 
	      charsonline = 0 ; 
	      (void) fprintf(stdout, "%s%s\n", "Bad TFM file: " ,               "Ligature unconditional stop command address is too big." ) ; 
	    } 
	  } 
	  else if ( tfm [ k + 2 ] >= 128 ) 
	  {
	    if ( ( ( tfm [ k + 1 ] < bc ) || ( tfm [ k + 1 ] > ec ) || ( tfm [ 
	    4 * ( charbase + tfm [ k + 1 ] ) ] == 0 ) ) ) 
	    if ( tfm [ k + 1 ] != boundarychar ) 
	    {
	      perfect = false ; 
	      if ( charsonline > 0 ) 
	      (void) fprintf(stdout, "%c\n", ' ' ) ; 
	      charsonline = 0 ; 
	      (void) fprintf(stdout, "%s%s%s", "Bad TFM file: " , "Kern step for" ,               " nonexistent character " ) ; 
	      printoctal ( tfm [ k + 1 ] ) ; 
	      (void) fprintf(stdout, "%c\n", '.' ) ; 
	      tfm [ k + 1 ] = bc ; 
	    } 
	    left () ; 
	    (void) Fputs( plfile ,  "KRN" ) ; 
	    outchar ( tfm [ k + 1 ] ) ; 
	    r = 256 * ( tfm [ k + 2 ] - 128 ) + tfm [ k + 3 ] ; 
	    if ( r >= nk ) 
	    {
	      {
		perfect = false ; 
		if ( charsonline > 0 ) 
		(void) fprintf(stdout, "%c\n", ' ' ) ; 
		charsonline = 0 ; 
		(void) fprintf(stdout, "%s%s\n", "Bad TFM file: " , "Kern index too large." ) ; 
	      } 
	      (void) Fputs( plfile ,  " R 0.0" ) ; 
	    } 
	    else outfix ( 4 * ( kernbase + r ) ) ; 
	    right () ; 
	  } 
	  else {
	      
	    if ( ( ( tfm [ k + 1 ] < bc ) || ( tfm [ k + 1 ] > ec ) || ( tfm [ 
	    4 * ( charbase + tfm [ k + 1 ] ) ] == 0 ) ) ) 
	    if ( tfm [ k + 1 ] != boundarychar ) 
	    {
	      perfect = false ; 
	      if ( charsonline > 0 ) 
	      (void) fprintf(stdout, "%c\n", ' ' ) ; 
	      charsonline = 0 ; 
	      (void) fprintf(stdout, "%s%s%s", "Bad TFM file: " , "Ligature step for" ,               " nonexistent character " ) ; 
	      printoctal ( tfm [ k + 1 ] ) ; 
	      (void) fprintf(stdout, "%c\n", '.' ) ; 
	      tfm [ k + 1 ] = bc ; 
	    } 
	    if ( ( ( tfm [ k + 3 ] < bc ) || ( tfm [ k + 3 ] > ec ) || ( tfm [ 
	    4 * ( charbase + tfm [ k + 3 ] ) ] == 0 ) ) ) 
	    {
	      perfect = false ; 
	      if ( charsonline > 0 ) 
	      (void) fprintf(stdout, "%c\n", ' ' ) ; 
	      charsonline = 0 ; 
	      (void) fprintf(stdout, "%s%s%s", "Bad TFM file: " , "Ligature step produces the" ,               " nonexistent character " ) ; 
	      printoctal ( tfm [ k + 3 ] ) ; 
	      (void) fprintf(stdout, "%c\n", '.' ) ; 
	      tfm [ k + 3 ] = bc ; 
	    } 
	    left () ; 
	    r = tfm [ k + 2 ] ; 
	    if ( ( r == 4 ) || ( ( r > 7 ) && ( r != 11 ) ) ) 
	    {
	      (void) fprintf(stdout, "%s\n", "Ligature step with nonstandard code changed to LIG" ) 
	      ; 
	      r = 0 ; 
	      tfm [ k + 2 ] = 0 ; 
	    } 
	    if ( r % 4 > 1 ) 
	    (void) putc( '/' ,  plfile );
	    (void) Fputs( plfile ,  "LIG" ) ; 
	    if ( odd ( r ) ) 
	    (void) putc( '/' ,  plfile );
	    while ( r > 3 ) {
		
	      (void) putc( '>' ,  plfile );
	      r = r - 4 ; 
	    } 
	    outchar ( tfm [ k + 1 ] ) ; 
	    outchar ( tfm [ k + 3 ] ) ; 
	    right () ; 
	  } 
	  if ( tfm [ k ] > 0 ) 
	  if ( level == 1 ) 
	  {
	    if ( tfm [ k ] >= 128 ) 
	    (void) Fputs( plfile ,  "(STOP)" ) ; 
	    else {
		
	      count = 0 ; 
	      {register integer for_end; ai = i + 1 ; for_end = i + tfm [ k ] 
	      ; if ( ai <= for_end) do 
		if ( activity [ ai ] == 2 ) 
		count = count + 1 ; 
	      while ( ai++ < for_end ) ; } 
	      (void) fprintf( plfile , "%s%ld%c",  "(SKIP D " , (long)count , ')' ) ; 
	    } 
	    outln () ; 
	  } 
	} 
      } 
    while ( acti++ < for_end ) ; } 
    if ( level == 2 ) 
    right () ; 
    right () ; 
    hashptr = 0 ; 
    yligcycle = 256 ; 
    {register integer for_end; hh = 0 ; for_end = hashsize ; if ( hh <= 
    for_end) do 
      hash [ hh ] = 0 ; 
    while ( hh++ < for_end ) ; } 
    {register integer for_end; c = bc ; for_end = ec ; if ( c <= for_end) do 
      if ( ( tfm [ 4 * ( charbase + c ) + 2 ] % 4 ) == 1 ) 
      {
	i = tfm [ 4 * ( charbase + c ) + 3 ] ; 
	if ( tfm [ 4 * ( ligkernbase + ( i ) ) ] > 128 ) 
	i = 256 * tfm [ 4 * ( ligkernbase + ( i ) ) + 2 ] + tfm [ 4 * ( 
	ligkernbase + ( i ) ) + 3 ] ; 
	do {
	    hashinput () ; 
	  k = tfm [ 4 * ( ligkernbase + ( i ) ) ] ; 
	  if ( k >= 128 ) 
	  i = nl ; 
	  else i = i + 1 + k ; 
	} while ( ! ( i >= nl ) ) ; 
      } 
    while ( c++ < for_end ) ; } 
    if ( bcharlabel < nl ) 
    {
      c = 256 ; 
      i = bcharlabel ; 
      do {
	  hashinput () ; 
	k = tfm [ 4 * ( ligkernbase + ( i ) ) ] ; 
	if ( k >= 128 ) 
	i = nl ; 
	else i = i + 1 + k ; 
      } while ( ! ( i >= nl ) ) ; 
    } 
    if ( hashptr == hashsize ) 
    {
      (void) fprintf(stdout, "%s\n", "Sorry, I haven't room for so many ligature/kern pairs!" ) ; 
      goto lab9999 ; 
    } 
    {register integer for_end; hh = 1 ; for_end = hashptr ; if ( hh <= 
    for_end) do 
      {
	r = hashlist [ hh ] ; 
	if ( class [ r ] > 0 ) 
	r = ffn ( r , ( hash [ r ] - 1 ) / 256 , ( hash [ r ] - 1 ) % 256 ) ; 
      } 
    while ( hh++ < for_end ) ; } 
    if ( yligcycle < 256 ) 
    {
      (void) Fputs(stdout, "Infinite ligature loop starting with " ) ; 
      if ( xligcycle == 256 ) 
      (void) Fputs(stdout, "boundary" ) ; 
      else printoctal ( xligcycle ) ; 
      (void) Fputs(stdout, " and " ) ; 
      printoctal ( yligcycle ) ; 
      (void) fprintf(stdout, "%c\n", '!' ) ; 
      (void) Fputs( plfile ,  "(INFINITE LIGATURE LOOP MUST BE BROKEN!)" ) ; 
      goto lab9999 ; 
    } 
  } 
  if ( ne > 0 ) 
  {register integer for_end; c = 0 ; for_end = ne - 1 ; if ( c <= for_end) do 
    {register integer for_end; d = 0 ; for_end = 3 ; if ( d <= for_end) do 
      {
	k = 4 * ( extenbase + c ) + d ; 
	if ( ( tfm [ k ] > 0 ) || ( d == 3 ) ) 
	{
	  if ( ( ( tfm [ k ] < bc ) || ( tfm [ k ] > ec ) || ( tfm [ 4 * ( 
	  charbase + tfm [ k ] ) ] == 0 ) ) ) 
	  {
	    {
	      perfect = false ; 
	      if ( charsonline > 0 ) 
	      (void) fprintf(stdout, "%c\n", ' ' ) ; 
	      charsonline = 0 ; 
	      (void) fprintf(stdout, "%s%s%s", "Bad TFM file: " , "Extensible recipe involves the" ,               " nonexistent character " ) ; 
	      printoctal ( tfm [ k ] ) ; 
	      (void) fprintf(stdout, "%c\n", '.' ) ; 
	    } 
	    if ( d < 3 ) 
	    tfm [ k ] = 0 ; 
	  } 
	} 
      } 
    while ( d++ < for_end ) ; } 
  while ( c++ < for_end ) ; } 
  docharacters () ; 
  if ( verbose ) 
  (void) fprintf(stdout, "%c\n", '.' ) ; 
  if ( level != 0 ) 
  (void) fprintf(stdout, "%s\n", "This program isn't working!" ) ; 
  if ( ! perfect ) 
  (void) Fputs( plfile ,    "(COMMENT THE TFM FILE WAS BAD, SO THE DATA HAS BEEN CHANGED!)" ) ; 
  lab9999: ; 
} 
