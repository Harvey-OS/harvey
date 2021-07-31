{4:}{9:}{:9}program TEX;const{11:}memmax=262140;memmin=0;bufsize=3000;
errorline=79;halferrorline=50;maxprintline=79;stacksize=300;
maxinopen=15;fontmax=255;fontmemsize=72000;paramsize=60;nestsize=40;
maxstrings=7500;stringvacancies=74000;poolsize=100000;savesize=4000;
triesize=8000;trieopsize=500;negtrieopsize=-500;dvibufsize=16384;
poolname='tex.pool';memtop=262140;{:11}type{18:}ASCIIcode=0..255;{:18}
{25:}eightbits=0..255;{:25}{38:}poolpointer=0..poolsize;
strnumber=0..maxstrings;packedASCIIcode=0..255;{:38}{101:}
scaled=integer;nonnegativeinteger=0..2147483647;smallnumber=0..63;{:101}
{109:}{:109}{113:}quarterword=0..511;halfword=0..262143;twochoices=1..2;
fourchoices=1..4;#include "memory.h";{:113}{150:}glueord=0..3;{:150}
{212:}liststaterecord=record modefield:-203..203;
headfield,tailfield:halfword;pgfield,mlfield:integer;
auxfield:memoryword;lhmfield,rhmfield:quarterword;end;{:212}{269:}
groupcode=0..16;{:269}{300:}
instaterecord=record statefield,indexfield:quarterword;
startfield,locfield,limitfield,namefield:halfword;end;{:300}{548:}
internalfontnumber=0..fontmax;fontindex=0..fontmemsize;{:548}{594:}
dviindex=0..dvibufsize;{:594}{920:}triepointer=0..triesize;{:920}{925:}
hyphpointer=0..607;{:925}var{13:}bad:integer;{:13}{20:}
xord:array[ASCIIcode]of ASCIIcode;xchr:array[ASCIIcode]of ASCIIcode;
{:20}{26:}
nameoffile,realnameoffile:packed array[1..FILENAMESIZE]of char;
namelength:0..FILENAMESIZE;{:26}{30:}
buffer:array[0..bufsize]of ASCIIcode;first:0..bufsize;last:0..bufsize;
maxbufstack:0..bufsize;{:30}{39:}
strpool:packed array[poolpointer]of packedASCIIcode;
strstart:array[strnumber]of poolpointer;poolptr:poolpointer;
strptr:strnumber;initpoolptr:poolpointer;initstrptr:strnumber;{:39}{50:}
ifdef('INITEX')poolfile:alphafile;endif('INITEX'){:50}{54:}
logfile:alphafile;selector:0..21;dig:array[0..22]of 0..15;tally:integer;
termoffset:0..maxprintline;fileoffset:0..maxprintline;
trickbuf:array[0..errorline]of ASCIIcode;trickcount:integer;
firstcount:integer;{:54}{73:}interaction:0..3;{:73}{76:}
deletionsallowed:boolean;history:0..3;errorcount:-1..100;{:76}{79:}
helpline:array[0..5]of strnumber;helpptr:0..6;useerrhelp:boolean;{:79}
{96:}interrupt:integer;OKtointerrupt:boolean;{:96}{104:}
aritherror:boolean;remainder:scaled;{:104}{115:}tempptr:halfword;{:115}
{116:}zmem:array[memmin..memmax]of memoryword;lomemmax:halfword;
himemmin:halfword;{:116}{117:}varused,dynused:integer;{:117}{118:}
avail:halfword;memend:halfword;{:118}{124:}rover:halfword;{:124}{165:}
ifdef('DEBUG')freearr:packed array[memmin..memmax]of boolean;
wasfree:packed array[memmin..memmax]of boolean;
wasmemend,waslomax,washimin:halfword;panicking:boolean;
endif('DEBUG'){:165}{173:}fontinshortdisplay:integer;{:173}{181:}
depththreshold:integer;breadthmax:integer;{:181}{213:}
nest:array[0..nestsize]of liststaterecord;nestptr:0..nestsize;
maxneststack:0..nestsize;curlist:liststaterecord;shownmode:-203..203;
{:213}{246:}oldsetting:0..21;{:246}{253:}
zeqtb:array[1..13506]of memoryword;
xeqlevel:array[12663..13506]of quarterword;{:253}{256:}
hash:array[514..10280]of twohalves;hashused:halfword;
nonewcontrolsequence:boolean;cscount:integer;{:256}{271:}
savestack:array[0..savesize]of memoryword;saveptr:0..savesize;
maxsavestack:0..savesize;curlevel:quarterword;curgroup:groupcode;
curboundary:0..savesize;{:271}{286:}magset:integer;{:286}{297:}
curcmd:eightbits;curchr:halfword;curcs:halfword;curtok:halfword;{:297}
{301:}inputstack:array[0..stacksize]of instaterecord;
inputptr:0..stacksize;maxinstack:0..stacksize;curinput:instaterecord;
{:301}{304:}inopen:0..maxinopen;openparens:0..maxinopen;
inputfile:array[1..maxinopen]of alphafile;line:integer;
linestack:array[1..maxinopen]of integer;{:304}{305:}scannerstatus:0..5;
warningindex:halfword;defref:halfword;{:305}{308:}
paramstack:array[0..paramsize]of halfword;paramptr:0..paramsize;
maxparamstack:integer;{:308}{309:}alignstate:integer;{:309}{310:}
baseptr:0..stacksize;{:310}{333:}parloc:halfword;partoken:halfword;
{:333}{361:}forceeof:boolean;{:361}{382:}curmark:array[0..4]of halfword;
{:382}{387:}longstate:111..114;{:387}{388:}
pstack:array[0..8]of halfword;{:388}{410:}curval:integer;
curvallevel:0..5;{:410}{438:}radix:smallnumber;{:438}{447:}
curorder:glueord;{:447}{480:}readfile:array[0..15]of alphafile;
readopen:array[0..16]of 0..2;{:480}{489:}condptr:halfword;iflimit:0..4;
curif:smallnumber;ifline:integer;{:489}{493:}skipline:integer;{:493}
{512:}curname:strnumber;curarea:strnumber;curext:strnumber;{:512}{513:}
areadelimiter:poolpointer;extdelimiter:poolpointer;{:513}{520:}
TEXformatdefault:ccharpointer;{:520}{527:}nameinprogress:boolean;
jobname:strnumber;logopened:boolean;{:527}{532:}dvifile:bytefile;
outputfilename:strnumber;logname:strnumber;{:532}{539:}tfmfile:bytefile;
{:539}{549:}fontinfo:array[fontindex]of memoryword;fmemptr:fontindex;
fontptr:internalfontnumber;
fontcheck:array[internalfontnumber]of fourquarters;
fontsize:array[internalfontnumber]of scaled;
fontdsize:array[internalfontnumber]of scaled;
fontparams:array[internalfontnumber]of halfword;
fontname:array[internalfontnumber]of strnumber;
fontarea:array[internalfontnumber]of strnumber;
fontbc:array[internalfontnumber]of eightbits;
fontec:array[internalfontnumber]of eightbits;
fontglue:array[internalfontnumber]of halfword;
fontused:array[internalfontnumber]of boolean;
hyphenchar:array[internalfontnumber]of integer;
skewchar:array[internalfontnumber]of integer;
bcharlabel:array[internalfontnumber]of fontindex;
fontbchar:array[internalfontnumber]of 0..256;
fontfalsebchar:array[internalfontnumber]of 0..256;{:549}{550:}
charbase:array[internalfontnumber]of integer;
widthbase:array[internalfontnumber]of integer;
heightbase:array[internalfontnumber]of integer;
depthbase:array[internalfontnumber]of integer;
italicbase:array[internalfontnumber]of integer;
ligkernbase:array[internalfontnumber]of integer;
kernbase:array[internalfontnumber]of integer;
extenbase:array[internalfontnumber]of integer;
parambase:array[internalfontnumber]of integer;{:550}{555:}
nullcharacter:fourquarters;{:555}{592:}totalpages:integer;maxv:scaled;
maxh:scaled;maxpush:integer;lastbop:integer;deadcycles:integer;
doingleaders:boolean;c,f:quarterword;ruleht,ruledp,rulewd:scaled;
g:halfword;lq,lr:integer;{:592}{595:}dvibuf:array[dviindex]of eightbits;
halfbuf:dviindex;dvilimit:dviindex;dviptr:dviindex;dvioffset:integer;
dvigone:integer;{:595}{605:}downptr,rightptr:halfword;{:605}{616:}
dvih,dviv:scaled;curh,curv:scaled;dvif:internalfontnumber;curs:integer;
{:616}{646:}totalstretch,totalshrink:array[glueord]of scaled;
lastbadness:integer;{:646}{647:}adjusttail:halfword;{:647}{661:}
packbeginline:integer;{:661}{684:}emptyfield:twohalves;
nulldelimiter:fourquarters;{:684}{719:}curmlist:halfword;
curstyle:smallnumber;cursize:smallnumber;curmu:scaled;
mlistpenalties:boolean;{:719}{724:}curf:internalfontnumber;
curc:quarterword;curi:fourquarters;{:724}{764:}magicoffset:integer;
{:764}{770:}curalign:halfword;curspan:halfword;curloop:halfword;
alignptr:halfword;curhead,curtail:halfword;{:770}{814:}justbox:halfword;
{:814}{821:}passive:halfword;printednode:halfword;passnumber:halfword;
{:821}{823:}activewidth:array[1..6]of scaled;
curactivewidth:array[1..6]of scaled;background:array[1..6]of scaled;
breakwidth:array[1..6]of scaled;{:823}{825:}noshrinkerroryet:boolean;
{:825}{828:}curp:halfword;secondpass:boolean;finalpass:boolean;
threshold:integer;{:828}{833:}minimaldemerits:array[0..3]of integer;
minimumdemerits:integer;bestplace:array[0..3]of halfword;
bestplline:array[0..3]of halfword;{:833}{839:}discwidth:scaled;{:839}
{847:}easyline:halfword;lastspecialline:halfword;firstwidth:scaled;
secondwidth:scaled;firstindent:scaled;secondindent:scaled;{:847}{872:}
bestbet:halfword;fewestdemerits:integer;bestline:halfword;
actuallooseness:integer;linediff:integer;{:872}{892:}
hc:array[0..65]of 0..256;hn:smallnumber;ha,hb:halfword;
hf:internalfontnumber;hu:array[0..63]of 0..256;hyfchar:integer;
curlang:ASCIIcode;lhyf,rhyf:integer;{:892}{900:}hyf:array[0..64]of 0..9;
initlist:halfword;initlig:boolean;initlft:boolean;{:900}{905:}
hyphenpassed:smallnumber;{:905}{907:}curl,curr:halfword;curq:halfword;
ligstack:halfword;ligaturepresent:boolean;lfthit,rthit:boolean;{:907}
{921:}trie:array[triepointer]of twohalves;
hyfdistance:array[1..trieopsize]of smallnumber;
hyfnum:array[1..trieopsize]of smallnumber;
hyfnext:array[1..trieopsize]of quarterword;
opstart:array[ASCIIcode]of 0..trieopsize;{:921}{926:}
hyphword:array[hyphpointer]of strnumber;
hyphlist:array[hyphpointer]of halfword;hyphcount:hyphpointer;{:926}
{943:}ifdef('INITEX')trieophash:array[negtrieopsize..trieopsize]of 0..
trieopsize;trieused:array[ASCIIcode]of quarterword;
trieoplang:array[1..trieopsize]of ASCIIcode;
trieopval:array[1..trieopsize]of quarterword;trieopptr:0..trieopsize;
endif('INITEX'){:943}{947:}
ifdef('INITEX')triec:packed array[triepointer]of packedASCIIcode;
trieo:packed array[triepointer]of quarterword;
triel:packed array[triepointer]of triepointer;
trier:packed array[triepointer]of triepointer;trieptr:triepointer;
triehash:packed array[triepointer]of triepointer;endif('INITEX'){:947}
{950:}ifdef('INITEX')trietaken:packed array[1..triesize]of boolean;
triemin:array[ASCIIcode]of triepointer;triemax:triepointer;
trienotready:boolean;endif('INITEX'){:950}{971:}
bestheightplusdepth:scaled;{:971}{980:}pagetail:halfword;
pagecontents:0..2;pagemaxdepth:scaled;bestpagebreak:halfword;
leastpagecost:integer;bestsize:scaled;{:980}{982:}
pagesofar:array[0..7]of scaled;lastglue:halfword;lastpenalty:integer;
lastkern:scaled;insertpenalties:integer;{:982}{989:}
outputactive:boolean;{:989}{1032:}mainf:internalfontnumber;
maini:fourquarters;mainj:fourquarters;maink:fontindex;mainp:halfword;
mains:integer;bchar:halfword;falsebchar:halfword;cancelboundary:boolean;
insdisc:boolean;{:1032}{1074:}curbox:halfword;{:1074}{1266:}
aftertoken:halfword;{:1266}{1281:}longhelpseen:boolean;{:1281}{1299:}
formatident:strnumber;{:1299}{1305:}fmtfile:wordfile;{:1305}{1331:}
readyalready:integer;{:1331}{1342:}writefile:array[0..15]of alphafile;
writeopen:array[0..17]of boolean;{:1342}{1345:}writeloc:halfword;{:1345}
{1379:}editnamestart:poolpointer;
editnamelength,editline,tfmtemp:integer;{:1379}procedure initialize;
var{19:}i:integer;{:19}{163:}k:integer;{:163}{927:}z:hyphpointer;{:927}
begin{8:}{21:}xchr[32]:=' ';xchr[33]:='!';xchr[34]:='"';xchr[35]:='#';
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
xchr[125]:='}';xchr[126]:='~';{:21}{23:}
for i:=0 to 31 do xchr[i]:=chr(i);for i:=127 to 255 do xchr[i]:=chr(i);
{:23}{24:}for i:=0 to 255 do xord[chr(i)]:=127;
for i:=128 to 255 do xord[xchr[i]]:=i;
for i:=0 to 126 do xord[xchr[i]]:=i;{:24}{74:}interaction:=3;{:74}{77:}
deletionsallowed:=true;errorcount:=0;{:77}{80:}helpptr:=0;
useerrhelp:=false;{:80}{97:}interrupt:=0;OKtointerrupt:=true;{:97}{166:}
ifdef('DEBUG')wasmemend:=memmin;waslomax:=memmin;washimin:=memmax;
panicking:=false;endif('DEBUG'){:166}{215:}nestptr:=0;maxneststack:=0;
curlist.modefield:=1;curlist.headfield:=memtop-1;
curlist.tailfield:=memtop-1;curlist.auxfield.int:=-65536000;
curlist.mlfield:=0;curlist.lhmfield:=0;curlist.rhmfield:=0;
curlist.pgfield:=0;shownmode:=0;{991:}pagecontents:=0;
pagetail:=memtop-2;mem[memtop-2].hh.rh:=0;lastglue:=262143;
lastpenalty:=0;lastkern:=0;pagesofar[7]:=0;pagemaxdepth:=0{:991};{:215}
{254:}for k:=12663 to 13506 do xeqlevel[k]:=1;{:254}{257:}
nonewcontrolsequence:=true;hash[514].lh:=0;hash[514].rh:=0;
for k:=515 to 10280 do hash[k]:=hash[514];{:257}{272:}saveptr:=0;
curlevel:=1;curgroup:=0;curboundary:=0;maxsavestack:=0;{:272}{287:}
magset:=0;{:287}{383:}curmark[0]:=0;curmark[1]:=0;curmark[2]:=0;
curmark[3]:=0;curmark[4]:=0;{:383}{439:}curval:=0;curvallevel:=0;
radix:=0;curorder:=0;{:439}{481:}for k:=0 to 16 do readopen[k]:=2;{:481}
{490:}condptr:=0;iflimit:=0;curif:=0;ifline:=0;{:490}{521:}
TEXformatdefault:=' plain.fmt';{:521}{551:}
for k:=0 to fontmax do fontused[k]:=false;{:551}{556:}
nullcharacter.b0:=0;nullcharacter.b1:=0;nullcharacter.b2:=0;
nullcharacter.b3:=0;{:556}{593:}totalpages:=0;maxv:=0;maxh:=0;
maxpush:=0;lastbop:=-1;doingleaders:=false;deadcycles:=0;curs:=-1;{:593}
{596:}halfbuf:=dvibufsize div 2;dvilimit:=dvibufsize;dviptr:=0;
dvioffset:=0;dvigone:=0;{:596}{606:}downptr:=0;rightptr:=0;{:606}{648:}
adjusttail:=0;lastbadness:=0;{:648}{662:}packbeginline:=0;{:662}{685:}
emptyfield.rh:=0;emptyfield.lh:=0;nulldelimiter.b0:=0;
nulldelimiter.b1:=0;nulldelimiter.b2:=0;nulldelimiter.b3:=0;{:685}{771:}
alignptr:=0;curalign:=0;curspan:=0;curloop:=0;curhead:=0;curtail:=0;
{:771}{928:}for z:=0 to 607 do begin hyphword[z]:=0;hyphlist[z]:=0;end;
hyphcount:=0;{:928}{990:}outputactive:=false;insertpenalties:=0;{:990}
{1033:}ligaturepresent:=false;cancelboundary:=false;lfthit:=false;
rthit:=false;insdisc:=false;{:1033}{1267:}aftertoken:=0;{:1267}{1282:}
longhelpseen:=false;{:1282}{1300:}formatident:=0;{:1300}{1343:}
for k:=0 to 17 do writeopen[k]:=false;{:1343}{1380:}editnamestart:=0;
{:1380}ifdef('INITEX'){164:}for k:=1 to 19 do mem[k].int:=0;k:=0;
while k<=19 do begin mem[k].hh.rh:=1;mem[k].hh.b0:=0;mem[k].hh.b1:=0;
k:=k+4;end;mem[6].int:=65536;mem[4].hh.b0:=1;mem[10].int:=65536;
mem[8].hh.b0:=2;mem[14].int:=65536;mem[12].hh.b0:=1;mem[15].int:=65536;
mem[12].hh.b1:=1;mem[18].int:=-65536;mem[16].hh.b0:=1;rover:=20;
mem[rover].hh.rh:=262143;mem[rover].hh.lh:=1000;
mem[rover+1].hh.lh:=rover;mem[rover+1].hh.rh:=rover;
lomemmax:=rover+1000;mem[lomemmax].hh.rh:=0;mem[lomemmax].hh.lh:=0;
for k:=memtop-13 to memtop do mem[k]:=mem[lomemmax];{790:}
mem[memtop-10].hh.lh:=14114;{:790}{797:}mem[memtop-9].hh.rh:=512;
mem[memtop-9].hh.lh:=0;{:797}{820:}mem[memtop-7].hh.b0:=1;
mem[memtop-6].hh.lh:=262143;mem[memtop-7].hh.b1:=0;{:820}{981:}
mem[memtop].hh.b1:=255;mem[memtop].hh.b0:=1;mem[memtop].hh.rh:=memtop;
{:981}{988:}mem[memtop-2].hh.b0:=10;mem[memtop-2].hh.b1:=0;{:988};
avail:=0;memend:=memtop;himemmin:=memtop-13;varused:=20;dynused:=14;
{:164}{222:}eqtb[10281].hh.b0:=101;eqtb[10281].hh.rh:=0;
eqtb[10281].hh.b1:=0;for k:=1 to 10280 do eqtb[k]:=eqtb[10281];{:222}
{228:}eqtb[10282].hh.rh:=0;eqtb[10282].hh.b1:=1;eqtb[10282].hh.b0:=117;
for k:=10283 to 10811 do eqtb[k]:=eqtb[10282];
mem[0].hh.rh:=mem[0].hh.rh+530;{:228}{232:}eqtb[10812].hh.rh:=0;
eqtb[10812].hh.b0:=118;eqtb[10812].hh.b1:=1;
for k:=10813 to 11077 do eqtb[k]:=eqtb[10281];eqtb[11078].hh.rh:=0;
eqtb[11078].hh.b0:=119;eqtb[11078].hh.b1:=1;
for k:=11079 to 11333 do eqtb[k]:=eqtb[11078];eqtb[11334].hh.rh:=0;
eqtb[11334].hh.b0:=120;eqtb[11334].hh.b1:=1;
for k:=11335 to 11382 do eqtb[k]:=eqtb[11334];eqtb[11383].hh.rh:=0;
eqtb[11383].hh.b0:=120;eqtb[11383].hh.b1:=1;
for k:=11384 to 12662 do eqtb[k]:=eqtb[11383];
for k:=0 to 255 do begin eqtb[11383+k].hh.rh:=12;eqtb[12407+k].hh.rh:=k;
eqtb[12151+k].hh.rh:=1000;end;eqtb[11396].hh.rh:=5;
eqtb[11415].hh.rh:=10;eqtb[11475].hh.rh:=0;eqtb[11420].hh.rh:=14;
eqtb[11510].hh.rh:=15;eqtb[11383].hh.rh:=9;
for k:=48 to 57 do eqtb[12407+k].hh.rh:=k+28672;
for k:=65 to 90 do begin eqtb[11383+k].hh.rh:=11;
eqtb[11383+k+32].hh.rh:=11;eqtb[12407+k].hh.rh:=k+28928;
eqtb[12407+k+32].hh.rh:=k+28960;eqtb[11639+k].hh.rh:=k+32;
eqtb[11639+k+32].hh.rh:=k+32;eqtb[11895+k].hh.rh:=k;
eqtb[11895+k+32].hh.rh:=k;eqtb[12151+k].hh.rh:=999;end;{:232}{240:}
for k:=12663 to 12973 do eqtb[k].int:=0;eqtb[12680].int:=1000;
eqtb[12664].int:=10000;eqtb[12704].int:=1;eqtb[12703].int:=25;
eqtb[12708].int:=92;eqtb[12711].int:=13;
for k:=0 to 255 do eqtb[12974+k].int:=-1;eqtb[13020].int:=0;{:240}{250:}
for k:=13230 to 13506 do eqtb[k].int:=0;{:250}{258:}hashused:=10014;
cscount:=0;eqtb[10023].hh.b0:=116;hash[10023].rh:=498;{:258}{552:}
fontptr:=0;fmemptr:=7;fontname[0]:=794;fontarea[0]:=335;
hyphenchar[0]:=45;skewchar[0]:=-1;fontbc[0]:=1;fontec[0]:=0;
fontsize[0]:=0;fontdsize[0]:=0;charbase[0]:=0;widthbase[0]:=0;
heightbase[0]:=0;depthbase[0]:=0;italicbase[0]:=0;ligkernbase[0]:=0;
kernbase[0]:=0;extenbase[0]:=0;fontglue[0]:=0;fontparams[0]:=7;
parambase[0]:=-1;for k:=0 to 6 do fontinfo[k].int:=0;{:552}{946:}
for k:=-trieopsize to trieopsize do trieophash[k]:=0;
for k:=0 to 255 do trieused[k]:=0;trieopptr:=0;{:946}{951:}
trienotready:=true;triel[0]:=0;triec[0]:=0;trieptr:=0;{:951}{1216:}
hash[10014].rh:=1183;{:1216}{1301:}formatident:=1248;{:1301}{1369:}
hash[10022].rh:=1286;eqtb[10022].hh.b1:=1;eqtb[10022].hh.b0:=113;
eqtb[10022].hh.rh:=0;{:1369}endif('INITEX'){:8}end;{57:}
procedure println;begin case selector of 19:begin writeln(stdout);
writeln(logfile);termoffset:=0;fileoffset:=0;end;
18:begin writeln(logfile);fileoffset:=0;end;17:begin writeln(stdout);
termoffset:=0;end;16,20,21:;others:writeln(writefile[selector])end;end;
{:57}{58:}procedure printchar(s:ASCIIcode);label 10;begin if{244:}
s=eqtb[12712].int{:244}then if selector<20 then begin println;goto 10;
end;case selector of 19:begin write(stdout,xchr[s]);
write(logfile,xchr[s]);incr(termoffset);incr(fileoffset);
if termoffset=maxprintline then begin writeln(stdout);termoffset:=0;end;
if fileoffset=maxprintline then begin writeln(logfile);fileoffset:=0;
end;end;18:begin write(logfile,xchr[s]);incr(fileoffset);
if fileoffset=maxprintline then println;end;
17:begin write(stdout,xchr[s]);incr(termoffset);
if termoffset=maxprintline then println;end;16:;
20:if tally<trickcount then trickbuf[tally mod errorline]:=s;
21:begin if poolptr<poolsize then begin strpool[poolptr]:=s;
incr(poolptr);end;end;others:write(writefile[selector],xchr[s])end;
incr(tally);10:end;{:58}{59:}procedure print(s:integer);label 10;
var j:poolpointer;
begin if s>=strptr then s:=259 else if s<256 then if s<0 then s:=259
else if({244:}s=eqtb[12712].int{:244}
)then if selector<20 then begin println;goto 10;end;j:=strstart[s];
while j<strstart[s+1]do begin printchar(strpool[j]);incr(j);end;10:end;
{:59}{60:}procedure slowprint(s:integer);label 10;var j:poolpointer;
begin if s>=strptr then s:=259 else if s<256 then if s<0 then s:=259
else if({244:}s=eqtb[12712].int{:244}
)then if selector<20 then begin println;goto 10;end;j:=strstart[s];
while j<strstart[s+1]do begin print(strpool[j]);incr(j);end;10:end;{:60}
{62:}procedure printnl(s:strnumber);
begin if((termoffset>0)and(odd(selector)))or((fileoffset>0)and(selector
>=18))then println;print(s);end;{:62}{63:}
procedure printesc(s:strnumber);var c:integer;begin{243:}
c:=eqtb[12708].int{:243};if c>=0 then if c<256 then print(c);print(s);
end;{:63}{64:}procedure printthedigs(k:eightbits);
begin while k>0 do begin decr(k);
if dig[k]<10 then printchar(48+dig[k])else printchar(55+dig[k]);end;end;
{:64}{65:}procedure printint(n:integer);var k:0..23;m:integer;
begin k:=0;if n<0 then begin printchar(45);
if n>-100000000 then n:=-n else begin m:=-1-n;n:=m div 10;
m:=(m mod 10)+1;k:=1;if m<10 then dig[0]:=m else begin dig[0]:=0;
incr(n);end;end;end;repeat dig[k]:=n mod 10;n:=n div 10;incr(k);
until n=0;printthedigs(k);end;{:65}{262:}procedure printcs(p:integer);
begin if p<514 then if p>=257 then if p=513 then begin printesc(500);
printesc(501);end else begin printesc(p-257);
if eqtb[11383+p-257].hh.rh=11 then printchar(32);
end else if p<1 then printesc(502)else print(p-1)else if p>=10281 then
printesc(502)else if(hash[p].rh>=strptr)then printesc(503)else begin
printesc(335);slowprint(hash[p].rh);printchar(32);end;end;{:262}{263:}
procedure sprintcs(p:halfword);
begin if p<514 then if p<257 then print(p-1)else if p<513 then printesc(
p-257)else begin printesc(500);printesc(501);
end else begin printesc(335);slowprint(hash[p].rh);end;end;{:263}{518:}
procedure printfilename(n,a,e:integer);begin print(a);print(n);print(e);
end;{:518}{699:}procedure printsize(s:integer);
begin if s=0 then printesc(408)else if s=16 then printesc(409)else
printesc(410);end;{:699}{1355:}procedure printwritewhatsit(s:strnumber;
p:halfword);begin printesc(s);
if mem[p+1].hh.lh<16 then printint(mem[p+1].hh.lh)else if mem[p+1].hh.lh
=16 then printchar(42)else printchar(45);end;{:1355}{78:}
procedure normalizeselector;forward;procedure gettoken;forward;
procedure terminput;forward;procedure showcontext;forward;
procedure beginfilereading;forward;procedure openlogfile;forward;
procedure closefilesandterminate;forward;procedure clearforerrorprompt;
forward;procedure giveerrhelp;forward;ifdef('DEBUG')procedure debughelp;
forward;endif('DEBUG'){:78}{81:}procedure jumpout;
begin closefilesandterminate;begin termflush(stdout);readyalready:=0;
if(history<>0)and(history<>1)then uexit(1)else uexit(0);end;end;{:81}
{82:}procedure error;label 22,10;var c:ASCIIcode;s1,s2,s3,s4:integer;
begin if history<2 then history:=2;printchar(46);showcontext;
if interaction=3 then{83:}while true do begin 22:clearforerrorprompt;
begin wakeupterminal;print(264);terminput;end;
if last=first then goto 10;c:=buffer[first];if c>=97 then c:=c-32;{84:}
case c of 48,49,50,51,52,53,54,55,56,57:if deletionsallowed then{88:}
begin s1:=curtok;s2:=curcmd;s3:=curchr;s4:=alignstate;
alignstate:=1000000;OKtointerrupt:=false;
if(last>first+1)and(buffer[first+1]>=48)and(buffer[first+1]<=57)then c:=
c*10+buffer[first+1]-48*11 else c:=c-48;while c>0 do begin gettoken;
decr(c);end;curtok:=s1;curcmd:=s2;curchr:=s3;alignstate:=s4;
OKtointerrupt:=true;begin helpptr:=2;helpline[1]:=277;helpline[0]:=278;
end;showcontext;goto 22;end{:88};ifdef('DEBUG')68:begin debughelp;
goto 22;end;
endif('DEBUG')69:if baseptr>0 then begin editnamestart:=strstart[
inputstack[baseptr].namefield];
editnamelength:=strstart[inputstack[baseptr].namefield+1]-strstart[
inputstack[baseptr].namefield];editline:=line;jumpout;end;72:{89:}
begin if useerrhelp then begin giveerrhelp;useerrhelp:=false;
end else begin if helpptr=0 then begin helpptr:=2;helpline[1]:=279;
helpline[0]:=280;end;repeat decr(helpptr);print(helpline[helpptr]);
println;until helpptr=0;end;begin helpptr:=4;helpline[3]:=281;
helpline[2]:=280;helpline[1]:=282;helpline[0]:=283;end;goto 22;end{:89};
73:{87:}begin beginfilereading;
if last>first+1 then begin curinput.locfield:=first+1;buffer[first]:=32;
end else begin begin wakeupterminal;print(276);terminput;end;
curinput.locfield:=first;end;first:=last;curinput.limitfield:=last-1;
goto 10;end{:87};81,82,83:{86:}begin errorcount:=0;interaction:=0+c-81;
print(271);case c of 81:begin printesc(272);decr(selector);end;
82:printesc(273);83:printesc(274);end;print(275);println;
termflush(stdout);goto 10;end{:86};88:begin interaction:=2;jumpout;end;
others:end;{85:}begin print(265);printnl(266);printnl(267);
if baseptr>0 then print(268);if deletionsallowed then printnl(269);
printnl(270);end{:85}{:84};end{:83};incr(errorcount);
if errorcount=100 then begin printnl(263);history:=3;jumpout;end;{90:}
if interaction>0 then decr(selector);if useerrhelp then begin println;
giveerrhelp;end else while helpptr>0 do begin decr(helpptr);
printnl(helpline[helpptr]);end;println;
if interaction>0 then incr(selector);println{:90};10:end;{:82}{93:}
procedure fatalerror(s:strnumber);begin normalizeselector;
begin if interaction=3 then wakeupterminal;printnl(262);print(285);end;
begin helpptr:=1;helpline[0]:=s;end;
begin if interaction=3 then interaction:=2;if logopened then error;
ifdef('DEBUG')if interaction>0 then debughelp;endif('DEBUG')history:=3;
jumpout;end;end;{:93}{94:}procedure overflow(s:strnumber;n:integer);
begin normalizeselector;begin if interaction=3 then wakeupterminal;
printnl(262);print(286);end;print(s);printchar(61);printint(n);
printchar(93);begin helpptr:=2;helpline[1]:=287;helpline[0]:=288;end;
begin if interaction=3 then interaction:=2;if logopened then error;
ifdef('DEBUG')if interaction>0 then debughelp;endif('DEBUG')history:=3;
jumpout;end;end;{:94}{95:}procedure confusion(s:strnumber);
begin normalizeselector;
if history<2 then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(289);end;print(s);printchar(41);begin helpptr:=1;
helpline[0]:=290;end;
end else begin begin if interaction=3 then wakeupterminal;printnl(262);
print(291);end;begin helpptr:=2;helpline[1]:=292;helpline[0]:=293;end;
end;begin if interaction=3 then interaction:=2;if logopened then error;
ifdef('DEBUG')if interaction>0 then debughelp;endif('DEBUG')history:=3;
jumpout;end;end;{:95}{:4}{37:}function initterminal:boolean;label 10;
begin topenin;if last>first then begin curinput.locfield:=first;
while(curinput.locfield<last)and(buffer[curinput.locfield]=' ')do incr(
curinput.locfield);
if curinput.locfield<last then begin initterminal:=true;goto 10;end;end;
while true do begin wakeupterminal;write(stdout,'**');termflush(stdout);
if not inputln(stdin,true)then begin writeln(stdout);
write(stdout,'! End of file on the terminal... why?');
initterminal:=false;goto 10;end;curinput.locfield:=first;
while(curinput.locfield<last)and(buffer[curinput.locfield]=32)do incr(
curinput.locfield);
if curinput.locfield<last then begin initterminal:=true;goto 10;end;
writeln(stdout,'Please type the name of your input file.');end;10:end;
{:37}{43:}function makestring:strnumber;
begin if strptr=maxstrings then overflow(258,maxstrings-initstrptr);
incr(strptr);strstart[strptr]:=poolptr;makestring:=strptr-1;end;{:43}
{45:}function streqbuf(s:strnumber;k:integer):boolean;label 45;
var j:poolpointer;result:boolean;begin j:=strstart[s];
while j<strstart[s+1]do begin if strpool[j]<>buffer[k]then begin result
:=false;goto 45;end;incr(j);incr(k);end;result:=true;
45:streqbuf:=result;end;{:45}{46:}
function streqstr(s,t:strnumber):boolean;label 45;var j,k:poolpointer;
result:boolean;begin result:=false;
if(strstart[s+1]-strstart[s])<>(strstart[t+1]-strstart[t])then goto 45;
j:=strstart[s];k:=strstart[t];
while j<strstart[s+1]do begin if strpool[j]<>strpool[k]then goto 45;
incr(j);incr(k);end;result:=true;45:streqstr:=result;end;{:46}{47:}
ifdef('INITEX')function getstringsstarted:boolean;label 30,10;
var k,l:0..255;m,n:ASCIIcode;g:strnumber;a:integer;c:boolean;
begin poolptr:=0;strptr:=0;strstart[0]:=0;{48:}
for k:=0 to 255 do begin if({49:}(k<32)or(k>126){:49}
)then begin begin strpool[poolptr]:=94;incr(poolptr);end;
begin strpool[poolptr]:=94;incr(poolptr);end;
if k<64 then begin strpool[poolptr]:=k+64;incr(poolptr);
end else if k<128 then begin strpool[poolptr]:=k-64;incr(poolptr);
end else begin l:=k div 16;if l<10 then begin strpool[poolptr]:=l+48;
incr(poolptr);end else begin strpool[poolptr]:=l+87;incr(poolptr);end;
l:=k mod 16;if l<10 then begin strpool[poolptr]:=l+48;incr(poolptr);
end else begin strpool[poolptr]:=l+87;incr(poolptr);end;end;
end else begin strpool[poolptr]:=k;incr(poolptr);end;g:=makestring;
end{:48};{51:}vstrcpy(nameoffile+1,poolname);
if aopenin(poolfile,poolpathspec)then begin c:=false;repeat{52:}
begin if eof(poolfile)then begin wakeupterminal;
writeln(stdout,'! tex.pool has no check sum.');aclose(poolfile);
getstringsstarted:=false;goto 10;end;read(poolfile,m);read(poolfile,n);
if m='*'then{53:}begin a:=0;k:=1;
while true do begin if(xord[n]<48)or(xord[n]>57)then begin
wakeupterminal;
writeln(stdout,'! tex.pool check sum doesn''t have nine digits.');
aclose(poolfile);getstringsstarted:=false;goto 10;end;
a:=10*a+xord[n]-48;if k=9 then goto 30;incr(k);read(poolfile,n);end;
30:if a<>127541235 then begin wakeupterminal;
writeln(stdout,'! tex.pool doesn''t match; tangle me again.');
aclose(poolfile);getstringsstarted:=false;goto 10;end;c:=true;end{:53}
else begin if(xord[m]<48)or(xord[m]>57)or(xord[n]<48)or(xord[n]>57)then
begin wakeupterminal;
writeln(stdout,'! tex.pool line doesn''t begin with two digits.');
aclose(poolfile);getstringsstarted:=false;goto 10;end;
l:=xord[m]*10+xord[n]-48*11;
if poolptr+l+stringvacancies>poolsize then begin wakeupterminal;
writeln(stdout,'! You have to increase POOLSIZE.');aclose(poolfile);
getstringsstarted:=false;goto 10;end;
for k:=1 to l do begin if eoln(poolfile)then m:=' 'else read(poolfile,m)
;begin strpool[poolptr]:=xord[m];incr(poolptr);end;end;readln(poolfile);
g:=makestring;end;end{:52};until c;aclose(poolfile);
getstringsstarted:=true;end else begin wakeupterminal;
writeln(stdout,'! I can''t read tex.pool.');getstringsstarted:=false;
goto 10;end{:51};10:end;endif('INITEX'){:47}{66:}
procedure printtwo(n:integer);begin n:=abs(n)mod 100;
printchar(48+(n div 10));printchar(48+(n mod 10));end;{:66}{67:}
procedure printhex(n:integer);var k:0..22;begin k:=0;printchar(34);
repeat dig[k]:=n mod 16;n:=n div 16;incr(k);until n=0;printthedigs(k);
end;{:67}{69:}procedure printromanint(n:integer);label 10;
var j,k:poolpointer;u,v:nonnegativeinteger;begin j:=strstart[260];
v:=1000;while true do begin while n>=v do begin printchar(strpool[j]);
n:=n-v;end;if n<=0 then goto 10;k:=j+2;u:=v div(strpool[k-1]-48);
if strpool[k-1]=50 then begin k:=k+2;u:=u div(strpool[k-1]-48);end;
if n+u>=v then begin printchar(strpool[k]);n:=n+u;end else begin j:=j+2;
v:=v div(strpool[j-1]-48);end;end;10:end;{:69}{70:}
procedure printcurrentstring;var j:poolpointer;
begin j:=strstart[strptr];
while j<poolptr do begin printchar(strpool[j]);incr(j);end;end;{:70}
{71:}procedure terminput;var k:0..bufsize;begin termflush(stdout);
if not inputln(stdin,true)then fatalerror(261);termoffset:=0;
decr(selector);
if last<>first then for k:=first to last-1 do print(buffer[k]);println;
incr(selector);end;{:71}{91:}procedure interror(n:integer);
begin print(284);printint(n);printchar(41);error;end;{:91}{92:}
procedure normalizeselector;
begin if logopened then selector:=19 else selector:=17;
if jobname=0 then openlogfile;if interaction=0 then decr(selector);end;
{:92}{98:}procedure pauseforinstructions;
begin if OKtointerrupt then begin interaction:=3;
if(selector=18)or(selector=16)then incr(selector);
begin if interaction=3 then wakeupterminal;printnl(262);print(294);end;
begin helpptr:=3;helpline[2]:=295;helpline[1]:=296;helpline[0]:=297;end;
deletionsallowed:=false;error;deletionsallowed:=true;interrupt:=0;end;
end;{:98}{100:}function half(x:integer):integer;
begin if odd(x)then half:=(x+1)div 2 else half:=x div 2;end;{:100}{102:}
function rounddecimals(k:smallnumber):scaled;var a:integer;begin a:=0;
while k>0 do begin decr(k);a:=(a+dig[k]*131072)div 10;end;
rounddecimals:=(a+1)div 2;end;{:102}{103:}
procedure printscaled(s:scaled);var delta:scaled;
begin if s<0 then begin printchar(45);s:=-s;end;printint(s div 65536);
printchar(46);s:=10*(s mod 65536)+5;delta:=10;
repeat if delta>65536 then s:=s-17232;printchar(48+(s div 65536));
s:=10*(s mod 65536);delta:=delta*10;until s<=delta;end;{:103}{105:}
function multandadd(n:integer;x,y,maxanswer:scaled):scaled;
begin if n<0 then begin x:=-x;n:=-n;end;
if n=0 then multandadd:=y else if((x<=(maxanswer-y)div n)and(-x<=(
maxanswer+y)div n))then multandadd:=n*x+y else begin aritherror:=true;
multandadd:=0;end;end;{:105}{106:}function xovern(x:scaled;
n:integer):scaled;var negative:boolean;begin negative:=false;
if n=0 then begin aritherror:=true;xovern:=0;remainder:=x;
end else begin if n<0 then begin x:=-x;n:=-n;negative:=true;end;
if x>=0 then begin xovern:=x div n;remainder:=x mod n;
end else begin xovern:=-((-x)div n);remainder:=-((-x)mod n);end;end;
if negative then remainder:=-remainder;end;{:106}{107:}
function xnoverd(x:scaled;n,d:integer):scaled;var positive:boolean;
t,u,v:nonnegativeinteger;
begin if x>=0 then positive:=true else begin x:=-x;positive:=false;end;
t:=(x mod 32768)*n;u:=(x div 32768)*n+(t div 32768);
v:=(u mod d)*32768+(t mod 32768);
if u div d>=32768 then aritherror:=true else u:=32768*(u div d)+(v div d
);if positive then begin xnoverd:=u;remainder:=v mod d;
end else begin xnoverd:=-u;remainder:=-(v mod d);end;end;{:107}{108:}
function badness(t,s:scaled):halfword;var r:integer;
begin if t=0 then badness:=0 else if s<=0 then badness:=10000 else begin
if t<=7230584 then r:=(t*297)div s else if s>=1663497 then r:=t div(s
div 297)else r:=t;
if r>1290 then badness:=10000 else badness:=(r*r*r+131072)div 262144;
end;end;{:108}{114:}ifdef('DEBUG')procedure printword(w:memoryword);
begin printint(w.int);printchar(32);printscaled(w.int);printchar(32);
printscaled(round(65536*w.gr));println;printint(w.hh.lh);printchar(61);
printint(w.hh.b0);printchar(58);printint(w.hh.b1);printchar(59);
printint(w.hh.rh);printchar(32);printint(w.qqqq.b0);printchar(58);
printint(w.qqqq.b1);printchar(58);printint(w.qqqq.b2);printchar(58);
printint(w.qqqq.b3);end;endif('DEBUG'){:114}{119:}{292:}
procedure showtokenlist(p,q:integer;l:integer);label 10;var m,c:integer;
matchchr:ASCIIcode;n:ASCIIcode;begin matchchr:=35;n:=48;tally:=0;
while(p<>0)and(tally<l)do begin if p=q then{320:}
begin firstcount:=tally;trickcount:=tally+1+errorline-halferrorline;
if trickcount<errorline then trickcount:=errorline;end{:320};{293:}
if(p<himemmin)or(p>memend)then begin printesc(307);goto 10;end;
if mem[p].hh.lh>=4095 then printcs(mem[p].hh.lh-4095)else begin m:=mem[p
].hh.lh div 256;c:=mem[p].hh.lh mod 256;
if mem[p].hh.lh<0 then printesc(551)else{294:}
case m of 1,2,3,4,7,8,10,11,12:print(c);6:begin print(c);print(c);end;
5:begin print(matchchr);
if c<=9 then printchar(c+48)else begin printchar(33);goto 10;end;end;
13:begin matchchr:=c;print(c);incr(n);printchar(n);if n>57 then goto 10;
end;14:print(552);others:printesc(551)end{:294};end{:293};
p:=mem[p].hh.rh;end;if p<>0 then printesc(550);10:end;{:292}{306:}
procedure runaway;var p:halfword;
begin if scannerstatus>1 then begin printnl(565);
case scannerstatus of 2:begin print(566);p:=defref;end;
3:begin print(567);p:=memtop-3;end;4:begin print(568);p:=memtop-4;end;
5:begin print(569);p:=defref;end;end;printchar(63);println;
showtokenlist(mem[p].hh.rh,0,errorline-10);end;end;{:306}{:119}{120:}
function getavail:halfword;var p:halfword;begin p:=avail;
if p<>0 then avail:=mem[avail].hh.rh else if memend<memmax then begin
incr(memend);p:=memend;end else begin decr(himemmin);p:=himemmin;
if himemmin<=lomemmax then begin runaway;overflow(298,memmax+1-memmin);
end;end;mem[p].hh.rh:=0;ifdef('STAT')incr(dynused);
endif('STAT')getavail:=p;end;{:120}{123:}
procedure flushlist(p:halfword);var q,r:halfword;
begin if p<>0 then begin r:=p;repeat q:=r;r:=mem[r].hh.rh;
ifdef('STAT')decr(dynused);endif('STAT')until r=0;mem[q].hh.rh:=avail;
avail:=p;end;end;{:123}{125:}function getnode(s:integer):halfword;
label 40,10,20;var p:halfword;q:halfword;r:integer;t:integer;
begin 20:p:=rover;repeat{127:}q:=p+mem[p].hh.lh;
while(mem[q].hh.rh=262143)do begin t:=mem[q+1].hh.rh;
if q=rover then rover:=t;mem[t+1].hh.lh:=mem[q+1].hh.lh;
mem[mem[q+1].hh.lh+1].hh.rh:=t;q:=q+mem[q].hh.lh;end;r:=q-s;
if r>toint(p+1)then{128:}begin mem[p].hh.lh:=r-p;rover:=p;goto 40;
end{:128};if r=p then if mem[p+1].hh.rh<>p then{129:}
begin rover:=mem[p+1].hh.rh;t:=mem[p+1].hh.lh;mem[rover+1].hh.lh:=t;
mem[t+1].hh.rh:=rover;goto 40;end{:129};mem[p].hh.lh:=q-p{:127};
p:=mem[p+1].hh.rh;until p=rover;
if s=1073741824 then begin getnode:=262143;goto 10;end;
if lomemmax+2<himemmin then if lomemmax+2<=262143 then{126:}
begin if himemmin-lomemmax>=1998 then t:=lomemmax+1000 else t:=lomemmax
+1+(himemmin-lomemmax)div 2;p:=mem[rover+1].hh.lh;q:=lomemmax;
mem[p+1].hh.rh:=q;mem[rover+1].hh.lh:=q;if t>262143 then t:=262143;
mem[q+1].hh.rh:=rover;mem[q+1].hh.lh:=p;mem[q].hh.rh:=262143;
mem[q].hh.lh:=t-lomemmax;lomemmax:=t;mem[lomemmax].hh.rh:=0;
mem[lomemmax].hh.lh:=0;rover:=q;goto 20;end{:126};
overflow(298,memmax+1-memmin);40:mem[r].hh.rh:=0;
ifdef('STAT')varused:=varused+s;endif('STAT')getnode:=r;10:end;{:125}
{130:}procedure freenode(p:halfword;s:halfword);var q:halfword;
begin mem[p].hh.lh:=s;mem[p].hh.rh:=262143;q:=mem[rover+1].hh.lh;
mem[p+1].hh.lh:=q;mem[p+1].hh.rh:=rover;mem[rover+1].hh.lh:=p;
mem[q+1].hh.rh:=p;ifdef('STAT')varused:=varused-s;endif('STAT')end;
{:130}{131:}ifdef('INITEX')procedure sortavail;var p,q,r:halfword;
oldrover:halfword;begin p:=getnode(1073741824);p:=mem[rover+1].hh.rh;
mem[rover+1].hh.rh:=262143;oldrover:=rover;while p<>oldrover do{132:}
if p<rover then begin q:=p;p:=mem[q+1].hh.rh;mem[q+1].hh.rh:=rover;
rover:=q;end else begin q:=rover;
while mem[q+1].hh.rh<p do q:=mem[q+1].hh.rh;r:=mem[p+1].hh.rh;
mem[p+1].hh.rh:=mem[q+1].hh.rh;mem[q+1].hh.rh:=p;p:=r;end{:132};
p:=rover;
while mem[p+1].hh.rh<>262143 do begin mem[mem[p+1].hh.rh+1].hh.lh:=p;
p:=mem[p+1].hh.rh;end;mem[p+1].hh.rh:=rover;mem[rover+1].hh.lh:=p;end;
endif('INITEX'){:131}{136:}function newnullbox:halfword;var p:halfword;
begin p:=getnode(7);mem[p].hh.b0:=0;mem[p].hh.b1:=0;mem[p+1].int:=0;
mem[p+2].int:=0;mem[p+3].int:=0;mem[p+4].int:=0;mem[p+5].hh.rh:=0;
mem[p+5].hh.b0:=0;mem[p+5].hh.b1:=0;mem[p+6].gr:=0.0;newnullbox:=p;end;
{:136}{139:}function newrule:halfword;var p:halfword;
begin p:=getnode(4);mem[p].hh.b0:=2;mem[p].hh.b1:=0;
mem[p+1].int:=-1073741824;mem[p+2].int:=-1073741824;
mem[p+3].int:=-1073741824;newrule:=p;end;{:139}{144:}
function newligature(f,c:quarterword;q:halfword):halfword;
var p:halfword;begin p:=getnode(2);mem[p].hh.b0:=6;mem[p+1].hh.b0:=f;
mem[p+1].hh.b1:=c;mem[p+1].hh.rh:=q;mem[p].hh.b1:=0;newligature:=p;end;
function newligitem(c:quarterword):halfword;var p:halfword;
begin p:=getnode(2);mem[p].hh.b1:=c;mem[p+1].hh.rh:=0;newligitem:=p;end;
{:144}{145:}function newdisc:halfword;var p:halfword;
begin p:=getnode(2);mem[p].hh.b0:=7;mem[p].hh.b1:=0;mem[p+1].hh.lh:=0;
mem[p+1].hh.rh:=0;newdisc:=p;end;{:145}{147:}function newmath(w:scaled;
s:smallnumber):halfword;var p:halfword;begin p:=getnode(2);
mem[p].hh.b0:=9;mem[p].hh.b1:=s;mem[p+1].int:=w;newmath:=p;end;{:147}
{151:}function newspec(p:halfword):halfword;var q:halfword;
begin q:=getnode(4);mem[q]:=mem[p];mem[q].hh.rh:=0;
mem[q+1].int:=mem[p+1].int;mem[q+2].int:=mem[p+2].int;
mem[q+3].int:=mem[p+3].int;newspec:=q;end;{:151}{152:}
function newparamglue(n:smallnumber):halfword;var p:halfword;q:halfword;
begin p:=getnode(2);mem[p].hh.b0:=10;mem[p].hh.b1:=n+1;
mem[p+1].hh.rh:=0;q:={224:}eqtb[10282+n].hh.rh{:224};mem[p+1].hh.lh:=q;
incr(mem[q].hh.rh);newparamglue:=p;end;{:152}{153:}
function newglue(q:halfword):halfword;var p:halfword;
begin p:=getnode(2);mem[p].hh.b0:=10;mem[p].hh.b1:=0;mem[p+1].hh.rh:=0;
mem[p+1].hh.lh:=q;incr(mem[q].hh.rh);newglue:=p;end;{:153}{154:}
function newskipparam(n:smallnumber):halfword;var p:halfword;
begin tempptr:=newspec({224:}eqtb[10282+n].hh.rh{:224});
p:=newglue(tempptr);mem[tempptr].hh.rh:=0;mem[p].hh.b1:=n+1;
newskipparam:=p;end;{:154}{156:}function newkern(w:scaled):halfword;
var p:halfword;begin p:=getnode(2);mem[p].hh.b0:=11;mem[p].hh.b1:=0;
mem[p+1].int:=w;newkern:=p;end;{:156}{158:}
function newpenalty(m:integer):halfword;var p:halfword;
begin p:=getnode(2);mem[p].hh.b0:=12;mem[p].hh.b1:=0;mem[p+1].int:=m;
newpenalty:=p;end;{:158}{167:}
ifdef('DEBUG')procedure checkmem(printlocs:boolean);label 31,32;
var p,q:halfword;clobbered:boolean;
begin for p:=memmin to lomemmax do freearr[p]:=false;
for p:=himemmin to memend do freearr[p]:=false;{168:}p:=avail;q:=0;
clobbered:=false;
while p<>0 do begin if(p>memend)or(p<himemmin)then clobbered:=true else
if freearr[p]then clobbered:=true;if clobbered then begin printnl(299);
printint(q);goto 31;end;freearr[p]:=true;q:=p;p:=mem[q].hh.rh;end;
31:{:168};{169:}p:=rover;q:=0;clobbered:=false;
repeat if(p>=lomemmax)or(p<memmin)then clobbered:=true else if(mem[p+1].
hh.rh>=lomemmax)or(mem[p+1].hh.rh<memmin)then clobbered:=true else if
not((mem[p].hh.rh=262143))or(mem[p].hh.lh<2)or(p+mem[p].hh.lh>lomemmax)
or(mem[mem[p+1].hh.rh+1].hh.lh<>p)then clobbered:=true;
if clobbered then begin printnl(300);printint(q);goto 32;end;
for q:=p to p+mem[p].hh.lh-1 do begin if freearr[q]then begin printnl(
301);printint(q);goto 32;end;freearr[q]:=true;end;q:=p;
p:=mem[p+1].hh.rh;until p=rover;32:{:169};{170:}p:=memmin;
while p<=lomemmax do begin if(mem[p].hh.rh=262143)then begin printnl(302
);printint(p);end;while(p<=lomemmax)and not freearr[p]do incr(p);
while(p<=lomemmax)and freearr[p]do incr(p);end{:170};
if printlocs then{171:}begin printnl(303);
for p:=memmin to lomemmax do if not freearr[p]and((p>waslomax)or wasfree
[p])then begin printchar(32);printint(p);end;
for p:=himemmin to memend do if not freearr[p]and((p<washimin)or(p>
wasmemend)or wasfree[p])then begin printchar(32);printint(p);end;
end{:171};for p:=memmin to lomemmax do wasfree[p]:=freearr[p];
for p:=himemmin to memend do wasfree[p]:=freearr[p];wasmemend:=memend;
waslomax:=lomemmax;washimin:=himemmin;end;endif('DEBUG'){:167}{172:}
ifdef('DEBUG')procedure searchmem(p:halfword);var q:integer;
begin for q:=memmin to lomemmax do begin if mem[q].hh.rh=p then begin
printnl(304);printint(q);printchar(41);end;
if mem[q].hh.lh=p then begin printnl(305);printint(q);printchar(41);end;
end;
for q:=himemmin to memend do begin if mem[q].hh.rh=p then begin printnl(
304);printint(q);printchar(41);end;
if mem[q].hh.lh=p then begin printnl(305);printint(q);printchar(41);end;
end;{255:}
for q:=1 to 11333 do begin if eqtb[q].hh.rh=p then begin printnl(497);
printint(q);printchar(41);end;end{:255};{285:}
if saveptr>0 then for q:=0 to saveptr-1 do begin if savestack[q].hh.rh=p
then begin printnl(542);printint(q);printchar(41);end;end{:285};{933:}
for q:=0 to 607 do begin if hyphlist[q]=p then begin printnl(933);
printint(q);printchar(41);end;end{:933};end;endif('DEBUG'){:172}{174:}
procedure shortdisplay(p:integer);var n:integer;
begin while p>memmin do begin if(p>=himemmin)then begin if p<=memend
then begin if mem[p].hh.b0<>fontinshortdisplay then begin if(mem[p].hh.
b0>fontmax)then printchar(42)else{267:}
printesc(hash[10024+mem[p].hh.b0].rh){:267};printchar(32);
fontinshortdisplay:=mem[p].hh.b0;end;print(mem[p].hh.b1);end;
end else{175:}case mem[p].hh.b0 of 0,1,3,8,4,5,13:print(306);
2:printchar(124);10:if mem[p+1].hh.lh<>0 then printchar(32);
9:printchar(36);6:shortdisplay(mem[p+1].hh.rh);
7:begin shortdisplay(mem[p+1].hh.lh);shortdisplay(mem[p+1].hh.rh);
n:=mem[p].hh.b1;
while n>0 do begin if mem[p].hh.rh<>0 then p:=mem[p].hh.rh;decr(n);end;
end;others:end{:175};p:=mem[p].hh.rh;end;end;{:174}{176:}
procedure printfontandchar(p:integer);
begin if p>memend then printesc(307)else begin if(mem[p].hh.b0>fontmax)
then printchar(42)else{267:}printesc(hash[10024+mem[p].hh.b0].rh){:267};
printchar(32);print(mem[p].hh.b1);end;end;
procedure printmark(p:integer);begin printchar(123);
if(p<himemmin)or(p>memend)then printesc(307)else showtokenlist(mem[p].hh
.rh,0,maxprintline-10);printchar(125);end;
procedure printruledimen(d:scaled);
begin if(d=-1073741824)then printchar(42)else printscaled(d);end;{:176}
{177:}procedure printglue(d:scaled;order:integer;s:strnumber);
begin printscaled(d);
if(order<0)or(order>3)then print(308)else if order>0 then begin print(
309);while order>1 do begin printchar(108);decr(order);end;
end else if s<>0 then print(s);end;{:177}{178:}
procedure printspec(p:integer;s:strnumber);
begin if(p<memmin)or(p>=lomemmax)then printchar(42)else begin
printscaled(mem[p+1].int);if s<>0 then print(s);
if mem[p+2].int<>0 then begin print(310);
printglue(mem[p+2].int,mem[p].hh.b0,s);end;
if mem[p+3].int<>0 then begin print(311);
printglue(mem[p+3].int,mem[p].hh.b1,s);end;end;end;{:178}{179:}{691:}
procedure printfamandchar(p:halfword);begin printesc(460);
printint(mem[p].hh.b0);printchar(32);print(mem[p].hh.b1);end;
procedure printdelimiter(p:halfword);var a:integer;
begin a:=mem[p].qqqq.b0*256+mem[p].qqqq.b1;
a:=a*4096+mem[p].qqqq.b2*256+mem[p].qqqq.b3;
if a<0 then printint(a)else printhex(a);end;{:691}{692:}
procedure showinfo;forward;procedure printsubsidiarydata(p:halfword;
c:ASCIIcode);
begin if(poolptr-strstart[strptr])>=depththreshold then begin if mem[p].
hh.rh<>0 then print(312);end else begin begin strpool[poolptr]:=c;
incr(poolptr);end;tempptr:=p;case mem[p].hh.rh of 1:begin println;
printcurrentstring;printfamandchar(p);end;2:showinfo;
3:if mem[p].hh.lh=0 then begin println;printcurrentstring;print(853);
end else showinfo;others:end;decr(poolptr);end;end;{:692}{694:}
procedure printstyle(c:integer);begin case c div 2 of 0:printesc(854);
1:printesc(855);2:printesc(856);3:printesc(857);others:print(858)end;
end;{:694}{225:}procedure printskipparam(n:integer);
begin case n of 0:printesc(372);1:printesc(373);2:printesc(374);
3:printesc(375);4:printesc(376);5:printesc(377);6:printesc(378);
7:printesc(379);8:printesc(380);9:printesc(381);10:printesc(382);
11:printesc(383);12:printesc(384);13:printesc(385);14:printesc(386);
15:printesc(387);16:printesc(388);17:printesc(389);others:print(390)end;
end;{:225}{:179}{182:}procedure shownodelist(p:integer);label 10;
var n:integer;g:real;
begin if(poolptr-strstart[strptr])>depththreshold then begin if p>0 then
print(312);goto 10;end;n:=0;while p>memmin do begin println;
printcurrentstring;if p>memend then begin print(313);goto 10;end;
incr(n);if n>breadthmax then begin print(314);goto 10;end;{183:}
if(p>=himemmin)then printfontandchar(p)else case mem[p].hh.b0 of 0,1,13:
{184:}
begin if mem[p].hh.b0=0 then printesc(104)else if mem[p].hh.b0=1 then
printesc(118)else printesc(316);print(317);printscaled(mem[p+3].int);
printchar(43);printscaled(mem[p+2].int);print(318);
printscaled(mem[p+1].int);if mem[p].hh.b0=13 then{185:}
begin if mem[p].hh.b1<>0 then begin print(284);printint(mem[p].hh.b1+1);
print(320);end;if mem[p+6].int<>0 then begin print(321);
printglue(mem[p+6].int,mem[p+5].hh.b1,0);end;
if mem[p+4].int<>0 then begin print(322);
printglue(mem[p+4].int,mem[p+5].hh.b0,0);end;end{:185}else begin{186:}
g:=mem[p+6].gr;if(g<>0.0)and(mem[p+5].hh.b0<>0)then begin print(323);
if mem[p+5].hh.b0=2 then print(324);
if fabs(g)>20000.0 then begin if g>0.0 then printchar(62)else print(325)
;printglue(20000*65536,mem[p+5].hh.b1,0);
end else printglue(round(65536*g),mem[p+5].hh.b1,0);end{:186};
if mem[p+4].int<>0 then begin print(319);printscaled(mem[p+4].int);end;
end;begin begin strpool[poolptr]:=46;incr(poolptr);end;
shownodelist(mem[p+5].hh.rh);decr(poolptr);end;end{:184};2:{187:}
begin printesc(326);printruledimen(mem[p+3].int);printchar(43);
printruledimen(mem[p+2].int);print(318);printruledimen(mem[p+1].int);
end{:187};3:{188:}begin printesc(327);printint(mem[p].hh.b1);print(328);
printscaled(mem[p+3].int);print(329);printspec(mem[p+4].hh.rh,0);
printchar(44);printscaled(mem[p+2].int);print(330);
printint(mem[p+1].int);begin begin strpool[poolptr]:=46;incr(poolptr);
end;shownodelist(mem[p+4].hh.lh);decr(poolptr);end;end{:188};8:{1356:}
case mem[p].hh.b1 of 0:begin printwritewhatsit(1276,p);printchar(61);
printfilename(mem[p+1].hh.rh,mem[p+2].hh.lh,mem[p+2].hh.rh);end;
1:begin printwritewhatsit(590,p);printmark(mem[p+1].hh.rh);end;
2:printwritewhatsit(1277,p);3:begin printesc(1278);
printmark(mem[p+1].hh.rh);end;4:begin printesc(1280);
printint(mem[p+1].hh.rh);print(362);printint(mem[p+1].hh.b0);
printchar(44);printint(mem[p+1].hh.b1);printchar(41);end;
others:print(1283)end{:1356};10:{189:}if mem[p].hh.b1>=100 then{190:}
begin printesc(335);
if mem[p].hh.b1=101 then printchar(99)else if mem[p].hh.b1=102 then
printchar(120);print(336);printspec(mem[p+1].hh.lh,0);
begin begin strpool[poolptr]:=46;incr(poolptr);end;
shownodelist(mem[p+1].hh.rh);decr(poolptr);end;end{:190}
else begin printesc(331);if mem[p].hh.b1<>0 then begin printchar(40);
if mem[p].hh.b1<98 then printskipparam(mem[p].hh.b1-1)else if mem[p].hh.
b1=98 then printesc(332)else printesc(333);printchar(41);end;
if mem[p].hh.b1<>98 then begin printchar(32);
if mem[p].hh.b1<98 then printspec(mem[p+1].hh.lh,0)else printspec(mem[p
+1].hh.lh,334);end;end{:189};11:{191:}
if mem[p].hh.b1<>99 then begin printesc(337);
if mem[p].hh.b1<>0 then printchar(32);printscaled(mem[p+1].int);
if mem[p].hh.b1=2 then print(338);end else begin printesc(339);
printscaled(mem[p+1].int);print(334);end{:191};9:{192:}
begin printesc(340);if mem[p].hh.b1=0 then print(341)else print(342);
if mem[p+1].int<>0 then begin print(343);printscaled(mem[p+1].int);end;
end{:192};6:{193:}begin printfontandchar(p+1);print(344);
if mem[p].hh.b1>1 then printchar(124);
fontinshortdisplay:=mem[p+1].hh.b0;shortdisplay(mem[p+1].hh.rh);
if odd(mem[p].hh.b1)then printchar(124);printchar(41);end{:193};
12:{194:}begin printesc(345);printint(mem[p+1].int);end{:194};7:{195:}
begin printesc(346);if mem[p].hh.b1>0 then begin print(347);
printint(mem[p].hh.b1);end;begin begin strpool[poolptr]:=46;
incr(poolptr);end;shownodelist(mem[p+1].hh.lh);decr(poolptr);end;
begin strpool[poolptr]:=124;incr(poolptr);end;
shownodelist(mem[p+1].hh.rh);decr(poolptr);end{:195};4:{196:}
begin printesc(348);printmark(mem[p+1].int);end{:196};5:{197:}
begin printesc(349);begin begin strpool[poolptr]:=46;incr(poolptr);end;
shownodelist(mem[p+1].int);decr(poolptr);end;end{:197};{690:}
14:printstyle(mem[p].hh.b1);15:{695:}begin printesc(521);
begin strpool[poolptr]:=68;incr(poolptr);end;
shownodelist(mem[p+1].hh.lh);decr(poolptr);begin strpool[poolptr]:=84;
incr(poolptr);end;shownodelist(mem[p+1].hh.rh);decr(poolptr);
begin strpool[poolptr]:=83;incr(poolptr);end;
shownodelist(mem[p+2].hh.lh);decr(poolptr);begin strpool[poolptr]:=115;
incr(poolptr);end;shownodelist(mem[p+2].hh.rh);decr(poolptr);end{:695};
16,17,18,19,20,21,22,23,24,27,26,29,28,30,31:{696:}
begin case mem[p].hh.b0 of 16:printesc(859);17:printesc(860);
18:printesc(861);19:printesc(862);20:printesc(863);21:printesc(864);
22:printesc(865);23:printesc(866);27:printesc(867);26:printesc(868);
29:printesc(535);24:begin printesc(529);printdelimiter(p+4);end;
28:begin printesc(504);printfamandchar(p+4);end;30:begin printesc(869);
printdelimiter(p+1);end;31:begin printesc(870);printdelimiter(p+1);end;
end;if mem[p].hh.b1<>0 then if mem[p].hh.b1=1 then printesc(871)else
printesc(872);if mem[p].hh.b0<30 then printsubsidiarydata(p+1,46);
printsubsidiarydata(p+2,94);printsubsidiarydata(p+3,95);end{:696};
25:{697:}begin printesc(873);
if mem[p+1].int=1073741824 then print(874)else printscaled(mem[p+1].int)
;
if(mem[p+4].qqqq.b0<>0)or(mem[p+4].qqqq.b1<>0)or(mem[p+4].qqqq.b2<>0)or(
mem[p+4].qqqq.b3<>0)then begin print(875);printdelimiter(p+4);end;
if(mem[p+5].qqqq.b0<>0)or(mem[p+5].qqqq.b1<>0)or(mem[p+5].qqqq.b2<>0)or(
mem[p+5].qqqq.b3<>0)then begin print(876);printdelimiter(p+5);end;
printsubsidiarydata(p+2,92);printsubsidiarydata(p+3,47);end{:697};{:690}
others:print(315)end{:183};p:=mem[p].hh.rh;end;10:end;{:182}{198:}
procedure showbox(p:halfword);begin{236:}
depththreshold:=eqtb[12688].int;breadthmax:=eqtb[12687].int{:236};
if breadthmax<=0 then breadthmax:=5;
if poolptr+depththreshold>=poolsize then depththreshold:=poolsize-
poolptr-1;shownodelist(p);println;end;{:198}{200:}
procedure deletetokenref(p:halfword);
begin if mem[p].hh.lh=0 then flushlist(p)else decr(mem[p].hh.lh);end;
{:200}{201:}procedure deleteglueref(p:halfword);
begin if mem[p].hh.rh=0 then freenode(p,4)else decr(mem[p].hh.rh);end;
{:201}{202:}procedure flushnodelist(p:halfword);label 30;var q:halfword;
begin while p<>0 do begin q:=mem[p].hh.rh;
if(p>=himemmin)then begin mem[p].hh.rh:=avail;avail:=p;
ifdef('STAT')decr(dynused);
endif('STAT')end else begin case mem[p].hh.b0 of 0,1,13:begin
flushnodelist(mem[p+5].hh.rh);freenode(p,7);goto 30;end;
2:begin freenode(p,4);goto 30;end;3:begin flushnodelist(mem[p+4].hh.lh);
deleteglueref(mem[p+4].hh.rh);freenode(p,5);goto 30;end;8:{1358:}
begin case mem[p].hh.b1 of 0:freenode(p,3);
1,3:begin deletetokenref(mem[p+1].hh.rh);freenode(p,2);goto 30;end;
2,4:freenode(p,2);others:confusion(1285)end;goto 30;end{:1358};
10:begin begin if mem[mem[p+1].hh.lh].hh.rh=0 then freenode(mem[p+1].hh.
lh,4)else decr(mem[mem[p+1].hh.lh].hh.rh);end;
if mem[p+1].hh.rh<>0 then flushnodelist(mem[p+1].hh.rh);end;11,9,12:;
6:flushnodelist(mem[p+1].hh.rh);4:deletetokenref(mem[p+1].int);
7:begin flushnodelist(mem[p+1].hh.lh);flushnodelist(mem[p+1].hh.rh);end;
5:flushnodelist(mem[p+1].int);{698:}14:begin freenode(p,3);goto 30;end;
15:begin flushnodelist(mem[p+1].hh.lh);flushnodelist(mem[p+1].hh.rh);
flushnodelist(mem[p+2].hh.lh);flushnodelist(mem[p+2].hh.rh);
freenode(p,3);goto 30;end;
16,17,18,19,20,21,22,23,24,27,26,29,28:begin if mem[p+1].hh.rh>=2 then
flushnodelist(mem[p+1].hh.lh);
if mem[p+2].hh.rh>=2 then flushnodelist(mem[p+2].hh.lh);
if mem[p+3].hh.rh>=2 then flushnodelist(mem[p+3].hh.lh);
if mem[p].hh.b0=24 then freenode(p,5)else if mem[p].hh.b0=28 then
freenode(p,5)else freenode(p,4);goto 30;end;30,31:begin freenode(p,4);
goto 30;end;25:begin flushnodelist(mem[p+2].hh.lh);
flushnodelist(mem[p+3].hh.lh);freenode(p,6);goto 30;end;{:698}
others:confusion(350)end;freenode(p,2);30:end;p:=q;end;end;{:202}{204:}
function copynodelist(p:halfword):halfword;var h:halfword;q:halfword;
r:halfword;words:0..5;begin h:=getavail;q:=h;while p<>0 do begin{205:}
words:=1;if(p>=himemmin)then r:=getavail else{206:}
case mem[p].hh.b0 of 0,1,13:begin r:=getnode(7);mem[r+6]:=mem[p+6];
mem[r+5]:=mem[p+5];mem[r+5].hh.rh:=copynodelist(mem[p+5].hh.rh);
words:=5;end;2:begin r:=getnode(4);words:=4;end;3:begin r:=getnode(5);
mem[r+4]:=mem[p+4];incr(mem[mem[p+4].hh.rh].hh.rh);
mem[r+4].hh.lh:=copynodelist(mem[p+4].hh.lh);words:=4;end;8:{1357:}
case mem[p].hh.b1 of 0:begin r:=getnode(3);words:=3;end;
1,3:begin r:=getnode(2);incr(mem[mem[p+1].hh.rh].hh.lh);words:=2;end;
2,4:begin r:=getnode(2);words:=2;end;others:confusion(1284)end{:1357};
10:begin r:=getnode(2);incr(mem[mem[p+1].hh.lh].hh.rh);
mem[r+1].hh.lh:=mem[p+1].hh.lh;
mem[r+1].hh.rh:=copynodelist(mem[p+1].hh.rh);end;
11,9,12:begin r:=getnode(2);words:=2;end;6:begin r:=getnode(2);
mem[r+1]:=mem[p+1];mem[r+1].hh.rh:=copynodelist(mem[p+1].hh.rh);end;
7:begin r:=getnode(2);mem[r+1].hh.lh:=copynodelist(mem[p+1].hh.lh);
mem[r+1].hh.rh:=copynodelist(mem[p+1].hh.rh);end;4:begin r:=getnode(2);
incr(mem[mem[p+1].int].hh.lh);words:=2;end;5:begin r:=getnode(2);
mem[r+1].int:=copynodelist(mem[p+1].int);end;
others:confusion(351)end{:206};while words>0 do begin decr(words);
mem[r+words]:=mem[p+words];end{:205};mem[q].hh.rh:=r;q:=r;
p:=mem[p].hh.rh;end;mem[q].hh.rh:=0;q:=mem[h].hh.rh;
begin mem[h].hh.rh:=avail;avail:=h;ifdef('STAT')decr(dynused);
endif('STAT')end;copynodelist:=q;end;{:204}{211:}
procedure printmode(m:integer);
begin if m>0 then case m div(101)of 0:print(352);1:print(353);
2:print(354);
end else if m=0 then print(355)else case(-m)div(101)of 0:print(356);
1:print(357);2:print(340);end;print(358);end;{:211}{216:}
procedure pushnest;
begin if nestptr>maxneststack then begin maxneststack:=nestptr;
if nestptr=nestsize then overflow(359,nestsize);end;
nest[nestptr]:=curlist;incr(nestptr);curlist.headfield:=getavail;
curlist.tailfield:=curlist.headfield;curlist.pgfield:=0;
curlist.mlfield:=line;end;{:216}{217:}procedure popnest;
begin begin mem[curlist.headfield].hh.rh:=avail;
avail:=curlist.headfield;ifdef('STAT')decr(dynused);endif('STAT')end;
decr(nestptr);curlist:=nest[nestptr];end;{:217}{218:}
procedure printtotals;forward;procedure showactivities;
var p:0..nestsize;m:-203..203;a:memoryword;q,r:halfword;t:integer;
begin nest[nestptr]:=curlist;printnl(335);println;
for p:=nestptr downto 0 do begin m:=nest[p].modefield;
a:=nest[p].auxfield;printnl(360);printmode(m);print(361);
printint(abs(nest[p].mlfield));
if m=102 then if(nest[p].lhmfield<>2)or(nest[p].rhmfield<>3)then begin
print(362);printint(nest[p].lhmfield);printchar(44);
printint(nest[p].rhmfield);printchar(41);end;
if nest[p].mlfield<0 then print(363);if p=0 then begin{986:}
if memtop-2<>pagetail then begin printnl(973);
if outputactive then print(974);showbox(mem[memtop-2].hh.rh);
if pagecontents>0 then begin printnl(975);printtotals;printnl(976);
printscaled(pagesofar[0]);r:=mem[memtop].hh.rh;
while r<>memtop do begin println;printesc(327);t:=mem[r].hh.b1;
printint(t);print(977);t:=xovern(mem[r+3].int,1000)*eqtb[12718+t].int;
printscaled(t);if mem[r].hh.b0=1 then begin q:=memtop-2;t:=0;
repeat q:=mem[q].hh.rh;
if(mem[q].hh.b0=3)and(mem[q].hh.b1=mem[r].hh.b1)then incr(t);
until q=mem[r+1].hh.lh;print(978);printint(t);print(979);end;
r:=mem[r].hh.rh;end;end;end{:986};
if mem[memtop-1].hh.rh<>0 then printnl(364);end;
showbox(mem[nest[p].headfield].hh.rh);{219:}
case abs(m)div(101)of 0:begin printnl(365);
if a.int<=-65536000 then print(366)else printscaled(a.int);
if nest[p].pgfield<>0 then begin print(367);printint(nest[p].pgfield);
print(368);if nest[p].pgfield<>1 then printchar(115);end;end;
1:begin printnl(369);printint(a.hh.lh);
if m>0 then if a.hh.rh>0 then begin print(370);printint(a.hh.rh);end;
end;2:if a.int<>0 then begin print(371);showbox(a.int);end;end{:219};
end;end;{:218}{237:}procedure printparam(n:integer);
begin case n of 0:printesc(416);1:printesc(417);2:printesc(418);
3:printesc(419);4:printesc(420);5:printesc(421);6:printesc(422);
7:printesc(423);8:printesc(424);9:printesc(425);10:printesc(426);
11:printesc(427);12:printesc(428);13:printesc(429);14:printesc(430);
15:printesc(431);16:printesc(432);17:printesc(433);18:printesc(434);
19:printesc(435);20:printesc(436);21:printesc(437);22:printesc(438);
23:printesc(439);24:printesc(440);25:printesc(441);26:printesc(442);
27:printesc(443);28:printesc(444);29:printesc(445);30:printesc(446);
31:printesc(447);32:printesc(448);33:printesc(449);34:printesc(450);
35:printesc(451);36:printesc(452);37:printesc(453);38:printesc(454);
39:printesc(455);40:printesc(456);41:printesc(457);42:printesc(458);
43:printesc(459);44:printesc(460);45:printesc(461);46:printesc(462);
47:printesc(463);48:printesc(464);49:printesc(465);50:printesc(466);
51:printesc(467);52:printesc(468);53:printesc(469);54:printesc(470);
others:print(471)end;end;{:237}{245:}procedure begindiagnostic;
begin oldsetting:=selector;
if(eqtb[12692].int<=0)and(selector=19)then begin decr(selector);
if history=0 then history:=1;end;end;
procedure enddiagnostic(blankline:boolean);begin printnl(335);
if blankline then println;selector:=oldsetting;end;{:245}{247:}
procedure printlengthparam(n:integer);begin case n of 0:printesc(474);
1:printesc(475);2:printesc(476);3:printesc(477);4:printesc(478);
5:printesc(479);6:printesc(480);7:printesc(481);8:printesc(482);
9:printesc(483);10:printesc(484);11:printesc(485);12:printesc(486);
13:printesc(487);14:printesc(488);15:printesc(489);16:printesc(490);
17:printesc(491);18:printesc(492);19:printesc(493);20:printesc(494);
others:print(495)end;end;{:247}{252:}{298:}
procedure printcmdchr(cmd:quarterword;chrcode:halfword);
begin case cmd of 1:begin print(553);print(chrcode);end;
2:begin print(554);print(chrcode);end;3:begin print(555);print(chrcode);
end;6:begin print(556);print(chrcode);end;7:begin print(557);
print(chrcode);end;8:begin print(558);print(chrcode);end;9:print(559);
10:begin print(560);print(chrcode);end;11:begin print(561);
print(chrcode);end;12:begin print(562);print(chrcode);end;{227:}
75,76:if chrcode<10300 then printskipparam(chrcode-10282)else if chrcode
<10556 then begin printesc(391);printint(chrcode-10300);
end else begin printesc(392);printint(chrcode-10556);end;{:227}{231:}
72:if chrcode>=10822 then begin printesc(403);printint(chrcode-10822);
end else case chrcode of 10813:printesc(394);10814:printesc(395);
10815:printesc(396);10816:printesc(397);10817:printesc(398);
10818:printesc(399);10819:printesc(400);10820:printesc(401);
others:printesc(402)end;{:231}{239:}
73:if chrcode<12718 then printparam(chrcode-12663)else begin printesc(
472);printint(chrcode-12718);end;{:239}{249:}
74:if chrcode<13251 then printlengthparam(chrcode-13230)else begin
printesc(496);printint(chrcode-13251);end;{:249}{266:}45:printesc(504);
90:printesc(505);40:printesc(506);41:printesc(507);77:printesc(515);
61:printesc(508);42:printesc(527);16:printesc(509);107:printesc(500);
88:printesc(514);15:printesc(510);92:printesc(511);67:printesc(501);
62:printesc(512);64:printesc(32);102:printesc(513);32:printesc(516);
36:printesc(517);39:printesc(518);37:printesc(327);44:printesc(47);
18:printesc(348);46:printesc(519);17:printesc(520);54:printesc(521);
91:printesc(522);34:printesc(523);65:printesc(524);103:printesc(525);
55:printesc(332);63:printesc(526);66:printesc(529);96:printesc(530);
0:printesc(531);98:printesc(532);80:printesc(528);84:printesc(404);
109:printesc(533);71:printesc(403);38:printesc(349);33:printesc(534);
56:printesc(535);35:printesc(536);{:266}{335:}13:printesc(593);{:335}
{377:}104:if chrcode=0 then printesc(625)else printesc(626);{:377}{385:}
110:case chrcode of 1:printesc(628);2:printesc(629);3:printesc(630);
4:printesc(631);others:printesc(627)end;{:385}{412:}
89:if chrcode=0 then printesc(472)else if chrcode=1 then printesc(496)
else if chrcode=2 then printesc(391)else printesc(392);{:412}{417:}
79:if chrcode=1 then printesc(665)else printesc(664);
82:if chrcode=0 then printesc(666)else printesc(667);
83:if chrcode=1 then printesc(668)else if chrcode=3 then printesc(669)
else printesc(670);70:case chrcode of 0:printesc(671);1:printesc(672);
2:printesc(673);3:printesc(674);others:printesc(675)end;{:417}{469:}
108:case chrcode of 0:printesc(731);1:printesc(732);2:printesc(733);
3:printesc(734);4:printesc(735);others:printesc(736)end;{:469}{488:}
105:case chrcode of 1:printesc(753);2:printesc(754);3:printesc(755);
4:printesc(756);5:printesc(757);6:printesc(758);7:printesc(759);
8:printesc(760);9:printesc(761);10:printesc(762);11:printesc(763);
12:printesc(764);13:printesc(765);14:printesc(766);15:printesc(767);
16:printesc(768);others:printesc(752)end;{:488}{492:}
106:if chrcode=2 then printesc(769)else if chrcode=4 then printesc(770)
else printesc(771);{:492}{781:}
4:if chrcode=256 then printesc(891)else begin print(895);print(chrcode);
end;5:if chrcode=257 then printesc(892)else printesc(893);{:781}{984:}
81:case chrcode of 0:printesc(963);1:printesc(964);2:printesc(965);
3:printesc(966);4:printesc(967);5:printesc(968);6:printesc(969);
others:printesc(970)end;{:984}{1053:}
14:if chrcode=1 then printesc(1019)else printesc(1018);{:1053}{1059:}
26:case chrcode of 4:printesc(1020);0:printesc(1021);1:printesc(1022);
2:printesc(1023);others:printesc(1024)end;
27:case chrcode of 4:printesc(1025);0:printesc(1026);1:printesc(1027);
2:printesc(1028);others:printesc(1029)end;28:printesc(333);
29:printesc(337);30:printesc(339);{:1059}{1072:}
21:if chrcode=1 then printesc(1047)else printesc(1048);
22:if chrcode=1 then printesc(1049)else printesc(1050);
20:case chrcode of 0:printesc(405);1:printesc(1051);2:printesc(1052);
3:printesc(958);4:printesc(1053);5:printesc(960);
others:printesc(1054)end;
31:if chrcode=100 then printesc(1056)else if chrcode=101 then printesc(
1057)else if chrcode=102 then printesc(1058)else printesc(1055);{:1072}
{1089:}43:if chrcode=0 then printesc(1074)else printesc(1073);{:1089}
{1108:}
25:if chrcode=10 then printesc(1085)else if chrcode=11 then printesc(
1084)else printesc(1083);
23:if chrcode=1 then printesc(1087)else printesc(1086);
24:if chrcode=1 then printesc(1089)else printesc(1088);{:1108}{1115:}
47:if chrcode=1 then printesc(45)else printesc(346);{:1115}{1143:}
48:if chrcode=1 then printesc(1121)else printesc(1120);{:1143}{1157:}
50:case chrcode of 16:printesc(859);17:printesc(860);18:printesc(861);
19:printesc(862);20:printesc(863);21:printesc(864);22:printesc(865);
23:printesc(866);26:printesc(868);others:printesc(867)end;
51:if chrcode=1 then printesc(871)else if chrcode=2 then printesc(872)
else printesc(1122);{:1157}{1170:}53:printstyle(chrcode);{:1170}{1179:}
52:case chrcode of 1:printesc(1141);2:printesc(1142);3:printesc(1143);
4:printesc(1144);5:printesc(1145);others:printesc(1140)end;{:1179}
{1189:}49:if chrcode=30 then printesc(869)else printesc(870);{:1189}
{1209:}
93:if chrcode=1 then printesc(1164)else if chrcode=2 then printesc(1165)
else printesc(1166);
97:if chrcode=0 then printesc(1167)else if chrcode=1 then printesc(1168)
else if chrcode=2 then printesc(1169)else printesc(1170);{:1209}{1220:}
94:if chrcode<>0 then printesc(1185)else printesc(1184);{:1220}{1223:}
95:case chrcode of 0:printesc(1186);1:printesc(1187);2:printesc(1188);
3:printesc(1189);4:printesc(1190);5:printesc(1191);
others:printesc(1192)end;68:begin printesc(509);printhex(chrcode);end;
69:begin printesc(520);printhex(chrcode);end;{:1223}{1231:}
85:if chrcode=11383 then printesc(411)else if chrcode=12407 then
printesc(415)else if chrcode=11639 then printesc(412)else if chrcode=
11895 then printesc(413)else if chrcode=12151 then printesc(414)else
printesc(473);86:printsize(chrcode-11335);{:1231}{1251:}
99:if chrcode=1 then printesc(946)else printesc(934);{:1251}{1255:}
78:if chrcode=0 then printesc(1208)else printesc(1209);{:1255}{1261:}
87:begin print(1217);print(fontname[chrcode]);
if fontsize[chrcode]<>fontdsize[chrcode]then begin print(737);
printscaled(fontsize[chrcode]);print(393);end;end;{:1261}{1263:}
100:case chrcode of 0:printesc(272);1:printesc(273);2:printesc(274);
others:printesc(1218)end;{:1263}{1273:}
60:if chrcode=0 then printesc(1220)else printesc(1219);{:1273}{1278:}
58:if chrcode=0 then printesc(1221)else printesc(1222);{:1278}{1287:}
57:if chrcode=11639 then printesc(1228)else printesc(1229);{:1287}
{1292:}19:case chrcode of 1:printesc(1231);2:printesc(1232);
3:printesc(1233);others:printesc(1230)end;{:1292}{1295:}101:print(1240);
111:print(1241);112:printesc(1242);113:printesc(1243);
114:begin printesc(1164);printesc(1243);end;115:printesc(1244);{:1295}
{1346:}59:case chrcode of 0:printesc(1276);1:printesc(590);
2:printesc(1277);3:printesc(1278);4:printesc(1279);5:printesc(1280);
others:print(1281)end;{:1346}others:print(563)end;end;{:298}
ifdef('STAT')procedure showeqtb(n:halfword);
begin if n<1 then printchar(63)else if n<10282 then{223:}
begin sprintcs(n);printchar(61);
printcmdchr(eqtb[n].hh.b0,eqtb[n].hh.rh);
if eqtb[n].hh.b0>=111 then begin printchar(58);
showtokenlist(mem[eqtb[n].hh.rh].hh.rh,0,32);end;end{:223}
else if n<10812 then{229:}if n<10300 then begin printskipparam(n-10282);
printchar(61);
if n<10297 then printspec(eqtb[n].hh.rh,393)else printspec(eqtb[n].hh.rh
,334);end else if n<10556 then begin printesc(391);printint(n-10300);
printchar(61);printspec(eqtb[n].hh.rh,393);end else begin printesc(392);
printint(n-10556);printchar(61);printspec(eqtb[n].hh.rh,334);end{:229}
else if n<12663 then{233:}if n=10812 then begin printesc(404);
printchar(61);
if eqtb[10812].hh.rh=0 then printchar(48)else printint(mem[eqtb[10812].
hh.rh].hh.lh);end else if n<10822 then begin printcmdchr(72,n);
printchar(61);
if eqtb[n].hh.rh<>0 then showtokenlist(mem[eqtb[n].hh.rh].hh.rh,0,32);
end else if n<11078 then begin printesc(403);printint(n-10822);
printchar(61);
if eqtb[n].hh.rh<>0 then showtokenlist(mem[eqtb[n].hh.rh].hh.rh,0,32);
end else if n<11334 then begin printesc(405);printint(n-11078);
printchar(61);
if eqtb[n].hh.rh=0 then print(406)else begin depththreshold:=0;
breadthmax:=1;shownodelist(eqtb[n].hh.rh);end;
end else if n<11383 then{234:}
begin if n=11334 then print(407)else if n<11351 then begin printesc(408)
;printint(n-11335);end else if n<11367 then begin printesc(409);
printint(n-11351);end else begin printesc(410);printint(n-11367);end;
printchar(61);printesc(hash[10024+eqtb[n].hh.rh].rh);end{:234}else{235:}
if n<12407 then begin if n<11639 then begin printesc(411);
printint(n-11383);end else if n<11895 then begin printesc(412);
printint(n-11639);end else if n<12151 then begin printesc(413);
printint(n-11895);end else begin printesc(414);printint(n-12151);end;
printchar(61);printint(eqtb[n].hh.rh);end else begin printesc(415);
printint(n-12407);printchar(61);printint(eqtb[n].hh.rh);end{:235}{:233}
else if n<13230 then{242:}
begin if n<12718 then printparam(n-12663)else if n<12974 then begin
printesc(472);printint(n-12718);end else begin printesc(473);
printint(n-12974);end;printchar(61);printint(eqtb[n].int);end{:242}
else if n<=13506 then{251:}
begin if n<13251 then printlengthparam(n-13230)else begin printesc(496);
printint(n-13251);end;printchar(61);printscaled(eqtb[n].int);print(393);
end{:251}else printchar(63);end;endif('STAT'){:252}{259:}
function idlookup(j,l:integer):halfword;label 40;var h:integer;
d:integer;p:halfword;k:halfword;begin{261:}h:=buffer[j];
for k:=j+1 to j+l-1 do begin h:=h+h+buffer[k];
while h>=7919 do h:=h-7919;end{:261};p:=h+514;
while true do begin if hash[p].rh>0 then if(strstart[hash[p].rh+1]-
strstart[hash[p].rh])=l then if streqbuf(hash[p].rh,j)then goto 40;
if hash[p].lh=0 then begin if nonewcontrolsequence then p:=10281 else{
260:}
begin if hash[p].rh>0 then begin repeat if(hashused=514)then overflow(
499,9500);decr(hashused);until hash[hashused].rh=0;hash[p].lh:=hashused;
p:=hashused;end;
begin if poolptr+l>poolsize then overflow(257,poolsize-initpoolptr);end;
d:=(poolptr-strstart[strptr]);
while poolptr>strstart[strptr]do begin decr(poolptr);
strpool[poolptr+l]:=strpool[poolptr];end;
for k:=j to j+l-1 do begin strpool[poolptr]:=buffer[k];incr(poolptr);
end;hash[p].rh:=makestring;poolptr:=poolptr+d;
ifdef('STAT')incr(cscount);endif('STAT')end{:260};goto 40;end;
p:=hash[p].lh;end;40:idlookup:=p;end;{:259}{264:}
ifdef('INITEX')procedure primitive(s:strnumber;c:quarterword;
o:halfword);var k:poolpointer;j:smallnumber;l:smallnumber;
begin if s<256 then curval:=s+257 else begin k:=strstart[s];
l:=strstart[s+1]-k;for j:=0 to l-1 do buffer[j]:=strpool[k+j];
curval:=idlookup(0,l);begin decr(strptr);poolptr:=strstart[strptr];end;
hash[curval].rh:=s;end;eqtb[curval].hh.b1:=1;eqtb[curval].hh.b0:=c;
eqtb[curval].hh.rh:=o;end;endif('INITEX'){:264}{274:}
procedure newsavelevel(c:groupcode);
begin if saveptr>maxsavestack then begin maxsavestack:=saveptr;
if maxsavestack>savesize-6 then overflow(537,savesize);end;
savestack[saveptr].hh.b0:=3;savestack[saveptr].hh.b1:=curgroup;
savestack[saveptr].hh.rh:=curboundary;
if curlevel=511 then overflow(538,511);curboundary:=saveptr;
incr(curlevel);incr(saveptr);curgroup:=c;end;{:274}{275:}
procedure eqdestroy(w:memoryword);var q:halfword;
begin case w.hh.b0 of 111,112,113,114:deletetokenref(w.hh.rh);
117:deleteglueref(w.hh.rh);118:begin q:=w.hh.rh;
if q<>0 then freenode(q,mem[q].hh.lh+mem[q].hh.lh+1);end;
119:flushnodelist(w.hh.rh);others:end;end;{:275}{276:}
procedure eqsave(p:halfword;l:quarterword);
begin if saveptr>maxsavestack then begin maxsavestack:=saveptr;
if maxsavestack>savesize-6 then overflow(537,savesize);end;
if l=0 then savestack[saveptr].hh.b0:=1 else begin savestack[saveptr]:=
eqtb[p];incr(saveptr);savestack[saveptr].hh.b0:=0;end;
savestack[saveptr].hh.b1:=l;savestack[saveptr].hh.rh:=p;incr(saveptr);
end;{:276}{277:}procedure eqdefine(p:halfword;t:quarterword;e:halfword);
begin if eqtb[p].hh.b1=curlevel then eqdestroy(eqtb[p])else if curlevel>
1 then eqsave(p,eqtb[p].hh.b1);eqtb[p].hh.b1:=curlevel;eqtb[p].hh.b0:=t;
eqtb[p].hh.rh:=e;end;{:277}{278:}procedure eqworddefine(p:halfword;
w:integer);
begin if xeqlevel[p]<>curlevel then begin eqsave(p,xeqlevel[p]);
xeqlevel[p]:=curlevel;end;eqtb[p].int:=w;end;{:278}{279:}
procedure geqdefine(p:halfword;t:quarterword;e:halfword);
begin eqdestroy(eqtb[p]);eqtb[p].hh.b1:=1;eqtb[p].hh.b0:=t;
eqtb[p].hh.rh:=e;end;procedure geqworddefine(p:halfword;w:integer);
begin eqtb[p].int:=w;xeqlevel[p]:=1;end;{:279}{280:}
procedure saveforafter(t:halfword);
begin if curlevel>1 then begin if saveptr>maxsavestack then begin
maxsavestack:=saveptr;
if maxsavestack>savesize-6 then overflow(537,savesize);end;
savestack[saveptr].hh.b0:=2;savestack[saveptr].hh.b1:=0;
savestack[saveptr].hh.rh:=t;incr(saveptr);end;end;{:280}{281:}{284:}
ifdef('STAT')procedure restoretrace(p:halfword;s:strnumber);
begin begindiagnostic;printchar(123);print(s);printchar(32);showeqtb(p);
printchar(125);enddiagnostic(false);end;endif('STAT'){:284}
procedure backinput;forward;procedure unsave;label 30;var p:halfword;
l:quarterword;t:halfword;begin if curlevel>1 then begin decr(curlevel);
{282:}while true do begin decr(saveptr);
if savestack[saveptr].hh.b0=3 then goto 30;p:=savestack[saveptr].hh.rh;
if savestack[saveptr].hh.b0=2 then{326:}begin t:=curtok;curtok:=p;
backinput;curtok:=t;end{:326}
else begin if savestack[saveptr].hh.b0=0 then begin l:=savestack[saveptr
].hh.b1;decr(saveptr);end else savestack[saveptr]:=eqtb[10281];{283:}
if p<12663 then if eqtb[p].hh.b1=1 then begin eqdestroy(savestack[
saveptr]);ifdef('STAT')if eqtb[12700].int>0 then restoretrace(p,540);
endif('STAT')end else begin eqdestroy(eqtb[p]);
eqtb[p]:=savestack[saveptr];
ifdef('STAT')if eqtb[12700].int>0 then restoretrace(p,541);
endif('STAT')end else if xeqlevel[p]<>1 then begin eqtb[p]:=savestack[
saveptr];xeqlevel[p]:=l;
ifdef('STAT')if eqtb[12700].int>0 then restoretrace(p,541);
endif('STAT')end else begin ifdef('STAT')if eqtb[12700].int>0 then
restoretrace(p,540);endif('STAT')end{:283};end;end;
30:curgroup:=savestack[saveptr].hh.b1;
curboundary:=savestack[saveptr].hh.rh{:282};end else confusion(539);end;
{:281}{288:}procedure preparemag;
begin if(magset>0)and(eqtb[12680].int<>magset)then begin begin if
interaction=3 then wakeupterminal;printnl(262);print(543);end;
printint(eqtb[12680].int);print(544);printnl(545);begin helpptr:=2;
helpline[1]:=546;helpline[0]:=547;end;interror(magset);
geqworddefine(12680,magset);end;
if(eqtb[12680].int<=0)or(eqtb[12680].int>32768)then begin begin if
interaction=3 then wakeupterminal;printnl(262);print(548);end;
begin helpptr:=1;helpline[0]:=549;end;interror(eqtb[12680].int);
geqworddefine(12680,1000);end;magset:=eqtb[12680].int;end;{:288}{295:}
procedure tokenshow(p:halfword);
begin if p<>0 then showtokenlist(mem[p].hh.rh,0,10000000);end;{:295}
{296:}procedure printmeaning;begin printcmdchr(curcmd,curchr);
if curcmd>=111 then begin printchar(58);println;tokenshow(curchr);
end else if curcmd=110 then begin printchar(58);println;
tokenshow(curmark[curchr]);end;end;{:296}{299:}procedure showcurcmdchr;
begin begindiagnostic;printnl(123);
if curlist.modefield<>shownmode then begin printmode(curlist.modefield);
print(564);shownmode:=curlist.modefield;end;printcmdchr(curcmd,curchr);
printchar(125);enddiagnostic(false);end;{:299}{311:}
procedure showcontext;label 30;var oldsetting:0..21;nn:integer;
bottomline:boolean;{315:}i:0..bufsize;j:0..bufsize;l:0..halferrorline;
m:integer;n:0..errorline;p:integer;q:integer;{:315}
begin baseptr:=inputptr;inputstack[baseptr]:=curinput;nn:=-1;
bottomline:=false;while true do begin curinput:=inputstack[baseptr];
if(curinput.statefield<>0)then if(curinput.namefield>17)or(baseptr=0)
then bottomline:=true;
if(baseptr=inputptr)or bottomline or(nn<eqtb[12717].int)then{312:}
begin if(baseptr=inputptr)or(curinput.statefield<>0)or(curinput.
indexfield<>3)or(curinput.locfield<>0)then begin tally:=0;
oldsetting:=selector;if curinput.statefield<>0 then begin{313:}
if curinput.namefield<=17 then if(curinput.namefield=0)then if baseptr=0
then printnl(570)else printnl(571)else begin printnl(572);
if curinput.namefield=17 then printchar(42)else printint(curinput.
namefield-1);printchar(62);end else begin printnl(573);printint(line);
end;printchar(32){:313};{318:}begin l:=tally;tally:=0;selector:=20;
trickcount:=1000000;end;
if buffer[curinput.limitfield]=eqtb[12711].int then j:=curinput.
limitfield else j:=curinput.limitfield+1;
if j>0 then for i:=curinput.startfield to j-1 do begin if i=curinput.
locfield then begin firstcount:=tally;
trickcount:=tally+1+errorline-halferrorline;
if trickcount<errorline then trickcount:=errorline;end;print(buffer[i]);
end{:318};end else begin{314:}
case curinput.indexfield of 0:printnl(574);1,2:printnl(575);
3:if curinput.locfield=0 then printnl(576)else printnl(577);
4:printnl(578);5:begin println;printcs(curinput.namefield);end;
6:printnl(579);7:printnl(580);8:printnl(581);9:printnl(582);
10:printnl(583);11:printnl(584);12:printnl(585);13:printnl(586);
14:printnl(587);15:printnl(588);others:printnl(63)end{:314};{319:}
begin l:=tally;tally:=0;selector:=20;trickcount:=1000000;end;
if curinput.indexfield<5 then showtokenlist(curinput.startfield,curinput
.locfield,100000)else showtokenlist(mem[curinput.startfield].hh.rh,
curinput.locfield,100000){:319};end;selector:=oldsetting;{317:}
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
if m+n>errorline then print(275){:317};incr(nn);end;end{:312}
else if nn=eqtb[12717].int then begin printnl(275);incr(nn);end;
if bottomline then goto 30;decr(baseptr);end;
30:curinput:=inputstack[inputptr];end;{:311}{323:}
procedure begintokenlist(p:halfword;t:quarterword);
begin begin if inputptr>maxinstack then begin maxinstack:=inputptr;
if inputptr=stacksize then overflow(589,stacksize);end;
inputstack[inputptr]:=curinput;incr(inputptr);end;
curinput.statefield:=0;curinput.startfield:=p;curinput.indexfield:=t;
if t>=5 then begin incr(mem[p].hh.lh);
if t=5 then curinput.limitfield:=paramptr else begin curinput.locfield:=
mem[p].hh.rh;if eqtb[12693].int>1 then begin begindiagnostic;
printnl(335);case t of 14:printesc(348);15:printesc(590);
others:printcmdchr(72,t+10807)end;print(552);tokenshow(p);
enddiagnostic(false);end;end;end else curinput.locfield:=p;end;{:323}
{324:}procedure endtokenlist;
begin if curinput.indexfield>=3 then begin if curinput.indexfield<=4
then flushlist(curinput.startfield)else begin deletetokenref(curinput.
startfield);
if curinput.indexfield=5 then while paramptr>curinput.limitfield do
begin decr(paramptr);flushlist(paramstack[paramptr]);end;end;
end else if curinput.indexfield=1 then if alignstate>500000 then
alignstate:=0 else fatalerror(591);begin decr(inputptr);
curinput:=inputstack[inputptr];end;
begin if interrupt<>0 then pauseforinstructions;end;end;{:324}{325:}
procedure backinput;var p:halfword;
begin while(curinput.statefield=0)and(curinput.locfield=0)do
endtokenlist;p:=getavail;mem[p].hh.lh:=curtok;
if curtok<768 then if curtok<512 then decr(alignstate)else incr(
alignstate);
begin if inputptr>maxinstack then begin maxinstack:=inputptr;
if inputptr=stacksize then overflow(589,stacksize);end;
inputstack[inputptr]:=curinput;incr(inputptr);end;
curinput.statefield:=0;curinput.startfield:=p;curinput.indexfield:=3;
curinput.locfield:=p;end;{:325}{327:}procedure backerror;
begin OKtointerrupt:=false;backinput;OKtointerrupt:=true;error;end;
procedure inserror;begin OKtointerrupt:=false;backinput;
curinput.indexfield:=4;OKtointerrupt:=true;error;end;{:327}{328:}
procedure beginfilereading;
begin if inopen=maxinopen then overflow(592,maxinopen);
if first=bufsize then overflow(256,bufsize);incr(inopen);
begin if inputptr>maxinstack then begin maxinstack:=inputptr;
if inputptr=stacksize then overflow(589,stacksize);end;
inputstack[inputptr]:=curinput;incr(inputptr);end;
curinput.indexfield:=inopen;linestack[curinput.indexfield]:=line;
curinput.startfield:=first;curinput.statefield:=1;curinput.namefield:=0;
end;{:328}{329:}procedure endfilereading;
begin first:=curinput.startfield;line:=linestack[curinput.indexfield];
if curinput.namefield>17 then aclose(inputfile[curinput.indexfield]);
begin decr(inputptr);curinput:=inputstack[inputptr];end;decr(inopen);
end;{:329}{330:}procedure clearforerrorprompt;
begin while(curinput.statefield<>0)and(curinput.namefield=0)and(inputptr
>0)and(curinput.locfield>curinput.limitfield)do endfilereading;println;
clearterminal;end;{:330}{336:}procedure checkoutervalidity;
var p:halfword;q:halfword;
begin if scannerstatus<>0 then begin deletionsallowed:=false;{337:}
if curcs<>0 then begin if(curinput.statefield=0)or(curinput.namefield<1)
or(curinput.namefield>17)then begin p:=getavail;
mem[p].hh.lh:=4095+curcs;begintokenlist(p,3);end;curcmd:=10;curchr:=32;
end{:337};if scannerstatus>1 then{338:}begin runaway;
if curcs=0 then begin if interaction=3 then wakeupterminal;printnl(262);
print(600);end else begin curcs:=0;
begin if interaction=3 then wakeupterminal;printnl(262);print(601);end;
end;print(602);{339:}p:=getavail;
case scannerstatus of 2:begin print(566);mem[p].hh.lh:=637;end;
3:begin print(608);mem[p].hh.lh:=partoken;longstate:=113;end;
4:begin print(568);mem[p].hh.lh:=637;q:=p;p:=getavail;mem[p].hh.rh:=q;
mem[p].hh.lh:=14110;alignstate:=-1000000;end;5:begin print(569);
mem[p].hh.lh:=637;end;end;begintokenlist(p,4){:339};print(603);
sprintcs(warningindex);begin helpptr:=4;helpline[3]:=604;
helpline[2]:=605;helpline[1]:=606;helpline[0]:=607;end;error;end{:338}
else begin begin if interaction=3 then wakeupterminal;printnl(262);
print(594);end;printcmdchr(105,curif);print(595);printint(skipline);
begin helpptr:=3;helpline[2]:=596;helpline[1]:=597;helpline[0]:=598;end;
if curcs<>0 then curcs:=0 else helpline[2]:=599;curtok:=14113;inserror;
end;deletionsallowed:=true;end;end;{:336}{340:}procedure firmuptheline;
forward;{:340}{341:}procedure getnext;label 20,25,21,26,40,10;
var k:0..bufsize;t:halfword;cat:0..15;c,cc:ASCIIcode;d:2..3;
begin 20:curcs:=0;if curinput.statefield<>0 then{343:}
begin 25:if curinput.locfield<=curinput.limitfield then begin curchr:=
buffer[curinput.locfield];incr(curinput.locfield);
21:curcmd:=eqtb[11383+curchr].hh.rh;{344:}
case curinput.statefield+curcmd of{345:}10,26,42,27,43{:345}:goto 25;
1,17,33:{354:}
begin if curinput.locfield>curinput.limitfield then curcs:=513 else
begin 26:k:=curinput.locfield;curchr:=buffer[k];
cat:=eqtb[11383+curchr].hh.rh;incr(k);
if cat=11 then curinput.statefield:=17 else if cat=10 then curinput.
statefield:=17 else curinput.statefield:=1;
if(cat=11)and(k<=curinput.limitfield)then{356:}
begin repeat curchr:=buffer[k];cat:=eqtb[11383+curchr].hh.rh;incr(k);
until(cat<>11)or(k>curinput.limitfield);{355:}
begin if buffer[k]=curchr then if cat=7 then if k<curinput.limitfield
then begin c:=buffer[k+1];if c<128 then begin d:=2;
if(((c>=48)and(c<=57))or((c>=97)and(c<=102)))then if k+2<=curinput.
limitfield then begin cc:=buffer[k+2];
if(((cc>=48)and(cc<=57))or((cc>=97)and(cc<=102)))then incr(d);end;
if d>2 then begin if c<=57 then curchr:=c-48 else curchr:=c-87;
if cc<=57 then curchr:=16*curchr+cc-48 else curchr:=16*curchr+cc-87;
buffer[k-1]:=curchr;
end else if c<64 then buffer[k-1]:=c+64 else buffer[k-1]:=c-64;
curinput.limitfield:=curinput.limitfield-d;first:=first-d;
while k<=curinput.limitfield do begin buffer[k]:=buffer[k+d];incr(k);
end;goto 26;end;end;end{:355};if cat<>11 then decr(k);
if k>curinput.locfield+1 then begin curcs:=idlookup(curinput.locfield,k-
curinput.locfield);curinput.locfield:=k;goto 40;end;end{:356}else{355:}
begin if buffer[k]=curchr then if cat=7 then if k<curinput.limitfield
then begin c:=buffer[k+1];if c<128 then begin d:=2;
if(((c>=48)and(c<=57))or((c>=97)and(c<=102)))then if k+2<=curinput.
limitfield then begin cc:=buffer[k+2];
if(((cc>=48)and(cc<=57))or((cc>=97)and(cc<=102)))then incr(d);end;
if d>2 then begin if c<=57 then curchr:=c-48 else curchr:=c-87;
if cc<=57 then curchr:=16*curchr+cc-48 else curchr:=16*curchr+cc-87;
buffer[k-1]:=curchr;
end else if c<64 then buffer[k-1]:=c+64 else buffer[k-1]:=c-64;
curinput.limitfield:=curinput.limitfield-d;first:=first-d;
while k<=curinput.limitfield do begin buffer[k]:=buffer[k+d];incr(k);
end;goto 26;end;end;end{:355};curcs:=257+buffer[curinput.locfield];
incr(curinput.locfield);end;40:curcmd:=eqtb[curcs].hh.b0;
curchr:=eqtb[curcs].hh.rh;if curcmd>=113 then checkoutervalidity;
end{:354};14,30,46:{353:}begin curcs:=curchr+1;
curcmd:=eqtb[curcs].hh.b0;curchr:=eqtb[curcs].hh.rh;
curinput.statefield:=1;if curcmd>=113 then checkoutervalidity;end{:353};
8,24,40:{352:}
begin if curchr=buffer[curinput.locfield]then if curinput.locfield<
curinput.limitfield then begin c:=buffer[curinput.locfield+1];
if c<128 then begin curinput.locfield:=curinput.locfield+2;
if(((c>=48)and(c<=57))or((c>=97)and(c<=102)))then if curinput.locfield<=
curinput.limitfield then begin cc:=buffer[curinput.locfield];
if(((cc>=48)and(cc<=57))or((cc>=97)and(cc<=102)))then begin incr(
curinput.locfield);if c<=57 then curchr:=c-48 else curchr:=c-87;
if cc<=57 then curchr:=16*curchr+cc-48 else curchr:=16*curchr+cc-87;
goto 21;end;end;if c<64 then curchr:=c+64 else curchr:=c-64;goto 21;end;
end;curinput.statefield:=1;end{:352};16,32,48:{346:}
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(609);end;begin helpptr:=2;helpline[1]:=610;helpline[0]:=611;end;
deletionsallowed:=false;error;deletionsallowed:=true;goto 20;end{:346};
{347:}11:{349:}begin curinput.statefield:=17;curchr:=32;end{:349};
6:{348:}begin curinput.locfield:=curinput.limitfield+1;curcmd:=10;
curchr:=32;end{:348};22,15,31,47:{350:}
begin curinput.locfield:=curinput.limitfield+1;goto 25;end{:350};
38:{351:}begin curinput.locfield:=curinput.limitfield+1;curcs:=parloc;
curcmd:=eqtb[curcs].hh.b0;curchr:=eqtb[curcs].hh.rh;
if curcmd>=113 then checkoutervalidity;end{:351};2:incr(alignstate);
18,34:begin curinput.statefield:=1;incr(alignstate);end;
3:decr(alignstate);19,35:begin curinput.statefield:=1;decr(alignstate);
end;20,21,23,25,28,29,36,37,39,41,44,45:curinput.statefield:=1;{:347}
others:end{:344};end else begin curinput.statefield:=33;{360:}
if curinput.namefield>17 then{362:}begin incr(line);
first:=curinput.startfield;
if not forceeof then begin if inputln(inputfile[curinput.indexfield],
true)then firmuptheline else forceeof:=true;end;
if forceeof then begin printchar(41);decr(openparens);termflush(stdout);
forceeof:=false;endfilereading;checkoutervalidity;goto 20;end;
if(eqtb[12711].int<0)or(eqtb[12711].int>255)then decr(curinput.
limitfield)else buffer[curinput.limitfield]:=eqtb[12711].int;
first:=curinput.limitfield+1;curinput.locfield:=curinput.startfield;
end{:362}else begin if not(curinput.namefield=0)then begin curcmd:=0;
curchr:=0;goto 10;end;if inputptr>0 then begin endfilereading;goto 20;
end;if selector<18 then openlogfile;
if interaction>1 then begin if(eqtb[12711].int<0)or(eqtb[12711].int>255)
then incr(curinput.limitfield);
if curinput.limitfield=curinput.startfield then printnl(612);println;
first:=curinput.startfield;begin wakeupterminal;print(42);terminput;end;
curinput.limitfield:=last;
if(eqtb[12711].int<0)or(eqtb[12711].int>255)then decr(curinput.
limitfield)else buffer[curinput.limitfield]:=eqtb[12711].int;
first:=curinput.limitfield+1;curinput.locfield:=curinput.startfield;
end else fatalerror(613);end{:360};
begin if interrupt<>0 then pauseforinstructions;end;goto 25;end;
end{:343}else{357:}
if curinput.locfield<>0 then begin t:=mem[curinput.locfield].hh.lh;
curinput.locfield:=mem[curinput.locfield].hh.rh;
if t>=4095 then begin curcs:=t-4095;curcmd:=eqtb[curcs].hh.b0;
curchr:=eqtb[curcs].hh.rh;if curcmd>=113 then if curcmd=116 then{358:}
begin curcs:=mem[curinput.locfield].hh.lh-4095;curinput.locfield:=0;
curcmd:=eqtb[curcs].hh.b0;curchr:=eqtb[curcs].hh.rh;
if curcmd>100 then begin curcmd:=0;curchr:=257;end;end{:358}
else checkoutervalidity;end else begin curcmd:=t div 256;
curchr:=t mod 256;case curcmd of 1:incr(alignstate);2:decr(alignstate);
5:{359:}
begin begintokenlist(paramstack[curinput.limitfield+curchr-1],0);
goto 20;end{:359};others:end;end;end else begin endtokenlist;goto 20;
end{:357};{342:}
if curcmd<=5 then if curcmd>=4 then if alignstate=0 then{789:}
begin if scannerstatus=4 then fatalerror(591);
curcmd:=mem[curalign+5].hh.lh;mem[curalign+5].hh.lh:=curchr;
if curcmd=63 then begintokenlist(memtop-10,2)else begintokenlist(mem[
curalign+2].int,2);alignstate:=1000000;goto 20;end{:789}{:342};10:end;
{:341}{363:}procedure firmuptheline;var k:0..bufsize;
begin curinput.limitfield:=last;
if eqtb[12691].int>0 then if interaction>1 then begin wakeupterminal;
println;if curinput.startfield<curinput.limitfield then for k:=curinput.
startfield to curinput.limitfield-1 do print(buffer[k]);
first:=curinput.limitfield;begin wakeupterminal;print(614);terminput;
end;
if last>first then begin for k:=first to last-1 do buffer[k+curinput.
startfield-first]:=buffer[k];
curinput.limitfield:=curinput.startfield+last-first;end;end;end;{:363}
{365:}procedure gettoken;begin nonewcontrolsequence:=false;getnext;
nonewcontrolsequence:=true;
if curcs=0 then curtok:=(curcmd*256)+curchr else curtok:=4095+curcs;end;
{:365}{366:}{389:}procedure macrocall;label 10,22,30,31,40;
var r:halfword;p:halfword;q:halfword;s:halfword;t:halfword;u,v:halfword;
rbraceptr:halfword;n:smallnumber;unbalance:halfword;m:halfword;
refcount:halfword;savescannerstatus:smallnumber;
savewarningindex:halfword;matchchr:ASCIIcode;
begin savescannerstatus:=scannerstatus;savewarningindex:=warningindex;
warningindex:=curcs;refcount:=curchr;r:=mem[refcount].hh.rh;n:=0;
if eqtb[12693].int>0 then{401:}begin begindiagnostic;println;
printcs(warningindex);tokenshow(refcount);enddiagnostic(false);end{:401}
;if mem[r].hh.lh<>3584 then{391:}begin scannerstatus:=3;unbalance:=0;
longstate:=eqtb[curcs].hh.b0;
if longstate>=113 then longstate:=longstate-2;
repeat mem[memtop-3].hh.rh:=0;
if(mem[r].hh.lh>3583)or(mem[r].hh.lh<3328)then s:=0 else begin matchchr
:=mem[r].hh.lh-3328;s:=mem[r].hh.rh;r:=s;p:=memtop-3;m:=0;end;{392:}
22:gettoken;if curtok=mem[r].hh.lh then{394:}begin r:=mem[r].hh.rh;
if(mem[r].hh.lh>=3328)and(mem[r].hh.lh<=3584)then begin if curtok<512
then decr(alignstate);goto 40;end else goto 22;end{:394};{397:}
if s<>r then if s=0 then{398:}
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(646);end;sprintcs(warningindex);print(647);begin helpptr:=4;
helpline[3]:=648;helpline[2]:=649;helpline[1]:=650;helpline[0]:=651;end;
error;goto 10;end{:398}else begin t:=s;repeat begin q:=getavail;
mem[p].hh.rh:=q;mem[q].hh.lh:=mem[t].hh.lh;p:=q;end;incr(m);
u:=mem[t].hh.rh;v:=s;
while true do begin if u=r then if curtok<>mem[v].hh.lh then goto 30
else begin r:=mem[v].hh.rh;goto 22;end;
if mem[u].hh.lh<>mem[v].hh.lh then goto 30;u:=mem[u].hh.rh;
v:=mem[v].hh.rh;end;30:t:=mem[t].hh.rh;until t=r;r:=s;end{:397};
if curtok=partoken then if longstate<>112 then{396:}
begin if longstate=111 then begin runaway;
begin if interaction=3 then wakeupterminal;printnl(262);print(641);end;
sprintcs(warningindex);print(642);begin helpptr:=3;helpline[2]:=643;
helpline[1]:=644;helpline[0]:=645;end;backerror;end;
pstack[n]:=mem[memtop-3].hh.rh;alignstate:=alignstate-unbalance;
for m:=0 to n do flushlist(pstack[m]);goto 10;end{:396};
if curtok<768 then if curtok<512 then{399:}begin unbalance:=1;
while true do begin begin begin q:=avail;
if q=0 then q:=getavail else begin avail:=mem[q].hh.rh;mem[q].hh.rh:=0;
ifdef('STAT')incr(dynused);endif('STAT')end;end;mem[p].hh.rh:=q;
mem[q].hh.lh:=curtok;p:=q;end;gettoken;
if curtok=partoken then if longstate<>112 then{396:}
begin if longstate=111 then begin runaway;
begin if interaction=3 then wakeupterminal;printnl(262);print(641);end;
sprintcs(warningindex);print(642);begin helpptr:=3;helpline[2]:=643;
helpline[1]:=644;helpline[0]:=645;end;backerror;end;
pstack[n]:=mem[memtop-3].hh.rh;alignstate:=alignstate-unbalance;
for m:=0 to n do flushlist(pstack[m]);goto 10;end{:396};
if curtok<768 then if curtok<512 then incr(unbalance)else begin decr(
unbalance);if unbalance=0 then goto 31;end;end;31:rbraceptr:=p;
begin q:=getavail;mem[p].hh.rh:=q;mem[q].hh.lh:=curtok;p:=q;end;
end{:399}else{395:}begin backinput;
begin if interaction=3 then wakeupterminal;printnl(262);print(633);end;
sprintcs(warningindex);print(634);begin helpptr:=6;helpline[5]:=635;
helpline[4]:=636;helpline[3]:=637;helpline[2]:=638;helpline[1]:=639;
helpline[0]:=640;end;incr(alignstate);longstate:=111;curtok:=partoken;
inserror;end{:395}else{393:}
begin if curtok=2592 then if mem[r].hh.lh<=3584 then if mem[r].hh.lh>=
3328 then goto 22;begin q:=getavail;mem[p].hh.rh:=q;
mem[q].hh.lh:=curtok;p:=q;end;end{:393};incr(m);
if mem[r].hh.lh>3584 then goto 22;if mem[r].hh.lh<3328 then goto 22;
40:if s<>0 then{400:}
begin if(m=1)and(mem[p].hh.lh<768)and(p<>memtop-3)then begin mem[
rbraceptr].hh.rh:=0;begin mem[p].hh.rh:=avail;avail:=p;
ifdef('STAT')decr(dynused);endif('STAT')end;p:=mem[memtop-3].hh.rh;
pstack[n]:=mem[p].hh.rh;begin mem[p].hh.rh:=avail;avail:=p;
ifdef('STAT')decr(dynused);endif('STAT')end;
end else pstack[n]:=mem[memtop-3].hh.rh;incr(n);
if eqtb[12693].int>0 then begin begindiagnostic;printnl(matchchr);
printint(n);print(652);showtokenlist(pstack[n-1],0,1000);
enddiagnostic(false);end;end{:400}{:392};until mem[r].hh.lh=3584;
end{:391};{390:}
while(curinput.statefield=0)and(curinput.locfield=0)do endtokenlist;
begintokenlist(refcount,5);curinput.namefield:=warningindex;
curinput.locfield:=mem[r].hh.rh;
if n>0 then begin if paramptr+n>maxparamstack then begin maxparamstack:=
paramptr+n;if maxparamstack>paramsize then overflow(632,paramsize);end;
for m:=0 to n-1 do paramstack[paramptr+m]:=pstack[m];
paramptr:=paramptr+n;end{:390};10:scannerstatus:=savescannerstatus;
warningindex:=savewarningindex;end;{:389}{379:}procedure insertrelax;
begin curtok:=4095+curcs;backinput;curtok:=14116;backinput;
curinput.indexfield:=4;end;{:379}procedure passtext;forward;
procedure startinput;forward;procedure conditional;forward;
procedure getxtoken;forward;procedure convtoks;forward;
procedure insthetoks;forward;procedure expand;var t:halfword;
p,q,r:halfword;j:0..bufsize;cvbackup:integer;
cvlbackup,radixbackup,cobackup:smallnumber;backupbackup:halfword;
savescannerstatus:smallnumber;begin cvbackup:=curval;
cvlbackup:=curvallevel;radixbackup:=radix;cobackup:=curorder;
backupbackup:=mem[memtop-13].hh.rh;if curcmd<111 then{367:}
begin if eqtb[12699].int>1 then showcurcmdchr;case curcmd of 110:{386:}
begin if curmark[curchr]<>0 then begintokenlist(curmark[curchr],14);
end{:386};102:{368:}begin gettoken;t:=curtok;gettoken;
if curcmd>100 then expand else backinput;curtok:=t;backinput;end{:368};
103:{369:}begin savescannerstatus:=scannerstatus;scannerstatus:=0;
gettoken;scannerstatus:=savescannerstatus;t:=curtok;backinput;
if t>=4095 then begin p:=getavail;mem[p].hh.lh:=14118;
mem[p].hh.rh:=curinput.locfield;curinput.startfield:=p;
curinput.locfield:=p;end;end{:369};107:{372:}begin r:=getavail;p:=r;
repeat getxtoken;if curcs=0 then begin q:=getavail;mem[p].hh.rh:=q;
mem[q].hh.lh:=curtok;p:=q;end;until curcs<>0;if curcmd<>67 then{373:}
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(621);end;printesc(501);print(622);begin helpptr:=2;
helpline[1]:=623;helpline[0]:=624;end;backerror;end{:373};{374:}
j:=first;p:=mem[r].hh.rh;
while p<>0 do begin if j>=maxbufstack then begin maxbufstack:=j+1;
if maxbufstack=bufsize then overflow(256,bufsize);end;
buffer[j]:=mem[p].hh.lh mod 256;incr(j);p:=mem[p].hh.rh;end;
if j>first+1 then begin nonewcontrolsequence:=false;
curcs:=idlookup(first,j-first);nonewcontrolsequence:=true;
end else if j=first then curcs:=513 else curcs:=257+buffer[first]{:374};
flushlist(r);if eqtb[curcs].hh.b0=101 then begin eqdefine(curcs,0,256);
end;curtok:=curcs+4095;backinput;end{:372};108:convtoks;109:insthetoks;
105:conditional;106:{510:}
if curchr>iflimit then if iflimit=1 then insertrelax else begin begin if
interaction=3 then wakeupterminal;printnl(262);print(772);end;
printcmdchr(106,curchr);begin helpptr:=1;helpline[0]:=773;end;error;
end else begin while curchr<>2 do passtext;{496:}begin p:=condptr;
ifline:=mem[p+1].int;curif:=mem[p].hh.b1;iflimit:=mem[p].hh.b0;
condptr:=mem[p].hh.rh;freenode(p,2);end{:496};end{:510};104:{378:}
if curchr>0 then forceeof:=true else if nameinprogress then insertrelax
else startinput{:378};others:{370:}
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(615);end;begin helpptr:=5;helpline[4]:=616;helpline[3]:=617;
helpline[2]:=618;helpline[1]:=619;helpline[0]:=620;end;error;end{:370}
end;end{:367}else if curcmd<115 then macrocall else{375:}
begin curtok:=14115;backinput;end{:375};curval:=cvbackup;
curvallevel:=cvlbackup;radix:=radixbackup;curorder:=cobackup;
mem[memtop-13].hh.rh:=backupbackup;end;{:366}{380:}procedure getxtoken;
label 20,30;begin 20:getnext;if curcmd<=100 then goto 30;
if curcmd>=111 then if curcmd<115 then macrocall else begin curcs:=10020
;curcmd:=9;goto 30;end else expand;goto 20;
30:if curcs=0 then curtok:=(curcmd*256)+curchr else curtok:=4095+curcs;
end;{:380}{381:}procedure xtoken;begin while curcmd>100 do begin expand;
getnext;end;
if curcs=0 then curtok:=(curcmd*256)+curchr else curtok:=4095+curcs;end;
{:381}{403:}procedure scanleftbrace;begin{404:}repeat getxtoken;
until(curcmd<>10)and(curcmd<>0){:404};
if curcmd<>1 then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(653);end;begin helpptr:=4;helpline[3]:=654;
helpline[2]:=655;helpline[1]:=656;helpline[0]:=657;end;backerror;
curtok:=379;curcmd:=1;curchr:=123;incr(alignstate);end;end;{:403}{405:}
procedure scanoptionalequals;begin{406:}repeat getxtoken;
until curcmd<>10{:406};if curtok<>3133 then backinput;end;{:405}{407:}
function scankeyword(s:strnumber):boolean;label 10;var p:halfword;
q:halfword;k:poolpointer;begin p:=memtop-13;mem[p].hh.rh:=0;
k:=strstart[s];while k<strstart[s+1]do begin getxtoken;
if(curcs=0)and((curchr=strpool[k])or(curchr=strpool[k]-32))then begin
begin q:=getavail;mem[p].hh.rh:=q;mem[q].hh.lh:=curtok;p:=q;end;incr(k);
end else if(curcmd<>10)or(p<>memtop-13)then begin backinput;
if p<>memtop-13 then begintokenlist(mem[memtop-13].hh.rh,3);
scankeyword:=false;goto 10;end;end;flushlist(mem[memtop-13].hh.rh);
scankeyword:=true;10:end;{:407}{408:}procedure muerror;
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(658);end;begin helpptr:=1;helpline[0]:=659;end;error;end;{:408}
{409:}procedure scanint;forward;{433:}procedure scaneightbitint;
begin scanint;
if(curval<0)or(curval>255)then begin begin if interaction=3 then
wakeupterminal;printnl(262);print(683);end;begin helpptr:=2;
helpline[1]:=684;helpline[0]:=685;end;interror(curval);curval:=0;end;
end;{:433}{434:}procedure scancharnum;begin scanint;
if(curval<0)or(curval>255)then begin begin if interaction=3 then
wakeupterminal;printnl(262);print(686);end;begin helpptr:=2;
helpline[1]:=687;helpline[0]:=685;end;interror(curval);curval:=0;end;
end;{:434}{435:}procedure scanfourbitint;begin scanint;
if(curval<0)or(curval>15)then begin begin if interaction=3 then
wakeupterminal;printnl(262);print(688);end;begin helpptr:=2;
helpline[1]:=689;helpline[0]:=685;end;interror(curval);curval:=0;end;
end;{:435}{436:}procedure scanfifteenbitint;begin scanint;
if(curval<0)or(curval>32767)then begin begin if interaction=3 then
wakeupterminal;printnl(262);print(690);end;begin helpptr:=2;
helpline[1]:=691;helpline[0]:=685;end;interror(curval);curval:=0;end;
end;{:436}{437:}procedure scantwentysevenbitint;begin scanint;
if(curval<0)or(curval>134217727)then begin begin if interaction=3 then
wakeupterminal;printnl(262);print(692);end;begin helpptr:=2;
helpline[1]:=693;helpline[0]:=685;end;interror(curval);curval:=0;end;
end;{:437}{577:}procedure scanfontident;var f:internalfontnumber;
m:halfword;begin{406:}repeat getxtoken;until curcmd<>10{:406};
if curcmd=88 then f:=eqtb[11334].hh.rh else if curcmd=87 then f:=curchr
else if curcmd=86 then begin m:=curchr;scanfourbitint;
f:=eqtb[m+curval].hh.rh;
end else begin begin if interaction=3 then wakeupterminal;printnl(262);
print(810);end;begin helpptr:=2;helpline[1]:=811;helpline[0]:=812;end;
backerror;f:=0;end;curval:=f;end;{:577}{578:}
procedure findfontdimen(writing:boolean);var f:internalfontnumber;
n:integer;begin scanint;n:=curval;scanfontident;f:=curval;
if n<=0 then curval:=fmemptr else begin if writing and(n<=4)and(n>=2)and
(fontglue[f]<>0)then begin deleteglueref(fontglue[f]);fontglue[f]:=0;
end;if n>fontparams[f]then if f<fontptr then curval:=fmemptr else{580:}
begin repeat if fmemptr=fontmemsize then overflow(817,fontmemsize);
fontinfo[fmemptr].int:=0;incr(fmemptr);incr(fontparams[f]);
until n=fontparams[f];curval:=fmemptr-1;end{:580}
else curval:=n+parambase[f];end;{579:}
if curval=fmemptr then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(795);end;printesc(hash[10024+f].rh);print(813);
printint(fontparams[f]);print(814);begin helpptr:=2;helpline[1]:=815;
helpline[0]:=816;end;error;end{:579};end;{:578}{:409}{413:}
procedure scansomethinginternal(level:smallnumber;negative:boolean);
var m:halfword;p:0..nestsize;begin m:=curchr;case curcmd of 85:{414:}
begin scancharnum;
if m=12407 then begin curval:=eqtb[12407+curval].hh.rh;curvallevel:=0;
end else if m<12407 then begin curval:=eqtb[m+curval].hh.rh;
curvallevel:=0;end else begin curval:=eqtb[m+curval].int;curvallevel:=0;
end;end{:414};71,72,86,87,88:{415:}
if level<>5 then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(660);end;begin helpptr:=3;helpline[2]:=661;
helpline[1]:=662;helpline[0]:=663;end;backerror;begin curval:=0;
curvallevel:=1;end;
end else if curcmd<=72 then begin if curcmd<72 then begin
scaneightbitint;m:=10822+curval;end;begin curval:=eqtb[m].hh.rh;
curvallevel:=5;end;end else begin backinput;scanfontident;
begin curval:=10024+curval;curvallevel:=4;end;end{:415};
73:begin curval:=eqtb[m].int;curvallevel:=0;end;
74:begin curval:=eqtb[m].int;curvallevel:=1;end;
75:begin curval:=eqtb[m].hh.rh;curvallevel:=2;end;
76:begin curval:=eqtb[m].hh.rh;curvallevel:=3;end;79:{418:}
if abs(curlist.modefield)<>m then begin begin if interaction=3 then
wakeupterminal;printnl(262);print(676);end;printcmdchr(79,m);
begin helpptr:=4;helpline[3]:=677;helpline[2]:=678;helpline[1]:=679;
helpline[0]:=680;end;error;if level<>5 then begin curval:=0;
curvallevel:=1;end else begin curval:=0;curvallevel:=0;end;
end else if m=1 then begin curval:=curlist.auxfield.int;curvallevel:=1;
end else begin curval:=curlist.auxfield.hh.lh;curvallevel:=0;end{:418};
80:{422:}begin nest[nestptr]:=curlist;p:=nestptr;
while abs(nest[p].modefield)<>1 do decr(p);
begin curval:=nest[p].pgfield;curvallevel:=0;end;end{:422};82:{419:}
begin if m=0 then curval:=deadcycles else curval:=insertpenalties;
curvallevel:=0;end{:419};81:{421:}
begin if(pagecontents=0)and(not outputactive)then if m=0 then curval:=
1073741823 else curval:=0 else curval:=pagesofar[m];curvallevel:=1;
end{:421};84:{423:}
begin if eqtb[10812].hh.rh=0 then curval:=0 else curval:=mem[eqtb[10812]
.hh.rh].hh.lh;curvallevel:=0;end{:423};83:{420:}begin scaneightbitint;
if eqtb[11078+curval].hh.rh=0 then curval:=0 else curval:=mem[eqtb[
11078+curval].hh.rh+m].int;curvallevel:=1;end{:420};
68,69:begin curval:=curchr;curvallevel:=0;end;77:{425:}
begin findfontdimen(false);fontinfo[fmemptr].int:=0;
begin curval:=fontinfo[curval].int;curvallevel:=1;end;end{:425};
78:{426:}begin scanfontident;
if m=0 then begin curval:=hyphenchar[curval];curvallevel:=0;
end else begin curval:=skewchar[curval];curvallevel:=0;end;end{:426};
89:{427:}begin scaneightbitint;
case m of 0:curval:=eqtb[12718+curval].int;
1:curval:=eqtb[13251+curval].int;2:curval:=eqtb[10300+curval].hh.rh;
3:curval:=eqtb[10556+curval].hh.rh;end;curvallevel:=m;end{:427};
70:{424:}
if curchr>2 then begin if curchr=3 then curval:=line else curval:=
lastbadness;curvallevel:=0;
end else begin if curchr=2 then curval:=0 else curval:=0;
curvallevel:=curchr;
if not(curlist.tailfield>=himemmin)and(curlist.modefield<>0)then case
curchr of 0:if mem[curlist.tailfield].hh.b0=12 then curval:=mem[curlist.
tailfield+1].int;
1:if mem[curlist.tailfield].hh.b0=11 then curval:=mem[curlist.tailfield
+1].int;
2:if mem[curlist.tailfield].hh.b0=10 then begin curval:=mem[curlist.
tailfield+1].hh.lh;
if mem[curlist.tailfield].hh.b1=99 then curvallevel:=3;end;
end else if(curlist.modefield=1)and(curlist.tailfield=curlist.headfield)
then case curchr of 0:curval:=lastpenalty;1:curval:=lastkern;
2:if lastglue<>262143 then curval:=lastglue;end;end{:424};others:{428:}
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(681);end;printcmdchr(curcmd,curchr);print(682);printesc(533);
begin helpptr:=1;helpline[0]:=680;end;error;
if level<>5 then begin curval:=0;curvallevel:=1;
end else begin curval:=0;curvallevel:=0;end;end{:428}end;
while curvallevel>level do{429:}
begin if curvallevel=2 then curval:=mem[curval+1].int else if
curvallevel=3 then muerror;decr(curvallevel);end{:429};{430:}
if negative then if curvallevel>=2 then begin curval:=newspec(curval);
{431:}begin mem[curval+1].int:=-mem[curval+1].int;
mem[curval+2].int:=-mem[curval+2].int;
mem[curval+3].int:=-mem[curval+3].int;end{:431};
end else curval:=-curval else if(curvallevel>=2)and(curvallevel<=3)then
incr(mem[curval].hh.rh){:430};end;{:413}{440:}procedure scanint;
label 30;var negative:boolean;m:integer;d:smallnumber;vacuous:boolean;
OKsofar:boolean;begin radix:=0;OKsofar:=true;{441:}negative:=false;
repeat{406:}repeat getxtoken;until curcmd<>10{:406};
if curtok=3117 then begin negative:=not negative;curtok:=3115;end;
until curtok<>3115{:441};if curtok=3168 then{442:}begin gettoken;
if curtok<4095 then begin curval:=curchr;
if curcmd<=2 then if curcmd=2 then incr(alignstate)else decr(alignstate)
;end else if curtok<4352 then curval:=curtok-4096 else curval:=curtok
-4352;
if curval>255 then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(694);end;begin helpptr:=2;helpline[1]:=695;
helpline[0]:=696;end;curval:=48;backerror;end else{443:}begin getxtoken;
if curcmd<>10 then backinput;end{:443};end{:442}
else if(curcmd>=68)and(curcmd<=89)then scansomethinginternal(0,false)
else{444:}begin radix:=10;m:=214748364;
if curtok=3111 then begin radix:=8;m:=268435456;getxtoken;
end else if curtok=3106 then begin radix:=16;m:=134217728;getxtoken;end;
vacuous:=true;curval:=0;{445:}
while true do begin if(curtok<3120+radix)and(curtok>=3120)and(curtok<=
3129)then d:=curtok-3120 else if radix=16 then if(curtok<=2886)and(
curtok>=2881)then d:=curtok-2871 else if(curtok<=3142)and(curtok>=3137)
then d:=curtok-3127 else goto 30 else goto 30;vacuous:=false;
if(curval>=m)and((curval>m)or(d>7)or(radix<>10))then begin if OKsofar
then begin begin if interaction=3 then wakeupterminal;printnl(262);
print(697);end;begin helpptr:=2;helpline[1]:=698;helpline[0]:=699;end;
error;curval:=2147483647;OKsofar:=false;end;
end else curval:=curval*radix+d;getxtoken;end;30:{:445};
if vacuous then{446:}begin begin if interaction=3 then wakeupterminal;
printnl(262);print(660);end;begin helpptr:=3;helpline[2]:=661;
helpline[1]:=662;helpline[0]:=663;end;backerror;end{:446}
else if curcmd<>10 then backinput;end{:444};
if negative then curval:=-curval;end;{:440}{448:}
procedure scandimen(mu,inf,shortcut:boolean);label 30,31,32,40,45,88,89;
var negative:boolean;f:integer;{450:}num,denom:1..65536;
k,kk:smallnumber;p,q:halfword;v:scaled;savecurval:integer;{:450}
begin f:=0;aritherror:=false;curorder:=0;negative:=false;
if not shortcut then begin{441:}negative:=false;repeat{406:}
repeat getxtoken;until curcmd<>10{:406};
if curtok=3117 then begin negative:=not negative;curtok:=3115;end;
until curtok<>3115{:441};if(curcmd>=68)and(curcmd<=89)then{449:}
if mu then begin scansomethinginternal(3,false);{451:}
if curvallevel>=2 then begin v:=mem[curval+1].int;deleteglueref(curval);
curval:=v;end{:451};if curvallevel=3 then goto 89;
if curvallevel<>0 then muerror;
end else begin scansomethinginternal(1,false);
if curvallevel=1 then goto 89;end{:449}else begin backinput;
if curtok=3116 then curtok:=3118;
if curtok<>3118 then scanint else begin radix:=10;curval:=0;end;
if curtok=3116 then curtok:=3118;if(radix=10)and(curtok=3118)then{452:}
begin k:=0;p:=0;gettoken;while true do begin getxtoken;
if(curtok>3129)or(curtok<3120)then goto 31;
if k<17 then begin q:=getavail;mem[q].hh.rh:=p;
mem[q].hh.lh:=curtok-3120;p:=q;incr(k);end;end;
31:for kk:=k downto 1 do begin dig[kk-1]:=mem[p].hh.lh;q:=p;
p:=mem[p].hh.rh;begin mem[q].hh.rh:=avail;avail:=q;
ifdef('STAT')decr(dynused);endif('STAT')end;end;f:=rounddecimals(k);
if curcmd<>10 then backinput;end{:452};end;end;
if curval<0 then begin negative:=not negative;curval:=-curval;end;{453:}
if inf then{454:}if scankeyword(309)then begin curorder:=1;
while scankeyword(108)do begin if curorder=3 then begin begin if
interaction=3 then wakeupterminal;printnl(262);print(701);end;
print(702);begin helpptr:=1;helpline[0]:=703;end;error;
end else incr(curorder);end;goto 88;end{:454};{455:}savecurval:=curval;
{406:}repeat getxtoken;until curcmd<>10{:406};
if(curcmd<68)or(curcmd>89)then backinput else begin if mu then begin
scansomethinginternal(3,false);{451:}
if curvallevel>=2 then begin v:=mem[curval+1].int;deleteglueref(curval);
curval:=v;end{:451};if curvallevel<>3 then muerror;
end else scansomethinginternal(1,false);v:=curval;goto 40;end;
if mu then goto 45;if scankeyword(704)then v:=({558:}
fontinfo[6+parambase[eqtb[11334].hh.rh]].int{:558}
)else if scankeyword(705)then v:=({559:}
fontinfo[5+parambase[eqtb[11334].hh.rh]].int{:559})else goto 45;{443:}
begin getxtoken;if curcmd<>10 then backinput;end{:443};
40:curval:=multandadd(savecurval,v,xnoverd(v,f,65536),1073741823);
goto 89;45:{:455};if mu then{456:}
if scankeyword(334)then goto 88 else begin begin if interaction=3 then
wakeupterminal;printnl(262);print(701);end;print(706);begin helpptr:=4;
helpline[3]:=707;helpline[2]:=708;helpline[1]:=709;helpline[0]:=710;end;
error;goto 88;end{:456};if scankeyword(700)then{457:}begin preparemag;
if eqtb[12680].int<>1000 then begin curval:=xnoverd(curval,1000,eqtb[
12680].int);f:=(1000*f+65536*remainder)div eqtb[12680].int;
curval:=curval+(f div 65536);f:=f mod 65536;end;end{:457};
if scankeyword(393)then goto 88;{458:}
if scankeyword(711)then begin num:=7227;denom:=100;
end else if scankeyword(712)then begin num:=12;denom:=1;
end else if scankeyword(713)then begin num:=7227;denom:=254;
end else if scankeyword(714)then begin num:=7227;denom:=2540;
end else if scankeyword(715)then begin num:=7227;denom:=7200;
end else if scankeyword(716)then begin num:=1238;denom:=1157;
end else if scankeyword(717)then begin num:=14856;denom:=1157;
end else if scankeyword(718)then goto 30 else{459:}
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(701);end;print(719);begin helpptr:=6;helpline[5]:=720;
helpline[4]:=721;helpline[3]:=722;helpline[2]:=708;helpline[1]:=709;
helpline[0]:=710;end;error;goto 32;end{:459};
curval:=xnoverd(curval,num,denom);f:=(num*f+65536*remainder)div denom;
curval:=curval+(f div 65536);f:=f mod 65536;32:{:458};
88:if curval>=16384 then aritherror:=true else curval:=curval*65536+f;
30:{:453};{443:}begin getxtoken;if curcmd<>10 then backinput;end{:443};
89:if aritherror or(abs(curval)>=1073741824)then{460:}
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(723);end;begin helpptr:=2;helpline[1]:=724;helpline[0]:=725;end;
error;curval:=1073741823;aritherror:=false;end{:460};
if negative then curval:=-curval;end;{:448}{461:}
procedure scanglue(level:smallnumber);label 10;var negative:boolean;
q:halfword;mu:boolean;begin mu:=(level=3);{441:}negative:=false;
repeat{406:}repeat getxtoken;until curcmd<>10{:406};
if curtok=3117 then begin negative:=not negative;curtok:=3115;end;
until curtok<>3115{:441};
if(curcmd>=68)and(curcmd<=89)then begin scansomethinginternal(level,
negative);
if curvallevel>=2 then begin if curvallevel<>level then muerror;goto 10;
end;if curvallevel=0 then scandimen(mu,false,true)else if level=3 then
muerror;end else begin backinput;scandimen(mu,false,false);
if negative then curval:=-curval;end;{462:}q:=newspec(0);
mem[q+1].int:=curval;
if scankeyword(726)then begin scandimen(mu,true,false);
mem[q+2].int:=curval;mem[q].hh.b0:=curorder;end;
if scankeyword(727)then begin scandimen(mu,true,false);
mem[q+3].int:=curval;mem[q].hh.b1:=curorder;end;curval:=q{:462};10:end;
{:461}{463:}function scanrulespec:halfword;label 21;var q:halfword;
begin q:=newrule;
if curcmd=35 then mem[q+1].int:=26214 else begin mem[q+3].int:=26214;
mem[q+2].int:=0;end;
21:if scankeyword(728)then begin scandimen(false,false,false);
mem[q+1].int:=curval;goto 21;end;
if scankeyword(729)then begin scandimen(false,false,false);
mem[q+3].int:=curval;goto 21;end;
if scankeyword(730)then begin scandimen(false,false,false);
mem[q+2].int:=curval;goto 21;end;scanrulespec:=q;end;{:463}{464:}
function strtoks(b:poolpointer):halfword;var p:halfword;q:halfword;
t:halfword;k:poolpointer;
begin begin if poolptr+1>poolsize then overflow(257,poolsize-initpoolptr
);end;p:=memtop-3;mem[p].hh.rh:=0;k:=b;
while k<poolptr do begin t:=strpool[k];
if t=32 then t:=2592 else t:=3072+t;begin begin q:=avail;
if q=0 then q:=getavail else begin avail:=mem[q].hh.rh;mem[q].hh.rh:=0;
ifdef('STAT')incr(dynused);endif('STAT')end;end;mem[p].hh.rh:=q;
mem[q].hh.lh:=t;p:=q;end;incr(k);end;poolptr:=b;strtoks:=p;end;{:464}
{465:}function thetoks:halfword;var oldsetting:0..21;p,q,r:halfword;
b:poolpointer;begin getxtoken;scansomethinginternal(5,false);
if curvallevel>=4 then{466:}begin p:=memtop-3;mem[p].hh.rh:=0;
if curvallevel=4 then begin q:=getavail;mem[p].hh.rh:=q;
mem[q].hh.lh:=4095+curval;p:=q;
end else if curval<>0 then begin r:=mem[curval].hh.rh;
while r<>0 do begin begin begin q:=avail;
if q=0 then q:=getavail else begin avail:=mem[q].hh.rh;mem[q].hh.rh:=0;
ifdef('STAT')incr(dynused);endif('STAT')end;end;mem[p].hh.rh:=q;
mem[q].hh.lh:=mem[r].hh.lh;p:=q;end;r:=mem[r].hh.rh;end;end;thetoks:=p;
end{:466}else begin oldsetting:=selector;selector:=21;b:=poolptr;
case curvallevel of 0:printint(curval);1:begin printscaled(curval);
print(393);end;2:begin printspec(curval,393);deleteglueref(curval);end;
3:begin printspec(curval,334);deleteglueref(curval);end;end;
selector:=oldsetting;thetoks:=strtoks(b);end;end;{:465}{467:}
procedure insthetoks;begin mem[memtop-12].hh.rh:=thetoks;
begintokenlist(mem[memtop-3].hh.rh,4);end;{:467}{470:}
procedure convtoks;var oldsetting:0..21;c:0..5;
savescannerstatus:smallnumber;b:poolpointer;begin c:=curchr;{471:}
case c of 0,1:scanint;2,3:begin savescannerstatus:=scannerstatus;
scannerstatus:=0;gettoken;scannerstatus:=savescannerstatus;end;
4:scanfontident;5:if jobname=0 then openlogfile;end{:471};
oldsetting:=selector;selector:=21;b:=poolptr;{472:}
case c of 0:printint(curval);1:printromanint(curval);
2:if curcs<>0 then sprintcs(curcs)else printchar(curchr);3:printmeaning;
4:begin print(fontname[curval]);
if fontsize[curval]<>fontdsize[curval]then begin print(737);
printscaled(fontsize[curval]);print(393);end;end;5:print(jobname);
end{:472};selector:=oldsetting;mem[memtop-12].hh.rh:=strtoks(b);
begintokenlist(mem[memtop-3].hh.rh,4);end;{:470}{473:}
function scantoks(macrodef,xpand:boolean):halfword;label 40,30,31,32;
var t:halfword;s:halfword;p:halfword;q:halfword;unbalance:halfword;
hashbrace:halfword;
begin if macrodef then scannerstatus:=2 else scannerstatus:=5;
warningindex:=curcs;defref:=getavail;mem[defref].hh.lh:=0;p:=defref;
hashbrace:=0;t:=3120;if macrodef then{474:}
begin while true do begin gettoken;if curtok<768 then goto 31;
if curcmd=6 then{476:}begin s:=3328+curchr;gettoken;
if curcmd=1 then begin hashbrace:=curtok;begin q:=getavail;
mem[p].hh.rh:=q;mem[q].hh.lh:=curtok;p:=q;end;begin q:=getavail;
mem[p].hh.rh:=q;mem[q].hh.lh:=3584;p:=q;end;goto 30;end;
if t=3129 then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(740);end;begin helpptr:=1;helpline[0]:=741;end;error;
end else begin incr(t);
if curtok<>t then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(742);end;begin helpptr:=2;helpline[1]:=743;
helpline[0]:=744;end;backerror;end;curtok:=s;end;end{:476};
begin q:=getavail;mem[p].hh.rh:=q;mem[q].hh.lh:=curtok;p:=q;end;end;
31:begin q:=getavail;mem[p].hh.rh:=q;mem[q].hh.lh:=3584;p:=q;end;
if curcmd=2 then{475:}begin begin if interaction=3 then wakeupterminal;
printnl(262);print(653);end;incr(alignstate);begin helpptr:=2;
helpline[1]:=738;helpline[0]:=739;end;error;goto 40;end{:475};
30:end{:474}else scanleftbrace;{477:}unbalance:=1;
while true do begin if xpand then{478:}
begin while true do begin getnext;if curcmd<=100 then goto 32;
if curcmd<>109 then expand else begin q:=thetoks;
if mem[memtop-3].hh.rh<>0 then begin mem[p].hh.rh:=mem[memtop-3].hh.rh;
p:=q;end;end;end;32:xtoken end{:478}else gettoken;
if curtok<768 then if curcmd<2 then incr(unbalance)else begin decr(
unbalance);if unbalance=0 then goto 40;
end else if curcmd=6 then if macrodef then{479:}begin s:=curtok;
if xpand then getxtoken else gettoken;
if curcmd<>6 then if(curtok<=3120)or(curtok>t)then begin begin if
interaction=3 then wakeupterminal;printnl(262);print(745);end;
sprintcs(warningindex);begin helpptr:=3;helpline[2]:=746;
helpline[1]:=747;helpline[0]:=748;end;backerror;curtok:=s;
end else curtok:=1232+curchr;end{:479};begin q:=getavail;
mem[p].hh.rh:=q;mem[q].hh.lh:=curtok;p:=q;end;end{:477};
40:scannerstatus:=0;if hashbrace<>0 then begin q:=getavail;
mem[p].hh.rh:=q;mem[q].hh.lh:=hashbrace;p:=q;end;scantoks:=p;end;{:473}
{482:}procedure readtoks(n:integer;r:halfword);label 30;var p:halfword;
q:halfword;s:integer;m:smallnumber;begin scannerstatus:=2;
warningindex:=r;defref:=getavail;mem[defref].hh.lh:=0;p:=defref;
begin q:=getavail;mem[p].hh.rh:=q;mem[q].hh.lh:=3584;p:=q;end;
if(n<0)or(n>15)then m:=16 else m:=n;s:=alignstate;alignstate:=1000000;
repeat{483:}beginfilereading;curinput.namefield:=m+1;
if readopen[m]=2 then{484:}
if interaction>1 then if n<0 then begin wakeupterminal;print(335);
terminput;end else begin wakeupterminal;println;sprintcs(r);
begin wakeupterminal;print(61);terminput;end;n:=-1;
end else fatalerror(749){:484}else if readopen[m]=1 then{485:}
if inputln(readfile[m],false)then readopen[m]:=0 else begin aclose(
readfile[m]);readopen[m]:=2;end{:485}else{486:}
begin if not inputln(readfile[m],true)then begin aclose(readfile[m]);
readopen[m]:=2;if alignstate<>1000000 then begin runaway;
begin if interaction=3 then wakeupterminal;printnl(262);print(750);end;
printesc(530);begin helpptr:=1;helpline[0]:=751;end;alignstate:=1000000;
error;end;end;end{:486};curinput.limitfield:=last;
if(eqtb[12711].int<0)or(eqtb[12711].int>255)then decr(curinput.
limitfield)else buffer[curinput.limitfield]:=eqtb[12711].int;
first:=curinput.limitfield+1;curinput.locfield:=curinput.startfield;
curinput.statefield:=33;while true do begin gettoken;
if curtok=0 then goto 30;begin q:=getavail;mem[p].hh.rh:=q;
mem[q].hh.lh:=curtok;p:=q;end;end;30:endfilereading{:483};
until alignstate=1000000;curval:=defref;scannerstatus:=0;alignstate:=s;
end;{:482}{494:}procedure passtext;label 30;var l:integer;
savescannerstatus:smallnumber;begin savescannerstatus:=scannerstatus;
scannerstatus:=1;l:=0;skipline:=line;while true do begin getnext;
if curcmd=106 then begin if l=0 then goto 30;if curchr=2 then decr(l);
end else if curcmd=105 then incr(l);end;
30:scannerstatus:=savescannerstatus;end;{:494}{497:}
procedure changeiflimit(l:smallnumber;p:halfword);label 10;
var q:halfword;begin if p=condptr then iflimit:=l else begin q:=condptr;
while true do begin if q=0 then confusion(752);
if mem[q].hh.rh=p then begin mem[q].hh.b0:=l;goto 10;end;
q:=mem[q].hh.rh;end;end;10:end;{:497}{498:}procedure conditional;
label 10,50;var b:boolean;r:60..62;m,n:integer;p,q:halfword;
savescannerstatus:smallnumber;savecondptr:halfword;thisif:smallnumber;
begin{495:}begin p:=getnode(2);mem[p].hh.rh:=condptr;
mem[p].hh.b0:=iflimit;mem[p].hh.b1:=curif;mem[p+1].int:=ifline;
condptr:=p;curif:=curchr;iflimit:=1;ifline:=line;end{:495};
savecondptr:=condptr;thisif:=curchr;{501:}case thisif of 0,1:{506:}
begin begin getxtoken;
if curcmd=0 then if curchr=257 then begin curcmd:=13;
curchr:=curtok-4096;end;end;if(curcmd>13)or(curchr>255)then begin m:=0;
n:=256;end else begin m:=curcmd;n:=curchr;end;begin getxtoken;
if curcmd=0 then if curchr=257 then begin curcmd:=13;
curchr:=curtok-4096;end;end;
if(curcmd>13)or(curchr>255)then begin curcmd:=0;curchr:=256;end;
if thisif=0 then b:=(n=curchr)else b:=(m=curcmd);end{:506};2,3:{503:}
begin if thisif=2 then scanint else scandimen(false,false,false);
n:=curval;{406:}repeat getxtoken;until curcmd<>10{:406};
if(curtok>=3132)and(curtok<=3134)then r:=curtok-3072 else begin begin if
interaction=3 then wakeupterminal;printnl(262);print(776);end;
printcmdchr(105,thisif);begin helpptr:=1;helpline[0]:=777;end;backerror;
r:=61;end;if thisif=2 then scanint else scandimen(false,false,false);
case r of 60:b:=(n<curval);61:b:=(n=curval);62:b:=(n>curval);end;
end{:503};4:{504:}begin scanint;b:=odd(curval);end{:504};
5:b:=(abs(curlist.modefield)=1);6:b:=(abs(curlist.modefield)=102);
7:b:=(abs(curlist.modefield)=203);8:b:=(curlist.modefield<0);
9,10,11:{505:}begin scaneightbitint;p:=eqtb[11078+curval].hh.rh;
if thisif=9 then b:=(p=0)else if p=0 then b:=false else if thisif=10
then b:=(mem[p].hh.b0=0)else b:=(mem[p].hh.b0=1);end{:505};12:{507:}
begin savescannerstatus:=scannerstatus;scannerstatus:=0;getnext;
n:=curcs;p:=curcmd;q:=curchr;getnext;
if curcmd<>p then b:=false else if curcmd<111 then b:=(curchr=q)else{508
:}begin p:=mem[curchr].hh.rh;q:=mem[eqtb[n].hh.rh].hh.rh;
if p=q then b:=true else begin while(p<>0)and(q<>0)do if mem[p].hh.lh<>
mem[q].hh.lh then p:=0 else begin p:=mem[p].hh.rh;q:=mem[q].hh.rh;end;
b:=((p=0)and(q=0));end;end{:508};scannerstatus:=savescannerstatus;
end{:507};13:begin scanfourbitint;b:=(readopen[curval]=2);end;
14:b:=true;15:b:=false;16:{509:}begin scanint;n:=curval;
if eqtb[12699].int>1 then begin begindiagnostic;print(778);printint(n);
printchar(125);enddiagnostic(false);end;while n<>0 do begin passtext;
if condptr=savecondptr then if curchr=4 then decr(n)else goto 50 else if
curchr=2 then{496:}begin p:=condptr;ifline:=mem[p+1].int;
curif:=mem[p].hh.b1;iflimit:=mem[p].hh.b0;condptr:=mem[p].hh.rh;
freenode(p,2);end{:496};end;changeiflimit(4,savecondptr);goto 10;
end{:509};end{:501};if eqtb[12699].int>1 then{502:}
begin begindiagnostic;if b then print(774)else print(775);
enddiagnostic(false);end{:502};
if b then begin changeiflimit(3,savecondptr);goto 10;end;{500:}
while true do begin passtext;
if condptr=savecondptr then begin if curchr<>4 then goto 50;
begin if interaction=3 then wakeupterminal;printnl(262);print(772);end;
printesc(770);begin helpptr:=1;helpline[0]:=773;end;error;
end else if curchr=2 then{496:}begin p:=condptr;ifline:=mem[p+1].int;
curif:=mem[p].hh.b1;iflimit:=mem[p].hh.b0;condptr:=mem[p].hh.rh;
freenode(p,2);end{:496};end{:500};50:if curchr=2 then{496:}
begin p:=condptr;ifline:=mem[p+1].int;curif:=mem[p].hh.b1;
iflimit:=mem[p].hh.b0;condptr:=mem[p].hh.rh;freenode(p,2);end{:496}
else iflimit:=2;10:end;{:498}{515:}procedure beginname;
begin areadelimiter:=0;extdelimiter:=0;end;{:515}{516:}
function morename(c:ASCIIcode):boolean;
begin if c=32 then morename:=false else begin begin if poolptr+1>
poolsize then overflow(257,poolsize-initpoolptr);end;
begin strpool[poolptr]:=c;incr(poolptr);end;
if(c=47)then begin areadelimiter:=(poolptr-strstart[strptr]);
extdelimiter:=0;
end else if c=46 then extdelimiter:=(poolptr-strstart[strptr]);
morename:=true;end;end;{:516}{517:}procedure endname;
begin if strptr+3>maxstrings then overflow(258,maxstrings-initstrptr);
if areadelimiter=0 then curarea:=335 else begin curarea:=strptr;
strstart[strptr+1]:=strstart[strptr]+areadelimiter;incr(strptr);end;
if extdelimiter=0 then begin curext:=335;curname:=makestring;
end else begin curname:=strptr;
strstart[strptr+1]:=strstart[strptr]+extdelimiter-areadelimiter-1;
incr(strptr);curext:=makestring;end;end;{:517}{519:}
procedure packfilename(n,a,e:strnumber);var k:integer;c:ASCIIcode;
j:poolpointer;begin k:=0;
for j:=strstart[a]to strstart[a+1]-1 do begin c:=strpool[j];incr(k);
if k<=FILENAMESIZE then nameoffile[k]:=xchr[c];end;
for j:=strstart[n]to strstart[n+1]-1 do begin c:=strpool[j];incr(k);
if k<=FILENAMESIZE then nameoffile[k]:=xchr[c];end;
for j:=strstart[e]to strstart[e+1]-1 do begin c:=strpool[j];incr(k);
if k<=FILENAMESIZE then nameoffile[k]:=xchr[c];end;
if k<=FILENAMESIZE then namelength:=k else namelength:=FILENAMESIZE;
for k:=namelength+1 to FILENAMESIZE do nameoffile[k]:=' ';end;{:519}
{523:}procedure packbufferedname(n:smallnumber;a,b:integer);
var k:integer;c:ASCIIcode;j:integer;
begin if n+b-a+5>FILENAMESIZE then b:=a+FILENAMESIZE-n-5;k:=0;
for j:=1 to n do begin c:=xord[TEXformatdefault[j]];incr(k);
if k<=FILENAMESIZE then nameoffile[k]:=xchr[c];end;
for j:=a to b do begin c:=buffer[j];incr(k);
if k<=FILENAMESIZE then nameoffile[k]:=xchr[c];end;
for j:=6 to 9 do begin c:=xord[TEXformatdefault[j]];incr(k);
if k<=FILENAMESIZE then nameoffile[k]:=xchr[c];end;
if k<=FILENAMESIZE then namelength:=k else namelength:=FILENAMESIZE;
for k:=namelength+1 to FILENAMESIZE do nameoffile[k]:=' ';end;{:523}
{525:}function makenamestring:strnumber;var k,kstart:1..FILENAMESIZE;
begin k:=1;
while(k<FILENAMESIZE)and(xord[realnameoffile[k]]<>32)do incr(k);
namelength:=k-1;
if(poolptr+namelength>poolsize)or(strptr=maxstrings)or((poolptr-strstart
[strptr])>0)then makenamestring:=63 else begin if(xord[realnameoffile[1]
]=46)and(xord[realnameoffile[2]]=47)then kstart:=3 else kstart:=1;
for k:=kstart to namelength do begin strpool[poolptr]:=xord[
realnameoffile[k]];incr(poolptr);end;makenamestring:=makestring;end;end;
{:525}{526:}procedure scanfilename;label 30;begin nameinprogress:=true;
beginname;{406:}repeat getxtoken;until curcmd<>10{:406};
while true do begin if(curcmd>12)or(curchr>255)then begin backinput;
goto 30;end;if not morename(curchr)then goto 30;getxtoken;end;
30:endname;nameinprogress:=false;end;{:526}{529:}
procedure packjobname(s:strnumber);begin curarea:=335;curext:=s;
curname:=jobname;packfilename(curname,curarea,curext);end;{:529}{530:}
procedure promptfilename(s,e:strnumber);label 30;var k:0..bufsize;
begin if interaction=2 then wakeupterminal;
if s=780 then begin if interaction=3 then wakeupterminal;printnl(262);
print(781);end else begin if interaction=3 then wakeupterminal;
printnl(262);print(782);end;printfilename(curname,curarea,curext);
print(783);if e=784 then showcontext;printnl(785);print(s);
if interaction<2 then fatalerror(786);clearterminal;
begin wakeupterminal;print(564);terminput;end;{531:}begin beginname;
k:=first;while(buffer[k]=32)and(k<last)do incr(k);
while true do begin if k=last then goto 30;
if not morename(buffer[k])then goto 30;incr(k);end;30:endname;end{:531};
if curext=335 then curext:=e;packfilename(curname,curarea,curext);end;
{:530}{534:}procedure openlogfile;var oldsetting:0..21;k:0..bufsize;
l:0..bufsize;months:ccharpointer;begin oldsetting:=selector;
if jobname=0 then jobname:=789;packjobname(790);
while not aopenout(logfile)do{535:}begin selector:=17;
promptfilename(792,790);end{:535};logname:=amakenamestring(logfile);
selector:=18;logopened:=true;{536:}
begin write(logfile,'This is TeX, C Version 3.0');print(formatident);
print(793);printint(eqtb[12684].int);printchar(32);
months:=' JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC';
for k:=3*eqtb[12685].int-2 to 3*eqtb[12685].int do write(logfile,months[
k]);printchar(32);printint(eqtb[12686].int);printchar(32);
printtwo(eqtb[12683].int div 60);printchar(58);
printtwo(eqtb[12683].int mod 60);end{:536};
inputstack[inputptr]:=curinput;printnl(791);l:=inputstack[0].limitfield;
if buffer[l]=eqtb[12711].int then decr(l);
for k:=1 to l do print(buffer[k]);println;selector:=oldsetting+2;end;
{:534}{537:}procedure startinput;label 30;begin scanfilename;
if curext=335 then curext:=784;packfilename(curname,curarea,curext);
while true do begin beginfilereading;
if aopenin(inputfile[curinput.indexfield],inputpathspec)then goto 30;
endfilereading;promptfilename(780,784);end;
30:curinput.namefield:=amakenamestring(inputfile[curinput.indexfield]);
if jobname=0 then begin jobname:=curname;openlogfile;end;
if termoffset+(strstart[curinput.namefield+1]-strstart[curinput.
namefield])>maxprintline-3 then println else if(termoffset>0)or(
fileoffset>0)then printchar(32);printchar(40);incr(openparens);
print(curinput.namefield);termflush(stdout);curinput.statefield:=33;
{538:}begin if inputln(inputfile[curinput.indexfield],false)then;
firmuptheline;
if(eqtb[12711].int<0)or(eqtb[12711].int>255)then decr(curinput.
limitfield)else buffer[curinput.limitfield]:=eqtb[12711].int;
first:=curinput.limitfield+1;curinput.locfield:=curinput.startfield;
line:=1;end{:538};end;{:537}{560:}function readfontinfo(u:halfword;
nom,aire:strnumber;s:scaled):internalfontnumber;label 30,11,45;
var k:fontindex;fileopened:boolean;
lf,lh,bc,ec,nw,nh,nd,ni,nl,nk,ne,np:halfword;f:internalfontnumber;
g:internalfontnumber;a,b,c,d:eightbits;qw:fourquarters;sw:scaled;
bchlabel:integer;bchar:0..256;z:scaled;alpha:integer;beta:1..16;
begin g:=0;{562:}{563:}fileopened:=false;packfilename(nom,aire,804);
if not bopenin(tfmfile)then goto 11;fileopened:=true{:563};{565:}
begin begin lf:=tfmtemp;if lf>127 then goto 11;tfmtemp:=getc(tfmfile);
lf:=lf*256+tfmtemp;end;tfmtemp:=getc(tfmfile);begin lh:=tfmtemp;
if lh>127 then goto 11;tfmtemp:=getc(tfmfile);lh:=lh*256+tfmtemp;end;
tfmtemp:=getc(tfmfile);begin bc:=tfmtemp;if bc>127 then goto 11;
tfmtemp:=getc(tfmfile);bc:=bc*256+tfmtemp;end;tfmtemp:=getc(tfmfile);
begin ec:=tfmtemp;if ec>127 then goto 11;tfmtemp:=getc(tfmfile);
ec:=ec*256+tfmtemp;end;if(bc>ec+1)or(ec>255)then goto 11;
if bc>255 then begin bc:=1;ec:=0;end;tfmtemp:=getc(tfmfile);
begin nw:=tfmtemp;if nw>127 then goto 11;tfmtemp:=getc(tfmfile);
nw:=nw*256+tfmtemp;end;tfmtemp:=getc(tfmfile);begin nh:=tfmtemp;
if nh>127 then goto 11;tfmtemp:=getc(tfmfile);nh:=nh*256+tfmtemp;end;
tfmtemp:=getc(tfmfile);begin nd:=tfmtemp;if nd>127 then goto 11;
tfmtemp:=getc(tfmfile);nd:=nd*256+tfmtemp;end;tfmtemp:=getc(tfmfile);
begin ni:=tfmtemp;if ni>127 then goto 11;tfmtemp:=getc(tfmfile);
ni:=ni*256+tfmtemp;end;tfmtemp:=getc(tfmfile);begin nl:=tfmtemp;
if nl>127 then goto 11;tfmtemp:=getc(tfmfile);nl:=nl*256+tfmtemp;end;
tfmtemp:=getc(tfmfile);begin nk:=tfmtemp;if nk>127 then goto 11;
tfmtemp:=getc(tfmfile);nk:=nk*256+tfmtemp;end;tfmtemp:=getc(tfmfile);
begin ne:=tfmtemp;if ne>127 then goto 11;tfmtemp:=getc(tfmfile);
ne:=ne*256+tfmtemp;end;tfmtemp:=getc(tfmfile);begin np:=tfmtemp;
if np>127 then goto 11;tfmtemp:=getc(tfmfile);np:=np*256+tfmtemp;end;
if lf<>6+lh+(ec-bc+1)+nw+nh+nd+ni+nl+nk+ne+np then goto 11;end{:565};
{566:}lf:=lf-6-lh;if np<7 then lf:=lf+7-np;
if(fontptr=fontmax)or(fmemptr+lf>fontmemsize)then{567:}
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(795);end;sprintcs(u);printchar(61);printfilename(nom,aire,335);
if s>=0 then begin print(737);printscaled(s);print(393);
end else if s<>-1000 then begin print(796);printint(-s);end;print(805);
begin helpptr:=4;helpline[3]:=806;helpline[2]:=807;helpline[1]:=808;
helpline[0]:=809;end;error;goto 30;end{:567};f:=fontptr+1;
charbase[f]:=fmemptr-bc;widthbase[f]:=charbase[f]+ec+1;
heightbase[f]:=widthbase[f]+nw;depthbase[f]:=heightbase[f]+nh;
italicbase[f]:=depthbase[f]+nd;ligkernbase[f]:=italicbase[f]+ni;
kernbase[f]:=ligkernbase[f]+nl-256*(128);
extenbase[f]:=kernbase[f]+256*(128)+nk;
parambase[f]:=extenbase[f]+ne{:566};{568:}begin if lh<2 then goto 11;
begin tfmtemp:=getc(tfmfile);a:=tfmtemp;qw.b0:=a;tfmtemp:=getc(tfmfile);
b:=tfmtemp;qw.b1:=b;tfmtemp:=getc(tfmfile);c:=tfmtemp;qw.b2:=c;
tfmtemp:=getc(tfmfile);d:=tfmtemp;qw.b3:=d;fontcheck[f]:=qw;end;
tfmtemp:=getc(tfmfile);begin z:=tfmtemp;if z>127 then goto 11;
tfmtemp:=getc(tfmfile);z:=z*256+tfmtemp;end;tfmtemp:=getc(tfmfile);
z:=z*256+tfmtemp;tfmtemp:=getc(tfmfile);z:=(z*16)+(tfmtemp div 16);
if z<65536 then goto 11;while lh>2 do begin tfmtemp:=getc(tfmfile);
tfmtemp:=getc(tfmfile);tfmtemp:=getc(tfmfile);tfmtemp:=getc(tfmfile);
decr(lh);end;fontdsize[f]:=z;
if s<>-1000 then if s>=0 then z:=s else z:=xnoverd(z,-s,1000);
fontsize[f]:=z;end{:568};{569:}
for k:=fmemptr to widthbase[f]-1 do begin begin tfmtemp:=getc(tfmfile);
a:=tfmtemp;qw.b0:=a;tfmtemp:=getc(tfmfile);b:=tfmtemp;qw.b1:=b;
tfmtemp:=getc(tfmfile);c:=tfmtemp;qw.b2:=c;tfmtemp:=getc(tfmfile);
d:=tfmtemp;qw.b3:=d;fontinfo[k].qqqq:=qw;end;
if(a>=nw)or(b div 16>=nh)or(b mod 16>=nd)or(c div 4>=ni)then goto 11;
case c mod 4 of 1:if d>=nl then goto 11;3:if d>=ne then goto 11;2:{570:}
begin begin if(d<bc)or(d>ec)then goto 11 end;
while d<k+bc-fmemptr do begin qw:=fontinfo[charbase[f]+d].qqqq;
if((qw.b2)mod 4)<>2 then goto 45;d:=qw.b3;end;
if d=k+bc-fmemptr then goto 11;45:end{:570};others:end;end{:569};{571:}
begin{572:}begin alpha:=16;while z>=8388608 do begin z:=z div 2;
alpha:=alpha+alpha;end;beta:=256 div alpha;alpha:=alpha*z;end{:572};
for k:=widthbase[f]to ligkernbase[f]-1 do begin tfmtemp:=getc(tfmfile);
a:=tfmtemp;tfmtemp:=getc(tfmfile);b:=tfmtemp;tfmtemp:=getc(tfmfile);
c:=tfmtemp;tfmtemp:=getc(tfmfile);d:=tfmtemp;
sw:=(((((d*z)div 256)+(c*z))div 256)+(b*z))div beta;
if a=0 then fontinfo[k].int:=sw else if a=255 then fontinfo[k].int:=sw-
alpha else goto 11;end;if fontinfo[widthbase[f]].int<>0 then goto 11;
if fontinfo[heightbase[f]].int<>0 then goto 11;
if fontinfo[depthbase[f]].int<>0 then goto 11;
if fontinfo[italicbase[f]].int<>0 then goto 11;end{:571};{573:}
bchlabel:=32767;bchar:=256;
if nl>0 then begin for k:=ligkernbase[f]to kernbase[f]+256*(128)-1 do
begin begin tfmtemp:=getc(tfmfile);a:=tfmtemp;qw.b0:=a;
tfmtemp:=getc(tfmfile);b:=tfmtemp;qw.b1:=b;tfmtemp:=getc(tfmfile);
c:=tfmtemp;qw.b2:=c;tfmtemp:=getc(tfmfile);d:=tfmtemp;qw.b3:=d;
fontinfo[k].qqqq:=qw;end;
if a>128 then begin if 256*c+d>=nl then goto 11;
if a=255 then if k=ligkernbase[f]then bchar:=b;
end else begin if b<>bchar then begin begin if(b<bc)or(b>ec)then goto 11
end;qw:=fontinfo[charbase[f]+b].qqqq;if not(qw.b0>0)then goto 11;end;
if c<128 then begin begin if(d<bc)or(d>ec)then goto 11 end;
qw:=fontinfo[charbase[f]+d].qqqq;if not(qw.b0>0)then goto 11;
end else if 256*(c-128)+d>=nk then goto 11;
if a<128 then if k-ligkernbase[f]+a+1>=nl then goto 11;end;end;
if a=255 then bchlabel:=256*c+d;end;
for k:=kernbase[f]+256*(128)to extenbase[f]-1 do begin tfmtemp:=getc(
tfmfile);a:=tfmtemp;tfmtemp:=getc(tfmfile);b:=tfmtemp;
tfmtemp:=getc(tfmfile);c:=tfmtemp;tfmtemp:=getc(tfmfile);d:=tfmtemp;
sw:=(((((d*z)div 256)+(c*z))div 256)+(b*z))div beta;
if a=0 then fontinfo[k].int:=sw else if a=255 then fontinfo[k].int:=sw-
alpha else goto 11;end;{:573};{574:}
for k:=extenbase[f]to parambase[f]-1 do begin begin tfmtemp:=getc(
tfmfile);a:=tfmtemp;qw.b0:=a;tfmtemp:=getc(tfmfile);b:=tfmtemp;qw.b1:=b;
tfmtemp:=getc(tfmfile);c:=tfmtemp;qw.b2:=c;tfmtemp:=getc(tfmfile);
d:=tfmtemp;qw.b3:=d;fontinfo[k].qqqq:=qw;end;
if a<>0 then begin begin if(a<bc)or(a>ec)then goto 11 end;
qw:=fontinfo[charbase[f]+a].qqqq;if not(qw.b0>0)then goto 11;end;
if b<>0 then begin begin if(b<bc)or(b>ec)then goto 11 end;
qw:=fontinfo[charbase[f]+b].qqqq;if not(qw.b0>0)then goto 11;end;
if c<>0 then begin begin if(c<bc)or(c>ec)then goto 11 end;
qw:=fontinfo[charbase[f]+c].qqqq;if not(qw.b0>0)then goto 11;end;
begin begin if(d<bc)or(d>ec)then goto 11 end;
qw:=fontinfo[charbase[f]+d].qqqq;if not(qw.b0>0)then goto 11;end;
end{:574};{575:}
begin for k:=1 to np do if k=1 then begin tfmtemp:=getc(tfmfile);
sw:=tfmtemp;if sw>127 then sw:=sw-256;tfmtemp:=getc(tfmfile);
sw:=sw*256+tfmtemp;tfmtemp:=getc(tfmfile);sw:=sw*256+tfmtemp;
tfmtemp:=getc(tfmfile);
fontinfo[parambase[f]].int:=(sw*16)+(tfmtemp div 16);
end else begin tfmtemp:=getc(tfmfile);a:=tfmtemp;tfmtemp:=getc(tfmfile);
b:=tfmtemp;tfmtemp:=getc(tfmfile);c:=tfmtemp;tfmtemp:=getc(tfmfile);
d:=tfmtemp;sw:=(((((d*z)div 256)+(c*z))div 256)+(b*z))div beta;
if a=0 then fontinfo[parambase[f]+k-1].int:=sw else if a=255 then
fontinfo[parambase[f]+k-1].int:=sw-alpha else goto 11;end;
if eof(tfmfile)then goto 11;
for k:=np+1 to 7 do fontinfo[parambase[f]+k-1].int:=0;end{:575};{576:}
if np>=7 then fontparams[f]:=np else fontparams[f]:=7;
hyphenchar[f]:=eqtb[12709].int;skewchar[f]:=eqtb[12710].int;
if bchlabel<nl then bcharlabel[f]:=bchlabel+ligkernbase[f]else
bcharlabel[f]:=fontmemsize;fontbchar[f]:=bchar;fontfalsebchar[f]:=bchar;
if bchar<=ec then if bchar>=bc then begin qw:=fontinfo[charbase[f]+bchar
].qqqq;if(qw.b0>0)then fontfalsebchar[f]:=256;end;fontname[f]:=nom;
fontarea[f]:=aire;fontbc[f]:=bc;fontec[f]:=ec;fontglue[f]:=0;
charbase[f]:=charbase[f];widthbase[f]:=widthbase[f];
ligkernbase[f]:=ligkernbase[f];kernbase[f]:=kernbase[f];
extenbase[f]:=extenbase[f];decr(parambase[f]);fmemptr:=fmemptr+lf;
fontptr:=f;g:=f;goto 30{:576}{:562};11:{561:}
begin if interaction=3 then wakeupterminal;printnl(262);print(795);end;
sprintcs(u);printchar(61);printfilename(nom,aire,335);
if s>=0 then begin print(737);printscaled(s);print(393);
end else if s<>-1000 then begin print(796);printint(-s);end;
if fileopened then print(797)else print(798);begin helpptr:=5;
helpline[4]:=799;helpline[3]:=800;helpline[2]:=801;helpline[1]:=802;
helpline[0]:=803;end;error{:561};30:if fileopened then bclose(tfmfile);
readfontinfo:=g;end;{:560}{581:}
procedure charwarning(f:internalfontnumber;c:eightbits);
begin if eqtb[12698].int>0 then begin begindiagnostic;printnl(818);
print(c);print(819);print(fontname[f]);printchar(33);
enddiagnostic(false);end;end;{:581}{582:}
function newcharacter(f:internalfontnumber;c:eightbits):halfword;
label 10;var p:halfword;
begin if fontbc[f]<=c then if fontec[f]>=c then if(fontinfo[charbase[f]+
c].qqqq.b0>0)then begin p:=getavail;mem[p].hh.b0:=f;mem[p].hh.b1:=c;
newcharacter:=p;goto 10;end;charwarning(f,c);newcharacter:=0;10:end;
{:582}{598:}procedure dviswap;
begin if dvilimit=dvibufsize then begin writedvi(0,halfbuf-1);
dvilimit:=halfbuf;dvioffset:=dvioffset+dvibufsize;dviptr:=0;
end else begin writedvi(halfbuf,dvibufsize-1);dvilimit:=dvibufsize;end;
dvigone:=dvigone+halfbuf;end;{:598}{600:}procedure dvifour(x:integer);
begin if x>=0 then begin dvibuf[dviptr]:=x div 16777216;incr(dviptr);
if dviptr=dvilimit then dviswap;end else begin x:=x+1073741824;
x:=x+1073741824;begin dvibuf[dviptr]:=(x div 16777216)+128;incr(dviptr);
if dviptr=dvilimit then dviswap;end;end;x:=x mod 16777216;
begin dvibuf[dviptr]:=x div 65536;incr(dviptr);
if dviptr=dvilimit then dviswap;end;x:=x mod 65536;
begin dvibuf[dviptr]:=x div 256;incr(dviptr);
if dviptr=dvilimit then dviswap;end;begin dvibuf[dviptr]:=x mod 256;
incr(dviptr);if dviptr=dvilimit then dviswap;end;end;{:600}{601:}
procedure dvipop(l:integer);
begin if(l=dvioffset+dviptr)and(dviptr>0)then decr(dviptr)else begin
dvibuf[dviptr]:=142;incr(dviptr);if dviptr=dvilimit then dviswap;end;
end;{:601}{602:}procedure dvifontdef(f:internalfontnumber);
var k:poolpointer;begin begin dvibuf[dviptr]:=243;incr(dviptr);
if dviptr=dvilimit then dviswap;end;begin dvibuf[dviptr]:=f-1;
incr(dviptr);if dviptr=dvilimit then dviswap;end;
begin dvibuf[dviptr]:=fontcheck[f].b0;incr(dviptr);
if dviptr=dvilimit then dviswap;end;
begin dvibuf[dviptr]:=fontcheck[f].b1;incr(dviptr);
if dviptr=dvilimit then dviswap;end;
begin dvibuf[dviptr]:=fontcheck[f].b2;incr(dviptr);
if dviptr=dvilimit then dviswap;end;
begin dvibuf[dviptr]:=fontcheck[f].b3;incr(dviptr);
if dviptr=dvilimit then dviswap;end;dvifour(fontsize[f]);
dvifour(fontdsize[f]);
begin dvibuf[dviptr]:=(strstart[fontarea[f]+1]-strstart[fontarea[f]]);
incr(dviptr);if dviptr=dvilimit then dviswap;end;
begin dvibuf[dviptr]:=(strstart[fontname[f]+1]-strstart[fontname[f]]);
incr(dviptr);if dviptr=dvilimit then dviswap;end;{603:}
for k:=strstart[fontarea[f]]to strstart[fontarea[f]+1]-1 do begin dvibuf
[dviptr]:=strpool[k];incr(dviptr);if dviptr=dvilimit then dviswap;end;
for k:=strstart[fontname[f]]to strstart[fontname[f]+1]-1 do begin dvibuf
[dviptr]:=strpool[k];incr(dviptr);if dviptr=dvilimit then dviswap;
end{:603};end;{:602}{607:}procedure movement(w:scaled;o:eightbits);
label 10,40,45,2,1;var mstate:smallnumber;p,q:halfword;k:integer;
begin q:=getnode(3);mem[q+1].int:=w;mem[q+2].int:=dvioffset+dviptr;
if o=157 then begin mem[q].hh.rh:=downptr;downptr:=q;
end else begin mem[q].hh.rh:=rightptr;rightptr:=q;end;{611:}
p:=mem[q].hh.rh;mstate:=0;
while p<>0 do begin if mem[p+1].int=w then{612:}
case mstate+mem[p].hh.lh of 3,4,15,16:if mem[p+2].int<dvigone then goto
45 else{613:}begin k:=mem[p+2].int-dvioffset;
if k<0 then k:=k+dvibufsize;dvibuf[k]:=dvibuf[k]+5;mem[p].hh.lh:=1;
goto 40;end{:613};5,9,11:if mem[p+2].int<dvigone then goto 45 else{614:}
begin k:=mem[p+2].int-dvioffset;if k<0 then k:=k+dvibufsize;
dvibuf[k]:=dvibuf[k]+10;mem[p].hh.lh:=2;goto 40;end{:614};
1,2,8,13:goto 40;others:end{:612}
else case mstate+mem[p].hh.lh of 1:mstate:=6;2:mstate:=12;8,13:goto 45;
others:end;p:=mem[p].hh.rh;end;45:{:611};{610:}mem[q].hh.lh:=3;
if abs(w)>=8388608 then begin begin dvibuf[dviptr]:=o+3;incr(dviptr);
if dviptr=dvilimit then dviswap;end;dvifour(w);goto 10;end;
if abs(w)>=32768 then begin begin dvibuf[dviptr]:=o+2;incr(dviptr);
if dviptr=dvilimit then dviswap;end;if w<0 then w:=w+16777216;
begin dvibuf[dviptr]:=w div 65536;incr(dviptr);
if dviptr=dvilimit then dviswap;end;w:=w mod 65536;goto 2;end;
if abs(w)>=128 then begin begin dvibuf[dviptr]:=o+1;incr(dviptr);
if dviptr=dvilimit then dviswap;end;if w<0 then w:=w+65536;goto 2;end;
begin dvibuf[dviptr]:=o;incr(dviptr);if dviptr=dvilimit then dviswap;
end;if w<0 then w:=w+256;goto 1;2:begin dvibuf[dviptr]:=w div 256;
incr(dviptr);if dviptr=dvilimit then dviswap;end;
1:begin dvibuf[dviptr]:=w mod 256;incr(dviptr);
if dviptr=dvilimit then dviswap;end;goto 10{:610};40:{609:}
mem[q].hh.lh:=mem[p].hh.lh;
if mem[q].hh.lh=1 then begin begin dvibuf[dviptr]:=o+4;incr(dviptr);
if dviptr=dvilimit then dviswap;end;
while mem[q].hh.rh<>p do begin q:=mem[q].hh.rh;
case mem[q].hh.lh of 3:mem[q].hh.lh:=5;4:mem[q].hh.lh:=6;others:end;end;
end else begin begin dvibuf[dviptr]:=o+9;incr(dviptr);
if dviptr=dvilimit then dviswap;end;
while mem[q].hh.rh<>p do begin q:=mem[q].hh.rh;
case mem[q].hh.lh of 3:mem[q].hh.lh:=4;5:mem[q].hh.lh:=6;others:end;end;
end{:609};10:end;{:607}{615:}procedure prunemovements(l:integer);
label 30,10;var p:halfword;
begin while downptr<>0 do begin if mem[downptr+2].int<l then goto 30;
p:=downptr;downptr:=mem[p].hh.rh;freenode(p,3);end;
30:while rightptr<>0 do begin if mem[rightptr+2].int<l then goto 10;
p:=rightptr;rightptr:=mem[p].hh.rh;freenode(p,3);end;10:end;{:615}{618:}
procedure vlistout;forward;{:618}{619:}{1368:}
procedure specialout(p:halfword);var oldsetting:0..21;k:poolpointer;
begin if curh<>dvih then begin movement(curh-dvih,143);dvih:=curh;end;
if curv<>dviv then begin movement(curv-dviv,157);dviv:=curv;end;
oldsetting:=selector;selector:=21;
showtokenlist(mem[mem[p+1].hh.rh].hh.rh,0,poolsize-poolptr);
selector:=oldsetting;
begin if poolptr+1>poolsize then overflow(257,poolsize-initpoolptr);end;
if(poolptr-strstart[strptr])<256 then begin begin dvibuf[dviptr]:=239;
incr(dviptr);if dviptr=dvilimit then dviswap;end;
begin dvibuf[dviptr]:=(poolptr-strstart[strptr]);incr(dviptr);
if dviptr=dvilimit then dviswap;end;
end else begin begin dvibuf[dviptr]:=242;incr(dviptr);
if dviptr=dvilimit then dviswap;end;dvifour((poolptr-strstart[strptr]));
end;
for k:=strstart[strptr]to poolptr-1 do begin dvibuf[dviptr]:=strpool[k];
incr(dviptr);if dviptr=dvilimit then dviswap;end;
poolptr:=strstart[strptr];end;{:1368}{1370:}
procedure writeout(p:halfword);var oldsetting:0..21;oldmode:integer;
j:smallnumber;q,r:halfword;begin{1371:}q:=getavail;mem[q].hh.lh:=637;
r:=getavail;mem[q].hh.rh:=r;mem[r].hh.lh:=14117;begintokenlist(q,4);
begintokenlist(mem[p+1].hh.rh,15);q:=getavail;mem[q].hh.lh:=379;
begintokenlist(q,4);oldmode:=curlist.modefield;curlist.modefield:=0;
curcs:=writeloc;q:=scantoks(false,true);gettoken;
if curtok<>14117 then{1372:}
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(1287);end;begin helpptr:=2;helpline[1]:=1288;helpline[0]:=1005;
end;error;repeat gettoken;until curtok=14117;end{:1372};
curlist.modefield:=oldmode;endtokenlist{:1371};oldsetting:=selector;
j:=mem[p+1].hh.lh;
if writeopen[j]then selector:=j else begin if(j=17)and(selector=19)then
selector:=18;printnl(335);end;tokenshow(defref);println;
flushlist(defref);selector:=oldsetting;end;{:1370}{1373:}
procedure outwhat(p:halfword);var j:smallnumber;
begin case mem[p].hh.b1 of 0,1,2:{1374:}
if not doingleaders then begin j:=mem[p+1].hh.lh;
if mem[p].hh.b1=1 then writeout(p)else begin if writeopen[j]then aclose(
writefile[j]);
if mem[p].hh.b1=2 then writeopen[j]:=false else if j<16 then begin
curname:=mem[p+1].hh.rh;curarea:=mem[p+2].hh.lh;curext:=mem[p+2].hh.rh;
if curext=335 then curext:=784;packfilename(curname,curarea,curext);
while not aopenout(writefile[j])do promptfilename(1290,784);
writeopen[j]:=true;end;end;end{:1374};3:specialout(p);4:;
others:confusion(1289)end;end;{:1373}procedure hlistout;
label 21,13,14,15;var baseline:scaled;leftedge:scaled;
saveh,savev:scaled;thisbox:halfword;gorder:glueord;gsign:0..2;
p:halfword;saveloc:integer;leaderbox:halfword;leaderwd:scaled;lx:scaled;
outerdoingleaders:boolean;edge:scaled;begin thisbox:=tempptr;
gorder:=mem[thisbox+5].hh.b1;gsign:=mem[thisbox+5].hh.b0;
p:=mem[thisbox+5].hh.rh;incr(curs);
if curs>0 then begin dvibuf[dviptr]:=141;incr(dviptr);
if dviptr=dvilimit then dviswap;end;if curs>maxpush then maxpush:=curs;
saveloc:=dvioffset+dviptr;baseline:=curv;leftedge:=curh;
while p<>0 do{620:}
21:if(p>=himemmin)then begin if curh<>dvih then begin movement(curh-dvih
,143);dvih:=curh;end;if curv<>dviv then begin movement(curv-dviv,157);
dviv:=curv;end;repeat f:=mem[p].hh.b0;c:=mem[p].hh.b1;
if f<>dvif then{621:}begin if not fontused[f]then begin dvifontdef(f);
fontused[f]:=true;end;if f<=64 then begin dvibuf[dviptr]:=f+170;
incr(dviptr);if dviptr=dvilimit then dviswap;
end else begin begin dvibuf[dviptr]:=235;incr(dviptr);
if dviptr=dvilimit then dviswap;end;begin dvibuf[dviptr]:=f-1;
incr(dviptr);if dviptr=dvilimit then dviswap;end;end;dvif:=f;end{:621};
if c>=128 then begin dvibuf[dviptr]:=128;incr(dviptr);
if dviptr=dvilimit then dviswap;end;begin dvibuf[dviptr]:=c;
incr(dviptr);if dviptr=dvilimit then dviswap;end;
curh:=curh+fontinfo[widthbase[f]+fontinfo[charbase[f]+c].qqqq.b0].int;
p:=mem[p].hh.rh;until not(p>=himemmin);dvih:=curh;end else{622:}
begin case mem[p].hh.b0 of 0,1:{623:}
if mem[p+5].hh.rh=0 then curh:=curh+mem[p+1].int else begin saveh:=dvih;
savev:=dviv;curv:=baseline+mem[p+4].int;tempptr:=p;edge:=curh;
if mem[p].hh.b0=1 then vlistout else hlistout;dvih:=saveh;dviv:=savev;
curh:=edge+mem[p+1].int;curv:=baseline;end{:623};
2:begin ruleht:=mem[p+3].int;ruledp:=mem[p+2].int;rulewd:=mem[p+1].int;
goto 14;end;8:{1367:}outwhat(p){:1367};10:{625:}begin g:=mem[p+1].hh.lh;
rulewd:=mem[g+1].int;
if gsign<>0 then begin if gsign=1 then begin if mem[g].hh.b0=gorder then
rulewd:=rulewd+round(mem[thisbox+6].gr*mem[g+2].int);
end else begin if mem[g].hh.b1=gorder then rulewd:=rulewd-round(mem[
thisbox+6].gr*mem[g+3].int);end;end;if mem[p].hh.b1>=100 then{626:}
begin leaderbox:=mem[p+1].hh.rh;
if mem[leaderbox].hh.b0=2 then begin ruleht:=mem[leaderbox+3].int;
ruledp:=mem[leaderbox+2].int;goto 14;end;leaderwd:=mem[leaderbox+1].int;
if(leaderwd>0)and(rulewd>0)then begin rulewd:=rulewd+10;
edge:=curh+rulewd;lx:=0;{627:}
if mem[p].hh.b1=100 then begin saveh:=curh;
curh:=leftedge+leaderwd*((curh-leftedge)div leaderwd);
if curh<saveh then curh:=curh+leaderwd;
end else begin lq:=rulewd div leaderwd;lr:=rulewd mod leaderwd;
if mem[p].hh.b1=101 then curh:=curh+(lr div 2)else begin lx:=(2*lr+lq+1)
div(2*lq+2);curh:=curh+((lr-(lq-1)*lx)div 2);end;end{:627};
while curh+leaderwd<=edge do{628:}
begin curv:=baseline+mem[leaderbox+4].int;
if curv<>dviv then begin movement(curv-dviv,157);dviv:=curv;end;
savev:=dviv;if curh<>dvih then begin movement(curh-dvih,143);dvih:=curh;
end;saveh:=dvih;tempptr:=leaderbox;outerdoingleaders:=doingleaders;
doingleaders:=true;
if mem[leaderbox].hh.b0=1 then vlistout else hlistout;
doingleaders:=outerdoingleaders;dviv:=savev;dvih:=saveh;curv:=savev;
curh:=saveh+leaderwd+lx;end{:628};curh:=edge-10;goto 15;end;end{:626};
goto 13;end{:625};11,9:curh:=curh+mem[p+1].int;6:{652:}
begin mem[memtop-12]:=mem[p+1];mem[memtop-12].hh.rh:=mem[p].hh.rh;
p:=memtop-12;goto 21;end{:652};others:end;goto 15;14:{624:}
if(ruleht=-1073741824)then ruleht:=mem[thisbox+3].int;
if(ruledp=-1073741824)then ruledp:=mem[thisbox+2].int;
ruleht:=ruleht+ruledp;
if(ruleht>0)and(rulewd>0)then begin if curh<>dvih then begin movement(
curh-dvih,143);dvih:=curh;end;curv:=baseline+ruledp;
if curv<>dviv then begin movement(curv-dviv,157);dviv:=curv;end;
begin dvibuf[dviptr]:=132;incr(dviptr);if dviptr=dvilimit then dviswap;
end;dvifour(ruleht);dvifour(rulewd);curv:=baseline;dvih:=dvih+rulewd;
end{:624};13:curh:=curh+rulewd;15:p:=mem[p].hh.rh;end{:622}{:620};
prunemovements(saveloc);if curs>0 then dvipop(saveloc);decr(curs);end;
{:619}{629:}procedure vlistout;label 13,14,15;var leftedge:scaled;
topedge:scaled;saveh,savev:scaled;thisbox:halfword;gorder:glueord;
gsign:0..2;p:halfword;saveloc:integer;leaderbox:halfword;
leaderht:scaled;lx:scaled;outerdoingleaders:boolean;edge:scaled;
begin thisbox:=tempptr;gorder:=mem[thisbox+5].hh.b1;
gsign:=mem[thisbox+5].hh.b0;p:=mem[thisbox+5].hh.rh;incr(curs);
if curs>0 then begin dvibuf[dviptr]:=141;incr(dviptr);
if dviptr=dvilimit then dviswap;end;if curs>maxpush then maxpush:=curs;
saveloc:=dvioffset+dviptr;leftedge:=curh;curv:=curv-mem[thisbox+3].int;
topedge:=curv;while p<>0 do{630:}
begin if(p>=himemmin)then confusion(821)else{631:}
begin case mem[p].hh.b0 of 0,1:{632:}
if mem[p+5].hh.rh=0 then curv:=curv+mem[p+3].int+mem[p+2].int else begin
curv:=curv+mem[p+3].int;
if curv<>dviv then begin movement(curv-dviv,157);dviv:=curv;end;
saveh:=dvih;savev:=dviv;curh:=leftedge+mem[p+4].int;tempptr:=p;
if mem[p].hh.b0=1 then vlistout else hlistout;dvih:=saveh;dviv:=savev;
curv:=savev+mem[p+2].int;curh:=leftedge;end{:632};
2:begin ruleht:=mem[p+3].int;ruledp:=mem[p+2].int;rulewd:=mem[p+1].int;
goto 14;end;8:{1366:}outwhat(p){:1366};10:{634:}begin g:=mem[p+1].hh.lh;
ruleht:=mem[g+1].int;
if gsign<>0 then begin if gsign=1 then begin if mem[g].hh.b0=gorder then
ruleht:=ruleht+round(mem[thisbox+6].gr*mem[g+2].int);
end else begin if mem[g].hh.b1=gorder then ruleht:=ruleht-round(mem[
thisbox+6].gr*mem[g+3].int);end;end;if mem[p].hh.b1>=100 then{635:}
begin leaderbox:=mem[p+1].hh.rh;
if mem[leaderbox].hh.b0=2 then begin rulewd:=mem[leaderbox+1].int;
ruledp:=0;goto 14;end;
leaderht:=mem[leaderbox+3].int+mem[leaderbox+2].int;
if(leaderht>0)and(ruleht>0)then begin ruleht:=ruleht+10;
edge:=curv+ruleht;lx:=0;{636:}
if mem[p].hh.b1=100 then begin savev:=curv;
curv:=topedge+leaderht*((curv-topedge)div leaderht);
if curv<savev then curv:=curv+leaderht;
end else begin lq:=ruleht div leaderht;lr:=ruleht mod leaderht;
if mem[p].hh.b1=101 then curv:=curv+(lr div 2)else begin lx:=(2*lr+lq+1)
div(2*lq+2);curv:=curv+((lr-(lq-1)*lx)div 2);end;end{:636};
while curv+leaderht<=edge do{637:}
begin curh:=leftedge+mem[leaderbox+4].int;
if curh<>dvih then begin movement(curh-dvih,143);dvih:=curh;end;
saveh:=dvih;curv:=curv+mem[leaderbox+3].int;
if curv<>dviv then begin movement(curv-dviv,157);dviv:=curv;end;
savev:=dviv;tempptr:=leaderbox;outerdoingleaders:=doingleaders;
doingleaders:=true;
if mem[leaderbox].hh.b0=1 then vlistout else hlistout;
doingleaders:=outerdoingleaders;dviv:=savev;dvih:=saveh;curh:=saveh;
curv:=savev-mem[leaderbox+3].int+leaderht+lx;end{:637};curv:=edge-10;
goto 15;end;end{:635};goto 13;end{:634};11:curv:=curv+mem[p+1].int;
others:end;goto 15;14:{633:}
if(rulewd=-1073741824)then rulewd:=mem[thisbox+1].int;
ruleht:=ruleht+ruledp;curv:=curv+ruleht;
if(ruleht>0)and(rulewd>0)then begin if curh<>dvih then begin movement(
curh-dvih,143);dvih:=curh;end;
if curv<>dviv then begin movement(curv-dviv,157);dviv:=curv;end;
begin dvibuf[dviptr]:=137;incr(dviptr);if dviptr=dvilimit then dviswap;
end;dvifour(ruleht);dvifour(rulewd);end;goto 15{:633};
13:curv:=curv+ruleht;end{:631};15:p:=mem[p].hh.rh;end{:630};
prunemovements(saveloc);if curs>0 then dvipop(saveloc);decr(curs);end;
{:629}{638:}procedure shipout(p:halfword);label 30;var pageloc:integer;
j,k:0..9;s:poolpointer;oldsetting:0..21;
begin if eqtb[12697].int>0 then begin printnl(335);println;print(822);
end;if termoffset>maxprintline-9 then println else if(termoffset>0)or(
fileoffset>0)then printchar(32);printchar(91);j:=9;
while(eqtb[12718+j].int=0)and(j>0)do decr(j);
for k:=0 to j do begin printint(eqtb[12718+k].int);
if k<j then printchar(46);end;termflush(stdout);
if eqtb[12697].int>0 then begin printchar(93);begindiagnostic;
showbox(p);enddiagnostic(true);end;{640:}{641:}
if(mem[p+3].int>1073741823)or(mem[p+2].int>1073741823)or(mem[p+3].int+
mem[p+2].int+eqtb[13249].int>1073741823)or(mem[p+1].int+eqtb[13248].int>
1073741823)then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(826);end;begin helpptr:=2;helpline[1]:=827;
helpline[0]:=828;end;error;
if eqtb[12697].int<=0 then begin begindiagnostic;printnl(829);
showbox(p);enddiagnostic(true);end;goto 30;end;
if mem[p+3].int+mem[p+2].int+eqtb[13249].int>maxv then maxv:=mem[p+3].
int+mem[p+2].int+eqtb[13249].int;
if mem[p+1].int+eqtb[13248].int>maxh then maxh:=mem[p+1].int+eqtb[13248]
.int{:641};{617:}dvih:=0;dviv:=0;curh:=eqtb[13248].int;dvif:=0;
if outputfilename=0 then begin if jobname=0 then openlogfile;
packjobname(787);while not bopenout(dvifile)do promptfilename(788,787);
outputfilename:=bmakenamestring(dvifile);end;
if totalpages=0 then begin begin dvibuf[dviptr]:=247;incr(dviptr);
if dviptr=dvilimit then dviswap;end;begin dvibuf[dviptr]:=2;
incr(dviptr);if dviptr=dvilimit then dviswap;end;dvifour(25400000);
dvifour(473628672);preparemag;dvifour(eqtb[12680].int);
oldsetting:=selector;selector:=21;print(820);printint(eqtb[12686].int);
printchar(46);printtwo(eqtb[12685].int);printchar(46);
printtwo(eqtb[12684].int);printchar(58);
printtwo(eqtb[12683].int div 60);printtwo(eqtb[12683].int mod 60);
selector:=oldsetting;begin dvibuf[dviptr]:=(poolptr-strstart[strptr]);
incr(dviptr);if dviptr=dvilimit then dviswap;end;
for s:=strstart[strptr]to poolptr-1 do begin dvibuf[dviptr]:=strpool[s];
incr(dviptr);if dviptr=dvilimit then dviswap;end;
poolptr:=strstart[strptr];end{:617};pageloc:=dvioffset+dviptr;
begin dvibuf[dviptr]:=139;incr(dviptr);if dviptr=dvilimit then dviswap;
end;for k:=0 to 9 do dvifour(eqtb[12718+k].int);dvifour(lastbop);
lastbop:=pageloc;curv:=mem[p+3].int+eqtb[13249].int;tempptr:=p;
if mem[p].hh.b0=1 then vlistout else hlistout;begin dvibuf[dviptr]:=140;
incr(dviptr);if dviptr=dvilimit then dviswap;end;incr(totalpages);
curs:=-1;30:{:640};if eqtb[12697].int<=0 then printchar(93);
deadcycles:=0;termflush(stdout);{639:}
ifdef('STAT')if eqtb[12694].int>1 then begin printnl(823);
printint(varused);printchar(38);printint(dynused);printchar(59);end;
endif('STAT')flushnodelist(p);
ifdef('STAT')if eqtb[12694].int>1 then begin print(824);
printint(varused);printchar(38);printint(dynused);print(825);
printint(himemmin-lomemmax-1);println;end;endif('STAT'){:639};end;{:638}
{645:}procedure scanspec(c:groupcode;threecodes:boolean);label 40;
var s:integer;speccode:0..1;
begin if threecodes then s:=savestack[saveptr+0].int;
if scankeyword(835)then speccode:=0 else if scankeyword(836)then
speccode:=1 else begin speccode:=1;curval:=0;goto 40;end;
scandimen(false,false,false);
40:if threecodes then begin savestack[saveptr+0].int:=s;incr(saveptr);
end;savestack[saveptr+0].int:=speccode;savestack[saveptr+1].int:=curval;
saveptr:=saveptr+2;newsavelevel(c);scanleftbrace;end;{:645}{649:}
function hpack(p:halfword;w:scaled;m:smallnumber):halfword;
label 21,50,10;var r:halfword;q:halfword;h,d,x:scaled;s:scaled;
g:halfword;o:glueord;f:internalfontnumber;i:fourquarters;hd:eightbits;
begin lastbadness:=0;r:=getnode(7);mem[r].hh.b0:=0;mem[r].hh.b1:=0;
mem[r+4].int:=0;q:=r+5;mem[q].hh.rh:=p;h:=0;{650:}d:=0;x:=0;
totalstretch[0]:=0;totalshrink[0]:=0;totalstretch[1]:=0;
totalshrink[1]:=0;totalstretch[2]:=0;totalshrink[2]:=0;
totalstretch[3]:=0;totalshrink[3]:=0{:650};while p<>0 do{651:}
begin 21:while(p>=himemmin)do{654:}begin f:=mem[p].hh.b0;
i:=fontinfo[charbase[f]+mem[p].hh.b1].qqqq;hd:=i.b1;
x:=x+fontinfo[widthbase[f]+i.b0].int;
s:=fontinfo[heightbase[f]+(hd)div 16].int;if s>h then h:=s;
s:=fontinfo[depthbase[f]+(hd)mod 16].int;if s>d then d:=s;
p:=mem[p].hh.rh;end{:654};
if p<>0 then begin case mem[p].hh.b0 of 0,1,2,13:{653:}
begin x:=x+mem[p+1].int;
if mem[p].hh.b0>=2 then s:=0 else s:=mem[p+4].int;
if mem[p+3].int-s>h then h:=mem[p+3].int-s;
if mem[p+2].int+s>d then d:=mem[p+2].int+s;end{:653};
3,4,5:if adjusttail<>0 then{655:}
begin while mem[q].hh.rh<>p do q:=mem[q].hh.rh;
if mem[p].hh.b0=5 then begin mem[adjusttail].hh.rh:=mem[p+1].int;
while mem[adjusttail].hh.rh<>0 do adjusttail:=mem[adjusttail].hh.rh;
p:=mem[p].hh.rh;freenode(mem[q].hh.rh,2);
end else begin mem[adjusttail].hh.rh:=p;adjusttail:=p;p:=mem[p].hh.rh;
end;mem[q].hh.rh:=p;p:=q;end{:655};8:{1360:}{:1360};10:{656:}
begin g:=mem[p+1].hh.lh;x:=x+mem[g+1].int;o:=mem[g].hh.b0;
totalstretch[o]:=totalstretch[o]+mem[g+2].int;o:=mem[g].hh.b1;
totalshrink[o]:=totalshrink[o]+mem[g+3].int;
if mem[p].hh.b1>=100 then begin g:=mem[p+1].hh.rh;
if mem[g+3].int>h then h:=mem[g+3].int;
if mem[g+2].int>d then d:=mem[g+2].int;end;end{:656};
11,9:x:=x+mem[p+1].int;6:{652:}begin mem[memtop-12]:=mem[p+1];
mem[memtop-12].hh.rh:=mem[p].hh.rh;p:=memtop-12;goto 21;end{:652};
others:end;p:=mem[p].hh.rh;end;end{:651};
if adjusttail<>0 then mem[adjusttail].hh.rh:=0;mem[r+3].int:=h;
mem[r+2].int:=d;{657:}if m=1 then w:=x+w;mem[r+1].int:=w;x:=w-x;
if x=0 then begin mem[r+5].hh.b0:=0;mem[r+5].hh.b1:=0;mem[r+6].gr:=0.0;
goto 10;end else if x>0 then{658:}begin{659:}
if totalstretch[3]<>0 then o:=3 else if totalstretch[2]<>0 then o:=2
else if totalstretch[1]<>0 then o:=1 else o:=0{:659};mem[r+5].hh.b1:=o;
mem[r+5].hh.b0:=1;
if totalstretch[o]<>0 then mem[r+6].gr:=x/totalstretch[o]else begin mem[
r+5].hh.b0:=0;mem[r+6].gr:=0.0;end;
if o=0 then if mem[r+5].hh.rh<>0 then{660:}
begin lastbadness:=badness(x,totalstretch[0]);
if lastbadness>eqtb[12689].int then begin println;
if lastbadness>100 then printnl(837)else printnl(838);print(839);
printint(lastbadness);goto 50;end;end{:660};goto 10;end{:658}else{664:}
begin{665:}
if totalshrink[3]<>0 then o:=3 else if totalshrink[2]<>0 then o:=2 else
if totalshrink[1]<>0 then o:=1 else o:=0{:665};mem[r+5].hh.b1:=o;
mem[r+5].hh.b0:=2;
if totalshrink[o]<>0 then mem[r+6].gr:=(-x)/totalshrink[o]else begin mem
[r+5].hh.b0:=0;mem[r+6].gr:=0.0;end;
if(totalshrink[o]<-x)and(o=0)and(mem[r+5].hh.rh<>0)then begin
lastbadness:=1000000;mem[r+6].gr:=1.0;{666:}
if(-x-totalshrink[0]>eqtb[13238].int)or(eqtb[12689].int<100)then begin
if(eqtb[13246].int>0)and(-x-totalshrink[0]>eqtb[13238].int)then begin
while mem[q].hh.rh<>0 do q:=mem[q].hh.rh;mem[q].hh.rh:=newrule;
mem[mem[q].hh.rh+1].int:=eqtb[13246].int;end;println;printnl(845);
printscaled(-x-totalshrink[0]);print(846);goto 50;end{:666};
end else if o=0 then if mem[r+5].hh.rh<>0 then{667:}
begin lastbadness:=badness(-x,totalshrink[0]);
if lastbadness>eqtb[12689].int then begin println;printnl(847);
printint(lastbadness);goto 50;end;end{:667};goto 10;end{:664}{:657};
50:{663:}
if outputactive then print(840)else begin if packbeginline<>0 then begin
if packbeginline>0 then print(841)else print(842);
printint(abs(packbeginline));print(843);end else print(844);
printint(line);end;println;fontinshortdisplay:=0;
shortdisplay(mem[r+5].hh.rh);println;begindiagnostic;showbox(r);
enddiagnostic(true){:663};10:hpack:=r;end;{:649}{668:}
function vpackage(p:halfword;h:scaled;m:smallnumber;l:scaled):halfword;
label 50,10;var r:halfword;w,d,x:scaled;s:scaled;g:halfword;o:glueord;
begin lastbadness:=0;r:=getnode(7);mem[r].hh.b0:=1;mem[r].hh.b1:=0;
mem[r+4].int:=0;mem[r+5].hh.rh:=p;w:=0;{650:}d:=0;x:=0;
totalstretch[0]:=0;totalshrink[0]:=0;totalstretch[1]:=0;
totalshrink[1]:=0;totalstretch[2]:=0;totalshrink[2]:=0;
totalstretch[3]:=0;totalshrink[3]:=0{:650};while p<>0 do{669:}
begin if(p>=himemmin)then confusion(848)else case mem[p].hh.b0 of 0,1,2,
13:{670:}begin x:=x+d+mem[p+3].int;d:=mem[p+2].int;
if mem[p].hh.b0>=2 then s:=0 else s:=mem[p+4].int;
if mem[p+1].int+s>w then w:=mem[p+1].int+s;end{:670};8:{1359:}{:1359};
10:{671:}begin x:=x+d;d:=0;g:=mem[p+1].hh.lh;x:=x+mem[g+1].int;
o:=mem[g].hh.b0;totalstretch[o]:=totalstretch[o]+mem[g+2].int;
o:=mem[g].hh.b1;totalshrink[o]:=totalshrink[o]+mem[g+3].int;
if mem[p].hh.b1>=100 then begin g:=mem[p+1].hh.rh;
if mem[g+1].int>w then w:=mem[g+1].int;end;end{:671};
11:begin x:=x+d+mem[p+1].int;d:=0;end;others:end;p:=mem[p].hh.rh;
end{:669};mem[r+1].int:=w;if d>l then begin x:=x+d-l;mem[r+2].int:=l;
end else mem[r+2].int:=d;{672:}if m=1 then h:=x+h;mem[r+3].int:=h;
x:=h-x;if x=0 then begin mem[r+5].hh.b0:=0;mem[r+5].hh.b1:=0;
mem[r+6].gr:=0.0;goto 10;end else if x>0 then{673:}begin{659:}
if totalstretch[3]<>0 then o:=3 else if totalstretch[2]<>0 then o:=2
else if totalstretch[1]<>0 then o:=1 else o:=0{:659};mem[r+5].hh.b1:=o;
mem[r+5].hh.b0:=1;
if totalstretch[o]<>0 then mem[r+6].gr:=x/totalstretch[o]else begin mem[
r+5].hh.b0:=0;mem[r+6].gr:=0.0;end;
if o=0 then if mem[r+5].hh.rh<>0 then{674:}
begin lastbadness:=badness(x,totalstretch[0]);
if lastbadness>eqtb[12690].int then begin println;
if lastbadness>100 then printnl(837)else printnl(838);print(849);
printint(lastbadness);goto 50;end;end{:674};goto 10;end{:673}else{676:}
begin{665:}
if totalshrink[3]<>0 then o:=3 else if totalshrink[2]<>0 then o:=2 else
if totalshrink[1]<>0 then o:=1 else o:=0{:665};mem[r+5].hh.b1:=o;
mem[r+5].hh.b0:=2;
if totalshrink[o]<>0 then mem[r+6].gr:=(-x)/totalshrink[o]else begin mem
[r+5].hh.b0:=0;mem[r+6].gr:=0.0;end;
if(totalshrink[o]<-x)and(o=0)and(mem[r+5].hh.rh<>0)then begin
lastbadness:=1000000;mem[r+6].gr:=1.0;{677:}
if(-x-totalshrink[0]>eqtb[13239].int)or(eqtb[12690].int<100)then begin
println;printnl(850);printscaled(-x-totalshrink[0]);print(851);goto 50;
end{:677};end else if o=0 then if mem[r+5].hh.rh<>0 then{678:}
begin lastbadness:=badness(-x,totalshrink[0]);
if lastbadness>eqtb[12690].int then begin println;printnl(852);
printint(lastbadness);goto 50;end;end{:678};goto 10;end{:676}{:672};
50:{675:}
if outputactive then print(840)else begin if packbeginline<>0 then begin
print(842);printint(abs(packbeginline));print(843);end else print(844);
printint(line);println;end;begindiagnostic;showbox(r);
enddiagnostic(true){:675};10:vpackage:=r;end;{:668}{679:}
procedure appendtovlist(b:halfword);var d:scaled;p:halfword;
begin if curlist.auxfield.int>-65536000 then begin d:=mem[eqtb[10283].hh
.rh+1].int-curlist.auxfield.int-mem[b+3].int;
if d<eqtb[13232].int then p:=newparamglue(0)else begin p:=newskipparam(1
);mem[tempptr+1].int:=d;end;mem[curlist.tailfield].hh.rh:=p;
curlist.tailfield:=p;end;mem[curlist.tailfield].hh.rh:=b;
curlist.tailfield:=b;curlist.auxfield.int:=mem[b+2].int;end;{:679}{686:}
function newnoad:halfword;var p:halfword;begin p:=getnode(4);
mem[p].hh.b0:=16;mem[p].hh.b1:=0;mem[p+1].hh:=emptyfield;
mem[p+3].hh:=emptyfield;mem[p+2].hh:=emptyfield;newnoad:=p;end;{:686}
{688:}function newstyle(s:smallnumber):halfword;var p:halfword;
begin p:=getnode(3);mem[p].hh.b0:=14;mem[p].hh.b1:=s;mem[p+1].int:=0;
mem[p+2].int:=0;newstyle:=p;end;{:688}{689:}function newchoice:halfword;
var p:halfword;begin p:=getnode(3);mem[p].hh.b0:=15;mem[p].hh.b1:=0;
mem[p+1].hh.lh:=0;mem[p+1].hh.rh:=0;mem[p+2].hh.lh:=0;mem[p+2].hh.rh:=0;
newchoice:=p;end;{:689}{693:}procedure showinfo;
begin shownodelist(mem[tempptr].hh.lh);end;{:693}{704:}
function fractionrule(t:scaled):halfword;var p:halfword;
begin p:=newrule;mem[p+3].int:=t;mem[p+2].int:=0;fractionrule:=p;end;
{:704}{705:}function overbar(b:halfword;k,t:scaled):halfword;
var p,q:halfword;begin p:=newkern(k);mem[p].hh.rh:=b;q:=fractionrule(t);
mem[q].hh.rh:=p;p:=newkern(t);mem[p].hh.rh:=q;
overbar:=vpackage(p,0,1,1073741823);end;{:705}{706:}{709:}
function charbox(f:internalfontnumber;c:quarterword):halfword;
var q:fourquarters;hd:eightbits;b,p:halfword;
begin q:=fontinfo[charbase[f]+c].qqqq;hd:=q.b1;b:=newnullbox;
mem[b+1].int:=fontinfo[widthbase[f]+q.b0].int+fontinfo[italicbase[f]+(q.
b2)div 4].int;mem[b+3].int:=fontinfo[heightbase[f]+(hd)div 16].int;
mem[b+2].int:=fontinfo[depthbase[f]+(hd)mod 16].int;p:=getavail;
mem[p].hh.b1:=c;mem[p].hh.b0:=f;mem[b+5].hh.rh:=p;charbox:=b;end;{:709}
{711:}procedure stackintobox(b:halfword;f:internalfontnumber;
c:quarterword);var p:halfword;begin p:=charbox(f,c);
mem[p].hh.rh:=mem[b+5].hh.rh;mem[b+5].hh.rh:=p;
mem[b+3].int:=mem[p+3].int;end;{:711}{712:}
function heightplusdepth(f:internalfontnumber;c:quarterword):scaled;
var q:fourquarters;hd:eightbits;begin q:=fontinfo[charbase[f]+c].qqqq;
hd:=q.b1;
heightplusdepth:=fontinfo[heightbase[f]+(hd)div 16].int+fontinfo[
depthbase[f]+(hd)mod 16].int;end;{:712}function vardelimiter(d:halfword;
s:smallnumber;v:scaled):halfword;label 40,22;var b:halfword;
f,g:internalfontnumber;c,x,y:quarterword;m,n:integer;u:scaled;w:scaled;
q:fourquarters;hd:eightbits;r:fourquarters;z:smallnumber;
largeattempt:boolean;begin f:=0;w:=0;largeattempt:=false;
z:=mem[d].qqqq.b0;x:=mem[d].qqqq.b1;while true do begin{707:}
if(z<>0)or(x<>0)then begin z:=z+s+16;repeat z:=z-16;
g:=eqtb[11335+z].hh.rh;if g<>0 then{708:}begin y:=x;
if(y>=fontbc[g])and(y<=fontec[g])then begin 22:q:=fontinfo[charbase[g]+y
].qqqq;if(q.b0>0)then begin if((q.b2)mod 4)=3 then begin f:=g;c:=y;
goto 40;end;hd:=q.b1;
u:=fontinfo[heightbase[g]+(hd)div 16].int+fontinfo[depthbase[g]+(hd)mod
16].int;if u>w then begin f:=g;c:=y;w:=u;if u>=v then goto 40;end;
if((q.b2)mod 4)=2 then begin y:=q.b3;goto 22;end;end;end;end{:708};
until z<16;end{:707};if largeattempt then goto 40;largeattempt:=true;
z:=mem[d].qqqq.b2;x:=mem[d].qqqq.b3;end;40:if f<>0 then{710:}
if((q.b2)mod 4)=3 then{713:}begin b:=newnullbox;mem[b].hh.b0:=1;
r:=fontinfo[extenbase[f]+q.b3].qqqq;{714:}c:=r.b3;
u:=heightplusdepth(f,c);w:=0;q:=fontinfo[charbase[f]+c].qqqq;
mem[b+1].int:=fontinfo[widthbase[f]+q.b0].int+fontinfo[italicbase[f]+(q.
b2)div 4].int;c:=r.b2;if c<>0 then w:=w+heightplusdepth(f,c);c:=r.b1;
if c<>0 then w:=w+heightplusdepth(f,c);c:=r.b0;
if c<>0 then w:=w+heightplusdepth(f,c);n:=0;
if u>0 then while w<v do begin w:=w+u;incr(n);if r.b1<>0 then w:=w+u;
end{:714};c:=r.b2;if c<>0 then stackintobox(b,f,c);c:=r.b3;
for m:=1 to n do stackintobox(b,f,c);c:=r.b1;
if c<>0 then begin stackintobox(b,f,c);c:=r.b3;
for m:=1 to n do stackintobox(b,f,c);end;c:=r.b0;
if c<>0 then stackintobox(b,f,c);mem[b+2].int:=w-mem[b+3].int;end{:713}
else b:=charbox(f,c){:710}else begin b:=newnullbox;
mem[b+1].int:=eqtb[13241].int;end;
mem[b+4].int:=half(mem[b+3].int-mem[b+2].int)-fontinfo[22+parambase[eqtb
[11337+s].hh.rh]].int;vardelimiter:=b;end;{:706}{715:}
function rebox(b:halfword;w:scaled):halfword;var p:halfword;
f:internalfontnumber;v:scaled;
begin if(mem[b+1].int<>w)and(mem[b+5].hh.rh<>0)then begin if mem[b].hh.
b0=1 then b:=hpack(b,0,1);p:=mem[b+5].hh.rh;
if((p>=himemmin))and(mem[p].hh.rh=0)then begin f:=mem[p].hh.b0;
v:=fontinfo[widthbase[f]+fontinfo[charbase[f]+mem[p].hh.b1].qqqq.b0].int
;if v<>mem[b+1].int then mem[p].hh.rh:=newkern(mem[b+1].int-v);end;
freenode(b,7);b:=newglue(12);mem[b].hh.rh:=p;
while mem[p].hh.rh<>0 do p:=mem[p].hh.rh;mem[p].hh.rh:=newglue(12);
rebox:=hpack(b,w,0);end else begin mem[b+1].int:=w;rebox:=b;end;end;
{:715}{716:}function mathglue(g:halfword;m:scaled):halfword;
var p:halfword;n:integer;f:scaled;begin n:=xovern(m,65536);f:=remainder;
p:=getnode(4);
mem[p+1].int:=multandadd(n,mem[g+1].int,xnoverd(mem[g+1].int,f,65536),
1073741823);mem[p].hh.b0:=mem[g].hh.b0;
if mem[p].hh.b0=0 then mem[p+2].int:=multandadd(n,mem[g+2].int,xnoverd(
mem[g+2].int,f,65536),1073741823)else mem[p+2].int:=mem[g+2].int;
mem[p].hh.b1:=mem[g].hh.b1;
if mem[p].hh.b1=0 then mem[p+3].int:=multandadd(n,mem[g+3].int,xnoverd(
mem[g+3].int,f,65536),1073741823)else mem[p+3].int:=mem[g+3].int;
mathglue:=p;end;{:716}{717:}procedure mathkern(p:halfword;m:scaled);
var n:integer;f:scaled;
begin if mem[p].hh.b1=99 then begin n:=xovern(m,65536);f:=remainder;
mem[p+1].int:=multandadd(n,mem[p+1].int,xnoverd(mem[p+1].int,f,65536),
1073741823);mem[p].hh.b1:=0;end;end;{:717}{718:}procedure flushmath;
begin flushnodelist(mem[curlist.headfield].hh.rh);
flushnodelist(curlist.auxfield.int);mem[curlist.headfield].hh.rh:=0;
curlist.tailfield:=curlist.headfield;curlist.auxfield.int:=0;end;{:718}
{720:}procedure mlisttohlist;forward;function cleanbox(p:halfword;
s:smallnumber):halfword;label 40;var q:halfword;savestyle:smallnumber;
x:halfword;r:halfword;
begin case mem[p].hh.rh of 1:begin curmlist:=newnoad;
mem[curmlist+1]:=mem[p];end;2:begin q:=mem[p].hh.lh;goto 40;end;
3:curmlist:=mem[p].hh.lh;others:begin q:=newnullbox;goto 40;end end;
savestyle:=curstyle;curstyle:=s;mlistpenalties:=false;mlisttohlist;
q:=mem[memtop-3].hh.rh;curstyle:=savestyle;{703:}
begin if curstyle<4 then cursize:=0 else cursize:=16*((curstyle-2)div 2)
;curmu:=xovern(fontinfo[6+parambase[eqtb[11337+cursize].hh.rh]].int,18);
end{:703};
40:if(q>=himemmin)or(q=0)then x:=hpack(q,0,1)else if(mem[q].hh.rh=0)and(
mem[q].hh.b0<=1)and(mem[q+4].int=0)then x:=q else x:=hpack(q,0,1);{721:}
q:=mem[x+5].hh.rh;if(q>=himemmin)then begin r:=mem[q].hh.rh;
if r<>0 then if mem[r].hh.rh=0 then if not(r>=himemmin)then if mem[r].hh
.b0=11 then begin freenode(r,2);mem[q].hh.rh:=0;end;end{:721};
cleanbox:=x;end;{:720}{722:}procedure fetch(a:halfword);
begin curc:=mem[a].hh.b1;curf:=eqtb[11335+mem[a].hh.b0+cursize].hh.rh;
if curf=0 then{723:}begin begin if interaction=3 then wakeupterminal;
printnl(262);print(335);end;printsize(cursize);printchar(32);
printint(mem[a].hh.b0);print(877);print(curc);printchar(41);
begin helpptr:=4;helpline[3]:=878;helpline[2]:=879;helpline[1]:=880;
helpline[0]:=881;end;error;curi:=nullcharacter;mem[a].hh.rh:=0;end{:723}
else begin if(curc>=fontbc[curf])and(curc<=fontec[curf])then curi:=
fontinfo[charbase[curf]+curc].qqqq else curi:=nullcharacter;
if not((curi.b0>0))then begin charwarning(curf,curc);mem[a].hh.rh:=0;
end;end;end;{:722}{726:}{734:}procedure makeover(q:halfword);
begin mem[q+1].hh.lh:=overbar(cleanbox(q+1,2*(curstyle div 2)+1),3*
fontinfo[8+parambase[eqtb[11338+cursize].hh.rh]].int,fontinfo[8+
parambase[eqtb[11338+cursize].hh.rh]].int);mem[q+1].hh.rh:=2;end;{:734}
{735:}procedure makeunder(q:halfword);var p,x,y:halfword;delta:scaled;
begin x:=cleanbox(q+1,curstyle);
p:=newkern(3*fontinfo[8+parambase[eqtb[11338+cursize].hh.rh]].int);
mem[x].hh.rh:=p;
mem[p].hh.rh:=fractionrule(fontinfo[8+parambase[eqtb[11338+cursize].hh.
rh]].int);y:=vpackage(x,0,1,1073741823);
delta:=mem[y+3].int+mem[y+2].int+fontinfo[8+parambase[eqtb[11338+cursize
].hh.rh]].int;mem[y+3].int:=mem[x+3].int;
mem[y+2].int:=delta-mem[y+3].int;mem[q+1].hh.lh:=y;mem[q+1].hh.rh:=2;
end;{:735}{736:}procedure makevcenter(q:halfword);var v:halfword;
delta:scaled;begin v:=mem[q+1].hh.lh;
if mem[v].hh.b0<>1 then confusion(535);delta:=mem[v+3].int+mem[v+2].int;
mem[v+3].int:=fontinfo[22+parambase[eqtb[11337+cursize].hh.rh]].int+half
(delta);mem[v+2].int:=delta-mem[v+3].int;end;{:736}{737:}
procedure makeradical(q:halfword);var x,y:halfword;delta,clr:scaled;
begin x:=cleanbox(q+1,2*(curstyle div 2)+1);
if curstyle<2 then clr:=fontinfo[8+parambase[eqtb[11338+cursize].hh.rh]]
.int+(abs(fontinfo[5+parambase[eqtb[11337+cursize].hh.rh]].int)div 4)
else begin clr:=fontinfo[8+parambase[eqtb[11338+cursize].hh.rh]].int;
clr:=clr+(abs(clr)div 4);end;
y:=vardelimiter(q+4,cursize,mem[x+3].int+mem[x+2].int+clr+fontinfo[8+
parambase[eqtb[11338+cursize].hh.rh]].int);
delta:=mem[y+2].int-(mem[x+3].int+mem[x+2].int+clr);
if delta>0 then clr:=clr+half(delta);mem[y+4].int:=-(mem[x+3].int+clr);
mem[y].hh.rh:=overbar(x,clr,mem[y+3].int);mem[q+1].hh.lh:=hpack(y,0,1);
mem[q+1].hh.rh:=2;end;{:737}{738:}procedure makemathaccent(q:halfword);
label 30,31;var p,x,y:halfword;a:integer;c:quarterword;
f:internalfontnumber;i:fourquarters;s:scaled;h:scaled;delta:scaled;
w:scaled;begin fetch(q+4);if(curi.b0>0)then begin i:=curi;c:=curc;
f:=curf;{741:}s:=0;if mem[q+1].hh.rh=1 then begin fetch(q+1);
if((curi.b2)mod 4)=1 then begin a:=ligkernbase[curf]+curi.b3;
curi:=fontinfo[a].qqqq;
if curi.b0>128 then begin a:=ligkernbase[curf]+256*curi.b2+curi.b3
+32768-256*(128);curi:=fontinfo[a].qqqq;end;
while true do begin if curi.b1=skewchar[curf]then begin if curi.b2>=128
then if curi.b0<=128 then s:=fontinfo[kernbase[curf]+256*curi.b2+curi.b3
].int;goto 31;end;if curi.b0>=128 then goto 31;a:=a+curi.b0+1;
curi:=fontinfo[a].qqqq;end;end;end;31:{:741};
x:=cleanbox(q+1,2*(curstyle div 2)+1);w:=mem[x+1].int;h:=mem[x+3].int;
{740:}while true do begin if((i.b2)mod 4)<>2 then goto 30;y:=i.b3;
i:=fontinfo[charbase[f]+y].qqqq;if not(i.b0>0)then goto 30;
if fontinfo[widthbase[f]+i.b0].int>w then goto 30;c:=y;end;30:{:740};
if h<fontinfo[5+parambase[f]].int then delta:=h else delta:=fontinfo[5+
parambase[f]].int;
if(mem[q+2].hh.rh<>0)or(mem[q+3].hh.rh<>0)then if mem[q+1].hh.rh=1 then{
742:}begin flushnodelist(x);x:=newnoad;mem[x+1]:=mem[q+1];
mem[x+2]:=mem[q+2];mem[x+3]:=mem[q+3];mem[q+2].hh:=emptyfield;
mem[q+3].hh:=emptyfield;mem[q+1].hh.rh:=3;mem[q+1].hh.lh:=x;
x:=cleanbox(q+1,curstyle);delta:=delta+mem[x+3].int-h;h:=mem[x+3].int;
end{:742};y:=charbox(f,c);mem[y+4].int:=s+half(w-mem[y+1].int);
mem[y+1].int:=0;p:=newkern(-delta);mem[p].hh.rh:=x;mem[y].hh.rh:=p;
y:=vpackage(y,0,1,1073741823);mem[y+1].int:=mem[x+1].int;
if mem[y+3].int<h then{739:}begin p:=newkern(h-mem[y+3].int);
mem[p].hh.rh:=mem[y+5].hh.rh;mem[y+5].hh.rh:=p;mem[y+3].int:=h;end{:739}
;mem[q+1].hh.lh:=y;mem[q+1].hh.rh:=2;end;end;{:738}{743:}
procedure makefraction(q:halfword);var p,v,x,y,z:halfword;
delta,delta1,delta2,shiftup,shiftdown,clr:scaled;
begin if mem[q+1].int=1073741824 then mem[q+1].int:=fontinfo[8+parambase
[eqtb[11338+cursize].hh.rh]].int;{744:}
x:=cleanbox(q+2,curstyle+2-2*(curstyle div 6));
z:=cleanbox(q+3,2*(curstyle div 2)+3-2*(curstyle div 6));
if mem[x+1].int<mem[z+1].int then x:=rebox(x,mem[z+1].int)else z:=rebox(
z,mem[x+1].int);
if curstyle<2 then begin shiftup:=fontinfo[8+parambase[eqtb[11337+
cursize].hh.rh]].int;
shiftdown:=fontinfo[11+parambase[eqtb[11337+cursize].hh.rh]].int;
end else begin shiftdown:=fontinfo[12+parambase[eqtb[11337+cursize].hh.
rh]].int;
if mem[q+1].int<>0 then shiftup:=fontinfo[9+parambase[eqtb[11337+cursize
].hh.rh]].int else shiftup:=fontinfo[10+parambase[eqtb[11337+cursize].hh
.rh]].int;end{:744};if mem[q+1].int=0 then{745:}
begin if curstyle<2 then clr:=7*fontinfo[8+parambase[eqtb[11338+cursize]
.hh.rh]].int else clr:=3*fontinfo[8+parambase[eqtb[11338+cursize].hh.rh]
].int;
delta:=half(clr-((shiftup-mem[x+2].int)-(mem[z+3].int-shiftdown)));
if delta>0 then begin shiftup:=shiftup+delta;shiftdown:=shiftdown+delta;
end;end{:745}else{746:}
begin if curstyle<2 then clr:=3*mem[q+1].int else clr:=mem[q+1].int;
delta:=half(mem[q+1].int);
delta1:=clr-((shiftup-mem[x+2].int)-(fontinfo[22+parambase[eqtb[11337+
cursize].hh.rh]].int+delta));
delta2:=clr-((fontinfo[22+parambase[eqtb[11337+cursize].hh.rh]].int-
delta)-(mem[z+3].int-shiftdown));
if delta1>0 then shiftup:=shiftup+delta1;
if delta2>0 then shiftdown:=shiftdown+delta2;end{:746};{747:}
v:=newnullbox;mem[v].hh.b0:=1;mem[v+3].int:=shiftup+mem[x+3].int;
mem[v+2].int:=mem[z+2].int+shiftdown;mem[v+1].int:=mem[x+1].int;
if mem[q+1].int=0 then begin p:=newkern((shiftup-mem[x+2].int)-(mem[z+3]
.int-shiftdown));mem[p].hh.rh:=z;
end else begin y:=fractionrule(mem[q+1].int);
p:=newkern((fontinfo[22+parambase[eqtb[11337+cursize].hh.rh]].int-delta)
-(mem[z+3].int-shiftdown));mem[y].hh.rh:=p;mem[p].hh.rh:=z;
p:=newkern((shiftup-mem[x+2].int)-(fontinfo[22+parambase[eqtb[11337+
cursize].hh.rh]].int+delta));mem[p].hh.rh:=y;end;mem[x].hh.rh:=p;
mem[v+5].hh.rh:=x{:747};{748:}
if curstyle<2 then delta:=fontinfo[20+parambase[eqtb[11337+cursize].hh.
rh]].int else delta:=fontinfo[21+parambase[eqtb[11337+cursize].hh.rh]].
int;x:=vardelimiter(q+4,cursize,delta);mem[x].hh.rh:=v;
z:=vardelimiter(q+5,cursize,delta);mem[v].hh.rh:=z;
mem[q+1].int:=hpack(x,0,1){:748};end;{:743}{749:}
function makeop(q:halfword):scaled;var delta:scaled;p,v,x,y,z:halfword;
c:quarterword;i:fourquarters;shiftup,shiftdown:scaled;
begin if(mem[q].hh.b1=0)and(curstyle<2)then mem[q].hh.b1:=1;
if mem[q+1].hh.rh=1 then begin fetch(q+1);
if(curstyle<2)and(((curi.b2)mod 4)=2)then begin c:=curi.b3;
i:=fontinfo[charbase[curf]+c].qqqq;if(i.b0>0)then begin curc:=c;curi:=i;
mem[q+1].hh.b1:=c;end;end;
delta:=fontinfo[italicbase[curf]+(curi.b2)div 4].int;
x:=cleanbox(q+1,curstyle);
if(mem[q+3].hh.rh<>0)and(mem[q].hh.b1<>1)then mem[x+1].int:=mem[x+1].int
-delta;
mem[x+4].int:=half(mem[x+3].int-mem[x+2].int)-fontinfo[22+parambase[eqtb
[11337+cursize].hh.rh]].int;mem[q+1].hh.rh:=2;mem[q+1].hh.lh:=x;
end else delta:=0;if mem[q].hh.b1=1 then{750:}
begin x:=cleanbox(q+2,2*(curstyle div 4)+4+(curstyle mod 2));
y:=cleanbox(q+1,curstyle);z:=cleanbox(q+3,2*(curstyle div 4)+5);
v:=newnullbox;mem[v].hh.b0:=1;mem[v+1].int:=mem[y+1].int;
if mem[x+1].int>mem[v+1].int then mem[v+1].int:=mem[x+1].int;
if mem[z+1].int>mem[v+1].int then mem[v+1].int:=mem[z+1].int;
x:=rebox(x,mem[v+1].int);y:=rebox(y,mem[v+1].int);
z:=rebox(z,mem[v+1].int);mem[x+4].int:=half(delta);
mem[z+4].int:=-mem[x+4].int;mem[v+3].int:=mem[y+3].int;
mem[v+2].int:=mem[y+2].int;{751:}
if mem[q+2].hh.rh=0 then begin freenode(x,7);mem[v+5].hh.rh:=y;
end else begin shiftup:=fontinfo[11+parambase[eqtb[11338+cursize].hh.rh]
].int-mem[x+2].int;
if shiftup<fontinfo[9+parambase[eqtb[11338+cursize].hh.rh]].int then
shiftup:=fontinfo[9+parambase[eqtb[11338+cursize].hh.rh]].int;
p:=newkern(shiftup);mem[p].hh.rh:=y;mem[x].hh.rh:=p;
p:=newkern(fontinfo[13+parambase[eqtb[11338+cursize].hh.rh]].int);
mem[p].hh.rh:=x;mem[v+5].hh.rh:=p;
mem[v+3].int:=mem[v+3].int+fontinfo[13+parambase[eqtb[11338+cursize].hh.
rh]].int+mem[x+3].int+mem[x+2].int+shiftup;end;
if mem[q+3].hh.rh=0 then freenode(z,7)else begin shiftdown:=fontinfo[12+
parambase[eqtb[11338+cursize].hh.rh]].int-mem[z+3].int;
if shiftdown<fontinfo[10+parambase[eqtb[11338+cursize].hh.rh]].int then
shiftdown:=fontinfo[10+parambase[eqtb[11338+cursize].hh.rh]].int;
p:=newkern(shiftdown);mem[y].hh.rh:=p;mem[p].hh.rh:=z;
p:=newkern(fontinfo[13+parambase[eqtb[11338+cursize].hh.rh]].int);
mem[z].hh.rh:=p;
mem[v+2].int:=mem[v+2].int+fontinfo[13+parambase[eqtb[11338+cursize].hh.
rh]].int+mem[z+3].int+mem[z+2].int+shiftdown;end{:751};mem[q+1].int:=v;
end{:750};makeop:=delta;end;{:749}{752:}procedure makeord(q:halfword);
label 20,10;var a:integer;p,r:halfword;
begin 20:if mem[q+3].hh.rh=0 then if mem[q+2].hh.rh=0 then if mem[q+1].
hh.rh=1 then begin p:=mem[q].hh.rh;
if p<>0 then if(mem[p].hh.b0>=16)and(mem[p].hh.b0<=22)then if mem[p+1].
hh.rh=1 then if mem[p+1].hh.b0=mem[q+1].hh.b0 then begin mem[q+1].hh.rh
:=4;fetch(q+1);
if((curi.b2)mod 4)=1 then begin a:=ligkernbase[curf]+curi.b3;
curc:=mem[p+1].hh.b1;curi:=fontinfo[a].qqqq;
if curi.b0>128 then begin a:=ligkernbase[curf]+256*curi.b2+curi.b3
+32768-256*(128);curi:=fontinfo[a].qqqq;end;while true do begin{753:}
if curi.b1=curc then if curi.b0<=128 then if curi.b2>=128 then begin p:=
newkern(fontinfo[kernbase[curf]+256*curi.b2+curi.b3].int);
mem[p].hh.rh:=mem[q].hh.rh;mem[q].hh.rh:=p;goto 10;
end else begin begin if interrupt<>0 then pauseforinstructions;end;
case curi.b2 of 1,5:mem[q+1].hh.b1:=curi.b3;2,6:mem[p+1].hh.b1:=curi.b3;
3,7,11:begin r:=newnoad;mem[r+1].hh.b1:=curi.b3;
mem[r+1].hh.b0:=mem[q+1].hh.b0;mem[q].hh.rh:=r;mem[r].hh.rh:=p;
if curi.b2<11 then mem[r+1].hh.rh:=1 else mem[r+1].hh.rh:=4;end;
others:begin mem[q].hh.rh:=mem[p].hh.rh;mem[q+1].hh.b1:=curi.b3;
mem[q+3]:=mem[p+3];mem[q+2]:=mem[p+2];freenode(p,4);end end;
if curi.b2>3 then goto 10;mem[q+1].hh.rh:=1;goto 20;end{:753};
if curi.b0>=128 then goto 10;a:=a+curi.b0+1;curi:=fontinfo[a].qqqq;end;
end;end;end;10:end;{:752}{756:}procedure makescripts(q:halfword;
delta:scaled);var p,x,y,z:halfword;shiftup,shiftdown,clr:scaled;
t:smallnumber;begin p:=mem[q+1].int;
if(p>=himemmin)then begin shiftup:=0;shiftdown:=0;
end else begin z:=hpack(p,0,1);if curstyle<4 then t:=16 else t:=32;
shiftup:=mem[z+3].int-fontinfo[18+parambase[eqtb[11337+t].hh.rh]].int;
shiftdown:=mem[z+2].int+fontinfo[19+parambase[eqtb[11337+t].hh.rh]].int;
freenode(z,7);end;if mem[q+2].hh.rh=0 then{757:}
begin x:=cleanbox(q+3,2*(curstyle div 4)+5);
mem[x+1].int:=mem[x+1].int+eqtb[13242].int;
if shiftdown<fontinfo[16+parambase[eqtb[11337+cursize].hh.rh]].int then
shiftdown:=fontinfo[16+parambase[eqtb[11337+cursize].hh.rh]].int;
clr:=mem[x+3].int-(abs(fontinfo[5+parambase[eqtb[11337+cursize].hh.rh]].
int*4)div 5);if shiftdown<clr then shiftdown:=clr;
mem[x+4].int:=shiftdown;end{:757}else begin{758:}
begin x:=cleanbox(q+2,2*(curstyle div 4)+4+(curstyle mod 2));
mem[x+1].int:=mem[x+1].int+eqtb[13242].int;
if odd(curstyle)then clr:=fontinfo[15+parambase[eqtb[11337+cursize].hh.
rh]].int else if curstyle<2 then clr:=fontinfo[13+parambase[eqtb[11337+
cursize].hh.rh]].int else clr:=fontinfo[14+parambase[eqtb[11337+cursize]
.hh.rh]].int;if shiftup<clr then shiftup:=clr;
clr:=mem[x+2].int+(abs(fontinfo[5+parambase[eqtb[11337+cursize].hh.rh]].
int)div 4);if shiftup<clr then shiftup:=clr;end{:758};
if mem[q+3].hh.rh=0 then mem[x+4].int:=-shiftup else{759:}
begin y:=cleanbox(q+3,2*(curstyle div 4)+5);
mem[y+1].int:=mem[y+1].int+eqtb[13242].int;
if shiftdown<fontinfo[17+parambase[eqtb[11337+cursize].hh.rh]].int then
shiftdown:=fontinfo[17+parambase[eqtb[11337+cursize].hh.rh]].int;
clr:=4*fontinfo[8+parambase[eqtb[11338+cursize].hh.rh]].int-((shiftup-
mem[x+2].int)-(mem[y+3].int-shiftdown));
if clr>0 then begin shiftdown:=shiftdown+clr;
clr:=(abs(fontinfo[5+parambase[eqtb[11337+cursize].hh.rh]].int*4)div 5)-
(shiftup-mem[x+2].int);if clr>0 then begin shiftup:=shiftup+clr;
shiftdown:=shiftdown-clr;end;end;mem[x+4].int:=delta;
p:=newkern((shiftup-mem[x+2].int)-(mem[y+3].int-shiftdown));
mem[x].hh.rh:=p;mem[p].hh.rh:=y;x:=vpackage(x,0,1,1073741823);
mem[x+4].int:=shiftdown;end{:759};end;
if mem[q+1].int=0 then mem[q+1].int:=x else begin p:=mem[q+1].int;
while mem[p].hh.rh<>0 do p:=mem[p].hh.rh;mem[p].hh.rh:=x;end;end;{:756}
{762:}function makeleftright(q:halfword;style:smallnumber;
maxd,maxh:scaled):smallnumber;var delta,delta1,delta2:scaled;
begin if style<4 then cursize:=0 else cursize:=16*((style-2)div 2);
delta2:=maxd+fontinfo[22+parambase[eqtb[11337+cursize].hh.rh]].int;
delta1:=maxh+maxd-delta2;if delta2>delta1 then delta1:=delta2;
delta:=(delta1 div 500)*eqtb[12681].int;
delta2:=delta1+delta1-eqtb[13240].int;
if delta<delta2 then delta:=delta2;
mem[q+1].int:=vardelimiter(q+1,cursize,delta);
makeleftright:=mem[q].hh.b0-(10);end;{:762}procedure mlisttohlist;
label 21,82,80,81,83,30;var mlist:halfword;penalties:boolean;
style:smallnumber;savestyle:smallnumber;q:halfword;r:halfword;
rtype:smallnumber;t:smallnumber;p,x,y,z:halfword;pen:integer;
s:smallnumber;maxh,maxd:scaled;delta:scaled;begin mlist:=curmlist;
penalties:=mlistpenalties;style:=curstyle;q:=mlist;r:=0;rtype:=17;
maxh:=0;maxd:=0;{703:}
begin if curstyle<4 then cursize:=0 else cursize:=16*((curstyle-2)div 2)
;curmu:=xovern(fontinfo[6+parambase[eqtb[11337+cursize].hh.rh]].int,18);
end{:703};while q<>0 do{727:}begin{728:}21:delta:=0;
case mem[q].hh.b0 of 18:case rtype of 18,17,19,20,22,30:begin mem[q].hh.
b0:=16;goto 21;end;others:end;19,21,22,31:begin{729:}
if rtype=18 then mem[r].hh.b0:=16{:729};if mem[q].hh.b0=31 then goto 80;
end;{733:}30:goto 80;25:begin makefraction(q);goto 82;end;
17:begin delta:=makeop(q);if mem[q].hh.b1=1 then goto 82;end;
16:makeord(q);20,23:;24:makeradical(q);27:makeover(q);26:makeunder(q);
28:makemathaccent(q);29:makevcenter(q);{:733}{730:}
14:begin curstyle:=mem[q].hh.b1;{703:}
begin if curstyle<4 then cursize:=0 else cursize:=16*((curstyle-2)div 2)
;curmu:=xovern(fontinfo[6+parambase[eqtb[11337+cursize].hh.rh]].int,18);
end{:703};goto 81;end;15:{731:}
begin case curstyle div 2 of 0:begin p:=mem[q+1].hh.lh;
mem[q+1].hh.lh:=0;end;1:begin p:=mem[q+1].hh.rh;mem[q+1].hh.rh:=0;end;
2:begin p:=mem[q+2].hh.lh;mem[q+2].hh.lh:=0;end;
3:begin p:=mem[q+2].hh.rh;mem[q+2].hh.rh:=0;end;end;
flushnodelist(mem[q+1].hh.lh);flushnodelist(mem[q+1].hh.rh);
flushnodelist(mem[q+2].hh.lh);flushnodelist(mem[q+2].hh.rh);
mem[q].hh.b0:=14;mem[q].hh.b1:=curstyle;mem[q+1].int:=0;mem[q+2].int:=0;
if p<>0 then begin z:=mem[q].hh.rh;mem[q].hh.rh:=p;
while mem[p].hh.rh<>0 do p:=mem[p].hh.rh;mem[p].hh.rh:=z;end;goto 81;
end{:731};3,4,5,8,12,7:goto 81;
2:begin if mem[q+3].int>maxh then maxh:=mem[q+3].int;
if mem[q+2].int>maxd then maxd:=mem[q+2].int;goto 81;end;10:begin{732:}
if mem[q].hh.b1=99 then begin x:=mem[q+1].hh.lh;y:=mathglue(x,curmu);
deleteglueref(x);mem[q+1].hh.lh:=y;mem[q].hh.b1:=0;
end else if(cursize<>0)and(mem[q].hh.b1=98)then begin p:=mem[q].hh.rh;
if p<>0 then if(mem[p].hh.b0=10)or(mem[p].hh.b0=11)then begin mem[q].hh.
rh:=mem[p].hh.rh;mem[p].hh.rh:=0;flushnodelist(p);end;end{:732};goto 81;
end;11:begin mathkern(q,curmu);goto 81;end;{:730}
others:confusion(882)end;{754:}case mem[q+1].hh.rh of 1,4:{755:}
begin fetch(q+1);
if(curi.b0>0)then begin delta:=fontinfo[italicbase[curf]+(curi.b2)div 4]
.int;p:=newcharacter(curf,curc);
if(mem[q+1].hh.rh=4)and(fontinfo[2+parambase[curf]].int<>0)then delta:=0
;
if(mem[q+3].hh.rh=0)and(delta<>0)then begin mem[p].hh.rh:=newkern(delta)
;delta:=0;end;end else p:=0;end{:755};0:p:=0;2:p:=mem[q+1].hh.lh;
3:begin curmlist:=mem[q+1].hh.lh;savestyle:=curstyle;
mlistpenalties:=false;mlisttohlist;curstyle:=savestyle;{703:}
begin if curstyle<4 then cursize:=0 else cursize:=16*((curstyle-2)div 2)
;curmu:=xovern(fontinfo[6+parambase[eqtb[11337+cursize].hh.rh]].int,18);
end{:703};p:=hpack(mem[memtop-3].hh.rh,0,1);end;
others:confusion(883)end;mem[q+1].int:=p;
if(mem[q+3].hh.rh=0)and(mem[q+2].hh.rh=0)then goto 82;
makescripts(q,delta){:754}{:728};82:z:=hpack(mem[q+1].int,0,1);
if mem[z+3].int>maxh then maxh:=mem[z+3].int;
if mem[z+2].int>maxd then maxd:=mem[z+2].int;freenode(z,7);80:r:=q;
rtype:=mem[r].hh.b0;81:q:=mem[q].hh.rh;end{:727};{729:}
if rtype=18 then mem[r].hh.b0:=16{:729};{760:}p:=memtop-3;
mem[p].hh.rh:=0;q:=mlist;rtype:=0;curstyle:=style;{703:}
begin if curstyle<4 then cursize:=0 else cursize:=16*((curstyle-2)div 2)
;curmu:=xovern(fontinfo[6+parambase[eqtb[11337+cursize].hh.rh]].int,18);
end{:703};while q<>0 do begin{761:}t:=16;s:=4;pen:=10000;
case mem[q].hh.b0 of 17,20,21,22,23:t:=mem[q].hh.b0;18:begin t:=18;
pen:=eqtb[12672].int;end;19:begin t:=19;pen:=eqtb[12673].int;end;
16,29,27,26:;24:s:=5;28:s:=5;25:begin t:=23;s:=6;end;
30,31:t:=makeleftright(q,style,maxd,maxh);14:{763:}
begin curstyle:=mem[q].hh.b1;s:=3;{703:}
begin if curstyle<4 then cursize:=0 else cursize:=16*((curstyle-2)div 2)
;curmu:=xovern(fontinfo[6+parambase[eqtb[11337+cursize].hh.rh]].int,18);
end{:703};goto 83;end{:763};8,12,2,7,5,3,4,10,11:begin mem[p].hh.rh:=q;
p:=q;q:=mem[q].hh.rh;mem[p].hh.rh:=0;goto 30;end;
others:confusion(884)end{:761};{766:}
if rtype>0 then begin case strpool[rtype*8+t+magicoffset]of 48:x:=0;
49:if curstyle<4 then x:=15 else x:=0;50:x:=15;
51:if curstyle<4 then x:=16 else x:=0;
52:if curstyle<4 then x:=17 else x:=0;others:confusion(886)end;
if x<>0 then begin y:=mathglue(eqtb[10282+x].hh.rh,curmu);z:=newglue(y);
mem[y].hh.rh:=0;mem[p].hh.rh:=z;p:=z;mem[z].hh.b1:=x+1;end;end{:766};
{767:}if mem[q+1].int<>0 then begin mem[p].hh.rh:=mem[q+1].int;
repeat p:=mem[p].hh.rh;until mem[p].hh.rh=0;end;
if penalties then if mem[q].hh.rh<>0 then if pen<10000 then begin rtype
:=mem[mem[q].hh.rh].hh.b0;
if rtype<>12 then if rtype<>19 then begin z:=newpenalty(pen);
mem[p].hh.rh:=z;p:=z;end;end{:767};rtype:=t;83:r:=q;q:=mem[q].hh.rh;
freenode(r,s);30:end{:760};end;{:726}{772:}procedure pushalignment;
var p:halfword;begin p:=getnode(5);mem[p].hh.rh:=alignptr;
mem[p].hh.lh:=curalign;mem[p+1].hh.lh:=mem[memtop-8].hh.rh;
mem[p+1].hh.rh:=curspan;mem[p+2].int:=curloop;mem[p+3].int:=alignstate;
mem[p+4].hh.lh:=curhead;mem[p+4].hh.rh:=curtail;alignptr:=p;
curhead:=getavail;end;procedure popalignment;var p:halfword;
begin begin mem[curhead].hh.rh:=avail;avail:=curhead;
ifdef('STAT')decr(dynused);endif('STAT')end;p:=alignptr;
curtail:=mem[p+4].hh.rh;curhead:=mem[p+4].hh.lh;
alignstate:=mem[p+3].int;curloop:=mem[p+2].int;curspan:=mem[p+1].hh.rh;
mem[memtop-8].hh.rh:=mem[p+1].hh.lh;curalign:=mem[p].hh.lh;
alignptr:=mem[p].hh.rh;freenode(p,5);end;{:772}{774:}{782:}
procedure getpreambletoken;label 20;begin 20:gettoken;
while(curchr=256)and(curcmd=4)do begin gettoken;
if curcmd>100 then begin expand;gettoken;end;end;
if curcmd=9 then fatalerror(591);
if(curcmd=75)and(curchr=10293)then begin scanoptionalequals;scanglue(2);
if eqtb[12706].int>0 then geqdefine(10293,117,curval)else eqdefine(10293
,117,curval);goto 20;end;end;{:782}procedure alignpeek;forward;
procedure normalparagraph;forward;procedure initalign;label 30,31,32,22;
var savecsptr:halfword;p:halfword;begin savecsptr:=curcs;pushalignment;
alignstate:=-1000000;{776:}
if(curlist.modefield=203)and((curlist.tailfield<>curlist.headfield)or(
curlist.auxfield.int<>0))then begin begin if interaction=3 then
wakeupterminal;printnl(262);print(676);end;printesc(516);print(887);
begin helpptr:=3;helpline[2]:=888;helpline[1]:=889;helpline[0]:=890;end;
error;flushmath;end{:776};pushnest;{775:}
if curlist.modefield=203 then begin curlist.modefield:=-1;
curlist.auxfield.int:=nest[nestptr-2].auxfield.int;
end else if curlist.modefield>0 then curlist.modefield:=-curlist.
modefield{:775};scanspec(6,false);{777:}mem[memtop-8].hh.rh:=0;
curalign:=memtop-8;curloop:=0;scannerstatus:=4;warningindex:=savecsptr;
alignstate:=-1000000;while true do begin{778:}
mem[curalign].hh.rh:=newparamglue(11);
curalign:=mem[curalign].hh.rh{:778};if curcmd=5 then goto 30;{779:}
{783:}p:=memtop-4;mem[p].hh.rh:=0;while true do begin getpreambletoken;
if curcmd=6 then goto 31;
if(curcmd<=5)and(curcmd>=4)and(alignstate=-1000000)then if(p=memtop-4)
and(curloop=0)and(curcmd=4)then curloop:=curalign else begin begin if
interaction=3 then wakeupterminal;printnl(262);print(896);end;
begin helpptr:=3;helpline[2]:=897;helpline[1]:=898;helpline[0]:=899;end;
backerror;goto 31;
end else if(curcmd<>10)or(p<>memtop-4)then begin mem[p].hh.rh:=getavail;
p:=mem[p].hh.rh;mem[p].hh.lh:=curtok;end;end;31:{:783};
mem[curalign].hh.rh:=newnullbox;curalign:=mem[curalign].hh.rh;
mem[curalign].hh.lh:=memtop-9;mem[curalign+1].int:=-1073741824;
mem[curalign+3].int:=mem[memtop-4].hh.rh;{784:}p:=memtop-4;
mem[p].hh.rh:=0;while true do begin 22:getpreambletoken;
if(curcmd<=5)and(curcmd>=4)and(alignstate=-1000000)then goto 32;
if curcmd=6 then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(900);end;begin helpptr:=3;helpline[2]:=897;
helpline[1]:=898;helpline[0]:=901;end;error;goto 22;end;
mem[p].hh.rh:=getavail;p:=mem[p].hh.rh;mem[p].hh.lh:=curtok;end;
32:mem[p].hh.rh:=getavail;p:=mem[p].hh.rh;mem[p].hh.lh:=14114{:784};
mem[curalign+2].int:=mem[memtop-4].hh.rh{:779};end;
30:scannerstatus:=0{:777};newsavelevel(6);
if eqtb[10820].hh.rh<>0 then begintokenlist(eqtb[10820].hh.rh,13);
alignpeek;end;{:774}{786:}{787:}procedure initspan(p:halfword);
begin pushnest;
if curlist.modefield=-102 then curlist.auxfield.hh.lh:=1000 else begin
curlist.auxfield.int:=-65536000;normalparagraph;end;curspan:=p;end;
{:787}procedure initrow;begin pushnest;
curlist.modefield:=(-103)-curlist.modefield;
if curlist.modefield=-102 then curlist.auxfield.hh.lh:=0 else curlist.
auxfield.int:=0;
begin mem[curlist.tailfield].hh.rh:=newglue(mem[mem[memtop-8].hh.rh+1].
hh.lh);curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
mem[curlist.tailfield].hh.b1:=12;
curalign:=mem[mem[memtop-8].hh.rh].hh.rh;curtail:=curhead;
initspan(curalign);end;{:786}{788:}procedure initcol;
begin mem[curalign+5].hh.lh:=curcmd;
if curcmd=63 then alignstate:=0 else begin backinput;
begintokenlist(mem[curalign+3].int,1);end;end;{:788}{791:}
function fincol:boolean;label 10;var p:halfword;q,r:halfword;s:halfword;
u:halfword;w:scaled;o:glueord;n:halfword;
begin if curalign=0 then confusion(902);q:=mem[curalign].hh.rh;
if q=0 then confusion(902);if alignstate<500000 then fatalerror(591);
p:=mem[q].hh.rh;{792:}
if(p=0)and(mem[curalign+5].hh.lh<257)then if curloop<>0 then{793:}
begin mem[q].hh.rh:=newnullbox;p:=mem[q].hh.rh;mem[p].hh.lh:=memtop-9;
mem[p+1].int:=-1073741824;curloop:=mem[curloop].hh.rh;{794:}q:=memtop-4;
r:=mem[curloop+3].int;while r<>0 do begin mem[q].hh.rh:=getavail;
q:=mem[q].hh.rh;mem[q].hh.lh:=mem[r].hh.lh;r:=mem[r].hh.rh;end;
mem[q].hh.rh:=0;mem[p+3].int:=mem[memtop-4].hh.rh;q:=memtop-4;
r:=mem[curloop+2].int;while r<>0 do begin mem[q].hh.rh:=getavail;
q:=mem[q].hh.rh;mem[q].hh.lh:=mem[r].hh.lh;r:=mem[r].hh.rh;end;
mem[q].hh.rh:=0;mem[p+2].int:=mem[memtop-4].hh.rh{:794};
curloop:=mem[curloop].hh.rh;mem[p].hh.rh:=newglue(mem[curloop+1].hh.lh);
end{:793}else begin begin if interaction=3 then wakeupterminal;
printnl(262);print(903);end;printesc(892);begin helpptr:=3;
helpline[2]:=904;helpline[1]:=905;helpline[0]:=906;end;
mem[curalign+5].hh.lh:=257;error;end{:792};
if mem[curalign+5].hh.lh<>256 then begin unsave;newsavelevel(6);{796:}
begin if curlist.modefield=-102 then begin adjusttail:=curtail;
u:=hpack(mem[curlist.headfield].hh.rh,0,1);w:=mem[u+1].int;
curtail:=adjusttail;adjusttail:=0;
end else begin u:=vpackage(mem[curlist.headfield].hh.rh,0,1,0);
w:=mem[u+3].int;end;n:=0;if curspan<>curalign then{798:}
begin q:=curspan;repeat incr(n);q:=mem[mem[q].hh.rh].hh.rh;
until q=curalign;if n>511 then confusion(907);q:=curspan;
while mem[mem[q].hh.lh].hh.rh<n do q:=mem[q].hh.lh;
if mem[mem[q].hh.lh].hh.rh>n then begin s:=getnode(2);
mem[s].hh.lh:=mem[q].hh.lh;mem[s].hh.rh:=n;mem[q].hh.lh:=s;
mem[s+1].int:=w;
end else if mem[mem[q].hh.lh+1].int<w then mem[mem[q].hh.lh+1].int:=w;
end{:798}else if w>mem[curalign+1].int then mem[curalign+1].int:=w;
mem[u].hh.b0:=13;mem[u].hh.b1:=n;{659:}
if totalstretch[3]<>0 then o:=3 else if totalstretch[2]<>0 then o:=2
else if totalstretch[1]<>0 then o:=1 else o:=0{:659};mem[u+5].hh.b1:=o;
mem[u+6].int:=totalstretch[o];{665:}
if totalshrink[3]<>0 then o:=3 else if totalshrink[2]<>0 then o:=2 else
if totalshrink[1]<>0 then o:=1 else o:=0{:665};mem[u+5].hh.b0:=o;
mem[u+4].int:=totalshrink[o];popnest;mem[curlist.tailfield].hh.rh:=u;
curlist.tailfield:=u;end{:796};{795:}
begin mem[curlist.tailfield].hh.rh:=newglue(mem[mem[curalign].hh.rh+1].
hh.lh);curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
mem[curlist.tailfield].hh.b1:=12{:795};
if mem[curalign+5].hh.lh>=257 then begin fincol:=true;goto 10;end;
initspan(p);end;alignstate:=1000000;{406:}repeat getxtoken;
until curcmd<>10{:406};curalign:=p;initcol;fincol:=false;10:end;{:791}
{799:}procedure finrow;var p:halfword;
begin if curlist.modefield=-102 then begin p:=hpack(mem[curlist.
headfield].hh.rh,0,1);popnest;appendtovlist(p);
if curhead<>curtail then begin mem[curlist.tailfield].hh.rh:=mem[curhead
].hh.rh;curlist.tailfield:=curtail;end;
end else begin p:=vpackage(mem[curlist.headfield].hh.rh,0,1,1073741823);
popnest;mem[curlist.tailfield].hh.rh:=p;curlist.tailfield:=p;
curlist.auxfield.hh.lh:=1000;end;mem[p].hh.b0:=13;mem[p+6].int:=0;
if eqtb[10820].hh.rh<>0 then begintokenlist(eqtb[10820].hh.rh,13);
alignpeek;end;{:799}{800:}procedure doassignments;forward;
procedure resumeafterdisplay;forward;procedure buildpage;forward;
procedure finalign;var p,q,r,s,u,v:halfword;t,w:scaled;o:scaled;
n:halfword;rulesave:scaled;auxsave:memoryword;
begin if curgroup<>6 then confusion(908);unsave;
if curgroup<>6 then confusion(909);unsave;
if nest[nestptr-1].modefield=203 then o:=eqtb[13245].int else o:=0;
{801:}q:=mem[mem[memtop-8].hh.rh].hh.rh;repeat flushlist(mem[q+3].int);
flushlist(mem[q+2].int);p:=mem[mem[q].hh.rh].hh.rh;
if mem[q+1].int=-1073741824 then{802:}begin mem[q+1].int:=0;
r:=mem[q].hh.rh;s:=mem[r+1].hh.lh;if s<>0 then begin incr(mem[0].hh.rh);
deleteglueref(s);mem[r+1].hh.lh:=0;end;end{:802};
if mem[q].hh.lh<>memtop-9 then{803:}
begin t:=mem[q+1].int+mem[mem[mem[q].hh.rh+1].hh.lh+1].int;
r:=mem[q].hh.lh;s:=memtop-9;mem[s].hh.lh:=p;n:=1;
repeat mem[r+1].int:=mem[r+1].int-t;u:=mem[r].hh.lh;
while mem[r].hh.rh>n do begin s:=mem[s].hh.lh;
n:=mem[mem[s].hh.lh].hh.rh+1;end;
if mem[r].hh.rh<n then begin mem[r].hh.lh:=mem[s].hh.lh;mem[s].hh.lh:=r;
decr(mem[r].hh.rh);s:=r;
end else begin if mem[r+1].int>mem[mem[s].hh.lh+1].int then mem[mem[s].
hh.lh+1].int:=mem[r+1].int;freenode(r,2);end;r:=u;until r=memtop-9;
end{:803};mem[q].hh.b0:=13;mem[q].hh.b1:=0;mem[q+3].int:=0;
mem[q+2].int:=0;mem[q+5].hh.b1:=0;mem[q+5].hh.b0:=0;mem[q+6].int:=0;
mem[q+4].int:=0;q:=p;until q=0{:801};{804:}saveptr:=saveptr-2;
packbeginline:=-curlist.mlfield;
if curlist.modefield=-1 then begin rulesave:=eqtb[13246].int;
eqtb[13246].int:=0;
p:=hpack(mem[memtop-8].hh.rh,savestack[saveptr+1].int,savestack[saveptr
+0].int);eqtb[13246].int:=rulesave;
end else begin q:=mem[mem[memtop-8].hh.rh].hh.rh;
repeat mem[q+3].int:=mem[q+1].int;mem[q+1].int:=0;
q:=mem[mem[q].hh.rh].hh.rh;until q=0;
p:=vpackage(mem[memtop-8].hh.rh,savestack[saveptr+1].int,savestack[
saveptr+0].int,1073741823);q:=mem[mem[memtop-8].hh.rh].hh.rh;
repeat mem[q+1].int:=mem[q+3].int;mem[q+3].int:=0;
q:=mem[mem[q].hh.rh].hh.rh;until q=0;end;packbeginline:=0{:804};{805:}
q:=mem[curlist.headfield].hh.rh;s:=curlist.headfield;
while q<>0 do begin if not(q>=himemmin)then if mem[q].hh.b0=13 then{807:
}begin if curlist.modefield=-1 then begin mem[q].hh.b0:=0;
mem[q+1].int:=mem[p+1].int;end else begin mem[q].hh.b0:=1;
mem[q+3].int:=mem[p+3].int;end;mem[q+5].hh.b1:=mem[p+5].hh.b1;
mem[q+5].hh.b0:=mem[p+5].hh.b0;mem[q+6].gr:=mem[p+6].gr;mem[q+4].int:=o;
r:=mem[mem[q+5].hh.rh].hh.rh;s:=mem[mem[p+5].hh.rh].hh.rh;repeat{808:}
n:=mem[r].hh.b1;t:=mem[s+1].int;w:=t;u:=memtop-4;
while n>0 do begin decr(n);{809:}s:=mem[s].hh.rh;v:=mem[s+1].hh.lh;
mem[u].hh.rh:=newglue(v);u:=mem[u].hh.rh;mem[u].hh.b1:=12;
t:=t+mem[v+1].int;
if mem[p+5].hh.b0=1 then begin if mem[v].hh.b0=mem[p+5].hh.b1 then t:=t+
round(mem[p+6].gr*mem[v+2].int);
end else if mem[p+5].hh.b0=2 then begin if mem[v].hh.b1=mem[p+5].hh.b1
then t:=t-round(mem[p+6].gr*mem[v+3].int);end;s:=mem[s].hh.rh;
mem[u].hh.rh:=newnullbox;u:=mem[u].hh.rh;t:=t+mem[s+1].int;
if curlist.modefield=-1 then mem[u+1].int:=mem[s+1].int else begin mem[u
].hh.b0:=1;mem[u+3].int:=mem[s+1].int;end{:809};end;
if curlist.modefield=-1 then{810:}begin mem[r+3].int:=mem[q+3].int;
mem[r+2].int:=mem[q+2].int;
if t=mem[r+1].int then begin mem[r+5].hh.b0:=0;mem[r+5].hh.b1:=0;
mem[r+6].gr:=0.0;
end else if t>mem[r+1].int then begin mem[r+5].hh.b0:=1;
if mem[r+6].int=0 then mem[r+6].gr:=0.0 else mem[r+6].gr:=(t-mem[r+1].
int)/mem[r+6].int;end else begin mem[r+5].hh.b1:=mem[r+5].hh.b0;
mem[r+5].hh.b0:=2;
if mem[r+4].int=0 then mem[r+6].gr:=0.0 else if(mem[r+5].hh.b1=0)and(mem
[r+1].int-t>mem[r+4].int)then mem[r+6].gr:=1.0 else mem[r+6].gr:=(mem[r
+1].int-t)/mem[r+4].int;end;mem[r+1].int:=w;mem[r].hh.b0:=0;end{:810}
else{811:}begin mem[r+1].int:=mem[q+1].int;
if t=mem[r+3].int then begin mem[r+5].hh.b0:=0;mem[r+5].hh.b1:=0;
mem[r+6].gr:=0.0;
end else if t>mem[r+3].int then begin mem[r+5].hh.b0:=1;
if mem[r+6].int=0 then mem[r+6].gr:=0.0 else mem[r+6].gr:=(t-mem[r+3].
int)/mem[r+6].int;end else begin mem[r+5].hh.b1:=mem[r+5].hh.b0;
mem[r+5].hh.b0:=2;
if mem[r+4].int=0 then mem[r+6].gr:=0.0 else if(mem[r+5].hh.b1=0)and(mem
[r+3].int-t>mem[r+4].int)then mem[r+6].gr:=1.0 else mem[r+6].gr:=(mem[r
+3].int-t)/mem[r+4].int;end;mem[r+3].int:=w;mem[r].hh.b0:=1;end{:811};
mem[r+4].int:=0;if u<>memtop-4 then begin mem[u].hh.rh:=mem[r].hh.rh;
mem[r].hh.rh:=mem[memtop-4].hh.rh;r:=u;end{:808};
r:=mem[mem[r].hh.rh].hh.rh;s:=mem[mem[s].hh.rh].hh.rh;until r=0;
end{:807}else if mem[q].hh.b0=2 then{806:}
begin if(mem[q+1].int=-1073741824)then mem[q+1].int:=mem[p+1].int;
if(mem[q+3].int=-1073741824)then mem[q+3].int:=mem[p+3].int;
if(mem[q+2].int=-1073741824)then mem[q+2].int:=mem[p+2].int;
if o<>0 then begin r:=mem[q].hh.rh;mem[q].hh.rh:=0;q:=hpack(q,0,1);
mem[q+4].int:=o;mem[q].hh.rh:=r;mem[s].hh.rh:=q;end;end{:806};s:=q;
q:=mem[q].hh.rh;end{:805};flushnodelist(p);popalignment;{812:}
auxsave:=curlist.auxfield;p:=mem[curlist.headfield].hh.rh;
q:=curlist.tailfield;popnest;if curlist.modefield=203 then{1206:}
begin doassignments;if curcmd<>3 then{1207:}
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(1163);end;begin helpptr:=2;helpline[1]:=888;helpline[0]:=889;end;
backerror;end{:1207}else{1197:}begin getxtoken;
if curcmd<>3 then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(1159);end;begin helpptr:=2;helpline[1]:=1160;
helpline[0]:=1161;end;backerror;end;end{:1197};popnest;
begin mem[curlist.tailfield].hh.rh:=newpenalty(eqtb[12674].int);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
begin mem[curlist.tailfield].hh.rh:=newparamglue(3);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
mem[curlist.tailfield].hh.rh:=p;if p<>0 then curlist.tailfield:=q;
begin mem[curlist.tailfield].hh.rh:=newpenalty(eqtb[12675].int);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
begin mem[curlist.tailfield].hh.rh:=newparamglue(4);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
curlist.auxfield.int:=auxsave.int;resumeafterdisplay;end{:1206}
else begin curlist.auxfield:=auxsave;mem[curlist.tailfield].hh.rh:=p;
if p<>0 then curlist.tailfield:=q;if curlist.modefield=1 then buildpage;
end{:812};end;{785:}procedure alignpeek;label 20;
begin 20:alignstate:=1000000;{406:}repeat getxtoken;
until curcmd<>10{:406};if curcmd=34 then begin scanleftbrace;
newsavelevel(7);if curlist.modefield=-1 then normalparagraph;
end else if curcmd=2 then finalign else if(curcmd=5)and(curchr=258)then
goto 20 else begin initrow;initcol;end;end;{:785}{:800}{815:}{826:}
function finiteshrink(p:halfword):halfword;var q:halfword;
begin if noshrinkerroryet then begin noshrinkerroryet:=false;
begin if interaction=3 then wakeupterminal;printnl(262);print(910);end;
begin helpptr:=5;helpline[4]:=911;helpline[3]:=912;helpline[2]:=913;
helpline[1]:=914;helpline[0]:=915;end;error;end;q:=newspec(p);
mem[q].hh.b1:=0;deleteglueref(p);finiteshrink:=q;end;{:826}{829:}
procedure trybreak(pi:integer;breaktype:smallnumber);
label 10,30,31,22,60;var r:halfword;prevr:halfword;oldl:halfword;
nobreakyet:boolean;{830:}prevprevr:halfword;s:halfword;q:halfword;
v:halfword;t:integer;f:internalfontnumber;l:halfword;
noderstaysactive:boolean;linewidth:scaled;fitclass:0..3;b:halfword;
d:integer;artificialdemerits:boolean;savelink:halfword;shortfall:scaled;
{:830}begin{831:}
if abs(pi)>=10000 then if pi>0 then goto 10 else pi:=-10000{:831};
nobreakyet:=true;prevr:=memtop-7;oldl:=0;
curactivewidth[1]:=activewidth[1];curactivewidth[2]:=activewidth[2];
curactivewidth[3]:=activewidth[3];curactivewidth[4]:=activewidth[4];
curactivewidth[5]:=activewidth[5];curactivewidth[6]:=activewidth[6];
while true do begin 22:r:=mem[prevr].hh.rh;{832:}
if mem[r].hh.b0=2 then begin curactivewidth[1]:=curactivewidth[1]+mem[r
+1].int;curactivewidth[2]:=curactivewidth[2]+mem[r+2].int;
curactivewidth[3]:=curactivewidth[3]+mem[r+3].int;
curactivewidth[4]:=curactivewidth[4]+mem[r+4].int;
curactivewidth[5]:=curactivewidth[5]+mem[r+5].int;
curactivewidth[6]:=curactivewidth[6]+mem[r+6].int;prevprevr:=prevr;
prevr:=r;goto 22;end{:832};{835:}begin l:=mem[r+1].hh.lh;
if l>oldl then begin if(minimumdemerits<1073741823)and((oldl<>easyline)
or(r=memtop-7))then{836:}begin if nobreakyet then{837:}
begin nobreakyet:=false;breakwidth[1]:=background[1];
breakwidth[2]:=background[2];breakwidth[3]:=background[3];
breakwidth[4]:=background[4];breakwidth[5]:=background[5];
breakwidth[6]:=background[6];s:=curp;
if breaktype>0 then if curp<>0 then{840:}begin t:=mem[curp].hh.b1;
v:=curp;s:=mem[curp+1].hh.rh;while t>0 do begin decr(t);v:=mem[v].hh.rh;
{841:}if(v>=himemmin)then begin f:=mem[v].hh.b0;
breakwidth[1]:=breakwidth[1]-fontinfo[widthbase[f]+fontinfo[charbase[f]+
mem[v].hh.b1].qqqq.b0].int;
end else case mem[v].hh.b0 of 6:begin f:=mem[v+1].hh.b0;
breakwidth[1]:=breakwidth[1]-fontinfo[widthbase[f]+fontinfo[charbase[f]+
mem[v+1].hh.b1].qqqq.b0].int;end;
0,1,2,11:breakwidth[1]:=breakwidth[1]-mem[v+1].int;
others:confusion(916)end{:841};end;while s<>0 do begin{842:}
if(s>=himemmin)then begin f:=mem[s].hh.b0;
breakwidth[1]:=breakwidth[1]+fontinfo[widthbase[f]+fontinfo[charbase[f]+
mem[s].hh.b1].qqqq.b0].int;
end else case mem[s].hh.b0 of 6:begin f:=mem[s+1].hh.b0;
breakwidth[1]:=breakwidth[1]+fontinfo[widthbase[f]+fontinfo[charbase[f]+
mem[s+1].hh.b1].qqqq.b0].int;end;
0,1,2:breakwidth[1]:=breakwidth[1]+mem[s+1].int;
11:if(t=0)and(mem[s].hh.b1<>2)then t:=-1 else breakwidth[1]:=breakwidth[
1]+mem[s+1].int;others:confusion(917)end;incr(t){:842};s:=mem[s].hh.rh;
end;breakwidth[1]:=breakwidth[1]+discwidth;if t=0 then s:=mem[v].hh.rh;
end{:840};while s<>0 do begin if(s>=himemmin)then goto 30;
case mem[s].hh.b0 of 10:{838:}begin v:=mem[s+1].hh.lh;
breakwidth[1]:=breakwidth[1]-mem[v+1].int;
breakwidth[2+mem[v].hh.b0]:=breakwidth[2+mem[v].hh.b0]-mem[v+2].int;
breakwidth[6]:=breakwidth[6]-mem[v+3].int;end{:838};12:;
9,11:if mem[s].hh.b1=2 then goto 30 else breakwidth[1]:=breakwidth[1]-
mem[s+1].int;others:goto 30 end;s:=mem[s].hh.rh;end;30:end{:837};{843:}
if mem[prevr].hh.b0=2 then begin mem[prevr+1].int:=mem[prevr+1].int-
curactivewidth[1]+breakwidth[1];
mem[prevr+2].int:=mem[prevr+2].int-curactivewidth[2]+breakwidth[2];
mem[prevr+3].int:=mem[prevr+3].int-curactivewidth[3]+breakwidth[3];
mem[prevr+4].int:=mem[prevr+4].int-curactivewidth[4]+breakwidth[4];
mem[prevr+5].int:=mem[prevr+5].int-curactivewidth[5]+breakwidth[5];
mem[prevr+6].int:=mem[prevr+6].int-curactivewidth[6]+breakwidth[6];
end else if prevr=memtop-7 then begin activewidth[1]:=breakwidth[1];
activewidth[2]:=breakwidth[2];activewidth[3]:=breakwidth[3];
activewidth[4]:=breakwidth[4];activewidth[5]:=breakwidth[5];
activewidth[6]:=breakwidth[6];end else begin q:=getnode(7);
mem[q].hh.rh:=r;mem[q].hh.b0:=2;mem[q].hh.b1:=0;
mem[q+1].int:=breakwidth[1]-curactivewidth[1];
mem[q+2].int:=breakwidth[2]-curactivewidth[2];
mem[q+3].int:=breakwidth[3]-curactivewidth[3];
mem[q+4].int:=breakwidth[4]-curactivewidth[4];
mem[q+5].int:=breakwidth[5]-curactivewidth[5];
mem[q+6].int:=breakwidth[6]-curactivewidth[6];mem[prevr].hh.rh:=q;
prevprevr:=prevr;prevr:=q;end{:843};
if abs(eqtb[12679].int)>=1073741823-minimumdemerits then minimumdemerits
:=1073741822 else minimumdemerits:=minimumdemerits+abs(eqtb[12679].int);
for fitclass:=0 to 3 do begin if minimaldemerits[fitclass]<=
minimumdemerits then{845:}begin q:=getnode(2);mem[q].hh.rh:=passive;
passive:=q;mem[q+1].hh.rh:=curp;ifdef('STAT')incr(passnumber);
mem[q].hh.lh:=passnumber;
endif('STAT')mem[q+1].hh.lh:=bestplace[fitclass];q:=getnode(3);
mem[q+1].hh.rh:=passive;mem[q+1].hh.lh:=bestplline[fitclass]+1;
mem[q].hh.b1:=fitclass;mem[q].hh.b0:=breaktype;
mem[q+2].int:=minimaldemerits[fitclass];mem[q].hh.rh:=r;
mem[prevr].hh.rh:=q;prevr:=q;
ifdef('STAT')if eqtb[12695].int>0 then{846:}begin printnl(918);
printint(mem[passive].hh.lh);print(919);printint(mem[q+1].hh.lh-1);
printchar(46);printint(fitclass);if breaktype=1 then printchar(45);
print(920);printint(mem[q+2].int);print(921);
if mem[passive+1].hh.lh=0 then printchar(48)else printint(mem[mem[
passive+1].hh.lh].hh.lh);end{:846};endif('STAT')end{:845};
minimaldemerits[fitclass]:=1073741823;end;minimumdemerits:=1073741823;
{844:}if r<>memtop-7 then begin q:=getnode(7);mem[q].hh.rh:=r;
mem[q].hh.b0:=2;mem[q].hh.b1:=0;
mem[q+1].int:=curactivewidth[1]-breakwidth[1];
mem[q+2].int:=curactivewidth[2]-breakwidth[2];
mem[q+3].int:=curactivewidth[3]-breakwidth[3];
mem[q+4].int:=curactivewidth[4]-breakwidth[4];
mem[q+5].int:=curactivewidth[5]-breakwidth[5];
mem[q+6].int:=curactivewidth[6]-breakwidth[6];mem[prevr].hh.rh:=q;
prevprevr:=prevr;prevr:=q;end{:844};end{:836};
if r=memtop-7 then goto 10;{850:}
if l>easyline then begin linewidth:=secondwidth;oldl:=262142;
end else begin oldl:=l;
if l>lastspecialline then linewidth:=secondwidth else if eqtb[10812].hh.
rh=0 then linewidth:=firstwidth else linewidth:=mem[eqtb[10812].hh.rh+2*
l].int;end{:850};end;end{:835};{851:}begin artificialdemerits:=false;
shortfall:=linewidth-curactivewidth[1];if shortfall>0 then{852:}
if(curactivewidth[3]<>0)or(curactivewidth[4]<>0)or(curactivewidth[5]<>0)
then begin b:=0;fitclass:=2;
end else begin if shortfall>7230584 then if curactivewidth[2]<1663497
then begin b:=10000;fitclass:=0;goto 31;end;
b:=badness(shortfall,curactivewidth[2]);
if b>12 then if b>99 then fitclass:=0 else fitclass:=1 else fitclass:=2;
31:end{:852}else{853:}
begin if-shortfall>curactivewidth[6]then b:=10001 else b:=badness(-
shortfall,curactivewidth[6]);if b>12 then fitclass:=3 else fitclass:=2;
end{:853};if(b>10000)or(pi=-10000)then{854:}
begin if finalpass and(minimumdemerits=1073741823)and(mem[r].hh.rh=
memtop-7)and(prevr=memtop-7)then artificialdemerits:=true else if b>
threshold then goto 60;noderstaysactive:=false;end{:854}
else begin prevr:=r;if b>threshold then goto 22;noderstaysactive:=true;
end;{855:}if artificialdemerits then d:=0 else{859:}
begin d:=eqtb[12665].int+b;
if abs(d)>=10000 then d:=100000000 else d:=d*d;
if pi<>0 then if pi>0 then d:=d+pi*pi else if pi>-10000 then d:=d-pi*pi;
if(breaktype=1)and(mem[r].hh.b0=1)then if curp<>0 then d:=d+eqtb[12677].
int else d:=d+eqtb[12678].int;
if abs(toint(fitclass)-toint(mem[r].hh.b1))>1 then d:=d+eqtb[12679].int;
end{:859};ifdef('STAT')if eqtb[12695].int>0 then{856:}
begin if printednode<>curp then{857:}begin printnl(335);
if curp=0 then shortdisplay(mem[printednode].hh.rh)else begin savelink:=
mem[curp].hh.rh;mem[curp].hh.rh:=0;printnl(335);
shortdisplay(mem[printednode].hh.rh);mem[curp].hh.rh:=savelink;end;
printednode:=curp;end{:857};printnl(64);
if curp=0 then printesc(593)else if mem[curp].hh.b0<>10 then begin if
mem[curp].hh.b0=12 then printesc(527)else if mem[curp].hh.b0=7 then
printesc(346)else if mem[curp].hh.b0=11 then printesc(337)else printesc(
340);end;print(922);
if mem[r+1].hh.rh=0 then printchar(48)else printint(mem[mem[r+1].hh.rh].
hh.lh);print(923);if b>10000 then printchar(42)else printint(b);
print(924);printint(pi);print(925);
if artificialdemerits then printchar(42)else printint(d);end{:856};
endif('STAT')d:=d+mem[r+2].int;
if d<=minimaldemerits[fitclass]then begin minimaldemerits[fitclass]:=d;
bestplace[fitclass]:=mem[r+1].hh.rh;bestplline[fitclass]:=l;
if d<minimumdemerits then minimumdemerits:=d;end{:855};
if noderstaysactive then goto 22;60:{860:}
mem[prevr].hh.rh:=mem[r].hh.rh;freenode(r,3);
if prevr=memtop-7 then{861:}begin r:=mem[memtop-7].hh.rh;
if mem[r].hh.b0=2 then begin activewidth[1]:=activewidth[1]+mem[r+1].int
;activewidth[2]:=activewidth[2]+mem[r+2].int;
activewidth[3]:=activewidth[3]+mem[r+3].int;
activewidth[4]:=activewidth[4]+mem[r+4].int;
activewidth[5]:=activewidth[5]+mem[r+5].int;
activewidth[6]:=activewidth[6]+mem[r+6].int;
curactivewidth[1]:=activewidth[1];curactivewidth[2]:=activewidth[2];
curactivewidth[3]:=activewidth[3];curactivewidth[4]:=activewidth[4];
curactivewidth[5]:=activewidth[5];curactivewidth[6]:=activewidth[6];
mem[memtop-7].hh.rh:=mem[r].hh.rh;freenode(r,7);end;end{:861}
else if mem[prevr].hh.b0=2 then begin r:=mem[prevr].hh.rh;
if r=memtop-7 then begin curactivewidth[1]:=curactivewidth[1]-mem[prevr
+1].int;curactivewidth[2]:=curactivewidth[2]-mem[prevr+2].int;
curactivewidth[3]:=curactivewidth[3]-mem[prevr+3].int;
curactivewidth[4]:=curactivewidth[4]-mem[prevr+4].int;
curactivewidth[5]:=curactivewidth[5]-mem[prevr+5].int;
curactivewidth[6]:=curactivewidth[6]-mem[prevr+6].int;
mem[prevprevr].hh.rh:=memtop-7;freenode(prevr,7);prevr:=prevprevr;
end else if mem[r].hh.b0=2 then begin curactivewidth[1]:=curactivewidth[
1]+mem[r+1].int;curactivewidth[2]:=curactivewidth[2]+mem[r+2].int;
curactivewidth[3]:=curactivewidth[3]+mem[r+3].int;
curactivewidth[4]:=curactivewidth[4]+mem[r+4].int;
curactivewidth[5]:=curactivewidth[5]+mem[r+5].int;
curactivewidth[6]:=curactivewidth[6]+mem[r+6].int;
mem[prevr+1].int:=mem[prevr+1].int+mem[r+1].int;
mem[prevr+2].int:=mem[prevr+2].int+mem[r+2].int;
mem[prevr+3].int:=mem[prevr+3].int+mem[r+3].int;
mem[prevr+4].int:=mem[prevr+4].int+mem[r+4].int;
mem[prevr+5].int:=mem[prevr+5].int+mem[r+5].int;
mem[prevr+6].int:=mem[prevr+6].int+mem[r+6].int;
mem[prevr].hh.rh:=mem[r].hh.rh;freenode(r,7);end;end{:860};end{:851};
end;10:ifdef('STAT'){858:}
if curp=printednode then if curp<>0 then if mem[curp].hh.b0=7 then begin
t:=mem[curp].hh.b1;while t>0 do begin decr(t);
printednode:=mem[printednode].hh.rh;end;end{:858}endif('STAT')end;{:829}
{877:}procedure postlinebreak(finalwidowpenalty:integer);label 30,31;
var q,r,s:halfword;discbreak:boolean;postdiscbreak:boolean;
curwidth:scaled;curindent:scaled;t:quarterword;pen:integer;
curline:halfword;begin{878:}q:=mem[bestbet+1].hh.rh;curp:=0;repeat r:=q;
q:=mem[q+1].hh.lh;mem[r+1].hh.lh:=curp;curp:=r;until q=0{:878};
curline:=curlist.pgfield+1;repeat{880:}{881:}q:=mem[curp+1].hh.rh;
discbreak:=false;postdiscbreak:=false;
if q<>0 then if mem[q].hh.b0=10 then begin deleteglueref(mem[q+1].hh.lh)
;mem[q+1].hh.lh:=eqtb[10290].hh.rh;mem[q].hh.b1:=9;
incr(mem[eqtb[10290].hh.rh].hh.rh);goto 30;
end else begin if mem[q].hh.b0=7 then{882:}begin t:=mem[q].hh.b1;{883:}
if t=0 then r:=mem[q].hh.rh else begin r:=q;
while t>1 do begin r:=mem[r].hh.rh;decr(t);end;s:=mem[r].hh.rh;
if not(s>=himemmin)then if mem[curp+1].hh.lh<>0 then if mem[mem[curp+1].
hh.lh+1].hh.rh=s then s:=r;r:=mem[s].hh.rh;mem[s].hh.rh:=0;
flushnodelist(mem[q].hh.rh);mem[q].hh.b1:=0;end{:883};
if mem[q+1].hh.rh<>0 then{884:}begin s:=mem[q+1].hh.rh;
while mem[s].hh.rh<>0 do s:=mem[s].hh.rh;mem[s].hh.rh:=r;
r:=mem[q+1].hh.rh;mem[q+1].hh.rh:=0;postdiscbreak:=true;end{:884};
if mem[q+1].hh.lh<>0 then{885:}begin s:=mem[q+1].hh.lh;mem[q].hh.rh:=s;
while mem[s].hh.rh<>0 do s:=mem[s].hh.rh;mem[q+1].hh.lh:=0;q:=s;
end{:885};mem[q].hh.rh:=r;discbreak:=true;end{:882}
else if(mem[q].hh.b0=9)or(mem[q].hh.b0=11)then mem[q+1].int:=0;
end else begin q:=memtop-3;while mem[q].hh.rh<>0 do q:=mem[q].hh.rh;end;
{886:}r:=newparamglue(8);mem[r].hh.rh:=mem[q].hh.rh;mem[q].hh.rh:=r;
q:=r{:886};30:{:881};{887:}r:=mem[q].hh.rh;mem[q].hh.rh:=0;
q:=mem[memtop-3].hh.rh;mem[memtop-3].hh.rh:=r;
if eqtb[10289].hh.rh<>0 then begin r:=newparamglue(7);mem[r].hh.rh:=q;
q:=r;end{:887};{889:}
if curline>lastspecialline then begin curwidth:=secondwidth;
curindent:=secondindent;
end else if eqtb[10812].hh.rh=0 then begin curwidth:=firstwidth;
curindent:=firstindent;
end else begin curwidth:=mem[eqtb[10812].hh.rh+2*curline].int;
curindent:=mem[eqtb[10812].hh.rh+2*curline-1].int;end;
adjusttail:=memtop-5;justbox:=hpack(q,curwidth,0);
mem[justbox+4].int:=curindent{:889};{888:}appendtovlist(justbox);
if memtop-5<>adjusttail then begin mem[curlist.tailfield].hh.rh:=mem[
memtop-5].hh.rh;curlist.tailfield:=adjusttail;end;adjusttail:=0{:888};
{890:}if curline+1<>bestline then begin pen:=eqtb[12676].int;
if curline=curlist.pgfield+1 then pen:=pen+eqtb[12668].int;
if curline+2=bestline then pen:=pen+finalwidowpenalty;
if discbreak then pen:=pen+eqtb[12671].int;
if pen<>0 then begin r:=newpenalty(pen);mem[curlist.tailfield].hh.rh:=r;
curlist.tailfield:=r;end;end{:890}{:880};incr(curline);
curp:=mem[curp+1].hh.lh;if curp<>0 then if not postdiscbreak then{879:}
begin r:=memtop-3;while true do begin q:=mem[r].hh.rh;
if q=mem[curp+1].hh.rh then goto 31;if(q>=himemmin)then goto 31;
if(mem[q].hh.b0<9)then goto 31;
if mem[q].hh.b1=2 then if mem[q].hh.b0=11 then goto 31;r:=q;end;
31:if r<>memtop-3 then begin mem[r].hh.rh:=0;
flushnodelist(mem[memtop-3].hh.rh);mem[memtop-3].hh.rh:=q;end;end{:879};
until curp=0;
if(curline<>bestline)or(mem[memtop-3].hh.rh<>0)then confusion(932);
curlist.pgfield:=bestline-1;end;{:877}{895:}{906:}
function reconstitute(j,n:smallnumber;bchar,hchar:halfword):smallnumber;
label 22,30;var p:halfword;t:halfword;q:fourquarters;currh:halfword;
testchar:halfword;w:scaled;k:fontindex;begin hyphenpassed:=0;
t:=memtop-4;w:=0;mem[memtop-4].hh.rh:=0;{908:}curl:=hu[j];curq:=t;
if j=0 then begin ligaturepresent:=initlig;p:=initlist;
if ligaturepresent then lfthit:=initlft;
while p>0 do begin begin mem[t].hh.rh:=getavail;t:=mem[t].hh.rh;
mem[t].hh.b0:=hf;mem[t].hh.b1:=mem[p].hh.b1;end;p:=mem[p].hh.rh;end;
end else if curl<256 then begin mem[t].hh.rh:=getavail;t:=mem[t].hh.rh;
mem[t].hh.b0:=hf;mem[t].hh.b1:=curl;end;ligstack:=0;
begin if j<n then curr:=hu[j+1]else curr:=bchar;
if odd(hyf[j])then currh:=hchar else currh:=256;end{:908};22:{909:}
if curl=256 then begin k:=bcharlabel[hf];
if k=fontmemsize then goto 30 else q:=fontinfo[k].qqqq;
end else begin q:=fontinfo[charbase[hf]+curl].qqqq;
if((q.b2)mod 4)<>1 then goto 30;k:=ligkernbase[hf]+q.b3;
q:=fontinfo[k].qqqq;
if q.b0>128 then begin k:=ligkernbase[hf]+256*q.b2+q.b3+32768-256*(128);
q:=fontinfo[k].qqqq;end;end;
if currh<256 then testchar:=currh else testchar:=curr;
while true do begin if q.b1=testchar then if q.b0<=128 then if currh<256
then begin hyphenpassed:=j;hchar:=256;currh:=256;goto 22;
end else begin if hchar<256 then if odd(hyf[j])then begin hyphenpassed:=
j;hchar:=256;end;if q.b2<128 then{911:}
begin if curl=256 then lfthit:=true;
if j=n then if ligstack=0 then rthit:=true;
begin if interrupt<>0 then pauseforinstructions;end;
case q.b2 of 1,5:begin curl:=q.b3;ligaturepresent:=true;end;
2,6:begin curr:=q.b3;
if ligstack>0 then mem[ligstack].hh.b1:=curr else begin ligstack:=
newligitem(curr);if j=n then bchar:=256 else begin p:=getavail;
mem[ligstack+1].hh.rh:=p;mem[p].hh.b1:=hu[j+1];mem[p].hh.b0:=hf;end;end;
end;3:begin curr:=q.b3;p:=ligstack;ligstack:=newligitem(curr);
mem[ligstack].hh.rh:=p;end;
7,11:begin if ligaturepresent then begin p:=newligature(hf,curl,mem[curq
].hh.rh);if lfthit then begin mem[p].hh.b1:=2;lfthit:=false;end;
if false then if ligstack=0 then begin incr(mem[p].hh.b1);rthit:=false;
end;mem[curq].hh.rh:=p;t:=p;ligaturepresent:=false;end;curq:=t;
curl:=q.b3;ligaturepresent:=true;end;others:begin curl:=q.b3;
ligaturepresent:=true;
if ligstack>0 then begin if mem[ligstack+1].hh.rh>0 then begin mem[t].hh
.rh:=mem[ligstack+1].hh.rh;t:=mem[t].hh.rh;incr(j);end;p:=ligstack;
ligstack:=mem[p].hh.rh;freenode(p,2);
if ligstack=0 then begin if j<n then curr:=hu[j+1]else curr:=bchar;
if odd(hyf[j])then currh:=hchar else currh:=256;
end else curr:=mem[ligstack].hh.b1;
end else if j=n then goto 30 else begin begin mem[t].hh.rh:=getavail;
t:=mem[t].hh.rh;mem[t].hh.b0:=hf;mem[t].hh.b1:=curr;end;incr(j);
begin if j<n then curr:=hu[j+1]else curr:=bchar;
if odd(hyf[j])then currh:=hchar else currh:=256;end;end;end end;
if q.b2>4 then if q.b2<>7 then goto 30;goto 22;end{:911};
w:=fontinfo[kernbase[hf]+256*q.b2+q.b3].int;goto 30;end;
if q.b0>=128 then if currh=256 then goto 30 else begin currh:=256;
goto 22;end;k:=k+q.b0+1;q:=fontinfo[k].qqqq;end;30:{:909};{910:}
if ligaturepresent then begin p:=newligature(hf,curl,mem[curq].hh.rh);
if lfthit then begin mem[p].hh.b1:=2;lfthit:=false;end;
if rthit then if ligstack=0 then begin incr(mem[p].hh.b1);rthit:=false;
end;mem[curq].hh.rh:=p;t:=p;ligaturepresent:=false;end;
if w<>0 then begin mem[t].hh.rh:=newkern(w);t:=mem[t].hh.rh;w:=0;end;
if ligstack>0 then begin curq:=t;curl:=mem[ligstack].hh.b1;
ligaturepresent:=true;
begin if mem[ligstack+1].hh.rh>0 then begin mem[t].hh.rh:=mem[ligstack+1
].hh.rh;t:=mem[t].hh.rh;incr(j);end;p:=ligstack;ligstack:=mem[p].hh.rh;
freenode(p,2);
if ligstack=0 then begin if j<n then curr:=hu[j+1]else curr:=bchar;
if odd(hyf[j])then currh:=hchar else currh:=256;
end else curr:=mem[ligstack].hh.b1;end;goto 22;end{:910};
reconstitute:=j;end;{:906}procedure hyphenate;
label 50,30,40,41,42,45,10;var{901:}i,j,l:0..65;q,r,s:halfword;
bchar:halfword;{:901}{912:}majortail,minortail:halfword;c:ASCIIcode;
cloc:0..63;rcount:integer;hyfnode:halfword;{:912}{922:}z:triepointer;
v:integer;{:922}{929:}h:hyphpointer;k:strnumber;u:poolpointer;{:929}
begin{923:}for j:=0 to hn do hyf[j]:=0;{930:}h:=hc[1];incr(hn);
hc[hn]:=curlang;for j:=2 to hn do h:=(h+h+hc[j])mod 607;
while true do begin{931:}k:=hyphword[h];if k=0 then goto 45;
if(strstart[k+1]-strstart[k])<hn then goto 45;
if(strstart[k+1]-strstart[k])=hn then begin j:=1;u:=strstart[k];
repeat if strpool[u]<hc[j]then goto 45;if strpool[u]>hc[j]then goto 30;
incr(j);incr(u);until j>hn;{932:}s:=hyphlist[h];
while s<>0 do begin hyf[mem[s].hh.lh]:=1;s:=mem[s].hh.rh;end{:932};
decr(hn);goto 40;end;30:{:931};if h>0 then decr(h)else h:=607;end;
45:decr(hn){:930};if trie[curlang+1].b1<>curlang then goto 10;hc[0]:=0;
hc[hn+1]:=0;hc[hn+2]:=256;
for j:=0 to hn-rhyf+1 do begin z:=trie[curlang+1].rh+hc[j];l:=j;
while hc[l]=trie[z].b1 do begin if trie[z].b0<>0 then{924:}
begin v:=trie[z].b0;repeat v:=v+opstart[curlang];i:=l-hyfdistance[v];
if hyfnum[v]>hyf[i]then hyf[i]:=hyfnum[v];v:=hyfnext[v];until v=0;
end{:924};incr(l);z:=trie[z].rh+hc[l];end;end;
40:for j:=0 to lhyf-1 do hyf[j]:=0;
for j:=0 to rhyf-1 do hyf[hn-j]:=0{:923};{902:}
for j:=lhyf to hn-rhyf do if odd(hyf[j])then goto 41;goto 10;41:{:902};
{903:}q:=mem[hb].hh.rh;mem[hb].hh.rh:=0;r:=mem[ha].hh.rh;
mem[ha].hh.rh:=0;bchar:=256;
if not(hb>=himemmin)then if mem[hb].hh.b0=6 then if odd(mem[hb].hh.b1)
then bchar:=fontbchar[hf];
if(ha>=himemmin)then if mem[ha].hh.b0<>hf then goto 42 else begin
initlist:=ha;initlig:=false;hu[0]:=mem[ha].hh.b1;
end else if mem[ha].hh.b0=6 then if mem[ha+1].hh.b0<>hf then goto 42
else begin initlist:=mem[ha+1].hh.rh;initlig:=true;
initlft:=(mem[ha].hh.b1>1);hu[0]:=mem[ha+1].hh.b1;
if initlist=0 then if initlft then begin hu[0]:=256;initlig:=false;end;
freenode(ha,2);
end else begin if not(r>=himemmin)then if mem[r].hh.b0=6 then if mem[r].
hh.b1>1 then goto 42;j:=1;s:=ha;initlist:=0;goto 50;end;s:=curp;
while mem[s].hh.rh<>ha do s:=mem[s].hh.rh;j:=0;goto 50;42:s:=ha;j:=0;
hu[0]:=256;initlig:=false;initlist:=0;50:flushnodelist(r);{913:}
repeat l:=j;j:=reconstitute(j,hn,bchar,hyfchar)+1;
if hyphenpassed=0 then begin mem[s].hh.rh:=mem[memtop-4].hh.rh;
while mem[s].hh.rh>0 do s:=mem[s].hh.rh;if odd(hyf[j-1])then begin l:=j;
hyphenpassed:=j-1;mem[memtop-4].hh.rh:=0;end;end;
if hyphenpassed>0 then{914:}repeat r:=getnode(2);
mem[r].hh.rh:=mem[memtop-4].hh.rh;mem[r].hh.b0:=7;majortail:=r;
rcount:=0;
while mem[majortail].hh.rh>0 do begin majortail:=mem[majortail].hh.rh;
incr(rcount);end;i:=hyphenpassed;hyf[i]:=0;{915:}minortail:=0;
mem[r+1].hh.lh:=0;hyfnode:=newcharacter(hf,hyfchar);
if hyfnode<>0 then begin incr(i);c:=hu[i];hu[i]:=hyfchar;
begin mem[hyfnode].hh.rh:=avail;avail:=hyfnode;
ifdef('STAT')decr(dynused);endif('STAT')end;end;
while l<=i do begin l:=reconstitute(l,i,fontbchar[hf],256)+1;
if mem[memtop-4].hh.rh>0 then begin if minortail=0 then mem[r+1].hh.lh:=
mem[memtop-4].hh.rh else mem[minortail].hh.rh:=mem[memtop-4].hh.rh;
minortail:=mem[memtop-4].hh.rh;
while mem[minortail].hh.rh>0 do minortail:=mem[minortail].hh.rh;end;end;
if hyfnode<>0 then begin hu[i]:=c;l:=i;decr(i);end{:915};{916:}
minortail:=0;mem[r+1].hh.rh:=0;cloc:=0;
if bcharlabel[hf]<fontmemsize then begin decr(l);c:=hu[l];cloc:=l;
hu[l]:=256;end;
while l<j do begin repeat l:=reconstitute(l,hn,bchar,256)+1;
if cloc>0 then begin hu[cloc]:=c;cloc:=0;end;
if mem[memtop-4].hh.rh>0 then begin if minortail=0 then mem[r+1].hh.rh:=
mem[memtop-4].hh.rh else mem[minortail].hh.rh:=mem[memtop-4].hh.rh;
minortail:=mem[memtop-4].hh.rh;
while mem[minortail].hh.rh>0 do minortail:=mem[minortail].hh.rh;end;
until l>=j;while l>j do{917:}begin j:=reconstitute(j,hn,bchar,256)+1;
mem[majortail].hh.rh:=mem[memtop-4].hh.rh;
while mem[majortail].hh.rh>0 do begin majortail:=mem[majortail].hh.rh;
incr(rcount);end;end{:917};end{:916};{918:}
if rcount>127 then begin mem[s].hh.rh:=mem[r].hh.rh;mem[r].hh.rh:=0;
flushnodelist(r);end else begin mem[s].hh.rh:=r;mem[r].hh.b1:=rcount;
end;s:=majortail{:918};hyphenpassed:=j-1;mem[memtop-4].hh.rh:=0;
until not odd(hyf[j-1]){:914};until j>hn;mem[s].hh.rh:=q{:913};
flushlist(initlist){:903};10:end;{:895}{942:}ifdef('INITEX'){944:}
function newtrieop(d,n:smallnumber;v:quarterword):quarterword;label 10;
var h:negtrieopsize..trieopsize;u:quarterword;l:0..trieopsize;
begin h:=abs(toint(n)+313*toint(d)+361*toint(v)+1009*toint(curlang))mod(
trieopsize+trieopsize)-trieopsize;while true do begin l:=trieophash[h];
if l=0 then begin if trieopptr=trieopsize then overflow(942,trieopsize);
u:=trieused[curlang];if u=511 then overflow(943,511);incr(trieopptr);
incr(u);trieused[curlang]:=u;hyfdistance[trieopptr]:=d;
hyfnum[trieopptr]:=n;hyfnext[trieopptr]:=v;
trieoplang[trieopptr]:=curlang;trieophash[h]:=trieopptr;
trieopval[trieopptr]:=u;newtrieop:=u;goto 10;end;
if(hyfdistance[l]=d)and(hyfnum[l]=n)and(hyfnext[l]=v)and(trieoplang[l]=
curlang)then begin newtrieop:=trieopval[l];goto 10;end;
if h>-trieopsize then decr(h)else h:=trieopsize;end;10:end;{:944}{948:}
function trienode(p:triepointer):triepointer;label 10;var h:triepointer;
q:triepointer;
begin h:=abs(toint(triec[p])+1009*toint(trieo[p])+2718*toint(triel[p])
+3142*toint(trier[p]))mod triesize;while true do begin q:=triehash[h];
if q=0 then begin triehash[h]:=p;trienode:=p;goto 10;end;
if(triec[q]=triec[p])and(trieo[q]=trieo[p])and(triel[q]=triel[p])and(
trier[q]=trier[p])then begin trienode:=q;goto 10;end;
if h>0 then decr(h)else h:=triesize;end;10:end;{:948}{949:}
function compresstrie(p:triepointer):triepointer;
begin if p=0 then compresstrie:=0 else begin triel[p]:=compresstrie(
triel[p]);trier[p]:=compresstrie(trier[p]);compresstrie:=trienode(p);
end;end;{:949}{953:}procedure firstfit(p:triepointer);label 45,40;
var h:triepointer;z:triepointer;q:triepointer;c:ASCIIcode;
l,r:triepointer;ll:1..256;begin c:=triec[p];z:=triemin[c];
while true do begin h:=z-c;{954:}
if triemax<h+256 then begin if triesize<=h+256 then overflow(944,
triesize);repeat incr(triemax);trietaken[triemax]:=false;
trie[triemax].rh:=triemax+1;trie[triemax].lh:=triemax-1;
until triemax=h+256;end{:954};if trietaken[h]then goto 45;{955:}
q:=trier[p];while q>0 do begin if trie[h+triec[q]].rh=0 then goto 45;
q:=trier[q];end;goto 40{:955};45:z:=trie[z].rh;end;40:{956:}
trietaken[h]:=true;triehash[p]:=h;q:=p;repeat z:=h+triec[q];
l:=trie[z].lh;r:=trie[z].rh;trie[r].lh:=l;trie[l].rh:=r;trie[z].rh:=0;
if l<256 then begin if z<256 then ll:=z else ll:=256;
repeat triemin[l]:=r;incr(l);until l=ll;end;q:=trier[q];until q=0{:956};
end;{:953}{957:}procedure triepack(p:triepointer);var q:triepointer;
begin repeat q:=triel[p];
if(q>0)and(triehash[q]=0)then begin firstfit(q);triepack(q);end;
p:=trier[p];until p=0;end;{:957}{959:}procedure triefix(p:triepointer);
var q:triepointer;c:ASCIIcode;z:triepointer;begin z:=triehash[p];
repeat q:=triel[p];c:=triec[p];trie[z+c].rh:=triehash[q];
trie[z+c].b1:=c;trie[z+c].b0:=trieo[p];if q>0 then triefix(q);
p:=trier[p];until p=0;end;{:959}{960:}procedure newpatterns;label 30,31;
var k,l:smallnumber;digitsensed:boolean;v:quarterword;p,q:triepointer;
firstchild:boolean;c:ASCIIcode;
begin if trienotready then begin if eqtb[12713].int<=0 then curlang:=0
else if eqtb[12713].int>255 then curlang:=0 else curlang:=eqtb[12713].
int;scanleftbrace;{961:}k:=0;hyf[0]:=0;digitsensed:=false;
while true do begin getxtoken;case curcmd of 11,12:{962:}
if digitsensed or(curchr<48)or(curchr>57)then begin if curchr=46 then
curchr:=0 else begin curchr:=eqtb[11639+curchr].hh.rh;
if curchr=0 then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(950);end;begin helpptr:=1;helpline[0]:=949;end;error;
end;end;if k<63 then begin incr(k);hc[k]:=curchr;hyf[k]:=0;
digitsensed:=false;end;end else if k<63 then begin hyf[k]:=curchr-48;
digitsensed:=true;end{:962};10,2:begin if k>0 then{963:}begin{965:}
if hc[1]=0 then hyf[0]:=0;if hc[k]=0 then hyf[k]:=0;l:=k;v:=0;
while true do begin if hyf[l]<>0 then v:=newtrieop(k-l,hyf[l],v);
if l>0 then decr(l)else goto 31;end;31:{:965};q:=0;hc[0]:=curlang;
while l<=k do begin c:=hc[l];incr(l);p:=triel[q];firstchild:=true;
while(p>0)and(c>triec[p])do begin q:=p;p:=trier[q];firstchild:=false;
end;if(p=0)or(c<triec[p])then{964:}
begin if trieptr=triesize then overflow(944,triesize);incr(trieptr);
trier[trieptr]:=p;p:=trieptr;triel[p]:=0;
if firstchild then triel[q]:=p else trier[q]:=p;triec[p]:=c;trieo[p]:=0;
end{:964};q:=p;end;
if trieo[q]<>0 then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(951);end;begin helpptr:=1;helpline[0]:=949;end;error;
end;trieo[q]:=v;end{:963};if curcmd=2 then goto 30;k:=0;hyf[0]:=0;
digitsensed:=false;end;
others:begin begin if interaction=3 then wakeupterminal;printnl(262);
print(948);end;printesc(946);begin helpptr:=1;helpline[0]:=949;end;
error;end end;end;30:{:961};
end else begin begin if interaction=3 then wakeupterminal;printnl(262);
print(945);end;printesc(946);begin helpptr:=1;helpline[0]:=947;end;
error;mem[memtop-12].hh.rh:=scantoks(false,false);flushlist(defref);end;
end;{:960}{966:}procedure inittrie;var p:triepointer;j,k,t:integer;
r,s:triepointer;h:twohalves;begin{952:}{945:}opstart[0]:=-0;
for j:=1 to 255 do opstart[j]:=opstart[j-1]+trieused[j-1];
for j:=1 to trieopptr do trieophash[j]:=opstart[trieoplang[j]]+trieopval
[j];
for j:=1 to trieopptr do while trieophash[j]>j do begin k:=trieophash[j]
;t:=hyfdistance[k];hyfdistance[k]:=hyfdistance[j];hyfdistance[j]:=t;
t:=hyfnum[k];hyfnum[k]:=hyfnum[j];hyfnum[j]:=t;t:=hyfnext[k];
hyfnext[k]:=hyfnext[j];hyfnext[j]:=t;trieophash[j]:=trieophash[k];
trieophash[k]:=k;end{:945};for p:=0 to triesize do triehash[p]:=0;
triel[0]:=compresstrie(triel[0]);for p:=0 to trieptr do triehash[p]:=0;
for p:=0 to 255 do triemin[p]:=p+1;trie[0].rh:=1;triemax:=0{:952};
if triel[0]<>0 then begin firstfit(triel[0]);triepack(triel[0]);end;
{958:}h.rh:=0;h.b0:=0;h.b1:=0;
if triel[0]=0 then begin for r:=0 to 256 do trie[r]:=h;triemax:=256;
end else begin triefix(triel[0]);r:=0;repeat s:=trie[r].rh;trie[r]:=h;
r:=s;until r>triemax;end;trie[0].b1:=63;{:958};trienotready:=false;end;
{:966}endif('INITEX'){:942}
procedure linebreak(finalwidowpenalty:integer);
label 30,31,32,33,34,35,22;var{862:}autobreaking:boolean;prevp:halfword;
q,r,s,prevs:halfword;f:internalfontnumber;{:862}{893:}j:smallnumber;
c:0..255;{:893}begin packbeginline:=curlist.mlfield;{816:}
mem[memtop-3].hh.rh:=mem[curlist.headfield].hh.rh;
if(curlist.tailfield>=himemmin)then begin mem[curlist.tailfield].hh.rh:=
newpenalty(10000);curlist.tailfield:=mem[curlist.tailfield].hh.rh;
end else if mem[curlist.tailfield].hh.b0<>10 then begin mem[curlist.
tailfield].hh.rh:=newpenalty(10000);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;
end else begin mem[curlist.tailfield].hh.b0:=12;
deleteglueref(mem[curlist.tailfield+1].hh.lh);
flushnodelist(mem[curlist.tailfield+1].hh.rh);
mem[curlist.tailfield+1].int:=10000;end;
mem[curlist.tailfield].hh.rh:=newparamglue(14);popnest;{:816}{827:}
noshrinkerroryet:=true;
if(mem[eqtb[10289].hh.rh].hh.b1<>0)and(mem[eqtb[10289].hh.rh+3].int<>0)
then begin eqtb[10289].hh.rh:=finiteshrink(eqtb[10289].hh.rh);end;
if(mem[eqtb[10290].hh.rh].hh.b1<>0)and(mem[eqtb[10290].hh.rh+3].int<>0)
then begin eqtb[10290].hh.rh:=finiteshrink(eqtb[10290].hh.rh);end;
q:=eqtb[10289].hh.rh;r:=eqtb[10290].hh.rh;
background[1]:=mem[q+1].int+mem[r+1].int;background[2]:=0;
background[3]:=0;background[4]:=0;background[5]:=0;
background[2+mem[q].hh.b0]:=mem[q+2].int;
background[2+mem[r].hh.b0]:=background[2+mem[r].hh.b0]+mem[r+2].int;
background[6]:=mem[q+3].int+mem[r+3].int;{:827}{834:}
minimumdemerits:=1073741823;minimaldemerits[3]:=1073741823;
minimaldemerits[2]:=1073741823;minimaldemerits[1]:=1073741823;
minimaldemerits[0]:=1073741823;{:834}{848:}
if eqtb[10812].hh.rh=0 then if eqtb[13247].int=0 then begin
lastspecialline:=0;secondwidth:=eqtb[13233].int;secondindent:=0;
end else{849:}begin lastspecialline:=abs(eqtb[12704].int);
if eqtb[12704].int<0 then begin firstwidth:=eqtb[13233].int-abs(eqtb[
13247].int);
if eqtb[13247].int>=0 then firstindent:=eqtb[13247].int else firstindent
:=0;secondwidth:=eqtb[13233].int;secondindent:=0;
end else begin firstwidth:=eqtb[13233].int;firstindent:=0;
secondwidth:=eqtb[13233].int-abs(eqtb[13247].int);
if eqtb[13247].int>=0 then secondindent:=eqtb[13247].int else
secondindent:=0;end;end{:849}
else begin lastspecialline:=mem[eqtb[10812].hh.rh].hh.lh-1;
secondwidth:=mem[eqtb[10812].hh.rh+2*(lastspecialline+1)].int;
secondindent:=mem[eqtb[10812].hh.rh+2*lastspecialline+1].int;end;
if eqtb[12682].int=0 then easyline:=lastspecialline else easyline:=
262143{:848};{863:}threshold:=eqtb[12663].int;
if threshold>=0 then begin ifdef('STAT')if eqtb[12695].int>0 then begin
begindiagnostic;printnl(926);end;endif('STAT')secondpass:=false;
finalpass:=false;end else begin threshold:=eqtb[12664].int;
secondpass:=true;finalpass:=(eqtb[13250].int<=0);
ifdef('STAT')if eqtb[12695].int>0 then begindiagnostic;endif('STAT')end;
while true do begin if threshold>10000 then threshold:=10000;
if secondpass then{891:}
begin ifdef('INITEX')if trienotready then inittrie;
endif('INITEX')lhyf:=curlist.lhmfield;rhyf:=curlist.rhmfield;curlang:=0;
end{:891};{864:}q:=getnode(3);mem[q].hh.b0:=0;mem[q].hh.b1:=2;
mem[q].hh.rh:=memtop-7;mem[q+1].hh.rh:=0;
mem[q+1].hh.lh:=curlist.pgfield+1;mem[q+2].int:=0;
mem[memtop-7].hh.rh:=q;activewidth[1]:=background[1];
activewidth[2]:=background[2];activewidth[3]:=background[3];
activewidth[4]:=background[4];activewidth[5]:=background[5];
activewidth[6]:=background[6];passive:=0;printednode:=memtop-3;
passnumber:=0;fontinshortdisplay:=0{:864};curp:=mem[memtop-3].hh.rh;
autobreaking:=true;prevp:=curp;
while(curp<>0)and(mem[memtop-7].hh.rh<>memtop-7)do{866:}
begin if(curp>=himemmin)then{867:}begin prevp:=curp;
repeat f:=mem[curp].hh.b0;
activewidth[1]:=activewidth[1]+fontinfo[widthbase[f]+fontinfo[charbase[f
]+mem[curp].hh.b1].qqqq.b0].int;curp:=mem[curp].hh.rh;
until not(curp>=himemmin);end{:867};
case mem[curp].hh.b0 of 0,1,2:activewidth[1]:=activewidth[1]+mem[curp+1]
.int;8:{1362:}
if mem[curp].hh.b1=4 then begin curlang:=mem[curp+1].hh.rh;
lhyf:=mem[curp+1].hh.b0;rhyf:=mem[curp+1].hh.b1;end{:1362};
10:begin{868:}
if autobreaking then begin if(prevp>=himemmin)then trybreak(0,0)else if(
mem[prevp].hh.b0<9)then trybreak(0,0);end;
if(mem[mem[curp+1].hh.lh].hh.b1<>0)and(mem[mem[curp+1].hh.lh+3].int<>0)
then begin mem[curp+1].hh.lh:=finiteshrink(mem[curp+1].hh.lh);end;
q:=mem[curp+1].hh.lh;activewidth[1]:=activewidth[1]+mem[q+1].int;
activewidth[2+mem[q].hh.b0]:=activewidth[2+mem[q].hh.b0]+mem[q+2].int;
activewidth[6]:=activewidth[6]+mem[q+3].int{:868};
if secondpass and autobreaking then{894:}begin prevs:=curp;
s:=mem[prevs].hh.rh;if s<>0 then begin{896:}
while true do begin if(s>=himemmin)then begin c:=mem[s].hh.b1;
hf:=mem[s].hh.b0;
end else if mem[s].hh.b0=6 then if mem[s+1].hh.rh=0 then goto 22 else
begin q:=mem[s+1].hh.rh;c:=mem[q].hh.b1;hf:=mem[q].hh.b0;
end else if(mem[s].hh.b0=11)and(mem[s].hh.b1=0)then goto 22 else if mem[
s].hh.b0=8 then begin{1363:}
if mem[s].hh.b1=4 then begin curlang:=mem[s+1].hh.rh;
lhyf:=mem[s+1].hh.b0;rhyf:=mem[s+1].hh.b1;end{:1363};goto 22;
end else goto 31;
if eqtb[11639+c].hh.rh<>0 then if(eqtb[11639+c].hh.rh=c)or(eqtb[12701].
int>0)then goto 32 else goto 31;22:prevs:=s;s:=mem[prevs].hh.rh;end;
32:hyfchar:=hyphenchar[hf];if hyfchar<0 then goto 31;
if hyfchar>255 then goto 31;ha:=prevs{:896};
if lhyf+rhyf>63 then goto 31;{897:}hn:=0;
while true do begin if(s>=himemmin)then begin if mem[s].hh.b0<>hf then
goto 33;c:=mem[s].hh.b1;if eqtb[11639+c].hh.rh=0 then goto 33;
if hn=63 then goto 33;hb:=s;incr(hn);hu[hn]:=c;
hc[hn]:=eqtb[11639+c].hh.rh;end else if mem[s].hh.b0=6 then{898:}
begin if mem[s+1].hh.b0<>hf then goto 33;j:=hn;q:=mem[s+1].hh.rh;
while q>0 do begin c:=mem[q].hh.b1;
if eqtb[11639+c].hh.rh=0 then goto 33;if j=63 then goto 33;incr(j);
hu[j]:=c;hc[j]:=eqtb[11639+c].hh.rh;q:=mem[q].hh.rh;end;hb:=s;hn:=j;
end{:898}else if(mem[s].hh.b0<>11)or(mem[s].hh.b1<>0)then goto 33;
s:=mem[s].hh.rh;end;33:{:897};{899:}if hn<lhyf+rhyf then goto 31;
while true do begin if not((s>=himemmin))then case mem[s].hh.b0 of 6:;
11:if mem[s].hh.b1<>0 then goto 34;8,10,12,3,5,4:goto 34;
others:goto 31 end;s:=mem[s].hh.rh;end;34:{:899};hyphenate;end;
31:end{:894};end;
11:begin if not(mem[curp].hh.rh>=himemmin)and autobreaking then if mem[
mem[curp].hh.rh].hh.b0=10 then trybreak(0,0);
activewidth[1]:=activewidth[1]+mem[curp+1].int;end;
6:begin f:=mem[curp+1].hh.b0;
activewidth[1]:=activewidth[1]+fontinfo[widthbase[f]+fontinfo[charbase[f
]+mem[curp+1].hh.b1].qqqq.b0].int;end;7:{869:}
begin s:=mem[curp+1].hh.lh;discwidth:=0;
if s=0 then trybreak(eqtb[12667].int,1)else begin repeat{870:}
if(s>=himemmin)then begin f:=mem[s].hh.b0;
discwidth:=discwidth+fontinfo[widthbase[f]+fontinfo[charbase[f]+mem[s].
hh.b1].qqqq.b0].int;
end else case mem[s].hh.b0 of 6:begin f:=mem[s+1].hh.b0;
discwidth:=discwidth+fontinfo[widthbase[f]+fontinfo[charbase[f]+mem[s+1]
.hh.b1].qqqq.b0].int;end;0,1,2,11:discwidth:=discwidth+mem[s+1].int;
others:confusion(930)end{:870};s:=mem[s].hh.rh;until s=0;
activewidth[1]:=activewidth[1]+discwidth;trybreak(eqtb[12666].int,1);
activewidth[1]:=activewidth[1]-discwidth;end;r:=mem[curp].hh.b1;
s:=mem[curp].hh.rh;while r>0 do begin{871:}
if(s>=himemmin)then begin f:=mem[s].hh.b0;
activewidth[1]:=activewidth[1]+fontinfo[widthbase[f]+fontinfo[charbase[f
]+mem[s].hh.b1].qqqq.b0].int;
end else case mem[s].hh.b0 of 6:begin f:=mem[s+1].hh.b0;
activewidth[1]:=activewidth[1]+fontinfo[widthbase[f]+fontinfo[charbase[f
]+mem[s+1].hh.b1].qqqq.b0].int;end;
0,1,2,11:activewidth[1]:=activewidth[1]+mem[s+1].int;
others:confusion(931)end{:871};decr(r);s:=mem[s].hh.rh;end;prevp:=curp;
curp:=s;goto 35;end{:869};9:begin autobreaking:=(mem[curp].hh.b1=1);
begin if not(mem[curp].hh.rh>=himemmin)and autobreaking then if mem[mem[
curp].hh.rh].hh.b0=10 then trybreak(0,0);
activewidth[1]:=activewidth[1]+mem[curp+1].int;end;end;
12:trybreak(mem[curp+1].int,0);4,3,5:;others:confusion(929)end;
prevp:=curp;curp:=mem[curp].hh.rh;35:end{:866};if curp=0 then{873:}
begin trybreak(-10000,1);
if mem[memtop-7].hh.rh<>memtop-7 then begin{874:}r:=mem[memtop-7].hh.rh;
fewestdemerits:=1073741823;
repeat if mem[r].hh.b0<>2 then if mem[r+2].int<fewestdemerits then begin
fewestdemerits:=mem[r+2].int;bestbet:=r;end;r:=mem[r].hh.rh;
until r=memtop-7;bestline:=mem[bestbet+1].hh.lh{:874};
if eqtb[12682].int=0 then goto 30;{875:}begin r:=mem[memtop-7].hh.rh;
actuallooseness:=0;
repeat if mem[r].hh.b0<>2 then begin linediff:=toint(mem[r+1].hh.lh)-
toint(bestline);
if((linediff<actuallooseness)and(eqtb[12682].int<=linediff))or((linediff
>actuallooseness)and(eqtb[12682].int>=linediff))then begin bestbet:=r;
actuallooseness:=linediff;fewestdemerits:=mem[r+2].int;
end else if(linediff=actuallooseness)and(mem[r+2].int<fewestdemerits)
then begin bestbet:=r;fewestdemerits:=mem[r+2].int;end;end;
r:=mem[r].hh.rh;until r=memtop-7;bestline:=mem[bestbet+1].hh.lh;
end{:875};if(actuallooseness=eqtb[12682].int)or finalpass then goto 30;
end;end{:873};{865:}q:=mem[memtop-7].hh.rh;
while q<>memtop-7 do begin curp:=mem[q].hh.rh;
if mem[q].hh.b0=2 then freenode(q,7)else freenode(q,3);q:=curp;end;
q:=passive;while q<>0 do begin curp:=mem[q].hh.rh;freenode(q,2);q:=curp;
end{:865};
if not secondpass then begin ifdef('STAT')if eqtb[12695].int>0 then
printnl(927);endif('STAT')threshold:=eqtb[12664].int;secondpass:=true;
finalpass:=(eqtb[13250].int<=0);
end else begin ifdef('STAT')if eqtb[12695].int>0 then printnl(928);
endif('STAT')background[2]:=background[2]+eqtb[13250].int;
finalpass:=true;end;end;
30:ifdef('STAT')if eqtb[12695].int>0 then begin enddiagnostic(true);
normalizeselector;end;endif('STAT'){:863};{876:}
postlinebreak(finalwidowpenalty){:876};{865:}q:=mem[memtop-7].hh.rh;
while q<>memtop-7 do begin curp:=mem[q].hh.rh;
if mem[q].hh.b0=2 then freenode(q,7)else freenode(q,3);q:=curp;end;
q:=passive;while q<>0 do begin curp:=mem[q].hh.rh;freenode(q,2);q:=curp;
end{:865};packbeginline:=0;end;{:815}{934:}procedure newhyphexceptions;
label 21,10,40,45;var n:smallnumber;j:smallnumber;h:hyphpointer;
k:strnumber;p:halfword;q:halfword;s,t:strnumber;u,v:poolpointer;
begin scanleftbrace;
if eqtb[12713].int<=0 then curlang:=0 else if eqtb[12713].int>255 then
curlang:=0 else curlang:=eqtb[12713].int;{935:}n:=0;p:=0;
while true do begin getxtoken;21:case curcmd of 11,12,68:{937:}
if curchr=45 then{938:}begin if n<63 then begin q:=getavail;
mem[q].hh.rh:=p;mem[q].hh.lh:=n;p:=q;end;end{:938}
else begin if eqtb[11639+curchr].hh.rh=0 then begin begin if interaction
=3 then wakeupterminal;printnl(262);print(938);end;begin helpptr:=2;
helpline[1]:=939;helpline[0]:=940;end;error;
end else if n<63 then begin incr(n);hc[n]:=eqtb[11639+curchr].hh.rh;end;
end{:937};16:begin scancharnum;curchr:=curval;curcmd:=68;goto 21;end;
10,2:begin if n>1 then{939:}begin incr(n);hc[n]:=curlang;
begin if poolptr+n>poolsize then overflow(257,poolsize-initpoolptr);end;
h:=0;for j:=1 to n do begin h:=(h+h+hc[j])mod 607;
begin strpool[poolptr]:=hc[j];incr(poolptr);end;end;s:=makestring;{940:}
if hyphcount=607 then overflow(941,607);incr(hyphcount);
while hyphword[h]<>0 do begin{941:}k:=hyphword[h];
if(strstart[k+1]-strstart[k])<(strstart[s+1]-strstart[s])then goto 40;
if(strstart[k+1]-strstart[k])>(strstart[s+1]-strstart[s])then goto 45;
u:=strstart[k];v:=strstart[s];
repeat if strpool[u]<strpool[v]then goto 40;
if strpool[u]>strpool[v]then goto 45;incr(u);incr(v);
until u=strstart[k+1];40:q:=hyphlist[h];hyphlist[h]:=p;p:=q;
t:=hyphword[h];hyphword[h]:=s;s:=t;45:{:941};
if h>0 then decr(h)else h:=607;end;hyphword[h]:=s;hyphlist[h]:=p{:940};
end{:939};if curcmd=2 then goto 10;n:=0;p:=0;end;others:{936:}
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(676);end;printesc(934);print(935);begin helpptr:=2;
helpline[1]:=936;helpline[0]:=937;end;error;end{:936}end;end{:935};
10:end;{:934}{968:}function prunepagetop(p:halfword):halfword;
var prevp:halfword;q:halfword;begin prevp:=memtop-3;
mem[memtop-3].hh.rh:=p;while p<>0 do case mem[p].hh.b0 of 0,1,2:{969:}
begin q:=newskipparam(10);mem[prevp].hh.rh:=q;mem[q].hh.rh:=p;
if mem[tempptr+1].int>mem[p+3].int then mem[tempptr+1].int:=mem[tempptr
+1].int-mem[p+3].int else mem[tempptr+1].int:=0;p:=0;end{:969};
8,4,3:begin prevp:=p;p:=mem[prevp].hh.rh;end;10,11,12:begin q:=p;
p:=mem[q].hh.rh;mem[q].hh.rh:=0;mem[prevp].hh.rh:=p;flushnodelist(q);
end;others:confusion(952)end;prunepagetop:=mem[memtop-3].hh.rh;end;
{:968}{970:}function vertbreak(p:halfword;h,d:scaled):halfword;
label 30,45,90;var prevp:halfword;q,r:halfword;pi:integer;b:integer;
leastcost:integer;bestplace:halfword;prevdp:scaled;t:smallnumber;
begin prevp:=p;leastcost:=1073741823;activewidth[1]:=0;
activewidth[2]:=0;activewidth[3]:=0;activewidth[4]:=0;activewidth[5]:=0;
activewidth[6]:=0;prevdp:=0;while true do begin{972:}
if p=0 then pi:=-10000 else{973:}
case mem[p].hh.b0 of 0,1,2:begin activewidth[1]:=activewidth[1]+prevdp+
mem[p+3].int;prevdp:=mem[p+2].int;goto 45;end;8:{1365:}goto 45{:1365};
10:if(mem[prevp].hh.b0<9)then pi:=0 else goto 90;
11:begin if mem[p].hh.rh=0 then t:=12 else t:=mem[mem[p].hh.rh].hh.b0;
if t=10 then pi:=0 else goto 90;end;12:pi:=mem[p+1].int;4,3:goto 45;
others:confusion(953)end{:973};{974:}if pi<10000 then begin{975:}
if activewidth[1]<h then if(activewidth[3]<>0)or(activewidth[4]<>0)or(
activewidth[5]<>0)then b:=0 else b:=badness(h-activewidth[1],activewidth
[2])else if activewidth[1]-h>activewidth[6]then b:=1073741823 else b:=
badness(activewidth[1]-h,activewidth[6]){:975};
if b<1073741823 then if pi<=-10000 then b:=pi else if b<10000 then b:=b+
pi else b:=100000;if b<=leastcost then begin bestplace:=p;leastcost:=b;
bestheightplusdepth:=activewidth[1]+prevdp;end;
if(b=1073741823)or(pi<=-10000)then goto 30;end{:974};
if(mem[p].hh.b0<10)or(mem[p].hh.b0>11)then goto 45;90:{976:}
if mem[p].hh.b0=11 then q:=p else begin q:=mem[p+1].hh.lh;
activewidth[2+mem[q].hh.b0]:=activewidth[2+mem[q].hh.b0]+mem[q+2].int;
activewidth[6]:=activewidth[6]+mem[q+3].int;
if(mem[q].hh.b1<>0)and(mem[q+3].int<>0)then begin begin if interaction=3
then wakeupterminal;printnl(262);print(954);end;begin helpptr:=4;
helpline[3]:=955;helpline[2]:=956;helpline[1]:=957;helpline[0]:=915;end;
error;r:=newspec(q);mem[r].hh.b1:=0;deleteglueref(q);mem[p+1].hh.lh:=r;
q:=r;end;end;activewidth[1]:=activewidth[1]+prevdp+mem[q+1].int;
prevdp:=0{:976};
45:if prevdp>d then begin activewidth[1]:=activewidth[1]+prevdp-d;
prevdp:=d;end;{:972};prevp:=p;p:=mem[prevp].hh.rh;end;
30:vertbreak:=bestplace;end;{:970}{977:}function vsplit(n:eightbits;
h:scaled):halfword;label 10,30;var v:halfword;p:halfword;q:halfword;
begin v:=eqtb[11078+n].hh.rh;
if curmark[3]<>0 then begin deletetokenref(curmark[3]);curmark[3]:=0;
deletetokenref(curmark[4]);curmark[4]:=0;end;{978:}
if v=0 then begin vsplit:=0;goto 10;end;
if mem[v].hh.b0<>1 then begin begin if interaction=3 then wakeupterminal
;printnl(262);print(335);end;printesc(958);print(959);printesc(960);
begin helpptr:=2;helpline[1]:=961;helpline[0]:=962;end;error;vsplit:=0;
goto 10;end{:978};q:=vertbreak(mem[v+5].hh.rh,h,eqtb[13236].int);{979:}
p:=mem[v+5].hh.rh;
if p=q then mem[v+5].hh.rh:=0 else while true do begin if mem[p].hh.b0=4
then if curmark[3]=0 then begin curmark[3]:=mem[p+1].int;
curmark[4]:=curmark[3];mem[curmark[3]].hh.lh:=mem[curmark[3]].hh.lh+2;
end else begin deletetokenref(curmark[4]);curmark[4]:=mem[p+1].int;
incr(mem[curmark[4]].hh.lh);end;
if mem[p].hh.rh=q then begin mem[p].hh.rh:=0;goto 30;end;
p:=mem[p].hh.rh;end;30:{:979};q:=prunepagetop(q);p:=mem[v+5].hh.rh;
freenode(v,7);
if q=0 then eqtb[11078+n].hh.rh:=0 else eqtb[11078+n].hh.rh:=vpackage(q,
0,1,1073741823);vsplit:=vpackage(p,h,0,eqtb[13236].int);10:end;{:977}
{985:}procedure printtotals;begin printscaled(pagesofar[1]);
if pagesofar[2]<>0 then begin print(310);printscaled(pagesofar[2]);
print(335);end;if pagesofar[3]<>0 then begin print(310);
printscaled(pagesofar[3]);print(309);end;
if pagesofar[4]<>0 then begin print(310);printscaled(pagesofar[4]);
print(971);end;if pagesofar[5]<>0 then begin print(310);
printscaled(pagesofar[5]);print(972);end;
if pagesofar[6]<>0 then begin print(311);printscaled(pagesofar[6]);end;
end;{:985}{987:}procedure freezepagespecs(s:smallnumber);
begin pagecontents:=s;pagesofar[0]:=eqtb[13234].int;
pagemaxdepth:=eqtb[13235].int;pagesofar[7]:=0;pagesofar[1]:=0;
pagesofar[2]:=0;pagesofar[3]:=0;pagesofar[4]:=0;pagesofar[5]:=0;
pagesofar[6]:=0;leastpagecost:=1073741823;
ifdef('STAT')if eqtb[12696].int>0 then begin begindiagnostic;
printnl(980);printscaled(pagesofar[0]);print(981);
printscaled(pagemaxdepth);enddiagnostic(false);end;endif('STAT')end;
{:987}{992:}procedure boxerror(n:eightbits);begin error;begindiagnostic;
printnl(829);showbox(eqtb[11078+n].hh.rh);enddiagnostic(true);
flushnodelist(eqtb[11078+n].hh.rh);eqtb[11078+n].hh.rh:=0;end;{:992}
{993:}procedure ensurevbox(n:eightbits);var p:halfword;
begin p:=eqtb[11078+n].hh.rh;
if p<>0 then if mem[p].hh.b0=0 then begin begin if interaction=3 then
wakeupterminal;printnl(262);print(982);end;begin helpptr:=3;
helpline[2]:=983;helpline[1]:=984;helpline[0]:=985;end;boxerror(n);end;
end;{:993}{994:}{1012:}procedure fireup(c:halfword);label 10;
var p,q,r,s:halfword;prevp:halfword;n:0..255;wait:boolean;
savevbadness:integer;savevfuzz:scaled;savesplittopskip:halfword;
begin{1013:}
if mem[bestpagebreak].hh.b0=12 then begin geqworddefine(12702,mem[
bestpagebreak+1].int);mem[bestpagebreak+1].int:=10000;
end else geqworddefine(12702,10000){:1013};
if curmark[2]<>0 then begin if curmark[0]<>0 then deletetokenref(curmark
[0]);curmark[0]:=curmark[2];incr(mem[curmark[0]].hh.lh);
deletetokenref(curmark[1]);curmark[1]:=0;end;{1014:}
if c=bestpagebreak then bestpagebreak:=0;{1015:}
if eqtb[11333].hh.rh<>0 then begin begin if interaction=3 then
wakeupterminal;printnl(262);print(335);end;printesc(405);print(996);
begin helpptr:=2;helpline[1]:=997;helpline[0]:=985;end;boxerror(255);
end{:1015};insertpenalties:=0;savesplittopskip:=eqtb[10292].hh.rh;
if eqtb[12716].int<=0 then{1018:}begin r:=mem[memtop].hh.rh;
while r<>memtop do begin if mem[r+2].hh.lh<>0 then begin n:=mem[r].hh.b1
;ensurevbox(n);
if eqtb[11078+n].hh.rh=0 then eqtb[11078+n].hh.rh:=newnullbox;
p:=eqtb[11078+n].hh.rh+5;while mem[p].hh.rh<>0 do p:=mem[p].hh.rh;
mem[r+2].hh.rh:=p;end;r:=mem[r].hh.rh;end;end{:1018};q:=memtop-4;
mem[q].hh.rh:=0;prevp:=memtop-2;p:=mem[prevp].hh.rh;
while p<>bestpagebreak do begin if mem[p].hh.b0=3 then begin if eqtb[
12716].int<=0 then{1020:}begin r:=mem[memtop].hh.rh;
while mem[r].hh.b1<>mem[p].hh.b1 do r:=mem[r].hh.rh;
if mem[r+2].hh.lh=0 then wait:=true else begin wait:=false;
s:=mem[r+2].hh.rh;mem[s].hh.rh:=mem[p+4].hh.lh;
if mem[r+2].hh.lh=p then{1021:}
begin if mem[r].hh.b0=1 then if(mem[r+1].hh.lh=p)and(mem[r+1].hh.rh<>0)
then begin while mem[s].hh.rh<>mem[r+1].hh.rh do s:=mem[s].hh.rh;
mem[s].hh.rh:=0;eqtb[10292].hh.rh:=mem[p+4].hh.rh;
mem[p+4].hh.lh:=prunepagetop(mem[r+1].hh.rh);
if mem[p+4].hh.lh<>0 then begin tempptr:=vpackage(mem[p+4].hh.lh,0,1,
1073741823);mem[p+3].int:=mem[tempptr+3].int+mem[tempptr+2].int;
freenode(tempptr,7);wait:=true;end;end;mem[r+2].hh.lh:=0;
n:=mem[r].hh.b1;tempptr:=mem[eqtb[11078+n].hh.rh+5].hh.rh;
freenode(eqtb[11078+n].hh.rh,7);
eqtb[11078+n].hh.rh:=vpackage(tempptr,0,1,1073741823);end{:1021}
else begin while mem[s].hh.rh<>0 do s:=mem[s].hh.rh;mem[r+2].hh.rh:=s;
end;end;{1022:}mem[prevp].hh.rh:=mem[p].hh.rh;mem[p].hh.rh:=0;
if wait then begin mem[q].hh.rh:=p;q:=p;incr(insertpenalties);
end else begin deleteglueref(mem[p+4].hh.rh);freenode(p,5);end;
p:=prevp{:1022};end{:1020};end else if mem[p].hh.b0=4 then{1016:}
begin if curmark[1]=0 then begin curmark[1]:=mem[p+1].int;
incr(mem[curmark[1]].hh.lh);end;
if curmark[2]<>0 then deletetokenref(curmark[2]);
curmark[2]:=mem[p+1].int;incr(mem[curmark[2]].hh.lh);end{:1016};
prevp:=p;p:=mem[prevp].hh.rh;end;eqtb[10292].hh.rh:=savesplittopskip;
{1017:}
if p<>0 then begin if mem[memtop-1].hh.rh=0 then if nestptr=0 then
curlist.tailfield:=pagetail else nest[0].tailfield:=pagetail;
mem[pagetail].hh.rh:=mem[memtop-1].hh.rh;mem[memtop-1].hh.rh:=p;
mem[prevp].hh.rh:=0;end;savevbadness:=eqtb[12690].int;
eqtb[12690].int:=10000;savevfuzz:=eqtb[13239].int;
eqtb[13239].int:=1073741823;
eqtb[11333].hh.rh:=vpackage(mem[memtop-2].hh.rh,bestsize,0,pagemaxdepth)
;eqtb[12690].int:=savevbadness;eqtb[13239].int:=savevfuzz;
if lastglue<>262143 then deleteglueref(lastglue);{991:}pagecontents:=0;
pagetail:=memtop-2;mem[memtop-2].hh.rh:=0;lastglue:=262143;
lastpenalty:=0;lastkern:=0;pagesofar[7]:=0;pagemaxdepth:=0{:991};
if q<>memtop-4 then begin mem[memtop-2].hh.rh:=mem[memtop-4].hh.rh;
pagetail:=q;end{:1017};{1019:}r:=mem[memtop].hh.rh;
while r<>memtop do begin q:=mem[r].hh.rh;freenode(r,4);r:=q;end;
mem[memtop].hh.rh:=memtop{:1019}{:1014};
if(curmark[0]<>0)and(curmark[1]=0)then begin curmark[1]:=curmark[0];
incr(mem[curmark[0]].hh.lh);end;
if eqtb[10813].hh.rh<>0 then if deadcycles>=eqtb[12703].int then{1024:}
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(998);end;printint(deadcycles);print(999);begin helpptr:=3;
helpline[2]:=1000;helpline[1]:=1001;helpline[0]:=1002;end;error;
end{:1024}else{1025:}begin outputactive:=true;incr(deadcycles);pushnest;
curlist.modefield:=-1;curlist.auxfield.int:=-65536000;
curlist.mlfield:=-line;begintokenlist(eqtb[10813].hh.rh,6);
newsavelevel(8);normalparagraph;scanleftbrace;goto 10;end{:1025};{1023:}
begin if mem[memtop-2].hh.rh<>0 then begin if mem[memtop-1].hh.rh=0 then
if nestptr=0 then curlist.tailfield:=pagetail else nest[0].tailfield:=
pagetail else mem[pagetail].hh.rh:=mem[memtop-1].hh.rh;
mem[memtop-1].hh.rh:=mem[memtop-2].hh.rh;mem[memtop-2].hh.rh:=0;
pagetail:=memtop-2;end;shipout(eqtb[11333].hh.rh);eqtb[11333].hh.rh:=0;
end{:1023};10:end;{:1012}procedure buildpage;label 10,30,31,22,80,90;
var p:halfword;q,r:halfword;b,c:integer;pi:integer;n:0..255;
delta,h,w:scaled;
begin if(mem[memtop-1].hh.rh=0)or outputactive then goto 10;
repeat 22:p:=mem[memtop-1].hh.rh;{996:}
if lastglue<>262143 then deleteglueref(lastglue);lastpenalty:=0;
lastkern:=0;if mem[p].hh.b0=10 then begin lastglue:=mem[p+1].hh.lh;
incr(mem[lastglue].hh.rh);end else begin lastglue:=262143;
if mem[p].hh.b0=12 then lastpenalty:=mem[p+1].int else if mem[p].hh.b0=
11 then lastkern:=mem[p+1].int;end{:996};{997:}{1000:}
case mem[p].hh.b0 of 0,1,2:if pagecontents<2 then{1001:}
begin if pagecontents=0 then freezepagespecs(2)else pagecontents:=2;
q:=newskipparam(9);
if mem[tempptr+1].int>mem[p+3].int then mem[tempptr+1].int:=mem[tempptr
+1].int-mem[p+3].int else mem[tempptr+1].int:=0;mem[q].hh.rh:=p;
mem[memtop-1].hh.rh:=q;goto 22;end{:1001}else{1002:}
begin pagesofar[1]:=pagesofar[1]+pagesofar[7]+mem[p+3].int;
pagesofar[7]:=mem[p+2].int;goto 80;end{:1002};8:{1364:}goto 80{:1364};
10:if pagecontents<2 then goto 31 else if(mem[pagetail].hh.b0<9)then pi
:=0 else goto 90;
11:if pagecontents<2 then goto 31 else if mem[p].hh.rh=0 then goto 10
else if mem[mem[p].hh.rh].hh.b0=10 then pi:=0 else goto 90;
12:if pagecontents<2 then goto 31 else pi:=mem[p+1].int;4:goto 80;
3:{1008:}begin if pagecontents=0 then freezepagespecs(1);
n:=mem[p].hh.b1;r:=memtop;
while n>=mem[mem[r].hh.rh].hh.b1 do r:=mem[r].hh.rh;n:=n;
if mem[r].hh.b1<>n then{1009:}begin q:=getnode(4);
mem[q].hh.rh:=mem[r].hh.rh;mem[r].hh.rh:=q;r:=q;mem[r].hh.b1:=n;
mem[r].hh.b0:=0;ensurevbox(n);
if eqtb[11078+n].hh.rh=0 then mem[r+3].int:=0 else mem[r+3].int:=mem[
eqtb[11078+n].hh.rh+3].int+mem[eqtb[11078+n].hh.rh+2].int;
mem[r+2].hh.lh:=0;q:=eqtb[10300+n].hh.rh;
if eqtb[12718+n].int=1000 then h:=mem[r+3].int else h:=xovern(mem[r+3].
int,1000)*eqtb[12718+n].int;pagesofar[0]:=pagesofar[0]-h-mem[q+1].int;
pagesofar[2+mem[q].hh.b0]:=pagesofar[2+mem[q].hh.b0]+mem[q+2].int;
pagesofar[6]:=pagesofar[6]+mem[q+3].int;
if(mem[q].hh.b1<>0)and(mem[q+3].int<>0)then begin begin if interaction=3
then wakeupterminal;printnl(262);print(991);end;printesc(391);
printint(n);begin helpptr:=3;helpline[2]:=992;helpline[1]:=993;
helpline[0]:=915;end;error;end;end{:1009};
if mem[r].hh.b0=1 then insertpenalties:=insertpenalties+mem[p+1].int
else begin mem[r+2].hh.rh:=p;
delta:=pagesofar[0]-pagesofar[1]-pagesofar[7]+pagesofar[6];
if eqtb[12718+n].int=1000 then h:=mem[p+3].int else h:=xovern(mem[p+3].
int,1000)*eqtb[12718+n].int;
if((h<=0)or(h<=delta))and(mem[p+3].int+mem[r+3].int<=eqtb[13251+n].int)
then begin pagesofar[0]:=pagesofar[0]-h;
mem[r+3].int:=mem[r+3].int+mem[p+3].int;end else{1010:}
begin if eqtb[12718+n].int<=0 then w:=1073741823 else begin w:=pagesofar
[0]-pagesofar[1]-pagesofar[7];
if eqtb[12718+n].int<>1000 then w:=xovern(w,eqtb[12718+n].int)*1000;end;
if w>eqtb[13251+n].int-mem[r+3].int then w:=eqtb[13251+n].int-mem[r+3].
int;q:=vertbreak(mem[p+4].hh.lh,w,mem[p+2].int);
mem[r+3].int:=mem[r+3].int+bestheightplusdepth;
ifdef('STAT')if eqtb[12696].int>0 then{1011:}begin begindiagnostic;
printnl(994);printint(n);print(995);printscaled(w);printchar(44);
printscaled(bestheightplusdepth);print(924);
if q=0 then printint(-10000)else if mem[q].hh.b0=12 then printint(mem[q
+1].int)else printchar(48);enddiagnostic(false);end{:1011};
endif('STAT')if eqtb[12718+n].int<>1000 then bestheightplusdepth:=xovern
(bestheightplusdepth,1000)*eqtb[12718+n].int;
pagesofar[0]:=pagesofar[0]-bestheightplusdepth;mem[r].hh.b0:=1;
mem[r+1].hh.rh:=q;mem[r+1].hh.lh:=p;
if q=0 then insertpenalties:=insertpenalties-10000 else if mem[q].hh.b0=
12 then insertpenalties:=insertpenalties+mem[q+1].int;end{:1010};end;
goto 80;end{:1008};others:confusion(986)end{:1000};{1005:}
if pi<10000 then begin{1007:}
if pagesofar[1]<pagesofar[0]then if(pagesofar[3]<>0)or(pagesofar[4]<>0)
or(pagesofar[5]<>0)then b:=0 else b:=badness(pagesofar[0]-pagesofar[1],
pagesofar[2])else if pagesofar[1]-pagesofar[0]>pagesofar[6]then b:=
1073741823 else b:=badness(pagesofar[1]-pagesofar[0],pagesofar[6]){:1007
};
if b<1073741823 then if pi<=-10000 then c:=pi else if b<10000 then c:=b+
pi+insertpenalties else c:=100000 else c:=b;
if insertpenalties>=10000 then c:=1073741823;
ifdef('STAT')if eqtb[12696].int>0 then{1006:}begin begindiagnostic;
printnl(37);print(920);printtotals;print(989);printscaled(pagesofar[0]);
print(923);if b=1073741823 then printchar(42)else printint(b);
print(924);printint(pi);print(990);
if c=1073741823 then printchar(42)else printint(c);
if c<=leastpagecost then printchar(35);enddiagnostic(false);end{:1006};
endif('STAT')if c<=leastpagecost then begin bestpagebreak:=p;
bestsize:=pagesofar[0];leastpagecost:=c;r:=mem[memtop].hh.rh;
while r<>memtop do begin mem[r+2].hh.lh:=mem[r+2].hh.rh;r:=mem[r].hh.rh;
end;end;if(c=1073741823)or(pi<=-10000)then begin fireup(p);
if outputactive then goto 10;goto 30;end;end{:1005};
if(mem[p].hh.b0<10)or(mem[p].hh.b0>11)then goto 80;90:{1004:}
if mem[p].hh.b0=11 then q:=p else begin q:=mem[p+1].hh.lh;
pagesofar[2+mem[q].hh.b0]:=pagesofar[2+mem[q].hh.b0]+mem[q+2].int;
pagesofar[6]:=pagesofar[6]+mem[q+3].int;
if(mem[q].hh.b1<>0)and(mem[q+3].int<>0)then begin begin if interaction=3
then wakeupterminal;printnl(262);print(987);end;begin helpptr:=4;
helpline[3]:=988;helpline[2]:=956;helpline[1]:=957;helpline[0]:=915;end;
error;r:=newspec(q);mem[r].hh.b1:=0;deleteglueref(q);mem[p+1].hh.lh:=r;
q:=r;end;end;pagesofar[1]:=pagesofar[1]+pagesofar[7]+mem[q+1].int;
pagesofar[7]:=0{:1004};80:{1003:}
if pagesofar[7]>pagemaxdepth then begin pagesofar[1]:=pagesofar[1]+
pagesofar[7]-pagemaxdepth;pagesofar[7]:=pagemaxdepth;end;{:1003};{998:}
mem[pagetail].hh.rh:=p;pagetail:=p;mem[memtop-1].hh.rh:=mem[p].hh.rh;
mem[p].hh.rh:=0;goto 30{:998};31:{999:}
mem[memtop-1].hh.rh:=mem[p].hh.rh;mem[p].hh.rh:=0;flushnodelist(p){:999}
;30:{:997};until mem[memtop-1].hh.rh=0;{995:}
if nestptr=0 then curlist.tailfield:=memtop-1 else nest[0].tailfield:=
memtop-1{:995};10:end;{:994}{1030:}{1043:}procedure appspace;
var q:halfword;
begin if(curlist.auxfield.hh.lh>=2000)and(eqtb[10295].hh.rh<>0)then q:=
newparamglue(13)else begin if eqtb[10294].hh.rh<>0 then mainp:=eqtb[
10294].hh.rh else{1042:}begin mainp:=fontglue[eqtb[11334].hh.rh];
if mainp=0 then begin mainp:=newspec(0);
maink:=parambase[eqtb[11334].hh.rh]+2;
mem[mainp+1].int:=fontinfo[maink].int;
mem[mainp+2].int:=fontinfo[maink+1].int;
mem[mainp+3].int:=fontinfo[maink+2].int;
fontglue[eqtb[11334].hh.rh]:=mainp;end;end{:1042};mainp:=newspec(mainp);
{1044:}
if curlist.auxfield.hh.lh>=2000 then mem[mainp+1].int:=mem[mainp+1].int+
fontinfo[7+parambase[eqtb[11334].hh.rh]].int;
mem[mainp+2].int:=xnoverd(mem[mainp+2].int,curlist.auxfield.hh.lh,1000);
mem[mainp+3].int:=xnoverd(mem[mainp+3].int,1000,curlist.auxfield.hh.lh){
:1044};q:=newglue(mainp);mem[mainp].hh.rh:=0;end;
mem[curlist.tailfield].hh.rh:=q;curlist.tailfield:=q;end;{:1043}{1047:}
procedure insertdollarsign;begin backinput;curtok:=804;
begin if interaction=3 then wakeupterminal;printnl(262);print(1010);end;
begin helpptr:=2;helpline[1]:=1011;helpline[0]:=1012;end;inserror;end;
{:1047}{1049:}procedure youcant;
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(681);end;printcmdchr(curcmd,curchr);print(1013);
printmode(curlist.modefield);end;{:1049}{1050:}
procedure reportillegalcase;begin youcant;begin helpptr:=4;
helpline[3]:=1014;helpline[2]:=1015;helpline[1]:=1016;helpline[0]:=1017;
end;error;end;{:1050}{1051:}function privileged:boolean;
begin if curlist.modefield>0 then privileged:=true else begin
reportillegalcase;privileged:=false;end;end;{:1051}{1054:}
function itsallover:boolean;label 10;
begin if privileged then begin if(memtop-2=pagetail)and(curlist.
headfield=curlist.tailfield)and(deadcycles=0)then begin itsallover:=true
;goto 10;end;backinput;begin mem[curlist.tailfield].hh.rh:=newnullbox;
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
mem[curlist.tailfield+1].int:=eqtb[13233].int;
begin mem[curlist.tailfield].hh.rh:=newglue(8);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
begin mem[curlist.tailfield].hh.rh:=newpenalty(-1073741824);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;buildpage;end;
itsallover:=false;10:end;{:1054}{1060:}procedure appendglue;
var s:smallnumber;begin s:=curchr;case s of 0:curval:=4;1:curval:=8;
2:curval:=12;3:curval:=16;4:scanglue(2);5:scanglue(3);end;
begin mem[curlist.tailfield].hh.rh:=newglue(curval);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
if s>=4 then begin decr(mem[curval].hh.rh);
if s>4 then mem[curlist.tailfield].hh.b1:=99;end;end;{:1060}{1061:}
procedure appendkern;var s:quarterword;begin s:=curchr;
scandimen(s=99,false,false);
begin mem[curlist.tailfield].hh.rh:=newkern(curval);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
mem[curlist.tailfield].hh.b1:=s;end;{:1061}{1064:}procedure offsave;
var p:halfword;begin if curgroup=0 then{1066:}
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(772);end;printcmdchr(curcmd,curchr);begin helpptr:=1;
helpline[0]:=1036;end;error;end{:1066}else begin backinput;p:=getavail;
mem[memtop-3].hh.rh:=p;begin if interaction=3 then wakeupterminal;
printnl(262);print(621);end;{1065:}
case curgroup of 14:begin mem[p].hh.lh:=14111;printesc(512);end;
15:begin mem[p].hh.lh:=804;printchar(36);end;
16:begin mem[p].hh.lh:=14112;mem[p].hh.rh:=getavail;p:=mem[p].hh.rh;
mem[p].hh.lh:=3118;printesc(1035);end;others:begin mem[p].hh.lh:=637;
printchar(125);end end{:1065};print(622);
begintokenlist(mem[memtop-3].hh.rh,4);begin helpptr:=5;
helpline[4]:=1030;helpline[3]:=1031;helpline[2]:=1032;helpline[1]:=1033;
helpline[0]:=1034;end;error;end;end;{:1064}{1069:}
procedure extrarightbrace;
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(1041);end;case curgroup of 14:printesc(512);15:printchar(36);
16:printesc(870);end;begin helpptr:=5;helpline[4]:=1042;
helpline[3]:=1043;helpline[2]:=1044;helpline[1]:=1045;helpline[0]:=1046;
end;error;incr(alignstate);end;{:1069}{1070:}procedure normalparagraph;
begin if eqtb[12682].int<>0 then eqworddefine(12682,0);
if eqtb[13247].int<>0 then eqworddefine(13247,0);
if eqtb[12704].int<>1 then eqworddefine(12704,1);
if eqtb[10812].hh.rh<>0 then eqdefine(10812,118,0);end;{:1070}{1075:}
procedure boxend(boxcontext:integer);var p:halfword;
begin if boxcontext<1073741824 then{1076:}
begin if curbox<>0 then begin mem[curbox+4].int:=boxcontext;
if abs(curlist.modefield)=1 then begin appendtovlist(curbox);
if adjusttail<>0 then begin if memtop-5<>adjusttail then begin mem[
curlist.tailfield].hh.rh:=mem[memtop-5].hh.rh;
curlist.tailfield:=adjusttail;end;adjusttail:=0;end;
if curlist.modefield>0 then buildpage;
end else begin if abs(curlist.modefield)=102 then curlist.auxfield.hh.lh
:=1000 else begin p:=newnoad;mem[p+1].hh.rh:=2;mem[p+1].hh.lh:=curbox;
curbox:=p;end;mem[curlist.tailfield].hh.rh:=curbox;
curlist.tailfield:=curbox;end;end;end{:1076}
else if boxcontext<1073742336 then{1077:}
if boxcontext<1073742080 then eqdefine(-1073730746+boxcontext,119,curbox
)else geqdefine(-1073731002+boxcontext,119,curbox){:1077}
else if curbox<>0 then if boxcontext>1073742336 then{1078:}begin{404:}
repeat getxtoken;until(curcmd<>10)and(curcmd<>0){:404};
if((curcmd=26)and(abs(curlist.modefield)<>1))or((curcmd=27)and(abs(
curlist.modefield)=1))or((curcmd=28)and(abs(curlist.modefield)=203))then
begin appendglue;mem[curlist.tailfield].hh.b1:=boxcontext-(1073742237);
mem[curlist.tailfield+1].hh.rh:=curbox;
end else begin begin if interaction=3 then wakeupterminal;printnl(262);
print(1059);end;begin helpptr:=3;helpline[2]:=1060;helpline[1]:=1061;
helpline[0]:=1062;end;backerror;flushnodelist(curbox);end;end{:1078}
else shipout(curbox);end;{:1075}{1079:}
procedure beginbox(boxcontext:integer);label 10,30;var p,q:halfword;
m:quarterword;k:halfword;n:eightbits;
begin case curchr of 0:begin scaneightbitint;
curbox:=eqtb[11078+curval].hh.rh;eqtb[11078+curval].hh.rh:=0;end;
1:begin scaneightbitint;curbox:=copynodelist(eqtb[11078+curval].hh.rh);
end;2:{1080:}begin curbox:=0;
if abs(curlist.modefield)=203 then begin youcant;begin helpptr:=1;
helpline[0]:=1063;end;error;
end else if(curlist.modefield=1)and(curlist.headfield=curlist.tailfield)
then begin youcant;begin helpptr:=2;helpline[1]:=1064;helpline[0]:=1065;
end;error;
end else begin if not(curlist.tailfield>=himemmin)then if(mem[curlist.
tailfield].hh.b0=0)or(mem[curlist.tailfield].hh.b0=1)then{1081:}
begin q:=curlist.headfield;repeat p:=q;
if not(q>=himemmin)then if mem[q].hh.b0=7 then begin for m:=1 to mem[q].
hh.b1 do p:=mem[p].hh.rh;if p=curlist.tailfield then goto 30;end;
q:=mem[p].hh.rh;until q=curlist.tailfield;curbox:=curlist.tailfield;
mem[curbox+4].int:=0;curlist.tailfield:=p;mem[p].hh.rh:=0;30:end{:1081};
end;end{:1080};3:{1082:}begin scaneightbitint;n:=curval;
if not scankeyword(835)then begin begin if interaction=3 then
wakeupterminal;printnl(262);print(1066);end;begin helpptr:=2;
helpline[1]:=1067;helpline[0]:=1068;end;error;end;
scandimen(false,false,false);curbox:=vsplit(n,curval);end{:1082};
others:{1083:}begin k:=curchr-4;savestack[saveptr+0].int:=boxcontext;
if k=102 then if(boxcontext<1073741824)and(abs(curlist.modefield)=1)then
scanspec(3,true)else scanspec(2,true)else begin if k=1 then scanspec(4,
true)else begin scanspec(5,true);k:=1;end;normalparagraph;end;pushnest;
curlist.modefield:=-k;if k=1 then begin curlist.auxfield.int:=-65536000;
if eqtb[10818].hh.rh<>0 then begintokenlist(eqtb[10818].hh.rh,11);
end else begin curlist.auxfield.hh.lh:=1000;
if eqtb[10817].hh.rh<>0 then begintokenlist(eqtb[10817].hh.rh,10);end;
goto 10;end{:1083}end;boxend(boxcontext);10:end;{:1079}{1084:}
procedure scanbox(boxcontext:integer);begin{404:}repeat getxtoken;
until(curcmd<>10)and(curcmd<>0){:404};
if curcmd=20 then beginbox(boxcontext)else if(boxcontext>=1073742337)and
((curcmd=36)or(curcmd=35))then begin curbox:=scanrulespec;
boxend(boxcontext);
end else begin begin if interaction=3 then wakeupterminal;printnl(262);
print(1069);end;begin helpptr:=3;helpline[2]:=1070;helpline[1]:=1071;
helpline[0]:=1072;end;backerror;end;end;{:1084}{1086:}
procedure package(c:smallnumber);var h:scaled;p:halfword;d:scaled;
begin d:=eqtb[13237].int;unsave;saveptr:=saveptr-3;
if curlist.modefield=-102 then curbox:=hpack(mem[curlist.headfield].hh.
rh,savestack[saveptr+2].int,savestack[saveptr+1].int)else begin curbox:=
vpackage(mem[curlist.headfield].hh.rh,savestack[saveptr+2].int,savestack
[saveptr+1].int,d);if c=4 then{1087:}begin h:=0;p:=mem[curbox+5].hh.rh;
if p<>0 then if mem[p].hh.b0<=2 then h:=mem[p+3].int;
mem[curbox+2].int:=mem[curbox+2].int-h+mem[curbox+3].int;
mem[curbox+3].int:=h;end{:1087};end;popnest;
boxend(savestack[saveptr+0].int);end;{:1086}{1091:}
function normmin(h:integer):smallnumber;
begin if h<=0 then normmin:=1 else if h>=63 then normmin:=63 else
normmin:=h;end;procedure newgraf(indented:boolean);
begin curlist.pgfield:=0;
if(curlist.modefield=1)or(curlist.headfield<>curlist.tailfield)then
begin mem[curlist.tailfield].hh.rh:=newparamglue(2);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
curlist.lhmfield:=normmin(eqtb[12714].int);
curlist.rhmfield:=normmin(eqtb[12715].int);pushnest;
curlist.modefield:=102;curlist.auxfield.hh.lh:=1000;
curlist.auxfield.hh.rh:=0;
if indented then begin curlist.tailfield:=newnullbox;
mem[curlist.headfield].hh.rh:=curlist.tailfield;
mem[curlist.tailfield+1].int:=eqtb[13230].int;end;
if eqtb[10814].hh.rh<>0 then begintokenlist(eqtb[10814].hh.rh,7);
if nestptr=1 then buildpage;end;{:1091}{1093:}procedure indentinhmode;
var p,q:halfword;begin if curchr>0 then begin p:=newnullbox;
mem[p+1].int:=eqtb[13230].int;
if abs(curlist.modefield)=102 then curlist.auxfield.hh.lh:=1000 else
begin q:=newnoad;mem[q+1].hh.rh:=2;mem[q+1].hh.lh:=p;p:=q;end;
begin mem[curlist.tailfield].hh.rh:=p;
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;end;end;{:1093}
{1095:}procedure headforvmode;
begin if curlist.modefield<0 then if curcmd<>36 then offsave else begin
begin if interaction=3 then wakeupterminal;printnl(262);print(681);end;
printesc(517);print(1075);begin helpptr:=2;helpline[1]:=1076;
helpline[0]:=1077;end;error;end else begin backinput;curtok:=partoken;
backinput;curinput.indexfield:=4;end;end;{:1095}{1096:}
procedure endgraf;
begin if curlist.modefield=102 then begin if curlist.headfield=curlist.
tailfield then popnest else linebreak(eqtb[12669].int);normalparagraph;
errorcount:=0;end;end;{:1096}{1099:}procedure begininsertoradjust;
begin if curcmd=38 then curval:=255 else begin scaneightbitint;
if curval=255 then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(1078);end;printesc(327);printint(255);
begin helpptr:=1;helpline[0]:=1079;end;error;curval:=0;end;end;
savestack[saveptr+0].int:=curval;incr(saveptr);newsavelevel(11);
scanleftbrace;normalparagraph;pushnest;curlist.modefield:=-1;
curlist.auxfield.int:=-65536000;end;{:1099}{1101:}procedure makemark;
var p:halfword;begin p:=scantoks(false,true);p:=getnode(2);
mem[p].hh.b0:=4;mem[p].hh.b1:=0;mem[p+1].int:=defref;
mem[curlist.tailfield].hh.rh:=p;curlist.tailfield:=p;end;{:1101}{1103:}
procedure appendpenalty;begin scanint;
begin mem[curlist.tailfield].hh.rh:=newpenalty(curval);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
if curlist.modefield=1 then buildpage;end;{:1103}{1105:}
procedure deletelast;label 10;var p,q:halfword;m:quarterword;
begin if(curlist.modefield=1)and(curlist.tailfield=curlist.headfield)
then{1106:}begin if(curchr<>10)or(lastglue<>262143)then begin youcant;
begin helpptr:=2;helpline[1]:=1064;helpline[0]:=1080;end;
if curchr=11 then helpline[0]:=(1081)else if curchr<>10 then helpline[0]
:=(1082);error;end;end{:1106}
else begin if not(curlist.tailfield>=himemmin)then if mem[curlist.
tailfield].hh.b0=curchr then begin q:=curlist.headfield;repeat p:=q;
if not(q>=himemmin)then if mem[q].hh.b0=7 then begin for m:=1 to mem[q].
hh.b1 do p:=mem[p].hh.rh;if p=curlist.tailfield then goto 10;end;
q:=mem[p].hh.rh;until q=curlist.tailfield;mem[p].hh.rh:=0;
flushnodelist(curlist.tailfield);curlist.tailfield:=p;end;end;10:end;
{:1105}{1110:}procedure unpackage;label 10;var p:halfword;c:0..1;
begin c:=curchr;scaneightbitint;p:=eqtb[11078+curval].hh.rh;
if p=0 then goto 10;
if(abs(curlist.modefield)=203)or((abs(curlist.modefield)=1)and(mem[p].hh
.b0<>1))or((abs(curlist.modefield)=102)and(mem[p].hh.b0<>0))then begin
begin if interaction=3 then wakeupterminal;printnl(262);print(1090);end;
begin helpptr:=3;helpline[2]:=1091;helpline[1]:=1092;helpline[0]:=1093;
end;error;goto 10;end;
if c=1 then mem[curlist.tailfield].hh.rh:=copynodelist(mem[p+5].hh.rh)
else begin mem[curlist.tailfield].hh.rh:=mem[p+5].hh.rh;
eqtb[11078+curval].hh.rh:=0;freenode(p,7);end;
while mem[curlist.tailfield].hh.rh<>0 do curlist.tailfield:=mem[curlist.
tailfield].hh.rh;10:end;{:1110}{1113:}procedure appenditaliccorrection;
label 10;var p:halfword;f:internalfontnumber;
begin if curlist.tailfield<>curlist.headfield then begin if(curlist.
tailfield>=himemmin)then p:=curlist.tailfield else if mem[curlist.
tailfield].hh.b0=6 then p:=curlist.tailfield+1 else goto 10;
f:=mem[p].hh.b0;
begin mem[curlist.tailfield].hh.rh:=newkern(fontinfo[italicbase[f]+(
fontinfo[charbase[f]+mem[p].hh.b1].qqqq.b2)div 4].int);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
mem[curlist.tailfield].hh.b1:=1;end;10:end;{:1113}{1117:}
procedure appenddiscretionary;var c:integer;
begin begin mem[curlist.tailfield].hh.rh:=newdisc;
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
if curchr=1 then begin c:=hyphenchar[eqtb[11334].hh.rh];
if c>=0 then if c<256 then mem[curlist.tailfield+1].hh.lh:=newcharacter(
eqtb[11334].hh.rh,c);end else begin incr(saveptr);
savestack[saveptr-1].int:=0;newsavelevel(10);scanleftbrace;pushnest;
curlist.modefield:=-102;curlist.auxfield.hh.lh:=1000;end;end;{:1117}
{1119:}procedure builddiscretionary;label 30,10;var p,q:halfword;
n:integer;begin unsave;{1121:}q:=curlist.headfield;p:=mem[q].hh.rh;n:=0;
while p<>0 do begin if not(p>=himemmin)then if mem[p].hh.b0>2 then if
mem[p].hh.b0<>11 then if mem[p].hh.b0<>6 then begin begin if interaction
=3 then wakeupterminal;printnl(262);print(1100);end;begin helpptr:=1;
helpline[0]:=1101;end;error;begindiagnostic;printnl(1102);showbox(p);
enddiagnostic(true);flushnodelist(p);mem[q].hh.rh:=0;goto 30;end;q:=p;
p:=mem[q].hh.rh;incr(n);end;30:{:1121};p:=mem[curlist.headfield].hh.rh;
popnest;
case savestack[saveptr-1].int of 0:mem[curlist.tailfield+1].hh.lh:=p;
1:mem[curlist.tailfield+1].hh.rh:=p;2:{1120:}
begin if(n>0)and(abs(curlist.modefield)=203)then begin begin if
interaction=3 then wakeupterminal;printnl(262);print(1094);end;
printesc(346);begin helpptr:=2;helpline[1]:=1095;helpline[0]:=1096;end;
flushnodelist(p);n:=0;error;end else mem[curlist.tailfield].hh.rh:=p;
if n<=511 then mem[curlist.tailfield].hh.b1:=n else begin begin if
interaction=3 then wakeupterminal;printnl(262);print(1097);end;
begin helpptr:=2;helpline[1]:=1098;helpline[0]:=1099;end;error;end;
if n>0 then curlist.tailfield:=q;decr(saveptr);goto 10;end{:1120};end;
incr(savestack[saveptr-1].int);newsavelevel(10);scanleftbrace;pushnest;
curlist.modefield:=-102;curlist.auxfield.hh.lh:=1000;10:end;{:1119}
{1123:}procedure makeaccent;var s,t:real;p,q,r:halfword;
f:internalfontnumber;a,h,x,w,delta:scaled;i:fourquarters;
begin scancharnum;f:=eqtb[11334].hh.rh;p:=newcharacter(f,curval);
if p<>0 then begin x:=fontinfo[5+parambase[f]].int;
s:=fontinfo[1+parambase[f]].int/65536.0;
a:=fontinfo[widthbase[f]+fontinfo[charbase[f]+mem[p].hh.b1].qqqq.b0].int
;doassignments;{1124:}q:=0;f:=eqtb[11334].hh.rh;
if(curcmd=11)or(curcmd=12)or(curcmd=68)then q:=newcharacter(f,curchr)
else if curcmd=16 then begin scancharnum;q:=newcharacter(f,curval);
end else backinput{:1124};if q<>0 then{1125:}
begin t:=fontinfo[1+parambase[f]].int/65536.0;
i:=fontinfo[charbase[f]+mem[q].hh.b1].qqqq;
w:=fontinfo[widthbase[f]+i.b0].int;
h:=fontinfo[heightbase[f]+(i.b1)div 16].int;
if h<>x then begin p:=hpack(p,0,1);mem[p+4].int:=x-h;end;
delta:=round((w-a)/2.0+h*t-x*s);r:=newkern(delta);mem[r].hh.b1:=2;
mem[curlist.tailfield].hh.rh:=r;mem[r].hh.rh:=p;
curlist.tailfield:=newkern(-a-delta);mem[curlist.tailfield].hh.b1:=2;
mem[p].hh.rh:=curlist.tailfield;p:=q;end{:1125};
mem[curlist.tailfield].hh.rh:=p;curlist.tailfield:=p;
curlist.auxfield.hh.lh:=1000;end;end;{:1123}{1127:}procedure alignerror;
begin if abs(alignstate)>2 then{1128:}
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(1107);end;printcmdchr(curcmd,curchr);
if curtok=1062 then begin begin helpptr:=6;helpline[5]:=1108;
helpline[4]:=1109;helpline[3]:=1110;helpline[2]:=1111;helpline[1]:=1112;
helpline[0]:=1113;end;end else begin begin helpptr:=5;helpline[4]:=1108;
helpline[3]:=1114;helpline[2]:=1111;helpline[1]:=1112;helpline[0]:=1113;
end;end;error;end{:1128}else begin backinput;
if alignstate<0 then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(653);end;incr(alignstate);curtok:=379;
end else begin begin if interaction=3 then wakeupterminal;printnl(262);
print(1103);end;decr(alignstate);curtok:=637;end;begin helpptr:=3;
helpline[2]:=1104;helpline[1]:=1105;helpline[0]:=1106;end;inserror;end;
end;{:1127}{1129:}procedure noalignerror;
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(1107);end;printesc(523);begin helpptr:=2;helpline[1]:=1115;
helpline[0]:=1116;end;error;end;procedure omiterror;
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(1107);end;printesc(526);begin helpptr:=2;helpline[1]:=1117;
helpline[0]:=1116;end;error;end;{:1129}{1131:}procedure doendv;
begin if curgroup=6 then begin endgraf;if fincol then finrow;
end else offsave;end;{:1131}{1135:}procedure cserror;
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(772);end;printesc(501);begin helpptr:=1;helpline[0]:=1119;end;
error;end;{:1135}{1136:}procedure pushmath(c:groupcode);begin pushnest;
curlist.modefield:=-203;curlist.auxfield.int:=0;newsavelevel(c);end;
{:1136}{1138:}procedure initmath;label 21,40,45,30;var w:scaled;
l:scaled;s:scaled;p:halfword;q:halfword;f:internalfontnumber;n:integer;
v:scaled;d:scaled;begin gettoken;
if(curcmd=3)and(curlist.modefield>0)then{1145:}
begin if curlist.headfield=curlist.tailfield then begin popnest;
w:=-1073741823;end else begin linebreak(eqtb[12670].int);{1146:}
v:=mem[justbox+4].int+2*fontinfo[6+parambase[eqtb[11334].hh.rh]].int;
w:=-1073741823;p:=mem[justbox+5].hh.rh;while p<>0 do begin{1147:}
21:if(p>=himemmin)then begin f:=mem[p].hh.b0;
d:=fontinfo[widthbase[f]+fontinfo[charbase[f]+mem[p].hh.b1].qqqq.b0].int
;goto 40;end;case mem[p].hh.b0 of 0,1,2:begin d:=mem[p+1].int;goto 40;
end;6:{652:}begin mem[memtop-12]:=mem[p+1];
mem[memtop-12].hh.rh:=mem[p].hh.rh;p:=memtop-12;goto 21;end{:652};
11,9:d:=mem[p+1].int;10:{1148:}begin q:=mem[p+1].hh.lh;d:=mem[q+1].int;
if mem[justbox+5].hh.b0=1 then begin if(mem[justbox+5].hh.b1=mem[q].hh.
b0)and(mem[q+2].int<>0)then v:=1073741823;
end else if mem[justbox+5].hh.b0=2 then begin if(mem[justbox+5].hh.b1=
mem[q].hh.b1)and(mem[q+3].int<>0)then v:=1073741823;end;
if mem[p].hh.b1>=100 then goto 40;end{:1148};8:{1361:}d:=0{:1361};
others:d:=0 end{:1147};if v<1073741823 then v:=v+d;goto 45;
40:if v<1073741823 then begin v:=v+d;w:=v;end else begin w:=1073741823;
goto 30;end;45:p:=mem[p].hh.rh;end;30:{:1146};end;{1149:}
if eqtb[10812].hh.rh=0 then if(eqtb[13247].int<>0)and(((eqtb[12704].int
>=0)and(curlist.pgfield+2>eqtb[12704].int))or(curlist.pgfield+1<-eqtb[
12704].int))then begin l:=eqtb[13233].int-abs(eqtb[13247].int);
if eqtb[13247].int>0 then s:=eqtb[13247].int else s:=0;
end else begin l:=eqtb[13233].int;s:=0;
end else begin n:=mem[eqtb[10812].hh.rh].hh.lh;
if curlist.pgfield+2>=n then p:=eqtb[10812].hh.rh+2*n else p:=eqtb[10812
].hh.rh+2*(curlist.pgfield+2);s:=mem[p-1].int;l:=mem[p].int;end{:1149};
pushmath(15);curlist.modefield:=203;eqworddefine(12707,-1);
eqworddefine(13243,w);eqworddefine(13244,l);eqworddefine(13245,s);
if eqtb[10816].hh.rh<>0 then begintokenlist(eqtb[10816].hh.rh,9);
if nestptr=1 then buildpage;end{:1145}else begin backinput;{1139:}
begin pushmath(15);eqworddefine(12707,-1);
if eqtb[10815].hh.rh<>0 then begintokenlist(eqtb[10815].hh.rh,8);
end{:1139};end;end;{:1138}{1142:}procedure starteqno;
begin savestack[saveptr+0].int:=curchr;incr(saveptr);{1139:}
begin pushmath(15);eqworddefine(12707,-1);
if eqtb[10815].hh.rh<>0 then begintokenlist(eqtb[10815].hh.rh,8);
end{:1139};end;{:1142}{1151:}procedure scanmath(p:halfword);
label 20,21,10;var c:integer;begin 20:{404:}repeat getxtoken;
until(curcmd<>10)and(curcmd<>0){:404};
21:case curcmd of 11,12,68:begin c:=eqtb[12407+curchr].hh.rh;
if c=32768 then begin{1152:}begin curcs:=curchr+1;
curcmd:=eqtb[curcs].hh.b0;curchr:=eqtb[curcs].hh.rh;xtoken;backinput;
end{:1152};goto 20;end;end;16:begin scancharnum;curchr:=curval;
curcmd:=68;goto 21;end;17:begin scanfifteenbitint;c:=curval;end;
69:c:=curchr;15:begin scantwentysevenbitint;c:=curval div 4096;end;
others:{1153:}begin backinput;scanleftbrace;savestack[saveptr+0].int:=p;
incr(saveptr);pushmath(9);goto 10;end{:1153}end;mem[p].hh.rh:=1;
mem[p].hh.b1:=c mod 256;
if(c>=28672)and((eqtb[12707].int>=0)and(eqtb[12707].int<16))then mem[p].
hh.b0:=eqtb[12707].int else mem[p].hh.b0:=(c div 256)mod 16;10:end;
{:1151}{1155:}procedure setmathchar(c:integer);var p:halfword;
begin if c>=32768 then{1152:}begin curcs:=curchr+1;
curcmd:=eqtb[curcs].hh.b0;curchr:=eqtb[curcs].hh.rh;xtoken;backinput;
end{:1152}else begin p:=newnoad;mem[p+1].hh.rh:=1;
mem[p+1].hh.b1:=c mod 256;mem[p+1].hh.b0:=(c div 256)mod 16;
if c>=28672 then begin if((eqtb[12707].int>=0)and(eqtb[12707].int<16))
then mem[p+1].hh.b0:=eqtb[12707].int;mem[p].hh.b0:=16;
end else mem[p].hh.b0:=16+(c div 4096);mem[curlist.tailfield].hh.rh:=p;
curlist.tailfield:=p;end;end;{:1155}{1159:}procedure mathlimitswitch;
label 10;
begin if curlist.headfield<>curlist.tailfield then if mem[curlist.
tailfield].hh.b0=17 then begin mem[curlist.tailfield].hh.b1:=curchr;
goto 10;end;begin if interaction=3 then wakeupterminal;printnl(262);
print(1123);end;begin helpptr:=1;helpline[0]:=1124;end;error;10:end;
{:1159}{1160:}procedure scandelimiter(p:halfword;r:boolean);
begin if r then scantwentysevenbitint else begin{404:}repeat getxtoken;
until(curcmd<>10)and(curcmd<>0){:404};
case curcmd of 11,12:curval:=eqtb[12974+curchr].int;
15:scantwentysevenbitint;others:curval:=-1 end;end;
if curval<0 then{1161:}begin begin if interaction=3 then wakeupterminal;
printnl(262);print(1125);end;begin helpptr:=6;helpline[5]:=1126;
helpline[4]:=1127;helpline[3]:=1128;helpline[2]:=1129;helpline[1]:=1130;
helpline[0]:=1131;end;backerror;curval:=0;end{:1161};
mem[p].qqqq.b0:=(curval div 1048576)mod 16;
mem[p].qqqq.b1:=(curval div 4096)mod 256;
mem[p].qqqq.b2:=(curval div 256)mod 16;mem[p].qqqq.b3:=curval mod 256;
end;{:1160}{1163:}procedure mathradical;
begin begin mem[curlist.tailfield].hh.rh:=getnode(5);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
mem[curlist.tailfield].hh.b0:=24;mem[curlist.tailfield].hh.b1:=0;
mem[curlist.tailfield+1].hh:=emptyfield;
mem[curlist.tailfield+3].hh:=emptyfield;
mem[curlist.tailfield+2].hh:=emptyfield;
scandelimiter(curlist.tailfield+4,true);scanmath(curlist.tailfield+1);
end;{:1163}{1165:}procedure mathac;begin if curcmd=45 then{1166:}
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(1132);end;printesc(519);print(1133);begin helpptr:=2;
helpline[1]:=1134;helpline[0]:=1135;end;error;end{:1166};
begin mem[curlist.tailfield].hh.rh:=getnode(5);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
mem[curlist.tailfield].hh.b0:=28;mem[curlist.tailfield].hh.b1:=0;
mem[curlist.tailfield+1].hh:=emptyfield;
mem[curlist.tailfield+3].hh:=emptyfield;
mem[curlist.tailfield+2].hh:=emptyfield;
mem[curlist.tailfield+4].hh.rh:=1;scanfifteenbitint;
mem[curlist.tailfield+4].hh.b1:=curval mod 256;
if(curval>=28672)and((eqtb[12707].int>=0)and(eqtb[12707].int<16))then
mem[curlist.tailfield+4].hh.b0:=eqtb[12707].int else mem[curlist.
tailfield+4].hh.b0:=(curval div 256)mod 16;
scanmath(curlist.tailfield+1);end;{:1165}{1172:}procedure appendchoices;
begin begin mem[curlist.tailfield].hh.rh:=newchoice;
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;incr(saveptr);
savestack[saveptr-1].int:=0;pushmath(13);scanleftbrace;end;{:1172}
{1174:}{1184:}function finmlist(p:halfword):halfword;var q:halfword;
begin if curlist.auxfield.int<>0 then{1185:}
begin mem[curlist.auxfield.int+3].hh.rh:=3;
mem[curlist.auxfield.int+3].hh.lh:=mem[curlist.headfield].hh.rh;
if p=0 then q:=curlist.auxfield.int else begin q:=mem[curlist.auxfield.
int+2].hh.lh;if mem[q].hh.b0<>30 then confusion(870);
mem[curlist.auxfield.int+2].hh.lh:=mem[q].hh.rh;
mem[q].hh.rh:=curlist.auxfield.int;mem[curlist.auxfield.int].hh.rh:=p;
end;end{:1185}else begin mem[curlist.tailfield].hh.rh:=p;
q:=mem[curlist.headfield].hh.rh;end;popnest;finmlist:=q;end;{:1184}
procedure buildchoices;label 10;var p:halfword;begin unsave;
p:=finmlist(0);
case savestack[saveptr-1].int of 0:mem[curlist.tailfield+1].hh.lh:=p;
1:mem[curlist.tailfield+1].hh.rh:=p;2:mem[curlist.tailfield+2].hh.lh:=p;
3:begin mem[curlist.tailfield+2].hh.rh:=p;decr(saveptr);goto 10;end;end;
incr(savestack[saveptr-1].int);pushmath(13);scanleftbrace;10:end;{:1174}
{1176:}procedure subsup;var t:smallnumber;p:halfword;begin t:=0;p:=0;
if curlist.tailfield<>curlist.headfield then if(mem[curlist.tailfield].
hh.b0>=16)and(mem[curlist.tailfield].hh.b0<30)then begin p:=curlist.
tailfield+2+curcmd-7;t:=mem[p].hh.rh;end;if(p=0)or(t<>0)then{1177:}
begin begin mem[curlist.tailfield].hh.rh:=newnoad;
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
p:=curlist.tailfield+2+curcmd-7;
if t<>0 then begin if curcmd=7 then begin begin if interaction=3 then
wakeupterminal;printnl(262);print(1136);end;begin helpptr:=1;
helpline[0]:=1137;end;
end else begin begin if interaction=3 then wakeupterminal;printnl(262);
print(1138);end;begin helpptr:=1;helpline[0]:=1139;end;end;error;end;
end{:1177};scanmath(p);end;{:1176}{1181:}procedure mathfraction;
var c:smallnumber;begin c:=curchr;if curlist.auxfield.int<>0 then{1183:}
begin if c>=3 then begin scandelimiter(memtop-12,false);
scandelimiter(memtop-12,false);end;
if c mod 3=0 then scandimen(false,false,false);
begin if interaction=3 then wakeupterminal;printnl(262);print(1146);end;
begin helpptr:=3;helpline[2]:=1147;helpline[1]:=1148;helpline[0]:=1149;
end;error;end{:1183}else begin curlist.auxfield.int:=getnode(6);
mem[curlist.auxfield.int].hh.b0:=25;mem[curlist.auxfield.int].hh.b1:=0;
mem[curlist.auxfield.int+2].hh.rh:=3;
mem[curlist.auxfield.int+2].hh.lh:=mem[curlist.headfield].hh.rh;
mem[curlist.auxfield.int+3].hh:=emptyfield;
mem[curlist.auxfield.int+4].qqqq:=nulldelimiter;
mem[curlist.auxfield.int+5].qqqq:=nulldelimiter;
mem[curlist.headfield].hh.rh:=0;curlist.tailfield:=curlist.headfield;
{1182:}if c>=3 then begin scandelimiter(curlist.auxfield.int+4,false);
scandelimiter(curlist.auxfield.int+5,false);end;
case c mod 3 of 0:begin scandimen(false,false,false);
mem[curlist.auxfield.int+1].int:=curval;end;
1:mem[curlist.auxfield.int+1].int:=1073741824;
2:mem[curlist.auxfield.int+1].int:=0;end{:1182};end;end;{:1181}{1191:}
procedure mathleftright;var t:smallnumber;p:halfword;begin t:=curchr;
if(t=31)and(curgroup<>16)then{1192:}
begin if curgroup=15 then begin scandelimiter(memtop-12,false);
begin if interaction=3 then wakeupterminal;printnl(262);print(772);end;
printesc(870);begin helpptr:=1;helpline[0]:=1150;end;error;
end else offsave;end{:1192}else begin p:=newnoad;mem[p].hh.b0:=t;
scandelimiter(p+1,false);if t=30 then begin pushmath(16);
mem[curlist.headfield].hh.rh:=p;curlist.tailfield:=p;
end else begin p:=finmlist(p);unsave;
begin mem[curlist.tailfield].hh.rh:=newnoad;
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
mem[curlist.tailfield].hh.b0:=23;mem[curlist.tailfield+1].hh.rh:=3;
mem[curlist.tailfield+1].hh.lh:=p;end;end;end;{:1191}{1194:}
procedure aftermath;var l:boolean;danger:boolean;m:integer;p:halfword;
a:halfword;{1198:}b:halfword;w:scaled;z:scaled;e:scaled;q:scaled;
d:scaled;s:scaled;g1,g2:smallnumber;r:halfword;t:halfword;{:1198}
begin danger:=false;{1195:}
if(fontparams[eqtb[11337].hh.rh]<22)or(fontparams[eqtb[11353].hh.rh]<22)
or(fontparams[eqtb[11369].hh.rh]<22)then begin begin if interaction=3
then wakeupterminal;printnl(262);print(1151);end;begin helpptr:=3;
helpline[2]:=1152;helpline[1]:=1153;helpline[0]:=1154;end;error;
flushmath;danger:=true;
end else if(fontparams[eqtb[11338].hh.rh]<13)or(fontparams[eqtb[11354].
hh.rh]<13)or(fontparams[eqtb[11370].hh.rh]<13)then begin begin if
interaction=3 then wakeupterminal;printnl(262);print(1155);end;
begin helpptr:=3;helpline[2]:=1156;helpline[1]:=1157;helpline[0]:=1158;
end;error;flushmath;danger:=true;end{:1195};m:=curlist.modefield;
l:=false;p:=finmlist(0);if curlist.modefield=-m then begin{1197:}
begin getxtoken;
if curcmd<>3 then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(1159);end;begin helpptr:=2;helpline[1]:=1160;
helpline[0]:=1161;end;backerror;end;end{:1197};curmlist:=p;curstyle:=2;
mlistpenalties:=false;mlisttohlist;a:=hpack(mem[memtop-3].hh.rh,0,1);
unsave;decr(saveptr);if savestack[saveptr+0].int=1 then l:=true;
danger:=false;{1195:}
if(fontparams[eqtb[11337].hh.rh]<22)or(fontparams[eqtb[11353].hh.rh]<22)
or(fontparams[eqtb[11369].hh.rh]<22)then begin begin if interaction=3
then wakeupterminal;printnl(262);print(1151);end;begin helpptr:=3;
helpline[2]:=1152;helpline[1]:=1153;helpline[0]:=1154;end;error;
flushmath;danger:=true;
end else if(fontparams[eqtb[11338].hh.rh]<13)or(fontparams[eqtb[11354].
hh.rh]<13)or(fontparams[eqtb[11370].hh.rh]<13)then begin begin if
interaction=3 then wakeupterminal;printnl(262);print(1155);end;
begin helpptr:=3;helpline[2]:=1156;helpline[1]:=1157;helpline[0]:=1158;
end;error;flushmath;danger:=true;end{:1195};m:=curlist.modefield;
p:=finmlist(0);end else a:=0;if m<0 then{1196:}
begin begin mem[curlist.tailfield].hh.rh:=newmath(eqtb[13231].int,0);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;curmlist:=p;
curstyle:=2;mlistpenalties:=(curlist.modefield>0);mlisttohlist;
mem[curlist.tailfield].hh.rh:=mem[memtop-3].hh.rh;
while mem[curlist.tailfield].hh.rh<>0 do curlist.tailfield:=mem[curlist.
tailfield].hh.rh;
begin mem[curlist.tailfield].hh.rh:=newmath(eqtb[13231].int,1);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
curlist.auxfield.hh.lh:=1000;unsave;end{:1196}
else begin if a=0 then{1197:}begin getxtoken;
if curcmd<>3 then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(1159);end;begin helpptr:=2;helpline[1]:=1160;
helpline[0]:=1161;end;backerror;end;end{:1197};{1199:}curmlist:=p;
curstyle:=0;mlistpenalties:=false;mlisttohlist;p:=mem[memtop-3].hh.rh;
adjusttail:=memtop-5;b:=hpack(p,0,1);p:=mem[b+5].hh.rh;t:=adjusttail;
adjusttail:=0;w:=mem[b+1].int;z:=eqtb[13244].int;s:=eqtb[13245].int;
if(a=0)or danger then begin e:=0;q:=0;end else begin e:=mem[a+1].int;
q:=e+fontinfo[6+parambase[eqtb[11337].hh.rh]].int;end;
if w+q>z then{1201:}
begin if(e<>0)and((w-totalshrink[0]+q<=z)or(totalshrink[1]<>0)or(
totalshrink[2]<>0)or(totalshrink[3]<>0))then begin freenode(b,7);
b:=hpack(p,z-q,0);end else begin e:=0;if w>z then begin freenode(b,7);
b:=hpack(p,z,0);end;end;w:=mem[b+1].int;end{:1201};{1202:}d:=half(z-w);
if(e>0)and(d<2*e)then begin d:=half(z-w-e);
if p<>0 then if not(p>=himemmin)then if mem[p].hh.b0=10 then d:=0;
end{:1202};{1203:}
begin mem[curlist.tailfield].hh.rh:=newpenalty(eqtb[12674].int);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
if(d+s<=eqtb[13243].int)or l then begin g1:=3;g2:=4;
end else begin g1:=5;g2:=6;end;if l and(e=0)then begin mem[a+4].int:=s;
appendtovlist(a);begin mem[curlist.tailfield].hh.rh:=newpenalty(10000);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
end else begin mem[curlist.tailfield].hh.rh:=newparamglue(g1);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end{:1203};{1204:}
if e<>0 then begin r:=newkern(z-w-e-d);if l then begin mem[a].hh.rh:=r;
mem[r].hh.rh:=b;b:=a;d:=0;end else begin mem[b].hh.rh:=r;
mem[r].hh.rh:=a;end;b:=hpack(b,0,1);end;mem[b+4].int:=s+d;
appendtovlist(b){:1204};{1205:}
if(a<>0)and(e=0)and not l then begin begin mem[curlist.tailfield].hh.rh
:=newpenalty(10000);curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
mem[a+4].int:=s+z-mem[a+1].int;appendtovlist(a);g2:=0;end;
if t<>memtop-5 then begin mem[curlist.tailfield].hh.rh:=mem[memtop-5].hh
.rh;curlist.tailfield:=t;end;
begin mem[curlist.tailfield].hh.rh:=newpenalty(eqtb[12675].int);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
if g2>0 then begin mem[curlist.tailfield].hh.rh:=newparamglue(g2);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end{:1205};
resumeafterdisplay{:1199};end;end;{:1194}{1200:}
procedure resumeafterdisplay;begin if curgroup<>15 then confusion(1162);
unsave;curlist.pgfield:=curlist.pgfield+3;pushnest;
curlist.modefield:=102;curlist.auxfield.hh.lh:=1000;
curlist.auxfield.hh.rh:=0;{443:}begin getxtoken;
if curcmd<>10 then backinput;end{:443};if nestptr=1 then buildpage;end;
{:1200}{1211:}{1215:}procedure getrtoken;label 20;
begin 20:repeat gettoken;until curtok<>2592;
if(curcs=0)or(curcs>10014)then begin begin if interaction=3 then
wakeupterminal;printnl(262);print(1177);end;begin helpptr:=5;
helpline[4]:=1178;helpline[3]:=1179;helpline[2]:=1180;helpline[1]:=1181;
helpline[0]:=1182;end;if curcs=0 then backinput;curtok:=14109;inserror;
goto 20;end;end;{:1215}{1229:}procedure trapzeroglue;
begin if(mem[curval+1].int=0)and(mem[curval+2].int=0)and(mem[curval+3].
int=0)then begin incr(mem[0].hh.rh);deleteglueref(curval);curval:=0;end;
end;{:1229}{1236:}procedure doregistercommand(a:smallnumber);
label 40,10;var l,q,r,s:halfword;p:0..3;begin q:=curcmd;{1237:}
begin if q<>89 then begin getxtoken;
if(curcmd>=73)and(curcmd<=76)then begin l:=curchr;p:=curcmd-73;goto 40;
end;if curcmd<>89 then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(681);end;printcmdchr(curcmd,curchr);print(682);
printcmdchr(q,0);begin helpptr:=1;helpline[0]:=1203;end;error;goto 10;
end;end;p:=curchr;scaneightbitint;case p of 0:l:=curval+12718;
1:l:=curval+13251;2:l:=curval+10300;3:l:=curval+10556;end;end;40:{:1237}
;if q=89 then scanoptionalequals else if scankeyword(1199)then;
aritherror:=false;if q<91 then{1238:}
if p<2 then begin if p=0 then scanint else scandimen(false,false,false);
if q=90 then curval:=curval+eqtb[l].int;end else begin scanglue(p);
if q=90 then{1239:}begin q:=newspec(curval);r:=eqtb[l].hh.rh;
deleteglueref(curval);mem[q+1].int:=mem[q+1].int+mem[r+1].int;
if mem[q+2].int=0 then mem[q].hh.b0:=0;
if mem[q].hh.b0=mem[r].hh.b0 then mem[q+2].int:=mem[q+2].int+mem[r+2].
int else if(mem[q].hh.b0<mem[r].hh.b0)and(mem[r+2].int<>0)then begin mem
[q+2].int:=mem[r+2].int;mem[q].hh.b0:=mem[r].hh.b0;end;
if mem[q+3].int=0 then mem[q].hh.b1:=0;
if mem[q].hh.b1=mem[r].hh.b1 then mem[q+3].int:=mem[q+3].int+mem[r+3].
int else if(mem[q].hh.b1<mem[r].hh.b1)and(mem[r+3].int<>0)then begin mem
[q+3].int:=mem[r+3].int;mem[q].hh.b1:=mem[r].hh.b1;end;curval:=q;
end{:1239};end{:1238}else{1240:}begin scanint;
if p<2 then if q=91 then if p=0 then curval:=multandadd(eqtb[l].int,
curval,0,2147483647)else curval:=multandadd(eqtb[l].int,curval,0,
1073741823)else curval:=xovern(eqtb[l].int,curval)else begin s:=eqtb[l].
hh.rh;r:=newspec(s);
if q=91 then begin mem[r+1].int:=multandadd(mem[s+1].int,curval,0,
1073741823);mem[r+2].int:=multandadd(mem[s+2].int,curval,0,1073741823);
mem[r+3].int:=multandadd(mem[s+3].int,curval,0,1073741823);
end else begin mem[r+1].int:=xovern(mem[s+1].int,curval);
mem[r+2].int:=xovern(mem[s+2].int,curval);
mem[r+3].int:=xovern(mem[s+3].int,curval);end;curval:=r;end;end{:1240};
if aritherror then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(1200);end;begin helpptr:=2;helpline[1]:=1201;
helpline[0]:=1202;end;error;goto 10;end;
if p<2 then if(a>=4)then geqworddefine(l,curval)else eqworddefine(l,
curval)else begin trapzeroglue;
if(a>=4)then geqdefine(l,117,curval)else eqdefine(l,117,curval);end;
10:end;{:1236}{1243:}procedure alteraux;var c:halfword;
begin if curchr<>abs(curlist.modefield)then reportillegalcase else begin
c:=curchr;scanoptionalequals;
if c=1 then begin scandimen(false,false,false);
curlist.auxfield.int:=curval;end else begin scanint;
if(curval<=0)or(curval>32767)then begin begin if interaction=3 then
wakeupterminal;printnl(262);print(1204);end;begin helpptr:=1;
helpline[0]:=1205;end;interror(curval);
end else curlist.auxfield.hh.lh:=curval;end;end;end;{:1243}{1244:}
procedure alterprevgraf;var p:0..nestsize;begin nest[nestptr]:=curlist;
p:=nestptr;while abs(nest[p].modefield)<>1 do decr(p);
scanoptionalequals;scanint;
if curval<0 then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(948);end;printesc(528);begin helpptr:=1;
helpline[0]:=1206;end;interror(curval);
end else begin nest[p].pgfield:=curval;curlist:=nest[nestptr];end;end;
{:1244}{1245:}procedure alterpagesofar;var c:0..7;begin c:=curchr;
scanoptionalequals;scandimen(false,false,false);pagesofar[c]:=curval;
end;{:1245}{1246:}procedure alterinteger;var c:0..1;begin c:=curchr;
scanoptionalequals;scanint;
if c=0 then deadcycles:=curval else insertpenalties:=curval;end;{:1246}
{1247:}procedure alterboxdimen;var c:smallnumber;b:eightbits;
begin c:=curchr;scaneightbitint;b:=curval;scanoptionalequals;
scandimen(false,false,false);
if eqtb[11078+b].hh.rh<>0 then mem[eqtb[11078+b].hh.rh+c].int:=curval;
end;{:1247}{1257:}procedure newfont(a:smallnumber);label 50;
var u:halfword;s:scaled;f:internalfontnumber;t:strnumber;
oldsetting:0..21;begin if jobname=0 then openlogfile;getrtoken;u:=curcs;
if u>=514 then t:=hash[u].rh else if u>=257 then if u=513 then t:=1210
else t:=u-257 else begin oldsetting:=selector;selector:=21;print(1210);
print(u-1);selector:=oldsetting;
begin if poolptr+1>poolsize then overflow(257,poolsize-initpoolptr);end;
t:=makestring;end;if(a>=4)then geqdefine(u,87,0)else eqdefine(u,87,0);
scanoptionalequals;scanfilename;{1258:}nameinprogress:=true;
if scankeyword(1211)then{1259:}begin scandimen(false,false,false);
s:=curval;if(s<=0)or(s>=134217728)then begin begin if interaction=3 then
wakeupterminal;printnl(262);print(1213);end;printscaled(s);print(1214);
begin helpptr:=2;helpline[1]:=1215;helpline[0]:=1216;end;error;
s:=10*65536;end;end{:1259}else if scankeyword(1212)then begin scanint;
s:=-curval;
if(curval<=0)or(curval>32768)then begin begin if interaction=3 then
wakeupterminal;printnl(262);print(548);end;begin helpptr:=1;
helpline[0]:=549;end;interror(curval);s:=-1000;end;end else s:=-1000;
nameinprogress:=false{:1258};{1260:}
for f:=1 to fontptr do if streqstr(fontname[f],curname)and streqstr(
fontarea[f],curarea)then begin if s>0 then begin if s=fontsize[f]then
goto 50;
end else if fontsize[f]=xnoverd(fontdsize[f],-s,1000)then goto 50;
end{:1260};f:=readfontinfo(u,curname,curarea,s);50:eqtb[u].hh.rh:=f;
eqtb[10024+f]:=eqtb[u];hash[10024+f].rh:=t;end;{:1257}{1265:}
procedure newinteraction;begin println;interaction:=curchr;{75:}
if interaction=0 then selector:=16 else selector:=17{:75};
if logopened then selector:=selector+2;end;{:1265}
procedure prefixedcommand;label 30,10;var a:smallnumber;
f:internalfontnumber;j:halfword;k:fontindex;p,q:halfword;n:integer;
e:boolean;begin a:=0;
while curcmd=93 do begin if not odd(a div curchr)then a:=a+curchr;{404:}
repeat getxtoken;until(curcmd<>10)and(curcmd<>0){:404};
if curcmd<=70 then{1212:}
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(1172);end;printcmdchr(curcmd,curchr);printchar(39);
begin helpptr:=1;helpline[0]:=1173;end;backerror;goto 10;end{:1212};end;
{1213:}
if(curcmd<>97)and(a mod 4<>0)then begin begin if interaction=3 then
wakeupterminal;printnl(262);print(681);end;printesc(1164);print(1174);
printesc(1165);print(1175);printcmdchr(curcmd,curchr);printchar(39);
begin helpptr:=1;helpline[0]:=1176;end;error;end{:1213};{1214:}
if eqtb[12706].int<>0 then if eqtb[12706].int<0 then begin if(a>=4)then
a:=a-4;end else begin if not(a>=4)then a:=a+4;end{:1214};
case curcmd of{1217:}
87:if(a>=4)then geqdefine(11334,120,curchr)else eqdefine(11334,120,
curchr);{:1217}{1218:}
97:begin if odd(curchr)and not(a>=4)and(eqtb[12706].int>=0)then a:=a+4;
e:=(curchr>=2);getrtoken;p:=curcs;q:=scantoks(true,e);
if(a>=4)then geqdefine(p,111+(a mod 4),defref)else eqdefine(p,111+(a mod
4),defref);end;{:1218}{1221:}94:begin n:=curchr;getrtoken;p:=curcs;
if n=0 then begin repeat gettoken;until curcmd<>10;
if curtok=3133 then begin gettoken;if curcmd=10 then gettoken;end;
end else begin gettoken;q:=curtok;gettoken;backinput;curtok:=q;
backinput;end;if curcmd>=111 then incr(mem[curchr].hh.lh);
if(a>=4)then geqdefine(p,curcmd,curchr)else eqdefine(p,curcmd,curchr);
end;{:1221}{1224:}95:begin n:=curchr;getrtoken;p:=curcs;
if(a>=4)then geqdefine(p,0,256)else eqdefine(p,0,256);
scanoptionalequals;case n of 0:begin scancharnum;
if(a>=4)then geqdefine(p,68,curval)else eqdefine(p,68,curval);end;
1:begin scanfifteenbitint;
if(a>=4)then geqdefine(p,69,curval)else eqdefine(p,69,curval);end;
others:begin scaneightbitint;
case n of 2:if(a>=4)then geqdefine(p,73,12718+curval)else eqdefine(p,73,
12718+curval);
3:if(a>=4)then geqdefine(p,74,13251+curval)else eqdefine(p,74,13251+
curval);
4:if(a>=4)then geqdefine(p,75,10300+curval)else eqdefine(p,75,10300+
curval);
5:if(a>=4)then geqdefine(p,76,10556+curval)else eqdefine(p,76,10556+
curval);
6:if(a>=4)then geqdefine(p,72,10822+curval)else eqdefine(p,72,10822+
curval);end;end end;end;{:1224}{1225:}96:begin scanint;n:=curval;
if not scankeyword(835)then begin begin if interaction=3 then
wakeupterminal;printnl(262);print(1066);end;begin helpptr:=2;
helpline[1]:=1193;helpline[0]:=1194;end;error;end;getrtoken;p:=curcs;
readtoks(n,p);
if(a>=4)then geqdefine(p,111,curval)else eqdefine(p,111,curval);end;
{:1225}{1226:}71,72:begin q:=curcs;
if curcmd=71 then begin scaneightbitint;p:=10822+curval;
end else p:=curchr;scanoptionalequals;{404:}repeat getxtoken;
until(curcmd<>10)and(curcmd<>0){:404};if curcmd<>1 then{1227:}
begin if curcmd=71 then begin scaneightbitint;curcmd:=72;
curchr:=10822+curval;end;if curcmd=72 then begin q:=eqtb[curchr].hh.rh;
if q=0 then if(a>=4)then geqdefine(p,101,0)else eqdefine(p,101,0)else
begin incr(mem[q].hh.lh);
if(a>=4)then geqdefine(p,111,q)else eqdefine(p,111,q);end;goto 30;end;
end{:1227};backinput;curcs:=q;q:=scantoks(false,false);
if mem[defref].hh.rh=0 then begin if(a>=4)then geqdefine(p,101,0)else
eqdefine(p,101,0);begin mem[defref].hh.rh:=avail;avail:=defref;
ifdef('STAT')decr(dynused);endif('STAT')end;
end else begin if p=10813 then begin mem[q].hh.rh:=getavail;
q:=mem[q].hh.rh;mem[q].hh.lh:=637;q:=getavail;mem[q].hh.lh:=379;
mem[q].hh.rh:=mem[defref].hh.rh;mem[defref].hh.rh:=q;end;
if(a>=4)then geqdefine(p,111,defref)else eqdefine(p,111,defref);end;end;
{:1226}{1228:}73:begin p:=curchr;scanoptionalequals;scanint;
if(a>=4)then geqworddefine(p,curval)else eqworddefine(p,curval);end;
74:begin p:=curchr;scanoptionalequals;scandimen(false,false,false);
if(a>=4)then geqworddefine(p,curval)else eqworddefine(p,curval);end;
75,76:begin p:=curchr;n:=curcmd;scanoptionalequals;
if n=76 then scanglue(3)else scanglue(2);trapzeroglue;
if(a>=4)then geqdefine(p,117,curval)else eqdefine(p,117,curval);end;
{:1228}{1232:}85:begin{1233:}
if curchr=11383 then n:=15 else if curchr=12407 then n:=32768 else if
curchr=12151 then n:=32767 else if curchr=12974 then n:=16777215 else n
:=255{:1233};p:=curchr;scancharnum;p:=p+curval;scanoptionalequals;
scanint;
if((curval<0)and(p<12974))or(curval>n)then begin begin if interaction=3
then wakeupterminal;printnl(262);print(1195);end;printint(curval);
if p<12974 then print(1196)else print(1197);printint(n);
begin helpptr:=1;helpline[0]:=1198;end;error;curval:=0;end;
if p<12407 then if(a>=4)then geqdefine(p,120,curval)else eqdefine(p,120,
curval)else if p<12974 then if(a>=4)then geqdefine(p,120,curval)else
eqdefine(p,120,curval)else if(a>=4)then geqworddefine(p,curval)else
eqworddefine(p,curval);end;{:1232}{1234:}86:begin p:=curchr;
scanfourbitint;p:=p+curval;scanoptionalequals;scanfontident;
if(a>=4)then geqdefine(p,120,curval)else eqdefine(p,120,curval);end;
{:1234}{1235:}89,90,91,92:doregistercommand(a);{:1235}{1241:}
98:begin scaneightbitint;if(a>=4)then n:=256+curval else n:=curval;
scanoptionalequals;scanbox(1073741824+n);end;{:1241}{1242:}79:alteraux;
80:alterprevgraf;81:alterpagesofar;82:alterinteger;83:alterboxdimen;
{:1242}{1248:}84:begin scanoptionalequals;scanint;n:=curval;
if n<=0 then p:=0 else begin p:=getnode(2*n+1);mem[p].hh.lh:=n;
for j:=1 to n do begin scandimen(false,false,false);
mem[p+2*j-1].int:=curval;scandimen(false,false,false);
mem[p+2*j].int:=curval;end;end;
if(a>=4)then geqdefine(10812,118,p)else eqdefine(10812,118,p);end;
{:1248}{1252:}99:if curchr=1 then begin ifdef('INITEX')newpatterns;
goto 30;endif('INITEX')begin if interaction=3 then wakeupterminal;
printnl(262);print(1207);end;helpptr:=0;error;repeat gettoken;
until curcmd=2;goto 10;end else begin newhyphexceptions;goto 30;end;
{:1252}{1253:}77:begin findfontdimen(true);k:=curval;scanoptionalequals;
scandimen(false,false,false);fontinfo[k].int:=curval;end;
78:begin n:=curchr;scanfontident;f:=curval;scanoptionalequals;scanint;
if n=0 then hyphenchar[f]:=curval else skewchar[f]:=curval;end;{:1253}
{1256:}88:newfont(a);{:1256}{1264:}100:newinteraction;{:1264}
others:confusion(1171)end;30:{1269:}
if aftertoken<>0 then begin curtok:=aftertoken;backinput;aftertoken:=0;
end{:1269};10:end;{:1211}{1270:}procedure doassignments;label 10;
begin while true do begin{404:}repeat getxtoken;
until(curcmd<>10)and(curcmd<>0){:404};if curcmd<=70 then goto 10;
prefixedcommand;end;10:end;{:1270}{1275:}procedure openorclosein;
var c:0..1;n:0..15;begin c:=curchr;scanfourbitint;n:=curval;
if readopen[n]<>2 then begin aclose(readfile[n]);readopen[n]:=2;end;
if c<>0 then begin scanoptionalequals;scanfilename;
if curext=335 then curext:=784;packfilename(curname,curarea,curext);
if aopenin(readfile[n],readpathspec)then readopen[n]:=1;end;end;{:1275}
{1279:}procedure issuemessage;var oldsetting:0..21;c:0..1;s:strnumber;
begin c:=curchr;mem[memtop-12].hh.rh:=scantoks(false,true);
oldsetting:=selector;selector:=21;tokenshow(defref);
selector:=oldsetting;flushlist(defref);
begin if poolptr+1>poolsize then overflow(257,poolsize-initpoolptr);end;
s:=makestring;if c=0 then{1280:}
begin if termoffset+(strstart[s+1]-strstart[s])>maxprintline-2 then
println else if(termoffset>0)or(fileoffset>0)then printchar(32);
print(s);termflush(stdout);end{:1280}else{1283:}
begin begin if interaction=3 then wakeupterminal;printnl(262);print(s);
end;
if eqtb[10821].hh.rh<>0 then useerrhelp:=true else if longhelpseen then
begin helpptr:=1;helpline[0]:=1223;
end else begin if interaction<3 then longhelpseen:=true;
begin helpptr:=4;helpline[3]:=1224;helpline[2]:=1225;helpline[1]:=1226;
helpline[0]:=1227;end;end;error;useerrhelp:=false;end{:1283};
begin decr(strptr);poolptr:=strstart[strptr];end;end;{:1279}{1288:}
procedure shiftcase;var b:halfword;p:halfword;t:halfword;c:eightbits;
begin b:=curchr;p:=scantoks(false,false);p:=mem[defref].hh.rh;
while p<>0 do begin{1289:}t:=mem[p].hh.lh;
if t<4352 then begin c:=t mod 256;
if eqtb[b+c].hh.rh<>0 then mem[p].hh.lh:=t-c+eqtb[b+c].hh.rh;end{:1289};
p:=mem[p].hh.rh;end;begintokenlist(mem[defref].hh.rh,3);
begin mem[defref].hh.rh:=avail;avail:=defref;ifdef('STAT')decr(dynused);
endif('STAT')end;end;{:1288}{1293:}procedure showwhatever;label 50;
var p:halfword;begin case curchr of 3:begin begindiagnostic;
showactivities;end;1:{1296:}begin scaneightbitint;begindiagnostic;
printnl(1245);printint(curval);printchar(61);
if eqtb[11078+curval].hh.rh=0 then print(406)else showbox(eqtb[11078+
curval].hh.rh);end{:1296};0:{1294:}begin gettoken;
if interaction=3 then wakeupterminal;printnl(1239);
if curcs<>0 then begin sprintcs(curcs);printchar(61);end;printmeaning;
goto 50;end{:1294};others:{1297:}begin p:=thetoks;
if interaction=3 then wakeupterminal;printnl(1239);tokenshow(memtop-3);
flushlist(mem[memtop-3].hh.rh);goto 50;end{:1297}end;{1298:}
enddiagnostic(true);begin if interaction=3 then wakeupterminal;
printnl(262);print(1246);end;
if selector=19 then if eqtb[12692].int<=0 then begin selector:=17;
print(1247);selector:=19;end{:1298};
50:if interaction<3 then begin helpptr:=0;decr(errorcount);
end else if eqtb[12692].int>0 then begin begin helpptr:=3;
helpline[2]:=1234;helpline[1]:=1235;helpline[0]:=1236;end;
end else begin begin helpptr:=5;helpline[4]:=1234;helpline[3]:=1235;
helpline[2]:=1236;helpline[1]:=1237;helpline[0]:=1238;end;end;error;end;
{:1293}{1302:}ifdef('INITEX')procedure storefmtfile;label 41,42,31,32;
var j,k,l:integer;p,q:halfword;x:integer;begin{1304:}
if saveptr<>0 then begin begin if interaction=3 then wakeupterminal;
printnl(262);print(1249);end;begin helpptr:=1;helpline[0]:=1250;end;
begin if interaction=3 then interaction:=2;if logopened then error;
ifdef('DEBUG')if interaction>0 then debughelp;endif('DEBUG')history:=3;
jumpout;end;end{:1304};{1328:}selector:=21;print(1263);print(jobname);
printchar(32);printint(eqtb[12686].int mod 100);printchar(46);
printint(eqtb[12685].int);printchar(46);printint(eqtb[12684].int);
printchar(41);if interaction=0 then selector:=18 else selector:=19;
begin if poolptr+1>poolsize then overflow(257,poolsize-initpoolptr);end;
formatident:=makestring;packjobname(779);
while not wopenout(fmtfile)do promptfilename(1264,779);printnl(1265);
print(wmakenamestring(fmtfile));begin decr(strptr);
poolptr:=strstart[strptr];end;printnl(formatident){:1328};{1307:}
putfmtint(127541235);putfmtint(0);putfmtint(memtop);putfmtint(13506);
putfmtint(7919);putfmtint(607){:1307};{1309:}putfmtint(poolptr);
putfmtint(strptr);dumpthings(strstart[0],strptr+1);
dumpthings(strpool[0],poolptr);println;printint(strptr);print(1251);
printint(poolptr){:1309};{1311:}sortavail;varused:=0;
putfmtint(lomemmax);putfmtint(rover);p:=0;q:=rover;x:=0;
repeat dumpthings(mem[p],q+2-p);x:=x+q+2-p;varused:=varused+q-p;
p:=q+mem[q].hh.lh;q:=mem[q+1].hh.rh;until q=rover;
varused:=varused+lomemmax-p;dynused:=memend+1-himemmin;
dumpthings(mem[p],lomemmax+1-p);x:=x+lomemmax+1-p;putfmtint(himemmin);
putfmtint(avail);dumpthings(mem[himemmin],memend+1-himemmin);
x:=x+memend+1-himemmin;p:=avail;while p<>0 do begin decr(dynused);
p:=mem[p].hh.rh;end;putfmtint(varused);putfmtint(dynused);println;
printint(x);print(1252);printint(varused);printchar(38);
printint(dynused){:1311};{1313:}{1315:}k:=1;repeat j:=k;
while j<12662 do begin if(eqtb[j].hh.rh=eqtb[j+1].hh.rh)and(eqtb[j].hh.
b0=eqtb[j+1].hh.b0)and(eqtb[j].hh.b1=eqtb[j+1].hh.b1)then goto 41;
incr(j);end;l:=12663;goto 31;41:incr(j);l:=j;
while j<12662 do begin if(eqtb[j].hh.rh<>eqtb[j+1].hh.rh)or(eqtb[j].hh.
b0<>eqtb[j+1].hh.b0)or(eqtb[j].hh.b1<>eqtb[j+1].hh.b1)then goto 31;
incr(j);end;31:putfmtint(l-k);dumpthings(eqtb[k],l-k);k:=j+1;
putfmtint(k-l);until k=12663{:1315};{1316:}repeat j:=k;
while j<13506 do begin if eqtb[j].int=eqtb[j+1].int then goto 42;
incr(j);end;l:=13507;goto 32;42:incr(j);l:=j;
while j<13506 do begin if eqtb[j].int<>eqtb[j+1].int then goto 32;
incr(j);end;32:putfmtint(l-k);dumpthings(eqtb[k],l-k);k:=j+1;
putfmtint(k-l);until k>13506{:1316};putfmtint(parloc);
putfmtint(writeloc);{1318:}putfmtint(hashused);cscount:=10013-hashused;
for p:=514 to hashused do if hash[p].rh<>0 then begin putfmtint(p);
putfmthh(hash[p]);incr(cscount);end;
dumpthings(hash[hashused+1],10280-hashused);putfmtint(cscount);println;
printint(cscount);print(1253){:1318}{:1313};{1320:}putfmtint(fmemptr);
{1322:}begin dumpthings(fontinfo[0],fmemptr);putfmtint(fontptr);
dumpthings(fontcheck[0],fontptr+1);dumpthings(fontsize[0],fontptr+1);
dumpthings(fontdsize[0],fontptr+1);dumpthings(fontparams[0],fontptr+1);
dumpthings(hyphenchar[0],fontptr+1);dumpthings(skewchar[0],fontptr+1);
dumpthings(fontname[0],fontptr+1);dumpthings(fontarea[0],fontptr+1);
dumpthings(fontbc[0],fontptr+1);dumpthings(fontec[0],fontptr+1);
dumpthings(charbase[0],fontptr+1);dumpthings(widthbase[0],fontptr+1);
dumpthings(heightbase[0],fontptr+1);dumpthings(depthbase[0],fontptr+1);
dumpthings(italicbase[0],fontptr+1);
dumpthings(ligkernbase[0],fontptr+1);dumpthings(kernbase[0],fontptr+1);
dumpthings(extenbase[0],fontptr+1);dumpthings(parambase[0],fontptr+1);
dumpthings(fontglue[0],fontptr+1);dumpthings(bcharlabel[0],fontptr+1);
dumpthings(fontbchar[0],fontptr+1);
dumpthings(fontfalsebchar[0],fontptr+1);
for k:=0 to fontptr do begin printnl(1256);printesc(hash[10024+k].rh);
printchar(61);printfilename(fontname[k],fontarea[k],335);
if fontsize[k]<>fontdsize[k]then begin print(737);
printscaled(fontsize[k]);print(393);end;end;end{:1322};println;
printint(fmemptr-7);print(1254);printint(fontptr-0);print(1255);
if fontptr<>1 then printchar(115){:1320};{1324:}putfmtint(hyphcount);
for k:=0 to 607 do if hyphword[k]<>0 then begin putfmtint(k);
putfmtint(hyphword[k]);putfmtint(hyphlist[k]);end;println;
printint(hyphcount);print(1257);if hyphcount<>1 then printchar(115);
if trienotready then inittrie;putfmtint(triemax);
dumpthings(trie[0],triemax+1);putfmtint(trieopptr);
dumpthings(hyfdistance[1],trieopptr);dumpthings(hyfnum[1],trieopptr);
dumpthings(hyfnext[1],trieopptr);printnl(1258);printint(triemax);
print(1259);printint(trieopptr);print(1260);
if trieopptr<>1 then printchar(115);print(1261);printint(trieopsize);
for k:=255 downto 0 do if trieused[k]>0 then begin printnl(793);
printint(trieused[k]);print(1262);printint(k);putfmtint(k);
putfmtint(trieused[k]);end{:1324};{1326:}putfmtint(interaction);
putfmtint(formatident);putfmtint(69069);eqtb[12694].int:=0{:1326};
{1329:}wclose(fmtfile){:1329};end;endif('INITEX'){:1302}{1348:}{1349:}
procedure newwhatsit(s:smallnumber;w:smallnumber);var p:halfword;
begin p:=getnode(w);mem[p].hh.b0:=8;mem[p].hh.b1:=s;
mem[curlist.tailfield].hh.rh:=p;curlist.tailfield:=p;end;{:1349}{1350:}
procedure newwritewhatsit(w:smallnumber);begin newwhatsit(curchr,w);
if w<>2 then scanfourbitint else begin scanint;
if curval<0 then curval:=17 else if curval>15 then curval:=16;end;
mem[curlist.tailfield+1].hh.lh:=curval;end;{:1350}procedure doextension;
var i,j,k:integer;p,q,r:halfword;begin case curchr of 0:{1351:}
begin newwritewhatsit(3);scanoptionalequals;scanfilename;
mem[curlist.tailfield+1].hh.rh:=curname;
mem[curlist.tailfield+2].hh.lh:=curarea;
mem[curlist.tailfield+2].hh.rh:=curext;end{:1351};1:{1352:}
begin k:=curcs;newwritewhatsit(2);curcs:=k;p:=scantoks(false,false);
mem[curlist.tailfield+1].hh.rh:=defref;end{:1352};2:{1353:}
begin newwritewhatsit(2);mem[curlist.tailfield+1].hh.rh:=0;end{:1353};
3:{1354:}begin newwhatsit(3,2);mem[curlist.tailfield+1].hh.lh:=0;
p:=scantoks(false,true);mem[curlist.tailfield+1].hh.rh:=defref;
end{:1354};4:{1375:}begin getxtoken;
if(curcmd=59)and(curchr<=2)then begin p:=curlist.tailfield;doextension;
outwhat(curlist.tailfield);flushnodelist(curlist.tailfield);
curlist.tailfield:=p;mem[p].hh.rh:=0;end else backinput;end{:1375};
5:{1377:}
if abs(curlist.modefield)<>102 then reportillegalcase else begin
newwhatsit(4,2);scanint;
if curval<=0 then curlist.auxfield.hh.rh:=0 else if curval>255 then
curlist.auxfield.hh.rh:=0 else curlist.auxfield.hh.rh:=curval;
mem[curlist.tailfield+1].hh.rh:=curlist.auxfield.hh.rh;
mem[curlist.tailfield+1].hh.b0:=normmin(eqtb[12714].int);
mem[curlist.tailfield+1].hh.b1:=normmin(eqtb[12715].int);end{:1377};
others:confusion(1282)end;end;{:1348}{1376:}procedure fixlanguage;
var l:ASCIIcode;
begin if eqtb[12713].int<=0 then l:=0 else if eqtb[12713].int>255 then l
:=0 else l:=eqtb[12713].int;
if l<>curlist.auxfield.hh.rh then begin newwhatsit(4,2);
mem[curlist.tailfield+1].hh.rh:=l;curlist.auxfield.hh.rh:=l;
mem[curlist.tailfield+1].hh.b0:=normmin(eqtb[12714].int);
mem[curlist.tailfield+1].hh.b1:=normmin(eqtb[12715].int);end;end;{:1376}
{1068:}procedure handlerightbrace;var p,q:halfword;d:scaled;f:integer;
begin case curgroup of 1:unsave;
0:begin begin if interaction=3 then wakeupterminal;printnl(262);
print(1037);end;begin helpptr:=2;helpline[1]:=1038;helpline[0]:=1039;
end;error;end;14,15,16:extrarightbrace;{1085:}2:package(0);
3:begin adjusttail:=memtop-5;package(0);end;4:begin endgraf;package(0);
end;5:begin endgraf;package(4);end;{:1085}{1100:}11:begin endgraf;
q:=eqtb[10292].hh.rh;incr(mem[q].hh.rh);d:=eqtb[13236].int;
f:=eqtb[12705].int;unsave;decr(saveptr);
p:=vpackage(mem[curlist.headfield].hh.rh,0,1,1073741823);popnest;
if savestack[saveptr+0].int<255 then begin begin mem[curlist.tailfield].
hh.rh:=getnode(5);curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
mem[curlist.tailfield].hh.b0:=3;
mem[curlist.tailfield].hh.b1:=savestack[saveptr+0].int;
mem[curlist.tailfield+3].int:=mem[p+3].int+mem[p+2].int;
mem[curlist.tailfield+4].hh.lh:=mem[p+5].hh.rh;
mem[curlist.tailfield+4].hh.rh:=q;mem[curlist.tailfield+2].int:=d;
mem[curlist.tailfield+1].int:=f;
end else begin begin mem[curlist.tailfield].hh.rh:=getnode(2);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
mem[curlist.tailfield].hh.b0:=5;mem[curlist.tailfield].hh.b1:=0;
mem[curlist.tailfield+1].int:=mem[p+5].hh.rh;deleteglueref(q);end;
freenode(p,7);if nestptr=0 then buildpage;end;8:{1026:}
begin if(curinput.locfield<>0)or((curinput.indexfield<>6)and(curinput.
indexfield<>3))then{1027:}
begin begin if interaction=3 then wakeupterminal;printnl(262);
print(1003);end;begin helpptr:=2;helpline[1]:=1004;helpline[0]:=1005;
end;error;repeat gettoken;until curinput.locfield=0;end{:1027};
endtokenlist;endgraf;unsave;outputactive:=false;insertpenalties:=0;
{1028:}if eqtb[11333].hh.rh<>0 then begin begin if interaction=3 then
wakeupterminal;printnl(262);print(1006);end;printesc(405);printint(255);
begin helpptr:=3;helpline[2]:=1007;helpline[1]:=1008;helpline[0]:=1009;
end;boxerror(255);end{:1028};
if curlist.tailfield<>curlist.headfield then begin mem[pagetail].hh.rh:=
mem[curlist.headfield].hh.rh;pagetail:=curlist.tailfield;end;
if mem[memtop-2].hh.rh<>0 then begin if mem[memtop-1].hh.rh=0 then nest[
0].tailfield:=pagetail;mem[pagetail].hh.rh:=mem[memtop-1].hh.rh;
mem[memtop-1].hh.rh:=mem[memtop-2].hh.rh;mem[memtop-2].hh.rh:=0;
pagetail:=memtop-2;end;popnest;buildpage;end{:1026};{:1100}{1118:}
10:builddiscretionary;{:1118}{1132:}6:begin backinput;curtok:=14110;
begin if interaction=3 then wakeupterminal;printnl(262);print(621);end;
printesc(892);print(622);begin helpptr:=1;helpline[0]:=1118;end;
inserror;end;{:1132}{1133:}7:begin endgraf;unsave;alignpeek;end;{:1133}
{1168:}12:begin endgraf;unsave;saveptr:=saveptr-2;
p:=vpackage(mem[curlist.headfield].hh.rh,savestack[saveptr+1].int,
savestack[saveptr+0].int,1073741823);popnest;
begin mem[curlist.tailfield].hh.rh:=newnoad;
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
mem[curlist.tailfield].hh.b0:=29;mem[curlist.tailfield+1].hh.rh:=2;
mem[curlist.tailfield+1].hh.lh:=p;end;{:1168}{1173:}13:buildchoices;
{:1173}{1186:}9:begin unsave;decr(saveptr);
mem[savestack[saveptr+0].int].hh.rh:=3;p:=finmlist(0);
mem[savestack[saveptr+0].int].hh.lh:=p;
if p<>0 then if mem[p].hh.rh=0 then if mem[p].hh.b0=16 then begin if mem
[p+3].hh.rh=0 then if mem[p+2].hh.rh=0 then begin mem[savestack[saveptr
+0].int].hh:=mem[p+1].hh;freenode(p,4);end;
end else if mem[p].hh.b0=28 then if savestack[saveptr+0].int=curlist.
tailfield+1 then if mem[curlist.tailfield].hh.b0=16 then{1187:}
begin q:=curlist.headfield;
while mem[q].hh.rh<>curlist.tailfield do q:=mem[q].hh.rh;
mem[q].hh.rh:=p;freenode(curlist.tailfield,4);curlist.tailfield:=p;
end{:1187};end;{:1186}others:confusion(1040)end;end;{:1068}
procedure maincontrol;
label 60,21,70,80,90,91,92,95,100,101,110,111,112,120,10;var t:integer;
begin if eqtb[10819].hh.rh<>0 then begintokenlist(eqtb[10819].hh.rh,12);
60:getxtoken;21:{1031:}
if interrupt<>0 then if OKtointerrupt then begin backinput;
begin if interrupt<>0 then pauseforinstructions;end;goto 60;end;
ifdef('DEBUG')if panicking then checkmem(false);
endif('DEBUG')if eqtb[12699].int>0 then showcurcmdchr{:1031};
case abs(curlist.modefield)+curcmd of 113,114,170:goto 70;
118:begin scancharnum;curchr:=curval;goto 70;end;167:begin getxtoken;
if(curcmd=11)or(curcmd=12)or(curcmd=68)or(curcmd=16)then cancelboundary
:=true;goto 21;end;
112:if curlist.auxfield.hh.lh=1000 then goto 120 else appspace;
166,267:goto 120;{1045:}1,102,203,11,213,268:;40,141,242:begin{406:}
repeat getxtoken;until curcmd<>10{:406};goto 21;end;
15:if itsallover then goto 10;{1048:}23,123,224,71,172,273,{:1048}
{1098:}39,{:1098}{1111:}45,{:1111}{1144:}49,150,{:1144}
7,108,209:reportillegalcase;{1046:}
8,109,9,110,18,119,70,171,51,152,16,117,50,151,53,154,67,168,54,155,55,
156,57,158,56,157,31,132,52,153,29,130,47,148,212,216,217,230,227,236,
239{:1046}:insertdollarsign;{1056:}
37,137,238:begin begin mem[curlist.tailfield].hh.rh:=scanrulespec;
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
if abs(curlist.modefield)=1 then curlist.auxfield.int:=-65536000 else if
abs(curlist.modefield)=102 then curlist.auxfield.hh.lh:=1000;end;{:1056}
{1057:}28,128,229,231:appendglue;30,131,232,233:appendkern;{:1057}
{1063:}2,103:newsavelevel(1);62,163,264:newsavelevel(14);
63,164,265:if curgroup=14 then unsave else offsave;{:1063}{1067:}
3,104,205:handlerightbrace;{:1067}{1073:}22,124,225:begin t:=curchr;
scandimen(false,false,false);
if t=0 then scanbox(curval)else scanbox(-curval);end;
32,133,234:scanbox(1073742237+curchr);21,122,223:beginbox(0);{:1073}
{1090:}44:newgraf(curchr>0);
12,13,17,69,4,24,36,46,48,27,34,65,66:begin backinput;newgraf(true);end;
{:1090}{1092:}145,246:indentinhmode;{:1092}{1094:}
14:begin normalparagraph;if curlist.modefield>0 then buildpage;end;
115:begin if alignstate<0 then offsave;endgraf;
if curlist.modefield=1 then buildpage;end;
116,129,138,126,134:headforvmode;{:1094}{1097:}
38,139,240,140,241:begininsertoradjust;19,120,221:makemark;{:1097}
{1102:}43,144,245:appendpenalty;{:1102}{1104:}26,127,228:deletelast;
{:1104}{1109:}25,125,226:unpackage;{:1109}{1112:}
146:appenditaliccorrection;
247:begin mem[curlist.tailfield].hh.rh:=newkern(0);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;{:1112}{1116:}
149,250:appenddiscretionary;{:1116}{1122:}147:makeaccent;{:1122}{1126:}
6,107,208,5,106,207:alignerror;35,136,237:noalignerror;
64,165,266:omiterror;{:1126}{1130:}33,135:initalign;
235:if privileged then if curgroup=15 then initalign else offsave;
10,111:doendv;{:1130}{1134:}68,169,270:cserror;{:1134}{1137:}
105:initmath;{:1137}{1140:}
251:if privileged then if curgroup=15 then starteqno else offsave;
{:1140}{1150:}204:begin begin mem[curlist.tailfield].hh.rh:=newnoad;
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;backinput;
scanmath(curlist.tailfield+1);end;{:1150}{1154:}
214,215,271:setmathchar(eqtb[12407+curchr].hh.rh);219:begin scancharnum;
curchr:=curval;setmathchar(eqtb[12407+curchr].hh.rh);end;
220:begin scanfifteenbitint;setmathchar(curval);end;
272:setmathchar(curchr);218:begin scantwentysevenbitint;
setmathchar(curval div 4096);end;{:1154}{1158:}
253:begin begin mem[curlist.tailfield].hh.rh:=newnoad;
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
mem[curlist.tailfield].hh.b0:=curchr;scanmath(curlist.tailfield+1);end;
254:mathlimitswitch;{:1158}{1162:}269:mathradical;{:1162}{1164:}
248,249:mathac;{:1164}{1167:}259:begin scanspec(12,false);
normalparagraph;pushnest;curlist.modefield:=-1;
curlist.auxfield.int:=-65536000;
if eqtb[10818].hh.rh<>0 then begintokenlist(eqtb[10818].hh.rh,11);end;
{:1167}{1171:}256:begin mem[curlist.tailfield].hh.rh:=newstyle(curchr);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
258:begin begin mem[curlist.tailfield].hh.rh:=newglue(0);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;
mem[curlist.tailfield].hh.b1:=98;end;257:appendchoices;{:1171}{1175:}
211,210:subsup;{:1175}{1180:}255:mathfraction;{:1180}{1190:}
252:mathleftright;{:1190}{1193:}
206:if curgroup=15 then aftermath else offsave;{:1193}{1210:}
72,173,274,73,174,275,74,175,276,75,176,277,76,177,278,77,178,279,78,179
,280,79,180,281,80,181,282,81,182,283,82,183,284,83,184,285,84,185,286,
85,186,287,86,187,288,87,188,289,88,189,290,89,190,291,90,191,292,91,192
,293,92,193,294,93,194,295,94,195,296,95,196,297,96,197,298,97,198,299,
98,199,300,99,200,301,100,201,302,101,202,303:prefixedcommand;{:1210}
{1268:}41,142,243:begin gettoken;aftertoken:=curtok;end;{:1268}{1271:}
42,143,244:begin gettoken;saveforafter(curtok);end;{:1271}{1274:}
61,162,263:openorclosein;{:1274}{1276:}59,160,261:issuemessage;{:1276}
{1285:}58,159,260:shiftcase;{:1285}{1290:}20,121,222:showwhatever;
{:1290}{1347:}60,161,262:doextension;{:1347}{:1045}end;goto 60;
70:{1034:}mains:=eqtb[12151+curchr].hh.rh;
if mains=1000 then curlist.auxfield.hh.lh:=1000 else if mains<1000 then
begin if mains>0 then curlist.auxfield.hh.lh:=mains;
end else if curlist.auxfield.hh.lh<1000 then curlist.auxfield.hh.lh:=
1000 else curlist.auxfield.hh.lh:=mains;mainf:=eqtb[11334].hh.rh;
bchar:=fontbchar[mainf];falsebchar:=fontfalsebchar[mainf];
if curlist.modefield>0 then if eqtb[12713].int<>curlist.auxfield.hh.rh
then fixlanguage;begin ligstack:=avail;
if ligstack=0 then ligstack:=getavail else begin avail:=mem[ligstack].hh
.rh;mem[ligstack].hh.rh:=0;ifdef('STAT')incr(dynused);endif('STAT')end;
end;mem[ligstack].hh.b0:=mainf;curl:=curchr;mem[ligstack].hh.b1:=curl;
curq:=curlist.tailfield;
if cancelboundary then begin cancelboundary:=false;maink:=fontmemsize;
end else maink:=bcharlabel[mainf];if maink=fontmemsize then goto 92;
curr:=curl;curl:=256;goto 111;80:{1035:}
if curl<256 then begin if mem[curlist.tailfield].hh.b1=hyphenchar[mainf]
then if mem[curq].hh.rh>0 then insdisc:=true;
if ligaturepresent then begin mainp:=newligature(mainf,curl,mem[curq].hh
.rh);if lfthit then begin mem[mainp].hh.b1:=2;lfthit:=false;end;
if rthit then if ligstack=0 then begin incr(mem[mainp].hh.b1);
rthit:=false;end;mem[curq].hh.rh:=mainp;curlist.tailfield:=mainp;
ligaturepresent:=false;end;if insdisc then begin insdisc:=false;
if curlist.modefield>0 then begin mem[curlist.tailfield].hh.rh:=newdisc;
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;end;end{:1035};
90:{1036:}if ligstack=0 then goto 21;curq:=curlist.tailfield;curl:=curr;
91:if not(ligstack>=himemmin)then goto 95;
92:if(curchr<fontbc[mainf])or(curchr>fontec[mainf])then begin
charwarning(mainf,curchr);begin mem[ligstack].hh.rh:=avail;
avail:=ligstack;ifdef('STAT')decr(dynused);endif('STAT')end;goto 60;end;
maini:=fontinfo[charbase[mainf]+curl].qqqq;
if not(maini.b0>0)then begin charwarning(mainf,curchr);
begin mem[ligstack].hh.rh:=avail;avail:=ligstack;
ifdef('STAT')decr(dynused);endif('STAT')end;goto 60;end;
begin mem[curlist.tailfield].hh.rh:=ligstack;
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end{:1036};100:{1038:}
getnext;if curcmd=11 then goto 101;if curcmd=12 then goto 101;
if curcmd=68 then goto 101;xtoken;if curcmd=11 then goto 101;
if curcmd=12 then goto 101;if curcmd=68 then goto 101;
if curcmd=16 then begin scancharnum;curchr:=curval;goto 101;end;
if curcmd=65 then bchar:=256;curr:=bchar;ligstack:=0;goto 110;
101:mains:=eqtb[12151+curchr].hh.rh;
if mains=1000 then curlist.auxfield.hh.lh:=1000 else if mains<1000 then
begin if mains>0 then curlist.auxfield.hh.lh:=mains;
end else if curlist.auxfield.hh.lh<1000 then curlist.auxfield.hh.lh:=
1000 else curlist.auxfield.hh.lh:=mains;begin ligstack:=avail;
if ligstack=0 then ligstack:=getavail else begin avail:=mem[ligstack].hh
.rh;mem[ligstack].hh.rh:=0;ifdef('STAT')incr(dynused);endif('STAT')end;
end;mem[ligstack].hh.b0:=mainf;curr:=curchr;mem[ligstack].hh.b1:=curr;
if curr=falsebchar then curr:=256{:1038};110:{1039:}
if((maini.b2)mod 4)<>1 then goto 80;maink:=ligkernbase[mainf]+maini.b3;
mainj:=fontinfo[maink].qqqq;if mainj.b0<=128 then goto 112;
maink:=ligkernbase[mainf]+256*mainj.b2+mainj.b3+32768-256*(128);
111:mainj:=fontinfo[maink].qqqq;
112:if mainj.b1=curr then if mainj.b0<=128 then{1040:}
begin if mainj.b2>=128 then begin if curl<256 then begin if mem[curlist.
tailfield].hh.b1=hyphenchar[mainf]then if mem[curq].hh.rh>0 then insdisc
:=true;
if ligaturepresent then begin mainp:=newligature(mainf,curl,mem[curq].hh
.rh);if lfthit then begin mem[mainp].hh.b1:=2;lfthit:=false;end;
if rthit then if ligstack=0 then begin incr(mem[mainp].hh.b1);
rthit:=false;end;mem[curq].hh.rh:=mainp;curlist.tailfield:=mainp;
ligaturepresent:=false;end;if insdisc then begin insdisc:=false;
if curlist.modefield>0 then begin mem[curlist.tailfield].hh.rh:=newdisc;
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;end;end;
begin mem[curlist.tailfield].hh.rh:=newkern(fontinfo[kernbase[mainf]+256
*mainj.b2+mainj.b3].int);
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;goto 90;end;
if curl=256 then lfthit:=true else if ligstack=0 then rthit:=true;
begin if interrupt<>0 then pauseforinstructions;end;
case mainj.b2 of 1,5:begin curl:=mainj.b3;
maini:=fontinfo[charbase[mainf]+curl].qqqq;ligaturepresent:=true;end;
2,6:begin curr:=mainj.b3;
if ligstack=0 then begin ligstack:=newligitem(curr);bchar:=256;
end else if(ligstack>=himemmin)then begin mainp:=ligstack;
ligstack:=newligitem(curr);mem[ligstack+1].hh.rh:=mainp;
end else mem[ligstack].hh.b1:=curr;end;3:begin curr:=mainj.b3;
mainp:=ligstack;ligstack:=newligitem(curr);mem[ligstack].hh.rh:=mainp;
end;7,11:begin if curl<256 then begin if mem[curlist.tailfield].hh.b1=
hyphenchar[mainf]then if mem[curq].hh.rh>0 then insdisc:=true;
if ligaturepresent then begin mainp:=newligature(mainf,curl,mem[curq].hh
.rh);if lfthit then begin mem[mainp].hh.b1:=2;lfthit:=false;end;
if false then if ligstack=0 then begin incr(mem[mainp].hh.b1);
rthit:=false;end;mem[curq].hh.rh:=mainp;curlist.tailfield:=mainp;
ligaturepresent:=false;end;if insdisc then begin insdisc:=false;
if curlist.modefield>0 then begin mem[curlist.tailfield].hh.rh:=newdisc;
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;end;end;
curq:=curlist.tailfield;curl:=mainj.b3;
maini:=fontinfo[charbase[mainf]+curl].qqqq;ligaturepresent:=true;end;
others:begin curl:=mainj.b3;ligaturepresent:=true;
if ligstack=0 then goto 80 else goto 91;end end;
if mainj.b2>4 then if mainj.b2<>7 then goto 80;
if curl<256 then goto 110;maink:=bcharlabel[mainf];goto 111;end{:1040};
if mainj.b0=0 then incr(maink)else begin if mainj.b0>=128 then goto 80;
maink:=maink+mainj.b0+1;end;goto 111{:1039};95:{1037:}
mainp:=mem[ligstack+1].hh.rh;
if mainp>0 then begin mem[curlist.tailfield].hh.rh:=mainp;
curlist.tailfield:=mem[curlist.tailfield].hh.rh;end;tempptr:=ligstack;
ligstack:=mem[tempptr].hh.rh;freenode(tempptr,2);
maini:=fontinfo[charbase[mainf]+curl].qqqq;ligaturepresent:=true;
if ligstack=0 then if mainp>0 then goto 100 else curr:=bchar else curr:=
mem[ligstack].hh.b1;goto 110{:1037}{:1034};120:{1041:}
if eqtb[10294].hh.rh=0 then begin{1042:}
begin mainp:=fontglue[eqtb[11334].hh.rh];
if mainp=0 then begin mainp:=newspec(0);
maink:=parambase[eqtb[11334].hh.rh]+2;
mem[mainp+1].int:=fontinfo[maink].int;
mem[mainp+2].int:=fontinfo[maink+1].int;
mem[mainp+3].int:=fontinfo[maink+2].int;
fontglue[eqtb[11334].hh.rh]:=mainp;end;end{:1042};
tempptr:=newglue(mainp);end else tempptr:=newparamglue(12);
mem[curlist.tailfield].hh.rh:=tempptr;curlist.tailfield:=tempptr;
goto 60{:1041};10:end;{:1030}{1284:}procedure giveerrhelp;
begin tokenshow(eqtb[10821].hh.rh);end;{:1284}{1303:}{524:}
function openfmtfile:boolean;label 40,10;var j:0..bufsize;
begin j:=curinput.locfield;
if buffer[curinput.locfield]=38 then begin incr(curinput.locfield);
j:=curinput.locfield;buffer[last]:=32;while buffer[j]<>32 do incr(j);
packbufferedname(0,curinput.locfield,j-1);
if wopenin(fmtfile)then goto 40;wakeupterminal;
writeln(stdout,'Sorry, I can''t find that format;',
' will try the default.');termflush(stdout);end;packbufferedname(5,1,0);
if not wopenin(fmtfile)then begin wakeupterminal;
writeln(stdout,'I can''t find the default format file!');
openfmtfile:=false;goto 10;end;40:curinput.locfield:=j;
openfmtfile:=true;10:end;{:524}function loadfmtfile:boolean;
label 6666,10;var j,k:integer;p,q:halfword;x:integer;begin{1308:}
getfmtint(x);if x<>127541235 then goto 6666;getfmtint(x);
if x<>0 then goto 6666;getfmtint(x);if x<>memtop then goto 6666;
getfmtint(x);if x<>13506 then goto 6666;getfmtint(x);
if x<>7919 then goto 6666;getfmtint(x);if x<>607 then goto 6666{:1308};
{1310:}begin getfmtint(x);if x<0 then goto 6666;
if x>poolsize then begin wakeupterminal;
writeln(stdout,'---! Must increase the ','string pool size');goto 6666;
end else poolptr:=x;end;begin getfmtint(x);if x<0 then goto 6666;
if x>maxstrings then begin wakeupterminal;
writeln(stdout,'---! Must increase the ','max strings');goto 6666;
end else strptr:=x;end;undumpthings(strstart[0],strptr+1);
undumpthings(strpool[0],poolptr);initstrptr:=strptr;
initpoolptr:=poolptr{:1310};{1312:}begin getfmtint(x);
if(x<1019)or(x>memtop-14)then goto 6666 else lomemmax:=x;end;
begin getfmtint(x);if(x<20)or(x>lomemmax)then goto 6666 else rover:=x;
end;p:=0;q:=rover;repeat undumpthings(mem[p],q+2-p);p:=q+mem[q].hh.lh;
if(p>lomemmax)or((q>=mem[q+1].hh.rh)and(mem[q+1].hh.rh<>rover))then goto
6666;q:=mem[q+1].hh.rh;until q=rover;undumpthings(mem[p],lomemmax+1-p);
if memmin<-2 then begin p:=mem[rover+1].hh.lh;q:=memmin+1;
mem[memmin].hh.rh:=0;mem[memmin].hh.lh:=0;mem[p+1].hh.rh:=q;
mem[rover+1].hh.lh:=q;mem[q+1].hh.rh:=rover;mem[q+1].hh.lh:=p;
mem[q].hh.rh:=262143;mem[q].hh.lh:=-0-q;end;begin getfmtint(x);
if(x<lomemmax+1)or(x>memtop-13)then goto 6666 else himemmin:=x;end;
begin getfmtint(x);if(x<0)or(x>memtop)then goto 6666 else avail:=x;end;
memend:=memtop;undumpthings(mem[himemmin],memend+1-himemmin);
getfmtint(varused);getfmtint(dynused){:1312};{1314:}{1317:}k:=1;
repeat getfmtint(x);if(x<1)or(k+x>13507)then goto 6666;
undumpthings(eqtb[k],x);k:=k+x;getfmtint(x);
if(x<0)or(k+x>13507)then goto 6666;
for j:=k to k+x-1 do eqtb[j]:=eqtb[k-1];k:=k+x;until k>13506{:1317};
begin getfmtint(x);if(x<514)or(x>10014)then goto 6666 else parloc:=x;
end;partoken:=4095+parloc;begin getfmtint(x);
if(x<514)or(x>10014)then goto 6666 else writeloc:=x;end;{1319:}
begin getfmtint(x);if(x<514)or(x>10014)then goto 6666 else hashused:=x;
end;p:=513;repeat begin getfmtint(x);
if(x<p+1)or(x>hashused)then goto 6666 else p:=x;end;getfmthh(hash[p]);
until p=hashused;undumpthings(hash[hashused+1],10280-hashused);
getfmtint(cscount){:1319}{:1314};{1321:}begin getfmtint(x);
if x<7 then goto 6666;if x>fontmemsize then begin wakeupterminal;
writeln(stdout,'---! Must increase the ','font mem size');goto 6666;
end else fmemptr:=x;end;{1323:}begin undumpthings(fontinfo[0],fmemptr);
begin getfmtint(x);if x<0 then goto 6666;
if x>fontmax then begin wakeupterminal;
writeln(stdout,'---! Must increase the ','font max');goto 6666;
end else fontptr:=x;end;undumpthings(fontcheck[0],fontptr+1);
undumpthings(fontsize[0],fontptr+1);
undumpthings(fontdsize[0],fontptr+1);
undumpthings(fontparams[0],fontptr+1);
undumpthings(hyphenchar[0],fontptr+1);
undumpthings(skewchar[0],fontptr+1);undumpthings(fontname[0],fontptr+1);
undumpthings(fontarea[0],fontptr+1);undumpthings(fontbc[0],fontptr+1);
undumpthings(fontec[0],fontptr+1);undumpthings(charbase[0],fontptr+1);
undumpthings(widthbase[0],fontptr+1);
undumpthings(heightbase[0],fontptr+1);
undumpthings(depthbase[0],fontptr+1);
undumpthings(italicbase[0],fontptr+1);
undumpthings(ligkernbase[0],fontptr+1);
undumpthings(kernbase[0],fontptr+1);
undumpthings(extenbase[0],fontptr+1);
undumpthings(parambase[0],fontptr+1);
undumpthings(fontglue[0],fontptr+1);
undumpthings(bcharlabel[0],fontptr+1);
undumpthings(fontbchar[0],fontptr+1);
undumpthings(fontfalsebchar[0],fontptr+1);end{:1323};{:1321};{1325:}
begin getfmtint(x);if(x<0)or(x>607)then goto 6666 else hyphcount:=x;end;
for k:=1 to hyphcount do begin begin getfmtint(x);
if(x<0)or(x>607)then goto 6666 else j:=x;end;begin getfmtint(x);
if(x<0)or(x>strptr)then goto 6666 else hyphword[j]:=x;end;
begin getfmtint(x);
if(x<0)or(x>262143)then goto 6666 else hyphlist[j]:=x;end;end;
begin getfmtint(x);if x<0 then goto 6666;
if x>triesize then begin wakeupterminal;
writeln(stdout,'---! Must increase the ','trie size');goto 6666;
end else j:=x;end;ifdef('INITEX')triemax:=j;
endif('INITEX')undumpthings(trie[0],j+1);begin getfmtint(x);
if x<0 then goto 6666;if x>trieopsize then begin wakeupterminal;
writeln(stdout,'---! Must increase the ','trie op size');goto 6666;
end else j:=x;end;ifdef('INITEX')trieopptr:=j;
endif('INITEX')undumpthings(hyfdistance[1],j);undumpthings(hyfnum[1],j);
undumpthings(hyfnext[1],j);
ifdef('INITEX')for k:=0 to 255 do trieused[k]:=0;endif('INITEX')k:=256;
while j>0 do begin begin getfmtint(x);
if(x<0)or(x>k-1)then goto 6666 else k:=x;end;begin getfmtint(x);
if(x<1)or(x>j)then goto 6666 else x:=x;end;
ifdef('INITEX')trieused[k]:=x;endif('INITEX')j:=j-x;opstart[k]:=j;end;
ifdef('INITEX')trienotready:=false endif('INITEX'){:1325};{1327:}
begin getfmtint(x);if(x<0)or(x>3)then goto 6666 else interaction:=x;end;
begin getfmtint(x);
if(x<0)or(x>strptr)then goto 6666 else formatident:=x;end;getfmtint(x);
if(x<>69069)or eof(fmtfile)then goto 6666{:1327};loadfmtfile:=true;
goto 10;6666:wakeupterminal;
writeln(stdout,'(Fatal format file error; I''m stymied)');
loadfmtfile:=false;10:end;{:1303}{1330:}{1333:}
procedure closefilesandterminate;var k:integer;begin{1378:}
for k:=0 to 15 do if writeopen[k]then aclose(writefile[k]){:1378};
ifdef('STAT')if eqtb[12694].int>0 then{1334:}
if logopened then begin writeln(logfile,' ');
writeln(logfile,'Here is how much of TeX''s memory',' you used:');
write(logfile,' ',strptr-initstrptr:1,' string');
if strptr<>initstrptr+1 then write(logfile,'s');
writeln(logfile,' out of ',maxstrings-initstrptr:1);
writeln(logfile,' ',poolptr-initpoolptr:1,' string characters out of ',
poolsize-initpoolptr:1);
writeln(logfile,' ',lomemmax-memmin+memend-himemmin+2:1,
' words of memory out of ',memend+1-memmin:1);
writeln(logfile,' ',cscount:1,' multiletter control sequences out of ',
9500:1);
write(logfile,' ',fmemptr:1,' words of font info for ',fontptr-0:1,
' font');if fontptr<>1 then write(logfile,'s');
writeln(logfile,', out of ',fontmemsize:1,' for ',fontmax-0:1);
write(logfile,' ',hyphcount:1,' hyphenation exception');
if hyphcount<>1 then write(logfile,'s');
writeln(logfile,' out of ',607:1);
writeln(logfile,' ',maxinstack:1,'i,',maxneststack:1,'n,',maxparamstack:
1,'p,',maxbufstack+1:1,'b,',maxsavestack+6:1,'s stack positions out of '
,stacksize:1,'i,',nestsize:1,'n,',paramsize:1,'p,',bufsize:1,'b,',
savesize:1,'s');end{:1334};endif('STAT'){642:}
while curs>-1 do begin if curs>0 then begin dvibuf[dviptr]:=142;
incr(dviptr);if dviptr=dvilimit then dviswap;
end else begin begin dvibuf[dviptr]:=140;incr(dviptr);
if dviptr=dvilimit then dviswap;end;incr(totalpages);end;decr(curs);end;
if totalpages=0 then printnl(830)else begin begin dvibuf[dviptr]:=248;
incr(dviptr);if dviptr=dvilimit then dviswap;end;dvifour(lastbop);
lastbop:=dvioffset+dviptr-5;dvifour(25400000);dvifour(473628672);
preparemag;dvifour(eqtb[12680].int);dvifour(maxv);dvifour(maxh);
begin dvibuf[dviptr]:=maxpush div 256;incr(dviptr);
if dviptr=dvilimit then dviswap;end;
begin dvibuf[dviptr]:=maxpush mod 256;incr(dviptr);
if dviptr=dvilimit then dviswap;end;
begin dvibuf[dviptr]:=totalpages div 256;incr(dviptr);
if dviptr=dvilimit then dviswap;end;
begin dvibuf[dviptr]:=totalpages mod 256;incr(dviptr);
if dviptr=dvilimit then dviswap;end;{643:}
while fontptr>0 do begin if fontused[fontptr]then dvifontdef(fontptr);
decr(fontptr);end{:643};begin dvibuf[dviptr]:=249;incr(dviptr);
if dviptr=dvilimit then dviswap;end;dvifour(lastbop);
begin dvibuf[dviptr]:=2;incr(dviptr);if dviptr=dvilimit then dviswap;
end;k:=4+((dvibufsize-dviptr)mod 4);
while k>0 do begin begin dvibuf[dviptr]:=223;incr(dviptr);
if dviptr=dvilimit then dviswap;end;decr(k);end;{599:}
if dvilimit=halfbuf then writedvi(halfbuf,dvibufsize-1);
if dviptr>0 then writedvi(0,dviptr-1){:599};printnl(831);
print(outputfilename);print(284);printint(totalpages);print(832);
if totalpages<>1 then printchar(115);print(833);
printint(dvioffset+dviptr);print(834);bclose(dvifile);end{:642};
if logopened then begin writeln(logfile);aclose(logfile);
selector:=selector-2;if selector=17 then begin printnl(1266);
print(logname);printchar(46);end;end;println;
if(editnamestart<>0)and(interaction>0)then calledit(strpool,
editnamestart,editnamelength,editline);end;{:1333}{1335:}
procedure finalcleanup;label 10;var c:smallnumber;begin c:=curchr;
if jobname=0 then openlogfile;while openparens>0 do begin print(1267);
decr(openparens);end;if curlevel>1 then begin printnl(40);
printesc(1268);print(1269);printint(curlevel-1);printchar(41);end;
while condptr<>0 do begin printnl(40);printesc(1268);print(1270);
printcmdchr(105,curif);if ifline<>0 then begin print(1271);
printint(ifline);end;print(1272);ifline:=mem[condptr+1].int;
curif:=mem[condptr].hh.b1;condptr:=mem[condptr].hh.rh;end;
if history<>0 then if((history=1)or(interaction<3))then if selector=19
then begin selector:=17;printnl(1273);selector:=19;end;
if c=1 then begin ifdef('INITEX')storefmtfile;goto 10;
endif('INITEX')printnl(1274);goto 10;end;10:end;{:1335}{1336:}
ifdef('INITEX')procedure initprim;begin nonewcontrolsequence:=false;
{226:}primitive(372,75,10282);primitive(373,75,10283);
primitive(374,75,10284);primitive(375,75,10285);primitive(376,75,10286);
primitive(377,75,10287);primitive(378,75,10288);primitive(379,75,10289);
primitive(380,75,10290);primitive(381,75,10291);primitive(382,75,10292);
primitive(383,75,10293);primitive(384,75,10294);primitive(385,75,10295);
primitive(386,75,10296);primitive(387,76,10297);primitive(388,76,10298);
primitive(389,76,10299);{:226}{230:}primitive(394,72,10813);
primitive(395,72,10814);primitive(396,72,10815);primitive(397,72,10816);
primitive(398,72,10817);primitive(399,72,10818);primitive(400,72,10819);
primitive(401,72,10820);primitive(402,72,10821);{:230}{238:}
primitive(416,73,12663);primitive(417,73,12664);primitive(418,73,12665);
primitive(419,73,12666);primitive(420,73,12667);primitive(421,73,12668);
primitive(422,73,12669);primitive(423,73,12670);primitive(424,73,12671);
primitive(425,73,12672);primitive(426,73,12673);primitive(427,73,12674);
primitive(428,73,12675);primitive(429,73,12676);primitive(430,73,12677);
primitive(431,73,12678);primitive(432,73,12679);primitive(433,73,12680);
primitive(434,73,12681);primitive(435,73,12682);primitive(436,73,12683);
primitive(437,73,12684);primitive(438,73,12685);primitive(439,73,12686);
primitive(440,73,12687);primitive(441,73,12688);primitive(442,73,12689);
primitive(443,73,12690);primitive(444,73,12691);primitive(445,73,12692);
primitive(446,73,12693);primitive(447,73,12694);primitive(448,73,12695);
primitive(449,73,12696);primitive(450,73,12697);primitive(451,73,12698);
primitive(452,73,12699);primitive(453,73,12700);primitive(454,73,12701);
primitive(455,73,12702);primitive(456,73,12703);primitive(457,73,12704);
primitive(458,73,12705);primitive(459,73,12706);primitive(460,73,12707);
primitive(461,73,12708);primitive(462,73,12709);primitive(463,73,12710);
primitive(464,73,12711);primitive(465,73,12712);primitive(466,73,12713);
primitive(467,73,12714);primitive(468,73,12715);primitive(469,73,12716);
primitive(470,73,12717);{:238}{248:}primitive(474,74,13230);
primitive(475,74,13231);primitive(476,74,13232);primitive(477,74,13233);
primitive(478,74,13234);primitive(479,74,13235);primitive(480,74,13236);
primitive(481,74,13237);primitive(482,74,13238);primitive(483,74,13239);
primitive(484,74,13240);primitive(485,74,13241);primitive(486,74,13242);
primitive(487,74,13243);primitive(488,74,13244);primitive(489,74,13245);
primitive(490,74,13246);primitive(491,74,13247);primitive(492,74,13248);
primitive(493,74,13249);primitive(494,74,13250);{:248}{265:}
primitive(32,64,0);primitive(47,44,0);primitive(504,45,0);
primitive(505,90,0);primitive(506,40,0);primitive(507,41,0);
primitive(508,61,0);primitive(509,16,0);primitive(500,107,0);
primitive(510,15,0);primitive(511,92,0);primitive(501,67,0);
primitive(512,62,0);hash[10016].rh:=512;eqtb[10016]:=eqtb[curval];
primitive(513,102,0);primitive(514,88,0);primitive(515,77,0);
primitive(516,32,0);primitive(517,36,0);primitive(518,39,0);
primitive(327,37,0);primitive(348,18,0);primitive(519,46,0);
primitive(520,17,0);primitive(521,54,0);primitive(522,91,0);
primitive(523,34,0);primitive(524,65,0);primitive(525,103,0);
primitive(332,55,0);primitive(526,63,0);primitive(404,84,0);
primitive(527,42,0);primitive(528,80,0);primitive(529,66,0);
primitive(530,96,0);primitive(531,0,256);hash[10021].rh:=531;
eqtb[10021]:=eqtb[curval];primitive(532,98,0);primitive(533,109,0);
primitive(403,71,0);primitive(349,38,0);primitive(534,33,0);
primitive(535,56,0);primitive(536,35,0);{:265}{334:}
primitive(593,13,256);parloc:=curval;partoken:=4095+parloc;{:334}{376:}
primitive(625,104,0);primitive(626,104,1);{:376}{384:}
primitive(627,110,0);primitive(628,110,1);primitive(629,110,2);
primitive(630,110,3);primitive(631,110,4);{:384}{411:}
primitive(472,89,0);primitive(496,89,1);primitive(391,89,2);
primitive(392,89,3);{:411}{416:}primitive(664,79,102);
primitive(665,79,1);primitive(666,82,0);primitive(667,82,1);
primitive(668,83,1);primitive(669,83,3);primitive(670,83,2);
primitive(671,70,0);primitive(672,70,1);primitive(673,70,2);
primitive(674,70,3);primitive(675,70,4);{:416}{468:}
primitive(731,108,0);primitive(732,108,1);primitive(733,108,2);
primitive(734,108,3);primitive(735,108,4);primitive(736,108,5);{:468}
{487:}primitive(752,105,0);primitive(753,105,1);primitive(754,105,2);
primitive(755,105,3);primitive(756,105,4);primitive(757,105,5);
primitive(758,105,6);primitive(759,105,7);primitive(760,105,8);
primitive(761,105,9);primitive(762,105,10);primitive(763,105,11);
primitive(764,105,12);primitive(765,105,13);primitive(766,105,14);
primitive(767,105,15);primitive(768,105,16);{:487}{491:}
primitive(769,106,2);hash[10018].rh:=769;eqtb[10018]:=eqtb[curval];
primitive(770,106,4);primitive(771,106,3);{:491}{553:}
primitive(794,87,0);hash[10024].rh:=794;eqtb[10024]:=eqtb[curval];{:553}
{780:}primitive(891,4,256);primitive(892,5,257);hash[10015].rh:=892;
eqtb[10015]:=eqtb[curval];primitive(893,5,258);hash[10019].rh:=894;
hash[10020].rh:=894;eqtb[10020].hh.b0:=9;eqtb[10020].hh.rh:=memtop-11;
eqtb[10020].hh.b1:=1;eqtb[10019]:=eqtb[10020];eqtb[10019].hh.b0:=115;
{:780}{983:}primitive(963,81,0);primitive(964,81,1);primitive(965,81,2);
primitive(966,81,3);primitive(967,81,4);primitive(968,81,5);
primitive(969,81,6);primitive(970,81,7);{:983}{1052:}
primitive(1018,14,0);primitive(1019,14,1);{:1052}{1058:}
primitive(1020,26,4);primitive(1021,26,0);primitive(1022,26,1);
primitive(1023,26,2);primitive(1024,26,3);primitive(1025,27,4);
primitive(1026,27,0);primitive(1027,27,1);primitive(1028,27,2);
primitive(1029,27,3);primitive(333,28,5);primitive(337,29,1);
primitive(339,30,99);{:1058}{1071:}primitive(1047,21,1);
primitive(1048,21,0);primitive(1049,22,1);primitive(1050,22,0);
primitive(405,20,0);primitive(1051,20,1);primitive(1052,20,2);
primitive(958,20,3);primitive(1053,20,4);primitive(960,20,5);
primitive(1054,20,106);primitive(1055,31,99);primitive(1056,31,100);
primitive(1057,31,101);primitive(1058,31,102);{:1071}{1088:}
primitive(1073,43,1);primitive(1074,43,0);{:1088}{1107:}
primitive(1083,25,12);primitive(1084,25,11);primitive(1085,25,10);
primitive(1086,23,0);primitive(1087,23,1);primitive(1088,24,0);
primitive(1089,24,1);{:1107}{1114:}primitive(45,47,1);
primitive(346,47,0);{:1114}{1141:}primitive(1120,48,0);
primitive(1121,48,1);{:1141}{1156:}primitive(859,50,16);
primitive(860,50,17);primitive(861,50,18);primitive(862,50,19);
primitive(863,50,20);primitive(864,50,21);primitive(865,50,22);
primitive(866,50,23);primitive(868,50,26);primitive(867,50,27);
primitive(1122,51,0);primitive(871,51,1);primitive(872,51,2);{:1156}
{1169:}primitive(854,53,0);primitive(855,53,2);primitive(856,53,4);
primitive(857,53,6);{:1169}{1178:}primitive(1140,52,0);
primitive(1141,52,1);primitive(1142,52,2);primitive(1143,52,3);
primitive(1144,52,4);primitive(1145,52,5);{:1178}{1188:}
primitive(869,49,30);primitive(870,49,31);hash[10017].rh:=870;
eqtb[10017]:=eqtb[curval];{:1188}{1208:}primitive(1164,93,1);
primitive(1165,93,2);primitive(1166,93,4);primitive(1167,97,0);
primitive(1168,97,1);primitive(1169,97,2);primitive(1170,97,3);{:1208}
{1219:}primitive(1184,94,0);primitive(1185,94,1);{:1219}{1222:}
primitive(1186,95,0);primitive(1187,95,1);primitive(1188,95,2);
primitive(1189,95,3);primitive(1190,95,4);primitive(1191,95,5);
primitive(1192,95,6);{:1222}{1230:}primitive(411,85,11383);
primitive(415,85,12407);primitive(412,85,11639);primitive(413,85,11895);
primitive(414,85,12151);primitive(473,85,12974);primitive(408,86,11335);
primitive(409,86,11351);primitive(410,86,11367);{:1230}{1250:}
primitive(934,99,0);primitive(946,99,1);{:1250}{1254:}
primitive(1208,78,0);primitive(1209,78,1);{:1254}{1262:}
primitive(272,100,0);primitive(273,100,1);primitive(274,100,2);
primitive(1218,100,3);{:1262}{1272:}primitive(1219,60,1);
primitive(1220,60,0);{:1272}{1277:}primitive(1221,58,0);
primitive(1222,58,1);{:1277}{1286:}primitive(1228,57,11639);
primitive(1229,57,11895);{:1286}{1291:}primitive(1230,19,0);
primitive(1231,19,1);primitive(1232,19,2);primitive(1233,19,3);{:1291}
{1344:}primitive(1276,59,0);primitive(590,59,1);writeloc:=curval;
primitive(1277,59,2);primitive(1278,59,3);primitive(1279,59,4);
primitive(1280,59,5);{:1344};nonewcontrolsequence:=true;end;
endif('INITEX'){:1336}{1338:}ifdef('DEBUG')procedure debughelp;
label 888,10;var k,l,m,n:integer;
begin while true do begin wakeupterminal;printnl(1275);
termflush(stdout);read(stdin,m);
if m<0 then goto 10 else if m=0 then dumpcore else begin read(stdin,n);
case m of{1339:}1:printword(mem[n]);2:printint(mem[n].hh.lh);
3:printint(mem[n].hh.rh);4:printword(eqtb[n]);5:printword(fontinfo[n]);
6:printword(savestack[n]);7:showbox(n);8:begin breadthmax:=10000;
depththreshold:=poolsize-poolptr-10;shownodelist(n);end;
9:showtokenlist(n,0,1000);10:print(n);11:checkmem(n>0);12:searchmem(n);
13:begin read(stdin,l);printcmdchr(n,l);end;
14:for k:=0 to n do print(buffer[k]);15:begin fontinshortdisplay:=0;
shortdisplay(n);end;16:panicking:=not panicking;{:1339}
others:print(63)end;end;end;10:end;endif('DEBUG'){:1338}{:1330}{1332:}
procedure texbody;label{6:}1,9998,9999;{:6}var bufindx:0..bufsize;
begin history:=3;;setpaths;if readyalready=314159 then goto 1;{14:}
bad:=0;if(halferrorline<30)or(halferrorline>errorline-15)then bad:=1;
if maxprintline<60 then bad:=2;if dvibufsize mod 8<>0 then bad:=3;
if 1100>memtop then bad:=4;if 7919>9500 then bad:=5;
if maxinopen>=128 then bad:=6;if memtop<267 then bad:=7;{:14}{111:}
ifdef('INITEX')if(memmin<>0)or(memmax<>memtop)then bad:=10;
endif('INITEX')if(memmin>0)or(memmax<memtop)then bad:=10;
if(0>0)or(511<127)then bad:=11;if(0>0)or(262143<32767)then bad:=12;
if(0<0)or(511>262143)then bad:=13;
if(memmin<0)or(memmax>=262143)or(-0-memmin>262144)then bad:=14;
if(0<0)or(fontmax>511)then bad:=15;if fontmax>256 then bad:=16;
if(savesize>262143)or(maxstrings>262143)then bad:=17;
if bufsize>262143 then bad:=18;if 511<255 then bad:=19;{:111}{290:}
if 14376>262143 then bad:=21;{:290}{522:}if 9>FILENAMESIZE then bad:=31;
{:522}{1249:}if 2*262143<memtop-memmin then bad:=41;{:1249}
if bad>0 then begin writeln(stdout,
'Ouch---my internal constants have been clobbered!','---case ',bad:1);
goto 9999;end;initialize;
ifdef('INITEX')if not getstringsstarted then goto 9999;initprim;
initstrptr:=strptr;initpoolptr:=poolptr;
dateandtime(eqtb[12683].int,eqtb[12684].int,eqtb[12685].int,eqtb[12686].
int);endif('INITEX')readyalready:=314159;1:{55:}selector:=17;tally:=0;
termoffset:=0;fileoffset:=0;{:55}{61:}
write(stdout,'This is TeX, C Version 3.0');
if formatident>0 then print(formatident);println;termflush(stdout);{:61}
{528:}jobname:=0;nameinprogress:=false;logopened:=false;{:528}{533:}
outputfilename:=0;{:533};{1337:}begin{331:}begin inputptr:=0;
maxinstack:=0;inopen:=0;openparens:=0;maxbufstack:=0;paramptr:=0;
maxparamstack:=0;bufindx:=bufsize;repeat buffer[bufindx]:=0;
decr(bufindx);until bufindx=0;scannerstatus:=0;warningindex:=0;first:=1;
curinput.statefield:=33;curinput.startfield:=1;curinput.indexfield:=0;
line:=0;curinput.namefield:=0;forceeof:=false;alignstate:=1000000;
if not initterminal then goto 9999;curinput.limitfield:=last;
first:=last+1;end{:331};
if(formatident=0)or(buffer[curinput.locfield]=38)then begin if
formatident<>0 then initialize;if not openfmtfile then goto 9999;
if not loadfmtfile then begin wclose(fmtfile);goto 9999;end;
wclose(fmtfile);
while(curinput.locfield<curinput.limitfield)and(buffer[curinput.locfield
]=32)do incr(curinput.locfield);end;
if(eqtb[12711].int<0)or(eqtb[12711].int>255)then decr(curinput.
limitfield)else buffer[curinput.limitfield]:=eqtb[12711].int;
dateandtime(eqtb[12683].int,eqtb[12684].int,eqtb[12685].int,eqtb[12686].
int);{765:}magicoffset:=strstart[885]-9*16{:765};{75:}
if interaction=0 then selector:=16 else selector:=17{:75};
if(curinput.locfield<curinput.limitfield)and(eqtb[11383+buffer[curinput.
locfield]].hh.rh<>0)then startinput;end{:1337};history:=0;maincontrol;
finalcleanup;9998:closefilesandterminate;9999:begin termflush(stdout);
readyalready:=0;if(history<>0)and(history<>1)then uexit(1)else uexit(0);
end;end;{:1332}
