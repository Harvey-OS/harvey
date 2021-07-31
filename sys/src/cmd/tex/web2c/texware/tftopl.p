{2:}
@define var tfm;
program TFtoPL;label{3:}9999;{:3}const{4:}tfmsize=30000;ligsize=5000;
hashsize=5003;{:4}{107:}charcodeascii=0;charcodeoctal=1;
charcodedefault=2;{:107}{111:}noptions=2;argoptions=1;{:111}type{18:}
byte=0..255;index=integer;{:18}{106:}
charcodeformattype=charcodeascii..charcodedefault;{:106}var{6:}
tfmfile:packed file of 0..255;tfmname:packed array[1..PATHMAX]of char;
{:6}{8:}lf,lh,bc,ec,nw,nh,nd,ni,nl,nk,ne,np:0..32767;{:8}{16:}
plfile:text;plname:array[1..PATHMAX]of char;{:16}{19:}
#define tfm (tfmfilearray + 1001);
tfmfilearray:pointertobyte;{:19}{22:}
charbase,widthbase,heightbase,depthbase,italicbase,ligkernbase,kernbase,
extenbase,parambase:integer;{:22}{25:}fonttype:0..2;{:25}{27:}
ASCII04,ASCII10,ASCII14:ccharpointer;
ASCIIall:packed array[0..256]of char;
MBLstring,RIstring,RCEstring:ccharpointer;{:27}{29:}
dig:array[0..11]of 0..9;{:29}{32:}level:0..5;{:32}{45:}charsonline:0..8;
perfect:boolean;{:45}{47:}i:0..32767;c:0..256;d:0..3;k:index;r:0..65535;
count:0..127;{:47}{63:}labeltable:array[0..258]of record cc:0..256;
rr:0..ligsize;end;labelptr:0..257;sortptr:0..257;boundarychar:0..256;
bcharlabel:0..32767;{:63}{65:}activity:array[0..ligsize]of 0..2;
ai,acti:0..ligsize;{:65}{89:}hash:array[0..hashsize]of 0..66048;
class:array[0..hashsize]of 0..4;ligz:array[0..hashsize]of 0..257;
hashptr:0..hashsize;hashlist:array[0..hashsize]of 0..hashsize;
h,hh:0..hashsize;xligcycle,yligcycle:0..256;{:89}{103:}verbose:integer;
{:103}{108:}charcodeformat:charcodeformattype;{:108}
procedure initialize;var{101:}
longoptions:array[0..noptions]of getoptstruct;getoptreturnval:integer;
optionindex:integer;currentoption:0..noptions;{:101}
begin if(argc<2)or(argc>noptions+argoptions+3)then begin write(
'Usage: tftopl ');write('[-verbose] ');
write('[-charcode-format=<format>] ');
writeln('<tfm file> [<property list file>].');uexit(1);end;
tfmfilearray:=casttobytepointer(xmalloc(1002));{104:}verbose:=false;
{:104}{109:}charcodeformat:=charcodedefault;{:109};{100:}begin{102:}
currentoption:=0;longoptions[0].name:='verbose';
longoptions[0].hasarg:=0;longoptions[0].flag:=addressofint(verbose);
longoptions[0].val:=1;currentoption:=currentoption+1;{:102}{105:}
longoptions[currentoption].name:='charcode-format';
longoptions[currentoption].hasarg:=1;longoptions[currentoption].flag:=0;
longoptions[currentoption].val:=0;currentoption:=currentoption+1;{:105}
{110:}longoptions[currentoption].name:=0;
longoptions[currentoption].hasarg:=0;longoptions[currentoption].flag:=0;
longoptions[currentoption].val:=0;{:110};
repeat getoptreturnval:=getoptlongonly(argc,gargv,'',longoptions,
addressofint(optionindex));
if getoptreturnval<>-1 then begin if getoptreturnval=63 then uexit(1);
if(strcmp(longoptions[optionindex].name,'charcode-format')=0)then begin
if strcmp(optarg,'ascii')=0 then charcodeformat:=charcodeascii else if
strcmp(optarg,'octal')=0 then charcodeformat:=charcodeoctal else write(
'Bad character code format',optarg,'.');end else;end;
until getoptreturnval=-1;end{:100};{7:}setpaths(TFMFILEPATHBIT);
argv(optind,tfmname);
if testreadaccess(tfmname,TFMFILEPATH)then begin reset(tfmfile,tfmname);
end else begin printpascalstring(tfmname);
writeln(': TFM file not found.');uexit(1);end;
if verbose then writeln('This is TFtoPL, C Version 3.1');{:7}{17:}
if optind+1=argc then plfile:=stdout else begin argv(optind+1,plname);
rewrite(plfile,plname);end;{:17}{28:}
ASCII04:='  !"#$%&''()*+,-./0123456789:;<=>?';
ASCII10:=' @ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_';
ASCII14:=' `abcdefghijklmnopqrstuvwxyz{|}~ ';vstrcpy(ASCIIall,ASCII04);
vstrcat(ASCIIall,'@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_');
vstrcat(ASCIIall,'`abcdefghijklmnopqrstuvwxyz{|}~');MBLstring:=' MBL';
RIstring:=' RI ';RCEstring:=' RCE';{:28}{33:}level:=0;{:33}{46:}
charsonline:=0;perfect:=true;{:46}{64:}boundarychar:=256;
bcharlabel:=32767;labelptr:=0;labeltable[0].rr:=0;{:64}end;{:2}{30:}
procedure outdigs(j:integer);begin repeat j:=j-1;write(plfile,dig[j]:1);
until j=0;end;procedure printdigs(j:integer);begin repeat j:=j-1;
write(dig[j]:1);until j=0;end;{:30}{31:}procedure printoctal(c:byte);
var j:0..2;begin write('''');for j:=0 to 2 do begin dig[j]:=c mod 8;
c:=c div 8;end;printdigs(3);end;{:31}{34:}procedure outln;var l:0..5;
begin writeln(plfile);for l:=1 to level do write(plfile,'   ');end;
procedure left;begin level:=level+1;write(plfile,'(');end;
procedure right;begin level:=level-1;write(plfile,')');outln;end;{:34}
{35:}procedure outBCPL(k:index);var l:0..39;begin write(plfile,' ');
l:=tfm[k];while l>0 do begin k:=k+1;l:=l-1;
case tfm[k]div 32 of 1:write(plfile,ASCII04[1+(tfm[k]mod 32)]);
2:write(plfile,ASCII10[1+(tfm[k]mod 32)]);
3:write(plfile,ASCII14[1+(tfm[k]mod 32)]);end;end;end;{:35}{36:}
procedure outoctal(k,l:index);var a:0..1023;b:0..32;j:0..11;
begin write(plfile,' O ');a:=0;b:=0;j:=0;while l>0 do{37:}begin l:=l-1;
if tfm[k+l]<>0 then begin while b>2 do begin dig[j]:=a mod 8;a:=a div 8;
b:=b-3;j:=j+1;end;case b of 0:a:=tfm[k+l];1:a:=a+2*tfm[k+l];
2:a:=a+4*tfm[k+l];end;end;b:=b+8;end{:37};
while(a>0)or(j=0)do begin dig[j]:=a mod 8;a:=a div 8;j:=j+1;end;
outdigs(j);end;{:36}{38:}procedure outchar(c:byte);
begin if(fonttype>0)or(charcodeformat=charcodeoctal)then begin tfm[0]:=c
;outoctal(0,1)end else if(charcodeformat=charcodeascii)and(c>32)and(c<=
126)and(c<>40)and(c<>41)then write(plfile,' C ',ASCIIall[c-31])else if(c
>=48)and(c<=57)then write(plfile,' C ',c-48:1)else if(c>=65)and(c<=90)
then write(plfile,' C ',ASCII10[c-63])else if(c>=97)and(c<=122)then
write(plfile,' C ',ASCII14[c-95])else begin tfm[0]:=c;outoctal(0,1);end;
end;{:38}{39:}procedure outface(k:index);var s:0..1;b:0..8;
begin if tfm[k]>=18 then outoctal(k,1)else begin write(plfile,' F ');
s:=tfm[k]mod 2;b:=tfm[k]div 2;putbyte(MBLstring[1+(b mod 3)],plfile);
putbyte(RIstring[1+s],plfile);putbyte(RCEstring[1+(b div 3)],plfile);
end;end;{:39}{40:}procedure outfix(k:index);var a:0..4095;f:integer;
j:0..12;delta:integer;begin write(plfile,' R ');
a:=(tfm[k]*16)+(tfm[k+1]div 16);
f:=((tfm[k+1]mod 16)*toint(256)+tfm[k+2])*256+tfm[k+3];
if a>2047 then{43:}begin write(plfile,'-');a:=4096-a;
if f>0 then begin f:=1048576-f;a:=a-1;end;end{:43};{41:}begin j:=0;
repeat dig[j]:=a mod 10;a:=a div 10;j:=j+1;until a=0;outdigs(j);end{:41}
;{42:}begin write(plfile,'.');f:=10*f+5;delta:=10;
repeat if delta>1048576 then f:=f+524288-(delta div 2);
write(plfile,f div 1048576:1);f:=10*(f mod 1048576);delta:=delta*10;
until f<=delta;end;{:42};end;{:40}{52:}procedure checkBCPL(k,l:index);
var j:index;c:byte;begin if tfm[k]>=l then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ',
'String is too long; I''ve shortened it drastically.');end;tfm[k]:=1;
end;for j:=k+1 to k+tfm[k]do begin c:=tfm[j];
if(c=40)or(c=41)then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ',
'Parenthesis in string has been changed to slash.');end;tfm[j]:=47;
end else if(c<32)or(c>126)then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Nonstandard ASCII code has been blotted out.')
;end;tfm[j]:=63;end else if(c>=97)and(c<=122)then tfm[j]:=c-32;end;end;
{:52}{92:}procedure hashinput;label 30;var cc:0..3;zz:0..255;y:0..255;
key:integer;t:integer;begin if hashptr=hashsize then goto 30;{93:}
k:=4*(ligkernbase+(i));y:=tfm[k+1];t:=tfm[k+2];cc:=0;zz:=tfm[k+3];
if t>=128 then zz:=y else begin case t of 0,6:;5,11:zz:=y;1,7:cc:=1;
2:cc:=2;3:cc:=3;end;end{:93};key:=256*c+y+1;h:=(1009*key)mod hashsize;
while hash[h]>0 do begin if hash[h]<=key then begin if hash[h]=key then
goto 30;t:=hash[h];hash[h]:=key;key:=t;t:=class[h];class[h]:=cc;cc:=t;
t:=ligz[h];ligz[h]:=zz;zz:=t;end;if h>0 then h:=h-1 else h:=hashsize;
end;hash[h]:=key;class[h]:=cc;ligz[h]:=zz;hashptr:=hashptr+1;
hashlist[hashptr]:=h;30:end;{:92}{94:}
ifdef('notdef')function ffn(h,x,y:index):index;begin end;
endif('notdef')function eval(x,y:index):index;var key:integer;
begin key:=256*x+y+1;h:=(1009*key)mod hashsize;
while hash[h]>key do if h>0 then h:=h-1 else h:=hashsize;
if hash[h]<key then eval:=y else eval:=ffn(h,x,y);end;{:94}{95:}
function ffn(h,x,y:index):index;begin case class[h]of 0:;
1:begin class[h]:=4;ligz[h]:=eval(ligz[h],y);class[h]:=0;end;
2:begin class[h]:=4;ligz[h]:=eval(x,ligz[h]);class[h]:=0;end;
3:begin class[h]:=4;ligz[h]:=eval(eval(x,ligz[h]),y);class[h]:=0;end;
4:begin xligcycle:=x;yligcycle:=y;ligz[h]:=257;class[h]:=0;end;end;
ffn:=ligz[h];end;{:95}{96:}function organize:boolean;label 9999,30;
var tfmptr:index;begin{20:}read(tfmfile,tfm[0]);
if tfm[0]>127 then begin writeln(
'The first byte of the input file exceeds 127!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;if eof(tfmfile)then begin writeln(
'The input file is only one byte long!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;read(tfmfile,tfm[1]);lf:=tfm[0]*256+tfm[1];
if lf=0 then begin writeln(
'The file claims to have length zero, but that''s impossible!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;
tfmfilearray:=casttobytepointer(xrealloc(tfmfilearray,4*lf+1001));
for tfmptr:=2 to 4*lf-1 do begin if eof(tfmfile)then begin writeln(
'The file has fewer bytes than it claims!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;read(tfmfile,tfm[tfmptr]);end;
if not eof(tfmfile)then begin writeln(
'There''s some extra junk at the end of the TFM file,');
writeln('but I''ll proceed as if it weren''t there.');end{:20};{21:}
begin tfmptr:=2;begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;lh:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;bc:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;ec:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;nw:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;nh:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;nd:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;ni:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;nl:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;nk:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;ne:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
begin if tfm[tfmptr]>127 then begin writeln(
'One of the subfile sizes is negative!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;np:=tfm[tfmptr]*256+tfm[tfmptr+1];tfmptr:=tfmptr+2;end;
if lh<2 then begin writeln('The header length is only ',lh:1,'!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;if nl>4*ligsize then begin writeln(
'The lig/kern program is longer than I can handle!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;
if(bc>ec+1)or(ec>255)then begin writeln('The character code range ',bc:1
,'..',ec:1,'is illegal!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;if(nw=0)or(nh=0)or(nd=0)or(ni=0)then begin writeln(
'Incomplete subfiles for character dimensions!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;
if ne>256 then begin writeln('There are ',ne:1,' extensible recipes!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;
if lf<>6+lh+(ec-bc+1)+nw+nh+nd+ni+nl+nk+ne+np then begin writeln(
'Subfile sizes don''t add up to the stated total!');
writeln('Sorry, but I can''t go on; are you sure this is a TFM?');
goto 9999;end;end{:21};{23:}begin charbase:=6+lh-bc;
widthbase:=charbase+ec+1;heightbase:=widthbase+nw;
depthbase:=heightbase+nh;italicbase:=depthbase+nd;
ligkernbase:=italicbase+ni;kernbase:=ligkernbase+nl;
extenbase:=kernbase+nk;parambase:=extenbase+ne-1;end{:23};
organize:=true;goto 30;9999:organize:=false;30:end;{:96}{97:}
procedure dosimplethings;var i:0..32767;begin{48:}begin fonttype:=0;
if lh>=12 then begin{53:}begin checkBCPL(32,40);
if(tfm[32]>=11)and(tfm[33]=84)and(tfm[34]=69)and(tfm[35]=88)and(tfm[36]=
32)and(tfm[37]=77)and(tfm[38]=65)and(tfm[39]=84)and(tfm[40]=72)and(tfm[
41]=32)then begin if(tfm[42]=83)and(tfm[43]=89)then fonttype:=1 else if(
tfm[42]=69)and(tfm[43]=88)then fonttype:=2;end;end{:53};
if lh>=17 then begin{55:}left;write(plfile,'FAMILY');checkBCPL(72,20);
outBCPL(72);right{:55};if lh>=18 then{56:}begin left;
write(plfile,'FACE');outface(95);right;for i:=18 to lh-1 do begin left;
write(plfile,'HEADER D ',i:1);outoctal(24+4*i,4);right;end;end{:56};end;
{54:}left;write(plfile,'CODINGSCHEME');outBCPL(32);right{:54};end;{51:}
left;write(plfile,'DESIGNSIZE');
if tfm[28]>127 then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Design size ','negative','!');end;
writeln('I''ve set it to 10 points.');write(plfile,' D 10');
end else if(tfm[28]=0)and(tfm[29]<16)then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Design size ','too small','!');end;
writeln('I''ve set it to 10 points.');write(plfile,' D 10');
end else outfix(28);right;
write(plfile,'(COMMENT DESIGNSIZE IS IN POINTS)');outln;
write(plfile,'(COMMENT OTHER SIZES ARE MULTIPLES OF DESIGNSIZE)');
outln{:51};{49:}left;write(plfile,'CHECKSUM');outoctal(24,4);right{:49};
{57:}if(lh>17)and(tfm[92]>127)then begin left;
write(plfile,'SEVENBITSAFEFLAG TRUE');right;end{:57};end{:48};{58:}
if np>0 then begin left;write(plfile,'FONTDIMEN');outln;
for i:=1 to np do{60:}begin left;
if i=1 then write(plfile,'SLANT')else begin if(tfm[4*(parambase+i)]>0)
and(tfm[4*(parambase+i)]<255)then begin tfm[4*(parambase+i)]:=0;
tfm[(4*(parambase+i))+1]:=0;tfm[(4*(parambase+i))+2]:=0;
tfm[(4*(parambase+i))+3]:=0;begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Parameter',' ',i:1,' is too big;');end;
writeln('I have set it to zero.');end;{61:}
if i<=7 then case i of 2:write(plfile,'SPACE');
3:write(plfile,'STRETCH');4:write(plfile,'SHRINK');
5:write(plfile,'XHEIGHT');6:write(plfile,'QUAD');
7:write(plfile,'EXTRASPACE')end else if(i<=22)and(fonttype=1)then case i
of 8:write(plfile,'NUM1');9:write(plfile,'NUM2');
10:write(plfile,'NUM3');11:write(plfile,'DENOM1');
12:write(plfile,'DENOM2');13:write(plfile,'SUP1');
14:write(plfile,'SUP2');15:write(plfile,'SUP3');16:write(plfile,'SUB1');
17:write(plfile,'SUB2');18:write(plfile,'SUPDROP');
19:write(plfile,'SUBDROP');20:write(plfile,'DELIM1');
21:write(plfile,'DELIM2');
22:write(plfile,'AXISHEIGHT')end else if(i<=13)and(fonttype=2)then if i=
8 then write(plfile,'DEFAULTRULETHICKNESS')else write(plfile,
'BIGOPSPACING',i-8:1)else write(plfile,'PARAMETER D ',i:1){:61};end;
outfix(4*(parambase+i));right;end{:60};right;end;{59:}
if(fonttype=1)and(np<>22)then writeln(
'Unusual number of fontdimen parameters for a math symbols font (',np:1,
' not 22).')else if(fonttype=2)and(np<>13)then writeln(
'Unusual number of fontdimen parameters for an extension font (',np:1,
' not 13).'){:59};{:58};{62:}
if(tfm[4*widthbase]>0)or(tfm[4*widthbase+1]>0)or(tfm[4*widthbase+2]>0)or
(tfm[4*widthbase+3]>0)then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','width[0] should be zero.');end;
if(tfm[4*heightbase]>0)or(tfm[4*heightbase+1]>0)or(tfm[4*heightbase+2]>0
)or(tfm[4*heightbase+3]>0)then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','height[0] should be zero.');end;
if(tfm[4*depthbase]>0)or(tfm[4*depthbase+1]>0)or(tfm[4*depthbase+2]>0)or
(tfm[4*depthbase+3]>0)then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','depth[0] should be zero.');end;
if(tfm[4*italicbase]>0)or(tfm[4*italicbase+1]>0)or(tfm[4*italicbase+2]>0
)or(tfm[4*italicbase+3]>0)then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','italic[0] should be zero.');end;
for i:=0 to nw-1 do if(tfm[4*(widthbase+i)]>0)and(tfm[4*(widthbase+i)]<
255)then begin tfm[4*(widthbase+i)]:=0;tfm[(4*(widthbase+i))+1]:=0;
tfm[(4*(widthbase+i))+2]:=0;tfm[(4*(widthbase+i))+3]:=0;
begin perfect:=false;if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Width',' ',i:1,' is too big;');end;
writeln('I have set it to zero.');end;
for i:=0 to nh-1 do if(tfm[4*(heightbase+i)]>0)and(tfm[4*(heightbase+i)]
<255)then begin tfm[4*(heightbase+i)]:=0;tfm[(4*(heightbase+i))+1]:=0;
tfm[(4*(heightbase+i))+2]:=0;tfm[(4*(heightbase+i))+3]:=0;
begin perfect:=false;if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Height',' ',i:1,' is too big;');end;
writeln('I have set it to zero.');end;
for i:=0 to nd-1 do if(tfm[4*(depthbase+i)]>0)and(tfm[4*(depthbase+i)]<
255)then begin tfm[4*(depthbase+i)]:=0;tfm[(4*(depthbase+i))+1]:=0;
tfm[(4*(depthbase+i))+2]:=0;tfm[(4*(depthbase+i))+3]:=0;
begin perfect:=false;if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Depth',' ',i:1,' is too big;');end;
writeln('I have set it to zero.');end;
for i:=0 to ni-1 do if(tfm[4*(italicbase+i)]>0)and(tfm[4*(italicbase+i)]
<255)then begin tfm[4*(italicbase+i)]:=0;tfm[(4*(italicbase+i))+1]:=0;
tfm[(4*(italicbase+i))+2]:=0;tfm[(4*(italicbase+i))+3]:=0;
begin perfect:=false;if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Italic correction',' ',i:1,' is too big;');
end;writeln('I have set it to zero.');end;
if nk>0 then for i:=0 to nk-1 do if(tfm[4*(kernbase+i)]>0)and(tfm[4*(
kernbase+i)]<255)then begin tfm[4*(kernbase+i)]:=0;
tfm[(4*(kernbase+i))+1]:=0;tfm[(4*(kernbase+i))+2]:=0;
tfm[(4*(kernbase+i))+3]:=0;begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Kern',' ',i:1,' is too big;');end;
writeln('I have set it to zero.');end;{:62}end;{:97}{98:}
procedure docharacters;var c:byte;k:index;ai:0..ligsize;begin{78:}
sortptr:=0;
for c:=bc to ec do if tfm[4*(charbase+c)]>0 then begin if charsonline=8
then begin writeln(' ');charsonline:=1;
end else begin if charsonline>0 then write(' ');
if verbose then charsonline:=charsonline+1;end;
if verbose then printoctal(c);left;write(plfile,'CHARACTER');outchar(c);
outln;{79:}begin left;write(plfile,'CHARWD');
if tfm[4*(charbase+c)]>=nw then begin perfect:=false;writeln(' ');
write('Width',' index for character ');printoctal(c);
writeln(' is too large;');writeln('so I reset it to zero.');
end else outfix(4*(widthbase+tfm[4*(charbase+c)]));right;end{:79};
if(tfm[4*(charbase+c)+1]div 16)>0 then{80:}
if(tfm[4*(charbase+c)+1]div 16)>=nh then begin perfect:=false;
writeln(' ');write('Height',' index for character ');printoctal(c);
writeln(' is too large;');writeln('so I reset it to zero.');
end else begin left;write(plfile,'CHARHT');
outfix(4*(heightbase+(tfm[4*(charbase+c)+1]div 16)));right;end{:80};
if(tfm[4*(charbase+c)+1]mod 16)>0 then{81:}
if(tfm[4*(charbase+c)+1]mod 16)>=nd then begin perfect:=false;
writeln(' ');write('Depth',' index for character ');printoctal(c);
writeln(' is too large;');writeln('so I reset it to zero.');
end else begin left;write(plfile,'CHARDP');
outfix(4*(depthbase+(tfm[4*(charbase+c)+1]mod 16)));right;end{:81};
if(tfm[4*(charbase+c)+2]div 4)>0 then{82:}
if(tfm[4*(charbase+c)+2]div 4)>=ni then begin perfect:=false;
writeln(' ');write('Italic correction',' index for character ');
printoctal(c);writeln(' is too large;');
writeln('so I reset it to zero.');end else begin left;
write(plfile,'CHARIC');
outfix(4*(italicbase+(tfm[4*(charbase+c)+2]div 4)));right;end{:82};
case(tfm[4*(charbase+c)+2]mod 4)of 0:;1:{83:}begin left;
write(plfile,'COMMENT');outln;i:=tfm[4*(charbase+c)+3];
r:=4*(ligkernbase+(i));if tfm[r]>128 then i:=256*tfm[r+2]+tfm[r+3];
repeat{74:}begin k:=4*(ligkernbase+(i));
if tfm[k]>128 then begin if 256*tfm[k+2]+tfm[k+3]>=nl then begin perfect
:=false;if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ',
'Ligature unconditional stop command address is too big.');end;
end else if tfm[k+2]>=128 then{76:}
begin if((tfm[k+1]<bc)or(tfm[k+1]>ec)or(tfm[4*(charbase+tfm[k+1])]=0))
then if tfm[k+1]<>boundarychar then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
write('Bad TFM file: ','Kern step for',' nonexistent character ');
printoctal(tfm[k+1]);writeln('.');tfm[k+1]:=bc;end;left;
write(plfile,'KRN');outchar(tfm[k+1]);r:=256*(tfm[k+2]-128)+tfm[k+3];
if r>=nk then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Kern index too large.');end;
write(plfile,' R 0.0');end else outfix(4*(kernbase+r));right;end{:76}
else{77:}
begin if((tfm[k+1]<bc)or(tfm[k+1]>ec)or(tfm[4*(charbase+tfm[k+1])]=0))
then if tfm[k+1]<>boundarychar then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
write('Bad TFM file: ','Ligature step for',' nonexistent character ');
printoctal(tfm[k+1]);writeln('.');tfm[k+1]:=bc;end;
if((tfm[k+3]<bc)or(tfm[k+3]>ec)or(tfm[4*(charbase+tfm[k+3])]=0))then
begin perfect:=false;if charsonline>0 then writeln(' ');charsonline:=0;
write('Bad TFM file: ','Ligature step produces the',
' nonexistent character ');printoctal(tfm[k+3]);writeln('.');
tfm[k+3]:=bc;end;left;r:=tfm[k+2];
if(r=4)or((r>7)and(r<>11))then begin writeln(
'Ligature step with nonstandard code changed to LIG');r:=0;tfm[k+2]:=0;
end;if r mod 4>1 then write(plfile,'/');write(plfile,'LIG');
if odd(r)then write(plfile,'/');while r>3 do begin write(plfile,'>');
r:=r-4;end;outchar(tfm[k+1]);outchar(tfm[k+3]);right;end{:77};
if tfm[k]>0 then if level=1 then{75:}
begin if tfm[k]>=128 then write(plfile,'(STOP)')else begin count:=0;
for ai:=i+1 to i+tfm[k]do if activity[ai]=2 then count:=count+1;
write(plfile,'(SKIP D ',count:1,')');end;outln;end{:75};end{:74};
if tfm[k]>=128 then i:=nl else i:=i+1+tfm[k];until i>=nl;right;end{:83};
2:{84:}begin r:=tfm[4*(charbase+c)+3];
if((r<bc)or(r>ec)or(tfm[4*(charbase+r)]=0))then begin begin perfect:=
false;if charsonline>0 then writeln(' ');charsonline:=0;
write('Bad TFM file: ','Character list link to',
' nonexistent character ');printoctal(r);writeln('.');end;
tfm[4*(charbase+c)+2]:=4*(tfm[4*(charbase+c)+2]div 4)+0;
end else begin while(r<c)and((tfm[4*(charbase+r)+2]mod 4)=2)do r:=tfm[4*
(charbase+r)+3];if r=c then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Cycle in a character list!');end;
write('Character ');printoctal(c);writeln(' now ends the list.');
tfm[4*(charbase+c)+2]:=4*(tfm[4*(charbase+c)+2]div 4)+0;
end else begin left;write(plfile,'NEXTLARGER');
outchar(tfm[4*(charbase+c)+3]);right;end;end;end{:84};3:{85:}
if tfm[4*(charbase+c)+3]>=ne then begin begin perfect:=false;
writeln(' ');write('Extensible',' index for character ');printoctal(c);
writeln(' is too large;');writeln('so I reset it to zero.');end;
tfm[4*(charbase+c)+2]:=4*(tfm[4*(charbase+c)+2]div 4)+0;
end else begin left;write(plfile,'VARCHAR');outln;{86:}
for k:=0 to 3 do if(k=3)or(tfm[4*(extenbase+tfm[4*(charbase+c)+3])+k]>0)
then begin left;case k of 0:write(plfile,'TOP');1:write(plfile,'MID');
2:write(plfile,'BOT');3:write(plfile,'REP')end;
if((tfm[4*(extenbase+tfm[4*(charbase+c)+3])+k]<bc)or(tfm[4*(extenbase+
tfm[4*(charbase+c)+3])+k]>ec)or(tfm[4*(charbase+tfm[4*(extenbase+tfm[4*(
charbase+c)+3])+k])]=0))then outchar(c)else outchar(tfm[4*(extenbase+tfm
[4*(charbase+c)+3])+k]);right;end{:86};right;end{:85};end;right;end{:78}
;end;{:98}{99:}begin initialize;if not organize then goto 9999;
dosimplethings;{66:}
if nl>0 then begin for ai:=0 to nl-1 do activity[ai]:=0;{69:}
if tfm[4*(ligkernbase+(0))]=255 then begin left;
write(plfile,'BOUNDARYCHAR');boundarychar:=tfm[4*(ligkernbase+(0))+1];
outchar(boundarychar);right;activity[0]:=1;end;
if tfm[4*(ligkernbase+(nl-1))]=255 then begin r:=256*tfm[4*(ligkernbase+
(nl-1))+2]+tfm[4*(ligkernbase+(nl-1))+3];
if r>=nl then begin perfect:=false;writeln(' ');
write('Ligature/kern starting index for boundarychar is too large;');
writeln('so I removed it.');end else begin labelptr:=1;
labeltable[1].cc:=256;labeltable[1].rr:=r;bcharlabel:=r;activity[r]:=2;
end;activity[nl-1]:=1;end{:69};end;{67:}
for c:=bc to ec do if(tfm[4*(charbase+c)+2]mod 4)=1 then begin r:=tfm[4*
(charbase+c)+3];
if r<nl then begin if tfm[4*(ligkernbase+(r))]>128 then begin r:=256*tfm
[4*(ligkernbase+(r))+2]+tfm[4*(ligkernbase+(r))+3];
if r<nl then if activity[tfm[4*(charbase+c)+3]]=0 then activity[tfm[4*(
charbase+c)+3]]:=1;end;end;if r>=nl then begin perfect:=false;
writeln(' ');write('Ligature/kern starting index for character ');
printoctal(c);writeln(' is too large;');writeln('so I removed it.');
tfm[4*(charbase+c)+2]:=4*(tfm[4*(charbase+c)+2]div 4)+0;end else{68:}
begin sortptr:=labelptr;
while labeltable[sortptr].rr>r do begin labeltable[sortptr+1]:=
labeltable[sortptr];sortptr:=sortptr-1;end;labeltable[sortptr+1].cc:=c;
labeltable[sortptr+1].rr:=r;labelptr:=labelptr+1;activity[r]:=2;end{:68}
;end;labeltable[labelptr+1].rr:=ligsize;{:67};if nl>0 then begin left;
write(plfile,'LIGTABLE');outln;{70:}
for ai:=0 to nl-1 do if activity[ai]=2 then begin r:=tfm[4*(ligkernbase+
(ai))];if r<128 then begin r:=r+ai+1;
if r>=nl then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Ligature/kern step ',ai:1,' skips too far;');
end;writeln('I made it stop.');tfm[4*(ligkernbase+(ai))]:=128;
end else activity[r]:=2;end;end{:70};{71:}sortptr:=1;
for acti:=0 to nl-1 do if activity[acti]<>1 then begin i:=acti;{73:}
if activity[i]=0 then begin if level=1 then begin left;
write(plfile,'COMMENT THIS PART OF THE PROGRAM IS NEVER USED!');outln;
end end else if level=2 then right{:73};{72:}
while i=labeltable[sortptr].rr do begin left;write(plfile,'LABEL');
if labeltable[sortptr].cc=256 then write(plfile,' BOUNDARYCHAR')else
outchar(labeltable[sortptr].cc);right;sortptr:=sortptr+1;end{:72};{74:}
begin k:=4*(ligkernbase+(i));
if tfm[k]>128 then begin if 256*tfm[k+2]+tfm[k+3]>=nl then begin perfect
:=false;if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ',
'Ligature unconditional stop command address is too big.');end;
end else if tfm[k+2]>=128 then{76:}
begin if((tfm[k+1]<bc)or(tfm[k+1]>ec)or(tfm[4*(charbase+tfm[k+1])]=0))
then if tfm[k+1]<>boundarychar then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
write('Bad TFM file: ','Kern step for',' nonexistent character ');
printoctal(tfm[k+1]);writeln('.');tfm[k+1]:=bc;end;left;
write(plfile,'KRN');outchar(tfm[k+1]);r:=256*(tfm[k+2]-128)+tfm[k+3];
if r>=nk then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
writeln('Bad TFM file: ','Kern index too large.');end;
write(plfile,' R 0.0');end else outfix(4*(kernbase+r));right;end{:76}
else{77:}
begin if((tfm[k+1]<bc)or(tfm[k+1]>ec)or(tfm[4*(charbase+tfm[k+1])]=0))
then if tfm[k+1]<>boundarychar then begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
write('Bad TFM file: ','Ligature step for',' nonexistent character ');
printoctal(tfm[k+1]);writeln('.');tfm[k+1]:=bc;end;
if((tfm[k+3]<bc)or(tfm[k+3]>ec)or(tfm[4*(charbase+tfm[k+3])]=0))then
begin perfect:=false;if charsonline>0 then writeln(' ');charsonline:=0;
write('Bad TFM file: ','Ligature step produces the',
' nonexistent character ');printoctal(tfm[k+3]);writeln('.');
tfm[k+3]:=bc;end;left;r:=tfm[k+2];
if(r=4)or((r>7)and(r<>11))then begin writeln(
'Ligature step with nonstandard code changed to LIG');r:=0;tfm[k+2]:=0;
end;if r mod 4>1 then write(plfile,'/');write(plfile,'LIG');
if odd(r)then write(plfile,'/');while r>3 do begin write(plfile,'>');
r:=r-4;end;outchar(tfm[k+1]);outchar(tfm[k+3]);right;end{:77};
if tfm[k]>0 then if level=1 then{75:}
begin if tfm[k]>=128 then write(plfile,'(STOP)')else begin count:=0;
for ai:=i+1 to i+tfm[k]do if activity[ai]=2 then count:=count+1;
write(plfile,'(SKIP D ',count:1,')');end;outln;end{:75};end{:74};end;
if level=2 then right{:71};right;{90:}hashptr:=0;yligcycle:=256;
for hh:=0 to hashsize do hash[hh]:=0;
for c:=bc to ec do if(tfm[4*(charbase+c)+2]mod 4)=1 then begin i:=tfm[4*
(charbase+c)+3];
if tfm[4*(ligkernbase+(i))]>128 then i:=256*tfm[4*(ligkernbase+(i))+2]+
tfm[4*(ligkernbase+(i))+3];{91:}repeat hashinput;
k:=tfm[4*(ligkernbase+(i))];if k>=128 then i:=nl else i:=i+1+k;
until i>=nl{:91};end;if bcharlabel<nl then begin c:=256;i:=bcharlabel;
{91:}repeat hashinput;k:=tfm[4*(ligkernbase+(i))];
if k>=128 then i:=nl else i:=i+1+k;until i>=nl{:91};end;
if hashptr=hashsize then begin writeln(
'Sorry, I haven''t room for so many ligature/kern pairs!');goto 9999;
end;for hh:=1 to hashptr do begin r:=hashlist[hh];
if class[r]>0 then r:=ffn(r,(hash[r]-1)div 256,(hash[r]-1)mod 256);end;
if yligcycle<256 then begin write(
'Infinite ligature loop starting with ');
if xligcycle=256 then write('boundary')else printoctal(xligcycle);
write(' and ');printoctal(yligcycle);writeln('!');
write(plfile,'(INFINITE LIGATURE LOOP MUST BE BROKEN!)');goto 9999;
end{:90};end{:66};{87:}
if ne>0 then for c:=0 to ne-1 do for d:=0 to 3 do begin k:=4*(extenbase+
c)+d;if(tfm[k]>0)or(d=3)then begin if((tfm[k]<bc)or(tfm[k]>ec)or(tfm[4*(
charbase+tfm[k])]=0))then begin begin perfect:=false;
if charsonline>0 then writeln(' ');charsonline:=0;
write('Bad TFM file: ','Extensible recipe involves the',
' nonexistent character ');printoctal(tfm[k]);writeln('.');end;
if d<3 then tfm[k]:=0;end;end;end{:87};docharacters;
if verbose then writeln('.');
if level<>0 then writeln('This program isn''t working!');
if not perfect then write(plfile,
'(COMMENT THE TFM FILE WAS BAD, SO THE DATA HAS BEEN CHANGED!)');
9999:end.{:99}
