{4:}{9:}{$C-,A+,D-}ifdef('TEXMF_DEBUG'){$C+,D+}endif('TEXMF_DEBUG'){:9}
program MP;label{6:}1,9998,9999;{:6}const{11:}maxinternal=300;
bufsize=3000;emergencylinelength=255;stacksize=300;maxreadfiles=30;
stringsvacant=1000;fontmax=50;fontmemsize=10000;poolname='mp.pool';
pstabname='psfonts.map';pathsize=1000;bistacksize=1500;headersize=100;
ligtablesize=15000;maxkerns=2500;maxfontdimen=50;infmainmemory=2999;
supmainmemory=8000000;infmaxstrings=2500;supmaxstrings=32767;
infpoolsize=32000;suppoolsize=10000000;infpoolfree=1000;
suppoolfree=suppoolsize;infstringvacancies=8000;
supstringvacancies=suppoolsize-23000;{:11}type{18:}ASCIIcode=0..255;
{:18}{24:}eightbits=0..255;alphafile=packed file of ASCIIcode;
bytefile=packed file of eightbits;{:24}{37:}poolpointer=0..poolsize;
strnumber=0..maxstrings;poolASCIIcode=0..255;{:37}{116:}scaled=integer;
smallnumber=0..63;{:116}{120:}fraction=integer;{:120}{121:}
angle=integer;{:121}{171:}quarterword=0..255;halfword=0..268435455;
twochoices=1..2;threechoices=1..3;
#include "texmfmem.h";wordfile=file of memoryword;{:171}{204:}
commandcode=1..84;{:204}{581:}
instaterecord=record indexfield:quarterword;
startfield,locfield,limitfield,namefield:halfword;end;{:581}{779:}
readfindex=0..maxreadfiles;writeindex=0..4;{:779}{1174:}
fontnumber=0..fontmax;{:1174}{1311:}strreftype=0..127;{:1311}var{13:}
bad:integer;ifdef('INIMP')iniversion:boolean;dumpoption:boolean;
dumpline:boolean;endif('INIMP')bounddefault:integer;boundname:^char;
mainmemory:integer;memtop:integer;memmax:integer;errorline:integer;
halferrorline:integer;maxprintline:integer;poolsize:integer;
stringvacancies:integer;poolfree:integer;maxstrings:integer;{:13}{20:}
xord:array[ASCIIcode]of ASCIIcode;xchr:array[ASCIIcode]of ASCIIcode;
{:20}{25:}nameoffile:^ASCIIcode;namelength:0..maxint;{:25}{29:}
buffer:array[0..bufsize]of ASCIIcode;first:0..bufsize;last:0..bufsize;
maxbufstack:0..bufsize;{:29}{38:}strpool:^poolASCIIcode;
strstart:^poolpointer;nextstr:^strnumber;poolptr:poolpointer;
strptr:strnumber;initpoolptr:poolpointer;initstruse:strnumber;
maxpoolptr:poolpointer;maxstrptr:strnumber;{:38}{39:}strsusedup:integer;
poolinuse:integer;strsinuse:integer;maxplused:integer;
maxstrsused:integer;{:39}{44:}strref:^strreftype;{:44}{48:}
lastfixedstr:strnumber;fixedstruse:strnumber;{:48}{56:}
stroverflowed:boolean;{:56}{58:}pactcount:integer;pactchars:integer;
pactstrs:integer;{:58}{65:}ifdef('INIMP')poolfile:alphafile;
endif('INIMP'){:65}{69:}logfile:alphafile;psfile:alphafile;
selector:0..10;dig:array[0..22]of 0..15;tally:integer;
termoffset:0..maxprintline;fileoffset:0..maxprintline;psoffset:integer;
trickbuf:array[0..255]of ASCIIcode;trickcount:integer;
firstcount:integer;{:69}{83:}interaction:0..3;interactionoption:0..4;
{:83}{86:}deletionsallowed:boolean;history:0..3;errorcount:-1..100;{:86}
{89:}helpline:array[0..5]of strnumber;helpptr:0..6;useerrhelp:boolean;
errhelp:strnumber;{:89}{106:}interrupt:integer;OKtointerrupt:boolean;
{:106}{112:}aritherror:boolean;{:112}{144:}
twotothe:array[0..30]of integer;speclog:array[1..28]of integer;{:144}
{152:}specatan:array[1..26]of angle;{:152}{159:}nsin,ncos:fraction;
{:159}{163:}randoms:array[0..54]of fraction;jrandom:0..54;{:163}{174:}
mem:^memoryword;lomemmax:halfword;himemmin:halfword;{:174}{175:}
varused,dynused:integer;{:175}{176:}avail:halfword;memend:halfword;
{:176}{181:}rover:halfword;{:181}{193:}
ifdef('TEXMF_DEBUG')freearr:packed array[0..1]of boolean;
wasfree:packed array[0..1]of boolean;
wasmemend,waslomax,washimin:halfword;panicking:boolean;
endif('TEXMF_DEBUG'){:193}{208:}internal:array[1..maxinternal]of scaled;
intname:array[1..maxinternal]of strnumber;intptr:33..maxinternal;{:208}
{214:}oldsetting,nonpssetting:0..10;{:214}{216:}
charclass:array[ASCIIcode]of 0..20;{:216}{218:}hashused:halfword;
stcount:integer;{:218}{219:}hash:array[1..9771]of twohalves;
eqtb:array[1..9771]of twohalves;{:219}{244:}gpointer:halfword;{:244}
{249:}bignodesize:array[12..14]of smallnumber;
sector0:array[12..14]of smallnumber;
sectoroffset:array[5..13]of smallnumber;{:249}{269:}saveptr:halfword;
{:269}{287:}pathtail:halfword;{:287}{300:}
deltax,deltay,delta:array[0..pathsize]of scaled;
psi:array[1..pathsize]of angle;{:300}{304:}
theta:array[0..pathsize]of angle;uu:array[0..pathsize]of fraction;
vv:array[0..pathsize]of angle;ww:array[0..pathsize]of fraction;{:304}
{319:}st,ct,sf,cf:fraction;{:319}{329:}bbmin,bbmax:array[0..1]of scaled;
{:329}{373:}halfcos:array[0..7]of fraction;dcos:array[0..7]of fraction;
{:373}{387:}curx,cury:scaled;{:387}{401:}
grobjectsize:array[1..7]of smallnumber;{:401}{462:}specoffset:integer;
{:462}{464:}specp1,specp2:halfword;{:464}{526:}tolstep:0..6;{:526}{527:}
bisectstack:array[0..bistacksize]of integer;bisectptr:0..bistacksize;
{:527}{530:}curt,curtt:integer;timetogo:integer;maxt:integer;{:530}
{532:}delx,dely:integer;tol:integer;uv,xy:0..bistacksize;threel:integer;
apprt,apprtt:integer;{:532}{539:}serialno:integer;{:539}{546:}
fixneeded:boolean;watchcoefs:boolean;depfinal:halfword;{:546}{578:}
curcmd:eightbits;curmod:integer;cursym:halfword;{:578}{582:}
inputstack:array[0..stacksize]of instaterecord;inputptr:0..stacksize;
maxinstack:0..stacksize;curinput:instaterecord;{:582}{585:}inopen:0..15;
openparens:0..15;inputfile:array[1..15]of alphafile;
linestack:array[0..15]of integer;inamestack:array[0..15]of strnumber;
iareastack:array[0..15]of strnumber;mpxname:array[0..15]of halfword;
{:585}{587:}paramstack:array[0..150]of halfword;paramptr:0..150;
maxparamstack:integer;{:587}{589:}fileptr:0..stacksize;{:589}{618:}
scannerstatus:0..7;warninginfo:integer;{:618}{641:}forceeof:boolean;
{:641}{671:}bgloc,egloc:1..9771;{:671}{710:}condptr:halfword;
iflimit:0..4;curif:smallnumber;ifline:integer;{:710}{724:}
loopptr:halfword;{:724}{743:}curname:strnumber;curarea:strnumber;
curext:strnumber;{:743}{745:}areadelimiter:integer;extdelimiter:integer;
{:745}{752:}memdefaultlength:integer;MPmemdefault:^char;
troffmode:boolean;{:752}{760:}jobname:strnumber;logopened:boolean;
texmflogname:strnumber;{:760}{775:}oldfilename:^char;
oldnamelength:0..maxint;{:775}{780:}
rdfile:array[readfindex]of alphafile;
rdfname:array[readfindex]of strnumber;readfiles:readfindex;
wrfile:array[writeindex]of alphafile;
wrfname:array[writeindex]of strnumber;writefiles:writeindex;{:780}{784:}
curtype:smallnumber;curexp:integer;{:784}{801:}
maxc:array[17..18]of integer;maxptr:array[17..18]of halfword;
maxlink:array[17..18]of halfword;{:801}{809:}varflag:0..84;{:809}{903:}
sepic:halfword;sesf:scaled;{:903}{927:}eofline:strnumber;{:927}{961:}
txx,txy,tyx,tyy,tx,ty:scaled;{:961}{1085:}lastaddtype:quarterword;
{:1085}{1101:}startsym:halfword;{:1101}{1109:}longhelpseen:boolean;
{:1109}{1118:}tfmfile:bytefile;metricfilename:strnumber;{:1118}{1127:}
bc,ec:eightbits;tfmwidth:array[eightbits]of scaled;
tfmheight:array[eightbits]of scaled;tfmdepth:array[eightbits]of scaled;
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
labelchar:array[1..256]of eightbits;labelptr:0..256;{:1127}{1150:}
perturbation:scaled;excess:integer;{:1150}{1156:}
dimenhead:array[1..4]of halfword;{:1156}{1161:}maxtfmdimen:scaled;
tfmchanged:integer;{:1161}{1173:}tfminfile:bytefile;{:1173}{1175:}
fontinfo:array[0..fontmemsize]of memoryword;nextfmem:0..fontmemsize;
lastfnum:fontnumber;fontdsize:array[fontnumber]of scaled;
fontname:array[fontnumber]of strnumber;
fontpsname:array[fontnumber]of strnumber;lastpsfnum:fontnumber;
fontbc,fontec:array[fontnumber]of eightbits;{:1175}{1176:}
charbase:array[fontnumber]of 0..fontmemsize;
widthbase:array[fontnumber]of 0..fontmemsize;
heightbase:array[fontnumber]of 0..fontmemsize;
depthbase:array[fontnumber]of 0..fontmemsize;{:1176}{1196:}
pstabfile:alphafile;{:1196}{1204:}firstfilename,lastfilename:strnumber;
firstoutputcode,lastoutputcode:integer;totalshipped:integer;{:1204}
{1212:}neednewpath:boolean;{:1212}{1215:}gsred,gsgreen,gsblue:scaled;
gsljoin,gslcap:quarterword;gsmiterlim:scaled;gsdashp:halfword;
gsdashsc:scaled;gswidth:scaled;gsadjwx:boolean;{:1215}{1250:}
fontsizes:array[fontnumber]of halfword;{:1250}{1254:}
lastpending:halfword;{:1254}{1277:}memident:strnumber;{:1277}{1282:}
memfile:wordfile;{:1282}{1297:}readyalready:integer;{:1297}{1309:}
editnamestart:poolpointer;editnamelength,editline:integer;{:1309}
procedure initialize;var{19:}i:integer;{:19}{145:}k:integer;{:145}
begin{21:}xchr[32]:=' ';xchr[33]:='!';xchr[34]:='"';xchr[35]:='#';
xchr[36]:='$';xchr[37]:='%';xchr[38]:='&';xchr[39]:='''';xchr[40]:='(';
xchr[41]:=')';xchr[42]:='*';xchr[43]:='+';xchr[44]:=',';xchr[45]:='-';
xchr[46]:='.';xchr[47]:='/';xchr[48]:='0';xchr[49]:='1';xchr[50]:='2';
xchr[51]:='3';xchr[52]:='4';xchr[53]:='5';xchr[54]:='6';xchr[55]:='7';
xchr[56]:='8';xchr[57]:='9';xchr[58]:=':';xchr[59]:=';';xchr[60]:='<';
xchr[61]:='=';xchr[62]:='>';xchr[63]:='?';xchr[64]:='@';xchr[65]:='A';
xchr[66]:='B';xchr[67]:='C';xchr[68]:='D';xchr[69]:='E';xchr[70]:='F';
xchr[71]:='G';xchr[72]:='H';xchr[73]:='I';xchr[74]:='J';xchr[75]:='K';
xchr[76]:='L';xchr[77]:='M';xchr[78]:='N';xchr[79]:='O';xchr[80]:='P';
xchr[81]:='Q';xchr[82]:='R';xchr[83]:='S';xchr[84]:='T';xchr[85]:='U';
xchr[86]:='V';xchr[87]:='W';xchr[88]:='X';xchr[89]:='Y';xchr[90]:='Z';
xchr[91]:='[';xchr[92]:='\';xchr[93]:=']';xchr[94]:='^';xchr[95]:='_';
xchr[96]:='`';xchr[97]:='a';xchr[98]:='b';xchr[99]:='c';xchr[100]:='d';
xchr[101]:='e';xchr[102]:='f';xchr[103]:='g';xchr[104]:='h';
xchr[105]:='i';xchr[106]:='j';xchr[107]:='k';xchr[108]:='l';
xchr[109]:='m';xchr[110]:='n';xchr[111]:='o';xchr[112]:='p';
xchr[113]:='q';xchr[114]:='r';xchr[115]:='s';xchr[116]:='t';
xchr[117]:='u';xchr[118]:='v';xchr[119]:='w';xchr[120]:='x';
xchr[121]:='y';xchr[122]:='z';xchr[123]:='{';xchr[124]:='|';
xchr[125]:='}';xchr[126]:='~';{:21}{22:}
for i:=0 to 31 do xchr[i]:=chr(i);for i:=127 to 255 do xchr[i]:=chr(i);
{:22}{23:}for i:=0 to 255 do xord[chr(i)]:=127;
for i:=128 to 255 do xord[xchr[i]]:=i;
for i:=0 to 126 do xord[xchr[i]]:=i;{:23}{84:}
if interactionoption=4 then interaction:=3 else interaction:=
interactionoption;{:84}{87:}deletionsallowed:=true;errorcount:=0;{:87}
{90:}helpptr:=0;useerrhelp:=false;errhelp:=0;{:90}{107:}interrupt:=0;
OKtointerrupt:=true;{:107}{113:}aritherror:=false;{:113}{146:}
twotothe[0]:=1;for k:=1 to 30 do twotothe[k]:=2*twotothe[k-1];
speclog[1]:=93032640;speclog[2]:=38612034;speclog[3]:=17922280;
speclog[4]:=8662214;speclog[5]:=4261238;speclog[6]:=2113709;
speclog[7]:=1052693;speclog[8]:=525315;speclog[9]:=262400;
speclog[10]:=131136;speclog[11]:=65552;speclog[12]:=32772;
speclog[13]:=16385;for k:=14 to 27 do speclog[k]:=twotothe[27-k];
speclog[28]:=1;{:146}{153:}specatan[1]:=27855475;specatan[2]:=14718068;
specatan[3]:=7471121;specatan[4]:=3750058;specatan[5]:=1876857;
specatan[6]:=938658;specatan[7]:=469357;specatan[8]:=234682;
specatan[9]:=117342;specatan[10]:=58671;specatan[11]:=29335;
specatan[12]:=14668;specatan[13]:=7334;specatan[14]:=3667;
specatan[15]:=1833;specatan[16]:=917;specatan[17]:=458;
specatan[18]:=229;specatan[19]:=115;specatan[20]:=57;specatan[21]:=29;
specatan[22]:=14;specatan[23]:=7;specatan[24]:=4;specatan[25]:=2;
specatan[26]:=1;{:153}{194:}ifdef('TEXMF_DEBUG')wasmemend:=0;
waslomax:=0;washimin:=memmax;panicking:=false;endif('TEXMF_DEBUG'){:194}
{209:}for k:=1 to 33 do internal[k]:=0;intptr:=33;{:209}{217:}
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
{:217}{220:}hash[1].lh:=0;hash[1].rh:=0;eqtb[1].lh:=43;eqtb[1].rh:=0;
for k:=2 to 9771 do begin hash[k]:=hash[1];eqtb[k]:=eqtb[1];end;{:220}
{250:}bignodesize[12]:=12;bignodesize[14]:=4;bignodesize[13]:=6;
sector0[12]:=5;sector0[14]:=5;sector0[13]:=11;
for k:=5 to 10 do sectoroffset[k]:=2*(k-5);
for k:=11 to 13 do sectoroffset[k]:=2*(k-11);{:250}{270:}saveptr:=0;
{:270}{374:}halfcos[0]:=134217728;halfcos[1]:=94906266;halfcos[2]:=0;
dcos[0]:=35596755;dcos[1]:=25170707;dcos[2]:=0;
for k:=3 to 4 do begin halfcos[k]:=-halfcos[4-k];dcos[k]:=-dcos[4-k];
end;for k:=5 to 7 do begin halfcos[k]:=halfcos[8-k];dcos[k]:=dcos[8-k];
end;{:374}{402:}grobjectsize[1]:=6;grobjectsize[2]:=8;
grobjectsize[3]:=14;grobjectsize[4]:=2;grobjectsize[6]:=2;
grobjectsize[5]:=2;grobjectsize[7]:=2;{:402}{465:}specp1:=0;specp2:=0;
{:465}{547:}fixneeded:=false;watchcoefs:=true;{:547}{711:}condptr:=0;
iflimit:=0;curif:=0;ifline:=0;{:711}{725:}loopptr:=0;{:725}{744:}
curname:=284;curarea:=284;curext:=284;{:744}{781:}readfiles:=0;
writefiles:=0;{:781}{785:}curexp:=0;{:785}{810:}varflag:=0;{:810}{928:}
eofline:=0;{:928}{1102:}startsym:=0;{:1102}{1110:}longhelpseen:=false;
{:1110}{1128:}for k:=0 to 255 do begin tfmwidth[k]:=0;tfmheight[k]:=0;
tfmdepth[k]:=0;tfmitalcorr[k]:=0;charexists[k]:=false;chartag[k]:=0;
charremainder[k]:=0;skiptable[k]:=ligtablesize;end;
for k:=1 to headersize do headerbyte[k]:=-1;bc:=255;ec:=0;nl:=0;nk:=0;
ne:=0;np:=0;internal[31]:=-65536;bchlabel:=ligtablesize;labelloc[0]:=-1;
labelptr:=0;{:1128}{1205:}firstfilename:=284;lastfilename:=284;
firstoutputcode:=32768;lastoutputcode:=-32768;totalshipped:=0;{:1205}
{1255:}lastpending:=memtop-3;{:1255}{1278:}memident:=0;{:1278}{1310:}
editnamestart:=0;{:1310}end;{72:}procedure println;
begin case selector of 10:begin writeln(stdout);writeln(logfile);
termoffset:=0;fileoffset:=0;end;9:begin writeln(logfile);fileoffset:=0;
end;8:begin writeln(stdout);termoffset:=0;end;5:begin writeln(psfile);
psoffset:=0;end;7,6,4:;others:writeln(wrfile[selector])end;end;{:72}
{73:}procedure unitstrroom;forward;
procedure printvisiblechar(s:ASCIIcode);label 30;
begin case selector of 10:begin write(stdout,xchr[s]);
write(logfile,xchr[s]);incr(termoffset);incr(fileoffset);
if termoffset=maxprintline then begin writeln(stdout);termoffset:=0;end;
if fileoffset=maxprintline then begin writeln(logfile);fileoffset:=0;
end;end;9:begin write(logfile,xchr[s]);incr(fileoffset);
if fileoffset=maxprintline then println;end;
8:begin write(stdout,xchr[s]);incr(termoffset);
if termoffset=maxprintline then println;end;
5:begin write(psfile,xchr[s]);incr(psoffset);end;7:;
6:if tally<trickcount then trickbuf[tally mod errorline]:=s;
4:begin if poolptr>=maxpoolptr then begin unitstrroom;
if poolptr>=poolsize then goto 30;end;begin strpool[poolptr]:=s;
incr(poolptr);end;end;others:write(wrfile[selector],xchr[s])end;
30:incr(tally);end;{:73}{74:}procedure printchar(k:ASCIIcode);
var l:0..255;begin if selector<6 then printvisiblechar(k)else if{64:}
(k<32)or(k>126){:64}then begin printvisiblechar(94);
printvisiblechar(94);
if k<64 then printvisiblechar(k+64)else if k<128 then printvisiblechar(k
-64)else begin l:=k div 16;
if l<10 then printvisiblechar(l+48)else printvisiblechar(l+87);
l:=k mod 16;
if l<10 then printvisiblechar(l+48)else printvisiblechar(l+87);end;
end else printvisiblechar(k);end;{:74}{75:}procedure print(s:integer);
var j:poolpointer;begin if(s<0)or(s>maxstrptr)then s:=260;
j:=strstart[s];
while j<strstart[nextstr[s]]do begin printchar(strpool[j]);incr(j);end;
end;{:75}{77:}procedure printnl(s:strnumber);
begin case selector of 10:if(termoffset>0)or(fileoffset>0)then println;
9:if fileoffset>0 then println;8:if termoffset>0 then println;
5:if psoffset>0 then println;7,6,4:;end;print(s);end;{:77}{78:}
procedure printthedigs(k:eightbits);begin while k>0 do begin decr(k);
printchar(48+dig[k]);end;end;{:78}{79:}procedure printint(n:integer);
var k:0..23;m:integer;begin k:=0;if n<0 then begin printchar(45);
if n>-100000000 then n:=-n else begin m:=-1-n;n:=m div 10;
m:=(m mod 10)+1;k:=1;if m<10 then dig[0]:=m else begin dig[0]:=0;
incr(n);end;end;end;repeat dig[k]:=n mod 10;n:=n div 10;incr(k);
until n=0;printthedigs(k);end;{:79}{118:}
procedure printscaled(s:scaled);var delta:scaled;
begin if s<0 then begin printchar(45);s:=-s;end;printint(s div 65536);
s:=10*(s mod 65536)+5;if s<>5 then begin delta:=10;printchar(46);
repeat if delta>65536 then s:=s+32768-(delta div 2);
printchar(48+(s div 65536));s:=10*(s mod 65536);delta:=delta*10;
until s<=delta;end;end;{:118}{119:}procedure printtwo(x,y:scaled);
begin printchar(40);printscaled(x);printchar(44);printscaled(y);
printchar(41);end;{:119}{205:}procedure printtype(t:smallnumber);
begin case t of 1:print(324);2:print(325);3:print(326);4:print(259);
5:print(327);6:print(328);7:print(329);8:print(330);9:print(331);
10:print(332);11:print(333);12:print(334);13:print(335);14:print(336);
16:print(337);17:print(338);18:print(339);15:print(340);19:print(341);
20:print(342);21:print(343);22:print(344);23:print(345);
others:print(346)end;end;{:205}{213:}{588:}function trueline:integer;
var k:0..stacksize;
begin if(curinput.indexfield<=15)and(curinput.namefield>2)then trueline
:=linestack[curinput.indexfield]else begin k:=inputptr;
while(k>0)and(inputstack[k].indexfield>15)or(inputstack[k].namefield<=2)
do decr(k);trueline:=linestack[k];end;end;{:588}
procedure begindiagnostic;begin oldsetting:=selector;
if selector=5 then selector:=nonpssetting;
if(internal[12]<=0)and(selector=10)then begin decr(selector);
if history=0 then history:=1;end;end;
procedure enddiagnostic(blankline:boolean);begin printnl(284);
if blankline then println;selector:=oldsetting;end;{:213}{215:}
procedure printdiagnostic(s,t:strnumber;nuline:boolean);
begin begindiagnostic;if nuline then printnl(s)else print(s);print(463);
printint(trueline);print(t);printchar(58);end;{:215}{750:}
procedure printfilename(n,a,e:integer);begin print(a);print(n);print(e);
end;{:750}{88:}procedure normalizeselector;forward;procedure getnext;
forward;procedure terminput;forward;procedure showcontext;forward;
procedure beginfilereading;forward;procedure openlogfile;forward;
procedure closefilesandterminate;forward;procedure clearforerrorprompt;
forward;ifdef('TEXMF_DEBUG')procedure debughelp;forward;
endif('TEXMF_DEBUG'){45:}procedure flushstring(s:strnumber);
begin ifdef('STAT')poolinuse:=poolinuse-(strstart[nextstr[s]]-strstart[s
]);decr(strsinuse);
endif('STAT')if nextstr[s]<>strptr then strref[s]:=0 else begin strptr:=
s;decr(strsusedup);end;poolptr:=strstart[strptr];end;{:45}{:88}{91:}
procedure jumpout;begin closefilesandterminate;begin fflush(stdout);
readyalready:=0;if(history<>0)and(history<>1)then uexit(1)else uexit(0);
end;end;{:91}{92:}procedure error;label 22,10;var c:ASCIIcode;
s1,s2,s3:integer;j:poolpointer;begin if history<2 then history:=2;
printchar(46);showcontext;if interaction=3 then{93:}
while true do begin 22:clearforerrorprompt;begin;print(264);terminput;
end;if last=first then goto 10;c:=buffer[first];if c>=97 then c:=c-32;
{94:}
case c of 48,49,50,51,52,53,54,55,56,57:if deletionsallowed then{98:}
begin s1:=curcmd;s2:=curmod;s3:=cursym;OKtointerrupt:=false;
if(last>first+1)and(buffer[first+1]>=48)and(buffer[first+1]<=57)then c:=
c*10+buffer[first+1]-48*11 else c:=c-48;while c>0 do begin getnext;
{715:}
if curcmd=41 then begin if strref[curmod]<127 then if strref[curmod]>1
then decr(strref[curmod])else flushstring(curmod);end{:715};decr(c);end;
curcmd:=s1;curmod:=s2;cursym:=s3;OKtointerrupt:=true;begin helpptr:=2;
helpline[1]:=277;helpline[0]:=278;end;showcontext;goto 22;end{:98};
ifdef('TEXMF_DEBUG')68:begin debughelp;goto 22;end;
endif('TEXMF_DEBUG')69:if fileptr>0 then begin editnamestart:=strstart[
inputstack[fileptr].namefield];
editnamelength:=(strstart[nextstr[inputstack[fileptr].namefield]]-
strstart[inputstack[fileptr].namefield]);editline:=trueline;jumpout;end;
72:{99:}begin if useerrhelp then begin{100:}j:=strstart[errhelp];
while j<strstart[nextstr[errhelp]]do begin if strpool[j]<>37 then print(
strpool[j])else if j+1=strstart[nextstr[errhelp]]then println else if
strpool[j+1]<>37 then println else begin incr(j);printchar(37);end;
incr(j);end{:100};useerrhelp:=false;
end else begin if helpptr=0 then begin helpptr:=2;helpline[1]:=279;
helpline[0]:=280;end;repeat decr(helpptr);print(helpline[helpptr]);
println;until helpptr=0;end;begin helpptr:=4;helpline[3]:=281;
helpline[2]:=280;helpline[1]:=282;helpline[0]:=283;end;goto 22;end{:99};
73:{97:}begin beginfilereading;
if last>first+1 then begin curinput.locfield:=first+1;buffer[first]:=32;
end else begin begin;print(276);terminput;end;curinput.locfield:=first;
end;first:=last+1;curinput.limitfield:=last;goto 10;end{:97};
81,82,83:{96:}begin errorcount:=0;interaction:=0+c-81;print(271);
case c of 81:begin print(272);decr(selector);end;82:print(273);
83:print(274);end;print(275);println;fflush(stdout);goto 10;end{:96};
88:begin interaction:=2;jumpout;end;others:end;{95:}begin print(265);
printnl(266);printnl(267);if fileptr>0 then print(268);
if deletionsallowed then printnl(269);printnl(270);end{:95}{:94};
end{:93};incr(errorcount);if errorcount=100 then begin printnl(263);
history:=3;jumpout;end;{101:}if interaction>0 then decr(selector);
if useerrhelp then begin printnl(284);{100:}j:=strstart[errhelp];
while j<strstart[nextstr[errhelp]]do begin if strpool[j]<>37 then print(
strpool[j])else if j+1=strstart[nextstr[errhelp]]then println else if
strpool[j+1]<>37 then println else begin incr(j);printchar(37);end;
incr(j);end{:100};end else while helpptr>0 do begin decr(helpptr);
printnl(helpline[helpptr]);end;println;
if interaction>0 then incr(selector);println{:101};10:end;{:92}{103:}
procedure fatalerror(s:strnumber);begin normalizeselector;
begin if interaction=3 then;printnl(262);print(285);end;
begin helpptr:=1;helpline[0]:=s;end;
begin if interaction=3 then interaction:=2;if logopened then error;
ifdef('TEXMF_DEBUG')if interaction>0 then debughelp;
endif('TEXMF_DEBUG')history:=3;jumpout;end;end;{:103}{104:}
procedure overflow(s:strnumber;n:integer);begin normalizeselector;
begin if interaction=3 then;printnl(262);print(286);end;print(s);
printchar(61);printint(n);printchar(93);begin helpptr:=2;
helpline[1]:=287;helpline[0]:=288;end;
begin if interaction=3 then interaction:=2;if logopened then error;
ifdef('TEXMF_DEBUG')if interaction>0 then debughelp;
endif('TEXMF_DEBUG')history:=3;jumpout;end;end;{:104}{105:}
procedure confusion(s:strnumber);begin normalizeselector;
if history<2 then begin begin if interaction=3 then;printnl(262);
print(289);end;print(s);printchar(41);begin helpptr:=1;helpline[0]:=290;
end;end else begin begin if interaction=3 then;printnl(262);print(291);
end;begin helpptr:=2;helpline[1]:=292;helpline[0]:=293;end;end;
begin if interaction=3 then interaction:=2;if logopened then error;
ifdef('TEXMF_DEBUG')if interaction>0 then debughelp;
endif('TEXMF_DEBUG')history:=3;jumpout;end;end;{:105}{:4}{30:}
{[34:]if memident=0 then begin writeln(stdout,'Buffer size exceeded!');
goto 9999;end else begin curinput.locfield:=first;
curinput.limitfield:=last-1;overflow(256,bufsize);end[:34]}{:30}{36:}
function initterminal:boolean;label 10;begin topenin;
if last>first then begin curinput.locfield:=first;
while(curinput.locfield<last)and(buffer[curinput.locfield]=' ')do incr(
curinput.locfield);
if curinput.locfield<last then begin initterminal:=true;goto 10;end;end;
while true do begin;write(stdout,'**');fflush(stdout);
if not inputln(stdin,true)then begin writeln(stdout);
write(stdout,'! End of file on the terminal... why?');
initterminal:=false;goto 10;end;curinput.locfield:=first;
while(curinput.locfield<last)and(buffer[curinput.locfield]=32)do incr(
curinput.locfield);
if curinput.locfield<last then begin initterminal:=true;goto 10;end;
writeln(stdout,'Please type the name of your input file.');end;10:end;
{:36}{46:}{49:}procedure docompaction(needed:poolpointer);label 30;
var struse:strnumber;r,s,t:strnumber;p,q:poolpointer;begin{50:}
t:=nextstr[lastfixedstr];
while(strref[t]=127)and(t<>strptr)do begin incr(fixedstruse);
lastfixedstr:=t;t:=nextstr[t];end;struse:=fixedstruse{:50};
r:=lastfixedstr;s:=nextstr[r];p:=strstart[s];
while s<>strptr do begin while strref[s]=0 do{51:}begin t:=s;
s:=nextstr[s];nextstr[r]:=s;nextstr[t]:=nextstr[strptr];
nextstr[strptr]:=t;if s=strptr then goto 30;end{:51};r:=s;s:=nextstr[s];
incr(struse);{52:}q:=strstart[r];strstart[r]:=p;
while q<strstart[s]do begin strpool[p]:=strpool[q];incr(p);incr(q);
end{:52};end;30:{54:}q:=strstart[strptr];strstart[strptr]:=p;
while q<poolptr do begin strpool[p]:=strpool[q];incr(p);incr(q);end;
poolptr:=p{:54};if needed<poolsize then{55:}
begin if struse>=maxstrings-1 then begin stroverflowed:=true;
overflow(257,maxstrings-1-initstruse);end;
if poolptr+needed>maxpoolptr then if poolptr+needed>poolsize then begin
stroverflowed:=true;overflow(258,poolsize-initpoolptr);
end else maxpoolptr:=poolptr+needed;end{:55};ifdef('STAT'){57:}
if(strstart[strptr]<>poolinuse)or(struse<>strsinuse)then confusion(259);
incr(pactcount);
pactchars:=pactchars+poolptr-strstart[nextstr[lastfixedstr]];
pactstrs:=pactstrs+struse-fixedstruse;ifdef('TEXMF_DEBUG')s:=strptr;
t:=struse;while s<=maxstrptr do begin if t>maxstrptr then confusion(34);
incr(t);s:=nextstr[s];end;if t<=maxstrptr then confusion(34);
endif('TEXMF_DEBUG'){:57};endif('STAT')strsusedup:=struse;end;{:49}{43:}
procedure unitstrroom;
begin if poolptr>=poolsize then docompaction(poolsize);
if poolptr>=maxpoolptr then maxpoolptr:=poolptr+1;end;{:43}
function makestring:strnumber;label 20;var s:strnumber;
begin 20:s:=strptr;strptr:=nextstr[s];
if strptr>maxstrptr then if strptr=maxstrings then begin strptr:=s;
docompaction(0);goto 20;
end else begin ifdef('TEXMF_DEBUG')if strsusedup<>maxstrptr then
confusion(115);endif('TEXMF_DEBUG')maxstrptr:=strptr;
nextstr[strptr]:=maxstrptr+1;end;strref[s]:=1;strstart[strptr]:=poolptr;
incr(strsusedup);ifdef('STAT')incr(strsinuse);
poolinuse:=poolinuse+(strstart[nextstr[s]]-strstart[s]);
if poolinuse>maxplused then maxplused:=poolinuse;
if strsinuse>maxstrsused then maxstrsused:=strsinuse;
endif('STAT')makestring:=s;end;{:46}{47:}
procedure choplaststring(p:poolpointer);
begin ifdef('STAT')poolinuse:=poolinuse-(strstart[strptr]-p);
endif('STAT');strstart[strptr]:=p;end;{:47}{60:}
function streqbuf(s:strnumber;k:integer):boolean;label 45;
var j:poolpointer;result:boolean;begin j:=strstart[s];
while j<strstart[nextstr[s]]do begin if strpool[j]<>buffer[k]then begin
result:=false;goto 45;end;incr(j);incr(k);end;result:=true;
45:streqbuf:=result;end;{:60}{61:}
function strvsstr(s,t:strnumber):integer;label 10;var j,k:poolpointer;
ls,lt:integer;l:integer;begin ls:=(strstart[nextstr[s]]-strstart[s]);
lt:=(strstart[nextstr[t]]-strstart[t]);if ls<=lt then l:=ls else l:=lt;
j:=strstart[s];k:=strstart[t];
while l>0 do begin if strpool[j]<>strpool[k]then begin strvsstr:=strpool
[j]-strpool[k];goto 10;end;incr(j);incr(k);decr(l);end;strvsstr:=ls-lt;
10:end;{:61}{62:}ifdef('INIMP')function getstringsstarted:boolean;
label 30,10;var k,l:0..255;m,n:ASCIIcode;g:strnumber;a:integer;
c:boolean;begin poolptr:=0;strptr:=0;maxpoolptr:=0;maxstrptr:=0;
strstart[0]:=0;nextstr[0]:=1;stroverflowed:=false;
ifdef('STAT')poolinuse:=0;strsinuse:=0;maxplused:=0;maxstrsused:=0;{59:}
pactcount:=0;pactchars:=0;pactstrs:=0{:59};endif('STAT')strsusedup:=0;
{63:}for k:=0 to 255 do begin begin strpool[poolptr]:=k;incr(poolptr);
end;g:=makestring;strref[g]:=127;end;{:63};{66:}
namelength:=strlen(poolname);nameoffile:=xmalloc(1+namelength+1);
strcpy(nameoffile+1,poolname);
if aopenin(poolfile,kpsemppoolformat)then begin c:=false;repeat{67:}
begin if eof(poolfile)then begin;
writeln(stdout,'! mp.pool has no check sum.');aclose(poolfile);
getstringsstarted:=false;goto 10;end;read(poolfile,m);read(poolfile,n);
if m='*'then{68:}begin a:=0;k:=1;
while true do begin if(xord[n]<48)or(xord[n]>57)then begin;
writeln(stdout,'! mp.pool check sum doesn''t have nine digits.');
aclose(poolfile);getstringsstarted:=false;goto 10;end;
a:=10*a+xord[n]-48;if k=9 then goto 30;incr(k);read(poolfile,n);end;
30:if a<>136687108 then begin;writeln(stdout,
'! mp.pool doesn''t match; tangle me again (or fix the path).');
aclose(poolfile);getstringsstarted:=false;goto 10;end;c:=true;end{:68}
else begin if(xord[m]<48)or(xord[m]>57)or(xord[n]<48)or(xord[n]>57)then
begin;writeln(stdout,'! mp.pool line doesn''t begin with two digits.');
aclose(poolfile);getstringsstarted:=false;goto 10;end;
l:=xord[m]*10+xord[n]-48*11;
if poolptr+l+stringvacancies>poolsize then begin;
writeln(stdout,'! You have to increase POOLSIZE.');aclose(poolfile);
getstringsstarted:=false;goto 10;end;
if strptr+stringsvacant>=maxstrings then begin;
writeln(stdout,'! You have to increase MAXSTRINGS.');aclose(poolfile);
getstringsstarted:=false;goto 10;end;
for k:=1 to l do begin if eoln(poolfile)then m:=' 'else read(poolfile,m)
;begin strpool[poolptr]:=xord[m];incr(poolptr);end;end;readln(poolfile);
g:=makestring;strref[g]:=127;end;end{:67};until c;aclose(poolfile);
getstringsstarted:=true;end else begin;
writeln(stdout,'! I can''t read mp.pool; bad path?');aclose(poolfile);
getstringsstarted:=false;goto 10;end{:66};lastfixedstr:=strptr-1;
fixedstruse:=strptr;10:end;endif('INIMP'){:62}{80:}
procedure printdd(n:integer);begin n:=abs(n)mod 100;
printchar(48+(n div 10));printchar(48+(n mod 10));end;{:80}{81:}
procedure terminput;var k:0..bufsize;begin fflush(stdout);
if not inputln(stdin,true)then fatalerror(261);termoffset:=0;
decr(selector);
if last<>first then for k:=first to last-1 do print(buffer[k]);println;
buffer[last]:=37;incr(selector);end;{:81}{102:}
procedure normalizeselector;
begin if logopened then selector:=10 else selector:=8;
if jobname=0 then openlogfile;if interaction=0 then decr(selector);end;
{:102}{108:}procedure pauseforinstructions;
begin if OKtointerrupt then begin interaction:=3;
if(selector=9)or(selector=7)then incr(selector);
begin if interaction=3 then;printnl(262);print(294);end;
begin helpptr:=3;helpline[2]:=295;helpline[1]:=296;helpline[0]:=297;end;
deletionsallowed:=false;error;deletionsallowed:=true;interrupt:=0;end;
end;{:108}{109:}procedure missingerr(s:strnumber);
begin begin if interaction=3 then;printnl(262);print(298);end;print(s);
print(299);end;{:109}{114:}procedure cleararith;
begin begin if interaction=3 then;printnl(262);print(300);end;
begin helpptr:=4;helpline[3]:=301;helpline[2]:=302;helpline[1]:=303;
helpline[0]:=304;end;error;aritherror:=false;end;{:114}{115:}
function slowadd(x,y:integer):integer;
begin if x>=0 then if y<=2147483647-x then slowadd:=x+y else begin
aritherror:=true;slowadd:=2147483647;
end else if-y<=2147483647+x then slowadd:=x+y else begin aritherror:=
true;slowadd:=-2147483647;end;end;{:115}{117:}
function rounddecimals(k:smallnumber):scaled;var a:integer;begin a:=0;
while k>0 do begin decr(k);a:=(a+dig[k]*131072)div 10;end;
rounddecimals:=halfp(a+1);end;{:117}{122:}
ifdef('FIXPT')function makefraction(p,q:integer):fraction;var f:integer;
n:integer;negative:boolean;becareful:integer;
begin if p>=0 then negative:=false else begin p:=-p;negative:=true;end;
if q<=0 then begin ifdef('TEXMF_DEBUG')if q=0 then confusion(47);
endif('TEXMF_DEBUG')q:=-q;negative:=not negative;end;n:=p div q;
p:=p mod q;if n>=8 then begin aritherror:=true;
if negative then makefraction:=-2147483647 else makefraction:=2147483647
;end else begin n:=(n-1)*268435456;{123:}f:=1;repeat becareful:=p-q;
p:=becareful+p;if p>=0 then f:=f+f+1 else begin f:=f+f;p:=p+q;end;
until f>=268435456;becareful:=p-q;if becareful+p>=0 then incr(f){:123};
if negative then makefraction:=-(f+n)else makefraction:=f+n;end;end;
endif('FIXPT'){:122}{124:}ifdef('FIXPT')function takefraction(q:integer;
f:fraction):integer;var p:integer;negative:boolean;n:integer;
becareful:integer;begin{125:}
if f>=0 then negative:=false else begin f:=-f;negative:=true;end;
if q<0 then begin q:=-q;negative:=not negative;end;{:125};
if f<268435456 then n:=0 else begin n:=f div 268435456;
f:=f mod 268435456;
if q<=2147483647 div n then n:=n*q else begin aritherror:=true;
n:=2147483647;end;end;f:=f+268435456;{126:}p:=134217728;
if q<1073741824 then repeat if odd(f)then p:=halfp(p+q)else p:=halfp(p);
f:=halfp(f);
until f=1 else repeat if odd(f)then p:=p+halfp(q-p)else p:=halfp(p);
f:=halfp(f);until f=1{:126};becareful:=n-2147483647;
if becareful+p>0 then begin aritherror:=true;n:=2147483647-p;end;
if negative then takefraction:=-(n+p)else takefraction:=n+p;end;
endif('FIXPT'){:124}{127:}ifdef('FIXPT')function takescaled(q:integer;
f:scaled):integer;var p:integer;negative:boolean;n:integer;
becareful:integer;begin{125:}
if f>=0 then negative:=false else begin f:=-f;negative:=true;end;
if q<0 then begin q:=-q;negative:=not negative;end;{:125};
if f<65536 then n:=0 else begin n:=f div 65536;f:=f mod 65536;
if q<=2147483647 div n then n:=n*q else begin aritherror:=true;
n:=2147483647;end;end;f:=f+65536;{128:}p:=32768;
if q<1073741824 then repeat if odd(f)then p:=halfp(p+q)else p:=halfp(p);
f:=halfp(f);
until f=1 else repeat if odd(f)then p:=p+halfp(q-p)else p:=halfp(p);
f:=halfp(f);until f=1{:128};becareful:=n-2147483647;
if becareful+p>0 then begin aritherror:=true;n:=2147483647-p;end;
if negative then takescaled:=-(n+p)else takescaled:=n+p;end;
endif('FIXPT'){:127}{129:}
ifdef('FIXPT')function makescaled(p,q:integer):scaled;var f:integer;
n:integer;negative:boolean;becareful:integer;
begin if p>=0 then negative:=false else begin p:=-p;negative:=true;end;
if q<=0 then begin ifdef('TEXMF_DEBUG')if q=0 then confusion(47);
endif('TEXMF_DEBUG')q:=-q;negative:=not negative;end;n:=p div q;
p:=p mod q;if n>=32768 then begin aritherror:=true;
if negative then makescaled:=-2147483647 else makescaled:=2147483647;
end else begin n:=(n-1)*65536;{130:}f:=1;repeat becareful:=p-q;
p:=becareful+p;if p>=0 then f:=f+f+1 else begin f:=f+f;p:=p+q;end;
until f>=65536;becareful:=p-q;if becareful+p>=0 then incr(f){:130};
if negative then makescaled:=-(f+n)else makescaled:=f+n;end;end;
endif('FIXPT'){:129}{131:}function velocity(st,ct,sf,cf:fraction;
t:scaled):fraction;var acc,num,denom:integer;
begin acc:=takefraction(st-(sf div 16),sf-(st div 16));
acc:=takefraction(acc,ct-cf);num:=536870912+takefraction(acc,379625062);
denom:=805306368+takefraction(ct,497706707)+takefraction(cf,307599661);
if t<>65536 then num:=makescaled(num,t);
if num div 4>=denom then velocity:=1073741824 else velocity:=
makefraction(num,denom);end;{:131}{132:}
function abvscd(a,b,c,d:integer):integer;label 10;var q,r:integer;
begin{133:}if a<0 then begin a:=-a;b:=-b;end;if c<0 then begin c:=-c;
d:=-d;end;
if d<=0 then begin if b>=0 then if((a=0)or(b=0))and((c=0)or(d=0))then
begin abvscd:=0;goto 10;end else begin abvscd:=1;goto 10;end;
if d=0 then if a=0 then begin abvscd:=0;goto 10;
end else begin abvscd:=-1;goto 10;end;q:=a;a:=c;c:=q;q:=-b;b:=-d;d:=q;
end else if b<=0 then begin if b<0 then if a>0 then begin abvscd:=-1;
goto 10;end;if c=0 then begin abvscd:=0;goto 10;
end else begin abvscd:=-1;goto 10;end;end{:133};
while true do begin q:=a div d;r:=c div b;
if q<>r then if q>r then begin abvscd:=1;goto 10;
end else begin abvscd:=-1;goto 10;end;q:=a mod d;r:=c mod b;
if r=0 then if q=0 then begin abvscd:=0;goto 10;
end else begin abvscd:=1;goto 10;end;if q=0 then begin abvscd:=-1;
goto 10;end;a:=b;b:=q;c:=d;d:=r;end;10:end;{:132}{136:}
function squarert(x:scaled):scaled;var k:smallnumber;y,q:integer;
begin if x<=0 then{137:}
begin if x<0 then begin begin if interaction=3 then;printnl(262);
print(305);end;printscaled(x);print(306);begin helpptr:=2;
helpline[1]:=307;helpline[0]:=308;end;error;end;squarert:=0;end{:137}
else begin k:=23;q:=2;while x<536870912 do begin decr(k);x:=x+x+x+x;end;
if x<1073741824 then y:=0 else begin x:=x-1073741824;y:=1;end;
repeat{138:}x:=x+x;y:=y+y;if x>=1073741824 then begin x:=x-1073741824;
incr(y);end;x:=x+x;y:=y+y-q;q:=q+q;
if x>=1073741824 then begin x:=x-1073741824;incr(y);end;
if y>q then begin y:=y-q;q:=q+2;end else if y<=0 then begin q:=q-2;
y:=y+q;end;decr(k){:138};until k=0;squarert:=halfp(q);end;end;{:136}
{139:}function pythadd(a,b:integer):integer;label 30;var r:fraction;
big:boolean;begin a:=abs(a);b:=abs(b);if a<b then begin r:=b;b:=a;a:=r;
end;
if b>0 then begin if a<536870912 then big:=false else begin a:=a div 4;
b:=b div 4;big:=true;end;{140:}while true do begin r:=makefraction(b,a);
r:=takefraction(r,r);if r=0 then goto 30;
r:=makefraction(r,1073741824+r);a:=a+takefraction(a+a,r);
b:=takefraction(b,r);end;30:{:140};
if big then if a<536870912 then a:=a+a+a+a else begin aritherror:=true;
a:=2147483647;end;end;pythadd:=a;end;{:139}{141:}
function pythsub(a,b:integer):integer;label 30;var r:fraction;
big:boolean;begin a:=abs(a);b:=abs(b);if a<=b then{143:}
begin if a<b then begin begin if interaction=3 then;printnl(262);
print(309);end;printscaled(a);print(310);printscaled(b);print(306);
begin helpptr:=2;helpline[1]:=307;helpline[0]:=308;end;error;end;a:=0;
end{:143}
else begin if a<1073741824 then big:=false else begin a:=halfp(a);
b:=halfp(b);big:=true;end;{142:}
while true do begin r:=makefraction(b,a);r:=takefraction(r,r);
if r=0 then goto 30;r:=makefraction(r,1073741824-r);
a:=a-takefraction(a+a,r);b:=takefraction(b,r);end;30:{:142};
if big then a:=a+a;end;pythsub:=a;end;{:141}{147:}
function mlog(x:scaled):scaled;var y,z:integer;k:integer;
begin if x<=0 then{149:}begin begin if interaction=3 then;printnl(262);
print(311);end;printscaled(x);print(306);begin helpptr:=2;
helpline[1]:=312;helpline[0]:=308;end;error;mlog:=0;end{:149}
else begin y:=1302456860;z:=6581195;while x<1073741824 do begin x:=x+x;
y:=y-93032639;z:=z-48782;end;y:=y+(z div 65536);k:=2;
while x>1073741828 do{148:}begin z:=((x-1)div twotothe[k])+1;
while x<1073741824+z do begin z:=halfp(z+1);k:=k+1;end;y:=y+speclog[k];
x:=x-z;end{:148};mlog:=y div 8;end;end;{:147}{150:}
function mexp(x:scaled):scaled;var k:smallnumber;y,z:integer;
begin if x>174436200 then begin aritherror:=true;mexp:=2147483647;
end else if x<-197694359 then mexp:=0 else begin if x<=0 then begin z:=
-8*x;y:=1048576;
end else begin if x<=127919879 then z:=1023359037-8*x else z:=8*(
174436200-x);y:=2147483647;end;{151:}k:=1;
while z>0 do begin while z>=speclog[k]do begin z:=z-speclog[k];
y:=y-1-((y-twotothe[k-1])div twotothe[k]);end;incr(k);end{:151};
if x<=127919879 then mexp:=(y+8)div 16 else mexp:=y;end;end;{:150}{154:}
function narg(x,y:integer):angle;var z:angle;t:integer;k:smallnumber;
octant:1..8;begin if x>=0 then octant:=1 else begin x:=-x;octant:=2;end;
if y<0 then begin y:=-y;octant:=octant+2;end;if x<y then begin t:=y;
y:=x;x:=t;octant:=octant+4;end;if x=0 then{155:}
begin begin if interaction=3 then;printnl(262);print(313);end;
begin helpptr:=2;helpline[1]:=314;helpline[0]:=308;end;error;narg:=0;
end{:155}else begin{157:}while x>=536870912 do begin x:=halfp(x);
y:=halfp(y);end;z:=0;
if y>0 then begin while x<268435456 do begin x:=x+x;y:=y+y;end;{158:}
k:=0;repeat y:=y+y;incr(k);if y>x then begin z:=z+specatan[k];t:=x;
x:=x+(y div twotothe[k+k]);y:=y-t;end;until k=15;repeat y:=y+y;incr(k);
if y>x then begin z:=z+specatan[k];y:=y-x;end;until k=26{:158};end{:157}
;{156:}case octant of 1:narg:=z;5:narg:=94371840-z;6:narg:=94371840+z;
2:narg:=188743680-z;4:narg:=z-188743680;8:narg:=-z-94371840;
7:narg:=z-94371840;3:narg:=-z;end{:156};end;end;{:154}{160:}
procedure nsincos(z:angle);var k:smallnumber;q:0..7;r:fraction;
x,y,t:integer;begin while z<0 do z:=z+377487360;z:=z mod 377487360;
q:=z div 47185920;z:=z mod 47185920;x:=268435456;y:=x;
if not odd(q)then z:=47185920-z;{162:}k:=1;
while z>0 do begin if z>=specatan[k]then begin z:=z-specatan[k];t:=x;
x:=t+y div twotothe[k];y:=y-t div twotothe[k];end;incr(k);end;
if y<0 then y:=0{:162};{161:}case q of 0:;1:begin t:=x;x:=y;y:=t;end;
2:begin t:=x;x:=-y;y:=t;end;3:x:=-x;4:begin x:=-x;y:=-y;end;
5:begin t:=x;x:=-y;y:=-t;end;6:begin t:=x;x:=y;y:=-t;end;7:y:=-y;
end{:161};r:=pythadd(x,y);ncos:=makefraction(x,r);
nsin:=makefraction(y,r);end;{:160}{164:}procedure newrandoms;
var k:0..54;x:fraction;
begin for k:=0 to 23 do begin x:=randoms[k]-randoms[k+31];
if x<0 then x:=x+268435456;randoms[k]:=x;end;
for k:=24 to 54 do begin x:=randoms[k]-randoms[k-24];
if x<0 then x:=x+268435456;randoms[k]:=x;end;jrandom:=54;end;{:164}
{165:}procedure initrandoms(seed:scaled);var j,jj,k:fraction;i:0..54;
begin j:=abs(seed);while j>=268435456 do j:=halfp(j);k:=1;
for i:=0 to 54 do begin jj:=k;k:=j-k;j:=jj;if k<0 then k:=k+268435456;
randoms[(i*21)mod 55]:=j;end;newrandoms;newrandoms;newrandoms;end;{:165}
{166:}function unifrand(x:scaled):scaled;var y:scaled;
begin if jrandom=0 then newrandoms else decr(jrandom);
y:=takefraction(abs(x),randoms[jrandom]);
if y=abs(x)then unifrand:=0 else if x>0 then unifrand:=y else unifrand:=
-y;end;{:166}{167:}function normrand:scaled;var x,u,l:integer;
begin repeat repeat if jrandom=0 then newrandoms else decr(jrandom);
x:=takefraction(112429,randoms[jrandom]-134217728);
if jrandom=0 then newrandoms else decr(jrandom);u:=randoms[jrandom];
until abs(x)<u;x:=makefraction(x,u);l:=139548960-mlog(u);
until abvscd(1024,l,x,x)>=0;normrand:=x;end;{:167}{172:}
ifdef('TEXMF_DEBUG')procedure printword(w:memoryword);
begin printint(w.int);printchar(32);printscaled(w.int);printchar(32);
printscaled(w.int div 4096);println;printint(w.hh.lh);printchar(61);
printint(w.hh.b0);printchar(58);printint(w.hh.b1);printchar(59);
printint(w.hh.rh);printchar(32);printint(w.qqqq.b0);printchar(58);
printint(w.qqqq.b1);printchar(58);printint(w.qqqq.b2);printchar(58);
printint(w.qqqq.b3);end;endif('TEXMF_DEBUG'){:172}{177:}{236:}
procedure printcapsule;forward;procedure showtokenlist(p,q:integer;
l,nulltally:integer);label 10;var class,c:smallnumber;r,v:integer;
begin class:=3;tally:=nulltally;
while(p<>0)and(tally<l)do begin if p=q then{601:}
begin firstcount:=tally;trickcount:=tally+1+errorline-halferrorline;
if trickcount<errorline then trickcount:=errorline;end{:601};{237:}c:=9;
if(p<0)or(p>memend)then begin print(505);goto 10;end;
if p<himemmin then{238:}
if mem[p].hh.b1=15 then if mem[p].hh.b0=16 then{239:}
begin if class=0 then printchar(32);v:=mem[p+1].int;
if v<0 then begin if class=17 then printchar(32);printchar(91);
printscaled(v);printchar(93);c:=18;end else begin printscaled(v);c:=0;
end;end{:239}
else if mem[p].hh.b0<>4 then print(508)else begin printchar(34);
print(mem[p+1].int);printchar(34);c:=4;
end else if(mem[p].hh.b1<>14)or(mem[p].hh.b0<1)or(mem[p].hh.b0>19)then
print(508)else begin gpointer:=p;printcapsule;c:=8;end{:238}
else begin r:=mem[p].hh.lh;if r>=9772 then{241:}
begin if r<9922 then begin print(510);r:=r-(9772);
end else if r<10072 then begin print(511);r:=r-(9922);
end else begin print(512);r:=r-(10072);end;printint(r);printchar(41);
c:=8;end{:241}else if r<1 then if r=0 then{240:}
begin if class=17 then printchar(32);print(509);c:=18;end{:240}
else print(506)else begin r:=hash[r].rh;
if(r<0)or(r>maxstrptr)then print(507)else{242:}
begin c:=charclass[strpool[strstart[r]]];
if c=class then case c of 9:printchar(46);5,6,7,8:;
others:printchar(32)end;print(r);end{:242};end;end{:237};class:=c;
p:=mem[p].hh.rh;end;if p<>0 then print(504);10:end;{:236}{625:}
procedure runaway;begin if scannerstatus>2 then begin printnl(658);
case scannerstatus of 3:print(659);4,5:print(660);6:print(661);end;
println;showtokenlist(mem[memtop-2].hh.rh,0,errorline-10,0);end;end;
{:625}{:177}{178:}function getavail:halfword;var p:halfword;
begin p:=avail;
if p<>0 then avail:=mem[avail].hh.rh else if memend<memmax then begin
incr(memend);p:=memend;end else begin decr(himemmin);p:=himemmin;
if himemmin<=lomemmax then begin runaway;overflow(315,memmax+1);end;end;
mem[p].hh.rh:=0;ifdef('STAT')incr(dynused);endif('STAT')getavail:=p;end;
{:178}{182:}function getnode(s:integer):halfword;label 40,10,20;
var p:halfword;q:halfword;r:integer;t,tt:integer;begin 20:p:=rover;
repeat{184:}q:=p+mem[p].hh.lh;
while(mem[q].hh.rh=268435455)do begin t:=mem[q+1].hh.rh;
tt:=mem[q+1].hh.lh;if q=rover then rover:=t;mem[t+1].hh.lh:=tt;
mem[tt+1].hh.rh:=t;q:=q+mem[q].hh.lh;end;r:=q-s;
if r>toint(p+1)then{185:}begin mem[p].hh.lh:=r-p;rover:=p;goto 40;
end{:185};if r=p then if mem[p+1].hh.rh<>p then{186:}
begin rover:=mem[p+1].hh.rh;t:=mem[p+1].hh.lh;mem[rover+1].hh.lh:=t;
mem[t+1].hh.rh:=rover;goto 40;end{:186};mem[p].hh.lh:=q-p{:184};
p:=mem[p+1].hh.rh;until p=rover;
if s=1073741824 then begin getnode:=268435455;goto 10;end;
if lomemmax+2<himemmin then if lomemmax+2<=268435455 then{183:}
begin if himemmin-lomemmax>=1998 then t:=lomemmax+1000 else t:=lomemmax
+1+(himemmin-lomemmax)div 2;if t>268435455 then t:=268435455;
p:=mem[rover+1].hh.lh;q:=lomemmax;mem[p+1].hh.rh:=q;
mem[rover+1].hh.lh:=q;mem[q+1].hh.rh:=rover;mem[q+1].hh.lh:=p;
mem[q].hh.rh:=268435455;mem[q].hh.lh:=t-lomemmax;lomemmax:=t;
mem[lomemmax].hh.rh:=0;mem[lomemmax].hh.lh:=0;rover:=q;goto 20;end{:183}
;overflow(315,memmax+1);40:mem[r].hh.rh:=0;
ifdef('STAT')varused:=varused+s;endif('STAT')getnode:=r;10:end;{:182}
{187:}procedure freenode(p:halfword;s:halfword);var q:halfword;
begin mem[p].hh.lh:=s;mem[p].hh.rh:=268435455;q:=mem[rover+1].hh.lh;
mem[p+1].hh.lh:=q;mem[p+1].hh.rh:=rover;mem[rover+1].hh.lh:=p;
mem[q+1].hh.rh:=p;ifdef('STAT')varused:=varused-s;endif('STAT')end;
{:187}{188:}ifdef('INIMP')procedure sortavail;var p,q,r:halfword;
oldrover:halfword;begin p:=getnode(1073741824);p:=mem[rover+1].hh.rh;
mem[rover+1].hh.rh:=268435455;oldrover:=rover;while p<>oldrover do{189:}
if p<rover then begin q:=p;p:=mem[q+1].hh.rh;mem[q+1].hh.rh:=rover;
rover:=q;end else begin q:=rover;
while mem[q+1].hh.rh<p do q:=mem[q+1].hh.rh;r:=mem[p+1].hh.rh;
mem[p+1].hh.rh:=mem[q+1].hh.rh;mem[q+1].hh.rh:=p;p:=r;end{:189};
p:=rover;
while mem[p+1].hh.rh<>268435455 do begin mem[mem[p+1].hh.rh+1].hh.lh:=p;
p:=mem[p+1].hh.rh;end;mem[p+1].hh.rh:=rover;mem[rover+1].hh.lh:=p;end;
endif('INIMP'){:188}{192:}procedure flushlist(p:halfword);label 30;
var q,r:halfword;begin if p>=himemmin then if p<>memtop then begin r:=p;
repeat q:=r;r:=mem[r].hh.rh;ifdef('STAT')decr(dynused);
endif('STAT')if r<himemmin then goto 30;until r=memtop;
30:mem[q].hh.rh:=avail;avail:=p;end;end;
procedure flushnodelist(p:halfword);var q:halfword;
begin while p<>0 do begin q:=p;p:=mem[p].hh.rh;
if q<himemmin then freenode(q,2)else begin mem[q].hh.rh:=avail;avail:=q;
ifdef('STAT')decr(dynused);endif('STAT')end;end;end;{:192}{195:}
ifdef('TEXMF_DEBUG')procedure checkmem(printlocs:boolean);
label 31,32,33;var p,q,r:halfword;clobbered:boolean;
begin for p:=0 to lomemmax do freearr[p]:=false;
for p:=himemmin to memend do freearr[p]:=false;{196:}p:=avail;q:=0;
clobbered:=false;
while p<>0 do begin if(p>memend)or(p<himemmin)then clobbered:=true else
if freearr[p]then clobbered:=true;if clobbered then begin printnl(316);
printint(q);goto 31;end;freearr[p]:=true;q:=p;p:=mem[q].hh.rh;end;
31:{:196};{197:}p:=rover;q:=0;clobbered:=false;
repeat if(p>=lomemmax)then clobbered:=true else if(mem[p+1].hh.rh>=
lomemmax)then clobbered:=true else if not((mem[p].hh.rh=268435455))or(
mem[p].hh.lh<2)or(p+mem[p].hh.lh>lomemmax)or(mem[mem[p+1].hh.rh+1].hh.lh
<>p)then clobbered:=true;if clobbered then begin printnl(317);
printint(q);goto 32;end;
for q:=p to p+mem[p].hh.lh-1 do begin if freearr[q]then begin printnl(
318);printint(q);goto 32;end;freearr[q]:=true;end;q:=p;
p:=mem[p+1].hh.rh;until p=rover;32:{:197};{198:}p:=0;
while p<=lomemmax do begin if(mem[p].hh.rh=268435455)then begin printnl(
319);printint(p);end;while(p<=lomemmax)and not freearr[p]do incr(p);
while(p<=lomemmax)and freearr[p]do incr(p);end{:198};{571:}q:=5;
p:=mem[q].hh.rh;
while p<>5 do begin if mem[p+1].hh.lh<>q then begin printnl(609);
printint(p);end;p:=mem[p+1].hh.rh;while true do begin r:=mem[p].hh.lh;
q:=p;p:=mem[q].hh.rh;if r=0 then goto 33;
if mem[mem[p].hh.lh+1].int>=mem[r+1].int then begin printnl(610);
printint(p);end;end;33:;end{:571};if printlocs then{199:}begin{201:}
q:=memmax;r:=memmax{:201};printnl(320);
for p:=0 to lomemmax do if not freearr[p]and((p>waslomax)or wasfree[p])
then{200:}begin if p>q+1 then begin if q>r then begin print(321);
printint(q);end;printchar(32);printint(p);r:=p;end;q:=p;end{:200};
for p:=himemmin to memend do if not freearr[p]and((p<washimin)or(p>
wasmemend)or wasfree[p])then{200:}
begin if p>q+1 then begin if q>r then begin print(321);printint(q);end;
printchar(32);printint(p);r:=p;end;q:=p;end{:200};{202:}
if q>r then begin print(321);printint(q);end{:202};end{:199};
for p:=0 to lomemmax do wasfree[p]:=freearr[p];
for p:=himemmin to memend do wasfree[p]:=freearr[p];wasmemend:=memend;
waslomax:=lomemmax;washimin:=himemmin;end;endif('TEXMF_DEBUG'){:195}
{203:}ifdef('TEXMF_DEBUG')procedure searchmem(p:halfword);var q:integer;
begin for q:=0 to lomemmax do begin if mem[q].hh.rh=p then begin printnl
(322);printint(q);printchar(41);end;
if mem[q].hh.lh=p then begin printnl(323);printint(q);printchar(41);end;
end;
for q:=himemmin to memend do begin if mem[q].hh.rh=p then begin printnl(
322);printint(q);printchar(41);end;
if mem[q].hh.lh=p then begin printnl(323);printint(q);printchar(41);end;
end;{227:}
for q:=1 to 9771 do begin if eqtb[q].rh=p then begin printnl(473);
printint(q);printchar(41);end;end{:227};end;endif('TEXMF_DEBUG'){:203}
{207:}procedure printop(c:quarterword);
begin if c<=15 then printtype(c)else case c of 30:print(347);
31:print(348);32:print(349);33:print(350);34:print(351);35:print(352);
36:print(353);37:print(354);38:print(355);39:print(356);40:print(357);
41:print(358);42:print(359);43:print(360);44:print(361);45:print(362);
46:print(363);47:print(364);48:print(365);49:print(366);50:print(367);
51:print(368);52:print(369);53:print(370);54:print(371);55:print(372);
56:print(373);57:print(374);58:print(375);59:print(376);60:print(377);
61:print(378);62:print(379);63:print(380);64:print(381);65:print(382);
66:print(383);67:print(384);68:print(385);69:print(386);70:print(387);
71:print(388);72:print(389);73:print(390);74:print(391);75:print(392);
76:print(393);77:print(394);78:print(395);79:print(396);80:print(397);
81:print(398);82:print(399);83:print(400);84:print(401);85:print(402);
86:print(403);87:print(404);88:print(405);89:printchar(43);
90:printchar(45);91:printchar(42);92:printchar(47);93:print(406);
94:print(310);95:print(407);96:print(408);97:printchar(60);
98:print(409);99:printchar(62);100:print(410);101:printchar(61);
102:print(411);103:print(38);104:print(412);105:print(413);
106:print(414);107:print(415);108:print(416);109:print(417);
110:print(418);111:print(419);112:print(420);113:print(421);
115:print(422);116:print(423);117:print(424);118:print(425);
119:print(426);120:print(427);121:print(428);122:print(429);
others:print(321)end;end;{:207}{212:}procedure fixdateandtime;
begin dateandtime(internal[16],internal[15],internal[14],internal[13]);
internal[16]:=internal[16]*65536;internal[15]:=internal[15]*65536;
internal[14]:=internal[14]*65536;internal[13]:=internal[13]*65536;end;
{:212}{223:}function idlookup(j,l:integer):halfword;label 40;
var h:integer;p:halfword;k:halfword;begin if l=1 then{224:}
begin p:=buffer[j]+1;hash[p].rh:=p-1;goto 40;end{:224};{226:}
h:=buffer[j];for k:=j+1 to j+l-1 do begin h:=h+h+buffer[k];
while h>=7919 do h:=h-7919;end{:226};p:=h+257;
while true do begin if hash[p].rh>0 then if(strstart[nextstr[hash[p].rh]
]-strstart[hash[p].rh])=l then if streqbuf(hash[p].rh,j)then goto 40;
if hash[p].lh=0 then{225:}
begin if hash[p].rh>0 then begin repeat if(hashused=257)then overflow(
472,9500);decr(hashused);until hash[hashused].rh=0;hash[p].lh:=hashused;
p:=hashused;end;
begin if poolptr+l>maxpoolptr then if poolptr+l>poolsize then
docompaction(l)else maxpoolptr:=poolptr+l;end;
for k:=j to j+l-1 do begin strpool[poolptr]:=buffer[k];incr(poolptr);
end;hash[p].rh:=makestring;strref[hash[p].rh]:=127;
ifdef('STAT')incr(stcount);endif('STAT')goto 40;end{:225};p:=hash[p].lh;
end;40:idlookup:=p;end;{:223}{228:}
ifdef('INIMP')procedure primitive(s:strnumber;c:halfword;o:halfword);
var k:poolpointer;j:smallnumber;l:smallnumber;begin k:=strstart[s];
l:=strstart[nextstr[s]]-k;for j:=0 to l-1 do buffer[j]:=strpool[k+j];
cursym:=idlookup(0,l);if s>=256 then begin flushstring(hash[cursym].rh);
hash[cursym].rh:=s;end;eqtb[cursym].lh:=c;eqtb[cursym].rh:=o;end;
endif('INIMP'){:228}{234:}function newnumtok(v:scaled):halfword;
var p:halfword;begin p:=getnode(2);mem[p+1].int:=v;mem[p].hh.b0:=16;
mem[p].hh.b1:=15;newnumtok:=p;end;{:234}{235:}procedure tokenrecycle;
forward;procedure flushtokenlist(p:halfword);var q:halfword;
begin while p<>0 do begin q:=p;p:=mem[p].hh.rh;
if q>=himemmin then begin mem[q].hh.rh:=avail;avail:=q;
ifdef('STAT')decr(dynused);
endif('STAT')end else begin case mem[q].hh.b0 of 1,2,16:;
4:begin if strref[mem[q+1].int]<127 then if strref[mem[q+1].int]>1 then
decr(strref[mem[q+1].int])else flushstring(mem[q+1].int);end;
3,5,7,11,9,6,8,10,14,13,12,17,18,19:begin gpointer:=q;tokenrecycle;end;
others:confusion(503)end;freenode(q,2);end;end;end;{:235}{245:}
procedure deletemacref(p:halfword);
begin if mem[p].hh.lh=0 then flushtokenlist(p)else decr(mem[p].hh.lh);
end;{:245}{246:}{579:}procedure printcmdmod(c,m:integer);
begin case c of{230:}20:print(477);76:print(476);61:print(478);
78:print(475);34:print(479);80:print(58);81:print(44);59:print(480);
62:print(481);29:print(482);79:print(474);83:print(468);28:print(483);
9:print(484);12:print(485);15:print(486);48:print(123);65:print(91);
16:print(487);17:print(488);70:print(489);49:print(321);26:print(490);
10:printchar(92);67:print(125);66:print(93);14:print(491);11:print(492);
82:print(59);19:print(493);77:print(494);30:print(495);72:print(496);
37:print(497);60:print(498);71:print(499);73:print(500);74:print(501);
31:print(502);{:230}{648:}1:if m=0 then print(681)else print(682);
2:print(465);3:print(466);{:648}{656:}
18:if m<=2 then if m=1 then print(695)else if m<1 then print(469)else
print(696)else if m=55 then print(697)else if m=46 then print(698)else
print(699);
7:if m<=1 then if m=1 then print(702)else print(470)else if m=9772 then
print(700)else print(701);{:656}{661:}63:case m of 1:print(704);
2:printchar(64);3:print(705);others:print(703)end;{:661}{668:}
58:if m>=9772 then if m=9772 then print(716)else if m=9922 then print(
717)else print(718)else if m<2 then print(719)else if m=2 then print(720
)else print(721);{:668}{682:}6:if m=0 then print(731)else print(628);
{:682}{713:}4,5:case m of 1:print(758);2:print(467);3:print(759);
others:print(760)end;{:713}{881:}
35,36,39,57,47,52,38,45,56,50,53,54:printop(m);{:881}{1031:}
32:printtype(m);{:1031}{1036:}84:if m=0 then print(960)else print(961);
{:1036}{1042:}25:case m of 0:print(272);1:print(273);2:print(274);
others:print(967)end;{:1042}{1045:}
23:if m=0 then print(968)else print(969);{:1045}{1055:}
24:case m of 0:print(983);1:print(984);2:print(985);3:print(986);
others:print(987)end;{:1055}{1060:}
33,64:begin if c=33 then print(990)else print(991);print(992);
print(hash[m].rh);end;43:if m=0 then print(993)else print(994);
13:print(995);55,46,51:begin printcmdmod(18,c);print(996);println;
showtokenlist(mem[mem[m].hh.rh].hh.rh,0,1000,0);end;8:print(997);
42:print(intname[m]);{:1060}{1070:}
69:if m=1 then print(1007)else if m=0 then print(1006)else print(1008);
68:if m=6 then print(1009)else if m=13 then print(1011)else print(1010);
{:1070}{1084:}21:if m=4 then print(1020)else print(1021);{:1084}{1104:}
27:if m<1 then print(1034)else if m=1 then print(1035)else print(1036);
{:1104}{1133:}22:case m of 0:print(1051);1:print(1052);2:print(1053);
3:print(1054);others:print(1055)end;{:1133}{1140:}
75:case m of 0:print(1073);1:print(1074);2:print(1076);3:print(1078);
5:print(1075);6:print(1077);7:print(1079);11:print(1080);
others:print(1081)end;{:1140}others:print(614)end;end;{:579}
procedure showmacro(p:halfword;q,l:integer);label 10;var r:halfword;
begin p:=mem[p].hh.rh;while mem[p].hh.lh>7 do begin r:=mem[p].hh.rh;
mem[p].hh.rh:=0;showtokenlist(p,0,l,0);mem[p].hh.rh:=r;p:=r;
if l>0 then l:=l-tally else goto 10;end;tally:=0;
case mem[p].hh.lh of 0:print(513);1,2,3:begin printchar(60);
printcmdmod(58,mem[p].hh.lh);print(514);end;4:print(515);5:print(516);
6:print(517);7:print(518);end;showtokenlist(mem[p].hh.rh,q,l-tally,0);
10:end;{:246}{251:}procedure initbignode(p:halfword);var q:halfword;
s:smallnumber;begin s:=bignodesize[mem[p].hh.b0];q:=getnode(s);
repeat s:=s-2;{540:}begin mem[q+s].hh.b0:=19;serialno:=serialno+64;
mem[q+s+1].int:=serialno;end{:540};
mem[q+s].hh.b1:=halfp(s)+sector0[mem[p].hh.b0];mem[q+s].hh.rh:=0;
until s=0;mem[q].hh.rh:=p;mem[p+1].int:=q;end;{:251}{252:}
function idtransform:halfword;var p,q,r:halfword;begin p:=getnode(2);
mem[p].hh.b0:=12;mem[p].hh.b1:=14;mem[p+1].int:=0;initbignode(p);
q:=mem[p+1].int;r:=q+12;repeat r:=r-2;mem[r].hh.b0:=16;mem[r+1].int:=0;
until r=q;mem[q+5].int:=65536;mem[q+11].int:=65536;idtransform:=p;end;
{:252}{253:}procedure newroot(x:halfword);var p:halfword;
begin p:=getnode(2);mem[p].hh.b0:=0;mem[p].hh.b1:=0;mem[p].hh.rh:=x;
eqtb[x].rh:=p;end;{:253}{254:}procedure printvariablename(p:halfword);
label 40,10;var q:halfword;r:halfword;
begin while mem[p].hh.b1>=5 do{256:}
begin case mem[p].hh.b1 of 5:printchar(120);6:printchar(121);
7:print(521);8:print(522);9:print(523);10:print(524);11:print(525);
12:print(526);13:print(527);14:begin print(528);printint(p-0);goto 10;
end;end;print(529);p:=mem[p-sectoroffset[mem[p].hh.b1]].hh.rh;end{:256};
q:=0;while mem[p].hh.b1>1 do{255:}
begin if mem[p].hh.b1=3 then begin r:=newnumtok(mem[p+2].int);
repeat p:=mem[p].hh.rh;until mem[p].hh.b1=4;
end else if mem[p].hh.b1=2 then begin p:=mem[p].hh.rh;goto 40;
end else begin if mem[p].hh.b1<>4 then confusion(520);r:=getavail;
mem[r].hh.lh:=mem[p+2].hh.lh;end;mem[r].hh.rh:=q;q:=r;
40:p:=mem[p+2].hh.rh;end{:255};r:=getavail;mem[r].hh.lh:=mem[p].hh.rh;
mem[r].hh.rh:=q;if mem[p].hh.b1=1 then print(519);
showtokenlist(r,0,2147483647,tally);flushtokenlist(r);10:end;{:254}
{257:}function interesting(p:halfword):boolean;var t:smallnumber;
begin if internal[3]>0 then interesting:=true else begin t:=mem[p].hh.b1
;if t>=5 then if t<>14 then t:=mem[mem[p-sectoroffset[t]].hh.rh].hh.b1;
interesting:=(t<>14);end;end;{:257}{258:}
function newstructure(p:halfword):halfword;var q,r:halfword;
begin case mem[p].hh.b1 of 0:begin q:=mem[p].hh.rh;r:=getnode(2);
eqtb[q].rh:=r;end;3:{259:}begin q:=p;repeat q:=mem[q].hh.rh;
until mem[q].hh.b1=4;q:=mem[q+2].hh.rh;r:=q+1;repeat q:=r;
r:=mem[r].hh.rh;until r=p;r:=getnode(3);mem[q].hh.rh:=r;
mem[r+2].int:=mem[p+2].int;end{:259};4:{260:}begin q:=mem[p+2].hh.rh;
r:=mem[q+1].hh.lh;repeat q:=r;r:=mem[r].hh.rh;until r=p;r:=getnode(3);
mem[q].hh.rh:=r;mem[r+2]:=mem[p+2];
if mem[p+2].hh.lh=0 then begin q:=mem[p+2].hh.rh+1;
while mem[q].hh.rh<>p do q:=mem[q].hh.rh;mem[q].hh.rh:=r;end;end{:260};
others:confusion(530)end;mem[r].hh.rh:=mem[p].hh.rh;mem[r].hh.b0:=21;
mem[r].hh.b1:=mem[p].hh.b1;mem[r+1].hh.lh:=p;mem[p].hh.b1:=2;
q:=getnode(3);mem[p].hh.rh:=q;mem[r+1].hh.rh:=q;mem[q+2].hh.rh:=r;
mem[q].hh.b0:=0;mem[q].hh.b1:=4;mem[q].hh.rh:=9;mem[q+2].hh.lh:=0;
newstructure:=r;end;{:258}{261:}
function findvariable(t:halfword):halfword;label 10;
var p,q,r,s:halfword;pp,qq,rr,ss:halfword;n:integer;saveword:memoryword;
begin p:=mem[t].hh.lh;t:=mem[t].hh.rh;
if eqtb[p].lh mod 85<>43 then begin findvariable:=0;goto 10;end;
if eqtb[p].rh=0 then newroot(p);p:=eqtb[p].rh;pp:=p;
while t<>0 do begin{262:}
if mem[pp].hh.b0<>21 then begin if mem[pp].hh.b0>21 then begin
findvariable:=0;goto 10;end;ss:=newstructure(pp);if p=pp then p:=ss;
pp:=ss;end;if mem[p].hh.b0<>21 then p:=newstructure(p){:262};
if t<himemmin then{263:}begin n:=mem[t+1].int;
pp:=mem[mem[pp+1].hh.lh].hh.rh;q:=mem[mem[p+1].hh.lh].hh.rh;
saveword:=mem[q+2];mem[q+2].int:=2147483647;s:=p+1;repeat r:=s;
s:=mem[s].hh.rh;until n<=mem[s+2].int;
if n=mem[s+2].int then p:=s else begin p:=getnode(3);mem[r].hh.rh:=p;
mem[p].hh.rh:=s;mem[p+2].int:=n;mem[p].hh.b1:=3;mem[p].hh.b0:=0;end;
mem[q+2]:=saveword;end{:263}else{264:}begin n:=mem[t].hh.lh;
ss:=mem[pp+1].hh.lh;repeat rr:=ss;ss:=mem[ss].hh.rh;
until n<=mem[ss+2].hh.lh;if n<mem[ss+2].hh.lh then begin qq:=getnode(3);
mem[rr].hh.rh:=qq;mem[qq].hh.rh:=ss;mem[qq+2].hh.lh:=n;mem[qq].hh.b1:=4;
mem[qq].hh.b0:=0;mem[qq+2].hh.rh:=pp;ss:=qq;end;
if p=pp then begin p:=ss;pp:=ss;end else begin pp:=ss;s:=mem[p+1].hh.lh;
repeat r:=s;s:=mem[s].hh.rh;until n<=mem[s+2].hh.lh;
if n=mem[s+2].hh.lh then p:=s else begin q:=getnode(3);mem[r].hh.rh:=q;
mem[q].hh.rh:=s;mem[q+2].hh.lh:=n;mem[q].hh.b1:=4;mem[q].hh.b0:=0;
mem[q+2].hh.rh:=p;p:=q;end;end;end{:264};t:=mem[t].hh.rh;end;
if mem[pp].hh.b0>=21 then if mem[pp].hh.b0=21 then pp:=mem[pp+1].hh.lh
else begin findvariable:=0;goto 10;end;
if mem[p].hh.b0=21 then p:=mem[p+1].hh.lh;
if mem[p].hh.b0=0 then begin if mem[pp].hh.b0=0 then begin mem[pp].hh.b0
:=15;mem[pp+1].int:=0;end;mem[p].hh.b0:=mem[pp].hh.b0;mem[p+1].int:=0;
end;findvariable:=p;10:end;{:261}{265:}{276:}
procedure prpath(h:halfword);label 30,31;var p,q:halfword;begin p:=h;
repeat q:=mem[p].hh.rh;if(p=0)or(q=0)then begin printnl(260);goto 30;
end;{277:}printtwo(mem[p+1].int,mem[p+2].int);
case mem[p].hh.b1 of 0:begin if mem[p].hh.b0=4 then print(532);
if(mem[q].hh.b0<>0)or(q<>h)then q:=0;goto 31;end;1:{280:}
begin print(538);printtwo(mem[p+5].int,mem[p+6].int);print(537);
if mem[q].hh.b0<>1 then print(539)else printtwo(mem[q+3].int,mem[q+4].
int);goto 31;end{:280};4:{281:}
if(mem[p].hh.b0<>1)and(mem[p].hh.b0<>4)then print(532){:281};3,2:{282:}
begin if mem[p].hh.b0=4 then print(539);
if mem[p].hh.b1=3 then begin print(535);printscaled(mem[p+5].int);
end else begin nsincos(mem[p+5].int);printchar(123);printscaled(ncos);
printchar(44);printscaled(nsin);end;printchar(125);end{:282};
others:print(260)end;
if mem[q].hh.b0<=1 then print(533)else if(mem[p+6].int<>65536)or(mem[q+4
].int<>65536)then{279:}begin print(536);
if mem[p+6].int<0 then print(478);printscaled(abs(mem[p+6].int));
if mem[p+6].int<>mem[q+4].int then begin print(537);
if mem[q+4].int<0 then print(478);printscaled(abs(mem[q+4].int));end;
end{:279};31:{:277};p:=q;if(p<>h)or(mem[h].hh.b0<>0)then{278:}
begin printnl(534);if mem[p].hh.b0=2 then begin nsincos(mem[p+3].int);
printchar(123);printscaled(ncos);printchar(44);printscaled(nsin);
printchar(125);end else if mem[p].hh.b0=3 then begin print(535);
printscaled(mem[p+3].int);printchar(125);end;end{:278};until p=h;
if mem[h].hh.b0<>0 then print(400);30:end;{:276}{283:}
procedure printpath(h:halfword;s:strnumber;nuline:boolean);
begin printdiagnostic(540,s,nuline);println;prpath(h);
enddiagnostic(true);end;{:283}{363:}procedure prpen(h:halfword);
label 30;var p,q:halfword;begin if(h=mem[h].hh.rh)then{365:}
begin print(549);printscaled(mem[h+1].int);printchar(44);
printscaled(mem[h+2].int);printchar(44);
printscaled(mem[h+3].int-mem[h+1].int);printchar(44);
printscaled(mem[h+5].int-mem[h+1].int);printchar(44);
printscaled(mem[h+4].int-mem[h+2].int);printchar(44);
printscaled(mem[h+6].int-mem[h+2].int);printchar(41);end{:365}
else begin p:=h;repeat printtwo(mem[p+1].int,mem[p+2].int);printnl(548);
{364:}q:=mem[p].hh.rh;if(q=0)or(mem[q].hh.lh<>p)then begin printnl(260);
goto 30;end;p:=q{:364};until p=h;print(400);end;30:end;{:363}{366:}
procedure printpen(h:halfword;s:strnumber;nuline:boolean);
begin printdiagnostic(550,s,nuline);println;prpen(h);
enddiagnostic(true);end;{:366}{417:}{397:}
function sqrtdet(a,b,c,d:scaled):scaled;var maxabs:scaled;s:integer;
begin{398:}maxabs:=abs(a);if abs(b)>maxabs then maxabs:=abs(b);
if abs(c)>maxabs then maxabs:=abs(c);
if abs(d)>maxabs then maxabs:=abs(d){:398};s:=64;
while(maxabs<268435456)and(s>1)do begin a:=a+a;b:=b+b;c:=c+c;d:=d+d;
maxabs:=maxabs+maxabs;s:=halfp(s);end;
sqrtdet:=s*squarert(abs(takefraction(a,d)-takefraction(b,c)));end;
function getpenscale(p:halfword):scaled;
begin getpenscale:=sqrtdet(mem[p+3].int-mem[p+1].int,mem[p+5].int-mem[p
+1].int,mem[p+4].int-mem[p+2].int,mem[p+6].int-mem[p+2].int);end;{:397}
{421:}{422:}procedure printcompactnode(p:halfword;k:smallnumber);
var q:halfword;begin q:=p+k-1;printchar(40);
while p<=q do begin printscaled(mem[p].int);if p<q then printchar(44);
incr(p);end;printchar(41);end;{:422}procedure printobjcolor(p:halfword);
begin if(mem[p+2].int>0)or(mem[p+3].int>0)or(mem[p+4].int>0)then begin
print(565);printcompactnode(p+2,3);end;end;{:421}{425:}
function dashoffset(h:halfword):scaled;var x:scaled;
begin if(mem[h].hh.rh=2)or(mem[h+1].int<0)then confusion(573);
if mem[h+1].int=0 then x:=0 else begin x:=-(mem[mem[h].hh.rh+1].int mod
mem[h+1].int);if x<0 then x:=x+mem[h+1].int;end;dashoffset:=x;end;{:425}
procedure printedges(h:halfword;s:strnumber;nuline:boolean);
var p:halfword;hh,pp:halfword;scf:scaled;oktodash:boolean;
begin printdiagnostic(552,s,nuline);p:=h+7;
while mem[p].hh.rh<>0 do begin p:=mem[p].hh.rh;println;
case mem[p].hh.b0 of{418:}1:begin print(555);printobjcolor(p);
printchar(58);println;prpath(mem[p+1].hh.rh);println;
if(mem[p+1].hh.lh<>0)then begin{419:}
case mem[p].hh.b1 of 0:begin print(557);printscaled(mem[p+5].int);end;
1:print(558);2:print(559);others:print(560);end{:419};print(556);
println;prpen(mem[p+1].hh.lh);end;end;{:418}{423:}2:begin print(566);
printobjcolor(p);printchar(58);println;prpath(mem[p+1].hh.rh);
if mem[p+6].hh.rh<>0 then begin printnl(567);{424:}
oktodash:=(mem[p+1].hh.lh=mem[mem[p+1].hh.lh].hh.rh);
if not oktodash then scf:=65536 else scf:=mem[p+7].int;
hh:=mem[p+6].hh.rh;pp:=mem[hh].hh.rh;
if(pp=2)or(mem[hh+1].int<0)then print(568)else begin mem[3].int:=mem[pp
+1].int+mem[hh+1].int;while pp<>2 do begin print(569);
printscaled(takescaled(mem[pp+2].int-mem[pp+1].int,scf));print(570);
printscaled(takescaled(mem[mem[pp].hh.rh+1].int-mem[pp+2].int,scf));
pp:=mem[pp].hh.rh;if pp<>2 then printchar(32);end;print(571);
printscaled(-takescaled(dashoffset(hh),scf));
if not oktodash or(mem[hh+1].int=0)then print(572);end{:424};end;
println;{420:}case mem[p+6].hh.b0 of 0:print(561);1:print(562);
2:print(563);others:print(539)end;print(564);{419:}
case mem[p].hh.b1 of 0:begin print(557);printscaled(mem[p+5].int);end;
1:print(558);2:print(559);others:print(560);end{:419}{:420};print(556);
println;if mem[p+1].hh.lh=0 then print(260)else prpen(mem[p+1].hh.lh);
end;{:423}{426:}3:begin printchar(34);print(mem[p+1].hh.rh);print(574);
print(fontname[mem[p+1].hh.lh]);printchar(34);println;printobjcolor(p);
print(575);printcompactnode(p+8,6);end;{:426}{427:}4:begin print(576);
println;prpath(mem[p+1].hh.rh);end;6:print(577);{:427}{428:}
5:begin print(578);println;prpath(mem[p+1].hh.rh);end;7:print(579);
{:428}others:begin print(553);end end;end;printnl(554);
if p<>mem[h+7].hh.lh then print(63);enddiagnostic(true);end;{:417}{543:}
procedure printdependency(p:halfword;t:smallnumber);label 10;
var v:integer;pp,q:halfword;begin pp:=p;
while true do begin v:=abs(mem[p+1].int);q:=mem[p].hh.lh;
if q=0 then begin if(v<>0)or(p=pp)then begin if mem[p+1].int>0 then if p
<>pp then printchar(43);printscaled(mem[p+1].int);end;goto 10;end;{544:}
if mem[p+1].int<0 then printchar(45)else if p<>pp then printchar(43);
if t=17 then v:=roundfraction(v);if v<>65536 then printscaled(v){:544};
if mem[q].hh.b0<>19 then confusion(600);printvariablename(q);
v:=mem[q+1].int mod 64;while v>0 do begin print(601);v:=v-2;end;
p:=mem[p].hh.rh;end;10:end;{:543}{789:}{793:}
procedure printdp(t:smallnumber;p:halfword;verbosity:smallnumber);
var q:halfword;begin q:=mem[p].hh.rh;
if(mem[q].hh.lh=0)or(verbosity>0)then printdependency(p,t)else print(814
);end;{:793}{787:}function stashcurexp:halfword;var p:halfword;
begin case curtype of 3,5,7,11,9,12,13,14,17,18,19:p:=curexp;
others:begin p:=getnode(2);mem[p].hh.b1:=14;mem[p].hh.b0:=curtype;
mem[p+1].int:=curexp;end end;curtype:=1;mem[p].hh.rh:=1;stashcurexp:=p;
end;{:787}{788:}procedure unstashcurexp(p:halfword);
begin curtype:=mem[p].hh.b0;
case curtype of 3,5,7,11,9,12,13,14,17,18,19:curexp:=p;
others:begin curexp:=mem[p+1].int;freenode(p,2);end end;end;{:788}
procedure printexp(p:halfword;verbosity:smallnumber);
var restorecurexp:boolean;t:smallnumber;v:integer;q:halfword;
begin if p<>0 then restorecurexp:=false else begin p:=stashcurexp;
restorecurexp:=true;end;t:=mem[p].hh.b0;
if t<17 then v:=mem[p+1].int else if t<19 then v:=mem[p+1].hh.rh;{790:}
case t of 1:print(324);2:if v=30 then print(347)else print(348);
3,5,7,11,9,15:{794:}begin printtype(t);if v<>0 then begin printchar(32);
while(mem[v].hh.b1=14)and(v<>p)do v:=mem[v+1].int;printvariablename(v);
end;end{:794};4:begin printchar(34);print(v);printchar(34);end;
6,8,10:{792:}
if verbosity<=1 then printtype(t)else begin if selector=10 then if
internal[12]<=0 then begin selector:=8;printtype(t);print(813);
selector:=10;end;case t of 6:printpen(v,284,false);
8:printpath(v,284,false);10:printedges(v,284,false);end;end{:792};
12,13,14:if v=0 then printtype(t)else{791:}begin printchar(40);
q:=v+bignodesize[t];
repeat if mem[v].hh.b0=16 then printscaled(mem[v+1].int)else if mem[v].
hh.b0=19 then printvariablename(v)else printdp(mem[v].hh.b0,mem[v+1].hh.
rh,verbosity);v:=v+2;if v<>q then printchar(44);until v=q;printchar(41);
end{:791};16:printscaled(v);17,18:printdp(t,v,verbosity);
19:printvariablename(p);others:confusion(812)end{:790};
if restorecurexp then unstashcurexp(p);end;{:789}{795:}
procedure disperr(p:halfword;s:strnumber);begin if interaction=3 then;
printnl(804);printexp(p,1);if s<>284 then begin printnl(262);print(s);
end;end;{:795}{548:}function pplusfq(p:halfword;f:integer;q:halfword;
t,tt:smallnumber):halfword;label 30;var pp,qq:halfword;r,s:halfword;
threshold:integer;v:integer;
begin if t=17 then threshold:=2685 else threshold:=8;r:=memtop-1;
pp:=mem[p].hh.lh;qq:=mem[q].hh.lh;
while true do if pp=qq then if pp=0 then goto 30 else{549:}
begin if tt=17 then v:=mem[p+1].int+takefraction(f,mem[q+1].int)else v:=
mem[p+1].int+takescaled(f,mem[q+1].int);mem[p+1].int:=v;s:=p;
p:=mem[p].hh.rh;
if abs(v)<threshold then freenode(s,2)else begin if abs(v)>=626349397
then if watchcoefs then begin mem[qq].hh.b0:=0;fixneeded:=true;end;
mem[r].hh.rh:=s;r:=s;end;pp:=mem[p].hh.lh;q:=mem[q].hh.rh;
qq:=mem[q].hh.lh;end{:549}else if mem[pp+1].int<mem[qq+1].int then{550:}
begin if tt=17 then v:=takefraction(f,mem[q+1].int)else v:=takescaled(f,
mem[q+1].int);if abs(v)>halfp(threshold)then begin s:=getnode(2);
mem[s].hh.lh:=qq;mem[s+1].int:=v;
if abs(v)>=626349397 then if watchcoefs then begin mem[qq].hh.b0:=0;
fixneeded:=true;end;mem[r].hh.rh:=s;r:=s;end;q:=mem[q].hh.rh;
qq:=mem[q].hh.lh;end{:550}else begin mem[r].hh.rh:=p;r:=p;
p:=mem[p].hh.rh;pp:=mem[p].hh.lh;end;
30:if t=17 then mem[p+1].int:=slowadd(mem[p+1].int,takefraction(mem[q+1]
.int,f))else mem[p+1].int:=slowadd(mem[p+1].int,takescaled(mem[q+1].int,
f));mem[r].hh.rh:=p;depfinal:=p;pplusfq:=mem[memtop-1].hh.rh;end;{:548}
{554:}function poverv(p:halfword;v:scaled;t0,t1:smallnumber):halfword;
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
end;{:554}{556:}procedure valtoobig(x:scaled);
begin if internal[30]>0 then begin begin if interaction=3 then;
printnl(262);print(602);end;printscaled(x);printchar(41);
begin helpptr:=4;helpline[3]:=603;helpline[2]:=604;helpline[1]:=605;
helpline[0]:=606;end;error;end;end;{:556}{557:}
procedure makeknown(p,q:halfword);var t:17..18;
begin mem[mem[q].hh.rh+1].hh.lh:=mem[p+1].hh.lh;
mem[mem[p+1].hh.lh].hh.rh:=mem[q].hh.rh;t:=mem[p].hh.b0;
mem[p].hh.b0:=16;mem[p+1].int:=mem[q+1].int;freenode(q,2);
if abs(mem[p+1].int)>=268435456 then valtoobig(mem[p+1].int);
if internal[2]>0 then if interesting(p)then begin begindiagnostic;
printnl(607);printvariablename(p);printchar(61);
printscaled(mem[p+1].int);enddiagnostic(false);end;
if curexp=p then if curtype=t then begin curtype:=16;
curexp:=mem[p+1].int;freenode(p,2);end;end;{:557}{558:}
procedure fixdependencies;label 30;var p,q,r,s,t:halfword;x:halfword;
begin r:=mem[5].hh.rh;s:=0;while r<>5 do begin t:=r;{559:}r:=t+1;
while true do begin q:=mem[r].hh.rh;x:=mem[q].hh.lh;if x=0 then goto 30;
if mem[x].hh.b0<=1 then begin if mem[x].hh.b0<1 then begin p:=getavail;
mem[p].hh.rh:=s;s:=p;mem[s].hh.lh:=x;mem[x].hh.b0:=1;end;
mem[q+1].int:=mem[q+1].int div 4;
if mem[q+1].int=0 then begin mem[r].hh.rh:=mem[q].hh.rh;freenode(q,2);
q:=r;end;end;r:=q;end;30:{:559};r:=mem[q].hh.rh;
if q=mem[t+1].hh.rh then makeknown(t,q);end;
while s<>0 do begin p:=mem[s].hh.rh;x:=mem[s].hh.lh;
begin mem[s].hh.rh:=avail;avail:=s;ifdef('STAT')decr(dynused);
endif('STAT')end;s:=p;mem[x].hh.b0:=19;mem[x+1].int:=mem[x+1].int+2;end;
fixneeded:=false;end;{:558}{288:}procedure tossknotlist(p:halfword);
var q:halfword;r:halfword;begin q:=p;repeat r:=mem[q].hh.rh;
freenode(q,7);q:=r;until q=p;end;{:288}{406:}{407:}
procedure flushdashlist(h:halfword);var p,q:halfword;
begin q:=mem[h].hh.rh;while q<>2 do begin p:=q;q:=mem[q].hh.rh;
freenode(p,3);end;mem[h].hh.rh:=2;end;{:407}{408:}
function tossgrobject(p:halfword):halfword;var e:halfword;begin e:=0;
{409:}case mem[p].hh.b0 of 1:begin tossknotlist(mem[p+1].hh.rh);
if mem[p+1].hh.lh<>0 then tossknotlist(mem[p+1].hh.lh);end;
2:begin tossknotlist(mem[p+1].hh.rh);
if mem[p+1].hh.lh<>0 then tossknotlist(mem[p+1].hh.lh);
e:=mem[p+6].hh.rh;end;
3:begin if strref[mem[p+1].hh.rh]<127 then if strref[mem[p+1].hh.rh]>1
then decr(strref[mem[p+1].hh.rh])else flushstring(mem[p+1].hh.rh);end;
4,5:tossknotlist(mem[p+1].hh.rh);6,7:;end;{:409};
freenode(p,grobjectsize[mem[p].hh.b0]);tossgrobject:=e;end;{:408}
procedure tossedges(h:halfword);var p,q:halfword;r:halfword;
begin flushdashlist(h);q:=mem[h+7].hh.rh;while(q<>0)do begin p:=q;
q:=mem[q].hh.rh;r:=tossgrobject(p);
if r<>0 then if mem[r].hh.lh=0 then tossedges(r)else decr(mem[r].hh.lh);
end;freenode(h,8);end;{:406}{574:}procedure ringdelete(p:halfword);
var q:halfword;begin q:=mem[p+1].int;
if q<>0 then if q<>p then begin while mem[q+1].int<>p do q:=mem[q+1].int
;mem[q+1].int:=mem[p+1].int;end;end;{:574}{797:}
procedure recyclevalue(p:halfword);label 30;var t:smallnumber;v:integer;
vv:integer;q,r,s,pp:halfword;begin t:=mem[p].hh.b0;
if t<17 then v:=mem[p+1].int;case t of 0,1,2,16,15:;
3,5,7,11,9:ringdelete(p);
4:begin if strref[v]<127 then if strref[v]>1 then decr(strref[v])else
flushstring(v);end;8,6:tossknotlist(v);
10:if mem[v].hh.lh=0 then tossedges(v)else decr(mem[v].hh.lh);
14,13,12:{798:}if v<>0 then begin q:=v+bignodesize[t];repeat q:=q-2;
recyclevalue(q);until q=v;freenode(v,bignodesize[t]);end{:798};
17,18:{799:}begin q:=mem[p+1].hh.rh;
while mem[q].hh.lh<>0 do q:=mem[q].hh.rh;
mem[mem[p+1].hh.lh].hh.rh:=mem[q].hh.rh;
mem[mem[q].hh.rh+1].hh.lh:=mem[p+1].hh.lh;mem[q].hh.rh:=0;
flushnodelist(mem[p+1].hh.rh);end{:799};19:{800:}begin maxc[17]:=0;
maxc[18]:=0;maxlink[17]:=0;maxlink[18]:=0;q:=mem[5].hh.rh;
while q<>5 do begin s:=q+1;while true do begin r:=mem[s].hh.rh;
if mem[r].hh.lh=0 then goto 30;
if mem[r].hh.lh<>p then s:=r else begin t:=mem[q].hh.b0;
mem[s].hh.rh:=mem[r].hh.rh;mem[r].hh.lh:=q;
if abs(mem[r+1].int)>maxc[t]then{802:}
begin if maxc[t]>0 then begin mem[maxptr[t]].hh.rh:=maxlink[t];
maxlink[t]:=maxptr[t];end;maxc[t]:=abs(mem[r+1].int);maxptr[t]:=r;
end{:802}else begin mem[r].hh.rh:=maxlink[t];maxlink[t]:=r;end;end;end;
30:q:=mem[r].hh.rh;end;if(maxc[17]>0)or(maxc[18]>0)then{803:}
begin if(maxc[17]div 4096>=maxc[18])then t:=17 else t:=18;{804:}
s:=maxptr[t];pp:=mem[s].hh.lh;v:=mem[s+1].int;
if t=17 then mem[s+1].int:=-268435456 else mem[s+1].int:=-65536;
r:=mem[pp+1].hh.rh;mem[s].hh.rh:=r;
while mem[r].hh.lh<>0 do r:=mem[r].hh.rh;q:=mem[r].hh.rh;
mem[r].hh.rh:=0;mem[q+1].hh.lh:=mem[pp+1].hh.lh;
mem[mem[pp+1].hh.lh].hh.rh:=q;begin mem[pp].hh.b0:=19;
serialno:=serialno+64;mem[pp+1].int:=serialno;end;
if curexp=pp then if curtype=t then curtype:=19;
if internal[2]>0 then{805:}if interesting(p)then begin begindiagnostic;
printnl(816);if v>0 then printchar(45);
if t=17 then vv:=roundfraction(maxc[17])else vv:=maxc[18];
if vv<>65536 then printscaled(vv);printvariablename(p);
while mem[p+1].int mod 64>0 do begin print(601);
mem[p+1].int:=mem[p+1].int-2;end;
if t=17 then printchar(61)else print(817);printdependency(s,t);
enddiagnostic(false);end{:805}{:804};t:=35-t;
if maxc[t]>0 then begin mem[maxptr[t]].hh.rh:=maxlink[t];
maxlink[t]:=maxptr[t];end;if t<>17 then{806:}
for t:=17 to 18 do begin r:=maxlink[t];
while r<>0 do begin q:=mem[r].hh.lh;
mem[q+1].hh.rh:=pplusfq(mem[q+1].hh.rh,makefraction(mem[r+1].int,-v),s,t
,17);if mem[q+1].hh.rh=depfinal then makeknown(q,depfinal);q:=r;
r:=mem[r].hh.rh;freenode(q,2);end;end{:806}else{807:}
for t:=17 to 18 do begin r:=maxlink[t];
while r<>0 do begin q:=mem[r].hh.lh;
if t=17 then begin if curexp=q then if curtype=17 then curtype:=18;
mem[q+1].hh.rh:=poverv(mem[q+1].hh.rh,65536,17,18);mem[q].hh.b0:=18;
mem[r+1].int:=roundfraction(mem[r+1].int);end;
mem[q+1].hh.rh:=pplusfq(mem[q+1].hh.rh,makescaled(mem[r+1].int,-v),s,18,
18);if mem[q+1].hh.rh=depfinal then makeknown(q,depfinal);q:=r;
r:=mem[r].hh.rh;freenode(q,2);end;end{:807};flushnodelist(s);
if fixneeded then fixdependencies;begin if aritherror then cleararith;
end;end{:803};end{:800};20,21:confusion(815);
22,23:deletemacref(mem[p+1].int);end;mem[p].hh.b0:=0;end;{:797}{796:}
procedure flushcurexp(v:scaled);
begin case curtype of 3,5,7,11,9,12,13,14,17,18,19:begin recyclevalue(
curexp);freenode(curexp,2);end;
4:begin if strref[curexp]<127 then if strref[curexp]>1 then decr(strref[
curexp])else flushstring(curexp);end;6,8:tossknotlist(curexp);
10:if mem[curexp].hh.lh=0 then tossedges(curexp)else decr(mem[curexp].hh
.lh);others:end;curtype:=16;curexp:=v;end;{:796}{808:}
procedure flusherror(v:scaled);begin error;flushcurexp(v);end;
procedure backerror;forward;procedure getxnext;forward;
procedure putgeterror;begin backerror;getxnext;end;
procedure putgetflusherror(v:scaled);begin putgeterror;flushcurexp(v);
end;{:808}{266:}procedure flushbelowvariable(p:halfword);
var q,r:halfword;
begin if mem[p].hh.b0<>21 then recyclevalue(p)else begin q:=mem[p+1].hh.
rh;while mem[q].hh.b1=3 do begin flushbelowvariable(q);r:=q;
q:=mem[q].hh.rh;freenode(r,3);end;r:=mem[p+1].hh.lh;q:=mem[r].hh.rh;
recyclevalue(r);if mem[p].hh.b1<=1 then freenode(r,2)else freenode(r,3);
repeat flushbelowvariable(q);r:=q;q:=mem[q].hh.rh;freenode(r,3);
until q=9;mem[p].hh.b0:=0;end;end;{:266}
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
21 then p:=mem[p+1].hh.lh;recyclevalue(p);end;10:end;{:265}{267:}
function undtype(p:halfword):smallnumber;
begin case mem[p].hh.b0 of 0,1:undtype:=0;2,3:undtype:=3;4,5:undtype:=5;
6,7:undtype:=7;8,9:undtype:=9;10,11:undtype:=11;
12,13,14,15:undtype:=mem[p].hh.b0;16,17,18,19:undtype:=15;end;end;{:267}
{268:}procedure clearsymbol(p:halfword;saving:boolean);var q:halfword;
begin q:=eqtb[p].rh;
case eqtb[p].lh mod 85 of 13,55,46,51:if not saving then deletemacref(q)
;43:if q<>0 then if saving then mem[q].hh.b1:=1 else begin
flushbelowvariable(q);freenode(q,2);end;others:end;eqtb[p]:=eqtb[9771];
end;{:268}{271:}procedure savevariable(q:halfword);var p:halfword;
begin if saveptr<>0 then begin p:=getnode(2);mem[p].hh.lh:=q;
mem[p].hh.rh:=saveptr;mem[p+1].hh:=eqtb[q];saveptr:=p;end;
clearsymbol(q,(saveptr<>0));end;{:271}{272:}
procedure saveinternal(q:halfword);var p:halfword;
begin if saveptr<>0 then begin p:=getnode(2);mem[p].hh.lh:=9771+q;
mem[p].hh.rh:=saveptr;mem[p+1].int:=internal[q];saveptr:=p;end;end;
{:272}{273:}procedure unsave;var q:halfword;p:halfword;
begin while mem[saveptr].hh.lh<>0 do begin q:=mem[saveptr].hh.lh;
if q>9771 then begin if internal[7]>0 then begin begindiagnostic;
printnl(531);print(intname[q-(9771)]);printchar(61);
printscaled(mem[saveptr+1].int);printchar(125);enddiagnostic(false);end;
internal[q-(9771)]:=mem[saveptr+1].int;
end else begin if internal[7]>0 then begin begindiagnostic;printnl(531);
print(hash[q].rh);printchar(125);enddiagnostic(false);end;
clearsymbol(q,false);eqtb[q]:=mem[saveptr+1].hh;
if eqtb[q].lh mod 85=43 then begin p:=eqtb[q].rh;
if p<>0 then mem[p].hh.b1:=0;end;end;p:=mem[saveptr].hh.rh;
freenode(saveptr,2);saveptr:=p;end;p:=mem[saveptr].hh.rh;
begin mem[saveptr].hh.rh:=avail;avail:=saveptr;
ifdef('STAT')decr(dynused);endif('STAT')end;saveptr:=p;end;{:273}{284:}
function copyknot(p:halfword):halfword;var q:halfword;k:0..6;
begin q:=getnode(7);for k:=0 to 6 do mem[q+k]:=mem[p+k];copyknot:=q;end;
{:284}{285:}function copypath(p:halfword):halfword;var q,pp,qq:halfword;
begin q:=copyknot(p);qq:=q;pp:=mem[p].hh.rh;
while pp<>p do begin mem[qq].hh.rh:=copyknot(pp);qq:=mem[qq].hh.rh;
pp:=mem[pp].hh.rh;end;mem[qq].hh.rh:=q;copypath:=q;end;{:285}{286:}
function htapypoc(p:halfword):halfword;label 10;var q,pp,qq,rr:halfword;
begin q:=getnode(7);qq:=q;pp:=p;
while true do begin mem[qq].hh.b1:=mem[pp].hh.b0;
mem[qq].hh.b0:=mem[pp].hh.b1;mem[qq+1].int:=mem[pp+1].int;
mem[qq+2].int:=mem[pp+2].int;mem[qq+5].int:=mem[pp+3].int;
mem[qq+6].int:=mem[pp+4].int;mem[qq+3].int:=mem[pp+5].int;
mem[qq+4].int:=mem[pp+6].int;
if mem[pp].hh.rh=p then begin mem[q].hh.rh:=qq;pathtail:=pp;htapypoc:=q;
goto 10;end;rr:=getnode(7);mem[rr].hh.rh:=qq;qq:=rr;pp:=mem[pp].hh.rh;
end;10:end;{:286}{289:}{305:}{317:}
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
curlratio:=makefraction(num,denom);end;{:317}{320:}
procedure setcontrols(p,q:halfword;k:integer);var rr,ss:fraction;
lt,rt:scaled;sine:fraction;begin lt:=abs(mem[q+4].int);
rt:=abs(mem[p+6].int);rr:=velocity(st,ct,sf,cf,rt);
ss:=velocity(sf,cf,st,ct,lt);
if(mem[p+6].int<0)or(mem[q+4].int<0)then{321:}
if((st>=0)and(sf>=0))or((st<=0)and(sf<=0))then begin sine:=takefraction(
abs(st),cf)+takefraction(abs(sf),ct);
if sine>0 then begin sine:=takefraction(sine,268500992);
if mem[p+6].int<0 then if abvscd(abs(sf),268435456,rr,sine)<0 then rr:=
makefraction(abs(sf),sine);
if mem[q+4].int<0 then if abvscd(abs(st),268435456,ss,sine)<0 then ss:=
makefraction(abs(st),sine);end;end{:321};
mem[p+5].int:=mem[p+1].int+takefraction(takefraction(deltax[k],ct)-
takefraction(deltay[k],st),rr);
mem[p+6].int:=mem[p+2].int+takefraction(takefraction(deltay[k],ct)+
takefraction(deltax[k],st),rr);
mem[q+3].int:=mem[q+1].int-takefraction(takefraction(deltax[k],cf)+
takefraction(deltay[k],sf),ss);
mem[q+4].int:=mem[q+2].int-takefraction(takefraction(deltay[k],cf)-
takefraction(deltax[k],sf),ss);mem[p].hh.b1:=1;mem[q].hh.b0:=1;end;
{:320}procedure solvechoices(p,q:halfword;n:halfword);label 40,10;
var k:0..pathsize;r,s,t:halfword;{307:}aa,bb,cc,ff,acc:fraction;
dd,ee:scaled;lt,rt:scaled;{:307}begin k:=0;s:=p;
while true do begin t:=mem[s].hh.rh;if k=0 then{306:}
case mem[s].hh.b1 of 2:if mem[t].hh.b0=2 then{322:}
begin aa:=narg(deltax[0],deltay[0]);nsincos(mem[p+5].int-aa);ct:=ncos;
st:=nsin;nsincos(mem[q+3].int-aa);cf:=ncos;sf:=-nsin;setcontrols(p,q,0);
goto 10;end{:322}else{314:}
begin vv[0]:=mem[s+5].int-narg(deltax[0],deltay[0]);
if abs(vv[0])>188743680 then if vv[0]>0 then vv[0]:=vv[0]-377487360 else
vv[0]:=vv[0]+377487360;uu[0]:=0;ww[0]:=0;end{:314};
3:if mem[t].hh.b0=3 then{323:}begin mem[p].hh.b1:=1;mem[q].hh.b0:=1;
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
end{:323}else{315:}begin cc:=mem[s+5].int;lt:=abs(mem[t+4].int);
rt:=abs(mem[s+6].int);
if(rt=65536)and(lt=65536)then uu[0]:=makefraction(cc+cc+65536,cc+131072)
else uu[0]:=curlratio(cc,rt,lt);vv[0]:=-takefraction(psi[1],uu[0]);
ww[0]:=0;end{:315};4:begin uu[0]:=0;vv[0]:=0;ww[0]:=268435456;end;
end{:306}else case mem[s].hh.b0 of 5,4:{308:}begin{309:}
if abs(mem[r+6].int)=65536 then begin aa:=134217728;dd:=2*delta[k];
end else begin aa:=makefraction(65536,3*abs(mem[r+6].int)-65536);
dd:=takefraction(delta[k],805306368-makefraction(65536,abs(mem[r+6].int)
));end;if abs(mem[t+4].int)=65536 then begin bb:=134217728;
ee:=2*delta[k-1];
end else begin bb:=makefraction(65536,3*abs(mem[t+4].int)-65536);
ee:=takefraction(delta[k-1],805306368-makefraction(65536,abs(mem[t+4].
int)));end;cc:=268435456-takefraction(uu[k-1],aa){:309};{310:}
dd:=takefraction(dd,cc);lt:=abs(mem[s+4].int);rt:=abs(mem[s+6].int);
if lt<>rt then if lt<rt then begin ff:=makefraction(lt,rt);
ff:=takefraction(ff,ff);dd:=takefraction(dd,ff);
end else begin ff:=makefraction(rt,lt);ff:=takefraction(ff,ff);
ee:=takefraction(ee,ff);end;ff:=makefraction(ee,ee+dd){:310};
uu[k]:=takefraction(ff,bb);{311:}acc:=-takefraction(psi[k+1],uu[k]);
if mem[r].hh.b1=3 then begin ww[k]:=0;
vv[k]:=acc-takefraction(psi[1],268435456-ff);
end else begin ff:=makefraction(268435456-ff,cc);
acc:=acc-takefraction(psi[k],ff);ff:=takefraction(ff,aa);
vv[k]:=acc-takefraction(vv[k-1],ff);
if ww[k-1]=0 then ww[k]:=0 else ww[k]:=-takefraction(ww[k-1],ff);
end{:311};if mem[s].hh.b0=5 then{312:}begin aa:=0;bb:=268435456;
repeat decr(k);if k=0 then k:=n;aa:=vv[k]-takefraction(aa,uu[k]);
bb:=ww[k]-takefraction(bb,uu[k]);until k=n;
aa:=makefraction(aa,268435456-bb);theta[n]:=aa;vv[0]:=aa;
for k:=1 to n-1 do vv[k]:=vv[k]+takefraction(aa,ww[k]);goto 40;end{:312}
;end{:308};3:{316:}begin cc:=mem[s+3].int;lt:=abs(mem[s+4].int);
rt:=abs(mem[r+6].int);
if(rt=65536)and(lt=65536)then ff:=makefraction(cc+cc+65536,cc+131072)
else ff:=curlratio(cc,lt,rt);
theta[n]:=-makefraction(takefraction(vv[n-1],ff),268435456-takefraction(
ff,uu[n-1]));goto 40;end{:316};2:{313:}
begin theta[n]:=mem[s+3].int-narg(deltax[n-1],deltay[n-1]);
if abs(theta[n])>188743680 then if theta[n]>0 then theta[n]:=theta[n]
-377487360 else theta[n]:=theta[n]+377487360;goto 40;end{:313};end;r:=s;
s:=t;incr(k);end;40:{318:}
for k:=n-1 downto 0 do theta[k]:=vv[k]-takefraction(theta[k+1],uu[k]);
s:=p;k:=0;repeat t:=mem[s].hh.rh;nsincos(theta[k]);st:=nsin;ct:=ncos;
nsincos(-psi[k+1]-theta[k+1]);sf:=nsin;cf:=ncos;setcontrols(s,t,k);
incr(k);s:=t;until k=n{:318};10:end;{:305}
procedure makechoices(knots:halfword);label 30;var h:halfword;
p,q:halfword;{301:}k,n:0..pathsize;s,t:halfword;delx,dely:scaled;
sine,cosine:fraction;{:301}begin begin if aritherror then cleararith;
end;if internal[4]>0 then printpath(knots,541,true);{291:}p:=knots;
repeat q:=mem[p].hh.rh;
if mem[p+1].int=mem[q+1].int then if mem[p+2].int=mem[q+2].int then if
mem[p].hh.b1>1 then begin mem[p].hh.b1:=1;
if mem[p].hh.b0=4 then begin mem[p].hh.b0:=3;mem[p+3].int:=65536;end;
mem[q].hh.b0:=1;if mem[q].hh.b1=4 then begin mem[q].hh.b1:=3;
mem[q+5].int:=65536;end;mem[p+5].int:=mem[p+1].int;
mem[q+3].int:=mem[p+1].int;mem[p+6].int:=mem[p+2].int;
mem[q+4].int:=mem[p+2].int;end;p:=q;until p=knots{:291};{292:}h:=knots;
while true do begin if mem[h].hh.b0<>4 then goto 30;
if mem[h].hh.b1<>4 then goto 30;h:=mem[h].hh.rh;
if h=knots then begin mem[h].hh.b0:=5;goto 30;end;end;30:{:292};p:=h;
repeat{293:}q:=mem[p].hh.rh;
if mem[p].hh.b1>=2 then begin while(mem[q].hh.b0=4)and(mem[q].hh.b1=4)do
q:=mem[q].hh.rh;{299:}{302:}k:=0;s:=p;n:=pathsize;
repeat t:=mem[s].hh.rh;deltax[k]:=mem[t+1].int-mem[s+1].int;
deltay[k]:=mem[t+2].int-mem[s+2].int;
delta[k]:=pythadd(deltax[k],deltay[k]);
if k>0 then begin sine:=makefraction(deltay[k-1],delta[k-1]);
cosine:=makefraction(deltax[k-1],delta[k-1]);
psi[k]:=narg(takefraction(deltax[k],cosine)+takefraction(deltay[k],sine)
,takefraction(deltay[k],cosine)-takefraction(deltax[k],sine));end;
incr(k);s:=t;if k=pathsize then overflow(546,pathsize);if s=q then n:=k;
until(k>=n)and(mem[s].hh.b0<>5);
if k=n then psi[n]:=0 else psi[k]:=psi[1]{:302};{303:}
if mem[q].hh.b0=4 then begin delx:=mem[q+5].int-mem[q+1].int;
dely:=mem[q+6].int-mem[q+2].int;
if(delx=0)and(dely=0)then begin mem[q].hh.b0:=3;mem[q+3].int:=65536;
end else begin mem[q].hh.b0:=2;mem[q+3].int:=narg(delx,dely);end;end;
if(mem[p].hh.b1=4)and(mem[p].hh.b0=1)then begin delx:=mem[p+1].int-mem[p
+3].int;dely:=mem[p+2].int-mem[p+4].int;
if(delx=0)and(dely=0)then begin mem[p].hh.b1:=3;mem[p+5].int:=65536;
end else begin mem[p].hh.b1:=2;mem[p+5].int:=narg(delx,dely);end;
end{:303};solvechoices(p,q,n){:299};
end else if mem[p].hh.b1=0 then{294:}begin mem[p+5].int:=mem[p+1].int;
mem[p+6].int:=mem[p+2].int;mem[q+3].int:=mem[q+1].int;
mem[q+4].int:=mem[q+2].int;end{:294};p:=q{:293};until p=h;
if internal[4]>0 then printpath(knots,542,true);if aritherror then{290:}
begin begin if interaction=3 then;printnl(262);print(543);end;
begin helpptr:=2;helpline[1]:=544;helpline[0]:=545;end;putgeterror;
aritherror:=false;end{:290};end;{:289}{326:}
function crossingpoint(a,b,c:integer):fraction;label 10;var d:integer;
x,xx,x0,x1,x2:integer;begin if a<0 then begin crossingpoint:=0;goto 10;
end;if c>=0 then begin if b>=0 then if c>0 then begin crossingpoint:=
268435457;goto 10;
end else if(a=0)and(b=0)then begin crossingpoint:=268435457;goto 10;
end else begin crossingpoint:=268435456;goto 10;end;
if a=0 then begin crossingpoint:=0;goto 10;end;
end else if a=0 then if b<=0 then begin crossingpoint:=0;goto 10;end;
{327:}d:=1;x0:=a;x1:=a-b;x2:=b-c;repeat x:=half(x1+x2);
if x1-x0>x0 then begin x2:=x;x0:=x0+x0;d:=d+d;
end else begin xx:=x1+x-x0;if xx>x0 then begin x2:=x;x0:=x0+x0;d:=d+d;
end else begin x0:=x0-xx;
if x<=x0 then if x+x2<=x0 then begin crossingpoint:=268435457;goto 10;
end;x1:=x;d:=d+d+1;end;end;until d>=268435456;
crossingpoint:=d-268435456{:327};10:end;{:326}{328:}
function evalcubic(p,q:halfword;t:fraction):scaled;var x1,x2,x3:scaled;
begin x1:=mem[p].int-takefraction(mem[p].int-mem[p+4].int,t);
x2:=mem[p+4].int-takefraction(mem[p+4].int-mem[q+2].int,t);
x3:=mem[q+2].int-takefraction(mem[q+2].int-mem[q].int,t);
x1:=x1-takefraction(x1-x2,t);x2:=x2-takefraction(x2-x3,t);
evalcubic:=x1-takefraction(x1-x2,t);end;{:328}{330:}
procedure boundcubic(p,q:halfword;c:smallnumber);var wavy:boolean;
del1,del2,del3,del,dmax:scaled;t,tt:fraction;x:scaled;
begin x:=mem[q].int;{331:}if x<bbmin[c]then bbmin[c]:=x;
if x>bbmax[c]then bbmax[c]:=x{:331};{332:}wavy:=true;
if bbmin[c]<=mem[p+4].int then if mem[p+4].int<=bbmax[c]then if bbmin[c]
<=mem[q+2].int then if mem[q+2].int<=bbmax[c]then wavy:=false{:332};
if wavy then begin del1:=mem[p+4].int-mem[p].int;
del2:=mem[q+2].int-mem[p+4].int;del3:=mem[q].int-mem[q+2].int;{333:}
if del1<>0 then del:=del1 else if del2<>0 then del:=del2 else del:=del3;
if del<>0 then begin dmax:=abs(del1);
if abs(del2)>dmax then dmax:=abs(del2);
if abs(del3)>dmax then dmax:=abs(del3);
while dmax<134217728 do begin dmax:=dmax+dmax;del1:=del1+del1;
del2:=del2+del2;del3:=del3+del3;end;end{:333};
if del<0 then begin del1:=-del1;del2:=-del2;del3:=-del3;end;
t:=crossingpoint(del1,del2,del3);if t<268435456 then{334:}
begin x:=evalcubic(p,q,t);{331:}if x<bbmin[c]then bbmin[c]:=x;
if x>bbmax[c]then bbmax[c]:=x{:331};
del2:=del2-takefraction(del2-del3,t);if del2>0 then del2:=0;
tt:=crossingpoint(0,-del2,-del3);if tt<268435456 then{335:}
begin x:=evalcubic(p,q,tt-takefraction(tt-268435456,t));{331:}
if x<bbmin[c]then bbmin[c]:=x;if x>bbmax[c]then bbmax[c]:=x{:331};
end{:335};end{:334};end;end;{:330}{336:}procedure pathbbox(h:halfword);
label 10;var p,q:halfword;begin bbmin[0]:=mem[h+1].int;
bbmin[1]:=mem[h+2].int;bbmax[0]:=bbmin[0];bbmax[1]:=bbmin[1];p:=h;
repeat if mem[p].hh.b1=0 then goto 10;q:=mem[p].hh.rh;
boundcubic(p+1,q+1,0);boundcubic(p+2,q+2,1);p:=q;until p=h;10:end;{:336}
{339:}{349:}function solverisingcubic(a,b,c,x:scaled):scaled;
var ab,bc,ac:scaled;t:integer;xx:integer;
begin if(a<0)or(c<0)then confusion(547);
if x<=0 then solverisingcubic:=0 else if x>=a+b+c then solverisingcubic
:=65536 else begin t:=1;{351:}
while(a>715827882)or(b>715827882)or(c>715827882)do begin a:=halfp(a);
b:=half(b);c:=halfp(c);x:=halfp(x);end{:351};repeat t:=t+t;{350:}
ab:=half(a+b);bc:=half(b+c);ac:=half(ab+bc){:350};xx:=x-a-ab-ac;
if xx<-x then begin x:=x+x;b:=ab;c:=ac;end else begin x:=x+xx;a:=ac;
b:=bc;t:=t+1;end;until t>=65536;solverisingcubic:=t-65536;end;end;{:349}
function arctest(dx0,dy0,dx1,dy1,dx2,dy2,v0,v02,v2,agoal,tol:scaled):
scaled;label 10;var simple:boolean;dx01,dy01,dx12,dy12,dx02,dy02:scaled;
v002,v022:scaled;arc:scaled;{341:}a,b:scaled;anew,aaux:scaled;{:341}
{346:}tmp,tmp2:scaled;arc1:scaled;{:346}begin{344:}dx01:=half(dx0+dx1);
dx12:=half(dx1+dx2);dx02:=half(dx01+dx12);dy01:=half(dy0+dy1);
dy12:=half(dy1+dy2);dy02:=half(dy01+dy12){:344};{345:}
v002:=pythadd(dx01+half(dx0+dx02),dy01+half(dy0+dy02));
v022:=pythadd(dx12+half(dx02+dx2),dy12+half(dy02+dy2));
tmp:=halfp(v02+2);arc1:=v002+half(halfp(v0+tmp)-v002);
arc:=v022+half(halfp(v2+tmp)-v022);
if(arc<2147483647-arc1)then arc:=arc+arc1 else begin aritherror:=true;
if agoal=2147483647 then arctest:=2147483647 else arctest:=-131072;
goto 10;end{:345};{347:}
simple:=(dx0>=0)and(dx1>=0)and(dx2>=0)or(dx0<=0)and(dx1<=0)and(dx2<=0);
if simple then simple:=(dy0>=0)and(dy1>=0)and(dy2>=0)or(dy0<=0)and(dy1<=
0)and(dy2<=0);
if not simple then begin simple:=(dx0>=dy0)and(dx1>=dy1)and(dx2>=dy2)or(
dx0<=dy0)and(dx1<=dy1)and(dx2<=dy2);
if simple then simple:=(-dx0>=dy0)and(-dx1>=dy1)and(-dx2>=dy2)or(-dx0<=
dy0)and(-dx1<=dy1)and(-dx2<=dy2);end{:347};
if simple and(abs(arc-v02-halfp(v0+v2))<=tol)then if arc<agoal then
arctest:=arc else{348:}begin tmp:=(v02+2)div 4;
if agoal<=arc1 then begin tmp2:=halfp(v0);
arctest:=halfp(solverisingcubic(tmp2,arc1-tmp2-tmp,tmp,agoal))-131072;
end else begin tmp2:=halfp(v2);
arctest:=(-98304)+halfp(solverisingcubic(tmp,arc-arc1-tmp-tmp2,tmp2,
agoal-arc1));end;end{:348}else{340:}begin{342:}aaux:=2147483647-agoal;
if agoal>aaux then begin aaux:=agoal-aaux;anew:=2147483647;
end else begin anew:=agoal+agoal;aaux:=0;end{:342};tol:=tol+halfp(tol);
a:=arctest(dx0,dy0,dx01,dy01,dx02,dy02,v0,v002,halfp(v02),anew,tol);
if a<0 then arctest:=-halfp(131072-a)else begin{343:}
if a>aaux then begin aaux:=aaux-a;anew:=anew+aaux;end{:343};
b:=arctest(dx02,dy02,dx12,dy12,dx2,dy2,halfp(v02),v022,v2,anew,tol);
if b<0 then arctest:=-halfp(-b)-32768 else arctest:=a+half(b-a);end;
end{:340};10:end;{:339}{352:}
function doarctest(dx0,dy0,dx1,dy1,dx2,dy2,agoal:scaled):scaled;
var v0,v1,v2:scaled;v02:scaled;begin v0:=pythadd(dx0,dy0);
v1:=pythadd(dx1,dy1);v2:=pythadd(dx2,dy2);
if(v0>=1073741824)or(v1>=1073741824)or(v2>=1073741824)then begin
aritherror:=true;
if agoal=2147483647 then doarctest:=2147483647 else doarctest:=-131072;
end else begin v02:=pythadd(dx1+half(dx0+dx2),dy1+half(dy0+dy2));
doarctest:=arctest(dx0,dy0,dx1,dy1,dx2,dy2,v0,v02,v2,agoal,16);end;end;
{:352}{353:}function getarclength(h:halfword):scaled;label 30;
var p,q:halfword;a,atot:scaled;begin atot:=0;p:=h;
while mem[p].hh.b1<>0 do begin q:=mem[p].hh.rh;
a:=doarctest(mem[p+5].int-mem[p+1].int,mem[p+6].int-mem[p+2].int,mem[q+3
].int-mem[p+5].int,mem[q+4].int-mem[p+6].int,mem[q+1].int-mem[q+3].int,
mem[q+2].int-mem[q+4].int,2147483647);atot:=slowadd(a,atot);
if q=h then goto 30 else p:=q;end;
30:begin if aritherror then cleararith;end;getarclength:=atot;end;{:353}
{354:}function getarctime(h:halfword;arc0:scaled):scaled;label 30;
var p,q:halfword;ttot:scaled;t:scaled;arc:scaled;n:integer;
begin if arc0<0 then{356:}
begin if mem[h].hh.b0=0 then ttot:=0 else begin p:=htapypoc(h);
ttot:=-getarctime(p,-arc0);tossknotlist(p);end;goto 30;end{:356};
if arc0=2147483647 then decr(arc0);ttot:=0;arc:=arc0;p:=h;
while(mem[p].hh.b1<>0)and(arc>0)do begin q:=mem[p].hh.rh;
t:=doarctest(mem[p+5].int-mem[p+1].int,mem[p+6].int-mem[p+2].int,mem[q+3
].int-mem[p+5].int,mem[q+4].int-mem[p+6].int,mem[q+1].int-mem[q+3].int,
mem[q+2].int-mem[q+4].int,arc);{355:}
if t<0 then begin ttot:=ttot+t+131072;arc:=0;
end else begin ttot:=ttot+65536;arc:=arc-t;end{:355};if q=h then{357:}
if arc>0 then begin n:=arc div(arc0-arc);arc:=arc-n*(arc0-arc);
if ttot>2147483647 div(n+1)then begin aritherror:=true;ttot:=2147483647;
goto 30;end;ttot:=(n+1)*ttot;end{:357};p:=q;end;
30:begin if aritherror then cleararith;end;getarctime:=ttot;end;{:354}
{359:}{375:}{380:}procedure moveknot(p,q:halfword);
begin mem[mem[p].hh.lh].hh.rh:=mem[p].hh.rh;
mem[mem[p].hh.rh].hh.lh:=mem[p].hh.lh;mem[p].hh.lh:=q;
mem[p].hh.rh:=mem[q].hh.rh;mem[q].hh.rh:=p;mem[mem[p].hh.rh].hh.lh:=p;
end;{:380}function convexhull(h:halfword):halfword;label 31,32,33;
var l,r:halfword;p,q:halfword;s:halfword;dx,dy:scaled;
begin if(h=mem[h].hh.rh)then convexhull:=h else begin{376:}l:=h;
p:=mem[h].hh.rh;
while p<>h do begin if mem[p+1].int<=mem[l+1].int then if(mem[p+1].int<
mem[l+1].int)or(mem[p+2].int<mem[l+2].int)then l:=p;p:=mem[p].hh.rh;
end{:376};{377:}r:=h;p:=mem[h].hh.rh;
while p<>h do begin if mem[p+1].int>=mem[r+1].int then if(mem[p+1].int>
mem[r+1].int)or(mem[p+2].int>mem[r+2].int)then r:=p;p:=mem[p].hh.rh;
end{:377};if l<>r then begin s:=mem[r].hh.rh;{378:}
dx:=mem[r+1].int-mem[l+1].int;dy:=mem[r+2].int-mem[l+2].int;
p:=mem[l].hh.rh;while p<>r do begin q:=mem[p].hh.rh;
if abvscd(dx,mem[p+2].int-mem[l+2].int,dy,mem[p+1].int-mem[l+1].int)>0
then moveknot(p,r);p:=q;end{:378};{381:}p:=s;
while p<>l do begin q:=mem[p].hh.rh;
if abvscd(dx,mem[p+2].int-mem[l+2].int,dy,mem[p+1].int-mem[l+1].int)<0
then moveknot(p,l);p:=q;end{:381};{382:}p:=mem[l].hh.rh;
while p<>r do begin q:=mem[p].hh.lh;
while mem[q+1].int>mem[p+1].int do q:=mem[q].hh.lh;
while mem[q+1].int=mem[p+1].int do if mem[q+2].int>mem[p+2].int then q:=
mem[q].hh.lh else goto 31;
31:if q=mem[p].hh.lh then p:=mem[p].hh.rh else begin p:=mem[p].hh.rh;
moveknot(mem[p].hh.lh,q);end;end{:382};{383:}p:=mem[r].hh.rh;
while p<>l do begin q:=mem[p].hh.lh;
while mem[q+1].int<mem[p+1].int do q:=mem[q].hh.lh;
while mem[q+1].int=mem[p+1].int do if mem[q+2].int<mem[p+2].int then q:=
mem[q].hh.lh else goto 32;
32:if q=mem[p].hh.lh then p:=mem[p].hh.rh else begin p:=mem[p].hh.rh;
moveknot(mem[p].hh.lh,q);end;end{:383};end;if l<>mem[l].hh.rh then{384:}
begin p:=l;q:=mem[l].hh.rh;
while true do begin dx:=mem[q+1].int-mem[p+1].int;
dy:=mem[q+2].int-mem[p+2].int;p:=q;q:=mem[q].hh.rh;if p=l then goto 33;
if p<>r then if abvscd(dx,mem[q+2].int-mem[p+2].int,dy,mem[q+1].int-mem[
p+1].int)<=0 then{385:}begin s:=mem[p].hh.lh;freenode(p,7);
mem[s].hh.rh:=q;mem[q].hh.lh:=s;
if s=l then p:=s else begin p:=mem[s].hh.lh;q:=s;end;end{:385};end;33:;
end{:384};convexhull:=l;end;end;{:375}function makepen(h:halfword;
needhull:boolean):halfword;var p,q:halfword;begin q:=h;repeat p:=q;
q:=mem[q].hh.rh;mem[q].hh.lh:=p;until q=h;
if needhull then begin h:=convexhull(h);{361:}
if(h=mem[h].hh.rh)then begin mem[h+3].int:=mem[h+1].int;
mem[h+4].int:=mem[h+2].int;mem[h+5].int:=mem[h+1].int;
mem[h+6].int:=mem[h+2].int;end{:361};end;makepen:=h;end;{:359}{360:}
function getpencircle(diam:scaled):halfword;var h:halfword;
begin h:=getnode(7);mem[h].hh.rh:=h;mem[h].hh.lh:=h;mem[h+1].int:=0;
mem[h+2].int:=0;mem[h+3].int:=diam;mem[h+4].int:=0;mem[h+5].int:=0;
mem[h+6].int:=diam;getpencircle:=h;end;{:360}{367:}
procedure makepath(h:halfword);var p:halfword;k:smallnumber;{371:}
centerx,centery:scaled;widthx,widthy:scaled;heightx,heighty:scaled;
dx,dy:scaled;kk:integer;{:371}begin if(h=mem[h].hh.rh)then{369:}
begin{370:}centerx:=mem[h+1].int;centery:=mem[h+2].int;
widthx:=mem[h+3].int-centerx;widthy:=mem[h+4].int-centery;
heightx:=mem[h+5].int-centerx;heighty:=mem[h+6].int-centery{:370};p:=h;
for k:=0 to 7 do begin{372:}kk:=(k+6)mod 8;
mem[p+1].int:=centerx+takefraction(halfcos[k],widthx)+takefraction(
halfcos[kk],heightx);
mem[p+2].int:=centery+takefraction(halfcos[k],widthy)+takefraction(
halfcos[kk],heighty);
dx:=-takefraction(dcos[kk],widthx)+takefraction(dcos[k],heightx);
dy:=-takefraction(dcos[kk],widthy)+takefraction(dcos[k],heighty);
mem[p+5].int:=mem[p+1].int+dx;mem[p+6].int:=mem[p+2].int+dy;
mem[p+3].int:=mem[p+1].int-dx;mem[p+4].int:=mem[p+2].int-dy;
mem[p].hh.b0:=1;mem[p].hh.b1:=1{:372};
if k=7 then mem[p].hh.rh:=h else mem[p].hh.rh:=getnode(7);
p:=mem[p].hh.rh;end;end{:369}else begin p:=h;repeat mem[p].hh.b0:=1;
mem[p].hh.b1:=1;{368:}mem[p+3].int:=mem[p+1].int;
mem[p+4].int:=mem[p+2].int;mem[p+5].int:=mem[p+1].int;
mem[p+6].int:=mem[p+2].int{:368};p:=mem[p].hh.rh;until p=h;end;end;
{:367}{386:}procedure findoffset(x,y:scaled;h:halfword);
var p,q:halfword;wx,wy,hx,hy:scaled;xx,yy:fraction;d:fraction;
begin if(h=mem[h].hh.rh)then{388:}
if(x=0)and(y=0)then begin curx:=mem[h+1].int;cury:=mem[h+2].int;
end else begin{389:}wx:=mem[h+3].int-mem[h+1].int;
wy:=mem[h+4].int-mem[h+2].int;hx:=mem[h+5].int-mem[h+1].int;
hy:=mem[h+6].int-mem[h+2].int{:389};
while(abs(x)<134217728)and(abs(y)<134217728)do begin x:=x+x;y:=y+y;end;
{390:}yy:=-(takefraction(x,hy)+takefraction(y,-hx));
xx:=takefraction(x,-wy)+takefraction(y,wx);d:=pythadd(xx,yy);
if d>0 then begin xx:=half(makefraction(xx,d));
yy:=half(makefraction(yy,d));end{:390};
curx:=mem[h+1].int+takefraction(xx,wx)+takefraction(yy,hx);
cury:=mem[h+2].int+takefraction(xx,wy)+takefraction(yy,hy);end{:388}
else begin q:=h;repeat p:=q;q:=mem[q].hh.rh;
until abvscd(mem[q+1].int-mem[p+1].int,y,mem[q+2].int-mem[p+2].int,x)>=0
;repeat p:=q;q:=mem[q].hh.rh;
until abvscd(mem[q+1].int-mem[p+1].int,y,mem[q+2].int-mem[p+2].int,x)<=0
;curx:=mem[p+1].int;cury:=mem[p+2].int;end;end;{:386}{391:}
procedure penbbox(h:halfword);var p:halfword;
begin if(h=mem[h].hh.rh)then{392:}begin findoffset(0,268435456,h);
bbmax[0]:=curx;bbmin[0]:=2*mem[h+1].int-curx;findoffset(-268435456,0,h);
bbmax[1]:=cury;bbmin[1]:=2*mem[h+2].int-cury;end{:392}
else begin bbmin[0]:=mem[h+1].int;bbmax[0]:=bbmin[0];
bbmin[1]:=mem[h+2].int;bbmax[1]:=bbmin[1];p:=mem[h].hh.rh;
while p<>h do begin if mem[p+1].int<bbmin[0]then bbmin[0]:=mem[p+1].int;
if mem[p+2].int<bbmin[1]then bbmin[1]:=mem[p+2].int;
if mem[p+1].int>bbmax[0]then bbmax[0]:=mem[p+1].int;
if mem[p+2].int>bbmax[1]then bbmax[1]:=mem[p+2].int;p:=mem[p].hh.rh;end;
end;end;{:391}{394:}function newfillnode(p:halfword):halfword;
var t:halfword;begin t:=getnode(6);mem[t].hh.b0:=1;mem[t+1].hh.rh:=p;
mem[t+1].hh.lh:=0;mem[t+2].int:=0;mem[t+3].int:=0;mem[t+4].int:=0;{395:}
if internal[27]>65536 then mem[t].hh.b1:=2 else if internal[27]>0 then
mem[t].hh.b1:=1 else mem[t].hh.b1:=0;
if internal[29]<65536 then mem[t+5].int:=65536 else mem[t+5].int:=
internal[29]{:395};newfillnode:=t;end;{:394}{396:}
function newstrokednode(p:halfword):halfword;var t:halfword;
begin t:=getnode(8);mem[t].hh.b0:=2;mem[t+1].hh.rh:=p;mem[t+1].hh.lh:=0;
mem[t+6].hh.rh:=0;mem[t+7].int:=65536;mem[t+2].int:=0;mem[t+3].int:=0;
mem[t+4].int:=0;{395:}
if internal[27]>65536 then mem[t].hh.b1:=2 else if internal[27]>0 then
mem[t].hh.b1:=1 else mem[t].hh.b1:=0;
if internal[29]<65536 then mem[t+5].int:=65536 else mem[t+5].int:=
internal[29]{:395};
if internal[28]>65536 then mem[t+6].hh.b0:=2 else if internal[28]>0 then
mem[t+6].hh.b0:=1 else mem[t+6].hh.b0:=0;newstrokednode:=t;end;{:396}
{399:}{1179:}{747:}procedure beginname;
begin begin if strref[curname]<127 then if strref[curname]>1 then decr(
strref[curname])else flushstring(curname);end;
begin if strref[curarea]<127 then if strref[curarea]>1 then decr(strref[
curarea])else flushstring(curarea);end;
begin if strref[curext]<127 then if strref[curext]>1 then decr(strref[
curext])else flushstring(curext);end;areadelimiter:=-1;extdelimiter:=-1;
end;{:747}{748:}function morename(c:ASCIIcode):boolean;
begin if(c=32)or(c=9)then morename:=false else begin if ISDIRSEP(c)then
begin areadelimiter:=poolptr-strstart[strptr];extdelimiter:=-1;
end else if(c=46)then extdelimiter:=poolptr-strstart[strptr];
begin if poolptr+1>maxpoolptr then if poolptr+1>poolsize then
docompaction(1)else maxpoolptr:=poolptr+1;end;begin strpool[poolptr]:=c;
incr(poolptr);end;morename:=true;end;end;{:748}{749:}procedure endname;
var a,n,e:poolpointer;begin e:=poolptr-strstart[strptr];
if extdelimiter<0 then extdelimiter:=e;a:=areadelimiter+1;
n:=extdelimiter-a;e:=e-extdelimiter;
if a=0 then curarea:=284 else begin curarea:=makestring;
choplaststring(strstart[curarea]+a);end;
if n=0 then curname:=284 else begin curname:=makestring;
choplaststring(strstart[curname]+n);end;
if e=0 then curext:=284 else curext:=makestring;end;{:749}{751:}
procedure packfilename(n,a,e:strnumber);var k:integer;c:ASCIIcode;
j:poolpointer;begin k:=0;if nameoffile then libcfree(nameoffile);
nameoffile:=xmalloc(1+(strstart[nextstr[a]]-strstart[a])+(strstart[
nextstr[n]]-strstart[n])+(strstart[nextstr[e]]-strstart[e])+1);
for j:=strstart[a]to strstart[nextstr[a]]-1 do begin c:=strpool[j];
incr(k);if k<=maxint then nameoffile[k]:=xchr[c];end;
for j:=strstart[n]to strstart[nextstr[n]]-1 do begin c:=strpool[j];
incr(k);if k<=maxint then nameoffile[k]:=xchr[c];end;
for j:=strstart[e]to strstart[nextstr[e]]-1 do begin c:=strpool[j];
incr(k);if k<=maxint then nameoffile[k]:=xchr[c];end;
if k<=maxint then namelength:=k else namelength:=maxint;
nameoffile[namelength+1]:=0;end;{:751}{759:}
procedure strscanfile(s:strnumber);label 30;var p,q:poolpointer;
begin beginname;p:=strstart[s];q:=strstart[nextstr[s]];
while p<q do begin if not morename(strpool[p])then goto 30;incr(p);end;
30:endname;end;{:759}function readfontinfo(fname:strnumber):fontnumber;
label 11,30;var fileopened:boolean;n:fontnumber;
lf,lh,bc,ec,nw,nh,nd:halfword;whdsize:integer;i,ii:0..fontmemsize;
jj:0..fontmemsize;z:scaled;d:fraction;handd:eightbits;begin n:=0;{1188:}
fileopened:=false;strscanfile(fname);if curext=284 then curext:=1092;
packfilename(curname,curarea,curext);
if not bopenin(tfminfile)then goto 11;fileopened:=true{:1188};{1181:}
{1182:}begin lf:=tfmtemp;if lf>127 then goto 11;
tfmtemp:=getc(tfminfile);lf:=lf*256+tfmtemp;end;
tfmtemp:=getc(tfminfile);begin lh:=tfmtemp;if lh>127 then goto 11;
tfmtemp:=getc(tfminfile);lh:=lh*256+tfmtemp;end;
tfmtemp:=getc(tfminfile);begin bc:=tfmtemp;if bc>127 then goto 11;
tfmtemp:=getc(tfminfile);bc:=bc*256+tfmtemp;end;
tfmtemp:=getc(tfminfile);begin ec:=tfmtemp;if ec>127 then goto 11;
tfmtemp:=getc(tfminfile);ec:=ec*256+tfmtemp;end;
if(bc>1+ec)or(ec>255)then goto 11;tfmtemp:=getc(tfminfile);
begin nw:=tfmtemp;if nw>127 then goto 11;tfmtemp:=getc(tfminfile);
nw:=nw*256+tfmtemp;end;tfmtemp:=getc(tfminfile);begin nh:=tfmtemp;
if nh>127 then goto 11;tfmtemp:=getc(tfminfile);nh:=nh*256+tfmtemp;end;
tfmtemp:=getc(tfminfile);begin nd:=tfmtemp;if nd>127 then goto 11;
tfmtemp:=getc(tfminfile);nd:=nd*256+tfmtemp;end;
whdsize:=(ec+1-bc)+nw+nh+nd;if lf<6+lh+whdsize then goto 11;
for jj:=10 downto 1 do tfmtemp:=getc(tfminfile){:1182};{1183:}
if nextfmem<bc+0 then nextfmem:=bc+0;
if(lastfnum=fontmax)or(nextfmem+whdsize>=fontmemsize)then{1184:}
begin begin if interaction=3 then;printnl(262);print(1100);end;
print(fname);print(1107);begin helpptr:=3;helpline[2]:=1108;
helpline[1]:=1109;helpline[0]:=1110;end;error;goto 30;end{:1184};
incr(lastfnum);n:=lastfnum;fontbc[n]:=bc;fontec[n]:=ec;
charbase[n]:=nextfmem-bc-0;widthbase[n]:=nextfmem+ec-bc+1;
heightbase[n]:=widthbase[n]+0+nw;depthbase[n]:=heightbase[n]+nh;
nextfmem:=nextfmem+whdsize;{:1183};{1185:}if lh<2 then goto 11;
for jj:=4 downto 1 do tfmtemp:=getc(tfminfile);tfmtemp:=getc(tfminfile);
begin z:=tfmtemp;if z>127 then goto 11;tfmtemp:=getc(tfminfile);
z:=z*256+tfmtemp;end;tfmtemp:=getc(tfminfile);z:=z*256+tfmtemp;
tfmtemp:=getc(tfminfile);z:=z*256+tfmtemp;
fontdsize[n]:=takefraction(z,267432584);
for jj:=4*(lh-2)downto 1 do tfmtemp:=getc(tfminfile){:1185};{1186:}
ii:=widthbase[n]+0;i:=charbase[n]+0+bc;
while i<ii do begin tfmtemp:=getc(tfminfile);
fontinfo[i].qqqq.b0:=tfmtemp;tfmtemp:=getc(tfminfile);handd:=tfmtemp;
fontinfo[i].qqqq.b1:=handd div 16;fontinfo[i].qqqq.b2:=handd mod 16;
tfmtemp:=getc(tfminfile);tfmtemp:=getc(tfminfile);incr(i);end;
while i<nextfmem do{1187:}begin tfmtemp:=getc(tfminfile);d:=tfmtemp;
if d>=128 then d:=d-256;tfmtemp:=getc(tfminfile);d:=d*256+tfmtemp;
tfmtemp:=getc(tfminfile);d:=d*256+tfmtemp;tfmtemp:=getc(tfminfile);
d:=d*256+tfmtemp;fontinfo[i].int:=takefraction(d*16,fontdsize[n]);
incr(i);end{:1187};if feof(tfminfile)then goto 11;goto 30{:1186}{:1181};
11:{1180:}begin if interaction=3 then;printnl(262);print(1100);end;
print(fname);if fileopened then print(1101)else print(1102);
begin helpptr:=3;helpline[2]:=1103;helpline[1]:=1104;helpline[0]:=1105;
end;if fileopened then helpline[0]:=1106;error{:1180};
30:if fileopened then bclose(tfminfile);
if n<>0 then begin fontpsname[n]:=fname;fontname[n]:=fname;
strref[fname]:=127;end;readfontinfo:=n;end;{:1179}{1189:}
function findfont(f:strnumber):fontnumber;label 10,40;var n:fontnumber;
begin for n:=0 to lastfnum do if strvsstr(f,fontname[n])=0 then goto 40;
findfont:=readfontinfo(f);goto 10;40:findfont:=n;10:end;{:1189}{1191:}
procedure lostwarning(f:fontnumber;k:poolpointer);
begin if internal[11]>0 then begin begindiagnostic;printnl(1111);
print(strpool[k]);print(1112);print(fontname[f]);printchar(33);
enddiagnostic(false);end;end;{:1191}{1192:}
procedure settextbox(p:halfword);var f:fontnumber;bc,ec:poolASCIIcode;
k,kk:poolpointer;cc:fourquarters;h,d:scaled;begin mem[p+5].int:=0;
mem[p+6].int:=-2147483647;mem[p+7].int:=-2147483647;f:=mem[p+1].hh.lh;
bc:=fontbc[f];ec:=fontec[f];kk:=strstart[nextstr[mem[p+1].hh.rh]];
k:=strstart[mem[p+1].hh.rh];while k<kk do{1193:}
begin if(strpool[k]<bc)or(strpool[k]>ec)then lostwarning(f,k)else begin
cc:=fontinfo[charbase[f]+strpool[k]].qqqq;
if not(cc.b0>0)then lostwarning(f,k)else begin mem[p+5].int:=mem[p+5].
int+fontinfo[widthbase[f]+cc.b0].int;
h:=fontinfo[heightbase[f]+cc.b1].int;
d:=fontinfo[depthbase[f]+cc.b2].int;
if h>mem[p+6].int then mem[p+6].int:=h;
if d>mem[p+7].int then mem[p+7].int:=d;end;end;incr(k);end{:1193};
{1194:}if mem[p+6].int<-mem[p+7].int then begin mem[p+6].int:=0;
mem[p+7].int:=0;end{:1194};end;{:1192}
function newtextnode(f,s:strnumber):halfword;var t:halfword;
begin t:=getnode(14);mem[t].hh.b0:=3;mem[t+1].hh.rh:=s;
mem[t+1].hh.lh:=findfont(f);mem[t+2].int:=0;mem[t+3].int:=0;
mem[t+4].int:=0;mem[t+8].int:=0;mem[t+9].int:=0;mem[t+10].int:=65536;
mem[t+11].int:=0;mem[t+12].int:=0;mem[t+13].int:=65536;settextbox(t);
newtextnode:=t;end;{:399}{400:}function newboundsnode(p:halfword;
c:smallnumber):halfword;var t:halfword;
begin t:=getnode(grobjectsize[c]);mem[t].hh.b0:=c;mem[t+1].hh.rh:=p;
newboundsnode:=t;end;{:400}{404:}procedure initbbox(h:halfword);
begin mem[h+6].hh.rh:=h+7;mem[h+6].hh.lh:=0;mem[h+2].int:=2147483647;
mem[h+3].int:=2147483647;mem[h+4].int:=-2147483647;
mem[h+5].int:=-2147483647;end;{:404}{405:}
procedure initedges(h:halfword);begin mem[h].hh.rh:=2;
mem[h+7].hh.lh:=h+7;mem[h+7].hh.rh:=0;mem[h].hh.lh:=0;initbbox(h);end;
{:405}{410:}{413:}function copyobjects(p,q:halfword):halfword;
var hh:halfword;pp:halfword;k:smallnumber;begin hh:=getnode(8);
mem[hh].hh.rh:=2;mem[hh].hh.lh:=0;pp:=hh+7;while(p<>q)do{414:}
begin k:=grobjectsize[mem[p].hh.b0];mem[pp].hh.rh:=getnode(k);
pp:=mem[pp].hh.rh;while(k>0)do begin decr(k);mem[pp+k]:=mem[p+k];end;
{415:}
case mem[p].hh.b0 of 4,5:mem[pp+1].hh.rh:=copypath(mem[p+1].hh.rh);
1:begin mem[pp+1].hh.rh:=copypath(mem[p+1].hh.rh);
if mem[p+1].hh.lh<>0 then mem[pp+1].hh.lh:=makepen(copypath(mem[p+1].hh.
lh),false);end;2:begin mem[pp+1].hh.rh:=copypath(mem[p+1].hh.rh);
mem[pp+1].hh.lh:=makepen(copypath(mem[p+1].hh.lh),false);
if mem[p+6].hh.rh<>0 then incr(mem[mem[pp+6].hh.rh].hh.lh);end;
3:begin if strref[mem[pp+1].hh.rh]<127 then incr(strref[mem[pp+1].hh.rh]
);end;6,7:;end{:415};p:=mem[p].hh.rh;end{:414};mem[hh+7].hh.lh:=pp;
mem[pp].hh.rh:=0;copyobjects:=hh;end;{:413}
function privateedges(h:halfword):halfword;var hh:halfword;
p,pp:halfword;
begin if mem[h].hh.lh=0 then privateedges:=h else begin decr(mem[h].hh.
lh);hh:=copyobjects(mem[h+7].hh.rh,0);{411:}pp:=hh;p:=mem[h].hh.rh;
while(p<>2)do begin mem[pp].hh.rh:=getnode(3);pp:=mem[pp].hh.rh;
mem[pp+1].int:=mem[p+1].int;mem[pp+2].int:=mem[p+2].int;p:=mem[p].hh.rh;
end;mem[pp].hh.rh:=2;mem[hh+1].int:=mem[h+1].int{:411};{412:}
mem[hh+2].int:=mem[h+2].int;mem[hh+3].int:=mem[h+3].int;
mem[hh+4].int:=mem[h+4].int;mem[hh+5].int:=mem[h+5].int;
mem[hh+6].hh.lh:=mem[h+6].hh.lh;p:=h+7;pp:=hh+7;
while(p<>mem[h+6].hh.rh)do begin if p=0 then confusion(551);
p:=mem[p].hh.rh;pp:=mem[pp].hh.rh;end;mem[hh+6].hh.rh:=pp{:412};
privateedges:=hh;end;end;{:410}{416:}
function skip1component(p:halfword):halfword;var lev:integer;
begin lev:=0;
repeat if(mem[p].hh.b0>=4)then if(mem[p].hh.b0>=6)then decr(lev)else
incr(lev);p:=mem[p].hh.rh;until lev=0;skip1component:=p;end;{:416}{429:}
{431:}procedure xretraceerror;begin begin if interaction=3 then;
printnl(262);print(580);end;begin helpptr:=3;helpline[2]:=584;
helpline[1]:=585;helpline[0]:=583;end;putgeterror;end;{:431}
function makedashes(h:halfword):halfword;label 10,40,45;var p:halfword;
y0:scaled;p0:halfword;pp,qq,rr:halfword;d,dd:halfword;{434:}
x0,x1,x2,x3:scaled;{:434}{440:}dln:halfword;hh:halfword;hsf:scaled;
ds:halfword;xoff:scaled;{:440}begin if mem[h].hh.rh<>2 then goto 40;
p0:=0;p:=mem[h+7].hh.rh;
while p<>0 do begin if mem[p].hh.b0<>2 then{430:}
begin begin if interaction=3 then;printnl(262);print(580);end;
begin helpptr:=3;helpline[2]:=581;helpline[1]:=582;helpline[0]:=583;end;
putgeterror;goto 45;end{:430};pp:=mem[p+1].hh.rh;
if p0=0 then begin p0:=p;y0:=mem[pp+2].int;end;{432:}{435:}
if(mem[p+2].int<>mem[p0+2].int)or(mem[p+3].int<>mem[p0+3].int)or(mem[p+4
].int<>mem[p0+4].int)then begin begin if interaction=3 then;
printnl(262);print(580);end;begin helpptr:=3;helpline[2]:=586;
helpline[1]:=587;helpline[0]:=583;end;putgeterror;goto 45;end{:435};
rr:=pp;if mem[pp].hh.rh<>pp then repeat qq:=rr;rr:=mem[rr].hh.rh;{433:}
x0:=mem[qq+1].int;x1:=mem[qq+5].int;x2:=mem[rr+3].int;x3:=mem[rr+1].int;
if(x0>x1)or(x1>x2)or(x2>x3)then if(x0<x1)or(x1<x2)or(x2<x3)then if
abvscd(x2-x1,x2-x1,x1-x0,x3-x2)>0 then begin xretraceerror;goto 45;end;
if(mem[pp+1].int>x0)or(x0>x3)then if(mem[pp+1].int<x0)or(x0<x3)then
begin xretraceerror;goto 45;end{:433};until mem[rr].hh.b1=0;
d:=getnode(3);
if mem[p+6].hh.rh=0 then mem[d].hh.lh:=0 else mem[d].hh.lh:=p;
if mem[pp+1].int<mem[rr+1].int then begin mem[d+1].int:=mem[pp+1].int;
mem[d+2].int:=mem[rr+1].int;end else begin mem[d+1].int:=mem[rr+1].int;
mem[d+2].int:=mem[pp+1].int;end;{:432};{436:}mem[3].int:=mem[d+2].int;
dd:=h;while mem[mem[dd].hh.rh+1].int<mem[d+2].int do dd:=mem[dd].hh.rh;
if dd<>h then if(mem[dd+2].int>mem[d+1].int)then begin xretraceerror;
goto 45;end;mem[d].hh.rh:=mem[dd].hh.rh;mem[dd].hh.rh:=d{:436};
p:=mem[p].hh.rh;end;if mem[h].hh.rh=2 then goto 45;{439:}d:=h;
while mem[d].hh.rh<>2 do begin ds:=mem[mem[d].hh.rh].hh.lh;
if ds=0 then d:=mem[d].hh.rh else begin hh:=mem[ds+6].hh.rh;
hsf:=mem[ds+7].int;if(hh=0)then confusion(588);
if mem[hh+1].int=0 then d:=mem[d].hh.rh else begin if mem[hh].hh.rh=0
then confusion(588);{441:}dln:=mem[d].hh.rh;dd:=mem[hh].hh.rh;
xoff:=mem[dln+1].int-takescaled(hsf,mem[dd+1].int)-takescaled(hsf,
dashoffset(hh));
mem[3].int:=takescaled(hsf,mem[dd+1].int)+takescaled(hsf,mem[hh+1].int);
mem[4].int:=mem[3].int;{442:}
while xoff+takescaled(hsf,mem[dd+2].int)<mem[dln+1].int do dd:=mem[dd].
hh.rh{:442};while mem[dln+1].int<=mem[dln+2].int do begin{443:}
if dd=2 then begin dd:=mem[hh].hh.rh;
xoff:=xoff+takescaled(hsf,mem[hh+1].int);end{:443};{444:}
if xoff+takescaled(hsf,mem[dd+1].int)<=mem[dln+2].int then begin mem[d].
hh.rh:=getnode(3);d:=mem[d].hh.rh;mem[d].hh.rh:=dln;
if mem[dln+1].int>xoff+takescaled(hsf,mem[dd+1].int)then mem[d+1].int:=
mem[dln+1].int else mem[d+1].int:=xoff+takescaled(hsf,mem[dd+1].int);
if mem[dln+2].int<xoff+takescaled(hsf,mem[dd+2].int)then mem[d+2].int:=
mem[dln+2].int else mem[d+2].int:=xoff+takescaled(hsf,mem[dd+2].int);
end{:444};dd:=mem[dd].hh.rh;
mem[dln+1].int:=xoff+takescaled(hsf,mem[dd+1].int);end;
mem[d].hh.rh:=mem[dln].hh.rh;freenode(dln,3){:441};end;end;end{:439};
{437:}d:=mem[h].hh.rh;while(mem[d].hh.rh<>2)do d:=mem[d].hh.rh;
dd:=mem[h].hh.rh;mem[h+1].int:=mem[d+2].int-mem[dd+1].int;
if abs(y0)>mem[h+1].int then mem[h+1].int:=abs(y0)else if d<>dd then
begin mem[h].hh.rh:=mem[dd].hh.rh;
mem[d+2].int:=mem[dd+2].int+mem[h+1].int;freenode(dd,3);end{:437};
40:makedashes:=h;goto 10;45:{438:}flushdashlist(h);
if mem[h].hh.lh=0 then tossedges(h)else decr(mem[h].hh.lh);
makedashes:=0{:438};10:end;{:429}{445:}procedure adjustbbox(h:halfword);
begin if bbmin[0]<mem[h+2].int then mem[h+2].int:=bbmin[0];
if bbmin[1]<mem[h+3].int then mem[h+3].int:=bbmin[1];
if bbmax[0]>mem[h+4].int then mem[h+4].int:=bbmax[0];
if bbmax[1]>mem[h+5].int then mem[h+5].int:=bbmax[1];end;{:445}{446:}
procedure boxends(p,pp,h:halfword);label 10;var q:halfword;
dx,dy:fraction;d:scaled;z:scaled;xx,yy:scaled;i:integer;
begin if mem[p].hh.b1<>0 then begin q:=mem[p].hh.rh;
while true do begin{447:}
if q=mem[p].hh.rh then begin dx:=mem[p+1].int-mem[p+5].int;
dy:=mem[p+2].int-mem[p+6].int;
if(dx=0)and(dy=0)then begin dx:=mem[p+1].int-mem[q+3].int;
dy:=mem[p+2].int-mem[q+4].int;end;
end else begin dx:=mem[p+1].int-mem[p+3].int;
dy:=mem[p+2].int-mem[p+4].int;
if(dx=0)and(dy=0)then begin dx:=mem[p+1].int-mem[q+5].int;
dy:=mem[p+2].int-mem[q+6].int;end;end;dx:=mem[p+1].int-mem[q+1].int;
dy:=mem[p+2].int-mem[q+2].int{:447};d:=pythadd(dx,dy);
if d>0 then begin{448:}dx:=makefraction(dx,d);dy:=makefraction(dy,d);
findoffset(-dy,dx,pp);xx:=curx;yy:=cury{:448};
for i:=1 to 2 do begin{449:}findoffset(dx,dy,pp);
d:=takefraction(xx-curx,dx)+takefraction(yy-cury,dy);
if(d<0)and(i=1)or(d>0)and(i=2)then confusion(589);
z:=mem[p+1].int+curx+takefraction(d,dx);
if z<mem[h+2].int then mem[h+2].int:=z;
if z>mem[h+4].int then mem[h+4].int:=z;
z:=mem[p+2].int+cury+takefraction(d,dy);
if z<mem[h+3].int then mem[h+3].int:=z;
if z>mem[h+5].int then mem[h+5].int:=z{:449};dx:=-dx;dy:=-dy;end;end;
if mem[p].hh.b1=0 then goto 10 else{450:}repeat q:=p;p:=mem[p].hh.rh;
until mem[p].hh.b1=0{:450};end;end;10:;end;{:446}{451:}
procedure setbbox(h:halfword;toplevel:boolean);label 10;var p:halfword;
sminx,sminy,smaxx,smaxy:scaled;x0,x1,y0,y1:scaled;lev:integer;
begin{452:}case mem[h+6].hh.lh of 0:;
1:if internal[33]>0 then initbbox(h);
2:if internal[33]<=0 then initbbox(h);end{:452};
while mem[mem[h+6].hh.rh].hh.rh<>0 do begin p:=mem[mem[h+6].hh.rh].hh.rh
;mem[h+6].hh.rh:=p;
case mem[p].hh.b0 of 6:if toplevel then confusion(590)else goto 10;
{453:}1:begin pathbbox(mem[p+1].hh.rh);adjustbbox(h);end;{:453}{454:}
5:if internal[33]>0 then mem[h+6].hh.lh:=2 else begin mem[h+6].hh.lh:=1;
pathbbox(mem[p+1].hh.rh);adjustbbox(h);{455:}lev:=1;
while lev<>0 do begin if mem[p].hh.rh=0 then confusion(591);
p:=mem[p].hh.rh;
if mem[p].hh.b0=5 then incr(lev)else if mem[p].hh.b0=7 then decr(lev);
end;mem[h+6].hh.rh:=p{:455};end;
7:if internal[33]<=0 then confusion(591);{:454}{456:}
2:begin pathbbox(mem[p+1].hh.rh);x0:=bbmin[0];y0:=bbmin[1];x1:=bbmax[0];
y1:=bbmax[1];penbbox(mem[p+1].hh.lh);bbmin[0]:=bbmin[0]+x0;
bbmin[1]:=bbmin[1]+y0;bbmax[0]:=bbmax[0]+x1;bbmax[1]:=bbmax[1]+y1;
adjustbbox(h);
if(mem[mem[p+1].hh.rh].hh.b0=0)and(mem[p+6].hh.b0=2)then boxends(mem[p+1
].hh.rh,mem[p+1].hh.lh,h);end;{:456}{457:}
3:begin x1:=takescaled(mem[p+10].int,mem[p+5].int);
y0:=takescaled(mem[p+11].int,-mem[p+7].int);
y1:=takescaled(mem[p+11].int,mem[p+6].int);bbmin[0]:=mem[p+8].int;
bbmax[0]:=bbmin[0];if y0<y1 then begin bbmin[0]:=bbmin[0]+y0;
bbmax[0]:=bbmax[0]+y1;end else begin bbmin[0]:=bbmin[0]+y1;
bbmax[0]:=bbmax[0]+y0;end;
if x1<0 then bbmin[0]:=bbmin[0]+x1 else bbmax[0]:=bbmax[0]+x1;
x1:=takescaled(mem[p+12].int,mem[p+5].int);
y0:=takescaled(mem[p+13].int,-mem[p+7].int);
y1:=takescaled(mem[p+13].int,mem[p+6].int);bbmin[1]:=mem[p+9].int;
bbmax[1]:=bbmin[1];if y0<y1 then begin bbmin[1]:=bbmin[1]+y0;
bbmax[1]:=bbmax[1]+y1;end else begin bbmin[1]:=bbmin[1]+y1;
bbmax[1]:=bbmax[1]+y0;end;
if x1<0 then bbmin[1]:=bbmin[1]+x1 else bbmax[1]:=bbmax[1]+x1;
adjustbbox(h);end;{:457}{458:}4:begin pathbbox(mem[p+1].hh.rh);
x0:=bbmin[0];y0:=bbmin[1];x1:=bbmax[0];y1:=bbmax[1];sminx:=mem[h+2].int;
sminy:=mem[h+3].int;smaxx:=mem[h+4].int;smaxy:=mem[h+5].int;{459:}
mem[h+2].int:=2147483647;mem[h+3].int:=2147483647;
mem[h+4].int:=-2147483647;mem[h+5].int:=-2147483647;
setbbox(h,false){:459};{460:}if mem[h+2].int<x0 then mem[h+2].int:=x0;
if mem[h+3].int<y0 then mem[h+3].int:=y0;
if mem[h+4].int>x1 then mem[h+4].int:=x1;
if mem[h+5].int>y1 then mem[h+5].int:=y1{:460};bbmin[0]:=sminx;
bbmin[1]:=sminy;bbmax[0]:=smaxx;bbmax[1]:=smaxy;adjustbbox(h);end;{:458}
end;end;if not toplevel then confusion(590);10:end;{:451}{463:}{470:}
procedure splitcubic(p:halfword;t:fraction);var v:scaled;q,r:halfword;
begin q:=mem[p].hh.rh;r:=getnode(7);mem[p].hh.rh:=r;mem[r].hh.rh:=q;
mem[r].hh.b0:=1;mem[r].hh.b1:=1;
v:=mem[p+5].int-takefraction(mem[p+5].int-mem[q+3].int,t);
mem[p+5].int:=mem[p+1].int-takefraction(mem[p+1].int-mem[p+5].int,t);
mem[q+3].int:=mem[q+3].int-takefraction(mem[q+3].int-mem[q+1].int,t);
mem[r+3].int:=mem[p+5].int-takefraction(mem[p+5].int-v,t);
mem[r+5].int:=v-takefraction(v-mem[q+3].int,t);
mem[r+1].int:=mem[r+3].int-takefraction(mem[r+3].int-mem[r+5].int,t);
v:=mem[p+6].int-takefraction(mem[p+6].int-mem[q+4].int,t);
mem[p+6].int:=mem[p+2].int-takefraction(mem[p+2].int-mem[p+6].int,t);
mem[q+4].int:=mem[q+4].int-takefraction(mem[q+4].int-mem[q+2].int,t);
mem[r+4].int:=mem[p+6].int-takefraction(mem[p+6].int-v,t);
mem[r+6].int:=v-takefraction(v-mem[q+4].int,t);
mem[r+2].int:=mem[r+4].int-takefraction(mem[r+4].int-mem[r+6].int,t);
end;{:470}{471:}procedure removecubic(p:halfword);var q:halfword;
begin q:=mem[p].hh.rh;mem[p].hh.rh:=mem[q].hh.rh;
mem[p+5].int:=mem[q+5].int;mem[p+6].int:=mem[q+6].int;freenode(q,7);end;
{:471}{473:}function penwalk(w:halfword;k:integer):halfword;
begin while k>0 do begin w:=mem[w].hh.rh;decr(k);end;
while k<0 do begin w:=mem[w].hh.lh;incr(k);end;penwalk:=w;end;{:473}
{476:}procedure finoffsetprep(p:halfword;w:halfword;
x0,x1,x2,y0,y1,y2:integer;rise,turnamt:integer);label 10;
var ww:halfword;du,dv:scaled;t0,t1,t2:integer;t:fraction;s:fraction;
v:integer;q:halfword;begin q:=mem[p].hh.rh;
while true do begin if rise>0 then ww:=mem[w].hh.rh else ww:=mem[w].hh.
lh;{477:}du:=mem[ww+1].int-mem[w+1].int;dv:=mem[ww+2].int-mem[w+2].int;
if abs(du)>=abs(dv)then begin s:=makefraction(dv,du);
t0:=takefraction(x0,s)-y0;t1:=takefraction(x1,s)-y1;
t2:=takefraction(x2,s)-y2;if du<0 then begin t0:=-t0;t1:=-t1;t2:=-t2;
end end else begin s:=makefraction(du,dv);t0:=x0-takefraction(y0,s);
t1:=x1-takefraction(y1,s);t2:=x2-takefraction(y2,s);
if dv<0 then begin t0:=-t0;t1:=-t1;t2:=-t2;end end;
if t0<0 then t0:=0{:477};t:=crossingpoint(t0,t1,t2);
if t>=268435456 then if turnamt>0 then t:=268435456 else goto 10;{478:}
begin splitcubic(p,t);p:=mem[p].hh.rh;mem[p].hh.lh:=16384+rise;
decr(turnamt);v:=x0-takefraction(x0-x1,t);x1:=x1-takefraction(x1-x2,t);
x0:=v-takefraction(v-x1,t);v:=y0-takefraction(y0-y1,t);
y1:=y1-takefraction(y1-y2,t);y0:=v-takefraction(v-y1,t);
if turnamt<0 then begin t1:=t1-takefraction(t1-t2,t);if t1>0 then t1:=0;
t:=crossingpoint(0,-t1,-t2);if t>268435456 then t:=268435456;
incr(turnamt);
if(t=268435456)and(mem[p].hh.rh<>q)then mem[mem[p].hh.rh].hh.lh:=mem[mem
[p].hh.rh].hh.lh-rise else begin splitcubic(p,t);
mem[mem[p].hh.rh].hh.lh:=16384-rise;v:=x1-takefraction(x1-x2,t);
x1:=x0-takefraction(x0-x1,t);x2:=x1-takefraction(x1-v,t);
v:=y1-takefraction(y1-y2,t);y1:=y0-takefraction(y0-y1,t);
y2:=y1-takefraction(y1-v,t);end;end;end{:478};w:=ww;end;10:end;{:476}
{482:}function getturnamt(w:halfword;dx,dy:scaled;ccw:boolean):integer;
label 30;var ww:halfword;s:integer;t:integer;begin s:=0;
if ccw then begin ww:=mem[w].hh.rh;
repeat t:=abvscd(dy,mem[ww+1].int-mem[w+1].int,dx,mem[ww+2].int-mem[w+2]
.int);if t<0 then goto 30;incr(s);w:=ww;ww:=mem[ww].hh.rh;until t<=0;
30:end else begin ww:=mem[w].hh.lh;
while abvscd(dy,mem[w+1].int-mem[ww+1].int,dx,mem[w+2].int-mem[ww+2].int
)<0 do begin decr(s);w:=ww;ww:=mem[ww].hh.lh;end;end;getturnamt:=s;end;
{:482}function offsetprep(c,h:halfword):halfword;label 45;
var n:halfword;p,q,r,w,ww:halfword;kneeded:integer;w0:halfword;
dxin,dyin:scaled;turnamt:integer;{474:}x0,x1,x2,y0,y1,y2:integer;
t0,t1,t2:integer;du,dv,dx,dy:integer;dx0,dy0:integer;maxcoef:integer;
x0a,x1a,x2a,y0a,y1a,y2a:integer;t:fraction;s:fraction;{:474}{487:}
u0,u1,v0,v1:integer;ss:integer;dsign:-1..1;{:487}begin{466:}n:=0;p:=h;
repeat incr(n);p:=mem[p].hh.rh;until p=h{:466};{467:}
dxin:=mem[mem[h].hh.rh+1].int-mem[mem[h].hh.lh+1].int;
dyin:=mem[mem[h].hh.rh+2].int-mem[mem[h].hh.lh+2].int;
if(dxin=0)and(dyin=0)then begin dxin:=mem[mem[h].hh.lh+2].int-mem[h+2].
int;dyin:=mem[h+1].int-mem[mem[h].hh.lh+1].int;end;w0:=h{:467};p:=c;
kneeded:=0;repeat q:=mem[p].hh.rh;{472:}mem[p].hh.lh:=16384+kneeded;
kneeded:=0;{475:}x0:=mem[p+5].int-mem[p+1].int;
x2:=mem[q+1].int-mem[q+3].int;x1:=mem[q+3].int-mem[p+5].int;
y0:=mem[p+6].int-mem[p+2].int;y2:=mem[q+2].int-mem[q+4].int;
y1:=mem[q+4].int-mem[p+6].int;maxcoef:=abs(x0);
if abs(x1)>maxcoef then maxcoef:=abs(x1);
if abs(x2)>maxcoef then maxcoef:=abs(x2);
if abs(y0)>maxcoef then maxcoef:=abs(y0);
if abs(y1)>maxcoef then maxcoef:=abs(y1);
if abs(y2)>maxcoef then maxcoef:=abs(y2);if maxcoef=0 then goto 45;
while maxcoef<134217728 do begin maxcoef:=maxcoef+maxcoef;x0:=x0+x0;
x1:=x1+x1;x2:=x2+x2;y0:=y0+y0;y1:=y1+y1;y2:=y2+y2;end{:475};{479:}
dx:=x0;dy:=y0;if dx=0 then if dy=0 then begin dx:=x1;dy:=y1;
if dx=0 then if dy=0 then begin dx:=x2;dy:=y2;end;end;
if p=c then begin dx0:=dx;dy0:=dy;end{:479};{481:}
turnamt:=getturnamt(w0,dx,dy,abvscd(dy,dxin,dx,dyin)>=0);
w:=penwalk(w0,turnamt);w0:=w;mem[p].hh.lh:=mem[p].hh.lh+turnamt{:481};
{480:}dxin:=x2;dyin:=y2;if dxin=0 then if dyin=0 then begin dxin:=x1;
dyin:=y1;if dxin=0 then if dyin=0 then begin dxin:=x0;dyin:=y0;end;
end{:480};{488:}dsign:=abvscd(dx,dyin,dxin,dy);
if dsign=0 then if dx=0 then if dy>0 then dsign:=1 else dsign:=-1 else
if dx>0 then dsign:=1 else dsign:=-1;{489:}
t0:=half(takefraction(x0,y2))-half(takefraction(x2,y0));
t1:=half(takefraction(x1,y0+y2))-half(takefraction(y1,x0+x2));
if t0=0 then t0:=dsign;if t0>0 then begin t:=crossingpoint(t0,t1,-t0);
u0:=x0-takefraction(x0-x1,t);u1:=x1-takefraction(x1-x2,t);
v0:=y0-takefraction(y0-y1,t);v1:=y1-takefraction(y1-y2,t);
end else begin t:=crossingpoint(-t0,t1,t0);u0:=x2-takefraction(x2-x1,t);
u1:=x1-takefraction(x1-x0,t);v0:=y2-takefraction(y2-y1,t);
v1:=y1-takefraction(y1-y0,t);end;
ss:=takefraction(x0+x2,u0-takefraction(u0-u1,t))+takefraction(y0+y2,v0-
takefraction(v0-v1,t)){:489};turnamt:=getturnamt(w,dxin,dyin,dsign>0);
if ss<0 then turnamt:=turnamt-dsign*n{:488};{484:}ww:=mem[w].hh.lh;
{477:}du:=mem[ww+1].int-mem[w+1].int;dv:=mem[ww+2].int-mem[w+2].int;
if abs(du)>=abs(dv)then begin s:=makefraction(dv,du);
t0:=takefraction(x0,s)-y0;t1:=takefraction(x1,s)-y1;
t2:=takefraction(x2,s)-y2;if du<0 then begin t0:=-t0;t1:=-t1;t2:=-t2;
end end else begin s:=makefraction(du,dv);t0:=x0-takefraction(y0,s);
t1:=x1-takefraction(y1,s);t2:=x2-takefraction(y2,s);
if dv<0 then begin t0:=-t0;t1:=-t1;t2:=-t2;end end;
if t0<0 then t0:=0{:477};{486:}t:=crossingpoint(t0,t1,t2);
if turnamt>=0 then if t2<0 then t:=268435457 else begin u0:=x0-
takefraction(x0-x1,t);u1:=x1-takefraction(x1-x2,t);
ss:=takefraction(-du,u0-takefraction(u0-u1,t));
v0:=y0-takefraction(y0-y1,t);v1:=y1-takefraction(y1-y2,t);
ss:=ss+takefraction(-dv,v0-takefraction(v0-v1,t));
if ss<0 then t:=268435457;end else if t>268435456 then t:=268435456;
{:486};
if t>268435456 then finoffsetprep(p,w,x0,x1,x2,y0,y1,y2,1,turnamt)else
begin splitcubic(p,t);r:=mem[p].hh.rh;x1a:=x0-takefraction(x0-x1,t);
x1:=x1-takefraction(x1-x2,t);x2a:=x1a-takefraction(x1a-x1,t);
y1a:=y0-takefraction(y0-y1,t);y1:=y1-takefraction(y1-y2,t);
y2a:=y1a-takefraction(y1a-y1,t);
finoffsetprep(p,w,x0,x1a,x2a,y0,y1a,y2a,1,0);x0:=x2a;y0:=y2a;
mem[r].hh.lh:=16383;
if turnamt>=0 then begin t1:=t1-takefraction(t1-t2,t);
if t1>0 then t1:=0;t:=crossingpoint(0,-t1,-t2);
if t>268435456 then t:=268435456;{485:}splitcubic(r,t);
mem[mem[r].hh.rh].hh.lh:=16385;x1a:=x1-takefraction(x1-x2,t);
x1:=x0-takefraction(x0-x1,t);x0a:=x1-takefraction(x1-x1a,t);
y1a:=y1-takefraction(y1-y2,t);y1:=y0-takefraction(y0-y1,t);
y0a:=y1-takefraction(y1-y1a,t);
finoffsetprep(mem[r].hh.rh,w,x0a,x1a,x2,y0a,y1a,y2,1,turnamt);x2:=x0a;
y2:=y0a{:485};finoffsetprep(r,ww,x0,x1,x2,y0,y1,y2,-1,0);
end else finoffsetprep(r,ww,x0,x1,x2,y0,y1,y2,-1,-1-turnamt);end{:484};
w0:=penwalk(w0,turnamt);45:{:472};{468:}repeat r:=mem[p].hh.rh;
if mem[p+1].int=mem[p+5].int then if mem[p+2].int=mem[p+6].int then if
mem[p+1].int=mem[r+3].int then if mem[p+2].int=mem[r+4].int then if mem[
p+1].int=mem[r+1].int then if mem[p+2].int=mem[r+2].int then if r<>p
then{469:}begin kneeded:=mem[p].hh.lh-16384;
if r=q then q:=p else begin mem[p].hh.lh:=kneeded+mem[r].hh.lh;
kneeded:=0;end;if r=c then begin mem[p].hh.lh:=mem[c].hh.lh;c:=p;end;
if r=specp1 then specp1:=p;if r=specp2 then specp2:=p;r:=p;
removecubic(p);end{:469};p:=r;until p=q{:468};until q=c;{483:}
specoffset:=mem[c].hh.lh-16384;
if mem[c].hh.rh=c then mem[c].hh.lh:=16384+n else begin mem[c].hh.lh:=
mem[c].hh.lh+kneeded;while w0<>h do begin mem[c].hh.lh:=mem[c].hh.lh+1;
w0:=mem[w0].hh.rh;end;
while mem[c].hh.lh<=16384-n do mem[c].hh.lh:=mem[c].hh.lh+n;
while mem[c].hh.lh>16384 do mem[c].hh.lh:=mem[c].hh.lh-n;
if(mem[c].hh.lh<>16384)and(abvscd(dy0,dxin,dx0,dyin)>=0)then mem[c].hh.
lh:=mem[c].hh.lh+n;end;offsetprep:=c{:483};end;{:463}{490:}
procedure printspec(curspec,curpen:halfword;s:strnumber);
var p,q:halfword;w:halfword;begin printdiagnostic(592,s,true);
p:=curspec;w:=penwalk(curpen,specoffset);println;
printtwo(mem[curspec+1].int,mem[curspec+2].int);print(593);
printtwo(mem[w+1].int,mem[w+2].int);repeat repeat q:=mem[p].hh.rh;{492:}
begin printnl(598);printtwo(mem[p+5].int,mem[p+6].int);print(537);
printtwo(mem[q+3].int,mem[q+4].int);printnl(534);
printtwo(mem[q+1].int,mem[q+2].int);end{:492};p:=q;
until(p=curspec)or(mem[p].hh.lh<>16384);
if mem[p].hh.lh<>16384 then{491:}begin w:=penwalk(w,mem[p].hh.lh-16384);
print(595);if mem[p].hh.lh>16384 then print(596);print(597);
printtwo(mem[w+1].int,mem[w+2].int);end{:491};until p=curspec;
printnl(594);enddiagnostic(true);end;{:490}{493:}{500:}
function insertknot(q:halfword;x,y:scaled):halfword;var r:halfword;
begin r:=getnode(7);mem[r].hh.rh:=mem[q].hh.rh;mem[q].hh.rh:=r;
mem[r+5].int:=mem[q+5].int;mem[r+6].int:=mem[q+6].int;mem[r+1].int:=x;
mem[r+2].int:=y;mem[q+5].int:=mem[q+1].int;mem[q+6].int:=mem[q+2].int;
mem[r+3].int:=mem[r+1].int;mem[r+4].int:=mem[r+2].int;mem[r].hh.b0:=1;
mem[r].hh.b1:=1;insertknot:=r;end;{:500}
function makeenvelope(c,h:halfword;ljoin,lcap:smallnumber;
miterlim:scaled):halfword;label 30;var p,q,r,q0:halfword;jointype:0..3;
w,w0:halfword;qx,qy:scaled;k,k0:halfword;{497:}
dxin,dyin,dxout,dyout:fraction;tmp:scaled;{:497}{503:}det:fraction;
{:503}{505:}htx,hty:fraction;maxht:scaled;kk:halfword;ww:halfword;{:505}
begin specp1:=0;specp2:=0;if mem[c].hh.b0=0 then{508:}
begin specp1:=htapypoc(c);specp2:=pathtail;
mem[specp2].hh.rh:=mem[specp1].hh.rh;mem[specp1].hh.rh:=c;
removecubic(specp1);c:=specp1;
if c<>mem[c].hh.rh then removecubic(specp2)else{509:}
begin mem[c].hh.b0:=1;mem[c].hh.b1:=1;mem[c+3].int:=mem[c+1].int;
mem[c+4].int:=mem[c+2].int;mem[c+5].int:=mem[c+1].int;
mem[c+6].int:=mem[c+2].int;end;{:509};end{:508};{494:}
c:=offsetprep(c,h);if internal[5]>0 then printspec(c,h,284);
h:=penwalk(h,specoffset){:494};w:=h;p:=c;repeat q:=mem[p].hh.rh;q0:=q;
qx:=mem[q+1].int;qy:=mem[q+2].int;k:=mem[q].hh.lh;k0:=k;w0:=w;
if k<>16384 then{495:}
if k<16384 then jointype:=2 else begin if(q<>specp1)and(q<>specp2)then
jointype:=ljoin else if lcap=2 then jointype:=3 else jointype:=2-lcap;
if(jointype=0)or(jointype=3)then begin{510:}
dxin:=mem[q+1].int-mem[q+3].int;dyin:=mem[q+2].int-mem[q+4].int;
if(dxin=0)and(dyin=0)then begin dxin:=mem[q+1].int-mem[p+5].int;
dyin:=mem[q+2].int-mem[p+6].int;
if(dxin=0)and(dyin=0)then begin dxin:=mem[q+1].int-mem[p+1].int;
dyin:=mem[q+2].int-mem[p+2].int;
if p<>c then begin dxin:=dxin+mem[w+1].int;dyin:=dyin+mem[w+2].int;end;
end;end;tmp:=pythadd(dxin,dyin);
if tmp=0 then jointype:=2 else begin dxin:=makefraction(dxin,tmp);
dyin:=makefraction(dyin,tmp);{511:}dxout:=mem[q+5].int-mem[q+1].int;
dyout:=mem[q+6].int-mem[q+2].int;
if(dxout=0)and(dyout=0)then begin r:=mem[q].hh.rh;
dxout:=mem[r+3].int-mem[q+1].int;dyout:=mem[r+4].int-mem[q+2].int;
if(dxout=0)and(dyout=0)then begin dxout:=mem[r+1].int-mem[q+1].int;
dyout:=mem[r+2].int-mem[q+2].int;end;end;
if q=c then begin dxout:=dxout-mem[h+1].int;dyout:=dyout-mem[h+2].int;
end;tmp:=pythadd(dxout,dyout);if tmp=0 then confusion(599);
dxout:=makefraction(dxout,tmp);dyout:=makefraction(dyout,tmp){:511};
end{:510};if jointype=0 then{496:}
begin tmp:=takefraction(miterlim,134217728+half(takefraction(dxin,dxout)
+takefraction(dyin,dyout)));
if tmp<65536 then if takescaled(miterlim,tmp)<65536 then jointype:=2;
end{:496};end;end{:495};{498:}mem[p+5].int:=mem[p+5].int+mem[w+1].int;
mem[p+6].int:=mem[p+6].int+mem[w+2].int;
mem[q+3].int:=mem[q+3].int+mem[w+1].int;
mem[q+4].int:=mem[q+4].int+mem[w+2].int;
mem[q+1].int:=mem[q+1].int+mem[w+1].int;
mem[q+2].int:=mem[q+2].int+mem[w+2].int;mem[q].hh.b0:=1;
mem[q].hh.b1:=1{:498};while k<>16384 do begin{499:}
if k>16384 then begin w:=mem[w].hh.rh;decr(k);
end else begin w:=mem[w].hh.lh;incr(k);end{:499};
if(jointype=1)or(k=16384)then q:=insertknot(q,qx+mem[w+1].int,qy+mem[w+2
].int);end;if q<>mem[p].hh.rh then{501:}begin p:=mem[p].hh.rh;
if(jointype=0)or(jointype=3)then begin if jointype=0 then{502:}
begin det:=takefraction(dyout,dxin)-takefraction(dxout,dyin);
if abs(det)<26844 then r:=0 else begin tmp:=takefraction(mem[q+1].int-
mem[p+1].int,dyout)-takefraction(mem[q+2].int-mem[p+2].int,dxout);
tmp:=makefraction(tmp,det);
r:=insertknot(p,mem[p+1].int+takefraction(tmp,dxin),mem[p+2].int+
takefraction(tmp,dyin));end;end{:502}else{504:}
begin htx:=mem[w+2].int-mem[w0+2].int;hty:=mem[w0+1].int-mem[w+1].int;
while(abs(htx)<134217728)and(abs(hty)<134217728)do begin htx:=htx+htx;
hty:=hty+hty;end;{506:}maxht:=0;kk:=16384;ww:=w;
while true do begin{507:}if kk>k0 then begin ww:=mem[ww].hh.rh;decr(kk);
end else begin ww:=mem[ww].hh.lh;incr(kk);end{:507};
if kk=k0 then goto 30;
tmp:=takefraction(mem[ww+1].int-mem[w0+1].int,htx)+takefraction(mem[ww+2
].int-mem[w0+2].int,hty);if tmp>maxht then maxht:=tmp;end;30:{:506};
tmp:=makefraction(maxht,takefraction(dxin,htx)+takefraction(dyin,hty));
r:=insertknot(p,mem[p+1].int+takefraction(tmp,dxin),mem[p+2].int+
takefraction(tmp,dyin));
tmp:=makefraction(maxht,takefraction(dxout,htx)+takefraction(dyout,hty))
;r:=insertknot(r,mem[q+1].int+takefraction(tmp,dxout),mem[q+2].int+
takefraction(tmp,dyout));end{:504};
if r<>0 then begin mem[r+5].int:=mem[r+1].int;
mem[r+6].int:=mem[r+2].int;end;end;end{:501};p:=q;until q0=c;
makeenvelope:=c;end;{:493}{513:}function finddirectiontime(x,y:scaled;
h:halfword):scaled;label 10,40,45,30;var max:scaled;p,q:halfword;
n:scaled;tt:scaled;{516:}x1,x2,x3,y1,y2,y3:scaled;theta,phi:angle;
t:fraction;{:516}begin{514:}
if abs(x)<abs(y)then begin x:=makefraction(x,abs(y));
if y>0 then y:=268435456 else y:=-268435456;
end else if x=0 then begin finddirectiontime:=0;goto 10;
end else begin y:=makefraction(y,abs(x));
if x>0 then x:=268435456 else x:=-268435456;end{:514};n:=0;p:=h;
while true do begin if mem[p].hh.b1=0 then goto 45;q:=mem[p].hh.rh;
{515:}tt:=0;{517:}x1:=mem[p+5].int-mem[p+1].int;
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
y3:=takefraction(y3,x)-takefraction(t,y){:517};
if y1=0 then if x1>=0 then goto 40;if n>0 then begin{518:}
theta:=narg(x1,y1);
if theta>=0 then if phi<=0 then if phi>=theta-188743680 then goto 40;
if theta<=0 then if phi>=0 then if phi<=theta+188743680 then goto 40{:
518};if p=h then goto 45;end;if(x3<>0)or(y3<>0)then phi:=narg(x3,y3);
{520:}if x1<0 then if x2<0 then if x3<0 then goto 30;
if abvscd(y1,y3,y2,y2)=0 then{522:}
begin if abvscd(y1,y2,0,0)<0 then begin t:=makefraction(y1,y1-y2);
x1:=x1-takefraction(x1-x2,t);x2:=x2-takefraction(x2-x3,t);
if x1-takefraction(x1-x2,t)>=0 then begin tt:=(t+2048)div 4096;goto 40;
end;end else if y3=0 then if y1=0 then{523:}
begin t:=crossingpoint(-x1,-x2,-x3);
if t<=268435456 then begin tt:=(t+2048)div 4096;goto 40;end;
if abvscd(x1,x3,x2,x2)<=0 then begin t:=makefraction(x1,x1-x2);
begin tt:=(t+2048)div 4096;goto 40;end;end;end{:523}
else if x3>=0 then begin tt:=65536;goto 40;end;goto 30;end{:522};
if y1<=0 then if y1<0 then begin y1:=-y1;y2:=-y2;y3:=-y3;
end else if y2>0 then begin y2:=-y2;y3:=-y3;end;{521:}
t:=crossingpoint(y1,y2,y3);if t>268435456 then goto 30;
y2:=y2-takefraction(y2-y3,t);x1:=x1-takefraction(x1-x2,t);
x2:=x2-takefraction(x2-x3,t);x1:=x1-takefraction(x1-x2,t);
if x1>=0 then begin tt:=(t+2048)div 4096;goto 40;end;if y2>0 then y2:=0;
tt:=t;t:=crossingpoint(0,-y2,-y3);if t>268435456 then goto 30;
x1:=x1-takefraction(x1-x2,t);x2:=x2-takefraction(x2-x3,t);
if x1-takefraction(x1-x2,t)>=0 then begin t:=tt-takefraction(tt
-268435456,t);begin tt:=(t+2048)div 4096;goto 40;end;end{:521};30:{:520}
{:515};p:=q;n:=n+65536;end;45:finddirectiontime:=-65536;goto 10;
40:finddirectiontime:=n+tt;10:end;{:513}{531:}
procedure cubicintersection(p,pp:halfword);label 22,45,10;
var q,qq:halfword;begin timetogo:=5000;maxt:=2;{533:}q:=mem[p].hh.rh;
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
tol:=0;uv:=bisectptr;xy:=bisectptr;threel:=0;curt:=1;curtt:=1{:533};
while true do begin 22:if delx-tol<=bisectstack[xy-11]-bisectstack[uv-2]
then if delx+tol>=bisectstack[xy-12]-bisectstack[uv-1]then if dely-tol<=
bisectstack[xy-16]-bisectstack[uv-7]then if dely+tol>=bisectstack[xy-17]
-bisectstack[uv-6]then begin if curt>=maxt then begin if maxt=131072
then begin curt:=halfp(curt+1);curtt:=halfp(curtt+1);goto 10;end;
maxt:=maxt+maxt;apprt:=curt;apprtt:=curtt;end;{534:}
bisectstack[bisectptr]:=delx;bisectstack[bisectptr+1]:=dely;
bisectstack[bisectptr+2]:=tol;bisectstack[bisectptr+3]:=uv;
bisectstack[bisectptr+4]:=xy;bisectptr:=bisectptr+45;curt:=curt+curt;
curtt:=curtt+curtt;bisectstack[bisectptr-25]:=bisectstack[uv-5];
bisectstack[bisectptr-3]:=bisectstack[uv-3];
bisectstack[bisectptr-24]:=half(bisectstack[bisectptr-25]+bisectstack[uv
-4]);
bisectstack[bisectptr-4]:=half(bisectstack[bisectptr-3]+bisectstack[uv-4
]);
bisectstack[bisectptr-23]:=half(bisectstack[bisectptr-24]+bisectstack[
bisectptr-4]);bisectstack[bisectptr-5]:=bisectstack[bisectptr-23];
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
bisectstack[bisectptr-29]:=half(bisectstack[bisectptr-30]+bisectstack[uv
-9]);
bisectstack[bisectptr-9]:=half(bisectstack[bisectptr-8]+bisectstack[uv-9
]);
bisectstack[bisectptr-28]:=half(bisectstack[bisectptr-29]+bisectstack[
bisectptr-9]);bisectstack[bisectptr-10]:=bisectstack[bisectptr-28];
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
bisectstack[bisectptr-34]:=half(bisectstack[bisectptr-35]+bisectstack[xy
-14]);
bisectstack[bisectptr-14]:=half(bisectstack[bisectptr-13]+bisectstack[xy
-14]);
bisectstack[bisectptr-33]:=half(bisectstack[bisectptr-34]+bisectstack[
bisectptr-14]);bisectstack[bisectptr-15]:=bisectstack[bisectptr-33];
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
bisectstack[bisectptr-39]:=half(bisectstack[bisectptr-40]+bisectstack[xy
-19]);
bisectstack[bisectptr-19]:=half(bisectstack[bisectptr-18]+bisectstack[xy
-19]);
bisectstack[bisectptr-38]:=half(bisectstack[bisectptr-39]+bisectstack[
bisectptr-19]);bisectstack[bisectptr-20]:=bisectstack[bisectptr-38];
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
tol:=tol-threel+tolstep;tol:=tol+tol;threel:=threel+tolstep{:534};
goto 22;end;
if timetogo>0 then decr(timetogo)else begin while apprt<65536 do begin
apprt:=apprt+apprt;apprtt:=apprtt+apprtt;end;curt:=apprt;curtt:=apprtt;
goto 10;end;{535:}45:if odd(curtt)then if odd(curt)then{536:}
begin curt:=halfp(curt);curtt:=halfp(curtt);if curt=0 then goto 10;
bisectptr:=bisectptr-45;threel:=threel-tolstep;
delx:=bisectstack[bisectptr];dely:=bisectstack[bisectptr+1];
tol:=bisectstack[bisectptr+2];uv:=bisectstack[bisectptr+3];
xy:=bisectstack[bisectptr+4];goto 45;end{:536}else begin incr(curt);
delx:=delx+bisectstack[uv-5]+bisectstack[uv-4]+bisectstack[uv-3];
dely:=dely+bisectstack[uv-10]+bisectstack[uv-9]+bisectstack[uv-8];
uv:=uv+20;decr(curtt);xy:=xy-20;
delx:=delx+bisectstack[xy-15]+bisectstack[xy-14]+bisectstack[xy-13];
dely:=dely+bisectstack[xy-20]+bisectstack[xy-19]+bisectstack[xy-18];
end else begin incr(curtt);tol:=tol+threel;
delx:=delx-bisectstack[xy-15]-bisectstack[xy-14]-bisectstack[xy-13];
dely:=dely-bisectstack[xy-20]-bisectstack[xy-19]-bisectstack[xy-18];
xy:=xy+20;end{:535};end;10:end;{:531}{537:}
procedure pathintersection(h,hh:halfword);label 10;var p,pp:halfword;
n,nn:integer;begin{538:}
if mem[h].hh.b1=0 then begin mem[h+5].int:=mem[h+1].int;
mem[h+3].int:=mem[h+1].int;mem[h+6].int:=mem[h+2].int;
mem[h+4].int:=mem[h+2].int;mem[h].hh.b1:=1;end;
if mem[hh].hh.b1=0 then begin mem[hh+5].int:=mem[hh+1].int;
mem[hh+3].int:=mem[hh+1].int;mem[hh+6].int:=mem[hh+2].int;
mem[hh+4].int:=mem[hh+2].int;mem[hh].hh.b1:=1;end;{:538};tolstep:=0;
repeat n:=-65536;p:=h;repeat if mem[p].hh.b1<>0 then begin nn:=-65536;
pp:=hh;repeat if mem[pp].hh.b1<>0 then begin cubicintersection(p,pp);
if curt>0 then begin curt:=curt+n;curtt:=curtt+nn;goto 10;end;end;
nn:=nn+65536;pp:=mem[pp].hh.rh;until pp=hh;end;n:=n+65536;
p:=mem[p].hh.rh;until p=h;tolstep:=tolstep+3;until tolstep>3;
curt:=-65536;curtt:=-65536;10:end;{:537}{545:}
function maxcoef(p:halfword):fraction;var x:fraction;begin x:=0;
while mem[p].hh.lh<>0 do begin if abs(mem[p+1].int)>x then x:=abs(mem[p
+1].int);p:=mem[p].hh.rh;end;maxcoef:=x;end;{:545}{551:}
function pplusq(p:halfword;q:halfword;t:smallnumber):halfword;label 30;
var pp,qq:halfword;r,s:halfword;threshold:integer;v:integer;
begin if t=17 then threshold:=2685 else threshold:=8;r:=memtop-1;
pp:=mem[p].hh.lh;qq:=mem[q].hh.lh;
while true do if pp=qq then if pp=0 then goto 30 else{552:}
begin v:=mem[p+1].int+mem[q+1].int;mem[p+1].int:=v;s:=p;p:=mem[p].hh.rh;
pp:=mem[p].hh.lh;
if abs(v)<threshold then freenode(s,2)else begin if abs(v)>=626349397
then if watchcoefs then begin mem[qq].hh.b0:=0;fixneeded:=true;end;
mem[r].hh.rh:=s;r:=s;end;q:=mem[q].hh.rh;qq:=mem[q].hh.lh;end{:552}
else if mem[pp+1].int<mem[qq+1].int then begin s:=getnode(2);
mem[s].hh.lh:=qq;mem[s+1].int:=mem[q+1].int;q:=mem[q].hh.rh;
qq:=mem[q].hh.lh;mem[r].hh.rh:=s;r:=s;end else begin mem[r].hh.rh:=p;
r:=p;p:=mem[p].hh.rh;pp:=mem[p].hh.lh;end;
30:mem[p+1].int:=slowadd(mem[p+1].int,mem[q+1].int);mem[r].hh.rh:=p;
depfinal:=p;pplusq:=mem[memtop-1].hh.rh;end;{:551}{553:}
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
{:553}{555:}function pwithxbecomingq(p,x,q:halfword;
t:smallnumber):halfword;var r,s:halfword;v:integer;sx:integer;
begin s:=p;r:=memtop-1;sx:=mem[x+1].int;
while mem[mem[s].hh.lh+1].int>sx do begin r:=s;s:=mem[s].hh.rh;end;
if mem[s].hh.lh<>x then pwithxbecomingq:=p else begin mem[memtop-1].hh.
rh:=p;mem[r].hh.rh:=mem[s].hh.rh;v:=mem[s+1].int;freenode(s,2);
pwithxbecomingq:=pplusfq(mem[memtop-1].hh.rh,v,q,t,17);end;end;{:555}
{560:}procedure newdep(q,p:halfword);var r:halfword;
begin mem[q+1].hh.rh:=p;mem[q+1].hh.lh:=5;r:=mem[5].hh.rh;
mem[depfinal].hh.rh:=r;mem[r+1].hh.lh:=depfinal;mem[5].hh.rh:=q;end;
{:560}{561:}function constdependency(v:scaled):halfword;
begin depfinal:=getnode(2);mem[depfinal+1].int:=v;
mem[depfinal].hh.lh:=0;constdependency:=depfinal;end;{:561}{562:}
function singledependency(p:halfword):halfword;var q:halfword;m:integer;
begin m:=mem[p+1].int mod 64;
if m>28 then singledependency:=constdependency(0)else begin q:=getnode(2
);mem[q+1].int:=twotothe[28-m];mem[q].hh.lh:=p;
mem[q].hh.rh:=constdependency(0);singledependency:=q;end;end;{:562}
{563:}function copydeplist(p:halfword):halfword;label 30;var q:halfword;
begin q:=getnode(2);depfinal:=q;
while true do begin mem[depfinal].hh.lh:=mem[p].hh.lh;
mem[depfinal+1].int:=mem[p+1].int;if mem[depfinal].hh.lh=0 then goto 30;
mem[depfinal].hh.rh:=getnode(2);depfinal:=mem[depfinal].hh.rh;
p:=mem[p].hh.rh;end;30:copydeplist:=q;end;{:563}{564:}
procedure lineareq(p:halfword;t:smallnumber);var q,r,s:halfword;
x:halfword;n:integer;v:integer;prevr:halfword;finalnode:halfword;
w:integer;begin{565:}q:=p;r:=mem[p].hh.rh;v:=mem[q+1].int;
while mem[r].hh.lh<>0 do begin if abs(mem[r+1].int)>abs(v)then begin q:=
r;v:=mem[r+1].int;end;r:=mem[r].hh.rh;end{:565};x:=mem[q].hh.lh;
n:=mem[x+1].int mod 64;{566:}s:=memtop-1;mem[s].hh.rh:=p;r:=p;
repeat if r=q then begin mem[s].hh.rh:=mem[r].hh.rh;freenode(r,2);
end else begin w:=makefraction(mem[r+1].int,v);
if abs(w)<=1342 then begin mem[s].hh.rh:=mem[r].hh.rh;freenode(r,2);
end else begin mem[r+1].int:=-w;s:=r;end;end;r:=mem[s].hh.rh;
until mem[r].hh.lh=0;
if t=18 then mem[r+1].int:=-makescaled(mem[r+1].int,v)else if v<>
-268435456 then mem[r+1].int:=-makefraction(mem[r+1].int,v);
finalnode:=r;p:=mem[memtop-1].hh.rh{:566};if internal[2]>0 then{567:}
if interesting(x)then begin begindiagnostic;printnl(608);
printvariablename(x);w:=n;while w>0 do begin print(601);w:=w-2;end;
printchar(61);printdependency(p,17);enddiagnostic(false);end{:567};
{568:}prevr:=5;r:=mem[5].hh.rh;while r<>5 do begin s:=mem[r+1].hh.rh;
q:=pwithxbecomingq(s,x,p,mem[r].hh.b0);
if mem[q].hh.lh=0 then makeknown(r,q)else begin mem[r+1].hh.rh:=q;
repeat q:=mem[q].hh.rh;until mem[q].hh.lh=0;prevr:=q;end;
r:=mem[prevr].hh.rh;end{:568};{569:}if n>0 then{570:}begin s:=memtop-1;
mem[memtop-1].hh.rh:=p;r:=p;
repeat if n>30 then w:=0 else w:=mem[r+1].int div twotothe[n];
if(abs(w)<=1342)and(mem[r].hh.lh<>0)then begin mem[s].hh.rh:=mem[r].hh.
rh;freenode(r,2);end else begin mem[r+1].int:=w;s:=r;end;
r:=mem[s].hh.rh;until mem[s].hh.lh=0;p:=mem[memtop-1].hh.rh;end{:570};
if mem[p].hh.lh=0 then begin mem[x].hh.b0:=16;
mem[x+1].int:=mem[p+1].int;
if abs(mem[x+1].int)>=268435456 then valtoobig(mem[x+1].int);
freenode(p,2);
if curexp=x then if curtype=19 then begin curexp:=mem[x+1].int;
curtype:=16;freenode(x,2);end;end else begin mem[x].hh.b0:=17;
depfinal:=finalnode;newdep(x,p);
if curexp=x then if curtype=19 then curtype:=17;end{:569};
if fixneeded then fixdependencies;end;{:564}{573:}
function newringentry(p:halfword):halfword;var q:halfword;
begin q:=getnode(2);mem[q].hh.b1:=14;mem[q].hh.b0:=mem[p].hh.b0;
if mem[p+1].int=0 then mem[q+1].int:=p else mem[q+1].int:=mem[p+1].int;
mem[p+1].int:=q;newringentry:=q;end;{:573}{575:}
procedure nonlineareq(v:integer;p:halfword;flushp:boolean);
var t:smallnumber;q,r:halfword;begin t:=mem[p].hh.b0-1;q:=mem[p+1].int;
if flushp then mem[p].hh.b0:=1 else p:=q;repeat r:=mem[q+1].int;
mem[q].hh.b0:=t;case t of 2:mem[q+1].int:=v;4:begin mem[q+1].int:=v;
begin if strref[v]<127 then incr(strref[v]);end;end;
6:mem[q+1].int:=makepen(copypath(v),false);8:mem[q+1].int:=copypath(v);
10:begin mem[q+1].int:=v;incr(mem[v].hh.lh);end;end;q:=r;until q=p;end;
{:575}{576:}procedure ringmerge(p,q:halfword);label 10;var r:halfword;
begin r:=mem[p+1].int;while r<>p do begin if r=q then begin{577:}
begin begin if interaction=3 then;printnl(262);print(611);end;
begin helpptr:=2;helpline[1]:=612;helpline[0]:=613;end;putgeterror;
end{:577};goto 10;end;r:=mem[r+1].int;end;r:=mem[p+1].int;
mem[p+1].int:=mem[q+1].int;mem[q+1].int:=r;10:end;{:576}{580:}
procedure showcmdmod(c,m:integer);begin begindiagnostic;printnl(123);
printcmdmod(c,m);printchar(125);enddiagnostic(false);end;{:580}{590:}
procedure showcontext;label 30;var oldsetting:0..10;{596:}i:0..bufsize;
l:integer;m:integer;n:0..errorline;p:integer;q:integer;{:596}
begin fileptr:=inputptr;inputstack[fileptr]:=curinput;
while true do begin curinput:=inputstack[fileptr];{591:}
if(fileptr=inputptr)or(curinput.indexfield<=15)or(curinput.indexfield<>
19)or(curinput.locfield<>0)then begin tally:=0;oldsetting:=selector;
if(curinput.indexfield<=15)then begin{592:}
if curinput.namefield>2 then begin printnl(curinput.namefield);
print(58);printint(trueline);print(58);
end else if(curinput.namefield=0)then if fileptr=0 then printnl(615)else
printnl(616)else if curinput.namefield=2 then printnl(617)else printnl(
618);printchar(32){:592};{599:}begin l:=tally;tally:=0;selector:=6;
trickcount:=1000000;end;
if curinput.limitfield>0 then for i:=curinput.startfield to curinput.
limitfield-1 do begin if i=curinput.locfield then begin firstcount:=
tally;trickcount:=tally+1+errorline-halferrorline;
if trickcount<errorline then trickcount:=errorline;end;print(buffer[i]);
end{:599};end else begin{593:}
case curinput.indexfield of 16:printnl(619);17:{594:}begin printnl(624);
p:=paramstack[curinput.limitfield];
if p<>0 then if mem[p].hh.rh=1 then printexp(p,0)else showtokenlist(p,0,
20,tally);print(625);end{:594};18:printnl(620);
19:if curinput.locfield=0 then printnl(621)else printnl(622);
20:printnl(623);21:begin println;
if curinput.namefield<>0 then print(hash[curinput.namefield].rh)else{595
:}begin p:=paramstack[curinput.limitfield];
if p=0 then showtokenlist(paramstack[curinput.limitfield+1],0,20,tally)
else begin q:=p;while mem[q].hh.rh<>0 do q:=mem[q].hh.rh;
mem[q].hh.rh:=paramstack[curinput.limitfield+1];
showtokenlist(p,0,20,tally);mem[q].hh.rh:=0;end;end{:595};print(513);
end;others:printnl(63)end{:593};{600:}begin l:=tally;tally:=0;
selector:=6;trickcount:=1000000;end;
if curinput.indexfield<>21 then showtokenlist(curinput.startfield,
curinput.locfield,100000,0)else showmacro(curinput.startfield,curinput.
locfield,100000){:600};end;selector:=oldsetting;{598:}
if trickcount=1000000 then begin firstcount:=tally;
trickcount:=tally+1+errorline-halferrorline;
if trickcount<errorline then trickcount:=errorline;end;
if tally<trickcount then m:=tally-firstcount else m:=trickcount-
firstcount;if l+firstcount<=halferrorline then begin p:=0;
n:=l+firstcount;end else begin print(275);
p:=l+firstcount-halferrorline+3;n:=halferrorline;end;
for q:=p to firstcount-1 do printchar(trickbuf[q mod errorline]);
println;for q:=1 to n do printchar(32);
if m+n<=errorline then p:=firstcount+m else p:=firstcount+(errorline-n-3
);for q:=firstcount to p-1 do printchar(trickbuf[q mod errorline]);
if m+n>errorline then print(275){:598};end{:591};
if(curinput.indexfield<=15)then if(curinput.namefield>2)or(fileptr=0)
then goto 30;decr(fileptr);end;30:curinput:=inputstack[inputptr];end;
{:590}{604:}procedure begintokenlist(p:halfword;t:quarterword);
begin begin if inputptr>maxinstack then begin maxinstack:=inputptr;
if inputptr=stacksize then overflow(626,stacksize);end;
inputstack[inputptr]:=curinput;incr(inputptr);end;
curinput.startfield:=p;curinput.indexfield:=t;
curinput.limitfield:=paramptr;curinput.locfield:=p;end;{:604}{605:}
procedure endtokenlist;label 30;var p:halfword;
begin if curinput.indexfield>=19 then if curinput.indexfield<=20 then
begin flushtokenlist(curinput.startfield);goto 30;
end else deletemacref(curinput.startfield);
while paramptr>curinput.limitfield do begin decr(paramptr);
p:=paramstack[paramptr];
if p<>0 then if mem[p].hh.rh=1 then begin recyclevalue(p);freenode(p,2);
end else flushtokenlist(p);end;30:begin decr(inputptr);
curinput:=inputstack[inputptr];end;
begin if interrupt<>0 then pauseforinstructions;end;end;{:605}{606:}
{845:}{846:}procedure encapsulate(p:halfword);begin curexp:=getnode(2);
mem[curexp].hh.b0:=curtype;mem[curexp].hh.b1:=14;newdep(curexp,p);end;
{:846}{848:}procedure install(r,q:halfword);var p:halfword;
begin if mem[q].hh.b0=16 then begin mem[r+1].int:=mem[q+1].int;
mem[r].hh.b0:=16;
end else if mem[q].hh.b0=19 then begin p:=singledependency(q);
if p=depfinal then begin mem[r].hh.b0:=16;mem[r+1].int:=0;freenode(p,2);
end else begin mem[r].hh.b0:=17;newdep(r,p);end;
end else begin mem[r].hh.b0:=mem[q].hh.b0;
newdep(r,copydeplist(mem[q+1].hh.rh));end;end;{:848}
procedure makeexpcopy(p:halfword);label 20;var q,r,t:halfword;
begin 20:curtype:=mem[p].hh.b0;
case curtype of 1,2,16:curexp:=mem[p+1].int;
3,5,7,11,9:curexp:=newringentry(p);4:begin curexp:=mem[p+1].int;
begin if strref[curexp]<127 then incr(strref[curexp]);end;end;
10:begin curexp:=mem[p+1].int;incr(mem[curexp].hh.lh);end;
6:curexp:=makepen(copypath(mem[p+1].int),false);
8:curexp:=copypath(mem[p+1].int);12,13,14:{847:}
begin if mem[p+1].int=0 then initbignode(p);t:=getnode(2);
mem[t].hh.b1:=14;mem[t].hh.b0:=curtype;initbignode(t);
q:=mem[p+1].int+bignodesize[curtype];
r:=mem[t+1].int+bignodesize[curtype];repeat q:=q-2;r:=r-2;install(r,q);
until q=mem[p+1].int;curexp:=t;end{:847};
17,18:encapsulate(copydeplist(mem[p+1].hh.rh));
15:begin begin mem[p].hh.b0:=19;serialno:=serialno+64;
mem[p+1].int:=serialno;end;goto 20;end;19:begin q:=singledependency(p);
if q=depfinal then begin curtype:=16;curexp:=0;freenode(q,2);
end else begin curtype:=17;encapsulate(q);end;end;
others:confusion(851)end;end;{:845}function curtok:halfword;
var p:halfword;savetype:smallnumber;saveexp:integer;
begin if cursym=0 then if curcmd=40 then begin savetype:=curtype;
saveexp:=curexp;makeexpcopy(curmod);p:=stashcurexp;mem[p].hh.rh:=0;
curtype:=savetype;curexp:=saveexp;end else begin p:=getnode(2);
mem[p+1].int:=curmod;mem[p].hh.b1:=15;
if curcmd=44 then mem[p].hh.b0:=16 else mem[p].hh.b0:=4;
end else begin begin p:=avail;
if p=0 then p:=getavail else begin avail:=mem[p].hh.rh;mem[p].hh.rh:=0;
ifdef('STAT')incr(dynused);endif('STAT')end;end;mem[p].hh.lh:=cursym;
end;curtok:=p;end;{:606}{607:}procedure backinput;var p:halfword;
begin p:=curtok;
while(curinput.indexfield>15)and(curinput.locfield=0)do endtokenlist;
begintokenlist(p,19);end;{:607}{608:}procedure backerror;
begin OKtointerrupt:=false;backinput;OKtointerrupt:=true;error;end;
procedure inserror;begin OKtointerrupt:=false;backinput;
curinput.indexfield:=20;OKtointerrupt:=true;error;end;{:608}{609:}
procedure beginfilereading;begin if inopen=15 then overflow(627,15);
if first=bufsize then overflow(256,bufsize);incr(inopen);
begin if inputptr>maxinstack then begin maxinstack:=inputptr;
if inputptr=stacksize then overflow(626,stacksize);end;
inputstack[inputptr]:=curinput;incr(inputptr);end;
curinput.indexfield:=inopen;mpxname[curinput.indexfield]:=1;
curinput.startfield:=first;curinput.namefield:=0;end;{:609}{610:}
procedure endfilereading;
begin if inopen>curinput.indexfield then if(mpxname[inopen]=1)or(
curinput.namefield<=2)then confusion(628)else begin aclose(inputfile[
inopen]);
begin if strref[mpxname[inopen]]<127 then if strref[mpxname[inopen]]>1
then decr(strref[mpxname[inopen]])else flushstring(mpxname[inopen]);end;
decr(inopen);end;first:=curinput.startfield;
if curinput.indexfield<>inopen then confusion(628);
if curinput.namefield>2 then begin aclose(inputfile[curinput.indexfield]
);begin if strref[curinput.namefield]<127 then if strref[curinput.
namefield]>1 then decr(strref[curinput.namefield])else flushstring(
curinput.namefield);end;
begin if strref[inamestack[curinput.indexfield]]<127 then if strref[
inamestack[curinput.indexfield]]>1 then decr(strref[inamestack[curinput.
indexfield]])else flushstring(inamestack[curinput.indexfield]);end;
begin if strref[iareastack[curinput.indexfield]]<127 then if strref[
iareastack[curinput.indexfield]]>1 then decr(strref[iareastack[curinput.
indexfield]])else flushstring(iareastack[curinput.indexfield]);end;end;
begin decr(inputptr);curinput:=inputstack[inputptr];end;decr(inopen);
end;{:610}{611:}function beginmpxreading:boolean;
begin if inopen<>curinput.indexfield+1 then beginmpxreading:=false else
begin if mpxname[inopen]<=1 then confusion(629);
if first=bufsize then overflow(256,bufsize);
begin if inputptr>maxinstack then begin maxinstack:=inputptr;
if inputptr=stacksize then overflow(626,stacksize);end;
inputstack[inputptr]:=curinput;incr(inputptr);end;
curinput.indexfield:=inopen;curinput.startfield:=first;
curinput.namefield:=mpxname[inopen];
begin if strref[curinput.namefield]<127 then incr(strref[curinput.
namefield]);end;{644:}last:=first;curinput.limitfield:=last;
buffer[curinput.limitfield]:=37;first:=curinput.limitfield+1;
curinput.locfield:=curinput.startfield{:644};beginmpxreading:=true;end;
end;{:611}{612:}procedure endmpxreading;
begin if inopen<>curinput.indexfield then confusion(629);
if curinput.locfield<curinput.limitfield then{614:}
begin begin if interaction=3 then;printnl(262);print(630);end;
begin helpptr:=4;helpline[3]:=631;helpline[2]:=632;helpline[1]:=633;
helpline[0]:=634;end;error;end{:614};first:=curinput.startfield;
begin decr(inputptr);curinput:=inputstack[inputptr];end;end;{:612}{615:}
procedure clearforerrorprompt;
begin while(curinput.indexfield<=15)and(curinput.namefield=0)and(
inputptr>0)and(curinput.locfield=curinput.limitfield)do endfilereading;
println;;end;{:615}{620:}function checkoutervalidity:boolean;
var p:halfword;
begin if scannerstatus=0 then checkoutervalidity:=true else if
scannerstatus=7 then{621:}
if cursym<>0 then checkoutervalidity:=true else begin deletionsallowed:=
false;begin if interaction=3 then;printnl(262);print(640);end;
printint(warninginfo);begin helpptr:=2;helpline[1]:=641;
helpline[0]:=642;end;cursym:=9768;inserror;deletionsallowed:=true;
checkoutervalidity:=false;end{:621}else begin deletionsallowed:=false;
{622:}if cursym<>0 then begin p:=getavail;mem[p].hh.lh:=cursym;
begintokenlist(p,19);end{:622};if scannerstatus>1 then{623:}
begin runaway;if cursym=0 then begin if interaction=3 then;printnl(262);
print(643);end else begin begin if interaction=3 then;printnl(262);
print(644);end;end;print(645);begin helpptr:=4;helpline[3]:=646;
helpline[2]:=647;helpline[1]:=648;helpline[0]:=649;end;
case scannerstatus of{624:}2:begin print(650);helpline[3]:=651;
cursym:=9763;end;3:begin print(652);helpline[3]:=653;
if warninginfo=0 then cursym:=9767 else begin cursym:=9759;
eqtb[9759].rh:=warninginfo;end;end;4,5:begin print(654);
if scannerstatus=5 then print(hash[warninginfo].rh)else
printvariablename(warninginfo);cursym:=9765;end;6:begin print(655);
print(hash[warninginfo].rh);print(656);helpline[3]:=657;cursym:=9764;
end;{:624}end;inserror;end{:623}else begin begin if interaction=3 then;
printnl(262);print(635);end;printint(warninginfo);begin helpptr:=3;
helpline[2]:=636;helpline[1]:=637;helpline[0]:=638;end;
if cursym=0 then helpline[2]:=639;cursym:=9766;inserror;end;
deletionsallowed:=true;checkoutervalidity:=false;end;end;{:620}{626:}
procedure firmuptheline;forward;{:626}{627:}procedure getnext;
label 20,10,50,40,25,85,86,87,30;var k:0..bufsize;c:ASCIIcode;
class:ASCIIcode;n,f:integer;begin 20:cursym:=0;
if(curinput.indexfield<=15)then{629:}
begin 25:c:=buffer[curinput.locfield];incr(curinput.locfield);
class:=charclass[c];case class of 0:goto 85;
1:begin class:=charclass[buffer[curinput.locfield]];
if class>1 then goto 25 else if class<1 then begin n:=0;goto 86;end;end;
2:goto 25;
3:begin if scannerstatus=7 then if curinput.locfield<curinput.limitfield
then goto 25;{640:}if curinput.namefield>2 then{642:}
begin incr(linestack[curinput.indexfield]);first:=curinput.startfield;
if not forceeof then begin if inputln(inputfile[curinput.indexfield],
true)then firmuptheline else forceeof:=true;end;
if forceeof then begin forceeof:=false;decr(curinput.locfield);
if(mpxname[curinput.indexfield]>1)then{643:}
begin mpxname[curinput.indexfield]:=0;begin if interaction=3 then;
printnl(262);print(676);end;begin helpptr:=4;helpline[3]:=677;
helpline[2]:=632;helpline[1]:=678;helpline[0]:=679;end;
deletionsallowed:=false;error;deletionsallowed:=true;cursym:=9769;
goto 50;end{:643}else begin printchar(41);decr(openparens);
fflush(stdout);endfilereading;
if checkoutervalidity then goto 20 else goto 20;end end;
buffer[curinput.limitfield]:=37;first:=curinput.limitfield+1;
curinput.locfield:=curinput.startfield;end{:642}
else begin if inputptr>0 then begin endfilereading;goto 20;end;
if selector<9 then openlogfile;
if interaction>1 then begin if curinput.limitfield=curinput.startfield
then printnl(674);println;first:=curinput.startfield;begin;print(42);
terminput;end;curinput.limitfield:=last;buffer[curinput.limitfield]:=37;
first:=curinput.limitfield+1;curinput.locfield:=curinput.startfield;
end else fatalerror(675);end{:640};
begin if interrupt<>0 then pauseforinstructions;end;goto 25;end;
4:if scannerstatus=7 then goto 25 else{631:}
begin if buffer[curinput.locfield]=34 then curmod:=284 else begin k:=
curinput.locfield;buffer[curinput.limitfield+1]:=34;
repeat incr(curinput.locfield);until buffer[curinput.locfield]=34;
if curinput.locfield>curinput.limitfield then{632:}
begin curinput.locfield:=curinput.limitfield;
begin if interaction=3 then;printnl(262);print(665);end;
begin helpptr:=3;helpline[2]:=666;helpline[1]:=667;helpline[0]:=668;end;
deletionsallowed:=false;error;deletionsallowed:=true;goto 20;end{:632};
if curinput.locfield=k+1 then curmod:=buffer[k]else begin begin if
poolptr+curinput.locfield-k>maxpoolptr then if poolptr+curinput.locfield
-k>poolsize then docompaction(curinput.locfield-k)else maxpoolptr:=
poolptr+curinput.locfield-k;end;
repeat begin strpool[poolptr]:=buffer[k];incr(poolptr);end;incr(k);
until k=curinput.locfield;curmod:=makestring;end;end;
incr(curinput.locfield);curcmd:=41;goto 10;end{:631};
5,6,7,8:begin k:=curinput.locfield-1;goto 40;end;
20:if scannerstatus=7 then goto 25 else{630:}
begin begin if interaction=3 then;printnl(262);print(662);end;
begin helpptr:=2;helpline[1]:=663;helpline[0]:=664;end;
deletionsallowed:=false;error;deletionsallowed:=true;goto 20;end{:630};
others:end;k:=curinput.locfield-1;
while charclass[buffer[curinput.locfield]]=class do incr(curinput.
locfield);goto 40;85:{633:}n:=c-48;
while charclass[buffer[curinput.locfield]]=0 do begin if n<32768 then n
:=10*n+buffer[curinput.locfield]-48;incr(curinput.locfield);end;
if buffer[curinput.locfield]=46 then if charclass[buffer[curinput.
locfield+1]]=0 then goto 30;f:=0;goto 87;
30:incr(curinput.locfield){:633};86:{634:}k:=0;
repeat if k<17 then begin dig[k]:=buffer[curinput.locfield]-48;incr(k);
end;incr(curinput.locfield);
until charclass[buffer[curinput.locfield]]<>0;f:=rounddecimals(k);
if f=65536 then begin incr(n);f:=0;end{:634};87:{635:}
if n<32768 then{636:}begin curmod:=n*65536+f;
if curmod>=268435456 then if(internal[30]>0)and(scannerstatus<>7)then
begin begin if interaction=3 then;printnl(262);print(672);end;
printscaled(curmod);printchar(41);begin helpptr:=3;helpline[2]:=673;
helpline[1]:=605;helpline[0]:=606;end;error;end;end{:636}
else if scannerstatus<>7 then begin begin if interaction=3 then;
printnl(262);print(669);end;begin helpptr:=2;helpline[1]:=670;
helpline[0]:=671;end;deletionsallowed:=false;error;
deletionsallowed:=true;curmod:=2147483647;end;curcmd:=44;goto 10{:635};
40:cursym:=idlookup(k,curinput.locfield-k);end{:629}else{637:}
if curinput.locfield>=himemmin then begin cursym:=mem[curinput.locfield]
.hh.lh;curinput.locfield:=mem[curinput.locfield].hh.rh;
if cursym>=9772 then if cursym>=9922 then{638:}
begin if cursym>=10072 then cursym:=cursym-150;
begintokenlist(paramstack[curinput.limitfield+cursym-(9922)],18);
goto 20;end{:638}else begin curcmd:=40;
curmod:=paramstack[curinput.limitfield+cursym-(9772)];cursym:=0;goto 10;
end;end else if curinput.locfield>0 then{639:}
begin if mem[curinput.locfield].hh.b1=15 then begin curmod:=mem[curinput
.locfield+1].int;
if mem[curinput.locfield].hh.b0=16 then curcmd:=44 else begin curcmd:=41
;begin if strref[curmod]<127 then incr(strref[curmod]);end;end;
end else begin curmod:=curinput.locfield;curcmd:=40;end;
curinput.locfield:=mem[curinput.locfield].hh.rh;goto 10;end{:639}
else begin endtokenlist;goto 20;end{:637};50:{628:}
curcmd:=eqtb[cursym].lh;curmod:=eqtb[cursym].rh;
if curcmd>=85 then if checkoutervalidity then curcmd:=curcmd-85 else
goto 20{:628};10:end;{:627}{645:}procedure firmuptheline;
var k:0..bufsize;begin curinput.limitfield:=last;
if internal[24]>0 then if interaction>1 then begin;println;
if curinput.startfield<curinput.limitfield then for k:=curinput.
startfield to curinput.limitfield-1 do print(buffer[k]);
first:=curinput.limitfield;begin;print(680);terminput;end;
if last>first then begin for k:=first to last-1 do buffer[k+curinput.
startfield-first]:=buffer[k];
curinput.limitfield:=curinput.startfield+last-first;end;end;end;{:645}
{649:}procedure startmpxinput;forward;procedure tnext;label 65,50;
var oldstatus:0..6;oldinfo:integer;
begin while curcmd<=3 do begin if curcmd=3 then if not(curinput.
indexfield<=15)or(mpxname[curinput.indexfield]=1)then{653:}
begin begin if interaction=3 then;printnl(262);print(690);end;
begin helpptr:=2;helpline[1]:=691;helpline[0]:=692;end;error;end{:653}
else begin endmpxreading;goto 65;
end else if curcmd=1 then if(curinput.indexfield>15)or(curinput.
namefield<=2)then{652:}begin begin if interaction=3 then;printnl(262);
print(686);end;begin helpptr:=3;helpline[2]:=687;helpline[1]:=688;
helpline[0]:=689;end;error;end{:652}
else if(mpxname[curinput.indexfield]>1)then{651:}
begin begin if interaction=3 then;printnl(262);print(683);end;
begin helpptr:=4;helpline[3]:=631;helpline[2]:=632;helpline[1]:=684;
helpline[0]:=685;end;error;end{:651}
else if(curmod<>1)and(mpxname[curinput.indexfield]<>0)then begin if not
beginmpxreading then startmpxinput;end else goto 65 else{654:}
begin begin if interaction=3 then;printnl(262);print(693);end;
begin helpptr:=1;helpline[0]:=694;end;error;end{:654};goto 50;65:{650:}
oldstatus:=scannerstatus;oldinfo:=warninginfo;scannerstatus:=7;
warninginfo:=linestack[curinput.indexfield];repeat getnext;
until curcmd=2;scannerstatus:=oldstatus;warninginfo:=oldinfo{:650};
50:getnext;end;end;{:649}{657:}function scantoks(terminator:commandcode;
substlist,tailend:halfword;suffixcount:smallnumber):halfword;
label 30,40;var p:halfword;q:halfword;balance:integer;begin p:=memtop-2;
balance:=1;mem[memtop-2].hh.rh:=0;while true do begin begin getnext;
if curcmd<=3 then tnext;end;if cursym>0 then begin{658:}
begin q:=substlist;
while q<>0 do begin if mem[q].hh.lh=cursym then begin cursym:=mem[q+1].
int;curcmd:=10;goto 40;end;q:=mem[q].hh.rh;end;40:end{:658};
if curcmd=terminator then{659:}
if curmod>0 then incr(balance)else begin decr(balance);
if balance=0 then goto 30;end{:659}else if curcmd=63 then{662:}
begin if curmod=0 then begin getnext;if curcmd<=3 then tnext;
end else if curmod<=suffixcount then cursym:=9921+curmod;end{:662};end;
mem[p].hh.rh:=curtok;p:=mem[p].hh.rh;end;30:mem[p].hh.rh:=tailend;
flushnodelist(substlist);scantoks:=mem[memtop-2].hh.rh;end;{:657}{663:}
procedure getsymbol;label 20;begin 20:begin getnext;
if curcmd<=3 then tnext;end;
if(cursym=0)or(cursym>9757)then begin begin if interaction=3 then;
printnl(262);print(706);end;begin helpptr:=3;helpline[2]:=707;
helpline[1]:=708;helpline[0]:=709;end;
if cursym>0 then helpline[2]:=710 else if curcmd=41 then begin if strref
[curmod]<127 then if strref[curmod]>1 then decr(strref[curmod])else
flushstring(curmod);end;cursym:=9757;inserror;goto 20;end;end;{:663}
{664:}procedure getclearsymbol;begin getsymbol;
clearsymbol(cursym,false);end;{:664}{665:}procedure checkequals;
begin if curcmd<>53 then if curcmd<>76 then begin missingerr(61);
begin helpptr:=5;helpline[4]:=711;helpline[3]:=712;helpline[2]:=713;
helpline[1]:=714;helpline[0]:=715;end;backerror;end;end;{:665}{666:}
procedure makeopdef;var m:commandcode;p,q,r:halfword;begin m:=curmod;
getsymbol;q:=getnode(2);mem[q].hh.lh:=cursym;mem[q+1].int:=9772;
getclearsymbol;warninginfo:=cursym;getsymbol;p:=getnode(2);
mem[p].hh.lh:=cursym;mem[p+1].int:=9773;mem[p].hh.rh:=q;begin getnext;
if curcmd<=3 then tnext;end;checkequals;scannerstatus:=5;q:=getavail;
mem[q].hh.lh:=0;r:=getavail;mem[q].hh.rh:=r;mem[r].hh.lh:=0;
mem[r].hh.rh:=scantoks(18,p,0,0);scannerstatus:=0;
eqtb[warninginfo].lh:=m;eqtb[warninginfo].rh:=q;getxnext;end;{:666}
{669:}{1049:}procedure checkdelimiter(ldelim,rdelim:halfword);label 10;
begin if curcmd=64 then if curmod=ldelim then goto 10;
if cursym<>rdelim then begin missingerr(hash[rdelim].rh);
begin helpptr:=2;helpline[1]:=970;helpline[0]:=971;end;backerror;
end else begin begin if interaction=3 then;printnl(262);print(972);end;
print(hash[rdelim].rh);print(973);begin helpptr:=3;helpline[2]:=974;
helpline[1]:=975;helpline[0]:=976;end;error;end;10:end;{:1049}{1028:}
function scandeclaredvariable:halfword;label 30;var x:halfword;
h,t:halfword;l:halfword;begin getsymbol;x:=cursym;
if curcmd<>43 then clearsymbol(x,false);h:=getavail;mem[h].hh.lh:=x;
t:=h;while true do begin getxnext;if cursym=0 then goto 30;
if curcmd<>43 then if curcmd<>42 then if curcmd=65 then{1029:}
begin l:=cursym;getxnext;if curcmd<>66 then begin backinput;cursym:=l;
curcmd:=65;goto 30;end else cursym:=0;end{:1029}else goto 30;
mem[t].hh.rh:=getavail;t:=mem[t].hh.rh;mem[t].hh.lh:=cursym;end;
30:if eqtb[x].lh<>43 then clearsymbol(x,false);
if eqtb[x].rh=0 then newroot(x);scandeclaredvariable:=h;end;{:1028}
procedure scandef;var m:1..2;n:0..3;k:0..150;c:0..7;r:halfword;
q:halfword;p:halfword;base:halfword;ldelim,rdelim:halfword;
begin m:=curmod;c:=0;mem[memtop-2].hh.rh:=0;q:=getavail;mem[q].hh.lh:=0;
r:=0;{672:}if m=1 then begin getclearsymbol;warninginfo:=cursym;
begin getnext;if curcmd<=3 then tnext;end;scannerstatus:=5;n:=0;
eqtb[warninginfo].lh:=13;eqtb[warninginfo].rh:=q;
end else begin p:=scandeclaredvariable;
flushvariable(eqtb[mem[p].hh.lh].rh,mem[p].hh.rh,true);
warninginfo:=findvariable(p);flushlist(p);if warninginfo=0 then{673:}
begin begin if interaction=3 then;printnl(262);print(722);end;
begin helpptr:=2;helpline[1]:=723;helpline[0]:=724;end;error;
warninginfo:=22;end{:673};scannerstatus:=4;n:=2;
if curcmd=63 then if curmod=3 then begin n:=3;begin getnext;
if curcmd<=3 then tnext;end;end;mem[warninginfo].hh.b0:=20+n;
mem[warninginfo+1].int:=q;end{:672};k:=n;if curcmd=33 then{675:}
repeat ldelim:=cursym;rdelim:=curmod;begin getnext;
if curcmd<=3 then tnext;end;
if(curcmd=58)and(curmod>=9772)then base:=curmod else begin begin if
interaction=3 then;printnl(262);print(725);end;begin helpptr:=1;
helpline[0]:=726;end;backerror;base:=9772;end;{676:}
repeat mem[q].hh.rh:=getavail;q:=mem[q].hh.rh;mem[q].hh.lh:=base+k;
getsymbol;p:=getnode(2);mem[p+1].int:=base+k;mem[p].hh.lh:=cursym;
if k=150 then overflow(727,150);incr(k);mem[p].hh.rh:=r;r:=p;
begin getnext;if curcmd<=3 then tnext;end;until curcmd<>81{:676};
checkdelimiter(ldelim,rdelim);begin getnext;if curcmd<=3 then tnext;end;
until curcmd<>33{:675};if curcmd=58 then{677:}begin p:=getnode(2);
if curmod<9772 then begin c:=curmod;mem[p+1].int:=9772+k;
end else begin mem[p+1].int:=curmod+k;
if curmod=9772 then c:=4 else if curmod=9922 then c:=6 else c:=7;end;
if k=150 then overflow(727,150);incr(k);getsymbol;mem[p].hh.lh:=cursym;
mem[p].hh.rh:=r;r:=p;begin getnext;if curcmd<=3 then tnext;end;
if c=4 then if curcmd=70 then begin c:=5;p:=getnode(2);
if k=150 then overflow(727,150);mem[p+1].int:=9772+k;getsymbol;
mem[p].hh.lh:=cursym;mem[p].hh.rh:=r;r:=p;begin getnext;
if curcmd<=3 then tnext;end;end;end{:677};checkequals;p:=getavail;
mem[p].hh.lh:=c;mem[q].hh.rh:=p;{670:}
if m=1 then mem[p].hh.rh:=scantoks(18,r,0,n)else begin q:=getavail;
mem[q].hh.lh:=bgloc;mem[p].hh.rh:=q;p:=getavail;mem[p].hh.lh:=egloc;
mem[q].hh.rh:=scantoks(18,r,p,n);end;
if warninginfo=22 then flushtokenlist(mem[23].int){:670};
scannerstatus:=0;getxnext;end;{:669}{678:}procedure scanprimary;forward;
procedure scansecondary;forward;procedure scantertiary;forward;
procedure scanexpression;forward;procedure scansuffix;forward;{692:}
{694:}procedure printmacroname(a,n:halfword);var p,q:halfword;
begin if n<>0 then print(hash[n].rh)else begin p:=mem[a].hh.lh;
if p=0 then print(hash[mem[mem[mem[a].hh.rh].hh.lh].hh.lh].rh)else begin
q:=p;while mem[q].hh.rh<>0 do q:=mem[q].hh.rh;
mem[q].hh.rh:=mem[mem[a].hh.rh].hh.lh;showtokenlist(p,0,1000,0);
mem[q].hh.rh:=0;end;end;end;{:694}{695:}procedure printarg(q:halfword;
n:integer;b:halfword);
begin if mem[q].hh.rh=1 then printnl(510)else if(b<10072)and(b<>7)then
printnl(511)else printnl(512);printint(n);print(743);
if mem[q].hh.rh=1 then printexp(q,1)else showtokenlist(q,0,1000,0);end;
{:695}{702:}procedure scantextarg(ldelim,rdelim:halfword);label 30;
var balance:integer;p:halfword;begin warninginfo:=ldelim;
scannerstatus:=3;p:=memtop-2;balance:=1;mem[memtop-2].hh.rh:=0;
while true do begin begin getnext;if curcmd<=3 then tnext;end;
if ldelim=0 then{704:}
begin if curcmd>81 then begin if balance=1 then goto 30 else if curcmd=
83 then decr(balance);end else if curcmd=34 then incr(balance);end{:704}
else{703:}
begin if curcmd=64 then begin if curmod=ldelim then begin decr(balance);
if balance=0 then goto 30;end;
end else if curcmd=33 then if curmod=rdelim then incr(balance);end{:703}
;mem[p].hh.rh:=curtok;p:=mem[p].hh.rh;end;
30:curexp:=mem[memtop-2].hh.rh;curtype:=20;scannerstatus:=0;end;{:702}
procedure macrocall(defref,arglist,macroname:halfword);label 40;
var r:halfword;p,q:halfword;n:integer;ldelim,rdelim:halfword;
tail:halfword;begin r:=mem[defref].hh.rh;incr(mem[defref].hh.lh);
if arglist=0 then n:=0 else{696:}begin n:=1;tail:=arglist;
while mem[tail].hh.rh<>0 do begin incr(n);tail:=mem[tail].hh.rh;end;
end{:696};if internal[8]>0 then{693:}begin begindiagnostic;println;
printmacroname(arglist,macroname);if n=3 then print(705);
showmacro(defref,0,100000);if arglist<>0 then begin n:=0;p:=arglist;
repeat q:=mem[p].hh.lh;printarg(q,n,0);incr(n);p:=mem[p].hh.rh;
until p=0;end;enddiagnostic(false);end{:693};{697:}curcmd:=82;
while mem[r].hh.lh>=9772 do begin{698:}
if curcmd<>81 then begin getxnext;
if curcmd<>33 then begin begin if interaction=3 then;printnl(262);
print(749);end;printmacroname(arglist,macroname);begin helpptr:=3;
helpline[2]:=750;helpline[1]:=751;helpline[0]:=752;end;
if mem[r].hh.lh>=9922 then begin curexp:=0;curtype:=20;
end else begin curexp:=0;curtype:=16;end;backerror;curcmd:=64;goto 40;
end;ldelim:=cursym;rdelim:=curmod;end;{701:}
if mem[r].hh.lh>=10072 then scantextarg(ldelim,rdelim)else begin
getxnext;if mem[r].hh.lh>=9922 then scansuffix else scanexpression;
end{:701};if curcmd<>81 then{699:}
if(curcmd<>64)or(curmod<>ldelim)then if mem[mem[r].hh.rh].hh.lh>=9772
then begin missingerr(44);begin helpptr:=3;helpline[2]:=753;
helpline[1]:=754;helpline[0]:=748;end;backerror;curcmd:=81;
end else begin missingerr(hash[rdelim].rh);begin helpptr:=2;
helpline[1]:=755;helpline[0]:=748;end;backerror;end{:699};40:{700:}
begin p:=getavail;
if curtype=20 then mem[p].hh.lh:=curexp else mem[p].hh.lh:=stashcurexp;
if internal[8]>0 then begin begindiagnostic;
printarg(mem[p].hh.lh,n,mem[r].hh.lh);enddiagnostic(false);end;
if arglist=0 then arglist:=p else mem[tail].hh.rh:=p;tail:=p;incr(n);
end{:700}{:698};r:=mem[r].hh.rh;end;
if curcmd=81 then begin begin if interaction=3 then;printnl(262);
print(744);end;printmacroname(arglist,macroname);printchar(59);
printnl(745);print(hash[rdelim].rh);print(299);begin helpptr:=3;
helpline[2]:=746;helpline[1]:=747;helpline[0]:=748;end;error;end;
if mem[r].hh.lh<>0 then{705:}
begin if mem[r].hh.lh<7 then begin getxnext;
if mem[r].hh.lh<>6 then if(curcmd=53)or(curcmd=76)then getxnext;end;
case mem[r].hh.lh of 1:scanprimary;2:scansecondary;3:scantertiary;
4:scanexpression;5:{706:}begin scanexpression;p:=getavail;
mem[p].hh.lh:=stashcurexp;if internal[8]>0 then begin begindiagnostic;
printarg(mem[p].hh.lh,n,0);enddiagnostic(false);end;
if arglist=0 then arglist:=p else mem[tail].hh.rh:=p;tail:=p;incr(n);
if curcmd<>70 then begin missingerr(489);print(756);
printmacroname(arglist,macroname);begin helpptr:=1;helpline[0]:=757;end;
backerror;end;getxnext;scanprimary;end{:706};6:{707:}
begin if curcmd<>33 then ldelim:=0 else begin ldelim:=cursym;
rdelim:=curmod;getxnext;end;scansuffix;
if ldelim<>0 then begin if(curcmd<>64)or(curmod<>ldelim)then begin
missingerr(hash[rdelim].rh);begin helpptr:=2;helpline[1]:=755;
helpline[0]:=748;end;backerror;end;getxnext;end;end{:707};
7:scantextarg(0,0);end;backinput;{700:}begin p:=getavail;
if curtype=20 then mem[p].hh.lh:=curexp else mem[p].hh.lh:=stashcurexp;
if internal[8]>0 then begin begindiagnostic;
printarg(mem[p].hh.lh,n,mem[r].hh.lh);enddiagnostic(false);end;
if arglist=0 then arglist:=p else mem[tail].hh.rh:=p;tail:=p;incr(n);
end{:700};end{:705};r:=mem[r].hh.rh{:697};{708:}
while(curinput.indexfield>15)and(curinput.locfield=0)do endtokenlist;
if paramptr+n>maxparamstack then begin maxparamstack:=paramptr+n;
if maxparamstack>150 then overflow(727,150);end;
begintokenlist(defref,21);curinput.namefield:=macroname;
curinput.locfield:=r;if n>0 then begin p:=arglist;
repeat paramstack[paramptr]:=mem[p].hh.lh;incr(paramptr);
p:=mem[p].hh.rh;until p=0;flushlist(arglist);end{:708};end;{:692}
procedure getboolean;forward;procedure passtext;forward;
procedure conditional;forward;procedure startinput;forward;
procedure beginiteration;forward;procedure resumeiteration;forward;
procedure stopiteration;forward;{:678}{679:}procedure expand;
var p:halfword;k:integer;j:poolpointer;
begin if internal[6]>65536 then if curcmd<>13 then showcmdmod(curcmd,
curmod);case curcmd of 4:conditional;5:{723:}
if curmod>iflimit then if iflimit=1 then begin missingerr(58);backinput;
cursym:=9762;inserror;end else begin begin if interaction=3 then;
printnl(262);print(764);end;printcmdmod(5,curmod);begin helpptr:=1;
helpline[0]:=765;end;error;end else begin while curmod<>2 do passtext;
{717:}begin p:=condptr;ifline:=mem[p+1].int;curif:=mem[p].hh.b1;
iflimit:=mem[p].hh.b0;condptr:=mem[p].hh.rh;freenode(p,2);end{:717};
end{:723};6:{683:}if curmod>0 then forceeof:=true else startinput{:683};
7:if curmod=0 then{680:}begin begin if interaction=3 then;printnl(262);
print(728);end;begin helpptr:=2;helpline[1]:=729;helpline[0]:=730;end;
error;end{:680}else beginiteration;8:{684:}
begin while(curinput.indexfield>15)and(curinput.locfield=0)do
endtokenlist;if loopptr=0 then begin begin if interaction=3 then;
printnl(262);print(732);end;begin helpptr:=2;helpline[1]:=733;
helpline[0]:=734;end;error;end else resumeiteration;end{:684};9:{685:}
begin getboolean;if internal[6]>65536 then showcmdmod(35,curexp);
if curexp=30 then if loopptr=0 then begin begin if interaction=3 then;
printnl(262);print(735);end;begin helpptr:=1;helpline[0]:=736;end;
if curcmd=82 then error else backerror;end else{686:}begin p:=0;
repeat if(curinput.indexfield<=15)then endfilereading else begin if
curinput.indexfield<=17 then p:=curinput.startfield;endtokenlist;end;
until p<>0;if p<>mem[loopptr].hh.lh then fatalerror(739);stopiteration;
end{:686}else if curcmd<>82 then begin missingerr(59);begin helpptr:=2;
helpline[1]:=737;helpline[0]:=738;end;backerror;end;end{:685};10:;
12:{687:}begin begin getnext;if curcmd<=3 then tnext;end;p:=curtok;
begin getnext;if curcmd<=3 then tnext;end;
if curcmd<14 then expand else backinput;begintokenlist(p,19);end{:687};
11:{688:}begin getxnext;scanprimary;
if curtype<>4 then begin disperr(0,740);begin helpptr:=2;
helpline[1]:=741;helpline[0]:=742;end;putgetflusherror(0);
end else begin backinput;
if(strstart[nextstr[curexp]]-strstart[curexp])>0 then{689:}
begin beginfilereading;curinput.namefield:=2;
k:=first+(strstart[nextstr[curexp]]-strstart[curexp]);
if k>=maxbufstack then begin if k>=bufsize then begin maxbufstack:=
bufsize;overflow(256,bufsize);end;maxbufstack:=k+1;end;
j:=strstart[curexp];curinput.limitfield:=k;
while first<curinput.limitfield do begin buffer[first]:=strpool[j];
incr(j);incr(first);end;buffer[curinput.limitfield]:=37;
first:=curinput.limitfield+1;curinput.locfield:=curinput.startfield;
flushcurexp(0);end{:689};end;end{:688};13:macrocall(curmod,0,cursym);
end;end;{:679}{690:}procedure getxnext;var saveexp:halfword;
begin begin getnext;if curcmd<=3 then tnext;end;
if curcmd<14 then begin saveexp:=stashcurexp;
repeat if curcmd=13 then macrocall(curmod,0,cursym)else expand;
begin getnext;if curcmd<=3 then tnext;end;until curcmd>=14;
unstashcurexp(saveexp);end;end;{:690}{709:}
procedure stackargument(p:halfword);
begin if paramptr=maxparamstack then begin incr(maxparamstack);
if maxparamstack>150 then overflow(727,150);end;paramstack[paramptr]:=p;
incr(paramptr);end;{:709}{714:}procedure passtext;label 30;
var l:integer;begin scannerstatus:=1;l:=0;warninginfo:=trueline;
while true do begin begin getnext;if curcmd<=3 then tnext;end;
if curcmd<=5 then if curcmd<5 then incr(l)else begin if l=0 then goto 30
;if curmod=2 then decr(l);end else{715:}
if curcmd=41 then begin if strref[curmod]<127 then if strref[curmod]>1
then decr(strref[curmod])else flushstring(curmod);end{:715};end;
30:scannerstatus:=0;end;{:714}{718:}
procedure changeiflimit(l:smallnumber;p:halfword);label 10;
var q:halfword;begin if p=condptr then iflimit:=l else begin q:=condptr;
while true do begin if q=0 then confusion(758);
if mem[q].hh.rh=p then begin mem[q].hh.b0:=l;goto 10;end;
q:=mem[q].hh.rh;end;end;10:end;{:718}{719:}procedure checkcolon;
begin if curcmd<>80 then begin missingerr(58);begin helpptr:=2;
helpline[1]:=761;helpline[0]:=738;end;backerror;end;end;{:719}{720:}
procedure conditional;label 10,30,21,40;var savecondptr:halfword;
newiflimit:2..4;p:halfword;begin{716:}begin p:=getnode(2);
mem[p].hh.rh:=condptr;mem[p].hh.b0:=iflimit;mem[p].hh.b1:=curif;
mem[p+1].int:=ifline;condptr:=p;iflimit:=1;ifline:=trueline;curif:=1;
end{:716};savecondptr:=condptr;21:getboolean;newiflimit:=4;
if internal[6]>65536 then{722:}begin begindiagnostic;
if curexp=30 then print(762)else print(763);enddiagnostic(false);
end{:722};40:checkcolon;
if curexp=30 then begin changeiflimit(newiflimit,savecondptr);goto 10;
end;{721:}while true do begin passtext;
if condptr=savecondptr then goto 30 else if curmod=2 then{717:}
begin p:=condptr;ifline:=mem[p+1].int;curif:=mem[p].hh.b1;
iflimit:=mem[p].hh.b0;condptr:=mem[p].hh.rh;freenode(p,2);end{:717};
end{:721};30:curif:=curmod;ifline:=trueline;if curmod=2 then{717:}
begin p:=condptr;ifline:=mem[p+1].int;curif:=mem[p].hh.b1;
iflimit:=mem[p].hh.b0;condptr:=mem[p].hh.rh;freenode(p,2);end{:717}
else if curmod=4 then goto 21 else begin curexp:=30;newiflimit:=2;
getxnext;goto 40;end;10:end;{:720}{726:}procedure badfor(s:strnumber);
begin disperr(0,766);print(s);print(306);begin helpptr:=4;
helpline[3]:=767;helpline[2]:=768;helpline[1]:=769;helpline[0]:=308;end;
putgetflusherror(0);end;{:726}{727:}procedure beginiteration;
label 22,30;var m:halfword;n:halfword;s:halfword;p:halfword;q:halfword;
pp:halfword;begin m:=curmod;n:=cursym;s:=getnode(2);
if m=1 then begin mem[s+1].hh.lh:=1;p:=0;getxnext;
end else begin getsymbol;p:=getnode(2);mem[p].hh.lh:=cursym;
mem[p+1].int:=m;getxnext;if curcmd=74 then{740:}begin getxnext;
scanexpression;{741:}if curtype<>10 then begin disperr(0,782);
begin helpptr:=1;helpline[0]:=783;end;putgetflusherror(getnode(8));
initedges(curexp);curtype:=10;end{:741};mem[s+1].hh.lh:=curexp;
curtype:=1;q:=mem[curexp+7].hh.rh;
if q<>0 then if(mem[q].hh.b0>=4)then if skip1component(q)=0 then q:=mem[
q].hh.rh;mem[s+1].hh.rh:=q;end{:740}else begin{728:}
if(curcmd<>53)and(curcmd<>76)then begin missingerr(61);begin helpptr:=3;
helpline[2]:=770;helpline[1]:=713;helpline[0]:=771;end;backerror;
end{:728};{738:}mem[s+1].hh.lh:=0;q:=s+1;mem[q].hh.rh:=0;
repeat getxnext;
if m<>9772 then scansuffix else begin if curcmd>=80 then if curcmd<=81
then goto 22;scanexpression;if curcmd=72 then if q=s+1 then{739:}
begin if curtype<>16 then badfor(777);pp:=getnode(4);
mem[pp+1].int:=curexp;getxnext;scanexpression;
if curtype<>16 then badfor(778);mem[pp+2].int:=curexp;
if curcmd<>73 then begin missingerr(500);begin helpptr:=2;
helpline[1]:=779;helpline[0]:=780;end;backerror;end;getxnext;
scanexpression;if curtype<>16 then badfor(781);mem[pp+3].int:=curexp;
mem[s+1].hh.rh:=pp;mem[s+1].hh.lh:=2;goto 30;end{:739};
curexp:=stashcurexp;end;mem[q].hh.rh:=getavail;q:=mem[q].hh.rh;
mem[q].hh.lh:=curexp;curtype:=1;22:until curcmd<>81;30:{:738};end;end;
{729:}if curcmd<>80 then begin missingerr(58);begin helpptr:=3;
helpline[2]:=772;helpline[1]:=773;helpline[0]:=774;end;backerror;
end{:729};{731:}q:=getavail;mem[q].hh.lh:=9758;scannerstatus:=6;
warninginfo:=n;mem[s].hh.lh:=scantoks(7,p,q,0);scannerstatus:=0;
mem[s].hh.rh:=loopptr;loopptr:=s{:731};resumeiteration;end;{:727}{733:}
procedure resumeiteration;label 45,10;var p,q:halfword;
begin p:=mem[loopptr+1].hh.lh;if p=2 then begin p:=mem[loopptr+1].hh.rh;
curexp:=mem[p+1].int;if{734:}
((mem[p+2].int>0)and(curexp>mem[p+3].int))or((mem[p+2].int<0)and(curexp<
mem[p+3].int)){:734}then goto 45;curtype:=16;q:=stashcurexp;
mem[p+1].int:=curexp+mem[p+2].int;
end else if p=0 then begin p:=mem[loopptr+1].hh.rh;if p=0 then goto 45;
mem[loopptr+1].hh.rh:=mem[p].hh.rh;q:=mem[p].hh.lh;
begin mem[p].hh.rh:=avail;avail:=p;ifdef('STAT')decr(dynused);
endif('STAT')end;
end else if p=1 then begin begintokenlist(mem[loopptr].hh.lh,16);
goto 10;end else{736:}begin q:=mem[loopptr+1].hh.rh;if q=0 then goto 45;
if not(mem[q].hh.b0>=4)then q:=mem[q].hh.rh else if not(mem[q].hh.b0>=6)
then q:=skip1component(q)else goto 45;
curexp:=copyobjects(mem[loopptr+1].hh.rh,q);initbbox(curexp);
curtype:=10;mem[loopptr+1].hh.rh:=q;q:=stashcurexp;end{:736};
begintokenlist(mem[loopptr].hh.lh,17);stackargument(q);
if internal[6]>65536 then{735:}begin begindiagnostic;printnl(776);
if(q<>0)and(mem[q].hh.rh=1)then printexp(q,1)else showtokenlist(q,0,50,0
);printchar(125);enddiagnostic(false);end{:735};goto 10;
45:stopiteration;10:end;{:733}{737:}procedure stopiteration;
var p,q:halfword;begin p:=mem[loopptr+1].hh.lh;
if p=2 then freenode(mem[loopptr+1].hh.rh,4)else if p=0 then begin q:=
mem[loopptr+1].hh.rh;while q<>0 do begin p:=mem[q].hh.lh;
if p<>0 then if mem[p].hh.rh=1 then begin recyclevalue(p);freenode(p,2);
end else flushtokenlist(p);p:=q;q:=mem[q].hh.rh;
begin mem[p].hh.rh:=avail;avail:=p;ifdef('STAT')decr(dynused);
endif('STAT')end;end;
end else if p>2 then if mem[p].hh.lh=0 then tossedges(p)else decr(mem[p]
.hh.lh);p:=loopptr;loopptr:=mem[p].hh.rh;flushtokenlist(mem[p].hh.lh);
freenode(p,2);end;{:737}{755:}procedure packbufferedname(n:smallnumber;
a,b:integer);var k:integer;c:ASCIIcode;j:integer;
begin if n+b-a+5>maxint then b:=a+maxint-n-5;k:=0;
if nameoffile then libcfree(nameoffile);
nameoffile:=xmalloc(1+n+(b-a+1)+5);
for j:=1 to n do begin c:=xord[MPmemdefault[j]];incr(k);
if k<=maxint then nameoffile[k]:=xchr[c];end;
for j:=a to b do begin c:=buffer[j];incr(k);
if k<=maxint then nameoffile[k]:=xchr[c];end;
for j:=memdefaultlength-3 to memdefaultlength do begin c:=xord[
MPmemdefault[j]];incr(k);if k<=maxint then nameoffile[k]:=xchr[c];end;
if k<=maxint then namelength:=k else namelength:=maxint;
nameoffile[namelength+1]:=0;end;{:755}{757:}
function makenamestring:strnumber;var k:1..maxint;
begin if stroverflowed then makenamestring:=63 else begin begin if
poolptr+namelength>maxpoolptr then if poolptr+namelength>poolsize then
docompaction(namelength)else maxpoolptr:=poolptr+namelength;end;
for k:=1 to namelength do begin strpool[poolptr]:=xord[nameoffile[k]];
incr(poolptr);end;makenamestring:=makestring;end;end;
function amakenamestring(var f:alphafile):strnumber;
begin amakenamestring:=makenamestring;end;
function bmakenamestring(var f:bytefile):strnumber;
begin bmakenamestring:=makenamestring;end;
function wmakenamestring(var f:wordfile):strnumber;
begin wmakenamestring:=makenamestring;end;{:757}{758:}
procedure scanfilename;label 30;begin beginname;
while(buffer[curinput.locfield]=32)or(buffer[curinput.locfield]=9)do
incr(curinput.locfield);
while true do begin if(buffer[curinput.locfield]=59)or(buffer[curinput.
locfield]=37)then goto 30;
if not morename(buffer[curinput.locfield])then goto 30;
incr(curinput.locfield);end;30:endname;end;{:758}{762:}
procedure packjobname(s:strnumber);
begin begin if strref[s]<127 then incr(strref[s]);end;
begin if strref[curname]<127 then if strref[curname]>1 then decr(strref[
curname])else flushstring(curname);end;
begin if strref[curarea]<127 then if strref[curarea]>1 then decr(strref[
curarea])else flushstring(curarea);end;
begin if strref[curext]<127 then if strref[curext]>1 then decr(strref[
curext])else flushstring(curext);end;curarea:=284;curext:=s;
curname:=jobname;packfilename(curname,curarea,curext);end;{:762}{763:}
procedure promptfilename(s,e:strnumber);label 30;var k:0..bufsize;
begin if interaction=2 then;if s=785 then begin if interaction=3 then;
printnl(262);print(786);end else begin if interaction=3 then;
printnl(262);print(787);end;printfilename(curname,curarea,curext);
print(788);if e=284 then showcontext;printnl(789);print(s);
if interaction<2 then fatalerror(790);;begin;print(791);terminput;end;
{764:}begin beginname;k:=first;
while((buffer[k]=32)or(buffer[k]=9))and(k<last)do incr(k);
while true do begin if k=last then goto 30;
if not morename(buffer[k])then goto 30;incr(k);end;30:endname;end{:764};
if curext=284 then curext:=e;packfilename(curname,curarea,curext);end;
{:763}{765:}procedure openlogfile;var oldsetting:0..10;k:0..bufsize;
l:0..bufsize;m:integer;months:^char;begin oldsetting:=selector;
if jobname=0 then jobname:=792;packjobname(793);
while not aopenout(logfile)do{766:}begin selector:=8;
promptfilename(795,793);end{:766};
texmflogname:=amakenamestring(logfile);selector:=9;logopened:=true;
{767:}begin write(logfile,'This is MetaPost, Version 0.641');
write(logfile,versionstring);print(memident);print(796);
printint(roundunscaled(internal[15]));printchar(32);
months:=' JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC';
m:=roundunscaled(internal[14]);
for k:=3*m-2 to 3*m do write(logfile,months[k]);printchar(32);
printint(roundunscaled(internal[13]));printchar(32);
m:=roundunscaled(internal[16]);printdd(m div 60);printchar(58);
printdd(m mod 60);end{:767};inputstack[inputptr]:=curinput;printnl(794);
l:=inputstack[0].limitfield-1;for k:=1 to l do print(buffer[k]);println;
selector:=oldsetting+2;end;{:765}{768:}
function tryextension(ext:strnumber):boolean;
begin packfilename(curname,curarea,curext);
inamestack[curinput.indexfield]:=curname;
iareastack[curinput.indexfield]:=curarea;
if strvsstr(ext,797)=0 then tryextension:=aopenin(inputfile[curinput.
indexfield],kpsemfformat)else tryextension:=aopenin(inputfile[curinput.
indexfield],kpsempformat);end;{:768}{770:}procedure startinput;label 30;
var j:integer;begin{773:}
while(curinput.indexfield>15)and(curinput.locfield=0)do endtokenlist;
if(curinput.indexfield>15)then begin begin if interaction=3 then;
printnl(262);print(799);end;begin helpptr:=3;helpline[2]:=800;
helpline[1]:=801;helpline[0]:=802;end;error;end;
if(curinput.indexfield<=15)then scanfilename else begin curname:=284;
curext:=284;curarea:=284;end{:773};while true do begin beginfilereading;
if tryextension(798)then goto 30 else if tryextension(797)then goto 30
else;endfilereading;promptfilename(785,284);end;
30:curinput.namefield:=amakenamestring(inputfile[curinput.indexfield]);
{769:}
if inamestack[curinput.indexfield]=curname then begin if strref[curname]
<127 then incr(strref[curname]);end;
if iareastack[curinput.indexfield]=curarea then begin if strref[curarea]
<127 then incr(strref[curarea]);end{:769};if jobname=0 then begin j:=1;
beginname;while(j<=namelength)and(morename(nameoffile[j]))do incr(j);
endname;jobname:=curname;strref[jobname]:=127;
ifdef('INIMP')if iniversion and dumpoption then begin begin if poolptr+
memdefaultlength>maxpoolptr then if poolptr+memdefaultlength>poolsize
then docompaction(memdefaultlength)else maxpoolptr:=poolptr+
memdefaultlength;end;
for j:=1 to memdefaultlength-4 do begin strpool[poolptr]:=xord[
MPmemdefault[j]];incr(poolptr);end;jobname:=makestring;
strref[jobname]:=127;end;endif('INIMP')openlogfile;end;
if termoffset+(strstart[nextstr[curinput.namefield]]-strstart[curinput.
namefield])>maxprintline-2 then println else if(termoffset>0)or(
fileoffset>0)then printchar(32);printchar(40);incr(openparens);
print(curinput.namefield);fflush(stdout);{771:}{:771};{772:}
begin linestack[curinput.indexfield]:=1;
if inputln(inputfile[curinput.indexfield],false)then;firmuptheline;
buffer[curinput.limitfield]:=37;first:=curinput.limitfield+1;
curinput.locfield:=curinput.startfield;end{:772};end;{:770}{774:}
procedure copyoldname(s:strnumber);var k:integer;j:poolpointer;
begin k:=0;if oldfilename then libcfree(oldfilename);
oldfilename:=xmalloc(1+(strstart[nextstr[s]]-strstart[s])+1);
for j:=strstart[s]to strstart[nextstr[s]]-1 do begin incr(k);
if k<=maxint then oldfilename[k]:=xchr[strpool[j]];end;
if k<=maxint then oldnamelength:=k else oldnamelength:=maxint;
oldfilename[oldnamelength+1]:=0;end;{:774}{776:}procedure startmpxinput;
label 10,45;var k:1..maxint;
begin packfilename(inamestack[curinput.indexfield],iareastack[curinput.
indexfield],803);{777:}copyoldname(curinput.namefield);
if not callmakempx(oldfilename+1,nameoffile+1)then goto 45{:777};
beginfilereading;
if not aopenin(inputfile[curinput.indexfield],-1)then begin
endfilereading;goto 45;end;
curinput.namefield:=amakenamestring(inputfile[curinput.indexfield]);
mpxname[curinput.indexfield]:=curinput.namefield;
begin if strref[curinput.namefield]<127 then incr(strref[curinput.
namefield]);end;{772:}begin linestack[curinput.indexfield]:=1;
if inputln(inputfile[curinput.indexfield],false)then;firmuptheline;
buffer[curinput.limitfield]:=37;first:=curinput.limitfield+1;
curinput.locfield:=curinput.startfield;end{:772};goto 10;45:{778:}
if interaction=3 then;printnl(804);
for k:=1 to oldnamelength do print(xord[oldfilename[k]]);printnl(804);
for k:=1 to namelength do print(xord[nameoffile[k]]);printnl(805);
begin helpptr:=4;helpline[3]:=806;helpline[2]:=807;helpline[1]:=808;
helpline[0]:=809;end;begin if interaction=3 then interaction:=2;
if logopened then error;
ifdef('TEXMF_DEBUG')if interaction>0 then debughelp;
endif('TEXMF_DEBUG')history:=3;jumpout;end;{:778};10:end;{:776}{782:}
function startreadinput(s:strnumber;n:readfindex):boolean;label 10,45;
begin strscanfile(s);packfilename(curname,curarea,curext);
beginfilereading;if not aopenin(rdfile[n],-1)then goto 45;
if not inputln(rdfile[n],false)then begin aclose(rdfile[n]);goto 45;end;
rdfname[n]:=s;begin if strref[s]<127 then incr(strref[s]);end;
startreadinput:=true;goto 10;45:endfilereading;startreadinput:=false;
10:end;{:782}{783:}procedure openwritefile(s:strnumber;n:readfindex);
ifdef('AMIGA')label 30;var n0:readfindex;
endif('AMIGA')begin strscanfile(s);packfilename(curname,curarea,curext);
ifdef('AMIGA')for n0:=0 to readfiles-1 do begin if rdfname[n0]<>0 then
begin if strvsstr(s,rdfname[n0])=0 then begin aclose(rdfile[n0]);
begin if strref[rdfname[n0]]<127 then if strref[rdfname[n0]]>1 then decr
(strref[rdfname[n0]])else flushstring(rdfname[n0]);end;rdfname[n0]:=0;
if n0=readfiles-1 then readfiles:=n0;goto 30;end;end;end;30:;
endif('AMIGA')while not openoutnameok(nameoffile+1)or not aopenout(
wrfile[n])do promptfilename(810,284);wrfname[n]:=s;
begin if strref[s]<127 then incr(strref[s]);end;
if logopened then begin oldsetting:=selector;
if(internal[12]<=0)then selector:=9 else selector:=10;printnl(502);
printint(n);print(811);printfilename(curname,curarea,curext);print(788);
printnl(284);println;selector:=oldsetting;end;end;{:783}{812:}
procedure badexp(s:strnumber);var saveflag:0..84;
begin begin if interaction=3 then;printnl(262);print(s);end;print(819);
printcmdmod(curcmd,curmod);printchar(39);begin helpptr:=4;
helpline[3]:=820;helpline[2]:=821;helpline[1]:=822;helpline[0]:=823;end;
backinput;cursym:=0;curcmd:=44;curmod:=0;inserror;saveflag:=varflag;
varflag:=0;getxnext;varflag:=saveflag;end;{:812}{815:}
procedure stashin(p:halfword);var q:halfword;
begin mem[p].hh.b0:=curtype;
if curtype=16 then mem[p+1].int:=curexp else begin if curtype=19 then{
817:}begin q:=singledependency(curexp);
if q=depfinal then begin mem[p].hh.b0:=16;mem[p+1].int:=0;freenode(q,2);
end else begin mem[p].hh.b0:=17;newdep(p,q);end;recyclevalue(curexp);
end{:817}else begin mem[p+1]:=mem[curexp+1];
mem[mem[p+1].hh.lh].hh.rh:=p;end;freenode(curexp,2);end;curtype:=1;end;
{:815}{838:}procedure backexpr;var p:halfword;begin p:=stashcurexp;
mem[p].hh.rh:=0;begintokenlist(p,19);end;{:838}{839:}
procedure badsubscript;begin disperr(0,837);begin helpptr:=3;
helpline[2]:=838;helpline[1]:=839;helpline[0]:=840;end;flusherror(0);
end;{:839}{841:}procedure obliterated(q:halfword);
begin begin if interaction=3 then;printnl(262);print(841);end;
showtokenlist(q,0,1000,0);print(842);begin helpptr:=5;helpline[4]:=843;
helpline[3]:=844;helpline[2]:=845;helpline[1]:=846;helpline[0]:=847;end;
end;{:841}{853:}procedure binarymac(p,c,n:halfword);var q,r:halfword;
begin q:=getavail;r:=getavail;mem[q].hh.rh:=r;mem[q].hh.lh:=p;
mem[r].hh.lh:=stashcurexp;macrocall(c,q,n);end;{:853}{858:}{859:}
procedure knownpair;var p:halfword;
begin if curtype<>14 then begin disperr(0,858);begin helpptr:=5;
helpline[4]:=859;helpline[3]:=860;helpline[2]:=861;helpline[1]:=862;
helpline[0]:=863;end;putgetflusherror(0);curx:=0;cury:=0;
end else begin p:=mem[curexp+1].int;{860:}
if mem[p].hh.b0=16 then curx:=mem[p+1].int else begin disperr(p,864);
begin helpptr:=5;helpline[4]:=865;helpline[3]:=860;helpline[2]:=861;
helpline[1]:=862;helpline[0]:=863;end;putgeterror;recyclevalue(p);
curx:=0;end;
if mem[p+2].hh.b0=16 then cury:=mem[p+3].int else begin disperr(p+2,866)
;begin helpptr:=5;helpline[4]:=867;helpline[3]:=860;helpline[2]:=861;
helpline[1]:=862;helpline[0]:=863;end;putgeterror;recyclevalue(p+2);
cury:=0;end{:860};flushcurexp(0);end;end;{:859}
function newknot:halfword;var q:halfword;begin q:=getnode(7);
mem[q].hh.b0:=0;mem[q].hh.b1:=0;mem[q].hh.rh:=q;knownpair;
mem[q+1].int:=curx;mem[q+2].int:=cury;newknot:=q;end;{:858}{862:}
function scandirection:smallnumber;var t:2..4;x:scaled;begin getxnext;
if curcmd=62 then{863:}begin getxnext;scanexpression;
if(curtype<>16)or(curexp<0)then begin disperr(0,870);begin helpptr:=1;
helpline[0]:=871;end;putgetflusherror(65536);end;t:=3;end{:863}
else{864:}begin scanexpression;if curtype>14 then{865:}
begin if curtype<>16 then begin disperr(0,864);begin helpptr:=5;
helpline[4]:=865;helpline[3]:=860;helpline[2]:=861;helpline[1]:=862;
helpline[0]:=863;end;putgetflusherror(0);end;x:=curexp;
if curcmd<>81 then begin missingerr(44);begin helpptr:=2;
helpline[1]:=872;helpline[0]:=873;end;backerror;end;getxnext;
scanexpression;if curtype<>16 then begin disperr(0,866);
begin helpptr:=5;helpline[4]:=867;helpline[3]:=860;helpline[2]:=861;
helpline[1]:=862;helpline[0]:=863;end;putgetflusherror(0);end;
cury:=curexp;curx:=x;end{:865}else knownpair;
if(curx=0)and(cury=0)then t:=4 else begin t:=2;curexp:=narg(curx,cury);
end;end{:864};if curcmd<>67 then begin missingerr(125);begin helpptr:=3;
helpline[2]:=868;helpline[1]:=869;helpline[0]:=738;end;backerror;end;
getxnext;scandirection:=t;end;{:862}{882:}{884:}procedure finishread;
var k:poolpointer;
begin begin if poolptr+last-curinput.startfield>maxpoolptr then if
poolptr+last-curinput.startfield>poolsize then docompaction(last-
curinput.startfield)else maxpoolptr:=poolptr+last-curinput.startfield;
end;
for k:=curinput.startfield to last-1 do begin strpool[poolptr]:=buffer[k
];incr(poolptr);end;endfilereading;curtype:=4;curexp:=makestring;end;
{:884}procedure donullary(c:quarterword);
begin begin if aritherror then cleararith;end;
if internal[6]>131072 then showcmdmod(35,c);
case c of 30,31:begin curtype:=2;curexp:=c;end;32:begin curtype:=10;
curexp:=getnode(8);initedges(curexp);end;33:begin curtype:=6;
curexp:=getpencircle(0);end;37:begin curtype:=16;curexp:=normrand;end;
36:begin curtype:=6;curexp:=getpencircle(65536);end;
34:begin if jobname=0 then openlogfile;curtype:=4;curexp:=jobname;end;
35:{883:}begin if interaction<=1 then fatalerror(884);beginfilereading;
curinput.namefield:=1;curinput.limitfield:=curinput.startfield;begin;
print(284);terminput;end;finishread;end{:883};end;
begin if aritherror then cleararith;end;end;{:882}{885:}{886:}
function nicepair(p:integer;t:quarterword):boolean;label 10;
begin if t=14 then begin p:=mem[p+1].int;
if mem[p].hh.b0=16 then if mem[p+2].hh.b0=16 then begin nicepair:=true;
goto 10;end;end;nicepair:=false;10:end;{:886}{887:}
function nicecolororpair(p:integer;t:quarterword):boolean;label 10;
var q,r:halfword;
begin if(t<>14)and(t<>13)then nicecolororpair:=false else begin q:=mem[p
+1].int;r:=q+bignodesize[mem[p].hh.b0];repeat r:=r-2;
if mem[r].hh.b0<>16 then begin nicecolororpair:=false;goto 10;end;
until r=q;nicecolororpair:=true;end;10:end;{:887}{888:}
procedure printknownorunknowntype(t:smallnumber;v:integer);
begin printchar(40);
if t>16 then print(885)else begin if(t=14)or(t=13)then if not
nicecolororpair(v,t)then print(886);printtype(t);end;printchar(41);end;
{:888}{889:}procedure badunary(c:quarterword);begin disperr(0,887);
printop(c);printknownorunknowntype(curtype,curexp);begin helpptr:=3;
helpline[2]:=888;helpline[1]:=889;helpline[0]:=890;end;putgeterror;end;
{:889}{892:}procedure negatedeplist(p:halfword);label 10;
begin while true do begin mem[p+1].int:=-mem[p+1].int;
if mem[p].hh.lh=0 then goto 10;p:=mem[p].hh.rh;end;10:end;{:892}{896:}
procedure pairtopath;begin curexp:=newknot;curtype:=8;end;{:896}{898:}
procedure takepart(c:quarterword);var p:halfword;
begin p:=mem[curexp+1].int;mem[10].int:=p;mem[9].hh.b0:=curtype;
mem[p].hh.rh:=9;freenode(curexp,2);makeexpcopy(p+sectoroffset[c-49]);
recyclevalue(9);end;{:898}{901:}procedure scaleedges;forward;
procedure takepictpart(c:quarterword);label 10,45;var p:halfword;
begin p:=mem[curexp+7].hh.rh;
if p<>0 then begin case c of 54,55,56,57,58,59:if mem[p].hh.b0=3 then
flushcurexp(mem[p+c-46].int)else goto 45;
60,61,62:if(mem[p].hh.b0<4)then flushcurexp(mem[p+c-58].int)else goto 45
;{902:}
64:if mem[p].hh.b0<>3 then goto 45 else begin flushcurexp(mem[p+1].hh.rh
);begin if strref[curexp]<127 then incr(strref[curexp]);end;curtype:=4;
end;
63:if mem[p].hh.b0<>3 then goto 45 else begin flushcurexp(fontname[mem[p
+1].hh.lh]);begin if strref[curexp]<127 then incr(strref[curexp]);end;
curtype:=4;end;
65:if mem[p].hh.b0=3 then goto 45 else if(mem[p].hh.b0>=6)then confusion
(892)else begin flushcurexp(copypath(mem[p+1].hh.rh));curtype:=8;end;
66:if not(mem[p].hh.b0<3)then goto 45 else if mem[p+1].hh.lh=0 then goto
45 else begin flushcurexp(makepen(copypath(mem[p+1].hh.lh),false));
curtype:=6;end;
67:if mem[p].hh.b0<>2 then goto 45 else if mem[p+6].hh.rh=0 then goto 45
else begin incr(mem[mem[p+6].hh.rh].hh.lh);sesf:=mem[p+7].int;
sepic:=mem[p+6].hh.rh;scaleedges;flushcurexp(sepic);curtype:=10;end;
{:902}end;goto 10;end;45:{904:}case c of 64,63:begin flushcurexp(284);
curtype:=4;end;65:begin flushcurexp(getnode(7));mem[curexp].hh.b0:=0;
mem[curexp].hh.b1:=0;mem[curexp].hh.rh:=curexp;mem[curexp+1].int:=0;
mem[curexp+2].int:=0;curtype:=8;end;
66:begin flushcurexp(getpencircle(0));curtype:=6;end;
67:begin flushcurexp(getnode(8));initedges(curexp);curtype:=10;end;
others:flushcurexp(0)end{:904};10:end;{:901}{906:}
procedure strtonum(c:quarterword);var n:integer;m:ASCIIcode;
k:poolpointer;b:8..16;badchar:boolean;
begin if c=50 then if(strstart[nextstr[curexp]]-strstart[curexp])=0 then
n:=-1 else n:=strpool[strstart[curexp]]else begin if c=48 then b:=8 else
b:=16;n:=0;badchar:=false;
for k:=strstart[curexp]to strstart[nextstr[curexp]]-1 do begin m:=
strpool[k];
if(m>=48)and(m<=57)then m:=m-48 else if(m>=65)and(m<=70)then m:=m-55
else if(m>=97)and(m<=102)then m:=m-87 else begin badchar:=true;m:=0;end;
if m>=b then begin badchar:=true;m:=0;end;
if n<32768 div b then n:=n*b+m else n:=32767;end;{907:}
if badchar then begin disperr(0,893);if c=48 then begin helpptr:=1;
helpline[0]:=894;end else begin helpptr:=1;helpline[0]:=895;end;
putgeterror;end;
if(n>4095)then if internal[30]>0 then begin begin if interaction=3 then;
printnl(262);print(896);end;printint(n);printchar(41);begin helpptr:=2;
helpline[1]:=897;helpline[0]:=606;end;putgeterror;end{:907};end;
flushcurexp(n*65536);end;{:906}{909:}function pathlength:scaled;
var n:scaled;p:halfword;begin p:=curexp;
if mem[p].hh.b0=0 then n:=-65536 else n:=0;repeat p:=mem[p].hh.rh;
n:=n+65536;until p=curexp;pathlength:=n;end;{:909}{910:}
function pictlength:scaled;label 40;var n:scaled;p:halfword;begin n:=0;
p:=mem[curexp+7].hh.rh;
if p<>0 then begin if(mem[p].hh.b0>=4)then if skip1component(p)=0 then p
:=mem[p].hh.rh;
while p<>0 do begin if not(mem[p].hh.b0>=4)then p:=mem[p].hh.rh else if
not(mem[p].hh.b0>=6)then p:=skip1component(p)else goto 40;n:=n+65536;
end;end;40:pictlength:=n;end;{:910}{912:}
function countturns(c:halfword):scaled;var p:halfword;t:integer;
begin t:=0;p:=c;repeat t:=t+mem[p].hh.lh-16384;p:=mem[p].hh.rh;
until p=c;countturns:=(t div 3)*65536;end;{:912}{914:}
procedure testknown(c:quarterword);label 30;var b:30..31;p,q:halfword;
begin b:=31;case curtype of 1,2,4,6,8,10,16:b:=30;
12,13,14:begin p:=mem[curexp+1].int;q:=p+bignodesize[curtype];
repeat q:=q-2;if mem[q].hh.b0<>16 then goto 30;until q=p;b:=30;30:end;
others:end;if c=41 then flushcurexp(b)else flushcurexp(61-b);curtype:=2;
end;{:914}{919:}procedure pairvalue(x,y:scaled);var p:halfword;
begin p:=getnode(2);flushcurexp(p);curtype:=14;mem[p].hh.b0:=14;
mem[p].hh.b1:=14;initbignode(p);p:=mem[p+1].int;mem[p].hh.b0:=16;
mem[p+1].int:=x;mem[p+2].hh.b0:=16;mem[p+3].int:=y;end;{:919}{921:}
function getcurbbox:boolean;label 10;
begin case curtype of 10:begin setbbox(curexp,true);
if mem[curexp+2].int>mem[curexp+4].int then begin bbmin[0]:=0;
bbmax[0]:=0;bbmin[1]:=0;bbmax[1]:=0;
end else begin bbmin[0]:=mem[curexp+2].int;bbmax[0]:=mem[curexp+4].int;
bbmin[1]:=mem[curexp+3].int;bbmax[1]:=mem[curexp+5].int;end;end;
8:pathbbox(curexp);6:penbbox(curexp);others:begin getcurbbox:=false;
goto 10;end end;getcurbbox:=true;10:end;{:921}{923:}
procedure doreadorclose(c:quarterword);label 10,22,40,45,46;
var n,n0:readfindex;begin{924:}n:=readfiles;n0:=readfiles;
repeat 22:if n>0 then decr(n)else if c=39 then goto 46 else{925:}
begin if n0=readfiles then if readfiles<maxreadfiles then incr(readfiles
)else overflow(899,maxreadfiles);n:=n0;
if startreadinput(curexp,n)then goto 40 else goto 45;end{:925};
if rdfname[n]=0 then begin n0:=n;goto 22;end;
until strvsstr(curexp,rdfname[n])=0;
if c=39 then begin aclose(rdfile[n]);goto 45;end{:924};beginfilereading;
curinput.namefield:=1;if inputln(rdfile[n],true)then goto 40;
endfilereading;45:{926:}
begin if strref[rdfname[n]]<127 then if strref[rdfname[n]]>1 then decr(
strref[rdfname[n]])else flushstring(rdfname[n]);end;rdfname[n]:=0;
if n=readfiles-1 then readfiles:=n;if c=39 then goto 46;{929:}
if eofline=0 then begin begin strpool[poolptr]:=0;incr(poolptr);end;
eofline:=makestring;strref[eofline]:=127;end{:929};flushcurexp(eofline);
curtype:=4{:926};goto 10;46:flushcurexp(0);curtype:=1;goto 10;
40:flushcurexp(0);finishread;10:end;{:923}
procedure dounary(c:quarterword);var p,q,r:halfword;x:integer;
begin begin if aritherror then cleararith;end;
if internal[6]>131072 then{890:}begin begindiagnostic;printnl(123);
printop(c);printchar(40);printexp(0,0);print(891);enddiagnostic(false);
end{:890};case c of 89:if curtype<13 then badunary(89);90:{891:}
case curtype of 13,14,19:begin q:=curexp;makeexpcopy(q);
if curtype=17 then negatedeplist(mem[curexp+1].hh.rh)else if curtype<=14
then begin p:=mem[curexp+1].int;r:=p+bignodesize[curtype];repeat r:=r-2;
if mem[r].hh.b0=16 then mem[r+1].int:=-mem[r+1].int else negatedeplist(
mem[r+1].hh.rh);until r=p;end;recyclevalue(q);freenode(q,2);end;
17,18:negatedeplist(mem[curexp+1].hh.rh);16:curexp:=-curexp;
others:badunary(90)end{:891};{893:}
43:if curtype<>2 then badunary(43)else curexp:=61-curexp;{:893}{894:}
68,69,70,71,72,73,74,40,75:if curtype<>16 then badunary(c)else case c of
68:curexp:=squarert(curexp);69:curexp:=mexp(curexp);
70:curexp:=mlog(curexp);71,72:begin nsincos((curexp mod 23592960)*16);
if c=71 then curexp:=roundfraction(nsin)else curexp:=roundfraction(ncos)
;end;73:curexp:=floorscaled(curexp);74:curexp:=unifrand(curexp);
40:begin if odd(roundunscaled(curexp))then curexp:=30 else curexp:=31;
curtype:=2;end;75:{1276:}begin curexp:=roundunscaled(curexp)mod 256;
if curexp<0 then curexp:=curexp+256;
if charexists[curexp]then curexp:=30 else curexp:=31;curtype:=2;
end{:1276};end;{:894}{895:}
82:if nicepair(curexp,curtype)then begin p:=mem[curexp+1].int;
x:=narg(mem[p+1].int,mem[p+3].int);
if x>=0 then flushcurexp((x+8)div 16)else flushcurexp(-((-x+8)div 16));
end else badunary(82);{:895}{897:}
54,55:if(curtype=14)or(curtype=12)then takepart(c)else if curtype=10
then takepictpart(c)else badunary(c);
56,57,58,59:if curtype=12 then takepart(c)else if curtype=10 then
takepictpart(c)else badunary(c);
60,61,62:if curtype=13 then takepart(c)else if curtype=10 then
takepictpart(c)else badunary(c);{:897}{900:}
63,64,65,66,67:if curtype=10 then takepictpart(c)else badunary(c);{:900}
{905:}
51:if curtype<>16 then badunary(51)else begin curexp:=roundunscaled(
curexp)mod 256;curtype:=4;if curexp<0 then curexp:=curexp+256;end;
44:if curtype<>16 then badunary(44)else begin oldsetting:=selector;
selector:=4;printscaled(curexp);curexp:=makestring;selector:=oldsetting;
curtype:=4;end;48,49,50:if curtype<>4 then badunary(c)else strtonum(c);
76:if curtype<>4 then badunary(76)else{1190:}
flushcurexp((fontdsize[findfont(curexp)]+8)div 16){:1190};{:905}{908:}
52:case curtype of 4:flushcurexp((strstart[nextstr[curexp]]-strstart[
curexp])*65536);8:flushcurexp(pathlength);16:curexp:=abs(curexp);
10:flushcurexp(pictlength);
others:if nicepair(curexp,curtype)then flushcurexp(pythadd(mem[mem[
curexp+1].int+1].int,mem[mem[curexp+1].int+3].int))else badunary(c)end;
{:908}{911:}
53:if curtype=14 then flushcurexp(0)else if curtype<>8 then badunary(53)
else if mem[curexp].hh.b0=0 then flushcurexp(0)else begin curexp:=
offsetprep(curexp,13);
if internal[5]>65536 then printspec(curexp,13,898);
flushcurexp(countturns(curexp));end;{:911}{913:}
2:begin if(curtype>=2)and(curtype<=3)then flushcurexp(30)else
flushcurexp(31);curtype:=2;end;
4:begin if(curtype>=4)and(curtype<=5)then flushcurexp(30)else
flushcurexp(31);curtype:=2;end;
6:begin if(curtype>=6)and(curtype<=7)then flushcurexp(30)else
flushcurexp(31);curtype:=2;end;
8:begin if(curtype>=8)and(curtype<=9)then flushcurexp(30)else
flushcurexp(31);curtype:=2;end;
10:begin if(curtype>=10)and(curtype<=11)then flushcurexp(30)else
flushcurexp(31);curtype:=2;end;
12,13,14:begin if curtype=c then flushcurexp(30)else flushcurexp(31);
curtype:=2;end;
15:begin if(curtype>=16)and(curtype<=19)then flushcurexp(30)else
flushcurexp(31);curtype:=2;end;41,42:testknown(c);{:913}{915:}
83:begin if curtype<>8 then flushcurexp(31)else if mem[curexp].hh.b0<>0
then flushcurexp(30)else flushcurexp(31);curtype:=2;end;{:915}{916:}
81:begin if curtype=14 then pairtopath;
if curtype<>8 then badunary(81)else flushcurexp(getarclength(curexp));
end;{:916}{917:}
84,85,86,87,88:begin if curtype<>10 then flushcurexp(31)else if mem[
curexp+7].hh.rh=0 then flushcurexp(31)else if mem[mem[curexp+7].hh.rh].
hh.b0=c-83 then flushcurexp(30)else flushcurexp(31);curtype:=2;end;
{:917}{918:}47:begin if curtype=14 then pairtopath;
if curtype<>8 then badunary(47)else begin curtype:=6;
curexp:=makepen(curexp,true);end;end;
46:if curtype<>6 then badunary(46)else begin curtype:=8;
makepath(curexp);end;45:if curtype=8 then begin p:=htapypoc(curexp);
if mem[p].hh.b1=0 then p:=mem[p].hh.rh;tossknotlist(curexp);curexp:=p;
end else if curtype=14 then pairtopath else badunary(45);{:918}{920:}
77:if not getcurbbox then badunary(77)else pairvalue(bbmin[0],bbmin[1]);
78:if not getcurbbox then badunary(78)else pairvalue(bbmax[0],bbmin[1]);
79:if not getcurbbox then badunary(79)else pairvalue(bbmin[0],bbmax[1]);
80:if not getcurbbox then badunary(80)else pairvalue(bbmax[0],bbmax[1]);
{:920}{922:}38,39:if curtype<>4 then badunary(c)else doreadorclose(c);
{:922}end;begin if aritherror then cleararith;end;end;{:885}{930:}{931:}
procedure badbinary(p:halfword;c:quarterword);begin disperr(p,284);
disperr(0,887);if c>=115 then printop(c);
printknownorunknowntype(mem[p].hh.b0,p);
if c>=115 then print(489)else printop(c);
printknownorunknowntype(curtype,curexp);begin helpptr:=3;
helpline[2]:=888;helpline[1]:=900;helpline[0]:=901;end;putgeterror;end;
{:931}{936:}function tarnished(p:halfword):halfword;label 10;
var q:halfword;r:halfword;begin q:=mem[p+1].int;
r:=q+bignodesize[mem[p].hh.b0];repeat r:=r-2;
if mem[r].hh.b0=19 then begin tarnished:=1;goto 10;end;until r=q;
tarnished:=0;10:end;{:936}{938:}{943:}procedure depfinish(v,q:halfword;
t:smallnumber);var p:halfword;vv:scaled;
begin if q=0 then p:=curexp else p:=q;mem[p+1].hh.rh:=v;mem[p].hh.b0:=t;
if mem[v].hh.lh=0 then begin vv:=mem[v+1].int;
if q=0 then flushcurexp(vv)else begin recyclevalue(p);mem[q].hh.b0:=16;
mem[q+1].int:=vv;end;end else if q=0 then curtype:=t;
if fixneeded then fixdependencies;end;{:943}
procedure addorsubtract(p,q:halfword;c:quarterword);label 30,10;
var s,t:smallnumber;r:halfword;v:integer;
begin if q=0 then begin t:=curtype;
if t<17 then v:=curexp else v:=mem[curexp+1].hh.rh;
end else begin t:=mem[q].hh.b0;
if t<17 then v:=mem[q+1].int else v:=mem[q+1].hh.rh;end;
if t=16 then begin if c=90 then v:=-v;
if mem[p].hh.b0=16 then begin v:=slowadd(mem[p+1].int,v);
if q=0 then curexp:=v else mem[q+1].int:=v;goto 10;end;{939:}
r:=mem[p+1].hh.rh;while mem[r].hh.lh<>0 do r:=mem[r].hh.rh;
mem[r+1].int:=slowadd(mem[r+1].int,v);if q=0 then begin q:=getnode(2);
curexp:=q;curtype:=mem[p].hh.b0;mem[q].hh.b1:=14;end;
mem[q+1].hh.rh:=mem[p+1].hh.rh;mem[q].hh.b0:=mem[p].hh.b0;
mem[q+1].hh.lh:=mem[p+1].hh.lh;mem[mem[p+1].hh.lh].hh.rh:=q;
mem[p].hh.b0:=16;{:939};end else begin if c=90 then negatedeplist(v);
{940:}if mem[p].hh.b0=16 then{941:}
begin while mem[v].hh.lh<>0 do v:=mem[v].hh.rh;
mem[v+1].int:=slowadd(mem[p+1].int,mem[v+1].int);end{:941}
else begin s:=mem[p].hh.b0;r:=mem[p+1].hh.rh;
if t=17 then begin if s=17 then if maxcoef(r)+maxcoef(v)<626349397 then
begin v:=pplusq(v,r,17);goto 30;end;t:=18;v:=poverv(v,65536,17,18);end;
if s=18 then v:=pplusq(v,r,18)else v:=pplusfq(v,65536,r,18,17);30:{942:}
if q<>0 then depfinish(v,q,t)else begin curtype:=t;depfinish(v,0,t);
end{:942};end{:940};end;10:end;{:938}{951:}procedure depmult(p:halfword;
v:integer;visscaled:boolean);label 10;var q:halfword;s,t:smallnumber;
begin if p=0 then q:=curexp else if mem[p].hh.b0<>16 then q:=p else
begin if visscaled then mem[p+1].int:=takescaled(mem[p+1].int,v)else mem
[p+1].int:=takefraction(mem[p+1].int,v);goto 10;end;t:=mem[q].hh.b0;
q:=mem[q+1].hh.rh;s:=t;
if t=17 then if visscaled then if abvscd(maxcoef(q),abs(v),626349396,
65536)>=0 then t:=18;q:=ptimesv(q,v,s,t,visscaled);depfinish(q,p,t);
10:end;{:951}{954:}procedure hardtimes(p:halfword);label 30;
var q:halfword;r:halfword;v:scaled;
begin if mem[p].hh.b0<=14 then begin q:=stashcurexp;unstashcurexp(p);
p:=q;end;r:=mem[curexp+1].int+bignodesize[curtype];
while true do begin r:=r-2;v:=mem[r+1].int;mem[r].hh.b0:=mem[p].hh.b0;
if r=mem[curexp+1].int then goto 30;
newdep(r,copydeplist(mem[p+1].hh.rh));depmult(r,v,true);end;
30:mem[r+1]:=mem[p+1];mem[mem[p+1].hh.lh].hh.rh:=r;freenode(p,2);
depmult(r,v,true);end;{:954}{956:}procedure depdiv(p:halfword;v:scaled);
label 10;var q:halfword;s,t:smallnumber;
begin if p=0 then q:=curexp else if mem[p].hh.b0<>16 then q:=p else
begin mem[p+1].int:=makescaled(mem[p+1].int,v);goto 10;end;
t:=mem[q].hh.b0;q:=mem[q+1].hh.rh;s:=t;
if t=17 then if abvscd(maxcoef(q),65536,626349396,abs(v))>=0 then t:=18;
q:=poverv(q,v,s,t);depfinish(q,p,t);10:end;{:956}{960:}
procedure setuptrans(c:quarterword);label 30,10;var p,q,r:halfword;
begin if(c<>108)or(curtype<>12)then{962:}begin p:=stashcurexp;
curexp:=idtransform;curtype:=12;q:=mem[curexp+1].int;case c of{964:}
104:if mem[p].hh.b0=16 then{965:}
begin nsincos((mem[p+1].int mod 23592960)*16);
mem[q+5].int:=roundfraction(ncos);mem[q+9].int:=roundfraction(nsin);
mem[q+7].int:=-mem[q+9].int;mem[q+11].int:=mem[q+5].int;goto 30;
end{:965};105:if mem[p].hh.b0>14 then begin install(q+6,p);goto 30;end;
106:if mem[p].hh.b0>14 then begin install(q+4,p);install(q+10,p);
goto 30;end;107:if mem[p].hh.b0=14 then begin r:=mem[p+1].int;
install(q,r);install(q+2,r+2);goto 30;end;
109:if mem[p].hh.b0>14 then begin install(q+4,p);goto 30;end;
110:if mem[p].hh.b0>14 then begin install(q+10,p);goto 30;end;
111:if mem[p].hh.b0=14 then{966:}begin r:=mem[p+1].int;install(q+4,r);
install(q+10,r);install(q+8,r+2);
if mem[r+2].hh.b0=16 then mem[r+3].int:=-mem[r+3].int else negatedeplist
(mem[r+3].hh.rh);install(q+6,r+2);goto 30;end{:966};108:;{:964}end;
disperr(p,910);begin helpptr:=3;helpline[2]:=911;helpline[1]:=912;
helpline[0]:=913;end;putgeterror;30:recyclevalue(p);freenode(p,2);
end{:962};{963:}q:=mem[curexp+1].int;r:=q+12;repeat r:=r-2;
if mem[r].hh.b0<>16 then goto 10;until r=q;txx:=mem[q+5].int;
txy:=mem[q+7].int;tyx:=mem[q+9].int;tyy:=mem[q+11].int;tx:=mem[q+1].int;
ty:=mem[q+3].int;flushcurexp(0){:963};10:end;{:960}{967:}
procedure setupknowntrans(c:quarterword);begin setuptrans(c);
if curtype<>16 then begin disperr(0,914);begin helpptr:=3;
helpline[2]:=915;helpline[1]:=916;helpline[0]:=913;end;
putgetflusherror(0);txx:=65536;txy:=0;tyx:=0;tyy:=65536;tx:=0;ty:=0;end;
end;{:967}{968:}procedure trans(p,q:halfword);var v:scaled;
begin v:=takescaled(mem[p].int,txx)+takescaled(mem[q].int,txy)+tx;
mem[q].int:=takescaled(mem[p].int,tyx)+takescaled(mem[q].int,tyy)+ty;
mem[p].int:=v;end;{:968}{969:}procedure dopathtrans(p:halfword);
label 10;var q:halfword;begin q:=p;
repeat if mem[q].hh.b0<>0 then trans(q+3,q+4);trans(q+1,q+2);
if mem[q].hh.b1<>0 then trans(q+5,q+6);q:=mem[q].hh.rh;until q=p;10:end;
{:969}{970:}procedure dopentrans(p:halfword);label 10;var q:halfword;
begin if(p=mem[p].hh.rh)then begin trans(p+3,p+4);trans(p+5,p+6);end;
q:=p;repeat trans(q+1,q+2);q:=mem[q].hh.rh;until q=p;10:end;{:970}{971:}
function edgestrans(h:halfword):halfword;label 31;var q:halfword;
r,s:halfword;sx,sy:scaled;sqdet:scaled;sgndet:integer;v:scaled;
begin h:=privateedges(h);sqdet:=sqrtdet(txx,txy,tyx,tyy);
sgndet:=abvscd(txx,tyy,txy,tyx);if mem[h].hh.rh<>2 then{972:}
if(txy<>0)or(tyx<>0)or(ty<>0)or(abs(txx)<>abs(tyy))then flushdashlist(h)
else begin if txx<0 then{973:}begin r:=mem[h].hh.rh;mem[h].hh.rh:=2;
while r<>2 do begin s:=r;r:=mem[r].hh.rh;v:=mem[s+1].int;
mem[s+1].int:=mem[s+2].int;mem[s+2].int:=v;mem[s].hh.rh:=mem[h].hh.rh;
mem[h].hh.rh:=s;end;end{:973};{974:}r:=mem[h].hh.rh;
while r<>2 do begin mem[r+1].int:=takescaled(mem[r+1].int,txx)+tx;
mem[r+2].int:=takescaled(mem[r+2].int,txx)+tx;r:=mem[r].hh.rh;end{:974};
mem[h+1].int:=takescaled(mem[h+1].int,abs(tyy));end{:972};{975:}
if(txx=0)and(tyy=0)then{976:}begin v:=mem[h+2].int;
mem[h+2].int:=mem[h+3].int;mem[h+3].int:=v;v:=mem[h+4].int;
mem[h+4].int:=mem[h+5].int;mem[h+5].int:=v;end{:976}
else if(txy<>0)or(tyx<>0)then begin initbbox(h);goto 31;end;
if mem[h+2].int<=mem[h+4].int then{977:}
begin mem[h+2].int:=takescaled(mem[h+2].int,txx+txy)+tx;
mem[h+4].int:=takescaled(mem[h+4].int,txx+txy)+tx;
mem[h+3].int:=takescaled(mem[h+3].int,tyx+tyy)+ty;
mem[h+5].int:=takescaled(mem[h+5].int,tyx+tyy)+ty;
if txx+txy<0 then begin v:=mem[h+2].int;mem[h+2].int:=mem[h+4].int;
mem[h+4].int:=v;end;if tyx+tyy<0 then begin v:=mem[h+3].int;
mem[h+3].int:=mem[h+5].int;mem[h+5].int:=v;end;end{:977};31:{:975};
q:=mem[h+7].hh.rh;while q<>0 do begin{978:}
case mem[q].hh.b0 of 1,2:begin dopathtrans(mem[q+1].hh.rh);{979:}
if mem[q+1].hh.lh<>0 then begin sx:=tx;sy:=ty;tx:=0;ty:=0;
dopentrans(mem[q+1].hh.lh);
if((mem[q].hh.b0=2)and(mem[q+6].hh.rh<>0))then mem[q+7].int:=takescaled(
mem[q+7].int,sqdet);
if not(mem[q+1].hh.lh=mem[mem[q+1].hh.lh].hh.rh)then if sgndet<0 then
mem[q+1].hh.lh:=makepen(copypath(mem[q+1].hh.lh),true);tx:=sx;ty:=sy;
end{:979};end;4,5:dopathtrans(mem[q+1].hh.rh);3:begin r:=q+8;{980:}
trans(r,r+1);sx:=tx;sy:=ty;tx:=0;ty:=0;trans(r+2,r+4);trans(r+3,r+5);
tx:=sx;ty:=sy{:980};end;6,7:;end{:978};q:=mem[q].hh.rh;end;
edgestrans:=h;end;procedure doedgestrans(p:halfword;c:quarterword);
begin setupknowntrans(c);mem[p+1].int:=edgestrans(mem[p+1].int);
unstashcurexp(p);end;procedure scaleedges;begin txx:=sesf;tyy:=sesf;
txy:=0;tyx:=0;tx:=0;ty:=0;sepic:=edgestrans(sepic);end;{:971}{981:}
{983:}procedure bilin1(p:halfword;t:scaled;q:halfword;u,delta:scaled);
var r:halfword;begin if t<>65536 then depmult(p,t,true);
if u<>0 then if mem[q].hh.b0=16 then delta:=delta+takescaled(mem[q+1].
int,u)else begin{984:}
if mem[p].hh.b0<>18 then begin if mem[p].hh.b0=16 then newdep(p,
constdependency(mem[p+1].int))else mem[p+1].hh.rh:=ptimesv(mem[p+1].hh.
rh,65536,17,18,true);mem[p].hh.b0:=18;end{:984};
mem[p+1].hh.rh:=pplusfq(mem[p+1].hh.rh,u,mem[q+1].hh.rh,18,mem[q].hh.b0)
;end;
if mem[p].hh.b0=16 then mem[p+1].int:=mem[p+1].int+delta else begin r:=
mem[p+1].hh.rh;while mem[r].hh.lh<>0 do r:=mem[r].hh.rh;
delta:=mem[r+1].int+delta;
if r<>mem[p+1].hh.rh then mem[r+1].int:=delta else begin recyclevalue(p)
;mem[p].hh.b0:=16;mem[p+1].int:=delta;end;end;
if fixneeded then fixdependencies;end;{:983}{986:}
procedure addmultdep(p:halfword;v:scaled;r:halfword);
begin if mem[r].hh.b0=16 then mem[depfinal+1].int:=mem[depfinal+1].int+
takescaled(mem[r+1].int,v)else begin mem[p+1].hh.rh:=pplusfq(mem[p+1].hh
.rh,v,mem[r+1].hh.rh,18,mem[r].hh.b0);if fixneeded then fixdependencies;
end;end;{:986}{987:}procedure bilin2(p,t:halfword;v:scaled;
u,q:halfword);var vv:scaled;begin vv:=mem[p+1].int;mem[p].hh.b0:=18;
newdep(p,constdependency(0));if vv<>0 then addmultdep(p,vv,t);
if v<>0 then addmultdep(p,v,u);if q<>0 then addmultdep(p,65536,q);
if mem[p+1].hh.rh=depfinal then begin vv:=mem[depfinal+1].int;
recyclevalue(p);mem[p].hh.b0:=16;mem[p+1].int:=vv;end;end;{:987}{989:}
procedure bilin3(p:halfword;t,v,u,delta:scaled);
begin if t<>65536 then delta:=delta+takescaled(mem[p+1].int,t)else delta
:=delta+mem[p+1].int;
if u<>0 then mem[p+1].int:=delta+takescaled(v,u)else mem[p+1].int:=delta
;end;{:989}procedure bigtrans(p:halfword;c:quarterword);label 10;
var q,r,pp,qq:halfword;s:smallnumber;begin s:=bignodesize[mem[p].hh.b0];
q:=mem[p+1].int;r:=q+s;repeat r:=r-2;if mem[r].hh.b0<>16 then{982:}
begin setupknowntrans(c);makeexpcopy(p);r:=mem[curexp+1].int;
if curtype=12 then begin bilin1(r+10,tyy,q+6,tyx,0);
bilin1(r+8,tyy,q+4,tyx,0);bilin1(r+6,txx,q+10,txy,0);
bilin1(r+4,txx,q+8,txy,0);end;bilin1(r+2,tyy,q,tyx,ty);
bilin1(r,txx,q+2,txy,tx);goto 10;end{:982};until r=q;{985:}
setuptrans(c);if curtype=16 then{988:}begin makeexpcopy(p);
r:=mem[curexp+1].int;
if curtype=12 then begin bilin3(r+10,tyy,mem[q+7].int,tyx,0);
bilin3(r+8,tyy,mem[q+5].int,tyx,0);bilin3(r+6,txx,mem[q+11].int,txy,0);
bilin3(r+4,txx,mem[q+9].int,txy,0);end;
bilin3(r+2,tyy,mem[q+1].int,tyx,ty);bilin3(r,txx,mem[q+3].int,txy,tx);
end{:988}else begin pp:=stashcurexp;qq:=mem[pp+1].int;makeexpcopy(p);
r:=mem[curexp+1].int;
if curtype=12 then begin bilin2(r+10,qq+10,mem[q+7].int,qq+8,0);
bilin2(r+8,qq+10,mem[q+5].int,qq+8,0);
bilin2(r+6,qq+4,mem[q+11].int,qq+6,0);
bilin2(r+4,qq+4,mem[q+9].int,qq+6,0);end;
bilin2(r+2,qq+10,mem[q+1].int,qq+8,qq+2);
bilin2(r,qq+4,mem[q+3].int,qq+6,qq);recyclevalue(pp);freenode(pp,2);end;
{:985};10:end;{:981}{991:}procedure cat(p:halfword);var a,b:strnumber;
k:poolpointer;begin a:=mem[p+1].int;b:=curexp;
begin if poolptr+(strstart[nextstr[a]]-strstart[a])+(strstart[nextstr[b]
]-strstart[b])>maxpoolptr then if poolptr+(strstart[nextstr[a]]-strstart
[a])+(strstart[nextstr[b]]-strstart[b])>poolsize then docompaction((
strstart[nextstr[a]]-strstart[a])+(strstart[nextstr[b]]-strstart[b]))
else maxpoolptr:=poolptr+(strstart[nextstr[a]]-strstart[a])+(strstart[
nextstr[b]]-strstart[b]);end;
for k:=strstart[a]to strstart[nextstr[a]]-1 do begin strpool[poolptr]:=
strpool[k];incr(poolptr);end;
for k:=strstart[b]to strstart[nextstr[b]]-1 do begin strpool[poolptr]:=
strpool[k];incr(poolptr);end;curexp:=makestring;
begin if strref[b]<127 then if strref[b]>1 then decr(strref[b])else
flushstring(b);end;end;{:991}{992:}procedure chopstring(p:halfword);
var a,b:integer;l:integer;k:integer;s:strnumber;reversed:boolean;
begin a:=roundunscaled(mem[p+1].int);b:=roundunscaled(mem[p+3].int);
if a<=b then reversed:=false else begin reversed:=true;k:=a;a:=b;b:=k;
end;s:=curexp;l:=(strstart[nextstr[s]]-strstart[s]);
if a<0 then begin a:=0;if b<0 then b:=0;end;if b>l then begin b:=l;
if a>l then a:=l;end;
begin if poolptr+b-a>maxpoolptr then if poolptr+b-a>poolsize then
docompaction(b-a)else maxpoolptr:=poolptr+b-a;end;
if reversed then for k:=strstart[s]+b-1 downto strstart[s]+a do begin
strpool[poolptr]:=strpool[k];incr(poolptr);
end else for k:=strstart[s]+a to strstart[s]+b-1 do begin strpool[
poolptr]:=strpool[k];incr(poolptr);end;curexp:=makestring;
begin if strref[s]<127 then if strref[s]>1 then decr(strref[s])else
flushstring(s);end;end;{:992}{993:}procedure choppath(p:halfword);
var q:halfword;pp,qq,rr,ss:halfword;a,b,k,l:scaled;reversed:boolean;
begin l:=pathlength;a:=mem[p+1].int;b:=mem[p+3].int;
if a<=b then reversed:=false else begin reversed:=true;k:=a;a:=b;b:=k;
end;{994:}if a<0 then if mem[curexp].hh.b0=0 then begin a:=0;
if b<0 then b:=0;end else repeat a:=a+l;b:=b+l;until a>=0;
if b>l then if mem[curexp].hh.b0=0 then begin b:=l;if a>l then a:=l;
end else while a>=l do begin a:=a-l;b:=b-l;end{:994};q:=curexp;
while a>=65536 do begin q:=mem[q].hh.rh;a:=a-65536;b:=b-65536;end;
if b=a then{996:}begin if a>0 then begin splitcubic(q,a*4096);
q:=mem[q].hh.rh;end;pp:=copyknot(q);qq:=pp;end{:996}else{995:}
begin pp:=copyknot(q);qq:=pp;repeat q:=mem[q].hh.rh;rr:=qq;
qq:=copyknot(q);mem[rr].hh.rh:=qq;b:=b-65536;until b<=0;
if a>0 then begin ss:=pp;pp:=mem[pp].hh.rh;splitcubic(ss,a*4096);
pp:=mem[ss].hh.rh;freenode(ss,7);
if rr=ss then begin b:=makescaled(b,65536-a);rr:=pp;end;end;
if b<0 then begin splitcubic(rr,(b+65536)*4096);freenode(qq,7);
qq:=mem[rr].hh.rh;end;end{:995};mem[pp].hh.b0:=0;mem[qq].hh.b1:=0;
mem[qq].hh.rh:=pp;tossknotlist(curexp);
if reversed then begin curexp:=mem[htapypoc(pp)].hh.rh;tossknotlist(pp);
end else curexp:=pp;end;{:993}{998:}procedure setupoffset(p:halfword);
begin findoffset(mem[p+1].int,mem[p+3].int,curexp);pairvalue(curx,cury);
end;procedure setupdirectiontime(p:halfword);
begin flushcurexp(finddirectiontime(mem[p+1].int,mem[p+3].int,curexp));
end;{:998}{999:}procedure findpoint(v:scaled;c:quarterword);
var p:halfword;n:scaled;begin p:=curexp;
if mem[p].hh.b0=0 then n:=-65536 else n:=0;repeat p:=mem[p].hh.rh;
n:=n+65536;until p=curexp;
if n=0 then v:=0 else if v<0 then if mem[p].hh.b0=0 then v:=0 else v:=n
-1-((-v-1)mod n)else if v>n then if mem[p].hh.b0=0 then v:=n else v:=v
mod n;p:=curexp;while v>=65536 do begin p:=mem[p].hh.rh;v:=v-65536;end;
if v<>0 then{1000:}begin splitcubic(p,v*4096);p:=mem[p].hh.rh;end{:1000}
;{1001:}case c of 118:pairvalue(mem[p+1].int,mem[p+2].int);
119:if mem[p].hh.b0=0 then pairvalue(mem[p+1].int,mem[p+2].int)else
pairvalue(mem[p+3].int,mem[p+4].int);
120:if mem[p].hh.b1=0 then pairvalue(mem[p+1].int,mem[p+2].int)else
pairvalue(mem[p+5].int,mem[p+6].int);end{:1001};end;{:999}{1005:}
procedure doinfont(p:halfword);var q:halfword;begin q:=getnode(8);
initedges(q);
mem[mem[q+7].hh.lh].hh.rh:=newtextnode(curexp,mem[p+1].int);
mem[q+7].hh.lh:=mem[mem[q+7].hh.lh].hh.rh;freenode(p,2);flushcurexp(q);
curtype:=10;end;{:1005}procedure dobinary(p:halfword;c:quarterword);
label 30,31,10;var q,r,rr:halfword;oldp,oldexp:halfword;v:integer;
begin begin if aritherror then cleararith;end;
if internal[6]>131072 then{932:}begin begindiagnostic;printnl(902);
printexp(p,0);printchar(41);printop(c);printchar(40);printexp(0,0);
print(891);enddiagnostic(false);end{:932};{934:}
case mem[p].hh.b0 of 12,13,14:oldp:=tarnished(p);19:oldp:=1;
others:oldp:=0 end;if oldp<>0 then begin q:=stashcurexp;oldp:=p;
makeexpcopy(oldp);p:=stashcurexp;unstashcurexp(q);end;{:934};{935:}
case curtype of 12,13,14:oldexp:=tarnished(curexp);19:oldexp:=1;
others:oldexp:=0 end;if oldexp<>0 then begin oldexp:=curexp;
makeexpcopy(oldexp);end{:935};case c of 89,90:{937:}
if(curtype<13)or(mem[p].hh.b0<13)then badbinary(p,c)else if(curtype>14)
and(mem[p].hh.b0>14)then addorsubtract(p,0,c)else if curtype<>mem[p].hh.
b0 then badbinary(p,c)else begin q:=mem[p+1].int;r:=mem[curexp+1].int;
rr:=r+bignodesize[curtype];while r<rr do begin addorsubtract(q,r,c);
q:=q+2;r:=r+2;end;end{:937};{944:}
97,98,99,100,101,102:begin begin if aritherror then cleararith;end;
if(curtype>14)and(mem[p].hh.b0>14)then addorsubtract(p,0,90)else if
curtype<>mem[p].hh.b0 then begin badbinary(p,c);goto 30;
end else if curtype=4 then flushcurexp(strvsstr(mem[p+1].int,curexp))
else if(curtype=5)or(curtype=3)then{946:}begin q:=mem[curexp+1].int;
while(q<>curexp)and(q<>p)do q:=mem[q+1].int;if q=p then flushcurexp(0);
end{:946}else if(curtype<=14)and(curtype>=12)then{947:}
begin q:=mem[p+1].int;r:=mem[curexp+1].int;rr:=r+bignodesize[curtype]-2;
while true do begin addorsubtract(q,r,90);
if mem[r].hh.b0<>16 then goto 31;if mem[r+1].int<>0 then goto 31;
if r=rr then goto 31;q:=q+2;r:=r+2;end;31:takepart(mem[r].hh.b1+49);
end{:947}
else if curtype=2 then flushcurexp(curexp-mem[p+1].int)else begin
badbinary(p,c);goto 30;end;{945:}
if curtype<>16 then begin if curtype<16 then begin disperr(p,284);
begin helpptr:=1;helpline[0]:=903;end end else begin helpptr:=2;
helpline[1]:=904;helpline[0]:=905;end;disperr(0,906);
putgetflusherror(31);
end else case c of 97:if curexp<0 then curexp:=30 else curexp:=31;
98:if curexp<=0 then curexp:=30 else curexp:=31;
99:if curexp>0 then curexp:=30 else curexp:=31;
100:if curexp>=0 then curexp:=30 else curexp:=31;
101:if curexp=0 then curexp:=30 else curexp:=31;
102:if curexp<>0 then curexp:=30 else curexp:=31;end;curtype:=2{:945};
30:aritherror:=false;end;{:944}{948:}
96,95:if(mem[p].hh.b0<>2)or(curtype<>2)then badbinary(p,c)else if mem[p
+1].int=c-65 then curexp:=mem[p+1].int;{:948}{949:}
91:if(curtype<13)or(mem[p].hh.b0<13)then badbinary(p,91)else if(curtype=
16)or(mem[p].hh.b0=16)then{950:}
begin if mem[p].hh.b0=16 then begin v:=mem[p+1].int;freenode(p,2);
end else begin v:=curexp;unstashcurexp(p);end;
if curtype=16 then curexp:=takescaled(curexp,v)else if(curtype=14)or(
curtype=13)then begin p:=mem[curexp+1].int+bignodesize[curtype];
repeat p:=p-2;depmult(p,v,true);until p=mem[curexp+1].int;
end else depmult(0,v,true);goto 10;end{:950}
else if(nicecolororpair(p,mem[p].hh.b0)and(curtype>14))or(
nicecolororpair(curexp,curtype)and(mem[p].hh.b0>14))then begin hardtimes
(p);goto 10;end else badbinary(p,91);{:949}{955:}
92:if(curtype<>16)or(mem[p].hh.b0<13)then badbinary(p,92)else begin v:=
curexp;unstashcurexp(p);if v=0 then{957:}begin disperr(0,835);
begin helpptr:=2;helpline[1]:=908;helpline[0]:=909;end;putgeterror;
end{:957}
else begin if curtype=16 then curexp:=makescaled(curexp,v)else if
curtype<=14 then begin p:=mem[curexp+1].int+bignodesize[curtype];
repeat p:=p-2;depdiv(p,v);until p=mem[curexp+1].int;
end else depdiv(0,v);end;goto 10;end;{:955}{958:}
93,94:if(curtype=16)and(mem[p].hh.b0=16)then if c=93 then curexp:=
pythadd(mem[p+1].int,curexp)else curexp:=pythsub(mem[p+1].int,curexp)
else badbinary(p,c);{:958}{959:}
104,105,106,107,108,109,110,111:if mem[p].hh.b0=8 then begin begin
setupknowntrans(c);unstashcurexp(p);dopathtrans(curexp);end;goto 10;
end else if mem[p].hh.b0=6 then begin begin setupknowntrans(c);
unstashcurexp(p);dopentrans(curexp);end;curexp:=convexhull(curexp);
goto 10;
end else if(mem[p].hh.b0=14)or(mem[p].hh.b0=12)then bigtrans(p,c)else if
mem[p].hh.b0=10 then begin doedgestrans(p,c);goto 10;
end else badbinary(p,c);{:959}{990:}
103:if(curtype=4)and(mem[p].hh.b0=4)then cat(p)else badbinary(p,103);
115:if nicepair(p,mem[p].hh.b0)and(curtype=4)then chopstring(mem[p+1].
int)else badbinary(p,115);116:begin if curtype=14 then pairtopath;
if nicepair(p,mem[p].hh.b0)and(curtype=8)then choppath(mem[p+1].int)else
badbinary(p,116);end;{:990}{997:}
118,119,120:begin if curtype=14 then pairtopath;
if(curtype=8)and(mem[p].hh.b0=16)then findpoint(mem[p+1].int,c)else
badbinary(p,c);end;
121:if(curtype=6)and nicepair(p,mem[p].hh.b0)then setupoffset(mem[p+1].
int)else badbinary(p,121);117:begin if curtype=14 then pairtopath;
if(curtype=8)and nicepair(p,mem[p].hh.b0)then setupdirectiontime(mem[p+1
].int)else badbinary(p,117);end;{:997}{1002:}
122:begin if curtype=14 then pairtopath;
if(curtype=8)and(mem[p].hh.b0=16)then flushcurexp(getarctime(curexp,mem[
p+1].int))else badbinary(p,c);end;{:1002}{1003:}
113:begin if mem[p].hh.b0=14 then begin q:=stashcurexp;unstashcurexp(p);
pairtopath;p:=stashcurexp;unstashcurexp(q);end;
if curtype=14 then pairtopath;
if(curtype=8)and(mem[p].hh.b0=8)then begin pathintersection(mem[p+1].int
,curexp);pairvalue(curt,curtt);end else badbinary(p,113);end;{:1003}
{1004:}
112:if(curtype<>4)or(mem[p].hh.b0<>4)then badbinary(p,112)else begin
doinfont(p);goto 10;end;{:1004}end;recyclevalue(p);freenode(p,2);
10:begin if aritherror then cleararith;end;{933:}
if oldp<>0 then begin recyclevalue(oldp);freenode(oldp,2);end;
if oldexp<>0 then begin recyclevalue(oldexp);freenode(oldexp,2);
end{:933};end;{:930}{952:}procedure fracmult(n,d:scaled);var p:halfword;
oldexp:halfword;v:fraction;begin if internal[6]>131072 then{953:}
begin begindiagnostic;printnl(902);printscaled(n);printchar(47);
printscaled(d);print(907);printexp(0,0);print(891);enddiagnostic(false);
end{:953};case curtype of 12,13,14:oldexp:=tarnished(curexp);
19:oldexp:=1;others:oldexp:=0 end;
if oldexp<>0 then begin oldexp:=curexp;makeexpcopy(oldexp);end;
v:=makefraction(n,d);
if curtype=16 then curexp:=takefraction(curexp,v)else if curtype<=14
then begin p:=mem[curexp+1].int+bignodesize[curtype];repeat p:=p-2;
depmult(p,v,false);until p=mem[curexp+1].int;
end else depmult(0,v,false);
if oldexp<>0 then begin recyclevalue(oldexp);freenode(oldexp,2);end end;
{:952}{1006:}{1012:}{1023:}procedure tryeq(l,r:halfword);label 30,31;
var p:halfword;t:16..19;q:halfword;pp:halfword;tt:17..19;copied:boolean;
begin{1024:}t:=mem[l].hh.b0;if t=16 then begin t:=17;
p:=constdependency(-mem[l+1].int);q:=p;
end else if t=19 then begin t:=17;p:=singledependency(l);
mem[p+1].int:=-mem[p+1].int;q:=depfinal;
end else begin p:=mem[l+1].hh.rh;q:=p;
while true do begin mem[q+1].int:=-mem[q+1].int;
if mem[q].hh.lh=0 then goto 30;q:=mem[q].hh.rh;end;
30:mem[mem[l+1].hh.lh].hh.rh:=mem[q].hh.rh;
mem[mem[q].hh.rh+1].hh.lh:=mem[l+1].hh.lh;mem[l].hh.b0:=16;end{:1024};
{1026:}
if r=0 then if curtype=16 then begin mem[q+1].int:=mem[q+1].int+curexp;
goto 31;end else begin tt:=curtype;
if tt=19 then pp:=singledependency(curexp)else pp:=mem[curexp+1].hh.rh;
end else if mem[r].hh.b0=16 then begin mem[q+1].int:=mem[q+1].int+mem[r
+1].int;goto 31;end else begin tt:=mem[r].hh.b0;
if tt=19 then pp:=singledependency(r)else pp:=mem[r+1].hh.rh;end;
if tt<>19 then copied:=false else begin copied:=true;tt:=17;end;{1027:}
watchcoefs:=false;
if t=tt then p:=pplusq(p,pp,t)else if t=18 then p:=pplusfq(p,65536,pp,18
,17)else begin q:=p;
while mem[q].hh.lh<>0 do begin mem[q+1].int:=roundfraction(mem[q+1].int)
;q:=mem[q].hh.rh;end;t:=18;p:=pplusq(p,pp,t);end;watchcoefs:=true;
{:1027};if copied then flushnodelist(pp);31:{:1026};
if mem[p].hh.lh=0 then{1025:}
begin if abs(mem[p+1].int)>64 then begin begin if interaction=3 then;
printnl(262);print(945);end;print(947);printscaled(mem[p+1].int);
printchar(41);begin helpptr:=2;helpline[1]:=946;helpline[0]:=944;end;
putgeterror;end else if r=0 then{577:}begin begin if interaction=3 then;
printnl(262);print(611);end;begin helpptr:=2;helpline[1]:=612;
helpline[0]:=613;end;putgeterror;end{:577};freenode(p,2);end{:1025}
else begin lineareq(p,t);
if r=0 then if curtype<>16 then if mem[curexp].hh.b0=16 then begin pp:=
curexp;curexp:=mem[curexp+1].int;curtype:=16;freenode(pp,2);end;end;end;
{:1023}{1018:}procedure makeeq(lhs:halfword);label 20,30,45;
var t:smallnumber;v:integer;p,q:halfword;begin 20:t:=mem[lhs].hh.b0;
if t<=14 then v:=mem[lhs+1].int;case t of{1020:}
2,4,6,8,10:if curtype=t+1 then begin nonlineareq(v,curexp,false);
goto 30;end else if curtype=t then{1021:}
begin if curtype<=4 then begin if curtype=4 then begin if strvsstr(v,
curexp)<>0 then goto 45;end else if v<>curexp then goto 45;{577:}
begin begin if interaction=3 then;printnl(262);print(611);end;
begin helpptr:=2;helpline[1]:=612;helpline[0]:=613;end;putgeterror;
end{:577};goto 30;end;begin if interaction=3 then;printnl(262);
print(942);end;begin helpptr:=2;helpline[1]:=943;helpline[0]:=944;end;
putgeterror;goto 30;45:begin if interaction=3 then;printnl(262);
print(945);end;begin helpptr:=2;helpline[1]:=946;helpline[0]:=944;end;
putgeterror;goto 30;end{:1021};
3,5,7,11,9:if curtype=t-1 then begin nonlineareq(curexp,lhs,true);
goto 30;end else if curtype=t then begin ringmerge(lhs,curexp);goto 30;
end else if curtype=14 then if t=9 then begin pairtopath;goto 20;end;
12,13,14:if curtype=t then{1022:}begin p:=v+bignodesize[t];
q:=mem[curexp+1].int+bignodesize[t];repeat p:=p-2;q:=q-2;tryeq(p,q);
until p=v;goto 30;end{:1022};
16,17,18,19:if curtype>=16 then begin tryeq(lhs,0);goto 30;end;1:;
{:1020}end;{1019:}disperr(lhs,284);disperr(0,939);
if mem[lhs].hh.b0<=14 then printtype(mem[lhs].hh.b0)else print(340);
printchar(61);if curtype<=14 then printtype(curtype)else print(340);
printchar(41);begin helpptr:=2;helpline[1]:=940;helpline[0]:=941;end;
putgeterror{:1019};30:begin if aritherror then cleararith;end;
recyclevalue(lhs);freenode(lhs,2);end;{:1018}procedure doassignment;
forward;procedure doequation;var lhs:halfword;p:halfword;
begin lhs:=stashcurexp;getxnext;varflag:=76;scanexpression;
if curcmd=53 then doequation else if curcmd=76 then doassignment;
if internal[6]>131072 then{1014:}begin begindiagnostic;printnl(902);
printexp(lhs,0);print(934);printexp(0,0);print(891);
enddiagnostic(false);end{:1014};
if curtype=9 then if mem[lhs].hh.b0=14 then begin p:=stashcurexp;
unstashcurexp(lhs);lhs:=p;end;makeeq(lhs);end;{:1012}{1013:}
procedure doassignment;var lhs:halfword;p:halfword;q:halfword;
begin if curtype<>20 then begin disperr(0,931);begin helpptr:=2;
helpline[1]:=932;helpline[0]:=933;end;error;doequation;
end else begin lhs:=curexp;curtype:=1;getxnext;varflag:=76;
scanexpression;
if curcmd=53 then doequation else if curcmd=76 then doassignment;
if internal[6]>131072 then{1015:}begin begindiagnostic;printnl(123);
if mem[lhs].hh.lh>9771 then print(intname[mem[lhs].hh.lh-(9771)])else
showtokenlist(lhs,0,1000,0);print(476);printexp(0,0);printchar(125);
enddiagnostic(false);end{:1015};if mem[lhs].hh.lh>9771 then{1016:}
if curtype=16 then internal[mem[lhs].hh.lh-(9771)]:=curexp else begin
disperr(0,935);print(intname[mem[lhs].hh.lh-(9771)]);print(936);
begin helpptr:=2;helpline[1]:=937;helpline[0]:=938;end;putgeterror;
end{:1016}else{1017:}begin p:=findvariable(lhs);
if p<>0 then begin q:=stashcurexp;curtype:=undtype(p);recyclevalue(p);
mem[p].hh.b0:=curtype;mem[p+1].int:=0;makeexpcopy(p);p:=stashcurexp;
unstashcurexp(q);makeeq(p);end else begin obliterated(lhs);putgeterror;
end;end{:1017};flushnodelist(lhs);end;end;{:1013}{1032:}
procedure dotypedeclaration;var t:smallnumber;p:halfword;q:halfword;
begin if curmod>=12 then t:=curmod else t:=curmod+1;
repeat p:=scandeclaredvariable;
flushvariable(eqtb[mem[p].hh.lh].rh,mem[p].hh.rh,false);
q:=findvariable(p);if q<>0 then begin mem[q].hh.b0:=t;mem[q+1].int:=0;
end else begin begin if interaction=3 then;printnl(262);print(948);end;
begin helpptr:=2;helpline[1]:=949;helpline[0]:=950;end;putgeterror;end;
flushlist(p);if curcmd<81 then{1033:}begin begin if interaction=3 then;
printnl(262);print(951);end;begin helpptr:=5;helpline[4]:=952;
helpline[3]:=953;helpline[2]:=954;helpline[1]:=955;helpline[0]:=956;end;
if curcmd=44 then helpline[2]:=957;putgeterror;scannerstatus:=2;
repeat begin getnext;if curcmd<=3 then tnext;end;{715:}
if curcmd=41 then begin if strref[curmod]<127 then if strref[curmod]>1
then decr(strref[curmod])else flushstring(curmod);end{:715};
until curcmd>=81;scannerstatus:=0;end{:1033};until curcmd>81;end;{:1032}
{1038:}procedure dorandomseed;begin getxnext;
if curcmd<>76 then begin missingerr(476);begin helpptr:=1;
helpline[0]:=962;end;backerror;end;getxnext;scanexpression;
if curtype<>16 then begin disperr(0,963);begin helpptr:=2;
helpline[1]:=964;helpline[0]:=965;end;putgetflusherror(0);
end else{1039:}begin initrandoms(curexp);
if selector>=9 then begin oldsetting:=selector;selector:=9;printnl(966);
printscaled(curexp);printchar(125);printnl(284);selector:=oldsetting;
end;end{:1039};end;{:1038}{1046:}procedure doprotection;var m:0..1;
t:halfword;begin m:=curmod;repeat getsymbol;t:=eqtb[cursym].lh;
if m=0 then begin if t>=85 then eqtb[cursym].lh:=t-85;
end else if t<85 then eqtb[cursym].lh:=t+85;getxnext;until curcmd<>81;
end;{:1046}{1048:}procedure defdelims;var ldelim,rdelim:halfword;
begin getclearsymbol;ldelim:=cursym;getclearsymbol;rdelim:=cursym;
eqtb[ldelim].lh:=33;eqtb[ldelim].rh:=rdelim;eqtb[rdelim].lh:=64;
eqtb[rdelim].rh:=ldelim;getxnext;end;{:1048}{1051:}
procedure dostatement;forward;procedure dointerim;begin getxnext;
if curcmd<>42 then begin begin if interaction=3 then;printnl(262);
print(972);end;if cursym=0 then print(977)else print(hash[cursym].rh);
print(978);begin helpptr:=1;helpline[0]:=979;end;backerror;
end else begin saveinternal(curmod);backinput;end;dostatement;end;
{:1051}{1052:}procedure dolet;var l:halfword;begin getsymbol;l:=cursym;
getxnext;if curcmd<>53 then if curcmd<>76 then begin missingerr(61);
begin helpptr:=3;helpline[2]:=980;helpline[1]:=713;helpline[0]:=981;end;
backerror;end;getsymbol;
case curcmd of 13,55,46,51:incr(mem[curmod].hh.lh);others:end;
clearsymbol(l,false);eqtb[l].lh:=curcmd;
if curcmd=43 then eqtb[l].rh:=0 else eqtb[l].rh:=curmod;getxnext;end;
{:1052}{1053:}procedure donewinternal;
begin repeat if intptr=maxinternal then overflow(982,maxinternal);
getclearsymbol;incr(intptr);eqtb[cursym].lh:=42;eqtb[cursym].rh:=intptr;
intname[intptr]:=hash[cursym].rh;internal[intptr]:=0;getxnext;
until curcmd<>81;end;{:1053}{1057:}procedure doshow;
begin repeat getxnext;scanexpression;printnl(804);printexp(0,2);
flushcurexp(0);until curcmd<>81;end;{:1057}{1058:}procedure disptoken;
begin printnl(988);if cursym=0 then{1059:}
begin if curcmd=44 then printscaled(curmod)else if curcmd=40 then begin
gpointer:=curmod;printcapsule;end else begin printchar(34);
print(curmod);printchar(34);
begin if strref[curmod]<127 then if strref[curmod]>1 then decr(strref[
curmod])else flushstring(curmod);end;end;end{:1059}
else begin print(hash[cursym].rh);printchar(61);
if eqtb[cursym].lh>=85 then print(989);printcmdmod(curcmd,curmod);
if curcmd=13 then begin println;showmacro(curmod,0,100000);end;end;end;
{:1058}{1061:}procedure doshowtoken;begin repeat begin getnext;
if curcmd<=3 then tnext;end;disptoken;getxnext;until curcmd<>81;end;
{:1061}{1062:}procedure doshowstats;begin printnl(998);
ifdef('STAT')printint(varused);printchar(38);printint(dynused);
if false then endif('STAT')print(359);print(999);
printint(himemmin-lomemmax-1);print(1000);println;printnl(1001);
ifdef('STAT')printint(strsinuse-initstruse);printchar(38);
printint(poolinuse-initpoolptr);if false then endif('STAT')print(359);
print(999);printint(maxstrings-1-strsusedup);printchar(38);
printint(poolsize-poolptr);print(1002);println;getxnext;end;{:1062}
{1063:}procedure dispvar(p:halfword);var q:halfword;n:0..maxprintline;
begin if mem[p].hh.b0=21 then{1064:}begin q:=mem[p+1].hh.lh;
repeat dispvar(q);q:=mem[q].hh.rh;until q=9;q:=mem[p+1].hh.rh;
while mem[q].hh.b1=3 do begin dispvar(q);q:=mem[q].hh.rh;end;end{:1064}
else if mem[p].hh.b0>=22 then{1065:}begin printnl(284);
printvariablename(p);if mem[p].hh.b0>22 then print(705);print(1003);
if fileoffset>=maxprintline-20 then n:=5 else n:=maxprintline-fileoffset
-15;showmacro(mem[p+1].int,0,n);end{:1065}
else if mem[p].hh.b0<>0 then begin printnl(284);printvariablename(p);
printchar(61);printexp(p,0);end;end;{:1063}{1066:}procedure doshowvar;
label 30;begin repeat begin getnext;if curcmd<=3 then tnext;end;
if cursym>0 then if cursym<=9771 then if curcmd=43 then if curmod<>0
then begin dispvar(curmod);goto 30;end;disptoken;30:getxnext;
until curcmd<>81;end;{:1066}{1067:}procedure doshowdependencies;
var p:halfword;begin p:=mem[5].hh.rh;
while p<>5 do begin if interesting(p)then begin printnl(284);
printvariablename(p);
if mem[p].hh.b0=17 then printchar(61)else print(817);
printdependency(mem[p+1].hh.rh,mem[p].hh.b0);end;p:=mem[p+1].hh.rh;
while mem[p].hh.lh<>0 do p:=mem[p].hh.rh;p:=mem[p].hh.rh;end;getxnext;
end;{:1067}{1068:}procedure doshowwhatever;begin if interaction=3 then;
case curmod of 0:doshowtoken;1:doshowstats;2:doshow;3:doshowvar;
4:doshowdependencies;end;
if internal[25]>0 then begin begin if interaction=3 then;printnl(262);
print(1004);end;if interaction<3 then begin helpptr:=0;decr(errorcount);
end else begin helpptr:=1;helpline[0]:=1005;end;
if curcmd=82 then error else putgeterror;end;end;{:1068}{1071:}
procedure scanwithlist(p:halfword);label 30,31,32;var t:smallnumber;
q:halfword;cp,pp,dp:halfword;begin cp:=1;pp:=1;dp:=1;
while curcmd=68 do begin t:=curmod;getxnext;scanexpression;
if curtype<>t then{1072:}begin disperr(0,1012);begin helpptr:=2;
helpline[1]:=1013;helpline[0]:=1014;end;
if t=10 then helpline[1]:=1015 else if t=13 then helpline[1]:=1016;
putgetflusherror(0);end{:1072}
else if t=13 then begin if cp=1 then{1074:}begin cp:=p;
while cp<>0 do begin if(mem[cp].hh.b0<4)then goto 30;cp:=mem[cp].hh.rh;
end;30:;end{:1074};if cp<>0 then{1073:}begin q:=mem[curexp+1].int;
mem[cp+2].int:=mem[q+1].int;mem[cp+3].int:=mem[q+3].int;
mem[cp+4].int:=mem[q+5].int;if mem[cp+2].int<0 then mem[cp+2].int:=0;
if mem[cp+3].int<0 then mem[cp+3].int:=0;
if mem[cp+4].int<0 then mem[cp+4].int:=0;
if mem[cp+2].int>65536 then mem[cp+2].int:=65536;
if mem[cp+3].int>65536 then mem[cp+3].int:=65536;
if mem[cp+4].int>65536 then mem[cp+4].int:=65536;end{:1073};
flushcurexp(0);end else if t=6 then begin if pp=1 then{1075:}
begin pp:=p;while pp<>0 do begin if(mem[pp].hh.b0<3)then goto 31;
pp:=mem[pp].hh.rh;end;31:;end{:1075};
if pp<>0 then begin if mem[pp+1].hh.lh<>0 then tossknotlist(mem[pp+1].hh
.lh);mem[pp+1].hh.lh:=curexp;curtype:=1;end;
end else begin if dp=1 then{1076:}begin dp:=p;
while dp<>0 do begin if mem[dp].hh.b0=2 then goto 32;dp:=mem[dp].hh.rh;
end;32:;end{:1076};
if dp<>0 then begin if mem[dp+6].hh.rh<>0 then if mem[mem[dp+6].hh.rh].
hh.lh=0 then tossedges(mem[dp+6].hh.rh)else decr(mem[mem[dp+6].hh.rh].hh
.lh);mem[dp+6].hh.rh:=makedashes(curexp);mem[dp+7].int:=65536;
curtype:=1;end;end;end;{1077:}if cp>1 then{1078:}begin q:=mem[cp].hh.rh;
while q<>0 do begin if(mem[q].hh.b0<4)then begin mem[q+2].int:=mem[cp+2]
.int;mem[q+3].int:=mem[cp+3].int;mem[q+4].int:=mem[cp+4].int;end;
q:=mem[q].hh.rh;end;end{:1078};if pp>1 then{1079:}
begin q:=mem[pp].hh.rh;
while q<>0 do begin if(mem[q].hh.b0<3)then begin if mem[q+1].hh.lh<>0
then tossknotlist(mem[q+1].hh.lh);
mem[q+1].hh.lh:=makepen(copypath(mem[pp+1].hh.lh),false);end;
q:=mem[q].hh.rh;end;end{:1079};if dp>1 then{1080:}
begin q:=mem[dp].hh.rh;
while q<>0 do begin if mem[q].hh.b0=2 then begin if mem[q+6].hh.rh<>0
then if mem[mem[q+6].hh.rh].hh.lh=0 then tossedges(mem[q+6].hh.rh)else
decr(mem[mem[q+6].hh.rh].hh.lh);mem[q+6].hh.rh:=mem[dp+6].hh.rh;
mem[q+7].int:=65536;
if mem[q+6].hh.rh<>0 then incr(mem[mem[q+6].hh.rh].hh.lh);end;
q:=mem[q].hh.rh;end;end{:1080}{:1077};end;{:1071}{1081:}
function findedgesvar(t:halfword):halfword;var p:halfword;
curedges:halfword;begin p:=findvariable(t);curedges:=0;
if p=0 then begin obliterated(t);putgeterror;
end else if mem[p].hh.b0<>10 then begin begin if interaction=3 then;
printnl(262);print(841);end;showtokenlist(t,0,1000,0);print(1017);
printtype(mem[p].hh.b0);printchar(41);begin helpptr:=2;
helpline[1]:=1018;helpline[0]:=1019;end;putgeterror;
end else begin mem[p+1].int:=privateedges(mem[p+1].int);
curedges:=mem[p+1].int;end;flushnodelist(t);findedgesvar:=curedges;end;
{:1081}{1086:}function startdrawcmd(sep:quarterword):halfword;
var lhv:halfword;addtype:quarterword;begin lhv:=0;getxnext;varflag:=sep;
scanprimary;if curtype<>20 then{1087:}begin disperr(0,1022);
begin helpptr:=4;helpline[3]:=1023;helpline[2]:=1024;helpline[1]:=1025;
helpline[0]:=1019;end;putgetflusherror(0);end{:1087}
else begin lhv:=curexp;addtype:=curmod;curtype:=1;getxnext;
scanexpression;end;lastaddtype:=addtype;startdrawcmd:=lhv;end;{:1086}
{1088:}procedure dobounds;var lhv,lhe:halfword;p:halfword;m:integer;
begin m:=curmod;lhv:=startdrawcmd(71);
if lhv<>0 then begin lhe:=findedgesvar(lhv);
if lhe=0 then flushcurexp(0)else if curtype<>8 then begin disperr(0,1026
);begin helpptr:=2;helpline[1]:=1027;helpline[0]:=1019;end;
putgetflusherror(0);end else if mem[curexp].hh.b0=0 then{1089:}
begin begin if interaction=3 then;printnl(262);print(1028);end;
begin helpptr:=2;helpline[1]:=1029;helpline[0]:=1019;end;putgeterror;
end{:1089}else{1090:}begin p:=newboundsnode(curexp,m);
mem[p].hh.rh:=mem[lhe+7].hh.rh;mem[lhe+7].hh.rh:=p;
if mem[lhe+7].hh.lh=lhe+7 then mem[lhe+7].hh.lh:=p;
p:=getnode(grobjectsize[(m+2)]);mem[p].hh.b0:=(m+2);
mem[mem[lhe+7].hh.lh].hh.rh:=p;mem[lhe+7].hh.lh:=p;initbbox(lhe);
end{:1090};end;end;{:1088}{1091:}procedure doaddto;var lhv,lhe:halfword;
p:halfword;e:halfword;addtype:quarterword;begin lhv:=startdrawcmd(69);
addtype:=lastaddtype;if lhv<>0 then begin if addtype=2 then{1093:}
begin p:=0;e:=0;if curtype<>10 then begin disperr(0,1030);
begin helpptr:=2;helpline[1]:=1031;helpline[0]:=1019;end;
putgetflusherror(0);end else begin e:=privateedges(curexp);curtype:=1;
p:=mem[e+7].hh.rh;end;end{:1093}else{1094:}begin e:=0;p:=0;
if curtype=14 then pairtopath;if curtype<>8 then begin disperr(0,1030);
begin helpptr:=2;helpline[1]:=1027;helpline[0]:=1019;end;
putgetflusherror(0);
end else if addtype=1 then if mem[curexp].hh.b0=0 then{1089:}
begin begin if interaction=3 then;printnl(262);print(1028);end;
begin helpptr:=2;helpline[1]:=1029;helpline[0]:=1019;end;putgeterror;
end{:1089}else begin p:=newfillnode(curexp);curtype:=1;
end else begin p:=newstrokednode(curexp);curtype:=1;end;end{:1094};
scanwithlist(p);{1095:}lhe:=findedgesvar(lhv);
if lhe=0 then begin if(e=0)and(p<>0)then e:=tossgrobject(p);
if e<>0 then if mem[e].hh.lh=0 then tossedges(e)else decr(mem[e].hh.lh);
end else if addtype=2 then if e<>0 then{1096:}
begin if mem[e+7].hh.rh<>0 then begin mem[mem[lhe+7].hh.lh].hh.rh:=mem[e
+7].hh.rh;mem[lhe+7].hh.lh:=mem[e+7].hh.lh;mem[e+7].hh.lh:=e+7;
mem[e+7].hh.rh:=0;flushdashlist(lhe);end;tossedges(e);end{:1096}
else else if p<>0 then begin mem[mem[lhe+7].hh.lh].hh.rh:=p;
mem[lhe+7].hh.lh:=p;
if addtype=0 then if mem[p+1].hh.lh=0 then mem[p+1].hh.lh:=getpencircle(
0);end{:1095};end;end;{:1091}{1098:}{1129:}
function tfmcheck(m:smallnumber):scaled;
begin if abs(internal[m])>=134217728 then begin begin if interaction=3
then;printnl(262);print(1048);end;print(intname[m]);print(1049);
begin helpptr:=1;helpline[0]:=1050;end;putgeterror;
if internal[m]>0 then tfmcheck:=134217727 else tfmcheck:=-134217727;
end else tfmcheck:=internal[m];end;{:1129}{1195:}
procedure readpsnametable;label 50,30;var k:fontnumber;lmax:integer;
j:integer;c:ASCIIcode;s:strnumber;begin namelength:=strlen(pstabname);
nameoffile:=xmalloc(1+namelength+1);strcpy(nameoffile+1,pstabname);
if aopenin(pstabfile,kpsedvipsconfigformat)then begin{1197:}lmax:=0;
for k:=lastpsfnum+1 to lastfnum do if(strstart[nextstr[fontname[k]]]-
strstart[fontname[k]])>lmax then lmax:=(strstart[nextstr[fontname[k]]]-
strstart[fontname[k]]){:1197};while not eof(pstabfile)do begin{1198:}
begin if poolptr+lmax>maxpoolptr then if poolptr+lmax>poolsize then
docompaction(lmax)else maxpoolptr:=poolptr+lmax;end;j:=lmax;
while true do begin if eoln(pstabfile)then if j=lmax then begin poolptr
:=strstart[strptr];goto 50;end else fatalerror(1113);read(pstabfile,c);
if((c='%')or(c='*')or(c=';')or(c='#'))then begin poolptr:=strstart[
strptr];goto 50;end;if((c=' ')or(c=9))then goto 30;decr(j);
if j>=0 then begin strpool[poolptr]:=xord[c];incr(poolptr);
end else begin poolptr:=strstart[strptr];goto 50;end;end;
30:s:=makestring{:1198};
for k:=lastpsfnum+1 to lastfnum do if strvsstr(s,fontname[k])=0 then{
1199:}begin flushstring(s);j:=32;
begin if poolptr+j>maxpoolptr then if poolptr+j>poolsize then
docompaction(j)else maxpoolptr:=poolptr+j;end;
repeat if eoln(pstabfile)then fatalerror(1113);read(pstabfile,c);
until((c<>' ')and(c<>9));repeat decr(j);if j<0 then fatalerror(1113);
begin strpool[poolptr]:=xord[c];incr(poolptr);end;
if eoln(pstabfile)then c:=' 'else read(pstabfile,c);
until((c=' ')or(c=9));
begin if strref[fontpsname[k]]<127 then if strref[fontpsname[k]]>1 then
decr(strref[fontpsname[k]])else flushstring(fontpsname[k]);end;
fontpsname[k]:=makestring;goto 50;end{:1199};flushstring(s);
50:readln(pstabfile);end;lastpsfnum:=lastfnum;aclose(pstabfile);end;end;
{:1195}{1201:}procedure openoutputfile;var c:integer;oldsetting:0..10;
s:strnumber;begin if jobname=0 then openlogfile;
c:=roundunscaled(internal[17]);if c<0 then s:=1114 else{1202:}
begin oldsetting:=selector;selector:=4;printchar(46);printint(c);
s:=makestring;selector:=oldsetting;end{:1202};packjobname(s);
while not aopenout(psfile)do promptfilename(1115,s);
begin if strref[s]<127 then if strref[s]>1 then decr(strref[s])else
flushstring(s);end;{1203:}
if(c<firstoutputcode)and(firstoutputcode>=0)then begin firstoutputcode:=
c;
begin if strref[firstfilename]<127 then if strref[firstfilename]>1 then
decr(strref[firstfilename])else flushstring(firstfilename);end;
firstfilename:=amakenamestring(psfile);end;
if c>=lastoutputcode then begin lastoutputcode:=c;
begin if strref[lastfilename]<127 then if strref[lastfilename]>1 then
decr(strref[lastfilename])else flushstring(lastfilename);end;
lastfilename:=amakenamestring(psfile);end{:1203};{1206:}
if termoffset>maxprintline-6 then println else if(termoffset>0)or(
fileoffset>0)then printchar(32);printchar(91);
if c>=0 then printint(c){:1206};end;{:1201}{1209:}
procedure pspairout(x,y:scaled);
begin if psoffset+26>maxprintline then println;printscaled(x);
printchar(32);printscaled(y);printchar(32)end;{:1209}{1210:}
procedure psprint(s:strnumber);
begin if psoffset+(strstart[nextstr[s]]-strstart[s])>maxprintline then
println;print(s);end;{:1210}{1211:}procedure pspathout(h:halfword);
label 10;var p,q:halfword;d:scaled;curved:boolean;
begin if psoffset+40>maxprintline then println;
if neednewpath then print(1118);neednewpath:=true;
pspairout(mem[h+1].int,mem[h+2].int);print(1119);p:=h;
repeat if mem[p].hh.b1=0 then begin if p=h then psprint(1120);goto 10;
end;q:=mem[p].hh.rh;{1213:}curved:=true;{1214:}
if mem[p+5].int=mem[p+1].int then if mem[p+6].int=mem[p+2].int then if
mem[q+3].int=mem[q+1].int then if mem[q+4].int=mem[q+2].int then curved
:=false;d:=mem[q+3].int-mem[p+5].int;
if abs(mem[p+5].int-mem[p+1].int-d)<=131 then if abs(mem[q+1].int-mem[q
+3].int-d)<=131 then begin d:=mem[q+4].int-mem[p+6].int;
if abs(mem[p+6].int-mem[p+2].int-d)<=131 then if abs(mem[q+2].int-mem[q
+4].int-d)<=131 then curved:=false;end{:1214};println;
if curved then begin pspairout(mem[p+5].int,mem[p+6].int);
pspairout(mem[q+3].int,mem[q+4].int);
pspairout(mem[q+1].int,mem[q+2].int);psprint(1122);
end else if q<>h then begin pspairout(mem[q+1].int,mem[q+2].int);
psprint(1123);end{:1213};p:=q;until p=h;psprint(1121);10:end;{:1211}
{1216:}procedure unknowngraphicsstate(c:scaled);begin gsred:=c;
gsgreen:=c;gsblue:=c;gsljoin:=3;gslcap:=3;gsmiterlim:=0;gsdashp:=1;
gsdashsc:=0;gswidth:=-1;end;{:1216}{1217:}{1224:}
function coordrangeOK(h:halfword;zoff:smallnumber;dz:scaled):boolean;
label 40,45,10;var p:halfword;zlo,zhi:scaled;z:scaled;
begin zlo:=mem[h+zoff].int;zhi:=zlo;p:=h;
while mem[p].hh.b1<>0 do begin z:=mem[p+zoff+4].int;{1225:}
if z<zlo then zlo:=z else if z>zhi then zhi:=z;
if zhi-zlo>dz then goto 40{:1225};p:=mem[p].hh.rh;z:=mem[p+zoff+2].int;
{1225:}if z<zlo then zlo:=z else if z>zhi then zhi:=z;
if zhi-zlo>dz then goto 40{:1225};z:=mem[p+zoff].int;{1225:}
if z<zlo then zlo:=z else if z>zhi then zhi:=z;
if zhi-zlo>dz then goto 40{:1225};if p=h then goto 45;end;
45:coordrangeOK:=true;goto 10;40:coordrangeOK:=false;10:end;{:1224}
{1228:}function samedashes(h,hh:halfword):boolean;label 30;
var p,pp:halfword;
begin if h=hh then samedashes:=true else if(h<=1)or(hh<=1)then
samedashes:=false else if mem[h+1].int<>mem[hh+1].int then samedashes:=
false else{1229:}begin p:=mem[h].hh.rh;pp:=mem[hh].hh.rh;
while(p<>2)and(pp<>2)do if(mem[p+1].int<>mem[pp+1].int)or(mem[p+2].int<>
mem[pp+2].int)then goto 30 else begin p:=mem[p].hh.rh;pp:=mem[pp].hh.rh;
end;30:samedashes:=p=pp;end{:1229};end;{:1228}
procedure fixgraphicsstate(p:halfword);var hh,pp:halfword;
wx,wy,ww:scaled;adjwx:boolean;tx,ty:integer;scf:scaled;
begin if(mem[p].hh.b0<4)then{1220:}
if(gsred<>mem[p+2].int)or(gsgreen<>mem[p+3].int)or(gsblue<>mem[p+4].int)
then begin gsred:=mem[p+2].int;gsgreen:=mem[p+3].int;
gsblue:=mem[p+4].int;
if(gsred=gsgreen)and(gsgreen=gsblue)then begin if psoffset+16>
maxprintline then println;printchar(32);printscaled(gsred);print(1127);
end else begin if psoffset+36>maxprintline then println;printchar(32);
printscaled(gsred);printchar(32);printscaled(gsgreen);printchar(32);
printscaled(gsblue);print(1128);end;end;{:1220};
if(mem[p].hh.b0=1)or(mem[p].hh.b0=2)then if mem[p+1].hh.lh<>0 then if(
mem[p+1].hh.lh=mem[mem[p+1].hh.lh].hh.rh)then begin{1221:}{1222:}
pp:=mem[p+1].hh.lh;
if(mem[pp+5].int=mem[pp+1].int)and(mem[pp+4].int=mem[pp+2].int)then
begin wx:=abs(mem[pp+3].int-mem[pp+1].int);
wy:=abs(mem[pp+6].int-mem[pp+2].int);
end else begin wx:=pythadd(mem[pp+3].int-mem[pp+1].int,mem[pp+5].int-mem
[pp+1].int);
wy:=pythadd(mem[pp+4].int-mem[pp+2].int,mem[pp+6].int-mem[pp+2].int);
end{:1222};{1223:}tx:=1;ty:=1;
if coordrangeOK(mem[p+1].hh.rh,2,wy)then tx:=10 else if coordrangeOK(mem
[p+1].hh.rh,1,wx)then ty:=10;if wy div ty>=wx div tx then begin ww:=wy;
adjwx:=false;end else begin ww:=wx;adjwx:=true;end{:1223};
if(ww<>gswidth)or(adjwx<>gsadjwx)then begin if adjwx then begin if
psoffset+13>maxprintline then println;printchar(32);printscaled(ww);
psprint(1129);end else begin if psoffset+15>maxprintline then println;
print(1130);printscaled(ww);psprint(1131);end;gswidth:=ww;
gsadjwx:=adjwx;end{:1221};{1226:}
if mem[p].hh.b0=1 then hh:=0 else begin hh:=mem[p+6].hh.rh;
scf:=getpenscale(mem[p+1].hh.lh);
if scf=0 then if gswidth=0 then scf:=mem[p+7].int else hh:=0 else begin
scf:=makescaled(gswidth,scf);scf:=takescaled(scf,mem[p+7].int);end;end;
if hh=0 then begin if gsdashp<>0 then begin psprint(1132);gsdashp:=0;
end;end else if(gsdashsc<>scf)or not samedashes(gsdashp,hh)then{1227:}
begin gsdashp:=hh;gsdashsc:=scf;
if(mem[hh+1].int=0)or(abs(mem[hh+1].int)div 65536>=2147483647 div scf)
then psprint(1132)else begin pp:=mem[hh].hh.rh;
mem[3].int:=mem[pp+1].int+mem[hh+1].int;
if psoffset+28>maxprintline then println;print(1133);
while pp<>2 do begin pspairout(takescaled(mem[pp+2].int-mem[pp+1].int,
scf),takescaled(mem[mem[pp].hh.rh+1].int-mem[pp+2].int,scf));
pp:=mem[pp].hh.rh;end;if psoffset+22>maxprintline then println;
print(1134);printscaled(takescaled(dashoffset(hh),scf));print(1135);end;
end{:1227}{:1226};{1218:}
if mem[p].hh.b0=2 then if(mem[mem[p+1].hh.rh].hh.b0=0)or(mem[p+6].hh.rh
<>0)then if gslcap<>mem[p+6].hh.b0 then begin if psoffset+13>
maxprintline then println;printchar(32);printchar(48+mem[p+6].hh.b0);
print(1124);gslcap:=mem[p+6].hh.b0;end{:1218};{1219:}
if gsljoin<>mem[p].hh.b1 then begin if psoffset+14>maxprintline then
println;printchar(32);printchar(48+mem[p].hh.b1);print(1125);
gsljoin:=mem[p].hh.b1;end;
if gsmiterlim<>mem[p+5].int then begin if psoffset+27>maxprintline then
println;printchar(32);printscaled(mem[p+5].int);print(1126);
gsmiterlim:=mem[p+5].int;end{:1219};end;if psoffset>0 then println;end;
{:1217}{1230:}procedure strokeellipse(h:halfword;fillalso:boolean);
var txx,txy,tyx,tyy:scaled;p:halfword;d1,det:scaled;s:integer;
transformed:boolean;begin transformed:=false;{1231:}p:=mem[h+1].hh.lh;
txx:=mem[p+3].int;tyx:=mem[p+4].int;txy:=mem[p+5].int;tyy:=mem[p+6].int;
if(mem[p+1].int<>0)or(mem[p+2].int<>0)then begin printnl(1139);
pspairout(mem[p+1].int,mem[p+2].int);psprint(1140);
txx:=txx-mem[p+1].int;tyx:=tyx-mem[p+2].int;txy:=txy-mem[p+1].int;
tyy:=tyy-mem[p+2].int;transformed:=true;end else printnl(284);{1232:}
if gswidth<>65536 then if gswidth=0 then begin txx:=65536;tyy:=65536;
end else begin txx:=makescaled(txx,gswidth);
txy:=makescaled(txy,gswidth);tyx:=makescaled(tyx,gswidth);
tyy:=makescaled(tyy,gswidth);end;
if(txy<>0)or(tyx<>0)or(txx<>65536)or(tyy<>65536)then if(not transformed)
then begin psprint(1139);transformed:=true;end{:1232}{:1231};{1234:}
det:=takescaled(txx,tyy)-takescaled(txy,tyx);d1:=4*10+1;
if abs(det)<d1 then begin if det>=0 then begin d1:=d1-det;s:=1;
end else begin d1:=-d1-det;s:=-1;end;d1:=d1*65536;
if abs(txx)+abs(tyy)>=abs(txy)+abs(tyy)then if abs(txx)>abs(tyy)then tyy
:=tyy+(d1+s*abs(txx))div txx else txx:=txx+(d1+s*abs(tyy))div tyy else
if abs(txy)>abs(tyx)then tyx:=tyx+(d1+s*abs(txy))div txy else txy:=txy+(
d1+s*abs(tyx))div tyx;end{:1234};pspathout(mem[h+1].hh.rh);
if fillalso then printnl(1136);{1233:}
if(txy<>0)or(tyx<>0)then begin println;printchar(91);pspairout(txx,tyx);
pspairout(txy,tyy);psprint(1141);
end else if(txx<>65536)or(tyy<>65536)then begin println;
pspairout(txx,tyy);print(1142);end{:1233};psprint(1137);
if transformed then psprint(1138);println;end;{:1230}{1235:}
procedure psfillout(p:halfword);begin pspathout(p);psprint(1143);
println;end;{:1235}{1236:}procedure doouterenvelope(p,h:halfword);
begin p:=makeenvelope(p,mem[h+1].hh.lh,mem[h].hh.b1,0,mem[h+5].int);
psfillout(p);tossknotlist(p);end;{:1236}{1237:}
function choosescale(p:halfword):scaled;var a,b,c,d,ad,bc:scaled;
begin a:=mem[p+10].int;b:=mem[p+11].int;c:=mem[p+12].int;
d:=mem[p+13].int;if(a<0)then a:=-a;if(b<0)then b:=-b;if(c<0)then c:=-c;
if(d<0)then d:=-d;ad:=half(a-d);bc:=half(b-c);
choosescale:=pythadd(pythadd(d+ad,ad),pythadd(c+bc,bc));end;{:1237}
{1238:}procedure psstringout(s:strnumber);var i:poolpointer;k:ASCIIcode;
begin print(40);i:=strstart[s];
while i<strstart[nextstr[s]]do begin if psoffset+5>maxprintline then
begin printchar(92);println;end;k:=strpool[i];if({64:}
(k<32)or(k>126){:64})then begin printchar(92);printchar(48+(k div 64));
printchar(48+((k div 8)mod 8));printchar(48+(k mod 8));
end else begin if(k=40)or(k=41)or(k=92)then printchar(92);printchar(k);
end;incr(i);end;print(41);end;{:1238}{1239:}
function ispsname(s:strnumber):boolean;label 45,10;var i:poolpointer;
k:ASCIIcode;begin i:=strstart[s];
while i<strstart[nextstr[s]]do begin k:=strpool[i];
if(k<=32)or(k>126)then goto 45;
if(k=40)or(k=41)or(k=60)or(k=62)or(k=123)or(k=125)or(k=47)or(k=37)then
goto 45;incr(i);end;ispsname:=true;goto 10;45:ispsname:=false;10:end;
{:1239}{1240:}procedure psnameout(s:strnumber;lit:boolean);
begin if psoffset+(strstart[nextstr[s]]-strstart[s])+2>maxprintline then
println;printchar(32);
if ispsname(s)then begin if lit then printchar(47);print(s);
end else begin psstringout(s);if not lit then psprint(1144);
psprint(1145);end;end;{:1240}{1242:}procedure unmarkfont(f:fontnumber);
var k:0..fontmemsize;
begin for k:=charbase[f]+fontbc[f]to charbase[f]+fontec[f]do fontinfo[k]
.qqqq.b3:=0;end;{:1242}{1243:}procedure markstringchars(f:fontnumber;
s:strnumber);var b:integer;bc,ec:poolASCIIcode;k:poolpointer;
begin b:=charbase[f];bc:=fontbc[f];ec:=fontec[f];
k:=strstart[nextstr[s]];while k>strstart[s]do begin decr(k);
if(strpool[k]>=bc)and(strpool[k]<=ec)then fontinfo[b+strpool[k]].qqqq.b3
:=1;end end;{:1243}{1244:}procedure hexdigitout(d:smallnumber);
begin if d<10 then printchar(d+48)else printchar(d+87);end;{:1244}
{1245:}function psmarksout(f:fontnumber;c:eightbits):halfword;
var bc,ec:eightbits;lim:integer;p:0..fontmemsize;d,b:0..15;
begin lim:=4*(emergencylinelength-psoffset-4);bc:=fontbc[f];
ec:=fontec[f];if c>bc then bc:=c;{1246:}p:=charbase[f]+bc;
while(fontinfo[p].qqqq.b3=0)and(bc<ec)do begin incr(p);incr(bc);end;
if ec>=bc+lim then ec:=bc+lim-1;p:=charbase[f]+ec;
while(fontinfo[p].qqqq.b3=0)and(bc<ec)do begin decr(p);decr(ec);end;
{:1246};{1247:}printchar(32);hexdigitout(bc div 16);
hexdigitout(bc mod 16);printchar(58){:1247};{1248:}b:=8;d:=0;
for p:=charbase[f]+bc to charbase[f]+ec do begin if b=0 then begin
hexdigitout(d);d:=0;b:=8;end;if fontinfo[p].qqqq.b3<>0 then d:=d+b;
b:=halfp(b);end;hexdigitout(d){:1248};
while(ec<fontec[f])and(fontinfo[p].qqqq.b3=0)do begin incr(p);incr(ec);
end;psmarksout:=ec+1;end;{:1245}{1249:}
function checkpsmarks(f:fontnumber;c:integer):boolean;label 10;
var p:0..fontmemsize;
begin for p:=charbase[f]+c to charbase[f]+fontec[f]do if fontinfo[p].
qqqq.b3=1 then begin checkpsmarks:=true;goto 10;end;checkpsmarks:=false;
10:end;{:1249}{1251:}function sizeindex(f:fontnumber;
s:scaled):quarterword;label 40;var p,q:halfword;i:quarterword;
begin q:=fontsizes[f];i:=0;
while q<>0 do begin if abs(s-mem[q+1].int)<=65 then goto 40 else begin p
:=q;q:=mem[q].hh.rh;incr(i);end;if i=255 then overflow(1146,255);end;
q:=getnode(2);mem[q+1].int:=s;
if i=0 then fontsizes[f]:=q else mem[p].hh.rh:=q;40:sizeindex:=i;end;
{:1251}{1252:}function indexedsize(f:fontnumber;j:quarterword):scaled;
var p:halfword;i:quarterword;begin p:=fontsizes[f];i:=0;
if p=0 then confusion(1147);while(i<>j)do begin incr(i);p:=mem[p].hh.rh;
if p=0 then confusion(1147);end;indexedsize:=mem[p+1].int;end;{:1252}
{1253:}procedure clearsizes;var f:fontnumber;p:halfword;
begin for f:=1 to lastfnum do while fontsizes[f]<>0 do begin p:=
fontsizes[f];fontsizes[f]:=mem[p].hh.rh;freenode(p,2);end;end;{:1253}
{1260:}procedure shipout(h:halfword);label 30,40;var p:halfword;
q:halfword;t:integer;f,ff:fontnumber;ldf:fontnumber;donefonts:boolean;
nextsize:quarterword;curfsize:array[fontnumber]of halfword;
ds,scf:scaled;transformed:boolean;begin openoutputfile;
if(internal[32]>0)and(lastpsfnum<lastfnum)then readpsnametable;
nonpssetting:=selector;selector:=5;{1261:}print(1156);
if internal[32]>0 then print(1157);printnl(1158);setbbox(h,true);
if mem[h+2].int>mem[h+4].int then print(1159)else if internal[32]<0 then
begin pspairout(mem[h+2].int,mem[h+3].int);
pspairout(mem[h+4].int,mem[h+5].int);
end else begin pspairout(floorscaled(mem[h+2].int),floorscaled(mem[h+3].
int));
pspairout(-floorscaled(-mem[h+4].int),-floorscaled(-mem[h+5].int));end;
printnl(1160);printnl(1161);printint(roundunscaled(internal[13]));
printchar(46);printdd(roundunscaled(internal[14]));printchar(46);
printdd(roundunscaled(internal[15]));printchar(58);
t:=roundunscaled(internal[16]);printdd(t div 60);printdd(t mod 60);
printnl(1162);{1262:}{1265:}for f:=1 to lastfnum do fontsizes[f]:=0;
p:=mem[h+7].hh.rh;
while p<>0 do begin if mem[p].hh.b0=3 then if mem[p+1].hh.lh<>0 then
begin f:=mem[p+1].hh.lh;
if internal[32]>0 then fontsizes[f]:=1 else begin if fontsizes[f]=0 then
unmarkfont(f);mem[p].hh.b1:=sizeindex(f,choosescale(p));
if mem[p].hh.b1=0 then markstringchars(f,mem[p+1].hh.rh);end;end;
p:=mem[p].hh.rh;end{:1265};if internal[32]>0 then{1264:}begin ldf:=0;
for f:=1 to lastfnum do if fontsizes[f]<>0 then begin if ldf=0 then
printnl(1163);
for ff:=ldf downto 0 do if fontsizes[ff]<>0 then if strvsstr(fontpsname[
f],fontpsname[ff])=0 then goto 40;
if psoffset+1+(strstart[nextstr[fontpsname[f]]]-strstart[fontpsname[f]])
>maxprintline then printnl(1164);printchar(32);print(fontpsname[f]);
ldf:=f;40:end;end{:1264}else begin nextsize:=0;{1263:}
for f:=1 to lastfnum do curfsize[f]:=fontsizes[f]{:1263};
repeat donefonts:=true;
for f:=1 to lastfnum do begin if curfsize[f]<>0 then{1266:}begin t:=0;
while checkpsmarks(f,t)do begin printnl(1165);
if psoffset+(strstart[nextstr[fontname[f]]]-strstart[fontname[f]])+12>
emergencylinelength then goto 30;print(fontname[f]);printchar(32);
ds:=(fontdsize[f]+8)div 16;
printscaled(takescaled(ds,mem[curfsize[f]+1].int));
if psoffset+12>emergencylinelength then goto 30;printchar(32);
printscaled(ds);if psoffset+5>emergencylinelength then goto 30;
t:=psmarksout(f,t);end;30:curfsize[f]:=mem[curfsize[f]].hh.rh;end{:1266}
;if curfsize[f]<>0 then begin unmarkfont(f);donefonts:=false;end;end;
if not donefonts then{1267:}begin incr(nextsize);p:=mem[h+7].hh.rh;
while p<>0 do begin if mem[p].hh.b0=3 then if mem[p+1].hh.lh<>0 then if
mem[p].hh.b1=nextsize then markstringchars(mem[p+1].hh.lh,mem[p+1].hh.rh
);p:=mem[p].hh.rh;end;end{:1267};until donefonts;end{:1262};
println{:1261};if internal[32]>0 then{1268:}
begin if ldf<>0 then begin for f:=1 to lastfnum do if fontsizes[f]<>0
then begin psnameout(fontname[f],true);psnameout(fontpsname[f],true);
psprint(1166);println;end;print(1167);println;end;end{:1268};
print(1151);printnl(1152);println;{1259:}t:=mem[memtop-3].hh.rh;
while t<>0 do begin if(strstart[nextstr[mem[t+1].int]]-strstart[mem[t+1]
.int])<=emergencylinelength then print(mem[t+1].int)else overflow(1150,
emergencylinelength);println;t:=mem[t].hh.rh;end;
flushtokenlist(mem[memtop-3].hh.rh);mem[memtop-3].hh.rh:=0;
lastpending:=memtop-3{:1259};unknowngraphicsstate(0);neednewpath:=true;
p:=mem[h+7].hh.rh;while p<>0 do begin fixgraphicsstate(p);
case mem[p].hh.b0 of{1269:}4:begin printnl(1139);
pspathout(mem[p+1].hh.rh);psprint(1168);println;end;
6:begin printnl(1169);println;unknowngraphicsstate(-1);end;{:1269}
{1270:}
1:if mem[p+1].hh.lh=0 then psfillout(mem[p+1].hh.rh)else if(mem[p+1].hh.
lh=mem[mem[p+1].hh.lh].hh.rh)then strokeellipse(p,true)else begin
doouterenvelope(copypath(mem[p+1].hh.rh),p);
doouterenvelope(htapypoc(mem[p+1].hh.rh),p);end;
2:if(mem[p+1].hh.lh=mem[mem[p+1].hh.lh].hh.rh)then strokeellipse(p,false
)else begin q:=copypath(mem[p+1].hh.rh);t:=mem[p+6].hh.b0;{1271:}
if mem[q].hh.b0<>0 then begin mem[insertknot(q,mem[q+1].int,mem[q+2].int
)].hh.b0:=0;mem[q].hh.b1:=0;q:=mem[q].hh.rh;t:=1;end{:1271};
q:=makeenvelope(q,mem[p+1].hh.lh,mem[p].hh.b1,t,mem[p+5].int);
psfillout(q);tossknotlist(q);end;{:1270}{1272:}
3:if(mem[p+1].hh.lh<>0)and((strstart[nextstr[mem[p+1].hh.rh]]-strstart[
mem[p+1].hh.rh])>0)then begin if internal[32]>0 then scf:=choosescale(p)
else scf:=indexedsize(mem[p+1].hh.lh,mem[p].hh.b1);{1274:}
transformed:=(mem[p+10].int<>scf)or(mem[p+13].int<>scf)or(mem[p+11].int
<>0)or(mem[p+12].int<>0);if transformed then begin print(1171);
pspairout(makescaled(mem[p+10].int,scf),makescaled(mem[p+12].int,scf));
pspairout(makescaled(mem[p+11].int,scf),makescaled(mem[p+13].int,scf));
pspairout(mem[p+8].int,mem[p+9].int);psprint(1172);
end else begin pspairout(mem[p+8].int,mem[p+9].int);psprint(1119);end;
println{:1274};psstringout(mem[p+1].hh.rh);
psnameout(fontname[mem[p+1].hh.lh],false);{1273:}
if psoffset+18>maxprintline then println;printchar(32);
ds:=(fontdsize[mem[p+1].hh.lh]+8)div 16;printscaled(takescaled(ds,scf));
print(1170);if transformed then psprint(1138){:1273};println;end;{:1272}
5,7:;end;p:=mem[p].hh.rh;end;print(1153);println;print(1154);println;
aclose(psfile);selector:=nonpssetting;
if internal[32]<=0 then clearsizes;{1207:}printchar(93);fflush(stdout);
incr(totalshipped){:1207};if internal[9]>0 then printedges(h,1155,true);
end;{:1260}procedure doshipout;var c:integer;begin getxnext;
scanexpression;if curtype<>10 then{1099:}begin disperr(0,1032);
begin helpptr:=1;helpline[0]:=1033;end;putgetflusherror(0);end{:1099}
else begin c:=roundunscaled(internal[17])mod 256;if c<0 then c:=c+256;
{1130:}if c<bc then bc:=c;if c>ec then ec:=c;charexists[c]:=true;
tfmwidth[c]:=tfmcheck(19);tfmheight[c]:=tfmcheck(20);
tfmdepth[c]:=tfmcheck(21);tfmitalcorr[c]:=tfmcheck(22){:1130};
shipout(curexp);flushcurexp(0);end;end;{:1098}{1106:}{1107:}
procedure nostringerr(s:strnumber);begin disperr(0,740);
begin helpptr:=1;helpline[0]:=s;end;putgeterror;end;{:1107}
procedure domessage;var m:0..2;begin m:=curmod;getxnext;scanexpression;
if curtype<>4 then nostringerr(1037)else case m of 0:begin printnl(284);
print(curexp);end;1:{1111:}begin begin if interaction=3 then;
printnl(262);print(284);end;print(curexp);
if errhelp<>0 then useerrhelp:=true else if longhelpseen then begin
helpptr:=1;helpline[0]:=1038;
end else begin if interaction<3 then longhelpseen:=true;
begin helpptr:=4;helpline[3]:=1039;helpline[2]:=1040;helpline[1]:=1041;
helpline[0]:=1042;end;end;putgeterror;useerrhelp:=false;end{:1111};
2:{1108:}
begin if errhelp<>0 then begin if strref[errhelp]<127 then if strref[
errhelp]>1 then decr(strref[errhelp])else flushstring(errhelp);end;
if(strstart[nextstr[curexp]]-strstart[curexp])=0 then errhelp:=0 else
begin errhelp:=curexp;
begin if strref[errhelp]<127 then incr(strref[errhelp]);end;end;
end{:1108};end;flushcurexp(0);end;{:1106}{1113:}procedure dowrite;
label 22;var t:strnumber;n,n0:writeindex;oldsetting:0..10;
begin getxnext;scanexpression;
if curtype<>4 then nostringerr(1043)else if curcmd<>71 then begin begin
if interaction=3 then;printnl(262);print(1044);end;begin helpptr:=1;
helpline[0]:=1045;end;putgeterror;end else begin t:=curexp;curtype:=1;
getxnext;scanexpression;if curtype<>4 then nostringerr(1046)else{1114:}
begin{1115:}n:=writefiles;n0:=writefiles;repeat 22:if n=0 then{1116:}
begin if n0=writefiles then if writefiles<4 then incr(writefiles)else
overflow(1047,4);n:=n0;openwritefile(curexp,n);end{:1116}
else begin decr(n);if wrfname[n]=0 then begin n0:=n;goto 22;end;end;
until strvsstr(curexp,wrfname[n])=0{:1115};{929:}
if eofline=0 then begin begin strpool[poolptr]:=0;incr(poolptr);end;
eofline:=makestring;strref[eofline]:=127;end{:929};
if strvsstr(t,eofline)=0 then{1117:}begin aclose(wrfile[n]);
begin if strref[wrfname[n]]<127 then if strref[wrfname[n]]>1 then decr(
strref[wrfname[n]])else flushstring(wrfname[n]);end;wrfname[n]:=0;
if n=writefiles-1 then writefiles:=n;end{:1117}
else begin oldsetting:=selector;selector:=n;print(t);println;
selector:=oldsetting;end;end{:1114};
begin if strref[t]<127 then if strref[t]>1 then decr(strref[t])else
flushstring(t);end;end;flushcurexp(0);end;{:1113}{1134:}
function getcode:eightbits;label 40;var c:integer;begin getxnext;
scanexpression;if curtype=16 then begin c:=roundunscaled(curexp);
if c>=0 then if c<256 then goto 40;
end else if curtype=4 then if(strstart[nextstr[curexp]]-strstart[curexp]
)=1 then begin c:=strpool[strstart[curexp]];goto 40;end;disperr(0,1056);
begin helpptr:=2;helpline[1]:=1057;helpline[0]:=1058;end;
putgetflusherror(0);c:=0;40:getcode:=c;end;{:1134}{1135:}
procedure settag(c:halfword;t:smallnumber;r:halfword);
begin if chartag[c]=0 then begin chartag[c]:=t;charremainder[c]:=r;
if t=1 then begin incr(labelptr);labelloc[labelptr]:=r;
labelchar[labelptr]:=c;end;end else{1136:}
begin begin if interaction=3 then;printnl(262);print(1059);end;
if(c>32)and(c<127)then print(c)else if c=256 then print(1060)else begin
print(1061);printint(c);end;print(1062);case chartag[c]of 1:print(1063);
2:print(1064);3:print(1053);end;begin helpptr:=2;helpline[1]:=1065;
helpline[0]:=1019;end;putgeterror;end{:1136};end;{:1135}{1137:}
procedure dotfmcommand;label 22,30;var c,cc:0..256;k:0..maxkerns;
j:integer;begin case curmod of 0:begin c:=getcode;
while curcmd=80 do begin cc:=getcode;settag(c,2,cc);c:=cc;end;end;
1:{1138:}begin lkstarted:=false;22:getxnext;
if(curcmd=77)and lkstarted then{1141:}begin c:=getcode;
if nl-skiptable[c]>128 then begin begin begin if interaction=3 then;
printnl(262);print(1082);end;begin helpptr:=1;helpline[0]:=1083;end;
error;ll:=skiptable[c];repeat lll:=ligkern[ll].b0;ligkern[ll].b0:=128;
ll:=ll-lll;until lll=0;end;skiptable[c]:=ligtablesize;end;
if skiptable[c]=ligtablesize then ligkern[nl-1].b0:=0 else ligkern[nl-1]
.b0:=nl-skiptable[c]-1;skiptable[c]:=nl-1;goto 30;end{:1141};
if curcmd=78 then begin c:=256;curcmd:=80;end else begin backinput;
c:=getcode;end;if(curcmd=80)or(curcmd=79)then{1142:}
begin if curcmd=80 then if c=256 then bchlabel:=nl else settag(c,1,nl)
else if skiptable[c]<ligtablesize then begin ll:=skiptable[c];
skiptable[c]:=ligtablesize;repeat lll:=ligkern[ll].b0;
if nl-ll>128 then begin begin begin if interaction=3 then;printnl(262);
print(1082);end;begin helpptr:=1;helpline[0]:=1083;end;error;ll:=ll;
repeat lll:=ligkern[ll].b0;ligkern[ll].b0:=128;ll:=ll-lll;until lll=0;
end;goto 22;end;ligkern[ll].b0:=nl-ll-1;ll:=ll-lll;until lll=0;end;
goto 22;end{:1142};if curcmd=75 then{1143:}begin ligkern[nl].b1:=c;
ligkern[nl].b0:=0;if curmod<128 then begin ligkern[nl].b2:=curmod;
ligkern[nl].b3:=getcode;end else begin getxnext;scanexpression;
if curtype<>16 then begin disperr(0,1084);begin helpptr:=2;
helpline[1]:=1085;helpline[0]:=308;end;putgetflusherror(0);end;
kern[nk]:=curexp;k:=0;while kern[k]<>curexp do incr(k);
if k=nk then begin if nk=maxkerns then overflow(1081,maxkerns);incr(nk);
end;ligkern[nl].b2:=128+(k div 256);ligkern[nl].b3:=(k mod 256);end;
lkstarted:=true;end{:1143}else begin begin if interaction=3 then;
printnl(262);print(1070);end;begin helpptr:=1;helpline[0]:=1071;end;
backerror;ligkern[nl].b1:=0;ligkern[nl].b2:=0;ligkern[nl].b3:=0;
ligkern[nl].b0:=129;end;
if nl=ligtablesize then overflow(1072,ligtablesize);incr(nl);
if curcmd=81 then goto 22;
if ligkern[nl-1].b0<128 then ligkern[nl-1].b0:=128;30:end{:1138};
2:{1144:}begin if ne=256 then overflow(1053,256);c:=getcode;
settag(c,3,ne);if curcmd<>80 then begin missingerr(58);begin helpptr:=1;
helpline[0]:=1086;end;backerror;end;exten[ne].b0:=getcode;
if curcmd<>81 then begin missingerr(44);begin helpptr:=1;
helpline[0]:=1086;end;backerror;end;exten[ne].b1:=getcode;
if curcmd<>81 then begin missingerr(44);begin helpptr:=1;
helpline[0]:=1086;end;backerror;end;exten[ne].b2:=getcode;
if curcmd<>81 then begin missingerr(44);begin helpptr:=1;
helpline[0]:=1086;end;backerror;end;exten[ne].b3:=getcode;incr(ne);
end{:1144};3,4:begin c:=curmod;getxnext;scanexpression;
if(curtype<>16)or(curexp<32768)then begin disperr(0,1066);
begin helpptr:=2;helpline[1]:=1067;helpline[0]:=1068;end;putgeterror;
end else begin j:=roundunscaled(curexp);
if curcmd<>80 then begin missingerr(58);begin helpptr:=1;
helpline[0]:=1069;end;backerror;end;if c=3 then{1145:}
repeat if j>headersize then overflow(1054,headersize);
headerbyte[j]:=getcode;incr(j);until curcmd<>81{:1145}else{1146:}
repeat if j>maxfontdimen then overflow(1055,maxfontdimen);
while j>np do begin incr(np);param[np]:=0;end;getxnext;scanexpression;
if curtype<>16 then begin disperr(0,1087);begin helpptr:=1;
helpline[0]:=308;end;putgetflusherror(0);end;param[j]:=curexp;incr(j);
until curcmd<>81{:1146};end;end;end;end;{:1137}{1257:}
procedure dospecial;begin getxnext;scanexpression;
if curtype<>4 then{1258:}begin disperr(0,1148);begin helpptr:=1;
helpline[0]:=1149;end;putgeterror;end{:1258}
else begin mem[lastpending].hh.rh:=stashcurexp;
lastpending:=mem[lastpending].hh.rh;mem[lastpending].hh.rh:=0;end;end;
{:1257}{1280:}ifdef('INIMP')procedure storememfile;label 30;
var k:integer;p,q:halfword;x:integer;w:fourquarters;s:strnumber;
begin{1294:}selector:=4;print(1178);print(jobname);printchar(32);
printint(roundunscaled(internal[13]));printchar(46);
printint(roundunscaled(internal[14]));printchar(46);
printint(roundunscaled(internal[15]));printchar(41);
if interaction=0 then selector:=9 else selector:=10;
begin if poolptr+1>maxpoolptr then if poolptr+1>poolsize then
docompaction(1)else maxpoolptr:=poolptr+1;end;memident:=makestring;
strref[memident]:=127;packjobname(784);
while not wopenout(memfile)do promptfilename(1179,784);printnl(1180);
s:=wmakenamestring(memfile);print(s);flushstring(s);
printnl(memident){:1294};{1284:}dumpint(136687108);dumpint(0);
dumpint(memtop);dumpint(9500);dumpint(7919);dumpint(15){:1284};{1286:}
docompaction(poolsize);dumpint(poolptr);dumpint(maxstrptr);
dumpint(strptr);k:=0;while(nextstr[k]=k+1)and(k<=maxstrptr)do incr(k);
dumpint(k);while k<=maxstrptr do begin dumpint(nextstr[k]);incr(k);end;
k:=0;while true do begin dumpint(strstart[k]);
if k=strptr then goto 30 else k:=nextstr[k];end;30:k:=0;
while k+4<poolptr do begin w.b0:=strpool[k];w.b1:=strpool[k+1];
w.b2:=strpool[k+2];w.b3:=strpool[k+3];dumpqqqq(w);k:=k+4;end;
k:=poolptr-4;w.b0:=strpool[k];w.b1:=strpool[k+1];w.b2:=strpool[k+2];
w.b3:=strpool[k+3];dumpqqqq(w);println;print(1174);printint(maxstrptr);
print(1175);printint(poolptr){:1286};{1288:}sortavail;varused:=0;
dumpint(lomemmax);dumpint(rover);p:=0;q:=rover;x:=0;
repeat for k:=p to q+1 do dumpwd(mem[k]);x:=x+q+2-p;
varused:=varused+q-p;p:=q+mem[q].hh.lh;q:=mem[q+1].hh.rh;until q=rover;
varused:=varused+lomemmax-p;dynused:=memend+1-himemmin;
for k:=p to lomemmax do dumpwd(mem[k]);x:=x+lomemmax+1-p;
dumpint(himemmin);dumpint(avail);
for k:=himemmin to memend do dumpwd(mem[k]);x:=x+memend+1-himemmin;
p:=avail;while p<>0 do begin decr(dynused);p:=mem[p].hh.rh;end;
dumpint(varused);dumpint(dynused);println;printint(x);print(1176);
printint(varused);printchar(38);printint(dynused){:1288};{1290:}
dumpint(hashused);stcount:=9756-hashused;
for p:=1 to hashused do if hash[p].rh<>0 then begin dumpint(p);
dumphh(hash[p]);dumphh(eqtb[p]);incr(stcount);end;
for p:=hashused+1 to 9771 do begin dumphh(hash[p]);dumphh(eqtb[p]);end;
dumpint(stcount);println;printint(stcount);print(1177){:1290};{1292:}
dumpint(intptr);for k:=1 to intptr do begin dumpint(internal[k]);
dumpint(intname[k]);end;dumpint(startsym);dumpint(interaction);
dumpint(memident);dumpint(bgloc);dumpint(egloc);dumpint(serialno);
dumpint(69073);internal[10]:=0{:1292};{1295:}wclose(memfile){:1295};end;
endif('INIMP'){:1280}procedure dostatement;begin curtype:=1;getxnext;
if curcmd>45 then{1007:}
begin if curcmd<82 then begin begin if interaction=3 then;printnl(262);
print(917);end;printcmdmod(curcmd,curmod);printchar(39);
begin helpptr:=5;helpline[4]:=918;helpline[3]:=919;helpline[2]:=920;
helpline[1]:=921;helpline[0]:=922;end;backerror;getxnext;end;end{:1007}
else if curcmd>32 then{1010:}begin varflag:=76;scanexpression;
if curcmd<83 then begin if curcmd=53 then doequation else if curcmd=76
then doassignment else if curtype=4 then{1011:}
begin if internal[1]>0 then begin printnl(284);print(curexp);
fflush(stdout);end;end{:1011}
else if curtype<>1 then begin disperr(0,927);begin helpptr:=3;
helpline[2]:=928;helpline[1]:=929;helpline[0]:=930;end;putgeterror;end;
flushcurexp(0);curtype:=1;end;end{:1010}else{1009:}
begin if internal[6]>0 then showcmdmod(curcmd,curmod);
case curcmd of 32:dotypedeclaration;
18:if curmod>2 then makeopdef else if curmod>0 then scandef;{1037:}
26:dorandomseed;{:1037}{1040:}25:begin println;interaction:=curmod;
if interaction=0 then kpsemaketexdiscarderrors:=1 else
kpsemaketexdiscarderrors:=0;{85:}
if interaction=0 then selector:=7 else selector:=8{:85};
if logopened then selector:=selector+2;getxnext;end;{:1040}{1043:}
23:doprotection;{:1043}{1047:}29:defdelims;{:1047}{1050:}
14:repeat getsymbol;savevariable(cursym);getxnext;until curcmd<>81;
15:dointerim;16:dolet;17:donewinternal;{:1050}{1056:}24:doshowwhatever;
{:1056}{1082:}20:doaddto;21:dobounds;{:1082}{1097:}19:doshipout;{:1097}
{1100:}28:begin getsymbol;startsym:=cursym;getxnext;end;{:1100}{1105:}
27:domessage;{:1105}{1112:}31:dowrite;{:1112}{1131:}22:dotfmcommand;
{:1131}{1256:}30:dospecial;{:1256}end;curtype:=1;end{:1009};
if curcmd<82 then{1008:}begin begin if interaction=3 then;printnl(262);
print(923);end;begin helpptr:=6;helpline[5]:=924;helpline[4]:=925;
helpline[3]:=926;helpline[2]:=920;helpline[1]:=921;helpline[0]:=922;end;
backerror;scannerstatus:=2;repeat begin getnext;if curcmd<=3 then tnext;
end;{715:}
if curcmd=41 then begin if strref[curmod]<127 then if strref[curmod]>1
then decr(strref[curmod])else flushstring(curmod);end{:715};
until curcmd>81;scannerstatus:=0;end{:1008};errorcount:=0;end;{:1006}
{1034:}procedure maincontrol;begin repeat dostatement;
if curcmd=83 then begin begin if interaction=3 then;printnl(262);
print(958);end;begin helpptr:=2;helpline[1]:=959;helpline[0]:=730;end;
flusherror(0);end;until curcmd=84;end;{:1034}{1148:}
function sortin(v:scaled):halfword;label 40;var p,q,r:halfword;
begin p:=memtop-1;while true do begin q:=mem[p].hh.rh;
if v<=mem[q+1].int then goto 40;p:=q;end;
40:if v<mem[q+1].int then begin r:=getnode(2);mem[r+1].int:=v;
mem[r].hh.rh:=q;mem[p].hh.rh:=r;end;sortin:=mem[p].hh.rh;end;{:1148}
{1149:}function mincover(d:scaled):integer;var p:halfword;l:scaled;
m:integer;begin m:=0;p:=mem[memtop-1].hh.rh;perturbation:=2147483647;
while p<>11 do begin incr(m);l:=mem[p+1].int;repeat p:=mem[p].hh.rh;
until mem[p+1].int>l+d;
if mem[p+1].int-l<perturbation then perturbation:=mem[p+1].int-l;end;
mincover:=m;end;{:1149}{1151:}
function computethreshold(m:integer):scaled;var d:scaled;
begin excess:=mincover(0)-m;
if excess<=0 then computethreshold:=0 else begin repeat d:=perturbation;
until mincover(d+d)<=m;while mincover(d)>m do d:=perturbation;
computethreshold:=d;end;end;{:1151}{1152:}
function skimp(m:integer):integer;var d:scaled;p,q,r:halfword;l:scaled;
v:scaled;begin d:=computethreshold(m);perturbation:=0;q:=memtop-1;m:=0;
p:=mem[memtop-1].hh.rh;while p<>11 do begin incr(m);l:=mem[p+1].int;
mem[p].hh.lh:=m;if mem[mem[p].hh.rh+1].int<=l+d then{1153:}
begin repeat p:=mem[p].hh.rh;mem[p].hh.lh:=m;decr(excess);
if excess=0 then d:=0;until mem[mem[p].hh.rh+1].int>l+d;
v:=l+halfp(mem[p+1].int-l);
if mem[p+1].int-v>perturbation then perturbation:=mem[p+1].int-v;r:=q;
repeat r:=mem[r].hh.rh;mem[r+1].int:=v;until r=p;mem[q].hh.rh:=p;
end{:1153};q:=p;p:=mem[p].hh.rh;end;skimp:=m;end;{:1152}{1154:}
procedure tfmwarning(m:smallnumber);begin printnl(1088);
print(intname[m]);print(1089);printscaled(perturbation);print(1090);end;
{:1154}{1159:}procedure fixdesignsize;var d:scaled;
begin d:=internal[23];
if(d<65536)or(d>=134217728)then begin if d<>0 then printnl(1091);
d:=8388608;internal[23]:=d;end;
if headerbyte[5]<0 then if headerbyte[6]<0 then if headerbyte[7]<0 then
if headerbyte[8]<0 then begin headerbyte[5]:=d div 1048576;
headerbyte[6]:=(d div 4096)mod 256;headerbyte[7]:=(d div 16)mod 256;
headerbyte[8]:=(d mod 16)*16;end;
maxtfmdimen:=16*internal[23]-internal[23]div 2097152;
if maxtfmdimen>=134217728 then maxtfmdimen:=134217727;end;{:1159}{1160:}
function dimenout(x:scaled):integer;
begin if abs(x)>maxtfmdimen then begin incr(tfmchanged);
if x>0 then x:=16777215 else x:=-16777215;
end else x:=makescaled(x*16,internal[23]);dimenout:=x;end;{:1160}{1162:}
procedure fixchecksum;label 10;var k:eightbits;b1,b2,b3,b4:eightbits;
x:integer;
begin if headerbyte[1]<0 then if headerbyte[2]<0 then if headerbyte[3]<0
then if headerbyte[4]<0 then begin{1163:}b1:=bc;b2:=ec;b3:=bc;b4:=ec;
tfmchanged:=0;
for k:=bc to ec do if charexists[k]then begin x:=dimenout(mem[tfmwidth[k
]+1].int)+(k+4)*4194304;b1:=(b1+b1+x)mod 255;b2:=(b2+b2+x)mod 253;
b3:=(b3+b3+x)mod 251;b4:=(b4+b4+x)mod 247;end{:1163};headerbyte[1]:=b1;
headerbyte[2]:=b2;headerbyte[3]:=b3;headerbyte[4]:=b4;goto 10;end;
for k:=1 to 4 do if headerbyte[k]<0 then headerbyte[k]:=0;10:end;{:1162}
{1164:}procedure tfmqqqq(x:fourquarters);begin putbyte(x.b0,tfmfile);
putbyte(x.b1,tfmfile);putbyte(x.b2,tfmfile);putbyte(x.b3,tfmfile);end;
{:1164}{1281:}{756:}function openmemfile:boolean;label 40,10;
var j:0..bufsize;begin j:=curinput.locfield;
if buffer[curinput.locfield]=38 then begin incr(curinput.locfield);
j:=curinput.locfield;buffer[last]:=32;while buffer[j]<>32 do incr(j);
packbufferedname(0,curinput.locfield,j-1);
if wopenin(memfile)then goto 40;;
write(stdout,'Sorry, I can''t find the mem file `');
fputs(nameoffile+1,stdout);write(stdout,'''; will try `');
fputs(MPmemdefault+1,stdout);writeln(stdout,'''.');fflush(stdout);end;
packbufferedname(memdefaultlength-4,1,0);
if not wopenin(memfile)then begin;
write(stdout,'I can''t find the mem file `');
fputs(MPmemdefault+1,stdout);writeln(stdout,'''!');openmemfile:=false;
goto 10;end;40:curinput.locfield:=j;openmemfile:=true;10:end;{:756}
function loadmemfile:boolean;label 30,6666,10;var k:integer;
p,q:halfword;x:integer;s:strnumber;w:fourquarters;begin{1285:}
undumpint(x);if x<>136687108 then goto 6666;undumpint(x);
if x<>0 then goto 6666;
ifdef('INIMP')if iniversion then begin libcfree(mem);libcfree(strref);
libcfree(nextstr);libcfree(strstart);libcfree(strpool);end;
endif('INIMP')undumpint(memtop);if memmax<memtop then memmax:=memtop;
if 1100>memtop then goto 6666;xmallocarray(mem,memmax-0);undumpint(x);
if x<>9500 then goto 6666;undumpint(x);if x<>7919 then goto 6666;
undumpint(x);if x<>15 then goto 6666{:1285};{1287:}begin undumpint(x);
if x<0 then goto 6666;if x>suppoolsize-poolfree then begin;
writeln(stdout,'---! Must increase the ','string pool size');goto 6666;
end else poolptr:=x;end;
if poolsize<poolptr+poolfree then poolsize:=poolptr+poolfree;
begin undumpint(x);if x<0 then goto 6666;if x>supmaxstrings then begin;
writeln(stdout,'---! Must increase the ','max strings');goto 6666;
end else maxstrptr:=x;end;xmallocarray(strref,maxstrings);
xmallocarray(nextstr,maxstrings);xmallocarray(strstart,maxstrings);
xmallocarray(strpool,poolsize);begin undumpint(x);
if(x<0)or(x>maxstrptr)then goto 6666 else strptr:=x;end;
begin undumpint(x);if(x<0)or(x>maxstrptr+1)then goto 6666 else s:=x;end;
for k:=0 to s-1 do nextstr[k]:=k+1;
for k:=s to maxstrptr do begin undumpint(x);
if(x<s+1)or(x>maxstrptr+1)then goto 6666 else nextstr[k]:=x;end;
fixedstruse:=0;k:=0;while true do begin begin undumpint(x);
if(x<0)or(x>poolptr)then goto 6666 else strstart[k]:=x;end;
if k=strptr then goto 30;strref[k]:=127;incr(fixedstruse);
lastfixedstr:=k;k:=nextstr[k];end;30:k:=0;
while k+4<poolptr do begin undumpqqqq(w);strpool[k]:=w.b0;
strpool[k+1]:=w.b1;strpool[k+2]:=w.b2;strpool[k+3]:=w.b3;k:=k+4;end;
k:=poolptr-4;undumpqqqq(w);strpool[k]:=w.b0;strpool[k+1]:=w.b1;
strpool[k+2]:=w.b2;strpool[k+3]:=w.b3;initstruse:=fixedstruse;
initpoolptr:=poolptr;maxpoolptr:=poolptr;strsusedup:=fixedstruse;
ifdef('STAT')poolinuse:=strstart[strptr];strsinuse:=fixedstruse;
maxplused:=poolinuse;maxstrsused:=strsinuse;pactcount:=0;pactchars:=0;
pactstrs:=0;endif('STAT'){:1287};{1289:}begin undumpint(x);
if(x<1023)or(x>memtop-4)then goto 6666 else lomemmax:=x;end;
begin undumpint(x);if(x<24)or(x>lomemmax)then goto 6666 else rover:=x;
end;p:=0;q:=rover;repeat for k:=p to q+1 do undumpwd(mem[k]);
p:=q+mem[q].hh.lh;
if(p>lomemmax)or((q>=mem[q+1].hh.rh)and(mem[q+1].hh.rh<>rover))then goto
6666;q:=mem[q+1].hh.rh;until q=rover;
for k:=p to lomemmax do undumpwd(mem[k]);begin undumpint(x);
if(x<lomemmax+1)or(x>memtop-3)then goto 6666 else himemmin:=x;end;
begin undumpint(x);if(x<0)or(x>memtop)then goto 6666 else avail:=x;end;
memend:=memtop;for k:=himemmin to memend do undumpwd(mem[k]);
undumpint(varused);undumpint(dynused){:1289};{1291:}begin undumpint(x);
if(x<1)or(x>9757)then goto 6666 else hashused:=x;end;p:=0;
repeat begin undumpint(x);
if(x<p+1)or(x>hashused)then goto 6666 else p:=x;end;undumphh(hash[p]);
undumphh(eqtb[p]);until p=hashused;
for p:=hashused+1 to 9771 do begin undumphh(hash[p]);undumphh(eqtb[p]);
end;undumpint(stcount){:1291};{1293:}begin undumpint(x);
if(x<33)or(x>maxinternal)then goto 6666 else intptr:=x;end;
for k:=1 to intptr do begin undumpint(internal[k]);begin undumpint(x);
if(x<0)or(x>strptr)then goto 6666 else intname[k]:=x;end;end;
begin undumpint(x);if(x<0)or(x>9757)then goto 6666 else startsym:=x;end;
begin undumpint(x);if(x<0)or(x>3)then goto 6666 else interaction:=x;end;
if interactionoption<>4 then interaction:=interactionoption;
begin undumpint(x);if(x<0)or(x>strptr)then goto 6666 else memident:=x;
end;begin undumpint(x);if(x<1)or(x>9771)then goto 6666 else bgloc:=x;
end;begin undumpint(x);if(x<1)or(x>9771)then goto 6666 else egloc:=x;
end;undumpint(serialno);undumpint(x);
if(x<>69073)or feof(memfile)then goto 6666{:1293};loadmemfile:=true;
goto 10;6666:;writeln(stdout,'(Fatal mem file error; I''m stymied)');
loadmemfile:=false;10:end;{:1281}{1296:}{811:}procedure scanprimary;
label 20,30,31,32;var p,q,r:halfword;c:quarterword;myvarflag:0..84;
ldelim,rdelim:halfword;{821:}groupline:integer;{:821}{826:}
num,denom:scaled;{:826}{833:}prehead,posthead,tail:halfword;
tt:smallnumber;t:halfword;macroref:halfword;{:833}
begin myvarflag:=varflag;varflag:=0;
20:begin if aritherror then cleararith;end;{813:}
ifdef('TEXMF_DEBUG')if panicking then checkmem(false);
endif('TEXMF_DEBUG')if interrupt<>0 then if OKtointerrupt then begin
backinput;begin if interrupt<>0 then pauseforinstructions;end;getxnext;
end{:813};case curcmd of 33:{814:}begin ldelim:=cursym;rdelim:=curmod;
getxnext;scanexpression;if(curcmd=81)and(curtype>=16)then{818:}
begin p:=stashcurexp;getxnext;scanexpression;{819:}
if curtype<16 then begin disperr(0,824);begin helpptr:=4;
helpline[3]:=825;helpline[2]:=826;helpline[1]:=827;helpline[0]:=828;end;
putgetflusherror(0);end{:819};q:=getnode(2);mem[q].hh.b1:=14;
if curcmd=81 then mem[q].hh.b0:=13 else mem[q].hh.b0:=14;initbignode(q);
r:=mem[q+1].int;stashin(r+2);unstashcurexp(p);stashin(r);
if curcmd=81 then{820:}begin getxnext;scanexpression;
if curtype<16 then begin disperr(0,829);begin helpptr:=3;
helpline[2]:=830;helpline[1]:=827;helpline[0]:=828;end;
putgetflusherror(0);end;stashin(r+4);end{:820};
checkdelimiter(ldelim,rdelim);curtype:=mem[q].hh.b0;curexp:=q;end{:818}
else checkdelimiter(ldelim,rdelim);end{:814};34:{822:}
begin groupline:=trueline;
if internal[6]>0 then showcmdmod(curcmd,curmod);begin p:=getavail;
mem[p].hh.lh:=0;mem[p].hh.rh:=saveptr;saveptr:=p;end;repeat dostatement;
until curcmd<>82;if curcmd<>83 then begin begin if interaction=3 then;
printnl(262);print(831);end;printint(groupline);print(832);
begin helpptr:=2;helpline[1]:=833;helpline[0]:=834;end;backerror;
curcmd:=83;end;unsave;if internal[6]>0 then showcmdmod(curcmd,curmod);
end{:822};41:{823:}begin curtype:=4;curexp:=curmod;end{:823};44:{827:}
begin curexp:=curmod;curtype:=16;getxnext;
if curcmd<>56 then begin num:=0;denom:=0;end else begin getxnext;
if curcmd<>44 then begin backinput;curcmd:=56;curmod:=92;cursym:=9761;
goto 30;end;num:=curexp;denom:=curmod;if denom=0 then{828:}
begin begin if interaction=3 then;printnl(262);print(835);end;
begin helpptr:=1;helpline[0]:=836;end;error;end{:828}
else curexp:=makescaled(num,denom);begin if aritherror then cleararith;
end;getxnext;end;
if curcmd>=32 then if curcmd<44 then begin p:=stashcurexp;scanprimary;
if(abs(num)>=abs(denom))or(curtype<13)then dobinary(p,91)else begin
fracmult(num,denom);freenode(p,2);end;end;goto 30;end{:827};35:{824:}
donullary(curmod){:824};36,32,38,45:{825:}begin c:=curmod;getxnext;
scanprimary;dounary(c);goto 30;end{:825};39:{829:}begin c:=curmod;
getxnext;scanexpression;if curcmd<>70 then begin missingerr(489);
print(756);printcmdmod(39,c);begin helpptr:=1;helpline[0]:=757;end;
backerror;end;p:=stashcurexp;getxnext;scanprimary;dobinary(p,c);goto 30;
end{:829};37:{830:}begin getxnext;scansuffix;oldsetting:=selector;
selector:=4;showtokenlist(curexp,0,100000,0);flushtokenlist(curexp);
curexp:=makestring;selector:=oldsetting;curtype:=4;goto 30;end{:830};
42:{831:}begin q:=curmod;if myvarflag=76 then begin getxnext;
if curcmd=76 then begin curexp:=getavail;mem[curexp].hh.lh:=q+9771;
curtype:=20;goto 30;end;backinput;end;curtype:=16;curexp:=internal[q];
end{:831};40:makeexpcopy(curmod);43:{834:}begin begin prehead:=avail;
if prehead=0 then prehead:=getavail else begin avail:=mem[prehead].hh.rh
;mem[prehead].hh.rh:=0;ifdef('STAT')incr(dynused);endif('STAT')end;end;
tail:=prehead;posthead:=0;tt:=1;while true do begin t:=curtok;
mem[tail].hh.rh:=t;if tt<>0 then begin{840:}begin p:=mem[prehead].hh.rh;
q:=mem[p].hh.lh;tt:=0;if eqtb[q].lh mod 85=43 then begin q:=eqtb[q].rh;
if q=0 then goto 32;while true do begin p:=mem[p].hh.rh;
if p=0 then begin tt:=mem[q].hh.b0;goto 32;end;
if mem[q].hh.b0<>21 then goto 32;q:=mem[mem[q+1].hh.lh].hh.rh;
if p>=himemmin then begin repeat q:=mem[q].hh.rh;
until mem[q+2].hh.lh>=mem[p].hh.lh;
if mem[q+2].hh.lh>mem[p].hh.lh then goto 32;end;end;end;32:end{:840};
if tt>=22 then{835:}begin mem[tail].hh.rh:=0;
if tt>22 then begin posthead:=getavail;tail:=posthead;
mem[tail].hh.rh:=t;tt:=0;macroref:=mem[q+1].int;
incr(mem[macroref].hh.lh);end else{843:}begin p:=getavail;
mem[prehead].hh.lh:=mem[prehead].hh.rh;mem[prehead].hh.rh:=p;
mem[p].hh.lh:=t;macrocall(mem[q+1].int,prehead,0);getxnext;goto 20;
end{:843};end{:835};end;getxnext;tail:=t;if curcmd=65 then{836:}
begin getxnext;scanexpression;if curcmd<>66 then{837:}begin backinput;
backexpr;curcmd:=65;curmod:=0;cursym:=9760;end{:837}
else begin if curtype<>16 then badsubscript;curcmd:=44;curmod:=curexp;
cursym:=0;end;end{:836};if curcmd>44 then goto 31;
if curcmd<42 then goto 31;end;31:{842:}if posthead<>0 then{844:}
begin backinput;p:=getavail;q:=mem[posthead].hh.rh;
mem[prehead].hh.lh:=mem[prehead].hh.rh;mem[prehead].hh.rh:=posthead;
mem[posthead].hh.lh:=q;mem[posthead].hh.rh:=p;
mem[p].hh.lh:=mem[q].hh.rh;mem[q].hh.rh:=0;
macrocall(macroref,prehead,0);decr(mem[macroref].hh.lh);getxnext;
goto 20;end{:844};q:=mem[prehead].hh.rh;begin mem[prehead].hh.rh:=avail;
avail:=prehead;ifdef('STAT')decr(dynused);endif('STAT')end;
if curcmd=myvarflag then begin curtype:=20;curexp:=q;goto 30;end;
p:=findvariable(q);if p<>0 then makeexpcopy(p)else begin obliterated(q);
helpline[2]:=848;helpline[1]:=849;helpline[0]:=850;putgetflusherror(0);
end;flushnodelist(q);goto 30{:842};end{:834};others:begin badexp(818);
goto 20;end end;getxnext;30:if curcmd=65 then if curtype>=16 then{849:}
begin p:=stashcurexp;getxnext;scanexpression;
if curcmd<>81 then begin{837:}begin backinput;backexpr;curcmd:=65;
curmod:=0;cursym:=9760;end{:837};unstashcurexp(p);
end else begin q:=stashcurexp;getxnext;scanexpression;
if curcmd<>66 then begin missingerr(93);begin helpptr:=3;
helpline[2]:=852;helpline[1]:=853;helpline[0]:=738;end;backerror;end;
r:=stashcurexp;makeexpcopy(q);dobinary(r,90);dobinary(p,91);
dobinary(q,89);getxnext;end;end{:849};end;{:811}{850:}
procedure scansuffix;label 30;var h,t:halfword;p:halfword;
begin h:=getavail;t:=h;while true do begin if curcmd=65 then{851:}
begin getxnext;scanexpression;if curtype<>16 then badsubscript;
if curcmd<>66 then begin missingerr(93);begin helpptr:=3;
helpline[2]:=854;helpline[1]:=853;helpline[0]:=738;end;backerror;end;
curcmd:=44;curmod:=curexp;end{:851};
if curcmd=44 then p:=newnumtok(curmod)else if(curcmd=43)or(curcmd=42)
then begin p:=getavail;mem[p].hh.lh:=cursym;end else goto 30;
mem[t].hh.rh:=p;t:=p;getxnext;end;30:curexp:=mem[h].hh.rh;
begin mem[h].hh.rh:=avail;avail:=h;ifdef('STAT')decr(dynused);
endif('STAT')end;curtype:=20;end;{:850}{852:}procedure scansecondary;
label 20,22;var p:halfword;c,d:halfword;macname:halfword;
begin 20:if(curcmd<32)or(curcmd>45)then badexp(855);scanprimary;
22:if curcmd<=57 then if curcmd>=54 then begin p:=stashcurexp;c:=curmod;
d:=curcmd;if d=55 then begin macname:=cursym;incr(mem[c].hh.lh);end;
getxnext;scanprimary;if d<>55 then dobinary(p,c)else begin backinput;
binarymac(p,c,macname);decr(mem[c].hh.lh);getxnext;goto 20;end;goto 22;
end;end;{:852}{854:}procedure scantertiary;label 20,22;var p:halfword;
c,d:halfword;macname:halfword;
begin 20:if(curcmd<32)or(curcmd>45)then badexp(856);scansecondary;
22:if curcmd<=47 then if curcmd>=45 then begin p:=stashcurexp;c:=curmod;
d:=curcmd;if d=46 then begin macname:=cursym;incr(mem[c].hh.lh);end;
getxnext;scansecondary;if d<>46 then dobinary(p,c)else begin backinput;
binarymac(p,c,macname);decr(mem[c].hh.lh);getxnext;goto 20;end;goto 22;
end;end;{:854}{855:}procedure scanexpression;label 20,30,22,25,26,10;
var p,q,r,pp,qq:halfword;c,d:halfword;myvarflag:0..84;macname:halfword;
cyclehit:boolean;x,y:scaled;t:0..4;begin myvarflag:=varflag;
20:if(curcmd<32)or(curcmd>45)then badexp(857);scantertiary;
22:if curcmd<=53 then if curcmd>=48 then if(curcmd<>53)or(myvarflag<>76)
then begin p:=stashcurexp;c:=curmod;d:=curcmd;
if d=51 then begin macname:=cursym;incr(mem[c].hh.lh);end;
if(d<50)or((d=50)and((mem[p].hh.b0=14)or(mem[p].hh.b0=8)))then{856:}
begin cyclehit:=false;{857:}begin unstashcurexp(p);
if curtype=14 then p:=newknot else if curtype=8 then p:=curexp else goto
10;q:=p;while mem[q].hh.rh<>p do q:=mem[q].hh.rh;
if mem[p].hh.b0<>0 then begin r:=copyknot(p);mem[q].hh.rh:=r;q:=r;end;
mem[p].hh.b0:=4;mem[q].hh.b1:=4;end{:857};25:{861:}
if curcmd=48 then{866:}begin t:=scandirection;
if t<>4 then begin mem[q].hh.b1:=t;mem[q+5].int:=curexp;
if mem[q].hh.b0=4 then begin mem[q].hh.b0:=t;mem[q+3].int:=curexp;end;
end;end{:866};d:=curcmd;if d=49 then{868:}begin getxnext;
if curcmd=60 then{869:}begin getxnext;y:=curcmd;
if curcmd=61 then getxnext;scanprimary;{870:}
if(curtype<>16)or(curexp<49152)then begin disperr(0,875);
begin helpptr:=1;helpline[0]:=876;end;putgetflusherror(65536);end{:870};
if y=61 then curexp:=-curexp;mem[q+6].int:=curexp;
if curcmd=54 then begin getxnext;y:=curcmd;if curcmd=61 then getxnext;
scanprimary;{870:}
if(curtype<>16)or(curexp<49152)then begin disperr(0,875);
begin helpptr:=1;helpline[0]:=876;end;putgetflusherror(65536);end{:870};
if y=61 then curexp:=-curexp;end;y:=curexp;end{:869}
else if curcmd=59 then{871:}begin mem[q].hh.b1:=1;t:=1;getxnext;
scanprimary;knownpair;mem[q+5].int:=curx;mem[q+6].int:=cury;
if curcmd<>54 then begin x:=mem[q+5].int;y:=mem[q+6].int;
end else begin getxnext;scanprimary;knownpair;x:=curx;y:=cury;end;
end{:871}else begin mem[q+6].int:=65536;y:=65536;backinput;goto 30;end;
if curcmd<>49 then begin missingerr(321);begin helpptr:=1;
helpline[0]:=874;end;backerror;end;30:end{:868}
else if d<>50 then goto 26;getxnext;if curcmd=48 then{867:}
begin t:=scandirection;if mem[q].hh.b1<>1 then x:=curexp else t:=1;
end{:867}else if mem[q].hh.b1<>1 then begin t:=4;x:=0;end{:861};
if curcmd=38 then{873:}begin cyclehit:=true;getxnext;pp:=p;qq:=p;
if d=50 then if p=q then begin d:=49;mem[q+6].int:=65536;y:=65536;end;
end{:873}else begin scantertiary;{872:}
begin if curtype<>8 then pp:=newknot else pp:=curexp;qq:=pp;
while mem[qq].hh.rh<>pp do qq:=mem[qq].hh.rh;
if mem[pp].hh.b0<>0 then begin r:=copyknot(pp);mem[qq].hh.rh:=r;qq:=r;
end;mem[pp].hh.b0:=4;mem[qq].hh.b1:=4;end{:872};end;{874:}
begin if d=50 then if(mem[q+1].int<>mem[pp+1].int)or(mem[q+2].int<>mem[
pp+2].int)then begin begin if interaction=3 then;printnl(262);
print(877);end;begin helpptr:=3;helpline[2]:=878;helpline[1]:=879;
helpline[0]:=880;end;putgeterror;d:=49;mem[q+6].int:=65536;y:=65536;end;
{876:}if mem[pp].hh.b1=4 then if(t=3)or(t=2)then begin mem[pp].hh.b1:=t;
mem[pp+5].int:=x;end{:876};if d=50 then{877:}
begin if mem[q].hh.b0=4 then if mem[q].hh.b1=4 then begin mem[q].hh.b0:=
3;mem[q+3].int:=65536;end;
if mem[pp].hh.b1=4 then if t=4 then begin mem[pp].hh.b1:=3;
mem[pp+5].int:=65536;end;mem[q].hh.b1:=mem[pp].hh.b1;
mem[q].hh.rh:=mem[pp].hh.rh;mem[q+5].int:=mem[pp+5].int;
mem[q+6].int:=mem[pp+6].int;freenode(pp,7);if qq=pp then qq:=q;end{:877}
else begin{875:}
if mem[q].hh.b1=4 then if(mem[q].hh.b0=3)or(mem[q].hh.b0=2)then begin
mem[q].hh.b1:=mem[q].hh.b0;mem[q+5].int:=mem[q+3].int;end{:875};
mem[q].hh.rh:=pp;mem[pp+4].int:=y;if t<>4 then begin mem[pp+3].int:=x;
mem[pp].hh.b0:=t;end;end;q:=qq;end{:874};
if curcmd>=48 then if curcmd<=50 then if not cyclehit then goto 25;
26:{878:}if cyclehit then begin if d=50 then p:=q;
end else begin mem[p].hh.b0:=0;
if mem[p].hh.b1=4 then begin mem[p].hh.b1:=3;mem[p+5].int:=65536;end;
mem[q].hh.b1:=0;if mem[q].hh.b0=4 then begin mem[q].hh.b0:=3;
mem[q+3].int:=65536;end;mem[q].hh.rh:=p;end;makechoices(p);curtype:=8;
curexp:=p{:878};end{:856}else begin getxnext;scantertiary;
if d<>51 then dobinary(p,c)else begin backinput;binarymac(p,c,macname);
decr(mem[c].hh.lh);getxnext;goto 20;end;end;goto 22;end;10:end;{:855}
{879:}procedure getboolean;begin getxnext;scanexpression;
if curtype<>2 then begin disperr(0,881);begin helpptr:=2;
helpline[1]:=882;helpline[0]:=883;end;putgetflusherror(31);curtype:=2;
end;end;{:879}{243:}procedure printcapsule;begin printchar(40);
printexp(gpointer,0);printchar(41);end;procedure tokenrecycle;
begin recyclevalue(gpointer);end;{:243}{1299:}
procedure closefilesandterminate;var k:integer;lh:integer;
lkoffset:0..256;p:halfword;begin{1300:}
for k:=0 to readfiles-1 do if rdfname[k]<>0 then aclose(rdfile[k]);
for k:=0 to writefiles-1 do if wrfname[k]<>0 then aclose(wrfile[k]){:
1300};ifdef('STAT')if internal[10]>0 then{1303:}
if logopened then begin writeln(logfile,' ');
writeln(logfile,'Here is how much of MetaPost''s memory',' you used:');
write(logfile,' ',maxstrsused-initstruse:1,' string');
if maxstrsused<>initstruse+1 then write(logfile,'s');
writeln(logfile,' out of ',maxstrings-1-initstruse:1);
writeln(logfile,' ',maxplused-initpoolptr:1,' string characters out of '
,poolsize-initpoolptr:1);
writeln(logfile,' ',lomemmax+0+memend-himemmin+2:1,
' words of memory out of ',memend+1:1);
writeln(logfile,' ',stcount:1,' symbolic tokens out of ',9500:1);
writeln(logfile,' ',maxinstack:1,'i,',intptr:1,'n,',maxparamstack:1,'p,'
,maxbufstack+1:1,'b stack positions out of ',stacksize:1,'i,',
maxinternal:1,'n,',150:1,'p,',bufsize:1,'b');
writeln(logfile,' ',pactcount:1,' string compactions (moved ',pactchars:
1,' characters, ',pactstrs:1,' strings)');end{:1303};endif('STAT');
{1301:}if internal[26]>0 then begin{1302:}rover:=24;
mem[rover].hh.rh:=268435455;lomemmax:=himemmin-1;
if lomemmax-rover>268435455 then lomemmax:=268435455+rover;
mem[rover].hh.lh:=lomemmax-rover;mem[rover+1].hh.lh:=rover;
mem[rover+1].hh.rh:=rover;mem[lomemmax].hh.rh:=0;
mem[lomemmax].hh.lh:=0{:1302};{1155:}mem[memtop-1].hh.rh:=11;
for k:=bc to ec do if charexists[k]then tfmwidth[k]:=sortin(tfmwidth[k])
;nw:=skimp(255)+1;dimenhead[1]:=mem[memtop-1].hh.rh;
if perturbation>=4096 then tfmwarning(19){:1155};fixdesignsize;
fixchecksum;{1157:}mem[memtop-1].hh.rh:=11;
for k:=bc to ec do if charexists[k]then if tfmheight[k]=0 then tfmheight
[k]:=7 else tfmheight[k]:=sortin(tfmheight[k]);nh:=skimp(15)+1;
dimenhead[2]:=mem[memtop-1].hh.rh;
if perturbation>=4096 then tfmwarning(20);mem[memtop-1].hh.rh:=11;
for k:=bc to ec do if charexists[k]then if tfmdepth[k]=0 then tfmdepth[k
]:=7 else tfmdepth[k]:=sortin(tfmdepth[k]);nd:=skimp(15)+1;
dimenhead[3]:=mem[memtop-1].hh.rh;
if perturbation>=4096 then tfmwarning(21);mem[memtop-1].hh.rh:=11;
for k:=bc to ec do if charexists[k]then if tfmitalcorr[k]=0 then
tfmitalcorr[k]:=7 else tfmitalcorr[k]:=sortin(tfmitalcorr[k]);
ni:=skimp(63)+1;dimenhead[4]:=mem[memtop-1].hh.rh;
if perturbation>=4096 then tfmwarning(22){:1157};internal[26]:=0;{1165:}
if jobname=0 then openlogfile;packjobname(1092);
while not bopenout(tfmfile)do promptfilename(1093,1092);
metricfilename:=bmakenamestring(tfmfile);{1166:}k:=headersize;
while headerbyte[k]<0 do decr(k);lh:=(k+3)div 4;if bc>ec then bc:=1;
{1168:}bchar:=roundunscaled(internal[31]);
if(bchar<0)or(bchar>255)then begin bchar:=-1;lkstarted:=false;
lkoffset:=0;end else begin lkstarted:=true;lkoffset:=1;end;{1169:}
k:=labelptr;if labelloc[k]+lkoffset>255 then begin lkoffset:=0;
lkstarted:=false;repeat charremainder[labelchar[k]]:=lkoffset;
while labelloc[k-1]=labelloc[k]do begin decr(k);
charremainder[labelchar[k]]:=lkoffset;end;incr(lkoffset);decr(k);
until lkoffset+labelloc[k]<256;end;
if lkoffset>0 then while k>0 do begin charremainder[labelchar[k]]:=
charremainder[labelchar[k]]+lkoffset;decr(k);end{:1169};
if bchlabel<ligtablesize then begin ligkern[nl].b0:=255;
ligkern[nl].b1:=0;ligkern[nl].b2:=((bchlabel+lkoffset)div 256);
ligkern[nl].b3:=((bchlabel+lkoffset)mod 256);incr(nl);end{:1168};
put2bytes(tfmfile,6+lh+(ec-bc+1)+nw+nh+nd+ni+nl+lkoffset+nk+ne+np);
put2bytes(tfmfile,lh);put2bytes(tfmfile,bc);put2bytes(tfmfile,ec);
put2bytes(tfmfile,nw);put2bytes(tfmfile,nh);put2bytes(tfmfile,nd);
put2bytes(tfmfile,ni);put2bytes(tfmfile,nl+lkoffset);
put2bytes(tfmfile,nk);put2bytes(tfmfile,ne);put2bytes(tfmfile,np);
for k:=1 to 4*lh do begin if headerbyte[k]<0 then headerbyte[k]:=0;
putbyte(headerbyte[k],tfmfile);end{:1166};{1167:}
for k:=bc to ec do if not charexists[k]then put4bytes(tfmfile,0)else
begin putbyte(mem[tfmwidth[k]].hh.lh,tfmfile);
putbyte((mem[tfmheight[k]].hh.lh)*16+mem[tfmdepth[k]].hh.lh,tfmfile);
putbyte((mem[tfmitalcorr[k]].hh.lh)*4+chartag[k],tfmfile);
putbyte(charremainder[k],tfmfile);end;tfmchanged:=0;
for k:=1 to 4 do begin put4bytes(tfmfile,0);p:=dimenhead[k];
while p<>11 do begin put4bytes(tfmfile,dimenout(mem[p+1].int));
p:=mem[p].hh.rh;end;end{:1167};{1170:}
for k:=0 to 255 do if skiptable[k]<ligtablesize then begin printnl(1095)
;printint(k);print(1096);ll:=skiptable[k];repeat lll:=ligkern[ll].b0;
ligkern[ll].b0:=128;ll:=ll-lll;until lll=0;end;
if lkstarted then begin putbyte(255,tfmfile);putbyte(bchar,tfmfile);
put2bytes(tfmfile,0);
end else for k:=1 to lkoffset do begin ll:=labelloc[labelptr];
if bchar<0 then begin putbyte(254,tfmfile);putbyte(0,tfmfile);
end else begin putbyte(255,tfmfile);putbyte(bchar,tfmfile);end;
put2bytes(tfmfile,ll+lkoffset);repeat decr(labelptr);
until labelloc[labelptr]<ll;end;for k:=0 to nl-1 do tfmqqqq(ligkern[k]);
for k:=0 to nk-1 do put4bytes(tfmfile,dimenout(kern[k])){:1170};{1171:}
for k:=0 to ne-1 do tfmqqqq(exten[k]);
for k:=1 to np do if k=1 then if abs(param[1])<134217728 then put4bytes(
tfmfile,param[1]*16)else begin incr(tfmchanged);
if param[1]>0 then put4bytes(tfmfile,2147483647)else put4bytes(tfmfile,
-2147483647);end else put4bytes(tfmfile,dimenout(param[k]));
if tfmchanged>0 then begin if tfmchanged=1 then printnl(1097)else begin
printnl(40);printint(tfmchanged);print(1098);end;print(1099);end{:1171};
ifdef('STAT')if internal[10]>0 then{1172:}begin writeln(logfile,' ');
if bchlabel<ligtablesize then decr(nl);
writeln(logfile,'(You used ',nw:1,'w,',nh:1,'h,',nd:1,'d,',ni:1,'i,',nl:
1,'l,',nk:1,'k,',ne:1,'e,',np:1,'p metric file positions');
writeln(logfile,'  out of ','256w,16h,16d,64i,',ligtablesize:1,'l,',
maxkerns:1,'k,256e,',maxfontdimen:1,'p)');end{:1172};
endif('STAT')printnl(1094);print(metricfilename);printchar(46);
bclose(tfmfile){:1165};end{:1301};{1208:}
if totalshipped>0 then begin printnl(284);printint(totalshipped);
print(1116);if totalshipped>1 then printchar(115);print(1117);
print(firstfilename);
if totalshipped>1 then begin if 31+(strstart[nextstr[firstfilename]]-
strstart[firstfilename])+(strstart[nextstr[lastfilename]]-strstart[
lastfilename])>maxprintline then println;print(548);print(lastfilename);
end;end{:1208};if logopened then begin writeln(logfile);aclose(logfile);
selector:=selector-2;if selector=8 then begin printnl(1181);
print(texmflogname);printchar(46);end;end;println;
if(editnamestart<>0)and(interaction>0)then calledit(strpool,
editnamestart,editnamelength,editline);end;{:1299}{1304:}
procedure finalcleanup;label 10;var c:smallnumber;begin c:=curmod;
if jobname=0 then openlogfile;
while inputptr>0 do if(curinput.indexfield>15)then endtokenlist else
endfilereading;while loopptr<>0 do stopiteration;
while openparens>0 do begin print(1182);decr(openparens);end;
while condptr<>0 do begin printnl(1183);printcmdmod(5,curif);
if ifline<>0 then begin print(1184);printint(ifline);end;print(1185);
ifline:=mem[condptr+1].int;curif:=mem[condptr].hh.b1;
condptr:=mem[condptr].hh.rh;end;
if history<>0 then if((history=1)or(interaction<3))then if selector=10
then begin selector:=8;printnl(1186);selector:=10;end;
if c=1 then begin ifdef('INIMP')if iniversion then begin storememfile;
goto 10;end;endif('INIMP')printnl(1187);goto 10;end;10:end;{:1304}
{1305:}ifdef('INIMP')procedure initprim;begin{210:}primitive(430,42,1);
primitive(431,42,2);primitive(432,42,3);primitive(433,42,4);
primitive(434,42,5);primitive(435,42,6);primitive(436,42,7);
primitive(437,42,8);primitive(438,42,9);primitive(439,42,10);
primitive(440,42,11);primitive(441,42,12);primitive(442,42,13);
primitive(443,42,14);primitive(444,42,15);primitive(445,42,16);
primitive(446,42,17);primitive(447,42,18);primitive(448,42,19);
primitive(449,42,20);primitive(450,42,21);primitive(451,42,22);
primitive(452,42,23);primitive(453,42,24);primitive(454,42,25);
primitive(455,42,26);primitive(456,42,27);primitive(457,42,28);
primitive(458,42,29);primitive(459,42,30);primitive(460,42,31);
primitive(461,42,32);primitive(462,42,33);{:210}{229:}
primitive(321,49,0);primitive(91,65,0);eqtb[9760]:=eqtb[cursym];
primitive(93,66,0);primitive(125,67,0);primitive(123,48,0);
primitive(58,80,0);eqtb[9762]:=eqtb[cursym];primitive(474,79,0);
primitive(475,78,0);primitive(476,76,0);primitive(44,81,0);
primitive(59,82,0);eqtb[9763]:=eqtb[cursym];primitive(92,10,0);
primitive(477,20,0);primitive(478,61,0);primitive(479,34,0);
bgloc:=cursym;primitive(480,59,0);primitive(481,62,0);
primitive(482,29,0);primitive(468,83,0);eqtb[9767]:=eqtb[cursym];
egloc:=cursym;primitive(483,28,0);primitive(484,9,0);
primitive(485,12,0);primitive(486,15,0);primitive(487,16,0);
primitive(488,17,0);primitive(489,70,0);primitive(490,26,0);
primitive(491,14,0);primitive(492,11,0);primitive(493,19,0);
primitive(494,77,0);primitive(495,30,0);primitive(496,72,0);
primitive(497,37,0);primitive(498,60,0);primitive(499,71,0);
primitive(500,73,0);primitive(501,74,0);primitive(502,31,0);{:229}{647:}
primitive(681,1,0);primitive(682,1,1);primitive(465,2,0);
eqtb[9768]:=eqtb[cursym];primitive(466,3,0);eqtb[9769]:=eqtb[cursym];
{:647}{655:}primitive(695,18,1);primitive(696,18,2);
primitive(697,18,55);primitive(698,18,46);primitive(699,18,51);
primitive(469,18,0);eqtb[9765]:=eqtb[cursym];primitive(700,7,9772);
primitive(701,7,9922);primitive(702,7,1);primitive(470,7,0);
eqtb[9764]:=eqtb[cursym];{:655}{660:}primitive(703,63,0);
primitive(704,63,1);primitive(64,63,2);primitive(705,63,3);{:660}{667:}
primitive(716,58,9772);primitive(717,58,9922);primitive(718,58,10072);
primitive(719,58,1);primitive(720,58,2);primitive(721,58,3);{:667}{681:}
primitive(731,6,0);primitive(628,6,1);{:681}{712:}primitive(758,4,1);
primitive(467,5,2);eqtb[9766]:=eqtb[cursym];primitive(759,5,3);
primitive(760,5,4);{:712}{880:}primitive(347,35,30);
primitive(348,35,31);primitive(349,35,32);primitive(350,35,33);
primitive(351,35,34);primitive(352,35,35);primitive(353,35,36);
primitive(354,35,37);primitive(355,36,38);primitive(356,36,39);
primitive(357,36,40);primitive(358,36,41);primitive(359,36,42);
primitive(360,36,43);primitive(361,36,44);primitive(362,36,45);
primitive(363,36,46);primitive(364,36,47);primitive(365,36,48);
primitive(366,36,49);primitive(367,36,50);primitive(368,36,51);
primitive(369,36,52);primitive(370,36,53);primitive(371,36,54);
primitive(372,36,55);primitive(373,36,56);primitive(374,36,57);
primitive(375,36,58);primitive(376,36,59);primitive(377,36,60);
primitive(378,36,61);primitive(379,36,62);primitive(380,36,63);
primitive(381,36,64);primitive(382,36,65);primitive(383,36,66);
primitive(384,36,67);primitive(385,36,68);primitive(386,36,69);
primitive(387,36,70);primitive(388,36,71);primitive(389,36,72);
primitive(390,36,73);primitive(391,36,74);primitive(392,36,75);
primitive(393,36,76);primitive(394,36,77);primitive(395,36,78);
primitive(396,36,79);primitive(397,36,80);primitive(398,36,81);
primitive(399,36,82);primitive(400,38,83);primitive(402,36,85);
primitive(401,36,84);primitive(403,36,86);primitive(404,36,87);
primitive(405,36,88);primitive(43,45,89);primitive(45,45,90);
primitive(42,57,91);primitive(47,56,92);eqtb[9761]:=eqtb[cursym];
primitive(406,47,93);primitive(310,47,94);primitive(407,47,95);
primitive(408,54,96);primitive(60,52,97);primitive(409,52,98);
primitive(62,52,99);primitive(410,52,100);primitive(61,53,101);
primitive(411,52,102);primitive(422,39,115);primitive(423,39,116);
primitive(424,39,117);primitive(425,39,118);primitive(426,39,119);
primitive(427,39,120);primitive(428,39,121);primitive(429,39,122);
primitive(38,50,103);primitive(412,57,104);primitive(413,57,105);
primitive(414,57,106);primitive(415,57,107);primitive(416,57,108);
primitive(417,57,109);primitive(418,57,110);primitive(419,57,111);
primitive(420,57,112);primitive(421,47,113);{:880}{1030:}
primitive(340,32,15);primitive(259,32,4);primitive(325,32,2);
primitive(330,32,8);primitive(328,32,6);primitive(332,32,10);
primitive(334,32,12);primitive(335,32,13);primitive(336,32,14);{:1030}
{1035:}primitive(960,84,0);primitive(961,84,1);{:1035}{1041:}
primitive(272,25,0);primitive(273,25,1);primitive(274,25,2);
primitive(967,25,3);{:1041}{1044:}primitive(968,23,0);
primitive(969,23,1);{:1044}{1054:}primitive(983,24,0);
primitive(984,24,1);primitive(985,24,2);primitive(986,24,3);
primitive(987,24,4);{:1054}{1069:}primitive(1006,69,0);
primitive(1007,69,1);primitive(1008,69,2);primitive(1009,68,6);
primitive(1010,68,10);primitive(1011,68,13);{:1069}{1083:}
primitive(1020,21,4);primitive(1021,21,5);{:1083}{1103:}
primitive(1034,27,0);primitive(1035,27,1);primitive(1036,27,2);{:1103}
{1132:}primitive(1051,22,0);primitive(1052,22,1);primitive(1053,22,2);
primitive(1054,22,3);primitive(1055,22,4);{:1132}{1139:}
primitive(1073,75,0);primitive(1074,75,1);primitive(1075,75,5);
primitive(1076,75,2);primitive(1077,75,6);primitive(1078,75,3);
primitive(1079,75,7);primitive(1080,75,11);primitive(1081,75,128);
{:1139};end;procedure inittab;var k:integer;begin{191:}rover:=24;
mem[rover].hh.rh:=268435455;mem[rover].hh.lh:=1000;
mem[rover+1].hh.lh:=rover;mem[rover+1].hh.rh:=rover;
lomemmax:=rover+1000;mem[lomemmax].hh.rh:=0;mem[lomemmax].hh.lh:=0;
for k:=memtop-3 to memtop do mem[k]:=mem[lomemmax];avail:=0;
memend:=memtop;himemmin:=memtop-3;varused:=24;
dynused:=memtop+1-(memtop-3);{362:}mem[14].int:=-32768;mem[15].int:=0;
mem[17].int:=32768;mem[18].int:=0;mem[20].int:=0;mem[21].int:=65536;
mem[13].hh.rh:=16;mem[16].hh.rh:=19;mem[19].hh.rh:=13;mem[13].hh.lh:=19;
mem[16].hh.lh:=13;mem[19].hh.lh:=16{:362};{:191}{211:}intname[1]:=430;
intname[2]:=431;intname[3]:=432;intname[4]:=433;intname[5]:=434;
intname[6]:=435;intname[7]:=436;intname[8]:=437;intname[9]:=438;
intname[10]:=439;intname[11]:=440;intname[12]:=441;intname[13]:=442;
intname[14]:=443;intname[15]:=444;intname[16]:=445;intname[17]:=446;
intname[18]:=447;intname[19]:=448;intname[20]:=449;intname[21]:=450;
intname[22]:=451;intname[23]:=452;intname[24]:=453;intname[25]:=454;
intname[26]:=455;intname[27]:=456;intname[28]:=457;intname[29]:=458;
intname[30]:=459;intname[31]:=460;intname[32]:=461;intname[33]:=462;
{:211}{221:}hashused:=9757;stcount:=0;hash[9770].rh:=464;
hash[9768].rh:=465;hash[9769].rh:=466;hash[9766].rh:=467;
hash[9767].rh:=468;hash[9765].rh:=469;hash[9764].rh:=470;
hash[9763].rh:=59;hash[9762].rh:=58;hash[9761].rh:=47;hash[9760].rh:=91;
hash[9759].rh:=41;hash[9757].rh:=471;eqtb[9759].lh:=64;{:221}{233:}
mem[0].hh.rh:=0;mem[1].int:=0;{:233}{248:}mem[11].hh.lh:=9772;
mem[11].hh.rh:=0;{:248}{541:}serialno:=0;mem[5].hh.rh:=5;
mem[6].hh.lh:=5;mem[5].hh.lh:=0;mem[6].hh.rh:=0;{:541}{674:}
mem[22].hh.b1:=0;mem[22].hh.rh:=9770;eqtb[9770].rh:=22;
eqtb[9770].lh:=43;{:674}{732:}eqtb[9758].lh:=93;hash[9758].rh:=775;
{:732}{899:}mem[9].hh.b1:=14;{:899}{1147:}mem[12].int:=1073741824;
{:1147}{1158:}mem[8].int:=0;mem[7].hh.lh:=0;{:1158}{1177:}
fontdsize[0]:=0;fontname[0]:=284;fontpsname[0]:=284;fontbc[0]:=1;
fontec[0]:=0;charbase[0]:=0;widthbase[0]:=0;heightbase[0]:=0;
depthbase[0]:=0;nextfmem:=0;lastfnum:=0;lastpsfnum:=0;{:1177}{1279:}
if iniversion then memident:=1173;{:1279}end;endif('INIMP'){:1305}
{1307:}ifdef('TEXMF_DEBUG')procedure debughelp;label 888,10;
var k,l,m,n:integer;begin while true do begin;printnl(1188);
fflush(stdout);m:=inputint(stdin);
if m<0 then goto 10 else if m=0 then begin goto 888;
888:m:=0;{'BREAKPOINT'}
end else begin n:=inputint(stdin);case m of{1308:}1:printword(mem[n]);
2:printint(mem[n].hh.lh);3:printint(mem[n].hh.rh);
4:begin printint(eqtb[n].lh);printchar(58);printint(eqtb[n].rh);end;
5:printvariablename(n);6:printint(internal[n]);7:doshowdependencies;
9:showtokenlist(n,0,100000,0);10:print(n);11:checkmem(n>0);
12:searchmem(n);13:begin l:=inputint(stdin);printcmdmod(n,l);end;
14:for k:=0 to n do print(buffer[k]);15:panicking:=not panicking;{:1308}
others:print(63)end;end;end;10:end;endif('TEXMF_DEBUG'){:1307}{:1296}
{1298:}begin bounddefault:=250000;boundname:='main_memory';
setupboundvariable(addressof(mainmemory),boundname,bounddefault);;
bounddefault:=100000;boundname:='pool_size';
setupboundvariable(addressof(poolsize),boundname,bounddefault);;
bounddefault:=75000;boundname:='string_vacancies';
setupboundvariable(addressof(stringvacancies),boundname,bounddefault);;
bounddefault:=5000;boundname:='pool_free';
setupboundvariable(addressof(poolfree),boundname,bounddefault);;
bounddefault:=15000;boundname:='max_strings';
setupboundvariable(addressof(maxstrings),boundname,bounddefault);;
bounddefault:=79;boundname:='error_line';
setupboundvariable(addressof(errorline),boundname,bounddefault);;
bounddefault:=50;boundname:='half_error_line';
setupboundvariable(addressof(halferrorline),boundname,bounddefault);;
bounddefault:=79;boundname:='max_print_line';
setupboundvariable(addressof(maxprintline),boundname,bounddefault);;
if errorline>255 then errorline:=255;
begin if mainmemory<infmainmemory then mainmemory:=infmainmemory else if
mainmemory>supmainmemory then mainmemory:=supmainmemory end;
memtop:=0+mainmemory;memmax:=memtop;
begin if poolsize<infpoolsize then poolsize:=infpoolsize else if
poolsize>suppoolsize then poolsize:=suppoolsize end;
begin if stringvacancies<infstringvacancies then stringvacancies:=
infstringvacancies else if stringvacancies>supstringvacancies then
stringvacancies:=supstringvacancies end;
begin if poolfree<infpoolfree then poolfree:=infpoolfree else if
poolfree>suppoolfree then poolfree:=suppoolfree end;
begin if maxstrings<infmaxstrings then maxstrings:=infmaxstrings else if
maxstrings>supmaxstrings then maxstrings:=supmaxstrings end;
ifdef('INIMP')if iniversion then begin xmallocarray(mem,memtop-0);
xmallocarray(strref,maxstrings);xmallocarray(nextstr,maxstrings);
xmallocarray(strstart,maxstrings);xmallocarray(strpool,poolsize);end;
endif('INIMP')history:=3;;if readyalready=314159 then goto 1;{14:}
bad:=0;if(halferrorline<30)or(halferrorline>errorline-15)then bad:=1;
if maxprintline<60 then bad:=2;
if emergencylinelength<maxprintline then bad:=3;
if 1100>memtop then bad:=4;if 7919>9500 then bad:=5;
if headersize mod 4<>0 then bad:=6;
if(ligtablesize<255)or(ligtablesize>32510)then bad:=7;{:14}{169:}
ifdef('INIMP')if memmax<>memtop then bad:=8;
endif('INIMP')if memmax<memtop then bad:=8;
if(0>0)or(255<127)then bad:=9;if(0>0)or(268435455<32767)then bad:=10;
if(0<0)or(255>268435455)then bad:=11;
if(0<0)or(memmax>=268435455)then bad:=12;
if maxstrings>268435455 then bad:=13;if bufsize>268435455 then bad:=14;
if fontmax>268435455 then bad:=15;
if(255<255)or(268435455<65535)then bad:=16;{:169}{222:}
if 9771+maxinternal>268435455 then bad:=17;{:222}{232:}
if 10222>268435455 then bad:=18;{:232}{528:}
if 20+17*45>bistacksize then bad:=19;{:528}{754:}
if memdefaultlength>maxint then bad:=20;{:754}
if bad>0 then begin writeln(stdout,
'Ouch---my internal constants have been clobbered!','---case ',bad:1);
goto 9999;end;initialize;
ifdef('INIMP')if iniversion then begin if not getstringsstarted then
goto 9999;inittab;initprim;initstruse:=strptr;initpoolptr:=poolptr;
maxstrptr:=strptr;maxpoolptr:=poolptr;fixdateandtime;end;
endif('INIMP')readyalready:=314159;1:{70:}selector:=8;tally:=0;
termoffset:=0;fileoffset:=0;psoffset:=0;{:70}{76:}
write(stdout,'This is MetaPost, Version 0.641');
write(stdout,versionstring);if memident>0 then print(memident);println;
fflush(stdout);{:76}{761:}jobname:=0;logopened:=false;{:761};{1306:}
begin{616:}begin inputptr:=0;maxinstack:=0;inopen:=0;openparens:=0;
maxbufstack:=0;paramptr:=0;maxparamstack:=0;first:=1;
curinput.startfield:=1;curinput.indexfield:=0;
linestack[curinput.indexfield]:=0;curinput.namefield:=0;mpxname[0]:=1;
forceeof:=false;if not initterminal then goto 9999;
curinput.limitfield:=last;first:=last+1;end;{:616}{619:}
scannerstatus:=0;{:619};
if(memident=0)or(buffer[curinput.locfield]=38)or dumpline then begin if
memident<>0 then initialize;if not openmemfile then goto 9999;
if not loadmemfile then begin wclose(memfile);goto 9999;end;
wclose(memfile);
while(curinput.locfield<curinput.limitfield)and(buffer[curinput.locfield
]=32)do incr(curinput.locfield);end;buffer[curinput.limitfield]:=37;
fixdateandtime;initrandoms((internal[16]div 65536)+internal[15]);{85:}
if interaction=0 then selector:=7 else selector:=8{:85};
if curinput.locfield<curinput.limitfield then if buffer[curinput.
locfield]<>92 then startinput;end{:1306};history:=0;
if troffmode then internal[32]:=65536;
if startsym>0 then begin cursym:=startsym;backinput;end;maincontrol;
finalcleanup;closefilesandterminate;9999:begin fflush(stdout);
readyalready:=0;if(history<>0)and(history<>1)then uexit(1)else uexit(0);
end;end.{:1298}
