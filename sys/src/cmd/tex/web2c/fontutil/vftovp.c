#include "config.h"
/* 9999 */ 
#define tfmsize 30000 
#define vfsize 10000 
#define maxfonts 300 
#define ligsize 5000 
#define hashsize 5003 
#define maxstack 50 
#define charcodeascii 0 
#define charcodeoctal 1 
#define charcodedefault 2 
#define noptions 2 
#define argoptions 1 
typedef unsigned char byte  ; 
typedef integer index  ; 
typedef integer charcodeformattype  ; 
file_ptr /* of  byte */ vffile  ; 
char vfname[PATHMAX + 1]  ; 
file_ptr /* of  byte */ tfmfile  ; 
char tfmname[PATHMAX + 1]  ; 
short lf, lh, bc, ec, nw, nh, nd, ni, nl, nk, ne, np  ; 
text vplfile  ; 
char vplname[PATHMAX + 1]  ; 
byte 
#define tfm (zzzaa +1000)
  zzzaa[tfmsize + 1001]  ; 
integer charbase, widthbase, heightbase, depthbase, italicbase, ligkernbase, 
kernbase, extenbase, parambase  ; 
schar fonttype  ; 
byte vf[vfsize + 1]  ; 
integer fontnumber[maxfonts + 1]  ; 
integer fontstart[maxfonts + 1], fontchars[maxfonts + 1]  ; 
integer fontptr  ; 
integer packetstart[256], packetend[256]  ; 
boolean packetfound  ; 
byte tempbyte  ; 
integer count  ; 
real realdsize  ; 
integer pl  ; 
integer vfptr  ; 
integer vfcount  ; 
integer a  ; 
integer l  ; 
char curname[PATHMAX + 1]  ; 
byte b0, b1, b2, b3  ; 
short fontlh  ; 
short fontbc, fontec  ; 
char defaultdirectory[10]  ; 
ccharpointer ASCII04, ASCII10, ASCII14  ; 
char xchr[256]  ; 
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
integer top  ; 
integer wstack[maxstack + 1], xstack[maxstack + 1], ystack[maxstack + 1], 
zstack[maxstack + 1]  ; 
integer vflimit  ; 
byte o  ; 
integer verbose  ; 
charcodeformattype charcodeformat  ; 

