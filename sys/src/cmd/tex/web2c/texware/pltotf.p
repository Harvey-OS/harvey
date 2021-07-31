{2:}program PLtoTF;const{3:}bufsize=60;maxheaderbytes=100;
maxparamwords=30;maxligsteps=32000;maxkerns=15000;hashsize=15077;{:3}
{154:}noptions=1;{:154}type{17:}byte=0..255;ASCIIcode=32..127;{:17}{57:}
fourbytes=record b0:byte;b1:byte;b2:byte;b3:byte;end;{:57}{61:}
fixword=integer;{:61}{68:}headerindex=0..maxheaderbytes;indx=0..32767;
{:68}{71:}pointer=0..1032;{:71}var{5:}plfile:text;{:5}{15:}
tfmfile:packed file of 0..255;
tfmname,plname:packed array[1..PATHMAX]of char;{:15}{18:}
xord:array[char]of ASCIIcode;{:18}{21:}line:integer;goodindent:integer;
indent:integer;level:integer;{:21}{23:}leftln,rightln:boolean;
limit:0..bufsize;loc:0..bufsize;buffer:array[1..bufsize]of char;
inputhasended:boolean;{:23}{25:}charsonline:0..8;{:25}{30:}
curchar:ASCIIcode;{:30}{36:}start:array[1..88]of 0..600;
dictionary:array[0..600]of ASCIIcode;startptr:0..88;dictptr:0..600;{:36}
{38:}curname:array[1..20]of ASCIIcode;namelength:0..20;nameptr:0..88;
{:38}{39:}nhash:array[0..100]of 0..88;curhash:0..100;{:39}{44:}
equiv:array[0..88]of byte;curcode:byte;{:44}{58:}curbytes:fourbytes;
{:58}{65:}fractiondigits:array[1..7]of integer;{:65}{67:}
headerbytes:array[headerindex]of byte;headerptr:headerindex;
designsize:fixword;designunits:fixword;sevenbitsafeflag:boolean;
ligkern:array[0..maxligsteps]of fourbytes;nl:0..32767;minnl:0..32767;
kern:array[0..maxkerns]of fixword;nk:0..maxkerns;
exten:array[0..255]of fourbytes;ne:0..256;
param:array[1..maxparamwords]of fixword;np:0..maxparamwords;
checksumspecified:boolean;bchar:0..256;{:67}{72:}
memory:array[pointer]of fixword;memptr:pointer;
link:array[pointer]of pointer;charwd:array[byte]of pointer;
charht:array[byte]of pointer;chardp:array[byte]of pointer;
charic:array[byte]of pointer;chartag:array[byte]of 0..3;
charremainder:array[0..256]of 0..65535;{:72}{76:}nextd:fixword;{:76}
{79:}index:array[pointer]of byte;excess:byte;{:79}{81:}c:byte;{:81}{98:}
lkstepended:boolean;krnptr:0..maxkerns;{:98}{109:}sevenunsafe:boolean;
{:109}{114:}delta:fixword;{:114}{118:}ligptr:0..maxligsteps;
hash:array[0..hashsize]of 0..66048;class:array[0..hashsize]of 0..4;
ligz:array[0..hashsize]of 0..257;hashptr:0..hashsize;
hashlist:array[0..hashsize]of 0..hashsize;h,hh:0..hashsize;tt:indx;
xligcycle,yligcycle:0..256;{:118}{129:}bc:byte;ec:byte;lh:byte;
lf:0..32767;notfound:boolean;tempwidth:fixword;{:129}{132:}
j:0..maxheaderbytes;p:pointer;q:1..4;parptr:0..maxparamwords;{:132}
{138:}labeltable:array[0..256]of record rr:-1..32767;cc:byte;end;
labelptr:0..256;sortptr:0..256;lkoffset:0..256;t:0..32767;
extralocneeded:boolean;{:138}{151:}verbose:integer;{:151}
procedure initialize;var{19:}k:integer;{:19}{40:}h:0..100;{:40}{69:}
d:headerindex;{:69}{73:}c:byte;{:73}{149:}
longoptions:array[0..noptions]of getoptstruct;getoptreturnval:integer;
optionindex:integer;{:149}
begin if(argc<3)or(argc>noptions+3)then begin writeln(
'Usage: pltotf [-verbose] <property list file> <tfm file>.');uexit(1);
end;{152:}verbose:=false;{:152};{148:}begin{150:}
longoptions[0].name:='verbose';longoptions[0].hasarg:=0;
longoptions[0].flag:=addressofint(verbose);longoptions[0].val:=1;{:150}
{153:}longoptions[1].name:=0;longoptions[1].hasarg:=0;
longoptions[1].flag:=0;longoptions[1].val:=0;{:153};
repeat getoptreturnval:=getoptlongonly(argc,gargv,'',longoptions,
addressofint(optionindex));
if getoptreturnval<>-1 then begin if getoptreturnval=63 then uexit(1);
end;until getoptreturnval=-1;end{:148};{6:}argv(optind,plname);
reset(plfile,plname);
if verbose then writeln('This is PLtoTF, C Version 3.4');{:6}{16:}
argv(optind+1,tfmname);rewrite(tfmfile,tfmname);{:16}{20:}
for k:=0 to 127 do xord[chr(k)]:=127;xord[' ']:=32;xord['!']:=33;
xord['"']:=34;xord['#']:=35;xord['$']:=36;xord['%']:=37;xord['&']:=38;
xord['''']:=39;xord['(']:=40;xord[')']:=41;xord['*']:=42;xord['+']:=43;
xord[',']:=44;xord['-']:=45;xord['.']:=46;xord['/']:=47;xord['0']:=48;
xord['1']:=49;xord['2']:=50;xord['3']:=51;xord['4']:=52;xord['5']:=53;
xord['6']:=54;xord['7']:=55;xord['8']:=56;xord['9']:=57;xord[':']:=58;
xord[';']:=59;xord['<']:=60;xord['=']:=61;xord['>']:=62;xord['?']:=63;
xord['@']:=64;xord['A']:=65;xord['B']:=66;xord['C']:=67;xord['D']:=68;
xord['E']:=69;xord['F']:=70;xord['G']:=71;xord['H']:=72;xord['I']:=73;
xord['J']:=74;xord['K']:=75;xord['L']:=76;xord['M']:=77;xord['N']:=78;
xord['O']:=79;xord['P']:=80;xord['Q']:=81;xord['R']:=82;xord['S']:=83;
xord['T']:=84;xord['U']:=85;xord['V']:=86;xord['W']:=87;xord['X']:=88;
xord['Y']:=89;xord['Z']:=90;xord['[']:=91;xord['\']:=92;xord[']']:=93;
xord['^']:=94;xord['_']:=95;xord['`']:=96;xord['a']:=97;xord['b']:=98;
xord['c']:=99;xord['d']:=100;xord['e']:=101;xord['f']:=102;
xord['g']:=103;xord['h']:=104;xord['i']:=105;xord['j']:=106;
xord['k']:=107;xord['l']:=108;xord['m']:=109;xord['n']:=110;
xord['o']:=111;xord['p']:=112;xord['q']:=113;xord['r']:=114;
xord['s']:=115;xord['t']:=116;xord['u']:=117;xord['v']:=118;
xord['w']:=119;xord['x']:=120;xord['y']:=121;xord['z']:=122;
xord['{']:=123;xord['|']:=124;xord['}']:=125;xord['~']:=126;{:20}{22:}
line:=0;goodindent:=0;indent:=0;level:=0;{:22}{24:}limit:=0;loc:=0;
leftln:=true;rightln:=true;inputhasended:=false;{:24}{26:}
charsonline:=0;{:26}{37:}startptr:=1;start[1]:=0;dictptr:=0;{:37}{41:}
for h:=0 to 100 do nhash[h]:=0;{:41}{70:}
for d:=0 to 18*4-1 do headerbytes[d]:=0;headerbytes[8]:=11;
headerbytes[9]:=85;headerbytes[10]:=78;headerbytes[11]:=83;
headerbytes[12]:=80;headerbytes[13]:=69;headerbytes[14]:=67;
headerbytes[15]:=73;headerbytes[16]:=70;headerbytes[17]:=73;
headerbytes[18]:=69;headerbytes[19]:=68;
for d:=48 to 59 do headerbytes[d]:=headerbytes[d-40];
designsize:=10*1048576;designunits:=1048576;sevenbitsafeflag:=false;
headerptr:=18*4;nl:=0;minnl:=0;nk:=0;ne:=0;np:=0;
checksumspecified:=false;bchar:=256;{:70}{74:}charremainder[256]:=32767;
for c:=0 to 255 do begin charwd[c]:=0;charht[c]:=0;chardp[c]:=0;
charic[c]:=0;chartag[c]:=0;charremainder[c]:=0;end;
memory[0]:=2147483647;memory[1]:=0;link[1]:=0;memory[2]:=0;link[2]:=0;
memory[3]:=0;link[3]:=0;memory[4]:=0;link[4]:=0;memptr:=4;{:74}{119:}
hashptr:=0;yligcycle:=256;for k:=0 to hashsize do hash[k]:=0;{:119}end;
{:2}{27:}procedure showerrorcontext;var k:0..bufsize;
begin writeln(' (line ',line:1,').');if not leftln then write('...');
for k:=1 to loc do write(buffer[k]);writeln(' ');
if not leftln then write('   ');for k:=1 to loc do write(' ');
for k:=loc+1 to limit do write(buffer[k]);
if rightln then writeln(' ')else writeln('...');charsonline:=0;end;{:27}
{28:}procedure fillbuffer;begin leftln:=rightln;limit:=0;loc:=0;
if leftln then begin if line>0 then readln(plfile);line:=line+1;end;
if eof(plfile)then begin limit:=1;buffer[1]:=')';rightln:=false;
inputhasended:=true;
end else begin while(limit<bufsize-1)and(not eoln(plfile))do begin limit
:=limit+1;read(plfile,buffer[limit]);end;buffer[limit+1]:=' ';
rightln:=eoln(plfile);if leftln then{29:}
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
goodindent:=0;indent:=0;end;end;end{:29};end;end;{:28}{31:}
procedure getkeywordchar;
begin while(loc=limit)and(not rightln)do fillbuffer;
if loc=limit then curchar:=32 else begin curchar:=xord[buffer[loc+1]];
if curchar>=97 then curchar:=curchar-32;
if((curchar>=48)and(curchar<=57))then loc:=loc+1 else if((curchar>=65)
and(curchar<=90))then loc:=loc+1 else if curchar=47 then loc:=loc+1 else
if curchar=62 then loc:=loc+1 else curchar:=32;end;end;{:31}{32:}
procedure getnext;begin while loc=limit do fillbuffer;loc:=loc+1;
curchar:=xord[buffer[loc]];
if curchar>=97 then if curchar<=122 then curchar:=curchar-32 else begin
if curchar=127 then begin begin if charsonline>0 then writeln(' ');
write('Illegal character in the file');showerrorcontext;end;curchar:=63;
end;end else if(curchar<=41)and(curchar>=40)then loc:=loc-1;end;{:32}
{33:}procedure skiptoendofitem;var l:integer;begin l:=level;
while level>=l do begin while loc=limit do fillbuffer;loc:=loc+1;
if buffer[loc]=')'then level:=level-1 else if buffer[loc]='('then level
:=level+1;end;
if inputhasended then begin if charsonline>0 then writeln(' ');
write('File ended unexpectedly: No closing ")"');showerrorcontext;end;
curchar:=32;end;{:33}{35:}procedure finishtheproperty;
begin while curchar=32 do getnext;
if curchar<>41 then begin if charsonline>0 then writeln(' ');
write('Junk after property value will be ignored');showerrorcontext;end;
skiptoendofitem;end;{:35}{42:}procedure lookup;var k:0..20;j:0..600;
notfound:boolean;begin{43:}curhash:=curname[1];
for k:=2 to namelength do curhash:=(curhash+curhash+curname[k])mod 101{:
43};notfound:=true;
while notfound do begin if curhash=0 then curhash:=100 else curhash:=
curhash-1;
if nhash[curhash]=0 then notfound:=false else begin j:=start[nhash[
curhash]];
if start[nhash[curhash]+1]=j+namelength then begin notfound:=false;
for k:=1 to namelength do if dictionary[j+k-1]<>curname[k]then notfound
:=true;end;end;end;nameptr:=nhash[curhash];end;{:42}{45:}
procedure entername(v:byte);var k:0..20;
begin for k:=1 to namelength do curname[k]:=curname[k+20-namelength];
lookup;nhash[curhash]:=startptr;equiv[startptr]:=v;
for k:=1 to namelength do begin dictionary[dictptr]:=curname[k];
dictptr:=dictptr+1;end;startptr:=startptr+1;start[startptr]:=dictptr;
end;{:45}{49:}procedure getname;begin loc:=loc+1;level:=level+1;
curchar:=32;while curchar=32 do getnext;
if(curchar>41)or(curchar<40)then loc:=loc-1;namelength:=0;
getkeywordchar;
while curchar<>32 do begin if namelength=20 then curname[1]:=88 else
namelength:=namelength+1;curname[namelength]:=curchar;getkeywordchar;
end;lookup;if nameptr=0 then begin if charsonline>0 then writeln(' ');
write('Sorry, I don''t know that property name');showerrorcontext;end;
curcode:=equiv[nameptr];end;{:49}{51:}function getbyte:byte;
var acc:integer;t:ASCIIcode;begin repeat getnext;until curchar<>32;
t:=curchar;acc:=0;repeat getnext;until curchar<>32;if t=67 then{52:}
if(curchar>=33)and(curchar<=126)and((curchar<40)or(curchar>41))then acc
:=xord[buffer[loc]]else begin begin if charsonline>0 then writeln(' ');
write('"C" value must be standard ASCII and not a paren');
showerrorcontext;end;repeat getnext until(curchar=40)or(curchar=41);
end{:52}else if t=68 then{53:}
begin while(curchar>=48)and(curchar<=57)do begin acc:=acc*10+curchar-48;
if acc>255 then begin begin begin if charsonline>0 then writeln(' ');
write('This value shouldn''t exceed 255');showerrorcontext;end;
repeat getnext until(curchar=40)or(curchar=41);end;acc:=0;curchar:=32;
end else getnext;end;begin if(curchar>41)or(curchar<40)then loc:=loc-1;
end;end{:53}else if t=79 then{54:}
begin while(curchar>=48)and(curchar<=55)do begin acc:=acc*8+curchar-48;
if acc>255 then begin begin begin if charsonline>0 then writeln(' ');
write('This value shouldn''t exceed ''377');showerrorcontext;end;
repeat getnext until(curchar=40)or(curchar=41);end;acc:=0;curchar:=32;
end else getnext;end;begin if(curchar>41)or(curchar<40)then loc:=loc-1;
end;end{:54}else if t=72 then{55:}
begin while((curchar>=48)and(curchar<=57))or((curchar>=65)and(curchar<=
70))do begin if curchar>=65 then curchar:=curchar-7;
acc:=acc*16+curchar-48;
if acc>255 then begin begin begin if charsonline>0 then writeln(' ');
write('This value shouldn''t exceed "FF');showerrorcontext;end;
repeat getnext until(curchar=40)or(curchar=41);end;acc:=0;curchar:=32;
end else getnext;end;begin if(curchar>41)or(curchar<40)then loc:=loc-1;
end;end{:55}else if t=70 then{56:}
begin if curchar=66 then acc:=2 else if curchar=76 then acc:=4 else if
curchar<>77 then acc:=18;getnext;
if curchar=73 then acc:=acc+1 else if curchar<>82 then acc:=18;getnext;
if curchar=67 then acc:=acc+6 else if curchar=69 then acc:=acc+12 else
if curchar<>82 then acc:=18;
if acc>=18 then begin begin begin if charsonline>0 then writeln(' ');
write('Illegal face code, I changed it to MRR');showerrorcontext;end;
repeat getnext until(curchar=40)or(curchar=41);end;acc:=0;end;end{:56}
else begin begin if charsonline>0 then writeln(' ');
write('You need "C" or "D" or "O" or "H" or "F" here');showerrorcontext;
end;repeat getnext until(curchar=40)or(curchar=41);end;curchar:=32;
getbyte:=acc;end;{:51}{59:}procedure getfourbytes;var c:integer;
r:integer;q:integer;begin repeat getnext;until curchar<>32;r:=0;
curbytes.b0:=0;curbytes.b1:=0;curbytes.b2:=0;curbytes.b3:=0;
if curchar=72 then r:=16 else if curchar=79 then r:=8 else begin begin
if charsonline>0 then writeln(' ');
write('An octal ("O") or hex ("H") value is needed here');
showerrorcontext;end;repeat getnext until(curchar=40)or(curchar=41);end;
if r>0 then begin q:=256 div r;repeat getnext;until curchar<>32;
while((curchar>=48)and(curchar<=57))or((curchar>=65)and(curchar<=70))do{
60:}begin if curchar>=65 then curchar:=curchar-7;
c:=(r*curbytes.b0)+(curbytes.b1 div q);
if c>255 then begin curbytes.b0:=0;curbytes.b1:=0;curbytes.b2:=0;
curbytes.b3:=0;
if r=8 then begin begin if charsonline>0 then writeln(' ');
write('Sorry, the maximum octal value is O 37777777777');
showerrorcontext;end;repeat getnext until(curchar=40)or(curchar=41);
end else begin begin if charsonline>0 then writeln(' ');
write('Sorry, the maximum hex value is H FFFFFFFF');showerrorcontext;
end;repeat getnext until(curchar=40)or(curchar=41);end;
end else if curchar>=48+r then begin begin if charsonline>0 then writeln
(' ');write('Illegal digit');showerrorcontext;end;
repeat getnext until(curchar=40)or(curchar=41);
end else begin curbytes.b0:=c;
curbytes.b1:=(r*(curbytes.b1 mod q))+(curbytes.b2 div q);
curbytes.b2:=(r*(curbytes.b2 mod q))+(curbytes.b3 div q);
curbytes.b3:=(r*(curbytes.b3 mod q))+curchar-48;getnext;end;end{:60};
end;end;{:59}{62:}function getfix:fixword;var negative:boolean;
acc:integer;intpart:integer;j:0..7;begin repeat getnext;
until curchar<>32;negative:=false;acc:=0;
if(curchar<>82)and(curchar<>68)then begin begin if charsonline>0 then
writeln(' ');write('An "R" or "D" value is needed here');
showerrorcontext;end;repeat getnext until(curchar=40)or(curchar=41);
end else begin{63:}repeat getnext;if curchar=45 then begin curchar:=32;
negative:=true;end else if curchar=43 then curchar:=32;
until curchar<>32{:63};while(curchar>=48)and(curchar<=57)do{64:}
begin acc:=acc*10+curchar-48;
if acc>=2048 then begin begin begin if charsonline>0 then writeln(' ');
write('Real constants must be less than 2048');showerrorcontext;end;
repeat getnext until(curchar=40)or(curchar=41);end;acc:=0;curchar:=32;
end else getnext;end{:64};intpart:=acc;acc:=0;if curchar=46 then{66:}
begin j:=0;getnext;
while(curchar>=48)and(curchar<=57)do begin if j<7 then begin j:=j+1;
fractiondigits[j]:=2097152*(curchar-48);end;getnext;end;acc:=0;
while j>0 do begin acc:=fractiondigits[j]+(acc div 10);j:=j-1;end;
acc:=(acc+10)div 20;end{:66};
if(acc>=1048576)and(intpart=2047)then begin begin if charsonline>0 then
writeln(' ');write('Real constants must be less than 2048');
showerrorcontext;end;repeat getnext until(curchar=40)or(curchar=41);
end else acc:=intpart*1048576+acc;end;
if negative then getfix:=-acc else getfix:=acc;end;{:62}{75:}
function sortin(h:pointer;d:fixword):pointer;var p:pointer;
begin if(d=0)and(h<>1)then sortin:=0 else begin p:=h;
while d>=memory[link[p]]do p:=link[p];
if(d=memory[p])and(p<>h)then sortin:=p else if memptr=1032 then begin
begin if charsonline>0 then writeln(' ');
write('Memory overflow: more than 1028 widths, etc');showerrorcontext;
end;writeln('Congratulations! It''s hard to make this error.');
sortin:=p;end else begin memptr:=memptr+1;memory[memptr]:=d;
link[memptr]:=link[p];link[p]:=memptr;memory[h]:=memory[h]+1;
sortin:=memptr;end;end;end;{:75}{77:}function mincover(h:pointer;
d:fixword):integer;var p:pointer;l:fixword;m:integer;begin m:=0;
p:=link[h];nextd:=memory[0];while p<>0 do begin m:=m+1;l:=memory[p];
while memory[link[p]]<=l+d do p:=link[p];p:=link[p];
if memory[p]-l<nextd then nextd:=memory[p]-l;end;mincover:=m;end;{:77}
{78:}function shorten(h:pointer;m:integer):fixword;var d:fixword;
k:integer;begin if memory[h]>m then begin excess:=memory[h]-m;
k:=mincover(h,0);d:=nextd;repeat d:=d+d;k:=mincover(h,d);until k<=m;
d:=d div 2;k:=mincover(h,d);while k>m do begin d:=nextd;
k:=mincover(h,d);end;shorten:=d;end else shorten:=0;end;{:78}{80:}
procedure setindices(h:pointer;d:fixword);var p:pointer;q:pointer;
m:byte;l:fixword;begin q:=h;p:=link[q];m:=0;while p<>0 do begin m:=m+1;
l:=memory[p];index[p]:=m;while memory[link[p]]<=l+d do begin p:=link[p];
index[p]:=m;excess:=excess-1;if excess=0 then d:=0;end;link[q]:=p;
memory[p]:=l+(memory[p]-l)div 2;q:=p;p:=link[p];end;memory[h]:=m;end;
{:80}{83:}procedure junkerror;
begin begin if charsonline>0 then writeln(' ');
write('There''s junk here that is not in parentheses');showerrorcontext;
end;repeat getnext until(curchar=40)or(curchar=41);end;{:83}{86:}
procedure readfourbytes(l:headerindex);begin getfourbytes;
headerbytes[l]:=curbytes.b0;headerbytes[l+1]:=curbytes.b1;
headerbytes[l+2]:=curbytes.b2;headerbytes[l+3]:=curbytes.b3;end;{:86}
{87:}procedure readBCPL(l:headerindex;n:byte);var k:headerindex;
begin k:=l;while curchar=32 do getnext;
while(curchar<>40)and(curchar<>41)do begin if k<l+n then k:=k+1;
if k<l+n then headerbytes[k]:=curchar;getnext;end;
if k=l+n then begin begin if charsonline>0 then writeln(' ');
write('String is too long; its first ',n-1:1,' characters will be kept')
;showerrorcontext;end;k:=k-1;end;headerbytes[l]:=k-l;
while k<l+n-1 do begin k:=k+1;headerbytes[k]:=0;end;end;{:87}{96:}
procedure checktag(c:byte);begin case chartag[c]of 0:;
1:begin if charsonline>0 then writeln(' ');
write('This character already appeared in a LIGTABLE LABEL');
showerrorcontext;end;2:begin if charsonline>0 then writeln(' ');
write('This character already has a NEXTLARGER spec');showerrorcontext;
end;3:begin if charsonline>0 then writeln(' ');
write('This character already has a VARCHAR spec');showerrorcontext;end;
end;end;{:96}{107:}procedure printoctal(c:byte);
begin write('''',(c div 64):1,((c div 8)mod 8):1,(c mod 8):1);end;{:107}
{121:}function hashinput(p,c:indx):boolean;label 30;var cc:0..3;
zz:0..255;y:0..255;key:integer;t:integer;
begin if hashptr=hashsize then begin hashinput:=false;goto 30;end;{122:}
y:=ligkern[p].b1;t:=ligkern[p].b2;cc:=0;zz:=ligkern[p].b3;
if t>=128 then zz:=y else begin case t of 0,6:;5,11:zz:=y;1,7:cc:=1;
2:cc:=2;3:cc:=3;end;end{:122};key:=256*c+y+1;h:=(1009*key)mod hashsize;
while hash[h]>0 do begin if hash[h]<=key then begin if hash[h]=key then
begin hashinput:=false;goto 30;end;t:=hash[h];hash[h]:=key;key:=t;
t:=class[h];class[h]:=cc;cc:=t;t:=ligz[h];ligz[h]:=zz;zz:=t;end;
if h>0 then h:=h-1 else h:=hashsize;end;hash[h]:=key;class[h]:=cc;
ligz[h]:=zz;hashptr:=hashptr+1;hashlist[hashptr]:=h;hashinput:=true;
30:end;{:121}{123:}ifdef('notdef')function f(h,x,y:indx):indx;begin end;
endif('notdef')function eval(x,y:indx):indx;var key:integer;
begin key:=256*x+y+1;h:=(1009*key)mod hashsize;
while hash[h]>key do if h>0 then h:=h-1 else h:=hashsize;
if hash[h]<key then eval:=y else eval:=f(h,x,y);end;{:123}{124:}
function f(h,x,y:indx):indx;begin case class[h]of 0:;
1:begin class[h]:=4;ligz[h]:=eval(ligz[h],y);class[h]:=0;end;
2:begin class[h]:=4;ligz[h]:=eval(x,ligz[h]);class[h]:=0;end;
3:begin class[h]:=4;ligz[h]:=eval(eval(x,ligz[h]),y);class[h]:=0;end;
4:begin xligcycle:=x;yligcycle:=y;ligz[h]:=257;class[h]:=0;end;end;
f:=ligz[h];end;{:124}{136:}procedure outscaled(x:fixword);var n:byte;
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
putbyte(m div 256,tfmfile);putbyte(m mod 256,tfmfile);end;{:136}{146:}
procedure paramenter;begin{48:}namelength:=5;curname[16]:=83;
curname[17]:=76;curname[18]:=65;curname[19]:=78;curname[20]:=84;
entername(21);namelength:=5;curname[16]:=83;curname[17]:=80;
curname[18]:=65;curname[19]:=67;curname[20]:=69;entername(22);
namelength:=7;curname[14]:=83;curname[15]:=84;curname[16]:=82;
curname[17]:=69;curname[18]:=84;curname[19]:=67;curname[20]:=72;
entername(23);namelength:=6;curname[15]:=83;curname[16]:=72;
curname[17]:=82;curname[18]:=73;curname[19]:=78;curname[20]:=75;
entername(24);namelength:=7;curname[14]:=88;curname[15]:=72;
curname[16]:=69;curname[17]:=73;curname[18]:=71;curname[19]:=72;
curname[20]:=84;entername(25);namelength:=4;curname[17]:=81;
curname[18]:=85;curname[19]:=65;curname[20]:=68;entername(26);
namelength:=10;curname[11]:=69;curname[12]:=88;curname[13]:=84;
curname[14]:=82;curname[15]:=65;curname[16]:=83;curname[17]:=80;
curname[18]:=65;curname[19]:=67;curname[20]:=69;entername(27);
namelength:=4;curname[17]:=78;curname[18]:=85;curname[19]:=77;
curname[20]:=49;entername(28);namelength:=4;curname[17]:=78;
curname[18]:=85;curname[19]:=77;curname[20]:=50;entername(29);
namelength:=4;curname[17]:=78;curname[18]:=85;curname[19]:=77;
curname[20]:=51;entername(30);namelength:=6;curname[15]:=68;
curname[16]:=69;curname[17]:=78;curname[18]:=79;curname[19]:=77;
curname[20]:=49;entername(31);namelength:=6;curname[15]:=68;
curname[16]:=69;curname[17]:=78;curname[18]:=79;curname[19]:=77;
curname[20]:=50;entername(32);namelength:=4;curname[17]:=83;
curname[18]:=85;curname[19]:=80;curname[20]:=49;entername(33);
namelength:=4;curname[17]:=83;curname[18]:=85;curname[19]:=80;
curname[20]:=50;entername(34);namelength:=4;curname[17]:=83;
curname[18]:=85;curname[19]:=80;curname[20]:=51;entername(35);
namelength:=4;curname[17]:=83;curname[18]:=85;curname[19]:=66;
curname[20]:=49;entername(36);namelength:=4;curname[17]:=83;
curname[18]:=85;curname[19]:=66;curname[20]:=50;entername(37);
namelength:=7;curname[14]:=83;curname[15]:=85;curname[16]:=80;
curname[17]:=68;curname[18]:=82;curname[19]:=79;curname[20]:=80;
entername(38);namelength:=7;curname[14]:=83;curname[15]:=85;
curname[16]:=66;curname[17]:=68;curname[18]:=82;curname[19]:=79;
curname[20]:=80;entername(39);namelength:=6;curname[15]:=68;
curname[16]:=69;curname[17]:=76;curname[18]:=73;curname[19]:=77;
curname[20]:=49;entername(40);namelength:=6;curname[15]:=68;
curname[16]:=69;curname[17]:=76;curname[18]:=73;curname[19]:=77;
curname[20]:=50;entername(41);namelength:=10;curname[11]:=65;
curname[12]:=88;curname[13]:=73;curname[14]:=83;curname[15]:=72;
curname[16]:=69;curname[17]:=73;curname[18]:=71;curname[19]:=72;
curname[20]:=84;entername(42);namelength:=20;curname[1]:=68;
curname[2]:=69;curname[3]:=70;curname[4]:=65;curname[5]:=85;
curname[6]:=76;curname[7]:=84;curname[8]:=82;curname[9]:=85;
curname[10]:=76;curname[11]:=69;curname[12]:=84;curname[13]:=72;
curname[14]:=73;curname[15]:=67;curname[16]:=75;curname[17]:=78;
curname[18]:=69;curname[19]:=83;curname[20]:=83;entername(28);
namelength:=13;curname[8]:=66;curname[9]:=73;curname[10]:=71;
curname[11]:=79;curname[12]:=80;curname[13]:=83;curname[14]:=80;
curname[15]:=65;curname[16]:=67;curname[17]:=73;curname[18]:=78;
curname[19]:=71;curname[20]:=49;entername(29);namelength:=13;
curname[8]:=66;curname[9]:=73;curname[10]:=71;curname[11]:=79;
curname[12]:=80;curname[13]:=83;curname[14]:=80;curname[15]:=65;
curname[16]:=67;curname[17]:=73;curname[18]:=78;curname[19]:=71;
curname[20]:=50;entername(30);namelength:=13;curname[8]:=66;
curname[9]:=73;curname[10]:=71;curname[11]:=79;curname[12]:=80;
curname[13]:=83;curname[14]:=80;curname[15]:=65;curname[16]:=67;
curname[17]:=73;curname[18]:=78;curname[19]:=71;curname[20]:=51;
entername(31);namelength:=13;curname[8]:=66;curname[9]:=73;
curname[10]:=71;curname[11]:=79;curname[12]:=80;curname[13]:=83;
curname[14]:=80;curname[15]:=65;curname[16]:=67;curname[17]:=73;
curname[18]:=78;curname[19]:=71;curname[20]:=52;entername(32);
namelength:=13;curname[8]:=66;curname[9]:=73;curname[10]:=71;
curname[11]:=79;curname[12]:=80;curname[13]:=83;curname[14]:=80;
curname[15]:=65;curname[16]:=67;curname[17]:=73;curname[18]:=78;
curname[19]:=71;curname[20]:=53;entername(33);{:48};end;
procedure nameenter;begin{47:}equiv[0]:=0;namelength:=8;curname[13]:=67;
curname[14]:=72;curname[15]:=69;curname[16]:=67;curname[17]:=75;
curname[18]:=83;curname[19]:=85;curname[20]:=77;entername(1);
namelength:=10;curname[11]:=68;curname[12]:=69;curname[13]:=83;
curname[14]:=73;curname[15]:=71;curname[16]:=78;curname[17]:=83;
curname[18]:=73;curname[19]:=90;curname[20]:=69;entername(2);
namelength:=11;curname[10]:=68;curname[11]:=69;curname[12]:=83;
curname[13]:=73;curname[14]:=71;curname[15]:=78;curname[16]:=85;
curname[17]:=78;curname[18]:=73;curname[19]:=84;curname[20]:=83;
entername(3);namelength:=12;curname[9]:=67;curname[10]:=79;
curname[11]:=68;curname[12]:=73;curname[13]:=78;curname[14]:=71;
curname[15]:=83;curname[16]:=67;curname[17]:=72;curname[18]:=69;
curname[19]:=77;curname[20]:=69;entername(4);namelength:=6;
curname[15]:=70;curname[16]:=65;curname[17]:=77;curname[18]:=73;
curname[19]:=76;curname[20]:=89;entername(5);namelength:=4;
curname[17]:=70;curname[18]:=65;curname[19]:=67;curname[20]:=69;
entername(6);namelength:=16;curname[5]:=83;curname[6]:=69;
curname[7]:=86;curname[8]:=69;curname[9]:=78;curname[10]:=66;
curname[11]:=73;curname[12]:=84;curname[13]:=83;curname[14]:=65;
curname[15]:=70;curname[16]:=69;curname[17]:=70;curname[18]:=76;
curname[19]:=65;curname[20]:=71;entername(7);namelength:=6;
curname[15]:=72;curname[16]:=69;curname[17]:=65;curname[18]:=68;
curname[19]:=69;curname[20]:=82;entername(8);namelength:=9;
curname[12]:=70;curname[13]:=79;curname[14]:=78;curname[15]:=84;
curname[16]:=68;curname[17]:=73;curname[18]:=77;curname[19]:=69;
curname[20]:=78;entername(9);namelength:=8;curname[13]:=76;
curname[14]:=73;curname[15]:=71;curname[16]:=84;curname[17]:=65;
curname[18]:=66;curname[19]:=76;curname[20]:=69;entername(10);
namelength:=12;curname[9]:=66;curname[10]:=79;curname[11]:=85;
curname[12]:=78;curname[13]:=68;curname[14]:=65;curname[15]:=82;
curname[16]:=89;curname[17]:=67;curname[18]:=72;curname[19]:=65;
curname[20]:=82;entername(11);namelength:=9;curname[12]:=67;
curname[13]:=72;curname[14]:=65;curname[15]:=82;curname[16]:=65;
curname[17]:=67;curname[18]:=84;curname[19]:=69;curname[20]:=82;
entername(12);namelength:=9;curname[12]:=80;curname[13]:=65;
curname[14]:=82;curname[15]:=65;curname[16]:=77;curname[17]:=69;
curname[18]:=84;curname[19]:=69;curname[20]:=82;entername(20);
namelength:=6;curname[15]:=67;curname[16]:=72;curname[17]:=65;
curname[18]:=82;curname[19]:=87;curname[20]:=68;entername(51);
namelength:=6;curname[15]:=67;curname[16]:=72;curname[17]:=65;
curname[18]:=82;curname[19]:=72;curname[20]:=84;entername(52);
namelength:=6;curname[15]:=67;curname[16]:=72;curname[17]:=65;
curname[18]:=82;curname[19]:=68;curname[20]:=80;entername(53);
namelength:=6;curname[15]:=67;curname[16]:=72;curname[17]:=65;
curname[18]:=82;curname[19]:=73;curname[20]:=67;entername(54);
namelength:=10;curname[11]:=78;curname[12]:=69;curname[13]:=88;
curname[14]:=84;curname[15]:=76;curname[16]:=65;curname[17]:=82;
curname[18]:=71;curname[19]:=69;curname[20]:=82;entername(55);
namelength:=7;curname[14]:=86;curname[15]:=65;curname[16]:=82;
curname[17]:=67;curname[18]:=72;curname[19]:=65;curname[20]:=82;
entername(56);namelength:=3;curname[18]:=84;curname[19]:=79;
curname[20]:=80;entername(57);namelength:=3;curname[18]:=77;
curname[19]:=73;curname[20]:=68;entername(58);namelength:=3;
curname[18]:=66;curname[19]:=79;curname[20]:=84;entername(59);
namelength:=3;curname[18]:=82;curname[19]:=69;curname[20]:=80;
entername(60);namelength:=3;curname[18]:=69;curname[19]:=88;
curname[20]:=84;entername(60);namelength:=7;curname[14]:=67;
curname[15]:=79;curname[16]:=77;curname[17]:=77;curname[18]:=69;
curname[19]:=78;curname[20]:=84;entername(0);namelength:=5;
curname[16]:=76;curname[17]:=65;curname[18]:=66;curname[19]:=69;
curname[20]:=76;entername(70);namelength:=4;curname[17]:=83;
curname[18]:=84;curname[19]:=79;curname[20]:=80;entername(71);
namelength:=4;curname[17]:=83;curname[18]:=75;curname[19]:=73;
curname[20]:=80;entername(72);namelength:=3;curname[18]:=75;
curname[19]:=82;curname[20]:=78;entername(73);namelength:=3;
curname[18]:=76;curname[19]:=73;curname[20]:=71;entername(74);
namelength:=4;curname[17]:=47;curname[18]:=76;curname[19]:=73;
curname[20]:=71;entername(76);namelength:=5;curname[16]:=47;
curname[17]:=76;curname[18]:=73;curname[19]:=71;curname[20]:=62;
entername(80);namelength:=4;curname[17]:=76;curname[18]:=73;
curname[19]:=71;curname[20]:=47;entername(75);namelength:=5;
curname[16]:=76;curname[17]:=73;curname[18]:=71;curname[19]:=47;
curname[20]:=62;entername(79);namelength:=5;curname[16]:=47;
curname[17]:=76;curname[18]:=73;curname[19]:=71;curname[20]:=47;
entername(77);namelength:=6;curname[15]:=47;curname[16]:=76;
curname[17]:=73;curname[18]:=71;curname[19]:=47;curname[20]:=62;
entername(81);namelength:=7;curname[14]:=47;curname[15]:=76;
curname[16]:=73;curname[17]:=71;curname[18]:=47;curname[19]:=62;
curname[20]:=62;entername(85);{:47};paramenter;end;
procedure readligkern;var krnptr:0..maxkerns;c:byte;begin{94:}
begin while level=1 do begin while curchar=32 do getnext;
if curchar=40 then{95:}begin getname;
if curcode=0 then skiptoendofitem else if curcode<70 then begin begin if
charsonline>0 then writeln(' ');
write('This property name doesn''t belong in a LIGTABLE list');
showerrorcontext;end;skiptoendofitem;
end else begin case curcode of 70:{97:}
begin while curchar=32 do getnext;
if curchar=66 then begin charremainder[256]:=nl;
repeat getnext until(curchar=40)or(curchar=41);
end else begin begin if(curchar>41)or(curchar<40)then loc:=loc-1;end;
c:=getbyte;checktag(c);chartag[c]:=1;charremainder[c]:=nl;end;
if minnl<=nl then minnl:=nl+1;lkstepended:=false;end{:97};71:{99:}
if not lkstepended then begin if charsonline>0 then writeln(' ');
write('STOP must follow LIG or KRN');showerrorcontext;
end else begin ligkern[nl-1].b0:=128;lkstepended:=false;end{:99};
72:{100:}
if not lkstepended then begin if charsonline>0 then writeln(' ');
write('SKIP must follow LIG or KRN');showerrorcontext;
end else begin c:=getbyte;
if c>=128 then begin if charsonline>0 then writeln(' ');
write('Maximum SKIP amount is 127');showerrorcontext;
end else if nl+c>=maxligsteps then begin if charsonline>0 then writeln(
' ');write('Sorry, LIGTABLE too long for me to handle');
showerrorcontext;end else begin ligkern[nl-1].b0:=c;
if minnl<=nl+c then minnl:=nl+c+1;end;lkstepended:=false;end{:100};
73:{102:}begin ligkern[nl].b0:=0;ligkern[nl].b1:=getbyte;
kern[nk]:=getfix;krnptr:=0;
while kern[krnptr]<>kern[nk]do krnptr:=krnptr+1;
if krnptr=nk then begin if nk<maxkerns then nk:=nk+1 else begin begin if
charsonline>0 then writeln(' ');
write('Sorry, too many different kerns for me to handle');
showerrorcontext;end;krnptr:=krnptr-1;end;end;
ligkern[nl].b2:=128+(krnptr div 256);ligkern[nl].b3:=krnptr mod 256;
if nl>=maxligsteps-1 then begin if charsonline>0 then writeln(' ');
write('Sorry, LIGTABLE too long for me to handle');showerrorcontext;
end else nl:=nl+1;lkstepended:=true;end{:102};
74,75,76,77,79,80,81,85:{101:}begin ligkern[nl].b0:=0;
ligkern[nl].b2:=curcode-74;ligkern[nl].b1:=getbyte;
ligkern[nl].b3:=getbyte;
if nl>=maxligsteps-1 then begin if charsonline>0 then writeln(' ');
write('Sorry, LIGTABLE too long for me to handle');showerrorcontext;
end else nl:=nl+1;lkstepended:=true;end{:101};end;finishtheproperty;end;
end{:95}else if curchar=41 then skiptoendofitem else junkerror;end;
begin loc:=loc-1;level:=level+1;curchar:=41;end;end{:94};end;
procedure readcharinfo;var c:byte;begin{103:}begin c:=getbyte;
if verbose then{108:}begin if charsonline=8 then begin writeln(' ');
charsonline:=1;end else begin if charsonline>0 then write(' ');
charsonline:=charsonline+1;end;printoctal(c);end{:108};
while level=1 do begin while curchar=32 do getnext;
if curchar=40 then{104:}begin getname;
if curcode=0 then skiptoendofitem else if(curcode<51)or(curcode>56)then
begin begin if charsonline>0 then writeln(' ');
write('This property name doesn''t belong in a CHARACTER list');
showerrorcontext;end;skiptoendofitem;
end else begin case curcode of 51:charwd[c]:=sortin(1,getfix);
52:charht[c]:=sortin(2,getfix);53:chardp[c]:=sortin(3,getfix);
54:charic[c]:=sortin(4,getfix);55:begin checktag(c);chartag[c]:=2;
charremainder[c]:=getbyte;end;56:{105:}
begin if ne=256 then begin if charsonline>0 then writeln(' ');
write('At most 256 VARCHAR specs are allowed');showerrorcontext;
end else begin checktag(c);chartag[c]:=3;charremainder[c]:=ne;
exten[ne].b0:=0;exten[ne].b1:=0;exten[ne].b2:=0;exten[ne].b3:=0;
while level=2 do begin while curchar=32 do getnext;
if curchar=40 then{106:}begin getname;
if curcode=0 then skiptoendofitem else if(curcode<57)or(curcode>60)then
begin begin if charsonline>0 then writeln(' ');
write('This property name doesn''t belong in a VARCHAR list');
showerrorcontext;end;skiptoendofitem;
end else begin case curcode-(57)of 0:exten[ne].b0:=getbyte;
1:exten[ne].b1:=getbyte;2:exten[ne].b2:=getbyte;3:exten[ne].b3:=getbyte;
end;finishtheproperty;end;end{:106}
else if curchar=41 then skiptoendofitem else junkerror;end;ne:=ne+1;
begin loc:=loc-1;level:=level+1;curchar:=41;end;end;end{:105};end;
finishtheproperty;end;end{:104}
else if curchar=41 then skiptoendofitem else junkerror;end;
if charwd[c]=0 then charwd[c]:=sortin(1,0);begin loc:=loc-1;
level:=level+1;curchar:=41;end;end{:103};end;procedure readinput;
var c:byte;begin{82:}curchar:=32;repeat while curchar=32 do getnext;
if curchar=40 then{84:}begin getname;
if curcode=0 then skiptoendofitem else if curcode>12 then begin begin if
charsonline>0 then writeln(' ');
write('This property name doesn''t belong on the outer level');
showerrorcontext;end;skiptoendofitem;end else begin{85:}
case curcode of 1:begin checksumspecified:=true;readfourbytes(0);end;
2:{88:}begin nextd:=getfix;
if nextd<1048576 then begin if charsonline>0 then writeln(' ');
write('The design size must be at least 1');showerrorcontext;
end else designsize:=nextd;end{:88};3:{89:}begin nextd:=getfix;
if nextd<=0 then begin if charsonline>0 then writeln(' ');
write('The number of units per design size must be positive');
showerrorcontext;end else designunits:=nextd;end{:89};4:readBCPL(8,40);
5:readBCPL(48,20);6:headerbytes[71]:=getbyte;7:{90:}
begin while curchar=32 do getnext;
if curchar=84 then sevenbitsafeflag:=true else if curchar=70 then
sevenbitsafeflag:=false else begin if charsonline>0 then writeln(' ');
write('The flag value should be "TRUE" or "FALSE"');showerrorcontext;
end;repeat getnext until(curchar=40)or(curchar=41);end{:90};8:{91:}
begin c:=getbyte;
if c<18 then begin begin if charsonline>0 then writeln(' ');
write('HEADER indices should be 18 or more');showerrorcontext;end;
repeat getnext until(curchar=40)or(curchar=41);
end else if 4*c+4>maxheaderbytes then begin begin if charsonline>0 then
writeln(' ');
write('This HEADER index is too big for my present table size');
showerrorcontext;end;repeat getnext until(curchar=40)or(curchar=41);
end else begin while headerptr<4*c+4 do begin headerbytes[headerptr]:=0;
headerptr:=headerptr+1;end;readfourbytes(4*c);end;end{:91};9:{92:}
begin while level=1 do begin while curchar=32 do getnext;
if curchar=40 then{93:}begin getname;
if curcode=0 then skiptoendofitem else if(curcode<20)or(curcode>=51)then
begin begin if charsonline>0 then writeln(' ');
write('This property name doesn''t belong in a FONTDIMEN list');
showerrorcontext;end;skiptoendofitem;
end else begin if curcode=20 then c:=getbyte else c:=curcode-20;
if c=0 then begin begin if charsonline>0 then writeln(' ');
write('PARAMETER index must not be zero');showerrorcontext;end;
skiptoendofitem;
end else if c>maxparamwords then begin begin if charsonline>0 then
writeln(' ');
write('This PARAMETER index is too big for my present table size');
showerrorcontext;end;skiptoendofitem;
end else begin while np<c do begin np:=np+1;param[np]:=0;end;
param[c]:=getfix;finishtheproperty;end;end;end{:93}
else if curchar=41 then skiptoendofitem else junkerror;end;
begin loc:=loc-1;level:=level+1;curchar:=41;end;end{:92};10:readligkern;
11:bchar:=getbyte;12:readcharinfo;end{:85};finishtheproperty;end;
end{:84}
else if(curchar=41)and not inputhasended then begin begin if charsonline
>0 then writeln(' ');write('Extra right parenthesis');showerrorcontext;
end;loc:=loc+1;curchar:=32;end else if not inputhasended then junkerror;
until inputhasended{:82};end;procedure corrandcheck;var c:0..256;
hh:0..hashsize;ligptr:0..maxligsteps;g:byte;begin{110:}
if nl>0 then{116:}
begin if charremainder[256]<32767 then begin ligkern[nl].b0:=255;
ligkern[nl].b1:=0;ligkern[nl].b2:=0;ligkern[nl].b3:=0;nl:=nl+1;end;
while minnl>nl do begin ligkern[nl].b0:=255;ligkern[nl].b1:=0;
ligkern[nl].b2:=0;ligkern[nl].b3:=0;nl:=nl+1;end;
if ligkern[nl-1].b0=0 then ligkern[nl-1].b0:=128;end{:116};
sevenunsafe:=false;for c:=0 to 255 do if charwd[c]<>0 then{111:}
case chartag[c]of 0:;1:{120:}begin ligptr:=charremainder[c];
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
[ligptr].b0;until ligptr>=nl;end{:120};2:begin g:=charremainder[c];
if(g>=128)and(c<128)then sevenunsafe:=true;
if charwd[g]=0 then begin charwd[g]:=sortin(1,0);
write('The character NEXTLARGER than',' ');printoctal(c);
writeln(' had no CHARACTER spec.');end;end;3:{112:}
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
writeln(' had no CHARACTER spec.');end;end;end{:112};end{:111};
if charremainder[256]<32767 then begin c:=256;{120:}
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
[ligptr].b0;until ligptr>=nl;end{:120};end;
if sevenbitsafeflag and sevenunsafe then writeln(
'The font is not really seven-bit-safe!');{125:}
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
end{:125};{126:}
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
printoctal(c);writeln('!');end;end;end{:126};for c:=0 to 255 do{113:}
if chartag[c]=2 then begin g:=charremainder[c];
while(g<c)and(chartag[g]=2)do g:=charremainder[g];
if g=c then begin chartag[c]:=0;
write('A cycle of NEXTLARGER characters has been broken at ');
printoctal(c);writeln('.');end;end{:113};{115:}delta:=shorten(1,255);
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
end;{:115}{:110}end;{:146}{147:}begin initialize;nameenter;readinput;
if verbose then writeln('.');corrandcheck;{128:}{130:}
lh:=headerptr div 4;notfound:=true;bc:=0;
while notfound do if(charwd[bc]>0)or(bc=255)then notfound:=false else bc
:=bc+1;notfound:=true;ec:=255;
while notfound do if(charwd[ec]>0)or(ec=0)then notfound:=false else ec:=
ec-1;if bc>ec then bc:=1;memory[1]:=memory[1]+1;memory[2]:=memory[2]+1;
memory[3]:=memory[3]+1;memory[4]:=memory[4]+1;{139:}{140:}labelptr:=0;
labeltable[0].rr:=-1;
for c:=bc to ec do if chartag[c]=1 then begin sortptr:=labelptr;
while labeltable[sortptr].rr>toint(charremainder[c])do begin labeltable[
sortptr+1]:=labeltable[sortptr];sortptr:=sortptr-1;end;
labeltable[sortptr+1].cc:=c;labeltable[sortptr+1].rr:=charremainder[c];
labelptr:=labelptr+1;end{:140};
if bchar<256 then begin extralocneeded:=true;lkoffset:=1;
end else begin extralocneeded:=false;lkoffset:=0;end;{141:}
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
sortptr:=sortptr-1;end;end{:141};
if charremainder[256]<32767 then begin ligkern[nl-1].b2:=(charremainder[
256]+lkoffset)div 256;
ligkern[nl-1].b3:=(charremainder[256]+lkoffset)mod 256;end{:139};
lf:=6+lh+(ec-bc+1)+memory[1]+memory[2]+memory[3]+memory[4]+nl+lkoffset+
nk+ne+np;{:130};{131:}putbyte((lf)div 256,tfmfile);
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
putbyte((np)mod 256,tfmfile);{:131};{133:}
if not checksumspecified then{134:}begin curbytes.b0:=bc;
curbytes.b1:=ec;curbytes.b2:=bc;curbytes.b3:=ec;
for c:=bc to ec do if charwd[c]>0 then begin tempwidth:=memory[charwd[c]
];if designunits<>1048576 then tempwidth:=round((tempwidth/designunits)
*1048576.0);tempwidth:=tempwidth+(c+4)*4194304;
curbytes.b0:=(curbytes.b0+curbytes.b0+tempwidth)mod 255;
curbytes.b1:=(curbytes.b1+curbytes.b1+tempwidth)mod 253;
curbytes.b2:=(curbytes.b2+curbytes.b2+tempwidth)mod 251;
curbytes.b3:=(curbytes.b3+curbytes.b3+tempwidth)mod 247;end;
headerbytes[0]:=curbytes.b0;headerbytes[1]:=curbytes.b1;
headerbytes[2]:=curbytes.b2;headerbytes[3]:=curbytes.b3;end{:134};
headerbytes[4]:=designsize div 16777216;
headerbytes[5]:=(designsize div 65536)mod 256;
headerbytes[6]:=(designsize div 256)mod 256;
headerbytes[7]:=designsize mod 256;
if not sevenunsafe then headerbytes[68]:=128;
for j:=0 to headerptr-1 do putbyte(headerbytes[j],tfmfile);{:133};{135:}
index[0]:=0;for c:=bc to ec do begin putbyte(index[charwd[c]],tfmfile);
putbyte(index[charht[c]]*16+index[chardp[c]],tfmfile);
putbyte(index[charic[c]]*4+chartag[c],tfmfile);
putbyte(charremainder[c],tfmfile);end{:135};{137:}
for q:=1 to 4 do begin putbyte(0,tfmfile);putbyte(0,tfmfile);
putbyte(0,tfmfile);putbyte(0,tfmfile);p:=link[q];
while p>0 do begin outscaled(memory[p]);p:=link[p];end;end;{:137};{142:}
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
end;if nk>0 then for krnptr:=0 to nk-1 do outscaled(kern[krnptr]){:142};
{143:}
if ne>0 then for c:=0 to ne-1 do begin putbyte(exten[c].b0,tfmfile);
putbyte(exten[c].b1,tfmfile);putbyte(exten[c].b2,tfmfile);
putbyte(exten[c].b3,tfmfile);end;{:143};{144:}
for parptr:=1 to np do begin if parptr=1 then{145:}
begin if param[1]<0 then begin param[1]:=param[1]+1073741824;
putbyte((param[1]div 16777216)+192,tfmfile);
end else putbyte(param[1]div 16777216,tfmfile);
putbyte((param[1]div 65536)mod 256,tfmfile);
putbyte((param[1]div 256)mod 256,tfmfile);
putbyte(param[1]mod 256,tfmfile);end{:145}else outscaled(param[parptr]);
end{:144}{:128};end.{:147}
