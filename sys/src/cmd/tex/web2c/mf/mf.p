{4:}{9:}{No compiler directives for C.}{:9}program MF;label{6:}
1,9998,9999;{:6}const{11:}memmax=262140;maxinternal=300;bufsize=3000;
errorline=79;halferrorline=50;maxprintline=79;screenwidth=1664;
screendepth=1200;stacksize=300;maxstrings=7500;stringvacancies=74000;
poolsize=100000;movesize=20000;maxwiggle=1000;gfbufsize=16384;
poolname='mf.pool';pathsize=1000;bistacksize=1500;headersize=100;
ligtablesize=15000;maxkerns=2500;maxfontdimen=50;memtop=262140;{:11}
type{18:}ASCIIcode=0..255;{:18}{24:}eightbits=0..255;{:24}{37:}
poolpointer=0..poolsize;strnumber=0..maxstrings;packedASCIIcode=0..255;
{:37}{101:}scaled=integer;smallnumber=0..63;{:101}{105:}
fraction=integer;{:105}{106:}angle=integer;{:106}{156:}
quarterword=0..255;halfword=0..262143;twochoices=1..2;threechoices=1..3;
#include "texmfmem.h";{:156}{186:}commandcode=1..85;{:186}{565:}
screenrow=0..screendepth;screencol=0..screenwidth;
transspec=array[screencol]of screencol;pixelcolor=0..1;{:565}{571:}
windownumber=0..15;{:571}{627:}
instaterecord=record indexfield:quarterword;
startfield,locfield,limitfield,namefield:halfword;end;{:627}{1151:}
gfindex=0..gfbufsize;{:1151}var{13:}bad:integer;{:13}{20:}
xord:array[ASCIIcode]of ASCIIcode;xchr:array[ASCIIcode]of ASCIIcode;
{:20}{25:}nameoffile:packed array[1..PATHMAX]of char;
namelength:0..PATHMAX;{:25}{29:}buffer:array[0..bufsize]of ASCIIcode;
first:0..bufsize;last:0..bufsize;maxbufstack:0..bufsize;{:29}{38:}
strpool:packed array[poolpointer]of packedASCIIcode;
strstart:array[strnumber]of poolpointer;poolptr:poolpointer;
strptr:strnumber;initpoolptr:poolpointer;initstrptr:strnumber;
maxpoolptr:poolpointer;maxstrptr:strnumber;{:38}{42:}
strref:array[strnumber]of 0..127;{:42}{50:}
ifdef('INIMF')poolfile:alphafile;endif('INIMF'){:50}{54:}
logfile:alphafile;selector:0..5;dig:array[0..22]of 0..15;tally:integer;
termoffset:0..maxprintline;fileoffset:0..maxprintline;
trickbuf:array[0..errorline]of ASCIIcode;trickcount:integer;
firstcount:integer;{:54}{68:}interaction:0..3;{:68}{71:}
deletionsallowed:boolean;history:0..3;errorcount:-1..100;{:71}{74:}
helpline:array[0..5]of strnumber;helpptr:0..6;useerrhelp:boolean;
errhelp:strnumber;{:74}{91:}interrupt:integer;OKtointerrupt:boolean;
{:91}{97:}aritherror:boolean;{:97}{129:}twotothe:array[0..30]of integer;
speclog:array[1..28]of integer;{:129}{137:}
specatan:array[1..26]of angle;{:137}{144:}nsin,ncos:fraction;{:144}
{148:}randoms:array[0..54]of fraction;jrandom:0..54;{:148}{159:}
mem:array[0..memmax]of memoryword;lomemmax:halfword;himemmin:halfword;
{:159}{160:}varused,dynused:integer;{:160}{161:}avail:halfword;
memend:halfword;{:161}{166:}rover:halfword;{:166}{178:}
ifdef('DEBUG')freearr:packed array[0..memmax]of boolean;
wasfree:packed array[0..memmax]of boolean;
wasmemend,waslomax,washimin:halfword;panicking:boolean;
endif('DEBUG'){:178}{190:}internal:array[1..maxinternal]of scaled;
intname:array[1..maxinternal]of strnumber;intptr:41..maxinternal;{:190}
{196:}oldsetting:0..5;{:196}{198:}charclass:array[ASCIIcode]of 0..20;
{:198}{200:}hashused:halfword;stcount:integer;{:200}{201:}
hash:array[1..9769]of twohalves;eqtb:array[1..9769]of twohalves;{:201}
{225:}gpointer:halfword;{:225}{230:}
bignodesize:array[13..14]of smallnumber;{:230}{250:}saveptr:halfword;
{:250}{267:}pathtail:halfword;{:267}{279:}
deltax,deltay,delta:array[0..pathsize]of scaled;
psi:array[1..pathsize]of angle;{:279}{283:}
theta:array[0..pathsize]of angle;uu:array[0..pathsize]of fraction;
vv:array[0..pathsize]of angle;ww:array[0..pathsize]of fraction;{:283}
{298:}st,ct,sf,cf:fraction;{:298}{308:}
move:array[0..movesize]of integer;moveptr:0..movesize;{:308}{309:}
bisectstack:array[0..bistacksize]of integer;bisectptr:0..bistacksize;
{:309}{327:}curedges:halfword;curwt:integer;{:327}{371:}tracex:integer;
tracey:integer;traceyy:integer;{:371}{379:}octant:1..8;{:379}{389:}
curx,cury:scaled;{:389}{395:}octantdir:array[1..8]of strnumber;{:395}
{403:}curspec:halfword;turningnumber:integer;curpen:halfword;
curpathtype:0..1;maxallowed:scaled;{:403}{427:}
before,after:array[0..maxwiggle]of scaled;
nodetoround:array[0..maxwiggle]of halfword;curroundingptr:0..maxwiggle;
maxroundingptr:0..maxwiggle;{:427}{430:}curgran:scaled;{:430}{448:}
octantnumber:array[1..8]of 1..8;octantcode:array[1..8]of 1..8;{:448}
{455:}revturns:boolean;{:455}{461:}
ycorr,xycorr,zcorr:array[1..8]of 0..1;xcorr:array[1..8]of-1..1;{:461}
{464:}m0,n0,m1,n1:integer;d0,d1:0..1;{:464}{507:}
envmove:array[0..movesize]of integer;{:507}{552:}tolstep:0..6;{:552}
{555:}curt,curtt:integer;timetogo:integer;maxt:integer;{:555}{557:}
delx,dely:integer;tol:integer;uv,xy:0..bistacksize;threel:integer;
apprt,apprtt:integer;{:557}{566:}
{screenpixel:array[screenrow,screencol]of pixelcolor;}{:566}{569:}
screenstarted:boolean;screenOK:boolean;{:569}{572:}
windowopen:array[windownumber]of boolean;
leftcol:array[windownumber]of screencol;
rightcol:array[windownumber]of screencol;
toprow:array[windownumber]of screenrow;
botrow:array[windownumber]of screenrow;
mwindow:array[windownumber]of integer;
nwindow:array[windownumber]of integer;
windowtime:array[windownumber]of integer;{:572}{579:}
rowtransition:transspec;{:579}{585:}serialno:integer;{:585}{592:}
fixneeded:boolean;watchcoefs:boolean;depfinal:halfword;{:592}{624:}
curcmd:eightbits;curmod:integer;cursym:halfword;{:624}{628:}
inputstack:array[0..stacksize]of instaterecord;inputptr:0..stacksize;
maxinstack:0..stacksize;curinput:instaterecord;{:628}{631:}inopen:0..15;
openparens:0..15;inputfile:array[1..15]of alphafile;line:integer;
linestack:array[1..15]of integer;{:631}{633:}
paramstack:array[0..150]of halfword;paramptr:0..150;
maxparamstack:integer;{:633}{634:}fileptr:0..stacksize;{:634}{659:}
scannerstatus:0..6;warninginfo:integer;{:659}{680:}forceeof:boolean;
{:680}{699:}bgloc,egloc:1..9769;{:699}{738:}condptr:halfword;
iflimit:0..4;curif:smallnumber;ifline:integer;{:738}{752:}
loopptr:halfword;{:752}{767:}curname:strnumber;curarea:strnumber;
curext:strnumber;{:767}{768:}areadelimiter:poolpointer;
extdelimiter:poolpointer;{:768}{775:}basedefaultlength:integer;
MFbasedefault:ccharpointer;{:775}{782:}jobname:strnumber;
logopened:boolean;texmflogname:strnumber;{:782}{785:}gfext:strnumber;
{:785}{791:}gffile:bytefile;outputfilename:strnumber;{:791}{796:}
curtype:smallnumber;curexp:integer;{:796}{813:}
maxc:array[17..18]of integer;maxptr:array[17..18]of halfword;
maxlink:array[17..18]of halfword;{:813}{821:}varflag:0..85;{:821}{954:}
txx,txy,tyx,tyy,tx,ty:scaled;{:954}{1077:}startsym:halfword;{:1077}
{1084:}longhelpseen:boolean;{:1084}{1087:}tfmfile:bytefile;
metricfilename:strnumber;{:1087}{1096:}bc,ec:eightbits;
tfmwidth:array[eightbits]of scaled;tfmheight:array[eightbits]of scaled;
tfmdepth:array[eightbits]of scaled;
tfmitalcorr:array[eightbits]of scaled;
charexists:array[eightbits]of boolean;chartag:array[eightbits]of 0..3;
charremainder:array[eightbits]of 0..ligtablesize;
headerbyte:array[1..headersize]of-1..255;
ligkern:array[0..ligtablesize]of fourquarters;nl:0..32511;
kern:array[0..maxkerns]of scaled;nk:0..maxkerns;
exten:array[eightbits]of fourquarters;ne:0..256;
param:array[1..maxfontdimen]of scaled;np:0..maxfontdimen;
nw,nh,nd,ni:0..256;skiptable:array[eightbits]of 0..ligtablesize;
lkstarted:boolean;bchar:integer;bchlabel:0..ligtablesize;
ll,lll:0..ligtablesize;labelloc:array[0..256]of-1..ligtablesize;
labelchar:array[1..256]of eightbits;labelptr:0..256;{:1096}{1119:}
perturbation:scaled;excess:integer;{:1119}{1125:}
dimenhead:array[1..4]of halfword;{:1125}{1130:}maxtfmdimen:scaled;
tfmchanged:integer;{:1130}{1149:}gfminm,gfmaxm,gfminn,gfmaxn:integer;
gfprevptr:integer;totalchars:integer;charptr:array[eightbits]of integer;
gfdx,gfdy:array[eightbits]of integer;{:1149}{1152:}
gfbuf:array[gfindex]of eightbits;halfbuf:gfindex;gflimit:gfindex;
gfptr:gfindex;gfoffset:integer;{:1152}{1162:}bocc,bocp:integer;{:1162}
{1183:}baseident:strnumber;{:1183}{1188:}basefile:wordfile;{:1188}
{1203:}readyalready:integer;{:1203}{1214:}editnamestart:poolpointer;
editnamelength,editline:integer;{:1214}procedure initialize;var{19:}
i:integer;{:19}{130:}k:integer;{:130}begin{21:}xchr[32]:=' ';
xchr[33]:='!';xchr[34]:='"';xchr[35]:='#';xchr[36]:='$';xchr[37]:='%';
xchr[38]:='&';xchr[39]:='''';xchr[40]:='(';xchr[41]:=')';xchr[42]:='*';
xchr[43]:='+';xchr[44]:=',';xchr[45]:='-';xchr[46]:='.';xchr[47]:='/';
xchr[48]:='0';xchr[49]:='1';xchr[50]:='2';xchr[51]:='3';xchr[52]:='4';
xchr[53]:='5';xchr[54]:='6';xchr[55]:='7';xchr[56]:='8';xchr[57]:='9';
xchr[58]:=':';xchr[59]:=';';xchr[60]:='<';xchr[61]:='=';xchr[62]:='>';
xchr[63]:='?';xchr[64]:='@';xchr[65]:='A';xchr[66]:='B';xchr[67]:='C';
xchr[68]:='D';xchr[69]:='E';xchr[70]:='F';xchr[71]:='G';xchr[72]:='H';
xchr[73]:='I';xchr[74]:='J';xchr[75]:='K';xchr[76]:='L';xchr[77]:='M';
xchr[78]:='N';xchr[79]:='O';xchr[80]:='P';xchr[81]:='Q';xchr[82]:='R';
xchr[83]:='S';xchr[84]:='T';xchr[85]:='U';xchr[86]:='V';xchr[87]:='W';
xchr[88]:='X';xchr[89]:='Y';xchr[90]:='Z';xchr[91]:='[';xchr[92]:='\';
xchr[93]:=']';xchr[94]:='^';xchr[95]:='_';xchr[96]:='`';xchr[97]:='a';
xchr[98]:='b';xchr[99]:='c';xchr[100]:='d';xchr[101]:='e';
xchr[102]:='f';xchr[103]:='g';xchr[104]:='h';xchr[105]:='i';
xchr[106]:='j';xchr[107]:='k';xchr[108]:='l';xchr[109]:='m';
xchr[110]:='n';xchr[111]:='o';xchr[112]:='p';xchr[113]:='q';
xchr[114]:='r';xchr[115]:='s';xchr[116]:='t';xchr[117]:='u';
xchr[118]:='v';xchr[119]:='w';xchr[120]:='x';xchr[121]:='y';
xchr[122]:='z';xchr[123]:='{';xchr[124]:='|';xchr[125]:='}';
xchr[126]:='~';{:21}{22:}for i:=0 to 31 do xchr[i]:=chr(i);
for i:=127 to 255 do xchr[i]:=chr(i);{:22}{23:}
for i:=0 to 255 do xord[chr(i)]:=127;
for i:=128 to 255 do xord[xchr[i]]:=i;
for i:=0 to 126 do xord[xchr[i]]:=i;{:23}{69:}interaction:=3;{:69}{72:}
deletionsallowed:=true;errorcount:=0;{:72}{75:}helpptr:=0;
useerrhelp:=false;errhelp:=0;{:75}{92:}interrupt:=0;OKtointerrupt:=true;
{:92}{98:}aritherror:=false;{:98}{131:}twotothe[0]:=1;
for k:=1 to 30 do twotothe[k]:=2*twotothe[k-1];speclog[1]:=93032640;
speclog[2]:=38612034;speclog[3]:=17922280;speclog[4]:=8662214;
speclog[5]:=4261238;speclog[6]:=2113709;speclog[7]:=1052693;
speclog[8]:=525315;speclog[9]:=262400;speclog[10]:=131136;
speclog[11]:=65552;speclog[12]:=32772;speclog[13]:=16385;
for k:=14 to 27 do speclog[k]:=twotothe[27-k];speclog[28]:=1;{:131}
{138:}specatan[1]:=27855475;specatan[2]:=14718068;specatan[3]:=7471121;
specatan[4]:=3750058;specatan[5]:=1876857;specatan[6]:=938658;
specatan[7]:=469357;specatan[8]:=234682;specatan[9]:=117342;
specatan[10]:=58671;specatan[11]:=29335;specatan[12]:=14668;
specatan[13]:=7334;specatan[14]:=3667;specatan[15]:=1833;
specatan[16]:=917;specatan[17]:=458;specatan[18]:=229;specatan[19]:=115;
specatan[20]:=57;specatan[21]:=29;specatan[22]:=14;specatan[23]:=7;
specatan[24]:=4;specatan[25]:=2;specatan[26]:=1;{:138}{179:}
ifdef('DEBUG')wasmemend:=0;waslomax:=0;washimin:=memmax;
panicking:=false;endif('DEBUG'){:179}{191:}
for k:=1 to 41 do internal[k]:=0;intptr:=41;{:191}{199:}
for k:=48 to 57 do charclass[k]:=0;charclass[46]:=1;charclass[32]:=2;
charclass[37]:=3;charclass[34]:=4;charclass[44]:=5;charclass[59]:=6;
charclass[40]:=7;charclass[41]:=8;for k:=65 to 90 do charclass[k]:=9;
for k:=97 to 122 do charclass[k]:=9;charclass[95]:=9;charclass[60]:=10;
charclass[61]:=10;charclass[62]:=10;charclass[58]:=10;
charclass[124]:=10;charclass[96]:=11;charclass[39]:=11;
charclass[43]:=12;charclass[45]:=12;charclass[47]:=13;charclass[42]:=13;
charclass[92]:=13;charclass[33]:=14;charclass[63]:=14;charclass[35]:=15;
charclass[38]:=15;charclass[64]:=15;charclass[36]:=15;charclass[94]:=16;
charclass[126]:=16;charclass[91]:=17;charclass[93]:=18;
charclass[123]:=19;charclass[125]:=19;
for k:=0 to 31 do charclass[k]:=20;
for k:=127 to 255 do charclass[k]:=20;charclass[9]:=2;charclass[12]:=2;
{:199}{202:}hash[1].lh:=0;hash[1].rh:=0;eqtb[1].lh:=41;eqtb[1].rh:=0;
for k:=2 to 9769 do begin hash[k]:=hash[1];eqtb[k]:=eqtb[1];end;{:202}
{231:}bignodesize[13]:=12;bignodesize[14]:=4;{:231}{251:}saveptr:=0;
{:251}{396:}octantdir[1]:=547;octantdir[5]:=548;octantdir[6]:=549;
octantdir[2]:=550;octantdir[4]:=551;octantdir[8]:=552;octantdir[7]:=553;
octantdir[3]:=554;{:396}{428:}maxroundingptr:=0;{:428}{449:}
octantcode[1]:=1;octantcode[2]:=5;octantcode[3]:=6;octantcode[4]:=2;
octantcode[5]:=4;octantcode[6]:=8;octantcode[7]:=7;octantcode[8]:=3;
for k:=1 to 8 do octantnumber[octantcode[k]]:=k;{:449}{456:}
revturns:=false;{:456}{462:}xcorr[1]:=0;ycorr[1]:=0;xycorr[1]:=0;
xcorr[5]:=0;ycorr[5]:=0;xycorr[5]:=1;xcorr[6]:=-1;ycorr[6]:=1;
xycorr[6]:=0;xcorr[2]:=1;ycorr[2]:=0;xycorr[2]:=1;xcorr[4]:=0;
ycorr[4]:=1;xycorr[4]:=1;xcorr[8]:=0;ycorr[8]:=1;xycorr[8]:=0;
xcorr[7]:=1;ycorr[7]:=0;xycorr[7]:=1;xcorr[3]:=-1;ycorr[3]:=1;
xycorr[3]:=0;for k:=1 to 8 do zcorr[k]:=xycorr[k]-xcorr[k];{:462}{570:}
screenstarted:=false;screenOK:=false;{:570}{573:}
for k:=0 to 15 do begin windowopen[k]:=false;windowtime[k]:=0;end;{:573}
{593:}fixneeded:=false;watchcoefs:=true;{:593}{739:}condptr:=0;
iflimit:=0;curif:=0;ifline:=0;{:739}{753:}loopptr:=0;{:753}{797:}
curexp:=0;{:797}{822:}varflag:=0;{:822}{1078:}startsym:=0;{:1078}{1085:}
longhelpseen:=false;{:1085}{1097:}
for k:=0 to 255 do begin tfmwidth[k]:=0;tfmheight[k]:=0;tfmdepth[k]:=0;
tfmitalcorr[k]:=0;charexists[k]:=false;chartag[k]:=0;
charremainder[k]:=0;skiptable[k]:=ligtablesize;end;
for k:=1 to headersize do headerbyte[k]:=-1;bc:=255;ec:=0;nl:=0;nk:=0;
ne:=0;np:=0;internal[41]:=-65536;bchlabel:=ligtablesize;labelloc[0]:=-1;
labelptr:=0;{:1097}{1150:}gfprevptr:=0;totalchars:=0;{:1150}{1153:}
halfbuf:=gfbufsize div 2;gflimit:=gfbufsize;gfptr:=0;gfoffset:=0;{:1153}
{1184:}baseident:=0;{:1184}{1215:}editnamestart:=0;{:1215}end;{57:}
procedure println;begin case selector of 3:begin writeln(stdout);
writeln(logfile);termoffset:=0;fileoffset:=0;end;
2:begin writeln(logfile);fileoffset:=0;end;1:begin writeln(stdout);
termoffset:=0;end;0,4,5:;end;end;{:57}{58:}
procedure printchar(s:ASCIIcode);
begin case selector of 3:begin write(stdout,xchr[s]);
write(logfile,xchr[s]);incr(termoffset);incr(fileoffset);
if termoffset=maxprintline then begin writeln(stdout);termoffset:=0;end;
if fileoffset=maxprintline then begin writeln(logfile);fileoffset:=0;
end;end;2:begin write(logfile,xchr[s]);incr(fileoffset);
if fileoffset=maxprintline then println;end;
1:begin write(stdout,xchr[s]);incr(termoffset);
if termoffset=maxprintline then println;end;0:;
4:if tally<trickcount then trickbuf[tally mod errorline]:=s;
5:begin if poolptr<poolsize then begin strpool[poolptr]:=s;
incr(poolptr);end;end;end;incr(tally);end;{:58}{59:}
procedure print(s:integer);var j:poolpointer;
begin if(s<0)or(s>=strptr)then s:=259;
if(s<256)and(selector>4)then printchar(s)else begin j:=strstart[s];
while j<strstart[s+1]do begin printchar(strpool[j]);incr(j);end;end;end;
{:59}{60:}procedure slowprint(s:integer);var j:poolpointer;
begin if(s<0)or(s>=strptr)then s:=259;
if(s<256)and(selector>4)then printchar(s)else begin j:=strstart[s];
while j<strstart[s+1]do begin print(strpool[j]);incr(j);end;end;end;
{:60}{62:}procedure printnl(s:strnumber);
begin if((termoffset>0)and(odd(selector)))or((fileoffset>0)and(selector
>=2))then println;print(s);end;{:62}{63:}
procedure printthedigs(k:eightbits);begin while k>0 do begin decr(k);
printchar(48+dig[k]);end;end;{:63}{64:}procedure printint(n:integer);
var k:0..23;m:integer;begin k:=0;if n<0 then begin printchar(45);
if n>-100000000 then n:=-n else begin m:=-1-n;n:=m div 10;
m:=(m mod 10)+1;k:=1;if m<10 then dig[0]:=m else begin dig[0]:=0;
incr(n);end;end;end;repeat dig[k]:=n mod 10;n:=n div 10;incr(k);
until n=0;printthedigs(k);end;{:64}{103:}
procedure printscaled(s:scaled);var delta:scaled;
begin if s<0 then begin printchar(45);s:=-s;end;printint(s div 65536);
s:=10*(s mod 65536)+5;if s<>5 then begin delta:=10;printchar(46);
repeat if delta>65536 then s:=s+32768-(delta div 2);
printchar(48+(s div 65536));s:=10*(s mod 65536);delta:=delta*10;
until s<=delta;end;end;{:103}{104:}procedure printtwo(x,y:scaled);
begin printchar(40);printscaled(x);printchar(44);printscaled(y);
printchar(41);end;{:104}{187:}procedure printtype(t:smallnumber);
begin case t of 1:print(322);2:print(323);3:print(324);4:print(325);
5:print(326);6:print(327);7:print(328);8:print(329);9:print(330);
10:print(331);11:print(332);12:print(333);13:print(334);14:print(335);
16:print(336);17:print(337);18:print(338);15:print(339);19:print(340);
20:print(341);21:print(342);22:print(343);23:print(344);
others:print(345)end;end;{:187}{195:}procedure begindiagnostic;
begin oldsetting:=selector;
if(internal[13]<=0)and(selector=3)then begin decr(selector);
if history=0 then history:=1;end;end;
procedure enddiagnostic(blankline:boolean);begin printnl(283);
if blankline then println;selector:=oldsetting;end;{:195}{197:}
procedure printdiagnostic(s,t:strnumber;nuline:boolean);
begin begindiagnostic;if nuline then printnl(s)else print(s);print(449);
printint(line);print(t);printchar(58);end;{:197}{773:}
procedure printfilename(n,a,e:integer);begin slowprint(a);slowprint(n);
slowprint(e);end;{:773}{73:}procedure normalizeselector;forward;
procedure getnext;forward;procedure terminput;forward;
procedure showcontext;forward;procedure beginfilereading;forward;
procedure openlogfile;forward;procedure closefilesandterminate;forward;
procedure clearforerrorprompt;forward;ifdef('DEBUG')procedure debughelp;
forward;endif('DEBUG'){43:}procedure flushstring(s:strnumber);
begin if s<strptr-1 then strref[s]:=0 else repeat decr(strptr);
until strref[strptr-1]<>0;poolptr:=strstart[strptr];end;{:43}{:73}{76:}
procedure jumpout;begin closefilesandterminate;begin flush(stdout);
readyalready:=0;if(history<>0)and(history<>1)then uexit(1)else uexit(0);
end;end;{:76}{77:}procedure error;label 22,10;var c:ASCIIcode;
s1,s2,s3:integer;j:poolpointer;begin if history<2 then history:=2;
printchar(46);showcontext;if interaction=3 then{78:}
while true do begin 22:clearforerrorprompt;begin;print(263);terminput;
end;if last=first then goto 10;c:=buffer[first];if c>=97 then c:=c-32;
{79:}
case c of 48,49,50,51,52,53,54,55,56,57:if deletionsallowed then{83:}
begin s1:=curcmd;s2:=curmod;s3:=cursym;OKtointerrupt:=false;
if(last>first+1)and(buffer[first+1]>=48)and(buffer[first+1]<=57)then c:=
c*10+buffer[first+1]-48*11 else c:=c-48;while c>0 do begin getnext;
{743:}
if curcmd=39 then begin if strref[curmod]<127 then if strref[curmod]>1
then decr(strref[curmod])else flushstring(curmod);end{:743};decr(c);end;
curcmd:=s1;curmod:=s2;cursym:=s3;OKtointerrupt:=true;begin helpptr:=2;
helpline[1]:=276;helpline[0]:=277;end;showcontext;goto 22;end{:83};
ifdef('DEBUG')68:begin debughelp;goto 22;end;
endif('DEBUG')69:if fileptr>0 then begin editnamestart:=strstart[
inputstack[fileptr].namefield];
editnamelength:=strstart[inputstack[fileptr].namefield+1]-strstart[
inputstack[fileptr].namefield];editline:=line;jumpout;end;72:{84:}
begin if useerrhelp then begin{85:}j:=strstart[errhelp];
while j<strstart[errhelp+1]do begin if strpool[j]<>37 then print(strpool
[j])else if j+1=strstart[errhelp+1]then println else if strpool[j+1]<>37
then println else begin incr(j);printchar(37);end;incr(j);end{:85};
useerrhelp:=false;end else begin if helpptr=0 then begin helpptr:=2;
helpline[1]:=278;helpline[0]:=279;end;repeat decr(helpptr);
print(helpline[helpptr]);println;until helpptr=0;end;begin helpptr:=4;
helpline[3]:=280;helpline[2]:=279;helpline[1]:=281;helpline[0]:=282;end;
goto 22;end{:84};73:{82:}begin beginfilereading;
if last>first+1 then begin curinput.locfield:=first+1;buffer[first]:=32;
end else begin begin;print(275);terminput;end;curinput.locfield:=first;
end;first:=last+1;curinput.limitfield:=last;goto 10;end{:82};
81,82,83:{81:}begin errorcount:=0;interaction:=0+c-81;print(270);
case c of 81:begin print(271);decr(selector);end;82:print(272);
83:print(273);end;print(274);println;flush(stdout);goto 10;end{:81};
88:begin interaction:=2;jumpout;end;others:end;{80:}begin print(264);
printnl(265);printnl(266);if fileptr>0 then print(267);
if deletionsallowed then printnl(268);printnl(269);end{:80}{:79};
end{:78};incr(errorcount);if errorcount=100 then begin printnl(262);
history:=3;jumpout;end;{86:}if interaction>0 then decr(selector);
if useerrhelp then begin printnl(283);{85:}j:=strstart[errhelp];
while j<strstart[errhelp+1]do begin if strpool[j]<>37 then print(strpool
[j])else if j+1=strstart[errhelp+1]then println else if strpool[j+1]<>37
then println else begin incr(j);printchar(37);end;incr(j);end{:85};
end else while helpptr>0 do begin decr(helpptr);
printnl(helpline[helpptr]);end;println;
if interaction>0 then incr(selector);println{:86};10:end;{:77}{88:}
procedure fatalerror(s:strnumber);begin normalizeselector;
begin if interaction=3 then;printnl(261);print(284);end;
begin helpptr:=1;helpline[0]:=s;end;
begin if interaction=3 then interaction:=2;if logopened then error;
ifdef('DEBUG')if interaction>0 then debughelp;endif('DEBUG')history:=3;
jumpout;end;end;{:88}{89:}procedure overflow(s:strnumber;n:integer);
begin normalizeselector;begin if interaction=3 then;printnl(261);
print(285);end;print(s);printchar(61);printint(n);printchar(93);
begin helpptr:=2;helpline[1]:=286;helpline[0]:=287;end;
begin if interaction=3 then interaction:=2;if logopened then error;
ifdef('DEBUG')if interaction>0 then debughelp;endif('DEBUG')history:=3;
jumpout;end;end;{:89}{90:}procedure confusion(s:strnumber);
begin normalizeselector;
if history<2 then begin begin if interaction=3 then;printnl(261);
print(288);end;print(s);printchar(41);begin helpptr:=1;helpline[0]:=289;
end;end else begin begin if interaction=3 then;printnl(261);print(290);
end;begin helpptr:=2;helpline[1]:=291;helpline[0]:=292;end;end;
begin if interaction=3 then interaction:=2;if logopened then error;
ifdef('DEBUG')if interaction>0 then debughelp;endif('DEBUG')history:=3;
jumpout;end;end;{:90}{:4}{36:}function initterminal:boolean;label 10;
begin topenin;if last>first then begin curinput.locfield:=first;
while(curinput.locfield<last)and(buffer[curinput.locfield]=' ')do incr(
curinput.locfield);
if curinput.locfield<last then begin initterminal:=true;goto 10;end;end;
while true do begin;write(stdout,'**');flush(stdout);
if not inputln(stdin,true)then begin writeln(stdout);
write(stdout,'! End of file on the terminal... why?');
initterminal:=false;goto 10;end;curinput.locfield:=first;
while(curinput.locfield<last)and(buffer[curinput.locfield]=32)do incr(
curinput.locfield);
if curinput.locfield<last then begin initterminal:=true;goto 10;end;
writeln(stdout,'Please type the name of your input file.');end;10:end;
{:36}{44:}function makestring:strnumber;
begin if strptr=maxstrptr then begin if strptr=maxstrings then overflow(
258,maxstrings-initstrptr);incr(maxstrptr);end;strref[strptr]:=1;
incr(strptr);strstart[strptr]:=poolptr;makestring:=strptr-1;end;{:44}
{45:}function streqbuf(s:strnumber;k:integer):boolean;label 45;
var j:poolpointer;result:boolean;begin j:=strstart[s];
while j<strstart[s+1]do begin if strpool[j]<>buffer[k]then begin result
:=false;goto 45;end;incr(j);incr(k);end;result:=true;
45:streqbuf:=result;end;{:45}{46:}
function strvsstr(s,t:strnumber):integer;label 10;var j,k:poolpointer;
ls,lt:integer;l:integer;begin ls:=(strstart[s+1]-strstart[s]);
lt:=(strstart[t+1]-strstart[t]);if ls<=lt then l:=ls else l:=lt;
j:=strstart[s];k:=strstart[t];
while l>0 do begin if strpool[j]<>strpool[k]then begin strvsstr:=strpool
[j]-strpool[k];goto 10;end;incr(j);incr(k);decr(l);end;strvsstr:=ls-lt;
10:end;{:46}{47:}ifdef('INIMF')function getstringsstarted:boolean;
label 30,10;var k,l:0..255;m,n:ASCIIcode;g:strnumber;a:integer;
c:boolean;begin poolptr:=0;strptr:=0;maxpoolptr:=0;maxstrptr:=0;
strstart[0]:=0;{48:}for k:=0 to 255 do begin if({49:}
(k<32)or(k>126){:49})then begin begin strpool[poolptr]:=94;
incr(poolptr);end;begin strpool[poolptr]:=94;incr(poolptr);end;
if k<64 then begin strpool[poolptr]:=k+64;incr(poolptr);
end else if k<128 then begin strpool[poolptr]:=k-64;incr(poolptr);
end else begin l:=k div 16;if l<10 then begin strpool[poolptr]:=l+48;
incr(poolptr);end else begin strpool[poolptr]:=l+87;incr(poolptr);end;
l:=k mod 16;if l<10 then begin strpool[poolptr]:=l+48;incr(poolptr);
end else begin strpool[poolptr]:=l+87;incr(poolptr);end;end;
end else begin strpool[poolptr]:=k;incr(poolptr);end;g:=makestring;
strref[g]:=127;end{:48};{51:}vstrcpy(nameoffile+1,poolname);
nameoffile[0]:=' ';nameoffile[strlen(poolname)+1]:=' ';
namelength:=strlen(poolname);
if aopenin(poolfile,MFPOOLPATH)then begin c:=false;repeat{52:}
begin if eof(poolfile)then begin;
writeln(stdout,'! mf.pool has no check sum.');aclose(poolfile);
getstringsstarted:=false;goto 10;end;read(poolfile,m);read(poolfile,n);
if m='*'then{53:}begin a:=0;k:=1;
while true do begin if(xord[n]<48)or(xord[n]>57)then begin;
writeln(stdout,'! mf.pool check sum doesn''t have nine digits.');
aclose(poolfile);getstringsstarted:=false;goto 10;end;
a:=10*a+xord[n]-48;if k=9 then goto 30;incr(k);read(poolfile,n);end;
30:if a<>426472440 then begin;
writeln(stdout,'! mf.pool doesn''t match; tangle me again.');
aclose(poolfile);getstringsstarted:=false;goto 10;end;c:=true;end{:53}
else begin if(xord[m]<48)or(xord[m]>57)or(xord[n]<48)or(xord[n]>57)then
begin;writeln(stdout,'! mf.pool line doesn''t begin with two digits.');
aclose(poolfile);getstringsstarted:=false;goto 10;end;
l:=xord[m]*10+xord[n]-48*11;
if poolptr+l+stringvacancies>poolsize then begin;
writeln(stdout,'! You have to increase POOLSIZE.');aclose(poolfile);
getstringsstarted:=false;goto 10;end;
for k:=1 to l do begin if eoln(poolfile)then m:=' 'else read(poolfile,m)
;begin strpool[poolptr]:=xord[m];incr(poolptr);end;end;readln(poolfile);
g:=makestring;strref[g]:=127;end;end{:52};until c;aclose(poolfile);
getstringsstarted:=true;end else begin;
writeln(stdout,'! I can''t read mf.pool.');aclose(poolfile);
getstringsstarted:=false;goto 10;end{:51};10:end;endif('INIMF'){:47}
{65:}procedure printdd(n:integer);begin n:=abs(n)mod 100;
printchar(48+(n div 10));printchar(48+(n mod 10));end;{:65}{66:}
procedure terminput;var k:0..bufsize;begin flush(stdout);
if not inputln(stdin,true)then fatalerror(260);termoffset:=0;
decr(selector);
if last<>first then for k:=first to last-1 do print(buffer[k]);println;
buffer[last]:=37;incr(selector);end;{:66}{87:}
procedure normalizeselector;
begin if logopened then selector:=3 else selector:=1;
if jobname=0 then openlogfile;if interaction=0 then decr(selector);end;
{:87}{93:}procedure pauseforinstructions;
begin if OKtointerrupt then begin interaction:=3;
if(selector=2)or(selector=0)then incr(selector);
begin if interaction=3 then;printnl(261);print(293);end;
begin helpptr:=3;helpline[2]:=294;helpline[1]:=295;helpline[0]:=296;end;
deletionsallowed:=false;error;deletionsallowed:=true;interrupt:=0;end;
end;{:93}{94:}procedure missingerr(s:strnumber);
begin begin if interaction=3 then;printnl(261);print(297);end;print(s);
print(298);end;{:94}{99:}procedure cleararith;
begin begin if interaction=3 then;printnl(261);print(299);end;
begin helpptr:=4;helpline[3]:=300;helpline[2]:=301;helpline[1]:=302;
helpline[0]:=303;end;error;aritherror:=false;end;{:99}{100:}
function slowadd(x,y:integer):integer;
begin if x>=0 then if y<=2147483647-x then slowadd:=x+y else begin
aritherror:=true;slowadd:=2147483647;
end else if-y<=2147483647+x then slowadd:=x+y else begin aritherror:=
true;slowadd:=-2147483647;end;end;{:100}{102:}
function rounddecimals(k:smallnumber):scaled;var a:integer;begin a:=0;
while k>0 do begin decr(k);a:=(a+dig[k]*131072)div 10;end;
rounddecimals:=(a+1)div 2;end;{:102}{107:}
function makefraction(p,q:integer):fraction;var f:integer;n:integer;
negative:boolean;becareful:integer;
begin if p>=0 then negative:=false else begin p:=-p;negative:=true;end;
if q<=0 then begin ifdef('DEBUG')if q=0 then confusion(47);
endif('DEBUG')q:=-q;negative:=not negative;end;n:=p div q;p:=p mod q;
if n>=8 then begin aritherror:=true;
if negative then makefraction:=-2147483647 else makefraction:=2147483647
;end else begin n:=(n-1)*268435456;{108:}f:=1;repeat becareful:=p-q;
p:=becareful+p;if p>=0 then f:=f+f+1 else begin f:=f+f;p:=p+q;end;
until f>=268435456;becareful:=p-q;if becareful+p>=0 then incr(f){:108};
if negative then makefraction:=-(f+n)else makefraction:=f+n;end;end;
{:107}{109:}function takefraction(q:integer;f:fraction):integer;
var p:integer;negative:boolean;n:integer;becareful:integer;begin{110:}
if f>=0 then negative:=false else begin f:=-f;negative:=true;end;
if q<0 then begin q:=-q;negative:=not negative;end;{:110};
if f<268435456 then n:=0 else begin n:=f div 268435456;
f:=f mod 268435456;
if q<=2147483647 div n then n:=n*q else begin aritherror:=true;
n:=2147483647;end;end;f:=f+268435456;{111:}p:=134217728;
if q<1073741824 then repeat if odd(f)then p:=(p+q)div 2 else p:=(p)div 2
;f:=(f)div 2;
until f=1 else repeat if odd(f)then p:=p+(q-p)div 2 else p:=(p)div 2;
f:=(f)div 2;until f=1{:111};becareful:=n-2147483647;
if becareful+p>0 then begin aritherror:=true;n:=2147483647-p;end;
if negative then takefraction:=-(n+p)else takefraction:=n+p;end;{:109}
{112:}function takescaled(q:integer;f:scaled):integer;var p:integer;
negative:boolean;n:integer;becareful:integer;begin{110:}
if f>=0 then negative:=false else begin f:=-f;negative:=true;end;
if q<0 then begin q:=-q;negative:=not negative;end;{:110};
if f<65536 then n:=0 else begin n:=f div 65536;f:=f mod 65536;
if q<=2147483647 div n then n:=n*q else begin aritherror:=true;
n:=2147483647;end;end;f:=f+65536;{113:}p:=32768;
if q<1073741824 then repeat if odd(f)then p:=(p+q)div 2 else p:=(p)div 2
;f:=(f)div 2;
until f=1 else repeat if odd(f)then p:=p+(q-p)div 2 else p:=(p)div 2;
f:=(f)div 2;until f=1{:113};becareful:=n-2147483647;
if becareful+p>0 then begin aritherror:=true;n:=2147483647-p;end;
if negative then takescaled:=-(n+p)else takescaled:=n+p;end;{:112}{114:}
function makescaled(p,q:integer):scaled;var f:integer;n:integer;
negative:boolean;becareful:integer;
begin if p>=0 then negative:=false else begin p:=-p;negative:=true;end;
if q<=0 then begin ifdef('DEBUG')if q=0 then confusion(47);
endif('DEBUG')q:=-q;negative:=not negative;end;n:=p div q;p:=p mod q;
if n>=32768 then begin aritherror:=true;
if negative then makescaled:=-2147483647 else makescaled:=2147483647;
end else begin n:=(n-1)*65536;{115:}f:=1;repeat becareful:=p-q;
p:=becareful+p;if p>=0 then f:=f+f+1 else begin f:=f+f;p:=p+q;end;
until f>=65536;becareful:=p-q;if becareful+p>=0 then incr(f){:115};
if negative then makescaled:=-(f+n)else makescaled:=f+n;end;end;{:114}
{116:}function velocity(st,ct,sf,cf:fraction;t:scaled):fraction;
var acc,num,denom:integer;
begin acc:=takefraction(st-(sf div 16),sf-(st div 16));
acc:=takefraction(acc,ct-cf);num:=536870912+takefraction(acc,379625062);
denom:=805306368+takefraction(ct,497706707)+takefraction(cf,307599661);
if t<>65536 then num:=makescaled(num,t);
if num div 4>=denom then velocity:=1073741824 else velocity:=
makefraction(num,denom);end;{:116}{117:}
function abvscd(a,b,c,d:integer):integer;label 10;var q,r:integer;
begin{118:}if a<0 then begin a:=-a;b:=-b;end;if c<0 then begin c:=-c;
d:=-d;end;
if d<=0 then begin if b>=0 then if((a=0)or(b=0))and((c=0)or(d=0))then
begin abvscd:=0;goto 10;end else begin abvscd:=1;goto 10;end;
if d=0 then if a=0 then begin abvscd:=0;goto 10;
end else begin abvscd:=-1;goto 10;end;q:=a;a:=c;c:=q;q:=-b;b:=-d;d:=q;
end else if b<=0 then begin if b<0 then if a>0 then begin abvscd:=-1;
goto 10;end;if c=0 then begin abvscd:=0;goto 10;
end else begin abvscd:=-1;goto 10;end;end{:118};
while true do begin q:=a div d;r:=c div b;
if q<>r then if q>r then begin abvscd:=1;goto 10;
end else begin abvscd:=-1;goto 10;end;q:=a mod d;r:=c mod b;
if r=0 then if q=0 then begin abvscd:=0;goto 10;
end else begin abvscd:=1;goto 10;end;if q=0 then begin abvscd:=-1;
goto 10;end;a:=b;b:=q;c:=d;d:=r;end;10:end;{:117}{119:}
function floorscaled(x:scaled):scaled;var becareful:integer;
begin if x>=0 then floorscaled:=x-(x mod 65536)else begin becareful:=x+1
;floorscaled:=x+((-becareful)mod 65536)-65535;end;end;
function floorunscaled(x:scaled):integer;var becareful:integer;
begin if x>=0 then floorunscaled:=x div 65536 else begin becareful:=x+1;
floorunscaled:=-(1+((-becareful)div 65536));end;end;
function roundunscaled(x:scaled):integer;var becareful:integer;
begin if x>=32768 then roundunscaled:=1+((x-32768)div 65536)else if x>=
-32768 then roundunscaled:=0 else begin becareful:=x+1;
roundunscaled:=-(1+((-becareful-32768)div 65536));end;end;
function roundfraction(x:fraction):scaled;var becareful:integer;
begin if x>=2048 then roundfraction:=1+((x-2048)div 4096)else if x>=
-2048 then roundfraction:=0 else begin becareful:=x+1;
roundfraction:=-(1+((-becareful-2048)div 4096));end;end;{:119}{121:}
function squarert(x:scaled):scaled;var k:smallnumber;y,q:integer;
begin if x<=0 then{122:}
begin if x<0 then begin begin if interaction=3 then;printnl(261);
print(304);end;printscaled(x);print(305);begin helpptr:=2;
helpline[1]:=306;helpline[0]:=307;end;error;end;squarert:=0;end{:122}
else begin k:=23;q:=2;while x<536870912 do begin decr(k);x:=x+x+x+x;end;
if x<1073741824 then y:=0 else begin x:=x-1073741824;y:=1;end;
repeat{123:}x:=x+x;y:=y+y;if x>=1073741824 then begin x:=x-1073741824;
incr(y);end;x:=x+x;y:=y+y-q;q:=q+q;
if x>=1073741824 then begin x:=x-1073741824;incr(y);end;
if y>q then begin y:=y-q;q:=q+2;end else if y<=0 then begin q:=q-2;
y:=y+q;end;decr(k){:123};until k=0;squarert:=(q)div 2;end;end;{:121}
{124:}function pythadd(a,b:integer):integer;label 30;var r:fraction;
big:boolean;begin a:=abs(a);b:=abs(b);if a<b then begin r:=b;b:=a;a:=r;
end;
if a>0 then begin if a<536870912 then big:=false else begin a:=a div 4;
b:=b div 4;big:=true;end;{125:}while true do begin r:=makefraction(b,a);
r:=takefraction(r,r);if r=0 then goto 30;
r:=makefraction(r,1073741824+r);a:=a+takefraction(a+a,r);
b:=takefraction(b,r);end;30:{:125};
if big then if a<536870912 then a:=a+a+a+a else begin aritherror:=true;
a:=2147483647;end;end;pythadd:=a;end;{:124}{126:}
function pythsub(a,b:integer):integer;label 30;var r:fraction;
big:boolean;begin a:=abs(a);b:=abs(b);if a<=b then{128:}
begin if a<b then begin begin if interaction=3 then;printnl(261);
print(308);end;printscaled(a);print(309);printscaled(b);print(305);
begin helpptr:=2;helpline[1]:=306;helpline[0]:=307;end;error;end;a:=0;
end{:128}
else begin if a<1073741824 then big:=false else begin a:=(a)div 2;
b:=(b)div 2;big:=true;end;{127:}
while true do begin r:=makefraction(b,a);r:=takefraction(r,r);
if r=0 then goto 30;r:=makefraction(r,1073741824-r);
a:=a-takefraction(a+a,r);b:=takefraction(b,r);end;30:{:127};
if big then a:=a+a;end;pythsub:=a;end;{:126}{132:}
function mlog(x:scaled):scaled;var y,z:integer;k:integer;
begin if x<=0 then{134:}begin begin if interaction=3 then;printnl(261);
print(310);end;printscaled(x);print(305);begin helpptr:=2;
helpline[1]:=311;helpline[0]:=307;end;error;mlog:=0;end{:134}
else begin y:=1302456860;z:=6581195;while x<1073741824 do begin x:=x+x;
y:=y-93032639;z:=z-48782;end;y:=y+(z div 65536);k:=2;
while x>1073741828 do{133:}begin z:=((x-1)div twotothe[k])+1;
while x<1073741824+z do begin z:=(z+1)div 2;k:=k+1;end;y:=y+speclog[k];
x:=x-z;end{:133};mlog:=y div 8;end;end;{:132}{135:}
function mexp(x:scaled):scaled;var k:smallnumber;y,z:integer;
begin if x>174436200 then begin aritherror:=true;mexp:=2147483647;
end else if x<-197694359 then mexp:=0 else begin if x<=0 then begin z:=
-8*x;y:=1048576;
end else begin if x<=127919879 then z:=1023359037-8*x else z:=8*(
174436200-x);y:=2147483647;end;{136:}k:=1;
while z>0 do begin while z>=speclog[k]do begin z:=z-speclog[k];
y:=y-1-((y-twotothe[k-1])div twotothe[k]);end;incr(k);end{:136};
if x<=127919879 then mexp:=(y+8)div 16 else mexp:=y;end;end;{:135}{139:}
function narg(x,y:integer):angle;var z:angle;t:integer;k:smallnumber;
octant:1..8;begin if x>=0 then octant:=1 else begin x:=-x;octant:=2;end;
if y<0 then begin y:=-y;octant:=octant+2;end;if x<y then begin t:=y;
y:=x;x:=t;octant:=octant+4;end;if x=0 then{140:}
begin begin if interaction=3 then;printnl(261);print(312);end;
begin helpptr:=2;helpline[1]:=313;helpline[0]:=307;end;error;narg:=0;
end{:140}else begin{142:}while x>=536870912 do begin x:=(x)div 2;
y:=(y)div 2;end;z:=0;
if y>0 then begin while x<268435456 do begin x:=x+x;y:=y+y;end;{143:}
k:=0;repeat y:=y+y;incr(k);if y>x then begin z:=z+specatan[k];t:=x;
x:=x+(y div twotothe[k+k]);y:=y-t;end;until k=15;repeat y:=y+y;incr(k);
if y>x then begin z:=z+specatan[k];y:=y-x;end;until k=26{:143};end{:142}
;{141:}case octant of 1:narg:=z;5:narg:=94371840-z;6:narg:=94371840+z;
2:narg:=188743680-z;4:narg:=z-188743680;8:narg:=-z-94371840;
7:narg:=z-94371840;3:narg:=-z;end{:141};end;end;{:139}{145:}
procedure nsincos(z:angle);var k:smallnumber;q:0..7;r:fraction;
x,y,t:integer;begin while z<0 do z:=z+377487360;z:=z mod 377487360;
q:=z div 47185920;z:=z mod 47185920;x:=268435456;y:=x;
if not odd(q)then z:=47185920-z;{147:}k:=1;
while z>0 do begin if z>=specatan[k]then begin z:=z-specatan[k];t:=x;
x:=t+y div twotothe[k];y:=y-t div twotothe[k];end;incr(k);end;
if y<0 then y:=0{:147};{146:}case q of 0:;1:begin t:=x;x:=y;y:=t;end;
2:begin t:=x;x:=-y;y:=t;end;3:x:=-x;4:begin x:=-x;y:=-y;end;
5:begin t:=x;x:=-y;y:=-t;end;6:begin t:=x;x:=y;y:=-t;end;7:y:=-y;
end{:146};r:=pythadd(x,y);ncos:=makefraction(x,r);
nsin:=makefraction(y,r);end;{:145}{149:}procedure newrandoms;
var k:0..54;x:fraction;
begin for k:=0 to 23 do begin x:=randoms[k]-randoms[k+31];
if x<0 then x:=x+268435456;randoms[k]:=x;end;
for k:=24 to 54 do begin x:=randoms[k]-randoms[k-24];
if x<0 then x:=x+268435456;randoms[k]:=x;end;jrandom:=54;end;{:149}
{150:}procedure initrandoms(seed:scaled);var j,jj,k:fraction;i:0..54;
begin j:=abs(seed);while j>=268435456 do j:=(j)div 2;k:=1;
for i:=0 to 54 do begin jj:=k;k:=j-k;j:=jj;if k<0 then k:=k+268435456;
randoms[(i*21)mod 55]:=j;end;newrandoms;newrandoms;newrandoms;end;{:150}
{151:}function unifrand(x:scaled):scaled;var y:scaled;
begin if jrandom=0 then newrandoms else decr(jrandom);
y:=takefraction(abs(x),randoms[jrandom]);
if y=abs(x)then unifrand:=0 else if x>0 then unifrand:=y else unifrand:=
-y;end;{:151}{152:}function normrand:scaled;var x,u,l:integer;
begin repeat repeat if jrandom=0 then newrandoms else decr(jrandom);
x:=takefraction(112429,randoms[jrandom]-134217728);
if jrandom=0 then newrandoms else decr(jrandom);u:=randoms[jrandom];
until abs(x)<u;x:=makefraction(x,u);l:=139548960-mlog(u);
until abvscd(1024,l,x,x)>=0;normrand:=x;end;{:152}{157:}
ifdef('DEBUG')procedure printword(w:memoryword);begin printint(w.int);
printchar(32);printscaled(w.int);printchar(32);
printscaled(w.int div 4096);println;printint(w.hh.lh);printchar(61);
printint(w.hh.b0);printchar(58);printint(w.hh.b1);printchar(59);
printint(w.hh.rh);printchar(32);printint(w.qqqq.b0);printchar(58);
printint(w.qqqq.b1);printchar(58);printint(w.qqqq.b2);printchar(58);
printint(w.qqqq.b3);end;endif('DEBUG'){:157}{162:}{217:}
procedure printcapsule;forward;procedure showtokenlist(p,q:integer;
l,nulltally:integer);label 10;var class,c:smallnumber;r,v:integer;
begin class:=3;tally:=nulltally;
while(p<>0)and(tally<l)do begin if p=q then{646:}
begin firstcount:=tally;trickcount:=tally+1+errorline-halferrorline;
if trickcount<errorline then trickcount:=errorline;end{:646};{218:}c:=9;
if(p<0)or(p>memend)then begin print(492);goto 10;end;
if p<himemmin then{219:}
if mem[p].hh.b1=12 then if mem[p].hh.b0=16 then{220:}
begin if class=0 then printchar(32);v:=mem[p+1].int;
if v<0 then begin if class=17 then printchar(32);printchar(91);
printscaled(v);printchar(93);c:=18;end else begin printscaled(v);c:=0;
end;end{:220}
else if mem[p].hh.b0<>4 then print(495)else begin printchar(34);
slowprint(mem[p+1].int);printchar(34);c:=4;
end else if(mem[p].hh.b1<>11)or(mem[p].hh.b0<1)or(mem[p].hh.b0>19)then
print(495)else begin gpointer:=p;printcapsule;c:=8;end{:219}
else begin r:=mem[p].hh.lh;if r>=9770 then{222:}
begin if r<9920 then begin print(497);r:=r-(9770);
end else if r<10070 then begin print(498);r:=r-(9920);
end else begin print(499);r:=r-(10070);end;printint(r);printchar(41);
c:=8;end{:222}else if r<1 then if r=0 then{221:}
begin if class=17 then printchar(32);print(496);c:=18;end{:221}
else print(493)else begin r:=hash[r].rh;
if(r<0)or(r>=strptr)then print(494)else{223:}
begin c:=charclass[strpool[strstart[r]]];
if c=class then case c of 9:printchar(46);5,6,7,8:;
others:printchar(32)end;slowprint(r);end{:223};end;end{:218};class:=c;
p:=mem[p].hh.rh;end;if p<>0 then print(491);10:end;{:217}{665:}
procedure runaway;begin if scannerstatus>2 then begin printnl(635);
case scannerstatus of 3:print(636);4,5:print(637);6:print(638);end;
println;showtokenlist(mem[memtop-2].hh.rh,0,errorline-10,0);end;end;
{:665}{:162}{163:}function getavail:halfword;var p:halfword;
begin p:=avail;
if p<>0 then avail:=mem[avail].hh.rh else if memend<memmax then begin
incr(memend);p:=memend;end else begin decr(himemmin);p:=himemmin;
if himemmin<=lomemmax then begin runaway;overflow(314,memmax+1);end;end;
mem[p].hh.rh:=0;ifdef('STAT')incr(dynused);endif('STAT')getavail:=p;end;
{:163}{167:}function getnode(s:integer):halfword;label 40,10,20;
var p:halfword;q:halfword;r:integer;t,tt:integer;begin 20:p:=rover;
repeat{169:}q:=p+mem[p].hh.lh;
while(mem[q].hh.rh=262143)do begin t:=mem[q+1].hh.rh;tt:=mem[q+1].hh.lh;
if q=rover then rover:=t;mem[t+1].hh.lh:=tt;mem[tt+1].hh.rh:=t;
q:=q+mem[q].hh.lh;end;r:=q-s;if r>toint(p+1)then{170:}
begin mem[p].hh.lh:=r-p;rover:=p;goto 40;end{:170};
if r=p then if mem[p+1].hh.rh<>p then{171:}begin rover:=mem[p+1].hh.rh;
t:=mem[p+1].hh.lh;mem[rover+1].hh.lh:=t;mem[t+1].hh.rh:=rover;goto 40;
end{:171};mem[p].hh.lh:=q-p{:169};p:=mem[p+1].hh.rh;until p=rover;
if s=1073741824 then begin getnode:=262143;goto 10;end;
if lomemmax+2<himemmin then if lomemmax+2<=262143 then{168:}
begin if himemmin-lomemmax>=1998 then t:=lomemmax+1000 else t:=lomemmax
+1+(himemmin-lomemmax)div 2;if t>262143 then t:=262143;
p:=mem[rover+1].hh.lh;q:=lomemmax;mem[p+1].hh.rh:=q;
mem[rover+1].hh.lh:=q;mem[q+1].hh.rh:=rover;mem[q+1].hh.lh:=p;
mem[q].hh.rh:=262143;mem[q].hh.lh:=t-lomemmax;lomemmax:=t;
mem[lomemmax].hh.rh:=0;mem[lomemmax].hh.lh:=0;rover:=q;goto 20;end{:168}
;overflow(314,memmax+1);40:mem[r].hh.rh:=0;
ifdef('STAT')varused:=varused+s;endif('STAT')getnode:=r;10:end;{:167}
{172:}procedure freenode(p:halfword;s:halfword);var q:halfword;
begin mem[p].hh.lh:=s;mem[p].hh.rh:=262143;q:=mem[rover+1].hh.lh;
mem[p+1].hh.lh:=q;mem[p+1].hh.rh:=rover;mem[rover+1].hh.lh:=p;
mem[q+1].hh.rh:=p;ifdef('STAT')varused:=varused-s;endif('STAT')end;
{:172}{173:}ifdef('INIMF')procedure sortavail;var p,q,r:halfword;
oldrover:halfword;begin p:=getnode(1073741824);p:=mem[rover+1].hh.rh;
mem[rover+1].hh.rh:=262143;oldrover:=rover;while p<>oldrover do{174:}
if p<rover then begin q:=p;p:=mem[q+1].hh.rh;mem[q+1].hh.rh:=rover;
rover:=q;end else begin q:=rover;
while mem[q+1].hh.rh<p do q:=mem[q+1].hh.rh;r:=mem[p+1].hh.rh;
mem[p+1].hh.rh:=mem[q+1].hh.rh;mem[q+1].hh.rh:=p;p:=r;end{:174};
p:=rover;
while mem[p+1].hh.rh<>262143 do begin mem[mem[p+1].hh.rh+1].hh.lh:=p;
p:=mem[p+1].hh.rh;end;mem[p+1].hh.rh:=rover;mem[rover+1].hh.lh:=p;end;
endif('INIMF'){:173}{177:}procedure flushlist(p:halfword);label 30;
var q,r:halfword;begin if p>=himemmin then if p<>memtop then begin r:=p;
repeat q:=r;r:=mem[r].hh.rh;ifdef('STAT')decr(dynused);
endif('STAT')if r<himemmin then goto 30;until r=memtop;
30:mem[q].hh.rh:=avail;avail:=p;end;end;
procedure flushnodelist(p:halfword);var q:halfword;
begin while p<>0 do begin q:=p;p:=mem[p].hh.rh;
if q<himemmin then freenode(q,2)else begin mem[q].hh.rh:=avail;avail:=q;
ifdef('STAT')decr(dynused);endif('STAT')end;end;end;{:177}{180:}
ifdef('DEBUG')procedure checkmem(printlocs:boolean);label 31,32;
var p,q,r:halfword;clobbered:boolean;
begin for p:=0 to lomemmax do freearr[p]:=false;
for p:=himemmin to memend do freearr[p]:=false;{181:}p:=avail;q:=0;
clobbered:=false;
while p<>0 do begin if(p>memend)or(p<himemmin)then clobbered:=true else
if freearr[p]then clobbered:=true;if clobbered then begin printnl(315);
printint(q);goto 31;end;freearr[p]:=true;q:=p;p:=mem[q].hh.rh;end;
31:{:181};{182:}p:=rover;q:=0;clobbered:=false;
repeat if(p>=lomemmax)then clobbered:=true else if(mem[p+1].hh.rh>=
lomemmax)then clobbered:=true else if not((mem[p].hh.rh=262143))or(mem[p
].hh.lh<2)or(p+mem[p].hh.lh>lomemmax)or(mem[mem[p+1].hh.rh+1].hh.lh<>p)
then clobbered:=true;if clobbered then begin printnl(316);printint(q);
goto 32;end;
for q:=p to p+mem[p].hh.lh-1 do begin if freearr[q]then begin printnl(
317);printint(q);goto 32;end;freearr[q]:=true;end;q:=p;
p:=mem[p+1].hh.rh;until p=rover;32:{:182};{183:}p:=0;
while p<=lomemmax do begin if(mem[p].hh.rh=262143)then begin printnl(318
);printint(p);end;while(p<=lomemmax)and not freearr[p]do incr(p);
while(p<=lomemmax)and freearr[p]do incr(p);end{:183};{617:}q:=13;
p:=mem[q].hh.rh;
while p<>13 do begin if mem[p+1].hh.lh<>q then begin printnl(595);
printint(p);end;p:=mem[p+1].hh.rh;r:=19;
repeat if mem[mem[p].hh.lh+1].int>=mem[r+1].int then begin printnl(596);
printint(p);end;r:=mem[p].hh.lh;q:=p;p:=mem[q].hh.rh;until r=0;end{:617}
;if printlocs then{184:}begin printnl(319);
for p:=0 to lomemmax do if not freearr[p]and((p>waslomax)or wasfree[p])
then begin printchar(32);printint(p);end;
for p:=himemmin to memend do if not freearr[p]and((p<washimin)or(p>
wasmemend)or wasfree[p])then begin printchar(32);printint(p);end;
end{:184};for p:=0 to lomemmax do wasfree[p]:=freearr[p];
for p:=himemmin to memend do wasfree[p]:=freearr[p];wasmemend:=memend;
waslomax:=lomemmax;washimin:=himemmin;end;endif('DEBUG'){:180}{185:}
ifdef('DEBUG')procedure searchmem(p:halfword);var q:integer;
begin for q:=0 to lomemmax do begin if mem[q].hh.rh=p then begin printnl
(320);printint(q);printchar(41);end;
if mem[q].hh.lh=p then begin printnl(321);printint(q);printchar(41);end;
end;
for q:=himemmin to memend do begin if mem[q].hh.rh=p then begin printnl(
320);printint(q);printchar(41);end;
if mem[q].hh.lh=p then begin printnl(321);printint(q);printchar(41);end;
end;{209:}
for q:=1 to 9769 do begin if eqtb[q].rh=p then begin printnl(457);
printint(q);printchar(41);end;end{:209};end;endif('DEBUG'){:185}{189:}
procedure printop(c:quarterword);
begin if c<=15 then printtype(c)else case c of 30:print(346);
31:print(347);32:print(348);33:print(349);34:print(350);35:print(351);
36:print(352);37:print(353);38:print(354);39:print(355);40:print(356);
41:print(357);42:print(358);43:print(359);44:print(360);45:print(361);
46:print(362);47:print(363);48:print(364);49:print(365);50:print(366);
51:print(367);52:print(368);53:print(369);54:print(370);55:print(371);
56:print(372);57:print(373);58:print(374);59:print(375);60:print(376);
61:print(377);62:print(378);63:print(379);64:print(380);65:print(381);
66:print(382);67:print(383);68:print(384);69:printchar(43);
70:printchar(45);71:printchar(42);72:printchar(47);73:print(385);
74:print(309);75:print(386);76:print(387);77:printchar(60);
78:print(388);79:printchar(62);80:print(389);81:printchar(61);
82:print(390);83:print(38);84:print(391);85:print(392);86:print(393);
87:print(394);88:print(395);89:print(396);90:print(397);91:print(398);
92:print(399);94:print(400);95:print(401);96:print(402);97:print(403);
98:print(404);99:print(405);100:print(406);others:print(407)end;end;
{:189}{194:}procedure fixdateandtime;
begin dateandtime(internal[17],internal[16],internal[15],internal[14]);
internal[17]:=internal[17]*65536;internal[16]:=internal[16]*65536;
internal[15]:=internal[15]*65536;internal[14]:=internal[14]*65536;end;
{:194}{205:}function idlookup(j,l:integer):halfword;label 40;
var h:integer;p:halfword;k:halfword;begin if l=1 then{206:}
begin p:=buffer[j]+1;hash[p].rh:=p-1;goto 40;end{:206};{208:}
h:=buffer[j];for k:=j+1 to j+l-1 do begin h:=h+h+buffer[k];
while h>=7919 do h:=h-7919;end{:208};p:=h+257;
while true do begin if hash[p].rh>0 then if(strstart[hash[p].rh+1]-
strstart[hash[p].rh])=l then if streqbuf(hash[p].rh,j)then goto 40;
if hash[p].lh=0 then{207:}
begin if hash[p].rh>0 then begin repeat if(hashused=257)then overflow(
456,9500);decr(hashused);until hash[hashused].rh=0;hash[p].lh:=hashused;
p:=hashused;end;
begin if poolptr+l>maxpoolptr then begin if poolptr+l>poolsize then
overflow(257,poolsize-initpoolptr);maxpoolptr:=poolptr+l;end;end;
for k:=j to j+l-1 do begin strpool[poolptr]:=buffer[k];incr(poolptr);
end;hash[p].rh:=makestring;strref[hash[p].rh]:=127;
ifdef('STAT')incr(stcount);endif('STAT')goto 40;end{:207};p:=hash[p].lh;
end;40:idlookup:=p;end;{:205}{210:}
ifdef('INIMF')procedure primitive(s:strnumber;c:halfword;o:halfword);
var k:poolpointer;j:smallnumber;l:smallnumber;begin k:=strstart[s];
l:=strstart[s+1]-k;for j:=0 to l-1 do buffer[j]:=strpool[k+j];
cursym:=idlookup(0,l);if s>=256 then begin flushstring(strptr-1);
hash[cursym].rh:=s;end;eqtb[cursym].lh:=c;eqtb[cursym].rh:=o;end;
endif('INIMF'){:210}{215:}function newnumtok(v:scaled):halfword;
var p:halfword;begin p:=getnode(2);mem[p+1].int:=v;mem[p].hh.b0:=16;
mem[p].hh.b1:=12;newnumtok:=p;end;{:215}{216:}procedure tokenrecycle;
forward;procedure flushtokenlist(p:halfword);var q:halfword;
begin while p<>0 do begin q:=p;p:=mem[p].hh.rh;
if q>=himemmin then begin mem[q].hh.rh:=avail;avail:=q;
ifdef('STAT')decr(dynused);
endif('STAT')end else begin case mem[q].hh.b0 of 1,2,16:;
4:begin if strref[mem[q+1].int]<127 then if strref[mem[q+1].int]>1 then
decr(strref[mem[q+1].int])else flushstring(mem[q+1].int);end;
3,5,7,12,10,6,9,8,11,14,13,17,18,19:begin gpointer:=q;tokenrecycle;end;
others:confusion(490)end;freenode(q,2);end;end;end;{:216}{226:}
procedure deletemacref(p:halfword);
begin if mem[p].hh.lh=0 then flushtokenlist(p)else decr(mem[p].hh.lh);
end;{:226}{227:}{625:}procedure printcmdmod(c,m:integer);
begin case c of{212:}18:print(461);77:print(460);59:print(463);
72:print(462);79:print(459);32:print(464);81:print(58);82:print(44);
57:print(465);19:print(466);60:print(467);27:print(468);11:print(469);
80:print(458);84:print(452);26:print(470);6:print(471);9:print(472);
70:print(473);73:print(474);13:print(475);46:print(123);63:print(91);
14:print(476);15:print(477);69:print(478);28:print(479);47:print(407);
24:print(480);7:printchar(92);65:print(125);64:print(93);12:print(481);
8:print(482);83:print(59);17:print(483);78:print(484);74:print(485);
35:print(486);58:print(487);71:print(488);75:print(489);{:212}{684:}
16:if m<=2 then if m=1 then print(652)else if m<1 then print(453)else
print(653)else if m=53 then print(654)else if m=44 then print(655)else
print(656);
4:if m<=1 then if m=1 then print(659)else print(454)else if m=9770 then
print(657)else print(658);{:684}{689:}61:case m of 1:print(661);
2:printchar(64);3:print(662);others:print(660)end;{:689}{696:}
56:if m>=9770 then if m=9770 then print(673)else if m=9920 then print(
674)else print(675)else if m<2 then print(676)else if m=2 then print(677
)else print(678);{:696}{710:}3:if m=0 then print(688)else print(614);
{:710}{741:}1,2:case m of 1:print(715);2:print(451);3:print(716);
others:print(717)end;{:741}{894:}
33,34,37,55,45,50,36,43,54,48,51,52:printop(m);{:894}{1014:}
30:printtype(m);{:1014}{1019:}85:if m=0 then print(909)else print(910);
{:1019}{1025:}23:case m of 0:print(271);1:print(272);2:print(273);
others:print(916)end;{:1025}{1028:}
21:if m=0 then print(917)else print(918);{:1028}{1038:}
22:case m of 0:print(932);1:print(933);2:print(934);3:print(935);
others:print(936)end;{:1038}{1043:}
31,62:begin if c=31 then print(939)else print(940);print(941);
slowprint(hash[m].rh);end;41:if m=0 then print(942)else print(943);
10:print(944);53,44,49:begin printcmdmod(16,c);print(945);println;
showtokenlist(mem[mem[m].hh.rh].hh.rh,0,1000,0);end;5:print(946);
40:slowprint(intname[m]);{:1043}{1053:}
68:if m=1 then print(953)else if m=0 then print(954)else print(955);
66:if m=6 then print(956)else print(957);
67:if m=0 then print(958)else print(959);{:1053}{1080:}
25:if m<1 then print(989)else if m=1 then print(990)else print(991);
{:1080}{1102:}20:case m of 0:print(1001);1:print(1002);2:print(1003);
3:print(1004);others:print(1005)end;{:1102}{1109:}
76:case m of 0:print(1023);1:print(1024);2:print(1026);3:print(1028);
5:print(1025);6:print(1027);7:print(1029);11:print(1030);
others:print(1031)end;{:1109}{1180:}
29:if m=16 then print(1056)else print(1055);{:1180}others:print(600)end;
end;{:625}procedure showmacro(p:halfword;q,l:integer);label 10;
var r:halfword;begin p:=mem[p].hh.rh;
while mem[p].hh.lh>7 do begin r:=mem[p].hh.rh;mem[p].hh.rh:=0;
showtokenlist(p,0,l,0);mem[p].hh.rh:=r;p:=r;
if l>0 then l:=l-tally else goto 10;end;tally:=0;
case mem[p].hh.lh of 0:print(500);1,2,3:begin printchar(60);
printcmdmod(56,mem[p].hh.lh);print(501);end;4:print(502);5:print(503);
6:print(504);7:print(505);end;showtokenlist(mem[p].hh.rh,q,l-tally,0);
10:end;{:227}{232:}procedure initbignode(p:halfword);var q:halfword;
s:smallnumber;begin s:=bignodesize[mem[p].hh.b0];q:=getnode(s);
repeat s:=s-2;{586:}begin mem[q+s].hh.b0:=19;serialno:=serialno+64;
mem[q+s+1].int:=serialno;end{:586};mem[q+s].hh.b1:=(s)div 2+5;
mem[q+s].hh.rh:=0;until s=0;mem[q].hh.rh:=p;mem[p+1].int:=q;end;{:232}
{233:}function idtransform:halfword;var p,q,r:halfword;
begin p:=getnode(2);mem[p].hh.b0:=13;mem[p].hh.b1:=11;mem[p+1].int:=0;
initbignode(p);q:=mem[p+1].int;r:=q+12;repeat r:=r-2;mem[r].hh.b0:=16;
mem[r+1].int:=0;until r=q;mem[q+5].int:=65536;mem[q+11].int:=65536;
idtransform:=p;end;{:233}{234:}procedure newroot(x:halfword);
var p:halfword;begin p:=getnode(2);mem[p].hh.b0:=0;mem[p].hh.b1:=0;
mem[p].hh.rh:=x;eqtb[x].rh:=p;end;{:234}{235:}
procedure printvariablename(p:halfword);label 40,10;var q:halfword;
r:halfword;begin while mem[p].hh.b1>=5 do{237:}
begin case mem[p].hh.b1 of 5:printchar(120);6:printchar(121);
7:print(508);8:print(509);9:print(510);10:print(511);
11:begin print(512);printint(p-0);goto 10;end;end;print(513);
p:=mem[p-2*(mem[p].hh.b1-5)].hh.rh;end{:237};q:=0;
while mem[p].hh.b1>1 do{236:}
begin if mem[p].hh.b1=3 then begin r:=newnumtok(mem[p+2].int);
repeat p:=mem[p].hh.rh;until mem[p].hh.b1=4;
end else if mem[p].hh.b1=2 then begin p:=mem[p].hh.rh;goto 40;
end else begin if mem[p].hh.b1<>4 then confusion(507);r:=getavail;
mem[r].hh.lh:=mem[p+2].hh.lh;end;mem[r].hh.rh:=q;q:=r;
40:p:=mem[p+2].hh.rh;end{:236};r:=getavail;mem[r].hh.lh:=mem[p].hh.rh;
mem[r].hh.rh:=q;if mem[p].hh.b1=1 then print(506);
showtokenlist(r,0,2147483647,tally);flushtokenlist(r);10:end;{:235}
{238:}function interesting(p:halfword):boolean;var t:smallnumber;
begin if internal[3]>0 then interesting:=true else begin t:=mem[p].hh.b1
;if t>=5 then if t<>11 then t:=mem[mem[p-2*(t-5)].hh.rh].hh.b1;
interesting:=(t<>11);end;end;{:238}{239:}
function newstructure(p:halfword):halfword;var q,r:halfword;
begin case mem[p].hh.b1 of 0:begin q:=mem[p].hh.rh;r:=getnode(2);
eqtb[q].rh:=r;end;3:{240:}begin q:=p;repeat q:=mem[q].hh.rh;
until mem[q].hh.b1=4;q:=mem[q+2].hh.rh;r:=q+1;repeat q:=r;
r:=mem[r].hh.rh;until r=p;r:=getnode(3);mem[q].hh.rh:=r;
mem[r+2].int:=mem[p+2].int;end{:240};4:{241:}begin q:=mem[p+2].hh.rh;
r:=mem[q+1].hh.lh;repeat q:=r;r:=mem[r].hh.rh;until r=p;r:=getnode(3);
mem[q].hh.rh:=r;mem[r+2]:=mem[p+2];
if mem[p+2].hh.lh=0 then begin q:=mem[p+2].hh.rh+1;
while mem[q].hh.rh<>p do q:=mem[q].hh.rh;mem[q].hh.rh:=r;end;end{:241};
others:confusion(514)end;mem[r].hh.rh:=mem[p].hh.rh;mem[r].hh.b0:=21;
mem[r].hh.b1:=mem[p].hh.b1;mem[r+1].hh.lh:=p;mem[p].hh.b1:=2;
q:=getnode(3);mem[p].hh.rh:=q;mem[r+1].hh.rh:=q;mem[q+2].hh.rh:=r;
mem[q].hh.b0:=0;mem[q].hh.b1:=4;mem[q].hh.rh:=17;mem[q+2].hh.lh:=0;
newstructure:=r;end;{:239}{242:}
function findvariable(t:halfword):halfword;label 10;
var p,q,r,s:halfword;pp,qq,rr,ss:halfword;n:integer;saveword:memoryword;
begin p:=mem[t].hh.lh;t:=mem[t].hh.rh;
if eqtb[p].lh mod 86<>41 then begin findvariable:=0;goto 10;end;
if eqtb[p].rh=0 then newroot(p);p:=eqtb[p].rh;pp:=p;
while t<>0 do begin{243:}
if mem[pp].hh.b0<>21 then begin if mem[pp].hh.b0>21 then begin
findvariable:=0;goto 10;end;ss:=newstructure(pp);if p=pp then p:=ss;
pp:=ss;end;if mem[p].hh.b0<>21 then p:=newstructure(p){:243};
if t<himemmin then{244:}begin n:=mem[t+1].int;
pp:=mem[mem[pp+1].hh.lh].hh.rh;q:=mem[mem[p+1].hh.lh].hh.rh;
saveword:=mem[q+2];mem[q+2].int:=2147483647;s:=p+1;repeat r:=s;
s:=mem[s].hh.rh;until n<=mem[s+2].int;
if n=mem[s+2].int then p:=s else begin p:=getnode(3);mem[r].hh.rh:=p;
mem[p].hh.rh:=s;mem[p+2].int:=n;mem[p].hh.b1:=3;mem[p].hh.b0:=0;end;
mem[q+2]:=saveword;end{:244}else{245:}begin n:=mem[t].hh.lh;
ss:=mem[pp+1].hh.lh;repeat rr:=ss;ss:=mem[ss].hh.rh;
until n<=mem[ss+2].hh.lh;if n<mem[ss+2].hh.lh then begin qq:=getnode(3);
mem[rr].hh.rh:=qq;mem[qq].hh.rh:=ss;mem[qq+2].hh.lh:=n;mem[qq].hh.b1:=4;
mem[qq].hh.b0:=0;mem[qq+2].hh.rh:=pp;ss:=qq;end;
if p=pp then begin p:=ss;pp:=ss;end else begin pp:=ss;s:=mem[p+1].hh.lh;
repeat r:=s;s:=mem[s].hh.rh;until n<=mem[s+2].hh.lh;
if n=mem[s+2].hh.lh then p:=s else begin q:=getnode(3);mem[r].hh.rh:=q;
mem[q].hh.rh:=s;mem[q+2].hh.lh:=n;mem[q].hh.b1:=4;mem[q].hh.b0:=0;
mem[q+2].hh.rh:=p;p:=q;end;end;end{:245};t:=mem[t].hh.rh;end;
if mem[pp].hh.b0>=21 then if mem[pp].hh.b0=21 then pp:=mem[pp+1].hh.lh
else begin findvariable:=0;goto 10;end;
if mem[p].hh.b0=21 then p:=mem[p+1].hh.lh;
if mem[p].hh.b0=0 then begin if mem[pp].hh.b0=0 then begin mem[pp].hh.b0
:=15;mem[pp+1].int:=0;end;mem[p].hh.b0:=mem[pp].hh.b0;mem[p+1].int:=0;
end;findvariable:=p;10:end;{:242}{246:}{257:}
procedure printpath(h:halfword;s:strnumber;nuline:boolean);label 30,31;
var p,q:halfword;begin printdiagnostic(516,s,nuline);println;p:=h;
repeat q:=mem[p].hh.rh;if(p=0)or(q=0)then begin printnl(259);goto 30;
end;{258:}printtwo(mem[p+1].int,mem[p+2].int);
case mem[p].hh.b1 of 0:begin if mem[p].hh.b0=4 then print(517);
if(mem[q].hh.b0<>0)or(q<>h)then q:=0;goto 31;end;1:{261:}
begin print(523);printtwo(mem[p+5].int,mem[p+6].int);print(522);
if mem[q].hh.b0<>1 then print(524)else printtwo(mem[q+3].int,mem[q+4].
int);goto 31;end{:261};4:{262:}
if(mem[p].hh.b0<>1)and(mem[p].hh.b0<>4)then print(517){:262};3,2:{263:}
begin if mem[p].hh.b0=4 then print(524);
if mem[p].hh.b1=3 then begin print(520);printscaled(mem[p+5].int);
end else begin nsincos(mem[p+5].int);printchar(123);printscaled(ncos);
printchar(44);printscaled(nsin);end;printchar(125);end{:263};
others:print(259)end;
if mem[q].hh.b0<=1 then print(518)else if(mem[p+6].int<>65536)or(mem[q+4
].int<>65536)then{260:}begin print(521);
if mem[p+6].int<0 then print(463);printscaled(abs(mem[p+6].int));
if mem[p+6].int<>mem[q+4].int then begin print(522);
if mem[q+4].int<0 then print(463);printscaled(abs(mem[q+4].int));end;
end{:260};31:{:258};p:=q;if(p<>h)or(mem[h].hh.b0<>0)then{259:}
begin printnl(519);if mem[p].hh.b0=2 then begin nsincos(mem[p+3].int);
printchar(123);printscaled(ncos);printchar(44);printscaled(nsin);
printchar(125);end else if mem[p].hh.b0=3 then begin print(520);
printscaled(mem[p+3].int);printchar(125);end;end{:259};until p=h;
if mem[h].hh.b0<>0 then print(384);30:enddiagnostic(true);end;{:257}
{332:}{333:}procedure printweight(q:halfword;xoff:integer);
var w,m:integer;d:integer;begin d:=mem[q].hh.lh;w:=d mod 8;
m:=(d div 8)-mem[curedges+3].hh.lh;
if fileoffset>maxprintline-9 then printnl(32)else printchar(32);
printint(m+xoff);while w>4 do begin printchar(43);decr(w);end;
while w<4 do begin printchar(45);incr(w);end;end;{:333}
procedure printedges(s:strnumber;nuline:boolean;xoff,yoff:integer);
var p,q,r:halfword;n:integer;begin printdiagnostic(531,s,nuline);
p:=mem[curedges].hh.lh;n:=mem[curedges+1].hh.rh-4096;
while p<>curedges do begin q:=mem[p+1].hh.lh;r:=mem[p+1].hh.rh;
if(q>1)or(r<>memtop)then begin printnl(532);printint(n+yoff);
printchar(58);while q>1 do begin printweight(q,xoff);q:=mem[q].hh.rh;
end;print(533);while r<>memtop do begin printweight(r,xoff);
r:=mem[r].hh.rh;end;end;p:=mem[p].hh.lh;decr(n);end;enddiagnostic(true);
end;{:332}{388:}procedure unskew(x,y:scaled;octant:smallnumber);
begin case octant of 1:begin curx:=x+y;cury:=y;end;5:begin curx:=y;
cury:=x+y;end;6:begin curx:=-y;cury:=x+y;end;2:begin curx:=-x-y;cury:=y;
end;4:begin curx:=-x-y;cury:=-y;end;8:begin curx:=-y;cury:=-x-y;end;
7:begin curx:=y;cury:=-x-y;end;3:begin curx:=x+y;cury:=-y;end;end;end;
{:388}{473:}procedure printpen(p:halfword;s:strnumber;nuline:boolean);
var nothingprinted:boolean;k:1..8;h:halfword;m,n:integer;w,ww:halfword;
begin printdiagnostic(568,s,nuline);nothingprinted:=true;println;
for k:=1 to 8 do begin octant:=octantcode[k];h:=p+octant;
n:=mem[h].hh.lh;w:=mem[h].hh.rh;if not odd(k)then w:=mem[w].hh.lh;
for m:=1 to n+1 do begin if odd(k)then ww:=mem[w].hh.rh else ww:=mem[w].
hh.lh;
if(mem[ww+1].int<>mem[w+1].int)or(mem[ww+2].int<>mem[w+2].int)then{474:}
begin if nothingprinted then nothingprinted:=false else printnl(570);
unskew(mem[ww+1].int,mem[ww+2].int,octant);printtwo(curx,cury);end{:474}
;w:=ww;end;end;if nothingprinted then begin w:=mem[p+1].hh.rh;
printtwo(mem[w+1].int+mem[w+2].int,mem[w+2].int);end;printnl(569);
enddiagnostic(true);end;{:473}{589:}
procedure printdependency(p:halfword;t:smallnumber);label 10;
var v:integer;pp,q:halfword;begin pp:=p;
while true do begin v:=abs(mem[p+1].int);q:=mem[p].hh.lh;
if q=0 then begin if(v<>0)or(p=pp)then begin if mem[p+1].int>0 then if p
<>pp then printchar(43);printscaled(mem[p+1].int);end;goto 10;end;{590:}
if mem[p+1].int<0 then printchar(45)else if p<>pp then printchar(43);
if t=17 then v:=roundfraction(v);if v<>65536 then printscaled(v){:590};
if mem[q].hh.b0<>19 then confusion(586);printvariablename(q);
v:=mem[q+1].int mod 64;while v>0 do begin print(587);v:=v-2;end;
p:=mem[p].hh.rh;end;10:end;{:589}{801:}{805:}
procedure printdp(t:smallnumber;p:halfword;verbosity:smallnumber);
var q:halfword;begin q:=mem[p].hh.rh;
if(mem[q].hh.lh=0)or(verbosity>0)then printdependency(p,t)else print(761
);end;{:805}{799:}function stashcurexp:halfword;var p:halfword;
begin case curtype of 3,5,7,12,10,13,14,17,18,19:p:=curexp;
others:begin p:=getnode(2);mem[p].hh.b1:=11;mem[p].hh.b0:=curtype;
mem[p+1].int:=curexp;end end;curtype:=1;mem[p].hh.rh:=1;stashcurexp:=p;
end;{:799}{800:}procedure unstashcurexp(p:halfword);
begin curtype:=mem[p].hh.b0;
case curtype of 3,5,7,12,10,13,14,17,18,19:curexp:=p;
others:begin curexp:=mem[p+1].int;freenode(p,2);end end;end;{:800}
procedure printexp(p:halfword;verbosity:smallnumber);
var restorecurexp:boolean;t:smallnumber;v:integer;q:halfword;
begin if p<>0 then restorecurexp:=false else begin p:=stashcurexp;
restorecurexp:=true;end;t:=mem[p].hh.b0;
if t<17 then v:=mem[p+1].int else if t<19 then v:=mem[p+1].hh.rh;{802:}
case t of 1:print(322);2:if v=30 then print(346)else print(347);
3,5,7,12,10,15:{806:}begin printtype(t);
if v<>0 then begin printchar(32);
while(mem[v].hh.b1=11)and(v<>p)do v:=mem[v+1].int;printvariablename(v);
end;end{:806};4:begin printchar(34);slowprint(v);printchar(34);end;
6,8,9,11:{804:}
if verbosity<=1 then printtype(t)else begin if selector=3 then if
internal[13]<=0 then begin selector:=1;printtype(t);print(759);
selector:=3;end;case t of 6:printpen(v,283,false);
8:printpath(v,760,false);9:printpath(v,283,false);11:begin curedges:=v;
printedges(283,false,0,0);end;end;end{:804};
13,14:if v=0 then printtype(t)else{803:}begin printchar(40);
q:=v+bignodesize[t];
repeat if mem[v].hh.b0=16 then printscaled(mem[v+1].int)else if mem[v].
hh.b0=19 then printvariablename(v)else printdp(mem[v].hh.b0,mem[v+1].hh.
rh,verbosity);v:=v+2;if v<>q then printchar(44);until v=q;printchar(41);
end{:803};16:printscaled(v);17,18:printdp(t,v,verbosity);
19:printvariablename(p);others:confusion(758)end{:802};
if restorecurexp then unstashcurexp(p);end;{:801}{807:}
procedure disperr(p:halfword;s:strnumber);begin if interaction=3 then;
printnl(762);printexp(p,1);if s<>283 then begin printnl(261);print(s);
end;end;{:807}{594:}function pplusfq(p:halfword;f:integer;q:halfword;
t,tt:smallnumber):halfword;label 30;var pp,qq:halfword;r,s:halfword;
threshold:integer;v:integer;
begin if t=17 then threshold:=2685 else threshold:=8;r:=memtop-1;
pp:=mem[p].hh.lh;qq:=mem[q].hh.lh;
while true do if pp=qq then if pp=0 then goto 30 else{595:}
begin if tt=17 then v:=mem[p+1].int+takefraction(f,mem[q+1].int)else v:=
mem[p+1].int+takescaled(f,mem[q+1].int);mem[p+1].int:=v;s:=p;
p:=mem[p].hh.rh;
if abs(v)<threshold then freenode(s,2)else begin if abs(v)>=626349397
then if watchcoefs then begin mem[qq].hh.b0:=0;fixneeded:=true;end;
mem[r].hh.rh:=s;r:=s;end;pp:=mem[p].hh.lh;q:=mem[q].hh.rh;
qq:=mem[q].hh.lh;end{:595}else if mem[pp+1].int<mem[qq+1].int then{596:}
begin if tt=17 then v:=takefraction(f,mem[q+1].int)else v:=takescaled(f,
mem[q+1].int);if abs(v)>(threshold)div 2 then begin s:=getnode(2);
mem[s].hh.lh:=qq;mem[s+1].int:=v;
if abs(v)>=626349397 then if watchcoefs then begin mem[qq].hh.b0:=0;
fixneeded:=true;end;mem[r].hh.rh:=s;r:=s;end;q:=mem[q].hh.rh;
qq:=mem[q].hh.lh;end{:596}else begin mem[r].hh.rh:=p;r:=p;
p:=mem[p].hh.rh;pp:=mem[p].hh.lh;end;
30:if t=17 then mem[p+1].int:=slowadd(mem[p+1].int,takefraction(mem[q+1]
.int,f))else mem[p+1].int:=slowadd(mem[p+1].int,takescaled(mem[q+1].int,
f));mem[r].hh.rh:=p;depfinal:=p;pplusfq:=mem[memtop-1].hh.rh;end;{:594}
{600:}function poverv(p:halfword;v:scaled;t0,t1:smallnumber):halfword;
var r,s:halfword;w:integer;threshold:integer;scalingdown:boolean;
begin if t0<>t1 then scalingdown:=true else scalingdown:=false;
if t1=17 then threshold:=1342 else threshold:=4;r:=memtop-1;
while mem[p].hh.lh<>0 do begin if scalingdown then if abs(v)<524288 then
w:=makescaled(mem[p+1].int,v*4096)else w:=makescaled(roundfraction(mem[p
+1].int),v)else w:=makescaled(mem[p+1].int,v);
if abs(w)<=threshold then begin s:=mem[p].hh.rh;freenode(p,2);p:=s;
end else begin if abs(w)>=626349397 then begin fixneeded:=true;
mem[mem[p].hh.lh].hh.b0:=0;end;mem[r].hh.rh:=p;r:=p;mem[p+1].int:=w;
p:=mem[p].hh.rh;end;end;mem[r].hh.rh:=p;
mem[p+1].int:=makescaled(mem[p+1].int,v);poverv:=mem[memtop-1].hh.rh;
end;{:600}{602:}procedure valtoobig(x:scaled);
begin if internal[40]>0 then begin begin if interaction=3 then;
printnl(261);print(588);end;printscaled(x);printchar(41);
begin helpptr:=4;helpline[3]:=589;helpline[2]:=590;helpline[1]:=591;
helpline[0]:=592;end;error;end;end;{:602}{603:}
procedure makeknown(p,q:halfword);var t:17..18;
begin mem[mem[q].hh.rh+1].hh.lh:=mem[p+1].hh.lh;
mem[mem[p+1].hh.lh].hh.rh:=mem[q].hh.rh;t:=mem[p].hh.b0;
mem[p].hh.b0:=16;mem[p+1].int:=mem[q+1].int;freenode(q,2);
if abs(mem[p+1].int)>=268435456 then valtoobig(mem[p+1].int);
if internal[2]>0 then if interesting(p)then begin begindiagnostic;
printnl(593);printvariablename(p);printchar(61);
printscaled(mem[p+1].int);enddiagnostic(false);end;
if curexp=p then if curtype=t then begin curtype:=16;
curexp:=mem[p+1].int;freenode(p,2);end;end;{:603}{604:}
procedure fixdependencies;label 30;var p,q,r,s,t:halfword;x:halfword;
begin r:=mem[13].hh.rh;s:=0;while r<>13 do begin t:=r;{605:}r:=t+1;
while true do begin q:=mem[r].hh.rh;x:=mem[q].hh.lh;if x=0 then goto 30;
if mem[x].hh.b0<=1 then begin if mem[x].hh.b0<1 then begin p:=getavail;
mem[p].hh.rh:=s;s:=p;mem[s].hh.lh:=x;mem[x].hh.b0:=1;end;
mem[q+1].int:=mem[q+1].int div 4;
if mem[q+1].int=0 then begin mem[r].hh.rh:=mem[q].hh.rh;freenode(q,2);
q:=r;end;end;r:=q;end;30:{:605};r:=mem[q].hh.rh;
if q=mem[t+1].hh.rh then makeknown(t,q);end;
while s<>0 do begin p:=mem[s].hh.rh;x:=mem[s].hh.lh;
begin mem[s].hh.rh:=avail;avail:=s;ifdef('STAT')decr(dynused);
endif('STAT')end;s:=p;mem[x].hh.b0:=19;mem[x+1].int:=mem[x+1].int+2;end;
fixneeded:=false;end;{:604}{268:}procedure tossknotlist(p:halfword);
var q:halfword;r:halfword;begin q:=p;repeat r:=mem[q].hh.rh;
freenode(q,7);q:=r;until q=p;end;{:268}{385:}
procedure tossedges(h:halfword);var p,q:halfword;begin q:=mem[h].hh.rh;
while q<>h do begin flushlist(mem[q+1].hh.rh);
if mem[q+1].hh.lh>1 then flushlist(mem[q+1].hh.lh);p:=q;q:=mem[q].hh.rh;
freenode(p,2);end;freenode(h,6);end;{:385}{487:}
procedure tosspen(p:halfword);var k:1..8;w,ww:halfword;
begin if p<>3 then begin for k:=1 to 8 do begin w:=mem[p+k].hh.rh;
repeat ww:=mem[w].hh.rh;freenode(w,3);w:=ww;until w=mem[p+k].hh.rh;end;
freenode(p,10);end;end;{:487}{620:}procedure ringdelete(p:halfword);
var q:halfword;begin q:=mem[p+1].int;
if q<>0 then if q<>p then begin while mem[q+1].int<>p do q:=mem[q+1].int
;mem[q+1].int:=mem[p+1].int;end;end;{:620}{809:}
procedure recyclevalue(p:halfword);label 30;var t:smallnumber;v:integer;
vv:integer;q,r,s,pp:halfword;begin t:=mem[p].hh.b0;
if t<17 then v:=mem[p+1].int;case t of 0,1,2,16,15:;
3,5,7,12,10:ringdelete(p);
4:begin if strref[v]<127 then if strref[v]>1 then decr(strref[v])else
flushstring(v);end;
6:if mem[v].hh.lh=0 then tosspen(v)else decr(mem[v].hh.lh);
9,8:tossknotlist(v);11:tossedges(v);14,13:{810:}
if v<>0 then begin q:=v+bignodesize[t];repeat q:=q-2;recyclevalue(q);
until q=v;freenode(v,bignodesize[t]);end{:810};17,18:{811:}
begin q:=mem[p+1].hh.rh;while mem[q].hh.lh<>0 do q:=mem[q].hh.rh;
mem[mem[p+1].hh.lh].hh.rh:=mem[q].hh.rh;
mem[mem[q].hh.rh+1].hh.lh:=mem[p+1].hh.lh;mem[q].hh.rh:=0;
flushnodelist(mem[p+1].hh.rh);end{:811};19:{812:}begin maxc[17]:=0;
maxc[18]:=0;maxlink[17]:=0;maxlink[18]:=0;q:=mem[13].hh.rh;
while q<>13 do begin s:=q+1;while true do begin r:=mem[s].hh.rh;
if mem[r].hh.lh=0 then goto 30;
if mem[r].hh.lh<>p then s:=r else begin t:=mem[q].hh.b0;
mem[s].hh.rh:=mem[r].hh.rh;mem[r].hh.lh:=q;
if abs(mem[r+1].int)>maxc[t]then{814:}
begin if maxc[t]>0 then begin mem[maxptr[t]].hh.rh:=maxlink[t];
maxlink[t]:=maxptr[t];end;maxc[t]:=abs(mem[r+1].int);maxptr[t]:=r;
end{:814}else begin mem[r].hh.rh:=maxlink[t];maxlink[t]:=r;end;end;end;
30:q:=mem[r].hh.rh;end;if(maxc[17]>0)or(maxc[18]>0)then{815:}
begin if(maxc[17]>=268435456)or(maxc[17]div 4096>=maxc[18])then t:=17
else t:=18;{816:}s:=maxptr[t];pp:=mem[s].hh.lh;v:=mem[s+1].int;
if t=17 then mem[s+1].int:=-268435456 else mem[s+1].int:=-65536;
r:=mem[pp+1].hh.rh;mem[s].hh.rh:=r;
while mem[r].hh.lh<>0 do r:=mem[r].hh.rh;q:=mem[r].hh.rh;
mem[r].hh.rh:=0;mem[q+1].hh.lh:=mem[pp+1].hh.lh;
mem[mem[pp+1].hh.lh].hh.rh:=q;begin mem[pp].hh.b0:=19;
serialno:=serialno+64;mem[pp+1].int:=serialno;end;
if curexp=pp then if curtype=t then curtype:=19;
if internal[2]>0 then{817:}if interesting(p)then begin begindiagnostic;
printnl(764);if v>0 then printchar(45);
if t=17 then vv:=roundfraction(maxc[17])else vv:=maxc[18];
if vv<>65536 then printscaled(vv);printvariablename(p);
while mem[p+1].int mod 64>0 do begin print(587);
mem[p+1].int:=mem[p+1].int-2;end;
if t=17 then printchar(61)else print(765);printdependency(s,t);
enddiagnostic(false);end{:817}{:816};t:=35-t;
if maxc[t]>0 then begin mem[maxptr[t]].hh.rh:=maxlink[t];
maxlink[t]:=maxptr[t];end;if t<>17 then{818:}
for t:=17 to 18 do begin r:=maxlink[t];
while r<>0 do begin q:=mem[r].hh.lh;
mem[q+1].hh.rh:=pplusfq(mem[q+1].hh.rh,makefraction(mem[r+1].int,-v),s,t
,17);if mem[q+1].hh.rh=depfinal then makeknown(q,depfinal);q:=r;
r:=mem[r].hh.rh;freenode(q,2);end;end{:818}else{819:}
for t:=17 to 18 do begin r:=maxlink[t];
while r<>0 do begin q:=mem[r].hh.lh;
if t=17 then begin if curexp=q then if curtype=17 then curtype:=18;
mem[q+1].hh.rh:=poverv(mem[q+1].hh.rh,65536,17,18);mem[q].hh.b0:=18;
mem[r+1].int:=roundfraction(mem[r+1].int);end;
mem[q+1].hh.rh:=pplusfq(mem[q+1].hh.rh,makescaled(mem[r+1].int,-v),s,18,
18);if mem[q+1].hh.rh=depfinal then makeknown(q,depfinal);q:=r;
r:=mem[r].hh.rh;freenode(q,2);end;end{:819};flushnodelist(s);
if fixneeded then fixdependencies;begin if aritherror then cleararith;
end;end{:815};end{:812};20,21:confusion(763);
22,23:deletemacref(mem[p+1].int);end;mem[p].hh.b0:=0;end;{:809}{808:}
procedure flushcurexp(v:scaled);
begin case curtype of 3,5,7,12,10,13,14,17,18,19:begin recyclevalue(
curexp);freenode(curexp,2);end;
6:if mem[curexp].hh.lh=0 then tosspen(curexp)else decr(mem[curexp].hh.lh
);
4:begin if strref[curexp]<127 then if strref[curexp]>1 then decr(strref[
curexp])else flushstring(curexp);end;8,9:tossknotlist(curexp);
11:tossedges(curexp);others:end;curtype:=16;curexp:=v;end;{:808}{820:}
procedure flusherror(v:scaled);begin error;flushcurexp(v);end;
procedure backerror;forward;procedure getxnext;forward;
procedure putgeterror;begin backerror;getxnext;end;
procedure putgetflusherror(v:scaled);begin putgeterror;flushcurexp(v);
end;{:820}{247:}procedure flushbelowvariable(p:halfword);
var q,r:halfword;
begin if mem[p].hh.b0<>21 then recyclevalue(p)else begin q:=mem[p+1].hh.
rh;while mem[q].hh.b1=3 do begin flushbelowvariable(q);r:=q;
q:=mem[q].hh.rh;freenode(r,3);end;r:=mem[p+1].hh.lh;q:=mem[r].hh.rh;
recyclevalue(r);if mem[p].hh.b1<=1 then freenode(r,2)else freenode(r,3);
repeat flushbelowvariable(q);r:=q;q:=mem[q].hh.rh;freenode(r,3);
until q=17;mem[p].hh.b0:=0;end;end;{:247}
procedure flushvariable(p,t:halfword;discardsuffixes:boolean);label 10;
var q,r:halfword;n:halfword;
begin while t<>0 do begin if mem[p].hh.b0<>21 then goto 10;
n:=mem[t].hh.lh;t:=mem[t].hh.rh;if n=0 then begin r:=p+1;
q:=mem[r].hh.rh;
while mem[q].hh.b1=3 do begin flushvariable(q,t,discardsuffixes);
if t=0 then if mem[q].hh.b0=21 then r:=q else begin mem[r].hh.rh:=mem[q]
.hh.rh;freenode(q,3);end else r:=q;q:=mem[r].hh.rh;end;end;
p:=mem[p+1].hh.lh;repeat r:=p;p:=mem[p].hh.rh;until mem[p+2].hh.lh>=n;
if mem[p+2].hh.lh<>n then goto 10;end;
if discardsuffixes then flushbelowvariable(p)else begin if mem[p].hh.b0=
21 then p:=mem[p+1].hh.lh;recyclevalue(p);end;10:end;{:246}{248:}
function undtype(p:halfword):smallnumber;
begin case mem[p].hh.b0 of 0,1:undtype:=0;2,3:undtype:=3;4,5:undtype:=5;
6,7,8:undtype:=7;9,10:undtype:=10;11,12:undtype:=12;
13,14,15:undtype:=mem[p].hh.b0;16,17,18,19:undtype:=15;end;end;{:248}
{249:}procedure clearsymbol(p:halfword;saving:boolean);var q:halfword;
begin q:=eqtb[p].rh;
case eqtb[p].lh mod 86 of 10,53,44,49:if not saving then deletemacref(q)
;41:if q<>0 then if saving then mem[q].hh.b1:=1 else begin
flushbelowvariable(q);freenode(q,2);end;others:end;eqtb[p]:=eqtb[9769];
end;{:249}{252:}procedure savevariable(q:halfword);var p:halfword;
begin if saveptr<>0 then begin p:=getnode(2);mem[p].hh.lh:=q;
mem[p].hh.rh:=saveptr;mem[p+1].hh:=eqtb[q];saveptr:=p;end;
clearsymbol(q,(saveptr<>0));end;{:252}{253:}
procedure saveinternal(q:halfword);var p:halfword;
begin if saveptr<>0 then begin p:=getnode(2);mem[p].hh.lh:=9769+q;
mem[p].hh.rh:=saveptr;mem[p+1].int:=internal[q];saveptr:=p;end;end;
{:253}{254:}procedure unsave;var q:halfword;p:halfword;
begin while mem[saveptr].hh.lh<>0 do begin q:=mem[saveptr].hh.lh;
if q>9769 then begin if internal[8]>0 then begin begindiagnostic;
printnl(515);slowprint(intname[q-(9769)]);printchar(61);
printscaled(mem[saveptr+1].int);printchar(125);enddiagnostic(false);end;
internal[q-(9769)]:=mem[saveptr+1].int;
end else begin if internal[8]>0 then begin begindiagnostic;printnl(515);
slowprint(hash[q].rh);printchar(125);enddiagnostic(false);end;
clearsymbol(q,false);eqtb[q]:=mem[saveptr+1].hh;
if eqtb[q].lh mod 86=41 then begin p:=eqtb[q].rh;
if p<>0 then mem[p].hh.b1:=0;end;end;p:=mem[saveptr].hh.rh;
freenode(saveptr,2);saveptr:=p;end;p:=mem[saveptr].hh.rh;
begin mem[saveptr].hh.rh:=avail;avail:=saveptr;
ifdef('STAT')decr(dynused);endif('STAT')end;saveptr:=p;end;{:254}{264:}
function copyknot(p:halfword):halfword;var q:halfword;k:0..6;
begin q:=getnode(7);for k:=0 to 6 do mem[q+k]:=mem[p+k];copyknot:=q;end;
{:264}{265:}function copypath(p:halfword):halfword;label 10;
var q,pp,qq:halfword;begin q:=getnode(7);qq:=q;pp:=p;
while true do begin mem[qq].hh.b0:=mem[pp].hh.b0;
mem[qq].hh.b1:=mem[pp].hh.b1;mem[qq+1].int:=mem[pp+1].int;
mem[qq+2].int:=mem[pp+2].int;mem[qq+3].int:=mem[pp+3].int;
mem[qq+4].int:=mem[pp+4].int;mem[qq+5].int:=mem[pp+5].int;
mem[qq+6].int:=mem[pp+6].int;
if mem[pp].hh.rh=p then begin mem[qq].hh.rh:=q;copypath:=q;goto 10;end;
mem[qq].hh.rh:=getnode(7);qq:=mem[qq].hh.rh;pp:=mem[pp].hh.rh;end;
10:end;{:265}{266:}function htapypoc(p:halfword):halfword;label 10;
var q,pp,qq,rr:halfword;begin q:=getnode(7);qq:=q;pp:=p;
while true do begin mem[qq].hh.b1:=mem[pp].hh.b0;
mem[qq].hh.b0:=mem[pp].hh.b1;mem[qq+1].int:=mem[pp+1].int;
mem[qq+2].int:=mem[pp+2].int;mem[qq+5].int:=mem[pp+3].int;
mem[qq+6].int:=mem[pp+4].int;mem[qq+3].int:=mem[pp+5].int;
mem[qq+4].int:=mem[pp+6].int;
if mem[pp].hh.rh=p then begin mem[q].hh.rh:=qq;pathtail:=pp;htapypoc:=q;
goto 10;end;rr:=getnode(7);mem[rr].hh.rh:=qq;qq:=rr;pp:=mem[pp].hh.rh;
end;10:end;{:266}{269:}{284:}{296:}
function curlratio(gamma,atension,btension:scaled):fraction;
var alpha,beta,num,denom,ff:fraction;
begin alpha:=makefraction(65536,atension);
beta:=makefraction(65536,btension);
if alpha<=beta then begin ff:=makefraction(alpha,beta);
ff:=takefraction(ff,ff);gamma:=takefraction(gamma,ff);
beta:=beta div 4096;denom:=takefraction(gamma,alpha)+196608-beta;
num:=takefraction(gamma,805306368-alpha)+beta;
end else begin ff:=makefraction(beta,alpha);ff:=takefraction(ff,ff);
beta:=takefraction(beta,ff)div 4096;
denom:=takefraction(gamma,alpha)+(ff div 1365)-beta;
num:=takefraction(gamma,805306368-alpha)+beta;end;
if num>=denom+denom+denom+denom then curlratio:=1073741824 else
curlratio:=makefraction(num,denom);end;{:296}{299:}
procedure setcontrols(p,q:halfword;k:integer);var rr,ss:fraction;
lt,rt:scaled;sine:fraction;begin lt:=abs(mem[q+4].int);
rt:=abs(mem[p+6].int);rr:=velocity(st,ct,sf,cf,rt);
ss:=velocity(sf,cf,st,ct,lt);
if(mem[p+6].int<0)or(mem[q+4].int<0)then{300:}
if((st>=0)and(sf>=0))or((st<=0)and(sf<=0))then begin sine:=takefraction(
abs(st),cf)+takefraction(abs(sf),ct);
if sine>0 then begin sine:=takefraction(sine,268500992);
if mem[p+6].int<0 then if abvscd(abs(sf),268435456,rr,sine)<0 then rr:=
makefraction(abs(sf),sine);
if mem[q+4].int<0 then if abvscd(abs(st),268435456,ss,sine)<0 then ss:=
makefraction(abs(st),sine);end;end{:300};
mem[p+5].int:=mem[p+1].int+takefraction(takefraction(deltax[k],ct)-
takefraction(deltay[k],st),rr);
mem[p+6].int:=mem[p+2].int+takefraction(takefraction(deltay[k],ct)+
takefraction(deltax[k],st),rr);
mem[q+3].int:=mem[q+1].int-takefraction(takefraction(deltax[k],cf)+
takefraction(deltay[k],sf),ss);
mem[q+4].int:=mem[q+2].int-takefraction(takefraction(deltay[k],cf)-
takefraction(deltax[k],sf),ss);mem[p].hh.b1:=1;mem[q].hh.b0:=1;end;
{:299}procedure solvechoices(p,q:halfword;n:halfword);label 40,10;
var k:0..pathsize;r,s,t:halfword;{286:}aa,bb,cc,ff,acc:fraction;
dd,ee:scaled;lt,rt:scaled;{:286}begin k:=0;s:=p;
while true do begin t:=mem[s].hh.rh;if k=0 then{285:}
case mem[s].hh.b1 of 2:if mem[t].hh.b0=2 then{301:}
begin aa:=narg(deltax[0],deltay[0]);nsincos(mem[p+5].int-aa);ct:=ncos;
st:=nsin;nsincos(mem[q+3].int-aa);cf:=ncos;sf:=-nsin;setcontrols(p,q,0);
goto 10;end{:301}else{293:}
begin vv[0]:=mem[s+5].int-narg(deltax[0],deltay[0]);
if abs(vv[0])>188743680 then if vv[0]>0 then vv[0]:=vv[0]-377487360 else
vv[0]:=vv[0]+377487360;uu[0]:=0;ww[0]:=0;end{:293};
3:if mem[t].hh.b0=3 then{302:}begin mem[p].hh.b1:=1;mem[q].hh.b0:=1;
lt:=abs(mem[q+4].int);rt:=abs(mem[p+6].int);
if rt=65536 then begin if deltax[0]>=0 then mem[p+5].int:=mem[p+1].int+(
(deltax[0]+1)div 3)else mem[p+5].int:=mem[p+1].int+((deltax[0]-1)div 3);
if deltay[0]>=0 then mem[p+6].int:=mem[p+2].int+((deltay[0]+1)div 3)else
mem[p+6].int:=mem[p+2].int+((deltay[0]-1)div 3);
end else begin ff:=makefraction(65536,3*rt);
mem[p+5].int:=mem[p+1].int+takefraction(deltax[0],ff);
mem[p+6].int:=mem[p+2].int+takefraction(deltay[0],ff);end;
if lt=65536 then begin if deltax[0]>=0 then mem[q+3].int:=mem[q+1].int-(
(deltax[0]+1)div 3)else mem[q+3].int:=mem[q+1].int-((deltax[0]-1)div 3);
if deltay[0]>=0 then mem[q+4].int:=mem[q+2].int-((deltay[0]+1)div 3)else
mem[q+4].int:=mem[q+2].int-((deltay[0]-1)div 3);
end else begin ff:=makefraction(65536,3*lt);
mem[q+3].int:=mem[q+1].int-takefraction(deltax[0],ff);
mem[q+4].int:=mem[q+2].int-takefraction(deltay[0],ff);end;goto 10;
end{:302}else{294:}begin cc:=mem[s+5].int;lt:=abs(mem[t+4].int);
rt:=abs(mem[s+6].int);
if(rt=65536)and(lt=65536)then uu[0]:=makefraction(cc+cc+65536,cc+131072)
else uu[0]:=curlratio(cc,rt,lt);vv[0]:=-takefraction(psi[1],uu[0]);
ww[0]:=0;end{:294};4:begin uu[0]:=0;vv[0]:=0;ww[0]:=268435456;end;
end{:285}else case mem[s].hh.b0 of 5,4:{287:}begin{288:}
if abs(mem[r+6].int)=65536 then begin aa:=134217728;dd:=2*delta[k];
end else begin aa:=makefraction(65536,3*abs(mem[r+6].int)-65536);
dd:=takefraction(delta[k],805306368-makefraction(65536,abs(mem[r+6].int)
));end;if abs(mem[t+4].int)=65536 then begin bb:=134217728;
ee:=2*delta[k-1];
end else begin bb:=makefraction(65536,3*abs(mem[t+4].int)-65536);
ee:=takefraction(delta[k-1],805306368-makefraction(65536,abs(mem[t+4].
int)));end;cc:=268435456-takefraction(uu[k-1],aa){:288};{289:}
dd:=takefraction(dd,cc);lt:=abs(mem[s+4].int);rt:=abs(mem[s+6].int);
if lt<>rt then if lt<rt then begin ff:=makefraction(lt,rt);
ff:=takefraction(ff,ff);dd:=takefraction(dd,ff);
end else begin ff:=makefraction(rt,lt);ff:=takefraction(ff,ff);
ee:=takefraction(ee,ff);end;ff:=makefraction(ee,ee+dd){:289};
uu[k]:=takefraction(ff,bb);{290:}acc:=-takefraction(psi[k+1],uu[k]);
if mem[r].hh.b1=3 then begin ww[k]:=0;
vv[k]:=acc-takefraction(psi[1],268435456-ff);
end else begin ff:=makefraction(268435456-ff,cc);
acc:=acc-takefraction(psi[k],ff);ff:=takefraction(ff,aa);
vv[k]:=acc-takefraction(vv[k-1],ff);
if ww[k-1]=0 then ww[k]:=0 else ww[k]:=-takefraction(ww[k-1],ff);
end{:290};if mem[s].hh.b0=5 then{291:}begin aa:=0;bb:=268435456;
repeat decr(k);if k=0 then k:=n;aa:=vv[k]-takefraction(aa,uu[k]);
bb:=ww[k]-takefraction(bb,uu[k]);until k=n;
aa:=makefraction(aa,268435456-bb);theta[n]:=aa;vv[0]:=aa;
for k:=1 to n-1 do vv[k]:=vv[k]+takefraction(aa,ww[k]);goto 40;end{:291}
;end{:287};3:{295:}begin cc:=mem[s+3].int;lt:=abs(mem[s+4].int);
rt:=abs(mem[r+6].int);
if(rt=65536)and(lt=65536)then ff:=makefraction(cc+cc+65536,cc+131072)
else ff:=curlratio(cc,lt,rt);
theta[n]:=-makefraction(takefraction(vv[n-1],ff),268435456-takefraction(
ff,uu[n-1]));goto 40;end{:295};2:{292:}
begin theta[n]:=mem[s+3].int-narg(deltax[n-1],deltay[n-1]);
if abs(theta[n])>188743680 then if theta[n]>0 then theta[n]:=theta[n]
-377487360 else theta[n]:=theta[n]+377487360;goto 40;end{:292};end;r:=s;
s:=t;incr(k);end;40:{297:}
for k:=n-1 downto 0 do theta[k]:=vv[k]-takefraction(theta[k+1],uu[k]);
s:=p;k:=0;repeat t:=mem[s].hh.rh;nsincos(theta[k]);st:=nsin;ct:=ncos;
nsincos(-psi[k+1]-theta[k+1]);sf:=nsin;cf:=ncos;setcontrols(s,t,k);
incr(k);s:=t;until k=n{:297};10:end;{:284}
procedure makechoices(knots:halfword);label 30;var h:halfword;
p,q:halfword;{280:}k,n:0..pathsize;s,t:halfword;delx,dely:scaled;
sine,cosine:fraction;{:280}begin begin if aritherror then cleararith;
end;if internal[4]>0 then printpath(knots,525,true);{271:}p:=knots;
repeat q:=mem[p].hh.rh;
if mem[p+1].int=mem[q+1].int then if mem[p+2].int=mem[q+2].int then if
mem[p].hh.b1>1 then begin mem[p].hh.b1:=1;
if mem[p].hh.b0=4 then begin mem[p].hh.b0:=3;mem[p+3].int:=65536;end;
mem[q].hh.b0:=1;if mem[q].hh.b1=4 then begin mem[q].hh.b1:=3;
mem[q+5].int:=65536;end;mem[p+5].int:=mem[p+1].int;
mem[q+3].int:=mem[p+1].int;mem[p+6].int:=mem[p+2].int;
mem[q+4].int:=mem[p+2].int;end;p:=q;until p=knots{:271};{272:}h:=knots;
while true do begin if mem[h].hh.b0<>4 then goto 30;
if mem[h].hh.b1<>4 then goto 30;h:=mem[h].hh.rh;
if h=knots then begin mem[h].hh.b0:=5;goto 30;end;end;30:{:272};p:=h;
repeat{273:}q:=mem[p].hh.rh;
if mem[p].hh.b1>=2 then begin while(mem[q].hh.b0=4)and(mem[q].hh.b1=4)do
q:=mem[q].hh.rh;{278:}{281:}k:=0;s:=p;n:=pathsize;
repeat t:=mem[s].hh.rh;deltax[k]:=mem[t+1].int-mem[s+1].int;
deltay[k]:=mem[t+2].int-mem[s+2].int;
delta[k]:=pythadd(deltax[k],deltay[k]);
if k>0 then begin sine:=makefraction(deltay[k-1],delta[k-1]);
cosine:=makefraction(deltax[k-1],delta[k-1]);
psi[k]:=narg(takefraction(deltax[k],cosine)+takefraction(deltay[k],sine)
,takefraction(deltay[k],cosine)-takefraction(deltax[k],sine));end;
incr(k);s:=t;if k=pathsize then overflow(530,pathsize);if s=q then n:=k;
until(k>=n)and(mem[s].hh.b0<>5);
if k=n then psi[n]:=0 else psi[k]:=psi[1]{:281};{282:}
if mem[q].hh.b0=4 then begin delx:=mem[q+5].int-mem[q+1].int;
dely:=mem[q+6].int-mem[q+2].int;
if(delx=0)and(dely=0)then begin mem[q].hh.b0:=3;mem[q+3].int:=65536;
end else begin mem[q].hh.b0:=2;mem[q+3].int:=narg(delx,dely);end;end;
if(mem[p].hh.b1=4)and(mem[p].hh.b0=1)then begin delx:=mem[p+1].int-mem[p
+3].int;dely:=mem[p+2].int-mem[p+4].int;
if(delx=0)and(dely=0)then begin mem[p].hh.b1:=3;mem[p+5].int:=65536;
end else begin mem[p].hh.b1:=2;mem[p+5].int:=narg(delx,dely);end;
end{:282};solvechoices(p,q,n){:278};end;p:=q{:273};until p=h;
if internal[4]>0 then printpath(knots,526,true);if aritherror then{270:}
begin begin if interaction=3 then;printnl(261);print(527);end;
begin helpptr:=2;helpline[1]:=528;helpline[0]:=529;end;putgeterror;
aritherror:=false;end{:270};end;{:269}{311:}
procedure makemoves(xx0,xx1,xx2,xx3,yy0,yy1,yy2,yy3:scaled;
xicorr,etacorr:smallnumber);label 22,30,10;
var x1,x2,x3,m,r,y1,y2,y3,n,s,l:integer;q,t,u,x2a,x3a,y2a,y3a:integer;
begin if(xx3<xx0)or(yy3<yy0)then confusion(109);l:=16;bisectptr:=0;
x1:=xx1-xx0;x2:=xx2-xx1;x3:=xx3-xx2;
if xx0>=xicorr then r:=(xx0-xicorr)mod 65536 else r:=65535-((-xx0+xicorr
-1)mod 65536);m:=(xx3-xx0+r)div 65536;y1:=yy1-yy0;y2:=yy2-yy1;
y3:=yy3-yy2;
if yy0>=etacorr then s:=(yy0-etacorr)mod 65536 else s:=65535-((-yy0+
etacorr-1)mod 65536);n:=(yy3-yy0+s)div 65536;
if(xx3-xx0>=268435456)or(yy3-yy0>=268435456)then{313:}
begin x1:=(x1+xicorr)div 2;x2:=(x2+xicorr)div 2;x3:=(x3+xicorr)div 2;
r:=(r+xicorr)div 2;y1:=(y1+etacorr)div 2;y2:=(y2+etacorr)div 2;
y3:=(y3+etacorr)div 2;s:=(s+etacorr)div 2;l:=15;end{:313};
while true do begin 22:{314:}if m=0 then{315:}
while n>0 do begin incr(moveptr);move[moveptr]:=1;decr(n);end{:315}
else if n=0 then{316:}move[moveptr]:=move[moveptr]+m{:316}
else if m+n=2 then{317:}begin r:=twotothe[l]-r;s:=twotothe[l]-s;
while l<30 do begin x3a:=x3;x2a:=(x2+x3+xicorr)div 2;
x2:=(x1+x2+xicorr)div 2;x3:=(x2+x2a+xicorr)div 2;t:=x1+x2+x3;
r:=r+r-xicorr;y3a:=y3;y2a:=(y2+y3+etacorr)div 2;
y2:=(y1+y2+etacorr)div 2;y3:=(y2+y2a+etacorr)div 2;u:=y1+y2+y3;
s:=s+s-etacorr;if t<r then if u<s then{318:}begin x1:=x3;x2:=x2a;
x3:=x3a;r:=r-t;y1:=y3;y2:=y2a;y3:=y3a;s:=s-u;end{:318}else begin{320:}
begin incr(moveptr);move[moveptr]:=2;end{:320};goto 30;
end else if u<s then begin{319:}begin incr(move[moveptr]);incr(moveptr);
move[moveptr]:=1;end{:319};goto 30;end;incr(l);end;r:=r-xicorr;
s:=s-etacorr;if abvscd(x1+x2+x3,s,y1+y2+y3,r)-xicorr>=0 then{319:}
begin incr(move[moveptr]);incr(moveptr);move[moveptr]:=1;end{:319}
else{320:}begin incr(moveptr);move[moveptr]:=2;end{:320};30:end{:317}
else begin incr(l);bisectstack[bisectptr+10]:=l;
bisectstack[bisectptr+2]:=x3;
bisectstack[bisectptr+1]:=(x2+x3+xicorr)div 2;x2:=(x1+x2+xicorr)div 2;
x3:=(x2+bisectstack[bisectptr+1]+xicorr)div 2;
bisectstack[bisectptr]:=x3;r:=r+r+xicorr;t:=x1+x2+x3+r;
q:=t div twotothe[l];bisectstack[bisectptr+3]:=t mod twotothe[l];
bisectstack[bisectptr+4]:=m-q;m:=q;bisectstack[bisectptr+7]:=y3;
bisectstack[bisectptr+6]:=(y2+y3+etacorr)div 2;y2:=(y1+y2+etacorr)div 2;
y3:=(y2+bisectstack[bisectptr+6]+etacorr)div 2;
bisectstack[bisectptr+5]:=y3;s:=s+s+etacorr;u:=y1+y2+y3+s;
q:=u div twotothe[l];bisectstack[bisectptr+8]:=u mod twotothe[l];
bisectstack[bisectptr+9]:=n-q;n:=q;bisectptr:=bisectptr+11;goto 22;
end{:314};if bisectptr=0 then goto 10;{312:}bisectptr:=bisectptr-11;
x1:=bisectstack[bisectptr];x2:=bisectstack[bisectptr+1];
x3:=bisectstack[bisectptr+2];r:=bisectstack[bisectptr+3];
m:=bisectstack[bisectptr+4];y1:=bisectstack[bisectptr+5];
y2:=bisectstack[bisectptr+6];y3:=bisectstack[bisectptr+7];
s:=bisectstack[bisectptr+8];n:=bisectstack[bisectptr+9];
l:=bisectstack[bisectptr+10]{:312};end;10:end;{:311}{321:}
procedure smoothmoves(b,t:integer);var k:1..movesize;a,aa,aaa:integer;
begin if t-b>=3 then begin k:=b+2;aa:=move[k-1];aaa:=move[k-2];
repeat a:=move[k];if abs(a-aa)>1 then{322:}
if a>aa then begin if aaa>=aa then if a>=move[k+1]then begin incr(move[k
-1]);move[k]:=a-1;end;
end else begin if aaa<=aa then if a<=move[k+1]then begin decr(move[k-1])
;move[k]:=a+1;end;end{:322};incr(k);aaa:=aa;aa:=a;until k=t;end;end;
{:321}{326:}procedure initedges(h:halfword);begin mem[h].hh.lh:=h;
mem[h].hh.rh:=h;mem[h+1].hh.lh:=8191;mem[h+1].hh.rh:=1;
mem[h+2].hh.lh:=8191;mem[h+2].hh.rh:=1;mem[h+3].hh.lh:=4096;
mem[h+3].hh.rh:=0;mem[h+4].int:=0;mem[h+5].hh.rh:=h;mem[h+5].hh.lh:=0;
end;{:326}{328:}procedure fixoffset;var p,q:halfword;delta:integer;
begin delta:=8*(mem[curedges+3].hh.lh-4096);mem[curedges+3].hh.lh:=4096;
q:=mem[curedges].hh.rh;while q<>curedges do begin p:=mem[q+1].hh.rh;
while p<>memtop do begin mem[p].hh.lh:=mem[p].hh.lh-delta;
p:=mem[p].hh.rh;end;p:=mem[q+1].hh.lh;
while p>1 do begin mem[p].hh.lh:=mem[p].hh.lh-delta;p:=mem[p].hh.rh;end;
q:=mem[q].hh.rh;end;end;{:328}{329:}
procedure edgeprep(ml,mr,nl,nr:integer);var delta:halfword;temp:integer;
p,q:halfword;begin ml:=ml+4096;mr:=mr+4096;nl:=nl+4096;nr:=nr+4095;
if ml<mem[curedges+2].hh.lh then mem[curedges+2].hh.lh:=ml;
if mr>mem[curedges+2].hh.rh then mem[curedges+2].hh.rh:=mr;
temp:=mem[curedges+3].hh.lh-4096;
if not(abs(mem[curedges+2].hh.lh+temp-4096)<4096)or not(abs(mem[curedges
+2].hh.rh+temp-4096)<4096)then fixoffset;
if mem[curedges].hh.rh=curedges then begin mem[curedges+1].hh.lh:=nr+1;
mem[curedges+1].hh.rh:=nr;end;if nl<mem[curedges+1].hh.lh then{330:}
begin delta:=mem[curedges+1].hh.lh-nl;mem[curedges+1].hh.lh:=nl;
p:=mem[curedges].hh.rh;repeat q:=getnode(2);mem[q+1].hh.rh:=memtop;
mem[q+1].hh.lh:=1;mem[p].hh.lh:=q;mem[q].hh.rh:=p;p:=q;decr(delta);
until delta=0;mem[p].hh.lh:=curedges;mem[curedges].hh.rh:=p;
if mem[curedges+5].hh.rh=curedges then mem[curedges+5].hh.lh:=nl-1;
end{:330};if nr>mem[curedges+1].hh.rh then{331:}
begin delta:=nr-mem[curedges+1].hh.rh;mem[curedges+1].hh.rh:=nr;
p:=mem[curedges].hh.lh;repeat q:=getnode(2);mem[q+1].hh.rh:=memtop;
mem[q+1].hh.lh:=1;mem[p].hh.rh:=q;mem[q].hh.lh:=p;p:=q;decr(delta);
until delta=0;mem[p].hh.rh:=curedges;mem[curedges].hh.lh:=p;
if mem[curedges+5].hh.rh=curedges then mem[curedges+5].hh.lh:=nr+1;
end{:331};end;{:329}{334:}function copyedges(h:halfword):halfword;
var p,r:halfword;hh,pp,qq,rr,ss:halfword;begin hh:=getnode(6);
mem[hh+1]:=mem[h+1];mem[hh+2]:=mem[h+2];mem[hh+3]:=mem[h+3];
mem[hh+4]:=mem[h+4];mem[hh+5].hh.lh:=mem[hh+1].hh.rh+1;
mem[hh+5].hh.rh:=hh;p:=mem[h].hh.rh;qq:=hh;
while p<>h do begin pp:=getnode(2);mem[qq].hh.rh:=pp;mem[pp].hh.lh:=qq;
{335:}r:=mem[p+1].hh.rh;rr:=pp+1;while r<>memtop do begin ss:=getavail;
mem[rr].hh.rh:=ss;rr:=ss;mem[rr].hh.lh:=mem[r].hh.lh;r:=mem[r].hh.rh;
end;mem[rr].hh.rh:=memtop;r:=mem[p+1].hh.lh;rr:=memtop-1;
while r>1 do begin ss:=getavail;mem[rr].hh.rh:=ss;rr:=ss;
mem[rr].hh.lh:=mem[r].hh.lh;r:=mem[r].hh.rh;end;mem[rr].hh.rh:=r;
mem[pp+1].hh.lh:=mem[memtop-1].hh.rh{:335};p:=mem[p].hh.rh;qq:=pp;end;
mem[qq].hh.rh:=hh;mem[hh].hh.lh:=qq;copyedges:=hh;end;{:334}{336:}
procedure yreflectedges;var p,q,r:halfword;
begin p:=mem[curedges+1].hh.lh;
mem[curedges+1].hh.lh:=8191-mem[curedges+1].hh.rh;
mem[curedges+1].hh.rh:=8191-p;
mem[curedges+5].hh.lh:=8191-mem[curedges+5].hh.lh;
p:=mem[curedges].hh.rh;q:=curedges;repeat r:=mem[p].hh.rh;
mem[p].hh.rh:=q;mem[q].hh.lh:=p;q:=p;p:=r;until q=curedges;
mem[curedges+4].int:=0;end;{:336}{337:}procedure xreflectedges;
var p,q,r,s:halfword;m:integer;begin p:=mem[curedges+2].hh.lh;
mem[curedges+2].hh.lh:=8192-mem[curedges+2].hh.rh;
mem[curedges+2].hh.rh:=8192-p;m:=(4096+mem[curedges+3].hh.lh)*8+8;
mem[curedges+3].hh.lh:=4096;p:=mem[curedges].hh.rh;repeat{339:}
q:=mem[p+1].hh.rh;r:=memtop;while q<>memtop do begin s:=mem[q].hh.rh;
mem[q].hh.rh:=r;r:=q;mem[r].hh.lh:=m-mem[q].hh.lh;q:=s;end;
mem[p+1].hh.rh:=r{:339};{338:}q:=mem[p+1].hh.lh;
while q>1 do begin mem[q].hh.lh:=m-mem[q].hh.lh;q:=mem[q].hh.rh;
end{:338};p:=mem[p].hh.rh;until p=curedges;mem[curedges+4].int:=0;end;
{:337}{340:}procedure yscaleedges(s:integer);
var p,q,pp,r,rr,ss:halfword;t:integer;
begin if(s*(mem[curedges+1].hh.rh-4095)>=4096)or(s*(mem[curedges+1].hh.
lh-4096)<=-4096)then begin begin if interaction=3 then;printnl(261);
print(534);end;begin helpptr:=3;helpline[2]:=535;helpline[1]:=536;
helpline[0]:=537;end;putgeterror;
end else begin mem[curedges+1].hh.rh:=s*(mem[curedges+1].hh.rh-4095)
+4095;mem[curedges+1].hh.lh:=s*(mem[curedges+1].hh.lh-4096)+4096;{341:}
p:=curedges;repeat q:=p;p:=mem[p].hh.rh;
for t:=2 to s do begin pp:=getnode(2);mem[q].hh.rh:=pp;mem[p].hh.lh:=pp;
mem[pp].hh.rh:=p;mem[pp].hh.lh:=q;q:=pp;{335:}r:=mem[p+1].hh.rh;
rr:=pp+1;while r<>memtop do begin ss:=getavail;mem[rr].hh.rh:=ss;rr:=ss;
mem[rr].hh.lh:=mem[r].hh.lh;r:=mem[r].hh.rh;end;mem[rr].hh.rh:=memtop;
r:=mem[p+1].hh.lh;rr:=memtop-1;while r>1 do begin ss:=getavail;
mem[rr].hh.rh:=ss;rr:=ss;mem[rr].hh.lh:=mem[r].hh.lh;r:=mem[r].hh.rh;
end;mem[rr].hh.rh:=r;mem[pp+1].hh.lh:=mem[memtop-1].hh.rh{:335};end;
until mem[p].hh.rh=curedges{:341};mem[curedges+4].int:=0;end;end;{:340}
{342:}procedure xscaleedges(s:integer);var p,q:halfword;t:0..65535;
w:0..7;delta:integer;
begin if(s*(mem[curedges+2].hh.rh-4096)>=4096)or(s*(mem[curedges+2].hh.
lh-4096)<=-4096)then begin begin if interaction=3 then;printnl(261);
print(534);end;begin helpptr:=3;helpline[2]:=538;helpline[1]:=536;
helpline[0]:=537;end;putgeterror;
end else if(mem[curedges+2].hh.rh<>4096)or(mem[curedges+2].hh.lh<>4096)
then begin mem[curedges+2].hh.rh:=s*(mem[curedges+2].hh.rh-4096)+4096;
mem[curedges+2].hh.lh:=s*(mem[curedges+2].hh.lh-4096)+4096;
delta:=8*(4096-s*mem[curedges+3].hh.lh)+0;mem[curedges+3].hh.lh:=4096;
{343:}q:=mem[curedges].hh.rh;repeat p:=mem[q+1].hh.rh;
while p<>memtop do begin t:=mem[p].hh.lh;w:=t mod 8;
mem[p].hh.lh:=(t-w)*s+w+delta;p:=mem[p].hh.rh;end;p:=mem[q+1].hh.lh;
while p>1 do begin t:=mem[p].hh.lh;w:=t mod 8;
mem[p].hh.lh:=(t-w)*s+w+delta;p:=mem[p].hh.rh;end;q:=mem[q].hh.rh;
until q=curedges{:343};mem[curedges+4].int:=0;end;end;{:342}{344:}
procedure negateedges(h:halfword);label 30;var p,q,r,s,t,u:halfword;
begin p:=mem[h].hh.rh;while p<>h do begin q:=mem[p+1].hh.lh;
while q>1 do begin mem[q].hh.lh:=8-2*((mem[q].hh.lh)mod 8)+mem[q].hh.lh;
q:=mem[q].hh.rh;end;q:=mem[p+1].hh.rh;
if q<>memtop then begin repeat mem[q].hh.lh:=8-2*((mem[q].hh.lh)mod 8)+
mem[q].hh.lh;q:=mem[q].hh.rh;until q=memtop;{345:}u:=p+1;
q:=mem[u].hh.rh;r:=q;s:=mem[r].hh.rh;
while true do if mem[s].hh.lh>mem[r].hh.lh then begin mem[u].hh.rh:=q;
if s=memtop then goto 30;u:=r;q:=s;r:=q;s:=mem[r].hh.rh;
end else begin t:=s;s:=mem[t].hh.rh;mem[t].hh.rh:=q;q:=t;end;
30:mem[r].hh.rh:=memtop{:345};end;p:=mem[p].hh.rh;end;mem[h+4].int:=0;
end;{:344}{346:}procedure sortedges(h:halfword);label 30;var k:halfword;
p,q,r,s:halfword;begin r:=mem[h+1].hh.lh;mem[h+1].hh.lh:=0;
p:=mem[r].hh.rh;mem[r].hh.rh:=memtop;mem[memtop-1].hh.rh:=r;
while p>1 do begin k:=mem[p].hh.lh;q:=memtop-1;repeat r:=q;
q:=mem[r].hh.rh;until k<=mem[q].hh.lh;mem[r].hh.rh:=p;r:=mem[p].hh.rh;
mem[p].hh.rh:=q;p:=r;end;{347:}begin r:=h+1;q:=mem[r].hh.rh;
p:=mem[memtop-1].hh.rh;while true do begin k:=mem[p].hh.lh;
while k>mem[q].hh.lh do begin r:=q;q:=mem[r].hh.rh;end;mem[r].hh.rh:=p;
s:=mem[p].hh.rh;mem[p].hh.rh:=q;if s=memtop then goto 30;r:=p;p:=s;end;
30:end{:347};end;{:346}{348:}
procedure culledges(wlo,whi,wout,win:integer);label 30;
var p,q,r,s:halfword;w:integer;d:integer;m:integer;mm:integer;
ww:integer;prevw:integer;n,minn,maxn:halfword;mind,maxd:halfword;
begin mind:=262143;maxd:=0;minn:=262143;maxn:=0;p:=mem[curedges].hh.rh;
n:=mem[curedges+1].hh.lh;
while p<>curedges do begin if mem[p+1].hh.lh>1 then sortedges(p);
if mem[p+1].hh.rh<>memtop then{349:}begin r:=memtop-1;q:=mem[p+1].hh.rh;
ww:=0;m:=1000000;prevw:=0;
while true do begin if q=memtop then mm:=1000000 else begin d:=mem[q].hh
.lh;mm:=d div 8;ww:=ww+(d mod 8)-4;end;if mm>m then begin{350:}
if w<>prevw then begin s:=getavail;mem[r].hh.rh:=s;
mem[s].hh.lh:=8*m+4+w-prevw;r:=s;prevw:=w;end{:350};
if q=memtop then goto 30;end;m:=mm;
if ww>=wlo then if ww<=whi then w:=win else w:=wout else w:=wout;
s:=mem[q].hh.rh;begin mem[q].hh.rh:=avail;avail:=q;
ifdef('STAT')decr(dynused);endif('STAT')end;q:=s;end;
30:mem[r].hh.rh:=memtop;mem[p+1].hh.rh:=mem[memtop-1].hh.rh;
if r<>memtop-1 then{351:}begin if minn=262143 then minn:=n;maxn:=n;
if mind>mem[mem[memtop-1].hh.rh].hh.lh then mind:=mem[mem[memtop-1].hh.
rh].hh.lh;if maxd<mem[r].hh.lh then maxd:=mem[r].hh.lh;end{:351};
end{:349};p:=mem[p].hh.rh;incr(n);end;{352:}if minn>maxn then{353:}
begin p:=mem[curedges].hh.rh;while p<>curedges do begin q:=mem[p].hh.rh;
freenode(p,2);p:=q;end;initedges(curedges);end{:353}
else begin n:=mem[curedges+1].hh.lh;mem[curedges+1].hh.lh:=minn;
while minn>n do begin p:=mem[curedges].hh.rh;
mem[curedges].hh.rh:=mem[p].hh.rh;mem[mem[p].hh.rh].hh.lh:=curedges;
freenode(p,2);incr(n);end;n:=mem[curedges+1].hh.rh;
mem[curedges+1].hh.rh:=maxn;mem[curedges+5].hh.lh:=maxn+1;
mem[curedges+5].hh.rh:=curedges;
while maxn<n do begin p:=mem[curedges].hh.lh;
mem[curedges].hh.lh:=mem[p].hh.lh;mem[mem[p].hh.lh].hh.rh:=curedges;
freenode(p,2);decr(n);end;
mem[curedges+2].hh.lh:=((mind)div 8)-mem[curedges+3].hh.lh+4096;
mem[curedges+2].hh.rh:=((maxd)div 8)-mem[curedges+3].hh.lh+4096;
end{:352};mem[curedges+4].int:=0;end;{:348}{354:}procedure xyswapedges;
label 30;var mmagic,nmagic:integer;p,q,r,s:halfword;{357:}
mspread:integer;j,jj:0..movesize;m,mm:integer;pd,rd:integer;
pm,rm:integer;w:integer;ww:integer;dw:integer;{:357}{363:}
extras:integer;xw:-3..3;k:integer;{:363}begin{356:}
mspread:=mem[curedges+2].hh.rh-mem[curedges+2].hh.lh;
if mspread>movesize then overflow(539,movesize);
for j:=0 to mspread do move[j]:=memtop{:356};{355:}p:=getnode(2);
mem[p+1].hh.rh:=memtop;mem[p+1].hh.lh:=0;mem[p].hh.lh:=curedges;
mem[mem[curedges].hh.rh].hh.lh:=p;p:=getnode(2);mem[p+1].hh.rh:=memtop;
mem[p].hh.lh:=mem[curedges].hh.lh;{:355};{365:}
mmagic:=mem[curedges+2].hh.lh+mem[curedges+3].hh.lh-4096;
nmagic:=8*mem[curedges+1].hh.rh+12{:365};repeat q:=mem[p].hh.lh;
if mem[q+1].hh.lh>1 then sortedges(q);{358:}r:=mem[p+1].hh.rh;
freenode(p,2);p:=r;pd:=mem[p].hh.lh;pm:=pd div 8;r:=mem[q+1].hh.rh;
rd:=mem[r].hh.lh;rm:=rd div 8;w:=0;
while true do begin if pm<rm then mm:=pm else mm:=rm;if w<>0 then{362:}
if m<>mm then begin if mm-mmagic>=movesize then confusion(509);
extras:=(abs(w)-1)div 3;
if extras>0 then begin if w>0 then xw:=+3 else xw:=-3;ww:=w-extras*xw;
end else ww:=w;repeat j:=m-mmagic;
for k:=1 to extras do begin s:=getavail;mem[s].hh.lh:=nmagic+xw;
mem[s].hh.rh:=move[j];move[j]:=s;end;s:=getavail;
mem[s].hh.lh:=nmagic+ww;mem[s].hh.rh:=move[j];move[j]:=s;incr(m);
until m=mm;end{:362};if pd<rd then begin dw:=(pd mod 8)-4;{360:}
s:=mem[p].hh.rh;begin mem[p].hh.rh:=avail;avail:=p;
ifdef('STAT')decr(dynused);endif('STAT')end;p:=s;pd:=mem[p].hh.lh;
pm:=pd div 8{:360};end else begin if r=memtop then goto 30;
dw:=-((rd mod 8)-4);{359:}r:=mem[r].hh.rh;rd:=mem[r].hh.lh;
rm:=rd div 8{:359};end;m:=mm;w:=w+dw;end;30:{:358};p:=q;
nmagic:=nmagic-8;until mem[p].hh.lh=curedges;freenode(p,2);{364:}
move[mspread]:=0;j:=0;while move[j]=memtop do incr(j);
if j=mspread then initedges(curedges)else begin mm:=mem[curedges+2].hh.
lh;mem[curedges+2].hh.lh:=mem[curedges+1].hh.lh;
mem[curedges+2].hh.rh:=mem[curedges+1].hh.rh+1;
mem[curedges+3].hh.lh:=4096;jj:=mspread-1;
while move[jj]=memtop do decr(jj);mem[curedges+1].hh.lh:=j+mm;
mem[curedges+1].hh.rh:=jj+mm;q:=curedges;repeat p:=getnode(2);
mem[q].hh.rh:=p;mem[p].hh.lh:=q;mem[p+1].hh.rh:=move[j];
mem[p+1].hh.lh:=0;incr(j);q:=p;until j>jj;mem[q].hh.rh:=curedges;
mem[curedges].hh.lh:=q;mem[curedges+5].hh.lh:=mem[curedges+1].hh.rh+1;
mem[curedges+5].hh.rh:=curedges;mem[curedges+4].int:=0;end;{:364};end;
{:354}{366:}procedure mergeedges(h:halfword);label 30;
var p,q,r,pp,qq,rr:halfword;n:integer;k:halfword;delta:integer;
begin if mem[h].hh.rh<>h then begin if(mem[h+2].hh.lh<mem[curedges+2].hh
.lh)or(mem[h+2].hh.rh>mem[curedges+2].hh.rh)or(mem[h+1].hh.lh<mem[
curedges+1].hh.lh)or(mem[h+1].hh.rh>mem[curedges+1].hh.rh)then edgeprep(
mem[h+2].hh.lh-4096,mem[h+2].hh.rh-4096,mem[h+1].hh.lh-4096,mem[h+1].hh.
rh-4095);if mem[h+3].hh.lh<>mem[curedges+3].hh.lh then{367:}
begin pp:=mem[h].hh.rh;delta:=8*(mem[curedges+3].hh.lh-mem[h+3].hh.lh);
repeat qq:=mem[pp+1].hh.rh;
while qq<>memtop do begin mem[qq].hh.lh:=mem[qq].hh.lh+delta;
qq:=mem[qq].hh.rh;end;qq:=mem[pp+1].hh.lh;
while qq>1 do begin mem[qq].hh.lh:=mem[qq].hh.lh+delta;
qq:=mem[qq].hh.rh;end;pp:=mem[pp].hh.rh;until pp=h;end{:367};
n:=mem[curedges+1].hh.lh;p:=mem[curedges].hh.rh;pp:=mem[h].hh.rh;
while n<mem[h+1].hh.lh do begin incr(n);p:=mem[p].hh.rh;end;repeat{368:}
qq:=mem[pp+1].hh.lh;
if qq>1 then if mem[p+1].hh.lh<=1 then mem[p+1].hh.lh:=qq else begin
while mem[qq].hh.rh>1 do qq:=mem[qq].hh.rh;
mem[qq].hh.rh:=mem[p+1].hh.lh;mem[p+1].hh.lh:=mem[pp+1].hh.lh;end;
mem[pp+1].hh.lh:=0;qq:=mem[pp+1].hh.rh;
if qq<>memtop then begin if mem[p+1].hh.lh=1 then mem[p+1].hh.lh:=0;
mem[pp+1].hh.rh:=memtop;r:=p+1;q:=mem[r].hh.rh;
if q=memtop then mem[p+1].hh.rh:=qq else while true do begin k:=mem[qq].
hh.lh;while k>mem[q].hh.lh do begin r:=q;q:=mem[r].hh.rh;end;
mem[r].hh.rh:=qq;rr:=mem[qq].hh.rh;mem[qq].hh.rh:=q;
if rr=memtop then goto 30;r:=qq;qq:=rr;end;end;30:{:368};
pp:=mem[pp].hh.rh;p:=mem[p].hh.rh;until pp=h;end;end;{:366}{369:}
function totalweight(h:halfword):integer;var p,q:halfword;n:integer;
m:0..65535;begin n:=0;p:=mem[h].hh.rh;
while p<>h do begin q:=mem[p+1].hh.rh;while q<>memtop do{370:}
begin m:=mem[q].hh.lh;n:=n-((m mod 8)-4)*(m div 8);q:=mem[q].hh.rh;
end{:370};q:=mem[p+1].hh.lh;while q>1 do{370:}begin m:=mem[q].hh.lh;
n:=n-((m mod 8)-4)*(m div 8);q:=mem[q].hh.rh;end{:370};p:=mem[p].hh.rh;
end;totalweight:=n;end;{:369}{372:}procedure beginedgetracing;
begin printdiagnostic(540,283,true);print(541);printint(curwt);
printchar(41);tracex:=-4096;end;procedure traceacorner;
begin if fileoffset>maxprintline-13 then printnl(283);printchar(40);
printint(tracex);printchar(44);printint(traceyy);printchar(41);
tracey:=traceyy;end;procedure endedgetracing;
begin if tracex=-4096 then printnl(542)else begin traceacorner;
printchar(46);end;enddiagnostic(true);end;{:372}{373:}
procedure tracenewedge(r:halfword;n:integer);var d:integer;w:-3..3;
m,n0,n1:integer;begin d:=mem[r].hh.lh;w:=(d mod 8)-4;
m:=(d div 8)-mem[curedges+3].hh.lh;if w=curwt then begin n0:=n+1;n1:=n;
end else begin n0:=n;n1:=n+1;end;
if m<>tracex then begin if tracex=-4096 then begin printnl(283);
traceyy:=n0;end else if traceyy<>n0 then printchar(63)else traceacorner;
tracex:=m;traceacorner;end else begin if n0<>traceyy then printchar(33);
if((n0<n1)and(tracey>traceyy))or((n0>n1)and(tracey<traceyy))then
traceacorner;end;traceyy:=n1;end;{:373}{374:}
procedure lineedges(x0,y0,x1,y1:scaled);label 30,31;
var m0,n0,m1,n1:integer;delx,dely:scaled;yt:scaled;tx:scaled;
p,r:halfword;base:integer;n:integer;begin n0:=roundunscaled(y0);
n1:=roundunscaled(y1);if n0<>n1 then begin m0:=roundunscaled(x0);
m1:=roundunscaled(x1);delx:=x1-x0;dely:=y1-y0;yt:=n0*65536-32768;
y0:=y0-yt;y1:=y1-yt;if n0<n1 then{375:}
begin base:=8*mem[curedges+3].hh.lh+4-curwt;
if m0<=m1 then edgeprep(m0,m1,n0,n1)else edgeprep(m1,m0,n0,n1);{377:}
n:=mem[curedges+5].hh.lh-4096;p:=mem[curedges+5].hh.rh;
if n<>n0 then if n<n0 then repeat incr(n);p:=mem[p].hh.rh;
until n=n0 else repeat decr(n);p:=mem[p].hh.lh;until n=n0{:377};
y0:=65536-y0;while true do begin r:=getavail;
mem[r].hh.rh:=mem[p+1].hh.lh;mem[p+1].hh.lh:=r;
tx:=takefraction(delx,makefraction(y0,dely));
if abvscd(delx,y0,dely,tx)<0 then decr(tx);
mem[r].hh.lh:=8*roundunscaled(x0+tx)+base;y1:=y1-65536;
if internal[10]>0 then tracenewedge(r,n);if y1<65536 then goto 30;
p:=mem[p].hh.rh;y0:=y0+65536;incr(n);end;30:end{:375}else{376:}
begin base:=8*mem[curedges+3].hh.lh+4+curwt;
if m0<=m1 then edgeprep(m0,m1,n1,n0)else edgeprep(m1,m0,n1,n0);decr(n0);
{377:}n:=mem[curedges+5].hh.lh-4096;p:=mem[curedges+5].hh.rh;
if n<>n0 then if n<n0 then repeat incr(n);p:=mem[p].hh.rh;
until n=n0 else repeat decr(n);p:=mem[p].hh.lh;until n=n0{:377};
while true do begin r:=getavail;mem[r].hh.rh:=mem[p+1].hh.lh;
mem[p+1].hh.lh:=r;tx:=takefraction(delx,makefraction(y0,dely));
if abvscd(delx,y0,dely,tx)<0 then incr(tx);
mem[r].hh.lh:=8*roundunscaled(x0-tx)+base;y1:=y1+65536;
if internal[10]>0 then tracenewedge(r,n);if y1>=0 then goto 31;
p:=mem[p].hh.lh;y0:=y0+65536;decr(n);end;31:end{:376};
mem[curedges+5].hh.rh:=p;mem[curedges+5].hh.lh:=n+4096;end;end;{:374}
{378:}procedure movetoedges(m0,n0,m1,n1:integer);label 60,61,62,63,30;
var delta:0..movesize;k:0..movesize;p,r:halfword;dx:integer;
edgeandweight:integer;j:integer;n:integer;ifdef('DEBUG')sum:integer;
endif('DEBUG')begin delta:=n1-n0;ifdef('DEBUG')sum:=move[0];
for k:=1 to delta do sum:=sum+abs(move[k]);
if sum<>m1-m0 then confusion(48);endif('DEBUG'){380:}
case octant of 1:begin dx:=8;edgeprep(m0,m1,n0,n1);goto 60;end;
5:begin dx:=8;edgeprep(n0,n1,m0,m1);goto 62;end;6:begin dx:=-8;
edgeprep(-n1,-n0,m0,m1);n0:=-n0;goto 62;end;2:begin dx:=-8;
edgeprep(-m1,-m0,n0,n1);m0:=-m0;goto 60;end;4:begin dx:=-8;
edgeprep(-m1,-m0,-n1,-n0);m0:=-m0;goto 61;end;8:begin dx:=-8;
edgeprep(-n1,-n0,-m1,-m0);n0:=-n0;goto 63;end;7:begin dx:=8;
edgeprep(n0,n1,-m1,-m0);goto 63;end;3:begin dx:=8;
edgeprep(m0,m1,-n1,-n0);goto 61;end;end;{:380};60:{381:}{377:}
n:=mem[curedges+5].hh.lh-4096;p:=mem[curedges+5].hh.rh;
if n<>n0 then if n<n0 then repeat incr(n);p:=mem[p].hh.rh;
until n=n0 else repeat decr(n);p:=mem[p].hh.lh;until n=n0{:377};
if delta>0 then begin k:=0;
edgeandweight:=8*(m0+mem[curedges+3].hh.lh)+4-curwt;
repeat edgeandweight:=edgeandweight+dx*move[k];begin r:=avail;
if r=0 then r:=getavail else begin avail:=mem[r].hh.rh;mem[r].hh.rh:=0;
ifdef('STAT')incr(dynused);endif('STAT')end;end;
mem[r].hh.rh:=mem[p+1].hh.lh;mem[r].hh.lh:=edgeandweight;
if internal[10]>0 then tracenewedge(r,n);mem[p+1].hh.lh:=r;
p:=mem[p].hh.rh;incr(k);incr(n);until k=delta;end;goto 30{:381};
61:{382:}n0:=-n0-1;{377:}n:=mem[curedges+5].hh.lh-4096;
p:=mem[curedges+5].hh.rh;if n<>n0 then if n<n0 then repeat incr(n);
p:=mem[p].hh.rh;until n=n0 else repeat decr(n);p:=mem[p].hh.lh;
until n=n0{:377};if delta>0 then begin k:=0;
edgeandweight:=8*(m0+mem[curedges+3].hh.lh)+4+curwt;
repeat edgeandweight:=edgeandweight+dx*move[k];begin r:=avail;
if r=0 then r:=getavail else begin avail:=mem[r].hh.rh;mem[r].hh.rh:=0;
ifdef('STAT')incr(dynused);endif('STAT')end;end;
mem[r].hh.rh:=mem[p+1].hh.lh;mem[r].hh.lh:=edgeandweight;
if internal[10]>0 then tracenewedge(r,n);mem[p+1].hh.lh:=r;
p:=mem[p].hh.lh;incr(k);decr(n);until k=delta;end;goto 30{:382};
62:{383:}edgeandweight:=8*(n0+mem[curedges+3].hh.lh)+4-curwt;n0:=m0;
k:=0;{377:}n:=mem[curedges+5].hh.lh-4096;p:=mem[curedges+5].hh.rh;
if n<>n0 then if n<n0 then repeat incr(n);p:=mem[p].hh.rh;
until n=n0 else repeat decr(n);p:=mem[p].hh.lh;until n=n0{:377};
repeat j:=move[k];while j>0 do begin begin r:=avail;
if r=0 then r:=getavail else begin avail:=mem[r].hh.rh;mem[r].hh.rh:=0;
ifdef('STAT')incr(dynused);endif('STAT')end;end;
mem[r].hh.rh:=mem[p+1].hh.lh;mem[r].hh.lh:=edgeandweight;
if internal[10]>0 then tracenewedge(r,n);mem[p+1].hh.lh:=r;
p:=mem[p].hh.rh;decr(j);incr(n);end;edgeandweight:=edgeandweight+dx;
incr(k);until k>delta;goto 30{:383};63:{384:}
edgeandweight:=8*(n0+mem[curedges+3].hh.lh)+4+curwt;n0:=-m0-1;k:=0;
{377:}n:=mem[curedges+5].hh.lh-4096;p:=mem[curedges+5].hh.rh;
if n<>n0 then if n<n0 then repeat incr(n);p:=mem[p].hh.rh;
until n=n0 else repeat decr(n);p:=mem[p].hh.lh;until n=n0{:377};
repeat j:=move[k];while j>0 do begin begin r:=avail;
if r=0 then r:=getavail else begin avail:=mem[r].hh.rh;mem[r].hh.rh:=0;
ifdef('STAT')incr(dynused);endif('STAT')end;end;
mem[r].hh.rh:=mem[p+1].hh.lh;mem[r].hh.lh:=edgeandweight;
if internal[10]>0 then tracenewedge(r,n);mem[p+1].hh.lh:=r;
p:=mem[p].hh.lh;decr(j);decr(n);end;edgeandweight:=edgeandweight+dx;
incr(k);until k>delta;goto 30{:384};30:mem[curedges+5].hh.lh:=n+4096;
mem[curedges+5].hh.rh:=p;end;{:378}{387:}procedure skew(x,y:scaled;
octant:smallnumber);begin case octant of 1:begin curx:=x-y;cury:=y;end;
5:begin curx:=y-x;cury:=x;end;6:begin curx:=y+x;cury:=-x;end;
2:begin curx:=-x-y;cury:=y;end;4:begin curx:=-x+y;cury:=-y;end;
8:begin curx:=-y+x;cury:=-x;end;7:begin curx:=-y-x;cury:=x;end;
3:begin curx:=x+y;cury:=-y;end;end;end;{:387}{390:}
procedure abnegate(x,y:scaled;octantbefore,octantafter:smallnumber);
begin if odd(octantbefore)=odd(octantafter)then curx:=x else curx:=-x;
if(octantbefore>2)=(octantafter>2)then cury:=y else cury:=-y;end;{:390}
{391:}function crossingpoint(a,b,c:integer):fraction;label 10;
var d:integer;x,xx,x0,x1,x2:integer;
begin if a<0 then begin crossingpoint:=0;goto 10;end;
if c>=0 then begin if b>=0 then if c>0 then begin crossingpoint:=
268435457;goto 10;
end else if(a=0)and(b=0)then begin crossingpoint:=268435457;goto 10;
end else begin crossingpoint:=268435456;goto 10;end;
if a=0 then begin crossingpoint:=0;goto 10;end;
end else if a=0 then if b<=0 then begin crossingpoint:=0;goto 10;end;
{392:}d:=1;x0:=a;x1:=a-b;x2:=b-c;repeat x:=(x1+x2)div 2;
if x1-x0>x0 then begin x2:=x;x0:=x0+x0;d:=d+d;
end else begin xx:=x1+x-x0;if xx>x0 then begin x2:=x;x0:=x0+x0;d:=d+d;
end else begin x0:=x0-xx;
if x<=x0 then if x+x2<=x0 then begin crossingpoint:=268435457;goto 10;
end;x1:=x;d:=d+d+1;end;end;until d>=268435456;
crossingpoint:=d-268435456{:392};10:end;{:391}{394:}
procedure printspec(s:strnumber);label 45,30;var p,q:halfword;
octant:smallnumber;begin printdiagnostic(543,s,true);p:=curspec;
octant:=mem[p+3].int;println;
unskew(mem[curspec+1].int,mem[curspec+2].int,octant);
printtwo(curx,cury);print(544);
while true do begin print(octantdir[octant]);printchar(39);
while true do begin q:=mem[p].hh.rh;if mem[p].hh.b1=0 then goto 45;
{397:}begin printnl(555);unskew(mem[p+5].int,mem[p+6].int,octant);
printtwo(curx,cury);print(522);unskew(mem[q+3].int,mem[q+4].int,octant);
printtwo(curx,cury);printnl(519);
unskew(mem[q+1].int,mem[q+2].int,octant);printtwo(curx,cury);print(556);
printint(mem[q].hh.b0-1);end{:397};p:=q;end;
45:if q=curspec then goto 30;p:=q;octant:=mem[p+3].int;printnl(545);end;
30:printnl(546);enddiagnostic(true);end;{:394}{398:}
procedure printstrange(s:strnumber);var p:halfword;f:halfword;
q:halfword;t:integer;begin if interaction=3 then;printnl(62);{399:}
p:=curspec;t:=256;repeat p:=mem[p].hh.rh;
if mem[p].hh.b0<>0 then begin if mem[p].hh.b0<t then f:=p;
t:=mem[p].hh.b0;end;until p=curspec{:399};{400:}p:=curspec;q:=p;
repeat p:=mem[p].hh.rh;if mem[p].hh.b0=0 then q:=p;until p=f{:400};t:=0;
repeat if mem[p].hh.b0<>0 then begin if mem[p].hh.b0<>t then begin t:=
mem[p].hh.b0;printchar(32);printint(t-1);end;if q<>0 then begin{401:}
if mem[mem[q].hh.rh].hh.b0=0 then begin print(557);
print(octantdir[mem[q+3].int]);q:=mem[q].hh.rh;
while mem[mem[q].hh.rh].hh.b0=0 do begin printchar(32);
print(octantdir[mem[q+3].int]);q:=mem[q].hh.rh;end;printchar(41);
end{:401};printchar(32);print(octantdir[mem[q+3].int]);q:=0;end;
end else if q=0 then q:=p;p:=mem[p].hh.rh;until p=f;printchar(32);
printint(mem[p].hh.b0-1);if q<>0 then{401:}
if mem[mem[q].hh.rh].hh.b0=0 then begin print(557);
print(octantdir[mem[q+3].int]);q:=mem[q].hh.rh;
while mem[mem[q].hh.rh].hh.b0=0 do begin printchar(32);
print(octantdir[mem[q+3].int]);q:=mem[q].hh.rh;end;printchar(41);
end{:401};begin if interaction=3 then;printnl(261);print(s);end;end;
{:398}{402:}{405:}procedure removecubic(p:halfword);var q:halfword;
begin q:=mem[p].hh.rh;mem[p].hh.b1:=mem[q].hh.b1;
mem[p].hh.rh:=mem[q].hh.rh;mem[p+1].int:=mem[q+1].int;
mem[p+2].int:=mem[q+2].int;mem[p+5].int:=mem[q+5].int;
mem[p+6].int:=mem[q+6].int;freenode(q,7);end;{:405}{406:}{410:}
procedure splitcubic(p:halfword;t:fraction;xq,yq:scaled);var v:scaled;
q,r:halfword;begin q:=mem[p].hh.rh;r:=getnode(7);mem[p].hh.rh:=r;
mem[r].hh.rh:=q;mem[r].hh.b0:=mem[q].hh.b0;mem[r].hh.b1:=mem[p].hh.b1;
v:=mem[p+5].int-takefraction(mem[p+5].int-mem[q+3].int,t);
mem[p+5].int:=mem[p+1].int-takefraction(mem[p+1].int-mem[p+5].int,t);
mem[q+3].int:=mem[q+3].int-takefraction(mem[q+3].int-xq,t);
mem[r+3].int:=mem[p+5].int-takefraction(mem[p+5].int-v,t);
mem[r+5].int:=v-takefraction(v-mem[q+3].int,t);
mem[r+1].int:=mem[r+3].int-takefraction(mem[r+3].int-mem[r+5].int,t);
v:=mem[p+6].int-takefraction(mem[p+6].int-mem[q+4].int,t);
mem[p+6].int:=mem[p+2].int-takefraction(mem[p+2].int-mem[p+6].int,t);
mem[q+4].int:=mem[q+4].int-takefraction(mem[q+4].int-yq,t);
mem[r+4].int:=mem[p+6].int-takefraction(mem[p+6].int-v,t);
mem[r+6].int:=v-takefraction(v-mem[q+4].int,t);
mem[r+2].int:=mem[r+4].int-takefraction(mem[r+4].int-mem[r+6].int,t);
end;{:410}procedure quadrantsubdivide;label 22,10;
var p,q,r,s,pp,qq:halfword;firstx,firsty:scaled;
del1,del2,del3,del,dmax:scaled;t:fraction;destx,desty:scaled;
constantx:boolean;begin p:=curspec;firstx:=mem[curspec+1].int;
firsty:=mem[curspec+2].int;repeat 22:q:=mem[p].hh.rh;{407:}
if q=curspec then begin destx:=firstx;desty:=firsty;
end else begin destx:=mem[q+1].int;desty:=mem[q+2].int;end;
del1:=mem[p+5].int-mem[p+1].int;del2:=mem[q+3].int-mem[p+5].int;
del3:=destx-mem[q+3].int;{408:}
if del1<>0 then del:=del1 else if del2<>0 then del:=del2 else del:=del3;
if del<>0 then begin dmax:=abs(del1);
if abs(del2)>dmax then dmax:=abs(del2);
if abs(del3)>dmax then dmax:=abs(del3);
while dmax<134217728 do begin dmax:=dmax+dmax;del1:=del1+del1;
del2:=del2+del2;del3:=del3+del3;end;end{:408};
if del=0 then constantx:=true else begin constantx:=false;
if del<0 then{409:}begin mem[p+1].int:=-mem[p+1].int;
mem[p+5].int:=-mem[p+5].int;mem[q+3].int:=-mem[q+3].int;del1:=-del1;
del2:=-del2;del3:=-del3;destx:=-destx;mem[p].hh.b1:=2;end{:409};
t:=crossingpoint(del1,del2,del3);if t<268435456 then{411:}
begin splitcubic(p,t,destx,desty);r:=mem[p].hh.rh;
if mem[r].hh.b1>1 then mem[r].hh.b1:=1 else mem[r].hh.b1:=2;
if mem[r+1].int<mem[p+1].int then mem[r+1].int:=mem[p+1].int;
mem[r+3].int:=mem[r+1].int;
if mem[p+5].int>mem[r+1].int then mem[p+5].int:=mem[r+1].int;
mem[r+1].int:=-mem[r+1].int;mem[r+5].int:=mem[r+1].int;
mem[q+3].int:=-mem[q+3].int;destx:=-destx;
del2:=del2-takefraction(del2-del3,t);if del2>0 then del2:=0;
t:=crossingpoint(0,-del2,-del3);if t<268435456 then{412:}
begin splitcubic(r,t,destx,desty);s:=mem[r].hh.rh;
if mem[s+1].int<destx then mem[s+1].int:=destx;
if mem[s+1].int<mem[r+1].int then mem[s+1].int:=mem[r+1].int;
mem[s].hh.b1:=mem[p].hh.b1;mem[s+3].int:=mem[s+1].int;
if mem[q+3].int<destx then mem[q+3].int:=-destx else if mem[q+3].int>mem
[s+1].int then mem[q+3].int:=-mem[s+1].int else mem[q+3].int:=-mem[q+3].
int;mem[s+1].int:=-mem[s+1].int;mem[s+5].int:=mem[s+1].int;end{:412}
else begin if mem[r+1].int>destx then begin mem[r+1].int:=destx;
mem[r+3].int:=-mem[r+1].int;mem[r+5].int:=mem[r+1].int;end;
if mem[q+3].int>destx then mem[q+3].int:=destx else if mem[q+3].int<mem[
r+1].int then mem[q+3].int:=mem[r+1].int;end;end{:411};end{:407};{413:}
pp:=p;repeat qq:=mem[pp].hh.rh;
abnegate(mem[qq+1].int,mem[qq+2].int,mem[qq].hh.b1,mem[pp].hh.b1);
destx:=curx;desty:=cury;del1:=mem[pp+6].int-mem[pp+2].int;
del2:=mem[qq+4].int-mem[pp+6].int;del3:=desty-mem[qq+4].int;{408:}
if del1<>0 then del:=del1 else if del2<>0 then del:=del2 else del:=del3;
if del<>0 then begin dmax:=abs(del1);
if abs(del2)>dmax then dmax:=abs(del2);
if abs(del3)>dmax then dmax:=abs(del3);
while dmax<134217728 do begin dmax:=dmax+dmax;del1:=del1+del1;
del2:=del2+del2;del3:=del3+del3;end;end{:408};
if del<>0 then begin if del<0 then{414:}
begin mem[pp+2].int:=-mem[pp+2].int;mem[pp+6].int:=-mem[pp+6].int;
mem[qq+4].int:=-mem[qq+4].int;del1:=-del1;del2:=-del2;del3:=-del3;
desty:=-desty;mem[pp].hh.b1:=mem[pp].hh.b1+2;end{:414};
t:=crossingpoint(del1,del2,del3);if t<268435456 then{415:}
begin splitcubic(pp,t,destx,desty);r:=mem[pp].hh.rh;
if mem[r].hh.b1>2 then mem[r].hh.b1:=mem[r].hh.b1-2 else mem[r].hh.b1:=
mem[r].hh.b1+2;
if mem[r+2].int<mem[pp+2].int then mem[r+2].int:=mem[pp+2].int;
mem[r+4].int:=mem[r+2].int;
if mem[pp+6].int>mem[r+2].int then mem[pp+6].int:=mem[r+2].int;
mem[r+2].int:=-mem[r+2].int;mem[r+6].int:=mem[r+2].int;
mem[qq+4].int:=-mem[qq+4].int;desty:=-desty;
if mem[r+1].int<mem[pp+1].int then mem[r+1].int:=mem[pp+1].int else if
mem[r+1].int>destx then mem[r+1].int:=destx;
if mem[r+3].int>mem[r+1].int then begin mem[r+3].int:=mem[r+1].int;
if mem[pp+5].int>mem[r+1].int then mem[pp+5].int:=mem[r+1].int;end;
if mem[r+5].int<mem[r+1].int then begin mem[r+5].int:=mem[r+1].int;
if mem[qq+3].int<mem[r+1].int then mem[qq+3].int:=mem[r+1].int;end;
del2:=del2-takefraction(del2-del3,t);if del2>0 then del2:=0;
t:=crossingpoint(0,-del2,-del3);if t<268435456 then{416:}
begin splitcubic(r,t,destx,desty);s:=mem[r].hh.rh;
if mem[s+2].int<desty then mem[s+2].int:=desty;
if mem[s+2].int<mem[r+2].int then mem[s+2].int:=mem[r+2].int;
mem[s].hh.b1:=mem[pp].hh.b1;mem[s+4].int:=mem[s+2].int;
if mem[qq+4].int<desty then mem[qq+4].int:=-desty else if mem[qq+4].int>
mem[s+2].int then mem[qq+4].int:=-mem[s+2].int else mem[qq+4].int:=-mem[
qq+4].int;mem[s+2].int:=-mem[s+2].int;mem[s+6].int:=mem[s+2].int;
if mem[s+1].int<mem[r+1].int then mem[s+1].int:=mem[r+1].int else if mem
[s+1].int>destx then mem[s+1].int:=destx;
if mem[s+3].int>mem[s+1].int then begin mem[s+3].int:=mem[s+1].int;
if mem[r+5].int>mem[s+1].int then mem[r+5].int:=mem[s+1].int;end;
if mem[s+5].int<mem[s+1].int then begin mem[s+5].int:=mem[s+1].int;
if mem[qq+3].int<mem[s+1].int then mem[qq+3].int:=mem[s+1].int;end;
end{:416}
else begin if mem[r+2].int>desty then begin mem[r+2].int:=desty;
mem[r+4].int:=-mem[r+2].int;mem[r+6].int:=mem[r+2].int;end;
if mem[qq+4].int>desty then mem[qq+4].int:=desty else if mem[qq+4].int<
mem[r+2].int then mem[qq+4].int:=mem[r+2].int;end;end{:415};
end else{417:}if constantx then begin if q<>p then begin removecubic(p);
if curspec<>q then goto 22 else begin curspec:=p;goto 10;end;end;
end else if not odd(mem[pp].hh.b1)then{414:}
begin mem[pp+2].int:=-mem[pp+2].int;mem[pp+6].int:=-mem[pp+6].int;
mem[qq+4].int:=-mem[qq+4].int;del1:=-del1;del2:=-del2;del3:=-del3;
desty:=-desty;mem[pp].hh.b1:=mem[pp].hh.b1+2;end{:414}{:417};pp:=qq;
until pp=q;if constantx then{418:}begin pp:=p;repeat qq:=mem[pp].hh.rh;
if mem[pp].hh.b1>2 then begin mem[pp].hh.b1:=mem[pp].hh.b1+1;
mem[pp+1].int:=-mem[pp+1].int;mem[pp+5].int:=-mem[pp+5].int;
mem[qq+3].int:=-mem[qq+3].int;end;pp:=qq;until pp=q;end{:418}{:413};
p:=q;until p=curspec;10:end;{:406}{419:}procedure octantsubdivide;
var p,q,r,s:halfword;del1,del2,del3,del,dmax:scaled;t:fraction;
destx,desty:scaled;begin p:=curspec;repeat q:=mem[p].hh.rh;
mem[p+1].int:=mem[p+1].int-mem[p+2].int;
mem[p+5].int:=mem[p+5].int-mem[p+6].int;
mem[q+3].int:=mem[q+3].int-mem[q+4].int;{420:}{421:}
if q=curspec then begin unskew(mem[q+1].int,mem[q+2].int,mem[q].hh.b1);
skew(curx,cury,mem[p].hh.b1);destx:=curx;desty:=cury;
end else begin abnegate(mem[q+1].int,mem[q+2].int,mem[q].hh.b1,mem[p].hh
.b1);destx:=curx-cury;desty:=cury;end;del1:=mem[p+5].int-mem[p+1].int;
del2:=mem[q+3].int-mem[p+5].int;del3:=destx-mem[q+3].int{:421};{408:}
if del1<>0 then del:=del1 else if del2<>0 then del:=del2 else del:=del3;
if del<>0 then begin dmax:=abs(del1);
if abs(del2)>dmax then dmax:=abs(del2);
if abs(del3)>dmax then dmax:=abs(del3);
while dmax<134217728 do begin dmax:=dmax+dmax;del1:=del1+del1;
del2:=del2+del2;del3:=del3+del3;end;end{:408};
if del<>0 then begin if del<0 then{423:}
begin mem[p+2].int:=mem[p+1].int+mem[p+2].int;
mem[p+1].int:=-mem[p+1].int;mem[p+6].int:=mem[p+5].int+mem[p+6].int;
mem[p+5].int:=-mem[p+5].int;mem[q+4].int:=mem[q+3].int+mem[q+4].int;
mem[q+3].int:=-mem[q+3].int;del1:=-del1;del2:=-del2;del3:=-del3;
desty:=destx+desty;destx:=-destx;mem[p].hh.b1:=mem[p].hh.b1+4;end{:423};
t:=crossingpoint(del1,del2,del3);if t<268435456 then{424:}
begin splitcubic(p,t,destx,desty);r:=mem[p].hh.rh;
if mem[r].hh.b1>4 then mem[r].hh.b1:=mem[r].hh.b1-4 else mem[r].hh.b1:=
mem[r].hh.b1+4;
if mem[r+2].int<mem[p+2].int then mem[r+2].int:=mem[p+2].int else if mem
[r+2].int>desty then mem[r+2].int:=desty;
if mem[p+1].int+mem[r+2].int>destx+desty then mem[r+2].int:=destx+desty-
mem[p+1].int;
if mem[r+4].int>mem[r+2].int then begin mem[r+4].int:=mem[r+2].int;
if mem[p+6].int>mem[r+2].int then mem[p+6].int:=mem[r+2].int;end;
if mem[r+6].int<mem[r+2].int then begin mem[r+6].int:=mem[r+2].int;
if mem[q+4].int<mem[r+2].int then mem[q+4].int:=mem[r+2].int;end;
if mem[r+1].int<mem[p+1].int then mem[r+1].int:=mem[p+1].int else if mem
[r+1].int+mem[r+2].int>destx+desty then mem[r+1].int:=destx+desty-mem[r
+2].int;mem[r+3].int:=mem[r+1].int;
if mem[p+5].int>mem[r+1].int then mem[p+5].int:=mem[r+1].int;
mem[r+2].int:=mem[r+2].int+mem[r+1].int;
mem[r+6].int:=mem[r+6].int+mem[r+1].int;mem[r+1].int:=-mem[r+1].int;
mem[r+5].int:=mem[r+1].int;mem[q+4].int:=mem[q+4].int+mem[q+3].int;
mem[q+3].int:=-mem[q+3].int;desty:=desty+destx;destx:=-destx;
if mem[r+6].int<mem[r+2].int then begin mem[r+6].int:=mem[r+2].int;
if mem[q+4].int<mem[r+2].int then mem[q+4].int:=mem[r+2].int;end;
del2:=del2-takefraction(del2-del3,t);if del2>0 then del2:=0;
t:=crossingpoint(0,-del2,-del3);if t<268435456 then{425:}
begin splitcubic(r,t,destx,desty);s:=mem[r].hh.rh;
if mem[s+2].int<mem[r+2].int then mem[s+2].int:=mem[r+2].int else if mem
[s+2].int>desty then mem[s+2].int:=desty;
if mem[r+1].int+mem[s+2].int>destx+desty then mem[s+2].int:=destx+desty-
mem[r+1].int;
if mem[s+4].int>mem[s+2].int then begin mem[s+4].int:=mem[s+2].int;
if mem[r+6].int>mem[s+2].int then mem[r+6].int:=mem[s+2].int;end;
if mem[s+6].int<mem[s+2].int then begin mem[s+6].int:=mem[s+2].int;
if mem[q+4].int<mem[s+2].int then mem[q+4].int:=mem[s+2].int;end;
if mem[s+1].int+mem[s+2].int>destx+desty then mem[s+1].int:=destx+desty-
mem[s+2].int else begin if mem[s+1].int<destx then mem[s+1].int:=destx;
if mem[s+1].int<mem[r+1].int then mem[s+1].int:=mem[r+1].int;end;
mem[s].hh.b1:=mem[p].hh.b1;mem[s+3].int:=mem[s+1].int;
if mem[q+3].int<destx then begin mem[q+4].int:=mem[q+4].int+destx;
mem[q+3].int:=-destx;
end else if mem[q+3].int>mem[s+1].int then begin mem[q+4].int:=mem[q+4].
int+mem[s+1].int;mem[q+3].int:=-mem[s+1].int;
end else begin mem[q+4].int:=mem[q+4].int+mem[q+3].int;
mem[q+3].int:=-mem[q+3].int;end;mem[s+2].int:=mem[s+2].int+mem[s+1].int;
mem[s+6].int:=mem[s+6].int+mem[s+1].int;mem[s+1].int:=-mem[s+1].int;
mem[s+5].int:=mem[s+1].int;
if mem[s+6].int<mem[s+2].int then begin mem[s+6].int:=mem[s+2].int;
if mem[q+4].int<mem[s+2].int then mem[q+4].int:=mem[s+2].int;end;
end{:425}
else begin if mem[r+1].int>destx then begin mem[r+1].int:=destx;
mem[r+3].int:=-mem[r+1].int;mem[r+5].int:=mem[r+1].int;end;
if mem[q+3].int>destx then mem[q+3].int:=destx else if mem[q+3].int<mem[
r+1].int then mem[q+3].int:=mem[r+1].int;end;end{:424};end{:420};p:=q;
until p=curspec;end;{:419}{426:}procedure makesafe;var k:0..maxwiggle;
allsafe:boolean;nexta:scaled;deltaa,deltab:scaled;
begin before[curroundingptr]:=before[0];
nodetoround[curroundingptr]:=nodetoround[0];
repeat after[curroundingptr]:=after[0];allsafe:=true;nexta:=after[0];
for k:=0 to curroundingptr-1 do begin deltab:=before[k+1]-before[k];
if deltab>=0 then deltaa:=after[k+1]-nexta else deltaa:=nexta-after[k+1]
;nexta:=after[k+1];
if(deltaa<0)or(deltaa>abs(deltab+deltab))then begin allsafe:=false;
after[k]:=before[k];
if k=curroundingptr-1 then after[0]:=before[0]else after[k+1]:=before[k
+1];end;end;until allsafe;end;{:426}{429:}
procedure beforeandafter(b,a:scaled;p:halfword);
begin if curroundingptr=maxroundingptr then if maxroundingptr<maxwiggle
then incr(maxroundingptr)else overflow(567,maxwiggle);
after[curroundingptr]:=a;before[curroundingptr]:=b;
nodetoround[curroundingptr]:=p;incr(curroundingptr);end;{:429}{431:}
function goodval(b,o:scaled):scaled;var a:scaled;begin a:=b+o;
if a>=0 then a:=a-(a mod curgran)-o else a:=a+((-(a+1))mod curgran)-
curgran+1-o;if b-a<a+curgran-b then goodval:=a else goodval:=a+curgran;
end;{:431}{432:}function compromise(u,v:scaled):scaled;
begin compromise:=(goodval(u+u,-u-v))div 2;end;{:432}{433:}
procedure xyround;var p,q:halfword;b,a:scaled;penedge:scaled;
alpha:fraction;begin curgran:=abs(internal[37]);
if curgran=0 then curgran:=65536;p:=curspec;curroundingptr:=0;
repeat q:=mem[p].hh.rh;{434:}
if odd(mem[p].hh.b1)<>odd(mem[q].hh.b1)then begin if odd(mem[q].hh.b1)
then b:=mem[q+1].int else b:=-mem[q+1].int;
if(abs(mem[q+1].int-mem[q+5].int)<655)or(abs(mem[q+1].int+mem[q+3].int)<
655)then{435:}
begin if curpen=3 then penedge:=0 else if curpathtype=0 then penedge:=
compromise(mem[mem[curpen+5].hh.rh+2].int,mem[mem[curpen+7].hh.rh+2].int
)else if odd(mem[q].hh.b1)then penedge:=mem[mem[curpen+7].hh.rh+2].int
else penedge:=mem[mem[curpen+5].hh.rh+2].int;a:=goodval(b,penedge);
end{:435}else a:=b;
if abs(a)>maxallowed then if a>0 then a:=maxallowed else a:=-maxallowed;
beforeandafter(b,a,q);end{:434};p:=q;until p=curspec;
if curroundingptr>0 then{436:}begin makesafe;
repeat decr(curroundingptr);
if(after[curroundingptr]<>before[curroundingptr])or(after[curroundingptr
+1]<>before[curroundingptr+1])then begin p:=nodetoround[curroundingptr];
if odd(mem[p].hh.b1)then begin b:=before[curroundingptr];
a:=after[curroundingptr];end else begin b:=-before[curroundingptr];
a:=-after[curroundingptr];end;
if before[curroundingptr]=before[curroundingptr+1]then alpha:=268435456
else alpha:=makefraction(after[curroundingptr+1]-after[curroundingptr],
before[curroundingptr+1]-before[curroundingptr]);
repeat mem[p+1].int:=takefraction(alpha,mem[p+1].int-b)+a;
mem[p+5].int:=takefraction(alpha,mem[p+5].int-b)+a;p:=mem[p].hh.rh;
mem[p+3].int:=takefraction(alpha,mem[p+3].int-b)+a;
until p=nodetoround[curroundingptr+1];end;until curroundingptr=0;
end{:436};p:=curspec;curroundingptr:=0;repeat q:=mem[p].hh.rh;{437:}
if(mem[p].hh.b1>2)<>(mem[q].hh.b1>2)then begin if mem[q].hh.b1<=2 then b
:=mem[q+2].int else b:=-mem[q+2].int;
if(abs(mem[q+2].int-mem[q+6].int)<655)or(abs(mem[q+2].int+mem[q+4].int)<
655)then{438:}
begin if curpen=3 then penedge:=0 else if curpathtype=0 then penedge:=
compromise(mem[mem[curpen+2].hh.rh+2].int,mem[mem[curpen+1].hh.rh+2].int
)else if mem[q].hh.b1<=2 then penedge:=mem[mem[curpen+1].hh.rh+2].int
else penedge:=mem[mem[curpen+2].hh.rh+2].int;a:=goodval(b,penedge);
end{:438}else a:=b;
if abs(a)>maxallowed then if a>0 then a:=maxallowed else a:=-maxallowed;
beforeandafter(b,a,q);end{:437};p:=q;until p=curspec;
if curroundingptr>0 then{439:}begin makesafe;
repeat decr(curroundingptr);
if(after[curroundingptr]<>before[curroundingptr])or(after[curroundingptr
+1]<>before[curroundingptr+1])then begin p:=nodetoround[curroundingptr];
if mem[p].hh.b1<=2 then begin b:=before[curroundingptr];
a:=after[curroundingptr];end else begin b:=-before[curroundingptr];
a:=-after[curroundingptr];end;
if before[curroundingptr]=before[curroundingptr+1]then alpha:=268435456
else alpha:=makefraction(after[curroundingptr+1]-after[curroundingptr],
before[curroundingptr+1]-before[curroundingptr]);
repeat mem[p+2].int:=takefraction(alpha,mem[p+2].int-b)+a;
mem[p+6].int:=takefraction(alpha,mem[p+6].int-b)+a;p:=mem[p].hh.rh;
mem[p+4].int:=takefraction(alpha,mem[p+4].int-b)+a;
until p=nodetoround[curroundingptr+1];end;until curroundingptr=0;
end{:439};end;{:433}{440:}procedure diaground;var p,q,pp:halfword;
b,a,bb,aa,d,c,dd,cc:scaled;penedge:scaled;alpha,beta:fraction;
nexta:scaled;allsafe:boolean;k:0..maxwiggle;firstx,firsty:scaled;
begin p:=curspec;curroundingptr:=0;repeat q:=mem[p].hh.rh;{441:}
if mem[p].hh.b1<>mem[q].hh.b1 then begin if mem[q].hh.b1>4 then b:=-mem[
q+1].int else b:=mem[q+1].int;
if abs(mem[q].hh.b1-mem[p].hh.b1)=4 then if(abs(mem[q+1].int-mem[q+5].
int)<655)or(abs(mem[q+1].int+mem[q+3].int)<655)then{442:}
begin if curpen=3 then penedge:=0 else if curpathtype=0 then{443:}
case mem[q].hh.b1 of 1,5:penedge:=compromise(mem[mem[mem[curpen+1].hh.rh
].hh.lh+1].int,-mem[mem[mem[curpen+4].hh.rh].hh.lh+1].int);
4,8:penedge:=-compromise(mem[mem[mem[curpen+1].hh.rh].hh.lh+1].int,-mem[
mem[mem[curpen+4].hh.rh].hh.lh+1].int);
6,2:penedge:=compromise(mem[mem[mem[curpen+2].hh.rh].hh.lh+1].int,-mem[
mem[mem[curpen+3].hh.rh].hh.lh+1].int);
7,3:penedge:=-compromise(mem[mem[mem[curpen+2].hh.rh].hh.lh+1].int,-mem[
mem[mem[curpen+3].hh.rh].hh.lh+1].int);end{:443}
else if mem[q].hh.b1<=4 then penedge:=mem[mem[mem[curpen+mem[q].hh.b1].
hh.rh].hh.lh+1].int else penedge:=-mem[mem[mem[curpen+mem[q].hh.b1].hh.
rh].hh.lh+1].int;
if odd(mem[q].hh.b1)then a:=goodval(b,penedge+(curgran)div 2)else a:=
goodval(b-1,penedge+(curgran)div 2);end{:442}else a:=b else a:=b;
beforeandafter(b,a,q);end{:441};p:=q;until p=curspec;
if curroundingptr>0 then{444:}begin p:=nodetoround[0];
firstx:=mem[p+1].int;firsty:=mem[p+2].int;{446:}
before[curroundingptr]:=before[0];
nodetoround[curroundingptr]:=nodetoround[0];
repeat after[curroundingptr]:=after[0];allsafe:=true;nexta:=after[0];
for k:=0 to curroundingptr-1 do begin a:=nexta;b:=before[k];
nexta:=after[k+1];aa:=nexta;bb:=before[k+1];
if(a<>b)or(aa<>bb)then begin p:=nodetoround[k];pp:=nodetoround[k+1];
{445:}
if aa=bb then begin if pp=nodetoround[0]then unskew(firstx,firsty,mem[pp
].hh.b1)else unskew(mem[pp+1].int,mem[pp+2].int,mem[pp].hh.b1);
skew(curx,cury,mem[p].hh.b1);bb:=curx;aa:=bb;dd:=cury;cc:=dd;
if mem[p].hh.b1>4 then begin b:=-b;a:=-a;end;
end else begin if mem[p].hh.b1>4 then begin bb:=-bb;aa:=-aa;b:=-b;a:=-a;
end;if pp=nodetoround[0]then dd:=firsty-bb else dd:=mem[pp+2].int-bb;
if odd(aa-bb)then if mem[p].hh.b1>4 then cc:=dd-(aa-bb+1)div 2 else cc:=
dd-(aa-bb-1)div 2 else cc:=dd-(aa-bb)div 2;end;d:=mem[p+2].int;
if odd(a-b)then if mem[p].hh.b1>4 then c:=d-(a-b-1)div 2 else c:=d-(a-b
+1)div 2 else c:=d-(a-b)div 2{:445};
if(aa<a)or(cc<c)or(aa-a>2*(bb-b))or(cc-c>2*(dd-d))then begin allsafe:=
false;after[k]:=before[k];
if k=curroundingptr-1 then after[0]:=before[0]else after[k+1]:=before[k
+1];end;end;end;until allsafe{:446};
for k:=0 to curroundingptr-1 do begin a:=after[k];b:=before[k];
aa:=after[k+1];bb:=before[k+1];
if(a<>b)or(aa<>bb)then begin p:=nodetoround[k];pp:=nodetoround[k+1];
{445:}
if aa=bb then begin if pp=nodetoround[0]then unskew(firstx,firsty,mem[pp
].hh.b1)else unskew(mem[pp+1].int,mem[pp+2].int,mem[pp].hh.b1);
skew(curx,cury,mem[p].hh.b1);bb:=curx;aa:=bb;dd:=cury;cc:=dd;
if mem[p].hh.b1>4 then begin b:=-b;a:=-a;end;
end else begin if mem[p].hh.b1>4 then begin bb:=-bb;aa:=-aa;b:=-b;a:=-a;
end;if pp=nodetoround[0]then dd:=firsty-bb else dd:=mem[pp+2].int-bb;
if odd(aa-bb)then if mem[p].hh.b1>4 then cc:=dd-(aa-bb+1)div 2 else cc:=
dd-(aa-bb-1)div 2 else cc:=dd-(aa-bb)div 2;end;d:=mem[p+2].int;
if odd(a-b)then if mem[p].hh.b1>4 then c:=d-(a-b-1)div 2 else c:=d-(a-b
+1)div 2 else c:=d-(a-b)div 2{:445};
if b=bb then alpha:=268435456 else alpha:=makefraction(aa-a,bb-b);
if d=dd then beta:=268435456 else beta:=makefraction(cc-c,dd-d);
repeat mem[p+1].int:=takefraction(alpha,mem[p+1].int-b)+a;
mem[p+2].int:=takefraction(beta,mem[p+2].int-d)+c;
mem[p+5].int:=takefraction(alpha,mem[p+5].int-b)+a;
mem[p+6].int:=takefraction(beta,mem[p+6].int-d)+c;p:=mem[p].hh.rh;
mem[p+3].int:=takefraction(alpha,mem[p+3].int-b)+a;
mem[p+4].int:=takefraction(beta,mem[p+4].int-d)+c;until p=pp;end;end;
end{:444};end;{:440}{451:}procedure newboundary(p:halfword;
octant:smallnumber);var q,r:halfword;begin q:=mem[p].hh.rh;
r:=getnode(7);mem[r].hh.rh:=q;mem[p].hh.rh:=r;
mem[r].hh.b0:=mem[q].hh.b0;mem[r+3].int:=mem[q+3].int;
mem[r+4].int:=mem[q+4].int;mem[r].hh.b1:=0;mem[q].hh.b0:=0;
mem[r+5].int:=octant;mem[q+3].int:=mem[q].hh.b1;
unskew(mem[q+1].int,mem[q+2].int,mem[q].hh.b1);skew(curx,cury,octant);
mem[r+1].int:=curx;mem[r+2].int:=cury;end;{:451}
function makespec(h:halfword;safetymargin:scaled;
tracing:integer):halfword;label 22,30;var p,q,r,s:halfword;k:integer;
chopped:boolean;{453:}o1,o2:smallnumber;clockwise:boolean;
dx1,dy1,dx2,dy2:integer;dmax,del:integer;{:453}begin curspec:=h;
if tracing>0 then printpath(curspec,558,true);
maxallowed:=268402687-safetymargin;{404:}p:=curspec;k:=1;chopped:=false;
repeat if abs(mem[p+3].int)>maxallowed then begin chopped:=true;
if mem[p+3].int>0 then mem[p+3].int:=maxallowed else mem[p+3].int:=-
maxallowed;end;if abs(mem[p+4].int)>maxallowed then begin chopped:=true;
if mem[p+4].int>0 then mem[p+4].int:=maxallowed else mem[p+4].int:=-
maxallowed;end;if abs(mem[p+1].int)>maxallowed then begin chopped:=true;
if mem[p+1].int>0 then mem[p+1].int:=maxallowed else mem[p+1].int:=-
maxallowed;end;if abs(mem[p+2].int)>maxallowed then begin chopped:=true;
if mem[p+2].int>0 then mem[p+2].int:=maxallowed else mem[p+2].int:=-
maxallowed;end;if abs(mem[p+5].int)>maxallowed then begin chopped:=true;
if mem[p+5].int>0 then mem[p+5].int:=maxallowed else mem[p+5].int:=-
maxallowed;end;if abs(mem[p+6].int)>maxallowed then begin chopped:=true;
if mem[p+6].int>0 then mem[p+6].int:=maxallowed else mem[p+6].int:=-
maxallowed;end;p:=mem[p].hh.rh;mem[p].hh.b0:=k;
if k<255 then incr(k)else k:=1;until p=curspec;
if chopped then begin begin if interaction=3 then;printnl(261);
print(562);end;begin helpptr:=4;helpline[3]:=563;helpline[2]:=564;
helpline[1]:=565;helpline[0]:=566;end;putgeterror;end{:404};
quadrantsubdivide;if internal[36]>0 then xyround;octantsubdivide;
if internal[36]>65536 then diaground;{447:}p:=curspec;
repeat 22:q:=mem[p].hh.rh;
if p<>q then begin if mem[p+1].int=mem[p+5].int then if mem[p+2].int=mem
[p+6].int then if mem[p+1].int=mem[q+3].int then if mem[p+2].int=mem[q+4
].int then begin unskew(mem[q+1].int,mem[q+2].int,mem[q].hh.b1);
skew(curx,cury,mem[p].hh.b1);
if mem[p+1].int=curx then if mem[p+2].int=cury then begin removecubic(p)
;if q<>curspec then goto 22;curspec:=p;q:=p;end;end;end;p:=q;
until p=curspec;{:447};{450:}turningnumber:=0;p:=curspec;
q:=mem[p].hh.rh;repeat r:=mem[q].hh.rh;
if(mem[p].hh.b1<>mem[q].hh.b1)or(q=r)then{452:}
begin newboundary(p,mem[p].hh.b1);s:=mem[p].hh.rh;
o1:=octantnumber[mem[p].hh.b1];o2:=octantnumber[mem[q].hh.b1];
case o2-o1 of 1,-7,7,-1:goto 30;2,-6:clockwise:=false;
3,-5,4,-4,5,-3:{454:}begin{457:}dx1:=mem[s+1].int-mem[s+3].int;
dy1:=mem[s+2].int-mem[s+4].int;
if dx1=0 then if dy1=0 then begin dx1:=mem[s+1].int-mem[p+5].int;
dy1:=mem[s+2].int-mem[p+6].int;
if dx1=0 then if dy1=0 then begin dx1:=mem[s+1].int-mem[p+1].int;
dy1:=mem[s+2].int-mem[p+2].int;end;end;dmax:=abs(dx1);
if abs(dy1)>dmax then dmax:=abs(dy1);
while dmax<268435456 do begin dmax:=dmax+dmax;dx1:=dx1+dx1;dy1:=dy1+dy1;
end;dx2:=mem[q+5].int-mem[q+1].int;dy2:=mem[q+6].int-mem[q+2].int;
if dx2=0 then if dy2=0 then begin dx2:=mem[r+3].int-mem[q+1].int;
dy2:=mem[r+4].int-mem[q+2].int;
if dx2=0 then if dy2=0 then begin if mem[r].hh.b1=0 then begin curx:=mem
[r+1].int;cury:=mem[r+2].int;
end else begin unskew(mem[r+1].int,mem[r+2].int,mem[r].hh.b1);
skew(curx,cury,mem[q].hh.b1);end;dx2:=curx-mem[q+1].int;
dy2:=cury-mem[q+2].int;end;end;dmax:=abs(dx2);
if abs(dy2)>dmax then dmax:=abs(dy2);
while dmax<268435456 do begin dmax:=dmax+dmax;dx2:=dx2+dx2;dy2:=dy2+dy2;
end{:457};unskew(dx1,dy1,mem[p].hh.b1);del:=pythadd(curx,cury);
dx1:=makefraction(curx,del);dy1:=makefraction(cury,del);
unskew(dx2,dy2,mem[q].hh.b1);del:=pythadd(curx,cury);
dx2:=makefraction(curx,del);dy2:=makefraction(cury,del);
del:=takefraction(dx1,dy2)-takefraction(dx2,dy1);
if del>4684844 then clockwise:=false else if del<-4684844 then clockwise
:=true else clockwise:=revturns;end{:454};6,-2:clockwise:=true;
0:clockwise:=revturns;end;{458:}
while true do begin if clockwise then if o1=1 then o1:=8 else decr(o1)
else if o1=8 then o1:=1 else incr(o1);if o1=o2 then goto 30;
newboundary(s,octantcode[o1]);s:=mem[s].hh.rh;
mem[s+3].int:=mem[s+5].int;end{:458};
30:if q=r then begin q:=mem[q].hh.rh;r:=q;p:=s;mem[s].hh.rh:=q;
mem[q+3].int:=mem[q+5].int;mem[q].hh.b0:=0;freenode(curspec,7);
curspec:=q;end;{459:}p:=mem[p].hh.rh;repeat s:=mem[p].hh.rh;
o1:=octantnumber[mem[p+5].int];o2:=octantnumber[mem[s+3].int];
if abs(o1-o2)=1 then begin if o2<o1 then o2:=o1;
if odd(o2)then mem[p+6].int:=0 else mem[p+6].int:=1;
end else begin if o1=8 then incr(turningnumber)else decr(turningnumber);
mem[p+6].int:=0;end;mem[s+4].int:=mem[p+6].int;p:=s;until p=q{:459};
end{:452};p:=q;q:=r;until p=curspec;{:450};
while mem[curspec].hh.b0<>0 do curspec:=mem[curspec].hh.rh;
if tracing>0 then if internal[36]<=0 then printspec(559)else if internal
[36]>65536 then printspec(560)else printspec(561);makespec:=curspec;end;
{:402}{463:}procedure endround(x,y:scaled);
begin y:=y+32768-ycorr[octant];x:=x+y-xcorr[octant];
m1:=floorunscaled(x);n1:=floorunscaled(y);
if x-65536*m1>=y-65536*n1+zcorr[octant]then d1:=1 else d1:=0;end;{:463}
{465:}procedure fillspec(h:halfword);var p,q,r,s:halfword;
begin if internal[10]>0 then beginedgetracing;p:=h;
repeat octant:=mem[p+3].int;{466:}q:=p;
while mem[q].hh.b1<>0 do q:=mem[q].hh.rh{:466};if q<>p then begin{467:}
endround(mem[p+1].int,mem[p+2].int);m0:=m1;n0:=n1;d0:=d1;
endround(mem[q+1].int,mem[q+2].int){:467};{468:}
if n1-n0>=movesize then overflow(539,movesize);move[0]:=d0;moveptr:=0;
r:=p;repeat s:=mem[r].hh.rh;
makemoves(mem[r+1].int,mem[r+5].int,mem[s+3].int,mem[s+1].int,mem[r+2].
int+32768,mem[r+6].int+32768,mem[s+4].int+32768,mem[s+2].int+32768,
xycorr[octant],ycorr[octant]);r:=s;until r=q;
move[moveptr]:=move[moveptr]-d1;
if internal[35]>0 then smoothmoves(0,moveptr){:468};
movetoedges(m0,n0,m1,n1);end;p:=mem[q].hh.rh;until p=h;tossknotlist(h);
if internal[10]>0 then endedgetracing;end;{:465}{476:}
procedure dupoffset(w:halfword);var r:halfword;begin r:=getnode(3);
mem[r+1].int:=mem[w+1].int;mem[r+2].int:=mem[w+2].int;
mem[r].hh.rh:=mem[w].hh.rh;mem[mem[w].hh.rh].hh.lh:=r;mem[r].hh.lh:=w;
mem[w].hh.rh:=r;end;{:476}{477:}function makepen(h:halfword):halfword;
label 30,31,45,40;var o,oo,k:smallnumber;p:halfword;q,r,s,w,hh:halfword;
n:integer;dx,dy:scaled;mc:scaled;begin{479:}q:=h;r:=mem[q].hh.rh;
mc:=abs(mem[h+1].int);if q=r then begin hh:=h;mem[h].hh.b1:=0;
if mc<abs(mem[h+2].int)then mc:=abs(mem[h+2].int);end else begin o:=0;
hh:=0;while true do begin s:=mem[r].hh.rh;
if mc<abs(mem[r+1].int)then mc:=abs(mem[r+1].int);
if mc<abs(mem[r+2].int)then mc:=abs(mem[r+2].int);
dx:=mem[r+1].int-mem[q+1].int;dy:=mem[r+2].int-mem[q+2].int;
if dx=0 then if dy=0 then goto 45;
if abvscd(dx,mem[s+2].int-mem[r+2].int,dy,mem[s+1].int-mem[r+1].int)<0
then goto 45;{480:}
if dx>0 then octant:=1 else if dx=0 then if dy>0 then octant:=1 else
octant:=2 else begin dx:=-dx;octant:=2;end;if dy<0 then begin dy:=-dy;
octant:=octant+2;end else if dy=0 then if octant>1 then octant:=4;
if dx<dy then octant:=octant+4{:480};mem[q].hh.b1:=octant;
oo:=octantnumber[octant];if o>oo then begin if hh<>0 then goto 45;hh:=q;
end;o:=oo;if(q=h)and(hh<>0)then goto 30;q:=r;r:=s;end;30:end{:479};
if mc>=268402688 then goto 45;p:=getnode(10);q:=hh;mem[p+9].int:=mc;
mem[p].hh.lh:=0;if mem[q].hh.rh<>q then mem[p].hh.rh:=1;
for k:=1 to 8 do{481:}begin octant:=octantcode[k];n:=0;h:=p+octant;
while true do begin r:=getnode(3);
skew(mem[q+1].int,mem[q+2].int,octant);mem[r+1].int:=curx;
mem[r+2].int:=cury;if n=0 then mem[h].hh.rh:=r else{482:}
if odd(k)then begin mem[w].hh.rh:=r;mem[r].hh.lh:=w;
end else begin mem[w].hh.lh:=r;mem[r].hh.rh:=w;end{:482};w:=r;
if mem[q].hh.b1<>octant then goto 31;q:=mem[q].hh.rh;incr(n);end;
31:{483:}r:=mem[h].hh.rh;if odd(k)then begin mem[w].hh.rh:=r;
mem[r].hh.lh:=w;end else begin mem[w].hh.lh:=r;mem[r].hh.rh:=w;
mem[h].hh.rh:=w;r:=w;end;
if(mem[r+2].int<>mem[mem[r].hh.rh+2].int)or(n=0)then begin dupoffset(r);
incr(n);end;r:=mem[r].hh.lh;
if mem[r+1].int<>mem[mem[r].hh.lh+1].int then dupoffset(r)else decr(n){:
483};if n>=255 then overflow(578,255);mem[h].hh.lh:=n;end{:481};goto 40;
45:p:=3;{478:}if mc>=268402688 then begin begin if interaction=3 then;
printnl(261);print(572);end;begin helpptr:=2;helpline[1]:=573;
helpline[0]:=574;end;end else begin begin if interaction=3 then;
printnl(261);print(575);end;begin helpptr:=3;helpline[2]:=576;
helpline[1]:=577;helpline[0]:=574;end;end;putgeterror{:478};
40:if internal[6]>0 then printpen(p,571,true);makepen:=p;end;{:477}
{484:}{486:}function trivialknot(x,y:scaled):halfword;var p:halfword;
begin p:=getnode(7);mem[p].hh.b0:=1;mem[p].hh.b1:=1;mem[p+1].int:=x;
mem[p+3].int:=x;mem[p+5].int:=x;mem[p+2].int:=y;mem[p+4].int:=y;
mem[p+6].int:=y;trivialknot:=p;end;{:486}
function makepath(penhead:halfword):halfword;var p:halfword;k:1..8;
h:halfword;m,n:integer;w,ww:halfword;begin p:=memtop-1;
for k:=1 to 8 do begin octant:=octantcode[k];h:=penhead+octant;
n:=mem[h].hh.lh;w:=mem[h].hh.rh;if not odd(k)then w:=mem[w].hh.lh;
for m:=1 to n+1 do begin if odd(k)then ww:=mem[w].hh.rh else ww:=mem[w].
hh.lh;
if(mem[ww+1].int<>mem[w+1].int)or(mem[ww+2].int<>mem[w+2].int)then{485:}
begin unskew(mem[ww+1].int,mem[ww+2].int,octant);
mem[p].hh.rh:=trivialknot(curx,cury);p:=mem[p].hh.rh;end{:485};w:=ww;
end;end;if p=memtop-1 then begin w:=mem[penhead+1].hh.rh;
p:=trivialknot(mem[w+1].int+mem[w+2].int,mem[w+2].int);
mem[memtop-1].hh.rh:=p;end;mem[p].hh.rh:=mem[memtop-1].hh.rh;
makepath:=mem[memtop-1].hh.rh;end;{:484}{488:}
procedure findoffset(x,y:scaled;p:halfword);label 30,10;var octant:1..8;
s:-1..+1;n:integer;h,w,ww:halfword;begin{489:}
if x>0 then octant:=1 else if x=0 then if y<=0 then if y=0 then begin
curx:=0;cury:=0;goto 10;
end else octant:=2 else octant:=1 else begin x:=-x;
if y=0 then octant:=4 else octant:=2;end;
if y<0 then begin octant:=octant+2;y:=-y;end;
if x>=y then x:=x-y else begin octant:=octant+4;x:=y-x;y:=y-x;end{:489};
if odd(octantnumber[octant])then s:=-1 else s:=+1;h:=p+octant;
w:=mem[mem[h].hh.rh].hh.rh;ww:=mem[w].hh.rh;n:=mem[h].hh.lh;
while n>1 do begin if abvscd(x,mem[ww+2].int-mem[w+2].int,y,mem[ww+1].
int-mem[w+1].int)<>s then goto 30;w:=ww;ww:=mem[w].hh.rh;decr(n);end;
30:unskew(mem[w+1].int,mem[w+2].int,octant);10:end;{:488}{491:}{493:}
procedure splitforoffset(p:halfword;t:fraction);var q:halfword;
r:halfword;begin q:=mem[p].hh.rh;
splitcubic(p,t,mem[q+1].int,mem[q+2].int);r:=mem[p].hh.rh;
if mem[r+2].int<mem[p+2].int then mem[r+2].int:=mem[p+2].int else if mem
[r+2].int>mem[q+2].int then mem[r+2].int:=mem[q+2].int;
if mem[r+1].int<mem[p+1].int then mem[r+1].int:=mem[p+1].int else if mem
[r+1].int>mem[q+1].int then mem[r+1].int:=mem[q+1].int;end;{:493}{497:}
procedure finoffsetprep(p:halfword;k:halfword;w:halfword;
x0,x1,x2,y0,y1,y2:integer;rising:boolean;n:integer);label 10;
var ww:halfword;du,dv:scaled;t0,t1,t2:integer;t:fraction;s:fraction;
v:integer;begin while true do begin mem[p].hh.b1:=k;
if rising then if k=n then goto 10 else ww:=mem[w].hh.rh else if k=1
then goto 10 else ww:=mem[w].hh.lh;{498:}du:=mem[ww+1].int-mem[w+1].int;
dv:=mem[ww+2].int-mem[w+2].int;
if abs(du)>=abs(dv)then begin s:=makefraction(dv,du);
t0:=takefraction(x0,s)-y0;t1:=takefraction(x1,s)-y1;
t2:=takefraction(x2,s)-y2;end else begin s:=makefraction(du,dv);
t0:=x0-takefraction(y0,s);t1:=x1-takefraction(y1,s);
t2:=x2-takefraction(y2,s);end{:498};t:=crossingpoint(t0,t1,t2);
if t>=268435456 then goto 10;{499:}begin splitforoffset(p,t);
mem[p].hh.b1:=k;p:=mem[p].hh.rh;v:=x0-takefraction(x0-x1,t);
x1:=x1-takefraction(x1-x2,t);x0:=v-takefraction(v-x1,t);
v:=y0-takefraction(y0-y1,t);y1:=y1-takefraction(y1-y2,t);
y0:=v-takefraction(v-y1,t);t1:=t1-takefraction(t1-t2,t);
if t1>0 then t1:=0;t:=crossingpoint(0,-t1,-t2);
if t<268435456 then begin splitforoffset(p,t);
mem[mem[p].hh.rh].hh.b1:=k;v:=x1-takefraction(x1-x2,t);
x1:=x0-takefraction(x0-x1,t);x2:=x1-takefraction(x1-v,t);
v:=y1-takefraction(y1-y2,t);y1:=y0-takefraction(y0-y1,t);
y2:=y1-takefraction(y1-v,t);end;end{:499};
if rising then incr(k)else decr(k);w:=ww;end;10:end;{:497}
procedure offsetprep(c,h:halfword);label 30,45;var n:halfword;
p,q,r,lh,ww:halfword;k:halfword;w:halfword;{495:}
x0,x1,x2,y0,y1,y2:integer;t0,t1,t2:integer;du,dv,dx,dy:integer;
maxcoef:integer;x0a,x1a,x2a,y0a,y1a,y2a:integer;t:fraction;s:fraction;
{:495}begin p:=c;n:=mem[h].hh.lh;lh:=mem[h].hh.rh;
while mem[p].hh.b1<>0 do begin q:=mem[p].hh.rh;{494:}
if n<=1 then mem[p].hh.b1:=1 else begin{496:}
x0:=mem[p+5].int-mem[p+1].int;x2:=mem[q+1].int-mem[q+3].int;
x1:=mem[q+3].int-mem[p+5].int;y0:=mem[p+6].int-mem[p+2].int;
y2:=mem[q+2].int-mem[q+4].int;y1:=mem[q+4].int-mem[p+6].int;
maxcoef:=abs(x0);if abs(x1)>maxcoef then maxcoef:=abs(x1);
if abs(x2)>maxcoef then maxcoef:=abs(x2);
if abs(y0)>maxcoef then maxcoef:=abs(y0);
if abs(y1)>maxcoef then maxcoef:=abs(y1);
if abs(y2)>maxcoef then maxcoef:=abs(y2);if maxcoef=0 then goto 45;
while maxcoef<134217728 do begin maxcoef:=maxcoef+maxcoef;x0:=x0+x0;
x1:=x1+x1;x2:=x2+x2;y0:=y0+y0;y1:=y1+y1;y2:=y2+y2;end{:496};{501:}
dx:=x0;dy:=y0;if dx=0 then if dy=0 then begin dx:=x1;dy:=y1;
if dx=0 then if dy=0 then begin dx:=x2;dy:=y2;end;end{:501};
if dx=0 then{505:}
finoffsetprep(p,n,mem[mem[lh].hh.lh].hh.lh,-x0,-x1,-x2,-y0,-y1,-y2,false
,n){:505}else begin{502:}k:=1;w:=mem[lh].hh.rh;
while true do begin if k=n then goto 30;ww:=mem[w].hh.rh;
if abvscd(dy,abs(mem[ww+1].int-mem[w+1].int),dx,abs(mem[ww+2].int-mem[w
+2].int))>=0 then begin incr(k);w:=ww;end else goto 30;end;30:{:502};
{503:}if k=1 then t:=268435457 else begin ww:=mem[w].hh.lh;{498:}
du:=mem[ww+1].int-mem[w+1].int;dv:=mem[ww+2].int-mem[w+2].int;
if abs(du)>=abs(dv)then begin s:=makefraction(dv,du);
t0:=takefraction(x0,s)-y0;t1:=takefraction(x1,s)-y1;
t2:=takefraction(x2,s)-y2;end else begin s:=makefraction(du,dv);
t0:=x0-takefraction(y0,s);t1:=x1-takefraction(y1,s);
t2:=x2-takefraction(y2,s);end{:498};t:=crossingpoint(-t0,-t1,-t2);end;
if t>=268435456 then finoffsetprep(p,k,w,x0,x1,x2,y0,y1,y2,true,n)else
begin splitforoffset(p,t);r:=mem[p].hh.rh;x1a:=x0-takefraction(x0-x1,t);
x1:=x1-takefraction(x1-x2,t);x2a:=x1a-takefraction(x1a-x1,t);
y1a:=y0-takefraction(y0-y1,t);y1:=y1-takefraction(y1-y2,t);
y2a:=y1a-takefraction(y1a-y1,t);
finoffsetprep(p,k,w,x0,x1a,x2a,y0,y1a,y2a,true,n);x0:=x2a;y0:=y2a;
t1:=t1-takefraction(t1-t2,t);if t1<0 then t1:=0;
t:=crossingpoint(0,t1,t2);if t<268435456 then{504:}
begin splitforoffset(r,t);x1a:=x1-takefraction(x1-x2,t);
x1:=x0-takefraction(x0-x1,t);x0a:=x1-takefraction(x1-x1a,t);
y1a:=y1-takefraction(y1-y2,t);y1:=y0-takefraction(y0-y1,t);
y0a:=y1-takefraction(y1-y1a,t);
finoffsetprep(mem[r].hh.rh,k,w,x0a,x1a,x2,y0a,y1a,y2,true,n);x2:=x0a;
y2:=y0a;end{:504};
finoffsetprep(r,k-1,ww,-x0,-x1,-x2,-y0,-y1,-y2,false,n);end{:503};end;
45:end{:494};{492:}repeat r:=mem[p].hh.rh;
if mem[p+1].int=mem[p+5].int then if mem[p+2].int=mem[p+6].int then if
mem[p+1].int=mem[r+3].int then if mem[p+2].int=mem[r+4].int then if mem[
p+1].int=mem[r+1].int then if mem[p+2].int=mem[r+2].int then begin
removecubic(p);if r=q then q:=p;r:=p;end;p:=r;until p=q{:492};end;end;
{:491}{506:}{510:}procedure skewlineedges(p,w,ww:halfword);
var x0,y0,x1,y1:scaled;
begin if(mem[w+1].int<>mem[ww+1].int)or(mem[w+2].int<>mem[ww+2].int)then
begin x0:=mem[p+1].int+mem[w+1].int;y0:=mem[p+2].int+mem[w+2].int;
x1:=mem[p+1].int+mem[ww+1].int;y1:=mem[p+2].int+mem[ww+2].int;
unskew(x0,y0,octant);x0:=curx;y0:=cury;unskew(x1,y1,octant);
ifdef('STAT')if internal[10]>65536 then begin printnl(583);
printtwo(x0,y0);print(582);printtwo(curx,cury);printnl(283);end;
endif('STAT')lineedges(x0,y0,curx,cury);end;end;{:510}{518:}
procedure dualmoves(h,p,q:halfword);label 30,31;var r,s:halfword;{511:}
m,n:integer;mm0,mm1:integer;k:integer;w,ww:halfword;
smoothbot,smoothtop:0..movesize;xx,yy,xp,yp,delx,dely,tx,ty:scaled;
{:511}begin{519:}k:=mem[h].hh.lh+1;ww:=mem[h].hh.rh;w:=mem[ww].hh.lh;
mm0:=floorunscaled(mem[p+1].int+mem[w+1].int-xycorr[octant]);
mm1:=floorunscaled(mem[q+1].int+mem[ww+1].int-xycorr[octant]);
for n:=1 to n1-n0+1 do envmove[n]:=mm1;envmove[0]:=mm0;moveptr:=0;
m:=mm0{:519};r:=p;while true do begin if r=q then smoothtop:=moveptr;
while mem[r].hh.b1<>k do{521:}begin xx:=mem[r+1].int+mem[w+1].int;
yy:=mem[r+2].int+mem[w+2].int+32768;
ifdef('STAT')if internal[10]>65536 then begin printnl(584);printint(k);
print(585);unskew(xx,yy-32768,octant);printtwo(curx,cury);end;
endif('STAT')if mem[r].hh.b1<k then begin decr(k);w:=mem[w].hh.lh;
xp:=mem[r+1].int+mem[w+1].int;yp:=mem[r+2].int+mem[w+2].int+32768;
if yp<>yy then{522:}begin ty:=floorscaled(yy-ycorr[octant]);dely:=yp-yy;
yy:=yy-ty;ty:=yp-ycorr[octant]-ty;if ty>=65536 then begin delx:=xp-xx;
yy:=65536-yy;
while true do begin if m<envmove[moveptr]then envmove[moveptr]:=m;
tx:=takefraction(delx,makefraction(yy,dely));
if abvscd(tx,dely,delx,yy)+xycorr[octant]>0 then decr(tx);
m:=floorunscaled(xx+tx);ty:=ty-65536;incr(moveptr);
if ty<65536 then goto 31;yy:=yy+65536;end;
31:if m<envmove[moveptr]then envmove[moveptr]:=m;end;end{:522};
end else begin incr(k);w:=mem[w].hh.rh;xp:=mem[r+1].int+mem[w+1].int;
yp:=mem[r+2].int+mem[w+2].int+32768;end;
ifdef('STAT')if internal[10]>65536 then begin print(582);
unskew(xp,yp-32768,octant);printtwo(curx,cury);printnl(283);end;
endif('STAT')m:=floorunscaled(xp-xycorr[octant]);
moveptr:=floorunscaled(yp-ycorr[octant])-n0;
if m<envmove[moveptr]then envmove[moveptr]:=m;end{:521};
if r=p then smoothbot:=moveptr;if r=q then goto 30;move[moveptr]:=1;
n:=moveptr;s:=mem[r].hh.rh;
makemoves(mem[r+1].int+mem[w+1].int,mem[r+5].int+mem[w+1].int,mem[s+3].
int+mem[w+1].int,mem[s+1].int+mem[w+1].int,mem[r+2].int+mem[w+2].int
+32768,mem[r+6].int+mem[w+2].int+32768,mem[s+4].int+mem[w+2].int+32768,
mem[s+2].int+mem[w+2].int+32768,xycorr[octant],ycorr[octant]);{520:}
repeat if m<envmove[n]then envmove[n]:=m;m:=m+move[n]-1;incr(n);
until n>moveptr{:520};r:=s;end;30:{523:}
ifdef('DEBUG')if(m<>mm1)or(moveptr<>n1-n0)then confusion(50);
endif('DEBUG')move[0]:=d0+envmove[1]-mm0;
for n:=1 to moveptr do move[n]:=envmove[n+1]-envmove[n]+1;
move[moveptr]:=move[moveptr]-d1;
if internal[35]>0 then smoothmoves(smoothbot,smoothtop);
movetoedges(m0,n0,m1,n1);if mem[q+6].int=1 then begin w:=mem[h].hh.rh;
skewlineedges(q,w,mem[w].hh.lh);end{:523};end;{:518}
procedure fillenvelope(spechead:halfword);label 30,31;
var p,q,r,s:halfword;h:halfword;www:halfword;{511:}m,n:integer;
mm0,mm1:integer;k:integer;w,ww:halfword;smoothbot,smoothtop:0..movesize;
xx,yy,xp,yp,delx,dely,tx,ty:scaled;{:511}
begin if internal[10]>0 then beginedgetracing;p:=spechead;
repeat octant:=mem[p+3].int;h:=curpen+octant;{466:}q:=p;
while mem[q].hh.b1<>0 do q:=mem[q].hh.rh{:466};{508:}w:=mem[h].hh.rh;
if mem[p+4].int=1 then w:=mem[w].hh.lh;
ifdef('STAT')if internal[10]>65536 then{509:}begin printnl(579);
print(octantdir[octant]);print(557);printint(mem[h].hh.lh);print(580);
if mem[h].hh.lh<>1 then printchar(115);print(581);
unskew(mem[p+1].int+mem[w+1].int,mem[p+2].int+mem[w+2].int,octant);
printtwo(curx,cury);ww:=mem[h].hh.rh;
if mem[q+6].int=1 then ww:=mem[ww].hh.lh;print(582);
unskew(mem[q+1].int+mem[ww+1].int,mem[q+2].int+mem[ww+2].int,octant);
printtwo(curx,cury);end{:509};endif('STAT')ww:=mem[h].hh.rh;www:=ww;
if odd(octantnumber[octant])then www:=mem[www].hh.lh else ww:=mem[ww].hh
.lh;if w<>ww then skewlineedges(p,w,ww);
endround(mem[p+1].int+mem[ww+1].int,mem[p+2].int+mem[ww+2].int);m0:=m1;
n0:=n1;d0:=d1;
endround(mem[q+1].int+mem[www+1].int,mem[q+2].int+mem[www+2].int);
if n1-n0>=movesize then overflow(539,movesize){:508};offsetprep(p,h);
{466:}q:=p;while mem[q].hh.b1<>0 do q:=mem[q].hh.rh{:466};{512:}
if odd(octantnumber[octant])then begin{513:}k:=0;w:=mem[h].hh.rh;
ww:=mem[w].hh.lh;
mm0:=floorunscaled(mem[p+1].int+mem[w+1].int-xycorr[octant]);
mm1:=floorunscaled(mem[q+1].int+mem[ww+1].int-xycorr[octant]);
for n:=0 to n1-n0 do envmove[n]:=mm0;envmove[n1-n0]:=mm1;moveptr:=0;
m:=mm0{:513};r:=p;mem[q].hh.b1:=mem[h].hh.lh+1;
while true do begin if r=q then smoothtop:=moveptr;
while mem[r].hh.b1<>k do{515:}begin xx:=mem[r+1].int+mem[w+1].int;
yy:=mem[r+2].int+mem[w+2].int+32768;
ifdef('STAT')if internal[10]>65536 then begin printnl(584);printint(k);
print(585);unskew(xx,yy-32768,octant);printtwo(curx,cury);end;
endif('STAT')if mem[r].hh.b1>k then begin incr(k);w:=mem[w].hh.rh;
xp:=mem[r+1].int+mem[w+1].int;yp:=mem[r+2].int+mem[w+2].int+32768;
if yp<>yy then{516:}begin ty:=floorscaled(yy-ycorr[octant]);dely:=yp-yy;
yy:=yy-ty;ty:=yp-ycorr[octant]-ty;if ty>=65536 then begin delx:=xp-xx;
yy:=65536-yy;
while true do begin tx:=takefraction(delx,makefraction(yy,dely));
if abvscd(tx,dely,delx,yy)+xycorr[octant]>0 then decr(tx);
m:=floorunscaled(xx+tx);if m>envmove[moveptr]then envmove[moveptr]:=m;
ty:=ty-65536;if ty<65536 then goto 31;yy:=yy+65536;incr(moveptr);end;
31:end;end{:516};end else begin decr(k);w:=mem[w].hh.lh;
xp:=mem[r+1].int+mem[w+1].int;yp:=mem[r+2].int+mem[w+2].int+32768;end;
ifdef('STAT')if internal[10]>65536 then begin print(582);
unskew(xp,yp-32768,octant);printtwo(curx,cury);printnl(283);end;
endif('STAT')m:=floorunscaled(xp-xycorr[octant]);
moveptr:=floorunscaled(yp-ycorr[octant])-n0;
if m>envmove[moveptr]then envmove[moveptr]:=m;end{:515};
if r=p then smoothbot:=moveptr;if r=q then goto 30;move[moveptr]:=1;
n:=moveptr;s:=mem[r].hh.rh;
makemoves(mem[r+1].int+mem[w+1].int,mem[r+5].int+mem[w+1].int,mem[s+3].
int+mem[w+1].int,mem[s+1].int+mem[w+1].int,mem[r+2].int+mem[w+2].int
+32768,mem[r+6].int+mem[w+2].int+32768,mem[s+4].int+mem[w+2].int+32768,
mem[s+2].int+mem[w+2].int+32768,xycorr[octant],ycorr[octant]);{514:}
repeat m:=m+move[n]-1;if m>envmove[n]then envmove[n]:=m;incr(n);
until n>moveptr{:514};r:=s;end;30:{517:}
ifdef('DEBUG')if(m<>mm1)or(moveptr<>n1-n0)then confusion(49);
endif('DEBUG')move[0]:=d0+envmove[0]-mm0;
for n:=1 to moveptr do move[n]:=envmove[n]-envmove[n-1]+1;
move[moveptr]:=move[moveptr]-d1;
if internal[35]>0 then smoothmoves(smoothbot,smoothtop);
movetoedges(m0,n0,m1,n1);if mem[q+6].int=0 then begin w:=mem[h].hh.rh;
skewlineedges(q,mem[w].hh.lh,w);end{:517};end else dualmoves(h,p,q);
mem[q].hh.b1:=0{:512};p:=mem[q].hh.rh;until p=spechead;
if internal[10]>0 then endedgetracing;tossknotlist(spechead);end;{:506}
{527:}function makeellipse(majoraxis,minoraxis:scaled;
theta:angle):halfword;label 30,31,40;var p,q,r,s:halfword;h:halfword;
alpha,beta,gamma,delta:integer;c,d:integer;u,v:integer;
symmetric:boolean;begin{528:}{530:}
if(majoraxis=minoraxis)or(theta mod 94371840=0)then begin symmetric:=
true;alpha:=0;if odd(theta div 94371840)then begin beta:=majoraxis;
gamma:=minoraxis;nsin:=268435456;ncos:=0;end else begin beta:=minoraxis;
gamma:=majoraxis;end;end else begin symmetric:=false;nsincos(theta);
gamma:=takefraction(majoraxis,nsin);delta:=takefraction(minoraxis,ncos);
beta:=pythadd(gamma,delta);alpha:=makefraction(gamma,beta);
alpha:=takefraction(majoraxis,alpha);alpha:=takefraction(alpha,ncos);
alpha:=(alpha+32768)div 65536;gamma:=takefraction(minoraxis,nsin);
gamma:=pythadd(takefraction(majoraxis,ncos),gamma);end;
beta:=(beta+32768)div 65536;gamma:=(gamma+32768)div 65536{:530};
p:=getnode(7);q:=getnode(7);r:=getnode(7);
if symmetric then s:=0 else s:=getnode(7);h:=p;mem[p].hh.rh:=q;
mem[q].hh.rh:=r;mem[r].hh.rh:=s;{529:}if beta=0 then beta:=1;
if gamma=0 then gamma:=1;
if gamma<=abs(alpha)then if alpha>0 then alpha:=gamma-1 else alpha:=1-
gamma{:529};mem[p+1].int:=-alpha*32768;mem[p+2].int:=-beta*32768;
mem[q+1].int:=gamma*32768;mem[q+2].int:=mem[p+2].int;
mem[r+1].int:=mem[q+1].int;mem[p+5].int:=0;mem[q+3].int:=-32768;
mem[q+5].int:=32768;mem[r+3].int:=0;mem[r+5].int:=0;mem[p+6].int:=beta;
mem[q+6].int:=gamma;mem[r+6].int:=beta;mem[q+4].int:=gamma+alpha;
if symmetric then begin mem[r+2].int:=0;mem[r+4].int:=beta;
end else begin mem[r+2].int:=-mem[p+2].int;mem[r+4].int:=beta+beta;
mem[s+1].int:=-mem[p+1].int;mem[s+2].int:=mem[r+2].int;
mem[s+3].int:=32768;mem[s+4].int:=gamma-alpha;end{:528};{531:}
while true do begin u:=mem[p+5].int+mem[q+5].int;
v:=mem[q+3].int+mem[r+3].int;c:=mem[p+6].int+mem[q+6].int;{533:}
delta:=pythadd(u,v);
if majoraxis=minoraxis then d:=majoraxis else begin if theta=0 then
begin alpha:=u;beta:=v;
end else begin alpha:=takefraction(u,ncos)+takefraction(v,nsin);
beta:=takefraction(v,ncos)-takefraction(u,nsin);end;
alpha:=makefraction(alpha,delta);beta:=makefraction(beta,delta);
d:=pythadd(takefraction(majoraxis,alpha),takefraction(minoraxis,beta));
end;alpha:=abs(u);beta:=abs(v);if alpha<beta then begin alpha:=abs(v);
beta:=abs(u);end;
if internal[38]<>0 then d:=d-takefraction(internal[38],makefraction(beta
+beta,delta));d:=takefraction((d+4)div 8,delta);alpha:=alpha div 32768;
if d<alpha then d:=alpha{:533};delta:=c-d;
if delta>0 then begin if delta>mem[r+4].int then delta:=mem[r+4].int;
if delta>=mem[q+4].int then{534:}begin delta:=mem[q+4].int;
mem[p+6].int:=c-delta;mem[p+5].int:=u;mem[q+3].int:=v;
mem[q+1].int:=mem[q+1].int-delta*mem[r+3].int;
mem[q+2].int:=mem[q+2].int+delta*mem[q+5].int;
mem[r+4].int:=mem[r+4].int-delta;end{:534}else{535:}begin s:=getnode(7);
mem[p].hh.rh:=s;mem[s].hh.rh:=q;
mem[s+1].int:=mem[q+1].int+delta*mem[q+3].int;
mem[s+2].int:=mem[q+2].int-delta*mem[p+5].int;
mem[q+1].int:=mem[q+1].int-delta*mem[r+3].int;
mem[q+2].int:=mem[q+2].int+delta*mem[q+5].int;
mem[s+3].int:=mem[q+3].int;mem[s+5].int:=u;mem[q+3].int:=v;
mem[s+6].int:=c-delta;mem[s+4].int:=mem[q+4].int-delta;
mem[q+4].int:=delta;mem[r+4].int:=mem[r+4].int-delta;end{:535};
end else p:=q;{532:}while true do begin q:=mem[p].hh.rh;
if q=0 then goto 30;
if mem[q+4].int=0 then begin mem[p].hh.rh:=mem[q].hh.rh;
mem[p+6].int:=mem[q+6].int;mem[p+5].int:=mem[q+5].int;freenode(q,7);
end else begin r:=mem[q].hh.rh;if r=0 then goto 30;
if mem[r+4].int=0 then begin mem[p].hh.rh:=r;freenode(q,7);p:=r;
end else goto 40;end;end;40:{:532};end;30:{:531};if symmetric then{536:}
begin s:=0;q:=h;while true do begin r:=getnode(7);mem[r].hh.rh:=s;s:=r;
mem[s+1].int:=mem[q+1].int;mem[s+2].int:=-mem[q+2].int;
if q=p then goto 31;q:=mem[q].hh.rh;if mem[q+2].int=0 then goto 31;end;
31:mem[p].hh.rh:=s;beta:=-mem[h+2].int;
while mem[p+2].int<>beta do p:=mem[p].hh.rh;q:=mem[p].hh.rh;end{:536};
{537:}if q<>0 then begin if mem[h+5].int=0 then begin p:=h;
h:=mem[h].hh.rh;freenode(p,7);mem[q+1].int:=-mem[h+1].int;end;p:=q;
end else q:=p;r:=mem[h].hh.rh;repeat s:=getnode(7);mem[p].hh.rh:=s;p:=s;
mem[p+1].int:=-mem[r+1].int;mem[p+2].int:=-mem[r+2].int;r:=mem[r].hh.rh;
until r=q;mem[p].hh.rh:=h{:537};makeellipse:=h;end;{:527}{539:}
function finddirectiontime(x,y:scaled;h:halfword):scaled;
label 10,40,45,30;var max:scaled;p,q:halfword;n:scaled;tt:scaled;{542:}
x1,x2,x3,y1,y2,y3:scaled;theta,phi:angle;t:fraction;{:542}begin{540:}
if abs(x)<abs(y)then begin x:=makefraction(x,abs(y));
if y>0 then y:=268435456 else y:=-268435456;
end else if x=0 then begin finddirectiontime:=0;goto 10;
end else begin y:=makefraction(y,abs(x));
if x>0 then x:=268435456 else x:=-268435456;end{:540};n:=0;p:=h;
while true do begin if mem[p].hh.b1=0 then goto 45;q:=mem[p].hh.rh;
{541:}tt:=0;{543:}x1:=mem[p+5].int-mem[p+1].int;
x2:=mem[q+3].int-mem[p+5].int;x3:=mem[q+1].int-mem[q+3].int;
y1:=mem[p+6].int-mem[p+2].int;y2:=mem[q+4].int-mem[p+6].int;
y3:=mem[q+2].int-mem[q+4].int;max:=abs(x1);
if abs(x2)>max then max:=abs(x2);if abs(x3)>max then max:=abs(x3);
if abs(y1)>max then max:=abs(y1);if abs(y2)>max then max:=abs(y2);
if abs(y3)>max then max:=abs(y3);if max=0 then goto 40;
while max<134217728 do begin max:=max+max;x1:=x1+x1;x2:=x2+x2;x3:=x3+x3;
y1:=y1+y1;y2:=y2+y2;y3:=y3+y3;end;t:=x1;
x1:=takefraction(x1,x)+takefraction(y1,y);
y1:=takefraction(y1,x)-takefraction(t,y);t:=x2;
x2:=takefraction(x2,x)+takefraction(y2,y);
y2:=takefraction(y2,x)-takefraction(t,y);t:=x3;
x3:=takefraction(x3,x)+takefraction(y3,y);
y3:=takefraction(y3,x)-takefraction(t,y){:543};
if y1=0 then if x1>=0 then goto 40;if n>0 then begin{544:}
theta:=narg(x1,y1);
if theta>=0 then if phi<=0 then if phi>=theta-188743680 then goto 40;
if theta<=0 then if phi>=0 then if phi<=theta+188743680 then goto 40{:
544};if p=h then goto 45;end;if(x3<>0)or(y3<>0)then phi:=narg(x3,y3);
{546:}if x1<0 then if x2<0 then if x3<0 then goto 30;
if abvscd(y1,y3,y2,y2)=0 then{548:}
begin if abvscd(y1,y2,0,0)<0 then begin t:=makefraction(y1,y1-y2);
x1:=x1-takefraction(x1-x2,t);x2:=x2-takefraction(x2-x3,t);
if x1-takefraction(x1-x2,t)>=0 then begin tt:=(t+2048)div 4096;goto 40;
end;end else if y3=0 then if y1=0 then{549:}
begin t:=crossingpoint(-x1,-x2,-x3);
if t<=268435456 then begin tt:=(t+2048)div 4096;goto 40;end;
if abvscd(x1,x3,x2,x2)<=0 then begin t:=makefraction(x1,x1-x2);
begin tt:=(t+2048)div 4096;goto 40;end;end;end{:549}
else if x3>=0 then begin tt:=65536;goto 40;end;goto 30;end{:548};
if y1<=0 then if y1<0 then begin y1:=-y1;y2:=-y2;y3:=-y3;
end else if y2>0 then begin y2:=-y2;y3:=-y3;end;{547:}
t:=crossingpoint(y1,y2,y3);if t>268435456 then goto 30;
y2:=y2-takefraction(y2-y3,t);x1:=x1-takefraction(x1-x2,t);
x2:=x2-takefraction(x2-x3,t);x1:=x1-takefraction(x1-x2,t);
if x1>=0 then begin tt:=(t+2048)div 4096;goto 40;end;if y2>0 then y2:=0;
tt:=t;t:=crossingpoint(0,-y2,-y3);if t>268435456 then goto 30;
x1:=x1-takefraction(x1-x2,t);x2:=x2-takefraction(x2-x3,t);
if x1-takefraction(x1-x2,t)>=0 then begin t:=tt-takefraction(tt
-268435456,t);begin tt:=(t+2048)div 4096;goto 40;end;end{:547};30:{:546}
{:541};p:=q;n:=n+65536;end;45:finddirectiontime:=-65536;goto 10;
40:finddirectiontime:=n+tt;10:end;{:539}{556:}
procedure cubicintersection(p,pp:halfword);label 22,45,10;
var q,qq:halfword;begin timetogo:=5000;maxt:=2;{558:}q:=mem[p].hh.rh;
qq:=mem[pp].hh.rh;bisectptr:=20;
bisectstack[bisectptr-5]:=mem[p+5].int-mem[p+1].int;
bisectstack[bisectptr-4]:=mem[q+3].int-mem[p+5].int;
bisectstack[bisectptr-3]:=mem[q+1].int-mem[q+3].int;
if bisectstack[bisectptr-5]<0 then if bisectstack[bisectptr-3]>=0 then
begin if bisectstack[bisectptr-4]<0 then bisectstack[bisectptr-2]:=
bisectstack[bisectptr-5]+bisectstack[bisectptr-4]else bisectstack[
bisectptr-2]:=bisectstack[bisectptr-5];
bisectstack[bisectptr-1]:=bisectstack[bisectptr-5]+bisectstack[bisectptr
-4]+bisectstack[bisectptr-3];
if bisectstack[bisectptr-1]<0 then bisectstack[bisectptr-1]:=0;
end else begin bisectstack[bisectptr-2]:=bisectstack[bisectptr-5]+
bisectstack[bisectptr-4]+bisectstack[bisectptr-3];
if bisectstack[bisectptr-2]>bisectstack[bisectptr-5]then bisectstack[
bisectptr-2]:=bisectstack[bisectptr-5];
bisectstack[bisectptr-1]:=bisectstack[bisectptr-5]+bisectstack[bisectptr
-4];if bisectstack[bisectptr-1]<0 then bisectstack[bisectptr-1]:=0;
end else if bisectstack[bisectptr-3]<=0 then begin if bisectstack[
bisectptr-4]>0 then bisectstack[bisectptr-1]:=bisectstack[bisectptr-5]+
bisectstack[bisectptr-4]else bisectstack[bisectptr-1]:=bisectstack[
bisectptr-5];
bisectstack[bisectptr-2]:=bisectstack[bisectptr-5]+bisectstack[bisectptr
-4]+bisectstack[bisectptr-3];
if bisectstack[bisectptr-2]>0 then bisectstack[bisectptr-2]:=0;
end else begin bisectstack[bisectptr-1]:=bisectstack[bisectptr-5]+
bisectstack[bisectptr-4]+bisectstack[bisectptr-3];
if bisectstack[bisectptr-1]<bisectstack[bisectptr-5]then bisectstack[
bisectptr-1]:=bisectstack[bisectptr-5];
bisectstack[bisectptr-2]:=bisectstack[bisectptr-5]+bisectstack[bisectptr
-4];if bisectstack[bisectptr-2]>0 then bisectstack[bisectptr-2]:=0;end;
bisectstack[bisectptr-10]:=mem[p+6].int-mem[p+2].int;
bisectstack[bisectptr-9]:=mem[q+4].int-mem[p+6].int;
bisectstack[bisectptr-8]:=mem[q+2].int-mem[q+4].int;
if bisectstack[bisectptr-10]<0 then if bisectstack[bisectptr-8]>=0 then
begin if bisectstack[bisectptr-9]<0 then bisectstack[bisectptr-7]:=
bisectstack[bisectptr-10]+bisectstack[bisectptr-9]else bisectstack[
bisectptr-7]:=bisectstack[bisectptr-10];
bisectstack[bisectptr-6]:=bisectstack[bisectptr-10]+bisectstack[
bisectptr-9]+bisectstack[bisectptr-8];
if bisectstack[bisectptr-6]<0 then bisectstack[bisectptr-6]:=0;
end else begin bisectstack[bisectptr-7]:=bisectstack[bisectptr-10]+
bisectstack[bisectptr-9]+bisectstack[bisectptr-8];
if bisectstack[bisectptr-7]>bisectstack[bisectptr-10]then bisectstack[
bisectptr-7]:=bisectstack[bisectptr-10];
bisectstack[bisectptr-6]:=bisectstack[bisectptr-10]+bisectstack[
bisectptr-9];
if bisectstack[bisectptr-6]<0 then bisectstack[bisectptr-6]:=0;
end else if bisectstack[bisectptr-8]<=0 then begin if bisectstack[
bisectptr-9]>0 then bisectstack[bisectptr-6]:=bisectstack[bisectptr-10]+
bisectstack[bisectptr-9]else bisectstack[bisectptr-6]:=bisectstack[
bisectptr-10];
bisectstack[bisectptr-7]:=bisectstack[bisectptr-10]+bisectstack[
bisectptr-9]+bisectstack[bisectptr-8];
if bisectstack[bisectptr-7]>0 then bisectstack[bisectptr-7]:=0;
end else begin bisectstack[bisectptr-6]:=bisectstack[bisectptr-10]+
bisectstack[bisectptr-9]+bisectstack[bisectptr-8];
if bisectstack[bisectptr-6]<bisectstack[bisectptr-10]then bisectstack[
bisectptr-6]:=bisectstack[bisectptr-10];
bisectstack[bisectptr-7]:=bisectstack[bisectptr-10]+bisectstack[
bisectptr-9];
if bisectstack[bisectptr-7]>0 then bisectstack[bisectptr-7]:=0;end;
bisectstack[bisectptr-15]:=mem[pp+5].int-mem[pp+1].int;
bisectstack[bisectptr-14]:=mem[qq+3].int-mem[pp+5].int;
bisectstack[bisectptr-13]:=mem[qq+1].int-mem[qq+3].int;
if bisectstack[bisectptr-15]<0 then if bisectstack[bisectptr-13]>=0 then
begin if bisectstack[bisectptr-14]<0 then bisectstack[bisectptr-12]:=
bisectstack[bisectptr-15]+bisectstack[bisectptr-14]else bisectstack[
bisectptr-12]:=bisectstack[bisectptr-15];
bisectstack[bisectptr-11]:=bisectstack[bisectptr-15]+bisectstack[
bisectptr-14]+bisectstack[bisectptr-13];
if bisectstack[bisectptr-11]<0 then bisectstack[bisectptr-11]:=0;
end else begin bisectstack[bisectptr-12]:=bisectstack[bisectptr-15]+
bisectstack[bisectptr-14]+bisectstack[bisectptr-13];
if bisectstack[bisectptr-12]>bisectstack[bisectptr-15]then bisectstack[
bisectptr-12]:=bisectstack[bisectptr-15];
bisectstack[bisectptr-11]:=bisectstack[bisectptr-15]+bisectstack[
bisectptr-14];
if bisectstack[bisectptr-11]<0 then bisectstack[bisectptr-11]:=0;
end else if bisectstack[bisectptr-13]<=0 then begin if bisectstack[
bisectptr-14]>0 then bisectstack[bisectptr-11]:=bisectstack[bisectptr-15
]+bisectstack[bisectptr-14]else bisectstack[bisectptr-11]:=bisectstack[
bisectptr-15];
bisectstack[bisectptr-12]:=bisectstack[bisectptr-15]+bisectstack[
bisectptr-14]+bisectstack[bisectptr-13];
if bisectstack[bisectptr-12]>0 then bisectstack[bisectptr-12]:=0;
end else begin bisectstack[bisectptr-11]:=bisectstack[bisectptr-15]+
bisectstack[bisectptr-14]+bisectstack[bisectptr-13];
if bisectstack[bisectptr-11]<bisectstack[bisectptr-15]then bisectstack[
bisectptr-11]:=bisectstack[bisectptr-15];
bisectstack[bisectptr-12]:=bisectstack[bisectptr-15]+bisectstack[
bisectptr-14];
if bisectstack[bisectptr-12]>0 then bisectstack[bisectptr-12]:=0;end;
bisectstack[bisectptr-20]:=mem[pp+6].int-mem[pp+2].int;
bisectstack[bisectptr-19]:=mem[qq+4].int-mem[pp+6].int;
bisectstack[bisectptr-18]:=mem[qq+2].int-mem[qq+4].int;
if bisectstack[bisectptr-20]<0 then if bisectstack[bisectptr-18]>=0 then
begin if bisectstack[bisectptr-19]<0 then bisectstack[bisectptr-17]:=
bisectstack[bisectptr-20]+bisectstack[bisectptr-19]else bisectstack[
bisectptr-17]:=bisectstack[bisectptr-20];
bisectstack[bisectptr-16]:=bisectstack[bisectptr-20]+bisectstack[
bisectptr-19]+bisectstack[bisectptr-18];
if bisectstack[bisectptr-16]<0 then bisectstack[bisectptr-16]:=0;
end else begin bisectstack[bisectptr-17]:=bisectstack[bisectptr-20]+
bisectstack[bisectptr-19]+bisectstack[bisectptr-18];
if bisectstack[bisectptr-17]>bisectstack[bisectptr-20]then bisectstack[
bisectptr-17]:=bisectstack[bisectptr-20];
bisectstack[bisectptr-16]:=bisectstack[bisectptr-20]+bisectstack[
bisectptr-19];
if bisectstack[bisectptr-16]<0 then bisectstack[bisectptr-16]:=0;
end else if bisectstack[bisectptr-18]<=0 then begin if bisectstack[
bisectptr-19]>0 then bisectstack[bisectptr-16]:=bisectstack[bisectptr-20
]+bisectstack[bisectptr-19]else bisectstack[bisectptr-16]:=bisectstack[
bisectptr-20];
bisectstack[bisectptr-17]:=bisectstack[bisectptr-20]+bisectstack[
bisectptr-19]+bisectstack[bisectptr-18];
if bisectstack[bisectptr-17]>0 then bisectstack[bisectptr-17]:=0;
end else begin bisectstack[bisectptr-16]:=bisectstack[bisectptr-20]+
bisectstack[bisectptr-19]+bisectstack[bisectptr-18];
if bisectstack[bisectptr-16]<bisectstack[bisectptr-20]then bisectstack[
bisectptr-16]:=bisectstack[bisectptr-20];
bisectstack[bisectptr-17]:=bisectstack[bisectptr-20]+bisectstack[
bisectptr-19];
if bisectstack[bisectptr-17]>0 then bisectstack[bisectptr-17]:=0;end;
delx:=mem[p+1].int-mem[pp+1].int;dely:=mem[p+2].int-mem[pp+2].int;
tol:=0;uv:=bisectptr;xy:=bisectptr;threel:=0;curt:=1;curtt:=1{:558};
while true do begin 22:if delx-tol<=bisectstack[xy-11]-bisectstack[uv-2]
then if delx+tol>=bisectstack[xy-12]-bisectstack[uv-1]then if dely-tol<=
bisectstack[xy-16]-bisectstack[uv-7]then if dely+tol>=bisectstack[xy-17]
-bisectstack[uv-6]then begin if curt>=maxt then begin if maxt=131072
then begin curt:=(curt+1)div 2;curtt:=(curtt+1)div 2;goto 10;end;
maxt:=maxt+maxt;apprt:=curt;apprtt:=curtt;end;{559:}
bisectstack[bisectptr]:=delx;bisectstack[bisectptr+1]:=dely;
bisectstack[bisectptr+2]:=tol;bisectstack[bisectptr+3]:=uv;
bisectstack[bisectptr+4]:=xy;bisectptr:=bisectptr+45;curt:=curt+curt;
curtt:=curtt+curtt;bisectstack[bisectptr-25]:=bisectstack[uv-5];
bisectstack[bisectptr-3]:=bisectstack[uv-3];
bisectstack[bisectptr-24]:=(bisectstack[bisectptr-25]+bisectstack[uv-4])
div 2;
bisectstack[bisectptr-4]:=(bisectstack[bisectptr-3]+bisectstack[uv-4])
div 2;bisectstack[bisectptr-23]:=(bisectstack[bisectptr-24]+bisectstack[
bisectptr-4])div 2;bisectstack[bisectptr-5]:=bisectstack[bisectptr-23];
if bisectstack[bisectptr-25]<0 then if bisectstack[bisectptr-23]>=0 then
begin if bisectstack[bisectptr-24]<0 then bisectstack[bisectptr-22]:=
bisectstack[bisectptr-25]+bisectstack[bisectptr-24]else bisectstack[
bisectptr-22]:=bisectstack[bisectptr-25];
bisectstack[bisectptr-21]:=bisectstack[bisectptr-25]+bisectstack[
bisectptr-24]+bisectstack[bisectptr-23];
if bisectstack[bisectptr-21]<0 then bisectstack[bisectptr-21]:=0;
end else begin bisectstack[bisectptr-22]:=bisectstack[bisectptr-25]+
bisectstack[bisectptr-24]+bisectstack[bisectptr-23];
if bisectstack[bisectptr-22]>bisectstack[bisectptr-25]then bisectstack[
bisectptr-22]:=bisectstack[bisectptr-25];
bisectstack[bisectptr-21]:=bisectstack[bisectptr-25]+bisectstack[
bisectptr-24];
if bisectstack[bisectptr-21]<0 then bisectstack[bisectptr-21]:=0;
end else if bisectstack[bisectptr-23]<=0 then begin if bisectstack[
bisectptr-24]>0 then bisectstack[bisectptr-21]:=bisectstack[bisectptr-25
]+bisectstack[bisectptr-24]else bisectstack[bisectptr-21]:=bisectstack[
bisectptr-25];
bisectstack[bisectptr-22]:=bisectstack[bisectptr-25]+bisectstack[
bisectptr-24]+bisectstack[bisectptr-23];
if bisectstack[bisectptr-22]>0 then bisectstack[bisectptr-22]:=0;
end else begin bisectstack[bisectptr-21]:=bisectstack[bisectptr-25]+
bisectstack[bisectptr-24]+bisectstack[bisectptr-23];
if bisectstack[bisectptr-21]<bisectstack[bisectptr-25]then bisectstack[
bisectptr-21]:=bisectstack[bisectptr-25];
bisectstack[bisectptr-22]:=bisectstack[bisectptr-25]+bisectstack[
bisectptr-24];
if bisectstack[bisectptr-22]>0 then bisectstack[bisectptr-22]:=0;end;
if bisectstack[bisectptr-5]<0 then if bisectstack[bisectptr-3]>=0 then
begin if bisectstack[bisectptr-4]<0 then bisectstack[bisectptr-2]:=
bisectstack[bisectptr-5]+bisectstack[bisectptr-4]else bisectstack[
bisectptr-2]:=bisectstack[bisectptr-5];
bisectstack[bisectptr-1]:=bisectstack[bisectptr-5]+bisectstack[bisectptr
-4]+bisectstack[bisectptr-3];
if bisectstack[bisectptr-1]<0 then bisectstack[bisectptr-1]:=0;
end else begin bisectstack[bisectptr-2]:=bisectstack[bisectptr-5]+
bisectstack[bisectptr-4]+bisectstack[bisectptr-3];
if bisectstack[bisectptr-2]>bisectstack[bisectptr-5]then bisectstack[
bisectptr-2]:=bisectstack[bisectptr-5];
bisectstack[bisectptr-1]:=bisectstack[bisectptr-5]+bisectstack[bisectptr
-4];if bisectstack[bisectptr-1]<0 then bisectstack[bisectptr-1]:=0;
end else if bisectstack[bisectptr-3]<=0 then begin if bisectstack[
bisectptr-4]>0 then bisectstack[bisectptr-1]:=bisectstack[bisectptr-5]+
bisectstack[bisectptr-4]else bisectstack[bisectptr-1]:=bisectstack[
bisectptr-5];
bisectstack[bisectptr-2]:=bisectstack[bisectptr-5]+bisectstack[bisectptr
-4]+bisectstack[bisectptr-3];
if bisectstack[bisectptr-2]>0 then bisectstack[bisectptr-2]:=0;
end else begin bisectstack[bisectptr-1]:=bisectstack[bisectptr-5]+
bisectstack[bisectptr-4]+bisectstack[bisectptr-3];
if bisectstack[bisectptr-1]<bisectstack[bisectptr-5]then bisectstack[
bisectptr-1]:=bisectstack[bisectptr-5];
bisectstack[bisectptr-2]:=bisectstack[bisectptr-5]+bisectstack[bisectptr
-4];if bisectstack[bisectptr-2]>0 then bisectstack[bisectptr-2]:=0;end;
bisectstack[bisectptr-30]:=bisectstack[uv-10];
bisectstack[bisectptr-8]:=bisectstack[uv-8];
bisectstack[bisectptr-29]:=(bisectstack[bisectptr-30]+bisectstack[uv-9])
div 2;
bisectstack[bisectptr-9]:=(bisectstack[bisectptr-8]+bisectstack[uv-9])
div 2;bisectstack[bisectptr-28]:=(bisectstack[bisectptr-29]+bisectstack[
bisectptr-9])div 2;bisectstack[bisectptr-10]:=bisectstack[bisectptr-28];
if bisectstack[bisectptr-30]<0 then if bisectstack[bisectptr-28]>=0 then
begin if bisectstack[bisectptr-29]<0 then bisectstack[bisectptr-27]:=
bisectstack[bisectptr-30]+bisectstack[bisectptr-29]else bisectstack[
bisectptr-27]:=bisectstack[bisectptr-30];
bisectstack[bisectptr-26]:=bisectstack[bisectptr-30]+bisectstack[
bisectptr-29]+bisectstack[bisectptr-28];
if bisectstack[bisectptr-26]<0 then bisectstack[bisectptr-26]:=0;
end else begin bisectstack[bisectptr-27]:=bisectstack[bisectptr-30]+
bisectstack[bisectptr-29]+bisectstack[bisectptr-28];
if bisectstack[bisectptr-27]>bisectstack[bisectptr-30]then bisectstack[
bisectptr-27]:=bisectstack[bisectptr-30];
bisectstack[bisectptr-26]:=bisectstack[bisectptr-30]+bisectstack[
bisectptr-29];
if bisectstack[bisectptr-26]<0 then bisectstack[bisectptr-26]:=0;
end else if bisectstack[bisectptr-28]<=0 then begin if bisectstack[
bisectptr-29]>0 then bisectstack[bisectptr-26]:=bisectstack[bisectptr-30
]+bisectstack[bisectptr-29]else bisectstack[bisectptr-26]:=bisectstack[
bisectptr-30];
bisectstack[bisectptr-27]:=bisectstack[bisectptr-30]+bisectstack[
bisectptr-29]+bisectstack[bisectptr-28];
if bisectstack[bisectptr-27]>0 then bisectstack[bisectptr-27]:=0;
end else begin bisectstack[bisectptr-26]:=bisectstack[bisectptr-30]+
bisectstack[bisectptr-29]+bisectstack[bisectptr-28];
if bisectstack[bisectptr-26]<bisectstack[bisectptr-30]then bisectstack[
bisectptr-26]:=bisectstack[bisectptr-30];
bisectstack[bisectptr-27]:=bisectstack[bisectptr-30]+bisectstack[
bisectptr-29];
if bisectstack[bisectptr-27]>0 then bisectstack[bisectptr-27]:=0;end;
if bisectstack[bisectptr-10]<0 then if bisectstack[bisectptr-8]>=0 then
begin if bisectstack[bisectptr-9]<0 then bisectstack[bisectptr-7]:=
bisectstack[bisectptr-10]+bisectstack[bisectptr-9]else bisectstack[
bisectptr-7]:=bisectstack[bisectptr-10];
bisectstack[bisectptr-6]:=bisectstack[bisectptr-10]+bisectstack[
bisectptr-9]+bisectstack[bisectptr-8];
if bisectstack[bisectptr-6]<0 then bisectstack[bisectptr-6]:=0;
end else begin bisectstack[bisectptr-7]:=bisectstack[bisectptr-10]+
bisectstack[bisectptr-9]+bisectstack[bisectptr-8];
if bisectstack[bisectptr-7]>bisectstack[bisectptr-10]then bisectstack[
bisectptr-7]:=bisectstack[bisectptr-10];
bisectstack[bisectptr-6]:=bisectstack[bisectptr-10]+bisectstack[
bisectptr-9];
if bisectstack[bisectptr-6]<0 then bisectstack[bisectptr-6]:=0;
end else if bisectstack[bisectptr-8]<=0 then begin if bisectstack[
bisectptr-9]>0 then bisectstack[bisectptr-6]:=bisectstack[bisectptr-10]+
bisectstack[bisectptr-9]else bisectstack[bisectptr-6]:=bisectstack[
bisectptr-10];
bisectstack[bisectptr-7]:=bisectstack[bisectptr-10]+bisectstack[
bisectptr-9]+bisectstack[bisectptr-8];
if bisectstack[bisectptr-7]>0 then bisectstack[bisectptr-7]:=0;
end else begin bisectstack[bisectptr-6]:=bisectstack[bisectptr-10]+
bisectstack[bisectptr-9]+bisectstack[bisectptr-8];
if bisectstack[bisectptr-6]<bisectstack[bisectptr-10]then bisectstack[
bisectptr-6]:=bisectstack[bisectptr-10];
bisectstack[bisectptr-7]:=bisectstack[bisectptr-10]+bisectstack[
bisectptr-9];
if bisectstack[bisectptr-7]>0 then bisectstack[bisectptr-7]:=0;end;
bisectstack[bisectptr-35]:=bisectstack[xy-15];
bisectstack[bisectptr-13]:=bisectstack[xy-13];
bisectstack[bisectptr-34]:=(bisectstack[bisectptr-35]+bisectstack[xy-14]
)div 2;
bisectstack[bisectptr-14]:=(bisectstack[bisectptr-13]+bisectstack[xy-14]
)div 2;
bisectstack[bisectptr-33]:=(bisectstack[bisectptr-34]+bisectstack[
bisectptr-14])div 2;
bisectstack[bisectptr-15]:=bisectstack[bisectptr-33];
if bisectstack[bisectptr-35]<0 then if bisectstack[bisectptr-33]>=0 then
begin if bisectstack[bisectptr-34]<0 then bisectstack[bisectptr-32]:=
bisectstack[bisectptr-35]+bisectstack[bisectptr-34]else bisectstack[
bisectptr-32]:=bisectstack[bisectptr-35];
bisectstack[bisectptr-31]:=bisectstack[bisectptr-35]+bisectstack[
bisectptr-34]+bisectstack[bisectptr-33];
if bisectstack[bisectptr-31]<0 then bisectstack[bisectptr-31]:=0;
end else begin bisectstack[bisectptr-32]:=bisectstack[bisectptr-35]+
bisectstack[bisectptr-34]+bisectstack[bisectptr-33];
if bisectstack[bisectptr-32]>bisectstack[bisectptr-35]then bisectstack[
bisectptr-32]:=bisectstack[bisectptr-35];
bisectstack[bisectptr-31]:=bisectstack[bisectptr-35]+bisectstack[
bisectptr-34];
if bisectstack[bisectptr-31]<0 then bisectstack[bisectptr-31]:=0;
end else if bisectstack[bisectptr-33]<=0 then begin if bisectstack[
bisectptr-34]>0 then bisectstack[bisectptr-31]:=bisectstack[bisectptr-35
]+bisectstack[bisectptr-34]else bisectstack[bisectptr-31]:=bisectstack[
bisectptr-35];
bisectstack[bisectptr-32]:=bisectstack[bisectptr-35]+bisectstack[
bisectptr-34]+bisectstack[bisectptr-33];
if bisectstack[bisectptr-32]>0 then bisectstack[bisectptr-32]:=0;
end else begin bisectstack[bisectptr-31]:=bisectstack[bisectptr-35]+
bisectstack[bisectptr-34]+bisectstack[bisectptr-33];
if bisectstack[bisectptr-31]<bisectstack[bisectptr-35]then bisectstack[
bisectptr-31]:=bisectstack[bisectptr-35];
bisectstack[bisectptr-32]:=bisectstack[bisectptr-35]+bisectstack[
bisectptr-34];
if bisectstack[bisectptr-32]>0 then bisectstack[bisectptr-32]:=0;end;
if bisectstack[bisectptr-15]<0 then if bisectstack[bisectptr-13]>=0 then
begin if bisectstack[bisectptr-14]<0 then bisectstack[bisectptr-12]:=
bisectstack[bisectptr-15]+bisectstack[bisectptr-14]else bisectstack[
bisectptr-12]:=bisectstack[bisectptr-15];
bisectstack[bisectptr-11]:=bisectstack[bisectptr-15]+bisectstack[
bisectptr-14]+bisectstack[bisectptr-13];
if bisectstack[bisectptr-11]<0 then bisectstack[bisectptr-11]:=0;
end else begin bisectstack[bisectptr-12]:=bisectstack[bisectptr-15]+
bisectstack[bisectptr-14]+bisectstack[bisectptr-13];
if bisectstack[bisectptr-12]>bisectstack[bisectptr-15]then bisectstack[
bisectptr-12]:=bisectstack[bisectptr-15];
bisectstack[bisectptr-11]:=bisectstack[bisectptr-15]+bisectstack[
bisectptr-14];
if bisectstack[bisectptr-11]<0 then bisectstack[bisectptr-11]:=0;
end else if bisectstack[bisectptr-13]<=0 then begin if bisectstack[
bisectptr-14]>0 then bisectstack[bisectptr-11]:=bisectstack[bisectptr-15
]+bisectstack[bisectptr-14]else bisectstack[bisectptr-11]:=bisectstack[
bisectptr-15];
bisectstack[bisectptr-12]:=bisectstack[bisectptr-15]+bisectstack[
bisectptr-14]+bisectstack[bisectptr-13];
if bisectstack[bisectptr-12]>0 then bisectstack[bisectptr-12]:=0;
end else begin bisectstack[bisectptr-11]:=bisectstack[bisectptr-15]+
bisectstack[bisectptr-14]+bisectstack[bisectptr-13];
if bisectstack[bisectptr-11]<bisectstack[bisectptr-15]then bisectstack[
bisectptr-11]:=bisectstack[bisectptr-15];
bisectstack[bisectptr-12]:=bisectstack[bisectptr-15]+bisectstack[
bisectptr-14];
if bisectstack[bisectptr-12]>0 then bisectstack[bisectptr-12]:=0;end;
bisectstack[bisectptr-40]:=bisectstack[xy-20];
bisectstack[bisectptr-18]:=bisectstack[xy-18];
bisectstack[bisectptr-39]:=(bisectstack[bisectptr-40]+bisectstack[xy-19]
)div 2;
bisectstack[bisectptr-19]:=(bisectstack[bisectptr-18]+bisectstack[xy-19]
)div 2;
bisectstack[bisectptr-38]:=(bisectstack[bisectptr-39]+bisectstack[
bisectptr-19])div 2;
bisectstack[bisectptr-20]:=bisectstack[bisectptr-38];
if bisectstack[bisectptr-40]<0 then if bisectstack[bisectptr-38]>=0 then
begin if bisectstack[bisectptr-39]<0 then bisectstack[bisectptr-37]:=
bisectstack[bisectptr-40]+bisectstack[bisectptr-39]else bisectstack[
bisectptr-37]:=bisectstack[bisectptr-40];
bisectstack[bisectptr-36]:=bisectstack[bisectptr-40]+bisectstack[
bisectptr-39]+bisectstack[bisectptr-38];
if bisectstack[bisectptr-36]<0 then bisectstack[bisectptr-36]:=0;
end else begin bisectstack[bisectptr-37]:=bisectstack[bisectptr-40]+
bisectstack[bisectptr-39]+bisectstack[bisectptr-38];
if bisectstack[bisectptr-37]>bisectstack[bisectptr-40]then bisectstack[
bisectptr-37]:=bisectstack[bisectptr-40];
bisectstack[bisectptr-36]:=bisectstack[bisectptr-40]+bisectstack[
bisectptr-39];
if bisectstack[bisectptr-36]<0 then bisectstack[bisectptr-36]:=0;
end else if bisectstack[bisectptr-38]<=0 then begin if bisectstack[
bisectptr-39]>0 then bisectstack[bisectptr-36]:=bisectstack[bisectptr-40
]+bisectstack[bisectptr-39]else bisectstack[bisectptr-36]:=bisectstack[
bisectptr-40];
bisectstack[bisectptr-37]:=bisectstack[bisectptr-40]+bisectstack[
bisectptr-39]+bisectstack[bisectptr-38];
if bisectstack[bisectptr-37]>0 then bisectstack[bisectptr-37]:=0;
end else begin bisectstack[bisectptr-36]:=bisectstack[bisectptr-40]+
bisectstack[bisectptr-39]+bisectstack[bisectptr-38];
if bisectstack[bisectptr-36]<bisectstack[bisectptr-40]then bisectstack[
bisectptr-36]:=bisectstack[bisectptr-40];
bisectstack[bisectptr-37]:=bisectstack[bisectptr-40]+bisectstack[
bisectptr-39];
if bisectstack[bisectptr-37]>0 then bisectstack[bisectptr-37]:=0;end;
if bisectstack[bisectptr-20]<0 then if bisectstack[bisectptr-18]>=0 then
begin if bisectstack[bisectptr-19]<0 then bisectstack[bisectptr-17]:=
bisectstack[bisectptr-20]+bisectstack[bisectptr-19]else bisectstack[
bisectptr-17]:=bisectstack[bisectptr-20];
bisectstack[bisectptr-16]:=bisectstack[bisectptr-20]+bisectstack[
bisectptr-19]+bisectstack[bisectptr-18];
if bisectstack[bisectptr-16]<0 then bisectstack[bisectptr-16]:=0;
end else begin bisectstack[bisectptr-17]:=bisectstack[bisectptr-20]+
bisectstack[bisectptr-19]+bisectstack[bisectptr-18];
if bisectstack[bisectptr-17]>bisectstack[bisectptr-20]then bisectstack[
bisectptr-17]:=bisectstack[bisectptr-20];
bisectstack[bisectptr-16]:=bisectstack[bisectptr-20]+bisectstack[
bisectptr-19];
if bisectstack[bisectptr-16]<0 then bisectstack[bisectptr-16]:=0;
end else if bisectstack[bisectptr-18]<=0 then begin if bisectstack[
bisectptr-19]>0 then bisectstack[bisectptr-16]:=bisectstack[bisectptr-20
]+bisectstack[bisectptr-19]else bisectstack[bisectptr-16]:=bisectstack[
bisectptr-20];
bisectstack[bisectptr-17]:=bisectstack[bisectptr-20]+bisectstack[
bisectptr-19]+bisectstack[bisectptr-18];
if bisectstack[bisectptr-17]>0 then bisectstack[bisectptr-17]:=0;
end else begin bisectstack[bisectptr-16]:=bisectstack[bisectptr-20]+
bisectstack[bisectptr-19]+bisectstack[bisectptr-18];
if bisectstack[bisectptr-16]<bisectstack[bisectptr-20]then bisectstack[
bisectptr-16]:=bisectstack[bisectptr-20];
bisectstack[bisectptr-17]:=bisectstack[bisectptr-20]+bisectstack[
bisectptr-19];
if bisectstack[bisectptr-17]>0 then bisectstack[bisectptr-17]:=0;end;
uv:=bisectptr-20;xy:=bisectptr-20;delx:=delx+delx;dely:=dely+dely;
tol:=tol-threel+tolstep;tol:=tol+tol;threel:=threel+tolstep{:559};
goto 22;end;
if timetogo>0 then decr(timetogo)else begin while apprt<65536 do begin
apprt:=apprt+apprt;apprtt:=apprtt+apprtt;end;curt:=apprt;curtt:=apprtt;
goto 10;end;{560:}45:if odd(curtt)then if odd(curt)then{561:}
begin curt:=(curt)div 2;curtt:=(curtt)div 2;if curt=0 then goto 10;
bisectptr:=bisectptr-45;threel:=threel-tolstep;
delx:=bisectstack[bisectptr];dely:=bisectstack[bisectptr+1];
tol:=bisectstack[bisectptr+2];uv:=bisectstack[bisectptr+3];
xy:=bisectstack[bisectptr+4];goto 45;end{:561}else begin incr(curt);
delx:=delx+bisectstack[uv-5]+bisectstack[uv-4]+bisectstack[uv-3];
dely:=dely+bisectstack[uv-10]+bisectstack[uv-9]+bisectstack[uv-8];
uv:=uv+20;decr(curtt);xy:=xy-20;
delx:=delx+bisectstack[xy-15]+bisectstack[xy-14]+bisectstack[xy-13];
dely:=dely+bisectstack[xy-20]+bisectstack[xy-19]+bisectstack[xy-18];
end else begin incr(curtt);tol:=tol+threel;
delx:=delx-bisectstack[xy-15]-bisectstack[xy-14]-bisectstack[xy-13];
dely:=dely-bisectstack[xy-20]-bisectstack[xy-19]-bisectstack[xy-18];
xy:=xy+20;end{:560};end;10:end;{:556}{562:}
procedure pathintersection(h,hh:halfword);label 10;var p,pp:halfword;
n,nn:integer;begin{563:}
if mem[h].hh.b1=0 then begin mem[h+5].int:=mem[h+1].int;
mem[h+3].int:=mem[h+1].int;mem[h+6].int:=mem[h+2].int;
mem[h+4].int:=mem[h+2].int;mem[h].hh.b1:=1;end;
if mem[hh].hh.b1=0 then begin mem[hh+5].int:=mem[hh+1].int;
mem[hh+3].int:=mem[hh+1].int;mem[hh+6].int:=mem[hh+2].int;
mem[hh+4].int:=mem[hh+2].int;mem[hh].hh.b1:=1;end;{:563};tolstep:=0;
repeat n:=-65536;p:=h;repeat if mem[p].hh.b1<>0 then begin nn:=-65536;
pp:=hh;repeat if mem[pp].hh.b1<>0 then begin cubicintersection(p,pp);
if curt>0 then begin curt:=curt+n;curtt:=curtt+nn;goto 10;end;end;
nn:=nn+65536;pp:=mem[pp].hh.rh;until pp=hh;end;n:=n+65536;
p:=mem[p].hh.rh;until p=h;tolstep:=tolstep+3;until tolstep>3;
curt:=-65536;curtt:=-65536;10:end;{:562}{574:}
procedure openawindow(k:windownumber;r0,c0,r1,c1:scaled;x,y:scaled);
var m,n:integer;begin{575:}
if r0<0 then r0:=0 else r0:=roundunscaled(r0);r1:=roundunscaled(r1);
if r1>screendepth then r1:=screendepth;
if r1<r0 then if r0>screendepth then r0:=r1 else r1:=r0;
if c0<0 then c0:=0 else c0:=roundunscaled(c0);c1:=roundunscaled(c1);
if c1>screenwidth then c1:=screenwidth;
if c1<c0 then if c0>screenwidth then c0:=c1 else c1:=c0{:575};
windowopen[k]:=true;incr(windowtime[k]);leftcol[k]:=c0;rightcol[k]:=c1;
toprow[k]:=r0;botrow[k]:=r1;{576:}m:=roundunscaled(x);
n:=roundunscaled(y)-1;mwindow[k]:=c0-m;nwindow[k]:=r0+n{:576};
begin if not screenstarted then begin screenOK:=initscreen;
screenstarted:=true;end;end;
if screenOK then begin blankrectangle(c0,c1,r0,r1);updatescreen;end;end;
{:574}{577:}procedure dispedges(k:windownumber);label 30,40;
var p,q:halfword;alreadythere:boolean;r:integer;{580:}n:screencol;
w,ww:integer;b:pixelcolor;m,mm:integer;d:integer;madjustment:integer;
rightedge:integer;mincol:screencol;{:580}
begin if screenOK then if leftcol[k]<rightcol[k]then if toprow[k]<botrow
[k]then begin alreadythere:=false;
if mem[curedges+3].hh.rh=k then if mem[curedges+4].int=windowtime[k]then
alreadythere:=true;
if not alreadythere then blankrectangle(leftcol[k],rightcol[k],toprow[k]
,botrow[k]);{581:}madjustment:=mwindow[k]-mem[curedges+3].hh.lh;
rightedge:=8*(rightcol[k]-madjustment);mincol:=leftcol[k]{:581};
p:=mem[curedges].hh.rh;r:=nwindow[k]-(mem[curedges+1].hh.lh-4096);
while(p<>curedges)and(r>=toprow[k])do begin if r<botrow[k]then{578:}
begin if mem[p+1].hh.lh>1 then sortedges(p)else if mem[p+1].hh.lh=1 then
if alreadythere then goto 30;mem[p+1].hh.lh:=1;{582:}n:=0;ww:=0;m:=-1;
w:=0;q:=mem[p+1].hh.rh;rowtransition[0]:=mincol;
while true do begin if q=memtop then d:=rightedge else d:=mem[q].hh.lh;
mm:=(d div 8)+madjustment;if mm<>m then begin{583:}
if w<=0 then begin if ww>0 then if m>mincol then begin if n=0 then if
alreadythere then begin b:=0;incr(n);end else b:=1 else incr(n);
rowtransition[n]:=m;end;
end else if ww<=0 then if m>mincol then begin if n=0 then b:=1;incr(n);
rowtransition[n]:=m;end{:583};m:=mm;w:=ww;end;
if d>=rightedge then goto 40;ww:=ww+(d mod 8)-4;q:=mem[q].hh.rh;end;
40:{584:}
if alreadythere or(ww>0)then begin if n=0 then if ww>0 then b:=1 else b
:=0;incr(n);rowtransition[n]:=rightcol[k];
end else if n=0 then goto 30{:584};{:582};paintrow(r,b,rowtransition,n);
30:end{:578};p:=mem[p].hh.rh;decr(r);end;updatescreen;
incr(windowtime[k]);mem[curedges+3].hh.rh:=k;
mem[curedges+4].int:=windowtime[k];end;end;{:577}{591:}
function maxcoef(p:halfword):fraction;var x:fraction;begin x:=0;
while mem[p].hh.lh<>0 do begin if abs(mem[p+1].int)>x then x:=abs(mem[p
+1].int);p:=mem[p].hh.rh;end;maxcoef:=x;end;{:591}{597:}
function pplusq(p:halfword;q:halfword;t:smallnumber):halfword;label 30;
var pp,qq:halfword;r,s:halfword;threshold:integer;v:integer;
begin if t=17 then threshold:=2685 else threshold:=8;r:=memtop-1;
pp:=mem[p].hh.lh;qq:=mem[q].hh.lh;
while true do if pp=qq then if pp=0 then goto 30 else{598:}
begin v:=mem[p+1].int+mem[q+1].int;mem[p+1].int:=v;s:=p;p:=mem[p].hh.rh;
pp:=mem[p].hh.lh;
if abs(v)<threshold then freenode(s,2)else begin if abs(v)>=626349397
then if watchcoefs then begin mem[qq].hh.b0:=0;fixneeded:=true;end;
mem[r].hh.rh:=s;r:=s;end;q:=mem[q].hh.rh;qq:=mem[q].hh.lh;end{:598}
else if mem[pp+1].int<mem[qq+1].int then begin s:=getnode(2);
mem[s].hh.lh:=qq;mem[s+1].int:=mem[q+1].int;q:=mem[q].hh.rh;
qq:=mem[q].hh.lh;mem[r].hh.rh:=s;r:=s;end else begin mem[r].hh.rh:=p;
r:=p;p:=mem[p].hh.rh;pp:=mem[p].hh.lh;end;
30:mem[p+1].int:=slowadd(mem[p+1].int,mem[q+1].int);mem[r].hh.rh:=p;
depfinal:=p;pplusq:=mem[memtop-1].hh.rh;end;{:597}{599:}
function ptimesv(p:halfword;v:integer;t0,t1:smallnumber;
visscaled:boolean):halfword;var r,s:halfword;w:integer;
threshold:integer;scalingdown:boolean;
begin if t0<>t1 then scalingdown:=true else scalingdown:=not visscaled;
if t1=17 then threshold:=1342 else threshold:=4;r:=memtop-1;
while mem[p].hh.lh<>0 do begin if scalingdown then w:=takefraction(v,mem
[p+1].int)else w:=takescaled(v,mem[p+1].int);
if abs(w)<=threshold then begin s:=mem[p].hh.rh;freenode(p,2);p:=s;
end else begin if abs(w)>=626349397 then begin fixneeded:=true;
mem[mem[p].hh.lh].hh.b0:=0;end;mem[r].hh.rh:=p;r:=p;mem[p+1].int:=w;
p:=mem[p].hh.rh;end;end;mem[r].hh.rh:=p;
if visscaled then mem[p+1].int:=takescaled(mem[p+1].int,v)else mem[p+1].
int:=takefraction(mem[p+1].int,v);ptimesv:=mem[memtop-1].hh.rh;end;
{:599}{601:}function pwithxbecomingq(p,x,q:halfword;
t:smallnumber):halfword;var r,s:halfword;v:integer;sx:integer;
begin s:=p;r:=memtop-1;sx:=mem[x+1].int;
while mem[mem[s].hh.lh+1].int>sx do begin r:=s;s:=mem[s].hh.rh;end;
if mem[s].hh.lh<>x then pwithxbecomingq:=p else begin mem[memtop-1].hh.
rh:=p;mem[r].hh.rh:=mem[s].hh.rh;v:=mem[s+1].int;freenode(s,2);
pwithxbecomingq:=pplusfq(mem[memtop-1].hh.rh,v,q,t,17);end;end;{:601}
{606:}procedure newdep(q,p:halfword);var r:halfword;
begin mem[q+1].hh.rh:=p;mem[q+1].hh.lh:=13;r:=mem[13].hh.rh;
mem[depfinal].hh.rh:=r;mem[r+1].hh.lh:=depfinal;mem[13].hh.rh:=q;end;
{:606}{607:}function constdependency(v:scaled):halfword;
begin depfinal:=getnode(2);mem[depfinal+1].int:=v;
mem[depfinal].hh.lh:=0;constdependency:=depfinal;end;{:607}{608:}
function singledependency(p:halfword):halfword;var q:halfword;m:integer;
begin m:=mem[p+1].int mod 64;
if m>28 then singledependency:=constdependency(0)else begin q:=getnode(2
);mem[q+1].int:=twotothe[28-m];mem[q].hh.lh:=p;
mem[q].hh.rh:=constdependency(0);singledependency:=q;end;end;{:608}
{609:}function copydeplist(p:halfword):halfword;label 30;var q:halfword;
begin q:=getnode(2);depfinal:=q;
while true do begin mem[depfinal].hh.lh:=mem[p].hh.lh;
mem[depfinal+1].int:=mem[p+1].int;if mem[depfinal].hh.lh=0 then goto 30;
mem[depfinal].hh.rh:=getnode(2);depfinal:=mem[depfinal].hh.rh;
p:=mem[p].hh.rh;end;30:copydeplist:=q;end;{:609}{610:}
procedure lineareq(p:halfword;t:smallnumber);var q,r,s:halfword;
x:halfword;n:integer;v:integer;prevr:halfword;finalnode:halfword;
w:integer;begin{611:}q:=p;r:=mem[p].hh.rh;v:=mem[q+1].int;
while mem[r].hh.lh<>0 do begin if abs(mem[r+1].int)>abs(v)then begin q:=
r;v:=mem[r+1].int;end;r:=mem[r].hh.rh;end{:611};x:=mem[q].hh.lh;
n:=mem[x+1].int mod 64;{612:}s:=memtop-1;mem[s].hh.rh:=p;r:=p;
repeat if r=q then begin mem[s].hh.rh:=mem[r].hh.rh;freenode(r,2);
end else begin w:=makefraction(mem[r+1].int,v);
if abs(w)<=1342 then begin mem[s].hh.rh:=mem[r].hh.rh;freenode(r,2);
end else begin mem[r+1].int:=-w;s:=r;end;end;r:=mem[s].hh.rh;
until mem[r].hh.lh=0;
if t=18 then mem[r+1].int:=-makescaled(mem[r+1].int,v)else if v<>
-268435456 then mem[r+1].int:=-makefraction(mem[r+1].int,v);
finalnode:=r;p:=mem[memtop-1].hh.rh{:612};if internal[2]>0 then{613:}
if interesting(x)then begin begindiagnostic;printnl(594);
printvariablename(x);w:=n;while w>0 do begin print(587);w:=w-2;end;
printchar(61);printdependency(p,17);enddiagnostic(false);end{:613};
{614:}prevr:=13;r:=mem[13].hh.rh;while r<>13 do begin s:=mem[r+1].hh.rh;
q:=pwithxbecomingq(s,x,p,mem[r].hh.b0);
if mem[q].hh.lh=0 then makeknown(r,q)else begin mem[r+1].hh.rh:=q;
repeat q:=mem[q].hh.rh;until mem[q].hh.lh=0;prevr:=q;end;
r:=mem[prevr].hh.rh;end{:614};{615:}if n>0 then{616:}begin s:=memtop-1;
mem[memtop-1].hh.rh:=p;r:=p;
repeat if n>30 then w:=0 else w:=mem[r+1].int div twotothe[n];
if(abs(w)<=1342)and(mem[r].hh.lh<>0)then begin mem[s].hh.rh:=mem[r].hh.
rh;freenode(r,2);end else begin mem[r+1].int:=w;s:=r;end;
r:=mem[s].hh.rh;until mem[s].hh.lh=0;p:=mem[memtop-1].hh.rh;end{:616};
if mem[p].hh.lh=0 then begin mem[x].hh.b0:=16;
mem[x+1].int:=mem[p+1].int;
if abs(mem[x+1].int)>=268435456 then valtoobig(mem[x+1].int);
freenode(p,2);
if curexp=x then if curtype=19 then begin curexp:=mem[x+1].int;
curtype:=16;freenode(x,2);end;end else begin mem[x].hh.b0:=17;
depfinal:=finalnode;newdep(x,p);
if curexp=x then if curtype=19 then curtype:=17;end{:615};
if fixneeded then fixdependencies;end;{:610}{619:}
function newringentry(p:halfword):halfword;var q:halfword;
begin q:=getnode(2);mem[q].hh.b1:=11;mem[q].hh.b0:=mem[p].hh.b0;
if mem[p+1].int=0 then mem[q+1].int:=p else mem[q+1].int:=mem[p+1].int;
mem[p+1].int:=q;newringentry:=q;end;{:619}{621:}
procedure nonlineareq(v:integer;p:halfword;flushp:boolean);
var t:smallnumber;q,r:halfword;begin t:=mem[p].hh.b0-1;q:=mem[p+1].int;
if flushp then mem[p].hh.b0:=1 else p:=q;repeat r:=mem[q+1].int;
mem[q].hh.b0:=t;case t of 2:mem[q+1].int:=v;4:begin mem[q+1].int:=v;
begin if strref[v]<127 then incr(strref[v]);end;end;
6:begin mem[q+1].int:=v;incr(mem[v].hh.lh);end;
9:mem[q+1].int:=copypath(v);11:mem[q+1].int:=copyedges(v);end;q:=r;
until q=p;end;{:621}{622:}procedure ringmerge(p,q:halfword);label 10;
var r:halfword;begin r:=mem[p+1].int;
while r<>p do begin if r=q then begin{623:}
begin begin if interaction=3 then;printnl(261);print(597);end;
begin helpptr:=2;helpline[1]:=598;helpline[0]:=599;end;putgeterror;
end{:623};goto 10;end;r:=mem[r+1].int;end;r:=mem[p+1].int;
mem[p+1].int:=mem[q+1].int;mem[q+1].int:=r;10:end;{:622}{626:}
procedure showcmdmod(c,m:integer);begin begindiagnostic;printnl(123);
printcmdmod(c,m);printchar(125);enddiagnostic(false);end;{:626}{635:}
procedure showcontext;label 30;var oldsetting:0..5;{641:}i:0..bufsize;
l:integer;m:integer;n:0..errorline;p:integer;q:integer;{:641}
begin fileptr:=inputptr;inputstack[fileptr]:=curinput;
while true do begin curinput:=inputstack[fileptr];{636:}
if(fileptr=inputptr)or(curinput.indexfield<=15)or(curinput.indexfield<>
19)or(curinput.locfield<>0)then begin tally:=0;oldsetting:=selector;
if(curinput.indexfield<=15)then begin{637:}
if curinput.namefield<=1 then if(curinput.namefield=0)and(fileptr=0)then
printnl(601)else printnl(602)else if curinput.namefield=2 then printnl(
603)else begin printnl(604);printint(line);end;printchar(32){:637};
{644:}begin l:=tally;tally:=0;selector:=4;trickcount:=1000000;end;
if curinput.limitfield>0 then for i:=curinput.startfield to curinput.
limitfield-1 do begin if i=curinput.locfield then begin firstcount:=
tally;trickcount:=tally+1+errorline-halferrorline;
if trickcount<errorline then trickcount:=errorline;end;print(buffer[i]);
end{:644};end else begin{638:}
case curinput.indexfield of 16:printnl(605);17:{639:}begin printnl(610);
p:=paramstack[curinput.limitfield];
if p<>0 then if mem[p].hh.rh=1 then printexp(p,0)else showtokenlist(p,0,
20,tally);print(611);end{:639};18:printnl(606);
19:if curinput.locfield=0 then printnl(607)else printnl(608);
20:printnl(609);21:begin println;
if curinput.namefield<>0 then slowprint(hash[curinput.namefield].rh)else
{640:}begin p:=paramstack[curinput.limitfield];
if p=0 then showtokenlist(paramstack[curinput.limitfield+1],0,20,tally)
else begin q:=p;while mem[q].hh.rh<>0 do q:=mem[q].hh.rh;
mem[q].hh.rh:=paramstack[curinput.limitfield+1];
showtokenlist(p,0,20,tally);mem[q].hh.rh:=0;end;end{:640};print(500);
end;others:printnl(63)end{:638};{645:}begin l:=tally;tally:=0;
selector:=4;trickcount:=1000000;end;
if curinput.indexfield<>21 then showtokenlist(curinput.startfield,
curinput.locfield,100000,0)else showmacro(curinput.startfield,curinput.
locfield,100000){:645};end;selector:=oldsetting;{643:}
if trickcount=1000000 then begin firstcount:=tally;
trickcount:=tally+1+errorline-halferrorline;
if trickcount<errorline then trickcount:=errorline;end;
if tally<trickcount then m:=tally-firstcount else m:=trickcount-
firstcount;if l+firstcount<=halferrorline then begin p:=0;
n:=l+firstcount;end else begin print(274);
p:=l+firstcount-halferrorline+3;n:=halferrorline;end;
for q:=p to firstcount-1 do printchar(trickbuf[q mod errorline]);
println;for q:=1 to n do printchar(32);
if m+n<=errorline then p:=firstcount+m else p:=firstcount+(errorline-n-3
);for q:=firstcount to p-1 do printchar(trickbuf[q mod errorline]);
if m+n>errorline then print(274){:643};end{:636};
if(curinput.indexfield<=15)then if(curinput.namefield>2)or(fileptr=0)
then goto 30;decr(fileptr);end;30:curinput:=inputstack[inputptr];end;
{:635}{649:}procedure begintokenlist(p:halfword;t:quarterword);
begin begin if inputptr>maxinstack then begin maxinstack:=inputptr;
if inputptr=stacksize then overflow(612,stacksize);end;
inputstack[inputptr]:=curinput;incr(inputptr);end;
curinput.startfield:=p;curinput.indexfield:=t;
curinput.limitfield:=paramptr;curinput.locfield:=p;end;{:649}{650:}
procedure endtokenlist;label 30;var p:halfword;
begin if curinput.indexfield>=19 then if curinput.indexfield<=20 then
begin flushtokenlist(curinput.startfield);goto 30;
end else deletemacref(curinput.startfield);
while paramptr>curinput.limitfield do begin decr(paramptr);
p:=paramstack[paramptr];
if p<>0 then if mem[p].hh.rh=1 then begin recyclevalue(p);freenode(p,2);
end else flushtokenlist(p);end;30:begin decr(inputptr);
curinput:=inputstack[inputptr];end;
begin if interrupt<>0 then pauseforinstructions;end;end;{:650}{651:}
{855:}{856:}procedure encapsulate(p:halfword);begin curexp:=getnode(2);
mem[curexp].hh.b0:=curtype;mem[curexp].hh.b1:=11;newdep(curexp,p);end;
{:856}{858:}procedure install(r,q:halfword);var p:halfword;
begin if mem[q].hh.b0=16 then begin mem[r+1].int:=mem[q+1].int;
mem[r].hh.b0:=16;
end else if mem[q].hh.b0=19 then begin p:=singledependency(q);
if p=depfinal then begin mem[r].hh.b0:=16;mem[r+1].int:=0;freenode(p,2);
end else begin mem[r].hh.b0:=17;newdep(r,p);end;
end else begin mem[r].hh.b0:=mem[q].hh.b0;
newdep(r,copydeplist(mem[q+1].hh.rh));end;end;{:858}
procedure makeexpcopy(p:halfword);label 20;var q,r,t:halfword;
begin 20:curtype:=mem[p].hh.b0;
case curtype of 1,2,16:curexp:=mem[p+1].int;
3,5,7,12,10:curexp:=newringentry(p);4:begin curexp:=mem[p+1].int;
begin if strref[curexp]<127 then incr(strref[curexp]);end;end;
6:begin curexp:=mem[p+1].int;incr(mem[curexp].hh.lh);end;
11:curexp:=copyedges(mem[p+1].int);9,8:curexp:=copypath(mem[p+1].int);
13,14:{857:}begin if mem[p+1].int=0 then initbignode(p);t:=getnode(2);
mem[t].hh.b1:=11;mem[t].hh.b0:=curtype;initbignode(t);
q:=mem[p+1].int+bignodesize[curtype];
r:=mem[t+1].int+bignodesize[curtype];repeat q:=q-2;r:=r-2;install(r,q);
until q=mem[p+1].int;curexp:=t;end{:857};
17,18:encapsulate(copydeplist(mem[p+1].hh.rh));
15:begin begin mem[p].hh.b0:=19;serialno:=serialno+64;
mem[p+1].int:=serialno;end;goto 20;end;19:begin q:=singledependency(p);
if q=depfinal then begin curtype:=16;curexp:=0;freenode(q,2);
end else begin curtype:=17;encapsulate(q);end;end;
others:confusion(797)end;end;{:855}function curtok:halfword;
var p:halfword;savetype:smallnumber;saveexp:integer;
begin if cursym=0 then if curcmd=38 then begin savetype:=curtype;
saveexp:=curexp;makeexpcopy(curmod);p:=stashcurexp;mem[p].hh.rh:=0;
curtype:=savetype;curexp:=saveexp;end else begin p:=getnode(2);
mem[p+1].int:=curmod;mem[p].hh.b1:=12;
if curcmd=42 then mem[p].hh.b0:=16 else mem[p].hh.b0:=4;
end else begin begin p:=avail;
if p=0 then p:=getavail else begin avail:=mem[p].hh.rh;mem[p].hh.rh:=0;
ifdef('STAT')incr(dynused);endif('STAT')end;end;mem[p].hh.lh:=cursym;
end;curtok:=p;end;{:651}{652:}procedure backinput;var p:halfword;
begin p:=curtok;
while(curinput.indexfield>15)and(curinput.locfield=0)do endtokenlist;
begintokenlist(p,19);end;{:652}{653:}procedure backerror;
begin OKtointerrupt:=false;backinput;OKtointerrupt:=true;error;end;
procedure inserror;begin OKtointerrupt:=false;backinput;
curinput.indexfield:=20;OKtointerrupt:=true;error;end;{:653}{654:}
procedure beginfilereading;begin if inopen=15 then overflow(613,15);
if first=bufsize then overflow(256,bufsize);incr(inopen);
begin if inputptr>maxinstack then begin maxinstack:=inputptr;
if inputptr=stacksize then overflow(612,stacksize);end;
inputstack[inputptr]:=curinput;incr(inputptr);end;
curinput.indexfield:=inopen;linestack[curinput.indexfield]:=line;
curinput.startfield:=first;curinput.namefield:=0;end;{:654}{655:}
procedure endfilereading;begin first:=curinput.startfield;
line:=linestack[curinput.indexfield];
if curinput.indexfield<>inopen then confusion(614);
if curinput.namefield>2 then aclose(inputfile[curinput.indexfield]);
begin decr(inputptr);curinput:=inputstack[inputptr];end;decr(inopen);
end;{:655}{656:}procedure clearforerrorprompt;
begin while(curinput.indexfield<=15)and(curinput.namefield=0)and(
inputptr>0)and(curinput.locfield=curinput.limitfield)do endfilereading;
println;;end;{:656}{661:}function checkoutervalidity:boolean;
var p:halfword;
begin if scannerstatus=0 then checkoutervalidity:=true else begin
deletionsallowed:=false;{662:}if cursym<>0 then begin p:=getavail;
mem[p].hh.lh:=cursym;begintokenlist(p,19);end{:662};
if scannerstatus>1 then{663:}begin runaway;
if cursym=0 then begin if interaction=3 then;printnl(261);print(620);
end else begin begin if interaction=3 then;printnl(261);print(621);end;
end;print(622);begin helpptr:=4;helpline[3]:=623;helpline[2]:=624;
helpline[1]:=625;helpline[0]:=626;end;case scannerstatus of{664:}
2:begin print(627);helpline[3]:=628;cursym:=9763;end;3:begin print(629);
helpline[3]:=630;
if warninginfo=0 then cursym:=9767 else begin cursym:=9759;
eqtb[9759].rh:=warninginfo;end;end;4,5:begin print(631);
if scannerstatus=5 then slowprint(hash[warninginfo].rh)else
printvariablename(warninginfo);cursym:=9765;end;6:begin print(632);
slowprint(hash[warninginfo].rh);print(633);helpline[3]:=634;
cursym:=9764;end;{:664}end;inserror;end{:663}
else begin begin if interaction=3 then;printnl(261);print(615);end;
printint(warninginfo);begin helpptr:=3;helpline[2]:=616;
helpline[1]:=617;helpline[0]:=618;end;if cursym=0 then helpline[2]:=619;
cursym:=9766;inserror;end;deletionsallowed:=true;
checkoutervalidity:=false;end;end;{:661}{666:}procedure firmuptheline;
forward;{:666}{667:}procedure getnext;label 20,10,40,25,85,86,87,30;
var k:0..bufsize;c:ASCIIcode;class:ASCIIcode;n,f:integer;
begin 20:cursym:=0;if(curinput.indexfield<=15)then{669:}
begin 25:c:=buffer[curinput.locfield];incr(curinput.locfield);
class:=charclass[c];case class of 0:goto 85;
1:begin class:=charclass[buffer[curinput.locfield]];
if class>1 then goto 25 else if class<1 then begin n:=0;goto 86;end;end;
2:goto 25;3:begin{679:}if curinput.namefield>2 then{681:}
begin incr(line);first:=curinput.startfield;
if not forceeof then begin if inputln(inputfile[curinput.indexfield],
true)then firmuptheline else forceeof:=true;end;
if forceeof then begin printchar(41);decr(openparens);flush(stdout);
forceeof:=false;endfilereading;
if checkoutervalidity then goto 20 else goto 20;end;
buffer[curinput.limitfield]:=37;first:=curinput.limitfield+1;
curinput.locfield:=curinput.startfield;end{:681}
else begin if inputptr>0 then begin endfilereading;goto 20;end;
if selector<2 then openlogfile;
if interaction>1 then begin if curinput.limitfield=curinput.startfield
then printnl(649);println;first:=curinput.startfield;begin;print(42);
terminput;end;curinput.limitfield:=last;buffer[curinput.limitfield]:=37;
first:=curinput.limitfield+1;curinput.locfield:=curinput.startfield;
end else fatalerror(650);end{:679};
begin if interrupt<>0 then pauseforinstructions;end;goto 25;end;4:{671:}
begin if buffer[curinput.locfield]=34 then curmod:=283 else begin k:=
curinput.locfield;buffer[curinput.limitfield+1]:=34;
repeat incr(curinput.locfield);until buffer[curinput.locfield]=34;
if curinput.locfield>curinput.limitfield then{672:}
begin curinput.locfield:=curinput.limitfield;
begin if interaction=3 then;printnl(261);print(642);end;
begin helpptr:=3;helpline[2]:=643;helpline[1]:=644;helpline[0]:=645;end;
deletionsallowed:=false;error;deletionsallowed:=true;goto 20;end{:672};
if curinput.locfield=k+1 then curmod:=buffer[k]else begin begin if
poolptr+curinput.locfield-k>maxpoolptr then begin if poolptr+curinput.
locfield-k>poolsize then overflow(257,poolsize-initpoolptr);
maxpoolptr:=poolptr+curinput.locfield-k;end;end;
repeat begin strpool[poolptr]:=buffer[k];incr(poolptr);end;incr(k);
until k=curinput.locfield;curmod:=makestring;end;end;
incr(curinput.locfield);curcmd:=39;goto 10;end{:671};
5,6,7,8:begin k:=curinput.locfield-1;goto 40;end;20:{670:}
begin begin if interaction=3 then;printnl(261);print(639);end;
begin helpptr:=2;helpline[1]:=640;helpline[0]:=641;end;
deletionsallowed:=false;error;deletionsallowed:=true;goto 20;end{:670};
others:end;k:=curinput.locfield-1;
while charclass[buffer[curinput.locfield]]=class do incr(curinput.
locfield);goto 40;85:{673:}n:=c-48;
while charclass[buffer[curinput.locfield]]=0 do begin if n<4096 then n:=
10*n+buffer[curinput.locfield]-48;incr(curinput.locfield);end;
if buffer[curinput.locfield]=46 then if charclass[buffer[curinput.
locfield+1]]=0 then goto 30;f:=0;goto 87;
30:incr(curinput.locfield){:673};86:{674:}k:=0;
repeat if k<17 then begin dig[k]:=buffer[curinput.locfield]-48;incr(k);
end;incr(curinput.locfield);
until charclass[buffer[curinput.locfield]]<>0;f:=rounddecimals(k);
if f=65536 then begin incr(n);f:=0;end{:674};87:{675:}
if n<4096 then curmod:=n*65536+f else begin begin if interaction=3 then;
printnl(261);print(646);end;begin helpptr:=2;helpline[1]:=647;
helpline[0]:=648;end;deletionsallowed:=false;error;
deletionsallowed:=true;curmod:=268435455;end;curcmd:=42;goto 10{:675};
40:cursym:=idlookup(k,curinput.locfield-k);end{:669}else{676:}
if curinput.locfield>=himemmin then begin cursym:=mem[curinput.locfield]
.hh.lh;curinput.locfield:=mem[curinput.locfield].hh.rh;
if cursym>=9770 then if cursym>=9920 then{677:}
begin if cursym>=10070 then cursym:=cursym-150;
begintokenlist(paramstack[curinput.limitfield+cursym-(9920)],18);
goto 20;end{:677}else begin curcmd:=38;
curmod:=paramstack[curinput.limitfield+cursym-(9770)];cursym:=0;goto 10;
end;end else if curinput.locfield>0 then{678:}
begin if mem[curinput.locfield].hh.b1=12 then begin curmod:=mem[curinput
.locfield+1].int;
if mem[curinput.locfield].hh.b0=16 then curcmd:=42 else begin curcmd:=39
;begin if strref[curmod]<127 then incr(strref[curmod]);end;end;
end else begin curmod:=curinput.locfield;curcmd:=38;end;
curinput.locfield:=mem[curinput.locfield].hh.rh;goto 10;end{:678}
else begin endtokenlist;goto 20;end{:676};{668:}curcmd:=eqtb[cursym].lh;
curmod:=eqtb[cursym].rh;
if curcmd>=86 then if checkoutervalidity then curcmd:=curcmd-86 else
goto 20{:668};10:end;{:667}{682:}procedure firmuptheline;
var k:0..bufsize;begin curinput.limitfield:=last;
if internal[31]>0 then if interaction>1 then begin;println;
if curinput.startfield<curinput.limitfield then for k:=curinput.
startfield to curinput.limitfield-1 do print(buffer[k]);
first:=curinput.limitfield;begin;print(651);terminput;end;
if last>first then begin for k:=first to last-1 do buffer[k+curinput.
startfield-first]:=buffer[k];
curinput.limitfield:=curinput.startfield+last-first;end;end;end;{:682}
{685:}function scantoks(terminator:commandcode;
substlist,tailend:halfword;suffixcount:smallnumber):halfword;
label 30,40;var p:halfword;q:halfword;balance:integer;begin p:=memtop-2;
balance:=1;mem[memtop-2].hh.rh:=0;while true do begin getnext;
if cursym>0 then begin{686:}begin q:=substlist;
while q<>0 do begin if mem[q].hh.lh=cursym then begin cursym:=mem[q+1].
int;curcmd:=7;goto 40;end;q:=mem[q].hh.rh;end;40:end{:686};
if curcmd=terminator then{687:}
if curmod>0 then incr(balance)else begin decr(balance);
if balance=0 then goto 30;end{:687}else if curcmd=61 then{690:}
begin if curmod=0 then getnext else if curmod<=suffixcount then cursym:=
9919+curmod;end{:690};end;mem[p].hh.rh:=curtok;p:=mem[p].hh.rh;end;
30:mem[p].hh.rh:=tailend;flushnodelist(substlist);
scantoks:=mem[memtop-2].hh.rh;end;{:685}{691:}procedure getsymbol;
label 20;begin 20:getnext;
if(cursym=0)or(cursym>9757)then begin begin if interaction=3 then;
printnl(261);print(663);end;begin helpptr:=3;helpline[2]:=664;
helpline[1]:=665;helpline[0]:=666;end;
if cursym>0 then helpline[2]:=667 else if curcmd=39 then begin if strref
[curmod]<127 then if strref[curmod]>1 then decr(strref[curmod])else
flushstring(curmod);end;cursym:=9757;inserror;goto 20;end;end;{:691}
{692:}procedure getclearsymbol;begin getsymbol;
clearsymbol(cursym,false);end;{:692}{693:}procedure checkequals;
begin if curcmd<>51 then if curcmd<>77 then begin missingerr(61);
begin helpptr:=5;helpline[4]:=668;helpline[3]:=669;helpline[2]:=670;
helpline[1]:=671;helpline[0]:=672;end;backerror;end;end;{:693}{694:}
procedure makeopdef;var m:commandcode;p,q,r:halfword;begin m:=curmod;
getsymbol;q:=getnode(2);mem[q].hh.lh:=cursym;mem[q+1].int:=9770;
getclearsymbol;warninginfo:=cursym;getsymbol;p:=getnode(2);
mem[p].hh.lh:=cursym;mem[p+1].int:=9771;mem[p].hh.rh:=q;getnext;
checkequals;scannerstatus:=5;q:=getavail;mem[q].hh.lh:=0;r:=getavail;
mem[q].hh.rh:=r;mem[r].hh.lh:=0;mem[r].hh.rh:=scantoks(16,p,0,0);
scannerstatus:=0;eqtb[warninginfo].lh:=m;eqtb[warninginfo].rh:=q;
getxnext;end;{:694}{697:}{1032:}
procedure checkdelimiter(ldelim,rdelim:halfword);label 10;
begin if curcmd=62 then if curmod=ldelim then goto 10;
if cursym<>rdelim then begin missingerr(hash[rdelim].rh);
begin helpptr:=2;helpline[1]:=919;helpline[0]:=920;end;backerror;
end else begin begin if interaction=3 then;printnl(261);print(921);end;
slowprint(hash[rdelim].rh);print(922);begin helpptr:=3;helpline[2]:=923;
helpline[1]:=924;helpline[0]:=925;end;error;end;10:end;{:1032}{1011:}
function scandeclaredvariable:halfword;label 30;var x:halfword;
h,t:halfword;l:halfword;begin getsymbol;x:=cursym;
if curcmd<>41 then clearsymbol(x,false);h:=getavail;mem[h].hh.lh:=x;
t:=h;while true do begin getxnext;if cursym=0 then goto 30;
if curcmd<>41 then if curcmd<>40 then if curcmd=63 then{1012:}
begin l:=cursym;getxnext;if curcmd<>64 then begin backinput;cursym:=l;
curcmd:=63;goto 30;end else cursym:=0;end{:1012}else goto 30;
mem[t].hh.rh:=getavail;t:=mem[t].hh.rh;mem[t].hh.lh:=cursym;end;
30:if eqtb[x].lh<>41 then clearsymbol(x,false);
if eqtb[x].rh=0 then newroot(x);scandeclaredvariable:=h;end;{:1011}
procedure scandef;var m:1..2;n:0..3;k:0..150;c:0..7;r:halfword;
q:halfword;p:halfword;base:halfword;ldelim,rdelim:halfword;
begin m:=curmod;c:=0;mem[memtop-2].hh.rh:=0;q:=getavail;mem[q].hh.lh:=0;
r:=0;{700:}if m=1 then begin getclearsymbol;warninginfo:=cursym;getnext;
scannerstatus:=5;n:=0;eqtb[warninginfo].lh:=10;eqtb[warninginfo].rh:=q;
end else begin p:=scandeclaredvariable;
flushvariable(eqtb[mem[p].hh.lh].rh,mem[p].hh.rh,true);
warninginfo:=findvariable(p);flushlist(p);if warninginfo=0 then{701:}
begin begin if interaction=3 then;printnl(261);print(679);end;
begin helpptr:=2;helpline[1]:=680;helpline[0]:=681;end;error;
warninginfo:=21;end{:701};scannerstatus:=4;n:=2;
if curcmd=61 then if curmod=3 then begin n:=3;getnext;end;
mem[warninginfo].hh.b0:=20+n;mem[warninginfo+1].int:=q;end{:700};k:=n;
if curcmd=31 then{703:}repeat ldelim:=cursym;rdelim:=curmod;getnext;
if(curcmd=56)and(curmod>=9770)then base:=curmod else begin begin if
interaction=3 then;printnl(261);print(682);end;begin helpptr:=1;
helpline[0]:=683;end;backerror;base:=9770;end;{704:}
repeat mem[q].hh.rh:=getavail;q:=mem[q].hh.rh;mem[q].hh.lh:=base+k;
getsymbol;p:=getnode(2);mem[p+1].int:=base+k;mem[p].hh.lh:=cursym;
if k=150 then overflow(684,150);incr(k);mem[p].hh.rh:=r;r:=p;getnext;
until curcmd<>82{:704};checkdelimiter(ldelim,rdelim);getnext;
until curcmd<>31{:703};if curcmd=56 then{705:}begin p:=getnode(2);
if curmod<9770 then begin c:=curmod;mem[p+1].int:=9770+k;
end else begin mem[p+1].int:=curmod+k;
if curmod=9770 then c:=4 else if curmod=9920 then c:=6 else c:=7;end;
if k=150 then overflow(684,150);incr(k);getsymbol;mem[p].hh.lh:=cursym;
mem[p].hh.rh:=r;r:=p;getnext;if c=4 then if curcmd=69 then begin c:=5;
p:=getnode(2);if k=150 then overflow(684,150);mem[p+1].int:=9770+k;
getsymbol;mem[p].hh.lh:=cursym;mem[p].hh.rh:=r;r:=p;getnext;end;
end{:705};checkequals;p:=getavail;mem[p].hh.lh:=c;mem[q].hh.rh:=p;{698:}
if m=1 then mem[p].hh.rh:=scantoks(16,r,0,n)else begin q:=getavail;
mem[q].hh.lh:=bgloc;mem[p].hh.rh:=q;p:=getavail;mem[p].hh.lh:=egloc;
mem[q].hh.rh:=scantoks(16,r,p,n);end;
if warninginfo=21 then flushtokenlist(mem[22].int){:698};
scannerstatus:=0;getxnext;end;{:697}{706:}procedure scanprimary;forward;
procedure scansecondary;forward;procedure scantertiary;forward;
procedure scanexpression;forward;procedure scansuffix;forward;{720:}
{722:}procedure printmacroname(a,n:halfword);var p,q:halfword;
begin if n<>0 then slowprint(hash[n].rh)else begin p:=mem[a].hh.lh;
if p=0 then slowprint(hash[mem[mem[mem[a].hh.rh].hh.lh].hh.lh].rh)else
begin q:=p;while mem[q].hh.rh<>0 do q:=mem[q].hh.rh;
mem[q].hh.rh:=mem[mem[a].hh.rh].hh.lh;showtokenlist(p,0,1000,0);
mem[q].hh.rh:=0;end;end;end;{:722}{723:}procedure printarg(q:halfword;
n:integer;b:halfword);
begin if mem[q].hh.rh=1 then printnl(497)else if(b<10070)and(b<>7)then
printnl(498)else printnl(499);printint(n);print(700);
if mem[q].hh.rh=1 then printexp(q,1)else showtokenlist(q,0,1000,0);end;
{:723}{730:}procedure scantextarg(ldelim,rdelim:halfword);label 30;
var balance:integer;p:halfword;begin warninginfo:=ldelim;
scannerstatus:=3;p:=memtop-2;balance:=1;mem[memtop-2].hh.rh:=0;
while true do begin getnext;if ldelim=0 then{732:}
begin if curcmd>82 then begin if balance=1 then goto 30 else if curcmd=
84 then decr(balance);end else if curcmd=32 then incr(balance);end{:732}
else{731:}
begin if curcmd=62 then begin if curmod=ldelim then begin decr(balance);
if balance=0 then goto 30;end;
end else if curcmd=31 then if curmod=rdelim then incr(balance);end{:731}
;mem[p].hh.rh:=curtok;p:=mem[p].hh.rh;end;
30:curexp:=mem[memtop-2].hh.rh;curtype:=20;scannerstatus:=0;end;{:730}
procedure macrocall(defref,arglist,macroname:halfword);label 40;
var r:halfword;p,q:halfword;n:integer;ldelim,rdelim:halfword;
tail:halfword;begin r:=mem[defref].hh.rh;incr(mem[defref].hh.lh);
if arglist=0 then n:=0 else{724:}begin n:=1;tail:=arglist;
while mem[tail].hh.rh<>0 do begin incr(n);tail:=mem[tail].hh.rh;end;
end{:724};if internal[9]>0 then{721:}begin begindiagnostic;println;
printmacroname(arglist,macroname);if n=3 then print(662);
showmacro(defref,0,100000);if arglist<>0 then begin n:=0;p:=arglist;
repeat q:=mem[p].hh.lh;printarg(q,n,0);incr(n);p:=mem[p].hh.rh;
until p=0;end;enddiagnostic(false);end{:721};{725:}curcmd:=83;
while mem[r].hh.lh>=9770 do begin{726:}
if curcmd<>82 then begin getxnext;
if curcmd<>31 then begin begin if interaction=3 then;printnl(261);
print(706);end;printmacroname(arglist,macroname);begin helpptr:=3;
helpline[2]:=707;helpline[1]:=708;helpline[0]:=709;end;
if mem[r].hh.lh>=9920 then begin curexp:=0;curtype:=20;
end else begin curexp:=0;curtype:=16;end;backerror;curcmd:=62;goto 40;
end;ldelim:=cursym;rdelim:=curmod;end;{729:}
if mem[r].hh.lh>=10070 then scantextarg(ldelim,rdelim)else begin
getxnext;if mem[r].hh.lh>=9920 then scansuffix else scanexpression;
end{:729};if curcmd<>82 then{727:}
if(curcmd<>62)or(curmod<>ldelim)then if mem[mem[r].hh.rh].hh.lh>=9770
then begin missingerr(44);begin helpptr:=3;helpline[2]:=710;
helpline[1]:=711;helpline[0]:=705;end;backerror;curcmd:=82;
end else begin missingerr(hash[rdelim].rh);begin helpptr:=2;
helpline[1]:=712;helpline[0]:=705;end;backerror;end{:727};40:{728:}
begin p:=getavail;
if curtype=20 then mem[p].hh.lh:=curexp else mem[p].hh.lh:=stashcurexp;
if internal[9]>0 then begin begindiagnostic;
printarg(mem[p].hh.lh,n,mem[r].hh.lh);enddiagnostic(false);end;
if arglist=0 then arglist:=p else mem[tail].hh.rh:=p;tail:=p;incr(n);
end{:728}{:726};r:=mem[r].hh.rh;end;
if curcmd=82 then begin begin if interaction=3 then;printnl(261);
print(701);end;printmacroname(arglist,macroname);printchar(59);
printnl(702);slowprint(hash[rdelim].rh);print(298);begin helpptr:=3;
helpline[2]:=703;helpline[1]:=704;helpline[0]:=705;end;error;end;
if mem[r].hh.lh<>0 then{733:}
begin if mem[r].hh.lh<7 then begin getxnext;
if mem[r].hh.lh<>6 then if(curcmd=51)or(curcmd=77)then getxnext;end;
case mem[r].hh.lh of 1:scanprimary;2:scansecondary;3:scantertiary;
4:scanexpression;5:{734:}begin scanexpression;p:=getavail;
mem[p].hh.lh:=stashcurexp;if internal[9]>0 then begin begindiagnostic;
printarg(mem[p].hh.lh,n,0);enddiagnostic(false);end;
if arglist=0 then arglist:=p else mem[tail].hh.rh:=p;tail:=p;incr(n);
if curcmd<>69 then begin missingerr(478);print(713);
printmacroname(arglist,macroname);begin helpptr:=1;helpline[0]:=714;end;
backerror;end;getxnext;scanprimary;end{:734};6:{735:}
begin if curcmd<>31 then ldelim:=0 else begin ldelim:=cursym;
rdelim:=curmod;getxnext;end;scansuffix;
if ldelim<>0 then begin if(curcmd<>62)or(curmod<>ldelim)then begin
missingerr(hash[rdelim].rh);begin helpptr:=2;helpline[1]:=712;
helpline[0]:=705;end;backerror;end;getxnext;end;end{:735};
7:scantextarg(0,0);end;backinput;{728:}begin p:=getavail;
if curtype=20 then mem[p].hh.lh:=curexp else mem[p].hh.lh:=stashcurexp;
if internal[9]>0 then begin begindiagnostic;
printarg(mem[p].hh.lh,n,mem[r].hh.lh);enddiagnostic(false);end;
if arglist=0 then arglist:=p else mem[tail].hh.rh:=p;tail:=p;incr(n);
end{:728};end{:733};r:=mem[r].hh.rh{:725};{736:}
while(curinput.indexfield>15)and(curinput.locfield=0)do endtokenlist;
if paramptr+n>maxparamstack then begin maxparamstack:=paramptr+n;
if maxparamstack>150 then overflow(684,150);end;
begintokenlist(defref,21);curinput.namefield:=macroname;
curinput.locfield:=r;if n>0 then begin p:=arglist;
repeat paramstack[paramptr]:=mem[p].hh.lh;incr(paramptr);
p:=mem[p].hh.rh;until p=0;flushlist(arglist);end{:736};end;{:720}
procedure getboolean;forward;procedure passtext;forward;
procedure conditional;forward;procedure startinput;forward;
procedure beginiteration;forward;procedure resumeiteration;forward;
procedure stopiteration;forward;{:706}{707:}procedure expand;
var p:halfword;k:integer;j:poolpointer;
begin if internal[7]>65536 then if curcmd<>10 then showcmdmod(curcmd,
curmod);case curcmd of 1:conditional;2:{751:}
if curmod>iflimit then if iflimit=1 then begin missingerr(58);backinput;
cursym:=9762;inserror;end else begin begin if interaction=3 then;
printnl(261);print(721);end;printcmdmod(2,curmod);begin helpptr:=1;
helpline[0]:=722;end;error;end else begin while curmod<>2 do passtext;
{745:}begin p:=condptr;ifline:=mem[p+1].int;curif:=mem[p].hh.b1;
iflimit:=mem[p].hh.b0;condptr:=mem[p].hh.rh;freenode(p,2);end{:745};
end{:751};3:{711:}if curmod>0 then forceeof:=true else startinput{:711};
4:if curmod=0 then{708:}begin begin if interaction=3 then;printnl(261);
print(685);end;begin helpptr:=2;helpline[1]:=686;helpline[0]:=687;end;
error;end{:708}else beginiteration;5:{712:}
begin while(curinput.indexfield>15)and(curinput.locfield=0)do
endtokenlist;if loopptr=0 then begin begin if interaction=3 then;
printnl(261);print(689);end;begin helpptr:=2;helpline[1]:=690;
helpline[0]:=691;end;error;end else resumeiteration;end{:712};6:{713:}
begin getboolean;if internal[7]>65536 then showcmdmod(33,curexp);
if curexp=30 then if loopptr=0 then begin begin if interaction=3 then;
printnl(261);print(692);end;begin helpptr:=1;helpline[0]:=693;end;
if curcmd=83 then error else backerror;end else{714:}begin p:=0;
repeat if(curinput.indexfield<=15)then endfilereading else begin if
curinput.indexfield<=17 then p:=curinput.startfield;endtokenlist;end;
until p<>0;if p<>mem[loopptr].hh.lh then fatalerror(696);stopiteration;
end{:714}else if curcmd<>83 then begin missingerr(59);begin helpptr:=2;
helpline[1]:=694;helpline[0]:=695;end;backerror;end;end{:713};7:;
9:{715:}begin getnext;p:=curtok;getnext;
if curcmd<11 then expand else backinput;begintokenlist(p,19);end{:715};
8:{716:}begin getxnext;scanprimary;
if curtype<>4 then begin disperr(0,697);begin helpptr:=2;
helpline[1]:=698;helpline[0]:=699;end;putgetflusherror(0);
end else begin backinput;
if(strstart[curexp+1]-strstart[curexp])>0 then{717:}
begin beginfilereading;curinput.namefield:=2;
k:=first+(strstart[curexp+1]-strstart[curexp]);
if k>=maxbufstack then begin if k>=bufsize then begin maxbufstack:=
bufsize;overflow(256,bufsize);end;maxbufstack:=k+1;end;
j:=strstart[curexp];curinput.limitfield:=k;
while first<curinput.limitfield do begin buffer[first]:=strpool[j];
incr(j);incr(first);end;buffer[curinput.limitfield]:=37;
first:=curinput.limitfield+1;curinput.locfield:=curinput.startfield;
flushcurexp(0);end{:717};end;end{:716};10:macrocall(curmod,0,cursym);
end;end;{:707}{718:}procedure getxnext;var saveexp:halfword;
begin getnext;if curcmd<11 then begin saveexp:=stashcurexp;
repeat if curcmd=10 then macrocall(curmod,0,cursym)else expand;getnext;
until curcmd>=11;unstashcurexp(saveexp);end;end;{:718}{737:}
procedure stackargument(p:halfword);
begin if paramptr=maxparamstack then begin incr(maxparamstack);
if maxparamstack>150 then overflow(684,150);end;paramstack[paramptr]:=p;
incr(paramptr);end;{:737}{742:}procedure passtext;label 30;
var l:integer;begin scannerstatus:=1;l:=0;warninginfo:=line;
while true do begin getnext;
if curcmd<=2 then if curcmd<2 then incr(l)else begin if l=0 then goto 30
;if curmod=2 then decr(l);end else{743:}
if curcmd=39 then begin if strref[curmod]<127 then if strref[curmod]>1
then decr(strref[curmod])else flushstring(curmod);end{:743};end;
30:scannerstatus:=0;end;{:742}{746:}
procedure changeiflimit(l:smallnumber;p:halfword);label 10;
var q:halfword;begin if p=condptr then iflimit:=l else begin q:=condptr;
while true do begin if q=0 then confusion(715);
if mem[q].hh.rh=p then begin mem[q].hh.b0:=l;goto 10;end;
q:=mem[q].hh.rh;end;end;10:end;{:746}{747:}procedure checkcolon;
begin if curcmd<>81 then begin missingerr(58);begin helpptr:=2;
helpline[1]:=718;helpline[0]:=695;end;backerror;end;end;{:747}{748:}
procedure conditional;label 10,30,21,40;var savecondptr:halfword;
newiflimit:2..4;p:halfword;begin{744:}begin p:=getnode(2);
mem[p].hh.rh:=condptr;mem[p].hh.b0:=iflimit;mem[p].hh.b1:=curif;
mem[p+1].int:=ifline;condptr:=p;iflimit:=1;ifline:=line;curif:=1;
end{:744};savecondptr:=condptr;21:getboolean;newiflimit:=4;
if internal[7]>65536 then{750:}begin begindiagnostic;
if curexp=30 then print(719)else print(720);enddiagnostic(false);
end{:750};40:checkcolon;
if curexp=30 then begin changeiflimit(newiflimit,savecondptr);goto 10;
end;{749:}while true do begin passtext;
if condptr=savecondptr then goto 30 else if curmod=2 then{745:}
begin p:=condptr;ifline:=mem[p+1].int;curif:=mem[p].hh.b1;
iflimit:=mem[p].hh.b0;condptr:=mem[p].hh.rh;freenode(p,2);end{:745};
end{:749};30:curif:=curmod;ifline:=line;if curmod=2 then{745:}
begin p:=condptr;ifline:=mem[p+1].int;curif:=mem[p].hh.b1;
iflimit:=mem[p].hh.b0;condptr:=mem[p].hh.rh;freenode(p,2);end{:745}
else if curmod=4 then goto 21 else begin curexp:=30;newiflimit:=2;
getxnext;goto 40;end;10:end;{:748}{754:}procedure badfor(s:strnumber);
begin disperr(0,723);print(s);print(305);begin helpptr:=4;
helpline[3]:=724;helpline[2]:=725;helpline[1]:=726;helpline[0]:=307;end;
putgetflusherror(0);end;{:754}{755:}procedure beginiteration;
label 22,30,40;var m:halfword;n:halfword;p,q,s,pp:halfword;
begin m:=curmod;n:=cursym;s:=getnode(2);
if m=1 then begin mem[s+1].hh.lh:=1;p:=0;getxnext;goto 40;end;getsymbol;
p:=getnode(2);mem[p].hh.lh:=cursym;mem[p+1].int:=m;getxnext;
if(curcmd<>51)and(curcmd<>77)then begin missingerr(61);begin helpptr:=3;
helpline[2]:=727;helpline[1]:=670;helpline[0]:=728;end;backerror;end;
{764:}mem[s+1].hh.lh:=0;q:=s+1;mem[q].hh.rh:=0;repeat getxnext;
if m<>9770 then scansuffix else begin if curcmd>=81 then if curcmd<=82
then goto 22;scanexpression;if curcmd=74 then if q=s+1 then{765:}
begin if curtype<>16 then badfor(734);pp:=getnode(4);
mem[pp+1].int:=curexp;getxnext;scanexpression;
if curtype<>16 then badfor(735);mem[pp+2].int:=curexp;
if curcmd<>75 then begin missingerr(489);begin helpptr:=2;
helpline[1]:=736;helpline[0]:=737;end;backerror;end;getxnext;
scanexpression;if curtype<>16 then badfor(738);mem[pp+3].int:=curexp;
mem[s+1].hh.lh:=pp;goto 30;end{:765};curexp:=stashcurexp;end;
mem[q].hh.rh:=getavail;q:=mem[q].hh.rh;mem[q].hh.lh:=curexp;curtype:=1;
22:until curcmd<>82;30:{:764};40:{756:}
if curcmd<>81 then begin missingerr(58);begin helpptr:=3;
helpline[2]:=729;helpline[1]:=730;helpline[0]:=731;end;backerror;
end{:756};{758:}q:=getavail;mem[q].hh.lh:=9758;scannerstatus:=6;
warninginfo:=n;mem[s].hh.lh:=scantoks(4,p,q,0);scannerstatus:=0;
mem[s].hh.rh:=loopptr;loopptr:=s{:758};resumeiteration;end;{:755}{760:}
procedure resumeiteration;label 45,10;var p,q:halfword;
begin p:=mem[loopptr+1].hh.lh;if p>1 then begin curexp:=mem[p+1].int;
if{761:}
((mem[p+2].int>0)and(curexp>mem[p+3].int))or((mem[p+2].int<0)and(curexp<
mem[p+3].int)){:761}then goto 45;curtype:=16;q:=stashcurexp;
mem[p+1].int:=curexp+mem[p+2].int;
end else if p<1 then begin p:=mem[loopptr+1].hh.rh;if p=0 then goto 45;
mem[loopptr+1].hh.rh:=mem[p].hh.rh;q:=mem[p].hh.lh;
begin mem[p].hh.rh:=avail;avail:=p;ifdef('STAT')decr(dynused);
endif('STAT')end;end else begin begintokenlist(mem[loopptr].hh.lh,16);
goto 10;end;begintokenlist(mem[loopptr].hh.lh,17);stackargument(q);
if internal[7]>65536 then{762:}begin begindiagnostic;printnl(733);
if(q<>0)and(mem[q].hh.rh=1)then printexp(q,1)else showtokenlist(q,0,50,0
);printchar(125);enddiagnostic(false);end{:762};goto 10;
45:stopiteration;10:end;{:760}{763:}procedure stopiteration;
var p,q:halfword;begin p:=mem[loopptr+1].hh.lh;
if p>1 then freenode(p,4)else if p<1 then begin q:=mem[loopptr+1].hh.rh;
while q<>0 do begin p:=mem[q].hh.lh;
if p<>0 then if mem[p].hh.rh=1 then begin recyclevalue(p);freenode(p,2);
end else flushtokenlist(p);p:=q;q:=mem[q].hh.rh;
begin mem[p].hh.rh:=avail;avail:=p;ifdef('STAT')decr(dynused);
endif('STAT')end;end;end;p:=loopptr;loopptr:=mem[p].hh.rh;
flushtokenlist(mem[p].hh.lh);freenode(p,2);end;{:763}{770:}
procedure beginname;begin areadelimiter:=0;extdelimiter:=0;end;{:770}
{771:}function morename(c:ASCIIcode):boolean;
begin if(c=32)or(c=9)then morename:=false else begin if(c=47)then begin
areadelimiter:=poolptr;extdelimiter:=0;
end else if(c=46)and(extdelimiter=0)then extdelimiter:=poolptr;
begin if poolptr+1>maxpoolptr then begin if poolptr+1>poolsize then
overflow(257,poolsize-initpoolptr);maxpoolptr:=poolptr+1;end;end;
begin strpool[poolptr]:=c;incr(poolptr);end;morename:=true;end;end;
{:771}{772:}procedure endname;
begin if strptr+3>maxstrptr then begin if strptr+3>maxstrings then
overflow(258,maxstrings-initstrptr);maxstrptr:=strptr+3;end;
if areadelimiter=0 then curarea:=283 else begin curarea:=strptr;
incr(strptr);strstart[strptr]:=areadelimiter+1;end;
if extdelimiter=0 then begin curext:=283;curname:=makestring;
end else begin curname:=strptr;incr(strptr);
strstart[strptr]:=extdelimiter;curext:=makestring;end;end;{:772}{774:}
procedure packfilename(n,a,e:strnumber);var k:integer;c:ASCIIcode;
j:poolpointer;begin k:=0;
for j:=strstart[a]to strstart[a+1]-1 do begin c:=strpool[j];incr(k);
if k<=PATHMAX then nameoffile[k]:=xchr[c];end;
for j:=strstart[n]to strstart[n+1]-1 do begin c:=strpool[j];incr(k);
if k<=PATHMAX then nameoffile[k]:=xchr[c];end;
for j:=strstart[e]to strstart[e+1]-1 do begin c:=strpool[j];incr(k);
if k<=PATHMAX then nameoffile[k]:=xchr[c];end;
if k<PATHMAX then namelength:=k else namelength:=PATHMAX-1;
for k:=namelength+1 to PATHMAX do nameoffile[k]:=' ';end;{:774}{778:}
procedure packbufferedname(n:smallnumber;a,b:integer);var k:integer;
c:ASCIIcode;j:integer;begin if n+b-a+6>PATHMAX then b:=a+PATHMAX-n-6;
k:=0;for j:=1 to n do begin c:=xord[MFbasedefault[j]];incr(k);
if k<=PATHMAX then nameoffile[k]:=xchr[c];end;
for j:=a to b do begin c:=buffer[j];incr(k);
if k<=PATHMAX then nameoffile[k]:=xchr[c];end;
for j:=basedefaultlength-4 to basedefaultlength do begin c:=xord[
MFbasedefault[j]];incr(k);if k<=PATHMAX then nameoffile[k]:=xchr[c];end;
if k<PATHMAX then namelength:=k else namelength:=PATHMAX-1;
for k:=namelength+1 to PATHMAX do nameoffile[k]:=' ';end;{:778}{780:}
function makenamestring:strnumber;var k:1..PATHMAX;
begin if(poolptr+namelength>poolsize)or(strptr=maxstrings)then
makenamestring:=63 else begin for k:=1 to namelength do begin strpool[
poolptr]:=xord[nameoffile[k]];incr(poolptr);end;
makenamestring:=makestring;end;end;
function amakenamestring(var f:alphafile):strnumber;
begin amakenamestring:=makenamestring;end;
function bmakenamestring(var f:bytefile):strnumber;
begin bmakenamestring:=makenamestring;end;
function wmakenamestring(var f:wordfile):strnumber;
begin wmakenamestring:=makenamestring;end;{:780}{781:}
procedure scanfilename;label 30;begin beginname;
while(buffer[curinput.locfield]=32)or(buffer[curinput.locfield]=9)do
incr(curinput.locfield);
while true do begin if(buffer[curinput.locfield]=59)or(buffer[curinput.
locfield]=37)then goto 30;
if not morename(buffer[curinput.locfield])then goto 30;
incr(curinput.locfield);end;30:endname;end;{:781}{784:}
procedure packjobname(s:strnumber);begin curarea:=283;curext:=s;
curname:=jobname;packfilename(curname,curarea,curext);end;{:784}{786:}
procedure promptfilename(s,e:strnumber);label 30;var k:0..bufsize;
begin if interaction=2 then;if s=740 then begin if interaction=3 then;
printnl(261);print(741);end else begin if interaction=3 then;
printnl(261);print(742);end;printfilename(curname,curarea,curext);
print(743);if e=744 then showcontext;printnl(745);print(s);
if interaction<2 then fatalerror(746);;begin;print(747);terminput;end;
{787:}begin beginname;k:=first;
while((buffer[k]=32)or(buffer[k]=9))and(k<last)do incr(k);
while true do begin if k=last then goto 30;
if not morename(buffer[k])then goto 30;incr(k);end;30:endname;end{:787};
if curext=283 then curext:=e;packfilename(curname,curarea,curext);end;
{:786}{788:}procedure openlogfile;var oldsetting:0..5;k:0..bufsize;
l:0..bufsize;m:integer;months:ccharpointer;begin oldsetting:=selector;
if jobname=0 then jobname:=748;packjobname(749);
while not aopenout(logfile)do{789:}begin selector:=1;
promptfilename(751,749);end{:789};
texmflogname:=amakenamestring(logfile);selector:=2;logopened:=true;
{790:}begin write(logfile,'This is METAFONT, C Version 2.71');
slowprint(baseident);print(752);printint(roundunscaled(internal[16]));
printchar(32);months:=' JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC';
m:=roundunscaled(internal[15]);
for k:=3*m-2 to 3*m do write(logfile,months[k]);printchar(32);
printint(roundunscaled(internal[14]));printchar(32);
m:=roundunscaled(internal[17]);printdd(m div 60);printchar(58);
printdd(m mod 60);end{:790};inputstack[inputptr]:=curinput;printnl(750);
l:=inputstack[0].limitfield-1;for k:=1 to l do print(buffer[k]);println;
selector:=oldsetting+2;end;{:788}{793:}procedure startinput;label 30;
begin{795:}
while(curinput.indexfield>15)and(curinput.locfield=0)do endtokenlist;
if(curinput.indexfield>15)then begin begin if interaction=3 then;
printnl(261);print(754);end;begin helpptr:=3;helpline[2]:=755;
helpline[1]:=756;helpline[0]:=757;end;error;end;
if(curinput.indexfield<=15)then scanfilename else begin curname:=283;
curext:=283;curarea:=283;end{:795};packfilename(curname,curarea,curext);
while true do begin beginfilereading;
if(curext<>744)and(namelength+4<PATHMAX)and(not extensionirrelevantp(
nameoffile,'mf'))then begin nameoffile[namelength+1]:=46;
nameoffile[namelength+2]:=109;nameoffile[namelength+3]:=102;
namelength:=namelength+3;end;
if aopenin(inputfile[curinput.indexfield],MFINPUTPATH)then goto 30;
if curext=744 then curext:=283;packfilename(curname,curarea,curext);
if aopenin(inputfile[curinput.indexfield],MFINPUTPATH)then goto 30;
endfilereading;promptfilename(740,744);end;
30:curinput.namefield:=amakenamestring(inputfile[curinput.indexfield]);
strref[curname]:=127;if jobname=0 then begin jobname:=curname;
openlogfile;end;
if termoffset+(strstart[curinput.namefield+1]-strstart[curinput.
namefield])>maxprintline-2 then println else if(termoffset>0)or(
fileoffset>0)then printchar(32);printchar(40);incr(openparens);
slowprint(curinput.namefield);flush(stdout);{794:}begin line:=1;
if inputln(inputfile[curinput.indexfield],false)then;firmuptheline;
buffer[curinput.limitfield]:=37;first:=curinput.limitfield+1;
curinput.locfield:=curinput.startfield;end{:794};end;{:793}{824:}
procedure badexp(s:strnumber);var saveflag:0..85;
begin begin if interaction=3 then;printnl(261);print(s);end;print(767);
printcmdmod(curcmd,curmod);printchar(39);begin helpptr:=4;
helpline[3]:=768;helpline[2]:=769;helpline[1]:=770;helpline[0]:=771;end;
backinput;cursym:=0;curcmd:=42;curmod:=0;inserror;saveflag:=varflag;
varflag:=0;getxnext;varflag:=saveflag;end;{:824}{827:}
procedure stashin(p:halfword);var q:halfword;
begin mem[p].hh.b0:=curtype;
if curtype=16 then mem[p+1].int:=curexp else begin if curtype=19 then{
829:}begin q:=singledependency(curexp);
if q=depfinal then begin mem[p].hh.b0:=16;mem[p+1].int:=0;freenode(q,2);
end else begin mem[p].hh.b0:=17;newdep(p,q);end;recyclevalue(curexp);
end{:829}else begin mem[p+1]:=mem[curexp+1];
mem[mem[p+1].hh.lh].hh.rh:=p;end;freenode(curexp,2);end;curtype:=1;end;
{:827}{848:}procedure backexpr;var p:halfword;begin p:=stashcurexp;
mem[p].hh.rh:=0;begintokenlist(p,19);end;{:848}{849:}
procedure badsubscript;begin disperr(0,783);begin helpptr:=3;
helpline[2]:=784;helpline[1]:=785;helpline[0]:=786;end;flusherror(0);
end;{:849}{851:}procedure obliterated(q:halfword);
begin begin if interaction=3 then;printnl(261);print(787);end;
showtokenlist(q,0,1000,0);print(788);begin helpptr:=5;helpline[4]:=789;
helpline[3]:=790;helpline[2]:=791;helpline[1]:=792;helpline[0]:=793;end;
end;{:851}{863:}procedure binarymac(p,c,n:halfword);var q,r:halfword;
begin q:=getavail;r:=getavail;mem[q].hh.rh:=r;mem[q].hh.lh:=p;
mem[r].hh.lh:=stashcurexp;macrocall(c,q,n);end;{:863}{865:}
procedure materializepen;label 50;
var aminusb,aplusb,majoraxis,minoraxis:scaled;theta:angle;p:halfword;
q:halfword;begin q:=curexp;
if mem[q].hh.b0=0 then begin begin if interaction=3 then;printnl(261);
print(803);end;begin helpptr:=2;helpline[1]:=804;helpline[0]:=574;end;
putgeterror;curexp:=3;goto 50;end else if mem[q].hh.b0=4 then{866:}
begin tx:=mem[q+1].int;ty:=mem[q+2].int;txx:=mem[q+3].int-tx;
tyx:=mem[q+4].int-ty;txy:=mem[q+5].int-tx;tyy:=mem[q+6].int-ty;
aminusb:=pythadd(txx-tyy,tyx+txy);aplusb:=pythadd(txx+tyy,tyx-txy);
majoraxis:=(aminusb+aplusb)div 2;minoraxis:=(abs(aplusb-aminusb))div 2;
if majoraxis=minoraxis then theta:=0 else theta:=(narg(txx-tyy,tyx+txy)+
narg(txx+tyy,tyx-txy))div 2;freenode(q,7);
q:=makeellipse(majoraxis,minoraxis,theta);if(tx<>0)or(ty<>0)then{867:}
begin p:=q;repeat mem[p+1].int:=mem[p+1].int+tx;
mem[p+2].int:=mem[p+2].int+ty;p:=mem[p].hh.rh;until p=q;end{:867};
end{:866};curexp:=makepen(q);50:tossknotlist(q);curtype:=6;end;{:865}
{871:}{872:}procedure knownpair;var p:halfword;
begin if curtype<>14 then begin disperr(0,806);begin helpptr:=5;
helpline[4]:=807;helpline[3]:=808;helpline[2]:=809;helpline[1]:=810;
helpline[0]:=811;end;putgetflusherror(0);curx:=0;cury:=0;
end else begin p:=mem[curexp+1].int;{873:}
if mem[p].hh.b0=16 then curx:=mem[p+1].int else begin disperr(p,812);
begin helpptr:=5;helpline[4]:=813;helpline[3]:=808;helpline[2]:=809;
helpline[1]:=810;helpline[0]:=811;end;putgeterror;recyclevalue(p);
curx:=0;end;
if mem[p+2].hh.b0=16 then cury:=mem[p+3].int else begin disperr(p+2,814)
;begin helpptr:=5;helpline[4]:=815;helpline[3]:=808;helpline[2]:=809;
helpline[1]:=810;helpline[0]:=811;end;putgeterror;recyclevalue(p+2);
cury:=0;end{:873};flushcurexp(0);end;end;{:872}
function newknot:halfword;var q:halfword;begin q:=getnode(7);
mem[q].hh.b0:=0;mem[q].hh.b1:=0;mem[q].hh.rh:=q;knownpair;
mem[q+1].int:=curx;mem[q+2].int:=cury;newknot:=q;end;{:871}{875:}
function scandirection:smallnumber;var t:2..4;x:scaled;begin getxnext;
if curcmd=60 then{876:}begin getxnext;scanexpression;
if(curtype<>16)or(curexp<0)then begin disperr(0,818);begin helpptr:=1;
helpline[0]:=819;end;putgetflusherror(65536);end;t:=3;end{:876}
else{877:}begin scanexpression;if curtype>14 then{878:}
begin if curtype<>16 then begin disperr(0,812);begin helpptr:=5;
helpline[4]:=813;helpline[3]:=808;helpline[2]:=809;helpline[1]:=810;
helpline[0]:=811;end;putgetflusherror(0);end;x:=curexp;
if curcmd<>82 then begin missingerr(44);begin helpptr:=2;
helpline[1]:=820;helpline[0]:=821;end;backerror;end;getxnext;
scanexpression;if curtype<>16 then begin disperr(0,814);
begin helpptr:=5;helpline[4]:=815;helpline[3]:=808;helpline[2]:=809;
helpline[1]:=810;helpline[0]:=811;end;putgetflusherror(0);end;
cury:=curexp;curx:=x;end{:878}else knownpair;
if(curx=0)and(cury=0)then t:=4 else begin t:=2;curexp:=narg(curx,cury);
end;end{:877};if curcmd<>65 then begin missingerr(125);begin helpptr:=3;
helpline[2]:=816;helpline[1]:=817;helpline[0]:=695;end;backerror;end;
getxnext;scandirection:=t;end;{:875}{895:}
procedure donullary(c:quarterword);var k:integer;
begin begin if aritherror then cleararith;end;
if internal[7]>131072 then showcmdmod(33,c);
case c of 30,31:begin curtype:=2;curexp:=c;end;32:begin curtype:=11;
curexp:=getnode(6);initedges(curexp);end;33:begin curtype:=6;curexp:=3;
end;37:begin curtype:=16;curexp:=normrand;end;36:{896:}begin curtype:=8;
curexp:=getnode(7);mem[curexp].hh.b0:=4;mem[curexp].hh.b1:=4;
mem[curexp].hh.rh:=curexp;mem[curexp+1].int:=0;mem[curexp+2].int:=0;
mem[curexp+3].int:=65536;mem[curexp+4].int:=0;mem[curexp+5].int:=0;
mem[curexp+6].int:=65536;end{:896};
34:begin if jobname=0 then openlogfile;curtype:=4;curexp:=jobname;end;
35:{897:}begin if interaction<=1 then fatalerror(832);beginfilereading;
curinput.namefield:=1;begin;print(283);terminput;end;
begin if poolptr+last-curinput.startfield>maxpoolptr then begin if
poolptr+last-curinput.startfield>poolsize then overflow(257,poolsize-
initpoolptr);maxpoolptr:=poolptr+last-curinput.startfield;end;end;
for k:=curinput.startfield to last-1 do begin strpool[poolptr]:=buffer[k
];incr(poolptr);end;endfilereading;curtype:=4;curexp:=makestring;
end{:897};end;begin if aritherror then cleararith;end;end;{:895}{898:}
{899:}function nicepair(p:integer;t:quarterword):boolean;label 10;
begin if t=14 then begin p:=mem[p+1].int;
if mem[p].hh.b0=16 then if mem[p+2].hh.b0=16 then begin nicepair:=true;
goto 10;end;end;nicepair:=false;10:end;{:899}{900:}
procedure printknownorunknowntype(t:smallnumber;v:integer);
begin printchar(40);
if t<17 then if t<>14 then printtype(t)else if nicepair(v,14)then print(
335)else print(833)else print(834);printchar(41);end;{:900}{901:}
procedure badunary(c:quarterword);begin disperr(0,835);printop(c);
printknownorunknowntype(curtype,curexp);begin helpptr:=3;
helpline[2]:=836;helpline[1]:=837;helpline[0]:=838;end;putgeterror;end;
{:901}{904:}procedure negatedeplist(p:halfword);label 10;
begin while true do begin mem[p+1].int:=-mem[p+1].int;
if mem[p].hh.lh=0 then goto 10;p:=mem[p].hh.rh;end;10:end;{:904}{908:}
procedure pairtopath;begin curexp:=newknot;curtype:=9;end;{:908}{910:}
procedure takepart(c:quarterword);var p:halfword;
begin p:=mem[curexp+1].int;mem[18].int:=p;mem[17].hh.b0:=curtype;
mem[p].hh.rh:=17;freenode(curexp,2);makeexpcopy(p+2*(c-53));
recyclevalue(17);end;{:910}{913:}procedure strtonum(c:quarterword);
var n:integer;m:ASCIIcode;k:poolpointer;b:8..16;badchar:boolean;
begin if c=49 then if(strstart[curexp+1]-strstart[curexp])=0 then n:=-1
else n:=strpool[strstart[curexp]]else begin if c=47 then b:=8 else b:=16
;n:=0;badchar:=false;
for k:=strstart[curexp]to strstart[curexp+1]-1 do begin m:=strpool[k];
if(m>=48)and(m<=57)then m:=m-48 else if(m>=65)and(m<=70)then m:=m-55
else if(m>=97)and(m<=102)then m:=m-87 else begin badchar:=true;m:=0;end;
if m>=b then begin badchar:=true;m:=0;end;
if n<32768 div b then n:=n*b+m else n:=32767;end;{914:}
if badchar then begin disperr(0,840);if c=47 then begin helpptr:=1;
helpline[0]:=841;end else begin helpptr:=1;helpline[0]:=842;end;
putgeterror;end;if n>4095 then begin begin if interaction=3 then;
printnl(261);print(843);end;printint(n);printchar(41);begin helpptr:=1;
helpline[0]:=844;end;putgeterror;end{:914};end;flushcurexp(n*65536);end;
{:913}{916:}function pathlength:scaled;var n:scaled;p:halfword;
begin p:=curexp;if mem[p].hh.b0=0 then n:=-65536 else n:=0;
repeat p:=mem[p].hh.rh;n:=n+65536;until p=curexp;pathlength:=n;end;
{:916}{919:}procedure testknown(c:quarterword);label 30;var b:30..31;
p,q:halfword;begin b:=31;case curtype of 1,2,4,6,8,9,11,16:b:=30;
13,14:begin p:=mem[curexp+1].int;q:=p+bignodesize[curtype];
repeat q:=q-2;if mem[q].hh.b0<>16 then goto 30;until q=p;b:=30;30:end;
others:end;if c=39 then flushcurexp(b)else flushcurexp(61-b);curtype:=2;
end;{:919}procedure dounary(c:quarterword);var p,q:halfword;x:integer;
begin begin if aritherror then cleararith;end;
if internal[7]>131072 then{902:}begin begindiagnostic;printnl(123);
printop(c);printchar(40);printexp(0,0);print(839);enddiagnostic(false);
end{:902};
case c of 69:if curtype<14 then if curtype<>11 then badunary(69);
70:{903:}case curtype of 14,19:begin q:=curexp;makeexpcopy(q);
if curtype=17 then negatedeplist(mem[curexp+1].hh.rh)else if curtype=14
then begin p:=mem[curexp+1].int;
if mem[p].hh.b0=16 then mem[p+1].int:=-mem[p+1].int else negatedeplist(
mem[p+1].hh.rh);
if mem[p+2].hh.b0=16 then mem[p+3].int:=-mem[p+3].int else negatedeplist
(mem[p+3].hh.rh);end;recyclevalue(q);freenode(q,2);end;
17,18:negatedeplist(mem[curexp+1].hh.rh);16:curexp:=-curexp;
11:negateedges(curexp);others:badunary(70)end{:903};{905:}
41:if curtype<>2 then badunary(41)else curexp:=61-curexp;{:905}{906:}
59,60,61,62,63,64,65,38,66:if curtype<>16 then badunary(c)else case c of
59:curexp:=squarert(curexp);60:curexp:=mexp(curexp);
61:curexp:=mlog(curexp);62,63:begin nsincos((curexp mod 23592960)*16);
if c=62 then curexp:=roundfraction(nsin)else curexp:=roundfraction(ncos)
;end;64:curexp:=floorscaled(curexp);65:curexp:=unifrand(curexp);
38:begin if odd(roundunscaled(curexp))then curexp:=30 else curexp:=31;
curtype:=2;end;66:{1181:}begin curexp:=roundunscaled(curexp)mod 256;
if curexp<0 then curexp:=curexp+256;
if charexists[curexp]then curexp:=30 else curexp:=31;curtype:=2;
end{:1181};end;{:906}{907:}
67:if nicepair(curexp,curtype)then begin p:=mem[curexp+1].int;
x:=narg(mem[p+1].int,mem[p+3].int);
if x>=0 then flushcurexp((x+8)div 16)else flushcurexp(-((-x+8)div 16));
end else badunary(67);{:907}{909:}
53,54:if(curtype<=14)and(curtype>=13)then takepart(c)else badunary(c);
55,56,57,58:if curtype=13 then takepart(c)else badunary(c);{:909}{912:}
50:if curtype<>16 then badunary(50)else begin curexp:=roundunscaled(
curexp)mod 256;curtype:=4;if curexp<0 then curexp:=curexp+256;
if(strstart[curexp+1]-strstart[curexp])<>1 then begin begin if poolptr+1
>maxpoolptr then begin if poolptr+1>poolsize then overflow(257,poolsize-
initpoolptr);maxpoolptr:=poolptr+1;end;end;
begin strpool[poolptr]:=curexp;incr(poolptr);end;curexp:=makestring;end;
end;42:if curtype<>16 then badunary(42)else begin oldsetting:=selector;
selector:=5;printscaled(curexp);curexp:=makestring;selector:=oldsetting;
curtype:=4;end;47,48,49:if curtype<>4 then badunary(c)else strtonum(c);
{:912}{915:}
51:if curtype=4 then flushcurexp((strstart[curexp+1]-strstart[curexp])
*65536)else if curtype=9 then flushcurexp(pathlength)else if curtype=16
then curexp:=abs(curexp)else if nicepair(curexp,curtype)then flushcurexp
(pythadd(mem[mem[curexp+1].int+1].int,mem[mem[curexp+1].int+3].int))else
badunary(c);{:915}{917:}
52:if curtype=14 then flushcurexp(0)else if curtype<>9 then badunary(52)
else if mem[curexp].hh.b0=0 then flushcurexp(0)else begin curpen:=3;
curpathtype:=1;curexp:=makespec(curexp,-1879080960,0);
flushcurexp(turningnumber*65536);end;{:917}{918:}
2:begin if(curtype>=2)and(curtype<=3)then flushcurexp(30)else
flushcurexp(31);curtype:=2;end;
4:begin if(curtype>=4)and(curtype<=5)then flushcurexp(30)else
flushcurexp(31);curtype:=2;end;
6:begin if(curtype>=6)and(curtype<=8)then flushcurexp(30)else
flushcurexp(31);curtype:=2;end;
9:begin if(curtype>=9)and(curtype<=10)then flushcurexp(30)else
flushcurexp(31);curtype:=2;end;
11:begin if(curtype>=11)and(curtype<=12)then flushcurexp(30)else
flushcurexp(31);curtype:=2;end;
13,14:begin if curtype=c then flushcurexp(30)else flushcurexp(31);
curtype:=2;end;
15:begin if(curtype>=16)and(curtype<=19)then flushcurexp(30)else
flushcurexp(31);curtype:=2;end;39,40:testknown(c);{:918}{920:}
68:begin if curtype<>9 then flushcurexp(31)else if mem[curexp].hh.b0<>0
then flushcurexp(30)else flushcurexp(31);curtype:=2;end;{:920}{921:}
45:begin if curtype=14 then pairtopath;
if curtype=9 then curtype:=8 else badunary(45);end;
44:begin if curtype=8 then materializepen;
if curtype<>6 then badunary(44)else begin flushcurexp(makepath(curexp));
curtype:=9;end;end;
46:if curtype<>11 then badunary(46)else flushcurexp(totalweight(curexp))
;43:if curtype=9 then begin p:=htapypoc(curexp);
if mem[p].hh.b1=0 then p:=mem[p].hh.rh;tossknotlist(curexp);curexp:=p;
end else if curtype=14 then pairtopath else badunary(43);{:921}end;
begin if aritherror then cleararith;end;end;{:898}{922:}{923:}
procedure badbinary(p:halfword;c:quarterword);begin disperr(p,283);
disperr(0,835);if c>=94 then printop(c);
printknownorunknowntype(mem[p].hh.b0,p);
if c>=94 then print(478)else printop(c);
printknownorunknowntype(curtype,curexp);begin helpptr:=3;
helpline[2]:=836;helpline[1]:=845;helpline[0]:=846;end;putgeterror;end;
{:923}{928:}function tarnished(p:halfword):halfword;label 10;
var q:halfword;r:halfword;begin q:=mem[p+1].int;
r:=q+bignodesize[mem[p].hh.b0];repeat r:=r-2;
if mem[r].hh.b0=19 then begin tarnished:=1;goto 10;end;until r=q;
tarnished:=0;10:end;{:928}{930:}{935:}procedure depfinish(v,q:halfword;
t:smallnumber);var p:halfword;vv:scaled;
begin if q=0 then p:=curexp else p:=q;mem[p+1].hh.rh:=v;mem[p].hh.b0:=t;
if mem[v].hh.lh=0 then begin vv:=mem[v+1].int;
if q=0 then flushcurexp(vv)else begin recyclevalue(p);mem[q].hh.b0:=16;
mem[q+1].int:=vv;end;end else if q=0 then curtype:=t;
if fixneeded then fixdependencies;end;{:935}
procedure addorsubtract(p,q:halfword;c:quarterword);label 30,10;
var s,t:smallnumber;r:halfword;v:integer;
begin if q=0 then begin t:=curtype;
if t<17 then v:=curexp else v:=mem[curexp+1].hh.rh;
end else begin t:=mem[q].hh.b0;
if t<17 then v:=mem[q+1].int else v:=mem[q+1].hh.rh;end;
if t=16 then begin if c=70 then v:=-v;
if mem[p].hh.b0=16 then begin v:=slowadd(mem[p+1].int,v);
if q=0 then curexp:=v else mem[q+1].int:=v;goto 10;end;{931:}
r:=mem[p+1].hh.rh;while mem[r].hh.lh<>0 do r:=mem[r].hh.rh;
mem[r+1].int:=slowadd(mem[r+1].int,v);if q=0 then begin q:=getnode(2);
curexp:=q;curtype:=mem[p].hh.b0;mem[q].hh.b1:=11;end;
mem[q+1].hh.rh:=mem[p+1].hh.rh;mem[q].hh.b0:=mem[p].hh.b0;
mem[q+1].hh.lh:=mem[p+1].hh.lh;mem[mem[p+1].hh.lh].hh.rh:=q;
mem[p].hh.b0:=16;{:931};end else begin if c=70 then negatedeplist(v);
{932:}if mem[p].hh.b0=16 then{933:}
begin while mem[v].hh.lh<>0 do v:=mem[v].hh.rh;
mem[v+1].int:=slowadd(mem[p+1].int,mem[v+1].int);end{:933}
else begin s:=mem[p].hh.b0;r:=mem[p+1].hh.rh;
if t=17 then begin if s=17 then if maxcoef(r)+maxcoef(v)<626349397 then
begin v:=pplusq(v,r,17);goto 30;end;t:=18;v:=poverv(v,65536,17,18);end;
if s=18 then v:=pplusq(v,r,18)else v:=pplusfq(v,65536,r,18,17);30:{934:}
if q<>0 then depfinish(v,q,t)else begin curtype:=t;depfinish(v,0,t);
end{:934};end{:932};end;10:end;{:930}{943:}procedure depmult(p:halfword;
v:integer;visscaled:boolean);label 10;var q:halfword;s,t:smallnumber;
begin if p=0 then q:=curexp else if mem[p].hh.b0<>16 then q:=p else
begin if visscaled then mem[p+1].int:=takescaled(mem[p+1].int,v)else mem
[p+1].int:=takefraction(mem[p+1].int,v);goto 10;end;t:=mem[q].hh.b0;
q:=mem[q+1].hh.rh;s:=t;
if t=17 then if visscaled then if abvscd(maxcoef(q),abs(v),626349396,
65536)>=0 then t:=18;q:=ptimesv(q,v,s,t,visscaled);depfinish(q,p,t);
10:end;{:943}{946:}procedure hardtimes(p:halfword);var q:halfword;
r:halfword;u,v:scaled;
begin if mem[p].hh.b0=14 then begin q:=stashcurexp;unstashcurexp(p);
p:=q;end;r:=mem[curexp+1].int;u:=mem[r+1].int;v:=mem[r+3].int;{947:}
mem[r+2].hh.b0:=mem[p].hh.b0;newdep(r+2,copydeplist(mem[p+1].hh.rh));
mem[r].hh.b0:=mem[p].hh.b0;mem[r+1]:=mem[p+1];
mem[mem[p+1].hh.lh].hh.rh:=r;freenode(p,2){:947};depmult(r,u,true);
depmult(r+2,v,true);end;{:946}{949:}procedure depdiv(p:halfword;
v:scaled);label 10;var q:halfword;s,t:smallnumber;
begin if p=0 then q:=curexp else if mem[p].hh.b0<>16 then q:=p else
begin mem[p+1].int:=makescaled(mem[p+1].int,v);goto 10;end;
t:=mem[q].hh.b0;q:=mem[q+1].hh.rh;s:=t;
if t=17 then if abvscd(maxcoef(q),65536,626349396,abs(v))>=0 then t:=18;
q:=poverv(q,v,s,t);depfinish(q,p,t);10:end;{:949}{953:}
procedure setuptrans(c:quarterword);label 30,10;var p,q,r:halfword;
begin if(c<>88)or(curtype<>13)then{955:}begin p:=stashcurexp;
curexp:=idtransform;curtype:=13;q:=mem[curexp+1].int;case c of{957:}
84:if mem[p].hh.b0=16 then{958:}
begin nsincos((mem[p+1].int mod 23592960)*16);
mem[q+5].int:=roundfraction(ncos);mem[q+9].int:=roundfraction(nsin);
mem[q+7].int:=-mem[q+9].int;mem[q+11].int:=mem[q+5].int;goto 30;
end{:958};85:if mem[p].hh.b0>14 then begin install(q+6,p);goto 30;end;
86:if mem[p].hh.b0>14 then begin install(q+4,p);install(q+10,p);goto 30;
end;87:if mem[p].hh.b0=14 then begin r:=mem[p+1].int;install(q,r);
install(q+2,r+2);goto 30;end;
89:if mem[p].hh.b0>14 then begin install(q+4,p);goto 30;end;
90:if mem[p].hh.b0>14 then begin install(q+10,p);goto 30;end;
91:if mem[p].hh.b0=14 then{959:}begin r:=mem[p+1].int;install(q+4,r);
install(q+10,r);install(q+8,r+2);
if mem[r+2].hh.b0=16 then mem[r+3].int:=-mem[r+3].int else negatedeplist
(mem[r+3].hh.rh);install(q+6,r+2);goto 30;end{:959};88:;{:957}end;
disperr(p,855);begin helpptr:=3;helpline[2]:=856;helpline[1]:=857;
helpline[0]:=537;end;putgeterror;30:recyclevalue(p);freenode(p,2);
end{:955};{956:}q:=mem[curexp+1].int;r:=q+12;repeat r:=r-2;
if mem[r].hh.b0<>16 then goto 10;until r=q;txx:=mem[q+5].int;
txy:=mem[q+7].int;tyx:=mem[q+9].int;tyy:=mem[q+11].int;tx:=mem[q+1].int;
ty:=mem[q+3].int;flushcurexp(0){:956};10:end;{:953}{960:}
procedure setupknowntrans(c:quarterword);begin setuptrans(c);
if curtype<>16 then begin disperr(0,858);begin helpptr:=3;
helpline[2]:=859;helpline[1]:=860;helpline[0]:=537;end;
putgetflusherror(0);txx:=65536;txy:=0;tyx:=0;tyy:=65536;tx:=0;ty:=0;end;
end;{:960}{961:}procedure trans(p,q:halfword);var v:scaled;
begin v:=takescaled(mem[p].int,txx)+takescaled(mem[q].int,txy)+tx;
mem[q].int:=takescaled(mem[p].int,tyx)+takescaled(mem[q].int,tyy)+ty;
mem[p].int:=v;end;{:961}{962:}procedure pathtrans(p:halfword;
c:quarterword);label 10;var q:halfword;begin setupknowntrans(c);
unstashcurexp(p);
if curtype=6 then begin if mem[curexp+9].int=0 then if tx=0 then if ty=0
then goto 10;flushcurexp(makepath(curexp));curtype:=8;end;q:=curexp;
repeat if mem[q].hh.b0<>0 then trans(q+3,q+4);trans(q+1,q+2);
if mem[q].hh.b1<>0 then trans(q+5,q+6);q:=mem[q].hh.rh;until q=curexp;
10:end;{:962}{963:}procedure edgestrans(p:halfword;c:quarterword);
label 10;begin setupknowntrans(c);unstashcurexp(p);curedges:=curexp;
if mem[curedges].hh.rh=curedges then goto 10;
if txx=0 then if tyy=0 then if txy mod 65536=0 then if tyx mod 65536=0
then begin xyswapedges;txx:=txy;tyy:=tyx;txy:=0;tyx:=0;
if mem[curedges].hh.rh=curedges then goto 10;end;
if txy=0 then if tyx=0 then if txx mod 65536=0 then if tyy mod 65536=0
then{964:}begin if(txx=0)or(tyy=0)then begin tossedges(curedges);
curexp:=getnode(6);initedges(curexp);
end else begin if txx<0 then begin xreflectedges;txx:=-txx;end;
if tyy<0 then begin yreflectedges;tyy:=-tyy;end;
if txx<>65536 then xscaleedges(txx div 65536);
if tyy<>65536 then yscaleedges(tyy div 65536);{965:}
tx:=roundunscaled(tx);ty:=roundunscaled(ty);
if(toint(mem[curedges+2].hh.lh)+tx<=0)or(mem[curedges+2].hh.rh+tx>=8192)
or(toint(mem[curedges+1].hh.lh)+ty<=0)or(mem[curedges+1].hh.rh+ty>=8191)
or(abs(tx)>=4096)or(abs(ty)>=4096)then begin begin if interaction=3 then
;printnl(261);print(864);end;begin helpptr:=3;helpline[2]:=865;
helpline[1]:=536;helpline[0]:=537;end;putgeterror;
end else begin if tx<>0 then begin if not(abs(mem[curedges+3].hh.lh-tx
-4096)<4096)then fixoffset;
mem[curedges+2].hh.lh:=mem[curedges+2].hh.lh+tx;
mem[curedges+2].hh.rh:=mem[curedges+2].hh.rh+tx;
mem[curedges+3].hh.lh:=mem[curedges+3].hh.lh-tx;mem[curedges+4].int:=0;
end;if ty<>0 then begin mem[curedges+1].hh.lh:=mem[curedges+1].hh.lh+ty;
mem[curedges+1].hh.rh:=mem[curedges+1].hh.rh+ty;
mem[curedges+5].hh.lh:=mem[curedges+5].hh.lh+ty;mem[curedges+4].int:=0;
end;end{:965};end;goto 10;end{:964};begin if interaction=3 then;
printnl(261);print(861);end;begin helpptr:=3;helpline[2]:=862;
helpline[1]:=863;helpline[0]:=537;end;putgeterror;10:end;{:963}{966:}
{968:}procedure bilin1(p:halfword;t:scaled;q:halfword;u,delta:scaled);
var r:halfword;begin if t<>65536 then depmult(p,t,true);
if u<>0 then if mem[q].hh.b0=16 then delta:=delta+takescaled(mem[q+1].
int,u)else begin{969:}
if mem[p].hh.b0<>18 then begin if mem[p].hh.b0=16 then newdep(p,
constdependency(mem[p+1].int))else mem[p+1].hh.rh:=ptimesv(mem[p+1].hh.
rh,65536,17,18,true);mem[p].hh.b0:=18;end{:969};
mem[p+1].hh.rh:=pplusfq(mem[p+1].hh.rh,u,mem[q+1].hh.rh,18,mem[q].hh.b0)
;end;
if mem[p].hh.b0=16 then mem[p+1].int:=mem[p+1].int+delta else begin r:=
mem[p+1].hh.rh;while mem[r].hh.lh<>0 do r:=mem[r].hh.rh;
delta:=mem[r+1].int+delta;
if r<>mem[p+1].hh.rh then mem[r+1].int:=delta else begin recyclevalue(p)
;mem[p].hh.b0:=16;mem[p+1].int:=delta;end;end;
if fixneeded then fixdependencies;end;{:968}{971:}
procedure addmultdep(p:halfword;v:scaled;r:halfword);
begin if mem[r].hh.b0=16 then mem[depfinal+1].int:=mem[depfinal+1].int+
takescaled(mem[r+1].int,v)else begin mem[p+1].hh.rh:=pplusfq(mem[p+1].hh
.rh,v,mem[r+1].hh.rh,18,mem[r].hh.b0);if fixneeded then fixdependencies;
end;end;{:971}{972:}procedure bilin2(p,t:halfword;v:scaled;
u,q:halfword);var vv:scaled;begin vv:=mem[p+1].int;mem[p].hh.b0:=18;
newdep(p,constdependency(0));if vv<>0 then addmultdep(p,vv,t);
if v<>0 then addmultdep(p,v,u);if q<>0 then addmultdep(p,65536,q);
if mem[p+1].hh.rh=depfinal then begin vv:=mem[depfinal+1].int;
recyclevalue(p);mem[p].hh.b0:=16;mem[p+1].int:=vv;end;end;{:972}{974:}
procedure bilin3(p:halfword;t,v,u,delta:scaled);
begin if t<>65536 then delta:=delta+takescaled(mem[p+1].int,t)else delta
:=delta+mem[p+1].int;
if u<>0 then mem[p+1].int:=delta+takescaled(v,u)else mem[p+1].int:=delta
;end;{:974}procedure bigtrans(p:halfword;c:quarterword);label 10;
var q,r,pp,qq:halfword;s:smallnumber;begin s:=bignodesize[mem[p].hh.b0];
q:=mem[p+1].int;r:=q+s;repeat r:=r-2;if mem[r].hh.b0<>16 then{967:}
begin setupknowntrans(c);makeexpcopy(p);r:=mem[curexp+1].int;
if curtype=13 then begin bilin1(r+10,tyy,q+6,tyx,0);
bilin1(r+8,tyy,q+4,tyx,0);bilin1(r+6,txx,q+10,txy,0);
bilin1(r+4,txx,q+8,txy,0);end;bilin1(r+2,tyy,q,tyx,ty);
bilin1(r,txx,q+2,txy,tx);goto 10;end{:967};until r=q;{970:}
setuptrans(c);if curtype=16 then{973:}begin makeexpcopy(p);
r:=mem[curexp+1].int;
if curtype=13 then begin bilin3(r+10,tyy,mem[q+7].int,tyx,0);
bilin3(r+8,tyy,mem[q+5].int,tyx,0);bilin3(r+6,txx,mem[q+11].int,txy,0);
bilin3(r+4,txx,mem[q+9].int,txy,0);end;
bilin3(r+2,tyy,mem[q+1].int,tyx,ty);bilin3(r,txx,mem[q+3].int,txy,tx);
end{:973}else begin pp:=stashcurexp;qq:=mem[pp+1].int;makeexpcopy(p);
r:=mem[curexp+1].int;
if curtype=13 then begin bilin2(r+10,qq+10,mem[q+7].int,qq+8,0);
bilin2(r+8,qq+10,mem[q+5].int,qq+8,0);
bilin2(r+6,qq+4,mem[q+11].int,qq+6,0);
bilin2(r+4,qq+4,mem[q+9].int,qq+6,0);end;
bilin2(r+2,qq+10,mem[q+1].int,qq+8,qq+2);
bilin2(r,qq+4,mem[q+3].int,qq+6,qq);recyclevalue(pp);freenode(pp,2);end;
{:970};10:end;{:966}{976:}procedure cat(p:halfword);var a,b:strnumber;
k:poolpointer;begin a:=mem[p+1].int;b:=curexp;
begin if poolptr+(strstart[a+1]-strstart[a])+(strstart[b+1]-strstart[b])
>maxpoolptr then begin if poolptr+(strstart[a+1]-strstart[a])+(strstart[
b+1]-strstart[b])>poolsize then overflow(257,poolsize-initpoolptr);
maxpoolptr:=poolptr+(strstart[a+1]-strstart[a])+(strstart[b+1]-strstart[
b]);end;end;
for k:=strstart[a]to strstart[a+1]-1 do begin strpool[poolptr]:=strpool[
k];incr(poolptr);end;
for k:=strstart[b]to strstart[b+1]-1 do begin strpool[poolptr]:=strpool[
k];incr(poolptr);end;curexp:=makestring;
begin if strref[b]<127 then if strref[b]>1 then decr(strref[b])else
flushstring(b);end;end;{:976}{977:}procedure chopstring(p:halfword);
var a,b:integer;l:integer;k:integer;s:strnumber;reversed:boolean;
begin a:=roundunscaled(mem[p+1].int);b:=roundunscaled(mem[p+3].int);
if a<=b then reversed:=false else begin reversed:=true;k:=a;a:=b;b:=k;
end;s:=curexp;l:=(strstart[s+1]-strstart[s]);if a<0 then begin a:=0;
if b<0 then b:=0;end;if b>l then begin b:=l;if a>l then a:=l;end;
begin if poolptr+b-a>maxpoolptr then begin if poolptr+b-a>poolsize then
overflow(257,poolsize-initpoolptr);maxpoolptr:=poolptr+b-a;end;end;
if reversed then for k:=strstart[s]+b-1 downto strstart[s]+a do begin
strpool[poolptr]:=strpool[k];incr(poolptr);
end else for k:=strstart[s]+a to strstart[s]+b-1 do begin strpool[
poolptr]:=strpool[k];incr(poolptr);end;curexp:=makestring;
begin if strref[s]<127 then if strref[s]>1 then decr(strref[s])else
flushstring(s);end;end;{:977}{978:}procedure choppath(p:halfword);
var q:halfword;pp,qq,rr,ss:halfword;a,b,k,l:scaled;reversed:boolean;
begin l:=pathlength;a:=mem[p+1].int;b:=mem[p+3].int;
if a<=b then reversed:=false else begin reversed:=true;k:=a;a:=b;b:=k;
end;{979:}if a<0 then if mem[curexp].hh.b0=0 then begin a:=0;
if b<0 then b:=0;end else repeat a:=a+l;b:=b+l;until a>=0;
if b>l then if mem[curexp].hh.b0=0 then begin b:=l;if a>l then a:=l;
end else while a>=l do begin a:=a-l;b:=b-l;end{:979};q:=curexp;
while a>=65536 do begin q:=mem[q].hh.rh;a:=a-65536;b:=b-65536;end;
if b=a then{981:}begin if a>0 then begin qq:=mem[q].hh.rh;
splitcubic(q,a*4096,mem[qq+1].int,mem[qq+2].int);q:=mem[q].hh.rh;end;
pp:=copyknot(q);qq:=pp;end{:981}else{980:}begin pp:=copyknot(q);qq:=pp;
repeat q:=mem[q].hh.rh;rr:=qq;qq:=copyknot(q);mem[rr].hh.rh:=qq;
b:=b-65536;until b<=0;if a>0 then begin ss:=pp;pp:=mem[pp].hh.rh;
splitcubic(ss,a*4096,mem[pp+1].int,mem[pp+2].int);pp:=mem[ss].hh.rh;
freenode(ss,7);if rr=ss then begin b:=makescaled(b,65536-a);rr:=pp;end;
end;
if b<0 then begin splitcubic(rr,(b+65536)*4096,mem[qq+1].int,mem[qq+2].
int);freenode(qq,7);qq:=mem[rr].hh.rh;end;end{:980};mem[pp].hh.b0:=0;
mem[qq].hh.b1:=0;mem[qq].hh.rh:=pp;tossknotlist(curexp);
if reversed then begin curexp:=mem[htapypoc(pp)].hh.rh;tossknotlist(pp);
end else curexp:=pp;end;{:978}{982:}procedure pairvalue(x,y:scaled);
var p:halfword;begin p:=getnode(2);flushcurexp(p);curtype:=14;
mem[p].hh.b0:=14;mem[p].hh.b1:=11;initbignode(p);p:=mem[p+1].int;
mem[p].hh.b0:=16;mem[p+1].int:=x;mem[p+2].hh.b0:=16;mem[p+3].int:=y;end;
{:982}{984:}procedure setupoffset(p:halfword);
begin findoffset(mem[p+1].int,mem[p+3].int,curexp);pairvalue(curx,cury);
end;procedure setupdirectiontime(p:halfword);
begin flushcurexp(finddirectiontime(mem[p+1].int,mem[p+3].int,curexp));
end;{:984}{985:}procedure findpoint(v:scaled;c:quarterword);
var p:halfword;n:scaled;q:halfword;begin p:=curexp;
if mem[p].hh.b0=0 then n:=-65536 else n:=0;repeat p:=mem[p].hh.rh;
n:=n+65536;until p=curexp;
if n=0 then v:=0 else if v<0 then if mem[p].hh.b0=0 then v:=0 else v:=n
-1-((-v-1)mod n)else if v>n then if mem[p].hh.b0=0 then v:=n else v:=v
mod n;p:=curexp;while v>=65536 do begin p:=mem[p].hh.rh;v:=v-65536;end;
if v<>0 then{986:}begin q:=mem[p].hh.rh;
splitcubic(p,v*4096,mem[q+1].int,mem[q+2].int);p:=mem[p].hh.rh;end{:986}
;{987:}case c of 97:pairvalue(mem[p+1].int,mem[p+2].int);
98:if mem[p].hh.b0=0 then pairvalue(mem[p+1].int,mem[p+2].int)else
pairvalue(mem[p+3].int,mem[p+4].int);
99:if mem[p].hh.b1=0 then pairvalue(mem[p+1].int,mem[p+2].int)else
pairvalue(mem[p+5].int,mem[p+6].int);end{:987};end;{:985}
procedure dobinary(p:halfword;c:quarterword);label 30,31,10;
var q,r,rr:halfword;oldp,oldexp:halfword;v:integer;
begin begin if aritherror then cleararith;end;
if internal[7]>131072 then{924:}begin begindiagnostic;printnl(847);
printexp(p,0);printchar(41);printop(c);printchar(40);printexp(0,0);
print(839);enddiagnostic(false);end{:924};{926:}
case mem[p].hh.b0 of 13,14:oldp:=tarnished(p);19:oldp:=1;
others:oldp:=0 end;if oldp<>0 then begin q:=stashcurexp;oldp:=p;
makeexpcopy(oldp);p:=stashcurexp;unstashcurexp(q);end;{:926};{927:}
case curtype of 13,14:oldexp:=tarnished(curexp);19:oldexp:=1;
others:oldexp:=0 end;if oldexp<>0 then begin oldexp:=curexp;
makeexpcopy(oldexp);end{:927};case c of 69,70:{929:}
if(curtype<14)or(mem[p].hh.b0<14)then if(curtype=11)and(mem[p].hh.b0=11)
then begin if c=70 then negateedges(curexp);curedges:=curexp;
mergeedges(mem[p+1].int);
end else badbinary(p,c)else if curtype=14 then if mem[p].hh.b0<>14 then
badbinary(p,c)else begin q:=mem[p+1].int;r:=mem[curexp+1].int;
addorsubtract(q,r,c);addorsubtract(q+2,r+2,c);
end else if mem[p].hh.b0=14 then badbinary(p,c)else addorsubtract(p,0,c)
{:929};{936:}
77,78,79,80,81,82:begin if(curtype>14)and(mem[p].hh.b0>14)then
addorsubtract(p,0,70)else if curtype<>mem[p].hh.b0 then begin badbinary(
p,c);goto 30;
end else if curtype=4 then flushcurexp(strvsstr(mem[p+1].int,curexp))
else if(curtype=5)or(curtype=3)then{938:}begin q:=mem[curexp+1].int;
while(q<>curexp)and(q<>p)do q:=mem[q+1].int;if q=p then flushcurexp(0);
end{:938}else if(curtype=14)or(curtype=13)then{939:}
begin q:=mem[p+1].int;r:=mem[curexp+1].int;rr:=r+bignodesize[curtype]-2;
while true do begin addorsubtract(q,r,70);
if mem[r].hh.b0<>16 then goto 31;if mem[r+1].int<>0 then goto 31;
if r=rr then goto 31;q:=q+2;r:=r+2;end;
31:takepart(53+(r-mem[curexp+1].int)div 2);end{:939}
else if curtype=2 then flushcurexp(curexp-mem[p+1].int)else begin
badbinary(p,c);goto 30;end;{937:}
if curtype<>16 then begin if curtype<16 then begin disperr(p,283);
begin helpptr:=1;helpline[0]:=848;end end else begin helpptr:=2;
helpline[1]:=849;helpline[0]:=850;end;disperr(0,851);
putgetflusherror(31);
end else case c of 77:if curexp<0 then curexp:=30 else curexp:=31;
78:if curexp<=0 then curexp:=30 else curexp:=31;
79:if curexp>0 then curexp:=30 else curexp:=31;
80:if curexp>=0 then curexp:=30 else curexp:=31;
81:if curexp=0 then curexp:=30 else curexp:=31;
82:if curexp<>0 then curexp:=30 else curexp:=31;end;curtype:=2{:937};
30:end;{:936}{940:}
76,75:if(mem[p].hh.b0<>2)or(curtype<>2)then badbinary(p,c)else if mem[p
+1].int=c-45 then curexp:=mem[p+1].int;{:940}{941:}
71:if(curtype<14)or(mem[p].hh.b0<14)then badbinary(p,71)else if(curtype=
16)or(mem[p].hh.b0=16)then{942:}
begin if mem[p].hh.b0=16 then begin v:=mem[p+1].int;freenode(p,2);
end else begin v:=curexp;unstashcurexp(p);end;
if curtype=16 then curexp:=takescaled(curexp,v)else if curtype=14 then
begin p:=mem[curexp+1].int;depmult(p,v,true);depmult(p+2,v,true);
end else depmult(0,v,true);goto 10;end{:942}
else if(nicepair(p,mem[p].hh.b0)and(curtype>14))or(nicepair(curexp,
curtype)and(mem[p].hh.b0>14))then begin hardtimes(p);goto 10;
end else badbinary(p,71);{:941}{948:}
72:if(curtype<>16)or(mem[p].hh.b0<14)then badbinary(p,72)else begin v:=
curexp;unstashcurexp(p);if v=0 then{950:}begin disperr(0,781);
begin helpptr:=2;helpline[1]:=853;helpline[0]:=854;end;putgeterror;
end{:950}
else begin if curtype=16 then curexp:=makescaled(curexp,v)else if
curtype=14 then begin p:=mem[curexp+1].int;depdiv(p,v);depdiv(p+2,v);
end else depdiv(0,v);end;goto 10;end;{:948}{951:}
73,74:if(curtype=16)and(mem[p].hh.b0=16)then if c=73 then curexp:=
pythadd(mem[p+1].int,curexp)else curexp:=pythsub(mem[p+1].int,curexp)
else badbinary(p,c);{:951}{952:}
84,85,86,87,88,89,90,91:if(mem[p].hh.b0=9)or(mem[p].hh.b0=8)or(mem[p].hh
.b0=6)then begin pathtrans(p,c);goto 10;
end else if(mem[p].hh.b0=14)or(mem[p].hh.b0=13)then bigtrans(p,c)else if
mem[p].hh.b0=11 then begin edgestrans(p,c);goto 10;
end else badbinary(p,c);{:952}{975:}
83:if(curtype=4)and(mem[p].hh.b0=4)then cat(p)else badbinary(p,83);
94:if nicepair(p,mem[p].hh.b0)and(curtype=4)then chopstring(mem[p+1].int
)else badbinary(p,94);95:begin if curtype=14 then pairtopath;
if nicepair(p,mem[p].hh.b0)and(curtype=9)then choppath(mem[p+1].int)else
badbinary(p,95);end;{:975}{983:}
97,98,99:begin if curtype=14 then pairtopath;
if(curtype=9)and(mem[p].hh.b0=16)then findpoint(mem[p+1].int,c)else
badbinary(p,c);end;100:begin if curtype=8 then materializepen;
if(curtype=6)and nicepair(p,mem[p].hh.b0)then setupoffset(mem[p+1].int)
else badbinary(p,100);end;96:begin if curtype=14 then pairtopath;
if(curtype=9)and nicepair(p,mem[p].hh.b0)then setupdirectiontime(mem[p+1
].int)else badbinary(p,96);end;{:983}{988:}
92:begin if mem[p].hh.b0=14 then begin q:=stashcurexp;unstashcurexp(p);
pairtopath;p:=stashcurexp;unstashcurexp(q);end;
if curtype=14 then pairtopath;
if(curtype=9)and(mem[p].hh.b0=9)then begin pathintersection(mem[p+1].int
,curexp);pairvalue(curt,curtt);end else badbinary(p,92);end;{:988}end;
recyclevalue(p);freenode(p,2);10:begin if aritherror then cleararith;
end;{925:}if oldp<>0 then begin recyclevalue(oldp);freenode(oldp,2);end;
if oldexp<>0 then begin recyclevalue(oldexp);freenode(oldexp,2);
end{:925};end;{:922}{944:}procedure fracmult(n,d:scaled);var p:halfword;
oldexp:halfword;v:fraction;begin if internal[7]>131072 then{945:}
begin begindiagnostic;printnl(847);printscaled(n);printchar(47);
printscaled(d);print(852);printexp(0,0);print(839);enddiagnostic(false);
end{:945};case curtype of 13,14:oldexp:=tarnished(curexp);19:oldexp:=1;
others:oldexp:=0 end;if oldexp<>0 then begin oldexp:=curexp;
makeexpcopy(oldexp);end;v:=makefraction(n,d);
if curtype=16 then curexp:=takefraction(curexp,v)else if curtype=14 then
begin p:=mem[curexp+1].int;depmult(p,v,false);depmult(p+2,v,false);
end else depmult(0,v,false);
if oldexp<>0 then begin recyclevalue(oldexp);freenode(oldexp,2);end end;
{:944}{989:}{1155:}procedure gfswap;
begin if gflimit=gfbufsize then begin writegf(0,halfbuf-1);
gflimit:=halfbuf;gfoffset:=gfoffset+gfbufsize;gfptr:=0;
end else begin writegf(halfbuf,gfbufsize-1);gflimit:=gfbufsize;end;end;
{:1155}{1157:}procedure gffour(x:integer);
begin if x>=0 then begin gfbuf[gfptr]:=x div 16777216;incr(gfptr);
if gfptr=gflimit then gfswap;end else begin x:=x+1073741824;
x:=x+1073741824;begin gfbuf[gfptr]:=(x div 16777216)+128;incr(gfptr);
if gfptr=gflimit then gfswap;end;end;x:=x mod 16777216;
begin gfbuf[gfptr]:=x div 65536;incr(gfptr);
if gfptr=gflimit then gfswap;end;x:=x mod 65536;
begin gfbuf[gfptr]:=x div 256;incr(gfptr);if gfptr=gflimit then gfswap;
end;begin gfbuf[gfptr]:=x mod 256;incr(gfptr);
if gfptr=gflimit then gfswap;end;end;{:1157}{1158:}
procedure gftwo(x:integer);begin begin gfbuf[gfptr]:=x div 256;
incr(gfptr);if gfptr=gflimit then gfswap;end;
begin gfbuf[gfptr]:=x mod 256;incr(gfptr);if gfptr=gflimit then gfswap;
end;end;procedure gfthree(x:integer);
begin begin gfbuf[gfptr]:=x div 65536;incr(gfptr);
if gfptr=gflimit then gfswap;end;
begin gfbuf[gfptr]:=(x mod 65536)div 256;incr(gfptr);
if gfptr=gflimit then gfswap;end;begin gfbuf[gfptr]:=x mod 256;
incr(gfptr);if gfptr=gflimit then gfswap;end;end;{:1158}{1159:}
procedure gfpaint(d:integer);begin if d<64 then begin gfbuf[gfptr]:=0+d;
incr(gfptr);if gfptr=gflimit then gfswap;
end else if d<256 then begin begin gfbuf[gfptr]:=64;incr(gfptr);
if gfptr=gflimit then gfswap;end;begin gfbuf[gfptr]:=d;incr(gfptr);
if gfptr=gflimit then gfswap;end;end else begin begin gfbuf[gfptr]:=65;
incr(gfptr);if gfptr=gflimit then gfswap;end;gftwo(d);end;end;{:1159}
{1160:}procedure gfstring(s,t:strnumber);var k:poolpointer;l:integer;
begin if s<>0 then begin l:=(strstart[s+1]-strstart[s]);
if t<>0 then l:=l+(strstart[t+1]-strstart[t]);
if l<=255 then begin begin gfbuf[gfptr]:=239;incr(gfptr);
if gfptr=gflimit then gfswap;end;begin gfbuf[gfptr]:=l;incr(gfptr);
if gfptr=gflimit then gfswap;end;end else begin begin gfbuf[gfptr]:=241;
incr(gfptr);if gfptr=gflimit then gfswap;end;gfthree(l);end;
for k:=strstart[s]to strstart[s+1]-1 do begin gfbuf[gfptr]:=strpool[k];
incr(gfptr);if gfptr=gflimit then gfswap;end;end;
if t<>0 then for k:=strstart[t]to strstart[t+1]-1 do begin gfbuf[gfptr]
:=strpool[k];incr(gfptr);if gfptr=gflimit then gfswap;end;end;{:1160}
{1161:}procedure gfboc(minm,maxm,minn,maxn:integer);label 10;
begin if minm<gfminm then gfminm:=minm;if maxn>gfmaxn then gfmaxn:=maxn;
if bocp=-1 then if bocc>=0 then if bocc<256 then if maxm-minm>=0 then if
maxm-minm<256 then if maxm>=0 then if maxm<256 then if maxn-minn>=0 then
if maxn-minn<256 then if maxn>=0 then if maxn<256 then begin begin gfbuf
[gfptr]:=68;incr(gfptr);if gfptr=gflimit then gfswap;end;
begin gfbuf[gfptr]:=bocc;incr(gfptr);if gfptr=gflimit then gfswap;end;
begin gfbuf[gfptr]:=maxm-minm;incr(gfptr);if gfptr=gflimit then gfswap;
end;begin gfbuf[gfptr]:=maxm;incr(gfptr);if gfptr=gflimit then gfswap;
end;begin gfbuf[gfptr]:=maxn-minn;incr(gfptr);
if gfptr=gflimit then gfswap;end;begin gfbuf[gfptr]:=maxn;incr(gfptr);
if gfptr=gflimit then gfswap;end;goto 10;end;begin gfbuf[gfptr]:=67;
incr(gfptr);if gfptr=gflimit then gfswap;end;gffour(bocc);gffour(bocp);
gffour(minm);gffour(maxm);gffour(minn);gffour(maxn);10:end;{:1161}
{1163:}procedure initgf;var k:0..256;t:integer;begin gfminm:=4096;
gfmaxm:=-4096;gfminn:=4096;gfmaxn:=-4096;
for k:=0 to 255 do charptr[k]:=-1;{1164:}
if internal[27]<=0 then gfext:=1051 else begin oldsetting:=selector;
selector:=5;printchar(46);printint(makescaled(internal[27],59429463));
print(1052);gfext:=makestring;selector:=oldsetting;end{:1164};
begin if jobname=0 then openlogfile;packjobname(gfext);
while not bopenout(gffile)do promptfilename(753,gfext);
outputfilename:=bmakenamestring(gffile);end;begin gfbuf[gfptr]:=247;
incr(gfptr);if gfptr=gflimit then gfswap;end;begin gfbuf[gfptr]:=131;
incr(gfptr);if gfptr=gflimit then gfswap;end;oldsetting:=selector;
selector:=5;print(1050);printint(roundunscaled(internal[14]));
printchar(46);printdd(roundunscaled(internal[15]));printchar(46);
printdd(roundunscaled(internal[16]));printchar(58);
t:=roundunscaled(internal[17]);printdd(t div 60);printdd(t mod 60);
selector:=oldsetting;begin gfbuf[gfptr]:=(poolptr-strstart[strptr]);
incr(gfptr);if gfptr=gflimit then gfswap;end;
strstart[strptr+1]:=poolptr;gfstring(0,strptr);
poolptr:=strstart[strptr];gfprevptr:=gfoffset+gfptr;end;{:1163}{1165:}
procedure shipout(c:eightbits);label 30;var f:integer;
prevm,m,mm:integer;prevn,n:integer;p,q:halfword;prevw,w,ww:integer;
d:integer;delta:integer;curminm:integer;xoff,yoff:integer;
begin if outputfilename=0 then initgf;f:=roundunscaled(internal[19]);
xoff:=roundunscaled(internal[29]);yoff:=roundunscaled(internal[30]);
if termoffset>maxprintline-9 then println else if(termoffset>0)or(
fileoffset>0)then printchar(32);printchar(91);printint(c);
if f<>0 then begin printchar(46);printint(f);end;flush(stdout);
bocc:=256*f+c;bocp:=charptr[c];charptr[c]:=gfprevptr;
if internal[34]>0 then{1166:}
begin if xoff<>0 then begin gfstring(436,0);begin gfbuf[gfptr]:=243;
incr(gfptr);if gfptr=gflimit then gfswap;end;gffour(xoff*65536);end;
if yoff<>0 then begin gfstring(437,0);begin gfbuf[gfptr]:=243;
incr(gfptr);if gfptr=gflimit then gfswap;end;gffour(yoff*65536);end;
end{:1166};{1167:}prevn:=4096;p:=mem[curedges].hh.lh;
n:=mem[curedges+1].hh.rh-4096;while p<>curedges do begin{1169:}
if mem[p+1].hh.lh>1 then sortedges(p);q:=mem[p+1].hh.rh;w:=0;
prevm:=-268435456;ww:=0;prevw:=0;m:=prevm;
repeat if q=memtop then mm:=268435456 else begin d:=mem[q].hh.lh;
mm:=d div 8;ww:=ww+(d mod 8)-4;end;
if mm<>m then begin if prevw<=0 then begin if w>0 then{1170:}
begin if prevm=-268435456 then{1172:}
begin if prevn=4096 then begin gfboc(mem[curedges+2].hh.lh+xoff-4096,mem
[curedges+2].hh.rh+xoff-4096,mem[curedges+1].hh.lh+yoff-4096,n+yoff);
curminm:=mem[curedges+2].hh.lh-4096+mem[curedges+3].hh.lh;
end else if prevn>n+1 then{1174:}begin delta:=prevn-n-1;
if delta<256 then begin begin gfbuf[gfptr]:=71;incr(gfptr);
if gfptr=gflimit then gfswap;end;begin gfbuf[gfptr]:=delta;incr(gfptr);
if gfptr=gflimit then gfswap;end;end else begin begin gfbuf[gfptr]:=72;
incr(gfptr);if gfptr=gflimit then gfswap;end;gftwo(delta);end;end{:1174}
else{1173:}begin delta:=m-curminm;
if delta>164 then begin gfbuf[gfptr]:=70;incr(gfptr);
if gfptr=gflimit then gfswap;
end else begin begin gfbuf[gfptr]:=74+delta;incr(gfptr);
if gfptr=gflimit then gfswap;end;goto 30;end;end{:1173};
gfpaint(m-curminm);30:prevn:=n;end{:1172}else gfpaint(m-prevm);prevm:=m;
prevw:=w;end{:1170};end else if w<=0 then{1171:}begin gfpaint(m-prevm);
prevm:=m;prevw:=w;end{:1171};m:=mm;end;w:=ww;q:=mem[q].hh.rh;
until mm=268435456;if w<>0 then printnl(1054);
if prevm-toint(mem[curedges+3].hh.lh)+xoff>gfmaxm then gfmaxm:=prevm-mem
[curedges+3].hh.lh+xoff{:1169};p:=mem[p].hh.lh;decr(n);end;
if prevn=4096 then{1168:}begin gfboc(0,0,0,0);
if gfmaxm<0 then gfmaxm:=0;if gfminn>0 then gfminn:=0;end{:1168}
else if prevn+yoff<gfminn then gfminn:=prevn+yoff{:1167};
begin gfbuf[gfptr]:=69;incr(gfptr);if gfptr=gflimit then gfswap;end;
gfprevptr:=gfoffset+gfptr;incr(totalchars);printchar(93);flush(stdout);
if internal[11]>0 then printedges(1053,true,xoff,yoff);end;{:1165}{995:}
{1006:}procedure tryeq(l,r:halfword);label 30,31;var p:halfword;
t:16..19;q:halfword;pp:halfword;tt:17..19;copied:boolean;begin{1007:}
t:=mem[l].hh.b0;if t=16 then begin t:=17;
p:=constdependency(-mem[l+1].int);q:=p;
end else if t=19 then begin t:=17;p:=singledependency(l);
mem[p+1].int:=-mem[p+1].int;q:=depfinal;
end else begin p:=mem[l+1].hh.rh;q:=p;
while true do begin mem[q+1].int:=-mem[q+1].int;
if mem[q].hh.lh=0 then goto 30;q:=mem[q].hh.rh;end;
30:mem[mem[l+1].hh.lh].hh.rh:=mem[q].hh.rh;
mem[mem[q].hh.rh+1].hh.lh:=mem[l+1].hh.lh;mem[l].hh.b0:=16;end{:1007};
{1009:}
if r=0 then if curtype=16 then begin mem[q+1].int:=mem[q+1].int+curexp;
goto 31;end else begin tt:=curtype;
if tt=19 then pp:=singledependency(curexp)else pp:=mem[curexp+1].hh.rh;
end else if mem[r].hh.b0=16 then begin mem[q+1].int:=mem[q+1].int+mem[r
+1].int;goto 31;end else begin tt:=mem[r].hh.b0;
if tt=19 then pp:=singledependency(r)else pp:=mem[r+1].hh.rh;end;
if tt<>19 then copied:=false else begin copied:=true;tt:=17;end;{1010:}
watchcoefs:=false;
if t=tt then p:=pplusq(p,pp,t)else if t=18 then p:=pplusfq(p,65536,pp,18
,17)else begin q:=p;
while mem[q].hh.lh<>0 do begin mem[q+1].int:=roundfraction(mem[q+1].int)
;q:=mem[q].hh.rh;end;t:=18;p:=pplusq(p,pp,t);end;watchcoefs:=true;
{:1010};if copied then flushnodelist(pp);31:{:1009};
if mem[p].hh.lh=0 then{1008:}
begin if abs(mem[p+1].int)>64 then begin begin if interaction=3 then;
printnl(261);print(894);end;print(896);printscaled(mem[p+1].int);
printchar(41);begin helpptr:=2;helpline[1]:=895;helpline[0]:=893;end;
putgeterror;end else if r=0 then{623:}begin begin if interaction=3 then;
printnl(261);print(597);end;begin helpptr:=2;helpline[1]:=598;
helpline[0]:=599;end;putgeterror;end{:623};freenode(p,2);end{:1008}
else begin lineareq(p,t);
if r=0 then if curtype<>16 then if mem[curexp].hh.b0=16 then begin pp:=
curexp;curexp:=mem[curexp+1].int;curtype:=16;freenode(pp,2);end;end;end;
{:1006}{1001:}procedure makeeq(lhs:halfword);label 20,30,45;
var t:smallnumber;v:integer;p,q:halfword;begin 20:t:=mem[lhs].hh.b0;
if t<=14 then v:=mem[lhs+1].int;case t of{1003:}
2,4,6,9,11:if curtype=t+1 then begin nonlineareq(v,curexp,false);
goto 30;end else if curtype=t then{1004:}
begin if curtype<=4 then begin if curtype=4 then begin if strvsstr(v,
curexp)<>0 then goto 45;end else if v<>curexp then goto 45;{623:}
begin begin if interaction=3 then;printnl(261);print(597);end;
begin helpptr:=2;helpline[1]:=598;helpline[0]:=599;end;putgeterror;
end{:623};goto 30;end;begin if interaction=3 then;printnl(261);
print(891);end;begin helpptr:=2;helpline[1]:=892;helpline[0]:=893;end;
putgeterror;goto 30;45:begin if interaction=3 then;printnl(261);
print(894);end;begin helpptr:=2;helpline[1]:=895;helpline[0]:=893;end;
putgeterror;goto 30;end{:1004};
3,5,7,12,10:if curtype=t-1 then begin nonlineareq(curexp,lhs,true);
goto 30;end else if curtype=t then begin ringmerge(lhs,curexp);goto 30;
end else if curtype=14 then if t=10 then begin pairtopath;goto 20;end;
13,14:if curtype=t then{1005:}begin p:=v+bignodesize[t];
q:=mem[curexp+1].int+bignodesize[t];repeat p:=p-2;q:=q-2;tryeq(p,q);
until p=v;goto 30;end{:1005};
16,17,18,19:if curtype>=16 then begin tryeq(lhs,0);goto 30;end;1:;
{:1003}end;{1002:}disperr(lhs,283);disperr(0,888);
if mem[lhs].hh.b0<=14 then printtype(mem[lhs].hh.b0)else print(339);
printchar(61);if curtype<=14 then printtype(curtype)else print(339);
printchar(41);begin helpptr:=2;helpline[1]:=889;helpline[0]:=890;end;
putgeterror{:1002};30:begin if aritherror then cleararith;end;
recyclevalue(lhs);freenode(lhs,2);end;{:1001}procedure doassignment;
forward;procedure doequation;var lhs:halfword;p:halfword;
begin lhs:=stashcurexp;getxnext;varflag:=77;scanexpression;
if curcmd=51 then doequation else if curcmd=77 then doassignment;
if internal[7]>131072 then{997:}begin begindiagnostic;printnl(847);
printexp(lhs,0);print(883);printexp(0,0);print(839);
enddiagnostic(false);end{:997};
if curtype=10 then if mem[lhs].hh.b0=14 then begin p:=stashcurexp;
unstashcurexp(lhs);lhs:=p;end;makeeq(lhs);end;{:995}{996:}
procedure doassignment;var lhs:halfword;p:halfword;q:halfword;
begin if curtype<>20 then begin disperr(0,880);begin helpptr:=2;
helpline[1]:=881;helpline[0]:=882;end;error;doequation;
end else begin lhs:=curexp;curtype:=1;getxnext;varflag:=77;
scanexpression;
if curcmd=51 then doequation else if curcmd=77 then doassignment;
if internal[7]>131072 then{998:}begin begindiagnostic;printnl(123);
if mem[lhs].hh.lh>9769 then slowprint(intname[mem[lhs].hh.lh-(9769)])
else showtokenlist(lhs,0,1000,0);print(460);printexp(0,0);
printchar(125);enddiagnostic(false);end{:998};
if mem[lhs].hh.lh>9769 then{999:}
if curtype=16 then internal[mem[lhs].hh.lh-(9769)]:=curexp else begin
disperr(0,884);slowprint(intname[mem[lhs].hh.lh-(9769)]);print(885);
begin helpptr:=2;helpline[1]:=886;helpline[0]:=887;end;putgeterror;
end{:999}else{1000:}begin p:=findvariable(lhs);
if p<>0 then begin q:=stashcurexp;curtype:=undtype(p);recyclevalue(p);
mem[p].hh.b0:=curtype;mem[p+1].int:=0;makeexpcopy(p);p:=stashcurexp;
unstashcurexp(q);makeeq(p);end else begin obliterated(lhs);putgeterror;
end;end{:1000};flushnodelist(lhs);end;end;{:996}{1015:}
procedure dotypedeclaration;var t:smallnumber;p:halfword;q:halfword;
begin if curmod>=13 then t:=curmod else t:=curmod+1;
repeat p:=scandeclaredvariable;
flushvariable(eqtb[mem[p].hh.lh].rh,mem[p].hh.rh,false);
q:=findvariable(p);if q<>0 then begin mem[q].hh.b0:=t;mem[q+1].int:=0;
end else begin begin if interaction=3 then;printnl(261);print(897);end;
begin helpptr:=2;helpline[1]:=898;helpline[0]:=899;end;putgeterror;end;
flushlist(p);if curcmd<82 then{1016:}begin begin if interaction=3 then;
printnl(261);print(900);end;begin helpptr:=5;helpline[4]:=901;
helpline[3]:=902;helpline[2]:=903;helpline[1]:=904;helpline[0]:=905;end;
if curcmd=42 then helpline[2]:=906;putgeterror;scannerstatus:=2;
repeat getnext;{743:}
if curcmd=39 then begin if strref[curmod]<127 then if strref[curmod]>1
then decr(strref[curmod])else flushstring(curmod);end{:743};
until curcmd>=82;scannerstatus:=0;end{:1016};until curcmd>82;end;{:1015}
{1021:}procedure dorandomseed;begin getxnext;
if curcmd<>77 then begin missingerr(460);begin helpptr:=1;
helpline[0]:=911;end;backerror;end;getxnext;scanexpression;
if curtype<>16 then begin disperr(0,912);begin helpptr:=2;
helpline[1]:=913;helpline[0]:=914;end;putgetflusherror(0);
end else{1022:}begin initrandoms(curexp);
if selector>=2 then begin oldsetting:=selector;selector:=2;printnl(915);
printscaled(curexp);printchar(125);printnl(283);selector:=oldsetting;
end;end{:1022};end;{:1021}{1029:}procedure doprotection;var m:0..1;
t:halfword;begin m:=curmod;repeat getsymbol;t:=eqtb[cursym].lh;
if m=0 then begin if t>=86 then eqtb[cursym].lh:=t-86;
end else if t<86 then eqtb[cursym].lh:=t+86;getxnext;until curcmd<>82;
end;{:1029}{1031:}procedure defdelims;var ldelim,rdelim:halfword;
begin getclearsymbol;ldelim:=cursym;getclearsymbol;rdelim:=cursym;
eqtb[ldelim].lh:=31;eqtb[ldelim].rh:=rdelim;eqtb[rdelim].lh:=62;
eqtb[rdelim].rh:=ldelim;getxnext;end;{:1031}{1034:}
procedure dostatement;forward;procedure dointerim;begin getxnext;
if curcmd<>40 then begin begin if interaction=3 then;printnl(261);
print(921);end;
if cursym=0 then print(926)else slowprint(hash[cursym].rh);print(927);
begin helpptr:=1;helpline[0]:=928;end;backerror;
end else begin saveinternal(curmod);backinput;end;dostatement;end;
{:1034}{1035:}procedure dolet;var l:halfword;begin getsymbol;l:=cursym;
getxnext;if curcmd<>51 then if curcmd<>77 then begin missingerr(61);
begin helpptr:=3;helpline[2]:=929;helpline[1]:=670;helpline[0]:=930;end;
backerror;end;getsymbol;
case curcmd of 10,53,44,49:incr(mem[curmod].hh.lh);others:end;
clearsymbol(l,false);eqtb[l].lh:=curcmd;
if curcmd=41 then eqtb[l].rh:=0 else eqtb[l].rh:=curmod;getxnext;end;
{:1035}{1036:}procedure donewinternal;
begin repeat if intptr=maxinternal then overflow(931,maxinternal);
getclearsymbol;incr(intptr);eqtb[cursym].lh:=40;eqtb[cursym].rh:=intptr;
intname[intptr]:=hash[cursym].rh;internal[intptr]:=0;getxnext;
until curcmd<>82;end;{:1036}{1040:}procedure doshow;
begin repeat getxnext;scanexpression;printnl(762);printexp(0,2);
flushcurexp(0);until curcmd<>82;end;{:1040}{1041:}procedure disptoken;
begin printnl(937);if cursym=0 then{1042:}
begin if curcmd=42 then printscaled(curmod)else if curcmd=38 then begin
gpointer:=curmod;printcapsule;end else begin printchar(34);
slowprint(curmod);printchar(34);
begin if strref[curmod]<127 then if strref[curmod]>1 then decr(strref[
curmod])else flushstring(curmod);end;end;end{:1042}
else begin slowprint(hash[cursym].rh);printchar(61);
if eqtb[cursym].lh>=86 then print(938);printcmdmod(curcmd,curmod);
if curcmd=10 then begin println;showmacro(curmod,0,100000);end;end;end;
{:1041}{1044:}procedure doshowtoken;begin repeat getnext;disptoken;
getxnext;until curcmd<>82;end;{:1044}{1045:}procedure doshowstats;
begin printnl(947);ifdef('STAT')printint(varused);printchar(38);
printint(dynused);if false then endif('STAT')print(356);print(557);
printint(himemmin-lomemmax-1);print(948);println;printnl(949);
printint(strptr-initstrptr);printchar(38);printint(poolptr-initpoolptr);
print(557);printint(maxstrings-maxstrptr);printchar(38);
printint(poolsize-maxpoolptr);print(948);println;getxnext;end;{:1045}
{1046:}procedure dispvar(p:halfword);var q:halfword;n:0..maxprintline;
begin if mem[p].hh.b0=21 then{1047:}begin q:=mem[p+1].hh.lh;
repeat dispvar(q);q:=mem[q].hh.rh;until q=17;q:=mem[p+1].hh.rh;
while mem[q].hh.b1=3 do begin dispvar(q);q:=mem[q].hh.rh;end;end{:1047}
else if mem[p].hh.b0>=22 then{1048:}begin printnl(283);
printvariablename(p);if mem[p].hh.b0>22 then print(662);print(950);
if fileoffset>=maxprintline-20 then n:=5 else n:=maxprintline-fileoffset
-15;showmacro(mem[p+1].int,0,n);end{:1048}
else if mem[p].hh.b0<>0 then begin printnl(283);printvariablename(p);
printchar(61);printexp(p,0);end;end;{:1046}{1049:}procedure doshowvar;
label 30;begin repeat getnext;
if cursym>0 then if cursym<=9769 then if curcmd=41 then if curmod<>0
then begin dispvar(curmod);goto 30;end;disptoken;30:getxnext;
until curcmd<>82;end;{:1049}{1050:}procedure doshowdependencies;
var p:halfword;begin p:=mem[13].hh.rh;
while p<>13 do begin if interesting(p)then begin printnl(283);
printvariablename(p);
if mem[p].hh.b0=17 then printchar(61)else print(765);
printdependency(mem[p+1].hh.rh,mem[p].hh.b0);end;p:=mem[p+1].hh.rh;
while mem[p].hh.lh<>0 do p:=mem[p].hh.rh;p:=mem[p].hh.rh;end;getxnext;
end;{:1050}{1051:}procedure doshowwhatever;begin if interaction=3 then;
case curmod of 0:doshowtoken;1:doshowstats;2:doshow;3:doshowvar;
4:doshowdependencies;end;
if internal[32]>0 then begin begin if interaction=3 then;printnl(261);
print(951);end;if interaction<3 then begin helpptr:=0;decr(errorcount);
end else begin helpptr:=1;helpline[0]:=952;end;
if curcmd=83 then error else putgeterror;end;end;{:1051}{1054:}
function scanwith:boolean;var t:smallnumber;result:boolean;
begin t:=curmod;curtype:=1;getxnext;scanexpression;result:=false;
if curtype<>t then{1055:}begin disperr(0,960);begin helpptr:=2;
helpline[1]:=961;helpline[0]:=962;end;if t=6 then helpline[1]:=963;
putgetflusherror(0);end{:1055}
else if curtype=6 then result:=true else{1056:}
begin curexp:=roundunscaled(curexp);
if(abs(curexp)<4)and(curexp<>0)then result:=true else begin begin if
interaction=3 then;printnl(261);print(964);end;begin helpptr:=1;
helpline[0]:=962;end;putgetflusherror(0);end;end{:1056};
scanwith:=result;end;{:1054}{1057:}procedure findedgesvar(t:halfword);
var p:halfword;begin p:=findvariable(t);curedges:=0;
if p=0 then begin obliterated(t);putgeterror;
end else if mem[p].hh.b0<>11 then begin begin if interaction=3 then;
printnl(261);print(787);end;showtokenlist(t,0,1000,0);print(965);
printtype(mem[p].hh.b0);printchar(41);begin helpptr:=2;helpline[1]:=966;
helpline[0]:=967;end;putgeterror;end else curedges:=mem[p+1].int;
flushnodelist(t);end;{:1057}{1059:}procedure doaddto;label 30,45;
var lhs,rhs:halfword;w:integer;p:halfword;q:halfword;addtotype:0..2;
begin getxnext;varflag:=68;scanprimary;if curtype<>20 then{1060:}
begin disperr(0,968);begin helpptr:=4;helpline[3]:=969;helpline[2]:=970;
helpline[1]:=971;helpline[0]:=967;end;putgetflusherror(0);end{:1060}
else begin lhs:=curexp;addtotype:=curmod;curtype:=1;getxnext;
scanexpression;if addtotype=2 then{1061:}begin findedgesvar(lhs);
if curedges=0 then flushcurexp(0)else if curtype<>11 then begin disperr(
0,972);begin helpptr:=2;helpline[1]:=973;helpline[0]:=967;end;
putgetflusherror(0);end else begin mergeedges(curexp);flushcurexp(0);
end;end{:1061}else{1062:}begin if curtype=14 then pairtopath;
if curtype<>9 then begin disperr(0,972);begin helpptr:=2;
helpline[1]:=974;helpline[0]:=967;end;putgetflusherror(0);
flushtokenlist(lhs);end else begin rhs:=curexp;w:=1;curpen:=3;
while curcmd=66 do if scanwith then if curtype=16 then w:=curexp else{
1063:}
begin if mem[curpen].hh.lh=0 then tosspen(curpen)else decr(mem[curpen].
hh.lh);curpen:=curexp;end{:1063};{1064:}findedgesvar(lhs);
if curedges=0 then tossknotlist(rhs)else begin lhs:=0;
curpathtype:=addtotype;
if mem[rhs].hh.b0=0 then if curpathtype=0 then{1065:}
if mem[rhs].hh.rh=rhs then{1066:}begin mem[rhs+5].int:=mem[rhs+1].int;
mem[rhs+6].int:=mem[rhs+2].int;mem[rhs+3].int:=mem[rhs+1].int;
mem[rhs+4].int:=mem[rhs+2].int;mem[rhs].hh.b0:=1;mem[rhs].hh.b1:=1;
end{:1066}else begin p:=htapypoc(rhs);q:=mem[p].hh.rh;
mem[pathtail+5].int:=mem[q+5].int;mem[pathtail+6].int:=mem[q+6].int;
mem[pathtail].hh.b1:=mem[q].hh.b1;mem[pathtail].hh.rh:=mem[q].hh.rh;
freenode(q,7);mem[p+5].int:=mem[rhs+5].int;mem[p+6].int:=mem[rhs+6].int;
mem[p].hh.b1:=mem[rhs].hh.b1;mem[p].hh.rh:=mem[rhs].hh.rh;
freenode(rhs,7);rhs:=p;end{:1065}else{1067:}
begin begin if interaction=3 then;printnl(261);print(975);end;
begin helpptr:=2;helpline[1]:=976;helpline[0]:=967;end;putgeterror;
tossknotlist(rhs);goto 45;end{:1067}
else if curpathtype=0 then lhs:=htapypoc(rhs);curwt:=w;
rhs:=makespec(rhs,mem[curpen+9].int,internal[5]);{1068:}
if turningnumber<=0 then if curpathtype<>0 then if internal[39]>0 then
if(turningnumber<0)and(mem[curpen].hh.rh=0)then curwt:=-curwt else begin
if turningnumber=0 then if(internal[39]<=65536)and(mem[curpen].hh.rh=0)
then goto 30 else printstrange(977)else printstrange(978);
begin helpptr:=3;helpline[2]:=979;helpline[1]:=980;helpline[0]:=981;end;
putgeterror;end;30:{:1068};
if mem[curpen+9].int=0 then fillspec(rhs)else fillenvelope(rhs);
if lhs<>0 then begin revturns:=true;
lhs:=makespec(lhs,mem[curpen+9].int,internal[5]);revturns:=false;
if mem[curpen+9].int=0 then fillspec(lhs)else fillenvelope(lhs);end;
45:end{:1064};
if mem[curpen].hh.lh=0 then tosspen(curpen)else decr(mem[curpen].hh.lh);
end;end{:1062};end;end;{:1059}{1070:}{1098:}
function tfmcheck(m:smallnumber):scaled;
begin if abs(internal[m])>=134217728 then begin begin if interaction=3
then;printnl(261);print(998);end;print(intname[m]);print(999);
begin helpptr:=1;helpline[0]:=1000;end;putgeterror;
if internal[m]>0 then tfmcheck:=134217727 else tfmcheck:=-134217727;
end else tfmcheck:=internal[m];end;{:1098}procedure doshipout;label 10;
var c:integer;begin getxnext;varflag:=83;scanexpression;
if curtype<>20 then if curtype=11 then curedges:=curexp else begin{1060:
}begin disperr(0,968);begin helpptr:=4;helpline[3]:=969;
helpline[2]:=970;helpline[1]:=971;helpline[0]:=967;end;
putgetflusherror(0);end{:1060};goto 10;
end else begin findedgesvar(curexp);curtype:=1;end;
if curedges<>0 then begin c:=roundunscaled(internal[18])mod 256;
if c<0 then c:=c+256;{1099:}if c<bc then bc:=c;if c>ec then ec:=c;
charexists[c]:=true;gfdx[c]:=internal[24];gfdy[c]:=internal[25];
tfmwidth[c]:=tfmcheck(20);tfmheight[c]:=tfmcheck(21);
tfmdepth[c]:=tfmcheck(22);tfmitalcorr[c]:=tfmcheck(23){:1099};
if internal[34]>=0 then shipout(c);end;flushcurexp(0);10:end;{:1070}
{1071:}procedure dodisplay;label 45,50,10;var e:halfword;begin getxnext;
varflag:=73;scanprimary;if curtype<>20 then{1060:}begin disperr(0,968);
begin helpptr:=4;helpline[3]:=969;helpline[2]:=970;helpline[1]:=971;
helpline[0]:=967;end;putgetflusherror(0);end{:1060}else begin e:=curexp;
curtype:=1;getxnext;scanexpression;if curtype<>16 then goto 50;
curexp:=roundunscaled(curexp);if curexp<0 then goto 45;
if curexp>15 then goto 45;if not windowopen[curexp]then goto 45;
findedgesvar(e);if curedges<>0 then dispedges(curexp);goto 10;
45:curexp:=curexp*65536;50:disperr(0,982);begin helpptr:=1;
helpline[0]:=983;end;putgetflusherror(0);flushtokenlist(e);end;10:end;
{:1071}{1072:}function getpair(c:commandcode):boolean;var p:halfword;
b:boolean;begin if curcmd<>c then getpair:=false else begin getxnext;
scanexpression;
if nicepair(curexp,curtype)then begin p:=mem[curexp+1].int;
curx:=mem[p+1].int;cury:=mem[p+3].int;b:=true;end else b:=false;
flushcurexp(0);getpair:=b;end;end;{:1072}{1073:}procedure doopenwindow;
label 45,10;var k:integer;r0,c0,r1,c1:scaled;begin getxnext;
scanexpression;if curtype<>16 then goto 45;k:=roundunscaled(curexp);
if k<0 then goto 45;if k>15 then goto 45;if not getpair(70)then goto 45;
r0:=curx;c0:=cury;if not getpair(71)then goto 45;r1:=curx;c1:=cury;
if not getpair(72)then goto 45;openawindow(k,r0,c0,r1,c1,curx,cury);
goto 10;45:begin if interaction=3 then;printnl(261);print(984);end;
begin helpptr:=2;helpline[1]:=985;helpline[0]:=986;end;putgeterror;
10:end;{:1073}{1074:}procedure docull;label 45,10;var e:halfword;
keeping:0..1;w,win,wout:integer;begin w:=1;getxnext;varflag:=67;
scanprimary;if curtype<>20 then{1060:}begin disperr(0,968);
begin helpptr:=4;helpline[3]:=969;helpline[2]:=970;helpline[1]:=971;
helpline[0]:=967;end;putgetflusherror(0);end{:1060}else begin e:=curexp;
curtype:=1;keeping:=curmod;if not getpair(67)then goto 45;
while(curcmd=66)and(curmod=16)do if scanwith then w:=curexp;{1075:}
if curx>cury then goto 45;
if keeping=0 then begin if(curx>0)or(cury<0)then goto 45;wout:=w;win:=0;
end else begin if(curx<=0)and(cury>=0)then goto 45;wout:=0;win:=w;
end{:1075};findedgesvar(e);
if curedges<>0 then culledges(floorunscaled(curx+65535),floorunscaled(
cury),wout,win);goto 10;45:begin if interaction=3 then;printnl(261);
print(987);end;begin helpptr:=1;helpline[0]:=988;end;putgeterror;
flushtokenlist(e);end;10:end;{:1074}{1082:}procedure domessage;
var m:0..2;begin m:=curmod;getxnext;scanexpression;
if curtype<>4 then begin disperr(0,697);begin helpptr:=1;
helpline[0]:=992;end;putgeterror;
end else case m of 0:begin printnl(283);slowprint(curexp);end;1:{1086:}
begin begin if interaction=3 then;printnl(261);print(283);end;
slowprint(curexp);
if errhelp<>0 then useerrhelp:=true else if longhelpseen then begin
helpptr:=1;helpline[0]:=993;
end else begin if interaction<3 then longhelpseen:=true;
begin helpptr:=4;helpline[3]:=994;helpline[2]:=995;helpline[1]:=996;
helpline[0]:=997;end;end;putgeterror;useerrhelp:=false;end{:1086};
2:{1083:}
begin if errhelp<>0 then begin if strref[errhelp]<127 then if strref[
errhelp]>1 then decr(strref[errhelp])else flushstring(errhelp);end;
if(strstart[curexp+1]-strstart[curexp])=0 then errhelp:=0 else begin
errhelp:=curexp;begin if strref[errhelp]<127 then incr(strref[errhelp]);
end;end;end{:1083};end;flushcurexp(0);end;{:1082}{1103:}
function getcode:eightbits;label 40;var c:integer;begin getxnext;
scanexpression;if curtype=16 then begin c:=roundunscaled(curexp);
if c>=0 then if c<256 then goto 40;
end else if curtype=4 then if(strstart[curexp+1]-strstart[curexp])=1
then begin c:=strpool[strstart[curexp]];goto 40;end;disperr(0,1006);
begin helpptr:=2;helpline[1]:=1007;helpline[0]:=1008;end;
putgetflusherror(0);c:=0;40:getcode:=c;end;{:1103}{1104:}
procedure settag(c:halfword;t:smallnumber;r:halfword);
begin if chartag[c]=0 then begin chartag[c]:=t;charremainder[c]:=r;
if t=1 then begin incr(labelptr);labelloc[labelptr]:=r;
labelchar[labelptr]:=c;end;end else{1105:}
begin begin if interaction=3 then;printnl(261);print(1009);end;
if(c>32)and(c<127)then print(c)else if c=256 then print(1010)else begin
print(1011);printint(c);end;print(1012);case chartag[c]of 1:print(1013);
2:print(1014);3:print(1003);end;begin helpptr:=2;helpline[1]:=1015;
helpline[0]:=967;end;putgeterror;end{:1105};end;{:1104}{1106:}
procedure dotfmcommand;label 22,30;var c,cc:0..256;k:0..maxkerns;
j:integer;begin case curmod of 0:begin c:=getcode;
while curcmd=81 do begin cc:=getcode;settag(c,2,cc);c:=cc;end;end;
1:{1107:}begin lkstarted:=false;22:getxnext;
if(curcmd=78)and lkstarted then{1110:}begin c:=getcode;
if nl-skiptable[c]>128 then begin begin begin if interaction=3 then;
printnl(261);print(1032);end;begin helpptr:=1;helpline[0]:=1033;end;
error;ll:=skiptable[c];repeat lll:=ligkern[ll].b0;ligkern[ll].b0:=128;
ll:=ll-lll;until lll=0;end;skiptable[c]:=ligtablesize;end;
if skiptable[c]=ligtablesize then ligkern[nl-1].b0:=0 else ligkern[nl-1]
.b0:=nl-skiptable[c]-1;skiptable[c]:=nl-1;goto 30;end{:1110};
if curcmd=79 then begin c:=256;curcmd:=81;end else begin backinput;
c:=getcode;end;if(curcmd=81)or(curcmd=80)then{1111:}
begin if curcmd=81 then if c=256 then bchlabel:=nl else settag(c,1,nl)
else if skiptable[c]<ligtablesize then begin ll:=skiptable[c];
skiptable[c]:=ligtablesize;repeat lll:=ligkern[ll].b0;
if nl-ll>128 then begin begin begin if interaction=3 then;printnl(261);
print(1032);end;begin helpptr:=1;helpline[0]:=1033;end;error;ll:=ll;
repeat lll:=ligkern[ll].b0;ligkern[ll].b0:=128;ll:=ll-lll;until lll=0;
end;goto 22;end;ligkern[ll].b0:=nl-ll-1;ll:=ll-lll;until lll=0;end;
goto 22;end{:1111};if curcmd=76 then{1112:}begin ligkern[nl].b1:=c;
ligkern[nl].b0:=0;if curmod<128 then begin ligkern[nl].b2:=curmod;
ligkern[nl].b3:=getcode;end else begin getxnext;scanexpression;
if curtype<>16 then begin disperr(0,1034);begin helpptr:=2;
helpline[1]:=1035;helpline[0]:=307;end;putgetflusherror(0);end;
kern[nk]:=curexp;k:=0;while kern[k]<>curexp do incr(k);
if k=nk then begin if nk=maxkerns then overflow(1031,maxkerns);incr(nk);
end;ligkern[nl].b2:=128+(k div 256);ligkern[nl].b3:=(k mod 256);end;
lkstarted:=true;end{:1112}else begin begin if interaction=3 then;
printnl(261);print(1020);end;begin helpptr:=1;helpline[0]:=1021;end;
backerror;ligkern[nl].b1:=0;ligkern[nl].b2:=0;ligkern[nl].b3:=0;
ligkern[nl].b0:=129;end;
if nl=ligtablesize then overflow(1022,ligtablesize);incr(nl);
if curcmd=82 then goto 22;
if ligkern[nl-1].b0<128 then ligkern[nl-1].b0:=128;30:end{:1107};
2:{1113:}begin if ne=256 then overflow(1003,256);c:=getcode;
settag(c,3,ne);if curcmd<>81 then begin missingerr(58);begin helpptr:=1;
helpline[0]:=1036;end;backerror;end;exten[ne].b0:=getcode;
if curcmd<>82 then begin missingerr(44);begin helpptr:=1;
helpline[0]:=1036;end;backerror;end;exten[ne].b1:=getcode;
if curcmd<>82 then begin missingerr(44);begin helpptr:=1;
helpline[0]:=1036;end;backerror;end;exten[ne].b2:=getcode;
if curcmd<>82 then begin missingerr(44);begin helpptr:=1;
helpline[0]:=1036;end;backerror;end;exten[ne].b3:=getcode;incr(ne);
end{:1113};3,4:begin c:=curmod;getxnext;scanexpression;
if(curtype<>16)or(curexp<32768)then begin disperr(0,1016);
begin helpptr:=2;helpline[1]:=1017;helpline[0]:=1018;end;putgeterror;
end else begin j:=roundunscaled(curexp);
if curcmd<>81 then begin missingerr(58);begin helpptr:=1;
helpline[0]:=1019;end;backerror;end;if c=3 then{1114:}
repeat if j>headersize then overflow(1004,headersize);
headerbyte[j]:=getcode;incr(j);until curcmd<>82{:1114}else{1115:}
repeat if j>maxfontdimen then overflow(1005,maxfontdimen);
while j>np do begin incr(np);param[np]:=0;end;getxnext;scanexpression;
if curtype<>16 then begin disperr(0,1037);begin helpptr:=1;
helpline[0]:=307;end;putgetflusherror(0);end;param[j]:=curexp;incr(j);
until curcmd<>82{:1115};end;end;end;end;{:1106}{1177:}
procedure dospecial;var m:smallnumber;begin m:=curmod;getxnext;
scanexpression;if internal[34]>=0 then if curtype<>m then{1178:}
begin disperr(0,1057);begin helpptr:=1;helpline[0]:=1058;end;
putgeterror;end{:1178}else begin if outputfilename=0 then initgf;
if m=4 then gfstring(curexp,0)else begin begin gfbuf[gfptr]:=243;
incr(gfptr);if gfptr=gflimit then gfswap;end;gffour(curexp);end;end;
flushcurexp(0);end;{:1177}{1186:}ifdef('INIMF')procedure storebasefile;
var k:integer;p,q:halfword;x:integer;w:fourquarters;begin{1200:}
selector:=5;print(1068);print(jobname);printchar(32);
printint(roundunscaled(internal[14])mod 100);printchar(46);
printint(roundunscaled(internal[15]));printchar(46);
printint(roundunscaled(internal[16]));printchar(41);
if interaction=0 then selector:=2 else selector:=3;
begin if poolptr+1>maxpoolptr then begin if poolptr+1>poolsize then
overflow(257,poolsize-initpoolptr);maxpoolptr:=poolptr+1;end;end;
baseident:=makestring;strref[baseident]:=127;packjobname(739);
while not wopenout(basefile)do promptfilename(1069,739);printnl(1070);
slowprint(wmakenamestring(basefile));flushstring(strptr-1);printnl(283);
slowprint(baseident){:1200};{1190:}dumpint(426472440);dumpint(0);
dumpint(memtop);dumpint(9500);dumpint(7919);dumpint(15){:1190};{1192:}
dumpint(poolptr);dumpint(strptr);
for k:=0 to strptr do dumpint(strstart[k]);k:=0;
while k+4<poolptr do begin w.b0:=strpool[k];w.b1:=strpool[k+1];
w.b2:=strpool[k+2];w.b3:=strpool[k+3];dumpqqqq(w);k:=k+4;end;
k:=poolptr-4;w.b0:=strpool[k];w.b1:=strpool[k+1];w.b2:=strpool[k+2];
w.b3:=strpool[k+3];dumpqqqq(w);println;printint(strptr);print(1065);
printint(poolptr){:1192};{1194:}sortavail;varused:=0;dumpint(lomemmax);
dumpint(rover);p:=0;q:=rover;x:=0;
repeat for k:=p to q+1 do dumpwd(mem[k]);x:=x+q+2-p;
varused:=varused+q-p;p:=q+mem[q].hh.lh;q:=mem[q+1].hh.rh;until q=rover;
varused:=varused+lomemmax-p;dynused:=memend+1-himemmin;
for k:=p to lomemmax do dumpwd(mem[k]);x:=x+lomemmax+1-p;
dumpint(himemmin);dumpint(avail);
for k:=himemmin to memend do dumpwd(mem[k]);x:=x+memend+1-himemmin;
p:=avail;while p<>0 do begin decr(dynused);p:=mem[p].hh.rh;end;
dumpint(varused);dumpint(dynused);println;printint(x);print(1066);
printint(varused);printchar(38);printint(dynused){:1194};{1196:}
dumpint(hashused);stcount:=9756-hashused;
for p:=1 to hashused do if hash[p].rh<>0 then begin dumpint(p);
dumphh(hash[p]);dumphh(eqtb[p]);incr(stcount);end;
for p:=hashused+1 to 9769 do begin dumphh(hash[p]);dumphh(eqtb[p]);end;
dumpint(stcount);println;printint(stcount);print(1067){:1196};{1198:}
dumpint(intptr);for k:=1 to intptr do begin dumpint(internal[k]);
dumpint(intname[k]);end;dumpint(startsym);dumpint(interaction);
dumpint(baseident);dumpint(bgloc);dumpint(egloc);dumpint(serialno);
dumpint(69069);internal[12]:=0{:1198};{1201:}wclose(basefile){:1201};
end;endif('INIMF'){:1186}procedure dostatement;begin curtype:=1;
getxnext;if curcmd>43 then{990:}
begin if curcmd<83 then begin begin if interaction=3 then;printnl(261);
print(866);end;printcmdmod(curcmd,curmod);printchar(39);
begin helpptr:=5;helpline[4]:=867;helpline[3]:=868;helpline[2]:=869;
helpline[1]:=870;helpline[0]:=871;end;backerror;getxnext;end;end{:990}
else if curcmd>30 then{993:}begin varflag:=77;scanexpression;
if curcmd<84 then begin if curcmd=51 then doequation else if curcmd=77
then doassignment else if curtype=4 then{994:}
begin if internal[1]>0 then begin printnl(283);slowprint(curexp);
flush(stdout);end;if internal[34]>0 then{1179:}
begin if outputfilename=0 then initgf;gfstring(1059,curexp);end{:1179};
end{:994}else if curtype<>1 then begin disperr(0,876);begin helpptr:=3;
helpline[2]:=877;helpline[1]:=878;helpline[0]:=879;end;putgeterror;end;
flushcurexp(0);curtype:=1;end;end{:993}else{992:}
begin if internal[7]>0 then showcmdmod(curcmd,curmod);
case curcmd of 30:dotypedeclaration;
16:if curmod>2 then makeopdef else if curmod>0 then scandef;{1020:}
24:dorandomseed;{:1020}{1023:}23:begin println;interaction:=curmod;{70:}
if interaction=0 then selector:=0 else selector:=1{:70};
if logopened then selector:=selector+2;getxnext;end;{:1023}{1026:}
21:doprotection;{:1026}{1030:}27:defdelims;{:1030}{1033:}
12:repeat getsymbol;savevariable(cursym);getxnext;until curcmd<>82;
13:dointerim;14:dolet;15:donewinternal;{:1033}{1039:}22:doshowwhatever;
{:1039}{1058:}18:doaddto;{:1058}{1069:}17:doshipout;11:dodisplay;
28:doopenwindow;19:docull;{:1069}{1076:}26:begin getsymbol;
startsym:=cursym;getxnext;end;{:1076}{1081:}25:domessage;{:1081}{1100:}
20:dotfmcommand;{:1100}{1175:}29:dospecial;{:1175}end;curtype:=1;
end{:992};if curcmd<83 then{991:}begin begin if interaction=3 then;
printnl(261);print(872);end;begin helpptr:=6;helpline[5]:=873;
helpline[4]:=874;helpline[3]:=875;helpline[2]:=869;helpline[1]:=870;
helpline[0]:=871;end;backerror;scannerstatus:=2;repeat getnext;{743:}
if curcmd=39 then begin if strref[curmod]<127 then if strref[curmod]>1
then decr(strref[curmod])else flushstring(curmod);end{:743};
until curcmd>82;scannerstatus:=0;end{:991};errorcount:=0;end;{:989}
{1017:}procedure maincontrol;begin repeat dostatement;
if curcmd=84 then begin begin if interaction=3 then;printnl(261);
print(907);end;begin helpptr:=2;helpline[1]:=908;helpline[0]:=687;end;
flusherror(0);end;until curcmd=85;end;{:1017}{1117:}
function sortin(v:scaled):halfword;label 40;var p,q,r:halfword;
begin p:=memtop-1;while true do begin q:=mem[p].hh.rh;
if v<=mem[q+1].int then goto 40;p:=q;end;
40:if v<mem[q+1].int then begin r:=getnode(2);mem[r+1].int:=v;
mem[r].hh.rh:=q;mem[p].hh.rh:=r;end;sortin:=mem[p].hh.rh;end;{:1117}
{1118:}function mincover(d:scaled):integer;var p:halfword;l:scaled;
m:integer;begin m:=0;p:=mem[memtop-1].hh.rh;perturbation:=2147483647;
while p<>19 do begin incr(m);l:=mem[p+1].int;repeat p:=mem[p].hh.rh;
until mem[p+1].int>l+d;
if mem[p+1].int-l<perturbation then perturbation:=mem[p+1].int-l;end;
mincover:=m;end;{:1118}{1120:}function thresholdfn(m:integer):scaled;
var d:scaled;begin excess:=mincover(0)-m;
if excess<=0 then thresholdfn:=0 else begin repeat d:=perturbation;
until mincover(d+d)<=m;while mincover(d)>m do d:=perturbation;
thresholdfn:=d;end;end;{:1120}{1121:}function skimp(m:integer):integer;
var d:scaled;p,q,r:halfword;l:scaled;v:scaled;begin d:=thresholdfn(m);
perturbation:=0;q:=memtop-1;m:=0;p:=mem[memtop-1].hh.rh;
while p<>19 do begin incr(m);l:=mem[p+1].int;mem[p].hh.lh:=m;
if mem[mem[p].hh.rh+1].int<=l+d then{1122:}begin repeat p:=mem[p].hh.rh;
mem[p].hh.lh:=m;decr(excess);if excess=0 then d:=0;
until mem[mem[p].hh.rh+1].int>l+d;v:=l+(mem[p+1].int-l)div 2;
if mem[p+1].int-v>perturbation then perturbation:=mem[p+1].int-v;r:=q;
repeat r:=mem[r].hh.rh;mem[r+1].int:=v;until r=p;mem[q].hh.rh:=p;
end{:1122};q:=p;p:=mem[p].hh.rh;end;skimp:=m;end;{:1121}{1123:}
procedure tfmwarning(m:smallnumber);begin printnl(1038);
print(intname[m]);print(1039);printscaled(perturbation);print(1040);end;
{:1123}{1128:}procedure fixdesignsize;var d:scaled;
begin d:=internal[26];
if(d<65536)or(d>=134217728)then begin if d<>0 then printnl(1041);
d:=8388608;internal[26]:=d;end;
if headerbyte[5]<0 then if headerbyte[6]<0 then if headerbyte[7]<0 then
if headerbyte[8]<0 then begin headerbyte[5]:=d div 1048576;
headerbyte[6]:=(d div 4096)mod 256;headerbyte[7]:=(d div 16)mod 256;
headerbyte[8]:=(d mod 16)*16;end;
maxtfmdimen:=16*internal[26]-internal[26]div 2097152;
if maxtfmdimen>=134217728 then maxtfmdimen:=134217727;end;{:1128}{1129:}
function dimenout(x:scaled):integer;
begin if abs(x)>maxtfmdimen then begin incr(tfmchanged);
if x>0 then x:=16777215 else x:=-16777215;
end else x:=makescaled(x*16,internal[26]);dimenout:=x;end;{:1129}{1131:}
procedure fixchecksum;label 10;var k:eightbits;b1,b2,b3,b4:eightbits;
x:integer;
begin if headerbyte[1]<0 then if headerbyte[2]<0 then if headerbyte[3]<0
then if headerbyte[4]<0 then begin{1132:}b1:=bc;b2:=ec;b3:=bc;b4:=ec;
tfmchanged:=0;
for k:=bc to ec do if charexists[k]then begin x:=dimenout(mem[tfmwidth[k
]+1].int)+(k+4)*4194304;b1:=(b1+b1+x)mod 255;b2:=(b2+b2+x)mod 253;
b3:=(b3+b3+x)mod 251;b4:=(b4+b4+x)mod 247;end{:1132};headerbyte[1]:=b1;
headerbyte[2]:=b2;headerbyte[3]:=b3;headerbyte[4]:=b4;goto 10;end;
for k:=1 to 4 do if headerbyte[k]<0 then headerbyte[k]:=0;10:end;{:1131}
{1133:}procedure tfmqqqq(x:fourquarters);begin bwritebyte(tfmfile,x.b0);
bwritebyte(tfmfile,x.b1);bwritebyte(tfmfile,x.b2);
bwritebyte(tfmfile,x.b3);end;{:1133}{1187:}{779:}
function openbasefile:boolean;label 40,10;var j:0..bufsize;
begin j:=curinput.locfield;
if buffer[curinput.locfield]=38 then begin incr(curinput.locfield);
j:=curinput.locfield;buffer[last]:=32;while buffer[j]<>32 do incr(j);
packbufferedname(0,curinput.locfield,j-1);
if wopenin(basefile)then goto 40;;
writeln(stdout,'Sorry, I can''t find that base;',
' will try the default.');flush(stdout);end;
packbufferedname(basedefaultlength-5,1,0);
if not wopenin(basefile)then begin;
writeln(stdout,'I can''t find the default base file!');
openbasefile:=false;goto 10;end;40:curinput.locfield:=j;
openbasefile:=true;10:end;{:779}function loadbasefile:boolean;
label 6666,10;var k:integer;p,q:halfword;x:integer;w:fourquarters;
begin{1191:}undumpint(x);if x<>426472440 then goto 6666;undumpint(x);
if x<>0 then goto 6666;undumpint(x);if x<>memtop then goto 6666;
undumpint(x);if x<>9500 then goto 6666;undumpint(x);
if x<>7919 then goto 6666;undumpint(x);if x<>15 then goto 6666{:1191};
{1193:}begin undumpint(x);if x<0 then goto 6666;
if x>poolsize then begin;
writeln(stdout,'---! Must increase the ','string pool size');goto 6666;
end else poolptr:=x;end;begin undumpint(x);if x<0 then goto 6666;
if x>maxstrings then begin;
writeln(stdout,'---! Must increase the ','max strings');goto 6666;
end else strptr:=x;end;for k:=0 to strptr do begin begin undumpint(x);
if(x<0)or(x>poolptr)then goto 6666 else strstart[k]:=x;end;
strref[k]:=127;end;k:=0;while k+4<poolptr do begin undumpqqqq(w);
strpool[k]:=w.b0;strpool[k+1]:=w.b1;strpool[k+2]:=w.b2;
strpool[k+3]:=w.b3;k:=k+4;end;k:=poolptr-4;undumpqqqq(w);
strpool[k]:=w.b0;strpool[k+1]:=w.b1;strpool[k+2]:=w.b2;
strpool[k+3]:=w.b3;initstrptr:=strptr;initpoolptr:=poolptr;
maxstrptr:=strptr;maxpoolptr:=poolptr{:1193};{1195:}begin undumpint(x);
if(x<1022)or(x>memtop-3)then goto 6666 else lomemmax:=x;end;
begin undumpint(x);if(x<23)or(x>lomemmax)then goto 6666 else rover:=x;
end;p:=0;q:=rover;repeat for k:=p to q+1 do undumpwd(mem[k]);
p:=q+mem[q].hh.lh;
if(p>lomemmax)or((q>=mem[q+1].hh.rh)and(mem[q+1].hh.rh<>rover))then goto
6666;q:=mem[q+1].hh.rh;until q=rover;
for k:=p to lomemmax do undumpwd(mem[k]);begin undumpint(x);
if(x<lomemmax+1)or(x>memtop-2)then goto 6666 else himemmin:=x;end;
begin undumpint(x);if(x<0)or(x>memtop)then goto 6666 else avail:=x;end;
memend:=memtop;for k:=himemmin to memend do undumpwd(mem[k]);
undumpint(varused);undumpint(dynused){:1195};{1197:}begin undumpint(x);
if(x<1)or(x>9757)then goto 6666 else hashused:=x;end;p:=0;
repeat begin undumpint(x);
if(x<p+1)or(x>hashused)then goto 6666 else p:=x;end;undumphh(hash[p]);
undumphh(eqtb[p]);until p=hashused;
for p:=hashused+1 to 9769 do begin undumphh(hash[p]);undumphh(eqtb[p]);
end;undumpint(stcount){:1197};{1199:}begin undumpint(x);
if(x<41)or(x>maxinternal)then goto 6666 else intptr:=x;end;
for k:=1 to intptr do begin undumpint(internal[k]);begin undumpint(x);
if(x<0)or(x>strptr)then goto 6666 else intname[k]:=x;end;end;
begin undumpint(x);if(x<0)or(x>9757)then goto 6666 else startsym:=x;end;
begin undumpint(x);if(x<0)or(x>3)then goto 6666 else interaction:=x;end;
begin undumpint(x);if(x<0)or(x>strptr)then goto 6666 else baseident:=x;
end;begin undumpint(x);if(x<1)or(x>9769)then goto 6666 else bgloc:=x;
end;begin undumpint(x);if(x<1)or(x>9769)then goto 6666 else egloc:=x;
end;undumpint(serialno);undumpint(x);
if(x<>69069)or feof(basefile)then goto 6666{:1199};loadbasefile:=true;
goto 10;6666:;writeln(stdout,'(Fatal base file error; I''m stymied)');
loadbasefile:=false;10:end;{:1187}{1202:}{823:}procedure scanprimary;
label 20,30,31,32;var p,q,r:halfword;c:quarterword;myvarflag:0..85;
ldelim,rdelim:halfword;{831:}groupline:integer;{:831}{836:}
num,denom:scaled;{:836}{843:}prehead,posthead,tail:halfword;
tt:smallnumber;t:halfword;macroref:halfword;{:843}
begin myvarflag:=varflag;varflag:=0;
20:begin if aritherror then cleararith;end;{825:}
ifdef('DEBUG')if panicking then checkmem(false);
endif('DEBUG')if interrupt<>0 then if OKtointerrupt then begin backinput
;begin if interrupt<>0 then pauseforinstructions;end;getxnext;end{:825};
case curcmd of 31:{826:}begin ldelim:=cursym;rdelim:=curmod;getxnext;
scanexpression;if(curcmd=82)and(curtype>=16)then{830:}
begin p:=getnode(2);mem[p].hh.b0:=14;mem[p].hh.b1:=11;initbignode(p);
q:=mem[p+1].int;stashin(q);getxnext;scanexpression;
if curtype<16 then begin disperr(0,772);begin helpptr:=4;
helpline[3]:=773;helpline[2]:=774;helpline[1]:=775;helpline[0]:=776;end;
putgetflusherror(0);end;stashin(q+2);checkdelimiter(ldelim,rdelim);
curtype:=14;curexp:=p;end{:830}else checkdelimiter(ldelim,rdelim);
end{:826};32:{832:}begin groupline:=line;
if internal[7]>0 then showcmdmod(curcmd,curmod);begin p:=getavail;
mem[p].hh.lh:=0;mem[p].hh.rh:=saveptr;saveptr:=p;end;repeat dostatement;
until curcmd<>83;if curcmd<>84 then begin begin if interaction=3 then;
printnl(261);print(777);end;printint(groupline);print(778);
begin helpptr:=2;helpline[1]:=779;helpline[0]:=780;end;backerror;
curcmd:=84;end;unsave;if internal[7]>0 then showcmdmod(curcmd,curmod);
end{:832};39:{833:}begin curtype:=4;curexp:=curmod;end{:833};42:{837:}
begin curexp:=curmod;curtype:=16;getxnext;
if curcmd<>54 then begin num:=0;denom:=0;end else begin getxnext;
if curcmd<>42 then begin backinput;curcmd:=54;curmod:=72;cursym:=9761;
goto 30;end;num:=curexp;denom:=curmod;if denom=0 then{838:}
begin begin if interaction=3 then;printnl(261);print(781);end;
begin helpptr:=1;helpline[0]:=782;end;error;end{:838}
else curexp:=makescaled(num,denom);begin if aritherror then cleararith;
end;getxnext;end;
if curcmd>=30 then if curcmd<42 then begin p:=stashcurexp;scanprimary;
if(abs(num)>=abs(denom))or(curtype<14)then dobinary(p,71)else begin
fracmult(num,denom);freenode(p,2);end;end;goto 30;end{:837};33:{834:}
donullary(curmod){:834};34,30,36,43:{835:}begin c:=curmod;getxnext;
scanprimary;dounary(c);goto 30;end{:835};37:{839:}begin c:=curmod;
getxnext;scanexpression;if curcmd<>69 then begin missingerr(478);
print(713);printcmdmod(37,c);begin helpptr:=1;helpline[0]:=714;end;
backerror;end;p:=stashcurexp;getxnext;scanprimary;dobinary(p,c);goto 30;
end{:839};35:{840:}begin getxnext;scansuffix;oldsetting:=selector;
selector:=5;showtokenlist(curexp,0,100000,0);flushtokenlist(curexp);
curexp:=makestring;selector:=oldsetting;curtype:=4;goto 30;end{:840};
40:{841:}begin q:=curmod;if myvarflag=77 then begin getxnext;
if curcmd=77 then begin curexp:=getavail;mem[curexp].hh.lh:=q+9769;
curtype:=20;goto 30;end;backinput;end;curtype:=16;curexp:=internal[q];
end{:841};38:makeexpcopy(curmod);41:{844:}begin begin prehead:=avail;
if prehead=0 then prehead:=getavail else begin avail:=mem[prehead].hh.rh
;mem[prehead].hh.rh:=0;ifdef('STAT')incr(dynused);endif('STAT')end;end;
tail:=prehead;posthead:=0;tt:=1;while true do begin t:=curtok;
mem[tail].hh.rh:=t;if tt<>0 then begin{850:}begin p:=mem[prehead].hh.rh;
q:=mem[p].hh.lh;tt:=0;if eqtb[q].lh mod 86=41 then begin q:=eqtb[q].rh;
if q=0 then goto 32;while true do begin p:=mem[p].hh.rh;
if p=0 then begin tt:=mem[q].hh.b0;goto 32;end;
if mem[q].hh.b0<>21 then goto 32;q:=mem[mem[q+1].hh.lh].hh.rh;
if p>=himemmin then begin repeat q:=mem[q].hh.rh;
until mem[q+2].hh.lh>=mem[p].hh.lh;
if mem[q+2].hh.lh>mem[p].hh.lh then goto 32;end;end;end;32:end{:850};
if tt>=22 then{845:}begin mem[tail].hh.rh:=0;
if tt>22 then begin posthead:=getavail;tail:=posthead;
mem[tail].hh.rh:=t;tt:=0;macroref:=mem[q+1].int;
incr(mem[macroref].hh.lh);end else{853:}begin p:=getavail;
mem[prehead].hh.lh:=mem[prehead].hh.rh;mem[prehead].hh.rh:=p;
mem[p].hh.lh:=t;macrocall(mem[q+1].int,prehead,0);getxnext;goto 20;
end{:853};end{:845};end;getxnext;tail:=t;if curcmd=63 then{846:}
begin getxnext;scanexpression;if curcmd<>64 then{847:}begin backinput;
backexpr;curcmd:=63;curmod:=0;cursym:=9760;end{:847}
else begin if curtype<>16 then badsubscript;curcmd:=42;curmod:=curexp;
cursym:=0;end;end{:846};if curcmd>42 then goto 31;
if curcmd<40 then goto 31;end;31:{852:}if posthead<>0 then{854:}
begin backinput;p:=getavail;q:=mem[posthead].hh.rh;
mem[prehead].hh.lh:=mem[prehead].hh.rh;mem[prehead].hh.rh:=posthead;
mem[posthead].hh.lh:=q;mem[posthead].hh.rh:=p;
mem[p].hh.lh:=mem[q].hh.rh;mem[q].hh.rh:=0;
macrocall(macroref,prehead,0);decr(mem[macroref].hh.lh);getxnext;
goto 20;end{:854};q:=mem[prehead].hh.rh;begin mem[prehead].hh.rh:=avail;
avail:=prehead;ifdef('STAT')decr(dynused);endif('STAT')end;
if curcmd=myvarflag then begin curtype:=20;curexp:=q;goto 30;end;
p:=findvariable(q);if p<>0 then makeexpcopy(p)else begin obliterated(q);
helpline[2]:=794;helpline[1]:=795;helpline[0]:=796;putgetflusherror(0);
end;flushnodelist(q);goto 30{:852};end{:844};others:begin badexp(766);
goto 20;end end;getxnext;30:if curcmd=63 then if curtype>=16 then{859:}
begin p:=stashcurexp;getxnext;scanexpression;
if curcmd<>82 then begin{847:}begin backinput;backexpr;curcmd:=63;
curmod:=0;cursym:=9760;end{:847};unstashcurexp(p);
end else begin q:=stashcurexp;getxnext;scanexpression;
if curcmd<>64 then begin missingerr(93);begin helpptr:=3;
helpline[2]:=798;helpline[1]:=799;helpline[0]:=695;end;backerror;end;
r:=stashcurexp;makeexpcopy(q);dobinary(r,70);dobinary(p,71);
dobinary(q,69);getxnext;end;end{:859};end;{:823}{860:}
procedure scansuffix;label 30;var h,t:halfword;p:halfword;
begin h:=getavail;t:=h;while true do begin if curcmd=63 then{861:}
begin getxnext;scanexpression;if curtype<>16 then badsubscript;
if curcmd<>64 then begin missingerr(93);begin helpptr:=3;
helpline[2]:=800;helpline[1]:=799;helpline[0]:=695;end;backerror;end;
curcmd:=42;curmod:=curexp;end{:861};
if curcmd=42 then p:=newnumtok(curmod)else if(curcmd=41)or(curcmd=40)
then begin p:=getavail;mem[p].hh.lh:=cursym;end else goto 30;
mem[t].hh.rh:=p;t:=p;getxnext;end;30:curexp:=mem[h].hh.rh;
begin mem[h].hh.rh:=avail;avail:=h;ifdef('STAT')decr(dynused);
endif('STAT')end;curtype:=20;end;{:860}{862:}procedure scansecondary;
label 20,22;var p:halfword;c,d:halfword;macname:halfword;
begin 20:if(curcmd<30)or(curcmd>43)then badexp(801);scanprimary;
22:if curcmd<=55 then if curcmd>=52 then begin p:=stashcurexp;c:=curmod;
d:=curcmd;if d=53 then begin macname:=cursym;incr(mem[c].hh.lh);end;
getxnext;scanprimary;if d<>53 then dobinary(p,c)else begin backinput;
binarymac(p,c,macname);decr(mem[c].hh.lh);getxnext;goto 20;end;goto 22;
end;end;{:862}{864:}procedure scantertiary;label 20,22;var p:halfword;
c,d:halfword;macname:halfword;
begin 20:if(curcmd<30)or(curcmd>43)then badexp(802);scansecondary;
if curtype=8 then materializepen;
22:if curcmd<=45 then if curcmd>=43 then begin p:=stashcurexp;c:=curmod;
d:=curcmd;if d=44 then begin macname:=cursym;incr(mem[c].hh.lh);end;
getxnext;scansecondary;if d<>44 then dobinary(p,c)else begin backinput;
binarymac(p,c,macname);decr(mem[c].hh.lh);getxnext;goto 20;end;goto 22;
end;end;{:864}{868:}procedure scanexpression;label 20,30,22,25,26,10;
var p,q,r,pp,qq:halfword;c,d:halfword;myvarflag:0..85;macname:halfword;
cyclehit:boolean;x,y:scaled;t:0..4;begin myvarflag:=varflag;
20:if(curcmd<30)or(curcmd>43)then badexp(805);scantertiary;
22:if curcmd<=51 then if curcmd>=46 then if(curcmd<>51)or(myvarflag<>77)
then begin p:=stashcurexp;c:=curmod;d:=curcmd;
if d=49 then begin macname:=cursym;incr(mem[c].hh.lh);end;
if(d<48)or((d=48)and((mem[p].hh.b0=14)or(mem[p].hh.b0=9)))then{869:}
begin cyclehit:=false;{870:}begin unstashcurexp(p);
if curtype=14 then p:=newknot else if curtype=9 then p:=curexp else goto
10;q:=p;while mem[q].hh.rh<>p do q:=mem[q].hh.rh;
if mem[p].hh.b0<>0 then begin r:=copyknot(p);mem[q].hh.rh:=r;q:=r;end;
mem[p].hh.b0:=4;mem[q].hh.b1:=4;end{:870};25:{874:}
if curcmd=46 then{879:}begin t:=scandirection;
if t<>4 then begin mem[q].hh.b1:=t;mem[q+5].int:=curexp;
if mem[q].hh.b0=4 then begin mem[q].hh.b0:=t;mem[q+3].int:=curexp;end;
end;end{:879};d:=curcmd;if d=47 then{881:}begin getxnext;
if curcmd=58 then{882:}begin getxnext;y:=curcmd;
if curcmd=59 then getxnext;scanprimary;{883:}
if(curtype<>16)or(curexp<49152)then begin disperr(0,823);
begin helpptr:=1;helpline[0]:=824;end;putgetflusherror(65536);end{:883};
if y=59 then curexp:=-curexp;mem[q+6].int:=curexp;
if curcmd=52 then begin getxnext;y:=curcmd;if curcmd=59 then getxnext;
scanprimary;{883:}
if(curtype<>16)or(curexp<49152)then begin disperr(0,823);
begin helpptr:=1;helpline[0]:=824;end;putgetflusherror(65536);end{:883};
if y=59 then curexp:=-curexp;end;y:=curexp;end{:882}
else if curcmd=57 then{884:}begin mem[q].hh.b1:=1;t:=1;getxnext;
scanprimary;knownpair;mem[q+5].int:=curx;mem[q+6].int:=cury;
if curcmd<>52 then begin x:=mem[q+5].int;y:=mem[q+6].int;
end else begin getxnext;scanprimary;knownpair;x:=curx;y:=cury;end;
end{:884}else begin mem[q+6].int:=65536;y:=65536;backinput;goto 30;end;
if curcmd<>47 then begin missingerr(407);begin helpptr:=1;
helpline[0]:=822;end;backerror;end;30:end{:881}
else if d<>48 then goto 26;getxnext;if curcmd=46 then{880:}
begin t:=scandirection;if mem[q].hh.b1<>1 then x:=curexp else t:=1;
end{:880}else if mem[q].hh.b1<>1 then begin t:=4;x:=0;end{:874};
if curcmd=36 then{886:}begin cyclehit:=true;getxnext;pp:=p;qq:=p;
if d=48 then if p=q then begin d:=47;mem[q+6].int:=65536;y:=65536;end;
end{:886}else begin scantertiary;{885:}
begin if curtype<>9 then pp:=newknot else pp:=curexp;qq:=pp;
while mem[qq].hh.rh<>pp do qq:=mem[qq].hh.rh;
if mem[pp].hh.b0<>0 then begin r:=copyknot(pp);mem[qq].hh.rh:=r;qq:=r;
end;mem[pp].hh.b0:=4;mem[qq].hh.b1:=4;end{:885};end;{887:}
begin if d=48 then if(mem[q+1].int<>mem[pp+1].int)or(mem[q+2].int<>mem[
pp+2].int)then begin begin if interaction=3 then;printnl(261);
print(825);end;begin helpptr:=3;helpline[2]:=826;helpline[1]:=827;
helpline[0]:=828;end;putgeterror;d:=47;mem[q+6].int:=65536;y:=65536;end;
{889:}if mem[pp].hh.b1=4 then if(t=3)or(t=2)then begin mem[pp].hh.b1:=t;
mem[pp+5].int:=x;end{:889};if d=48 then{890:}
begin if mem[q].hh.b0=4 then if mem[q].hh.b1=4 then begin mem[q].hh.b0:=
3;mem[q+3].int:=65536;end;
if mem[pp].hh.b1=4 then if t=4 then begin mem[pp].hh.b1:=3;
mem[pp+5].int:=65536;end;mem[q].hh.b1:=mem[pp].hh.b1;
mem[q].hh.rh:=mem[pp].hh.rh;mem[q+5].int:=mem[pp+5].int;
mem[q+6].int:=mem[pp+6].int;freenode(pp,7);if qq=pp then qq:=q;end{:890}
else begin{888:}
if mem[q].hh.b1=4 then if(mem[q].hh.b0=3)or(mem[q].hh.b0=2)then begin
mem[q].hh.b1:=mem[q].hh.b0;mem[q+5].int:=mem[q+3].int;end{:888};
mem[q].hh.rh:=pp;mem[pp+4].int:=y;if t<>4 then begin mem[pp+3].int:=x;
mem[pp].hh.b0:=t;end;end;q:=qq;end{:887};
if curcmd>=46 then if curcmd<=48 then if not cyclehit then goto 25;
26:{891:}if cyclehit then begin if d=48 then p:=q;
end else begin mem[p].hh.b0:=0;
if mem[p].hh.b1=4 then begin mem[p].hh.b1:=3;mem[p+5].int:=65536;end;
mem[q].hh.b1:=0;if mem[q].hh.b0=4 then begin mem[q].hh.b0:=3;
mem[q+3].int:=65536;end;mem[q].hh.rh:=p;end;makechoices(p);curtype:=9;
curexp:=p{:891};end{:869}else begin getxnext;scantertiary;
if d<>49 then dobinary(p,c)else begin backinput;binarymac(p,c,macname);
decr(mem[c].hh.lh);getxnext;goto 20;end;end;goto 22;end;10:end;{:868}
{892:}procedure getboolean;begin getxnext;scanexpression;
if curtype<>2 then begin disperr(0,829);begin helpptr:=2;
helpline[1]:=830;helpline[0]:=831;end;putgetflusherror(31);curtype:=2;
end;end;{:892}{224:}procedure printcapsule;begin printchar(40);
printexp(gpointer,0);printchar(41);end;procedure tokenrecycle;
begin recyclevalue(gpointer);end;{:224}{1205:}
procedure closefilesandterminate;var k:integer;lh:integer;
lkoffset:0..256;p:halfword;x:scaled;
begin ifdef('STAT')if internal[12]>0 then{1208:}
if logopened then begin writeln(logfile,' ');
writeln(logfile,'Here is how much of METAFONT''s memory',' you used:');
write(logfile,' ',maxstrptr-initstrptr:1,' string');
if maxstrptr<>initstrptr+1 then write(logfile,'s');
writeln(logfile,' out of ',maxstrings-initstrptr:1);
writeln(logfile,' ',maxpoolptr-initpoolptr:1,
' string characters out of ',poolsize-initpoolptr:1);
writeln(logfile,' ',lomemmax+0+memend-himemmin+2:1,
' words of memory out of ',memend+1:1);
writeln(logfile,' ',stcount:1,' symbolic tokens out of ',9500:1);
writeln(logfile,' ',maxinstack:1,'i,',intptr:1,'n,',maxroundingptr:1,
'r,',maxparamstack:1,'p,',maxbufstack+1:1,'b stack positions out of ',
stacksize:1,'i,',maxinternal:1,'n,',maxwiggle:1,'r,',150:1,'p,',bufsize:
1,'b');end{:1208};endif('STAT');{1206:}
if(gfprevptr>0)or(internal[33]>0)then begin{1207:}rover:=23;
mem[rover].hh.rh:=262143;lomemmax:=himemmin-1;
if lomemmax-rover>262143 then lomemmax:=262143+rover;
mem[rover].hh.lh:=lomemmax-rover;mem[rover+1].hh.lh:=rover;
mem[rover+1].hh.rh:=rover;mem[lomemmax].hh.rh:=0;
mem[lomemmax].hh.lh:=0{:1207};{1124:}mem[memtop-1].hh.rh:=19;
for k:=bc to ec do if charexists[k]then tfmwidth[k]:=sortin(tfmwidth[k])
;nw:=skimp(255)+1;dimenhead[1]:=mem[memtop-1].hh.rh;
if perturbation>=4096 then tfmwarning(20){:1124};fixdesignsize;
fixchecksum;if internal[33]>0 then begin{1126:}mem[memtop-1].hh.rh:=19;
for k:=bc to ec do if charexists[k]then if tfmheight[k]=0 then tfmheight
[k]:=15 else tfmheight[k]:=sortin(tfmheight[k]);nh:=skimp(15)+1;
dimenhead[2]:=mem[memtop-1].hh.rh;
if perturbation>=4096 then tfmwarning(21);mem[memtop-1].hh.rh:=19;
for k:=bc to ec do if charexists[k]then if tfmdepth[k]=0 then tfmdepth[k
]:=15 else tfmdepth[k]:=sortin(tfmdepth[k]);nd:=skimp(15)+1;
dimenhead[3]:=mem[memtop-1].hh.rh;
if perturbation>=4096 then tfmwarning(22);mem[memtop-1].hh.rh:=19;
for k:=bc to ec do if charexists[k]then if tfmitalcorr[k]=0 then
tfmitalcorr[k]:=15 else tfmitalcorr[k]:=sortin(tfmitalcorr[k]);
ni:=skimp(63)+1;dimenhead[4]:=mem[memtop-1].hh.rh;
if perturbation>=4096 then tfmwarning(23){:1126};internal[33]:=0;{1134:}
if jobname=0 then openlogfile;packjobname(1042);
while not bopenout(tfmfile)do promptfilename(1043,1042);
metricfilename:=bmakenamestring(tfmfile);{1135:}k:=headersize;
while headerbyte[k]<0 do decr(k);lh:=(k+3)div 4;if bc>ec then bc:=1;
{1137:}bchar:=roundunscaled(internal[41]);
if(bchar<0)or(bchar>255)then begin bchar:=-1;lkstarted:=false;
lkoffset:=0;end else begin lkstarted:=true;lkoffset:=1;end;{1138:}
k:=labelptr;if labelloc[k]+lkoffset>255 then begin lkoffset:=0;
lkstarted:=false;repeat charremainder[labelchar[k]]:=lkoffset;
while labelloc[k-1]=labelloc[k]do begin decr(k);
charremainder[labelchar[k]]:=lkoffset;end;incr(lkoffset);decr(k);
until lkoffset+labelloc[k]<256;end;
if lkoffset>0 then while k>0 do begin charremainder[labelchar[k]]:=
charremainder[labelchar[k]]+lkoffset;decr(k);end{:1138};
if bchlabel<ligtablesize then begin ligkern[nl].b0:=255;
ligkern[nl].b1:=0;ligkern[nl].b2:=((bchlabel+lkoffset)div 256);
ligkern[nl].b3:=((bchlabel+lkoffset)mod 256);incr(nl);end{:1137};
bwrite2bytes(tfmfile,6+lh+(ec-bc+1)+nw+nh+nd+ni+nl+lkoffset+nk+ne+np);
bwrite2bytes(tfmfile,lh);bwrite2bytes(tfmfile,bc);
bwrite2bytes(tfmfile,ec);bwrite2bytes(tfmfile,nw);
bwrite2bytes(tfmfile,nh);bwrite2bytes(tfmfile,nd);
bwrite2bytes(tfmfile,ni);bwrite2bytes(tfmfile,nl+lkoffset);
bwrite2bytes(tfmfile,nk);bwrite2bytes(tfmfile,ne);
bwrite2bytes(tfmfile,np);
for k:=1 to 4*lh do begin if headerbyte[k]<0 then headerbyte[k]:=0;
bwritebyte(tfmfile,headerbyte[k]);end{:1135};{1136:}
for k:=bc to ec do if not charexists[k]then bwrite4bytes(tfmfile,0)else
begin bwritebyte(tfmfile,mem[tfmwidth[k]].hh.lh);
bwritebyte(tfmfile,(mem[tfmheight[k]].hh.lh)*16+mem[tfmdepth[k]].hh.lh);
bwritebyte(tfmfile,(mem[tfmitalcorr[k]].hh.lh)*4+chartag[k]);
bwritebyte(tfmfile,charremainder[k]);end;tfmchanged:=0;
for k:=1 to 4 do begin bwrite4bytes(tfmfile,0);p:=dimenhead[k];
while p<>19 do begin bwrite4bytes(tfmfile,dimenout(mem[p+1].int));
p:=mem[p].hh.rh;end;end{:1136};{1139:}
for k:=0 to 255 do if skiptable[k]<ligtablesize then begin printnl(1045)
;printint(k);print(1046);ll:=skiptable[k];repeat lll:=ligkern[ll].b0;
ligkern[ll].b0:=128;ll:=ll-lll;until lll=0;end;
if lkstarted then begin bwritebyte(tfmfile,255);
bwritebyte(tfmfile,bchar);bwrite2bytes(tfmfile,0);
end else for k:=1 to lkoffset do begin ll:=labelloc[labelptr];
if bchar<0 then begin bwritebyte(tfmfile,254);bwritebyte(tfmfile,0);
end else begin bwritebyte(tfmfile,255);bwritebyte(tfmfile,bchar);end;
bwrite2bytes(tfmfile,ll+lkoffset);repeat decr(labelptr);
until labelloc[labelptr]<ll;end;for k:=0 to nl-1 do tfmqqqq(ligkern[k]);
for k:=0 to nk-1 do bwrite4bytes(tfmfile,dimenout(kern[k])){:1139};
{1140:}for k:=0 to ne-1 do tfmqqqq(exten[k]);
for k:=1 to np do if k=1 then if abs(param[1])<134217728 then
bwrite4bytes(tfmfile,param[1]*16)else begin incr(tfmchanged);
if param[1]>0 then bwrite4bytes(tfmfile,2147483647)else bwrite4bytes(
tfmfile,-2147483647);end else bwrite4bytes(tfmfile,dimenout(param[k]));
if tfmchanged>0 then begin if tfmchanged=1 then printnl(1047)else begin
printnl(40);printint(tfmchanged);print(1048);end;print(1049);end{:1140};
ifdef('STAT')if internal[12]>0 then{1141:}begin writeln(logfile,' ');
if bchlabel<ligtablesize then decr(nl);
writeln(logfile,'(You used ',nw:1,'w,',nh:1,'h,',nd:1,'d,',ni:1,'i,',nl:
1,'l,',nk:1,'k,',ne:1,'e,',np:1,'p metric file positions');
writeln(logfile,'  out of ','256w,16h,16d,64i,',ligtablesize:1,'l,',
maxkerns:1,'k,256e,',maxfontdimen:1,'p)');end{:1141};
endif('STAT')printnl(1044);slowprint(metricfilename);printchar(46);
bclose(tfmfile){:1134};end;if gfprevptr>0 then{1182:}
begin begin gfbuf[gfptr]:=248;incr(gfptr);if gfptr=gflimit then gfswap;
end;gffour(gfprevptr);gfprevptr:=gfoffset+gfptr-5;
gffour(internal[26]*16);
for k:=1 to 4 do begin gfbuf[gfptr]:=headerbyte[k];incr(gfptr);
if gfptr=gflimit then gfswap;end;gffour(internal[27]);
gffour(internal[28]);gffour(gfminm);gffour(gfmaxm);gffour(gfminn);
gffour(gfmaxn);
for k:=0 to 255 do if charexists[k]then begin x:=gfdx[k]div 65536;
if(gfdy[k]=0)and(x>=0)and(x<256)and(gfdx[k]=x*65536)then begin begin
gfbuf[gfptr]:=246;incr(gfptr);if gfptr=gflimit then gfswap;end;
begin gfbuf[gfptr]:=k;incr(gfptr);if gfptr=gflimit then gfswap;end;
begin gfbuf[gfptr]:=x;incr(gfptr);if gfptr=gflimit then gfswap;end;
end else begin begin gfbuf[gfptr]:=245;incr(gfptr);
if gfptr=gflimit then gfswap;end;begin gfbuf[gfptr]:=k;incr(gfptr);
if gfptr=gflimit then gfswap;end;gffour(gfdx[k]);gffour(gfdy[k]);end;
x:=mem[tfmwidth[k]+1].int;
if abs(x)>maxtfmdimen then if x>0 then x:=16777215 else x:=-16777215
else x:=makescaled(x*16,internal[26]);gffour(x);gffour(charptr[k]);end;
begin gfbuf[gfptr]:=249;incr(gfptr);if gfptr=gflimit then gfswap;end;
gffour(gfprevptr);begin gfbuf[gfptr]:=131;incr(gfptr);
if gfptr=gflimit then gfswap;end;k:=4+((gfbufsize-gfptr)mod 4);
while k>0 do begin begin gfbuf[gfptr]:=223;incr(gfptr);
if gfptr=gflimit then gfswap;end;decr(k);end;{1156:}
if gflimit=halfbuf then writegf(halfbuf,gfbufsize-1);
if gfptr>0 then writegf(0,gfptr-1){:1156};printnl(1060);
slowprint(outputfilename);print(557);printint(totalchars);print(1061);
if totalchars<>1 then printchar(115);print(1062);
printint(gfoffset+gfptr);print(1063);bclose(gffile);end{:1182};
end{:1206};if logopened then begin writeln(logfile);aclose(logfile);
selector:=selector-2;if selector=1 then begin printnl(1071);
slowprint(texmflogname);printchar(46);end;end;println;
if(editnamestart<>0)and(interaction>0)then calledit(strpool,
editnamestart,editnamelength,editline);end;{:1205}{1209:}
procedure finalcleanup;label 10;var c:smallnumber;begin c:=curmod;
if jobname=0 then openlogfile;while openparens>0 do begin print(1072);
decr(openparens);end;while condptr<>0 do begin printnl(1073);
printcmdmod(2,curif);if ifline<>0 then begin print(1074);
printint(ifline);end;print(1075);ifline:=mem[condptr+1].int;
curif:=mem[condptr].hh.b1;condptr:=mem[condptr].hh.rh;end;
if history<>0 then if((history=1)or(interaction<3))then if selector=3
then begin selector:=1;printnl(1076);selector:=3;end;
if c=1 then begin ifdef('INIMF')storebasefile;goto 10;
endif('INIMF')printnl(1077);goto 10;end;10:end;{:1209}{1210:}
ifdef('INIMF')procedure initprim;begin{192:}primitive(408,40,1);
primitive(409,40,2);primitive(410,40,3);primitive(411,40,4);
primitive(412,40,5);primitive(413,40,6);primitive(414,40,7);
primitive(415,40,8);primitive(416,40,9);primitive(417,40,10);
primitive(418,40,11);primitive(419,40,12);primitive(420,40,13);
primitive(421,40,14);primitive(422,40,15);primitive(423,40,16);
primitive(424,40,17);primitive(425,40,18);primitive(426,40,19);
primitive(427,40,20);primitive(428,40,21);primitive(429,40,22);
primitive(430,40,23);primitive(431,40,24);primitive(432,40,25);
primitive(433,40,26);primitive(434,40,27);primitive(435,40,28);
primitive(436,40,29);primitive(437,40,30);primitive(438,40,31);
primitive(439,40,32);primitive(440,40,33);primitive(441,40,34);
primitive(442,40,35);primitive(443,40,36);primitive(444,40,37);
primitive(445,40,38);primitive(446,40,39);primitive(447,40,40);
primitive(448,40,41);{:192}{211:}primitive(407,47,0);primitive(91,63,0);
eqtb[9760]:=eqtb[cursym];primitive(93,64,0);primitive(125,65,0);
primitive(123,46,0);primitive(58,81,0);eqtb[9762]:=eqtb[cursym];
primitive(458,80,0);primitive(459,79,0);primitive(460,77,0);
primitive(44,82,0);primitive(59,83,0);eqtb[9763]:=eqtb[cursym];
primitive(92,7,0);primitive(461,18,0);primitive(462,72,0);
primitive(463,59,0);primitive(464,32,0);bgloc:=cursym;
primitive(465,57,0);primitive(466,19,0);primitive(467,60,0);
primitive(468,27,0);primitive(469,11,0);primitive(452,84,0);
eqtb[9767]:=eqtb[cursym];egloc:=cursym;primitive(470,26,0);
primitive(471,6,0);primitive(472,9,0);primitive(473,70,0);
primitive(474,73,0);primitive(475,13,0);primitive(476,14,0);
primitive(477,15,0);primitive(478,69,0);primitive(479,28,0);
primitive(480,24,0);primitive(481,12,0);primitive(482,8,0);
primitive(483,17,0);primitive(484,78,0);primitive(485,74,0);
primitive(486,35,0);primitive(487,58,0);primitive(488,71,0);
primitive(489,75,0);{:211}{683:}primitive(652,16,1);primitive(653,16,2);
primitive(654,16,53);primitive(655,16,44);primitive(656,16,49);
primitive(453,16,0);eqtb[9765]:=eqtb[cursym];primitive(657,4,9770);
primitive(658,4,9920);primitive(659,4,1);primitive(454,4,0);
eqtb[9764]:=eqtb[cursym];{:683}{688:}primitive(660,61,0);
primitive(661,61,1);primitive(64,61,2);primitive(662,61,3);{:688}{695:}
primitive(673,56,9770);primitive(674,56,9920);primitive(675,56,10070);
primitive(676,56,1);primitive(677,56,2);primitive(678,56,3);{:695}{709:}
primitive(688,3,0);primitive(614,3,1);{:709}{740:}primitive(715,1,1);
primitive(451,2,2);eqtb[9766]:=eqtb[cursym];primitive(716,2,3);
primitive(717,2,4);{:740}{893:}primitive(346,33,30);
primitive(347,33,31);primitive(348,33,32);primitive(349,33,33);
primitive(350,33,34);primitive(351,33,35);primitive(352,33,36);
primitive(353,33,37);primitive(354,34,38);primitive(355,34,39);
primitive(356,34,40);primitive(357,34,41);primitive(358,34,42);
primitive(359,34,43);primitive(360,34,44);primitive(361,34,45);
primitive(362,34,46);primitive(363,34,47);primitive(364,34,48);
primitive(365,34,49);primitive(366,34,50);primitive(367,34,51);
primitive(368,34,52);primitive(369,34,53);primitive(370,34,54);
primitive(371,34,55);primitive(372,34,56);primitive(373,34,57);
primitive(374,34,58);primitive(375,34,59);primitive(376,34,60);
primitive(377,34,61);primitive(378,34,62);primitive(379,34,63);
primitive(380,34,64);primitive(381,34,65);primitive(382,34,66);
primitive(383,34,67);primitive(384,36,68);primitive(43,43,69);
primitive(45,43,70);primitive(42,55,71);primitive(47,54,72);
eqtb[9761]:=eqtb[cursym];primitive(385,45,73);primitive(309,45,74);
primitive(387,52,76);primitive(386,45,75);primitive(60,50,77);
primitive(388,50,78);primitive(62,50,79);primitive(389,50,80);
primitive(61,51,81);primitive(390,50,82);primitive(400,37,94);
primitive(401,37,95);primitive(402,37,96);primitive(403,37,97);
primitive(404,37,98);primitive(405,37,99);primitive(406,37,100);
primitive(38,48,83);primitive(391,55,84);primitive(392,55,85);
primitive(393,55,86);primitive(394,55,87);primitive(395,55,88);
primitive(396,55,89);primitive(397,55,90);primitive(398,55,91);
primitive(399,45,92);{:893}{1013:}primitive(339,30,15);
primitive(325,30,4);primitive(323,30,2);primitive(330,30,9);
primitive(327,30,6);primitive(332,30,11);primitive(334,30,13);
primitive(335,30,14);{:1013}{1018:}primitive(909,85,0);
primitive(910,85,1);{:1018}{1024:}primitive(271,23,0);
primitive(272,23,1);primitive(273,23,2);primitive(916,23,3);{:1024}
{1027:}primitive(917,21,0);primitive(918,21,1);{:1027}{1037:}
primitive(932,22,0);primitive(933,22,1);primitive(934,22,2);
primitive(935,22,3);primitive(936,22,4);{:1037}{1052:}
primitive(953,68,1);primitive(954,68,0);primitive(955,68,2);
primitive(956,66,6);primitive(957,66,16);primitive(958,67,0);
primitive(959,67,1);{:1052}{1079:}primitive(989,25,0);
primitive(990,25,1);primitive(991,25,2);{:1079}{1101:}
primitive(1001,20,0);primitive(1002,20,1);primitive(1003,20,2);
primitive(1004,20,3);primitive(1005,20,4);{:1101}{1108:}
primitive(1023,76,0);primitive(1024,76,1);primitive(1025,76,5);
primitive(1026,76,2);primitive(1027,76,6);primitive(1028,76,3);
primitive(1029,76,7);primitive(1030,76,11);primitive(1031,76,128);
{:1108}{1176:}primitive(1055,29,4);primitive(1056,29,16);{:1176};end;
procedure inittab;var k:integer;begin{176:}rover:=23;
mem[rover].hh.rh:=262143;mem[rover].hh.lh:=1000;
mem[rover+1].hh.lh:=rover;mem[rover+1].hh.rh:=rover;
lomemmax:=rover+1000;mem[lomemmax].hh.rh:=0;mem[lomemmax].hh.lh:=0;
for k:=memtop-2 to memtop do mem[k]:=mem[lomemmax];avail:=0;
memend:=memtop;himemmin:=memtop-2;varused:=23;
dynused:=memtop+1-himemmin;{:176}{193:}intname[1]:=408;intname[2]:=409;
intname[3]:=410;intname[4]:=411;intname[5]:=412;intname[6]:=413;
intname[7]:=414;intname[8]:=415;intname[9]:=416;intname[10]:=417;
intname[11]:=418;intname[12]:=419;intname[13]:=420;intname[14]:=421;
intname[15]:=422;intname[16]:=423;intname[17]:=424;intname[18]:=425;
intname[19]:=426;intname[20]:=427;intname[21]:=428;intname[22]:=429;
intname[23]:=430;intname[24]:=431;intname[25]:=432;intname[26]:=433;
intname[27]:=434;intname[28]:=435;intname[29]:=436;intname[30]:=437;
intname[31]:=438;intname[32]:=439;intname[33]:=440;intname[34]:=441;
intname[35]:=442;intname[36]:=443;intname[37]:=444;intname[38]:=445;
intname[39]:=446;intname[40]:=447;intname[41]:=448;{:193}{203:}
hashused:=9757;stcount:=0;hash[9768].rh:=450;hash[9766].rh:=451;
hash[9767].rh:=452;hash[9765].rh:=453;hash[9764].rh:=454;
hash[9763].rh:=59;hash[9762].rh:=58;hash[9761].rh:=47;hash[9760].rh:=91;
hash[9759].rh:=41;hash[9757].rh:=455;eqtb[9759].lh:=62;{:203}{229:}
mem[19].hh.lh:=9770;mem[19].hh.rh:=0;{:229}{324:}
mem[memtop].hh.lh:=262143;{:324}{475:}mem[3].hh.lh:=0;mem[3].hh.rh:=0;
mem[4].hh.lh:=1;mem[4].hh.rh:=0;for k:=5 to 11 do mem[k]:=mem[4];
mem[12].int:=0;mem[0].hh.rh:=0;mem[0].hh.lh:=0;mem[1].int:=0;
mem[2].int:=0;{:475}{587:}serialno:=0;mem[13].hh.rh:=13;
mem[14].hh.lh:=13;mem[13].hh.lh:=0;mem[14].hh.rh:=0;{:587}{702:}
mem[21].hh.b1:=0;mem[21].hh.rh:=9768;eqtb[9768].rh:=21;
eqtb[9768].lh:=41;{:702}{759:}eqtb[9758].lh:=91;hash[9758].rh:=732;
{:759}{911:}mem[17].hh.b1:=11;{:911}{1116:}mem[20].int:=1073741824;
{:1116}{1127:}mem[16].int:=0;mem[15].hh.lh:=0;{:1127}{1185:}
baseident:=1064;{:1185}end;endif('INIMF'){:1210}{1212:}
ifdef('DEBUG')procedure debughelp;label 888,10;var k,l,m,n:integer;
begin while true do begin;printnl(1078);flush(stdout);read(stdin,m);
if m<0 then goto 10 else if m=0 then begin goto 888;
888:m:=0;{'BREAKPOINT'}
end else begin read(stdin,n);case m of{1213:}1:printword(mem[n]);
2:printint(mem[n].hh.lh);3:printint(mem[n].hh.rh);
4:begin printint(eqtb[n].lh);printchar(58);printint(eqtb[n].rh);end;
5:printvariablename(n);6:printint(internal[n]);7:doshowdependencies;
9:showtokenlist(n,0,100000,0);10:slowprint(n);11:checkmem(n>0);
12:searchmem(n);13:begin read(stdin,l);printcmdmod(n,l);end;
14:for k:=0 to n do print(buffer[k]);15:panicking:=not panicking;{:1213}
others:print(63)end;end;end;10:end;endif('DEBUG'){:1212}{:1202}{1204:}
begin history:=3;;setpaths(MFBASEPATHBIT+MFINPUTPATHBIT+MFPOOLPATHBIT);
if readyalready=314159 then goto 1;{14:}bad:=0;
if(halferrorline<30)or(halferrorline>errorline-15)then bad:=1;
if maxprintline<60 then bad:=2;if gfbufsize mod 8<>0 then bad:=3;
if 1100>memtop then bad:=4;if 7919>9500 then bad:=5;
if headersize mod 4<>0 then bad:=6;
if(ligtablesize<255)or(ligtablesize>32510)then bad:=7;{:14}{154:}
ifdef('INIMF')if memmax<>memtop then bad:=10;
endif('INIMF')if memmax<memtop then bad:=10;
if(0>0)or(255<127)then bad:=11;if(0>0)or(262143<32767)then bad:=12;
if(0<0)or(255>262143)then bad:=13;if(0<0)or(memmax>=262143)then bad:=14;
if maxstrings>262143 then bad:=15;if bufsize>262143 then bad:=16;
if(255<255)or(262143<65535)then bad:=17;{:154}{204:}
if 9769+maxinternal>262143 then bad:=21;{:204}{214:}
if 10220>262143 then bad:=22;{:214}{310:}
if 15*11>bistacksize then bad:=31;{:310}{553:}
if 20+17*45>bistacksize then bad:=32;{:553}{777:}
if basedefaultlength>PATHMAX then bad:=41;{:777}
if bad>0 then begin writeln(stdout,
'Ouch---my internal constants have been clobbered!','---case ',bad:1);
goto 9999;end;initialize;
ifdef('INIMF')if not getstringsstarted then goto 9999;inittab;initprim;
initstrptr:=strptr;initpoolptr:=poolptr;maxstrptr:=strptr;
maxpoolptr:=poolptr;fixdateandtime;endif('INIMF')readyalready:=314159;
1:{55:}selector:=1;tally:=0;termoffset:=0;fileoffset:=0;{:55}{61:}
write(stdout,'This is METAFONT, C Version 2.71');
if baseident>0 then slowprint(baseident);println;flush(stdout);{:61}
{783:}jobname:=0;logopened:=false;{:783}{792:}outputfilename:=0;{:792};
{1211:}begin{657:}begin inputptr:=0;maxinstack:=0;inopen:=0;
openparens:=0;maxbufstack:=0;paramptr:=0;maxparamstack:=0;first:=1;
curinput.startfield:=1;curinput.indexfield:=0;line:=0;
curinput.namefield:=0;forceeof:=false;
if not initterminal then goto 9999;curinput.limitfield:=last;
first:=last+1;end;{:657}{660:}scannerstatus:=0;{:660};
if(baseident=0)or(buffer[curinput.locfield]=38)then begin if baseident<>
0 then initialize;if not openbasefile then goto 9999;
if not loadbasefile then begin wclose(basefile);goto 9999;end;
wclose(basefile);
while(curinput.locfield<curinput.limitfield)and(buffer[curinput.locfield
]=32)do incr(curinput.locfield);end;buffer[curinput.limitfield]:=37;
fixdateandtime;initrandoms((internal[17]div 65536)+internal[16]);{70:}
if interaction=0 then selector:=0 else selector:=1{:70};
if curinput.locfield<curinput.limitfield then if buffer[curinput.
locfield]<>92 then startinput;end{:1211};history:=0;
if startsym>0 then begin cursym:=startsym;backinput;end;maincontrol;
finalcleanup;closefilesandterminate;9999:begin flush(stdout);
readyalready:=0;if(history<>0)and(history<>1)then uexit(1)else uexit(0);
end;end.{:1204}