#include "vftovp.h"
void initialize ( ) 
{integer k  ; 
  getoptstruct longoptions[noptions + 1]  ; 
  integer getoptreturnval  ; 
  integer optionindex  ; 
  integer currentoption  ; 
  if ( ( argc < 3 ) || ( argc > noptions + argoptions + 4 ) ) 
  {
    (void) Fputs(stdout, "Usage: vftovp " ) ; 
    (void) Fputs(stdout, "[-verbose] " ) ; 
    (void) fprintf(stdout, "%s\n", "[-charcode-format=<format>] " ) ; 
    (void) fprintf(stdout, "%s\n", "  <vfm file> <tfm file> [<vpl file>]." ) ; 
    uexit ( 1 ) ; 
  } 
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
  setpaths ( TFMFILEPATHBIT + VFFILEPATHBIT ) ; 
  argv ( optind , vfname ) ; 
  if ( testreadaccess ( vfname , VFFILEPATH ) ) 
  reset ( vffile , vfname ) ; 
  else {
      
    printpascalstring ( vfname ) ; 
    (void) fprintf(stdout, "%s\n", ": VF file not found." ) ; 
    uexit ( 1 ) ; 
  } 
  argv ( optind + 1 , tfmname ) ; 
  if ( testreadaccess ( tfmname , TFMFILEPATH ) ) 
  reset ( tfmfile , tfmname ) ; 
  else {
      
    printpascalstring ( tfmname ) ; 
    (void) fprintf(stdout, "%s\n", ": TFM file not found." ) ; 
    uexit ( 1 ) ; 
  } 
  if ( verbose ) 
  (void) fprintf(stdout, "%s\n", "This is VFtoVP, C Version 1.2" ) ; 
  if ( optind + 2 == argc ) 
  vplfile = stdout ; 
  else {
      
    argv ( optind + 2 , vplname ) ; 
    rewrite ( vplfile , vplname ) ; 
  } 
  ASCII04 = "  !\"#$%&'()*+,-./0123456789:;<=>?" ; 
  ASCII10 = " @ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_" ; 
  ASCII14 = " `abcdefghijklmnopqrstuvwxyz{|}~?" ; 
  {register integer for_end; k = 0 ; for_end = 255 ; if ( k <= for_end) do 
    xchr [ k ] = '?' ; 
  while ( k++ < for_end ) ; } 
  {register integer for_end; k = 0 ; for_end = 31 ; if ( k <= for_end) do 
    {
      xchr [ k + 32 ] = ASCII04 [ k + 1 ] ; 
      xchr [ k + 64 ] = ASCII10 [ k + 1 ] ; 
      xchr [ k + 96 ] = ASCII14 [ k + 1 ] ; 
    } 
  while ( k++ < for_end ) ; } 
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
void readtfmword ( ) 
{if ( eof ( tfmfile ) ) 
  b0 = 0 ; 
  else read ( tfmfile , b0 ) ; 
  if ( eof ( tfmfile ) ) 
  b1 = 0 ; 
  else read ( tfmfile , b1 ) ; 
  if ( eof ( tfmfile ) ) 
  b2 = 0 ; 
  else read ( tfmfile , b2 ) ; 
  if ( eof ( tfmfile ) ) 
  b3 = 0 ; 
  else read ( tfmfile , b3 ) ; 
} 
integer zvfread ( k ) 
integer k ; 
{register integer Result; byte b  ; 
  integer a  ; 
  vfcount = vfcount + k ; 
  if ( eof ( vffile ) ) 
  b = 0 ; 
  else read ( vffile , b ) ; 
  a = b ; 
  if ( k == 4 ) 
  if ( b >= 128 ) 
  a = a - 256 ; 
  while ( k > 1 ) {
      
    if ( eof ( vffile ) ) 
    b = 0 ; 
    else read ( vffile , b ) ; 
    a = 256 * a + b ; 
    k = k - 1 ; 
  } 
  Result = a ; 
  return(Result) ; 
} 
integer ztfmwidth ( c ) 
byte c ; 
{register integer Result; integer a  ; 
  index k  ; 
  k = 4 * ( widthbase + tfm [ 4 * ( charbase + c ) ] ) ; 
  a = tfm [ k ] ; 
  if ( a >= 128 ) 
  a = a - 256 ; 
  Result = ( ( 256 * a + tfm [ k + 1 ] ) * 256 + tfm [ k + 2 ] ) * 256 + tfm [ 
  k + 3 ] ; 
  return(Result) ; 
} 
void zoutdigs ( j ) 
integer j ; 
{do {
    j = j - 1 ; 
    (void) fprintf( vplfile , "%ld",  (long)dig [ j ] ) ; 
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
  (void) putc('\n',  vplfile );
  {register integer for_end; l = 1 ; for_end = level ; if ( l <= for_end) do 
    (void) Fputs( vplfile ,  "   " ) ; 
  while ( l++ < for_end ) ; } 
} 
void left ( ) 
{level = level + 1 ; 
  (void) putc( '(' ,  vplfile );
} 
void right ( ) 
{level = level - 1 ; 
  (void) putc( ')' ,  vplfile );
  outln () ; 
} 
void zoutBCPL ( k ) 
index k ; 
{schar l  ; 
  (void) putc( ' ' ,  vplfile );
  l = tfm [ k ] ; 
  while ( l > 0 ) {
      
    k = k + 1 ; 
    l = l - 1 ; 
    (void) putc( xchr [ tfm [ k ] ] ,  vplfile );
  } 
} 
void zoutoctal ( k , l ) 
index k ; 
index l ; 
{short a  ; 
  schar b  ; 
  schar j  ; 
  (void) Fputs( vplfile ,  " O " ) ; 
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
  (void) fprintf( vplfile , "%s%c",  " C " , xchr [ c - 31 ] ) ; 
  else if ( ( ( c >= 48 ) && ( c <= 57 ) ) || ( ( c >= 65 ) && ( c <= 90 ) ) 
  || ( ( c >= 97 ) && ( c <= 122 ) ) ) 
  (void) fprintf( vplfile , "%s%c",  " C " , xchr [ c ] ) ; 
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
      
    (void) Fputs( vplfile ,  " F " ) ; 
    s = tfm [ k ] % 2 ; 
    b = tfm [ k ] / 2 ; 
    putbyte ( MBLstring [ 1 + ( b % 3 ) ] , vplfile ) ; 
    putbyte ( RIstring [ 1 + s ] , vplfile ) ; 
    putbyte ( RCEstring [ 1 + ( b / 3 ) ] , vplfile ) ; 
  } 
} 
void zoutfix ( k ) 
index k ; 
{short a  ; 
  integer f  ; 
  schar j  ; 
  integer delta  ; 
  (void) Fputs( vplfile ,  " R " ) ; 
  a = ( tfm [ k ] * 16 ) + ( tfm [ k + 1 ] / 16 ) ; 
  f = ( ( tfm [ k + 1 ] % 16 ) * toint ( 256 ) + tfm [ k + 2 ] ) * 256 + tfm [ 
  k + 3 ] ; 
  if ( a > 2047 ) 
  {
    (void) putc( '-' ,  vplfile );
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
    (void) putc( '.' ,  vplfile );
    f = 10 * f + 5 ; 
    delta = 10 ; 
    do {
	if ( delta > 1048576L ) 
      f = f + 524288L - ( delta / 2 ) ; 
      (void) fprintf( vplfile , "%ld",  (long)f / 1048576L ) ; 
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
{/* 10 */ schar cc  ; 
  unsigned char zz  ; 
  unsigned char y  ; 
  integer key  ; 
  integer t  ; 
  if ( hashptr == hashsize ) 
  goto lab10 ; 
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
      goto lab10 ; 
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
  lab10: ; 
} 
#ifdef notdef
index zligf ( h , x , y ) 
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
  else Result = ligf ( h , x , y ) ; 
  return(Result) ; 
} 
index zligf ( h , x , y ) 
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
boolean zstringbalance ( k , l ) 
integer k ; 
integer l ; 
{/* 45 10 */ register boolean Result; integer j, bal  ; 
  if ( l > 0 ) 
  if ( vf [ k ] == 32 ) 
  goto lab45 ; 
  bal = 0 ; 
  {register integer for_end; j = k ; for_end = k + l - 1 ; if ( j <= for_end) 
  do 
    {
      if ( ( vf [ j ] < 32 ) || ( vf [ j ] >= 127 ) ) 
      goto lab45 ; 
      if ( vf [ j ] == 40 ) 
      bal = bal + 1 ; 
      else if ( vf [ j ] == 41 ) 
      if ( bal == 0 ) 
      goto lab45 ; 
      else bal = bal - 1 ; 
    } 
  while ( j++ < for_end ) ; } 
  if ( bal > 0 ) 
  goto lab45 ; 
  Result = true ; 
  goto lab10 ; 
  lab45: Result = false ; 
  lab10: ; 
  return(Result) ; 
} 
void zoutasfix ( x ) 
integer x ; 
{schar k  ; 
  if ( abs ( x ) >= 16777216L ) 
  {
    perfect = false ; 
    if ( charsonline > 0 ) 
    (void) fprintf(stdout, "%c\n", ' ' ) ; 
    charsonline = 0 ; 
    (void) fprintf(stdout, "%s%s\n", "Bad VF file: " , "Oversize dimension has been reset to zero." ) 
    ; 
  } 
  if ( x >= 0 ) 
  tfm [ 28 ] = 0 ; 
  else {
      
    tfm [ 28 ] = 255 ; 
    x = x + 16777216L ; 
  } 
  {register integer for_end; k = 3 ; for_end = 1 ; if ( k >= for_end) do 
    {
      tfm [ 28 + k ] = x % 256 ; 
      x = x / 256 ; 
    } 
  while ( k-- > for_end ) ; } 
  outfix ( 28 ) ; 
} 
integer zgetbytes ( k , issigned ) 
integer k ; 
boolean issigned ; 
{register integer Result; integer a  ; 
  if ( vfptr + k > vflimit ) 
  {
    {
      perfect = false ; 
      if ( charsonline > 0 ) 
      (void) fprintf(stdout, "%c\n", ' ' ) ; 
      charsonline = 0 ; 
      (void) fprintf(stdout, "%s%s\n", "Bad VF file: " , "Packet ended prematurely" ) ; 
    } 
    k = vflimit - vfptr ; 
  } 
  a = vf [ vfptr ] ; 
  if ( ( k == 4 ) || issigned ) 
  if ( a >= 128 ) 
  a = a - 256 ; 
  vfptr = vfptr + 1 ; 
  while ( k > 1 ) {
      
    a = a * 256 + vf [ vfptr ] ; 
    vfptr = vfptr + 1 ; 
    k = k - 1 ; 
  } 
  Result = a ; 
  return(Result) ; 
} 
boolean vfinput ( ) 
{/* 9999 10 */ register boolean Result; integer vfptr  ; 
  integer k  ; 
  integer c  ; 
  read ( vffile , tempbyte ) ; 
  if ( tempbyte != 247 ) 
  {
    (void) fprintf(stdout, "%s\n", "The first byte isn't `pre'!" ) ; 
    (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?" ) ; 
    uexit ( 1 ) ; 
  } 
  if ( eof ( vffile ) ) 
  {
    (void) fprintf(stdout, "%s\n", "The input file is only one byte long!" ) ; 
    (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?" ) ; 
    uexit ( 1 ) ; 
  } 
  read ( vffile , tempbyte ) ; 
  if ( tempbyte != 202 ) 
  {
    (void) fprintf(stdout, "%s\n", "Wrong VF version number in second byte!" ) ; 
    (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?" ) ; 
    uexit ( 1 ) ; 
  } 
  if ( eof ( vffile ) ) 
  {
    (void) fprintf(stdout, "%s\n", "The input file is only two bytes long!" ) ; 
    (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?" ) ; 
    uexit ( 1 ) ; 
  } 
  read ( vffile , tempbyte ) ; 
  vfcount = 11 ; 
  vfptr = 0 ; 
  if ( vfptr + tempbyte >= vfsize ) 
  {
    (void) fprintf(stdout, "%s\n", "The file is bigger than I can handle!" ) ; 
    (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?" ) ; 
    uexit ( 1 ) ; 
  } 
  {register integer for_end; k = vfptr ; for_end = vfptr + tempbyte - 1 
  ; if ( k <= for_end) do 
    {
      if ( eof ( vffile ) ) 
      {
	(void) fprintf(stdout, "%s\n", "The file ended prematurely!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?" ) ; 
	uexit ( 1 ) ; 
      } 
      read ( vffile , vf [ k ] ) ; 
    } 
  while ( k++ < for_end ) ; } 
  vfcount = vfcount + tempbyte ; 
  vfptr = vfptr + tempbyte ; 
  if ( verbose ) 
  {
    {register integer for_end; k = 0 ; for_end = vfptr - 1 ; if ( k <= 
    for_end) do 
      (void) putc(xchr [ vf [ k ] ] , stdout);
    while ( k++ < for_end ) ; } 
    (void) fprintf(stdout, "%c\n", ' ' ) ; 
  } 
  count = 0 ; 
  {register integer for_end; k = 0 ; for_end = 7 ; if ( k <= for_end) do 
    {
      if ( eof ( vffile ) ) 
      {
	(void) fprintf(stdout, "%s\n", "The file ended prematurely!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?" ) ; 
	uexit ( 1 ) ; 
      } 
      read ( vffile , tempbyte ) ; 
      if ( tempbyte == tfm [ 24 + k ] ) 
      count = count + 1 ; 
    } 
  while ( k++ < for_end ) ; } 
  realdsize = ( ( ( tfm [ 28 ] * 256 + tfm [ 29 ] ) * 256 + tfm [ 30 ] ) * 256 
  + tfm [ 31 ] ) / ((double) 1048576L ) ; 
  if ( count != 8 ) 
  {
    (void) fprintf(stdout, "%s\n", "Check sum and/or design size mismatch." ) ; 
    (void) fprintf(stdout, "%s\n", "Data from TFM file will be assumed correct." ) ; 
  } 
  {register integer for_end; k = 0 ; for_end = 255 ; if ( k <= for_end) do 
    packetstart [ k ] = vfsize ; 
  while ( k++ < for_end ) ; } 
  fontptr = 0 ; 
  packetfound = false ; 
  fontstart [ 0 ] = vfptr ; 
  do {
      if ( eof ( vffile ) ) 
    {
      (void) fprintf(stdout, "%s\n", "File ended without a postamble!" ) ; 
      tempbyte = 248 ; 
    } 
    else {
	
      read ( vffile , tempbyte ) ; 
      vfcount = vfcount + 1 ; 
      if ( tempbyte != 248 ) 
      if ( tempbyte > 242 ) 
      {
	if ( packetfound || ( tempbyte >= 247 ) ) 
	{
	  (void) fprintf(stdout, "%s%ld%s\n", "Illegal byte " , (long)tempbyte ,           " at beginning of character packet!" ) ; 
	  (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?" ) ; 
	  uexit ( 1 ) ; 
	} 
	fontnumber [ fontptr ] = vfread ( tempbyte - 242 ) ; 
	if ( fontptr == maxfonts ) 
	{
	  (void) fprintf(stdout, "%s\n", "I can't handle that many fonts!" ) ; 
	  (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?" ) ; 
	  uexit ( 1 ) ; 
	} 
	if ( vfptr + 14 >= vfsize ) 
	{
	  (void) fprintf(stdout, "%s\n", "The file is bigger than I can handle!" ) ; 
	  (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?" ) ; 
	  uexit ( 1 ) ; 
	} 
	{register integer for_end; k = vfptr ; for_end = vfptr + 13 ; if ( k 
	<= for_end) do 
	  {
	    if ( eof ( vffile ) ) 
	    {
	      (void) fprintf(stdout, "%s\n", "The file ended prematurely!" ) ; 
	      (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?"               ) ; 
	      uexit ( 1 ) ; 
	    } 
	    read ( vffile , vf [ k ] ) ; 
	  } 
	while ( k++ < for_end ) ; } 
	vfcount = vfcount + 14 ; 
	vfptr = vfptr + 14 ; 
	if ( vf [ vfptr - 10 ] > 0 ) 
	{
	  (void) fprintf(stdout, "%s\n", "Mapped font size is too big!" ) ; 
	  (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?" ) ; 
	  uexit ( 1 ) ; 
	} 
	a = vf [ vfptr - 2 ] ; 
	l = vf [ vfptr - 1 ] ; 
	if ( vfptr + a + l >= vfsize ) 
	{
	  (void) fprintf(stdout, "%s\n", "The file is bigger than I can handle!" ) ; 
	  (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?" ) ; 
	  uexit ( 1 ) ; 
	} 
	{register integer for_end; k = vfptr ; for_end = vfptr + a + l - 1 
	; if ( k <= for_end) do 
	  {
	    if ( eof ( vffile ) ) 
	    {
	      (void) fprintf(stdout, "%s\n", "The file ended prematurely!" ) ; 
	      (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?"               ) ; 
	      uexit ( 1 ) ; 
	    } 
	    read ( vffile , vf [ k ] ) ; 
	  } 
	while ( k++ < for_end ) ; } 
	vfcount = vfcount + a + l ; 
	vfptr = vfptr + a + l ; 
	if ( verbose ) 
	{
	  (void) fprintf(stdout, "%s%ld%s", "MAPFONT " , (long)fontptr , ": " ) ; 
	  {register integer for_end; k = fontstart [ fontptr ] + 14 ; for_end 
	  = vfptr - 1 ; if ( k <= for_end) do 
	    (void) putc(xchr [ vf [ k ] ] , stdout);
	  while ( k++ < for_end ) ; } 
	  k = fontstart [ fontptr ] + 5 ; 
	  (void) Fputs(stdout, " at " ) ; 
	  printreal ( ( ( ( vf [ k ] * 256 + vf [ k + 1 ] ) * 256 + vf [ k + 2 
	  ] ) / ((double) 1048576L ) ) * realdsize , 2 , 2 ) ; 
	  (void) fprintf(stdout, "%s\n", "pt" ) ; 
	} 
	fontchars [ fontptr ] = vfptr ; 
	{register integer for_end; k = 1 ; for_end = PATHMAX ; if ( k <= 
	for_end) do 
	  curname [ k ] = ' ' ; 
	while ( k++ < for_end ) ; } 
	r = 0 ; 
	{register integer for_end; k = fontstart [ fontptr ] + 14 ; for_end = 
	vfptr - 1 ; if ( k <= for_end) do 
	  {
	    r = r + 1 ; 
	    if ( r + 4 > PATHMAX ) 
	    {
	      (void) fprintf(stdout, "%s\n", "Font name too long for me!" ) ; 
	      (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?"               ) ; 
	      uexit ( 1 ) ; 
	    } 
	    curname [ r ] = xchr [ vf [ k ] ] ; 
	  } 
	while ( k++ < for_end ) ; } 
	curname [ r + 1 ] = '.' ; 
	curname [ r + 2 ] = 't' ; 
	curname [ r + 3 ] = 'f' ; 
	curname [ r + 4 ] = 'm' ; 
	if ( ! testreadaccess ( curname , TFMFILEPATH ) ) 
	(void) fprintf(stdout, "%s\n", "---not loaded, TFM file can't be opened!" ) ; 
	else {
	    
	  reset ( tfmfile , curname ) ; 
	  fontbc = 0 ; 
	  fontec = 256 ; 
	  readtfmword () ; 
	  if ( b2 < 128 ) 
	  {
	    fontlh = b2 * 256 + b3 ; 
	    readtfmword () ; 
	    if ( ( b0 < 128 ) && ( b2 < 128 ) ) 
	    {
	      fontbc = b0 * 256 + b1 ; 
	      fontec = b2 * 256 + b3 ; 
	    } 
	  } 
	  if ( fontbc <= fontec ) 
	  if ( fontec > 255 ) 
	  (void) fprintf(stdout, "%s\n", "---not loaded, bad TFM file!" ) ; 
	  else {
	      
	    {register integer for_end; k = 0 ; for_end = 3 + fontlh ; if ( k 
	    <= for_end) do 
	      {
		readtfmword () ; 
		if ( k == 4 ) 
		if ( b0 + b1 + b2 + b3 > 0 ) 
		if ( ( b0 != vf [ fontstart [ fontptr ] ] ) || ( b1 != vf [ 
		fontstart [ fontptr ] + 1 ] ) || ( b2 != vf [ fontstart [ 
		fontptr ] + 2 ] ) || ( b3 != vf [ fontstart [ fontptr ] + 3 ] 
		) ) 
		{
		  if ( verbose ) 
		  (void) fprintf(stdout, "%s\n", "Check sum in VF file being replaced by TFM check sum" ) ; 
		  vf [ fontstart [ fontptr ] ] = b0 ; 
		  vf [ fontstart [ fontptr ] + 1 ] = b1 ; 
		  vf [ fontstart [ fontptr ] + 2 ] = b2 ; 
		  vf [ fontstart [ fontptr ] + 3 ] = b3 ; 
		} 
		if ( k == 5 ) 
		if ( ( b0 != vf [ fontstart [ fontptr ] + 8 ] ) || ( b1 != vf 
		[ fontstart [ fontptr ] + 9 ] ) || ( b2 != vf [ fontstart [ 
		fontptr ] + 10 ] ) || ( b3 != vf [ fontstart [ fontptr ] + 11 
		] ) ) 
		{
		  (void) fprintf(stdout, "%s\n", "Design size in VF file being replaced by TFM design size" ) 
		  ; 
		  vf [ fontstart [ fontptr ] + 8 ] = b0 ; 
		  vf [ fontstart [ fontptr ] + 9 ] = b1 ; 
		  vf [ fontstart [ fontptr ] + 10 ] = b2 ; 
		  vf [ fontstart [ fontptr ] + 11 ] = b3 ; 
		} 
	      } 
	    while ( k++ < for_end ) ; } 
	    {register integer for_end; k = fontbc ; for_end = fontec ; if ( k 
	    <= for_end) do 
	      {
		readtfmword () ; 
		if ( b0 > 0 ) 
		{
		  vf [ vfptr ] = k ; 
		  vfptr = vfptr + 1 ; 
		  if ( vfptr == vfsize ) 
		  {
		    (void) fprintf(stdout, "%s\n", "I'm out of VF memory!" ) ; 
		    (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?" ) ; 
		    uexit ( 1 ) ; 
		  } 
		} 
	      } 
	    while ( k++ < for_end ) ; } 
	  } 
	  if ( eof ( tfmfile ) ) 
	  (void) fprintf(stdout, "%s\n", "---trouble is brewing, TFM file ended too soon!" ) ; 
	} 
	vfptr = vfptr + 1 ; 
	fontptr = fontptr + 1 ; 
	fontstart [ fontptr ] = vfptr ; 
      } 
      else {
	  
	if ( tempbyte == 242 ) 
	{
	  pl = vfread ( 4 ) ; 
	  c = vfread ( 4 ) ; 
	  count = vfread ( 4 ) ; 
	} 
	else {
	    
	  pl = tempbyte ; 
	  c = vfread ( 1 ) ; 
	  count = vfread ( 3 ) ; 
	} 
	if ( ( ( c < bc ) || ( c > ec ) || ( tfm [ 4 * ( charbase + c ) ] == 0 
	) ) ) 
	{
	  (void) fprintf(stdout, "%s%ld%s\n", "Character " , (long)c , " does not exist!" ) ; 
	  (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?" ) ; 
	  uexit ( 1 ) ; 
	} 
	if ( packetstart [ c ] < vfsize ) 
	(void) fprintf(stdout, "%s%ld\n", "Discarding earlier packet for character " , (long)c ) ; 
	if ( count != tfmwidth ( c ) ) 
	(void) fprintf(stdout, "%s%ld%s\n", "Incorrect TFM width for character " , (long)c , " in VF file" ) ; 
	if ( pl < 0 ) 
	{
	  (void) fprintf(stdout, "%s\n", "Negative packet length!" ) ; 
	  (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?" ) ; 
	  uexit ( 1 ) ; 
	} 
	packetstart [ c ] = vfptr ; 
	if ( vfptr + pl >= vfsize ) 
	{
	  (void) fprintf(stdout, "%s\n", "The file is bigger than I can handle!" ) ; 
	  (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?" ) ; 
	  uexit ( 1 ) ; 
	} 
	{register integer for_end; k = vfptr ; for_end = vfptr + pl - 1 
	; if ( k <= for_end) do 
	  {
	    if ( eof ( vffile ) ) 
	    {
	      (void) fprintf(stdout, "%s\n", "The file ended prematurely!" ) ; 
	      (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a VF?"               ) ; 
	      uexit ( 1 ) ; 
	    } 
	    read ( vffile , vf [ k ] ) ; 
	  } 
	while ( k++ < for_end ) ; } 
	vfcount = vfcount + pl ; 
	vfptr = vfptr + pl ; 
	packetend [ c ] = vfptr - 1 ; 
	packetfound = true ; 
      } 
    } 
  } while ( ! ( tempbyte == 248 ) ) ; 
  while ( ( tempbyte == 248 ) && ! eof ( vffile ) ) {
      
    read ( vffile , tempbyte ) ; 
    vfcount = vfcount + 1 ; 
  } 
  if ( ! eof ( vffile ) ) 
  {
    (void) fprintf(stdout, "%s\n", "There's some extra junk at the end of the VF file." ) ; 
    (void) fprintf(stdout, "%s\n", "I'll proceed as if it weren't there." ) ; 
  } 
  if ( vfcount % 4 != 0 ) 
  (void) fprintf(stdout, "%s\n", "VF data not a multiple of 4 bytes" ) ; 
  Result = true ; 
  goto lab10 ; 
  lab9999: Result = false ; 
  lab10: ; 
  return(Result) ; 
} 
boolean organize ( ) 
{/* 9999 10 */ register boolean Result; index tfmptr  ; 
  read ( tfmfile , tfm [ 0 ] ) ; 
  if ( tfm [ 0 ] > 127 ) 
  {
    (void) fprintf(stdout, "%s\n", "The first byte of the input file exceeds 127!" ) ; 
    (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
    uexit ( 1 ) ; 
  } 
  if ( eof ( tfmfile ) ) 
  {
    (void) fprintf(stdout, "%s\n", "The input file is only one byte long!" ) ; 
    (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
    uexit ( 1 ) ; 
  } 
  read ( tfmfile , tfm [ 1 ] ) ; 
  lf = tfm [ 0 ] * 256 + tfm [ 1 ] ; 
  if ( lf == 0 ) 
  {
    (void) fprintf(stdout, "%s\n", "The file claims to have length zero, but that's impossible!" ) 
    ; 
    (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
    uexit ( 1 ) ; 
  } 
  if ( 4 * lf - 1 > tfmsize ) 
  {
    (void) fprintf(stdout, "%s\n", "The file is bigger than I can handle!" ) ; 
    (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
    uexit ( 1 ) ; 
  } 
  {register integer for_end; tfmptr = 2 ; for_end = 4 * lf - 1 ; if ( tfmptr 
  <= for_end) do 
    {
      if ( eof ( tfmfile ) ) 
      {
	(void) fprintf(stdout, "%s\n", "The file has fewer bytes than it claims!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	uexit ( 1 ) ; 
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
	uexit ( 1 ) ; 
      } 
      lh = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	uexit ( 1 ) ; 
      } 
      bc = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	uexit ( 1 ) ; 
      } 
      ec = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	uexit ( 1 ) ; 
      } 
      nw = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	uexit ( 1 ) ; 
      } 
      nh = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	uexit ( 1 ) ; 
      } 
      nd = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	uexit ( 1 ) ; 
      } 
      ni = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	uexit ( 1 ) ; 
      } 
      nl = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	uexit ( 1 ) ; 
      } 
      nk = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	uexit ( 1 ) ; 
      } 
      ne = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    {
      if ( tfm [ tfmptr ] > 127 ) 
      {
	(void) fprintf(stdout, "%s\n", "One of the subfile sizes is negative!" ) ; 
	(void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
	uexit ( 1 ) ; 
      } 
      np = tfm [ tfmptr ] * 256 + tfm [ tfmptr + 1 ] ; 
      tfmptr = tfmptr + 2 ; 
    } 
    if ( lh < 2 ) 
    {
      (void) fprintf(stdout, "%s%ld%c\n", "The header length is only " , (long)lh , '!' ) ; 
      (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
      uexit ( 1 ) ; 
    } 
    if ( nl > 4 * ligsize ) 
    {
      (void) fprintf(stdout, "%s\n", "The lig/kern program is longer than I can handle!" ) ; 
      (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
      uexit ( 1 ) ; 
    } 
    if ( ( bc > ec + 1 ) || ( ec > 255 ) ) 
    {
      (void) fprintf(stdout, "%s%ld%s%ld%s\n", "The character code range " , (long)bc , ".." , (long)ec , "is illegal!" ) 
      ; 
      (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
      uexit ( 1 ) ; 
    } 
    if ( ( nw == 0 ) || ( nh == 0 ) || ( nd == 0 ) || ( ni == 0 ) ) 
    {
      (void) fprintf(stdout, "%s\n", "Incomplete subfiles for character dimensions!" ) ; 
      (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
      uexit ( 1 ) ; 
    } 
    if ( ne > 256 ) 
    {
      (void) fprintf(stdout, "%s%ld%s\n", "There are " , (long)ne , " extensible recipes!" ) ; 
      (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
      uexit ( 1 ) ; 
    } 
    if ( lf != 6 + lh + ( ec - bc + 1 ) + nw + nh + nd + ni + nl + nk + ne + 
    np ) 
    {
      (void) fprintf(stdout, "%s\n", "Subfile sizes don't add up to the stated total!" ) ; 
      (void) fprintf(stdout, "%s\n", "Sorry, but I can't go on; are you sure this is a TFM?" ) ; 
      uexit ( 1 ) ; 
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
  Result = vfinput () ; 
  goto lab10 ; 
  lab9999: Result = false ; 
  lab10: ; 
  return(Result) ; 
} 
void dosimplethings ( ) 
{short i  ; 
  integer f  ; 
  integer k  ; 
  if ( stringbalance ( 0 , fontstart [ 0 ] ) ) 
  {
    left () ; 
    (void) Fputs( vplfile ,  "VTITLE " ) ; 
    {register integer for_end; k = 0 ; for_end = fontstart [ 0 ] - 1 ; if ( k 
    <= for_end) do 
      (void) putc( xchr [ vf [ k ] ] ,  vplfile );
    while ( k++ < for_end ) ; } 
    right () ; 
  } 
  else {
      
    perfect = false ; 
    if ( charsonline > 0 ) 
    (void) fprintf(stdout, "%c\n", ' ' ) ; 
    charsonline = 0 ; 
    (void) fprintf(stdout, "%s%s\n", "Bad VF file: " , "Title is not a balanced ASCII string" ) ; 
  } 
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
	(void) Fputs( vplfile ,  "FAMILY" ) ; 
	checkBCPL ( 72 , 20 ) ; 
	outBCPL ( 72 ) ; 
	right () ; 
	if ( lh >= 18 ) 
	{
	  left () ; 
	  (void) Fputs( vplfile ,  "FACE" ) ; 
	  outface ( 95 ) ; 
	  right () ; 
	  {register integer for_end; i = 18 ; for_end = lh - 1 ; if ( i <= 
	  for_end) do 
	    {
	      left () ; 
	      (void) fprintf( vplfile , "%s%ld",  "HEADER D " , (long)i ) ; 
	      outoctal ( 24 + 4 * i , 4 ) ; 
	      right () ; 
	    } 
	  while ( i++ < for_end ) ; } 
	} 
      } 
      left () ; 
      (void) Fputs( vplfile ,  "CODINGSCHEME" ) ; 
      outBCPL ( 32 ) ; 
      right () ; 
    } 
    left () ; 
    (void) Fputs( vplfile ,  "DESIGNSIZE" ) ; 
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
      (void) Fputs( vplfile ,  " D 10" ) ; 
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
      (void) Fputs( vplfile ,  " D 10" ) ; 
    } 
    else outfix ( 28 ) ; 
    right () ; 
    (void) Fputs( vplfile ,  "(COMMENT DESIGNSIZE IS IN POINTS)" ) ; 
    outln () ; 
    (void) Fputs( vplfile ,  "(COMMENT OTHER SIZES ARE MULTIPLES OF DESIGNSIZE)" ) ; 
    outln () ; 
    left () ; 
    (void) Fputs( vplfile ,  "CHECKSUM" ) ; 
    outoctal ( 24 , 4 ) ; 
    right () ; 
    if ( ( lh > 17 ) && ( tfm [ 92 ] > 127 ) ) 
    {
      left () ; 
      (void) Fputs( vplfile ,  "SEVENBITSAFEFLAG TRUE" ) ; 
      right () ; 
    } 
  } 
  if ( np > 0 ) 
  {
    left () ; 
    (void) Fputs( vplfile ,  "FONTDIMEN" ) ; 
    outln () ; 
    {register integer for_end; i = 1 ; for_end = np ; if ( i <= for_end) do 
      {
	left () ; 
	if ( i == 1 ) 
	(void) Fputs( vplfile ,  "SLANT" ) ; 
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
	    (void) Fputs( vplfile ,  "SPACE" ) ; 
	    break ; 
	  case 3 : 
	    (void) Fputs( vplfile ,  "STRETCH" ) ; 
	    break ; 
	  case 4 : 
	    (void) Fputs( vplfile ,  "SHRINK" ) ; 
	    break ; 
	  case 5 : 
	    (void) Fputs( vplfile ,  "XHEIGHT" ) ; 
	    break ; 
	  case 6 : 
	    (void) Fputs( vplfile ,  "QUAD" ) ; 
	    break ; 
	  case 7 : 
	    (void) Fputs( vplfile ,  "EXTRASPACE" ) ; 
	    break ; 
	  } 
	  else if ( ( i <= 22 ) && ( fonttype == 1 ) ) 
	  switch ( i ) 
	  {case 8 : 
	    (void) Fputs( vplfile ,  "NUM1" ) ; 
	    break ; 
	  case 9 : 
	    (void) Fputs( vplfile ,  "NUM2" ) ; 
	    break ; 
	  case 10 : 
	    (void) Fputs( vplfile ,  "NUM3" ) ; 
	    break ; 
	  case 11 : 
	    (void) Fputs( vplfile ,  "DENOM1" ) ; 
	    break ; 
	  case 12 : 
	    (void) Fputs( vplfile ,  "DENOM2" ) ; 
	    break ; 
	  case 13 : 
	    (void) Fputs( vplfile ,  "SUP1" ) ; 
	    break ; 
	  case 14 : 
	    (void) Fputs( vplfile ,  "SUP2" ) ; 
	    break ; 
	  case 15 : 
	    (void) Fputs( vplfile ,  "SUP3" ) ; 
	    break ; 
	  case 16 : 
	    (void) Fputs( vplfile ,  "SUB1" ) ; 
	    break ; 
	  case 17 : 
	    (void) Fputs( vplfile ,  "SUB2" ) ; 
	    break ; 
	  case 18 : 
	    (void) Fputs( vplfile ,  "SUPDROP" ) ; 
	    break ; 
	  case 19 : 
	    (void) Fputs( vplfile ,  "SUBDROP" ) ; 
	    break ; 
	  case 20 : 
	    (void) Fputs( vplfile ,  "DELIM1" ) ; 
	    break ; 
	  case 21 : 
	    (void) Fputs( vplfile ,  "DELIM2" ) ; 
	    break ; 
	  case 22 : 
	    (void) Fputs( vplfile ,  "AXISHEIGHT" ) ; 
	    break ; 
	  } 
	  else if ( ( i <= 13 ) && ( fonttype == 2 ) ) 
	  if ( i == 8 ) 
	  (void) Fputs( vplfile ,  "DEFAULTRULETHICKNESS" ) ; 
	  else
	  (void) fprintf( vplfile , "%s%ld",  "BIGOPSPACING" , (long)i - 8 ) ; 
	  else
	  (void) fprintf( vplfile , "%s%ld",  "PARAMETER D " , (long)i ) ; 
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
  {register integer for_end; f = 0 ; for_end = fontptr - 1 ; if ( f <= 
  for_end) do 
    {
      left () ; 
      (void) fprintf( vplfile , "%s%ld",  "MAPFONT D " , (long)f ) ; 
      outln () ; 
      a = vf [ fontstart [ f ] + 12 ] ; 
      l = vf [ fontstart [ f ] + 13 ] ; 
      if ( a > 0 ) 
      if ( ! stringbalance ( fontstart [ f ] + 14 , a ) ) 
      {
	perfect = false ; 
	if ( charsonline > 0 ) 
	(void) fprintf(stdout, "%c\n", ' ' ) ; 
	charsonline = 0 ; 
	(void) fprintf(stdout, "%s%s\n", "Bad VF file: " , "Improper font area will be ignored" ) ; 
      } 
      else {
	  
	left () ; 
	(void) Fputs( vplfile ,  "FONTAREA " ) ; 
	{register integer for_end; k = fontstart [ f ] + 14 ; for_end = 
	fontstart [ f ] + a + 13 ; if ( k <= for_end) do 
	  (void) putc( xchr [ vf [ k ] ] ,  vplfile );
	while ( k++ < for_end ) ; } 
	right () ; 
      } 
      if ( ( l == 0 ) || ! stringbalance ( fontstart [ f ] + 14 + a , l ) ) 
      {
	perfect = false ; 
	if ( charsonline > 0 ) 
	(void) fprintf(stdout, "%c\n", ' ' ) ; 
	charsonline = 0 ; 
	(void) fprintf(stdout, "%s%s\n", "Bad VF file: " , "Improper font name will be ignored" ) ; 
      } 
      else {
	  
	left () ; 
	(void) Fputs( vplfile ,  "FONTNAME " ) ; 
	{register integer for_end; k = fontstart [ f ] + 14 + a ; for_end = 
	fontstart [ f ] + a + l + 13 ; if ( k <= for_end) do 
	  (void) putc( xchr [ vf [ k ] ] ,  vplfile );
	while ( k++ < for_end ) ; } 
	right () ; 
      } 
      {register integer for_end; k = 0 ; for_end = 11 ; if ( k <= for_end) do 
	tfm [ k ] = vf [ fontstart [ f ] + k ] ; 
      while ( k++ < for_end ) ; } 
      if ( tfm [ 0 ] + tfm [ 1 ] + tfm [ 2 ] + tfm [ 3 ] > 0 ) 
      {
	left () ; 
	(void) Fputs( vplfile ,  "FONTCHECKSUM" ) ; 
	outoctal ( 0 , 4 ) ; 
	right () ; 
      } 
      left () ; 
      (void) Fputs( vplfile ,  "FONTAT" ) ; 
      outfix ( 4 ) ; 
      right () ; 
      left () ; 
      (void) Fputs( vplfile ,  "FONTDSIZE" ) ; 
      outfix ( 8 ) ; 
      right () ; 
      right () ; 
    } 
  while ( f++ < for_end ) ; } 
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
boolean zdomap ( c ) 
byte c ; 
{/* 9999 10 */ register boolean Result; integer k  ; 
  integer f  ; 
  if ( packetstart [ c ] == vfsize ) 
  {
    perfect = false ; 
    if ( charsonline > 0 ) 
    (void) fprintf(stdout, "%c\n", ' ' ) ; 
    charsonline = 0 ; 
    (void) fprintf(stdout, "%s%s%ld\n", "Bad VF file: " , "Missing packet for character " , (long)c ) ; 
  } 
  else {
      
    left () ; 
    (void) Fputs( vplfile ,  "MAP" ) ; 
    outln () ; 
    top = 0 ; 
    wstack [ 0 ] = 0 ; 
    xstack [ 0 ] = 0 ; 
    ystack [ 0 ] = 0 ; 
    zstack [ 0 ] = 0 ; 
    vfptr = packetstart [ c ] ; 
    vflimit = packetend [ c ] + 1 ; 
    f = 0 ; 
    while ( vfptr < vflimit ) {
	
      o = vf [ vfptr ] ; 
      vfptr = vfptr + 1 ; 
      if ( ( ( o >= 0 ) && ( o <= 127 ) ) || ( ( o >= 128 ) && ( o <= 131 ) ) 
      || ( ( o >= 133 ) && ( o <= 136 ) ) ) 
      {
	if ( o >= 128 ) 
	if ( o >= 133 ) 
	c = getbytes ( o - 132 , false ) ; 
	else c = getbytes ( o - 127 , false ) ; 
	else c = o ; 
	if ( f == fontptr ) 
	{
	  perfect = false ; 
	  if ( charsonline > 0 ) 
	  (void) fprintf(stdout, "%c\n", ' ' ) ; 
	  charsonline = 0 ; 
	  (void) fprintf(stdout, "%s%s%ld%s\n", "Bad VF file: " , "Character " , (long)c ,           " in undeclared font will be ignored" ) ; 
	} 
	else {
	    
	  vf [ fontstart [ f + 1 ] - 1 ] = c ; 
	  k = fontchars [ f ] ; 
	  while ( vf [ k ] != c ) k = k + 1 ; 
	  if ( k == fontstart [ f + 1 ] - 1 ) 
	  {
	    perfect = false ; 
	    if ( charsonline > 0 ) 
	    (void) fprintf(stdout, "%c\n", ' ' ) ; 
	    charsonline = 0 ; 
	    (void) fprintf(stdout, "%s%s%ld%s%ld%s\n", "Bad VF file: " , "Character " , (long)c , " in font " , (long)f ,             " will be ignored" ) ; 
	  } 
	  else {
	      
	    if ( o >= 133 ) 
	    (void) Fputs( vplfile ,  "(PUSH)" ) ; 
	    left () ; 
	    (void) Fputs( vplfile ,  "SETCHAR" ) ; 
	    outchar ( c ) ; 
	    if ( o >= 133 ) 
	    (void) Fputs( vplfile ,  ")(POP" ) ; 
	    right () ; 
	  } 
	} 
      } 
      else switch ( o ) 
      {case 138 : 
	; 
	break ; 
      case 141 : 
	{
	  if ( top == maxstack ) 
	  {
	    (void) fprintf(stdout, "%s\n", "Stack overflow!" ) ; 
	    uexit ( 1 ) ; 
	  } 
	  top = top + 1 ; 
	  wstack [ top ] = wstack [ top - 1 ] ; 
	  xstack [ top ] = xstack [ top - 1 ] ; 
	  ystack [ top ] = ystack [ top - 1 ] ; 
	  zstack [ top ] = zstack [ top - 1 ] ; 
	  (void) Fputs( vplfile ,  "(PUSH)" ) ; 
	  outln () ; 
	} 
	break ; 
      case 142 : 
	if ( top == 0 ) 
	{
	  perfect = false ; 
	  if ( charsonline > 0 ) 
	  (void) fprintf(stdout, "%c\n", ' ' ) ; 
	  charsonline = 0 ; 
	  (void) fprintf(stdout, "%s%s\n", "Bad VF file: " , "More pops than pushes!" ) ; 
	} 
	else {
	    
	  top = top - 1 ; 
	  (void) Fputs( vplfile ,  "(POP)" ) ; 
	  outln () ; 
	} 
	break ; 
      case 132 : 
      case 137 : 
	{
	  if ( o == 137 ) 
	  (void) Fputs( vplfile ,  "(PUSH)" ) ; 
	  left () ; 
	  (void) Fputs( vplfile ,  "SETRULE" ) ; 
	  outasfix ( getbytes ( 4 , true ) ) ; 
	  outasfix ( getbytes ( 4 , true ) ) ; 
	  if ( o == 137 ) 
	  (void) Fputs( vplfile ,  ")(POP" ) ; 
	  right () ; 
	} 
	break ; 
      case 143 : 
      case 144 : 
      case 145 : 
      case 146 : 
	{
	  (void) Fputs( vplfile ,  "(MOVERIGHT" ) ; 
	  outasfix ( getbytes ( o - 142 , true ) ) ; 
	  (void) putc( ')' ,  vplfile );
	  outln () ; 
	} 
	break ; 
      case 147 : 
      case 148 : 
      case 149 : 
      case 150 : 
      case 151 : 
	{
	  if ( o != 147 ) 
	  wstack [ top ] = getbytes ( o - 147 , true ) ; 
	  (void) Fputs( vplfile ,  "(MOVERIGHT" ) ; 
	  outasfix ( wstack [ top ] ) ; 
	  (void) putc( ')' ,  vplfile );
	  outln () ; 
	} 
	break ; 
      case 152 : 
      case 153 : 
      case 154 : 
      case 155 : 
      case 156 : 
	{
	  if ( o != 152 ) 
	  xstack [ top ] = getbytes ( o - 152 , true ) ; 
	  (void) Fputs( vplfile ,  "(MOVERIGHT" ) ; 
	  outasfix ( xstack [ top ] ) ; 
	  (void) putc( ')' ,  vplfile );
	  outln () ; 
	} 
	break ; 
      case 157 : 
      case 158 : 
      case 159 : 
      case 160 : 
	{
	  (void) Fputs( vplfile ,  "(MOVEDOWN" ) ; 
	  outasfix ( getbytes ( o - 156 , true ) ) ; 
	  (void) putc( ')' ,  vplfile );
	  outln () ; 
	} 
	break ; 
      case 161 : 
      case 162 : 
      case 163 : 
      case 164 : 
      case 165 : 
	{
	  if ( o != 161 ) 
	  ystack [ top ] = getbytes ( o - 161 , true ) ; 
	  (void) Fputs( vplfile ,  "(MOVEDOWN" ) ; 
	  outasfix ( ystack [ top ] ) ; 
	  (void) putc( ')' ,  vplfile );
	  outln () ; 
	} 
	break ; 
      case 166 : 
      case 167 : 
      case 168 : 
      case 169 : 
      case 170 : 
	{
	  if ( o != 166 ) 
	  zstack [ top ] = getbytes ( o - 166 , true ) ; 
	  (void) Fputs( vplfile ,  "(MOVEDOWN" ) ; 
	  outasfix ( zstack [ top ] ) ; 
	  (void) putc( ')' ,  vplfile );
	  outln () ; 
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
      case 235 : 
      case 236 : 
      case 237 : 
      case 238 : 
	{
	  f = 0 ; 
	  if ( o >= 235 ) 
	  fontnumber [ fontptr ] = getbytes ( o - 234 , false ) ; 
	  else fontnumber [ fontptr ] = o - 171 ; 
	  while ( fontnumber [ f ] != fontnumber [ fontptr ] ) f = f + 1 ; 
	  if ( f == fontptr ) 
	  {
	    perfect = false ; 
	    if ( charsonline > 0 ) 
	    (void) fprintf(stdout, "%c\n", ' ' ) ; 
	    charsonline = 0 ; 
	    (void) fprintf(stdout, "%s%s\n", "Bad VF file: " , "Undeclared font selected" ) ; 
	  } 
	  else {
	      
	    (void) fprintf( vplfile , "%s%ld%c",  "(SELECTFONT D " , (long)f , ')' ) ; 
	    outln () ; 
	  } 
	} 
	break ; 
      case 239 : 
      case 240 : 
      case 241 : 
      case 242 : 
	{
	  k = getbytes ( o - 238 , false ) ; 
	  if ( k < 0 ) 
	  {
	    perfect = false ; 
	    if ( charsonline > 0 ) 
	    (void) fprintf(stdout, "%c\n", ' ' ) ; 
	    charsonline = 0 ; 
	    (void) fprintf(stdout, "%s%s\n", "Bad VF file: " , "String of negative length!" ) ; 
	  } 
	  else {
	      
	    left () ; 
	    if ( k + vfptr > vflimit ) 
	    {
	      {
		perfect = false ; 
		if ( charsonline > 0 ) 
		(void) fprintf(stdout, "%c\n", ' ' ) ; 
		charsonline = 0 ; 
		(void) fprintf(stdout, "%s%s\n", "Bad VF file: " ,                 "Special command truncated to packet length" ) ; 
	      } 
	      k = vflimit - vfptr ; 
	    } 
	    if ( ( k > 64 ) || ! stringbalance ( vfptr , k ) ) 
	    {
	      (void) Fputs( vplfile ,  "SPECIALHEX " ) ; 
	      while ( k > 0 ) {
		  
		if ( k % 32 == 0 ) 
		outln () ; 
		else if ( k % 4 == 0 ) 
		(void) putc( ' ' ,  vplfile );
		{
		  a = vf [ vfptr ] / 16 ; 
		  if ( a < 10 ) 
		  (void) fprintf( vplfile , "%ld",  (long)a ) ; 
		  else
		  (void) putc( xchr [ a + 55 ] ,  vplfile );
		} 
		{
		  a = vf [ vfptr ] % 16 ; 
		  if ( a < 10 ) 
		  (void) fprintf( vplfile , "%ld",  (long)a ) ; 
		  else
		  (void) putc( xchr [ a + 55 ] ,  vplfile );
		} 
		vfptr = vfptr + 1 ; 
		k = k - 1 ; 
	      } 
	    } 
	    else {
		
	      (void) Fputs( vplfile ,  "SPECIAL " ) ; 
	      while ( k > 0 ) {
		  
		(void) putc( xchr [ vf [ vfptr ] ] ,  vplfile );
		vfptr = vfptr + 1 ; 
		k = k - 1 ; 
	      } 
	    } 
	    right () ; 
	  } 
	} 
	break ; 
      case 139 : 
      case 140 : 
      case 243 : 
      case 244 : 
      case 245 : 
      case 246 : 
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
	  perfect = false ; 
	  if ( charsonline > 0 ) 
	  (void) fprintf(stdout, "%c\n", ' ' ) ; 
	  charsonline = 0 ; 
	  (void) fprintf(stdout, "%s%s%ld%s\n", "Bad VF file: " , "Illegal DVI code " , (long)o ,           " will be ignored" ) ; 
	} 
	break ; 
      } 
    } 
    if ( top > 0 ) 
    {
      {
	perfect = false ; 
	if ( charsonline > 0 ) 
	(void) fprintf(stdout, "%c\n", ' ' ) ; 
	charsonline = 0 ; 
	(void) fprintf(stdout, "%s%s\n", "Bad VF file: " , "More pushes than pops!" ) ; 
      } 
      do {
	  (void) Fputs( vplfile ,  "(POP)" ) ; 
	top = top - 1 ; 
      } while ( ! ( top == 0 ) ) ; 
    } 
    right () ; 
  } 
  Result = true ; 
  goto lab10 ; 
  lab9999: Result = false ; 
  lab10: ; 
  return(Result) ; 
} 
boolean docharacters ( ) 
{/* 9999 10 */ register boolean Result; byte c  ; 
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
      (void) Fputs( vplfile ,  "CHARACTER" ) ; 
      outchar ( c ) ; 
      outln () ; 
      {
	left () ; 
	(void) Fputs( vplfile ,  "CHARWD" ) ; 
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
	(void) Fputs( vplfile ,  "CHARHT" ) ; 
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
	(void) Fputs( vplfile ,  "CHARDP" ) ; 
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
	(void) Fputs( vplfile ,  "CHARIC" ) ; 
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
	  (void) Fputs( vplfile ,  "COMMENT" ) ; 
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
		(void) Fputs( vplfile ,  "KRN" ) ; 
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
		  (void) Fputs( vplfile ,  " R 0.0" ) ; 
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
		(void) putc( '/' ,  vplfile );
		(void) Fputs( vplfile ,  "LIG" ) ; 
		if ( odd ( r ) ) 
		(void) putc( '/' ,  vplfile );
		while ( r > 3 ) {
		    
		  (void) putc( '>' ,  vplfile );
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
		(void) Fputs( vplfile ,  "(STOP)" ) ; 
		else {
		    
		  count = 0 ; 
		  {register integer for_end; ai = i + 1 ; for_end = i + tfm [ 
		  k ] ; if ( ai <= for_end) do 
		    if ( activity [ ai ] == 2 ) 
		    count = count + 1 ; 
		  while ( ai++ < for_end ) ; } 
		  (void) fprintf( vplfile , "%s%ld%c",  "(SKIP D " , (long)count , ')' ) ; 
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
	      (void) Fputs( vplfile ,  "NEXTLARGER" ) ; 
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
	  (void) Fputs( vplfile ,  "VARCHAR" ) ; 
	  outln () ; 
	  {register integer for_end; k = 0 ; for_end = 3 ; if ( k <= for_end) 
	  do 
	    if ( ( k == 3 ) || ( tfm [ 4 * ( extenbase + tfm [ 4 * ( charbase 
	    + c ) + 3 ] ) + k ] > 0 ) ) 
	    {
	      left () ; 
	      switch ( k ) 
	      {case 0 : 
		(void) Fputs( vplfile ,  "TOP" ) ; 
		break ; 
	      case 1 : 
		(void) Fputs( vplfile ,  "MID" ) ; 
		break ; 
	      case 2 : 
		(void) Fputs( vplfile ,  "BOT" ) ; 
		break ; 
	      case 3 : 
		(void) Fputs( vplfile ,  "REP" ) ; 
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
      if ( ! domap ( c ) ) 
      goto lab9999 ; 
      right () ; 
    } 
  while ( c++ < for_end ) ; } 
  Result = true ; 
  goto lab10 ; 
  lab9999: Result = false ; 
  lab10: ; 
  return(Result) ; 
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
      (void) Fputs( vplfile ,  "BOUNDARYCHAR" ) ; 
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
    (void) Fputs( vplfile ,  "LIGTABLE" ) ; 
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
	    (void) Fputs( vplfile ,              "COMMENT THIS PART OF THE PROGRAM IS NEVER USED!" ) ; 
	    outln () ; 
	  } 
	} 
	else if ( level == 2 ) 
	right () ; 
	while ( i == labeltable [ sortptr ] .rr ) {
	    
	  left () ; 
	  (void) Fputs( vplfile ,  "LABEL" ) ; 
	  if ( labeltable [ sortptr ] .cc == 256 ) 
	  (void) Fputs( vplfile ,  " BOUNDARYCHAR" ) ; 
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
	    (void) Fputs( vplfile ,  "KRN" ) ; 
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
	      (void) Fputs( vplfile ,  " R 0.0" ) ; 
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
	    (void) putc( '/' ,  vplfile );
	    (void) Fputs( vplfile ,  "LIG" ) ; 
	    if ( odd ( r ) ) 
	    (void) putc( '/' ,  vplfile );
	    while ( r > 3 ) {
		
	      (void) putc( '>' ,  vplfile );
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
	    (void) Fputs( vplfile ,  "(STOP)" ) ; 
	    else {
		
	      count = 0 ; 
	      {register integer for_end; ai = i + 1 ; for_end = i + tfm [ k ] 
	      ; if ( ai <= for_end) do 
		if ( activity [ ai ] == 2 ) 
		count = count + 1 ; 
	      while ( ai++ < for_end ) ; } 
	      (void) fprintf( vplfile , "%s%ld%c",  "(SKIP D " , (long)count , ')' ) ; 
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
      uexit ( 1 ) ; 
    } 
    {register integer for_end; hh = 1 ; for_end = hashptr ; if ( hh <= 
    for_end) do 
      {
	r = hashlist [ hh ] ; 
	if ( class [ r ] > 0 ) 
	r = ligf ( r , ( hash [ r ] - 1 ) / 256 , ( hash [ r ] - 1 ) % 256 ) ; 
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
      (void) Fputs( vplfile ,  "(INFINITE LIGATURE LOOP MUST BE BROKEN!)" ) ; 
      uexit ( 1 ) ; 
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
  if ( ! docharacters () ) 
  goto lab9999 ; 
  if ( verbose ) 
  (void) fprintf(stdout, "%c\n", '.' ) ; 
  if ( level != 0 ) 
  (void) fprintf(stdout, "%s\n", "This program isn't working!" ) ; 
  if ( ! perfect ) 
  {
    (void) Fputs( vplfile ,  "(COMMENT THE TFM AND/OR VF FILE WAS BAD, " ) ; 
    (void) Fputs( vplfile ,  "SO THE DATA HAS BEEN CHANGED!)" ) ; 
  } 
  lab9999: ; 
} 
