{3:}{9:}{$C-,A+,D-}{[$C+,D+]}{:9}program DVIcopy;label 9999;const{5:}
maxfonts=100;maxchars=10000;maxwidths=3000;maxpackets=5000;
maxbytes=30000;maxrecursion=10;stacksize=100;terminallinelength=150;{:5}
type{7:}int31=0..2147483647;int24u=0..16777215;int24=-8388608..8388607;
int23=0..8388607;int16u=0..65535;int16=-32768..32767;int15=0..32767;
int8u=0..255;int8=-128..127;int7=0..127;{:7}{14:}ASCIIcode=0..255;{:14}
{15:}textfile=packed file of ASCIIcode;{:15}{27:}signedbyte=-128..127;
eightbits=0..255;signedpair=-32768..32767;sixteenbits=0..65535;
signedtrio=-8388608..8388607;twentyfourbits=0..16777215;
signedquad=integer;{:27}{29:}bytefile=packed file of eightbits;{:29}
{31:}packedbyte=eightbits;bytepointer=0..maxbytes;
pcktpointer=0..maxpackets;{:31}{36:}hashcode=0..353;{:36}{71:}
widthpointer=0..maxwidths;{pixvalue=-32768..32767;}{:71}{77:}
charoffset=-255..maxchars;charpointer=0..maxchars;{:77}{80:}ftype=0..2;
fontnumber=0..maxfonts;{:80}{84:}typeflag=0..31;{:84}{117:}cmdpar=0..12;
{:117}{120:}cmdcl=0..17;{:120}{154:}vfstate=array[0..1,0..1]of boolean;
{:154}{156:}vftype=0..4;{:156}{183:}stackpointer=0..stacksize;
stackindex=1..stacksize;pair32=array[0..1]of integer;
stackrecord=record hfield:integer;vfield:integer;{hhfield:pixvalue;
vvfield:pixvalue;}wxfield:pair32;yzfield:pair32;end;{:183}{209:}
recurpointer=0..maxrecursion;{:209}var{17:}
xord:array[ASCIIcode]of ASCIIcode;xchr:array[0..255]of ASCIIcode;{:17}
{21:}history:0..3;{:21}{32:}
bytemem:packed array[bytepointer]of packedbyte;
pcktstart:array[pcktpointer]of bytepointer;byteptr:bytepointer;
pcktptr:pcktpointer;{:32}{37:}plink:array[pcktpointer]of pcktpointer;
phash:array[hashcode]of pcktpointer;{:37}{46:}
strfonts,strchars,strwidths,strpackets,strbytes,strrecursion,strstack,
strnamelength:pcktpointer;{:46}{49:}curpckt:pcktpointer;
curloc:bytepointer;curlimit:bytepointer;{:49}{62:}
curname:packed array[1..PATHMAX]of char;curnamelength:int15;{:62}{65:}
{fres:int16u;resdigits:array[1..5]of char;nresdigits:int7;}{:65}{72:}
widths:array[widthpointer]of integer;
{pixwidths:array[widthpointer]of pixvalue;}
wlink:array[widthpointer]of widthpointer;
whash:array[hashcode]of widthpointer;nwidths:widthpointer;{:72}{78:}
charwidths:array[charpointer]of widthpointer;
{charpixels:array[charpointer]of pixvalue;}
charpackets:array[charpointer]of pcktpointer;nchars:charpointer;{:78}
{81:}nf:fontnumber;{:81}{82:}fntcheck:array[fontnumber]of integer;
fntscaled:array[fontnumber]of int31;fntdesign:array[fontnumber]of int31;
{fntspace:array[fontnumber]of integer;}
fntname:array[fontnumber]of pcktpointer;
fntbc:array[fontnumber]of eightbits;fntec:array[fontnumber]of eightbits;
fntchars:array[fontnumber]of charoffset;
fnttype:array[fontnumber]of ftype;
fntfont:array[fontnumber]of fontnumber;{:82}{85:}curfnt:fontnumber;
curext:int24;curres:int8u;curtype:typeflag;pcktext:int24;pcktres:int8u;
pcktdup:boolean;pcktprev:pcktpointer;pcktmmsg,pcktsmsg,pcktdmsg:int7;
{:85}{91:}tfmfile:bytefile;tfmext:pcktpointer;{:91}{97:}
tfmb0,tfmb1,tfmb2,tfmb3:eightbits;{:97}{102:}tfmconv:real;{:102}{109:}
dvifile:bytefile;dviloc:integer;
realnameoffile:packed array[0..PATHMAX]of ASCIIcode;i:integer;{:109}
{118:}dvipar:packed array[eightbits]of cmdpar;{:118}{121:}
dvicl:packed array[eightbits]of cmdcl;{:121}{123:}
dvicharcmd:array[boolean]of eightbits;
dvirulecmd:array[boolean]of eightbits;
dvirightcmd:array[7..9]of eightbits;
dvidowncmd:array[12..14]of eightbits;{:123}{125:}curcmd:eightbits;
curparm:integer;curclass:cmdcl;{:125}{126:}curwp:widthpointer;
curupd:boolean;curvdimen:integer;curhdimen:integer;{:126}{128:}
dviefnts:array[fontnumber]of integer;
dviifnts:array[fontnumber]of fontnumber;dvinf:fontnumber;{:128}{134:}
vffile:bytefile;vfloc:integer;vflimit:integer;vfext:pcktpointer;
vfcurfnt:fontnumber;{:134}{142:}{105:}z:integer;alpha:integer;
beta:int15;{:105}{:142}{146:}vfefnts:array[fontnumber]of integer;
vfifnts:array[fontnumber]of fontnumber;vfnf:fontnumber;lclnf:fontnumber;
{:146}{157:}vfmove:array[stackpointer]of vfstate;
vfpushloc:array[stackpointer]of bytepointer;
vflastloc:array[stackpointer]of bytepointer;
vflastend:array[stackpointer]of bytepointer;
vfpushnum:array[stackpointer]of eightbits;
vflast:array[stackpointer]of vftype;vfptr:stackpointer;
stackused:stackpointer;{:157}{158:}vfchartype:array[boolean]of vftype;
vfruletype:array[boolean]of vftype;{:158}{173:}widthdimen:integer;{:173}
{176:}scanptr:bytepointer;{:176}{181:}typesetting:boolean;{hconv:real;
vconv:real;hpixels:pixvalue;vpixels:pixvalue;temppix:pixvalue;}{:181}
{184:}stack:array[stackindex]of stackrecord;curstack:stackrecord;
zerostack:stackrecord;stackptr:stackpointer;{:184}{187:}
selectcount:array[0..9,0..9]of integer;
selectthere:array[0..9,0..9]of boolean;selectvals:array[0..9]of 0..9;
selectmax:array[0..9]of integer;outmag:integer;
count:array[0..9]of integer;numselect:0..10;curselect:0..10;
selected:boolean;alldone:boolean;strmag,strselect:pcktpointer;{:187}
{210:}recurfnt:array[recurpointer]of fontnumber;
recurext:array[recurpointer]of int24;
recurres:array[recurpointer]of eightbits;
recurpckt:array[recurpointer]of pcktpointer;
recurloc:array[recurpointer]of bytepointer;nrecur:recurpointer;
recurused:recurpointer;{:210}{221:}dvinum:int31;dviden:int31;
dvimag:int31;{:221}{234:}outfile:bytefile;
outfname:packed array[1..PATHMAX]of char;outloc:integer;outback:integer;
outmaxv:int31;outmaxh:int31;outstack:int16u;outpages:int16u;{:234}{245:}
outfnts:array[fontnumber]of fontnumber;outnf:fontnumber;
outfnt:fontnumber;{:245}{23:}{48:}procedure printpacket(p:pcktpointer);
var k:bytepointer;
begin for k:=pcktstart[p]to pcktstart[p+1]-1 do write(stdout,xchr[
bytemem[k]]);end;{:48}{60:}{procedure hexpacket(p:pcktpointer);
var j,k,l:bytepointer;d:int8u;begin j:=pcktstart[p]-1;
k:=pcktstart[p+1]-1;
writeln(stdout,' packet=',p:1,' start=',j+1:1,' length=',k-j:1);
for l:=j+1 to k do begin d:=(bytemem[l])div 16;
if d<10 then write(stdout,xchr[d+48])else write(stdout,xchr[d+55]);
d:=(bytemem[l])mod 16;
if d<10 then write(stdout,xchr[d+48])else write(stdout,xchr[d+55]);
if(l=k)or(((l-j)mod 16)=0)then writeln(stdout)else if((l-j)mod 4)=0 then
write(stdout,'  ')else write(stdout,' ');end;end;}{:60}{61:}
procedure printfont(f:fontnumber);var p:pcktpointer;k:bytepointer;
m:int31;begin write(stdout,' = ');p:=fntname[f];
for k:=pcktstart[p]+1 to pcktstart[p+1]-1 do write(stdout,xchr[bytemem[k
]]);m:=round((fntscaled[f]/fntdesign[f])*outmag);
if m<>1000 then write(stdout,' scaled ',m:1);end;{:61}
procedure closefilesandterminate;forward;procedure jumpout;
begin history:=3;closefilesandterminate;uexit(1);end;{:23}{24:}
procedure confusion(p:pcktpointer);
begin write(stdout,' !This can''t happen (');printpacket(p);
writeln(stdout,').');jumpout;end;{:24}{25:}
procedure overflow(p:pcktpointer;n:int16u);
begin write(stdout,' !Sorry, ','DVIcopy',' capacity exceeded [');
printpacket(p);writeln(stdout,'=',n:1,'].');jumpout;end;{:25}{95:}
procedure badtfm;begin write(stdout,'Bad TFM file');printfont(curfnt);
writeln(stdout,'!');begin writeln(stdout,' ',
'Use TFtoPL/PLtoTF to diagnose and correct the problem','.');jumpout;
end;end;procedure badfont;begin writeln(stdout);
case fnttype[curfnt]of 0:badtfm;{136:}
1:begin write(stdout,'Bad VF file');printfont(curfnt);
writeln(stdout,' loc=',vfloc:1);begin writeln(stdout,' ',
'Use VFtoVP/VPtoVF to diagnose and correct the problem','.');jumpout;
end;end;{:136}end;end;{:95}{110:}procedure baddvi;begin writeln(stdout);
writeln(stdout,'Bad DVI file: loc=',dviloc:1,'!');
write(stdout,' Use DVItype with output level');
if true then write(stdout,'=4')else write(stdout,'<4');
begin writeln(stdout,' ','to diagnose the problem','.');jumpout;end;end;
{:110}procedure initialize;var{16:}i:int16;{:16}{39:}h:hashcode;{:39}
begin writeln(stdout,'This is DVIcopy, C Version 1.2');
writeln(stdout,'Copyright (C) 1990,91 Peter Breitenlohner');
writeln(stdout,'Distributed under terms of GNU General Public License');
{18:}for i:=0 to 31 do xchr[i]:='?';xchr[32]:=' ';xchr[33]:='!';
xchr[34]:='"';xchr[35]:='#';xchr[36]:='$';xchr[37]:='%';xchr[38]:='&';
xchr[39]:='''';xchr[40]:='(';xchr[41]:=')';xchr[42]:='*';xchr[43]:='+';
xchr[44]:=',';xchr[45]:='-';xchr[46]:='.';xchr[47]:='/';xchr[48]:='0';
xchr[49]:='1';xchr[50]:='2';xchr[51]:='3';xchr[52]:='4';xchr[53]:='5';
xchr[54]:='6';xchr[55]:='7';xchr[56]:='8';xchr[57]:='9';xchr[58]:=':';
xchr[59]:=';';xchr[60]:='<';xchr[61]:='=';xchr[62]:='>';xchr[63]:='?';
xchr[64]:='@';xchr[65]:='A';xchr[66]:='B';xchr[67]:='C';xchr[68]:='D';
xchr[69]:='E';xchr[70]:='F';xchr[71]:='G';xchr[72]:='H';xchr[73]:='I';
xchr[74]:='J';xchr[75]:='K';xchr[76]:='L';xchr[77]:='M';xchr[78]:='N';
xchr[79]:='O';xchr[80]:='P';xchr[81]:='Q';xchr[82]:='R';xchr[83]:='S';
xchr[84]:='T';xchr[85]:='U';xchr[86]:='V';xchr[87]:='W';xchr[88]:='X';
xchr[89]:='Y';xchr[90]:='Z';xchr[91]:='[';xchr[92]:='\';xchr[93]:=']';
xchr[94]:='^';xchr[95]:='_';xchr[96]:='`';xchr[97]:='a';xchr[98]:='b';
xchr[99]:='c';xchr[100]:='d';xchr[101]:='e';xchr[102]:='f';
xchr[103]:='g';xchr[104]:='h';xchr[105]:='i';xchr[106]:='j';
xchr[107]:='k';xchr[108]:='l';xchr[109]:='m';xchr[110]:='n';
xchr[111]:='o';xchr[112]:='p';xchr[113]:='q';xchr[114]:='r';
xchr[115]:='s';xchr[116]:='t';xchr[117]:='u';xchr[118]:='v';
xchr[119]:='w';xchr[120]:='x';xchr[121]:='y';xchr[122]:='z';
xchr[123]:='{';xchr[124]:='|';xchr[125]:='}';xchr[126]:='~';
for i:=127 to 255 do xchr[i]:='?';{:18}{19:}
for i:=0 to 255 do xord[chr(i)]:=32;
for i:=32 to 126 do xord[xchr[i]]:=i;{:19}{22:}history:=0;{:22}{38:}
pcktptr:=1;byteptr:=1;pcktstart[0]:=1;pcktstart[1]:=1;
for h:=0 to 352 do phash[h]:=0;{:38}{73:}whash[0]:=1;wlink[1]:=0;
widths[0]:=0;widths[1]:=0;nwidths:=2;{pixwidths[0]:=0;pixwidths[1]:=0;}
for h:=1 to 352 do whash[h]:=0;{:73}{79:}nchars:=0;{:79}{83:}
{fntspace[maxfonts]:=0;}nf:=0;{:83}{86:}curfnt:=maxfonts;pcktmmsg:=0;
pcktsmsg:=0;pcktdmsg:=0;{:86}{119:}for i:=0 to 136 do dvipar[i]:=0;
for i:=138 to 255 do dvipar[i]:=1;dvipar[132]:=11;dvipar[137]:=11;
dvipar[143]:=2;dvipar[144]:=4;dvipar[145]:=6;dvipar[146]:=8;
for i:=171 to 234 do dvipar[i]:=12;dvipar[235]:=3;dvipar[236]:=5;
dvipar[237]:=7;dvipar[238]:=9;dvipar[239]:=3;dvipar[240]:=5;
dvipar[241]:=7;dvipar[242]:=10;
for i:=0 to 3 do begin dvipar[i+148]:=dvipar[i+143];
dvipar[i+153]:=dvipar[i+143];dvipar[i+157]:=dvipar[i+143];
dvipar[i+162]:=dvipar[i+143];dvipar[i+167]:=dvipar[i+143];
dvipar[i+243]:=dvipar[i+235];end;{:119}{122:}
for i:=0 to 136 do dvicl[i]:=0;dvicl[132]:=1;dvicl[137]:=1;
dvicl[138]:=17;dvicl[139]:=17;dvicl[140]:=17;dvicl[141]:=3;
dvicl[142]:=4;dvicl[147]:=5;dvicl[152]:=6;dvicl[161]:=10;dvicl[166]:=11;
for i:=0 to 3 do begin dvicl[i+143]:=7;dvicl[i+148]:=8;dvicl[i+153]:=9;
dvicl[i+157]:=12;dvicl[i+162]:=13;dvicl[i+167]:=14;dvicl[i+239]:=2;
dvicl[i+243]:=16;end;for i:=171 to 238 do dvicl[i]:=15;
for i:=247 to 255 do dvicl[i]:=17;{:122}{124:}dvicharcmd[false]:=133;
dvicharcmd[true]:=128;dvirulecmd[false]:=137;dvirulecmd[true]:=132;
dvirightcmd[7]:=143;dvirightcmd[8]:=148;dvirightcmd[9]:=153;
dvidowncmd[12]:=157;dvidowncmd[13]:=162;dvidowncmd[14]:=167;{:124}{129:}
dvinf:=0;{:129}{147:}lclnf:=0;{:147}{159:}vfmove[0][0][0]:=false;
vfmove[0][0][1]:=false;vfmove[0][1][0]:=false;vfmove[0][1][1]:=false;
stackused:=0;vfchartype[false]:=3;vfchartype[true]:=0;
vfruletype[false]:=4;vfruletype[true]:=1;{:159}{174:}
widthdimen:=-1073741824;widthdimen:=widthdimen-1073741824;{:174}{182:}
typesetting:=false;{:182}{185:}zerostack.hfield:=0;zerostack.vfield:=0;
{zerostack.hhfield:=0;zerostack.vvfield:=0;}
for i:=0 to 1 do begin zerostack.wxfield[i]:=0;zerostack.yzfield[i]:=0;
end;{:185}{211:}nrecur:=0;recurused:=0;{:211}{235:}outloc:=0;
outback:=-1;outmaxv:=0;outmaxh:=0;outstack:=0;outpages:=0;{:235}{246:}
outnf:=0;{:246}end;{:3}{40:}function makepacket:pcktpointer;label 31;
var i,k:bytepointer;h:hashcode;s,l:bytepointer;p:pcktpointer;
begin s:=pcktstart[pcktptr];l:=byteptr-s;
if l=0 then p:=0 else begin{41:}h:=bytemem[s];i:=s+1;
while i<byteptr do begin h:=(h+h+bytemem[i])mod 353;incr(i);end{:41};
{42:}p:=phash[h];
while p<>0 do begin if(pcktstart[p+1]-pcktstart[p])=l then{43:}
begin i:=s;k:=pcktstart[p];
while(i<byteptr)and(bytemem[i]=bytemem[k])do begin incr(i);incr(k);end;
if i=byteptr then begin byteptr:=pcktstart[pcktptr];goto 31;end;end{:43}
;p:=plink[p];end;p:=pcktptr;plink[p]:=phash[h];phash[h]:=p{:42};
if pcktptr=maxpackets then overflow(strpackets,maxpackets);
incr(pcktptr);pcktstart[pcktptr]:=byteptr;end;31:makepacket:=p;end;{:40}
{47:}function newpacket:pcktpointer;
begin if pcktptr=maxpackets then overflow(strpackets,maxpackets);
newpacket:=pcktptr;incr(pcktptr);pcktstart[pcktptr]:=byteptr;end;
procedure flushpacket;begin decr(pcktptr);byteptr:=pcktstart[pcktptr];
end;{:47}{50:}function pcktsbyte:int8;var a:eightbits;
begin{if curloc>=curlimit then confusion(strpackets)else}
begin a:=bytemem[curloc];incr(curloc);end;
if a<128 then pcktsbyte:=a else pcktsbyte:=a-256;end;
function pcktubyte:int8u;var a:eightbits;
begin{if curloc>=curlimit then confusion(strpackets)else}
begin a:=bytemem[curloc];incr(curloc);end;pcktubyte:=a;end;{:50}{51:}
function pcktspair:int16;var a,b:eightbits;
begin{if curloc>=curlimit then confusion(strpackets)else}
begin a:=bytemem[curloc];incr(curloc);end;
{if curloc>=curlimit then confusion(strpackets)else}
begin b:=bytemem[curloc];incr(curloc);end;
if a<128 then pcktspair:=a*toint(256)+b else pcktspair:=(a-toint(256))*
toint(256)+b;end;function pcktupair:int16u;var a,b:eightbits;
begin{if curloc>=curlimit then confusion(strpackets)else}
begin a:=bytemem[curloc];incr(curloc);end;
{if curloc>=curlimit then confusion(strpackets)else}
begin b:=bytemem[curloc];incr(curloc);end;pcktupair:=a*toint(256)+b;end;
{:51}{52:}function pcktstrio:int24;var a,b,c:eightbits;
begin{if curloc>=curlimit then confusion(strpackets)else}
begin a:=bytemem[curloc];incr(curloc);end;
{if curloc>=curlimit then confusion(strpackets)else}
begin b:=bytemem[curloc];incr(curloc);end;
{if curloc>=curlimit then confusion(strpackets)else}
begin c:=bytemem[curloc];incr(curloc);end;
if a<128 then pcktstrio:=(a*toint(256)+b)*toint(256)+c else pcktstrio:=(
(a-toint(256))*toint(256)+b)*toint(256)+c;end;function pcktutrio:int24u;
var a,b,c:eightbits;
begin{if curloc>=curlimit then confusion(strpackets)else}
begin a:=bytemem[curloc];incr(curloc);end;
{if curloc>=curlimit then confusion(strpackets)else}
begin b:=bytemem[curloc];incr(curloc);end;
{if curloc>=curlimit then confusion(strpackets)else}
begin c:=bytemem[curloc];incr(curloc);end;
pcktutrio:=(a*toint(256)+b)*toint(256)+c;end;{:52}{53:}
function pcktsquad:integer;var a,b,c,d:eightbits;
begin{if curloc>=curlimit then confusion(strpackets)else}
begin a:=bytemem[curloc];incr(curloc);end;
{if curloc>=curlimit then confusion(strpackets)else}
begin b:=bytemem[curloc];incr(curloc);end;
{if curloc>=curlimit then confusion(strpackets)else}
begin c:=bytemem[curloc];incr(curloc);end;
{if curloc>=curlimit then confusion(strpackets)else}
begin d:=bytemem[curloc];incr(curloc);end;
if a<128 then pcktsquad:=((a*toint(256)+b)*toint(256)+c)*toint(256)+d
else pcktsquad:=(((a-toint(256))*toint(256)+b)*toint(256)+c)*toint(256)+
d;end;{:53}{54:}{procedure pcktone(x:integer);begin;
if maxbytes-byteptr<1 then overflow(strbytes,maxbytes);
if x<0 then x:=x+256;begin bytemem[byteptr]:=x;incr(byteptr);end;end;}
{:54}{55:}{procedure pckttwo(x:integer);begin;
if maxbytes-byteptr<2 then overflow(strbytes,maxbytes);
if x<0 then x:=x+65536;begin bytemem[byteptr]:=x div 256;incr(byteptr);
end;begin bytemem[byteptr]:=x mod 256;incr(byteptr);end;end;}{:55}{56:}
procedure pcktfour(x:integer);begin;
if maxbytes-byteptr<4 then overflow(strbytes,maxbytes);
if x>=0 then begin bytemem[byteptr]:=x div 16777216;incr(byteptr);
end else begin x:=x+1073741824;x:=x+1073741824;
begin bytemem[byteptr]:=(x div 16777216)+128;incr(byteptr);end;end;
x:=x mod 16777216;begin bytemem[byteptr]:=x div 65536;incr(byteptr);end;
x:=x mod 65536;begin bytemem[byteptr]:=x div 256;incr(byteptr);end;
begin bytemem[byteptr]:=x mod 256;incr(byteptr);end;end;{:56}{57:}
procedure pcktchar(upd:boolean;ext:integer;res:eightbits);
var o:eightbits;begin;
if maxbytes-byteptr<5 then overflow(strbytes,maxbytes);
if(not upd)or(res>127)or(ext<>0)then begin o:=dvicharcmd[upd];
if ext<0 then ext:=ext+16777216;if ext=0 then begin bytemem[byteptr]:=o;
incr(byteptr);
end else begin if ext<256 then begin bytemem[byteptr]:=o+1;
incr(byteptr);
end else begin if ext<65536 then begin bytemem[byteptr]:=o+2;
incr(byteptr);end else begin begin bytemem[byteptr]:=o+3;incr(byteptr);
end;begin bytemem[byteptr]:=ext div 65536;incr(byteptr);end;
ext:=ext mod 65536;end;begin bytemem[byteptr]:=ext div 256;
incr(byteptr);end;ext:=ext mod 256;end;begin bytemem[byteptr]:=ext;
incr(byteptr);end;end;end;begin bytemem[byteptr]:=res;incr(byteptr);end;
end;{:57}{58:}procedure pcktunsigned(o:eightbits;x:integer);begin;
if maxbytes-byteptr<5 then overflow(strbytes,maxbytes);
if(x<256)and(x>=0)then if(o=235)and(x<64)then x:=x+171 else begin
bytemem[byteptr]:=o;incr(byteptr);
end else begin if(x<65536)and(x>=0)then begin bytemem[byteptr]:=o+1;
incr(byteptr);
end else begin if(x<16777216)and(x>=0)then begin bytemem[byteptr]:=o+2;
incr(byteptr);end else begin begin bytemem[byteptr]:=o+3;incr(byteptr);
end;if x>=0 then begin bytemem[byteptr]:=x div 16777216;incr(byteptr);
end else begin x:=x+1073741824;x:=x+1073741824;
begin bytemem[byteptr]:=(x div 16777216)+128;incr(byteptr);end;end;
x:=x mod 16777216;end;begin bytemem[byteptr]:=x div 65536;incr(byteptr);
end;x:=x mod 65536;end;begin bytemem[byteptr]:=x div 256;incr(byteptr);
end;x:=x mod 256;end;begin bytemem[byteptr]:=x;incr(byteptr);end;end;
{:58}{59:}procedure pcktsigned(o:eightbits;x:integer);var xx:int31;
begin;if maxbytes-byteptr<5 then overflow(strbytes,maxbytes);
if x>=0 then xx:=x else xx:=-(x+1);
if xx<128 then begin begin bytemem[byteptr]:=o;incr(byteptr);end;
if x<0 then x:=x+256;
end else begin if xx<32768 then begin begin bytemem[byteptr]:=o+1;
incr(byteptr);end;if x<0 then x:=x+65536;
end else begin if xx<8388608 then begin begin bytemem[byteptr]:=o+2;
incr(byteptr);end;if x<0 then x:=x+16777216;
end else begin begin bytemem[byteptr]:=o+3;incr(byteptr);end;
if x>=0 then begin bytemem[byteptr]:=x div 16777216;incr(byteptr);
end else begin x:=2147483647-xx;
begin bytemem[byteptr]:=(x div 16777216)+128;incr(byteptr);end;end;
x:=x mod 16777216;end;begin bytemem[byteptr]:=x div 65536;incr(byteptr);
end;x:=x mod 65536;end;begin bytemem[byteptr]:=x div 256;incr(byteptr);
end;x:=x mod 256;end;begin bytemem[byteptr]:=x;incr(byteptr);end;end;
{:59}{66:}{procedure makeres;var r:int16u;begin nresdigits:=0;r:=fres;
repeat incr(nresdigits);resdigits[nresdigits]:=xchr[48+(r mod 10)];
r:=r div 10;until r=0;end;}{:66}{67:}procedure makename(e:pcktpointer);
var b:eightbits;n:pcktpointer;curloc,curlimit:bytepointer;{ll:int15;}
c:char;begin n:=fntname[curfnt];curloc:=pcktstart[n];
curlimit:=pcktstart[n+1];
{if curloc>=curlimit then confusion(strpackets)else}
begin b:=bytemem[curloc];incr(curloc);end;if b>0 then curnamelength:=0;
while curloc<curlimit do begin{if curloc>=curlimit then confusion(
strpackets)else}begin b:=bytemem[curloc];incr(curloc);end;
if curnamelength<PATHMAX then begin incr(curnamelength);
curname[curnamelength]:=xchr[b];
end else overflow(strnamelength,PATHMAX);end;curloc:=pcktstart[e];
curlimit:=pcktstart[e+1];
while curloc<curlimit do begin{if curloc>=curlimit then confusion(
strpackets)else}begin b:=bytemem[curloc];incr(curloc);end;
begin c:=xchr[b];
{if c='?'then for ll:=nresdigits downto 1 do if curnamelength<PATHMAX
then begin incr(curnamelength);curname[curnamelength]:=resdigits[ll];
end else overflow(strnamelength,PATHMAX)else}
if curnamelength<PATHMAX then begin incr(curnamelength);
curname[curnamelength]:=c;end else overflow(strnamelength,PATHMAX);end;
end;while curnamelength<PATHMAX do begin incr(curnamelength);
curname[curnamelength]:=' ';end;end;{:67}{74:}
function makewidth(w:integer):widthpointer;label 31;var h:hashcode;
p:widthpointer;x:int16;begin widths[nwidths]:=w;{75:}
if w>=0 then x:=w div 16777216 else begin w:=w+1073741824;
w:=w+1073741824;x:=(w div 16777216)+128;end;w:=w mod 16777216;
x:=x+x+(w div 65536);w:=w mod 65536;x:=x+x+(w div 256);
h:=(x+x+(w mod 256))mod 353{:75};{76:}p:=whash[h];
while p<>0 do begin if widths[p]=widths[nwidths]then goto 31;
p:=wlink[p];end;p:=nwidths;wlink[p]:=whash[h];whash[h]:=p{:76};
if nwidths=maxwidths then overflow(strwidths,maxwidths);incr(nwidths);
{pixwidths[p]:=round(hconv*(w));}31:makewidth:=p;end;{:74}{87:}
function findpacket:boolean;label 31,10;var p,q:pcktpointer;f:eightbits;
e:int24;begin{88:}q:=charpackets[fntchars[curfnt]+curres];
while q<>maxpackets do begin p:=q;q:=maxpackets;curloc:=pcktstart[p];
curlimit:=pcktstart[p+1];if p=0 then begin e:=0;f:=0;
end else begin{if curloc>=curlimit then confusion(strpackets)else}
begin f:=bytemem[curloc];incr(curloc);end;case(f div 64)of 0:e:=0;
1:e:=pcktubyte;2:e:=pcktupair;3:e:=pcktstrio;end;
if(f mod 64)>=32 then q:=pcktupair;f:=f mod 32;end;
if e=curext then goto 31;end{:88};
if charpackets[fntchars[curfnt]+curres]=maxpackets then begin if
pcktmmsg<10 then begin writeln(stdout,
'---missing character packet for character ',curres:1,' font ',curfnt:1)
;incr(pcktmmsg);history:=2;
if pcktmmsg=10 then writeln(stdout,'---further messages suppressed.');
end;findpacket:=false;goto 10;end;
if pcktsmsg<10 then begin writeln(stdout,
'---substituted character packet with extension ',e:1,' instead of ',
curext:1,' for character ',curres:1,' font ',curfnt:1);incr(pcktsmsg);
history:=2;
if pcktsmsg=10 then writeln(stdout,'---further messages suppressed.');
end;curext:=e;31:curpckt:=p;curtype:=f;findpacket:=true;10:end;{:87}
{89:}procedure startpacket(t:typeflag);label 31,32;var p,q:pcktpointer;
f:int8u;e:integer;curloc:bytepointer;curlimit:bytepointer;begin{88:}
q:=charpackets[fntchars[curfnt]+curres];
while q<>maxpackets do begin p:=q;q:=maxpackets;curloc:=pcktstart[p];
curlimit:=pcktstart[p+1];if p=0 then begin e:=0;f:=0;
end else begin{if curloc>=curlimit then confusion(strpackets)else}
begin f:=bytemem[curloc];incr(curloc);end;case(f div 64)of 0:e:=0;
1:e:=pcktubyte;2:e:=pcktupair;3:e:=pcktstrio;end;
if(f mod 64)>=32 then q:=pcktupair;f:=f mod 32;end;
if e=curext then goto 31;end{:88};
q:=charpackets[fntchars[curfnt]+curres];pcktdup:=false;goto 32;
31:pcktdup:=true;pcktprev:=p;32:pcktext:=curext;pcktres:=curres;
if maxbytes-byteptr<6 then overflow(strbytes,maxbytes);
{if byteptr<>pcktstart[pcktptr]then confusion(strpackets);}
if q=maxpackets then f:=t else f:=t+32;e:=curext;
if e<0 then e:=e+16777216;if e=0 then begin bytemem[byteptr]:=f;
incr(byteptr);end else begin if e<256 then begin bytemem[byteptr]:=f+64;
incr(byteptr);
end else begin if e<65536 then begin bytemem[byteptr]:=f+128;
incr(byteptr);end else begin begin bytemem[byteptr]:=f+192;
incr(byteptr);end;begin bytemem[byteptr]:=e div 65536;incr(byteptr);end;
e:=e mod 65536;end;begin bytemem[byteptr]:=e div 256;incr(byteptr);end;
e:=e mod 256;end;begin bytemem[byteptr]:=e;incr(byteptr);end;end;
if q<>maxpackets then begin begin bytemem[byteptr]:=q div 256;
incr(byteptr);end;begin bytemem[byteptr]:=q mod 256;incr(byteptr);end;
end;end;{:89}{90:}procedure buildpacket;var k,l:bytepointer;
begin if pcktdup then begin k:=pcktstart[pcktprev+1];
l:=pcktstart[pcktptr];
if(byteptr-l)<>(k-pcktstart[pcktprev])then pcktdup:=false;
while pcktdup and(byteptr>l)do begin decr(byteptr);decr(k);
if bytemem[byteptr]<>bytemem[k]then pcktdup:=false;end;
if(not pcktdup)and(pcktdmsg<10)then begin write(stdout,
'---duplicate packet for character ',pcktres:1);
if pcktext<>0 then write(stdout,'.',pcktext:1);
writeln(stdout,' font ',curfnt:1);incr(pcktdmsg);history:=2;
if pcktdmsg=10 then writeln(stdout,'---further messages suppressed.');
end;byteptr:=l;
end else charpackets[fntchars[curfnt]+pcktres]:=makepacket;end;{:90}
{98:}procedure readtfmword;begin read(tfmfile,tfmb0);
read(tfmfile,tfmb1);read(tfmfile,tfmb2);read(tfmfile,tfmb3);
if eof(tfmfile)then badfont;end;{:98}{99:}
procedure checkchecksum(c:integer;u:boolean);
begin if(c<>fntcheck[curfnt])and(c<>0)then begin if fntcheck[curfnt]<>0
then begin writeln(stdout);
writeln(stdout,'---beware: check sums do not agree!   (',c:1,' vs. ',
fntcheck[curfnt]:1,')');history:=2;end;if u then fntcheck[curfnt]:=c;
end;end;procedure checkdesignsize(d:integer);
begin if abs(d-fntdesign[curfnt])>2 then begin writeln(stdout);
writeln(stdout,'---beware: design sizes do not agree!   (',d:1,' vs. ',
fntdesign[curfnt]:1,')');history:=2;end;end;
function checkwidth(w:integer):widthpointer;var wp:widthpointer;
begin if(curres>=fntbc[curfnt])and(curres<=fntec[curfnt])then wp:=
charwidths[fntchars[curfnt]+curres]else wp:=0;
if wp=0 then begin begin writeln(stdout);
write(stdout,'Bad char ',curres:1);end;
if curext<>0 then write(stdout,'.',curext:1);
write(stdout,' font ',curfnt:1);printfont(curfnt);
begin writeln(stdout,' ',' (compare TFM file)','.');jumpout;end;end;
if w<>widths[wp]then begin writeln(stdout);
writeln(stdout,'---beware: char widths do not agree!   (',w:1,' vs. ',
widths[wp]:1,')');history:=2;end;checkwidth:=wp;end;{:99}{100:}
function makefont:fontnumber;var l:int16;p:charpointer;q:widthpointer;
bc,ec:int15;lh:int15;nw:int15;w:integer;savefnt:fontnumber;{105:}
z:integer;alpha:integer;beta:int15;{:105}begin savefnt:=curfnt;
curfnt:=0;
while(fntname[curfnt]<>fntname[nf])or(fntscaled[curfnt]<>fntscaled[nf])
do incr(curfnt);{write(stdout,' => ',curfnt:1)};printfont(curfnt);
if curfnt<nf then begin checkchecksum(fntcheck[nf],true);
checkdesignsize(fntdesign[nf]);{write(stdout,' loaded previously')};
end else{101:}begin if nf=maxfonts then overflow(strfonts,maxfonts);
fnttype[curfnt]:=0;fntfont[curfnt]:=maxfonts;{96:}curnamelength:=0;
makename(tfmext);
if testreadaccess(curname,TFMFILEPATH)then begin reset(tfmfile,curname);
end else begin printpascalstring(curname);
begin writeln(stdout,' ','---not loaded, TFM file can''t be opened!','.'
);jumpout;end;end;{:96};{103:}readtfmword;
if tfmb2>127 then badfont else lh:=tfmb2*toint(256)+tfmb3;readtfmword;
if tfmb0>127 then badfont else bc:=tfmb0*toint(256)+tfmb1;
if tfmb2>127 then badfont else ec:=tfmb2*toint(256)+tfmb3;
if ec<bc then begin bc:=1;ec:=0;end else if ec>255 then badfont;
readtfmword;if tfmb0>127 then badfont else nw:=tfmb0*toint(256)+tfmb1;
if(nw=0)or(nw>256)then badfont;for l:=-2 to lh do begin readtfmword;
if l=1 then begin if tfmb0<128 then w:=((tfmb0*toint(256)+tfmb1)*toint(
256)+tfmb2)*toint(256)+tfmb3 else w:=(((tfmb0-toint(256))*toint(256)+
tfmb1)*toint(256)+tfmb2)*toint(256)+tfmb3;checkchecksum(w,true);
end else if l=2 then begin if tfmb0>127 then badfont;
checkdesignsize(round(tfmconv*(((tfmb0*toint(256)+tfmb1)*toint(256)+
tfmb2)*toint(256)+tfmb3)));end;end{:103};{104:}readtfmword;
while(tfmb0=0)and(bc<=ec)do begin incr(bc);readtfmword;end;
fntbc[curfnt]:=bc;fntchars[curfnt]:=nchars-bc;
if ec>=maxchars-fntchars[curfnt]then overflow(strchars,maxchars);
for l:=bc to ec do begin charwidths[nchars]:=tfmb0;incr(nchars);
readtfmword;end;
while(charwidths[nchars-1]=0)and(ec>=bc)do begin decr(nchars);decr(ec);
end;fntec[curfnt]:=ec{:104};{107:}
if nw-1>maxchars-nchars then overflow(strchars,maxchars);
if(tfmb0<>0)or(tfmb1<>0)or(tfmb2<>0)or(tfmb3<>0)then badfont else
charwidths[nchars]:=0;z:=fntscaled[curfnt];{fntspace[curfnt]:=z div 6;}
{106:}alpha:=16;while z>=8388608 do begin z:=z div 2;alpha:=alpha+alpha;
end;beta:=256 div alpha;alpha:=alpha*z{:106};
for p:=nchars+1 to nchars+nw-1 do begin readtfmword;
w:=(((((tfmb3*z)div 256)+(tfmb2*z))div 256)+(tfmb1*z))div beta;
if tfmb0>0 then if tfmb0=255 then w:=w-alpha else badfont;
charwidths[p]:=makewidth(w);end{:107};{108:}
for p:=fntchars[curfnt]+bc to nchars-1 do begin q:=charwidths[nchars+
charwidths[p]];charwidths[p]:=q;{charpixels[p]:=pixwidths[q];}
charpackets[p]:=maxpackets;end{:108};;
{write(stdout,' loaded at ',fntscaled[curfnt]:1,' DVI units')};incr(nf);
end{:101};writeln(stdout);makefont:=curfnt;curfnt:=savefnt;end;{:100}
{113:}function dvilength:integer;begin checkedfseek(dvifile,0,2);
dviloc:=ftell(dvifile);dvilength:=dviloc;end;
procedure dvimove(n:integer);begin checkedfseek(dvifile,n,0);dviloc:=n;
end;{:113}{114:}function dvisbyte:int8;var a:eightbits;
begin if eof(dvifile)then baddvi else read(dvifile,a);incr(dviloc);
if a<128 then dvisbyte:=a else dvisbyte:=a-256;end;
function dviubyte:int8u;var a:eightbits;
begin if eof(dvifile)then baddvi else read(dvifile,a);incr(dviloc);
dviubyte:=a;end;function dvispair:int16;var a,b:eightbits;
begin if eof(dvifile)then baddvi else read(dvifile,a);
if eof(dvifile)then baddvi else read(dvifile,b);dviloc:=dviloc+2;
if a<128 then dvispair:=a*toint(256)+b else dvispair:=(a-toint(256))*
toint(256)+b;end;function dviupair:int16u;var a,b:eightbits;
begin if eof(dvifile)then baddvi else read(dvifile,a);
if eof(dvifile)then baddvi else read(dvifile,b);dviloc:=dviloc+2;
dviupair:=a*toint(256)+b;end;function dvistrio:int24;
var a,b,c:eightbits;
begin if eof(dvifile)then baddvi else read(dvifile,a);
if eof(dvifile)then baddvi else read(dvifile,b);
if eof(dvifile)then baddvi else read(dvifile,c);dviloc:=dviloc+3;
if a<128 then dvistrio:=(a*toint(256)+b)*toint(256)+c else dvistrio:=((a
-toint(256))*toint(256)+b)*toint(256)+c;end;function dviutrio:int24u;
var a,b,c:eightbits;
begin if eof(dvifile)then baddvi else read(dvifile,a);
if eof(dvifile)then baddvi else read(dvifile,b);
if eof(dvifile)then baddvi else read(dvifile,c);dviloc:=dviloc+3;
dviutrio:=(a*toint(256)+b)*toint(256)+c;end;function dvisquad:integer;
var a,b,c,d:eightbits;
begin if eof(dvifile)then baddvi else read(dvifile,a);
if eof(dvifile)then baddvi else read(dvifile,b);
if eof(dvifile)then baddvi else read(dvifile,c);
if eof(dvifile)then baddvi else read(dvifile,d);dviloc:=dviloc+4;
if a<128 then dvisquad:=((a*toint(256)+b)*toint(256)+c)*toint(256)+d
else dvisquad:=(((a-toint(256))*toint(256)+b)*toint(256)+c)*toint(256)+d
;end;{:114}{115:}function dviuquad:int31;var x:integer;
begin x:=dvisquad;if x<0 then baddvi else dviuquad:=x;end;
function dvipquad:int31;var x:integer;begin x:=dvisquad;
if x<=0 then baddvi else dvipquad:=x;end;function dvipointer:integer;
var x:integer;begin x:=dvisquad;
if(x<=0)and(x<>-1)then baddvi else dvipointer:=x;end;{:115}{127:}
procedure dvifirstpar;begin repeat curcmd:=dviubyte;until curcmd<>138;
case dvipar[curcmd]of 0:begin curext:=0;
if curcmd<128 then begin curres:=curcmd;
curupd:=true end else begin curres:=dviubyte;curupd:=(curcmd<133);
curcmd:=curcmd-dvicharcmd[curupd];
while curcmd>0 do begin if curcmd=3 then if curres>127 then curext:=-1;
curext:=curext*256+curres;curres:=dviubyte;decr(curcmd);end;end;end;1:;
2:curparm:=dvisbyte;3:curparm:=dviubyte;4:curparm:=dvispair;
5:curparm:=dviupair;6:curparm:=dvistrio;7:curparm:=dviutrio;
8,9:curparm:=dvisquad;10:curparm:=dviuquad;11:begin curvdimen:=dvisquad;
curhdimen:=dvisquad;curupd:=(curcmd=132);end;12:curparm:=curcmd-171;end;
curclass:=dvicl[curcmd];end;{:127}{130:}procedure dvifont;
var f:fontnumber;begin{131:}f:=0;dviefnts[dvinf]:=curparm;
while curparm<>dviefnts[f]do incr(f){:131};if f=dvinf then baddvi;
curfnt:=dviifnts[f];end;{:130}{132:}procedure dvidofont(second:boolean);
var f:fontnumber;k:int15;begin write(stdout,'DVI: font ',curparm:1);
{131:}f:=0;dviefnts[dvinf]:=curparm;
while curparm<>dviefnts[f]do incr(f){:131};
if(f=dvinf)=second then baddvi;fntcheck[nf]:=dvisquad;
fntscaled[nf]:=dvipquad;fntdesign[nf]:=dvipquad;k:=dviubyte;
if maxbytes-byteptr<1 then overflow(strbytes,maxbytes);
begin bytemem[byteptr]:=k;incr(byteptr);end;k:=k+dviubyte;
if maxbytes-byteptr<k then overflow(strbytes,maxbytes);
while k>0 do begin begin bytemem[byteptr]:=dviubyte;incr(byteptr);end;
decr(k);end;fntname[nf]:=makepacket;dviifnts[dvinf]:=makefont;
if not second then begin if dvinf=maxfonts then overflow(strfonts,
maxfonts);incr(dvinf);
end else if dviifnts[f]<>dviifnts[dvinf]then baddvi;end;{:132}{141:}
function vfubyte:int8u;var a:eightbits;
begin if eof(vffile)then badfont else read(vffile,a);incr(vfloc);
vfubyte:=a;end;function vfupair:int16u;var a,b:eightbits;
begin if eof(vffile)then badfont else read(vffile,a);
if eof(vffile)then badfont else read(vffile,b);vfloc:=vfloc+2;
vfupair:=a*toint(256)+b;end;function vfstrio:int24;var a,b,c:eightbits;
begin if eof(vffile)then badfont else read(vffile,a);
if eof(vffile)then badfont else read(vffile,b);
if eof(vffile)then badfont else read(vffile,c);vfloc:=vfloc+3;
if a<128 then vfstrio:=(a*toint(256)+b)*toint(256)+c else vfstrio:=((a-
toint(256))*toint(256)+b)*toint(256)+c;end;function vfutrio:int24u;
var a,b,c:eightbits;
begin if eof(vffile)then badfont else read(vffile,a);
if eof(vffile)then badfont else read(vffile,b);
if eof(vffile)then badfont else read(vffile,c);vfloc:=vfloc+3;
vfutrio:=(a*toint(256)+b)*toint(256)+c;end;function vfsquad:integer;
var a,b,c,d:eightbits;
begin if eof(vffile)then badfont else read(vffile,a);
if eof(vffile)then badfont else read(vffile,b);
if eof(vffile)then badfont else read(vffile,c);
if eof(vffile)then badfont else read(vffile,d);vfloc:=vfloc+4;
if a<128 then vfsquad:=((a*toint(256)+b)*toint(256)+c)*toint(256)+d else
vfsquad:=(((a-toint(256))*toint(256)+b)*toint(256)+c)*toint(256)+d;end;
{:141}{143:}function vffix1:integer;var x:integer;
begin if eof(vffile)then badfont else read(vffile,tfmb3);incr(vfloc);
if tfmb3>127 then tfmb1:=255 else tfmb1:=0;tfmb2:=tfmb1;
x:=(((((tfmb3*z)div 256)+(tfmb2*z))div 256)+(tfmb1*z))div beta;
if tfmb1>127 then x:=x-alpha;vffix1:=x;end;function vffix2:integer;
var x:integer;begin if eof(vffile)then badfont else read(vffile,tfmb2);
if eof(vffile)then badfont else read(vffile,tfmb3);vfloc:=vfloc+2;
if tfmb2>127 then tfmb1:=255 else tfmb1:=0;
x:=(((((tfmb3*z)div 256)+(tfmb2*z))div 256)+(tfmb1*z))div beta;
if tfmb1>127 then x:=x-alpha;vffix2:=x;end;function vffix3:integer;
var x:integer;begin if eof(vffile)then badfont else read(vffile,tfmb1);
if eof(vffile)then badfont else read(vffile,tfmb2);
if eof(vffile)then badfont else read(vffile,tfmb3);vfloc:=vfloc+3;
x:=(((((tfmb3*z)div 256)+(tfmb2*z))div 256)+(tfmb1*z))div beta;
if tfmb1>127 then x:=x-alpha;vffix3:=x;end;function vffix3u:integer;
begin if eof(vffile)then badfont else read(vffile,tfmb1);
if eof(vffile)then badfont else read(vffile,tfmb2);
if eof(vffile)then badfont else read(vffile,tfmb3);vfloc:=vfloc+3;
vffix3u:=(((((tfmb3*z)div 256)+(tfmb2*z))div 256)+(tfmb1*z))div beta;
end;function vffix4:integer;var x:integer;
begin if eof(vffile)then badfont else read(vffile,tfmb0);
if eof(vffile)then badfont else read(vffile,tfmb1);
if eof(vffile)then badfont else read(vffile,tfmb2);
if eof(vffile)then badfont else read(vffile,tfmb3);vfloc:=vfloc+4;
x:=(((((tfmb3*z)div 256)+(tfmb2*z))div 256)+(tfmb1*z))div beta;
if tfmb0>0 then if tfmb0=255 then x:=x-alpha else badfont;vffix4:=x;end;
{:143}{144:}function vfuquad:int31;var x:integer;begin x:=vfsquad;
if x<0 then badfont else vfuquad:=x;end;function vfpquad:int31;
var x:integer;begin x:=vfsquad;if x<=0 then badfont else vfpquad:=x;end;
function vffixp:int31;var x:integer;
begin if eof(vffile)then badfont else read(vffile,tfmb0);
if eof(vffile)then badfont else read(vffile,tfmb1);
if eof(vffile)then badfont else read(vffile,tfmb2);
if eof(vffile)then badfont else read(vffile,tfmb3);vfloc:=vfloc+4;
if tfmb0>0 then badfont;
vffixp:=(((((tfmb3*z)div 256)+(tfmb2*z))div 256)+(tfmb1*z))div beta;end;
{:144}{145:}procedure vffirstpar;begin curcmd:=vfubyte;
case dvipar[curcmd]of 0:begin begin curext:=0;
if curcmd<128 then begin curres:=curcmd;
curupd:=true end else begin curres:=vfubyte;curupd:=(curcmd<133);
curcmd:=curcmd-dvicharcmd[curupd];
while curcmd>0 do begin if curcmd=3 then if curres>127 then curext:=-1;
curext:=curext*256+curres;curres:=vfubyte;decr(curcmd);end;end;end;
curwp:=0;
if vfcurfnt<>maxfonts then if(curres>=fntbc[vfcurfnt])and(curres<=fntec[
vfcurfnt])then curwp:=charwidths[fntchars[vfcurfnt]+curres];
if curwp=0 then badfont;end;1:;2:curparm:=vffix1;3:curparm:=vfubyte;
4:curparm:=vffix2;5:curparm:=vfupair;6:curparm:=vffix3;
7:curparm:=vfutrio;8:curparm:=vffix4;9:curparm:=vfsquad;
10:curparm:=vfuquad;11:begin curvdimen:=vffix4;curhdimen:=vffix4;
curupd:=(curcmd=132);end;12:curparm:=curcmd-171;end;
curclass:=dvicl[curcmd];end;{:145}{148:}procedure vffont;
var f:fontnumber;begin{149:}f:=0;vfefnts[vfnf]:=curparm;
while curparm<>vfefnts[f]do incr(f){:149};if f=vfnf then badfont;
vfcurfnt:=vfifnts[f];end;{:148}{150:}procedure vfdofont;
var f:fontnumber;k:int15;begin write(stdout,'VF: font ',curparm:1);
{149:}f:=0;vfefnts[vfnf]:=curparm;
while curparm<>vfefnts[f]do incr(f){:149};if f<>vfnf then badfont;
fntcheck[nf]:=vfsquad;fntscaled[nf]:=vffixp;
fntdesign[nf]:=round(tfmconv*vfpquad);k:=vfubyte;
if maxbytes-byteptr<1 then overflow(strbytes,maxbytes);
begin bytemem[byteptr]:=k;incr(byteptr);end;k:=k+vfubyte;
if maxbytes-byteptr<k then overflow(strbytes,maxbytes);
while k>0 do begin begin bytemem[byteptr]:=vfubyte;incr(byteptr);end;
decr(k);end;fntname[nf]:=makepacket;vfifnts[vfnf]:=makefont;
if vfnf=lclnf then if lclnf=maxfonts then overflow(strfonts,maxfonts)
else incr(lclnf);incr(vfnf);end;{:150}{151:}function dovf:boolean;
label 21,30,32,10;var tempint:integer;tempbyte:int8u;k:bytepointer;
l:int15;saveext:int24;saveres:int8u;savewp:widthpointer;saveupd:boolean;
vfwp:widthpointer;vffnt:fontnumber;movezero:boolean;lastpop:boolean;
begin{139:}curnamelength:=0;makename(vfext);
if testreadaccess(curname,VFFILEPATH)then begin reset(vffile,curname);
end else goto 32;vfloc:=0{:139};saveext:=curext;saveres:=curres;
savewp:=curwp;saveupd:=curupd;fnttype[curfnt]:=1;{152:}
if vfubyte<>247 then badfont;if vfubyte<>202 then badfont;
tempbyte:=vfubyte;
if maxbytes-byteptr<tempbyte then overflow(strbytes,maxbytes);
for l:=1 to tempbyte do begin bytemem[byteptr]:=vfubyte;incr(byteptr);
end;write(stdout,'VF file: ''');printpacket(newpacket);
write(stdout,''',');flushpacket;checkchecksum(vfsquad,false);
checkdesignsize(round(tfmconv*vfpquad));z:=fntscaled[curfnt];{106:}
alpha:=16;while z>=8388608 do begin z:=z div 2;alpha:=alpha+alpha;end;
beta:=256 div alpha;alpha:=alpha*z{:106};begin writeln(stdout);
write(stdout,'   for font ',curfnt:1);end;printfont(curfnt);
writeln(stdout,'.'){:152};{153:}vfifnts[0]:=maxfonts;vfnf:=0;
curcmd:=vfubyte;
while(curcmd>=243)and(curcmd<=246)do begin case curcmd-243 of 0:curparm
:=vfubyte;1:curparm:=vfupair;2:curparm:=vfutrio;3:curparm:=vfsquad;end;
vfdofont;curcmd:=vfubyte;end;fntfont[curfnt]:=vfifnts[0]{:153};
while curcmd<=242 do{160:}
begin if curcmd<242 then begin vflimit:=curcmd;curext:=0;
curres:=vfubyte;vfwp:=checkwidth(vffix3u);
end else begin vflimit:=vfuquad;curext:=vfstrio;curres:=vfubyte;
vfwp:=checkwidth(vffix4);end;vflimit:=vflimit+vfloc;
vfpushloc[0]:=byteptr;vflastend[0]:=byteptr;vflast[0]:=4;vfptr:=0;
startpacket(1);{161:}vfcurfnt:=fntfont[curfnt];vffnt:=vfcurfnt;
lastpop:=false;curclass:=3;
while true do begin 21:case curclass of 0,1,2:{164:}
begin if(vfptr=0)or(byteptr>vfpushloc[vfptr])then movezero:=false else
case curclass of 0:movezero:=(not curupd)or(vfcurfnt<>vffnt);
1:movezero:=not curupd;2:movezero:=true;end;
if movezero then begin decr(byteptr);decr(vfptr);end;
case curclass of 0:{165:}
begin if vfcurfnt<>vffnt then begin vflast[vfptr]:=4;
pcktunsigned(235,vfcurfnt);vffnt:=vfcurfnt;end;
if(not movezero)or(not curupd)then begin vflast[vfptr]:=vfchartype[
curupd];vflastloc[vfptr]:=byteptr;pcktchar(curupd,curext,curres);end;
end{:165};1:{166:}begin vflast[vfptr]:=vfruletype[curupd];
vflastloc[vfptr]:=byteptr;
begin if maxbytes-byteptr<1 then overflow(strbytes,maxbytes);
begin bytemem[byteptr]:=dvirulecmd[curupd];incr(byteptr);end;end;
pcktfour(curvdimen);pcktfour(curhdimen);end{:166};2:{167:}
begin vflast[vfptr]:=4;pcktunsigned(239,curparm);
if maxbytes-byteptr<curparm then overflow(strbytes,maxbytes);
while curparm>0 do begin begin bytemem[byteptr]:=vfubyte;incr(byteptr);
end;decr(curparm);end;end{:167};end;vflastend[vfptr]:=byteptr;
if movezero then begin incr(vfptr);
begin if maxbytes-byteptr<1 then overflow(strbytes,maxbytes);
begin bytemem[byteptr]:=141;incr(byteptr);end;end;
vfpushloc[vfptr]:=byteptr;vflastend[vfptr]:=byteptr;
if curclass=0 then if curupd then goto 21;end;end{:164};3:{162:}
if(vfptr>0)and(vfpushloc[vfptr]=byteptr)then begin if vfpushnum[vfptr]=
255 then overflow(strstack,255);incr(vfpushnum[vfptr]);
end else begin if vfptr=stackused then if stackused=stacksize then
overflow(strstack,stacksize)else incr(stackused);incr(vfptr);{163:}
begin if maxbytes-byteptr<1 then overflow(strbytes,maxbytes);
begin bytemem[byteptr]:=141;incr(byteptr);end;end;
begin vfmove[vfptr][0][0]:=vfmove[vfptr-1][0][0];
vfmove[vfptr][0][1]:=vfmove[vfptr-1][0][1];
vfmove[vfptr][1][0]:=vfmove[vfptr-1][1][0];
vfmove[vfptr][1][1]:=vfmove[vfptr-1][1][1]end;vfpushloc[vfptr]:=byteptr;
vflastend[vfptr]:=byteptr;vflast[vfptr]:=4{:163};vfpushnum[vfptr]:=0;
end{:162};4:{168:}begin if vfptr<1 then badfont;
byteptr:=vflastend[vfptr];
if vflast[vfptr]<=1 then if vflastloc[vfptr]=vfpushloc[vfptr]then{169:}
begin curclass:=vflast[vfptr];curupd:=false;byteptr:=vfpushloc[vfptr];
end{:169};if byteptr=vfpushloc[vfptr]then{170:}
begin if vfpushnum[vfptr]>0 then begin decr(vfpushnum[vfptr]);
begin vfmove[vfptr][0][0]:=vfmove[vfptr-1][0][0];
vfmove[vfptr][0][1]:=vfmove[vfptr-1][0][1];
vfmove[vfptr][1][0]:=vfmove[vfptr-1][1][0];
vfmove[vfptr][1][1]:=vfmove[vfptr-1][1][1]end;
end else begin decr(byteptr);decr(vfptr);end;
if curclass<>4 then goto 21;end{:170}
else begin if vflast[vfptr]=2 then{171:}begin byteptr:=byteptr-2;
for k:=vflastloc[vfptr]+1 to byteptr do bytemem[k-1]:=bytemem[k];
vflast[vfptr]:=4;vflastend[vfptr]:=byteptr;end{:171};
begin if maxbytes-byteptr<1 then overflow(strbytes,maxbytes);
begin bytemem[byteptr]:=142;incr(byteptr);end;end;decr(vfptr);
vflast[vfptr]:=2;vflastloc[vfptr]:=vfpushloc[vfptr+1]-1;
vflastend[vfptr]:=byteptr;if vfpushnum[vfptr+1]>0 then{172:}
begin incr(vfptr);{163:}
begin if maxbytes-byteptr<1 then overflow(strbytes,maxbytes);
begin bytemem[byteptr]:=141;incr(byteptr);end;end;
begin vfmove[vfptr][0][0]:=vfmove[vfptr-1][0][0];
vfmove[vfptr][0][1]:=vfmove[vfptr-1][0][1];
vfmove[vfptr][1][0]:=vfmove[vfptr-1][1][0];
vfmove[vfptr][1][1]:=vfmove[vfptr-1][1][1]end;vfpushloc[vfptr]:=byteptr;
vflastend[vfptr]:=byteptr;vflast[vfptr]:=4{:163};decr(vfpushnum[vfptr]);
end{:172};end;end{:168};
5,6:if vfmove[vfptr][0][curclass-5]then begin if maxbytes-byteptr<1 then
overflow(strbytes,maxbytes);begin bytemem[byteptr]:=curcmd;
incr(byteptr);end;end;
7,8,9:begin pcktsigned(dvirightcmd[curclass],curparm);
if curclass>=8 then vfmove[vfptr][0][curclass-8]:=true;end;
10,11:if vfmove[vfptr][1][curclass-10]then begin if maxbytes-byteptr<1
then overflow(strbytes,maxbytes);begin bytemem[byteptr]:=curcmd;
incr(byteptr);end;end;
12,13,14:begin pcktsigned(dvidowncmd[curclass],curparm);
if curclass>=13 then vfmove[vfptr][1][curclass-13]:=true;end;15:vffont;
16:badfont;17:if curcmd<>138 then badfont;end;
if vfloc<vflimit then vffirstpar else if lastpop then goto 30 else begin
curclass:=4;lastpop:=true;end;end;
30:if(vfptr<>0)or(vfloc<>vflimit)then badfont{:161};
k:=pcktstart[pcktptr];
if vflast[0]=3 then if curwp=vfwp then begin decr(bytemem[k]);
if(bytemem[k]=0)and(vfpushloc[0]=vflastloc[0])and(curext=0)and(curres=
pcktres)then byteptr:=k;end;buildpacket;curcmd:=vfubyte;end{:160};
if curcmd<>248 then badfont;{write(stdout,'VF file for font ',curfnt:1);
printfont(curfnt);writeln(stdout,' loaded.');};curext:=saveext;
curres:=saveres;curwp:=savewp;curupd:=saveupd;dovf:=true;goto 10;
32:dovf:=false;10:end;{:151}{179:}{175:}procedure inputln;
var k:0..terminallinelength;begin write(stdout,'Enter option: ');
flush(stdout);k:=0;
if maxbytes-byteptr<terminallinelength then overflow(strbytes,maxbytes);
while(k<terminallinelength)and not eoln(stdin)do begin begin bytemem[
byteptr]:=xord[getc(stdin)];incr(byteptr);end;incr(k);end;end;{:175}
{177:}function scankeyword(p:pcktpointer;l:int7):boolean;
var i,j,k:bytepointer;begin i:=pcktstart[p];j:=pcktstart[p+1];
k:=scanptr;
while(i<j)and((bytemem[k]=bytemem[i])or(bytemem[k]=bytemem[i]-32))do
begin incr(i);incr(k);end;
if(bytemem[k]=32)and(i-pcktstart[p]>=l)then begin scanptr:=k;
while(bytemem[scanptr]=32)and(scanptr<byteptr)do incr(scanptr);
scankeyword:=true;end else scankeyword:=false;end;{:177}{178:}
function scanint:integer;var x:integer;negative:boolean;
begin if bytemem[scanptr]=45 then begin negative:=true;incr(scanptr);
end else negative:=false;x:=0;
while(bytemem[scanptr]>=48)and(bytemem[scanptr]<=57)do begin x:=10*x+
bytemem[scanptr]-48;incr(scanptr);end;
while(bytemem[scanptr]=32)and(scanptr<byteptr)do incr(scanptr);
if negative then scanint:=-x else scanint:=x;end;{:178}{191:}
procedure scancount;
begin if bytemem[scanptr]=42 then begin selectthere[curselect][
selectvals[curselect]]:=false;incr(scanptr);
while(bytemem[scanptr]=32)and(scanptr<byteptr)do incr(scanptr);
end else begin selectthere[curselect][selectvals[curselect]]:=true;
selectcount[curselect][selectvals[curselect]]:=scanint;
if curselect=0 then selected:=false;end;end;{:191}procedure dialog;
label 10;var p:pcktpointer;begin{189:}outmag:=0;curselect:=0;
selectmax[curselect]:=0;selected:=true;{:189}
while true do begin inputln;p:=newpacket;bytemem[byteptr]:=32;
scanptr:=pcktstart[pcktptr-1];
while(bytemem[scanptr]=32)and(scanptr<byteptr)do incr(scanptr);
if scanptr=byteptr then begin flushpacket;goto 10;end{192:}
else if scankeyword(strmag,3)then outmag:=scanint else if scankeyword(
strselect,3)then if curselect=10 then writeln(stdout,
'Too many page selections')else begin selectvals[curselect]:=0;
scancount;
while(selectvals[curselect]<9)and(bytemem[scanptr]=46)do begin incr(
selectvals[curselect]);incr(scanptr);scancount;end;
selectmax[curselect]:=scanint;incr(curselect);end{:192}
else begin writeln(stdout,'Valid options are:');{190:}
writeln(stdout,'  mag <mag>');
writeln(stdout,'  select <first page> [<num pages>]');{:190}end;
if eoln(stdin)then readln(stdin);flushpacket;end;10:end;{:179}{180:}
{240:}procedure outpacket(p:pcktpointer);var k:bytepointer;
begin outloc:=outloc+(pcktstart[p+1]-pcktstart[p]);
for k:=pcktstart[p]to pcktstart[p+1]-1 do putbyte(bytemem[k],outfile);
end;{:240}{241:}procedure outfour(x:integer);begin;
if x>=0 then putbyte(x div 16777216,outfile)else begin x:=x+1073741824;
x:=x+1073741824;putbyte((x div 16777216)+128,outfile);end;
x:=x mod 16777216;putbyte(x div 65536,outfile);x:=x mod 65536;
putbyte(x div 256,outfile);putbyte(x mod 256,outfile);outloc:=outloc+4;
end;{:241}{242:}procedure outchar(upd:boolean;ext:integer;
res:eightbits);var o:eightbits;begin;
if(not upd)or(res>127)or(ext<>0)then begin o:=dvicharcmd[upd];
if ext<0 then ext:=ext+16777216;if ext=0 then begin putbyte(o,outfile);
incr(outloc);end else begin if ext<256 then begin putbyte(o+1,outfile);
incr(outloc);
end else begin if ext<65536 then begin putbyte(o+2,outfile);
incr(outloc);end else begin begin putbyte(o+3,outfile);incr(outloc);end;
begin putbyte(ext div 65536,outfile);incr(outloc);end;
ext:=ext mod 65536;end;begin putbyte(ext div 256,outfile);incr(outloc);
end;ext:=ext mod 256;end;begin putbyte(ext,outfile);incr(outloc);end;
end;end;begin putbyte(res,outfile);incr(outloc);end;end;{:242}{243:}
procedure outunsigned(o:eightbits;x:integer);begin;
if(x<256)and(x>=0)then if(o=235)and(x<64)then x:=x+171 else begin
putbyte(o,outfile);incr(outloc);
end else begin if(x<65536)and(x>=0)then begin putbyte(o+1,outfile);
incr(outloc);
end else begin if(x<16777216)and(x>=0)then begin putbyte(o+2,outfile);
incr(outloc);end else begin begin putbyte(o+3,outfile);incr(outloc);end;
if x>=0 then begin putbyte(x div 16777216,outfile);incr(outloc);
end else begin x:=x+1073741824;x:=x+1073741824;
begin putbyte((x div 16777216)+128,outfile);incr(outloc);end;end;
x:=x mod 16777216;end;begin putbyte(x div 65536,outfile);incr(outloc);
end;x:=x mod 65536;end;begin putbyte(x div 256,outfile);incr(outloc);
end;x:=x mod 256;end;begin putbyte(x,outfile);incr(outloc);end;end;
{:243}{244:}procedure outsigned(o:eightbits;x:integer);var xx:int31;
begin;if x>=0 then xx:=x else xx:=-(x+1);
if xx<128 then begin begin putbyte(o,outfile);incr(outloc);end;
if x<0 then x:=x+256;
end else begin if xx<32768 then begin begin putbyte(o+1,outfile);
incr(outloc);end;if x<0 then x:=x+65536;
end else begin if xx<8388608 then begin begin putbyte(o+2,outfile);
incr(outloc);end;if x<0 then x:=x+16777216;
end else begin begin putbyte(o+3,outfile);incr(outloc);end;
if x>=0 then begin putbyte(x div 16777216,outfile);incr(outloc);
end else begin x:=2147483647-xx;
begin putbyte((x div 16777216)+128,outfile);incr(outloc);end;end;
x:=x mod 16777216;end;begin putbyte(x div 65536,outfile);incr(outloc);
end;x:=x mod 65536;end;begin putbyte(x div 256,outfile);incr(outloc);
end;x:=x mod 256;end;begin putbyte(x,outfile);incr(outloc);end;end;
{:244}{248:}procedure outfntdef(f:fontnumber);var p:pcktpointer;
k,l:bytepointer;a:eightbits;begin outunsigned(243,fntfont[f]);
outfour(fntcheck[f]);outfour(fntscaled[f]);outfour(fntdesign[f]);
p:=fntname[f];k:=pcktstart[p];l:=pcktstart[p+1]-1;a:=bytemem[k];
outloc:=outloc+l-k+2;putbyte(a,outfile);putbyte(l-k-a,outfile);
while k<l do begin incr(k);putbyte(bytemem[k],outfile);end;end;{:248}
{:180}{188:}function startmatch:boolean;var k:0..9;match:boolean;
begin match:=true;
for k:=0 to selectvals[curselect]do if selectthere[curselect][k]and(
selectcount[curselect][k]<>count[k])then match:=false;startmatch:=match;
end;{:188}{194:}procedure dopre;{250:}var k:int15;p,q,r:bytepointer;
comment:ccharpointer;{:250}begin alldone:=false;numselect:=curselect;
curselect:=0;if numselect=0 then selectmax[curselect]:=0;{251:}
begin putbyte(247,outfile);incr(outloc);end;begin putbyte(2,outfile);
incr(outloc);end;outfour(dvinum);outfour(dviden);outfour(outmag);
p:=pcktstart[pcktptr-1];q:=byteptr;comment:=' DVIcopy 1.2 output from ';
if maxbytes-byteptr<24 then overflow(strbytes,maxbytes);
for k:=1 to 24 do begin bytemem[byteptr]:=xord[comment[k]];
incr(byteptr);end;while bytemem[p]=32 do incr(p);
if p=q then byteptr:=byteptr-6 else begin k:=0;
while(k<24)and(bytemem[p+k]=bytemem[q+k])do incr(k);
if k=24 then p:=p+24;end;k:=byteptr-p;if k>255 then begin k:=255;
q:=p+231;end;begin putbyte(k,outfile);incr(outloc);end;
outpacket(newpacket);flushpacket;
for r:=p to q-1 do begin putbyte(bytemem[r],outfile);incr(outloc);end;
{:251}{hconv:=(dvinum/254000.0)*(300/dviden)*(outmag/1000.0);
vconv:=(dvinum/254000.0)*(300/dviden)*(outmag/1000.0);}end;{:194}{195:}
procedure dobop;{252:}var{:252}i,j:0..9;begin{196:}
if not selected then selected:=startmatch;typesetting:=selected{:196};
write(stdout,'DVI: ');
if typesetting then write(stdout,'process')else write(stdout,'skipp');
write(stdout,'ing page ',count[0]:1);j:=9;
while(j>0)and(count[j]=0)do decr(j);
for i:=1 to j do write(stdout,'.',count[i]:1);
{write(stdout,' at ',dviloc-45:1)};writeln(stdout,'.');
if typesetting then begin stackptr:=0;curstack:=zerostack;
curfnt:=maxfonts;{253:}begin putbyte(139,outfile);incr(outloc);end;
incr(outpages);for i:=0 to 9 do outfour(count[i]);outfour(outback);
outback:=outloc-45;outfnt:=maxfonts;{:253}end;end;{:195}{197:}
procedure doeop;{254:}{:254}begin if stackptr<>0 then baddvi;{255:}
begin putbyte(140,outfile);incr(outloc);end;{:255}
if selectmax[curselect]>0 then begin decr(selectmax[curselect]);
if selectmax[curselect]=0 then begin selected:=false;incr(curselect);
if curselect=numselect then alldone:=true;end;end;typesetting:=false;
end;{:197}{198:}procedure dopush;{256:}{:256}
begin if stackptr=stackused then if stackused=stacksize then overflow(
strstack,stacksize)else incr(stackused);incr(stackptr);
stack[stackptr]:=curstack;{257:}
if stackptr>outstack then outstack:=stackptr;begin putbyte(141,outfile);
incr(outloc);end;{:257}end;procedure dopop;{258:}{:258}
begin if stackptr=0 then baddvi;{259:}begin putbyte(142,outfile);
incr(outloc);end;{:259}curstack:=stack[stackptr];decr(stackptr);end;
{:198}{199:}procedure doxxx;{260:}var{:260}p:pcktpointer;
begin p:=newpacket;{261:}outunsigned(239,(pcktstart[p+1]-pcktstart[p]));
outpacket(p);{:261}flushpacket;end;{:199}{200:}procedure doright;{262:}
{:262}
begin if curclass>=8 then curstack.wxfield[curclass-8]:=curparm else if
curclass<7 then curparm:=curstack.wxfield[curclass-5];{263:}
if curclass<7 then begin putbyte(curcmd,outfile);incr(outloc);
end else outsigned(dvirightcmd[curclass],curparm);{:263}
curstack.hfield:=curstack.hfield+curparm;
{if(curparm>=fntspace[curfnt])or(curparm<=-4*fntspace[curfnt])then
curstack.hhfield:=round(hconv*(curstack.hfield))else begin curstack.
hhfield:=curstack.hhfield+round(hconv*(curparm));
temppix:=round(hconv*(curstack.hfield));
if abs(temppix-curstack.hhfield)>2 then if temppix>curstack.hhfield then
curstack.hhfield:=temppix-2 else curstack.hhfield:=temppix+2;end;}{264:}
if abs(curstack.hfield)>outmaxh then outmaxh:=abs(curstack.hfield);
{:264}end;{:200}{201:}procedure dodown;{265:}{:265}
begin if curclass>=13 then curstack.yzfield[curclass-13]:=curparm else
if curclass<12 then curparm:=curstack.yzfield[curclass-10];{266:}
if curclass<12 then begin putbyte(curcmd,outfile);incr(outloc);
end else outsigned(dvidowncmd[curclass],curparm);{:266}
curstack.vfield:=curstack.vfield+curparm;
{if abs(curparm)>=5*fntspace[curfnt]then curstack.vvfield:=round(vconv*(
curstack.vfield))else begin curstack.vvfield:=curstack.vvfield+round(
vconv*(curparm));temppix:=round(vconv*(curstack.vfield));
if abs(temppix-curstack.vvfield)>2 then if temppix>curstack.vvfield then
curstack.vvfield:=temppix-2 else curstack.vvfield:=temppix+2;end;}{267:}
if abs(curstack.vfield)>outmaxv then outmaxv:=abs(curstack.vfield);
{:267}end;{:201}{202:}procedure dowidth;{268:}{:268}begin{269:}
begin putbyte(132,outfile);incr(outloc);end;outfour(widthdimen);
outfour(curhdimen);{:269}curstack.hfield:=curstack.hfield+curhdimen;
{begin curstack.hhfield:=curstack.hhfield+hpixels;
temppix:=round(hconv*(curstack.hfield));
if abs(temppix-curstack.hhfield)>2 then if temppix>curstack.hhfield then
curstack.hhfield:=temppix-2 else curstack.hhfield:=temppix+2;end;}{264:}
if abs(curstack.hfield)>outmaxh then outmaxh:=abs(curstack.hfield);
{:264}end;{:202}{203:}{function hrulepixels(x:integer):pixvalue;
var n:integer;begin n:=trunc(hconv*x);
if n<hconv*x then hrulepixels:=n+1 else hrulepixels:=n;end;
function vrulepixels(x:integer):pixvalue;var n:integer;
begin n:=trunc(vconv*x);
if n<vconv*x then vrulepixels:=n+1 else vrulepixels:=n;end;}{:203}{204:}
procedure dorule;{270:}var{:270}visible:boolean;
begin if(curhdimen>0)and(curvdimen>0)then begin visible:=true;
{hpixels:=hrulepixels(curhdimen);vpixels:=vrulepixels(curvdimen);}{271:}
begin putbyte(dvirulecmd[curupd],outfile);incr(outloc);end;
outfour(curvdimen);outfour(curhdimen);{:271}
end else begin visible:=false;{272:}{271:}
begin putbyte(dvirulecmd[curupd],outfile);incr(outloc);end;
outfour(curvdimen);outfour(curhdimen);{:271}{:272}end;
if curupd then begin curstack.hfield:=curstack.hfield+curhdimen;
{if not visible then hpixels:=hrulepixels(curhdimen);
begin curstack.hhfield:=curstack.hhfield+hpixels;
temppix:=round(hconv*(curstack.hfield));
if abs(temppix-curstack.hhfield)>2 then if temppix>curstack.hhfield then
curstack.hhfield:=temppix-2 else curstack.hhfield:=temppix+2;end;}{264:}
if abs(curstack.hfield)>outmaxh then outmaxh:=abs(curstack.hfield);
{:264}end;end;{:204}{205:}procedure dochar;{276:}{:276}begin{277:}
{if fnttype[curfnt]<>2 then confusion(strfonts);}
if curfnt<>outfnt then begin outunsigned(235,fntfont[curfnt]);
outfnt:=curfnt;end;outchar(curupd,curext,curres);{:277}
if curupd then begin curstack.hfield:=curstack.hfield+widths[curwp];
{begin curstack.hhfield:=curstack.hhfield+charpixels[fntchars[curfnt]+
curres];temppix:=round(hconv*(curstack.hfield));
if abs(temppix-curstack.hhfield)>2 then if temppix>curstack.hhfield then
curstack.hhfield:=temppix-2 else curstack.hhfield:=temppix+2;end;}{264:}
if abs(curstack.hfield)>outmaxh then outmaxh:=abs(curstack.hfield);
{:264}end;end;{:205}{207:}procedure dofont;label 30;{273:}{:273}
begin{274:}{:274}if dovf then goto 30;{275:}
if(outnf>=maxfonts)then overflow(strfonts,maxfonts);
write(stdout,'OUT: font ',curfnt:1);{write(stdout,' => ',outnf:1)};
printfont(curfnt);
{write(stdout,' at ',fntscaled[curfnt]:1,' DVI units')};
writeln(stdout,'.');fnttype[curfnt]:=2;fntfont[curfnt]:=outnf;
outfnts[outnf]:=curfnt;incr(outnf);outfntdef(curfnt);{:275}
30:{if fnttype[curfnt]=0 then confusion(strfonts);}end;{:207}{208:}
procedure pcktfirstpar;begin curcmd:=pcktubyte;
case dvipar[curcmd]of 0:begin curext:=0;
if curcmd<128 then begin curres:=curcmd;
curupd:=true end else begin curres:=pcktubyte;curupd:=(curcmd<133);
curcmd:=curcmd-dvicharcmd[curupd];
while curcmd>0 do begin if curcmd=3 then if curres>127 then curext:=-1;
curext:=curext*256+curres;curres:=pcktubyte;decr(curcmd);end;end;end;1:;
2:curparm:=pcktsbyte;3:curparm:=pcktubyte;4:curparm:=pcktspair;
5:curparm:=pcktupair;6:curparm:=pcktstrio;7:curparm:=pcktutrio;
8,9,10:curparm:=pcktsquad;11:begin curvdimen:=pcktsquad;
curhdimen:=pcktsquad;curupd:=(curcmd=132);end;12:curparm:=curcmd-171;
end;curclass:=dvicl[curcmd];end;{:208}{212:}procedure dovfpacket;
label 22,31,30;var k:recurpointer;f:int8u;saveupd:boolean;
savewp:widthpointer;savelimit:bytepointer;begin{213:}saveupd:=curupd;
savewp:=curwp;recurfnt[nrecur]:=curfnt;recurext[nrecur]:=curext;
recurres[nrecur]:=curres{:213};{215:}
if findpacket then f:=curtype else goto 30;recurpckt[nrecur]:=curpckt;
savelimit:=curlimit;curfnt:=fntfont[curfnt];
if curpckt=0 then begin curclass:=0;goto 31;end;
if curloc>=curlimit then goto 30;22:pcktfirstpar;
31:case curclass of 0:{216:}
begin curwp:=charwidths[fntchars[curfnt]+curres];
if fnttype[curfnt]=0 then dofont;
if(curloc=curlimit)and(f=0)and saveupd then begin saveupd:=false;
curupd:=true;end;if fnttype[curfnt]=1 then{217:}
begin recurloc[nrecur]:=curloc;
if curloc<curlimit then if bytemem[curloc]=142 then curupd:=false;
if nrecur=recurused then if recurused=maxrecursion then{218:}
begin writeln(stdout,' !Infinite VF recursion?');
for k:=maxrecursion downto 0 do begin write(stdout,'level=',k:1,' font')
;{write(stdout,'=',recurfnt[k]:1)};printfont(recurfnt[k]);
write(stdout,' char=',recurres[k]:1);
if recurext[k]<>0 then write(stdout,'.',recurext[k]:1);writeln(stdout);
{hexpacket(recurpckt[k]);writeln(stdout,'loc=',recurloc[k]:1);}end;
overflow(strrecursion,maxrecursion);end{:218}else incr(recurused);
incr(nrecur);dovfpacket;decr(nrecur);curloc:=recurloc[nrecur];
curlimit:=savelimit;end{:217}else dochar;end{:216};1:dorule;
2:begin if maxbytes-byteptr<curparm then overflow(strbytes,maxbytes);
while curparm>0 do begin begin bytemem[byteptr]:=pcktubyte;
incr(byteptr);end;decr(curparm);end;doxxx;end;3:dopush;4:dopop;
5,6,7,8,9:doright;10,11,12,13,14:dodown;15:curfnt:=curparm;
others:confusion(strpackets);end;if curloc<curlimit then goto 22;
30:{:215}if saveupd then begin curhdimen:=widths[savewp];
{hpixels:=pixwidths[savewp];}dowidth;end;{214:}
curfnt:=recurfnt[nrecur]{:214};end;{:212}{219:}procedure dodvi;
label 30,10;var tempbyte:int8u;tempint:integer;dvistart:integer;
dviboppost:integer;dviback:integer;k:int15;begin{220:}
if dviubyte<>247 then baddvi;if dviubyte<>2 then baddvi;
dvinum:=dvipquad;dviden:=dvipquad;dvimag:=dvipquad;
tfmconv:=(25400000.0/dvinum)*(dviden/473628672)/16.0;tempbyte:=dviubyte;
if maxbytes-byteptr<tempbyte then overflow(strbytes,maxbytes);
for k:=1 to tempbyte do begin bytemem[byteptr]:=dviubyte;incr(byteptr);
end;write(stdout,'DVI file: ''');printpacket(newpacket);
writeln(stdout,''',');
write(stdout,'   num=',dvinum:1,', den=',dviden:1,', mag=',dvimag:1);
if outmag<=0 then outmag:=dvimag else write(stdout,' => ',outmag:1);
writeln(stdout,'.');dopre;flushpacket{:220};if true then{222:}
begin dvistart:=dviloc;{223:}tempint:=dvilength-5;
repeat if tempint<49 then baddvi;dvimove(tempint);tempbyte:=dviubyte;
decr(tempint);until tempbyte<>223;if tempbyte<>2 then baddvi;
dvimove(tempint-4);if dviubyte<>249 then baddvi;dviboppost:=dvipointer;
if(dviboppost<15)or(dviboppost>dviloc-34)then baddvi;
dvimove(dviboppost);if dviubyte<>248 then baddvi{:223};
{writeln(stdout,'DVI: postamble at ',dviboppost:1)};dviback:=dvipointer;
if dvinum<>dvipquad then baddvi;if dviden<>dvipquad then baddvi;
if dvimag<>dvipquad then baddvi;tempint:=dvisquad;tempint:=dvisquad;
if stacksize<dviupair then overflow(strstack,stacksize);
tempint:=dviupair;dvifirstpar;
while curclass=16 do begin dvidofont(false);dvifirstpar;end;
if curcmd<>249 then baddvi;if not selected then{224:}
begin dvistart:=dviboppost;
while dviback<>-1 do begin if(dviback<15)or(dviback>dviboppost-46)then
baddvi;dviboppost:=dviback;dvimove(dviback);
if dviubyte<>139 then baddvi;for k:=0 to 9 do count[k]:=dvisquad;
if startmatch then dvistart:=dviboppost;dviback:=dvipointer;end;
end{:224};dvimove(dvistart);end{:222};repeat dvifirstpar;
while curclass=16 do begin dvidofont(true);dvifirstpar;end;
if curcmd=139 then{225:}begin for k:=0 to 9 do count[k]:=dvisquad;
tempint:=dvipointer;dobop;dvifirstpar;if typesetting then{226:}
while true do begin case curclass of 0:{228:}begin curwp:=0;
if curfnt<>maxfonts then if(curres>=fntbc[curfnt])and(curres<=fntec[
curfnt])then curwp:=charwidths[fntchars[curfnt]+curres];
if curwp=0 then baddvi;if fnttype[curfnt]=0 then dofont;
if fnttype[curfnt]=1 then dovfpacket else dochar;end{:228};
1:if curupd and(curvdimen=widthdimen)then begin{hpixels:=round(hconv*(
curhdimen));}dowidth;end else dorule;
2:begin if maxbytes-byteptr<curparm then overflow(strbytes,maxbytes);
while curparm>0 do begin begin bytemem[byteptr]:=dviubyte;incr(byteptr);
end;decr(curparm);end;doxxx;end;3:dopush;4:dopop;5,6,7,8,9:doright;
10,11,12,13,14:dodown;15:dvifont;16:dvidofont(true);17:goto 30;end;
dvifirstpar;end{:226}else{227:}
while true do begin case curclass of 2:while curparm>0 do begin tempbyte
:=dviubyte;decr(curparm);end;16:dvidofont(true);17:goto 30;others:;end;
dvifirstpar;end{:227};30:if curcmd<>140 then baddvi;
if typesetting then begin doeop;if alldone then goto 10;end;end{:225};
until curcmd<>140;if curcmd<>248 then baddvi;10:end;{:219}{230:}
procedure closefilesandterminate;var k:int15;begin;
if history<3 then{206:}begin if typesetting then{278:}
begin while stackptr>0 do begin begin putbyte(142,outfile);incr(outloc);
end;decr(stackptr);end;begin putbyte(140,outfile);incr(outloc);end;
end{:278};{279:}if outloc>0 then begin{280:}begin putbyte(248,outfile);
incr(outloc);end;outfour(outback);outback:=outloc-5;outfour(dvinum);
outfour(dviden);outfour(outmag);outfour(outmaxv);outfour(outmaxh);
begin putbyte(outstack div 256,outfile);incr(outloc);end;
begin putbyte(outstack mod 256,outfile);incr(outloc);end;
begin putbyte(outpages div 256,outfile);incr(outloc);end;
begin putbyte(outpages mod 256,outfile);incr(outloc);end;k:=outnf;
while k>0 do begin decr(k);outfntdef(outfnts[k]);end;
begin putbyte(249,outfile);incr(outloc);end;outfour(outback);
begin putbyte(2,outfile);incr(outloc);end{:280};k:=7-((outloc-1)mod 4);
while k>0 do begin begin putbyte(223,outfile);incr(outloc);end;decr(k);
end;write(stdout,'OUT file: ',outloc:1,' bytes, ',outpages:1,' page');
if outpages<>1 then write(stdout,'s');
end else write(stdout,'OUT file: no output');
writeln(stdout,' written.');
if outpages=0 then if history=0 then history:=1;{:279}end{:206};
{[232:]writeln(stdout,'Memory usage statistics:');
write(stdout,dvinf:1,' dvi, ',lclnf:1,' local, ');
[247:]write(stdout,outnf:1,' out, ');
[:247]writeln(stdout,'and ',nf:1,' internal fonts of ',maxfonts:1);
writeln(stdout,nwidths:1,' widths of ',maxwidths:1,' for ',nchars:1,
' characters of ',maxchars:1);
writeln(stdout,pcktptr:1,' byte packets of ',maxpackets:1,' with ',
byteptr:1,' bytes of ',maxbytes:1);
[281:][:281]writeln(stdout,stackused:1,' of ',stacksize:1,' stack and ',
recurused:1,' of ',maxrecursion:1,' recursion levels.')[:232];}{237:}
{:237}{233:}case history of 0:writeln(stdout,'(No errors were found.)');
1:writeln(stdout,'(Did you see the warning message above?)');
2:writeln(stdout,'(Pardon me, but I think I spotted something wrong.)');
3:writeln(stdout,'(That was a fatal error, my friend.)');end{:233};end;
{:230}{231:}begin setpaths(TFMFILEPATHBIT+VFFILEPATHBIT);initialize;
if argc<>3 then begin writeln('Usage: dvicopy <dvi file> <outfile>.');
uexit(1);end;{45:}
{if maxbytes-byteptr<5 then overflow(strbytes,maxbytes);}
byteptr:=byteptr+5;bytemem[byteptr-5]:=102;bytemem[byteptr-4]:=111;
bytemem[byteptr-3]:=110;bytemem[byteptr-2]:=116;bytemem[byteptr-1]:=115;
strfonts:=makepacket;
{if maxbytes-byteptr<5 then overflow(strbytes,maxbytes);}
byteptr:=byteptr+5;bytemem[byteptr-5]:=99;bytemem[byteptr-4]:=104;
bytemem[byteptr-3]:=97;bytemem[byteptr-2]:=114;bytemem[byteptr-1]:=115;
strchars:=makepacket;
{if maxbytes-byteptr<6 then overflow(strbytes,maxbytes);}
byteptr:=byteptr+6;bytemem[byteptr-6]:=119;bytemem[byteptr-5]:=105;
bytemem[byteptr-4]:=100;bytemem[byteptr-3]:=116;bytemem[byteptr-2]:=104;
bytemem[byteptr-1]:=115;strwidths:=makepacket;
{if maxbytes-byteptr<7 then overflow(strbytes,maxbytes);}
byteptr:=byteptr+7;bytemem[byteptr-7]:=112;bytemem[byteptr-6]:=97;
bytemem[byteptr-5]:=99;bytemem[byteptr-4]:=107;bytemem[byteptr-3]:=101;
bytemem[byteptr-2]:=116;bytemem[byteptr-1]:=115;strpackets:=makepacket;
{if maxbytes-byteptr<5 then overflow(strbytes,maxbytes);}
byteptr:=byteptr+5;bytemem[byteptr-5]:=98;bytemem[byteptr-4]:=121;
bytemem[byteptr-3]:=116;bytemem[byteptr-2]:=101;bytemem[byteptr-1]:=115;
strbytes:=makepacket;
{if maxbytes-byteptr<9 then overflow(strbytes,maxbytes);}
byteptr:=byteptr+9;bytemem[byteptr-9]:=114;bytemem[byteptr-8]:=101;
bytemem[byteptr-7]:=99;bytemem[byteptr-6]:=117;bytemem[byteptr-5]:=114;
bytemem[byteptr-4]:=115;bytemem[byteptr-3]:=105;bytemem[byteptr-2]:=111;
bytemem[byteptr-1]:=110;strrecursion:=makepacket;
{if maxbytes-byteptr<5 then overflow(strbytes,maxbytes);}
byteptr:=byteptr+5;bytemem[byteptr-5]:=115;bytemem[byteptr-4]:=116;
bytemem[byteptr-3]:=97;bytemem[byteptr-2]:=99;bytemem[byteptr-1]:=107;
strstack:=makepacket;
{if maxbytes-byteptr<10 then overflow(strbytes,maxbytes);}
byteptr:=byteptr+10;bytemem[byteptr-10]:=110;bytemem[byteptr-9]:=97;
bytemem[byteptr-8]:=109;bytemem[byteptr-7]:=101;bytemem[byteptr-6]:=108;
bytemem[byteptr-5]:=101;bytemem[byteptr-4]:=110;bytemem[byteptr-3]:=103;
bytemem[byteptr-2]:=116;bytemem[byteptr-1]:=104;
strnamelength:=makepacket;{:45}{92:}
{if maxbytes-byteptr<4 then overflow(strbytes,maxbytes);}
byteptr:=byteptr+4;bytemem[byteptr-4]:=46;bytemem[byteptr-3]:=116;
bytemem[byteptr-2]:=102;bytemem[byteptr-1]:=109;tfmext:=makepacket;{:92}
{135:}{if maxbytes-byteptr<3 then overflow(strbytes,maxbytes);}
byteptr:=byteptr+3;bytemem[byteptr-3]:=46;bytemem[byteptr-2]:=118;
bytemem[byteptr-1]:=102;vfext:=makepacket;{:135}{193:}
{if maxbytes-byteptr<3 then overflow(strbytes,maxbytes);}
byteptr:=byteptr+3;bytemem[byteptr-3]:=109;bytemem[byteptr-2]:=97;
bytemem[byteptr-1]:=103;strmag:=makepacket;
{if maxbytes-byteptr<6 then overflow(strbytes,maxbytes);}
byteptr:=byteptr+6;bytemem[byteptr-6]:=115;bytemem[byteptr-5]:=101;
bytemem[byteptr-4]:=108;bytemem[byteptr-3]:=101;bytemem[byteptr-2]:=99;
bytemem[byteptr-1]:=116;strselect:=makepacket;{:193}dialog;{111:}
argv(toint(1),curname);reset(dvifile,curname);dviloc:=0;{:111}{236:}
argv(toint(2),outfname);rewrite(outfile,outfname);{:236}dodvi;
closefilesandterminate;end.{:231}
