{3:}program GFtoDVI;const{5:}maxlabels=2000;poolsize=10000;
maxstrings=1100;terminallinelength=150;fontmemsize=2000;dvibufsize=800;
widestrow=8192;liglookahead=20;{:5}{231:}noptions=2;argoptions=1;{:231}
type{9:}scaled=integer;{:9}{10:}ASCIIcode=0..255;{:10}{11:}
textfile=packed file of ASCIIcode;{:11}{45:}eightbits=0..255;
bytefile=packed file of eightbits;{:45}{52:}fontindex=0..fontmemsize;
quarterword=0..255;fourquarters=packed record B0:quarterword;
B1:quarterword;B2:quarterword;B3:quarterword;end;
#include "gftodmem.h";
internalfontnumber=1..5;{:52}{70:}poolpointer=0..poolsize;
strnumber=0..maxstrings;{:70}{79:}keywordcode=0..19;{:79}{104:}
dviindex=0..dvibufsize;{:104}{139:}nodepointer=0..maxlabels;{:139}
var{12:}xord:array[ASCIIcode]of ASCIIcode;
xchr:array[ASCIIcode]of ASCIIcode;{:12}{16:}
buffer:array[0..terminallinelength]of 0..255;{:16}{18:}
bufptr:0..terminallinelength;linelength:0..terminallinelength;{:18}{37:}
lf,lh,bc,ec,nw,nh,nd,ni,nl,nk,ne,np:0..32767;{:37}{46:}gffile:bytefile;
dvifile:bytefile;tfmfile:bytefile;{:46}{48:}curloc:integer;
nameoffile:packed array[1..PATHMAX]of char;{:48}{49:}
b0,b1,b2,b3:eightbits;{:49}{53:}fontinfo:array[fontindex]of memoryword;
fmemptr:fontindex;fontcheck:array[internalfontnumber]of fourquarters;
fontsize:array[internalfontnumber]of scaled;
fontdsize:array[internalfontnumber]of scaled;
fontbc:array[internalfontnumber]of eightbits;
fontec:array[internalfontnumber]of eightbits;
charbase:array[internalfontnumber]of integer;
widthbase:array[internalfontnumber]of integer;
heightbase:array[internalfontnumber]of integer;
depthbase:array[internalfontnumber]of integer;
italicbase:array[internalfontnumber]of integer;
ligkernbase:array[internalfontnumber]of integer;
kernbase:array[internalfontnumber]of integer;
extenbase:array[internalfontnumber]of integer;
parambase:array[internalfontnumber]of integer;
bcharlabel:array[internalfontnumber]of fontindex;
fontbchar:array[internalfontnumber]of 0..256;{:53}{71:}
strpool:packed array[poolpointer]of ASCIIcode;
strstart:array[strnumber]of poolpointer;poolptr:poolpointer;
strptr:strnumber;initstrptr:strnumber;{:71}{76:}l:integer;{:76}{80:}
curgf:eightbits;curstring:strnumber;labeltype:eightbits;{:80}{86:}
curname:strnumber;curarea:strnumber;curext:strnumber;{:86}{87:}
areadelimiter:poolpointer;extdelimiter:poolpointer;{:87}{93:}
jobname:strnumber;{:93}{96:}interaction:boolean;fontsnotloaded:boolean;
fontname:array[internalfontnumber]of strnumber;
fontarea:array[internalfontnumber]of strnumber;
fontat:array[internalfontnumber]of scaled;{:96}{102:}totalpages:integer;
maxv:scaled;maxh:scaled;lastbop:integer;{:102}{105:}
dvibuf:array[dviindex]of eightbits;halfbuf:dviindex;dvilimit:dviindex;
dviptr:dviindex;dvioffset:integer;{:105}{117:}boxwidth:scaled;
boxheight:scaled;boxdepth:scaled;
ligstack:array[1..liglookahead]of quarterword;dummyinfo:fourquarters;
suppresslig:boolean;{:117}{127:}c:array[1..120]of 1..4095;
d:array[1..120]of 2..4096;twotothe:array[0..13]of 1..8192;{:127}{134:}
ruleslant:real;slantn:integer;slantunit:real;slantreported:real;{:134}
{140:}xl,xr,yt,yb:array[1..maxlabels]of scaled;
xx,yy:array[0..maxlabels]of scaled;
prev,next:array[0..maxlabels]of nodepointer;
info:array[1..maxlabels]of strnumber;maxnode:nodepointer;
maxheight:scaled;maxdepth:scaled;{:140}{149:}firstdot:nodepointer;
twin:boolean;{:149}{155:}rulethickness:scaled;offsetx,offsety:scaled;
xoffset,yoffset:scaled;preminx,premaxx,preminy,premaxy:scaled;{:155}
{158:}ruleptr:nodepointer;{:158}{160:}labeltail:nodepointer;
titlehead,titletail:nodepointer;{:160}{166:}charcode,ext:integer;
minx,maxx,miny,maxy:integer;x,y:integer;z:integer;{:166}{168:}
xratio,yratio,slantratio:real;unscxratio,unscyratio,unscslantratio:real;
fudgefactor:real;deltax,deltay:scaled;dvix,dviy:scaled;overcol:scaled;
pageheight,pagewidth:scaled;{:168}{174:}grayrulethickness:scaled;
tempx,tempy:scaled;{:174}{182:}overflowline:integer;{:182}{183:}
delta:scaled;halfxheight:scaled;thricexheight:scaled;
dotwidth,dotheight:scaled;{:183}{207:}b:array[0..4095]of 0..120;
rho:array[0..4095]of 1..4096;{:207}{211:}
a:array[0..widestrow]of 0..4095;{:211}{212:}blankrows:integer;{:212}
{220:}k,m,p,q,r,s,t,dx,dy:integer;timestamp:strnumber;uselogo:boolean;
{:220}{225:}verbose:integer;{:225}{228:}overflowlabeloffset:integer;
offsetinpoints:real;{:228}procedure initialize;var i,j,m,n:integer;
{223:}longoptions:array[0..noptions]of getoptstruct;
getoptreturnval:integer;optionindex:integer;currentoption:0..noptions;
{:223}begin if argc>noptions+argoptions+2 then begin writeln(stdout,
'Usage: gftodvi [-verbose] [-overflow-label-offset=<real>] <gf file>.');
uexit(1);end;{226:}verbose:=false;{:226}{229:}
overflowlabeloffset:=10000000;{:229};{222:}begin{224:}currentoption:=0;
longoptions[0].name:='verbose';longoptions[0].hasarg:=0;
longoptions[0].flag:=addressofint(verbose);longoptions[0].val:=1;
currentoption:=currentoption+1;{:224}{227:}
longoptions[currentoption].name:='overflow-label-offset';
longoptions[currentoption].hasarg:=1;longoptions[currentoption].flag:=0;
longoptions[currentoption].val:=0;currentoption:=currentoption+1;{:227}
{230:}longoptions[currentoption].name:=0;
longoptions[currentoption].hasarg:=0;longoptions[currentoption].flag:=0;
longoptions[currentoption].val:=0;{:230};
repeat getoptreturnval:=getoptlongonly(argc,gargv,'',longoptions,
addressofint(optionindex));
if getoptreturnval<>-1 then begin if getoptreturnval=63 then uexit(1);
if(strcmp(longoptions[optionindex].name,'overflow-label-offset')=0)then
begin offsetinpoints:=atof(optarg);
overflowlabeloffset:=round(offsetinpoints*65536);end else;end;
until getoptreturnval=-1;end{:222};
if verbose then writeln(stdout,'This is GFtoDVI, C Version 3.0');{13:}
xchr[32]:=' ';xchr[33]:='!';xchr[34]:='"';xchr[35]:='#';xchr[36]:='$';
xchr[37]:='%';xchr[38]:='&';xchr[39]:='''';xchr[40]:='(';xchr[41]:=')';
xchr[42]:='*';xchr[43]:='+';xchr[44]:=',';xchr[45]:='-';xchr[46]:='.';
xchr[47]:='/';xchr[48]:='0';xchr[49]:='1';xchr[50]:='2';xchr[51]:='3';
xchr[52]:='4';xchr[53]:='5';xchr[54]:='6';xchr[55]:='7';xchr[56]:='8';
xchr[57]:='9';xchr[58]:=':';xchr[59]:=';';xchr[60]:='<';xchr[61]:='=';
xchr[62]:='>';xchr[63]:='?';xchr[64]:='@';xchr[65]:='A';xchr[66]:='B';
xchr[67]:='C';xchr[68]:='D';xchr[69]:='E';xchr[70]:='F';xchr[71]:='G';
xchr[72]:='H';xchr[73]:='I';xchr[74]:='J';xchr[75]:='K';xchr[76]:='L';
xchr[77]:='M';xchr[78]:='N';xchr[79]:='O';xchr[80]:='P';xchr[81]:='Q';
xchr[82]:='R';xchr[83]:='S';xchr[84]:='T';xchr[85]:='U';xchr[86]:='V';
xchr[87]:='W';xchr[88]:='X';xchr[89]:='Y';xchr[90]:='Z';xchr[91]:='[';
xchr[92]:='\';xchr[93]:=']';xchr[94]:='^';xchr[95]:='_';xchr[96]:='`';
xchr[97]:='a';xchr[98]:='b';xchr[99]:='c';xchr[100]:='d';xchr[101]:='e';
xchr[102]:='f';xchr[103]:='g';xchr[104]:='h';xchr[105]:='i';
xchr[106]:='j';xchr[107]:='k';xchr[108]:='l';xchr[109]:='m';
xchr[110]:='n';xchr[111]:='o';xchr[112]:='p';xchr[113]:='q';
xchr[114]:='r';xchr[115]:='s';xchr[116]:='t';xchr[117]:='u';
xchr[118]:='v';xchr[119]:='w';xchr[120]:='x';xchr[121]:='y';
xchr[122]:='z';xchr[123]:='{';xchr[124]:='|';xchr[125]:='}';
xchr[126]:='~';{:13}{14:}for i:=1 to 31 do xchr[i]:=chr(i);
for i:=127 to 255 do xchr[i]:=chr(i);{:14}{15:}
for i:=0 to 255 do xord[chr(i)]:=32;for i:=1 to 255 do xord[xchr[i]]:=i;
xord['?']:=63;{:15}{54:}fmemptr:=0;{:54}{97:}interaction:=false;
fontsnotloaded:=true;fontname[1]:=29;fontname[2]:=30;fontname[3]:=31;
fontname[4]:=0;fontname[5]:=32;for k:=1 to 5 do begin fontarea[k]:=0;
fontat[k]:=0;end;{:97}{103:}totalpages:=0;maxv:=0;maxh:=0;lastbop:=-1;
{:103}{106:}halfbuf:=dvibufsize div 2;dvilimit:=dvibufsize;dviptr:=0;
dvioffset:=0;{:106}{118:}dummyinfo.B0:=0;dummyinfo.B1:=0;
dummyinfo.B2:=0;dummyinfo.B3:=0;{:118}{126:}c[1]:=1;d[1]:=2;
twotothe[0]:=1;m:=1;for k:=1 to 13 do twotothe[k]:=2*twotothe[k-1];
for k:=2 to 6 do{128:}begin n:=twotothe[k-1];
for j:=0 to n-1 do begin m:=m+1;c[m]:=m;d[m]:=n+n;end;end{:128};
for k:=7 to 12 do{129:}begin n:=twotothe[k-1];
for j:=k downto 1 do begin m:=m+1;d[m]:=n+n;
if j=k then c[m]:=n else c[m]:=c[m-1]+twotothe[j-1];end;end{:129};{:126}
{142:}yy[0]:=-1073741824;yy[maxlabels]:=1073741824;{:142}end;{:3}{8:}
procedure jumpout;begin uexit(0);end;{:8}{17:}procedure inputln;
begin flush(stdout);if eoln(stdin)then readln(stdin);linelength:=0;
while(linelength<terminallinelength)and not eoln(stdin)do begin buffer[
linelength]:=xord[getc(stdin)];linelength:=linelength+1;end;end;{:17}
{47:}procedure opengffile;
begin if testreadaccess(nameoffile,GFFILEPATH)then begin reset(gffile,
nameoffile);end else begin printpascalstring(nameoffile);
begin writeln(stdout,': GF file not found.');uexit(1);end;end;curloc:=0;
end;procedure opentfmfile;
begin if testreadaccess(nameoffile,TFMFILEPATH)then begin reset(tfmfile,
nameoffile);end else begin printpascalstring(nameoffile);
begin writeln(stdout,': TFM file not found.');uexit(1);end;end;end;
procedure opendvifile;begin rewrite(dvifile,nameoffile);end;{:47}{50:}
procedure readtfmword;begin read(tfmfile,b0);read(tfmfile,b1);
read(tfmfile,b2);read(tfmfile,b3);end;{:50}{51:}
function getbyte:integer;var b:eightbits;
begin if eof(gffile)then getbyte:=0 else begin read(gffile,b);
curloc:=curloc+1;getbyte:=b;end;end;function gettwobytes:integer;
var a,b:eightbits;begin read(gffile,a);read(gffile,b);curloc:=curloc+2;
gettwobytes:=a*toint(256)+b;end;function getthreebytes:integer;
var a,b,c:eightbits;begin read(gffile,a);read(gffile,b);read(gffile,c);
curloc:=curloc+3;getthreebytes:=(a*toint(256)+b)*256+c;end;
function signedquad:integer;var a,b,c,d:eightbits;begin read(gffile,a);
read(gffile,b);read(gffile,c);read(gffile,d);curloc:=curloc+4;
if a<128 then signedquad:=((a*toint(256)+b)*256+c)*256+d else signedquad
:=(((a-256)*toint(256)+b)*256+c)*256+d;end;{:51}{58:}
procedure readfontinfo(f:integer;s:scaled);label 30,11;var k:fontindex;
lf,lh,bc,ec,nw,nh,nd,ni,nl,nk,ne,np:0..65535;bchlabel:integer;
bchar:0..256;qw:fourquarters;sw:scaled;z:scaled;alpha:integer;
beta:1..16;begin{59:}{60:}begin readtfmword;lf:=b0*toint(256)+b1;
lh:=b2*toint(256)+b3;readtfmword;bc:=b0*toint(256)+b1;
ec:=b2*toint(256)+b3;if(bc>ec+1)or(ec>255)then goto 11;
if bc>255 then begin bc:=1;ec:=0;end;readtfmword;nw:=b0*toint(256)+b1;
nh:=b2*toint(256)+b3;readtfmword;nd:=b0*toint(256)+b1;
ni:=b2*toint(256)+b3;readtfmword;nl:=b0*toint(256)+b1;
nk:=b2*toint(256)+b3;readtfmword;ne:=b0*toint(256)+b1;
np:=b2*toint(256)+b3;
if lf<>6+lh+(ec-bc+1)+nw+nh+nd+ni+nl+nk+ne+np then goto 11;end{:60};
{61:}lf:=lf-6-lh;if np<8 then lf:=lf+8-np;
if fmemptr+lf>fontmemsize then begin writeln(stdout,
'No room for TFM file!');uexit(1);end;charbase[f]:=fmemptr-bc;
widthbase[f]:=charbase[f]+ec+1;heightbase[f]:=widthbase[f]+nw;
depthbase[f]:=heightbase[f]+nh;italicbase[f]:=depthbase[f]+nd;
ligkernbase[f]:=italicbase[f]+ni;kernbase[f]:=ligkernbase[f]+nl;
extenbase[f]:=kernbase[f]+nk;parambase[f]:=extenbase[f]+ne{:61};{62:}
begin if lh<2 then goto 11;begin readtfmword;qw.B0:=b0+0;qw.B1:=b1+0;
qw.B2:=b2+0;qw.B3:=b3+0;fontcheck[f]:=qw;end;readtfmword;
if b0>127 then goto 11;
z:=((b0*toint(256)+b1)*toint(256)+b2)*16+(b3 div 16);
if z<65536 then goto 11;while lh>2 do begin readtfmword;lh:=lh-1;end;
fontdsize[f]:=z;if s>0 then z:=s;fontsize[f]:=z;end{:62};{63:}
for k:=fmemptr to widthbase[f]-1 do begin begin readtfmword;qw.B0:=b0+0;
qw.B1:=b1+0;qw.B2:=b2+0;qw.B3:=b3+0;fontinfo[k].qqqq:=qw;end;
if(b0>=nw)or(b1 div 16>=nh)or(b1 mod 16>=nd)or(b2 div 4>=ni)then goto 11
;case b2 mod 4 of 1:if b3>=nl then goto 11;3:if b3>=ne then goto 11;
0,2:;end;end{:63};{64:}begin{65:}begin alpha:=16*z;beta:=16;
while z>=8388608 do begin z:=z div 2;beta:=beta div 2;end;end{:65};
for k:=widthbase[f]to ligkernbase[f]-1 do begin readtfmword;
sw:=(((((b3*z)div 256)+(b2*z))div 256)+(b1*z))div beta;
if b0=0 then fontinfo[k].sc:=sw else if b0=255 then fontinfo[k].sc:=sw-
alpha else goto 11;end;if fontinfo[widthbase[f]].sc<>0 then goto 11;
if fontinfo[heightbase[f]].sc<>0 then goto 11;
if fontinfo[depthbase[f]].sc<>0 then goto 11;
if fontinfo[italicbase[f]].sc<>0 then goto 11;end{:64};{66:}
begin bchlabel:=32767;bchar:=256;
if nl>0 then begin for k:=ligkernbase[f]to kernbase[f]-1 do begin begin
readtfmword;qw.B0:=b0+0;qw.B1:=b1+0;qw.B2:=b2+0;qw.B3:=b3+0;
fontinfo[k].qqqq:=qw;end;
if b0>128 then begin if 256*b2+b3>=nl then goto 11;
if b0=255 then if k=ligkernbase[f]then bchar:=b1;
end else begin if b1<>bchar then begin if(b1<bc)or(b1>ec)then goto 11
end;
if b2<128 then begin if(b3<bc)or(b3>ec)then goto 11 end else if toint(
256)*(b2-128)+b3>=nk then goto 11;end;end;
if b0=255 then bchlabel:=256*b2+b3;end;
for k:=kernbase[f]to extenbase[f]-1 do begin readtfmword;
sw:=(((((b3*z)div 256)+(b2*z))div 256)+(b1*z))div beta;
if b0=0 then fontinfo[k].sc:=sw else if b0=255 then fontinfo[k].sc:=sw-
alpha else goto 11;end;end{:66};{67:}
for k:=extenbase[f]to parambase[f]-1 do begin begin readtfmword;
qw.B0:=b0+0;qw.B1:=b1+0;qw.B2:=b2+0;qw.B3:=b3+0;fontinfo[k].qqqq:=qw;
end;if b0<>0 then begin if(b0<bc)or(b0>ec)then goto 11 end;
if b1<>0 then begin if(b1<bc)or(b1>ec)then goto 11 end;
if b2<>0 then begin if(b2<bc)or(b2>ec)then goto 11 end;
begin if(b3<bc)or(b3>ec)then goto 11 end;end{:67};{68:}
begin for k:=1 to np do if k=1 then begin readtfmword;
if b0>127 then sw:=b0-256 else sw:=b0;sw:=sw*256+b1;sw:=sw*256+b2;
fontinfo[parambase[f]].sc:=(sw*16)+(b3 div 16);
end else begin readtfmword;
sw:=(((((b3*z)div 256)+(b2*z))div 256)+(b1*z))div beta;
if b0=0 then fontinfo[parambase[f]+k-1].sc:=sw else if b0=255 then
fontinfo[parambase[f]+k-1].sc:=sw-alpha else goto 11;end;
for k:=np+1 to 8 do fontinfo[parambase[f]+k-1].sc:=0;end{:68};{69:}
fontbc[f]:=bc;fontec[f]:=ec;
if bchlabel<nl then bcharlabel[f]:=bchlabel+ligkernbase[f]else
bcharlabel[f]:=fontmemsize;fontbchar[f]:=bchar+0;
widthbase[f]:=widthbase[f]-0;ligkernbase[f]:=ligkernbase[f]-0;
kernbase[f]:=kernbase[f]-0;extenbase[f]:=extenbase[f]-0;
parambase[f]:=parambase[f]-1;fmemptr:=fmemptr+lf;goto 30{:69}{:59};
11:begin writeln(stdout);write(stdout,'Bad TFM file for');end;
case f of 1:begin writeln(stdout,'titles!');uexit(1);end;
2:begin writeln(stdout,'labels!');uexit(1);end;
3:begin writeln(stdout,'pixels!');uexit(1);end;
4:begin writeln(stdout,'slants!');uexit(1);end;
5:begin writeln(stdout,'METAFONT logo!');uexit(1);end;end;30:end;{:58}
{74:}function makestring:strnumber;
begin if strptr=maxstrings then begin writeln(stdout,'Too many labels!')
;uexit(1);end;strptr:=strptr+1;strstart[strptr]:=poolptr;
makestring:=strptr-1;end;{:74}{75:}procedure firststring(c:integer);
begin if strptr<>c then begin writeln(stdout,'?');uexit(1);end;
while l>0 do begin begin strpool[poolptr]:=buffer[l];poolptr:=poolptr+1;
end;l:=l-1;end;strptr:=strptr+1;strstart[strptr]:=poolptr;end;{:75}{81:}
function interpretxxx:keywordcode;label 30,31,45;var k:integer;
j:integer;l:0..13;m:keywordcode;n1:0..13;n2:poolpointer;c:keywordcode;
begin c:=19;curstring:=0;case curgf of 244:goto 30;
243:begin k:=signedquad;goto 30;end;239:k:=getbyte;240:k:=gettwobytes;
241:k:=getthreebytes;242:k:=signedquad;end;{82:}j:=0;
if k<2 then goto 45;while true do begin l:=j;if j=k then goto 31;
if j=13 then goto 45;j:=j+1;buffer[j]:=getbyte;
if buffer[j]=32 then goto 31;end;31:{83:}
for m:=0 to 18 do if(strstart[m+1]-strstart[m])=l then begin n1:=0;
n2:=strstart[m];
while(n1<l)and(buffer[n1+1]=strpool[n2])do begin n1:=n1+1;n2:=n2+1;end;
if n1=l then begin c:=m;if m=0 then begin j:=j+1;labeltype:=getbyte;end;
begin if poolptr+k-j>poolsize then begin writeln(stdout,
'Too many strings!');uexit(1);end;end;while j<k do begin j:=j+1;
begin strpool[poolptr]:=getbyte;poolptr:=poolptr+1;end;end;
curstring:=makestring;goto 30;end;end{:83};45:while j<k do begin j:=j+1;
curgf:=getbyte;end{:82};30:curgf:=getbyte;interpretxxx:=c;end;{:81}{84:}
function getyyy:scaled;var v:scaled;
begin if curgf<>243 then getyyy:=0 else begin v:=signedquad;
curgf:=getbyte;getyyy:=v;end;end;{:84}{85:}procedure skipnop;label 30;
var k:integer;j:integer;begin case curgf of 244:goto 30;
243:begin k:=signedquad;goto 30;end;239:k:=getbyte;240:k:=gettwobytes;
241:k:=getthreebytes;242:k:=signedquad;end;
for j:=1 to k do curgf:=getbyte;30:curgf:=getbyte;end;{:85}{89:}
procedure beginname;begin areadelimiter:=0;extdelimiter:=0;end;{:89}
{90:}function morename(c:ASCIIcode):boolean;
begin if c=32 then morename:=false else begin if(c=47)then begin
areadelimiter:=poolptr;extdelimiter:=0;
end else if c=46 then extdelimiter:=poolptr;
begin if poolptr+1>poolsize then begin writeln(stdout,
'Too many strings!');uexit(1);end;end;begin strpool[poolptr]:=c;
poolptr:=poolptr+1;end;morename:=true;end;end;{:90}{91:}
procedure endname;
begin if strptr+3>maxstrings then begin writeln(stdout,
'Too many strings!');uexit(1);end;
if areadelimiter=0 then curarea:=0 else begin curarea:=strptr;
strptr:=strptr+1;strstart[strptr]:=areadelimiter+1;end;
if extdelimiter=0 then begin curext:=0;curname:=makestring;
end else begin curname:=strptr;strptr:=strptr+1;
strstart[strptr]:=extdelimiter;curext:=makestring;end;end;{:91}{92:}
procedure packfilename(n,a,e:strnumber);var k:integer;c:ASCIIcode;
j:integer;namelength:0..PATHMAX;begin k:=0;
for j:=strstart[a]to strstart[a+1]-1 do begin c:=strpool[j];k:=k+1;
if k<=PATHMAX then nameoffile[k]:=xchr[c];end;
for j:=strstart[n]to strstart[n+1]-1 do begin c:=strpool[j];k:=k+1;
if k<=PATHMAX then nameoffile[k]:=xchr[c];end;
for j:=strstart[e]to strstart[e+1]-1 do begin c:=strpool[j];k:=k+1;
if k<=PATHMAX then nameoffile[k]:=xchr[c];end;
if k<=PATHMAX then namelength:=k else namelength:=PATHMAX;
for k:=namelength+1 to PATHMAX do nameoffile[k]:=' ';end;{:92}{94:}
procedure startgf;label 30;
var argbuffer:packed array[1..PATHMAX]of char;argbufptr:1..PATHMAX;
begin if optind=argc then begin write(stdout,'GF file name: ');inputln;
end else begin argv(optind,argbuffer);argbuffer[PATHMAX]:=' ';
argbufptr:=1;linelength:=0;
while(argbufptr<PATHMAX)and(argbuffer[argbufptr]=' ')do argbufptr:=
argbufptr+1;
while(argbufptr<PATHMAX)and(linelength<terminallinelength)and(argbuffer[
argbufptr]<>' ')do begin buffer[linelength]:=xord[argbuffer[argbufptr]];
linelength:=linelength+1;argbufptr:=argbufptr+1;end;end;bufptr:=0;
buffer[linelength]:=63;while buffer[bufptr]=32 do bufptr:=bufptr+1;
if bufptr<linelength then begin{95:}
if buffer[linelength-1]=47 then begin interaction:=true;
linelength:=linelength-1;end;beginname;
while true do begin if bufptr=linelength then goto 30;
if not morename(buffer[bufptr])then goto 30;bufptr:=bufptr+1;end;
30:endname{:95};if curext=0 then curext:=19;
packfilename(curname,curarea,curext);opengffile;end;jobname:=curname;
packfilename(jobname,0,20);opendvifile;end;{:94}{107:}
procedure writedvi(a,b:dviindex);begin writechunk(dvifile,dvibuf,a,b);
end;{:107}{108:}procedure dviswap;
begin if dvilimit=dvibufsize then begin writedvi(0,halfbuf-1);
dvilimit:=halfbuf;dvioffset:=dvioffset+dvibufsize;dviptr:=0;
end else begin writedvi(halfbuf,dvibufsize-1);dvilimit:=dvibufsize;end;
end;{:108}{110:}procedure dvifour(x:integer);
begin if x>=0 then begin dvibuf[dviptr]:=x div 16777216;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;
end else begin x:=x+1073741824;x:=x+1073741824;
begin dvibuf[dviptr]:=(x div 16777216)+128;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;end;x:=x mod 16777216;
begin dvibuf[dviptr]:=x div 65536;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;x:=x mod 65536;
begin dvibuf[dviptr]:=x div 256;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;begin dvibuf[dviptr]:=x mod 256;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;end;{:110}{111:}
procedure dvifontdef(f:internalfontnumber);var k:integer;
begin begin dvibuf[dviptr]:=243;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;begin dvibuf[dviptr]:=f;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;
begin dvibuf[dviptr]:=fontcheck[f].B0-0;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;
begin dvibuf[dviptr]:=fontcheck[f].B1-0;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;
begin dvibuf[dviptr]:=fontcheck[f].B2-0;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;
begin dvibuf[dviptr]:=fontcheck[f].B3-0;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;dvifour(fontsize[f]);
dvifour(fontdsize[f]);
begin dvibuf[dviptr]:=(strstart[fontarea[f]+1]-strstart[fontarea[f]]);
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;
begin dvibuf[dviptr]:=(strstart[fontname[f]+1]-strstart[fontname[f]]);
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;{112:}
for k:=strstart[fontarea[f]]to strstart[fontarea[f]+1]-1 do begin dvibuf
[dviptr]:=strpool[k];dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;
end;
for k:=strstart[fontname[f]]to strstart[fontname[f]+1]-1 do begin dvibuf
[dviptr]:=strpool[k];dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;
end{:112};end;{98:}procedure loadfonts;label 30,22,40,45;
var f:internalfontnumber;i:fourquarters;j,k,v:integer;m:1..8;n1:0..13;
n2:poolpointer;begin if interaction then{99:}
while true do begin 45:begin writeln(stdout);
write(stdout,'Special font substitution: ');end;22:inputln;
if linelength=0 then goto 30;{100:}bufptr:=0;buffer[linelength]:=32;
while buffer[bufptr]<>32 do bufptr:=bufptr+1;
for m:=1 to 8 do if(strstart[m+1]-strstart[m])=bufptr then begin n1:=0;
n2:=strstart[m];
while(n1<bufptr)and(buffer[n1]=strpool[n2])do begin n1:=n1+1;n2:=n2+1;
end;if n1=bufptr then goto 40;end{:100};
write(stdout,'Please say, e.g., "grayfont foo" or "slantfontarea baz".')
;goto 45;40:{101:}bufptr:=bufptr+1;
begin if poolptr+linelength-bufptr>poolsize then begin writeln(stdout,
'Too many strings!');uexit(1);end;end;
while bufptr<linelength do begin begin strpool[poolptr]:=buffer[bufptr];
poolptr:=poolptr+1;end;bufptr:=bufptr+1;end;
if m>4 then fontarea[m-4]:=makestring else begin fontname[m]:=makestring
;fontarea[m]:=0;fontat[m]:=0;end;initstrptr:=strptr{:101};
write(stdout,'OK; any more? ');goto 22;end;30:{:99};
fontsnotloaded:=false;
for f:=1 to 5 do if(f<>4)or((strstart[fontname[f]+1]-strstart[fontname[f
]])>0)then begin if(strstart[fontarea[f]+1]-strstart[fontarea[f]])=0
then fontarea[f]:=34;packfilename(fontname[f],fontarea[f],21);
opentfmfile;readfontinfo(f,fontat[f]);
if fontarea[f]=34 then fontarea[f]:=0;dvifontdef(f);end;{137:}
if(strstart[fontname[4]+1]-strstart[fontname[4]])=0 then ruleslant:=0.0
else begin ruleslant:=fontinfo[1+parambase[4]].sc/65536;
slantn:=fontec[4];i:=fontinfo[charbase[4]+slantn].qqqq;
slantunit:=fontinfo[heightbase[4]+(i.B1-0)div 16].sc/slantn;end;
slantreported:=0.0;{:137}{169:}i:=fontinfo[charbase[3]+1].qqqq;
if not(i.B0>0)then begin writeln(stdout,'Missing pixel char!');uexit(1);
end;unscxratio:=fontinfo[widthbase[3]+i.B0].sc;xratio:=unscxratio/65536;
unscyratio:=fontinfo[heightbase[3]+(i.B1-0)div 16].sc;
yratio:=unscyratio/65536;
unscslantratio:=fontinfo[1+parambase[3]].sc*yratio;
slantratio:=unscslantratio/65536;
if xratio*yratio=0 then begin writeln(stdout,'Vanishing pixel size!');
uexit(1);end;fudgefactor:=(slantratio/xratio)/yratio;{:169}{175:}
grayrulethickness:=fontinfo[8+parambase[3]].sc;
if grayrulethickness=0 then grayrulethickness:=26214;{:175}{184:}
i:=fontinfo[charbase[3]+0].qqqq;
if not(i.B0>0)then begin writeln(stdout,'Missing dot char!');uexit(1);
end;dotwidth:=fontinfo[widthbase[3]+i.B0].sc;
dotheight:=fontinfo[heightbase[3]+(i.B1-0)div 16].sc;
delta:=fontinfo[2+parambase[2]].sc div 2;
thricexheight:=3*fontinfo[5+parambase[2]].sc;
halfxheight:=thricexheight div 6;{:184}{205:}
for k:=0 to 4095 do b[k]:=0;
for k:=fontbc[3]to fontec[3]do if k>=1 then if k<=120 then if(fontinfo[
charbase[3]+k].qqqq.B0>0)then begin v:=c[k];repeat b[v]:=k;v:=v+d[k];
until v>4095;end;{:205}{206:}for j:=0 to 11 do begin k:=twotothe[j];
v:=k;repeat rho[v]:=k;v:=v+k+k;until v>4095;end;rho[0]:=4096;{:206};end;
{:98}{:111}{113:}procedure typeset(c:eightbits);
begin if c>=128 then begin dvibuf[dviptr]:=128;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;begin dvibuf[dviptr]:=c;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;end;{:113}{114:}
procedure dviscaled(x:real);var n:integer;m:integer;k:integer;
begin n:=round(x/6553.6);if n<0 then begin begin dvibuf[dviptr]:=45;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;n:=-n;end;
m:=n div 10;k:=0;repeat k:=k+1;buffer[k]:=(m mod 10)+48;m:=m div 10;
until m=0;repeat begin dvibuf[dviptr]:=buffer[k];dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;k:=k-1;until k=0;
if n mod 10<>0 then begin begin dvibuf[dviptr]:=46;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;begin dvibuf[dviptr]:=(n mod 10)+48;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;end;end;{:114}
{116:}procedure hbox(s:strnumber;f:internalfontnumber;sendit:boolean);
label 22,30;var k,endk,maxk:poolpointer;i,j:fourquarters;curl:0..256;
curr:0..256;bchar:0..256;stackptr:0..liglookahead;l:fontindex;
kernamount:scaled;hd:eightbits;x:scaled;savec:ASCIIcode;
begin boxwidth:=0;boxheight:=0;boxdepth:=0;k:=strstart[s];
maxk:=strstart[s+1];savec:=strpool[maxk];strpool[maxk]:=32;
while k<maxk do begin if strpool[k]=32 then{119:}
begin boxwidth:=boxwidth+fontinfo[2+parambase[f]].sc;
if sendit then begin begin dvibuf[dviptr]:=146;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;
dvifour(fontinfo[2+parambase[f]].sc);end;k:=k+1;end{:119}
else begin endk:=k;repeat endk:=endk+1;until strpool[endk]=32;
kernamount:=0;curl:=256;stackptr:=0;bchar:=fontbchar[f];
if k<endk then curr:=strpool[k]+0 else curr:=bchar;suppresslig:=false;
22:{120:}if(curl<fontbc[f])or(curl>fontec[f])then begin i:=dummyinfo;
if curl=256 then l:=bcharlabel[f]else l:=fontmemsize;
end else begin i:=fontinfo[charbase[f]+curl].qqqq;
if((i.B2-0)mod 4)<>1 then l:=fontmemsize else begin l:=ligkernbase[f]+i.
B3;j:=fontinfo[l].qqqq;
if j.B0-0>128 then l:=ligkernbase[f]+256*(j.B2-0)+j.B3;end;end;
if suppresslig then suppresslig:=false else while l<kernbase[f]+0 do
begin j:=fontinfo[l].qqqq;
if j.B1=curr then if j.B0-0<=128 then if j.B2-0>=128 then begin
kernamount:=fontinfo[kernbase[f]+256*(j.B2-128)+j.B3].sc;goto 30;
end else{122:}begin case j.B2-0 of 1,5:curl:=j.B3-0;
2,6:begin curr:=j.B3;if stackptr=0 then begin stackptr:=1;
if k<endk then k:=k+1 else bchar:=256;end;ligstack[stackptr]:=curr;end;
3,7,11:begin curr:=j.B3;stackptr:=stackptr+1;ligstack[stackptr]:=curr;
if j.B2-0=11 then suppresslig:=true;end;others:begin curl:=j.B3-0;
if stackptr>0 then begin stackptr:=stackptr-1;
if stackptr>0 then curr:=ligstack[stackptr]else if k<endk then curr:=
strpool[k]+0 else curr:=bchar;
end else if k=endk then goto 30 else begin k:=k+1;
if k<endk then curr:=strpool[k]+0 else curr:=bchar;end;end end;
if j.B2-0>3 then goto 30;goto 22;end{:122};if j.B0-0>=128 then goto 30;
l:=l+j.B0+1;end;30:{:120};{121:}
if(i.B0>0)then begin boxwidth:=boxwidth+fontinfo[widthbase[f]+i.B0].sc+
kernamount;hd:=i.B1-0;x:=fontinfo[heightbase[f]+(hd)div 16].sc;
if x>boxheight then boxheight:=x;x:=fontinfo[depthbase[f]+hd mod 16].sc;
if x>boxdepth then boxdepth:=x;if sendit then begin typeset(curl);
if kernamount<>0 then begin begin dvibuf[dviptr]:=146;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;dvifour(kernamount);end;end;
kernamount:=0;end{:121};{123:}curl:=curr-0;
if stackptr>0 then begin begin stackptr:=stackptr-1;
if stackptr>0 then curr:=ligstack[stackptr]else if k<endk then curr:=
strpool[k]+0 else curr:=bchar;end;goto 22;end;
if k<endk then begin k:=k+1;
if k<endk then curr:=strpool[k]+0 else curr:=bchar;goto 22;end{:123};
end;end;strpool[maxk]:=savec;end;{:116}{138:}
procedure slantcomplaint(r:real);
begin if abs(r-slantreported)>0.001 then begin begin writeln(stdout);
write(stdout,'Sorry, I can''t make diagonal rules of slant ');end;
printreal(r,10,5);write(stdout,'!');slantreported:=r;end;end;{:138}
{141:}function getavail:nodepointer;begin maxnode:=maxnode+1;
if maxnode=maxlabels then begin writeln(stdout,
'Too many labels and/or rules!');uexit(1);end;getavail:=maxnode;end;
{:141}{143:}procedure nodeins(p,q:nodepointer);var r:nodepointer;
begin if yy[p]>=yy[q]then begin repeat r:=q;q:=next[q];
until yy[p]<=yy[q];next[r]:=p;prev[p]:=r;next[p]:=q;prev[q]:=p;
end else begin repeat r:=q;q:=prev[q];until yy[p]>=yy[q];prev[r]:=p;
next[p]:=r;prev[p]:=q;next[q]:=p;end;
if yy[p]-yt[p]>maxheight then maxheight:=yy[p]-yt[p];
if yb[p]-yy[p]>maxdepth then maxdepth:=yb[p]-yy[p];end;{:143}{145:}
function overlap(p,q:nodepointer):boolean;label 10;var ythresh:scaled;
xleft,xright,ytop,ybot:scaled;r:nodepointer;begin xleft:=xl[p];
xright:=xr[p];ytop:=yt[p];ybot:=yb[p];{146:}ythresh:=ybot+maxheight;
r:=next[q];
while yy[r]<ythresh do begin if ybot>yt[r]then if xleft<xr[r]then if
xright>xl[r]then if ytop<yb[r]then begin overlap:=true;goto 10;end;
r:=next[r];end{:146};{147:}ythresh:=ytop-maxdepth;r:=q;
while yy[r]>ythresh do begin if ybot>yt[r]then if xleft<xr[r]then if
xright>xl[r]then if ytop<yb[r]then begin overlap:=true;goto 10;end;
r:=prev[r];end{:147};overlap:=false;10:end;{:145}{150:}
function nearestdot(p:nodepointer;d0:scaled):nodepointer;
var bestq:nodepointer;dmin,d:scaled;begin twin:=false;bestq:=0;
dmin:=268435456;{151:}q:=next[p];
while yy[q]<yy[p]+dmin do begin d:=abs(xx[q]-xx[p]);
if d<yy[q]-yy[p]then d:=yy[q]-yy[p];
if d<d0 then twin:=true else if d<dmin then begin dmin:=d;bestq:=q;end;
q:=next[q];end{:151};{152:}q:=prev[p];
while yy[q]>yy[p]-dmin do begin d:=abs(xx[q]-xx[p]);
if d<yy[p]-yy[q]then d:=yy[p]-yy[q];
if d<d0 then twin:=true else if d<dmin then begin dmin:=d;bestq:=q;end;
q:=prev[q];end{:152};nearestdot:=bestq;end;{:150}{167:}
procedure convert(x,y:scaled);begin x:=x+xoffset;y:=y+yoffset;
dviy:=-round(yratio*y)+deltay;dvix:=round(xratio*x+slantratio*y)+deltax;
end;{:167}{171:}procedure dvigoto(x,y:scaled);
begin begin dvibuf[dviptr]:=141;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;
if x<>0 then begin begin dvibuf[dviptr]:=146;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;dvifour(x);end;
if y<>0 then begin begin dvibuf[dviptr]:=160;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;dvifour(y);end;end;{:171}{185:}
procedure topcoords(p:nodepointer);begin xx[p]:=dvix-(boxwidth div 2);
xl[p]:=xx[p]-delta;xr[p]:=xx[p]+boxwidth+delta;yb[p]:=dviy-dotheight;
yy[p]:=yb[p]-boxdepth;yt[p]:=yy[p]-boxheight-delta;end;{:185}{186:}
procedure botcoords(p:nodepointer);begin xx[p]:=dvix-(boxwidth div 2);
xl[p]:=xx[p]-delta;xr[p]:=xx[p]+boxwidth+delta;yt[p]:=dviy+dotheight;
yy[p]:=yt[p]+boxheight;yb[p]:=yy[p]+boxdepth+delta;end;
procedure rightcoords(p:nodepointer);begin xl[p]:=dvix+dotwidth;
xx[p]:=xl[p];xr[p]:=xx[p]+boxwidth+delta;yy[p]:=dviy+halfxheight;
yb[p]:=yy[p]+boxdepth+delta;yt[p]:=yy[p]-boxheight-delta;end;
procedure leftcoords(p:nodepointer);begin xr[p]:=dvix-dotwidth;
xx[p]:=xr[p]-boxwidth;xl[p]:=xx[p]-delta;yy[p]:=dviy+halfxheight;
yb[p]:=yy[p]+boxdepth+delta;yt[p]:=yy[p]-boxheight-delta;end;{:186}
{194:}function placelabel(p:nodepointer):boolean;label 10,40;
var oct:0..15;dfl:nodepointer;begin hbox(info[p],2,false);dvix:=xx[p];
dviy:=yy[p];{195:}dfl:=xl[p];oct:=xr[p];{196:}
case oct of 0,4,9,13:leftcoords(p);1,2,8,11:botcoords(p);
3,7,10,14:rightcoords(p);6,5,15,12:topcoords(p);end;
if not overlap(p,dfl)then goto 40{:196};{197:}
case oct of 0,3,15,12:botcoords(p);1,5,10,14:leftcoords(p);
2,6,9,13:rightcoords(p);7,4,8,11:topcoords(p);end;
if not overlap(p,dfl)then goto 40{:197};{198:}
case oct of 0,3,14,13:topcoords(p);1,5,11,15:rightcoords(p);
2,6,8,12:leftcoords(p);7,4,9,10:botcoords(p);end;
if not overlap(p,dfl)then goto 40{:198};{199:}
case oct of 0,4,8,12:rightcoords(p);1,2,9,10:topcoords(p);
3,7,11,15:leftcoords(p);6,5,14,13:botcoords(p);end;
if not overlap(p,dfl)then goto 40{:199};xx[p]:=dvix;yy[p]:=dviy;
xl[p]:=dfl;placelabel:=false;goto 10{:195};40:nodeins(p,dfl);
dvigoto(xx[p],yy[p]);hbox(info[p],2,true);begin dvibuf[dviptr]:=142;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;placelabel:=true;
10:end;{:194}{218:}procedure dopixels;label 30,31,21,22,10;
var paintblack:boolean;startingcol,finishingcol:0..widestrow;
j:0..widestrow;l:integer;i:fourquarters;v:eightbits;
begin begin dvibuf[dviptr]:=174;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;
deltax:=deltax+round(unscxratio*minx);for j:=0 to maxx-minx do a[j]:=0;
l:=1;z:=0;startingcol:=0;finishingcol:=0;y:=maxy+12;paintblack:=false;
blankrows:=0;curgf:=getbyte;while true do begin{213:}repeat{214:}
if blankrows>0 then blankrows:=blankrows-1 else if curgf<>69 then begin
x:=z;if startingcol>x then startingcol:=x;{215:}
while true do begin 22:if(curgf>=74)and(curgf<=238)then begin z:=curgf
-74;paintblack:=true;curgf:=getbyte;goto 31;
end else case curgf of 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19
,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43
,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63:k:=curgf;
64:k:=getbyte;65:k:=gettwobytes;66:k:=getthreebytes;69:goto 31;
70:begin blankrows:=0;z:=0;paintblack:=false;curgf:=getbyte;goto 31;end;
71:begin blankrows:=getbyte;z:=0;paintblack:=false;curgf:=getbyte;
goto 31;end;72:begin blankrows:=gettwobytes;z:=0;paintblack:=false;
curgf:=getbyte;goto 31;end;73:begin blankrows:=getthreebytes;z:=0;
paintblack:=false;curgf:=getbyte;goto 31;end;
239,240,241,242,243,244:begin skipnop;goto 22;end;
others:begin writeln(stdout,'Bad GF file: ','Improper opcode',
'! (at byte ',curloc-1:1,')');uexit(1);end end;{216:}
if x+k>finishingcol then finishingcol:=x+k;
if paintblack then for j:=x to x+k-1 do a[j]:=a[j]+l;
paintblack:=not paintblack;x:=x+k;curgf:=getbyte{:216};end;31:{:215};
end;{:214};l:=l+l;y:=y-1;until l=4096;{:213};
dvigoto(0,deltay-round(unscyratio*y));{208:}j:=startingcol;
while true do begin while(j<=finishingcol)and(b[a[j]]=0)do j:=j+1;
if j>finishingcol then goto 30;begin dvibuf[dviptr]:=141;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;{209:}
begin dvibuf[dviptr]:=146;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;
dvifour(round(unscxratio*j+unscslantratio*y)+deltax){:209};
repeat v:=b[a[j]];a[j]:=a[j]-c[v];k:=j;j:=j+1;
while b[a[j]]=v do begin a[j]:=a[j]-c[v];j:=j+1;end;k:=j-k;{210:}
21:if k=1 then typeset(v)else begin i:=fontinfo[charbase[3]+v].qqqq;
if((i.B2-0)mod 4)=2 then begin if odd(k)then typeset(v);k:=k div 2;
v:=i.B3-0;goto 21;end else repeat typeset(v);k:=k-1;until k=0;end{:210};
until b[a[j]]=0;begin dvibuf[dviptr]:=142;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;end;30:{:208};
begin dvibuf[dviptr]:=142;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;{217:}l:=rho[a[startingcol]];
for j:=startingcol+1 to finishingcol do if l>rho[a[j]]then l:=rho[a[j]];
if l=4096 then if curgf=69 then goto 10 else begin y:=y-blankrows;
blankrows:=0;l:=1;startingcol:=z;finishingcol:=z;
end else begin while a[startingcol]=0 do startingcol:=startingcol+1;
while a[finishingcol]=0 do finishingcol:=finishingcol-1;
for j:=startingcol to finishingcol do a[j]:=a[j]div l;l:=4096 div l;
end{:217};end;10:end;{:218}{219:}begin initialize;{77:}strptr:=0;
poolptr:=0;strstart[0]:=0;l:=0;firststring(0);l:=9;buffer[9]:=116;
buffer[8]:=105;buffer[7]:=116;buffer[6]:=108;buffer[5]:=101;
buffer[4]:=102;buffer[3]:=111;buffer[2]:=110;buffer[1]:=116;
firststring(1);l:=9;buffer[9]:=108;buffer[8]:=97;buffer[7]:=98;
buffer[6]:=101;buffer[5]:=108;buffer[4]:=102;buffer[3]:=111;
buffer[2]:=110;buffer[1]:=116;firststring(2);l:=8;buffer[8]:=103;
buffer[7]:=114;buffer[6]:=97;buffer[5]:=121;buffer[4]:=102;
buffer[3]:=111;buffer[2]:=110;buffer[1]:=116;firststring(3);l:=9;
buffer[9]:=115;buffer[8]:=108;buffer[7]:=97;buffer[6]:=110;
buffer[5]:=116;buffer[4]:=102;buffer[3]:=111;buffer[2]:=110;
buffer[1]:=116;firststring(4);l:=13;buffer[13]:=116;buffer[12]:=105;
buffer[11]:=116;buffer[10]:=108;buffer[9]:=101;buffer[8]:=102;
buffer[7]:=111;buffer[6]:=110;buffer[5]:=116;buffer[4]:=97;
buffer[3]:=114;buffer[2]:=101;buffer[1]:=97;firststring(5);l:=13;
buffer[13]:=108;buffer[12]:=97;buffer[11]:=98;buffer[10]:=101;
buffer[9]:=108;buffer[8]:=102;buffer[7]:=111;buffer[6]:=110;
buffer[5]:=116;buffer[4]:=97;buffer[3]:=114;buffer[2]:=101;
buffer[1]:=97;firststring(6);l:=12;buffer[12]:=103;buffer[11]:=114;
buffer[10]:=97;buffer[9]:=121;buffer[8]:=102;buffer[7]:=111;
buffer[6]:=110;buffer[5]:=116;buffer[4]:=97;buffer[3]:=114;
buffer[2]:=101;buffer[1]:=97;firststring(7);l:=13;buffer[13]:=115;
buffer[12]:=108;buffer[11]:=97;buffer[10]:=110;buffer[9]:=116;
buffer[8]:=102;buffer[7]:=111;buffer[6]:=110;buffer[5]:=116;
buffer[4]:=97;buffer[3]:=114;buffer[2]:=101;buffer[1]:=97;
firststring(8);l:=11;buffer[11]:=116;buffer[10]:=105;buffer[9]:=116;
buffer[8]:=108;buffer[7]:=101;buffer[6]:=102;buffer[5]:=111;
buffer[4]:=110;buffer[3]:=116;buffer[2]:=97;buffer[1]:=116;
firststring(9);l:=11;buffer[11]:=108;buffer[10]:=97;buffer[9]:=98;
buffer[8]:=101;buffer[7]:=108;buffer[6]:=102;buffer[5]:=111;
buffer[4]:=110;buffer[3]:=116;buffer[2]:=97;buffer[1]:=116;
firststring(10);l:=10;buffer[10]:=103;buffer[9]:=114;buffer[8]:=97;
buffer[7]:=121;buffer[6]:=102;buffer[5]:=111;buffer[4]:=110;
buffer[3]:=116;buffer[2]:=97;buffer[1]:=116;firststring(11);l:=11;
buffer[11]:=115;buffer[10]:=108;buffer[9]:=97;buffer[8]:=110;
buffer[7]:=116;buffer[6]:=102;buffer[5]:=111;buffer[4]:=110;
buffer[3]:=116;buffer[2]:=97;buffer[1]:=116;firststring(12);l:=4;
buffer[4]:=114;buffer[3]:=117;buffer[2]:=108;buffer[1]:=101;
firststring(13);l:=5;buffer[5]:=116;buffer[4]:=105;buffer[3]:=116;
buffer[2]:=108;buffer[1]:=101;firststring(14);l:=13;buffer[13]:=114;
buffer[12]:=117;buffer[11]:=108;buffer[10]:=101;buffer[9]:=116;
buffer[8]:=104;buffer[7]:=105;buffer[6]:=99;buffer[5]:=107;
buffer[4]:=110;buffer[3]:=101;buffer[2]:=115;buffer[1]:=115;
firststring(15);l:=6;buffer[6]:=111;buffer[5]:=102;buffer[4]:=102;
buffer[3]:=115;buffer[2]:=101;buffer[1]:=116;firststring(16);l:=7;
buffer[7]:=120;buffer[6]:=111;buffer[5]:=102;buffer[4]:=102;
buffer[3]:=115;buffer[2]:=101;buffer[1]:=116;firststring(17);l:=7;
buffer[7]:=121;buffer[6]:=111;buffer[5]:=102;buffer[4]:=102;
buffer[3]:=115;buffer[2]:=101;buffer[1]:=116;firststring(18);{:77}{78:}
l:=7;buffer[7]:=46;buffer[6]:=50;buffer[5]:=54;buffer[4]:=48;
buffer[3]:=50;buffer[2]:=103;buffer[1]:=102;firststring(19);l:=4;
buffer[4]:=46;buffer[3]:=100;buffer[2]:=118;buffer[1]:=105;
firststring(20);l:=4;buffer[4]:=46;buffer[3]:=116;buffer[2]:=102;
buffer[1]:=109;firststring(21);l:=7;buffer[7]:=32;buffer[6]:=32;
buffer[5]:=80;buffer[4]:=97;buffer[3]:=103;buffer[2]:=101;buffer[1]:=32;
firststring(22);l:=12;buffer[12]:=32;buffer[11]:=32;buffer[10]:=67;
buffer[9]:=104;buffer[8]:=97;buffer[7]:=114;buffer[6]:=97;buffer[5]:=99;
buffer[4]:=116;buffer[3]:=101;buffer[2]:=114;buffer[1]:=32;
firststring(23);l:=6;buffer[6]:=32;buffer[5]:=32;buffer[4]:=69;
buffer[3]:=120;buffer[2]:=116;buffer[1]:=32;firststring(24);l:=4;
buffer[4]:=32;buffer[3]:=32;buffer[2]:=96;buffer[1]:=96;firststring(25);
l:=2;buffer[2]:=39;buffer[1]:=39;firststring(26);l:=3;buffer[3]:=32;
buffer[2]:=61;buffer[1]:=32;firststring(27);l:=4;buffer[4]:=32;
buffer[3]:=43;buffer[2]:=32;buffer[1]:=40;firststring(28);l:=4;
buffer[4]:=99;buffer[3]:=109;buffer[2]:=114;buffer[1]:=56;
firststring(29);l:=6;buffer[6]:=99;buffer[5]:=109;buffer[4]:=116;
buffer[3]:=116;buffer[2]:=49;buffer[1]:=48;firststring(30);l:=4;
buffer[4]:=103;buffer[3]:=114;buffer[2]:=97;buffer[1]:=121;
firststring(31);l:=5;buffer[5]:=108;buffer[4]:=111;buffer[3]:=103;
buffer[2]:=111;buffer[1]:=56;firststring(32);l:=8;buffer[8]:=77;
buffer[7]:=69;buffer[6]:=84;buffer[5]:=65;buffer[4]:=70;buffer[3]:=79;
buffer[2]:=78;buffer[1]:=84;firststring(33);{:78}{88:}l:=0;
firststring(34);{:88};setpaths(GFFILEPATHBIT+TFMFILEPATHBIT);startgf;
{221:}
if getbyte<>247 then begin writeln(stdout,'Bad GF file: ','No preamble',
'! (at byte ',curloc-1:1,')');uexit(1);end;
if getbyte<>131 then begin writeln(stdout,'Bad GF file: ','Wrong ID',
'! (at byte ',curloc-1:1,')');uexit(1);end;k:=getbyte;
for m:=1 to k do begin strpool[poolptr]:=getbyte;poolptr:=poolptr+1;end;
begin dvibuf[dviptr]:=247;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;begin dvibuf[dviptr]:=2;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;dvifour(25400000);
dvifour(473628672);dvifour(1000);begin dvibuf[dviptr]:=k;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;uselogo:=false;
s:=strstart[strptr];
for m:=1 to k do begin dvibuf[dviptr]:=strpool[s+m-1];dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;
if strpool[s]=32 then if strpool[s+1]=77 then if strpool[s+2]=69 then if
strpool[s+3]=84 then if strpool[s+4]=65 then if strpool[s+5]=70 then if
strpool[s+6]=79 then if strpool[s+7]=78 then if strpool[s+8]=84 then
begin strptr:=strptr+1;strstart[strptr]:=s+9;uselogo:=true;end;
timestamp:=makestring{:221};curgf:=getbyte;initstrptr:=strptr;
while true do begin{144:}maxnode:=0;next[0]:=maxlabels;
prev[maxlabels]:=0;maxheight:=0;maxdepth:=0;{:144}{156:}
rulethickness:=0;offsetx:=0;offsety:=0;xoffset:=0;yoffset:=0;
preminx:=268435456;premaxx:=-268435456;preminy:=268435456;
premaxy:=-268435456;{:156}{161:}ruleptr:=0;titlehead:=0;titletail:=0;
next[maxlabels]:=0;labeltail:=maxlabels;firstdot:=maxlabels;{:161};
while(curgf>=239)and(curgf<=244)do{154:}begin k:=interpretxxx;
case k of 19:;
1,2,3,4:if fontsnotloaded then begin fontname[k]:=curstring;
fontarea[k]:=0;fontat[k]:=0;initstrptr:=strptr;
end else begin writeln(stdout);
write(stdout,'(Tardy font change will be ignored (byte ',curloc:1,')!)')
;end;5,6,7,8:if fontsnotloaded then begin fontarea[k-4]:=curstring;
initstrptr:=strptr;end else begin writeln(stdout);
write(stdout,'(Tardy font change will be ignored (byte ',curloc:1,')!)')
;end;9,10,11,12:if fontsnotloaded then begin fontat[k-8]:=getyyy;
initstrptr:=strptr;end else begin writeln(stdout);
write(stdout,'(Tardy font change will be ignored (byte ',curloc:1,')!)')
;end;15:rulethickness:=getyyy;13:{159:}begin p:=getavail;
next[p]:=ruleptr;ruleptr:=p;xl[p]:=getyyy;yt[p]:=getyyy;xr[p]:=getyyy;
yb[p]:=getyyy;if xl[p]<preminx then preminx:=xl[p];
if xl[p]>premaxx then premaxx:=xl[p];
if yt[p]<preminy then preminy:=yt[p];
if yt[p]>premaxy then premaxy:=yt[p];
if xr[p]<preminx then preminx:=xr[p];
if xr[p]>premaxx then premaxx:=xr[p];
if yb[p]<preminy then preminy:=yb[p];
if yb[p]>premaxy then premaxy:=yb[p];xx[p]:=rulethickness;end{:159};
16:{157:}begin offsetx:=getyyy;offsety:=getyyy;end{:157};
17:xoffset:=getyyy;18:yoffset:=getyyy;14:{162:}begin p:=getavail;
info[p]:=curstring;
if titlehead=0 then titlehead:=p else next[titletail]:=p;titletail:=p;
end{:162};0:{163:}
if(labeltype<47)or(labeltype>56)then begin writeln(stdout);
write(stdout,'Bad label type precedes byte ',curloc:1,'!');
end else begin p:=getavail;next[labeltail]:=p;labeltail:=p;
prev[p]:=labeltype;info[p]:=curstring;xx[p]:=getyyy;yy[p]:=getyyy;
if xx[p]<preminx then preminx:=xx[p];
if xx[p]>premaxx then premaxx:=xx[p];
if yy[p]<preminy then preminy:=yy[p];
if yy[p]>premaxy then premaxy:=yy[p];end{:163};end;end{:154};
if curgf=248 then{115:}begin begin dvibuf[dviptr]:=248;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;dvifour(lastbop);
lastbop:=dvioffset+dviptr-5;dvifour(25400000);dvifour(473628672);
dvifour(1000);dvifour(maxv);dvifour(maxh);begin dvibuf[dviptr]:=0;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;
begin dvibuf[dviptr]:=3;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;
begin dvibuf[dviptr]:=totalpages div 256;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;
begin dvibuf[dviptr]:=totalpages mod 256;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;
if not fontsnotloaded then for k:=1 to 5 do if(strstart[fontname[k]+1]-
strstart[fontname[k]])>0 then dvifontdef(k);begin dvibuf[dviptr]:=249;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;dvifour(lastbop);
begin dvibuf[dviptr]:=2;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;k:=4+((dvibufsize-dviptr)mod 4);
while k>0 do begin begin dvibuf[dviptr]:=223;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;k:=k-1;end;{109:}
if dvilimit=halfbuf then writedvi(halfbuf,dvibufsize-1);
if dviptr>0 then writedvi(0,dviptr-1){:109};
if verbose then writeln(stdout,' ');uexit(0);end{:115};
if curgf<>67 then if curgf<>68 then begin writeln(stdout,'Missing boc!')
;uexit(1);end;{164:}begin if fontsnotloaded then loadfonts;{165:}
if curgf=67 then begin ext:=signedquad;charcode:=ext mod 256;
if charcode<0 then charcode:=charcode+256;ext:=(ext-charcode)div 256;
k:=signedquad;minx:=signedquad;maxx:=signedquad;miny:=signedquad;
maxy:=signedquad;end else begin ext:=0;charcode:=getbyte;minx:=getbyte;
maxx:=getbyte;minx:=maxx-minx;miny:=getbyte;maxy:=getbyte;
miny:=maxy-miny;end;
if maxx-minx>widestrow then begin writeln(stdout,'Character too wide!');
uexit(1);end{:165};{170:}
if preminx<minx*65536 then offsetx:=offsetx+minx*65536-preminx;
if premaxy>maxy*65536 then offsety:=offsety+maxy*65536-premaxy;
if premaxx>maxx*65536 then premaxx:=premaxx div 65536 else premaxx:=maxx
;
if preminy<miny*65536 then preminy:=preminy div 65536 else preminy:=miny
;deltay:=round(unscyratio*(maxy+1)-yratio*offsety)+3276800;
deltax:=round(xratio*offsetx-unscxratio*minx);
if slantratio>=0 then overcol:=round(unscxratio*premaxx+unscslantratio*
maxy)else overcol:=round(unscxratio*premaxx+unscslantratio*miny);
overcol:=overcol+deltax+overflowlabeloffset;
pageheight:=round(unscyratio*(maxy+1-preminy))+3276800-offsety;
if pageheight>maxv then maxv:=pageheight;
pagewidth:=overcol-10000000{:170};{172:}begin dvibuf[dviptr]:=139;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;
totalpages:=totalpages+1;dvifour(totalpages);dvifour(charcode);
dvifour(ext);for k:=3 to 9 do dvifour(0);dvifour(lastbop);
lastbop:=dvioffset+dviptr-45;dvigoto(0,655360);
if uselogo then begin begin dvibuf[dviptr]:=176;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;hbox(33,5,true);end;
begin dvibuf[dviptr]:=172;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;hbox(timestamp,1,true);
hbox(22,1,true);dviscaled(totalpages*65536.0);
if(charcode<>0)or(ext<>0)then begin hbox(23,1,true);
dviscaled(charcode*65536.0);if ext<>0 then begin hbox(24,1,true);
dviscaled(ext*65536.0);end;end;
if titlehead<>0 then begin next[titletail]:=0;repeat hbox(25,1,true);
hbox(info[titlehead],1,true);hbox(26,1,true);titlehead:=next[titlehead];
until titlehead=0;end;begin dvibuf[dviptr]:=142;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end{:172};
if verbose then begin write(stdout,'[',totalpages:1);flush(stdout);end;
{173:}if ruleslant<>0 then begin dvibuf[dviptr]:=175;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;
while ruleptr<>0 do begin p:=ruleptr;ruleptr:=next[p];
if xx[p]=0 then xx[p]:=grayrulethickness;
if xx[p]>0 then begin convert(xl[p],yt[p]);tempx:=dvix;tempy:=dviy;
convert(xr[p],yb[p]);if abs(tempx-dvix)<6554 then{176:}
begin if tempy>dviy then begin k:=tempy;tempy:=dviy;dviy:=k;end;
dvigoto(dvix-(xx[p]div 2),dviy);begin dvibuf[dviptr]:=137;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;
dvifour(dviy-tempy);dvifour(xx[p]);begin dvibuf[dviptr]:=142;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;end{:176}
else if abs(tempy-dviy)<6554 then{177:}
begin if tempx<dvix then begin k:=tempx;tempx:=dvix;dvix:=k;end;
dvigoto(dvix,dviy+(xx[p]div 2));begin dvibuf[dviptr]:=137;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;dvifour(xx[p]);
dvifour(tempx-dvix);begin dvibuf[dviptr]:=142;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;end{:177}else{178:}
if(ruleslant=0)or(abs(tempx+ruleslant*(tempy-dviy)-dvix)>xx[p])then
slantcomplaint((dvix-tempx)/(tempy-dviy))else begin if tempy>dviy then
begin k:=tempy;tempy:=dviy;dviy:=k;k:=tempx;tempx:=dvix;dvix:=k;end;
m:=round((dviy-tempy)/slantunit);if m>0 then begin dvigoto(dvix,dviy);
q:=((m-1)div slantn)+1;k:=m div q;p:=m mod q;q:=q-p;{179:}typeset(k);
dy:=round(k*slantunit);begin dvibuf[dviptr]:=170;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;dvifour(-dy);
while q>1 do begin typeset(k);begin dvibuf[dviptr]:=166;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;q:=q-1;end{:179};
{180:}if p>0 then begin k:=k+1;typeset(k);dy:=round(k*slantunit);
begin dvibuf[dviptr]:=170;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;dvifour(-dy);
while p>1 do begin typeset(k);begin dvibuf[dviptr]:=166;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;p:=p-1;end;
end{:180};begin dvibuf[dviptr]:=142;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;end;end{:178};end;end{:173};{181:}
overflowline:=1;if next[maxlabels]<>0 then begin next[labeltail]:=0;
begin dvibuf[dviptr]:=174;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;{187:}p:=next[maxlabels];
firstdot:=maxnode+1;while p<>0 do begin convert(xx[p],yy[p]);
xx[p]:=dvix;yy[p]:=dviy;if prev[p]<53 then{188:}begin q:=getavail;
xl[p]:=q;info[q]:=p;xx[q]:=dvix;xl[q]:=dvix-dotwidth;
xr[q]:=dvix+dotwidth;yy[q]:=dviy;yt[q]:=dviy-dotheight;
yb[q]:=dviy+dotheight;nodeins(q,0);dvigoto(xx[q],yy[q]);
begin dvibuf[dviptr]:=0;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;begin dvibuf[dviptr]:=142;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;end{:188};
p:=next[p];end{:187};{191:}p:=next[maxlabels];
while p<>0 do begin if prev[p]<=48 then{192:}begin r:=xl[p];
q:=nearestdot(r,10);if twin then xr[p]:=8 else xr[p]:=0;
if q<>0 then begin dx:=xx[q]-xx[r];dy:=yy[q]-yy[r];
if dy>0 then xr[p]:=xr[p]+4;if dx<0 then xr[p]:=xr[p]+1;
if dy>dx then xr[p]:=xr[p]+1;if-dy>dx then xr[p]:=xr[p]+1;end;end{:192};
p:=next[p];end;{:191};begin dvibuf[dviptr]:=173;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;{189:}q:=maxlabels;
while next[q]<>0 do begin p:=next[q];
if prev[p]>48 then begin next[q]:=next[p];{190:}
begin hbox(info[p],2,false);dvix:=xx[p];dviy:=yy[p];
if prev[p]<53 then r:=xl[p]else r:=0;case prev[p]of 49,53:topcoords(p);
50,54:leftcoords(p);51,55:rightcoords(p);52,56:botcoords(p);end;
nodeins(p,r);dvigoto(xx[p],yy[p]);hbox(info[p],2,true);
begin dvibuf[dviptr]:=142;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;end{:190};end else q:=next[q];
end{:189};{193:}q:=maxlabels;while next[q]<>0 do begin p:=next[q];
r:=next[p];s:=xl[p];
if placelabel(p)then next[q]:=r else begin info[s]:=0;
if prev[p]=47 then next[q]:=r else q:=p;end;end{:193};{200:}{201:}
p:=next[0];while p<>maxlabels do begin q:=next[p];
if(p<firstdot)or(info[p]=0)then begin r:=prev[p];next[r]:=q;prev[q]:=r;
next[p]:=r;end;p:=q;end{:201};p:=next[maxlabels];
while p<>0 do begin{202:}begin r:=next[xl[p]];s:=next[r];t:=next[p];
next[p]:=s;prev[s]:=p;next[r]:=p;prev[p]:=r;q:=nearestdot(p,0);
next[r]:=s;prev[s]:=r;next[p]:=t;overflowline:=overflowline+1;
dvigoto(overcol,overflowline*thricexheight+655360);hbox(info[p],2,true);
if q<>0 then begin hbox(27,2,true);hbox(info[info[q]],2,true);
hbox(28,2,true);
dviscaled((xx[p]-xx[q])/xratio+(yy[p]-yy[q])*fudgefactor);
begin dvibuf[dviptr]:=44;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;dviscaled((yy[q]-yy[p])/yratio);
begin dvibuf[dviptr]:=41;dviptr:=dviptr+1;
if dviptr=dvilimit then dviswap;end;end;begin dvibuf[dviptr]:=142;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;end{:202};
p:=next[p];end{:200};end{:181};dopixels;begin dvibuf[dviptr]:=140;
dviptr:=dviptr+1;if dviptr=dvilimit then dviswap;end;{203:}
if overflowline>1 then pagewidth:=overcol+10000000;
if pagewidth>maxh then maxh:=pagewidth{:203};
if verbose then begin write(stdout,']');
if totalpages mod 13=0 then writeln(stdout,' ')else write(stdout,' ');
flush(stdout);end;end{:164};curgf:=getbyte;strptr:=initstrptr;
poolptr:=strstart[strptr];end;
if verbose and(totalpages mod 13<>0)then writeln(stdout,' ');end.{:219}
