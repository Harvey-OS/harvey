{2:}program VPtoVF;const{3:}bufsize=60;maxheaderbytes=100;vfsize=10000;
maxstack=100;maxparamwords=30;maxligsteps=5000;maxkerns=500;
hashsize=5003;{:3}{188:}noptions=1;{:188}type{23:}byte=0..255;
ASCIIcode=32..127;{:23}{66:}fourbytes=record b0:byte;b1:byte;b2:byte;
b3:byte;end;{:66}{71:}fixword=integer;{:71}{78:}
headerindex=0..maxheaderbytes;indx=0..32767;{:78}{81:}pointer=0..1032;
{:81}var{5:}vplfile:text;{:5}{21:}vffile:packed file of 0..255;
tfmfile:packed file of 0..255;
vfname,tfmname,vplname:packed array[1..PATHMAX]of char;{:21}{24:}
xord:array[char]of ASCIIcode;{:24}{27:}line:integer;goodindent:integer;
indent:integer;level:integer;{:27}{29:}leftln,rightln:boolean;
limit:0..bufsize;loc:0..bufsize;buffer:array[1..bufsize]of char;
inputhasended:boolean;{:29}{31:}charsonline:0..8;{:31}{36:}
curchar:ASCIIcode;{:36}{44:}start:array[1..100]of 0..666;
dictionary:array[0..666]of ASCIIcode;startptr:0..100;dictptr:0..666;
{:44}{46:}curname:array[1..20]of ASCIIcode;namelength:0..20;
nameptr:0..100;{:46}{47:}nhash:array[0..140]of 0..100;curhash:0..140;
{:47}{52:}equiv:array[0..100]of byte;curcode:byte;{:52}{67:}
curbytes:fourbytes;zerobytes:fourbytes;{:67}{75:}
fractiondigits:array[1..7]of integer;{:75}{77:}
headerbytes:array[headerindex]of byte;headerptr:headerindex;
designsize:fixword;designunits:fixword;frozendu:boolean;
sevenbitsafeflag:boolean;ligkern:array[0..maxligsteps]of fourbytes;
nl:0..32767;minnl:0..32767;kern:array[0..maxkerns]of fixword;
nk:0..maxkerns;exten:array[0..255]of fourbytes;ne:0..256;
param:array[1..maxparamwords]of fixword;np:0..maxparamwords;
checksumspecified:boolean;bchar:0..256;vf:array[0..vfsize]of byte;
vfptr:0..vfsize;vtitlestart:0..vfsize;vtitlelength:byte;
packetstart:array[byte]of 0..vfsize;packetlength:array[byte]of integer;
fontptr:0..256;curfont:0..256;fnamestart:array[byte]of 0..vfsize;
fnamelength:array[byte]of byte;fareastart:array[byte]of 0..vfsize;
farealength:array[byte]of byte;fontchecksum:array[byte]of fourbytes;
fontnumber:array[0..256]of fourbytes;fontat:array[byte]of fixword;
fontdsize:array[byte]of fixword;{:77}{82:}
memory:array[pointer]of fixword;memptr:pointer;
link:array[pointer]of pointer;charwd:array[byte]of pointer;
charht:array[byte]of pointer;chardp:array[byte]of pointer;
charic:array[byte]of pointer;chartag:array[byte]of 0..3;
charremainder:array[0..256]of 0..65535;{:82}{86:}nextd:fixword;{:86}
{89:}index:array[pointer]of byte;excess:byte;{:89}{91:}c:byte;x:fixword;
k:integer;{:91}{113:}lkstepended:boolean;krnptr:0..maxkerns;{:113}{123:}
hstack:array[0..maxstack]of 0..2;vstack:array[0..maxstack]of 0..2;
wstack,xstack,ystack,zstack:array[0..maxstack]of fixword;
stackptr:0..maxstack;{:123}{138:}sevenunsafe:boolean;{:138}{143:}
delta:fixword;{:143}{147:}ligptr:0..maxligsteps;
hash:array[0..hashsize]of 0..66048;class:array[0..hashsize]of 0..4;
ligz:array[0..hashsize]of 0..257;hashptr:0..hashsize;
hashlist:array[0..hashsize]of 0..hashsize;h,hh:0..hashsize;tt:indx;
xligcycle,yligcycle:0..256;{:147}{158:}bc:byte;ec:byte;lh:byte;
lf:0..32767;notfound:boolean;tempwidth:fixword;{:158}{161:}
j:0..maxheaderbytes;p:pointer;q:1..4;parptr:0..maxparamwords;{:161}
{167:}labeltable:array[0..256]of record rr:-1..32767;cc:byte;end;
labelptr:0..256;sortptr:0..256;lkoffset:0..256;t:0..32767;
extralocneeded:boolean;{:167}{175:}vcount:integer;{:175}{185:}
verbose:integer;{:185}procedure initialize;var{25:}k:integer;{:25}{48:}
h:0..140;{:48}{79:}d:headerindex;{:79}{83:}c:byte;{:83}{183:}
longoptions:array[0..noptions]of getoptstruct;getoptreturnval:integer;
optionindex:integer;{:183}begin{6:}
if(argc<4)or(argc>noptions+4)then begin writeln(
'Usage: vptovf [-verbose] <vpl file> <vfm file> <tfm file>.');uexit(1);
end;{186:}verbose:=false;{:186};{182:}begin{184:}
longoptions[0].name:='verbose';longoptions[0].hasarg:=0;
longoptions[0].flag:=addressofint(verbose);longoptions[0].val:=1;{:184}
{187:}longoptions[1].name:=0;longoptions[1].hasarg:=0;
longoptions[1].flag:=0;longoptions[1].val:=0;{:187};
repeat getoptreturnval:=getoptlongonly(argc,gargv,'',longoptions,
addressofint(optionindex));
if getoptreturnval<>-1 then begin if getoptreturnval=63 then uexit(1);
end;until getoptreturnval=-1;end{:182};argv(optind,vplname);
reset(vplfile,vplname);
if verbose then writeln('This is VPtoVF, C Version 1.3');{:6}{22:}
argv(optind+1,vfname);rewrite(vffile,vfname);argv(optind+2,tfmname);
rewrite(tfmfile,tfmname);{:22}{26:}for k:=0 to 127 do xord[chr(k)]:=127;
xord[' ']:=32;xord['!']:=33;xord['"']:=34;xord['#']:=35;xord['$']:=36;
xord['%']:=37;xord['&']:=38;xord['''']:=39;xord['(']:=40;xord[')']:=41;
xord['*']:=42;xord['+']:=43;xord[',']:=44;xord['-']:=45;xord['.']:=46;
xord['/']:=47;xord['0']:=48;xord['1']:=49;xord['2']:=50;xord['3']:=51;
xord['4']:=52;xord['5']:=53;xord['6']:=54;xord['7']:=55;xord['8']:=56;
xord['9']:=57;xord[':']:=58;xord[';']:=59;xord['<']:=60;xord['=']:=61;
xord['>']:=62;xord['?']:=63;xord['@']:=64;xord['A']:=65;xord['B']:=66;
xord['C']:=67;xord['D']:=68;xord['E']:=69;xord['F']:=70;xord['G']:=71;
xord['H']:=72;xord['I']:=73;xord['J']:=74;xord['K']:=75;xord['L']:=76;
xord['M']:=77;xord['N']:=78;xord['O']:=79;xord['P']:=80;xord['Q']:=81;
xord['R']:=82;xord['S']:=83;xord['T']:=84;xord['U']:=85;xord['V']:=86;
xord['W']:=87;xord['X']:=88;xord['Y']:=89;xord['Z']:=90;xord['[']:=91;
xord['\']:=92;xord[']']:=93;xord['^']:=94;xord['_']:=95;xord['`']:=96;
xord['a']:=97;xord['b']:=98;xord['c']:=99;xord['d']:=100;xord['e']:=101;
xord['f']:=102;xord['g']:=103;xord['h']:=104;xord['i']:=105;
xord['j']:=106;xord['k']:=107;xord['l']:=108;xord['m']:=109;
xord['n']:=110;xord['o']:=111;xord['p']:=112;xord['q']:=113;
xord['r']:=114;xord['s']:=115;xord['t']:=116;xord['u']:=117;
xord['v']:=118;xord['w']:=119;xord['x']:=120;xord['y']:=121;
xord['z']:=122;xord['{']:=123;xord['|']:=124;xord['}']:=125;
xord['~']:=126;{:26}{28:}line:=0;goodindent:=0;indent:=0;level:=0;{:28}
{30:}limit:=0;loc:=0;leftln:=true;rightln:=true;inputhasended:=false;
{:30}{32:}charsonline:=0;{:32}{45:}startptr:=1;start[1]:=0;dictptr:=0;
{:45}{49:}for h:=0 to 140 do nhash[h]:=0;{:49}{68:}zerobytes.b0:=0;
zerobytes.b1:=0;zerobytes.b2:=0;zerobytes.b3:=0;{:68}{80:}
for d:=0 to 18*4-1 do headerbytes[d]:=0;headerbytes[8]:=11;
headerbytes[9]:=85;headerbytes[10]:=78;headerbytes[11]:=83;
headerbytes[12]:=80;headerbytes[13]:=69;headerbytes[14]:=67;
headerbytes[15]:=73;headerbytes[16]:=70;headerbytes[17]:=73;
headerbytes[18]:=69;headerbytes[19]:=68;
for d:=48 to 59 do headerbytes[d]:=headerbytes[d-40];
designsize:=10*1048576;designunits:=1048576;frozendu:=false;
sevenbitsafeflag:=false;headerptr:=18*4;nl:=0;minnl:=0;nk:=0;ne:=0;
np:=0;checksumspecified:=false;bchar:=256;vfptr:=0;vtitlestart:=0;
vtitlelength:=0;fontptr:=0;for k:=0 to 255 do packetstart[k]:=vfsize;
for k:=0 to 127 do packetlength[k]:=1;
for k:=128 to 255 do packetlength[k]:=2;{:80}{84:}
charremainder[256]:=32767;for c:=0 to 255 do begin charwd[c]:=0;
charht[c]:=0;chardp[c]:=0;charic[c]:=0;chartag[c]:=0;
charremainder[c]:=0;end;memory[0]:=2147483647;memory[1]:=0;link[1]:=0;
memory[2]:=0;link[2]:=0;memory[3]:=0;link[3]:=0;memory[4]:=0;link[4]:=0;
memptr:=4;{:84}{148:}hashptr:=0;yligcycle:=256;
for k:=0 to hashsize do hash[k]:=0;{:148}end;{:2}{33:}
procedure showerrorcontext;var k:0..bufsize;
begin writeln(' (line ',line:1,').');if not leftln then write('...');
for k:=1 to loc do write(buffer[k]);writeln(' ');
if not leftln then write('   ');for k:=1 to loc do write(' ');
for k:=loc+1 to limit do write(buffer[k]);
if rightln then writeln(' ')else writeln('...');charsonline:=0;end;{:33}
{34:}procedure fillbuffer;begin leftln:=rightln;limit:=0;loc:=0;
if leftln then begin if line>0 then readln(vplfile);line:=line+1;end;
if eof(vplfile)then begin limit:=1;buffer[1]:=')';rightln:=false;
inputhasended:=true;
end else begin while(limit<bufsize-1)and(not eoln(vplfile))do begin
limit:=limit+1;read(vplfile,buffer[limit]);end;buffer[limit+1]:=' ';
rightln:=eoln(vplfile);if leftln then{35:}
begin while(loc<limit)and(buffer[loc+1]=' ')do loc:=loc+1;
if loc<limit then begin if level=0 then if loc=0 then goodindent:=
goodindent+1 else begin if goodindent>=10 then begin if charsonline>0
then writeln(' ');
write('Warning: Indented line occurred at level zero');showerrorcontext;
end;goodindent:=0;indent:=0;
end else if indent=0 then if loc mod level=0 then begin indent:=loc div
level;goodindent:=1;
end else goodindent:=0 else if indent*level=loc then goodindent:=
goodindent+1 else begin if goodindent>=10 then begin if charsonline>0
then writeln(' ');write('Warning: Inconsistent indentation; ',
'you are at parenthesis level ',level:1);showerrorcontext;end;
goodindent:=0;indent:=0;end;end;end{:35};end;end;{:34}{37:}
procedure getkeywordchar;
begin while(loc=limit)and(not rightln)do fillbuffer;
if loc=limit then curchar:=32 else begin curchar:=xord[buffer[loc+1]];
if curchar>=97 then curchar:=curchar-32;
if((curchar>=48)and(curchar<=57))then loc:=loc+1 else if((curchar>=65)
and(curchar<=90))then loc:=loc+1 else if curchar=47 then loc:=loc+1 else
if curchar=62 then loc:=loc+1 else curchar:=32;end;end;{:37}{38:}
procedure getnext;begin while loc=limit do fillbuffer;loc:=loc+1;
curchar:=xord[buffer[loc]];
if curchar>=97 then if curchar<=122 then curchar:=curchar-32 else begin
if curchar=127 then begin begin if charsonline>0 then writeln(' ');
write('Illegal character in the file');showerrorcontext;end;curchar:=63;
end;end else if(curchar<=41)and(curchar>=40)then loc:=loc-1;end;{:38}
{39:}function gethex:byte;var a:integer;begin repeat getnext;
until curchar<>32;a:=curchar-41;if a>0 then begin a:=curchar-48;
if curchar>57 then if curchar<65 then a:=-1 else a:=curchar-55;end;
if(a<0)or(a>15)then begin begin if charsonline>0 then writeln(' ');
write('Illegal hexadecimal digit');showerrorcontext;end;gethex:=0;
end else gethex:=a;end;{:39}{40:}procedure skiptoendofitem;
var l:integer;begin l:=level;
while level>=l do begin while loc=limit do fillbuffer;loc:=loc+1;
if buffer[loc]=')'then level:=level-1 else if buffer[loc]='('then level
:=level+1;end;
if inputhasended then begin if charsonline>0 then writeln(' ');
write('File ended unexpectedly: No closing ")"');showerrorcontext;end;
curchar:=32;end;{:40}{41:}procedure copytoendofitem;label 30;
var l:integer;nonblankfound:boolean;begin l:=level;nonblankfound:=false;
while true do begin while loc=limit do fillbuffer;
if buffer[loc+1]=')'then if level=l then goto 30 else level:=level-1;
loc:=loc+1;if buffer[loc]='('then level:=level+1;
if buffer[loc]<>' 'then nonblankfound:=true;
if nonblankfound then if xord[buffer[loc]]=127 then begin begin if
charsonline>0 then writeln(' ');write('Illegal character in the file');
showerrorcontext;end;begin vf[vfptr]:=63;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end;end else begin vf[vfptr]:=xord[buffer[loc]];
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end;end;30:end;{:41}{43:}
procedure finishtheproperty;begin while curchar=32 do getnext;
if curchar<>41 then begin if charsonline>0 then writeln(' ');
write('Junk after property value will be ignored');showerrorcontext;end;
skiptoendofitem;end;{:43}{50:}procedure lookup;var k:0..20;j:0..666;
notfound:boolean;begin{51:}curhash:=curname[1];
for k:=2 to namelength do curhash:=(curhash+curhash+curname[k])mod 141{:
51};notfound:=true;
while notfound do begin if curhash=0 then curhash:=140 else curhash:=
curhash-1;
if nhash[curhash]=0 then notfound:=false else begin j:=start[nhash[
curhash]];
if start[nhash[curhash]+1]=j+namelength then begin notfound:=false;
for k:=1 to namelength do if dictionary[j+k-1]<>curname[k]then notfound
:=true;end;end;end;nameptr:=nhash[curhash];end;{:50}{53:}
procedure entername(v:byte);var k:0..20;
begin for k:=1 to namelength do curname[k]:=curname[k+20-namelength];
lookup;nhash[curhash]:=startptr;equiv[startptr]:=v;
for k:=1 to namelength do begin dictionary[dictptr]:=curname[k];
dictptr:=dictptr+1;end;startptr:=startptr+1;start[startptr]:=dictptr;
end;{:53}{58:}procedure getname;begin loc:=loc+1;level:=level+1;
curchar:=32;while curchar=32 do getnext;
if(curchar>41)or(curchar<40)then loc:=loc-1;namelength:=0;
getkeywordchar;
while curchar<>32 do begin if namelength=20 then curname[1]:=88 else
namelength:=namelength+1;curname[namelength]:=curchar;getkeywordchar;
end;lookup;if nameptr=0 then begin if charsonline>0 then writeln(' ');
write('Sorry, I don''t know that property name');showerrorcontext;end;
curcode:=equiv[nameptr];end;{:58}{60:}function getbyte:byte;
var acc:integer;t:ASCIIcode;begin repeat getnext;until curchar<>32;
t:=curchar;acc:=0;repeat getnext;until curchar<>32;if t=67 then{61:}
if(curchar>=33)and(curchar<=126)and((curchar<40)or(curchar>41))then acc
:=xord[buffer[loc]]else begin begin if charsonline>0 then writeln(' ');
write('"C" value must be standard ASCII and not a paren');
showerrorcontext;end;repeat getnext until(curchar=40)or(curchar=41);
end{:61}else if t=68 then{62:}
begin while(curchar>=48)and(curchar<=57)do begin acc:=acc*10+curchar-48;
if acc>255 then begin begin begin if charsonline>0 then writeln(' ');
write('This value shouldn''t exceed 255');showerrorcontext;end;
repeat getnext until(curchar=40)or(curchar=41);end;acc:=0;curchar:=32;
end else getnext;end;begin if(curchar>41)or(curchar<40)then loc:=loc-1;
end;end{:62}else if t=79 then{63:}
begin while(curchar>=48)and(curchar<=55)do begin acc:=acc*8+curchar-48;
if acc>255 then begin begin begin if charsonline>0 then writeln(' ');
write('This value shouldn''t exceed ''377');showerrorcontext;end;
repeat getnext until(curchar=40)or(curchar=41);end;acc:=0;curchar:=32;
end else getnext;end;begin if(curchar>41)or(curchar<40)then loc:=loc-1;
end;end{:63}else if t=72 then{64:}
begin while((curchar>=48)and(curchar<=57))or((curchar>=65)and(curchar<=
70))do begin if curchar>=65 then curchar:=curchar-7;
acc:=acc*16+curchar-48;
if acc>255 then begin begin begin if charsonline>0 then writeln(' ');
write('This value shouldn''t exceed "FF');showerrorcontext;end;
repeat getnext until(curchar=40)or(curchar=41);end;acc:=0;curchar:=32;
end else getnext;end;begin if(curchar>41)or(curchar<40)then loc:=loc-1;
end;end{:64}else if t=70 then{65:}
begin if curchar=66 then acc:=2 else if curchar=76 then acc:=4 else if
curchar<>77 then acc:=18;getnext;
if curchar=73 then acc:=acc+1 else if curchar<>82 then acc:=18;getnext;
if curchar=67 then acc:=acc+6 else if curchar=69 then acc:=acc+12 else
if curchar<>82 then acc:=18;
if acc>=18 then begin begin begin if charsonline>0 then writeln(' ');
write('Illegal face code, I changed it to MRR');showerrorcontext;end;
repeat getnext until(curchar=40)or(curchar=41);end;acc:=0;end;end{:65}
else begin begin if charsonline>0 then writeln(' ');
write('You need "C" or "D" or "O" or "H" or "F" here');showerrorcontext;
end;repeat getnext until(curchar=40)or(curchar=41);end;curchar:=32;
getbyte:=acc;end;{:60}{69:}procedure getfourbytes;var c:integer;
r:integer;begin repeat getnext;until curchar<>32;r:=0;
curbytes:=zerobytes;
if curchar=72 then r:=16 else if curchar=79 then r:=8 else if curchar=68
then r:=10 else begin begin if charsonline>0 then writeln(' ');
write('Decimal ("D"), octal ("O"), or hex ("H") value needed here');
showerrorcontext;end;repeat getnext until(curchar=40)or(curchar=41);end;
if r>0 then begin repeat getnext;until curchar<>32;
while((curchar>=48)and(curchar<=57))or((curchar>=65)and(curchar<=70))do{
70:}begin if curchar>=65 then curchar:=curchar-7;
if curchar>=48+r then begin begin if charsonline>0 then writeln(' ');
write('Illegal digit');showerrorcontext;end;
repeat getnext until(curchar=40)or(curchar=41);
end else begin c:=curbytes.b3*r+curchar-48;curbytes.b3:=c mod 256;
c:=curbytes.b2*r+c div 256;curbytes.b2:=c mod 256;
c:=curbytes.b1*r+c div 256;curbytes.b1:=c mod 256;
c:=curbytes.b0*r+c div 256;
if c<256 then curbytes.b0:=c else begin curbytes:=zerobytes;
if r=8 then begin begin if charsonline>0 then writeln(' ');
write('Sorry, the maximum octal value is O 37777777777');
showerrorcontext;end;repeat getnext until(curchar=40)or(curchar=41);
end else if r=10 then begin begin if charsonline>0 then writeln(' ');
write('Sorry, the maximum decimal value is D 4294967295');
showerrorcontext;end;repeat getnext until(curchar=40)or(curchar=41);
end else begin begin if charsonline>0 then writeln(' ');
write('Sorry, the maximum hex value is H FFFFFFFF');showerrorcontext;
end;repeat getnext until(curchar=40)or(curchar=41);end;end;getnext;end;
end{:70};end;end;{:69}{72:}function getfix:fixword;var negative:boolean;
acc:integer;intpart:integer;j:0..7;begin repeat getnext;
until curchar<>32;negative:=false;acc:=0;
if(curchar<>82)and(curchar<>68)then begin begin if charsonline>0 then
writeln(' ');write('An "R" or "D" value is needed here');
showerrorcontext;end;repeat getnext until(curchar=40)or(curchar=41);
end else begin{73:}repeat getnext;if curchar=45 then begin curchar:=32;
negative:=true;end else if curchar=43 then curchar:=32;
until curchar<>32{:73};while(curchar>=48)and(curchar<=57)do{74:}
begin acc:=acc*10+curchar-48;
if acc>=2048 then begin begin begin if charsonline>0 then writeln(' ');
write('Real constants must be less than 2048');showerrorcontext;end;
repeat getnext until(curchar=40)or(curchar=41);end;acc:=0;curchar:=32;
end else getnext;end{:74};intpart:=acc;acc:=0;if curchar=46 then{76:}
begin j:=0;getnext;
while(curchar>=48)and(curchar<=57)do begin if j<7 then begin j:=j+1;
fractiondigits[j]:=2097152*(curchar-48);end;getnext;end;acc:=0;
while j>0 do begin acc:=fractiondigits[j]+(acc div 10);j:=j-1;end;
acc:=(acc+10)div 20;end{:76};
if(acc>=1048576)and(intpart=2047)then begin begin if charsonline>0 then
writeln(' ');write('Real constants must be less than 2048');
showerrorcontext;end;repeat getnext until(curchar=40)or(curchar=41);
end else acc:=intpart*1048576+acc;end;
if negative then getfix:=-acc else getfix:=acc;end;{:72}{85:}
function sortin(h:pointer;d:fixword):pointer;var p:pointer;
begin if(d=0)and(h<>1)then sortin:=0 else begin p:=h;
while d>=memory[link[p]]do p:=link[p];
if(d=memory[p])and(p<>h)then sortin:=p else if memptr=1032 then begin
begin if charsonline>0 then writeln(' ');
write('Memory overflow: more than 1028 widths, etc');showerrorcontext;
end;writeln('Congratulations! It''s hard to make this error.');
sortin:=p;end else begin memptr:=memptr+1;memory[memptr]:=d;
link[memptr]:=link[p];link[p]:=memptr;memory[h]:=memory[h]+1;
sortin:=memptr;end;end;end;{:85}{87:}function mincover(h:pointer;
d:fixword):integer;var p:pointer;l:fixword;m:integer;begin m:=0;
p:=link[h];nextd:=memory[0];while p<>0 do begin m:=m+1;l:=memory[p];
while memory[link[p]]<=l+d do p:=link[p];p:=link[p];
if memory[p]-l<nextd then nextd:=memory[p]-l;end;mincover:=m;end;{:87}
{88:}function shorten(h:pointer;m:integer):fixword;var d:fixword;
k:integer;begin if memory[h]>m then begin excess:=memory[h]-m;
k:=mincover(h,0);d:=nextd;repeat d:=d+d;k:=mincover(h,d);until k<=m;
d:=d div 2;k:=mincover(h,d);while k>m do begin d:=nextd;
k:=mincover(h,d);end;shorten:=d;end else shorten:=0;end;{:88}{90:}
procedure setindices(h:pointer;d:fixword);var p:pointer;q:pointer;
m:byte;l:fixword;begin q:=h;p:=link[q];m:=0;while p<>0 do begin m:=m+1;
l:=memory[p];index[p]:=m;while memory[link[p]]<=l+d do begin p:=link[p];
index[p]:=m;excess:=excess-1;if excess=0 then d:=0;end;link[q]:=p;
memory[p]:=l+(memory[p]-l)div 2;q:=p;p:=link[p];end;memory[h]:=m;end;
{:90}{93:}procedure junkerror;
begin begin if charsonline>0 then writeln(' ');
write('There''s junk here that is not in parentheses');showerrorcontext;
end;repeat getnext until(curchar=40)or(curchar=41);end;{:93}{96:}
procedure readfourbytes(l:headerindex);begin getfourbytes;
headerbytes[l]:=curbytes.b0;headerbytes[l+1]:=curbytes.b1;
headerbytes[l+2]:=curbytes.b2;headerbytes[l+3]:=curbytes.b3;end;{:96}
{97:}procedure readBCPL(l:headerindex;n:byte);var k:headerindex;
begin k:=l;while curchar=32 do getnext;
while(curchar<>40)and(curchar<>41)do begin if k<l+n then k:=k+1;
if k<l+n then headerbytes[k]:=curchar;getnext;end;
if k=l+n then begin begin if charsonline>0 then writeln(' ');
write('String is too long; its first ',n-1:1,' characters will be kept')
;showerrorcontext;end;k:=k-1;end;headerbytes[l]:=k-l;
while k<l+n-1 do begin k:=k+1;headerbytes[k]:=0;end;end;{:97}{111:}
procedure checktag(c:byte);begin case chartag[c]of 0:;
1:begin if charsonline>0 then writeln(' ');
write('This character already appeared in a LIGTABLE LABEL');
showerrorcontext;end;2:begin if charsonline>0 then writeln(' ');
write('This character already has a NEXTLARGER spec');showerrorcontext;
end;3:begin if charsonline>0 then writeln(' ');
write('This character already has a VARCHAR spec');showerrorcontext;end;
end;end;{:111}{124:}{128:}procedure vffix(opcode:byte;x:fixword);
var negative:boolean;k:0..4;t:integer;begin frozendu:=true;
if designunits<>1048576 then x:=round((x/designunits)*1048576.0);
if x>0 then negative:=false else begin negative:=true;x:=-1-x;end;
if opcode=0 then begin k:=4;t:=16777216;end else begin t:=127;k:=1;
while x>t do begin t:=256*t+255;k:=k+1;end;begin vf[vfptr]:=opcode+k-1;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end;t:=t div 128+1;end;
repeat if negative then begin begin vf[vfptr]:=255-(x div t);
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end;negative:=false;x:=(x div t)*t+t-1-x;
end else begin vf[vfptr]:=(x div t)mod 256;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end;k:=k-1;t:=t div 256;until k=0;end;{:128}
procedure readpacket(c:byte);var cc:byte;x:fixword;h,v:0..2;
specialstart:0..vfsize;k:0..vfsize;begin packetstart[c]:=vfptr;
stackptr:=0;h:=0;v:=0;curfont:=0;
while level=2 do begin while curchar=32 do getnext;
if curchar=40 then{125:}begin getname;
if curcode=0 then skiptoendofitem else if(curcode<80)or(curcode>90)then
begin begin if charsonline>0 then writeln(' ');
write('This property name doesn''t belong in a MAP list');
showerrorcontext;end;skiptoendofitem;
end else begin case curcode of 80:{126:}begin getfourbytes;
fontnumber[fontptr]:=curbytes;curfont:=0;
while(fontnumber[curfont].b3<>fontnumber[fontptr].b3)or(fontnumber[
curfont].b2<>fontnumber[fontptr].b2)or(fontnumber[curfont].b1<>
fontnumber[fontptr].b1)or(fontnumber[curfont].b0<>fontnumber[fontptr].b0
)do curfont:=curfont+1;
if curfont=fontptr then begin if charsonline>0 then writeln(' ');
write('Undefined MAPFONT cannot be selected');showerrorcontext;
end else if curfont<64 then begin vf[vfptr]:=171+curfont;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end else begin begin vf[vfptr]:=235;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end;begin vf[vfptr]:=curfont;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end;end;end{:126};81:{127:}
if curfont=fontptr then begin if charsonline>0 then writeln(' ');
write('Character cannot be typeset in undefined font');showerrorcontext;
end else begin cc:=getbyte;if cc>=128 then begin vf[vfptr]:=128;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end;begin vf[vfptr]:=cc;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end;end{:127};82:{129:}
begin begin vf[vfptr]:=132;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end;vffix(0,getfix);vffix(0,getfix);end{:129};
83,84:{130:}begin if curcode=83 then x:=getfix else x:=-getfix;
if h=0 then begin wstack[stackptr]:=x;h:=1;vffix(148,x);
end else if x=wstack[stackptr]then begin vf[vfptr]:=147;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end else if h=1 then begin xstack[stackptr]:=x;
h:=2;vffix(153,x);
end else if x=xstack[stackptr]then begin vf[vfptr]:=152;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end else vffix(143,x);end{:130};85,86:{131:}
begin if curcode=85 then x:=getfix else x:=-getfix;
if v=0 then begin ystack[stackptr]:=x;v:=1;vffix(162,x);
end else if x=ystack[stackptr]then begin vf[vfptr]:=161;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end else if v=1 then begin zstack[stackptr]:=x;
v:=2;vffix(167,x);
end else if x=zstack[stackptr]then begin vf[vfptr]:=166;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end else vffix(157,x);end{:131};87:{132:}
if stackptr=maxstack then begin if charsonline>0 then writeln(' ');
write('Don''t push so much---stack is full!');showerrorcontext;
end else begin begin vf[vfptr]:=141;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end;hstack[stackptr]:=h;vstack[stackptr]:=v;
stackptr:=stackptr+1;h:=0;v:=0;end{:132};88:{133:}
if stackptr=0 then begin if charsonline>0 then writeln(' ');
write('Empty stack cannot be popped');showerrorcontext;
end else begin begin vf[vfptr]:=142;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end;stackptr:=stackptr-1;h:=hstack[stackptr];
v:=vstack[stackptr];end{:133};89,90:{134:}begin begin vf[vfptr]:=239;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end;begin vf[vfptr]:=0;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end;specialstart:=vfptr;
if curcode=89 then copytoendofitem else begin repeat x:=gethex;
if curchar>41 then begin vf[vfptr]:=x*16+gethex;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end;until curchar<=41;end;
if vfptr-specialstart>255 then{135:}
if vfptr+3>vfsize then begin begin if charsonline>0 then writeln(' ');
write('Special command being clipped---no room left!');showerrorcontext;
end;vfptr:=specialstart+255;vf[specialstart-1]:=255;
end else begin for k:=vfptr downto specialstart do vf[k+3]:=vf[k];
x:=vfptr-specialstart;vfptr:=vfptr+3;vf[specialstart-2]:=242;
vf[specialstart-1]:=x div 16777216;
vf[specialstart]:=(x div 65536)mod 256;
vf[specialstart+1]:=(x div 256)mod 256;vf[specialstart+2]:=x mod 256;
end{:135}else vf[specialstart-1]:=vfptr-specialstart;end{:134};end;
finishtheproperty;end;end{:125}
else if curchar=41 then skiptoendofitem else junkerror;end;
while stackptr>0 do begin begin if charsonline>0 then writeln(' ');
write('Missing POP supplied');showerrorcontext;end;begin vf[vfptr]:=142;
if vfptr=vfsize then begin if charsonline>0 then writeln(' ');
write('I''m out of memory---increase my vfsize!');showerrorcontext;
end else vfptr:=vfptr+1;end;stackptr:=stackptr-1;end;
packetlength[c]:=vfptr-packetstart[c];begin loc:=loc-1;level:=level+1;
curchar:=41;end;end;{:124}{136:}procedure printoctal(c:byte);
begin write('''',(c div 64):1,((c div 8)mod 8):1,(c mod 8):1);end;{:136}
{150:}function hashinput(p,c:indx):boolean;label 30;var cc:0..3;
zz:0..255;y:0..255;key:integer;t:integer;
begin if hashptr=hashsize then begin hashinput:=false;goto 30;end;{151:}
y:=ligkern[p].b1;t:=ligkern[p].b2;cc:=0;zz:=ligkern[p].b3;
if t>=128 then zz:=y else begin case t of 0,6:;5,11:zz:=y;1,7:cc:=1;
2:cc:=2;3:cc:=3;end;end{:151};key:=256*c+y+1;h:=(1009*key)mod hashsize;
while hash[h]>0 do begin if hash[h]<=key then begin if hash[h]=key then
begin hashinput:=false;goto 30;end;t:=hash[h];hash[h]:=key;key:=t;
t:=class[h];class[h]:=cc;cc:=t;t:=ligz[h];ligz[h]:=zz;zz:=t;end;
if h>0 then h:=h-1 else h:=hashsize;end;hash[h]:=key;class[h]:=cc;
ligz[h]:=zz;hashptr:=hashptr+1;hashlist[hashptr]:=h;hashinput:=true;
30:end;{:150}{152:}ifdef('notdef')function f(h,x,y:indx):indx;begin end;
endif('notdef')function eval(x,y:indx):indx;var key:integer;
begin key:=256*x+y+1;h:=(1009*key)mod hashsize;
while hash[h]>key do if h>0 then h:=h-1 else h:=hashsize;
if hash[h]<key then eval:=y else eval:=f(h,x,y);end;{:152}{153:}
function f(h,x,y:indx):indx;begin case class[h]of 0:;
1:begin class[h]:=4;ligz[h]:=eval(ligz[h],y);class[h]:=0;end;
2:begin class[h]:=4;ligz[h]:=eval(x,ligz[h]);class[h]:=0;end;
3:begin class[h]:=4;ligz[h]:=eval(eval(x,ligz[h]),y);class[h]:=0;end;
4:begin xligcycle:=x;yligcycle:=y;ligz[h]:=257;class[h]:=0;end;end;
f:=ligz[h];end;{:153}{165:}procedure outscaled(x:fixword);var n:byte;
m:0..65535;begin if fabs(x/designunits)>=16.0 then begin write(
'The relative dimension ');printreal(x/1048576,1,3);
writeln(' is too large.');write('  (Must be less than 16*designsize');
if designunits<>1048576 then begin write(' =');
printreal(designunits/65536,1,3);write(' designunits');end;writeln(')');
x:=0;end;
if designunits<>1048576 then x:=round((x/designunits)*1048576.0);
if x<0 then begin putbyte(255,tfmfile);x:=x+16777216;if x<=0 then x:=1;
end else begin putbyte(0,tfmfile);if x>=16777216 then x:=16777215;end;
n:=x div 65536;m:=x mod 65536;putbyte(n,tfmfile);
putbyte(m div 256,tfmfile);putbyte(m mod 256,tfmfile);end;{:165}{176:}
procedure voutint(x:integer);
begin if x>=0 then putbyte(x div 16777216,vffile)else begin putbyte(255,
vffile);x:=x+16777216;end;putbyte((x div 65536)mod 256,vffile);
putbyte((x div 256)mod 256,vffile);putbyte(x mod 256,vffile);end;{:176}
{180:}procedure paramenter;begin{57:}namelength:=5;curname[16]:=83;
curname[17]:=76;curname[18]:=65;curname[19]:=78;curname[20]:=84;
entername(31);namelength:=5;curname[16]:=83;curname[17]:=80;
curname[18]:=65;curname[19]:=67;curname[20]:=69;entername(32);
namelength:=7;curname[14]:=83;curname[15]:=84;curname[16]:=82;
curname[17]:=69;curname[18]:=84;curname[19]:=67;curname[20]:=72;
entername(33);namelength:=6;curname[15]:=83;curname[16]:=72;
curname[17]:=82;curname[18]:=73;curname[19]:=78;curname[20]:=75;
entername(34);namelength:=7;curname[14]:=88;curname[15]:=72;
curname[16]:=69;curname[17]:=73;curname[18]:=71;curname[19]:=72;
curname[20]:=84;entername(35);namelength:=4;curname[17]:=81;
curname[18]:=85;curname[19]:=65;curname[20]:=68;entername(36);
namelength:=10;curname[11]:=69;curname[12]:=88;curname[13]:=84;
curname[14]:=82;curname[15]:=65;curname[16]:=83;curname[17]:=80;
curname[18]:=65;curname[19]:=67;curname[20]:=69;entername(37);
namelength:=4;curname[17]:=78;curname[18]:=85;curname[19]:=77;
curname[20]:=49;entername(38);namelength:=4;curname[17]:=78;
curname[18]:=85;curname[19]:=77;curname[20]:=50;entername(39);
namelength:=4;curname[17]:=78;curname[18]:=85;curname[19]:=77;
curname[20]:=51;entername(40);namelength:=6;curname[15]:=68;
curname[16]:=69;curname[17]:=78;curname[18]:=79;curname[19]:=77;
curname[20]:=49;entername(41);namelength:=6;curname[15]:=68;
curname[16]:=69;curname[17]:=78;curname[18]:=79;curname[19]:=77;
curname[20]:=50;entername(42);namelength:=4;curname[17]:=83;
curname[18]:=85;curname[19]:=80;curname[20]:=49;entername(43);
namelength:=4;curname[17]:=83;curname[18]:=85;curname[19]:=80;
curname[20]:=50;entername(44);namelength:=4;curname[17]:=83;
curname[18]:=85;curname[19]:=80;curname[20]:=51;entername(45);
namelength:=4;curname[17]:=83;curname[18]:=85;curname[19]:=66;
curname[20]:=49;entername(46);namelength:=4;curname[17]:=83;
curname[18]:=85;curname[19]:=66;curname[20]:=50;entername(47);
namelength:=7;curname[14]:=83;curname[15]:=85;curname[16]:=80;
curname[17]:=68;curname[18]:=82;curname[19]:=79;curname[20]:=80;
entername(48);namelength:=7;curname[14]:=83;curname[15]:=85;
curname[16]:=66;curname[17]:=68;curname[18]:=82;curname[19]:=79;
curname[20]:=80;entername(49);namelength:=6;curname[15]:=68;
curname[16]:=69;curname[17]:=76;curname[18]:=73;curname[19]:=77;
curname[20]:=49;entername(50);namelength:=6;curname[15]:=68;
curname[16]:=69;curname[17]:=76;curname[18]:=73;curname[19]:=77;
curname[20]:=50;entername(51);namelength:=10;curname[11]:=65;
curname[12]:=88;curname[13]:=73;curname[14]:=83;curname[15]:=72;
curname[16]:=69;curname[17]:=73;curname[18]:=71;curname[19]:=72;
curname[20]:=84;entername(52);namelength:=20;curname[1]:=68;
curname[2]:=69;curname[3]:=70;curname[4]:=65;curname[5]:=85;
curname[6]:=76;curname[7]:=84;curname[8]:=82;curname[9]:=85;
curname[10]:=76;curname[11]:=69;curname[12]:=84;curname[13]:=72;
curname[14]:=73;curname[15]:=67;curname[16]:=75;curname[17]:=78;
curname[18]:=69;curname[19]:=83;curname[20]:=83;entername(38);
namelength:=13;curname[8]:=66;curname[9]:=73;curname[10]:=71;
curname[11]:=79;curname[12]:=80;curname[13]:=83;curname[14]:=80;
curname[15]:=65;curname[16]:=67;curname[17]:=73;curname[18]:=78;
curname[19]:=71;curname[20]:=49;entername(39);namelength:=13;
curname[8]:=66;curname[9]:=73;curname[10]:=71;curname[11]:=79;
curname[12]:=80;curname[13]:=83;curname[14]:=80;curname[15]:=65;
curname[16]:=67;curname[17]:=73;curname[18]:=78;curname[19]:=71;
curname[20]:=50;entername(40);namelength:=13;curname[8]:=66;
curname[9]:=73;curname[10]:=71;curname[11]:=79;curname[12]:=80;
curname[13]:=83;curname[14]:=80;curname[15]:=65;curname[16]:=67;
curname[17]:=73;curname[18]:=78;curname[19]:=71;curname[20]:=51;
entername(41);namelength:=13;curname[8]:=66;curname[9]:=73;
curname[10]:=71;curname[11]:=79;curname[12]:=80;curname[13]:=83;
curname[14]:=80;curname[15]:=65;curname[16]:=67;curname[17]:=73;
curname[18]:=78;curname[19]:=71;curname[20]:=52;entername(42);
namelength:=13;curname[8]:=66;curname[9]:=73;curname[10]:=71;
curname[11]:=79;curname[12]:=80;curname[13]:=83;curname[14]:=80;
curname[15]:=65;curname[16]:=67;curname[17]:=73;curname[18]:=78;
curname[19]:=71;curname[20]:=53;entername(43);{:57};end;
procedure vplenter;begin{56:}namelength:=6;curname[15]:=86;
curname[16]:=84;curname[17]:=73;curname[18]:=84;curname[19]:=76;
curname[20]:=69;entername(12);namelength:=7;curname[14]:=77;
curname[15]:=65;curname[16]:=80;curname[17]:=70;curname[18]:=79;
curname[19]:=78;curname[20]:=84;entername(13);namelength:=3;
curname[18]:=77;curname[19]:=65;curname[20]:=80;entername(66);
namelength:=8;curname[13]:=70;curname[14]:=79;curname[15]:=78;
curname[16]:=84;curname[17]:=78;curname[18]:=65;curname[19]:=77;
curname[20]:=69;entername(20);namelength:=8;curname[13]:=70;
curname[14]:=79;curname[15]:=78;curname[16]:=84;curname[17]:=65;
curname[18]:=82;curname[19]:=69;curname[20]:=65;entername(21);
namelength:=12;curname[9]:=70;curname[10]:=79;curname[11]:=78;
curname[12]:=84;curname[13]:=67;curname[14]:=72;curname[15]:=69;
curname[16]:=67;curname[17]:=75;curname[18]:=83;curname[19]:=85;
curname[20]:=77;entername(22);namelength:=6;curname[15]:=70;
curname[16]:=79;curname[17]:=78;curname[18]:=84;curname[19]:=65;
curname[20]:=84;entername(23);namelength:=9;curname[12]:=70;
curname[13]:=79;curname[14]:=78;curname[15]:=84;curname[16]:=68;
curname[17]:=83;curname[18]:=73;curname[19]:=90;curname[20]:=69;
entername(24);namelength:=10;curname[11]:=83;curname[12]:=69;
curname[13]:=76;curname[14]:=69;curname[15]:=67;curname[16]:=84;
curname[17]:=70;curname[18]:=79;curname[19]:=78;curname[20]:=84;
entername(80);namelength:=7;curname[14]:=83;curname[15]:=69;
curname[16]:=84;curname[17]:=67;curname[18]:=72;curname[19]:=65;
curname[20]:=82;entername(81);namelength:=7;curname[14]:=83;
curname[15]:=69;curname[16]:=84;curname[17]:=82;curname[18]:=85;
curname[19]:=76;curname[20]:=69;entername(82);namelength:=9;
curname[12]:=77;curname[13]:=79;curname[14]:=86;curname[15]:=69;
curname[16]:=82;curname[17]:=73;curname[18]:=71;curname[19]:=72;
curname[20]:=84;entername(83);namelength:=8;curname[13]:=77;
curname[14]:=79;curname[15]:=86;curname[16]:=69;curname[17]:=76;
curname[18]:=69;curname[19]:=70;curname[20]:=84;entername(84);
namelength:=8;curname[13]:=77;curname[14]:=79;curname[15]:=86;
curname[16]:=69;curname[17]:=68;curname[18]:=79;curname[19]:=87;
curname[20]:=78;entername(85);namelength:=6;curname[15]:=77;
curname[16]:=79;curname[17]:=86;curname[18]:=69;curname[19]:=85;
curname[20]:=80;entername(86);namelength:=4;curname[17]:=80;
curname[18]:=85;curname[19]:=83;curname[20]:=72;entername(87);
namelength:=3;curname[18]:=80;curname[19]:=79;curname[20]:=80;
entername(88);namelength:=7;curname[14]:=83;curname[15]:=80;
curname[16]:=69;curname[17]:=67;curname[18]:=73;curname[19]:=65;
curname[20]:=76;entername(89);namelength:=10;curname[11]:=83;
curname[12]:=80;curname[13]:=69;curname[14]:=67;curname[15]:=73;
curname[16]:=65;curname[17]:=76;curname[18]:=72;curname[19]:=69;
curname[20]:=88;entername(90);{:56};end;procedure nameenter;begin{55:}
equiv[0]:=0;namelength:=8;curname[13]:=67;curname[14]:=72;
curname[15]:=69;curname[16]:=67;curname[17]:=75;curname[18]:=83;
curname[19]:=85;curname[20]:=77;entername(1);namelength:=10;
curname[11]:=68;curname[12]:=69;curname[13]:=83;curname[14]:=73;
curname[15]:=71;curname[16]:=78;curname[17]:=83;curname[18]:=73;
curname[19]:=90;curname[20]:=69;entername(2);namelength:=11;
curname[10]:=68;curname[11]:=69;curname[12]:=83;curname[13]:=73;
curname[14]:=71;curname[15]:=78;curname[16]:=85;curname[17]:=78;
curname[18]:=73;curname[19]:=84;curname[20]:=83;entername(3);
namelength:=12;curname[9]:=67;curname[10]:=79;curname[11]:=68;
curname[12]:=73;curname[13]:=78;curname[14]:=71;curname[15]:=83;
curname[16]:=67;curname[17]:=72;curname[18]:=69;curname[19]:=77;
curname[20]:=69;entername(4);namelength:=6;curname[15]:=70;
curname[16]:=65;curname[17]:=77;curname[18]:=73;curname[19]:=76;
curname[20]:=89;entername(5);namelength:=4;curname[17]:=70;
curname[18]:=65;curname[19]:=67;curname[20]:=69;entername(6);
namelength:=16;curname[5]:=83;curname[6]:=69;curname[7]:=86;
curname[8]:=69;curname[9]:=78;curname[10]:=66;curname[11]:=73;
curname[12]:=84;curname[13]:=83;curname[14]:=65;curname[15]:=70;
curname[16]:=69;curname[17]:=70;curname[18]:=76;curname[19]:=65;
curname[20]:=71;entername(7);namelength:=6;curname[15]:=72;
curname[16]:=69;curname[17]:=65;curname[18]:=68;curname[19]:=69;
curname[20]:=82;entername(8);namelength:=9;curname[12]:=70;
curname[13]:=79;curname[14]:=78;curname[15]:=84;curname[16]:=68;
curname[17]:=73;curname[18]:=77;curname[19]:=69;curname[20]:=78;
entername(9);namelength:=8;curname[13]:=76;curname[14]:=73;
curname[15]:=71;curname[16]:=84;curname[17]:=65;curname[18]:=66;
curname[19]:=76;curname[20]:=69;entername(10);namelength:=12;
curname[9]:=66;curname[10]:=79;curname[11]:=85;curname[12]:=78;
curname[13]:=68;curname[14]:=65;curname[15]:=82;curname[16]:=89;
curname[17]:=67;curname[18]:=72;curname[19]:=65;curname[20]:=82;
entername(11);namelength:=9;curname[12]:=67;curname[13]:=72;
curname[14]:=65;curname[15]:=82;curname[16]:=65;curname[17]:=67;
curname[18]:=84;curname[19]:=69;curname[20]:=82;entername(14);
namelength:=9;curname[12]:=80;curname[13]:=65;curname[14]:=82;
curname[15]:=65;curname[16]:=77;curname[17]:=69;curname[18]:=84;
curname[19]:=69;curname[20]:=82;entername(30);namelength:=6;
curname[15]:=67;curname[16]:=72;curname[17]:=65;curname[18]:=82;
curname[19]:=87;curname[20]:=68;entername(61);namelength:=6;
curname[15]:=67;curname[16]:=72;curname[17]:=65;curname[18]:=82;
curname[19]:=72;curname[20]:=84;entername(62);namelength:=6;
curname[15]:=67;curname[16]:=72;curname[17]:=65;curname[18]:=82;
curname[19]:=68;curname[20]:=80;entername(63);namelength:=6;
curname[15]:=67;curname[16]:=72;curname[17]:=65;curname[18]:=82;
curname[19]:=73;curname[20]:=67;entername(64);namelength:=10;
curname[11]:=78;curname[12]:=69;curname[13]:=88;curname[14]:=84;
curname[15]:=76;curname[16]:=65;curname[17]:=82;curname[18]:=71;
curname[19]:=69;curname[20]:=82;entername(65);namelength:=7;
curname[14]:=86;curname[15]:=65;curname[16]:=82;curname[17]:=67;
curname[18]:=72;curname[19]:=65;curname[20]:=82;entername(67);
namelength:=3;curname[18]:=84;curname[19]:=79;curname[20]:=80;
entername(68);namelength:=3;curname[18]:=77;curname[19]:=73;
curname[20]:=68;entername(69);namelength:=3;curname[18]:=66;
curname[19]:=79;curname[20]:=84;entername(70);namelength:=3;
curname[18]:=82;curname[19]:=69;curname[20]:=80;entername(71);
namelength:=3;curname[18]:=69;curname[19]:=88;curname[20]:=84;
entername(71);namelength:=7;curname[14]:=67;curname[15]:=79;
curname[16]:=77;curname[17]:=77;curname[18]:=69;curname[19]:=78;
curname[20]:=84;entername(0);namelength:=5;curname[16]:=76;
curname[17]:=65;curname[18]:=66;curname[19]:=69;curname[20]:=76;
entername(100);namelength:=4;curname[17]:=83;curname[18]:=84;
curname[19]:=79;curname[20]:=80;entername(101);namelength:=4;
curname[17]:=83;curname[18]:=75;curname[19]:=73;curname[20]:=80;
entername(102);namelength:=3;curname[18]:=75;curname[19]:=82;
curname[20]:=78;entername(103);namelength:=3;curname[18]:=76;
curname[19]:=73;curname[20]:=71;entername(104);namelength:=4;
curname[17]:=47;curname[18]:=76;curname[19]:=73;curname[20]:=71;
entername(106);namelength:=5;curname[16]:=47;curname[17]:=76;
curname[18]:=73;curname[19]:=71;curname[20]:=62;entername(110);
namelength:=4;curname[17]:=76;curname[18]:=73;curname[19]:=71;
curname[20]:=47;entername(105);namelength:=5;curname[16]:=76;
curname[17]:=73;curname[18]:=71;curname[19]:=47;curname[20]:=62;
entername(109);namelength:=5;curname[16]:=47;curname[17]:=76;
curname[18]:=73;curname[19]:=71;curname[20]:=47;entername(107);
namelength:=6;curname[15]:=47;curname[16]:=76;curname[17]:=73;
curname[18]:=71;curname[19]:=47;curname[20]:=62;entername(111);
namelength:=7;curname[14]:=47;curname[15]:=76;curname[16]:=73;
curname[17]:=71;curname[18]:=47;curname[19]:=62;curname[20]:=62;
entername(115);{:55};vplenter;paramenter;end;procedure readligkern;
var krnptr:0..maxkerns;c:byte;begin{109:}
begin while level=1 do begin while curchar=32 do getnext;
if curchar=40 then{110:}begin getname;
if curcode=0 then skiptoendofitem else if curcode<100 then begin begin
if charsonline>0 then writeln(' ');
write('This property name doesn''t belong in a LIGTABLE list');
showerrorcontext;end;skiptoendofitem;
end else begin case curcode of 100:{112:}
begin while curchar=32 do getnext;
if curchar=66 then begin charremainder[256]:=nl;
repeat getnext until(curchar=40)or(curchar=41);
end else begin begin if(curchar>41)or(curchar<40)then loc:=loc-1;end;
c:=getbyte;checktag(c);chartag[c]:=1;charremainder[c]:=nl;end;
if minnl<=nl then minnl:=nl+1;lkstepended:=false;end{:112};101:{114:}
if not lkstepended then begin if charsonline>0 then writeln(' ');
write('STOP must follow LIG or KRN');showerrorcontext;
end else begin ligkern[nl-1].b0:=128;lkstepended:=false;end{:114};
102:{115:}
if not lkstepended then begin if charsonline>0 then writeln(' ');
write('SKIP must follow LIG or KRN');showerrorcontext;
end else begin c:=getbyte;
if c>=128 then begin if charsonline>0 then writeln(' ');
write('Maximum SKIP amount is 127');showerrorcontext;
end else if nl+c>=maxligsteps then begin if charsonline>0 then writeln(
' ');write('Sorry, LIGTABLE too long for me to handle');
showerrorcontext;end else begin ligkern[nl-1].b0:=c;
if minnl<=nl+c then minnl:=nl+c+1;end;lkstepended:=false;end{:115};
103:{117:}begin ligkern[nl].b0:=0;ligkern[nl].b1:=getbyte;
kern[nk]:=getfix;krnptr:=0;
while kern[krnptr]<>kern[nk]do krnptr:=krnptr+1;
if krnptr=nk then begin if nk<maxkerns then nk:=nk+1 else begin begin if
charsonline>0 then writeln(' ');
write('Sorry, too many different kerns for me to handle');
showerrorcontext;end;krnptr:=krnptr-1;end;end;
ligkern[nl].b2:=128+(krnptr div 256);ligkern[nl].b3:=krnptr mod 256;
if nl>=maxligsteps-1 then begin if charsonline>0 then writeln(' ');
write('Sorry, LIGTABLE too long for me to handle');showerrorcontext;
end else nl:=nl+1;lkstepended:=true;end{:117};
104,105,106,107,109,110,111,115:{116:}begin ligkern[nl].b0:=0;
ligkern[nl].b2:=curcode-104;ligkern[nl].b1:=getbyte;
ligkern[nl].b3:=getbyte;
if nl>=maxligsteps-1 then begin if charsonline>0 then writeln(' ');
write('Sorry, LIGTABLE too long for me to handle');showerrorcontext;
end else nl:=nl+1;lkstepended:=true;end{:116};end;finishtheproperty;end;
end{:110}else if curchar=41 then skiptoendofitem else junkerror;end;
begin loc:=loc-1;level:=level+1;curchar:=41;end;end{:109};end;
procedure readcharinfo;var c:byte;begin{118:}begin c:=getbyte;
if verbose then{137:}begin if charsonline=8 then begin writeln(' ');
charsonline:=1;end else begin if charsonline>0 then write(' ');
charsonline:=charsonline+1;end;printoctal(c);end{:137};
while level=1 do begin while curchar=32 do getnext;
if curchar=40 then{119:}begin getname;
if curcode=0 then skiptoendofitem else if(curcode<61)or(curcode>67)then
begin begin if charsonline>0 then writeln(' ');
write('This property name doesn''t belong in a CHARACTER list');
showerrorcontext;end;skiptoendofitem;
end else begin case curcode of 61:charwd[c]:=sortin(1,getfix);
62:charht[c]:=sortin(2,getfix);63:chardp[c]:=sortin(3,getfix);
64:charic[c]:=sortin(4,getfix);65:begin checktag(c);chartag[c]:=2;
charremainder[c]:=getbyte;end;66:readpacket(c);67:{120:}
begin if ne=256 then begin if charsonline>0 then writeln(' ');
write('At most 256 VARCHAR specs are allowed');showerrorcontext;
end else begin checktag(c);chartag[c]:=3;charremainder[c]:=ne;
exten[ne]:=zerobytes;while level=2 do begin while curchar=32 do getnext;
if curchar=40 then{121:}begin getname;
if curcode=0 then skiptoendofitem else if(curcode<68)or(curcode>71)then
begin begin if charsonline>0 then writeln(' ');
write('This property name doesn''t belong in a VARCHAR list');
showerrorcontext;end;skiptoendofitem;
end else begin case curcode-(68)of 0:exten[ne].b0:=getbyte;
1:exten[ne].b1:=getbyte;2:exten[ne].b2:=getbyte;3:exten[ne].b3:=getbyte;
end;finishtheproperty;end;end{:121}
else if curchar=41 then skiptoendofitem else junkerror;end;ne:=ne+1;
begin loc:=loc-1;level:=level+1;curchar:=41;end;end;end{:120};end;
finishtheproperty;end;end{:119}
else if curchar=41 then skiptoendofitem else junkerror;end;
if charwd[c]=0 then charwd[c]:=sortin(1,0);begin loc:=loc-1;
level:=level+1;curchar:=41;end;end{:118};end;procedure readinput;
var c:byte;begin{92:}curchar:=32;repeat while curchar=32 do getnext;
if curchar=40 then{94:}begin getname;
if curcode=0 then skiptoendofitem else if curcode>14 then begin begin if
charsonline>0 then writeln(' ');
write('This property name doesn''t belong on the outer level');
showerrorcontext;end;skiptoendofitem;end else begin{95:}
case curcode of 1:begin checksumspecified:=true;readfourbytes(0);end;
2:{98:}begin nextd:=getfix;
if nextd<1048576 then begin if charsonline>0 then writeln(' ');
write('The design size must be at least 1');showerrorcontext;
end else designsize:=nextd;end{:98};3:{99:}begin nextd:=getfix;
if nextd<=0 then begin if charsonline>0 then writeln(' ');
write('The number of units per design size must be positive');
showerrorcontext;
end else if frozendu then begin if charsonline>0 then writeln(' ');
write('Sorry, it''s too late to change the design units');
showerrorcontext;end else designunits:=nextd;end{:99};4:readBCPL(8,40);
5:readBCPL(48,20);6:headerbytes[71]:=getbyte;7:{100:}
begin while curchar=32 do getnext;
if curchar=84 then sevenbitsafeflag:=true else if curchar=70 then
sevenbitsafeflag:=false else begin if charsonline>0 then writeln(' ');
write('The flag value should be "TRUE" or "FALSE"');showerrorcontext;
end;repeat getnext until(curchar=40)or(curchar=41);end{:100};8:{101:}
begin c:=getbyte;
if c<18 then begin begin if charsonline>0 then writeln(' ');
write('HEADER indices should be 18 or more');showerrorcontext;end;
repeat getnext until(curchar=40)or(curchar=41);
end else if 4*c+4>maxheaderbytes then begin begin if charsonline>0 then
writeln(' ');
write('This HEADER index is too big for my present table size');
showerrorcontext;end;repeat getnext until(curchar=40)or(curchar=41);
end else begin while headerptr<4*c+4 do begin headerbytes[headerptr]:=0;
headerptr:=headerptr+1;end;readfourbytes(4*c);end;end{:101};9:{102:}
begin while level=1 do begin while curchar=32 do getnext;
if curchar=40 then{103:}begin getname;
if curcode=0 then skiptoendofitem else if(curcode<30)or(curcode>=61)then
begin begin if charsonline>0 then writeln(' ');
write('This property name doesn''t belong in a FONTDIMEN list');
showerrorcontext;end;skiptoendofitem;
end else begin if curcode=30 then c:=getbyte else c:=curcode-30;
if c=0 then begin begin if charsonline>0 then writeln(' ');
write('PARAMETER index must not be zero');showerrorcontext;end;
skiptoendofitem;
end else if c>maxparamwords then begin begin if charsonline>0 then
writeln(' ');
write('This PARAMETER index is too big for my present table size');
showerrorcontext;end;skiptoendofitem;
end else begin while np<c do begin np:=np+1;param[np]:=0;end;
param[c]:=getfix;finishtheproperty;end;end;end{:103}
else if curchar=41 then skiptoendofitem else junkerror;end;
begin loc:=loc-1;level:=level+1;curchar:=41;end;end{:102};
10:readligkern;11:bchar:=getbyte;12:begin vtitlestart:=vfptr;
copytoendofitem;
if vfptr>vtitlestart+255 then begin begin if charsonline>0 then writeln(
' ');write('VTITLE clipped to 255 characters');showerrorcontext;end;
vtitlelength:=255;end else vtitlelength:=vfptr-vtitlestart;end;13:{104:}
begin getfourbytes;fontnumber[fontptr]:=curbytes;curfont:=0;
while(fontnumber[curfont].b3<>fontnumber[fontptr].b3)or(fontnumber[
curfont].b2<>fontnumber[fontptr].b2)or(fontnumber[curfont].b1<>
fontnumber[fontptr].b1)or(fontnumber[curfont].b0<>fontnumber[fontptr].b0
)do curfont:=curfont+1;if curfont=fontptr then if fontptr<256 then{105:}
begin fontptr:=fontptr+1;fnamestart[curfont]:=vfsize;
fnamelength[curfont]:=4;fareastart[curfont]:=vfsize;
farealength[curfont]:=0;fontchecksum[curfont]:=zerobytes;
fontat[curfont]:=1048576;fontdsize[curfont]:=10485760;end{:105}
else begin if charsonline>0 then writeln(' ');
write('I can handle only 256 different mapfonts');showerrorcontext;end;
if curfont=fontptr then skiptoendofitem else while level=1 do begin
while curchar=32 do getnext;if curchar=40 then{106:}begin getname;
if curcode=0 then skiptoendofitem else if(curcode<20)or(curcode>24)then
begin begin if charsonline>0 then writeln(' ');
write('This property name doesn''t belong in a MAPFONT list');
showerrorcontext;end;skiptoendofitem;
end else begin case curcode of 20:{107:}
begin fnamestart[curfont]:=vfptr;copytoendofitem;
if vfptr>fnamestart[curfont]+255 then begin begin if charsonline>0 then
writeln(' ');write('FONTNAME clipped to 255 characters');
showerrorcontext;end;fnamelength[curfont]:=255;
end else fnamelength[curfont]:=vfptr-fnamestart[curfont];end{:107};
21:{108:}begin fareastart[curfont]:=vfptr;copytoendofitem;
if vfptr>fareastart[curfont]+255 then begin begin if charsonline>0 then
writeln(' ');write('FONTAREA clipped to 255 characters');
showerrorcontext;end;farealength[curfont]:=255;
end else farealength[curfont]:=vfptr-fareastart[curfont];end{:108};
22:begin getfourbytes;fontchecksum[curfont]:=curbytes;end;
23:begin frozendu:=true;
if designunits=1048576 then fontat[curfont]:=getfix else fontat[curfont]
:=round((getfix/designunits)*1048576.0);end;
24:fontdsize[curfont]:=getfix;end;finishtheproperty;end;end{:106}
else if curchar=41 then skiptoendofitem else junkerror;end;
begin loc:=loc-1;level:=level+1;curchar:=41;end;end{:104};
14:readcharinfo;end{:95};finishtheproperty;end;end{:94}
else if(curchar=41)and not inputhasended then begin begin if charsonline
>0 then writeln(' ');write('Extra right parenthesis');showerrorcontext;
end;loc:=loc+1;curchar:=32;end else if not inputhasended then junkerror;
until inputhasended{:92};end;procedure corrandcheck;var c:0..256;
hh:0..hashsize;ligptr:0..maxligsteps;g:byte;begin{139:}
if nl>0 then{145:}
begin if charremainder[256]<32767 then begin ligkern[nl].b0:=255;
ligkern[nl].b1:=0;ligkern[nl].b2:=0;ligkern[nl].b3:=0;nl:=nl+1;end;
while minnl>nl do begin ligkern[nl].b0:=255;ligkern[nl].b1:=0;
ligkern[nl].b2:=0;ligkern[nl].b3:=0;nl:=nl+1;end;
if ligkern[nl-1].b0=0 then ligkern[nl-1].b0:=128;end{:145};
sevenunsafe:=false;for c:=0 to 255 do if charwd[c]<>0 then{140:}
case chartag[c]of 0:;1:{149:}begin ligptr:=charremainder[c];
repeat if hashinput(ligptr,c)then begin if ligkern[ligptr].b2<128 then
begin if ligkern[ligptr].b1<>bchar then begin g:=ligkern[ligptr].b1;
if charwd[g]=0 then begin charwd[g]:=sortin(1,0);
write('LIG character examined by',' ');printoctal(c);
writeln(' had no CHARACTER spec.');end;end;begin g:=ligkern[ligptr].b3;
if charwd[g]=0 then begin charwd[g]:=sortin(1,0);
write('LIG character generated by',' ');printoctal(c);
writeln(' had no CHARACTER spec.');end;end;
if ligkern[ligptr].b3>=128 then if(c<128)or(c=256)then if(ligkern[ligptr
].b1<128)or(ligkern[ligptr].b1=bchar)then sevenunsafe:=true;
end else if ligkern[ligptr].b1<>bchar then begin g:=ligkern[ligptr].b1;
if charwd[g]=0 then begin charwd[g]:=sortin(1,0);
write('KRN character examined by',' ');printoctal(c);
writeln(' had no CHARACTER spec.');end;end;end;
if ligkern[ligptr].b0>=128 then ligptr:=nl else ligptr:=ligptr+1+ligkern
[ligptr].b0;until ligptr>=nl;end{:149};2:begin g:=charremainder[c];
if(g>=128)and(c<128)then sevenunsafe:=true;
if charwd[g]=0 then begin charwd[g]:=sortin(1,0);
write('The character NEXTLARGER than',' ');printoctal(c);
writeln(' had no CHARACTER spec.');end;end;3:{141:}
begin if exten[charremainder[c]].b0>0 then begin g:=exten[charremainder[
c]].b0;if(g>=128)and(c<128)then sevenunsafe:=true;
if charwd[g]=0 then begin charwd[g]:=sortin(1,0);
write('TOP piece of character',' ');printoctal(c);
writeln(' had no CHARACTER spec.');end;end;
if exten[charremainder[c]].b1>0 then begin g:=exten[charremainder[c]].b1
;if(g>=128)and(c<128)then sevenunsafe:=true;
if charwd[g]=0 then begin charwd[g]:=sortin(1,0);
write('MID piece of character',' ');printoctal(c);
writeln(' had no CHARACTER spec.');end;end;
if exten[charremainder[c]].b2>0 then begin g:=exten[charremainder[c]].b2
;if(g>=128)and(c<128)then sevenunsafe:=true;
if charwd[g]=0 then begin charwd[g]:=sortin(1,0);
write('BOT piece of character',' ');printoctal(c);
writeln(' had no CHARACTER spec.');end;end;
begin g:=exten[charremainder[c]].b3;
if(g>=128)and(c<128)then sevenunsafe:=true;
if charwd[g]=0 then begin charwd[g]:=sortin(1,0);
write('REP piece of character',' ');printoctal(c);
writeln(' had no CHARACTER spec.');end;end;end{:141};end{:140};
if charremainder[256]<32767 then begin c:=256;{149:}
begin ligptr:=charremainder[c];
repeat if hashinput(ligptr,c)then begin if ligkern[ligptr].b2<128 then
begin if ligkern[ligptr].b1<>bchar then begin g:=ligkern[ligptr].b1;
if charwd[g]=0 then begin charwd[g]:=sortin(1,0);
write('LIG character examined by',' ');printoctal(c);
writeln(' had no CHARACTER spec.');end;end;begin g:=ligkern[ligptr].b3;
if charwd[g]=0 then begin charwd[g]:=sortin(1,0);
write('LIG character generated by',' ');printoctal(c);
writeln(' had no CHARACTER spec.');end;end;
if ligkern[ligptr].b3>=128 then if(c<128)or(c=256)then if(ligkern[ligptr
].b1<128)or(ligkern[ligptr].b1=bchar)then sevenunsafe:=true;
end else if ligkern[ligptr].b1<>bchar then begin g:=ligkern[ligptr].b1;
if charwd[g]=0 then begin charwd[g]:=sortin(1,0);
write('KRN character examined by',' ');printoctal(c);
writeln(' had no CHARACTER spec.');end;end;end;
if ligkern[ligptr].b0>=128 then ligptr:=nl else ligptr:=ligptr+1+ligkern
[ligptr].b0;until ligptr>=nl;end{:149};end;
if sevenbitsafeflag and sevenunsafe then writeln(
'The font is not really seven-bit-safe!');{154:}
if hashptr<hashsize then for hh:=1 to hashptr do begin tt:=hashlist[hh];
if class[tt]>0 then tt:=f(tt,(hash[tt]-1)div 256,(hash[tt]-1)mod 256);
end;
if(hashptr=hashsize)or(yligcycle<256)then begin if hashptr<hashsize then
begin write('Infinite ligature loop starting with ');
if xligcycle=256 then write('boundary')else printoctal(xligcycle);
write(' and ');printoctal(yligcycle);writeln('!');
end else writeln(
'Sorry, I haven''t room for so many ligature/kern pairs!');
writeln('All ligatures will be cleared.');
for c:=0 to 255 do if chartag[c]=1 then begin chartag[c]:=0;
charremainder[c]:=0;end;nl:=0;bchar:=256;charremainder[256]:=32767;
end{:154};{155:}
if nl>0 then for ligptr:=0 to nl-1 do if ligkern[ligptr].b2<128 then
begin if ligkern[ligptr].b0<255 then begin begin c:=ligkern[ligptr].b1;
if charwd[c]=0 then if c<>bchar then begin ligkern[ligptr].b1:=0;
if charwd[0]=0 then charwd[0]:=sortin(1,0);
write('Unused ','LIG step',' refers to nonexistent character ');
printoctal(c);writeln('!');end;end;begin c:=ligkern[ligptr].b3;
if charwd[c]=0 then if c<>bchar then begin ligkern[ligptr].b3:=0;
if charwd[0]=0 then charwd[0]:=sortin(1,0);
write('Unused ','LIG step',' refers to nonexistent character ');
printoctal(c);writeln('!');end;end;end;
end else begin c:=ligkern[ligptr].b1;
if charwd[c]=0 then if c<>bchar then begin ligkern[ligptr].b1:=0;
if charwd[0]=0 then charwd[0]:=sortin(1,0);
write('Unused ','KRN step',' refers to nonexistent character ');
printoctal(c);writeln('!');end;end;
if ne>0 then for g:=0 to ne-1 do begin begin c:=exten[g].b0;
if c>0 then if charwd[c]=0 then begin exten[g].b0:=0;
if charwd[0]=0 then charwd[0]:=sortin(1,0);
write('Unused ','VARCHAR TOP',' refers to nonexistent character ');
printoctal(c);writeln('!');end;end;begin c:=exten[g].b1;
if c>0 then if charwd[c]=0 then begin exten[g].b1:=0;
if charwd[0]=0 then charwd[0]:=sortin(1,0);
write('Unused ','VARCHAR MID',' refers to nonexistent character ');
printoctal(c);writeln('!');end;end;begin c:=exten[g].b2;
if c>0 then if charwd[c]=0 then begin exten[g].b2:=0;
if charwd[0]=0 then charwd[0]:=sortin(1,0);
write('Unused ','VARCHAR BOT',' refers to nonexistent character ');
printoctal(c);writeln('!');end;end;begin c:=exten[g].b3;
if charwd[c]=0 then begin exten[g].b3:=0;
if charwd[0]=0 then charwd[0]:=sortin(1,0);
write('Unused ','VARCHAR REP',' refers to nonexistent character ');
printoctal(c);writeln('!');end;end;end{:155};for c:=0 to 255 do{142:}
if chartag[c]=2 then begin g:=charremainder[c];
while(g<c)and(chartag[g]=2)do g:=charremainder[g];
if g=c then begin chartag[c]:=0;
write('A cycle of NEXTLARGER characters has been broken at ');
printoctal(c);writeln('.');end;end{:142};{144:}delta:=shorten(1,255);
setindices(1,delta);
if delta>0 then begin write('I had to round some ','width','s by ');
printreal((((delta+1)div 2)/1048576),1,7);writeln(' units.');end;
delta:=shorten(2,15);setindices(2,delta);
if delta>0 then begin write('I had to round some ','height','s by ');
printreal((((delta+1)div 2)/1048576),1,7);writeln(' units.');end;
delta:=shorten(3,15);setindices(3,delta);
if delta>0 then begin write('I had to round some ','depth','s by ');
printreal((((delta+1)div 2)/1048576),1,7);writeln(' units.');end;
delta:=shorten(4,63);setindices(4,delta);
if delta>0 then begin write('I had to round some ','italic correction',
's by ');printreal((((delta+1)div 2)/1048576),1,7);writeln(' units.');
end;{:144}{:139}end;procedure vfoutput;var c:byte;curfont:0..256;
k:integer;begin{177:}putbyte(247,vffile);putbyte(202,vffile);
putbyte(vtitlelength,vffile);
for k:=0 to vtitlelength-1 do putbyte(vf[vtitlestart+k],vffile);
for k:=0 to 7 do putbyte(headerbytes[k],vffile);vcount:=vtitlelength+11;
for curfont:=0 to fontptr-1 do{178:}begin putbyte(243,vffile);
putbyte(curfont,vffile);putbyte(fontchecksum[curfont].b0,vffile);
putbyte(fontchecksum[curfont].b1,vffile);
putbyte(fontchecksum[curfont].b2,vffile);
putbyte(fontchecksum[curfont].b3,vffile);voutint(fontat[curfont]);
voutint(fontdsize[curfont]);putbyte(farealength[curfont],vffile);
putbyte(fnamelength[curfont],vffile);
for k:=0 to farealength[curfont]-1 do putbyte(vf[fareastart[curfont]+k],
vffile);if fnamestart[curfont]=vfsize then begin putbyte(78,vffile);
putbyte(85,vffile);putbyte(76,vffile);putbyte(76,vffile);
end else for k:=0 to fnamelength[curfont]-1 do putbyte(vf[fnamestart[
curfont]+k],vffile);
vcount:=vcount+12+farealength[curfont]+fnamelength[curfont];end{:178};
for c:=bc to ec do if charwd[c]>0 then{179:}begin x:=memory[charwd[c]];
if designunits<>1048576 then x:=round((x/designunits)*1048576.0);
if(packetlength[c]>241)or(x<0)or(x>=16777216)then begin putbyte(242,
vffile);voutint(packetlength[c]);voutint(c);voutint(x);
vcount:=vcount+13+packetlength[c];
end else begin putbyte(packetlength[c],vffile);putbyte(c,vffile);
putbyte(x div 65536,vffile);putbyte((x div 256)mod 256,vffile);
putbyte(x mod 256,vffile);vcount:=vcount+5+packetlength[c];end;
if packetstart[c]=vfsize then begin if c>=128 then putbyte(128,vffile);
putbyte(c,vffile);
end else for k:=0 to packetlength[c]-1 do putbyte(vf[packetstart[c]+k],
vffile);end{:179};repeat putbyte(248,vffile);vcount:=vcount+1;
until vcount mod 4=0{:177};end;{:180}{181:}begin initialize;nameenter;
readinput;if verbose then writeln('.');corrandcheck;{157:}{159:}
lh:=headerptr div 4;notfound:=true;bc:=0;
while notfound do if(charwd[bc]>0)or(bc=255)then notfound:=false else bc
:=bc+1;notfound:=true;ec:=255;
while notfound do if(charwd[ec]>0)or(ec=0)then notfound:=false else ec:=
ec-1;if bc>ec then bc:=1;memory[1]:=memory[1]+1;memory[2]:=memory[2]+1;
memory[3]:=memory[3]+1;memory[4]:=memory[4]+1;{168:}{169:}labelptr:=0;
labeltable[0].rr:=-1;
for c:=bc to ec do if chartag[c]=1 then begin sortptr:=labelptr;
while labeltable[sortptr].rr>toint(charremainder[c])do begin labeltable[
sortptr+1]:=labeltable[sortptr];sortptr:=sortptr-1;end;
labeltable[sortptr+1].cc:=c;labeltable[sortptr+1].rr:=charremainder[c];
labelptr:=labelptr+1;end{:169};
if bchar<256 then begin extralocneeded:=true;lkoffset:=1;
end else begin extralocneeded:=false;lkoffset:=0;end;{170:}
begin sortptr:=labelptr;
if labeltable[sortptr].rr+lkoffset>255 then begin lkoffset:=0;
extralocneeded:=false;
repeat charremainder[labeltable[sortptr].cc]:=lkoffset;
while labeltable[sortptr-1].rr=labeltable[sortptr].rr do begin sortptr:=
sortptr-1;charremainder[labeltable[sortptr].cc]:=lkoffset;end;
lkoffset:=lkoffset+1;sortptr:=sortptr-1;
until lkoffset+labeltable[sortptr].rr<256;end;
if lkoffset>0 then while sortptr>0 do begin charremainder[labeltable[
sortptr].cc]:=charremainder[labeltable[sortptr].cc]+lkoffset;
sortptr:=sortptr-1;end;end{:170};
if charremainder[256]<32767 then begin ligkern[nl-1].b2:=(charremainder[
256]+lkoffset)div 256;
ligkern[nl-1].b3:=(charremainder[256]+lkoffset)mod 256;end{:168};
lf:=6+lh+(ec-bc+1)+memory[1]+memory[2]+memory[3]+memory[4]+nl+lkoffset+
nk+ne+np;{:159};{160:}putbyte((lf)div 256,tfmfile);
putbyte((lf)mod 256,tfmfile);putbyte((lh)div 256,tfmfile);
putbyte((lh)mod 256,tfmfile);putbyte((bc)div 256,tfmfile);
putbyte((bc)mod 256,tfmfile);putbyte((ec)div 256,tfmfile);
putbyte((ec)mod 256,tfmfile);putbyte((memory[1])div 256,tfmfile);
putbyte((memory[1])mod 256,tfmfile);putbyte((memory[2])div 256,tfmfile);
putbyte((memory[2])mod 256,tfmfile);putbyte((memory[3])div 256,tfmfile);
putbyte((memory[3])mod 256,tfmfile);putbyte((memory[4])div 256,tfmfile);
putbyte((memory[4])mod 256,tfmfile);
putbyte((nl+lkoffset)div 256,tfmfile);
putbyte((nl+lkoffset)mod 256,tfmfile);putbyte((nk)div 256,tfmfile);
putbyte((nk)mod 256,tfmfile);putbyte((ne)div 256,tfmfile);
putbyte((ne)mod 256,tfmfile);putbyte((np)div 256,tfmfile);
putbyte((np)mod 256,tfmfile);{:160};{162:}
if not checksumspecified then{163:}begin curbytes.b0:=bc;
curbytes.b1:=ec;curbytes.b2:=bc;curbytes.b3:=ec;
for c:=bc to ec do if charwd[c]>0 then begin tempwidth:=memory[charwd[c]
];if designunits<>1048576 then tempwidth:=round((tempwidth/designunits)
*1048576.0);tempwidth:=tempwidth+(c+4)*4194304;
curbytes.b0:=(curbytes.b0+curbytes.b0+tempwidth)mod 255;
curbytes.b1:=(curbytes.b1+curbytes.b1+tempwidth)mod 253;
curbytes.b2:=(curbytes.b2+curbytes.b2+tempwidth)mod 251;
curbytes.b3:=(curbytes.b3+curbytes.b3+tempwidth)mod 247;end;
headerbytes[0]:=curbytes.b0;headerbytes[1]:=curbytes.b1;
headerbytes[2]:=curbytes.b2;headerbytes[3]:=curbytes.b3;end{:163};
headerbytes[4]:=designsize div 16777216;
headerbytes[5]:=(designsize div 65536)mod 256;
headerbytes[6]:=(designsize div 256)mod 256;
headerbytes[7]:=designsize mod 256;
if not sevenunsafe then headerbytes[68]:=128;
for j:=0 to headerptr-1 do putbyte(headerbytes[j],tfmfile);{:162};{164:}
index[0]:=0;for c:=bc to ec do begin putbyte(index[charwd[c]],tfmfile);
putbyte(index[charht[c]]*16+index[chardp[c]],tfmfile);
putbyte(index[charic[c]]*4+chartag[c],tfmfile);
putbyte(charremainder[c],tfmfile);end{:164};{166:}
for q:=1 to 4 do begin putbyte(0,tfmfile);putbyte(0,tfmfile);
putbyte(0,tfmfile);putbyte(0,tfmfile);p:=link[q];
while p>0 do begin outscaled(memory[p]);p:=link[p];end;end;{:166};{171:}
if extralocneeded then begin putbyte(255,tfmfile);
putbyte(bchar,tfmfile);putbyte(0,tfmfile);putbyte(0,tfmfile);
end else for sortptr:=1 to lkoffset do begin t:=labeltable[labelptr].rr;
if bchar<256 then begin putbyte(255,tfmfile);putbyte(bchar,tfmfile);
end else begin putbyte(254,tfmfile);putbyte(0,tfmfile);end;
putbyte((t+lkoffset)div 256,tfmfile);
putbyte((t+lkoffset)mod 256,tfmfile);repeat labelptr:=labelptr-1;
until labeltable[labelptr].rr<t;end;
if nl>0 then for ligptr:=0 to nl-1 do begin putbyte(ligkern[ligptr].b0,
tfmfile);putbyte(ligkern[ligptr].b1,tfmfile);
putbyte(ligkern[ligptr].b2,tfmfile);putbyte(ligkern[ligptr].b3,tfmfile);
end;if nk>0 then for krnptr:=0 to nk-1 do outscaled(kern[krnptr]){:171};
{172:}
if ne>0 then for c:=0 to ne-1 do begin putbyte(exten[c].b0,tfmfile);
putbyte(exten[c].b1,tfmfile);putbyte(exten[c].b2,tfmfile);
putbyte(exten[c].b3,tfmfile);end;{:172};{173:}
for parptr:=1 to np do begin if parptr=1 then{174:}
begin if param[1]<0 then begin param[1]:=param[1]+1073741824;
putbyte((param[1]div 16777216)+192,tfmfile);
end else putbyte(param[1]div 16777216,tfmfile);
putbyte((param[1]div 65536)mod 256,tfmfile);
putbyte((param[1]div 256)mod 256,tfmfile);
putbyte(param[1]mod 256,tfmfile);end{:174}else outscaled(param[parptr]);
end{:173}{:157};vfoutput;end.{:181}
